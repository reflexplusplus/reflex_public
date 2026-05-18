#pragma once

#include "../[require].h"




//
//impl

REFLEX_NS(Reflex::IDE::Detail)

class PropertyEditor;

REFLEX_END




//
//view

class Reflex::IDE::Detail::PropertyEditor :
	public GLX::Split,
	public Data::History,
	public Data::iStreamable
{
public:

	REFLEX_OBJECT(IDE::Detail::PropertyEditor, GLX::Split);



	//types

	class Interface;

	REFLEX_DECLARE_KEY32(SelectNode);

	REFLEX_DECLARE_KEY32(EditNode);

	REFLEX_DECLARE_KEY32(node);



	//standard attributes

	static constexpr CString::View kType = "type";

	static constexpr CString::View kID = "id";



	//lifetime

	static TRef <PropertyEditor> Create(const Data::Detail::StandardPropertySheetInterface & propertysheet_interface, const Interface & iface);



	//data

	virtual void Clear() = 0;

	virtual void SetRoot(Data::PropertySet & node) = 0;

	virtual TRef <Data::PropertySet> GetRoot() = 0;



	//open

	virtual void SetFocus(Data::PropertySet & node) = 0;

	virtual TRef <Data::PropertySet> GetFocus() = 0;


	virtual void Open(Data::PropertySet & node) = 0;

	virtual void Close(Data::PropertySet & node) = 0;

	virtual bool IsOpen(const Data::PropertySet & node) = 0;



	//links

	const ConstTRef <Interface> interface;
	
	const ConstTRef <GLX::Style> ide_styles;



protected:

	PropertyEditor(const Interface & iface);

};




//
//view

class Reflex::IDE::Detail::PropertyEditor::Interface : public Reflex::Object
{
public:

	static Interface & null;

	//types

	using Children = Array < Pair <WString, Reference <Data::PropertySet> > >;	//id | node

	using PropertyRef = Tuple < Address, Reference <Object>, Data::Detail::ObjectToStringFn >;

	using Properties = Array <PropertyRef>;



	//notifications

	enum Notification
	{
		kNotificationUpdate,
		kNotificationReset,
		kNotificationRestore,
		kNotificationStore,
	};



	//structure

	virtual Pair < Reflex::Detail::DynamicTypeRef, TRef <Data::PropertySet> > GetObjectType() const = 0;	//return object_t and null instance

	virtual TRef <Data::PropertySet> CreateNode() const { return GetObjectType().b; }

	virtual Array <WString> GetPropertyGroups() const { return {}; }

	virtual void SetChildren(Data::PropertySet & node, Children children) const {}

	virtual void GetChildren(Data::PropertySet & node, Children & children) const {}



	//properties

	virtual ConstTRef <Data::KeyMap> GetKeyMap() const { return Null<Data::KeyMap>(); }

	virtual void RemoveProperty(Data::PropertySet & node, Address address) const { }

	virtual void SetProperty(Data::PropertySet & node, Type type, const CString::View & key, const Data::Archive::View & value) const { }

	virtual void GetProperties(Data::PropertySet & node, Key32 group, Properties & properties) const {}



	//stream (used for undo)

	virtual void Serialize(const Data::PropertySet & node, Data::Archive & stream) const {}

	virtual void Deserialize(Data::PropertySet & node, Data::Archive::View & stream) const {}

};
