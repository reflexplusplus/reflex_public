#include "plugin.h"
#include "reflex/system/ext/vst3api.h"

#include "instance.cpp"
#include "plugin.cpp"




REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

REFLEX_NOINLINE void RawStringCopy(const CString::View & from, VST3API::String128 out)
{
	Reflex::RawStringCopy(from.data, out, Min<UInt>(from.size, 128));
}

template <class COMPONENT> inline COMPONENT * QueryInterface(VST3API::IUnknown * funknown)
{
	if (funknown)
	{
		VST3API::IUnknown * ptr = nullptr;

		if (funknown->queryInterface(VST3API::ClassInfo<COMPONENT>::iid, ptr) == VST3API::kResultOk) return Cast<COMPONENT>(ptr);
	}

	return nullptr;
}

template <typename... INTERFACES>
struct ComComponent : public INTERFACES...
{
	typedef VST3API::UID UID;

	typedef VST3API::IUnknown IUnknown;

	struct Callback
	{
		template <UInt IDX, class IFACE, class COMPONENT> static void Visit(Tuple <COMPONENT*,const UID&,IUnknown*&,VST3API::tresult> & data)
		{
			REFLEX_USE(VST3API);

			if (data.d == kResultFalse)
			{
				typedef NonRefT <IFACE> Type;

				typedef typename BaseClass<Type>::Type BaseType;

				if (VST3API::ClassInfo<Type>::iid == data.b)
				{
					data.c = Cast<Type>(data.a);

					data.c->addRef();

					data.d = kResultOk;

					return;
				}

				if constexpr (!IsType<BaseType,void>::value)
				{
					Tuple <Type*,const UID&,IUnknown*&,tresult> subclass = { Cast<Type>(data.a), data.b, data.c, kResultFalse };

					Visit<IDX,BaseType>(subclass);

					data.c = subclass.c;

					data.d = subclass.d;
				}
			}
		}
	};

	VST3API::tresult REFLEX_STDCALL queryInterface(const UID & iid, IUnknown * & obj) override
	{
		REFLEX_USE(VST3API);

		Tuple <ComComponent*,const UID&,IUnknown*&,tresult> data = { this, iid, obj, kResultFalse };

		Reflex::Detail::EnumerateTupleTypes<Tuple<INTERFACES&...>>([&data]<UInt IDX, class IFACE>()
		{
			Callback::template Visit<IDX, IFACE>(data);
		});

		return data.d;
	}
};

template <typename... INTERFACES>
struct ComObject : public ComComponent <INTERFACES...>
{
	ComObject(TRef <Object> owner)
		: owner(owner)
	{
	}

	UInt32 REFLEX_STDCALL addRef() override { Retain(owner); return owner.GetRetainCount(); }

	UInt32 REFLEX_STDCALL release() override
	{
		auto c = owner.GetRetainCount() - 1;

		Release(owner);

		return c;
	}

	Object & owner;
};

template <class TYPE, class ... ARGS> TYPE * CreateRetained(ARGS && ... args)
{
	auto t = REFLEX_CREATE(TYPE, std::forward<ARGS>(args)...);

	Retain(*t);

	return t;
}

inline void SetSize(VST3API::ViewRect & rect, const iSize & size)
{
	rect.right = rect.left + size.w;
	rect.bottom = rect.top + size.h;
}

inline iSize GetSize(const VST3API::ViewRect & rect)
{
	return { rect.right - rect.left, rect.bottom - rect.top };
}

typedef Tuple <UInt8,UInt8,UInt8,UInt8> MidiMsg;

template <UInt MSG> REFLEX_INLINE UInt32 MakeMidiMessage(UInt8 channel, UInt8 idx, UInt8 value)
{
	return Reinterpret<UInt32>(MakeTuple(UInt8(channel | (MSG << 4)), idx, value, UInt8(0)));
}

typedef VST3API::tresult tresult;

struct Vst3EditController;
struct Vst3View;

template <class TYPE>
struct NullImpl : public TYPE
{
	tresult REFLEX_STDCALL queryInterface(const VST3API::UID & iid, VST3API::IUnknown * & obj) override { return VST3API::kResultNoInterface; };
	UInt32 REFLEX_STDCALL addRef() override { return 1; }
	UInt32 REFLEX_STDCALL release() override { return 0; }
};

struct NullEventList : public NullImpl <VST3API::IEventList>
{
	Int32 REFLEX_STDCALL getEventCount() override { return 0; }
	tresult REFLEX_STDCALL getEvent(Int32 index, VST3API::Event &) override { return VST3API::kResultFalse; }
	tresult REFLEX_STDCALL addEvent(VST3API::Event & e_in) override { return VST3API::kResultNotImplemented; }
};

struct Vst3Session :
	public PluginSession,
	ComObject <VST3API::IPluginFactory2>
{
	Vst3Session(void * hinstance);

	tresult REFLEX_STDCALL getFactoryInfo(VST3API::PFactoryInfo & info) override;
	Int32 REFLEX_STDCALL countClasses() override { return config.classes.GetSize(); }
	tresult REFLEX_STDCALL getClassInfo(Int32 idx, VST3API::PClassInfo & info) override;
	tresult REFLEX_STDCALL getClassInfo2(Int32 idx, VST3API::PClassInfo2 & info) override;
	tresult REFLEX_STDCALL createInstance(const VST3API::UID & cid, const VST3API::UID & iid, VST3API::IUnknown * & obj) override;
};

typedef The <Vst3Session> TheVst3Session;

struct Vst3Instance :
	public PluginInstance,
	public ComObject <VST3API::IPluginComponent,VST3API::IPluginAudioProcessor,VST3API::IPluginEditController,VST3API::IPluginMidiMapping,VST3API::IUnitInfo>
{
	REFLEX_OBJECT(Vst3Instance, PluginInstance);


	static inline VST3API::ProcessContext st_nullprocesscontext;


	Vst3Instance(PluginSession & pluginsession, const Configuration::Class & cls);

	~Vst3Instance();


	void OnSetViewSize(const iSize & size) override;


	void ReportProcessingDelay(UInt32 delay) override;

	void ReportStateChange(UInt8 change_flags) override;

	void BeginAutomation(UInt32 idx) override { m_ihostcallbacks->beginEdit(idx); }

	void Automate(UInt32 idx, Float32 value) override { m_ihostcallbacks->performEdit(idx, value); }

	void EndAutomation(UInt32 idx) override { m_ihostcallbacks->endEdit(idx); }



	//IComponent

	tresult REFLEX_STDCALL initialize(VST3API::IUnknown * context) override;
	tresult REFLEX_STDCALL terminate() override { return VST3API::kResultOk; }

	tresult REFLEX_STDCALL getControllerClassId(VST3API::UID & uid) override;
	tresult REFLEX_STDCALL setState(VST3API::IBStream * state) override;
	tresult REFLEX_STDCALL getState(VST3API::IBStream * state) override;
	tresult REFLEX_STDCALL setIoMode(Int32 iomode) override { return VST3API::kResultNotImplemented; }
	Int32 REFLEX_STDCALL getBusCount(Int32 evnt, Int32 output) override;
	tresult REFLEX_STDCALL getBusInfo(Int32 evnt, Int32 output, Int32 idx, VST3API::BusInfo & bus) override;
	tresult REFLEX_STDCALL getRoutingInfo(VST3API::RoutingInfo & inInfo, VST3API::RoutingInfo & outInfo) override { return VST3API::kResultNotImplemented; }
	tresult REFLEX_STDCALL activateBus(Int32 evnt, Int32 output, Int32 index, UInt8 state) override { return VST3API::kResultOk; }
	tresult REFLEX_STDCALL setActive(UInt8 state) override { return VST3API::kResultOk; }


	//IAudioProcessor

	tresult REFLEX_STDCALL setBusArrangements(UInt64 * input_speakerarrangements, Int32 ninput, UInt64 * output_speakerarrangements, Int32 noutput) override;
	tresult REFLEX_STDCALL getBusArrangement(Int32 busdirection, Int32 index, UInt64 & speakerarrangement) override;
	tresult REFLEX_STDCALL canProcessSampleSize(Int32 symbolicSampleSize) override { return symbolicSampleSize ? VST3API::kResultFalse : VST3API::kResultOk; }
	UInt32 REFLEX_STDCALL getLatencySamples() override { return m_latency; }
	tresult REFLEX_STDCALL setupProcessing(VST3API::ProcessSetup & setup) override;
	tresult REFLEX_STDCALL setProcessing(UInt8 state) override { return VST3API::kResultOk; }
	tresult REFLEX_STDCALL process(VST3API::ProcessData & data) override;
	UInt32 REFLEX_STDCALL getTailSamples() override { return 0; }


	//IEditController

	tresult REFLEX_STDCALL setComponentState(VST3API::IBStream * state) override;
	Int32 REFLEX_STDCALL getParameterCount() override { return cls.num_params + GetArraySize(m_midiccs); }
	tresult REFLEX_STDCALL getParameterInfo(Int32 idx, VST3API::ParameterInfo & info_out) override;
	tresult REFLEX_STDCALL getParamStringByValue(UInt32 paramid, Float64 valueNormalized_in, VST3API::String128 string_out) override;
	tresult REFLEX_STDCALL getParamValueByString(UInt32 paramid, char16_t * string_in, Float64 & valueNormalized_out) override;
	Float64 REFLEX_STDCALL normalizedParamToPlain(UInt32 paramid, Float64 normal) override;
	Float64 REFLEX_STDCALL plainParamToNormalized(UInt32 paramid, Float64 value) override;
	Float64 REFLEX_STDCALL getParamNormalized(UInt32 paramid) override;
	tresult REFLEX_STDCALL setParamNormalized(UInt32 paramid, Float64 value) override;
	tresult REFLEX_STDCALL setComponentHandler(VST3API::IHostAutomation * handler) override;
	VST3API::IPluginView * REFLEX_STDCALL createView(const char * name) override;


	//IPluginMidiMapping

	tresult REFLEX_STDCALL getMidiControllerAssignment(Int32 busIndex, Int16 channel, Int16 cc, UInt32 & id) override;


	//IUnitInfo

	Int32 REFLEX_STDCALL getUnitCount() override;
	tresult REFLEX_STDCALL getUnitInfo(Int32 unitIndex, UnitInfo & info_out) override;
	Int32 REFLEX_STDCALL getProgramListCount() override;
	tresult REFLEX_STDCALL getProgramListInfo(Int32 list, ProgramListInfo & info_out) override;
	tresult REFLEX_STDCALL getProgramName(Int32 listId, Int32 programIndex, VST3API::String128 name_out) override { return VST3API::kResultNotImplemented; }
	tresult REFLEX_STDCALL getProgramInfo(Int32 listId, Int32 programIndex, const char * attributeId_in, VST3API::String128 attributeValue_out) override { return VST3API::kResultNotImplemented; }
	tresult REFLEX_STDCALL hasProgramPitchNames(Int32 listId, Int32 programIndex) override { return VST3API::kResultFalse; }
	tresult REFLEX_STDCALL getProgramPitchName(Int32 listId, Int32 programIndex, Int16 midiPitch, VST3API::String128 name_out) override { return VST3API::kResultNotImplemented; }
	Int32 REFLEX_STDCALL getSelectedUnit() override { return 0; }
	tresult REFLEX_STDCALL selectUnit(Int32 id) override { return id == 0 ? VST3API::kResultOk : VST3API::kResultFalse; }
	tresult REFLEX_STDCALL getUnitByBus(Int32 evnt, Int32 output, Int32 idx, Int32 channel, Int32 & unit_out) override;
	tresult REFLEX_STDCALL setUnitProgramData(Int32 listOrUnitId, Int32 programIndex, VST3API::IBStream* data) override { return VST3API::kResultNotImplemented; }

	static void QueueControlChange(Vst3Instance & self, UInt16 position, UInt16 paramid, UInt8 value)
	{
		self.QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(position), 0, MakeMidiMessage<11>(0, UInt8(paramid), value));
	}

	static void QueueProgramChange(Vst3Instance & self, UInt16 position, UInt16 paramid, UInt8 value)
	{
		self.QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(position), 0, MakeMidiMessage<12>(0, value, 0));
	}

	using PluginInstance::QueueEvent;



	UInt32 m_latency;

	VST3API::IHostAutomation * m_ihostcallbacks = nullptr;

	Vst3View * m_iview = nullptr;

	Float32 m_midiccs[129] = { 0.0f };


	//cache

	CString m_buffer;

};

struct Vst3View :
	public Object,
	public ComObject <VST3API::IPluginView>
{
	#ifdef REFLEX_OS_WINDOWS
	static constexpr CString::View kWindowType = "HWND";
	#else
	static constexpr CString::View kWindowType = "NSView";
	#endif

	static iSize ToPixelUnits(const iSize & size);


	Vst3View(Vst3Instance & plugin);

	~Vst3View();

	tresult REFLEX_STDCALL isPlatformTypeSupported(const char * type) override;
	tresult REFLEX_STDCALL attached(void * parent, const char * type) override;
	tresult REFLEX_STDCALL removed() override;
	tresult REFLEX_STDCALL onWheel(float distance) override;
	tresult REFLEX_STDCALL onKeyDown(char16_t key, Int16 keyCode, Int16 modifiers) override;
	tresult REFLEX_STDCALL onKeyUp(char16_t key, Int16 keyCode, Int16 modifiers) override;
	tresult REFLEX_STDCALL getSize(VST3API::ViewRect * size) override;
	tresult REFLEX_STDCALL onSize(VST3API::ViewRect * rect) override;
	tresult REFLEX_STDCALL onFocus(UInt8 state) override;
	tresult REFLEX_STDCALL setFrame(VST3API::IHostView * frame) override;
	tresult REFLEX_STDCALL canResize() override;
	tresult REFLEX_STDCALL checkSizeConstraint(VST3API::ViewRect * rect) override;


	Vst3Instance & plugin;

	VST3API::IHostView * m_hostview = nullptr;

	VST3API::ViewRect m_viewrect = { 0,0,0,0 };
};

Vst3Session::Vst3Session(void * hinstance)
	: PluginSession(hinstance, kPluginFormatVST3)
	, ComObject<VST3API::IPluginFactory2>(this)
{
#ifdef REFLEX_OS_MACOS
	for (auto & cls : config.classes)
	{
		auto & pluginid = Reinterpret<UID>(RemoveConst(cls.vst3.uid));

		Quad <UInt8> & a = Reinterpret<Quad<UInt8>>(pluginid.a);

		Quad <UInt8> ta = a;

		Quad <UInt8> & b = Reinterpret<Quad<UInt8>>(pluginid.b);

		Quad <UInt8> tb = b;

		a.a = ta.d;
		a.b = ta.c;
		a.c = ta.b;
		a.d = ta.a;

		b.a = tb.b;
		b.b = tb.a;
		b.c = tb.d;
		b.d = tb.c;
	}
#endif
}

tresult Vst3Session::getFactoryInfo(VST3API::PFactoryInfo & info)
{
	info = {};

	if (auto & classes = config.classes)
	{
		auto & cls = classes.GetFirst();

		Reflex::RawStringCopy(cls.vendor.GetData(), info.vendor, GetArraySize(info.vendor));
	}

	return VST3API::kResultOk;
}

tresult Vst3Session::getClassInfo(Int32 idx, VST3API::PClassInfo & info)
{
	REFLEX_USE(VST3API);

	info = {};

	auto & cls = config.classes[idx];

	info.cid = Reinterpret<UID>(cls.vst3.uid);

	Reflex::RawStringCopy("Audio Module Class", info.category, GetArraySize(info.category));

	Reflex::RawStringCopy(cls.product.GetData(), info.name, GetArraySize(info.name));

	return kResultOk;
}

tresult Vst3Session::getClassInfo2(Int32 idx, VST3API::PClassInfo2 & info)
{
	using Class = AudioPlugin::Configuration::Class;

	info = {};

	getClassInfo(idx, Reinterpret<VST3API::PClassInfo>(info));

	auto & cls = config.classes[idx];

	Reflex::RawStringCopy(cls.vendor.GetData(), info.vendor, GetArraySize(info.vendor));
	Reflex::RawStringCopy(cls.version.GetData(), info.version, GetArraySize(info.version));

	auto type = cls.type;
	auto cat = cls.category;

	CString temp;

	auto Add = [&temp](const char * s)
	{
		temp.Append(s);

		temp.Push('|');
	};

	switch (type)
	{
	case Class::kTypeAudioGenerator:
		Add("Instrument");
		if (cat & Class::kSynth) Add("Synth");
		if (cat & Class::kSampler) Add("Sampler");
		if (cat & Class::kDrum) Add("Drum");
		break;

	case Class::kTypeEventProcessor:
	case Class::kTypeEventGenerator:	//VST3 has no dedicated "NoteEffect" root string; common practice is to tag it as Fx.
		Add("Fx");
		Add("Note");
		break;

	//case Class::kTypeAudioProcessor:
	default:
		Add("Fx");
		if (cat & Class::kEQ) Add("EQ");
		if (cat & Class::kDynamics) Add("Dynamics");
		if (cat & Class::kReverb) Add("Reverb");
		if (cat & Class::kDelay) Add("Delay");
		if (cat & Class::kModulation) Add("Modulation");
		if (cat & Class::kPitchShift) Add("Pitch Shift");
		if (cat & Class::kDistortion) Add("Distortion");
		if (cat & Class::kFilter) Add("Filter");
		if (cat & Class::kRestoration) Add("Restoration");
		if (cat & Class::kMastering) Add("Mastering");
		if (cat & Class::kSurround) Add("Surround");
		if (cat & Class::kAnalyzer) Add("Analyzer");
		break;
	}

	if (temp) temp.Pop();

	Reflex::RawStringCopy(temp.GetData(), info.subcategory, GetArraySize(info.subcategory));

	return VST3API::kResultOk;
}

tresult Vst3Session::createInstance(const VST3API::UID & cid, const VST3API::UID & uid, VST3API::IUnknown * & obj)
{
	REFLEX_USE(VST3API);

	auto & classes = config.classes;

	for (auto & cls : classes)
	{
		if (cid == Reinterpret<UID>(cls.vst3.uid))
		{
			obj = Cast<IPluginComponent>(CreateRetained<Vst3Instance>(*this, cls));

			return kResultOk;
		}
	}

	if (uid == VST3API::ClassInfo<IPluginComponent>::iid)
	{
		obj = Cast<IPluginComponent>(CreateRetained<Vst3Instance>(*this, classes.GetFirst()));

		return kResultOk;
	}
	else
	{
		return kResultNoInterface;
	}
}

Vst3Instance::Vst3Instance(PluginSession & pluginsession, const Configuration::Class & cls)
	: PluginInstance(pluginsession, cls)
	, ComObject<VST3API::IPluginComponent,VST3API::IPluginAudioProcessor,VST3API::IPluginEditController,VST3API::IPluginMidiMapping,VST3API::IUnitInfo>(this)
	, m_latency(0)
{
	REFLEX_ASSERT(!(cls.channels_io.a & 1) && !(cls.channels_io.b & 1));	//enforce stereo for now

	Initialise();
}

Vst3Instance::~Vst3Instance()
{
	Deinitialise();
}

void Vst3Instance::OnSetViewSize(const iSize & size)
{
	auto & rect = m_iview->m_viewrect;

	SetSize(rect, size);

	if (!IsResizingWindow())
	{
		m_iview->m_hostview->resizeView(m_iview, rect);
	}
}

void Vst3Instance::ReportProcessingDelay(UInt32 delay)
{
	REFLEX_ASSERT_MAINTHREAD("Vst3Instance::ReportProcessingDelay");

	if (SetFiltered(m_latency, delay))
	{
		m_ihostcallbacks->restartComponent(VST3API::kLatencyChanged);
	}
}

void Vst3Instance::ReportStateChange(UInt8 change_flags)
{
	REFLEX_ASSERT_MAINTHREAD("Vst3Instance::ReportStateChange");

	Int32 restart_component_flags = 0;

	if (change_flags & kChangeParameterValues) restart_component_flags |= VST3API::kParamValuesChanged;

	if (change_flags & kChangeParameterInfo) restart_component_flags |= VST3API::kParamTitlesChanged;

	m_ihostcallbacks->restartComponent(restart_component_flags);

	//TODO kChangeState -> IComponentHandler2::setDirty(true);
}

tresult Vst3Instance::initialize(IUnknown * context)
{
	REFLEX_USE(VST3API);

	if (session->hostname.Empty())
	{
		if (auto host = QueryInterface<IHostApplication>(context))
		{
			String128 name;

			host->getName(name);

			host->release();

			CString cname;

			cname.SetSize(Reflex::RawStringLength(name));

			Reflex::RawStringCopy(name, cname.GetData(), cname.GetSize() + 1);

			session->SetHostName(std::move(cname));
		}
	}

	return kResultOk;
}

tresult Vst3Instance::getControllerClassId(VST3API::UID & uid)
{
	uid = { kMaxUInt32,kMaxUInt32,kMaxUInt32,kMaxUInt32 };

	return VST3API::kResultOk;
}

tresult Vst3Instance::setState(VST3API::IBStream * state)
{
	Array <UInt8> archive;

	Int64 end;

	state->seek(0, 2, &end);

	state->seek(0, 0);

	archive.SetSize(UInt(end));

	state->read(archive.GetData(), Int32(end));

	GetCallbacks().OnSetPluginChunk(archive);

	return VST3API::kResultOk;
}

tresult Vst3Instance::getState(VST3API::IBStream * state)
{
	auto archive = GetCallbacks().OnGetPluginChunk();

	state->write(archive.GetData(), archive.GetSize());

	return VST3API::kResultOk;
}

Int32 Vst3Instance::getBusCount(Int32 evnt, Int32 output)
{
	if (evnt)
	{
		return (&cls.midi_io.a)[output];
	}
	else
	{
		return GetAudioChannels(output).size / 2;
	}
}

tresult Vst3Instance::getBusInfo(Int32 evnt, Int32 output, Int32 idx, VST3API::BusInfo & bus)
{
	REFLEX_USE(VST3API);

	bus.event = evnt;
	bus.direction = output;
	bus.aux = false;

	if (evnt)
	{
		bus.channelCount = 1;

		RawStringCopy("Event", bus.name);
	}
	else
	{
		UInt total_channels = (&cls.channels_io.a)[output];
		
		bool stereo = (total_channels & 1) == 0;
		
		bus.channelCount = stereo ? 2 : 1;

		char temp[16];

		MakeAudioPinName(bool(output), stereo, UInt(idx), temp, GetArraySize(temp));

		RawStringCopy(temp + 0, bus.name);
	}

	bus.flags = BusInfo::kDefaultActive;

	return kResultOk;
}

tresult Vst3Instance::setBusArrangements(UInt64 * input_speakerarrangements, Int32 ninput, UInt64 * output_speakerarrangements, Int32 noutput)
{
	return cls.channels_io == MakeTuple(UInt8(ninput), UInt8(noutput)) ? VST3API::kResultOk : VST3API::kResultFalse;
}

tresult Vst3Instance::getBusArrangement(Int32 busdirection, Int32 index, UInt64 & speakerarrangement)
{
	speakerarrangement = 3;

	return VST3API::kResultOk;
}

tresult Vst3Instance::setupProcessing(VST3API::ProcessSetup & setup)
{
	PrepareProcessing(setup.maxSamplesPerBlock, Float32(setup.sampleRate));

	return VST3API::kResultOk;
}

tresult Vst3Instance::process(VST3API::ProcessData & data)
{
	REFLEX_USE(VST3API);

	auto inputs = GetAudioChannels(false).data;

	auto itr = inputs;

	REFLEX_LOOP_PTR(data.inputs, pbus, data.numInputs)
	{
		auto & lr = *reinterpret_cast<Pair<Float*>*>(pbus->channelBuffers32);;

		(*itr++) = lr.a;
		(*itr++) = lr.b;
	}

	auto outputs = GetAudioChannels(true).data;

	itr = outputs;

	REFLEX_LOOP_PTR(data.outputs, pbus, data.numOutputs)
	{
		auto & lr = *reinterpret_cast<Pair<Float*>*>(pbus->channelBuffers32);;

		(*itr++) = lr.a;
		(*itr++) = lr.b;
	}

	if (auto vstparamsin = data.inputParameterChanges)
	{
		Int32 position;

		Float64 value;

		auto nparam = cls.num_params;

		REFLEX_LOOP(idx, vstparamsin->getParameterCount())
		{
			auto data = vstparamsin->getParameterData(idx);

			auto paramid = UInt16(data->getParameterId());

			if (auto n = data->getPointCount())
			{
				data->getPoint(n - 1, position, value);

				auto fvalue32 = Float32(value);

				if (paramid < nparam)
				{
					QueueEvent(AudioPlugin::Event::kTypeAutomation, UInt16(position), paramid, fvalue32);
				}
				else 
				{
					paramid -= nparam;

					m_midiccs[paramid] = fvalue32;

					auto byte_value = UInt8(Truncate(fvalue32 * 127.0f));

					if (paramid == 128)
					{
						QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(position), 0, MakeMidiMessage<12>(0, byte_value, 0));
					}
					else
					{
						QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(position), 0, MakeMidiMessage<11>(0, UInt8(paramid), byte_value));
					}
				}
			}
		}
	}

	if (auto vsteventsin = data.inputEvents)
	{
		VST3API::Event e;

		REFLEX_LOOP(idx, vsteventsin->getEventCount())
		{
			vsteventsin->getEvent(idx, e);

			switch (e.type)
			{
			case VST3API::Event::kTypeNoteOn:
				QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(e.sampleOffset), UInt16(e.busIndex), MakeMidiMessage<9>(0, UInt8(e.noteOn.pitch), UInt8(Truncate(e.noteOn.velocity * 127.0f))));
				break;

			case VST3API::Event::kTypeNoteOff:
				QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(e.sampleOffset), UInt16(e.busIndex), MakeMidiMessage<8>(0, UInt8(e.noteOff.pitch), 0));
				break;

			case VST3API::Event::kTypePolyPressure:
				break;
			}
		}
	}

	auto & context = data.processContext ? *data.processContext : st_nullprocesscontext;

	ProcessRt(data.numSamples, True(context.state & ProcessContext::kPlaying), context.tempo * kRcpSixty, context.projectTimeMusic);

	if (data.outputs)
	{
		data.outputs->silenceFlags = 0;
	}

	return kResultOk;
}

tresult Vst3Instance::setComponentState(VST3API::IBStream * state)
{
	return VST3API::kResultNotImplemented;
}

tresult Vst3Instance::getParameterInfo(Int32 idx, VST3API::ParameterInfo & info_out)
{
	REFLEX_USE(VST3API);

	info_out.id = idx;
	info_out.flags = ParameterInfo::kCanAutomate;
	info_out.stepCount = 0;
	info_out.defaultNormalizedValue = 0;
	info_out.unitId = 0;

	if (idx < cls.num_params)
	{
		GetCallbacks().OnGetParameterInfo(idx, m_buffer);

		Reflex::RawStringCopy(m_buffer.GetData(), info_out.title, GetArraySize(info_out.title));

		info_out.shortTitle[0] = 0;
	}
	else if (idx == (cls.num_params + 128))
	{
		info_out.flags = ParameterInfo::kCanAutomate | ParameterInfo::kIsProgramChange;
		info_out.stepCount = 128;
	}
	else
	{
		UInt cc = idx - cls.num_params;

		info_out.shortTitle[0] = 'C';
		info_out.shortTitle[1] = 'C';

		m_buffer = Reflex::ToCString(cc);

		Reflex::RawStringCopy(m_buffer.GetData(), info_out.shortTitle + 2, GetArraySize(info_out.shortTitle) - 2);

		Reflex::RawStringCopy(info_out.shortTitle, info_out.title, GetArraySize(info_out.title));
	}

	info_out.units[0] = 0;

	return kResultOk;
}

tresult Vst3Instance::getParamStringByValue(UInt32 paramid, Float64 valueNormalized_in, VST3API::String128 string_out)
{
	m_buffer = ToCString(valueNormalized_in, 8);

	Reflex::RawStringCopy(m_buffer.GetData(), string_out, 128);

	return VST3API::kResultOk;
}

tresult Vst3Instance::getParamValueByString(UInt32 paramid, char16_t * string_in, Float64 & valueNormalized_out)
{
	valueNormalized_out = 0;

	return VST3API::kResultNotImplemented;
}

Float64 Vst3Instance::normalizedParamToPlain(UInt32 paramid, Float64 normal)
{
	return normal;
}

Float64 Vst3Instance::plainParamToNormalized(UInt32 paramid, Float64 value)
{
	return value;
}

Float64 Vst3Instance::getParamNormalized(UInt32 paramid)
{
	if (paramid < cls.num_params)
	{
		return GetCallbacks().OnGetParameterValue(paramid);
	}
	else
	{
		return m_midiccs[paramid - cls.num_params];
	}
}

tresult Vst3Instance::setParamNormalized(UInt32 paramid, Float64 value)
{
	if (GetThreadID() == kMainThreadID)
	{
		if (paramid < cls.num_params)
		{
			GetCallbacks().OnSetParameterValue(paramid, Float(value));
		}
		else
		{
			m_midiccs[paramid - cls.num_params] = Float(value);
		}

		return VST3API::kResultOk;
	}
	else
	{
		return VST3API::kResultFalse;
	}
}

tresult Vst3Instance::setComponentHandler(VST3API::IHostAutomation * handler)
{
	m_ihostcallbacks = handler;

	return VST3API::kResultOk;
}

VST3API::IPluginView * Vst3Instance::createView(const char * name)
{
	return CreateRetained<Vst3View>(*this);
}

tresult Vst3Instance::getMidiControllerAssignment(Int32 busIndex, Int16 channel, Int16 cc, UInt32 & id)
{
	if (!(busIndex | channel) && Reinterpret<UInt16>(cc) < 128) 
	{
		id = cls.num_params + cc;

		return VST3API::kResultOk;
	}
	else
	{
		return VST3API::kResultFalse;
	}
}

Int32 Vst3Instance::getUnitCount()
{
	return 1;
}

tresult Vst3Instance::getUnitInfo(Int32 unitIndex, UnitInfo & info_out)
{
	info_out.id = 0;
	info_out.name[0] = 0;
	info_out.parentUnitId = 0;
	info_out.programListId = 0;

	return VST3API::kResultOk;
}

Int32 Vst3Instance::getProgramListCount()
{
	return 1;
}

tresult Vst3Instance::getProgramListInfo(Int32 list, ProgramListInfo & info_out)
{
	if (list == 0)
	{
		info_out.name[0] = 0;

		info_out.programCount = 128;

		return VST3API::kResultOk;
	}

	return VST3API::kResultFalse;
}

tresult Vst3Instance::getUnitByBus(Int32 evnt, Int32 output, Int32 idx, Int32 channel, Int32 & unit_out)
{
	unit_out = 0;

	return evnt && output == false && idx == 0 && channel == 0 ? VST3API::kResultOk : VST3API::kResultFalse;
}

Vst3View::Vst3View(Vst3Instance & plugin)
	: ComObject<VST3API::IPluginView>(this)
	, plugin(plugin)
{
	plugin.m_iview = this;
}

Vst3View::~Vst3View()
{
	plugin.m_iview = 0;
}

tresult Vst3View::isPlatformTypeSupported(const char * type)
{
	return CString::View(type) == kWindowType ? VST3API::kResultOk : VST3API::kResultFalse;
}

tresult Vst3View::attached(void * parent, const char * type)
{
	plugin.ShowEditor(parent);

	//getSize(&m_viewrect);

	//m_hostview->resizeView(this, m_viewrect);

	return VST3API::kResultOk;
}

tresult Vst3View::removed()
{
	plugin.DiscardEditor();

	return VST3API::kResultOk;
}

tresult Vst3View::onWheel(float distance)
{
	return VST3API::kResultOk;
}

tresult Vst3View::onKeyDown(char16_t key, Int16 keyCode, Int16 modifiers)
{
	return VST3API::kResultFalse;
}

tresult Vst3View::onKeyUp(char16_t key, Int16 keyCode, Int16 modifiers)
{
	return VST3API::kResultFalse;
}

tresult Vst3View::getSize(VST3API::ViewRect * size)
{
	auto window = plugin.GetEditor();

	if (IsValid(window))
	{
		auto contentsize = ToPixelUnits(window->GetClient()->OnGetContentSize());

		SetSize(m_viewrect, contentsize);

		*size = m_viewrect;

		return VST3API::kResultOk;
	}
	else
	{
		//ableton needs this

		return VST3API::kResultFalse;
	}
}

tresult Vst3View::onSize(VST3API::ViewRect * rect)
{
	if (!plugin.IsResizingWindow())
	{
		auto size = GetSize(*rect);

		SetSize(m_viewrect, size);

		auto pixeldensity = ToPixelUnits({ 1,1 }).w;

		size.w /= pixeldensity;
		size.h /= pixeldensity;

		plugin.SetEditorSize(size);
	}

	return VST3API::kResultOk;
}

tresult Vst3View::onFocus(UInt8 state)
{
	return VST3API::kResultOk;
}

tresult Vst3View::setFrame(VST3API::IHostView * frame)
{
	m_hostview = frame;

	return VST3API::kResultOk;
}

tresult Vst3View::canResize()
{
	return plugin.IsEditorResizable()  ? VST3API::kResultOk : VST3API::kResultFalse;
}

tresult Vst3View::checkSizeConstraint(VST3API::ViewRect * rect)
{
	auto window = plugin.GetEditor();

	auto contentsize = ToPixelUnits(window->GetClient()->OnGetContentSize());

	auto size = GetSize(*rect);

	size.w = Clip(size.w, contentsize.w, kMaxInt32);
	size.h = Clip(size.h, contentsize.h, kMaxInt32);

	SetSize(*rect, size);

	return VST3API::kResultOk;
}

REFLEX_END_INTERNAL
