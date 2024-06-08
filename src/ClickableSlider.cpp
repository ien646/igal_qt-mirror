#include "ClickableSlider.hpp"

#include <ien/math_utils.hpp>

#include <QPainter>

void ClickableSlider::mousePressEvent(QMouseEvent* ev)
{
    constexpr int MARGIN_PIXELS = 16;

    const auto currentPos = ien::remap(value(), 0, maximum(), 0, width());
    const auto mousePos = ev->pos().x();

    const float sliderPos = ien::remap<float>(mousePos, 0.0f, width(), 0.0f, 1.0f);

    emit sliderMoved(sliderPos * maximum());
    QSlider::mousePressEvent(ev);
}

void ClickableSlider::paintEvent(QPaintEvent* ev)
{
    QPainter painter(this);
    const auto seekPos = ien::remap(value(), 0, maximum(), 0, width());

    // Background
    painter.setBrush(QBrush("#4466CA", Qt::FDiagPattern));
    painter.setPen(Qt::transparent);
    painter.drawRect(this->rect());

    // Filled segment    
    const QRect filledRect(0, 0, seekPos, height());
    painter.setBrush(QBrush("#44CA66"));
    painter.drawRect(filledRect);

    // Seek bar
    const QRect barRect(seekPos - 1, 0, 2, height());
    painter.setBrush(QBrush("#FFFF00"));
    painter.drawRect(barRect);

    // Border
    painter.setBrush(Qt::BrushStyle::NoBrush);
    painter.setPen(QPen(QBrush(QColor("#ffffff")), 2, Qt::PenStyle::SolidLine, Qt::PenCapStyle::FlatCap, Qt::PenJoinStyle::BevelJoin));
    painter.drawRect(this->rect());
}