#ifndef RANKDIALOG_H
#define RANKDIALOG_H

#include <QDialog>
#include <chat.pb.h>
namespace Ui {
class RankDialog;
}

class RankDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RankDialog(QWidget *parent = nullptr);
    ~RankDialog();

private:
    void onChatCntTopUsers(std::shared_ptr<chatpb::S2CChatCntTopUsers> pMsg);
    Ui::RankDialog *ui;
    bool init();
};

#endif // RANKDIALOG_H
