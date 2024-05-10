#include "Utils.hpp"

#include <ien/fs_utils.hpp>
#include <ien/io_utils.hpp>
#include <ien/str_utils.hpp>

#include <filesystem>
#include <format>
#include <unordered_set>

const std::unordered_set<std::string> ANIMATION_EXTENSIONS = { ".gif", ".png", ".webp" };
const std::unordered_set<std::string> IMAGE_EXTENSIONS = { ".png", ".jpg", ".jpeg", ".webp" };
const std::unordered_set<std::string> VIDEO_EXTENSIONS = { ".mkv", ".mp4", ".webm", ".mov" };

constexpr const char* APNG_IDAT_STR = "IDAT";
constexpr const char* APNG_ACTL_STR = "acTL";
constexpr const char* WEBP_ANIM_STR = "ANIM";
constexpr const char* WEBP_ANMF_STR = "ANMF";

bool isPngAnimated(std::span<const uint8_t> data)
{
    if (data.empty())
    {
        return false;
    }

    const std::string_view sv(reinterpret_cast<const char*>(data.data()), data.size());
    const size_t idat_pos = sv.find(APNG_IDAT_STR);
    if (idat_pos == std::string_view::npos)
    {
        return false;
    }

    return sv.substr(0, idat_pos).find(APNG_ACTL_STR) != std::string_view::npos;
}

bool isWebpAnimated(std::span<const uint8_t> data)
{
    const std::string_view sv(reinterpret_cast<const char*>(data.data()), data.size());
    return sv.find(WEBP_ANIM_STR) != std::string_view::npos && sv.find(WEBP_ANMF_STR) != std::string_view::npos;
}

bool hasMediaExtension(const std::string& path)
{
    const auto ext = ien::str_tolower(ien::get_file_extension(path));
    return IMAGE_EXTENSIONS.count(ext) || ANIMATION_EXTENSIONS.count(ext) || VIDEO_EXTENSIONS.count(ext);
}

bool isImage(const std::string& source)
{
    if (isAnimation(source))
    {
        return false;
    }
    return IMAGE_EXTENSIONS.count(std::filesystem::path(source).extension());
}

bool isAnimation(const std::string& path)
{
    const std::string extension = ien::str_tolower(ien::get_file_extension(path));
    if (ANIMATION_EXTENSIONS.count(extension))
    {
        const auto data = ien::read_file_binary(path);
        if (!data)
        {
            return false;
        }

        if (extension == ".png")
        {
            return isPngAnimated(*data);
        }
        if (extension == ".webp")
        {
            return isWebpAnimated(*data);
        }
        if (extension == ".gif")
        {
            return true;
        }
    }
    return false;
}

bool isVideo(const std::string& path)
{
    return VIDEO_EXTENSIONS.count(ien::str_tolower(ien::get_file_extension(path)));
}