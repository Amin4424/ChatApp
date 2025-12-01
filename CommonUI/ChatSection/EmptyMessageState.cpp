#include "EmptyMessageState.h"
#include <QPixmap>

EmptyMessageState::EmptyMessageState(QWidget *parent)
    : QWidget(parent)
    , m_imageLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_mainLayout(nullptr)
{
    setupUI();
}

EmptyMessageState::~EmptyMessageState()
{
}

void EmptyMessageState::setupUI()
{
    // Main layout - centered vertically and horizontally
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Image label
    m_imageLabel = new QLabel(this);
    QPixmap pixmap("assets/NoMessage.png");
    
    // Scale the image to the specified size (237.19 x 179.99)
    if (!pixmap.isNull()) {
        m_imageLabel->setPixmap(pixmap.scaled(237, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setFixedSize(237, 180);
    
    // Title label - "No messages here yet!"
    m_titleLabel = new QLabel("No messages here yet!", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setFixedSize(204, 22);
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "    font-family: 'Inter';"
        "    font-weight: 600;"
        "    font-size: 18px;"
        "    line-height: 100%;"
        "    letter-spacing: 0%;"
        "    color: #000000;"
        "    background: transparent;"
        "}"
    );
    
    // Subtitle label - "say something"
    m_subtitleLabel = new QLabel("say something", this);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setFixedSize(110, 19);
    m_subtitleLabel->setStyleSheet(
        "QLabel {"
        "    font-family: 'Inter';"
        "    font-weight: 400;"
        "    font-size: 16px;"
        "    line-height: 100%;"
        "    letter-spacing: 0%;"
        "    color: #000000;"
        "    background: transparent;"
        "}"
    );
    
    // Add widgets to layout
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_imageLabel, 0, Qt::AlignCenter);
    m_mainLayout->addSpacing(16);  // Space between image and title
    m_mainLayout->addWidget(m_titleLabel, 0, Qt::AlignCenter);
    m_mainLayout->addSpacing(8);   // Space between title and subtitle
    m_mainLayout->addWidget(m_subtitleLabel, 0, Qt::AlignCenter);
    m_mainLayout->addStretch();
    
    // Set gray background to match chat section
    setStyleSheet("EmptyMessageState { background-color: #f4f5f7; }");
}
