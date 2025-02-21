#include "HelpOverlay.hpp"

#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QScrollBar>

#include "Utils.hpp"

const std::vector<std::string> CONTROLS_LINES = {
    "<b>Left arrow</b>: Previous item",
    "<b>Right arrow</b>: Next item",
    "<b>PgUp</b>: Skip 10 items back",
    "<b>PgDown</b>: Skip 10 items forward",
    "<b>Home</b>: Skip to first item",
    "<b>End</b>: Skip to last item",
    "<b>O</b>: Open Directory",
    "<b>Delete</b>: Delete current item",
    "<b>I</b>: Toggle info overlay",
    "<b>M</b>: Toggle mute (video)",
    "<b>Space</b>: Play/Pause (video)",
    "<b>.</b>: Mark image",
    "<b>Num[+]</b>: Zoom in (image/animation)",
    "<b>Num[-]</b>: Zoom out (image/animation)",
    "<b>Num[8]</b>: Move up (image/animation)",
    "<b>Num[6]</b>: Move right (image/animation)",
    "<b>Num[4]</b>: Move down (image/animation)",
    "<b>Num[2]</b>: Move left (image/animation)",
    "<b>Num[0]</b>: Reset transform (image/animation)",
    "<b>Ctrl+.</b>: Toggle marked-mode",
    "<b>Ctrl+Shift+Num[+]</b>: Open upscale dialog",
    "<b>Ctrl+Shift+Num[-]</b>: Filter videos",
    "<b>Ctrl+Shift+{Key}</b>: Copy to link directory",
    "<b>Shift+Return</b>: Open linked directory navigator",
    "<b>Shift+Up</b>: Change video/animation speed (+5%)",
    "<b>Shift+Down</b>: Change video/animation speed (-5%)"
};

HelpOverlay::HelpOverlay(QWidget* parent)
    : QFrame(parent)
{
    setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::FocusPolicy::NoFocus);
    setStyleSheet("QFrame {background-color: rgba(50, 50, 25, 120); }");
    auto* layout = new QVBoxLayout(this);
    setLayout(layout);

    _list = new QListWidget(this);
    _list->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    _list->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    _list->setSelectionRectVisible(false);
    layout->addWidget(_list);

    for (const auto& line : CONTROLS_LINES)
    {
        QLabel* label = new QLabel(this);
        label->setTextFormat(Qt::TextFormat::RichText);
        label->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        label->setText(QString::fromStdString(line));
        label->setFont(getTextFont());
        label->setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);

        QListWidgetItem* item = new QListWidgetItem();
        _list->addItem(item);
        _list->setItemWidget(item, label);
    }
}

void HelpOverlay::moveUp()
{
    int step = _list->verticalScrollBar()->singleStep();
    _list->verticalScrollBar()->setValue(_list->verticalScrollBar()->value() - step);
}

void HelpOverlay::moveDown()
{
    int step = _list->verticalScrollBar()->singleStep();
    _list->verticalScrollBar()->setValue(_list->verticalScrollBar()->value() + step);
}