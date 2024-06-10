attribute vec3 vPosition;
varying float green;
varying vec4 color;

const mat4 moving =
mat4(1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1);
void main (void)
{
	gl_Position = moving * gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(vPosition, 1);
	green = (float(1.0) + gl_Position.z ) / float(2.0);
	color = vec4(0.5, green, 0.5, 1);
}
