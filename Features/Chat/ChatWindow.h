// Change MAINWINDOW_H to CHATWINDOW_H
#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class ChatWindow;
}
QT_END_NAMESPACE

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow();
signals:
    void sendMessageRequested(const QString &message);
    void reconnectRequested();
    
private slots:
    void onsendMessageBtnclicked();
    void onchatHistoryWdgtitemClicked(QListWidgetItem *item);
    void onReconnectClicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::ChatWindow *ui;

public:
    void showMessage(QString msg);
    void updateConnectionInfo(const QString &serverUrl, const QString &status);
};

#endif
