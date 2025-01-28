#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"
#include "packet.h"
#include "protocol.h"

#include "skill.h"
#include "unique_item.h"
#include "profiler.h"
#include "marriage.h"
#include "item_addon.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "affect.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/VnumHelper.h"
#include "../../common/Controls.h"
#include "war_map.h"
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
#include "mob_manager.h"
#endif

CItem::CItem(DWORD dwVnum)
	: m_dwVnum(dwVnum), m_bWindow(0), m_dwID(0), m_bEquipped(false), m_dwVID(0), m_wCell(0), m_dwCount(0), m_lFlag(0), m_dwLastOwnerPID(0),
	m_bExchanging(false), m_pkDestroyEvent(NULL), m_pkExpireEvent(NULL), m_pkUniqueExpireEvent(NULL),
	m_pkTimerBasedOnWearExpireEvent(NULL), m_pkRealTimeExpireEvent(NULL),
	m_pkAccessorySocketExpireEvent(NULL), m_pkOwnershipEvent(NULL),  /* ENABLE_EXTENDED_BLEND_AFFECT */ m_pkBlendUseEvent(NULL), /* ENABLE_EXTENDED_BLEND_AFFECT*/ m_dwOwnershipPID(0), m_bSkipSave(false), m_isLocked(false),
	m_dwMaskVnum(0), m_dwSIGVnum(0)
{
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));
}

CItem::~CItem()
{
	Destroy();
}

void CItem::Initialize()
{
	CEntity::Initialize(ENTITY_ITEM);

	m_bWindow = RESERVED_WINDOW;
	m_pOwner = NULL;
	m_dwID = 0;
	m_bEquipped = false;
	m_dwVID = m_wCell = m_dwCount = m_lFlag = 0;
	m_pProto = NULL;
	m_bExchanging = false;
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));

	m_pkDestroyEvent = NULL;
	m_pkOwnershipEvent = NULL;
	m_dwOwnershipPID = 0;
	m_pkUniqueExpireEvent = NULL;
	m_pkTimerBasedOnWearExpireEvent = NULL;
	m_pkRealTimeExpireEvent = NULL;
#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
	m_pkBlendUseEvent = NULL;
#endif

	m_pkAccessorySocketExpireEvent = NULL;

	m_bSkipSave = false;
	m_dwLastOwnerPID = 0;
}

void CItem::Destroy()
{
	event_cancel(&m_pkDestroyEvent);
	event_cancel(&m_pkOwnershipEvent);
	event_cancel(&m_pkUniqueExpireEvent);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);
	event_cancel(&m_pkRealTimeExpireEvent);
	event_cancel(&m_pkAccessorySocketExpireEvent);
#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
	event_cancel(&m_pkBlendUseEvent);
#endif

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);
}

EVENTFUNC(item_destroy_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("item_destroy_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetOwner())
		sys_err("item_destroy_event: Owner exist. (item %s owner %s)", pkItem->GetName(), pkItem->GetOwner()->GetName());

	pkItem->SetDestroyEvent(NULL);
	M2_DESTROY_ITEM(pkItem);
	return 0;
}

void CItem::SetDestroyEvent(LPEVENT pkEvent)
{
	m_pkDestroyEvent = pkEvent;
}

void CItem::StartDestroyEvent(int iSec)
{
	if (m_pkDestroyEvent)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetDestroyEvent(event_create(item_destroy_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::EncodeInsertPacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	const PIXEL_POSITION& c_pos = GetXYZ();

	struct packet_item_ground_add pack;

	pack.bHeader = HEADER_GC_ITEM_GROUND_ADD;
	pack.x = c_pos.x;
	pack.y = c_pos.y;
	pack.z = c_pos.z;
	pack.dwVnum = GetVnum();
	pack.dwVID = m_dwVID;
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = GetSocket(i);
	thecore_memcpy(pack.aAttrs, GetAttributes(), sizeof(pack.aAttrs));
#endif

	d->Packet(&pack, sizeof(pack));

	if (m_pkOwnershipEvent != NULL)
	{
		item_event_info* info = dynamic_cast<item_event_info*>(m_pkOwnershipEvent->info);

		if (info == NULL)
		{
			sys_err("CItem::EncodeInsertPacket> <Factor> Null pointer");
			return;
		}

		TPacketGCItemOwnership p;

		p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
		p.dwVID = m_dwVID;
		strlcpy(p.szName, info->szOwnerName, sizeof(p.szName));

		d->Packet(&p, sizeof(TPacketGCItemOwnership));
	}
}

void CItem::EncodeRemovePacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	struct packet_item_ground_del pack;

	pack.bHeader = HEADER_GC_ITEM_GROUND_DEL;
	pack.dwVID = m_dwVID;

	d->Packet(&pack, sizeof(pack));
}

void CItem::SetProto(const TItemTable* table)
{
	assert(table != NULL);
	m_pProto = table;
	SetFlag(m_pProto->dwFlags);
}

void CItem::UsePacketEncode(LPCHARACTER ch, LPCHARACTER victim, struct packet_item_use* packet)
{
	if (!GetVnum())
		return;

	packet->header = HEADER_GC_ITEM_USE;
	packet->ch_vid = ch->GetVID();
	packet->victim_vid = victim->GetVID();
	packet->Cell = TItemPos(GetWindow(), m_wCell);
	packet->vnum = GetVnum();
}

void CItem::RemoveFlag(long bit)
{
	REMOVE_BIT(m_lFlag, bit);
}

void CItem::AddFlag(long bit)
{
	SET_BIT(m_lFlag, bit);
}

void CItem::UpdatePacket()
{
	if (!m_pOwner || !m_pOwner->GetDesc())
		return;

#ifdef ENABLE_SWITCHBOT
	if (m_bWindow == SWITCHBOT)
		return;
#endif

	TPacketGCItemUpdate pack;

	pack.header = HEADER_GC_ITEM_UPDATE;
	pack.Cell = TItemPos(GetWindow(), m_wCell);
	pack.count = m_dwCount;

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = m_alSockets[i];

	thecore_memcpy(pack.aAttr, GetAttributes(), sizeof(pack.aAttr));

	m_pOwner->GetDesc()->Packet(&pack, sizeof(pack));
}

DWORD CItem::GetCount()
{
	if (GetType() == ITEM_ELK) return MIN(m_dwCount, INT_MAX);
	else
	{
		return MIN(m_dwCount, g_bItemCountLimit);
	}
}

bool CItem::SetCount(DWORD count)
{
	if (GetType() == ITEM_ELK)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
	else
	{
		m_dwCount = MIN(count, g_bItemCountLimit);
	}

	if (count == 0 && m_pOwner)
	{
		if (GetSubType() == USE_ABILITY_UP || GetSubType() == USE_POTION || GetVnum() == 70020)
		{
			LPCHARACTER pOwner = GetOwner();
			WORD wCell = GetCell();

			RemoveFromCharacter();

			if (!IsDragonSoul())
			{
				LPITEM pItem = pOwner->FindSpecifyItem(GetVnum());

				if (NULL != pItem)
				{
					pOwner->ChainQuickslotItem(pItem, QUICKSLOT_TYPE_ITEM, wCell);
				}
				else
				{
					pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wCell, UINT16_MAX);
				}
			}

			M2_DESTROY_ITEM(this);
		}
		else
		{
			if (!IsDragonSoul())
			{
				m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, m_wCell, UINT16_MAX);
			}
			M2_DESTROY_ITEM(RemoveFromCharacter());
		}

		return false;
	}

	UpdatePacket();
	Save();
	return true;
}

LPITEM CItem::RemoveFromCharacter()
{
	if (!m_pOwner)
	{
		sys_err("Item::RemoveFromCharacter owner null");
		return (this);
	}

	LPCHARACTER pOwner = m_pOwner;

	if (m_bEquipped)
	{
		Unequip();

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
	else
	{
		if (GetWindow() != SAFEBOX && GetWindow() != MALL)
		{
			if (IsDragonSoul())
			{
				if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= DRAGON_SOUL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
#ifdef ENABLE_SPECIAL_STORAGE
			else if (IsUpgradeItem() || IsBook() || IsStone() || IsChest())
			{
				if (m_wCell >= SPECIAL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= SPECIAL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
#endif
#ifdef ENABLE_SWITCHBOT
			else if (m_bWindow == SWITCHBOT)
			{
				if (m_wCell >= SWITCHBOT_SLOT_COUNT)
				{
					sys_err("CItem::RemoveFromCharacter: pos >= SWITCHBOT_SLOT_COUNT");
				}
				else
				{
					pOwner->SetItem(TItemPos(SWITCHBOT, m_wCell), NULL);
				}
			}
#endif
#ifdef ENABLE_6TH_7TH_ATTR
			else if (GetWindow() == ATTR_INVENTORY)
			{
				pOwner->SetItem(TItemPos(ATTR_INVENTORY, 0), nullptr);
			}
#endif
#ifdef ENABLE_BUFFI_SYSTEM
			else if (m_bWindow == BUFFI_INVENTORY)
			{
				if (m_wCell >= BUFFI_MAX_SLOT)
				{
					sys_err("CItem::RemoveFromCharacter: pos >= BUFFI_INVENTORY");
				}
				else
				{
					pOwner->SetItem(TItemPos(BUFFI_INVENTORY, m_wCell), NULL);
				}
			}
#endif
			else
			{
				TItemPos cell(INVENTORY, m_wCell);

				if (false == cell.IsDefaultInventoryPosition() && false == cell.IsBeltInventoryPosition())
					sys_err("CItem::RemoveFromCharacter: Invalid Item Position");
				else
				{
					pOwner->SetItem(cell, NULL);
				}
			}
		}

		m_pOwner = NULL;
		m_wCell = 0;

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
}

bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell
#ifdef ENABLE_HIGHLIGHT_SYSTEM
	, bool isHighLight
#endif
)
{
	assert(GetSectree() == NULL);
	assert(m_pOwner == NULL);

	WORD pos = Cell.cell;
	BYTE window_type = Cell.window_type;

	if (INVENTORY == window_type)
	{
		if (m_wCell >= INVENTORY_MAX_NUM && BELT_INVENTORY_SLOT_START > m_wCell)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
	else if (DRAGON_SOUL_INVENTORY == window_type)
	{
		if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (UPGRADE_INVENTORY == window_type || BOOK_INVENTORY == window_type || STONE_INVENTORY == window_type || CHEST_INVENTORY == window_type)
	{
		if (m_wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

#ifdef ENABLE_SWITCHBOT
	else if (SWITCHBOT == window_type)
	{
		if (m_wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CItem::AddToCharacter:switchbot cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	else if (BUFFI_INVENTORY == window_type)
	{
		if (m_wCell >= BUFFI_MAX_SLOT)
		{
			sys_err("CItem::AddToCharacter: BuffEquipment cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

	if (ch->GetDesc())
		m_dwLastOwnerPID = ch->GetPlayerID();

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_ACCE) && (GetSocket(ACCE_ABSORPTION_SOCKET) == 0))
	{
		long lVal = GetValue(ACCE_GRADE_VALUE_FIELD);
		switch (lVal)
		{
		case 2:
			lVal = ACCE_GRADE_2_ABS;
			break;
		case 3:
			lVal = ACCE_GRADE_3_ABS;
			break;
		case 4:
			lVal = number(ACCE_GRADE_4_ABS_MIN, ACCE_GRADE_4_ABS_MAX_COMB);
			break;
		default:
			lVal = ACCE_GRADE_1_ABS;
			break;
		}
		SetSocket(ACCE_ABSORPTION_SOCKET, lVal);
	}
#endif

#ifdef ENABLE_AURA_SYSTEM
	if ((GetType() == ITEM_COSTUME && GetSubType() == COSTUME_AURA) && (GetSocket(AURA_ABSORPTION_SOCKET) == 0))
	{
		long lVal = GetValue(AURA_GRADE_VALUE_FIELD);
		if (lVal < 1)
		{
			SetSocket(AURA_GRADE_VALUE_FIELD, 1);
			SetSocket(AURA_ABSORPTION_SOCKET, 1);
		}
		else
			SetSocket(AURA_ABSORPTION_SOCKET, lVal);
	}
#endif

	event_cancel(&m_pkDestroyEvent);

	ch->SetItem(TItemPos(window_type, pos), this
#ifdef ENABLE_HIGHLIGHT_SYSTEM
		, isHighLight
#endif
);

	m_pOwner = ch;

	Save();
	return true;
}

LPITEM CItem::RemoveFromGround()
{
	if (GetSectree())
	{
		SetOwnership(NULL);

		GetSectree()->RemoveEntity(this);

		ViewCleanup();

		Save();
	}

	return (this);
}

bool CItem::AddToGround(long lMapIndex, const PIXEL_POSITION& pos, bool skipOwnerCheck)
{
	if (0 == lMapIndex)
	{
		sys_err("wrong map index argument: %d", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		sys_err("sectree already assigned");
		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		sys_err("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d", pos.x, pos.y);
		return false;
	}

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}

bool CItem::DistanceValid(LPCHARACTER ch)
{
	if (!GetSectree())
		return false;

	int iDist = DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY());

	if (iDist > 600) // @fixme173 300 to 600
		return false;

	return true;
}

bool CItem::CanUsedBy(LPCHARACTER ch)
{
	// Anti flag check
	switch (ch->GetJob())
	{
	case JOB_WARRIOR:
		if (GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
			return false;
		break;

	case JOB_ASSASSIN:
		if (GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
			return false;
		break;

	case JOB_SHAMAN:
		if (GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
			return false;
		break;

	case JOB_SURA:
		if (GetAntiFlag() & ITEM_ANTIFLAG_SURA)
			return false;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case JOB_WOLFMAN:
		if (GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
			return false;
		break;
#endif
	}

	return true;
}

int CItem::FindEquipCell(LPCHARACTER ch, int iCandidateCell)
{
	switch (GetType())
	{
		case ITEM_DS:
		case ITEM_SPECIAL_DS:
		{
			if (iCandidateCell < 0)
			{
				return WEAR_MAX_NUM + GetSubType();
			}
			else
			{
				for (int i = 0; i < DRAGON_SOUL_DECK_MAX_NUM; i++)
				{
					if (WEAR_MAX_NUM + i * DS_SLOT_MAX + GetSubType() == iCandidateCell)
					{
						return iCandidateCell;
					}
				}
				return -1;
			}
			break;
		}//ITEM_DS

		case ITEM_COSTUME:
		{
			switch (GetSubType())
			{
				case COSTUME_BODY: { return WEAR_COSTUME_BODY; }
				case COSTUME_HAIR: { return WEAR_COSTUME_HAIR; }
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				case COSTUME_MOUNT: { return WEAR_COSTUME_MOUNT; }
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
				case COSTUME_ACCE: { return WEAR_COSTUME_ACCE; }
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				case COSTUME_WEAPON: { return WEAR_COSTUME_WEAPON; }
#endif
#ifdef ENABLE_AURA_SYSTEM
				case COSTUME_AURA: { return WEAR_COSTUME_AURA; }
#endif
#ifdef ENABLE_ACCE_COSTUME_SKIN
				case COSTUME_WING: { return WEAR_COSTUME_WING; }
#endif
				default: break;
			}
			break;
		}//ITEM_COSTUME

		case ITEM_BELT:
		{
			return WEAR_BELT;
		}
#ifdef ENABLE_PET_COSTUME_SYSTEM
		case ITEM_PET:
		{
			return WEAR_PET;
		}
#endif

#ifdef ENABLE_MOUNT_SKIN
		case ITEM_MOUNT_SKIN:
		{
			return WEAR_MOUNT_SKIN;
		}
#endif

#ifdef ENABLE_PET_SKIN
		case ITEM_PET_SKIN:
		{
			return WEAR_PET_SKIN;
		}
#endif

#ifdef ENABLE_CROWN_SYSTEM
		case ITEM_CROWN:
		{
			return WEAR_CROWN;
		}
#endif

#ifdef ENABLE_PENDANT_ITEM
		case ITEM_PENDANT:
		{
			return WEAR_PENDANT;
		}
#endif
#ifdef ENABLE_GLOVE_ITEM
		case ITEM_GLOVE:
		{
			return WEAR_GLOVE;
		}
#endif
#ifdef ENABLE_AURA_SKIN
		case ITEM_AURA_SKIN:
		{
			return WEAR_AURA_SKIN;
		}
#endif

#ifdef ENABLE_ITEMS_SHINING
		case ITEM_SHINING:
		{
			if (GetSubType() == SHINING_WRIST_LEFT)
				return WEAR_SHINING_WRIST_LEFT;
			else if (GetSubType() == SHINING_WRIST_RIGHT)
				return WEAR_SHINING_WRIST_RIGHT;
			else if (GetSubType() == SHINING_ARMOR)
				return WEAR_SHINING_ARMOR;
			break;
		}
#endif

#ifdef ENABLE_BOOSTER_ITEM
		case ITEM_BOOSTER:
		{
			switch (GetSubType())
			{
				case BOOSTER_WEAPON: { return WEAR_BOOSTER_WEAPON; }
				case BOOSTER_BODY: { return WEAR_BOOSTER_BODY; }
				case BOOSTER_HEAD: { return WEAR_BOOSTER_HEAD; }
				case BOOSTER_SASH: { return WEAR_BOOSTER_SASH; }
				case BOOSTER_PET: { return WEAR_BOOSTER_PET; }
				case BOOSTER_MOUNT: { return WEAR_BOOSTER_MOUNT; }
				default: break;
			}
			break;
		}
#endif
#ifdef ENABLE_RINGS
		case ITEM_RINGS:
		{
			switch (GetSubType())
			{
				case RING_0: { return WEAR_RING_0; }
				case RING_1: { return WEAR_RING_1; }
				case RING_2: { return WEAR_RING_2; }
				case RING_3: { return WEAR_RING_3; }
				case RING_4: { return WEAR_RING_4; }
				case RING_5: { return WEAR_RING_5; }
				case RING_6: { return WEAR_RING_6; }
				case RING_7: { return WEAR_RING_7; }
				default: break;
			}
			break;
		}
#endif
#ifdef ENABLE_DREAMSOUL
		case ITEM_DREAMSOUL:
		{
			switch (GetSubType())
			{
				case DREAMSOUL_RED: { return WEAR_DREAMSOUL_RED; }
				case DREAMSOUL_BLUE: { return WEAR_DREAMSOUL_BLUE; }
				default: break;
			}
			break;
		}
#endif

		default:
		{
			if (GetWearFlag() & WEARABLE_BODY)
				return WEAR_BODY;
			else if (GetWearFlag() & WEARABLE_HEAD)
				return WEAR_HEAD;
			else if (GetWearFlag() & WEARABLE_FOOTS)
				return WEAR_FOOTS;
			else if (GetWearFlag() & WEARABLE_WRIST)
				return WEAR_WRIST;
			else if (GetWearFlag() & WEARABLE_WEAPON)
				return WEAR_WEAPON;
			else if (GetWearFlag() & WEARABLE_SHIELD)
				return WEAR_SHIELD;
			else if (GetWearFlag() & WEARABLE_NECK)
				return WEAR_NECK;
			else if (GetWearFlag() & WEARABLE_EAR)
				return WEAR_EAR;
			else if (GetWearFlag() & WEARABLE_ARROW)
				return WEAR_ARROW;
			else if (GetWearFlag() & WEARABLE_UNIQUE)
			{
				if (ch->GetWear(WEAR_UNIQUE1))
					return WEAR_UNIQUE2;
				else
					return WEAR_UNIQUE1;
			}
#ifdef ENABLE_NEW_PET_SYSTEM
			if (GetVnum() == 55701)
				return WEAR_NEW_PET;
#endif
			break;
		}	
	}

	return -1;
}

void CItem::ModifyPoints(bool bAdd)
{
#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetVnum() == 55701)
		return;
#endif

	int accessoryGrade;

#ifdef ENABLE_JEWELS_RENEWAL
	if (!IsAccessoryForSocket() && !IsAccessoryForSocketNew())
#else
	if (false == IsAccessoryForSocket())
#endif
	{
		if (m_pProto->bType == ITEM_WEAPON || m_pProto->bType == ITEM_ARMOR)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				DWORD dwVnum;

				if ((dwVnum = GetSocket(i)) <= 2)
					continue;

				TItemTable* p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
				{
					sys_err("cannot find table by vnum %u", dwVnum);
					continue;
				}

				if (ITEM_METIN == p->bType)
				{
					//m_pOwner->ApplyPoint(p->alValues[0], bAdd ? p->alValues[1] : -p->alValues[1]);
					for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
					{
						if (p->aApplies[i].bType == APPLY_NONE)
							continue;

						if (p->aApplies[i].bType == APPLY_SKILL)
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : p->aApplies[i].lValue ^ 0x00800000);
						else
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : -p->aApplies[i].lValue);
					}
				}
			}
		}

		accessoryGrade = 0;
	}
	else
	{
		accessoryGrade = MIN(GetAccessorySocketGrade(), ITEM_ACCESSORY_SOCKET_MAX_NUM);
	}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_ACCE) && (GetSocket(ACCE_ABSORBED_SOCKET)))
	{
		TItemTable* pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(ACCE_ABSORBED_SOCKET));
		if (pkItemAbsorbed)
		{
			/*if ((pkItemAbsorbed->bType == ITEM_ARMOR) && (pkItemAbsorbed->bSubType == ARMOR_BODY))
			{
				// basic defense value from armor
				auto lDefGrade = CalcAcceBonus(pkItemAbsorbed->alValues[1] + (pkItemAbsorbed->alValues[5] * 2));
				m_pOwner->ApplyPoint(APPLY_DEF_GRADE_BONUS, bAdd ? lDefGrade : -lDefGrade);
				// basic magic defense value from armor
				auto lDefMagicBonus = CalcAcceBonus(pkItemAbsorbed->alValues[0]);
				m_pOwner->ApplyPoint(APPLY_MAGIC_DEF_GRADE, bAdd ? lDefMagicBonus : -lDefMagicBonus);
			}*/
			if (pkItemAbsorbed->bType == ITEM_WEAPON)
			{
				// basic attack value from weapon
				if (pkItemAbsorbed->alValues[3] + pkItemAbsorbed->alValues[4] > 0)
				{
					auto lAttGrade = CalcAcceBonus(pkItemAbsorbed->alValues[4] + pkItemAbsorbed->alValues[5]);
					if (pkItemAbsorbed->alValues[3] > pkItemAbsorbed->alValues[4])
						lAttGrade = CalcAcceBonus(pkItemAbsorbed->alValues[3] + pkItemAbsorbed->alValues[5]);
					m_pOwner->ApplyPoint(APPLY_ATT_GRADE_BONUS, bAdd ? lAttGrade : -lAttGrade);
				}
				// basic magic attack value from weapon
				if (pkItemAbsorbed->alValues[1] + pkItemAbsorbed->alValues[2] > 0)
				{
					long lAttMagicGrade = CalcAcceBonus(pkItemAbsorbed->alValues[2] + pkItemAbsorbed->alValues[5]);
					if (pkItemAbsorbed->alValues[1] > pkItemAbsorbed->alValues[2])
						lAttMagicGrade = CalcAcceBonus(pkItemAbsorbed->alValues[1] + pkItemAbsorbed->alValues[5]);
					m_pOwner->ApplyPoint(APPLY_MAGIC_ATT_GRADE, bAdd ? lAttMagicGrade : -lAttMagicGrade);
				}
			}
		}
	}
#endif

#ifdef ENABLE_AURA_SYSTEM
	if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_AURA) && (GetSocket(AURA_ABSORBED_SOCKET)))
	{
		TItemTable* pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(AURA_ABSORBED_SOCKET));
		if (pkItemAbsorbed)
		{
			if ((pkItemAbsorbed->bType == ITEM_ARMOR) && ((pkItemAbsorbed->bSubType == ARMOR_SHIELD) || (pkItemAbsorbed->bSubType == ARMOR_HEAD)))
			{
				long lDefGrade = pkItemAbsorbed->alValues[1] + long(pkItemAbsorbed->alValues[5] * 2);
				double dValue = lDefGrade * (GetSocket(AURA_ABSORPTION_SOCKET));
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				lDefGrade = (long)dValue;
				if (((pkItemAbsorbed->alValues[1] > 0) && (lDefGrade <= 0)) || ((pkItemAbsorbed->alValues[5] > 0) && (lDefGrade < 1)))
					lDefGrade += 1;
				else if ((pkItemAbsorbed->alValues[1] > 0) || (pkItemAbsorbed->alValues[5] > 0))
					lDefGrade += 1;

				m_pOwner->ApplyPoint(APPLY_DEF_GRADE_BONUS, bAdd ? lDefGrade : -lDefGrade);

				long lDefMagicBonus = pkItemAbsorbed->alValues[0];
				dValue = lDefMagicBonus * (GetSocket(AURA_ABSORPTION_SOCKET));
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				lDefMagicBonus = (long)dValue;
				if ((pkItemAbsorbed->alValues[0] > 0) && (lDefMagicBonus < 1))
					lDefMagicBonus += 1;
				else if (pkItemAbsorbed->alValues[0] > 0)
					lDefMagicBonus += 1;

				m_pOwner->ApplyPoint(APPLY_MAGIC_DEF_GRADE, bAdd ? lDefMagicBonus : -lDefMagicBonus);
			}
		}
	}
#endif

#ifdef ENABLE_NAMING_SCROLL
	const int bonusRate = BonusRate();
#endif

#if defined (ENABLE_AURA_SYSTEM) || defined (ENABLE_ACCE_COSTUME_SYSTEM)
	if (GetType() == ITEM_COSTUME && (GetSubType() == COSTUME_ACCE || GetSubType() == COSTUME_AURA))
	{
		TItemTable* pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(ACCE_ABSORBED_SOCKET));
		for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		{
			if (!pkItemAbsorbed)
				break;

			int value = m_pProto->aApplies[i].lValue;
			
			if (pkItemAbsorbed->aApplies[i].bType == APPLY_NONE)
				continue;

			value = CalcAcceBonus(pkItemAbsorbed->aApplies[i].lValue);

			if (pkItemAbsorbed->aApplies[i].bType == APPLY_SKILL)
			{
				m_pOwner->ApplyPoint(pkItemAbsorbed->aApplies[i].bType, bAdd ? value : value ^ 0x00800000);
			}
			else
			{
				if (0 != accessoryGrade && i < ITEM_APPLY_MAX_NUM - 1) // @fixme170
					value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

				m_pOwner->ApplyPoint(pkItemAbsorbed->aApplies[i].bType, bAdd ? value : -value);
			}
		}
	}
#endif
	else
	{
		for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		{
			if (m_pProto->aApplies[i].bType == APPLY_NONE)
				continue;
			int value = m_pProto->aApplies[i].lValue;
			if (m_pProto->aApplies[i].bType == APPLY_SKILL)
				m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : value ^ 0x00800000);

			else
			{
				if (0 != accessoryGrade && i < ITEM_APPLY_MAX_NUM - 1) // @fixme170
#ifdef ENABLE_JEWELS_RENEWAL
				{
					if (IsPermaEquipment())
						value += MAX(accessoryGrade, value * aiAccessorySocketPermaEffectivePct[accessoryGrade] / 100);
					else
						value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);
				}
#else
					value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);
#endif
#ifdef ENABLE_NAMING_SCROLL
				if (bonusRate != -1)
				{
					value += value * bonusRate / 100;
				}
#endif
				m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : -value);
			}
		}
	}


	for (int i = 0; i < 7; ++i)
	{
		if (GetAttributeType(i))
		{
			const TPlayerItemAttribute& ia = GetAttribute(i);
			auto value = ia.sValue;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			if (GetType() == ITEM_COSTUME && GetSubType() == COSTUME_ACCE)
				value = CalcAcceBonus(value);
#endif
#ifdef ENABLE_AURA_SYSTEM
			else if (GetType() == ITEM_COSTUME && GetSubType() == COSTUME_AURA)
				value = CalcAuraBonus(value);
#endif
#ifdef ENABLE_DS_SET_BONUS
			//short sValue = ia.sValue;
			else if ((IsDragonSoul()) && (m_pOwner->FindAffect(AFFECT_DS_SET)))
			{
				value += i < DSManager::instance().GetApplyCount(GetVnum()) ? DSManager::instance().GetBasicApplyValue(GetVnum(), ia.bType, true) : DSManager::instance().GetAdditionalApplyValue(GetVnum(), ia.bType, true);
			}
#endif
			if (bonusRate != -1)
			{
				value += value * bonusRate / 100;
			}
			if (ia.bType == APPLY_SKILL)
				m_pOwner->ApplyPoint(ia.bType, bAdd ? value : value ^ 0x00800000);
			else
				m_pOwner->ApplyPoint(ia.bType, bAdd ? value : -value);
		}
	}

	switch (m_pProto->bType)
	{
	case ITEM_PICK:
	case ITEM_ROD:
	{
		if (bAdd)
		{
			if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
				m_pOwner->SetPart(PART_WEAPON, GetVnum());
		}
		else
		{
			if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
				m_pOwner->SetPart(PART_WEAPON, 0);
		}
	}
	break;

	case ITEM_WEAPON:
	{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		if (0 != m_pOwner->GetWear(WEAR_COSTUME_WEAPON))
			break;
#endif

		if (bAdd)
		{
			if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
				m_pOwner->SetPart(PART_WEAPON, GetVnum());
		}
		else
		{
			if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
				m_pOwner->SetPart(PART_WEAPON, 0);
		}
	}
	break;

	case ITEM_ARMOR:
	{
		if (0 != m_pOwner->GetWear(WEAR_COSTUME_BODY))
			break;

		if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD)
		{
			if (bAdd)
			{
				if (GetProto()->bSubType == ARMOR_BODY)
					m_pOwner->SetPart(PART_MAIN, GetVnum());
			}
			else
			{
				if (GetProto()->bSubType == ARMOR_BODY)
					m_pOwner->SetPart(PART_MAIN, m_pOwner->GetOriginalPart(PART_MAIN));
			}
		}
	}
	break;

	case ITEM_COSTUME:
	{
		DWORD toSetValue = this->GetVnum();
		EParts toSetPart = PART_MAX_NUM;

		if (GetSubType() == COSTUME_BODY)
		{
			toSetPart = PART_MAIN;

			if (false == bAdd)
			{
				const CItem* pArmor = m_pOwner->GetWear(WEAR_BODY);
				toSetValue = (NULL != pArmor) ? pArmor->GetVnum() : m_pOwner->GetOriginalPart(PART_MAIN);
			}
		}

		else if (GetSubType() == COSTUME_HAIR)
		{
			toSetPart = PART_HAIR;
			toSetValue = (true == bAdd) ? this->GetValue(3) : 0;
		}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_ACCE)
		{
#ifdef ENABLE_ACCE_COSTUME_SKIN
			if (0 != m_pOwner->GetWear(WEAR_COSTUME_WING))//kana tak�l�ysa
			{
				UpdatePacket();
				break;
			}
#endif
			toSetValue = (bAdd == true) ? this->GetVnum() : 0;
			toSetPart = PART_ACCE;
		}
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_WEAPON)
		{
			toSetPart = PART_WEAPON;
			if (false == bAdd)
			{
				const CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
				toSetValue = (NULL != pWeapon) ? pWeapon->GetVnum() : 0;
			}
		}
#endif

#ifdef ENABLE_AURA_SYSTEM
		else if (GetSubType() == COSTUME_AURA)
		{
#ifdef ENABLE_AURA_SKIN
			if (0 != m_pOwner->GetWear(WEAR_AURA_SKIN))
			{
				UpdatePacket();
				break;
			}
#endif
			toSetValue = (bAdd == true) ? toSetValue : 0;
			toSetPart = PART_AURA;
		}
#endif

#ifdef ENABLE_ACCE_COSTUME_SKIN
		else if (GetSubType() == COSTUME_WING)
		{
			toSetPart = PART_ACCE;
			if (false == bAdd)
			{
				const CItem* pAcceCostume = m_pOwner->GetWear(WEAR_COSTUME_ACCE);
				toSetValue = (NULL != pAcceCostume) ? pAcceCostume->GetVnum() : 0;
			}
		}
#endif

		if (PART_MAX_NUM != toSetPart)
		{
			m_pOwner->SetPart((BYTE)toSetPart, toSetValue);
			m_pOwner->UpdatePacket();
		}
	}
	break;

#ifdef ENABLE_AURA_SKIN
	case ITEM_AURA_SKIN:
	{
		DWORD toSetValue = this->GetVnum();
		if (false == bAdd)
		{
			const CItem* pAuraCostume = m_pOwner->GetWear(WEAR_COSTUME_AURA);
			toSetValue = pAuraCostume ? pAuraCostume->GetVnum() : 0;
		}
		m_pOwner->SetPart(PART_AURA, toSetValue);
		m_pOwner->UpdatePacket();
	}
	break;
#endif

#ifdef ENABLE_CROWN_SYSTEM
	case ITEM_CROWN:
	{
		DWORD toSetValue = bAdd ? this->GetValue(1) : 0;
		m_pOwner->SetPart(PART_CROWN, toSetValue);
		m_pOwner->UpdatePacket();
	}
	break;
#endif

	case ITEM_UNIQUE:
	{
		if (0 != GetSIGVnum())
		{
			const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(GetSIGVnum());
			if (NULL == pItemGroup)
				break;
			DWORD dwAttrVnum = pItemGroup->GetAttrVnum(GetVnum());
			const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(dwAttrVnum);
			if (NULL == pAttrGroup)
				break;
			for (itertype(pAttrGroup->m_vecAttrs) it = pAttrGroup->m_vecAttrs.begin(); it != pAttrGroup->m_vecAttrs.end(); it++)
			{
				m_pOwner->ApplyPoint(it->apply_type, bAdd ? it->apply_value : -it->apply_value);
			}
		}
	}
	break;
	}
}

bool CItem::IsEquipable() const
{
	switch (this->GetType())
	{
	case ITEM_COSTUME:
	case ITEM_ARMOR:
	case ITEM_WEAPON:
	case ITEM_ROD:
	case ITEM_PICK:
	case ITEM_UNIQUE:
	case ITEM_DS:
	case ITEM_SPECIAL_DS:
	case ITEM_RING:
	case ITEM_BELT:
#ifdef ENABLE_PET_COSTUME_SYSTEM
	case ITEM_PET:
#endif
#ifdef ENABLE_MOUNT_SKIN
	case ITEM_MOUNT_SKIN:
#endif
#ifdef ENABLE_PET_SKIN
	case ITEM_PET_SKIN:
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
#ifdef ENABLE_AURA_SKIN
	case ITEM_AURA_SKIN:
#endif
#ifdef ENABLE_DREAMSOUL
	case ITEM_DREAMSOUL:
#endif
		return true;
	default:
	{
#ifdef ENABLE_NEW_PET_SYSTEM
		if (GetVnum() == 55701)
			return true;
#endif
		break;
	}
	}

	return false;
}

#define ENABLE_IMMUNE_FIX
// return false on error state
bool CItem::EquipTo(LPCHARACTER ch, BYTE bWearCell)
{
	if (!ch)
	{
		sys_err("EquipTo: nil character");
		return false;
	}

	if (IsDragonSoul())
	{
		if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
		{
			sys_err("EquipTo: invalid dragon soul cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM);
			return false;
		}
	}
	else
	{
		if (bWearCell >= WEAR_MAX_NUM)
		{
			sys_err("EquipTo: invalid wear cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
	}

	if (ch->GetWear(bWearCell))
	{
		sys_err("EquipTo: item already exist (this: #%d %s cell: %d %s)", GetOriginalVnum(), GetName(), bWearCell, ch->GetWear(bWearCell)->GetName());
		return false;
	}

	if (GetOwner())
		RemoveFromCharacter();

	ch->SetWear(bWearCell, this);

	m_pOwner = ch;
	m_bEquipped = true;
	m_wCell = INVENTORY_MAX_NUM + bWearCell;

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

	if (IsDragonSoul())
	{
		DSManager::instance().ActivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(true);
		StartUniqueExpireEvent();
		if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
			StartTimerBasedOnWearExpireEvent();

		// ACCESSORY_REFINE
		StartAccessorySocketExpireEvent();
		// END_OF_ACCESSORY_REFINE
	}

	ch->BuffOnAttr_AddBuffsFromItem(this);

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (IsMountItem())
	{
		ch->MountSummon(this);
	}
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
	if (IsPetItem())
	{
		ch->PetSummon(this);
	}
#endif

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();
	Save();

	return (true);
}

bool CItem::Unequip()
{
	if (!m_pOwner || GetCell() < INVENTORY_MAX_NUM)
	{
		// ITEM_OWNER_INVALID_PTR_BUG
		sys_err("%s %u m_pOwner %p, GetCell %d",
			GetName(), GetID(), get_pointer(m_pOwner), GetCell());
		// END_OF_ITEM_OWNER_INVALID_PTR_BUG
		return false;
	}

	if (this != m_pOwner->GetWear(GetCell() - INVENTORY_MAX_NUM))
	{
		sys_err("m_pOwner->GetWear() != this");
		return false;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (IsMountItem())
	{
		m_pOwner->MountUnsummon(this);
	}
#endif

#ifdef ENABLE_PET_COSTUME_SYSTEM
	if (IsPetItem())
	{
		m_pOwner->PetUnsummon(this);
	}
#endif

	if (IsRideItem())
	{
		ClearMountAttributeAndAffect();
	}

	if (IsDragonSoul())
	{
		DSManager::instance().DeactivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(false);
	}

	StopUniqueExpireEvent();

	if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
		StopTimerBasedOnWearExpireEvent();

	// ACCESSORY_REFINE
	StopAccessorySocketExpireEvent();
	// END_OF_ACCESSORY_REFINE

	m_pOwner->BuffOnAttr_RemoveBuffsFromItem(this);

	m_pOwner->SetWear(GetCell() - INVENTORY_MAX_NUM, NULL);

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	m_pOwner = NULL;
	m_wCell = 0;
	m_bEquipped = false;

	return true;
}

long CItem::GetValue(DWORD idx)
{
	assert(idx < ITEM_VALUES_MAX_NUM);
	return GetProto()->alValues[idx];
}

void CItem::SetExchanging(bool bOn)
{
	m_bExchanging = bOn;
}

void CItem::Save()
{
	if (m_bSkipSave)
		return;

	ITEM_MANAGER::instance().DelayedSave(this);
}

bool CItem::CreateSocket(BYTE bSlot, BYTE bGold)
{
	assert(bSlot < ITEM_SOCKET_MAX_NUM);

	if (m_alSockets[bSlot] != 0)
	{
		sys_err("Item::CreateSocket : socket already exist %s %d", GetName(), bSlot);
		return false;
	}

	if (bGold)
		m_alSockets[bSlot] = 2;
	else
		m_alSockets[bSlot] = 1;

	UpdatePacket();

	Save();
	return true;
}

void CItem::SetSockets(const long* c_al)
{
	thecore_memcpy(m_alSockets, c_al, sizeof(m_alSockets));
	Save();
}

void CItem::SetSocket(int i, long v, bool bLog)
{
	assert(i < ITEM_SOCKET_MAX_NUM);
	m_alSockets[i] = v;
	UpdatePacket();
	Save();
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
int64_t CItem::GetGold()
#else
int CItem::GetGold()
#endif
{
	if (IS_SET(GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (GetProto()->dwGold == 0)
			return GetCount();
		else
			return GetCount() / GetProto()->dwGold;
	}
	else
		return GetProto()->dwGold;
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
int64_t CItem::GetShopBuyPrice()
#else
int CItem::GetShopBuyPrice()
#endif
{
	return GetProto()->dwShopBuyPrice;
}

bool CItem::IsOwnership(LPCHARACTER ch)
{
	if (!m_pkOwnershipEvent)
		return true;

	return m_dwOwnershipPID == ch->GetPlayerID() ? true : false;
}

EVENTFUNC(ownership_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (!info)
	{
		sys_err("ownership_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM pkItem = info->item;

	pkItem->SetOwnershipEvent(NULL);

	TPacketGCItemOwnership p;

	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = pkItem->GetVID();
	p.szName[0] = '\0';

	pkItem->PacketAround(&p, sizeof(p));
	return 0;
}

void CItem::SetOwnershipEvent(LPEVENT pkEvent)
{
	m_pkOwnershipEvent = pkEvent;
}

void CItem::SetOwnership(LPCHARACTER ch, int iSec)
{
	if (!ch)
	{
		if (m_pkOwnershipEvent)
		{
			event_cancel(&m_pkOwnershipEvent);
			m_dwOwnershipPID = 0;

			TPacketGCItemOwnership p;

			p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
			p.dwVID = m_dwVID;
			p.szName[0] = '\0';

			PacketAround(&p, sizeof(p));
		}
		return;
	}

	if (m_pkOwnershipEvent)
		return;

	if (iSec <= 10)
		iSec = 30;

	m_dwOwnershipPID = ch->GetPlayerID();

	item_event_info* info = AllocEventInfo<item_event_info>();
	strlcpy(info->szOwnerName, ch->GetName(), sizeof(info->szOwnerName));
	info->item = this;

	SetOwnershipEvent(event_create(ownership_event, info, PASSES_PER_SEC(iSec)));

	TPacketGCItemOwnership p;

	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = m_dwVID;
	strlcpy(p.szName, ch->GetName(), sizeof(p.szName));

	PacketAround(&p, sizeof(p));
}

int CItem::GetSocketCount()
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		if (GetSocket(i) == 0)
			return i;
	}
	return ITEM_SOCKET_MAX_NUM;
}

bool CItem::AddSocket()
{
	int count = GetSocketCount();
	if (count == ITEM_SOCKET_MAX_NUM)
		return false;
	m_alSockets[count] = 1;
	return true;
}

void CItem::AlterToSocketItem(int iSocketCount)
{
	if (iSocketCount >= ITEM_SOCKET_MAX_NUM)
	{
		iSocketCount = ITEM_SOCKET_MAX_NUM;
	}

	for (int i = 0; i < iSocketCount; ++i)
		SetSocket(i, 1);
}

void CItem::AlterToMagicItem()
{
	int idx = GetAttributeSetIndex();

	if (idx < 0)
		return;

	//	  Appearance Second Third
	// Weapon 50		20	 5
	// Armor  30		10	 2
	// Acc	20		10	 1

	int iSecondPct;
	int iThirdPct;

	switch (GetType())
	{
	case ITEM_WEAPON:
		iSecondPct = 20;
		iThirdPct = 5;
		break;

	case ITEM_ARMOR:
	case ITEM_COSTUME:
		if (GetSubType() == ARMOR_BODY)
		{
			iSecondPct = 10;
			iThirdPct = 2;
		}
		else
		{
			iSecondPct = 10;
			iThirdPct = 1;
		}
		break;

	default:
		return;
	}

	PutAttribute(aiItemMagicAttributePercentHigh);

	if (number(1, 100) <= iSecondPct)
		PutAttribute(aiItemMagicAttributePercentLow);

	if (number(1, 100) <= iThirdPct)
		PutAttribute(aiItemMagicAttributePercentLow);
}

DWORD CItem::GetRefineFromVnum()
{
	return ITEM_MANAGER::instance().GetRefineFromVnum(GetVnum());
}

int CItem::GetRefineLevel()
{
	const char* name = GetBaseName();
	char* p = const_cast<char*>(strrchr(name, '+'));

	if (!p)
		return 0;

	int	rtn = 0;
	str_to_number(rtn, p + 1);

	const char* locale_name = GetName();
	p = const_cast<char*>(strrchr(locale_name, '+'));

	if (p)
	{
		int	locale_rtn = 0;
		str_to_number(locale_rtn, p + 1);
		if (locale_rtn != rtn)
		{
			sys_err("refine_level_based_on_NAME(%d) is not equal to refine_level_based_on_LOCALE_NAME(%d).", rtn, locale_rtn);
		}
	}

	return rtn;
}

bool CItem::IsPolymorphItem()
{
	return GetType() == ITEM_POLYMORPH;
}

EVENTFUNC(unique_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (!info)
	{
		sys_err("unique_expire_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetValue(2) == 0)
	{
		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= 1)
		{
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			pkItem->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - 1);
			return PASSES_PER_SEC(60);
		}
	}
	else
	{
		time_t cur = get_global_time();

		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= cur)
		{
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			// by rtsummit
			if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur < 600)
				return PASSES_PER_SEC(pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur);
			else
				return PASSES_PER_SEC(600);
		}
	}
}

EVENTFUNC(timer_based_on_wear_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (!info)
	{
		sys_err("expire_event <Factor> Null pointer");
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(ITEM_SOCKET_REMAIN_SEC) - processing_time / passes_per_sec;
	if (remain_time <= 0)
	{
		pkItem->SetTimerBasedOnWearExpireEvent(NULL);
		pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, 0);

		if (pkItem->IsDragonSoul())
		{
			DSManager::instance().DeactivateDragonSoul(pkItem);
		}
		else
		{
			ITEM_MANAGER::instance().RemoveItem(pkItem, "TIMER_BASED_ON_WEAR_EXPIRE");
		}
		return 0;
	}
	pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	return PASSES_PER_SEC(MIN(60, remain_time));
}

void CItem::SetUniqueExpireEvent(LPEVENT pkEvent)
{
	m_pkUniqueExpireEvent = pkEvent;
}

void CItem::SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent)
{
	m_pkTimerBasedOnWearExpireEvent = pkEvent;
}

EVENTFUNC(real_time_expire_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (!info)
		return 0;

	const LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);

	if (!item)
		return 0;

	const time_t current = get_global_time();

	if (current > item->GetSocket(0))
	{
		if (item->GetVnum() && item->IsNewMountItem()) // @fixme152
			item->ClearMountAttributeAndAffect();

		ITEM_MANAGER::instance().RemoveItem(item, "REAL_TIME_EXPIRE");

		return 0;
	}

	return PASSES_PER_SEC(1);
}

void CItem::StartRealTimeExpireEvent()
{
	if (m_pkRealTimeExpireEvent)
		return;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->aLimits[i].bType)
		{
			item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
			info->item_vid = GetVID();

			m_pkRealTimeExpireEvent = event_create(real_time_expire_event, info, PASSES_PER_SEC(1));
			return;
		}
	}
}

bool CItem::IsRealTimeItem()
{
	if (!GetProto())
		return false;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return true;
	}
	return false;
}

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
EVENTFUNC(blend_use_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("expire_event <Factor> Null pointer");
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(2) - processing_time / passes_per_sec;

	if (remain_time <= 0)
	{
		pkItem->SetSocket(2, 0);
		pkItem->SetSocket(3, false);
		pkItem->Lock(false);
		ITEM_MANAGER::instance().RemoveItem(pkItem, "BLEND_USE_EXPIRE");
		return 0;
	}

	pkItem->SetSocket(2, remain_time);
	return PASSES_PER_SEC(MIN(1, remain_time));
}

void CItem::StartBlendExpireEvent()
{
	if (m_pkBlendUseEvent)
		return;

	if (IsRealTimeItem())
		return;

	if (!IsBlendItem())
		return;

	int iSec = GetSocket(2);

	if (iSec >= 8640000)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	m_pkBlendUseEvent = event_create(blend_use_expire_event, info, PASSES_PER_SEC(1));
}

void CItem::StopBlendExpireEvent(bool &isDestroy)
{
	if (!m_pkBlendUseEvent)
		return;

	int remain_time = GetSocket(2) - event_processing_time(m_pkBlendUseEvent) / passes_per_sec;

	if (remain_time > 61) // decreasing item time 1min in every use
		remain_time -= 60;
	else
		isDestroy = true;

	SetSocket(2, remain_time);
	event_cancel(&m_pkBlendUseEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}
#endif

void CItem::StartUniqueExpireEvent()
{
	if (GetType() != ITEM_UNIQUE)
		return;

	if (m_pkUniqueExpireEvent)
		return;

	if (IsRealTimeItem())
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(false);

	int iSec = GetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME);

	if (iSec == 0)
		iSec = 60;
	else
		iSec = MIN(iSec, 60);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, 0);

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetUniqueExpireEvent(event_create(unique_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StartTimerBasedOnWearExpireEvent()
{
	if (m_pkTimerBasedOnWearExpireEvent)
		return;

	if (IsRealTimeItem())
		return;

	if (-1 == GetProto()->cLimitTimerBasedOnWearIndex)
		return;

	int iSec = GetSocket(0);

	if (0 != iSec)
	{
		iSec %= 60;
		if (0 == iSec)
			iSec = 60;
	}

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetTimerBasedOnWearExpireEvent(event_create(timer_based_on_wear_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopUniqueExpireEvent()
{
	if (!m_pkUniqueExpireEvent)
		return;

	if (GetValue(2) != 0)
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(true);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, event_time(m_pkUniqueExpireEvent) / passes_per_sec);
	event_cancel(&m_pkUniqueExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::StopTimerBasedOnWearExpireEvent()
{
	if (!m_pkTimerBasedOnWearExpireEvent)
		return;

	int remain_time = GetSocket(ITEM_SOCKET_REMAIN_SEC) - event_processing_time(m_pkTimerBasedOnWearExpireEvent) / passes_per_sec;

	SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::ApplyAddon(int iAddonType)
{
	CItemAddonManager::instance().ApplyAddonTo(iAddonType, this);
}

int CItem::GetSpecialGroup() const
{
	return ITEM_MANAGER::instance().GetSpecialGroupFromItem(GetVnum());
}

bool CItem::IsAccessoryForSocket()
{
	return (m_pProto->bType == ITEM_ARMOR && (m_pProto->bSubType == ARMOR_WRIST || m_pProto->bSubType == ARMOR_NECK || m_pProto->bSubType == ARMOR_EAR)) ||
		(m_pProto->bType == ITEM_BELT);
}

void CItem::SetAccessorySocketGrade(int iGrade)
{
	SetSocket(0, MINMAX(0, iGrade, GetAccessorySocketMaxGrade()));

	int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

	SetAccessorySocketDownGradeTime(iDownTime);
}

void CItem::SetAccessorySocketMaxGrade(int iMaxGrade)
{
	SetSocket(1, MINMAX(0, iMaxGrade, ITEM_ACCESSORY_SOCKET_MAX_NUM));
}

void CItem::SetAccessorySocketDownGradeTime(DWORD time)
{
#ifdef ENABLE_JEWELS_RENEWAL
	if (IsPermaEquipment())
		return;
#endif
	SetSocket(2, time);
}

EVENTFUNC(accessory_socket_expire_event)
{
	item_vid_event_info* info = dynamic_cast<item_vid_event_info*>(event->info);

	if (!info)
	{
		sys_err("accessory_socket_expire_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);

	if (item->GetAccessorySocketDownGradeTime() <= 1)
	{
	degrade:
		item->SetAccessorySocketExpireEvent(NULL);
		item->AccessorySocketDegrade();
		return 0;
	}
	else
	{
		int iTime = item->GetAccessorySocketDownGradeTime() - 60;

		if (iTime <= 1)
			goto degrade;

		item->SetAccessorySocketDownGradeTime(iTime);

		if (iTime > 60)
			return PASSES_PER_SEC(60);
		else
			return PASSES_PER_SEC(iTime);
	}
}

void CItem::StartAccessorySocketExpireEvent()
{
	if (!IsAccessoryForSocket())
		return;

	if (m_pkAccessorySocketExpireEvent)
		return;

	if (GetAccessorySocketMaxGrade() == 0)
		return;

	if (GetAccessorySocketGrade() == 0)
		return;

#ifdef ENABLE_JEWELS_RENEWAL
	if (IsPermaEquipment())
		return;
#endif

	int iSec = GetAccessorySocketDownGradeTime();
	SetAccessorySocketExpireEvent(NULL);

	if (iSec <= 1)
		iSec = 5;
	else
		iSec = MIN(iSec, 60);

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	SetAccessorySocketExpireEvent(event_create(accessory_socket_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopAccessorySocketExpireEvent()
{
	if (!m_pkAccessorySocketExpireEvent)
		return;

	if (!IsAccessoryForSocket())
		return;

#ifdef ENABLE_JEWELS_RENEWAL
	if (IsPermaEquipment())
		return;
#endif

	int new_time = GetAccessorySocketDownGradeTime() - (60 - event_time(m_pkAccessorySocketExpireEvent) / passes_per_sec);

	event_cancel(&m_pkAccessorySocketExpireEvent);

	if (new_time <= 1)
	{
		AccessorySocketDegrade();
	}
	else
	{
		SetAccessorySocketDownGradeTime(new_time);
	}
}

bool CItem::IsRideItem()
{
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_RIDE == GetSubType())
		return true;
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType())
		return true;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ITEM_COSTUME == GetType() && COSTUME_MOUNT == GetSubType())
		return true;
#endif
#ifdef ENABLE_MOUNT_SKIN
	if (GetType() == ITEM_MOUNT_SKIN)
		return true;
#endif

	return false;
}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
bool CItem::IsMountItem()
{
	if (GetType() == ITEM_COSTUME && GetSubType() == COSTUME_MOUNT)
		return true;

#ifdef ENABLE_MOUNT_SKIN
	if (GetType() == ITEM_MOUNT_SKIN)
		return true;
#endif

	return false;
}
#endif

#ifdef ENABLE_PET_COSTUME_SYSTEM
bool CItem::IsPetItem()
{
	if (GetType() == ITEM_PET)
		return true;

#ifdef ENABLE_PET_SKIN
	if (GetType() == ITEM_PET_SKIN)
		return true;
#endif


	return false;
}
#endif

void CItem::ClearMountAttributeAndAffect()
{
	LPCHARACTER ch = GetOwner();
	if (!ch) // @fixme186
		return;

	ch->MountVnum(0);
	ch->ComputePoints();
}

bool CItem::IsNewMountItem()
{
	return (
		(ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_RIDE == GetSubType() && IS_SET(GetFlag(), ITEM_FLAG_QUEST_USE))
		|| (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType() && IS_SET(GetFlag(), ITEM_FLAG_QUEST_USE))
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		|| (ITEM_COSTUME == GetType() && COSTUME_MOUNT == GetSubType())
#endif
		); // @fixme152
}

void CItem::SetAccessorySocketExpireEvent(LPEVENT pkEvent)
{
	m_pkAccessorySocketExpireEvent = pkEvent;
}

void CItem::AccessorySocketDegrade()
{
	if (GetAccessorySocketGrade() > 0)
	{
		LPCHARACTER ch = GetOwner();

		if (ch)
		{
			ch->NewChatPacket(ACCESSORY_SOCKET_DISSAPEARED, "%s", GetName() );
		}

		ModifyPoints(false);
		SetAccessorySocketGrade(GetAccessorySocketGrade() - 1);
		ModifyPoints(true);

		int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];
		SetAccessorySocketDownGradeTime(iDownTime);

		if (iDownTime)
			StartAccessorySocketExpireEvent();
	}
}

bool CItem::CanPutInto(LPITEM item)
{
	if (item->GetType() == ITEM_BELT)
		return this->GetSubType() == USE_PUT_INTO_BELT_SOCKET;

	else if (item->GetType() != ITEM_ARMOR)
		return false;

#ifdef ENABLE_JEWELS_RENEWAL
	if (isPermaJewels())
	{
		if (CanPutIntoPerma(item))
			return true;
		return false;
	}
#endif

	DWORD vnum = item->GetVnum();

	struct JewelAccessoryInfo
	{
		DWORD jewel;
		DWORD wrist;
		DWORD neck;
		DWORD ear;
	};
	const static JewelAccessoryInfo infos[] = {
		{ 50623, 14000, 16000, 17000 }, //Fosil oc
		{ 50624, 14020, 16020, 17020 }, //BAKIR
		{ 50625, 14040, 14040, 14040 }, //GUMUS
		{ 50626, 14060, 14060, 14060 }, //ALTIN
		{ 50627, 14080, 14080, 14080 }, //YESIMTASI
		{ 50628, 14100, 16100, 17100 }, //ABANOZ
		{ 50629, 14120, 16120, 17120 }, //INCI
		{ 50630, 14140, 16140, 17140 }, //BEYAZ ALTIN
		{ 50631, 14160, 16160, 17160 }, //KRISTAL
		{ 50632, 14180, 16180, 17180 }, //MOR YAKUT
		{ 50633, 14200, 16200, 17200 }, //CENNET OC
		{ 50634, 14220, 16220, 17220 }, //RUH KRISTALI
		{ 50635, 14500, 16500, 17500 }, //YAKUT
		{ 50636, 14520, 16520, 17520 }, //GRENA
		{ 50637, 14540, 16540, 17540 }, //ZUMRUT
		{ 50638, 14560, 16560, 17560 }, //SAFIR
		{ 50639, 14570, 16570, 17570 }, //TURMALIN
		{ 50640, 14580, 16580, 17580 }, // KARA EJDERHA
		{ 50640, 14590, 16590, 17590 }, // KARA EJDERHA
		{ 50641, 14600, 16600, 17600 } // Kyanit
	};

	DWORD item_type = (item->GetVnum() / 10) * 10;
	for (size_t i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch (item->GetSubType())
		{
		case ARMOR_WRIST:
			if (info.wrist == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_NECK:
			if (info.neck == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_EAR:
			if (info.ear == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		}
	}
	if (item->GetSubType() == ARMOR_WRIST)
		vnum -= 14000;
	else if (item->GetSubType() == ARMOR_NECK)
		vnum -= 16000;
	else if (item->GetSubType() == ARMOR_EAR)
		vnum -= 17000;
	else
		return false;

	DWORD type = vnum / 20;

	if (type < 0 || type > 11)
	{
		type = (vnum - 170) / 20;

		if (50623 + type != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16210 && item->GetVnum() <= 16219)
	{
		if (50625 != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16230 && item->GetVnum() <= 16239)
	{
		if (50626 != GetVnum())
			return false;
		else
			return true;
	}

	return 50623 + type == GetVnum();
}

bool CItem::CheckItemUseLevel(int nLevel)
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			if (this->m_pProto->aLimits[i].lValue > nLevel) return false;
			else return true;
		}
	}
	return true;
}

long CItem::FindApplyValue(BYTE bApplyType)
{
	if (m_pProto == NULL)
		return 0;

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == bApplyType)
			return m_pProto->aApplies[i].lValue;
	}

	return 0;
}

void CItem::CopySocketTo(LPITEM pItem)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		pItem->m_alSockets[i] = m_alSockets[i];
	}
}

int CItem::GetAccessorySocketGrade()
{
	return MINMAX(0, GetSocket(0), GetAccessorySocketMaxGrade());
}

int CItem::GetAccessorySocketMaxGrade()
{
	return MINMAX(0, GetSocket(1), ITEM_ACCESSORY_SOCKET_MAX_NUM);
}

int CItem::GetAccessorySocketDownGradeTime()
{
	return MINMAX(0, GetSocket(2), aiAccessorySocketDegradeTime[GetAccessorySocketGrade()]);
}

int CItem::GetLevelLimit()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			return this->m_pProto->aLimits[i].lValue;
		}
	}
	return 0;
}

bool CItem::OnAfterCreatedItem()
{
#if defined(ENABLE_EXTENDED_BLEND_AFFECT) && defined(ENABLE_SOCKET_LIMIT_5)
	if (IsBlendItem())
	{
		if (GetSocket(3) == true)
		{
			Lock(true);
			StartBlendExpireEvent();
		}
	}
#endif
	if (-1 != this->GetProto()->cLimitRealTimeFirstUseIndex)
	{
		if (0 != GetSocket(1))
		{
			StartRealTimeExpireEvent();
		}
	}

#ifdef ENABLE_BUFFI_SYSTEM
	if (IsBuffiItem())
	{
		if (GetSocket(3))
			Lock(true);
	}
#endif
	return true;
}

bool CItem::IsDragonSoul()
{
	return GetType() == ITEM_DS;
}

int CItem::GiveMoreTime_Per(float fPercent)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		DWORD given_time = fPercent * duration / 100u;
		if (remain_sec == duration)
			return false;
		if ((given_time + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, given_time + remain_sec);
			return given_time;
		}
	}

	else
		return 0;
}

int CItem::GiveMoreTime_Fix(DWORD dwTime)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		if (remain_sec == duration)
			return false;
		if ((dwTime + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, dwTime + remain_sec);
			return dwTime;
		}
	}

	else
		return 0;
}

int	CItem::GetDuration()
{
	if (!GetProto())
		return -1;

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return GetProto()->aLimits[i].lValue;
	}

	if (GetProto()->cLimitTimerBasedOnWearIndex >= 0)
	{
		BYTE cLTBOWI = GetProto()->cLimitTimerBasedOnWearIndex;
		return GetProto()->aLimits[cLTBOWI].lValue;
	}

	return -1;
}

bool CItem::IsSameSpecialGroup(const LPITEM item) const
{
	if (this->GetVnum() == item->GetVnum())
		return true;

	if (GetSpecialGroup() && (item->GetSpecialGroup() == GetSpecialGroup()))
		return true;

	return false;
}

#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
const char* CItem::GetName()
{
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLocale = GetOwner() ? GetOwner()->GetLanguage() : LOCALE_YMIR;
#endif
	static char szItemName[128];
	memset(szItemName, 0, sizeof(szItemName));
	if (GetProto())
	{
		int len = 0;
		switch (GetType())
		{
		case ITEM_POLYMORPH:
		{
			const DWORD dwMobVnum = GetSocket(0);
			const CMob* pMob = CMobManager::instance().Get(dwMobVnum);
			if (pMob)
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
				len = snprintf(szItemName, sizeof(szItemName), "%s", LC_LOCALE_MOB_TEXT(dwMobVnum, bLocale));
#else
				len = snprintf(szItemName, sizeof(szItemName), "%s", pMob->m_table.szLocaleName);
#endif
			break;
		}
/*		case ITEM_SKILLBOOK:
		case ITEM_SKILLFORGET:
		{
			const DWORD dwSkillVnum = (GetVnum() == ITEM_SKILLBOOK_VNUM || GetVnum() == ITEM_SKILLFORGET_VNUM) ? GetSocket(0) : 0;
			const CSkillProto* pSkill = (dwSkillVnum != 0) ? CSkillManager::instance().Get(dwSkillVnum) : NULL;
			if (pSkill)
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
				len = snprintf(szItemName, sizeof(szItemName), "%s", LC_LOCALE_SKILL_TEXT(dwSkillVnum, bLocale));
#else
				len = snprintf(szItemName, sizeof(szItemName), "%s", pSkill->szName);
#endif
			break;
		}*/
		}
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		len += snprintf(szItemName + len, sizeof(szItemName) - len, (len > 0) ? " %s" : "%s", LC_LOCALE_ITEM_TEXT(GetVnum(), bLocale));
#else
		len += snprintf(szItemName + len, sizeof(szItemName) - len, (len > 0) ? " %s" : "%s", GetProto()->szLocaleName);
#endif
	}
	return szItemName;
}

std::string CItem::GetNameString()
{
	static char szItemName[128];
	memset(szItemName, 0, sizeof(szItemName));
	if (GetProto())
	{
		int len = 0;
		switch (GetType())
		{
		case ITEM_POLYMORPH:
		{
			const DWORD dwMobVnum = GetSocket(0);
			const CMob* pMob = CMobManager::instance().Get(dwMobVnum);
			if (pMob)
				len = snprintf(szItemName, sizeof(szItemName), "%s", pMob->m_table.szLocaleName);

			break;
		}
		case ITEM_SKILLBOOK:
		case ITEM_SKILLFORGET:
		{
			const DWORD dwSkillVnum = (GetVnum() == ITEM_SKILLBOOK_VNUM || GetVnum() == ITEM_SKILLFORGET_VNUM) ? GetSocket(0) : 0;
			const CSkillProto* pSkill = (dwSkillVnum != 0) ? CSkillManager::instance().Get(dwSkillVnum) : NULL;
			if (pSkill)
				len = snprintf(szItemName, sizeof(szItemName), "%s", pSkill->szName);
			break;
		}
		}
		len += snprintf(szItemName + len, sizeof(szItemName) - len, (len > 0) ? " %s" : "%s", GetProto()->szLocaleName);
	}
	std::string returnSzItemName = szItemName;
	return returnSzItemName;
}
#endif

#ifdef ENABLE_SPECIAL_STORAGE
bool CItem::IsUpgradeItem()
{
	if (GetType() == ITEM_MATERIAL && GetSubType() == MATERIAL_LEATHER)
		return true;

	switch (GetVnum())
	{
		case 27992:
		case 27993:
		case 27994:
		{
			return true;
			break;
		}
		default: break;
	}
	return false;
}

bool CItem::IsBook()
{
	if (GetType() == ITEM_SKILLBOOK)
		return true;

	/*
	switch (GetVnum())
	{
	case 27992:
	case 27993:
	case 27994:
	{
		return true;
		break;
	}
	default: break;
	}*/
	return false;
}

bool CItem::IsStone()
{
	if (GetType() == ITEM_METIN && GetSubType() == METIN_NORMAL)
		return true;

	/*
	switch (GetVnum())
	{
	case 27992:
	case 27993:
	case 27994:
	{
		return true;
		break;
	}
	default: break;
	}*/
	return false;
}

bool CItem::IsChest()
{
	if (NotChestWindowItem(GetVnum()))
		return false;

	if (GetType() == ITEM_GIFTBOX)
		return true;
	
	switch (GetVnum())
	{
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
			return true;
			break;
		}
		default: break;
	}
	return false;
}
#endif

#ifdef ENABLE_ATTR_RARE_RENEWAL
bool CItem::IsRareAttrItem(int attrIndex)
{
	if (GetValue(ATTRIBUTE_RARE_VALUE_NUM) == attrIndex)
		return true;

	return false;
}
#endif

#ifdef ENABLE_JEWELS_RENEWAL
bool CItem::IsAccessoryForSocketNew()
{
	switch (m_pProto->bType)
	{
		case ITEM_ARMOR:
		{
			switch (m_pProto->bSubType)
			{
				case ARMOR_HEAD:
				case ARMOR_SHIELD:
				case ARMOR_FOOTS:
					return true;

				default: break;
			}
			break;
		}

		case ITEM_CROWN: {return true; }
		case ITEM_PENDANT: {return true; }
		case ITEM_GLOVE: {return true; }

		default: break;
	}
	return false;
}

bool CItem::CanPutIntoNew(LPITEM item)
{
	DWORD jewelsVnum = item->GetVnum();
	switch (m_pProto->bType)
	{
		case ITEM_ARMOR:
		{
			switch (m_pProto->bSubType)
			{
				case ARMOR_HEAD:
				{
					if (jewelsVnum == 24700 || jewelsVnum == 24701 || jewelsVnum == 24702)
						return true;
					break;
				}
				case ARMOR_SHIELD:
				{
					if (jewelsVnum == 24703 || jewelsVnum == 24704 || jewelsVnum == 24705)
						return true;
					break;
				}
				case ARMOR_FOOTS:
				{
					if (jewelsVnum == 24706 || jewelsVnum == 24707 || jewelsVnum == 24708)
						return true;
					break;
				}
				default: break;
			}
			break;
		}

		case ITEM_CROWN:
		{
			if (jewelsVnum == 24709 || jewelsVnum == 24710 || jewelsVnum == 24711)
				return true;
			break;
		}
		case ITEM_PENDANT:
		{
			if (jewelsVnum == 24712 || jewelsVnum == 24713 || jewelsVnum == 24714)
				return true;
			break;
		}
		case ITEM_GLOVE:
		{
			if (jewelsVnum == 24715 || jewelsVnum == 24716 || jewelsVnum == 24717)
				return true;
			break;
		}

		default: break;
	}
	return false;
}



bool CItem::CanPutIntoPerma(LPITEM item)
{
	struct JewelAccessoryInfo
	{
		DWORD jewel;
		DWORD wrist;
		DWORD neck;
		DWORD ear;
	};
	
	const static JewelAccessoryInfo infos[] = {
		{ 950623, 14000, 16000, 17000 }, //Fosil oc
		{ 950624, 14020, 16020, 17020 }, //BAKIR
		{ 950625, 14040, 14040, 14040 }, //GUMUS
		{ 950626, 14060, 14060, 14060 }, //ALTIN
		{ 950627, 14080, 14080, 14080 }, //YESIMTASI
		{ 950628, 14100, 16100, 17100 }, //ABANOZ
		{ 950629, 14120, 16120, 17120 }, //INCI
		{ 950630, 14140, 16140, 17140 }, //BEYAZ ALTIN
		{ 950631, 14160, 16160, 17160 }, //KRISTAL
		{ 950632, 14180, 16180, 17180 }, //MOR YAKUT
		{ 950633, 14200, 16200, 17200 }, //CENNET OC
		{ 950634, 14220, 16220, 17220 }, //RUH KRISTALI
		{ 950635, 14500, 16500, 17500 }, //YAKUT
		{ 950636, 14520, 16520, 17520 }, //GRENA
		{ 950637, 14540, 16540, 17540 }, //ZUMRUT
		{ 950638, 14560, 16560, 17560 }, //SAFIR
		{ 950639, 14570, 16570, 17570 }, //TURMALIN
		{ 950640, 14580, 16580, 17580 }, // KARA EJDERHA
		{ 950640, 14590, 16590, 17590 }, // KARA EJDERHA
		{ 950641, 14600, 16600, 17600 } // Kyanit
	};

	DWORD item_type = (item->GetVnum() / 10) * 10;
	for (size_t i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch (item->GetSubType())
		{
		case ARMOR_WRIST:
			if (info.wrist == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_NECK:
			if (info.neck == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_EAR:
			if (info.ear == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		}
	}
	return false;
}
#endif

#ifdef ENABLE_NAMING_SCROLL
int CItem::BonusRate()
{
	int ret = -1;
	switch (GetType())
	{
		case ITEM_PET:
		{
			CAffect* petAffect = m_pOwner->FindAffect(AFFECT_NAMING_SCROLL_PET);
			if (petAffect)
				ret = petAffect->lApplyValue;
			break;
		}

		case ITEM_COSTUME:
		{
			if (GetSubType() == COSTUME_MOUNT)
			{
				CAffect* mountAffect = m_pOwner->FindAffect(AFFECT_NAMING_SCROLL_MOUNT);
				if (mountAffect)
					ret = mountAffect->lApplyValue;
			}
			break;
		}
#ifdef ENABLE_BOOSTER_ITEM
		case ITEM_WEAPON:
		{
			LPITEM booster;
			if ((booster = m_pOwner->GetWear(WEAR_BOOSTER_WEAPON)))
			{
				ret = booster->GetValue(0);
				if (m_pOwner->GetBoosterSetBonus())
					ret += 5;	
			}
			break;
		}
		case ITEM_ARMOR:
		{
			if (GetSubType() == ARMOR_BODY)
			{
				LPITEM booster;
				if ((booster = m_pOwner->GetWear(WEAR_BOOSTER_BODY)))
				{
					ret = booster->GetValue(0);
					if (m_pOwner->GetBoosterSetBonus())
						ret += 5;
				}
			}
			else if (GetSubType() == ARMOR_HEAD)
			{
				LPITEM booster;
				if ((booster = m_pOwner->GetWear(WEAR_BOOSTER_HEAD)))
				{
					ret = booster->GetValue(0);
					if (m_pOwner->GetBoosterSetBonus())
						ret += 5;
				}
			}
			break;
		}
#endif
		default:
			break;
	}

	return ret;
}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
int CItem::FindBuffiEquipCell()
{
	switch (GetType())
	{
		case ITEM_COSTUME:
		{
			switch (GetSubType())
			{
				case COSTUME_BODY: {return BUFFI_BODY_SLOT; }
				case COSTUME_HAIR: {return BUFFI_HAIR_SLOT; }
				case COSTUME_ACCE: {return BUFFI_ACCE_SLOT; }
				case COSTUME_WING: {return BUFFI_ACCE_SLOT; }
				case COSTUME_WEAPON: {return BUFFI_WEAPON_SLOT; }
				case COSTUME_AURA: {return BUFFI_AURA_SLOT; }
				default:break;
			}
		}
		case ITEM_AURA_SKIN:
			return BUFFI_AURA_SLOT;
		case ITEM_ARMOR:
		{
			if (GetSubType() == ARMOR_BODY)
				return BUFFI_BODY_SLOT;
			break;
		}
		case ITEM_WEAPON:
		{
			if (GetSubType() == WEAPON_BELL || GetSubType() == WEAPON_FAN)
				return BUFFI_WEAPON_SLOT;
			break;
		}
		default:break;
	}

	return -1;
}

bool CItem::IsBuffiItem()
{
	switch (GetVnum())
	{
		case 71992:
		case 71993:
		case 71994:
		case 71995:
		case 71996:
		case 71997:
		case 71998:
		case 71999:
		case 64010:
			return true;
		default:
			return false;
	}
	return false;
}

#endif

