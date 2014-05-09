#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QThread>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    choice  = -1;
    ui->setupUi(this);


  //  pyInterface = new PythonInterface();

    radioGroupBox = new QButtonGroup(this);
    radioGroupBox->addButton(ui->daikonRadio);
    radioGroupBox->addButton(ui->rtoolNormalRadio);
    radioGroupBox->addButton(ui->rtoolCSRadio);
    radioGroupBox->setId(ui->daikonRadio,0);
    radioGroupBox->setId(ui->rtoolNormalRadio,1);
    radioGroupBox->setId(ui->rtoolCSRadio,2);

    //Connect all the signals to slots
    connect(ui->closeButton,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->okButton,SIGNAL(clicked()),this,SLOT(generateInvs()));
    connect(radioGroupBox,SIGNAL(buttonClicked(int)),this,SLOT(radioClicked(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::close(){
    cout<<"Calling the close slot of the application"<<endl;
    QCoreApplication::exit(0);
}

void MainWindow::radioClicked(int id) {
    cout<<"Button is cliecked with id "<<id<<endl;
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

void MainWindow::invariantGenerationFinished() {
    cout<<"Invariant Generation Finished"<<endl;
    enableWindow();
}

void MainWindow::enableWindow() {
    if(!ui->mainFrame->isEnabled()) {
        cout<<"Enabling"<<endl;
        ui->mainFrame->setEnabled(true);
    }else {
        cout<<"Already enabled"<<endl;
    }
}

void MainWindow::disableWindow() {
    ui->mainFrame->setEnabled(false);
}

void MainWindow::generateInvs() {
    assert(choice >= 0 && choice <=2);
    cout<<"cALLING THE INVARIANT GENERATION FUNCTION"<<endl;
    QThread *thread = new QThread();
    PythonInterface *pyInterface = new PythonInterface();
    pyInterface->moveToThread(thread);
    connect(thread,SIGNAL(started()),pyInterface,SLOT(execute()));
    connect(thread,SIGNAL(started()),this,SLOT(disableWindow()));
    connect(pyInterface,SIGNAL(finished()),thread,SLOT(quit()));
    connect(pyInterface,SIGNAL(finished()),this,SLOT(invariantGenerationFinished()));    \
    disableWindow();
    thread->start();
}
