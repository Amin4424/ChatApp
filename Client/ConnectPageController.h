#ifndef CONNECTPAGECONTROLLER_H
#define CONNECTPAGECONTROLLER_H

#include <QObject>

class ConnectPage;
class ClientChatWindow;
class ClientController;
class WebSocketClient;
class DatabaseManager;

class ConnectPageController : public QObject
{
    Q_OBJECT
public:
    explicit ConnectPageController(ConnectPage *connectPage, QObject *parent = nullptr);
    ~ConnectPageController();

private slots:
    void onConnectClient(const QString &ipAddress);
    void onConnectionSuccess();
    void onConnectionFailed(const QString &error);

private:
    ConnectPage *m_connectPage;
    ClientChatWindow *m_chatWindow;
    ClientController *m_clientController;
    WebSocketClient *m_client;
    DatabaseManager *m_db;
    QString m_serverUrl;
};

#endif // CONNECTPAGECONTROLLER_H
