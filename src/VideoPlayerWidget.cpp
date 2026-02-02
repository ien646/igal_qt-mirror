#include "VideoPlayerWidget.hpp"

#include <ien/activity.hpp>

#include <QMediaMetaData>
#include <QResizeEvent>

#include "Utils.hpp"

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QGraphicsView(parent)
{
    this->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    this->setMouseTracking(true);
    this->viewport()->installEventFilter(this);
    this->setAlignment(Qt::AlignmentFlag::AlignCenter);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    this->setRenderHints(
        QPainter::RenderHint::Antialiasing | QPainter::RenderHint::SmoothPixmapTransform |
        QPainter::RenderHint::VerticalSubpixelPositioning);

    _main_layout = new QStackedLayout(this);
    _video_controls = new VideoControls(this);
    _scene = new QGraphicsScene(this);
    _autoHideTimer = new QTimer(this);

    this->setScene(_scene);
    this->setLayout(_main_layout);

    _video_controls->setMinimumSize(600, 24);
    _video_controls->setMouseTracking(true);

    _scene_rect = _scene->addRect(QRect(QPoint(0, 0), size() * devicePixelRatio()));
    _scene_rect->setPen(Qt::NoPen);

    _autoHideTimer->setSingleShot(true);
    _autoHideTimer->setInterval(2500);
    _autoHideTimer->start();

    disableFocusOnChildWidgets(this);

    _media_player = new QMediaPlayer(this);
    _media_player->setLoops(QMediaPlayer::Infinite);

    setupConnections();

    // Delay initialization of audio output
    QTimer::singleShot(250, [this] {
        _audio_output = new QAudioOutput(this);
        _audio_output->setVolume(0.5f);
        _media_player->setAudioOutput(_audio_output);
        connect(_audio_output, &QAudioOutput::volumeChanged, this, [this](const float volume) {
            _video_controls->setCurrentVolume(static_cast<int>(volume * 100));
        });

        _video_item = new QGraphicsVideoItem(_scene_rect);
        _video_item->setAspectRatioMode(Qt::AspectRatioMode::KeepAspectRatio);
        _video_item->setSize(size() * devicePixelRatio());
        _media_player->setVideoOutput(_video_item);
    });
}

void VideoPlayerWidget::setupConnections()
{
    connect(_video_controls, &VideoControls::playClicked, this, [this] {
        _media_player->play();
        _autoHideTimer->start();
    });

    connect(_video_controls, &VideoControls::pauseClicked, this, [this] {
        _media_player->pause();
        _autoHideTimer->stop();
    });

    connect(_video_controls, &VideoControls::videoPositionChanged, this, [this](const float pos) {
        _media_player->setPosition(static_cast<int>(static_cast<float>(_media_player->duration()) * pos));
    });

    connect(_video_controls, &VideoControls::seekSliderClicked, this, [this] {
        _clicked = true;
        _was_playing_before_seek_click = _media_player->isPlaying();
        _media_player->pause();
        _autoHideTimer->stop();
    });

    connect(_video_controls, &VideoControls::seekSliderMoved, this, [this] { _autoHideTimer->stop(); });

    connect(_video_controls, &VideoControls::seekSliderReleased, this, [this] {
        _clicked = false;
        if (_was_playing_before_seek_click)
        {
            _media_player->play();
        }
        _autoHideTimer->start();
    });

    connect(_video_controls, &VideoControls::volumeSliderClicked, this, [this] {
        _autoHideTimer->stop();
        _clicked = true;
    });

    connect(_video_controls, &VideoControls::volumeSliderMoved, this, [this] { _autoHideTimer->stop(); });

    connect(_video_controls, &VideoControls::volumeSliderReleased, this, [this] {
        _autoHideTimer->start();
        _clicked = false;
    });

    connect(_video_controls, &VideoControls::volumeChanged, this, [this](const int volume) {
        if (_audio_output)
        {
            _audio_output->setVolume(static_cast<float>(volume) / 100);
        }
    });

    connect(_video_controls, &VideoControls::audioChannelChanged, this, [this](const int index) {
        _media_player->setActiveAudioTrack(index);
    });

    connect(_media_player, &QMediaPlayer::positionChanged, _video_controls, [&](const qint64 pos) {
        if (_media_player->isPlaying())
        {
            _video_controls->setCurrentVideoDuration(_media_player->duration());
            _video_controls->setCurrentVideoPosition(pos);
        }
    });

    connect(_autoHideTimer, &QTimer::timeout, this, [this] {
        if (_media_player->isPlaying())
        {
            _video_controls->hide();
        }
    });

    connect(_media_player, &QMediaPlayer::tracksChanged, this, [this] {
        std::map<int, std::string> channelInfo;
        for (size_t i = 0; i < _media_player->audioTracks().size(); ++i)
        {
            auto track = _media_player->audioTracks().at(i);
            const auto language = track[QMediaMetaData::Language].toString();
            const auto trackNum = i;
            channelInfo.emplace(trackNum, language.toStdString());
        }

        _video_controls->setAudioChannelInfo(channelInfo);
    });
}

void VideoPlayerWidget::setMedia(const std::string& src) const
{
    _media_player->stop();
    _media_player->setSource(QUrl{});
    _media_player->setSource(QString::fromStdString(src));
    _media_player->play();
    _video_controls->setCurrentVideoDuration(_media_player->duration());
    _autoHideTimer->start();
}

void VideoPlayerWidget::paintEvent(QPaintEvent* ev)
{
    QGraphicsView::paintEvent(ev);
}

void VideoPlayerWidget::resizeEvent(QResizeEvent* ev)
{
    const auto scaledRect = QRect(QPoint(0, 0), ev->size() * devicePixelRatio());
    setGeometry(scaledRect);
    _scene->setSceneRect(scaledRect);
    _scene_rect->setRect(scaledRect);
    if (_video_item)
    {
        _video_item->setSize(ev->size());
    }

    _video_controls->setFixedSize(ev->size());
    _video_controls->setMinimumSize(600, 24);
}

void VideoPlayerWidget::mouseMoveEvent(QMouseEvent* ev)
{
    _video_controls->show();
    if (!_clicked)
    {
        _autoHideTimer->start();
    }
}

void VideoPlayerWidget::mousePressEvent(QMouseEvent* ev)
{
    _video_controls->show();
    _autoHideTimer->start();
}

void VideoPlayerWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    _video_controls->show();
    _autoHideTimer->start();
}

QMediaPlayer* VideoPlayerWidget::mediaPlayer() const
{
    return _media_player;
}

QAudioOutput* VideoPlayerWidget::audioOutput() const
{
    return _audio_output;
}