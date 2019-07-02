#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <map>
#include <functional>
#include <proto.hpp>
#include <QStringListModel>
#include <QStandardItemModel>
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
    void on_send_pushButton_clicked();

    void onRead();

    void onListViewClicked(const QModelIndex &index);
private:
    Ui::MainWindow *ui;
    bool init();
    void protoDispath(uint32_t nProtoId, const std::string &strMsg);
    void onUserID(const std::string & strData);
    void onUserList(const std::string &strData);
    void onChat(const std::string &strData);


    std::shared_ptr<QTcpSocket> m_pSocket;
    std::map<ProtoId, CbFunc> m_mCb;
    QStringListModel m_listModel;
    uint32_t m_nPacketSize = 0;
    int32_t m_nSelfId = 0;
};

#endif // MAINWINDOW_H
