#if GL_ES
precision mediump float;
#endif
varying vec2 texUV;
varying vec4 color;
varying vec3 normal;
varying vec3 lightdir;
varying vec3 ambient;

uniform sampler2D vTexture;

void main(void)
{
	vec4 light = vec4(1.0,1.0,1.0, 1.0) * clamp(dot(normal, lightdir), 0.0, 1.0);
	vec4 color2 = vec4(vec3(color.rgb * light.rgb * ambient.rgb) * 2.0, 1.0);
	vec4 color3 = texture2D(vTexture, texUV);
//	gl_FragColor = vec4(color3.rgb, 1.0);
	if (length(color3.rgb) != 0.0)
	{
		gl_FragColor = vec4(color3.rgb * color2.rgb, color3.a);
		gl_FragColor = vec4(color3.rgb * color2.rgb, 1.0);
	}
	else
		gl_FragColor = vec4(color2.rgb, 1.0);
//	gl_FragColor = vec4(light.rgb, 1.0);
//	gl_FragColor = vec4(color.rgb, 1.0);
}
