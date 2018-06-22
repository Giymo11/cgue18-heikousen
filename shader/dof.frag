#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in VertexData {
    vec2 uv;
} vert;

layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform DepthOfField {
    vec2 proj;
    vec2 px;
    float far;
    float focusPoint;
    float focusScale;
    float dofEnable;
} dofInfo;

layout (binding = 1) uniform sampler2D lightImage;
layout (binding = 2) uniform sampler2D depth;

const float GOLDEN_ANGLE  = 2.39996323;
const float MAX_BLUR_SIZE = 20.0;
const float RAD_SCALE     = 0.5; // Smaller = nicer blur, larger = faster

float linearizeDepth(float depth) {
    return dofInfo.proj.y / (depth - dofInfo.proj.x);
}

float getBlurSize(float depth) {
    return smoothstep(0, dofInfo.focusScale, abs(dofInfo.focusPoint - depth)) * MAX_BLUR_SIZE;
}

vec4 depthOfField(vec2 texCoord) {
    float centerDepth = linearizeDepth(texture(depth, texCoord).r);
    float centerSize = getBlurSize(centerDepth);

    vec3 color = texture(lightImage, texCoord).rgb;
    float tot = 1.0;
    float radius = RAD_SCALE;
    float alpha = 0.0f;

    for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE) {
        vec2 tc = texCoord + vec2(cos(ang), sin(ang)) * dofInfo.px * radius;

        vec3 sampleColor = texture(lightImage, tc).rgb;
        float sampleDepth = linearizeDepth(texture(depth, tc).r);
        float sampleSize = getBlurSize(sampleDepth);

        if (sampleDepth > centerDepth)
            sampleSize = clamp(sampleSize, 0.0, centerSize*2.0);

        float m = smoothstep(radius-0.5, radius+0.5, sampleSize);
        color += mix(color/tot, sampleColor, m);
        tot += 1.0;
        radius += RAD_SCALE/radius;
        alpha += sampleSize;
    }

    return vec4(color, alpha) / tot;
}

void main () {
    if (dofInfo.dofEnable < 0.5) {
        outColor = vec4(texture(lightImage, vert.uv).rgb, 1.);
    } else {
        // float centerDepth = linearizeDepth(texture(depth, vert.uv).r);
        // float centerSize = getBlurFactor(centerDepth);
        outColor = vec4(depthOfField(vert.uv).rgb, 1.);
    }
}