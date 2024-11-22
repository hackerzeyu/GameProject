#ifndef GLOBAL_H
#define GLOBAL_H
#include <iostream>
#include <QTcpSocket>
#include <winsock2.h>

extern int rectWidth;        //棋盘宽度
extern int rectHeight;       //棋盘高度
extern int rectX;            //棋盘左端点x坐标
extern int rectY;            //棋盘左端点y左边
extern int interval;         //相邻点间距
extern int lines;            //棋盘行数

#pragma pack(1)
typedef struct PkgHeader
{
    uint16_t msgLen;
    uint16_t msgCode;
}*lpPkgHeader;
#pragma pack()

enum LogicCommand
{
    CMD_EMPTY,
    CMD_CREATE,         // 创建房间
    CMD_JOIN,           // 随机加入
    CMD_SEARCH,         // 搜索房间
    CMD_PLAY,           // 下棋
    CMD_END,            // 结束
    CMD_COUNT
};

void sendMsg(QTcpSocket& socket,QByteArray json,LogicCommand msgCode);


#endif // GLOBAL_H
