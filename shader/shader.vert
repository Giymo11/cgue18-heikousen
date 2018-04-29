#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform UBO {
    mat4 mvp;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 outNormal;

void main() {
    gl_Position = ubo.mvp * vec4(pos, 1.0);
    outNormal = normal;
}
