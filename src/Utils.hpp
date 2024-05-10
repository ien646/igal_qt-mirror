#pragma once

#include <string>

bool hasMediaExtension(const std::string& path);
bool isImage(const std::string& path);
bool isAnimation(const std::string& path);
bool isVideo(const std::string& path);