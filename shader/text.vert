#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out VertexData {
	vec2 pos;
	vec2 uv;
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
	vec2(1.0, 1.0),
	vec2(0.0, 1.0),
	
	vec2(1.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 0.0)
);

void main() {
	vert.uv = uvs[gl_VertexIndex];
	vert.pos = positions[gl_VertexIndex] * 0.7;
	gl_Position = vec4(vert.pos, 0.0, 1.0);
}