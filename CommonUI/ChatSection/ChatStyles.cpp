#include "ChatStyles.h"

QString ChatStyles::getScrollBarStyle()
{
    return 
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: #f0f0f0;"
        "    width: 8px;"
        "    margin: 0px 0px 0px 0px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: #c1c1c1;"
        "    min-height: 20px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: #a8a8a8;"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "    background-color: #787878;"
        "}"
        "QScrollBar::sub-line:vertical {"
        "    border: none;"
        "    background: none;"
        "    height: 0px;"
        "}"
        "QScrollBar::add-line:vertical {"
        "    border: none;"
        "    background: none;"
        "    height: 0px;"
        "}"
        "QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {"
        "    background: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "    background: none;"
        "}";
}

QString ChatStyles::getUserListStyle()
{
    return 
        "QListWidget {"
        "    background-color: #ffffff;"
        "    font-size: 11pt;"
        "    padding: 6px;"
        "    color: #111827;"
        "    min-width: 260px;"
        "    max-width: 260px;"
        "    outline: 0;"
        "}"
        "QListWidget::item {"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 10px;"
        "    padding: 0px 8px;"
        "    margin: 4px 0px;"
        "    color: #111827;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: transparent;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: transparent;"
        "    color: #ffffff;"
        "}";
}

QString ChatStyles::getChatHistoryStyle()
{
    return 
        "QListWidget {"
        "    background-color: #f4f5f7;"
        "    border: none;"
        "    font-size: 13pt;"
        "    padding: 12px;"
        "    color: #111827;"
        "}"
        "QListWidget::item {"
        "    background-color: transparent;"
        "    border: none;"
        "    padding: 0px;"
        "}"
        "QListWidget::item:selected,"
        "QListWidget::item:selected:active,"
        "QListWidget::item:selected:!active,"
        "QListWidget::item:selected:hover {"
        "    background-color: transparent;"
        "    border: none;"
        "    outline: none;"
        "}";
}
