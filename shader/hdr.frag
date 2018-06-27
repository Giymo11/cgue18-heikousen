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
    vec4 playerPos;
    LightSource lights[LIGHT_COUNT];
} lightInfo;

layout (binding = 1) uniform sampler2D hdrInputs;

vec3 tonemaping_exposure(vec3 hdrColor, float hdrExposure) {
     return vec3(1.0) - exp(-hdrColor * hdrExposure);
}

vec3 tonemaping_reinhard(vec3 hdrColor) {
    return hdrColor / (hdrColor + vec3(1.0));
}

vec3 gamma_adjust(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}

void main() {
    vec3 color        = texture(hdrInputs, vert.uv).rgb;
    float gamma       = lightInfo.parameters.x;
    float hdrMode     = lightInfo.parameters.y;
    float hdrExposure = lightInfo.parameters.z;

    if(hdrMode > 1.5) {
        color = tonemaping_exposure(color, hdrExposure);
    } else if (hdrMode > 0.5) {
        color = tonemaping_reinhard(color);
    }

    outColor = vec4(gamma_adjust(color, gamma), 1.);
}