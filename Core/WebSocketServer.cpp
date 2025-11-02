#include "WebSocketServer.h"
#include <QDebug>
#include <QHostAddress>

WebSocketServer::WebSocketServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Echo Server"),
                                              QWebSocketServer::NonSecureMode,
                                              this))
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Server listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebSocketServer::onNewConnection);
    } else {
        qWarning() << "Failed to listen on port" << port << "- check if the port is already in use or you have sufficient permissions.";
    }
}

WebSocketServer::~WebSocketServer()
{
    if (m_pWebSocketServer)
        m_pWebSocketServer->close();

    foreach (QWebSocket* m_client, m_clients)
    {
        if(m_client)
        {
            m_client->close();
            m_client->deleteLater();
            m_client = nullptr;
        }

    }
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    QString userId = generateUserId(pSocket);
    
    qDebug() << "New client connected:" << userId << "from" << pSocket->peerAddress().toString();

    connect(pSocket, &QWebSocket::textMessageReceived,
            this, &WebSocketServer::processMessage);
    connect(pSocket, &QWebSocket::disconnected,
            this, &WebSocketServer::socketDisconnected);

    m_clients.append(pSocket);
    m_clientIds.insert(pSocket, userId);
    
    updateUserList();
}

void WebSocketServer::processMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    QString senderId = m_clientIds.value(pClient, "Unknown");
    qDebug() << "Message received from client" << senderId << ":" << message;
    
    // Emit signal with sender ID so ChatController can display it on server
    emit messageReceived(message, senderId);
    
    // NOTE: Do NOT broadcast to other clients automatically
    // Server will decide whether to send to specific client or broadcast
}

void WebSocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        QString userId = m_clientIds.value(pClient, "Unknown");
        qDebug() << "Client disconnected:" << userId;
        
        m_clients.removeAll(pClient);
        m_clientIds.remove(pClient);
        pClient->deleteLater();
        
        updateUserList();
    }
}

void WebSocketServer::sendMessage(const QString &message){
    if (message.isEmpty()) {
        qWarning() << "Cannot send empty message";
        return;
    }
    
    // Send message to all connected clients
    bool sentToAny = false;
    for (QWebSocket *client : m_clients) {
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(message);
            sentToAny = true;
            qDebug() << "Message sent to client:" << message;
        }
    }
    
    if (!sentToAny) {
        qWarning() << "No connected clients to send message to.";
    }
}

void WebSocketServer::updateUserList()
{
    QStringList users = getConnectedUsers();
    int count = m_clients.size();
    
    qDebug() << "User list updated. Connected users:" << count;
    emit userListChanged(users);
    emit userCountChanged(count);
}

QStringList WebSocketServer::getConnectedUsers() const
{
    QStringList users;
    for (QWebSocket *client : m_clients) {
        QString userId = m_clientIds.value(client, "Unknown");
        QString userInfo = QString("%1 - %2").arg(userId, client->peerAddress().toString());
        users.append(userInfo);
    }
    return users;
}

QString WebSocketServer::generateUserId(QWebSocket *socket)
{
    static int userCounter = 1;
    return QString("کاربر شماره %1").arg(userCounter++);
}

void WebSocketServer::sendMessageToClient(const QString &userId, const QString &message)
{
    if (message.isEmpty()) {
        qWarning() << "Cannot send empty message";
        return;
    }
    
    qDebug() << "\n=== PRIVATE MESSAGE DEBUG ===";
    qDebug() << "Attempting to send message to userId:" << userId;
    qDebug() << "Message content:" << message;
    qDebug() << "Available client IDs:" << m_clientIds.values();
    
    QWebSocket *targetSocket = getSocketByUserId(userId);
    if (targetSocket) {
        qDebug() << "✓ Found target socket for:" << userId;
        qDebug() << "Socket state:" << targetSocket->state();
        
        if (targetSocket->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesSent = targetSocket->sendTextMessage(message);
            qDebug() << "✓ Message sent - bytes:" << bytesSent;
        } else {
            qWarning() << "✗ Socket not connected, state:" << targetSocket->state();
        }
    } else {
        qWarning() << "✗ Client" << userId << "not found";
    }
    qDebug() << "=== END DEBUG ===\n";
}

void WebSocketServer::broadcastToAll(const QString &message)
{
    if (message.isEmpty()) {
        qWarning() << "Cannot send empty message";
        return;
    }
    
    // Send message to all connected clients
    bool sentToAny = false;
    for (QWebSocket *client : m_clients) {
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(message);
            sentToAny = true;
        }
    }
    
    if (sentToAny) {
        qDebug() << "Broadcast message sent to all clients:" << message;
    } else {
        qWarning() << "No connected clients to send message to.";
    }
}

QWebSocket* WebSocketServer::getSocketByUserId(const QString &userId)
{
    // Extract just the user ID part (before " - IP") and trim whitespace
    QString cleanUserId = userId.split(" - ").first().trimmed();
    qDebug() << "Looking for socket with userId:" << cleanUserId;
    
    for (auto it = m_clientIds.begin(); it != m_clientIds.end(); ++it) {
        qDebug() << "Checking against:" << it.value();
        if (it.value() == cleanUserId || it.value().trimmed() == userId.trimmed()) {
            qDebug() << "Match found!";
            return it.key();
        }
    }
    qDebug() << "No match found";
    return nullptr;
}
