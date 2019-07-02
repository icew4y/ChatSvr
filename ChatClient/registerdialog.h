#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <chat.pb.h>
namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    void on_cancel_pushButton_clicked();

    void on_reg_pushButton_clicked();

signals:
    void sigRegistered(const QString strUn, const QString strPw);

private:
    bool init();
    void onRegister(std::shared_ptr<chatpb::S2CRegister> pMsg);
    Ui::RegisterDialog *ui;
};

#endif // REGISTERDIALOG_H
