#include "logic.h"
#include "log.h"
#include "tcpserver.h"
#include <iostream>
#include <ctime>

// 函数对映表
const CLogicHandler::handler CLogicHandler::m_statusHandle[CLogicHandler::CMD_COUNT] =
    {
        nullptr, // 为空,代表控制信息
        &CLogicHandler::_HandleCreateRoom,
        &CLogicHandler::_HandleJoinRoom,
        &CLogicHandler::_HandleSearchRoom,
        &CLogicHandler::_HandlePlay,
        &CLogicHandler::_HandleEnd};

void CLogicHandler::run()
{
    pthread_t pid;
    ThreadItem *item = new ThreadItem(this);
    int ret = pthread_create(&pid, nullptr, serverSendFunc, item);
    if (ret < 0)
    {
        Fatal("write thread create failed,errmsg=%s", strerror(errno));
        exit(1);
    }
    while (item->_Running == false)
        ;
}

CLogicHandler *CLogicHandler::getInstance()
{
    static CLogicHandler logicChannel;
    return &logicChannel;
}

void CLogicHandler::threadRecvProc(char *buf)
{
    MsgHeader msgHeader;
    memcpy(&msgHeader, buf, _MSG_HEAD_SIZE_);
    PkgHeader pkgHeader;
    memcpy(&pkgHeader, buf + _MSG_HEAD_SIZE_, _HEAD_SIZE_);
    uint16_t msgLen = ::ntohs(pkgHeader.len);
    uint16_t msgCode = ::ntohs(pkgHeader.msgCode);
    // 记录消息码和连接信息
    Channel *channel = msgHeader.channel;
    Debug("msgCurrence=%d,channelCurrence=%d,msgLen=%d,msgCode=%d", msgHeader.currence, channel->m_currence, msgLen, msgCode);
    // 检测废包
    // (1)如果从收到客户端发送来的包,到服务器释放一个线程池中的线程处理该包的过程中,客户端断开了,所以丢弃
    // (2)该连接对象被其他连接接管了
    // TODO
    if (channel->m_currence != msgHeader.currence)
    {
        // 丢弃
        Erro("rubbish package,throw away!!!");
        return;
    }
    if (msgCode >= CMD_COUNT)
    {
        // 恶意包,不能处理
        Erro("logic msgCode is invalid,kill!!!");
        TcpServer::getInstance()->recycle(channel);
        return;
    }
    if (m_statusHandle[msgCode] == nullptr)
    {
        // 正常现象,包不需要处理,应该是控制信息
        Info("client send control message!");
        return;
    }
    (this->*m_statusHandle[msgCode])(channel, msgHeader, (char *)(buf + _MSG_HEAD_SIZE_ + _HEAD_SIZE_), msgLen - _HEAD_SIZE_);
}

bool CLogicHandler::_HandleCreateRoom(Channel *channel, MsgHeader &msgHeader, char *body, uint16_t bodyLength)
{
    if (body == nullptr)
    {
        // 这种属于恶意包,既然不是控制信息,为啥还是空包体
        Erro("invalid empty packet...");
        TcpServer::getInstance()->recycle(channel);
        return false;
    }
    Json::Reader r;
    Json::Value val;
    // json解析
    if (!r.parse(body, val))
    {
        Erro("json string parse error in login!");
        return false;
    }
    string name = val["name"].asString();
    std::string oldRoom = channel->m_redis->getRoomIn(name);
    if (oldRoom != std::string())
    {
        // TODO
        Debug("用户已经加入房间...");
        return false;
    }
    int score = val["score"].asInt();
    Json::Value v;
    string roomName = channel->m_redis->createRoom(name, score);
    auto roomList = RoomList::getInstance();
    roomList->addPlayer(roomName, name, channel);
    // 序列化json数据
    v["room"] = roomName;
    string json = Json::FastWriter().write(v);
    std::lock_guard<std::mutex> lg(m_sendQueueMutex);
    initSendChannel(channel, msgHeader, json, LogicCommand::CMD_CREATE);
    return true;
}

bool CLogicHandler::_HandleJoinRoom(Channel *channel, MsgHeader &msgHeader, char *body, uint16_t bodyLength)
{
    if (body == nullptr)
    {
        // 这种属于恶意包,既然不是控制信息,为啥还是空包体
        Erro("invalid empty packet...");
        TcpServer::getInstance()->recycle(channel);
        return false;
    }
    Json::Reader r;
    Json::Value val;
    // json解析
    if (!r.parse(body, val))
    {
        Erro("json string parse error in login!");
        return false;
    }
    string name = val["name"].asString();
    string oldRoom = channel->m_redis->getRoomIn(name);
    if (oldRoom != std::string())
    {
        // TODO
        Debug("用户已经加入房间...");
        return false;
    }
    string roomName = channel->m_redis->joinRoom(name);
    int score = val["score"].asInt();
    Debug("new score=%d", score);
    // 实时更新分数
    channel->m_redis->updateScore(roomName, name, score);
    auto roomList = RoomList::getInstance();
    roomList->addPlayer(roomName, name, channel);
    // 获取房间信息
    UserInfo info = roomList->getRoomPlayers(roomName);
    Json::Value v;
    v["room"] = roomName;
    int count = info.size();
    v["num"] = count;
    // 准备发送信息
    std::lock_guard<std::mutex> lg(m_sendQueueMutex);
    if (count == 1)
    {
        string json = Json::FastWriter().write(v);
        initSendChannel(channel, msgHeader, json, LogicCommand::CMD_JOIN);
    }
    else if (count == 2)
    {
        Json::Value arr;
        for (auto it = info.begin(); it != info.end(); it++)
        {
            arr.append(it->first);
        }
        v["players"] = arr;
        // 决定谁先出棋
        srand(time(0));
        int index = rand() % 2;
        v["index"] = index;
        string json = Json::FastWriter().write(v);
        initSendChannel(channel, msgHeader, json, LogicCommand::CMD_JOIN);
        // 准备发送给对手
        Channel *op_channel = roomList->getOpponent(roomName, name).second;
        MsgHeader op_msgHeader;
        op_msgHeader.channel = op_channel;
        op_msgHeader.currence = op_channel->m_currence;
        initSendChannel(op_channel, op_msgHeader, json, LogicCommand::CMD_JOIN);
    }
    return true;
}

bool CLogicHandler::_HandleSearchRoom(Channel *channel, MsgHeader &msgHeader, char *body, uint16_t bodyLength)
{
    if (body == nullptr)
    {
        // 这种属于恶意包,既然不是控制信息,为啥还是空包体
        Erro("invalid empty packet...");
        TcpServer::getInstance()->recycle(channel);
        return false;
    }
    Json::Reader r;
    Json::Value val;
    // json解析
    if (!r.parse(body, val))
    {
        Erro("json string parse error in login!");
        return false;
    }
    string roomName = val["room"].asString();
    string name = val["name"].asString();
    int score = val["score"].asInt();
    RoomList *roomList = RoomList::getInstance();
    Json::Value v;
    if (roomList->getRoomPlayers(roomName).empty())
    {
        v["result"] = "not_exist";
        string json = Json::FastWriter().write(v);
        std::lock_guard<std::mutex> locker(m_logicMutex);
        initSendChannel(channel, msgHeader, json, LogicCommand::CMD_SEARCH);
    }
    else
    {
        channel->m_redis->joinRoom(name, roomName);
        // 实时更新分数
        channel->m_redis->updateScore(roomName, name, score);
        roomList->addPlayer(roomName, name, channel);
        v["result"] = "add_success";
        v["room"] = roomName;
        UserInfo info = roomList->getRoomPlayers(roomName);
        Json::Value arr;
        for (auto it = info.begin(); it != info.end(); it++)
        {
            arr.append(it->first);
        }
        v["players"] = arr;
        // 决定谁先出棋
        srand(time(0));
        int index = rand() % 2;
        v["index"] = index;
        string json = Json::FastWriter().write(v);
        std::lock_guard<std::mutex> locker(m_logicMutex);
        initSendChannel(channel, msgHeader, json, LogicCommand::CMD_SEARCH);
        // 准备发送给对手
        Channel *op_channel = roomList->getOpponent(roomName, name).second;
        MsgHeader op_msgHeader;
        op_msgHeader.channel = op_channel;
        op_msgHeader.currence = op_channel->m_currence;
        initSendChannel(op_channel, op_msgHeader, json, LogicCommand::CMD_SEARCH);
    }
    return true;
}

bool CLogicHandler::_HandlePlay(Channel *channel, MsgHeader &msgHeader, char *body, uint16_t bodyLength)
{
    if (body == nullptr)
    {
        // 这种属于恶意包,既然不是控制信息,为啥还是空包体
        Erro("invalid empty packet...");
        TcpServer::getInstance()->recycle(channel);
        return false;
    }
    Json::Reader r;
    Json::Value val;
    // json解析
    if (!r.parse(body, val))
    {
        Erro("json string parse error in login!");
        return false;
    }
    string roomName = val["room"].asString();
    string name = val["name"].asString();
    RoomList *roomList = RoomList::getInstance();
    std::pair<string, Channel *> info = roomList->getOpponent(roomName, name);
    // 发送给对手下棋坐标
    Channel *op_channel = info.second;
    MsgHeader op_header;
    op_header.channel = op_channel;
    op_header.currence = op_channel->m_currence;
    std::lock_guard<std::mutex> locker(m_logicMutex);
    initSendChannel(op_channel, op_header, body, CMD_PLAY);
    return true;
}

void CLogicHandler::initSendChannel(Channel *channel, MsgHeader &msgHeader, const string &str, uint16_t msgCode)
{
    uint16_t pkgLen = _HEAD_SIZE_ + str.length();
    char *pTmpBuffer = new char[_MSG_HEAD_SIZE_ + pkgLen + 1];
    memset(pTmpBuffer, 0, _MSG_HEAD_SIZE_ + pkgLen + 1);
    channel->m_psendMemPointer = pTmpBuffer;
    // 设置消息头
    memcpy(pTmpBuffer, &msgHeader, _MSG_HEAD_SIZE_);
    pTmpBuffer += _MSG_HEAD_SIZE_;
    // 设置包头
    PkgHeader pkgHeader;
    pkgHeader.msgCode = ::htons(msgCode);
    pkgHeader.len = ::htons(pkgLen);
    memcpy(pTmpBuffer, &pkgHeader, _HEAD_SIZE_);
    // 设置发送消息
    channel->m_psendBuf = pTmpBuffer;
    pTmpBuffer += _HEAD_SIZE_;
    memcpy(pTmpBuffer, str.data(), str.length());
    // 发送数据前置
    channel->m_sendLen = pkgLen;
    msgSend(channel->m_psendMemPointer);
}

void CLogicHandler::msgSend(char *buf)
{
    m_sendQueue.push_back(buf);
    m_cond.notify_one();
}

void *CLogicHandler::serverSendFunc(void *args)
{
    ThreadItem *item = static_cast<ThreadItem *>(args);
    MsgHeader msgHeader;
    PkgHeader pkgHeader;
    CLogicHandler *handler = item->_pThis;
    std::list<char *>::iterator pos, pos2, end;
    while (!handler->m_stop)
    {
        if (item->_Running == false)
        {
            item->_Running = true;
        }
        std::unique_lock<std::mutex> lck(handler->m_sendQueueMutex);
        while (handler->m_sendQueue.size() <= 0 && !handler->m_stop)
        {
            handler->m_cond.wait(lck);
        }
        pos = handler->m_sendQueue.begin();
        end = handler->m_sendQueue.end();
        Channel *channel;
        // 这段while用于找到一个可以处理的消息
        while (pos != end)
        {
            // 提取消息头和包头
            memcpy(&msgHeader, *pos, _MSG_HEAD_SIZE_);
            memcpy(&pkgHeader, *pos + _MSG_HEAD_SIZE_, _HEAD_SIZE_);
            channel = msgHeader.channel;
            // 包过期,丢弃
            if (channel->m_currence != msgHeader.currence)
            {
                Debug("rubbish package,throw away!!!");
                pos2 = pos;
                pos++;
                handler->m_sendQueue.erase(pos2);
                delete[] channel->m_psendMemPointer;
                continue;
            }
            if (channel->m_throwSendCount > 0)
            {
                // 靠系统驱动来发送消息,这里不能再发送
                pos++;
                continue;
            }
            // 这里才去除
            handler->m_sendQueue.erase(pos);
            break;
        }
        // 说明没有找到可以处理的消息
        if (pos == end)
        {
            continue;
        }
        int sendSize = channel->sendProc();
        if (sendSize > 0)
        {
            if (sendSize == channel->m_sendLen)
            {
                // 数据全部发送
                delete[] channel->m_psendMemPointer;
                channel->m_psendMemPointer = nullptr;
                channel->m_throwSendCount = 0;
            }
            else
            {
                // 标记发送缓冲区满了
                ++channel->m_throwSendCount;
                channel->m_psendBuf += sendSize;
                channel->m_sendLen -= sendSize;
                // 数据没有全部发送
                auto server = TcpServer::getInstance();
                if ((server->epollOper(channel->m_fd, EPOLLOUT, EPOLL_CTL_MOD, 0, channel)) == -1)
                {
                    // 直接让内核发送信息
                    Erro("add write event failed!");
                }
            }
            continue;
        }
        else if (sendSize == 0)
        {
            // 连接断开
            delete[] channel->m_psendMemPointer;
            channel->m_psendMemPointer = nullptr;
            channel->m_throwSendCount = 0;
            continue;
        }
        else if (sendSize == -1)
        {
            // 标记发送缓冲区满了
            ++channel->m_throwSendCount;
            // 数据没有全部发送
            auto server = TcpServer::getInstance();
            if ((server->epollOper(channel->m_fd, EPOLLOUT, EPOLL_CTL_MOD, 0, channel)) == -1)
            {
                // 直接让内核发送信息
                Erro("add write event failed!");
            }
            continue;
        }
        else
        {
            // 这里一般是对端关闭,或者一些难以想象的错误
            delete[] channel->m_psendMemPointer;
            channel->m_psendMemPointer = nullptr;
            channel->m_throwSendCount = 0;
        }
    }
    return nullptr;
}

bool CLogicHandler::_HandleEnd(Channel *channel, MsgHeader &msgHeader, char *body, uint16_t bodyLength)
{
    if (body == nullptr)
    {
        // 这种属于恶意包,既然不是控制信息,为啥还是空包体
        Erro("invalid empty packet...");
        TcpServer::getInstance()->recycle(channel);
        return false;
    }
    Json::Reader r;
    Json::Value val;
    // json解析
    if (!r.parse(body, val))
    {
        Erro("json string parse error in login!");
        return false;
    }
    string winner = val["winner"].asString();
    string roomName = val["room"].asString();
    Debug("winner=%s,roomName=%s", winner.c_str(), roomName.c_str());
    // 获取相应的房间信息
    RoomList *roomList = RoomList::getInstance();
    UserInfo info = roomList->getRoomPlayers(roomName);
    for (auto it = info.begin(); it != info.end(); it++)
    {
        Channel *channel = it->second;
        int score = channel->m_redis->getScore(roomName, it->first);
        if (it->first == winner)
        {
            // 赢的人加3分
            score += 3;
        }
        else
        {
            score -= 3;
        }
        msgHeader.channel = channel;
        msgHeader.currence = channel->m_currence;
        // 更新分数
        channel->m_redis->updateScore(roomName, it->first, score);
        // 退出房间
        channel->m_redis->leaveRoom(roomName, it->first);
        roomList->removePlayer(roomName, it->first);
        Json::Value v;
        // 交给客户端更新分数
        v["score"] = score;
        string json = Json::FastWriter().write(v);
        std::lock_guard<std::mutex> locker(m_logicMutex);
        initSendChannel(channel, msgHeader, json, LogicCommand::CMD_END);
    }
    return true;
}