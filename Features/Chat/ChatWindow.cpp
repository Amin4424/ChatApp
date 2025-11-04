#include "ClientChatWindow.h"
#include "ui_ClientChatWindow.h"
#include <QKeyEvent>
#include <QDebug>
#include <QRegularExpression>

// --- ADD THESE INCLUDES ---
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QFileInfo>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include "../FileTransfer/TusUploader.h" // <-- CORRECTED PATH
#include "FileMessageItem.h"
#include "TextMessageItem.h"
// ---

ClientChatWindow::ClientChatWindow(QWidget *parent)
    : BaseChatWindow(parent)
    , ui(new Ui::ClientChatWindow)
    , m_uploadProgressBar(nullptr) // <-- INITIALIZE
{
    ui->setupUi(this);

    // Connect send button click to slot
    connect(ui->sendMessageBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onsendMessageBtnclicked);

    // --- ADD THIS CONNECTION ---
    connect(ui->sendFileBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onSendFileClicked);

    // Connect reconnect button
    connect(ui->reconnectBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onReconnectClicked);

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

ClientChatWindow::~ClientChatWindow()
{
    delete ui;
}

bool ClientChatWindow::eventFilter(QObject *obj, QEvent *event)
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

void ClientChatWindow::onsendMessageBtnclicked()
{
    QString message = ui->typeMessageTxt->toPlainText();
    emit sendMessageRequested(message);
}

void ClientChatWindow::onchatHistoryWdgtitemClicked(QListWidgetItem *item)
{
    qDebug() << "You clicked on message:" << item->text();
}

void ClientChatWindow::showMessage(const QString &msg)
{
    qDebug() << "=== ClientChatWindow::showMessage called ===";
    qDebug() << "Message:" << msg;
    qDebug() << "Message length:" << msg.length();
    qDebug() << "Is empty?" << msg.isEmpty();

    if(!msg.isEmpty())
    {
        // Check if msg is a URL to an image
        if (msg.startsWith("http://") || msg.startsWith("https://")) {
            QString lowerMsg = msg.toLower();
            if (lowerMsg.endsWith(".jpg") || lowerMsg.endsWith(".jpeg") || lowerMsg.endsWith(".png") || lowerMsg.endsWith(".gif") || lowerMsg.endsWith(".bmp")) {
                // Create a custom widget for the image
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
                // Regular URL, make it clickable
                QLabel *urlLabel = new QLabel(QString("<a href='%1'>%1</a>").arg(msg));
                urlLabel->setOpenExternalLinks(true);

                QListWidgetItem *item = new QListWidgetItem();
                item->setSizeHint(urlLabel->sizeHint());
                ui->chatHistoryWdgt->addItem(item);
                ui->chatHistoryWdgt->setItemWidget(item, urlLabel);
            }
        } else {
            // --- UPDATED LOGIC TO HANDLE HISTORICAL MESSAGES ---

            // Parse message format: "[HH:mm:] Sender: message text"
            // Handles historical: [08:50] You: ...
            // And historical: [08:50] Server: ...
            // And new: [08:51] You: ... (from controller)
            QString senderInfo;
            QString messageText;
            QString sender; // "You" or "Server"

            // Regex 1: [Time] Sender: Message (for new messages from controller OR client historical)
            QRegularExpression rx_new("^\\[([^\\]]+)\\]\\s*(.+?):\\s*(.*)$");
            // Regex 2: [Time] Sender -> Receiver: Message (for server historical)
            QRegularExpression rx_hist("^\\[([^\\]]+)\\]\\s*([^\\s]+)\\s*->\\s*([^:]+):\\s*(.*)$");

            QRegularExpressionMatch match_hist = rx_hist.match(msg);
            QRegularExpressionMatch match_new = rx_new.match(msg);

            if (match_hist.hasMatch()) {
                // Historical format: [Time] Sender -> Receiver: Message
                QString time = match_hist.captured(1);
                sender = match_hist.captured(2); // "Server" or "Client"
                QString receiver = match_hist.captured(3);
                messageText = match_hist.captured(4); // "Hello" or "FILE|..."

                // --- FIX: Show correct name for historical ---
                if (sender == "Client") {
                    sender = "You";
                }
                senderInfo = QString("[%1] %2").arg(time, sender);
                // --- END FIX ---

            } else if (match_new.hasMatch()) {
                // New format: [Time] Sender: Message
                QString time = match_new.captured(1);
                sender = match_new.captured(2); // "You" or "Server"
                messageText = match_new.captured(3); // "Hello" or "FILE|..."
                senderInfo = QString("[%1] %2").arg(time, sender);
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
                    showFileMessage(fileName, fileSize, fileUrl, senderInfo);
                }
            } else {
                // It's a regular text message

                // --- FIX: Determine type based on sender name ---
                // This now works for "[Time] You: ..."
                TextMessageItem::MessageType type = TextMessageItem::Received;
                if (sender == "You") { // Client check is simple
                    type = TextMessageItem::Sent;
                }

                // Use TextMessageItem widget
                TextMessageItem *textItem = new TextMessageItem(messageText, senderInfo, type, this);
                // --- END FIX ---

                QListWidgetItem *item = new QListWidgetItem();
                ui->chatHistoryWdgt->addItem(item);
                ui->chatHistoryWdgt->setItemWidget(item, textItem);
                item->setSizeHint(textItem->sizeHint());
                ui->chatHistoryWdgt->scrollToBottom();
            }
            // --- END UPDATED LOGIC ---
        }
        ui->typeMessageTxt->clear();
    }
    qDebug() << "=== End ClientChatWindow::showMessage ===";
}

void ClientChatWindow::showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &senderInfo)
{
    if(fileName.isEmpty() || fileUrl.isEmpty()) return;

    // Create a container widget for the sender info and file item
    QWidget *messageWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(messageWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(2);

    // Add sender info label
    QLabel *senderLabel = new QLabel(senderInfo);
    senderLabel->setStyleSheet("font-size: 10px; color: #666; margin-bottom: 2px;");

    // --- FIX: Align sender label based on "You" ---
    // This works for "[Time] You"
    bool isSent = senderInfo.contains("You");

    if (isSent) {
        mainLayout->addWidget(senderLabel, 0, Qt::AlignLeft); // Align left (renders right in RTL)
    } else {
        mainLayout->addWidget(senderLabel, 0, Qt::AlignRight); // Align right (renders left in RTL)
    }
    // --- END FIX ---

    // Add file message item
    FileMessageItem *fileItem = new FileMessageItem(fileName, fileSize, fileUrl, senderInfo, messageWidget);

    // --- FIX: Align file item based on "You" ---
    if (isSent) {
        mainLayout->addWidget(fileItem, 0, Qt::AlignLeft); // Align left (renders right in RTL)
    } else {
        mainLayout->addWidget(fileItem, 0, Qt::AlignRight); // Align right (renders left in RTL)
    }
    // --- END FIX ---

    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(messageWidget->sizeHint());
    ui->chatHistoryWdgt->addItem(item);
    ui->chatHistoryWdgt->setItemWidget(item, messageWidget);

    ui->chatHistoryWdgt->scrollToBottom();
    ui->typeMessageTxt->clear();
}

void ClientChatWindow::updateConnectionInfo(const QString &serverUrl, const QString &status)
{
    QString infoText = QString("وضعیت اتصال : %1").arg(status);
    ui->clientConnectionLabel->setText(infoText);
}

void ClientChatWindow::onReconnectClicked()
{
    qDebug() << "Reconnect button clicked";
    emit reconnectRequested();
}

// --- ADD THIS ENTIRE NEW FUNCTION ---
void ClientChatWindow::onSendFileClicked()
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

void ClientChatWindow::handleSendMessage()
{
    onsendMessageBtnclicked();
}

void ClientChatWindow::handleFileUpload()
{
    onSendFileClicked();
}

