#pragma once
#include "channel.h"
#include "room.h"

using UserInfo = std::map<std::string, Channel *>;

class RoomList
{
public:
    static RoomList *getInstance();
    // 添加房间
    void createRoom(const std::string &roomName, const std::string &userName, Channel *channel);
    // 添加玩家
    void addPlayer(const std::string &roomName, const std::string &userName, Channel *channel);
    // 取出房间里的玩家信息
    UserInfo getRoomPlayers(const string &roomName);
    // 获取对手信息
    std::pair<string, Channel *> getOpponent(const string &roomName, const string &userName);
    // 移除玩家
    void removePlayer(const string &roomName, const string &userName);
    // 移除房间
    void removeRoom(const string &roomName);

private:
    RoomList() = default;
    ~RoomList() = default;
    RoomList(const RoomList &) = delete;
    RoomList &operator=(const RoomList &) = delete;

private:
    std::map<std::string, UserInfo> m_roomMap;
    std::mutex m_roomMutex;
};