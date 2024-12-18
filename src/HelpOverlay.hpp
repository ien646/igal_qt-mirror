#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QListWidget>
#include <QWidget>

class HelpOverlay : public QFrame
{
    Q_OBJECT

public:
    HelpOverlay(QWidget* parent);

    void moveUp();
    void moveDown();

private:
    QListWidget* _list = nullptr;
};