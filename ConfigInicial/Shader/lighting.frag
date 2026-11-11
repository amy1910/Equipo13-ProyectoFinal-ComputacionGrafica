#version 330 core

#define NUMBER_OF_POINT_LIGHTS 1

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NUMBER_OF_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform int transparency;
uniform int isWater;

// Parametros del material agua: corresponden a Noise Texture -> Bump -> Normal
// y al Principled BSDF configurado para agua.obj.
uniform float waterTime;
uniform vec3 waterBaseColor;
uniform float waterTransmission;
uniform float waterIor;
uniform float waterRoughness;
uniform float waterNoiseScale;
uniform float waterNoiseDetail;
uniform float waterBumpStrength;
uniform float waterBumpDistance;

// Function prototypes
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir );
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir );
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir );
float WaterNoise(vec2 p);
float WaterFbm(vec2 p);
vec3 WaterNormal(vec2 p);

// Ruido de valor suavizado y fBm de cinco octavas: el equivalente procedimental
// del nodo Noise Texture (Scale 30, Detail 5) sin requerir una textura externa.
float WaterHash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float WaterNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 local = fract(p);
    local = local * local * (3.0 - 2.0 * local);

    return mix(mix(WaterHash(cell), WaterHash(cell + vec2(1.0, 0.0)), local.x),
               mix(WaterHash(cell + vec2(0.0, 1.0)), WaterHash(cell + vec2(1.0, 1.0)), local.x),
               local.y);
}

float WaterFbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    int octaves = max(1, min(8, int(waterNoiseDetail)));
    for (int octave = 0; octave < octaves; ++octave) {
        value += amplitude * WaterNoise(p);
        p = p * 2.03 + vec2(19.1, 7.7);
        amplitude *= 0.5;
    }
    return value;
}

vec3 WaterNormal(vec2 p)
{
    float epsilon = 0.025;
    float center = WaterFbm(p);
    float slopeX = (WaterFbm(p + vec2(epsilon, 0.0)) - center) / epsilon;
    float slopeZ = (WaterFbm(p + vec2(0.0, epsilon)) - center) / epsilon;
    float bump = waterBumpStrength * waterBumpDistance;
    return normalize(vec3(-slopeX * bump, 1.0, -slopeZ * bump));
}

void main( )
{
    if (isWater == 1)
    {
        vec2 noisePosition = FragPos.xz * waterNoiseScale +
            vec2(waterTime * 0.55, waterTime * 0.32);
        vec3 waterNormal = WaterNormal(noisePosition);
        vec3 waterViewDir = normalize(viewPos - FragPos);
        vec3 waterLightDir = normalize(-dirLight.direction);
        vec3 halfVector = normalize(waterLightDir + waterViewDir);

        // Fresnel-Schlick con IOR real del agua (1.333) y reflejo nitido
        // por Roughness 0.05. En el renderer OpenGL, el alpha aproxima la
        // transmision porque no usa el motor de refraccion de Eevee.
        float f0 = pow((waterIor - 1.0) / (waterIor + 1.0), 2.0);
        float fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(waterNormal, waterViewDir), 0.0), 5.0);
        float specular = pow(max(dot(waterNormal, halfVector), 0.0),
            mix(16.0, 512.0, 1.0 - waterRoughness));
        float diffuse = max(dot(waterNormal, waterLightDir), 0.0);
        vec3 reflectedLight = dirLight.specular * (fresnel + specular) +
            dirLight.diffuse * diffuse * 0.10;
        vec3 waterColor = waterBaseColor * (dirLight.ambient + 0.20) + reflectedLight;
        float alpha = mix(0.30, 0.60, fresnel) * waterTransmission;
        color = vec4(waterColor, alpha);
        return;
    }

    // Properties
    vec3 norm = normalize( Normal );
    vec3 viewDir = normalize( viewPos - FragPos );
    
    // Directional lighting
    vec3 result = CalcDirLight( dirLight, norm, viewDir );
    
    // Point lights
    for ( int i = 0; i < NUMBER_OF_POINT_LIGHTS; i++ )
    {
        result += CalcPointLight( pointLights[i], norm, FragPos, viewDir );
    }
    
    // Spot light
    result += CalcSpotLight( spotLight, norm, FragPos, viewDir );
 	
    color = vec4( result,texture(material.diffuse, TexCoords).rgb );
	  if(color.a < 0.1 && transparency==1)
        discard;

}

// Calculates the color when using a directional light.
vec3 CalcDirLight( DirLight light, vec3 normal, vec3 viewDir )
{
    vec3 lightDir = normalize( -light.direction );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );
    
    // Combine results
    vec3 ambient = light.ambient * vec3( texture( material.diffuse, TexCoords ) );
    vec3 diffuse = light.diffuse * diff * vec3( texture( material.diffuse, TexCoords ) );
    vec3 specular = light.specular * spec * vec3( texture( material.specular, TexCoords ) );
    
    return ( ambient + diffuse + specular );
}

// Calculates the color when using a point light.
vec3 CalcPointLight( PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir )
{
    vec3 lightDir = normalize( light.position - fragPos );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );
    
    // Attenuation
    float distance = length( light.position - fragPos );
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );
    
    // Combine results
    vec3 ambient = light.ambient * vec3( texture( material.diffuse, TexCoords ) );
    vec3 diffuse = light.diffuse * diff * vec3( texture( material.diffuse, TexCoords ) );
    vec3 specular = light.specular * spec * vec3( texture( material.specular, TexCoords ) );
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ( ambient + diffuse + specular );
}

// Calculates the color when using a spot light.
vec3 CalcSpotLight( SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir )
{
    vec3 lightDir = normalize( light.position - fragPos );
    
    // Diffuse shading
    float diff = max( dot( normal, lightDir ), 0.0 );
    
    // Specular shading
    vec3 reflectDir = reflect( -lightDir, normal );
    float spec = pow( max( dot( viewDir, reflectDir ), 0.0 ), material.shininess );
    
    // Attenuation
    float distance = length( light.position - fragPos );
    float attenuation = 1.0f / ( light.constant + light.linear * distance + light.quadratic * ( distance * distance ) );
    
    // Spotlight intensity
    float theta = dot( lightDir, normalize( -light.direction ) );
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp( ( theta - light.outerCutOff ) / epsilon, 0.0, 1.0 );
    
    // Combine results
    vec3 ambient = light.ambient * vec3( texture( material.diffuse, TexCoords ) );
    vec3 diffuse = light.diffuse * diff * vec3( texture( material.diffuse, TexCoords ) );
    vec3 specular = light.specular * spec * vec3( texture( material.specular, TexCoords ) );
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return ( ambient + diffuse + specular );
}
