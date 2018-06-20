#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const int LIGHT_COUNT = 16;

struct LightSource {
    vec3 position;
    vec3 color;
    vec3 attenuation;
};

layout(location = 0) in VertexData {
    vec2 uv;
} vert;

layout (location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform LightBlock {
    vec4 parameters;
    vec4 playerPos;
    LightSource lights[LIGHT_COUNT];
} lightInfo;

layout (binding = 1) uniform sampler2D position;
layout (binding = 2) uniform sampler2D normal;
layout (binding = 3) uniform sampler2D albedo;
layout (binding = 4) uniform sampler2D material;

vec3 phong (
    vec3 objColor,
    vec3 intensity,
    vec3 l,
    vec3 n,
    vec3 v,
    vec4 material
) {
    vec3 diffuse = objColor
        * intensity
        * max(dot(n,l), 0.0)
        * material.y;

    vec3 reflection = normalize(-reflect(l, n));
    float rv = max(dot(reflection, v), 0.0);
    vec3 specular = intensity
        * material.z
        * pow(rv, material.w);

    return diffuse + specular;
}

vec3 point (
    vec3 objColor,
    LightSource light,
    vec3 p,
    vec3 n,
    vec3 v,
    vec4 material
) {
    float d = distance(p, light.position);
    float attenuation = light.attenuation.x
        + d * light.attenuation.y
        + d * d * light.attenuation.z;

    return phong (
        objColor, light.color,
        normalize(light.position - p),
        n, v, material
    ) * (1 / attenuation);
}

void main() {
    vec4 pos = texture(position, vert.uv);
    vec4 nml = texture(normal, vert.uv);
    vec4 col = texture(albedo, vert.uv);
    vec4 mat = texture(material, vert.uv);

    /* Ambient */
    vec3 color = col.rgb * mat.x;

    /* Viewer to fragment */
    vec3 v = pos.xyz - lightInfo.playerPos.xyz;
    v = normalize(v);

    int lightNum = int (lightInfo.parameters.w);
    for (int i = 0; i < lightNum; ++i) {
        color += point (
            col.rgb, lightInfo.lights[i],
            pos.xyz, nml.xyz, v, mat
        );
    }

    outColor = vec4(color, 1.0);  
}