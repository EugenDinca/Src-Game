#include "stdafx.h"
#include "../../common/tables.h"
#include "packet.h"
#include "item.h"
#include "char.h"
#include "item_manager.h"
#include "desc.h"
#include "char_manager.h"
#include "banword.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "config.h"
#include "event.h"
#include "locale_service.h"
#include <fstream>
#include "sectree_manager.h"
#include "sectree.h"
#include "config.h"
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"

#include "p2p.h"
#include "questmanager.h"
#include "db.h"

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLogManager.h"
#endif

#ifdef ENABLE_OFFLINE_SHOP
bool MatchWearFlag(DWORD dwWearFilter, DWORD dwWearTable)
{
	if (dwWearFilter == 0)
	{
		return true;
	}

	static const DWORD flags[] = {
		ITEM_ANTIFLAG_MALE,
		ITEM_ANTIFLAG_FEMALE,
		ITEM_ANTIFLAG_WARRIOR,
		ITEM_ANTIFLAG_ASSASSIN,
		ITEM_ANTIFLAG_SURA,
		ITEM_ANTIFLAG_SHAMAN,
#ifdef ENABLE_WOLFMAN_CHARACTER
		ITEM_ANTIFLAG_WOLFMAN,
#endif
	};

	const size_t counts = sizeof(flags) / sizeof(DWORD);

	for (size_t i = 0; i < counts; i++)
		if (IS_SET(dwWearFilter, flags[i]) && !IS_SET(dwWearTable, flags[i]))
			return false;
	return true;
}

bool MatchAvarage(const BYTE val, const TPlayerItemAttribute* pAttributesItem)
{
	bool bFound = false;
	for (int i = 0; i < 5; i++)
	{
		if (pAttributesItem[i].bType == APPLY_NORMAL_HIT_DAMAGE_BONUS)
		{
			bFound = pAttributesItem[i].sValue >= val;
			break;
		}
	}
	return bFound;
}

std::string StringToLower(const char* name, size_t len)
{
	std::string res;
	res.resize(len);
	for (size_t i = 0; i < len; i++)
		res[i] = tolower(*(name + i));
	return res;
}


bool MatchItemName(std::string stName, const char* table, const size_t tablelen)
{
	std::string stTable = StringToLower(table, tablelen);
	return stTable.find(stName) != std::string::npos;
}

long long GetTotalAmountFromPrice(const offlineshop::TPriceInfo& price)
{
	long long total = 0;
	total += price.illYang;
	return total;
}

namespace offlineshop
{
	EVENTINFO(offlineshopempty_info)
	{
		int empty;

		offlineshopempty_info()
			: empty(0)
		{
		}
	};

	EVENTFUNC(func_offlineshop_update_duration)
	{
		offlineshop::GetManager().UpdateShopsDuration();
		offlineshop::GetManager().ClearSearchTimeMap();
		return OFFLINESHOP_DURATION_UPDATE_TIME;
	}

	offlineshop::CShopManager& GetManager()
	{
		return offlineshop::CShopManager::instance();
	}

#ifdef ENABLE_IRA_REWORK
	offlineshop::CShop* CShopManager::PutsNewShop(TShopInfo* pInfo, TShopPosition* pPosInfo)
#else
	offlineshop::CShop* CShopManager::PutsNewShop(TShopInfo* pInfo)
#endif
	{
		SHOPMAP::iterator it = m_mapShops.insert(std::make_pair(pInfo->dwOwnerID, offlineshop::CShop())).first;
		offlineshop::CShop& rShop = it->second;

		rShop.SetDuration(pInfo->dwDuration);
		rShop.SetOwnerPID(pInfo->dwOwnerID);
		rShop.SetName(pInfo->szName);
#ifdef ENABLE_SHOP_DECORATION
		rShop.SetRace(pInfo->dwShopDecoration);
#endif
#ifdef ENABLE_IRA_REWORK
		rShop.SetPosInfo(*pPosInfo);
#endif
#ifdef ENABLE_SHOPS_IN_CITIES
#ifdef ENABLE_IRA_REWORK
		CreateNewShopEntities(rShop, *pPosInfo);
#else
		CreateNewShopEntities(rShop);
#endif
#endif
		return &rShop;
	}

	offlineshop::CShop* CShopManager::GetShopByOwnerID(DWORD dwPID)
	{
		SHOPMAP::iterator it = m_mapShops.find(dwPID);
		if (it == m_mapShops.end())
			return nullptr;

		return &(it->second);
	}

	void CShopManager::RemoveSafeboxFromCache(DWORD dwOwnerID)
	{
		SAFEBOXMAP::iterator it = m_mapSafeboxs.find(dwOwnerID);
		if (it == m_mapSafeboxs.end())
			return;

		m_mapSafeboxs.erase(it);
	}

	void CShopManager::RemoveGuestFromShops(LPCHARACTER ch)
	{
		if (ch->GetOfflineShopGuest())
			ch->GetOfflineShopGuest()->RemoveGuest(ch);

		ch->SetOfflineShopGuest(NULL);

		if (ch->GetOfflineShop())
			ch->GetOfflineShop()->RemoveGuest(ch);

		ch->SetOfflineShop(NULL);
	}

#ifdef ENABLE_IRA_REWORK
	int CShopManager::GetMapIndexAllowsList(int iMapIndex)
	{
		int index = 0;

		for (auto it = s_set_offlineshop_map_allows.begin(); it != s_set_offlineshop_map_allows.end(); it++)
		{
			if (*it == iMapIndex)
				return index;

			index++;
		}

		return -1;
	}
#endif

	CShopManager::CShopManager()
	{
		offlineshopempty_info* info = AllocEventInfo<offlineshopempty_info>();
		m_eventShopDuration = event_create(func_offlineshop_update_duration, info, OFFLINESHOP_DURATION_UPDATE_TIME);
#ifdef ENABLE_IRA_REWORK
		for (auto iMapIndex : { 1, 21, 41 })
			s_set_offlineshop_map_allows.insert(iMapIndex);

		m_vecCities.resize(s_set_offlineshop_map_allows.size());
#else
		m_vecCities.resize(Offlineshop_GetMapCount());
#endif
	}

	void CShopManager::Destroy()
	{
		if (m_eventShopDuration)
			event_cancel(&m_eventShopDuration);

		m_eventShopDuration = nullptr;
		m_mapSafeboxs.clear();
		m_mapShops.clear();

#ifdef ENABLE_SHOPS_IN_CITIES
		for (itertype(m_vecCities) itCities = m_vecCities.begin(); itCities != m_vecCities.end(); itCities++)
		{
			TCityShopInfo& city = *itCities;

			for (itertype(city.entitiesByPID) it = city.entitiesByPID.begin(); it != city.entitiesByPID.end(); it++) {
				auto* Entity = it->second;
				Entity->Destroy();
				delete(Entity);
			}

			city.entitiesByPID.clear();
			city.entitiesByVID.clear();
		}

		m_vecCities.clear();
#endif
	}

#ifdef ENABLE_SHOPS_IN_CITIES
	bool IsEmptyString(const std::string& st)
	{
		return st.find_first_not_of(" \t\r\n") == std::string::npos;
	}

#ifndef ENABLE_IRA_REWORK
	bool CShopManager::__CanUseCity(size_t index)
	{
		int map_index = 0;
		Offlineshop_GetMapIndex(index, &map_index);
		return SECTREE_MANAGER::instance().GetMap(map_index) != NULL;
	}

	bool CShopManager::__CheckEntitySpawnPos(const long x, const long y, const TCityShopInfo& city)
	{
		const SHOPENTITIES_MAP& entitiesMap = city.entitiesByPID;

		for (itertype(entitiesMap) it = entitiesMap.begin(); it != entitiesMap.end(); it++)
		{
			const ShopEntity& entity = *(it->second);
			const PIXEL_POSITION pos = entity.GetXYZ();

			if (!Offlineshop_CheckPositionDistance(pos.x, pos.y, x, y))
				return false;
		}

		return true;
	}
#endif

	void CShopManager::__UpdateEntity(const offlineshop::CShop& rShop)
	{
		itertype(m_vecCities) it = m_vecCities.begin();
		for (; it != m_vecCities.end(); it++)
		{
			itertype(it->entitiesByPID) itMap = it->entitiesByPID.find(rShop.GetOwnerPID());
			if (itMap == it->entitiesByPID.end())
				continue;

			ShopEntity& ent = *(itMap->second);
			ent.SetShopName(rShop.GetName());
#ifdef ENABLE_SHOP_DECORATION
			ent.SetShopRace(rShop.GetRace());
#endif
			if (ent.GetSectree())
				ent.ViewReencode();
		}
	}

#ifdef ENABLE_IRA_REWORK
	void CShopManager::CreateNewShopEntities(offlineshop::CShop& rShop, TShopPosition& pos)
	{
		if (pos.bChannel == g_bChannel)
		{
			int cityIndex = GetMapIndexAllowsList(pos.lMapIndex);

			if (cityIndex != -1 && cityIndex < m_vecCities.size())
			{
				long x = pos.x;
				long y = pos.y;

				LPSECTREE sectree = SECTREE_MANAGER::Instance().Get(pos.lMapIndex, x, y);

				if (sectree)
				{
					ShopEntity* pEntity = new ShopEntity();
					pEntity->SetShopName(rShop.GetName());
#ifdef ENABLE_SHOP_DECORATION
					pEntity->SetShopRace(rShop.GetRace());
#endif
					pEntity->SetShopType(0); //TODO: add differents shop skins
					pEntity->SetMapIndex(pos.lMapIndex);
					pEntity->SetXYZ(x, y, 0);
					pEntity->SetShop(&rShop);
					sectree->InsertEntity(pEntity);
					pEntity->UpdateSectree();
					m_vecCities[cityIndex].entitiesByPID.insert(std::make_pair(rShop.GetOwnerPID(), pEntity));
					m_vecCities[cityIndex].entitiesByVID.insert(std::make_pair(pEntity->GetVID(), pEntity));
				}
			}
		}
	}
#else
	void CShopManager::CreateNewShopEntities(offlineshop::CShop& rShop)
	{
#define PI 3.14159265
#define RADIANS_PER_DEGREE (PI/180.0)
#define TORAD(a)	((a)*RADIANS_PER_DEGREE)
		int index = 0;
		itertype(m_vecCities) it = m_vecCities.begin();
		for (; it != m_vecCities.end(); it++, index++)
		{
			TCityShopInfo& city = *it;

			long shop_pos_x = 0, shop_pos_y = 0;
			int iCheckCount = 0;

			int map_index = 0;
			Offlineshop_GetMapIndex(index, &map_index);

			size_t ent_count = it->entitiesByPID.size();

			do {
				Offlineshop_GetNewPos(index, ent_count, &shop_pos_x, &shop_pos_y);
			} while (!__CheckEntitySpawnPos(shop_pos_x, shop_pos_y, city) && iCheckCount++ < 10);

			LPSECTREE sectree = SECTREE_MANAGER::Instance().Get(map_index, shop_pos_x, shop_pos_y);

			if (sectree)
			{
				ShopEntity* pEntity = new ShopEntity();

				pEntity->SetShopName(rShop.GetName());
#ifdef ENABLE_SHOP_DECORATION
				pEntity->SetShopRace(rShop.GetRace());
#endif
				pEntity->SetShopType(0);
				pEntity->SetMapIndex(map_index);
				pEntity->SetXYZ(shop_pos_x, shop_pos_y, 0);
				pEntity->SetShop(&rShop);

				sectree->InsertEntity(pEntity);
				pEntity->UpdateSectree();

				city.entitiesByPID.insert(std::make_pair(rShop.GetOwnerPID(), pEntity));
				city.entitiesByVID.insert(std::make_pair(pEntity->GetVID(), pEntity));
			}
		}
	}
#endif

	void CShopManager::DestroyNewShopEntities(const offlineshop::CShop& rShop)
	{
		itertype(m_vecCities) it = m_vecCities.begin();
		for (; it != m_vecCities.end(); it++)
		{
			TCityShopInfo& city = *it;

			itertype(city.entitiesByPID) iter = city.entitiesByPID.find(rShop.GetOwnerPID());

			if (iter == city.entitiesByPID.end())
			{
				//sys_err("CANNOT FOUND NEW SHOP ENTITY : %u ",rShop.GetOwnerPID());
				continue;
			}

			ShopEntity* entity = iter->second;
			DWORD dwVID = entity->GetVID();

			if (entity->GetSectree())
			{
				entity->ViewCleanup();
				entity->GetSectree()->RemoveEntity(entity);
			}

#ifdef ENABLE_IRA_REWORK
			entity->Destroy();
#endif
			delete(entity);
			city.entitiesByPID.erase(iter);
			city.entitiesByVID.erase(city.entitiesByVID.find(dwVID));
		}
	}

	void CShopManager::EncodeInsertShopEntity(ShopEntity& shop, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_INSERT_SHOP_ENTITY;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCInsertShopEntity);

		const PIXEL_POSITION pos = shop.GetXYZ();

		TSubPacketGCInsertShopEntity subpack;
		subpack.dwVID = shop.GetVID();
		subpack.iType = shop.GetShopType();

		subpack.x = pos.x;
		subpack.y = pos.y;
		subpack.z = pos.z;
#ifdef ENABLE_SHOP_DECORATION
		subpack.dwShopDecoration = shop.GetShopRace();
#endif
		strncpy(subpack.szName, shop.GetShopName(), sizeof(subpack.szName));

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}

	void CShopManager::EncodeRemoveShopEntity(ShopEntity& shop, LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_REMOVE_SHOP_ENTITY;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCRemoveShopEntity);

		TSubPacketGCRemoveShopEntity subpack;
		subpack.dwVID = shop.GetVID();

		ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
		ch->GetDesc()->Packet(&subpack, sizeof(subpack));
	}
#endif

	CShopSafebox* CShopManager::GetShopSafeboxByOwnerID(DWORD dwPID)
	{
		SAFEBOXMAP::iterator it = m_mapSafeboxs.find(dwPID);
		if (it == m_mapSafeboxs.end())
			return nullptr;
		return &(it->second);
	}

	void CShopManager::SendShopBuyDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_BUY_ITEM;

		TSubPacketGDBuyItem subpack;
		subpack.dwGuestID = dwBuyerID;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopBuyDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if (!pkShop)
			return false;

		CShopItem* pItem = nullptr;
		if (!pkShop->GetItem(dwItemID, &pItem))
			return false;

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwBuyerID);
		LPCHARACTER owner = CHARACTER_MANAGER::instance().FindByPID(dwOwnerID);

		if (ch)
		{
			LPITEM pkItem = pItem->CreateItem();
			if (!pkItem)
			{
				sys_err("cannot create item ( dwItemID %u , dwVnum %u, dwShopOwner %u, dwBuyer %u ) ", dwItemID, pItem->GetInfo()->dwVnum, dwOwnerID, dwBuyerID);
				return false;
			}

			TItemPos pos;
			if (!ch->CanTakeInventoryItem(pkItem, &pos))
			{
				M2_DESTROY_ITEM(pkItem);

				CShopSafebox* pSafebox = ch->GetShopSafebox() ? ch->GetShopSafebox() : GetShopSafeboxByOwnerID(ch->GetPlayerID());
				if (!pSafebox)
					return false;

				SendShopSafeboxAddItemDBPacket(ch->GetPlayerID(), *pItem);
				ch->NewChatPacket(STRING_D199); // item sent to user's safebvox because no place in inventory
			}
			else
			{
				pkItem->AddToCharacter(ch, pos);
			}

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
			InGameLog::InGameLogManager& rIglMgr = InGameLog::GetManager();
			InGameLog::TOfflineShopSoldLog soldlog;
			{
				strcpy(soldlog.buyerName, ch->GetName());
				soldlog.sellPrice = pItem->GetPrice()->illYang;
				soldlog.item.dwVnum = pItem->GetInfo()->dwVnum;
				soldlog.item.dwCount = pItem->GetInfo()->dwCount;

				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
				{
					memcpy(&soldlog.item.alSockets[i], &pItem->GetInfo()->alSockets[i], sizeof(long));
				}
				for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
				{
					memcpy(&soldlog.item.aAttr[i], &pItem->GetInfo()->aAttr[i], sizeof(TPlayerItemAttribute));
				}
			}
			rIglMgr.SendOfflineShopAddItemGD(soldlog, dwOwnerID);
#endif
			if (owner)
			{
				LPDESC ownerdesc = owner->GetDesc();
				if (ownerdesc)
				{
#ifdef ENABLE_OFFLINESHOP_NOTIFICATION
					SendNotificationClientPacket(ownerdesc, pItem->GetInfo()->dwVnum, pItem->GetPrice()->illYang, pItem->GetInfo()->dwCount);
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
					rIglMgr.SendOfflineShopLogAddGC(ownerdesc, soldlog);
#endif
				}
			}
			else
			{
				CCI* pkCCI = P2P_MANAGER::instance().FindByPID(dwOwnerID);
				if (pkCCI)
				{
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
					pkCCI->pkDesc->SetRelay(pkCCI->szName);
					rIglMgr.SendOfflineShopLogAddGC(pkCCI->pkDesc, soldlog);
#endif
#ifdef ENABLE_OFFLINESHOP_NOTIFICATION
					pkCCI->pkDesc->SetRelay(pkCCI->szName);
					SendNotificationClientPacket(pkCCI->pkDesc, pItem->GetInfo()->dwVnum, pItem->GetPrice()->illYang, pItem->GetInfo()->dwCount);
#endif
				}
			}

			ch->SetOfflineShopUseTime();
			DWORD dwItemID = pItem->GetID();
			pkShop->BuyItem(dwItemID);
		}
		else
		{
			DWORD dwItemID = pItem->GetID();
			pkShop->BuyItem(dwItemID);
		}

		return true;
	}

#ifdef	ENABLE_OFFLINESHOP_NOTIFICATION
	void CShopManager::SendNotificationClientPacket(LPDESC d, DWORD dwItemID, long long dwItemPrice, WORD dwItemCount)
	{
		if (!d)
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_NOTIFICATION;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCNotification);

		TSubPacketGCNotification subpack;
		subpack.dwItemID = dwItemID;
		subpack.dwItemPrice = dwItemPrice;
		subpack.dwItemCount = dwItemCount;

		TEMP_BUFFER buffer;
		buffer.write(&pack, sizeof(TPacketGCNewOfflineshop));
		buffer.write(&subpack, sizeof(TSubPacketGCNotification));

		d->Packet(buffer.read_peek(), buffer.size());
	}
#endif

	void CShopManager::SendShopEditItemDBPacket(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& rPrice, bool allEdit)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_EDIT_ITEM;

		TSubPacketGDEditItem subpack;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;
		CopyObject(subpack.priceInfo, rPrice);
		subpack.allEdit = allEdit;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopEditItemDBPacket(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& rPrice)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if (!pkShop)
			return false;

		CShopItem* pItem = nullptr;
		if (!pkShop->GetItem(dwItemID, &pItem))
			return false;

		CShopItem newItem(*pItem);
		newItem.SetPrice(rPrice);

		pkShop->ModifyItem(dwItemID, newItem);
		return true;
	}

	void CShopManager::SendShopRemoveItemDBPacket(DWORD dwOwnerID, DWORD dwItemID)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_REMOVE_ITEM;

		TSubPacketGDRemoveItem subpack;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopRemoveItemDBPacket(DWORD dwOwnerID, DWORD dwItemID)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if (!pkShop)
			return false;

		return pkShop->RemoveItem(dwItemID);
	}

	void CShopManager::SendShopAddItemDBPacket(DWORD dwOwnerID, const TItemInfo& rItemInfo)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_ADD_ITEM;

		TSubPacketGDAddItem subpack;
		subpack.dwOwnerID = dwOwnerID;
		CopyObject(subpack.itemInfo, rItemInfo);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopAddItemDBPacket(DWORD dwOwnerID, const TItemInfo& rItemInfo)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if (!pkShop)
			return false;

		CShopItem newItem(rItemInfo.dwItemID);
		newItem.SetInfo(rItemInfo.item);
		newItem.SetPrice(rItemInfo.price);
		newItem.SetOwnerID(rItemInfo.dwOwnerID);

		return pkShop->AddItem(newItem);
	}

	void CShopManager::SendShopForceCloseDBPacket(DWORD dwPID)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_SHOP_FORCE_CLOSE;

		TSubPacketGDShopForceClose subpack;
		subpack.dwOwnerID = dwPID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopForceCloseDBPacket(DWORD dwPID)
	{
		CShop* pkShop = GetShopByOwnerID(dwPID);
		if (!pkShop)
			return false;

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

		if (ch)
			ch->SetOfflineShop(NULL);

		CShop::LISTGUEST* guests = pkShop->GetGuests();

		for (CShop::LISTGUEST::iterator it = guests->begin(); it != guests->end(); it++)
		{
			LPCHARACTER chGuest = GUEST_PTR(*it);
			if (!chGuest)
				continue;

			if (ch && ch == chGuest)
				SendShopOpenMyShopNoShopClientPacket(chGuest);
			else
				SendShopListClientPacket(chGuest);

			chGuest->SetOfflineShopGuest(NULL);
		}

#ifdef ENABLE_SHOPS_IN_CITIES
		DestroyNewShopEntities(*pkShop);
#endif
		pkShop->Clear();

		m_mapShops.erase(m_mapShops.find(pkShop->GetOwnerPID()));
		return true;
	}

	void CShopManager::SendShopLockBuyItemDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_BUY_LOCK_ITEM;

		TSubPacketGDLockBuyItem subpack;
		subpack.dwGuestID = dwBuyerID;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopLockedBuyItemDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwBuyerID);

		if (!ch || !pkShop)
			return false;

		CShopItem* pkItem = nullptr;
		if (!pkShop->GetItem(dwItemID, &pkItem))
			return false;

		if (!pkItem->CanBuy(ch))
			return false;

		TPriceInfo* pPrice = pkItem->GetPrice();
		if (pPrice->illYang < 0)
			return false;
		ch->PointChange(POINT_GOLD, -pPrice->illYang);

		ch->SetOfflineShopUseTime();
		SendShopBuyDBPacket(dwBuyerID, dwOwnerID, dwItemID);
		return true;
	}

	void CShopManager::SendShopCannotBuyLockedItemDBPacket(DWORD dwOwnerID, DWORD dwItemID)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_CANNOT_BUY_LOCK_ITEM;

		TSubPacketGDCannotBuyLockItem subpack;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopExpiredDBPacket(DWORD dwPID)
	{
		CShop* pkShop = GetShopByOwnerID(dwPID);
		if (!pkShop)
			return false;

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

		if (ch)
			ch->SetOfflineShop(NULL);

		//*getting the guest list before to remove the shop
		//*that is necessary to send the shop list packets
		CShop::LISTGUEST guests = *pkShop->GetGuests();

#ifdef ENABLE_SHOPS_IN_CITIES
		DestroyNewShopEntities(*pkShop);
#endif
		pkShop->Clear();
		m_mapShops.erase(m_mapShops.find(pkShop->GetOwnerPID()));

		for (CShop::LISTGUEST::iterator it = guests.begin(); it != guests.end(); it++)
		{
			LPCHARACTER chGuest = GUEST_PTR(*it);
			if (!chGuest)
				continue;

			if (ch && ch == chGuest)
				SendShopOpenMyShopNoShopClientPacket(chGuest);
			else
				SendShopListClientPacket(chGuest);

			chGuest->SetOfflineShopGuest(NULL);
		}

		return true;
	}

#ifdef ENABLE_IRA_REWORK
	void CShopManager::SendShopCreateNewDBPacket(const TShopInfo& shop, const TShopPosition& pos, std::vector<TItemInfo>& vec)
#else
	void CShopManager::SendShopCreateNewDBPacket(const TShopInfo& shop, std::vector<TItemInfo>& vec)
#endif
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_SHOP_CREATE_NEW;

		TSubPacketGDShopCreateNew subpack;
		CopyObject(subpack.shop, shop);
#ifdef ENABLE_IRA_REWORK
		CopyObject(subpack.pos, pos);
#endif
		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		for (DWORD i = 0; i < vec.size(); i++)
			buff.write(&vec[i], sizeof(TItemInfo));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

#ifdef ENABLE_IRA_REWORK
	bool CShopManager::RecvShopCreateNewDBPacket(const TShopInfo& shop, TShopPosition& pos, std::vector<TItemInfo>& vec)
#else
	bool CShopManager::RecvShopCreateNewDBPacket(const TShopInfo& shop, std::vector<TItemInfo>& vec)
#endif
	{
		if (m_mapShops.find(shop.dwOwnerID) != m_mapShops.end())
			return false;

		CShop newShop;
		newShop.SetOwnerPID(shop.dwOwnerID);
		newShop.SetDuration(shop.dwDuration);
		newShop.SetName(shop.szName);
#ifdef ENABLE_SHOP_DECORATION
		newShop.SetRace(shop.dwShopDecoration);
#endif
		std::vector<CShopItem> items;
		items.reserve(vec.size());

		for (DWORD i = 0; i < vec.size(); i++)
		{
			const TItemInfo& rItem = vec[i];

			CShopItem shopItem(rItem.dwItemID);

			shopItem.SetOwnerID(rItem.dwOwnerID);
			shopItem.SetPrice(rItem.price);
			shopItem.SetInfo(rItem.item);

			items.push_back(shopItem);
		}

		newShop.SetItems(&items);
		SHOPMAP::iterator it = m_mapShops.insert(std::make_pair(newShop.GetOwnerPID(), newShop)).first;

#ifdef ENABLE_SHOPS_IN_CITIES
#ifdef ENABLE_IRA_REWORK
		CreateNewShopEntities(it->second, pos);
#else
		CreateNewShopEntities(it->second);
#endif
#endif

		LPCHARACTER chOwner = it->second.FindOwnerCharacter();
		if (chOwner)
		{
			chOwner->SetOfflineShop(&(it->second));
			chOwner->SetOfflineShopGuest(&(it->second));

			it->second.AddGuest(chOwner);
			SendShopOpenMyShopClientPacket(chOwner);
			chOwner->SetOfflineShopUseTime();
		}

		return true;
	}

	void CShopManager::SendShopChangeNameDBPacket(DWORD dwOwnerID, const char* szName)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_SHOP_CHANGE_NAME;

		TSubPacketGDShopChangeName subpack;
		subpack.dwOwnerID = dwOwnerID;
		strncpy(subpack.szName, szName, sizeof(subpack.szName));

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopChangeNameDBPacket(DWORD dwOwnerID, const char* szName)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if (!pkShop)
			return false;

		pkShop->SetName(szName);
		pkShop->RefreshToOwner();

#ifdef ENABLE_SHOPS_IN_CITIES
		__UpdateEntity(*pkShop);
#endif
		return true;
	}

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	bool CShopManager::RecvShopExtendTimeClientPacket(LPCHARACTER ch, DWORD dwTime)
	{
		if(!ch || !ch->GetOfflineShop())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			return false;
		}
		if (ch->IsOfflineShopActivateTime(5))
		{
			return false;
		}

		if (ch->GetGold() < OFFLINE_SHOP_EXTEND_TIME_GOLD
#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
			&& !ch->IsPremium()
#endif
		)
		{
			ch->NewChatPacket(NOT_ENOUGH_YANG);
			return false;
		}
	
		// patch with warp check
		ch->SetOfflineShopUseTime();
	
		CShop* pkShop = GetShopByOwnerID(ch->GetPlayerID());
		if(!pkShop)
			return false;

		if (pkShop->GetDuration() > OFFLINESHOP_DURATION_MAX_MINUTES-1440)
		{
			ch->NewChatPacket(OFFLINESHOP_TEXT_7);
			return false;
		}

		SendShopExtendTimeDBPacket(ch->GetPlayerID(), dwTime);

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		if (!ch->IsPremium())
#endif
			ch->PointChange(POINT_GOLD, -OFFLINE_SHOP_EXTEND_TIME_GOLD);
	
		return true;
	}

	void CShopManager::SendShopExtendTimeDBPacket(DWORD dwOwnerID, DWORD dwTime)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader	= SUBHEADER_GD_SHOP_EXTEND_TIME;

		TSubPacketGDShopExtendTime subpack;
		subpack.dwOwnerID	= dwOwnerID;
		subpack.dwTime = dwTime;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}
	
	bool CShopManager::RecvShopExtendTimeDBPacket(DWORD dwOwnerID, DWORD dwTime)
	{
		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if(!pkShop)
			return false;

		pkShop->SetDuration(pkShop->GetDuration() + dwTime);
		pkShop->RefreshToOwner();

#ifdef ENABLE_SHOPS_IN_CITIES
		__UpdateEntity(*pkShop);
#endif

		return true;
	}
#endif


	bool CShopManager::RecvShopSafeboxAddItemDBPacket(DWORD dwOwnerID, DWORD dwItemID, const TItemInfoEx& item)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwOwnerID);
		CShopSafebox* pkSafebox = ch && ch->GetShopSafebox() ? ch->GetShopSafebox() : GetShopSafeboxByOwnerID(dwOwnerID);

		if (!pkSafebox)
			return false;

		CShopItem shopItem(dwItemID);
		shopItem.SetInfo(item);
		shopItem.SetOwnerID(dwOwnerID);

		pkSafebox->AddItem(&shopItem);
		if (ch && ch->GetShopSafebox()) 
		{
			pkSafebox->RefreshToOwner(ch);
			ch->SetOfflineShopUseTime();
		}
		return true;
	}

	bool CShopManager::SendShopSafeboxAddItemDBPacket(DWORD dwOwnerID, const CShopItem& item) {
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_SAFEBOX_ADD_ITEM;

		TSubPacketGDSafeboxAddItem subpack;
		subpack.dwOwnerID = dwOwnerID;
		CopyObject(subpack.item, *item.GetInfo());

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
		return true;
	}

	bool CShopManager::RecvShopSafeboxAddValutesDBPacket(DWORD dwOwnerID, const unsigned long long& valute)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwOwnerID);
		CShopSafebox* pkSafebox = ch && ch->GetShopSafebox() ? ch->GetShopSafebox() : GetShopSafeboxByOwnerID(dwOwnerID);

		if (!pkSafebox)
			return false;

		pkSafebox->AddValute(valute);
		if (ch && ch->GetShopSafebox())
		{
			pkSafebox->RefreshToOwner(ch);
			ch->SetOfflineShopUseTime();
		}
		return true;
	}

	void CShopManager::SendShopSafeboxGetItemDBPacket(DWORD dwOwnerID, DWORD dwItemID)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_SAFEBOX_GET_ITEM;

		TSubPacketGDSafeboxGetItem subpack;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopSafeboxGetValutesDBPacket(DWORD dwOwnerID, const unsigned long long& valutes)
	{
		TPacketGDNewOfflineShop pack;
		pack.bSubHeader = SUBHEADER_GD_SAFEBOX_GET_VALUTES;

		TSubPacketGDSafeboxGetValutes subpack;
		subpack.dwOwnerID = dwOwnerID;
		CopyObject(subpack.valute, valutes);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		db_clientdesc->DBPacket(HEADER_GD_NEW_OFFLINESHOP, 0, buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopSafeboxLoadDBPacket(DWORD dwOwnerID, const unsigned long long& valute, const std::vector<DWORD>& ids, const std::vector<TItemInfoEx>& items)
	{
		if (GetShopSafeboxByOwnerID(dwOwnerID))
			return false;

		CShopSafebox::VECITEM vec;
		vec.reserve(ids.size());

		for (DWORD i = 0; i < ids.size(); i++)
		{
			CShopItem item(ids[i]);
			item.SetInfo(items[i]);
			item.SetOwnerID(dwOwnerID);

			vec.push_back(item);
		}

		CShopSafebox safebox;
		safebox.SetItems(&vec);
		safebox.SetValuteAmount(valute);

		m_mapSafeboxs.insert(std::make_pair(dwOwnerID, safebox));
		return true;
	}

	bool CShopManager::RecvShopSafeboxExpiredItemDBPacket(DWORD dwOwnerID, DWORD dwItemID) 
	{
		CShopSafebox* data = GetShopSafeboxByOwnerID(dwOwnerID);
		if (data) 
		{
			data->RemoveItem(dwItemID);
			data->RefreshToOwner();
		}
		return true;
	}

	bool CShopManager::RecvShopCreateNewClientPacket(LPCHARACTER ch, TShopInfo& rShopInfo, std::vector<TShopItemInfo>& vec)
	{
		if (!ch || ch->GetOfflineShop())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
			return true;
		}

		if (ch->IsOfflineShopActivateTime(5))
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", 3);
			return true;
		}

		if (ch->GetGold() < OFFLINE_SHOP_CREATE_GOLD
#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
			&& !ch->IsPremium()
#endif
			)
		{
			ch->NewChatPacket(OFFLINESHOP_TEXT_1, "%d", OFFLINE_SHOP_CREATE_GOLD);
			return true;
		}

#ifdef ENABLE_SHOP_DECORATION
		if ((rShopInfo.dwShopDecoration < 30000) || (rShopInfo.dwShopDecoration > 30007)) {
			return false;
		}
#endif

		if (g_bChannel != 99)
		{
			ch->NewChatPacket(OFFLINESHOP_TEXT_2);
			return true;
		}
		if ((ch->GetMapIndex() == 21 || ch->GetMapIndex() == 1 || ch->GetMapIndex() == 41) == false)
		{
			ch->NewChatPacket(OFFLINESHOP_TEXT_3);
			return true;
		}

		static char szNameChecked[OFFLINE_SHOP_NAME_MAX_LEN];

		strncpy(szNameChecked, rShopInfo.szName, sizeof(szNameChecked));
		if (CBanwordManager::instance().CheckString(szNameChecked, strlen(szNameChecked)))
			return false;

		snprintf(rShopInfo.szName, sizeof(rShopInfo.szName), "%s@%s", ch->GetName(), szNameChecked);

		std::vector<TItemInfo> vecItem;
		vecItem.reserve(vec.size());

		rShopInfo.dwOwnerID = ch->GetPlayerID();
#ifdef ENABLE_IRA_REWORK
		TShopPosition rShopPos;
		rShopPos.lMapIndex = ch->GetMapIndex();
		rShopPos.x = ch->GetX();
		rShopPos.y = ch->GetY();
		rShopPos.bChannel = g_bChannel;
#endif
		TItemInfo itemInfo;

		for (DWORD i = 0; i < vec.size(); i++)
		{
			TShopItemInfo& rShopItem = vec[i];

			LPITEM item = ch->GetItem(rShopItem.pos);
			if (!item)
				return false;

			if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
				return false;

			if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MYSHOP))
				return false;

			if (item->isLocked() || item->IsEquipped() || item->IsExchanging())
			{
				ch->NewChatPacket(STRING_D61);
				return true;
			}

			for (DWORD j = 0; j < vec.size(); j++)
			{
				if (i == j)
					continue;

				TShopItemInfo& rShopItemCheck = vec[j];
				if (rShopItemCheck.pos == rShopItem.pos)
					return false;
			}

			ZeroObject(itemInfo);

			itemInfo.dwOwnerID = ch->GetPlayerID();
			memcpy(itemInfo.item.aAttr, item->GetAttributes(), sizeof(itemInfo.item.aAttr));
			memcpy(itemInfo.item.alSockets, item->GetSockets(), sizeof(itemInfo.item.alSockets));

			itemInfo.item.dwVnum = item->GetVnum();
			itemInfo.item.dwCount = item->GetCount();
			itemInfo.item.expiration = GetItemExpiration(item);
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
			itemInfo.item.dwTransmutation = item->GetTransmutation();
#endif
#ifdef ENABLE_OFFLINE_SHOP_GRID
			CopyObject(itemInfo.item.display_pos, rShopItem.display_pos);
#endif
			CopyObject(itemInfo.price, rShopItem.price);
			vecItem.push_back(itemInfo);
		}

		for (DWORD i = 0; i < vec.size(); i++)
		{
			TShopItemInfo& rShopItem = vec[i];
			LPITEM item = ch->GetItem(rShopItem.pos);
			M2_DESTROY_ITEM(item->RemoveFromCharacter());
		}

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		if (ch->IsPremium())
		{
			rShopInfo.dwDuration = MIN(rShopInfo.dwDuration, OFFLINESHOP_DURATION_MAX_MINUTES_PREMIUM);
		}
		else
		{
			rShopInfo.dwDuration = MIN(rShopInfo.dwDuration, OFFLINESHOP_DURATION_MAX_MINUTES);
			ch->PointChange(POINT_GOLD, -OFFLINE_SHOP_CREATE_GOLD);
		}
#else
		rShopInfo.dwDuration = MIN(rShopInfo.dwDuration, OFFLINESHOP_DURATION_MAX_MINUTES);
#endif

#ifdef ENABLE_IRA_REWORK
		SendShopCreateNewDBPacket(rShopInfo, rShopPos, vecItem);
#else
		SendShopCreateNewDBPacket(rShopInfo, vecItem);
#endif
		ch->SetOfflineShopUseTime();
		return true;
	}

	bool CShopManager::RecvShopChangeNameClientPacket(LPCHARACTER ch, const char* szName)
	{
		if (!ch || !ch->GetOfflineShop())
			return false;

		static char szNameChecked[OFFLINE_SHOP_NAME_MAX_LEN];
		static char szFullName[OFFLINE_SHOP_NAME_MAX_LEN];

		//cheching about bandword
		strncpy(szNameChecked, szName, sizeof(szNameChecked));
		if (CBanwordManager::instance().CheckString(szNameChecked, strlen(szNameChecked)))
			return false;

		//making full name
		snprintf(szFullName, sizeof(szFullName), "%s@%s", ch->GetName(), szNameChecked);
		ch->SetOfflineShopUseTime();
		SendShopChangeNameDBPacket(ch->GetPlayerID(), szFullName);
		return true;
	}

	bool CShopManager::RecvShopForceCloseClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetOfflineShop())
			return false;

		SendShopForceCloseDBPacket(ch->GetPlayerID());
		ch->SetOfflineShopUseTime();
		return true;
	}

	bool CShopManager::RecvShopRequestListClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return false;

		SendShopListClientPacket(ch);
		ch->SetOfflineShopUseTime();
		return true;
	}

	bool CShopManager::RecvShopOpenClientPacket(LPCHARACTER ch, DWORD dwOwnerID)
	{
		if (!ch || !ch->GetDesc())
			return false;

		CShop* pkShop = GetShopByOwnerID(dwOwnerID);
		if (!pkShop)
			return false;

		if (ch->GetOfflineShopGuest())
			ch->GetOfflineShopGuest()->RemoveGuest(ch);

		if (ch->GetPlayerID() == dwOwnerID)
			SendShopOpenMyShopClientPacket(ch);
		else
			SendShopOpenClientPacket(ch, pkShop);

		pkShop->AddGuest(ch);
		ch->SetOfflineShopGuest(pkShop);
		ch->SetOfflineShopUseTime();
		return true;
	}

	bool CShopManager::RecvShopOpenMyShopClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return false;

		if (!ch->GetOfflineShop())
		{
			SendShopOpenMyShopNoShopClientPacket(ch);
		}
		else
		{
			SendShopOpenMyShopClientPacket(ch);
			ch->GetOfflineShop()->AddGuest(ch);
			ch->SetOfflineShopGuest(ch->GetOfflineShop());
		}
		ch->SetOfflineShopUseTime();
		return true;
	}

	bool CShopManager::RecvShopBuyItemClientPacket(LPCHARACTER ch, DWORD dwOwnerID, DWORD dwItemID, bool isSearch)
	{
		if (!ch)
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			return false;
		}

		CShop* pkShop = nullptr;
		if (!(pkShop = GetShopByOwnerID(dwOwnerID)))
			return false;

		CShopItem* pitem = nullptr;
		if (!pkShop->GetItem(dwItemID, &pitem))
			return false;

		if (!pitem->CanBuy(ch))
			return false;


#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		if (isSearch)
		{
			if (!ch->IsPremium())
			{
				const TShopPosition& pos = pkShop->GetShopPositions();

				if (ch->GetMapIndex() == pos.lMapIndex)
				{
					ch->Show(ch->GetMapIndex(), pos.x, pos.y, 0); // Ayni maptaysa goto attik 
				}
				else
				{
					ch->WarpSet(pos.x, pos.y, pos.lMapIndex);
				}
			}
			else
			{
				SendShopBuyItemFromSearchClientPacket(ch, dwOwnerID, dwItemID);
				SendShopLockBuyItemDBPacket(ch->GetPlayerID(), dwOwnerID, dwItemID);
				ch->SetOfflineShopUseTime();
			}
		}
		else
		{
			SendShopBuyItemFromSearchClientPacket(ch, dwOwnerID, dwItemID);
			SendShopLockBuyItemDBPacket(ch->GetPlayerID(), dwOwnerID, dwItemID);
			ch->SetOfflineShopUseTime();
		}	
#else
		if (isSearch)
			SendShopBuyItemFromSearchClientPacket(ch, dwOwnerID, dwItemID);

		SendShopLockBuyItemDBPacket(ch->GetPlayerID(), dwOwnerID, dwItemID);
		ch->SetOfflineShopUseTime();
#endif
		return true;
	}

#ifdef ENABLE_SHOPS_IN_CITIES
	bool CShopManager::RecvShopClickEntity(LPCHARACTER ch, DWORD dwShopEntityVID)
	{
		for (itertype(m_vecCities) it = m_vecCities.begin(); it != m_vecCities.end(); it++)
		{
			itertype(it->entitiesByVID) iterMap = it->entitiesByVID.find(dwShopEntityVID);
			if (it->entitiesByVID.end() == iterMap)
				continue;

			DWORD dwPID = iterMap->second->GetShop()->GetOwnerPID();

			RecvShopOpenClientPacket(ch, dwPID);
			return true;
		}

		sys_err("cannot found clicked entity , %s vid %u ", ch->GetName(), dwShopEntityVID);
		return false;
	}
#endif

	void CShopManager::SendShopListClientPacket(LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TEMP_BUFFER buff;
		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_LIST;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCShopList) + (m_mapShops.size() * sizeof(TShopInfo));

		buff.write(&pack, sizeof(pack));

		TSubPacketGCShopList subPack;
		subPack.dwShopCount = m_mapShops.size();
		buff.write(&subPack, sizeof(subPack));

		TShopInfo shopInfo;

		itertype(m_mapShops) it = m_mapShops.begin();
		for (; it != m_mapShops.end(); it++)
		{
			const CShop& rShop = it->second;
			DWORD dwOwner = it->first;
			ZeroObject(shopInfo);
			shopInfo.dwCount = rShop.GetItems()->size();
			shopInfo.dwDuration = rShop.GetDuration();
			shopInfo.dwOwnerID = dwOwner;
			strncpy(shopInfo.szName, rShop.GetName(), sizeof(shopInfo.szName));

			buff.write(&shopInfo, sizeof(shopInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOpenClientPacket(LPCHARACTER ch, CShop* pkShop)
	{
		if (!ch || !ch->GetDesc() || !pkShop)
			return;

		CShop::VECSHOPITEM* pVec = pkShop->GetItems();
		TEMP_BUFFER buff;
		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_OPEN;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCShopOpen) + sizeof(TItemInfo) * pVec->size();

		buff.write(&pack, sizeof(pack));

		TSubPacketGCShopOpen subPack;
		subPack.shop.dwCount = pVec->size();
		subPack.shop.dwDuration = pkShop->GetDuration();
		subPack.shop.dwOwnerID = pkShop->GetOwnerPID();
		strncpy(subPack.shop.szName, pkShop->GetName(), sizeof(subPack.shop.szName));

		buff.write(&subPack, sizeof(subPack));

		TItemInfo itemInfo;

		for (DWORD i = 0; i < pVec->size(); i++)
		{
			CShopItem& rItem = pVec->at(i);
			ZeroObject(itemInfo);

			itemInfo.dwItemID = rItem.GetID();
			itemInfo.dwOwnerID = pkShop->GetOwnerPID();
			CopyObject(itemInfo.item, *(rItem.GetInfo()));
			CopyObject(itemInfo.price, *(rItem.GetPrice()));

			buff.write(&itemInfo, sizeof(itemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOpenMyShopNoShopClientPacket(LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP;
		pack.wSize = sizeof(pack);
		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}

	void CShopManager::SendShopBuyItemFromSearchClientPacket(LPCHARACTER ch, DWORD dwOwnerID, DWORD dwItemID)
	{
		if (!ch->GetDesc())
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_BUY_ITEM_FROM_SEARCH;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCShopBuyItemFromSearch);
		TSubPacketGCShopBuyItemFromSearch subpack;
		subpack.dwOwnerID = dwOwnerID;
		subpack.dwItemID = dwItemID;
		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));
		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::SendShopOpenMyShopClientPacket(LPCHARACTER ch)
	{
		if (!ch->GetDesc())
			return;

		if (!ch->GetOfflineShop())
			return;

		CShop* pkShop = ch->GetOfflineShop();
		DWORD dwOwnerID = ch->GetPlayerID();

		CShop::VECSHOPITEM* pVec = pkShop->GetItems();

		TEMP_BUFFER buff;
		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_OPEN_OWNER;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCShopOpenOwner) + sizeof(TItemInfo) * pVec->size();

		buff.write(&pack, sizeof(pack));

		TSubPacketGCShopOpenOwner subPack;
		subPack.shop.dwCount = pVec->size();
		subPack.shop.dwDuration = pkShop->GetDuration();
		subPack.shop.dwOwnerID = dwOwnerID;

		strncpy(subPack.shop.szName, pkShop->GetName(), sizeof(subPack.shop.szName));

		buff.write(&subPack, sizeof(subPack));

		TItemInfo itemInfo;

		for (DWORD i = 0; i < pVec->size(); i++)
		{
			CShopItem& rItem = pVec->at(i);
			ZeroObject(itemInfo);

			itemInfo.dwItemID = rItem.GetID();
			itemInfo.dwOwnerID = dwOwnerID;
			CopyObject(itemInfo.item, *(rItem.GetInfo()));
			CopyObject(itemInfo.price, *(rItem.GetPrice()));

			buff.write(&itemInfo, sizeof(itemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());

	}

	void CShopManager::SendShopForceClosedClientPacket(DWORD dwOwnerID)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwOwnerID);
		if (!ch || !ch->GetDesc())
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_OPEN_OWNER;

		pack.wSize = sizeof(pack);
		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}

	bool CShopManager::RecvShopAddItemClientPacket(LPCHARACTER ch, const TItemPos& pos, const TPriceInfo& price
#ifdef ENABLE_OFFLINE_SHOP_GRID
		, const TItemDisplayPos& display_pos
#endif
	)
	{
		if (!ch || !ch->GetOfflineShop())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
			return true;
		}

		if (ch->IsOfflineShopActivateTime(3))
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d",3);
			return true;
		}

		LPITEM pkItem = ch->GetItem(pos);
		if (!pkItem)
			return false;

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		if (!ch->IsPremium())
		{
			CShop* pkShop = ch->GetOfflineShop();

			const TShopPosition& pos = pkShop->GetShopPositions();

			if (ch->GetMapIndex() != pos.lMapIndex)
			{
				ch->NewChatPacket(OFFLINESHOP_TEXT_9);
#ifdef ENABLE_DIZCIYA_GOTTEN
				ch->ChatPacket(1, "Bulundugun index %d pazar index %d", ch->GetMapIndex(), pos.lMapIndex);
#endif
				return true;
			}
		}
#endif

		if (pkItem->isLocked() || pkItem->IsEquipped() || pkItem->IsExchanging())
		{
			ch->NewChatPacket(STRING_D61);
			return true;
		}

		if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_GIVE) || IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_MYSHOP))
		{
			ch->NewChatPacket(OFFLINESHOP_TEXT_10);
			return true;
		}

		TItemInfo itemInfo;
		ZeroObject(itemInfo);

		itemInfo.dwOwnerID = ch->GetPlayerID();
		itemInfo.item.dwVnum = pkItem->GetVnum();
		itemInfo.item.dwCount = (DWORD)pkItem->GetCount();
		itemInfo.item.expiration = GetItemExpiration(pkItem);
		memcpy(itemInfo.item.aAttr, pkItem->GetAttributes(), sizeof(itemInfo.item.aAttr));
		memcpy(itemInfo.item.alSockets, pkItem->GetSockets(), sizeof(itemInfo.item.alSockets));

#ifdef __ENABLE_CHANGELOOK_SYSTEM__
		itemInfo.item.dwTransmutation = pkItem->GetTransmutation();
#endif

		CopyObject(itemInfo.price, price);
#ifdef ENABLE_OFFLINE_SHOP_GRID
		CopyObject(itemInfo.item.display_pos, display_pos);
#endif

		M2_DESTROY_ITEM(pkItem->RemoveFromCharacter());

		SendShopAddItemDBPacket(ch->GetPlayerID(), itemInfo);
		return true;
	}

	bool CShopManager::RecvShopRemoveItemClientPacket(LPCHARACTER ch, DWORD dwItemID)
	{
		if (!ch || !ch->GetOfflineShop())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
			return true;
		}

		if (ch->IsOfflineShopActivateTime(3))
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", 3);
			return true;
		}

		CShop* pkShop = ch->GetOfflineShop();
		CShopItem* pItem = nullptr;

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		if (!ch->IsPremium())
		{
			const TShopPosition& pos = pkShop->GetShopPositions();

			if (ch->GetMapIndex() != pos.lMapIndex)
			{
				ch->NewChatPacket(OFFLINESHOP_TEXT_9);
#ifdef ENABLE_DIZCIYA_GOTTEN
				ch->ChatPacket(1, "Bulundugun index %d pazar index %d", ch->GetMapIndex(), pos.lMapIndex);
#endif
				return true;
			}
		}
#endif

		if (pkShop->GetItems()->size() == 1)
		{
			ch->NewChatPacket(STRING_D200);
			return true;
		}

		if (!pkShop->GetItem(dwItemID, &pItem))
			return false;

		SendShopRemoveItemDBPacket(pkShop->GetOwnerPID(), pItem->GetID());
		return true;
	}

	bool CShopManager::RecvShopEditItemClientPacket(LPCHARACTER ch, DWORD dwItemID, const TPriceInfo& price, bool allEdit)
	{
		if (!ch || !ch->GetOfflineShop())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
			return true;
		}

		if (ch->IsOfflineShopActivateTime(3))
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", 3);
			return true;
		}

		CShop* pkShop = ch->GetOfflineShop();
		CShopItem* pItem = nullptr;

		if (!pkShop->GetItem(dwItemID, &pItem))
			return false;

		TPriceInfo* pPrice = pItem->GetPrice();

		if (price.illYang == pPrice->illYang)
			return true;

		ch->SetOfflineShopUseTime();//Checker fix
		SendShopEditItemDBPacket(pkShop->GetOwnerPID(), dwItemID, price, allEdit);
		return true;
	}

	bool CShopManager::RecvShopFilterRequestClientPacket(LPCHARACTER ch, const TFilterInfo& filter)
	{
		if (!ch)
			return false;

		std::vector<TItemInfo> vec;
		if (!CheckSearchTime(ch->GetPlayerID()))
		{
			SendShopFilterResultClientPacket(ch, vec);
			ch->NewChatPacket(STRING_D201);
			return true;
		}
		std::string stName = StringToLower(filter.szName, strnlen(filter.szName, sizeof(filter.szName)));

		itertype(m_mapShops) cit = m_mapShops.begin();

		for (; cit != m_mapShops.end(); cit++)
		{
			const CShop& rcShop = cit->second;

			if (rcShop.GetOwnerPID() == ch->GetPlayerID())
				continue;

			CShop::VECSHOPITEM* pShopItems = rcShop.GetItems();

			if (filter.FindCharacter)
			{
				if (strnlen(filter.szName, sizeof(filter.szName)) == 0)
				{
					SendShopFilterResultClientPacket(ch, vec);
					ch->NewChatPacket(OFFLINESHOP_TEXT_6);
					return true;
				}

				std::string shopOwnerName = rcShop.GetName();
				shopOwnerName = shopOwnerName.substr(0, shopOwnerName.find("@"));
				if (!MatchItemName(stName, shopOwnerName.c_str(), shopOwnerName.length()))
				{
					continue;
				}
			}

			itertype(*pShopItems) cItemIter = pShopItems->begin();
			for (; cItemIter != pShopItems->end(); cItemIter++)
			{
				const CShopItem& rItem = *cItemIter;
				const TItemInfoEx& rItemInfo = *rItem.GetInfo();
				const TPriceInfo& rItemPrice = *rItem.GetPrice();

				TItemTable* pTable = ITEM_MANAGER::instance().GetTable(rItemInfo.dwVnum);
				if (!pTable)
				{
					sys_err("CANNOT FIND ITEM TABLE [%d]");
					continue;
				}

				if (filter.bType != ITEM_NONE && filter.bType != pTable->bType)
					continue;

				if (filter.bType != ITEM_NONE && filter.bSubType != SUBTYPE_NOSET && filter.bSubType != pTable->bSubType)
					continue;

				if ((filter.iLevelStart != 0 || filter.iLevelEnd != 0))
				{
					int iLimitLevel = pTable->aLimits[0].bType == LIMIT_LEVEL ? pTable->aLimits[0].lValue : pTable->aLimits[1].bType == LIMIT_LEVEL ? pTable->aLimits[1].lValue : 0;
					if (iLimitLevel == 0)
						continue;

					if (iLimitLevel < filter.iLevelStart && filter.iLevelStart != 0)
						continue;

					if (iLimitLevel > filter.iLevelEnd && filter.iLevelEnd != 0)
						continue;
				}

				if (filter.priceStart.illYang != 0)
				{
					if (GetTotalAmountFromPrice(rItemPrice) < filter.priceStart.illYang)
						continue;
				}

				if (filter.priceEnd.illYang != 0)
				{
					if (GetTotalAmountFromPrice(rItemPrice) > filter.priceEnd.illYang)
						continue;
				}

				if (filter.minCount != 0)
				{
					if (rItemInfo.dwCount < filter.minCount)
						continue;
				}

				if (filter.maxCount != 0)
				{
					if (rItemInfo.dwCount > filter.maxCount)
						continue;
				}

				if (filter.minAbs != 0)
				{
					if (pTable->bType != ITEM_COSTUME || pTable->bSubType != COSTUME_ACCE)
						continue;
					if (rItemInfo.alSockets[ACCE_ABSORPTION_SOCKET] < filter.minAbs)
						continue;
				}

				if (filter.maxAbs != 0)
				{
					if (pTable->bType != ITEM_COSTUME || pTable->bSubType != COSTUME_ACCE)
						continue;

					if (rItemInfo.alSockets[ACCE_ABSORPTION_SOCKET] > filter.maxAbs)
						continue;
				}

				if (filter.alchemyGrade != 0)
				{
					if (pTable->bType != ITEM_DS || pTable->bSubType != filter.alchemyGrade - 1)
						continue;
				}

				if (filter.alchemyPurity != 0)
				{
					if (pTable->bType != ITEM_DS)
						continue;

					DWORD dsGradeStep{ ((pTable->dwVnum / 100) % 10) +1 };

					if (filter.alchemyPurity != dsGradeStep)
						continue;
				}

				if (filter.sashLevel != 0)
				{
					if (pTable->bType != ITEM_COSTUME || pTable->bSubType != COSTUME_ACCE)
						continue;

					const auto& it{ acceGradeMap.find(filter.sashLevel) };

					if (it != acceGradeMap.end())
					{
						if (it->first != 4)
						{
							if (it->second != rItemInfo.alSockets[ACCE_ABSORPTION_SOCKET])
								continue;
						}
						else
						{
							if (rItemInfo.alSockets[ACCE_ABSORPTION_SOCKET] < it->second)
								continue;
						}
					}
				}

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
				if (!filter.FindCharacter)
				{
					if (strnlen(filter.szName, sizeof(filter.szName)) != 0)
					{
						const std::string& item_name = locale_item_find(rItemInfo.dwVnum, ch->GetLanguage());
						if (!MatchItemName(stName, item_name.c_str(), item_name.length()))
						{
							continue;
						}
					}
				}
#else
				if (!filter.FindCharacter)
				{
					if (strnlen(filter.szName, sizeof(filter.szName)) != 0 && !MatchItemName(stName, pTable->szLocaleName, strnlen(pTable->szLocaleName, ITEM_NAME_MAX_LEN)))
					{
						continue;
					}
				}
#endif

				if (!MatchWearFlag(filter.dwWearFlag, pTable->dwAntiFlags))
					continue;

				if (filter.minAvg != 0)
				{
					if (pTable->bType == ITEM_COSTUME && pTable->bSubType == COSTUME_ACCE)
					{
						BYTE acceAvg = (100 / rItemInfo.alSockets[ACCE_ABSORPTION_SOCKET]) * filter.minAvg;
						if (!MatchAvarage(acceAvg, rItemInfo.aAttr))
							continue;
					}
					else
					{
						if (pTable->bType != ITEM_WEAPON)
							continue;

						if (!MatchAvarage(filter.minAvg, rItemInfo.aAttr))
							continue;
					}
				}

				TItemInfo itemInfo;
				CopyObject(itemInfo.item, rItemInfo);
				CopyObject(itemInfo.price, rItemPrice);

				itemInfo.dwItemID = rItem.GetID();
				itemInfo.dwOwnerID = rcShop.GetOwnerPID();

				std::string shopOwnerName = rcShop.GetName();
				shopOwnerName = shopOwnerName.substr(0, shopOwnerName.find("@"));
				strcpy(itemInfo.dwOwnerName, shopOwnerName.c_str());

				vec.push_back(itemInfo);

				if (vec.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
					break;
			}

			if (vec.size() >= OFFLINESHOP_MAX_SEARCH_RESULT)
				break;
		}

		SendShopFilterResultClientPacket(ch, vec);
		return true;
	}

	void CShopManager::SendShopFilterResultClientPacket(LPCHARACTER ch, const std::vector<TItemInfo>& items)
	{
		if (!ch || !ch->GetDesc())
			return;

		TEMP_BUFFER buff;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.bSubHeader = SUBHEADER_GC_SHOP_FILTER_RESULT;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCShopFilterResult) + sizeof(TItemInfo) * items.size();
		buff.write(&pack, sizeof(pack));

		TSubPacketGCShopFilterResult subpack;
		subpack.dwCount = items.size();
		buff.write(&subpack, sizeof(subpack));

		for (DWORD i = 0; i < items.size(); i++)
		{
			const TItemInfo& rItemInfo = items[i];
			buff.write(&rItemInfo, sizeof(rItemInfo));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	bool CShopManager::RecvShopSafeboxOpenClientPacket(LPCHARACTER ch)
	{
		if (!ch || ch->GetShopSafebox())
			return false;

		CShopSafebox* pkSafebox = GetShopSafeboxByOwnerID(ch->GetPlayerID());
		if (!pkSafebox)
			return false;

		ch->SetShopSafebox(pkSafebox);
		pkSafebox->RefreshToOwner(ch);
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetItemClientPacket(LPCHARACTER ch, DWORD dwItemID)
	{
		if (!ch || !ch->GetShopSafebox())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
			return true;
		}

		if (ch->IsOfflineShopActivateTime(3))
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", 3);
			return true;
		}

		CShopSafebox* pkSafebox = ch->GetShopSafebox();
		CShopItem* pItem = nullptr;

		if (!pkSafebox->GetItem(dwItemID, &pItem))
		{
			// ch->ChatPacket(CHAT_TYPE_INFO, "Naptin kardes item yok ortada sen neyi istyon");
			sys_err("Adam safeboxta olmayan itemi cekmeye calisiyor %s", ch->GetName());
			return false;
		}		

		LPITEM pkItem = pItem->CreateItem();
		if (!pkItem)
			return false;

		TItemPos itemPos;
		if (!ch->CanTakeInventoryItem(pkItem, &itemPos))
		{
			M2_DESTROY_ITEM(pkItem);
			return false;
		}

		if (pkSafebox->RemoveItemControl(dwItemID))
		{
			if (pkItem->AddToCharacter(ch, itemPos))
			{
				pkSafebox->RemoveItem(dwItemID);
				pkSafebox->RefreshToOwner();
				SendShopSafeboxGetItemDBPacket(ch->GetPlayerID(), dwItemID);
#ifdef ENABLE_OFFLINE_SHOP_LOG
				SafeboxRequestItemLogs(ch->GetPlayerID(), pkItem->GetID(), pkItem->GetVnum());
#endif
			}
			else
			{
				M2_DESTROY_ITEM(pkItem);
				return false;
			}
		}
		else
		{
			return false;
		}
		return true;
	}

	bool CShopManager::RecvShopSafeboxGetValutesClientPacket(LPCHARACTER ch, const unsigned long long& valutes)
	{
		if (!ch || !ch->GetShopSafebox())
			return false;

		if (!ch->OfflineShopActivateCheck())
		{
			ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
			return true;
		}

		if (ch->IsOfflineShopActivateTime(3))
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", 3);
			return true;
		}

		if ((unsigned long long)ch->GetGold() + valutes > (unsigned long long)GOLD_MAX)
			return false;

		if (valutes < 1)
			return false;

		CShopSafebox* pkSafebox = ch->GetShopSafebox();

		if (!pkSafebox->RemoveValute(valutes))
			return false;

#ifdef ENABLE_OFFLINE_SHOP_LOG
		long long oldgold = ch->GetGold();
#endif

		ch->PointChange(POINT_GOLD, valutes);

#ifdef ENABLE_OFFLINE_SHOP_LOG
		long long newgold = ch->GetGold();
		SafeboxRequestGoldLogs(ch->GetPlayerID(), oldgold, newgold, valutes);
#endif

		pkSafebox->RefreshToOwner();
		SendShopSafeboxGetValutesDBPacket(ch->GetPlayerID(), valutes);
		return true;
	}

	bool CShopManager::RecvShopSafeboxCloseClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetShopSafebox())
			return false;

		ch->SetShopSafebox(NULL);
		return true;
	}

	void CShopManager::SendShopSafeboxRefresh(LPCHARACTER ch, const unsigned long long& valute, const std::vector<CShopItem>& vec)
	{
		if (!ch->GetDesc())
			return;

		if (!ch || !ch->GetShopSafebox())
			return;

		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketGCShopSafeboxRefresh) + ((sizeof(DWORD) + sizeof(TItemInfoEx)) * vec.size());
		pack.bSubHeader = SUBHEADER_GC_SHOP_SAFEBOX_REFRESH;

		TSubPacketGCShopSafeboxRefresh subpack;
		subpack.dwItemCount = vec.size();
		CopyObject(subpack.valute, valute);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		TItemInfoEx item;
		DWORD dwItemID = 0;

		for (itertype(vec) it = vec.begin(); it != vec.end(); it++)
		{
			const CShopItem& shopitem = *it;

			dwItemID = shopitem.GetID();
			CopyObject(item, *shopitem.GetInfo());

			buff.write(&dwItemID, sizeof(dwItemID));
			buff.write(&item, sizeof(item));
		}

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}

	void CShopManager::RecvCloseBoardClientPacket(LPCHARACTER ch)
	{
		if (!ch || !ch->GetDesc())
			return;

		if (ch->GetShopSafebox())
			ch->SetShopSafebox(NULL);

		if (ch->GetOfflineShopGuest())
		{
			ch->GetOfflineShopGuest()->RemoveGuest(ch);
			ch->SetOfflineShopGuest(NULL);
		}

		if (ch->GetOfflineShop())
			ch->GetOfflineShop()->RemoveGuest(ch);
	}

	void CShopManager::UpdateShopsDuration()
	{
		SHOPMAP::iterator it = m_mapShops.begin();
		for (; it != m_mapShops.end(); it++)
		{
			CShop& shop = it->second;

			if (shop.GetDuration() > 0)
				shop.DecreaseDuration();
		}
	}

	void CShopManager::ClearSearchTimeMap()
	{
		m_searchTimeMap.clear();
	}

	bool CShopManager::CheckSearchTime(DWORD dwPID)
	{
		itertype(m_searchTimeMap) it = m_searchTimeMap.find(dwPID);
		if (it == m_searchTimeMap.end())
		{
			m_searchTimeMap.insert(std::make_pair(dwPID, get_dword_time()));
			return true;
		}

		if (it->second + OFFLINESHOP_SECONDS_PER_SEARCH * 1000 > get_dword_time())
			return false;

		it->second = get_dword_time();
		return true;
	}

#ifdef ENABLE_OFFLINE_SHOP_LOG
	void CShopManager::OfflineShopLogs(DWORD ownerid, const char* log_type, const char* log)
	{
		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO log.offlineshop_logs (owner_id, log_type, log, event_date) VALUES(%d, '%s', '%s', NOW())",
			ownerid, log_type, log);
		DBManager::Instance().DirectQuery(szQuery);
	}

	void CShopManager::SafeboxRequestGoldLogs(DWORD ownerid, long long oldgold, long long newgold, unsigned long long requestgold)
	{
		char szLog[128];
		snprintf(szLog, sizeof(szLog), "Old gold: %lld New gold: %lld Request Gold: %llu", oldgold, newgold, requestgold);
		
		char szLog_type[64];
		snprintf(szLog_type, sizeof(szLog_type), "SAFEBOX_REQUEST_GOLD");

		OfflineShopLogs(ownerid, szLog_type, szLog);
	}
	void CShopManager::SafeboxRequestItemLogs(DWORD ownerid, DWORD itemid, DWORD itemvnum)
	{
		char szLog[128];
		snprintf(szLog, sizeof(szLog), "Item Id: %u ItemVnum: %u", itemid, itemvnum);

		char szLog_type[64];
		snprintf(szLog_type, sizeof(szLog_type), "SAFEBOX_REQUEST_ITEM");

		OfflineShopLogs(ownerid, szLog_type, szLog);
	}
#endif

#ifdef ENABLE_AVERAGE_PRICE
	void CShopManager::AveragePriceUpdate()
	{
		std::unordered_map<DWORD, long long> mapAverage{};

		itertype(m_mapShops) shopIt = m_mapShops.begin();
		for (; shopIt != m_mapShops.end(); shopIt++)
		{
			const CShop& shop = shopIt->second;
			CShop::VECSHOPITEM* shopItems = shop.GetItems();

			itertype(*shopItems) itemIt = shopItems->begin();
			for (; itemIt != shopItems->end(); itemIt++)
			{
				const CShopItem& item = *itemIt;
				const TPriceInfo& itemPrice = *item.GetPrice();

				long long onePrice = itemPrice.illYang / item.GetInfo()->dwCount;

				auto avgIt = mapAverage.find(item.GetInfo()->dwVnum);
				if (avgIt != mapAverage.end())
				{
					avgIt->second = (avgIt->second + onePrice) / 2;
				}
				else
				{
					mapAverage.insert(std::make_pair(item.GetInfo()->dwVnum, onePrice));
				}
			}
		}

		m_mapAveragePrice.clear();
		m_mapAveragePrice = mapAverage;

		for (auto& it : m_mapAveragePrice)
		{
			sys_log(0, "Average Price Update: Item Vnum: %d Item Price %lld", it.first, it.second);
		}
	}

	long long CShopManager::GetAveragePrice(DWORD vnum)
	{
		auto it = m_mapAveragePrice.find(vnum);
		if (it != m_mapAveragePrice.end())
			return it->second;

		return 0;
	}

	void CShopManager::RecvAveragePrice(LPCHARACTER ch, DWORD vnum)
	{
		if (!ch || !ch->GetDesc())
			return;


		TPacketGCNewOfflineshop pack;
		pack.bHeader = HEADER_GC_NEW_OFFLINESHOP;
		pack.wSize = sizeof(pack) + sizeof(TSubPacketAveragePrice);
		pack.bSubHeader = SUBHEADER_GC_AVERAGE_PRICE;

		TSubPacketAveragePrice subpack{vnum, GetAveragePrice(vnum)};

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(pack));
		buff.write(&subpack, sizeof(subpack));

		ch->GetDesc()->Packet(buff.read_peek(), buff.size());
	}
#endif
}
#endif