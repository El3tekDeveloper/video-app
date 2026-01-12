#include <iostream>
#include <thread>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VideoReader.h"
#include "VideoRenderer.h"
#include "UI.h"
#include "UIRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

GLFWwindow* window;

const char* TITLE = "Video App";
int WIDTH = 1920;
int HEIGHT = 1080;

UIRenderer uiRenderer;
UI ui;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "No video file provided\n";
        return -1;
    }

    const char* videoPath = argv[1];

    glfwInit();

    window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    VideoReader video;
    if (!video.Open(videoPath))
    {
        std::cout << "Couldn't open video\n";
        return -1;
    }
    glfwSetWindowTitle(window, videoPath);

    VideoRenderer videoRenderer;

    int frameWidth = video.GetWidth();
    int frameHeight = video.GetHeight();
    unsigned char* frameData = new unsigned char[frameWidth * frameHeight * 4];
    
    double videoDuration = video.GetDuration();
    double currentVideoTime = 0.0;
    double startTime = glfwGetTime();
    
    uiRenderer.init();
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, -1.0f, 1.0f);
    uiRenderer.setProjection(projection);

    bool play = true;
    bool seeking = false;
    float sliderValue = 0.0f;
    
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);
        
        glViewport(0, 0, window_width, window_height);

        projection = glm::ortho(0.0f, (float)window_width, 0.0f, (float)window_height, -1.0f, 1.0f);
        uiRenderer.setProjection(projection);

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        ui.mousePos = glm::vec2(mouseX, window_height - mouseY);
        ui.mouseButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (play && !seeking)
        {
            int64_t pts;
            if (!video.ReadFrame(frameData, &pts))
            {
                std::cout << "End of video or error\n";
                break;
            }

            double presentationTimestamp = pts * (double)video.GetTimeBase().num / (double)video.GetTimeBase().den;
            currentVideoTime = presentationTimestamp;
            
            while (true)
            {
                double currentTime = glfwGetTime();
                double delay = (startTime + presentationTimestamp) - currentTime;

                if (delay <= 0.0)
                    break;

                if (delay > 0.002)
                    std::this_thread::sleep_for(std::chrono::microseconds((int)(delay * 1e6)));
                else
                    std::this_thread::yield();
            }

            videoRenderer.UpdateTexture(frameData, frameWidth, frameHeight);
        }
        
        videoRenderer.Render(window_width, window_height, frameWidth, frameHeight);

        if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS)
            video.Seek(0);

        sliderValue = (videoDuration > 0) ? (currentVideoTime / videoDuration) : 0.0f;
        sliderValue = glm::clamp(sliderValue, 0.0f, 1.0f);

        ui.begin(glm::vec2(window_width / 2.0f, 150));
        
        float timelineWidth = window_width - 40.0f;
        bool sliderChanged = ui.slider(
            &uiRenderer,
            sliderValue,
            glm::vec2(timelineWidth, 10),
            5.0f,
            glm::vec4(0.5, 0.5f, 0.5f, 0.7f),
            glm::vec4(1.0, 0.3f, 0.3f, 0.9f),
            glm::vec4(1.0, 0.3f, 0.3f, 1.0f),
            1
        );
        
        if (sliderChanged)
        {
            seeking = true;
            double targetTime = sliderValue * videoDuration;
            
            if (video.Seek(targetTime))
            {
                currentVideoTime = targetTime;
                startTime = glfwGetTime() - targetTime;
                
                int64_t pts;
                if (video.ReadFrame(frameData, &pts))
                {
                    videoRenderer.UpdateTexture(frameData, frameWidth, frameHeight);
                }
            }
        }
        else if (seeking && !ui.mouseButton)
        {
            seeking = false;
            startTime = glfwGetTime() - currentVideoTime;
        }

        ui.end();

        ui.begin(glm::vec2(window_width / 2, 100));

        glm::vec4 pauseButtonColor = play ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 1, 0, 1);
        auto pauseButton = ui.button(&uiRenderer, pauseButtonColor, glm::vec2(60, 60), 50.0f, 0);
        
        if (pauseButton)
        {
            play = !play;
            if (play)
                startTime = glfwGetTime() - currentVideoTime;
        }

        ui.end();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete[] frameData;
    video.Close();
    uiRenderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
