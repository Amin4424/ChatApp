#ifndef FILEMESSAGEITEM_H
#define FILEMESSAGEITEM_H

#include "BaseMessageItem.h"
#include "BaseChatWindow.h"
#include "../FileTransfer/TusDownloader.h"
#include "../../Components/InfoCard.h" // <-- Include the new component
#include <QString>
#include <QFileIconProvider> // <-- Include for icon provider

class FileMessageItem : public BaseMessageItem
{
    Q_OBJECT

public:
    using MessageType = BaseChatWindow::MessageType;

    explicit FileMessageItem(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                             const QString &senderInfo, MessageType type, 
                             const QString &serverHost = "localhost", QWidget *parent = nullptr);
    ~FileMessageItem();

protected:
    void setupUI() override;

private slots:
    void downloadFile();
    void openFile();
    
    // Slots for TusDownloader
    void onDownloadFinished(const QString &filePath);
    void onDownloadError(const QString &errorMessage);

private:
    QPixmap getFileIconPixmap(const QString &fileName); // Changed parameter
    QString formatSize(qint64 bytes);

    // --- Member variables ---
    InfoCard* m_infoCard; // <-- Use InfoCard as the UI
    TusDownloader *m_tusDownloader;
    
    QString m_fileUrl;
    QString m_fileName;
    qint64 m_fileSize;
    MessageType m_messageType;
    QString m_serverHost; 
};

#endif // FILEMESSAGEITEM_H