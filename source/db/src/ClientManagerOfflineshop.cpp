#include "stdafx.h"
#include "../../common/building.h"
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "OfflineshopCache.h"
#include "ClientManager.h"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#define DIRECT_QUERY(query) CDBManager::instance().DirectQuery((query))

std::string CreateOfflineshopSelectShopItemsQuery();
template <class T>
const char* Decode(T*& pObj, const char* data)
{
	pObj = (T*)data;
	return data + sizeof(T);
}

bool CClientManager::InitializeOfflineshopTable()
{
	MYSQL_ROW row;
	{
#ifdef ENABLE_IRA_REWORK
#ifdef ENABLE_SHOP_DECORATION
		const char szQuery[] = "SELECT `owner_id` , `name` , `duration`, `pos_x`, `pos_y`, `map_index`, `channel`, `shop_decoration` FROM `player`.`offlineshop_shops`;";
#else
		const char szQuery[] = "SELECT `owner_id` , `name` , `duration`, `pos_x`, `pos_y`, `map_index`, `channel` FROM `player`.`offlineshop_shops`;";
#endif
#else
		const char szQuery[] = "SELECT `owner_id` , `name` , `duration` FROM `player`.`offlineshop_shops`;";
#endif
		std::unique_ptr<SQLMsg> pMsg(DIRECT_QUERY(szQuery));

		if (pMsg->uiSQLErrno != 0)
		{
			sys_err("CANNOT LOAD offlineshop_shops TABLE , errorcode %d ", pMsg->uiSQLErrno);
			return false;
		}

		if (pMsg->Get())
		{
#ifdef ENABLE_IRA_REWORK
			offlineshop::TShopPosition pos;
#endif
			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int col = 0;
				std::string name = "";
				DWORD dwOwner = 0, dwDuration = 0;

				str_to_number(dwOwner, row[col++]);
				name = row[col++];
				str_to_number(dwDuration, row[col++]);
#ifdef ENABLE_IRA_REWORK
				str_to_number(pos.x, row[col++]);
				str_to_number(pos.y, row[col++]);
				str_to_number(pos.lMapIndex, row[col++]);
				str_to_number(pos.bChannel, row[col++]);
#endif
#ifdef ENABLE_SHOP_DECORATION
				DWORD dwShopDecoration = 0;
				str_to_number(dwShopDecoration, row[col++]);
#endif
#ifdef ENABLE_IRA_REWORK
				if (!m_offlineshopShopCache.PutShop(dwOwner, dwDuration, name.c_str(), pos
#ifdef ENABLE_SHOP_DECORATION
				, dwShopDecoration
#endif
				))
#else
				if (!m_offlineshopShopCache.PutShop(dwOwner, dwDuration, name.c_str()))
#endif
					sys_err("cannot execute putsShop -> double shop id?!");
			}
		}
	}

	{
		std::string query = CreateOfflineshopSelectShopItemsQuery();
		std::unique_ptr<SQLMsg> pMsg(DIRECT_QUERY(query.c_str()));

		if (pMsg->uiSQLErrno != 0)
		{
			sys_err("CANNOT LOAD offlineshop_shop_items TABLE , errorcode %d ", pMsg->uiSQLErrno);
			return false;
		}

		if (pMsg->Get())
		{
			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int col = 0;
				DWORD dwItemID = 0, dwOwnerID = 0;

				offlineshop::CShopCache::TShopCacheItemInfo item;
				offlineshop::ZeroObject(item);

				str_to_number(dwItemID, row[col++]);
				str_to_number(dwOwnerID, row[col++]);
				str_to_number(item.price.illYang, row[col++]);

				str_to_number(item.item.dwVnum, row[col++]);
				str_to_number(item.item.dwCount, row[col++]);

				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
					str_to_number(item.item.alSockets[i], row[col++]);

				for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
				{
					str_to_number(item.item.aAttr[i].bType, row[col++]);
					str_to_number(item.item.aAttr[i].sValue, row[col++]);
				}

				BYTE bExpiration = 0;
				str_to_number(bExpiration, row[col++]);
				item.item.expiration = offlineshop::ExpirationType(bExpiration);
#ifdef ENABLE_OFFLINE_SHOP_GRID
				str_to_number(item.item.display_pos.pos, row[col++]);
				str_to_number(item.item.display_pos.page, row[col++]);
#endif
				if (!m_offlineshopShopCache.PutItem(dwOwnerID, dwItemID, item))
					sys_err("cannot execute putitem !? owner %d , item %d , didn't deleted items when closed?");
			}
		}
	}
	return true;
}

void CClientManager::SendOfflineshopTable(CPeer* peer)
{
	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0,
		sizeof(TPacketDGNewOfflineShop) + sizeof(offlineshop::TSubPacketDGLoadTables) +
#ifdef ENABLE_IRA_REWORK
		(sizeof(offlineshop::TShopInfo) + sizeof(offlineshop::TShopPosition)) * m_offlineshopShopCache.GetCount() + sizeof(offlineshop::TItemInfo) * m_offlineshopShopCache.GetItemCount()
#else
		(sizeof(offlineshop::TShopInfo)) * m_offlineshopShopCache.GetCount() + sizeof(offlineshop::TItemInfo) * m_offlineshopShopCache.GetItemCount()
#endif
	);

	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_LOAD_TABLES;

	offlineshop::TSubPacketDGLoadTables subPack;
	subPack.dwShopCount = m_offlineshopShopCache.GetCount();

	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subPack, sizeof(subPack));

	m_offlineshopShopCache.EncodeCache(peer);
}

void CClientManager::RecvOfflineShopPacket(CPeer* peer, const char* data)
{
	TPacketGDNewOfflineShop* pack;
	data = Decode(pack, data);

	bool bRet = false;

	switch (pack->bSubHeader)
	{
	case offlineshop::SUBHEADER_GD_BUY_ITEM:			bRet = RecvOfflineShopBuyItemPacket(data);			break;
	case offlineshop::SUBHEADER_GD_BUY_LOCK_ITEM:		bRet = RecvOfflineShopLockBuyItem(peer, data);			break;
	case offlineshop::SUBHEADER_GD_EDIT_ITEM:			bRet = RecvOfflineShopEditItemPacket(data);			break;
	case offlineshop::SUBHEADER_GD_REMOVE_ITEM:			bRet = RecvOfflineShopRemoveItemPacket(data);			break;
	case offlineshop::SUBHEADER_GD_ADD_ITEM:			bRet = RecvOfflineShopAddItemPacket(data);			break;

	case offlineshop::SUBHEADER_GD_SHOP_FORCE_CLOSE:	bRet = RecvOfflineShopForceClose(data);				break;
	case offlineshop::SUBHEADER_GD_SHOP_CREATE_NEW:		bRet = RecvOfflineShopCreateNew(data);				break;
	case offlineshop::SUBHEADER_GD_SHOP_CHANGE_NAME:	bRet = RecvOfflineShopChangeName(data);				break;

	case offlineshop::SUBHEADER_GD_SAFEBOX_GET_ITEM:	bRet = RecvOfflineShopSafeboxGetItem(data);			break;
	case offlineshop::SUBHEADER_GD_SAFEBOX_GET_VALUTES:	bRet = RecvOfflineShopSafeboxGetValutes(data);		break;
	case offlineshop::SUBHEADER_GD_SAFEBOX_ADD_ITEM:	bRet = RecvOfflineShopSafeboxAddItem(data);			break;
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	case offlineshop::SUBHEADER_GD_SHOP_EXTEND_TIME:	bRet=RecvOfflineShopExtendTime(data);				break;
#endif
	default:
		sys_err("UNKNOW NEW OFFLINESHOP SUBHEADER GD %d", pack->bSubHeader);
		break;
	}

	if (!bRet)
		sys_err("maybe some error during recv offline shop subheader %d ", pack->bSubHeader);
}

bool CClientManager::RecvOfflineShopLockBuyItem(CPeer* peer, const char* data)
{
	offlineshop::TSubPacketGDLockBuyItem* subpack;
	data = Decode(subpack, data);

	DWORD dwGuest = subpack->dwGuestID;
	DWORD dwOwner = subpack->dwOwnerID;
	DWORD dwItem = subpack->dwItemID;

	if (m_offlineshopShopCache.LockSellItem(dwOwner, dwItem))
		SendOfflineShopBuyLockedItemPacket(peer, dwOwner, dwGuest, dwItem);
	else
		sys_err("cannot find buy target item %u (owner %u , buyer %u) ", dwItem, dwOwner, dwGuest);

	return true;
}

bool CClientManager::RecvOfflineShopCannotBuyLockItem(const char* data)
{
	offlineshop::TSubPacketGDCannotBuyLockItem* subpack;
	data = Decode(subpack, data);

	DWORD dwOwner = subpack->dwOwnerID;
	DWORD dwItem = subpack->dwItemID;

	if (!m_offlineshopShopCache.UnlockSellItem(dwOwner, dwItem)) {
		sys_err("cannot find unlock requested item %u (owner %u ) ", dwItem, dwOwner);
		return false;
	}return true;
}

bool CClientManager::RecvOfflineShopBuyItemPacket(const char* data)
{
	offlineshop::TSubPacketGDBuyItem* subpack;
	data = Decode(subpack, data);

	DWORD dwGuest = subpack->dwGuestID;
	DWORD dwOwner = subpack->dwOwnerID;
	DWORD dwItem = subpack->dwItemID;

	offlineshop::CShopCache::TShopCacheInfo* pCache = nullptr;
	if (!m_offlineshopShopCache.Get(dwOwner, &pCache))
		return false;

	itertype(pCache->itemsmap) it = pCache->itemsmap.find(dwItem);
	if (pCache->itemsmap.end() == it)
		return false;

	const offlineshop::TPriceInfo& price = it->second.price;
	unsigned long long valute;
	valute = price.illYang;

	m_offlineshopSafeboxCache.AddValutes(dwOwner, valute);
	SendOfflineshopSafeboxAddValutes(dwOwner, valute);

	SendOfflineShopBuyItemPacket(dwOwner, dwGuest, dwItem);
	if (!m_offlineshopShopCache.SellItem(dwOwner, dwItem))
		sys_err("some problem with sell : %u %u ", dwOwner, dwItem);

	return true;
}

bool CClientManager::RecvOfflineShopEditItemPacket(const char* data)
{
	offlineshop::TSubPacketGDEditItem* subpack;
	data = Decode(subpack, data);

	if (subpack->allEdit)
	{
		offlineshop::CShopCache::TShopCacheInfo* cache;
		m_offlineshopShopCache.Get(subpack->dwOwnerID, &cache);

		std::map<DWORD, offlineshop::CShopCache::TShopCacheItemInfo>::iterator it = cache->itemsmap.find(subpack->dwItemID);
		if (it == cache->itemsmap.end())
			return false;

		if (it->second.bLock)
			return false;

		offlineshop::TPriceInfo price;

		price.illYang = subpack->priceInfo.illYang / it->second.item.dwCount;

		if (price.illYang < 1)
			return false;

		for (const auto& it2 : cache->itemsmap)
		{
			if (it2.second.item.dwVnum == it->second.item.dwVnum)
			{
				if (it2.second.bLock)
					continue;

				offlineshop::TPriceInfo tPrice;
				tPrice.illYang = it2.second.item.dwCount * price.illYang;

				if (m_offlineshopShopCache.EditItem(subpack->dwOwnerID, it2.first,tPrice))
					SendOfflineShopEditItemPacket(subpack->dwOwnerID, it2.first, tPrice);
				else
					sys_err("cannot edit item %u , shop %u ", it2.first, subpack->dwOwnerID);
			}
		}
	}
	else
	{
		if (m_offlineshopShopCache.EditItem(subpack->dwOwnerID, subpack->dwItemID, subpack->priceInfo))
			SendOfflineShopEditItemPacket(subpack->dwOwnerID, subpack->dwItemID, subpack->priceInfo);
		else
			sys_err("cannot edit item %u , shop %u ", subpack->dwItemID, subpack->dwOwnerID);
	}

	return true;
}

bool CClientManager::RecvOfflineShopRemoveItemPacket(const char* data)
{
	offlineshop::TSubPacketGDRemoveItem* subpack;
	data = Decode(subpack, data);

	offlineshop::CShopCache::TShopCacheInfo* pShop = nullptr;
	if (!m_offlineshopShopCache.Get(subpack->dwOwnerID, &pShop))
	{
		return false;
	}

	offlineshop::CShopCache::SHOPCACHE_MAP::iterator it = pShop->itemsmap.find(subpack->dwItemID);
	if (it == pShop->itemsmap.end())
	{
		return false;
	}

	auto copy = it->second.item;
	if (m_offlineshopShopCache.RemoveItem(subpack->dwOwnerID, subpack->dwItemID)) {
		SendOfflineShopRemoveItemPacket(subpack->dwOwnerID, subpack->dwItemID);
		m_offlineshopSafeboxCache.AddItem(subpack->dwOwnerID, copy);
	}

	else
		sys_err("cannot remove item %u shop %u item ?!", subpack->dwOwnerID, subpack->dwItemID);

	return true;
}

bool CClientManager::RecvOfflineShopAddItemPacket(const char* data)
{
	offlineshop::TSubPacketGDAddItem* subpack;
	data = Decode(subpack, data);

	offlineshop::CShopCache::TShopCacheItemInfo item;
	offlineshop::ZeroObject(item);
	offlineshop::CopyObject(item.item, subpack->itemInfo.item);
	offlineshop::CopyObject(item.price, subpack->itemInfo.price);

	m_offlineshopShopCache.AddItem(subpack->dwOwnerID, item);
	return true;
}

bool CClientManager::RecvOfflineShopForceClose(const char* data)
{
	offlineshop::TSubPacketGDShopForceClose* subpack;
	data = Decode(subpack, data);

	offlineshop::CShopCache::TShopCacheInfo* pInfo = nullptr;
	if (!m_offlineshopShopCache.Get(subpack->dwOwnerID, &pInfo))
		return false;

	offlineshop::CShopCache::ITEM_ITER it = pInfo->itemsmap.begin();
	for (; it != pInfo->itemsmap.end(); it++)
		m_offlineshopSafeboxCache.AddItem(subpack->dwOwnerID, it->second.item);

	m_offlineshopShopCache.CloseShop(subpack->dwOwnerID);

	SendOfflineShopForceClose(subpack->dwOwnerID);
	return true;
}

bool CClientManager::RecvOfflineShopCreateNew(const char* data)
{
	offlineshop::TSubPacketGDShopCreateNew* subpack;
	data = Decode(subpack, data);

	std::vector<offlineshop::CShopCache::TShopCacheItemInfo> vec;
	offlineshop::CShopCache::TShopCacheItemInfo itemInfo;
	offlineshop::ZeroObject(itemInfo);

	vec.reserve(subpack->shop.dwCount);

	for (DWORD i = 0; i < subpack->shop.dwCount; i++)
	{
		offlineshop::TItemInfo* pItemInfo;
		data = Decode(pItemInfo, data);

		offlineshop::CopyObject(itemInfo.item, pItemInfo->item);
		offlineshop::CopyObject(itemInfo.price, pItemInfo->price);

		vec.push_back(itemInfo);
	}

#ifdef ENABLE_IRA_REWORK
	m_offlineshopShopCache.CreateShop(subpack->shop.dwOwnerID, subpack->shop.dwDuration, subpack->shop.szName, vec, subpack->pos
#ifdef ENABLE_SHOP_DECORATION
	, subpack->shop.dwShopDecoration
#endif
	);
#else
	m_offlineshopShopCache.CreateShop(subpack->shop.dwOwnerID, subpack->shop.dwDuration, subpack->shop.szName, vec);
#endif
	return true;
}

bool CClientManager::RecvOfflineShopChangeName(const char* data)
{
	offlineshop::TSubPacketGDShopChangeName* subpack;
	data = Decode(subpack, data);

	if (m_offlineshopShopCache.ChangeShopName(subpack->dwOwnerID, subpack->szName))
		SendOfflineShopChangeName(subpack->dwOwnerID, subpack->szName);
	return true;
}

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
bool CClientManager::RecvOfflineShopExtendTime(const char* data)
{
	offlineshop::TSubPacketGDShopExtendTime* subpack;
	data = Decode(subpack, data);

	if(m_offlineshopShopCache.ExtendShopTime(subpack->dwOwnerID, subpack->dwTime))
		SendOfflineShopExtendTime(subpack->dwOwnerID, subpack->dwTime);
	return true;
}

bool CClientManager::SendOfflineShopExtendTime(DWORD dwOwnerID, DWORD dwTime)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader	= offlineshop::SUBHEADER_DG_SHOP_EXTEND_TIME;

	offlineshop::TSubPacketDGShopExtendTime subpack;
	subpack.dwOwnerID	= dwOwnerID;
	subpack.dwTime		= dwTime;


	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) );
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}
#endif

bool CClientManager::RecvOfflineShopSafeboxGetItem(const char* data)
{
	offlineshop::TSubPacketGDSafeboxGetItem* subpack;
	data = Decode(subpack, data);

	if (m_offlineshopSafeboxCache.RemoveItem(subpack->dwOwnerID, subpack->dwItemID))
	{
		return true;
	}
		
	return false;
}

bool CClientManager::RecvOfflineShopSafeboxGetValutes(const char* data)
{
	offlineshop::TSubPacketGDSafeboxGetValutes* subpack;
	data = Decode(subpack, data);

	if (m_offlineshopSafeboxCache.RemoveValutes(subpack->dwOwnerID, subpack->valute))
		return true;

	return false;
}

bool CClientManager::RecvOfflineShopSafeboxAddItem(const char* data)
{
	offlineshop::TSubPacketGDSafeboxAddItem* subpack;
	data = Decode(subpack, data);

	if (m_offlineshopSafeboxCache.AddItem(subpack->dwOwnerID, subpack->item))
		return false;
	return true;
}

bool CClientManager::SendOfflineShopBuyLockedItemPacket(CPeer* peer, DWORD dwOwner, DWORD dwGuest, DWORD dwItem)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_LOCKED_BUY_ITEM;

	offlineshop::TSubPacketDGLockedBuyItem subpack;
	subpack.dwOwnerID = dwOwner;
	subpack.dwBuyerID = dwGuest;
	subpack.dwItemID = dwItem;

	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));

	return true;
}

bool CClientManager::SendOfflineShopBuyItemPacket(DWORD dwOwner, DWORD dwGuest, DWORD dwItem)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_BUY_ITEM;

	offlineshop::TSubPacketDGBuyItem subpack;
	subpack.dwOwnerID = dwOwner;
	subpack.dwBuyerID = dwGuest;
	subpack.dwItemID = dwItem;

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

bool CClientManager::SendOfflineShopEditItemPacket(DWORD dwOwner, DWORD dwItem, const offlineshop::TPriceInfo& price)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_EDIT_ITEM;

	offlineshop::TSubPacketDGEditItem subpack;
	subpack.dwOwnerID = dwOwner;
	subpack.dwItemID = dwItem;
	offlineshop::CopyObject(subpack.price, price);

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

bool CClientManager::SendOfflineShopRemoveItemPacket(DWORD dwOwner, DWORD dwItem)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_REMOVE_ITEM;

	offlineshop::TSubPacketDGRemoveItem subpack;
	subpack.dwOwnerID = dwOwner;
	subpack.dwItemID = dwItem;

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

bool CClientManager::SendOfflineShopAddItemPacket(DWORD dwOwner, DWORD dwItemID, const offlineshop::TItemInfo& rInfo)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_ADD_ITEM;

	offlineshop::TSubPacketDGAddItem subpack;
	subpack.dwOwnerID = dwOwner;
	subpack.dwItemID = dwItemID;
	offlineshop::CopyObject(subpack.item, rInfo);

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

bool CClientManager::SendOfflineShopForceClose(DWORD dwOwnerID)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SHOP_FORCE_CLOSE;

	offlineshop::TSubPacketDGShopForceClose subpack;
	subpack.dwOwnerID = dwOwnerID;

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

#ifdef ENABLE_IRA_REWORK
bool CClientManager::SendOfflineShopCreateNew(const offlineshop::TShopInfo& shop, const std::vector<offlineshop::TItemInfo>& vec, offlineshop::TShopPosition& pos)
#else
bool CClientManager::SendOfflineShopCreateNew(const offlineshop::TShopInfo& shop, const std::vector<offlineshop::TItemInfo>& vec)
#endif
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SHOP_CREATE_NEW;

	offlineshop::TSubPacketDGShopCreateNew subpack;
	offlineshop::CopyObject(subpack.shop, shop);
#ifdef ENABLE_IRA_REWORK
	offlineshop::CopyObject(subpack.pos, pos);
#endif

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) + (sizeof(offlineshop::TItemInfo) * vec.size()));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));

		if (!vec.empty())
			peer->Encode(&vec[0], sizeof(offlineshop::TItemInfo) * vec.size());
	}
	return true;
}

bool CClientManager::SendOfflineShopChangeName(DWORD dwOwnerID, const char* szName)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SHOP_CHANGE_NAME;

	offlineshop::TSubPacketDGShopChangeName subpack;
	subpack.dwOwnerID = dwOwnerID;
	strncpy(subpack.szName, szName, sizeof(subpack.szName));
	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

bool CClientManager::SendOfflineshopShopExpired(DWORD dwOwnerID)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SHOP_EXPIRED;

	offlineshop::TSubPacketDGShopExpired subpack;
	subpack.dwOwnerID = dwOwnerID;

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}

	return true;
}

void CClientManager::SendOfflineshopSafeboxAddItem(DWORD dwOwnerID, DWORD dwItem, const offlineshop::TItemInfoEx& item)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SAFEBOX_ADD_ITEM;

	offlineshop::TSubPacketDGSafeboxAddItem subpack;
	subpack.dwOwnerID = dwOwnerID;
	subpack.dwItemID = dwItem;

	offlineshop::CopyObject(subpack.item, item);
	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}
}

void CClientManager::SendOfflineshopSafeboxAddValutes(DWORD dwOwnerID, const unsigned long long& valute)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SAFEBOX_ADD_VALUTES;

	offlineshop::TSubPacketDGSafeboxAddValutes subpack;
	subpack.dwOwnerID = dwOwnerID;
	offlineshop::CopyObject(subpack.valute, valute);

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;
		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}
}

void CClientManager::SendOfflineshopSafeboxLoad(CPeer* peer, DWORD dwOwnerID, const unsigned long long& valute, const std::vector<offlineshop::TItemInfoEx>& items, const std::vector<DWORD>& ids)
{
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SAFEBOX_LOAD;

	offlineshop::TSubPacketDGSafeboxLoad subpack;
	subpack.dwOwnerID = dwOwnerID;
	subpack.dwItemCount = items.size();
	offlineshop::CopyObject(subpack.valute, valute);
	peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack) + (sizeof(offlineshop::TItemInfoEx) * items.size()) + (sizeof(DWORD) * ids.size()));
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));

	for (DWORD i = 0; i < items.size(); i++)
	{
		peer->EncodeDWORD(ids[i]);
		peer->Encode(&items[i], sizeof(offlineshop::TItemInfoEx));
	}
}

void CClientManager::SendOfflineshopSafeboxExpiredItem(DWORD dwOwnerID, DWORD itemID) {
	TPacketDGNewOfflineShop pack;
	pack.bSubHeader = offlineshop::SUBHEADER_DG_SAFEBOX_EXPIRED_ITEM;

	offlineshop::TSubPacketDGSafeboxExpiredItem subpack;
	subpack.dwItemID = dwOwnerID;
	subpack.dwItemID = itemID;

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
	{
		CPeer* peer = *it;

		peer->EncodeHeader(HEADER_DG_NEW_OFFLINESHOP, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
	}
}

void CClientManager::OfflineShopResultAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	offlineshop::SQueryInfoAddItem* qi = (offlineshop::SQueryInfoAddItem*)pQueryInfo->pvData;

	DWORD dwItemID = msg->Get()->uiInsertID;

	offlineshop::TItemInfo info;
	offlineshop::CopyObject(info.item, qi->item.item);
	offlineshop::CopyObject(info.price, qi->item.price);

	info.dwItemID = dwItemID;
	info.dwOwnerID = qi->dwOwnerID;

	m_offlineshopShopCache.PutItem(qi->dwOwnerID, dwItemID, qi->item);
	SendOfflineShopAddItemPacket(qi->dwOwnerID, dwItemID, info);
	delete(qi);
}

void CClientManager::OfflineShopResultCreateShopQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	offlineshop::SQueryInfoCreateShop* qi = (offlineshop::SQueryInfoCreateShop*)pQueryInfo->pvData;
#ifdef ENABLE_IRA_REWORK
	if (!m_offlineshopShopCache.PutShop(qi->dwOwnerID, qi->dwDuration, qi->szName, qi->pos
#ifdef ENABLE_SHOP_DECORATION
	, qi->dwShopDecoration
#endif
	))
#else
	if (!m_offlineshopShopCache.PutShop(qi->dwOwnerID, qi->dwDuration, qi->szName))
#endif
		sys_err("cannot insert new shop , id %d ", qi->dwOwnerID);

	if (!qi->items.empty())
	{
		qi->dwItemIndex = 0;
		offlineshop::CShopCache::TShopCacheItemInfo& itemInfo = qi->items[qi->dwItemIndex];
		if (!m_offlineshopShopCache.CreateShopAddItem(qi, itemInfo))
			sys_err("some problem during shop puts? cannot find the shop in the cache");
	}
	else
	{
		offlineshop::TShopInfo shop;
		shop.dwOwnerID = qi->dwOwnerID;
		shop.dwCount = 0;
		strncpy(shop.szName, qi->szName, sizeof(shop.szName));

#ifdef ENABLE_IRA_REWORK
		offlineshop::TShopPosition pos;
		offlineshop::CopyObject(pos, qi->pos);
#endif

		std::vector<offlineshop::TItemInfo> vec;
#ifdef ENABLE_IRA_REWORK
		SendOfflineShopCreateNew(shop, vec, pos);
#else
		SendOfflineShopCreateNew(shop, vec);
#endif
	}
}

void CClientManager::OfflineShopResultCreateShopAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	offlineshop::SQueryInfoCreateShop* qi = (offlineshop::SQueryInfoCreateShop*)pQueryInfo->pvData;

	const offlineshop::CShopCache::TShopCacheItemInfo& item = qi->items[qi->dwItemIndex++];
	m_offlineshopShopCache.PutItem(qi->dwOwnerID, msg->Get()->uiInsertID, item);
	qi->ids.push_back(msg->Get()->uiInsertID);

	if (!qi->items.empty() && qi->dwItemIndex != qi->items.size())
	{
		offlineshop::CShopCache::TShopCacheItemInfo& itemInfo = qi->items[qi->dwItemIndex];
		m_offlineshopShopCache.CreateShopAddItem(qi, itemInfo);
	}
	else
	{
		offlineshop::TShopInfo shopInfo;
		shopInfo.dwOwnerID = qi->dwOwnerID;
		shopInfo.dwDuration = qi->dwDuration;
#ifdef ENABLE_SHOP_DECORATION
		shopInfo.dwShopDecoration = qi->dwShopDecoration;
#endif
		shopInfo.dwCount = qi->items.size();
		strncpy(shopInfo.szName, qi->szName, sizeof(shopInfo.szName));

		offlineshop::TItemInfo temp;
		std::vector<offlineshop::TItemInfo> vec;
		vec.reserve(qi->items.size());
		for (DWORD i = 0; i < qi->items.size(); i++)
		{
			offlineshop::CShopCache::TShopCacheItemInfo& itemInfo = qi->items[i];

			temp.dwItemID = qi->ids[i];
			temp.dwOwnerID = qi->dwOwnerID;
			offlineshop::CopyObject(temp.item, itemInfo.item);
			offlineshop::CopyObject(temp.price, itemInfo.price);
			vec.push_back(temp);
		}

#ifdef ENABLE_IRA_REWORK
		offlineshop::TShopPosition pos;
		offlineshop::CopyObject(pos, qi->pos);
		SendOfflineShopCreateNew(shopInfo, vec, pos);
#else
		SendOfflineShopCreateNew(shopInfo, vec);
#endif
		delete(qi);
	}
}

void CClientManager::OfflineShopResultSafeboxAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	offlineshop::SQueryInfoSafeboxAddItem* qi = (offlineshop::SQueryInfoSafeboxAddItem*)pQueryInfo->pvData;

	m_offlineshopSafeboxCache.PutItem(qi->dwOwnerID, msg->Get()->uiInsertID, qi->item);
	SendOfflineshopSafeboxAddItem(qi->dwOwnerID, msg->Get()->uiInsertID, qi->item);

	delete(qi);
}

void CClientManager::OfflineShopResultQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo)
{
	switch (pQueryInfo->iType)
	{
	case QID_OFFLINESHOP_ADD_ITEM:
		OfflineShopResultAddItemQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_CREATE_SHOP:
		OfflineShopResultCreateShopQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_CREATE_SHOP_ADD_ITEM:
		OfflineShopResultCreateShopAddItemQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_SAFEBOX_ADD_ITEM:
		OfflineShopResultSafeboxAddItemQuery(peer, msg, pQueryInfo);
		break;

	case QID_OFFLINESHOP_EDIT_ITEM:
	case QID_OFFLINESHOP_REMOVE_ITEM:
	case QID_OFFLINESHOP_DELETE_SHOP:
	case QID_OFFLINESHOP_DELETE_SHOP_ITEM:
	case QID_OFFLINESHOP_SHOP_CHANGE_NAME:
	case QID_OFFLINESHOP_SAFEBOX_DELETE_ITEM:
	case QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES:
	case QID_OFFLINESHOP_SAFEBOX_INSERT_VALUTES:
	case QID_OFFLINESHOP_SAFEBOX_UPDATE_VALUTES_ADDING:
	case QID_OFFLINESHOP_SHOP_UPDATE_DURATION:
		break;

	default:
		sys_err("Unknown offshop query type %d ", pQueryInfo->iType);
		break;
	}
}

void CClientManager::OfflineshopDurationProcess()
{
	m_offlineshopShopCache.ShopDurationProcess();
	m_offlineshopSafeboxCache.ItemExpirationProcess();
}

void CClientManager::OfflineshopExpiredShop(DWORD dwID)
{
	offlineshop::CShopCache::TShopCacheInfo* pInfo = nullptr;
	if (!m_offlineshopShopCache.Get(dwID, &pInfo))
		return;

	//store item in safebox
	offlineshop::CShopCache::ITEM_ITER it = pInfo->itemsmap.begin();
	for (; it != pInfo->itemsmap.end(); it++)
		m_offlineshopSafeboxCache.AddItem(dwID, it->second.item);

	//close shop
	m_offlineshopShopCache.CloseShop(dwID);
	SendOfflineshopShopExpired(dwID);
}

bool CClientManager::IsUsingOfflineshopSystem(DWORD dwID)
{
	offlineshop::CShopCache::TShopCacheInfo* pInfo = nullptr;
	if (m_offlineshopShopCache.Get(dwID, &pInfo))
	{
		return true;
	}
	else
	{
		static char query[256];
		snprintf(query, sizeof(query), "SELECT owner_id FROM player.offlineshop_shops WHERE owner_id ='%u';", dwID);
		std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(query));
		return pMsg->Get()->uiAffectedRows != 0;
	}
}

void CClientManager::OfflineshopLoadShopSafebox(CPeer* peer, DWORD dwID)
{
	offlineshop::CSafeboxCache::TSafeboxCacheInfo* pSafebox = nullptr;

	if (!m_offlineshopSafeboxCache.Get(dwID, &pSafebox))
		pSafebox = m_offlineshopSafeboxCache.LoadSafebox(dwID);

	if (!pSafebox)
	{
		sys_err("cannot load shop safebox for pid %d ", dwID);
		return;
	}

	std::vector<offlineshop::TItemInfoEx> items;
	std::vector<DWORD> ids;

	items.reserve(pSafebox->itemsmap.size());
	ids.reserve(pSafebox->itemsmap.size());

	for (itertype(pSafebox->itemsmap) it = pSafebox->itemsmap.begin();
		it != pSafebox->itemsmap.end();
		it++)
	{
		ids.push_back(it->first);
		items.push_back(it->second);
	}

	SendOfflineshopSafeboxLoad(peer, dwID, pSafebox->valutes, items, ids);
}

std::string CreateOfflineshopSelectShopItemsQuery()
{
	char szQuery[512] = "SELECT `item_id`, `owner_id`, `price_yang`, "
		" `vnum`, `count` ";
	size_t len = strlen(szQuery);

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		len += snprintf(szQuery + len, sizeof(szQuery) - len,
			",`socket%d` ", i);

	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
		len += snprintf(szQuery + len, sizeof(szQuery) - len,
			",`attr%d` , `attrval%d` ", i, i);

	len += snprintf(szQuery + len, sizeof(szQuery) - len, "%s",
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
		", trans "
#endif
		", expiration "
#ifdef ENABLE_OFFLINE_SHOP_GRID
		", display_pos "
		", display_page "
#endif
		" FROM  `player`.`offlineshop_shop_items`;");
	return szQuery;
}

