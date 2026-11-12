#pragma once

#include "types.h"




//
//Secondary API (use Async::HTTP)

namespace Reflex::System
{

	enum HttpResponseCode : UInt32
	{
		kHttpResponseCodeOK = 200,
		kHttpResponseCodeCreated = 201,
		kHttpResponseCodeAccepted = 202,
		kHttpResponseCodeNoContent = 204,
		kHttpResponseCodePartialContent = 206,

		kHttpResponseCodeMovedPermanently = 301,
		kHttpResponseCodeFound = 302,

		kHttpResponseCodeBadRequest = 400,
		kHttpResponseCodeUnauthorized = 401,
		kHttpResponseCodeForbidden = 403,
		kHttpResponseCodeNotFound = 404,

		kHttpResponseCodeInternalServerError = 500,
		kHttpResponseCodeServiceUnavailable = 503
	};

	class HttpConnection;

}




//
//HttpConnection

class Reflex::System::HttpConnection : public Object
{
public:

	REFLEX_OBJECT(System::HttpConnection, Object);

	static HttpConnection & null;


	//defines

	static const CString::View kHEAD;
	static const CString::View kGET;
	static const CString::View kPOST;

	static constexpr Float kDefaultConnectionTimeout = 4;
	static constexpr Float kDefaultReadTimeout = 60;


	//types

	struct Client : public Object
	{
		static Client & null;

		virtual bool OnResponse(UInt code) = 0;
		virtual void OnHeader(CString::View key, CString::View value) = 0;
		virtual bool OnChunk(ArrayView <UInt8> chunk) = 0;
	};

	enum Result
	{
		kResultCompleted,
		kResultRejected,	//Client::OnResponse returned false 
		kResultAborted,		//Client::OnChunk returned false 
		kResultTransportError
	};



	//lifetime

	[[nodiscard]] static Unretained <HttpConnection> Create(bool https, CString::View hostname, UInt16 port = 0);



	//access

	virtual void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds = kDefaultReadTimeout) = 0;

	virtual Result Request(CString::View method, CString::View resource, ArrayView < Pair <CString> > headers, ArrayView <UInt8> body, WillRetain <Client> client) = 0;
};
