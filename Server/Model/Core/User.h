#ifndef USER_H
#define USER_H

#include <QString>
#include <QDateTime>

class User
{
public:
    User();
    User(const QString &id, const QString &ipAddress);
    
    QString getId() const { return m_id; }
    QString getIpAddress() const { return m_ipAddress; }
    QDateTime getConnectedAt() const { return m_connectedAt; }
    
    void setId(const QString &id) { m_id = id; }
    void setIpAddress(const QString &ip) { m_ipAddress = ip; }
    
private:
    QString m_id;
    QString m_ipAddress;
    QDateTime m_connectedAt;
};

#endif // USER_H
