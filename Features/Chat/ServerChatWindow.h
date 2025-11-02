#ifndef SERVERCHATWINDOW_H
#define SERVERCHATWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class ServerChatWindow;
}
QT_END_NAMESPACE

class ServerChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    ServerChatWindow(QWidget *parent = nullptr);
    ~ServerChatWindow();
    
signals:
    void sendMessageRequested(const QString &message);
    void userSelected(const QString &userId);
    void restartServerRequested();
    
private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    void onUserListItemClicked(QListWidgetItem *item);
    void onRestartServerClicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public:
    void showMessage(QString msg);
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
};

#endif // SERVERCHATWINDOW_H
