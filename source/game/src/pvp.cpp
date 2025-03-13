#include "stdafx.h"
#include "constants.h"
#include "pvp.h"
#include "crc32.h"
#include "packet.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "config.h"
#include "sectree_manager.h"
#include "buffer_manager.h"
#include "locale_service.h"

using namespace std;

CPVP::CPVP(DWORD dwPID1, DWORD dwPID2)
{
	if (dwPID1 > dwPID2)
	{
		m_players[0].dwPID = dwPID1;
		m_players[1].dwPID = dwPID2;
		m_players[0].bAgree = true;
	}
	else
	{
		m_players[0].dwPID = dwPID2;
		m_players[1].dwPID = dwPID1;
		m_players[1].bAgree = true;
	}

	DWORD adwID[2];
	adwID[0] = m_players[0].dwPID;
	adwID[1] = m_players[1].dwPID;
	m_dwCRC = GetFastHash((const char*)&adwID, 8);
	m_bRevenge = false;

	SetLastFightTime();
}

CPVP::CPVP(CPVP& k)
{
	m_players[0] = k.m_players[0];
	m_players[1] = k.m_players[1];

	m_dwCRC = k.m_dwCRC;
	m_bRevenge = k.m_bRevenge;
	
#ifdef ENABLE_RENEWAL_PVP
	memset(&pvpSetting, true, sizeof(pvpSetting));
	pvpSetting[PVP_HALF_HUMAN] = false;
	pvpSetting[PVP_BUFFI_SKILLS] = false;
	pvpSetting[PVP_HP_ELIXIR] = false;
	pvpBet = 0;
#endif

	SetLastFightTime();
}

CPVP::~CPVP()
{
}

void CPVP::Packet(bool bDelete)
{
	if (!m_players[0].dwVID || !m_players[1].dwVID)
	{
		if (bDelete)
			sys_err("null vid when removing %u %u", m_players[0].dwVID, m_players[0].dwVID);

		return;
	}

	TPacketGCPVP pack;

	pack.bHeader = HEADER_GC_PVP;

	if (bDelete)
	{
#ifdef ENABLE_RENEWAL_PVP
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(m_players[0].dwPID);
		if (ch != NULL)
			ch->CheckPvPBonus(false, pvpSetting);
		ch = CHARACTER_MANAGER::Instance().FindByPID(m_players[1].dwPID);
		if (ch != NULL)
			ch->CheckPvPBonus(false, pvpSetting);
#endif
		pack.bMode = PVP_MODE_NONE;
		pack.dwVIDSrc = m_players[0].dwVID;
		pack.dwVIDDst = m_players[1].dwVID;
	}
	else if (IsFight())
	{
		pack.bMode = PVP_MODE_FIGHT;
		pack.dwVIDSrc = m_players[0].dwVID;
		pack.dwVIDDst = m_players[1].dwVID;
	}
	else
	{
		pack.bMode = m_bRevenge ? PVP_MODE_REVENGE : PVP_MODE_AGREE;

		if (m_players[0].bAgree)
		{
			pack.dwVIDSrc = m_players[0].dwVID;
			pack.dwVIDDst = m_players[1].dwVID;
		}
		else
		{
			pack.dwVIDSrc = m_players[1].dwVID;
			pack.dwVIDDst = m_players[0].dwVID;
		}
	}

	const DESC_MANAGER::DESC_SET& c_rSet = DESC_MANAGER::instance().GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = c_rSet.begin();

	while (it != c_rSet.end())
	{
		LPDESC d = *it++;

		if (d->IsPhase(PHASE_GAME) || d->IsPhase(PHASE_DEAD))
			d->Packet(&pack, sizeof(pack));
	}
}

bool CPVP::Agree(DWORD dwPID)
{
	m_players[m_players[0].dwPID != dwPID ? 1 : 0].bAgree = true;

	if (IsFight())
	{
		Packet();
		return true;
	}

	return false;
}

bool CPVP::IsFight()
{
	return (m_players[0].bAgree == m_players[1].bAgree) && m_players[0].bAgree;
}

void CPVP::Win(DWORD dwPID)
{
	int iSlot = m_players[0].dwPID != dwPID ? 1 : 0;

	m_bRevenge = true;

	m_players[iSlot].bAgree = true;
	m_players[!iSlot].bCanRevenge = true;
	m_players[!iSlot].bAgree = false;

	Packet();
}

bool CPVP::CanRevenge(DWORD dwPID)
{
	return m_players[m_players[0].dwPID != dwPID ? 1 : 0].bCanRevenge;
}

void CPVP::SetVID(DWORD dwPID, DWORD dwVID)
{
	if (m_players[0].dwPID == dwPID)
		m_players[0].dwVID = dwVID;
	else
		m_players[1].dwVID = dwVID;
}

void CPVP::SetLastFightTime()
{
	m_dwLastFightTime = get_dword_time();
}

DWORD CPVP::GetLastFightTime()
{
	return m_dwLastFightTime;
}

CPVPManager::CPVPManager()
{
}

CPVPManager::~CPVPManager()
{
}

#if defined(ENABLE_RENEWAL_PVP) || defined(ENABLE_NEWSTUFF)
bool CPVPManager::IsFighting(LPCHARACTER pkChr)
{
	if (!pkChr)
		return false;
	return IsFighting(pkChr->GetPlayerID());
}
bool CPVPManager::IsFighting(DWORD dwPID)
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(dwPID);

	if (it == m_map_pkPVPSetByID.end())
		return false;
	std::unordered_set<CPVP*>::iterator it2 = it->second.begin();
	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;
		if (pkPVP->IsFight())
			return true;
	}
	return false;
}
bool CPVPManager::HasPvP(LPCHARACTER pkChr, LPCHARACTER pkVictim)
{
	auto it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());
	if (it == m_map_pkPVPSetByID.end())
		return false;
	auto it2 = it->second.begin();
	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;
		DWORD dwCompanionPID;
		if (pkPVP->m_players[0].dwPID == pkChr->GetPlayerID())
			dwCompanionPID = pkPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pkPVP->m_players[0].dwPID;
		if (dwCompanionPID == pkVictim->GetPlayerID())
			return true;
	}
	return false;
}
void CPVPManager::RemoveCharactersPvP(LPCHARACTER pkChr, LPCHARACTER pkVictim)
{
	auto it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return;

	auto it2 = it->second.begin();
	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;
		DWORD dwCompanionPID;
		if (pkPVP->m_players[0].dwPID == pkChr->GetPlayerID())
			dwCompanionPID = pkPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pkPVP->m_players[0].dwPID;
		if (dwCompanionPID == pkVictim->GetPlayerID())
		{
			pkPVP->Packet(true);
			Delete(pkPVP);
		}
	}
}

EVENTINFO(duel_effect_info)
{
	DWORD players1, players2, crcPvP;
	BYTE state;
	duel_effect_info(): players1(0), players2(0), crcPvP(0), state(0){}
};

EVENTFUNC(start_duel_efect)
{
	duel_effect_info* info = dynamic_cast<duel_effect_info*>(event->info);
	if (!info)
		return 0;

	CPVP* pkPVP = CPVPManager::Instance().Find(info->crcPvP);
	if (!pkPVP)
		return 0;

	LPCHARACTER pkChr = CHARACTER_MANAGER::Instance().FindByPID(info->players1), pkVictim = CHARACTER_MANAGER::Instance().FindByPID(info->players2);
	if (!pkChr || !pkVictim)
		return 0;

	switch (info->state)
	{
		case 0:
			pkChr->SpecificEffectPacket("d:/ymir work/effect/pvp/3.mse");
			pkVictim->SpecificEffectPacket("d:/ymir work/effect/pvp/3.mse");
			break;
		case 1:
			pkChr->SpecificEffectPacket("d:/ymir work/effect/pvp/2.mse");
			pkVictim->SpecificEffectPacket("d:/ymir work/effect/pvp/2.mse");
			break;
		case 2:
			pkChr->SpecificEffectPacket("d:/ymir work/effect/pvp/1.mse");
			pkVictim->SpecificEffectPacket("d:/ymir work/effect/pvp/1.mse");
			break;
		case 3:
		{
			pkChr->SpecificEffectPacket("d:/ymir work/effect/pvp/go.mse");
			pkVictim->SpecificEffectPacket("d:/ymir work/effect/pvp/go.mse");
			
			pkChr->CheckPvPBonus(true, pkPVP->pvpSetting);
			pkVictim->CheckPvPBonus(true, pkPVP->pvpSetting);
			if (pkPVP->Agree(pkChr->GetPlayerID()))
			{
				pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The fight with %s begins!"), pkChr->GetName());
				pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The fight with %s begins!"), pkVictim->GetName());
			}
			return 0;
		}
		break;
	}
	info->state++;
	return PASSES_PER_SEC(1);
}
void CPVPManager::Insert(LPCHARACTER pkChr, LPCHARACTER pkVictim, bool* pvpSetting, long long pvpBet)
#else
void CPVPManager::Insert(LPCHARACTER pkChr, LPCHARACTER pkVictim)
#endif
{
	if (pkChr->IsDead() || pkVictim->IsDead())
		return;

	CPVP kPVP(pkChr->GetPlayerID(), pkVictim->GetPlayerID());

	CPVP* pkPVP;

	if ((pkPVP = Find(kPVP.m_dwCRC)))
	{
#ifdef ENABLE_RENEWAL_PVP
		if(pkPVP->pvpBet > 0)
		{
			if (pkPVP->pvpBet > pkChr->GetGold())
			{
				pkChr->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("1000"));
				return;
			}
			else if (pkPVP->pvpBet > pkVictim->GetGold())
			{
				pkChr->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("1003"));
				return;
			}

			pkVictim->PointChange(POINT_GOLD, -pkPVP->pvpBet);
			pkChr->PointChange(POINT_GOLD, -pkPVP->pvpBet);
		}
		duel_effect_info* info = AllocEventInfo<duel_effect_info>();
		info->players1 = pkChr->GetPlayerID();
		info->players2 = pkVictim->GetPlayerID();
		info->crcPvP = kPVP.m_dwCRC;
		event_create(start_duel_efect, info, PASSES_PER_SEC(1));
#else
		if (pkPVP->Agree(pkChr->GetPlayerID()))
		{
			pkVictim->NewChatPacket(STRING_D100, "%s", pkChr->GetName());
			pkChr->NewChatPacket(STRING_D100, "%s", pkVictim->GetName());
		}
#endif
		return;
	}

	pkPVP = M2_NEW CPVP(kPVP);

#ifdef ENABLE_RENEWAL_PVP
	thecore_memcpy(&pkPVP->pvpSetting, pvpSetting, sizeof(pkPVP->pvpSetting));
	pkPVP->pvpBet = pvpBet;
#endif

	pkPVP->SetVID(pkChr->GetPlayerID(), pkChr->GetVID());
	pkPVP->SetVID(pkVictim->GetPlayerID(), pkVictim->GetVID());

	m_map_pkPVP.insert(map<DWORD, CPVP*>::value_type(pkPVP->m_dwCRC, pkPVP));

	m_map_pkPVPSetByID[pkChr->GetPlayerID()].insert(pkPVP);
	m_map_pkPVPSetByID[pkVictim->GetPlayerID()].insert(pkPVP);

#ifdef ENABLE_RENEWAL_PVP
	char msg[CHAT_MAX_LEN + 1];
	int iLen = snprintf(msg, sizeof(msg), "OpenPvPWindow %s %u ", pkChr->GetName(), static_cast<unsigned int>(pkChr->GetVID()));
	for (DWORD j = 0; j < PVP_BET; ++j)
		iLen += snprintf(msg + iLen, sizeof(msg) - iLen, "#%d", pvpSetting[j]);
	iLen += snprintf(msg + iLen, sizeof(msg) - iLen, "#%lld", pvpBet);
	pkVictim->ChatPacket(CHAT_TYPE_COMMAND, msg);

	pkPVP->Packet();
#else
	pkPVP->Packet();
	char msg[CHAT_MAX_LEN + 1];
	snprintf(msg, sizeof(msg), LC_TEXT("%s wants to fight with you."), pkChr->GetName());

	pkVictim->ChatPacket(CHAT_TYPE_INFO, msg);
	pkChr->NewChatPacket(STRING_D99, "%s", pkVictim->GetName());

	// NOTIFY_PVP_MESSAGE
	LPDESC pkVictimDesc = pkVictim->GetDesc();
	if (pkVictimDesc)
	{
		TPacketGCWhisper pack;

		int32_t len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

		pack.bHeader = HEADER_GC_WHISPER;
		pack.wSize = sizeof(TPacketGCWhisper) + len;
		pack.bType = WHISPER_TYPE_SYSTEM;
		strlcpy(pack.szNameFrom, pkChr->GetName(), sizeof(pack.szNameFrom));

		TEMP_BUFFER buf;

		buf.write(&pack, sizeof(TPacketGCWhisper));
		buf.write(msg, len);

		pkVictimDesc->Packet(buf.read_peek(), buf.size());
	}
#endif
	// END_OF_NOTIFY_PVP_MESSAGE
}

//#ifdef ENABLE_NEWSTUFF
//bool CPVPManager::IsFighting(LPCHARACTER pkChr)
//{
//	if (!pkChr)
//		return false;
//
//	return IsFighting(pkChr->GetPlayerID());
//}
//
//bool CPVPManager::IsFighting(DWORD dwPID)
//{
//	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(dwPID);
//
//	if (it == m_map_pkPVPSetByID.end())
//		return false;
//
//	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();
//
//	while (it2 != it->second.end())
//	{
//		CPVP* pkPVP = *it2++;
//		if (pkPVP->IsFight())
//			return true;
//	}
//
//	return false;
//}
//#endif

void CPVPManager::ConnectEx(LPCHARACTER pkChr, bool bDisconnect)
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return;

	DWORD dwVID = bDisconnect ? 0 : pkChr->GetVID();

	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;
		pkPVP->SetVID(pkChr->GetPlayerID(), dwVID);
	}
}

void CPVPManager::Connect(LPCHARACTER pkChr)
{
	ConnectEx(pkChr, false);
}

void CPVPManager::Disconnect(LPCHARACTER pkChr)
{
#ifdef ENABLE_RENEWAL_PVP
	if (!pkChr)
		return;
	auto it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());
	if (it == m_map_pkPVPSetByID.end())
		return;
	auto it2 = it->second.begin();
	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;
		pkPVP->Packet(true);
		Delete(pkPVP);
	}
#endif
}

void CPVPManager::GiveUp(LPCHARACTER pkChr, DWORD dwKillerPID) // This method is calling from no where yet.
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return;

	sys_log(1, "PVPManager::Dead %d", pkChr->GetPlayerID());
	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;

		DWORD dwCompanionPID;

		if (pkPVP->m_players[0].dwPID == pkChr->GetPlayerID())
			dwCompanionPID = pkPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pkPVP->m_players[0].dwPID;

		if (dwCompanionPID != dwKillerPID)
			continue;

		pkPVP->SetVID(pkChr->GetPlayerID(), 0);

		m_map_pkPVPSetByID.erase(dwCompanionPID);

		it->second.erase(pkPVP);

		if (it->second.empty())
			m_map_pkPVPSetByID.erase(it);

		m_map_pkPVP.erase(pkPVP->m_dwCRC);

		pkPVP->Packet(true);
		M2_DELETE(pkPVP);
		break;
	}
}

#ifdef ENABLE_RENEWAL_PVP
bool CPVPManager::Dead(LPCHARACTER pkChr, LPCHARACTER pkKiller)
#else
bool CPVPManager::Dead(LPCHARACTER pkChr, DWORD dwKillerPID)
#endif
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return false;

	bool found = false;

	sys_log(1, "PVPManager::Dead %d", pkChr->GetPlayerID());
	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP* pkPVP = *it2++;

		DWORD dwCompanionPID;

		if (pkPVP->m_players[0].dwPID == pkChr->GetPlayerID())
			dwCompanionPID = pkPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pkPVP->m_players[0].dwPID;

#ifdef ENABLE_RENEWAL_PVP
		if (dwCompanionPID == pkKiller->GetPlayerID())
#else
		if (dwCompanionPID == dwKillerPID)
#endif
		{
			if (pkPVP->IsFight())
			{
				pkPVP->SetLastFightTime();
#ifdef ENABLE_RENEWAL_PVP
				if (pkPVP->pvpBet > 0)
				{
					pkKiller->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT("1004"), pkPVP->pvpBet);
					pkKiller->PointChange(POINT_GOLD, pkPVP->pvpBet * 2);
				}

				pkKiller->CheckPvPBonus(false, pkPVP->pvpSetting);
				pkKiller->SpecificEffectPacket("d:/ymir work/effect/pvp/win.mse");
				pkChr->CheckPvPBonus(false, pkPVP->pvpSetting);

				pkPVP->Win(pkKiller->GetPlayerID());
				pkPVP->Packet(true);
				Delete(pkPVP);
#else
				pkPVP->Win(dwKillerPID);
#endif
				found = true;
				break;
			}
			else if (get_dword_time() - pkPVP->GetLastFightTime() <= 15000)
			{
				found = true;
				break;
			}
		}
	}

	return found;
}

bool CPVPManager::CanAttack(LPCHARACTER pkChr, LPCHARACTER pkVictim)
{
	switch (pkVictim->GetCharType())
	{
	case CHAR_TYPE_NPC:
	case CHAR_TYPE_WARP:
	case CHAR_TYPE_GOTO:
		return false;
	}

	if (pkChr == pkVictim)
		return false;

	if (pkVictim->IsNPC() && pkChr->IsNPC() && !pkChr->IsGuardNPC())
		return false;
	
#ifdef ENABLE_AFK_MODE_SYSTEM
	if (pkVictim->IsPC() && pkVictim->IsAway())
		return false;
#endif

	if (true == pkChr->IsHorseRiding())
	{
		if (pkChr->GetHorseLevel() > 0 && 1 == pkChr->GetHorseGrade())
			return false;
	}
	else
	{
		eMountType eIsMount = GetMountLevelByVnum(pkChr->GetMountVnum(), false);
		switch (eIsMount)
		{
		case MOUNT_TYPE_NONE:
		case MOUNT_TYPE_COMBAT:
		case MOUNT_TYPE_MILITARY:
			break;
		case MOUNT_TYPE_NORMAL:
		default:
			return false;
			break;
		}
	}

	if (pkVictim->IsNPC() || pkChr->IsNPC())
	{
		return true;
	}

	if (pkVictim->IsObserverMode() || pkChr->IsObserverMode())
		return false;

	{
		BYTE bMapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(pkChr->GetMapIndex());

		if (((pkChr->GetPKMode() == PK_MODE_PROTECT) && (pkChr->GetEmpire() == bMapEmpire)) ||
			((pkVictim->GetPKMode() == PK_MODE_PROTECT) && (pkVictim->GetEmpire() == bMapEmpire)))
		{
			return false;
		}
	}

	if (pkChr->GetEmpire() != pkVictim->GetEmpire())
	{
		// @warme005
		{
			if (pkChr->GetPKMode() == PK_MODE_PROTECT || pkVictim->GetPKMode() == PK_MODE_PROTECT)
			{
				return false;
			}
		}

		return true;
	}

	bool beKillerMode = false;

	if (pkVictim->GetParty() && pkVictim->GetParty() == pkChr->GetParty())
	{
		return false;
		// Cannot attack same party on any pvp model
	}
	else
	{
		if (pkVictim->IsKillerMode())
		{
			return true;
		}

		if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() >= 0)
		{
			if (g_protectNormalPlayer)
			{
				if (PK_MODE_PEACE == pkVictim->GetPKMode())
					return false;
			}
		}

		switch (pkChr->GetPKMode())
		{
		case PK_MODE_PEACE:
		case PK_MODE_REVENGE:
			// Cannot attack same guild
			if (pkVictim->GetGuild() && pkVictim->GetGuild() == pkChr->GetGuild())
				break;

			if (pkChr->GetPKMode() == PK_MODE_REVENGE)
			{
				if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() >= 0)
				{
					pkChr->SetKillerMode(true);
					return true;
				}
				else if (pkChr->GetAlignment() >= 0 && pkVictim->GetAlignment() < 0)
					return true;
			}
			break;

		case PK_MODE_GUILD:
			// Same implementation from PK_MODE_FREE except for attacking same guild
			if (!pkChr->GetGuild() || (pkVictim->GetGuild() != pkChr->GetGuild()))
			{
				if (pkVictim->GetAlignment() >= 0)
					pkChr->SetKillerMode(true);
				else if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() < 0)
					pkChr->SetKillerMode(true);

				return true;
			}
			break;

		case PK_MODE_FREE:
			if (pkVictim->GetAlignment() >= 0)
				pkChr->SetKillerMode(true);
			else if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() < 0)
				pkChr->SetKillerMode(true);
			return true;
			break;
		}
	}

	CPVP kPVP(pkChr->GetPlayerID(), pkVictim->GetPlayerID());
	CPVP* pkPVP = Find(kPVP.m_dwCRC);

	if (!pkPVP || !pkPVP->IsFight())
	{
		if (beKillerMode)
			pkChr->SetKillerMode(true);

		return (beKillerMode);
	}

	pkPVP->SetLastFightTime();
	return true;
}

CPVP* CPVPManager::Find(DWORD dwCRC)
{
	map<DWORD, CPVP*>::iterator it = m_map_pkPVP.find(dwCRC);

	if (it == m_map_pkPVP.end())
		return NULL;

	return it->second;
}

void CPVPManager::Delete(CPVP* pkPVP)
{
	map<DWORD, CPVP*>::iterator it = m_map_pkPVP.find(pkPVP->m_dwCRC);

	if (it == m_map_pkPVP.end())
		return;

	m_map_pkPVP.erase(it);
	m_map_pkPVPSetByID[pkPVP->m_players[0].dwPID].erase(pkPVP);
	m_map_pkPVPSetByID[pkPVP->m_players[1].dwPID].erase(pkPVP);

	M2_DELETE(pkPVP);
}

void CPVPManager::SendList(LPDESC d)
{
	map<DWORD, CPVP*>::iterator it = m_map_pkPVP.begin();

	DWORD dwVID = d->GetCharacter()->GetVID();

	TPacketGCPVP pack;

	pack.bHeader = HEADER_GC_PVP;

	while (it != m_map_pkPVP.end())
	{
		CPVP* pkPVP = (it++)->second;

		if (!pkPVP->m_players[0].dwVID || !pkPVP->m_players[1].dwVID)
			continue;

		if (pkPVP->IsFight())
		{
			pack.bMode = PVP_MODE_FIGHT;
			pack.dwVIDSrc = pkPVP->m_players[0].dwVID;
			pack.dwVIDDst = pkPVP->m_players[1].dwVID;
		}
		else
		{
			pack.bMode = pkPVP->m_bRevenge ? PVP_MODE_REVENGE : PVP_MODE_AGREE;

			if (pkPVP->m_players[0].bAgree)
			{
				pack.dwVIDSrc = pkPVP->m_players[0].dwVID;
				pack.dwVIDDst = pkPVP->m_players[1].dwVID;
			}
			else
			{
				pack.dwVIDSrc = pkPVP->m_players[1].dwVID;
				pack.dwVIDDst = pkPVP->m_players[0].dwVID;
			}
		}

		d->Packet(&pack, sizeof(pack));
		sys_log(1, "PVPManager::SendList %d %d", pack.dwVIDSrc, pack.dwVIDDst);

		if (pkPVP->m_players[0].dwVID == dwVID)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(pkPVP->m_players[1].dwVID);
			if (ch && ch->GetDesc())
			{
				LPDESC d = ch->GetDesc();
				d->Packet(&pack, sizeof(pack));
			}
		}
		else if (pkPVP->m_players[1].dwVID == dwVID)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(pkPVP->m_players[0].dwVID);
			if (ch && ch->GetDesc())
			{
				LPDESC d = ch->GetDesc();
				d->Packet(&pack, sizeof(pack));
			}
		}
	}
}

void CPVPManager::Process()
{
	map<DWORD, CPVP*>::iterator it = m_map_pkPVP.begin();

	while (it != m_map_pkPVP.end())
	{
		CPVP* pvp = (it++)->second;

		if (get_dword_time() - pvp->GetLastFightTime() > 600000)
		{
			pvp->Packet(true);
			Delete(pvp);
		}
	}
}