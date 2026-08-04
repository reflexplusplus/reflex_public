#include "[include].h"
#include "jni/http.hpp"
#include "../common/http.h"

REFLEX_BEGIN_INTERNAL(Reflex::System::Android)

struct HttpImpl : public System::HttpConnection {
	HttpImpl(CString && url_base);

	void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) override;

	Response Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_headers, const ReceiveDataFn & receive_data) override;

	const CString m_url_base;
	Float m_connect_timeout = kDefaultConnectionTimeout;
	Float m_read_timeout = kDefaultReadTimeout;
	static constexpr UInt kBufferSize = 4096;
};

HttpImpl::HttpImpl(CString && url_base)
	: m_url_base(std::move(url_base))
{}

void HttpImpl::SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds) {
	if (connect_timeout_seconds > read_timeout_seconds) {
		DEV_WARN("System::Http: connection timeout cannot be bigger than read timeout.");
	}

	m_connect_timeout = Min(connect_timeout_seconds, read_timeout_seconds);
	m_read_timeout = read_timeout_seconds;
}

HttpConnection::Response HttpImpl::Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_headers, const ReceiveDataFn & receive_data) {
	REFLEX_USE(Jni)

	// MEMO: we are attaching the thread each time, because this will typically be called by another thread than the main one.
	AttachedJavaEnv env;
	auto& httpConnectionClass = g_reflexActivityInstance->GetClass<HttpConnectionClass>(env);
	Response response = kResponseNoConnection;

	CString url = Join(m_url_base, resource);

	JavaExceptionCheckScope scope(env);
	HttpConnectionInstance req(env, httpConnectionClass, scope, url.GetData());
	if (scope.CheckAndClearException()) return kResponseNoConnection;

	req.SetTimeout(m_connect_timeout, m_read_timeout);
	req.SetMethod(method.data);

	bool specifiedUserAgent = false;
	for (auto & [key,value] : headers) 
	{
		specifiedUserAgent = specifiedUserAgent || Lowercase(key) == "user-agent";
		req.PutHeader(key.GetData(), value.GetData());
	}

	// macOS adds Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36
	if (!specifiedUserAgent) {
		req.PutHeader("User-Agent", "Mozilla/5.0 (Android)");
	}

	if (body) {
		req.SetBody(scope, body);

		if (scope.CheckAndClearException()) return kResponseNoConnection;
	}

	response = Response(req.PerformRequest(scope));

	if (scope.CheckAndClearException()) return kResponseNoConnection;

	Array<Pair<CString>> responseHeaders;
	Array<UInt8> responseBody;

	req.ReadResponseHeaders(responseHeaders);
	for (auto & [key, value] : responseHeaders)
	{
		receive_headers(key, value);
	}

	while (req.ReadResponseBody(scope, kBufferSize, responseBody)) {
		if (scope.CheckAndClearException()) return kResponseNoConnection;

		if (!receive_data(responseBody)) {
			response = kResponseAborted;
			break;
		}
	}

	return response;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::HttpConnection> Reflex::System::HttpConnection::Create(bool https, const CString::View & server, UInt16 port) {
	return REFLEX_CREATE(Android::HttpImpl, Common::MakeUrlBase(https, server, port));
}
