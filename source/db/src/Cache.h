#pragma once
#include "../../common/cache.h"

#ifdef ENABLE_SKILL_COLOR_SYSTEM
class CSKillColorCache : public cache<TSkillColor>
{
    public:
	CSKillColorCache();
	virtual ~CSKillColorCache();
	virtual void OnFlush();
};
#endif

class CItemCache : public cache<TPlayerItem>
{
public:
	CItemCache();
	virtual ~CItemCache();

	void Delete();
	virtual void OnFlush();
};

class CPlayerTableCache : public cache<TPlayerTable>
{
public:
	CPlayerTableCache();
	virtual ~CPlayerTableCache();

	virtual void OnFlush();

	DWORD GetLastUpdateTime() { return m_lastUpdateTime; }
};

// MYSHOP_PRICE_LIST

class CItemPriceListTableCache : public cache< TItemPriceListTable >
{
public:

	/// Constructor

	CItemPriceListTableCache(void);
	virtual ~CItemPriceListTableCache();

	void	UpdateList(const TItemPriceListTable* pUpdateList);

	virtual void	OnFlush(void);

private:

	static const int	s_nMinFlushSec;		///< Minimum cache expire time
};

