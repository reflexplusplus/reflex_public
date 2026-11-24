#pragma once

#include "../functions/string.h"
#include "marker.h"
#include "transitional.h"




//
//Primary API

namespace Reflex::Data
{

	void SerializeUTF8(Archive & stream, const WString::View & string);

	void DeserializeUTF8(Archive::View & stream, WString & string_out);

	WString DeserializeUTF8(Archive::View & stream);


	void SerializeUCS2(Archive & stream, const WString::View & string);

	void DeserializeUCS2(Archive::View & stream, WString & string_out);

	WString DeserializeUCS2(Archive::View & stream);

}




//
//Detail/Legacy

namespace Reflex::Data::Detail
{

	struct WStringUCS2 : public WString
	{
		WStringUCS2() = default;
		WStringUCS2(WString::View value) : WString(value) {}

		void Deserialize(Data::Archive::View & stream)
		{
			Data::DeserializeUCS2(stream, Cast<WString>(*this));
		}

		void Serialize(Data::Archive & stream)
		{
			Data::SerializeUCS2(stream, Cast<WString>(*this));
		}
	};

	void RestoreLegacyWString(Archive::View & stream, WString & out);

}
