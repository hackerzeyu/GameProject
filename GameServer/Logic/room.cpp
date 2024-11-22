#include "room.h"
#include <random>

bool Room::connectToRedis()
{
    auto ini = INIParser::getInstance();
    string ip = (*ini)["redis"]["ip"];
    int port = (*ini)["redis"]["port"];
    string connStr = "tcp://" + ip + ":" + std::to_string(port);
    m_redis = new sw::redis::Redis(connStr);
    // 测试链接
    if (m_redis->ping() == "PONG")
    {
        Debug("connect redis successfully...");
        return true;
    }
    return false;
}

Room::~Room()
{
    if (m_redis)
    {
        delete m_redis;
    }
}

void Room::clear()
{
    // flushdb
    m_redis->flushdb();
}

string Room::createRoom(const std::string &userName, int score)
{
    std::string roomName = getNewRoomName();
    joinRoom(userName, roomName);
    updateScore(roomName, userName, score);
    return roomName;
}

string Room::joinRoom(const std::string &userName)
{
    std::optional<std::string> room;
    do
    {
        Debug("oneplayer num:%d", m_redis->scard(OnePlayer));
        if (m_redis->scard(OnePlayer) > 0)
        {
            room = m_redis->srandmember(OnePlayer);
            break;
        }
        room = getNewRoomName();
    } while (0);
    joinRoom(userName, room.value());
    return room.value();
}

string Room::getNewRoomName()
{
    // 创建随机设备对象
    std::random_device rd;
    // 创建随机数生成对象
    std::mt19937 gen(rd());
    // 创建随机数分布对象 -> 均匀分布
    std::uniform_int_distribution<int> dis(100000, 999999);
    int randNum = dis(gen);
    return std::to_string(randNum);
}

bool Room::joinRoom(const std::string &userName, const string &roomName)
{
    if (m_redis->zcard(roomName) >= 2)
    {
        return false;
    }
    // 检查房间是否存在
    if (!m_redis->exists(roomName))
    {
        Debug("new room create!");
        m_redis->sadd(OnePlayer, roomName);
    }
    // 移动房间
    else if (m_redis->sismember(OnePlayer, roomName))
    {
        m_redis->smove(OnePlayer, TwoPlayer, roomName);
    }
    else
    {
        Erro("join a unknown room!");
        return false;
    }
    m_redis->zadd(roomName, userName, 0);
    m_redis->hset("players", userName, roomName);
    return true;
}

bool Room::searchRoom(const std::string &roomName)
{
    return m_redis->sismember(OnePlayer, roomName);
}

string Room::getRoomIn(const std::string &userName)
{
    auto value = m_redis->hget("players", userName);
    if (value.has_value())
    {
        return value.value();
    }
    return std::string();
}

int Room::getScore(const std::string &roomName, const std::string &userName)
{
    auto score = m_redis->zscore(roomName, userName);
    if (score.has_value())
        return score.value();
    return 0;
}

void Room::updateScore(const std::string &roomName, const std::string &userName, int score)
{
    // 更新分数
    m_redis->zadd(roomName, userName, score);
}

void Room::leaveRoom(const std::string &roomName, const std::string &userName)
{
    if (m_redis->sismember(TwoPlayer, roomName))
    {
        m_redis->smove(TwoPlayer, Invalid, roomName);
    }
    // 从房间中删除玩家
    m_redis->zrem(roomName, userName);
    auto count = m_redis->zcard(roomName);
    m_redis->hdel("players", userName);
    // 删除房间
    if (count == 0)
    {
        m_redis->del(roomName);
        m_redis->srem(Invalid, roomName);
    }
}