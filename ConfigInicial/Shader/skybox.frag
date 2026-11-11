#version 330 core

in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube skybox;
uniform float time;

void main()
{
    vec3 direction = normalize(TexCoords);
    vec3 color = texture(skybox, direction).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float upperSky = smoothstep(0.08, 0.32, direction.y);
    float starMask = smoothstep(0.50, 0.92, luminance) * upperSky;
    float twinkle = 1.0 + 0.035 * sin(time * 2.2 + direction.x * 29.0
        + direction.z * 37.0);
    color *= 1.0 + starMask * (twinkle - 1.0);

    float horizon = exp(-pow((direction.y + 0.015) / 0.18, 2.0));
    float hazePulse = 0.012 * (0.5 + 0.5 * sin(time * 0.55));
    color += vec3(1.0, 0.10, 0.045) * horizon * hazePulse;
    FragColor = vec4(color, 1.0);
}
