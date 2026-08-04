#pragma once

#include "[require].h"




//
//Secondary API

namespace Reflex::Bootstrap
{

	class Streamable;

}




//
//Streamable

class Reflex::Bootstrap::Streamable : public Data::iStreamable
{
protected:

	Streamable(File::PersistentPropertySet & propertyset, Key32 chunkid, UInt16 chunkversion);

	void RestoreState(Key32 context = File::PersistentPropertySet::kContextSession);	//call post-constructor

	void StoreState();	//call pre-destructor



private:

	File::PersistentPropertySet & propertyset;

	Key32 m_chunkid;

	Reference <Object> m_listener;

};
