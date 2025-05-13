#ifndef __SIMPLESHADER_H__
#define __SIMPLESHADER_H__

const static char simple_vertex[] = "\
attribute vec3 vPosition;\n\
attribute vec4 vColor;\n\
attribute vec2 vUV;\n\
attribute vec3 vNormal;\n\
varying vec4 color;\n\
varying vec2 texUV;\n\
varying vec2 resolution;\n\
varying vec3 normal;\n\
\n\
uniform mat4 vMove;\n\
uniform vec4 vResolution;\n\
\n\
void main (void)\n\
{\n\
	gl_Position = vMove * vec4(vPosition, 1.0);\n\
	color = vColor;\n\
	texUV = vUV;\n\
	resolution = vResolution.zw;\n\
	normal = vNormal;\n\
}\n";

const static char simple_fragment[] = "\
#if GL_ES\n\
precision mediump float;\n\
#endif\n\
varying vec4 color;\n\
varying vec3 normal;\n\
varying vec2 resolution;\n\
\n\
void main(void)\n\
{\n\
	gl_FragColor = color;\n\
}\n";

#endif
