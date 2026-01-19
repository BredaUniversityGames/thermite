#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
/* Shouldn't happen */
#error THERMITE_EDITOR must be defined to 1 in editor builds
#endif

#include "editor/editor.hpp"

#include "engine/engine.hpp"

#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/scenes.hpp"

#include "editor/imgui/manager.hpp"
#include "editor/core/font_manager.hpp"
#include "engine/tools/profiler.hpp"

/* Windows */
#include "editor/windows/hierarchy.hpp"
#include "editor/windows/viewport.hpp"
#include "editor/windows/game_flow.hpp"
#include "editor/windows/inspector.hpp"
#include "editor/windows/goap_debugger.hpp"
#include "editor/windows/goap_action_editor.hpp"
#include "editor/windows/font_control.hpp"
#include "editor/windows/profiler_tracy.hpp"
#include "editor/windows/audio_mixer.hpp"
#include "editor/windows/scenes.hpp"
#include "editor/windows/asset_browser.hpp"
#include "editor/windows/imgui_demo.hpp"
#include "editor/windows/console.hpp"
#include "editor/windows/motion_math.hpp"

#include "editor/windows/voxel_editor.hpp"

/* Singleton */
tmt::Editor tmt::editor;

namespace tmt {

Editor::Editor() : imgui_manager(*new ImGuiManager()) {}

Editor::~Editor() { delete &imgui_manager; }

void Editor::init() { Log::info("Thermite Editor initialized."); }

void Editor::on_engine_init(const ApplicationSpecs&) {
    tmt::Log::info("Starting Thermite Editor...");
    imgui_manager.init();
    save_data.load();

    windows[Mode::GAME].add<Hierarchy>();
    windows[Mode::GAME].add<GameFlow>();
    windows[Mode::GAME].add<Inspector>();
    windows[Mode::GAME].add<Viewport>();
    windows[Mode::GAME].add<Profiler>();
    windows[Mode::GAME].add<GoapDebugger>();
    windows[Mode::GAME].add<GoapActionEditor>();
    windows[Mode::GAME].add<FontControl>();
    windows[Mode::GAME].add<AudioMixer>();
    windows[Mode::GAME].add<ScenesWindow>();
    windows[Mode::GAME].add<AssetBrowser>();
    windows[Mode::GAME].add<Console>();
    windows[Mode::GAME].add<ImguiDemo>();
    windows[Mode::GAME].add<MotionMathPreview>();

    windows[Mode::VOXEL].add<VoxelEditor>();

    for (auto& [mode, collection] : windows) {
        for (const auto& window : collection) {
            window->on_editor_start();
        }
    }
}

void Editor::on_engine_update(const FrameData& time) {
    TMT_ZONE_SCOPED_N("Editor Update");
    {
        TMT_ZONE_SCOPED_N("ImGui New Frame");
        imgui_manager.new_frame();
    }

    main_menu_bar();

    for (auto& [mode, collection] : windows) {
        for (const auto& window : collection) {
            window->on_editor_update(time);
        }
    }

    auto& open_windows = editor.save_data.open_windows;
    for (const auto& window : windows[editor_mode]) {
        const auto& name = window->get_title();
        TMT_ZONE_SCOPED_STRING(name);

        const ImGuiWindowFlags_ flags = static_cast<ImGuiWindowFlags_>(window->get_window_flags());
        if (open_windows.contains(name) == false) {
            open_windows[name] = window->default_open();
        }
        bool& open = open_windows[name];

        if (open) {
            window->before_begin();
            ImGui::Begin(name.c_str(), window->is_closable() ? &open : nullptr, flags);
            window->display();
            ImGui::End();
            window->end_display();
        }
    }

    {
        TMT_ZONE_SCOPED_N("ImGui End Frame");
        imgui_manager.end_frame();
    }
}

void Editor::on_engine_fixed_update(const FrameData& time) {
    for (auto& [mode, collection] : windows) {
        for (const auto& window : collection) {
            window->on_editor_fixed_update(time);
        }
    }
}

void Editor::on_engine_end() {
    for (auto& [mode, collection] : windows) {
        for (const auto& window : collection) {
            window->on_editor_end();
        }
    }

    tmt::Log::info("Shutting down Thermite Editor...");
    imgui_manager.deinit();
    save_data.save();
}

void Editor::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Save Scene")) {
                engine.scenes.serialize_active_scene();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows")) {
            for (const auto& window : windows[editor_mode]) {
                const auto& name = window->get_title();
                bool& open = save_data.open_windows[name];
                ImGui::MenuItem(name.c_str(), nullptr, &open);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer")) {
            /* List of display mode labels */
            static const std::vector<std::string> DISPLAY_MODE_LABELS {"Default", "Steps (0..128)", "Visibility", "Depth (0..100)", "Normals", "Albedo", "Illuminance"};

            static uint32_t display_mode_index = 0u;
            const std::string& display_mode = DISPLAY_MODE_LABELS[display_mode_index];

            if (ImGui::BeginMenu(("Display Mode (" + display_mode + ")").c_str())) {
                /* Render all display mode options */
                for (uint32_t i = 0u; i < DISPLAY_MODE_LABELS.size(); ++i) {
                    const bool selected = engine.renderer.display_mode == magic_enum::enum_cast<DisplayMode>(i).value_or(DisplayMode::DEFAULT);

                    if (ImGui::MenuItem(DISPLAY_MODE_LABELS[i].c_str(), nullptr, selected)) {
                        display_mode_index = i;
                        engine.renderer.display_mode = magic_enum::enum_cast<DisplayMode>(i).value_or(DisplayMode::DEFAULT);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Mode")) {
            for (const auto [mode, name] : magic_enum::enum_entries<Editor::Mode>()) {
                if (ImGui::MenuItem(name.data(), nullptr, editor_mode == mode)) editor_mode = mode;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

}  // namespace tmt
