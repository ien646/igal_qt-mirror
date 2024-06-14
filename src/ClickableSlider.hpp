#pragma once

#include <QMouseEvent>
#include <QSlider>

class ClickableSlider : public QSlider
{
public:
    using QSlider::QSlider;

private:
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void paintEvent(QPaintEvent* ev) override;

    bool _moving = false;
};