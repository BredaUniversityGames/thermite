#pragma once

#include <string>
#include <filesystem>

namespace tmt {
class IO {
   public:
    enum class Location : uint8_t { GAME, ENGINE, EDITOR };

    // Write arbitrary data, returns success
    static bool write_file(Location sub_category, const std::filesystem::path& relative_path, const char* data, size_t size);

    // Write text to file, overwrites if file exists. set overwrite = false to append. returns success
    static bool write_text_file(Location sub_category, const std::filesystem::path& relative_path, const std::string& text, bool overwrite = true);

    // Read file, returns vector containing bytes
    static std::vector<char> read_file(Location sub_category, const std::filesystem::path& relative_path);

    // Read text file (primarily for json)
    static std::string read_text_file(Location sub_category, const std::filesystem::path& relative_path);

   private:
    static const char* path_str[3];

    static std::filesystem::path GetAbsolutePath(Location sub_cat, const std::filesystem::path& relative_path);

    static bool stream_open(std::fstream& file_stream, const std::filesystem::path& absolute, std::ios::openmode open_mode);
    static bool create_directories(const std::filesystem::path& absolute);
};

}  // namespace tmt
