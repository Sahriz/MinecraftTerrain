#version 430 core

layout(location = 0) in uint aPackedData;


uniform mat4 projM;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat3 normalMatrix;
uniform vec2 offset;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

const vec3 normals[] = {
    vec3(1.0,0.0,0.0),
    vec3(-1.0,0.0,0.0),
    vec3(0.0,1.0,0.0),
    vec3(0.0,-1.0,0.0),
    vec3(0.0,0.0,1.0),
    vec3(0.0,0.0,-1.0)
};

vec3 getNormal(){
    vec3 normal = vec3(0.0);
    uint index = (aPackedData >> 19) & 0x7u;
    normal = normals[index];
    return normal;
}

vec3 unpackVertexPosition(){
    vec3 position = vec3(0.0);
    position.x = float(aPackedData & 0x1Fu);
    position.y = float((aPackedData >> 5) & 0x1FFu);
    position.z = float((aPackedData >> 14) & 0x1Fu);

    return position;
}

vec2 unpackUVs(){
    vec2 uvs = vec2(0.0);
    float U = float((aPackedData >> 22) & 0x1u);
    float V = float((aPackedData >> 23) & 0x1u);

    uvs.x = U;
    uvs.y = V;

    return uvs;
}

void main()
{
    vec3 position = unpackVertexPosition();
    vec3 worldPos = position + vec3(offset.x, 0.0, offset.y);
    vec3 normal = getNormal();
    FragPos = vec3(uModel*vec4(worldPos, 1.0f));
    Normal = normalMatrix*normal;
    gl_Position =  projM * uView * vec4(FragPos, 1.0);
    TexCoord = unpackUVs();
}