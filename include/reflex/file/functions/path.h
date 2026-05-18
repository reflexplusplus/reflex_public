#pragma once

#include "../defines.h"




//
//Primary API

namespace Reflex::File
{

	constexpr auto Exists = &System::Exists;

	constexpr auto IsDirectory = &System::IsDirectory;

	constexpr auto Rename = &System::Rename;

	constexpr auto Delete = &System::Delete;

	constexpr auto MakeDirectory = &System::MakeDirectory;


	WString GetSystemPath(Path path);


	Pair < Map <WString, NullType, CaseInsensitive> > List(const WString & path, bool hidden = false);

	Array < Pair <WString> > GetVolumes();


	void MakePath(const WString & path);

	void DeleteDirectoryContent(const WString & path);

	void DeletePath(const WString & path);


	WString CorrectStrokes(const WString::View & path);					//converts '\' to '/'

	WString RemoveDuplicateStrokes(const WString::View & path);			//remove "//"

	WString CorrectTrailingStroke(const WString::View & path);			//appends "/" if missing

	WString::View RemoveTrailingStroke(const WString::View & path);		//removes last char if == "/" 


	Pair <WString::View> SplitFilename(const WString::View & filename);

	Pair <WString::View> SplitExtension(const WString::View & path);

	WString::View GetExtension(const WString::View & filename);

	WString::View RemoveExtension(const WString::View & filename);


	bool CheckExtension(const WString::View & filename, const WString::View & extension);

	WString CorrectExtension(const WString::View & path, const WString::View & extension);


	WString::View ResolveExistingFolder(const WString::View & path);


	WString ResolveRelativePath(const WString::View & path);			//path can be either folder (trailing stroke) or filename

	WString MakeRelativePath(const WString::View & base_dir, const WString::View & path);


	WString ResolveIncludePath(const WString::View & base_dir, const WString::View & include_path);	//resolved path only, will ignore base if path begins with stroke or ':', and removes "./" from beginning of path

}




//
//Detail

namespace Reflex::File::Detail
{

	void CorrectStrokes(WString & path);

	void RemoveDuplicateStrokes(WString & path);

	void CorrectTrailingStroke(WString & path);

	void RemoveTrailingStroke(WString & path);

}




//
//impl

inline Reflex::WString Reflex::File::GetSystemPath(Path path)
{
	return System::GetPath(path);
}

inline void Reflex::File::DeletePath(const WString & path)
{
	DeleteDirectoryContent(path);

	System::Delete(path);
}

inline Reflex::WString::View Reflex::File::GetExtension(const WString::View & path)
{
	return SplitExtension(path).b;
}

inline Reflex::WString::View Reflex::File::RemoveExtension(const WString::View & path)
{
	return SplitExtension(path).a;
}

inline Reflex::WString Reflex::File::CorrectStrokes(const WString::View & path)
{
	WString rtn(path);

	Detail::CorrectStrokes(rtn);

	return rtn;
}

inline Reflex::WString Reflex::File::RemoveDuplicateStrokes(const WString::View & path)
{
	WString rtn(path);

	Detail::RemoveDuplicateStrokes(rtn);

	return rtn;
}

inline Reflex::WString Reflex::File::CorrectTrailingStroke(const WString::View & path)
{
	WString rtn(path);

	Detail::CorrectTrailingStroke(rtn);

	return rtn;
}

inline Reflex::WString::View Reflex::File::RemoveTrailingStroke(const WString::View & path)
{
	if (path && path.GetLast() == System::kPathDelimiter)
	{
		return { path.data, path.size - 1 };
	}
	else
	{
		return path;
	}
}

