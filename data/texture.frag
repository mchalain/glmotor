#if GL_ES
precision mediump float;
#endif
varying vec2 texUV;

uniform sampler2D vTexture;

void main(void)
{
	gl_FragColor = texture2D(vTexture, texUV);
}
