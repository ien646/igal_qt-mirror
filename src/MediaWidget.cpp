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

    _main_layout->addWidget(_image_label);
    _main_layout->addWidget(_video_player);

    _image_label->setAlignment(Qt::AlignCenter);
    _image_label->setMinimumSize(600, 400);
    _image_label->hide();

    _video_player->setMinimumSize(600, 400);
    _video_player->hide();
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
        _animation->start();
        _image_label->show();
        syncAnimationSize();
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

void MediaWidget::paintEvent(QPaintEvent* ev)
{
    if (!std::filesystem::exists(_target))
    {
        QPainter painter(this);
        painter.setFont(QFont("Pokemon Classic", 24));
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