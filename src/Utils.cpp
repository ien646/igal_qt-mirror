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
#include <span>
#include <sstream>
#include <unordered_set>

const std::unordered_set<std::string> ANIMATION_EXTENSIONS = { ".gif", ".png", ".webp" };
const std::unordered_set<std::string> IMAGE_EXTENSIONS = { ".png", ".jpg", ".jpeg", ".webp" };
const std::unordered_set<std::string> VIDEO_EXTENSIONS = { ".mkv", ".mp4", ".webm", ".mov" };

constexpr const char* APNG_IDAT_STR = "IDAT";
constexpr const char* APNG_ACTL_STR = "acTL";
constexpr const char* WEBP_ANIM_STR = "ANIM";
constexpr const char* WEBP_ANMF_STR = "ANMF";

bool isPngAnimated(std::span<const std::byte> data)
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

bool isWebpAnimated(std::span<const std::byte> data)
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
    const auto ext = ien::str_tolower(ien::get_file_extension(source));
    if (isAnimation(source))
    {
        return false;
    }
    return IMAGE_EXTENSIONS.count(ext);
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

std::vector<std::string> getUpscaleModels()
{
    static const std::vector<std::string>
        MODELS = { "realesrgan-x4plus-anime", "realesr-animevideov3", "realesrgan-x4plus", "realesrnet-x4plus" };

    return MODELS;
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

    const std::vector<std::string>
        searchPaths = { fileDir + linkDir, fileDir + "../" + linkDir, fileDir + "../../" + linkDir };

    for (const auto& searchPath : searchPaths)
    {
        if (std::filesystem::exists(searchPath))
        {
            const auto copyPath = searchPath + "/" + ien::get_file_name(file);
            bool targetExists = std::filesystem::exists(copyPath);
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
    std::variant<const QImage*, const QMovie*, const QMediaPlayer*> currentSource)
{
    std::stringstream sstr;
    sstr << "&nbsp;&nbsp;<b>Filename</b>: <i>" << ien::get_file_name(file) << "</i><br>";
    sstr << "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>Size</b>: <i>"
         << static_cast<float>(std::filesystem::file_size(file)) / 1000000 << "MB</i><br>";
    if (std::holds_alternative<const QMovie*>(currentSource))
    {
        const QMovie* movie = std::get<const QMovie*>(currentSource);
        sstr << "<b>Dimensions</b>: <i>" << movie->currentImage().size().width() << "x"
             << movie->currentImage().size().width() << "</i><br>";
    }
    else if (std::holds_alternative<const QImage*>(currentSource))
    {
        const QImage* image = std::get<const QImage*>(currentSource);
        sstr << "<b>Dimensions</b>: <i>" << image->width() << "x" << image->height() << "</i><br>";
    }
    else if (std::holds_alternative<const QMediaPlayer*>(currentSource))
    {
        const QMediaPlayer* player = std::get<const QMediaPlayer*>(currentSource);
        if (!player->videoTracks().isEmpty())
        {
            const auto resolution = player->videoTracks()[0].value(QMediaMetaData::Resolution).toSize();
            sstr << "<b>Dimensions</b>: <i>" << resolution.width() << "x" << resolution.height() << "</i><br>";
        }
    }

    return sstr.str();
}

QFont getTextFont(int size)
{
    QFont result("Johto Mono", size);
    result.setStyleHint(QFont::TypeWriter, QFont::StyleStrategy::NoAntialias);
    return result;
}

void runCommand(const std::string& command, const std::vector<std::string>& args, std::function<void(std::string)> messageCallback)
{
    QStringList cmdargs;
    for (const auto& a : args)
    {
        cmdargs.push_back(QString::fromStdString(a));
    }
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

void disableFocusOnChildWidgets(QWidget* widget)
{
    for(auto& child : widget->children())
    {
        if(auto* widget = dynamic_cast<QWidget*>(child))
        {
            widget->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        }
    }
}