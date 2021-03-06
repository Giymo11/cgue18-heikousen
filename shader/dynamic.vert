#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout(location = 0) out VertexData {
	vec3 position;
	vec3 normal;
	vec2 uv;
	float linearDepth;
} vert;

layout(binding = 0) uniform GlobalTransformations {
	mat4 projection;
	mat4 view;
} globalTrans;

layout(binding = 1) uniform ModelTransformations {
	mat4 model;
	mat4 normalMatrix;
} modelTrans;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 worldSpace = modelTrans.model * vec4(inPosition, 1.);
	vec4 viewPos    = globalTrans.view * worldSpace;

	vert.position    = worldSpace.xyz;
	vert.normal      = normalize(mat3(modelTrans.normalMatrix) * inNormal);
	vert.uv          = inUv;
	vert.linearDepth = -viewPos.z;

	gl_Position = globalTrans.projection * viewPos;
}