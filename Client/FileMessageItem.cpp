#include "FileMessageItem.h"
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
    
    // Connect InfoCard's button to *our* downloadFile slot
    connect(m_infoCard, &InfoCard::buttonClicked, this, &FileMessageItem::downloadFile);

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
              ->setIcon(getFileIconPixmap(m_fileName))
              ->setButtonText("Download")
              ->showProgressBar(false); // Start with progress bar hidden
              
    // Set max width so it wraps inside the chat
    m_infoCard->setMaximumWidth(450);

    // 4. Apply bubble styling (colors, alignment)
    if (m_messageType == MessageType::Sent) {
        // --- SENT (Your message) ---
        m_infoCard->setCardBackgroundColor(QColor("#a0e97e")) // Green bubble
                   ->setTitleColor(QColor("#666666"))
                   ->setButtonBackgroundColor(QColor("#075e54"))
                   ->setButtonTextColor(Qt::white)
                   ->setProgressBarColor(QColor("#075e54"));
        
        // For LTR: Add spacer first, then bubble to align RIGHT
        mainLayout->addStretch(1);
        mainLayout->addWidget(m_infoCard);

    } else {
        // --- RECEIVED (Other's message) ---
        m_infoCard->setCardBackgroundColor(QColor("#ffffffff")) // White bubble
                   ->setTitleColor(QColor("#075e54")) // Dark green title
                   ->setButtonBackgroundColor(QColor("#25d366")) // WhatsApp green button
                   ->setButtonTextColor(Qt::white)
                   ->setProgressBarColor(QColor("#25d366"));

        // For LTR: Add bubble first, then spacer to align LEFT
        mainLayout->addWidget(m_infoCard);
        mainLayout->addStretch(1);
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
        if (reply == QMessageBox::No) return;
    }

    // Build the correct download URL
    QString downloadUrlString = m_fileUrl;
    if (downloadUrlString.contains("localhost") && m_serverHost != "localhost") {
        downloadUrlString.replace("localhost", m_serverHost);
    }
    
    qDebug() << "FileMessageItem: Starting download from" << downloadUrlString;

    // Update UI to "Downloading" state
    m_infoCard->setButtonText("Downloading...")
              ->showProgressBar(true)
              ->setButtonEnabled(false);

    // Start the download
    m_tusDownloader->startDownload(QUrl(downloadUrlString), filePath);
}

void FileMessageItem::onDownloadFinished(const QString &filePath)
{
    QMessageBox::information(this, "Download Complete",
                             QString("File '%1' downloaded successfully.").arg(m_fileName));
    
    // Update UI to "Downloaded" state
    m_infoCard->setButtonText("Open")
              ->showProgressBar(false)
              ->setButtonEnabled(true);

    // Disconnect download slot, connect open slot
    disconnect(m_infoCard, &InfoCard::buttonClicked, this, &FileMessageItem::downloadFile);
    connect(m_infoCard, &InfoCard::buttonClicked, this, &FileMessageItem::openFile);
}

void FileMessageItem::onDownloadError(const QString &errorMessage)
{
    QMessageBox::critical(this, "Download Error", errorMessage);
    
    // Update UI back to "Retry" state
    m_infoCard->setButtonText("Retry")
              ->showProgressBar(false)
              ->setButtonEnabled(true);
}

void FileMessageItem::openFile()
{
    QString downloadDir = QCoreApplication::applicationDirPath() + "/client_downloads";
    QString filePath = downloadDir + "/" + m_fileName;

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        QMessageBox::warning(this, "Error", "Could not open file. It may have been moved or deleted.");
    }
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