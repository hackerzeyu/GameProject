#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QFile>
#include <QHostAddress>
#include <QMessageBox>
#include <ctime>
#include "searchroom.h"
#include <qDebug>

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    assert(loadConfig("../config.json"));
    initUI();
    connect(&m_socket,SIGNAL(connected()),this,SLOT(showConnect()));
    qDebug()<<m_ip;
    qDebug()<<m_port;
    m_socket.connectToHost(QHostAddress(m_ip),m_port);
    connect(&m_socket,&QTcpSocket::readyRead,this,&MainWidget::recvHandle);
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::initUI()
{
    QIcon icon(":/chess/wuziqi");
    this->setWindowIcon(icon);
    srand(time(0));
    for(int i=0;i<5;i++){
        char c='a'+rand()%26;
        m_name.append(c);
    }
    QString title=QString("五子棋-%1").arg(m_name);
    this->setWindowTitle(title);
    m_score=0;
    QString info=QString("当前分数:%1分").arg(m_score);
                       ui->roomInfo->setText(info);
}

void MainWidget::createRoomHandle(char* json,int length)
{
    QByteArray jsonArr=QByteArray(json,length);
    QJsonObject obj=QJsonDocument::fromJson(jsonArr).object();
    QString room=obj["room"].toString();
    QString info=QString("创建房间名:%1,当前房间1人,等待其他玩家进入...").arg(room);
    ui->roomInfo->setText(info);
    ui->randomBtn->setDisabled(true);
    ui->createBtn->setDisabled(true);
    ui->searchBtn->setDisabled(true);
}

void MainWidget::joinRoomHandle(char *json, int length)
{
    QByteArray jsonArr=QByteArray(json,length);
    QJsonObject obj=QJsonDocument::fromJson(jsonArr).object();
    QString room=obj["room"].toString();
    int cnt=obj["num"].toInt();
    if(cnt==1)
    {
        QString info=QString("加入房间名:%1,当前房间1人,等待其他玩家进入...").arg(room);
        ui->roomInfo->setText(info);
    }
    else if(cnt==2)
    {
        QJsonArray arr=obj["players"].toArray();
        QString op_name;
        for(int i=0;i<arr.size();i++)
        {
            if(arr[i].toString()!=m_name)
            {
                op_name=arr[i].toString();
            }
        }
        int index=obj["index"].toInt();
        // 由序号决定谁黑棋
        if(arr[index].toString()==m_name)
        {
            m_board=new ChessBoardWidget(&m_socket,m_name,room,true);
        }
        else
        {
            m_board=new ChessBoardWidget(&m_socket,m_name,room,false);
        }
        m_board->show();
        QString title=QString("房间号:%1-对手:%2").arg(room).arg(op_name);
        m_board->setWindowTitle(title);
        connect(this,&MainWidget::playInfo,m_board,&ChessBoardWidget::render);
        // 设置按钮不可用,防止软件错误
        ui->roomInfo->setText("游戏中...");
        ui->randomBtn->setDisabled(true);
        ui->createBtn->setDisabled(true);
        ui->searchBtn->setDisabled(true);
    }
}

void MainWidget::playHandle(char *json, int length)
{
    QByteArray jsonArr=QByteArray(json,length);
    QJsonObject obj=QJsonDocument::fromJson(jsonArr).object();
    int x=obj["x"].toInt();
    int y=obj["y"].toInt();
    emit playInfo(x,y);
}

void MainWidget::endHandle(char *json, int length)
{
    // 关闭游戏窗口
    m_board->close();
    QByteArray jsonArr=QByteArray(json,length);
    QJsonObject obj=QJsonDocument::fromJson(jsonArr).object();
    int score=obj["score"].toInt();
    m_score=score;
    QString info=QString("当前分数:%1").arg(m_score);
    ui->roomInfo->setText(info);
    ui->createBtn->setEnabled(true);
    ui->searchBtn->setEnabled(true);
    ui->randomBtn->setEnabled(true);
}

void MainWidget::searchHandle(char *json, int length)
{
    QByteArray jsonArr=QByteArray(json,length);
    QJsonObject obj=QJsonDocument::fromJson(jsonArr).object();
    QString result=obj["result"].toString();
    if(result=="not_exist")
    {
        QMessageBox::information(this,"提示","搜索的房间不存在!");
        return;
    }
    else if(result=="add_success")
    {
        QString room=obj["room"].toString();
        int index=obj["index"].toInt();
        // 由序号决定谁黑棋
        QJsonArray arr=obj["players"].toArray();
        QString op_name;
        for(int i=0;i<arr.size();i++)
        {
            if(arr[i].toString()!=m_name)
            {
                op_name=arr[i].toString();
            }
        }
        if(arr[index].toString()==m_name)
        {
            m_board=new ChessBoardWidget(&m_socket,m_name,room,true);
        }
        else
        {
            m_board=new ChessBoardWidget(&m_socket,m_name,room,false);
        }
        m_board->show();
        QString title=QString("房间号:%1-对手:%2").arg(room).arg(op_name);
                            m_board->setWindowTitle(title);
        connect(this,&MainWidget::playInfo,m_board,&ChessBoardWidget::render);
        // 设置按钮不可用,防止软件错误
        ui->roomInfo->setText("游戏中...");
        ui->randomBtn->setDisabled(true);
        ui->createBtn->setDisabled(true);
        ui->searchBtn->setDisabled(true);
    }
}

void MainWidget::showConnect()
{
    QMessageBox::information(this,"提示","连接服务器成功!");
}

void MainWidget::on_randomBtn_clicked()
{
    QJsonObject obj;
    obj["name"]=m_name;
    obj["score"]=m_score;
    QByteArray json=QJsonDocument(obj).toJson();
    ::sendMsg(m_socket,json,LogicCommand::CMD_JOIN);
}


void MainWidget::on_createBtn_clicked()
{
    QJsonObject obj;
    obj["name"]=m_name;
    obj["score"]=m_score;
    QByteArray json=QJsonDocument(obj).toJson();
    ::sendMsg(m_socket,json,LogicCommand::CMD_CREATE);
}


void MainWidget::on_searchBtn_clicked()
{
    SearchRoom *search=new SearchRoom(m_name,&m_socket,m_score);
    search->show();
}

bool MainWidget::loadConfig(QString fileName)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"警告","配置文件路径错误!");
        return false;
    }
    QByteArray config=file.readAll();
    QJsonDocument doc;
    QJsonObject obj=doc.fromJson(config).object();
    m_ip=obj["ip"].toString();
    m_port=obj["port"].toInt();
    return true;
}

void MainWidget::recvHandle()
{
    QByteArray data=m_socket.readAll();
    struct PkgHeader header;
    char* addr=data.data();
    memcpy(&header,addr,sizeof(header));
    int msgLen=::ntohs(header.msgLen);
    int msgCode=::ntohs(header.msgCode);
    qDebug()<<"msgCode="<<msgCode;
    char* json=addr+sizeof(header);
    qDebug()<<json;
    int length=msgLen-sizeof(header);
    switch(msgCode)
    {
    case LogicCommand::CMD_CREATE:
        createRoomHandle(json,length);
        break;
    case LogicCommand::CMD_JOIN:
        joinRoomHandle(json,length);
        break;
    case LogicCommand::CMD_PLAY:
        playHandle(json,length);
        break;
    case LogicCommand::CMD_END:
        endHandle(json,length);
        break;
    case LogicCommand::CMD_SEARCH:
        searchHandle(json,length);
        break;
    }
}

