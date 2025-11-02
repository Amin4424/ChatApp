#ifndef ConnectPageController_H
#define ConnectPageController_H

#include <QObject>

class ConnectPage;
class ChatWindow;
class ServerChatWindow;
class ChatController;
class WebSocketClient;
class WebSocketServer;
class DatabaseManager;

class ConnectPageController : public QObject
{
    Q_OBJECT
public:

    explicit ConnectPageController(ConnectPage *connectPage, ChatWindow *chatWindow, ServerChatWindow *serverChatWindow, QObject *parent = nullptr);
    ~ConnectPageController();

private slots:
    void onConnectClient(const QString &ipAddress);
    void onCreateServer();
    void onConnectionSuccess();
    void onConnectionFailed(const QString &error);

private:

    ConnectPage *m_connectPage;
    ChatWindow  *m_chatWindow;
    ServerChatWindow *m_serverChatWindow;
    ChatController *m_chatController;

    WebSocketClient *m_client;
    WebSocketServer *m_server;
    DatabaseManager *m_clientDb;
    DatabaseManager *m_serverDb;
    QString m_serverUrl;
};

#endif // ConnectPageController_H
