#ifndef INFOCARD_H
#define INFOCARD_H

#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QColor>
#include <QFont>
#include <QProgressBar>
#include <QMouseEvent>

class InfoCard : public QFrame
{
    Q_OBJECT

public:
    explicit InfoCard(QWidget *parent = nullptr);
    ~InfoCard();

    // State enum for managing different display modes
    enum class State {
        Idle_Downloadable,     // حالت دانلود (کل کارت قابل کلیک)
        In_Progress_Upload,    // در حال آپلود (نوار پیشرفت، ساعت 🕒)
        In_Progress_Download,  // در حال دانلود (نوار پیشرفت، بدون ساعت)
        Completed_Sent,        // ارسال شده (زمان، تیک ✓)
        Completed_Pending,     // در انتظار ارسال (زمان، ساعت 🕒)
        Completed_Downloaded   // دانلود شده (قابل کلیک برای باز کردن)
    };

    // Fluent API setters (return InfoCard* for method chaining)
    InfoCard* setTitle(const QString &title);
    InfoCard* setFileName(const QString &fileName);
    InfoCard* setFileSize(const QString &fileSize);
    InfoCard* setIcon(const QPixmap &pixmap);
    InfoCard* setTimestamp(const QString &timestamp);

    // Style setters (fluent API)
    InfoCard* setCardBackgroundColor(const QColor &color);
    InfoCard* setTitleColor(const QColor &color);
    InfoCard* setTitleFont(const QFont &font);
    InfoCard* setFileNameColor(const QColor &color);
    InfoCard* setFileNameFont(const QFont &font);

    // State management
    void setState(State newState);

    // Getters (for compatibility)
    QString title() const;
    QString fileName() const;
    QString fileSize() const;

signals:
    // Signal for card click (replaces buttonClicked)
    void cardClicked();
    void cancelClicked();

public slots:
    // Slot to update progress
    void updateProgress(qint64 bytesReceived, qint64 bytesTotal);

protected:
    // Override mouse press event for click detection
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onCancelClicked();

private:
    void setupUI();
    void updateStyles();

    // Sub-widgets
    QLabel *m_titleLabel;
    QLabel *m_fileNameLabel;
    QLabel *m_fileSizeLabel;
    QLabel *m_iconLabel;
    
    // Progress widgets (for in-progress state)
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;        // "25%"
    QToolButton *m_cancelButton;
    QWidget *m_progressWidget;      // Container for progress bar and buttons
    
    // Status widgets (for completed state)
    QLabel *m_timestampLabel;
    QLabel *m_statusIconLabel;      // 🕒 or ✓
    QWidget *m_statusWidget;        // Container for timestamp and status icon

    // Layouts
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_middleLayout;
    QVBoxLayout *m_fileInfoLayout;

    // Style member variables
    QColor m_cardBgColor;
    QColor m_titleColor;
    QFont m_titleFont;
    QColor m_fileNameColor;
    QFont m_fileNameFont;
    
    // Current state
    State m_currentState;
};

#endif // INFOCARD_H