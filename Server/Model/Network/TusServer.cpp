#include "Model/Network/TusServer.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QProcess>
#include <QThread>
#include <QCoreApplication>

TusServer::TusServer(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isRunning(false)
{
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TusServer::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &TusServer::onProcessError);

    // Connect to process output for debugging
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
    });
}

TusServer::~TusServer()
{
    stop();
}

void TusServer::killExistingProcessOnPort(quint16 port)
{

    // Use lsof to find processes using the port
    QProcess lsofProcess;
    lsofProcess.start("lsof", QStringList() << "-ti" << QString(":%1").arg(port));
    lsofProcess.waitForFinished(2000);

    if (lsofProcess.exitCode() == 0) {
        QString output = lsofProcess.readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            QStringList pids = output.split('\n');

            // Kill each process
            for (const QString &pid : pids) {
                if (!pid.isEmpty()) {
                    QProcess::execute("kill", QStringList() << "-9" << pid.trimmed());
                }
            }

            // Wait a bit for processes to die
            QThread::msleep(500);
        } else {
        }
    } else {

        // Alternative: try to kill any tusd processes
        QProcess::execute("pkill", QStringList() << "-9" << "tusd");
        QThread::msleep(500);
    }
}

bool TusServer::start(quint16 port, const QString &uploadDir)
{
    if (m_isRunning) {
        qWarning() << "TUS server is already running";
        return true;
    }

    // Kill any existing processes using the port
    killExistingProcessOnPort(port);

    // Look for tusd in the same directory as the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString tusdPath = appDir + "/tusd";
    
    
    // If not found, return silently (no error)
    if (!QFile::exists(tusdPath)) {
        qWarning() << "tusd binary not found at:" << tusdPath << "- File upload will not work without tusd server";
        return false;
    }
    

    // Create uploads directory if it doesn't exist
    QString fullUploadDir = appDir + "/" + uploadDir;
    QDir().mkpath(fullUploadDir);
    
    QString workingDir = appDir;
    QStringList arguments;
    arguments << "-host" << "0.0.0.0" << "-port" << QString::number(port) << "-upload-dir" << fullUploadDir;

    m_process->setWorkingDirectory(workingDir);
    m_process->start(tusdPath, arguments);

    if (m_process->waitForStarted(5000)) {
        m_isRunning = true;
        emit started();
        return true;
    } else {
        QString errorMsg = "Failed to start TUS server: " + m_process->errorString();
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
}

void TusServer::stop()
{
    if (!m_isRunning) {
        return;
    }

    m_process->terminate();

    if (!m_process->waitForFinished(3000)) {
        qWarning() << "TUS server didn't terminate gracefully, killing...";
        m_process->kill();
        m_process->waitForFinished(1000);
    }

    m_isRunning = false;
    emit stopped();
}

bool TusServer::isRunning() const
{
    return m_isRunning;
}

void TusServer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isRunning = false;
    emit stopped();
}

void TusServer::onProcessError(QProcess::ProcessError error)
{
    m_isRunning = false;
    QString errorMsg = "TUS process error: " + QString::number(error);
    qCritical() << errorMsg;
    emit errorOccurred(errorMsg);
}