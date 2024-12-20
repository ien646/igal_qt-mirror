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
    QTimer* _timer = nullptr;
};