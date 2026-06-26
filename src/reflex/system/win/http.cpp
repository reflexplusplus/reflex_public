#include "[require].h"

#pragma comment (lib, "winhttp.lib")




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct HttpImpl : public HttpConnection
{
	static constexpr DWORD kOptionReceiveTimeout = WINHTTP_OPTION_RECEIVE_TIMEOUT;


	HttpImpl(bool https, const CString::View & hostname, UInt16 port);

	~HttpImpl();

	void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) override;

	Response Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers_in, const ArrayView <UInt8> & postbody, const ReceiveHeaderFn & receive_headers, const ReceiveDataFn & receive_data) override;


	static HINTERNET Initialise();

	static Tuple <CString::View, UInt16> ExtractPort(const CString::View & url);

	static void AppendAsWChar(ArrayRegion <UInt8> & buffer, const CString::View & text);

	void ParseHeaders(const ArrayView <UInt8> & headers, const ReceiveHeaderFn & callback);


	bool m_https;

	UInt16 m_port;

	HINTERNET m_connection;


	static constexpr DWORD kFlags = WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_BYPASS_PROXY_CACHE;

	static constexpr UInt16 kDefaultPorts[] = { UInt16(INTERNET_DEFAULT_HTTP_PORT), UInt16(INTERNET_DEFAULT_HTTPS_PORT) };
};

HttpImpl::HttpImpl(bool https, const CString::View & hostname, UInt16 port)
	: m_https(https)
	, m_port(port ? port : kDefaultPorts[https])
{
	m_connection = WinHttpConnect(Initialise(), ToWString(hostname).GetData(), m_port, NULL);

	SetTimeout(kDefaultConnectionTimeout, kDefaultReadTimeout);
}

HttpImpl::~HttpImpl()
{
	WinHttpCloseHandle(m_connection);
}

void HttpImpl::SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds)
{
	if (Library::st_internet_ready)//deferred init is because can not use wininet until after main
	{
		auto internet_handle = Initialise();

		WinHttpSetTimeouts(internet_handle, 0, Truncate(connect_timeout_seconds * 1000.0f), 0, Truncate(read_timeout_seconds * 1000.0f));
	}
}

HttpConnection::Response HttpImpl::Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers_in, const ArrayView <UInt8> & body_, const ReceiveHeaderFn & receive_headers, const ReceiveDataFn & receive_data)
{
	REFLEX_ASSERT(Library::st_internet_ready);

	DWORD code = kResponseNoConnection;

	WString verb = ToWString(method);

	if (HINTERNET handle = WinHttpOpenRequest(m_connection, verb.GetData(), ToWString(resource).GetData(), 0, WINHTTP_NO_REFERER, 0, kFlags | (m_https ? WINHTTP_FLAG_SECURE : 0)))
	{
		UInt8 workspace[8192];

		ArrayRegion <UInt8> buffer = ToRegion(workspace);

		ArrayRegion <UInt8> wheaders = buffer;

		if (headers_in)
		{
			constexpr CString::View join(": ");

			constexpr CString::View eol("\r\n");

			for (auto & [key,value] : headers_in)
			{
				AppendAsWChar(wheaders, key);
				AppendAsWChar(wheaders, join);
				AppendAsWChar(wheaders, value);
				AppendAsWChar(wheaders, eol);
			}
		}

		*Reinterpret<WChar>(wheaders.data) = 0;

		auto body = body_;

		if (WinHttpSendRequest(handle, Reinterpret<WChar>(buffer.data), DWORD(-1L), WINHTTP_NO_REQUEST_DATA, 0, body.size, 0))
		{
			//post

			constexpr UInt kMaxChunkSize = 1024 * 64;

			while (body)
			{
				DWORD chunksize = Min(body.size, kMaxChunkSize);

				if (!WinHttpWriteData(handle, body.data, chunksize, &chunksize))
				{
					goto Done;
				}

				body = Nudge(body, chunksize);
			}

			WinHttpReceiveResponse(handle, 0);



			//headers

			DWORD size = 8192;

			if (receive_headers)
			{
				do
				{
					if (!WinHttpQueryHeaders(handle, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data, &size, NULL))
					{
						size = 0;
					}
				}
				while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

				ParseHeaders({ buffer.data, UInt(size) }, receive_headers);
			}



			//response

			DWORD codesize = sizeof(DWORD);

			WinHttpQueryHeaders(handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &codesize, WINHTTP_NO_HEADER_INDEX);



			//data

			if (receive_data)
			{
				size = 0;

				while (WinHttpQueryDataAvailable(handle, &size))
				{
					if (size = Min<DWORD>(size, buffer.size))
					{
						if (WinHttpReadData(handle, buffer.data, size, &size))
						{
							if (receive_data({ buffer.data, size }))
							{
								continue;
							}
							else
							{
								code = DWORD(kResponseAborted);
							}
						}
						else
						{
							code = DWORD(kResponseNoConnection);
						}
					}

					break;
				}

				if (GetLastError() == ERROR_WINHTTP_TIMEOUT)
				{
					DEV_LOG("Request read timeout");

					code = DWORD(kResponseNoConnection);
				}
			}
		}
		else if (GetLastError() == ERROR_WINHTTP_TIMEOUT)
		{
			DEV_LOG("Connection timeout");
		}

		REFLEX_MARKER(Done);

		WinHttpCloseHandle(handle);
	}

	return Response(code);
}

HINTERNET HttpImpl::Initialise()
{
	auto & internet_handle = globals->m_internet_handle;

	if (!internet_handle)
	{
		internet_handle = WinHttpOpen(0, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, 0, 0, 0);

		globals->m_internet_deinit = &WinHttpCloseHandle;

		DWORD redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;

		[[maybe_unused]] auto ok = WinHttpSetOption(internet_handle, WINHTTP_OPTION_REDIRECT_POLICY, &redirect_policy, sizeof(DWORD));

		REFLEX_ASSERT(ok);
	}

	return internet_handle;
}

void HttpImpl::AppendAsWChar(ArrayRegion <UInt8> & buffer, const CString::View & text)
{
	for (auto & i : text)
	{
		*Reinterpret<WChar>(buffer.data) = WChar(i);

		buffer = Nudge(buffer, sizeof(WChar));
	}
}

void HttpImpl::ParseHeaders(const ArrayView <UInt8> & headers, const ReceiveHeaderFn & callback)
{
	ArrayView <WChar> ucs16 = { Reinterpret<WChar>(headers.data), UInt(headers.size / sizeof(WChar)) };

	auto line = Reflex::Data::Detail::ReadLine(ucs16);

	while (ucs16)
	{
		auto line = Reflex::Data::Detail::ReadLine(ucs16);

		if (auto pos = Search(line, ':'))
		{
			auto item = Splice(line, pos.value);

			auto key = Trim(item.a);

			auto value = Trim(Mid(item.b, 1));

			callback(ToCString(key), ToCString(value));
		}
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::HttpConnection> Reflex::System::HttpConnection::Create(bool https, const CString::View & server, UInt16 port)
{
	if (Win::Library::st_internet_ready)
	{
		return REFLEX_CREATE(Win::HttpImpl, https, server, port);
	}
	else
	{
		//todo check does this restriction still apply to WinHttp

		DEV_ERROR("Win::HttpImpl must be created after ::main due to a win32 limitation");

		return {};
	}
}

Reflex::Tuple <Reflex::CString::View, Reflex::UInt16> Reflex::System::Win::HttpImpl::ExtractPort(const CString::View & url)
{
	if (auto pos = Search(url, ':'))
	{
		auto spliced = Splice(url, pos.value);

		return { spliced.a, UInt16(ToUInt32(Mid(spliced.b, 1))) };
	}
	else
	{
		return { url, 0 };
	}
}
