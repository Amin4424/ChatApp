#include "Model/Network/WebSocketServer.h"
#include "Model/Network/TusServer.h"
#include "CryptoManager.h"
#include <QDebug>
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
        qDebug() << "WebSocket Server listening on port" << port;
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
        qDebug() << "✓ TUS server started successfully on port 1080";
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
    QString decryptedMessage = CryptoManager::decryptMessage(message);
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    QString senderId = m_clientIds.value(pClient, "Unknown");
    qDebug() << "Message received from client" << senderId << ":" << decryptedMessage;
    
    // Emit signal with sender ID so ChatController can display it on server
    emit messageReceived(decryptedMessage, senderId);
    
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

// void WebSocketServer::sendMessage(const QString &message){
//     QString encryptedMessage = CryptoManager::encryptMessage(message);
//     if (encryptedMessage.isEmpty()) {
//         qWarning() << "Cannot send empty message";
//         return;
//     }
    
//     // Send message to all connected clients
//     bool sentToAny = false;
//     for (QWebSocket *client : m_clients) {
//         if (client && client->state() == QAbstractSocket::ConnectedState) {
//             client->sendTextMessage(encryptedMessage);
//             sentToAny = true;
//             qDebug() << "Message sent to client:" << encryptedMessage;
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
    return QString("User #%1").arg(userCounter++);
}

void WebSocketServer::sendMessageToClient(const QString &userId, const QString &message)
{
    QString encryptedMessage = CryptoManager::encryptMessage(message);
    if (encryptedMessage.isEmpty()) {
        qWarning() << "Cannot send empty message";
        return;
    }
    
    qDebug() << "\n=== PRIVATE MESSAGE DEBUG ===";
    qDebug() << "Attempting to send message to userId:" << userId;
    qDebug() << "Message content:" << encryptedMessage;
    qDebug() << "Available client IDs:" << m_clientIds.values();
    
    QWebSocket *targetSocket = getSocketByUserId(userId);
    if (targetSocket) {
        qDebug() << "✓ Found target socket for:" << userId;
        qDebug() << "Socket state:" << targetSocket->state();
        
        if (targetSocket->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesSent = targetSocket->sendTextMessage(encryptedMessage);
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
    qDebug() << "\n=== BROADCAST MESSAGE DEBUG ===";
    qDebug() << "Original message:" << message;
    qDebug() << "Number of connected clients:" << m_clients.size();
    
    QString encryptedMessage = CryptoManager::encryptMessage(message);
    qDebug() << "Encrypted message:" << encryptedMessage;
    
    if (encryptedMessage.isEmpty()) {
        qWarning() << "✗ Cannot send empty encrypted message";
        qDebug() << "=== END BROADCAST DEBUG ===\n";
        return;
    }
    
    // Send message to all connected clients
    int sentCount = 0;
    int totalClients = 0;
    for (QWebSocket *client : m_clients) {
        totalClients++;
        QString userId = m_clientIds.value(client, "Unknown");
        qDebug() << "Processing client" << totalClients << "- UserId:" << userId;
        
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesSent = client->sendTextMessage(encryptedMessage);
            sentCount++;
            qDebug() << "✓ Message sent to" << userId << "- bytes:" << bytesSent;
        } else {
            qWarning() << "✗ Client" << userId << "not connected, state:" 
                      << (client ? client->state() : -1);
        }
    }
    
    if (sentCount > 0) {
        qDebug() << "✓ Broadcast completed: sent to" << sentCount << "out of" << totalClients << "clients";
    } else {
        qWarning() << "✗ No connected clients to send message to.";
    }
    qDebug() << "=== END BROADCAST DEBUG ===\n";
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
            qDebug() << "Found server IP address:" << ip;
            return ip;
        }
    }
    
    // Fallback to localhost if no network interface found
    qDebug() << "No network IP found, using localhost";
    return "localhost";
}
