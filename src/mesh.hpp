#pragma once

#include <functional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "texture.hpp"


struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 tex_coords;
};

enum struct VertexType {
  Points,
  Triangles,
};

class Mesh {
public:
  enum struct Type {
    Points,
    Triangles,
  };

  Type type = Type::Triangles;
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices = {}, std::vector<Texture> textures = {});
  Mesh(Type type, std::vector<Vertex> vertices, std::vector<unsigned int> indices = {}, std::vector<Texture> textures = {});
  void Draw(Shader& shader);
  void DrawInstanced(Shader& shader, unsigned int count);
  void CustomSetup(std::function<void()> func);

private:
  void SetupMesh();
  void BindTextures(Shader& shader);

  unsigned int vao_, vbo_, ebo_;
};
