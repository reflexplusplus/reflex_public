#pragma once

#include "consolepanel.h"




//
//impl

REFLEX_NS(Reflex::IDE::Detail)

ConstTRef <GLX::StyleSheet> RetrieveStyleSheet();

TRef <GLX::Object> CreateInfoItem(const WString & key, const WString::View & value, bool path);

Array < Tuple <WString, Detail::ConsolePanel&> > CreatePanels(TRef <GLX::Object> root);


void ResetStreamable(Key32 context, GLX::Object & object);

void RestoreStreamable(const Data::PropertySet & propertyset, Key32 context, GLX::Object & object);

void StoreStreamable(Data::PropertySet & propertyset, const GLX::Object & object);


extern const Key32 kItemStates[3];

REFLEX_END




//
//impl

inline Reflex::ConstTRef <Reflex::GLX::StyleSheet> Reflex::IDE::Detail::RetrieveStyleSheet()
{
	return GLX::RetrieveStyleSheet(L":res:IDE/styles.txt");
}

