#include "MediaWidget.hpp"

#include <QPaintEvent>
#include <QPainter>

#include <ien/math_utils.hpp>

#include <filesystem>

#include "Utils.hpp"

#include <QApplication>

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
    _infoOverlay = new InfoOverlayWidget(this);

    _mainLayout->addWidget(_imageLabel);
    _mainLayout->addWidget(_infoOverlay);

    _imageLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _imageLabel->setMinimumSize(600, 400);
    _imageLabel->setContentsMargins(0, 0, 0, 0);
    _imageLabel->setScaledContents(true);
    _imageLabel->hide();

    _mainLayout->setCurrentWidget(_infoOverlay);

    QTimer::singleShot(1000, [this] { initVideoPlayer(); });
}

void MediaWidget::setMedia(const std::string& source)
{
    qDebug() << "Loading media: " << source;
    _target = source;

    if (_videoPlayer)
    {
        _videoPlayer->hide();
        _videoPlayer->mediaPlayer()->stop();
    }

    _imageLabel->hide();
    if (_animation)
    {
        _animation->stop();
    }

    if (isVideo(_target))
    {
        _currentMediaType = CurrentMediaType::Video;

        if (!_videoPlayer)
        {
            initVideoPlayer();
        }
        _videoPlayer->setMedia(source);
        _videoPlayer->show();
    }
    else if (isAnimation(_target))
    {
        _currentMediaType = CurrentMediaType::Animation;
        _animation = _cachedMediaProxy.getAnimation(source);
        _animation->setCacheMode(QMovie::CacheMode::CacheAll);
        connectAnimationSignals();
        _imageLabel->setPixmap({});
        _animation->start();
        syncAnimationSize();
        _imageLabel->show();
    }
    else if (isImage(_target))
    {
        _currentMediaType = CurrentMediaType::Image;
        _image = _cachedMediaProxy.getImage(source).get().image();
        _imageLabel->setMovie(nullptr);
        updateTransform();
        _imageLabel->show();
    }
}

std::variant<const QImage*, const QMovie*, const QMediaPlayer*> MediaWidget::currentMediaSource()
{
    switch (_currentMediaType)
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
    if (!_videoPlayer)
    {
        return;
    }

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

void MediaWidget::togglePlayPauseVideo()
{
    if (!_videoPlayer)
    {
        return;
    }

    if (_currentMediaType == CurrentMediaType::Video)
    {
        if (_videoPlayer->mediaPlayer()->isPlaying())
        {
            _videoPlayer->mediaPlayer()->pause();
        }
        else
        {
            _videoPlayer->mediaPlayer()->play();
        }
    }
}

void MediaWidget::paintEvent(QPaintEvent* ev)
{
    if (!std::filesystem::exists(_target))
    {
        QPainter painter(this);
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        if (now.count() % 2 == 0)
        {
            painter.setFont(getTextFont(24));
            painter.setBrush(QBrush(QColor("#CCDD00")));
            painter.setPen(QColor("#CCDD00"));
            painter.drawText(QPoint(24, 48), "No media");
        }
        else
        {
            painter.setBrush(QBrush(QColor("#000000")));
            painter.setPen(QColor("#000000"));
            painter.drawRect(16, 48, 100, -100);
        }
        update();
    }
}

void MediaWidget::resizeEvent(QResizeEvent* ev)
{
    if (_currentMediaType == CurrentMediaType::Image)
    {
        updateTransform();
        _imageLabel->setMinimumSize(600, 400);
    }
    else if (_currentMediaType == CurrentMediaType::Animation)
    {
        syncAnimationSize();
        _imageLabel->setMinimumSize(600, 400);
    }
    else if (_currentMediaType == CurrentMediaType::Video && _videoPlayer)
    {
        _videoPlayer->setMinimumSize(1, 1);
    }
    _imageLabel->setFixedSize(ev->size());
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
    updateTransform();
}

void MediaWidget::updateTransform()
{
    if (!_image)
    {
        return;
    }

    _currentTranslation.setX(std::clamp(_currentTranslation.x(), -1.0, 1.0));
    _currentTranslation.setY(std::clamp(_currentTranslation.y(), -1.0, 1.0));

    const auto currentSize = size();
    const auto aspectRatio = static_cast<float>(size().width()) / size().height();

    const auto pixmapSize = _image->size();
    auto sourceRectSize = size().scaled(pixmapSize, Qt::KeepAspectRatioByExpanding) / _currentZoom;
    const auto diff = pixmapSize - sourceRectSize;

    float translateX = ien::
        remap(_currentTranslation.x(), -1, 1, static_cast<float>(-diff.width()) / 2, static_cast<float>(diff.width()) / 2);
    float translateY = ien::
        remap(_currentTranslation.y(), -1, 1, static_cast<float>(-diff.height()) / 2, static_cast<float>(diff.height()) / 2);

    const QRect sourceRect(
        (diff.width() / 2.0f) + translateX,
        (diff.height() / 2.0f) + translateY,
        sourceRectSize.width(),
        sourceRectSize.height());

    auto imageRect = _image->copy(sourceRect);
    const QSize targetSize = imageRect.size().scaled(size() * devicePixelRatio(), Qt::KeepAspectRatio);
    auto pixmap = QPixmap::fromImage(imageRect.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    _imageLabel->setPixmap(pixmap);
    _imageLabel->setScaledContents(true);
}

void MediaWidget::resetTransform()
{
    _currentZoom = 1.0f;
    _currentTranslation = { 0.0f, 0.0f };
    updateTransform();
}

void MediaWidget::increaseVideoSpeed(float amount)
{
    if (_currentMediaType == CurrentMediaType::Video && _videoPlayer)
    {
        const auto rate = std::max(0.0, _videoPlayer->mediaPlayer()->playbackRate() + amount);
        _videoPlayer->mediaPlayer()->setPlaybackRate(rate);
        showMessage("x" + QString::number(rate, 10, 2));
    }
    else if (_currentMediaType == CurrentMediaType::Animation)
    {
        const auto rate = std::max(0.0f, _animation->speed() + (100.0f * amount));
        _animation->setSpeed(rate);
        showMessage("x" + QString::number(rate / 100.f, 10, 2));
    }
}

void MediaWidget::increaseVideoVolume(float amount)
{
    if (_currentMediaType == CurrentMediaType::Video)
    {
        auto* output = _videoPlayer->audioOutput();
        output->setVolume(output->volume() + amount);
    }
}

void MediaWidget::connectAnimationSignals()
{
    connect(_animation.get(), &QMovie::frameChanged, this, [this](int frame) {
        _image = std::make_shared<QImage>(_animation->currentImage());
        updateTransform();
    });
}

void MediaWidget::initVideoPlayer()
{
    if (_videoPlayer)
    {
        return;
    }
    _videoPlayer = new VideoPlayerWidget(this);
    _videoPlayer->setMinimumSize(600, 400);
    _videoPlayer->hide();

    _mainLayout->addWidget(_videoPlayer);
}