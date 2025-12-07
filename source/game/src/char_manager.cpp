#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "party.h"
#include "regen.h"
#include "p2p.h"
#include "dungeon.h"
#include "db.h"
#include "config.h"
#include "questmanager.h"
#include "questlua.h"
#include "locale_service.h"
#include "shutdown_manager.h"
#include "../../common/Controls.h"

#if defined(ENABLE_EVENT_MANAGER) || defined(ENABLE_ITEMSHOP)
#include "desc_client.h"
#include "desc_manager.h"
#endif
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
#include "bossdamagerankingmanager.hpp"
#endif

#if (!defined(__GNUC__) || defined(__clang__)) && !defined(CXX11_ENABLED)
#	include <boost/bind.hpp>

#elif defined(CXX11_ENABLED)
#	include <functional>
template <typename T>
decltype(std::bind(&T::second, std::placeholders::_1)) select2nd()
{
	return std::bind(&T::second, std::placeholders::_1);
}
#endif

CHARACTER_MANAGER::CHARACTER_MANAGER() :
	m_iVIDCount(0),
	m_pkChrSelectedStone(NULL),
	m_bUsePendingDestroy(false)
{
	m_iMobItemRate = 100;
	m_iMobDamageRate = 100;
	m_iMobGoldAmountRate = 100;
	m_iMobGoldDropRate = 100;
	m_iMobExpRate = 100;

	m_iMobItemRatePremium = 100;
	m_iMobGoldAmountRatePremium = 100;
	m_iMobGoldDropRatePremium = 100;
	m_iMobExpRatePremium = 100;

	m_iUserDamageRate = 100;
	m_iUserDamageRatePremium = 100;
#ifdef ENABLE_EVENT_MANAGER
	m_eventData.clear();
#endif
}

CHARACTER_MANAGER::~CHARACTER_MANAGER()
{
	Destroy();
}

void CHARACTER_MANAGER::Destroy()
{
#ifdef ENABLE_EVENT_MANAGER
	m_eventData.clear();
#endif
#ifdef ENABLE_ITEMSHOP
	m_IShopManager.clear();
#endif
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.begin();
	while (it != m_map_pkChrByVID.end())
	{
		LPCHARACTER ch = it->second;
		M2_DESTROY_CHARACTER(ch); // m_map_pkChrByVID is changed here
		it = m_map_pkChrByVID.begin();
	}
}

void CHARACTER_MANAGER::GracefulShutdown()
{
	NAME_MAP::iterator it = m_map_pkPCChr.begin();

	while (it != m_map_pkPCChr.end())
		(it++)->second->Disconnect("GracefulShutdown");
}

DWORD CHARACTER_MANAGER::AllocVID()
{
	++m_iVIDCount;
	return m_iVIDCount;
}

LPCHARACTER CHARACTER_MANAGER::CreateCharacter(const char* name, DWORD dwPID)
{
	DWORD dwVID = AllocVID();
	LPCHARACTER ch = M2_NEW CHARACTER;

	ch->Create(name, dwVID, dwPID ? true : false);

	m_map_pkChrByVID.insert(std::make_pair(dwVID, ch));

	if (dwPID)
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];
		str_lower(name, szName, sizeof(szName));

		m_map_pkPCChr.insert(NAME_MAP::value_type(szName, ch));
		m_map_pkChrByPID.insert(std::make_pair(dwPID, ch));
	}

	return (ch);
}

#ifndef DEBUG_ALLOC
//void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch)
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch, const char* file, size_t line)
#else
void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch, const char* file, size_t line)
#endif
{
	if (!ch)
		return;

	// <Factor> Check whether it has been already deleted or not.
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(ch->GetVID());
	if (it == m_map_pkChrByVID.end()) 
	{
		return; // prevent duplicated destrunction
	}

	if (ch->IsNPC() && !ch->IsPet() && ch->GetRider() == NULL
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		&& !ch->IsMount()
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		&& !ch->IsNewPet()
#endif
#ifdef ENABLE_BUFFI_SYSTEM
		&& !ch->IsBuffi()
#endif
		)
	{
		if (ch->GetDungeon())
		{
			ch->GetDungeon()->DeadCharacter(ch);
		}
	}

	if (m_bUsePendingDestroy)
	{
		m_set_pkChrPendingDestroy.insert(ch);
		return;
	}

	m_map_pkChrByVID.erase(it);

	if (true == ch->IsPC())
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];

		str_lower(ch->GetName(), szName, sizeof(szName));

		NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

		if (m_map_pkPCChr.end() != it)
			m_map_pkPCChr.erase(it);
	}

	if (0 != ch->GetPlayerID() && ch->IsPC())
	{
		itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(ch->GetPlayerID());

		if (m_map_pkChrByPID.end() != it)
		{
			m_map_pkChrByPID.erase(it);
		}
	}

	UnregisterRaceNumMap(ch);

	RemoveFromStateList(ch);
#ifndef DEBUG_ALLOC
	M2_DELETE(ch);
#else
	M2_DELETE_EX(ch, file, line);
#endif
}

LPCHARACTER CHARACTER_MANAGER::Find(DWORD dwVID)
{
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(dwVID);

	if (m_map_pkChrByVID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwVID != (DWORD)found->GetVID()) {
		sys_err("[CHARACTER_MANAGER::Find] <Factor> %u != %u", dwVID, (DWORD)found->GetVID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::Find(const VID& vid)
{
	LPCHARACTER tch = Find((DWORD)vid);

	if (!tch || tch->GetVID() != vid)
		return NULL;

	return tch;
}

LPCHARACTER CHARACTER_MANAGER::FindByPID(DWORD dwPID)
{
	itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(dwPID);

	if (m_map_pkChrByPID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwPID != found->GetPlayerID())
	{
		sys_err("[CHARACTER_MANAGER::FindByPID] <Factor> %u != %u", dwPID, found->GetPlayerID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::FindPC(const char* name)
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	str_lower(name, szName, sizeof(szName));
	NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

	if (it == m_map_pkPCChr.end())
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && strncasecmp(szName, found->GetName(), CHARACTER_NAME_MAX_LEN) != 0) {
		sys_err("[CHARACTER_MANAGER::FindPC] <Factor> %s != %s", name, found->GetName());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex)
{
	const CMob* pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
	{
		sys_err("no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!map_allow_find(lMapIndex))
	{
		sys_err("not allowed map %u", lMapIndex);
		return NULL;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pkSectreeMap == NULL) {
		return NULL;
	}

	int i;
	long x, y;
	for (i = 0; i < 2000; i++)
	{
		x = number(1, (pkSectreeMap->m_setting.iWidth / 100) - 1) * 100 + pkSectreeMap->m_setting.iBaseX;
		y = number(1, (pkSectreeMap->m_setting.iHeight / 100) - 1) * 100 + pkSectreeMap->m_setting.iBaseY;
		LPSECTREE tree = pkSectreeMap->Find(x, y);

		if (!tree)
			continue;

		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT))
			continue;

		if (IS_SET(dwAttr, ATTR_BANPK))
			continue;

		break;
	}

	if (i == 2000)
	{
		sys_err("cannot find valid location");
		return NULL;
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);

	if (!ch)
	{
		return NULL;
	}

	ch->SetProto(pkMob);

	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(number(0, 360));

	if (!ch->Show(lMapIndex, x, y, 0, false))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_err(0, "SpawnMobRandomPosition: cannot show monster");
		return NULL;
	}
	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion, int iRot, bool bShow)
{
#ifdef ENABLE_BOSS_SPAWN_NOTICE
	static const std::vector<int> bossList = {
		191, 192, 193, 194, 491, 492, 493, 494,
		531, 532, 533, 534, 591, 691, 791, 792,
		1091, 1092, 1093, 1095, 1191, 1192, 1304,
		1901, 2091, 2206, 2307, 2306, 2491, 2492,
		2494, 2495, 2597, 2598, 3190, 3191, 3290,
		3291, 3390, 3391, 3490, 3491, 3590, 3591,
		3690, 3691, 3790, 3791, 3890, 3891, 6390,
		6391, 3090, 3091, 2191, 6421, 4204, 4209,
		4210, 3596
	};
#endif
	
	const CMob* pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
	{
		sys_err("SpawnMob: no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!(pkMob->m_table.bType == CHAR_TYPE_NPC || pkMob->m_table.bType == CHAR_TYPE_WARP || pkMob->m_table.bType == CHAR_TYPE_GOTO) || mining::IsVeinOfOre(dwVnum))
	{
		LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

		if (!tree)
		{
			return NULL;
		}

		DWORD dwAttr = tree->GetAttribute(x, y);

		bool is_set = false;

		if (mining::IsVeinOfOre(dwVnum)) is_set = IS_SET(dwAttr, ATTR_BLOCK);
		else is_set = IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT);

		if (is_set)
		{
			return NULL;
		}

		if (IS_SET(dwAttr, ATTR_BANPK))
		{
			return NULL;
		}
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.szLocaleName);

	if (!ch)
	{
		return NULL;
	}

	if (iRot == -1)
		iRot = number(0, 360);

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(iRot);

	if (bShow && !ch->Show(lMapIndex, x, y, z, bSpawnMotion))
	{
		M2_DESTROY_CHARACTER(ch);
		return NULL;
	}
	
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
	if (const auto is_boss_in_ranking{
			bossdamageranking::boss_dmg_ranking_manager().is_boss_in_ranking(ch->GetRaceNum())};
		is_boss_in_ranking)
	{
		sys_err("add boss to list");
		bossdamageranking::BossDamageRankingBossInfo boss_info{};
		boss_info.mob_vnum = dwVnum;
		boss_info.mob_vid = ch->GetVID();
		boss_info.max_hp = ch->GetMaxHP();

		bossdamageranking::boss_dmg_ranking_manager().add_boss_to_list(boss_info);
	}
#endif

#ifdef ENABLE_BOSS_SPAWN_NOTICE
	if (std::find(bossList.begin(), bossList.end(), ch->GetRaceNum()) != bossList.end())
	{
		char szSpawnNotice[QUERY_MAX_LEN];
		snprintf(szSpawnNotice, sizeof(szSpawnNotice),
				 "[CH %d] : %s has spawned!",
				 g_bChannel,
				 ch->GetName());
		BroadcastNotice(szSpawnNotice);
	}
#endif

	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException, bool bSpawnMotion, bool bAggressive)
{
	const CMob* pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
		return NULL;

	if (pkMob->m_table.bType == CHAR_TYPE_STONE)
		bSpawnMotion = true;

	int i = 16;

	while (i--)
	{
		int x = number(sx, ex);
		int y = number(sy, ey);
		LPCHARACTER ch = SpawnMob(dwVnum, lMapIndex, x, y, 0, bSpawnMotion);

		if (ch)
		{
			if (bAggressive)
				ch->SetAggressive();
			return (ch);
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::SelectStone(LPCHARACTER pkChr)
{
	m_pkChrSelectedStone = pkChr;
}

bool CHARACTER_MANAGER::SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen, bool bAggressive_)
{
	CMobGroup* pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return false;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD>& c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;
		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)
				return false;

			continue;
		}

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}
		if (bAggressive)
			tch->SetAggressive();

		if (tch->Goto(tx, ty))
			tch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

bool CHARACTER_MANAGER::SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	const DWORD dwGroupID = CMobManager::Instance().GetGroupFromGroupGroup(dwVnum);

	if (dwGroupID != 0)
	{
		return SpawnGroup(dwGroupID, lMapIndex, sx, sy, ex, ey, pkRegen, bAggressive_, pDungeon);
	}
	else
	{
		sys_err("NOT_EXIST_GROUP_GROUP_VNUM(%u) MAP(%ld)", dwVnum, lMapIndex);
		return false;
	}
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	CMobGroup* pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return NULL;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD>& c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;

		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	LPCHARACTER chLeader = NULL;

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)
				return NULL;

			continue;
		}

		if (i == 0)
			chLeader = tch;

		tch->SetDungeon(pDungeon);

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}

		if (bAggressive)
			tch->SetAggressive();
	}

	return chLeader;
}

struct FuncUpdateAndResetChatCounter
{
	void operator () (LPCHARACTER ch)
	{
		ch->ResetChatCounter();
		ch->CFSM::Update();
	}
};

void CHARACTER_MANAGER::Update(int iPulse)
{
	using namespace std;
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
#endif

	BeginPendingDestroy();

	{
		if (!m_map_pkPCChr.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_map_pkPCChr.size());
#if (defined(__GNUC__) && !defined(__clang__)) || defined(CXX11_ENABLED)
			transform(m_map_pkPCChr.begin(), m_map_pkPCChr.end(), back_inserter(v), select2nd<NAME_MAP::value_type>());
#else
			transform(m_map_pkPCChr.begin(), m_map_pkPCChr.end(), back_inserter(v), boost::bind(&NAME_MAP::value_type::second, _1));
#endif

			if (0 == (iPulse % PASSES_PER_SEC(5)))
			{
				FuncUpdateAndResetChatCounter f;
				for_each(v.begin(), v.end(), f);
			}
			else
			{
				//for_each(v.begin(), v.end(), mem_fun(&CFSM::Update));
				for_each(v.begin(), v.end(), bind2nd(mem_fun(&CHARACTER::UpdateCharacter), iPulse));
			}
		}

		//		for_each_pc(bind2nd(mem_fun(&CHARACTER::UpdateCharacter), iPulse));
	}

	{
		if (!m_set_pkChrState.empty())
		{
			CHARACTER_VECTOR v;
			v.reserve(m_set_pkChrState.size());
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
			transform(m_set_pkChrState.begin(), m_set_pkChrState.end(), back_inserter(v), identity<CHARACTER_SET::value_type>());
#else
			v.insert(v.end(), m_set_pkChrState.begin(), m_set_pkChrState.end());
#endif
			for_each(v.begin(), v.end(), bind2nd(mem_fun(&CHARACTER::UpdateStateMachine), iPulse));
		}
	}

	FlushPendingDestroy();

	// ShutdownManager Update
	CShutdownManager::Instance().Update();
}

void CHARACTER_MANAGER::ProcessDelayedSave()
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.begin();

	while (it != m_set_pkChrForDelayedSave.end())
	{
		LPCHARACTER pkChr = *it++;
		pkChr->SaveReal();
	}

	m_set_pkChrForDelayedSave.clear();
}

bool CHARACTER_MANAGER::AddToStateList(LPCHARACTER ch)
{
	assert(ch != NULL);

	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it == m_set_pkChrState.end())
	{
		m_set_pkChrState.insert(ch);
		return true;
	}

	return false;
}

void CHARACTER_MANAGER::RemoveFromStateList(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it != m_set_pkChrState.end())
	{
		m_set_pkChrState.erase(it);
	}
}

void CHARACTER_MANAGER::DelayedSave(LPCHARACTER ch)
{
	m_set_pkChrForDelayedSave.insert(ch);
}

bool CHARACTER_MANAGER::FlushDelayedSave(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.find(ch);

	if (it == m_set_pkChrForDelayedSave.end())
		return false;

	m_set_pkChrForDelayedSave.erase(it);
	ch->SaveReal();
	return true;
}

void CHARACTER_MANAGER::RegisterRaceNum(DWORD dwVnum)
{
	m_set_dwRegisteredRaceNum.insert(dwVnum);
}

void CHARACTER_MANAGER::RegisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	if (m_set_dwRegisteredRaceNum.find(dwVnum) != m_set_dwRegisteredRaceNum.end())
	{
		m_map_pkChrByRaceNum[dwVnum].insert(ch);
	}
}

void CHARACTER_MANAGER::UnregisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	itertype(m_map_pkChrByRaceNum) it = m_map_pkChrByRaceNum.find(dwVnum);

	if (it != m_map_pkChrByRaceNum.end())
		it->second.erase(ch);
}

bool CHARACTER_MANAGER::GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor& i)
{
	std::map<DWORD, CHARACTER_SET>::iterator it = m_map_pkChrByRaceNum.find(dwRaceNum);

	if (it == m_map_pkChrByRaceNum.end())
		return false;

	i = it->second;
	return true;
}

#define FIND_JOB_WARRIOR_0	(1 << 3)
#define FIND_JOB_WARRIOR_1	(1 << 4)
#define FIND_JOB_WARRIOR_2	(1 << 5)
#define FIND_JOB_WARRIOR	(FIND_JOB_WARRIOR_0 | FIND_JOB_WARRIOR_1 | FIND_JOB_WARRIOR_2)
#define FIND_JOB_ASSASSIN_0	(1 << 6)
#define FIND_JOB_ASSASSIN_1	(1 << 7)
#define FIND_JOB_ASSASSIN_2	(1 << 8)
#define FIND_JOB_ASSASSIN	(FIND_JOB_ASSASSIN_0 | FIND_JOB_ASSASSIN_1 | FIND_JOB_ASSASSIN_2)
#define FIND_JOB_SURA_0		(1 << 9)
#define FIND_JOB_SURA_1		(1 << 10)
#define FIND_JOB_SURA_2		(1 << 11)
#define FIND_JOB_SURA		(FIND_JOB_SURA_0 | FIND_JOB_SURA_1 | FIND_JOB_SURA_2)
#define FIND_JOB_SHAMAN_0	(1 << 12)
#define FIND_JOB_SHAMAN_1	(1 << 13)
#define FIND_JOB_SHAMAN_2	(1 << 14)
#define FIND_JOB_SHAMAN		(FIND_JOB_SHAMAN_0 | FIND_JOB_SHAMAN_1 | FIND_JOB_SHAMAN_2)
#ifdef ENABLE_WOLFMAN_CHARACTER
#define FIND_JOB_WOLFMAN_0	(1 << 15)
#define FIND_JOB_WOLFMAN_1	(1 << 16)
#define FIND_JOB_WOLFMAN_2	(1 << 17)
#define FIND_JOB_WOLFMAN		(FIND_JOB_WOLFMAN_0 | FIND_JOB_WOLFMAN_1 | FIND_JOB_WOLFMAN_2)
#endif

//
// (job+1)*3+(skill_group)
//
LPCHARACTER CHARACTER_MANAGER::FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except, int iMinLevel, int iMaxLevel)
{
	LPCHARACTER chFind = NULL;
	itertype(m_map_pkChrByPID) it;
	int n = 0;

	for (it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
	{
		LPCHARACTER ch = it->second;

		if (ch == except)
			continue;

		if (ch->GetLevel() < iMinLevel)
			continue;

		if (ch->GetLevel() > iMaxLevel)
			continue;

		if (ch->GetMapIndex() != lMapIndex)
			continue;

		if (uiJobFlag)
		{
			unsigned int uiChrJob = (1 << ((ch->GetJob() + 1) * 3 + ch->GetSkillGroup()));

			if (!IS_SET(uiJobFlag, uiChrJob))
				continue;
		}

		if (!chFind || number(1, ++n) == 1)
			chFind = ch;
	}

	return chFind;
}

int CHARACTER_MANAGER::GetMobItemRate(LPCHARACTER ch)
{
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
		return m_iMobItemRatePremium;
	return m_iMobItemRate;
}

int CHARACTER_MANAGER::GetMobDamageRate(LPCHARACTER ch)
{
	return m_iMobDamageRate;
}

int CHARACTER_MANAGER::GetMobGoldAmountRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobGoldAmountRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldAmountRatePremium;
	return m_iMobGoldAmountRate;
}

int CHARACTER_MANAGER::GetMobGoldDropRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobGoldDropRate;
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldDropRatePremium;
	return m_iMobGoldDropRate;
}

int CHARACTER_MANAGER::GetMobExpRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobExpRate;
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iMobExpRatePremium;
	return m_iMobExpRate;
}

int	CHARACTER_MANAGER::GetUserDamageRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iUserDamageRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iUserDamageRatePremium;

	return m_iUserDamageRate;
}

void CHARACTER_MANAGER::SendScriptToMap(long lMapIndex, const std::string& s)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);

	if (NULL == pSecMap)
		return;

	struct packet_script p;

	p.header = HEADER_GC_SCRIPT;
	p.skin = 1;
	p.src_size = s.size();

	quest::FSendPacket f;
	p.size = p.src_size + sizeof(struct packet_script);
	f.buf.write(&p, sizeof(struct packet_script));
	f.buf.write(&s[0], s.size());

	pSecMap->for_each(f);
}

bool CHARACTER_MANAGER::BeginPendingDestroy()
{
	if (m_bUsePendingDestroy)
		return false;

	m_bUsePendingDestroy = true;
	return true;
}

void CHARACTER_MANAGER::FlushPendingDestroy()
{
	using namespace std;

	m_bUsePendingDestroy = false;

	if (!m_set_pkChrPendingDestroy.empty())
	{
		CHARACTER_SET::iterator it = m_set_pkChrPendingDestroy.begin(),
			end = m_set_pkChrPendingDestroy.end();
		for (; it != end; ++it) {
			M2_DESTROY_CHARACTER(*it);
		}

		m_set_pkChrPendingDestroy.clear();
	}
}

CharacterVectorInteractor::CharacterVectorInteractor(const CHARACTER_SET& r)
{
	using namespace std;
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	using namespace __gnu_cxx;
#endif

	reserve(r.size());
#if defined(__GNUC__) && !defined(__clang__) && !defined(CXX11_ENABLED)
	transform(r.begin(), r.end(), back_inserter(*this), identity<CHARACTER_SET::value_type>());
#else
	insert(end(), r.begin(), r.end());
#endif

	if (CHARACTER_MANAGER::instance().BeginPendingDestroy())
		m_bMyBegin = true;
}

CharacterVectorInteractor::~CharacterVectorInteractor()
{
	if (m_bMyBegin)
		CHARACTER_MANAGER::instance().FlushPendingDestroy();
}

#ifdef ENABLE_EVENT_MANAGER
#include "item_manager.h"
#include "item.h"
void CHARACTER_MANAGER::ClearEventData()
{
	m_eventData.clear();
}

const TEventManagerData* CHARACTER_MANAGER::CheckEventIsActive(BYTE eventIndex, BYTE empireIndex)
{
	for(auto it = m_eventData.begin();it!=m_eventData.end();++it)
	{
		const std::vector<TEventManagerData>& dayVector = it->second;
	
		for (DWORD j=0;j<dayVector.size();++j)
		{
			const TEventManagerData& eventData = dayVector[j];
	
			if (eventData.eventIndex == eventIndex)
			{
				if (eventData.channelFlag != 0)
				{
					if (eventData.channelFlag != g_bChannel)
						continue;
				}
	
				if (eventData.empireFlag != 0 && empireIndex != 0)
				{
					if (eventData.empireFlag != empireIndex)
						continue;
				}

				if(eventData.eventStatus == true)
					return &eventData;
			}
		}
	}
	return NULL;
}

void CHARACTER_MANAGER::CheckEventForDrop(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM>& vec_item)
{
	const BYTE killerEmpire = pkKiller->GetEmpire();
	const TEventManagerData* eventPtr = NULL;
	LPITEM rewardItem = NULL;

	if (pkChr->IsStone())
	{
		eventPtr = CheckEventIsActive(DOUBLE_METIN_LOOT_EVENT, killerEmpire);

		if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
		{
			const int prob = number(1, 100);
			const int success_prob = eventPtr->value[3];
	
			if (success_prob >= prob)
			{
				std::vector<LPITEM> m_cache;
				for (DWORD j=0;j<vec_item.size();++j)
				{
					const auto vItem = vec_item[j];
					rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);
					if (rewardItem) m_cache.emplace_back(rewardItem);
				}
	
				for (DWORD j=0;j<m_cache.size();++j)
					vec_item.emplace_back(m_cache[j]);
			}
		}
	}
	else if (pkChr->GetMobRank() >= MOB_RANK_BOSS)
	{
		eventPtr = CheckEventIsActive(DOUBLE_BOSS_LOOT_EVENT, killerEmpire);
		if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
		{
			const int prob = number(1, 100);
			const int success_prob = eventPtr->value[3];

			if (success_prob >= prob)
			{
				std::vector<LPITEM> m_cache;
				for (DWORD j=0;j<vec_item.size();++j)
				{
					const auto vItem = vec_item[j];
					rewardItem = ITEM_MANAGER::Instance().CreateItem(vItem->GetVnum(), vItem->GetCount(), 0, true);

					if (rewardItem)
						m_cache.emplace_back(rewardItem);
				}
				for (DWORD j=0;j<m_cache.size();++j)
					vec_item.emplace_back(m_cache[j]);
			}
		}
	}

	eventPtr = CheckEventIsActive(WHELL_OF_FORTUNE_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(200300, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(LETTER_EVENT, killerEmpire);

	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			constexpr DWORD letterVnums[7] = { 200301, 200302, 200303, 200304 , 200305 , 200306 , 200307 };

			const long long randomIndex{ number(0, 7) };

			LPITEM item = ITEM_MANAGER::Instance().CreateItem(letterVnums[randomIndex], 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(OKEY_CARD_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(79505, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(CATCH_KING_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(79603, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}
	
	eventPtr = CheckEventIsActive(BLACK_N_WHITE_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(79512, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}	
	eventPtr = CheckEventIsActive(YUTNORI_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(79507, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(MOONLIGHT_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(50011, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}

	eventPtr = CheckEventIsActive(SOCCER_BALL_EVENT, killerEmpire);
	if (eventPtr && LEVEL_DELTA(pkChr->GetLevel(), pkKiller->GetLevel(), 20))
	{
		const int prob = number(1, 1000);
		const int success_prob = eventPtr->value[3];
		if (success_prob >= prob)
		{
			LPITEM item = ITEM_MANAGER::Instance().CreateItem(50096, 1, 0, true);
			if (item) vec_item.emplace_back(item);
		}
	}
}

void CHARACTER_MANAGER::CompareEventSendData(TEMP_BUFFER* buf)
{
	const BYTE dayCount = m_eventData.size();
	const BYTE subIndex = EVENT_MANAGER_LOAD;
	const int cur_Time = time(NULL);
	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;
	p.size = sizeof(TPacketGCEventManager) + sizeof(BYTE)+sizeof(BYTE)+sizeof(int);

	for(auto it = m_eventData.begin();it != m_eventData.end();++it)
	{
		const auto& dayData = it->second;
		const BYTE dayEventCount = dayData.size();
		p.size += sizeof(BYTE) + sizeof(BYTE) + (dayEventCount * sizeof(TEventManagerData));
	}

	buf->write(&p, sizeof(TPacketGCEventManager));
	buf->write(&subIndex, sizeof(BYTE));
	buf->write(&dayCount, sizeof(BYTE));
	buf->write(&cur_Time, sizeof(int));

	for(auto it = m_eventData.begin();it != m_eventData.end();++it)
	{
		const auto& dayIndex = it->first;
		const auto& dayData = it->second;
		const BYTE dayEventCount = dayData.size();
		buf->write(&dayIndex, sizeof(BYTE));
		buf->write(&dayEventCount, sizeof(BYTE));
	
		if (dayEventCount > 0)
			buf->write(dayData.data(), dayEventCount * sizeof(TEventManagerData));
	}
}

void CHARACTER_MANAGER::UpdateAllPlayerEventData()
{
	TEMP_BUFFER buf;
	CompareEventSendData(&buf);
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

	for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		LPCHARACTER ch = (*it)->GetCharacter();
		if (!ch)
			continue;

		(*it)->Packet(buf.read_peek(), buf.size());
	}
}

void CHARACTER_MANAGER::SendDataPlayer(LPCHARACTER ch)
{
	auto desc = ch->GetDesc();
	if (!desc)
		return;

	TEMP_BUFFER buf;
	CompareEventSendData(&buf);
	desc->Packet(buf.read_peek(), buf.size());
}

bool CHARACTER_MANAGER::CloseEventManuel(BYTE eventIndex)
{
	auto eventPtr = CheckEventIsActive(eventIndex);

	if (eventPtr != NULL)
	{
		const BYTE subHeader = EVENT_MANAGER_REMOVE_EVENT;
		db_clientdesc->DBPacketHeader(HEADER_GD_EVENT_MANAGER, 0, sizeof(BYTE)+sizeof(WORD));
		db_clientdesc->Packet(&subHeader, sizeof(BYTE));
		db_clientdesc->Packet(&eventPtr->eventID, sizeof(WORD));
		return true;
	}
	return false;
}

void CHARACTER_MANAGER::SetEventStatus(const WORD eventID, const bool eventStatus, const int endTime, const char* endTimeText)
{
	TEventManagerData* eventData = NULL;
	for (auto it = m_eventData.begin(); it != m_eventData.end(); ++it)
	{
		if (it->second.size())
		{
			for (DWORD j = 0; j < it->second.size(); ++j)
			{
				TEventManagerData& pData = it->second[j];

				if (pData.eventID == eventID)
				{
					eventData = &pData;
					break;
				}
			}
		}
	}
	if (eventData == NULL)
		return;

	eventData->eventStatus = eventStatus;
	eventData->endTime = endTime;
	strlcpy(eventData->endTimeText, endTimeText, sizeof(eventData->endTimeText));

	EventItemTimeUpdate();

	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

	const int now = time(0);
	const BYTE subIndex = EVENT_MANAGER_EVENT_STATUS;
	
	TPacketGCEventManager p;
	p.header = HEADER_GC_EVENT_MANAGER;
	p.size = sizeof(TPacketGCEventManager)+sizeof(BYTE)+sizeof(WORD)+sizeof(bool)+sizeof(int)+sizeof(int)+sizeof(eventData->endTimeText);
	
	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketGCEventManager));
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&eventData->eventID, sizeof(WORD));
	buf.write(&eventData->eventStatus, sizeof(bool));
	buf.write(&eventData->endTime, sizeof(int));
	buf.write(&eventData->endTimeText, sizeof(eventData->endTimeText));
	buf.write(&now, sizeof(int));

	for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		LPCHARACTER ch = (*it)->GetCharacter();
		if (!ch)
			continue;

		(*it)->Packet(buf.read_peek(), buf.size());
	}

}
void CHARACTER_MANAGER::SetEventData(BYTE dayIndex, const std::vector<TEventManagerData>& m_data)
{
	const auto it = m_eventData.find(dayIndex);

	if (it == m_eventData.end())
	{
		m_eventData.emplace(dayIndex, m_data);
	}
	else
	{
		it->second.clear();
		for (DWORD j=0;j<m_data.size();++j)
			it->second.emplace_back(m_data[j]);
	}
	EventItemTimeUpdate();
}
#endif

#ifdef ENABLE_ITEMSHOP
void CHARACTER_MANAGER::LoadItemShopLogReal(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	BYTE subIndex = ITEMSHOP_LOG;

	const int logCount = *(int*)c_pData;
	c_pData += sizeof(int);
	std::vector<TIShopLogData> m_vec;
	if (logCount)
	{
		for (DWORD j = 0; j < logCount; ++j)
		{
			const TIShopLogData logData = *(TIShopLogData*)c_pData;
			m_vec.emplace_back(logData);
			c_pData += sizeof(TIShopLogData);
		}
	}

	TPacketGCItemShop p;
	p.header = HEADER_GC_ITEMSHOP;
	p.size = sizeof(TPacketGCItemShop) + sizeof(BYTE) + sizeof(int) + (sizeof(TIShopLogData) * logCount);

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketGCItemShop));
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&logCount, sizeof(int));

	if(logCount)
		buf.write(m_vec.data(), sizeof(TIShopLogData)* logCount);
	
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CHARACTER_MANAGER::LoadItemShopLog(LPCHARACTER ch)
{
	BYTE subIndex = ITEMSHOP_LOG;
	DWORD accountID = ch->GetDesc()->GetAccountTable().id;

	db_clientdesc->DBPacketHeader(HEADER_GD_ITEMSHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(DWORD));
	db_clientdesc->Packet(&subIndex, sizeof(BYTE));
	db_clientdesc->Packet(&accountID, sizeof(DWORD));
}

void CHARACTER_MANAGER::LoadItemShopData(LPCHARACTER ch, bool isAll)
{
	TEMP_BUFFER buf;
	TPacketGCItemShop p;
	p.header = HEADER_GC_ITEMSHOP;
	
	long long dragonCoin = ch->GetDragonCoin();

	if (isAll)
	{
		int calculateSize = 0;
		BYTE subIndex = ITEMSHOP_LOAD;
		calculateSize += sizeof(BYTE);

		calculateSize += sizeof(long long);//dragon coin
		calculateSize += sizeof(int);//updatetime

		int categoryTotalSize = m_IShopManager.size();
		calculateSize += sizeof(int);

		if (m_IShopManager.size())
		{
			for (auto it = m_IShopManager.begin(); it != m_IShopManager.end(); ++it)
			{
				calculateSize += sizeof(BYTE);
				calculateSize += sizeof(BYTE);

				if (it->second.size())
				{
					for (auto itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
					{
						calculateSize += sizeof(BYTE);
						BYTE categorySubSize = itEx->second.size();
						calculateSize += sizeof(BYTE);
						if (categorySubSize)
							calculateSize += sizeof(TIShopData) * categorySubSize;
					}
				}
			}
		}

		p.size = sizeof(TPacketGCItemShop) + calculateSize;

		buf.write(&p, sizeof(TPacketGCItemShop));
		buf.write(&subIndex, sizeof(BYTE));
		buf.write(&dragonCoin, sizeof(long long));
		buf.write(&itemshopUpdateTime, sizeof(int));
		buf.write(&categoryTotalSize, sizeof(int));

		if (m_IShopManager.size())
		{
			for (auto it = m_IShopManager.begin(); it != m_IShopManager.end(); ++it)
			{
				BYTE categoryIndex = it->first;
				buf.write(&categoryIndex, sizeof(BYTE));
				BYTE categorySize = it->second.size();
				buf.write(&categorySize, sizeof(BYTE));
				if (it->second.size())
				{
					for (auto itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
					{
						BYTE categorySubIndex = itEx->first;
						buf.write(&categorySubIndex, sizeof(BYTE));
						BYTE categorySubSize = itEx->second.size();
						buf.write(&categorySubSize, sizeof(BYTE));
						if (categorySubSize)
							buf.write(itEx->second.data(), sizeof(TIShopData) * categorySubSize);
					}
				}
			}
		}
	}
	else
	{
		p.size = sizeof(TPacketGCItemShop) + sizeof(BYTE)+sizeof(int)+sizeof(int);
		buf.write(&p, sizeof(TPacketGCItemShop));
		BYTE subIndex = ITEMSHOP_LOAD;
		buf.write(&subIndex, sizeof(BYTE));
		buf.write(&dragonCoin, sizeof(long long));
		buf.write(&itemshopUpdateTime, sizeof(int));
		int categoryTotalSize = 9999;
		buf.write(&categoryTotalSize, sizeof(int));
	}
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CHARACTER_MANAGER::LoadItemShopBuyReal(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	const BYTE returnType = *(BYTE*)c_pData;
	c_pData += sizeof(BYTE);

	if (returnType == 0)
	{
		ch->NewChatPacket(INGAME_SHOP_MSG_01);
		return;
	}
	else if (returnType == 1)
	{
		const int weekMaxCount = *(int*)c_pData;
		c_pData += sizeof(int);
		ch->NewChatPacket(INGAME_SHOP_MSG_02, "%d", weekMaxCount);
		return;
	}
	else if (returnType == 2)
	{
		const int monthMaxCount = *(int*)c_pData;
		c_pData += sizeof(int);
		ch->NewChatPacket(INGAME_SHOP_MSG_03, "%d", monthMaxCount);
		return;
	}
	else if (returnType == 4)
	{
		ch->NewChatPacket(INGAME_SHOP_MSG_04);
		return;
	}

	const bool isOpenLog = *(bool*)c_pData;
	c_pData += sizeof(bool);

	const DWORD itemVnum = *(DWORD*)c_pData;
	c_pData += sizeof(DWORD);

	const int itemCount = *(int*)c_pData;
	c_pData += sizeof(int);

	//const long long itemPrice = *(long long*)c_pData;
	c_pData += sizeof(long long);

	const bool buyUntradable = *(bool*)c_pData;
	c_pData += sizeof(bool);

	if (buyUntradable)
	{
		const DWORD untradableItemVnum = *(DWORD*)c_pData;
		c_pData += sizeof(DWORD);
		ch->AutoGiveItem(untradableItemVnum, itemCount);
	}	
	else
	{
#ifdef ENABLE_ITEMSHOP_TO_INVENTORY
		ch->AutoGiveItem(itemVnum, itemCount);
#endif
	}


	TEMP_BUFFER buf;
	TPacketGCItemShop p;
	p.header = HEADER_GC_ITEMSHOP;
	p.size = sizeof(TPacketGCItemShop) + sizeof(BYTE) + sizeof(long long) + sizeof(bool);

	if(isOpenLog)
		p.size += sizeof(TIShopLogData);

	BYTE subIndex = ITEMSHOP_DRAGONCOIN;
	long long dragonCoin = ch->GetDragonCoin();

	buf.write(&p, sizeof(TPacketGCItemShop));
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&dragonCoin, sizeof(long long));
	buf.write(&isOpenLog, sizeof(bool));

	if (isOpenLog)
	{
		const TIShopLogData logData = *(TIShopLogData*)c_pData;
		c_pData += sizeof(TIShopLogData);

		buf.write(&logData, sizeof(TIShopLogData));
	}
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CHARACTER_MANAGER::LoadItemShopBuy(LPCHARACTER ch, int itemID, int itemCount, const bool buyUntradable)
{
	if (itemCount <= 0 || itemCount > 20)
		return;

	if (m_IShopManager.size())
	{
		for (auto it = m_IShopManager.begin(); it != m_IShopManager.end(); ++it)
		{
			if (it->second.size())
			{
				for (auto itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
				{
					if (itEx->second.size())
					{
						for (auto itReal = itEx->second.begin(); itReal != itEx->second.end(); ++itReal)
						{
							const TIShopData& itemData = *itReal;
							if (itemData.id == itemID)
							{
								DWORD dragonCoin = ch->GetDragonCoin();

								long long itemPrice = itemData.itemPrice * itemCount;
	
								if (buyUntradable)
									itemPrice = itemData.itemUntradeablePrice * itemCount;

								if (itemData.discount > 0)
									itemPrice = long((float(itemData.itemPrice) / 100.0) * float(100 - itemData.discount));

								if (itemPrice > dragonCoin)
								{
									ch->NewChatPacket(INGAME_SHOP_MSG_01);
									return;
								}

								DWORD accountID = ch->GetDesc()->GetAccountTable().id;
								BYTE subIndex = ITEMSHOP_BUY;
								char playerName[CHARACTER_NAME_MAX_LEN + 1];
								strlcpy(playerName, ch->GetName(), sizeof(playerName));

								char ipAdress[16];
								strlcpy(ipAdress, ch->GetDesc()->GetHostName(), sizeof(ipAdress));

								TEMP_BUFFER buf;
								buf.write(&subIndex, sizeof(BYTE));
								buf.write(&accountID, sizeof(DWORD));
								buf.write(&playerName, sizeof(playerName));
								buf.write(&ipAdress, sizeof(ipAdress));
								buf.write(&itemID, sizeof(int));
								buf.write(&itemCount, sizeof(int));
								buf.write(&buyUntradable, sizeof(bool));
								bool isLogOpen = (ch->GetProtectTime("itemshop.log") == 1)?true:false;
								buf.write(&isLogOpen, sizeof(bool));

								db_clientdesc->DBPacketHeader(HEADER_GD_ITEMSHOP, ch->GetDesc()->GetHandle(), buf.size());
								db_clientdesc->Packet(buf.read_peek(), buf.size());

								return;
							}
						}
					}
				}
			}
		}
	}
}
void CHARACTER_MANAGER::UpdateItemShopItem(const char* c_pData)
{
	const TIShopData& updateItem = *(TIShopData*)c_pData;
	c_pData += sizeof(TIShopData);

	bool sendPacketProcess = false;

	if (m_IShopManager.size())
	{
		for (auto it = m_IShopManager.begin(); it != m_IShopManager.end(); ++it)
		{
			if (sendPacketProcess)
				break;
			if (it->second.size())
			{
				for (auto itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
				{
					if (sendPacketProcess)
						break;
					if (itEx->second.size())
					{
						for (auto itReal = itEx->second.begin(); itReal != itEx->second.end(); ++itReal)
						{
							TIShopData& itemData = *itReal;
							if (itemData.id == updateItem.id)
							{
								itemData.maxSellCount = updateItem.maxSellCount;
								sendPacketProcess = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	if (sendPacketProcess)
	{
		TEMP_BUFFER buf;
		TPacketGCItemShop p;
		p.header = HEADER_GC_ITEMSHOP;
		p.size = sizeof(TPacketGCItemShop) + sizeof(BYTE) + sizeof(TIShopData);
		BYTE subIndex = ITEMSHOP_UPDATE_ITEM;
		buf.write(&p, sizeof(TPacketGCItemShop));
		buf.write(&subIndex, sizeof(BYTE));
		buf.write(&updateItem, sizeof(TIShopData));

		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
		if (c_ref_set.size())
		{
			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				auto desc = *it;
				if (desc)
				{
					LPCHARACTER ch = desc->GetCharacter();
					if (ch)
					{
						if (ch->GetProtectTime("itemshop.load") == 1)
						{
							desc->Packet(buf.read_peek(), buf.size());
						}
					}
				}

			}
		}
	}
}

void RefreshItemShop(LPDESC d)
{
	LPCHARACTER ch = d->GetCharacter();
	if (!ch)
		return;
	if (ch->GetProtectTime("itemshop.load") == 1)
	{
		ch->NewChatPacket(INGAME_SHOP_MSG_07);
		CHARACTER_MANAGER::Instance().LoadItemShopData(ch, true);
	}
}
void CHARACTER_MANAGER::LoadItemShopData(const char* c_pData)
{
	m_IShopManager.clear();

	const int updateTime = *(int*)c_pData;
	c_pData += sizeof(int);

	const bool isManuelUpdate = *(bool*)c_pData;
	c_pData += sizeof(bool);

	const int categoryTotalSize = *(int*)c_pData;
	c_pData += sizeof(int);

	itemshopUpdateTime = updateTime;

	for (DWORD j = 0; j < categoryTotalSize; ++j)
	{
		const BYTE categoryIndex = *(BYTE*)c_pData;
		c_pData += sizeof(BYTE);
		const BYTE categorySize = *(BYTE*)c_pData;
		c_pData += sizeof(BYTE);

		std::map<BYTE, std::vector<TIShopData>> m_map;
		m_map.clear();

		for (DWORD x = 0; x < categorySize; ++x)
		{
			const BYTE categorySubIndex = *(BYTE*)c_pData;
			c_pData += sizeof(BYTE);

			const BYTE categorySubSize = *(BYTE*)c_pData;
			c_pData += sizeof(BYTE);

			std::vector<TIShopData> m_vec;
			m_vec.clear();

			for (DWORD b = 0; b < categorySubSize; ++b)
			{
				const TIShopData itemData = *(TIShopData*)c_pData;

				m_vec.emplace_back(itemData);
				c_pData += sizeof(TIShopData);
			}

			if(m_vec.size())
				m_map.emplace(categorySubIndex, m_vec);
		}
		if(m_map.size())
			m_IShopManager.emplace(categoryIndex, m_map);
	}

	if (isManuelUpdate)
	{
		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_ref_set.begin(), c_ref_set.end(), RefreshItemShop);
	}
}
#endif

#ifdef ENABLE_RELOAD_REGEN
void CHARACTER_MANAGER::DestroyCharacterInMap(long lMapIndex)
{
	std::vector<LPCHARACTER> tempVec;
	for (itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.begin(); it != m_map_pkChrByVID.end(); it++)
	{
		LPCHARACTER pkChr = it->second;
		if (pkChr && pkChr->GetMapIndex() == lMapIndex && pkChr->IsNPC() && !pkChr->IsPet() && pkChr->GetRider() == NULL)
		{
			tempVec.push_back(pkChr);
		}
	}
	for (std::vector<LPCHARACTER>::iterator it = tempVec.begin(); it != tempVec.end(); it++)
	{
		M2_DESTROY_CHARACTER(*it);
	}
}
#endif