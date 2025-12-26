#include "directory_watcher.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "core/logger.hpp"

namespace tmt {

DirectoryWatcher::DirectoryWatcher(const IO::FileLocation& location, const bool watch_recursively, const WatchReason watch_reasons)
    : DirectoryWatcher {absolute(location.get_relative_path()), watch_recursively, watch_reasons} {}

DirectoryWatcher::DirectoryWatcher(const std::filesystem::path& path, const bool watch_recursively, const WatchReason watch_reasons) {
    if (path.empty() || !exists(path)) {
        Log::error("DirectoryWatcher: Failed to create directory watcher because of invalid path.");
        return;
    }

    const std::string path_string = path.generic_string();
    watching_handle = FindFirstChangeNotification(path_string.c_str(), watch_recursively, static_cast<DWORD>(watch_reasons));

    if (watching_handle == INVALID_HANDLE_VALUE) {
        watching_handle = nullptr;

        Log::error("DirectoryWatcher: Failed to create directory watcher, error code {}.", GetLastError());
        return;
    }
}

DirectoryWatcher::DirectoryWatcher(DirectoryWatcher&& other) noexcept : watching_handle {other.watching_handle} { other.watching_handle = nullptr; }

DirectoryWatcher& DirectoryWatcher::operator=(DirectoryWatcher&& other) noexcept {
    watching_handle = other.watching_handle;
    other.watching_handle = nullptr;
    return *this;
}

bool DirectoryWatcher::check_changes() {
    if (!is_valid()) {
        Log::error("DirectoryWatcher: Instance was not created correctly, watcher is invalid.");

        return false;
    }

    const DWORD wait_status = WaitForSingleObject(watching_handle, 0);
    if (wait_status != WAIT_OBJECT_0) {
        if (wait_status == WAIT_FAILED) Log::error("DirectoryWatcher: Failed to check directory, error code {}.", GetLastError());

        return false;
    }

    if (FindNextChangeNotification(watching_handle) == FALSE) {
        Log::error("DirectoryWatcher: Failed updating watcher, error code {}.", GetLastError());
        clear();

        return false;
    }

    return true;
}

void DirectoryWatcher::clear() {
    if (is_valid() && watching_handle != INVALID_HANDLE_VALUE) FindCloseChangeNotification(watching_handle);

    watching_handle = nullptr;
}

}  // namespace tmt