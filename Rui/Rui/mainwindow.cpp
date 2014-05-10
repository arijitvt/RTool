#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QThread>
#include <QDialog>
#include <QScrollArea>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    choice  = -1;
  //  QMainWindow::showMaximized();

    ui->setupUi(this);
    ui->functionGroupBox->setVisible(false);
    ui->updatePPTButton->setVisible(false);
    ui->updateSrcButton->setVisible(false);


    radioGroupBox = new QButtonGroup(this);
    radioGroupBox->addButton(ui->daikonRadio);
    radioGroupBox->addButton(ui->rtoolNormalRadio);
    radioGroupBox->addButton(ui->rtoolCSRadio);
    radioGroupBox->setId(ui->daikonRadio,0);
    radioGroupBox->setId(ui->rtoolNormalRadio,1);
    radioGroupBox->setId(ui->rtoolCSRadio,2);

/*
    ui->algoCombo->addItem("DPOR");
    ui->algoCombo->addItem("PCB");
    ui->algoCombo->addItem("HaPSet");
*/
    //Connect all the signals to slots
    connect(ui->closeButton,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->okButton,SIGNAL(clicked()),this,SLOT(compile()));
    connect(radioGroupBox,SIGNAL(buttonClicked(int)),this,SLOT(radioClicked(int)));
    connect(ui->runButton,SIGNAL(clicked()),this,SLOT(generateTrace()));
    connect(ui->genInvButton,SIGNAL(clicked()),this,SLOT(generateInvariantSlot()));
    connect(ui->algoCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(updateAlgoSelect(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::radioClicked(int id) {
    cout<<"Button is cliecked with id "<<id<<endl;
    ui->okButton->setEnabled(true);
    switch(id) {
    case 0:
        choice  = id;
        break;

    case 1:
        choice = id;
        break;

    case 2:
        choice = id;
        break;

    default:
        assert(0);
    }
}

void MainWindow::compilationFinished() {
    cout<<"Invariant Generation Finished"<<endl;
    if(!ui->functionGroupBox->isVisible()){
        ui->functionGroupBox->setVisible(true);
    }
    switch(choice) {
    case 0:
        ui->updatePPTButton->setVisible(false);
        ui->updateSrcButton->setVisible(false);
        break;

    case 1:
        ui->updatePPTButton->setVisible(true);
        ui->updateSrcButton->setVisible(true);

        ui->algoComboLbl->setEnabled(true);
        ui->algoCombo->setEnabled(true);

        ui->algoCountLbl->setEnabled(true);
        ui->algoCount->setEnabled(true);

        break;

    case 2:
        ui->updatePPTButton->setVisible(true);
        ui->updateSrcButton->setVisible(true);

        ui->spacerCntLbl->setEnabled(true);
        ui->spacerCnt->setEnabled(true);

        ui->algoComboLbl->setEnabled(true);
        ui->algoCombo->setEnabled(true);

        ui->algoCountLbl->setEnabled(true);
        ui->algoCount->setEnabled(true);
        break;

    }
}


void MainWindow::compile() {
    assert(choice >= 0 && choice <=2);
    cout<<"cALLING THE INVARIANT GENERATION FUNCTION"<<endl;
    QThread *thread = new QThread();
    PythonInterface *pyInterface = new PythonInterface();
    //Handling all the choices in the PythonInterface
    pyInterface->setChoice(choice);
    pyInterface->moveToThread(thread);

    connect(thread,SIGNAL(started()),pyInterface,SLOT(compile()));

    connect(pyInterface,SIGNAL(finished()),thread,SLOT(quit()));
    connect(pyInterface,SIGNAL(finished()),this,SLOT(compilationFinished()));

    thread->start();
}

void MainWindow::generateTrace(){
    QThread *thread = new QThread();
    PythonInterface *pyInterface = new PythonInterface();
    int runCount = ui->runCount->text().toInt();
    pyInterface->setRunCount(runCount);
    pyInterface->setChoice(choice);

    if(choice == 1 || choice == 2) {
        cout<<"The Algo select in "<<__func__<<" is "<<algoSelect<<endl;
        pyInterface->setAlgoChoice(algoSelect);
        if(algoSelect > 0) {
            pyInterface->setAlgoCount(ui->algoCount->text().toInt());
        }else {
            pyInterface->setAlgoCount(0);
        }
    }

    pyInterface->moveToThread(thread);

    connect(thread,SIGNAL(started()),pyInterface,SLOT(execute()));
    connect(pyInterface,SIGNAL(finished()),thread,SLOT(quit()));
    connect(pyInterface,SIGNAL(finished()),this,SLOT(traceGenerationFinished()));
    ui->runButton->setEnabled(false);
    thread->start();
}

void MainWindow::traceGenerationFinished() {

    int count = ui->runCount->text().toInt();
    if(count == 3){
        ui->runCount->setEnabled(false);
        ui->runButton->setEnabled(false);
        ui->genInvButton->setEnabled(true);
    }else {
        ui->runButton->setEnabled(true);
        ui->runCount->setText(QString::number(count+1));
        ui->genInvButton->setEnabled(true);
    }
}

void MainWindow::generateInvariantSlot() {

    QThread *thread = new QThread();
    PythonInterface *pyInterface = new PythonInterface();

    pyInterface->moveToThread(thread);

    connect(thread,SIGNAL(started()),pyInterface,SLOT(genInvs()));
    connect(pyInterface,SIGNAL(finished()),thread,SLOT(quit()));
    connect(pyInterface,SIGNAL(finished()),this,SLOT(displayInvariants()));

    thread->start();


}

void MainWindow::displayInvariants() {
    QDialog *dailogBox = new QDialog(this);
    QVBoxLayout *boxLayout = new QVBoxLayout();

    QFile *file = new QFile(QString("result.inv"));
    QString fileContent ;
    if(!file->open(QIODevice::ReadOnly| QIODevice::Text)) {
        return;
    }
    QTextStream textStream(file);
    while(!textStream.atEnd()) {
        fileContent+= textStream.readLine();
        fileContent += QString("\n");
    }

    QTextEdit *textEdit = new QTextEdit(dailogBox);
    QTextDocument *doc = new QTextDocument(fileContent,textEdit);
    textEdit->setDocument(doc);
    textEdit->setReadOnly(true);
    boxLayout->addWidget(textEdit);

    QPushButton *closeButton = new QPushButton(tr("Close"),dailogBox);
    boxLayout->addWidget(closeButton);

    connect(closeButton,SIGNAL(clicked()),dailogBox,SLOT(reject()));

    dailogBox->setLayout(boxLayout);
    dailogBox->show();
}


void MainWindow::updateAlgoSelect(int selection) {
    algoSelect = selection;
    cout<<"Current Selection is"<<selection<<endl;
    switch (selection) {
    case 0:
        ui->spacerCntLbl->setEnabled(false);
        ui->spacerCnt->setEnabled(false);
        ui->algoComboLbl->setEnabled(false);
        ui->algoCombo->setEnabled(false);
        ui->algoCountLbl->setEnabled(false);
        ui->algoCount->setEnabled(false);
        break;
    case 1:
    case 2:
        ui->spacerCntLbl->setEnabled(true);
        ui->spacerCnt->setEnabled(true);
        ui->algoComboLbl->setEnabled(true);
        ui->algoCombo->setEnabled(true);
        ui->algoCountLbl->setEnabled(true);
        ui->algoCount->setEnabled(true);
        break;

    default:
        break;
    }
}
