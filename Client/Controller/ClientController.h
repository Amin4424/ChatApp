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
class TextMessage;
class FileMessage;
class AudioMessage;
using TextMessageItem = TextMessage;
using FileMessageItem = FileMessage;
using VoiceMessageItem = AudioMessage;
class MessageComponent;

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
    void onFileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "", const QVector<qreal> &waveform = QVector<qreal>());
    void onTextMessageCopyRequested(const QString &text);
    void onTextMessageEditRequested(TextMessageItem *item);
    void onTextMessageDeleteRequested(TextMessageItem *item);
    void onFileMessageDeleteRequested(FileMessageItem *item);
    void onVoiceMessageDeleteRequested(VoiceMessageItem *item);
    void onTextMessageEditConfirmed(TextMessageItem *item, const QString &newText);

private:
    void loadHistory();
    QWidget* createWidgetFromData(const MessageData &msgData);
    void setupTextMessageItem(TextMessageItem *item);
    void setupFileMessageItem(FileMessageItem *item);
    void setupVoiceMessageItem(VoiceMessageItem *item);
    QStringList splitMessageIntoChunks(const QString &text) const;
    enum class DeleteRequestScope {
        Prompt,
        MeOnly,
        BothSides
    };
    void handleDeleteRequest(MessageComponent *item, DeleteRequestScope scope);

    ClientChatWindow *m_clientView;
    WebSocketClient *m_client;
    DatabaseManager *m_db;
    QString m_clientUserId;
    QString m_serverHost;
};

#endif // CLIENTCONTROLLER_H
