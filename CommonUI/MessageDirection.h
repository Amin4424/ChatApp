#ifndef MESSAGEDIRECTION_H
#define MESSAGEDIRECTION_H

enum class MessageDirection {
    Incoming,
    Outgoing
};

inline bool isOutgoing(MessageDirection direction)
{
    return direction == MessageDirection::Outgoing;
}

#endif // MESSAGEDIRECTION_H
