#include "InfoCard.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>
#include <QStyle>

// Example usage of InfoCard component with Fluent API
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create main window
    QMainWindow window;
    window.setWindowTitle("InfoCard Fluent API Example");
    window.resize(400, 300);

    // Create central widget
    QWidget *centralWidget = new QWidget();
    window.setCentralWidget(centralWidget);

    // Create layout for central widget
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Create InfoCard instance and use Fluent API for method chaining
    InfoCard *card = new InfoCard();
    card->setTitle("گزارش نهایی")
        ->setFileName("report-final.pdf")
        ->setFileSize("14.2 MB")
        ->setButtonText("دانلود")
        ->setButtonBackgroundColor(Qt::blue)
        ->setButtonTextColor(Qt::white)
        ->setTitleColor(Qt::gray);

    // Set icon (using system icon as example)
    QPixmap icon = card->style()->standardPixmap(QStyle::SP_FileIcon);
    card->setIcon(icon);

    // Connect button click signal to slot
    QObject::connect(card, &InfoCard::buttonClicked, [&]() {
        qDebug() << "Button clicked! Downloading file...";
        // Here you would implement the actual download logic
    });

    // Add card to layout
    layout->addWidget(card);

    // Add stretch to center the card
    layout->addStretch();

    // Show window
    window.show();

    return app.exec();
}

/*
This example demonstrates the Fluent API (Method Chaining) for InfoCard:

InfoCard* card = new InfoCard(this);
card->setTitle("گزارش نهایی")
    ->setFileName("report-final.pdf")
    ->setFileSize("14.2 MB")
    ->setButtonText("دانلود")
    ->setButtonBackgroundColor(Qt::blue)
    ->setButtonTextColor(Qt::white)
    ->setTitleColor(Qt::gray);

Key features:
1. All setters return InfoCard* for chaining
2. Internal style management with member variables
3. Automatic style updates via updateStyles()
4. Clean, readable API similar to Flutter

The card will display:
- Title: "گزارش نهایی" (gray color)
- File name: "report-final.pdf"
- File size: "14.2 MB"
- Button: "دانلود" (blue background, white text)
- Icon: System file icon

When the button is clicked, it will print "Button clicked! Downloading file..." to the console.
*/