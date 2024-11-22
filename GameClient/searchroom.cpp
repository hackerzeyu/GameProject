#include "searchroom.h"
#include "ui_searchroom.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include "global.h"

SearchRoom::SearchRoom(QString name,QTcpSocket* socket,int score,QWidget *parent) :
    QWidget(parent),ui(new Ui::SearchRoom),m_name(name),m_socket(socket),m_score(score)
{
    ui->setupUi(this);
    this->setWindowTitle("搜索房间");
    this->setFixedSize(350,100);
}

SearchRoom::~SearchRoom()
{
    delete ui;
}

void SearchRoom::on_addBtn_clicked()
{
    if(ui->lineEdit->text().isEmpty())
    {
        QMessageBox::warning(this,"提示","房间号不能为空!");
        return;
    }
    QJsonObject obj;
    obj["name"]=m_name;
    obj["room"]=ui->lineEdit->text();
    obj["score"]=m_score;
    QByteArray json=QJsonDocument(obj).toJson();
    ::sendMsg(*m_socket,json,LogicCommand::CMD_SEARCH);
}

