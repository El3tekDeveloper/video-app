#pragma once

#include <iostream>
#include <glad/glad.h>

class VideoRenderer
{
private:
	GLuint VAO, VBO, EBO;
    GLuint shaderProgram;
    GLuint videoTexture;

    GLuint CompileShader(const char* source, GLenum type);
    GLuint CreateShaderProgram();

public:
    VideoRenderer();
    ~VideoRenderer();

    void UpdateTexture(unsigned char* data, int width, int height);
    void Render(int windowWidth, int windowHeight, int videoWidth, int videoHeight);
};
