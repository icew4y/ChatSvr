#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <chat.pb.h>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private slots:
    void on_login_pushButton_clicked();

    void onLogin(std::shared_ptr<chatpb::S2CLogin> pMsg);
    void onRegister(std::shared_ptr<chatpb::S2CRegister> pMsg);
    void on_reg_pushButton_clicked();
    void onRegisterSuccessed(QString strUn, QString strPw);
private:
    Ui::LoginDialog *ui;
    bool init();
};

#endif // LOGINDIALOG_H
