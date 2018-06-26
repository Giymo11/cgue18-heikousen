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
layout (binding = 3) uniform sampler2D coc;

// Formula for precalculating texture taps
//
// def gen_taps(num):
//     RAD_SCALE = 0.5
//     angle     = 0.0
//     radius    = RAD_SCALE
//    
//     for i in range(num):
//         x = math.cos(angle) * 1.0/1280.0 * radius
//         y = math.sin(angle) * 1.0/720.0  * radius
//         z = radius - 0.5
//         w = float(i + 1)
//         out = 'vec4({:2.8f}, {:2.8f}, {:2.8f}, {:2.8f}),'.format(x,y,z,w)
//         print(out)
//         radius += RAD_SCALE/radius
//         angle  += 2.39996323


const int TAP_COUNT = 25;

const vec4[TAP_COUNT] taps = vec4[TAP_COUNT](
    vec4(0.00039063, 0.00000000, 0.00000000, 1.00000000),
    vec4(-0.00086410, 0.00140727, 1.00000000, 2.00000000),
    vec4(0.00012522, -0.00253655, 1.33333333, 3.00000000),
    vec4(0.00100110, 0.00232135, 1.60606061, 4.00000000),
    vec4(-0.00180285, -0.00056693, 1.84347068, 5.00000000),
    vec4(0.00168542, -0.00190600, 2.05682944, 6.00000000),
    vec4(-0.00055823, 0.00369169, 2.25238413, 7.00000000),
    vec4(-0.00105650, -0.00361641, 2.43404482, 8.00000000),
    vec4(0.00227819, 0.00147910, 2.60445803, 9.00000000),
    vec4(-0.00235818, 0.00173053, 2.76551674, 10.00000000),
    vec4(0.00113201, -0.00430052, 2.91863187, 11.00000000),
    vec4(0.00083353, 0.00472429, 3.06488921, 12.00000000),
    vec4(-0.00250448, -0.00258026, 3.20514602, 13.00000000),
    vec4(0.00293010, -0.00114520, 3.34009347, 14.00000000),
    vec4(-0.00178393, 0.00451104, 3.47029863, 15.00000000),
    vec4(-0.00041126, -0.00564204, 3.59623374, 16.00000000),
    vec4(0.00251993, 0.00377565, 3.71829709, 17.00000000),
    vec4(-0.00338525, 0.00024887, 3.83682833, 18.00000000),
    vec4(0.00246546, -0.00436172, 3.95211996, 19.00000000),
    vec4(-0.00016472, 0.00633271, 4.06442601, 20.00000000),
    vec4(-0.00233957, -0.00498416, 4.17396881, 21.00000000),
    vec4(0.00370176, 0.00088545, 4.28094428, 22.00000000),
    vec4(-0.00313307, 0.00387539, 4.38552613, 23.00000000),
    vec4(0.00085527, -0.00675868, 4.48786925, 24.00000000),
    vec4(0.00197634, 0.00613151, 4.58811245, 25.00000000)
);

const float MAX_BLUR_RADIUS = taps[TAP_COUNT - 1].z + 0.5;

vec3 depthOfField(vec2 texCoord) {
    vec4  color        = texture(lightImage, texCoord);
    float centerDepth  = texture(depth, texCoord).r;
    float centerSize   = color.a * MAX_BLUR_RADIUS;
    float doubleCenter = centerSize * 2.0;

    for (int i = 0; i < TAP_COUNT; i++) {
        vec4 tap = taps[i];

        vec4 sampleColor  = texture(lightImage, texCoord + tap.xy);
        float sampleDepth = texture(depth, texCoord + tap.xy).r;
        float sampleSize  = sampleColor.a * MAX_BLUR_RADIUS;

        sampleSize = max(sampleSize, mix(sampleSize, doubleCenter, max(sign(centerDepth - sampleDepth), 0.0)));

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