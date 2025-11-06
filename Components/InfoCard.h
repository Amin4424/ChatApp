#ifndef INFOCARD_H
#define INFOCARD_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QColor>
#include <QFont>
#include <QProgressBar> // <-- ADD THIS

class InfoCard : public QFrame
{
    Q_OBJECT

public:
    explicit InfoCard(QWidget *parent = nullptr);
    ~InfoCard();

    // Fluent API setters (return InfoCard* for method chaining)
    InfoCard* setTitle(const QString &title);
    InfoCard* setFileName(const QString &fileName);
    InfoCard* setFileSize(const QString &fileSize);
    InfoCard* setButtonText(const QString &text);
    InfoCard* setIcon(const QPixmap &pixmap);

    // Style setters (fluent API)
    InfoCard* setCardBackgroundColor(const QColor &color);
    InfoCard* setTitleColor(const QColor &color);
    InfoCard* setTitleFont(const QFont &font);
    InfoCard* setFileNameColor(const QColor &color);
    InfoCard* setFileNameFont(const QFont &font);
    InfoCard* setButtonTextColor(const QColor &color);
    InfoCard* setButtonBackgroundColor(const QColor &color);
    InfoCard* setButtonBorderRadius(int radius);

    // --- NEW: Progress Bar functions ---
    InfoCard* showProgressBar(bool show);
    InfoCard* setProgressBarColor(const QColor &color);
    // --- END NEW ---

    // Getters (for compatibility)
    QString title() const;
    QString fileName() const;
    QString fileSize() const;
    QString buttonText() const;
    
    // Button state management
    InfoCard* setButtonEnabled(bool enabled);

signals:
    // Signal for button click
    void buttonClicked();

public slots:
    // --- NEW: Slot to update progress ---
    void updateProgress(qint64 bytesReceived, qint64 bytesTotal);
    // --- END NEW ---

private slots:
    void onButtonClicked();

private:
    void setupUI();
    void updateStyles();

    // Sub-widgets
    QLabel *m_titleLabel;
    QLabel *m_fileNameLabel;
    QLabel *m_fileSizeLabel;
    QLabel *m_iconLabel;
    QPushButton *m_actionButton;
    QProgressBar *m_progressBar; // <-- ADD THIS

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
    QColor m_buttonTextColor;
    QColor m_buttonBgColor;
    int m_buttonBorderRadius;
    QColor m_progressBarColor; // <-- ADD THIS
};

#endif // INFOCARD_H