#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

class ServerChatWindow;
class WebSocketServer;
class DatabaseManager;
class MessageData;
class MessageComponent;
class TextMessage;
class FileMessage;
class AudioMessage;
using TextMessageItem = TextMessage;
using FileMessageItem = FileMessage;
using VoiceMessageItem = AudioMessage;

class ServerController : public QObject
{
    Q_OBJECT

public:
    explicit ServerController(ServerChatWindow *view, WebSocketServer *server, DatabaseManager *db, QObject *parent = nullptr);
    ~ServerController();

    void displayNewConnection();

public slots:
    void displayNewMessages(const QString &message);
    void displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender = "");

private slots:
    void onUserListChanged(const QStringList &users);
    void onUserCountChanged(int count);
    void onUserSelected(const QString &userId);
    void onServerMessageReceived(const QString &message, const QString &senderId);
    void onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "", const QVector<qreal> &waveform = QVector<qreal>());
    void onTextMessageCopyRequested(const QString &text);
    void onTextMessageEditRequested(TextMessageItem *item);
    void onTextMessageDeleteRequested(TextMessageItem *item);
    void onFileMessageDeleteRequested(FileMessageItem *item);
    void onVoiceMessageDeleteRequested(VoiceMessageItem *item);
    void onTextMessageEditConfirmed(TextMessageItem *item, const QString &newText);

private:
    void loadHistoryForServer();
    void loadHistoryForUser(const QString &userId);
    QWidget* createWidgetFromData(const MessageData &msgData);
    void setupTextMessageItem(TextMessageItem *item);
    void setupFileMessageItem(FileMessageItem *item);
    void setupVoiceMessageItem(VoiceMessageItem *item);
    QStringList splitMessageIntoChunks(const QString &text) const;
    
    enum class DeleteRequestScope {
        Prompt,
        MeOnly,
        BothSides
    };
    void handleDeleteRequest(MessageComponent *item, DeleteRequestScope scope = DeleteRequestScope::Prompt);

    // View
    ServerChatWindow *m_serverView;

    // Model
    WebSocketServer *m_server;
    DatabaseManager *m_db;

    // State
    QString m_currentPrivateTargetUser;
    QString m_currentFilteredUser;
    bool m_isBroadcastMode;
};

#endif // SERVERCONTROLLER_H
