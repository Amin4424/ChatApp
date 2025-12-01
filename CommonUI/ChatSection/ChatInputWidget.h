#ifndef CHATINPUTWIDGET_H
#define CHATINPUTWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFrame>

class ChatInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatInputWidget(QWidget *parent = nullptr);
    ~ChatInputWidget();

    QString getMessageText() const;
    void clearMessageText();
    void setPlaceholderText(const QString &text);
    QTextEdit* textEdit() const;
    
    // Accessors for buttons if needed for specific styling or logic
    QPushButton* sendButton() const;
    QPushButton* attachButton() const;
    QPushButton* micButton() const;

signals:
    void sendMessageClicked();
    void sendFileClicked();
    void recordVoiceClicked();
    void textChanged();

private slots:
    void onTextChanged();

private:
    void setupUi();

    QTextEdit *m_textEdit;
    QPushButton *m_sendBtn;
    QPushButton *m_attachBtn;
    QPushButton *m_micBtn;
    QPushButton *m_smileyBtn;
    QFrame *m_inputContainer;
};

#endif // CHATINPUTWIDGET_H
