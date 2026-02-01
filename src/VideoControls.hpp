#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QWidget>

#include "ClickableSlider.hpp"

class VideoControls : public QWidget
{
    Q_OBJECT

public:
    VideoControls(QWidget* parent = nullptr);

    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    
    void setCurrentVideoDuration(int64_t ms);
    void setCurrentVideoPosition(int64_t posMs) const;

    void setCurrentVolume(int percent) const;

    void setAudioChannelInfo(const std::map<int, std::string>& channelInfo) const;

signals:
    void playClicked();
    void pauseClicked();
    void seekSliderClicked();
    void seekSliderMoved();
    void seekSliderReleased();
    void volumeSliderClicked();
    void volumeSliderMoved();
    void volumeSliderReleased();
    void videoPositionChanged(float pos);
    void volumeChanged(int percent);
    void audioChannelChanged(int channelIndex);

private:
    bool _clicking = false;
    uint64_t _current_video_duration = 0;
    QHBoxLayout* _main_layout = nullptr;
    QLabel* _seek_position_label = nullptr;
    QLabel* _play_label = nullptr;
    QLabel* _pause_label = nullptr;
    ClickableSlider* _seek_slider = nullptr;
    QLabel* _volume_label = nullptr;
    ClickableSlider* _volume_slider = nullptr;
    QComboBox* _audio_channel_combo = nullptr;

    void updateButtonStyles() const;
    void updateSeekLabel(int64_t posMs) const;
};