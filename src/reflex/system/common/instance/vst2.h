#pragma once

#include "reflex/system/ext/vst2api.h"
#include "plugin.h"




//
//vstinstance

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

typedef Reflex::The <PluginSession> ThePluginSession;

constexpr CString::View kSupportShell = "supportShell";

struct Vst2Instance : public PluginInstance
{
	static VST2API::Plugin * VSTPluginMainImpl(VST2API::HostCallback vstcallback);


	REFLEX_OBJECT(Vst2Instance, PluginInstance);

	static void * operator new(size_t size) { return REFLEX_ALLOC16(size); }

	static void operator delete(void * ptr) { return REFLEX_FREE16(ptr); }


	Vst2Instance(PluginSession & pluginsession, VST2API::HostCallback host, const Configuration::Class & cls);

	~Vst2Instance();



	//pluginimpl callbacks

	void OnSetViewSize(const iSize & size) override;


	//instance callbacks

	void ReportProcessingDelay(UInt32 delay) override;

	void ReportStateChange(UInt8 change_flags) override;

	void BeginAutomation(UInt32 idx) override;

	void Automate(UInt32 idx, Float32 value) override;

	void EndAutomation(UInt32 idx) override;


	//object callbacks (because deleted by vst framework)

	void OnDestruct() override {}



	//internal

	void UpdateOutputs();



	//internal

	void GetPinProperties(bool output, Int32 index, VST2API::PinProperties & properties);

	bool GetParameterProperties(Int32 index, VST2API::ParameterProperties & p);
	void GetParameterLabel(Int32 idx, char * label);
	void GetParameterDisplay(Int32 idx, char * text);
	void GetParameterName(Int32 idx, char * text);

	static Float32 REFLEX_STDCALL OnGetParameter(VST2API::Plugin & e, Int32 index);
	static void REFLEX_STDCALL OnSetParameter(VST2API::Plugin & e, Int32 index, Float32 value);
	static void REFLEX_STDCALL OnProcessReplacing(VST2API::Plugin & e, Float32 ** inputs, Float32 ** outputs, Int32 sample_frames);
	static UIntNative REFLEX_STDCALL OnDispatch(VST2API::Plugin & e, Int32 opcode, Int32 index, UIntNative value, void * ptr, Float32 opt);

	static void UpdateDisplay(void * self);



	//vst

	VST2API::HostCallback m_audiomaster;

	VST2API::Plugin m_aeffect;



	//config

	UInt16 m_automating;



	//state

	Array <UInt8> m_chunk;

	VST2API::EditorRect m_editor_rect;



	//processing

	struct VstEventsOut
	{
		VST2API::Events vstevents;
		VST2API::Event * pmidievents[64];
		VST2API::MidiEvent midievents[64];
	};

	VstEventsOut m_vsteventsout;


	static constexpr UInt32 st_timeinfo_filter = VST2API::TimeInfo::kFlagPpqPositionValid | VST2API::TimeInfo::kFlagTempoValid;

	static inline const CString::View kCanDos[5] = { VST2API::kCanDoSendEvents, VST2API::kCanDoSendMidiEvent, VST2API::kCanDoReceiveEvents, VST2API::kCanDoReceiveMidiEvent, VST2API::kCanDoReceiveTimeInfo };

};

REFLEX_INLINE VST2API::Plugin * Vst2Instance::VSTPluginMainImpl(Reflex::VST2API::HostCallback vstcallback)
{
	REFLEX_ASSERT(ThePluginSession::IsAwake());

	VST2API::Plugin * null = nullptr;

	if (vstcallback(*null, VST2API::kHostVersion, 0, 0, 0, 0))
	{
		auto session = ThePluginSession::Get();

		auto & classes = session->config.classes;

		if (UInt32 pluginid = True(classes.GetSize() == 1) ? classes.GetFirst().vst2.uid : Reinterpret<UInt32>(vstcallback(*null, VST2API::kHostCurrentId, 0, 0, 0, 0)))
		{
			for (auto & i : classes)
			{
				if (i.vst2.uid == pluginid)
				{
					return &(new Vst2Instance(session, vstcallback, i))->m_aeffect;	//DO NOT USE REFLEX_CREATE MACRO, incompatible with operator new
				}
			}

			return 0;
		}
		else
		{
			struct Shell : public VST2API::Plugin
			{
				UInt idx = 0;
			};

			auto & aeffect = *(new Shell);

			Reflex::MemClear(&aeffect, sizeof(Shell));
			aeffect.magic = VST2API::kMagic;
			aeffect.set_parameter = [](VST2API::Plugin & e, Int32 index, Float32 value) {};
			aeffect.get_parameter = [](VST2API::Plugin & e, Int32 index) {return 0.0f; };
			aeffect.process_replacing = [](VST2API::Plugin & e, Float32 * *inputs, Float32 * *outputs, Int32 sample_frames) {};
			aeffect.dispatcher = [](VST2API::Plugin & e, Int32 opcode, Int32 index, UIntNative value, void* ptr, Float32 opt) ->UIntNative
			{
				auto & shell = Cast<Shell>(e);

				if (shell.object)	//ableton does this, calls wrong dispatcher
				{
					return shell.dispatcher(shell, opcode, index, value, ptr, opt);
				}
				else
				{
					auto session = ThePluginSession::Get();

					auto & classes = session->config.classes;

					switch (opcode)
					{
					case VST2API::kPluginGetVstVersion:
						return 2400;

					case VST2API::kPluginGetPlugCategory:
						return VST2API::kPluginCategoryShell;

					case VST2API::kPluginCanDo:
						return kSupportShell == ToView(Cast<const char>(ptr));

					case VST2API::kPluginGetVendorString:
						if (classes)
						{
							auto & cls = classes.GetFirst();

							Reflex::RawStringCopy(cls.vendor.GetData(), (char*)ptr, VST2API::kMaxProductStringLength);

							return 1;
						}
						else
						{
							return 0;
						}

					case VST2API::kPluginShellGetNextPlugin:
						if (shell.idx < classes.GetSize())
						{
							auto & cls = classes[shell.idx++];

							Reflex::RawStringCopy(cls.product.GetData(), (char*)ptr, VST2API::kMaxEffectNameLength);

							return cls.vst2.uid;
						}
						return 0;

					case VST2API::kPluginClose:
						delete &shell;
						return 0;
					};
				}

				return 0;
			};

			return &aeffect;
		}
	}

	return 0;
}

REFLEX_END_INTERNAL

