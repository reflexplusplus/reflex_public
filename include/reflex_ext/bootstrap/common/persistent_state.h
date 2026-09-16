#pragma once

#include "[require].h"




//
//Secondary API

namespace Reflex::Bootstrap
{

	class PersistentState;

}




//
//PersistentState

class Reflex::Bootstrap::PersistentState : public Data::iSerializable
{
protected:

	PersistentState(File::PersistentPropertySet & propertyset, Key32 chunk_id, UInt16 chunkversion);

	void RestoreState(Key32 context = File::PersistentPropertySet::kContextSession);	//call post-constructor

	void StoreState();	//call pre-destructor


	const AlreadyRetained <File::PersistentPropertySet> propertyset;

	const Key32 chunkid;



private:

	Reference <Object> m_listener;

};




//
//deprecations

REFLEX_NS(Reflex::Bootstrap)
typedef PersistentState Streamable [[deprecated("use PersistentState")]];
REFLEX_END