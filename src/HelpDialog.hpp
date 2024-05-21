#pragma once

#include <QDialog>
#include <QGridLayout>
#include <QWidget>

class HelpDialog : public QDialog
{
public:
    HelpDialog(QWidget* parent);

private:
    QGridLayout* _layout = nullptr;
};