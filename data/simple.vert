// #version 100 es
attribute vec3 vPosition;
attribute vec3 vColor;
attribute vec2 vUV;
varying vec4 color;
varying vec2 texUV;

uniform mat4 vMove;

void main (void)
{
	gl_Position = vMove * vec4(vPosition, 1);
	color = vec4(vColor, 1.0);
	texUV = vUV;
}
