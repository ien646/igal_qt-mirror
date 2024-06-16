#pragma once

#include <QGraphicsLinearLayout>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsView>
#include <QStackedLayout>
#include <QTimer>

#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>

#include "VideoControls.hpp"

class VideoPlayerWidget : public QGraphicsView
{
    Q_OBJECT

public:
    VideoPlayerWidget(QWidget* parent = nullptr);

    void setMedia(const std::string& src);
    void stop();

    void paintEvent(QPaintEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;

    QMediaPlayer* mediaPlayer();
    QAudioOutput* audioOutput();

private:
    std::string _target;

    QStackedLayout* _main_layout = nullptr;
    VideoControls* _video_controls = nullptr;
    QGraphicsScene* _scene = nullptr;
    QGraphicsRectItem* _scene_rect = nullptr;
    QGraphicsVideoItem* _video_item = nullptr;
    QMediaPlayer* _media_player = nullptr;
    QAudioOutput* _audio_output = nullptr;

    QTimer* _autoHideTimer = nullptr;
    bool _clicked = false;
    bool _was_playing_before_seek_click = false;

    void setupConnections();
};