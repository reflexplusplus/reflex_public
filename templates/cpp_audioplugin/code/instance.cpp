#include "instance.h"




//
//_PRODUCT-NAME-SYMBOL_::Instance implementation

namespace _PRODUCT-NAME-SYMBOL_ { namespace {	//begin internal namespace

using namespace Reflex;

static constexpr UInt16 kParameterGroupMode = MakeBit(0);
static constexpr UInt16 kParameterGroupOsc = MakeBit(1);

class InstanceImpl : public Instance
{
public:
	
	static constexpr UInt16 kChunkVersion = 0;							//change to 1 to activate persistence callbacks

	InstanceImpl(const Class & cls, System::AudioPlugin & owner)
		: Instance(cls, owner, MakeKey32("_PRODUCT-NAME-SYMBOL_"), kChunkVersion)	//first parameter is 4 byte header for the file format
		, m_amp(0.0f)
		, m_phase_inc(0.0f)
		, m_phase(0.0f)
	{
		output.Log("_PRODUCT-NAME_ Instance constructed");
	}

	//Bootstrap::Streamable callbacks
	//parameters are stored automatically in the base class, but additional/custom data can be stored here
	//to enable these callbacks, set kChunkVersion to 1

	void OnReset(Key32 context) override
	{
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
	}

	void OnStore(Data::Archive & stream) const override
	{
	}

	//Bootstrap::AudioPlugin callbacks
	//implement these callbacks to process/generate audio

	bool OnPrepareProcessing(UInt max_buffersize, Float32 samplerate, UInt num_inputs, UInt num_outputs) override
	{
		m_sr = samplerate;

		return num_outputs > 0;	//allow processing if at least 1 output
	}

	void OnProcessRt(UInt num_samples, UInt32 parameter_group_flags, const EventBuffer & events_in, Array <Event> & events_out, const ArrayView <const Float*> & inputs, const ArrayView <Float*> & outputs) override
	{
		auto params = GetParameterValues();

		if (parameter_group_flags & kParameterGroupMode)
		{
			switch (params[0].ivalue)
			{
			case 1:
				m_generate_waveform = &InstanceImpl::GenerateWaveform<1>;
				break;

			case 0:
				m_generate_waveform = &InstanceImpl::GenerateWaveform<0>;
				break;

			default:
				m_generate_waveform = [](UInt, Float, Float, Float, Float*) { return 0.0f; };
				break;
			}

			m_process_fx = True(params[3].ivalue) ? &InstanceImpl::ProcessFX : [](UInt, Float*) {};

			output.Log("Updating mode");
		}

		if (parameter_group_flags & kParameterGroupOsc)
		{
			auto f = params[1].fvalue;

			m_phase_inc = f / m_sr;

			m_amp = params[2].fvalue;

			output.Log("Updating", "amp:", m_amp, "freq:", f);
		}

		auto l = outputs[0];

		m_phase = m_generate_waveform(num_samples, m_amp, m_phase_inc, m_phase, l);

		m_process_fx(num_samples, l);

		auto rest = Splice(outputs, 1).b;

		for (auto & i : rest)
		{
			memcpy(i, l, num_samples * sizeof(Float32));
		}
	}

	template <UInt SHAPE> static Float32 GenerateWaveform(UInt num_samples, Float amp, Float phase_inc, Float phase, Float * out)
	{
		//a 'naive' (non anti-aliased) waveform for this simple example

		while (num_samples--)
		{
			phase += phase_inc;

			if (phase > 1.0f) phase -= 1.0f;

			Float32 value;

			if constexpr (SHAPE)
			{
				value = Float(phase > 0.5f);
			}
			else
			{
				value = Sin(k2Pif * ((phase * 2.0f) - 1.0f));
			}

			value *= amp;

			*out++ = value;
		}

		return phase;
	}

	static void ProcessFX(UInt num_samples, Float * in_out)
	{
		Float32 q = 1.0f / 16.0f;

		while (num_samples--)
		{
			auto & sample = *in_out++;

			sample = Quantise(sample, q);
		}
	}

	
	//your state and data here...

	Float m_sr;

	Float m_amp, m_phase_inc, m_phase;

	decltype (&InstanceImpl::GenerateWaveform<0>) m_generate_waveform;

	decltype (&InstanceImpl::ProcessFX) m_process_fx;
};

} }	//end internal namespace

Reflex::Array <Reflex::Bootstrap::AudioPlugin::Class> _PRODUCT-NAME-SYMBOL_::Instance::MakeClasses()
{
	constexpr CString::View kProductIdentifier = REFLEX_STRINGIFY(PRODUCT_PACKAGE_IDENTIFIER);
	constexpr CString::View kProductVersion = REFLEX_STRINGIFY(PRODUCT_VERSION);

	const UInt kNumClass = 1;
	const auto kPluginType = Class::kTypeAudioEffect;

	//AudioUnit can not be published programatically, instead IDs are written into the plist
	//the standard AudioPlugin project writes the plist ids from the AU_COMPONENTS string
	constexpr CString::View kAudioUnitComponents = REFLEX_STRINGIFY(AU_COMPONENTS);
	constexpr CString::View kAudioUnitVendor4CC = REFLEX_STRINGIFY(AU_VENDOR_4CC);
	auto au_classes = Split(kAudioUnitComponents, ',');
	REFLEX_ASSERT(au_classes.GetSize() == kNumClass);

	Array <Class> classes(kNumClass);

	Class & cls = classes.GetFirst();

	cls.vendor = Bootstrap::global->vendor;
	cls.product = Bootstrap::global->product;
	cls.version = kProductVersion ? kProductVersion : ToView("1.0.0");

	cls.type = kPluginType;
	cls.category = Class::kUncategorised;


	//CLAP uid (required by Bootstrap::AudioPlugin)

	cls.clap.uid = kProductIdentifier;


	//VST3 uid

	auto sha = Data::SHA1(Data::Pack(kProductIdentifier));

	MemCopy(sha.GetData(), &cls.vst3.uid, 16);


	//AU uid & type

	if (kAudioUnitComponents)
	{
		auto parts = Split(au_classes.GetFirst(), ':');	//uid_4cc:type_4cc[:name]

		REFLEX_ASSERT(parts.GetSize() == 2 || parts.GetSize() == 3);

		if (parts.GetSize() > 2) cls.product = parts[2];

		auto write_4cc = [](CString::View uid)
		{
			REFLEX_ASSERT(uid.size == 4);
			UInt32 _4cc;
			auto dst = Reinterpret<UInt8>(&_4cc);
			dst[0] = uid[3];
			dst[1] = uid[2];
			dst[2] = uid[1];
			dst[3] = uid[0];
			return _4cc;
		};

		cls.audiounit.company_4cc = write_4cc(kAudioUnitVendor4CC);
		cls.audiounit.uid_4cc = write_4cc(parts[0]);

		auto idx = Search(Class::AudioUnit::kTypes, write_4cc(parts[1]));
		REFLEX_ASSERT_EX(idx, "unsupported AU component type");
		if (idx) cls.type = Class::Type(idx.value);
	}


	//parameters

	cls.num_params = 4;

	
	//io

	cls.channels_io = { UInt8(cls.type == Class::kTypeAudioGenerator ? 0 : 2), 2 };
	cls.midi_io = { cls.type != Class::kTypeAudioProcessor, false };

	return classes;
}

void _PRODUCT-NAME-SYMBOL_::Instance::PopulateParameters(const Class & cls, ArrayRegion < Pair <Key32, ConstReference <Bootstrap::ParameterDefinition> > > paramdefs)
{
	UInt idx = 0;

	auto add_param = [&paramdefs, &idx](Key32 id, TRef <Bootstrap::ParameterDefinition> desc)
	{
		paramdefs[idx++] = { id, desc };
	};


	add_param("mode", Bootstrap::DefineEnumParameter(L"Mode", { L"Sine", L"Square" }, 0, kParameterGroupMode));

	add_param("freq", Bootstrap::DefineContinuousParameter(L"Freq", 100.0f, 1000.0f, 1.0f, 0.0f, kParameterGroupOsc, [](Bootstrap::Value32 value)
	{
		if (value.fvalue >= 1000.0f)
		{
			return Join(ToWString(value.fvalue / 1000.0f, 0), L" kHz");
		}
		else
		{
			return Join(ToWString(value.fvalue, 0), L" Hz");
		}
	}));

	add_param("amp", Bootstrap::DefineContinuousParameter(L"Amp", 0.0f, 1.0f, 0.0f, 0.0f, kParameterGroupOsc, [](Bootstrap::Value32 value)
	{
		return Join(ToWString(value.fvalue * 100.0f, 1), L'%');
	}));

	add_param("fx", Bootstrap::DefineBoolParameter(L"FX", false, kParameterGroupMode));


	REFLEX_ASSERT(idx == paramdefs.size);
}

Reflex::TRef <_PRODUCT-NAME-SYMBOL_::Instance> _PRODUCT-NAME-SYMBOL_::Instance::Create(const Class & cls, System::AudioPlugin & instance)
{
	return New<InstanceImpl>(cls, instance);
}

Reflex::Output _PRODUCT-NAME-SYMBOL_::output("_PRODUCT-NAME_");
