#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out VertexData {
	vec2 pos;
	vec2 uv;
	vec2 st;
} vert;

out gl_PerVertex {
	vec4 gl_Position;
};

vec2 positions[6] = vec2[](
    vec2(-0.3, -0.2),
    vec2(0.3, 0.2),
    vec2(-0.3, 0.2),

    vec2(0.3, 0.2),
    vec2(-0.3, -0.2),
    vec2(0.3, -0.2)
);

vec2 uvs[6] = vec2[](
	vec2(0.0, 0.0),
	vec2(192.0, 64.0),
	vec2(0.0, 64.0),
	
	vec2(192.0, 64.0),
	vec2(0.0, 0.0),
	vec2(192.0, 0.0)
);

void main() {
	vert.uv = uvs[gl_VertexIndex];
	vert.st = uvs[gl_VertexIndex] / vec2(192.0, 64.0);
	vert.pos = positions[gl_VertexIndex];
	gl_Position = vec4(vert.pos, 0.0, 1.0);
}