attribute vec3 vPosition;
attribute vec3 vColor;
uniform mat4 vMove;
varying float green;
varying vec4 color;

void main (void)
{
	gl_Position = vMove * vec4(vPosition, 1);
	green = (float(1.0) + gl_Position.z ) / float(2.0);
	if (vColor.r == float(0.0) && vColor.g == float(0.0) && vColor.b == float(0.0))
		color = vec4(0.5, green, 0.5, 1);
	else
		color = vec4(vColor, 1);
}
