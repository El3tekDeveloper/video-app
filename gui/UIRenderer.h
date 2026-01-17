#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct AABB {
  glm::vec2 position;
  glm::vec2 size;
  float cornerRadius;

  AABB();
  AABB(const glm::vec2& pos, const glm::vec2& sz, float radius = 0.0f);

  bool intersects(const AABB& other) const;
  bool contains(const glm::vec2& point) const;

  glm::vec2 getMin() const;
  glm::vec2 getMax() const;

  glm::vec2 getTopLeft() const;
  glm::vec2 getTopRight() const;
  glm::vec2 getBottomLeft() const;
  glm::vec2 getBottomRight() const;
};

class UIRenderer {
  private:
    unsigned int VAO, VBO, EBO;
    unsigned int shaderProgram;
    unsigned int textureShaderProgram;
    bool initialized;
    glm::mat4 projection;

    void compileShader();
    void compileTextureShader();
    void setupBuffers();

  public:
    UIRenderer();
    ~UIRenderer();

    void init();
    void setProjection(const glm::mat4& proj);
    
    void renderAABB(const AABB& aabb, const glm::vec4 color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    void renderFilledAABB(const AABB& aabb, const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    void renderTexturedAABB(const AABB& aabb, unsigned int textureID, const glm::vec4& tintColor = glm::vec4(1.0f), const glm::vec4& colorMult = glm::vec4(1.0f));
    
    static unsigned int loadTexture(const char* filepath);
    static void deleteTexture(unsigned int textureID);
    
    void cleanup();
};
