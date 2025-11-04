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
            // Parse message format: "[HH:mm:] Sender: message text"
            QString senderInfo;
            QString messageText;
            
            QRegularExpression rx("^\\[([^\\]]+)\\]\\s*(.+?):\\s*(.*)$");
            QRegularExpressionMatch match = rx.match(msg);
            
            if (match.hasMatch()) {
                QString time = match.captured(1);
                QString sender = match.captured(2);
                messageText = match.captured(3);
                senderInfo = QString("[%1] %2").arg(time, sender);
            } else {
                // If parsing fails, use the whole message as text
                messageText = msg;
            }
            
            // Use TextMessageItem widget
            TextMessageItem *textItem = new TextMessageItem(messageText, senderInfo, TextMessageItem::Received, this);
            
            QListWidgetItem *item = new QListWidgetItem();
            ui->chatHistoryWdgt->addItem(item);
            ui->chatHistoryWdgt->setItemWidget(item, textItem);
            item->setSizeHint(textItem->sizeHint());
            ui->chatHistoryWdgt->scrollToBottom();
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
    mainLayout->addWidget(senderLabel, 0, Qt::AlignLeft);

    // Add file message item
    FileMessageItem *fileItem = new FileMessageItem(fileName, fileSize, fileUrl, senderInfo, messageWidget);
    mainLayout->addWidget(fileItem, 0, Qt::AlignLeft);

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
