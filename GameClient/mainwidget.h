#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpSocket>
#include "global.h"
#include "chessBoardWidget.h"

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
    void initUI();
public:
    void createRoomHandle(char* json,int length);
    void joinRoomHandle(char* json,int length);
    void playHandle(char* json,int length);
    void endHandle(char* json,int length);
    void searchHandle(char* json,int length);
signals:
    void playInfo(int x,int y);
private slots:
    // 连接成功
    void showConnect();
    // 随机加入房间
    void on_randomBtn_clicked();
    // 创建房间
    void on_createBtn_clicked();
    // 搜索房间
    void on_searchBtn_clicked();
    // 加载配置文件
    bool loadConfig(QString fileName);
    // 收取信息
    void recvHandle();

private:
    Ui::MainWidget *ui;
    QString m_name;
    QTcpSocket m_socket;
    QString m_ip;
    uint16_t m_port;
    int m_score;
    ChessBoardWidget* m_board;
};

#endif // MAINWIDGET_H
