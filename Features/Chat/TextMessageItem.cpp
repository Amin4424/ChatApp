#include "TextMessageItem.h"
#include "ui_TextMessageItem.h" // We still use ui for the labels
#include <QChar>
#include <QHBoxLayout>   // <-- NEW
#include <QVBoxLayout>   // <-- NEW
#include <QSpacerItem>  // <-- NEW

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
    // --- START RE-LAYOUT ---

    // 1. Delete the default layout from the .ui file
    // We only want the widgets (ui->senderLabel, ui->messageLabel)
    delete layout();

    // 2. Create new layouts
    QHBoxLayout *mainLayout = new QHBoxLayout(this); // Main layout for the whole item
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);

    QWidget *bubbleWidget = new QWidget(); // This widget will BE the bubble
    QVBoxLayout *bubbleLayout = new QVBoxLayout(bubbleWidget); // Layout *inside* the bubble
    bubbleLayout->setContentsMargins(8, 8, 8, 8); // Padding *inside* the bubble
    bubbleLayout->setSpacing(2);

    // 3. Configure and add labels to the bubble layout

    // --- Sender Label ---
    if (!m_senderInfo.isEmpty()) {
        ui->senderLabel->setText(m_senderInfo);
        ui->senderLabel->setVisible(true);
    } else {
        ui->senderLabel->setVisible(false);
    }
    bubbleLayout->addWidget(ui->senderLabel);

    // --- Message Label ---
    ui->messageLabel->setText(m_messageText);
    ui->messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->messageLabel->setWordWrap(true); // Enable word wrap

    // --- Dynamic Alignment (RTL/LTR) ---
    Qt::AlignmentFlag hAlign = Qt::AlignLeft;
    if (!m_messageText.isEmpty()) {
        QChar firstChar = m_messageText.at(0);
        // --- FIX: Compile Error (was Script_Arabic) ---
        if (firstChar.script() == QChar::Script_Arabic) {
            hAlign = Qt::AlignRight;
        }
    }
    ui->messageLabel->setAlignment(hAlign | Qt::AlignVCenter);

    // Remove default stylesheet from the label itself
    ui->messageLabel->setStyleSheet("background-color: transparent; border: none; padding: 0px; color: #000000;"); // Black text
    bubbleLayout->addWidget(ui->messageLabel);

    // 4. Configure bubble style and add to main layout

    // Make the bubble only take the space it needs
    bubbleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    // Set a max width for the bubble itself
    // We set a fixed max width for wrapping calculations
    bubbleWidget->setMaximumWidth(450);


    if (m_messageType == MessageType::Sent) {
        // --- SENT (Telegram Style) - LOCATE ON THE RIGHT ---
        // Green bubble
        bubbleWidget->setStyleSheet(
            "QWidget {"
            "    background-color: #a0e97e;"
            "    border: 1px solid #8dd06a;"
            "    border-radius: 10px;"
            "}"
            );
        // Sender label text color
        ui->senderLabel->setStyleSheet("color: #666666; font-weight: bold; background-color: transparent;");
        ui->senderLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // In RTL layout, add Bubble *then* Spacer to align RIGHT
        mainLayout->addWidget(bubbleWidget);
        mainLayout->addStretch(1); // Spacer on the left


    } else {
        // --- RECEIVED (Telegram Style) - LOCATE ON THE LEFT ---
        // White bubble
        bubbleWidget->setStyleSheet(
            "QWidget {"
            "    background-color: #ffffff;"
            "    border: 1px solid #eeeeee;"
            "    border-radius: 10px;"
            "}"
            );
        // Sender label text color
        ui->senderLabel->setStyleSheet("color: #075e54; font-weight: bold; background-color: transparent;");
        // Align sender based on text direction
        ui->senderLabel->setAlignment(hAlign | Qt::AlignVCenter);

        // In RTL layout, add Spacer *then* Bubble to align LEFT
        mainLayout->addStretch(1); // Spacer on the right
        mainLayout->addWidget(bubbleWidget);
    }

    // --- No adjustSize() needed on the root ---
    // The layout system will handle the height automatically
    // because the bubbleWidget has QSizePolicy::Preferred for vertical.
    // The QListWidgetItem still needs its sizeHint set in the window .cpp files.
}

