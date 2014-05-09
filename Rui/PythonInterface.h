#ifndef PYTHONINTERFACE_H
#define PYTHONINTERFACE_H

#include <QObject>

class PythonInterface : public QObject
{
    Q_OBJECT
public:
    explicit PythonInterface(QObject *parent = 0);

signals:

public slots:

};

#endif // PYTHONINTERFACE_H
