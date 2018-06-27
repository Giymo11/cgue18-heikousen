#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in VertexData {
    vec2 uv;
} vert;

layout (location = 0) out vec4 outColor;
layout (binding = 1) uniform sampler2D hdrInputs;

float luminance(vec3 rgb) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    return dot(rgb, W);
}

void main() {
    outColor = vec4(1.);
}