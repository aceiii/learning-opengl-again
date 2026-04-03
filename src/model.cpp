#include <stb_image.h>
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "image.hpp"
#include "model.hpp"
#include "logger.hpp"


static unsigned int TextureFromFile(std::string_view path, const std::string& directory, bool gamma = false) {
  std::string filename = directory + '/' + std::string{path};

  unsigned int texture_id;
  glGenTextures(1, &texture_id);

  Image image = Image::Load(filename, 0);

  if (!image.data) {
    auto logger = Logger::GetRootLogger();
    quill::warning(logger, "Texture failed to load at path: '{}'", filename);
    return texture_id;
  }

  GLenum format;
  switch (image.num_components) {
    case 1: format = GL_RED; break;
    case 3: format = GL_RGB; break;
    case 4: format = GL_RGBA; break;
    default: {
      auto logger = Logger::GetRootLogger();
      quill::warning(logger, "Unknown texture format: {}", image.num_components);
    }
  }

  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.data.get());
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  return texture_id;
}

Model::Model(std::string_view path) {
}

void Model::Draw(Shader& shader) {
  for (auto& mesh : meshes_) {
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

  directory_ = path.substr(0, path.find_last_of("/"));
  ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene) {
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    meshes_.push_back(ProcessMesh(mesh, scene));
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

    if (mesh->mTextureCoords[0]) {
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
  std::vector<Texture> textures;

  for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
    aiString str;
    mat->GetTexture(type, i, &str);

    bool skip = false;

    for (const auto& existing_texture : textures_) {
      if (std::strcmp(existing_texture.path.data(), str.C_Str()) == 0) {
        textures.push_back(existing_texture);
        skip = true;
        break;
      }
    }

    if (!skip) {
      Texture texture{
        .id = TextureFromFile(str.C_Str(), directory_),
        .type = type_name,
        .path = str.C_Str(),
      };

      textures.push_back(texture);
      textures_.push_back(texture);
    }
  }

  return textures;
}

