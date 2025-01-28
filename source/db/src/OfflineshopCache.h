#ifndef __INCLUDE_HEADER_OFFLINESHOP_CACHE__
#define __INCLUDE_HEADER_OFFLINESHOP_CACHE__

#ifdef ENABLE_OFFLINE_SHOP
namespace offlineshop
{
	template <class T>
	void ZeroObject(T& obj)
	{
		obj = {};
		// memset(&obj, 0, sizeof(obj));
	}

	template <class T>
	void CopyObject(T& objDest, const T& objSrc)
	{
		memcpy(&objDest, &objSrc, sizeof(objDest));
	}

	template <class T>
	void CopyContainer(T& objDest, const T& objSrc)
	{
		objDest = objSrc;
	}

	template <class T>
	void DeletePointersContainer(T& obj)
	{
		typename T::iterator it = obj.begin();
		for (; it != obj.end(); it++)
			delete(*it);
	}

	struct SQueryInfoAddItem;
	struct SQueryInfoCreateShop;

	class CShopCache
	{
	public:
		typedef struct SShopCacheItemInfo
		{
			TPriceInfo		price;
			TItemInfoEx		item;
			bool			bLock;

			SShopCacheItemInfo()
			{
				ZeroObject(price);
				ZeroObject(item);

				bLock = false;
			}
		} TShopCacheItemInfo;

		typedef std::map<DWORD, TShopCacheItemInfo> SHOPCACHE_MAP;
		typedef struct SShopCacheInfo
		{
			DWORD	dwDuration;
			char	szName[OFFLINE_SHOP_NAME_MAX_LEN];
#ifdef ENABLE_SHOP_DECORATION
			DWORD	dwShopDecoration;
#endif
			SHOPCACHE_MAP 	itemsmap;
#ifdef ENABLE_IRA_REWORK
			TShopPosition	posDetails;
#endif
		} TShopCacheInfo;

		typedef std::map<DWORD, TShopCacheItemInfo>::iterator ITEM_ITER;

	public:
		CShopCache();
		~CShopCache();

		bool		Get(DWORD dwOwnerID, TShopCacheInfo** ppCache) const;
		bool		AddItem(DWORD dwOwnerID, const TShopCacheItemInfo& rItem);
		bool		RemoveItem(DWORD dwOwnerID, DWORD dwItemID);
		bool		SellItem(DWORD dwOwnerID, DWORD dwItemID);
		bool		LockSellItem(DWORD dwOwnerID, DWORD dwItemID);
		bool		UnlockSellItem(DWORD dwOwnerID, DWORD dwItemID);
		bool		EditItem(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& rItemPrice);
		bool		CloseShop(DWORD dwOwnerID);
#ifdef ENABLE_IRA_REWORK
		bool		CreateShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName, std::vector<TShopCacheItemInfo>& items, TShopPosition pos
#ifdef ENABLE_SHOP_DECORATION
		, DWORD dwShopDecoration
#endif
		);
#else
		bool		CreateShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName, std::vector<TShopCacheItemInfo>& items);
#endif
		bool		CreateShopAddItem(SQueryInfoCreateShop* qi, const TShopCacheItemInfo& rItem);
		bool		ChangeShopName(DWORD dwOwnerID, const char* szName);
		bool		PutItem(DWORD dwOwnerID, DWORD dwItemID, const TShopCacheItemInfo& rItem);
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
		bool		ExtendShopTime(DWORD dwOwnerID, DWORD dwTime);
#endif
#ifdef ENABLE_IRA_REWORK
		bool		PutShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName, TShopPosition pos
#ifdef ENABLE_SHOP_DECORATION
		, DWORD dwShopDecoration
#endif
		);
#else
		bool		PutShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName);
#endif
		DWORD		GetCount() const { return m_shopsMap.size(); }
		void		EncodeCache(CPeer* peer) const;
		DWORD		GetItemCount() const;

		void		ShopDurationProcess();
		void		UpdateDurationQuery(DWORD dwOwnerID, const TShopCacheInfo& rShop);

	private:
		typedef std::map<DWORD, TShopCacheInfo>::iterator CACHEITER;
		typedef std::map<DWORD, TShopCacheInfo>::const_iterator CONST_CACHEITER;
		std::map<DWORD, TShopCacheInfo> m_shopsMap;
	};

	class CSafeboxCache
	{
	public:
		typedef struct SSafeboxCacheInfo
		{
			unsigned long long valutes;
			std::map<DWORD, TItemInfoEx>
				itemsmap;
		} TSafeboxCacheInfo;

		typedef std::map<DWORD, TSafeboxCacheInfo> CACHEMAP;
		typedef std::map<DWORD, TSafeboxCacheInfo>::iterator CHACHEITER;
		typedef std::map<DWORD, TSafeboxCacheInfo>::const_iterator CHACHECONSTITER;

	public:
		CSafeboxCache();
		~CSafeboxCache();

		bool				Get(DWORD dwOwnerID, TSafeboxCacheInfo** ppSafebox) const;
		bool				PutSafebox(DWORD dwOwnerID, const TSafeboxCacheInfo& rSafebox);
		bool				PutItem(DWORD dwOwnerID, DWORD dwItem, const TItemInfoEx& item);
		bool				RemoveItem(DWORD dwOwner, DWORD dwItemID);
		bool				AddItem(DWORD dwOwnerID, const TItemInfoEx& item);
		bool				AddValutes(DWORD dwOwnerID, const unsigned long long& val);
		bool				RemoveValutes(DWORD dwOwnerID, const unsigned long long& val);
		void				AddValutesAsQuery(DWORD dwOwnerID, const unsigned long long& val);
		void				ItemExpirationProcess();

		TSafeboxCacheInfo* CreateSafebox(DWORD dwOwnerID);
		DWORD				GetCount() const { return m_safeboxMap.size(); }
		DWORD				GetItemCount() const;

		TSafeboxCacheInfo* LoadSafebox(DWORD dwPID);

	private:
		CACHEMAP	m_safeboxMap;
	};

	struct SQueryInfoAddItem
	{
		DWORD dwOwnerID;
		CShopCache::TShopCacheItemInfo item;
	};

	struct SQueryInfoCreateShop
	{
		DWORD dwOwnerID;
		DWORD dwDuration;
		char  szName[OFFLINE_SHOP_NAME_MAX_LEN];
#ifdef ENABLE_SHOP_DECORATION
		DWORD	dwShopDecoration;
#endif
		std::vector<CShopCache::TShopCacheItemInfo> items;
		std::vector<DWORD> ids;
		DWORD dwItemIndex;
#ifdef ENABLE_IRA_REWORK
		TShopPosition pos;
#endif
	};

	struct SQueryInfoSafeboxAddItem
	{
		DWORD		dwOwnerID;
		TItemInfoEx item;
	};
}

#endif
#endif