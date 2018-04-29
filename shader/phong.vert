#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUv;

layout(binding = 0) uniform GlobalTransforms {
	uniform mat3 normalMatrix;
} globalTrans;

layout(binding = 1) uniform ModelTransformations {
	mat4 mvp;
} modelTrans;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	outPosition = modelTrans.mvp * vec4(inPosition, 1.);
	outNormal = globalTrans.normalMatrix * inNormal;
	outUv = inUv;
	gl_Position = outPosition;
}