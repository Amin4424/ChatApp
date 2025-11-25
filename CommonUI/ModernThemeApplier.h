#ifndef MODERNTHEMEAPPLIER_H
#define MODERNTHEMEAPPLIER_H

class QWidget;
class QPushButton;
class QTextEdit;
class QLabel;
class QListWidget;

class ModernThemeApplier
{
public:
    enum class LabelType {
        Heading,
        SubHeading,
        Body,
        Caption
    };
    
    static void applyToMainWindow(QWidget *mainWindow);
    static void applyToPrimaryButton(QPushButton *button);
    static void applyToSecondaryButton(QPushButton *button);
    static void applyToIconButton(QPushButton *button);
    static void applyToSupportBanner(QPushButton *button);
    static void applyToTextInput(QTextEdit *textEdit);
    static void applyToChatArea(QWidget *chatArea);
    static void applyToSidebar(QWidget *sidebar);
    static void applyToHeader(QWidget *header);
    static void applyToListWidget(QListWidget *listWidget);
    static void applyToLabel(QLabel *label, LabelType type = LabelType::Body);
};

#endif // MODERNTHEMEAPPLIER_H
