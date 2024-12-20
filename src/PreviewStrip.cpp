#include "PreviewStrip.hpp"

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
        label->setStyleSheet("QLabel { background-color: #222222; }");
        label->setFixedSize(112, 112);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);

        _labels.emplace_back(label);
        _layout->addWidget(label);
    }
    _layout->addStretch(1);

    _timer = new QTimer(this);

    setLayout(_layout);
}

void PreviewStrip::loadImages(const std::vector<std::string>& paths)
{
    assert(paths.size() <= _labels.size());

    _timer->setInterval(100);
    _timer->setSingleShot(false);
    disconnect(_timer);

    std::vector<bool> completed(paths.size(), false);

    connect(_timer, &QTimer::timeout, this, [this, paths = paths, completed = std::move(completed)]() mutable {
        for (size_t i = 0; i < paths.size(); ++i)
        {
            if (completed[i])
            {
                continue;
            }

            const auto& path = paths[i];
            if (path.empty())
            {
                _labels[i]->setText("NO-MEDIA");
                _labels[i]->setPixmap({});
                completed[i] = true;
            }
            else
            {
                auto future = _cachedMediaProxy.getImage(path);
                if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    auto& result = future.get();
                    _labels[i]->setText({});
                    _labels[i]->setPixmap(
                        QPixmap::fromImage(*result.image())
                            .scaled(
                                _labels[i]->size(),
                                Qt::AspectRatioMode::KeepAspectRatio,
                                Qt::TransformationMode::SmoothTransformation));
                    completed[i] = true;
                }
            }
        }
        if (paths.empty())
        {
            _timer->stop();
        }
    });

    _timer->start();
}