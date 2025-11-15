#include "ServerController.h"
#include "View/ServerChatWindow.h"
#include "Model/Network/WebSocketServer.h"
#include "Model/Core/DatabaseManager.h"
#include "TextMessageItem.h"
#include "FileMessageItem.h"
#include "MessageData.h"

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
    if (m_serverView) {
        // FIX: Use sendMessageRequested instead of messageSent
        connect(m_serverView, &ServerChatWindow::sendMessageRequested, this, &ServerController::displayNewMessages);
        connect(m_serverView, &ServerChatWindow::userSelected, this, &ServerController::onUserSelected);
        connect(m_serverView, &ServerChatWindow::fileUploaded, this, &ServerController::onFileUploaded);
    } else {
    }

    if (m_server) {
        connect(m_server, &WebSocketServer::messageReceived, this, &ServerController::onServerMessageReceived);
        connect(m_server, &WebSocketServer::userListChanged, this, &ServerController::onUserListChanged);
        connect(m_server, &WebSocketServer::userCountChanged, this, &ServerController::onUserCountChanged);
    } else {
    }

    loadHistoryForServer();
}

ServerController::~ServerController()
{
}

QWidget* ServerController::createWidgetFromData(const MessageData &msgData)
{
    QString serverHost = m_server ? m_server->getServerIpAddress() : "localhost";
    
    if (msgData.isFileMessage) {
        return new FileMessageItem(
            msgData.fileInfo.fileName,
            msgData.fileInfo.fileSize,
            msgData.fileInfo.fileUrl,
            msgData.senderName,
            (msgData.senderType == MessageData::User_Me) ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received,
            serverHost
        );
    } else {
        return new TextMessageItem(
            msgData.text,
            msgData.senderName,
            (msgData.senderType == MessageData::User_Me) ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received,
            msgData.timestamp
        );
    }
}

void ServerController::displayNewConnection()
{
}

void ServerController::displayNewMessages(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }
    // 1. Create MessageData for sent message
    MessageData msgData;
    msgData.text = message;
    msgData.senderName = "You";
    msgData.senderType = MessageData::User_Me;
    msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    msgData.isFileMessage = false;

    // 2. Prepare JSON message
    QString jsonMessage = QString(R"({"type":"text","content":"%1","sender":"Server","timestamp":"%2"})")
                              .arg(message)
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
        // Broadcast to all clients
        if (m_server) {
            m_server->broadcastToAll(jsonMessage);
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", "ALL", message, QDateTime::currentDateTime());
        }

    } else {
        // Send to specific user
        QString cleanUserId = m_currentPrivateTargetUser.split(" - ").first().trimmed();
        
        if (m_server) {
            m_server->sendMessageToClient(cleanUserId, jsonMessage);
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", cleanUserId, message, QDateTime::currentDateTime());
        }
    }
    
    // 3. Create widget and show in server window
    QWidget* itemWidget = createWidgetFromData(msgData);
    if (m_serverView) {
        m_serverView->addMessageItem(itemWidget);
        
        // **NEW: Mark text message as sent (change from 🕒 to ✓)**
        TextMessageItem* textItem = qobject_cast<TextMessageItem*>(itemWidget);
        if (textItem) {
            textItem->markAsSent();
        }
    }
}

void ServerController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender)
{
    
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
    if (m_serverView) {
        m_serverView->updateUserList(users);
    }
}

void ServerController::onUserCountChanged(int count)
{
    if (m_serverView) {
        m_serverView->updateServerInfo("0.0.0.0", 8080, QString("%1 users").arg(count));
    }
}

void ServerController::onUserSelected(const QString &userId)
{
    
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
    // 1. Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // 2. Create MessageData object (Model)
    MessageData msgData;
    msgData.senderType = MessageData::User_Other; // Always from client
    msgData.senderName = senderId; // e.g., "User #1"
    
    QString timestampStr = obj["timestamp"].toString();
    if (!timestampStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(timestampStr, Qt::ISODate);
        msgData.timestamp = dt.toString("hh:mm");
    } else {
        msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    }
    
    QString type = obj["type"].toString();
    if (type == "text") {
        msgData.isFileMessage = false;
        msgData.text = obj["content"].toString();
    } else if (type == "file") {
        msgData.isFileMessage = true;
        msgData.fileInfo.fileName = obj["fileName"].toString();
        msgData.fileInfo.fileSize = obj["fileSize"].toInteger();
        msgData.fileInfo.fileUrl = obj["fileUrl"].toString();
    }
    
    // 3. Save to database (with standard format)
    if (m_db) {
        QString dbMessage = msgData.isFileMessage ? 
            QString("FILE|%1|%2|%3").arg(msgData.fileInfo.fileName).arg(msgData.fileInfo.fileSize).arg(msgData.fileInfo.fileUrl) :
            msgData.text;
        m_db->saveMessage(senderId, "Server", dbMessage, QDateTime::currentDateTime());
    }
    
    // 4. Display in server window (only if viewing this user or in broadcast mode)
    if (m_serverView) {
        QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
        
        if (m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderId == cleanFilteredUser) {
            // Create widget and add to view
            QWidget* itemWidget = createWidgetFromData(msgData);
            m_serverView->addMessageItem(itemWidget);
        } else {
        }
    }
}

void ServerController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost)
{
    // 1. Create MessageData for sent file
    MessageData msgData;
    msgData.senderName = "You";
    msgData.senderType = MessageData::User_Me;
    msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    msgData.isFileMessage = true;
    msgData.fileInfo.fileName = fileName;
    msgData.fileInfo.fileSize = fileSize;
    msgData.fileInfo.fileUrl = url;
    
    // 2. Create file message JSON
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
        
        // Save to database in FILE|name|size|url format
        if (m_db) {
            QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(url);
            m_db->saveMessage("Server", "ALL", fileMessage, QDateTime::currentDateTime());
        }
    } else {
        // Send to specific user
        QString cleanUserId = m_currentPrivateTargetUser.split(" - ").first().trimmed();
        
        if (m_server) {
            m_server->sendMessageToClient(cleanUserId, jsonMessage);
        }
        
        // Save to database in FILE|name|size|url format
        if (m_db) {
            QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(url);
            m_db->saveMessage("Server", cleanUserId, fileMessage, QDateTime::currentDateTime());
        }
    }
    
    // **FIX: دیگر نیازی به ساخت FileMessageItem نیست - قبلاً در View ساخته شده**
    // FileMessageItem already created in ServerChatWindow::onSendFileClicked()
    // Just update state to Completed_Sent (already done in View)
}

void ServerController::loadHistoryForServer()
{
    if (!m_db || !m_serverView) return;
    
    m_serverView->clearChatHistory();
    // Load only broadcast messages (Server -> ALL)
    QStringList history = m_db->loadMessagesBetween("Server", "ALL");
    
    for (const QString &dbString : history) {
        // dbString format: "hh:mm|Sender|Message"
        QStringList parts = dbString.split("|");
        if (parts.size() < 3) continue;
        
        MessageData msgData;
        msgData.timestamp = parts[0];
        msgData.senderName = parts[1];
        QString messageContent = parts.mid(2).join("|");
        
        // Determine sender type
        if (msgData.senderName == "Server" || msgData.senderName == "You") {
            msgData.senderType = MessageData::User_Me;
            msgData.senderName = "You";
        } else {
            msgData.senderType = MessageData::User_Other;
        }
        
        // Check if it's a file message
        if (messageContent.startsWith("FILE|")) {
            QStringList fileParts = messageContent.split("|");
            if (fileParts.size() >= 4) {
                msgData.isFileMessage = true;
                msgData.fileInfo.fileName = fileParts[1];
                msgData.fileInfo.fileSize = fileParts[2].toLongLong();
                msgData.fileInfo.fileUrl = fileParts[3];
            }
        } else {
            msgData.isFileMessage = false;
            msgData.text = messageContent;
        }
        
        // Create widget and add to view
        QWidget* itemWidget = createWidgetFromData(msgData);
        m_serverView->addMessageItem(itemWidget);
    }
}

void ServerController::loadHistoryForUser(const QString &userId)
{
    if (!m_db || !m_serverView) return;
    
    QString cleanUserId = userId.split(" - ").first().trimmed();
    
    m_serverView->clearChatHistory();
    
    // Load messages between server and this user
    QStringList history = m_db->loadMessagesBetween("Server", cleanUserId);
    
    for (const QString &dbString : history) {
        // dbString format: "hh:mm|Sender|Message"
        QStringList parts = dbString.split("|");
        if (parts.size() < 3) continue;
        
        MessageData msgData;
        msgData.timestamp = parts[0];
        msgData.senderName = parts[1];
        QString messageContent = parts.mid(2).join("|");
        
        // Determine sender type
        if (msgData.senderName == "Server" || msgData.senderName == "You") {
            msgData.senderType = MessageData::User_Me;
            msgData.senderName = "You";
        } else {
            msgData.senderType = MessageData::User_Other;
            // Use the clean user ID for display
            if (msgData.senderName != cleanUserId) {
                msgData.senderName = cleanUserId;
            }
        }
        
        // Check if it's a file message
        if (messageContent.startsWith("FILE|")) {
            QStringList fileParts = messageContent.split("|");
            if (fileParts.size() >= 4) {
                msgData.isFileMessage = true;
                msgData.fileInfo.fileName = fileParts[1];
                msgData.fileInfo.fileSize = fileParts[2].toLongLong();
                msgData.fileInfo.fileUrl = fileParts[3];
            }
        } else {
            msgData.isFileMessage = false;
            msgData.text = messageContent;
        }
        
        // Create widget and add to view
        QWidget* itemWidget = createWidgetFromData(msgData);
        m_serverView->addMessageItem(itemWidget);
    }
}
