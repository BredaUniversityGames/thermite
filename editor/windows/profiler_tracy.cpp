#include "profiler_tracy.hpp"

#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "engine/core/logger.hpp"

namespace tmt {
namespace {

char g_profiler_path[512] = "tracy.exe";
char g_capture_path[512] = "tracy-capture.exe";
char g_output_path[512] = "profile.tracy";
char g_address[128] = "127.0.0.1";
int32_t g_port = 8086;
bool g_overwrite = true;
bool g_use_timer = false;
int32_t g_duration = 10;
HANDLE g_capture_process = nullptr;

constexpr SDL_DialogFileFilter k_exe_filters[] = {{"Executable", "exe"}, {"All files", "*"}};

constexpr SDL_DialogFileFilter k_output_filters[] = {{"Tracy trace", "tracy"}, {"All files", "*"}};

enum class DialogType { Profiler, Capture, Output };
DialogType g_pending_dialog = DialogType::Profiler;

void launch_profiler() {
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};

    std::string cmd = std::string(g_profiler_path);
    cmd += " -a " + std::string(g_address);
    cmd += " -p " + std::to_string(g_port);

    std::vector<char> buf(cmd.begin(), cmd.end());
    buf.push_back('\0');

    if (CreateProcessA(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Log::info("Launched Tracy profiler: {}", cmd);
    } else {
        Log::error("Failed to launch Tracy profiler. Error code: {}", GetLastError());
    }
}

void launch_capture() {
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};

    std::string cmd = std::string(g_capture_path);
    cmd += " -o " + std::string(g_output_path);
    cmd += " -a " + std::string(g_address);
    cmd += " -p " + std::to_string(g_port);
    if (g_overwrite) {
        cmd += " -f";
    }
    if (g_use_timer) {
        cmd += " -s " + std::to_string(g_duration);
    }

    std::vector<char> buf(cmd.begin(), cmd.end());
    buf.push_back('\0');

    if (CreateProcessA(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        g_capture_process = pi.hProcess;
        Log::info("Launched Tracy capture: {}", cmd);
    } else {
        Log::error("Failed to launch Tracy capture. Error code: {}", GetLastError());
    }
}

void stop_capture() {
    if (g_capture_process) {
        TerminateProcess(g_capture_process, 0);
        CloseHandle(g_capture_process);
        g_capture_process = nullptr;
        Log::info("Stopped Tracy capture");
    }
}

bool is_capture_running() {
    if (!g_capture_process) return false;
    DWORD exit_code;
    if (GetExitCodeProcess(g_capture_process, &exit_code)) {
        if (exit_code != STILL_ACTIVE) {
            CloseHandle(g_capture_process);
            g_capture_process = nullptr;
            return false;
        }
        return true;
    }
    return false;
}

void SDLCALL on_file_selected(void* userdata, const char* const* filelist, int filter) {
    (void)userdata;
    (void)filter;

    if (!filelist || !*filelist) {
        return;
    }

    switch (g_pending_dialog) {
        case DialogType::Profiler:
            std::strncpy(g_profiler_path, *filelist, sizeof(g_profiler_path) - 1);
            g_profiler_path[sizeof(g_profiler_path) - 1] = '\0';
            Log::info("Selected profiler: {}", g_profiler_path);
            break;
        case DialogType::Capture:
            std::strncpy(g_capture_path, *filelist, sizeof(g_capture_path) - 1);
            g_capture_path[sizeof(g_capture_path) - 1] = '\0';
            Log::info("Selected capture: {}", g_capture_path);
            break;
        case DialogType::Output:
            std::strncpy(g_output_path, *filelist, sizeof(g_output_path) - 1);
            g_output_path[sizeof(g_output_path) - 1] = '\0';
            Log::info("Selected output: {}", g_output_path);
            break;
    }
}

}  // namespace

void Profiler::on_editor_start() {}

void Profiler::on_editor_update(const tmt::FrameData& time) { (void)time; }

void Profiler::on_editor_end() {}

void Profiler::display() {
    ImGui::InputText("Address", g_address, sizeof(g_address));
    ImGui::InputInt("Port", &g_port);
    g_port = std::clamp(g_port, 1, 65535);

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Profiler", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Path##profiler", g_profiler_path, sizeof(g_profiler_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse##profiler")) {
            g_pending_dialog = DialogType::Profiler;
            SDL_ShowOpenFileDialog(on_file_selected, nullptr, nullptr, k_exe_filters, SDL_arraysize(k_exe_filters), nullptr, false);
        }

        if (ImGui::Button("Launch Profiler")) {
            launch_profiler();
        }
    }

    if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Path##capture", g_capture_path, sizeof(g_capture_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse##capture")) {
            g_pending_dialog = DialogType::Capture;
            SDL_ShowOpenFileDialog(on_file_selected, nullptr, nullptr, k_exe_filters, SDL_arraysize(k_exe_filters), nullptr, false);
        }

        ImGui::InputText("Output File", g_output_path, sizeof(g_output_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse##output")) {
            g_pending_dialog = DialogType::Output;
            SDL_ShowSaveFileDialog(on_file_selected, nullptr, nullptr, k_output_filters, SDL_arraysize(k_output_filters), nullptr);
        }

        ImGui::Checkbox("Overwrite existing", &g_overwrite);

        ImGui::Checkbox("Timed Capture", &g_use_timer);
        if (g_use_timer) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("seconds", &g_duration);
            g_duration = max(1, g_duration);
        }

        bool running = is_capture_running();
        if (running) {
            if (ImGui::Button("Stop Capture")) {
                stop_capture();
            }
        } else {
            if (ImGui::Button("Start Capture")) {
                launch_capture();
            }
        }
    }
}

}  // namespace tmt
