#include "ChatInputWidget.h"
#include <QIcon>
#include <QStyle>

ChatInputWidget::ChatInputWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

ChatInputWidget::~ChatInputWidget()
{
}

void ChatInputWidget::setupUi()
{
    // Main Layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Smiley Button
    m_smileyBtn = new QPushButton(this);
    m_smileyBtn->setText("☺"); 
    m_smileyBtn->setFixedSize(40, 40);
    m_smileyBtn->setFlat(true);
    m_smileyBtn->setCursor(Qt::PointingHandCursor);
    m_smileyBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #8e8e93; background: transparent; } QPushButton:hover { color: #007aff; }");
    mainLayout->addWidget(m_smileyBtn);

    // Input Container
    m_inputContainer = new QFrame(this);
    m_inputContainer->setObjectName("inputContainer");
    m_inputContainer->setStyleSheet(
        "QFrame#inputContainer {"
        "    background-color: #ffffff;"
        "    border: 1px solid #e5e5ea;"
        "    border-radius: 20px;"
        "}"
    );
    
    QHBoxLayout *containerLayout = new QHBoxLayout(m_inputContainer);
    containerLayout->setContentsMargins(15, 5, 10, 5);
    containerLayout->setSpacing(5);
    mainLayout->addWidget(m_inputContainer);

    // Text Edit
    m_textEdit = new QTextEdit(m_inputContainer);
    m_textEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #000000;"
        "    font-size: 14px;"
        "    padding-top: 12px;"
        "}"
    );
    m_textEdit->setFixedHeight(50);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->setPlaceholderText("Write message...");
    containerLayout->addWidget(m_textEdit);

    connect(m_textEdit, &QTextEdit::textChanged, this, &ChatInputWidget::onTextChanged);

    // Attach Button
    m_attachBtn = new QPushButton(m_inputContainer);
    QIcon attachIcon("assets/attach.svg"); 
    m_attachBtn->setIcon(attachIcon);
    m_attachBtn->setIconSize(QSize(24, 24));
    m_attachBtn->setFixedSize(32, 32);
    m_attachBtn->setFlat(true);
    m_attachBtn->setCursor(Qt::PointingHandCursor);
    m_attachBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: black; }");
    containerLayout->addWidget(m_attachBtn);

    connect(m_attachBtn, &QPushButton::clicked, this, &ChatInputWidget::sendFileClicked);

    // Mic Button
    m_micBtn = new QPushButton(m_inputContainer);
    QIcon micIcon("assets/mic.svg");
    m_micBtn->setIcon(micIcon);
    m_micBtn->setIconSize(QSize(24, 24));
    m_micBtn->setFixedSize(32, 32);
    m_micBtn->setFlat(true);
    m_micBtn->setCursor(Qt::PointingHandCursor);
    m_micBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: black; }");
    containerLayout->addWidget(m_micBtn);

    connect(m_micBtn, &QPushButton::clicked, this, &ChatInputWidget::recordVoiceClicked);

    // Send Button
    m_sendBtn = new QPushButton(this);
    m_sendBtn->setText("Send");
    m_sendBtn->setFixedSize(80, 40);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #007aff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 20px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover { background-color: #0062cc; }"
        "QPushButton:pressed { background-color: #004999; }"
        "QPushButton:disabled { background-color: #b3d7ff; color: white; }"
    );
    mainLayout->addWidget(m_sendBtn);

    connect(m_sendBtn, &QPushButton::clicked, this, &ChatInputWidget::sendMessageClicked);
}

QString ChatInputWidget::getMessageText() const
{
    return m_textEdit->toPlainText();
}

void ChatInputWidget::clearMessageText()
{
    m_textEdit->clear();
}

void ChatInputWidget::setPlaceholderText(const QString &text)
{
    m_textEdit->setPlaceholderText(text);
}

QTextEdit* ChatInputWidget::textEdit() const
{
    return m_textEdit;
}

QPushButton* ChatInputWidget::sendButton() const
{
    return m_sendBtn;
}

QPushButton* ChatInputWidget::attachButton() const
{
    return m_attachBtn;
}

QPushButton* ChatInputWidget::micButton() const
{
    return m_micBtn;
}

void ChatInputWidget::onTextChanged()
{
    emit textChanged();
}
