#pragma once

#include "core/io.hpp"

namespace tmt {

class DirectoryWatcher {
   public:
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

    DirectoryWatcher(const IO::FileLocation& location, bool watch_recursively = true, WatchReason watch_reasons = WatchReason::ANY);
    DirectoryWatcher(const std::filesystem::path& path, bool watch_recursively = true, WatchReason watch_reasons = WatchReason::ANY);

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
};

}  // namespace tmt