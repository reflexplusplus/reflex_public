#include "system/defines.h"

#include "system/file.cpp"
#include "system/process.cpp"
#include "system/http.cpp"




#define VM_SYSTEM_ENUM(ENUM) VM_QBIND_ENUM(System, ENUM),

REFLEX_SET_TRAIT(Reflex::System::FileHandle, IsThreadSafe);
REFLEX_SET_TRAIT(Reflex::System::FileHandle, IsNonCircular);
REFLEX_SET_TRAIT(Reflex::System::Process, IsNonCircular);

Reflex::VM::Detail::NotificationObject::NotificationObject(VM::Detail::FnObject & callback, Object & notification)
	: callback(callback),
	notification(notification)
{
}

REFLEX_BEGIN_INTERNAL(Reflex::VM)

struct Globals : public Object
{
	Globals()
		: kPathTemp(New<String>(System::GetPath(System::kPathTemp))),
		kPathDesktop(New<String>(System::GetPath(System::kPathDesktop))),
		kPathApplicationData(New<String>(System::GetPath(System::kPathApplicationData))),
		kPathUserData(New<String>(System::GetPath(System::kPathUserData))),
		kPathUserDocuments(New<String>(System::GetPath(System::kPathUserDocuments))),

		kPathDelimiter(New<String>(ToView(File::kStroke)))
	{
	}

	Reference <String> kPathTemp, kPathDesktop, kPathApplicationData, kPathUserData, kPathUserDocuments;

	Reference <String> kPathDelimiter;
};

constexpr UInt32 kOperatingSystems[System::kNumPlatform] = { K32("windows"), K32("macos"), K32("linux"), K32("android"), K32("ios"), K32("webasm") };

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gSystem("System", { &gCore }, Reflex::kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object &)
{
	auto bindings = cstate.bindings;

	auto string_t = bindings->string_t;

	auto float32_t = bindings->float32_t;


	AcquireStaticString(cstate, "System");


	auto & globals = *Data::Detail::AcquireProperty<Globals>(bindings, kNullKey);

	RegisterExternalObject<false>(cstate, string_t, kSystem, "kPathTemp", globals.kPathTemp);
	RegisterExternalObject<false>(cstate, string_t, kSystem, "kPathDesktop", globals.kPathDesktop);
	RegisterExternalObject<false>(cstate, string_t, kSystem, "kPathApplicationData", globals.kPathApplicationData);
	RegisterExternalObject<false>(cstate, string_t, kSystem, "kPathUserDocuments", globals.kPathUserDocuments);
	RegisterExternalObject<false>(cstate, string_t, kSystem, "kPathUserData", globals.kPathUserData);
	RegisterExternalObject<false>(cstate, string_t, kSystem, "kPathDelimiter", globals.kPathDelimiter);

	cstate.RegisterConstant(bindings->key32_t, kSystem, "kOperatingSystem", Data::Pack(kOperatingSystems[System::kPlatform]));


	auto pair_string_t = cstate.InstantiateTemplateType(kTuple, { string_t, string_t });

	auto array_pair_string_t = cstate.InstantiateTemplateType(kArray, { pair_string_t });


	#ifndef REFLEX_MINIMAL
	BindProcess(bindings);
	#endif

	BindFile(bindings, pair_string_t, array_pair_string_t);

	BindHttpAndSleep(cstate, context, pair_string_t, array_pair_string_t);

	AddFunction(bindings, kSystem, "GetElapsedTime", float32_t, {}, [](Context & context)
	{
		VM_RTN(Float32(System::GetElapsedTime()));
	});
});

#undef VM_SYSTEM_ENUM
