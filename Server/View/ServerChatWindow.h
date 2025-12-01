#ifndef SERVERCHATWINDOW_H
#define SERVERCHATWINDOW_H

#include "BaseChatWindow.h"
#include <QListWidgetItem>
#include <QProgressBar>
#include <QIcon>
#include "MessageAliases.h"
#include "ChatHeader.h" // Added ChatHeader
#include "Components/UserListManager.h" // Added UserListManager
#include "ChatInputWidget.h" // Added ChatInputWidget
#include "EmptyMessageState.h" // Empty state widget

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QMediaRecorder>
#include <QMediaCaptureSession>
class QAudioInput;
#else
class QAudioRecorder;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class ServerChatWindow;
}
QT_END_NAMESPACE

class ServerChatWindow : public BaseChatWindow
{
    Q_OBJECT

public:
    ServerChatWindow(QWidget *parent = nullptr);
    ~ServerChatWindow();

signals:
    void messageSent(const QString &message);
    void fileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "", const QVector<qreal> &waveform = QVector<qreal>());
    void userSelected(const QString &userId);
    void restartServerRequested();
    void messageEditConfirmed(TextMessageItem *item, const QString &newText);

protected:
    void handleSendMessage() override;
    void handleFileUpload() override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    // void onUserListItemClicked(QListWidgetItem *item); // Handled by UserListManager
    void onRestartServerClicked();
    void onSendFileClicked();
    void onRecordVoiceButtonClicked();
    void onRecorderStateChanged(QMediaRecorder::RecorderState state);

public:
    // Add message item directly
    void addMessageItem(QWidget *messageItem);
    void removeMessageItem(QWidget *messageItem);
    void removeMessageByDatabaseId(int databaseId);
    void updateMessageByDatabaseId(int databaseId, const QString &newText);
    void removeLastMessageFromSender(const QString &senderName);
    void updateLastMessageFromSender(const QString &senderName, const QString &newText);
    void setComposerText(const QString &text, bool focus = true);
    QString composerText() const;
    void clearComposerText();
    void enterMessageEditMode(TextMessageItem *item);
    void exitMessageEditMode();
    bool isEditingMessage() const { return m_editingMessageItem != nullptr; }
    TextMessageItem* editingMessageItem() const { return m_editingMessageItem; }
    void showTransientNotification(const QString &message, int durationMs = 3000);
    void refreshMessageItem(QWidget *messageItem);

    void updateUserList(const QStringList &users);
    void updateUserCount(int count);
    void setPrivateChatMode(const QString &userId);
    void setBroadcastMode();
    void onBroadcastModeClicked();
    void updateServerInfo(const QString &ip, int port, const QString &status);
    void clearChatHistory();
    // void updateUserListSelection(); // Handled by UserListManager
    void updateUserCardInfo(const QString &username, const QString &lastMessage, const QString &time, int unreadCount);

private:
    Ui::ServerChatWindow *ui;
    UserListManager *m_userListManager; // Added UserListManager
    ChatInputWidget *m_chatInput; // Added ChatInputWidget
    bool m_isPrivateChat;
    QString m_currentTargetUser;
    QProgressBar *m_uploadProgressBar;
    
    // Voice recording
    void updateInputModeButtons();
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
    TextMessageItem *m_editingMessageItem = nullptr;
    QIcon m_sendButtonDefaultIcon;
    QString m_sendButtonDefaultText;
    QIcon m_sendButtonEditIcon;

    ChatHeader *m_chatHeader; // Added ChatHeader
    EmptyMessageState *m_emptyMessageState; // Empty state widget

    void updateSendButtonForEditState();
    void updateEmptyStateVisibility();
    void applyModernTheme();
    void setupInputArea(); // Add this line
};

#endif // SERVERCHATWINDOW_H
