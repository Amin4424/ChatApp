#include "UI/ServerChatWindow.h"
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
#include "Network/TusUploader.h"
#include "FileMessageItem.h"
#include "TextMessageItem.h"
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
    if(msg.isEmpty()) return;

    // --- *** FIX: Declare senderInfo here *** ---
    QString senderInfo = "";

    // Check if msg is a URL (simplified check)
    if (msg.startsWith("http://") || msg.startsWith("https://")) {
        // ... (rest of URL/Image logic) ...
        // For simplicity, we'll just show it as a plain text message

        // Fallback to show as text
        QListWidgetItem *item = new QListWidgetItem(msg);
        ui->chatHistoryWdgt->addItem(item);
        ui->chatHistoryWdgt->scrollToBottom();

    } else {
        // --- UPDATED LOGIC TO HANDLE HISTORICAL AND NEW MESSAGES ---

        QString messageText;
        QString sender;
        QString timestamp;

        // Try new pipe-separated format first (from loadMessagesBetween)
        if (msg.contains("|") && !msg.startsWith("FILE|")) {
            QStringList parts = msg.split("|");
            if (parts.size() >= 3) {
                timestamp = parts[0];  // "11:28"
                sender = parts[1];     // "You", "Server", or client ID
                messageText = parts.mid(2).join("|");  // rejoin in case message contains |
                senderInfo = sender;
            } else {
                // Fallback
                messageText = msg;
                senderInfo = "";
                sender = "";
            }
        } else {
            // Old bracket format
            
            // Regex 1: [Time] Sender: Message (for new messages from controller)
            QRegularExpression rx_new("^\\[([^\\]]+)\\]\\s*(.+?):\\s*(.*)$");
            // Regex 2: [Time] Sender -> Receiver: Message (for historical messages from DB)
            QRegularExpression rx_hist("^\\[([^\\]]+)\\]\\s*([^\\s]+)\\s*->\\s*([^:]+):\\s*(.*)$");

            QRegularExpressionMatch match_hist = rx_hist.match(msg);
            QRegularExpressionMatch match_new = rx_new.match(msg);

            if (match_hist.hasMatch()) {
                // Historical format: [Time] Sender -> Receiver: Message
                timestamp = match_hist.captured(1);  // Extract time
                sender = match_hist.captured(2); // "Server" or client ID
                QString receiver = match_hist.captured(3); // "All" or "Server"
                messageText = match_hist.captured(4);

                // Don't include time in senderInfo, only sender/receiver
                if (receiver == "Server") {
                    senderInfo = sender;
                } else {
                    senderInfo = QString("%1 -> %2").arg(sender, receiver);
                }

            } else if (match_new.hasMatch()) {
                // New format: [Time] Sender: Message
                timestamp = match_new.captured(1);  // Extract time
                sender = match_new.captured(2);
                messageText = match_new.captured(3);

                // Clean up "You (to all)"
                if (sender.contains(" (to")) {
                    sender = sender.split(" (to").first().trimmed();
                }
                senderInfo = sender;  // No time in senderInfo!

            } else {
                // Fallback
                messageText = msg;
                senderInfo = "";
                sender = "";
                timestamp = "";
            }
        }

            // --- DETERMINE MESSAGE TYPE ---
            BaseChatWindow::MessageType type;
            if (sender == "Server" || sender == "You") {
                type = BaseChatWindow::MessageType::Sent;
            } else {
                type = BaseChatWindow::MessageType::Received;
            }        // Check if the *messageText* is a file
        if (messageText.startsWith("FILE|")) {
            QStringList parts = messageText.split("|");
            if (parts.size() >= 4) {
                QString fileName = parts[1];
                qint64 fileSize = parts[2].toLongLong();
                QString fileUrl = parts[3];
                // --- *** FIX: Call the NEW showFileMessage *** ---
                showFileMessage(fileName, fileSize, fileUrl, senderInfo, type);
            }
        } else {
            // It's a regular text message
            TextMessageItem *textItem = new TextMessageItem(messageText, senderInfo, type, timestamp, this);
            QListWidgetItem *item = new QListWidgetItem();
            item->setSizeHint(textItem->sizeHint());
            ui->chatHistoryWdgt->addItem(item);
            ui->chatHistoryWdgt->setItemWidget(item, textItem);
            ui->chatHistoryWdgt->scrollToBottom();
        }
        // --- END UPDATED LOGIC ---
    }

    // --- *** FIX: Check senderInfo, not msg *** ---
    if (senderInfo.contains("You")) {
        ui->typeMessageTxt->clear();
    }
}

void ServerChatWindow::showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                                       const QString &senderInfo, BaseChatWindow::MessageType type)
{
    if(fileName.isEmpty() || fileUrl.isEmpty()) return;

    // Create the FileMessageItem directly - Server uses localhost
    FileMessageItem *fileItem = new FileMessageItem(fileName, fileSize, fileUrl,
                                                    senderInfo, type, "localhost", this);

    QListWidgetItem *item = new QListWidgetItem();

    // --- Set the size hint ---
    item->setSizeHint(fileItem->sizeHint());

    ui->chatHistoryWdgt->addItem(item);
    ui->chatHistoryWdgt->setItemWidget(item, fileItem);
    ui->chatHistoryWdgt->scrollToBottom();

    // Only clear text if it was a "You" message
    if (senderInfo.contains("You")) {
        ui->typeMessageTxt->clear();
    }
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

    // This is the server you started with ./tusd
    QUrl tusEndpoint("http://localhost:1080/files/");

    TusUploader *uploader = new TusUploader(this);

    // --- Connect signals from the uploader ---

    connect(uploader, &TusUploader::finished, this, [=](const QString &uploadUrl, qint64 fileSize) {
        m_uploadProgressBar->setVisible(false);
        // --- FIX: Server uploads to localhost, so pass empty string (default) ---
        emit fileUploaded(QFileInfo(filePath).fileName(), uploadUrl, fileSize, "localhost");
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::error, this, [=](const QString &error) {
        m_uploadProgressBar->setVisible(false);
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::uploadProgress, this, [=](qint64 sent, qint64 total) {
        if (!m_uploadProgressBar->isVisible()) {
            m_uploadProgressBar->setVisible(true);
        }
        // Update progress bar
        if(m_uploadProgressBar->maximum() != total) {
            m_uploadProgressBar->setMaximum(total);
        }
        m_uploadProgressBar->setValue(sent);

        if (total > 0) {
            double percent = (static_cast<double>(sent) / static_cast<double>(total)) * 100.0;
            m_uploadProgressBar->setFormat(QString("Uploading... %1%").arg(percent, 0, 'f', 1));
        }
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
