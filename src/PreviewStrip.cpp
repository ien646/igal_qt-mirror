#include "PreviewStrip.hpp"
#include "Utils.hpp"

constexpr auto LABEL_STYLESHEET = R"(
    QLabel { 
        background-color: #222222; 
        color: #FFFFFF;
        border-style: solid;
        border-width: 1px;
        border-color: rgba(255, 255, 255, 50);
        border-radius: 2px;
    }
)";

PreviewStrip::PreviewStrip(CachedMediaProxy& cachedMediaProxy, QWidget* parent)
    : QWidget(parent)
    , _cachedMediaProxy(cachedMediaProxy)
{
    setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::FocusPolicy::NoFocus);

    _layout = new QHBoxLayout(this);
    _layout->setAlignment(Qt::AlignmentFlag::AlignBottom);

    _layout->addStretch(1);
    for (size_t i = 0; i < 5; ++i)
    {
        auto* label = new QLabel(this);
        label->setText("NO-MEDIA");
        label->setStyleSheet(LABEL_STYLESHEET);
        label->setFont(getTextFont());
        label->setFixedSize(116, 116);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);

        _labels.emplace_back(label);
        _layout->addWidget(label);
    }
    _layout->addStretch(1);

    _timer = new QTimer(this);
    _movies = { 5, nullptr };

    setLayout(_layout);
}

void PreviewStrip::loadImages(const std::vector<std::string>& paths)
{
    assert(paths.size() <= _labels.size());

    _paths = paths;

    for (auto* movie : _movies)
    {
        if (movie)
        {
            movie->stop();
        }
    }

    _timer->setInterval(100);
    _timer->setSingleShot(false);

    _completed = std::vector<bool>(paths.size(), false);

    connect(_timer, &QTimer::timeout, this, [this]() {
        for (size_t i = 0; i < _paths.size(); ++i)
        {
            if (_completed[i])
            {
                continue;
            }

            const auto& path = _paths[i];

            if (path.empty())
            {
                _labels[i]->setScaledContents(false);
                _labels[i]->clear();
                _labels[i]->setText("NO-MEDIA");
                _completed[i] = true;
            }
            else if (!isImage(path) && !isAnimation(path))
            {
                _labels[i]->setScaledContents(false);
                _labels[i]->clear();
                _labels[i]->setText("VIDEO");
                _completed[i] = true;
            }
            else if (isImage(path))
            {
                _labels[i]->setScaledContents(false);
                auto future = _cachedMediaProxy.getImage(path);
                if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    auto& result = future.get();
                    _labels[i]->clear();
                    _labels[i]->setPixmap(
                        QPixmap::fromImage(*result.image())
                            .scaled(
                                _labels[i]->size(),
                                Qt::AspectRatioMode::KeepAspectRatio,
                                Qt::TransformationMode::SmoothTransformation));
                    _labels[i]->update();
                    _completed[i] = true;
                }
                else
                {
                    _labels[i]->setScaledContents(false);
                    _labels[i]->clear();
                    _labels[i]->setText("LOADING...");
                }
            }
            else if (isAnimation(path))
            {
                auto anim = _cachedMediaProxy.getAnimation(path);
                if (_movies[i])
                {
                    _movies[i]->stop();
                    delete _movies[i];
                }
                _movies[i] = new QMovie(QString::fromStdString(path));
                _movies[i]->setScaledSize(_labels[i]->size());
                _movies[i]->setCacheMode(QMovie::CacheMode::CacheNone);
                _movies[i]->start();

                _labels[i]->clear();
                _labels[i]->setMovie(_movies[i]);
                _labels[i]->setScaledContents(true);

                _completed[i] = true;
            }
        }
        if (std::ranges::all_of(_completed, [](bool v) { return v; }))
        {
            _timer->stop();
        }
    });

    _timer->start();
}