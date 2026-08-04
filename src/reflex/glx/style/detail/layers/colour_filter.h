#pragma once

#include "render.h"




//
//Internal

REFLEX_NS(Reflex::GLX::Detail)

struct Matrix4x4
{
	Matrix4x4()
	{
		Identity();
	}

	Matrix4x4(NoValue) {}	//uninitalized

	void Identity()
	{
		Set
		({
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		});
	}

	void Set(const ArrayView <Float> & values)
	{
		REFLEX_ASSERT(values.size == 16);

		MemCopy(values.data, data, sizeof(Matrix4x4));
	}


	SIMD::FloatV4 data[4];	//SIMD::FloatV4 ensures alignment
};

struct ColourFilterImpl
{
	struct Properties : public StandardWrapperProperties
	{
		Colour values = kWhite;
	};

	struct Scratch : public StandardWrapperScratch
	{
		Matrix4x4 colour_matrix = kNoValue;	//no init required, is guaranteed to be set in OnAlign
	};

	struct Renderer;

	static void Init(GenericPropertiesSchema & schema);

	static void OnAlign(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth);

	static TRef <Graphic> OnRedraw(const GenericLayer& self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags);
};

struct ColourFilterImpl::Renderer : public StandardLayersRenderer
{
	struct CacheStackEntry
	{
		CacheStackEntry(UInt uid)
			: uid(uid)
		{
		}

		const UInt uid;

		Matrix4x4 matrix;
	};

	Renderer(Scratch & scratch, Size pixelsize);

	void Render(const System::Renderer::Transform & transform, const System::Colour & colour) const override;


	const Matrix4x4 & colour_matrix;

	mutable CacheStackEntry m_stack_entry;

	mutable UInt m_previous_parent_uid_z;


	inline static UInt st_uid_counter = 1;

	inline static CacheStackEntry st_initial_stack_entry = { 1 };

	inline static CacheStackEntry * st_current = &st_initial_stack_entry;
};

REFLEX_END
