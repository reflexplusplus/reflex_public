#include "[require].h"

REFLEX_BEGIN_INTERNAL(Reflex::System::Linux)

struct HttpImpl : public HttpConnection
{
	struct ClientData
	{
		const ReceiveHeaderFn & receive_headers;
		const ReceiveDataFn & receive_data;
		bool aborted = false;
	};

	HttpImpl(CString && url_base);

	void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) override;

	Response Request(const CString::View & method_view, const CString::View & resource, const ArrayView < Pair <CString> > & headers, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_headers, const ReceiveDataFn & receive_data) override;


	static size_t ReceiveHeaders(void * userdata, size_t size, size_t nitems, void * clientp)
	{
		auto & ctx = *Reinterpret<ClientData>(clientp);

		auto total = size * nitems;

		CString::View line = { Reinterpret<char>(userdata), UInt(total) };

		line = TrimRight(line);

		if (auto pos = Search(line, ':'))
		{
			auto item = Splice(line, pos.value);
			
			auto key = Trim(item.a);
			auto value = Trim(Mid(item.b, 1));

			ctx.receive_headers(key, value);
		}

		return total;
	}

	static size_t ReceiveData(void * userdata, size_t size, size_t nitems, void * clientp)
	{
		auto & ctx = *Reinterpret<ClientData>(clientp);

		auto total = size * nitems;

		if (ctx.receive_data({ Reinterpret<UInt8>(userdata), UInt(total) }))
		{
			return total;
		}

		ctx.aborted = true;

		return 0;
	}


	const CString m_url_base;
	Float m_connect_timeout = kDefaultConnectionTimeout;
	Float m_read_timeout = kDefaultReadTimeout;

	static constexpr CString::View kHeaderJoin = ": ";
};

HttpImpl::HttpImpl(CString && url_base)
	: m_url_base(std::move(url_base))
{
}

void HttpImpl::SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds)
{
	if (connect_timeout_seconds > read_timeout_seconds)
	{
		DEV_WARN("System::Http: connection timeout cannot be bigger than read timeout.");
	}

	m_connect_timeout = Min(connect_timeout_seconds, read_timeout_seconds);
	m_read_timeout = read_timeout_seconds;
}

HttpConnection::Response HttpImpl::Request(const CString::View & method_view, const CString::View & resource, const ArrayView < Pair <CString> > & headers, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_headers, const ReceiveDataFn & receive_data)
{
	auto curl = curl_easy_init();

	auto full_url = Join(m_url_base, resource);

	ClientData client_data = { receive_headers, receive_data, false };

	curl_slist * curl_headers = nullptr;

	curl_easy_setopt(curl, CURLOPT_URL, full_url.GetData());
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, long(m_connect_timeout * 1000.0f));
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, long(m_read_timeout * 1000.0f));
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &HttpImpl::ReceiveHeaders);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &client_data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HttpImpl::ReceiveData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &client_data);
	//curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

	if (CaseInsensitive::eq(method_view, kHEAD))
	{
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	}
	else if (CaseInsensitive::eq(method_view, kPOST))
	{
		curl_easy_setopt(curl, CURLOPT_POST, 1L);

		if (body)
		{
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data);
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t(body.size));
	}
	else if (!CaseInsensitive::eq(method_view, kGET))
	{
		CString method = method_view;

		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.GetData());

		if (body)
		{
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t(body.size));
		}
	}

	for (auto & [key,value] : headers)
	{
		auto line = Join(key, kHeaderJoin, value);

		curl_headers = curl_slist_append(curl_headers, line.GetData());
	}

	if (curl_headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

	auto result = curl_easy_perform(curl);

	long response_code = 0;

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	if (curl_headers) curl_slist_free_all(curl_headers);
	curl_easy_cleanup(curl);

	if (client_data.aborted)
	{
		return kResponseAborted;
	}
	else if (result != CURLE_OK)
	{
		return kResponseNoConnection;
	}

	return Response(response_code);
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::HttpConnection> Reflex::System::HttpConnection::Create(bool https, const CString::View & server, UInt16 port)
{
	return REFLEX_CREATE(Linux::HttpImpl, Common::MakeUrlBase(https, server, port));
}