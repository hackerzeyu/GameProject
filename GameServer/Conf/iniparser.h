#pragma once
#include <fstream>
#include <map>
#include <stdexcept>
#include <sstream>
#include <string>
using std::string;
#include "Value.h"

class INIParser
{
public:
    typedef std::map<std::string, Value> Section;
    static INIParser *getInstance();
    void init(const string &fileName);
    string str() const; // 输出配置信息,用于测试用途
    Section &operator[](const string &str) { return m_section[str]; }
    Section &operator[](const char *str) { return m_section[str]; }

private:
    INIParser() = default;
    ~INIParser();
    INIParser(const INIParser &) = delete;
    INIParser &operator=(const INIParser &) = delete;
    void open();
    void close();
    void parse();            // 解析配置文件
    void trim(string &line); // 去除两边空白字符
private:
    string m_fileName;
    std::ifstream m_ifs;

private:
    std::map<string, Section> m_section; // ini配置文件解析map
    Section m_info;                      // section内部key-value
    string m_curr;                       // 当前解析的区域
};