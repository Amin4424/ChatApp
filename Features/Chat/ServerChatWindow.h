#ifndef SERVERCHATWINDOW_H
#define SERVERCHATWINDOW_H

#include "BaseChatWindow.h"
#include <QListWidgetItem>
#include <QProgressBar>

QT_BEGIN_NAMESPACE
namespace Ui {
class ServerChatWindow;
}
QT_END_NAMESPACE

class ServerChatWindow : public BaseChatWindow
{
    Q_OBJECT

public:
    ServerChatWindow(QWidget *parent = nullptr);
    ~ServerChatWindow();

signals:
    void userSelected(const QString &userId);
    void restartServerRequested();

protected:
    void handleSendMessage() override;
    void handleFileUpload() override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    void onUserListItemClicked(QListWidgetItem *item);
    void onRestartServerClicked();
    void onSendFileClicked();

public:
    void showMessage(const QString &msg) override;
    void showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &senderInfo) override;
    void updateUserList(const QStringList &users);
    void updateUserCount(int count);
    void setPrivateChatMode(const QString &userId);
    void setBroadcastMode();
    void onBroadcastModeClicked();
    void updateServerInfo(const QString &ip, int port, const QString &status);
    void clearChatHistory();

private:
    Ui::ServerChatWindow *ui;
    bool m_isPrivateChat;
    QString m_currentTargetUser;
    QProgressBar *m_uploadProgressBar;
};

#endif // SERVERCHATWINDOW_H
