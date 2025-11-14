#include "ClientController.h"
#include "View/ClientChatWindow.h"
#include "Model/Network/WebSocketClient.h"
#include "Model/Core/DatabaseManager.h"
#include "TextMessageItem.h"
#include "FileMessageItem.h"
#include "MessageData.h"

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

QWidget* ClientController::createWidgetFromData(const MessageData &msgData)
{
    if (msgData.isFileMessage) {
        return new FileMessageItem(
            msgData.fileInfo.fileName,
            msgData.fileInfo.fileSize,
            msgData.fileInfo.fileUrl,
            msgData.senderName,
            (msgData.senderType == MessageData::User_Me) ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received,
            m_serverHost
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

void ClientController::displayNewConnection()
{
    qDebug() << "ClientController: Connection established";
}

void ClientController::displayNewMessages(const QString &message)
{
    qDebug() << "🔥🔥🔥 [ClientController::displayNewMessages] CALLED!";
    qDebug() << "🔥 [ClientController] Message received:" << message;
    
    if (message.isEmpty()) {
        qDebug() << "⚠️ [ClientController] Message is empty, aborting";
        return;
    }

    // 1. Create MessageData for sent message
    MessageData msgData;
    msgData.text = message;
    msgData.senderName = "You";
    msgData.senderType = MessageData::User_Me;
    msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    msgData.isFileMessage = false;
    
    // 2. Send via WebSocket (convert to JSON)
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
    
    // 3. Save to database
    if (m_db) {
        qDebug() << "✅ [ClientController] Saving message to database...";
        m_db->saveMessage(m_clientUserId, "Server", message, QDateTime::currentDateTime());
    } else {
        qDebug() << "⚠️ [ClientController] Database is NULL!";
    }
    
    // 4. Create widget from data
    QWidget* itemWidget = createWidgetFromData(msgData);
    
    // 5. Show in client window
    if (m_clientView) {
        qDebug() << "✅ [ClientController] Adding message to UI...";
        m_clientView->addMessageItem(itemWidget);
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
    
    // 1. Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON message";
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // 2. Create MessageData object (Model)
    MessageData msgData;
    msgData.senderType = MessageData::User_Other; // Always from other side
    msgData.senderName = obj["sender"].toString();
    if (msgData.senderName.isEmpty()) {
        msgData.senderName = "Server";
    }
    
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
        m_db->saveMessage("Server", m_clientUserId, dbMessage, QDateTime::currentDateTime());
    }
    
    // 4. Create widget (View)
    QWidget* itemWidget = createWidgetFromData(msgData);
    
    // 5. Deliver prepared widget to View
    if (m_clientView) {
        m_clientView->addMessageItem(itemWidget);
    }
}

void ClientController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost)
{
    qDebug() << "ClientController: File uploaded:" << fileName << url << fileSize << serverHost;
    
    // Store server host for future downloads
    if (!serverHost.isEmpty()) {
        m_serverHost = serverHost;
    }
    
    // 1. Create MessageData for sent file
    MessageData msgData;
    msgData.senderName = "You";
    msgData.senderType = MessageData::User_Me;
    msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    msgData.isFileMessage = true;
    msgData.fileInfo.fileName = fileName;
    msgData.fileInfo.fileSize = fileSize;
    msgData.fileInfo.fileUrl = url;
    
    // 2. Create file message JSON and send via WebSocket
    QString jsonMessage = QString(R"({"type":"file","fileName":"%1","fileSize":%2,"fileUrl":"%3","sender":"Client","timestamp":"%4"})")
                              .arg(fileName)
                              .arg(fileSize)
                              .arg(url)
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    
    if (m_client) {
        m_client->sendMessage(jsonMessage);
    }
    
    // 3. Save to database in FILE|name|size|url format
    if (m_db) {
        QString fileMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(url);
        m_db->saveMessage(m_clientUserId, "Server", fileMessage, QDateTime::currentDateTime());
    }
    
    // **FIX: دیگر نیازی به ساخت FileMessageItem نیست - قبلاً در View ساخته شده**
    // FileMessageItem already created in ClientChatWindow::onSendFileClicked()
    // Just update state to Completed_Sent (already done in View)
}

void ClientController::loadHistory()
{
    if (!m_db || !m_clientView) return;
    
    qDebug() << "ClientController: Loading message history";
    
    QStringList history = m_db->loadMessagesBetween(m_clientUserId, "Server");
    
    for (const QString &dbString : history) {
        // dbString format: "hh:mm|Sender|Message"
        QStringList parts = dbString.split("|");
        if (parts.size() < 3) continue;
        
        MessageData msgData;
        msgData.timestamp = parts[0];
        msgData.senderName = parts[1];
        QString messageContent = parts.mid(2).join("|"); // Rejoin in case message contains |
        
        // Determine sender type
        if (msgData.senderName == "Client" || msgData.senderName == "You") {
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
        m_clientView->addMessageItem(itemWidget);
    }
}
