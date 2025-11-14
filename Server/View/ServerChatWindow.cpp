#include "View/ServerChatWindow.h"
#include "ui_ServerChatWindow.h"
#include <QPushButton>
#include <QDebug>
#include <QKeyEvent>
#include <QRegularExpression>

// --- ADD THESE INCLUDES ---
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include "TusUploader.h"
#include "FileMessageItem.h"
#include "TextMessageItem.h"
#include "InfoCard.h"
// ---

ServerChatWindow::ServerChatWindow(QWidget *parent)
    : BaseChatWindow(parent)
    , ui(new Ui::ServerChatWindow)
    , m_isPrivateChat(false)
    , m_currentTargetUser("")
    , m_uploadProgressBar(nullptr) // <-- INITIALIZE
{
    ui->setupUi(this);
    ui->chatHistoryWdgt->setResizeMode(QListView::Adjust);
    // Connect send button click to slot
    connect(ui->sendMessageBtn, &QPushButton::clicked,
            this, &ServerChatWindow::onsendMessageBtnclicked);

    // Connect user list click to slot
    connect(ui->userListWdgt, &QListWidget::itemClicked,
            this, &ServerChatWindow::onUserListItemClicked);

    // Connect restart button
    connect(ui->restartServerBtn, &QPushButton::clicked,
            this, &ServerChatWindow::onRestartServerClicked);

    // --- ADD THIS CONNECTION ---
    connect(ui->sendFileBtn, &QPushButton::clicked,
            this, &ServerChatWindow::onSendFileClicked);

    // Install event filter for Enter key
    ui->typeMessageTxt->installEventFilter(this);

    // --- SETUP PROGRESS BAR ---
    m_uploadProgressBar = new QProgressBar(this);
    m_uploadProgressBar->setVisible(false);
    m_uploadProgressBar->setRange(0, 1000); // We'll set max later
    m_uploadProgressBar->setValue(0);
    m_uploadProgressBar->setTextVisible(true);
    // Add it to the status bar so it appears at the bottom
    ui->statusbar->addPermanentWidget(m_uploadProgressBar, 1);
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
    qDebug() << "🔥 [ServerChatWindow] Send button clicked!";
    
    QString message = ui->typeMessageTxt->toPlainText();
    
    qDebug() << "🔥 [ServerChatWindow] Message text:" << message;
    
    if (message.isEmpty()) {
        qDebug() << "⚠️ [ServerChatWindow] Message is empty - aborting";
        return;
    }
    
    qDebug() << "✅ [ServerChatWindow] Emitting sendMessageRequested signal";
    emit sendMessageRequested(message);
    
    qDebug() << "✅ [ServerChatWindow] Clearing input field";
    ui->typeMessageTxt->clear();
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
        if (userId.contains("All Users") || userId.contains("📢")) {
            setBroadcastMode();
            emit userSelected("All Users");
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

void ServerChatWindow::showMessage(const QString &msg)
{
    // DEPRECATED: This method should not be used anymore.
    // Controller should create the widget and call addMessageItem() directly.
    qWarning() << "DEPRECATED: ServerChatWindow::showMessage(QString) called.";
    qWarning() << "Controllers should use addMessageItem(QWidget*) instead.";
    qWarning() << "Message was:" << msg;
}

void ServerChatWindow::showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                                       const QString &senderInfo, BaseChatWindow::MessageType type)
{
    // DEPRECATED: This method should not be used anymore.
    // Controller should create FileMessageItem and call addMessageItem() directly.
    qWarning() << "DEPRECATED: ServerChatWindow::showFileMessage() called.";
    qWarning() << "Controllers should use addMessageItem(QWidget*) instead.";
}

void ServerChatWindow::updateUserList(const QStringList &users)
{
    ui->userListWdgt->clear();

    // Add "Broadcast to All" option at the top
    QListWidgetItem *broadcastItem = new QListWidgetItem("📢 All Users (Broadcast)");
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
    ui->userCountLabel->setText(QString("Connected Users: %1").arg(count));
}

void ServerChatWindow::setPrivateChatMode(const QString &userId)
{
    m_isPrivateChat = true;
    m_currentTargetUser = userId;

    // Update UI to show private chat mode
    ui->chatTitleLabel->setText(QString("Private Chat with %1").arg(userId));
    ui->chatTitleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 0px; color: #ffffff;");

    ui->chatSubtitleLabel->setText("Only this user will see your messages");
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
    ui->chatTitleLabel->setText("Public Chat");
    ui->chatTitleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 0px; color: #ffffff;");

    ui->chatSubtitleLabel->setText("Your messages will be sent to everyone");
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
    QString infoText = QString("Your IP Address: %1:%2\nConnection Status: %3")
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

// --- ADD THIS ENTIRE NEW FUNCTION ---
void ServerChatWindow::onSendFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (filePath.isEmpty()) {
        return;
    }

    // **FIX: ایجاد FileMessageItem قبل از شروع آپلود**
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    qint64 fileSize = fileInfo.size();
    
    // Create FileMessageItem with temporary URL
    FileMessageItem* fileItem = new FileMessageItem(
        fileName,
        fileSize,
        "", // URL بعداً set می‌شود
        "You",
        BaseChatWindow::MessageType::Sent,
        "localhost"
    );
    
    // Set initial state to In_Progress_Upload
    fileItem->findChild<InfoCard*>()->setState(InfoCard::State::In_Progress_Upload);
    
    // Add to chat immediately
    addMessageItem(fileItem);

    // This is the server you started with ./tusd
    QUrl tusEndpoint("http://localhost:1080/files/");
    TusUploader *uploader = new TusUploader(this);

    // **FIX: اتصال progress به InfoCard**
    InfoCard* infoCard = fileItem->findChild<InfoCard*>();
    if (infoCard) {
        connect(uploader, &TusUploader::uploadProgress, infoCard, &InfoCard::updateProgress);
        
        // **NEW: اتصال دکمه Cancel**
        connect(infoCard, &InfoCard::cancelClicked, this, [=]() {
            qDebug() << "❌ Cancel clicked - canceling upload";
            
            // Cancel upload safely
            if (uploader) {
                uploader->cancelUpload();
                uploader->deleteLater();
            }
            
            // **FIX: حذف QListWidgetItem از لیست چت**
            for (int i = 0; i < ui->chatHistoryWdgt->count(); ++i) {
                QListWidgetItem* item = ui->chatHistoryWdgt->item(i);
                if (item && ui->chatHistoryWdgt->itemWidget(item) == fileItem) {
                    qDebug() << "🗑️ Removing QListWidgetItem at index:" << i;
                    delete ui->chatHistoryWdgt->takeItem(i);
                    break;
                }
            }
            
            // Delete FileMessageItem
            fileItem->deleteLater();
        });
    }

    // --- Connect signals from the uploader ---

    connect(uploader, &TusUploader::finished, this, [=](const QString &uploadUrl, qint64 uploadedSize) {
        // **FIX: به جای ساخت FileMessageItem جدید، فقط state را تغییر بده**
        if (infoCard) {
            infoCard->setState(InfoCard::State::Completed_Sent);
        }
        
        // Emit signal for saving to database and sending via WebSocket
        emit fileUploaded(fileName, uploadUrl, uploadedSize, "localhost");
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::error, this, [=](const QString &error) {
        QMessageBox::critical(this, "Upload Error", error);
        // حذف FileMessageItem در صورت خطا
        fileItem->deleteLater();
        uploader->deleteLater();
    });

    // --- Start the upload ---
    qDebug() << "Starting upload of" << filePath;
    uploader->startUpload(filePath, tusEndpoint);
}

void ServerChatWindow::handleSendMessage()
{
    onsendMessageBtnclicked();
}

void ServerChatWindow::handleFileUpload()
{
    onSendFileClicked();
}

void ServerChatWindow::addMessageItem(QWidget *messageItem)
{
    if (!messageItem) return;
    
    QListWidgetItem *item = new QListWidgetItem(ui->chatHistoryWdgt);
    item->setSizeHint(messageItem->sizeHint());
    ui->chatHistoryWdgt->addItem(item);
    ui->chatHistoryWdgt->setItemWidget(item, messageItem);
    ui->chatHistoryWdgt->scrollToBottom();
}
