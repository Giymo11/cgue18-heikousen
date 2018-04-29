#version 450
#extension GL_ARB_separate_shader_objects : enable

const int LIGHT_COUNT = 2;

struct LightSource {
    vec3 position;
    vec3 color;
    vec3 attenuation;
};

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform MaterialInfo {
	float ambient;
	float diffuse;
	float specular;
	float alpha;
} materialInfo;

layout(binding = 3) uniform LightBlock {
    LightSource lights[LIGHT_COUNT];
};

layout(binding = 4) uniform sampler2D diffuseTex;

vec3 phong(vec3 intensity, vec3 l, vec3 n, vec3 v) {
    vec3 diffuse = texture(diffuseTex, inUv).xyz * intensity * max(dot(n,l), 0.0) * materialInfo.diffuse;

    vec3 reflection = normalize(-reflect(l, n));
    float rv = max(dot(reflection, v), 0.0);
    vec3 specular = intensity * materialInfo.specular * pow(rv, materialInfo.alpha);

    return clamp(diffuse, 0.0, 1.0) + specular;
}

vec3 point(LightSource light, vec3 p, vec3 n, vec3 v) {
    float d = distance(p, light.position);
    float attenuation = light.attenuation.x + d * light.attenuation.y + d * d * light.attenuation.z;

    return phong(light.color, normalize(light.position - p), n, v) * (1 / attenuation);
}

void main() {
    vec3 n = normalize(inNormal);
    vec3 v = normalize(-inPosition.xyz);

    outColor = texture(diffuseTex, inUv) * materialInfo.ambient;
    for (int i = 0; i < LIGHT_COUNT; i++) {
        outColor.xyz += point(lights[i], inPosition.xyz, n, v);
    }
}