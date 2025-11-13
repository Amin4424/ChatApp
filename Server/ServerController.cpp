#include "ServerController.h"
#include "UI/ServerChatWindow.h"
#include "Network/WebSocketServer.h"
#include "Core/DatabaseManager.h"
#include "TextMessageItem.h"
#include "FileMessageItem.h"

#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

ServerController::ServerController(ServerChatWindow *view, WebSocketServer *server, DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_serverView(view)
    , m_server(server)
    , m_db(db)
    , m_isBroadcastMode(true)
{
    qDebug() << "🔥 [ServerController] Constructor called";
    
    if (m_serverView) {
        qDebug() << "🔥 [ServerController] Connecting signals from ServerChatWindow...";
        
        // FIX: Use sendMessageRequested instead of messageSent
        connect(m_serverView, &ServerChatWindow::sendMessageRequested, this, &ServerController::displayNewMessages);
        qDebug() << "✅ [ServerController] Connected sendMessageRequested signal";
        
        connect(m_serverView, &ServerChatWindow::userSelected, this, &ServerController::onUserSelected);
        connect(m_serverView, &ServerChatWindow::fileUploaded, this, &ServerController::onFileUploaded);
    } else {
        qDebug() << "⚠️ [ServerController] m_serverView is NULL!";
    }

    if (m_server) {
        qDebug() << "🔥 [ServerController] Connecting signals from WebSocketServer...";
        connect(m_server, &WebSocketServer::messageReceived, this, &ServerController::onServerMessageReceived);
        connect(m_server, &WebSocketServer::userListChanged, this, &ServerController::onUserListChanged);
        connect(m_server, &WebSocketServer::userCountChanged, this, &ServerController::onUserCountChanged);
        qDebug() << "✅ [ServerController] Connected all WebSocketServer signals";
    } else {
        qDebug() << "⚠️ [ServerController] m_server is NULL!";
    }

    loadHistoryForServer();
}

ServerController::~ServerController()
{
}

void ServerController::displayNewConnection()
{
    qDebug() << "ServerController: New connection established";
}

void ServerController::displayNewMessages(const QString &message)
{
    qDebug() << "🔥 [ServerController] displayNewMessages called with message:" << message;
    
    if (message.isEmpty()) {
        qDebug() << "⚠️ [ServerController] Attempted to send empty message - aborting";
        return;
    }

    qDebug() << "✅ [ServerController] Message validation passed";
    qDebug() << "🔥 [ServerController] Broadcast mode:" << m_isBroadcastMode;
    qDebug() << "🔥 [ServerController] Current target user:" << m_currentPrivateTargetUser;

    if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
        qDebug() << "🔥 [ServerController] Broadcasting to all clients...";
        
        // Broadcast to all clients
        if (m_server) {
            QString jsonMessage = QString(R"({"type":"text","content":"%1","sender":"Server","timestamp":"%2"})")
                                      .arg(message)
                                      .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
            qDebug() << "✅ [ServerController] JSON created:" << jsonMessage;
            
            m_server->broadcastToAll(jsonMessage);
            qDebug() << "✅ [ServerController] Broadcast sent via WebSocket";
        } else {
            qDebug() << "❌ [ServerController] m_server is NULL - cannot broadcast!";
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", "ALL", message, QDateTime::currentDateTime());
            qDebug() << "✅ [ServerController] Message saved to database (broadcast)";
        } else {
            qDebug() << "⚠️ [ServerController] m_db is NULL - message not saved";
        }
        
        // Show in server window
        if (m_serverView) {
            QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
            TextMessageItem *msgItem = new TextMessageItem(
                message,
                "You",  // Just show "You" for sent messages
                BaseChatWindow::MessageType::Sent,
                timestamp
            );
            m_serverView->addMessageItem(msgItem);
            qDebug() << "✅ [ServerController] Message added to UI (broadcast)";
        } else {
            qDebug() << "⚠️ [ServerController] m_serverView is NULL - message not shown in UI";
        }
    } else {
        qDebug() << "🔥 [ServerController] Sending to specific user...";
        
        // Send to specific user
        QString cleanUserId = m_currentPrivateTargetUser.split(" - ").first().trimmed();
        qDebug() << "✅ [ServerController] Clean user ID:" << cleanUserId;
        
        if (m_server) {
            QString jsonMessage = QString(R"({"type":"text","content":"%1","sender":"Server","timestamp":"%2"})")
                                      .arg(message)
                                      .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
            qDebug() << "✅ [ServerController] JSON created:" << jsonMessage;
            
            m_server->sendMessageToClient(cleanUserId, jsonMessage);
            qDebug() << "✅ [ServerController] Message sent to client via WebSocket";
        } else {
            qDebug() << "❌ [ServerController] m_server is NULL - cannot send!";
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", cleanUserId, message, QDateTime::currentDateTime());
            qDebug() << "✅ [ServerController] Message saved to database (private)";
        } else {
            qDebug() << "⚠️ [ServerController] m_db is NULL - message not saved";
        }
        
        // Show in server window
        if (m_serverView) {
            QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
            TextMessageItem *msgItem = new TextMessageItem(
                message,
                "You",  // Just show "You" for sent messages
                BaseChatWindow::MessageType::Sent,
                timestamp
            );
            m_serverView->addMessageItem(msgItem);
            qDebug() << "✅ [ServerController] Message added to UI (private)";
        } else {
            qDebug() << "⚠️ [ServerController] m_serverView is NULL - message not shown in UI";
        }
    }
    
    qDebug() << "✅ [ServerController] displayNewMessages completed successfully";
}

void ServerController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender)
{
    qDebug() << "ServerController: Displaying file message:" << fileName << fileSize << fileUrl << sender;
    
    QString senderInfo = sender.isEmpty() ? "Server" : sender;
    auto type = sender.isEmpty() ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received;
    
    if (m_serverView) {
        QString serverHost = m_server ? m_server->getServerIpAddress() : "localhost";
        FileMessageItem *fileItem = new FileMessageItem(
            fileName,
            fileSize,
            fileUrl,
            senderInfo,
            type,
            serverHost
        );
        m_serverView->addMessageItem(fileItem);
    }
}

void ServerController::onUserListChanged(const QStringList &users)
{
    qDebug() << "ServerController: User list updated:" << users;
    
    if (m_serverView) {
        m_serverView->updateUserList(users);
    }
}

void ServerController::onUserCountChanged(int count)
{
    qDebug() << "ServerController: User count:" << count;
    
    if (m_serverView) {
        m_serverView->updateServerInfo("0.0.0.0", 8080, QString("%1 users").arg(count));
    }
}

void ServerController::onUserSelected(const QString &userId)
{
    qDebug() << "ServerController: User selected:" << userId;
    
    if (userId == "ALL" || userId.isEmpty() || userId == "All Users") {
        m_isBroadcastMode = true;
        m_currentPrivateTargetUser.clear();
        m_currentFilteredUser.clear();
        loadHistoryForServer();
    } else {
        m_isBroadcastMode = false;
        m_currentPrivateTargetUser = userId;
        m_currentFilteredUser = userId;
        loadHistoryForUser(userId);
    }
}

void ServerController::onServerMessageReceived(const QString &message, const QString &senderId)
{
    qDebug() << "🔥 [ServerController] Message received from" << senderId;
    qDebug() << "🔥 [ServerController] Raw message:" << message;
    
    // Parse JSON message
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "❌ [ServerController] Invalid JSON message - cannot parse";
        return;
    }
    
    qDebug() << "✅ [ServerController] JSON parsed successfully";
    
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QString content = obj["content"].toString();
    QString sender = obj["sender"].toString();
    
    qDebug() << "✅ [ServerController] Message type:" << type;
    qDebug() << "✅ [ServerController] Content:" << content;
    qDebug() << "✅ [ServerController] Sender:" << sender;
    
    if (type == "text") {
        qDebug() << "🔥 [ServerController] Processing text message...";
        
        // Save to database
        if (m_db) {
            m_db->saveMessage(senderId, "Server", content, QDateTime::currentDateTime());
            qDebug() << "✅ [ServerController] Message saved to database";
        } else {
            qDebug() << "⚠️ [ServerController] m_db is NULL - message not saved";
        }
        
        // Display in server window
        if (m_serverView) {
            QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
            qDebug() << "🔥 [ServerController] Current filtered user:" << cleanFilteredUser;
            qDebug() << "🔥 [ServerController] Broadcast mode:" << m_isBroadcastMode;
            
            if (m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderId == cleanFilteredUser) {
                // Extract timestamp from JSON or use current time
                QString timestamp = obj["timestamp"].toString();
                if (timestamp.isEmpty()) {
                    timestamp = QDateTime::currentDateTime().toString("hh:mm");
                } else {
                    // Parse ISO format timestamp and convert to hh:mm
                    QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
                    timestamp = dt.toString("hh:mm");
                }
                
                TextMessageItem *msgItem = new TextMessageItem(
                    content,
                    senderId,  // Use senderId (e.g., "User #1") instead of sender from JSON
                    BaseChatWindow::MessageType::Received,
                    timestamp
                );
                m_serverView->addMessageItem(msgItem);
                qDebug() << "✅ [ServerController] Message added to UI";
            } else {
                qDebug() << "⚠️ [ServerController] Message filtered out (not from current user)";
            }
        } else {
            qDebug() << "⚠️ [ServerController] m_serverView is NULL - message not shown in UI";
        }
    } else if (type == "file") {
        qDebug() << "🔥 [ServerController] Processing file message...";
        
        QString fileName = obj["fileName"].toString();
        qint64 fileSize = obj["fileSize"].toInteger();
        QString fileUrl = obj["fileUrl"].toString();
        
        qDebug() << "✅ [ServerController] File name:" << fileName;
        qDebug() << "✅ [ServerController] File size:" << fileSize;
        qDebug() << "✅ [ServerController] File URL:" << fileUrl;
        
        // Save file message to database
        if (m_db) {
            m_db->saveMessage(senderId, "Server", QString("File: %1 (%2 bytes) - %3").arg(fileName).arg(fileSize).arg(fileUrl), QDateTime::currentDateTime());
            qDebug() << "✅ [ServerController] File message saved to database";
        } else {
            qDebug() << "⚠️ [ServerController] m_db is NULL - file message not saved";
        }
        
        // Display in server window
        if (m_serverView) {
            QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
            if (m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderId == cleanFilteredUser) {
                QString serverHost = m_server ? m_server->getServerIpAddress() : "localhost";
                FileMessageItem *fileItem = new FileMessageItem(
                    fileName,
                    fileSize,
                    fileUrl,
                    senderId,  // Use senderId instead of sender from JSON
                    BaseChatWindow::MessageType::Received,
                    serverHost
                );
                m_serverView->addMessageItem(fileItem);
            }
        }
    }
}

void ServerController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost)
{
    qDebug() << "ServerController: File uploaded:" << fileName << url << fileSize << serverHost;
    
    // Create file message JSON
    QString jsonMessage = QString(R"({"type":"file","fileName":"%1","fileSize":%2,"fileUrl":"%3","sender":"Server","timestamp":"%4"})")
                              .arg(fileName)
                              .arg(fileSize)
                              .arg(url)
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
        // Broadcast to all
        if (m_server) {
            m_server->broadcastToAll(jsonMessage);
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", "ALL", QString("File: %1").arg(fileName), QDateTime::currentDateTime());
        }
    } else {
        // Send to specific user
        QString cleanUserId = m_currentPrivateTargetUser.split(" - ").first().trimmed();
        
        if (m_server) {
            m_server->sendMessageToClient(cleanUserId, jsonMessage);
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", cleanUserId, QString("File: %1").arg(fileName), QDateTime::currentDateTime());
        }
    }
    
    // Display in server window
    displayFileMessage(fileName, fileSize, url, "");
}

void ServerController::loadHistoryForServer()
{
    if (!m_db || !m_serverView) return;
    
    qDebug() << "ServerController: Loading broadcast message history (Server -> ALL)";
    
    m_serverView->clearChatHistory();
    // Load only broadcast messages (Server -> ALL)
    QStringList history = m_db->loadMessagesBetween("Server", "ALL");
    
    qDebug() << "Loaded" << history.size() << "broadcast messages from database";
    
    for (const QString &msg : history) {
        m_serverView->showMessage(msg);
    }
}

void ServerController::loadHistoryForUser(const QString &userId)
{
    if (!m_db || !m_serverView) return;
    
    QString cleanUserId = userId.split(" - ").first().trimmed();
    qDebug() << "ServerController: Loading message history for user:" << cleanUserId;
    
    m_serverView->clearChatHistory();
    
    // Load messages between server and this user
    QStringList history = m_db->loadMessagesBetween("Server", cleanUserId);
    
    for (const QString &msg : history) {
        m_serverView->showMessage(msg);
    }
}
