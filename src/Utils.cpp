#include "Utils.hpp"

#include <ien/fs_utils.hpp>
#include <ien/io_utils.hpp>
#include <ien/platform.hpp>
#include <ien/str_utils.hpp>

#include <QKeySequence>
#include <QMediaMetaData>
#include <QProcess>

#include <cstdio>
#include <filesystem>
#include <ranges>
#include <span>
#include <sstream>
#include <unordered_set>

const std::unordered_set<std::string> ANIMATION_EXTENSIONS = { ".gif", ".png", ".webp" };
const std::unordered_set<std::string> IMAGE_EXTENSIONS = { ".png", ".jpg", ".jpeg", ".webp" };
const std::unordered_set<std::string> VIDEO_EXTENSIONS = { ".mkv", ".mp4", ".webm", ".mov" };

constexpr auto JXL_EXTENSION = ".jxl";

constexpr auto APNG_IDAT_STR = "IDAT";
constexpr auto APNG_ACTL_STR = "acTL";
constexpr auto WEBP_ANIM_STR = "ANIM";
constexpr auto WEBP_ANMF_STR = "ANMF";

constexpr auto APNG_CHECK_BUFFER_SIZE = 256;

static const bool JXL_SUPPORT = QImageReader::supportedImageFormats().contains("jxl");

bool isPngAnimated(const std::span<const std::byte> data)
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

bool isWebpAnimated(const std::span<const std::byte> data)
{
    const std::string_view sv(reinterpret_cast<const char*>(data.data()), data.size());
    return sv.find(WEBP_ANIM_STR) != std::string_view::npos && sv.find(WEBP_ANMF_STR) != std::string_view::npos;
}

bool hasMediaExtension(const std::string& path)
{
    const auto ext = ien::str_tolower(ien::get_file_extension(path));
    return IMAGE_EXTENSIONS.contains(ext) || ANIMATION_EXTENSIONS.contains(ext) || VIDEO_EXTENSIONS.contains(ext) ||
           (JXL_SUPPORT && ext == JXL_EXTENSION);
}

bool isImage(const std::string& path)
{
    const auto ext = ien::str_tolower(ien::get_file_extension(path));
    if (isAnimation(path))
    {
        return false;
    }
    return IMAGE_EXTENSIONS.contains(ext) || (JXL_SUPPORT && ext == JXL_EXTENSION);
}

bool isAnimation(const std::string& path, const bool shallow)
{
    const std::string extension = ien::str_tolower(ien::get_file_extension(path));
    if (ANIMATION_EXTENSIONS.contains(extension))
    {
        if (shallow)
        {
            return false;
        }

        if (extension == ".png")
        {
            std::array<char, APNG_CHECK_BUFFER_SIZE> data{};
            ien::unique_file_descriptor fd(path, ien::unique_file_descriptor_mode::READ);

            bool first_chunk = true;
            bool eof = false;
            for (;;)
            {
                if (first_chunk)
                {
                    if (fd.read(data.data(), APNG_CHECK_BUFFER_SIZE) < APNG_CHECK_BUFFER_SIZE)
                    {
                        eof = true;
                    }
                    first_chunk = false;
                }
                else
                {
                    // Copy last three characters to the beginning of the buffer to check against new data
                    data[0] = data[APNG_CHECK_BUFFER_SIZE - 3];
                    data[1] = data[APNG_CHECK_BUFFER_SIZE - 2];
                    data[2] = data[APNG_CHECK_BUFFER_SIZE - 1];
                    if (fd.read(data.data(), APNG_CHECK_BUFFER_SIZE - 3) < APNG_CHECK_BUFFER_SIZE - 3)
                    {
                        eof = true;
                    }
                }
                std::string_view sv(data.data(), data.size());
                if (sv.contains(APNG_ACTL_STR))
                {
                    return true;
                }
                if (sv.contains(APNG_IDAT_STR) || eof)
                {
                    return false;
                }
            }
        }
        if (extension == ".webp")
        {
            return isWebpAnimated(*ien::read_file_binary(path));
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
    return VIDEO_EXTENSIONS.contains(ien::str_tolower(ien::get_file_extension(path)));
}

std::unordered_map<int, std::string> getLinksFromFile(const std::string& path)
{
    const auto links_text = ien::read_file_text(path);
    if (!links_text)
    {
        return {};
    }

    std::unordered_map<int, std::string> result;
    auto lines = ien::str_split(*links_text, '\n');

    for (auto& ln : lines)
    {
        ln = ien::str_trim(ln);
        if (ln.empty())
        {
            continue;
        }

        auto segments = ien::str_split(ln, ':');
        if (segments.size() != 2 || segments[0].empty() || segments[1].empty())
        {
            continue;
        }

        QKeySequence seq(QString::fromStdString(segments[0]));
        if (seq.isEmpty())
        {
            continue;
        }

        result.emplace(seq[0].key(), segments[1]);
    }
    return result;
}

std::vector<std::string> getImageUpscaleModels()
{
    static const std::vector<std::string>
        MODELS = { "realesrgan-x4plus-anime", "realesr-animevideov3", "realesrgan-x4plus", "realesrnet-x4plus" };

    return MODELS;
}

const std::map<std::string, std::pair<std::string, unsigned int>> VIDEO_UPSCALE_MODEL_MAP = {
    { "(x2) realesr-animevideov3", { "realesr-animevideov3", 2 } },
    { "(x4) realesr-animevideov3", { "realesr-animevideov3", 4 } },
    { "(x4) realesrgan-plus-anime", { "realesrgan-plus-anime", 4 } },
    { "(x4) realesrgan-plus", { "realesrgan-plus", 4 } }
};

std::vector<std::string> getVideoUpscaleModels()
{
    const auto& keys = std::views::keys(VIDEO_UPSCALE_MODEL_MAP);
    return { keys.begin(), keys.end() };
}

std::pair<std::string, unsigned int> videoUpscaleModelToStringAndFactor(const std::string& str)
{
    return VIDEO_UPSCALE_MODEL_MAP.at(str);
}

CopyFileToLinkDirResult copyFileToLinkDir(const std::string& file, const std::string& linkDir)
{
    if (!std::filesystem::exists(file))
    {
        return CopyFileToLinkDirResult::SourceFileNotFound;
    }

    auto fileDir = ien::get_file_directory(file);
    if (!fileDir.ends_with('/') && !fileDir.ends_with('\\'))
    {
        fileDir += std::filesystem::path::preferred_separator;
    }

    const std::vector searchPaths = { fileDir + linkDir, fileDir + "../" + linkDir, fileDir + "../../" + linkDir };

    for (const auto& searchPath : searchPaths)
    {
        if (std::filesystem::exists(searchPath))
        {
            const auto copyPath = searchPath + "/" + ien::get_file_name(file);
            const bool targetExists = std::filesystem::exists(copyPath);
            try
            {
                const auto sourceMtime = ien::get_file_mtime(file);
                std::filesystem::copy_file(file, copyPath, std::filesystem::copy_options::overwrite_existing);
                ien::set_file_mtime(copyPath, sourceMtime);
                return targetExists ? CopyFileToLinkDirResult::Overwriten : CopyFileToLinkDirResult::CreatedNew;
            }
            catch ([[maybe_unused]] std::exception&)
            {
                return CopyFileToLinkDirResult::Error;
            }
        }
    }

    return CopyFileToLinkDirResult::TargetDirNotFound;
}

std::string getFileInfoString(
    const std::string& file,
    const std::variant<const QImage*, const QMovie*, const QMediaPlayer*> currentSource)
{
    std::stringstream sstr;
    sstr << "&nbsp;&nbsp;<b>Filename</b>: <i>" << ien::get_file_name(file) << "</i><br>";
    sstr << "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>Size</b>: <i>"
         << static_cast<float>(std::filesystem::file_size(file)) / 1000000 << "MB</i><br>";
    if (std::holds_alternative<const QMovie*>(currentSource))
    {
        const auto movie = std::get<const QMovie*>(currentSource);
        sstr << "<b>Dimensions</b>: <i>" << movie->currentImage().size().width() << "x"
             << movie->currentImage().size().width() << "</i><br>";
    }
    else if (std::holds_alternative<const QImage*>(currentSource))
    {
        const auto image = std::get<const QImage*>(currentSource);
        sstr << "<b>Dimensions</b>: <i>" << image->width() << "x" << image->height() << "</i><br>";
    }
    else if (std::holds_alternative<const QMediaPlayer*>(currentSource))
    {
        const auto player = std::get<const QMediaPlayer*>(currentSource);
        if (!player->videoTracks().isEmpty())
        {
            const auto resolution = player->videoTracks()[0].value(QMediaMetaData::Resolution).toSize();
            sstr << "<b>Dimensions</b>: <i>" << resolution.width() << "x" << resolution.height() << "</i><br>";
        }
    }

    return sstr.str();
}

QFont getTextFont(const int size)
{
    QFont result("Johto Mono", size);
    result.setStyleHint(QFont::TypeWriter, QFont::StyleStrategy::NoAntialias);
    return result;
}

void runCommand(
    const std::string& command,
    const std::vector<std::string>& args,
    const std::function<void(std::string)>& messageCallback)
{
    printf("Running command: %s ", command.c_str());
    QStringList cmdargs;
    for (const auto& a : args)
    {
        printf("%s ", a.c_str());
        cmdargs.push_back(QString::fromStdString(a));
    }
    printf("\n");
    QProcess proc;
    proc.start(QString::fromStdString(command), cmdargs);

    while (!proc.waitForFinished(10))
    {
        auto message = proc.readAllStandardOutput() + proc.readAllStandardError();
        if (message.endsWith("\n"))
        {
            message = message.first(message.size() - 1);
        }
        if (!message.isEmpty())
        {
            messageCallback(message.toStdString());
        }
    }
}

void disableFocusOnChildWidgets(const QWidget* widget)
{
    for (auto& child : widget->children())
    {
        if (auto* child_widget = dynamic_cast<QWidget*>(child))
        {
            child_widget->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        }
    }
}