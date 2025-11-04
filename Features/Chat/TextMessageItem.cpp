#include "TextMessageItem.h"
#include "ui_TextMessageItem.h"

TextMessageItem::TextMessageItem(const QString &messageText, const QString &senderInfo, MessageType type, QWidget *parent)
    : BaseMessageItem(senderInfo, parent)
    , ui(new Ui::TextMessageItem)
    , m_messageText(messageText)
    , m_messageType(type)
{
    ui->setupUi(this);
    setupUI();
}

TextMessageItem::~TextMessageItem()
{
    delete ui;
}

void TextMessageItem::setupUI()
{
    // Set sender info (if provided, otherwise hide the label)
    if (!m_senderInfo.isEmpty()) {
        ui->senderLabel->setText(m_senderInfo);
        ui->senderLabel->setVisible(true);
    } else {
        ui->senderLabel->setVisible(false);
    }

    // Set message text
    ui->messageLabel->setText(m_messageText);
    ui->messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // Configure layout and styling based on message type
    if (m_messageType == Sent) {
        // For sent messages: align to the right
        ui->verticalLayout->setAlignment(Qt::AlignRight);
        ui->senderLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->messageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Update message label styling for sent messages
        ui->messageLabel->setStyleSheet(
            "QLabel {"
            "    background-color: #4caf50;"
            "    border: 1px solid #388e3c;"
            "    border-radius: 12px;"
            "    padding: 8px 12px;"
            "    color: white;"
            "    margin-left: 50px;"
            "    margin-right: 0px;"
            "}"
        );
    } else {
        // For received messages: align to the left (default)
        ui->verticalLayout->setAlignment(Qt::AlignLeft);
        ui->senderLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Keep the default styling for received messages
        ui->messageLabel->setStyleSheet(
            "QLabel {"
            "    background-color: #e3f2fd;"
            "    border: 1px solid #bbdefb;"
            "    border-radius: 12px;"
            "    padding: 8px 12px;"
            "    color: #1565c0;"
            "    margin-left: 0px;"
            "    margin-right: 50px;"
            "}"
        );
    }

    // Adjust widget size to fit content
    adjustSize();
}