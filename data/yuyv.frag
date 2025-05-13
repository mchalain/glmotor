#ifdef GL_ES
#extension GL_OES_EGL_image_external : require
precision mediump float;
uniform samplerExternalOES vTexture;
#else
uniform sampler2D vTexture;
#endif
varying vec4 color;
varying vec2 texUV;


void main()
{
	float s = texUV.x * 0.5;
	float t = texUV.y * 0.5;
	vec4 color = texture2D(vTexture, vec2(s, t)).argb;
	float y = (color.a + color.g) * 0.5;
	float u = color.b - 0.5;
	float v = color.r - 0.5;
	float r = clamp(y + 1.402 * v, 0.0, 1.0);
	float g = clamp(y - 0.344 * u - 0.714 * v, 0.0, 1.0);
	float b = clamp(y + 1.772 * u, 0.0, 1.0);
	gl_FragColor = vec4(r, g, b, 1);
}
