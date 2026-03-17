#include "text_layout.hpp"

#include <algorithm>
#include <cmath>

namespace tmt {

TextLayoutResult TextLayout::layout(const TextRenderer& text_renderer, const Font* font, const glm::vec2& container_size) {
    TextLayoutResult result;

    /* Use default font if none provided */
    if (!font || !font->is_valid()) {
        font = FontManager::get_default_font().get();
    }

    if (!font || !font->is_valid()) {
        /* No font available */
        return result;
    }

    /* Get text to layout */
    std::string text = text_renderer.text;
    if (text.empty()) {
        return result;
    }

    /* Set up layout state */
    LayoutState state {};
    state.font = font;
    state.base_font_size = text_renderer.font_size;
    state.letter_spacing = text_renderer.letter_spacing;
    state.line_spacing = text_renderer.line_spacing;
    /* Use container_size from UIComponent if max_width/max_height not explicitly set */
    state.max_width = text_renderer.word_wrap ? (text_renderer.max_width > 0 ? text_renderer.max_width : container_size.x) : 0.0f;
    state.word_wrap = text_renderer.word_wrap;
    state.base_color = text_renderer.color;

    /* Track max_height for overflow handling - prefer explicit value, fall back to container */
    float effective_max_height = text_renderer.max_height > 0 ? text_renderer.max_height : container_size.y;

    /* SDF effects */
    state.outline_width = text_renderer.outline_width;
    state.outline_color = text_renderer.outline_color;
    state.softness = text_renderer.softness;
    state.boldness = text_renderer.boldness;

    /* Initialize cursor */
    state.cursor_x = 0.0f;
    state.cursor_y = 0.0f;
    state.line_height = font->get_metrics().line_height * (state.base_font_size / font->get_metrics().base_size) * state.line_spacing;

    /* Start first line */
    result.lines.push_back(TextLine { 0, 0, 0.0f, state.line_height, 0.0f });
    state.line_start_glyph = 0;

    /* Parse rich text if enabled */
    if (text_renderer.rich_text_enabled && RichTextParser::has_tags(text)) {
        RichTextParseResult parsed = RichTextParser::parse(text, state.base_color);

        for (const auto& segment : parsed.segments) {
            for (size_t i = 0; i < segment.length; ++i) {
                uint32_t codepoint = static_cast<uint32_t>(parsed.stripped_text[segment.start_index + i]);
                process_char(codepoint, segment, state, result);
            }
        }
    } else {
        /* Plain text layout */
        TextSegment segment;
        segment.color = state.base_color;
        segment.size_scale = 1.0f;

        for (size_t i = 0; i < text.size(); ++i) {
            uint32_t codepoint = static_cast<uint32_t>(text[i]);
            process_char(codepoint, segment, state, result);
        }
    }

    /* Finalize last line */
    if (!result.lines.empty()) {
        auto& last_line = result.lines.back();
        last_line.glyph_count = result.glyphs.size() - last_line.start_index;
        last_line.width = state.line_width;
    }

    /* Calculate total bounds */
    result.bounds.y = state.cursor_y + state.line_height;
    result.bounds.x = 0.0f;
    for (const auto& line : result.lines) {
        result.bounds.x = std::max(result.bounds.x, line.width);
    }

    /* Apply horizontal alignment to each line */
    float align_width = container_size.x > 0 ? container_size.x : result.bounds.x;
    for (auto& line : result.lines) {
        align_line(line, result.glyphs, text_renderer.horizontal_align, align_width);
    }

    /* Apply vertical alignment */
    float align_height = container_size.y > 0 ? container_size.y : result.bounds.y;
    align_vertical(result, text_renderer.vertical_align, align_height);

    /* Handle overflow based on effective max height (explicit or from container) */
    if (effective_max_height > 0) {
        handle_overflow(result, text_renderer, effective_max_height);
    }

    return result;
}

TextLayoutResult TextLayout::layout_plain(
    const std::string& text, const Font& font, float font_size, const glm::vec4& color, float max_width, bool word_wrap, HorizontalAlign h_align, VerticalAlign v_align, float letter_spacing,
    float line_spacing
) {
    /* Create a temporary TextRenderer for layout */
    TextRenderer tr;
    tr.text = text;
    tr.font_size = font_size;
    tr.color = RGBA(color);
    tr.letter_spacing = letter_spacing;
    tr.line_spacing = line_spacing;
    tr.word_wrap = word_wrap;
    tr.max_width = max_width;
    tr.horizontal_align = h_align;
    tr.vertical_align = v_align;
    tr.rich_text_enabled = false;

    return layout(tr, &font, glm::vec2(max_width, 0.0f));
}

glm::vec2 TextLayout::measure(const std::string& text, const Font& font, float font_size, float max_width, bool word_wrap, float letter_spacing) {
    TextRenderer tr;
    tr.text = text;
    tr.font_size = font_size;
    tr.letter_spacing = letter_spacing;
    tr.word_wrap = word_wrap;
    tr.max_width = max_width;
    tr.rich_text_enabled = false;

    TextLayoutResult result = layout(tr, &font, glm::vec2(max_width, 0.0f));
    return result.bounds;
}

void TextLayout::process_char(uint32_t codepoint, const TextSegment& segment, LayoutState& state, TextLayoutResult& result) {
    const Font* font = state.font;
    const FontMetrics& metrics = font->get_metrics();
    const float scale_factor = (state.base_font_size / metrics.base_size) * segment.size_scale;

    /* Handle newline */
    if (codepoint == '\n') {
        new_line(state, result);
        return;
    }

    /* Handle carriage return (skip) */
    if (codepoint == '\r') {
        return;
    }

    /* Handle tab as spaces */
    if (codepoint == '\t') {
        /* Insert 4 spaces worth of advance */
        const GlyphInfo& space = font->get_glyph(' ');
        state.cursor_x += space.advance * scale_factor * 4.0f;
        state.line_width = state.cursor_x;
        return;
    }

    /* Track word boundaries for word wrap */
    if (codepoint == ' ') {
        state.word_start_glyph = result.glyphs.size();
        state.word_start_x = state.cursor_x;
    }

    /* Get glyph info */
    const GlyphInfo& glyph = font->get_glyph(codepoint);
    if (!glyph.valid) {
        return;
    }

    /* Calculate glyph dimensions */
    float glyph_width = glyph.size.x * scale_factor;
    float advance = glyph.advance * scale_factor + state.letter_spacing;

    /* Check if we need to wrap */
    if (state.word_wrap && state.max_width > 0) {
        if (state.cursor_x + glyph_width > state.max_width && state.cursor_x > 0) {
            /* Check if we're in the middle of a word */
            if (codepoint != ' ' && state.word_start_glyph < result.glyphs.size()) {
                /* Move the current word to the next line */
                break_word(state, result);
            } else {
                /* Just start a new line */
                new_line(state, result);
            }
        }
    }

    /* Create glyph instance */
    GlyphInstance instance;
    instance.position.x = state.cursor_x + glyph.bearing.x * scale_factor;
    /* Position glyph relative to the baseline (Y-down coordinate system, Y=0 at top):
     * - cursor_y is the top of the line
     * - ascender is the distance from baseline to top of line (positive)
     * - bearing.y is how far above the baseline the top of the glyph sits
     * - For baseline alignment: the baseline is at cursor_y + ascender
     * - The glyph top should be at baseline - bearing.y = cursor_y + ascender - bearing.y
     * - But in Y-down, "above" means smaller Y, so: glyph_y = cursor_y + (ascender - bearing.y) */
    instance.position.y = state.cursor_y + (metrics.ascender - glyph.bearing.y) * scale_factor;
    instance.size = glyph.size * scale_factor;
    instance.uv_rect = glyph.uv_rect;
    instance.color = segment.color;
    instance.atlas_index = font->atlas_image.get_index();
    instance.size_scale = segment.size_scale;

    /* SDF effects */
    instance.outline_width = state.outline_width;
    instance.outline_color = state.outline_color;
    instance.softness = state.softness;
    instance.boldness = state.boldness;

    /* Don't add glyphs for spaces (but still advance cursor) */
    if (codepoint != ' ') {
        result.glyphs.push_back(instance);
    }

    /* Advance cursor */
    state.cursor_x += advance;
    state.line_width = state.cursor_x;

    /* Update line height if this glyph is taller */
    float glyph_line_height = metrics.line_height * scale_factor * state.line_spacing;
    if (glyph_line_height > state.line_height) {
        state.line_height = glyph_line_height;
        if (!result.lines.empty()) {
            result.lines.back().height = state.line_height;
        }
    }
}

void TextLayout::new_line(LayoutState& state, TextLayoutResult& result) {
    /* Finalize current line */
    if (!result.lines.empty()) {
        auto& current_line = result.lines.back();
        current_line.glyph_count = result.glyphs.size() - current_line.start_index;
        current_line.width = state.line_width;
    }

    /* Move to next line */
    state.cursor_y += state.line_height;
    state.cursor_x = 0.0f;
    state.line_width = 0.0f;

    /* Reset line height for new line */
    const FontMetrics& metrics = state.font->get_metrics();
    state.line_height = metrics.line_height * (state.base_font_size / metrics.base_size) * state.line_spacing;

    /* Start new line */
    TextLine line;
    line.start_index = result.glyphs.size();
    line.glyph_count = 0;
    line.width = 0.0f;
    line.height = state.line_height;
    line.baseline_offset = metrics.ascender * (state.base_font_size / metrics.base_size);
    result.lines.push_back(line);

    /* Reset word tracking */
    state.line_start_glyph = result.glyphs.size();
    state.word_start_glyph = result.glyphs.size();
    state.word_start_x = 0.0f;
}

void TextLayout::break_word(LayoutState& state, TextLayoutResult& result) {
    if (state.word_start_glyph >= result.glyphs.size()) {
        /* No word to break, just start new line */
        new_line(state, result);
        return;
    }

    /* Calculate how much to move */
    float offset_x = state.word_start_x;

    /* Finalize current line (up to word start) */
    if (!result.lines.empty()) {
        auto& current_line = result.lines.back();
        current_line.glyph_count = state.word_start_glyph - current_line.start_index;
        current_line.width = state.word_start_x;
    }

    /* Move to next line */
    state.cursor_y += state.line_height;

    /* Reset line height */
    const FontMetrics& metrics = state.font->get_metrics();
    state.line_height = metrics.line_height * (state.base_font_size / metrics.base_size) * state.line_spacing;

    /* Start new line */
    TextLine line;
    line.start_index = state.word_start_glyph;
    line.height = state.line_height;
    line.baseline_offset = metrics.ascender * (state.base_font_size / metrics.base_size);
    result.lines.push_back(line);

    /* Move glyphs from word start to new line */
    for (size_t i = state.word_start_glyph; i < result.glyphs.size(); ++i) {
        result.glyphs[i].position.x -= offset_x;
        result.glyphs[i].position.y =
            state.cursor_y + (metrics.ascender - result.glyphs[i].position.y / (state.base_font_size / metrics.base_size)) * (state.base_font_size / metrics.base_size);
    }

    /* Update cursor position */
    state.cursor_x = state.line_width - offset_x;
    state.line_width = state.cursor_x;

    /* Reset word tracking */
    state.line_start_glyph = state.word_start_glyph;
    state.word_start_glyph = result.glyphs.size();
    state.word_start_x = state.cursor_x;
}

void TextLayout::align_line(TextLine& line, const std::vector<GlyphInstance>& glyphs, HorizontalAlign h_align, float container_width) {
    if (line.glyph_count == 0 || h_align == HorizontalAlign::LEFT) {
        return;
    }

    float offset = 0.0f;

    switch (h_align) {
        case HorizontalAlign::CENTER:
            offset = (container_width - line.width) * 0.5f;
            break;
        case HorizontalAlign::RIGHT:
            offset = container_width - line.width;
            break;
        default:
            break;
    }

    /* Apply offset to all glyphs in this line */
    /* Note: We need to cast away const here since we modify the glyphs */
    /* This is safe because we know the glyphs vector is mutable in the caller */
    for (size_t i = line.start_index; i < line.start_index + line.glyph_count; ++i) {
        const_cast<GlyphInstance&>(glyphs[i]).position.x += offset;
    }
}

void TextLayout::align_vertical(TextLayoutResult& result, VerticalAlign v_align, float container_height) {
    if (result.glyphs.empty() || v_align == VerticalAlign::TOP) {
        return;
    }

    float offset = 0.0f;

    switch (v_align) {
        case VerticalAlign::MIDDLE:
            offset = (container_height - result.bounds.y) * 0.5f;
            break;
        case VerticalAlign::BOTTOM:
            offset = container_height - result.bounds.y;
            break;
        default:
            break;
    }

    /* Apply offset to all glyphs */
    for (auto& glyph : result.glyphs) {
        glyph.position.y += offset;
    }

    result.offset.y = offset;
}

void TextLayout::handle_overflow(TextLayoutResult& result, const TextRenderer& text_renderer, float max_height) {
    if (result.bounds.y <= max_height) {
        return;
    }

    switch (text_renderer.overflow) {
        case TextOverflow::VISIBLE:
            /* Do nothing, allow overflow */
            break;

        case TextOverflow::CLIP:
            /* Remove glyphs that are below max_height */
            {
                auto it = std::remove_if(result.glyphs.begin(), result.glyphs.end(), [max_height](const GlyphInstance& g) { return g.position.y > max_height; });
                if (it != result.glyphs.end()) {
                    result.glyphs.erase(it, result.glyphs.end());
                    result.truncated = true;
                }
            }
            break;

        case TextOverflow::ELLIPSIS:
            /* TODO: Add "..." at the truncation point */
            /* For now, treat same as CLIP */
            {
                auto it = std::remove_if(result.glyphs.begin(), result.glyphs.end(), [max_height](const GlyphInstance& g) { return g.position.y > max_height; });
                if (it != result.glyphs.end()) {
                    result.glyphs.erase(it, result.glyphs.end());
                    result.truncated = true;
                }
            }
            break;

        case TextOverflow::WRAP:
            /* Word wrap is handled during layout, not here */
            break;
    }
}

}  // namespace tmt