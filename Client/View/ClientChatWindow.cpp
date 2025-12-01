#include "View/ClientChatWindow.h"
#include "ui_ClientChatWindow.h"
#include <QKeyEvent>
#include <QResizeEvent>
#include <QRegularExpression>

// --- ADD THESE INCLUDES ---
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QFileInfo>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include <QUrl>
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
#include "FileMessage.h"
#include "AudioMessage.h"
#include "TextMessage.h"
#include "InfoCard.h"
#include "ToastNotification.h"
#include "ModernThemeApplier.h"
#include "ModernTheme.h"
#include "ChatStyles.h"
#include "EmptyMessageState.h"
// ---

ClientChatWindow::ClientChatWindow(QWidget *parent)
    : BaseChatWindow(parent)
    , ui(new Ui::ClientChatWindow)
    , m_uploadProgressBar(nullptr) // <-- INITIALIZE
    , m_serverHost("localhost") // --- INITIALIZE server host
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

    // --- Add Chat Header ---
    // Hide existing header from UI file if it exists
    if (ui->chatHeader) {
        ui->chatHeader->hide();
    }

    m_chatHeader = new ChatHeader(this);
    // Insert at top of chat section (verticalLayout is the layout of chatSection)
    ui->verticalLayout->insertWidget(0, m_chatHeader);
    
    // Create empty message state widget as child of chatHistoryWdgt's parent
    m_emptyMessageState = new EmptyMessageState(ui->chatHistoryWdgt->parentWidget());
    m_emptyMessageState->setGeometry(ui->chatHistoryWdgt->geometry());
    m_emptyMessageState->raise(); // Bring to front
    updateEmptyStateVisibility();
    
    setupInputArea(); // <-- Call the new setup method
    m_recordButtonDefaultStyle = ui->recordVoiceBtn->styleSheet();
    m_sendButtonDefaultIcon = ui->sendMessageBtn->icon();
    m_sendButtonDefaultText = ui->sendMessageBtn->text();
    m_sendButtonEditIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);

    // Connect send button click to slot
    connect(ui->sendMessageBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onsendMessageBtnclicked);

    // --- ADD THIS CONNECTION ---
    connect(ui->sendFileBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onSendFileClicked);

    // Connect reconnect button
    connect(ui->reconnectBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onReconnectClicked);

    connect(ui->recordVoiceBtn, &QPushButton::clicked,
            this, &ClientChatWindow::onRecordVoiceButtonClicked);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Initialize capture session, media recorder and audio input
    m_captureSession = new QMediaCaptureSession(this);
    m_mediaRecorder = new QMediaRecorder(this);
    m_captureSession->setRecorder(m_mediaRecorder);
    
    // Connect state change signal
    connect(m_mediaRecorder, &QMediaRecorder::recorderStateChanged,
            this, &ClientChatWindow::onRecorderStateChanged);
    
#endif

    // Install event filter for Enter key
    ui->typeMessageTxt->installEventFilter(this);
    connect(ui->typeMessageTxt, &QTextEdit::textChanged,
            this, &ClientChatWindow::updateInputModeButtons);

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
    
    // Re-apply custom send button style after theme application
    ui->sendMessageBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #007aff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 20px;"
        "    font-weight: bold;"
        "    font-size: 14px;"  // Changed from 14px to 16px as example
        "}"
        "QPushButton:hover { background-color: #0062cc; }"
        "QPushButton:pressed { background-color: #004999; }"
        "QPushButton:disabled { background-color: #b3d7ff; }"
    );
}

void ClientChatWindow::applyModernTheme()
{
    // Apply to header - FORCE white background
    if (ui->chatHeader) {
        ModernThemeApplier::applyToHeader(ui->chatHeader);
    }
    
    // Apply to main components
    ModernThemeApplier::applyToPrimaryButton(ui->sendMessageBtn);
    // ModernThemeApplier::applyToIconButton(ui->sendFileBtn); // Disabled to preserve custom layout
    // if (ui->recordVoiceBtn) {
    //     ModernThemeApplier::applyToIconButton(ui->recordVoiceBtn); // Disabled to preserve custom layout
    // }
    // ModernThemeApplier::applyToTextInput(ui->typeMessageTxt); // Disabled to preserve custom layout
    ModernThemeApplier::applyToChatArea(ui->chatHistoryWdgt);
    
    // Apply to reconnect button as secondary button with new text
    if (ui->reconnectBtn) {
        ui->reconnectBtn->setText(tr("Having trouble?\nContact Technical Support"));
        ModernThemeApplier::applyToSecondaryButton(ui->reconnectBtn);
    }
    
    // Set overall window background
    setStyleSheet("QMainWindow { background-color: " + ModernTheme::CHAT_BACKGROUND + "; }");

    // Apply scrollable user list style so client sidebar matches server styling
    if (QListWidget *userList = this->findChild<QListWidget*>("userListWdgt")) {
        userList->setStyleSheet(ChatStyles::getUserListStyle() + ChatStyles::getScrollBarStyle());
    }

    ui->chatHistoryWdgt->setStyleSheet(ChatStyles::getChatHistoryStyle() + ChatStyles::getScrollBarStyle());
}


ClientChatWindow::~ClientChatWindow()
{
    stopVoiceRecording(false);
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

void ClientChatWindow::updateInputModeButtons()
{
    // Always show both buttons side by side, disable send if no text
    const bool hasText = !ui->typeMessageTxt->toPlainText().trimmed().isEmpty();
    ui->sendMessageBtn->setVisible(true);
    ui->sendMessageBtn->setEnabled(hasText || m_editingMessageItem); // allow send in edit mode
    ui->recordVoiceBtn->setVisible(true);
    // Enable record button even during recording so we can stop it
    ui->recordVoiceBtn->setEnabled(true);
}

void ClientChatWindow::onsendMessageBtnclicked()
{
    QString message = ui->typeMessageTxt->toPlainText().trimmed();
    
    if (message.isEmpty()) {
        QMessageBox::warning(this, "Empty Message", "Please type a message before sending!");
        return;
    }

    if (m_editingMessageItem) {
        TextMessage *itemBeingEdited = m_editingMessageItem;
        emit messageEditConfirmed(itemBeingEdited, message);
        exitMessageEditMode();
        return;
    }

    emit sendMessageRequested(message);
    // Clear the text box after sending
    clearComposerText();
    updateInputModeButtons();
}

void ClientChatWindow::onchatHistoryWdgtitemClicked(QListWidgetItem *item)
{
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
//     // Controller should create FileMessage and call addMessageItem() directly.
//     qWarning() << "DEPRECATED: ClientChatWindow::showFileMessage() called.";
//     qWarning() << "Controllers should use addMessageItem(QWidget*) instead.";
// }

void ClientChatWindow::updateConnectionInfo(const QString &serverUrl, const QString &status)
{
    // QString infoText = QString("Connection Status: %1").arg(status);
    // ui->clientConnectionLabel->setText(infoText);
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

    // **FIX: ایجاد FileMessage قبل از شروع آپلود**
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    qint64 fileSize = fileInfo.size();
    
    // Create FileMessage with temporary URL
    const QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
    FileMessage* fileItem = new FileMessage(
        fileName,
        fileSize,
        "", // URL بعداً set می‌شود
        "You",
        MessageDirection::Outgoing,
        m_serverHost,
        timestamp
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
            // Delete FileMessage
            fileItem->deleteLater();
        });
    }

    connect(uploader, &TusUploader::finished, this, [=](const QString &uploadUrl, qint64 uploadedSize) {
        // **FIX: به جای ساخت FileMessage جدید، فقط state را تغییر بده**
        if (infoCard) {
            infoCard->setState(InfoCard::State::Completed_Sent);
        }
        
        // Emit signal for saving to database and sending via WebSocket
        emit fileUploaded(fileName, uploadUrl, uploadedSize, m_serverHost);
        uploader->deleteLater();
    });

    connect(uploader, &TusUploader::error, this, [=](const QString &error) {
        QMessageBox::critical(this, "Upload Error", error);
        // حذف FileMessage در صورت خطا
        fileItem->deleteLater();
        uploader->deleteLater();
    });

    uploader->startUpload(filePath, tusEndpoint);
}

void ClientChatWindow::onRecordVoiceButtonClicked()
{
    
    if (m_isRecording) {
        stopVoiceRecording();
        return;
    }

    if (!ensureMicrophonePermission()) {
        return;
    }

    if (!startVoiceRecording()) {
        // Don't show dialog - just log error
    } else {
        ui->statusbar->showMessage(tr("Recording voice message..."));
    }
}

bool ClientChatWindow::ensureMicrophonePermission()
{
    // Always grant permission automatically - no dialog
    return true;
}

bool ClientChatWindow::startVoiceRecording()
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
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Always recreate audio input and recorder for each recording to avoid stale state
    if (m_audioInput) {
        m_captureSession->setAudioInput(nullptr);
        m_audioInput->deleteLater();
        m_audioInput = nullptr;
    }
    
    QAudioDevice audioDevice = QMediaDevices::defaultAudioInput();
    
    if (audioDevice.isNull()) {
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
    // Set format and location before recording
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
    ui->recordVoiceBtn->setIcon(QIcon("assets/recording.svg")); // Use recording icon
    ui->recordVoiceBtn->setIconSize(QSize(24, 24));
    ui->recordVoiceBtn->setText(""); // Clear text
    ui->recordVoiceBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: #ff3b30; } QPushButton:hover { color: #d93025; }");
    updateInputModeButtons();
    return true;
}

void ClientChatWindow::stopVoiceRecording(bool notifyUser)
{
    if (!m_isRecording) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_mediaRecorder && m_mediaRecorder->recorderState() != QMediaRecorder::StoppedState) {
        m_mediaRecorder->stop();
    }
#else
    if (m_audioRecorder) {
        m_audioRecorder->stop();
    }
#endif

    m_isRecording = false;
    ui->recordVoiceBtn->setText(""); // Clear text
    ui->recordVoiceBtn->setIcon(QIcon("assets/mic.svg")); // Restore icon
    ui->recordVoiceBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: black; }");
    updateInputModeButtons();
}

void ClientChatWindow::onRecorderStateChanged(QMediaRecorder::RecorderState state)
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
                
                // **NEW: ساخت AudioMessage قبل از شروع آپلود**
                AudioMessage* voiceItem = new AudioMessage(
                    fileInfo.fileName(),
                    fileInfo.size(),
                    "", // URL will be set after upload
                    0,  // Duration
                    "You",
                    MessageDirection::Outgoing,
                    QDateTime::currentDateTime().toString("hh:mm"),
                    m_serverHost,
                    waveform
                );
                
                // Add to chat immediately (در حال آپلود)
                addMessageItem(voiceItem);
                
                // Now upload with waveform data
                TusUploader *uploader = new TusUploader(this);
                QUrl tusEndpoint = QUrl("http://" + m_serverHost + ":1080/files/");

                // Connect progress to AudioMessage
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
                    emit fileUploaded(fileInfo.fileName(), fileUrl, fileInfo.size(), m_serverHost, waveform);
                    
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

    // Connect sizeChanged signal if it's a FileMessage
    if (FileMessage *fileMsg = qobject_cast<FileMessage*>(messageItem)) {
        connect(fileMsg, &FileMessage::sizeChanged, this, [this, messageItem]() {
            refreshMessageItem(messageItem);
        });
    }
    
    // Update empty state visibility
    updateEmptyStateVisibility();
}

void ClientChatWindow::removeMessageItem(QWidget *messageItem)
{
    if (!messageItem || !ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        if (list->itemWidget(item) == messageItem) {
            QListWidgetItem *removed = list->takeItem(index);
            delete removed;
            messageItem->deleteLater();
            break;
        }
    }
    
    // Update empty state visibility
    updateEmptyStateVisibility();
}

void ClientChatWindow::removeMessageByDatabaseId(int databaseId)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        int widgetDbId = -1;
        QString widgetType = "Unknown";
        
        // Try to cast to different message types
        if (TextMessage *textItem = qobject_cast<TextMessage*>(widget)) {
            widgetDbId = textItem->databaseId();
            widgetType = "TextMessage";
        } else if (FileMessage *fileItem = qobject_cast<FileMessage*>(widget)) {
            widgetDbId = fileItem->databaseId();
            widgetType = "FileMessage";
        } else if (AudioMessage *audioItem = qobject_cast<AudioMessage*>(widget)) {
            widgetDbId = audioItem->databaseId();
            widgetType = "AudioMessage";
        }
        
        
        if (widgetDbId == databaseId && widgetDbId >= 0) {
            QListWidgetItem *removed = list->takeItem(index);
            delete removed;
            widget->deleteLater();
            return;
        }
    }
}

void ClientChatWindow::updateMessageByDatabaseId(int databaseId, const QString &newText)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        // Try to cast to TextMessage (only text messages can be edited)
        if (TextMessage *textItem = qobject_cast<TextMessage*>(widget)) {
            if (textItem->databaseId() == databaseId) {
                textItem->updateMessageText(newText);
                textItem->markAsEdited(true);
                refreshMessageItem(textItem);
                return;
            }
        } else {
        }
    }
}

void ClientChatWindow::removeLastMessageFromSender(const QString &senderName)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    // Search from bottom to top (last message first)
    for (int index = list->count() - 1; index >= 0; --index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        if (TextMessage *textItem = qobject_cast<TextMessage*>(widget)) {
            if (textItem->senderInfo() == senderName || 
                (senderName == "Server" && textItem->direction() == MessageDirection::Incoming)) {
                QListWidgetItem *removed = list->takeItem(index);
                delete removed;
                widget->deleteLater();
                return;
            }
        }
    }
}

void ClientChatWindow::updateLastMessageFromSender(const QString &senderName, const QString &newText)
{
    
    if (!ui || !ui->chatHistoryWdgt) {
        return;
    }

    QListWidget *list = ui->chatHistoryWdgt;
    
    // Search from bottom to top (last message first)
    for (int index = list->count() - 1; index >= 0; --index) {
        QListWidgetItem *item = list->item(index);
        QWidget *widget = list->itemWidget(item);
        
        if (TextMessage *textItem = qobject_cast<TextMessage*>(widget)) {
            if (textItem->senderInfo() == senderName || 
                (senderName == "Server" && textItem->direction() == MessageDirection::Incoming)) {
                textItem->updateMessageText(newText);
                textItem->markAsEdited(true);
                refreshMessageItem(textItem);
                return;
            }
        }
    }
}

void ClientChatWindow::setComposerText(const QString &text, bool focus)
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

QString ClientChatWindow::composerText() const
{
    if (!ui || !ui->typeMessageTxt) {
        return QString();
    }
    return ui->typeMessageTxt->toPlainText();
}

void ClientChatWindow::clearComposerText()
{
    if (!ui || !ui->typeMessageTxt) {
        return;
    }

    ui->typeMessageTxt->clear();
    updateInputModeButtons();
}

void ClientChatWindow::enterMessageEditMode(TextMessage *item)
{
    if (!item) {
        return;
    }

    m_editingMessageItem = item;
    setComposerText(item->messageText());
    updateSendButtonForEditState();
    updateInputModeButtons();
}

void ClientChatWindow::exitMessageEditMode()
{
    m_editingMessageItem = nullptr;
    updateSendButtonForEditState();
    clearComposerText();
}

void ClientChatWindow::updateSendButtonForEditState()
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

void ClientChatWindow::showTransientNotification(const QString &message, int durationMs)
{
    ToastNotification::showMessage(this, message, durationMs);
}

void ClientChatWindow::refreshMessageItem(QWidget *messageItem)
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

void ClientChatWindow::setupInputArea()
{
    // 1. Get the layout of inputSection
    QWidget *inputSection = ui->inputSection;
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
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 3. Create Smiley Button
    QPushButton *smileyBtn = new QPushButton(inputSection);
    smileyBtn->setText("☺"); 
    smileyBtn->setFixedSize(40, 40);
    smileyBtn->setFlat(true);
    smileyBtn->setCursor(Qt::PointingHandCursor);
    smileyBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #8e8e93; background: transparent; } QPushButton:hover { color: #007aff; }");
    
    // 4. Create Input Container (White rounded box)
    QFrame *inputContainer = new QFrame(inputSection);
    inputContainer->setObjectName("inputContainer");
    inputContainer->setStyleSheet(
        "QFrame#inputContainer {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e5e5ea;"
        "    border-radius: 20px;"
        "}"
    );
    
    QHBoxLayout *containerLayout = new QHBoxLayout(inputContainer);
    containerLayout->setContentsMargins(15, 5, 10, 5);
    containerLayout->setSpacing(5);

    // 5. Configure Text Edit
    ui->typeMessageTxt->setParent(inputContainer);
    ui->typeMessageTxt->setStyleSheet(
        "QTextEdit {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #000000;"
        "    font-size: 14px;"
        "    padding-top: 12px;" // Add padding to center text vertically
        "}"
    );
    ui->typeMessageTxt->setFixedHeight(50); // Increased height
    ui->typeMessageTxt->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->typeMessageTxt->setPlaceholderText("Write message...");

    // 6. Configure Attachment Button
    ui->sendFileBtn->setParent(inputContainer);
    // Use SVG icon
    QIcon attachIcon("assets/attach.svg");
    ui->sendFileBtn->setIcon(attachIcon);
    ui->sendFileBtn->setIconSize(QSize(24, 24));
    ui->sendFileBtn->setText(""); 
    ui->sendFileBtn->setFixedSize(32, 32);
    ui->sendFileBtn->setFlat(true);
    ui->sendFileBtn->setCursor(Qt::PointingHandCursor);
    ui->sendFileBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: black; }");

    // 7. Configure Mic Button
    ui->recordVoiceBtn->setParent(inputContainer);
    // Use SVG icon
    QIcon micIcon("assets/mic.svg");
    ui->recordVoiceBtn->setIcon(micIcon);
    ui->recordVoiceBtn->setIconSize(QSize(24, 24));
    ui->recordVoiceBtn->setText("");
    ui->recordVoiceBtn->setFixedSize(32, 32);
    ui->recordVoiceBtn->setFlat(true);
    ui->recordVoiceBtn->setCursor(Qt::PointingHandCursor);
    ui->recordVoiceBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: black; }");

    // 8. Configure Send Button
    ui->sendMessageBtn->setParent(inputSection);
    ui->sendMessageBtn->setText("Send");
    ui->sendMessageBtn->setFixedSize(80, 40);
    ui->sendMessageBtn->setCursor(Qt::PointingHandCursor);
    ui->sendMessageBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #007aff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 20px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #0062cc; }"
        "QPushButton:pressed { background-color: #004999; }"
        "QPushButton:disabled { background-color: #b3d7ff; }"
    );

    // 9. Assemble
    mainLayout->addWidget(smileyBtn);
    
    containerLayout->addWidget(ui->typeMessageTxt);
    containerLayout->addWidget(ui->sendFileBtn);
    containerLayout->addWidget(ui->recordVoiceBtn);
    
    mainLayout->addWidget(inputContainer);
    mainLayout->addWidget(ui->sendMessageBtn);
}
void ClientChatWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        // Just emit the signal. The Controller decides what to do.
        emit windowStateChanged(isActiveWindow());
    }
    QMainWindow::changeEvent(event);
}

void ClientChatWindow::updateEmptyStateVisibility()
{
    if (!ui || !ui->chatHistoryWdgt || !m_emptyMessageState) {
        return;
    }
    
    // Show empty state only when there are no messages
    bool isEmpty = ui->chatHistoryWdgt->count() == 0;
    m_emptyMessageState->setVisible(isEmpty);
}

void ClientChatWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Update empty state geometry to match chat history widget
    if (m_emptyMessageState && ui && ui->chatHistoryWdgt) {
        m_emptyMessageState->setGeometry(ui->chatHistoryWdgt->geometry());
    }
}
