#include "../../../include/reflex_ext/async/http.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::Async::Detail)

AtomicUInt32 g_num_http_requests = 0;

struct HttpImpl : public System::HttpConnection::Client
{
	HttpImpl(Worker::Context & ctx, HttpRequestCallbacks & callbacks, UInt32 byte_rate)
		: ctx(ctx)
		, callbacks(callbacks)
		, byte_rate(byte_rate)
	{
	}

	bool OnResponse(UInt code) override
	{
		status_code = code;
		return !ctx.Cancelled() && callbacks.OnResponse(code);
	}

	void OnHeader(CString::View key, CString::View value) override
	{
		if (CaseInsensitive::eq(key, "content-length"))
		{
			if (auto bytes = ToUInt32(value))
			{
				progress_per_byte = 1.0f / Float32(bytes);
			}
		}

		callbacks.OnHeader(key, value);
	}

	bool OnChunk(Data::Archive::View chunk) override
	{
		if (ctx.Cancelled()) return false;

		bytes_received += chunk.size;

		if (byte_rate)
		{
			auto elapsed = System::GetElapsedTime() - start_time;
			auto active = REFLEX_ATOMIC_READ(g_num_http_requests);

			REFLEX_ASSERT(active);

			auto max_byte_rate_per_thread = Float64(byte_rate) / active;
			auto allowed = max_byte_rate_per_thread * elapsed;

			if (bytes_received > allowed)
			{
				auto sleep_ms = Truncate((bytes_received - allowed) * 1000.0 / max_byte_rate_per_thread);

				while (sleep_ms)
				{
					auto sleep_chunk = Min(sleep_ms, 25);

					System::SuspendThread(sleep_chunk);

					sleep_ms -= sleep_chunk;

					if (ctx.Cancelled()) return false;
				}
			}
		}

		callbacks.OnChunk(chunk);

		ctx.SetProgress(Min(Float32(bytes_received) * progress_per_byte, 1.0f));

		return !ctx.Cancelled();
	}

	Worker::Context & ctx;
	HttpRequestCallbacks & callbacks;
	const UInt32 byte_rate;
	const Float64 start_time = System::GetElapsedTime();
	UInt status_code = 0;
	Float32 progress_per_byte = 0;
	UInt64 bytes_received = 0;
};

REFLEX_END_INTERNAL

Reflex::Async::Detail::StandardHttpRequestCallbacks::StandardHttpRequestCallbacks(UInt8 decode_json_flags)
	: m_decode_json_flags(decode_json_flags)
{
}

void Reflex::Async::Detail::StandardHttpRequestCallbacks::OnHeader(const CString::View & key, const CString::View & value)
{
	if (CaseInsensitive::eq(key, "content-encoding"))
	{
		m_is_lz4 = CaseInsensitive::eq(value, "lz4");
	}
	else if (CaseInsensitive::eq(key, "content-type"))
	{
		m_is_json = True(Search(value, "application/json"));
	}
}

void Reflex::Async::Detail::StandardHttpRequestCallbacks::OnChunk(const Data::Archive::View & chunk)
{
	m_body.Append(chunk);
}

Reflex::Unretained <Reflex::Object> Reflex::Async::Detail::StandardHttpRequestCallbacks::OnComplete(System::HttpConnection::Result result)
{
	if (result == System::HttpConnection::kResultCompleted)
	{
		if (m_is_lz4 && m_body) m_body = Data::Decompress(Data::kLZ4, m_body);

		if (m_is_json)
		{
			Data::PropertySet options;
			Data::SetUInt32(options, "flags", m_decode_json_flags);
			return New<Data::PropertySet>(Data::DecodePropertySet(Data::kJsonFormat, m_body, options));
		}
		else
		{
			return New<Data::ArchiveObject>(m_body);
		}
	}
	else
	{
		return {};
	}
}

Reflex::Async::Worker::Result Reflex::Async::Detail::Fetch(Worker::Context & ctx, CString::View method, CString::View url, HttpHeaders::View headers, Data::Archive::View body, HttpRequestCallbacks & callbacks, NetworkSimulation network_simulation, Output & output, UInt retry_count)
{
	REFLEX_ATOMIC_INC(g_num_http_requests, 1);

	using Http = System::HttpConnection;

	auto url_parts = Data::SplitUrl(url);

	const bool simulate_response = network_simulation < kNetworkSimulation384k;
	const UInt32 byte_rate = network_simulation == kNetworkSimulationNone || simulate_response ? 0 : network_simulation;

	Retained <Http> connection = Http::null;
	if (!simulate_response && !ctx.Cancelled()) connection = callbacks.Connect(Data::IsHttps(url_parts.scheme), url_parts.domain, url_parts.port);

	Worker::Result result;

	for (UInt retries = retry_count; !ctx.Cancelled(); --retries)
	{
		auto client = Make<HttpImpl>(ctx, callbacks, byte_rate);
		auto outcome = Http::kResultTransportError;

		if (simulate_response)
		{
			System::SuspendThread(125);

			if (network_simulation != kNetworkSimulationNoConnection)
			{
				outcome = client->OnResponse(UInt(network_simulation)) ? Http::kResultCompleted : Http::kResultRejected;
			}
		}
		else
		{
			outcome = connection->Request(method, url_parts.resource, headers, body, client);
		}

		if (outcome == Http::kResultAborted || ctx.Cancelled())
		{
			output.LogEx(kLogError, {}, "[ABORTED] ", url_parts.resource);
			break;
		}

		auto elapsed_ms = (System::GetElapsedTime() - client->start_time) * 1000.0;
		auto status_code = client->status_code;

		bool retry = false;
		auto log_type = kLogNormal;
		bool completed = outcome == Http::kResultCompleted;

		if (completed)
		{
			ctx.SetProgress(1.0f);
		}
		else
		{
			// Once a response has been accepted, the callbacks may hold partial data.
			retry = retries && (outcome == Http::kResultRejected || !status_code);
			log_type = retry ? kLogWarning : kLogError;
		}

		output.LogEx(log_type, {}, '[', status_code, "] ", url_parts.resource, " (", elapsed_ms, "ms)");

		if (!retry)
		{
			result = { completed, callbacks.OnComplete(outcome) };
			break;
		}
	}

	REFLEX_ATOMIC_DEC(g_num_http_requests, 1);

	return result;
}
