#include "BaseChatWindow.h"
#include <QKeyEvent>

BaseChatWindow::BaseChatWindow(QWidget *parent)
    : QMainWindow(parent)
{
}

BaseChatWindow::~BaseChatWindow()
{
}

bool BaseChatWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Base implementation - can be overridden in derived classes
    return QMainWindow::eventFilter(obj, event);
}