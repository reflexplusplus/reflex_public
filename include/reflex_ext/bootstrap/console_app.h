#pragma once

#include "common/global.h"
#include "common/functions.h"
#include "reflex_ext/async.h"




//
//Experimental API

namespace Reflex::Bootstrap::CLI
{

	using TaskFn = FunctionPointer <void(const Data::PropertySet & args, System::FileHandle & out)>;

	struct TaskDef
	{
		Key32 id;
		TaskFn fn;
	};

	enum Flags : UInt8
	{
		kFlagPrintDuration = MakeBit(0),
		kFlagPrintError = MakeBit(1),
		kFlagForceVerbose = MakeBit(2),
	};

	enum Colour
	{
		kColourDefault,

		kColourBlack,
		kColourRed,
		kColourGreen,
		kColourYellow,
		kColourBlue,
		kColourMagenta,
		kColourCyan,
		kColourWhite,

		kColourBrightBlack,
		kColourBrightRed,
		kColourBrightGreen,
		kColourBrightYellow,
		kColourBrightBlue,
		kColourBrightMagenta,
		kColourBrightCyan,
		kColourBrightWhite,

		kNumColour
	};

	struct ProgressBar : public Object
	{
		static ProgressBar & null;

		[[nodiscard]] static Unretained <ProgressBar> Create(System::FileHandle & out, CString::View title, bool show_progress);

		virtual void Render(Float32 progress = 0.0f) = 0;
	};

	struct TaskContext : 
		public System::FileHandle, 
		public Async::Worker::Context
	{
	};



	//tasks

	consteval CLI::TaskDef MakeTask(Key32 id, CLI::TaskFn fn) { return { id, fn }; }



	//input

	CString::View GetString(const Data::PropertySet & args, Key32 id, CString::View fallback = {});

	Array <CString::View> GetStringArray(const Data::PropertySet & args, Key32 id);

	WString GetFilename(const Data::PropertySet & args, CString::View id, bool check_exists, CString::View default_value = {});

	WString GetFolder(const Data::PropertySet & args, CString::View id, bool check_exists);

	bool GetBool(const Data::PropertySet & args, Key32 id);



	//output

	void Print(System::FileHandle & out, const CString::View & line);

	void Print(System::FileHandle & out, Colour colour, const CString::View & line);

	void Print(System::FileHandle & out, const WString::View & line);

	void OutputBinary(System::FileHandle & out, const Data::Archive::View & blob);


	
	//progress spinner / bar

	void Await(System::FileHandle & out, CString::View title, bool progress_bar, const Function <bool()> & should_abort, const Function <void(TaskContext & ctx)> & bg_fn);	//with spinner or progress bar



	//errors

	void ThrowError(const CString & error);

	void ThrowMissingArg(CString::View id, CString::View example = {});

	void RequireArgs(const Data::PropertySet & args, ArrayView <CString::View> ids);



	//run

	UInt8 Dispatch(ArrayView <CString::View> cmdline, ArrayView <TaskDef> tasks, UInt8 flags = 0);

}




//
//impl

REFLEX_NS(Reflex::Bootstrap::CLI::Detail)

UInt8 Dispatch(ArrayView <CString::View> cmdline, ArrayView <TaskDef> tasks, UInt8 flags, System::FileHandle & out, void * client, FunctionPointer <bool(void * client, ArrayView <CString::View>, Key32 task, const Data::PropertySet&, System::FileHandle&)> fallback);

WString ExpandPath(CString::View id, WString::View input, bool folder, bool check_exists);

extern const CString::View kColours[kNumColour];

REFLEX_END

inline Reflex::UInt8 Reflex::Bootstrap::CLI::Dispatch(ArrayView <CString::View> cmdline, ArrayView <TaskDef> tasks, UInt8 flags)
{
	return Detail::Dispatch(cmdline, tasks, flags, Make<System::FileHandle>(System::FileHandle::kStandardStreamOut), nullptr, [](void * client, ArrayView <CString::View> cmdline, Key32 task, const Data::PropertySet & args, System::FileHandle & out)
	{ 
		ThrowError("unknown task"); 
		
		return false; 
	});
}

inline void Reflex::Bootstrap::CLI::RequireArgs(const Data::PropertySet & args, ArrayView <CString::View> ids)
{
	for (auto & id : ids)
	{
		if (!Data::GetCString(args, id)) ThrowMissingArg(id);
	}
}

inline Reflex::CString::View Reflex::Bootstrap::CLI::GetString(const Data::PropertySet & args, Key32 id, CString::View fallback)
{
	return Data::GetCString(args, id, fallback);
}

inline bool Reflex::Bootstrap::CLI::GetBool(const Data::PropertySet & args, Key32 id)
{
	if (auto value = Data::GetCString(args, id)) return value == Reflex::Detail::kFalseTrue[1];

	return Data::GetBool(args, id);
}

inline Reflex::WString Reflex::Bootstrap::CLI::GetFilename(const Data::PropertySet & args, CString::View id, bool check_exists, CString::View default_value)
{
	return Detail::ExpandPath(id, ToWString(Data::GetCString(args, id, default_value)), false, check_exists);
}

inline Reflex::WString Reflex::Bootstrap::CLI::GetFolder(const Data::PropertySet & args, CString::View id, bool check_exists)
{
	return File::CorrectTrailingStroke(Detail::ExpandPath(id, ToWString(Data::GetCString(args, id)), true, check_exists));
}

inline void Reflex::Bootstrap::CLI::Print(System::FileHandle & out, const CString::View & line)
{
	File::WriteLine(out, line);
}

inline void Reflex::Bootstrap::CLI::Print(System::FileHandle & out, Colour colour, const CString::View & line)
{
	File::WriteLine(out, Join(Detail::kColours[colour], line, Detail::kColours[kColourDefault]));
}

inline void Reflex::Bootstrap::CLI::Print(System::FileHandle & out, const WString::View & line)
{
	File::WriteLine(out, line);
}

inline void Reflex::Bootstrap::CLI::OutputBinary(System::FileHandle & out, const Data::Archive::View & blob)
{
	File::WriteBytes(out, blob);
}

inline void Reflex::Bootstrap::CLI::ThrowError(const CString & error)
{
	REFLEX_ASSERT_EX(false, error.GetData());

	throw(error);
}

inline void Reflex::Bootstrap::CLI::ThrowMissingArg(CString::View id, CString::View example)
{
	ThrowError(Join("missing arg ", Detail::kColours[kColourBrightWhite], "--", id, ' ', Detail::kColours[kColourBrightBlack], example, Detail::kColours[kColourDefault]));
}
