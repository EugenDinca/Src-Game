#include "stdafx.h"
#ifdef ENABLE_INVENTORY_REWORK
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "shop.h"
#include "config.h"
#include "questmanager.h"
#include "party.h"
#include "utils.h"

namespace PartyPickupWindowTypeToCh
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item)
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center)
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
		long long	iMoney;
		FMoneyDistributor(LPCHARACTER center, long long iMoney)
#else
		int		iMoney;
		FMoneyDistributor(LPCHARACTER center, int iMoney)
#endif
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch != c)
			{
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);
				}
			}
		}
	};
}

int	CHARACTER::WindowTypeToGetEmpty(BYTE window_type, LPITEM item)
{
	int iEmptyCell = -1;

	switch (window_type)
	{
		case UPGRADE_INVENTORY:
		{
			iEmptyCell = GetEmptyUpgradeInventory(item);
			break;
		}

		case BOOK_INVENTORY:
		{
			iEmptyCell = GetEmptyBookInventory(item);
			break;
		}

		case STONE_INVENTORY:
		{
			iEmptyCell = GetEmptyStoneInventory(item);
			break;
		}

		case CHEST_INVENTORY:
		{
			iEmptyCell = GetEmptyChestInventory(item);
			break;
		}

		case INVENTORY:
		{
			iEmptyCell = GetEmptyInventory(item->GetSize());
			break;
		}
		case DRAGON_SOUL_INVENTORY:
		{
			iEmptyCell = GetEmptyDragonSoulInventory(item);
			break;
		}
		default:
			break;
	}
	return iEmptyCell;
}

LPITEM CHARACTER::WindowTypeGetItem(WORD wCell, BYTE window_type) const
{
	return GetItem(TItemPos(window_type, wCell));
}

BYTE CHARACTER::VnumGetWindowType(DWORD vnum) const
{
	TItemTable* item = ITEM_MANAGER::instance().GetTable(vnum);
	if (!item)//Furkan'ýn proto sikmesi durumunda tecavüze karþý önlem
		return 0;
	BYTE window_type = 0;
	BYTE item_type = item->bType;
	switch (item_type)
	{
		case ITEM_SKILLBOOK:
		{
			window_type = BOOK_INVENTORY;
			break;
		}
		case ITEM_METIN:
		{
			window_type = STONE_INVENTORY;
			break;
		}
		case ITEM_GIFTBOX:
		{
			window_type = CHEST_INVENTORY;
			if (NotChestWindowItem(vnum))
				window_type = INVENTORY;
			break;
		}
		case ITEM_DS:
		{
			window_type = DRAGON_SOUL_INVENTORY;
			break;
		}
		case ITEM_MATERIAL:
		{
			if (item->bSubType == MATERIAL_LEATHER)
			{
				window_type = UPGRADE_INVENTORY;
				break;
			}	
		}
		default:
		{
			switch (vnum)
			{
				//UPGRADE_INVENTORY BEGIN
				case 27992:
				case 27993:
				case 27994:
				{
					window_type = UPGRADE_INVENTORY;
					break;
				}
				//UPGRADE_INVENTORY END

				//CHEST_INVENTORY BEGIN
				case 30300:
				case 50287:
				case 50288:
				case 50289:
				case 50290:
				case 50926:
				case 50927:
#ifdef ENABLE_GREEN_ATTRIBUTE_CHANGER
				case 71151:
#endif
				case 71084:
				case 24102:
				case 24103:
				case 24104:
				case 24105:
				case 72351:
				case 72346:
				{
					window_type = CHEST_INVENTORY;
					break;
				}
				//CHEST_INVENTORY END

				default:
				{
					window_type = INVENTORY;
					break;
				}	
			}
			break;
		}
	}
	return window_type;
}

void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count)
{
	if (count == 0)
		return;

	TItemTable* p = ITEM_MANAGER::instance().GetTable(vnum);
	if (!p) { return; }

	LPITEM item;
	BYTE window_type;

	window_type = VnumGetWindowType(vnum);

	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		item = WindowTypeGetItem(i, window_type);
		if (NULL == item)
			continue;

		if (item->GetVnum() != vnum)
			continue;

		if (m_pkMyShop)
		{
			const bool isItemSelling = m_pkMyShop->IsSellingItem(item->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= item->GetCount())
		{
			count -= item->GetCount();
			item->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			item->SetCount(item->GetCount() - count);
			return;
		}
	}

	if (window_type != INVENTORY)
	{
		for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			if (NULL == GetInventoryItem(i))
				continue;

			if (GetInventoryItem(i)->GetVnum() != vnum)
				continue;

			if (m_pkMyShop)
			{
				bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
				if (isItemSelling)
					continue;
			}

			if (count >= GetInventoryItem(i)->GetCount())
			{
				count -= GetInventoryItem(i)->GetCount();
				GetInventoryItem(i)->SetCount(0);

				if (0 == count)
					return;
			}
			else
			{
				GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
				return;
			}
		}
	}
}

int CHARACTER::CountSpecifyItem(DWORD vnum) const
{
	TItemTable* p = ITEM_MANAGER::instance().GetTable(vnum);
	if (!p) { return 0; }
	int	count = 0;
	LPITEM item;
	BYTE window_type;
	window_type = VnumGetWindowType(vnum);

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		item = WindowTypeGetItem(i, window_type);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

	if (window_type != INVENTORY)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			item = GetInventoryItem(i);
			if (NULL != item && item->GetVnum() == vnum)
			{
				if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
				{
					continue;
				}
				else
				{
					count += item->GetCount();
				}
			}
		}
	}
	return count;
}

bool CHARACTER::CountSpecifyItemText(DWORD vnum, int reqCount)
{
	if (CountSpecifyItem(vnum) < reqCount)
	{
		TItemTable* item = ITEM_MANAGER::instance().GetTable(vnum);
		if (!item)//Furkan'ýn proto sikmesi durumunda tecavuze karsi onlem
			return false;

		NewChatPacket(STRING_D184, "%s", item->szLocaleName);
		return false;
	}
	return true;
}



LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT bCount,
#else
	BYTE bCount,
#endif
	int iRarePct, bool bMsg)
{
	TItemTable* p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	BYTE window_type;
	DWORD ChatCount = bCount;
	window_type = VnumGetWindowType(dwItemVnum);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = WindowTypeGetItem(i, window_type);
			if (!item)
				continue;
			if (item->GetVnum() == dwItemVnum)
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#endif
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ItemWinnerChat(ChatCount, window_type, item->GetName());
					return item;
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
			if (inv_item->GetType() == ITEM_BLEND && !inv_item->IsExtendedBlend(inv_item->GetVnum()))
#else
			if (inv_item->GetType() == ITEM_BLEND)
#endif
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
						inv_item->GetSocket(1) == item->GetSocket(1) &&
						inv_item->GetSocket(2) == item->GetSocket(2) &&
						inv_item->GetCount() < g_bItemCountLimit)
					{
#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
						inv_item->SetSocket(2, inv_item->GetSocket(2) + item->GetSocket(2));
#else
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
#endif
						M2_DESTROY_ITEM(item);//@Lightwork#87
						return inv_item;
					}
				}
			}
		}
	}

	int iEmptyCell;
	iEmptyCell = WindowTypeToGetEmpty(window_type, item);

	if (iEmptyCell != -1)
	{
		item->AddToCharacter(this, TItemPos(window_type, iEmptyCell));
		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
		if (bMsg)
			ItemWinnerChat(ChatCount, window_type, item->GetName());
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
	}
	return item;
}

void CHARACTER::ItemWinnerChat(DWORD count, BYTE window_type, const char* itemname)
{
	switch (window_type)
	{
		case UPGRADE_INVENTORY:
		{
			NewChatPacket(WINNER_ITEM_01, "%d|%s", count, itemname);
			break;
		}

		case BOOK_INVENTORY:
		{
			NewChatPacket(WINNER_ITEM_02, "%d|%s", count, itemname);
			break;
		}

		case STONE_INVENTORY:
		{
			NewChatPacket(WINNER_ITEM_03, "%d|%s", count, itemname);
			break;
		}

		case CHEST_INVENTORY:
		{
			NewChatPacket(WINNER_ITEM_04, "%d|%s", count, itemname);
			break;
		}

		case INVENTORY:
		{
			NewChatPacket(WINNER_ITEM_05, "%d|%s", count, itemname);
			break;
		}

		case DRAGON_SOUL_INVENTORY:
		{
			NewChatPacket(WINNER_ITEM_06, "%d|%s", count, itemname);
			break;
		}
		default:
			break;
	}
}

#ifdef ENABLE_DROP_ITEM_TO_INVENTORY
bool CHARACTER::DropItemToInventory(LPITEM item)
{
	LPITEM weaponwear = GetWear(WEAR_WEAPON);
	if (weaponwear && weaponwear->GetSubType() == WEAPON_BOW)
		return false;

	DWORD ChatCount = item->GetCount();
	DWORD dwItemVnum = item->GetVnum();
	BYTE window_type = VnumGetWindowType(dwItemVnum);
	TItemTable const* p = item->m_pProto;
	MAX_COUNT bCount = item->GetCount();

	if (p->dwFlags & ITEM_FLAG_STACKABLE && item->GetType() != ITEM_BLEND)
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM items = WindowTypeGetItem(i, window_type);
			if (!items)
				continue;

			if (items->GetVnum() == dwItemVnum)
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - items->GetCount(), bCount);
				bCount -= bCount2;

				items->SetCount(items->GetCount() + bCount2);

				if (bCount == 0)
				{
#ifdef ENABLE_EXTENDED_BATTLE_PASS
					UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, ChatCount, dwItemVnum);
#endif
					ItemWinnerChat(ChatCount, window_type, items->GetName());
					M2_DESTROY_ITEM(item);
					return true;
				}
			}
		}

		if (ChatCount != bCount)
			item->SetCount(bCount);
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
						inv_item->GetSocket(1) == item->GetSocket(1) &&
						inv_item->GetSocket(2) == item->GetSocket(2) &&
						(inv_item->GetCount() + item->GetCount()) < g_bItemCountLimit)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						M2_DESTROY_ITEM(item);
						return true;
					}
				}
			}
		}
	}

	int iEmptyCell;
	iEmptyCell = WindowTypeToGetEmpty(window_type, item);

	if (iEmptyCell != -1)
	{
		item->AddToCharacter(this, TItemPos(window_type, iEmptyCell));
		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, ChatCount, dwItemVnum);
#endif
		ItemWinnerChat(ChatCount, window_type, item->GetName());
	}
	else
	{
		return false;
	}
	return true;
}
#endif

/**************************************************************************/
/********************DEFAULT CODE*********************************/
/**************************************************************************/
/*
void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}

#ifdef ENABLE_SPECIAL_STORAGE
	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetUpgradeInventoryItem(i))
			continue;

		if (GetUpgradeInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetUpgradeInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetUpgradeInventoryItem(i)->GetCount())
		{
			count -= GetUpgradeInventoryItem(i)->GetCount();
			GetUpgradeInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetUpgradeInventoryItem(i)->SetCount(GetUpgradeInventoryItem(i)->GetCount() - count);
			return;
		}
	}

	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetBookInventoryItem(i))
			continue;

		if (GetBookInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetBookInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetBookInventoryItem(i)->GetCount())
		{
			count -= GetBookInventoryItem(i)->GetCount();
			GetBookInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetBookInventoryItem(i)->SetCount(GetBookInventoryItem(i)->GetCount() - count);
			return;
		}
	}

	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetStoneInventoryItem(i))
			continue;

		if (GetStoneInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetStoneInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetStoneInventoryItem(i)->GetCount())
		{
			count -= GetStoneInventoryItem(i)->GetCount();
			GetStoneInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetStoneInventoryItem(i)->SetCount(GetStoneInventoryItem(i)->GetCount() - count);
			return;
		}
	}

	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetChestInventoryItem(i))
			continue;

		if (GetChestInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetChestInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetChestInventoryItem(i)->GetCount())
		{
			count -= GetChestInventoryItem(i)->GetCount();
			GetChestInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetChestInventoryItem(i)->SetCount(GetChestInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#endif
}

int CHARACTER::CountSpecifyItem(DWORD vnum) const
{
	int	count = 0;
	LPITEM item;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

#ifdef ENABLE_SPECIAL_STORAGE
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		item = GetUpgradeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		item = GetBookInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		item = GetStoneInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		item = GetChestInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}
#endif

	return count;
}

LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT bCount,
#else
		BYTE bCount,
#endif
		int iRarePct, bool bMsg
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	, bool inv_update, bool open_stuff
#endif
)
{
	TItemTable* p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#endif
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2
#ifdef ENABLE_CHEST_OPEN_RENEWAL
					, inv_update
#endif
				);

				if (bCount == 0)
				{
					if (bMsg)
						NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());

					return item;
				}
			}
		}
#ifdef ENABLE_SPECIAL_STORAGE
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetUpgradeInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#endif

				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2
#ifdef ENABLE_CHEST_OPEN_RENEWAL
					, inv_update
#endif
				);

				if (bCount == 0)
				{
					if (bMsg)
						NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());

					return item;
				}
			}
		}

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetBookInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#endif

				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2
#ifdef ENABLE_CHEST_OPEN_RENEWAL
					, inv_update
#endif
				);

				if (bCount == 0)
				{
					if (bMsg)
						NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());

					return item;
				}
			}
		}

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetStoneInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#endif

				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2
#ifdef ENABLE_CHEST_OPEN_RENEWAL
					, inv_update
#endif
				);

				if (bCount == 0)
				{
					if (bMsg)
						NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());

					return item;
				}
			}
		}

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetChestInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
				MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
#endif

				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2
#ifdef ENABLE_CHEST_OPEN_RENEWAL
					, inv_update
#endif
				);

				if (bCount == 0)
				{
					if (bMsg)
						NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());

					return item;
				}
			}
		}
#endif
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
						inv_item->GetSocket(1) == item->GetSocket(1) &&
						inv_item->GetSocket(2) == item->GetSocket(2) &&
						inv_item->GetCount() < g_bItemCountLimit)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						M2_DESTROY_ITEM(item);//@Lightwork#87
						return inv_item;
					}
				}
			}
		}
	}

	int iEmptyCell;
	if (item->IsDragonSoul())
	{
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
		iEmptyCell = GetEmptyUpgradeInventory(item);
	else if (item->IsBook())
		iEmptyCell = GetEmptyBookInventory(item);
	else if (item->IsStone())
		iEmptyCell = GetEmptyStoneInventory(item);
	else if (item->IsChest())
		iEmptyCell = GetEmptyChestInventory(item);
#endif
	else
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg)
			NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());

		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell)
#ifdef ENABLE_CHEST_OPEN_RENEWAL
				, inv_update
#endif
			);
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, iEmptyCell)
#ifdef ENABLE_CHEST_OPEN_RENEWAL
				, inv_update
#endif
			);
		else if (item->IsBook())
			item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, iEmptyCell)
#ifdef ENABLE_CHEST_OPEN_RENEWAL
				, inv_update
#endif
			);
		else if (item->IsStone())
			item->AddToCharacter(this, TItemPos(STONE_INVENTORY, iEmptyCell)
#ifdef ENABLE_CHEST_OPEN_RENEWAL
				, inv_update
#endif
			);
		else if (item->IsChest())
			item->AddToCharacter(this, TItemPos(CHEST_INVENTORY, iEmptyCell)
#ifdef ENABLE_CHEST_OPEN_RENEWAL
				, inv_update
#endif
			);
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell)
#ifdef ENABLE_CHEST_OPEN_RENEWAL
				, inv_update
#endif
			);

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		if (open_stuff) {
			M2_DESTROY_ITEM(item);
			return NULL;
		}
#endif
		item->AddToGround(GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif

		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
	}
	return item;
}

*/
#endif