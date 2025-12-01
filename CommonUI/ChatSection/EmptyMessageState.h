#ifndef EMPTYMESSAGESTATE_H
#define EMPTYMESSAGESTATE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class EmptyMessageState : public QWidget
{
    Q_OBJECT

public:
    explicit EmptyMessageState(QWidget *parent = nullptr);
    ~EmptyMessageState();

private:
    void setupUI();
    
    QLabel *m_imageLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QVBoxLayout *m_mainLayout;
};

#endif // EMPTYMESSAGESTATE_H
