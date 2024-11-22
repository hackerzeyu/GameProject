#include "iniparser.h"
#include <iostream>

INIParser *INIParser::getInstance()
{
    static INIParser ini;
    return &ini;
}

INIParser::~INIParser()
{
    close();
}

void INIParser::open()
{
    m_ifs.open(m_fileName);
    if (m_ifs.fail())
    {
        string error = m_fileName + " open failed!";
        throw std::logic_error(error);
    }
}

void INIParser::close()
{
    if (m_ifs.is_open())
        m_ifs.close();
}

void INIParser::init(const string &fileName)
{
    m_fileName = fileName;
    open();
    parse();
}

void INIParser::parse()
{
    string line, section;
    while (std::getline(m_ifs, line))
    {
        trim(line);
        if (line.empty())
        {
            // 空行
            continue;
        }
        if (line[0] == '[')
        {
            auto pos = line.find_first_of(']');
            if (pos == string::npos)
            {
                string error = line + ":invalid \'[\' in ini!";
                throw std::logic_error(error);
            }
            // 清空map
            m_info.clear();
            //[123]
            section = line.substr(1, pos - 1);
            trim(section);
            m_curr = section;
        }
        else if (line[0] == '#')
        {
            // 此行为注释
            continue;
        }
        else
        {
            if (m_curr.empty())
            {
                throw std::logic_error("No section define before key-value!");
            }
            auto pos = line.find_first_of('=');
            if (pos == string::npos)
            {
                string error = line + ":no '=' in sentence!";
                throw std::logic_error(error);
            }
            string key = line.substr(0, pos);
            trim(key);
            string val = line.substr(pos + 1);
            trim(val);
            // 插入键值
            m_info.insert(std::make_pair(key, val));
            m_section[m_curr] = m_info;
        }
    }
}

void INIParser::trim(string &line)
{
    int first = line.find_first_not_of(" \t\n\r");
    if (first == string::npos)
    {
        line = "";
        return;
    }
    int last = line.find_last_not_of(" \t\n\r");
    line = line.substr(first, last - first + 1);
}

string INIParser::str() const
{
    std::ostringstream oss;
    for (auto it = m_section.begin(); it != m_section.end(); it++)
    {
        oss << '[' << it->first << "]\n";
        for (auto iter = it->second.begin(); iter != it->second.end(); iter++)
        {
            oss << iter->first << "=" << iter->second << ',';
        }
        oss << '\n';
    }
    return oss.str();
}