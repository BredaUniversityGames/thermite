#pragma once

#include "engine/core/io.hpp"

#include <chrono>

namespace tmt {

enum class WatchReason : uint8_t {
    FILE_NAME_CHANGE = 0b1 << 0,
    DIR_NAME_CHANGE = 0b1 << 1,
    ATTRIBUTE_CHANGE = 0b1 << 2,
    SIZE_CHANGE = 0b1 << 3,
    LAST_WRITE_CHANGE = 0b1 << 4,
    LAST_ACCESS_CHANGE = 0b1 << 5,
    CREATION = 0b1 << 6,
    SECURITY_CHANGE = 0b1 << 7,

    ANY = FILE_NAME_CHANGE | DIR_NAME_CHANGE | ATTRIBUTE_CHANGE | SIZE_CHANGE | LAST_WRITE_CHANGE | LAST_ACCESS_CHANGE | CREATION | SECURITY_CHANGE
};

class DirectoryWatcher {
    // Time to ignore incoming change notices, this is to avoid double notifying about changes (some programs e.g. vengi, when they save a file will write an empty file to disk before write
    // the new contents to the next frame).
    static constexpr std::chrono::milliseconds DOUBLE_CHANGE_BUFFER_TIME {34};
    // Invalid time, used to clear the last_update_time after preemptively notifying about a changed file when avoiding double notifies.
    static constexpr std::chrono::time_point<std::chrono::system_clock> INVALID_TIME {};

   public:
    DirectoryWatcher() = default;
    DirectoryWatcher(const IO::FileLocation& location, bool watch_recursively = true, bool avoid_double_notify = false, WatchReason watch_reasons = WatchReason::ANY);
    DirectoryWatcher(const std::filesystem::path& path, bool watch_recursively = true, bool avoid_double_notify = false, WatchReason watch_reasons = WatchReason::ANY);

    DirectoryWatcher(DirectoryWatcher&& other) noexcept;
    DirectoryWatcher& operator=(DirectoryWatcher&& other) noexcept;

    // Delete the copy constructors since we don't want to accidentally copy these (usually not intended).
    DirectoryWatcher(DirectoryWatcher&) = delete;
    DirectoryWatcher(const DirectoryWatcher&) = delete;
    DirectoryWatcher(const DirectoryWatcher&&) = delete;

    [[nodiscard]] bool is_valid() const { return watching_handle != nullptr; }
    [[nodiscard]] bool check_changes();
    void clear();

   private:
    void* watching_handle {nullptr};
    bool avoid_double_notify;

    std::chrono::time_point<std::chrono::system_clock> last_update_time {INVALID_TIME};
};

class FileWatcher {
   public:
    FileWatcher() = default;
    FileWatcher(const IO::FileLocation& location, bool avoid_double_notify = true, WatchReason watch_reasons = WatchReason::LAST_WRITE_CHANGE);
    FileWatcher(const std::filesystem::path& path, bool avoid_double_notify = true, WatchReason watch_reasons = WatchReason::LAST_WRITE_CHANGE);

    FileWatcher(FileWatcher&& other) noexcept;
    FileWatcher& operator=(FileWatcher&& other) noexcept;

    // Delete the copy constructors since we don't want to accidentally copy these (usually not intended).
    FileWatcher(FileWatcher&) = delete;
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher(const FileWatcher&&) = delete;

    [[nodiscard]] bool is_valid() const { return !file_path.empty() && directory_watcher.is_valid(); }
    [[nodiscard]] bool check_changes();
    void clear();

   private:
    std::filesystem::path file_path;
    DirectoryWatcher directory_watcher;

    std::filesystem::file_time_type last_write_time_cache {};
};

}  // namespace tmt