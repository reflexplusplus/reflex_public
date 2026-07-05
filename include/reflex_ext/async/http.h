#pragma once

#include "task.h"




//
//Addon API

namespace Reflex::Async
{

	using HttpHeaders = Array < Pair <CString> >;

	TRef <Task> CreateHttpRequest(const CString::View & method, const CString::View & url, const HttpHeaders & headers, const Data::Archive::View & body, Output & debug_output = File::output);

}




//
//Detail API

REFLEX_NS(Reflex::Async::Detail)

struct HttpRequestCallbacks : public Object
{
	virtual TRef <System::HttpConnection> Connect(bool https, const CString::View & domain, UInt16 port) { return System::HttpConnection::Create(https, domain, port); }

	virtual bool OnResponse(System::HttpConnection::Response status_code) { return status_code >= 200 && status_code < 300; }

	virtual void OnHeader(const CString::View & key, const CString::View & value) = 0;
	
	virtual void OnChunk(const Data::Archive::View & chunk) = 0;
	
	virtual TRef <Object> OnComplete() = 0;
};

struct StandardHttpRequestCallbacks : public HttpRequestCallbacks	//for application/json, AsyncTask result will be Data::PropertySet, other Data::ArchiveObject
{
	StandardHttpRequestCallbacks(UInt8 decode_json_flags = 0);

	void OnHeader(const CString::View & key, const CString::View & value) override;

	void OnChunk(const Data::Archive::View & chunk) override;

	TRef <Object> OnComplete() override;

	
	const UInt8 m_decode_json_flags;

	bool m_is_lz4 = false;

	bool m_is_json = false;

	Data::Archive m_body;
};

enum NetworkSimulation : UInt32
{
	kNetworkSimulationNone = kMaxUInt32,
	
	//for testing
	kNetworkSimulationNoConnection = 0,
	
	kNetworkSimulation400 = 400,
	kNetworkSimulation401 = 401,
	kNetworkSimulation403 = 403,
	kNetworkSimulation404 = 404,

	kNetworkSimulation384k = 48000,				//384 kbps
	kNetworkSimulation2G3G = 125000,			//1 Mbps
	kNetworkSimulationSlowMobile = 375000,		//3 Mbps
	kNetworkSimulationPoor4G = 1000000,			//8 Mbps
	kNetworkSimulationTypicalMobile = 2000000,	//16 Mbps
	kNetworkSimulationBroadband = 8000000,		//64Mbps
};

TRef <Task> CreateHttpRequest(const CString::View & method, const CString::View & url, const HttpHeaders & headers, const Data::Archive::View & body, TRef <HttpRequestCallbacks> callbacks, NetworkSimulation network_simulation = kNetworkSimulationNone, Output & output = File::output);

Worker::Result Fetch(Worker::Context & ctx, CString::View method, CString::View url, HttpHeaders::View headers, Data::Archive::View body, HttpRequestCallbacks & callbacks, NetworkSimulation network_simulation = kNetworkSimulationNone, Output & output = File::output);	//synchronous function

REFLEX_END




//
//impl

inline Reflex::TRef <Reflex::Async::Task> Reflex::Async::Detail::CreateHttpRequest(const CString::View & method, const CString::View & url, const HttpHeaders & headers, const Data::Archive::View & body, TRef <HttpRequestCallbacks> callbacks, Detail::NetworkSimulation network_simulation, Output & output)
{
	REFLEX_ASSERT(method && url); //TODO assert on url validation

	output.Log(method, url);

	return Worker::Create([method = Join(method), url = Join(url), headers, body = Join(body), callbacks = AutoRelease(callbacks), network_simulation, &output](Worker::Context & ctx)
	{
		return Detail::Fetch(ctx, method, url, headers, body, callbacks, network_simulation, output);
	});
}

inline Reflex::TRef <Reflex::Async::Task> Reflex::Async::CreateHttpRequest(const CString::View & method, const CString::View & url, const HttpHeaders & headers, const Data::Archive::View & body, Output & output)
{
	return Detail::CreateHttpRequest(method, url, headers, body, New<Detail::StandardHttpRequestCallbacks>(), Detail::kNetworkSimulationNone, output);
}

