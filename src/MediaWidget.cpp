#include "MediaWidget.hpp"

#include <QPaintEvent>
#include <QPainter>

#include <ien/math_utils.hpp>

#include <filesystem>

#include "../cmake-build-release/_deps/libien-src/lib/include/ien/bits/str_utils/tolowerupper.hpp"
#include "Utils.hpp"
#include "ien/fs_utils.hpp"

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
        auto real_source = source;
        if (ien::str_tolower(_target).ends_with(".png"))
        {
            real_source = std::format("/tmp/igal_qt{}.gif", std::filesystem::absolute(_target).string());
            if (std::filesystem::file_size(real_source) == 0)
            {
                _currentMediaType = CurrentMediaType::Image;
                _image = _cachedMediaProxy.getImage(source).get().image();
                _imageLabel->setMovie(nullptr);
                updateTransform();
                _imageLabel->show();
                return;
            }
            if (!std::filesystem::exists(real_source))
            {
                std::filesystem::create_directories(ien::get_file_directory(real_source));
                runCommand("ffmpeg", { "-n", "-i", _target, real_source }, []([[maybe_unused]] const std::string&) {});
                if (!std::filesystem::exists(real_source) || std::filesystem::file_size(real_source) == 0)
                {
                    _currentMediaType = CurrentMediaType::Image;
                    _image = _cachedMediaProxy.getImage(source).get().image();
                    _imageLabel->setMovie(nullptr);
                    updateTransform();
                    _imageLabel->show();
                    return;
                }
            }
        }
        _currentMediaType = CurrentMediaType::Animation;
        _animation = _cachedMediaProxy.getAnimation(real_source);
        _animation->setCacheMode(QMovie::CacheMode::CacheAll);
        std::printf("Frame count: %d\n", _animation->frameCount());
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

std::variant<const QImage*, const QMovie*, const QMediaPlayer*> MediaWidget::currentMediaSource() const
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

bool MediaWidget::isInfoShown() const
{
    return _infoOverlay->isInfoShown();
}

void MediaWidget::showMessage(const QString& message) const
{
    _infoOverlay->showMessage(message);
}

void MediaWidget::showInfo(const QString& info) const
{
    _infoOverlay->showInfo(info);
}

void MediaWidget::hideInfo() const
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

void MediaWidget::togglePlayPauseVideo() const
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

void MediaWidget::updateTransform()
{
    if (!_image)
    {
        return;
    }

    _currentTranslation.setX(std::clamp(_currentTranslation.x(), -1.0, 1.0));
    _currentTranslation.setY(std::clamp(_currentTranslation.y(), -1.0, 1.0));

    const auto pixmapSize = _image->size();
    const auto sourceRectSize = size().scaled(pixmapSize, Qt::KeepAspectRatioByExpanding) / _currentZoom;
    const auto diff = pixmapSize - sourceRectSize;

    const float translateX = ien::
        remap(_currentTranslation.x(), -1, 1, static_cast<float>(-diff.width()) / 2, static_cast<float>(diff.width()) / 2);
    const float translateY = ien::
        remap(_currentTranslation.y(), -1, 1, static_cast<float>(-diff.height()) / 2, static_cast<float>(diff.height()) / 2);

    const QRect sourceRect(
        (diff.width() / 2.0f) + translateX,
        (diff.height() / 2.0f) + translateY,
        sourceRectSize.width(),
        sourceRectSize.height());

    const auto imageRect = _image->copy(sourceRect);
    const QSize targetSize = imageRect.size().scaled(size() * devicePixelRatio(), Qt::KeepAspectRatio);
    const auto pixmap = QPixmap::fromImage(imageRect.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    _imageLabel->setPixmap(pixmap);
    _imageLabel->setScaledContents(true);
}

void MediaWidget::paintEvent(QPaintEvent* ev)
{
    if (!std::filesystem::exists(_target))
    {
        QPainter painter(this);
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch());
        if (now.count() % 2 == 0)
        {
            painter.setFont(getTextFont(24));
            painter.setBrush(QBrush(QColor(0xCCDD00u)));
            painter.setPen(QColor(0xCCDD00u));
            painter.drawText(QPoint(24, 48), "No media");
        }
        else
        {
            painter.setBrush(QBrush(QColor(0x000000u)));
            painter.setPen(QColor(0x000000u));
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

void MediaWidget::zoomIn(const float amount)
{
    _currentZoom += amount;
    updateTransform();
}

void MediaWidget::zoomOut(const float amount)
{
    _currentZoom = std::max(1.0f, _currentZoom - amount);
    updateTransform();
}

void MediaWidget::translateLeft(const float amount)
{
    _currentTranslation -= QPointF{ amount, 0 };
    updateTransform();
}

void MediaWidget::translateRight(const float amount)
{
    _currentTranslation += QPointF{ amount, 0 };
    updateTransform();
}

void MediaWidget::translateUp(const float amount)
{
    _currentTranslation += QPointF{ 0, amount };
    updateTransform();
}

void MediaWidget::translateDown(const float amount)
{
    _currentTranslation -= QPointF{ 0, amount };
    updateTransform();
}

void MediaWidget::resetTransform()
{
    _currentZoom = 1.0f;
    _currentTranslation = { 0.0f, 0.0f };
    updateTransform();
}

void MediaWidget::increaseVideoSpeed(const float amount) const
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

void MediaWidget::increaseVideoVolume(const float amount) const
{
    if (_currentMediaType == CurrentMediaType::Video)
    {
        auto* output = _videoPlayer->audioOutput();
        output->setVolume(output->volume() + amount);
    }
}

void MediaWidget::syncAnimationSize()
{
    updateTransform();
}

void MediaWidget::connectAnimationSignals()
{
    connect(_animation.get(), &QMovie::frameChanged, this, [this]([[maybe_unused]] int frame) {
        _image = std::make_shared<QImage>(_animation->currentImage());
        updateTransform();
    });

    connect(_animation.get(), &QMovie::finished, _animation.get(), &QMovie::start);
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