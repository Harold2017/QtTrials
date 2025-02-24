#pragma once

#include <QMainWindow>

class QtPyConsole;
class PyInteractiveWrapper;

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void handleCommand(QString);

private:
    QtPyConsole* textEdit = nullptr;
    PyInteractiveWrapper* interactiveConsole = nullptr;
};