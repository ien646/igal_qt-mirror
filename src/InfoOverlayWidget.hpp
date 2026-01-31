#pragma once

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class InfoOverlayWidget : public QWidget
{
public:
    InfoOverlayWidget(QWidget* parent = nullptr);

    bool isInfoShown() const;
    bool isMessageShown() const;
    void showMessage(const QString& message) const;
    void showInfo(const QString& info) const;
    void hideInfo() const;

private:
    QVBoxLayout* _layout;
    QLabel* _message_label;
    QLabel* _info_label;
    QTimer* _message_timer;
};