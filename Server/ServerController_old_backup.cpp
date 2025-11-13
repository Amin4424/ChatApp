#include "ServerController.h"
#include "UI/ServerChatWindow.h"
#include "Network/WebSocketServer.h"
#include "Core/DatabaseManager.h"
#include "TextMessageItem.h"
#include "FileMessageItem.h"

#include <QDebug>
#include <QDateTime>
#include <QRegularExpression> // <-- ADD THIS

ChatController::ChatController(QWidget *parent)
    : QObject(parent)
    , m_currentPrivateTargetUser("")
    , m_isBroadcastMode(true)
    , m_client(nullptr)
    , m_server(nullptr)
    , m_clientView(nullptr)
    , m_serverView(nullptr)
    , m_db(nullptr)
{

    // Default client user id to avoid NULL/empty sender when saving messages
    m_clientUserId = "Client";
}

ChatController::~ChatController()
{
    
}

void ChatController::initClient(ClientChatWindow *view, WebSocketClient *client, DatabaseManager *db)
{
    m_clientView = view;
    m_client = client;
    m_db = db;
    m_clientUserId = "Client";
    
    if (m_db) {
        loadHistoryForClient();
    }
    
    connect(m_clientView, &ClientChatWindow::sendMessageRequested,
            this, &ChatController::displayNewMessages);
    
    // --- FIX: Connect to the overridden signal ---
    connect(m_clientView, &ClientChatWindow::fileUploaded,
            this, &ChatController::onFileUploaded);
    // --- END FIX ---
    
    if (m_client) {
        connect(m_client, &WebSocketClient::messageReceived,
                this, &ChatController::onMessageReceived);
    }
}

void ChatController::initServer(ServerChatWindow *view, WebSocketServer *server, DatabaseManager *db)
{
    m_serverView = view;
    m_server = server;
    m_db = db;
    m_isBroadcastMode = true;
    
    if (m_db) {
        loadHistoryForServer();
    }
    
    connect(m_serverView, &ServerChatWindow::sendMessageRequested,
            this, &ChatController::displayNewMessages);
    
    // --- FIX: Connect to the base signal ---
    connect(m_serverView, &ServerChatWindow::fileUploaded,
            this, &ChatController::onFileUploaded);
    // --- END FIX ---
    
    connect(m_serverView, &ServerChatWindow::userSelected,
            this, &ChatController::onUserSelected);
    
    if (m_server) {
        connect(m_server, &WebSocketServer::messageReceived,
                this, &ChatController::onServerMessageReceived);
        
        connect(m_server, &WebSocketServer::userListChanged,
                this, &ChatController::onUserListChanged);
        connect(m_server, &WebSocketServer::userCountChanged,
                this, &ChatController::onUserCountChanged);
        
        m_serverView->updateUserList(m_server->getConnectedUsers());
        m_serverView->updateUserCount(m_server->getConnectedUserCount());
    }
}

void ChatController::displayNewConnection()
{

}

void ChatController::displayNewMessages(const QString &message)
{
    if(message.isEmpty()) return;
    
    QDateTime utcTime = QDateTime::currentDateTimeUtc();
    QString timeString = utcTime.toString("HH:mm:");
    
    if (m_client) {
        qDebug() << "Sending message via client:" << message;
        m_client->sendMessage(message);
        
        if (m_db) {
            m_db->saveMessage(m_clientUserId, "Server", message, utcTime);
        }
        
        QString formattedMessage = QString("[%1] You: %2").arg(timeString, message);
        m_clientView->showMessage(formattedMessage);
    } else if (m_server) {
        if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
            qDebug() << "Broadcasting message to all clients:" << message;
            m_server->broadcastToAll(message);
            
            if (m_db) {
                m_db->saveMessage("Server", "All", message, utcTime);
            }
            
            QString formattedMessage = QString("[%1] You (to all): %2").arg(timeString, message);
            m_serverView->showMessage(formattedMessage);
        } else {
            qDebug() << "Sending private message to" << m_currentPrivateTargetUser << ":" << message;
            m_server->sendMessageToClient(m_currentPrivateTargetUser, message);
            
            QString targetUser = m_currentPrivateTargetUser.split(" - ").first().trimmed();
            if (m_db) {
                m_db->saveMessage("Server", targetUser, message, utcTime);
            }
            
            QString formattedMessage = QString("[%1] You (to %2): %3").arg(timeString, targetUser, message);
            m_serverView->showMessage(formattedMessage);
        }
    }
}

// --- FIX: Update signature to include serverHost ---
void ChatController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &filePath, const QString &sender, const QString &serverHost)
{
    if(fileName.isEmpty() || filePath.isEmpty()) return;

    QDateTime utcTime = QDateTime::currentDateTimeUtc();
    QString timeString = utcTime.toString("HH:mm:");

    // --- FIX: Construct the full, public URL ---
    QString publicHost = serverHost;
    if (publicHost.isEmpty()) {
        // If server is uploading, get its actual IP address
        if (m_server) {
            publicHost = m_server->getServerIpAddress();
            qDebug() << "Server detected, using IP:" << publicHost;
        } else {
            publicHost = "localhost";
        }
    }
    QString fullPublicUrl = QString("http://%1:1080%2").arg(publicHost, filePath);
    // --- END FIX ---

    // Create a special message format for files: "FILE|<filename>|<filesize>|<full_public_url>"
    QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(fullPublicUrl);

    qDebug() << "\n=== DEBUG ChatController::displayFileMessage ===";
    qDebug() << "Server Host:" << publicHost;
    qDebug() << "Full Public URL:" << fullPublicUrl;
    qDebug() << "File message being sent:" << fileMessage;

    if (m_client) {
        m_client->sendMessage(fileMessage);

        if (m_db) {
            m_db->saveMessage(m_clientUserId, "Server", fileMessage, utcTime);
        }

        QString formattedMessage = QString("[%1] You:").arg(timeString);
        // --- FIX: Pass full public URL and Sent type ---
        m_clientView->showFileMessage(fileName, fileSize, fullPublicUrl, formattedMessage, BaseChatWindow::MessageType::Sent);
        // --- END FIX ---
        
    } else if (m_server) {
        if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
            m_server->broadcastToAll(fileMessage);
            if (m_db) m_db->saveMessage("Server", "All", fileMessage, utcTime);
            
            QString formattedMessage = QString("[%1] You (to all):").arg(timeString);
            // --- FIX: Pass full public URL and Sent type ---
            m_serverView->showFileMessage(fileName, fileSize, fullPublicUrl, formattedMessage, BaseChatWindow::MessageType::Sent);
            // --- END FIX ---
            
        } else {
            m_server->sendMessageToClient(m_currentPrivateTargetUser, fileMessage);
            QString targetUser = m_currentPrivateTargetUser.split(" - ").first().trimmed();
            if (m_db) m_db->saveMessage("Server", targetUser, fileMessage, utcTime);
            
            QString formattedMessage = QString("[%1] You (to %2):").arg(timeString, targetUser);
            // --- FIX: Pass full public URL and Sent type ---
            m_serverView->showFileMessage(fileName, fileSize, fullPublicUrl, formattedMessage, BaseChatWindow::MessageType::Sent);
            // --- END FIX ---
        }
    }
    qDebug() << "=== END DEBUG ChatController::displayFileMessage ===\n";
}

void ChatController::onMessageReceived(const QString &message)
{
    if(message.isEmpty()) return;
    
    qDebug() << "\n=== ChatController::onMessageReceived (Client) ===";
    qDebug() << "Message:" << message;
    
    // Check if this is a file message
    if (message.startsWith("FILE|")) {
        QStringList parts = message.split("|");
        if (parts.size() >= 4) {
            QString fileName = parts[1];
            qint64 fileSize = parts[2].toLongLong();
            // --- FIX: Re-join URL parts in case it contained a | ---
            QString fileUrl = parts[3];
            for(int i = 4; i < parts.size(); ++i) fileUrl += "|" + parts[i];
            // --- END FIX ---

            QDateTime utcTime = QDateTime::currentDateTimeUtc();
            QString timeString = utcTime.toString("HH:mm:");

            if (m_db) {
                m_db->saveMessage("Server", m_clientUserId, message, utcTime);
            }

            QString formattedMessage = QString("[%1] Server:").arg(timeString);
            if (m_clientView) {
                // --- FIX: Pass Received type ---
                m_clientView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage, BaseChatWindow::MessageType::Received);
                // --- END FIX ---
            }
        }
    } else {
        // Regular text message
        QDateTime utcTime = QDateTime::currentDateTimeUtc();
        QString timeString = utcTime.toString("HH:mm:");
        
        if (m_db) {
            m_db->saveMessage("Server", m_clientUserId, message, utcTime);
        }
        
        if (m_clientView) {
            QString formattedMessage = QString("[%1] Server: %2").arg(timeString, message);
            m_clientView->showMessage(formattedMessage);
        }
    }
}

void ChatController::onUserListChanged(const QStringList &users)
{
    if (m_serverView) {
        m_serverView->updateUserList(users);
    }
}

void ChatController::onUserCountChanged(int count)
{
    if (m_serverView) {
        m_serverView->updateUserCount(count);
    }
}

void ChatController::onUserSelected(const QString &userId)
{
    if (userId == "همه کاربران") {
        qDebug() << "Switched to broadcast mode";
        m_currentPrivateTargetUser = "";
        m_isBroadcastMode = true;
        m_currentFilteredUser = "";
        if (m_serverView) {
            m_serverView->setBroadcastMode();
            loadHistoryForServer();
        }
    } else {
        qDebug() << "User selected for private chat:" << userId;
        m_currentPrivateTargetUser = userId;
        m_isBroadcastMode = false;
        m_currentFilteredUser = userId;
        
        if (m_serverView) {
            loadHistoryForUser(userId);
        }
    }
}

void ChatController::onServerMessageReceived(const QString &message, const QString &senderId)
{
    if(message.isEmpty()) return;
    
    // Check if this is a file message
    if (message.startsWith("FILE|")) {
        QStringList parts = message.split("|");
        if (parts.size() >= 4) {
            QString fileName = parts[1];
            qint64 fileSize = parts[2].toLongLong();
            // --- FIX: Re-join URL parts in case it contained a | ---
            QString fileUrl = parts[3];
            for(int i = 4; i < parts.size(); ++i) fileUrl += "|" + parts[i];
            // --- END FIX ---

            QDateTime utcTime = QDateTime::currentDateTimeUtc();
            QString timeString = utcTime.toString("HH:mm:");
            
            QString senderName = senderId.split(" - ").first();
            
            if (m_db) {
                m_db->saveMessage(senderName, "Server", message, utcTime);
            }
            
            QString formattedMessage = QString("[%1] %2:").arg(timeString, senderName);
            
            if (m_serverView) {
                QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
                if (m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderName == cleanFilteredUser) {
                    // --- FIX: Pass Received type ---
                    m_serverView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage, BaseChatWindow::MessageType::Received);
                    // --- END FIX ---
                }
            }
        }
    } else {
        // Regular text message
        QDateTime utcTime = QDateTime::currentDateTimeUtc();
        QString timeString = utcTime.toString("HH:mm:");
        
        QString senderName = senderId.split(" - ").first();
        
        if (m_db) {
            m_db->saveMessage(senderName, "Server", message, utcTime);
        }
        
        QString formattedMessage = QString("[%1] %2: %3").arg(timeString, senderName, message);
        
        if (m_serverView) {
            QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
            if (m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderName == cleanFilteredUser) {
                m_serverView->showMessage(formattedMessage);
            }
        }
    }
}

void ChatController::loadHistoryForClient()
{
    if (!m_db || !m_clientView) return;
    
    qDebug() << "Loading message history for client...";
    QStringList history = m_db->loadMessagesBetween(m_clientUserId, "Server");
    
    for (const QString &msg : history) {
        m_clientView->showMessage(msg);
    }
    
    qDebug() << "Loaded" << history.size() << "historical messages for client";
}

void ChatController::loadHistoryForServer()
{
    if (!m_db || !m_serverView) return;
    
    qDebug() << "Loading message history for server...";
    
    m_serverView->clearChatHistory();
    QStringList history = m_db->loadAllMessages();
    
    for (const QString &msg : history) {
        m_serverView->showMessage(msg);
    }
    
    qDebug() << "Loaded" << history.size() << "historical messages for server";
}

void ChatController::loadHistoryForUser(const QString &userId)
{
    if (!m_db || !m_serverView) return;
    
    qDebug() << "Loading message history for user:" << userId;
    
    m_serverView->clearChatHistory();
    QString cleanUserId = userId.split(" - ").first().trimmed();
    qDebug() << "Clean user ID:" << cleanUserId;
    
    QStringList history = m_db->loadMessagesBetween("Server", cleanUserId);
    qDebug() << "Loaded" << history.size() << "messages for user:" << cleanUserId;
    
    for (const QString &msg : history) {
        qDebug() << "Displaying message:" << msg;
        m_serverView->showMessage(msg);
    }
}

// --- FIX: Update signature to include serverHost ---
void ChatController::onFileUploaded(const QString &fileName, const QString &urlPath, qint64 fileSize, const QString &serverHost)
{
    qDebug() << "\n=== DEBUG ChatController::onFileUploaded ===";
    qDebug() << "Controller type - m_client:" << (m_client ? "SET" : "NULL") << "m_server:" << (m_server ? "SET" : "NULL");
    qDebug() << "File uploaded:" << fileName << "URL Path:" << urlPath << "Size:" << fileSize << "Host:" << serverHost;
    
    // Pass all info to displayFileMessage
    displayFileMessage(fileName, fileSize, urlPath, "", serverHost);
    
    qDebug() << "=== END DEBUG ChatController::onFileUploaded ===\n";
}