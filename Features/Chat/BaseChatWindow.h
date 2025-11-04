#ifndef BASECHATWINDOW_H
#define BASECHATWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QListWidgetItem>
#include <QEvent>
#include <QObject>

class BaseChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BaseChatWindow(QWidget *parent = nullptr);
    virtual ~BaseChatWindow();

    // Common interface methods
    virtual void showMessage(const QString &msg) = 0;
    virtual void showFileMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl, const QString &senderInfo) = 0;

signals:
    void sendMessageRequested(const QString &message);
    void fileUploaded(const QString &fileName, const QString &url, qint64 fileSize);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // Helper methods for common functionality
    virtual void handleSendMessage() = 0;
    virtual void handleFileUpload() = 0;
};

#endif // BASECHATWINDOW_H
