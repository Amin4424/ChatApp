#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>
#include <QList>
#include <QMap>

class WebSocketServer : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(quint16 port, QObject *parent = nullptr);
    ~WebSocketServer();
    void sendMessage(const QString &message);
    void sendMessageToClient(const QString &userId, const QString &message);
    void broadcastToAll(const QString &message);
    QStringList getConnectedUsers() const;
    int getConnectedUserCount() const { return m_clients.size(); }

signals:
    void newConnectionRequested();
    void messageReceived(const QString &message, const QString &senderId);
    void userListChanged(const QStringList &users);
    void userCountChanged(int count);

private slots:
    void onNewConnection();
    void processMessage(QString message);
    void socketDisconnected();

private:
    void updateUserList();
    QString generateUserId(QWebSocket *socket);
    QWebSocket* getSocketByUserId(const QString &userId);
    
    QWebSocketServer *m_pWebSocketServer;
    QList <QWebSocket*> m_clients;
    QMap<QWebSocket*, QString> m_clientIds; // Maps socket to user ID
};

#endif // WEBSOCKETSERVER_H
