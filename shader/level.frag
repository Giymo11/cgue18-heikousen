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

layout(std140, binding = 1) uniform LightBlock {
	vec4 parameters;
    LightSource lights[LIGHT_COUNT];
} lightInfo;

layout(binding = 2) uniform sampler2DArray diffuseTex1;
layout(binding = 3) uniform sampler2DArray lightmap;

vec3 phong(vec3 objColor, vec3 intensity, vec3 l, vec3 n, vec3 v) {
    vec3 diffuse = objColor * intensity * max(dot(n,l), 0.0) * diffuse;

    vec3 reflection = normalize(-reflect(l, n));
    float rv = max(dot(reflection, v), 0.0);
    vec3 specular = intensity * specular * pow(rv, alpha);

    return diffuse + specular;
}

vec3 point(vec3 objColor, LightSource light, vec3 p, vec3 n, vec3 v) {
    float d = distance(p, light.position);
    float attenuation = light.attenuation.x + d * light.attenuation.y + d * d * light.attenuation.z;

    return phong(objColor, light.color, normalize(light.position - p), n, v) * (1 / attenuation);
}

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

    outColor = vec4(objColor.rgb * lightmap * 1.4, objColor.w);
    /* for (int i = 0; i < LIGHT_COUNT; i++)
        outColor.xyz += point(objColor.xyz, lightInfo.lights[i], vert.position, vert.normal, v); */

	outColor.xyz = tonemaping_exposure(outColor.xyz, exposure);
	outColor.xyz = gamma_adjust(outColor.xyz, gamma);
}