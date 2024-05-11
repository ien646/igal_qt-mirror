#pragma once

#include <QImage>
#include <QLabel>
#include <QMovie>
#include <QStackedLayout>
#include <QTimer>

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>

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

    void paintEvent(QPaintEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

private:
    std::string _target;
    bool _is_video;

    QStackedLayout* _main_layout = nullptr;

    CurrentMediaType _current_media;

    std::unique_ptr<QImage> _image;
    std::unique_ptr<QPixmap> _pixmap;
    std::unique_ptr<QMovie> _animation;

    QLabel* _image_label = nullptr;    
    VideoPlayerWidget* _video_player = nullptr;    
    InfoOverlayWidget* _info_overlay = nullptr;

    void syncAnimationSize();
};