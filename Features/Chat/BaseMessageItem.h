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
    QString m_senderInfo;
    
    // Virtual method for subclasses to implement their specific UI setup
    virtual void setupUI() = 0;
};

#endif // BASEMESSAGEITEM_H
