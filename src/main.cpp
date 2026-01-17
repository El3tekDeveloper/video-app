#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VideoReader.h"
#include "AudioReader.h"
#include "VideoRenderer.h"
#include "UI.h"
#include "UIRenderer.h"
#include "glm/fwd.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

GLFWwindow* window;

const char* TITLE = "Video App";
int WIDTH = 1920;
int HEIGHT = 1080;

UIRenderer uiRenderer;
UI ui;

bool any_key_pressed = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    any_key_pressed |= (action == GLFW_PRESS);
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
  glfwSetKeyCallback(window, key_callback);

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
  
  AudioReader audio;
  bool hasAudio = audio.Open(videoPath);
  if (!hasAudio)
  {
    std::cout << "Warning: No audio stream found or couldn't open audio\n";
  }
  
  glfwSetWindowTitle(window, videoPath);

  VideoRenderer videoRenderer;

  int frameWidth = video.GetWidth();
  int frameHeight = video.GetHeight();
  unsigned char* frameData = new unsigned char[frameWidth * frameHeight * 4];

  double videoDuration = video.GetDuration();
  double currentVideoTime = 0.0;
  
  int64_t pts;
  if (video.ReadFrame(frameData, &pts))
  {
    videoRenderer.UpdateTexture(frameData, frameWidth, frameHeight);
    currentVideoTime = pts * (double)video.GetTimeBase().num / (double)video.GetTimeBase().den;
  }
  
  if (hasAudio)
  {
    if (!audio.PrefillBuffer())
    {
      std::cout << "Warning: Failed to prefill audio buffer\n";
    }
  }
  
  double startTime = glfwGetTime();

  uiRenderer.init();
  glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT, -1.0f, 1.0f);
  uiRenderer.setProjection(projection);

  auto smoothAnimation = [](float current, float target, float speed) -> float {
    float diff = target - current;
    return current + diff * speed;
  };

  bool play = true;
  bool mute = false;
  bool seeking = false;
  bool loopEnabled = false;
  bool isFullscreen = false;

  float sliderValue = 0.0f;
  float volumeValue = 1.0f;
  bool uiVisible = true;

  double lastMouseMoveTime = glfwGetTime();
  glm::vec2 lastMousePos = glm::vec2(0, 0);

  float uiSlideOffset = 0.0f;
  float targetSlideOffset = 0.0f;

  bool wasSpacePressed = false;
  
  unsigned int playIcon = uiRenderer.loadTexture("assets/play.png");
  unsigned int pauseIcon = uiRenderer.loadTexture("assets/pause.png");
  unsigned int audioIcon = uiRenderer.loadTexture("assets/volume.png");
  unsigned int muteIcon = uiRenderer.loadTexture("assets/volume-mute.png");
  unsigned int fullscreenIcon = uiRenderer.loadTexture("assets/fullscreen.png");
  unsigned int unfullscreenIcon = uiRenderer.loadTexture("assets/exit-fullscreen.png");
  unsigned int loopIcon = uiRenderer.loadTexture("assets/repeat.png");

  bool sliderJustReleased = false;
  bool volumeSliderJustReleased = false;
  bool volumeSeeking = false;
  double seekTargetTime = 0.0;
  float seekTargetVolume = 1.0f;
  
  std::thread audioThread;
  bool audioThreadRunning = true;
  
  if (hasAudio)
  {
    audioThread = std::thread([&]() {
      while (audioThreadRunning)
      {
        if (play && !seeking && hasAudio)
        {
          audio.FillBuffer();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
    });
    
    audio.Play();
  }

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
    
    if (glm::distance(ui.mousePos, lastMousePos) > 1.0f || any_key_pressed)
    {
      lastMousePos = ui.mousePos;
      lastMouseMoveTime = glfwGetTime();
      if (!uiVisible)
      {
        uiVisible = true;
      }
      targetSlideOffset = 0.0f;

      any_key_pressed = false;
    }
    
    double timeSinceMove = glfwGetTime() - lastMouseMoveTime;
    if (uiVisible && timeSinceMove > 2.0)
    {
      targetSlideOffset = -150.0f;
      if (uiSlideOffset <= -149.0f)
      {
        uiVisible = false;
      }
    }
    
    uiSlideOffset = smoothAnimation(uiSlideOffset, targetSlideOffset, 0.15f);

    if (play && !seeking)
    {
      int64_t pts;
      if (video.ReadFrame(frameData, &pts))
      {
        double presentationTimestamp = pts * (double)video.GetTimeBase().num / (double)video.GetTimeBase().den;
        currentVideoTime = presentationTimestamp;
        
        if (hasAudio && audio.IsPlaying())
        {
          double audioTime = audio.GetCurrentTime();
          double audioLatency = audio.GetAudioLatency();
          double effectiveAudioTime = audioTime - audioLatency;
          double audioDrift = currentVideoTime - effectiveAudioTime;
          
          if (std::abs(audioDrift) > 0.040)
          {
            if (audioDrift > 0)
            {
              std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
          }
        }

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
      else
      {
        if (loopEnabled)
        {
          seekTargetTime = 0.0;
          if (video.Seek(0.0))
          {
            int64_t pts;
            if (video.ReadFrame(frameData, &pts))
            {
              double actualTime = pts * (double)video.GetTimeBase().num / (double)video.GetTimeBase().den;
              currentVideoTime = actualTime;
              videoRenderer.UpdateTexture(frameData, frameWidth, frameHeight);
              startTime = glfwGetTime() - actualTime;
            }
          }
          
          if (hasAudio)
          {
            audio.Seek(0.0);
            audio.PrefillBuffer();
            audio.Play();
          }
        }
        else
        {
          play = false;
          if (hasAudio)
            audio.Pause();
        }
      }
    }
    else if (seeking)
    {
      currentVideoTime = seekTargetTime;
    }

    videoRenderer.Render(window_width, window_height, frameWidth, frameHeight);

    if (uiSlideOffset > -150.0f)
    {
      float containerWidth = 800.0f;
      float containerHeight = 100.0f;
      float containerX = window_width / 2.0f;
      float containerY = containerHeight / 2.0f + 20.0f + uiSlideOffset;
      
      AABB mainContainer(glm::vec2(containerX, containerY), 
                         glm::vec2(containerWidth, containerHeight), 12.0f);
      uiRenderer.renderFilledAABB(mainContainer, glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));

      if (!seeking)
      {
        sliderValue = (videoDuration > 0) ? (currentVideoTime / videoDuration) : 0.0f;
        sliderValue = glm::clamp(sliderValue, 0.0f, 1.0f);
      }

      float timelineWidth = containerWidth - 80.0f;
      float timelineY = containerY + 20.0f;
      
      ui.begin(glm::vec2(containerX, timelineY));

      bool sliderChanged = ui.slider(
          &uiRenderer,
          sliderValue,
          glm::vec2(timelineWidth, 8),
          10.0f,
          glm::vec4(0.2, 0.2f, 0.2f, 0.8f),
          glm::vec4(1.0, 0.3f, 0.3f, 1.0f),
          glm::vec4(1.0, 1.0f, 1.0f, 1.0f),
          1
      );
      
      if (glfwGetKey(window, GLFW_KEY_HOME) == GLFW_PRESS)
      {
        sliderValue = 0.0f;
        sliderChanged = true;

        if (!play)
          play = true;
      }
      if (glfwGetKey(window, GLFW_KEY_END) == GLFW_PRESS)
      {
        sliderValue = 1.0f;
        sliderChanged = true;
      }

      if (sliderChanged)
      {
        if (!seeking)
        {
          seeking = true;
          if (hasAudio)
            audio.Pause();
        }
        seekTargetTime = sliderValue * videoDuration;
        sliderJustReleased = false;
      }
      else if (seeking && !ui.mouseButton && !sliderJustReleased)
      {
        sliderJustReleased = true;
        
        seekTargetTime = glm::clamp(seekTargetTime, 0.0, videoDuration - 0.033);
        
        if (video.Seek(seekTargetTime))
        {
          int64_t pts;
          if (video.ReadFrame(frameData, &pts))
          {
            double actualTime = pts * (double)video.GetTimeBase().num / (double)video.GetTimeBase().den;
            currentVideoTime = actualTime;
            videoRenderer.UpdateTexture(frameData, frameWidth, frameHeight);
            
            startTime = glfwGetTime() - actualTime;
          }
        }
        
        if (hasAudio)
        {
          audio.Seek(seekTargetTime);
          audio.PrefillBuffer();
          if (play)
            audio.Play();
        }
        
        seeking = false;
      }

      ui.end();

      float buttonY = containerY - 20.0f;
      
      ui.begin(glm::vec2(containerX, buttonY));
      {
        unsigned int pauseButtonTexture = play ? pauseIcon : playIcon;
        auto pauseButton = ui.textureButton(&uiRenderer, pauseButtonTexture, glm::vec2(60, 60), 0);

        bool isSpacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (pauseButton || (isSpacePressed && !wasSpacePressed))
        {
          play = !play;
          if (play)
          {
            startTime = glfwGetTime() - currentVideoTime;
            if (hasAudio)
              audio.Play();
          }
          else
          {
            if (hasAudio)
              audio.Pause();
          }
        }
        wasSpacePressed = isSpacePressed;
      }
      ui.end();

      ui.begin(glm::vec2(mainContainer.getTopLeft().x + 60, buttonY));
      {
        unsigned int audioButtonIcon = mute ? muteIcon : audioIcon;
        auto audioButton = ui.textureButton(&uiRenderer, audioButtonIcon, glm::vec2(32, 32), 2);

        if (audioButton)
        {
          mute = !mute;
          if (mute) {
            ma_device_set_master_volume(&audio.GetDevice(), 0);
          } else {
            if (volumeValue == 0.0f)
              volumeValue = 0.5f;
           ma_device_set_master_volume(&audio.GetDevice(), volumeValue);
          }
        }
      }
      ui.end();

      ui.begin(glm::vec2((mainContainer.getTopLeft().x + 60) + 100, buttonY));
      {
        if (!volumeSeeking)
        {
          volumeValue = glm::clamp(volumeValue, 0.0f, 1.0f);
        }

        auto volumeSliderChanged = ui.slider(
          &uiRenderer,
          volumeValue,
          glm::vec2(150, 6.5f),
          10.0f,
          glm::vec4(0.2, 0.2f, 0.2f, 0.8f),
          glm::vec4(1.0, 1.0f, 1.0f, 1.0f),
          glm::vec4(1.0, 1.0f, 1.0f, 1.0f),
          3
        );
        
        if (volumeSliderChanged) {
          if (!volumeSeeking)
          {
            volumeSeeking = true;
          }
          seekTargetVolume = volumeValue;
          volumeSliderJustReleased = false;
        }
        else if (volumeSeeking && !ui.mouseButton && !volumeSliderJustReleased)
        {
          volumeSliderJustReleased = true;
          
          seekTargetVolume = glm::clamp(seekTargetVolume, 0.0f, 1.0f);
          volumeValue = seekTargetVolume;
          
          ma_device_set_master_volume(&audio.GetDevice(), volumeValue);
          
          if (volumeValue == 0.0f) 
            mute = true;
          else
            mute = false;
          
          volumeSeeking = false;
        }
      }
      ui.end();

      ui.begin(glm::vec2(mainContainer.getTopRight().x - 110, buttonY));
      {
        vec4 loopColor = loopEnabled ? vec4(0.3f, 0.8f, 1.0f, 1.0f) : vec4(1.0f, 1.0f, 1.0f, 1.0f);
        auto loopButton = ui.textureButton(&uiRenderer, loopIcon, glm::vec2(32, 32), 5, loopColor);
        
        if (loopButton)
        {
          loopEnabled = !loopEnabled;
        }
      }
      ui.end();

      ui.begin(glm::vec2(mainContainer.getTopRight().x - 60, buttonY));
      {
        unsigned int fullscreenButtonIcon = isFullscreen ? unfullscreenIcon : fullscreenIcon;
        auto fullscreenButton = ui.textureButton(&uiRenderer, fullscreenButtonIcon, glm::vec2(32, 32), 4);
        
        if (fullscreenButton)
        {
          isFullscreen = !isFullscreen;
          GLFWmonitor* monitor = glfwGetPrimaryMonitor();
          const GLFWvidmode* mode = glfwGetVideoMode(monitor);
          
          if (isFullscreen)
          {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
          }
          else
          {
            glfwSetWindowMonitor(window, NULL, 100, 100, WIDTH, HEIGHT, 0);
          }
        }
      }
      ui.end();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  audioThreadRunning = false;
  if (hasAudio && audioThread.joinable())
    audioThread.join();

  delete[] frameData;
  video.Close();
  if (hasAudio)
    audio.Close();
  
  uiRenderer.cleanup();
  uiRenderer.deleteTexture(pauseIcon);
  uiRenderer.deleteTexture(playIcon);
  uiRenderer.deleteTexture(audioIcon);
  uiRenderer.deleteTexture(muteIcon);
  uiRenderer.deleteTexture(fullscreenIcon);
  uiRenderer.deleteTexture(unfullscreenIcon);
  uiRenderer.deleteTexture(loopIcon);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
