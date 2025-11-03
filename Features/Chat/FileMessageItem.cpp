#include "FileMessageItem.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QFileIconProvider>

FileMessageItem::FileMessageItem(const QString &fileName, qint64 fileSize, const QString &fileUrl, QWidget *parent)
    : QWidget(parent), m_fileUrl(fileUrl), m_fileName(fileName), m_fileSize(fileSize)
{
    setupUi(fileName, fileSize);
}

FileMessageItem::~FileMessageItem() {}

void FileMessageItem::setupUi(const QString &fileName, qint64 fileSize)
{
    setFixedHeight(60);
    setStyleSheet(
        "FileMessageItem {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    margin: 2px;"
        "}"
        "FileMessageItem:hover {"
        "    background-color: #e8e8e8;"
        "}"
    );

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(10);

    // File icon
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(40, 40);
    QString extension = QFileInfo(fileName).suffix().toLower();
    QPixmap iconPixmap = getFileIconPixmap(extension);
    iconLabel->setPixmap(iconPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(iconLabel);

    // File info layout
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(fileName);
    nameLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 12px;");
    nameLabel->setWordWrap(true);
    infoLayout->addWidget(nameLabel);

    QLabel *sizeLabel = new QLabel(formatSize(fileSize));
    sizeLabel->setStyleSheet("color: #666; font-size: 10px;");
    infoLayout->addWidget(sizeLabel);

    mainLayout->addLayout(infoLayout);
    mainLayout->addStretch();

    // Download button
    m_downloadButton = new QPushButton("Download");
    m_downloadButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #25d366;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 5px 10px;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #20bd5a;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1da851;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #ccc;"
        "}"
    );
    connect(m_downloadButton, &QPushButton::clicked, this, &FileMessageItem::downloadFile);
    mainLayout->addWidget(m_downloadButton);

    // Progress bar (initially hidden)
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: none;"
        "    background-color: #ddd;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #25d366;"
        "}"
    );
    mainLayout->addWidget(m_progressBar);

    // Network manager for downloads
    m_networkManager = new QNetworkAccessManager(this);
}

QPixmap FileMessageItem::getFileIconPixmap(const QString &extension)
{
    // Try to get system icon for the file type
    QFileIconProvider iconProvider;
    QFileInfo fileInfo(QString("dummy.%1").arg(extension));
    QIcon fileIcon = iconProvider.icon(fileInfo);

    if (!fileIcon.isNull()) {
        return fileIcon.pixmap(32, 32);
    }

    // Fallback: create a generic document icon
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a simple document icon
    painter.setBrush(QColor("#666"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(4, 4, 24, 28, 2, 2);

    // Draw lines to represent text
    painter.setBrush(QColor("#fff"));
    painter.setPen(Qt::NoPen);
    painter.drawRect(8, 10, 16, 2);
    painter.drawRect(8, 14, 12, 2);
    painter.drawRect(8, 18, 14, 2);

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

void FileMessageItem::downloadFile()
{
    if (m_fileUrl.isEmpty()) {
        QMessageBox::warning(this, "Error", "File URL is not available.");
        return;
    }

    // Create downloads directory if it doesn't exist
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir dir(downloadDir);
    if (!dir.exists()) {
        dir.mkpath(downloadDir);
    }

    QString filePath = downloadDir + "/" + m_fileName;

    // Check if file already exists
    if (QFile::exists(filePath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "File Exists",
            QString("File '%1' already exists. Overwrite?").arg(m_fileName),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::No) {
            return;
        }
    }

    // Start download
    QNetworkRequest request(m_fileUrl);
    m_reply = m_networkManager->get(request);

    connect(m_reply, &QNetworkReply::downloadProgress, this, &FileMessageItem::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &FileMessageItem::onDownloadFinished);
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &FileMessageItem::onDownloadError);

    m_downloadButton->setText("Downloading...");
    m_downloadButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
}

void FileMessageItem::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percentage = (bytesReceived * 100) / bytesTotal;
        m_progressBar->setValue(percentage);
    }
}

void FileMessageItem::onDownloadFinished()
{
    if (m_reply->error() != QNetworkReply::NoError) {
        return; // Error handled in onDownloadError
    }

    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString filePath = downloadDir + "/" + m_fileName;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(m_reply->readAll());
        file.close();

        m_downloadButton->setText("Open");
        m_downloadButton->setEnabled(true);
        m_progressBar->setVisible(false);

        // Disconnect download signals and connect open signal
        disconnect(m_downloadButton, &QPushButton::clicked, this, &FileMessageItem::downloadFile);
        connect(m_downloadButton, &QPushButton::clicked, this, &FileMessageItem::openFile);

        QMessageBox::information(this, "Download Complete",
            QString("File '%1' downloaded successfully.").arg(m_fileName));
    } else {
        QMessageBox::critical(this, "Error", "Failed to save the downloaded file.");
        m_downloadButton->setText("Download");
        m_downloadButton->setEnabled(true);
        m_progressBar->setVisible(false);
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void FileMessageItem::onDownloadError(QNetworkReply::NetworkError code)
{
    QMessageBox::critical(this, "Download Error",
        QString("Failed to download file: %1").arg(m_reply->errorString()));

    m_downloadButton->setText("Download");
    m_downloadButton->setEnabled(true);
    m_progressBar->setVisible(false);

    m_reply->deleteLater();
    m_reply = nullptr;
}

void FileMessageItem::openFile()
{
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString filePath = downloadDir + "/" + m_fileName;

    if (QFile::exists(filePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    } else {
        QMessageBox::warning(this, "Error", "File not found. It may have been moved or deleted.");
    }
}