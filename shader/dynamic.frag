#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in VertexData {
    vec3 position;
    vec3 normal;
    vec2 uv;
	float linearDepth;
} vert;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec4 outMaterial;

layout(binding = 2) uniform MaterialInfo {
    float ambient;
    float diffuse;
    float specular;
    float alpha;

    float texture;
    float normal;
    float param2;
    float param3;
} materialInfo;

layout(binding = 3) uniform sampler2DArray tex;

layout(binding = 4) uniform DepthOfField {
    float dofEnable;
    float focalDistance;
    float focalWidth;
    int tapCount;

    int param0;
    int param1;
    int param2;
    int param3;

    vec4 taps[50];
} dofInfo;

// circle of confusion
float coc(float depth) {
    return smoothstep(0, dofInfo.focalWidth, abs(dofInfo.focalDistance - depth));
}

void main() {
    vec4 objColor = texture(tex, vec3(vert.uv, materialInfo.texture));
    vec3 normalsFromTexture = texture(tex, vec3(vert.uv, materialInfo.normal)).rgb;

    if(dofInfo.param0 > 1.5) {
        outNormal.xy = normalize((normalsFromTexture * 2.0 - 1.0) * vert.normal).xy;
    } else if(dofInfo.param0 > 0.5) {
        outNormal.xy = normalize(normalsFromTexture * 2.0 - 1.0).xy;
    } else {
        outNormal.xy = normalize(vert.normal).xy;
    }

	if (objColor.a < 0.99)
		discard;

    outPosition = vec4(vert.position, 1.);
    outNormal.z = coc(vert.linearDepth);
    outNormal.w = vert.linearDepth / 100.0f;
    outColor    = vec4(objColor.rgb, 1.0);
    outMaterial = vec4 (
        materialInfo.ambient,
        materialInfo.diffuse,
        materialInfo.specular,
        materialInfo.alpha
    ) / 50.;
}