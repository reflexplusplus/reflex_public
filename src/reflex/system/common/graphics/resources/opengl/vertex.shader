#if ES
	#define IN in
	#define OUT out
#else
	#define IN attribute
	#define OUT varying
#endif


uniform vec2 g_viewport_origin;
uniform vec2 g_viewport_size;

uniform vec2 g_offset;
uniform vec2 g_scale;
uniform vec4 g_colour;

IN vec2 position;

#if TX
	IN vec2 texcoord;
#else
	IN vec4 colour;
#endif

OUT vec4 m_VertexColor;	//specify a color output to the fragment shader

#if TX
	OUT vec2 m_texcoord;
#endif

void main()
{
	vec2 t = position * g_scale;

	t = t + g_offset;


	t += g_viewport_origin;

	t = t / g_viewport_size;


	gl_Position = vec4(t.x, t.y, 0.0, 1.0);

#if TX
	m_VertexColor = g_colour;
	m_texcoord = texcoord;
#else
	m_VertexColor = g_colour * colour;
#endif
	m_VertexColor.rgb *= m_VertexColor.a; //premult alpha
}
