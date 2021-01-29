#include "memoryqueue.h"
#include <iostream>
#include<string.h>

MemoryQueue::MemoryQueue(string sQueueName, char* pMemory, unsigned int nSize, bool bAutoDel) : m_sQueueName(sQueueName), m_pMemory(pMemory), m_nQueueSize(nSize), m_bAutoDelete(bAutoDel)
{
	std::lock_guard<std::mutex> lock(m_lock);
	if (sizeof(T_SHARED_MEMORY_HEAD) + sizeof(T_MESSAGE_HEAD) + 1 > nSize)
	{
		//logError("�����ڴ�С��%d, �����Կ��ٹ����ڴ�", sizeof(T_SHARED_MEMORY_HEAD) + sizeof(T_MESSAGE_HEAD));
	}

	//��ʼ�������ڴ�ͷ����Ϣ
	T_SHARED_MEMORY_HEAD* pMemoryHead = (T_SHARED_MEMORY_HEAD*)pMemory;
	pMemoryHead->nHeadSize = sizeof(T_SHARED_MEMORY_HEAD);
	pMemoryHead->nBodySize = nSize - sizeof(T_SHARED_MEMORY_HEAD);
	pMemoryHead->nReadIndex = 0;
	pMemoryHead->nWriteIndex = 0;
	pMemoryHead->nLeftSize = nSize - sizeof(T_SHARED_MEMORY_HEAD);
	printInfo(pMemoryHead);
}

MemoryQueue::~MemoryQueue()
{
	if (m_bAutoDelete && m_pMemory)
	{
		delete m_pMemory;
		m_pMemory = NULL;
	}
}

bool MemoryQueue::pushMsg(const char* pMsg, unsigned int nSize)
{
	std::lock_guard<std::mutex> lock(m_lock);

	//�ȶ�ȡͷ����Ϣ
	T_SHARED_MEMORY_HEAD* pMemoryHead = (T_SHARED_MEMORY_HEAD*)m_pMemory;
	unsigned nTotalPushSize = nSize + sizeof(T_MESSAGE_HEAD);

	T_MESSAGE_HEAD msgHead = { nSize };

	//ѭ��������Ҫ����һ���ֽڷ����ж�Ϊ�Ƿ�Ϊ��
	if (pMemoryHead->nLeftSize >= nTotalPushSize + 1)
	{
		//д���ݴ�С
		__writeMemory(pMemoryHead, (char*)(&msgHead), sizeof(T_MESSAGE_HEAD));

		//д��������
		__writeMemory(pMemoryHead, pMsg, nSize);

		return true;
	}
	else
	{
		//logError("ʣ�๲���ڴ�(%d), ��Ҫ���ڴ������ڴ�(%d)),���¿ռ䲻��", pMemoryHead->nLeftSize, nTotalPushSize);
		printInfo(pMemoryHead);
		return false;
	}
}

void MemoryQueue::__writeMemory(T_SHARED_MEMORY_HEAD* pMemoryHead, const char* pMsg, unsigned int nSize)
{
	char* pMsgHead = (char*)pMemoryHead + sizeof(T_SHARED_MEMORY_HEAD) + pMemoryHead->nWriteIndex;

	if (pMemoryHead->nWriteIndex + nSize > pMemoryHead->nBodySize - 1)
	{
		unsigned int leftSize = pMemoryHead->nBodySize - pMemoryHead->nWriteIndex;
		memcpy(pMsgHead, pMsg, leftSize);
		memcpy((char*)pMemoryHead + sizeof(T_SHARED_MEMORY_HEAD), pMsg + leftSize, nSize - leftSize);
		pMemoryHead->nWriteIndex = nSize - leftSize;
		pMemoryHead->nLeftSize = __getLeftSize(pMemoryHead);
	}
	else
	{
		memcpy(pMsgHead, pMsg, nSize);
		pMemoryHead->nWriteIndex = pMemoryHead->nWriteIndex + nSize;
		pMemoryHead->nLeftSize = __getLeftSize(pMemoryHead);
	}
	printInfo(pMemoryHead);
}

bool MemoryQueue::popMsg(vector<char>& vec)
{
	std::lock_guard<std::mutex> lock(m_lock);

	//�ȶ�ȡͷ����Ϣ
	T_SHARED_MEMORY_HEAD* pMemoryHead = (T_SHARED_MEMORY_HEAD*)m_pMemory;

	if (pMemoryHead->nReadIndex == pMemoryHead->nWriteIndex)
	{
		//std::cout << "��ǰ�޿ɶ�����" << endl;
		return false;
	}

	T_MESSAGE_HEAD msgHead;
	__readMemory(pMemoryHead, (char*)&msgHead, sizeof(T_MESSAGE_HEAD));

	vec.clear();
	vec.resize(msgHead.nMsgHeadSize);
	__readMemory(pMemoryHead, vec.data(), msgHead.nMsgHeadSize);
	return true;
}

bool MemoryQueue::isEmpty()
{
	std::lock_guard<std::mutex> lock(m_lock);
	if (((T_SHARED_MEMORY_HEAD*)m_pMemory)->nReadIndex == ((T_SHARED_MEMORY_HEAD*)m_pMemory)->nWriteIndex)
	{
		//logTrace("��ǰ�޿ɶ�����");
		return true;
	}
	return false;
}

void MemoryQueue::__readMemory(T_SHARED_MEMORY_HEAD* pMemoryHead, char* pMsg, unsigned int nSize)
{
	const char* pReadHead = (char*)pMemoryHead + sizeof(T_SHARED_MEMORY_HEAD) + pMemoryHead->nReadIndex;
	if (pMemoryHead->nReadIndex + nSize > pMemoryHead->nBodySize - 1)
	{
		unsigned int leftSize = pMemoryHead->nBodySize - pMemoryHead->nReadIndex;
		
		memcpy(pMsg, pReadHead, leftSize);
		memcpy(pMsg + leftSize, (char*)pMemoryHead + sizeof(T_SHARED_MEMORY_HEAD), nSize - leftSize);

		pMemoryHead->nReadIndex = nSize - leftSize;
		pMemoryHead->nLeftSize = __getLeftSize(pMemoryHead);
	}
	else
	{
		memcpy(pMsg, pReadHead, nSize);
		pMemoryHead->nReadIndex = pMemoryHead->nReadIndex + nSize;
		pMemoryHead->nLeftSize = __getLeftSize(pMemoryHead);
	}
}

void MemoryQueue::printInfo(T_SHARED_MEMORY_HEAD* pMemoryHead)
{
	//logTrace("******Address:%p, Head Size:%d, Body Size:%d, Read Index:%d, Write Index:%d, Left Size:%d", 
	//	pMemoryHead, pMemoryHead->nHeadSize, pMemoryHead->nBodySize, pMemoryHead->nReadIndex, pMemoryHead->nWriteIndex, pMemoryHead->nLeftSize);
}

void MemoryQueue::setReadIndex(unsigned int index)
{
	std::lock_guard<std::mutex> lock(m_lock);

	if (m_pMemory)
		((T_SHARED_MEMORY_HEAD*)m_pMemory)->nReadIndex = index;
}

void MemoryQueue::setWriteIndex(unsigned int index)
{
	std::lock_guard<std::mutex> lock(m_lock);

	if (m_pMemory)
		((T_SHARED_MEMORY_HEAD*)m_pMemory)->nWriteIndex = index;
}


unsigned int MemoryQueue::__getLeftSize(T_SHARED_MEMORY_HEAD* pMemoryHead)
{
	if (pMemoryHead->nReadIndex == pMemoryHead->nWriteIndex)
	{
		return pMemoryHead->nBodySize;
	}
	else
	{
		return (pMemoryHead->nBodySize - (pMemoryHead->nWriteIndex - pMemoryHead->nReadIndex)) % pMemoryHead->nBodySize;
	}
}

TunnelManger::TunnelManger()
{
}

TunnelManger::~TunnelManger()
{
	for (auto it = m_mpTunnels.begin(); it != m_mpTunnels.end();)
	{
		string sQueName = it->first;
		if (it->second)
			delete it->second;
		m_mpTunnels.erase(it++);
		//logInfo("ɾ���ڴ����(%s)�ɹ�", sQueName);
	}
}

MemoryQueue* TunnelManger::Regist(const string& sQueName, unsigned int nSize, MEMORY_TYPE type)
{
	if (m_mpTunnels.find(sQueName) != m_mpTunnels.end())
	{
		//logError("�ڴ����(%s)�Ѿ����ڣ�ע��ʧ��!", sQueName.c_str());
		return m_mpTunnels[sQueName];
	}
	char* pMemory = new char[nSize];
	MemoryQueue* memque = new MemoryQueue(sQueName, pMemory, nSize);
	if (memque && pMemory)
		m_mpTunnels[sQueName] = memque;
	return memque;
}

bool TunnelManger::pushMsg(const string& sQueName, const char* pMsg, unsigned int nSize)
{
	if (m_mpTunnels.find(sQueName) != m_mpTunnels.end())
	{
		//logError("�ڴ����(%s)�����ڣ���Ϣ����ʧ��!", sQueName.c_str());
		return m_mpTunnels[sQueName]->pushMsg(pMsg, nSize);
	}
	return false;
}

bool TunnelManger::popMsg(const string& sQueName, vector<char>& vec)
{
	if (m_mpTunnels.find(sQueName) != m_mpTunnels.end())
	{
		//logError("�ڴ����(%s)�����ڣ���Ϣ��ȡʧ��!", sQueName.c_str());
		return m_mpTunnels[sQueName]->popMsg(vec);
	}
	return false;
}

MemoryQueue* TunnelManger::getMemQue(const string& sQueName)
{
	if (m_mpTunnels.find(sQueName) != m_mpTunnels.end())
	{
		//logError("�ڴ����(%s)�����ڣ���Ϣ����ʧ��!", sQueName.c_str());
		return m_mpTunnels[sQueName];
	}
	return nullptr;
}
