#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout(location = 0) out VertexData {
	vec3 position;
	vec3 normal;
	vec2 uv;
} vert;

layout(binding = 0) uniform GlobalTransformations {
	mat4 projection;
	mat4 view;
} globalTrans;

layout(binding = 1) uniform ModelTransformations {
	mat4 model;
	mat3 normalMatrix;
} modelTrans;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 viewSpace = globalTrans.view * modelTrans.model * vec4(inPosition, 1.);

	vert.position = viewSpace.xyz;
	vert.normal = normalize(modelTrans.normalMatrix * inNormal);
	vert.uv = inUv;

	gl_Position = globalTrans.projection * viewSpace;
}