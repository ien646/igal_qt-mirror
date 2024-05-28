#include "HelpDialog.hpp"

#include <QLabel>

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
    "<b>Num[+]</b>: Zoom in (image/animation)",
    "<b>Num[-]</b>: Zoom out (image/animation)",
    "<b>Num[8]</b>: Translate up (image/animation)",
    "<b>Num[6]</b>: Translate right (image/animation)",
    "<b>Num[4]</b>: Translate down (image/animation)",
    "<b>Num[2]</b>: Translate left (image/animation)",
    "<b>Num[0]</b>: Reset transform (image/animation)",
    "<b>Ctrl+Shift+Num[+]</b>: Open upscale dialog",
    "<b>Ctrl+Shift+{Key}</b>: Copy to link directory",
    "<b>Shift+Return</b>: Open linked directory navigator"
};

HelpDialog::HelpDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowModality(Qt::WindowModality::NonModal);
    setFocusPolicy(Qt::FocusPolicy::NoFocus);    

    _layout = new QGridLayout(this);
    setLayout(_layout);

    for(size_t i = 0; i < CONTROLS_LINES.size(); ++i)
    {
        size_t index = (i / 2) + ((1 + CONTROLS_LINES.size() / 2) * (i % 2));
        const std::string& line = CONTROLS_LINES[index];
        const size_t col = i % 2;
        const size_t row = i / 2;

        QLabel* label = new QLabel(this);
        label->setTextFormat(Qt::TextFormat::RichText);
        label->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        label->setText(QString::fromStdString(line));

        _layout->addWidget(label, row, col);
    }
}