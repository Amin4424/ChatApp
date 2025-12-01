#include "ModernThemeApplier.h"
#include "ModernTheme.h"
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QWidget>
#include <QListWidget>

void ModernThemeApplier::applyToMainWindow(QWidget *mainWindow)
{
    if (!mainWindow) return;
    
    mainWindow->setStyleSheet(QString(
        "QMainWindow {"
        "    background-color: %1;"
        "    font-family: '%2';"
        "}"
        "QWidget#centralwidget {"
        "    background-color: %1;"
        "    font-family: '%2';"
        "}"
        "QWidget {"
        "    font-family: '%2';"
        "}"
    ).arg(ModernTheme::CHAT_BACKGROUND, ModernTheme::PRIMARY_FONT));
}

void ModernThemeApplier::applyToPrimaryButton(QPushButton *button)
{
    if (!button) return;
    
    button->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: none;"
        "    border-radius: %3px;"
        "    padding: 14px 26px;"
        "    font-size: 15px;"
        "    font-weight: 600;"
        "    letter-spacing: 0.2px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %4;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %5;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #CCCCCC;"
        "    color: #888888;"
        "}"
    ).arg(ModernTheme::PRIMARY_BLUE)
     .arg(ModernTheme::BUTTON_TEXT)
     .arg(ModernTheme::BUTTON_RADIUS)
     .arg(ModernTheme::PRIMARY_BLUE_HOVER)
     .arg(ModernTheme::PRIMARY_BLUE_PRESSED));
}

void ModernThemeApplier::applyToSecondaryButton(QPushButton *button)
{
    if (!button) return;
    
    button->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: %1;"
        "    border: 2px solid %1;"
        "    border-radius: %2px;"
        "    padding: 12px 24px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background-color: %3;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %1;"
        "    color: white;"
        "}"
    ).arg(ModernTheme::PRIMARY_BLUE)
     .arg(ModernTheme::BUTTON_RADIUS)
     .arg(ModernTheme::HOVER_BACKGROUND));
}

void ModernThemeApplier::applyToIconButton(QPushButton *button)
{
    if (!button) return;
    
    // Force the button to be circular by setting fixed size
    int size = 52; // Circular button
    button->setMinimumSize(size, size);
    button->setMaximumSize(size, size);
    button->setFixedSize(size, size);
    
    button->setStyleSheet(QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: white;"
        "    border: none;"
        "    border-radius: %2px;"  // Half of the size for perfect circle
        "    font-size: 18px;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background-color: %3;"
        "}"
        "QPushButton:pressed {"
        "    background-color: %4;"
        "}"
    ).arg(ModernTheme::PRIMARY_BLUE)
     .arg(size / 2)  // Border radius = half size for circle
     .arg(ModernTheme::PRIMARY_BLUE_HOVER)
     .arg(ModernTheme::PRIMARY_BLUE_PRESSED));
}

void ModernThemeApplier::applyToTextInput(QTextEdit *textEdit)
{
    if (!textEdit) return;
    
    textEdit->setStyleSheet(QString(
        "QTextEdit {"
        "    background-color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: %3px;"
        "    padding: 14px 18px;"
        "    font-size: 15px;"
        "    color: %4;"
        "    outline: none;"
        "}"
        "QTextEdit:focus {"
        "    border: 2px solid %5;"
        "}"
    ).arg(ModernTheme::INPUT_BACKGROUND)
     .arg(ModernTheme::BORDER_COLOR)
     .arg(ModernTheme::INPUT_RADIUS)
     .arg(ModernTheme::PRIMARY_TEXT)
     .arg(ModernTheme::PRIMARY_BLUE));
}

void ModernThemeApplier::applyToChatArea(QWidget *chatArea)
{
    if (!chatArea) return;
    
    chatArea->setStyleSheet(QString(
        "background-color: %1;"
        "border: none;"
        "padding: 6px;"
    ).arg(ModernTheme::CHAT_BACKGROUND));
}

void ModernThemeApplier::applyToSidebar(QWidget *sidebar)
{
    if (!sidebar) return;
    
    sidebar->setStyleSheet(QString(
        "background-color: %1;"
        "border-right: 1px solid %2;"
    ).arg(ModernTheme::SIDEBAR_BACKGROUND)
     .arg(ModernTheme::BORDER_COLOR));
}

void ModernThemeApplier::applyToHeader(QWidget *header)
{
    if (!header) return;
    
    header->setStyleSheet(QString(
        "background-color: %1;"
        "border-bottom: 1px solid %2;"
        "padding: 12px 20px;"
    ).arg(ModernTheme::HEADER_BACKGROUND)
     .arg(ModernTheme::BORDER_COLOR));
}

void ModernThemeApplier::applyToListWidget(QListWidget *listWidget)
{
    if (!listWidget) return;

    listWidget->setStyleSheet(QString(
        "QListWidget {"
        "    background-color: %1;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 12px 16px;"
        "    border-radius: %2px;"
        "    color: %3;"
        "    margin: 4px 6px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: %4;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: %5;"
        "    color: white;"
        "}"
    ).arg(ModernTheme::SIDEBAR_BACKGROUND)
     .arg(ModernTheme::CARD_RADIUS)
     .arg(ModernTheme::PRIMARY_TEXT)
     .arg(ModernTheme::HOVER_BACKGROUND)
     .arg(ModernTheme::PRIMARY_BLUE));
}

void ModernThemeApplier::applyToSupportBanner(QPushButton *button)
{
    if (!button) return;

    button->setStyleSheet(QString(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 18px;"
        "    padding: 16px 18px;"
        "    text-align: left;"
        "    font-size: 14px;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %3, stop:1 %2);"
        "}"
    ).arg(ModernTheme::SUPPORT_GRADIENT_START,
          ModernTheme::SUPPORT_GRADIENT_END,
          ModernTheme::PRIMARY_BLUE_HOVER));
}

void ModernThemeApplier::applyToLabel(QLabel *label, LabelType type)
{
    if (!label) return;
    
    QString color;
    QString fontSize;
    QString fontWeight;
    
    switch (type) {
    case LabelType::Heading:
        color = ModernTheme::PRIMARY_TEXT;
        fontSize = "18px";
        fontWeight = "bold";
        break;
    case LabelType::SubHeading:
        color = ModernTheme::PRIMARY_TEXT;
        fontSize = "14px";
        fontWeight = "600";
        break;
    case LabelType::Body:
        color = ModernTheme::PRIMARY_TEXT;
        fontSize = "14px";
        fontWeight = "normal";
        break;
    case LabelType::Caption:
        color = ModernTheme::SECONDARY_TEXT;
        fontSize = "12px";
        fontWeight = "normal";
        break;
    }
    
    label->setStyleSheet(QString(
        "color: %1;"
        "font-size: %2;"
        "font-weight: %3;"
        "background: transparent;"
    ).arg(color, fontSize, fontWeight));
}
