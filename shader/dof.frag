#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in VertexData {
    vec2 uv;
} vert;

layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform DepthOfField {
    float dofEnable;
    float focalDistance;
    float focalWidth;
    int tapCount;

    vec4 taps[50];
} dofInfo;

layout (binding = 1) uniform sampler2D lightImage;
layout (binding = 2) uniform sampler2D normals;

vec3 depthOfField(vec2 texCoord) {
    const int   TAP_COUNT       = dofInfo.tapCount;
    const float MAX_BLUR_RADIUS = dofInfo.taps[TAP_COUNT - 1].z + 0.5;

    vec4  color         = texture(lightImage, texCoord);
	vec2  center        = texture(normals, texCoord).wz;
    float centerDepth   = center.x;
    float centerSize    = center.y * MAX_BLUR_RADIUS;
    float doubleCenter  = centerSize * 2.0;

    for (int i = 0; i < TAP_COUNT; i++) {
        vec4 tap = dofInfo.taps[i];

        vec4 sampleColor  = texture(lightImage, texCoord + tap.xy);
		vec2 smp          = texture(normals, texCoord + tap.xy).wz;
        float sampleDepth = smp.x;
        float sampleSize  = smp.y * MAX_BLUR_RADIUS;

        if (sampleDepth > centerDepth)
            sampleSize = max(sampleSize, doubleCenter);

        float m    = smoothstep(tap.z, tap.z + 1.0, sampleSize);
        color.rgb += mix(color.rgb / tap.w, sampleColor.rgb, m);
    }

    return color.rgb / float(TAP_COUNT);
}

void main () {
    if (dofInfo.dofEnable < 0.5) {
        outColor = vec4(texture(lightImage, vert.uv).rgb, 1.);
    } else {
        outColor = vec4(depthOfField(vert.uv), 1.);
    }
}