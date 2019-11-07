// Texpool.h
// Created OW

#pragma once

#include <set>

namespace Containers {

template<class T>
class Pool
{
	typedef void (*NewElementCallback)(T *, LPVOID);

	public:
	Pool(int nInitialSize, NewElementCallback NECB, LPVOID NECBCtx)
	{
		Reset();

		m_NECB = NECB;
		m_NECBCtx = NECBCtx;

		if(nInitialSize)
		{
			GrowBy(nInitialSize);
			ShiAssert(GetAvailSize() == nInitialSize);
		}
	}

	~Pool()
	{
		Reset();
	}

	// Attributes
	protected:
	typedef std::set<T *> TSet;
	TSet m_setUsed;
	TSet m_setAvail;
	NewElementCallback m_NECB;
	LPVOID m_NECBCtx;

	#ifdef _DEBUG
	int m_nHits;
	int m_nMisses;
	#endif

	// Implementation
	void ClearSet(TSet &set)
	{
		TSet::iterator it = set.begin();
		T *val;

		while(it != set.end())
		{
			val = *it;
			if(val) delete val;
			set.erase(it);
			it = set.begin();
		}

		set.clear();
	}

	void Reset()
	{
		ClearSet(m_setUsed);
		ClearSet(m_setAvail);

		#ifdef _DEBUG
		m_nHits = 0;
		m_nMisses = 0;
		#endif
	}
	
	void GrowBy(int n)
	{
		for(int i=0;i<n;i++)
		{
			T *val = new T;

			ShiAssert(val);
			if(val == NULL)
				return;

			if(m_NECB)
				m_NECB(val, m_NECBCtx);

			m_setAvail.insert(val);
		}
	}

	void GrowTo(int n)
	{
		int nDiff = n - (m_setAvail.size() + m_setUsed.size());

		if(nDiff > 0) GrowBy(nDiff);
		else if(nDiff < 0) ShrinkBy(nDiff);

		ShiAssert(m_setAvail.size() + m_setUsed.size() == n);
	}

	void ShrinkBy(int n)
	{
		TSet::iterator it;

		for(int i=0;i<n;i++)
		{
			it = m_setAvail.begin();
			if(it == m_setAvail.end())
				return;

			delete *it;
			m_setAvail.erase(it);
		}
	}

	// Interface
	public:
	int GetUsedSize()
	{
		return m_setUsed.size();
	}

	int GetAvailSize()
	{
		return m_setAvail.size();
	}

	T *GetElement()
	{
		if(m_setAvail.size() == 0)
		{
			#ifdef _DEBUG
			m_nMisses++;
			#endif

			// GrowTo((m_setAvail.size() + m_setUsed.size()) * 2);
			GrowBy(32);
		}

		else
		{
			#ifdef _DEBUG
			m_nHits++;
			#endif
		}

		TSet::iterator it = m_setAvail.begin();
		if(it == m_setAvail.end())
			return NULL;

		T *pResult = *it;
		m_setAvail.erase(it);
		m_setUsed.insert(pResult);

		return pResult;
	}

	void FreeElement(T *val)
	{
		TSet::iterator it = m_setUsed.find(val);

		ShiAssert(it != m_setUsed.end());
		if(it == m_setUsed.end())
			return;

		T *pResult = *it;
		m_setUsed.erase(it);
		m_setAvail.insert(pResult);
	}
};

};	// namespace Containers
