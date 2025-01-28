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

#ifdef ENABLE_OFFLINE_SHOP
namespace offlineshop
{
	/*
	strcture
	-	vector of items
	-	amount of valute
	-	owner character

	metodhs
	- 	constructor which set the character owner

	-	setitems to set items when created object
	-	setvalute to set the valute when created object

	-	additem to add an item in the stock
	-	removeitem to remove item in the stock

	-	addvalute to add amount to the valute counts
	-	removevalute to remove amount to the valute counts (return false if val>amount)

	-	getitem to easly find item by id
	-	getvalutes to get the amounts
	-	getitems to get a pointer to the item vectors

	-	getowner to get the character owner
	*/

	CShopSafebox::CShopSafebox(LPCHARACTER chOwner)
	{
		m_safebox_gold = 0;
		m_pkOwner = chOwner;
	}

	CShopSafebox::CShopSafebox()
	{
		m_safebox_gold = 0;
		m_pkOwner = nullptr;
	}

	CShopSafebox::CShopSafebox(const CShopSafebox& rCopy)
	{

		CopyObject(m_safebox_gold, rCopy.m_safebox_gold);
		CopyContainer(m_vecItems, rCopy.m_vecItems);
		m_pkOwner = rCopy.m_pkOwner;
	}

	CShopSafebox::~CShopSafebox()
	{
	}

	void CShopSafebox::SetOwner(LPCHARACTER ch)
	{
		m_pkOwner = ch;
	}

	void CShopSafebox::SetItems(VECITEM* pVec)
	{
		CopyContainer(m_vecItems, *pVec);
	}

	void CShopSafebox::SetValuteAmount(unsigned long long val)
	{
		CopyObject(m_safebox_gold, val);
	}

	bool CShopSafebox::AddItem(CShopItem* pItem)
	{
		CShopItem* pSearch = NULL;
		if (GetItem(pItem->GetID(), &pSearch))
		{
			return false;
		}
		m_vecItems.push_back(CShopItem(*pItem));
		return true;
	}

	bool CShopSafebox::RemoveItem(DWORD dwItemID)
	{
		for (VECITEM::iterator it = m_vecItems.begin();
			it != m_vecItems.end();
			it++)
		{
			if (dwItemID == it->GetID())
			{
				m_vecItems.erase(it);
				return true;
			}
		}
		return false;
	}

	bool CShopSafebox::RemoveItemControl(DWORD dwItemID)
	{
		for (VECITEM::iterator it = m_vecItems.begin();
			it != m_vecItems.end();
			it++)
		{
			if (dwItemID == it->GetID())
			{
				return true;
			}
		}
		return false;
	}

	void CShopSafebox::AddValute(unsigned long long val)
	{
		m_safebox_gold += val;
	}

	bool CShopSafebox::RemoveValute(unsigned long long val)
	{
		if (m_safebox_gold < val)
		{
			return false;
		}
		m_safebox_gold -= val;
		return true;
	}

	CShopSafebox::VECITEM* CShopSafebox::GetItems()
	{
		return &m_vecItems;
	}

	unsigned long long CShopSafebox::GetValutes()
	{
		return m_safebox_gold;
	}

	bool CShopSafebox::GetItem(DWORD dwItemID, CShopItem** ppItem)
	{
		for (VECITEM::iterator it = m_vecItems.begin();
			it != m_vecItems.end();
			it++)
		{
			if (dwItemID == it->GetID())
			{
				*ppItem = &(*it);
				return true;
			}
		}
		return false;
	}

	LPCHARACTER CShopSafebox::GetOwner()
	{
		return m_pkOwner;
	}

	bool CShopSafebox::RefreshToOwner(LPCHARACTER ch)
	{
		if (!ch && !m_pkOwner)
		{
			return false;
		}

		if (!ch)
		{
			ch = m_pkOwner;
		}

		GetManager().SendShopSafeboxRefresh(ch, m_safebox_gold, m_vecItems);
		return true;
	}
}
#endif