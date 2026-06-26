#define kFilterModeNone 0
#define kFilterModePixelate 1
#define kFilterModeBlur 2

#if ES
    precision highp float;
    precision highp int;

	#define IN in mediump
	#define OUT out mediump

	#define gl_FragColor m_FragColor

	OUT vec4 m_FragColor;
#else

	#define IN varying
	#define mediump
	#define highp

	// because we use #version 120
	#define texture texture2D
#endif

#if TX
	uniform sampler2D g_texture;

    uniform highp vec4 g_filterparams[5]; // 4 filter params + 16 extra (blur weights)
    uniform mediump int g_filtermode;

	IN vec2 m_texcoord;


    vec4 FilterSampleFetch(vec2 uv)
    {
        if (g_filtermode == kFilterModePixelate) {
            vec2 mosaicSize = g_filterparams[0].xy;

            uv = floor(uv / mosaicSize) * mosaicSize;
        }
        else if (g_filtermode == kFilterModeBlur) {
            // Center tap
            float w = g_filterparams[0].w;
            vec4 color = w * texture(g_texture, uv);

            // symmetric taps
            vec2 texelSize = g_filterparams[0].xy;
            int radius = int(g_filterparams[0].z);

            for (int i = 1; i <= radius; ++i) {
                vec2 offset = texelSize * float(i);
                int vecIndex = (i + 3) / 4;
                int vecSubindex = (i + 3) - vecIndex * 4;

                float w = g_filterparams[vecIndex][vecSubindex];

                color += w * texture(g_texture, uv + offset);
                color += w * texture(g_texture, uv - offset);
            }

            return color;
        }

        return texture(g_texture, uv);
    }
#endif

uniform highp mat4 g_colourTransform;
uniform highp float g_dither_amount;

IN vec4 m_VertexColor;

highp float DitherNoise(vec2 position)
{
    return fract(52.9829189 * fract(dot(position, vec2(0.06711056, 0.00583715))));
}

void main()
{
#if TX
    mediump vec4 colour = FilterSampleFetch(m_texcoord) * m_VertexColor;
#else
	mediump vec4 colour = m_VertexColor;
#endif
	colour = g_colourTransform * colour;

    if (g_dither_amount > 0.0) {
        highp float noise = DitherNoise(gl_FragCoord.xy) - 0.5;
        colour.rgb = clamp(colour.rgb + (noise * g_dither_amount) * colour.a, 0.0, 1.0);
    }

	gl_FragColor = colour;
}
