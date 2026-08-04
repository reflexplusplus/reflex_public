#include "../../../include/reflex_ext/async/http.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::Async)

AtomicUInt32 g_num_http_requests = 0;

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

Reflex::TRef <Reflex::Object> Reflex::Async::Detail::StandardHttpRequestCallbacks::OnComplete()
{
	if (m_is_lz4 && m_body) m_body = Data::Decompress(Data::kLZ4, m_body);

	if (m_is_json)
	{
		return New<Data::PropertySet>(Data::DecodePropertySet(Data::kJsonFormat, m_body, m_decode_json_flags));
	}
	else
	{
		return New<Data::ArchiveObject>(m_body);
	}
}

Reflex::Async::Worker::Result Reflex::Async::Detail::Fetch(Worker::Context & ctx, CString::View method, CString::View url, HttpHeaders::View headers, Data::Archive::View body, HttpRequestCallbacks & callbacks, NetworkSimulation network_simulation, Output & output)
{
	REFLEX_ATOMIC_INC(g_num_http_requests, 1);

	auto url_parts = Data::SplitUrl(url);

	auto connection = AutoRelease(callbacks.Connect(Data::IsHttps(url_parts.scheme), url_parts.domain, url_parts.port));

	UInt retry = 3;

	Pair <Float32, UInt32> progress;

	auto receive_headers = [&callbacks, &progress](const CString::View & key, const CString::View & value)
	{
		if (CaseInsensitive::eq(key, "content-length"))
		{
			if (auto bytes = ToUInt32(value))
			{
				progress.a = 1.0f / Float32(bytes);
			}
		}

		callbacks.OnHeader(key, value);
	};

	REFLEX_MARKER(Try);

	progress = {};

	System::HttpConnection::ReceiveDataFn receive_data = [&callbacks, &ctx, &progress](const ArrayView <UInt8> & chunk) mutable
	{
		callbacks.OnChunk(chunk);

		progress.b += chunk.size;

		ctx.SetProgress(Min(Float32(progress.b) * progress.a, 1.0f));

		return !ctx.Cancelled();
	};

	auto start_time = System::GetElapsedTime();

	System::HttpConnection::Response code;

	if (network_simulation != kNetworkSimulationNone)
	{
		if (network_simulation < kNetworkSimulation384k)
		{
			System::SuspendThread(125);

			code = System::HttpConnection::Response(network_simulation);

			goto Complete;
		}
		else
		{
			receive_data = [receive_data, max_byte_rate = Float64(network_simulation), start_time, bytes_received = UInt32(0)](const ArrayView<UInt8> & chunk) mutable
			{
				if (max_byte_rate)
				{
					bytes_received += chunk.size;

					auto elapsed = System::GetElapsedTime() - start_time;

					auto active = REFLEX_ATOMIC_READ(g_num_http_requests);

					REFLEX_ASSERT(active);

					auto max_byte_rate_per_thread = (max_byte_rate / active);

					auto allowed = max_byte_rate_per_thread * elapsed;

					if (bytes_received > allowed)
					{
						auto sleep_ms = Truncate((bytes_received - allowed) * 1000.0 / max_byte_rate_per_thread);

						System::SuspendThread(sleep_ms);
					}

					return receive_data(chunk);
				}
				else
				{
					System::SuspendThread(250);

					return false;
				}
			};
		}
	}

	code = connection->Request(method, url_parts.resource, headers, body, receive_headers, receive_data);

	Complete:

	auto delta = SetDelta(start_time, System::GetElapsedTime());

	auto log = [&output, &url_parts, delta](System::HttpConnection::Response code, LogType type)
	{
		output.LogEx(type, {}, '[', UInt(code), "] ", url_parts.resource, " (", delta * 1000.0f, "ms)");
	};

	Worker::Result rtn;

	switch (code)
	{
	case System::HttpConnection::kResponseAborted:
		File::output.LogEx(kLogError, {}, "[ABORTED] ", url_parts.resource);
		break;

	default:
		if (callbacks.OnResponse(code))
		{
			log(code, kLogNormal);
			ctx.SetProgress(1.0f);
			rtn = { true, callbacks.OnComplete() };
			break;
		}
		else
		{
			log(code, retry ? kLogWarning : kLogError);
			if (retry--) goto Try;
		}
	}

	REFLEX_ATOMIC_DEC(g_num_http_requests, 1);

	return rtn;
}
