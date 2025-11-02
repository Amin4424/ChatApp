#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H

#include <QWidget>

class WebSocketServer;
class WebSocketClient;
class ChatWindow;
class ServerChatWindow;
class DatabaseManager;

class ChatController : public QWidget
{
    Q_OBJECT

public:
    explicit ChatController(QWidget *parent = nullptr);
    ~ChatController();
    void initClient(ChatWindow *view, WebSocketClient *client, DatabaseManager *db);
    void initServer(ServerChatWindow *view, WebSocketServer *server, DatabaseManager *db);

signals:
    void connectClient(QString ip);
    void createServer();
    
private slots:
    void displayNewConnection();
    void displayNewMessages(const QString &message);
    void onMessageReceived(const QString &message);
    void onServerMessageReceived(const QString &message, const QString &senderId);
    void onUserListChanged(const QStringList &users);
    void onUserCountChanged(int count);
    void onUserSelected(const QString &userId);

private:
    void loadHistoryForClient();
    void loadHistoryForServer();
    void loadHistoryForUser(const QString &userId);
    
    ChatWindow      *m_clientView = nullptr;
    ServerChatWindow *m_serverView = nullptr;
    WebSocketClient *m_client = nullptr;
    WebSocketServer *m_server = nullptr;
    DatabaseManager *m_db = nullptr;
    QString m_currentPrivateTargetUser;
    QString m_clientUserId; // For client to identify itself
    bool m_isBroadcastMode;
    QString m_currentFilteredUser; // Track which user's messages are currently displayed
};

#endif // CHATCONTROLLER_H
