#include "../include/model.h"
#include <iostream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

Model::Model(std::string const &path, bool gamma) : gammaCorrection(gamma)
{
    loadModel(path);
}

void Model::Draw(Shader &shader, bool enableTangentSpace)
{
    for(unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader, enableTangentSpace);
}

void Model::loadModel(std::string const &path)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_PreTransformVertices // 플랫하게 변환해 계층 오프셋 제거
    );
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        directory = path.substr(0, lastSlash);
    else
        directory = ".";
    
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Tangent-space normal mapping requires UVs and tangents/bitangents.
    // Track availability so we can disable normal maps when data is missing.
    bool hasTexCoords = mesh->mTextureCoords[0] != nullptr;
    bool hasTangentSpace = hasTexCoords && mesh->mTangents && mesh->mBitangents;
    
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector;
        
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        
        if (mesh->mNormals)
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        
        if(mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        
        if(hasTangentSpace)
        {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
        }
        else
        {
            vertex.Tangent = glm::vec3(0.0f);
        }
        
        if(hasTangentSpace)
        {
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
        {
            vertex.Bitangent = glm::vec3(0.0f);
        }
        
        vertices.push_back(vertex);
    }
    
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    
    // PBR 텍스처 맵 로드
    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_albedo");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    
    std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    if(!hasTangentSpace && !normalMaps.empty())
    {
        std::cout << "Normal map ignored for mesh (no UVs/tangents in model)" << std::endl;
        normalMaps.clear();
    }
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    
    // Metallic과 Roughness는 일반적으로 다른 타입으로 저장될 수 있음
    // aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS 등
    std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic");
    if(metallicMaps.empty())
    {
        // Metallic 맵이 없으면 specular를 사용
        metallicMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_metallic");
    }
    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
    
    std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness");
    if(roughnessMaps.empty())
    {
        // Roughness 맵이 없으면 shininess를 사용 (roughness = 1.0 - shininess/256.0)
        // 또는 다른 텍스처 타입에서 찾기
        roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS, "texture_roughness");
    }
    textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
    
    std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_ao");
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    
    // 텍스처가 하나도 없으면 Pbr 디렉토리에서 기본 텍스처 로드 시도
    if (textures.empty())
    {
        static bool defaultTexturesLoaded = false;
        static Texture defaultBaseColor, defaultNormal, defaultMetallic, defaultRoughness;
        
        if (!defaultTexturesLoaded)
        {
            std::cout << "No textures found in material, loading default PBR textures..." << std::endl;
            
            // BaseColor 텍스처 로드
            defaultBaseColor.id = TextureFromFile("mjolnir3_lp_GreyMetal_BaseColor.png", "Pbr");
            if (defaultBaseColor.id == 0)
                defaultBaseColor.id = TextureFromFile("mjolnir3_lp_GreyMetal_BaseColor.png", "../Pbr");
            if (defaultBaseColor.id != 0)
            {
                defaultBaseColor.type = "texture_albedo";
                defaultBaseColor.path = "Pbr/mjolnir3_lp_GreyMetal_BaseColor.png";
                std::cout << "  ✓ Loaded BaseColor texture (ID: " << defaultBaseColor.id << ")" << std::endl;
            }
            
            // Normal 텍스처 로드
            defaultNormal.id = TextureFromFile("mjolnir3_lp_GreyMetal_Normal.png", "Pbr");
            if (defaultNormal.id == 0)
                defaultNormal.id = TextureFromFile("mjolnir3_lp_GreyMetal_Normal.png", "../Pbr");
            if (defaultNormal.id != 0)
            {
                defaultNormal.type = "texture_normal";
                defaultNormal.path = "Pbr/mjolnir3_lp_GreyMetal_Normal.png";
                std::cout << "  ✓ Loaded Normal texture (ID: " << defaultNormal.id << ")" << std::endl;
            }
            
            // Metallic 텍스처 로드
            defaultMetallic.id = TextureFromFile("mjolnir3_lp_GreyMetal_Metallic.png", "Pbr");
            if (defaultMetallic.id == 0)
                defaultMetallic.id = TextureFromFile("mjolnir3_lp_GreyMetal_Metallic.png", "../Pbr");
            if (defaultMetallic.id != 0)
            {
                defaultMetallic.type = "texture_metallic";
                defaultMetallic.path = "Pbr/mjolnir3_lp_GreyMetal_Metallic.png";
                std::cout << "  ✓ Loaded Metallic texture (ID: " << defaultMetallic.id << ")" << std::endl;
            }
            
            // Roughness 텍스처 로드
            defaultRoughness.id = TextureFromFile("mjolnir3_lp_GreyMetal_Roughness.png", "Pbr");
            if (defaultRoughness.id == 0)
                defaultRoughness.id = TextureFromFile("mjolnir3_lp_GreyMetal_Roughness.png", "../Pbr");
            if (defaultRoughness.id != 0)
            {
                defaultRoughness.type = "texture_roughness";
                defaultRoughness.path = "Pbr/mjolnir3_lp_GreyMetal_Roughness.png";
                std::cout << "  ✓ Loaded Roughness texture (ID: " << defaultRoughness.id << ")" << std::endl;
            }
            
            defaultTexturesLoaded = true;
        }
        
        // 기본 텍스처를 메시에 추가
        if (defaultBaseColor.id != 0) textures.push_back(defaultBaseColor);
        if (defaultNormal.id != 0) textures.push_back(defaultNormal);
        if (defaultMetallic.id != 0) textures.push_back(defaultMetallic);
        if (defaultRoughness.id != 0) textures.push_back(defaultRoughness);
    }
    
    return Mesh(vertices, indices, textures, hasTangentSpace);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for(unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if(!skip)
        {
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory, bool gamma)
{
    std::string filename = std::string(path);
    
    // 먼저 원본 경로로 시도
    std::string fullPath = filename;
    if (!directory.empty() && directory != ".")
    {
        fullPath = directory + '/' + filename;
    }
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
    
    // 디버그: 첫 번째 시도 경로 출력
    if (!data)
    {
        std::cout << "  Trying to load texture: " << fullPath;
        if (!directory.empty())
            std::cout << " (from directory: " << directory << ")";
        std::cout << std::endl;
    }
    
    // 원본 경로에서 찾지 못하면 Pbr 디렉토리에서 시도
    if (!data)
    {
        // 파일명만 추출 (경로 제거)
        size_t lastSlash = filename.find_last_of("/\\");
        std::string textureName = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
        
        // 파일 확장자 제거
        size_t dotPos = textureName.find_last_of(".");
        std::string baseName = (dotPos != std::string::npos) ? textureName.substr(0, dotPos) : textureName;
        
        // Pbr 디렉토리에서 여러 가능한 파일명으로 시도
        std::vector<std::string> possibleNames = {
            "Pbr/" + textureName,  // 원본 이름
            "Pbr/mjolnir3_lp_GreyMetal_BaseColor.png",  // BaseColor
            "Pbr/mjolnir3_lp_GreyMetal_Normal.png",     // Normal
            "Pbr/mjolnir3_lp_GreyMetal_Metallic.png",  // Metallic
            "Pbr/mjolnir3_lp_GreyMetal_Roughness.png", // Roughness
            "Pbr/mjolnir3_lp_GreyMetal_Height.png"     // Height (AO로 사용 가능)
        };
        
        // BaseColor, Diffuse 관련이면 BaseColor.png 시도
        if (baseName.find("diffuse") != std::string::npos || 
            baseName.find("albedo") != std::string::npos ||
            baseName.find("base") != std::string::npos ||
            baseName.find("color") != std::string::npos)
        {
            std::string paths[] = {"Pbr/mjolnir3_lp_GreyMetal_BaseColor.png", "../Pbr/mjolnir3_lp_GreyMetal_BaseColor.png"};
            for (const auto& p : paths)
            {
                data = stbi_load(p.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded BaseColor texture from: " << p << std::endl;
                    break;
                }
            }
        }
        // Normal 관련이면 Normal.png 시도
        else if (baseName.find("normal") != std::string::npos)
        {
            std::string paths[] = {"Pbr/mjolnir3_lp_GreyMetal_Normal.png", "../Pbr/mjolnir3_lp_GreyMetal_Normal.png"};
            for (const auto& p : paths)
            {
                data = stbi_load(p.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded Normal texture from: " << p << std::endl;
                    break;
                }
            }
        }
        // Metallic 관련이면 Metallic.png 시도
        else if (baseName.find("metallic") != std::string::npos)
        {
            std::string paths[] = {"Pbr/mjolnir3_lp_GreyMetal_Metallic.png", "../Pbr/mjolnir3_lp_GreyMetal_Metallic.png"};
            for (const auto& p : paths)
            {
                data = stbi_load(p.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded Metallic texture from: " << p << std::endl;
                    break;
                }
            }
        }
        // Roughness 관련이면 Roughness.png 시도
        else if (baseName.find("roughness") != std::string::npos || 
                 baseName.find("rough") != std::string::npos)
        {
            std::string paths[] = {"Pbr/mjolnir3_lp_GreyMetal_Roughness.png", "../Pbr/mjolnir3_lp_GreyMetal_Roughness.png"};
            for (const auto& p : paths)
            {
                data = stbi_load(p.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded Roughness texture from: " << p << std::endl;
                    break;
                }
            }
        }
        // AO, Height 관련이면 Height.png 시도
        else if (baseName.find("ao") != std::string::npos || 
                 baseName.find("height") != std::string::npos ||
                 baseName.find("occlusion") != std::string::npos)
        {
            std::string paths[] = {"Pbr/mjolnir3_lp_GreyMetal_Height.png", "../Pbr/mjolnir3_lp_GreyMetal_Height.png"};
            for (const auto& p : paths)
            {
                data = stbi_load(p.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded AO/Height texture from: " << p << std::endl;
                    break;
                }
            }
        }
        
        // 위에서 찾지 못했으면 모든 가능한 경로 시도
        if (!data)
        {
            for (const auto& path : possibleNames)
            {
                data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded texture from: " << path << std::endl;
                    break;
                }
                // build 디렉토리에서 실행하는 경우를 대비해 ../ 추가
                std::string parentPath = "../" + path;
                data = stbi_load(parentPath.c_str(), &width, &height, &nrComponents, 0);
                if (data)
                {
                    std::cout << "  ✓ Loaded texture from: " << parentPath << std::endl;
                    break;
                }
            }
        }
    }
    
    if (data)
    {
        // 텍스처가 성공적으로 로드되었을 때만 OpenGL 텍스처 생성
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        return textureID;
    }
    else
    {
        std::cout << "  Texture failed to load at path: " << fullPath;
        if (!directory.empty() && directory != ".")
            std::cout << " (directory: " << directory << ")";
        std::cout << std::endl;
        return 0; // 실패 시 0 반환
    }
}
