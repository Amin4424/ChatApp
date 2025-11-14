#include "InfoCard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QToolButton>
#include <QMouseEvent>

InfoCard::InfoCard(QWidget *parent)
    : QFrame(parent),
      m_cardBgColor(Qt::white),
      m_titleColor(Qt::gray),
      m_titleFont("Arial", 10),
      m_fileNameColor(Qt::black),
      m_fileNameFont("Arial", 12, QFont::Bold),
      m_currentState(State::Idle_Downloadable)
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
    
    // **FIX: عرض ثابت، ارتفاع dynamic**
    setFixedWidth(280);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    // Create main layout (QVBoxLayout)
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(8);

    // Title label at the top
    m_titleLabel = new QLabel("User #2");
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
    // **FIX 2: Ellipsis برای نام‌های طولانی**
    m_fileNameLabel->setWordWrap(false);
    m_fileNameLabel->setMaximumWidth(200);
    QFontMetrics metrics(m_fileNameLabel->font());
    m_fileNameLabel->setText(metrics.elidedText(m_fileNameLabel->text(), Qt::ElideMiddle, 200));
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

    // === Progress Widget (for In-Progress state) ===
    // **FIX: نوار پیشرفت جداگانه AFTER middleLayout**
    m_progressWidget = new QWidget(this);
    QVBoxLayout *progressLayout = new QVBoxLayout(m_progressWidget);
    progressLayout->setContentsMargins(0, 5, 0, 0); // فاصله از بالا
    progressLayout->setSpacing(4);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setObjectName("progressBar");
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    height: 6px;"
        "    text-align: center;"
        "    border: none;"
        "    border-radius: 3px;"
        "    background-color: #e0e0e0;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #34b7f1;"
        "    border-radius: 3px;"
        "}"
    );
    progressLayout->addWidget(m_progressBar);
    
    // Progress controls (percentage label + cancel button)
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);
    
    m_progressLabel = new QLabel("0%");
    m_progressLabel->setStyleSheet("color: #666; font-size: 10pt;");
    controlsLayout->addWidget(m_progressLabel);
    
    controlsLayout->addStretch();
    
    // **FIX: حذف دکمه Pause - فقط Cancel نگه داریم**
    m_cancelButton = new QToolButton();
    m_cancelButton->setText("✕");
    m_cancelButton->setStyleSheet("border: none; font-size: 16pt; color: #f44336; background: transparent; font-weight: bold;");
    m_cancelButton->setToolTip("Cancel Upload");
    connect(m_cancelButton, &QToolButton::clicked, this, &InfoCard::onCancelClicked);
    controlsLayout->addWidget(m_cancelButton);
    
    progressLayout->addLayout(controlsLayout);
    // **FIX 4: نوار پیشرفت دایمی برای تست**
    m_progressWidget->setVisible(true);
    m_progressBar->setValue(45); // مقدار تست
    m_progressLabel->setText("45%");
    m_mainLayout->addWidget(m_progressWidget);

    // === 3. Status Widget (for Completed state) ===
    m_statusWidget = new QWidget(this);
    QHBoxLayout *statusLayout = new QHBoxLayout(m_statusWidget);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(4);
    
    statusLayout->addStretch();
    
    m_timestampLabel = new QLabel("11:38");
    m_timestampLabel->setStyleSheet("color: #999; font-size: 10pt;");
    statusLayout->addWidget(m_timestampLabel);
    
    m_statusIconLabel = new QLabel("✓");
    m_statusIconLabel->setStyleSheet("color: #34b7f1; font-size: 12pt; font-weight: bold;");
    statusLayout->addWidget(m_statusIconLabel);
    
    m_statusWidget->setVisible(false);
    m_mainLayout->addWidget(m_statusWidget);

    // Set default styling
    setFrameStyle(QFrame::NoFrame);
    setLineWidth(1);
    
    // Make the whole card clickable - set cursor to pointing hand
    setCursor(Qt::PointingHandCursor);
}

void InfoCard::updateStyles()
{
    // Apply card background
    QString cardStyle = QString("QFrame#infoCardFrame { background-color: %1; border: 1px solid #e0e0e0; border-radius: 8px; }")
                        .arg(m_cardBgColor.name());
    this->setStyleSheet(cardStyle);

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
}

InfoCard* InfoCard::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
    return this;
}

InfoCard* InfoCard::setFileName(const QString &fileName)
{
    // **FIX 3: اعمال Ellipsis به نام فایل**
    QFontMetrics metrics(m_fileNameLabel->font());
    QString elidedText = metrics.elidedText(fileName, Qt::ElideMiddle, 200);
    m_fileNameLabel->setText(elidedText);
    m_fileNameLabel->setToolTip(fileName); // نمایش نام کامل در tooltip
    return this;
}

InfoCard* InfoCard::setFileSize(const QString &fileSize)
{
    m_fileSizeLabel->setText(fileSize);
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

InfoCard* InfoCard::setTimestamp(const QString &timestamp)
{
    m_timestampLabel->setText(timestamp);
    return this;
}

void InfoCard::setState(State newState)
{
    qDebug() << "🔧 [InfoCard::setState] Called with state:" << (int)newState;
    qDebug() << "🔧 [InfoCard] Current size before state change:" << size();
    qDebug() << "🔧 [InfoCard] SizeHint before state change:" << sizeHint();
    qDebug() << "🔧 [InfoCard] MinimumSizeHint before:" << minimumSizeHint();
    
    m_currentState = newState;
    
    // ابتدا همه چیز را مخفی کن
    m_progressWidget->setVisible(false);
    m_statusWidget->setVisible(false);
    
    qDebug() << "🔧 [InfoCard] progressWidget hidden, statusWidget hidden";

    switch (newState) {
        case State::Idle_Downloadable:
            qDebug() << "🔧 [InfoCard] State: Idle_Downloadable";
            // کارت قابل کلیک است - هیچ ویجت اضافه‌ای نمی‌خواهد
            break;
        
        case State::In_Progress_Upload:
            qDebug() << "🔧 [InfoCard] State: In_Progress_Upload";
            m_statusIconLabel->setText("🕒");
            m_statusIconLabel->setStyleSheet("color: #999; font-size: 12pt;");
            m_statusWidget->setVisible(true);
            m_progressWidget->setVisible(true);
            qDebug() << "🔧 [InfoCard] progressWidget visible, statusWidget visible";
            break;

        case State::In_Progress_Download:
            qDebug() << "🔧 [InfoCard] State: In_Progress_Download";
            m_statusWidget->setVisible(false);
            m_progressWidget->setVisible(true);
            qDebug() << "🔧 [InfoCard] progressWidget visible, statusWidget hidden";
            break;

        case State::Completed_Sent:
            qDebug() << "🔧 [InfoCard] State: Completed_Sent";
            m_statusIconLabel->setText("✓");
            m_statusIconLabel->setStyleSheet("color: #34b7f1; font-size: 12pt; font-weight: bold;");
            m_statusWidget->setVisible(true);
            qDebug() << "🔧 [InfoCard] progressWidget hidden, statusWidget visible";
            break;

        case State::Completed_Pending:
            qDebug() << "🔧 [InfoCard] State: Completed_Pending";
            m_statusIconLabel->setText("🕒");
            m_statusIconLabel->setStyleSheet("color: #999; font-size: 12pt;");
            m_statusWidget->setVisible(true);
            qDebug() << "🔧 [InfoCard] progressWidget hidden, statusWidget visible";
            break;

        case State::Completed_Downloaded:
            qDebug() << "🔧 [InfoCard] State: Completed_Downloaded";
            // **FIX: فایل دریافت شده - نمایش timestamp + تیک**
            m_statusIconLabel->setText("✓");
            m_statusIconLabel->setStyleSheet("color: #34b7f1; font-size: 12pt; font-weight: bold;");
            m_statusWidget->setVisible(true);
            qDebug() << "🔧 [InfoCard] progressWidget hidden, statusWidget visible";
            break;
    }
    
    qDebug() << "🔧 [InfoCard] progressWidget isVisible:" << m_progressWidget->isVisible();
    qDebug() << "🔧 [InfoCard] progressWidget size:" << m_progressWidget->size();
    qDebug() << "🔧 [InfoCard] statusWidget isVisible:" << m_statusWidget->isVisible();
    qDebug() << "🔧 [InfoCard] statusWidget size:" << m_statusWidget->size();
    
    // **FIX: مجبور کردن layout به recalculate**
    m_mainLayout->activate();
    adjustSize();
    
    qDebug() << "🔧 [InfoCard] After adjustSize - size:" << size();
    qDebug() << "🔧 [InfoCard] After adjustSize - sizeHint:" << sizeHint();
    qDebug() << "🔧 [InfoCard] After adjustSize - minimumSizeHint:" << minimumSizeHint();
    
    // **CRITICAL FIX: قفل کردن ارتفاع جدید**
    int newHeight = sizeHint().height();
    setFixedHeight(newHeight);
    qDebug() << "🔧 [InfoCard] Height locked to:" << newHeight;
    
    updateGeometry();
    
    // Force parent widget to relayout
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
        qDebug() << "🔧 [InfoCard] Parent layout activated";
    }
    
    qDebug() << "🔧 [InfoCard] Final size:" << size();
    qDebug() << "========================================";
}

void InfoCard::updateProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percentage = (bytesReceived * 100) / bytesTotal;
        m_progressBar->setValue(percentage);
        m_progressLabel->setText(QString::number(percentage) + "%");
    } else {
        m_progressBar->setValue(0);
        m_progressLabel->setText("0%");
    }
}

void InfoCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit cardClicked();
    }
    QFrame::mousePressEvent(event);
}

void InfoCard::onCancelClicked()
{
    emit cancelClicked();
}