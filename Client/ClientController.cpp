#include "ClientController.h"
#include "UI/ClientChatWindow.h"
#include "Network/WebSocketClient.h"
#include "Core/DatabaseManager.h"
#include "TextMessageItem.h"
#include "FileMessageItem.h"

#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

ClientController::ClientController(ClientChatWindow *view, WebSocketClient *client, DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_clientView(view)
    , m_client(client)
    , m_db(db)
    , m_clientUserId("Client")
{
    qDebug() << "🔥 [ClientController] Constructor called";
    
    if (m_clientView) {
        qDebug() << "🔥 [ClientController] Connecting signals from ClientChatWindow...";
        
        // FIX: Use sendMessageRequested instead of messageSent
        connect(m_clientView, &ClientChatWindow::sendMessageRequested, this, &ClientController::displayNewMessages);
        qDebug() << "✅ [ClientController] Connected sendMessageRequested signal";
        
        connect(m_clientView, &ClientChatWindow::fileUploaded, this, &ClientController::onFileUploaded);
        qDebug() << "✅ [ClientController] Connected fileUploaded signal";
    } else {
        qDebug() << "⚠️ [ClientController] m_clientView is NULL!";
    }

    if (m_client) {
        qDebug() << "🔥 [ClientController] Connecting signals from WebSocketClient...";
        connect(m_client, &WebSocketClient::messageReceived, this, &ClientController::onMessageReceived);
        qDebug() << "✅ [ClientController] Connected messageReceived signal";
    } else {
        qDebug() << "⚠️ [ClientController] m_client is NULL!";
    }

    loadHistory();
}

ClientController::~ClientController()
{
}

void ClientController::displayNewConnection()
{
    qDebug() << "ClientController: Connection established";
}

void ClientController::displayNewMessages(const QString &message)
{
    qDebug() << "🔥🔥🔥 [ClientController::displayNewMessages] CALLED!";
    qDebug() << "🔥 [ClientController] Message received:" << message;
    qDebug() << "🔥 [ClientController] Message length:" << message.length();
    
    if (message.isEmpty()) {
        qDebug() << "⚠️ [ClientController] Message is empty, aborting";
        return;
    }

    qDebug() << "✅ [ClientController] Sending message via WebSocket...";

    // Send via WebSocket
    if (m_client) {
        QString jsonMessage = QString(R"({"type":"text","content":"%1","sender":"Client","timestamp":"%2"})")
                                  .arg(message)
                                  .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
        qDebug() << "✅ [ClientController] JSON message:" << jsonMessage;
        m_client->sendMessage(jsonMessage);
        qDebug() << "✅ [ClientController] Message sent to WebSocket";
    } else {
        qDebug() << "❌ [ClientController] WebSocket client is NULL!";
    }
    
    // Save to database
    if (m_db) {
        qDebug() << "✅ [ClientController] Saving message to database...";
        m_db->saveMessage(m_clientUserId, "Server", message, QDateTime::currentDateTime());
    } else {
        qDebug() << "⚠️ [ClientController] Database is NULL!";
    }
    
    // Show in client window
    if (m_clientView) {
        qDebug() << "✅ [ClientController] Adding message to UI...";
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
        TextMessageItem *msgItem = new TextMessageItem(
            message,
            "You",
            BaseChatWindow::MessageType::Sent,
            timestamp
        );
        m_clientView->addMessageItem(msgItem);
        qDebug() << "✅ [ClientController] Message added to UI";
    } else {
        qDebug() << "❌ [ClientController] ClientView is NULL!";
    }
    
    qDebug() << "🎉 [ClientController] displayNewMessages completed!";
}

void ClientController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender, const QString &serverHost)
{
    qDebug() << "ClientController: Displaying file message:" << fileName << fileSize << fileUrl << sender;
    
    QString senderInfo = sender.isEmpty() ? "You" : sender;
    auto type = sender.isEmpty() ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received;
    
    if (m_clientView) {
        FileMessageItem *fileItem = new FileMessageItem(
            fileName,
            fileSize,
            fileUrl,
            senderInfo,
            type,
            serverHost
        );
        m_clientView->addMessageItem(fileItem);
    }
}

void ClientController::onMessageReceived(const QString &message)
{
    qDebug() << "ClientController: Message received:" << message;
    
    // Parse JSON message
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON message";
        return;
    }
    
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QString content = obj["content"].toString();
    QString sender = obj["sender"].toString();
    
    if (type == "text") {
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", m_clientUserId, content, QDateTime::currentDateTime());
        }
        
        // Extract timestamp from JSON or use current time
        QString timestamp = obj["timestamp"].toString();
        if (timestamp.isEmpty()) {
            timestamp = QDateTime::currentDateTime().toString("hh:mm");
        } else {
            // Parse ISO format timestamp and convert to hh:mm
            QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
            timestamp = dt.toString("hh:mm");
        }
        
        // Display in client window
        if (m_clientView) {
            TextMessageItem *msgItem = new TextMessageItem(
                content,
                sender.isEmpty() ? "Server" : sender,
                BaseChatWindow::MessageType::Received,
                timestamp
            );
            m_clientView->addMessageItem(msgItem);
        }
    } else if (type == "file") {
        QString fileName = obj["fileName"].toString();
        qint64 fileSize = obj["fileSize"].toInteger();
        QString fileUrl = obj["fileUrl"].toString();
        
        // Save file message to database in FILE|name|size|url format
        if (m_db) {
            QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(fileUrl);
            m_db->saveMessage("Server", m_clientUserId, fileMessage, QDateTime::currentDateTime());
        }
        
        // Display in client window
        if (m_clientView) {
            // Extract server host from URL if possible
            QString serverHost = "localhost";
            if (fileUrl.contains("://")) {
                QUrl url(fileUrl);
                serverHost = url.host();
            }
            
            FileMessageItem *fileItem = new FileMessageItem(
                fileName,
                fileSize,
                fileUrl,
                sender.isEmpty() ? "Server" : sender,
                BaseChatWindow::MessageType::Received,
                serverHost
            );
            m_clientView->addMessageItem(fileItem);
        }
    }
}

void ClientController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost)
{
    qDebug() << "ClientController: File uploaded:" << fileName << url << fileSize << serverHost;
    
    // Create file message JSON
    QString jsonMessage = QString(R"({"type":"file","fileName":"%1","fileSize":%2,"fileUrl":"%3","sender":"Client","timestamp":"%4"})")
                              .arg(fileName)
                              .arg(fileSize)
                              .arg(url)
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    
    // Send via WebSocket
    if (m_client) {
        m_client->sendMessage(jsonMessage);
    }
    
    // Save to database in FILE|name|size|url format
    if (m_db) {
        QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(url);
        m_db->saveMessage(m_clientUserId, "Server", fileMessage, QDateTime::currentDateTime());
    }
    
    // Display in client window
    displayFileMessage(fileName, fileSize, url, "", serverHost);
}

void ClientController::loadHistory()
{
    if (!m_db || !m_clientView) return;
    
    qDebug() << "ClientController: Loading message history";
    
    QStringList history = m_db->loadMessagesBetween(m_clientUserId, "Server");
    
    for (const QString &msg : history) {
        m_clientView->showMessage(msg);
    }
}
