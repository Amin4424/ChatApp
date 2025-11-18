#ifndef MESSAGECOMPONENT_H
#define MESSAGECOMPONENT_H

#include <QWidget>
#include <QString>

#include "MessageDirection.h"

class MessageComponent : public QWidget
{
    Q_OBJECT

public:
    explicit MessageComponent(const QString &senderInfo = QString(),
                              MessageDirection direction = MessageDirection::Incoming,
                              QWidget *parent = nullptr);
    virtual ~MessageComponent();

    MessageComponent &withSender(const QString &senderInfo);
    MessageComponent &withTimestamp(const QString &timestamp);
    MessageComponent &withDirection(MessageDirection direction);

    const QString &senderInfo() const { return m_senderInfo; }
    const QString &timestamp() const { return m_timestamp; }
    MessageDirection direction() const { return m_direction; }
    virtual void setDatabaseId(int id) { m_databaseId = id; }
    int databaseId() const { return m_databaseId; }

protected:
    // Pure virtual function that derived classes must implement
    virtual void setupUI() = 0;
    
    // Common data
    QString m_senderInfo;
    QString m_timestamp;
    MessageDirection m_direction;
    int m_databaseId = -1;
};

#endif // MESSAGECOMPONENT_H
