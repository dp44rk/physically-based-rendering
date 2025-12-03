#include "../include/mesh.h"
#include "../include/shader.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, bool hasTangentSpace)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->hasTangentSpace = hasTangentSpace;
    
    setupMesh();
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
    
    glBindVertexArray(0);
}

void Mesh::Draw(Shader &shader, bool enableTangentSpace)
{
    // 텍스처 타입별 플래그 및 유닛 번호 매핑
    struct TextureBinding {
        bool* hasFlag;
        int textureUnit;
        const char* shaderName;
    };
    
    bool hasAlbedo = false;
    bool hasNormal = false;
    bool hasMetallic = false;
    bool hasRoughness = false;
    bool hasAo = false;
    
    // 한 번의 순회로 텍스처 체크 및 바인딩
    for(unsigned int i = 0; i < textures.size(); i++)
    {
        const std::string& name = textures[i].type;
        int textureUnit = -1;
        
        if(name == "texture_diffuse" || name == "texture_albedo")
        {
            hasAlbedo = true;
            textureUnit = 0;
        }
        else if(name == "texture_normal")
        {
            hasNormal = true;
            textureUnit = 1;
        }
        else if(name == "texture_metallic")
        {
            hasMetallic = true;
            textureUnit = 2;
        }
        else if(name == "texture_roughness")
        {
            hasRoughness = true;
            textureUnit = 3;
        }
        else if(name == "texture_ao")
        {
            hasAo = true;
            textureUnit = 4;
        }
        
        // 텍스처 바인딩
        if (textureUnit >= 0)
        {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
    }
    
    // 셰이더에 텍스처 유닛 번호 설정
    shader.setInt("albedoMap", 0);
    shader.setInt("normalMap", 1);
    shader.setInt("metallicMap", 2);
    shader.setInt("roughnessMap", 3);
    shader.setInt("aoMap", 4);
    
    shader.setBool("hasAlbedoMap", hasAlbedo);
    shader.setBool("hasNormalMap", hasNormal);
    shader.setBool("hasMetallicMap", hasMetallic);
    shader.setBool("hasRoughnessMap", hasRoughness);
    shader.setBool("hasAoMap", hasAo);
    shader.setBool("useTangentSpace", enableTangentSpace && hasTangentSpace);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glActiveTexture(GL_TEXTURE0);
}
