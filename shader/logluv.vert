#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out VertexData {
    vec2 uv;
} vert;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vert.uv.x = (gl_VertexIndex << 1) & 2;
    vert.uv.y = gl_VertexIndex & 2;

    gl_Position = vec4(vert.uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
