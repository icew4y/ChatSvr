#ifndef USERINFODIALOG_H
#define USERINFODIALOG_H

#include <QDialog>
#include <common.pb.h>
namespace Ui {
class UserInfoDialog;
}

class UserInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserInfoDialog(QWidget *parent = nullptr);
    ~UserInfoDialog();
    void SetUserInfo(chatpb::StructUser &user);
private:
    Ui::UserInfoDialog *ui;
};

#endif // USERINFODIALOG_H
