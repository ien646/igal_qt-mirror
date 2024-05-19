#include "VideoPlayerWidget.hpp"

#include <ien/activity.hpp>

#include <QResizeEvent>

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
    _media_player = new QMediaPlayer(this);
    _audio_output = new QAudioOutput(this);
    _scene = new QGraphicsScene(this);
    _timer = new QTimer(this);

    this->setScene(_scene);
    this->setLayout(_main_layout);

    _video_controls->setMinimumSize(600, 24);
    _video_controls->setMouseTracking(true);

    _audio_output->setVolume(0.5f);

    _scene_rect = _scene->addRect(QRect(QPoint(0, 0), size() * devicePixelRatio()));
    _scene_rect->setPen(Qt::NoPen);

    _video_item = new QGraphicsVideoItem(_scene_rect);
    _video_item->setAspectRatioMode(Qt::AspectRatioMode::KeepAspectRatio);
    _video_item->setSize(size() * devicePixelRatio());
    _scene->addItem(_video_item);

    _media_player->setVideoOutput(_video_item);
    _media_player->setLoops(QMediaPlayer::Infinite);
    _media_player->setAudioOutput(_audio_output);

    _timer->setSingleShot(true);
    _timer->setInterval(2500);
    _timer->start();

    setupConnections();
}

void VideoPlayerWidget::setupConnections()
{
    connect(_video_controls, &VideoControls::playClicked, this, [this] { _media_player->play(); });

    connect(_video_controls, &VideoControls::pauseClicked, this, [this] { _media_player->pause(); });

    connect(_video_controls, &VideoControls::videoPositionChanged, this, [this](float pos) {
        _media_player->setPosition(static_cast<float>(_media_player->duration()) * pos);
    });

    connect(_video_controls, &VideoControls::seekSliderClicked, this, [this] {
        _clicked = true;
        _was_playing_before_seek_click = _media_player->isPlaying();
        _media_player->pause();
        _timer->stop();
    });

    connect(_video_controls, &VideoControls::seekSliderReleased, this, [this] {
        _clicked = false;
        if (_was_playing_before_seek_click)
        {
            _media_player->play();
        }
        _timer->start();
    });

    connect(_video_controls, &VideoControls::volumeSliderClicked, this, [this] {
        _timer->stop();
        _clicked = true;
    });

    connect(_video_controls, &VideoControls::volumeSliderReleased, this, [this] {
        _timer->start();
        _clicked = false;
    });

    connect(_video_controls, &VideoControls::volumeChanged, this, [this](int volume) {
        _audio_output->setVolume(static_cast<float>(volume) / 100);
    });

    connect(_media_player, &QMediaPlayer::positionChanged, _video_controls, [&](qint64 pos) {
        if (_media_player->isPlaying())
        {
            _video_controls->setCurrentVideoDuration(_media_player->duration());
            _video_controls->setCurrentVideoPosition(pos);
            ien::simulate_user_activity();
        }
    });

    connect(_audio_output, &QAudioOutput::volumeChanged, this, [this](float volume) {
        _video_controls->setCurrentVolume(volume * 100);
    });

    connect(_timer, &QTimer::timeout, this, [this] {
        if (_media_player->isPlaying())
        {
            _video_controls->hide();
        }
    });
}

void VideoPlayerWidget::setMedia(const std::string& src)
{
    _media_player->setSource(QString::fromStdString(src));
    _media_player->play();
    _video_controls->setCurrentVideoDuration(_media_player->duration());
    _timer->start();
}

void VideoPlayerWidget::paintEvent(QPaintEvent* ev)
{
    QGraphicsView::paintEvent(ev);
}

void VideoPlayerWidget::resizeEvent(QResizeEvent* ev)
{
    const QRect scaledRect = QRect(QPoint(0, 0), ev->size() * devicePixelRatio());
    setGeometry(scaledRect);
    _scene->setSceneRect(scaledRect);
    _scene_rect->setRect(scaledRect);
    _video_item->setSize(ev->size());

    _video_controls->setFixedSize(ev->size());
    _video_controls->setMinimumSize(600, 24);
}

void VideoPlayerWidget::mouseMoveEvent(QMouseEvent* ev)
{
    _video_controls->show();
    if (!_clicked)
    {
        _timer->start();
    }
}

void VideoPlayerWidget::mousePressEvent(QMouseEvent* ev)
{
    _video_controls->show();
    _timer->start();
}

void VideoPlayerWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    _video_controls->show();
    _timer->start();
}

QMediaPlayer* VideoPlayerWidget::mediaPlayer()
{
    return _media_player;
}

QAudioOutput* VideoPlayerWidget::audioOutput()
{
    return _audio_output;
}