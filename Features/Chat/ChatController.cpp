#include "ChatController.h"
#include "ChatWindow.h"
#include "ServerChatWindow.h"
#include "../../Core/WebSocketClient.h"
#include "../../Core/WebSocketServer.h"
#include "../../Core/DatabaseManager.h"

#include <QDebug>
#include <QDateTime>

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

void ChatController::initClient(ChatWindow *view, WebSocketClient *client, DatabaseManager *db)
{
    m_clientView = view;
    m_client = client;
    m_db = db;
    m_clientUserId = "Client";
    
    // Load previous messages
    if (m_db) {
        loadHistoryForClient();
    }
    
    // Connect view's send button to our handler
    connect(m_clientView, &ChatWindow::sendMessageRequested,
            this, &ChatController::displayNewMessages);
    
    // Connect file upload signal
    connect(m_clientView, &ChatWindow::fileUploaded,
            this, &ChatController::onFileUploaded);
    
    // Connect to receive messages from server
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
    
    // Load previous messages
    if (m_db) {
        loadHistoryForServer();
    }
    
    // Connect view's send button to our handler
    connect(m_serverView, &ServerChatWindow::sendMessageRequested,
            this, &ChatController::displayNewMessages);
    
    // Connect file upload signal
    connect(m_serverView, &ServerChatWindow::fileUploaded,
            this, &ChatController::onFileUploaded);
    
    // Connect user selection
    connect(m_serverView, &ServerChatWindow::userSelected,
            this, &ChatController::onUserSelected);
    
    // Connect to receive messages from clients
    if (m_server) {
        connect(m_server, &WebSocketServer::messageReceived,
                this, &ChatController::onServerMessageReceived);
        
        // Connect user list updates
        connect(m_server, &WebSocketServer::userListChanged,
                this, &ChatController::onUserListChanged);
        connect(m_server, &WebSocketServer::userCountChanged,
                this, &ChatController::onUserCountChanged);
        
        // Initialize user list
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
    
    // Send message through WebSocket (client or server)
    if (m_client) {
        qDebug() << "Sending message via client:" << message;
        m_client->sendMessage(message);
        
        // Save to database
        if (m_db) {
            m_db->saveMessage(m_clientUserId, "Server", message, utcTime);
        }
        
        // For clients: display locally as "You"
        QString formattedMessage = QString("[%1] You: %2").arg(timeString, message);
        m_clientView->showMessage(formattedMessage);
    } else if (m_server) {
        // For server: check if in private chat mode or broadcast mode
        if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
            qDebug() << "Broadcasting message to all clients:" << message;
            m_server->broadcastToAll(message);
            
            // Save to database
            if (m_db) {
                m_db->saveMessage("Server", "All", message, utcTime);
            }
            
            QString formattedMessage = QString("[%1] You (to all): %2").arg(timeString, message);
            m_serverView->showMessage(formattedMessage);
        } else {
            qDebug() << "Sending private message to" << m_currentPrivateTargetUser << ":" << message;
            m_server->sendMessageToClient(m_currentPrivateTargetUser, message);
            
            // Save to database
            QString targetUser = m_currentPrivateTargetUser.split(" - ").first().trimmed();
            if (m_db) {
                m_db->saveMessage("Server", targetUser, message, utcTime);
            }
            
            QString formattedMessage = QString("[%1] You (to %2): %3").arg(timeString, targetUser, message);
            m_serverView->showMessage(formattedMessage);
        }
    }
}

void ChatController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender)
{
    if(fileName.isEmpty() || fileUrl.isEmpty()) return;

    QDateTime utcTime = QDateTime::currentDateTimeUtc();
    QString timeString = utcTime.toString("HH:mm:");

    // Create a special message format for files: "FILE|<filename>|<filesize>|<fileurl>"
    QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(fileUrl);

    // DEBUG: Show controller state
    qDebug() << "\n=== DEBUG ChatController::displayFileMessage ===";
    qDebug() << "Controller type - m_client:" << (m_client ? "SET" : "NULL") << "m_server:" << (m_server ? "SET" : "NULL");
    qDebug() << "m_clientView:" << (m_clientView ? "SET" : "NULL") << "m_serverView:" << (m_serverView ? "SET" : "NULL");
    qDebug() << "File message:" << fileMessage;
    qDebug() << "Sender param:" << sender;

    // Send message through WebSocket (client or server)
    if (m_client) {
        qDebug() << "DEBUG: Taking CLIENT branch - sending via m_client";
        m_client->sendMessage(fileMessage);

        // Save to database
        if (m_db) {
            m_db->saveMessage(m_clientUserId, "Server", fileMessage, utcTime);
        }

        // For clients: display locally as "You"
        QString formattedMessage = QString("[%1] You:").arg(timeString);
        m_clientView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage);
    } else if (m_server) {
        qDebug() << "DEBUG: Taking SERVER branch - broadcasting via m_server";
        qDebug() << "DEBUG: m_isBroadcastMode:" << m_isBroadcastMode << "m_currentPrivateTargetUser:" << m_currentPrivateTargetUser;

        // For server: check if in private chat mode or broadcast mode
        if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
            qDebug() << "DEBUG: Broadcasting to all clients";
            m_server->broadcastToAll(fileMessage);

            // Save to database
            if (m_db) {
                m_db->saveMessage("Server", "All", fileMessage, utcTime);
            }

            QString formattedMessage = QString("[%1] You (to all):").arg(timeString);
            m_serverView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage);
        } else {
            qDebug() << "DEBUG: Sending to private target:" << m_currentPrivateTargetUser;
            m_server->sendMessageToClient(m_currentPrivateTargetUser, fileMessage);

            // Save to database
            QString targetUser = m_currentPrivateTargetUser.split(" - ").first().trimmed();
            if (m_db) {
                m_db->saveMessage("Server", targetUser, fileMessage, utcTime);
            }

            QString formattedMessage = QString("[%1] You (to %2):").arg(timeString, targetUser);
            m_serverView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage);
        }
    } else {
        qDebug() << "DEBUG: ERROR - Neither m_client nor m_server is set!";
    }
    qDebug() << "=== END DEBUG ChatController::displayFileMessage ===\n";
}

void ChatController::onMessageReceived(const QString &message)
{
    if(message.isEmpty()) return;
    
    qDebug() << "\n=== ChatController::onMessageReceived ===";
    qDebug() << "Message:" << message;
    qDebug() << "m_clientView:" << m_clientView;
    
    // Check if this is a file message
    if (message.startsWith("FILE|")) {
        QStringList parts = message.split("|");
        if (parts.size() >= 4) {
            QString fileName = parts[1];
            qint64 fileSize = parts[2].toLongLong();
            QString fileUrl = parts[3];

            QDateTime utcTime = QDateTime::currentDateTimeUtc();
            QString timeString = utcTime.toString("HH:mm:");

            // Save to database
            if (m_db) {
                m_db->saveMessage("Server", m_clientUserId, message, utcTime);
            }

            // Display file message
            QString formattedMessage = QString("[%1] Server:").arg(timeString);
            if (m_clientView) {
                m_clientView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage);
            }
        }
    } else {
        // Regular text message
        // Display received message
        QDateTime utcTime = QDateTime::currentDateTimeUtc();
        QString timeString = utcTime.toString("HH:mm:");
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", m_clientUserId, message, utcTime);
        }
        
        // For client: messages from server show as "Server"
        if (m_clientView) {
            QString formattedMessage = QString("[%1] Server: %2").arg(timeString, message);
            qDebug() << "Displaying on client view:" << formattedMessage;
            m_clientView->showMessage(formattedMessage);
        } else {
            qDebug() << "✗ No client view available!";
        }
    }
    qDebug() << "=== END ChatController::onMessageReceived ===\n";
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
        // Special case: switch to broadcast mode
        qDebug() << "Switched to broadcast mode";
        m_currentPrivateTargetUser = "";
        m_isBroadcastMode = true;
        m_currentFilteredUser = "";
        if (m_serverView) {
            m_serverView->setBroadcastMode();
            // Reload all messages
            loadHistoryForServer();
        }
    } else {
        // Private chat with specific user
        qDebug() << "User selected for private chat:" << userId;
        m_currentPrivateTargetUser = userId;
        m_isBroadcastMode = false;
        m_currentFilteredUser = userId;
        
        // Load only messages for this user
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
            QString fileUrl = parts[3];

            // Display received message from client on server
            QDateTime utcTime = QDateTime::currentDateTimeUtc();
            QString timeString = utcTime.toString("HH:mm:");
            
            QString senderName = senderId.split(" - ").first();
            
            // Save to database
            if (m_db) {
                m_db->saveMessage(senderName, "Server", message, utcTime);
            }
            
            QString formattedMessage = QString("[%1] %2:").arg(timeString, senderName);
            
            // Only display if in broadcast mode OR if the message is from the currently filtered user
            if (m_serverView) {
                QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
                if (m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderName == cleanFilteredUser) {
                    m_serverView->showFileMessage(fileName, fileSize, fileUrl, formattedMessage);
                }
            }
        }
    } else {
        // Regular text message
        // Display received message from client on server
        QDateTime utcTime = QDateTime::currentDateTimeUtc();
        QString timeString = utcTime.toString("HH:mm:");
        
        QString senderName = senderId.split(" - ").first();
        
        // Save to database
        if (m_db) {
            m_db->saveMessage(senderName, "Server", message, utcTime);
        }
        
        QString formattedMessage = QString("[%1] %2: %3").arg(timeString, senderName, message);
        
        // Only display if in broadcast mode OR if the message is from the currently filtered user
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
        // Check if this is a file message
        if (msg.startsWith("FILE|")) {
            QStringList parts = msg.split("|");
            if (parts.size() >= 4) {
                QString fileName = parts[1];
                qint64 fileSize = parts[2].toLongLong();
                QString fileUrl = parts[3];
                
                // For historical messages, we don't have sender info, so use a generic one
                QString senderInfo = "[Historical]";
                m_clientView->showFileMessage(fileName, fileSize, fileUrl, senderInfo);
            } else {
                // Fallback to regular message display
                m_clientView->showMessage(msg);
            }
        } else {
            m_clientView->showMessage(msg);
        }
    }
    
    qDebug() << "Loaded" << history.size() << "historical messages for client";
}

void ChatController::loadHistoryForServer()
{
    if (!m_db || !m_serverView) return;
    
    qDebug() << "Loading message history for server...";
    
    // Clear current chat history
    m_serverView->clearChatHistory();
    
    QStringList history = m_db->loadAllMessages();
    
    for (const QString &msg : history) {
        // Check if this is a file message
        if (msg.startsWith("FILE|")) {
            QStringList parts = msg.split("|");
            if (parts.size() >= 4) {
                QString fileName = parts[1];
                qint64 fileSize = parts[2].toLongLong();
                QString fileUrl = parts[3];
                
                // For historical messages, we don't have sender info, so use a generic one
                QString senderInfo = "[Historical]";
                m_serverView->showFileMessage(fileName, fileSize, fileUrl, senderInfo);
            } else {
                // Fallback to regular message display
                m_serverView->showMessage(msg);
            }
        } else {
            m_serverView->showMessage(msg);
        }
    }
    
    qDebug() << "Loaded" << history.size() << "historical messages for server";
}

void ChatController::loadHistoryForUser(const QString &userId)
{
    if (!m_db || !m_serverView) return;
    
    qDebug() << "Loading message history for user:" << userId;
    
    // Clear current chat history
    m_serverView->clearChatHistory();
    
    // Extract just the user name (remove IP suffix like " - ::ffff:127.0.0.1")
    QString cleanUserId = userId.split(" - ").first().trimmed();
    
    qDebug() << "Clean user ID:" << cleanUserId;
    
    // Load messages between server and this user
    QStringList history = m_db->loadMessagesBetween("Server", cleanUserId);
    
    qDebug() << "Loaded" << history.size() << "messages for user:" << cleanUserId;
    
    for (const QString &msg : history) {
        // Check if this is a file message
        if (msg.startsWith("FILE|")) {
            QStringList parts = msg.split("|");
            if (parts.size() >= 4) {
                QString fileName = parts[1];
                qint64 fileSize = parts[2].toLongLong();
                QString fileUrl = parts[3];
                
                // For historical messages, we don't have sender info, so use a generic one
                QString senderInfo = "[Historical]";
                m_serverView->showFileMessage(fileName, fileSize, fileUrl, senderInfo);
            } else {
                // Fallback to regular message display
                m_serverView->showMessage(msg);
            }
        } else {
            qDebug() << "Displaying message:" << msg;
            m_serverView->showMessage(msg);
        }
    }
}

void ChatController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize)
{
    qDebug() << "\n=== DEBUG ChatController::onFileUploaded ===";
    qDebug() << "Controller type - m_client:" << (m_client ? "SET" : "NULL") << "m_server:" << (m_server ? "SET" : "NULL");
    qDebug() << "File uploaded:" << fileName << "URL:" << url << "Size:" << fileSize;
    qDebug() << "Calling displayFileMessage...";
    displayFileMessage(fileName, fileSize, url);
    qDebug() << "=== END DEBUG ChatController::onFileUploaded ===\n";
}

