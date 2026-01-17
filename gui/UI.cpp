#include "UI.h"

#include <iostream>

using ID = size_t;

vec2 UI::Layout::availablePos() const
{
  switch (kind)
  {
  case Kind::HORIZONTAL:
    return pos + (size + padding) * vec2(1.0f, 0.0f);
    break;
  
  case Kind::VERTICAL:
    return pos + (size + padding) * vec2(0.0f, 1.0f);
    break;
  
  default:
    std::cout << "the kind not defined: " << __FILE__ << std::endl;
    break;
  }
  
  return pos;
}

void UI::Layout::pushWidget(vec2 widgetSize)
{
  switch (kind)
  {
  case Kind::HORIZONTAL:
    size.x = size.x + widgetSize.x + padding;
    size.y = std::max(size.y, widgetSize.y);
    break;
  
  case Kind::VERTICAL:
    size.x = std::max(size.x, widgetSize.x);
    size.y = size.y + widgetSize.y + padding;
    break;
    
  default:
    std::cout << "the kind not defined: " << __FILE__ << std::endl;
    break;
  }
}

void UI::pushLayout(Layout layout)
{
  assert(layoutCount < LAYOUTS_CAPACITY);
  layouts[layoutCount++] = layout;
}

UI::Layout UI::popLayout()
{
  assert(layoutCount > 0);
  return layouts[--layoutCount];
}

UI::Layout* UI::topLayout()
{
  if (layoutCount > 0)
  {
    return &layouts[layoutCount - 1];
  }

  return NULL;
}

void UI::begin(vec2 pos, float padding)
{
  Layout layout = {};
  layout.kind = Layout::Kind::HORIZONTAL;
  layout.pos = pos;
  layout.size = {};
  layout.padding = padding;
  pushLayout(layout);
}

void UI::beginLayout(Layout::Kind kind)
{
  
}

bool UI::button(UIRenderer* renderer, vec4 color, vec2 size, float cornerRadius, ID id)
{
  auto layout = topLayout();
  assert(layout != NULL);

  const auto pos  = layout->availablePos();
  const auto rect = AABB(pos, size, cornerRadius);

  bool hovered = rect.contains(mousePos);
  bool clicked = false;

  if (hovered)
    hot = id;
  else if (hot == id)
    hot.reset();

  if (hovered && mouseButton && !active.has_value())
  {
    active = id;
  }

  if (active == id && !mouseButton)
  {
    if (hovered)
      clicked = true;

    active.reset();
  }

  if (active == id)
    ;
  else if (hot == id)
    color = vec4(vec3(color.r, color.g, color.b) * 0.7f, color.a);

  renderer->renderFilledAABB(rect, color);
  layout->pushWidget(size);

  return clicked;
}

bool UI::textureButton(UIRenderer* renderer, unsigned int textureID, vec2 size, 
                       ID id, const vec4& color, const vec4& tintColor)
{
  auto layout = topLayout();
  assert(layout != NULL);

  const auto pos  = layout->availablePos();
  const auto rect = AABB(pos, size, 0.0f);

  bool hovered = rect.contains(mousePos);
  bool clicked = false;

  if (hovered)
    hot = id;
  else if (hot == id)
    hot.reset();

  if (hovered && mouseButton && !active.has_value())
  {
    active = id;
  }

  if (active == id && !mouseButton)
  {
    if (hovered)
      clicked = true;

    active.reset();
  }

  vec4 finalTint = tintColor;
  if (active == id)
    finalTint = vec4(vec3(tintColor) * 0.8f, tintColor.a);
  else if (hot == id)
    finalTint = vec4(vec3(tintColor) * 0.9f, tintColor.a);

  renderer->renderTexturedAABB(rect, textureID, finalTint, color);
  layout->pushWidget(size);

  return clicked;
}

bool UI::slider(UIRenderer* renderer, float& value, vec2 size, float cornerRadius, vec4 trackColor, vec4 fillColor, vec4 scrubberColor, ID id)
{
  auto layout = topLayout();
  assert(layout != NULL);

  const auto pos = layout->availablePos();
  const auto trackRect = AABB(pos, size, cornerRadius);
  
  value = glm::clamp(value, 0.0f, 1.0f);
  
  bool hovered = trackRect.contains(mousePos);
  bool changed = false;
  
  if (hovered)
    hot = id;
  else if (hot == id)
    hot.reset();
  
  float scrubberX = pos.x - (size.x * 0.5f) + (size.x * value);
  vec2 scrubberSize = vec2(size.y * 2.2f, size.y * 2.25f);
  const auto scrubberRect = AABB(vec2(scrubberX, pos.y), scrubberSize, 100.0f);
  
  bool scrubberHovered = scrubberRect.contains(mousePos);
  
  if ((hovered || scrubberHovered) && mouseButton && !active.has_value())
  {
    active = id;
  }
  
  if (active == id)
  {
    if (mouseButton)
    {
      float trackLeft = pos.x - (size.x * 0.5f);
      float trackRight = pos.x + (size.x * 0.5f);
      float clickX = glm::clamp(mousePos.x, trackLeft, trackRight);
      
      value = (clickX - trackLeft) / size.x;
      value = glm::clamp(value, 0.0f, 1.0f);
      changed = true;
    }
    else
    {
      active.reset();
    }
  }
  
  renderer->renderFilledAABB(trackRect, trackColor);
  
  if (value > 0.0f)
  {
    float fillWidth = size.x * value;
    vec2 fillPos = vec2(pos.x - (size.x * 0.5f) + (fillWidth * 0.5f), pos.y);
    const auto fillRect = AABB(fillPos, vec2(fillWidth, size.y), cornerRadius);
    renderer->renderFilledAABB(fillRect, fillColor);
  }
  
  vec4 finalScrubberColor = scrubberColor;
  if (active == id || scrubberHovered)
    finalScrubberColor = vec4(vec3(scrubberColor) * 1.3f, scrubberColor.a);
  
  renderer->renderFilledAABB(scrubberRect, finalScrubberColor);
  
  layout->pushWidget(size);
  
  return changed;
}

void UI::endLayout()
{
  
}

void UI::end()
{
  popLayout();
}
