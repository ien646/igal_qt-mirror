#pragma once

#include <QImage>
#include <QLabel>
#include <QMovie>
#include <QStackedLayout>
#include <QTimer>

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>

#include "CachedMediaProxy.hpp"
#include "InfoOverlayWidget.hpp"
#include "VideoPlayerWidget.hpp"

enum class CurrentMediaType
{
    Image,
    Animation,
    Video
};

class MediaWidget : public QWidget
{
    Q_OBJECT

public:
    MediaWidget(QWidget* parent = nullptr);

    void setMedia(const std::string& source);

    std::variant<const QImage*, const QMovie*, const QMediaPlayer*> currentMediaSource();
    bool isInfoShown();
    void showMessage(const QString& message);
    void showInfo(const QString& info);
    void hideInfo();
    void toggleMute();

    void paintEvent(QPaintEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

    CachedMediaProxy& cachedMediaProxy();

    void zoomIn(float amount);
    void zoomOut(float amount);
    void translateLeft(float amount);
    void translateRight(float amount);
    void translateUp(float amount);
    void translateDown(float amount);
    void resetTransform();

private:
    std::string _target;
    bool _isVideo;
    float _volumeBeforeMute;

    QStackedLayout* _mainLayout = nullptr;

    CurrentMediaType _currentMedia;

    CachedMediaProxy _cachedMediaProxy;
    std::shared_ptr<QImage> _image;
    std::shared_ptr<QMovie> _animation;

    QLabel* _imageLabel = nullptr;
    VideoPlayerWidget* _videoPlayer = nullptr;
    InfoOverlayWidget* _infoOverlay = nullptr;

    float _currentZoom = 1.0f;
    QPointF _currentTranslation = { 0.0f, 0.0f };

    void syncAnimationSize();
    void updateTransform();    

    void connectAnimationSignals();
};