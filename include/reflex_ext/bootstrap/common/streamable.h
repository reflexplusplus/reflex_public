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

	Streamable(File::PersistentPropertySet & propertyset, Key32 chunk_id, UInt16 chunkversion);

	void RestoreState(Key32 context = File::PersistentPropertySet::kContextSession);	//call post-constructor

	void StoreState();	//call pre-destructor


	const TRef <File::PersistentPropertySet> propertyset;

	const Key32 chunkid;



private:

	Reference <Object> m_listener;

};
