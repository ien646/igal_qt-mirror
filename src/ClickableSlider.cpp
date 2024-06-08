#include "ClickableSlider.hpp"

#include <ien/math_utils.hpp>

void ClickableSlider::mousePressEvent(QMouseEvent* ev)
{
    constexpr int MARGIN_PIXELS = 16;

    const auto currentPos = ien::remap(value(), 0, maximum(), 0, width());
    const auto mousePos = ev->pos().x();

    if(std::abs(currentPos - mousePos) <= MARGIN_PIXELS)
    {
        QSlider::mousePressEvent(ev);
        return;
    }

    float sliderPos = ien::remap<float>(mousePos, 0.0f, width(), 0.0f, 1.0f);
    setValue(sliderPos * maximum());

    emit sliderMoved(value());
}