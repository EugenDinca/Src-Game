#include "stdafx.h"
#include "../../common/tables.h"
#include "packet.h"
#include "item.h"
#include "char.h"
#include "item_manager.h"
#include "desc.h"
#include "char_manager.h"
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
#include "db.h"
#include "desc_client.h"

#ifdef ENABLE_OFFLINE_SHOP
namespace offlineshop
{
	/*
		structure
		-	contain item informations (offlineshop::SShopItemInfo)
		-	contain price (prezzo)
		-	contain window type (shop/shop safebox)

		methods
		-	constructor using LPITEM, price, window (immediatly set)
		-	constructor default (all to zero, alloc id)
		-	copy constructor (using in vector)

		-	gettable to get prototype table directly (very useful in filtering)
		-	getprice to get the sell price
		-	getinfo to get the item informations

		-	setprice to set sell price
		-	setinfo using LPITEM to get immediatly information from item
		-	setwindow to set the current window (shop/ shop safebox)
		-	getwindow to get the current window ||
	*/

	CShopItem::CShopItem(DWORD dwID)
	{
		m_dwID = dwID;
		m_byWindow = 0;
		ZeroObject(m_itemInfo);
		ZeroObject(m_priceInfo);
	}

	CShopItem::CShopItem(const CShopItem& rCopy)
	{
		m_dwID = rCopy.GetID();
		m_byWindow = rCopy.GetWindow();
		CopyObject(m_itemInfo, *rCopy.GetInfo());
		CopyObject(m_priceInfo, *rCopy.GetPrice());
	}

	CShopItem::CShopItem(LPITEM pItem, const TPriceInfo& sPrice, BYTE bWindowType, DWORD dwID) : m_dwID(0)
	{
		SetWindow(bWindowType);
		CopyObject(m_priceInfo, sPrice);
		if (pItem)
		{
			m_itemInfo.dwCount = pItem->GetCount();
			m_dwOwnerID = pItem->GetOwner() ? pItem->GetOwner()->GetPlayerID() : 0;
			m_itemInfo.dwVnum = pItem->GetVnum();
			m_itemInfo.expiration = GetItemExpiration(pItem);
			const TPlayerItemAttribute* pAttributes = pItem->GetAttributes();
			memcpy(m_itemInfo.aAttr, pAttributes, sizeof(m_itemInfo.aAttr));
			const long* pSockets = pItem->GetSockets();
			memcpy(m_itemInfo.alSockets, pSockets, sizeof(m_itemInfo.alSockets));
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
			m_itemInfo.dwTransmutation = pItem->GetTransmutation();
#endif
		}
		else
		{
			sys_err("offlineshop::CShopItem - constructor using item/price/window : item == nullptr! ");
		}
		if (dwID != 0)
		{
			m_dwID = dwID;
		}
	}

	CShopItem::~CShopItem()
	{
	}

	bool CShopItem::GetTable(TItemTable** ppTable) const
	{
		if ((*ppTable = ITEM_MANAGER::instance().GetTable(m_itemInfo.dwVnum)))
			return true;
		return false;
	}

	TPriceInfo* CShopItem::GetPrice() const
	{
		return (TPriceInfo*)&m_priceInfo;
	}

	LPITEM CShopItem::CreateItem() const
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(m_itemInfo.dwVnum, m_itemInfo.dwCount);
		if (!item)
		{
			return item;
		}
		item->SetAttributes(m_itemInfo.aAttr);
		item->SetSockets(m_itemInfo.alSockets);
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
		item->SetTransmutation(m_itemInfo.dwTransmutation);
#endif
		return item;
	}

	TItemInfoEx* CShopItem::GetInfo() const
	{
		return (TItemInfoEx*)&m_itemInfo;
	}

	void CShopItem::SetInfo(LPITEM pItem)
	{
		if (pItem)
		{
			m_itemInfo.dwCount = pItem->GetCount();
			m_dwOwnerID = pItem->GetOwner() ? pItem->GetOwner()->GetPlayerID() : 0;
			m_itemInfo.dwVnum = pItem->GetVnum();
			const TPlayerItemAttribute* pAttributes = pItem->GetAttributes();
			memcpy(m_itemInfo.aAttr, pAttributes, sizeof(m_itemInfo.aAttr));
			const long* pSockets = pItem->GetSockets();
			memcpy(m_itemInfo.alSockets, pSockets, sizeof(m_itemInfo.alSockets));
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
			m_itemInfo.dwTransmutation = pItem->GetTransmutation();
#endif
		}
		else
		{
			sys_err("offlineshop::CShopItem - SetInfo: item == nullptr! ");
		}
	}

	void CShopItem::SetInfo(const TItemInfoEx& info)
	{
		CopyObject(m_itemInfo, info);
	}

	void CShopItem::SetPrice(const TPriceInfo& sPrice)
	{
		CopyObject(m_priceInfo, sPrice);
	}

	void CShopItem::SetWindow(BYTE byWin)
	{
		m_byWindow = byWin;
	}

	BYTE CShopItem::GetWindow() const
	{
		return m_byWindow;
	}

	DWORD CShopItem::GetID() const
	{
		return m_dwID;
	}

	void CShopItem::SetOwnerID(DWORD dwOwnerID)
	{
		m_dwOwnerID = dwOwnerID;
	}

	bool CShopItem::CanBuy(LPCHARACTER ch)
	{
		if (!ch)
		{
			return false;
		}

		if (m_priceInfo.illYang > (long long)ch->GetGold())
		{
			return false;
		}

		return true;
	}

	void CShopItem::operator=(const CShopItem& rItem)
	{
		m_dwID = rItem.GetID();
		m_byWindow = rItem.GetWindow();
		CopyObject(m_itemInfo, *rItem.GetInfo());
		CopyObject(m_priceInfo, *rItem.GetPrice());
	}

	/*
	CShop

	structure
	-	item pointers vector
	-	owner's player id
	-	duration
	-	guests list
	-	shop virtual id

	metodhs
	-	getguests to get a pointer to the guests list

	-	getduration to know duration of the shop
	-	setduration used to set the initi duration a the boot
	-	decreaseduration to get and decrease duration sametime

	-	setownerpid to set the owner's pid
	-	getownerpid to get the owner's pid

	-	addguest to add a new guest to the shop
	-	removeguest to remove a guest to the shop (close board or logout)

	-	setitems used to set initial item on boot
	-	modifyitem to modify an item and refresh to guest
	-	buyitem guest , used to buy an item when you are looking the shop
	-	buyitem character, used to buy item when you are looking a filtered search
	-	removeitem to remove an item and send refresh to uests
	-	additem to add an item to the shop and send refresh to guests
	-	clear to delete the item pointer in vector item and remove all element into containers
	-	getitem to find item by virtualid
	-	findowner to use find by pid (char manager) to search owner id
	*/

	CShop::CShop()
	{
		m_dwPID = 0;
		m_dwDuration = 0;
		m_stName.clear();
#ifdef ENABLE_SHOP_DECORATION
		m_Race = 0;
#endif
	}

	CShop::CShop(const CShop& rCopy)
	{
		CopyContainer(m_listGuests, *rCopy.GetGuests());
		CopyContainer(m_vecItems, *rCopy.GetItems());
		m_dwPID = rCopy.GetOwnerPID();
		m_dwDuration = rCopy.GetDuration();
		m_stName = rCopy.GetName();
#ifdef ENABLE_SHOP_DECORATION
		m_Race = rCopy.GetRace();
#endif
	}

	CShop::~CShop()
	{
	}

	CShop::VECSHOPITEM* CShop::GetItems() const
	{
		return (CShop::VECSHOPITEM*)&m_vecItems;
	}

	CShop::LISTGUEST* CShop::GetGuests() const
	{
		return (CShop::LISTGUEST*)&m_listGuests;
	}

	void CShop::SetDuration(DWORD dwDuration)
	{
		m_dwDuration = dwDuration;
	}

	DWORD CShop::DecreaseDuration()
	{
		return --m_dwDuration;
	}

	DWORD CShop::GetDuration() const
	{
		return m_dwDuration;
	}

	void CShop::SetOwnerPID(DWORD dwOwnerPID)
	{
		m_dwPID = dwOwnerPID;
	}

	DWORD CShop::GetOwnerPID() const
	{
		return m_dwPID;
	}

	bool CShop::AddGuest(LPCHARACTER ch)
	{
		for (LISTGUEST::iterator it = m_listGuests.begin(); it != m_listGuests.end(); it++)
		{
			if (*it == GUEST_FROM_CHARACTER(ch))
			{
				return false;
			}
		}
		m_listGuests.push_back(GUEST_FROM_CHARACTER(ch));
		return true;
	}

	bool CShop::RemoveGuest(LPCHARACTER ch)
	{
		for (LISTGUEST::iterator it = m_listGuests.begin(); it != m_listGuests.end(); it++)
		{
			if (*it == GUEST_FROM_CHARACTER(ch))
			{
				m_listGuests.erase(it);
				return true;
			}
		}
		return false;
	}

	void CShop::SetItems(VECSHOPITEM* pVec)
	{
		CopyContainer(m_vecItems, *pVec);
		for (DWORD i = 0; i < m_vecItems.size(); i++)
		{
			m_vecItems[i].SetWindow(NEW_OFFSHOP);
		}
		if (!m_listGuests.empty())
		{
			__RefreshItems();
		}
	}

	bool CShop::AddItem(CShopItem& rItem)
	{
		rItem.SetWindow(NEW_OFFSHOP);
		m_vecItems.push_back(rItem);
		__RefreshItems();
		return true;
	}

	bool CShop::RemoveItem(DWORD dwItemID)
	{
		for (VECSHOPITEM::iterator it = m_vecItems.begin();
			it != m_vecItems.end();
			it++)
		{
			if (dwItemID == it->GetID())
			{
				m_vecItems.erase(it);
				__RefreshItems();
				return true;
			}
		}
		return false;
	}

	bool CShop::ModifyItem(DWORD dwItemID, CShopItem& rItem)
	{
		if (rItem.GetID() != dwItemID)
		{
			sys_err("have you forgot to set item id ? %d - %d don't match ", rItem.GetID(), dwItemID);
			return false;
		}

		CShopItem* pItem = NULL;
		if (!GetItem(dwItemID, &pItem))
		{
			return false;
		}
		*pItem = rItem;
		pItem->SetWindow(NEW_OFFSHOP);
		__RefreshItems();
		return true;
	}

	bool CShop::BuyItem(DWORD dwItem)
	{
		CShopItem* pItem = NULL;
		if (!GetItem(dwItem, &pItem))
		{
			return false;
		}
		RemoveItem(dwItem);
		return true;
	}

	bool CShop::GetItem(DWORD dwItem, CShopItem** ppItem)
	{
		for (VECSHOPITEM::iterator it = m_vecItems.begin();
			it != m_vecItems.end();
			it++)
		{
			if (dwItem == it->GetID())
			{
				*ppItem = &(*it);
				return true;
			}
		}
		return false;
	}

	void CShop::__RefreshItems(LPCHARACTER ch)
	{
		if (m_listGuests.empty())
		{
			return;
		}
		if (!ch)
		{
			itertype(m_listGuests) it = m_listGuests.begin();
			for (; it != m_listGuests.end();)
			{
				LPCHARACTER chGuest = GUEST_PTR(*it);
				if (!chGuest)
				{
					it = m_listGuests.erase(it);
					continue;
				}

				if (chGuest->GetPlayerID() == m_dwPID)
				{
					GetManager().SendShopOpenMyShopClientPacket(chGuest);
				}
				else
				{
					GetManager().SendShopOpenClientPacket(chGuest, this);
				}

				it++;
			}
		}
		else
		{
			if (ch->GetPlayerID() == m_dwPID)
			{
				GetManager().SendShopOpenMyShopClientPacket(ch);
			}
			else
			{
				GetManager().SendShopOpenClientPacket(ch, this);
			}
		}
	}

	void CShop::Clear()
	{
		m_vecItems.clear();
		m_listGuests.clear();
	}

#ifdef ENABLE_IRA_REWORK
	void CShop::SetPosInfo(TShopPosition& pos)
	{
		CopyObject(m_posInfo, pos);
	}
#endif

	LPCHARACTER CShop::FindOwnerCharacter()
	{
		return CHARACTER_MANAGER::instance().FindByPID(GetOwnerPID());
	}

#ifdef ENABLE_SHOP_DECORATION
	void CShop::SetRace(DWORD dwRace) {
		m_Race = dwRace;
	}
	
	DWORD CShop::GetRace() const {
		return m_Race;
	}
#endif

	const char* CShop::GetName() const
	{
		return m_stName.c_str();
	}

	void CShop::SetName(const char* pcszName)
	{
		m_stName = pcszName;
	}

	void CShop::RefreshToOwner()
	{
		LPCHARACTER ch = FindOwnerCharacter();
		if (!ch)
		{
			return;
		}
		GetManager().SendShopOpenMyShopClientPacket(ch);
	}

	TShopPosition CShop::GetShopPositions()
	{
		if ((m_posInfo.lMapIndex != 41 && m_posInfo.lMapIndex != 21 && m_posInfo.lMapIndex != 1) || m_posInfo.bChannel != 99)
		{
			memset(&m_posInfo, 0, sizeof(m_posInfo));
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT pos_x, pos_y, map_index, channel FROM player.offlineshop_shops WHERE owner_id = %d", m_dwPID));
			if (pMsg->Get()->uiNumRows == 0)
			{
				return {0,0,0,0};
			}

			MYSQL_ROW row;
			if ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				str_to_number(m_posInfo.x, row[0]);
				str_to_number(m_posInfo.y, row[1]);
				str_to_number(m_posInfo.lMapIndex, row[2]);
				str_to_number(m_posInfo.bChannel, row[3]);
			}
			else
			{
				return { 0,0,0,0 };
			}
		}
		return m_posInfo;
	}

#ifdef ENABLE_SHOPS_IN_CITIES
	ShopEntity::ShopEntity()
	{
		m_dwVID = AllocID();
		m_pkShop = nullptr;
		CEntity::Initialize(ENTITY_NEWSHOPS);
	}

	void ShopEntity::EncodeInsertPacket(LPENTITY entity)
	{
		LPCHARACTER ch = entity->GetType() == ENTITY_CHARACTER ? (LPCHARACTER)entity : nullptr;
		if (!ch || !ch->IsPC())
		{
			return;
		}
		GetManager().EncodeInsertShopEntity(*this, ch);
	}

	void ShopEntity::EncodeRemovePacket(LPENTITY entity)
	{
		LPCHARACTER ch = entity->GetType() == ENTITY_CHARACTER ? (LPCHARACTER)entity : nullptr;
		if (!ch || !ch->IsPC())
		{
			return;
		}
		GetManager().EncodeRemoveShopEntity(*this, ch);
	}

	const char* ShopEntity::GetShopName() const
	{
		return m_szName;
	}

	void ShopEntity::SetShopName(const char* name)
	{
		strncpy(m_szName, name, sizeof(m_szName));
	}

#ifdef ENABLE_SHOP_DECORATION
	void ShopEntity::SetShopRace(DWORD dwRace) {
		m_Race = dwRace;
	}
	
	DWORD ShopEntity::GetShopRace() {
		return m_Race;
	}
#endif

	int ShopEntity::GetShopType()
	{
		return m_iType;
	}

	void ShopEntity::SetShopType(int iType)
	{
		m_iType = iType;
	}

	void ShopEntity::SetShop(offlineshop::CShop* pOfflineShop)
	{
		m_pkShop = pOfflineShop;
	}
#endif
}
#endif