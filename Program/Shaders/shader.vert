#version 430 core
#extension GL_ARB_shader_draw_parameters : require

layout(location = 0) in uint aPackedData;


uniform mat4 projM;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat3 normalMatrix;

// One world-space offset per pool slot (not per draw). Each multidraw command sets
// baseInstance to its chunk's slot; we index this with gl_BaseInstanceARB below.
// A slot is stable while the chunk stays resident, so this is written once at load,
// not per frame.
layout(std430, binding = 0) readonly buffer ChunkOffsets {
    vec2 chunkOffsets[];
};

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out flat uint textureID;

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
    uvs.y = 1.0-V;

    return uvs;
}

uint unpackTexID(){
    uint id =  (aPackedData >> 24) & 0xFFu;
    return (id * 3u) - 3;
   
}

void main()
{
    vec3 position = unpackVertexPosition();
    vec2 offset = chunkOffsets[gl_BaseInstanceARB];
    vec3 worldPos = position + vec3(offset.x, 0.0, offset.y);
    vec3 normal = getNormal();
    
    uint base = unpackTexID();
    
    // Offset water surface (Block ID 5 -> Texture ID 12)
    if (base == 12u) {
        float rawV = float((aPackedData >> 23) & 0x1u);
        if (normal.y > 0.1) {
            worldPos.y -= 0.1;
        } else if (abs(normal.y) < 0.1 && rawV < 0.5) {
            worldPos.y -= 0.1;
        }
    }

    FragPos = vec3(uModel*vec4(worldPos, 1.0f));
    Normal = normalMatrix*normal;
    gl_Position =  projM * uView * vec4(FragPos, 1.0);
    TexCoord = unpackUVs();
    textureID = base;
    if (normal.y > 0.1)  textureID = base;      // Top
    if (abs(normal.y) < 0.1) textureID = base + 1; // Side
    if (normal.y < -0.1) textureID = base + 2; // Bottom
}