#include "ClientController.h"
#include "View/ClientChatWindow.h"
#include "Model/Network/WebSocketClient.h"
#include "Model/Core/DatabaseManager.h"
#include "MessageAliases.h"
#include "MessageData.h"
#include "MessageComponent.h"
#include "../../CommonCore/NotificationManager.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QMessageBox>
#include <QClipboard>
#include <QGuiApplication>
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

ClientController::ClientController(ClientChatWindow *view, WebSocketClient *client, DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_clientView(view)
    , m_client(client)
    , m_db(db)
    , m_clientUserId("Client")
{
    
    if (m_clientView) {
        NotificationManager::instance().setup(m_clientView);
        // FIX: Use sendMessageRequested instead of messageSent
        connect(m_clientView, &ClientChatWindow::sendMessageRequested, this, &ClientController::displayNewMessages);
        
        connect(m_clientView, &ClientChatWindow::fileUploaded, this, &ClientController::onFileUploaded);
        
        connect(m_clientView, &ClientChatWindow::messageEditConfirmed,
                this, &ClientController::onTextMessageEditConfirmed);
        
        connect(m_clientView, &ClientChatWindow::windowStateChanged, this, &ClientController::onWindowStateChanged);
    } else {
    }

    if (m_client) {
        connect(m_client, &WebSocketClient::messageReceived, this, &ClientController::onMessageReceived);
    } else {
    }

    loadHistory();
}

ClientController::~ClientController()
{
}

QWidget* ClientController::createWidgetFromData(const MessageData &msgData)
{
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
            m_serverHost,
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
            m_serverHost,
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

void ClientController::setupTextMessageItem(TextMessageItem *item)
{
    if (!item) {
        return;
    }

    connect(item, &TextMessageItem::copyRequested,
            this, &ClientController::onTextMessageCopyRequested,
            Qt::UniqueConnection);
    connect(item, &TextMessageItem::editRequested,
            this, &ClientController::onTextMessageEditRequested,
            Qt::UniqueConnection);
    connect(item, &TextMessageItem::deleteRequested,
            this, &ClientController::onTextMessageDeleteRequested,
            Qt::UniqueConnection);
}

void ClientController::setupFileMessageItem(FileMessageItem *item)
{
    if (!item) {
        return;
    }

    connect(item, &FileMessageItem::deleteRequested,
            this, &ClientController::onFileMessageDeleteRequested,
            Qt::UniqueConnection);
}

void ClientController::setupVoiceMessageItem(VoiceMessageItem *item)
{
    if (!item) {
        return;
    }

    connect(item, &VoiceMessageItem::deleteRequested,
            this, &ClientController::onVoiceMessageDeleteRequested,
            Qt::UniqueConnection);
}

void ClientController::displayNewConnection()
{
}

void ClientController::displayNewMessages(const QString &message)
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
    msgData.isEdited = false;
    msgData.databaseId = -1;
    
    const QStringList chunks = splitMessageIntoChunks(message);
    const QString isoTimestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    for (const QString &chunk : chunks) {
        MessageData chunkData = msgData;
        chunkData.text = chunk;
        // 2. Send via WebSocket (convert to JSON)
        if (m_client) {
            QString jsonMessage = QString(R"({"type":"text","content":"%1","sender":"Client","timestamp":"%2"})")
                                      .arg(chunk)
                                      .arg(isoTimestamp);
            m_client->sendMessage(jsonMessage);
        }
        // 3. Save to database
        if (m_db) {
            int dbId = m_db->saveMessage(m_clientUserId, "Server", chunk, QDateTime::currentDateTime(), false);
            chunkData.databaseId = dbId;
        }
        chunkData.isEdited = false;
        // 4. Create widget from data
        QWidget* itemWidget = createWidgetFromData(chunkData);
        // 5. Show in client window
        if (m_clientView) {
            m_clientView->addMessageItem(itemWidget);
            TextMessageItem* textItem = qobject_cast<TextMessageItem*>(itemWidget);
            if (textItem) {
                textItem->setDatabaseId(chunkData.databaseId);
                if (chunkData.isEdited) {
                    textItem->markAsEdited(true);
                }
                textItem->markAsSent();
            }
        }
    }
}

void ClientController::displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender, const QString &serverHost)
{
    
    QString senderInfo = sender.isEmpty() ? "You" : sender;
    auto type = sender.isEmpty() ? BaseChatWindow::MessageType::Sent : BaseChatWindow::MessageType::Received;
    auto direction = messageTypeToDirection(type);
    
    if (m_clientView) {
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
        m_clientView->addMessageItem(fileItem);
    }
}

void ClientController::onMessageReceived(const QString &message)
{
    // 1. Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
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
    NotificationManager::instance().checkAndNotify(
        msgData.senderName,
        "You have unread message!",
        "Server", // Client is always looking at Server chat technically
        false
        );
    QString timestampStr = obj["timestamp"].toString();
    if (!timestampStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(timestampStr, Qt::ISODate);
        msgData.timestamp = dt.toString("hh:mm");
    } else {
        msgData.timestamp = QDateTime::currentDateTime().toString("hh:mm");
    }
    QString type = obj["type"].toString();
    // Handle delete message
    if (type == "delete") {
        int messageId = obj["messageId"].toInt();
        QString sender = msgData.senderName;
        // First try to find by database ID
        if (m_clientView) {
            m_clientView->removeMessageByDatabaseId(messageId);
            // If not found by ID, try to remove last message from sender
            m_clientView->removeLastMessageFromSender(sender);
        }
        if (m_db) {
            m_db->deleteMessage(messageId);
        }
        return;
    }
    
    // Handle edit message
    if (type == "edit") {
        int messageId = obj["messageId"].toInt();
        QString newText = obj["newText"].toString();
        QString sender = msgData.senderName;
        
        // First try to find by database ID
        if (m_clientView) {
            m_clientView->updateMessageByDatabaseId(messageId, newText);
            
            // If not found by ID, try to update last message from sender
            m_clientView->updateLastMessageFromSender(sender, newText);
        } else {
        }
        if (m_db) {
            m_db->updateMessage(messageId, newText, true);
        } else {
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
        }
        
        if (msgData.fileInfo.fileName.isEmpty()) {
            const QUrl url(msgData.voiceInfo.url);
            msgData.fileInfo.fileName = url.fileName();
        }
    }
    
    if (!msgData.isFileMessage && !msgData.isVoiceMessage) {
        QStringList chunks = splitMessageIntoChunks(msgData.text);
        for (const QString &chunk : chunks) {
            MessageData chunkData = msgData;
            chunkData.text = chunk;
            chunkData.isEdited = false;
            
            if (m_db) {
                chunkData.databaseId = m_db->saveMessage("Server", m_clientUserId, chunk, QDateTime::currentDateTime(), false);
            } else {
                chunkData.databaseId = -1;
            }
            
            if (m_clientView) {
                QWidget* itemWidget = createWidgetFromData(chunkData);
                m_clientView->addMessageItem(itemWidget);
            }
        }
        return;
    }
    
    // 3. Save to database (with standard format)
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
            msgData.databaseId = m_db->saveMessage("Server", m_clientUserId, dbMessage, QDateTime::currentDateTime(), false);
        }
    } else {
        msgData.databaseId = -1;
    }
    
    // 4. Create widget (View)
    QWidget* itemWidget = createWidgetFromData(msgData);
    
    // 5. Deliver prepared widget to View
    if (m_clientView) {
        m_clientView->addMessageItem(itemWidget);
    }
}

QStringList ClientController::splitMessageIntoChunks(const QString &text) const
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

void ClientController::onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost, const QVector<qreal> &waveform)
{
    
    // Store server host for future downloads
    if (!serverHost.isEmpty()) {
        m_serverHost = serverHost;
    }
    
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
    
    if (isVoice) {
        // Voice message WITH waveform data
        msgData.isVoiceMessage = true;
        msgData.isFileMessage = false;
        msgData.fileInfo.fileName = fileName;
        msgData.fileInfo.fileSize = fileSize;
        msgData.fileInfo.fileUrl = url;
        msgData.voiceInfo.url = url;
        msgData.voiceInfo.duration = 0; // Duration unknown for now
        msgData.voiceInfo.waveform = waveform; // Store real waveform!
        
        // Convert waveform to JSON array string
        QString waveformJson = "[";
        for (int i = 0; i < waveform.size(); ++i) {
            if (i > 0) waveformJson += ",";
            waveformJson += QString::number(waveform[i], 'f', 4);
        }
        waveformJson += "]";
        
        // 2. Send via WebSocket as voice message WITH waveform
        QString jsonMessage = QString(R"({"type":"voice","fileName":"%1","fileSize":%2,"fileUrl":"%3","duration":0,"waveform":%4,"sender":"Client","timestamp":"%5"})")
                                  .arg(fileName)
                                  .arg(fileSize)
                                  .arg(url)
                                  .arg(waveformJson)
                                  .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
        
        if (m_client) {
            m_client->sendMessage(jsonMessage);
        }
        
        // 3. Save to database with waveform: VOICE|name|duration|url|waveformJson
        if (m_db) {
            QString voiceMessage = QString("VOICE|%1|0|%2|%3").arg(fileName).arg(url).arg(waveformJson);
            m_db->saveMessage(m_clientUserId, "Server", voiceMessage, QDateTime::currentDateTime(), false);
        }
        
        // **FIX: voice widget قبلاً در View ساخته شده - نباید دوباره بسازیم**
    } else {
        // Regular file message
        msgData.isFileMessage = true;
        msgData.isVoiceMessage = false;
        msgData.fileInfo.fileName = fileName;
        msgData.fileInfo.fileSize = fileSize;
        msgData.fileInfo.fileUrl = url;
        
        // 2. Send via WebSocket as file message
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
            m_db->saveMessage(m_clientUserId, "Server", fileMessage, QDateTime::currentDateTime(), false);
        }
        
    }
}

void ClientController::loadHistory()
{
    if (!m_db || !m_clientView) return;
    
    
    QStringList history = m_db->loadMessagesBetween(m_clientUserId, "Server");
    
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
        if (msgData.senderName == "Client" || msgData.senderName == "You") {
            msgData.senderType = MessageData::User_Me;
            msgData.senderName = "You";
        } else {
            msgData.senderType = MessageData::User_Other;
        }
        
        // Check message type markers
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
        
        // Create widget(s) and add to view
        if (!msgData.isFileMessage && !msgData.isVoiceMessage) {
            QStringList chunks = splitMessageIntoChunks(msgData.text);
            for (const QString &chunk : chunks) {
                MessageData chunkData = msgData;
                chunkData.text = chunk;
                QWidget* chunkWidget = createWidgetFromData(chunkData);
                m_clientView->addMessageItem(chunkWidget);
            }
        } else {
            QWidget* itemWidget = createWidgetFromData(msgData);
            m_clientView->addMessageItem(itemWidget);
        }
    }
}

void ClientController::onTextMessageCopyRequested(const QString &text)
{
    if (!m_clientView || text.isEmpty()) {
        return;
    }

    if (QClipboard *clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
    m_clientView->showTransientNotification(QObject::tr("Text Copied!"));
}

void ClientController::onTextMessageEditRequested(TextMessageItem *item)
{
    if (!m_clientView || !item) {
        return;
    }

    if (item->direction() != MessageDirection::Outgoing) {
        m_clientView->showTransientNotification(tr("You can only edit your messages"));
        return;
    }

    if (item->databaseId() < 0) {
        m_clientView->showTransientNotification(tr("Message cannot be edited right now"));
        return;
    }

    m_clientView->enterMessageEditMode(item);
}

void ClientController::onTextMessageDeleteRequested(TextMessageItem *item)
{
    handleDeleteRequest(item, DeleteRequestScope::Prompt);
}

void ClientController::onFileMessageDeleteRequested(FileMessageItem *item)
{
    handleDeleteRequest(item, DeleteRequestScope::Prompt);
}

void ClientController::onVoiceMessageDeleteRequested(VoiceMessageItem *item)
{
    handleDeleteRequest(item, DeleteRequestScope::Prompt);
}

void ClientController::handleDeleteRequest(MessageComponent *item, DeleteRequestScope scope)
{
    
    if (!m_clientView || !item) {
        return;
    }

    
    if (scope != DeleteRequestScope::MeOnly && item->direction() != MessageDirection::Outgoing) {
        if (scope == DeleteRequestScope::Prompt) {
            m_clientView->showTransientNotification(tr("You can only delete your messages"));
        } else {
            m_clientView->showTransientNotification(tr("You can only delete your messages for everyone"));
        }
        return;
    }

    if (item->databaseId() < 0) {
        m_clientView->showTransientNotification(tr("Message cannot be deleted right now"));
        return;
    }

    
    bool deleteForBoth = false;
    QMessageBox msgBox(m_clientView);
    msgBox.setWindowTitle(tr("Delete Message"));
    msgBox.setIcon(QMessageBox::Question);

    if (scope == DeleteRequestScope::Prompt) {
        msgBox.setText(tr("This message will be removed. Are you sure?"));
        auto *deleteForBothCheckbox = new QCheckBox(tr("Delete for both sides"));
        deleteForBothCheckbox->setChecked(false);
        msgBox.setCheckBox(deleteForBothCheckbox);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        int result = msgBox.exec();
        if (result != QMessageBox::Yes) {
            return;
        }
        deleteForBoth = deleteForBothCheckbox->isChecked();
    } else {
        if (scope == DeleteRequestScope::BothSides) {
            msgBox.setText(tr("This message will be removed for both sides. Are you sure?"));
        } else {
            msgBox.setText(tr("This message will be removed from your chat. Are you sure?"));
        }
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int result = msgBox.exec();
        if (result != QMessageBox::Yes) {
            return;
        }
        deleteForBoth = (scope == DeleteRequestScope::BothSides);
    }


    if (m_db && item->databaseId() >= 0) {
        m_db->deleteMessage(item->databaseId());
    } else {
    }

    m_clientView->removeMessageItem(item);

    if (deleteForBoth && m_client) {
        QJsonObject deleteMsg;
        deleteMsg["type"] = "delete";
        deleteMsg["messageId"] = item->databaseId();
        deleteMsg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        deleteMsg["sender"] = m_clientUserId;

        QJsonDocument doc(deleteMsg);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);
        m_client->sendMessage(jsonMessage);
    } else if (deleteForBoth) {
    } else {
    }
    
}

void ClientController::onTextMessageEditConfirmed(TextMessageItem *item, const QString &newText)
{
    if (!item) {
        return;
    }


    const QString trimmed = newText.trimmed();
    if (trimmed.isEmpty()) {
        if (m_clientView) {
            QMessageBox::warning(m_clientView, QObject::tr("Empty Message"), QObject::tr("Message text cannot be empty."));
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
    }

    // Update UI locally
    item->updateMessageText(chunks.first());
    item->markAsEdited(true);
    if (m_clientView) {
        m_clientView->refreshMessageItem(item);
    }
    
    // Always send edit to other side (no dialog)
    if (m_client) {
        QJsonObject editMsg;
        editMsg["type"] = "edit";
        editMsg["messageId"] = item->databaseId();
        editMsg["newText"] = chunks.first();
        editMsg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        editMsg["sender"] = m_clientUserId;
        
        QJsonDocument doc(editMsg);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);
        m_client->sendMessage(jsonMessage);
    } else {
    }

    if (chunks.size() > 1 && m_clientView) {
        for (int i = 1; i < chunks.size(); ++i) {
            MessageData chunkData; 
            chunkData.text = chunks.at(i);
            chunkData.senderName = item->senderInfo().isEmpty() ? QStringLiteral("You") : item->senderInfo();
            chunkData.senderType = MessageData::User_Me;
            chunkData.timestamp = item->timestamp();
            chunkData.isFileMessage = false;
            chunkData.isVoiceMessage = false;
            chunkData.isEdited = true;
            chunkData.databaseId = m_db ? m_db->saveMessage(m_clientUserId, "Server", chunkData.text, QDateTime::currentDateTime(), true) : -1;

            QWidget *extraWidget = createWidgetFromData(chunkData);
            m_clientView->addMessageItem(extraWidget);

            if (TextMessageItem *extraText = qobject_cast<TextMessageItem*>(extraWidget)) {
                extraText->markAsEdited(true);
                extraText->setDatabaseId(chunkData.databaseId);
                m_clientView->refreshMessageItem(extraText);
            }
        }
    }

    if (m_clientView) {
        m_clientView->exitMessageEditMode();
    }
}
void ClientController::onWindowStateChanged(bool isActive)
{
    if (isActive) {

        // Logic: Clear OS notifications
        NotificationManager::instance().clearNotifications();

        // Logic: Reset internal unread count (if you have one)
        // m_unreadCount = 0;
    }
}
