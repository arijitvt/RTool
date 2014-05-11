#include "custompushbutton.h"

#include <iostream>
using namespace std;

CustomPushButton::CustomPushButton(QString buttonText,QString fName,QTextEdit *textEdt,QWidget *parent) :
    QPushButton(parent)
{
    buttonTextStr = buttonText;
    QPushButton::setText(buttonText);
    fileNameStr = fName;
    textEdit = textEdt;
    connect(this,SIGNAL(clicked()),this,SIGNAL(myclicked()));
}

void CustomPushButton::saveFile() {
    cout<<"Now I can save file from the custom button"<<endl;
    dataStr = textEdit->document()->toPlainText();
    QFile *file = new QFile(fileNameStr);

    if(!file->open(QIODevice::WriteOnly| QIODevice::Text)) {
        return;
    }
    QTextStream textStream(file);
    textStream<<dataStr;
    file->close();

}
