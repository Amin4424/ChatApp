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
    , m_serverHost("localhost") // --- INITIALIZE server host
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

    // --- *** FIX: Declare senderInfo here *** ---
    QString senderInfo = "";

    if(!msg.isEmpty())
    {
        // Check if msg is a URL (simplified check)
        if (msg.startsWith("http://") || msg.startsWith("https://")) {
            // ... (rest of URL/Image logic) ...
            // For simplicity, we'll just show it as a plain text message

            // Fallback to show as text
            QListWidgetItem *item = new QListWidgetItem(msg);
            ui->chatHistoryWdgt->addItem(item);
            ui->chatHistoryWdgt->scrollToBottom();

        } else {
            // Parse message format: "[HH:mm:] Sender: message text"
            // --- *** FIX: senderInfo already declared above *** ---
            QString messageText;
            QString sender; // Just the sender part, e.g., "You", "Server"

            QRegularExpression rx_new("^\\[([^\\]]+)\\]\\s*(.+?):\\s*(.*)$");
            QRegularExpression rx_hist("^\\[([^\\]]+)\\]\\s*([^\\s]+)\\s*->\\s*([^:]+):\\s*(.*)$");

            QRegularExpressionMatch match_hist = rx_hist.match(msg);
            QRegularExpressionMatch match_new = rx_new.match(msg);

            if (match_hist.hasMatch()) {
                QString time = match_hist.captured(1);
                sender = match_hist.captured(2); // "Server" or "Client"
                messageText = match_hist.captured(4);

                if (sender == "Client") {
                    sender = "You";
                }
                senderInfo = QString("[%1] %2").arg(time, sender);

            } else if (match_new.hasMatch()) {
                QString time = match_new.captured(1);
                sender = match_new.captured(2); // "You" or "Server"
                messageText = match_new.captured(3);
                senderInfo = QString("[%1] %2").arg(time, sender);
            } else {
                messageText = msg;
                senderInfo = "";
                sender = "";
            }

            // --- DETERMINE MESSAGE TYPE ---
            BaseChatWindow::MessageType type;
            if (sender.contains("You")) {
                type = BaseChatWindow::MessageType::Sent;
            } else {
                type = BaseChatWindow::MessageType::Received;
            }

            // Check if the *messageText* is a file
            if (messageText.startsWith("FILE|")) {
                QStringList parts = messageText.split("|");
                if (parts.size() >= 4) {
                    QString fileName = parts[1];
                    qint64 fileSize = parts[2].toLongLong();
                    QString fileUrl = parts[3];
                    // --- CALL THE NEW showFileMessage ---
                    showFileMessage(fileName, fileSize, fileUrl, senderInfo, type);
                }
            } else {
                // It's a regular text message
                TextMessageItem *textItem = new TextMessageItem(messageText, senderInfo, type, this);
                QListWidgetItem *item = new QListWidgetItem();
                item->setSizeHint(textItem->sizeHint());
                ui->chatHistoryWdgt->addItem(item);
                ui->chatHistoryWdgt->setItemWidget(item, textItem);
                ui->chatHistoryWdgt->scrollToBottom();
            }
        }

        // --- *** FIX: Check senderInfo, not msg *** ---
        if (senderInfo.contains("You")) {
            ui->typeMessageTxt->clear();
        }
    }
    qDebug() << "=== End ClientChatWindow::showMessage ===";
}

void ClientChatWindow::showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                                       const QString &senderInfo, BaseChatWindow::MessageType type)
{
    if(fileName.isEmpty() || fileUrl.isEmpty()) return;

    // Create the FileMessageItem directly - PASS server host
    FileMessageItem *fileItem = new FileMessageItem(fileName, fileSize, fileUrl,
                                                    senderInfo, type, m_serverHost, this);

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

void ClientChatWindow::onSendFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (filePath.isEmpty()) {
        return;
    }

    // --- FIX: Use the stored server host ---
    QString tusEndpointUrl = QString("http://%1:1080/files/").arg(m_serverHost);
    QUrl tusEndpoint(tusEndpointUrl);
    TusUploader *uploader = new TusUploader(this);

    connect(uploader, &TusUploader::finished, this, [=](const QString &uploadUrl, qint64 fileSize) {
        m_uploadProgressBar->setVisible(false);
        // --- FIX: Pass server host as the 4th parameter ---
        emit fileUploaded(QFileInfo(filePath).fileName(), uploadUrl, fileSize, m_serverHost);
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::error, this, [=](const QString &error) {
        m_uploadProgressBar->setVisible(false);
        QMessageBox::critical(this, "Upload Error", error);
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::uploadProgress, this, [=](qint64 sent, qint64 total) {
        if (!m_uploadProgressBar->isVisible()) {
            m_uploadProgressBar->setVisible(true);
        }
        if(m_uploadProgressBar->maximum() != total) {
            m_uploadProgressBar->setMaximum(total);
        }
        m_uploadProgressBar->setValue(sent);

        if (total > 0) {
            double percent = (static_cast<double>(sent) / static_cast<double>(total)) * 100.0;
            m_uploadProgressBar->setFormat(QString("Uploading... %1%").arg(percent, 0, 'f', 1));
        }
    });

    qDebug() << "Starting upload of" << filePath << "to" << tusEndpointUrl;
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

