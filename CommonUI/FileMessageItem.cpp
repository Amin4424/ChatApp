#include "FileMessageItem.h"
#include "TusDownloader.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QCoreApplication>
#include <QStyle>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QPainter> // Added for drawing file icons

FileMessageItem::FileMessageItem(const QString &fileName, qint64 fileSize, const QString &fileUrl,
                                 const QString &senderInfo, MessageType type, 
                                 const QString &serverHost, QWidget *parent)
    : BaseMessageItem(senderInfo, parent),
      m_fileUrl(fileUrl),
      m_fileName(fileName),
      m_fileSize(fileSize),
      m_messageType(type),
      m_serverHost(serverHost)
{
    // 1. Create the Downloader
    m_tusDownloader = new TusDownloader(this);

    // 2. Create the UI (InfoCard)
    setupUI();

    // 3. Connect signals
    
    // Connect InfoCard's cardClicked to our onCardClicked slot
    connect(m_infoCard, &InfoCard::cardClicked, this, &FileMessageItem::onCardClicked);

    // Connect TusDownloader signals
    connect(m_tusDownloader, &TusDownloader::finished, this, &FileMessageItem::onDownloadFinished);
    connect(m_tusDownloader, &TusDownloader::error, this, &FileMessageItem::onDownloadError);
    
    // Connect progress from downloader directly to InfoCard's progress bar slot
    connect(m_tusDownloader, &TusDownloader::downloadProgress, m_infoCard, &InfoCard::updateProgress);
}

FileMessageItem::~FileMessageItem()
{
    // m_infoCard and m_tusDownloader are children of this, Qt will delete them.
}

void FileMessageItem::setupUI()
{
    // 1. Remove any existing layout
    delete layout(); 

    // 2. Create the main horizontal layout (for alignment)
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(0);

    // 3. Create and configure the InfoCard
    m_infoCard = new InfoCard(this);
    m_infoCard->setTitle(m_senderInfo)
              ->setFileName(m_fileName)
              ->setFileSize(formatSize(m_fileSize))
              ->setIcon(getFileIconPixmap(m_fileName));
              
    // Set max width so it wraps inside the chat
    m_infoCard->setMaximumWidth(450);

    // 4. Apply bubble styling (colors, alignment)
    if (m_messageType == MessageType::Sent) {
        // --- SENT (Your message) ---
        m_infoCard->setCardBackgroundColor(QColor("#a0e97e")) // Green bubble
                   ->setTitleColor(QColor("#666666"));
        
        // **FIX: همیشه تیک نشان بده (پیام ارسال شده)**
        m_infoCard->setState(InfoCard::State::Completed_Sent);
        m_isSent = true; // همیشه true است برای پیام‌های ارسالی
        m_isDownloaded = true; // Sent files are considered "available"
        
        // For LTR: Add spacer first, then bubble to align RIGHT
        mainLayout->addStretch(1);
        mainLayout->addWidget(m_infoCard);

    } else {
        // --- RECEIVED (Other's message) ---
        m_infoCard->setCardBackgroundColor(QColor("#ffffffff")) // White bubble
                   ->setTitleColor(QColor("#075e54")); // Dark green title

        // **FIX: برای پیام‌های دریافتی هم تیک نشان بده (پیام دریافت شده)**
        m_infoCard->setState(InfoCard::State::Completed_Downloaded);
        m_isDownloaded = false; // Not downloaded to disk yet (but received from sender)
        
        // For LTR: Add bubble first, then spacer to align LEFT
        mainLayout->addWidget(m_infoCard);
        mainLayout->addStretch(1);
    }
}

void FileMessageItem::onCardClicked()
{
    if (m_isDownloaded) {
        openFile();
    } else {
        downloadFile();
    }
}

void FileMessageItem::downloadFile()
{
    if (m_fileUrl.isEmpty()) {
        QMessageBox::warning(this, "Error", "File URL is not available.");
        return;
    }

    // Get download path
    QString downloadDir = QCoreApplication::applicationDirPath() + "/client_downloads";
    QDir dir(downloadDir);
    if (!dir.exists()) dir.mkpath(".");
    QString filePath = downloadDir + "/" + m_fileName;

    // Check if file exists
    if (QFile::exists(filePath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "File Exists",
            QString("File '%1' already exists. Overwrite?").arg(m_fileName),
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::No) {
            qDebug() << "📥 [FileMessageItem] User cancelled overwrite";
            return;
        }
    }

    // Build the correct download URL
    QString downloadUrlString = m_fileUrl;
    
    // If URL doesn't start with http, build full URL
    if (!downloadUrlString.startsWith("http://") && !downloadUrlString.startsWith("https://")) {
        qDebug() << "📥 [FileMessageItem] URL is relative, building full URL...";
        
        // Default to localhost if serverHost is empty
        QString host = m_serverHost.isEmpty() ? "localhost" : m_serverHost;
        
        // Remove leading slash if present
        if (downloadUrlString.startsWith("/")) {
            downloadUrlString = downloadUrlString.mid(1);
        }
        
        downloadUrlString = QString("http://%1:1080/%2").arg(host, downloadUrlString);
        qDebug() << "📥 [FileMessageItem] Built full URL:" << downloadUrlString;
    } else {
        // URL already has protocol, just fix localhost if needed
        if (downloadUrlString.contains("localhost") && m_serverHost != "localhost") {
            downloadUrlString.replace("localhost", m_serverHost);
            qDebug() << "📥 [FileMessageItem] Replaced localhost with:" << m_serverHost;
        }
    }
    

    // Update UI to "Downloading" state
    m_infoCard->setState(InfoCard::State::In_Progress_Download);

    // Start the download
    m_tusDownloader->startDownload(QUrl(downloadUrlString), filePath);
}

void FileMessageItem::onDownloadFinished(const QString &filePath)
{
    QMessageBox::information(this, "Download Complete",
                             QString("File '%1' downloaded successfully.").arg(m_fileName));
    
    // Update state to downloaded
    m_isDownloaded = true;
    
    // Update UI to "Downloaded" state (clickable to open)
    m_infoCard->setState(InfoCard::State::Completed_Downloaded);
}

void FileMessageItem::onDownloadError(const QString &errorMessage)
{
    QMessageBox::critical(this, "Download Error", errorMessage);
    
    // Update state back to not downloaded
    m_isDownloaded = false;
    
    // Update UI back to "Idle_Downloadable" state (clickable to retry)
    m_infoCard->setState(InfoCard::State::Idle_Downloadable);
}

void FileMessageItem::openFile()
{
    QString downloadDir = QCoreApplication::applicationDirPath() + "/client_downloads";
    QString filePath = downloadDir + "/" + m_fileName;

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        QMessageBox::warning(this, "Error", "Could not open file. It may have been moved or deleted.");
    }
}

void FileMessageItem::markAsSent()
{
    qDebug() << "📤 [FileMessageItem] markAsSent() called for:" << m_fileName;
    // Change state from Pending (🕒) to Sent (✓)
    m_isSent = true;  // Set flag
    m_infoCard->setState(InfoCard::State::Completed_Sent);
}

void FileMessageItem::setUploadProgress(qint64 bytes, qint64 total)
{
    // Update progress bar during upload
    m_infoCard->updateProgress(bytes, total);
}

// --- Helper Functions ---

QPixmap FileMessageItem::getFileIconPixmap(const QString &fileName)
{
    // Use QFileInfo to get system icon
    QFileInfo fileInfo(fileName);
    QFileIconProvider iconProvider;
    QIcon fileIcon = iconProvider.icon(fileInfo);
    
    if (!fileIcon.isNull()) {
        return fileIcon.pixmap(40, 40);
    }

    // Fallback: create a generic document icon
    QPixmap pixmap(40, 40);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#666"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(5, 5, 30, 35, 3, 3);
    painter.setBrush(QColor("#fff"));
    painter.drawRect(10, 12, 20, 3);
    painter.drawRect(10, 18, 15, 3);
    painter.drawRect(10, 24, 18, 3);
    return pixmap;
}

QString FileMessageItem::formatSize(qint64 bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}