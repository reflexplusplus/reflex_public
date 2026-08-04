#include "../../../include/reflex/data/format.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_DECLARE_KEY32(stage);
REFLEX_DECLARE_KEY32(description);
REFLEX_DECLARE_KEY32(line);

static constexpr const char * kDeserializeErrorStrings[] =
{
	nullptr,
	"invalid_header",
	"unsupported_version",
	"invalid_stream",
	"unknown_type"
};

REFLEX_END_INTERNAL

#if REFLEX_DEBUG
bool Reflex::Data::Format::CheckTypes(const PropertySet & data) const
{
	for (auto & i : data.Iterate())
	{
		auto d = i.value->object_t->tname;

		if (!SupportsType(i.key.type_id))
		{
			File::output.Log(object_t->tname, "unsupported type", d);

			//REFLEX_ASSERT(false);
		}
	}

	for (auto & i : data.Iterate<PropertySet>())
	{
		CheckTypes(i.value);
	}

	return true;
}
#endif

bool Reflex::Data::SerializableFormat::OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const
{
	auto stream = in;

	if (auto error = OnDeserialize(stream, out, options))
	{
		SetError(out, 0, "Deserialize", kDeserializeErrorStrings[error]);

		return false;
	}
	else
	{
		return true;
	}
}

bool Reflex::Data::SerializableFormat::OnEncode(Archive & out, const PropertySet & in, UInt options) const
{
	REFLEX_ASSERT(!out);

	out.Clear();

	OnSerialize(out, in);

	return true;
}

Reflex::Data::PropertySet Reflex::Data::CopyPropertySet(const Format & format, const PropertySet & from)
{
	PropertySet rtn;

	Archive archive;

	format.Encode(archive, from);

	format.Decode(rtn, archive);

	return rtn;
}

void Reflex::Data::SetError(Object & object, UInt line, CString && stage, CString && desc)
{
	auto error = New<PropertySet>();

	Data::SetUInt32(error, kline, line);

	Data::SetCString(error, kstage, stage);

	Data::SetCString(error, kdescription, desc);

	object.SetProperty(kError, error);
}

Reflex::Tuple <Reflex::UInt, Reflex::CString::View, Reflex::CString::View> Reflex::Data::GetError(const Object & object)
{
	auto error = Data::GetPropertySet(object, kError);

	return { Data::GetUInt32(error, kline, kMaxUInt32), Data::GetCString(error, kstage), Data::GetCString(error, kdescription) };
}

void Reflex::Data::ClearError(Object & object)
{
	object.UnsetProperty<Data::PropertySet>(kError);
}
