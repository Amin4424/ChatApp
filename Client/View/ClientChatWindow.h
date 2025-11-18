#ifndef CLIENTCHATWINDOW_H
#define CLIENTCHATWINDOW_H

#include "BaseChatWindow.h"
#include <QProgressBar>
#include <QListWidgetItem>
#include <QIcon>
#include "TextMessage.h" // <-- *** ADD THIS INCLUDE ***

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QMediaRecorder>
#include <QMediaCaptureSession>
class QAudioInput;
#else
class QAudioRecorder;
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class ClientChatWindow; }
QT_END_NAMESPACE

class ClientChatWindow : public BaseChatWindow
{
    Q_OBJECT

public:
    ClientChatWindow(QWidget *parent = nullptr);
    ~ClientChatWindow();
    
    // --- ADD: Set server host ---
    void setServerHost(const QString &host) { m_serverHost = host; }

    // Add message item directly
    void addMessageItem(QWidget *messageItem);
    void removeMessageItem(QWidget *messageItem);
    void removeMessageByDatabaseId(int databaseId);
    void updateMessageByDatabaseId(int databaseId, const QString &newText);
    void removeLastMessageFromSender(const QString &senderName);
    void updateLastMessageFromSender(const QString &senderName, const QString &newText);

    // Composer helpers for controller
    void setComposerText(const QString &text, bool focus = true);
    QString composerText() const;
    void clearComposerText();
    void enterMessageEditMode(TextMessage *item);
    void exitMessageEditMode();
    bool isEditingMessage() const { return m_editingMessageItem != nullptr; }
    TextMessage* editingMessageItem() const { return m_editingMessageItem; }
    void showTransientNotification(const QString &message, int durationMs = 3000);
    void refreshMessageItem(QWidget *messageItem);

    void updateConnectionInfo(const QString &serverUrl, const QString &status);

signals:
    void messageSent(const QString &message);
    void fileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "", const QVector<qreal> &waveform = QVector<qreal>());
    void reconnectRequested();
    void messageEditConfirmed(TextMessage *item, const QString &newText);

protected:
    void handleSendMessage() override;
    void handleFileUpload() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    void onSendFileClicked();
    void onReconnectClicked();
    void onRecordVoiceButtonClicked();
    void onRecorderStateChanged(QMediaRecorder::RecorderState state);

private:
    Ui::ClientChatWindow *ui;
    QProgressBar *m_uploadProgressBar;
    QString m_serverHost; // --- ADD: Store server host
    void updateInputModeButtons();
    
    // Voice recording
    bool ensureMicrophonePermission();
    bool startVoiceRecording();
    void stopVoiceRecording(bool notifyUser = true);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMediaRecorder *m_mediaRecorder;
    QAudioInput *m_audioInput;
    QMediaCaptureSession *m_captureSession;
#else
    QAudioRecorder *m_audioRecorder;
#endif
    bool m_isRecording = false;
    QString m_currentRecordingPath;
    QString m_recordButtonDefaultStyle;
#ifdef Q_OS_WIN
    bool m_microphonePermissionAsked = false;
    bool m_microphonePermissionGranted = false;
#endif
    TextMessage *m_editingMessageItem = nullptr;
    QIcon m_sendButtonDefaultIcon;
    QString m_sendButtonDefaultText;
    QIcon m_sendButtonEditIcon;

    void updateSendButtonForEditState();
};

#endif // CLIENTCHATWINDOW_H
