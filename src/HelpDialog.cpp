#include "HelpDialog.hpp"

#include <QLabel>

#include "Utils.hpp"

const std::vector<std::string> CONTROLS_LINES = {
    "<u>Left arrow</u>: Previous item",
    "<u>Right arrow</u>: Next item",
    "<u>PgUp</u>: Skip 10 items back",
    "<u>PgDown</u>: Skip 10 items forward",
    "<u>Home</u>: Skip to first item",
    "<u>End</u>: Skip to last item",
    "<u>O</u>: Open Directory",
    "<u>Delete</u>: Delete current item",
    "<u>I</u>: Toggle info overlay",
    "<u>M</u>: Toggle mute (video)",
    "<u>Space</u>: Play/Pause (video)",
    "<u>Num[+]</u>: Zoom in (image/animation)",
    "<u>Num[-]</u>: Zoom out (image/animation)",
    "<u>Num[8]</u>: Translate up (image/animation)",
    "<u>Num[6]</u>: Translate right (image/animation)",
    "<u>Num[4]</u>: Translate down (image/animation)",
    "<u>Num[2]</u>: Translate left (image/animation)",
    "<u>Num[0]</u>: Reset transform (image/animation)",
    "<u>Ctrl+Shift+Num[+]</u>: Open upscale dialog",
    "<u>Ctrl+Shift+{Key}</u>: Copy to link directory",
    "<u>Shift+Return</u>: Open linked directory navigator"
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
        const std::string& line = CONTROLS_LINES[i];
        const size_t col = i % 2;
        const size_t row = i / 2;

        QLabel* label = new QLabel(this);
        label->setTextFormat(Qt::TextFormat::RichText);
        label->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        label->setText(QString::fromStdString(line));

        _layout->addWidget(label, row, col);
    }
}