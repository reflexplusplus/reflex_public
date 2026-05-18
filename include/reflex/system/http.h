#pragma once

#include "types.h"




//
//Primary API

namespace Reflex::System
{

	class HttpConnection;

}




//
//HttpConnection

class Reflex::System::HttpConnection : public Object
{
public:

	REFLEX_OBJECT(System::HttpConnection, Object);

	static HttpConnection & null;



	//types

	static const CString::View kHEAD;
	static const CString::View kGET;
	static const CString::View kPOST;

	static constexpr Float kDefaultConnectionTimeout = 4;
	static constexpr Float kDefaultReadTimeout = 60;

	using ReceiveHeaderFn = Function <void(const CString::View & key, const CString::View & value)>;

	using ReceiveDataFn = Function <bool(const ArrayView <UInt8> & chunk)>;

	enum Response
	{
		kResponseAborted = -1,
		kResponseNoConnection = 0,

		kResponseOK = 200,
		kResponseCreated = 201,
		kResponseCodeAccepted = 202,
		kResponseNoContent = 204,
		kResponsePartialContent = 206,

		kResponseMovedPermanently = 301,
		kResponseFound = 302,

		kResponseBadRequest = 400,
		kResponseUnauthorized = 401,
		kResponseForbidden = 403,
		kResponseNotFound = 404,

		kResponseInternalServerError = 500,
		kResponseServiceUnavailable = 503
	};



	//lifetime

	[[nodiscard]] static TRef <HttpConnection> Create(bool https, const CString::View & hostname, UInt16 = 0);



	//access

	virtual void SetTimeout(Float connect_timeout_seconds, Float read_timeout_seconds = kDefaultReadTimeout) = 0;

	virtual Response Request(const CString::View & method, const CString::View & resource, const ArrayView < Pair <CString> > & headers, const ArrayView <UInt8> & body, const ReceiveHeaderFn & receive_header, const ReceiveDataFn & receive_data) = 0;
};
