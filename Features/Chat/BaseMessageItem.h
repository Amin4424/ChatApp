#ifndef BASEMESSAGEITEM_H
#define BASEMESSAGEITEM_H

#include <QWidget>
#include <QString>

class BaseMessageItem : public QWidget
{
    Q_OBJECT

public:
    explicit BaseMessageItem(const QString &senderInfo, QWidget *parent = nullptr);
    virtual ~BaseMessageItem();

protected:
    // Pure virtual function that derived classes must implement
    virtual void setupUI() = 0;
    
    // Common data
    QString m_senderInfo;
};

#endif // BASEMESSAGEITEM_H