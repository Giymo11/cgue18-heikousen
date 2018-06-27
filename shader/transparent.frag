#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in VertexData {
    vec3 position;
    vec3 normal;
    vec3 uv;
    vec3 lightUv;
    float linearDepth;
} vert;

const int LIGHT_COUNT = 16;

struct LightSource {
    vec3 position;
    vec3 color;
    vec3 attenuation;
};

layout (location = 0) out vec4 outColor;

// layout(std140, binding = 0) uniform LightBlock {
//    vec4 parameters;
//    vec4 playerPos;
//    LightSource lights[LIGHT_COUNT];
// } lightInfo;

layout (binding = 1) uniform sampler2DArray diffuseTex1;
layout (binding = 2) uniform sampler2DArray lightmap;

void main() {
    vec4 objColor = texture(diffuseTex1, vert.uv);
    vec3 lightmap = texture(lightmap, vec3(vert.lightUv.xy, 1.0)).rgb;

    outColor     = vec4(objColor.rgb * lightmap, 0.3);
}