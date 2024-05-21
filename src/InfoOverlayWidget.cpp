#include "InfoOverlayWidget.hpp"

#include <QCommonStyle>
#include <QGraphicsDropShadowEffect>

#include "Utils.hpp"

InfoOverlayWidget::InfoOverlayWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::FocusPolicy::NoFocus);
    setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
    setStyleSheet("QWidget{background-color:rgba(0, 0, 0, 0);}");

    _layout = new QVBoxLayout(this);
    _message_label = new QLabel(this);
    _info_label = new QLabel(this);

    _layout->addWidget(_message_label, 0, Qt::AlignmentFlag::AlignTop);
    _layout->addStretch(1);
    _layout->addWidget(_info_label, 0, Qt::AlignmentFlag::AlignBottom);

    constexpr const char* stylesheetFormat =
        "QWidget{{background-color:{}; color:#ccffaa; padding:0.5em; "
        "margin:0px; outline-style:solid; outline-color:#000000; outline-width:1px;}}";

    _message_label->setAlignment(Qt::AlignmentFlag::AlignTop);
    _message_label->setFont(getTextFont());
    _message_label->setStyleSheet(QString::fromStdString(std::format(stylesheetFormat, "rgba(0, 150, 100, 150)")));
    _message_label->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    _message_label->setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
    _message_label->setWordWrap(true);

    _info_label->setAlignment(Qt::AlignmentFlag::AlignTop);
    _info_label->setFont(getTextFont());
    _info_label->setStyleSheet(QString::fromStdString(std::format(stylesheetFormat, "rgba(0, 64, 128, 150)")));
    _info_label->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    _info_label->setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
    _info_label->setWordWrap(false);
    _info_label->setTextFormat(Qt::TextFormat::RichText);

    QGraphicsDropShadowEffect* dropShadow = new QGraphicsDropShadowEffect(this);    
    dropShadow->setBlurRadius(0);
    dropShadow->setColor(QColor("#000000"));
    dropShadow->setOffset(2, 2);

    _message_label->setGraphicsEffect(dropShadow);
    _info_label->setGraphicsEffect(dropShadow);

    _message_timer = new QTimer(this);
    _message_timer->setSingleShot(true);
    _message_timer->setInterval(2500);
    _message_timer->stop();

    connect(_message_timer, &QTimer::timeout, this, [this] { _message_label->hide(); });

    _message_label->hide();
    _info_label->hide();

    disableFocusOnChildWidgets(this);
}

bool InfoOverlayWidget::isInfoShown()
{
    return !_info_label->isHidden();
}

bool InfoOverlayWidget::isMessageShown()
{
    return !_message_label->isHidden();
}

void InfoOverlayWidget::showMessage(const QString& message)
{
    _message_label->setText(message);
    _message_label->show();
    _message_timer->start();
}

void InfoOverlayWidget::showInfo(const QString& info)
{
    _info_label->setText(info);
    _info_label->show();
}

void InfoOverlayWidget::hideInfo()
{
    _info_label->hide();
}