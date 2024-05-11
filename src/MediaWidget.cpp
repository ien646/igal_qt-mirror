#include "MediaWidget.hpp"

#include <QPaintEvent>
#include <QPainter>

#include <filesystem>

#include "Utils.hpp"

MediaWidget::MediaWidget(QWidget* parent)
    : QWidget(parent)
{
    _main_layout = new QStackedLayout(this);
    _main_layout->setStackingMode(QStackedLayout::StackAll);
    setLayout(_main_layout);

    _image_label = new QLabel(this);
    _video_player = new VideoPlayerWidget(this);
    _info_overlay = new InfoOverlayWidget(this);

    _main_layout->addWidget(_image_label);
    _main_layout->addWidget(_video_player);
    _main_layout->addWidget(_info_overlay);

    _image_label->setAlignment(Qt::AlignCenter);
    _image_label->setMinimumSize(600, 400);
    _image_label->hide();

    _video_player->setMinimumSize(600, 400);
    _video_player->hide();

    _main_layout->setCurrentWidget(_info_overlay);
}

void MediaWidget::setMedia(const std::string& source)
{
    _target = source;

    _video_player->hide();
    _image_label->hide();

    if (isVideo(_target))
    {
        _current_media = CurrentMediaType::Video;
        _video_player->setMedia(source);
        _video_player->show();
    }
    else if (isAnimation(_target))
    {
        _current_media = CurrentMediaType::Animation;
        _animation = std::make_unique<QMovie>(QString::fromStdString(source));
        _image_label->setPixmap(QPixmap());
        _image_label->setMovie(_animation.get());
        syncAnimationSize();
        _animation->start();
        _image_label->show();
    }
    else if (isImage(_target))
    {
        _current_media = CurrentMediaType::Image;
        _image = std::make_unique<QImage>(QString::fromStdString(source));
        _pixmap = std::make_unique<QPixmap>(QPixmap::fromImage(*_image));
        _image_label->setMovie(nullptr);
        _image_label->setPixmap(
            _pixmap->scaled(size(), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        _image_label->show();
    }
}

std::variant<const QImage*, const QMovie*, const QMediaPlayer*> MediaWidget::currentMediaSource()
{
    switch(_current_media)
    {
        case CurrentMediaType::Image:
            return _image.get();
        case CurrentMediaType::Animation:
            return _animation.get();
        case CurrentMediaType::Video:
            return _video_player->mediaPlayer();
    }
    return {};
}

bool MediaWidget::isInfoShown()
{
    return _info_overlay->isInfoShown();
}

void MediaWidget::showMessage(const QString& message)
{
    _info_overlay->showMessage(message);
}

void MediaWidget::showInfo(const QString& info)
{
    _info_overlay->showInfo(info);
}

void MediaWidget::hideInfo()
{
    _info_overlay->hideInfo();
}

void MediaWidget::toggleMute()
{
    QAudioOutput* audioOutput = _video_player->audioOutput();

    if(!audioOutput->isMuted())
    {
        _volume_before_mute = audioOutput->volume();
        audioOutput->setVolume(0);
        audioOutput->setMuted(true);
    }
    else
    {
        audioOutput->setVolume(_volume_before_mute);
        audioOutput->setMuted(false);
    }
}

void MediaWidget::paintEvent(QPaintEvent* ev)
{
    if (!std::filesystem::exists(_target))
    {
        QPainter painter(this);
        painter.setFont(getTextFont(24));
        painter.setBrush(QBrush(QColor("#CCDD00")));
        painter.setPen(QColor("#CCDD00"));
        painter.drawText(QPoint(16, 48), "No media");
    }
}

void MediaWidget::resizeEvent(QResizeEvent* ev)
{
    if (_current_media == CurrentMediaType::Image)
    {
        _image_label->setPixmap(
            _pixmap->scaled(ev->size(), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        _image_label->setFixedSize(ev->size());
        _image_label->setMinimumSize(600, 400);
    }
    else if (_current_media == CurrentMediaType::Animation)
    {
        syncAnimationSize();
    }
    else if (_current_media == CurrentMediaType::Video)
    {
        _video_player->setFixedSize(ev->size());
        _video_player->setMinimumSize(1, 1);
    }
}

void MediaWidget::syncAnimationSize()
{
    const auto currentSize = _animation->scaledSize();
    const auto aspectRatio = static_cast<float>(currentSize.width()) / currentSize.height();

    if (static_cast<float>(size().width()) * aspectRatio <= size().height())
    {
        _animation->setScaledSize(
            QSize{ size().width(), static_cast<int>(static_cast<float>(size().width()) * aspectRatio) });
    }
    else
    {
        _animation->setScaledSize(
            QSize{ static_cast<int>(static_cast<float>(size().height()) * aspectRatio), size().height() });
    }
}