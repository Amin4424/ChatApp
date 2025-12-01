#include "View/ServerChatWindow.h"
#include "ui_ServerChatWindow.h"
#include "ModernThemeApplier.h"
#include "ModernTheme.h"
#include "ChatStyles.h"
#include "Components/UserCard.h"
#include <QPushButton>
#include <QPainter>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QRegularExpression>

#include "../../CommonCore/NotificationManager.h"
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
#include "EmptyMessageState.h"
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

    // Initialize UserListManager
    m_userListManager = new UserListManager(ui->userListWdgt, ui->userCountLabel, ui->sidebarStackedWidget, this);
    connect(m_userListManager, &UserListManager::userSelected, this, [this](const QString &userId) {
        emit userSelected(userId);
        setPrivateChatMode(userId);
        
        // Show input section when a user is selected
        if (ui->inputSection) {
            ui->inputSection->setVisible(true);
        }
    });

    // Setup NotificationManager with this window
    NotificationManager::instance().setup(this);

    // --- Add Chat Header ---
    // Hide existing header if it exists
    if (ui->chatHeader) {
        ui->chatHeader->hide();
    }
    
    m_chatHeader = new ChatHeader(this);
    // Insert at top of chat section (verticalLayout is the layout of chatSection)
    ui->verticalLayout->insertWidget(0, m_chatHeader);
    m_chatHeader->hide(); // Hide initially until a chat is selected

    // Create empty message state widget as child of chatHistoryWdgt's parent
    m_emptyMessageState = new EmptyMessageState(ui->chatHistoryWdgt->parentWidget());
    m_emptyMessageState->setGeometry(ui->chatHistoryWdgt->geometry());
    m_emptyMessageState->raise(); // Bring to front
    m_emptyMessageState->hide(); // Hide initially
    updateEmptyStateVisibility();

    setupInputArea(); // Initialize ChatInputWidget

    m_sendButtonDefaultIcon = m_chatInput->sendButton()->icon();
    m_sendButtonDefaultText = m_chatInput->sendButton()->text();
    m_sendButtonEditIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    
    // Record button style
    if (m_chatInput->micButton()) {
        m_recordButtonDefaultStyle = m_chatInput->micButton()->styleSheet();
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

    // Connect restart button
    connect(ui->restartServerBtn, &QPushButton::clicked,
            this, &ServerChatWindow::onRestartServerClicked);

    // --- SETUP PROGRESS BAR ---
    m_uploadProgressBar = new QProgressBar(this);
    m_uploadProgressBar->setVisible(false);
    m_uploadProgressBar->setRange(0, 1000); // We'll set max later
    m_uploadProgressBar->setValue(0);
    m_uploadProgressBar->setTextVisible(true);
    ui->statusbar->addPermanentWidget(m_uploadProgressBar, 1);
    
    updateInputModeButtons();
    updateSendButtonForEditState();
    
    // Apply Modern Theme
    applyModernTheme();
    
    // --- FIX SIDEBAR LAYOUT ---
    if (ui->serverInfoSection) {
        ui->serverInfoSection->setFixedWidth(292);
    }
    
    // Configure User List
    ui->userListWdgt->setViewMode(QListWidget::ListMode);
    ui->userListWdgt->setResizeMode(QListWidget::Adjust);
    ui->userListWdgt->setSpacing(8);
    ui->userListWdgt->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // ui->userListWdgt->setFixedHeight(360); // Scrollable after ~3 items (3 * 112 + spacing)

    // --- Apply Stylesheets (including Scrollbar) ---
    ui->chatHistoryWdgt->setStyleSheet(ChatStyles::getChatHistoryStyle() + ChatStyles::getScrollBarStyle());

    // Hide input section initially
    if (ui->inputSection) {
        ui->inputSection->setVisible(false);
    }
    
    // Set initial title

    // --- MOCK USERS FOR TESTING ---
    // QStringList mockUsers;
    // mockUsers << "Alice" << "Bob";
    // updateUserList(mockUsers);
}

void ServerChatWindow::applyModernTheme()
{
    ModernThemeApplier::applyToMainWindow(this);

    // Apply to headers - FORCE white backgrounds
    if (ui->chatHeader) {
        ModernThemeApplier::applyToHeader(ui->chatHeader);
    }
    
    // Note: ChatInputWidget has its own styling, don't override it

    // Apply scrollable user list style (copied from constructor) so the sidebar
    // uses the same custom scrollbar and item styling as the Server user list.
    ui->userListWdgt->setStyleSheet(ChatStyles::getUserListStyle() + ChatStyles::getScrollBarStyle());
    
    ModernThemeApplier::applyToChatArea(ui->chatHistoryWdgt);
    
    // Apply to sidebar
    // if (ui->serverInfoSection) {
    //     ModernThemeApplier::applyToSidebar(ui->serverInfoSection);
    // }
    
    // Apply to restart button as support banner with new text
    if (ui->restartServerBtn) {
        ui->restartServerBtn->setText(tr("Having trouble?\nContact Technical Support"));
        ui->restartServerBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
    if (m_chatInput && obj == m_chatInput->textEdit() && event->type() == QEvent::KeyPress) {
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

void ServerChatWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowStateChange) {
        if (this->isActiveWindow()) {
            NotificationManager::instance().clearNotifications();
        }
    }
}

void ServerChatWindow::onsendMessageBtnclicked()
{
    if (!m_chatInput) return;
    QString message = m_chatInput->getMessageText().trimmed();
    if (message.isEmpty()) {
        return;
    }
    if (m_editingMessageItem) {
        TextMessageItem *itemBeingEdited = m_editingMessageItem;
        emit messageEditConfirmed(itemBeingEdited, message);
        exitMessageEditMode();
        return;
    }
    emit sendMessageRequested(message);
    clearComposerText();
}

void ServerChatWindow::onchatHistoryWdgtitemClicked(QListWidgetItem *item)
{
}

void ServerChatWindow::onBroadcastModeClicked()
{
    setBroadcastMode();
}

void ServerChatWindow::updateUserList(const QStringList &users)
{
    if (m_userListManager) {
        m_userListManager->updateUsers(users);
    }
}

void ServerChatWindow::updateUserCount(int count)
{
    if (m_userListManager) {
        m_userListManager->updateUserCount(count);
    }
}

void ServerChatWindow::setPrivateChatMode(const QString &userId)
{
    m_isPrivateChat = true;
    m_currentTargetUser = userId;

    // Update ChatHeader
    if (m_chatHeader) {
        m_chatHeader->show();
        
        // Parse userId to separate Name and IP if possible
        // Format expected: "User #1 - ::ffff:127.0.0.1"
        QString name = userId;
        QString ip = "";
        
        if (userId.contains(" - ")) {
            QStringList parts = userId.split(" - ");
            if (parts.size() >= 2) {
                name = parts[0];
                ip = parts[1];
            }
        }
        
        m_chatHeader->setUserName(name);
        m_chatHeader->setUserIP(ip.isEmpty() ? "IP Unknown" : "IP " + ip);
        // Set a default avatar or try to find one
        // m_chatHeader->setAvatar(...); 
    }

    // Hide old header elements
    if (ui->chatHeader) {
        ui->chatHeader->hide();
    }
    if (ui->chatTitleLabel) ui->chatTitleLabel->hide();
    if (ui->chatSubtitleLabel) ui->chatSubtitleLabel->hide();

    // Show input section
    if (ui->inputSection) {
        ui->inputSection->setVisible(true);
    }
    
    // Update user list selection
    if (m_userListManager) {
        m_userListManager->updateSelection(m_currentTargetUser, m_isPrivateChat);
    }
    
    // Update empty state visibility
    updateEmptyStateVisibility();

}

void ServerChatWindow::setBroadcastMode()
{
    m_isPrivateChat = false;
    m_currentTargetUser = "";

    // Update UI to show broadcast mode
    ui->chatTitleLabel->setText("");

    ui->chatSubtitleLabel->setText("");
    
    if (m_chatHeader) {
        m_chatHeader->hide();
    }
    
    // Hide empty state in broadcast mode
    if (m_emptyMessageState) {
        m_emptyMessageState->hide();
    }

    // Hide input section
    if (ui->inputSection) {
        ui->inputSection->setVisible(false);
    }

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

}

void ServerChatWindow::updateServerInfo(const QString &ip, int port, const QString &status)
{
    // QString infoText = QString("Your IP Address: %1:%2\nConnection Status: %3")
    //                        .arg(ip)
    //                        .arg(port)
    //                        .arg(status);
    // ui->serverIpLabel->setText(infoText);
}

void ServerChatWindow::onRestartServerClicked()
{
    emit restartServerRequested();
}

void ServerChatWindow::clearChatHistory()
{
    ui->chatHistoryWdgt->clear();
    updateEmptyStateVisibility();
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

    // --- Connect signals from the uploader ---

    connect(uploader, &TusUploader::finished, this, [=](const QString &uploadUrl, qint64 uploadedSize) {
        // Update file item with actual URL
        if (fileItem) {
            fileItem->setFileUrl(uploadUrl);
        }
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

    // If not in private chat (no user selected), do not show messages
    if (!m_isPrivateChat) {
        messageItem->deleteLater();
        return;
    }

    QListWidgetItem *item = new QListWidgetItem(ui->chatHistoryWdgt);
    item->setSizeHint(messageItem->sizeHint());
    ui->chatHistoryWdgt->addItem(item);
    ui->chatHistoryWdgt->setItemWidget(item, messageItem);
    ui->chatHistoryWdgt->scrollToBottom();

    // Connect sizeChanged signal if it's a FileMessage
    if (FileMessageItem *fileMsg = qobject_cast<FileMessageItem*>(messageItem)) {
        connect(fileMsg, &FileMessageItem::sizeChanged, this, [this, messageItem]() {
            refreshMessageItem(messageItem);
        });
    }
    
    // Update empty state visibility
    updateEmptyStateVisibility();
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
    
    // Update empty state visibility
    updateEmptyStateVisibility();
}

void ServerChatWindow::removeMessageByDatabaseId(int databaseId)
{
    if (!ui || !ui->chatHistoryWdgt) {
        return;
    }
    QListWidget *list = ui->chatHistoryWdgt;
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        int widgetDbId = -1;
        // Try to cast to different message types
        if (TextMessageItem *textItem = qobject_cast<TextMessageItem*>(widget)) {
            widgetDbId = textItem->databaseId();
        } else if (FileMessageItem *fileItem = qobject_cast<FileMessageItem*>(widget)) {
            widgetDbId = fileItem->databaseId();
        } else if (VoiceMessageItem *audioItem = qobject_cast<VoiceMessageItem*>(widget)) {
            widgetDbId = audioItem->databaseId();
        }
        if (widgetDbId == databaseId && widgetDbId >= 0) {
            QListWidgetItem *removed = list->takeItem(index);
            delete removed;
            widget->deleteLater();
            return;
        }
    }
}

void ServerChatWindow::updateMessageByDatabaseId(int databaseId, const QString &newText)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        // Try to cast to TextMessageItem (only text messages can be edited)
        if (TextMessageItem *textItem = qobject_cast<TextMessageItem*>(widget)) {
            if (textItem->databaseId() == databaseId) {
                textItem->updateMessageText(newText);
                textItem->markAsEdited(true);
                refreshMessageItem(textItem);
                return;
            }
        }
    }
}

void ServerChatWindow::removeLastMessageFromSender(const QString &senderName)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
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
                return;
            }
        }
    }
}

void ServerChatWindow::updateLastMessageFromSender(const QString &senderName, const QString &newText)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
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
                return;
            }
        }
    }
}

void ServerChatWindow::setComposerText(const QString &text, bool focus)
{
    if (!m_chatInput) {
        return;
    }

    QTextEdit* edit = m_chatInput->textEdit();
    if (!edit) return;

    edit->setPlainText(text);
    if (focus) {
        edit->setFocus();
        QTextCursor cursor = edit->textCursor();
        cursor.movePosition(QTextCursor::End);
        edit->setTextCursor(cursor);
    }
    updateInputModeButtons();
}

QString ServerChatWindow::composerText() const
{
    if (!m_chatInput) {
        return QString();
    }
    return m_chatInput->getMessageText();
}

void ServerChatWindow::clearComposerText()
{
    if (!m_chatInput) {
        return;
    }

    m_chatInput->clearMessageText();
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
    if (!m_chatInput) {
        return;
    }
    
    QPushButton* btn = m_chatInput->sendButton();
    if (!btn) return;

    if (m_editingMessageItem) {
        if (m_sendButtonEditIcon.isNull()) {
            m_sendButtonEditIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
        }
        btn->setIcon(m_sendButtonEditIcon);
        btn->setText(QString());
        btn->setToolTip(tr("Update message"));
    } else {
        btn->setIcon(m_sendButtonDefaultIcon);
        btn->setText(m_sendButtonDefaultText);
        btn->setToolTip(QString());
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
    if (!m_chatInput) return;

    // Always show both buttons side by side, disable send if no text
    const bool hasText = !m_chatInput->getMessageText().trimmed().isEmpty();
    
    QPushButton* sendBtn = m_chatInput->sendButton();
    QPushButton* micBtn = m_chatInput->micButton();
    
    if (sendBtn) {
        sendBtn->setVisible(true);
        sendBtn->setEnabled(hasText || m_editingMessageItem); // allow send in edit mode
    }
    
    if (micBtn) {
        micBtn->setVisible(true);
        // Enable record button even during recording so we can stop it
        micBtn->setEnabled(true);
    }
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
    
    
    m_mediaRecorder->record();
    
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
    if (m_chatInput && m_chatInput->micButton()) {
        QPushButton* btn = m_chatInput->micButton();
        btn->setIcon(QIcon("assets/recording.svg")); // Use recording icon
        btn->setIconSize(QSize(24, 24));
        btn->setText(""); // Clear text
        btn->setStyleSheet("QPushButton { border: none; background: transparent; color: #ff3b30; } QPushButton:hover { color: #d93025; }");
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
    if (m_chatInput && m_chatInput->micButton()) {
        QPushButton* btn = m_chatInput->micButton();
        QIcon micIcon("assets/mic.svg");
        btn->setIcon(micIcon);
        btn->setIconSize(QSize(24, 24));
        btn->setText("");
        btn->setStyleSheet("QPushButton { border: none; background: transparent; color: black; }");
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
                m_currentRecordingPath.clear();
                return;
            }
            
            
            // Generate waveform from the recorded file BEFORE upload
            AudioWaveform *waveformGen = new AudioWaveform(this);
            QString recordingPath = m_currentRecordingPath; // Copy for lambda
            
            connect(waveformGen, &AudioWaveform::waveformReady, this, [this, fileInfo, recordingPath, waveformGen](const QVector<qreal> &waveform) {
                
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
                    
                    // Update voice item with actual URL
                    if (voiceItem) {
                        voiceItem->setFileUrl(fileUrl);
                        voiceItem->setCompletedState();
                    }
                    
                    // Emit with waveform data
                    emit fileUploaded(fileInfo.fileName(), fileUrl, fileInfo.size(), "localhost", waveform);
                    
                    uploader->deleteLater();
                });

                connect(uploader, &TusUploader::error, this, [this, uploader, voiceItem](const QString &errorMsg) {
                    
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

void ServerChatWindow::setupInputArea()
{
    // 1. Get the layout of inputSection
    QWidget *inputSection = ui->inputSection;
    
    // Hide original UI elements that might be in the input section
    // These are defined in the .ui file but we are replacing them with ChatInputWidget
    if (ui->sendMessageBtn) ui->sendMessageBtn->hide();
    if (ui->typeMessageTxt) ui->typeMessageTxt->hide();
    if (ui->sendFileBtn) ui->sendFileBtn->hide();
    if (ui->recordVoiceBtn) ui->recordVoiceBtn->hide();

    // Clear existing layout
    if (inputSection->layout()) {
        QLayoutItem *item;
        while ((item = inputSection->layout()->takeAt(0)) != nullptr) {
            delete item;
        }
        delete inputSection->layout();
    }

    // 2. Create new main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(inputSection);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    m_chatInput = new ChatInputWidget(inputSection);
    mainLayout->addWidget(m_chatInput);

    // Connect signals
    connect(m_chatInput, &ChatInputWidget::sendMessageClicked, this, &ServerChatWindow::onsendMessageBtnclicked);
    connect(m_chatInput, &ChatInputWidget::sendFileClicked, this, &ServerChatWindow::onSendFileClicked);
    connect(m_chatInput, &ChatInputWidget::recordVoiceClicked, this, &ServerChatWindow::onRecordVoiceButtonClicked);
    connect(m_chatInput, &ChatInputWidget::textChanged, this, &ServerChatWindow::updateInputModeButtons);
    
    // Install event filter on text edit
    m_chatInput->textEdit()->installEventFilter(this);
}

// --- ADD THIS NEW FUNCTION ---
void ServerChatWindow::updateUserCardInfo(const QString &username, const QString &lastMessage, const QString &time, int unreadCount)
{
    for (int i = 0; i < ui->userListWdgt->count(); ++i) {
        QListWidgetItem *item = ui->userListWdgt->item(i);
        QString itemUser = item->data(Qt::UserRole).toString();
        QString cleanItemUser = itemUser.split(" - ").first().trimmed();
        
        // Match against full string OR clean user ID
        if (itemUser == username || cleanItemUser == username) {
            QWidget *widget = ui->userListWdgt->itemWidget(item);
            if (UserCard *card = qobject_cast<UserCard*>(widget)) {
                if (!lastMessage.isEmpty()) {
                    card->setMessage(lastMessage);
                }
                if (!time.isEmpty()) {
                    card->setTime(time);
                }
                
                // Only update unread count if we are NOT currently chatting with this user
                // If we are chatting with them, the count should remain cleared (0)
                // Check against both full and clean ID for current target
                bool isCurrentTarget = (m_currentTargetUser == itemUser || m_currentTargetUser == cleanItemUser);
                
                if (m_isPrivateChat && isCurrentTarget) {
                    card->setUnreadCount(0);
                } else {
                    card->setUnreadCount(unreadCount);
                }
            }
            break;
        }
    }
}

void ServerChatWindow::updateEmptyStateVisibility()
{
    if (!ui || !ui->chatHistoryWdgt || !m_emptyMessageState) {
        return;
    }
    
    // Show empty state only when in private chat mode AND there are no messages
    bool isEmpty = m_isPrivateChat && ui->chatHistoryWdgt->count() == 0;
    m_emptyMessageState->setVisible(isEmpty);
}

void ServerChatWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Update empty state geometry to match chat history widget
    if (m_emptyMessageState && ui && ui->chatHistoryWdgt) {
        m_emptyMessageState->setGeometry(ui->chatHistoryWdgt->geometry());
    }
}

