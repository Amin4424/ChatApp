#ifndef TEXTMESSAGEITEM_H
#define TEXTMESSAGEITEM_H

#include <BaseMessageItem.h>
#include <BaseChatWindow.h>
#include <MessageBubble.h>
#include <QString>

class TextMessageItem : public BaseMessageItem
{
    Q_OBJECT

public:
    // Use the MessageType from BaseChatWindow
    using MessageType = BaseChatWindow::MessageType;

    explicit TextMessageItem(const QString &messageText, const QString &senderInfo = "", MessageType type = MessageType::Received, const QString &timestamp = "", QWidget *parent = nullptr);
    ~TextMessageItem();

protected:
    void setupUI() override;
    void applyStyles();  // New method to apply styling to the MessageBubble

private:
    MessageBubble *m_bubble;
    QString m_messageText;
    MessageType m_messageType;
    QString m_timestamp;
};

#endif // TEXTMESSAGEITEM_H
