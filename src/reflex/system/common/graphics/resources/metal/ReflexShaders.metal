#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Common with Reflex::System::Renderer::FilterMode
enum FilterMode : uint32_t
{
	kFilterModeNone,

	kFilterModePixelate,
	kFilterModeBlur,
};

enum VertexBufferIndex {
	VBI_Vertices, VBI_Uniforms
};

enum FragmentBufferIndex {
	FBI_Uniforms, FBI_ExtraParams
};

enum TextureBufferIndex {
	TBI_Color
};

enum SamplerBufferIndex {
	SBI_Color
};

typedef struct {
	float2 position [[attribute(0)]];
} VertexPoint;

typedef struct {
	float2 position [[attribute(0)]];
	float4 colour [[attribute(1)]];
} VertexColourPoint;

typedef struct {
	float2 position [[attribute(0)]];
	float2 texCoord [[attribute(1)]];
} VertexTextureCoord;

typedef struct {
	float2 viewportOrigin;
	float2 viewportSize; // in logical pixels
	float2 offset;
	float2 scale;
	float4 colour;
} VertexUniforms;

typedef struct {
	// Filters
	float4x4 colourTransform;
	float4 filterParams;
	FilterMode filterMode;
	float ditherAmount;
	uint3 pad;
} FragmentUniforms;

// End Common

typedef struct {
	float4 position [[position]];
	float4 colour;
	float2 texCoord;
	float pointSize [[point_size]]; // Special built-in for point size
} ColorInOut;

inline float2 viewportSizeInLogicalPixels(constant VertexUniforms& uniforms) {
	return float2(uniforms.viewportSize.x - uniforms.viewportOrigin.x, -uniforms.viewportSize.y - uniforms.viewportOrigin.y);
}

inline float4 filterColour(float4 colour, constant FragmentUniforms& uniforms) {
	return uniforms.colourTransform * colour;
}

inline float ditherNoise(float2 position) {
	return fract(52.9829189 * fract(dot(position, float2(0.06711056, 0.00583715))));
}

inline float4 filterSampleFetch(texture2d<float> colorMap, sampler samplr, float2 uv, constant FragmentUniforms& uniforms, constant float* filterExtra) {
	switch (uniforms.filterMode) {
		case kFilterModePixelate: {
			float2 mosaicSize = uniforms.filterParams.xy;

			uv = floor(uv / mosaicSize) * mosaicSize;
			break;
		}

		case kFilterModeBlur: {
			float w = filterExtra[0];
			float4 color = w * colorMap.sample(samplr, uv);

			// symmetric taps
			float2 texelSize = uniforms.filterParams.xy;
			uint radius = uint(uniforms.filterParams.z);

			for (uint i = 1; i <= radius; ++i)
			{
				float2 offset = texelSize * i;

				w = filterExtra[i];

				color += w * colorMap.sample(samplr, uv + offset);
				color += w * colorMap.sample(samplr, uv - offset);
			}

			return color;
		}

		default: break;
	}

	return colorMap.sample(samplr, uv).rgba;
}

// All vertex shaders must begin with the keyword vertex
// The function must return (at least) the final position of the vertex.
vertex ColorInOut vertFunc_point(VertexPoint			 in			[[stage_in]],
								 constant VertexUniforms &uniforms	[[buffer(VBI_Uniforms)]]) {
	float2 t = in.position * uniforms.scale;
	t += uniforms.offset;
	t += uniforms.viewportOrigin;
	t /= uniforms.viewportSize;

	ColorInOut out;
	out.position = float4(t.x, t.y, 0.0, 1.0);
	out.colour = uniforms.colour;
	out.pointSize = 1.0;

	out.colour.rgb *= out.colour.a;   //premult alpha
	return out;
}

vertex ColorInOut vertFunc_colPoint(VertexColourPoint		in        [[stage_in]],
								    constant VertexUniforms	&uniforms [[buffer(VBI_Uniforms)]]) {
	float2 t = in.position * uniforms.scale;
	t += uniforms.offset;
	t += uniforms.viewportOrigin;
	t /= uniforms.viewportSize;

	ColorInOut out;
	out.position = float4(t.x, t.y, 0.0, 1.0);
	out.colour = in.colour * uniforms.colour;
	out.pointSize = 1.0;

	out.colour.rgb *= out.colour.a;   //premult alpha
	return out;
}

vertex ColorInOut vertFunc_texCoord(VertexTextureCoord		in			[[stage_in]],
								    constant VertexUniforms	&uniforms	[[buffer(VBI_Uniforms)]]) {
	float2 t = in.position * uniforms.scale;
	t += uniforms.offset;
	t += uniforms.viewportOrigin;
	t /= uniforms.viewportSize;

	ColorInOut out;
	out.position = float4(t.x, t.y, 0.0, 1.0);
	out.colour = uniforms.colour;
	out.texCoord = in.texCoord;
	out.pointSize = 1.0;

	out.colour.rgb *= out.colour.a;   //premult alpha (not actually sure why needed here, but renders wrong without)
	return out;
}

// half4 is a four-component color value RGBA. Note that half4 is more memory efficient than float4 because you’re writing to less GPU memory.
fragment float4 fragFunc_colPoint(ColorInOut in [[stage_in]],
							      constant FragmentUniforms& uniforms [[buffer(FBI_Uniforms)]])
{
	float4 colour = filterColour(in.colour, uniforms);

	if (uniforms.ditherAmount > 0.0f) {
		float noise = ditherNoise(in.position.xy) - 0.5f;
		colour.rgb = clamp(colour.rgb + (noise * uniforms.ditherAmount) * colour.a, 0.0f, 1.0f);
	}

	return colour;
}

fragment float4 fragFunc_texRGBA(ColorInOut in [[stage_in]],
								 constant FragmentUniforms& uniforms [[buffer(FBI_Uniforms)]],
								 texture2d<float> colorMap [[texture(TBI_Color)]],
								 sampler samplr [[sampler(SBI_Color)]],
								 constant float* filterExtra [[buffer(FBI_ExtraParams)]])
{
	float4 sample = filterSampleFetch(colorMap, samplr, in.texCoord.xy, uniforms, filterExtra);
	float4 colour = filterColour(sample * in.colour, uniforms);

	if (uniforms.ditherAmount > 0.0f) {
		float noise = ditherNoise(in.position.xy) - 0.5f;
		colour.rgb = clamp(colour.rgb + (noise * uniforms.ditherAmount) * colour.a, 0.0f, 1.0f);
	}

	return colour;
}

fragment float4 fragFunc_texLuminance(ColorInOut in [[stage_in]],
									  constant FragmentUniforms& uniforms [[buffer(FBI_Uniforms)]],
									  texture2d<float> colorMap [[texture(TBI_Color)]],
									  sampler samplr [[sampler(SBI_Color)]],
									  constant float* filterExtra [[buffer(FBI_ExtraParams)]])
{
	float4 sample = filterSampleFetch(colorMap, samplr, in.texCoord.xy, uniforms, filterExtra);
	float a = sample.r;
	float4 colour = filterColour(float4(a, a, a, a) * in.colour, uniforms);

	if (uniforms.ditherAmount > 0.0f) {
		float noise = ditherNoise(in.position.xy) - 0.5f;
		colour.rgb = clamp(colour.rgb + (noise * uniforms.ditherAmount) * colour.a, 0.0f, 1.0f);
	}

	return colour;
}
