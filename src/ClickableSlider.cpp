#include "ClickableSlider.hpp"

#include <ien/math_utils.hpp>

#include <QPainter>

void ClickableSlider::mousePressEvent(QMouseEvent* ev)
{
    const auto mousePos = ev->pos().x();
    const float sliderPos = ien::remap(mousePos, 0.0f, width() - contentsMargins().left() - contentsMargins().right(), 0.0f, 1.0f);
    emit sliderPressed();
    emit sliderMoved(sliderPos * maximum());
    setValue(sliderPos * maximum());
    _moving = true;
}

void ClickableSlider::mouseMoveEvent(QMouseEvent* ev)
{
    if (_moving)
    {
        const auto mousePos = ev->pos().x();
        const float sliderPos = ien::remap(mousePos, 0.0f, width() - contentsMargins().left() - contentsMargins().right(), 0.0f, 1.0f);
        emit sliderMoved(sliderPos * maximum());
        setValue(sliderPos * maximum());
    }
}

void ClickableSlider::mouseReleaseEvent(QMouseEvent* ev)
{
    _moving = false;
    emit sliderReleased();
}

void ClickableSlider::paintEvent(QPaintEvent* ev)
{
    QPainter painter(this);
    const auto
        seekPos = ien::remap(value(), 0, maximum(), 0, width() - contentsMargins().left() - contentsMargins().right());

    // Background
    painter.setBrush(QBrush("#4466CA"));
    painter.setPen(Qt::transparent);
    painter.drawRect(this->rect());

    // Filled segment
    const QRect filledRect(0, 0, seekPos, height());
    painter.setBrush(QBrush("#CACA69"));
    painter.drawRect(filledRect);

    // Seek bar
    const QRect barRect(seekPos - 1, 0, 2, height());
    painter.setBrush(QBrush("#FFFF00"));
    painter.drawRect(barRect);

    // Border
    painter.setBrush(Qt::BrushStyle::NoBrush);
    painter.setPen(
        QPen(QBrush(QColor("#ffffff")), 2, Qt::PenStyle::SolidLine, Qt::PenCapStyle::FlatCap, Qt::PenJoinStyle::BevelJoin));
    painter.drawRect(this->rect());
}