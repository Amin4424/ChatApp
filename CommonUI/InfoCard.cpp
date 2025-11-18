#include "InfoCard.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QToolButton>
#include <QMouseEvent>

InfoCard::InfoCard(QWidget *parent)
    : QFrame(parent),
      m_cardBgColor(Qt::white),
      m_customBackgroundStyle(""),
      m_useCustomBackground(false),
      m_cornerRadius(12),
      m_titleColor(Qt::gray),
      m_titleFont("Arial", 10),
      m_fileNameColor(Qt::black),
      m_fileNameFont("Arial", 12, QFont::Bold),
      m_fileSizeColor(QColor("#666666")),
      m_timestampColor(QColor("#999999")),
      m_isOutgoing(true),
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
    
    // **FIX: عرض و ارتفاع ثابت برای تمام فایل‌ها**
    setFixedWidth(280);  // عرض ثابت 280 پیکسل
    setFixedHeight(123); // ارتفاع ثابت 123 پیکسل (بزرگترین حالت)
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

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
    m_progressWidget->setFixedHeight(50); // ارتفاع ثابت
    QVBoxLayout *progressLayout = new QVBoxLayout(m_progressWidget);
    progressLayout->setContentsMargins(0, 5, 0, 0);
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
        "    background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7bb6ff, stop:1 #3470ff);"
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
    m_statusWidget->setFixedHeight(30); // ارتفاع ثابت
    QHBoxLayout *statusLayout = new QHBoxLayout(m_statusWidget);
    statusLayout->setContentsMargins(0, 5, 0, 0); // همون margin با progressWidget
    statusLayout->setSpacing(4);
    
    statusLayout->addStretch();
    
    m_timestampLabel = new QLabel("");
    m_timestampLabel->setStyleSheet("color: #999; font-size: 10pt;");
    statusLayout->addWidget(m_timestampLabel);
    
    m_statusIconLabel = new QLabel("✓");
    m_statusIconLabel->setStyleSheet("color: #1f6bff; font-size: 12pt; font-weight: bold;");
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
    QString cardStyle;
    if (m_useCustomBackground && !m_customBackgroundStyle.isEmpty()) {
        cardStyle = QString("QFrame#infoCardFrame { %1 border-radius: %2px; }")
                        .arg(m_customBackgroundStyle)
                        .arg(m_cornerRadius);
    } else {
        cardStyle = QString("QFrame#infoCardFrame { background-color: %1; border: 1px solid #e0e0e0; border-radius: %2px; }")
                        .arg(m_cardBgColor.name())
                        .arg(m_cornerRadius);
    }
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
    m_fileSizeLabel->setStyleSheet(QString("background-color: transparent; color: %1;").arg(m_fileSizeColor.name()));
    m_timestampLabel->setStyleSheet(QString("color: %1; font-size: 10pt;").arg(m_timestampColor.name()));
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
    m_useCustomBackground = false;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setCardBackgroundGradient(const QString &style)
{
    m_customBackgroundStyle = style;
    m_useCustomBackground = true;
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

InfoCard* InfoCard::setFileSizeColor(const QColor &color)
{
    m_fileSizeColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setTimestampColor(const QColor &color)
{
    m_timestampColor = color;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setCornerRadius(int radius)
{
    m_cornerRadius = radius;
    updateStyles();
    return this;
}

InfoCard* InfoCard::setMessageDirection(bool isOutgoing)
{
    m_isOutgoing = isOutgoing;
    setState(m_currentState);
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
    m_currentState = newState;
    
    // ابتدا همه چیز را مخفی کن
    m_progressWidget->setVisible(false);
    m_statusWidget->setVisible(false);

    switch (newState) {
        case State::Idle_Downloadable:
            // نمایش تایم‌استمپ برای فایل‌های دریافت شده که هنوز دانلود نشده‌اند
            m_statusIconLabel->setText("⬇");
            m_statusIconLabel->setStyleSheet(QString("color: %1; font-size: 12pt;")
                                             .arg(m_isOutgoing ? "#8a94a6" : "#e7f1ff"));
            m_statusWidget->setVisible(true);
            break;
        
        case State::In_Progress_Upload:
            m_statusIconLabel->setText("🕒");
            m_statusIconLabel->setStyleSheet(QString("color: %1; font-size: 12pt;")
                                             .arg(m_isOutgoing ? "#8a94a6" : "#f5f7ff"));
            m_statusWidget->setVisible(true);
            m_progressWidget->setVisible(true);
            break;

        case State::In_Progress_Download:
            m_statusWidget->setVisible(false);
            m_progressWidget->setVisible(true);
            break;

        case State::Completed_Sent:
            m_statusIconLabel->setText("✓");
            m_statusIconLabel->setStyleSheet(QString("color: %1; font-size: 12pt; font-weight: bold;")
                                             .arg(m_isOutgoing ? "#1f6bff" : "#ffffff"));
            m_statusWidget->setVisible(true);
            break;

        case State::Completed_Pending:
            m_statusIconLabel->setText("🕒");
            m_statusIconLabel->setStyleSheet(QString("color: %1; font-size: 12pt;")
                                             .arg(m_isOutgoing ? "#8a94a6" : "#f5f7ff"));
            m_statusWidget->setVisible(true);
            break;

        case State::Completed_Downloaded:
            m_statusIconLabel->setText("✓");
            m_statusIconLabel->setStyleSheet(QString("color: %1; font-size: 12pt; font-weight: bold;")
                                             .arg(m_isOutgoing ? "#1f6bff" : "#ffffff"));
            m_statusWidget->setVisible(true);
            break;
    }
    
    
    qDebug() << "� [InfoCard] State:" << (int)newState << "| Size:" << size() << "| isOutgoing:" << m_isOutgoing;
    
    updateGeometry();
    
    // Force parent widget to relayout
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
    }
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
