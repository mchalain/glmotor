attribute vec3 vPosition;
attribute vec4 vColor;
attribute vec2 vUV;
attribute vec3 vNormal;
varying vec4 color;
varying vec2 texUV;
varying vec2 resolution;
varying vec3 normal;
varying vec3 lightdir;
varying vec3 ambient;

uniform mat4 vMove;
uniform vec4 vResolution;

void main (void)
{
	ambient = normalize(vec3(1.0, 1.0, 0.8) * clamp(vPosition.z + 1.0, 0.0, 2.0));
	lightdir = vec3(24.0, -6.0, 25.0);
	gl_Position = vMove * vec4(vPosition, 1);
	if (length(vColor.rgb) != 0.0)
		color = vColor;
	else
		color = vec4(vec3(1.0, 0.9, 0.5) * (vPosition.z + 1.0), 1.0);
	texUV = vUV;
	resolution = vResolution.zw;
	normal = vNormal;
}
