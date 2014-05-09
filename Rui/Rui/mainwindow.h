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


public slots:
    void close();
    void radioClicked(int);
    void generateInvs();
    void invariantGenerationFinished();
    void enableWindow();
    void disableWindow();
};

#endif // MAINWINDOW_H
