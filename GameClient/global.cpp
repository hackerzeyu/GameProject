#include "global.h"

int rectWidth=912;
int rectHeight=912;
int rectX=0;
int rectY=0;
int interval=0;
int lines=25;

void sendMsg(QTcpSocket &socket, QByteArray json,LogicCommand msgCode)
{
    PkgHeader header;
    header.msgCode=::htons(msgCode);
    qDebug()<<"bodylen="<<json.length();
    int len=sizeof(header)+json.length();
    header.msgLen=::htons(len);
    qDebug()<<"msgLen="<<len;
    char *buf=new char[len+1];
    memcpy(buf,&header,sizeof(header));
    memcpy(buf+sizeof(header),json.data(),json.length());
    buf[len]='\0';
    socket.write(buf,len);
    qDebug()<<buf+sizeof(header);
    delete[] buf;
}
