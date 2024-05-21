#pragma once

#include <QFont>
#include <QImage>
#include <QMediaPlayer>
#include <QMovie>
#include <QWidget>

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

bool hasMediaExtension(const std::string& path);
bool isImage(const std::string& path);
bool isAnimation(const std::string& path);
bool isVideo(const std::string& path);
std::unordered_map<int, std::string> getLinksFromFile(const std::string& path);
std::vector<std::string> getUpscaleModels();

enum class CopyFileToLinkDirResult
{
    CreatedNew,
    Overwriten,
    SourceFileNotFound,
    TargetDirNotFound,
    Error
};

[[nodiscard]] CopyFileToLinkDirResult copyFileToLinkDir(const std::string& file, const std::string& linkDir);

std::string getFileInfoString(const std::string& file, std::variant<const QImage*, const QMovie*, const QMediaPlayer*> currentSource);

QFont getTextFont(int size = 8);

void runCommand(const std::string& command, const std::vector<std::string>& args, std::function<void(std::string)> messageCallback);

void disableFocusOnChildWidgets(QWidget* widget);