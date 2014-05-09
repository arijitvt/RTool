#ifndef PYTHONINTERFACE_H
#define PYTHONINTERFACE_H

#include <QObject>

#include <iostream>
#include <string>
using namespace std;

namespace Ui {
  class PythonInterface;
}

class PythonInterface : public QObject
{
    Q_OBJECT
public:
    explicit PythonInterface(QObject *parent = 0);



signals:
        void finished();

public slots:
    void execute();

};

#endif // PYTHONINTERFACE_H
