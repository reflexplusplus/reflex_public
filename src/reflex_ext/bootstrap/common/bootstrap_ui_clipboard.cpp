#include "[require].h"




REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

REFLEX_DECLARE_KEY32(clipboard);

REFLEX_END_INTERNAL

void Reflex::Bootstrap::SetClipboard(Key32 type, const Data::Archive::View & data)
{
	Data::SetKey32(global, kclipboard, type);
	
	Data::SetBinary(global, kclipboard, data);
}

Reflex::Data::Archive::View Reflex::Bootstrap::GetClipboard(Key32 type)
{
	if (Data::GetKey32(global, kclipboard) == type)
	{
		return Data::GetBinary(global, kclipboard);
	}

	return {};
}
