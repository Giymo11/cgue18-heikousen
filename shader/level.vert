#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inUv;
layout(location = 3) in vec3 inLightUv;

layout(location = 0) out VertexData {
	vec3 position;
	vec3 normal;
	vec3 uv;
    vec3 lightUv;
} vert;

layout(binding = 0) uniform GlobalTransformations {
	mat4 projection;
	mat4 view;
} globalTrans;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 viewSpace = globalTrans.view * vec4(inPosition, 1.);

	vert.position = viewSpace.xyz;
	vert.normal   = normalize(transpose(inverse(mat3(globalTrans.view))) * inNormal);
	vert.uv       = inUv;
    vert.lightUv  = inLightUv;

	gl_Position = globalTrans.projection * viewSpace;
}