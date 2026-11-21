#include "tasks.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

Retained <System::SecretStorage> OpenStore(const Data::PropertySet & args)
{
	auto storage_name = CLI::GetString(args, "store", "com.reflexplusplus.cli.secrets");
	auto store = Make<System::SecretStorage>(storage_name, [](CString::View storage_name)
	{
		auto global = Bootstrap::global;
		auto path = Bootstrap::Detail::MakeProductPath(System::kPathUserData, global->vendor, global->product, ToWString(Data::BytesToHex(Data::SHA256(Data::Pack(storage_name)))));
		path.Push(File::kStroke);
		File::MakePath(path);
		return path;
	});
	if (!store) CLI::ThrowError("Could not open secret storage");
	return store;
}

void CheckResult(System::SecretStorage::Result result)
{
	constexpr CString::View kErrors[] = { "", "NotFound", "Denied", "TooLarge", "Failed" };
	if (result) ReflexCLI::ThrowError("SecretStorage", kErrors[result]);
}

REFLEX_END_INTERNAL

void ReflexCLI::UnsetSecret(const Data::PropertySet & args, System::FileHandle & out)
{
	auto store = OpenStore(args);
	CheckResult(store->Unset(CLI::GetString(args, kFirstPositionalArg)));
}

void ReflexCLI::GetSecret(const Data::PropertySet & args, System::FileHandle & out)
{
	auto store = OpenStore(args);
	Array <UInt8> value;
	CheckResult(store->Get(CLI::GetString(args, kFirstPositionalArg), value));
	if (File::WriteBytes(out,value) != value.GetSize()) CLI::ThrowError("Could not write secret to stdout");
}

void ReflexCLI::SetSecret(const Data::PropertySet & args, System::FileHandle & out)
{
	auto store = OpenStore(args);
	
	Data::Archive bytes;

	if (auto path = CLI::GetFilename(args, "path", false))
	{
		bytes = File::Open(path);
	}
	else
	{
		auto input = Make<System::FileHandle>(System::FileHandle::kStandardStreamIn);
		System::EnableConsoleEcho(false);
		char buffer[1024];
		auto n = input->Read(buffer, GetArraySize(buffer));
		CString::View value = { buffer, n };
		bytes = Data::Pack(Data::Detail::ReadLine(value));
		System::EnableConsoleEcho(true);
	}
	
	if (bytes) CheckResult(store->Set(CLI::GetString(args, kFirstPositionalArg), bytes));
}

