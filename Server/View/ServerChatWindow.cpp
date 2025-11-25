#include "View/ServerChatWindow.h"
#include "ui_ServerChatWindow.h"
#include "ModernThemeApplier.h"
#include "ModernTheme.h"
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
#include <QListWidget>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QTextEdit>
#include <QTextCursor>
#include <QTimer>
#include <QStyle>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QMediaRecorder>
#include <QAudioInput>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QMediaFormat>
#include <QMediaCaptureSession>
#else
#include <QAudioRecorder>
#include <QAudioEncoderSettings>
#include <QVideoEncoderSettings>
#include <QMultimedia>
#endif
#include "TusUploader.h"
#include "AudioWaveform.h"
#include "MessageAliases.h"
#include "InfoCard.h"
#include "ToastNotification.h"
// ---

ServerChatWindow::ServerChatWindow(QWidget *parent)
    : BaseChatWindow(parent)
    , ui(new Ui::ServerChatWindow)
    , m_isPrivateChat(false)
    , m_currentTargetUser("")
    , m_uploadProgressBar(nullptr) // <-- INITIALIZE
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    , m_mediaRecorder(nullptr)
    , m_audioInput(nullptr)
    , m_captureSession(nullptr)
#else
    , m_audioRecorder(nullptr)
#endif
    , m_isRecording(false)
{
    ui->setupUi(this);
    m_sendButtonDefaultIcon = ui->sendMessageBtn->icon();
    m_sendButtonDefaultText = ui->sendMessageBtn->text();
    m_sendButtonEditIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    
    // Check if recordVoiceBtn exists in UI
    if (ui->recordVoiceBtn) {
        m_recordButtonDefaultStyle = ui->recordVoiceBtn->styleSheet();
        connect(ui->recordVoiceBtn, &QPushButton::clicked,
                this, &ServerChatWindow::onRecordVoiceButtonClicked);
    }
    
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Initialize capture session, media recorder and audio input
    m_captureSession = new QMediaCaptureSession(this);
    m_mediaRecorder = new QMediaRecorder(this);
    m_captureSession->setRecorder(m_mediaRecorder);
    
    // Connect state change signal
    connect(m_mediaRecorder, &QMediaRecorder::recorderStateChanged,
            this, &ServerChatWindow::onRecorderStateChanged);
#endif
    
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
    connect(ui->typeMessageTxt, &QTextEdit::textChanged,
            this, &ServerChatWindow::updateInputModeButtons);

    // --- SETUP PROGRESS BAR ---
    m_uploadProgressBar = new QProgressBar(this);
    m_uploadProgressBar->setVisible(false);
    m_uploadProgressBar->setRange(0, 1000); // We'll set max later
    m_uploadProgressBar->setValue(0);
    m_uploadProgressBar->setTextVisible(true);
    // Add it to the status bar so it appears at the bottom
    ui->statusbar->addPermanentWidget(m_uploadProgressBar, 1);
    
    updateInputModeButtons();
    updateSendButtonForEditState();
    
    // Apply Modern Theme
    applyModernTheme();
}

void ServerChatWindow::applyModernTheme()
{
    ModernThemeApplier::applyToMainWindow(this);

    // Apply to headers - FORCE white backgrounds
    if (ui->chatHeader) {
        ModernThemeApplier::applyToHeader(ui->chatHeader);
    }
    
    // Apply to main components
    ModernThemeApplier::applyToPrimaryButton(ui->sendMessageBtn);
    ModernThemeApplier::applyToIconButton(ui->sendFileBtn);
    if (ui->recordVoiceBtn) {
        ModernThemeApplier::applyToIconButton(ui->recordVoiceBtn);
    }
    ModernThemeApplier::applyToTextInput(ui->typeMessageTxt);
    ModernThemeApplier::applyToListWidget(ui->userListWdgt);
    ModernThemeApplier::applyToChatArea(ui->chatHistoryWdgt);
    
    // Apply to sidebar
    if (ui->serverInfoSection) {
        ModernThemeApplier::applyToSidebar(ui->serverInfoSection);
    }
    
    // Apply to restart button as support banner with new text
    if (ui->restartServerBtn) {
        ui->restartServerBtn->setText(tr("Having trouble?\nContact Technical Support"));
        ModernThemeApplier::applyToSupportBanner(ui->restartServerBtn);
    }
}

ServerChatWindow::~ServerChatWindow()
{
    stopVoiceRecording(false);
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
    
    QString message = ui->typeMessageTxt->toPlainText().trimmed();
    
    qDebug() << "🔥 [ServerChatWindow] Message text:" << message;
    
    if (message.isEmpty()) {
        qDebug() << "⚠️ [ServerChatWindow] Message is empty - aborting";
        return;
    }

    if (m_editingMessageItem) {
        TextMessageItem *itemBeingEdited = m_editingMessageItem;
        emit messageEditConfirmed(itemBeingEdited, message);
        exitMessageEditMode();
        return;
    }
    
    qDebug() << "✅ [ServerChatWindow] Emitting sendMessageRequested signal";
    emit sendMessageRequested(message);
    
    qDebug() << "✅ [ServerChatWindow] Clearing input field";
    clearComposerText();
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
    // Update the title to show count instead of separate label
    ui->userListTitle->setText(QString("Recent Conversations (%1)").arg(count));
}

void ServerChatWindow::setPrivateChatMode(const QString &userId)
{
    m_isPrivateChat = true;
    m_currentTargetUser = userId;

    // Update UI to show private chat mode
    ui->chatTitleLabel->setText(QString("Private Chat with %1").arg(userId));
    ui->chatTitleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 0px; color: #2C2C2C;");

    ui->chatSubtitleLabel->setText("Only this user will see your messages");
    ui->chatSubtitleLabel->setStyleSheet("font-size: 10pt; padding: 2px; color: #666666;");

    // Keep the chatHeader white with modern theme
    QWidget *header = this->findChild<QWidget*>("chatHeader");
    if (header) {
        header->setStyleSheet(
            "QWidget#chatHeader {"
            "    background-color: #FFFFFF;"
            "    border-bottom: 1px solid #E0E0E0;"
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
    ui->chatTitleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; padding: 0px; color: #2C2C2C;");

    ui->chatSubtitleLabel->setText("Your messages will be sent to everyone");
    ui->chatSubtitleLabel->setStyleSheet("font-size: 10pt; padding: 2px; color: #666666;");

    // Keep the chatHeader white with modern theme
    QWidget *header = this->findChild<QWidget*>("chatHeader");
    if (header) {
        header->setStyleSheet(
            "QWidget#chatHeader {"
            "    background-color: #FFFFFF;"
            "    border-bottom: 1px solid #E0E0E0;"
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
    const QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
    FileMessageItem* fileItem = new FileMessageItem(
        fileName,
        fileSize,
        "", // URL بعداً set می‌شود
        "You",
        MessageDirection::Outgoing,
        "localhost",
        timestamp
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
        qDebug() << "🔵 [FILE UPLOAD] TusUploader finished - fileName:" << fileName;
        
        // Update file item with actual URL
        if (fileItem) {
            fileItem->setFileUrl(uploadUrl);
        }
        
        // **FIX: به جای ساخت FileMessageItem جدید، فقط state را تغییر بده**
        if (infoCard) {
            infoCard->setState(InfoCard::State::Completed_Sent);
        }
        
        // Emit signal for saving to database and sending via WebSocket
        qDebug() << "📤 [FILE UPLOAD] Emitting fileUploaded signal for file:" << fileName;
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

void ServerChatWindow::removeMessageItem(QWidget *messageWidget)
{
    if (!messageWidget || !ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        if (list->itemWidget(item) == messageWidget) {
            QListWidgetItem *removed = list->takeItem(index);
            delete removed;
            messageWidget->deleteLater();
            break;
        }
    }
}

void ServerChatWindow::removeMessageByDatabaseId(int databaseId)
{
    qDebug() << "🔍 [SERVER removeMessageByDatabaseId] Looking for message ID:" << databaseId;
    
    if (!ui || !ui->chatHistoryWdgt) {
        qDebug() << "❌ [SERVER removeMessageByDatabaseId] UI or chatHistoryWdgt is null";
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    qDebug() << "🔍 [SERVER removeMessageByDatabaseId] Total messages in list:" << list->count();
    
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        int widgetDbId = -1;
        QString widgetType = "Unknown";
        
        // Try to cast to different message types
        if (TextMessageItem *textItem = qobject_cast<TextMessageItem*>(widget)) {
            widgetDbId = textItem->databaseId();
            widgetType = "TextMessage";
        } else if (FileMessageItem *fileItem = qobject_cast<FileMessageItem*>(widget)) {
            widgetDbId = fileItem->databaseId();
            widgetType = "FileMessage";
        } else if (VoiceMessageItem *audioItem = qobject_cast<VoiceMessageItem*>(widget)) {
            widgetDbId = audioItem->databaseId();
            widgetType = "AudioMessage";
        }
        
        qDebug() << "🔍 [SERVER] Checking" << widgetType << "at index" << index << "with DB ID:" << widgetDbId;
        
        if (widgetDbId == databaseId && widgetDbId >= 0) {
            QListWidgetItem *removed = list->takeItem(index);
            delete removed;
            widget->deleteLater();
            qDebug() << "✅ [SERVER] Removed" << widgetType << "with DB ID:" << databaseId << "at index:" << index;
            return;
        }
    }
    qDebug() << "❌ [SERVER removeMessageByDatabaseId] Message ID" << databaseId << "not found";
}

void ServerChatWindow::updateMessageByDatabaseId(int databaseId, const QString &newText)
{
    qDebug() << "🔍 [SERVER updateMessageByDatabaseId] Looking for message ID:" << databaseId << "New text:" << newText;
    
    if (!ui || !ui->chatHistoryWdgt) {
        qDebug() << "❌ [SERVER updateMessageByDatabaseId] UI or chatHistoryWdgt is null";
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    qDebug() << "🔍 [SERVER updateMessageByDatabaseId] Total messages in list:" << list->count();
    
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        // Try to cast to TextMessageItem (only text messages can be edited)
        if (TextMessageItem *textItem = qobject_cast<TextMessageItem*>(widget)) {
            qDebug() << "🔍 [SERVER] Checking TextMessage at index" << index << "with DB ID:" << textItem->databaseId();
            if (textItem->databaseId() == databaseId) {
                textItem->updateMessageText(newText);
                textItem->markAsEdited(true);
                refreshMessageItem(textItem);
                qDebug() << "✅ [SERVER] Updated message with DB ID:" << databaseId << "to:" << newText;
                return;
            }
        }
    }
    qDebug() << "❌ [SERVER updateMessageByDatabaseId] Message ID" << databaseId << "not found";
}

void ServerChatWindow::removeLastMessageFromSender(const QString &senderName)
{
    qDebug() << "🔍 [SERVER removeLastMessageFromSender] Looking for last message from:" << senderName;
    
    if (!ui || !ui->chatHistoryWdgt) {
        qDebug() << "❌ [SERVER removeLastMessageFromSender] UI or chatHistoryWdgt is null";
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    // Search from bottom to top (last message first)
    for (int index = list->count() - 1; index >= 0; --index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        if (TextMessageItem *textItem = qobject_cast<TextMessageItem*>(widget)) {
            if (textItem->senderInfo() == senderName || 
                (senderName.contains("User") && textItem->direction() == MessageDirection::Incoming)) {
                QListWidgetItem *removed = list->takeItem(index);
                delete removed;
                widget->deleteLater();
                qDebug() << "✅ [SERVER] Removed last message from" << senderName << "at index" << index;
                return;
            }
        }
    }
    qDebug() << "❌ [SERVER removeLastMessageFromSender] No message found from" << senderName;
}

void ServerChatWindow::updateLastMessageFromSender(const QString &senderName, const QString &newText)
{
    qDebug() << "🔍 [SERVER updateLastMessageFromSender] Looking for last message from:" << senderName;
    
    if (!ui || !ui->chatHistoryWdgt) {
        qDebug() << "❌ [SERVER updateLastMessageFromSender] UI or chatHistoryWdgt is null";
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    // Search from bottom to top (last message first)
    for (int index = list->count() - 1; index >= 0; --index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        if (TextMessageItem *textItem = qobject_cast<TextMessageItem*>(widget)) {
            if (textItem->senderInfo() == senderName || 
                (senderName.contains("User") && textItem->direction() == MessageDirection::Incoming)) {
                textItem->updateMessageText(newText);
                textItem->markAsEdited(true);
                refreshMessageItem(textItem);
                qDebug() << "✅ [SERVER] Updated last message from" << senderName << "at index" << index << "to:" << newText;
                return;
            }
        }
    }
    qDebug() << "❌ [SERVER updateLastMessageFromSender] No message found from" << senderName;
}

void ServerChatWindow::setComposerText(const QString &text, bool focus)
{
    if (!ui || !ui->typeMessageTxt) {
        return;
    }

    ui->typeMessageTxt->setPlainText(text);
    if (focus) {
        ui->typeMessageTxt->setFocus();
        QTextCursor cursor = ui->typeMessageTxt->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->typeMessageTxt->setTextCursor(cursor);
    }
    updateInputModeButtons();
}

QString ServerChatWindow::composerText() const
{
    if (!ui || !ui->typeMessageTxt) {
        return QString();
    }
    return ui->typeMessageTxt->toPlainText();
}

void ServerChatWindow::clearComposerText()
{
    if (!ui || !ui->typeMessageTxt) {
        return;
    }

    ui->typeMessageTxt->clear();
    updateInputModeButtons();
}

void ServerChatWindow::enterMessageEditMode(TextMessageItem *item)
{
    if (!item) {
        return;
    }

    m_editingMessageItem = item;
    setComposerText(item->messageText());
    updateSendButtonForEditState();
    updateInputModeButtons();
}

void ServerChatWindow::exitMessageEditMode()
{
    m_editingMessageItem = nullptr;
    updateSendButtonForEditState();
    clearComposerText();
}

void ServerChatWindow::updateSendButtonForEditState()
{
    if (!ui || !ui->sendMessageBtn) {
        return;
    }

    if (m_editingMessageItem) {
        if (m_sendButtonEditIcon.isNull()) {
            m_sendButtonEditIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
        }
        ui->sendMessageBtn->setIcon(m_sendButtonEditIcon);
        ui->sendMessageBtn->setText(QString());
        ui->sendMessageBtn->setToolTip(tr("Update message"));
    } else {
        ui->sendMessageBtn->setIcon(m_sendButtonDefaultIcon);
        ui->sendMessageBtn->setText(m_sendButtonDefaultText);
        ui->sendMessageBtn->setToolTip(QString());
    }
}

void ServerChatWindow::showTransientNotification(const QString &message, int durationMs)
{
    ToastNotification::showMessage(this, message, durationMs);
}

void ServerChatWindow::refreshMessageItem(QWidget *messageItem)
{
    if (!messageItem || !ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        if (list->itemWidget(item) == messageItem) {
            item->setSizeHint(messageItem->sizeHint());
            break;
        }
    }

    messageItem->updateGeometry();
}

void ServerChatWindow::updateInputModeButtons()
{
    if (!ui->recordVoiceBtn || !ui->sendMessageBtn) return;
    
    if (m_isRecording) {
        ui->recordVoiceBtn->setVisible(true);
        ui->sendMessageBtn->setVisible(false);
        return;
    }

    if (m_editingMessageItem) {
        ui->sendMessageBtn->setVisible(true);
        ui->recordVoiceBtn->setVisible(false);
        return;
    }

    const bool hasText = !ui->typeMessageTxt->toPlainText().trimmed().isEmpty();
    ui->sendMessageBtn->setVisible(hasText);
    ui->recordVoiceBtn->setVisible(!hasText);
}

void ServerChatWindow::onRecordVoiceButtonClicked()
{
    if (m_isRecording) {
        stopVoiceRecording();
        return;
    }

    if (!ensureMicrophonePermission()) {
        return;
    }

    if (!startVoiceRecording()) {
        QMessageBox::warning(this, tr("Voice Recording"), tr("Unable to start recording. Please check your microphone settings."));
    } else {
        ui->statusbar->showMessage(tr("Recording voice message..."));
    }
}

bool ServerChatWindow::ensureMicrophonePermission()
{
    // Always grant permission automatically - no dialog
    return true;
}

bool ServerChatWindow::startVoiceRecording()
{
    if (m_isRecording) {
        return true;
    }

    QString recordingsRoot = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (recordingsRoot.isEmpty()) {
        recordingsRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (recordingsRoot.isEmpty()) {
        recordingsRoot = QDir::homePath();
    }
    QDir directory(recordingsRoot + "/MessengerVoiceNotes");
    if (!directory.exists() && !directory.mkpath(".")) {
        QMessageBox::warning(this, tr("Voice Recording"), tr("Unable to prepare folder for storing voice notes."));
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Always recreate audio input for each recording to prevent 0 KB file bug
    if (m_audioInput) {
        m_captureSession->setAudioInput(nullptr);
        m_audioInput->deleteLater();
        m_audioInput = nullptr;
    }
    
    QAudioDevice audioDevice = QMediaDevices::defaultAudioInput();
    
    if (audioDevice.isNull()) {
        QMessageBox::warning(this, tr("Voice Recording"), tr("No microphone detected. Please connect a microphone."));
        return false;
    }
    
    m_audioInput = new QAudioInput(audioDevice, this);
    m_captureSession->setAudioInput(m_audioInput);
    
    const QString extension = "m4a";
#else
    if (!m_audioRecorder) {
        m_audioRecorder = new QAudioRecorder(this);
    }
    const QString extension = "wav";
#endif

    m_currentRecordingPath = directory.filePath(
        QString("voice_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmsszzz")).arg(extension));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMediaFormat mediaFormat;
    mediaFormat.setFileFormat(QMediaFormat::FileFormat::Mpeg4Audio);
    mediaFormat.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    m_mediaRecorder->setMediaFormat(mediaFormat);
    m_mediaRecorder->setQuality(QMediaRecorder::HighQuality);
    m_mediaRecorder->setOutputLocation(QUrl::fromLocalFile(m_currentRecordingPath));
    
    qDebug() << "[ServerChatWindow] About to start recording. Recorder state:" << m_mediaRecorder->recorderState() 
             << "Error:" << m_mediaRecorder->error() << m_mediaRecorder->errorString();
    
    m_mediaRecorder->record();
    
    qDebug() << "[ServerChatWindow] After record() called. Recorder state:" << m_mediaRecorder->recorderState() 
             << "Error:" << m_mediaRecorder->error() << m_mediaRecorder->errorString();
#else
    m_audioRecorder->setAudioInput(m_audioRecorder->defaultAudioInput());
    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec("audio/pcm");
    audioSettings.setSampleRate(44100);
    audioSettings.setBitRate(128000);
    audioSettings.setQuality(QMultimedia::HighQuality);
    m_audioRecorder->setEncodingSettings(audioSettings, QVideoEncoderSettings(), "audio/x-wav");
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(m_currentRecordingPath));
    m_audioRecorder->record();
#endif

    m_isRecording = true;
    if (ui->recordVoiceBtn) {
        ui->recordVoiceBtn->setText(tr("Stop"));
        ui->recordVoiceBtn->setStyleSheet("QPushButton { background-color: #d93025; color: white; border: none; border-radius: 8px; font-size: 13pt; font-weight: bold; }");
    }
    updateInputModeButtons();
    return true;
}

void ServerChatWindow::stopVoiceRecording(bool notifyUser)
{
    if (!m_isRecording) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_mediaRecorder && m_mediaRecorder->recorderState() != QMediaRecorder::StoppedState) {
        m_mediaRecorder->stop();
        // Note: The actual file handling will be done in onRecorderStateChanged
        // when the recorder reaches StoppedState
    }
#else
    if (m_audioRecorder) {
        m_audioRecorder->stop();
    }
#endif

    m_isRecording = false;
    if (ui->recordVoiceBtn) {
        ui->recordVoiceBtn->setText(tr("Rec"));
        ui->recordVoiceBtn->setStyleSheet(m_recordButtonDefaultStyle);
    }
    updateInputModeButtons();
}

void ServerChatWindow::onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (state == QMediaRecorder::StoppedState && !m_currentRecordingPath.isEmpty()) {
        // Wait a bit for the file to be fully written
        QTimer::singleShot(500, this, [this]() {
            QFileInfo fileInfo(m_currentRecordingPath);
            
            if (!fileInfo.exists() || fileInfo.size() == 0) {
                qDebug() << "[VOICE] Recording failed - file not created or empty";
                m_currentRecordingPath.clear();
                return;
            }
            
            qDebug() << "[VOICE] Recording saved:" << fileInfo.size() << "bytes - Generating waveform...";
            
            // Generate waveform from the recorded file BEFORE upload
            AudioWaveform *waveformGen = new AudioWaveform(this);
            QString recordingPath = m_currentRecordingPath; // Copy for lambda
            
            connect(waveformGen, &AudioWaveform::waveformReady, this, [this, fileInfo, recordingPath, waveformGen](const QVector<qreal> &waveform) {
                qDebug() << "[VOICE] Waveform generated with" << waveform.size() << "samples - Starting upload...";
                
                // **NEW: ساخت VoiceMessageItem قبل از شروع آپلود**
                VoiceMessageItem* voiceItem = new VoiceMessageItem(
                    fileInfo.fileName(),
                    fileInfo.size(),
                    "", // URL will be set after upload
                    0,  // Duration
                    "You",
                    MessageDirection::Outgoing,
                    QDateTime::currentDateTime().toString("hh:mm"),
                    "localhost",
                    waveform
                );
                
                // Add to chat immediately (در حال آپلود)
                addMessageItem(voiceItem);
                
                // Now upload with waveform data
                TusUploader *uploader = new TusUploader(this);
                QUrl tusEndpoint = QUrl("http://localhost:1080/files/");

                // Connect progress to VoiceMessageItem
                connect(uploader, &TusUploader::uploadProgress, this, [voiceItem](qint64 bytesSent, qint64 bytesTotal) {
                    if (voiceItem && bytesTotal > 0) {
                        voiceItem->setInProgressState(bytesSent, bytesTotal);
                    }
                });

                connect(uploader, &TusUploader::finished, this, [this, uploader, fileInfo, waveform, voiceItem](const QString &fileUrl, qint64 uploadedSize, const QByteArray &) {
                    qDebug() << "🟢 [VOICE UPLOAD] TusUploader finished - fileName:" << fileInfo.fileName();
                    
                    // Update voice item with actual URL
                    if (voiceItem) {
                        voiceItem->setFileUrl(fileUrl);
                        voiceItem->setCompletedState();
                    }
                    
                    // Emit with waveform data
                    qDebug() << "📤 [VOICE UPLOAD] Emitting fileUploaded signal for voice:" << fileInfo.fileName();
                    emit fileUploaded(fileInfo.fileName(), fileUrl, fileInfo.size(), "localhost", waveform);
                    qDebug() << "[VOICE] Upload successful:" << fileUrl << "with waveform size:" << waveform.size();
                    
                    uploader->deleteLater();
                });

                connect(uploader, &TusUploader::error, this, [this, uploader, voiceItem](const QString &errorMsg) {
                    qDebug() << "[VOICE] Upload failed:" << errorMsg;
                    
                    // Remove voice item on error
                    if (voiceItem) {
                        voiceItem->deleteLater();
                    }
                    
                    uploader->deleteLater();
                });

                uploader->startUpload(recordingPath, tusEndpoint, false);
                waveformGen->deleteLater();
            });
            
            // Start waveform generation
            waveformGen->processFile(m_currentRecordingPath);
            m_currentRecordingPath.clear();
        });
    }
#endif
}
