#include "UIRenderer.h"

#include <iostream>

#include "stb/stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AABB::AABB() 
  : position(0.0f, 0.0f), size(1.0f, 1.0f), cornerRadius(0.0f) {}

AABB::AABB(const glm::vec2& pos, const glm::vec2& sz, float radius)
  : position(pos), size(sz), cornerRadius(radius) {}

bool AABB::intersects(const AABB& other) const {
  glm::vec2 min1 = getMin();
  glm::vec2 max1 = getMax();
  glm::vec2 min2 = other.getMin();
  glm::vec2 max2 = other.getMax();

  return (min1.x < max2.x && max1.x > min2.x &&
      min1.y < max2.y && max1.y > min2.y);
}

bool AABB::contains(const glm::vec2& point) const {
  glm::vec2 min = getMin();
  glm::vec2 max = getMax();

  return (point.x >= min.x && point.x <= max.x &&
      point.y >= min.y && point.y <= max.y);
}

glm::vec2 AABB::getMin() const {
  return position - size * 0.5f;
}

glm::vec2 AABB::getMax() const {
  return position + size * 0.5f;
}

glm::vec2 AABB::getTopLeft() const {
  return glm::vec2(position.x - size.x * 0.5f, position.y + size.y * 0.5f);
}

glm::vec2 AABB::getTopRight() const {
  return glm::vec2(position.x + size.x * 0.5f, position.y + size.y * 0.5f);
}

glm::vec2 AABB::getBottomLeft() const {
  return glm::vec2(position.x - size.x * 0.5f, position.y - size.y * 0.5f);
}

glm::vec2 AABB::getBottomRight() const {
  return glm::vec2(position.x + size.x * 0.5f, position.y - size.y * 0.5f);
}

UIRenderer::UIRenderer() : VAO(0), VBO(0), EBO(0), shaderProgram(0), textureShaderProgram(0), initialized(false), projection(glm::mat4(1.0f)) {}

UIRenderer::~UIRenderer() {
  cleanup();
}

void UIRenderer::compileShader() {
  const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;

        uniform mat4 model;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
        }
    )";

  const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        uniform vec4 color;

        void main() {
            FragColor = color;
        }
    )";

  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

void UIRenderer::compileTextureShader() {
  const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        uniform mat4 model;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

  const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoord;

        uniform sampler2D textureSampler;
        uniform vec4 tintColor;
        uniform vec4 colorMult;

        void main() {
            vec4 texColor = texture(textureSampler, TexCoord);
            FragColor = texColor * tintColor * colorMult;
        }
    )";

  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cerr << "ERROR::TEXTURE_SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cerr << "ERROR::TEXTURE_SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  textureShaderProgram = glCreateProgram();
  glAttachShader(textureShaderProgram, vertexShader);
  glAttachShader(textureShaderProgram, fragmentShader);
  glLinkProgram(textureShaderProgram);

  glGetProgramiv(textureShaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(textureShaderProgram, 512, NULL, infoLog);
    std::cerr << "ERROR::TEXTURE_SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

void UIRenderer::setupBuffers() {
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

void UIRenderer::init() {
  if (initialized) return;

  compileShader();
  compileTextureShader();
  setupBuffers();
  initialized = true;
}

void UIRenderer::setProjection(const glm::mat4& proj) {
  projection = proj;
}

std::vector<glm::vec2> generateRoundedRectVertices(float width, float height, float radius, int segments) {
  std::vector<glm::vec2> vertices;

  float maxRadius = std::min(width, height) * 0.5f;
  radius = std::min(radius, maxRadius);

  float halfW = width * 0.5f;
  float halfH = height * 0.5f;

  for (int i = 0; i <= segments; i++) {
    float angle = (float)i / segments * M_PI * 0.5f;
    float x = (halfW - radius) + radius * cos(angle);
    float y = (halfH - radius) + radius * sin(angle);
    vertices.push_back(glm::vec2(x, y));
  }

  for (int i = 0; i <= segments; i++) {
    float angle = M_PI * 0.5f + (float)i / segments * M_PI * 0.5f;
    float x = (-halfW + radius) + radius * cos(angle);
    float y = (halfH - radius) + radius * sin(angle);
    vertices.push_back(glm::vec2(x, y));
  }

  for (int i = 0; i <= segments; i++) {
    float angle = M_PI + (float)i / segments * M_PI * 0.5f;
    float x = (-halfW + radius) + radius * cos(angle);
    float y = (-halfH + radius) + radius * sin(angle);
    vertices.push_back(glm::vec2(x, y));
  }

  for (int i = 0; i <= segments; i++) {
    float angle = M_PI * 1.5f + (float)i / segments * M_PI * 0.5f;
    float x = (halfW - radius) + radius * cos(angle);
    float y = (-halfH + radius) + radius * sin(angle);
    vertices.push_back(glm::vec2(x, y));
  }

  return vertices;
}

void UIRenderer::renderAABB(const AABB& aabb, const glm::vec4 color) {
  if (!initialized) {
    std::cerr << "UIRenderer not initialized!" << std::endl;
    return;
  }

  glUseProgram(shaderProgram);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(aabb.position, 0.0f));

  int modelLoc = glGetUniformLocation(shaderProgram, "model");
  int projLoc = glGetUniformLocation(shaderProgram, "projection");
  int colorLoc = glGetUniformLocation(shaderProgram, "color");

  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
  glUniform4fv(colorLoc, 1, &color[0]);

  if (aabb.cornerRadius > 0.0f) {
    int segments = 32;
    std::vector<glm::vec2> vertices = generateRoundedRectVertices(aabb.size.x, aabb.size.y, aabb.cornerRadius, segments);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINE_LOOP, 0, vertices.size());
    glBindVertexArray(0);
  } else {
    float vertices[] = {
      -aabb.size.x * 0.5f, -aabb.size.y * 0.5f,
      aabb.size.x * 0.5f, -aabb.size.y * 0.5f,
      aabb.size.x * 0.5f,  aabb.size.y * 0.5f,
      -aabb.size.x * 0.5f,  aabb.size.y * 0.5f
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glBindVertexArray(0);
  }
}

void UIRenderer::renderFilledAABB(const AABB& aabb, const glm::vec4& color) {
  if (!initialized) {
    std::cerr << "UIRenderer not initialized!" << std::endl;
    return;
  }

  glUseProgram(shaderProgram);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(aabb.position, 0.0f));

  int modelLoc = glGetUniformLocation(shaderProgram, "model");
  int projLoc = glGetUniformLocation(shaderProgram, "projection");
  int colorLoc = glGetUniformLocation(shaderProgram, "color");

  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
  glUniform4fv(colorLoc, 1, &color[0]);

  if (aabb.cornerRadius > 0.0f) {
    int segments = 32;
    std::vector<glm::vec2> vertices = generateRoundedRectVertices(aabb.size.x, aabb.size.y, aabb.cornerRadius, segments);

    std::vector<unsigned int> indices;

    vertices.insert(vertices.begin(), glm::vec2(0.0f, 0.0f));

    for (size_t i = 1; i < vertices.size(); i++) {
      indices.push_back(0);
      indices.push_back(i);
      indices.push_back((i % (vertices.size() - 1)) + 1);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  } else {
    float vertices[] = {
      -aabb.size.x * 0.5f, -aabb.size.y * 0.5f,
      aabb.size.x * 0.5f, -aabb.size.y * 0.5f,
      aabb.size.x * 0.5f,  aabb.size.y * 0.5f,
      -aabb.size.x * 0.5f,  aabb.size.y * 0.5f
    };

    unsigned int indices[] = {
      0, 1, 2,
      2, 3, 0
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
}

void UIRenderer::renderTexturedAABB(const AABB& aabb, unsigned int textureID, const glm::vec4& tintColor, const glm::vec4& colorMult) {
  if (!initialized) {
    std::cerr << "UIRenderer not initialized!" << std::endl;
    return;
  }

  glUseProgram(textureShaderProgram);

  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(aabb.position, 0.0f));

  int modelLoc = glGetUniformLocation(textureShaderProgram, "model");
  int projLoc = glGetUniformLocation(textureShaderProgram, "projection");
  int tintLoc = glGetUniformLocation(textureShaderProgram, "tintColor");
  int colorMultLoc = glGetUniformLocation(textureShaderProgram, "colorMult");

  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
  glUniform4fv(tintLoc, 1, &tintColor[0]);
  glUniform4fv(colorMultLoc, 1, &colorMult[0]);

  float vertices[] = {
    -aabb.size.x * 0.5f, -aabb.size.y * 0.5f,  0.0f, 1.0f,
     aabb.size.x * 0.5f, -aabb.size.y * 0.5f,  1.0f, 1.0f,
     aabb.size.x * 0.5f,  aabb.size.y * 0.5f,  1.0f, 0.0f,
    -aabb.size.x * 0.5f,  aabb.size.y * 0.5f,  0.0f, 0.0f
  };

  unsigned int indices[] = {
    0, 1, 2,
    2, 3, 0
  };

  unsigned int texVAO, texVBO, texEBO;
  glGenVertexArrays(1, &texVAO);
  glGenBuffers(1, &texVBO);
  glGenBuffers(1, &texEBO);

  glBindVertexArray(texVAO);

  glBindBuffer(GL_ARRAY_BUFFER, texVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glUniform1i(glGetUniformLocation(textureShaderProgram, "textureSampler"), 0);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glDeleteVertexArrays(1, &texVAO);
  glDeleteBuffers(1, &texVBO);
  glDeleteBuffers(1, &texEBO);
}

unsigned int UIRenderer::loadTexture(const char* filepath) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);

  if (data) {
    GLenum format = GL_RGB;
    if (nrChannels == 1)
      format = GL_RED;
    else if (nrChannels == 3)
      format = GL_RGB;
    else if (nrChannels == 4)
      format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);
  } else {
    std::cerr << "Failed to load texture: " << filepath << std::endl;
    stbi_image_free(data);
    glDeleteTextures(1, &textureID);
    return 0;
  }

  return textureID;
}

void UIRenderer::deleteTexture(unsigned int textureID) {
  if (textureID != 0) {
    glDeleteTextures(1, &textureID);
  }
}

void UIRenderer::cleanup() {
  if (initialized) {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(textureShaderProgram);
    initialized = false;
  }
}
