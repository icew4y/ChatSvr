#ifndef USERINFODIALOG_H
#define USERINFODIALOG_H

#include <QDialog>
#include <common.pb.h>
#include <chat.pb.h>
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
    void onChatCnt(std::shared_ptr<chatpb::S2CChatCnt> pMsg);
    bool init();
};

#endif // USERINFODIALOG_H
