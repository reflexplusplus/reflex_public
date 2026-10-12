#pragma once

#include "types.h"




//
//Experimental API

namespace Reflex::System
{

	class Socket;

}




//
//Socket

class Reflex::System::Socket : public Object
{
public:

	REFLEX_OBJECT(System::Socket, Object);

	static Socket & null;

	struct Options
	{
		bool secure = false;
		CString server_name;
	};


	//lifetime

	[[nodiscard]] static TRef <Socket> Create(const Options & options);

	virtual void Close() = 0;


	//config

	enum TimeoutType : UInt8
	{
		// On Windows, limits each TCP connection wait and initial TLS I/O wait,
		// rather than the total Connect() duration. DNS resolution is not bounded.
		kConnectTimeout,
		kWriteTimeout,
		kReadTimeout
	};

	virtual void SetTimeout(TimeoutType type, Float timeout_seconds) = 0;


	//connection

	virtual bool Connect(const CString::View & host, UInt16 port) = 0;


	//io

	virtual Optional <UInt> Write(ArrayView <UInt8> data) = 0;

	virtual Optional <UInt> Read(ArrayRegion <UInt8> buffer) = 0;
};
