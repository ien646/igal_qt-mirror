#include "ListSelectWidget.hpp"

#include <QKeyEvent>

#include "Utils.hpp"

constexpr const char* UNSELECTED_STYLESHEET = "QLabel{ background-color: rgba(30, 30, 30, 200); padding: 0.5em; }";
constexpr const char* SELECTED_STYLESHEET = "QLabel{ background-color: rgba(50, 150, 255, 220); padding: 0.5em; "
                                            "border-style: solid; border-color: #ffff88; border-width: 2px; }";

ListSelectWidget::ListSelectWidget(const std::vector<std::string>& items, QWidget* parent)
    : QWidget(parent)
    , _items(items)
{
    this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    this->setStyleSheet("QWidget{ background-color: rgba(50, 100, 150, 100); }");

    _layout = new QVBoxLayout(this);
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

    _layout->itemAt(_selectedIndex + 1)->widget()->setStyleSheet(SELECTED_STYLESHEET);
}

void ListSelectWidget::keyPressEvent(QKeyEvent* ev)
{
    _layout->itemAt(_selectedIndex + 1)->widget()->setStyleSheet(UNSELECTED_STYLESHEET);
    if (ev->key() == Qt::Key_Down)
    {
        ++_selectedIndex;
        if (_selectedIndex >= _items.size())
        {
            _selectedIndex = _items.size() - 1;
        }
    }
    else if (ev->key() == Qt::Key_Up)
    {
        if (_selectedIndex > 0)
        {
            --_selectedIndex;
        }
    }
    else if (ev->key() == Qt::Key_Return)
    {
        QLabel* label = dynamic_cast<QLabel*>(_layout->itemAt(_selectedIndex + 1)->widget());
        emit itemSelected(label->text().toStdString());
    }
    else if(ev->key() == Qt::Key_Escape)
    {
        emit cancelled();
    }
    _layout->itemAt(_selectedIndex + 1)->widget()->setStyleSheet(SELECTED_STYLESHEET);
}