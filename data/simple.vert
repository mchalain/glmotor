varying vec4 color;
const mat4 moving =
mat4(1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1);
varying float green;
void main (void)
{
	gl_Position = moving * gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
	green = (float(1.0) + gl_Vertex.z ) / float(2.0);
	color = vec4(0.5, green, 0.5, 1);
}
