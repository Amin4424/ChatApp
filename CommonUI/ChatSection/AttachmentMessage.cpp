#include "AttachmentMessage.h"
#include "TusDownloader.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QPainter>
#include <QUrl>

AttachmentMessage::AttachmentMessage(const QString &fileName,
                                     qint64 fileSize,
                                     const QString &fileUrl,
                                     const QString &senderInfo,
                                     MessageDirection direction,
                                     const QString &serverHost,
                                     QWidget *parent)
    : MessageComponent(senderInfo, direction, parent)
    , m_tusDownloader(new TusDownloader(this))
    , m_fileUrl(fileUrl)
    , m_fileName(fileName)
    , m_fileSize(fileSize)
    , m_direction(direction)
    , m_serverHost(serverHost)
    , m_isDownloaded(false)
{
    connect(m_tusDownloader, &TusDownloader::finished, this, &AttachmentMessage::onDownloadFinished);
    connect(m_tusDownloader, &TusDownloader::error, this, &AttachmentMessage::onDownloadError);
}

AttachmentMessage::~AttachmentMessage() = default;

void AttachmentMessage::setFileUrl(const QString &fileUrl)
{
    m_fileUrl = fileUrl;
}

void AttachmentMessage::startDownload()
{
    if (m_fileUrl.isEmpty()) {
        return;
    }

    const QString downloadDir = QCoreApplication::applicationDirPath() + "/client_downloads";
    QDir dir(downloadDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_localFilePath = downloadDir + "/" + m_fileName;

    if (QFile::exists(m_localFilePath)) {
        QFile::remove(m_localFilePath);
    }

    QString downloadUrlString = m_fileUrl;

    if (!downloadUrlString.startsWith("http://") && !downloadUrlString.startsWith("https://")) {

        const QString host = m_serverHost.isEmpty() ? "localhost" : m_serverHost;

        if (downloadUrlString.startsWith("/")) {
            downloadUrlString = downloadUrlString.mid(1);
        }

        downloadUrlString = QString("http://%1:1080/%2").arg(host, downloadUrlString);
    } else if (downloadUrlString.contains("localhost") && !m_serverHost.isEmpty() && m_serverHost != "localhost") {
        downloadUrlString.replace("localhost", m_serverHost);
    }

    m_isDownloaded = false;

    setInProgressState(0, m_fileSize);

    m_tusDownloader->startDownload(QUrl(downloadUrlString), m_localFilePath);
}

void AttachmentMessage::onDownloadFinished(const QString &filePath)
{

    m_isDownloaded = true;
    m_localFilePath = filePath;

    setCompletedState();
}

void AttachmentMessage::onDownloadError(const QString &errorMessage)
{

    m_isDownloaded = false;
    setIdleState();
}

void AttachmentMessage::openFile(const QString &customPath)
{
    const QString pathToOpen = customPath.isEmpty() ? m_localFilePath : customPath;

    if (pathToOpen.isEmpty() || !QFile::exists(pathToOpen)) {
        m_isDownloaded = false;
        setIdleState();
        return;
    }

        QDesktopServices::openUrl(QUrl::fromLocalFile(pathToOpen));
}

QString AttachmentMessage::formatSize(qint64 bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }

    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}

QPixmap AttachmentMessage::getFileIconPixmap(const QString &fileName)
{
    const QFileInfo fileInfo(fileName);
    QFileIconProvider iconProvider;
    const QIcon fileIcon = iconProvider.icon(fileInfo);

    if (!fileIcon.isNull()) {
        return fileIcon.pixmap(40, 40);
    }

    QPixmap pixmap(40, 40);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#666666"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(5, 5, 30, 35, 3, 3);
    painter.setBrush(QColor("#ffffff"));
    painter.drawRect(10, 12, 20, 3);
    painter.drawRect(10, 18, 15, 3);
    painter.drawRect(10, 24, 18, 3);

    return pixmap;
}
