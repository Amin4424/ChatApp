#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>

class ServerChatWindow;
class WebSocketServer;
class DatabaseManager;

class ServerController : public QObject
{
    Q_OBJECT

public:
    explicit ServerController(ServerChatWindow *view, WebSocketServer *server, DatabaseManager *db, QObject *parent = nullptr);
    ~ServerController();

    void displayNewConnection();

public slots:
    void displayNewMessages(const QString &message);
    void displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender = "");

private slots:
    void onUserListChanged(const QStringList &users);
    void onUserCountChanged(int count);
    void onUserSelected(const QString &userId);
    void onServerMessageReceived(const QString &message, const QString &senderId);
    void onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "");

private:
    void loadHistoryForServer();
    void loadHistoryForUser(const QString &userId);

    // View
    ServerChatWindow *m_serverView;

    // Model
    WebSocketServer *m_server;
    DatabaseManager *m_db;

    // State
    QString m_currentPrivateTargetUser;
    QString m_currentFilteredUser;
    bool m_isBroadcastMode;
};

#endif // SERVERCONTROLLER_H