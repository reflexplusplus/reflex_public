#pragma once

#include "common/global.h"




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

		static TRef <ProgressBar> Create(System::FileHandle & out, const CString::View & title, bool show_progress);

		virtual void SetProgress(Float proportion) = 0;

		void Step() { SetProgress(0.0f); }
	};

	struct TaskContext : public System::FileHandle
	{
		virtual bool Cancelled() const = 0;

		virtual void SetProgress(Float progress) = 0;
	};



	//input

	CString::View GetString(const Data::PropertySet & args, Key32 id);

	WString GetFilenameArg(const Data::PropertySet & args, CString::View id, bool check_exists);

	WString GetFolderArg(const Data::PropertySet & args, CString::View id, bool check_exists);

	bool GetBoolArg(const Data::PropertySet & args, Key32 id);



	//output

	void Print(System::FileHandle & out, const CString::View & line);

	void Print(System::FileHandle & out, Colour colour, const CString::View & line);

	void Print(System::FileHandle & out, const WString::View & line);

	void OutputBinary(System::FileHandle & out, const Data::Archive::View & blob);


	
	//progress spinner / bar

	void Await(System::FileHandle & out, const CString::View & title, bool progress_bar, const Function<bool()> & should_abort, const Function<void(TaskContext & ctx)> & bg_fn);	//with spinner or progress bar



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

extern const CString::View kColours[kNumColour];

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

inline void Reflex::Bootstrap::CLI::ThrowMissingArg(const CString::View & id, const CString::View & example)
{
	REFLEX_ASSERT(false);

	throw(Join("missing arg ", Detail::kColours[kColourBrightWhite], "--", id, ' ', Detail::kColours[kColourBrightBlack], example, Detail::kColours[kColourDefault]));
}

inline void Reflex::Bootstrap::CLI::ThrowError(const CString & error)
{
	REFLEX_ASSERT(false);

	throw(error);
}
