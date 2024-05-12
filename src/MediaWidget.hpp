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

private:
    std::string _target;
    bool _isVideo;
    float _volumeBeforeMute;

    QStackedLayout* _mainLayout = nullptr;

    CurrentMediaType _currentMedia;

    CachedMediaProxy _cachedMediaProxy;
    std::shared_ptr<QImage> _image;
    std::unique_ptr<QPixmap> _pixmap;
    std::shared_ptr<QMovie> _animation;

    QLabel* _imageLabel = nullptr;    
    VideoPlayerWidget* _videoPlayer = nullptr;    
    InfoOverlayWidget* _infoOverlay = nullptr;

    void syncAnimationSize();
};