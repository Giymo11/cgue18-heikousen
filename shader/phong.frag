#version 450
#extension GL_ARB_separate_shader_objects : enable

const int LIGHT_COUNT = 1;

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

/* layout(binding = 2) uniform */ struct MaterialInfo {
	float ambient;
	float diffuse;
	float specular;
	float alpha;
} materialInfo;

/* layout(binding = 3) uniform LightBlock {
    LightSource lights[LIGHT_COUNT];
};*/

LightSource lights[LIGHT_COUNT];

layout(binding = 4) uniform sampler2D diffuseTex;

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
	// vec4 objColor = texture(diffuseTex, vert.uv);

	/* TEMPORARY START */

	materialInfo.ambient = 0.1;
	materialInfo.diffuse = 0.9;
	materialInfo.specular = 0.3;
	materialInfo.alpha = 10.0;

	vec4 objColor = vec4(0.0, 1.0, 0.0, 1.0);

	lights[0].position = vec3(0.0, 0.0, -5.0);
	lights[0].color = vec3(1.0, 1.0, 1.0);
	lights[0].attenuation = vec3(1.0f, 0.4f, 0.1f);

	/* TEMPORARY END */

	vec3 v = normalize(-vert.position);

    outColor = vec4(objColor.xyz * materialInfo.ambient, objColor.w);
    for (int i = 0; i < LIGHT_COUNT; i++) {
        outColor.xyz += point(objColor.xyz, lights[i], vert.position, vert.normal, v);
    }

	//outColor = vec4(normalize(vert.normal), 1.0);
}