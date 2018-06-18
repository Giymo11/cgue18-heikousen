#version 450
#extension GL_ARB_separate_shader_objects : enable

const int LIGHT_COUNT = 16;

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

layout(binding = 4) uniform sampler2DArray diffuseTex1;

vec3 phong(vec3 objColor, vec3 intensity, vec3 l, vec3 n, vec3 v) {
    vec3 diffuse = objColor * intensity * max(dot(n,l), 0.0) * materialInfo.diffuse;

    vec3 reflection = normalize(-reflect(l, n));
    float rv = max(dot(reflection, v), 0.0);
    vec3 specular = intensity * materialInfo.specular * pow(rv, materialInfo.alpha);

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

vec3 tonemaping_reinhard(vec3 hdrColor) {
    return hdrColor / (hdrColor + vec3(1.0));
}

vec3 gamma_adjust(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}


void main() {
	vec4 objColor = texture(diffuseTex1, vec3(vert.uv, materialInfo.texture));

	vec3 v = normalize(-vert.position);

    outColor = vec4(objColor.xyz * materialInfo.ambient, objColor.w);

	int lightNum = int(lightInfo.parameters.w);
    for (int i = 0; i < lightNum; i++) {
        outColor.xyz += point(objColor.xyz, lightInfo.lights[i], vert.position, vert.normal, v);
    }

    float gamma = lightInfo.parameters.x;
    float hdrMode = lightInfo.parameters.y;
    float hdrExposure = lightInfo.parameters.z;

    if(hdrMode > 1.5) {
        outColor.xyz = tonemaping_exposure(outColor.xyz, hdrExposure);
    } else if(hdrMode > 0.5) {
        outColor.xyz = tonemaping_reinhard(outColor.xyz);
    }

	// Gamma correction
	outColor.xyz = gamma_adjust(outColor.xyz, gamma);
}