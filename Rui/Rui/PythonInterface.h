#ifndef PYTHONINTERFACE_H
#define PYTHONINTERFACE_H

#include <QObject>

#include <iostream>
#include <string>
#include <cassert>
using namespace std;

namespace Ui {
  class PythonInterface;
}

class PythonInterface : public QObject
{
    Q_OBJECT
public:
    explicit PythonInterface(QObject *parent = 0);


private:
    int choice;
    int runCount;
    int algoChoice;
    int algoCount;// Count for PCB and HaPSet
    int spCount;//For CS
    static int COUNTER;

public:
    void setChoice(int);
    int getChoice();
    void setRunCount(int);
    int getRunCount();
    void setAlgoChoice(int);
    int getAlgoChoice();
    void setAlgoCount(int);
    int getAlgoCount();
    int getSpCount() const;
    void setSpCount(int value);

signals:
    void finished();
    void traceGenrationFinished();

public slots:
    void compile();
    void execute();
    void genInvs();

};

#endif // PYTHONINTERFACE_H
