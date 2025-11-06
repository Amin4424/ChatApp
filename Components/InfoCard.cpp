#include "InfoCard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar> // <-- ADD THIS

InfoCard::InfoCard(QWidget *parent)
    : QFrame(parent),
      m_cardBgColor(Qt::white),
      m_titleColor(Qt::gray),
      m_titleFont("Arial", 10),
      m_fileNameColor(Qt::black),
      m_fileNameFont("Arial", 12, QFont::Bold),
      m_buttonTextColor(Qt::white),
      m_buttonBgColor(Qt::blue),
      m_buttonBorderRadius(5),
      m_progressBarColor(QColor("#25d366")) // <-- Init progress bar color (e.g., green)
{
    setupUI();
    updateStyles();
}

InfoCard::~InfoCard()
{
    // Qt's parent-child system handles cleanup
}

void InfoCard::setupUI()
{
    // Set object name for styling
    setObjectName("infoCardFrame");

    // Create main layout (QVBoxLayout)
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(8);

    // Title label at the top
    m_titleLabel = new QLabel("کاربر شماره 2");
    m_titleLabel->setObjectName("titleLabel");
    m_mainLayout->addWidget(m_titleLabel);

    // Middle layout (QHBoxLayout)
    m_middleLayout = new QHBoxLayout();
    m_middleLayout->setSpacing(10);

    // File info layout (QVBoxLayout) on the left
    m_fileInfoLayout = new QVBoxLayout();
    m_fileInfoLayout->setSpacing(2);

    m_fileNameLabel = new QLabel("tusd.exe");
    m_fileNameLabel->setObjectName("fileNameLabel");
    m_fileInfoLayout->addWidget(m_fileNameLabel);

    m_fileSizeLabel = new QLabel("57.4 MB");
    m_fileSizeLabel->setObjectName("fileSizeLabel");
    m_fileInfoLayout->addWidget(m_fileSizeLabel);

    m_middleLayout->addLayout(m_fileInfoLayout);

    // Add stretch to push file info to the left
    m_middleLayout->addStretch();

    // Icon label on the right
    m_iconLabel = new QLabel();
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_middleLayout->addWidget(m_iconLabel);

    m_mainLayout->addLayout(m_middleLayout);

    // Action button
    m_actionButton = new QPushButton("Open");
    m_actionButton->setObjectName("actionButton");
    connect(m_actionButton, &QPushButton::clicked, this, &InfoCard::onButtonClicked);
    m_mainLayout->addWidget(m_actionButton);

    // --- NEW: Progress Bar ---
    m_progressBar = new QProgressBar();
    m_progressBar->setObjectName("progressBar");
    m_progressBar->setVisible(false); // Hidden by default
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setRange(0, 100); // Set range for percentage
    m_progressBar->setValue(0); // Start at 0%
    m_mainLayout->addWidget(m_progressBar);
    // --- END NEW ---


    // Set default styling
    setFrameStyle(QFrame::NoFrame); // Use NoFrame, we control border in stylesheet
    setLineWidth(1);
}

void InfoCard::updateStyles()
{
    // Apply card background
    QString cardStyle = QString("QFrame#infoCardFrame { background-color: %1; border: 1px solid #e0e0e0; border-radius: 8px; }")
                        .arg(m_cardBgColor.name());
    this->setStyleSheet(cardStyle); // Set stylesheet on the frame itself

    // Apply title style
    QString titleStyle = QString("color: %1; font-family: %2; font-size: %3pt; background-color: transparent;")
                         .arg(m_titleColor.name(), m_titleFont.family(), QString::number(m_titleFont.pointSize()));
    if (m_titleFont.bold()) titleStyle += " font-weight: bold;";
    m_titleLabel->setStyleSheet(titleStyle);

    // Apply file name style
    QString fileNameStyle = QString("color: %1; font-family: %2; font-size: %3pt; background-color: transparent;")
                            .arg(m_fileNameColor.name(), m_fileNameFont.family(), QString::number(m_fileNameFont.pointSize()));
    if (m_fileNameFont.bold()) fileNameStyle += " font-weight: bold;";
    m_fileNameLabel->setStyleSheet(fileNameStyle);
    
    // Apply file size style
    m_fileSizeLabel->setStyleSheet("background-color: transparent; color: #666;");

    // Apply button style
    QString buttonStyle = QString("background-color: %1; color: %2; border-radius: %3px; padding: 8px 16px; font-weight: bold; border: none;")
                          .arg(m_buttonBgColor.name(), m_buttonTextColor.name(), QString::number(m_buttonBorderRadius));
    m_actionButton->setStyleSheet(buttonStyle);
    
    // --- NEW: Progress Bar Style ---
    QString progressStyle = QString(
        "QProgressBar {"
        "    border: none;"
        "    background-color: #ddd;"
        "    border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: %1;"
        "    border-radius: 3px;"
        "}"
    ).arg(m_progressBarColor.name());
    m_progressBar->setStyleSheet(progressStyle);
    // --- END NEW ---
}

// --- NEW: Implementation ---
InfoCard* InfoCard::showProgressBar(bool show)
{
    m_progressBar->setVisible(show);
    return this;
}

InfoCard* InfoCard::setProgressBarColor(const QColor &color)
{
    m_progressBarColor = color;
    updateStyles(); // Re-apply styles
    return this;
}

void InfoCard::updateProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        m_progressBar->setValue((bytesReceived * 100) / bytesTotal);
    } else {
        m_progressBar->setValue(0);
    }
}
// --- END NEW ---


InfoCard* InfoCard::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
    return this;
}

InfoCard* InfoCard::setFileName(const QString &fileName)
{
    m_fileNameLabel->setText(fileName);
    return this;
}

InfoCard* InfoCard::setFileSize(const QString &fileSize)
{
    m_fileSizeLabel->setText(fileSize);
    return this;
}

InfoCard* InfoCard::setButtonText(const QString &text)
{
    m_actionButton->setText(text);
    return this;
}

InfoCard* InfoCard::setIcon(const QPixmap &pixmap)
{
    m_iconLabel->setPixmap(pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    return this;
}

InfoCard* InfoCard::setCardBackgroundColor(const QColor &color)
{
    m_cardBgColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setTitleColor(const QColor &color)
{
    m_titleColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setTitleFont(const QFont &font)
{
    m_titleFont = font;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setFileNameColor(const QColor &color)
{
    m_fileNameColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setFileNameFont(const QFont &font)
{
    m_fileNameFont = font;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setButtonTextColor(const QColor &color)
{
    m_buttonTextColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setButtonBackgroundColor(const QColor &color)
{
    m_buttonBgColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setButtonBorderRadius(int radius)
{
    m_buttonBorderRadius = radius;
    updateStyles();
    return this;
}

QString InfoCard::title() const
{
    return m_titleLabel->text();
}

QString InfoCard::fileName() const
{
    return m_fileNameLabel->text();
}

QString InfoCard::fileSize() const
{
    return m_fileSizeLabel->text();
}

QString InfoCard::buttonText() const
{
    return m_actionButton->text();
}

InfoCard* InfoCard::setButtonEnabled(bool enabled)
{
    m_actionButton->setEnabled(enabled);
    return this;
}

void InfoCard::onButtonClicked()
{
    emit buttonClicked();
}