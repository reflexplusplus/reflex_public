#define kFilterModeNone 0
#define kFilterModePixelate 1
#define kFilterModeBlur 2

cbuffer ViewPort : register(b0)
{
	uniform float2 gViewportOrigin;
	uniform float2 gViewportSize;
}

cbuffer Matrix : register(b1)
{
	uniform float2 gOffset;
	uniform float2 gScale;
	uniform float4 gColour;
}

cbuffer ColourFilters : register(b0)
{
	float4x4 gColourTransform;
	float gDitherAmount;
	uint pad1[3];
}

cbuffer TextureFilters : register(b1)
{
	float4 gFilterParams;
	uint gFilterMode;
	uint pad2[3];
}

struct FilterParam
{
	float4 params;
};

StructuredBuffer<FilterParam> gExtraParams : register(t2);


#if TEXTURE
	Texture2D gTexture : register(t0);
#endif

#if MASK
	Texture2D gMaskTexture : register(t1);
#endif

#if TEXTURE||MASK
	SamplerState gSampler
	{
		Filter      = MIN_MAG_LINEAR_MIP_POINT;
		AddressU    = CLAMP;
		AddressV    = CLAMP;
	};
#endif

struct VertexInput
{
	float2 position : POS;

#ifdef TEXTURE
	float2 uv : TEXCOORD0;
#endif

#ifdef COLOUR
	float4 colour : COLOR;
#endif
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float4 colour : COLOR;

#ifdef TEXTURE
	float2 uv : TEXCOORD0;
#endif

#ifdef MASK
	float2 maskuv : TEXCOORD1;
#endif
};

float4 FilterColour(float4 colour)
{
	return mul(gColourTransform, colour);
}

float DitherNoise(float2 position)
{
	return frac(52.9829189 * frac(dot(position, float2(0.06711056, 0.00583715))));
}

float4 FilterSampleFetch(Texture2D tex, SamplerState samp, float2 uv)
{
	if (gFilterMode == kFilterModePixelate)
	{
		float2 mosaicSize = gFilterParams.xy;

		uv = floor(uv / mosaicSize) * mosaicSize;
	}
	else if (gFilterMode == kFilterModeBlur)
	{
		float w = gExtraParams[0].params[0];
		float4 color = w * tex.Sample(samp, uv);

		// symmetric taps
		float2 texelSize = gFilterParams.xy;
		uint radius = uint(gFilterParams.z);

		for (uint i = 1; i <= radius; ++i)
		{
			float2 offset = texelSize * i;

			w = gExtraParams[i / 4].params[i % 4];

			color += w * tex.Sample(samp, uv + offset);
			color += w * tex.Sample(samp, uv - offset);
		}

		return color;
	}

	return tex.Sample(samp, uv);
}

VertexOutput VertexMain(VertexInput input)
{
	float2 t = input.position * gScale;

	t = t + gOffset;


	t += gViewportOrigin;

	t = t / gViewportSize;


	VertexOutput output;

	output.position = float4(t, 0.0, 1.0);

	output.colour = gColour;

#ifdef COLOUR
	output.colour *= input.colour;
#endif

#ifdef TEXTURE
	output.uv = input.uv;
#endif

#ifdef MASK
	output.maskuv = (t + float2(1.0, 1.0)) * 0.5;
#endif

	output.colour.rgb *= output.colour.a;

	return output;
}

float4 PixelMain(VertexOutput input) : SV_TARGET
{
#ifdef TEXTURE

	#ifdef LUMINANCE
		float a = FilterSampleFetch(gTexture, gSampler, input.uv).a;
		float4 colour = input.colour * a;
	#else
		float4 colour = FilterSampleFetch(gTexture, gSampler, input.uv) * input.colour;
	#endif

	#ifdef MASK
		colour *= gMaskTexture.Sample(gSampler, input.maskuv);
	#endif

		colour = FilterColour(colour);

#else

	float4 colour = input.colour;

	#ifdef MASK
		colour *= gMaskTexture.Sample(gSampler, input.maskuv);
	#endif

	colour = FilterColour(colour);

#endif

	if (gDitherAmount > 0.0f)
	{
		float noise = DitherNoise(input.position.xy) - 0.5;
		colour.rgb = saturate(colour.rgb + (noise * gDitherAmount) * colour.a);
	}

	return colour;
}
