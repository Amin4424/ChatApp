#ifndef ATTACHMENTMESSAGE_H
#define ATTACHMENTMESSAGE_H

#include "MessageComponent.h"
#include <QPixmap>
#include <QFileIconProvider>

// Forward declaration to avoid heavy includes
class TusDownloader;

class AttachmentMessage : public MessageComponent
{
    Q_OBJECT
public:
    explicit AttachmentMessage(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                               const QString &senderInfo, MessageDirection direction,
                               const QString &serverHost, QWidget *parent = nullptr);
    virtual ~AttachmentMessage();

    // Public method to set/update file URL after upload completes
    void setFileUrl(const QString &fileUrl);

protected:
    // Pure virtual methods - children must implement their own UI
    virtual void setupUI() = 0;
    
    // Virtual methods for UI state management
    virtual void setIdleState() = 0;      // e.g., show "Download" or "Play" button
    virtual void setInProgressState(qint64 bytes, qint64 total) = 0; // e.g., show ProgressBar
    virtual void setCompletedState() = 0;  // e.g., show "Open" button
    virtual void onClicked() = 0;

protected slots:
    virtual void onDownloadFinished(const QString &filePath);
    virtual void onDownloadError(const QString &errorMessage);

protected:
    // Common helper methods
    QString formatSize(qint64 bytes);
    QPixmap getFileIconPixmap(const QString &fileName);
    void startDownload();
    void openFile(const QString& customPath = ""); // Custom path for voice files

    // Common members
    TusDownloader *m_tusDownloader;
    QString m_fileUrl;
    QString m_fileName;
    qint64 m_fileSize;
    MessageDirection m_direction;
    QString m_serverHost;
    bool m_isDownloaded;
    QString m_localFilePath; // Path to downloaded file
};

#endif // ATTACHMENTMESSAGE_H
