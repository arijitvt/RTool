#include "PythonInterface.h"

PythonInterface::PythonInterface(QObject *parent) :
    QObject(parent)
{
}

void PythonInterface::execute() {
    cout<<"Excuting the execute function"<<endl;
    string command_string = "test.py";
    system(command_string.c_str());
    emit finished();
}
