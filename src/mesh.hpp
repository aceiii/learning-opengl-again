#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "shader.hpp"


struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 tex_coords;
};

struct Texture {
  unsigned int id;
  std::string type;
  std::string path;
};

class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
  void Draw(Shader& shader);

private:
  void SetupMesh();

  unsigned int vao_, vbo_, ebo_;
};
