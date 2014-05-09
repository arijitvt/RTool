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
    QMainWindow::showMaximized();

    ui->setupUi(this);
    ui->functionGroupBox->setVisible(false);



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
    connect(ui->okButton,SIGNAL(clicked()),this,SLOT(generateTrace()));
    connect(radioGroupBox,SIGNAL(buttonClicked(int)),this,SLOT(radioClicked(int)));
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

void MainWindow::traceGenerationFinished() {
    cout<<"Invariant Generation Finished"<<endl;
    if(!ui->functionGroupBox->isVisible()){
        ui->functionGroupBox->setVisible(true);
    }
}


void MainWindow::generateTrace() {
    assert(choice >= 0 && choice <=2);
    cout<<"cALLING THE INVARIANT GENERATION FUNCTION"<<endl;
    QThread *thread = new QThread();
    PythonInterface *pyInterface = new PythonInterface();
    pyInterface->moveToThread(thread);
    connect(thread,SIGNAL(started()),pyInterface,SLOT(execute()));

    connect(pyInterface,SIGNAL(finished()),thread,SLOT(quit()));
    connect(pyInterface,SIGNAL(finished()),this,SLOT(traceGenerationFinished()));    \

    thread->start();
}

void MainWindow::generateInvariantSlot() {
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
    cout<<"Current Selection is"<<selection<<endl;
    switch (selection) {
    case 0:
        ui->algoCountLbl->setEnabled(false);
        ui->algoCount->setEnabled(false);
        break;
    case 1:
    case 2:
        ui->algoCountLbl->setEnabled(true);
        ui->algoCount->setEnabled(true);
        break;

        break;
    default:
        break;
    }
}
