#ifndef CLIENTCONTROLLER_H
#define CLIENTCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

class ClientChatWindow;
class WebSocketClient;
class DatabaseManager;
class MessageData;

class ClientController : public QObject
{
    Q_OBJECT

public:
    explicit ClientController(ClientChatWindow *view, WebSocketClient *client, DatabaseManager *db, QObject *parent = nullptr);
    ~ClientController();

    void displayNewConnection();
    void setServerHost(const QString &host) { m_serverHost = host; }

public slots:
    void displayNewMessages(const QString &message);
    void displayFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &sender = "", const QString &serverHost = "");

private slots:
    void onMessageReceived(const QString &message);
    void onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "");

private:
    void loadHistory();
    QWidget* createWidgetFromData(const MessageData &msgData);

    ClientChatWindow *m_clientView;
    WebSocketClient *m_client;
    DatabaseManager *m_db;
    QString m_clientUserId;
    QString m_serverHost;
};

#endif // CLIENTCONTROLLER_H
