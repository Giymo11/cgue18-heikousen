#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in VertexData {
    vec3 position;
    vec3 normal;
    vec2 uv;
} vert;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;

layout(binding = 2) uniform MaterialInfo {
    float ambient;
    float diffuse;
    float specular;
    float alpha;

    float texture;
    float normal;
    float param2;
    float param3;
} materialInfo;

layout(binding = 3) uniform sampler2DArray tex;

void main() {
    vec4 objColor = texture(tex, vec3(vert.uv, materialInfo.texture));

    // Hack for alpha blending right now
    if (objColor.a < 0.99)
        discard;

    outPosition = vec4(vert.position, materialInfo.specular);
    outNormal   = vec4(vert.normal, materialInfo.alpha);
    outColor    = vec4(objColor.rgb, materialInfo.diffuse);
}