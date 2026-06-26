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

HttpConnection::Response HttpImpl::Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers_in, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_header, const ReceiveDataFn & receive_chunk)
{
	emscripten_fetch_attr_t attr;

	emscripten_fetch_attr_init(&attr);

	strcpy(attr.requestMethod, method.data);
  
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
  
	auto url = Join(m_domain, '/', resource);
	
	emscripten_fetch_t * fetch = emscripten_fetch(&attr, url.GetData()); 	//block until operation is complete.
  
	if (fetch->status == 200) 
	{
		ArrayView <UInt8> response = { Reinterpret<UInt8>(fetch->data), UInt32(fetch->numBytes) };
		
		receive_chunk(response);
	} 
	else 
	{
	}
  
	emscripten_fetch_close(fetch);
  
    return Response(fetch->status);
}

void HttpImpl::ReceiveHeadersWrapper(em_ptr_t state, const char * pheaders, em_val32 length)
{
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
