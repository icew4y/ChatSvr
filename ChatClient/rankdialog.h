#ifndef RANKDIALOG_H
#define RANKDIALOG_H

#include <QDialog>

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
    Ui::RankDialog *ui;
};

#endif // RANKDIALOG_H
