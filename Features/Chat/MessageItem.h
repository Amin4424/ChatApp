#ifndef MESSAGEITEM_H
#define MESSAGEITEM_H

#include <QWidget>
#include <QString>

class QLabel;

class MessageItem : public QWidget
{
    Q_OBJECT

public:
    explicit MessageItem(const QString &message, int messageId, QWidget *parent = nullptr);
    QString getMessage() const { return m_fullMessage; }
    int getMessageId() const { return m_messageId; }

private:
    void setupUI();

    QString m_fullMessage;
    int m_messageId;
    
    QLabel *m_messageLabel;
};

#endif // MESSAGEITEM_H
