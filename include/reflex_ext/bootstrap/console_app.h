#pragma once

#include "common/global.h"




//
//Experimental API

namespace Reflex::Bootstrap::CLI
{

	using TaskFn = FunctionPointer <void(Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)>;

	struct TaskDef
	{
		Key32 id;
		bool async = false;
		TaskFn fn;
	};

	enum Flags : UInt8
	{
		kFlagPrintDuration = MakeBit(0),
		kFlagPrintError = MakeBit(1),
		kFlagForceVerbose = MakeBit(2),
	};



	//input

	CString::View GetString(const Data::PropertySet & args, Key32 id);

	WString GetFilenameArg(const Data::PropertySet & args, CString::View id, bool check_exists);

	WString GetFolderArg(const Data::PropertySet & args, CString::View id, bool check_exists);

	bool GetBoolArg(const Data::PropertySet & args, Key32 id);



	//errors

	void ThrowError(const CString & error);

	void ThrowMissingArg(const CString::View & id, const CString::View & example = {});

	void RequireArgs(const Data::PropertySet & args, const ArrayView <CString::View> & ids);



	//run

	UInt8 Dispatch(const ArrayView <CString::View> & cmdline, const ArrayView <TaskDef> & tasks, UInt8 flags = 0);

}




//
//impl

REFLEX_NS(Reflex::Bootstrap::CLI::Detail)

Data::PropertySet PackArgs(ArrayView <CString::View> cmdline);

REFLEX_END

inline Reflex::CString::View Reflex::Bootstrap::CLI::GetString(const Data::PropertySet & args, Key32 id)
{
	return Data::GetCString(args, id);
}

inline bool Reflex::Bootstrap::CLI::GetBoolArg(const Data::PropertySet & args, Key32 id)
{
	return Data::GetCString(args, id) == Reflex::Detail::kFalseTrue[1];
}

inline void Reflex::Bootstrap::CLI::RequireArgs(const Data::PropertySet & args, const ArrayView <CString::View> & ids)
{
	for (auto & id : ids)
	{
		if (!Data::GetCString(args, id)) ThrowMissingArg(id);
	}
}

inline void Reflex::Bootstrap::CLI::ThrowMissingArg(const CString::View & id, const CString::View & example)
{
	REFLEX_ASSERT(false);

	throw(Join("missing arg --", id, ' ', example));
}

inline void Reflex::Bootstrap::CLI::ThrowError(const CString & error)
{
	REFLEX_ASSERT(false);

	throw(error);//ctx.SetResult(false, New<Data::CStringProperty>(error));
}
