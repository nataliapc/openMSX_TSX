// $Id$

uniform sampler2D edgeTex;
uniform sampler2D colorTex;
uniform sampler2D weightTex;

varying vec2 left;
varying vec2 mid;
varying vec2 right;
varying vec2 edgePos;
varying vec2 weightPos;

void main()
{
	float edgeBits = texture2D(edgeTex, edgePos).x;
	//gl_FragColor = vec4(edgeBits, fract(edgeBits * 16.0), fract(edgeBits * 256.0), 1.0);

	// transform (N x N x 4096) to (64N x 64N) texture coords
	float t = 64.0 * edgeBits;
	vec2 xy = vec2(fract(t), floor(t)/64.0);
	xy += fract(weightPos) / 64.0;
	vec3 weights = texture2D(weightTex, xy).xyz;
	//gl_FragColor = vec4(weights, 1.0);

	vec4 c4 = texture2D(colorTex, left );
	vec4 c5 = texture2D(colorTex, mid  );
	vec4 c6 = texture2D(colorTex, right);

	gl_FragColor = c4 * weights.x + c5 * weights.y + c6 * weights.z;
}
