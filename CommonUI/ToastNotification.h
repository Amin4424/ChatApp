#ifndef TOASTNOTIFICATION_H
#define TOASTNOTIFICATION_H

#include <QWidget>
#include <QString>

class ToastNotification : public QWidget
{
    Q_OBJECT

public:
    explicit ToastNotification(QWidget *parent = nullptr);
    ~ToastNotification() override;

    static void showMessage(QWidget *parent, const QString &message, int timeoutMs = 3000);

private:
    void setMessage(const QString &message);
    void showFor(int timeoutMs);
};

#endif // TOASTNOTIFICATION_H
