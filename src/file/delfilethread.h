#ifndef DELFILETHREAD_H
#define DELFILETHREAD_H
#include "../log/cplug_log.h"
#include <map>
#include "../thread/cplug_threadbase.h"
#include <string>
#include <mutex>
#if defined (_WIN32) || defined(_WIN64)
#include <io.h>
#elif defined(_unix_) || defined(_linux_)
#include <sys/io.h>
#endif
#include <stdio.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;

/*
����ʾ����
//����ͷ�ļ�
#include "DelFileThread.h"
//��ʼ���߳�
TDelFileThread::StartDelFileThread(); 
//���Ҫɾ����Ŀ¼
TDelFileThread::AddDelFileTask("C:\File","*.*",10,2); //ɾ��C:\File������10����ǰ���ļ����������ļ���   
//�Ƴ�Ҫɾ����Ŀ¼���Ƴ����̲߳��ٱ�����Ŀ¼��
TDelFileThread::RemoveDelFileTask("C:\File","*.*"); //ɾ��C:\File������10����ǰ���ļ����������ļ���
//ֹͣ�߳�
TDelFileThread::StopDelFileThread();
*/

//---------------------------------------------------------------------------
//·�����ļ����ͣ�����ʱ��
struct DELFILE_T
{
    string sFilePath; //�ļ�·�������� "C:\"
	string sFileType; //�ļ����ͣ����� "sav.*"�������ļ���"*.*"
    int iDelDay;          //ɾ���������������ʱ��֮ǰ���ļ����ᱻɾ��������ֵΪ5����5��ǰ���ļ��ᱻɾ�������Ϊ0�򲻻�ִ��ɾ��
    int iDelOption;       //ɾ��ѡ�0 - ɾ����Ŀ¼�µ��ļ�
                          //          1 - ɾ����Ŀ¼�µ��ļ��������ļ����ڹ��ڵ��ļ�
                          //          2 - ɾ����Ŀ¼�µ��ļ��������ļ����ڹ��ڵ��ļ����͹��ڵ����ļ���
};
//---------------------------------------------------------------------------

class TDelFileThread : public ThreadBase
{
private:
    bool m_bStop;
	int	m_nThreadParam;
	int	m_nThreadPoolParam;

    void SetName();
    map<string,DELFILE_T> m_mapFileToDel; //��ɾ���б� 
    ptime m_dtLastDel;                    //�ϴ�ִ��ʱ��
    mutex m_lock;            //ͬ����
    int DelOldFile(string sFilePath, string sFileTypes, int nDaysbefore, int nDelOption = 0);

	void StartThread();
	void StopThread();
protected:
    virtual void run();
public:
    TDelFileThread();
    virtual ~TDelFileThread();

	int AddTask(string sFilePath, string sFileType, int iDay, int iDelOption = 0);  //��ӳɹ��򷵻�1�����򷵻�0
	int RemoveTask(string sFilePath, string sFileType);                           //�Ƴ��ɹ��򷵻�1�����򷵻�0

    //ʹ�����º������е���
    static void StartDelFileThread();
    static void StopDelFileThread();  
    static int AddDelFileTask(string sFilePath,string sFileType,int iDay,int iDelOption = 0);      //��ӳɹ��򷵻�1�����ʧ�ܷ���0�����û�г����߳��򷵻�-2 �������ļ�·�����ļ����ͣ�ɾ��������ɾ���ļ�ѡ��
    static int RemoveDelFileTask(string sFilePath,string sFileType);                               //�Ƴ��ɹ��򷵻�1���Ƴ�ʧ�ܷ���0�����û�г����߳��򷵻�-2 �������ļ�·�����ļ�����
};

#endif
