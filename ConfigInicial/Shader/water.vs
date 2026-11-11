#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main()
{
    // Only the upper surface receives the animated displacement. The sides
    // remain stable, preserving the silhouette of the channel mesh.
    float surfaceMask = smoothstep(0.25, 0.85, normal.y);
    float phaseA = time * 1.35 + position.x * 2.20 + position.z * 1.55;
    float phaseB = time * 0.85 - position.x * 3.10 + position.z * 2.45;
    float wave = (0.060 * sin(phaseA) + 0.030 * cos(phaseB)) * surfaceMask;
    float lateralWave = (0.012 * cos(phaseA) + 0.008 * sin(phaseB)) * surfaceMask;

    vec3 displaced = position;
    displaced.y += wave;
    displaced.x += lateralWave;

    float dWaveDx = (0.060 * 2.20 * cos(phaseA) + 0.030 * 3.10 * sin(phaseB)) * surfaceMask;
    float dWaveDz = (0.060 * 1.55 * cos(phaseA) - 0.030 * 2.45 * sin(phaseB)) * surfaceMask;
    vec3 localSurfaceNormal = normalize(vec3(-dWaveDx, 1.0, -dWaveDz));
    vec3 localNormal = normalize(mix(normal, localSurfaceNormal, surfaceMask));

    vec4 worldPosition = model * vec4(displaced, 1.0);
    FragPos = worldPosition.xyz;
    Normal = mat3(transpose(inverse(model))) * localNormal;

    // Scrolling UVs produce a second motion cue independent of geometry.
    TexCoords = texCoords + vec2(time * 0.018 + sin(phaseA) * 0.008,
        time * 0.012 + cos(phaseB) * 0.006);
    gl_Position = projection * view * worldPosition;
}
