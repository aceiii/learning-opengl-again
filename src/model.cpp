#include <stb_image.h>
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "image.hpp"
#include "model.hpp"
#include "logger.hpp"


Model Model::Load(std::string_view path) {
  Model model{};
  model.LoadModel(path);
  return model;
}

void Model::Draw(Shader& shader) {
  for (auto& mesh : meshes) {
    mesh.Draw(shader);
  }
}

void Model::LoadModel(std::string_view path) {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(std::string{path}, aiProcess_Triangulate | aiProcess_FlipUVs);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    auto logger = Logger::GetRootLogger();
    quill::error(logger, "[MODEL] Assimp import error: {}", importer.GetErrorString());
    return;
  }

  directory = path.substr(0, path.find_last_of("/"));
  ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene) {
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(ProcessMesh(mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    ProcessNode(node->mChildren[i], scene);
  }
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    const auto& pos = mesh->mVertices[i];
    const auto& norm = mesh->mNormals[i];

    Vertex vertex{
      .position = glm::vec3(pos.x, pos.y, pos.z),
      .normal = glm::vec3(norm.x, norm.y, norm.z),
    };

    if (mesh->HasTextureCoords(0)) {
      const auto& tex = mesh->mTextureCoords[0][i];
      vertex.tex_coords = glm::vec2(tex.x, tex.y);
    }

    vertices.push_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    const aiFace& face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  if (mesh->mMaterialIndex >= 0) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    std::vector<Texture> diffuse_maps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    std::vector<Texture> specular_maps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");

    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());
    textures.insert(textures.end(), specular_maps.begin(), specular_maps.end());
  }

  return Mesh{vertices, indices, textures};
}

std::vector<Texture> Model::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string type_name) {
  std::vector<Texture> new_textures;

  auto logger = Logger::GetRootLogger();

  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
    aiString str;
    mat->GetTexture(type, i, &str);

    std::string path = directory + '/' + str.C_Str();
    bool skip = false;

    for (const auto& existing_texture : textures) {
      quill::info(logger, "Checking existing texture: {} -> {}", existing_texture.path, str.C_Str());
      if (existing_texture.path == path) {
        new_textures.push_back(existing_texture);
        skip = true;
        break;
      }
    }

    if (!skip) {
      Texture texture = Texture::Load(type_name, path);
      new_textures.push_back(texture);
      textures.push_back(texture);
    }
  }

  return new_textures;
}

