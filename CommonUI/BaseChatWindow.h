#ifndef BASECHATWINDOW_H
#define BASECHATWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QListWidgetItem>
#include <QEvent>
#include <QObject>

// Include aliases for backward compatibility
#include "MessageAliases.h"

class BaseChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BaseChatWindow(QWidget *parent = nullptr);
    virtual ~BaseChatWindow();

    // MessageType enum (shared between text and file messages)
    enum MessageType {
        Received,
        Sent
    };

signals:
    void sendMessageRequested(const QString &message);
    // --- FIX: Add serverHost parameter ---
    void fileUploaded(const QString &fileName, const QString &url, qint64 fileSize, const QString &serverHost = "");

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // Helper methods for common functionality
    virtual void handleSendMessage() = 0;
    virtual void handleFileUpload() = 0;
};

#endif // BASECHATWINDOW_H
