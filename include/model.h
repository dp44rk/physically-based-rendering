#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include "mesh.h"

class Shader;

class Model
{
public:
    Model(std::string const &path, bool gamma = false);
    void Draw(Shader& shader, bool enableTangentSpace);
    
private:
    std::vector<Mesh> meshes;
    std::vector<Texture> textures_loaded;
    std::string directory;
    bool gammaCorrection;
    
    void loadModel(std::string const &path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);
    
    // 텍스처 로딩 헬퍼 함수
    unsigned char* tryLoadTextureFromPaths(const std::vector<std::string>& paths, int& width, int& height, int& nrComponents);
    std::vector<std::string> getDefaultTexturePaths(const std::string& baseName, const std::string& textureName);
    unsigned int createGLTexture(unsigned char* data, int width, int height, int nrComponents);
};

#endif
