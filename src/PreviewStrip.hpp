#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QWidget>

#include "CachedMediaProxy.hpp"

class PreviewStrip : public QWidget
{
public:
    PreviewStrip(CachedMediaProxy& cachedMediaProxy, QWidget* parent);

    void loadImages(const std::vector<std::string>& paths);

private:
    CachedMediaProxy& _cachedMediaProxy;
    QHBoxLayout* _layout = nullptr;
    std::vector<QLabel*> _labels;
    std::vector<QMovie*> _movies;
    std::vector<bool> _completed;
    std::vector<std::string> _paths;
    QTimer* _timer = nullptr;
};