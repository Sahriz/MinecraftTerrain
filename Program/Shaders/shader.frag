#version 430 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in flat uint textureID;

uniform float Width;
uniform float Height;
uniform float Time;
uniform sampler2DArray uTextureArray;

out vec4 FragColor;

void main()
{

    vec3 lightDirection = normalize(vec3(10*cos(Time*0.1),10.0,10*sin(Time*0.1)));
    vec3 norm = normalize(Normal);

    float ambientStrength = 0.3; // Adjust this to make shadows lighter/darker
    vec3 ambientLight = vec3(ambientStrength);

    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuseLight = vec3(diff); // Assuming white sunlight
    

    vec3 totalLight = ambientLight + diffuseLight;

    vec4 texColor = texture(uTextureArray, vec3(TexCoord, float(textureID)));

    // Water (layers 12-14) is forced translucent regardless of its texture alpha.
    // 0.55 is the blend weight; lower is clearer water. Land keeps its texture alpha.
    float alpha = texColor.a;
    if (textureID >= 12u && textureID <= 14u) alpha = 0.55;

    FragColor = vec4(totalLight * texColor.rgb, alpha);

    
}
