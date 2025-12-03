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
    bool hasAlbedo = false;
    bool hasNormal = false;
    bool hasMetallic = false;
    bool hasRoughness = false;
    bool hasAo = false;
    
    // 고정된 텍스처 유닛 사용: 0=albedo, 1=normal, 2=metallic, 3=roughness, 4=ao
    // 먼저 모든 텍스처를 찾아서 바인딩
    for(unsigned int i = 0; i < textures.size(); i++)
    {
        std::string name = textures[i].type;
        
        if(name == "texture_diffuse" || name == "texture_albedo")
        {
            hasAlbedo = true;
        }
        else if(name == "texture_normal")
        {
            hasNormal = true;
        }
        else if(name == "texture_metallic")
        {
            hasMetallic = true;
        }
        else if(name == "texture_roughness")
        {
            hasRoughness = true;
        }
        else if(name == "texture_ao")
        {
            hasAo = true;
        }
    }
    
    // 텍스처 바인딩 (타입별로)
    for(unsigned int i = 0; i < textures.size(); i++)
    {
        std::string name = textures[i].type;
        
        if(name == "texture_diffuse" || name == "texture_albedo")
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        else if(name == "texture_normal")
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        else if(name == "texture_metallic")
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        else if(name == "texture_roughness")
        {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        else if(name == "texture_ao")
        {
            glActiveTexture(GL_TEXTURE4);
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
