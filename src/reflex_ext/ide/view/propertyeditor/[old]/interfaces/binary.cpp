#include "../propertyeditorimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE::Detail)

struct PropertyEditorImpl::iBinary : public PropertyEditor::Interface
{
	static iBinary self;

	inline static const Key32 kMap = K32("<map>");


	virtual Data::PropertySet & GetNull() const override { return Data::PropertySet::null; }

	virtual UInt8 GetFlags() const override { return Interface::kFlagProperties | Interface::kFlagWritable; }

	virtual Reflex::Object & CreateListener(Data::PropertySet & abstractnode, const Function <void(Notification)> & callback) const override
	{
		if (auto xml = DynamicCast<File::PropertySet>(abstractnode))
		{
			return *REFLEX_CREATE(File::PropertySet::Listener, *xml, [callback](Enum notification)
			{
				callback(Notification(notification));
			});
		}

		return Reflex::Object::null;
	}

	Data::PropertySet * CreateNode() const override
	{
		return REFLEX_CREATE(Data::PropertySet);
	}

	void SetChildren(Data::PropertySet & node, Children children) const override
	{
		//auto keymap = File::UnpackPropertyUNUSED<KeyMap>(node, kMap);

		//Data::UnsetAll<File::PropertySet>(node);

		//for (auto & i : children)
		//{
		//	auto id = File::UnpackPropertyUNUSED<CString::View>(*i.b, kid);

		//	Reflex::SetProperty(node, *i.b, i.a);
		//}

		//File::PackPropertyUNUSED(node, kMap, keymap);
	}

	Children GetChildren(Data::PropertySet & node) const override
	{
		return {};
		//auto keymap = File::UnpackPropertyUNUSED<KeyMap>(node, kMap);

		//Children rtn;
		//
		//auto range = node.Iterate<File::PropertySet>();

		//for (auto & i : range) rtn.Push(*i.value);

		//return rtn;
	}

	virtual void RemoveAttribute(Data::PropertySet & node, Type type, const CString::View & key) const override
	{
		Data::UnsetBinary(node, key);
	}

	virtual void SetAttribute(Data::PropertySet & node, Type type, const CString::View & key, const Data::Archive::View & value) const override
	{
		//File::PackPropertyUNUSED(node, key, Data::Unpack<CString>(value));

		//auto keymap = File::UnpackPropertyUNUSED<KeyMap>(node, kMap);

		//keymap.Retrieve(key) = key;

		//File::PackPropertyUNUSED(node, kMap, keymap);
	}

	virtual Attributes GetAttributes(Data::PropertySet & node) const override
	{
		return {};

		//auto keymap = File::UnpackPropertyUNUSED<KeyMap>(node, kMap);

		//Attributes rtn;

		//auto range = node.Iterate<Data::ArchiveObject>();

		//for (auto & i : range)
		//{
		//	if (i.key.id != kMap)
		//	{
		//		rtn.Push({ kTypeCString, Data::BytesToHex(Data::Pack(i.key.id)), *i.value });
		//	}
		//}

		//return rtn;
	};
};

PropertyEditorImpl::iBinary PropertyEditorImpl::iBinary::self;

REFLEX_END_INTERNAL

const Reflex::IDE::Detail::PropertyEditor::Interface & Reflex::IDE::Detail::PropertyEditor::kBinary = Reflex::IDE::Detail::PropertyEditorImpl::iBinary::self;
