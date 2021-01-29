#include "cplug_log.h"
#include "../cplug_datetime.h"
#include "../cplug_timer.hpp"
#include <mutex>
#include <map>
#include <list>
#include <fstream>  
#include <iostream>
#include "../cplug_filedir.h"
#include "../cplug_sys.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


using namespace std;

class ImpLog{
public:
    virtual ~ImpLog(){}
    virtual void writeLogLevel(LOGLEVEL loglevel, const char* msg) = 0;
    virtual void setLogPath(const char* path) = 0;  // ����Ĭ�ϵ�
    virtual void setStayDay(int stayDay) = 0;
    virtual void setPrefix(const char* prefix) = 0;
    virtual void setWriteLevel(LOGLEVEL level) = 0;
    virtual void setLogMode(const bool isInstantWrite = false) = 0;
    virtual void getLogPath(string &sLogPath) = 0;
    virtual void getLogLevel(int &iLevel) = 0;
protected:
    bool m_isInstantWrite;
    int m_iWriteLevel;
    string m_sLogPath;
    string m_sFileName;
    string m_sPrefix;
    int m_nStayDay;
};

class LogProperty{   // ��������Ĭ�ϵ����ԣ�һ��͸�������һ������־��������Щ����
public:
    static bool m_isInstantWrite;
    static int m_iWriteLevel;
    static string m_sLogPath;
    static string m_sFileName;
    static string m_sPrefix;
    static int m_nStayDay;
};
bool LogProperty::m_isInstantWrite;
int LogProperty::m_iWriteLevel;
string LogProperty::m_sLogPath;
string LogProperty::m_sFileName;
string LogProperty::m_sPrefix;
int LogProperty::m_nStayDay;



class LogBase : public ImpLog
{
private:
	LogBase(const string &sFileName);

public:
    LogBase();
	virtual ~LogBase();
    void writeLogLevel(LOGLEVEL loglevel, const char* msg);
    void setLogPath(const char* path);
    void setStayDay(int stayDay);
    void setPrefix(const char* prefix);
    void setWriteLevel(LOGLEVEL level);
    void setLogMode(const bool isInstantWrite = false);
    void getLogPath(string &sLogPath);
    void getLogLevel(int &iLevel);
private:
    void _doWriteLogToFile();
    void _writeLogFile(string sFileName,list<string> &qlLogMsg);
    void _log(LOGLEVEL logLevel, const char * sCPPFileName, int ilineNo, const char *msg, ...);
    void _makeSureLogDir();
    char _getLogLevelChar(LOGLEVEL loglevel);
    string _getLogPath();

private:
    boost::posix_time::ptime m_dtLastCheck;
    bool m_isPrintRunning;

    mutex m_aMutex;
    mutex m_WriteMutex;
    map<string,list<string>> m_qmLogMsg;
    cplug::Timer<boost::posix_time::minutes> m_timer;

private:
    void sltOnWriteLog();
    void initPrintToRun();
    void sltPrintRunningInfo();
};

class ADelLogThread
{
public:
    ADelLogThread();
    ~ADelLogThread();

    static ADelLogThread &getInstance();
    void setInfo(string sLogPath,int iStayDay);
protected:
    virtual void run();
    void removeFile();
private:
    bool m_bStop ;
    int  m_iStayDay;
    string m_sLogPath;
    boost::posix_time::ptime m_dtLastCheck;
};


// ����Ԫģʽ������һ���������������д��־���󣬹�������õ��������þ�̬��
class LogFactory
{
private:
    LogFactory(){}
    ~LogFactory(){}

public:
    static LogFactory & getInstance(){
        static LogFactory instance;
        return instance;
    }
    static ImpLog * getLogImp(const string &sPrefix = DEF_LOG_PREFIX){
        lock_guard<mutex> lock(m_mutex);
        if(sPrefix == LogProperty::m_sPrefix && (m_qmLogImp.end() != m_qmLogImp.find(DEF_LOG_PREFIX)))  // ע��ֻ��ȱʡ����־�����prefix��map���key��һ��
            return m_qmLogImp[DEF_LOG_PREFIX];
        if (m_qmLogImp.end() != m_qmLogImp.find(sPrefix))
            return m_qmLogImp[sPrefix];
        // ������Ӧ��ʵ�����
        ImpLog *p = new LogBase();   // pend �ж���������Ļ�����Ҫ��
        if(p != NULL && sPrefix != DEF_LOG_PREFIX){    // ֻ�з�Ĭ�ϵĲ���Ҫ�������
            p->setLogMode(LogProperty::m_isInstantWrite);
            p->setLogPath(LogProperty::m_sLogPath.data());
            p->setPrefix(sPrefix.data());
            p->setWriteLevel(LOGLEVEL(LogProperty::m_iWriteLevel));
            p->setStayDay(LogProperty::m_nStayDay);
        }
        m_qmLogImp.insert(pair<string, ImpLog*>(sPrefix, p));
        return p;
    }

    // ����ԭ������־�ӿڣ������ó�Ա������ȱʡֵ��Ŀǰ����������־������prefix�������һ��
    void setLogPath(const char* path, string sPrefix = DEF_LOG_PREFIX){
        if(sPrefix == DEF_LOG_PREFIX)
            LogProperty::m_sLogPath = path;
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->setLogPath(path);
    }

    void setStayDay(int stayDay, string sPrefix = DEF_LOG_PREFIX){
        if(sPrefix == DEF_LOG_PREFIX)
            LogProperty::m_nStayDay = stayDay;
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->setStayDay(stayDay);
    }

    void setPrefix(const char* prefix, string sPrefix = DEF_LOG_PREFIX){
        if(sPrefix == DEF_LOG_PREFIX)
            LogProperty::m_sPrefix = prefix; // ע�Ᵽ�����ʵ�ʵ�ǰ׺
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->setPrefix(prefix);
    }

    void setWriteLevel(LOGLEVEL level, string sPrefix = DEF_LOG_PREFIX){
        if(sPrefix == DEF_LOG_PREFIX)
            LogProperty::m_iWriteLevel = level;
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->setWriteLevel(level);
    }

    void setLogMode(const bool isInstantWrite = false, string sPrefix = DEF_LOG_PREFIX){
        if(sPrefix == DEF_LOG_PREFIX)
            LogProperty::m_isInstantWrite = isInstantWrite;
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->setLogMode(isInstantWrite);
    }

    void getLogPath(string &sLogPath, string sPrefix = DEF_LOG_PREFIX){
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->getLogPath(sLogPath);
    }

    void getLogLevel(int &iLevel, string sPrefix = DEF_LOG_PREFIX){
        ImpLog *p = getLogImp(sPrefix);
        if(NULL != p)
            p->getLogLevel(iLevel);
    }
    // ���ó�Ա������ȱʡֵ

    void logAllByLevel(const LOGLEVEL nLevel ,const string &msg)
    {
        for(auto& i : m_qmLogImp)
        {
            i.second->writeLogLevel(nLevel, msg.data());
        }
    }

private:
    static map<string, ImpLog*> m_qmLogImp;
    static mutex m_mutex;

};

map<string, ImpLog*> LogFactory::m_qmLogImp;
mutex LogFactory::m_mutex;

void writeSoftRunningInfo();

//------------------------------------------------------------------
LogBase::LogBase()
{
    m_iWriteLevel = 0;
    m_isInstantWrite = false;
    m_sLogPath = Cplug_FD_GetCurrentDir()+"/Log/";
    m_dtLastCheck = Cplug_DT_GetDiffDateTime(NOW, -1, DAY);
    _makeSureLogDir();
    m_isPrintRunning = false;
    int nTimerid = 1;
    m_timer.bind(writeSoftRunningInfo);
    m_timer.start(5);
}
//------------------------------------------------------------------
LogBase::LogBase(const string &sFileName)
{
    m_sFileName= sFileName;

    m_iWriteLevel = 0;
    m_isInstantWrite = false;
    m_sLogPath = Cplug_FD_GetCurrentDir()+"/Log/";
    m_dtLastCheck = NOW;
    _makeSureLogDir();
    m_isPrintRunning = false;

    int nTimerid = 1;
	m_timer.bind(writeSoftRunningInfo);
	m_timer.start(5);
}
//------------------------------------------------------------------
LogBase::~LogBase()
{
    _doWriteLogToFile();
}

char LogBase::_getLogLevelChar(LOGLEVEL loglevel)
{
    char c = ' ';
    switch (loglevel)
    {
    case LOG_LEVEL_TRACE:
        c = 'T';
        break;
    case LOG_LEVEL_DEBUG:
        c = 'D';
        break;
    case LOG_LEVEL_INFO:
        c = 'I';
        break;
    case LOG_LEVEL_WARN:
        c = 'W';
        break;
    case LOG_LEVEL_ERROR:
        c = 'E';
        break;
    case LOG_LEVEL_NUL:
        c = ' ';
    }

    return c;
}
//------------------------------------------------------------------
void LogBase::setLogPath(const char *sLogPath)
{
    m_sLogPath = sLogPath;
        
    if('/' != m_sLogPath.at(m_sLogPath.length() - 1) || '\\' == m_sLogPath.at(m_sLogPath.length() - 1))
        m_sLogPath += "/";

    _makeSureLogDir();
}
//------------------------------------------------------------------
void LogBase::setWriteLevel(LOGLEVEL iLevel)
{
    m_iWriteLevel = iLevel;
}
void LogBase::setLogMode(const bool isInstantWrite)
{
    m_isInstantWrite = isInstantWrite;
}

//------------------------------------------------------------------
void LogBase::sltOnWriteLog()
{
    _doWriteLogToFile();
}

void LogBase::initPrintToRun()
{
    if(false == m_isPrintRunning)
    {
        //ȷ�����յ��źź��ٿ�����ʱ��ӡ����״̬��Ϣ
        m_isPrintRunning = true;
        writeSoftRunningInfo();//��ӡһ�ΰ汾��Ϣ
    }
}

//------------------------------------------------------------------
void LogBase::_doWriteLogToFile()
{
    map<string,list<string> > qmLogMsg;

    {
        lock_guard<mutex> locker(m_aMutex);
        qmLogMsg.swap(m_qmLogMsg);
    }

    map<string,list<string> >::iterator it;
    for(it=qmLogMsg.begin();it!=qmLogMsg.end();++it)
    {
        string sFileName = it->first;
        _writeLogFile(sFileName,it->second);
    }
}
//------------------------------------------------------------------
void LogBase::_writeLogFile(string sFileName, list<string> &qlLogMsg)
{
    lock_guard<mutex> locker(m_WriteMutex);
    ofstream qfLog(sFileName, ios::out | ios::binary | ios::app);
    while(!qlLogMsg.empty())
    {
        qfLog.write(qlLogMsg.front().data(),qlLogMsg.front().length());
        qlLogMsg.pop_front();
    }
    qfLog.close();
}
//------------------------------------------------------------------
void LogBase::_makeSureLogDir()
{
    Cplug_FD_CreateDirectories(m_sLogPath);
}

string LogBase::_getLogPath()
{
    return m_sLogPath;
}

void LogBase::getLogPath(string &sLogPath)
{
    sLogPath = m_sLogPath;
}

void LogBase::getLogLevel(int &iLevel)
{
    iLevel = m_iWriteLevel;
}
//------------------------------------------------------------------
void LogBase::_log(LOGLEVEL logLevel, const char * sCPPFileName, int ilineNo, const char *msg, ...)
{
    if(logLevel<m_iWriteLevel) return;

    string sFileName = m_sLogPath;
    if(m_sPrefix.length()>0)
        sFileName = sFileName + m_sPrefix + "_" + Cplug_DT_GetFormatStrDateTime("yyyyMMdd") + ".log";
    else
        sFileName = sFileName + "Log_" + Cplug_DT_GetFormatStrDateTime("yyyyMMdd") + ".log";

    string sLogMsg = "[" + Cplug_DT_GetFormatStrDateTime("hh:mm:ss.zzz") + "]";
    char tmpstr[20] = { 0 };
    sprintf(tmpstr, "[%c][%s]", _getLogLevelChar(logLevel), Cplus_Sys_GetCurrentThreadID().c_str());

    sLogMsg += tmpstr;
    string sMsgTmp = msg;

    if (sMsgTmp.empty())
        return;

    const int SIZE = 3 * 1024;
    char strLog[SIZE + 1];
    strLog[SIZE] = {0};
    va_list list;
    va_start(list, msg);
    vsnprintf(strLog, SIZE, msg, list);
    va_end(list);
        
    sLogMsg += string(strLog) + "\r\n";
 
    {
        lock_guard<mutex> locker(m_aMutex);
        m_qmLogMsg[sFileName].push_back(sLogMsg);
    }

    sltOnWriteLog(); // ͬ��д��־
        
    if(false == m_isPrintRunning)
        initPrintToRun();
}
//------------------------------------------------------------------
void LogBase::writeLogLevel(LOGLEVEL loglevel,const char* msg)
{
    if (loglevel < m_iWriteLevel) return;

    string sFileName = m_sLogPath;
    if (m_sPrefix.length() > 0)
        sFileName = sFileName + m_sPrefix + "_" + Cplug_DT_GetFormatStrDateTime("yyyyMMdd") + ".log";
    else
        sFileName = sFileName + "Log_" + Cplug_DT_GetFormatStrDateTime("yyyyMMdd") + ".log";

    string sLogMsg = "[" + Cplug_DT_GetFormatStrDateTime("hh:mm:ss.zzz") + "]";
    char tmpstr[20] = { 0 };
    sprintf(tmpstr, "[%c][%s]", _getLogLevelChar(loglevel), Cplus_Sys_GetCurrentThreadID().c_str());

    sLogMsg += tmpstr;
    string sMsgTmp = msg;

    if (sMsgTmp.empty())
        return;

    sLogMsg += string(sMsgTmp) + "\r\n";

    {
        lock_guard<mutex> locker(m_aMutex);
        m_qmLogMsg[sFileName].push_back(sLogMsg);
    }

    sltOnWriteLog(); // ͬ��д��־

    if (false == m_isPrintRunning)
        initPrintToRun();
}
 
//------------------------------------------------------------------
void LogBase::setStayDay(int stayDay)
{

}
//------------------------------------------------------------------
void LogBase::setPrefix(const char* prefix)
{
    m_sPrefix = prefix;
}

void writeSoftRunningInfo()
{
    /*  logInfo("����汾�ţ�%s������ϵͳ�汾��%s��"
    "����ʱ����%d hours���������ʱ����%d hours���������%5���߳�����%6��CPUʹ���ʣ���ǰ����/ϵͳ����%7%/%8%"
    "�������ڴ棨����ʹ��/ϵͳʣ���ڴ�/ϵͳ���ڴ棩��%9K/%10K/%11K"
    "�������ڴ棨����ʹ��/ϵͳʣ���ڴ�/ϵͳ���ڴ棩��%12K/%13K/%14K"
    "��ϵͳ�ܽ�������%15��ϵͳ���߳�����%16��ϵͳ�ܾ������%17", */
    logInfo("Program version��%s��Program runtime��%d hours��%s��Thread cout��%d��Handle cout��%d",
        Cplus_Sys_GetSoftVersion().c_str(), Cplus_Sys_GetExeRunTime() / 3600, Cplus_Sys_GetMemoryInfo().c_str(), Cplus_Sys_GetThreadCnt(), Cplus_Sys_GetHandleCnt());
}

void setLogLevel(const int nLogLevel)
{
    LogFactory::getInstance().setWriteLevel(LOGLEVEL(nLogLevel));
}

void setLogPath(const string &sLogPath)
{
    LogFactory::getInstance().setLogPath(sLogPath.data());
}


void getLogPath(string &sLogPath)
{
    LogFactory::getInstance().getLogPath(sLogPath);
}

void getLogLevel(int &iLevel)
{
    LogFactory::getInstance().getLogLevel(iLevel);
}

void setPrefix(const string &prefix)
{
    LogFactory::getInstance().setPrefix(prefix.data());
}

void setLogMode(const bool isInstantWrite)
{
    LogFactory::getInstance().setLogMode(isInstantWrite);
}

void setStayDay(const int nDays)
{
    LogFactory::getInstance().setStayDay(nDays);
}

void logTrace(const string &msg)
{
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_TRACE, msg.data());
}

void logDebug(const string &msg)
{
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_DEBUG, msg.data());
}

void logInfo(const string &msg)
{
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_INFO, msg.data());
}

void logWarn(const string &msg)
{
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_WARN, msg.data());
}

void logError(const string &msg)
{
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_ERROR, msg.data());
}

void logTrace(const char* msg, ...)
{
    const int SIZE = 3 * 1024;
    char strLog[SIZE + 1];
    strLog[SIZE] = {0};
    va_list list;
    va_start(list, msg);
    vsnprintf(strLog, SIZE, msg, list);
    va_end(list);
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_TRACE, strLog);
}

void logDebug(const char* msg, ...)
{
    const int SIZE = 3 * 1024;
    char strLog[SIZE + 1];
    strLog[SIZE] = {0};
    va_list list;
    va_start(list, msg);
    vsnprintf(strLog, SIZE, msg, list);
    va_end(list);
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_DEBUG, strLog);
}

void logInfo(const char* msg, ...)
{
    const int SIZE = 3 * 1024;
    char strLog[SIZE + 1];
    strLog[SIZE] = {0};
    va_list list;
    va_start(list, msg);
    vsnprintf(strLog, SIZE, msg, list);
    va_end(list);
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_INFO, strLog);
}

void logWarn(const char* msg, ...)
{
    const int SIZE = 3 * 1024;
    char strLog[SIZE + 1];
    strLog[SIZE] = {0};
    va_list list;
    va_start(list, msg);
    vsnprintf(strLog, SIZE, msg, list);
    va_end(list);
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_WARN, strLog);
}

void logError(const char* msg, ...)
{
    const int SIZE = 3 * 1024;
    char strLog[SIZE + 1];
    strLog[SIZE] = {0};
    va_list list;
    va_start(list, msg);
    vsnprintf(strLog, SIZE, msg, list);
    va_end(list);
    LogFactory::getInstance().getLogImp()->writeLogLevel(LOG_LEVEL_ERROR, strLog);
}

void logTraceByPre(const string &sPrefix, const string &msg)
{
    LogFactory::getInstance().getLogImp(sPrefix)->writeLogLevel(LOG_LEVEL_TRACE, msg.data());
}

void logDebugByPre(const string &sPrefix, const string &msg)
{
    LogFactory::getInstance().getLogImp(sPrefix)->writeLogLevel(LOG_LEVEL_DEBUG, msg.data());
}

void logInfoByPre(const string &sPrefix, const string &msg)
{
    LogFactory::getInstance().getLogImp(sPrefix)->writeLogLevel(LOG_LEVEL_INFO, msg.data());
}

void logWarnByPre(const string &sPrefix, const string &msg)
{
    LogFactory::getInstance().getLogImp(sPrefix)->writeLogLevel(LOG_LEVEL_WARN, msg.data());
}

void logErrorByPre(const string &sPrefix, const string &msg)
{
    LogFactory::getInstance().getLogImp(sPrefix)->writeLogLevel(LOG_LEVEL_ERROR, msg.data());
}

void logAllByLevel(const int nLogLevel, const string &msg)
{
    LogFactory::getInstance().logAllByLevel((LOGLEVEL)nLogLevel, msg);
}

void logByLevel(const int nLogLevel, const string &msg)
{
    switch (nLogLevel)
    {
    case LOG_LEVEL_TRACE:
        logTrace(msg);
        break;
    case LOG_LEVEL_DEBUG:
        logDebug(msg);
        break;
    case LOG_LEVEL_INFO:
        logInfo(msg);
        break;
    case LOG_LEVEL_WARN:
        logWarn(msg);
        break;
    case LOG_LEVEL_ERROR:
        logError(msg);
        break;
    case LOG_LEVEL_NUL:
        break;
    default:
        break;
    }
}

