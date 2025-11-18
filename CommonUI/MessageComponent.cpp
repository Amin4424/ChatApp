#include "MessageComponent.h"

MessageComponent::MessageComponent(const QString &senderInfo,
                                   MessageDirection direction,
                                   QWidget *parent)
    : QWidget(parent)
    , m_senderInfo(senderInfo)
    , m_direction(direction)
{
}

MessageComponent::~MessageComponent() = default;

MessageComponent &MessageComponent::withSender(const QString &senderInfo)
{
    m_senderInfo = senderInfo;
    return *this;
}

MessageComponent &MessageComponent::withTimestamp(const QString &timestamp)
{
    m_timestamp = timestamp;
    return *this;
}

MessageComponent &MessageComponent::withDirection(MessageDirection direction)
{
    m_direction = direction;
    return *this;
}
