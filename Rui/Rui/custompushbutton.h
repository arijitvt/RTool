#ifndef CUSTOMPUSHBUTTON_H
#define CUSTOMPUSHBUTTON_H

#include <QPushButton>
#include <QString>
#include <QFile>
#include <QTextEdit>
#include <QTextStream>

class CustomPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit CustomPushButton(QString buttonText,QString fName,QTextEdit *textEdt,QWidget *parent = 0);

signals:
    void myclicked();

public slots:
    void saveFile();

private:
    QString fileNameStr;
    QString dataStr;
    QString buttonTextStr;
    QTextEdit *textEdit;

};

#endif // CUSTOMPUSHBUTTON_H
