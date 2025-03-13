#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"

#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "marriage.h"
#include "polymorph.h"
#include "blend_item.h"
#include "BattleArena.h"
#include "arena.h"
#include "dev_log.h"
#include "safebox.h"
#include "shop.h"
#ifdef ENABLE_NEWSTUFF
#include "pvp.h"
#endif
#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/Controls.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
#include "battlepass_manager.h"
#endif
#ifdef ENABLE_BUFFI_SYSTEM
#include "BuffiSystem.h"
#endif
const int ITEM_BROKEN_METIN_VNUM = 28960;
#define ENABLE_EFFECT_EXTRAPOT
#define ENABLE_BOOKS_STACKFIX
#define ENABLE_ITEM_RARE_ATTR_LEVEL_PCT

// CHANGE_ITEM_ATTRIBUTES
// const DWORD CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
// const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};

//@Lightwork#140_START
static bool BLOCKED_ATTR_LIST(int vnum)
{
	switch (vnum)
	{
	case 50201:
	case 50202:
	case 12000:
	case 12001:
	case 12002:
	case 12003:
	case 11911:
	case 11912:
	case 11913:
	case 11914:
		return true;
	}

	return false;
}
//@Lightwork#140_END

bool IS_SUMMONABLE_ZONE(int map_index)
{

	switch (map_index)
	{
	case 66:
	case 216:
	case 217:
	case 208:
		return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;

	if (map_index > 10000) return false;

	return true;
}

#ifndef ENABLE_INVENTORY_REWORK
static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}
#endif

static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE == GET_SEX(ch))
			return false;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
	{
		if (SEX_FEMALE == GET_SEX(ch))
			return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((m_bAcceCombination) || (m_bAcceAbsorption))
		return false;
#endif
#ifdef ENABLE_AURA_SYSTEM
	if ((m_bAuraRefine) || (m_bAuraAbsorption))
		return false;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	if (Is67AttrOpen())
		return false;
#endif

	return true;
}

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

#ifdef ENABLE_SPECIAL_STORAGE
LPITEM CHARACTER::GetUpgradeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(UPGRADE_INVENTORY, wCell));
}

LPITEM CHARACTER::GetBookInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(BOOK_INVENTORY, wCell));
}

LPITEM CHARACTER::GetStoneInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(STONE_INVENTORY, wCell));
}

LPITEM CHARACTER::GetChestInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(CHEST_INVENTORY, wCell));
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
LPITEM CHARACTER::GetAttrInventoryItem() const
{
	return m_pointsInstant.pAttrItems;
}
#endif

LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return nullptr;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pDSItems[wCell];
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSU item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSUItems[wCell];

	case BOOK_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSB item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSBItems[wCell];

	case STONE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSS item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSSItems[wCell];

	case CHEST_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSC item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSSCItems[wCell];
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid switchbot item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pSwitchbotItems[wCell];
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
		if (wCell >= BUFFI_MAX_SLOT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid buffi item cell %d", wCell);
			return nullptr;
		}
		return m_pointsInstant.pBuffiItems[wCell];
#endif

	default:
		return nullptr;
	}
	return nullptr;
}

void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem
#ifdef ENABLE_HIGHLIGHT_SYSTEM
	, bool isHighLight
#endif
)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}

	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
	{
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
			return;
		}

		LPITEM pOld = m_pointsInstant.pItems[wCell];

		if (pOld)
		{
			if (wCell < INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
						continue;

					m_pointsInstant.bItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.bItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell < INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.bItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.bItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pItems[wCell] = pItem;
	}
	break;

	case DRAGON_SOUL_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pDSItems[wCell];

		if (pOld)
		{
			if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
						continue;

					m_pointsInstant.wDSItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wDSItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
				return;
			}

			if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wDSItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pDSItems[wCell] = pItem;
	}
	break;
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSUItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSUItems[p] && m_pointsInstant.pSSUItems[p] != pOld)
						continue;

					m_pointsInstant.wSSUItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSUItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSU item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSUItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSUItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSUItems[wCell] = pItem;
	}
	break;

	case BOOK_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSBItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSBItems[p] && m_pointsInstant.pSSBItems[p] != pOld)
						continue;

					m_pointsInstant.wSSBItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSBItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSBItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSBItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSBItems[wCell] = pItem;
	}
	break;

	case STONE_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSSItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSSItems[p] && m_pointsInstant.pSSSItems[p] != pOld)
						continue;

					m_pointsInstant.wSSSItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSSItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSS item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSSItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSSItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSSItems[wCell] = pItem;
	}
	break;

	case CHEST_INVENTORY:
	{
		LPITEM pOld = m_pointsInstant.pSSCItems[wCell];
		if (pOld)
		{
			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pOld->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					if (m_pointsInstant.pSSCItems[p] && m_pointsInstant.pSSCItems[p] != pOld)
						continue;

					m_pointsInstant.wSSCItemGrid[p] = 0;
				}
			}
			else
				m_pointsInstant.wSSCItemGrid[wCell] = 0;
		}

		if (pItem)
		{
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			{
				sys_err("CHARACTER::SetItem: invalid SSC item cell %d", wCell);
				return;
			}

			if (wCell < SPECIAL_INVENTORY_MAX_NUM)
			{
				for (int i = 0; i < pItem->GetSize(); ++i)
				{
					int p = wCell + (i * 5);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						continue;

					m_pointsInstant.wSSCItemGrid[p] = wCell + 1;
				}
			}
			else
				m_pointsInstant.wSSCItemGrid[wCell] = wCell + 1;
		}

		m_pointsInstant.pSSCItems[wCell] = pItem;
	}
	break;
#endif

#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
	{
		LPITEM pOld = m_pointsInstant.pSwitchbotItems[wCell];
		if (pItem && pOld)
		{
			return;
		}

		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::SetItem: invalid switchbot item cell %d", wCell);
			return;
		}

		if (pItem)
		{
			CSwitchbotManager::Instance().RegisterItem(GetPlayerID(), pItem->GetID(), wCell);
		}
		else
		{
			CSwitchbotManager::Instance().UnregisterItem(GetPlayerID(), wCell);
		}

		m_pointsInstant.pSwitchbotItems[wCell] = pItem;
	}
	break;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	case ATTR_INVENTORY:
	{
		if (wCell >= 1)
		{
			sys_err("CHARACTER::SetItem: invalid ATTR_INVENTORY item cell %d", wCell);
			return;
		}

		m_pointsInstant.pAttrItems = pItem;
	}
	break;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
	{
		if (wCell >= BUFFI_MAX_SLOT) 
		{
			sys_err("CHARACTER::SetItem: invalid BUFFI_MAX_SLOT item cell %d", wCell);
			return;
		}

		LPITEM pOld = m_pointsInstant.pBuffiItems[wCell];

		if (pOld && pItem) 
		{
			return;
		}

		m_pointsInstant.pBuffiItems[wCell] = pItem;
		if (GetBuffiSystem())
			GetBuffiSystem()->UpdateBuffiItem();
	}
	break;
#endif

	default:
		sys_err("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;
			pack.count = pItem->GetCount();
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags = pItem->GetAntiFlag();
#ifdef ENABLE_HIGHLIGHT_SYSTEM
			if (isHighLight)
				pack.highlight = true;
			else
				pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#else
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#endif

			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			pack.count = 0;
			pack.vnum = 0;
			memset(pack.alSockets, 0, sizeof(pack.alSockets));
			memset(pack.aAttr, 0, sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
#ifdef ENABLE_SPECIAL_STORAGE
		case UPGRADE_INVENTORY:
			pItem->SetWindow(UPGRADE_INVENTORY);
			break;
		case BOOK_INVENTORY:
			pItem->SetWindow(BOOK_INVENTORY);
			break;
		case STONE_INVENTORY:
			pItem->SetWindow(STONE_INVENTORY);
			break;
		case CHEST_INVENTORY:
			pItem->SetWindow(CHEST_INVENTORY);
			break;
#endif
#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			pItem->SetWindow(SWITCHBOT);
			break;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		case ATTR_INVENTORY:
			pItem->SetWindow(ATTR_INVENTORY);
			break;
#endif
#ifdef ENABLE_BUFFI_SYSTEM
		case BUFFI_INVENTORY:
			pItem->SetWindow(BUFFI_INVENTORY);
			break;
#endif
		}
	}
}

LPITEM CHARACTER::GetWear(CELL_UINT bCell) const
{
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(CELL_UINT bCell, LPITEM item)
{
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

#ifdef ENABLE_HIGHLIGHT_SYSTEM
	SetItem(TItemPos(INVENTORY, INVENTORY_MAX_NUM + bCell), item, false);
#else
	SetItem(TItemPos(INVENTORY, INVENTORY_MAX_NUM + bCell), item);
#endif

	// if (!item && bCell == WEAR_WEAPON)
	// {
		// if (IsAffectFlag(AFF_GWIGUM))
			// RemoveAffect(SKILL_GWIGEOM);

		// if (IsAffectFlag(AFF_GEOMGYEONG))
			// RemoveAffect(SKILL_GEOMKYUNG);
	// }
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, UINT16_MAX);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(UPGRADE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(BOOK_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(STONE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(CHEST_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif
#ifdef ENABLE_SWITCHBOT
	for (i = 0; i < SWITCHBOT_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(SWITCHBOT, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	for (i = 0; i < BUFFI_MAX_SLOT; ++i)
	{
		if ((item = GetItem(TItemPos(BUFFI_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
	item = GetAttrInventoryItem();
	if (item)
	{
		item->SetSkipSave(true);
		ITEM_MANAGER::Instance().FlushDelayedSave(item);

		item->RemoveFromCharacter();
		M2_DESTROY_ITEM(item);
	}
#endif
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	switch (Cell.window_type)
	{
	case INVENTORY:
	{
		WORD bCell = Cell.cell;

		++iExceptionCell;

		if (Cell.IsBeltInventoryPosition())
		{
			LPITEM beltItem = GetWear(WEAR_BELT);

			if (NULL == beltItem)
				return false;

			if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
				return false;

			if (m_pointsInstant.bItemGrid[bCell])
			{
				if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
					return true;

				return false;
			}

			if (bSize == 1)
				return true;
		}
		else if (bCell >= INVENTORY_MAX_NUM)
			return false;

		if (m_pointsInstant.bItemGrid[bCell])
		{
			if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= INVENTORY_MAX_NUM)
						return false;

					if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

			do
			{
				CELL_UINT p = bCell + (5 * j);

				if (p >= INVENTORY_MAX_NUM)
					return false;

				if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;
	case DRAGON_SOUL_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wDSItemGrid[wCell])
		{
			if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;

				do
				{
					int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.wDSItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;

			do
			{
				int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

				if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSUItemGrid[wCell])
		{
			if (m_pointsInstant.wSSUItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					CELL_UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSUItemGrid[p])
						if (m_pointsInstant.wSSUItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSUItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

	case BOOK_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSBItemGrid[wCell])
		{
			if (m_pointsInstant.wSSBItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSBItemGrid[p])
						if (m_pointsInstant.wSSBItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSBItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

	case STONE_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSSItemGrid[wCell])
		{
			if (m_pointsInstant.wSSSItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSSItemGrid[p])
						if (m_pointsInstant.wSSSItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSSItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
	break;

	case CHEST_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			return false;

		iExceptionCell++;

		if (m_pointsInstant.wSSCItemGrid[wCell])
		{
			if (m_pointsInstant.wSSCItemGrid[wCell] == iExceptionCell)
			{
				if (bSize == 1)
					return true;

				int j = 1;
				BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

				do
				{
					UINT p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.wSSCItemGrid[p])
						if (m_pointsInstant.wSSCItemGrid[p] != iExceptionCell)
							return false;
				} while (++j < bSize);

				return true;
			}
			else
				return false;
		}

		if (1 == bSize)
			return true;
		else
		{
			int j = 1;
			BYTE bPage = wCell / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT);

			do
			{
				UINT p = wCell + (5 * j);

				if (p >= SPECIAL_INVENTORY_MAX_NUM)
					return false;

				if (p / (SPECIAL_INVENTORY_MAX_NUM / SPECIAL_INVENTORY_PAGE_COUNT) != bPage)
					return false;

				if (m_pointsInstant.bItemGrid[p])
					if (m_pointsInstant.wSSCItemGrid[p] != iExceptionCell)
						return false;
			} while (++j < bSize);

			return true;
		}
	}
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
	{
		uint16_t wCell = Cell.cell;
		if (wCell >= SWITCHBOT_SLOT_COUNT) {
			return false;
		}

		if (m_pointsInstant.pSwitchbotItems[wCell]){
			return false;
		}
		return true;
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
	{
		WORD wCell = Cell.cell;
		if (wCell >= BUFFI_MAX_SLOT)
		{
			return false;
		}

		if (m_pointsInstant.pBuffiItems[wCell])
		{
			return false;
		}

		return true;
	}
#endif

	}
	return false;
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(INVENTORY, i), size))
			return i;
	return -1;
}

#ifdef ENABLE_SPECIAL_STORAGE
int CHARACTER::GetEmptyUpgradeInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsUpgradeItem())
		return -1;

	BYTE bSize = pItem->GetSize();

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyBookInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsBook())
		return -1;

	BYTE bSize = pItem->GetSize();

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyStoneInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsStone())
		return -1;

	BYTE bSize = pItem->GetSize();

	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), bSize))
			return i;

	return -1;
}

int CHARACTER::GetEmptyChestInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsChest())
		return -1;
	BYTE bSize = pItem->GetSize();
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), bSize))
			return i;
	return -1;
}

#ifdef ENABLE_CHEST_OPEN_RENEWAL
int CHARACTER::IsEmptyUpgradeInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), freeSize))
			return i;

	return -1;
}

int CHARACTER::IsEmptyBookInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), freeSize))
			return i;

	return -1;
}

int CHARACTER::IsEmptyStoneInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), freeSize))
			return i;

	return -1;
}

int CHARACTER::IsEmptyChestInventory(BYTE freeSize) const
{
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), freeSize))
			return i;
	return -1;
}
#endif

#endif

#ifdef ENABLE_SPLIT_ITEMS_FAST
int CHARACTER::GetEmptyInventoryFromIndex(WORD index, BYTE itemSize, BYTE windowtype) const //SPLIT ITEMS
{
	if (index > INVENTORY_MAX_NUM)
		return -1;

	for (WORD i = index; i < INVENTORY_MAX_NUM; ++i)
	{
		if (IsEmptyItemGrid(TItemPos(windowtype, i), itemSize))
			return i;
	}
	return -1;
}
#endif

int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (INVENTORY_MAX_NUM - count);
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}
	}

	pkOldItem->CopyAttributeTo(pkNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if (ch != NULL)
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	const TRefineTable* prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	DWORD result_vnum = item->GetRefinedVnum();

	// REFINE_COST
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t cost = ComputeRefineFee(prt->cost);
#else
	int cost = ComputeRefineFee(prt->cost);
#endif

	if (result_vnum == 0)
	{
		NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

	TItemTable* pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		NewChatPacket(NOT_ENOUGH_YANG);
		return false;
	}

	if (!bMoneyOnly)
	{
		for (int i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				NewChatPacket(NOT_ENOUGH_MATERIAL);
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int prob = number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;

	// END_OF_REFINE_COST

	if (prob <= prt->prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);

			CELL_UINT bCell = item->GetCell();

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			PayRefineFee(cost);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
			UpdateExtBattlePassMissionProgress(BP_ITEM_REFINE, 1, item->GetVnum());
#endif
		}
		else
		{
			// DETAIL_REFINE_LOG

			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN = 1, // buyulu metal sanirim
	YONGSIN_SCROLL = 2, // ejderha parsomeni -- 1<90
	MUSIN_SCROLL = 3,
	YAGONG_SCROLL = 4, // demircinin el kitabi 90=>
	MEMO_SCROLL = 5,
	BDRAGON_SCROLL = 6,
#ifdef ENABLE_PLUS_SCROLL
	PLUS_SCROLL = 15,
#endif
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	const TRefineTable* prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	LPITEM pkItemScroll;

	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		return false;
	}

	// MUSIN_SCROLL
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			NewChatPacket(CAN_NOT_UPGRADE_WITH_THIS_ITEM_MORE);
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pkItemScroll->GetValue(1))
		{
			NewChatPacket(CAN_NOT_UPGRADE_WITH_THIS_ITEM_MORE);
			return false;
		}
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM_METIN || item->GetRefineLevel() != 4)
		{
			NewChatPacket(CANT_UPGRADE_THIS_ITEM);
			return false;
		}
	}

#ifdef ENABLE_PLUS_SCROLL
	else if (pkItemScroll->GetValue(0) == PLUS_SCROLL)
	{
		if (item->GetLevelLimit() < pkItemScroll->GetValue(1))
		{
			NewChatPacket(CAN_ONLY_UPGRADE_ITEMS_MORE_THAN, "%d", pkItemScroll->GetValue(1));
			return false;
		}
	}	
	else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
	{
		if (item->GetLevelLimit() >= 90)
		{
			NewChatPacket(STRING_D189);
			return false;
		}
	}	
	else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
	{
		if (item->GetLevelLimit() < 90)
		{
			NewChatPacket(STRING_D190);
			return false;
		}
	}
#endif

	TItemTable* pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		return false;
	}

	if (GetGold() < prt->cost)
	{
		NewChatPacket(NOT_ENOUGH_YANG);
		return false;
	}

	for (int i = 0; i < prt->material_count; ++i)
	{
		if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
		{
			NewChatPacket(NOT_ENOUGH_MATERIAL);
			return false;
		}
	}

	for (int i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);

	int prob = number(1, 100);
	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN ||
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL ||
		pkItemScroll->GetValue(0) == YAGONG_SCROLL
#ifdef ENABLE_PLUS_SCROLL
		|| pkItemScroll->GetValue(0) == PLUS_SCROLL
#endif
		)
	{
		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL) {}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL) {}
		else if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) {} // @fixme121
#ifdef ENABLE_PLUS_SCROLL
		else if (pkItemScroll->GetValue(0) == PLUS_SCROLL) {}
#endif
		else
		{
			sys_err("REFINE : Unknown refine scroll item. Value0: %d", pkItemScroll->GetValue(0));
		}

		if (test_server)
		{
			ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
		}
#ifdef ENABLE_PLUS_SCROLL
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN || pkItemScroll->GetValue(0) == PLUS_SCROLL)
#else
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
#endif
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			success_prob += 10;
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			success_prob += 10;
			szRefineType = "YAGONG_SCROLL";
		}
#ifdef ENABLE_PLUS_SCROLL
		else if (pkItemScroll->GetValue(0) == PLUS_SCROLL)
		{
			success_prob += pkItemScroll->GetValue(2);
			szRefineType = "PLUS_SCROLL";
		}
#endif
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);

#ifdef ENABLE_PASSIVE_SKILLS
	int upgradeSkillLevel{ GetSkillLevel(SKILL_PASSIVE_UPGRADING) };

	if ((upgradeSkillLevel / 5) >= 1)
		success_prob += upgradeSkillLevel / 5;
#endif

	if (prob <= success_prob)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);

			CELL_UINT bCell = item->GetCell();

			NotifyRefineSuccess(this, item, szRefineType);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
			UpdateExtBattlePassMissionProgress(BP_ITEM_REFINE, 1, item->GetVnum());
#endif
#ifdef ENABLE_PLAYER_STATS_SYSTEM
			PointChange(POINT_UPGRADE_ITEM, 1);
#endif
		}
		else
		{
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}

	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);

			CELL_UINT bCell = item->GetCell();

			NotifyRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType);

		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(CELL_UINT bCell, BYTE bType, int iAdditionalCell)
{
	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;

	if (p.result_vnum == 0)
	{
		sys_err("RefineInformation p.result_vnum == 0");
		NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			NewChatPacket(CANT_UPGRADE_THIS_ITEM);
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				NewChatPacket(CANT_COMBINE_SIMILAR_REFINE_ITEMS);
				NewChatPacket(MAGIC_STONE_REFINE_CAN_BE_COMBINED);
				return false;
			}
		}
	}

	CRefineManager& rm = CRefineManager::instance();

	const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());

	if (!prt)
	{
		sys_err("RefineInformation NOT GET REFINE SET %d", item->GetRefineSet());
		NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		return false;
	}

	p.cost = ComputeRefineFee(prt->cost);

	p.prob = prt->prob;
	p.material_count = prt->material_count;
	thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));


	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell());
#ifdef ENABLE_PLUS_SCROLL
		else if (pkItem->GetValue(0) == PLUS_SCROLL)
		{
			if (pkTarget->GetLevelLimit() < pkItem->GetValue(1)) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_PLUS_SCROLL, pkItem->GetCell());
		}
		else if (pkItem->GetValue(0) == YONGSIN_SCROLL)
		{
			if (pkTarget->GetLevelLimit() >= 90) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
		else if (pkItem->GetValue(0) == YAGONG_SCROLL)
		{
			if (pkTarget->GetLevelLimit() < 90) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
#endif
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::instance().GetTable(pkTarget->GetSocket(i));
					//pkTarget->SetSocket(i, pTable->alValues[2]);

					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			NewChatPacket(NO_SPIRIT_STONES_TO_REMOVE);
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("kill_campfire_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
	case 66:
	case 216:
		iEmpireByMapIndex = -1;
		break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		NewChatPacket(CANT_SAVE_THIS_POSITION);
		return false;
	}

	int pos;

	if (item->GetCount() == 1)
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1)
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
	case 66:
	case 216:
		iEmpireByMapIndex = -1;
		break;

	case 301:
	case 302:
	case 303:
	case 304:
		if (GetLevel() < 90)
		{
			NewChatPacket(YOU_HAVE_LOWER_LEVEL_THAN_ITEM);
			return;
		}
		else
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		NewChatPacket(CANT_TELEPORT_SAVEZONE_IN_DIFFERENT_EMPIRE);
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
#ifdef ENABLE_OPEN_SHOP_WITH_ARMOR
	ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
#else
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
	case 0:
	case 1:
	case 2:
		ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
		break;
	default:
		NewChatPacket(TAKE_OFF_YOUR_ARMOR_TO_OPEN_SHOP);
		break;
	}
#endif
}

// MYSHOP_PRICE_LIST
#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, int64_t dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %lld", dwItemVnum, dwItemPrice);
#else
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
#endif
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}

void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader * p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)

		SendMyShopPriceListCmd(1, 0);
	else {
		for (int idx = 0; idx < p->byCount; idx++)
			SendMyShopPriceListCmd(pInfo[idx].dwVnum, pInfo[idx].dwPrice);
	}

	__OpenPrivateShop();
}

//

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
bool CHARACTER::UseExtendedBlendAffect(LPITEM item, int affect_type, int apply_type, int apply_value, int apply_duration)
{
	apply_duration = apply_duration <= 0 ? INFINITE_AFFECT_DURATION : apply_duration;
	bool bStatus = item->GetSocket(3);

	if (FindAffect(affect_type, apply_type))
	{
		NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
		return false;
	}

	if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, apply_type) || FindAffect(AFFECT_MALL, apply_type))
	{
		NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
		return false;
	}

	switch (item->GetVnum())
	{
		// DEWS
	case 50821:
	case 950821:
	{
		if (FindAffect(AFFECT_BLEND_POTION_1))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_1);
			return false;
		}

		AddAffect(AFFECT_BLEND_POTION_1, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 50822:
	case 950822:
	{
		if (FindAffect(AFFECT_BLEND_POTION_2))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_2);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_2, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 50823:
	case 950823:
	{
		if (FindAffect(AFFECT_BLEND_POTION_3))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_3);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_3, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 50824:
	case 950824:
	{
		if (FindAffect(AFFECT_BLEND_POTION_4))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_4);
			return false;
		}
		if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
		{
			NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
			return false;
		}

		AddAffect(AFFECT_BLEND_POTION_4, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 50825:
	case 950825:
	{
		if (FindAffect(AFFECT_BLEND_POTION_5))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_5);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_5, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 50826:
	case 950826:
	{
		if (FindAffect(AFFECT_BLEND_POTION_6))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_6);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_6, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 50827:
	case 950827:
	{
		if (FindAffect(AFFECT_BLEND_POTION_7))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_7);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_7, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 50828:
	case 950828:
	{
		if (FindAffect(AFFECT_BLEND_POTION_8))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_8);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_8, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 50829:
	case 950829:
	{
		if (FindAffect(AFFECT_BLEND_POTION_9))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_BLEND_POTION_9);
			return false;
		}
		AddAffect(AFFECT_BLEND_POTION_9, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	// END_OF_DEWS

	// ENERGY_CRISTAL
	case 51002:
	case 951002:
	{
		if (FindAffect(AFFECT_ENERGY))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_ENERGY);
			return false;
		}
		AddAffect(AFFECT_ENERGY, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	// END_OF_ENERGY_CRISTAL

	// DRAGON_GOD_MEDALS
	case 71027:
	case 939017:
	{
		if (FindAffect(AFFECT_DRAGON_GOD_1))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_DRAGON_GOD_1);
			return false;
		}

		AddAffect(AFFECT_DRAGON_GOD_1, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 71028:
	case 939018:
	{
		if (FindAffect(AFFECT_DRAGON_GOD_2))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_DRAGON_GOD_2);
			return false;
		}
		AddAffect(AFFECT_DRAGON_GOD_2, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 71029:
	case 939019:
	{
		if (FindAffect(AFFECT_DRAGON_GOD_3))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_DRAGON_GOD_3);
			return false;
		}
		AddAffect(AFFECT_DRAGON_GOD_3, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 71030:
	case 939020:
	{
		if (FindAffect(AFFECT_DRAGON_GOD_4))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_DRAGON_GOD_4);
			return false;
		}
		AddAffect(AFFECT_DRAGON_GOD_4, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	// END_OF_DRAGON_GOD_MEDALS

	// CRITICAL_AND_PENETRATION
	case 71044:
	case 939024:
	{
		if (FindAffect(AFFECT_CRITICAL))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_CRITICAL);
			return false;
		}
		AddAffect(AFFECT_CRITICAL, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 71045:
	case 939025:
	{
		if (FindAffect(AFFECT_PENETRATE))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_PENETRATE);
			return false;
		}
		AddAffect(AFFECT_PENETRATE, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	// END_OF_CRITICAL_AND_PENETRATION

	// ATTACK_AND_MOVE_SPEED
	case 27102:
	case 927209:
	{
		if (FindAffect(AFFECT_ATTACK_SPEED))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_ATTACK_SPEED);
			return false;
		}

		if (FindAffect(AFFECT_ATT_SPEED))
		{
			NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
			return false;
		}

		AddAffect(AFFECT_ATTACK_SPEED, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 27105:
	case 927212:
	{
		if (FindAffect(AFFECT_MOVE_SPEED))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_MOVE_SPEED);
			return false;
		}

		if (FindAffect(AFFECT_MOV_SPEED))
		{
			NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
			return false;
		}

		AddAffect(AFFECT_MOVE_SPEED, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950817:
	case 50817:
	{
		if (FindAffect(AFFECT_WATER_POTION_1))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_WATER_POTION_1);
			return false;
		}
		AddAffect(AFFECT_WATER_POTION_1, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;
	case 950818:
	case 50818:
	{
		if (FindAffect(AFFECT_WATER_POTION_2))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_WATER_POTION_2);
			return false;
		}
		AddAffect(AFFECT_WATER_POTION_2, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950830:
	case 50830:
	{
		if (FindAffect(AFFECT_POTION_ELEMENT_ICE))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_POTION_ELEMENT_ICE);
			return false;
		}
		AddAffect(AFFECT_POTION_ELEMENT_ICE, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950831:
	case 50831:
	{
		if (FindAffect(AFFECT_POTION_ELEMENT_FIRE))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_POTION_ELEMENT_FIRE);
			return false;
		}
		AddAffect(AFFECT_POTION_ELEMENT_FIRE, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950832:
	case 50832:
	{
		if (FindAffect(AFFECT_POTION_ELEMENT_WIND))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_POTION_ELEMENT_WIND);
			return false;
		}
		AddAffect(AFFECT_POTION_ELEMENT_WIND, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950833:
	case 50833:
	{
		if (FindAffect(AFFECT_POTION_ELEMENT_DARK))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_POTION_ELEMENT_DARK);
			return false;
		}
		AddAffect(AFFECT_POTION_ELEMENT_DARK, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950834:
	case 50834:
	{
		if (FindAffect(AFFECT_POTION_ELEMENT_ELEC))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_POTION_ELEMENT_ELEC);
			return false;
		}
		AddAffect(AFFECT_POTION_ELEMENT_ELEC, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	case 950835:
	case 50835:
	{
		if (FindAffect(AFFECT_POTION_ELEMENT_EARTH))
		{
			if (!bStatus)
			{
				NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
				return false;
			}
			RemoveAffect(AFFECT_POTION_ELEMENT_EARTH);
			return false;
		}
		AddAffect(AFFECT_POTION_ELEMENT_EARTH, apply_type, apply_value, 0, apply_duration, 0, true);
	}
	break;

	// END_OF_ATTACK_AND_MOVE_SPEED
	}

	return true;
}
#endif

#if defined(ENABLE_BLEND_AFFECT)
bool CHARACTER::SetBlendAffect(LPITEM item)
{
	switch (item->GetVnum())
	{
		// DEWS
	case 50821:
	case 950821:
		AddAffect(AFFECT_BLEND_POTION_1, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50822:
	case 950822:
		AddAffect(AFFECT_BLEND_POTION_2, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50823:
	case 950823:
		AddAffect(AFFECT_BLEND_POTION_3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50824:
	case 950824:
		AddAffect(AFFECT_BLEND_POTION_4, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50825:
	case 950825:
		AddAffect(AFFECT_BLEND_POTION_3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50826:
	case 950826:
		AddAffect(AFFECT_BLEND_POTION_6, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50827:
	case 950827:
		AddAffect(AFFECT_BLEND_POTION_7, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50828:
	case 950828:
		AddAffect(AFFECT_BLEND_POTION_8, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50829:
	case 950829:
		AddAffect(AFFECT_BLEND_POTION_9, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

		// END_OF_DEWS

		// ENERGY_CRISTAL
	case 51002:
	case 951002:
		AddAffect(AFFECT_ENERGY, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
		// END_OF_ENERGY_CRISTAL

		// DRAGON_GOD_MEDALS
	case 71027:
	case 939017:
		AddAffect(AFFECT_DRAGON_GOD_1, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 71028:
	case 939018:
		AddAffect(AFFECT_DRAGON_GOD_2, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 71029:
	case 939019:
		AddAffect(AFFECT_DRAGON_GOD_3, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 71030:
	case 939020:
		AddAffect(AFFECT_DRAGON_GOD_4, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
		// END_OF_DRAGON_GOD_MEDALS

		// CRITICAL_AND_PENETRATION
	case 71044:
	case 939024:
		AddAffect(AFFECT_CRITICAL, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 71045:
	case 939025:
		AddAffect(AFFECT_PENETRATE, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
		// END_OF_CRITICAL_AND_PENETRATION

		// ATTACK_AND_MOVE_SPEED
	case 27102:
	case 927209:
		AddAffect(AFFECT_ATTACK_SPEED, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 27105:
	case 927212:
		AddAffect(AFFECT_MOVE_SPEED, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 50817:
	case 950817:
		AddAffect(AFFECT_WATER_POTION_1, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
	case 50818:
	case 950818:
		AddAffect(AFFECT_WATER_POTION_2, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;
		// END_OF_ATTACK_AND_MOVE_SPEED


		// ELEMENT POTIONS
	case 50830:
	case 950830:
		AddAffect(AFFECT_POTION_ELEMENT_ICE, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50831:
	case 950831:
		AddAffect(AFFECT_POTION_ELEMENT_FIRE, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50832:
	case 950832:
		AddAffect(AFFECT_POTION_ELEMENT_WIND, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50833:
	case 950833:
		AddAffect(AFFECT_POTION_ELEMENT_DARK, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50834:
	case 950834:
		AddAffect(AFFECT_POTION_ELEMENT_ELEC, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;

	case 50835:
	case 950835:
		AddAffect(AFFECT_POTION_ELEMENT_EARTH, APPLY_NONE, 0, AFF_NONE, item->GetSocket(2), 0, false, false);
		break;




	default:
		return false;
	}

	return true;
}
#endif


//
void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop) {
		DWORD dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(DWORD));
		m_bNoOpenedShop = false;
	}
	else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST

int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT = 30;
	static const int WARP_MIN_LIFE_PERCENT = 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const HP_LL curLife = ch->GetHP();
		const int needPercent = WARP_NEED_LIFE_PERCENT;
		const HP_LL needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->NewChatPacket(NOT_ENOUGH_HP);
			return -1;
		}

		consumeLife = needLife;

		const int minPercent = WARP_MIN_LIFE_PERCENT;
		const HP_LL minLife = ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->NewChatPacket(NOT_ENOUGH_SP);
		return -1;
	}

	return needSP;
}

// #define ENABLE_FIREWORK_STUN
#define ENABLE_ADDSTONE_FAILURE
bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	, MAX_COUNT item_open_count
#endif
)
{
#ifdef ENABLE_RENEWAL_PVP
	if (CheckPvPUse(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1005"));
		return false;
	}
#endif

	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	//int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
		case LIMIT_LEVEL:
			if (GetLevel() < limitValue)
			{
				NewChatPacket(YOU_HAVE_LOWER_LEVEL_THAN_ITEM);
				return false;
			}
			break;

		case LIMIT_REAL_TIME_START_FIRST_USE:
			iLimitRealtimeStartFirstUseFlagIndex = i;
			break;

		case LIMIT_TIMER_BASED_ON_WEAR:
			//iLimitTimerBasedOnWearFlagIndex = i;
			break;
		}
	}

	if (CArenaManager::instance().IsLimitedItem(GetMapIndex(), item->GetVnum()) == true)
	{
		NewChatPacket(CANT_DO_THIS_IN_DUEL);
		return false;
	}

	// @fixme402 (IsLoadedAffect to block affect hacking)
	if (!IsLoadedAffect())
	{
		NewChatPacket(AFFECTS_ARE_NOT_LOADED_YET);
		return false;
	}

	// @fixme141 BEGIN
	if (TItemPos(item->GetWindow(), item->GetCell()).IsBeltInventoryPosition())
	{
		LPITEM beltItem = GetWear(WEAR_BELT);

		if (NULL == beltItem)
		{
			NewChatPacket(CANT_DO_THIS_IF_NOT_WEAR_BELT);
			return false;
		}

		if (false == CBeltInventoryHelper::IsAvailableCell(item->GetCell() - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
		{
			NewChatPacket(CANT_DO_THIS_IF_NOT_UPGRADE_BELT);
			return false;
		}
	}
	// @fixme141 END

	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		if (0 == item->GetSocket(1))
		{
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}

		if (false == item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);
	}

	switch (item->GetType())
	{
		case ITEM_HAIR:
		{
			return ItemProcess_Hair(item, wDestCell);
			break;
		}

		case ITEM_POLYMORPH:
		{
			return ItemProcess_Polymorph(item);
			break;
		}

		case ITEM_QUEST:
		{
#ifdef __AUTO_HUNT__
		if (item->GetVnum() == 38014)
		{
			if (FindAffect(AFFECT_AUTO_HUNT))
			{
				ChatPacket(CHAT_TYPE_INFO, "You already has affect");
				return false;
			}
			AddAffect(AFFECT_AUTO_HUNT, POINT_NONE, 0, AFF_NONE, 60 * 60 * 24 * item->GetValue(0), 0, false);
			ChatPacket(CHAT_TYPE_INFO, "Affect added for %d day.", item->GetValue(0));
			item->SetCount(item->GetCount() - 1);
			return true;
		}
#endif

#ifdef ENABLE_QUEST_DND_EVENT
			if (IS_SET(item->GetFlag(), ITEM_FLAG_APPLICABLE))
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				quest::CQuestManager::instance().DND(GetPlayerID(), item, item2, false);
				return true;
			}
#endif

			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					NewChatPacket(CANT_DO_THIS_IN_DUEL);
					return false;
				}
			}
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			if (GetWear(WEAR_COSTUME_MOUNT))
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					NewChatPacket(CANT_DO_THIS_WHILE_MOUNTING);
					return false;
				}
			}
#endif
			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
			break;
		}
		case ITEM_CAMPFIRE:
		{

			if (GetMapIndex() == 113) // lightwork campfire ox fixed
			{
				NewChatPacket(ACTION_NOT_POSSIBLE_ON_THIS_MAP);
				return false;
			}

			float fx, fy;
			GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

			LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX() + fx), (long)(GetY() + fy));

			if (!tree)
			{
				NewChatPacket(CANT_SET_A_CAMPFIRE_HERE);
				return false;
			}

			if (tree->IsAttr((long)(GetX() + fx), (long)(GetY() + fy), ATTR_WATER))
			{
				NewChatPacket(CANT_SET_A_CAMPFIRE_HERE);
				return false;
			}

			if ((m_campFireUseTime + 30) > get_global_time()) //lightwork campfire spam fixed
			{
				NewChatPacket(WAIT_TO_USE_AGAIN, "%d",(m_campFireUseTime + 30) - get_global_time());
				return false;
			}

			m_campFireUseTime = get_global_time(); //lightwork campfire spam fixed

			LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX() + fx), (long)(GetY() + fy), 0, false, number(0, 359));

			char_event_info* info = AllocEventInfo<char_event_info>();

			info->ch = campfire;

			campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

			item->SetCount(item->GetCount() - 1);
			break;
		}
	

		case ITEM_UNIQUE:
		{
			switch (item->GetSubType())
			{
				case USE_ABILITY_UP:
				{
					switch (item->GetValue(0))
					{
					case APPLY_MOV_SPEED:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
						break;

					case APPLY_ATT_SPEED:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
						break;

					case APPLY_STR:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ST))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_DEX:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_DX))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_CON:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_HT))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_INT:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_CAST_SPEED:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_RESIST_MAGIC:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_ATT_GRADE_BONUS:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS,
							item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;

					case APPLY_DEF_GRADE_BONUS:
						if (FindAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS))// @fixme171
							return false;
						AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
							item->GetValue(2), 0, item->GetValue(1), 0, true, true);
						break;
					}
				}

				if (GetDungeon())
					GetDungeon()->UsePotion(this);

				if (GetWarMap())
					GetWarMap()->UsePotion(this, item);

				item->SetCount(item->GetCount() - 1);
				break;

				default:
				{
					if (item->GetSubType() == USE_SPECIAL)
					{
						switch (item->GetVnum())
						{
						case 71049:
							UseSilkBotary();
							break;
						}
					}
					else
					{
						if (!item->IsEquipped())
						{
							if (item->GetCount() > 1)
							{
								LPITEM uniqueItem{ GetWear(WEAR_UNIQUE1) };
								LPITEM uniqueItem2{ GetWear(WEAR_UNIQUE2) };

								if (uniqueItem && uniqueItem->GetVnum() == item->GetVnum())
									return false;

								if (uniqueItem2 && uniqueItem2->GetVnum() == item->GetVnum())
									return false;


								if (item->GetValue(1) &&
									((uniqueItem && uniqueItem->GetValue(1) == item->GetValue(1)) ||
									(uniqueItem2 && uniqueItem2->GetValue(1) == item->GetValue(1))))
								{
									NewChatPacket(CANT_WEAR_THIS_SECOND_TIME);
									return false; // same special group control
								}
										
								MAX_COUNT count = item->GetCount();
								LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count-1, 0, false);

								if (!pkNewItem)
								{
									sys_err("%d can't be created. ItemUnique", item->GetVnum());
									return false;
								}

								CELL_UINT bCell = item->GetCell();
								item->SetCount(1);

								if (!EquipItem(item))
								{
									item->SetCount(count);
									M2_DESTROY_ITEM(pkNewItem);
									return false;
								}

								pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
								ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

							}
							else
								EquipItem(item);
						}
							
						else
							UnequipItem(item);
					}
				}
				break;
			}
			break;
		}

		case ITEM_COSTUME:
		{
#ifdef ENABLE_ACCE_COSTUME_SKIN
			if (item->GetSubType() == COSTUME_WING)
			{
				LPITEM acce = GetWear(WEAR_COSTUME_ACCE);
				if (acce)
				{
					if (!item->IsEquipped())
					{
						EquipItem(item);
					}
					else
					{
						UnequipItem(item);
					}
				}
				else
				{
					NewChatPacket(CANT_WEAR_SASH_SKIN_IF_NOT_WEAR_SASH);
					return false;
				}
			}
#endif
			else
			{
				if (!item->IsEquipped())
					EquipItem(item);
				else
					UnequipItem(item);
			}
			break;
		}


		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:
		case ITEM_BELT:
#ifdef ENABLE_PET_COSTUME_SYSTEM
		case ITEM_PET:
#endif
#ifdef ENABLE_CROWN_SYSTEM
		case ITEM_CROWN:
#endif
#ifdef ENABLE_ITEMS_SHINING
		case ITEM_SHINING:
#endif
#ifdef ENABLE_BOOSTER_ITEM
		case ITEM_BOOSTER:
#endif
#ifdef ENABLE_PENDANT_ITEM
		case ITEM_PENDANT:
#endif
#ifdef ENABLE_GLOVE_ITEM
		case ITEM_GLOVE:
#endif
#ifdef ENABLE_RINGS
		case ITEM_RINGS:
#endif
#ifdef ENABLE_DREAMSOUL
		case ITEM_DREAMSOUL:
#endif
		case ITEM_PICK:
		{
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		}

		case ITEM_DS:
		{
			if (!item->IsEquipped())
				return false;
			return DSManager::instance().PullOut(this, NPOS, item);
			break;
		}
		case ITEM_SPECIAL_DS:
		{
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		}

#ifdef ENABLE_MOUNT_SKIN
		case ITEM_MOUNT_SKIN:
		{
			if (true == IsRiding())//binekteyse degistiremez
			{
				NewChatPacket(CANT_DO_THIS_WHILE_MOUNTING);
				return false;
			}
			LPITEM mount = GetWear(WEAR_COSTUME_MOUNT);
			if (mount)//normal binek takiliyken 
			{
				if (!item->IsEquipped())
				{
					EquipItem(item);
				}
				else
				{
					UnequipItem(item);
				}
			}
			else//takili degilse return
			{
				if (item->IsEquipped())
				{
					UnequipItem(item);
				}
				else
				{
					NewChatPacket(CANT_WEAR_MOUNT_SKIN_IF_NOT_WEAR_MOUNT);
					return false;
				}
			}
			break;
		}
#endif

#ifdef ENABLE_PET_SKIN
		case ITEM_PET_SKIN:
		{
			LPITEM pkPet = GetWear(WEAR_PET);
	
			if (pkPet)//normal Pet takiliyyken 
			{
				if (!item->IsEquipped())
				{
					EquipItem(item);
				}
				else
				{
					UnequipItem(item);
				}
			}
			else//takili degilse return
			{
				if (item->IsEquipped())
				{
					UnequipItem(item);
				}
				else
				{
					NewChatPacket(CANT_WEAR_PET_SKIN_IF_NOT_WEAR_PET);
					return false;
				}
			}
			break;
		}
#endif

#ifdef ENABLE_AURA_SKIN
		case ITEM_AURA_SKIN:
		{
			LPITEM aura = GetWear(WEAR_COSTUME_AURA);
			if (aura)
			{
				if (!item->IsEquipped())
				{
					EquipItem(item);
				}
				else
				{
					UnequipItem(item);
				}
			}
			else
			{
				NewChatPacket(CANT_WEAR_AURA_SKIN_IF_NOT_WEAR_AURA);
				return false;
			}
			break;
		}
#endif

		case ITEM_FISH:
		{
			if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
			{
				NewChatPacket(CANT_DO_THIS_IN_DUEL);
				return false;
			}
			if (item->GetSubType() == FISH_ALIVE)
				fishing::UseFish(this, item);
			break;
		}

#ifdef ENABLE_CHEST_OPEN_RENEWAL
		case ITEM_GIFTBOX:
		{
			bool multiopen = false;
			if (item_open_count > 1)
			{
				multiopen = true;
			}

			if (multiopen)
			{
				if (!IsActivateTime(OPEN_CHEST_CHECK_TIME, 1))
				{
					NewChatPacket(WAIT_SOME_TIME_TO_DO_THIS_AGAIN);
					return false;
				}
			}

			const CSpecialItemGroup* pGroup = ITEM_MANAGER::Instance().GetSpecialItemGroup(item->GetVnum());
			if (item->GetType() == ITEM_GIFTBOX && pGroup)
			{
				switch (item->GetVnum())//Simya sandığı vnumları eklencek
				{
					case 51511:
					case 51512:
					case 51513:
					case 51514:
					case 51515:
					case 51516:
					case 200020:
					{
						if (!ChestDSInventoryEmpety(item->GetVnum()))
						{
							NewChatPacket(NEED_2SPACE_IN_DS_INV);
							return false;
						}
						ChestOpenDragonSoul(item, multiopen);
						if (multiopen)
						{
							SetActivateTime(OPEN_CHEST_CHECK_TIME);
						}
						return true;
					}
				}

				if (!ChestInventoryEmpetyFull())
				{
					NewChatPacket(NEED_2SPACE_IN_INVS);
					return false;
				}

				switch (pGroup->GetOpenType())
				{
					case CSpecialItemGroup::CR_CONST:
					{
						if (multiopen)
							ChestOpenConst(item);
						else
							ChestOpenConstSingle(item);
						break;
					}
					case CSpecialItemGroup::CR_STONE:
					{
						if (multiopen)
							ChestOpenNewPct(item, CHEST_TYPE_STONE);
						else
							ChestOpenNewPctSingle(item);
						break;
					}
					case CSpecialItemGroup::CR_NEW_PCT:
					{
						if (multiopen)
							ChestOpenNewPct(item,CHEST_TYPE_NEWPCT);
						else
							ChestOpenNewPctSingle(item);
						break;
					}
					case CSpecialItemGroup::CR_NORM:
					{
						if (multiopen)
							ChestOpenNewPct(item, CHEST_TYPE_NORM);
						else
							ChestOpenNewPctSingle(item);
						break;
					}
					default:break;
				}
				if (multiopen)
				{
					SetActivateTime(OPEN_CHEST_CHECK_TIME);
				}
			}
			else
			{
				return false;
			}
			break;
		}
#endif

		case ITEM_SKILLFORGET:
		{
			if (!item->GetSocket(0))
			{
#ifdef ENABLE_BOOKS_STACKFIX
				item->SetCount(item->GetCount() - 1);
#else
				ITEM_MANAGER::instance().RemoveItem(item);
#endif
				return false;
			}

			DWORD dwVnum = item->GetSocket(0);

			if (SkillLevelDown(dwVnum))
			{
#ifdef ENABLE_BOOKS_STACKFIX
				item->SetCount(item->GetCount() - 1);
#else
				ITEM_MANAGER::instance().RemoveItem(item);
#endif
				NewChatPacket(LOWERING_YOUR_SKILL_SUCCEDEED);
			}
			else
				NewChatPacket(LOWERING_YOUR_SKILL_FAILED);
			break;
		}

		case ITEM_USE:
		{
			switch (item->GetSubType())
			{
				case USE_TIME_CHARGE_PER:
				{
					LPITEM pDestItem = GetItem(DestCell);
					if (NULL == pDestItem)
					{
						return false;
					}

					if (pDestItem->IsDragonSoul())
					{
						int ret;
						if (item->GetVnum() == DRAGON_HEART_VNUM)
						{
							ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
						}
						else
						{
							ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
						}
						if (ret > 0)
						{
							NewChatPacket(X_SECONDS_ARE_ADDED, "%d", ret);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							NewChatPacket(ADDING_TIME_FAILED);
							return false;
						}
					}
					else
						return false;
					break;
				}

				case USE_TIME_CHARGE_FIX:
				{
					LPITEM pDestItem = GetItem(DestCell);
					if (NULL == pDestItem)
					{
						return false;
					}

					if (pDestItem->IsDragonSoul())
					{
						int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
						if (ret)
						{
							NewChatPacket(X_SECONDS_ARE_ADDED, "%d", ret);
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							NewChatPacket(ADDING_TIME_FAILED);
							return false;
						}
					}
					else
						return false;
					break;
				}

				case USE_SPECIAL:
				{
					switch (item->GetVnum())
					{

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					case 93100:
					{
						int GetBPPremiumETime = CBattlePassManager::instance().BattlePassGetPremiumData();
						//if (GetBPPremiumETime != NULL)
						if (GetBPPremiumETime > 0) // Burada anladigim kadariysa premium battlepassin suresini cekiyor daha sonra suresi varsa veriyor
						{
							if (!FindAffect(AFFECT_BATTLEPASS))
							{
								AddAffect(AFFECT_BATTLEPASS, POINT_NONE, 0, AFF_BATTLEPASS, abs(time(0) - GetBPPremiumETime), 0, true, false);
								// CBattlePassManager::instance().BattlePassRequestOpen(this);
								item->SetCount(item->GetCount() - 1);
								UpdatePacket();

								WarpSet(GetX(), GetY());
								Stop();

							}
							else
							{
								NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
								return false;
							}
						}
						break;
					}
#endif

					case ITEM_MARRIAGE_RING:
					{
						marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
						if (pMarriage)
						{
							if (pMarriage->ch1 != NULL)
							{
								if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
								{
									NewChatPacket(CANT_DO_THIS_IN_DUEL);
									break;
								}
							}

							if (pMarriage->ch2 != NULL)
							{
								if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
								{
									NewChatPacket(CANT_DO_THIS_IN_DUEL);
									break;
								}
							}

							int consumeSP = CalculateConsumeSP(this);

							if (consumeSP < 0)
								return false;

							PointChange(POINT_SP, -consumeSP, false);

							WarpToPID(pMarriage->GetOther(GetPlayerID()));
						}
						else
							NewChatPacket(WEDDING_TEXT_1);
						break;
					}

					case 38060:
								{
									#ifdef ENABLE_EXPRESSING_EMOTION
										if(CountEmotion() >= 12){
											ChatPacket(CHAT_TYPE_INFO,"[EmotionExtra] You can't put more emotions.");
											return false;
										}

										item->SetCount(item->GetCount()-1);

										int pct = number(1, 100);

										if (pct <= 70){
											InsertEmotion();
											ChatPacket(CHAT_TYPE_INFO,"[EmotionExtra] Emotion successfully added.");
										}

									#else
										ChatPacket(CHAT_TYPE_INFO,"[EmotionExtra] System not working.");
									#endif

								}
								break;

					case UNIQUE_ITEM_CAPE_OF_COURAGE:
					case 70057:
					case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
					{
						int capeCost{0};
						const int playerLevel{ GetLevel() };

						if (playerLevel >= 30 && playerLevel < 50)
							capeCost = 5000;
						else if (playerLevel >= 50 && playerLevel < 75)
							capeCost = 10000;
						else if (playerLevel >= 75 && playerLevel < 90)
							capeCost = 15000;
						else if (playerLevel >= 90 && playerLevel < 105)
							capeCost = 25000;
						else if (playerLevel >= 105 && playerLevel <= 120)
							capeCost = 40000;

						if (capeCost > 0)
						{
							if (GetGold() < capeCost)
							{
								NewChatPacket(NOT_ENOUGH_YANG);
								return false;
							}
						}

						PointChange(POINT_GOLD, -static_cast<int64_t>(capeCost));

						AggregateMonster();
						AttractRanger();
#ifdef ENABLE_OFFICIAL_STUFF
						if (GetCapeEffectPulse() + 50 > thecore_pulse())
							return true;

						SpecificEffectPacket(1);
						SetCapeEffectPulse(thecore_pulse());
#endif
						return true;
					}

					case 72726:
					{
						if (FindAffect(AFFECT_AUTO_HP_RECOVERY, POINT_NONE))// @fixme171
							return false;

						AddAffect(AFFECT_AUTO_HP_RECOVERY, POINT_NONE, 0, 0, INFINITE_AFFECT_DURATION, 1, false);
						item->SetCount(item->GetCount() - 1);
						break;
					}
					case 72730:
					{
						if (FindAffect(AFFECT_AUTO_SP_RECOVERY, POINT_NONE))// @fixme171
							return false;

						AddAffect(AFFECT_AUTO_SP_RECOVERY, POINT_NONE, 0, 0, INFINITE_AFFECT_DURATION, 1, false);
						item->SetCount(item->GetCount() - 1);
						break;
					}


#ifdef ENABLE_ACCE_COSTUME_SYSTEM
					case ACCE_REVERSAL_VNUM_1:
					case ACCE_REVERSAL_VNUM_2:
					{
						LPITEM item2;
						if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
							return false;

						if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
							return false;

						if (!CleanAcceAttr(item, item2))
							return false;
						item->SetCount(item->GetCount() - 1);
						break;
					}
#endif
					case 27996:
					{
						item->SetCount(item->GetCount() - 1);
						AttackedByPoison(NULL); // @warme008
						break;
					}
					case 71013:
					{
						CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case 50100:
					case 50101:
					case 50102:
					case 50103:
					case 50104:
					case 50105:
					case 50106:
					{
						CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case fishing::FISH_MIND_PILL_VNUM:
					{
						AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20 * 60, 0, true);
						item->SetCount(item->GetCount() - 1);
						break;
					}

#ifdef ENABLE_ALIGNMENT_SYSTEM
					case 71107:
					{
						int alig = GetAlignment();
						if (alig >= 1000000)
						{
							NewChatPacket(CANT_UPGRADE_WITH_THIS_MORE);
							return false;
						}

						UpdateAlignment(20000);
						item->SetCount(item->GetCount() - 1);
						break;
					}
#endif
					case 70102:
					{
						if (GetAlignment() >= 0)
							return false;

						int delta = MIN(-GetAlignment(), item->GetValue(0));
						UpdateAlignment(delta);
						item->SetCount(item->GetCount() - 1);
						break;
					}

					case 71109:
					{
						LPITEM item2;

						if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
							return false;

						if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
							return false;

						if (item2->GetSocketCount() == 0)
							return false;

						switch (item2->GetType())
						{
						case ITEM_WEAPON:
							break;
						case ITEM_ARMOR:
							switch (item2->GetSubType())
							{
							case ARMOR_EAR:
							case ARMOR_WRIST:
							case ARMOR_NECK:
								NewChatPacket(NO_SPIRIT_STONES_TO_REMOVE);
								return false;
							}
							break;

						default:
							return false;
						}

						std::stack<long> socket;

						for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
							socket.push(item2->GetSocket(i));

						int idx = ITEM_SOCKET_MAX_NUM - 1;

						while (socket.size() > 0)
						{
							if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
								break;
							idx--;
							socket.pop();
						}

						if (socket.size() == 0)
						{
							NewChatPacket(NO_SPIRIT_STONES_TO_REMOVE);
							return false;
						}

						LPITEM pItemReward = AutoGiveItem(socket.top());

						if (pItemReward != NULL)
						{
							item2->SetSocket(idx, 1);
							item->SetCount(item->GetCount() - 1);
						}
						break;
					}
#ifdef ENABLE_NEW_PET_SYSTEM
					case 55701://Pet Item
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;

						LPITEM petItem = GetWear(WEAR_NEW_PET);
						if (petItem)
						{
							if (petItem == item)
							{
								if (UnequipItem(item))
								{
									newPet->PetEquip(item, false);
								}
							}
							else
							{
								NewChatPacket(UNEQUIP_YOUR_PET);
								return false;
							}
						}
						else
						{
							if (EquipItem(item))
							{
								newPet->PetEquip(item, true);
							}
						}
						return true;
					}

					case 55401://Pet egg
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						newPet->EggOpen(item);
						return true;
					}

					//Pet Feed Items
					case 55101:
					case 55102:
					case 55103:
					case 55104:
					case 55105:
					case 55106:
					{
						return false;//55-120 tarzı comar bir files yaparsak diye
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						newPet->Feed(item);
						return true;
					}
					case 55107:
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						newPet->IncreaseSkillSlot(item);
						return true;
					}
					
					case 55150:
					case 55151:
					case 55152:
					case 55153:
					case 55154:
					case 55155:
					case 55156:
					case 55157:
					case 55158:
					case 55159:
					case 55160:
					case 55161:
					case 55162:
					case 55163:
					case 55164:
					case 55165:
					{
						CNewPet* newPet = GetNewPetSystem();
						if (!newPet)
							return false;
						newPet->SetSkill(item);
						return true;
					}
#endif

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
					case 37000:
					case 37001:
					case 37002:
					case 37003:
					case 37004:
					case 37005:
					case 37006:
					case 37007:
					{
						if (FindAffect(AFFECT_PREMIUM))
						{
							NewChatPacket(PREMIUM_ALREADY_MEMBER);
							return false;
						}

						long time{ item->GetValue(0) };

						if (item->GetVnum() == 37003)
							time = INFINITE_AFFECT_DURATION_REALTIME;
						else if (item->GetVnum() == 37007)
							time = INFINITE_AFFECT_DURATION_REALTIME;

						AddAffect(AFFECT_PREMIUM, POINT_NONE, 0, AFF_PREMIUM, time, 0, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();


						WarpSet(GetX(), GetY());
						Stop();
					}
					break;

#endif

#ifdef ENABLE_AUTO_PICK_UP
					case 37008:
					case 37009:
					case 37010:
					case 37011:
					case 37012:
					case 37013:
					case 37014:
					case 37015:
					{
						if (FindAffect(AFFECT_AUTO_PICK_UP))
							return false;

						long time{ item->GetValue(0) };

						if (item->GetVnum() == 37011)
							time = INFINITE_AFFECT_DURATION_REALTIME;
						else if (item->GetVnum() == 37015)
							time = INFINITE_AFFECT_DURATION_REALTIME;

						AddAffect(AFFECT_AUTO_PICK_UP, POINT_NONE, 0, 0, time, 1, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();

						WarpSet(GetX(), GetY());
						Stop();

					}
					break;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
					case 71992:
					case 71993:
					case 71994:
					case 71995:
					case 71996:
					case 71997:
					case 71998:
					case 71999:
					case 64010:
					{
						if (!IsActivateTime(BUFFI_SUMMON_TIME, 5))
						{
							NewChatPacket(STRING_D53);
							return false;
						}
			
						if (GetQuestFlag("buffi.summon"))
						{
							if (!item->GetSocket(3))
							{
								NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
								return false;
							}
							else
							{
								if (GetBuffiSystem())
									GetBuffiSystem()->UnSummon();

								item->Lock(false);
								item->SetSocket(3, false);
								SetQuestFlag("buffi.summon", 0);
								SetQuestFlag("buffi.itemid", 0);
							}
						}
						else
						{
							if (!GetBuffiSystem())
								return false;

							if (item->GetVnum() == 71999 || item->GetVnum() == 71995)
							{
								SetQuestFlag("buffi.summon", 31);
							}
							else
							{
								if (get_global_time() >= item->GetSocket(0))
									return false;

								SetQuestFlag("buffi.summon", static_cast<int>(item->GetSocket(0)));
							}

							SetQuestFlag("buffi.itemid", static_cast<int>(item->GetID()));
							item->Lock(true);
							item->SetSocket(3, true);
							GetBuffiSystem()->Summon();
						}
						SetActivateTime(BUFFI_SUMMON_TIME);

						break;
					}
#endif
#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
					case 24600:
					{
						if (FindAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED))
						{
							NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
							return false;
						}

						long time = INFINITE_AFFECT_DURATION_REALTIME;

						AddAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED, POINT_NONE, 0, 0, time, 0, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();
					}
					break;
#endif

#ifdef ENABLE_METIN_QUEUE
					case 37016:
					case 37017:
					case 37018:
					case 37019:
					case 37020:
					case 37021:
					case 37022:
					case 37023:
					{
						if (FindAffect(AFFECT_METIN_QUEUE))
						{
							NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
							return false;
						}

						long time = 0;

						switch (item->GetVnum())
						{

						case 37016:
						case 37020:
						{
							time = (86400*7);
							break;
						}


						case 37017:
						case 37021:
						{
							time = (86400 * 14);
							break;
						}


						case 37018:
						case 37022:
						{
							time = (86400 * 30);
							break;
						}

						case 37019:
						case 37023:
						{
							time = INFINITE_AFFECT_DURATION_REALTIME;
							break;
						}

						default:
							return false;
						}

						AddAffect(AFFECT_METIN_QUEUE, POINT_NONE, 0, AFF_METIN_QUEUE, time, 0, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();
					}
					break;

#endif

#ifdef ENABLE_6TH_7TH_ATTR
					case 24601:
					case 24602:
					{
						Decrease67AttrCooldown(item);
						break;
					}

					case 79505:
					case 79512:
					case 79603:
					case 50096:
					{
						ConvertEventItems(item);
						break;
					}
					
#endif

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
					case 57000:
					case 57001:
					case 57002:
					case 57003:
					case 57004:
					case 57005:
					case 57006:
					case 57007:
					case 57008:
					case 57009:
					case 57010:
					case 57011:
					{
						const int fishType { static_cast<int>(item->GetVnum()) % 57000};
						const int currentAffect{ AFFECT_FISH_ITEM_01 + fishType };

						if (FindAffect(currentAffect))
						{
							NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
							return false;
						}

						AddAffect(currentAffect, fishItemInfo[fishType][0], fishItemInfo[fishType][1], 0, 30 * 60, 0, false);
						item->SetCount(item->GetCount() - 1);
						UpdatePacket();
						break;
					}

#endif

					default: break;
					}//switch getvnum
				}

				case USE_POTION_NODELAY:
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
						{
							NewChatPacket(ACTION_NOT_POSSIBLE_ON_THIS_MAP);
							return false;
						}

						switch (item->GetVnum())
						{
						case 70020:
						case 71018:
						case 71019:
						case 71020:
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
							{
								if (m_nPotionLimit <= 0)
								{
									NewChatPacket(THE_ITEM_USAGE_LIMIT_EXCEEDED);
									return false;
								}
							}
							break;
						}
						default:
						{
							NewChatPacket(ACTION_NOT_POSSIBLE_ON_THIS_MAP);
							return false;
						}
						}
					}
					
					if (CPVPManager::instance().IsFighting(GetPlayerID()) && IsBlockPvP(item->GetVnum()))
                            {
                                return false;
                            }
					
					bool used = false;

					if (item->GetValue(0) != 0)
					{
						if (GetHP() < GetMaxHP())
						{
							PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
							EffectPacket(SE_HPUP_RED);
							used = TRUE;
						}
					}

					if (item->GetValue(1) != 0)
					{
						if (GetSP() < GetMaxSP())
						{
							PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
							EffectPacket(SE_SPUP_BLUE);
							used = TRUE;
						}
					}

					if (item->GetValue(3) != 0)
					{
						if (GetHP() < GetMaxHP())
						{
							PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
							EffectPacket(SE_HPUP_RED);
							used = TRUE;
						}
					}

					if (item->GetValue(4) != 0)
					{
						if (GetSP() < GetMaxSP())
						{
							PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
							EffectPacket(SE_SPUP_BLUE);
							used = TRUE;
						}
					}

					if (used)
					{
						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						m_nPotionLimit--;

						//RESTRICT_USE_SEED_OR_MOONBOTTLE
						item->SetCount(item->GetCount() - 1);
						//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
					}
					break;
				}

				case USE_POTION:
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
						{
							NewChatPacket(ACTION_NOT_POSSIBLE_ON_THIS_MAP);
							return false;
						}

						switch (item->GetVnum())
						{
							case 27001:
							case 27002:
							case 27003:
							case 27004:
							case 27005:
							case 27006:
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
								{
									if (m_nPotionLimit <= 0)
									{
										NewChatPacket(THE_ITEM_USAGE_LIMIT_EXCEEDED);
										return false;
									}
								}
								break;
							}

							default:
							{
								NewChatPacket(ACTION_NOT_POSSIBLE_ON_THIS_MAP);
								return false;
							}
						}
					}

					if (item->GetValue(1) != 0)
					{
						if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
						{
							return false;
						}

						PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
						StartAffectEvent();
						EffectPacket(SE_SPUP_BLUE);
					}

					if (item->GetValue(0) != 0)
					{
						if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
						{
							return false;
						}

						PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
						StartAffectEvent();
						EffectPacket(SE_HPUP_RED);
					}

					if (GetDungeon())
						GetDungeon()->UsePotion(this);

					if (GetWarMap())
						GetWarMap()->UsePotion(this, item);

					item->SetCount(item->GetCount() - 1);
					m_nPotionLimit--;
					break;
				}

				case USE_POTION_CONTINUE:
				{
					if (item->GetValue(0) != 0)
					{
						AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
					}
					else if (item->GetValue(1) != 0)
					{
						AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
					}
					else
						return false;

					if (GetDungeon())
						GetDungeon()->UsePotion(this);

					if (GetWarMap())
						GetWarMap()->UsePotion(this, item);

					item->SetCount(item->GetCount() - 1);
					break;
				}



				case USE_ABILITY_UP:
				{
					switch (item->GetValue(0))
					{
						case APPLY_MOV_SPEED:
						{
							if (FindAffect(AFFECT_MOV_SPEED) || FindAffect(AFFECT_MOVE_SPEED))//@Lightwork#13
								return false;
							AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
							EffectPacket(SE_DXUP_PURPLE);
#endif
							break;
						}

						case APPLY_ATT_SPEED:
						{
							if (FindAffect(AFFECT_ATT_SPEED) || FindAffect(AFFECT_ATTACK_SPEED))//@Lightwork#13
								return false;
							AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
							EffectPacket(SE_SPEEDUP_GREEN);
#endif
							break;
						}

						case APPLY_STR:
						{
							AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_DEX:
						{
							AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_CON:
						{
							AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_INT:
						{
							AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_CAST_SPEED:
						{
							AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_ATT_GRADE_BONUS:
						{
							AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS,
								item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}

						case APPLY_DEF_GRADE_BONUS:
						{
							AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,
								item->GetValue(2), 0, item->GetValue(1), 0, true);
							break;
						}
					}
					if (GetDungeon())
						GetDungeon()->UsePotion(this);

					if (GetWarMap())
						GetWarMap()->UsePotion(this, item);

					item->SetCount(item->GetCount() - 1);
					break;
				}

				case USE_TUNING:
				case USE_DETACHMENT:
				{
					LPITEM item2;

					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
						return false;

					if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343)
					{
						NewChatPacket(CANT_UPGRADE_ITEM_WITH_LEVEL3_SPIRITS_STONES);
						return false;
					}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
					if (item->GetValue(0) == ACCE_CLEAN_ATTR_VALUE0
						|| item->GetVnum() == ACCE_REVERSAL_VNUM_1
						|| item->GetVnum() == ACCE_REVERSAL_VNUM_2
						)
					{
						if (item2->GetSubType() == COSTUME_ACCE)
						{
							if (!CleanAcceAttr(item, item2))
								return false;
						}

#ifdef ENABLE_AURA_SYSTEM
						else if (item2->GetSubType() == COSTUME_AURA)
						{
							if (!CleanAuraAttr(item, item2))
								return false;
						}
#endif
						item->SetCount(item->GetCount() - 1);
						return true;
					}
#endif
#ifdef ENABLE_PLUS_SCROLL
					if (item->GetValue(0) == PLUS_SCROLL)
					{
						if (item2->GetLevelLimit() < item->GetValue(1))
						{
							NewChatPacket(CAN_ONLY_UPGRADE_ITEMS_MORE_THAN, "%d", item->GetValue(1));
							return false;
						}
						else
							RefineItem(item, item2);
					}
					else if (item->GetValue(0) == YONGSIN_SCROLL)
					{
						if (item2->GetLevelLimit() >= 90)
						{
							NewChatPacket(STRING_D189);
							return false;
						}
						else
							RefineItem(item, item2);
					}
					else if (item->GetValue(0) == YAGONG_SCROLL)
					{
						if (item2->GetLevelLimit() < 90)
						{
							NewChatPacket(STRING_D190);
							return false;
						}
						else
							RefineItem(item, item2);
					}
#endif
					if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)
					{
						if (item->GetVnum() == 71056)
						{
							RefineItem(item, item2);
						}
						else
						{
							NewChatPacket(CANT_UPGRADE_ITEM_WITH_LEVEL3_SPIRITS_STONES);
						}
					}
					else
					{
						RefineItem(item, item2);
					}
					break;
				}
			

				case USE_CHANGE_COSTUME_ATTR:
				case USE_RESET_COSTUME_ATTR:
				{
					LPITEM item2;
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsEquipped())
					{
						BuffOnAttr_RemoveBuffsFromItem(item2);
					}

					if (ITEM_COSTUME != item2->GetType())
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					{
						return false;
					}

					if (item2->GetAttributeSetIndex() == -1)
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (item2->GetAttributeCount() == 0)
					{
						NewChatPacket(NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM);
						return false;
					}

					switch (item->GetSubType())
					{
						case USE_CHANGE_COSTUME_ATTR:
						{
							item2->ChangeAttribute();
							break;
						}
						case USE_RESET_COSTUME_ATTR:
						{
							item2->ClearAttribute();
							item2->AlterToMagicItem();
							break;
						}
					}
					NewChatPacket(CHANGE_ATTRIBUTE_WAS_SUCCESFULL);
					item->SetCount(item->GetCount() - 1);
					break;
				}

#ifdef ENABLE_COSTUME_ATTR
				case USE_CHANGE_ATTRIBUTE2:
				{
					LPITEM item2;
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging() || item2->IsEquipped())
						return false;

					if (ITEM_COSTUME != item2->GetType())
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (item2->GetAttributeSetIndex(true) != ATTRIBUTE_SET_RARE_COSTUME)
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (item2->GetLimitType(0) == LIMIT_REAL_TIME || item2->GetLimitType(1) == LIMIT_REAL_TIME)
					{
						NewChatPacket(STRING_D198);
						return false;
					}

					ChangeCostumeAttr(item2, item);
					return true;
				}
#endif
			//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
				case USE_PUT_INTO_BELT_SOCKET:
				case USE_PUT_INTO_ACCESSORY_SOCKET:
				case USE_ADD_ACCESSORY_SOCKET:
				case USE_CLEAN_SOCKET:
				case USE_CHANGE_ATTRIBUTE:
				case USE_ADD_ATTRIBUTE:
				case USE_ADD_ATTRIBUTE2:
				{
					LPITEM item2;
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (ITEM_COSTUME == item2->GetType())
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					{
						return false;
					}
					/*if (item2->IsEquipped())
					{
						BuffOnAttr_RemoveBuffsFromItem(item2);
					}*/

					switch (item->GetSubType())
					{
						case USE_CLEAN_SOCKET:
						{
							int i;
							for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
							{
								if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
									break;
							}

							if (i == ITEM_SOCKET_MAX_NUM)
							{
								NewChatPacket(NO_STONE_TO_CLEAR);
								return false;
							}

							int j = 0;

							for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
							{
								if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
									item2->SetSocket(j++, item2->GetSocket(i));
							}

							for (; j < ITEM_SOCKET_MAX_NUM; ++j)
							{
								if (item2->GetSocket(j) > 0)
									item2->SetSocket(j, 1);
							}

							item->SetCount(item->GetCount() - 1);
							break;
						}


						case USE_CHANGE_ATTRIBUTE:
						{

							if (item2->GetAttributeSetIndex() == -1)
							{
								NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
								return false;
							}

							if (item2->GetAttributeCount() == 0)
							{
								NewChatPacket(NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM);
								return false;
							}

							//@Lightwork#140_START
							if (BLOCKED_ATTR_LIST(item2->GetVnum()))
							{
								NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
								return false;
							}
							//@Lightwork#140_END

#ifdef ENABLE_6TH_7TH_ATTR
							if (item->GetVnum() == 72351 || item->GetVnum() == 72346)
							{
								Change67Attr(item, item2);
								return true;
							}
#endif
#ifdef ENABLE_GREEN_ATTRIBUTE_CHANGER
							if (item->GetVnum() == 71151)
							{
								if ((item2->GetType() == ITEM_WEAPON)
									|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
								{
									bool bCanUse = true;
									for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
									{
										if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
										{
											bCanUse = false;
											break;
										}
									}
									if (false == bCanUse)
									{
										NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
										break;
									}
								}
								else
								{
									NewChatPacket(THIS_CAN_BE_USED_IN_ONLY_WEAPONS_ARMORS);
									break;
								}
							}
#endif
							item2->ChangeAttribute();
#ifdef ENABLE_PLAYER_STATS_SYSTEM
							PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
							NewChatPacket(CHANGE_ATTRIBUTE_WAS_SUCCESFULL);
							item->SetCount(item->GetCount() - 1);
							break;
						}

						case USE_ADD_ATTRIBUTE:
						{
							if (item2->GetAttributeSetIndex() == -1)
							{
								NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
								return false;
							}
							//@Lightwork#140_START
							if (BLOCKED_ATTR_LIST(item2->GetVnum()))
							{
								NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
								return false;
							}
							//@Lightwork#140_END

							if (item2->IsExchanging())// @Lightwork#33
							{
								return false;
							}
							if (item2->IsEquipped())// @Lightwork#34
							{
								NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
								return false;
							}
							while (item2->GetAttributeCount() < 5)
							{
								item2->AddAttribute();
							}
							break;
						}

						case USE_ADD_ATTRIBUTE2:
						{
							if (item2->GetAttributeSetIndex() == -1)
							{
								NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
								return false;
							}

							if (item2->GetAttributeCount() == 4)
							{
								if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
								{
									item2->AddAttribute();
									NewChatPacket(ADDING_ATTRIBUTE_WAS_SUCCESSFULL);
								}
								else
								{
									NewChatPacket(ADDING_ATTRIBUTE_WAS_FAILED);
								}

								item->SetCount(item->GetCount() - 1);
							}
							else if (item2->GetAttributeCount() == 5)
							{
								NewChatPacket(CANT_ADD_MORE_BONUS_TO_THIS_ITEM);
							}
							else if (item2->GetAttributeCount() < 4)
							{
								NewChatPacket(TO_USE_THIS_YOU_NEED_FOR_BONUSSES_ON_THE_ITEM);
							}
							else
							{
								sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d) item id %d owner %s itemvnum %d", item2->GetAttributeCount(), item2->GetID(), item2->GetOwner()->GetName(), item2->GetVnum());
							}
							break;
						}

						case USE_ADD_ACCESSORY_SOCKET:
						{
#ifdef ENABLE_JEWELS_RENEWAL
							UseAddJewels(item, item2);
							return true;
#endif
							if (item2->IsAccessoryForSocket())
							{
								if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
								{
#ifdef ENABLE_ADDSTONE_FAILURE
									if (number(1, 100) <= 50)
#else
									if (1)
#endif
									{
										item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
										NewChatPacket(ADDING_SOCKET_SUCCESSFULL);
									}
									else
									{
										NewChatPacket(ADDING_SOCKET_UNSUCCESSFULL);
									}

									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									NewChatPacket(SOCKET_NOT_ENOUGH_SPACE);
								}
							}
							else
							{
								NewChatPacket(SOCKET_CANT_BE_ADDED_TO_THIS_ITEM);
							}
							break;
						}

						case USE_PUT_INTO_BELT_SOCKET:
						case USE_PUT_INTO_ACCESSORY_SOCKET:
						{
#ifdef ENABLE_JEWELS_RENEWAL
							UsePutIntoJewels(item, item2);
							return true;
#endif
							if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
							{
								if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
								{
									if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
									{
										item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
										NewChatPacket(ADDING_JEWEL_SUCCESFULL);
									}
									else
									{
										NewChatPacket(ADDING_JEWEL_UNSUCCESFULL);
									}

									item->SetCount(item->GetCount() - 1);
								}
								else
								{
									if (item2->GetAccessorySocketMaxGrade() == 0)
										NewChatPacket(ADD_JEWEL_INFO_01);
									else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
									{
										NewChatPacket(ADD_JEWEL_INFO_02);
										NewChatPacket(ADD_JEWEL_INFO_03);
									}
									else
										NewChatPacket(ADD_JEWEL_INFO_04);
								}
							}
							else
							{
								NewChatPacket(SOCKET_NOT_ENOUGH_SPACE);
							}
							break;
						}
						default: break;
					}//switch subtype

					if (item2->IsEquipped())
					{
						BuffOnAttr_AddBuffsFromItem(item2);
					}
					break;
				}

				case USE_BAIT:
				{
					if (m_pkFishingEvent)
					{
						NewChatPacket(FISHING_CANT_CHANGE_BAIT_WHEN_FISHING);
						return false;
					}

					LPITEM weapon = GetWear(WEAR_WEAPON);

					if (!weapon || weapon->GetType() != ITEM_ROD)
						return false;

					if (weapon->GetSocket(2))
					{
						NewChatPacket(FISHING_CHANGED_BAIT_WITH, "%s", item->GetName() );
					}
					else
					{
						NewChatPacket(FISHING_ADDED_BAIT, "%s", item->GetName());
					}

					weapon->SetSocket(2, item->GetValue(0));
					item->SetCount(item->GetCount() - 1);
					break;
				}
			

				case USE_AFFECT:
				{
					if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
					{
						NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
					}
					else
					{
						AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
						item->SetCount(item->GetCount() - 1);
					}
					break;
				}

#ifdef ENABLE_ATTR_RARE_RENEWAL
				case USE_CHANGE_RARE_ATTRIBUTE:
				case USE_ADD_RARE_ATTRIBUTE:
				{
					LPITEM item2;

					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsEquipped() || item2->IsExchanging())
						return false;

					int attrIndex = item2->GetAttributeSetIndex(true);
					if (attrIndex == -1)
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (!item->IsRareAttrItem(attrIndex))
					{
						NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
						return false;
					}

					if (item2->GetRareAttrCount() >= 5)
					{
						NewChatPacket(NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM);
						return false;
					}

					switch (item->GetSubType())
					{
						case USE_CHANGE_RARE_ATTRIBUTE:
						{
							if (!item2->ChangeRareAttribute())
							{
								NewChatPacket(NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM);
								return false;
							}
							NewChatPacket(CHANGE_ATTRIBUTE_WAS_SUCCESFULL);
							break;
						}
						case USE_ADD_RARE_ATTRIBUTE:
						{
							if (number(1, 100) <= 55)
							{
								if (!item2->AddRareAttribute())
								{
									NewChatPacket(NO_ATTRIBUTES_TO_CHANE_ON_THE_ITEM);
									return false;
								}
								NewChatPacket(IT_WAS_SUCCESFULL);
							}
							else
							{
								NewChatPacket(IT_WAS_FAILED);
							}
							break;
						}
						default:
							return false;
					}
					item->SetCount(item->GetCount() - 1);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
					PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
					break;
				}
#endif
			}//switch Subtype Item Type Use
			break;
		}
		

		case ITEM_METIN:
		{
			LPITEM item2;

			if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
				return false;

			if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
				return false;

#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
			const auto hasSuccesItem = FindAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED);
#endif

			if (item2->GetType() == ITEM_PICK) return false;
			if (item2->GetType() == ITEM_ROD) return false;

			int i;

			for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				DWORD dwVnum;

				if ((dwVnum = item2->GetSocket(i)) <= 2)
					continue;

				TItemTable* p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
					continue;

				if (item->GetValue(5) == p->alValues[5])
				{
					NewChatPacket(CANT_ADD_SAME_SPIRIT_STONES);
					return false;
				}
			}

			if (item2->GetType() == ITEM_ARMOR)
			{
				if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
				{
					NewChatPacket(CANT_ADD_THIS_SPIRIT_STONES_TO_THIS_ITEM);
					return false;
				}
			}
			else if (item2->GetType() == ITEM_WEAPON)
			{
				if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
				{
					NewChatPacket(CANT_ADD_THIS_SPIRIT_STONES_TO_THIS_ITEM);
					return false;
				}
			}
			else
			{
				NewChatPacket(NOT_ENOUGH_SPACE_TO_ADD_SPIRIT_STONE);
				return false;
			}

			for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
				{
#ifdef ENABLE_ADDSTONE_FAILURE
#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
					if (hasSuccesItem ? 1 : (number(1, 100) <= 60))
#else
					if (number(1, 100) <= 30)
#endif
#else
					if (1)
#endif
					{
						NewChatPacket(IT_WAS_SUCCESFULL);
						item2->SetSocket(i, item->GetVnum());
					}
					else
					{
						NewChatPacket(IT_WAS_FAILED);
						item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
					}
					item->SetCount(item->GetCount() - 1); // Lightwork@332 stack fix
					break;
				}
			}

#ifdef ENABLE_ADDSTONE_NOT_FAIL_ITEM
			if (hasSuccesItem)
				RemoveAffect(AFFECT_ADDSTONE_SUCCESS_ITEM_USED);
#endif

			if (i == ITEM_SOCKET_MAX_NUM)
				NewChatPacket(NOT_ENOUGH_SPACE_TO_ADD_SPIRIT_STONE);
			break;
		}
		

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
			break;

		case ITEM_TOTEM:
		{
			if (!item->IsEquipped())
				EquipItem(item);
			break;
		}
	

		case ITEM_BLEND: //#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
			{
				if (Blend_Item_find(item->GetVnum()))
				{
					int affect_type = AFFECT_BLEND;
					int apply_type = aApplyInfo[item->GetSocket(0)].bPointType;
					int apply_value = item->GetSocket(1);
					int apply_duration = item->GetSocket(2);

#if defined(ENABLE_EXTENDED_BLEND_AFFECT) && defined(ENABLE_SOCKET_LIMIT_5)
					if ((apply_duration != 0) && UseExtendedBlendAffect(item, affect_type, apply_type, apply_value, apply_duration))
					{
						item->Lock(true);
						item->SetSocket(3, true);
						item->StartBlendExpireEvent();
					}
					else
					{
						bool isDestroy{ false };

						item->Lock(false);
						item->SetSocket(3, false);
						item->StopBlendExpireEvent(isDestroy);

						if (isDestroy)
							ITEM_MANAGER::instance().RemoveItem(item);
					}
					break;
#endif

					if (FindAffect(affect_type, apply_type))
					{
						NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
					}
					else
					{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						NewChatPacket(AFFECT_IS_ALREADY_ACTIVE);
					}
					else
					{
#if defined(ENABLE_BLEND_AFFECT)
						if (SetBlendAffect(item))
						{
							AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
							item->SetCount(item->GetCount() - 1);
						}
#else
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
#endif
					}
				}
			}
		}
		break;

		case ITEM_EXTRACT:
		{
			LPITEM pDestItem = GetItem(DestCell);
			if (NULL == pDestItem)
			{
				return false;
			}
			switch (item->GetSubType())
			{
				case EXTRACT_DRAGON_SOUL:
				{
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				}
				case EXTRACT_DRAGON_HEART:
				{
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				}
				default:
					return false;
			}
			break;
		}
	

		case ITEM_NONE:
		{
			sys_err("Item type NONE %s", item->GetName());
			break;
		}

		default:
		{
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
		}
	}
	return true;
}

#ifdef ENABLE_DIZCIYA_GOTTEN
int g_nPortalLimitTime = 0;
#else
int g_nPortalLimitTime = 5;
#endif

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	, MAX_COUNT item_open_count
#endif
)
{
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;
	
#ifdef ENABLE_RENEWAL_PVP
	if (CheckPvPUse(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1005"));
		return false;
	}
#endif

	if (item->IsExchanging())
		return false;

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int32_t iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	if (Cell.IsBuffiEquipPosition())
	{
		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif

	if (!item->CanUsedBy(this))
	{
		NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
		return false;
	}

	if (false == FN_check_item_sex(this, item))
	{
		NewChatPacket(ONLY_OTHER_GENDER_CAN_USE_THIS);
		return false;
	}

	if (item->GetVnum() == ITEM_MARRIAGE_RING)
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()) || !CanWarp())
		{
			NewChatPacket(CANNOT_DO_THIS);
			return false;
		}
	}


#ifdef ENABLE_EXTENDED_BATTLE_PASS
	bool bpRet = (item && ((item->GetType() == ITEM_USE) || (item->GetType() == ITEM_SKILLBOOK) || (item->GetType() == ITEM_GIFTBOX)));
#endif

	bool ret = UseItemEx(item, DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		, item_open_count);
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	if (ret && bpRet)
		UpdateExtBattlePassMissionProgress(BP_ITEM_USE, item_open_count, item->GetVnum());
#endif

	return ret;
}

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
bool CHARACTER::DeleteItem(TItemPos Cell)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (true == item->isLocked() || item->IsEquipped())
		return false;

	if (!CanWarp())
		return false;

	NewChatPacket(ITEM_DELETED, "%s", item->GetName());

#ifdef ENABLE_ITEM_SELL_LOG
	TItemSellLog log;
	memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
	memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
	log.dwVnum = item->GetVnum();
	log.dwCount = item->GetCount();
	log.pid = GetPlayerID();
	snprintf(log.logType, sizeof(log.logType), "ITEM_DELETE");
	DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	UpdateExtBattlePassMissionProgress(BP_ITEM_DESTROY, 1, item->GetVnum());
#endif
	ITEM_MANAGER::instance().RemoveItem(item);

	return true;
}

bool CHARACTER::SellItem(TItemPos Cell)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (true == item->isLocked() || item->IsEquipped())
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
		return false;

	if (!CanWarp())
		return false;

	int64_t dwPrice;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT bCount;
#else
	BYTE bCount;
#endif

	bCount = item->GetCount();
	dwPrice = item->GetShopBuyPrice();

	if (IS_SET(item->GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

	const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] id %u name %s gold %lld", GetPlayerID(), GetName(), GetGold());
		NewChatPacket(YOU_CANNOT_CARRY_MORE_YANG);
		return false;
	}

#ifdef ENABLE_ITEM_SELL_LOG
	TItemSellLog log;
	memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
	memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
	log.dwVnum = item->GetVnum();
	log.dwCount = item->GetCount();
	log.pid = GetPlayerID();
	snprintf(log.logType, sizeof(log.logType), "ITEM_SELL");
	DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif
	NewChatPacket(ITEM_IS_SOLD, "%s", item->GetName());
	ITEM_MANAGER::instance().RemoveItem(item);
	PointChange(POINT_GOLD, dwPrice, false);
	return true;
}

#else
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
bool CHARACTER::DropItem(TItemPos Cell, MAX_COUNT bCount)
#else
bool CHARACTER::DropItem(TItemPos Cell, BYTE bCount)
#endif
{
	LPITEM item = NULL;

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			NewChatPacket(CLOSE_WINDOWS_ERROR);
		return false;
	}

	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		NewChatPacket(YOU_CANNOT_DROP_THIS_ITEM);
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		bCount = (MAX_COUNT)item->GetCount();
#else
		bCount = (BYTE)item->GetCount();
#endif

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, UINT16_MAX);

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		FN_copy_item_socket(pkItemToDrop, item);

	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		NewChatPacket(ITEM_DROPPED_WILL_DISSAPEAR_IN_MINS);
#ifdef ENABLE_NEWSTUFF
		pkItemToDrop->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]);
#else
		pkItemToDrop->StartDestroyEvent();
#endif

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);

	}

	return true;
}
#endif

bool CHARACTER::DropGold(int gold) // Lightwork@DropYangFix
{
	NewChatPacket(CANT_DROP_GOLD);
	return false;
}


bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT count
#else
	BYTE count
#endif
#ifdef ENABLE_SPLIT_ITEMS_FAST
	, bool isSplitItems
#endif
)
{
	LPITEM item = NULL;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (Cell.cell == DestCell.cell && Cell.window_type == DestCell.window_type)//c2fix1
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

#ifdef ENABLE_SPECIAL_STORAGE //moveitem fix
	if (!item->IsUpgradeItem() && DestCell.window_type == UPGRADE_INVENTORY)
	{
		NewChatPacket(CANT_MOVE_ITEM_TO_THIS_INVENTORY);
		return false;
	}

	if (!item->IsBook() && DestCell.window_type == BOOK_INVENTORY)
	{
		NewChatPacket(CANT_MOVE_ITEM_TO_THIS_INVENTORY);
		return false;
	}

	if (!item->IsStone() && DestCell.window_type == STONE_INVENTORY)
	{
		NewChatPacket(CANT_MOVE_ITEM_TO_THIS_INVENTORY);
		return false;
	}

	if (!item->IsChest() && DestCell.window_type == CHEST_INVENTORY)
	{
		NewChatPacket(CANT_MOVE_ITEM_TO_THIS_INVENTORY);
		return false;
	}
#endif

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition() && CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
	{
		NewChatPacket(YOU_CANNOT_MOVE_ACTIVE_SWITCHBOT_ITEMS);
		return false;
	}

	if (DestCell.IsSwitchbotPosition() && !SwitchbotHelper::IsValidItem(item))
	{
		NewChatPacket(YOU_CANNOT_ADD_THIS_ITEM_TO_SWITCHBOT);
		return false;
	}
#endif

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			NewChatPacket(CLOSE_WINDOWS_ERROR);
		return false;
	}

	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		NewChatPacket(CANT_MOVE_ITEM_TO_THIS_INVENTORY);
		return false;
	}
#ifdef ENABLE_BUFFI_SYSTEM
	if (DestCell.IsBuffiEquipPosition())
	{
		if (Cell.IsDefaultInventoryPosition())
		{
			if (!CanBuffiEquipItem(DestCell, item))
				return false;
		}
		else
		{
			return false;
		}
	}

	if (Cell.IsBuffiEquipPosition() && !DestCell.IsDefaultInventoryPosition())
	{
		return false;
	}
#endif

	if (Cell.IsEquipPosition())
	{
#ifdef ENABLE_NEW_PET_SYSTEM
		if (item->GetVnum() == 55701)
			return false;
#endif
		if (!CanUnequipNow(item))
			return false;

		int iWearCell = item->FindEquipCell(this);
		switch (iWearCell)
		{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			case WEAR_WEAPON:
			{
				LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
				if (costumeWeapon && !UnequipItem(costumeWeapon))
				{
					NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_MOUNT_SKIN
			case WEAR_COSTUME_MOUNT:
			{
				LPITEM mountCostume = GetWear(WEAR_MOUNT_SKIN);
				if (mountCostume && !UnequipItem(mountCostume))
				{
					NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_PET_SKIN
			case WEAR_PET:
			{
				LPITEM petCostume = GetWear(WEAR_PET_SKIN);
				if (petCostume && !UnequipItem(petCostume))
				{
					NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_AURA_SKIN
			case WEAR_COSTUME_AURA:
			{
				LPITEM auraCostume = GetWear(WEAR_AURA_SKIN);
				if (auraCostume && !UnequipItem(auraCostume))
				{
					NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
			case WEAR_COSTUME_ACCE:
			{
				LPITEM acceCostume = GetWear(WEAR_COSTUME_WING);
				if (acceCostume && !UnequipItem(acceCostume))
				{
					NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
					return false;
				}

				if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
					return UnequipItem(item);
				break;
			}
#endif
			default:
				break;
		}
	}

	if (DestCell.IsEquipPosition())
	{
#ifdef ENABLE_NEW_PET_SYSTEM
		if (item->GetVnum() == 55701)
			return false;
#endif
		if (GetItem(DestCell))
		{
			NewChatPacket(FULL_SOCKET_WARNING);
			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}

		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;

		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
			!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
			item2->GetVnum() == item->GetVnum())
		{

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
			if (item->IsBlendItem() && item2->IsBlendItem())
			{
				if (count == 0)
					count = item->GetCount();

				for (int i = 0; i < 2; ++i)
				{
					// 0 - apply_type
					// 1 - apply_value
					if (item2->GetSocket(i) != item->GetSocket(i))
						return false;
				}

				item2->SetSocket(2, item2->GetSocket(2) + item->GetSocket(2));
				item->SetCount(item->GetCount() - count);
				return true;
			}
#endif

			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			count = MIN(g_bItemCountLimit - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);

			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
			return false;

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			item->RemoveFromCharacter();
#ifdef ENABLE_HIGHLIGHT_SYSTEM
			SetItem(DestCell, item, true);
#else
			SetItem(DestCell, item);
#endif
			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			FN_copy_item_socket(item2, item);

#ifdef ENABLE_HIGHLIGHT_SYSTEM
#ifdef ENABLE_SPLIT_ITEMS_FAST // lightwork fixed highlight after split items.
			if (isSplitItems)
				item2->AddToCharacter(this, DestCell, false);
			else
				item2->AddToCharacter(this, DestCell);
#else
			item2->AddToCharacter(this, DestCell);
#endif
#else
			item2->AddToCharacter(this, DestCell);
#endif
		}
	}
	return true;
}

namespace NPartyPickupDistribute
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
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch != c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);
				}
		}
	};
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::GiveGold(int64_t iAmount)
#else
void CHARACTER::GiveGold(int iAmount)
#endif
{
	if (iAmount <= 0)
		return;

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

#ifdef ENABLE_EXTENDED_YANG_LIMIT
		int64_t dwTotal = iAmount;
		int64_t dwMyAmount = dwTotal;
#else
		DWORD dwTotal = iAmount;
		DWORD dwMyAmount = dwTotal;
#endif

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);

#ifdef ENABLE_EXTENDED_BATTLE_PASS
		UpdateExtBattlePassMissionProgress(YANG_COLLECT, dwMyAmount, GetMapIndex());
#endif
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		UpdateExtBattlePassMissionProgress(YANG_COLLECT, iAmount, GetMapIndex());
#endif
	}
}

bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		// @fixme150 BEGIN
		if (item->GetType() == ITEM_QUEST)
		{
			if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			{
				NewChatPacket(CLOSE_OTHER_WINDOWS_TO_DO_THIS);
				return false;
			}
		}
		// @fixme150 END

		if (item->IsOwnership(this))
		{
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				M2_DESTROY_ITEM(item);

				Save();
			}

			else
			{
#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
				if (item->IsBlendItem() && !item->IsExtendedBlend(item->GetVnum()))
				{
					MAX_COUNT bCount = item->GetCount();

					for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->IsBlendItem() && item->IsBlendItem())
						{
							bool bSameBlend = true;
							for (int j = 0; j < 2; ++j)
							{
								if (item2->GetSocket(j) != item->GetSocket(j))
								{
									bSameBlend = false;
									break;
								}
							}

							if (bSameBlend)
							{
								item2->SetSocket(2, item2->GetSocket(2) + item->GetSocket(2));
								item->SetCount(item->GetCount() - bCount);
								item->RemoveFromGround();

								M2_DESTROY_ITEM(item);
								return true;
							}
						}
					}
				}
				else if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#else
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
#endif
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_05, "%s", item2->GetName());
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST)
									quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
								return true;
							}
						}
					}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif
					item->SetCount(bCount);


				}

#ifdef ENABLE_SPECIAL_STORAGE
				if (item->IsUpgradeItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetUpgradeInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_01, "%s", item2->GetName());
								M2_DESTROY_ITEM(item);
								return true;
							}
						}
					}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif

					item->SetCount(bCount);

				}
				else if (item->IsBook() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetBookInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_02, "%s", item2->GetName());
								M2_DESTROY_ITEM(item);

								return true;
							}
						}
					}
#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif
					item->SetCount(bCount);

				}
				else if (item->IsStone() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetStoneInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_03, "%s", item2->GetName());
								M2_DESTROY_ITEM(item);

								return true;
							}
						}
					}
#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif
					item->SetCount(bCount);

				}
				else if (item->IsChest() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bCount = item->GetCount();
#else
					BYTE bCount = item->GetCount();
#endif

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetChestInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
							MAX_COUNT bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#else
							BYTE bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
#endif

							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_04, "%s", item2->GetName());
								M2_DESTROY_ITEM(item);


								return true;
							}
						}
					}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					if (item->IsOwnership(this))
						UpdateExtBattlePassMissionProgress(BP_ITEM_COLLECT, bCount, item->GetVnum());
#endif

					item->SetCount(bCount);

				}
#endif

				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
				else if (item->IsBook())
				{
					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
				else if (item->IsStone())
				{
					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
				else if (item->IsChest())
				{
					if ((iEmptyCell = GetEmptyChestInventory(item)) == -1)
					{
						NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
#endif
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}

				item->RemoveFromGround();

				if (item->IsDragonSoul())
				{
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
					NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());
				}
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
					NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_01, "%s", item->GetName());
				}
					
				else if (item->IsBook())
				{
					item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, iEmptyCell));
					NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_02, "%s", item->GetName());
				}
				else if (item->IsStone())
				{
					item->AddToCharacter(this, TItemPos(STONE_INVENTORY, iEmptyCell));
					NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_03, "%s", item->GetName());
				}
				else if (item->IsChest())
				{
					item->AddToCharacter(this, TItemPos(CHEST_INVENTORY, iEmptyCell));
					NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_04, "%s", item->GetName());
				}
#endif
				else
				{
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
					NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_05, "%s", item->GetName());
				}

				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem(GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
		{
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
			// @fixme115
			if (!owner)
				return false;

			int iEmptyCell;

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
			}
#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyUpgradeInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						owner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
			}
			else if (item->IsBook())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyBookInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						owner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
			}
			else if (item->IsStone())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyStoneInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						owner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
			}
			else if (item->IsChest())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyChestInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyChestInventory(item)) == -1)
					{
						owner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
			}
#endif

			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
			{
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
				NewChatPacket(YOU_GAINED_ITEM, "%s", item->GetName());
			}
#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
			{
				item->AddToCharacter(owner, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
				NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_01, "%s", item->GetName());
			}

			else if (item->IsBook())
			{
				item->AddToCharacter(owner, TItemPos(BOOK_INVENTORY, iEmptyCell));
				NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_02, "%s", item->GetName());
			}
			else if (item->IsStone())
			{
				item->AddToCharacter(owner, TItemPos(STONE_INVENTORY, iEmptyCell));
				NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_03, "%s", item->GetName());
			}
			else if (item->IsChest())
			{
				item->AddToCharacter(owner, TItemPos(CHEST_INVENTORY, iEmptyCell));
				NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_04, "%s", item->GetName());
			}
#endif
			else
			{
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));
				NewChatPacket(YOU_GAINED_ITEM_INV_TYPE_05, "%s", item->GetName());
			}

			if (owner != this)
			{
				sys_err("WTF3162 item %s owner %s this %s", item->GetName(), owner->GetName(), GetName());
			}

			if (item->GetType() == ITEM_QUEST)
				quest::CQuestManager::instance().PickupItem(owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(CELL_UINT bCell, CELL_UINT bDestCell)
{
	if (!CanHandleItem())
		return false;
	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	if (bCell == bDestCell)
		return false;

	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;

	if (item1 == item2)
	{
		return false;
	}

	if (!IsEmptyItemGrid(TItemPos(INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		BYTE bInvenCell = item1->GetCell();

		if (item2->IsDragonSoul() || item2->GetType() == ITEM_BELT) // @fixme117
		{
			if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
				return false;
		}

		if (bEquipCell != item1->FindEquipCell(this))
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
		{
#ifdef ENABLE_HIGHLIGHT_SYSTEM
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell), false);
#else
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
#endif
			item2->ModifyPoints(false); //item_swap fix ds_aim //lightwork17 fix start
			ComputePoints();			// item_swap fix ds_aim //lightwork17 end
		}
		else
		{
			sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
		}
	}
	else
	{
		CELL_UINT bCell1 = item1->GetCell();
		CELL_UINT bCell2 = item2->GetCell();

		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

#ifdef ENABLE_HIGHLIGHT_SYSTEM
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2), false);
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1), false);
#else
		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
#endif
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	int iWearCell = item->FindEquipCell(this);
	if (iWearCell == WEAR_WEAPON)
	{
		LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
		if (costumeWeapon && !UnequipItem(costumeWeapon))
		{
			NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
			return false;
		}
	}
#endif
	else if (iWearCell == WEAR_BODY)
	{
		LPITEM costumeBody = GetWear(WEAR_COSTUME_BODY);
		if (costumeBody && !UnequipItem(costumeBody))
		{
			NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
			return false;
		}
	}

#ifdef ENABLE_MOUNT_SKIN
	if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_MOUNT)//binek skin takiliysa normal binegi c�rakaramaz
	{
		LPITEM pkMountSkin = GetWear(WEAR_MOUNT_SKIN);

		if (pkMountSkin)
		{
			NewChatPacket(UNEQUIP_YOUR_SKIN_FIRST);
			return false;
		}
	}
#endif
#ifdef ENABLE_PET_SKIN
	else if (item->GetType() == ITEM_PET)//pet skin tak�l�ysa normal pet c�rakaramaz
	{
		LPITEM pkMountSkin = GetWear(WEAR_PET_SKIN);

		if (pkMountSkin)
		{
			NewChatPacket(UNEQUIP_YOUR_SKIN_FIRST);
			return false;
		}
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_ACCE)
	{
		LPITEM pkAcceSkin = GetWear(WEAR_COSTUME_WING);

		if (pkAcceSkin)
		{
			NewChatPacket(UNEQUIP_YOUR_SKIN_FIRST);
			return false;
		}
	}
#endif
#ifdef ENABLE_AURA_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_AURA)
	{
		LPITEM pkAuraSkin = GetWear(WEAR_AURA_SKIN);

		if (pkAuraSkin)
		{
			NewChatPacket(UNEQUIP_YOUR_SKIN_FIRST);
			return false;
		}
	}
#endif

	// Deleting the rings or thieves gloves when wanted to unequip.
	if (item->GetType() == ITEM_UNIQUE)
	{
		if (item->GetVnum() == UNIQUE_ITEM_DOUBLE_EXP ||
			item->GetVnum() == UNIQUE_ITEM_DOUBLE_EXP_PLUS ||
			item->GetVnum() == UNIQUE_ITEM_DOUBLE_ITEM ||
			item->GetVnum() == UNIQUE_ITEM_DOUBLE_ITEM_PLUS)
		{
			LPITEM wearItem = GetWear(WEAR_UNIQUE1);
			LPITEM wearItem2 = GetWear(WEAR_UNIQUE2);

			if (wearItem)
			{
				if (item->GetVnum() == wearItem->GetVnum())
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return true;
				}
			}
			if (wearItem2)
			{
				if (item->GetVnum() == wearItem2->GetVnum())
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return true;
				}
			}
		}
#ifdef ENABLE_NEW_PET_SYSTEM
		if (item->GetVnum() == 55140)
		{
			if (GetNewPetSystem())
				GetNewPetSystem()->SetExpRing(false);
		}
#endif
	}

	if (false == CanUnequipNow(item))
		return false;
		

	int pos;
	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	if (item->IsDragonSoul())
	{
#ifdef ENABLE_HIGHLIGHT_SYSTEM
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
#endif
	}
	else
#ifdef ENABLE_HIGHLIGHT_SYSTEM
		item->AddToCharacter(this, TItemPos(INVENTORY, pos), false);
#else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));
#endif

#ifdef ENABLE_MOUNT_SKIN
	if (item->GetType() == ITEM_MOUNT_SKIN)//binek skin cikinca normal binegi cagirir fonksyon sonunda kalmasi gerek
	{
		LPITEM pkMount = GetWear(WEAR_COSTUME_MOUNT);

		if (pkMount)
		{
			MountSummon(pkMount);
		}
#ifdef ENABLE_HIDE_COSTUME
		SetMountSkinHidden(false);
#endif
	}
#endif
#ifdef ENABLE_PET_SKIN
	else if (item->GetType() == ITEM_PET_SKIN)//pet skin cikinca normal pet cagirir fonksyon sonunda kalmasi gerek
	{
		LPITEM pkPet = GetWear(WEAR_PET);
		if (pkPet)
		{
			PetSummon(pkPet);
		}
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WING)
	{
		LPITEM pkAcce = GetWear(WEAR_COSTUME_ACCE);

		if (pkAcce)
		{
			SetPart(PART_ACCE, pkAcce->GetVnum());
			UpdatePacket();
		}
	}
#endif
#ifdef ENABLE_BOOSTER_ITEM
	else if (item->GetType() == ITEM_BOOSTER)
	{
		CalcuteBoosterSetBonus();
	}
#endif
	CheckMaximumPoints();

#ifdef ENABLE_POWER_RANKING
	CalcutePowerRank();
#endif

	return true;
}

bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{
	if (item->IsExchanging())
		return false;

	if (false == item->IsEquipable())
		return false;

	if (false == CanEquipNow(item))
		return false;

	int iWearCell = item->FindEquipCell(this, iCandidateCell);

	if (iWearCell < 0)
		return false;

	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		NewChatPacket(CANT_DO_THIS_WHILE_MOUNTING);
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		NewChatPacket(POLY_BLOCKED_WITH_THIS);
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		NewChatPacket(ONLY_OTHER_GENDER_CAN_USE_THIS);
		return false;
	}

#ifndef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsRideItem() && IsRiding())
	{
		NewChatPacket(YOU_ALREADY_RIDING_MOUNT);
		return false;
	}
#endif

	if (((GetParty() && GetParty()->GetLeader() != this) || !GetParty()) && (item->GetVnum() == UNIQUE_ITEM_PARTY_BONUS_EXP || item->GetVnum() == 71012 || item->GetVnum() == 72043 || item->GetVnum() == 72044 || item->GetVnum() == 72045 || item->GetVnum() == 76011))
	{
		NewChatPacket(ONLY_GROUP_LEADER_CAN_USE_THIS);
		return false; /* lightwork correction mert party fix*/
	}

#ifdef ENABLE_COSTUME_RELATED_FIXES
	if (GetWear(WEAR_BODY) && GetWear(WEAR_BODY)->GetVnum() >= 11901 && GetWear(WEAR_BODY)->GetVnum() <= 11904 && item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_BODY)
	{
		NewChatPacket(CANT_USE_THIS_WITH_COSTUME);
		return false;
	}

	if (GetWear(WEAR_COSTUME_BODY) && item->GetVnum() >= 11901 && item->GetVnum() <= 11904)
	{
		NewChatPacket(CANT_USE_COSTUME_WITH_THIS);
		return false;
	}
#endif

	DWORD dwCurTime = get_dword_time();

	if (iWearCell != WEAR_ARROW
		&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		NewChatPacket(YOU_HAVE_TO_STAND_STILL_TO_DO_THIS);
		return false;
	}

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (iWearCell == WEAR_WEAPON)
	{
		if (item->GetType() == ITEM_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && costumeWeapon->GetValue(3) != item->GetSubType() && !UnequipItem(costumeWeapon))
			{
				NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
				return false;
			}
		}
		else //fishrod/pickaxe
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
				return false;
			}
		}
	}
	else if (iWearCell == WEAR_COSTUME_WEAPON)
	{
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);

			if (!pkWeapon || pkWeapon->GetType() != ITEM_WEAPON || item->GetValue(3) != pkWeapon->GetSubType())
			{
				NewChatPacket(WRONG_EQUIPMENT_MATCH);
				return false;
			}
		}
	}
#endif

#ifdef ENABLE_MOUNT_SKIN
	if (item->GetType() == ITEM_MOUNT_SKIN)//normal binek takili degilse binek skin takilmaz
	{
		LPITEM pkWearMount = GetWear(WEAR_COSTUME_MOUNT);
		if (!pkWearMount)
		{
			NewChatPacket(CANT_DO_THIS_IF_NOT_WEAR_MOUNT);
			return false;
		}
	}
#endif
#ifdef ENABLE_PET_SKIN
	else if (item->GetType() == ITEM_PET_SKIN)//normal pet tak�l� de�ilse pet skin tak�lmaz
	{
		LPITEM pkWearPet = GetWear(WEAR_PET);
		if (!pkWearPet)
		{
			NewChatPacket(CANT_DO_THIS_IF_NOT_WEAR_PET);
			return false;
		}
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
	else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WING)
	{
		LPITEM pkWearAcce = GetWear(WEAR_COSTUME_ACCE);
		if (!pkWearAcce)
		{
			NewChatPacket(CANT_DO_THIS_IF_NOT_WEAR_SASH);
			return false;
		}
	}
#endif
#ifdef ENABLE_AURA_SKIN
	else if (item->GetType() == ITEM_AURA_SKIN)
	{
		LPITEM pkWearAura = GetWear(WEAR_COSTUME_AURA);
		if (!pkWearAura)
		{
			NewChatPacket(CANT_DO_THIS_IF_NOT_WEAR_AURA);
			return false;
		}
	}
#endif

	if (item->IsDragonSoul())
	{
		if (GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			NewChatPacket(ALREADY_WEAR_SAME_TYPE_DS);
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}

	else
	{
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			if (false == SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			BYTE bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

#ifdef ENABLE_NEW_PET_SYSTEM
	if (item->GetVnum() == 55140)
	{
		if (GetNewPetSystem())
			GetNewPetSystem()->SetExpRing(true);
	}
#endif

	if (true == item->IsEquipped())
	{
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			if (0 == item->GetSocket(1))
			{
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[(unsigned char)(item->GetProto()->cLimitRealTimeFirstUseIndex)].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_ACCE)
			this->EffectPacket(SE_EFFECT_ACCE_EQUIP);
#endif

		if (item->IsNewMountItem()) // @fixme152
			quest::CQuestManager::instance().SIGUse(GetPlayerID(), quest::QUEST_NO_NPC, item, false);
	}

#ifdef ENABLE_ITEMS_SHINING
	if (item->GetType() == ITEM_SHINING)
	{
		this->UpdatePacket();
	}	
#endif
#ifdef ENABLE_BOOSTER_ITEM
	else if (item->GetType() == ITEM_BOOSTER)
	{
		CalcuteBoosterSetBonus();
	}
#endif
#ifdef ENABLE_POWER_RANKING
	CalcutePowerRank();
#endif

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints) / sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints) / sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if (0 == bOldValue)
	{
		CBuffOnAttributes* pBuff = NULL;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
			{
				static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
				static std::vector <BYTE> vec_slots(abSlot, abSlot + _countof(abSlot));
				pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
			}
			break;
			case POINT_COSTUME_ATTR_BONUS:
			{
				static BYTE abSlot[] = {
					WEAR_COSTUME_BODY,
					WEAR_COSTUME_HAIR,
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
					WEAR_COSTUME_MOUNT,
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
					WEAR_COSTUME_WEAPON,
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
					WEAR_COSTUME_WING,
#endif
				};
				static std::vector <BYTE> vec_slots(abSlot, abSlot + _countof(abSlot));
				pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
			}
			break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));
		}
		else
			pBuff = it->second;
		if (pBuff != NULL)
			pBuff->On(bNewValue);
	}
	else
	{
		assert(m_map_buff_on_attrs.end() != it);
		it->second->ChangeBuffValue(bNewValue);
	}
}

LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);

	return NULL;
}

LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i = BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
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

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		sys_err("item %d 's owner exists!", item->GetID());
		return;
	}

	int cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
	{
		cell = GetEmptyUpgradeInventory(item);
	}
	else if (item->IsBook())
	{
		cell = GetEmptyBookInventory(item);
	}
	else if (item->IsStone())
	{
		cell = GetEmptyStoneInventory(item);
	}
	else if (item->IsChest())
	{
		cell = GetEmptyChestInventory(item);
	}
#endif
	else
	{
		cell = GetEmptyInventory(item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, cell));
		else if (item->IsBook())
			item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, cell));
		else if (item->IsStone())
			item->AddToCharacter(this, TItemPos(STONE_INVENTORY, cell));
		else if (item->IsChest())
			item->AddToCharacter(this, TItemPos(CHEST_INVENTORY, cell));
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif

		if (longOwnerShip)
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
	}
}


bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		NewChatPacket(CLOSE_OTHER_WINDOWS_TO_DO_THIS);
		return false;
	}
	// @fixme150 END

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
	case fishing::CAMPFIRE_MOB:
		if (item->GetType() == ITEM_FISH &&
			(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
			return true;
		break;

	case fishing::FISHER_MOB:
		if (item->GetType() == ITEM_ROD)
			return true;
		break;

		// BUILDING_NPC
	case BLACKSMITH_WEAPON_MOB:
	case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		if (item->GetType() == ITEM_WEAPON &&
			item->GetRefinedVnum())
			return true;
		else
			return false;
		break;

	case BLACKSMITH_ARMOR_MOB:
	case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		if (item->GetType() == ITEM_ARMOR &&
			(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
			item->GetRefinedVnum())
			return true;
		else
			return false;
		break;

	case BLACKSMITH_ACCESSORY_MOB:
	case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
		if (item->GetType() == ITEM_ARMOR &&
			!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
			item->GetRefinedVnum())
			return true;
		else
			return false;
		break;
		// END_OF_BUILDING_NPC

	case BLACKSMITH_MOB:
		if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
		{
			return true;
		}
		else
		{
			return false;
		}

	case BLACKSMITH2_MOB:
		if (item->GetRefineSet() >= 500)
		{
			return true;
		}
		else
		{
			return false;
		}

	case ALCHEMIST_MOB:
		if (item->GetRefinedVnum())
			return true;
		break;

	case 20101:
	case 20102:
	case 20103:

		if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
		{
			if (!IsDead())
			{
				from->NewChatPacket(CANT_FEED_HORSE_WITH_DRUGS);
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
		{
			if (IsDead())
			{
				from->NewChatPacket(CANT_FEED_DEAD_HORSE);
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			return false;
		}
		break;
	case 20104:
	case 20105:
	case 20106:

		if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
		{
			if (!IsDead())
			{
				from->NewChatPacket(CANT_FEED_HORSE_WITH_DRUGS);
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
		{
			if (IsDead())
			{
				from->NewChatPacket(CANT_FEED_DEAD_HORSE);
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			return false;
		}
		break;
	case 20107:
	case 20108:
	case 20109:

		if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
		{
			if (!IsDead())
			{
				from->NewChatPacket(CANT_FEED_HORSE_WITH_DRUGS);
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			if (IsDead())
			{
				from->NewChatPacket(CANT_FEED_DEAD_HORSE);
				return false;
			}
			return true;
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
		{
			return false;
		}
		break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
	case fishing::CAMPFIRE_MOB:
		if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
			fishing::Grill(from, item);
		else
		{
			// TAKE_ITEM_BUG_FIX
			from->SetQuestNPCID(GetVID());
			// END_OF_TAKE_ITEM_BUG_FIX
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
		}
		break;

		// DEVILTOWER_NPC
	case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
	case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
	case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
		if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
		{
			from->SetRefineNPC(this);
			from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
		}
		else
		{
			from->NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		}
		break;
		// END_OF_DEVILTOWER_NPC

	case BLACKSMITH_MOB:
	case BLACKSMITH2_MOB:
	case BLACKSMITH_WEAPON_MOB:
	case BLACKSMITH_ARMOR_MOB:
	case BLACKSMITH_ACCESSORY_MOB:
		if (item->GetRefinedVnum())
		{
			from->SetRefineNPC(this);
			from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
		}
		else
		{
			from->NewChatPacket(CANT_UPGRADE_THIS_ITEM);
		}
		break;

	case 20101:
	case 20102:
	case 20103:
	case 20104:
	case 20105:
	case 20106:
	case 20107:
	case 20108:
	case 20109:
		if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
			item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
			item->GetVnum() == ITEM_REVIVE_HORSE_3)
		{
			from->ReviveHorse();
			item->SetCount(item->GetCount() - 1);
			from->NewChatPacket(HORSE_FEED_TALK_01);
		}
		else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
			item->GetVnum() == ITEM_HORSE_FOOD_2 ||
			item->GetVnum() == ITEM_HORSE_FOOD_3)
		{
			from->FeedHorse();
			from->NewChatPacket(HORSE_FEED_TALK_02);
			item->SetCount(item->GetCount() - 1);
			EffectPacket(SE_HPUP_RED);
		}
		break;

	default:
		from->SetQuestNPCID(GetVID());
		quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
		break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	{
		LPITEM u = GetWear(WEAR_COSTUME_MOUNT);
		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}
#endif

	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum)
			return true;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	{
		LPITEM u = GetWear(WEAR_COSTUME_MOUNT);

		if (u && u->GetSpecialGroup() == (int)dwGroupVnum)
			return true;
	}
#endif

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC(NULL);
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD>&dwItemVnums,
	std::vector<DWORD>&dwItemCounts, std::vector <LPITEM>&item_gets, int& count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
		case CSpecialItemGroup::GOLD:
			PointChange(POINT_GOLD, dwCount);
			bSuccess = true;
			break;

		case CSpecialItemGroup::EXP:
		{
			PointChange(POINT_EXP, dwCount);
			bSuccess = true;
		}
		break;

		case CSpecialItemGroup::MOB:
		{
			int x = GetX() + number(-500, 500);
			int y = GetY() + number(-500, 500);

			LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
			if (ch)
				ch->SetAggressive();
			bSuccess = true;
		}
		break;
		case CSpecialItemGroup::SLOW:
		{
			AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
			bSuccess = true;
		}
		break;
		case CSpecialItemGroup::DRAIN_HP:
		{
			HP_LL iDropHP = GetMaxHP() * dwCount / 100;
			iDropHP = MIN(iDropHP, GetHP() - 1);
			PointChange(POINT_HP, -iDropHP);
			bSuccess = true;
		}
		break;
		case CSpecialItemGroup::POISON:
		{
			AttackedByPoison(NULL);
			bSuccess = true;
		}
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case CSpecialItemGroup::BLEEDING:
		{
			AttackedByBleeding(NULL);
			bSuccess = true;
		}
		break;
#endif
		case CSpecialItemGroup::MOB_GROUP:
		{
			int sx = GetX() - number(300, 500);
			int sy = GetY() - number(300, 500);
			int ex = GetX() + number(300, 500);
			int ey = GetY() + number(300, 500);
			CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

			bSuccess = true;
		}
		break;
		default:
		{
			item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

			if (item_get)
			{
				bSuccess = true;
			}
		}
		break;
		}

		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;
		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		NewChatPacket(YOU_HAVE_LOWER_LEVEL_THAN_ITEM);
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
	case JOB_WARRIOR:
		hair -= 72000;
		break;

	case JOB_ASSASSIN:
		hair -= 71250;
		break;

	case JOB_SURA:
		hair -= 70500;
		break;

	case JOB_SHAMAN:
		hair -= 69750;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
		break;
#endif
	default:
		return false;
		break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		NewChatPacket(YOU_ALREADY_HAVE_THIS_HAIRSTLE);
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
	if (IsPolymorphed())
	{
		NewChatPacket(YOU_ALREADY_POLYMORPHED);
		return false;
	}

	if (true == IsRiding())
	{
		NewChatPacket(POLY_BLOCKED_WITH_THIS);
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		NewChatPacket(THIS_ITEM_CANT_BE_USED);
		item->SetCount(item->GetCount() - 1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		NewChatPacket(THIS_ITEM_CANT_BE_USED);
		item->SetCount(item->GetCount() - 1);
		return false;
	}

	switch (item->GetVnum())
	{
	case 70104:
	case 70105:
	case 70106:
	case 70107:
	case 71093:
	{
		if (IsRiding() || IsDead())
		{
			NewChatPacket(YOU_CANT_POLYMORPH);
			return false;
		}

		int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
		if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
		{
			NewChatPacket(CANT_POLYMORPH_HIGHER_LEVEL);
			return false;
		}

		int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID) / 40 * 25));
		iDuration *= 60;

		DWORD dwBonus = 0;

		dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID) / 40) * 100;

		AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
		AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);

		item->SetCount(item->GetCount() - 1);
	}
	break;

	case 50322:
	{
		if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
		{
			CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
		}
	}
	break;

	default:
		sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
		return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i = 0; i < sizeof(stunSkills) / sizeof(DWORD); ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);

	if (NULL != pAffect)
	{
		if (pAffect->lSPCost == 0)
			return;

		int32_t amount = 0;

		if (AFFECT_AUTO_HP_RECOVERY == type)
		{
			amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
		}
		else if (AFFECT_AUTO_SP_RECOVERY == type)
		{
			amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
		}

		if (amount > 0)
		{
			if (AFFECT_AUTO_HP_RECOVERY == type)
			{
				PointChange(POINT_HP_RECOVERY, amount);
				EffectPacket(SE_AUTO_HPUP);
			}
			else if (AFFECT_AUTO_SP_RECOVERY == type)
			{
				PointChange(POINT_SP_RECOVERY, amount);
				EffectPacket(SE_AUTO_SPUP);
			}
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	case BOOK_INVENTORY:
	case STONE_INVENTORY:
	case CHEST_INVENTORY:
		return cell < (SPECIAL_INVENTORY_MAX_NUM);
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		return cell < SWITCHBOT_SLOT_COUNT;
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	case BUFFI_INVENTORY:
		return cell < BUFFI_MAX_SLOT;
#endif
	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;

	default:
		return false;
	}
}

#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos & srcCell, const TItemPos & destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	//BYTE itemType = item->GetType();
	//BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
	case JOB_WARRIOR:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
			return false;
		break;

	case JOB_ASSASSIN:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
			return false;
		break;

	case JOB_SHAMAN:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
			return false;
		break;

	case JOB_SURA:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
			return false;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
		if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
			return false;
		break;
#endif
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
		case LIMIT_LEVEL:
			if (GetLevel() < limit)
			{
				NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
				return false;
			}
			break;

		case LIMIT_STR:
			if (GetPoint(POINT_ST) < limit)
			{
				NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
				return false;
			}
			break;

		case LIMIT_INT:
			if (GetPoint(POINT_IQ) < limit)
			{
				NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
				return false;
			}
			break;

		case LIMIT_DEX:
			if (GetPoint(POINT_DX) < limit)
			{
				NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
				return false;
			}
			break;

		case LIMIT_CON:
			if (GetPoint(POINT_HT) < limit)
			{
				NewChatPacket(YOU_DO_NOT_MEET_THE_NEEDS_OF_THIS_ITEM);
				return false;
			}
			break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)) ||

			(item->GetValue(1) &&
			((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->GetValue(1) == item->GetValue(1)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->GetValue(1) == item->GetValue(1)))) ||

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			(GetWear(WEAR_COSTUME_MOUNT) && GetWear(WEAR_COSTUME_MOUNT)->IsSameSpecialGroup(item))
			)
#endif
		{
			NewChatPacket(CANT_WEAR_THIS_SECOND_TIME);
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) &&
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			NewChatPacket(WEDDING_TEXT_1);
			return false;
		}
	}

#ifdef ENABLE_DS_SET_BONUS
	if ((DragonSoul_IsDeckActivated()) && (item->IsDragonSoul())) {
		NewChatPacket(DS_DEACTIVATE_REQ);
		return false;
	}
#endif

	return true;
}

bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos & srcCell, const TItemPos & destCell) /*const*/
{
	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "Need to empty your belt inventory to unequip the belt.");

	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

#ifdef ENABLE_DS_SET_BONUS
	if ((DragonSoul_IsDeckActivated()) && (item->IsDragonSoul())) {
		NewChatPacket(DS_DEACTIVATE_REQ);
		return false;
	}
#endif

	{
		int pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG(-1 == pos, "No empty space.");
	}



	return true;
}

#ifdef ENABLE_ITEM_TRANSACTIONS
void CHARACTER::ItemTransactionSell(std::vector<TItemTransactionsInfo>& v_item)
{
	int64_t totalPrice = 0;
	int64_t CharacterMoney = GetGold();

	for (const TItemTransactionsInfo& info : v_item)
	{
		LPITEM item = WindowTypeGetItem(info.pos, info.window);
		if (!item)
			continue;

		if (true == item->isLocked() || item->IsEquipped())
			continue;

		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
			continue;

		MAX_COUNT bCount = item->GetCount();
		int64_t itemPrice = item->GetShopBuyPrice();
		itemPrice *= bCount;

		const int64_t nTotalMoney = static_cast<int64_t>(CharacterMoney) + static_cast<int64_t>(itemPrice) + static_cast<int64_t>(totalPrice);
		if (GOLD_MAX <= nTotalMoney)
		{
			break;
		}
		totalPrice += itemPrice;
#ifdef ENABLE_ITEM_SELL_LOG
		TItemSellLog log;
		memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
		memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
		log.dwVnum = item->GetVnum();
		log.dwCount = item->GetCount();
		log.pid = GetPlayerID();
		snprintf(log.logType, sizeof(log.logType), "ITEM_SELL");
		DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif
		ITEM_MANAGER::instance().RemoveItem(item);
	}
	PointChange(POINT_GOLD, totalPrice, false);
}

void CHARACTER::ItemTransactionDelete(std::vector<TItemTransactionsInfo>& v_item)
{
	for (const TItemTransactionsInfo& info : v_item)
	{
		LPITEM item = WindowTypeGetItem(info.pos, info.window);
		if (!item)
			continue;

		if (true == item->isLocked() || item->IsEquipped())
			continue;

#ifdef ENABLE_ITEM_SELL_LOG
		TItemSellLog log;
		memcpy(&log.alSockets, item->GetSockets(), sizeof(log.alSockets));
		memcpy(&log.aAttr, item->GetAttributes(), sizeof(log.aAttr));
		log.dwVnum = item->GetVnum();
		log.dwCount = item->GetCount();
		log.pid = GetPlayerID();
		snprintf(log.logType, sizeof(log.logType), "ITEM_DELETE");
		DBManager::instance().DirectQuery(ItemSellLogQuery(log).c_str());
#endif

		ITEM_MANAGER::instance().RemoveItem(item);
	
	}
}

#endif

#ifdef ENABLE_JEWELS_RENEWAL
void CHARACTER::UseAddJewels(LPITEM item, LPITEM targetItem)
{
	if ((targetItem->IsAccessoryForSocket() && item->GetVnum() == 50621) || targetItem->CanPutIntoNew(item))
	{
		if (targetItem->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
		{
			if (number(1, 100) <= 50)
			{
				targetItem->SetAccessorySocketMaxGrade(targetItem->GetAccessorySocketMaxGrade() + 1);
				NewChatPacket(ADDING_SOCKET_SUCCESSFULL);
			}
			else
			{
				NewChatPacket(ADDING_SOCKET_UNSUCCESSFULL);
			}

			item->SetCount(item->GetCount() - 1);
		}
		else
		{
			NewChatPacket(SOCKET_NOT_ENOUGH_SPACE);
		}
	}
	else
	{
		NewChatPacket(SOCKET_CANT_BE_ADDED_TO_THIS_ITEM);
	}
}

void CHARACTER::UsePutIntoJewels(LPITEM item, LPITEM targetItem)
{
	int openSocketCount = targetItem->GetAccessorySocketGrade();
	bool isPermaEq = targetItem->IsPermaEquipment();

	bool isPermaJewels = item->isPermaJewels();
	

	if (targetItem->IsAccessoryForSocket() && item->CanPutInto(targetItem))//kolye kupe bilezik kemer
	{
		if (openSocketCount < targetItem->GetAccessorySocketMaxGrade())//cevher icin delik varsa
		{
			if (isPermaJewels)
			{
				if (openSocketCount != 0 && !isPermaEq)//cevher perma item perma degilse
				{
					NewChatPacket(PERMANENT_JEWEWL_INFO_01);
					return;
				}

				if (!isPermaEq)
					targetItem->SetSocket(2, 62);
			}
			else
			{
				if (openSocketCount != 0 && isPermaEq)//cevher perma değilse item perma ise
				{
					NewChatPacket(PERMANENT_JEWEWL_INFO_02);
					return;
				}
			}
		}
		else
		{
			if (targetItem->GetAccessorySocketMaxGrade() == ITEM_ACCESSORY_SOCKET_MAX_NUM)
				NewChatPacket(ADD_JEWEL_INFO_04);
			else
				NewChatPacket(ADD_JEWEL_INFO_01);
			return;
		}
	}
	else
	{
		if (!targetItem->CanPutIntoNew(item))
		{
			NewChatPacket(PERMANENT_JEWEWL_INFO_03);
			return;
		}

		if (openSocketCount < targetItem->GetAccessorySocketMaxGrade())//cevher icin delik varsa
		{
			if (isPermaJewels)
			{
				if (openSocketCount != 0 && !isPermaEq)//cevher perma item perma degilse
				{
					NewChatPacket(PERMANENT_JEWEWL_INFO_01);
					return;
				}

				if (!isPermaEq)
					targetItem->SetSocket(2, 62);
			}
			else
			{
				if (openSocketCount != 0 && isPermaEq)//cevher perma değilse item perma ise
				{
					NewChatPacket(PERMANENT_JEWEWL_INFO_02);
					return;
				}
			}
		}
		else
		{
			if (targetItem->GetAccessorySocketMaxGrade() == ITEM_ACCESSORY_SOCKET_MAX_NUM)
				NewChatPacket(ADD_JEWEL_INFO_04);
			else
				NewChatPacket(ADD_JEWEL_INFO_01);
			return;
		}
	}

	targetItem->SetAccessorySocketGrade(openSocketCount + 1);
	NewChatPacket(ADDING_JEWEL_SUCCESFULL);
	item->SetCount(item->GetCount() - 1);
}
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
void CHARACTER::CheckPartyBonus()
{
	if (!IsPC() || GetParty())
		return;

	int bonusID = GetQuestFlag("party.bonus_id");
	float k = (float) GetSkillPowerByLevel( MIN(SKILL_MAX_LEVEL, GetLeadershipSkillLevel() ) )/ 100.0f;
	
	if (bonusID)	
	{
		if(FindAffect(AFFECT_PARTY_BONUS))
		{
			RemoveAffect(AFFECT_PARTY_BONUS);
			AddAffect(AFFECT_PARTY_BONUS, bonusID, 0, 0, INFINITE_AFFECT_DURATION_REALTIME, 0, false);
		}
		else
			AddAffect(AFFECT_PARTY_BONUS, bonusID, 0, 0, INFINITE_AFFECT_DURATION_REALTIME, 0, false);
	}
	else
	{
		if(FindAffect(AFFECT_PARTY_BONUS))
			RemoveAffect(AFFECT_PARTY_BONUS);
	}

	switch (bonusID)
	{
		case PARTY_ROLE_ATTACKER:
			{
				int iBonus = (int) (10 + 60 * k);

				if (GetPoint(POINT_PARTY_ATTACKER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_ATTACKER_BONUS, iBonus - GetPoint(POINT_PARTY_ATTACKER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_TANKER:
			{
				int iBonus = (int) (50 + 1450 * k);

				if (GetPoint(POINT_PARTY_TANKER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_TANKER_BONUS, iBonus - GetPoint(POINT_PARTY_TANKER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_BUFFER:
			{
				int iBonus = (int) (5 + 45 * k);

				if (GetPoint(POINT_PARTY_BUFFER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_BUFFER_BONUS, iBonus - GetPoint(POINT_PARTY_BUFFER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_SKILL_MASTER:
			{
				int iBonus = (int) (10*k);

				if (GetPoint(POINT_PARTY_SKILL_MASTER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_SKILL_MASTER_BONUS, iBonus - GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
				}
			}
			break;
		case PARTY_ROLE_HASTE:
			{
				int iBonus = (int) (1+5*k);
				if (GetPoint(POINT_PARTY_HASTE_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_HASTE_BONUS, iBonus - GetPoint(POINT_PARTY_HASTE_BONUS));
				}
			}
			break;
		case PARTY_ROLE_DEFENDER:
			{
				int iBonus = (int) (5+30*k);
				if (GetPoint(POINT_PARTY_DEFENDER_BONUS) != iBonus)
				{
					PointChange(POINT_PARTY_DEFENDER_BONUS, iBonus - GetPoint(POINT_PARTY_DEFENDER_BONUS));
				}
			}
			break;
			
		default:
			break;
	}
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
void CHARACTER::Give67AttrItem()
{
	LPITEM item = GetAttrInventoryItem();
	if (!item)
		return;

	int emptyPos = GetEmptyInventory(item->GetSize());

	if (emptyPos == -1)
	{
		NewChatPacket(STRING_D42);
		return;
	}	

	item->RemoveFromCharacter();
	item->AddToCharacter(this, TItemPos(INVENTORY, emptyPos));
}

void CHARACTER::Change67Attr(LPITEM changer, LPITEM item)
{
	if (!changer || !item)
		return;

	if (item->Get67AttrIdx() == -1)
	{
		NewChatPacket(CANT_CHANGE_THE_ATTRIBUTE_OF_THIS_ITEM);
		return;
	}

	if (item->Get67AttrCount() != 2)
	{
		ChatPacket(1, "You can not do this, this item does not have 7 attributes.");
		return;
	}

	if (changer->GetVnum() == 72351)
	{
		if (number(0, 100) <= 20)
		{
			NewChatPacket(IT_WAS_FAILED);
			changer->SetCount(changer->GetCount() - 1);
			return;
		}
	}

	changer->SetCount(changer->GetCount() - 1);
	item->Change67Attr();
	NewChatPacket(IT_WAS_SUCCESFULL);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
}

void CHARACTER::Decrease67AttrCooldown(LPITEM cooldownItem)
{
	if (!cooldownItem)
		return;

	const int currentTime = get_global_time();
	const int endTime = GetQuestFlag("67attr.time");

	int remainingTime = endTime - currentTime;

	if (remainingTime <= 0)
	{
		NewChatPacket(STRING_D191);
		return;
	}
		
	switch (cooldownItem->GetVnum())
	{
		case 24601: // 25 percent item
		{
			remainingTime = remainingTime * 3 / 4;
			break;
		}
		case 24602:// 50 percent item
		{
			remainingTime = remainingTime / 2;
			break;
		}
	default:
		return;
	}

	cooldownItem->SetCount(cooldownItem->GetCount() - 1);
	SetQuestFlag("67attr.time", currentTime + remainingTime);
}

#endif

#ifdef ENABLE_BUFFI_SYSTEM
bool CHARACTER::CanBuffiEquipItem(TItemPos& cell, LPITEM item)
{
	int wearCell = item->FindBuffiEquipCell();
	if (wearCell == -1)
		return false;

	if (GetItem(TItemPos(BUFFI_INVENTORY, wearCell)))
		return false;

	if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
		return false;

	if (item->GetAntiFlag() & ITEM_ANTIFLAG_FEMALE)
		return false;

	if (item->GetVnum() == 11903 || item->GetVnum() == 11904 )
		return false;

	cell.cell = wearCell;
	return true;
}
#endif

#ifdef ENABLE_COSTUME_ATTR
void CHARACTER::ChangeCostumeAttr(LPITEM costume, LPITEM changer)
{
	if (changer->GetValue(0) == 1)
	{
		const int deleteIdx = changer->GetValue(1);
		if (costume->GetAttributeType(deleteIdx) == 0)
			return;

		costume->SetAttribute(deleteIdx, 0, 0);
		changer->SetCount(changer->GetCount() - 1);
	}
	else if (changer->GetValue(0) == 2)
	{
		const BYTE changeType = changer->GetValue(1);
		const BYTE subType = costume->GetSubType();

		if ((changeType == 1 && subType != COSTUME_HAIR) || (changeType == 2 && subType != COSTUME_BODY) || (changeType == 3 && subType != COSTUME_WEAPON))
			return;

		TItemApply changerBonus = changer->GetProto()->aApplies[0];
		int applyIdx = costume->FindAttribute(changerBonus.bType);
		if (changer->GetValue(2) == 1)
		{
			if (applyIdx == -1)
				return;

			if (costume->GetAttributeValue(applyIdx) == changerBonus.lValue)
				return;

			costume->SetAttribute(applyIdx, changerBonus.bType, changerBonus.lValue);
			changer->SetCount(changer->GetCount() - 1);
		}
		else
		{
			if (applyIdx != -1)
				return;

			applyIdx = costume->FindAttribute(0);
			if (applyIdx == -1)
				return;

			costume->SetAttribute(applyIdx, changerBonus.bType, changerBonus.lValue);
			changer->SetCount(changer->GetCount() - 1);
		}
	}
	else
	{
		return;
	}
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
}
#endif

#ifdef ENABLE_EVENT_MANAGER

void CHARACTER::ConvertEventItems(LPITEM item)
{
	if (!item)
		return;

	BYTE neededItemCount{};
	const DWORD itemVnum{ item->GetVnum() };
	DWORD giveItemVnum{};

	switch (item->GetVnum())
	{
	case 79505:
	{
		neededItemCount = 24;
		giveItemVnum = 79506;
		break;
	}	
	case 79512:
	{
		neededItemCount = 18;
		giveItemVnum = 79513;
		break;
	}	
	case 79603:
	{
		neededItemCount = 25;
		giveItemVnum = 79604;
		break;
	}	
	case 50096:
	{
		neededItemCount = 50;
		giveItemVnum = 50265;
		break;
	}

	default:
		return;
	}

	if (CountSpecifyItem(itemVnum) < neededItemCount)
	{
		NewChatPacket(NOT_ENOUGH_MATERIAL);
		return;
	}

	RemoveSpecifyItem(itemVnum, neededItemCount);
	AutoGiveItem(giveItemVnum, 1);
}

#endif