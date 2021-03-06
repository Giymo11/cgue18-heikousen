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

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec4 outMaterial;

layout(binding = 1) uniform sampler2DArray diffuseTex1;
layout(binding = 2) uniform sampler2DArray lightmap;

layout(binding = 3) uniform DepthOfField {
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

float coc(float depth) {
    return smoothstep(0, dofInfo.focalWidth, abs(dofInfo.focalDistance - depth));
}

void main() {
	vec4 objColor = texture(diffuseTex1, vert.uv);
	vec3 lightmap = texture(lightmap, vec3(vert.lightUv.xy, 1.0)).rgb;

    outPosition  = vec4(vert.position, 1.);
    outNormal.xy = normalize(vert.normal).xy;
    outNormal.z  = coc(vert.linearDepth);
    outNormal.w  = vert.linearDepth / 100.0f;
    outColor     = vec4(objColor.rgb * lightmap, 1.);
    outMaterial  = vec4(1.4, 0., 0., 1.) / 50.;
}