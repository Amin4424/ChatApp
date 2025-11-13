#include "BaseMessageItem.h"

BaseMessageItem::BaseMessageItem(const QString &senderInfo, QWidget *parent)
    : QWidget(parent)
    , m_senderInfo(senderInfo)
{
}

BaseMessageItem::~BaseMessageItem()
{
}
