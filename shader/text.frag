#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VertexData {
    vec2 pos;
	vec2 uv;
} vert;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D msdf;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec4 fgColor = vec4(1.);
    vec4 bgColor = vec4(0.);

    vec2 msdfUnit = 4.0 / vec2(64.0);

    vec3 samp = texture(msdf, vert.uv).rgb;
    // float sigDist = median(samp.r, samp.g, samp.b) - 0.5;
    // sigDist *= dot(msdfUnit, 0.5 / fwidth(vert.pos));
    // float opacity = clamp(sigDist + 0.5, 0.0, 1.0);

    float sigDist = median( samp.r, samp.g, samp.b );
    float w = fwidth( sigDist );
    float opacity = smoothstep( 0.5 - w, 0.5 + w, sigDist );

    outColor = mix(bgColor, fgColor, opacity);
}
