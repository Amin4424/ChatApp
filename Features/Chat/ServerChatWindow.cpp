#include "ServerChatWindow.h"
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
#include "../FileTransfer/TusUploader.h"
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

void ServerChatWindow::showMessage(const QString &msg)
{
    if(msg.isEmpty()) return;

    // --- FIX: Declare senderInfo here to be in scope for the whole function ---
    QString senderInfo = "";

    // Check if msg is a URL to an image
    if (msg.startsWith("http://") || msg.startsWith("https://")) {
        QString lowerMsg = msg.toLower();
        if (lowerMsg.endsWith(".jpg") || lowerMsg.endsWith(".jpeg") || lowerMsg.endsWith(".png") || lowerMsg.endsWith(".gif") || lowerMsg.endsWith(".bmp")) {
            QWidget *imageWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(imageWidget);
            QLabel *imageLabel = new QLabel();
            QPixmap pixmap(msg);
            if (!pixmap.isNull()) {
                imageLabel->setPixmap(pixmap.scaled(200, 200, Qt::KeepAspectRatio));
            } else {
                imageLabel->setText("Failed to load image");
            }
            QLabel *urlLabel = new QLabel(QString("<a href='%1'>%1</a>").arg(msg));
            urlLabel->setOpenExternalLinks(true);
            layout->addWidget(imageLabel);
            layout->addWidget(urlLabel);
            layout->setContentsMargins(10, 10, 10, 10);
            QListWidgetItem *item = new QListWidgetItem();
            item->setSizeHint(imageWidget->sizeHint());
            ui->chatHistoryWdgt->addItem(item);
            ui->chatHistoryWdgt->setItemWidget(item, imageWidget);
        } else {
            QLabel *urlLabel = new QLabel(QString("<a href='%1'>%1</a>").arg(msg));
            urlLabel->setOpenExternalLinks(true);
            QListWidgetItem *item = new QListWidgetItem();
            item->setSizeHint(urlLabel->sizeHint());
            ui->chatHistoryWdgt->addItem(item);
            ui->chatHistoryWdgt->setItemWidget(item, urlLabel);
        }
    } else {
        // --- UPDATED LOGIC TO HANDLE HISTORICAL AND NEW MESSAGES ---

        // --- FIX: senderInfo is already declared above ---
        QString messageText;
        QString sender; // Just the sender part, e.g., "You", "Server", "کاربر شماره 1"
        QString time;   // The timestamp

        // Regex 1: [Time] Sender: Message (for new messages from controller)
        QRegularExpression rx_new("^\\[([^\\]]+)\\]\\s*(.+?):\\s*(.*)$");
        // Regex 2: [Time] Sender -> Receiver: Message (for historical messages from DB)
        QRegularExpression rx_hist("^\\[([^\\]]+)\\]\\s*([^\\s]+)\\s*->\\s*([^:]+):\\s*(.*)$");

        QRegularExpressionMatch match_hist = rx_hist.match(msg);
        QRegularExpressionMatch match_new = rx_new.match(msg);

        if (match_hist.hasMatch()) {
            // Historical format: [Time] Sender -> Receiver: Message
            time = match_hist.captured(1);
            sender = match_hist.captured(2); // "Server" or "کاربر شماره 1"
            QString receiver = match_hist.captured(3); // "All" or "Server"
            messageText = match_hist.captured(4); // "Hello" or "FILE|..."

            // --- FIX: Remove "-> Server" but keep "-> All" etc. ---
            if (receiver == "Server") {
                senderInfo = QString("[%1] %2").arg(time, sender); // [10:51] کاربر 1
            } else {
                senderInfo = QString("[%1] %2 -> %3").arg(time, sender, receiver); // [10:51] Server -> All
            }
            // --- END FIX ---

        } else if (match_new.hasMatch()) {
            // New format: [Time] Sender: Message
            time = match_new.captured(1);
            sender = match_new.captured(2); // "You (to all)" or "کاربر شماره 1"
            messageText = match_new.captured(3); // "Hello" or "FILE|..."

            // --- FIX: Clean up "You (to all)" ---
            if (sender.contains(" (to")) {
                sender = sender.split(" (to").first().trimmed(); // "You"
            }
            senderInfo = QString("[%1] %2").arg(time, sender); // Rebuild senderInfo
            // --- END FIX ---

        } else {
            // Fallback
            messageText = msg;
            senderInfo = "";
            sender = "";
        }

        // Check if the *messageText* is a file
        if (messageText.startsWith("FILE|")) {
            QStringList parts = messageText.split("|");
            if (parts.size() >= 4) {
                QString fileName = parts[1];
                qint64 fileSize = parts[2].toLongLong();
                QString fileUrl = parts[3];
                // Call the *other* function to handle file display
                // Pass the *CLEANED* senderInfo
                showFileMessage(fileName, fileSize, fileUrl, senderInfo);
            }
        } else {
            // It's a regular text message

            // Determine type based on sender name
            TextMessageItem::MessageType type;
            if (sender == "Server" || sender == "You") {
                type = TextMessageItem::Sent;
            } else {
                type = TextMessageItem::Received;
            }

            // Create TextMessageItem widget
            // Pass the *CLEANED* senderInfo
            TextMessageItem *textItem = new TextMessageItem(messageText, senderInfo, type, this);

            // Add to list
            QListWidgetItem *item = new QListWidgetItem();

            // Set the size hint for the item to match the widget's calculated height
            item->setSizeHint(textItem->sizeHint());

            ui->chatHistoryWdgt->addItem(item);
            ui->chatHistoryWdgt->setItemWidget(item, textItem);

            ui->chatHistoryWdgt->scrollToBottom();
        }
        // --- END UPDATED LOGIC ---
    }

    // Only clear text if it was a "You" message
    if (senderInfo.contains("You")) {
        ui->typeMessageTxt->clear();
    }
}

void ServerChatWindow::showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &senderInfo)
{
    if(fileName.isEmpty() || fileUrl.isEmpty()) return;

    // Create a container widget for the sender info and file item
    QWidget *messageWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(messageWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(2);

    // Add sender info label (it's already clean from showMessage)
    QLabel *senderLabel = new QLabel(senderInfo);
    senderLabel->setStyleSheet("font-size: 10px; color: #666; margin-bottom: 2px;");

    // --- FIX: Align sender label based on "You" or "Server" ---
    // We need to parse the sender from the (now clean) senderInfo
    QString sender = "";
    // Format is now either "[Time] Sender" or "[Time] Sender -> All"
    QRegularExpression rx_clean("^\\[([^\\]]+)\\]\\s*([^\\s]+).*$"); // Gets the first word after [Time]
    QRegularExpressionMatch match_clean = rx_clean.match(senderInfo);
    if (match_clean.hasMatch()) {
        sender = match_clean.captured(2); // "You", "Server", "کاربر"
    }

    bool isSent = (sender == "Server" || sender == "You");

    if (isSent) {
        mainLayout->addWidget(senderLabel, 0, Qt::AlignLeft); // Align left (renders right in RTL)
    } else {
        mainLayout->addWidget(senderLabel, 0, Qt::AlignRight); // Align right (renders left in RTL)
    }
    // --- END FIX ---

    // Add file message item
    FileMessageItem *fileItem = new FileMessageItem(fileName, fileSize, fileUrl, senderInfo, messageWidget);

    // --- FIX: Align the FileMessageItem itself ---
    if (isSent) {
        mainLayout->addWidget(fileItem, 0, Qt::AlignLeft);
    } else {
        mainLayout->addWidget(fileItem, 0, Qt::AlignRight);
    }
    // --- END FIX ---

    QListWidgetItem *item = new QListWidgetItem();

    // Set the size hint for the item to match the widget's calculated height
    item->setSizeHint(messageWidget->sizeHint());

    ui->chatHistoryWdgt->addItem(item);
    ui->chatHistoryWdgt->setItemWidget(item, messageWidget);

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
        emit fileUploaded(QFileInfo(filePath).fileName(), uploadUrl, fileSize);
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

