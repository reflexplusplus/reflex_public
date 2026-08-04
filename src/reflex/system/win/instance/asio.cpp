#include "asio.h"
#include "../library.h"
#include "../../common/simd_utils.h"

REFLEX_DISABLE_WARNINGS
#define IEEE754_64FLOAT 1
#include "ext/ASIOSDK2/common/asio.h"
#include "ext/ASIOSDK2/common/iasiodrv.h"
#include "ext/ASIOSDK2/host/pc/asiolist.h"
#include "ext/ASIOSDK2/host/pc/asiolist.cpp"		//(1) added #pragma once to "iasiodrv.h", (2) removed unicode reg key access in asiolist.cpp
REFLEX_ENABLE_WARNINGS

#pragma comment(lib, "Winmm.lib")

#define ASIO_OK(fn, msg) if (fn != ASE_OK) throw(Error(msg));




REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct ASIOImpl : public Common::DesktopAudioAppBase
{
	ASIOImpl();

	~ASIOImpl();


	typedef CString::View Error;

	typedef Array < Pair <WString, HANDLE> > MidiPortList;

	enum AudioFormat
	{
		kAudioFormatUnknown,
		kAudioFormatInt16,
		kAudioFormatInt24,
		kAudioFormatInt32,
		kAudioFormatFloat32,
		kAudioFormatFloat64,

		kNumAudioFormat,
	};

	enum State
	{
		kStateDriver,
		kStateBuffers,
		kStateStart,
	};

	typedef Pair <AudioFormat,CString::View> AsioFormat;


	void OnPause() override;

	void OnResume() override;


	Array < Pair <WString,bool> > GetMidiPorts(bool output) const override;

	void OnEnableMidiPort(bool output, UInt idx, bool enable) override;


	Array <WString> GetAvailableAudioDevices() const override;

	bool OnSelectAudioDevice(const WString::View & value) override;

	WString GetCurrentAudioDevice() const override { return m_current_device; }


	void OnRequestBufferSize(UInt32 value) override;

	UInt32 GetCurrentBufferSize() const override { return m_hw_latency; }


	void OnRequestSampleRate(Float32 value) override;

	Float32 GetCurrentSampleRate() const override { return m_hw_sr; }


	Array <UInt32> GetAvailableBufferSizes() const override { return m_latencies; }

	Array <Float32> GetAvailableSampleRates() const override { return m_samplerates; }


	Array < Pair <WString,bool> > GetAudioChannels(bool output) const override;

	void OnEnableAudioChannel(bool output, UInt32 idx, bool enable) override;


	void OnSetProperty(Address address, Object & object) override;

	void OnQueryProperty(Address address, Object * & pobject) const override;



	bool CollectDevices();

	template <bool OUTPUT> static HANDLE OpenMidiPort(ASIOImpl & self, UInt sytemidx);

	template <bool OUTPUT> static void CloseMidiPort(ASIOImpl & self, HANDLE & handle);


	void CreateBuffers();

	void DisposeBuffers();

	template <bool INPUTS, AudioFormat FORMAT> static void ProcessRt(ASIOImpl & self, UInt32 index);

	void OnError(const CString::View & error);

	void CloseDriver();


	static BOOL __stdcall GetWindow(HWND hwnd, LPARAM pwindows);


	static void CALLBACK MidiInProc(HMIDIIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);




	//midi

	Reference <Object> m_onchangedevices;

	MidiPortList m_midiports[2];

	Array <HANDLE> m_active_midiports[2];


	AsioDriverList m_asio_drivers;

	ASIOCallbacks m_asio_callbacks;


	Array < Pair<WString,ASIODRVSTRUCT*> > m_driverlist;

	IASIO * m_asio_current;

	UInt8 m_state;


	Array <ASIOBufferInfo> m_asio_buffers;

	WString m_current_device;


	Array <UInt32> m_latencies;

	UInt32 m_hw_latency;


	Array <Float32> m_samplerates;

	Float32 m_hw_sr;


	struct Channel
	{
		Channel()
			: enabled(false)
		{
		}

		WString label;

		bool enabled;

		Array <Float32> buffer;

		ASIOBufferInfo * bufferinfo;
	};

	Array <Channel> m_channels[2];

	Pair <AudioFormat> m_audioformat;


	Array <ASIOBufferInfo*> m_active_asioinputs;

	Array <ASIOBufferInfo*> m_active_asiooutputs;

	Array <ASIOBufferInfo*> m_inactive_asiooutputs;


	mutable ObjectOf <bool> m_has_control_panel;


	static ASIOImpl * st_self;

	static const decltype(&OpenMidiPort<false>) kOpenMidiPorts[2];

	static const decltype(&CloseMidiPort<false>) kCloseMidiPorts[2];

	static FunctionPointer <void(ASIOImpl&,UInt32)> st_processfn;

	static const AsioFormat kAsioChannelFormats[ASIOSTLastEntry];

	static FunctionPointer <void(ASIOImpl&,UInt32)> kProcessFns[2][kNumAudioFormat];
};

ASIOImpl::ASIOImpl()
	: m_hw_latency(kDefaultBufferSize)
	, m_hw_sr(kDefaultSampleRate)
	, m_audioformat({ kAudioFormatUnknown,kAudioFormatUnknown })
	, m_state(0)
{
	//init

	st_self = this;

	kProcessFns[0][kAudioFormatUnknown] = &ASIOImpl::ProcessRt<false,kAudioFormatUnknown>;
	kProcessFns[0][kAudioFormatInt16] = &ASIOImpl::ProcessRt<false,kAudioFormatInt16>;
	kProcessFns[0][kAudioFormatInt24] = &ASIOImpl::ProcessRt<false,kAudioFormatInt24>;
	kProcessFns[0][kAudioFormatInt32] = &ASIOImpl::ProcessRt<false,kAudioFormatInt32>;
	kProcessFns[0][kAudioFormatFloat32] = &ASIOImpl::ProcessRt<false,kAudioFormatFloat32>;
	kProcessFns[0][kAudioFormatFloat64] = &ASIOImpl::ProcessRt<false,kAudioFormatFloat64>;

	kProcessFns[1][kAudioFormatUnknown] = &ASIOImpl::ProcessRt<true,kAudioFormatUnknown>;
	kProcessFns[1][kAudioFormatInt16] = &ASIOImpl::ProcessRt<true,kAudioFormatInt16>;
	kProcessFns[1][kAudioFormatInt24] = &ASIOImpl::ProcessRt<true,kAudioFormatInt24>;
	kProcessFns[1][kAudioFormatInt32] = &ASIOImpl::ProcessRt<true,kAudioFormatInt32>;
	kProcessFns[1][kAudioFormatFloat32] = &ASIOImpl::ProcessRt<true,kAudioFormatFloat32>;
	kProcessFns[1][kAudioFormatFloat64] = &ASIOImpl::ProcessRt<true,kAudioFormatFloat64>;



	//query asio devices

	ASIODRVSTRUCT * driverinfo = m_asio_drivers.lpdrvlist;

	while (driverinfo)
	{
		auto & driver = m_driverlist.Push();

		driver.a = ToWString(CString::View(driverinfo->drvname + 0));

		driver.b = driverinfo;

		driverinfo = driverinfo->next;
	}

	m_asio_current = 0;



	//callbacks

	m_asio_callbacks.asioMessage = [](long selector, long value, void * message, double * opt) -> long
	{
		auto & self = *st_self;

		switch (selector)
		{
		case kAsioSelectorSupported:
			if (value == kAsioEngineVersion || value == kAsioSupportsTimeInfo || value == kAsioResetRequest) return 1;

		case kAsioEngineVersion:
			return 2L;

		case kAsioSupportsTimeInfo:
			return 1;

		case kAsioResetRequest:
			if (GetThreadID() == kMainThreadID)
			{
				Lock lock(self);
			}
			return 0;
		}

		return 0;
	};

	m_asio_callbacks.sampleRateDidChange = [](ASIOSampleRate sRate) {};

	m_asio_callbacks.bufferSwitch = [](long index, ASIOBool processnow)
	{
		st_processfn(*st_self, index);
	};

	m_asio_callbacks.bufferSwitchTimeInfo = [](ASIOTime*, long index, ASIOBool processnow) -> ASIOTime *
	{
		st_processfn(*st_self, index);

		return 0L;
	};

	m_onchangedevices = System::CreateListener(kNotificationChangeDevices, this, [](void * client)
	{
		auto & self = *st_self;

		Lock lock(self);

		self.CollectDevices();
	});


	CollectDevices();
}

ASIOImpl::~ASIOImpl()
{
	//stop midi

	REFLEX_LOOP(idx, 2) for (auto & i : m_midiports[idx]) kCloseMidiPorts[idx](*this, i.b);


	//stop audio

	REFLEX_ASSERT(!BitCheck(m_state, kStateStart));

	CloseDriver();
}

Array < Pair <WString,bool> > ASIOImpl::GetMidiPorts(bool output) const
{
	Array < Pair <WString,bool> > rtn;

	auto & ports = m_midiports[output];

	rtn.Allocate(ports.GetSize());

	for (auto & i : ports) rtn.Push({ i.a, True(i.b) });

	return rtn;
}

void ASIOImpl::OnEnableMidiPort(bool output, UInt idx, bool enable)
{
	//ProcessingLock lock(*this);

	auto & ports = m_midiports[output];

	if (idx < ports.GetSize())
	{
		Common::output.Log("ASIOImpl::EnableMidiPort", output, ports[idx].a, enable);

		auto & port = ports[idx];

		kCloseMidiPorts[output](*this, port.b);

		if (enable) port.b = kOpenMidiPorts[output](*this, idx);
	}
}

Array <WString> ASIOImpl::GetAvailableAudioDevices() const
{
	Array <WString> values;

	values.Allocate(m_driverlist.GetSize());

	for (auto & i : m_driverlist) values.Push(i.a);

	return values;
}

bool ASIOImpl::OnSelectAudioDevice(const WString::View & value)
{
	//ProcessingLock lock(*this);

	try
	{
		//open driver

		for (auto & i : m_driverlist)
		{
			if (i.a == value)
			{
				m_audioformat = { kAudioFormatUnknown, kAudioFormatUnknown };

				m_samplerates.Clear();

				m_latencies.Clear();

				m_channels[0].Clear();

				m_channels[1].Clear();

				CloseDriver();


				ASIODRVSTRUCT * drvstruct = i.b;

				Common::output.Log("ASIOImpl::SetAudioDevice", i.a);

				void * pvoid = 0;

				if (drvstruct) CoCreateInstance(drvstruct->clsid, 0, CLSCTX_INPROC_SERVER, drvstruct->clsid, &pvoid);

				m_asio_current = static_cast<IASIO*>(pvoid);

				if (!m_asio_current) throw(Error("CoCreateInstance"));

				if (!m_asio_current->init(GetDesktopWindow()))	//was GetDesktopWindow() until 3/4/2018
				{
					char buffer[512] = { 0 };

					m_asio_current->getErrorMessage(buffer);

					buffer[511] = 0;

					CString error = Join("init: ", buffer);

					throw(Error(error));
				}



				//channels

				long numchannels[2] = { 0,0 };

				ASIO_OK(m_asio_current->getChannels(numchannels, numchannels + 1), "getChannels");



				//latency

				long min, max, pref, step;

				ASIO_OK(m_asio_current->getBufferSize(&min, &max, &pref, &step), "getBufferSize");

				step = Max<long>(step, 1);

				UInt32 range = (max - min) + 1;

				m_latencies.Clear();

				for (auto & i : kStandardBufferSizes)
				{
					if (Inside(i, UInt32(min), range))
					{
						m_latencies.Push(QuantiseDown<UInt32>(i, step));
					}
				}

				if (!m_latencies) throw(Error("no buffersizes"));

				if (!Search(m_latencies, m_hw_latency))
				{
					if (Search(m_latencies, UInt32(pref)))
					{
						m_hw_latency = pref;
					}
					else
					{
						m_hw_latency = m_latencies.GetLast();
					}
				}



				//samplerate

				m_samplerates.Clear();

				for (auto & i : kStandardSampleRates)
				{
					Float64 samplerate = i;

					REFLEX_STATIC_ASSERT(sizeof(ASIOSampleRate) == sizeof(Float64));

					if (m_asio_current->canSampleRate(samplerate) == ASE_OK)
					{
						m_samplerates.Push(i);
					}
				}

				if (!m_samplerates) throw(Error("no samplerates"));

				if (!Search(m_samplerates, m_hw_sr)) m_hw_sr = m_samplerates.GetFirst();



				//channel names

				CString::View labels[2] = { "In", "Out" };

				ASIOChannelInfo temp;

				temp.isInput = ASIOTrue;

				auto audioformat = &m_audioformat.a;

				REFLEX_LOOP(output, 2)
				{
					auto & channels = m_channels[output];

					channels.SetSize(numchannels[output]);

					REFLEX_LOOP(chidx, channels.GetSize())
					{
						auto & channel = channels[chidx];

						temp.channel = chidx;

						m_asio_current->getChannelInfo(&temp);

						auto format = kAsioChannelFormats[temp.type];

						DEV_LOG(labels[output], chidx, ":", format.b);

						(*audioformat) = format.a;

						channel.enabled = false;

						channel.label = ToWString(CString::View(temp.name + 0));
					}

					temp.isInput = ASIOFalse;

					audioformat++;
				}

				m_current_device = i.a;

				return true;
			}
		}
	}
	catch (const Error & error)
	{
		OnError(error);
	}

	return false;
}

void ASIOImpl::OnRequestSampleRate(Float32 value)
{
	Common::output.Log("ASIOImpl::SelectSampleRate", value);

	//ProcessingLock lock(*this);

	m_hw_sr = value;
}

void ASIOImpl::OnRequestBufferSize(UInt32 value)
{
	Common::output.Log("ASIOImpl::SelectBufferSize ", value);

	//ProcessingLock lock(*this);

	m_hw_latency = value;
}

Array < Pair <WString, bool> > ASIOImpl::GetAudioChannels(bool output) const
{
	Array < Pair <WString, bool> > rtn;

	auto & channels = m_channels[output];

	rtn.Allocate(channels.GetSize());

	for (auto & i : channels) rtn.Push({ i.label, i.enabled });

	return rtn;
}

void ASIOImpl::OnEnableAudioChannel(bool output, UInt32 idx, bool enable)
{
	//ProcessingLock lock(*this);

	m_channels[output][idx].enabled = enable;
}

void ASIOImpl::OnError(const CString::View & error)
{
	Common::output.Log("ASIOImpl Error", error);

	m_current_device.Clear();

	m_active_asioinputs.Clear();

	m_active_asiooutputs.Clear();

	m_inactive_asiooutputs.Clear();

	ClearBuffers();

	CloseDriver();

	m_audioformat = { kAudioFormatUnknown, kAudioFormatUnknown };

	st_processfn = kProcessFns[0][0];
}

void ASIOImpl::CloseDriver()
{
	if (m_asio_current)
	{
		if (BitCheck(m_state, kStateStart)) m_asio_current->stop();

		if (BitCheck(m_state, kStateBuffers)) m_asio_current->disposeBuffers();

		m_asio_current->Release();

		m_state = 0;

		m_asio_current = 0;
	}

	m_asio_buffers.Clear();
}

void ASIOImpl::OnPause()
{
	if (BitCheck(m_state, kStateStart))
	{
		st_processfn = kProcessFns[0][0];

		m_asio_current->stop();

		m_state = BitClear(m_state, kStateStart);
	}
}

void ASIOImpl::OnResume()
{
	DEV_LOG(m_driverlist);

	REFLEX_ASSERT(!BitCheck(m_state, kStateStart));

	m_state = BitClear(m_state, kStateStart);	//unnecessary

	try
	{
		if (!m_asio_current) throw(Error("no driver"));

		if (!m_hw_latency) throw(Error("no latency"));

		CreateBuffers();	//TODO, only need to do this when buffersize or ports change

		if (PrepareProcessing(m_hw_latency, m_hw_sr))
		{
			m_asio_current->setSampleRate(m_hw_sr);

			ASIOSampleRate asiosr;

			if (m_asio_current->getSampleRate(&asiosr) == ASE_OK)
			{
				m_hw_sr = Float(asiosr);

				PrepareProcessing(m_hw_latency, m_hw_sr);
			}
		}

		ASIO_OK(m_asio_current->start(), "start");

		m_state = BitSet(m_state, kStateStart);

		st_processfn = kProcessFns[m_audioformat.a == m_audioformat.b][m_audioformat.b];

		DEV_LOG("ASIOImpl::Resume OK");
	}
	catch (const Error & error)
	{
		OnError(error);
	}
}

template <bool OUTPUT> HANDLE ASIOImpl::OpenMidiPort(ASIOImpl & self, UInt idx)
{
	HANDLE handle = 0;

	if constexpr (OUTPUT)
	{
		HMIDIOUT midiport = 0;

		midiOutOpen(&midiport, idx, 0, 0, CALLBACK_NULL);

		handle = midiport;
	}
	else
	{
		HMIDIIN midiport = 0;

		if (midiInOpen(&midiport, idx, DWORD_PTR(&ASIOImpl::MidiInProc), idx, CALLBACK_FUNCTION) == MMSYSERR_NOERROR)
		{
			midiInStart(midiport);
		}

		handle = midiport;
	}

	auto & active = self.m_active_midiports[OUTPUT];

	if (!SearchValue(active, handle)) active.Push(handle);

	return handle;
}

template <bool OUTPUT> void ASIOImpl::CloseMidiPort(ASIOImpl & self, HANDLE & handle)
{
	Remove(self.m_active_midiports[OUTPUT], handle);

	if constexpr (OUTPUT)
	{
		midiOutClose(HMIDIOUT(handle));
	}
	else
	{
		midiInReset(HMIDIIN(handle));

		midiInClose(HMIDIIN(handle));
	}

	handle = nullptr;
}

void ASIOImpl::DisposeBuffers()
{
	if (BitCheck(m_state, kStateBuffers)) ASIO_OK(m_asio_current->disposeBuffers(), "disposeBuffers");

	m_state = BitClear(m_state, kStateBuffers);
}

void ASIOImpl::CreateBuffers()
{
	UInt32 buffersize = m_hw_latency;

	DEV_LOG("ASIOImpl::CreateBuffers", buffersize);

	DisposeBuffers();

	Int32 numinput = m_channels[0].GetSize();

	Int32 numoutput = m_channels[1].GetSize();

	m_asio_buffers.SetSize(numinput + numoutput);

	auto pasiobuffer = m_asio_buffers.GetData();


	REFLEX_LOOP(output, 2)
	{
		auto & channels = m_channels[output];

		auto isinput = output ? ASIOFalse : ASIOTrue;

		REFLEX_LOOP(idx, channels.GetSize())
		{
			auto & bufferinfo = *pasiobuffer++;

			bufferinfo.isInput = isinput;
			bufferinfo.channelNum = idx;
			bufferinfo.buffers[0] = 0;
			bufferinfo.buffers[1] = 0;

			channels[idx].bufferinfo = &bufferinfo;
		}

		//REFLEX_LOOP(idx, numoutput)
		//{
		//	auto & bufferinfo = m_asio_buffers[numinput + idx];

		//	bufferinfo.isInput = ASIOFalse;
		//	bufferinfo.channelNum = idx;
		//	bufferinfo.buffers[0] = 0;
		//	bufferinfo.buffers[1] = 0;

		//	m_outputs[idx].bufferinfo = &bufferinfo;
		//}
	}

	ASIO_OK(m_asio_current->createBuffers(m_asio_buffers.GetData(), m_asio_buffers.GetSize(), buffersize, &m_asio_callbacks), "createBuffers");

	m_state = BitSet(m_state, kStateBuffers);



	//inputs

	ClearBuffers();

	m_active_asioinputs.Clear();

	for (auto & i : m_channels[0])
	{
		if (i.enabled)
		{
			i.buffer.SetSize(m_hw_latency);

			i.buffer.Wipe();

			AddBuffer(false, i.buffer.GetData());

			m_active_asioinputs.Push(i.bufferinfo);
		}
		else
		{
			i.buffer.Clear();
		}
	}



	//outputs

	m_active_asiooutputs.Clear();

	m_inactive_asiooutputs.Clear();

	for (auto & i : m_channels[1])
	{
		if (i.enabled)
		{
			i.buffer.SetSize(m_hw_latency);

			i.buffer.Wipe();

			AddBuffer(true, i.buffer.GetData());

			m_active_asiooutputs.Push(i.bufferinfo);
		}
		else
		{
			i.buffer.Clear();

			m_inactive_asiooutputs.Push(i.bufferinfo);
		}
	}
}

template <bool INPUTS, ASIOImpl::AudioFormat FORMAT> void ASIOImpl::ProcessRt(ASIOImpl & self, UInt32 dbl_bfr_idx)
{
	if constexpr (FORMAT == kAudioFormatUnknown) return;


	ProcessingScopeRt scope(&self);

	if (scope.IsLocked()) return;


	UInt32 buffersize = self.m_hw_latency;

	UInt32 nblock = buffersize / 4;

	UInt32 remainder = buffersize - (nblock * 4);


	//asio input

	if constexpr (INPUTS)
	{
		auto & inputs = self.GetBuffers(false);

		REFLEX_LOOP(idx, inputs.GetSize())
		{
			void * input = self.m_active_asioinputs[idx]->buffers[dbl_bfr_idx];

			Float32 * channel = RemoveConst(inputs[idx]);

			if constexpr (FORMAT == kAudioFormatInt16)
			{
				Common::FromIntStream<Int16>::Convert(nblock, remainder, input, channel);
			}
			else if constexpr (FORMAT == kAudioFormatInt24)
			{
				auto mult = Common::FromIntStream<Int32>::kMult;

				const Int8 * source = Reinterpret<Int8>(input);

				REFLEX_LOOP(sample, buffersize)
				{
					Int32 v24bit = *Reinterpret<Int>(source);

					source += 3;

					v24bit = v24bit << 8;

					(*channel++) = Float(Float64(v24bit) * mult);
				}
			}
			else if constexpr (FORMAT == kAudioFormatInt32)
			{
				Common::FromIntStream<Int32>::Convert(nblock, remainder, input, channel);
			}
			else if constexpr (FORMAT == kAudioFormatFloat32)
			{
				MemCopy(input, channel, buffersize << 2);
			}
			else if constexpr (FORMAT == kAudioFormatFloat64)
			{
				const Float64 * source = Reinterpret<Float64>(input);

				REFLEX_LOOP(block, nblock)
				{
					*channel++ = Float32(*source++);
					*channel++ = Float32(*source++);
					*channel++ = Float32(*source++);
					*channel++ = Float32(*source++);
				}

				REFLEX_LOOP(sample, remainder) *channel++ = Float32(*source++);
			}
		}
	}



	//process instance

	auto eventsout = self.Common::DesktopAudioAppBase::ProcessRt(scope, self.m_hw_latency);



	//asio output (tidy up like inputs with ToStream class)

	auto & outputs = self.GetBuffers(true);

	REFLEX_LOOP(idx, outputs.GetSize())
	{
		ASIOBufferInfo & asio_output = *self.m_active_asiooutputs[idx];

		if constexpr (FORMAT == kAudioFormatInt16)
		{
			const auto * source = Reinterpret<Float32x4>(outputs[idx]);

			Int16 * dest = Reinterpret<Int16>(asio_output.buffers[dbl_bfr_idx]);

			REFLEX_LOOP(block, nblock)
			{
				auto clipped = REFLEX_MIN_F32_X4(REFLEX_MAX_F32_X4(*source++, Common::kMinusOne_V4), Common::kOne_V4);

				auto output = REFLEX_F32_X4_TO_I32_X4(REFLEX_MUL_F32_X4(clipped, Common::kRangeInt16_V4));

				(*dest++) = Int16(output.m128i_i32[0]);
				(*dest++) = Int16(output.m128i_i32[1]);
				(*dest++) = Int16(output.m128i_i32[2]);
				(*dest++) = Int16(output.m128i_i32[3]);
			}

			REFLEX_LOOP(sample, remainder)
			{
				Int32 value = ToInt32(Clip(source->m128_f32[sample], -1.0f, 1.0f) * Common::kRangeInt16);

				(*dest++) = Int16(value);
			}
		}
		else if constexpr (FORMAT == kAudioFormatInt24)
		{
			const Float32 * source = outputs[idx];

			Int8 * dest = Reinterpret<Int8>(asio_output.buffers[dbl_bfr_idx]);

			REFLEX_LOOP(sample, buffersize)
			{
				Int32 value = ToInt32(Float64(Clip(*source++, -1.0f, 1.0f)) * 8388607.0);

				const Int8 * bytes = Reinterpret<Int8>(&value);

				(*dest++) = bytes[0];
				(*dest++) = bytes[1];
				(*dest++) = bytes[2];
			}
		}
		else if constexpr (FORMAT == kAudioFormatInt32)
		{
			constexpr Float64 kRangeInt32 = 2147483647.0;

			auto source = Reinterpret<Float32x4>(outputs[idx]);

			Int32 * dest = Reinterpret<Int32>(asio_output.buffers[dbl_bfr_idx]);

			REFLEX_LOOP(block, nblock)
			{
				auto clipped = REFLEX_MIN_F32_X4(REFLEX_MAX_F32_X4(*source++, Common::kMinusOne_V4), Common::kOne_V4);

				(*dest++) = ToInt32(Float64(clipped.m128_f32[0]) * kRangeInt32);
				(*dest++) = ToInt32(Float64(clipped.m128_f32[1]) * kRangeInt32);
				(*dest++) = ToInt32(Float64(clipped.m128_f32[2]) * kRangeInt32);
				(*dest++) = ToInt32(Float64(clipped.m128_f32[3]) * kRangeInt32);
			}

			REFLEX_LOOP(sample, remainder)
			{
				Float32 output = Clip(source->m128_f32[sample], -1.0f, 1.0f);

				(*dest++) = ToInt32(Float64(output) * kRangeInt32);
			}
		}
		else if constexpr (FORMAT == kAudioFormatFloat32)
		{
			MemCopy(outputs[idx], asio_output.buffers[dbl_bfr_idx], buffersize << 2);	//buffersize * 4
		}
		else if constexpr (FORMAT == kAudioFormatFloat64)
		{
			auto source = Reinterpret<Float32x4>(outputs[idx]);

			Float64 * dest = Reinterpret<Float64>(asio_output.buffers[dbl_bfr_idx]);

			REFLEX_LOOP(block, nblock)
			{
				auto clipped = REFLEX_MIN_F32_X4(REFLEX_MAX_F32_X4(*source++, Common::kMinusOne_V4), Common::kOne_V4);

				(*dest++) = clipped.m128_f32[0];
				(*dest++) = clipped.m128_f32[1];
				(*dest++) = clipped.m128_f32[2];
				(*dest++) = clipped.m128_f32[3];
			}

			REFLEX_LOOP(sample, remainder)
			{
				(*dest++) = Clip(source->m128_f32[sample], -1.0f, 1.0f);
			}
		}
	}


	//asio outputs (inactive)

	switch (FORMAT)
	{
	case kAudioFormatInt16:
		buffersize *= 2;
		break;

	case kAudioFormatInt24:
		buffersize *= 3;
		break;

	case kAudioFormatInt32:
	case kAudioFormatFloat32:
		buffersize *= 4;
		break;

	case kAudioFormatFloat64:
		buffersize *= 8;
		break;

	default:
		buffersize = 0;
		break;
	};

	for (auto & i : self.m_inactive_asiooutputs) MemClear(i->buffers[dbl_bfr_idx], buffersize);

	self.m_asio_current->outputReady();



	//midi out

	for (auto & e : eventsout)
	{
		for (auto & i : self.m_active_midiports[1]) midiOutShortMsg((HMIDIOUT)i, e.value.u32);
	}
}

bool ASIOImpl::CollectDevices()
{
	bool changed = false;

	UInt nports[] = { midiInGetNumDevs(), midiOutGetNumDevs() };

	typedef decltype(&midiOutGetDevCapsW) GetMidiDevCapsFn;

	Pair <GetMidiDevCapsFn,UInt> GetMidiDevCapsFns[] = 
	{
		{ reinterpret_cast<GetMidiDevCapsFn>(&midiInGetDevCapsW), sizeof(MIDIINCAPSW) },
		{ &midiOutGetDevCapsW, sizeof(MIDIOUTCAPSW) }
	};

	REFLEX_STATIC_ASSERT(REFLEX_OFFSETOF(MIDIINCAPSW, szPname) == REFLEX_OFFSETOF(MIDIOUTCAPSW, szPname));

	MidiPortList temp;

	MIDIOUTCAPSW midi_caps;

	REFLEX_LOOP(output, 2)
	{
		temp.Clear();

		UInt nport = nports[output];

		auto & ports = m_midiports[output];

		if (nport != ports.GetSize())
		{
			auto [GetMidiDevCaps,size] = GetMidiDevCapsFns[output];

			auto CloseMidiPort = kCloseMidiPorts[output];

			auto OpenMidiPort = kOpenMidiPorts[output];

			changed = true;

			REFLEX_LOOP(idx, nport)
			{
				GetMidiDevCaps(idx, &midi_caps, size);

				temp.Push({ midi_caps.szPname, 0 });
			}

			for (auto & i : ports)
			{
				if (HANDLE & handle = i.b)
				{
					CloseMidiPort(*this, handle);

					auto & name = i.a;

					if (auto portidx = Search<KeyCompare>(temp, name))
					{
						temp[portidx.value].b = OpenMidiPort(*this, portidx.value);
					}
				}
			}

			ports.Swap(temp);
		}
	}

	return changed;
}

BOOL __stdcall ASIOImpl::GetWindow(HWND hwnd, LPARAM pwindows)
{
	if (IsWindowVisible(hwnd))
	{
		auto & windows = *ToPointer< Array <HWND> >(Reinterpret<UIntNative>(pwindows));

		if (!Search(windows, hwnd)) windows.Push(hwnd);
	}

	return true;
}

void ASIOImpl::OnSetProperty(Address address, Object & object)
{
	struct Handler : public Object
	{
		Handler(ASIOImpl & asio)
		{
			EnumWindows(&GetWindow, ToUIntNative(&m_windows));

			m_clock = CreateListener(kNotificationClock, this, &Handler::OnClock);

			m_lock.Init(asio);

			try
			{
				asio.DisposeBuffers();
			}
			catch (const Error &)
			{
			}
		}

		~Handler()
		{
			m_lock.Deinit();
		}

		static void OnClock(void * data)
		{
			auto & self = *Reinterpret<Handler>(data);

			if (self.m_countdown == 0)
			{
				bool changed = false;

				for (auto & i : self.m_windows)
				{
					if (IsWindow(i))
					{
						if (IsWindowVisible(i)) continue;
					}

					changed = true;
				}

				if (changed)
				{
					auto asio = Cast<ASIOImpl>(self.m_lock->audioplugin);

					if (auto asio_current = asio->m_asio_current)
					{
						long min, max, pref, step;

						ASIO_OK(asio_current->getBufferSize(&min, &max, &pref, &step), "getBufferSize");

						asio->m_hw_latency = pref;
					}

					globals->m_signals[kNotificationChangeDevices].Notify();

					self.ReleaseSt();
				}
			}
			else if (self.m_countdown == 1)
			{
				Array <HWND> windows;

				EnumWindows(&GetWindow, ToUIntNative(&windows));

				if (self.m_windows != windows)
				{
					for (auto & i : self.m_windows)
					{
						Remove(windows, i);
					}

					if (windows)
					{
						self.m_windows = windows;

						self.m_countdown = 0;
					}
					else
					{
						self.ReleaseSt();
					}
				}
			}
			else
			{
				self.m_countdown--;
			}
		}

		Reflex::Detail::Initialiser <AudioPlugin::Lock> m_lock;

		Reference <Object> m_clock;

		Array <HWND> m_windows;

		UInt m_countdown = 10;
	};

	if (address == MakeAddress<ObjectOf<bool>>(K32("ShowControlPanel")))
	{
		if (Cast<ObjectOf<bool>>(object)->value)
		{
			if (m_asio_current)
			{
				auto handler = REFLEX_CREATE(Handler, *this);

				handler->RetainSt();

				m_asio_current->controlPanel();
			}
		}
	}

	Common::DesktopAudioAppBase::OnSetProperty(address, object);
}

void ASIOImpl::OnQueryProperty(Address address, Object * & pobject) const
{
	if (address == MakeAddress<ObjectOf<bool>>(K32("HasControlPanel")))
	{
		m_has_control_panel.value = True(m_asio_current);

		pobject = &m_has_control_panel;
	}
	else
	{
		Common::DesktopAudioAppBase::OnQueryProperty(address, pobject);
	}
}

void ASIOImpl::MidiInProc(HMIDIIN, UINT id0, DWORD_PTR idx, DWORD_PTR msg, DWORD_PTR)
{
	st_self->QueueEvent(Event::kTypeMIDI, 0, (UInt16)idx, (UInt32)msg);
}

ASIOImpl * ASIOImpl::st_self = 0;

const decltype(&ASIOImpl::CloseMidiPort<false>) ASIOImpl::kCloseMidiPorts[] = { &CloseMidiPort<false>, &CloseMidiPort<true> };

const decltype(&ASIOImpl::OpenMidiPort<false>) ASIOImpl::kOpenMidiPorts[] = { &OpenMidiPort<false>, &OpenMidiPort<true> };

FunctionPointer <void(ASIOImpl&,UInt32)> ASIOImpl::st_processfn = &ASIOImpl::ProcessRt<false,kAudioFormatUnknown>;

FunctionPointer <void(ASIOImpl&,UInt32)> ASIOImpl::kProcessFns[2][kNumAudioFormat];

const Pair<ASIOImpl::AudioFormat,CString::View> ASIOImpl::kAsioChannelFormats[ASIOSTLastEntry] =
{
	{ kAudioFormatUnknown,"ASIOSTInt16MSB" },
	{ kAudioFormatUnknown,"ASIOSTInt24MSB" },
	{ kAudioFormatUnknown,"ASIOSTInt32MSB" },
	{ kAudioFormatUnknown,"ASIOSTFloat32MSB" },
	{ kAudioFormatUnknown,"ASIOSTFloat64MSB" },
	{ kAudioFormatUnknown,"5" },
	{ kAudioFormatUnknown,"6" },
	{ kAudioFormatUnknown,"7" },
	{ kAudioFormatUnknown,"ASIOSTInt32MSB16" },
	{ kAudioFormatUnknown,"ASIOSTInt32MSB18" },
	{ kAudioFormatUnknown,"ASIOSTInt32MSB20" },
	{ kAudioFormatUnknown,"ASIOSTInt32MSB24" },
	{ kAudioFormatUnknown,"12" },
	{ kAudioFormatUnknown,"13" },
	{ kAudioFormatUnknown,"14" },
	{ kAudioFormatUnknown,"15" },

	{ kAudioFormatInt16,"ASIOSTInt16LSB" },
	{ kAudioFormatInt24,"ASIOSTInt24LSB" },
	{ kAudioFormatInt32,"ASIOSTInt32LSB" },
	{ kAudioFormatFloat32,"ASIOSTFloat32LSB" },
	{ kAudioFormatFloat64,"ASIOSTFloat64LSB" },

	{ kAudioFormatUnknown,"21" },
	{ kAudioFormatUnknown,"22" },
	{ kAudioFormatUnknown,"23" },
	{ kAudioFormatUnknown,"ASIOSTInt32LSB16" },
	{ kAudioFormatUnknown,"ASIOSTInt32LSB18" },
	{ kAudioFormatUnknown,"ASIOSTInt32LSB20" },
	{ kAudioFormatUnknown,"ASIOSTInt32LSB24" },
	{ kAudioFormatUnknown,"28" },
	{ kAudioFormatUnknown,"29" },
	{ kAudioFormatUnknown,"30" },
	{ kAudioFormatUnknown,"31" },
	{ kAudioFormatUnknown,"ASIOSTDSDInt8LSB1" },
	{ kAudioFormatUnknown,"ASIOSTDSDInt8MSB1" },
	{ kAudioFormatUnknown,"34" },
	{ kAudioFormatUnknown,"35" },
	{ kAudioFormatUnknown,"36" },
	{ kAudioFormatUnknown,"37" },
	{ kAudioFormatUnknown,"38" },
	{ kAudioFormatUnknown,"39" },
	{ kAudioFormatUnknown,"ASIOSTDSDInt8NER8" },
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Common::DesktopAudioAppBase> Reflex::System::Win::CreateASIO()
{
	return REFLEX_CREATE(ASIOImpl);
}

Reflex::WString Reflex::System::AudioPlugin::GetDefaultDevice(bool)
{
	return {};
}
