#ifndef CLIENTCHATWINDOW_H
#define CLIENTCHATWINDOW_H

#include "BaseChatWindow.h"
#include <QProgressBar>
#include <QListWidgetItem>
#include "TextMessageItem.h" // <-- *** ADD THIS INCLUDE ***

QT_BEGIN_NAMESPACE
namespace Ui { class ClientChatWindow; }
QT_END_NAMESPACE

class ClientChatWindow : public BaseChatWindow
{
    Q_OBJECT

public:
    ClientChatWindow(QWidget *parent = nullptr);
    ~ClientChatWindow();

    void showMessage(const QString &msg) override;
    
    // --- ADD: Set server host ---
    void setServerHost(const QString &host) { m_serverHost = host; }

    // Override the pure virtual function from BaseChatWindow
    void showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                         const QString &senderInfo, BaseChatWindow::MessageType type) override;

    void updateConnectionInfo(const QString &serverUrl, const QString &status);

signals:
    void reconnectRequested();

protected:
    void handleSendMessage() override;
    void handleFileUpload() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    void onSendFileClicked();
    void onReconnectClicked();

private:
    Ui::ClientChatWindow *ui;
    QProgressBar *m_uploadProgressBar;
    QString m_serverHost; // --- ADD: Store server host
};

#endif // CLIENTCHATWINDOW_H
