//#include "WioTMobility.h"
#include "LogManager.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <cstdio>
#include <string>


static const char* LevelToStr(CLogManager::ELevel eLevel)
{
    switch (eLevel)
    {
    case CLogManager::ELevel::None:
        return "[NONE]";
    case CLogManager::ELevel::Error:
        return "[ERROR]";
    case CLogManager::ELevel::Warning:
        return "[WARNING]";
    case CLogManager::ELevel::Inf:
        return "[INFORMATION]";
    case CLogManager::ELevel::Debug:
        return "[DEBUG]";
    }
    return "--UNKNOW--";
}

CLogManager::CLogManager()
    : m_eMaxLevel{ELevel::Inf}
{
}

CLogManager::~CLogManager()
{
}

CLogManager& CLogManager::GetInstance()
{
    static CLogManager s_Instance;
    
    return s_Instance;
}

void CLogManager::Log(ELevel eLevel, const char* zMessage, va_list args)
{
    if (eLevel > m_eMaxLevel)
    {
        return;
    }

    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());

    std::string zDate = to_simple_string(now);
    printf("\n--------------------------------------\n%s\n%s\n", zDate.c_str(),LevelToStr(eLevel));
    vprintf(zMessage, args);
    printf("\n");
}
    

void CLogManager::SetLogLevel(ELevel eLevel)
{
    m_eMaxLevel = eLevel;
}

CLogManager::ELevel CLogManager::GetLogLevel() const
{
    return m_eMaxLevel;
}
