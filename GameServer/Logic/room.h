#pragma once
#include <sw/redis++/redis.h>
#include "iniparser.h"
#include "log.h"

/*
 * 三个结构
 * 1.set：OnePlayer,TwoPlayer,Invalid->roomName
 * 2.sortedset：roomName->{userName,score}
 * 3.hash：players{userName->roomName};      //便于查找玩家所在的房间名
 */
class Room
{
public:
    Room() = default;
    ~Room();
    // 清空数据
    void clear();
    // 初始化环境
    bool connectToRedis();
    // 创建新房间
    string createRoom(const std::string &userName, int score);
    // 进入房间
    string joinRoom(const std::string &userName);
    bool joinRoom(const std::string &userName, const string &roomName);
    // 获取新的房间名称
    string getNewRoomName();
    // 搜索房间名称
    bool searchRoom(const std::string &roomName);
    // 离开房间
    void leaveRoom(const std::string &roomName, const std::string &userName);
    // 获取所在房间名
    string getRoomIn(const std::string &userName);
    // 获取成员分数
    int getScore(const std::string &roomName, const std::string &userName);
    // 更新分数
    void updateScore(const std::string &roomName, const std::string &userName, int score);

private:
    sw::redis::Redis *m_redis;
    const std::string OnePlayer = "OnePlayer";
    const std::string TwoPlayer = "TwoPlayer";
    const std::string Invalid = "Invalid";
};