#ifndef TEXTMESSAGEITEM_H
#define TEXTMESSAGEITEM_H

#include "BaseMessageItem.h"
#include "BaseChatWindow.h"
#include <QString>

namespace Ui {
class TextMessageItem;
}

class TextMessageItem : public BaseMessageItem
{
    Q_OBJECT

public:
    // Use the MessageType from BaseChatWindow
    using MessageType = BaseChatWindow::MessageType;

    explicit TextMessageItem(const QString &messageText, const QString &senderInfo = "", MessageType type = MessageType::Received, QWidget *parent = nullptr);
    ~TextMessageItem();

protected:
    void setupUI() override;

private:
    Ui::TextMessageItem *ui;
    QString m_messageText;
    MessageType m_messageType;
};

#endif // TEXTMESSAGEITEM_H
