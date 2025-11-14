#ifndef TUSSERVER_H
#define TUSSERVER_H

#include <QObject>
#include <QProcess>

class TusServer : public QObject
{
    Q_OBJECT
public:
    explicit TusServer(QObject *parent = nullptr);
    ~TusServer();

    bool start(quint16 port = 1080, const QString &uploadDir = "./uploads");
    void stop();
    bool isRunning() const;

signals:
    void started();
    void stopped();
    void errorOccurred(const QString &errorMessage);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void killExistingProcessOnPort(quint16 port);
    QProcess *m_process;
    bool m_isRunning;
};

#endif // TUSSERVER_H