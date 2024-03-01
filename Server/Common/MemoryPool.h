#ifndef	_MEMORY_POOL_H_
#define _MEMORY_POOL_H_

#include "PxDefine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

struct stMemoryPoolItem
{
	INT16	_refCount;
	UINT8	_isAddedList;
	UINT8	_Free;

	stMemoryPoolItem() noexcept
	{
		_refCount = 0;
		_isAddedList = 0;
		_Free = 0;
	}	
};

template<class T>
class _MemoryPool
{
public:
	_MemoryPool(int nSize, char* type) : m_Items(new T[nSize]), m_Type(type)
	{
		m_FreeItems.reserve(nSize);
		for (int i = 0; i < nSize; ++i)
		{
			m_FreeItems.push_back(&m_Items[i]);
		}
		m_AllocSize = nSize;
		InitializeCriticalSection(&m_csBufCritSec);
	}

	~_MemoryPool(void)
	{
		int leakCount = 0;
		for (int i = 0; i < m_AllocSize; i++)
		{
			if ( ((stMemoryPoolItem*)&m_Items[i])->_refCount >= 1 )
			{
				leakCount++;
			}
		}
		if (leakCount > 0)
		{
			// log
			std::string		fname = "logfile.txt";
			std::ofstream	fout;
			fout.open(fname.c_str(), std::ios::out | std::ios::app);
			fout << "Delete _MemoryPool Leak - Type : " << m_Type.c_str() << ", Count :" << leakCount << std::endl;
			fout.close();
		}

		delete[] m_Items;

		int		size = (int)m_OverflowItems.size();
		if (size > 0)
		{
			for (int i = 0; i < size; ++i)
			{
				T* item = m_OverflowItems[i];
				delete	item;
			}

			// log
			std::string		fname = "logfile.txt";
			std::ofstream	fout;
			fout.open(fname.c_str(), std::ios::out | std::ios::app);
			fout << "Delete _MemoryPool Overflow - Type : " << m_Type.c_str() << ", Size :" << size << std::endl;
			fout.close();
		}
		DeleteCriticalSection(&m_csBufCritSec);
	}

	T* Allocate()
	{
		T* pItem = NULL;

		EnterCriticalSection(&m_csBufCritSec);
		if (m_FreeItems.empty())
		{
			pItem = new T;
			m_OverflowItems.push_back(pItem);
		}
		else
		{
			pItem = m_FreeItems.back();
			m_FreeItems.pop_back();
		}

		if (dynamic_cast<stMemoryPoolItem*>(pItem))
		{
			((stMemoryPoolItem*)pItem)->_refCount = 1;
			((stMemoryPoolItem*)pItem)->_isAddedList = 0;
		}
		LeaveCriticalSection(&m_csBufCritSec);

		return(pItem);
	}

	void Free(T* pItem)
	{
		EnterCriticalSection(&m_csBufCritSec);
		if (dynamic_cast<stMemoryPoolItem*>(pItem))
		{
			if (((stMemoryPoolItem*)pItem)->_refCount >= 1)
			{
				--((stMemoryPoolItem*)pItem)->_refCount;
			}

			if (((stMemoryPoolItem*)pItem)->_refCount == 0)
			{
				if (((stMemoryPoolItem*)pItem)->_isAddedList == 0)
				{
					((stMemoryPoolItem*)pItem)->_isAddedList = 1;
					m_FreeItems.push_back(pItem);
				}
			}
		}
		else
		{
			m_FreeItems.push_back(pItem);
		}
		LeaveCriticalSection(&m_csBufCritSec);
	}

	void _Free(T* pItem)		// not subract refcount
	{
		EnterCriticalSection(&m_csBufCritSec);
		if (dynamic_cast<stMemoryPoolItem*>(pItem))
		{
			if (((stMemoryPoolItem*)pItem)->_refCount == 0)
			{
				if (((stMemoryPoolItem*)pItem)->_isAddedList == 0)
				{
					((stMemoryPoolItem*)pItem)->_isAddedList = 1;
					m_FreeItems.push_back(pItem);
				}
			}
		}
		else
		{
			m_FreeItems.push_back(pItem);
		}
		LeaveCriticalSection(&m_csBufCritSec);
	}

	void	AddRefCount(T* pItem)
	{
		EnterCriticalSection(&m_csBufCritSec);

		if (dynamic_cast<stMemoryPoolItem*>(pItem))
		{
			++((stMemoryPoolItem*)pItem)->_refCount;
		}

		LeaveCriticalSection(&m_csBufCritSec);
	}

	bool	SubRefCount(T* pItem)
	{
		bool result = false;
		EnterCriticalSection(&m_csBufCritSec);

		if (dynamic_cast<stMemoryPoolItem*>(pItem))
		{
			if (((stMemoryPoolItem*)pItem)->_refCount >= 1)
			{
				--((stMemoryPoolItem*)pItem)->_refCount;
			}

			if (((stMemoryPoolItem*)pItem)->_refCount == 0)
			{
				result = true;
			}
		}
		LeaveCriticalSection(&m_csBufCritSec);
		return result;
	}

private:
	std::vector< T* > m_FreeItems;
	T* m_Items;
	int		m_AllocSize;
	std::string	m_Type;
	CRITICAL_SECTION	m_csBufCritSec;

	std::vector< T* > m_OverflowItems;
};

#endif