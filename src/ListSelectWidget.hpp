#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <unordered_set>

class ListSelectWidget : public QWidget
{
    Q_OBJECT
    
public:
    ListSelectWidget(const std::vector<std::string>& items, QWidget* parent = nullptr, bool multiselect = false);

    void setItems(const std::vector<std::string>& items);

    void keyPressEvent(QKeyEvent* ev) override;

signals:
    void itemsSelected(const std::vector<std::string>& item);
    void cancelled();

private:
    std::vector<std::string> _items;
    QVBoxLayout* _layout = nullptr;
    std::vector<QLabel*> _labels;
    size_t _currentIndex = 0;
    bool _multiselect = false;
    std::unordered_set<size_t> _selectedIndices;
};