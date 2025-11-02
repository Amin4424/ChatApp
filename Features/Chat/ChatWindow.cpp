#include "ChatWindow.h"
#include "ui_ChatWindow.h"
#include <QKeyEvent>
#include <QDebug>

ChatWindow::ChatWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
    
    // Connect send button click to slot
    connect(ui->sendMessageBtn, &QPushButton::clicked,
            this, &ChatWindow::onsendMessageBtnclicked);
    
    // Connect reconnect button
    connect(ui->reconnectBtn, &QPushButton::clicked,
            this, &ChatWindow::onReconnectClicked);
    
    // Install event filter for Enter key
    ui->typeMessageTxt->installEventFilter(this);
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

bool ChatWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->typeMessageTxt && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // Check if Shift is NOT pressed (Shift+Enter = new line)
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                onsendMessageBtnclicked();
                return true; // Event handled
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void ChatWindow::onsendMessageBtnclicked()
{
    QString message = ui->typeMessageTxt->toPlainText();
    emit sendMessageRequested(message);
}

void ChatWindow::onchatHistoryWdgtitemClicked(QListWidgetItem *item)
{
    qDebug() << "You clicked on message:" << item->text();
}

void ChatWindow::showMessage(QString msg)
{
    if(!msg.isEmpty())
    {
        ui->chatHistoryWdgt->addItem(msg);
        ui->typeMessageTxt->clear();
    }
}

void ChatWindow::updateConnectionInfo(const QString &serverUrl, const QString &status)
{
    QString infoText = QString("وضعیت اتصال : %1").arg(status);
    ui->clientConnectionLabel->setText(infoText);
}

void ChatWindow::onReconnectClicked()
{
    qDebug() << "Reconnect button clicked";
    emit reconnectRequested();
}
