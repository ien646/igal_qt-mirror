#pragma once

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class InfoOverlayWidget : public QWidget
{
public:
    InfoOverlayWidget(QWidget* parent = nullptr);

    bool isInfoShown();
    bool isMessageShown();
    void showMessage(const QString& message);
    void showInfo(const QString& info);
    void hideInfo();

private:
    QVBoxLayout* _layout;
    QLabel* _message_label;
    QLabel* _info_label;
    QTimer* _message_timer;
};