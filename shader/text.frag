#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VertexData {
    vec2 pos;
	vec2 uv;
    vec2 st;
} vert;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D msdf;

float sdf(vec2 uv) {
    vec3 s = texture(msdf, uv).rgb;
    return 0.5 - max(min(s.r, s.g), min(max(s.r, s.g), s.b));
}

void main() {
    vec4 fgColor = vec4(1.);
    vec4 bgColor = vec4(0.);

    float dist = sdf(vert.st);
    vec2 grad_dist = normalize(vec2(dFdx(dist), dFdy(dist)));

    vec2 Jdx = dFdx(vert.uv);
    vec2 Jdy = dFdy(vert.uv);
    vec2 grad = vec2 (
        grad_dist.x * Jdx.x + grad_dist.y * Jdy.y,
        grad_dist.x * Jdx.y + grad_dist.y * Jdy.y
    );

    float afwidth = 0.7071 * length(grad);
    float opacity = smoothstep(afwidth, -afwidth, dist);

    // float dist = sdf(vert.st);
    // float w = fwidth(dist);
    // float opacity = smoothstep(w, -w, dist);

    outColor = mix(bgColor, fgColor, opacity);
}
