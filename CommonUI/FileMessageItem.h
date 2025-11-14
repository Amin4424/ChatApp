#ifndef FILEMESSAGEITEM_H
#define FILEMESSAGEITEM_H

#include <BaseMessageItem.h>
#include <BaseChatWindow.h>
#include <InfoCard.h>
#include <QString>
#include <QFileIconProvider>

// Forward declaration
class TusDownloader;

class FileMessageItem : public BaseMessageItem
{
    Q_OBJECT

public:
    using MessageType = BaseChatWindow::MessageType;

    explicit FileMessageItem(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                             const QString &senderInfo, MessageType type, 
                             const QString &serverHost = "localhost", QWidget *parent = nullptr);
    ~FileMessageItem();

    // Public methods to control state
    void markAsSent();  // Change from pending (🕒) to sent (✓)
    void setUploadProgress(qint64 bytes, qint64 total);  // Update progress during upload
    bool isSent() const { return m_isSent; }  // NEW: Check if message was sent

protected:
    void setupUI() override;

private slots:
    void onCardClicked();  // NEW: Handle card click
    void downloadFile();
    void openFile();
    
    // Slots for TusDownloader
    void onDownloadFinished(const QString &filePath);
    void onDownloadError(const QString &errorMessage);

private:
    QPixmap getFileIconPixmap(const QString &fileName);
    QString formatSize(qint64 bytes);

    // --- Member variables ---
    InfoCard* m_infoCard;
    TusDownloader *m_tusDownloader;
    
    QString m_fileUrl;
    QString m_fileName;
    qint64 m_fileSize;
    MessageType m_messageType;
    QString m_serverHost;
    bool m_isDownloaded = false;  // NEW: Track download state
    bool m_isSent = false;        // NEW: Track if message was sent (for Completed_Sent state)
};

#endif // FILEMESSAGEITEM_H