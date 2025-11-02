#include "User.h"

User::User()
    : m_connectedAt(QDateTime::currentDateTime())
{
}

User::User(const QString &id, const QString &ipAddress)
    : m_id(id)
    , m_ipAddress(ipAddress)
    , m_connectedAt(QDateTime::currentDateTime())
{
}
