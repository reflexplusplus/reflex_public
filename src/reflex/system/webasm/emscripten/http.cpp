#include "sdk.h"




//
//Javascript bindings

extern "C" void reflex_webasm_http_request(em_ptr_t clientdata, const char * method, const char * url, em_ptr_t headers, em_val32 headers_len, em_ptr_t body, em_val32 body_len, em_ptr_t receive_headers, em_ptr_t receive_chunk, em_ptr_t response_code_ptr);




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::WebASM)

struct HttpImpl : public HttpConnection
{
	struct RequestArgs
	{
		HttpImpl * self;
		
		const ReceiveHeaderFn receive_header;
		const ReceiveDataFn receive_chunk;

		std::atomic <UInt32> response_code;
	};

	HttpImpl(bool https, const CString::View & server);


	virtual void SetTimeout(UInt32 seconds);

	virtual Response Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers_in, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_headers_fn, const ReceiveDataFn & receive_data_fn);

	static void ReceiveHeadersWrapper(em_ptr_t state, const char * pheaders, em_val32 length);

	static bool ReceiveChunkWrapper(RequestArgs & state, const UInt8 * pchunk, UInt32 size);


	CString m_domain;

	static constexpr CString::View kHTTP = "http://";
	static constexpr CString::View kHTTPS = "https://";
};

HttpImpl::HttpImpl(bool https, const CString::View & host)
	: m_domain(Join(https ? kHTTPS : kHTTP, host))
{
}

void HttpImpl::SetTimeout(UInt32 seconds)
{
}

// HttpConnection::Response HttpImpl::Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers_in, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_header, const ReceiveDataFn & receive_chunk)
// {
// 	RequestArgs clientdata = { this, receive_header, receive_chunk };

// 	CString url = m_domain;

// 	if (resource) 
// 	{
// 		url.Push('/');
	
// 		url.Append(resource);
// 	}

// 	std::atomic <UInt32> response_code = kMaxUInt32;

// 	reflex_webasm_http_request
// 	(	
// 		ToUIntNative(&clientdata),
// 		method.data,
// 		url.GetData(),
// 		ToUIntNative(headers_in.data), 
// 		headers_in.size, 
// 		ToUIntNative(body.data), 
// 		body.size, 
// 		ToUIntNative(&HttpImpl::ReceiveHeadersWrapper), 
// 		ToUIntNative(&HttpImpl::ReceiveChunkWrapper), 
// 		ToUIntNative(&response_code)
// 	);

// 	//response_code.wait(kMaxUInt32);

// 	UInt n = 10;
//  	while (response_code.load() == kMaxUInt32) 
//  	{
//  		SuspendThread(25);

//  		if (!n) 
//  		{
//  			DebugLog("ERROR aborting");
// 			break;
//  		}

//  		n--;
//  	}

// 	return Response(response_code.load());
// }

HttpConnection::Response HttpImpl::Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers_in, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_header, const ReceiveDataFn & receive_chunk)
{
	DebugLog("HttpImpl::Request");

    RequestArgs clientdata = {this, receive_header, receive_chunk, kMaxUInt32};
    
	CString url = m_domain;

    if (resource)
    {
        url.Push('/');
        url.Append(resource);
    }

	emscripten_fetch_attr_t attr;

	emscripten_fetch_attr_init(&attr);

	RawStringCopy(method.data, attr.requestMethod, sizeof(attr.requestMethod));

	attr.userData = &clientdata;

	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

	//Array <const char*> headers;

	//headers.Allocate((headers_in.size * 2) + 1);

	for (auto & i : headers_in) 
	{
		//headers.Push(i.a.GetData());
	
		//headers.Push(i.b.GetData());
	}

	//headers.Push(0);

    //attr.requestHeaders = headers.GetData();

    if (body)
    {
        attr.requestData = Reinterpret<char>(body.data);
        attr.requestDataSize = body.size;
    }

	DebugLog("emscripten_fetch");
	
	url = "testfile.json";

	emscripten_fetch_t * fetch = emscripten_fetch(&attr, url.GetData());

    UInt32 response_code = fetch->status;

	emscripten_fetch_close(fetch);

	if (response_code == kMaxUInt32) response_code = kResponseNoConnection;

    return Response(response_code);
}

void HttpImpl::ReceiveHeadersWrapper(em_ptr_t state, const char * pheaders, em_val32 length)
{
	CString::View utf8 = { pheaders, length };

	while (utf8)
	{
		auto line = Data::Detail::ReadLine(utf8);

		if (auto pos = Search(line, ':'))
		{
			auto item = Splice(line, pos.value);

			auto key = item.a;

			auto value = item.b;

			value.data += 2;

			value.size -= 2;

			reinterpret_cast<RequestArgs*>(state)->receive_header(key, value);
		}
	}
}

bool HttpImpl::ReceiveChunkWrapper(RequestArgs & state, const UInt8 * pchunk, UInt32 size)
{
	DebugLog("HttpImpl::ReceiveChunkWrapper");
	
	return state.receive_chunk({pchunk, size});
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::HttpConnection> Reflex::System::HttpConnection::Create(bool https, const CString::View & server)
{
	return REFLEX_CREATE(WebASM::HttpImpl, https, server);
}
