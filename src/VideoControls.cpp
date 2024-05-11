#include "VideoControls.hpp"

#include <QLabel>
#include <QMouseEvent>

#include "Utils.hpp"

VideoControls::VideoControls(QWidget* parent)
    : QWidget(parent)
{
    this->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    this->setMouseTracking(true);
    this->setStyleSheet("background-color: rgba(0,0,0,0)");

    _main_layout = new QHBoxLayout(this);
    _main_layout->setAlignment(Qt::AlignmentFlag::AlignBottom);
    this->setLayout(_main_layout);

    _seek_position_label = new QLabel(this);
    _volume_slider = new QSlider(Qt::Orientation::Horizontal, this);
    _seek_slider = new QSlider(Qt::Orientation::Horizontal, this);
    _play_label = new QLabel(this);
    _pause_label = new QLabel(this);
    _volume_label = new QLabel(this);

    _seek_position_label->setFont(getTextFont());
    _seek_position_label->setStyleSheet("QLabel{color:#ffffff;}");
    _seek_position_label->setText("00*00*00");

    _seek_slider->setMinimum(0);
    _seek_slider->setMaximum(1000);

    _volume_label->setFont(getTextFont());
    _volume_slider->setMinimum(0);
    _volume_slider->setMaximum(100);
    _volume_slider->setMaximumWidth(100);
    _volume_label->setText("Volume: 100%");

    _play_label->setPixmap(
        QPixmap::fromImage(QImage(":/icon_play.png"))
            .scaled(24, 24, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));

    _pause_label->setPixmap(
        QPixmap::fromImage(QImage(":/icon_pause.png"))
            .scaled(24, 24, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));

    _play_label->setMouseTracking(true);
    _pause_label->setMouseTracking(true);

    _main_layout->addSpacing(16);
    _main_layout->addWidget(_play_label);
    _main_layout->addWidget(_pause_label);
    _main_layout->addWidget(_seek_position_label);
    _main_layout->addWidget(_seek_slider);
    _main_layout->addSpacing(16);
    _main_layout->addWidget(_volume_label);
    _main_layout->addWidget(_volume_slider);
    _main_layout->addSpacing(16);

    connect(_volume_slider, &QSlider::valueChanged, this, [this](int percent) {
        emit volumeChanged(percent);
        _volume_label->setText(QString::fromStdString(std::format("Volume: {}%", percent)));        
    });
    _volume_slider->setValue(50);

    connect(_seek_slider, &QSlider::sliderMoved, this, [this](int pos) {
        emit videoPositionChanged(static_cast<float>(pos) / _seek_slider->maximum());
        updateSeekLabel((static_cast<float>(pos) / _seek_slider->maximum()) * _current_video_duration);
    });

    connect(_seek_slider, &QSlider::sliderPressed, this, [this] { emit seekSliderClicked(); });
    connect(_seek_slider, &QSlider::sliderReleased, this, [this] { emit seekSliderReleased(); });
    connect(_volume_slider, &QSlider::sliderPressed, this, [this] { emit volumeSliderClicked(); });
    connect(_volume_slider, &QSlider::sliderReleased, this, [this] { emit volumeSliderReleased(); });

    for (const auto& child : children())
    {
        auto widget = dynamic_cast<QWidget*>(child);
        if (widget)
        {
            widget->setMouseTracking(true);
        }
    }
}

void VideoControls::mousePressEvent(QMouseEvent* ev)
{
    if (ev->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        _clicking = true;
        updateButtonStyles();
    }
}

void VideoControls::mouseReleaseEvent(QMouseEvent* ev)
{
    if (!ev->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        _clicking = false;
        updateButtonStyles();

        if (_play_label->underMouse())
        {
            emit playClicked();
        }
        else if (_pause_label->underMouse())
        {
            emit pauseClicked();
        }
    }
}

void VideoControls::mouseMoveEvent(QMouseEvent* ev)
{
    updateButtonStyles();
}

void VideoControls::setCurrentVideoDuration(int64_t ms)
{
    _current_video_duration = ms;
}

void VideoControls::setCurrentVideoPosition(int64_t posMs)
{
    _seek_slider->setSliderPosition((static_cast<float>(posMs) / _current_video_duration) * _seek_slider->maximum());
    updateSeekLabel(posMs);
}

void VideoControls::setCurrentVolume(int percent)
{
    _volume_slider->setSliderPosition(percent);
}

void VideoControls::updateButtonStyles()
{
    constexpr const char* standard_stylesheet =
        "QLabel{ padding: 2px; border-radius: 2px; background-color: rgba(0, "
        "0, 0, 0); }";
    constexpr const char* hover_stylesheet =
        "QLabel{ padding: 2px; border-radius: 2px; background-color: "
        "rgba(255,255,255,50); }";
    constexpr const char* active_stylesheet =
        "QLabel{ padding: 2px; border-radius: 2px; background-color: "
        "rgba(255,255,100,150); }";

    if (_pause_label->underMouse())
    {
        _pause_label->setStyleSheet(_clicking ? active_stylesheet : hover_stylesheet);
    }
    else
    {
        _pause_label->setStyleSheet(standard_stylesheet);
    }

    if (_play_label->underMouse())
    {
        _play_label->setStyleSheet(_clicking ? active_stylesheet : hover_stylesheet);
    }
    else
    {
        _play_label->setStyleSheet(standard_stylesheet);
    }
}

void VideoControls::updateSeekLabel(int64_t posMs)
{
    const auto ms = std::chrono::milliseconds(posMs);
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(ms);
    const auto h = std::chrono::duration_cast<std::chrono::hours>(ms);

    _seek_position_label->setText(QString::fromStdString(
        std::format("{:0<2}:{:0<2}:{:0<2}:{:0<2}", h.count() % 100, m.count() % 60, s.count() % 60, (ms.count() % 1000) / 10)));
}