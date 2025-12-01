#include "Model/Network/WebSocketServer.h"
#include "Model/Network/TusServer.h"
#include "CryptoManager.h"
#include <QHostAddress>
#include <QNetworkInterface>

WebSocketServer::WebSocketServer(quint16 port, QObject *parent)
    : QObject(parent)
    , m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Echo Server"),
                                              QWebSocketServer::NonSecureMode,
                                              this))
    , m_tusServer(new TusServer(this))
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebSocketServer::onNewConnection);
    } else {
        qWarning() << "Failed to listen on port" << port << "- check if the port is already in use or you have sufficient permissions.";
    }

    // Start TUS (HTTP) server on port 1080
    // Note: tusd binary should be in the same directory as the executable
    if (!m_tusServer->start(1080, "uploads")) {
        qWarning() << "TUS server not started (tusd binary not found) - File uploads will not work";
    } else {
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

    // Stop TUS server
    if (m_tusServer) {
        m_tusServer->stop();
    }
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();
    QString userId = generateUserId(pSocket);
    
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
    QString decryptedMessage = CryptoManager::decryptMessage(message);
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    QString senderId = m_clientIds.value(pClient, "Unknown");
    
    // Emit signal with sender ID so ChatController can display it on server
    emit messageReceived(decryptedMessage, senderId);
    
    // NOTE: Do NOT broadcast to other clients automatically
}

void WebSocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (pClient) {
        QString userId = m_clientIds.value(pClient, "Unknown");
        
        m_clients.removeAll(pClient);
        m_clientIds.remove(pClient);
        pClient->deleteLater();
        
        updateUserList();
    }
}

// void WebSocketServer::sendMessage(const QString &message){
//     QString encryptedMessage = CryptoManager::encryptMessage(message);
//     if (encryptedMessage.isEmpty()) {
//         qWarning() << "Cannot send empty message";
//         return;
    
//     // Send message to all connected clients
//     bool sentToAny = false;
//     for (QWebSocket *client : m_clients) {
//         if (client && client->state() == QAbstractSocket::ConnectedState) {
//             sentToAny = true;
//         }
//     }
    
//     if (!sentToAny) {
//         qWarning() << "No connected clients to send message to.";
//     }
// }

void WebSocketServer::updateUserList()
{
    QStringList users = getConnectedUsers();
    int count = m_clients.size();
    
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
    return QString("User #%1").arg(userCounter++);
}

void WebSocketServer::sendMessageToClient(const QString &userId, const QString &message)
{
    QString encryptedMessage = CryptoManager::encryptMessage(message);
    if (encryptedMessage.isEmpty()) {
        qWarning() << "Cannot send empty message";
        return;
    }
    
    
    QWebSocket *targetSocket = getSocketByUserId(userId);
    if (targetSocket) {
        
        if (targetSocket->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesSent = targetSocket->sendTextMessage(encryptedMessage);
        } else {
            qWarning() << "✗ Socket not connected, state:" << targetSocket->state();
        }
    } else {
        qWarning() << "✗ Client" << userId << "not found";
    }
}

void WebSocketServer::broadcastToAll(const QString &message)
{
    
    QString encryptedMessage = CryptoManager::encryptMessage(message);
    
    if (encryptedMessage.isEmpty()) {
        qWarning() << "✗ Cannot send empty encrypted message";
        return;
    }
    
    // Send message to all connected clients
    int sentCount = 0;
    int totalClients = 0;
    for (QWebSocket *client : m_clients) {
        totalClients++;
        QString userId = m_clientIds.value(client, "Unknown");
        
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesSent = client->sendTextMessage(encryptedMessage);
            sentCount++;
        } else {
        }
    }
    
    if (sentCount > 0) {
    } else {
    }
}

QWebSocket* WebSocketServer::getSocketByUserId(const QString &userId)
{
    // Extract just the user ID part (before " - IP") and trim whitespace
    QString cleanUserId = userId.split(" - ").first().trimmed();
    
    for (auto it = m_clientIds.begin(); it != m_clientIds.end(); ++it) {
        if (it.value() == cleanUserId || it.value().trimmed() == userId.trimmed()) {
            return it.key();
        }
    }
    return nullptr;
}

QString WebSocketServer::getServerIpAddress() const
{
    // Get the first non-loopback IPv4 address
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    
    for (const QHostAddress &address : addresses) {
        // Skip loopback and IPv6 addresses
        if (address.protocol() == QAbstractSocket::IPv4Protocol && 
            !address.isLoopback() && 
            address != QHostAddress::LocalHost) {
            
            QString ip = address.toString();
            return ip;
        }
    }
    
    // Fallback to localhost if no network interface found
    return "localhost";
}
