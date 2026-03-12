#pragma once

#include <imgui.h>

namespace tmt::theme {

// Main colors - very dark theme
constexpr ImVec4 bg_dark = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);           // #191919 - darkest background
constexpr ImVec4 bg_window = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);         // #1e1e1e - window background
constexpr ImVec4 bg_child = ImVec4(0.129f, 0.129f, 0.129f, 1.0f);          // #212121 - child window background
constexpr ImVec4 bg_popup = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);          // #252525 - popup background
constexpr ImVec4 bg_frame = ImVec4(0.161f, 0.161f, 0.161f, 1.0f);          // #292929 - frame background
constexpr ImVec4 bg_frame_hovered = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);  // #333333
constexpr ImVec4 bg_frame_active = ImVec4(0.239f, 0.239f, 0.239f, 1.0f);   // #3d3d3d

// Border colors
constexpr ImVec4 border_color = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);  // #373737 - subtle borders
constexpr ImVec4 border_shadow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

// Text colors
constexpr ImVec4 text_color = ImVec4(0.847f, 0.847f, 0.847f, 1.0f);     // #d8d8d8 - main text
constexpr ImVec4 text_disabled = ImVec4(0.500f, 0.500f, 0.500f, 1.0f);  // #808080 - disabled text

// Header colors (collapsing headers, tree nodes)
constexpr ImVec4 header_color = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);      // #3d3d3d
constexpr ImVec4 header_hovered = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);    // #3d3d3d
constexpr ImVec4 header_active = ImVec4(0.278f, 0.278f, 0.278f, 1.0f);  // #474747

// Button colors
constexpr ImVec4 button_color = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);    // #333333
constexpr ImVec4 button_hovered = ImVec4(0.278f, 0.278f, 0.278f, 1.0f);  // #474747
constexpr ImVec4 button_active = ImVec4(0.318f, 0.318f, 0.318f, 1.0f);   // #515151

// Tab colors
constexpr ImVec4 tab_color = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);             // #252525
constexpr ImVec4 tab_hovered = ImVec4(0.278f, 0.278f, 0.278f, 1.0f);           // #474747
constexpr ImVec4 tab_active = ImVec4(0.200f, 0.200f, 0.200f, 1.0f);            // #333333
constexpr ImVec4 tab_unfocused = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);         // #1e1e1e
constexpr ImVec4 tab_unfocused_active = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);  // #2d2d2d

// Title bar colors
constexpr ImVec4 title_bg = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);            // #191919
constexpr ImVec4 title_bg_active = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);     // #1e1e1e
constexpr ImVec4 title_bg_collapsed = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);  // #191919

// Scrollbar colors
constexpr ImVec4 scrollbar_bg = ImVec4(0.098f, 0.098f, 0.098f, 0.5f);
constexpr ImVec4 scrollbar_grab = ImVec4(0.318f, 0.318f, 0.318f, 1.0f);
constexpr ImVec4 scrollbar_grab_hovered = ImVec4(0.400f, 0.400f, 0.400f, 1.0f);
constexpr ImVec4 scrollbar_grab_active = ImVec4(0.478f, 0.478f, 0.478f, 1.0f);

// Slider/Checkbox/Progress colors
constexpr ImVec4 slider_grab = ImVec4(0.478f, 0.478f, 0.478f, 1.0f);
constexpr ImVec4 slider_grab_active = ImVec4(0.600f, 0.600f, 0.600f, 1.0f);
constexpr ImVec4 check_mark = ImVec4(0.847f, 0.847f, 0.847f, 1.0f);

// Separator
constexpr ImVec4 separator_color = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);
constexpr ImVec4 separator_hovered = ImVec4(0.400f, 0.400f, 0.400f, 1.0f);
constexpr ImVec4 separator_active = ImVec4(0.478f, 0.478f, 0.478f, 1.0f);

// Resize grip
constexpr ImVec4 resize_grip = ImVec4(0.278f, 0.278f, 0.278f, 0.2f);
constexpr ImVec4 resize_grip_hovered = ImVec4(0.400f, 0.400f, 0.400f, 0.67f);
constexpr ImVec4 resize_grip_active = ImVec4(0.478f, 0.478f, 0.478f, 0.95f);

// Docking
constexpr ImVec4 docking_preview = ImVec4(0.278f, 0.478f, 0.678f, 0.7f);
constexpr ImVec4 docking_empty_bg = ImVec4(0.098f, 0.098f, 0.098f, 1.0f);

// Menu bar
constexpr ImVec4 menubar_bg = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);

// Table colors
constexpr ImVec4 table_header_bg = ImVec4(0.145f, 0.145f, 0.145f, 1.0f);
constexpr ImVec4 table_border_strong = ImVec4(0.216f, 0.216f, 0.216f, 1.0f);
constexpr ImVec4 table_border_light = ImVec4(0.176f, 0.176f, 0.176f, 1.0f);
constexpr ImVec4 table_row_bg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
constexpr ImVec4 table_row_bg_alt = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

// Nav highlight
constexpr ImVec4 nav_highlight = ImVec4(0.278f, 0.478f, 0.678f, 1.0f);
constexpr ImVec4 nav_windowing_highlight = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
constexpr ImVec4 nav_windowing_dim_bg = ImVec4(0.8f, 0.8f, 0.8f, 0.20f);

// Modal dim
constexpr ImVec4 modal_dim = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

inline void apply_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Apply colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = text_color;
    colors[ImGuiCol_TextDisabled] = text_disabled;
    colors[ImGuiCol_WindowBg] = bg_window;
    colors[ImGuiCol_ChildBg] = bg_child;
    colors[ImGuiCol_PopupBg] = bg_popup;
    colors[ImGuiCol_Border] = border_color;
    colors[ImGuiCol_BorderShadow] = border_shadow;
    colors[ImGuiCol_FrameBg] = bg_frame;
    colors[ImGuiCol_FrameBgHovered] = bg_frame_hovered;
    colors[ImGuiCol_FrameBgActive] = bg_frame_active;
    colors[ImGuiCol_TitleBg] = title_bg;
    colors[ImGuiCol_TitleBgActive] = title_bg_active;
    colors[ImGuiCol_TitleBgCollapsed] = title_bg_collapsed;
    colors[ImGuiCol_MenuBarBg] = menubar_bg;
    colors[ImGuiCol_ScrollbarBg] = scrollbar_bg;
    colors[ImGuiCol_ScrollbarGrab] = scrollbar_grab;
    colors[ImGuiCol_ScrollbarGrabHovered] = scrollbar_grab_hovered;
    colors[ImGuiCol_ScrollbarGrabActive] = scrollbar_grab_active;
    colors[ImGuiCol_CheckMark] = check_mark;
    colors[ImGuiCol_SliderGrab] = slider_grab;
    colors[ImGuiCol_SliderGrabActive] = slider_grab_active;
    colors[ImGuiCol_Button] = button_color;
    colors[ImGuiCol_ButtonHovered] = button_hovered;
    colors[ImGuiCol_ButtonActive] = button_active;
    colors[ImGuiCol_Header] = header_color;
    colors[ImGuiCol_HeaderHovered] = header_hovered;
    colors[ImGuiCol_HeaderActive] = header_active;
    colors[ImGuiCol_Separator] = separator_color;
    colors[ImGuiCol_SeparatorHovered] = separator_hovered;
    colors[ImGuiCol_SeparatorActive] = separator_active;
    colors[ImGuiCol_ResizeGrip] = resize_grip;
    colors[ImGuiCol_ResizeGripHovered] = resize_grip_hovered;
    colors[ImGuiCol_ResizeGripActive] = resize_grip_active;
    colors[ImGuiCol_Tab] = tab_color;
    colors[ImGuiCol_TabHovered] = tab_hovered;
    colors[ImGuiCol_TabSelected] = tab_active;
    colors[ImGuiCol_TabDimmed] = tab_unfocused;
    colors[ImGuiCol_TabDimmedSelected] = tab_unfocused_active;
    colors[ImGuiCol_DockingPreview] = docking_preview;
    colors[ImGuiCol_DockingEmptyBg] = docking_empty_bg;
    colors[ImGuiCol_PlotLines] = ImVec4(0.610f, 0.610f, 0.610f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.430f, 0.350f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.900f, 0.700f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.600f, 0.0f, 1.0f);
    colors[ImGuiCol_TableHeaderBg] = table_header_bg;
    colors[ImGuiCol_TableBorderStrong] = table_border_strong;
    colors[ImGuiCol_TableBorderLight] = table_border_light;
    colors[ImGuiCol_TableRowBg] = table_row_bg;
    colors[ImGuiCol_TableRowBgAlt] = table_row_bg_alt;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.260f, 0.590f, 0.980f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.90f);
    colors[ImGuiCol_NavHighlight] = nav_highlight;
    colors[ImGuiCol_NavWindowingHighlight] = nav_windowing_highlight;
    colors[ImGuiCol_NavWindowingDimBg] = nav_windowing_dim_bg;
    colors[ImGuiCol_ModalWindowDimBg] = modal_dim;

    // Style settings
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    // Border sizes
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.TabBarBorderSize = 1.0f;

    // Rounding
    // Note: WindowRounding is set to 0 because docked windows should have sharp corners.
    // Undocked/floating windows will automatically get rounding from the OS or can be styled separately.
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    // Alignment
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    // Safe area
    style.DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
}

}  // namespace tmt::theme
