#pragma once

#include "../defines.h"




//
//Primary API

namespace Reflex::File
{

	Data::Archive Open(const WString & filename);

	bool Save(const WString & filename, const Data::Archive::View & data);


	UInt64 GetRemainder(const System::FileHandle & file_handle);

	Data::Archive ReadBytes(System::FileHandle & file_handle);

	Data::Archive ReadBytes(System::FileHandle & file_handle, UInt bytes);

	UInt WriteBytes(System::FileHandle & file_handle, const Data::Archive::View & bytes);


	Data::Archive Peek(System::FileHandle & file_handle, UInt bytes);


	template <class TYPE> bool ReadValue(System::FileHandle & file_handle, TYPE & inout);

	template <class TYPE> TYPE ReadValue(System::FileHandle & file_handle);

	template <class TYPE> bool WriteValue(System::FileHandle & file_handle, const TYPE & in);


	bool ReadLine(System::FileHandle & file_handle, CString & line_out);					//decodes ascii

	bool ReadLine(System::FileHandle & file_handle, WString & line_out);					//decodes from UTF8

	void WriteLine(System::FileHandle & file_handle, const CString::View & line = {});		//encodes to ascii

	void WriteLine(System::FileHandle & file_handle, const WString::View & line);			//encodes to UTF8


	bool Copy(const WString & from, const WString & to);

	bool Copy(System::FileHandle & from, System::FileHandle & to, UInt chunksize = (1024 * 1024));

}




//
//impl

inline Reflex::UInt64 Reflex::File::GetRemainder(const System::FileHandle & file_handle) 
{ 
	return file_handle.GetSize() - file_handle.GetPosition();
}

template <class TYPE> inline bool Reflex::File::ReadValue(System::FileHandle & file_handle, TYPE & inout)
{
	REFLEX_STATIC_ASSERT(Data::Detail::IsRawPackable<TYPE>::value);

	auto region = Reinterpret<Data::Archive::Region>(Data::Pack(inout));

	return file_handle.Read(region.data, sizeof(TYPE)) == sizeof(TYPE);
}

template <class TYPE> inline TYPE Reflex::File::ReadValue(System::FileHandle & file_handle)
{
	TYPE rtn = {};

	ReadValue(file_handle, rtn);

	return rtn;
}

inline Reflex::Data::Archive Reflex::File::ReadBytes(System::FileHandle & file_handle)
{
	return ReadBytes(file_handle, UInt32(file_handle.GetSize()));	//correct is GetSize() - GetPosition(), but usually pos will be 0 so faster to reduce size after, array shrink has no overhead
}

inline Reflex::UInt Reflex::File::WriteBytes(System::FileHandle & file_handle, const Data::Archive::View & bytes)
{
	return file_handle.Write(bytes.data, bytes.size);
}

template <class TYPE> inline bool Reflex::File::WriteValue(System::FileHandle & file_handle, const TYPE & in)
{
	REFLEX_STATIC_ASSERT(Data::Detail::IsRawPackable<TYPE>::value);

	return WriteBytes(file_handle, Data::Pack(in)) == sizeof(TYPE);
}

inline bool Reflex::File::Copy(const WString & from, const WString & to)
{
	if (from != to)
	{
		return Copy(Make<System::FileHandle>(from), Make<System::FileHandle>(to, System::FileHandle::kModeOverwrite));
	}

	return false;
}

inline bool Reflex::File::ReadLine(System::FileHandle & file_handle, WString & line)
{
	CString buffer;

	bool valid = ReadLine(file_handle, buffer);

	line.Clear();

	Data::DecodeUTF8(line, Data::Pack(buffer));

	return valid;
}

inline void Reflex::File::WriteLine(System::FileHandle & file, const CString::View & line)
{
	file.Write(line.data, line.size);

	UInt8 eol = 10;

	file.Write(&eol, 1);
}

inline void Reflex::File::WriteLine(System::FileHandle & file, const WString::View & line)
{
	auto buffer = Data::EncodeUTF8(line);

	buffer.GetData()[buffer.GetSize()] = UInt8(10);		//overwrite the guaranteed null terminator to save a Push

	file.Write(buffer.GetData(), buffer.GetSize() + 1);
}
