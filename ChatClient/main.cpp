#include "mainwindow.h"
#include <QApplication>
#include <logindialog.h>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    LoginDialog loginDlg;
//    loginDlg.exec();
    MainWindow w;
    //w.show();
    return a.exec();
}
