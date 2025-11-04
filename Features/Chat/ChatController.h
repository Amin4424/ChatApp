#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>

class ClientChatWindow;
class ServerChatWindow;
class WebSocketClient;
class WebSocketServer;
class DatabaseManager;

class ChatController : public QObject
{
    Q_OBJECT

public:
    explicit ChatController(QWidget *parent = nullptr);
    ~ChatController();

    // Initialize for client mode
    void initClient(ClientChatWindow *view, WebSocketClient *client, DatabaseManager *db);
    // Initialize for server mode
    void initServer(ServerChatWindow *view, WebSocketServer *server, DatabaseManager *db);

    void displayNewConnection();

public slots:
    void displayNewMessages(const QString &message);
    void displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender = "");

private slots:
    void onMessageReceived(const QString &message);
    void onUserListChanged(const QStringList &users);
    void onUserCountChanged(int count);
    void onUserSelected(const QString &userId);
    void onServerMessageReceived(const QString &message, const QString &senderId);
    void onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize);

private:
    void loadHistoryForClient();
    void loadHistoryForServer();
    void loadHistoryForUser(const QString &userId);

    // Views
    ClientChatWindow *m_clientView;
    ServerChatWindow *m_serverView;

    // Models
    WebSocketClient *m_client;
    WebSocketServer *m_server;
    DatabaseManager *m_db;

    // State
    QString m_clientUserId;
    QString m_currentPrivateTargetUser;
    QString m_currentFilteredUser;
    bool m_isBroadcastMode;
};

#endif // CHATCONTROLLER_H
