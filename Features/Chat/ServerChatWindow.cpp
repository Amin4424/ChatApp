#include "ServerChatWindow.h"
#include "ui_ServerChatWindow.h"
#include <QPushButton>
#include <QDebug>
#include <QKeyEvent>

ServerChatWindow::ServerChatWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerChatWindow)
    , m_isPrivateChat(false)
    , m_currentTargetUser("")
{
    ui->setupUi(this);
    
    // Connect send button click to slot
    connect(ui->sendMessageBtn, &QPushButton::clicked,
            this, &ServerChatWindow::onsendMessageBtnclicked);
    
    // Connect user list click to slot
    connect(ui->userListWdgt, &QListWidget::itemClicked,
            this, &ServerChatWindow::onUserListItemClicked);
    
    // Connect restart button
    connect(ui->restartServerBtn, &QPushButton::clicked,
            this, &ServerChatWindow::onRestartServerClicked);
    
    // Install event filter for Enter key
    ui->typeMessageTxt->installEventFilter(this);
}

ServerChatWindow::~ServerChatWindow()
{
    delete ui;
}

bool ServerChatWindow::eventFilter(QObject *obj, QEvent *event)
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

void ServerChatWindow::onsendMessageBtnclicked()
{
    QString message = ui->typeMessageTxt->toPlainText();
    emit sendMessageRequested(message);
}

void ServerChatWindow::onchatHistoryWdgtitemClicked(QListWidgetItem *item)
{
    qDebug() << "You clicked on message:" << item->text();
}

void ServerChatWindow::onUserListItemClicked(QListWidgetItem *item)
{
    if (item) {
        QString userId = item->text();
        
        // Check if it's the broadcast option
        if (userId.contains("همه کاربران") || userId.contains("📢")) {
            setBroadcastMode();
            emit userSelected("همه کاربران");
        } 
        // Check if it's the separator
        else if (userId.contains("──────")) {
            // Do nothing for separator
            return;
        }
        // It's a specific user
        else {
            // Remove the icon prefix if present
            QString cleanUserId = userId;
            if (cleanUserId.startsWith("👤 ")) {
                cleanUserId = cleanUserId.mid(2); // Remove "👤 "
            }
            emit userSelected(cleanUserId);
            setPrivateChatMode(cleanUserId);
        }
    }
}

void ServerChatWindow::onBroadcastModeClicked()
{
    setBroadcastMode();
}

void ServerChatWindow::showMessage(QString msg)
{
    if(msg.isEmpty()) return;
    
    ui->chatHistoryWdgt->addItem(msg);
    ui->chatHistoryWdgt->scrollToBottom();
    ui->typeMessageTxt->clear();
}

void ServerChatWindow::updateUserList(const QStringList &users)
{
    ui->userListWdgt->clear();
    
    // Add "Broadcast to All" option at the top
    QListWidgetItem *broadcastItem = new QListWidgetItem("📢 همه کاربران (پخش عمومی)");
    QFont font = broadcastItem->font();
    font.setBold(true);
    broadcastItem->setFont(font);
    broadcastItem->setForeground(QColor("#075e54"));
    ui->userListWdgt->addItem(broadcastItem);
    
    // Add separator
    QListWidgetItem *separator = new QListWidgetItem("──────────────");
    separator->setFlags(separator->flags() & ~Qt::ItemIsSelectable);
    separator->setForeground(QColor("#cccccc"));
    ui->userListWdgt->addItem(separator);
    
    // Add individual users
    for (const QString &user : users) {
        ui->userListWdgt->addItem("👤 " + user);
    }
}

void ServerChatWindow::updateUserCount(int count)
{
    ui->userCountLabel->setText(QString("کاربر متصل: %1").arg(count));
}

void ServerChatWindow::setPrivateChatMode(const QString &userId)
{
    m_isPrivateChat = true;
    m_currentTargetUser = userId;
    
    // Update UI to show private chat mode
    ui->chatTitleLabel->setText(QString("گفتگوی خصوصی با %1").arg(userId));
    ui->chatTitleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 0px; color: #ffffff;");
    
    ui->chatSubtitleLabel->setText("فقط این کاربر پیام‌های شما را می‌بیند");
    ui->chatSubtitleLabel->setStyleSheet("font-size: 10pt; padding: 2px; color: #ffffff;");
    
    // Find the chatHeader widget and change its color to indicate private mode
    QWidget *header = this->findChild<QWidget*>("chatHeader");
    if (header) {
        header->setStyleSheet(
            "QWidget#chatHeader {"
            "    background-color: #075e54;"
            "    border-bottom: 1px solid #0066cc;"
            "}"
        );
    }
    
    qDebug() << "Switched to private chat with:" << userId;
}

void ServerChatWindow::setBroadcastMode()
{
    m_isPrivateChat = false;
    m_currentTargetUser = "";
    
    // Update UI to show broadcast mode
    ui->chatTitleLabel->setText("گفتگوی عمومی");
    ui->chatTitleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 0px; color: #ffffff;");
    
    ui->chatSubtitleLabel->setText("پیام‌های شما به همه ارسال می‌شود");
    ui->chatSubtitleLabel->setStyleSheet("font-size: 10pt; padding: 2px; color: #ffffff;");
    
    // Find the chatHeader widget and reset to green color
    QWidget *header = this->findChild<QWidget*>("chatHeader");
    if (header) {
        header->setStyleSheet(
            "QWidget#chatHeader {"
            "    background-color: #075e54;"
            "    border-bottom: 1px solid #055346;"
            "}"
        );
    }
    
    qDebug() << "Switched to broadcast mode";
}

void ServerChatWindow::updateServerInfo(const QString &ip, int port, const QString &status)
{
    QString infoText = QString("آی پی ثابت شما : %1:%2\nوضعیت اتصال : %3")
                           .arg(ip)
                           .arg(port)
                           .arg(status);
    ui->serverIpLabel->setText(infoText);
}

void ServerChatWindow::onRestartServerClicked()
{
    qDebug() << "Restart server button clicked";
    emit restartServerRequested();
}

void ServerChatWindow::clearChatHistory()
{
    ui->chatHistoryWdgt->clear();
}
