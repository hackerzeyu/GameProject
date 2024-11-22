#include "roomlist.h"

RoomList *RoomList::getInstance()
{
    static RoomList roomList;
    return &roomList;
}

UserInfo RoomList::getRoomPlayers(const string &roomName)
{
    std::lock_guard<std::mutex> locker(m_roomMutex);
    auto room = m_roomMap.find(roomName);
    if (room == m_roomMap.end())
    {
        return UserInfo();
    }
    UserInfo user = room->second;
    return user;
}

void RoomList::createRoom(const std::string &roomName, const std::string &userName, Channel *channel)
{
    std::lock_guard<std::mutex> locker(m_roomMutex);
    UserInfo info;
    info.insert(std::make_pair(userName, channel));
}

void RoomList::addPlayer(const std::string &roomName, const std::string &userName, Channel *channel)
{
    std::lock_guard<std::mutex> locker(m_roomMutex);
    // 在当前map中搜索roomName
    if (m_roomMap.find(roomName) != m_roomMap.end())
    {
        auto &value = m_roomMap[roomName];
        value.insert(std::make_pair(userName, channel));
    }
    else
    {
        // 没找到
        UserInfo info;
        info.insert(std::make_pair(userName, channel));
        m_roomMap.insert(std::make_pair(roomName, info));
    }
}

std::pair<string, Channel *> RoomList::getOpponent(const string &roomName, const string &userName)
{
    auto players = getRoomPlayers(roomName);
    if (players.size() > 1)
    {
        for (auto it = players.begin(); it != players.end(); it++)
        {
            if (it->first != userName)
                return *it;
        }
    }
    return std::pair<string, Channel *>();
}

void RoomList::removePlayer(const string &roomName, const string &userName)
{
    std::lock_guard<std::mutex> locker(m_roomMutex);
    // 寻找房间
    auto item = m_roomMap.find(roomName);
    if (item != m_roomMap.end())
    {
        // 找人
        UserInfo playerInfo = item->second;
        for (auto it = playerInfo.begin(); it != playerInfo.end(); it++)
        {
            if (it->first == userName)
            {
                playerInfo.erase(it);
                break;
            }
        }
    }
}

void RoomList::removeRoom(const string &roomName)
{
    std::lock_guard<std::mutex> locker(m_roomMutex);
}