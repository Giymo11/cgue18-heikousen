#version 450
#extension GL_ARB_separate_shader_objects : enable

const int LIGHT_COUNT = 5;

struct LightSource {
    vec3 position;
    vec3 color;
    vec3 attenuation;
};

layout(location = 0) in VertexData {
	vec3 position;
	vec3 normal;
	vec3 uv;
	vec3 lightUv;
} vert;

layout(location = 0) out vec4 outColor;

const float ambient  = 0.2;
const float diffuse  = 0.95;
const float specular = 0.2;
const float alpha    = 2.0;
const float gamma    = 1.22;
const float exposure = 3.0;

layout(binding = 1) uniform sampler2DArray diffuseTex1;
layout(binding = 2) uniform sampler2DArray lightmap;

vec3 tonemaping_exposure(vec3 hdrColor, float hdrExposure) {
    return vec3(1.0) - exp(-hdrColor * hdrExposure);
}

vec3 gamma_adjust(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}

void main() {
	vec4 objColor = texture(diffuseTex1, vert.uv);
	vec3 lightmap = texture(lightmap, vec3(vert.lightUv.xy, 1.0)).rgb;
	vec3 v = normalize(-vert.position);

    outColor = vec4(objColor.rgb * lightmap, objColor.w);
	outColor.xyz = tonemaping_exposure(outColor.xyz, exposure);
	outColor.xyz = gamma_adjust(outColor.xyz, gamma);
}