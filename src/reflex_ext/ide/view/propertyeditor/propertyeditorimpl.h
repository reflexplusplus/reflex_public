#pragma once

#include "../../globalimpl.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::IDE::Detail)

struct PropertyEditorImpl : public PropertyEditor
{
	struct WeakRef : public Reflex::Detail::AbstractWeakRef
	{
		WeakRef(Reflex::Detail::DynamicTypeRef object_t, Object & null)
			: AbstractWeakRef(object_t, null, null)
		{
		}

		using AbstractWeakRef::Store;

		TRef <Data::PropertySet> Load() const
		{
			return Cast<Data::PropertySet>(AbstractWeakRef::Load());
		}
	};

	struct Dialog;

	struct ModificationScope;

	struct Transaction;

	struct Attribute;


	typedef PropertyEditorImpl View;

	typedef GLX::Tree::Node UI;


	struct iNull;

	struct iBinary;



	//lifetime

	PropertyEditorImpl(const Data::Detail::StandardPropertySheetInterface & propertysheetinterface, const Interface & iface);

	~PropertyEditorImpl();



	//data

	void Clear() override;

	void SetRoot(Data::PropertySet & node) override;

	TRef <Data::PropertySet> GetRoot() override;



	//open

	void SetFocus(Data::PropertySet & node) override;

	TRef <Data::PropertySet> GetFocus() override;


	void Open(Data::PropertySet & node) override;

	void Close(Data::PropertySet & node) override;

	bool IsOpen(const Data::PropertySet & node) override;



	//layout io

	virtual void OnRestore(Data::Archive::View & stream, Key32 context) override;

	virtual void OnStore(Data::Archive & stream) const override;


	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override;

	virtual void OnAttachWindow() override;

	virtual void OnDetachWindow() override;

	virtual void OnUpdate() override;


	virtual void OnRestoreHistory(Data::Archive::View stream, bool redo) override;



	void Associate(GLX::Object & object, Data::PropertySet & data)
	{
		auto handle = New<WeakRef>(m_typeinfo, m_null);

		handle->Store(data);

		object.SetProperty(kNullKey, handle);
	}

	static void Deassociate(GLX::Object & object)
	{
		object.UnsetProperty<WeakRef>(kNullKey);
	}

	Data::PropertySet * GetData(GLX::Object & object) const
	{
		if (auto weakref = object.QueryProperty<WeakRef>(kNullKey))
		{
			auto target = weakref->Load().Adr();

			if (target != m_null.Adr()) return target;
		}

		return 0;
	}


	static bool SetFocus(PropertyEditorImpl & view, Data::PropertySet & node);


	static Data::PropertySet * AddChild(PropertyEditorImpl & view, Data::PropertySet & parent)
	{
		auto & iface = *view.interface;

		auto child = iface.CreateNode();

		if (child != view.m_null)
		{
			auto children = view.GetChildren(parent);

			children.Push({ L"", child });

			iface.SetChildren(parent, children);

			return child.Adr();
		}

		return 0;
	}

	static void Insert(PropertyEditorImpl & view, Data::PropertySet & node, Data::PropertySet & sibling, bool after)
	{
		auto & iface = *view.interface;

		auto a = view.GetParent(node);

		auto b = view.GetParent(node);

		if (a && b && a == b)
		{
			auto & parent = *a;

			auto children = view.GetChildren(parent);

			REFLEX_LOOP(idx, children.GetSize())
			{
				if (children[idx].b.Adr() == &node)
				{
					Interface::Children::Type src = children[idx];

					children.Remove(idx);

					REFLEX_LOOP(idx, children.GetSize())
					{
						if (children[idx].b.Adr() == &sibling)
						{
							children.Insert(idx + after, src);

							iface.SetChildren(parent, children);

							return;
						}
					}
				}
			}
		}
	}

	static void RemoveChild(PropertyEditorImpl & view, Data::PropertySet & parent, Data::PropertySet & child)
	{
		auto & iface = *view.interface;

		auto children = view.GetChildren(parent);

		REFLEX_LOOP(idx, children.GetSize())
		{
			auto & item = children[idx];

			if (item.b.Adr() == &child)
			{
				children.Remove(idx);

				iface.SetChildren(parent, children);

				return;
			}
		}
	}

	Data::PropertySet * GetParent(Data::PropertySet & node);

	void GetParents(Data::PropertySet & node, Array <Data::PropertySet*> & output);

	Data::PropertySet * GetPrev(Data::PropertySet & node);

	Data::PropertySet * GetNext(Data::PropertySet & node);


	static bool BeginAddNode(PropertyEditorImpl & view, Data::PropertySet & parent);

	static void RemoveNode(PropertyEditorImpl & view, Data::PropertySet & node);

	template <bool FWD, bool END> static void Move(PropertyEditorImpl & view, Data::PropertySet & node);


	//static void DuplicateNode(PropertyEditorImpl & view, Data::PropertySet & node);

	//static void CopyNode(PropertyEditorImpl & view, Data::PropertySet & node);

	//static void PasteNode(PropertyEditorImpl & view, Data::PropertySet & node);


	static void BeginAddProperty(PropertyEditorImpl & view);

	void BeginEditProperty(Address address);

	static void RemoveSelectedProperties(PropertyEditorImpl & view);


	static void RefreshBranch(PropertyEditorImpl & view, Data::PropertySet & node, UI & ui);

	void RefreshNode(Data::PropertySet & node, UI & ui, const WString::View & id, bool animate = GLX::AnimationScope::IsEnabled());


	void PopulateProperties();


	Interface::Children GetChildren(Data::PropertySet & node) const
	{
		Interface::Children children;

		children.Allocate(4);

		interface->GetChildren(node, children);

		return children;
	}

	static void Visit(GLX::Tree::Node & node, const Function <void(GLX::Tree::Node&)> & callback)
	{
		callback(node);

		for (auto & i : node.body)
		{
			Visit(Cast<GLX::Tree::Node>(i), callback);
		}
	}



	ConstReference <Data::Detail::StandardPropertySheetInterface> m_propertysheet_interface;


	const Reflex::Detail::DynamicTypeRef m_typeinfo;

	Reference <Data::PropertySet> m_null;

	ConstReference <Data::KeyMap> m_keymap;


	Reference <Data::PropertySet> m_root;

	WeakRef m_focus;


	Key32 m_property_group;


	const GLX::Style & styles;

	struct Styles { ConstTRef <GLX::Style> nodes, properties, attribute, values_add, button; } m_styles;


	GLX::Object m_property_groups;

	GLX::Tree m_tree;


	GLX::Object m_properties_bar;

	GLX::Button m_add_property, m_remove_property;

	GLX::ListScroller m_properties;

	GLX::Object m_properties_area;


	UInt m_mute;

	bool m_animate;


	static inline const Key32 kOpen = K32("PropertyEditorImpl/Open");

	static const Key32 kTypeLabelID;

	static const Key32 kLeafState;
};

REFLEX_END_INTERNAL
