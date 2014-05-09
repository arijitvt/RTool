#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGroupBox>
#include <QButtonGroup>
#include <QAbstractButton>

#include <iostream>
#include <cassert>
using namespace std;

#include <PythonInterface.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QButtonGroup *radioGroupBox;
    //PythonInterface *pyInterface;
    int choice;
    int algoSelect;


public slots:    
    void radioClicked(int);
    void generateTrace();
    void traceGenerationFinished();
    void generateInvariantSlot();
    void updateAlgoSelect(int);

 };

#endif // MAINWINDOW_H
