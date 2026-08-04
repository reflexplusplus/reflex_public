#include "instance.h"




//
//_PRODUCT-NAME-SYMBOL_::Instance implementation

namespace _PRODUCT-NAME-SYMBOL_ { namespace {	//begin internal namespace

using namespace Reflex;

static constexpr UInt16 kParameterGroupMode = MakeBit(0);
static constexpr UInt16 kParameterGroupOsc = MakeBit(1);

struct InstanceImpl : public Instance
{
	static constexpr UInt16 kChunkVersion = 0;							//change to 1 to activate persistence callbacks

	InstanceImpl(System::AudioPlugin & owner)
		: Instance(owner, MakeKey32("_PRODUCT-NAME-SYMBOL_"), kChunkVersion)	//first parameter is 4 byte header for the file format
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

	void OnProcessRt(UInt num_samples, UInt32 parameter_change_flags, const EventBuffer & events_in, Array <Event> & events_out, const ArrayView <const Float*> & inputs, const ArrayView <Float*> & outputs) override
	{
		auto params = GetParameterValues();

		if (parameter_change_flags & kParameterGroupMode)
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

		if (parameter_change_flags & kParameterGroupOsc)
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

Reflex::Bootstrap::AudioPlugin::Class _PRODUCT-NAME-SYMBOL_::Instance::MakeClass()
{
	constexpr bool kIsInstrument = false;	//quick config of instrument or effect, for AU builds, ensure this matches the AU_TYPE_4CC eg "aufx"

	constexpr auto kType = kIsInstrument ? Class::kTypeAudioGenerator : Class::kTypeAudioProcessor;


	Class cls;

	cls.vendor = Bootstrap::global->vendor;

	cls.product = Bootstrap::global->product;

#ifdef VERSION
	cls.version = REFLEX_STRINGIFY(VERSION);
#else
	cls.version = "1.0.0";
#endif


	cls.type = kType;

	cls.category = Class::kUncategorised;

	cls.num_params = 4;

	cls.channels_io = { UInt8(kIsInstrument ? 0 : 2), 2 };

	cls.midi_io = { kIsInstrument, false };


	//CLAP specific

	cls.clap.uid = Lowercase(Join(Filter(cls.vendor, ' '), '.', Filter(cls.product, ' '), '.', ToCString(cls.channels_io.a), ':', ToCString(cls.channels_io.b)));


	//VST3 specific

	cls.vst3.uid = { K64("_VENDOR-NAME_"), K64("_PRODUCT-NAME_") };


	//AU specific

	cls.audiounit.company_4cc = CC32("_VENDOR-4CC_");

	cls.audiounit.uid_4cc = CC32("_PRODUCT-4CC_");
	
#ifdef AU_TYPE_4CC
	// AudioUnit requires the AU_TYPE 4CC to be declared in both the Info.plist
	// and at run time. Reflex derives the run-time value from cls.type.
	// Verify at compile time that the plist AU_TYPE_4CC matches.
	constexpr auto kAudioUnitType = Class::AudioUnit::kTypes[kType];
	REFLEX_STATIC_ASSERT(kAudioUnitType == CC32(REFLEX_STRINGIFY(AU_TYPE_4CC)));
#endif


	return cls;
}

void _PRODUCT-NAME-SYMBOL_::Instance::PopulateParameters(const Class & cls, ArrayRegion < Pair <Key32, ConstReference <ParamDesc> > > paramdefs)
{
	UInt idx = 0;

	auto AddParam = [&paramdefs, &idx](Key32 id, UInt16 change_flags, TRef <ParamDesc> desc)
	{
		desc->change_flags = change_flags;

		paramdefs[idx++] = { id, desc };

		return desc;
	};


	AddParam("mode", kParameterGroupMode, ParamDesc::CreateEnum("Mode", { "Sine", "Square" }, 0));

	AddParam("freq", kParameterGroupOsc, ParamDesc::CreateReal("Freq", 100.0f, 1000.0f, 0.0f, 0.0f))->to_string = [](Bootstrap::Value32 value)
	{
		if (value.fvalue >= 1000.0f)
		{
			return Join(ToWString(value.fvalue / 1000.0f, 0), L" kHz");
		}
		else
		{
			return Join(ToWString(value.fvalue, 0), L" Hz");
		}
	};

	AddParam("amp", kParameterGroupOsc, ParamDesc::CreateReal("Amp", 0.0f, 1.0f, 0.0f, 0.0f));

	AddParam("fx", kParameterGroupMode, ParamDesc::CreateBool("FX", false));


	REFLEX_ASSERT(idx == paramdefs.size);
}

Reflex::TRef <_PRODUCT-NAME-SYMBOL_::Instance> _PRODUCT-NAME-SYMBOL_::Instance::Create(System::AudioPlugin & instance)
{
	return New<InstanceImpl>(instance);
}

Reflex::Output _PRODUCT-NAME-SYMBOL_::output("_PRODUCT-NAME_");
