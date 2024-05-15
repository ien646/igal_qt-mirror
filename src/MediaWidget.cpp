#include "MediaWidget.hpp"

#include <QPaintEvent>
#include <QPainter>

#include <ien/math_utils.hpp>

#include <filesystem>

#include "Utils.hpp"

MediaWidget::MediaWidget(QWidget* parent)
    : QWidget(parent)
    , _cachedMediaProxy(1024)
{
    setAutoFillBackground(true);

    _mainLayout = new QStackedLayout(this);
    _mainLayout->setStackingMode(QStackedLayout::StackAll);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    _mainLayout->setAlignment(Qt::AlignmentFlag::AlignVCenter | Qt::AlignmentFlag::AlignHCenter);
    setLayout(_mainLayout);

    _imageLabel = new QLabel(this);
    _videoPlayer = new VideoPlayerWidget(this);
    _infoOverlay = new InfoOverlayWidget(this);

    _mainLayout->addWidget(_imageLabel);
    _mainLayout->addWidget(_videoPlayer);
    _mainLayout->addWidget(_infoOverlay);

    _imageLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _imageLabel->setMinimumSize(600, 400);
    _imageLabel->setContentsMargins(0, 0, 0, 0);
    _imageLabel->hide();

    _videoPlayer->setMinimumSize(600, 400);
    _videoPlayer->hide();

    _mainLayout->setCurrentWidget(_infoOverlay);
}

void MediaWidget::setMedia(const std::string& source)
{
    _target = source;

    _videoPlayer->hide();
    _videoPlayer->mediaPlayer()->stop();
    _imageLabel->hide();

    if (isVideo(_target))
    {
        _currentMedia = CurrentMediaType::Video;
        _videoPlayer->setMedia(source);
        _videoPlayer->show();
    }
    else if (isAnimation(_target))
    {
        _currentMedia = CurrentMediaType::Animation;
        _animation = _cachedMediaProxy.getAnimation(source);
        _imageLabel->setPixmap(QPixmap());
        _imageLabel->setMovie(_animation.get());
        syncAnimationSize();
        _animation->start();
        _imageLabel->show();
    }
    else if (isImage(_target))
    {
        _currentMedia = CurrentMediaType::Image;
        _image = _cachedMediaProxy.getImage(source);
        _pixmap = std::make_unique<QPixmap>(QPixmap::fromImage(*_image));
        _imageLabel->setMovie(nullptr);
        _imageLabel->setPixmap(_pixmap->scaled(size(), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        _imageLabel->show();
    }
}

std::variant<const QImage*, const QMovie*, const QMediaPlayer*> MediaWidget::currentMediaSource()
{
    switch (_currentMedia)
    {
    case CurrentMediaType::Image:
        return _image.get();
    case CurrentMediaType::Animation:
        return _animation.get();
    case CurrentMediaType::Video:
        return _videoPlayer->mediaPlayer();
    }
    return {};
}

bool MediaWidget::isInfoShown()
{
    return _infoOverlay->isInfoShown();
}

void MediaWidget::showMessage(const QString& message)
{
    _infoOverlay->showMessage(message);
}

void MediaWidget::showInfo(const QString& info)
{
    _infoOverlay->showInfo(info);
}

void MediaWidget::hideInfo()
{
    _infoOverlay->hideInfo();
}

void MediaWidget::toggleMute()
{
    QAudioOutput* audioOutput = _videoPlayer->audioOutput();

    if (!audioOutput->isMuted())
    {
        _volumeBeforeMute = audioOutput->volume();
        audioOutput->setVolume(0);
        audioOutput->setMuted(true);
    }
    else
    {
        audioOutput->setVolume(_volumeBeforeMute);
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
    if (_currentMedia == CurrentMediaType::Image)
    {
        _imageLabel->setPixmap(
            _pixmap->scaled(ev->size(), Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        _imageLabel->setFixedSize(ev->size());
        _imageLabel->setMinimumSize(600, 400);
    }
    else if (_currentMedia == CurrentMediaType::Animation)
    {
        syncAnimationSize();
    }
    else if (_currentMedia == CurrentMediaType::Video)
    {
        _videoPlayer->setFixedSize(ev->size());
        _videoPlayer->setMinimumSize(1, 1);
    }
}

CachedMediaProxy& MediaWidget::cachedMediaProxy()
{
    return _cachedMediaProxy;
}

void MediaWidget::zoomIn(float amount)
{
    _currentZoom += amount;
    updateTransform();
}

void MediaWidget::zoomOut(float amount)
{
    _currentZoom = std::max(1.0f, _currentZoom - amount);
    updateTransform();
}

void MediaWidget::translateLeft(float amount)
{
    _currentTranslation -= QPointF{ amount, 0 };
    updateTransform();
}

void MediaWidget::translateRight(float amount)
{
    _currentTranslation += QPointF{ amount, 0 };
    updateTransform();
}

void MediaWidget::translateUp(float amount)
{
    _currentTranslation += QPointF{ 0, amount };
    updateTransform();
}

void MediaWidget::translateDown(float amount)
{
    _currentTranslation -= QPointF{ 0, amount };
    updateTransform();
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

void MediaWidget::updateTransform()
{
    if (_currentMedia != CurrentMediaType::Image)
    {
        return;
    }

    _currentTranslation.setX(std::clamp(_currentTranslation.x(), -1.0, 1.0));
    _currentTranslation.setY(std::clamp(_currentTranslation.y(), -1.0, 1.0));

    const auto aspectRatio = static_cast<float>(size().width()) / size().height();

    const auto pixmapSize = _pixmap->size();
    auto sourceRectSize = (_pixmap->size() / _currentZoom);
    sourceRectSize.setWidth(sourceRectSize.height() * aspectRatio);
    sourceRectSize.setHeight(sourceRectSize.width() / aspectRatio);
    const auto diff = pixmapSize - sourceRectSize;

    float translateX = ien::remap(_currentTranslation.x(), -1, 1, -diff.width() / 2, diff.width() / 2);
    float translateY = ien::remap(_currentTranslation.y(), -1, 1, -diff.height() / 2, diff.height() / 2);

    const QRect sourceRect(
        (diff.width() / 2.0f) + translateX,
        (diff.height() / 2.0f) + translateY,
        sourceRectSize.width(),
        sourceRectSize.height());

    _imageLabel->setPixmap(_pixmap->copy(sourceRect).scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MediaWidget::resetTransform()
{
    _currentZoom = 1.0f;
    _currentTranslation = { 0.0f, 0.0f };
    updateTransform();
}