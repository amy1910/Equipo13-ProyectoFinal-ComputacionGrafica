#version 330 core

struct Material
{
    sampler2D diffuse;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform Material material;
uniform vec3 cameraPosition;
uniform float time;
#define WATER_POINT_LIGHTS 14
uniform vec3 pointLightPositions[WATER_POINT_LIGHTS];
uniform vec3 pointLightColors[WATER_POINT_LIGHTS];
uniform vec3 fogColor;
uniform float fogDensity;

void main()
{
    vec3 base = texture(material.diffuse, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(cameraPosition - FragPos);

    float facing = max(dot(normal, viewDirection), 0.0);
    float fresnel = pow(1.0 - facing, 3.0);
    float highlight = 0.5 + 0.5 * sin(TexCoords.x * 18.0 + TexCoords.y * 13.0
        + sin(TexCoords.y * 7.0) * 3.5 + time * 1.4);

    // Deep teal base plus cyan highlights gives the channels a readable
    // sea-water surface even under the dark Day-of-the-Dead lighting.
    vec3 deepWater = vec3(0.008, 0.085, 0.145);
    vec3 shallowWater = vec3(0.018, 0.255, 0.335);
    vec3 waterColor = mix(deepWater, shallowWater, 0.32 + 0.22 * highlight);
    waterColor = mix(waterColor, base, 0.40);
    waterColor += vec3(0.045, 0.16, 0.22) * fresnel;
    waterColor += vec3(0.018, 0.095, 0.13) * highlight;

    // Broad bands sell the large-scale swell while the micro ripples add
    // readable specular streaks between the chinampas.
    float swellA = 0.5 + 0.5 * sin(TexCoords.x * 6.0
        + TexCoords.y * 3.5 + time * 0.75);
    float swellB = 0.5 + 0.5 * cos(TexCoords.x * 4.0
        - TexCoords.y * 7.0 - time * 0.52);
    float swell = smoothstep(0.55, 0.92, 0.52 * swellA + 0.48 * swellB);
    waterColor += vec3(0.018, 0.105, 0.145) * swell;

    float microRipples = 0.5 + 0.5 * sin(TexCoords.x * 53.0
        - TexCoords.y * 37.0 + sin(TexCoords.x * 11.0) * 2.0 + time * 1.8);
    float crest = smoothstep(0.86, 1.0, highlight) * microRipples;
    waterColor += vec3(0.20, 0.52, 0.58) * crest * 0.16;

    // Guarantee a visible marine-blue floor even when the scene lights are
    // nearly black at night; distance fog is applied below afterward.
    waterColor = max(waterColor, vec3(0.008, 0.12, 0.22));

    float lightSpecular = 0.0;
    vec3 lightReflection = vec3(0.0);
    for (int i = 0; i < WATER_POINT_LIGHTS; ++i) {
        vec3 lightDirection = normalize(pointLightPositions[i] - FragPos);
        vec3 reflected = reflect(-lightDirection, normal);
        float specular = pow(max(dot(viewDirection, reflected), 0.0), 72.0);
        float distanceToLight = length(pointLightPositions[i] - FragPos);
        float attenuation = 1.0 / (1.0 + 0.045 * distanceToLight +
            0.0075 * distanceToLight * distanceToLight);
        lightSpecular += specular * attenuation;
        lightReflection += pointLightColors[i] * specular * attenuation;
    }
    waterColor += lightReflection * 0.11;
    waterColor += vec3(0.16, 0.20, 0.30) * lightSpecular * fresnel * 0.035;

    float fogDistance = length(cameraPosition - FragPos);
    float fogFactor = 1.0 - exp(-pow(fogDensity * fogDistance, 2.0));
    // Keep the water horizon chromatically continuous with the skybox.
    const vec3 xochimilcoHorizonFog = vec3(0.055, 0.018, 0.115);
    vec3 atmosphericFog = mix(fogColor, xochimilcoHorizonFog, 0.50);
    waterColor = mix(waterColor, atmosphericFog, clamp(fogFactor, 0.0, 0.92));

    color = vec4(waterColor, 1.0);
}
