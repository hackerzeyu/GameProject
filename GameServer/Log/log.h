#pragma once
#include <cstring>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unistd.h>
using std::string;

#define Debug(format, ...) \
    Logger::getInstance()->log(Logger::LEVEL::LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define Info(format, ...) \
    Logger::getInstance()->log(Logger::LEVEL::LOG_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define Warn(format, ...) \
    Logger::getInstance()->log(Logger::LEVEL::LOG_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define Erro(format, ...) \
    Logger::getInstance()->log(Logger::LEVEL::LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define Fatal(format, ...) \
    Logger::getInstance()->log(Logger::LEVEL::LOG_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

class Logger
{
public:
    enum class LEVEL : char
    {
        LOG_DEBUG, // 调试
        LOG_INFO,  // 提示
        LOG_WARN,  // 警告
        LOG_ERROR, // 错误
        LOG_FATAL, // 严重
        LOG_COUNT  // 日志等级数
    };

public:
    static Logger *getInstance();                                                         // 单例模式
    void init(const string &fileName, LEVEL level = LEVEL::LOG_DEBUG, int maxLen = 1024); // 初始化日志文件                                                                     // 析构函数
    void log(LEVEL level, const string &file, int lineNumber, const char *format, ...);   // 打印日志
    void setConsole(bool console);                                                        // 设置是否显示到控制台
private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    void getTime(char *date, int len); // 获取当前时间
private:
    void open();  // 打开日志文件
    void close(); // 关闭日志文件
    void roll();  // 日志翻滚
private:
    LEVEL m_level;          // 日志等级
    string m_fileName;      // 日志文件名
    std::ofstream m_ofs;    // 文件流
    std::mutex m_logMutex;  // 日志锁
    int m_maxLen;           // 单日志最大大小
    int m_fileLen;          // 日志文件大小
    bool m_console = false; // 是否输出到控制台
private:
    static const char *logger[(int)LEVEL::LOG_COUNT]; // 日志信息数组
};