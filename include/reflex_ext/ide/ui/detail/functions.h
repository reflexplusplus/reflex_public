#pragma once

#include "consolepanel.h"




//
//impl

REFLEX_NS(Reflex::IDE::Detail)

ConstAlreadyRetained <GLX::StyleSheet> RetrieveStyleSheet();

[[nodiscard]] Unretained <GLX::Object> CreateInfoItem(const WString & key, const WString::View & value, bool path);

Array < Tuple <WString, Detail::ConsolePanel&> > CreatePanels(AlreadyRetained <GLX::Object> root);


void ResetSerializable(Key32 context, GLX::Object & object);

void RestoreSerializable(const Data::PropertySet & propertyset, Key32 context, GLX::Object & object);

void StoreSerializable(Data::PropertySet & propertyset, const GLX::Object & object);


extern const Key32 kItemStates[3];

REFLEX_END




//
//impl

inline Reflex::ConstAlreadyRetained <Reflex::GLX::StyleSheet> Reflex::IDE::Detail::RetrieveStyleSheet()
{
	return GLX::RetrieveStyleSheet(L":res:Reflex::IDE/styles.txt");
}
