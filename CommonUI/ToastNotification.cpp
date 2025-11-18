#include "ToastNotification.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QGraphicsDropShadowEffect>
#include <QColor>
#include <QApplication>
#include <QScreen>

ToastNotification::ToastNotification(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    Qt::WindowFlags flags = Qt::ToolTip | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint;
    setWindowFlags(flags);

    auto *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(16);
    effect->setOffset(0, 4);
    effect->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(effect);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 12, 20, 12);

    auto *label = new QLabel(this);
    label->setObjectName(QStringLiteral("toastLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet(
        "QLabel#toastLabel {"
        "  color: white;"
        "  font-weight: bold;"
        "  font-size: 12pt;"
        "}"
    );
    layout->addWidget(label);

    setStyleSheet(
        "ToastNotification {"
        "  background-color: rgba(0, 0, 0, 210);"
        "  border-radius: 12px;"
        "}"
    );
}

ToastNotification::~ToastNotification() = default;

void ToastNotification::setMessage(const QString &message)
{
    if (auto *label = findChild<QLabel*>(QStringLiteral("toastLabel"))) {
        label->setText(message);
    }
    adjustSize();
}

void ToastNotification::showFor(int timeoutMs)
{
    show();
    QTimer::singleShot(timeoutMs, this, &ToastNotification::close);
}

void ToastNotification::showMessage(QWidget *parent, const QString &message, int timeoutMs)
{
    ToastNotification *toast = new ToastNotification(parent);
    toast->setMessage(message);

    QPoint targetPoint;
    if (parent) {
        QWidget *target = parent;
        const QSize size = toast->size();
        QPoint bottomCenter = target->mapToGlobal(QPoint(target->width() / 2, target->height()));
        targetPoint = QPoint(
            bottomCenter.x() - size.width() / 2,
            bottomCenter.y() - size.height() - 24
        );
    } else {
        QScreen *screen = QApplication::primaryScreen();
        QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 800, 600);
        targetPoint = QPoint(
            available.center().x() - toast->width() / 2,
            available.bottom() - toast->height() - 40
        );
    }

    toast->move(targetPoint);
    toast->showFor(timeoutMs);
}
