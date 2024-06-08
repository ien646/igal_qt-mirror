#pragma once

#include <QMouseEvent>
#include <QSlider>

class ClickableSlider : public QSlider
{
public:
    using QSlider::QSlider;

private:
    void mousePressEvent(QMouseEvent* ev) override;
};