#include "ServerController.h"
#include "View/ServerChatWindow.h"
#include "Model/Network/WebSocketServer.h"
#include "Model/Core/DatabaseManager.h"
#include "MessageAliases.h"
#include "MessageComponent.h"
#include "MessageData.h"

#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QCheckBox>
#include <QRegularExpression>

namespace {
constexpr int kMaxMessageLength = 1800;

// Helper function to convert between MessageType and MessageDirection
inline MessageDirection messageTypeToDirection(BaseChatWindow::MessageType type) {
    return (type == BaseChatWindow::MessageType::Sent) 
        ? MessageDirection::Outgoing 
        : MessageDirection::Incoming;
}
}

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
        connect(m_serverView, &ServerChatWindow::messageEditConfirmed,
                this, &ServerController::onTextMessageEditConfirmed);
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

    auto messageType = (msgData.senderType == MessageData::User_Me)
                           ? BaseChatWindow::MessageType::Sent
                           : BaseChatWindow::MessageType::Received;
    auto direction = messageTypeToDirection(messageType);
    
    if (msgData.isVoiceMessage) {
        QString fileName = msgData.fileInfo.fileName;
        if (fileName.isEmpty()) {
            const QUrl url(msgData.voiceInfo.url);
            fileName = url.fileName().isEmpty() ? QStringLiteral("voice_message_%1.m4a").arg(QDateTime::currentMSecsSinceEpoch()) : url.fileName();
        }
        auto *voiceItem = new VoiceMessageItem(
            fileName,
            msgData.fileInfo.fileSize,
            msgData.voiceInfo.url,
            msgData.voiceInfo.duration,
            msgData.senderName,
            direction,
            msgData.timestamp,
            serverHost,
            msgData.voiceInfo.waveform  // Pass waveform data!
        );
        voiceItem->setDatabaseId(msgData.databaseId);
        setupVoiceMessageItem(voiceItem);
        return voiceItem;
    } else if (msgData.isFileMessage) {
        auto *fileItem = new FileMessageItem(
            msgData.fileInfo.fileName,
            msgData.fileInfo.fileSize,
            msgData.fileInfo.fileUrl,
            msgData.senderName,
            direction,
            serverHost,
            msgData.timestamp
        );
        fileItem->setDatabaseId(msgData.databaseId);
        setupFileMessageItem(fileItem);
        return fileItem;
    }

    TextMessageItem *textItem = new TextMessageItem(
        msgData.text,
        msgData.senderName,
        direction,
        msgData.timestamp
    );
    setupTextMessageItem(textItem);
    textItem->setDatabaseId(msgData.databaseId);
    if (msgData.isEdited) {
        textItem->markAsEdited(true);
    }
    return textItem;
}

void ServerController::setupTextMessageItem(TextMessageItem *item)
{
    if (!item) {
        return;
    }

    connect(item, &TextMessageItem::copyRequested,
            this, &ServerController::onTextMessageCopyRequested,
            Qt::UniqueConnection);
    connect(item, &TextMessageItem::editRequested,
            this, &ServerController::onTextMessageEditRequested,
            Qt::UniqueConnection);
    connect(item, &TextMessageItem::deleteRequested,
            this, &ServerController::onTextMessageDeleteRequested,
            Qt::UniqueConnection);
}

void ServerController::setupFileMessageItem(FileMessageItem *item)
{
    if (!item) {
        return;
    }

    connect(item, &FileMessageItem::deleteRequested,
            this, &ServerController::onFileMessageDeleteRequested,
            Qt::UniqueConnection);
}

void ServerController::setupVoiceMessageItem(VoiceMessageItem *item)
{
    if (!item) {
        return;
    }

    connect(item, &VoiceMessageItem::deleteRequested,
            this, &ServerController::onVoiceMessageDeleteRequested,
            Qt::UniqueConnection);
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

    QStringList chunks = splitMessageIntoChunks(message);
    QString isoTimestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    for (const QString &chunk : chunks) {
        MessageData chunkData = msgData;
        chunkData.text = chunk;
        chunkData.isEdited = false;
        chunkData.databaseId = -1;
        
        QString jsonMessage = QString(R"({"type":"text","content":"%1","sender":"Server","timestamp":"%2"})")
                                  .arg(chunk)
                                  .arg(isoTimestamp);
    
        if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
            if (m_server) {
                m_server->broadcastToAll(jsonMessage);
            }
            
            if (m_db) {
                chunkData.databaseId = m_db->saveMessage("Server", "ALL", chunk, QDateTime::currentDateTime(), false);
            }
    
        } else {
            QString cleanUserId = m_currentPrivateTargetUser.split(" - ").first().trimmed();
            
            if (m_server) {
                m_server->sendMessageToClient(cleanUserId, jsonMessage);
            }
            
            if (m_db) {
                chunkData.databaseId = m_db->saveMessage("Server", cleanUserId, chunk, QDateTime::currentDateTime(), false);
            }
        }
        
        QWidget* itemWidget = createWidgetFromData(chunkData);
        if (m_serverView) {
            m_serverView->addMessageItem(itemWidget);
            
            TextMessageItem* textItem = qobject_cast<TextMessageItem*>(itemWidget);
            if (textItem) {
                textItem->markAsSent();
            }
        }
    }
}

void ServerController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender)
{
    
    QString senderInfo = sender.isEmpty() ? "Server" : sender;
    auto type = sender.isEmpty() ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received;
    auto direction = messageTypeToDirection(type);
    
    if (m_serverView) {
        QString serverHost = m_server ? m_server->getServerIpAddress() : "localhost";
        FileMessageItem *fileItem = new FileMessageItem(
            fileName,
            fileSize,
            fileUrl,
            senderInfo,
            direction,
            serverHost,
            QDateTime::currentDateTime().toString("hh:mm")
        );
        setupFileMessageItem(fileItem);
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
    
    qDebug() << "📨 [MessageReceived] Type:" << type << "From:" << senderId;
    
    // Handle delete message
    if (type == "delete") {
        int messageId = obj["messageId"].toInt();
        QString sender = senderId;
        qDebug() << "🗑️ [SERVER Delete Received] Message ID:" << messageId << "from sender:" << sender;
        
        // First try to find by database ID
        if (m_serverView) {
            m_serverView->removeMessageByDatabaseId(messageId);
            qDebug() << "✅ [SERVER Delete Received] Called removeMessageByDatabaseId";
            
            // If not found by ID, try to remove last message from sender
            m_serverView->removeLastMessageFromSender(sender);
            qDebug() << "✅ [SERVER Delete Received] Called removeLastMessageFromSender";
        }
        if (m_db) {
            m_db->deleteMessage(messageId);
            qDebug() << "✅ [SERVER Delete Received] Deleted from database";
        }
        return;
    }
    
    // Handle edit message
    if (type == "edit") {
        int messageId = obj["messageId"].toInt();
        QString newText = obj["newText"].toString();
        QString sender = senderId;
        qDebug() << "✏️ [SERVER Edit Received] Message ID:" << messageId << "New text:" << newText << "from sender:" << sender;
        
        // First try to find by database ID
        if (m_serverView) {
            m_serverView->updateMessageByDatabaseId(messageId, newText);
            qDebug() << "✅ [SERVER Edit Received] Called updateMessageByDatabaseId";
            
            // If not found by ID, try to update last message from sender
            m_serverView->updateLastMessageFromSender(sender, newText);
            qDebug() << "✅ [SERVER Edit Received] Called updateLastMessageFromSender";
        }
        if (m_db) {
            m_db->updateMessage(messageId, newText, true);
            qDebug() << "✅ [SERVER Edit Received] Updated database";
        }
        return;
    }
    
    if (type == "text") {
        msgData.isFileMessage = false;
        msgData.isVoiceMessage = false;
        msgData.text = obj["content"].toString();
    } else if (type == "file") {
        msgData.isFileMessage = true;
        msgData.isVoiceMessage = false;
        msgData.fileInfo.fileName = obj["fileName"].toString();
        msgData.fileInfo.fileSize = obj["fileSize"].toInteger();
        msgData.fileInfo.fileUrl = obj["fileUrl"].toString();
    } else if (type == "voice") {
        msgData.isVoiceMessage = true;
        msgData.isFileMessage = false;
        msgData.voiceInfo.url = obj["voiceUrl"].toString(obj["fileUrl"].toString());
        msgData.voiceInfo.duration = obj["duration"].toInt();
        msgData.fileInfo.fileName = obj["fileName"].toString();
        msgData.fileInfo.fileUrl = msgData.voiceInfo.url;
        msgData.fileInfo.fileSize = obj["fileSize"].toInteger();
        
        // Parse waveform array if present
        if (obj.contains("waveform") && obj["waveform"].isArray()) {
            QJsonArray waveformArray = obj["waveform"].toArray();
            QVector<qreal> waveform;
            waveform.reserve(waveformArray.size());
            for (const QJsonValue &val : waveformArray) {
                waveform.append(val.toDouble());
            }
            msgData.voiceInfo.waveform = waveform;
            qDebug() << "📊 Received waveform with" << waveform.size() << "samples";
        }
        
        if (msgData.fileInfo.fileName.isEmpty()) {
            const QUrl url(msgData.voiceInfo.url);
            msgData.fileInfo.fileName = url.fileName();
        }
    }
    
    QString cleanFilteredUser = m_currentFilteredUser.split(" - ").first().trimmed();
    bool shouldDisplay = m_isBroadcastMode || m_currentFilteredUser.isEmpty() || senderId == cleanFilteredUser;
    
    if (!msgData.isFileMessage && !msgData.isVoiceMessage) {
        QStringList chunks = splitMessageIntoChunks(msgData.text);
        for (const QString &chunk : chunks) {
            MessageData chunkData = msgData;
            chunkData.text = chunk;
            chunkData.isEdited = false;
            
            if (m_db) {
                chunkData.databaseId = m_db->saveMessage(senderId, "Server", chunk, QDateTime::currentDateTime(), false);
            } else {
                chunkData.databaseId = -1;
            }
            
            if (m_serverView && shouldDisplay) {
                QWidget* itemWidget = createWidgetFromData(chunkData);
                m_serverView->addMessageItem(itemWidget);
            }
        }
        return;
    }
    
    // 3. Save to database (with standard format) for file/voice
    if (m_db) {
        QString dbMessage;
        if (msgData.isVoiceMessage) {
            dbMessage = QString("VOICE|%1|%2|%3")
                            .arg(msgData.fileInfo.fileName)
                            .arg(msgData.voiceInfo.duration)
                            .arg(msgData.voiceInfo.url);
        } else if (msgData.isFileMessage) {
            dbMessage = QString("FILE|%1|%2|%3")
                            .arg(msgData.fileInfo.fileName)
                            .arg(msgData.fileInfo.fileSize)
                            .arg(msgData.fileInfo.fileUrl);
        }
        if (!dbMessage.isEmpty()) {
            msgData.databaseId = m_db->saveMessage(senderId, "Server", dbMessage, QDateTime::currentDateTime(), false);
            qDebug() << "✅ [SERVER] Saved file/voice to DB with ID:" << msgData.databaseId;
        }
    } else {
        msgData.databaseId = -1;
    }
    
    // 4. Display in server window (only if viewing this user or in broadcast mode)
    if (m_serverView && shouldDisplay) {
        QWidget* itemWidget = createWidgetFromData(msgData);
        m_serverView->addMessageItem(itemWidget);
    }
}

void ServerController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost, const QVector<qreal> &waveform)
{
    qDebug() << "🔴 [CONTROLLER] onFileUploaded called - fileName:" << fileName << "waveform size:" << waveform.size();
    qDebug() << "ServerController: File uploaded:" << fileName << url << fileSize << "waveform size:" << waveform.size();
    
    // Detect if this is a voice message based on file extension
    bool isVoice = fileName.endsWith(".m4a", Qt::CaseInsensitive) || 
                   fileName.endsWith(".ogg", Qt::CaseInsensitive) ||
                   fileName.endsWith(".wav", Qt::CaseInsensitive) ||
                   fileName.endsWith(".mp3", Qt::CaseInsensitive);
    
    // 1. Create MessageData
    MessageData msgData;
    msgData.senderName = "You";
    msgData.senderType = MessageData::User_Me;
    msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    
    QString jsonMessage;
    QString dbMessage;
    
    if (isVoice) {
        // Voice message WITH waveform data
        msgData.isVoiceMessage = true;
        msgData.isFileMessage = false;
        msgData.fileInfo.fileName = fileName;
        msgData.fileInfo.fileSize = fileSize;
        msgData.fileInfo.fileUrl = url;
        msgData.voiceInfo.url = url;
        msgData.voiceInfo.duration = 0;
        msgData.voiceInfo.waveform = waveform; // Store real waveform!
        
        // Convert waveform to JSON array string
        QString waveformJson = "[";
        for (int i = 0; i < waveform.size(); ++i) {
            if (i > 0) waveformJson += ",";
            waveformJson += QString::number(waveform[i], 'f', 4);
        }
        waveformJson += "]";
        
        jsonMessage = QString(R"({"type":"voice","fileName":"%1","fileSize":%2,"fileUrl":"%3","duration":0,"waveform":%4,"sender":"Server","timestamp":"%5"})")
                          .arg(fileName)
                          .arg(fileSize)
                          .arg(url)
                          .arg(waveformJson)
                          .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
        
        // Save waveform in database: VOICE|fileName|duration|url|waveformJson
        dbMessage = QString("VOICE|%1|0|%2|%3").arg(fileName).arg(url).arg(waveformJson);
        
        qDebug() << "🎙️ Saving voice to DB with waveform size:" << waveform.size();
    } else {
        // Regular file message
        msgData.isFileMessage = true;
        msgData.isVoiceMessage = false;
        msgData.fileInfo.fileName = fileName;
        msgData.fileInfo.fileSize = fileSize;
        msgData.fileInfo.fileUrl = url;
        
        jsonMessage = QString(R"({"type":"file","fileName":"%1","fileSize":%2,"fileUrl":"%3","sender":"Server","timestamp":"%4"})")
                          .arg(fileName)
                          .arg(fileSize)
                          .arg(url)
                          .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
        
        dbMessage = QString("FILE|%1|%2|%3").arg(fileName).arg(fileSize).arg(url);
    }
    
    if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
        // Broadcast to all
        if (m_server) {
            m_server->broadcastToAll(jsonMessage);
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", "ALL", dbMessage, QDateTime::currentDateTime(), false);
        }
    } else {
        // Send to specific user
        QString cleanUserId = m_currentPrivateTargetUser.split(" - ").first().trimmed();
        
        if (m_server) {
            m_server->sendMessageToClient(cleanUserId, jsonMessage);
        }
        
        // Save to database
        if (m_db) {
            m_db->saveMessage("Server", cleanUserId, dbMessage, QDateTime::currentDateTime(), false);
        }
    }
    
    // **FIX: Widget قبلاً در View ساخته شده - نباید دوباره بسازیم!**
    // این تابع فقط برای ارسال به WebSocket و ذخیره در DB است
    qDebug() << "🟣 [CONTROLLER] SKIPPING widget creation (already created in View):" << fileName;
}

void ServerController::loadHistoryForServer()
{
    if (!m_db || !m_serverView) return;
    
    m_serverView->clearChatHistory();
    // Load only broadcast messages (Server -> ALL)
    QStringList history = m_db->loadMessagesBetween("Server", "ALL");
    
    for (const QString &dbString : history) {
        QStringList parts = dbString.split("|");
        if (parts.size() < 3) continue;
        
        MessageData msgData;
        msgData.timestamp = parts[0];
        msgData.senderName = parts[1];
        bool hasMetadata = parts.size() >= 5;
        msgData.isEdited = hasMetadata ? (parts[2] == "1") : false;
        msgData.databaseId = hasMetadata ? parts[3].toInt() : -1;
        QString messageContent = hasMetadata ? parts.mid(4).join("|")
                                             : parts.mid(2).join("|");
        
        // Determine sender type
        if (msgData.senderName == "Server" || msgData.senderName == "You") {
            msgData.senderType = MessageData::User_Me;
            msgData.senderName = "You";
        } else {
            msgData.senderType = MessageData::User_Other;
        }
        
        // Check if it's a voice or file message
        if (messageContent.startsWith("VOICE|")) {
            QStringList voiceParts = messageContent.split("|");
            if (voiceParts.size() >= 4) {
                msgData.isVoiceMessage = true;
                msgData.isFileMessage = false;
                msgData.fileInfo.fileName = voiceParts[1];
                msgData.fileInfo.fileUrl = voiceParts[3];
                msgData.voiceInfo.duration = voiceParts[2].toInt();
                msgData.voiceInfo.url = voiceParts[3];
                
                // Parse waveform if available (voiceParts[4] is JSON array)
                if (voiceParts.size() >= 5) {
                    QString waveformJson = voiceParts[4];
                    QJsonDocument doc = QJsonDocument::fromJson(waveformJson.toUtf8());
                    if (doc.isArray()) {
                        QJsonArray waveformArray = doc.array();
                        QVector<qreal> waveform;
                        for (const QJsonValue &val : waveformArray) {
                            waveform.append(val.toDouble());
                        }
                        msgData.voiceInfo.waveform = waveform;
                        qDebug() << "📊 [loadHistoryForServer] Loaded waveform from DB with" << waveform.size() << "samples";
                    }
                }
            }
        } else if (messageContent.startsWith("FILE|")) {
            QStringList fileParts = messageContent.split("|");
            if (fileParts.size() >= 4) {
                msgData.isFileMessage = true;
                msgData.isVoiceMessage = false;
                msgData.fileInfo.fileName = fileParts[1];
                msgData.fileInfo.fileSize = fileParts[2].toLongLong();
                msgData.fileInfo.fileUrl = fileParts[3];
            }
        } else {
            msgData.isFileMessage = false;
            msgData.isVoiceMessage = false;
            // Check if message looks encrypted/unreadable
            if (messageContent.trimmed().isEmpty() || messageContent.contains(QRegularExpression("^[^a-zA-Z0-9\\s]{3,}$"))) {
                msgData.text = "(encrypted message - key unavailable)";
            } else {
                msgData.text = messageContent;
            }
        }
        
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
        QStringList parts = dbString.split("|");
        if (parts.size() < 3) continue;
        
        MessageData msgData;
        msgData.timestamp = parts[0];
        msgData.senderName = parts[1];
        bool hasMetadata = parts.size() >= 5;
        msgData.isEdited = hasMetadata ? (parts[2] == "1") : false;
        msgData.databaseId = hasMetadata ? parts[3].toInt() : -1;
        QString messageContent = hasMetadata ? parts.mid(4).join("|")
                                             : parts.mid(2).join("|");
        
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
        
        // Check if it's a voice or file message
        if (messageContent.startsWith("VOICE|")) {
            QStringList voiceParts = messageContent.split("|");
            if (voiceParts.size() >= 4) {
                msgData.isVoiceMessage = true;
                msgData.isFileMessage = false;
                msgData.fileInfo.fileName = voiceParts[1];
                msgData.fileInfo.fileUrl = voiceParts[3];
                msgData.voiceInfo.duration = voiceParts[2].toInt();
                msgData.voiceInfo.url = voiceParts[3];
                
                // Parse waveform if available (voiceParts[4] is JSON array)
                if (voiceParts.size() >= 5) {
                    QString waveformJson = voiceParts[4];
                    QJsonDocument doc = QJsonDocument::fromJson(waveformJson.toUtf8());
                    if (doc.isArray()) {
                        QJsonArray waveformArray = doc.array();
                        QVector<qreal> waveform;
                        for (const QJsonValue &val : waveformArray) {
                            waveform.append(val.toDouble());
                        }
                        msgData.voiceInfo.waveform = waveform;
                        qDebug() << "📊 [loadHistoryForUser] Loaded waveform from DB with" << waveform.size() << "samples";
                    }
                }
            }
        } else if (messageContent.startsWith("FILE|")) {
            QStringList fileParts = messageContent.split("|");
            if (fileParts.size() >= 4) {
                msgData.isFileMessage = true;
                msgData.isVoiceMessage = false;
                msgData.fileInfo.fileName = fileParts[1];
                msgData.fileInfo.fileSize = fileParts[2].toLongLong();
                msgData.fileInfo.fileUrl = fileParts[3];
            }
        } else {
            msgData.isFileMessage = false;
            msgData.isVoiceMessage = false;
            // Check if message looks encrypted/unreadable
            if (messageContent.trimmed().isEmpty() || messageContent.contains(QRegularExpression("^[^a-zA-Z0-9\\s]{3,}$"))) {
                msgData.text = "(encrypted message - key unavailable)";
            } else {
                msgData.text = messageContent;
            }
        }
        
        QWidget* itemWidget = createWidgetFromData(msgData);
        m_serverView->addMessageItem(itemWidget);
    }
}

void ServerController::onTextMessageCopyRequested(const QString &text)
{
    if (!m_serverView || text.isEmpty()) {
        return;
    }

    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
    m_serverView->showTransientNotification(QObject::tr("Text Copied!"));
}

void ServerController::onTextMessageEditRequested(TextMessageItem *item)
{
    if (!m_serverView || !item) {
        return;
    }

    if (item->direction() != MessageDirection::Outgoing) {
        m_serverView->showTransientNotification(QObject::tr("You can only edit your messages"));
        return;
    }

    if (item->databaseId() < 0) {
        m_serverView->showTransientNotification(QObject::tr("Message cannot be edited right now"));
        return;
    }

    m_serverView->enterMessageEditMode(item);
}

void ServerController::onTextMessageDeleteRequested(TextMessageItem *item)
{
    handleDeleteRequest(item, DeleteRequestScope::Prompt);
}

void ServerController::onFileMessageDeleteRequested(FileMessageItem *item)
{
    qDebug() << "🗑️🗑️🗑️ [SERVER onFileMessageDeleteRequested] CALLED!";
    qDebug() << "    Item:" << item;
    qDebug() << "    Database ID:" << (item ? item->databaseId() : -999);
    qDebug() << "    Direction:" << (item ? static_cast<int>(item->direction()) : -999);
    handleDeleteRequest(item, DeleteRequestScope::Prompt);
}

void ServerController::onVoiceMessageDeleteRequested(VoiceMessageItem *item)
{
    qDebug() << "🗑️🗑️🗑️ [SERVER onVoiceMessageDeleteRequested] CALLED!";
    qDebug() << "    Item:" << item;
    qDebug() << "    Database ID:" << (item ? item->databaseId() : -999);
    qDebug() << "    Direction:" << (item ? static_cast<int>(item->direction()) : -999);
    handleDeleteRequest(item, DeleteRequestScope::Prompt);
}

void ServerController::handleDeleteRequest(MessageComponent *item, DeleteRequestScope scope)
{
    if (!m_serverView || !item) {
        return;
    }

    if (scope != DeleteRequestScope::MeOnly && item->direction() != MessageDirection::Outgoing) {
        m_serverView->showTransientNotification(QObject::tr("You can only delete your messages"));
        return;
    }

    if (item->databaseId() < 0) {
        m_serverView->showTransientNotification(QObject::tr("Message cannot be deleted right now"));
        return;
    }

    bool deleteForBoth = false;
    QMessageBox msgBox(m_serverView);
    msgBox.setWindowTitle(QObject::tr("Delete Message"));
    msgBox.setIcon(QMessageBox::Question);

    switch (scope) {
    case DeleteRequestScope::Prompt: {
        msgBox.setText(QObject::tr("This message will be removed. Are you sure?"));
        QCheckBox *deleteForBothCheckbox = new QCheckBox(QObject::tr("Delete for both sides"));
        deleteForBothCheckbox->setChecked(false);
        msgBox.setCheckBox(deleteForBothCheckbox);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes) {
            return;
        }
        deleteForBoth = deleteForBothCheckbox->isChecked();
        break;
    }
    case DeleteRequestScope::BothSides: {
        msgBox.setText(QObject::tr("This message will be removed for both sides. Are you sure?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes) {
            return;
        }
        deleteForBoth = true;
        break;
    }
    case DeleteRequestScope::MeOnly: {
        msgBox.setText(QObject::tr("This message will be removed from your chat. Are you sure?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes) {
            return;
        }
        deleteForBoth = false;
        break;
    }
    }

    qDebug() << "🗑️ [Delete] Starting delete for message ID:" << item->databaseId() << "Delete for both:" << deleteForBoth;

    if (m_db && item->databaseId() >= 0) {
        m_db->deleteMessage(item->databaseId());
        qDebug() << "✅ [Delete] Deleted from database";
    }

    m_serverView->removeMessageItem(item);
    qDebug() << "✅ [Delete] Removed from UI";

    if (deleteForBoth) {
        QJsonObject deleteMsg;
        deleteMsg["type"] = "delete";
        deleteMsg["messageId"] = item->databaseId();
        deleteMsg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        deleteMsg["sender"] = "Server";

        QJsonDocument doc(deleteMsg);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);

        qDebug() << "📤 [Delete] Sending delete message:" << jsonMessage;

        if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
            if (m_server) {
                m_server->broadcastToAll(jsonMessage);
                qDebug() << "📡 [Delete] Broadcasted to all";
            } else {
                qDebug() << "❌ [Delete] Server is null (broadcast)";
            }
        } else {
            QString targetUser = m_currentPrivateTargetUser.split(" - ").first().trimmed();
            if (m_server) {
                m_server->sendMessageToClient(targetUser, jsonMessage);
                qDebug() << "📨 [Delete] Sent to user:" << targetUser;
            } else {
                qDebug() << "❌ [Delete] Server is null (private)";
            }
        }
    } else {
        qDebug() << "ℹ️ [Delete] Delete for both sides not selected";
    }
}

void ServerController::onTextMessageEditConfirmed(TextMessageItem *item, const QString &newText)
{
    if (!item) {
        qDebug() << "❌ [Edit] item is null";
        return;
    }

    qDebug() << "✏️ [Edit] Starting edit for message ID:" << item->databaseId() << "New text:" << newText;

    const QString trimmed = newText.trimmed();
    if (trimmed.isEmpty()) {
        if (m_serverView) {
            QMessageBox::warning(m_serverView, QObject::tr("Empty Message"), QObject::tr("Message text cannot be empty."));
        }
        return;
    }

    QStringList chunks = splitMessageIntoChunks(trimmed);
    if (chunks.isEmpty()) {
        chunks << trimmed;
    }

    // Update in database
    if (m_db && item->databaseId() >= 0) {
        m_db->updateMessage(item->databaseId(), chunks.first(), true);
        qDebug() << "✅ [Edit] Updated in database";
    }

    // Update UI locally
    item->updateMessageText(chunks.first());
    item->markAsEdited(true);
    if (m_serverView) {
        m_serverView->refreshMessageItem(item);
        qDebug() << "✅ [Edit] Updated UI locally";
    }
    
    // Always send edit to other side (no dialog)
    QJsonObject editMsg;
    editMsg["type"] = "edit";
    editMsg["messageId"] = item->databaseId();
    editMsg["newText"] = chunks.first();
    editMsg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    editMsg["sender"] = "Server";
    
    QJsonDocument doc(editMsg);
    QString jsonMessage = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "📤 [Edit] Sending edit message:" << jsonMessage;
    
    if (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty()) {
        if (m_server) {
            m_server->broadcastToAll(jsonMessage);
            qDebug() << "📡 [Edit] Broadcasted to all";
        } else {
            qDebug() << "❌ [Edit] Server is null (broadcast)";
        }
    } else {
        QString targetUser = m_currentPrivateTargetUser.split(" - ").first().trimmed();
        if (m_server) {
            m_server->sendMessageToClient(targetUser, jsonMessage);
            qDebug() << "📨 [Edit] Sent to user:" << targetUser;
        } else {
            qDebug() << "❌ [Edit] Server is null (private)";
        }
    }

    QString targetReceiver = (m_isBroadcastMode || m_currentPrivateTargetUser.isEmpty())
                                 ? QStringLiteral("ALL")
                                 : m_currentPrivateTargetUser.split(" - ").first().trimmed();

    if (chunks.size() > 1 && m_serverView) {
        for (int i = 1; i < chunks.size(); ++i) {
            MessageData chunkData;
            chunkData.text = chunks.at(i);
            chunkData.senderName = QStringLiteral("You");
            chunkData.senderType = MessageData::User_Me;
            chunkData.timestamp = item->timestamp();
            chunkData.isFileMessage = false;
            chunkData.isVoiceMessage = false;
            chunkData.isEdited = true;
            chunkData.databaseId = m_db ? m_db->saveMessage("Server", targetReceiver, chunkData.text, QDateTime::currentDateTime(), true) : -1;

            QWidget *extraWidget = createWidgetFromData(chunkData);
            m_serverView->addMessageItem(extraWidget);

            if (TextMessageItem *extraText = qobject_cast<TextMessageItem*>(extraWidget)) {
                extraText->markAsEdited(true);
                extraText->setDatabaseId(chunkData.databaseId);
                m_serverView->refreshMessageItem(extraText);
            }
        }
    }

    if (m_serverView) {
        m_serverView->exitMessageEditMode();
    }
}

QStringList ServerController::splitMessageIntoChunks(const QString &text) const
{
    QStringList chunks;
    if (text.isEmpty()) {
        chunks << QString();
        return chunks;
    }
    
    int start = 0;
    while (start < text.length()) {
        chunks << text.mid(start, kMaxMessageLength);
        start += kMaxMessageLength;
    }
    return chunks;
}
