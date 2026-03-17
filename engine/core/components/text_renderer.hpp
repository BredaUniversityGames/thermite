#pragma once

#include <string>

#include "engine/core/resources/font.hpp"
#include "engine/tools/types/color.hpp"

namespace tmt {

/* Horizontal text alignment */
enum class HorizontalAlign : uint8_t {
    LEFT,    // Align to left edge
    CENTER,  // Center horizontally
    RIGHT    // Align to right edge
};

/* Vertical text alignment */
enum class VerticalAlign : uint8_t {
    TOP,     // Align to top edge
    MIDDLE,  // Center vertically
    BOTTOM   // Align to bottom edge
};

/* Text overflow behavior */
enum class TextOverflow : uint8_t {
    VISIBLE,   // Text extends beyond bounds
    CLIP,      // Clip text at bounds
    ELLIPSIS,  // Add "..." when text exceeds bounds
    WRAP       // Wrap to next line (same as word_wrap=true)
};

/* Font style for rich text */
enum class FontStyle : uint8_t { NORMAL = 0, BOLD = 1 << 0, ITALIC = 1 << 1, BOLD_ITALIC = BOLD | ITALIC };

/*
 * TextRenderer Component
 *
 * Renders text using SDF (Signed Distance Field) fonts.
 * Supports rich text formatting with inline tags:
 *   <b>bold</b>, <i>italic</i>, <color=#RRGGBB>colored</color>,
 *   <size=N>sized</size>, <br> (line break)
 *
 * Usage:
 *   - Add TextRenderer component to an entity with UIComponent and Transform
 *   - Set the text content and font reference
 *   - Configure styling properties as needed
 */
struct TextRenderer {
    /* Text content - supports rich text tags */
    std::string text {};

    /* Font resource - if null, uses default font */
    ResourceRef<Font> font {};

    /* Base font size in pixels */
    float font_size = 16.0f;

    /* Text color (can be overridden by rich text tags) */
    RGBA color = RGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    /* Layout properties */
    float line_spacing = 1.0f;       // Multiplier for line height
    float letter_spacing = 0.0f;     // Extra spacing between characters
    float paragraph_spacing = 0.0f;  // Extra spacing after line breaks

    /* Alignment */
    HorizontalAlign horizontal_align = HorizontalAlign::LEFT;
    VerticalAlign vertical_align = VerticalAlign::TOP;

    /* Word wrapping and overflow */
    bool word_wrap = false;   // Enable word wrapping
    float max_width = 0.0f;   // Max width before wrapping (0 = unlimited)
    float max_height = 0.0f;  // Max height for text area (0 = unlimited)
    TextOverflow overflow = TextOverflow::VISIBLE;

    /* When enabled, max_width and max_height are automatically set from the
     * attached UIComponent's size every frame. This allows the text bounds
     * to follow the UI element's size. */
    bool use_ui_component_size = true;

    /* SDF Effects - these work because we use signed distance field fonts */

    /* Outline effect */
    float outline_width = 0.0f;  // Outline thickness (0 = no outline)
    RGBA outline_color = RGBA(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    /* Shadow effect */
    glm::vec2 shadow_offset = glm::vec2(0.0f);  // Shadow offset in pixels
    RGBA shadow_color = RGBA(glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    float shadow_softness = 0.0f;               // Shadow blur amount

    /* Glow effect */
    float glow_width = 0.0f;  // Glow radius (0 = no glow)
    RGBA glow_color = RGBA(glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));

    /* SDF rendering parameters */
    float softness = 0.0f;  // Edge softness (anti-aliasing, auto-calculated if 0)
    float boldness = 0.0f;  // Thicken/thin the text (-1 to 1)

    /* Rich text parsing */
    bool rich_text_enabled = true;  // Enable parsing of rich text tags

    /* Constructors */
    TextRenderer() = default;
    TextRenderer(const std::string& text) : text(text) {}
    TextRenderer(const std::string& text, ResourceRef<Font> font) : text(text), font(std::move(font)) {}
    ~TextRenderer() = default;
};

}  // namespace tmt

TMT_COMPONENT(
    tmt::TextRenderer, "TextRenderer",
    (text, font, font_size, color, line_spacing, letter_spacing, paragraph_spacing, horizontal_align, vertical_align, word_wrap, max_width, max_height, overflow, use_ui_component_size,
     outline_width, outline_color, shadow_offset, shadow_color, shadow_softness, glow_width, glow_color, softness, boldness, rich_text_enabled)
);