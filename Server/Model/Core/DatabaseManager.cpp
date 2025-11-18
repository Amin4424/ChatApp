#include "Model/Core/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

DatabaseManager::DatabaseManager(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initDatabase()
{
    // Ensure the directory exists
    QFileInfo fileInfo(m_dbPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create database directory:" << dir.absolutePath();
            return false;
        }
    }
    
    // Setup database connection
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbPath);
    m_db.setDatabaseName(m_dbPath);
    
    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }
    
    qDebug() << "Database opened successfully:" << m_dbPath;
    
    return createTables();
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);
    
    // Create messages table
    QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender TEXT NOT NULL,
            receiver TEXT NOT NULL,
            message TEXT NOT NULL,
            timestamp TEXT NOT NULL
        )
    )";
    
    if (!query.exec(createTableQuery)) {
        qWarning() << "Failed to create messages table:" << query.lastError().text();
        return false;
    }
    
    if (!ensureIsEditedColumn()) {
        return false;
    }
    
    qDebug() << "Database tables created successfully";
    return true;
}

bool DatabaseManager::ensureIsEditedColumn()
{
    QSqlQuery pragmaQuery(m_db);
    if (!pragmaQuery.exec("PRAGMA table_info(messages)")) {
        qWarning() << "Failed to inspect messages table:" << pragmaQuery.lastError().text();
        return false;
    }
    
    bool hasIsEdited = false;
    while (pragmaQuery.next()) {
        if (pragmaQuery.value(1).toString().compare("is_edited", Qt::CaseInsensitive) == 0) {
            hasIsEdited = true;
            break;
        }
    }
    
    if (!hasIsEdited) {
        QSqlQuery alterQuery(m_db);
        if (!alterQuery.exec("ALTER TABLE messages ADD COLUMN is_edited INTEGER DEFAULT 0")) {
            qWarning() << "Failed to add is_edited column:" << alterQuery.lastError().text();
            return false;
        }
    }
    return true;
}

int DatabaseManager::saveMessage(const QString &sender, const QString &receiver, 
                                   const QString &message, const QDateTime &timestamp, bool isEdited)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (sender, receiver, message, timestamp, is_edited) "
                  "VALUES (:sender, :receiver, :message, :timestamp, :is_edited)");
    // Ensure sender/receiver are not empty to satisfy NOT NULL constraints
    QString safeSender = sender;
    QString safeReceiver = receiver;
    if (safeSender.trimmed().isEmpty()) safeSender = "Unknown";
    if (safeReceiver.trimmed().isEmpty()) safeReceiver = "Unknown";

    query.bindValue(":sender", safeSender);
    query.bindValue(":receiver", safeReceiver);
    query.bindValue(":message", message);
    query.bindValue(":timestamp", timestamp.toString(Qt::ISODate));
    query.bindValue(":is_edited", isEdited ? 1 : 0);
    
    if (!query.exec()) {
        qWarning() << "Failed to save message:" << query.lastError().text();
        return -1;
    }
    
    qDebug() << "Message saved: From" << sender << "to" << receiver;
    return query.lastInsertId().toInt();
}

QStringList DatabaseManager::loadAllMessages()
{
    QStringList messages;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT sender, receiver, message, timestamp FROM messages ORDER BY timestamp ASC");
    
    if (!query.exec()) {
        qWarning() << "Failed to load messages:" << query.lastError().text();
        return messages;
    }
    
    while (query.next()) {
        QString sender = query.value(0).toString();
        QString receiver = query.value(1).toString();
        QString message = query.value(2).toString();
        QString timestamp = query.value(3).toString();
        
        QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
        QString timeStr = dt.toString("HH:mm:");
        
        // Format: [time] sender -> receiver: message
        QString formattedMsg = QString("[%1] %2 -> %3: %4")
            .arg(timeStr, sender, receiver, message);
        
        messages.append(formattedMsg);
    }
    
    qDebug() << "Loaded" << messages.size() << "messages from database";
    return messages;
}

QStringList DatabaseManager::loadMessagesBetween(const QString &user1, const QString &user2)
{
    QStringList messages;
    QSqlQuery query(m_db);
    
    query.prepare("SELECT id, sender, receiver, message, timestamp, is_edited FROM messages "
                  "WHERE (sender = :user1 AND receiver = :user2) OR (sender = :user2 AND receiver = :user1) "
                  "ORDER BY timestamp ASC");
    
    query.bindValue(":user1", user1);
    query.bindValue(":user2", user2);
    
    if (!query.exec()) {
        qWarning() << "Failed to load messages:" << query.lastError().text();
        return messages;
    }
    
    while (query.next()) {
        int id = query.value(0).toInt();
        QString sender = query.value(1).toString();
        QString receiver = query.value(2).toString();
        QString message = query.value(3).toString();
        QString timestamp = query.value(4).toString();
        bool isEdited = query.value(5).toInt() != 0;
        
        QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
        QString timeStr = dt.toString("hh:mm");
        
        // Determine label based on perspective
        QString label;
        if (sender == user1) {
            label = "You";
        } else if (sender == "Server") {
            label = "Server";
        } else {
            label = sender.split(" - ").first(); // Extract user name
        }
        
        // Format: TIMESTAMP|sender|message (using pipe as separator)
        QString formattedMsg = QString("%1|%2|%3|%4|%5")
                                   .arg(timeStr,
                                        label,
                                        QString::number(isEdited ? 1 : 0),
                                        QString::number(id),
                                        message);
        messages.append(formattedMsg);
    }
    
    qDebug() << "Loaded" << messages.size() << "messages between" << user1 << "and" << user2;
    return messages;
}

bool DatabaseManager::clearAllMessages()
{
    QSqlQuery query(m_db);
    
    if (!query.exec("DELETE FROM messages")) {
        qWarning() << "Failed to clear messages:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "All messages cleared from database";
    return true;
}

bool DatabaseManager::updateMessage(int messageId, const QString &newMessage, bool isEdited)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE messages SET message = :message, is_edited = :is_edited WHERE id = :id");
    
    query.bindValue(":message", newMessage);
    query.bindValue(":is_edited", isEdited ? 1 : 0);
    query.bindValue(":id", messageId);
    
    if (!query.exec()) {
        qWarning() << "Failed to update message:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Message" << messageId << "updated successfully";
    return true;
}

bool DatabaseManager::deleteMessage(int messageId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM messages WHERE id = :id");
    
    query.bindValue(":id", messageId);
    
    if (!query.exec()) {
        qWarning() << "Failed to delete message:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "Message" << messageId << "deleted successfully";
    return true;
}
