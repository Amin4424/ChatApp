#include "View/ClientChatWindow.h"
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
#include "TusUploader.h"
#include "FileMessageItem.h"
#include "TextMessageItem.h"
#include "InfoCard.h"
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
    QString message = ui->typeMessageTxt->toPlainText().trimmed();
    
    if (message.isEmpty()) {
        QMessageBox::warning(this, "Empty Message", "Please type a message before sending!");
        return;
    }
    emit sendMessageRequested(message);
    // Clear the text box after sending
    ui->typeMessageTxt->clear();
}

void ClientChatWindow::onchatHistoryWdgtitemClicked(QListWidgetItem *item)
{
    qDebug() << "You clicked on message:" << item->text();
}

// void ClientChatWindow::showMessage(const QString &msg)
// {
//     // DEPRECATED: This method should not be used anymore.
//     // Controller should create the widget and call addMessageItem() directly.
//     qWarning() << "DEPRECATED: ClientChatWindow::showMessage(QString) called.";
//     qWarning() << "Controllers should use addMessageItem(QWidget*) instead.";
//     qWarning() << "Message was:" << msg;
// }

// void ClientChatWindow::showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl,
//                                        const QString &senderInfo, BaseChatWindow::MessageType type)
// {
//     // DEPRECATED: This method should not be used anymore.
//     // Controller should create FileMessageItem and call addMessageItem() directly.
//     qWarning() << "DEPRECATED: ClientChatWindow::showFileMessage() called.";
//     qWarning() << "Controllers should use addMessageItem(QWidget*) instead.";
// }

void ClientChatWindow::updateConnectionInfo(const QString &serverUrl, const QString &status)
{
    QString infoText = QString("Connection Status: %1").arg(status);
    ui->clientConnectionLabel->setText(infoText);
}

void ClientChatWindow::onReconnectClicked()
{
    emit reconnectRequested();
}

void ClientChatWindow::onSendFileClicked()
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
        m_serverHost
    );
    
    // Set initial state to In_Progress_Upload
    fileItem->findChild<InfoCard*>()->setState(InfoCard::State::In_Progress_Upload);
    
    // Add to chat immediately
    addMessageItem(fileItem);

    // --- Start TUS Upload ---
    QString tusEndpointUrl = QString("http://%1:1080/files/").arg(m_serverHost);
    QUrl tusEndpoint(tusEndpointUrl);
    TusUploader *uploader = new TusUploader(this);

    // **FIX: اتصال progress به InfoCard**
    InfoCard* infoCard = fileItem->findChild<InfoCard*>();
    if (infoCard) {
        connect(uploader, &TusUploader::uploadProgress, infoCard, &InfoCard::updateProgress);
        
        // **NEW: اتصال دکمه Cancel**
        connect(infoCard, &InfoCard::cancelClicked, this, [=]() {
            
            // Cancel upload safely
            if (uploader) {
                uploader->cancelUpload();
                uploader->deleteLater();
            }
            
            // **FIX: حذف QListWidgetItem از لیست چت**
            for (int i = 0; i < ui->chatHistoryWdgt->count(); ++i) {
                QListWidgetItem* item = ui->chatHistoryWdgt->item(i);
                if (item && ui->chatHistoryWdgt->itemWidget(item) == fileItem) {
                    delete ui->chatHistoryWdgt->takeItem(i);
                    break;
                }
            }
            // Delete FileMessageItem
            fileItem->deleteLater();
        });
    }

    connect(uploader, &TusUploader::finished, this, [=](const QString &uploadUrl, qint64 uploadedSize) {
        // **FIX: به جای ساخت FileMessageItem جدید، فقط state را تغییر بده**
        if (infoCard) {
            infoCard->setState(InfoCard::State::Completed_Sent);
        }
        
        // Emit signal for saving to database and sending via WebSocket
        emit fileUploaded(fileName, uploadUrl, uploadedSize, m_serverHost);
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::error, this, [=](const QString &error) {
        QMessageBox::critical(this, "Upload Error", error);
        // حذف FileMessageItem در صورت خطا
        fileItem->deleteLater();
        uploader->deleteLater();
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

void ClientChatWindow::addMessageItem(QWidget *messageItem)
{
    if (!messageItem) return;
    
    QListWidgetItem *item = new QListWidgetItem(ui->chatHistoryWdgt);
    item->setSizeHint(messageItem->sizeHint());
    ui->chatHistoryWdgt->addItem(item);
    ui->chatHistoryWdgt->setItemWidget(item, messageItem);
    ui->chatHistoryWdgt->scrollToBottom();
}
