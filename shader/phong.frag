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
	vec2 uv;
} vert;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform MaterialInfo {
	float ambient;
	float diffuse;
	float specular;
	float alpha;

	float texture;
	float param1;
	float param2;
	float param3;
} materialInfo;

layout(std140, binding = 3) uniform LightBlock {
	vec4 parameters;
    LightSource lights[LIGHT_COUNT];
} lightInfo;

layout(binding = 4) uniform sampler2D diffuseTex1;
layout(binding = 5) uniform sampler2D diffuseTex2;
layout(binding = 6) uniform sampler2D diffuseTex3;

vec3 phong(vec3 objColor, vec3 intensity, vec3 l, vec3 n, vec3 v) {
    vec3 diffuse = objColor * intensity * max(dot(n,l), 0.0) * materialInfo.diffuse;

    vec3 reflection = normalize(-reflect(l, n));
    float rv = max(dot(reflection, v), 0.0);
    vec3 specular = intensity * materialInfo.specular * pow(rv, materialInfo.alpha);

    return clamp(diffuse + specular, 0.0, 1.0);
}

vec3 point(vec3 objColor, LightSource light, vec3 p, vec3 n, vec3 v) {
    float d = distance(p, light.position);
    float attenuation = light.attenuation.x + d * light.attenuation.y + d * d * light.attenuation.z;

    return phong(objColor, light.color, normalize(light.position - p), n, v) * (1 / attenuation);
}

void main() {
	vec4 objColor = texture(diffuseTex1, vert.uv);
	if (materialInfo.texture > 1.5)
		objColor = texture(diffuseTex3, vert.uv);
	else if (materialInfo.texture > 0.5)
		objColor = texture(diffuseTex2, vert.uv);

	vec3 v = normalize(-vert.position);

    outColor = vec4(objColor.xyz * materialInfo.ambient, objColor.w);
    for (int i = 0; i < LIGHT_COUNT; i++) {
        outColor.xyz += point(objColor.xyz, lightInfo.lights[i], vert.position, vert.normal, v);
    }

	// Gamma correction
	outColor.xyz = pow(outColor.xyz, vec3(1.0 / lightInfo.parameters.x));
}