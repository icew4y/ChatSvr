#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <map>
#include <functional>
#include <proto.hpp>
#include <QStringListModel>
#include <QStandardItemModel>
#include <chat.pb.h>
namespace Ui {
class MainWindow;
}
using CbFunc = std::function<void(const std::string &)>;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onListViewDBClicked(const QModelIndex &index);
    void onListViewClicked(const QModelIndex &index);
    void on_rank_pushButton_clicked();

    void on_send_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    void onLogin(std::shared_ptr<chatpb::S2CLogin> pMsg);
    void onOnlineUsers(std::shared_ptr<chatpb::S2COnlineUsers> pMsg);
    void onChat(std::shared_ptr<chatpb::S2CChat> pMsg);
    void onUserStatusChange(std::shared_ptr<chatpb::S2CStatusChange> pMsg);
    bool init();
    void updateUserList();
    QStringListModel m_listModel;
    std::map<std::string, chatpb::StructUser> m_mCurUsers;
    chatpb::StructUser m_selfInfo;
};

#endif // MAINWINDOW_H
