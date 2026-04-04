#pragma once

#include <string_view>
#include <vector>
#include <assimp/mesh.h>
#include <assimp/scene.h>

#include "mesh.hpp"
#include "shader.hpp"


class Model {
public:
  std::vector<Mesh> meshes;
  std::vector<Texture> textures;
  std::string directory;

  static Model Load(std::string_view path);

  void Draw(Shader& shader);

private:
  void LoadModel(std::string_view path);
  void ProcessNode(aiNode* node, const aiScene* scene);
  Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
  std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string type_name);
};
