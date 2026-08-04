#include "plugin.h"
#include "instance.cpp"
#include "plugin.cpp"

#include "../ext/aax/SDK/Interfaces/AAX.h"
#include "../ext/aax/SDK/Interfaces/AAX_Init.h"
#include "../ext/aax/SDK/Interfaces/AAX_ICollection.h"
#include "../ext/aax/SDK/Interfaces/AAX_IEffectDescriptor.h"
#include "../ext/aax/SDK/Interfaces/AAX_IComponentDescriptor.h"
#include "../ext/aax/SDK/Interfaces/AAX_IPropertyMap.h"
#include "../ext/aax/SDK/Interfaces/AAX_IViewContainer.h"
#include "../ext/aax/SDK/Interfaces/AAX_IMIDINode.h"
#include "../ext/aax/SDK/Interfaces/AAX_CEffectParameters.h"
#include "../ext/aax/SDK/Interfaces/AAX_CEffectGUI.h"
#include "../ext/aax/SDK/Interfaces/AAX_CParameter.h"
#include "../ext/aax/SDK/Interfaces/AAX_CLinearTaperDelegate.h"
#include "../ext/aax/SDK/Interfaces/AAX_CNumberDisplayDelegate.h"

#include <cstdio>

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

typedef Reflex::The<PluginSession> ThePluginSession;

struct AaxParameters;

struct AaxInstance : public PluginInstance
{
	REFLEX_OBJECT(AaxInstance, PluginInstance);

	AaxInstance(PluginSession & pluginsession, const Configuration::Class & cls, AaxParameters & owner)
		: PluginInstance(pluginsession, cls)
		, m_owner(owner)
		, m_automationDepth(0)
		, m_viewContainer(nullptr)
	{
		Initialise();
	}

	~AaxInstance()
	{
		Deinitialise();
	}

	virtual void OnSetViewSize(const iSize & size) override
	{
		if (m_viewContainer)
		{
			AAX_Point newSize = { Float32(size.w), Float32(size.h) };
			m_viewContainer->SetViewSize(newSize);
		}
	}

	virtual void ReportProcessingDelay(UInt32 delay) override;

	virtual void ReportParametersChanged() override;

	virtual void BeginAutomation(UInt32 idx) override;
	virtual void Automate(UInt32 idx, Float32 value) override;
	virtual void EndAutomation(UInt32 idx) override;

	bool PrepareForProcess(UInt32 buffersize, Float32 samplerate)
	{
		if (samplerate > 1.0f)
		{
			return PrepareProcessing(buffersize, samplerate);
		}

		return PrepareBufferSize(buffersize);
	}

	void SetBuffers(float ** in, float ** out)
	{
		auto inputs = GetAudioChannels(false);
		auto outputs = GetAudioChannels(true);

		if (in)
		{
			REFLEX_LOOP(i, inputs.size) inputs[i] = in[i];
		}
		if (out)
		{
			REFLEX_LOOP(i, outputs.size) outputs[i] = out[i];
		}
	}

	void PushMidiIn(AAX_CMidiStream * midi)
	{
		if (!midi) return;
		REFLEX_LOOP(i, midi->mBufferSize)
		{
			auto & p = midi->mBuffer[i];
			if (p.mLength)
			{
				UInt32 msg = 0;
				MemCopy(p.mData, &msg, Min<UInt32>(p.mLength, 4));
				QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(Min<UInt32>(p.mTimestamp, 0x3FFF)), 0, msg);
			}
		}
	}

	ArrayView<AudioPlugin::Event> Process(UInt32 frames, AAX_ITransport * transport)
	{
		bool clock = false;
		Float64 bps = 1.0;
		Float64 beatpos = 0.0;

		if (transport)
		{
			bool playing = false;
			if (transport->IsTransportPlaying(&playing) == AAX_SUCCESS)
			{
				clock = playing;
			}

			double tempo = 120.0;
			if (transport->GetCurrentTempo(&tempo) == AAX_SUCCESS)
			{
				bps = Max(tempo, 1.0) * kRcpSixty;
			}

			int64_t ticks = 0;
			if (transport->GetCurrentTickPosition(&ticks) == AAX_SUCCESS)
			{
				// AAX ticks: 960000 ticks per quarter note
				beatpos = Float64(ticks) / 960000.0;
			}
		}

		return ProcessRt(frames, clock, bps, beatpos);
	}

	void SetViewContainer(AAX_IViewContainer * container)
	{
		m_viewContainer = container;
	}

	AaxParameters & m_owner;
	UInt m_automationDepth;
	AAX_IViewContainer * m_viewContainer;
};

struct AaxAlgorithmContext
{
	int32_t * bufferSize;
	float ** inputs;
	float ** outputs;
	float * sampleRate;
	AAX_IMIDINode * midiIn;
	AAX_IMIDINode * midiOut;
	AaxParameters ** state;
};

struct AaxParameters : public AAX_CEffectParameters
{
	AaxParameters(PluginSession & session, UInt32 classIndex)
		: m_session(session)
		, m_classIndex(classIndex)
		, m_class(session.config.classes[classIndex])
		, m_instance(nullptr)
	{
		m_paramIDs.SetSize(m_class.num_params);
	}

	~AaxParameters() override
	{
		if (m_instance)
		{
			delete m_instance;
			m_instance = nullptr;
		}

		for (auto & i : m_paramIDs)
		{
			delete[] i;
		}
		m_paramIDs.Clear();
	}

	AAX_Result EffectInit() override
	{
		m_instance = new AaxInstance(m_session, m_class, *this);

		REFLEX_LOOP(i, m_class.num_params)
		{
			char id[16];
			snprintf(id, sizeof(id), "P%u", unsigned(i));
			m_paramIDs[i] = new char[16];
			RawStringCopy(id, m_paramIDs[i], 16);

			CString pname;
			m_instance->GetCallbacks().OnGetParameterInfo(i, pname);

			AAX_CString pnameAax(pname.GetData());
			auto * param = new AAX_CParameter<float>(
				m_paramIDs[i],
				pnameAax,
				m_instance->GetCallbacks().OnGetParameterValue(i),
				AAX_CLinearTaperDelegate<float>(0.0f, 1.0f),
				AAX_CNumberDisplayDelegate<float>(3),
				true);

			mParameterManager.AddParameter(param);
		}

		return AAX_SUCCESS;
	}

	AAX_Result GenerateCoefficients() override
	{
		if (!m_instance)
		{
			return AAX_SUCCESS;
		}

		REFLEX_LOOP(i, m_class.num_params)
		{
			double v = 0.0;
			if (GetParameterNormalizedValue(m_paramIDs[i], &v) == AAX_SUCCESS)
			{
				m_instance->GetCallbacks().OnSetParameterValue(i, Float32(v));
			}
		}

		return AAX_SUCCESS;
	}

	AAX_Result ResetFieldData(AAX_CFieldIndex inFieldIndex, void * oData, uint32_t inDataSize) const override
	{
		if (inDataSize == sizeof(AaxParameters*))
		{
			*(AaxParameters**)oData = const_cast<AaxParameters*>(this);
		}
		else
		{
			MemClear(oData, inDataSize);
		}

		return AAX_SUCCESS;
	}

	AAX_Result GetChunkSize(AAX_CTypeID, uint32_t * oSize) const override
	{
		if (!m_instance)
		{
			*oSize = 0;
			return AAX_SUCCESS;
		}

		auto chunk = m_instance->GetCallbacks().OnGetPluginChunk();
		*oSize = chunk.GetSize();
		return AAX_SUCCESS;
	}

	AAX_Result GetChunk(AAX_CTypeID, AAX_SPlugInChunk * oChunk) const override
	{
		if (!m_instance)
		{
			oChunk->fSize = 0;
			return AAX_SUCCESS;
		}

		auto chunk = m_instance->GetCallbacks().OnGetPluginChunk();
		oChunk->fSize = chunk.GetSize();
		MemCopy(chunk.GetData(), oChunk->fData, chunk.GetSize());
		return AAX_SUCCESS;
	}

	AAX_Result SetChunk(AAX_CTypeID, const AAX_SPlugInChunk * iChunk) override
	{
		if (!m_instance)
		{
			return AAX_SUCCESS;
		}

		ArrayView<UInt8> data = { (UInt8*)iChunk->fData, UInt32(iChunk->fSize) };
		m_instance->GetCallbacks().OnSetPluginChunk(data);
		return AAX_SUCCESS;
	}

	REFLEX_INLINE AAX_CParamID GetParamID(UInt32 idx) const { return m_paramIDs[idx]; }

	PluginSession & m_session;
	UInt32 m_classIndex;
	const Configuration::Class & m_class;
	AaxInstance * m_instance;
	Array<char*> m_paramIDs;
};

void AaxInstance::ReportProcessingDelay(UInt32 delay)
{
	if (auto * controller = m_owner.Controller())
	{
		controller->SetSignalLatency(delay);
	}
}

void AaxInstance::ReportParametersChanged()
{
	if (!m_owner.AutomationDelegate())
	{
		return;
	}

	REFLEX_LOOP(i, cls.num_params)
	{
		m_owner.AutomationDelegate()->PostCurrentValue(m_owner.GetParamID(i), GetCallbacks().OnGetParameterValue(i));
	}
}

void AaxInstance::BeginAutomation(UInt32 idx)
{
	if (m_owner.AutomationDelegate())
	{
		m_owner.AutomationDelegate()->PostTouchRequest(m_owner.GetParamID(idx));
		m_automationDepth++;
	}
}

void AaxInstance::Automate(UInt32 idx, Float32 value)
{
	if (m_owner.AutomationDelegate() && m_automationDepth)
	{
		m_owner.AutomationDelegate()->PostSetValueRequest(m_owner.GetParamID(idx), value);
	}
}

void AaxInstance::EndAutomation(UInt32 idx)
{
	if (m_owner.AutomationDelegate() && m_automationDepth)
	{
		m_owner.AutomationDelegate()->PostReleaseRequest(m_owner.GetParamID(idx));
		m_automationDepth--;
	}
}

struct AaxGUI : public AAX_CEffectGUI
{
	void CreateViewContents() override {}

	void CreateViewContainer() override
	{
		if (auto * params = dynamic_cast<AaxParameters*>(GetEffectParameters()))
		{
			auto * instance = params->m_instance;
			if (instance)
			{
				instance->SetViewContainer(GetViewContainer());
				instance->ShowEditor(GetViewContainer()->GetPtr());
			}
		}
	}

	void DeleteViewContainer() override
	{
		if (auto * params = dynamic_cast<AaxParameters*>(GetEffectParameters()))
		{
			if (auto * instance = params->m_instance)
			{
				instance->SetViewContainer(nullptr);
				instance->DiscardEditor();
			}
		}
	}
};

void AAX_CALLBACK AaxProcess(AaxAlgorithmContext * const contextBegin[], const void * contextEnd)
{
	REFLEX_UNUSED(contextEnd);

	AaxAlgorithmContext * ctx = contextBegin[0];

	if (!ctx || !ctx->state || !(*ctx->state))
	{
		return;
	}

	auto & params = *(*ctx->state);
	auto * instance = params.m_instance;

	if (!instance)
	{
		return;
	}

	if (!ctx->bufferSize)
	{
		return;
	}

	instance->PrepareForProcess(UInt32(*ctx->bufferSize), ctx->sampleRate ? *ctx->sampleRate : 0.0f);
	instance->SetBuffers(ctx->inputs, ctx->outputs);
	instance->PushMidiIn(ctx->midiIn ? ctx->midiIn->GetNodeBuffer() : nullptr);

	auto * transport = ctx->midiIn ? ctx->midiIn->GetTransport() : nullptr;
	auto evnts = instance->Process(UInt32(*ctx->bufferSize), transport);

	if (ctx->midiOut)
	{
		REFLEX_FOREACH(e, evnts)
		{
			if (e.type == AudioPlugin::Event::kTypeMIDI)
			{
				AAX_CMidiPacket pkt = {};
				pkt.mTimestamp = e.position;
				pkt.mLength = 3;
				MemCopy(&e.value.u32, pkt.mData, 3);
				ctx->midiOut->PostMIDIPacket(&pkt);
			}
		}
	}
}

AAX_IEffectParameters * AAX_CALLBACK CreateParameters(UInt32 classIndex)
{
	auto * session = ThePluginSession::Get();
	if (!session || classIndex >= session->config.classes.GetSize())
	{
		return nullptr;
	}

	return new AaxParameters(*session, classIndex);
}

template <UInt32 INDEX>
AAX_IEffectParameters * AAX_CALLBACK CreateParametersT()
{
	return CreateParameters(INDEX);
}

AAX_IEffectGUI * AAX_CALLBACK CreateGUI()
{
	return new AaxGUI();
}

static AAX_EStemFormat ChannelsToStemFormat(UInt32 n)
{
	switch (n)
	{
		case 0:  return AAX_eStemFormat_None;
		case 1:  return AAX_eStemFormat_Mono;
		case 2:  return AAX_eStemFormat_Stereo;
		case 3:  return AAX_eStemFormat_LCR;
		case 4:  return AAX_eStemFormat_Quad;
		case 5:  return AAX_eStemFormat_5_0;
		case 6:  return AAX_eStemFormat_5_1;
		default: return AAX_eStemFormat_Stereo;
	}
}

AAX_Result DescribeEffect(AAX_ICollection * collection, UInt32 classIndex)
{
	using Class = AudioPlugin::Configuration::Class;

	auto * session = ThePluginSession::Get();
	auto & cls = session->config.classes[classIndex];

	auto * desc = collection->NewDescriptor();
	if (!desc)
	{
		return AAX_ERROR_NULL_OBJECT;
	}

	desc->AddName(cls.product.GetData());

	
	// Derive AAX category bitmask from Type + Category.
	const auto type = cls.type;
	const UInt32 cat = cls.category;

	UInt32 aax_category = AAX_ePlugInCategory_None;

	if (type == Class::kTypeAudioGenerator)
	{
		aax_category = AAX_ePlugInCategory_SWGenerators;
	}
	else
	{
		// Analyzer is still an effect category in AAX
		if (cat & Class::kAnalyzer)    aax_category |= AAX_ePlugInCategory_Effect;

		if (cat & Class::kEQ)          aax_category |= AAX_ePlugInCategory_EQ;
		if (cat & Class::kDynamics)    aax_category |= AAX_ePlugInCategory_Dynamics;
		if (cat & Class::kPitchShift)  aax_category |= AAX_ePlugInCategory_PitchShift;
		if (cat & Class::kReverb)      aax_category |= AAX_ePlugInCategory_Reverb;
		if (cat & Class::kDelay)       aax_category |= AAX_ePlugInCategory_Delay;
		if (cat & Class::kModulation)  aax_category |= AAX_ePlugInCategory_Modulation;
		if (cat & Class::kDistortion)  aax_category |= AAX_ePlugInCategory_Harmonic;
		if (cat & Class::kRestoration) aax_category |= AAX_ePlugInCategory_NoiseReduction;
		if (cat & Class::kSurround)    aax_category |= AAX_ePlugInCategory_SoundField;

		if (aax_category == AAX_ePlugInCategory_None)
		{
			aax_category = AAX_ePlugInCategory_Effect;
		}
	}

	desc->AddCategory(aax_category);


	auto * comp = desc->NewComponentDescriptor();
	if (!comp)
	{
		return AAX_ERROR_NULL_OBJECT;
	}

	AAX_CFieldIndex idx = 0;
	comp->AddAudioIn(idx++);
	comp->AddAudioOut(idx++);
	comp->AddAudioBufferLength(idx++);
	comp->AddSampleRate(idx++);
	comp->AddMIDINode(idx++, AAX_eMIDINodeType_LocalInput,  "MIDI In",  0xffff);
	comp->AddMIDINode(idx++, AAX_eMIDINodeType_LocalOutput, "MIDI Out", 0xffff);
	comp->AddPrivateData(idx++, sizeof(AaxParameters*));

	auto * properties = comp->NewPropertyMap();
	properties->AddProperty(AAX_eProperty_ManufacturerID,       cls.aax.manufacturer_4cc ? cls.aax.manufacturer_4cc : CCONST('R','F','X','S'));
	properties->AddProperty(AAX_eProperty_ProductID,            cls.aax.product_4cc      ? cls.aax.product_4cc      : CCONST('R','F','X','P'));
	properties->AddProperty(AAX_eProperty_PlugInID_Native,      cls.aax.native_4cc       ? cls.aax.native_4cc       : CCONST('R','F','X','N'));
	properties->AddProperty(AAX_eProperty_PlugInID_AudioSuite,  cls.aax.audiosuite_4cc   ? cls.aax.audiosuite_4cc   : CCONST('R','F','X','A'));
	properties->AddProperty(AAX_eProperty_InputStemFormat,  ChannelsToStemFormat(cls.channels_io.a));
	properties->AddProperty(AAX_eProperty_OutputStemFormat, ChannelsToStemFormat(cls.channels_io.b));
	properties->AddProperty(AAX_eProperty_UsesClientGUI, session->config.view_ctr ? 1 : 0);
	properties->AddProperty(AAX_eProperty_CanBypass, 1);
	properties->AddProperty(AAX_eProperty_UsesTransport, cls.midi_io.a ? 1 : 0);

	comp->AddProcessProc_Native<AaxAlgorithmContext>(&AaxProcess, properties);

	desc->AddComponent(comp);
	desc->AddProcPtr((void*)&CreateGUI, kAAX_ProcPtrID_Create_EffectGUI);

	static void * const kCreateParamsFns[] =
	{
		(void*)&CreateParametersT<0>, (void*)&CreateParametersT<1>, (void*)&CreateParametersT<2>, (void*)&CreateParametersT<3>,
		(void*)&CreateParametersT<4>, (void*)&CreateParametersT<5>, (void*)&CreateParametersT<6>, (void*)&CreateParametersT<7>,
		(void*)&CreateParametersT<8>, (void*)&CreateParametersT<9>, (void*)&CreateParametersT<10>, (void*)&CreateParametersT<11>,
		(void*)&CreateParametersT<12>, (void*)&CreateParametersT<13>, (void*)&CreateParametersT<14>, (void*)&CreateParametersT<15>,
	};

	if (classIndex >= GetArraySize(kCreateParamsFns))
	{
		return AAX_ERROR_INVALID_PARAMETER_ID;
	}

	desc->AddProcPtr(kCreateParamsFns[classIndex], kAAX_ProcPtrID_Create_EffectParameters);

	collection->AddEffect(cls.product.GetData(), desc);
	return AAX_SUCCESS;
}

REFLEX_END_INTERNAL

extern "C" AAX_Result GetEffectDescriptions(AAX_ICollection * outCollection)
{
	using namespace Reflex::System::Common;

	auto * session = ThePluginSession::Get();
	if (!session)
	{
		return AAX_ERROR_NULL_OBJECT;
	}

	for (UInt32 i = 0; i < session->config.classes.GetSize(); ++i)
	{
		auto err = DescribeEffect(outCollection, i);
		if (err != AAX_SUCCESS)
		{
			return err;
		}
	}

	return AAX_SUCCESS;
}
