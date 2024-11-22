#include "log.h"
#include <chrono>
#include <cstdarg>
#include <iostream>
#include <sys/stat.h>

const char *Logger::logger[(int)LEVEL::LOG_COUNT] =
    {
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL"};

Logger *Logger::getInstance()
{
    static Logger logger;
    return &logger;
}

Logger::~Logger()
{
    close();
}

void Logger::init(const string &fileName, LEVEL level, int maxLen)
{
    m_fileName = fileName;
    m_level = level;
    m_maxLen = maxLen;
    // 打开日志文件
    open();
}

void Logger::open()
{
    m_ofs.open(m_fileName, std::ios::app);
    if (m_ofs.fail())
    {
        throw std::logic_error("file open failed!");
    }
    // 翻滚后文件大小要初始化
    struct stat fileStat;
    if (stat(m_fileName.data(), &fileStat) == 0)
    {
        m_fileLen = fileStat.st_size;
    }
    else
    {
        throw std::logic_error("stat file failed!");
    }
}

void Logger::close()
{
    if (m_ofs.is_open())
        m_ofs.close();
}

void Logger::setConsole(bool console)
{
    m_console = console;
}

void Logger::getTime(char *date, int len)
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&now_time);
    strftime(date, len, "%Y-%m-%d.%H:%M:%S", localTime);
}

void Logger::log(LEVEL level, const string &file, int lineNumber, const char *format, ...)
{
    if (!m_ofs.is_open())
    {
        throw std::logic_error("file is not open,please open before log!");
    }
    // 日志等级过低
    if (m_level > level)
        return;
    std::ostringstream oss;
    char date[20] = "";
    getTime(date, sizeof(date));
    int len = snprintf(nullptr, 0, "[%s]%s@%s:%d ", logger[(int)level], date, file.data(), lineNumber);
    if (len > 0)
    {
        char *temp = new char[len + 1];
        snprintf(temp, len + 1, "[%s]%s@%s:%d ", logger[(int)level], date, file.data(), lineNumber);
        temp[len] = '\0';
        oss << temp;
        delete[] temp;
    }
    // 解析参数列表
    va_list args;
    va_start(args, format);
    len = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    if (len > 0)
    {
        char *temp = new char[len + 1];
        va_start(args, format);
        vsnprintf(temp, len + 1, format, args);
        va_end(args);
        temp[len] = '\0';
        oss << temp << '\n';
        delete[] temp;
    }
    std::unique_lock<std::mutex> lck(m_logMutex);
    m_ofs << oss.str();
    m_ofs.flush();
    if (m_console)
        std::cout << oss.str();
    m_fileLen += oss.str().length();
    if (m_fileLen >= m_maxLen)
    {
        // 防止日志打印过快导致日志重名
        sleep(1);
        // 日志翻滚
        roll();
    }
}

void Logger::roll()
{
    close();
    char date[20] = "";
    getTime(date, sizeof(date));
    char newName[40] = "";
    snprintf(newName, sizeof(newName), "../server%s.log", date);
    // 文件重命名
    if (rename(m_fileName.data(), newName) != 0)
    {
        throw std::logic_error("file rename failed!");
    }
    open();
}