#extension GL_OES_EGL_image_external : require
#if GL_ES
precision mediump float;
#endif
varying vec2 texUV;

uniform samplerExternalOES vTexture;

void main(void)
{
	gl_FragColor = texture2D(vTexture, texUV);
}
