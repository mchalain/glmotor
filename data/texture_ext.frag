#if GL_ES
#extension GL_OES_EGL_image_external : require
precision mediump float;
uniform samplerExternalOES vTexture;
#else
uniform sampler2D vTexture;
#endif
varying vec2 texUV;

void main(void)
{
	gl_FragColor = texture2D(vTexture, texUV);
}
