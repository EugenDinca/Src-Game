#include "stdafx.h"

#ifdef ENABLE_OFFLINE_SHOP
#include "../../common/building.h"
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "ClientManager.h"
#include "OfflineshopCache.h"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
namespace offlineshop
{
	std::string CreateShopCacheInsertItemQuery(DWORD dwOwner, const CShopCache::TShopCacheItemInfo& rItem);
	std::string CreateShopCacheUpdateItemQuery(DWORD dwItemID, const TPriceInfo& rItemPrice);
	std::string CreateShopCacheDeleteShopQuery(DWORD dwOwner);
	std::string CreateShopCacheDeleteShopItemQuery(DWORD dwOwner);
#ifdef ENABLE_IRA_REWORK
	std::string CreateShopCacheInsertShopQuery(DWORD dwOwnerID, DWORD dwDuration, const char* name, TShopPosition pos
#ifdef ENABLE_SHOP_DECORATION
	, DWORD dwShopDecoration
#endif
	);
#else
	std::string CreateShopCacheInsertShopQuery(DWORD dwOwnerID, DWORD dwDuration, const char* name);
#endif
	std::string CreateShopCacheUpdateShopNameQuery(DWORD dwOwnerID, const char* name);
	std::string CreateShopCacheUpdateDurationQuery(DWORD dwOwnerID, DWORD dwDuration);
	std::string CreateShopCacheDeleteItemQuery(DWORD dwOwnerID, DWORD dwItemID);
	std::string CreateSafeboxCacheDeleteItemQuery(DWORD dwItem);
	std::string CreateSafeboxCacheInsertItemQuery(DWORD dwOwner, const TItemInfoEx& item);
	std::string CreateSafeboxCacheUpdateValutes(DWORD dwOwner, const unsigned long long& val);
	std::string CreateSafeboxCacheInsertSafeboxValutesQuery(DWORD dwOwnerID);
	std::string CreateSafeboxCacheUpdateValutesByAdding(DWORD dwOwner, const unsigned long long& val);
	std::string CreateSafeboxCacheLoadItemsQuery(DWORD dwOwnerID);
	std::string CreateSafeboxCacheLoadValutesQuery(DWORD dwOwnerID);
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	std::string CreateShopCacheUpdateShopTimeQuery(DWORD dwOwnerID, DWORD dwTime);
#endif

	CShopCache::CShopCache()
	{
	}

	CShopCache::~CShopCache()
	{
	}

	bool CShopCache::Get(DWORD dwOwnerID, TShopCacheInfo** ppCache) const
	{
		CONST_CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		*ppCache = (TShopCacheInfo*)&(it->second);
		return true;
	}

	bool CShopCache::AddItem(DWORD dwOwnerID, const TShopCacheItemInfo& rItem)
	{
		TShopCacheInfo* pCache;
		if (!Get(dwOwnerID, &pCache))
			return false;

		SQueryInfoAddItem* qi = new SQueryInfoAddItem;
		qi->dwOwnerID = dwOwnerID;
		CopyObject(qi->item, rItem);

		std::string query = CreateShopCacheInsertItemQuery(dwOwnerID, rItem);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_ADD_ITEM, 0, qi);
		return true;
	}

	bool CShopCache::RemoveItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		TShopCacheInfo* pCache;
		if (!Get(dwOwnerID, &pCache))
			return false;

		std::map<DWORD, TShopCacheItemInfo>::iterator it = pCache->itemsmap.find(dwItemID);
		if (it == pCache->itemsmap.end())
			return false;

		if (it->second.bLock)
			return false;
		pCache->itemsmap.erase(it);

		std::string query = CreateShopCacheDeleteItemQuery(dwOwnerID, dwItemID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_REMOVE_ITEM, 0, NULL);

		if (pCache->itemsmap.empty())
			CClientManager::instance().OfflineshopExpiredShop(dwOwnerID);

		return true;
	}

	bool CShopCache::SellItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		TShopCacheInfo* pCache;
		if (!Get(dwOwnerID, &pCache))
			return false;

		std::map<DWORD, TShopCacheItemInfo>::iterator it = pCache->itemsmap.find(dwItemID);
		if (it == pCache->itemsmap.end())
			return false;

		if (!it->second.bLock)
			return false;

		pCache->itemsmap.erase(it);

		std::string query = CreateShopCacheDeleteItemQuery(dwOwnerID, dwItemID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_REMOVE_ITEM, 0, NULL);

		if (pCache->itemsmap.empty())
			CClientManager::instance().OfflineshopExpiredShop(dwOwnerID);

		return true;
	}

	bool CShopCache::LockSellItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		TShopCacheInfo* pCache;
		if (!Get(dwOwnerID, &pCache))
			return false;

		std::map<DWORD, TShopCacheItemInfo>::iterator it = pCache->itemsmap.find(dwItemID);
		if (it == pCache->itemsmap.end())
			return false;

		if (it->second.bLock)
			return false;

		it->second.bLock = true;
		return true;
	}

	bool CShopCache::UnlockSellItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		TShopCacheInfo* pCache;
		if (!Get(dwOwnerID, &pCache))
			return false;

		std::map<DWORD, TShopCacheItemInfo>::iterator it = pCache->itemsmap.find(dwItemID);
		if (it == pCache->itemsmap.end())
			return false;

		if (!it->second.bLock)
			return false;

		it->second.bLock = false;
		return true;
	}

	bool CShopCache::EditItem(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& rItemPrice)
	{
		TShopCacheInfo* pCache;
		if (!Get(dwOwnerID, &pCache))
			return false;

		std::map<DWORD, TShopCacheItemInfo>::iterator it = pCache->itemsmap.find(dwItemID);
		if (it == pCache->itemsmap.end())
			return false;

		if (it->second.bLock)
			return false;

		TShopCacheItemInfo& rItem = it->second;
		CopyObject(rItem.price, rItemPrice);

		std::string query = CreateShopCacheUpdateItemQuery(dwItemID, rItemPrice);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_EDIT_ITEM, 0, NULL);
		return true;
	}

	bool CShopCache::CloseShop(DWORD dwOwnerID)
	{
		CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		m_shopsMap.erase(it);

		std::string query = CreateShopCacheDeleteShopQuery(dwOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_DELETE_SHOP, 0, NULL);

		query = CreateShopCacheDeleteShopItemQuery(dwOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_DELETE_SHOP_ITEM, 0, NULL);
		return true;
	}

#ifdef ENABLE_IRA_REWORK
	bool CShopCache::CreateShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName, std::vector<TShopCacheItemInfo>& items, TShopPosition pos
#ifdef ENABLE_SHOP_DECORATION
	, DWORD dwShopDecoration
#endif
	)
#else
	bool CShopCache::CreateShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName, std::vector<TShopCacheItemInfo>& items)
#endif
	{
		CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it != m_shopsMap.end())
			return false;

		SQueryInfoCreateShop* qi = new SQueryInfoCreateShop;
		qi->dwOwnerID = dwOwnerID;
		qi->dwDuration = dwDuration;
#ifdef ENABLE_SHOP_DECORATION
		qi->dwShopDecoration = dwShopDecoration;
#endif
		strncpy(qi->szName, szName, sizeof(qi->szName));
		CopyContainer(qi->items, items);
#ifdef ENABLE_IRA_REWORK
		CopyContainer(qi->pos, pos);
		std::string query = CreateShopCacheInsertShopQuery(dwOwnerID, dwDuration, szName, pos
#ifdef ENABLE_SHOP_DECORATION
		, dwShopDecoration
#endif
		);
#else
		std::string query = CreateShopCacheInsertShopQuery(dwOwnerID, dwDuration, szName);
#endif
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_CREATE_SHOP, 0, qi);
		return true;
	}

	bool CShopCache::CreateShopAddItem(SQueryInfoCreateShop* qi, const TShopCacheItemInfo& rItem)
	{
		CACHEITER it = m_shopsMap.find(qi->dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		std::string query = CreateShopCacheInsertItemQuery(qi->dwOwnerID, rItem);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_CREATE_SHOP_ADD_ITEM, 0, qi);
		return true;
	}

	bool CShopCache::ChangeShopName(DWORD dwOwnerID, const char* szName)
	{
		CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		TShopCacheInfo& rShop = it->second;
		strncpy(rShop.szName, szName, sizeof(rShop.szName));

		std::string query = CreateShopCacheUpdateShopNameQuery(dwOwnerID, szName);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SHOP_CHANGE_NAME, 0, NULL);
		return true;
	}

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	bool CShopCache::ExtendShopTime(DWORD dwOwnerID, DWORD dwTime)
	{
		CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		TShopCacheInfo& rShop = it->second;
		rShop.dwDuration += dwTime;

		std::string query = CreateShopCacheUpdateShopTimeQuery(dwOwnerID, dwTime);
		CDBManager::instance().DirectQuery(query.c_str());
		return true;
	}
#endif

	bool CShopCache::PutItem(DWORD dwOwnerID, DWORD dwItemID, const TShopCacheItemInfo& rItem)
	{
		CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it == m_shopsMap.end())
			return false;

		TShopCacheInfo& rShop = it->second;
		SHOPCACHE_MAP& rMap = rShop.itemsmap;

		if (rMap.find(dwItemID) != rMap.end())
			return false;

		rMap.insert(std::make_pair(dwItemID, rItem));
		return true;
	}

#ifdef ENABLE_IRA_REWORK
	bool CShopCache::PutShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName, TShopPosition pos
#ifdef ENABLE_SHOP_DECORATION
	, DWORD dwShopDecoration
#endif
	)
#else
	bool CShopCache::PutShop(DWORD dwOwnerID, DWORD dwDuration, const char* szName)
#endif
	{
		CACHEITER it = m_shopsMap.find(dwOwnerID);
		if (it != m_shopsMap.end())
			return false;

		TShopCacheInfo sShop;
		sShop.dwDuration = dwDuration;
		strncpy(sShop.szName, szName, sizeof(sShop.szName));
#ifdef ENABLE_IRA_REWORK
		CopyContainer(sShop.posDetails, pos);
#endif
#ifdef ENABLE_SHOP_DECORATION
		sShop.dwShopDecoration = dwShopDecoration;
#endif
		m_shopsMap.insert(std::make_pair(dwOwnerID, sShop));
		return true;
	}

	void CShopCache::EncodeCache(CPeer* peer) const
	{
		TShopInfo shopInfo;
#ifdef ENABLE_IRA_REWORK
		TShopPosition shopPosInfo;
#endif
		itertype(m_shopsMap) it = m_shopsMap.begin();

		while (it != m_shopsMap.end())
		{
			DWORD dwOwnerID = it->first;
			const TShopCacheInfo& rShop = it->second;

			strncpy(shopInfo.szName, rShop.szName, sizeof(shopInfo.szName));
			shopInfo.dwDuration = rShop.dwDuration;
			shopInfo.dwOwnerID = dwOwnerID;
			shopInfo.dwCount = rShop.itemsmap.size();
#ifdef ENABLE_SHOP_DECORATION
			shopInfo.dwShopDecoration = rShop.dwShopDecoration;
#endif
#ifdef ENABLE_IRA_REWORK
			shopPosInfo.x = rShop.posDetails.x;
			shopPosInfo.y = rShop.posDetails.y;
			shopPosInfo.lMapIndex = rShop.posDetails.lMapIndex;
			shopPosInfo.bChannel = rShop.posDetails.bChannel;
#endif
			peer->Encode(&shopInfo, sizeof(shopInfo));
#ifdef ENABLE_IRA_REWORK
			peer->Encode(&shopPosInfo, sizeof(shopPosInfo));
#endif
			itertype(rShop.itemsmap) itItem = rShop.itemsmap.begin();
			TItemInfo itemInfo;

			for (; itItem != rShop.itemsmap.end(); itItem++)
			{
				DWORD dwItemID = itItem->first;
				const TShopCacheItemInfo& rItem = itItem->second;

				itemInfo.dwOwnerID = dwOwnerID;
				itemInfo.dwItemID = dwItemID;

				CopyObject(itemInfo.item, rItem.item);
				CopyObject(itemInfo.price, rItem.price);
				peer->Encode(&itemInfo, sizeof(itemInfo));
			}
			it++;
		}
	}

	DWORD CShopCache::GetItemCount() const
	{
		DWORD dwItemCount = 0;
		CONST_CACHEITER it = m_shopsMap.begin();
		for (; it != m_shopsMap.end(); it++)
		{
			dwItemCount += it->second.itemsmap.size();
		}

		return dwItemCount;
	}

	void CShopCache::ShopDurationProcess()
	{
		CACHEITER it = m_shopsMap.begin();
		for (; it != m_shopsMap.end(); it++)
			if (--it->second.dwDuration != 0 && it->second.dwDuration % 5 == 0)
				UpdateDurationQuery(it->first, it->second);

		//expired check
		std::vector<DWORD> vec;

		//item expired check
		std::vector<std::pair<DWORD, DWORD>> item_vec;
		const time_t now = time(0);

		it = m_shopsMap.begin();
		for (; it != m_shopsMap.end(); it++)
		{
			CShopCache::TShopCacheInfo& shop = it->second;

			if (shop.dwDuration == 0) {
				vec.push_back(it->first);
				continue;
			}

			itertype(shop.itemsmap) it_item = shop.itemsmap.begin();
			itertype(shop.itemsmap) end_item = shop.itemsmap.end();

			for (; it_item != end_item; it_item++)
			{
				TItemInfoEx& item_info = it_item->second.item;
				if (item_info.expiration == offlineshop::ExpirationType::EXPIRE_REAL_TIME)
				{
					if (now > item_info.alSockets[0])
						item_vec.push_back(std::make_pair(it->first, it_item->first));
				}
				else if (item_info.expiration == offlineshop::ExpirationType::EXPIRE_REAL_TIME_FIRST_USE)
				{
					if (item_info.alSockets[1] != 0 && item_info.alSockets[0] < now)
						item_vec.push_back(std::make_pair(it->first, it_item->first));
				}
			}
		}

		for (DWORD i = 0; i < vec.size(); i++)
			CClientManager::instance().OfflineshopExpiredShop(vec[i]);

		itertype(item_vec) item_it = item_vec.begin();
		itertype(item_vec) item_end = item_vec.end();
		for (; item_it != item_end; item_it++) {
			CClientManager::Instance().SendOfflineShopRemoveItemPacket(item_it->first, item_it->second);
			RemoveItem(item_it->first, item_it->second);
		}
	}

	void CShopCache::UpdateDurationQuery(DWORD dwOwnerID, const TShopCacheInfo& rShop)
	{
		std::string query = CreateShopCacheUpdateDurationQuery(dwOwnerID, rShop.dwDuration);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SHOP_UPDATE_DURATION, 0, NULL);
	}

	CSafeboxCache::CSafeboxCache()
	{
	}

	CSafeboxCache::~CSafeboxCache()
	{
	}

	bool CSafeboxCache::Get(DWORD dwOwnerID, TSafeboxCacheInfo** ppSafebox) const
	{
		CHACHECONSTITER it = m_safeboxMap.find(dwOwnerID);
		if (it == m_safeboxMap.end())
			return false;

		*ppSafebox = (TSafeboxCacheInfo*)&(it->second);
		return true;
	}

	bool CSafeboxCache::PutSafebox(DWORD dwOwnerID, const TSafeboxCacheInfo& rSafebox)
	{
		CHACHECONSTITER it = m_safeboxMap.find(dwOwnerID);
		if (it != m_safeboxMap.end())
			return false;

		m_safeboxMap.insert(std::make_pair(dwOwnerID, rSafebox));
		return true;
	}

	bool CSafeboxCache::PutItem(DWORD dwOwnerID, DWORD dwItem, const TItemInfoEx& item)
	{
		TSafeboxCacheInfo* pSafebox = nullptr;
		if (!Get(dwOwnerID, &pSafebox))
			return false;

		std::map<DWORD, TItemInfoEx>::iterator it = pSafebox->itemsmap.find(dwItem);
		if (it != pSafebox->itemsmap.end())
			return false;

		pSafebox->itemsmap.insert(std::make_pair(dwItem, item));
		return true;
	}

	bool CSafeboxCache::RemoveItem(DWORD dwOwnerID, DWORD dwItemID)
	{
		TSafeboxCacheInfo* pSafebox = nullptr;
		if (!Get(dwOwnerID, &pSafebox))
			return false;

		std::map<DWORD, TItemInfoEx>::iterator it = pSafebox->itemsmap.find(dwItemID);
		if (it == pSafebox->itemsmap.end())
			return false;

		pSafebox->itemsmap.erase(it);

		std::string query = CreateSafeboxCacheDeleteItemQuery(dwItemID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_DELETE_ITEM, 0, NULL);
		return true;
	}

	bool CSafeboxCache::AddItem(DWORD dwOwnerID, const TItemInfoEx& item)
	{
		SQueryInfoSafeboxAddItem* qi = new SQueryInfoSafeboxAddItem;
		qi->dwOwnerID = dwOwnerID;
		CopyObject(qi->item, item);

		std::string query = CreateSafeboxCacheInsertItemQuery(dwOwnerID, item);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_ADD_ITEM, 0, qi);
		return true;
	}

	bool CSafeboxCache::AddValutes(DWORD dwOwnerID, const unsigned long long& val)
	{
		TSafeboxCacheInfo* pSafebox = nullptr;
		if (!Get(dwOwnerID, &pSafebox))
		{
			//sys_err("CANNOT FIND SAFEBOX BY ID %u !! ADDING VALUTES BY QUERY", dwOwnerID);
			AddValutesAsQuery(dwOwnerID, val);
			return true;
		}

		pSafebox->valutes += val;

		std::string query = CreateSafeboxCacheUpdateValutes(dwOwnerID, pSafebox->valutes);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES, 0, NULL);
		return true;
	}

	void CSafeboxCache::AddValutesAsQuery(DWORD dwOwnerID, const unsigned long long& val)
	{
		std::string query = CreateSafeboxCacheUpdateValutesByAdding(dwOwnerID, val);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES_ADDING, 0, NULL);
	}

	bool CSafeboxCache::RemoveValutes(DWORD dwOwnerID, const unsigned long long& val)
	{
		TSafeboxCacheInfo* pSafebox = nullptr;
		if (!Get(dwOwnerID, &pSafebox))
			return false;

		pSafebox->valutes -= val;

		std::string query = CreateSafeboxCacheUpdateValutes(dwOwnerID, pSafebox->valutes);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES, 0, NULL);
		return true;
	}

	CSafeboxCache::TSafeboxCacheInfo* CSafeboxCache::CreateSafebox(DWORD dwOwnerID)
	{
		if (!PutSafebox(dwOwnerID, TSafeboxCacheInfo()))
			return nullptr;

		std::string query = CreateSafeboxCacheInsertSafeboxValutesQuery(dwOwnerID);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_OFFLINESHOP_SAFEBOX_INSERT_VALUTES, 0, NULL);

		CSafeboxCache::TSafeboxCacheInfo* pInfo = nullptr;
		Get(dwOwnerID, &pInfo);

		return pInfo;
	}

	CSafeboxCache::TSafeboxCacheInfo* CSafeboxCache::LoadSafebox(DWORD dwPID)
	{
		TSafeboxCacheInfo* pSafebox = nullptr;
		if (Get(dwPID, &pSafebox))
			return pSafebox;

		TSafeboxCacheInfo safebox;
		MYSQL_ROW row;
		{
			std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(CreateSafeboxCacheLoadValutesQuery(dwPID).c_str()));
			if (pMsg->Get()->uiAffectedRows == 0)
				return CreateSafebox(dwPID);

			if (pMsg->Get()->uiAffectedRows != 1)
			{
				sys_err("multiple safebox rows for id %d ", dwPID);
				return nullptr;
			}

			if ((row = mysql_fetch_row(pMsg->Get()->pSQLResult))) {
				str_to_number(safebox.valutes, row[0]);
			}

			else
			{
				sys_err("cannot fetch safebox row for id %d ", dwPID);
				return nullptr;
			}
		}

		{
			std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(CreateSafeboxCacheLoadItemsQuery(dwPID).c_str()));
			DWORD dwItemID = 0;
			TItemInfoEx item;

			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int col = 0;

				str_to_number(dwItemID, row[col++]);
				str_to_number(item.dwVnum, row[col++]);
				str_to_number(item.dwCount, row[col++]);

				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
					str_to_number(item.alSockets[i], row[col++]);

				for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
				{
					str_to_number(item.aAttr[i].bType, row[col++]);
					str_to_number(item.aAttr[i].sValue, row[col++]);
				}

#ifdef __ENABLE_CHANGELOOK_SYSTEM__
				str_to_number(item.dwTransmutation, row[col++]);
#endif

				BYTE exp = 0;
				str_to_number(exp, row[col++]);
				item.expiration = offlineshop::ExpirationType(exp);
#ifdef ENABLE_OFFLINE_SHOP_GRID
				offlineshop::ZeroObject(item.display_pos);
#endif
				safebox.itemsmap.insert(std::make_pair(dwItemID, item));
			}
		}

		CACHEMAP::iterator it = m_safeboxMap.insert(std::make_pair(dwPID, safebox)).first;
		return &it->second;
	}

	DWORD CSafeboxCache::GetItemCount() const
	{
		DWORD dwItemCount = 0;
		CACHEMAP::const_iterator it = m_safeboxMap.begin();
		for (; it != m_safeboxMap.end(); it++)
			dwItemCount += it->second.itemsmap.size();

		return dwItemCount;
	}

	void CSafeboxCache::ItemExpirationProcess() {
		std::vector<std::pair<DWORD, DWORD>> vec;

		const auto now = time(0);
		for (auto& iter : m_safeboxMap) {
			auto& info = iter.second;

			for (auto& item : info.itemsmap)
			{
				auto& item_info = item.second;
				if (item_info.expiration == offlineshop::ExpirationType::EXPIRE_REAL_TIME)
				{
					if (now > item_info.alSockets[0])
						vec.emplace_back(std::make_pair(iter.first, item.first));
				}
				else if (item_info.expiration == offlineshop::ExpirationType::EXPIRE_REAL_TIME_FIRST_USE)
				{
					if (item_info.alSockets[1] != 0 && item_info.alSockets[0] < now)
						vec.emplace_back(std::make_pair(iter.first, item.first));
				}
			}
		}

		for (const auto& data : vec) {
			RemoveItem(data.first, data.second);
			CClientManager::instance().SendOfflineshopSafeboxExpiredItem(data.first, data.second);
		}
	}

	std::string CreateShopCacheInsertItemQuery(DWORD dwOwner, const CShopCache::TShopCacheItemInfo& rItem)
	{
		char szQuery[1540] = "INSERT INTO `player`.`offlineshop_shop_items` (`item_id`, `owner_id`, `price_yang`, "
			" `vnum`, `count` ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",`socket%d` ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",`attr%d` , `attrval%d` ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, "%s", " "
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
			", `trans` "
#endif
			", expiration "
#ifdef ENABLE_OFFLINE_SHOP_GRID
			", display_pos "
			", display_page "
#endif
			") VALUES (");
		len += snprintf(szQuery + len, sizeof(szQuery) - len, "0, %u, %lld,"
			" %u, %u ",
			dwOwner, rItem.price.illYang,
			rItem.item.dwVnum, rItem.item.dwCount
		);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",%ld ", rItem.item.alSockets[i]);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				", %d , %d ", rItem.item.aAttr[i].bType, rItem.item.aAttr[i].sValue);

#ifdef __ENABLE_CHANGELOOK_SYSTEM__
		len += snprintf(szQuery + len, sizeof(szQuery) - len, " , %u ", rItem.item.dwTransmutation);
#endif

		len += snprintf(szQuery + len, sizeof(szQuery) - len, ", %u ", (BYTE)rItem.item.expiration);

#ifdef ENABLE_OFFLINE_SHOP_GRID
		len += snprintf(szQuery + len, sizeof(szQuery) - len,
			", %u , %u ", rItem.item.display_pos.pos, rItem.item.display_pos.page);
#endif
		std::string query = szQuery;
		query += ");";

		return query;
	}

	std::string CreateShopCacheUpdateItemQuery(DWORD dwItemID, const TPriceInfo& rItemPrice)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "UPDATE `player`.`offlineshop_shop_items` SET `price_yang` = %lld "
			" WHERE `item_id` = %u;",
			rItemPrice.illYang,
			dwItemID);

		return szQuery;
	}

	std::string CreateShopCacheDeleteShopQuery(DWORD dwOwner)
	{
		static char szQuery[128];
		snprintf(szQuery, sizeof(szQuery), "DELETE from `player`.`offlineshop_shops` WHERE `owner_id` = %d;", dwOwner);
		return szQuery;
	}

	std::string CreateShopCacheDeleteShopItemQuery(DWORD dwOwner)
	{
		static char szQuery[128];
		snprintf(szQuery, sizeof(szQuery), "DELETE from `player`.`offlineshop_shop_items` WHERE `owner_id` = %d;", dwOwner);
		return szQuery;
	}

#ifdef ENABLE_IRA_REWORK
	std::string CreateShopCacheInsertShopQuery(DWORD dwOwnerID, DWORD dwDuration, const char* name, TShopPosition pos
#ifdef ENABLE_SHOP_DECORATION
	, DWORD dwShopDecoration
#endif
	)
#else
	std::string CreateShopCacheInsertShopQuery(DWORD dwOwnerID, DWORD dwDuration, const char* name)
#endif
	{
		static char szQuery[256];
		char szEscapeString[OFFLINE_SHOP_NAME_MAX_LEN + 32];

		CDBManager::instance().EscapeString(szEscapeString, name, strlen(name));

#ifdef ENABLE_IRA_REWORK
#ifdef ENABLE_SHOP_DECORATION
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO `player`.`offlineshop_shops` (`owner_id`, `duration`, `name`, `pos_x`, `pos_y`, `map_index`, `channel`, `shop_decoration`) VALUES(%u, %u, '%s', %lu, %lu, %ld, %d, %u);",
#else
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO `player`.`offlineshop_shops` (`owner_id`, `duration`, `name`, `pos_x`, `pos_y`, `map_index`, `channel`) VALUES(%u, %u, '%s', %lu, %lu, %ld, %d);",
#endif
			dwOwnerID, dwDuration, szEscapeString, pos.x, pos.y, pos.lMapIndex, pos.bChannel
#ifdef ENABLE_SHOP_DECORATION
			, dwShopDecoration
#endif
#else
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO `player`.`offlineshop_shops` (`owner_id`, `duration`, `name`) VALUES(%u, %u, '%s');",
			dwOwnerID, dwDuration, szEscapeString
#endif
		);

		return szQuery;
	}

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	std::string CreateShopCacheUpdateShopTimeQuery(DWORD dwOwnerID, DWORD dwTime)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "UPDATE `player`.`offlineshop_shops` SET `duration` = `duration`+'%d' WHERE `owner_id` = %u;", dwTime, dwOwnerID);
		return szQuery;
	}
#endif

	std::string CreateShopCacheUpdateShopNameQuery(DWORD dwOwnerID, const char* name)
	{
		static char szQuery[256];
		static char szEscapeString[OFFLINE_SHOP_NAME_MAX_LEN + 32];
		CDBManager::instance().EscapeString(szEscapeString, name, strlen(name));

		snprintf(szQuery, sizeof(szQuery), "UPDATE `player`.`offlineshop_shops` SET `name` = '%s' WHERE `owner_id` = %u;", szEscapeString, dwOwnerID);
		return szQuery;
	}

	std::string CreateShopCacheUpdateDurationQuery(DWORD dwOwnerID, DWORD dwDuration)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "UPDATE `player`.`offlineshop_shops` SET `duration` = '%d' WHERE `owner_id` = %u;", dwDuration, dwOwnerID);
		return szQuery;
	}

	std::string CreateSafeboxCacheDeleteItemQuery(DWORD dwItem)
	{
		static char szQuery[128];
		snprintf(szQuery, sizeof(szQuery), "DELETE from `player`.`offlineshop_safebox_items` WHERE `item_id` = %d;", dwItem);
		return szQuery;
	}

	std::string CreateSafeboxCacheInsertItemQuery(DWORD dwOwner, const TItemInfoEx& item)
	{
		char szQuery[1024] = "INSERT INTO `player`.`offlineshop_safebox_items` (`item_id`, `owner_id`, `vnum`, `count` ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",`socket%d` ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",`attr%d` , `attrval%d` ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, "%s", "  "
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
			" , `trans` "
#endif
			", expiration "
			") VALUES (");
		len += snprintf(szQuery + len, sizeof(szQuery) - len, "0, %u, %u, %u ",
			dwOwner, item.dwVnum, item.dwCount
		);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",%ld ", item.alSockets[i]);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				", %d , %d ", item.aAttr[i].bType, item.aAttr[i].sValue);

#ifdef __ENABLE_CHANGELOOK_SYSTEM__
		len += snprintf(szQuery + len, sizeof(szQuery) - len,
			", %u ", item.dwTransmutation);
#endif

		len += snprintf(szQuery + len, sizeof(szQuery) - len, ", %u ", (BYTE)item.expiration);
		std::string query = szQuery;
		query += ");";

		return query;
	}

	std::string CreateSafeboxCacheUpdateValutes(DWORD dwOwner, const unsigned long long& val)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "UPDATE `player`.`offlineshop_safebox_valutes` SET `yang` = %llu "
			" WHERE `owner_id` = %u ;",
			val,
			dwOwner);

		return szQuery;
	}

	std::string CreateSafeboxCacheUpdateValutesByAdding(DWORD dwOwner, const unsigned long long& val)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "UPDATE `player`.`offlineshop_safebox_valutes` SET `yang` = `yang`+%llu "
			" WHERE `owner_id` = %u ;", val,
			dwOwner);

		return szQuery;
	}

	std::string CreateSafeboxCacheInsertSafeboxValutesQuery(DWORD dwOwnerID)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO `player`.`offlineshop_safebox_valutes` (`owner_id`,`yang`) VALUES( %u , 0);", dwOwnerID);
		return szQuery;
	}

	std::string CreateSafeboxCacheLoadValutesQuery(DWORD dwOwnerID)
	{
		static char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT `yang` "
			" from `player`.`offlineshop_safebox_valutes` WHERE `owner_id` = %d;", dwOwnerID);
		return szQuery;
	}

	std::string CreateSafeboxCacheLoadItemsQuery(DWORD dwOwnerID)
	{
		char szQuery[1024] = "SELECT `item_id`, `vnum`, `count` ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",`socket%d` ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",`attr%d` , `attrval%d` ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, "%s%u;", " "

#ifdef __ENABLE_CHANGELOOK_SYSTEM__
			" , `trans` "
#endif
			", expiration "
			" from `player`.`offlineshop_safebox_items` WHERE `owner_id`=", dwOwnerID);

		return szQuery;
	}

	std::string CreateShopCacheDeleteItemQuery(DWORD dwOwnerID, DWORD dwItemID)
	{
		static char szQuery[128];
		snprintf(szQuery, sizeof(szQuery), "DELETE from `player`.`offlineshop_shop_items`  WHERE `owner_id` = %u AND item_id = '%u' ;", dwOwnerID, dwItemID);
		return szQuery;
	}
}
#endif