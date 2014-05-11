#include "PythonInterface.h"

PythonInterface::PythonInterface(QObject *parent) :
    QObject(parent)
{
    COUNTER = 0;
    choice = -1;
    algoChoice = -1;
    algoCount= 0;
    runCount= -1;
    spCount = 0;
}






void PythonInterface::compile() {
    string command_string = "";
    switch(choice){
    case 0:
        command_string = "controller_ui.py daikon dump";
        system(command_string.c_str());
        emit finished();
        break;

    case 1:
        command_string = "controller_ui.py rtool comp";
        system(command_string.c_str());
        emit finished();
        break;

    case 2:
        command_string = "controller_ui.py cs comp";
        system(command_string.c_str());
        emit finished();
        break;

    }
}


void PythonInterface::execute() {

    QString command_string = "";
    QString arg=" ";
    switch(choice) {
        case 0:
            command_string = "controller_ui.py daikon rep "+QString::number(runCount);
            system(command_string.toStdString().c_str());
            emit finished();
            break;

    case 1:
        command_string = "controller_ui.py rtool run "+QString::number(runCount)+\
                " "+QString::number(algoChoice)+\
                " "+QString::number(algoCount)+\
                " "+arg;
        cout<<"The command string "<<command_string.toStdString()<<endl;
        system(command_string.toStdString().c_str());
        emit finished();
        break;

    case 2:
         //The format is =>cs--run-- RUN_COUNTER--SPACER_COUNT--ALGO_CHOICE--ALGO_COUNT--ARGUMENT
        command_string = "controller_ui.py cs run "+QString::number(runCount)+\
                " "+QString::number(spCount)+\
                " "+QString::number(algoChoice)+\
                " "+QString::number(algoCount)+\
                " ";
        cout<<" The command for CS ------------>"<<command_string.toStdString().c_str()<<endl;
        system(command_string.toStdString().c_str());
        emit finished();
        break;

    default:
        assert(0);

    }
    ++COUNTER;
    if(COUNTER == 3) {
        emit traceGenrationFinished();
    }
}

void PythonInterface::genInvs(){
    string command_string = "java daikon.Daikon arijit/*.dtrace > result.inv  ";
    system(command_string.c_str());
    emit finished();
}

void PythonInterface::setChoice(int c) {
    choice = c;
}

int PythonInterface::getChoice() {
    return choice;
}

void PythonInterface::setRunCount(int c) {
    runCount = c;
}
int PythonInterface::getRunCount() {
    return runCount;
}
void PythonInterface::setAlgoChoice(int c) {
    algoChoice =c;
}
int PythonInterface::getAlgoChoice() {
    return algoChoice;
}
void PythonInterface::setAlgoCount(int c) {
    algoCount =c;
}
int PythonInterface::getAlgoCount() {
    return algoCount;
}
int PythonInterface::getSpCount() const {
    return spCount;
}

void PythonInterface::setSpCount(int value) {
    spCount = value;
}

int PythonInterface::COUNTER = 0;
