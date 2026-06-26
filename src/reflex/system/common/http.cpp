#include "http.h"




Reflex::CString Reflex::System::Common::MakeUrlBase(bool https, const CString::View & domain, UInt port)
{
	constexpr CString::View kProtocols[] = { "http://", "https://" };

	auto url = Join(kProtocols[https], domain);

	if (port)
	{
		url.Push(':');

		url.Append(ToCString(port));
	}

	url.Push('/');

	return url;
}

const Reflex::CString::View Reflex::System::HttpConnection::kHEAD = "HEAD";

const Reflex::CString::View Reflex::System::HttpConnection::kGET = "GET";

const Reflex::CString::View Reflex::System::HttpConnection::kPOST = "POST";
