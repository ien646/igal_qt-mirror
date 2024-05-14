#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class ListSelectWidget : public QWidget
{
    Q_OBJECT
    
public:
    ListSelectWidget(const std::vector<std::string>& items, QWidget* parent = nullptr);

    void setItems(const std::vector<std::string>& items);

    void keyPressEvent(QKeyEvent* ev) override;

signals:
    void itemSelected(const std::string& item);
    void cancelled();

private:
    std::vector<std::string> _items;
    QVBoxLayout* _layout;
    std::vector<QLabel*> _labels;
    size_t _selectedIndex = 0;
};