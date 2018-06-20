#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const int LIGHT_COUNT = 16;

struct LightSource {
    vec3 position;
    vec3 color;
    vec3 attenuation;
};

layout(location = 0) in VertexData {
    vec2 uv;
} vert;

layout (location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform LightBlock {
    vec4 parameters;
    LightSource lights[LIGHT_COUNT];
} lightInfo;

layout (binding = 1) uniform sampler2D position;
layout (binding = 2) uniform sampler2D normal;
layout (binding = 3) uniform sampler2D albedo;

void main() {
    outColor.rgb = texture(albedo, vert.uv).rgb;
    outColor.a   = 1.;
}