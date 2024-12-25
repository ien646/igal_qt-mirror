#include "ListSelectWidget.hpp"

#include <QKeyEvent>

#include "Utils.hpp"

#include <ranges>

constexpr const char* UNSELECTED_STYLESHEET =
    "QLabel{ background-color: rgba(30, 30, 30, 200); padding: 0.3em; "
    "border-width: 2px; border-style: solid; border-color: rgba(30, 30, 30, 30)}";

constexpr const char* CURRENT_STYLESHEET =
    "QLabel{ background-color: rgba(50, 150, 255, 220); padding: 0.3em; "
    "border-style: solid; border-color: #ffff88; border-width: 2px; }";

constexpr const char* SELECTED_STYLESHEET =
    "QLabel{ background-color: rgba(60, 160, 255, 220); padding: 0.3em;"
    "border-style: dashed; border-color: #ffff88; border-width: 2px; }";

ListSelectWidget::ListSelectWidget(const std::vector<std::string>& items, QWidget* parent, bool multiselect)
    : QWidget(parent)
    , _items(items)
    , _multiselect(multiselect)
{
    this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    this->setStyleSheet("QWidget{ background-color: rgba(50, 100, 150, 100); }");

    _layout = new QVBoxLayout(this);
    setLayout(_layout);

    setItems(_items);
}

void ListSelectWidget::setItems(const std::vector<std::string>& items)
{
    _items = items;

    while (QLayoutItem* item = _layout->takeAt(0))
    {
        auto widget = item->widget();
        delete widget;
        _layout->removeItem(item);
        delete item;
    }

    _layout->addStretch(1);
    for (const auto& item : _items)
    {
        QLabel* label = new QLabel(this);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label->setFont(getTextFont());
        label->setStyleSheet(UNSELECTED_STYLESHEET);
        label->setText(QString::fromStdString(item));
        _layout->addWidget(label, 0, Qt::AlignmentFlag::AlignTop);
    }
    _layout->addStretch(1);

    if (!_items.empty())
    {
        _layout->itemAt(_currentIndex + 1)->widget()->setStyleSheet(CURRENT_STYLESHEET);
    }
}

void ListSelectWidget::keyPressEvent(QKeyEvent* ev)
{
    if (_items.empty())
    {
        return;
    }

    if (!_selectedIndices.contains(_currentIndex))
    {
        _layout->itemAt(_currentIndex + 1)->widget()->setStyleSheet(UNSELECTED_STYLESHEET);
    }

    if (_multiselect && ev->key() == Qt::Key_Space)
    {
        if (_selectedIndices.contains(_currentIndex))
        {
            _selectedIndices.erase(_currentIndex);
        }
        else
        {
            _selectedIndices.insert(_currentIndex);
        }
    }

    if (ev->key() == Qt::Key_Down)
    {
        ++_currentIndex;
        if (_currentIndex >= _items.size())
        {
            _currentIndex = _items.size() - 1;
        }
    }
    else if (ev->key() == Qt::Key_Up)
    {
        if (_currentIndex > 0)
        {
            --_currentIndex;
        }
    }
    else if(ev->key() == Qt::Key_Home)
    {
        _currentIndex = 0;
    }
    else if(ev->key() == Qt::Key_End)
    {
        _currentIndex = _items.size() - 1;
    }
    else if (ev->key() == Qt::Key_Return)
    {
        _selectedIndices.insert(_currentIndex);
        auto selectedItems = _selectedIndices | std::views::transform([this](size_t i) { return _items[i]; });        
        emit itemsSelected({ selectedItems.begin(), selectedItems.end() });
        _currentIndex = 0;
        _selectedIndices.clear();
    }
    else if (ev->key() == Qt::Key_Escape)
    {
        _currentIndex = 0;
        _selectedIndices.clear();
        emit cancelled();
    }

    if (_selectedIndices.contains(_currentIndex))
    {
        _layout->itemAt(_currentIndex + 1)->widget()->setStyleSheet(SELECTED_STYLESHEET);
    }
    else
    {
        _layout->itemAt(_currentIndex + 1)->widget()->setStyleSheet(CURRENT_STYLESHEET);
    }
}