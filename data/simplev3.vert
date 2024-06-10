#version 300 es
layout(location = 0) in vec3 vPosition;
out vec4 color;
out float green;

void main (void)
{
	gl_Position = vec4(vPosition, 1);
	green = (float(1.0) + gl_Position.z ) / float(2.0);
	color = vec4(0, green, 0, 1);
}
