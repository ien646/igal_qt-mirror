#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QWidget>

class VideoControls : public QWidget
{
    Q_OBJECT

public:
    VideoControls(QWidget* parent = nullptr);

    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    
    void setCurrentVideoDuration(int64_t ms);
    void setCurrentVideoPosition(int64_t posMs);

    void setCurrentVolume(int percent);

signals:
    void playClicked();
    void pauseClicked();
    void seekSliderClicked();
    void seekSliderReleased();
    void volumeSliderClicked();
    void volumeSliderReleased();
    void videoPositionChanged(float pos);
    void volumeChanged(int percent);

private:
    bool _clicking = false;
    uint64_t _current_video_duration = 0;
    QHBoxLayout* _main_layout = nullptr;
    QLabel* _seek_position_label = nullptr;
    QLabel* _play_label = nullptr;
    QLabel* _pause_label = nullptr;
    QSlider* _seek_slider = nullptr;
    QLabel* _volume_label = nullptr;
    QSlider* _volume_slider = nullptr;

    void updateButtonStyles();
    void updateSeekLabel(int64_t posMs);
};