#include "chessBoardWidget.h"
#include "ui_chessBoardWidget.h"
#include "global.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QDebug>
#include <QMessageBox>
#include <QIcon>
#include <QJsonObject>
#include <QJsonDocument>

ChessBoardWidget::ChessBoardWidget(QTcpSocket *socket,QString name,QString roomName,bool flag,QWidget *parent)
    : QWidget(parent),m_flag(flag),m_name(name),ui(new Ui::Widget),m_socket(socket),
    m_roomName(roomName)
{
    ui->setupUi(this);
    this->initUI();
    this->init();
    if(m_flag==false)
    {
        // 一开始不允许下,说明是白棋
        QMessageBox::information(this,"提示","本场游戏你被选中为白棋!");
        m_index=0;
    }
    else
    {
        QMessageBox::information(this,"提示","本场游戏你被选中为黑棋!");
        m_index=1;
    }
}

void ChessBoardWidget::init()
{
    this->whiteChessPositions.clear();
    this->blackChessPositions.clear();
}

//初始化界面
void ChessBoardWidget::initUI()
{
    this->setFixedSize(1000,1000);
    this->setAutoFillBackground(true);
    QPixmap pix=QPixmap(":/chess/background").scaled(this->size());
    QPalette palette;
    palette.setBrush(QPalette::Window,QBrush(pix));
    this->setPalette(palette);
    QIcon icon(":/chess/wuziqi");
    this->setWindowIcon(icon);
    this->update();
}

//画棋盘
void ChessBoardWidget::drawChessBoard()
{
    //创建绘图对象
    QPainter painter(this);

    //设置画笔属性
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(2);
    painter.setPen(pen);

    rectX=(width()-rectWidth)/2;
    rectY=(height()-rectHeight)/2;

    painter.drawRect(rectX,rectY,rectWidth,rectHeight);

    interval=rectWidth/(lines-1);
    for(int i=1;i<25;i++)
    {
        painter.drawLine(rectX+interval*i,rectY,rectX+interval*i,rectY+rectHeight);
    }

    for(int i=1;i<25;i++)
    {
        painter.drawLine(rectX,rectY+interval*i,rectX+rectWidth,rectY+interval*i);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    for(auto &pos:blackChessPositions)
    {
        painter.setBrush(Qt::black);
        painter.drawEllipse(pos,interval/2,interval/2);
    }

    for(auto &pos:whiteChessPositions)
    {
        painter.setBrush(Qt::white);
        painter.drawEllipse(pos,interval/2,interval/2);
    }
}


void ChessBoardWidget::paintEvent(QPaintEvent *event)
{
    this->drawChessBoard();
}

void ChessBoardWidget::mousePressEvent(QMouseEvent *event)
{
    if(m_flag)
    {
        if(event->button()==Qt::LeftButton)
        {
            QPoint clickPos=event->pos();

            int x=((clickPos.x()-rectX+interval/2)/interval)*interval+rectX;
            int y=((clickPos.y()-rectY+interval/2)/interval)*interval+rectY;

            if(!(x<=rectX+24*interval && y<=rectY+24*interval))
                return;

            QPoint chessPos(x,y);

            if(!whiteChessPositions.contains(chessPos) && !blackChessPositions.contains(chessPos))
            {
                bool win=false;
                if(m_index%2==1)
                {
                    blackChessPositions.push_back(chessPos);
                    win=GameJudge(blackChessPositions,chessPos);
                    if(win)
                    {
                        QMessageBox::StandardButton result=QMessageBox::information(this,"游戏结束","黑棋获胜",QMessageBox::Ok | QMessageBox::Close);
                        this->init();
                        this->initUI();
                        this->drawChessBoard();
                        // 游戏结束,黑棋获胜
                        QJsonObject obj;
                        obj["winner"]=m_name;
                        obj["room"]=m_roomName;
                        QByteArray json=QJsonDocument(obj).toJson();
                        ::sendMsg(*m_socket,json,LogicCommand::CMD_END);
                        return;
                    }
                }
                else
                {
                    whiteChessPositions.push_back(chessPos);
                    win=GameJudge(whiteChessPositions,chessPos);
                    if(win)
                    {
                        QMessageBox::StandardButton result=QMessageBox::information(this,"游戏结束","白棋获胜",QMessageBox::Ok | QMessageBox::Close);
                        this->init();
                        this->initUI();
                        this->drawChessBoard();
                        // 游戏结束,白棋获胜
                        QJsonObject obj;
                        obj["winner"]=m_name;
                        obj["room"]=m_roomName;
                        QByteArray json=QJsonDocument(obj).toJson();
                        ::sendMsg(*m_socket,json,LogicCommand::CMD_END);
                        return;
                    }
                }
                update();
                // 此处发送下棋消息
                QJsonObject obj;
                obj["room"]=m_roomName;
                obj["name"]=m_name;
                obj["x"]=x;
                obj["y"]=y;
                m_flag=!m_flag;
                QByteArray json=QJsonDocument(obj).toJson();
                ::sendMsg(*m_socket,json,LogicCommand::CMD_PLAY);
            }
        }
    }
}

bool ChessBoardWidget::GameJudge(QVector<QPoint> &chessPositions,QPoint &pos)
{
    int x=pos.x();
    int y=pos.y();
    int i,j,count=1;
    //横向检查
    for(i=x+interval;i<=x+4*interval;i+=interval)
    {
        if(chessPositions.contains(QPoint(i,y)))
            count++;
        else
            break;
    }
    for(i=x-interval;i>=x-4*interval;i-=interval)
    {
        if(chessPositions.contains(QPoint(i,y)))
            count++;
        else
            break;
    }
    if(count>=5)
        return true;
    //纵向检查
    count=1;
    for(j=y+interval;j<=y+4*interval;j+=interval)
    {
        if(chessPositions.contains(QPoint(x,j)))
            count++;
        else
            break;
    }
    for(j=y-interval;j>=y-4*interval;j-=interval)
    {
        if(chessPositions.contains(QPoint(x,j)))
            count++;
        else
            break;
    }
    if(count>=5)
        return true;
    count=1;
    for(i=x+interval,j=y+interval;i<=x+4*interval,j<=y+4*interval;i+=interval,j+=interval)
    {
        if(chessPositions.contains(QPoint(i,j)))
            count++;
        else
            break;
    }
    for(i=x-interval,j=y-interval;i>=x-4*interval,j>=y-4*interval;i-=interval,j-=interval)
    {
        if(chessPositions.contains(QPoint(i,j)))
            count++;
        else
            break;
    }
    if(count>=5)
        return true;
    count=1;
    for(i=x+interval,j=y-interval;i<=x+4*interval,j>=y-4*interval;i+=interval,j-=interval)
    {
        if(chessPositions.contains(QPoint(i,j)))
            count++;
        else
            break;
    }
    for(i=x-interval,j=y+interval;i>=x-4*interval,j<=y+4*interval;i-=interval,j+=interval)
    {
        if(chessPositions.contains(QPoint(i,j)))
            count++;
        else
            break;
    }
    if(count>=5)
        return true;
    return false;
}

void ChessBoardWidget::render(int x, int y)
{
    QPoint point(x,y);
    m_flag=!m_flag;
    if(m_index==1)
    {
        whiteChessPositions.push_back(point);
    }
    else
    {
        blackChessPositions.push_back(point);
    }
    update();
}

ChessBoardWidget::~ChessBoardWidget()
{
    delete ui;
}


