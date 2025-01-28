#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "db.h"

#include "skill.h"
#include "text_file_loader.h"
#include "priv_manager.h"
#include "questmanager.h"
#include "unique_item.h"
#include "safebox.h"
#include "blend_item.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"

#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#ifndef ENABLE_CUBE_RENEWAL_WORLDARD
#include "cube.h"
#else
#include "cuberenewal.h"
#endif

ITEM_MANAGER::ITEM_MANAGER()
	: m_iTopOfTable(0), m_dwVIDCount(0), m_dwCurrentID(0)
{
	m_ItemIDRange.dwMin = m_ItemIDRange.dwMax = m_ItemIDRange.dwUsableItemIDMin = 0;
	m_ItemIDSpareRange.dwMin = m_ItemIDSpareRange.dwMax = m_ItemIDSpareRange.dwUsableItemIDMin = 0;
}

ITEM_MANAGER::~ITEM_MANAGER()
{
	Destroy();
}

void ITEM_MANAGER::Destroy()
{
	itertype(m_VIDMap) it = m_VIDMap.begin();
	for (; it != m_VIDMap.end(); ++it) {
		M2_DELETE(it->second);
	}
	m_VIDMap.clear();
}

void ITEM_MANAGER::GracefulShutdown()
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.begin();

	while (it != m_set_pkItemForDelayedSave.end())
		SaveSingleItem(*(it++));

	m_set_pkItemForDelayedSave.clear();
}

bool ITEM_MANAGER::Initialize(TItemTable* table, int size)
{
	if (!m_vec_prototype.empty())
		m_vec_prototype.clear();

	int	i;

	m_vec_prototype.resize(size);
	thecore_memcpy(&m_vec_prototype[0], table, sizeof(TItemTable) * size);
	for (int i = 0; i < size; i++)
	{
		if (0 != m_vec_prototype[i].dwVnumRange)
		{
			m_vec_item_vnum_range_info.push_back(&m_vec_prototype[i]);
		}
	}

	m_map_ItemRefineFrom.clear();
	for (i = 0; i < size; ++i)
	{
		if (m_vec_prototype[i].dwRefinedVnum)
			m_map_ItemRefineFrom.insert(std::make_pair(m_vec_prototype[i].dwRefinedVnum, m_vec_prototype[i].dwVnum));

		if (m_vec_prototype[i].bType == ITEM_QUEST || IS_SET(m_vec_prototype[i].dwFlags, ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			quest::CQuestManager::instance().RegisterNPCVnum(m_vec_prototype[i].dwVnum);

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		if (m_vec_prototype[i].bType == ITEM_COSTUME && m_vec_prototype[i].bSubType == COSTUME_MOUNT)
			quest::CQuestManager::instance().RegisterNPCVnum(m_vec_prototype[i].dwVnum);
#endif

#ifdef ENABLE_PET_COSTUME_SYSTEM
		if (m_vec_prototype[i].bType == ITEM_PET)
			quest::CQuestManager::instance().RegisterNPCVnum(m_vec_prototype[i].dwVnum);
#endif

		m_map_vid.insert(std::map<DWORD, TItemTable>::value_type(m_vec_prototype[i].dwVnum, m_vec_prototype[i]));
	}

	int len = 0, len2;
	char buf[512];

	for (i = 0; i < size; ++i)
	{
		len2 = snprintf(buf + len, sizeof(buf) - len, "%5u %-16s", m_vec_prototype[i].dwVnum, m_vec_prototype[i].szLocaleName);

		if (len2 < 0 || len2 >= (int)sizeof(buf) - len)
			len += (sizeof(buf) - len) - 1;
		else
			len += len2;

		if (!((i + 1) % 4))
		{
			len = 0;
		}
		else
		{
			buf[len++] = '\t';
			buf[len] = '\0';
		}
	}

	ITEM_VID_MAP::iterator it = m_VIDMap.begin();

	while (it != m_VIDMap.end())
	{
		LPITEM item = it->second;
		++it;

		const TItemTable* tableInfo = GetTable(item->GetOriginalVnum());

		if (NULL == tableInfo)
		{
			sys_err("cannot reset item table");
			item->SetProto(NULL);
		}

		item->SetProto(tableInfo);
	}

	return true;
}

LPITEM ITEM_MANAGER::CreateItem(DWORD vnum, DWORD count, DWORD id, bool bTryMagic, int iRarePct, bool bSkipSave)
{
	if (0 == vnum)
		return NULL;

	DWORD dwMaskVnum = 0;
	if (GetMaskVnum(vnum))
	{
		dwMaskVnum = GetMaskVnum(vnum);
	}

	const TItemTable* table = GetTable(vnum);

	if (NULL == table)
		return NULL;

	LPITEM item = NULL;

	if (m_map_pkItemByID.find(id) != m_map_pkItemByID.end())
	{
		item = m_map_pkItemByID[id];
		LPCHARACTER owner = item->GetOwner();
		if (!owner)	//@fixme527
			return nullptr;
		sys_err("ITEM_ID_DUP: %u vnum %u owner %p", id, item->GetVnum(), get_pointer(owner));
		return NULL;
	}

	item = M2_NEW CItem(vnum);

	bool bIsNewItem = (0 == id);

	item->Initialize();
	item->SetProto(table);
	item->SetMaskVnum(dwMaskVnum);

	if (item->GetType() == ITEM_ELK)
		item->SetSkipSave(true);

	else if (!bIsNewItem)
	{
		item->SetID(id);
		item->SetSkipSave(true);
	}
	else
	{
		item->SetID(GetNewID());

		if (item->GetType() == ITEM_UNIQUE)
		{
			if (item->GetValue(2) == 0)
				item->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, item->GetValue(0));
			else
			{
				//int globalTime = get_global_time();
				//int lastTime = item->GetValue(0);
				//int endTime = get_global_time() + item->GetValue(0);
				item->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, get_global_time() + item->GetValue(0));
			}
		}
	}

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
	if (item->GetVnum()== 51002)
		item->SetSocket(1, false);
#endif

	if (item->GetType() == ITEM_ELK)
		;
	else if (item->IsStackable())
	{
		count = MINMAX(1, count, g_bItemCountLimit);

		if (bTryMagic && count <= 1 && IS_SET(item->GetFlag(), ITEM_FLAG_MAKECOUNT))
			count = item->GetValue(1);
	}
	else
		count = 1;

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
	if (item->IsBlendItem() && count > 1)
		count = 1;
#endif

	item->SetVID(++m_dwVIDCount);

	if (bSkipSave == false)
		m_VIDMap.insert(ITEM_VID_MAP::value_type(item->GetVID(), item));

	if (item->GetID() != 0 && bSkipSave == false)
		m_map_pkItemByID.insert(std::map<DWORD, LPITEM>::value_type(item->GetID(), item));

	if (!item->SetCount(count))
		return NULL;

	item->SetSkipSave(false);

	if (item->GetType() == ITEM_UNIQUE && item->GetValue(2) != 0)
		item->StartUniqueExpireEvent();

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
	{
		if (LIMIT_REAL_TIME == item->GetLimitType(i))
		{
#ifdef ENABLE_EVENT_MANAGER
			if (eventItemList.find(item->GetVnum()) != eventItemList.end())
			{
				item->SetSocket(0, GetEventItemDestroyTime(item->GetVnum()));
			}
			else
			{
				if (item->GetLimitValue(i))
				{
					item->SetSocket(0, time(0) + item->GetLimitValue(i));
				}
				else
				{
					item->SetSocket(0, time(0) + 60 * 60 * 24 * 7);
				}
			}
#else
			if (item->GetLimitValue(i))
			{
				item->SetSocket(0, time(0) + item->GetLimitValue(i));
			}
			else
			{
				item->SetSocket(0, time(0) + 60 * 60 * 24 * 7);
			}
#endif

			item->StartRealTimeExpireEvent();
		}

		else if (LIMIT_TIMER_BASED_ON_WEAR == item->GetLimitType(i))
		{
			if (true == item->IsEquipped())
			{
				item->StartTimerBasedOnWearExpireEvent();
			}
			else if (0 == id)
			{
				long duration = item->GetSocket(0);
				if (0 == duration)
					duration = item->GetLimitValue(i);

				if (0 == duration)
					duration = 60 * 60 * 10;

				item->SetSocket(0, duration);
			}
		}
	}

	if (id == 0)
	{
		if (ITEM_BLEND == item->GetType())
		{
			if (Blend_Item_find(item->GetVnum()))
			{
				Blend_Item_set_value(item);
				return item;
			}
		}

		if (table->sAddonType)
		{
			item->ApplyAddon(table->sAddonType);
		}

		if (bTryMagic)
		{
			if (iRarePct == -1)
				iRarePct = table->bAlterToMagicItemPct;

			if (number(1, 100) <= iRarePct)
				item->AlterToMagicItem();
		}

		if (table->bGainSocketPct)
			item->AlterToSocketItem(table->bGainSocketPct);

		if (vnum == 50300 || vnum == ITEM_SKILLFORGET_VNUM)
		{
			extern const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);
			item->SetSocket(0, GetRandomSkillVnum());
		}
		else if (ITEM_SKILLFORGET2_VNUM == vnum)
		{
			DWORD dwSkillVnum;

			do
			{
				dwSkillVnum = number(112, 119);

				if (NULL != CSkillManager::instance().Get(dwSkillVnum))
					break;
			} while (true);

			item->SetSocket(0, dwSkillVnum);
		}
	}
	else
	{
		if (100 == table->bAlterToMagicItemPct && 0 == item->GetAttributeCount())
		{
			item->AlterToMagicItem();
		}
	}

	if (item->GetType() == ITEM_QUEST)
	{
		for (itertype(m_map_pkQuestItemGroup) it = m_map_pkQuestItemGroup.begin(); it != m_map_pkQuestItemGroup.end(); it++)
		{
			if (it->second->m_bType == CSpecialItemGroup::QUEST && it->second->Contains(vnum))
			{
				item->SetSIGVnum(it->first);
			}
		}
	}
	else if (item->GetType() == ITEM_UNIQUE)
	{
		for (itertype(m_map_pkSpecialItemGroup) it = m_map_pkSpecialItemGroup.begin(); it != m_map_pkSpecialItemGroup.end(); it++)
		{
			if (it->second->m_bType == CSpecialItemGroup::SPECIAL && it->second->Contains(vnum))
			{
				item->SetSIGVnum(it->first);
			}
		}
	}

	if (item->IsDragonSoul() && 0 == id)
	{
		DSManager::instance().DragonSoulItemInitialize(item);
	}

	return item;
}

void ITEM_MANAGER::DelayedSave(LPITEM item)
{
	if (item->GetID() != 0)
		m_set_pkItemForDelayedSave.insert(item);
}

void ITEM_MANAGER::FlushDelayedSave(LPITEM item)
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.find(item);

	if (it == m_set_pkItemForDelayedSave.end())
	{
		return;
	}

	m_set_pkItemForDelayedSave.erase(it);
	SaveSingleItem(item);
}

void ITEM_MANAGER::FlushDelayedSave()
{
    TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.begin();
    TR1_NS::unordered_set<LPITEM>::iterator this_it;

    while (it != m_set_pkItemForDelayedSave.end())
    {
        this_it = it++;
        SaveSingleItem(*this_it);
        m_set_pkItemForDelayedSave.erase(this_it);
    }

    if (db_clientdesc)
        db_clientdesc->FlushOutput();
}

void ITEM_MANAGER::SaveSingleItem(LPITEM item)
{
	if (!item->GetOwner())
	{
		DWORD dwID = item->GetID();
		DWORD dwOwnerID = item->GetLastOwnerPID();

		db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_DESTROY, 0, sizeof(DWORD) + sizeof(DWORD));
		db_clientdesc->Packet(&dwID, sizeof(DWORD));
		db_clientdesc->Packet(&dwOwnerID, sizeof(DWORD));
		return;
	}

	TPlayerItem t;

	t.id = item->GetID();
	t.window = item->GetWindow();
	switch (t.window)
	{
	case EQUIPMENT:
		t.pos = item->GetCell() - INVENTORY_MAX_NUM;
		break;
#ifdef ENABLE_BELT_INVENTORY_EX
	case INVENTORY:
		if (BELT_INVENTORY_SLOT_START <= item->GetCell() && BELT_INVENTORY_SLOT_END > item->GetCell())
		{
			t.window = BELT_INVENTORY;
			t.pos = item->GetCell() - BELT_INVENTORY_SLOT_START;
			break;
		}
#endif
	default:
		t.pos = item->GetCell();
		break;
	}
	t.count = item->GetCount();
	t.vnum = item->GetOriginalVnum();
	switch (t.window)
	{
	case SAFEBOX:
	case MALL:
		t.owner = item->GetOwner()->GetDesc()->GetAccountTable().id;
		break;
	default:
		t.owner = item->GetOwner()->GetPlayerID();
		break;
	}
	thecore_memcpy(t.alSockets, item->GetSockets(), sizeof(t.alSockets));
	thecore_memcpy(t.aAttr, item->GetAttributes(), sizeof(t.aAttr));

	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_SAVE, 0, sizeof(TPlayerItem));
	db_clientdesc->Packet(&t, sizeof(TPlayerItem));
}

void ITEM_MANAGER::Update()
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.begin();
	TR1_NS::unordered_set<LPITEM>::iterator this_it;

	while (it != m_set_pkItemForDelayedSave.end())
	{
		this_it = it++;
		LPITEM item = *this_it;

		if (item->GetOwner() && IS_SET(item->GetFlag(), ITEM_FLAG_SLOW_QUERY))
			continue;

		SaveSingleItem(item);

		m_set_pkItemForDelayedSave.erase(this_it);
	}
}

void ITEM_MANAGER::RemoveItem(LPITEM item, const char* c_pszReason)
{
	LPCHARACTER o;

	if ((o = item->GetOwner()))
	{
		// SAFEBOX_TIME_LIMIT_ITEM_BUG_FIX
		if (item->GetWindow() == MALL || item->GetWindow() == SAFEBOX)
		{
			CSafebox* pSafebox = item->GetWindow() == MALL ? o->GetMall() : o->GetSafebox();
			if (pSafebox)
			{
				pSafebox->Remove(item->GetCell());
			}
		}
		// END_OF_SAFEBOX_TIME_LIMIT_ITEM_BUG_FIX
		else
		{
			o->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), UINT16_MAX);
			item->RemoveFromCharacter();
		}
	}
	M2_DESTROY_ITEM(item);
}

#ifndef DEBUG_ALLOC
void ITEM_MANAGER::DestroyItem(LPITEM item)
#else
void ITEM_MANAGER::DestroyItem(LPITEM item, const char* file, size_t line)
#endif
{
	if (!item)
	{
		sys_err("DestroyItem:: item yok");
		return;
	}

	if (item->GetSectree())
		item->RemoveFromGround();

	if (item->GetOwner())
	{
		if (CHARACTER_MANAGER::instance().Find(item->GetOwner()->GetPlayerID()) != NULL)
		{
			sys_err("DestroyItem: GetOwner %s %s!!", item->GetName(), item->GetOwner()->GetName());
			item->RemoveFromCharacter();
		}
		else
		{
			sys_err("WTH! Invalid item owner. owner pointer : %p", item->GetOwner());
		}
	}

	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.find(item);

	if (it != m_set_pkItemForDelayedSave.end())
		m_set_pkItemForDelayedSave.erase(it);

	DWORD dwID = item->GetID();

	if (!item->GetSkipSave() && dwID)
	{
		DWORD dwOwnerID = item->GetLastOwnerPID();

		db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_DESTROY, 0, sizeof(DWORD) + sizeof(DWORD));
		db_clientdesc->Packet(&dwID, sizeof(DWORD));
		db_clientdesc->Packet(&dwOwnerID, sizeof(DWORD));
	}
	else
	{
		sys_log(2, "ITEM_DESTROY_SKIP %s:%u (skip=%d)", item->GetName(), dwID, item->GetSkipSave());
	}

	if (dwID)
		m_map_pkItemByID.erase(dwID);

	m_VIDMap.erase(item->GetVID());

	
#ifndef DEBUG_ALLOC
	M2_DELETE(item);
#else
	M2_DELETE_EX(item, file, line);
#endif
}

LPITEM ITEM_MANAGER::Find(DWORD id)
{
	itertype(m_map_pkItemByID) it = m_map_pkItemByID.find(id);
	if (it == m_map_pkItemByID.end())
		return NULL;
	return it->second;
}

LPITEM ITEM_MANAGER::FindByVID(DWORD vid)
{
	ITEM_VID_MAP::iterator it = m_VIDMap.find(vid);

	if (it == m_VIDMap.end())
		return NULL;

	return (it->second);
}

TItemTable* ITEM_MANAGER::GetTable(DWORD vnum)
{
	if (vnum == 0)
		return nullptr;

	int rnum = RealNumber(vnum);

	if (rnum < 0)
	{
		for (size_t i = 0; i < m_vec_item_vnum_range_info.size(); i++)
		{
			TItemTable* p = m_vec_item_vnum_range_info[i];
			if (p)
			{
				if ((p->dwVnum < vnum) &&
					vnum < (p->dwVnum + p->dwVnumRange))
				{
					return p;
				}
			}
		}

		return nullptr;
	}

	return &m_vec_prototype[rnum];
}

int ITEM_MANAGER::RealNumber(DWORD vnum)
{
	int bot, top, mid;

	bot = 0;
	top = m_vec_prototype.size();

	TItemTable* pTable = &m_vec_prototype[0];

	while (1)
	{
		mid = (bot + top) >> 1;

		if ((pTable + mid)->dwVnum == vnum)
			return (mid);

		if (bot >= top)
			return (-1);

		if ((pTable + mid)->dwVnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

bool ITEM_MANAGER::GetVnum(const char* c_pszName, DWORD& r_dwVnum)
{
	//Bu kodu yazarken arkadaþlar bonzai çekmiþ
	return false;
	int len = strlen(c_pszName);

	TItemTable* pTable = &m_vec_prototype[0];

	for (DWORD i = 0; i < m_vec_prototype.size(); ++i, ++pTable)
	{
		if (!strncasecmp(c_pszName, pTable->szLocaleName, len))
		{
			r_dwVnum = pTable->dwVnum;
			return true;
		}
	}

	return false;
}

bool ITEM_MANAGER::GetVnumByOriginalName(const char* c_pszName, DWORD& r_dwVnum)
{
	//Bu kodu yazarken arkadaþlar bonzai çekmiþ
	return false;
	int len = strlen(c_pszName);

	TItemTable* pTable = &m_vec_prototype[0];

	for (DWORD i = 0; i < m_vec_prototype.size(); ++i, ++pTable)
	{
		if (!strncasecmp(c_pszName, pTable->szName, len))
		{
			r_dwVnum = pTable->dwVnum;
			return true;
		}
	}

	return false;
}

class CItemDropInfo
{
public:
	CItemDropInfo(int iLevelStart, int iLevelEnd, int iPercent, DWORD dwVnum) :
		m_iLevelStart(iLevelStart), m_iLevelEnd(iLevelEnd), m_iPercent(iPercent), m_dwVnum(dwVnum)
	{
	}

	int	m_iLevelStart;
	int	m_iLevelEnd;
	int	m_iPercent; // 1 ~ 1000
	DWORD	m_dwVnum;

	friend bool operator < (const CItemDropInfo& l, const CItemDropInfo& r)
	{
		return l.m_iLevelEnd < r.m_iLevelEnd;
	}
};

int GetDropPerKillPct(int iMinimum, int iDefault, int iDeltaPercent, const char* c_pszFlag)
{
	int iVal = 0;

	if ((iVal = quest::CQuestManager::instance().GetEventFlag(c_pszFlag)))
	{
		if (iVal < iMinimum)
			iVal = iDefault;

		if (iVal < 0)
			iVal = iDefault;
	}

	if (iVal == 0)
		return 0;

	return (40000 * iDeltaPercent / iVal);
}

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#ifndef ENABLE_TARGET_INFORMATION_RENEWAL
bool ITEM_MANAGER::CreateDropItemVector(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item)
{
	if (!pkChr || !pkKiller) {
		return false;
	}

	if (pkChr->IsPolymorphed() || pkChr->IsPC()) {
		return false;
	}
	LPITEM item = NULL;

	// Drop Item Group
	{
		itertype(m_map_pkDropItemGroup) it;
		it = m_map_pkDropItemGroup.find(pkChr->GetRaceNum());

		if (it != m_map_pkDropItemGroup.end())
		{
			decltype(it->second->GetVector()) v = it->second->GetVector();

			for (DWORD i = 0; i < v.size(); ++i)
			{
				item = CreateItem(v[i].dwVnum, v[i].iCount, 0, true);

				if (item)
				{
					if (item->GetType() == ITEM_POLYMORPH)
					{
						if (item->GetVnum() == pkChr->GetPolymorphItemVnum())
						{
							item->SetSocket(0, pkChr->GetRaceNum());
						}
					}

					vec_item.push_back(item);
				}
			}
		}
	}

	if (pkChr->IsStone())
	{
		if (pkChr->GetDropMetinStoneVnum())
		{
			item = CreateItem(pkChr->GetDropMetinStoneVnum(), 1, 0, true);
			if (item) vec_item.push_back(item);
		}
	}

	return vec_item.size();
}
#endif
#endif

bool ITEM_MANAGER::GetDropPct(LPCHARACTER pkChr, LPCHARACTER pkKiller, OUT int& iDeltaPercent, OUT int& iRandRange)
{
	if (NULL == pkChr || NULL == pkKiller)
		return false;
	iDeltaPercent = 100;

	if (!pkChr->IsStone() && pkChr->GetMobRank() >= MOB_RANK_BOSS)
		iDeltaPercent = PERCENT_LVDELTA_BOSS(pkKiller->GetLevel(), pkChr->GetLevel());
	else
		iDeltaPercent = PERCENT_LVDELTA(pkKiller->GetLevel(), pkChr->GetLevel());

	if (1 == number(1, 50000))
		iDeltaPercent += 1000;
	else if (1 == number(1, 10000))
		iDeltaPercent += 500;

	iDeltaPercent = iDeltaPercent * CHARACTER_MANAGER::instance().GetMobItemRate(pkKiller) / 100;

	// ADD_PREMIUM
	if (pkKiller->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0 ||
		pkKiller->IsEquipUniqueGroup(UNIQUE_GROUP_DOUBLE_ITEM))
		iDeltaPercent += iDeltaPercent;
	// END_OF_ADD_PREMIUM

	iRandRange = 4000000;
	iRandRange = iRandRange * 100 /
		(100 +
			CPrivManager::instance().GetPriv(pkKiller, PRIV_ITEM_DROP) +
			(pkKiller->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_ITEM) ? 100 : 0) +
			(pkKiller->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_ITEM_PLUS) ? 150 : 0));
	return true;
}

bool ITEM_MANAGER::CreateDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item)
{
	int iDeltaPercent, iRandRange;
	if (!GetDropPct(pkChr, pkKiller, iDeltaPercent, iRandRange))
		return false;

	LPITEM item = NULL;

	{
		itertype(m_map_pkDropItemGroup) it;
		it = m_map_pkDropItemGroup.find(pkChr->GetRaceNum());

		if (it != m_map_pkDropItemGroup.end())
		{
			typeof(it->second->GetVector()) v = it->second->GetVector();
			for (DWORD i = 0; i < v.size(); ++i)
			{
				int iPercent = (v[i].dwPct * iDeltaPercent) / 100;
				if (iPercent >= number(1, iRandRange))
				{
					item = CreateItem(v[i].dwVnum, v[i].iCount, 0, true);
					if (item)
					{
						if (item->GetType() == ITEM_POLYMORPH)
						{
							if (item->GetVnum() == pkChr->GetPolymorphItemVnum())
							{
								item->SetSocket(0, pkChr->GetRaceNum());
							}
						}
						vec_item.push_back(item);
					}
				}
			}
		}
	}

	if (pkChr->IsStone())
	{
		if (pkChr->GetDropMetinStoneVnum())
		{
			int iPercent = (pkChr->GetDropMetinStonePct() * iDeltaPercent) * 400;

			if (iPercent >= number(1, iRandRange))
			{
				item = CreateItem(pkChr->GetDropMetinStoneVnum(), 1, 0, true);
				if (item) vec_item.push_back(item);
			}
		}
	}

	CreateQuestDropItem(pkChr, pkKiller, vec_item, iDeltaPercent, iRandRange);
#ifdef ENABLE_EVENT_MANAGER
	CHARACTER_MANAGER::Instance().CheckEventForDrop(pkChr, pkKiller, vec_item);
#endif
	return vec_item.size();
}

// ADD_GRANDMASTER_SKILL
int GetThreeSkillLevelAdjust(int level)
{
	if (level < 40)
		return 32;
	if (level < 45)
		return 16;
	if (level < 50)
		return 8;
	if (level < 55)
		return 4;
	if (level < 60)
		return 2;
	return 1;
}

void ITEM_MANAGER::CreateQuestDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item, int iDeltaPercent, int iRandRange)
{
	return;
	/*LPITEM item = NULL;

	if (!pkChr)
		return;

	if (!pkKiller)
		return;*/
}

DWORD ITEM_MANAGER::GetRefineFromVnum(DWORD dwVnum)
{
	itertype(m_map_ItemRefineFrom) it = m_map_ItemRefineFrom.find(dwVnum);
	if (it != m_map_ItemRefineFrom.end())
		return it->second;
	return 0;
}

const CSpecialItemGroup* ITEM_MANAGER::GetSpecialItemGroup(DWORD dwVnum)
{
	itertype(m_map_pkSpecialItemGroup) it = m_map_pkSpecialItemGroup.find(dwVnum);
	if (it != m_map_pkSpecialItemGroup.end())
	{
		return it->second;
	}
	return NULL;
}

const CSpecialAttrGroup* ITEM_MANAGER::GetSpecialAttrGroup(DWORD dwVnum)
{
	itertype(m_map_pkSpecialAttrGroup) it = m_map_pkSpecialAttrGroup.find(dwVnum);
	if (it != m_map_pkSpecialAttrGroup.end())
	{
		return it->second;
	}
	return NULL;
}

DWORD ITEM_MANAGER::GetMaskVnum(DWORD dwVnum)
{
	TMapDW2DW::iterator it = m_map_new_to_ori.find(dwVnum);
	if (it != m_map_new_to_ori.end())
	{
		return it->second;
	}
	else
		return 0;
}

void ITEM_MANAGER::CopyAllAttrTo(LPITEM pkOldItem, LPITEM pkNewItem)
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
			const int ITEM_BROKEN_METIN_VNUM = 28960;
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}
	}

	pkOldItem->CopyAttributeTo(pkNewItem);
}

#ifdef ENABLE_FURKANA_GOTTEN
void ITEM_MANAGER::DropCalculator(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::map<DWORD, int>& map_item, int mobCount)
{
	int iDeltaPercent, iRandRange;
	if (!GetDropPct(pkChr, pkKiller, iDeltaPercent, iRandRange))
		return;

	{
		itertype(m_map_pkDropItemGroup) it;
		it = m_map_pkDropItemGroup.find(pkChr->GetRaceNum());

		if (it != m_map_pkDropItemGroup.end())
		{
			typeof(it->second->GetVector()) v = it->second->GetVector();
			for (int y = 0; y < mobCount; y++)
			{
				for (DWORD i = 0; i < v.size(); ++i)
				{
					int iPercent = (v[i].dwPct * iDeltaPercent) / 100;
					if (iPercent >= number(1, iRandRange))
					{
						auto iter = map_item.find(v[i].dwVnum);
						if (iter == map_item.end())
						{
							map_item.insert(std::make_pair(v[i].dwVnum, v[i].iCount));
						}
						else
						{
							iter->second = iter->second + v[i].iCount;
						}
					}
				}
			}

		}
	}

}
#endif