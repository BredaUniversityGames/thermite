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
// only this file uses these
namespace {

// C style arrays for compatibility
char profiler_path[256] = "tracy.exe";
char capture_path[256] = "tracy-capture.exe";
char output_path[256] = "profile.tracy";
char address[128] = "127.0.0.1";
int32_t port = 8086;
bool overwrite = true;
bool use_timer = false;
int32_t duration = 10;
HANDLE capture_process = nullptr;

constexpr SDL_DialogFileFilter k_exe_filters[] = {{"Executable", "exe"}, {"All files", "*"}};

constexpr SDL_DialogFileFilter k_output_filters[] = {{"Tracy trace", "tracy"}, {"All files", "*"}};

enum class DialogType { PROFILER, CAPTURE, OUTPUT };
DialogType dialog = DialogType::PROFILER;

void launch_profiler() {
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};

    std::string cmd = std::string(profiler_path);
    cmd += " -a " + std::string(address);
    cmd += " -p " + std::to_string(port);

    std::vector buf(cmd.begin(), cmd.end());
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

    std::string cmd = std::string(capture_path);
    cmd += " -o " + std::string(output_path);
    cmd += " -a " + std::string(address);
    cmd += " -p " + std::to_string(port);
    if (overwrite) {
        cmd += " -f";
    }
    if (use_timer) {
        cmd += " -s " + std::to_string(duration);
    }

    std::vector buf(cmd.begin(), cmd.end());
    buf.push_back('\0');

    if (CreateProcessA(nullptr, buf.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        capture_process = pi.hProcess;
        Log::info("Launched Tracy capture: {}", cmd);
    } else {
        Log::error("Failed to launch Tracy capture. Error code: {}", GetLastError());
    }
}

void stop_capture() {
    if (capture_process) {
        TerminateProcess(capture_process, 0);
        CloseHandle(capture_process);
        capture_process = nullptr;
        Log::info("Stopped Tracy capture");
    }
}

bool is_capture_running() {
    if (!capture_process) return false;
    DWORD exit_code;
    if (GetExitCodeProcess(capture_process, &exit_code)) {
        if (exit_code != STILL_ACTIVE) {
            CloseHandle(capture_process);
            capture_process = nullptr;
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

    switch (dialog) {
        case DialogType::PROFILER:
            strncpy_s(profiler_path, *filelist, sizeof(profiler_path) - 1);
            profiler_path[sizeof(profiler_path) - 1] = '\0';
            Log::info("Selected profiler: {}", profiler_path);
            break;
        case DialogType::CAPTURE:
            strncpy_s(capture_path, *filelist, sizeof(capture_path) - 1);
            capture_path[sizeof(capture_path) - 1] = '\0';
            Log::info("Selected capture: {}", capture_path);
            break;
        case DialogType::OUTPUT:
            strncpy_s(output_path, *filelist, sizeof(output_path) - 1);
            output_path[sizeof(output_path) - 1] = '\0';
            Log::info("Selected output: {}", output_path);
            break;
    }
}

}  // namespace

void Profiler::on_editor_start() {}

void Profiler::on_editor_update(const tmt::FrameData& time) { (void)time; }

void Profiler::on_editor_end() {}

void Profiler::display() {
    ImGui::InputText("Address", address, sizeof(address));
    ImGui::InputInt("Port", &port);

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Profiler", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Path##profiler", profiler_path, sizeof(profiler_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse##profiler")) {
            dialog = DialogType::PROFILER;
            SDL_ShowOpenFileDialog(on_file_selected, nullptr, nullptr, k_exe_filters, SDL_arraysize(k_exe_filters), nullptr, false);
        }

        if (ImGui::Button("Launch Profiler")) {
            launch_profiler();
        }
    }

    if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Path##capture", capture_path, sizeof(capture_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse##capture")) {
            dialog = DialogType::CAPTURE;
            SDL_ShowOpenFileDialog(on_file_selected, nullptr, nullptr, k_exe_filters, SDL_arraysize(k_exe_filters), nullptr, false);
        }

        ImGui::InputText("Output File", output_path, sizeof(output_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse##output")) {
            dialog = DialogType::OUTPUT;
            SDL_ShowSaveFileDialog(on_file_selected, nullptr, nullptr, k_output_filters, SDL_arraysize(k_output_filters), nullptr);
        }

        ImGui::Checkbox("Overwrite existing", &overwrite);

        ImGui::Checkbox("Timed Capture", &use_timer);
        if (use_timer) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt("seconds", &duration);
            duration = max(1, duration);
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
