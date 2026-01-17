#ifndef UI_H
#define UI_H

#include <optional>
#include <cassert>
#include <glm/glm.hpp>

#include "UIRenderer.h"

using namespace glm;

struct UI
{
  struct Layout
  {
    enum Kind
    {
      HORIZONTAL = 0,
      VERTICAL
    };

    Kind kind;
    vec2 pos;
    vec2 size;
    float padding;

    vec2 availablePos() const;
    void pushWidget(vec2 widgetSize);
  };

  static const size_t LAYOUTS_CAPACITY = 1024;
  size_t layoutCount = 0;
  Layout layouts[LAYOUTS_CAPACITY];

  vec2 mousePos;
  bool mouseButton = false;

  std::optional<size_t> hot;
  std::optional<size_t> active;

  void pushLayout(Layout layout);
  Layout popLayout();
  Layout* topLayout();

  void begin(vec2 pos, float padding = 10);
  void beginLayout(Layout::Kind kind);

  bool button(UIRenderer* renderer, vec4 color, vec2 size, float cornerRadius, size_t id);
  bool textureButton(UIRenderer* renderer, unsigned int textureID, vec2 size, size_t id, 
                     const vec4& color = vec4(1.0f), const vec4& tintColor = vec4(1.0f));
  bool slider(UIRenderer* renderer, float& value, vec2 size, float cornerRadius, 
      vec4 trackColor, vec4 fillColor, vec4 scrubberColor, size_t id);

  void endLayout();
  void end();
};

#endif
