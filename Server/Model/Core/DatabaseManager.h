#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString &dbPath, QObject *parent = nullptr);
    ~DatabaseManager();
    
    bool initDatabase();
    
    // Message operations
    int saveMessage(const QString &sender, const QString &receiver, const QString &message, const QDateTime &timestamp, bool isEdited = false);
    QStringList loadAllMessages();
    QStringList loadMessagesBetween(const QString &user1, const QString &user2);
    bool updateMessage(int messageId, const QString &newMessage, bool isEdited = false);
    bool deleteMessage(int messageId);
    
    // Clear operations
    bool clearAllMessages();
    
private:
    bool createTables();
    bool ensureIsEditedColumn();
    
    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // DATABASEMANAGER_H
