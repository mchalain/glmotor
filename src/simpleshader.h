#ifndef __SIMPLESHADER_H__
#define __SIMPLESHADER_H__

const static char simple_vertex[] = "\
attribute vec3 vPosition;\n\
attribute vec3 vColor;\n\
attribute vec2 vUV;\n\
varying vec4 color;\n\
varying vec2 texUV;\n\
varying vec2 resolution;\n\
\n\
uniform mat4 vMove;\n\
uniform vec2 vResolution;\n\
\n\
void main (void)\n\
{\n\
	gl_Position = vMove * vec4(vPosition, 1);\n\
	color = vec4(vColor, 1.0);\n\
	texUV = vUV;\n\
	resolution = vResolution;\n\
}\n";

const static char simple_fragment[] = "\
#if GL_ES\n\
precision mediump float;\n\
#endif\n\
varying vec4 color;\n\
\n\
void main(void)\n\
{\n\
	gl_FragColor = color;\n\
}\n";

#endif
