// Change MAINWINDOW_H to CHATWINDOW_H
#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class ChatWindow; }
QT_END_NAMESPACE

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow();

    void showMessage(const QString &msg);
    void showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &senderInfo);
    void updateConnectionInfo(const QString &serverUrl, const QString &status);

signals:
    void sendMessageRequested(const QString &message);
    void fileUploaded(const QString &fileName, const QString &url, qint64 fileSize);
    void reconnectRequested();

private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    void onSendFileClicked();
    void onReconnectClicked();

private:
    bool eventFilter(QObject *obj, QEvent *event);

    Ui::ChatWindow *ui;
    QProgressBar *m_uploadProgressBar;
};

#endif // CHATWINDOW_H
