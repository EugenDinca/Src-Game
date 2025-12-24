#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "battle.h"
#include "pvp.h"
#include "skill.h"
#include "start_position.h"
#include "profiler.h"
#include "cmd.h"
#include "dungeon.h"

#include "unique_item.h"
#include "priv_manager.h"
#include "db.h"
#include "vector.h"
#include "marriage.h"
#include "arena.h"
#include "regen.h"
#include "exchange.h"
#include "shop_manager.h"
#include "dev_log.h"
#include "ani.h"
#include "BattleArena.h"
#include "packet.h"
#include "party.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "questmanager.h"
#include "questlua.h"
#ifdef ENABLE_LMW_PROTECTION
#include "event.h"
#endif
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
#include "bossdamagerankingmanager.hpp"
#endif
#ifdef ENABLE_ALIGNMENT_SYSTEM
#include "alignment_proto.h"
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

#define ENABLE_EFFECT_PENETRATE
static DWORD __GetPartyExpNP(const DWORD level)
{
	if (!level || level > PLAYER_EXP_TABLE_MAX)
		return 14000;
	return party_exp_distribute_table[level];
}

static int __GetExpLossPerc(const DWORD level)
{
	if (!level || level > PLAYER_EXP_TABLE_MAX)
		return 1;
	return aiExpLossPercents[level];
}

DWORD AdjustExpByLevel(const LPCHARACTER ch, const DWORD exp)
{
	if (PLAYER_MAX_LEVEL_CONST < ch->GetLevel())
	{
		double ret = 0.95;
		double factor = 0.1;

		for (ssize_t i = 0; i < ch->GetLevel() - 100; ++i)
		{
			if ((i % 10) == 0)
				factor /= 2.0;

			ret *= 1.0 - factor;
		}

		ret = ret * static_cast<double>(exp);

		if (ret < 1.0)
			return 1;

		return static_cast<DWORD>(ret);
	}

	return exp;
}

bool CHARACTER::CanBeginFight() const
{
	if (!CanMove())
		return false;

	return m_pointsInstant.position == POS_STANDING && !IsDead() && !IsStun();
}

void CHARACTER::BeginFight(LPCHARACTER pkVictim)
{
	SetVictim(pkVictim);
	SetPosition(POS_FIGHTING);
	SetNextStatePulse(1);
}

bool CHARACTER::CanFight() const
{
	return m_pointsInstant.position >= POS_FIGHTING ? true : false;
}

void CHARACTER::CreateFly(BYTE bType, LPCHARACTER pkVictim)
{
	TPacketGCCreateFly packFly;

	packFly.bHeader = HEADER_GC_CREATE_FLY;
	packFly.bType = bType;
	packFly.dwStartVID = GetVID();
	packFly.dwEndVID = pkVictim->GetVID();

	PacketAround(&packFly, sizeof(TPacketGCCreateFly));
}

void CHARACTER::DistributeSP(LPCHARACTER pkKiller, int iMethod)
{
	if (pkKiller->GetSP() >= pkKiller->GetMaxSP())
		return;

	bool bAttacking = (get_dword_time() - GetLastAttackTime()) < 3000;
	bool bMoving = (get_dword_time() - GetLastMoveTime()) < 3000;

	if (iMethod == 1)
	{
		int num = number(0, 3);

		if (!num)
		{
			int iLvDelta = GetLevel() - pkKiller->GetLevel();
			int iAmount = 0;

			if (iLvDelta >= 5)
				iAmount = 10;
			else if (iLvDelta >= 0)
				iAmount = 6;
			else if (iLvDelta >= -3)
				iAmount = 2;

			if (iAmount != 0)
			{
				iAmount += (iAmount * pkKiller->GetPoint(POINT_SP_REGEN)) / 100;

				if (iAmount >= 11)
					CreateFly(FLY_SP_BIG, pkKiller);
				else if (iAmount >= 7)
					CreateFly(FLY_SP_MEDIUM, pkKiller);
				else
					CreateFly(FLY_SP_SMALL, pkKiller);

				pkKiller->PointChange(POINT_SP, iAmount);
			}
		}
	}
	else
	{
		if (pkKiller->GetJob() == JOB_SHAMAN || (pkKiller->GetJob() == JOB_SURA && pkKiller->GetSkillGroup() == 2))
		{
			int iAmount;

			if (bAttacking)
				iAmount = 2 + GetMaxSP() / 100;
			else if (bMoving)
				iAmount = 3 + GetMaxSP() * 2 / 100;
			else
				iAmount = 10 + GetMaxSP() * 3 / 100;

			iAmount += (iAmount * pkKiller->GetPoint(POINT_SP_REGEN)) / 100;
			pkKiller->PointChange(POINT_SP, iAmount);
		}
		else
		{
			int iAmount;

			if (bAttacking)
				iAmount = 2 + pkKiller->GetMaxSP() / 200;
			else if (bMoving)
				iAmount = 2 + pkKiller->GetMaxSP() / 100;
			else
			{
				if (pkKiller->GetHP() < pkKiller->GetMaxHP())
					iAmount = 2 + (pkKiller->GetMaxSP() / 100);
				else
					iAmount = 9 + (pkKiller->GetMaxSP() / 100);
			}

			iAmount += (iAmount * pkKiller->GetPoint(POINT_SP_REGEN)) / 100;
			pkKiller->PointChange(POINT_SP, iAmount);
		}
	}
}

bool CHARACTER::Attack(LPCHARACTER pkVictim, BYTE bType)
{
	if (!CanMove())
	{
		return false;
	}
	
#ifdef ENABLE_AFK_MODE_SYSTEM
	if (pkVictim && pkVictim->IsPC() && pkVictim->IsAway())
	{
		if(IsPC())
		return false;
		
		if (!CanMove())
		return false;
	
	}
#endif

	if (pkVictim->GetMyShop())//L05fix88
	{
		return false;
	}

	// @fixme131
	if (!battle_is_attackable(this, pkVictim))
	{
		return false;
	}

#ifdef ENABLE_OBSERVER_DAMAGE_FIX
	if (pkVictim->IsPC() && pkVictim->IsObserverMode()) 
	{
		return false;
	}
#endif

#ifdef ENABLE_AFTERDEATH_SHIELD
	if(IsAffectFlag(AFF_AFTERDEATH_SHIELD))
		RemoveShieldAffect();
#endif

	DWORD dwCurrentTime = get_dword_time();

	if (IsPC())
	{
#ifdef ENABLE_LMW_PROTECTION
		if (pkVictim->IsStone() || pkVictim->GetMobRank() >= MOB_RANK_BOSS)
		{
			SetAttackCount(pkVictim->GetVID());
		}
#endif
	}
	
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
    if (IsPC())
    {
        if (const auto is_boss_in_ranking{
        bossdamageranking::boss_dmg_ranking_manager().is_boss_in_ranking(pkVictim->GetRaceNum())};
    is_boss_in_ranking)
        {
            bossdamageranking::boss_dmg_ranking_manager().add_player_to_list(
                this, {pkVictim->GetRaceNum(), static_cast<DWORD>(pkVictim->GetVID())});
        }
    }

#endif

	pkVictim->SetSyncOwner(this);

	if (pkVictim->CanBeginFight())
		pkVictim->BeginFight(this);

	DAM_LL iRet;

	if (bType == 0)
	{
		switch (GetMobBattleType())
		{
		case BATTLE_TYPE_MELEE:
		case BATTLE_TYPE_POWER:
		case BATTLE_TYPE_TANKER:
		case BATTLE_TYPE_SUPER_POWER:
		case BATTLE_TYPE_SUPER_TANKER:
			iRet = battle_melee_attack(this, pkVictim);
			break;

		case BATTLE_TYPE_RANGE:
			FlyTarget(pkVictim->GetVID(), pkVictim->GetX(), pkVictim->GetY(), HEADER_CG_FLY_TARGETING);
			iRet = Shoot(0) ? BATTLE_DAMAGE : BATTLE_NONE;
			break;

		case BATTLE_TYPE_MAGIC:
			FlyTarget(pkVictim->GetVID(), pkVictim->GetX(), pkVictim->GetY(), HEADER_CG_FLY_TARGETING);
			iRet = Shoot(1) ? BATTLE_DAMAGE : BATTLE_NONE;
			break;

		default:
			sys_err("Unhandled battle type %d", GetMobBattleType());
			iRet = BATTLE_NONE;
			break;
		}
	}
	else
	{
		if (IsPC() == true)
		{
			if (dwCurrentTime - m_dwLastSkillTime > 1500)
			{
				return false;
			}
		}
		iRet = ComputeSkill(bType, pkVictim);
	}

	if (iRet == BATTLE_DAMAGE || iRet == BATTLE_DEAD)
	{
		OnMove(true);
		pkVictim->OnMove();
		if (BATTLE_DEAD == iRet && IsPC())
			SetVictim(NULL);

		return true;
	}

	return false;
}

void CHARACTER::DeathPenalty(BYTE bTown)
{
	Cube_close(this);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CloseAcce();
#endif
#ifdef ENABLE_AURA_SYSTEM
	CloseAura();
#endif

	if (CBattleArena::instance().IsBattleArenaMap(GetMapIndex()))
		return;

	if (GetLevel() < 10)
	{
		NewChatPacket(WARNING_DRAGON_GOD);
		return;
	}

	if (number(0, 2))
	{
		NewChatPacket(WARNING_DRAGON_GOD);
		return;
	}

	if (IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY))
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);

		// NO_DEATH_PENALTY_BUG_FIX
		if (!bTown)
		{
			if (FindAffect(AFFECT_NO_DEATH_PENALTY))
			{
				NewChatPacket(WARNING_DRAGON_GOD);
				RemoveAffect(AFFECT_NO_DEATH_PENALTY);
				return;
			}
		}
		// END_OF_NO_DEATH_PENALTY_BUG_FIX

		int iLoss = ((GetNextExp() * __GetExpLossPerc(GetLevel())) / 100);

		iLoss = MIN(800000, iLoss);

		if (bTown)
			iLoss = 0;

		if (IsEquipUniqueItem(UNIQUE_ITEM_TEARDROP_OF_GODNESS))
			iLoss /= 2;

		PointChange(POINT_EXP, -iLoss, true);
	}
}

bool CHARACTER::IsStun() const
{
	if (IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN))
		return true;

	return false;
}

EVENTFUNC(StunEvent)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("StunEvent> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkStunEvent = NULL;
	ch->Dead();
	return 0;
}

void CHARACTER::Stun()
{
	if (IsStun())
		return;

	if (IsDead())
		return;

	if (!IsPC() && m_pkParty)
	{
		m_pkParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);
	}

	PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
	PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));

	CloseMyShop();

	event_cancel(&m_pkRecoveryEvent);

	TPacketGCStun pack;
	pack.header = HEADER_GC_STUN;
	pack.vid = m_vid;
	PacketAround(&pack, sizeof(pack));

	SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

	if (m_pkStunEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkStunEvent = event_create(StunEvent, info, PASSES_PER_SEC(3));
}

EVENTINFO(SCharDeadEventInfo)
{
	bool isPC;
	uint32_t dwID;

	SCharDeadEventInfo()
		: isPC(0)
		, dwID(0)
	{
	}
};

EVENTFUNC(dead_event)
{
	const SCharDeadEventInfo* info = dynamic_cast<SCharDeadEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("dead_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = NULL;

	if (true == info->isPC)
	{
		ch = CHARACTER_MANAGER::instance().FindByPID(info->dwID);
	}
	else
	{
		ch = CHARACTER_MANAGER::instance().Find(info->dwID);
	}

	if (NULL == ch)
	{
		sys_err("DEAD_EVENT: cannot find char pointer with %s id(%d)", info->isPC ? "PC" : "MOB", info->dwID);
		return 0;
	}

	ch->m_pkDeadEvent = NULL;

	if (ch->GetDesc())
	{
		ch->GetDesc()->SetPhase(PHASE_GAME);

		ch->SetPosition(POS_STANDING);

		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
			ch->WarpSet(pos.x, pos.y);
		else
		{
			sys_err("cannot find spawn position (name %s)", ch->GetName());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}

		ch->PointChange(POINT_HP, (ch->GetMaxHP() / 2) - ch->GetHP(), true);

		ch->DeathPenalty(0);

		ch->StartRecoveryEvent();

		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
	}
	else
	{
		if (ch->IsMonster() == true)
		{
			if (ch->IsRevive() == false && ch->HasReviverInParty() == true)
			{
				ch->SetPosition(POS_STANDING);
				ch->SetHP(ch->GetMaxHP());

				ch->ViewReencode();

				ch->SetAggressive();
				ch->SetRevive(true);

				return 0;
			}
		}

		M2_DESTROY_CHARACTER(ch);
	}

	return 0;
}

bool CHARACTER::IsDead() const
{
	if (m_pointsInstant.position == POS_DEAD)
		return true;

	return false;
}

void CHARACTER::RewardGold(LPCHARACTER pkAttacker)
{
	int iGoldPercent = MobRankStats[GetMobRank()].iGoldPercent;

	if (pkAttacker->IsPC())
		iGoldPercent = iGoldPercent * (100 + CPrivManager::instance().GetPriv(pkAttacker, PRIV_GOLD_DROP)) / 100;

	if (pkAttacker->GetPoint(POINT_MALL_GOLDBONUS))
		iGoldPercent += (iGoldPercent * pkAttacker->GetPoint(POINT_MALL_GOLDBONUS) / 100);

	iGoldPercent = iGoldPercent * CHARACTER_MANAGER::instance().GetMobGoldDropRate(pkAttacker) / 100;

	// ADD_PREMIUM
	if (pkAttacker->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		iGoldPercent += iGoldPercent;
	// END_OF_ADD_PREMIUM

	if (iGoldPercent > 100)
		iGoldPercent = 100;

	int iPercent;

	if (GetMobRank() >= MOB_RANK_BOSS)
		iPercent = ((iGoldPercent * PERCENT_LVDELTA_BOSS(pkAttacker->GetLevel(), GetLevel())) / 100);
	else
		iPercent = ((iGoldPercent * PERCENT_LVDELTA(pkAttacker->GetLevel(), GetLevel())) / 100);

	if (number(1, 100) > iPercent)
		return;

	int iGoldMultipler = GetGoldMultipler();

	if (1 == number(1, 50000))
		iGoldMultipler *= 10;
	else if (1 == number(1, 10000))
		iGoldMultipler *= 5;

	if (pkAttacker->GetPoint(POINT_GOLD_DOUBLE_BONUS))
		if (number(1, 100) <= pkAttacker->GetPoint(POINT_GOLD_DOUBLE_BONUS))
			iGoldMultipler *= 2;

	int iGold10DropPct = 100;

	iGold10DropPct = (iGold10DropPct * 100) / (100 + CPrivManager::instance().GetPriv(pkAttacker, PRIV_GOLD10_DROP));

	if (GetMobRank() >= MOB_RANK_BOSS && !IsStone() && GetMobTable().dwGoldMax != 0)
	{
		if (1 == number(1, iGold10DropPct))
			iGoldMultipler *= 10;

		int iGold = number(GetMobTable().dwGoldMin, GetMobTable().dwGoldMax);
		iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker) / 100;
		iGold *= iGoldMultipler;

		if (iGold != 0)
		{
			pkAttacker->GiveGold(iGold);
		}
	}

	else if (1 == number(1, iGold10DropPct))
	{
		int iGold = number(GetMobTable().dwGoldMin*10, GetMobTable().dwGoldMax*10);
		iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker) / 100;
		iGold *= iGoldMultipler;

		if (iGold != 0)
		{
			pkAttacker->GiveGold(iGold);
		}
	}
	else
	{
		int iGold = number(GetMobTable().dwGoldMin*10, GetMobTable().dwGoldMax*10);
		iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker) / 100;
		iGold *= iGoldMultipler;

		if (GetMobRank() >= MOB_RANK_BOSS)
		{
			iGold *= 2;
		}

		if (iGold != 0)
		{
			pkAttacker->GiveGold(iGold);
		}
	}
}

void CHARACTER::Reward(bool bItemDrop)
{
	LPCHARACTER pkAttacker = DistributeExp();

#ifdef ENABLE_KILL_EVENT_FIX
	if (!pkAttacker && !(pkAttacker = GetMostAttacked()))
		return;
#else
	if (!pkAttacker)
		return;
#endif

	//PROF_UNIT pu1("r1");
	//pu1.Pop();
	if (pkAttacker->IsPC())
	{
#ifdef ENABLE_ALIGNMENT_SYSTEM
		if (pkAttacker->GetAlignment() < 200000)
		{
#endif
			if ((GetLevel() - pkAttacker->GetLevel()) >= -10)
			{
				if (pkAttacker->GetRealAlignment() < 0)
				{
					if (pkAttacker->IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_KILL))
						pkAttacker->UpdateAlignment(14);
					else
						pkAttacker->UpdateAlignment(7);
				}
				else
					pkAttacker->UpdateAlignment(2);
			}
#ifdef ENABLE_ALIGNMENT_SYSTEM
		}
#endif
		pkAttacker->SetQuestNPCID(GetVID());
		quest::CQuestManager::instance().Kill(pkAttacker->GetPlayerID(), GetRaceNum());
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		pkAttacker->UpdateExtBattlePassMissionProgress(KILL_MONSTER, 1, GetRaceNum());
#endif

#define ANNOUNCEMENT_KILL_BOSS
		const int bossVnumMAP[30] = {
			2492, 2495, 2307, 2306,
			2597, 2598, 1093, 691,
			1304, 1192, 1901, 2091,
			2206, 2191, 2291, 2092,
			792, 4091, 4092, 4095,
			3690, 3691, 3590, 3591,
			3490, 3491, 191, 192,
			193, 194, 491, 492,
			493, 494
		};

		for (int i = 0; i < _countof(bossVnumMAP); i++)
		{
			if (GetRaceNum() == bossVnumMAP[i])
			{
				const CMob * pkMob = CMobManager::instance().Get(bossVnumMAP[i]);

				if (pkMob)
				{
					char szNotice[512+1];
					if (SEX_MALE == GET_SEX(pkAttacker))
						snprintf(szNotice, sizeof(szNotice), "[ANNOUNCEMENT:]: Lv. %d %s Killed: %s!", pkAttacker->GetLevel(), pkAttacker->GetName(), pkMob->m_table.szLocaleName);
					else
						snprintf(szNotice, sizeof(szNotice), "[ANNOUNCEMENT:]: Lv. %d %s Killed: %s!", pkAttacker->GetLevel(), pkAttacker->GetName(), pkMob->m_table.szLocaleName);
					BroadcastNotice(szNotice);
				}
			}
		}
#endif

		if (!number(0, 9))
		{
			if (pkAttacker->GetPoint(POINT_KILL_HP_RECOVERY))
			{
				HP_LL iHP = pkAttacker->GetMaxHP() * pkAttacker->GetPoint(POINT_KILL_HP_RECOVERY) / 100;
				pkAttacker->PointChange(POINT_HP, iHP);
				CreateFly(FLY_HP_SMALL, pkAttacker);
			}

			if (pkAttacker->GetPoint(POINT_KILL_SP_RECOVER))
			{
				int iSP = pkAttacker->GetMaxSP() * pkAttacker->GetPoint(POINT_KILL_SP_RECOVER) / 100;
				pkAttacker->PointChange(POINT_SP, iSP);
				CreateFly(FLY_SP_SMALL, pkAttacker);
			}
		}
#ifdef ENABLE_FARM_BLOCK
		if (pkAttacker->IsFarmBlock() && m_map_kDamage.size() == 1)
			return;
#endif
	}

	if (CBattleArena::instance().IsBattleArenaMap(GetMapIndex()) == true)
		return;

	if (!bItemDrop)
		return;

	PIXEL_POSITION pos = GetXYZ();

	if (!SECTREE_MANAGER::instance().GetMovablePosition(GetMapIndex(), pos.x, pos.y, pos))
		return;

#ifdef ENABLE_FARM_BLOCK
	if (pkAttacker->IsPC() && pkAttacker->IsFarmBlock());
	else
		RewardGold(pkAttacker);
#else
	RewardGold(pkAttacker);
#endif

	LPITEM item;

	static std::vector<LPITEM> s_vec_item;
	s_vec_item.clear();

	if (ITEM_MANAGER::instance().CreateDropItem(this, pkAttacker, s_vec_item))
	{
		if (s_vec_item.size() == 0);
		else if (s_vec_item.size() == 1)
		{
#ifdef ENABLE_FARM_BLOCK
			//Olen moba 2 farkli kisi vurduysa olduren kisi farm block luysa itemi diger vurana atmasini saglar
			if (pkAttacker->IsPC() && pkAttacker->IsFarmBlock())
			{
				bool destroyItem = true;
				for (TDamageMap::iterator it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
				{
					LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(it->first);
					if (ch && ch != pkAttacker && ch->IsPC() && !ch->IsFarmBlock())
					{
						destroyItem = false;
						pkAttacker = ch;
					}
				}
				if (destroyItem)
				{
					item = s_vec_item[0];
					if (item)
					{
						M2_DESTROY_ITEM(item);
					}
					m_map_kDamage.clear();
					return;
				}
			}
#endif
			item = s_vec_item[0];
			if (!item)
			{
				m_map_kDamage.clear();
				return;
			}
#ifdef ENABLE_DROP_ITEM_TO_INVENTORY
#ifdef ENABLE_AUTO_PICK_UP
			bool autoPickup = false;
			if (pkAttacker->IsAutoPickUp())
			{
				autoPickup = pkAttacker->DropItemToInventory(item);
			}
			if (!autoPickup)
			{
				item->AddToGround(GetMapIndex(), pos);
				item->SetOwnership(pkAttacker);
				item->StartDestroyEvent();
			}
#else
			if (!pkAttacker->DropItemToInventory(item))
			{
				item->AddToGround(GetMapIndex(), pos);
				item->SetOwnership(pkAttacker);
				item->StartDestroyEvent();
			}
#endif
#else
			item->AddToGround(GetMapIndex(), pos);
			item->SetOwnership(pkAttacker);
			item->StartDestroyEvent();
#endif
		}
		else
		{
			int iItemIdx = s_vec_item.size() - 1;

			std::priority_queue<std::pair<DAM_LL, LPCHARACTER>> pq; // long long damage item drop fixed

			DAM_LL total_dam = 0;

			for (TDamageMap::iterator it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
			{
				DAM_LL iDamage = it->second.iTotalDamage;
				if (iDamage > 0)
				{
					LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(it->first);
#ifdef ENABLE_FARM_BLOCK
					if (ch && !(ch->IsPC() && ch->IsFarmBlock()))
#else
					if (ch)
#endif
					{
						pq.push(std::make_pair(iDamage, ch));
						total_dam += iDamage;
					}
				}
			}

			std::vector<LPCHARACTER> v;
			while (!pq.empty() && pq.top().first * 10 >= total_dam)
			{
				v.push_back(pq.top().second);
				pq.pop();
			}

			if (v.empty())
			{
				while (iItemIdx >= 0)
				{
					item = s_vec_item[iItemIdx--];
					M2_DESTROY_ITEM(item);
				}
			}
			else
			{
				std::vector<LPCHARACTER>::iterator it = v.begin();

				while (iItemIdx >= 0)
				{
					item = s_vec_item[iItemIdx--];
					if (!item)
					{
						sys_err("item null in vector idx %d", iItemIdx + 1);
						continue;
					}

					LPCHARACTER ch = *it;
					if (ch->GetParty())
						ch = ch->GetParty()->GetNextOwnership(ch, GetX(), GetY());
					++it;

					if (it == v.end())
						it = v.begin();

#ifdef ENABLE_DROP_ITEM_TO_INVENTORY
#ifdef ENABLE_AUTO_PICK_UP
					if (ch->GetParty())
					{
						FPartyDropDiceRoll f(item, ch);
						f.Process(this, pos);
					}
					else
					{
						bool autoPickup = false;
						if (ch->IsAutoPickUp())
						{
							autoPickup = ch->DropItemToInventory(item);
						}
						if (!autoPickup)
						{
							item->AddToGround(ch->GetMapIndex(), pos);
							item->SetOwnership(ch);
							item->StartDestroyEvent();
						}
					}
#else
					if (ch->GetParty())
					{
						FPartyDropDiceRoll f(item, ch);
						f.Process(this, pos);
					}
					else
					{
						if (!ch->DropItemToInventory(item))
						{
							item->AddToGround(ch->GetMapIndex(), pos);
							item->SetOwnership(ch);
							item->StartDestroyEvent();
						}
					}
#endif
#else
					item->AddToGround(GetMapIndex(), pos);
#ifdef ENABLE_DICE_SYSTEM
					if (ch->GetParty())
					{
						FPartyDropDiceRoll f(item, ch);
						f.Process(this);
					}
					else
						item->SetOwnership(ch);
#else
					item->SetOwnership(ch);
#endif
					item->StartDestroyEvent();
#endif
				}
			}
		}
	}

	m_map_kDamage.clear();
}

class FPartyAlignmentCompute
{
public:
	FPartyAlignmentCompute(int iAmount, int x, int y)
	{
		m_iAmount = iAmount;
		m_iCount = 0;
		m_iStep = 0;
		m_iKillerX = x;
		m_iKillerY = y;
	}

	void operator () (LPCHARACTER pkChr)
	{
		if (DISTANCE_APPROX(pkChr->GetX() - m_iKillerX, pkChr->GetY() - m_iKillerY) < PARTY_DEFAULT_RANGE)
		{
			if (m_iStep == 0)
			{
				++m_iCount;
			}
			else
			{
				pkChr->UpdateAlignment(m_iAmount / m_iCount);
			}
		}
	}

	int m_iAmount;
	int m_iCount;
	int m_iStep;

	int m_iKillerX;
	int m_iKillerY;
};

void CHARACTER::Dead(LPCHARACTER pkKiller, bool bImmediateDead)
{
	if (IsDead())
	{
		return;
	}

	if (pkKiller && pkKiller->IsPC() && this != NULL)
	{
		if (!IsPC())
		{
#ifdef ENABLE_PLAYER_STATS_SYSTEM
			if (IsStone())
			{
				pkKiller->PointChange(POINT_STONE_KILLED, 1);
			}
			else if (GetMobRank() == MOB_RANK_BOSS)
			{
				pkKiller->PointChange(POINT_BOSS_KILLED, 1);
			}
			else
			{
				pkKiller->PointChange(POINT_MONSTER_KILLED, 1);
			}
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
			if (pkKiller->GetBiolog())
			{
				pkKiller->GetBiolog()->KillMob(GetRaceNum());
			}
#endif
		}
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		else
		{
			pkKiller->PointChange(POINT_PLAYER_KILLED, 1);
		}
#endif
	}

	if (IsHorseRiding())
	{
		StopRiding();
	}
	else if (GetMountVnum())
	{
		m_dwMountVnum = 0;
		UpdatePacket();
	}

	if (!pkKiller && m_dwKillerPID)
		pkKiller = CHARACTER_MANAGER::instance().FindByPID(m_dwKillerPID);

	m_dwKillerPID = 0;

	bool isAgreedPVP = false;
	bool isUnderGuildWar = false;
	bool isDuel = false;

	if (pkKiller && pkKiller->IsPC())
	{
		if (pkKiller->m_pkChrTarget == this)
			pkKiller->SetTarget(NULL);

		if (!IsPC() && pkKiller->GetDungeon())
			pkKiller->GetDungeon()->IncKillCount(pkKiller, this);

#ifdef ENABLE_RENEWAL_PVP
		isAgreedPVP = CPVPManager::instance().Dead(this, pkKiller);
#else
		isAgreedPVP = CPVPManager::instance().Dead(this, pkKiller->GetPlayerID());
#endif
		isDuel = CArenaManager::instance().OnDead(pkKiller, this);

		if (IsPC())
		{
			CGuild* g1 = GetGuild();
			CGuild* g2 = pkKiller->GetGuild();

			if (g1 && g2)
				if (g1->UnderWar(g2->GetID()))
					isUnderGuildWar = true;

			pkKiller->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().Kill(pkKiller->GetPlayerID(), quest::QUEST_NO_NPC);
			CGuildManager::instance().Kill(pkKiller, this);
			
#if defined(__BL_KILL_BAR__)
			TPacketGCKillBar kb;
			
			kb.bHeader = HEADER_GC_KILLBAR;
			kb.bKillerRace = static_cast<BYTE>(pkKiller->GetRaceNum());
			LPITEM KillerWeapon = pkKiller->GetWear(WEAR_WEAPON);
			kb.bKillerWeaponType = KillerWeapon ? KillerWeapon->GetSubType() : 255;
			kb.bVictimRace = static_cast<BYTE>(GetRaceNum());

			strlcpy(kb.szKiller, pkKiller->GetName(), sizeof(kb.szKiller));
			strlcpy(kb.szVictim, GetName(), sizeof(kb.szVictim));

			const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
			for (DESC_MANAGER::DESC_SET::const_iterator it = c_set_desc.begin(); it != c_set_desc.end(); ++it)
			{
				LPDESC d_c = *it;
				if (!d_c)
					continue;

				LPCHARACTER c_c = d_c->GetCharacter();
				if (!c_c)
					continue;

				if (pkKiller->GetMapIndex() != c_c->GetMapIndex())
					continue;

				d_c->Packet(&kb, sizeof(kb));
			}
#endif
		}
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
		if (const bool check_boss_and_map{!IsPC() && pkKiller->GetMapIndex() < 1000}; check_boss_and_map)
		{
			bossdamageranking::boss_dmg_ranking_manager().erase_boss_from_list({GetRaceNum(), static_cast<DWORD>(GetVID())});
		}
#endif
	}

#ifdef ENABLE_QUEST_DIE_EVENT
	if (IsPC())
	{
		if (pkKiller)
			SetQuestNPCID(pkKiller->GetVID());
		quest::CQuestManager::instance().Die(GetPlayerID(), (pkKiller) ? pkKiller->GetRaceNum() : quest::QUEST_NO_NPC);
	}
#endif


	SetPosition(POS_DEAD);

#ifdef ENABLE_NO_CLEAR_BUFF_WHEN_MONSTER_KILL
	if (pkKiller && IsPC() && !pkKiller->IsPC())
		ClearAffect(true, true);
	else
		ClearAffect(true);
#else
	ClearAffect(true);
#endif

	if (pkKiller && IsPC())
	{
		if (!pkKiller->IsPC())
		{
			SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
		}
		else
		{
#ifdef ENABLE_EXTENDED_BATTLE_PASS
			pkKiller->UpdateExtBattlePassMissionProgress(KILL_PLAYER, 1, GetLevel());
#endif
			if (GetEmpire() != pkKiller->GetEmpire())
			{
				int iEP = MIN(GetPoint(POINT_EMPIRE_POINT), pkKiller->GetPoint(POINT_EMPIRE_POINT));

				PointChange(POINT_EMPIRE_POINT, -(iEP / 10));
				pkKiller->PointChange(POINT_EMPIRE_POINT, iEP / 5);
			}
			else
			{
				if (!isAgreedPVP && !isUnderGuildWar && !IsKillerMode() && GetAlignment() >= 0 && !isDuel && pkKiller->GetPKMode() != PK_MODE_PEACE) /*** Fix bug for "duel, free mod" ***/
				{
					int iNoPenaltyProb = 0;

					if (pkKiller->GetAlignment() >= 0)
						iNoPenaltyProb = 33;
					else
						iNoPenaltyProb = 20;

					if (number(1, 100) < iNoPenaltyProb)
						pkKiller->NewChatPacket(WARNING_DRAGON_GOD2);
					else
					{
						if (pkKiller->GetParty())
						{
							FPartyAlignmentCompute f(-20000, pkKiller->GetX(), pkKiller->GetY());
							pkKiller->GetParty()->ForEachOnlineMember(f);

							if (f.m_iCount == 0)
								pkKiller->UpdateAlignment(-20000);
							else
							{
								f.m_iStep = 1;
								pkKiller->GetParty()->ForEachOnlineMember(f);
							}
						}
						else
							pkKiller->UpdateAlignment(-20000);
					}
				}
			}
		}
	}
	else
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
	}

	ClearSync();

	event_cancel(&m_pkStunEvent);

	if (IsPC())
	{
		m_dwLastDeadTime = get_dword_time();
		SetKillerMode(false);
		GetDesc()->SetPhase(PHASE_DEAD);
	}
	else
	{
		if (!IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_NO_REWARD))
		{
			if (!(pkKiller && pkKiller->IsPC() && pkKiller->GetGuild() && pkKiller->GetGuild()->UnderAnyWar(GUILD_WAR_TYPE_FIELD)))
			{
				if (GetMobTable().dwResurrectionVnum)
				{
					// DUNGEON_MONSTER_REBIRTH_BUG_FIX
					LPCHARACTER chResurrect = CHARACTER_MANAGER::instance().SpawnMob(GetMobTable().dwResurrectionVnum, GetMapIndex(), GetX(), GetY(), GetZ(), true, (int)GetRotation());
					if (GetDungeon() && chResurrect)
					{
						chResurrect->SetDungeon(GetDungeon());
					}
					// END_OF_DUNGEON_MONSTER_REBIRTH_BUG_FIX

					Reward(false);
				}
				else if (IsRevive() == true)
				{
					Reward(false);
				}
				else
				{
					Reward(true); // Drops gold, item, etc..
				}
			}
			else
			{
				if (pkKiller->m_dwUnderGuildWarInfoMessageTime < get_dword_time())
				{
					pkKiller->m_dwUnderGuildWarInfoMessageTime = get_dword_time() + 60000;
					pkKiller->NewChatPacket(GUILDWAR_WARNING_NO_ADVANTAGE_HUNTING);
				}
			}
		}
	}

	TPacketGCDead pack;
	pack.header = HEADER_GC_DEAD;
	pack.vid = m_vid;
	PacketAround(&pack, sizeof(pack));

	REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

	if (GetDesc() != NULL)
	{
		itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

		while (it != m_list_pkAffect.end())
			SendAffectAddPacket(GetDesc(), *it++);
	}

	if (isDuel == false)
	{
		if (m_pkDeadEvent)
		{
			event_cancel(&m_pkDeadEvent);
		}

		if (IsStone())
			ClearStone();

		if (GetDungeon())
		{
			GetDungeon()->DeadCharacter(this);
		}

		SCharDeadEventInfo* pEventInfo = AllocEventInfo<SCharDeadEventInfo>();

		if (IsPC())
		{
			pEventInfo->isPC = true;
			pEventInfo->dwID = this->GetPlayerID();

			m_pkDeadEvent = event_create(dead_event, pEventInfo, PASSES_PER_SEC(180));
		}
		else
		{
			pEventInfo->isPC = false;
			pEventInfo->dwID = this->GetVID();

			if (IsStone())
			{
				m_pkDeadEvent = event_create(dead_event, pEventInfo, 0);
			}
			else
			{
				m_pkDeadEvent = event_create(dead_event, pEventInfo, bImmediateDead ? 1 : PASSES_PER_SEC(3));
			}
		}
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();

	if (IsPC())
		WindowCloseAll();
}

struct FuncSetLastAttacked
{
	FuncSetLastAttacked(DWORD dwTime) : m_dwTime(dwTime)
	{
	}

	void operator () (LPCHARACTER ch)
	{
		ch->SetLastAttacked(m_dwTime);
	}

	DWORD m_dwTime;
};

void CHARACTER::SetLastAttacked(DWORD dwTime)
{
	assert(m_pkMobInst != NULL);

	m_pkMobInst->m_dwLastAttackedTime = dwTime;
	m_pkMobInst->m_posLastAttacked = GetXYZ();
}

void CHARACTER::SendDamagePacket(LPCHARACTER pAttacker, DAM_LL Damage, BYTE DamageFlag)
{
	if (IsPC() == true || (pAttacker->IsPC() == true && pAttacker->GetTarget() == this))
	{
		TPacketGCDamageInfo damageInfo;
		memset(&damageInfo, 0, sizeof(TPacketGCDamageInfo));

		damageInfo.header = HEADER_GC_DAMAGE_INFO;
		damageInfo.dwVID = (DWORD)GetVID();
		damageInfo.flag = DamageFlag;
		damageInfo.damage = Damage;

		if (GetDesc() != NULL)
		{
			GetDesc()->Packet(&damageInfo, sizeof(TPacketGCDamageInfo));
		}

		if (pAttacker->GetDesc() != NULL)
		{
			pAttacker->GetDesc()->Packet(&damageInfo, sizeof(TPacketGCDamageInfo));
		}
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		if (pAttacker->IsPC())
		{
			if (IsPC())	
			{
				if (Damage > pAttacker->GetStats(STATS_PLAYER_DAMAGE))
					pAttacker->PointChange(POINT_PLAYER_DAMAGE, Damage);

				pAttacker->UpdateExtBattlePassMissionProgress(DAMAGE_PLAYER, Damage, GetLevel());
			}
			else 
			{
				if (IsStone())
				{
					if (Damage > pAttacker->GetStats(STATS_STONE_DAMAGE))
						pAttacker->PointChange(POINT_STONE_DAMAGE, Damage);

#ifdef ENABLE_EXTENDED_BATTLE_PASS
					pAttacker->UpdateExtBattlePassMissionProgress(DAMAGE_METIN, Damage, GetRaceNum());

					if (Damage >= 75000)
						pAttacker->UpdateExtBattlePassMissionProgress(DAMAGE_METIN_REACH, 1, GetRaceNum());
#endif

				}
				else if (GetMobRank() == MOB_RANK_BOSS)
				{
					if (Damage > pAttacker->GetStats(STATS_BOSS_DAMAGE))
						pAttacker->PointChange(POINT_BOSS_DAMAGE, Damage);
				}
				else
				{
					if (Damage > pAttacker->GetStats(STATS_MONSTER_DAMAGE))
						pAttacker->PointChange(POINT_MONSTER_DAMAGE, Damage);
					pAttacker->UpdateExtBattlePassMissionProgress(DAMAGE_MONSTER, Damage, GetRaceNum());
				}
			}
		}
#endif
	}
}

// Arguments
// Return value
//    true		: dead
//    false		: not dead yet

bool CHARACTER::Damage(LPCHARACTER pAttacker, DAM_LL dam, EDamageType type) // returns true if dead
{
#ifdef ENABLE_OBSERVER_DAMAGE_FIX
	if (IsPC() && IsObserverMode()) {
		return false;
	}
#endif

#ifdef ENABLE_AFK_MODE_SYSTEM
	if (pAttacker && pAttacker->IsPC() && pAttacker->IsAway())
	{
		if(IsPC())
		return false;
		
		if (!CanMove())
		return false;
	}
#endif

#ifdef ENABLE_AFTERDEATH_SHIELD
	if(IsAffectFlag(AFF_AFTERDEATH_SHIELD))
		return false;
#endif

#ifdef ENABLE_NEWSTUFF
	if (pAttacker && IsStone() && pAttacker->IsPC())
	{
		if (GetEmpire() && GetEmpire() == pAttacker->GetEmpire())
		{
#ifdef ENABLE_RENEWAL_PVP
				bool SendDamageBlock = true;
				if (IsPC())
				{
					if (IsInFight() && pvpSettings[PVP_MISS_HITS] == false)
						SendDamageBlock = false;
				}
				if (SendDamageBlock)
				{
					SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
					return false;
				}
#else

				SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
				return false;
#endif
		}
	}
#endif
	if (DAMAGE_TYPE_MAGIC == type)
	{
		dam = (DAM_LL)((DAM_DOUBLE)dam * (100 + (pAttacker->GetPoint(POINT_MAGIC_ATT_BONUS_PER) + pAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100.f + 0.5f);
	}

	if (type != DAMAGE_TYPE_NORMAL && type != DAMAGE_TYPE_NORMAL_RANGE)
	{
		if (IsAffectFlag(AFF_TERROR))
		{
			int pct = GetSkillPower(SKILL_TERROR) / 400;

			if (number(1, 100) <= pct)
				return false;
		}
	}

	HP_LL iCurHP = GetHP();
	int iCurSP = GetSP();

	bool IsCritical = false;
	bool IsPenetrate = false;

	if (type == DAMAGE_TYPE_MELEE || type == DAMAGE_TYPE_RANGE || type == DAMAGE_TYPE_MAGIC)
	{
		if (pAttacker)
		{
			int64_t iCriticalPct = pAttacker->GetPoint(POINT_CRITICAL_PCT);

			if (iCriticalPct)
			{
				if (iCriticalPct >= 10)
					iCriticalPct = 5 + (iCriticalPct - 10) / 4;
				else
					iCriticalPct /= 2;

				iCriticalPct -= GetPoint(POINT_RESIST_CRITICAL);

				if (number(1, 125) <= iCriticalPct)
				{
					IsCritical = true;
					if (iCriticalPct > 100)
					{
						int64_t criticalDam = dam * ((static_cast<double>(iCriticalPct) - 100.0) / 3.0 / 100.0);
						dam *= 2;
						dam += criticalDam;
					}
					else
					{
						dam *= 2;
					}
					EffectPacket(SE_CRITICAL);

					if (IsAffectFlag(AFF_MANASHIELD))
					{
						RemoveAffect(AFF_MANASHIELD);
					}
				}
			}

			int64_t iPenetratePct = pAttacker->GetPoint(POINT_PENETRATE_PCT);

			if (iPenetratePct)
			{
				{
					CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_RESIST_PENETRATE);

					if (NULL != pkSk)
					{
						pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_RESIST_PENETRATE) / 100.0f);

						iPenetratePct -= static_cast<int>(pkSk->kPointPoly.Eval());
					}
				}

				if (iPenetratePct >= 10)
				{
					iPenetratePct = 5 + (iPenetratePct - 10) / 4;
				}
				else
				{
					iPenetratePct /= 2;
				}

				iPenetratePct -= GetPoint(POINT_RESIST_PENETRATE);

				if (number(1, 100) <= iPenetratePct)
				{
					IsPenetrate = true;
					dam += GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100;

					if (IsAffectFlag(AFF_MANASHIELD))
					{
						RemoveAffect(AFF_MANASHIELD);
					}
#ifdef ENABLE_EFFECT_PENETRATE
					EffectPacket(SE_PENETRATE);
#endif
				}
			}
		}
	}

	else if (type == DAMAGE_TYPE_NORMAL || type == DAMAGE_TYPE_NORMAL_RANGE)
	{
		if (type == DAMAGE_TYPE_NORMAL)
		{
			if (GetPoint(POINT_BLOCK) && number(1, 125) <= GetPoint(POINT_BLOCK))
			{
				SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
				return false;
			}
		}
		else if (type == DAMAGE_TYPE_NORMAL_RANGE)
		{
			if (GetPoint(POINT_DODGE) && number(1, 110) <= GetPoint(POINT_DODGE))
			{
#ifdef ENABLE_RENEWAL_PVP
				bool SendDamageDodge = true;
				if (IsPC())
				{
					if (IsInFight() && pvpSettings[PVP_MISS_HITS] == false)
						SendDamageDodge = false;
				}
				if (SendDamageDodge)
				{
					SendDamagePacket(pAttacker, 0, DAMAGE_DODGE);
					return false;
				}
#else

				SendDamagePacket(pAttacker, 0, DAMAGE_DODGE);
				return false;
#endif
			}
		}

		if (IsAffectFlag(AFF_JEONGWIHON))
			dam = (DAM_LL)(dam * (100 + GetSkillPower(SKILL_JEONGWI) * 25 / 100) / 100);

		if (IsAffectFlag(AFF_TERROR))
			dam = (DAM_LL)(dam * (95 - GetSkillPower(SKILL_TERROR) / 5) / 100);

		if (IsAffectFlag(AFF_HOSIN))
		{
			if (IsPC() && pAttacker->IsPC())
				dam = dam * (100 - (GetPoint(POINT_RESIST_NORMAL_DAMAGE) / 2)) / 100;
			else
				dam = dam * (100 - GetPoint(POINT_RESIST_NORMAL_DAMAGE)) / 100;
		}
			

		if (pAttacker)
		{
			if (type == DAMAGE_TYPE_NORMAL)
			{
				if (GetPoint(POINT_REFLECT_MELEE))
				{
					DAM_LL reflectDamage = dam * GetPoint(POINT_REFLECT_MELEE) / 100;

					if (pAttacker->IsImmune(IMMUNE_REFLECT))
						reflectDamage = DAM_LL(reflectDamage / 3.0f + 0.5f);

					pAttacker->Damage(this, reflectDamage, DAMAGE_TYPE_SPECIAL);
				}
			}

			int64_t iCriticalPct = pAttacker->GetPoint(POINT_CRITICAL_PCT);

			if (iCriticalPct)
			{
				iCriticalPct -= GetPoint(POINT_RESIST_CRITICAL);

				if (number(1, 125) <= iCriticalPct)
				{
					IsCritical = true;
					if (iCriticalPct > 100)
					{
						int64_t criticalDam = dam * ((static_cast<double>(iCriticalPct) - 100.0) / 2.5 / 100.0);
						dam *= 2;
						dam += criticalDam;
						
					}
					else
					{
						dam *= 2;
					}
					EffectPacket(SE_CRITICAL);
				}
			}

			int iPenetratePct = pAttacker->GetPoint(POINT_PENETRATE_PCT);

			if (!IsPC())
				iPenetratePct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_PENETRATE_BONUS);
			
#ifdef ENABLE_RENEWAL_PVP
			if (IsPC() && IsInFight() && pvpSettings[PVP_CRITICAL_DAMAGE_SKILLS] == false)
				iCriticalPct = 0;
#endif

			{
				CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_RESIST_PENETRATE);

				if (NULL != pkSk)
				{
					pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_RESIST_PENETRATE) / 100.0f);

					iPenetratePct -= static_cast<int>(pkSk->kPointPoly.Eval());
				}
			}

			if (iPenetratePct)
			{
				iPenetratePct -= GetPoint(POINT_RESIST_PENETRATE);

				if (number(1, 100) <= iPenetratePct)
				{
					IsPenetrate = true;
					dam += GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100;
#ifdef ENABLE_EFFECT_PENETRATE
					EffectPacket(SE_PENETRATE);
#endif
				}
			}

			if (pAttacker->GetPoint(POINT_STEAL_HP))
			{
				int pct = 1;

				if (number(1, 10) <= pct)
				{
					HP_LL iHP = MIN(dam, MAX(0, iCurHP)) * pAttacker->GetPoint(POINT_STEAL_HP) / 100;

					if (iHP > 0 && GetHP() >= iHP)
					{
						CreateFly(FLY_HP_SMALL, pAttacker);
						pAttacker->PointChange(POINT_HP, iHP);
						PointChange(POINT_HP, -iHP);
					}
				}
			}

			if (pAttacker->GetPoint(POINT_STEAL_SP))
			{
				int pct = 1;

				if (number(1, 10) <= pct)
				{
					HP_LL iCur;

					if (IsPC())
						iCur = iCurSP;
					else
						iCur = iCurHP;

					HP_LL iSP = MIN(dam, MAX(0, iCur)) * pAttacker->GetPoint(POINT_STEAL_SP) / 100;

					if (iSP > 0 && iCur >= iSP)
					{
						CreateFly(FLY_SP_SMALL, pAttacker);
						pAttacker->PointChange(POINT_SP, iSP);

						if (IsPC())
							PointChange(POINT_SP, -iSP);
					}
				}
			}

			if (pAttacker->GetPoint(POINT_HIT_HP_RECOVERY) && number(0, 4) > 0)
			{
				DAM_LL i = ((iCurHP >= 0) ? MIN(dam, iCurHP) : dam) * pAttacker->GetPoint(POINT_HIT_HP_RECOVERY) / 100; //@fixme107


				if (GetMobRank() == MOB_RANK_BOSS)
				{
					i /= 30; //@boss hp cok calma amk
				}

				if (i)
				{
					CreateFly(FLY_HP_SMALL, pAttacker);
					pAttacker->PointChange(POINT_HP, i);
				}
			}

			if (pAttacker->GetPoint(POINT_HIT_SP_RECOVERY) && number(0, 4) > 0)
			{
				DAM_LL i = ((iCurHP >= 0) ? MIN(dam, iCurHP) : dam) * pAttacker->GetPoint(POINT_HIT_SP_RECOVERY) / 100; //@fixme107

				if (i)
				{
					CreateFly(FLY_SP_SMALL, pAttacker);
					pAttacker->PointChange(POINT_SP, i);
				}
			}

			if (pAttacker->GetPoint(POINT_MANA_BURN_PCT))
			{
				if (number(1, 100) <= pAttacker->GetPoint(POINT_MANA_BURN_PCT))
					PointChange(POINT_SP, -50);
			}
		}
	}

	switch (type)
	{
	case DAMAGE_TYPE_NORMAL:
	case DAMAGE_TYPE_NORMAL_RANGE:
		if (pAttacker)
		{
#ifdef ENABLE_EXTRA_APPLY_BONUS
			if (pAttacker->GetPoint(POINT_ATTBONUS_PVM_AVG) && IsPvM())
			{
				dam = dam * (100 + (pAttacker->GetPoint(POINT_ATTBONUS_PVM_AVG)) + (pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS))) / 100;
			}
			else
			{
				if (pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS))
					dam = dam * (100 + pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS)) / 100;
			}
#else
			if (pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS))
				dam = dam * (100 + pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS)) / 100;
#endif
		}
		dam = dam * (100 - MIN(99, GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS))) / 100;
		break;

	case DAMAGE_TYPE_MELEE:
	case DAMAGE_TYPE_RANGE:
	case DAMAGE_TYPE_FIRE:
	case DAMAGE_TYPE_ICE:
	case DAMAGE_TYPE_ELEC:
	case DAMAGE_TYPE_MAGIC:
		if (pAttacker)
			if (pAttacker->GetPoint(POINT_SKILL_DAMAGE_BONUS))
				dam = dam * (100 + pAttacker->GetPoint(POINT_SKILL_DAMAGE_BONUS)) / 100;

		dam = dam * (100 - MIN(99, GetPoint(POINT_SKILL_DEFEND_BONUS))) / 100;
		break;

	default:
		break;
	}

	if (IsAffectFlag(AFF_MANASHIELD))
	{
		DAM_LL iDamageSPPart = dam / 3;
		DAM_LL iDamageToSP = iDamageSPPart * GetPoint(POINT_MANASHIELD) / 100;
		DAM_LL iSP = GetSP();

		if (iDamageToSP <= iSP)
		{
			PointChange(POINT_SP, -iDamageToSP);
			dam -= iDamageSPPart;
		}
		else
		{
			PointChange(POINT_SP, -GetSP());
			dam -= iSP * 100 / MAX(GetPoint(POINT_MANASHIELD), 1);
		}
	}

	if (GetPoint(POINT_MALL_DEFBONUS) > 0)
	{
		DAM_LL dec_dam = MIN(200, dam * GetPoint(POINT_MALL_DEFBONUS) / 100);
		dam -= dec_dam;
	}

	if (pAttacker)
	{
		if (pAttacker->GetPoint(POINT_MALL_ATTBONUS) > 0)
		{
			DAM_LL add_dam = MIN(300, dam * pAttacker->GetLimitPoint(POINT_MALL_ATTBONUS) / 100);
			dam += add_dam;
		}

		if (pAttacker->IsPC())
		{
			int iEmpire = pAttacker->GetEmpire();
			long lMapIndex = pAttacker->GetMapIndex();
			int iMapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex);

			if (iEmpire && iMapEmpire && iEmpire != iMapEmpire)
			{
				dam = dam * 9 / 10;
			}

			if (!IsPC() && GetMonsterDrainSPPoint())
			{
				int iDrain = GetMonsterDrainSPPoint();

				if (iDrain <= pAttacker->GetSP())
					pAttacker->PointChange(POINT_SP, -iDrain);
				else
				{
					int iSP = pAttacker->GetSP();
					pAttacker->PointChange(POINT_SP, -iSP);
				}
			}
		}
		else if (pAttacker->IsGuardNPC())
		{
			SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_NO_REWARD);
			Stun();
			return true;
		}
	}

	if (!GetSectree() || GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK))
		return false;

	if (!IsPC())
	{
		if (m_pkParty && m_pkParty->GetLeader())
			m_pkParty->GetLeader()->SetLastAttacked(get_dword_time());
		else
			SetLastAttacked(get_dword_time());

		// MonsterChat(MONSTER_CHAT_ATTACKED);
	}

	if (IsStun())
	{
		Dead(pAttacker);
		return true;
	}

	if (IsDead())
		return true;

	if (type == DAMAGE_TYPE_POISON)
	{
		if (GetHP() - dam <= 0)
		{
			dam = GetHP() - 1;
		}
	}
#ifdef ENABLE_WOLFMAN_CHARACTER
	else if (type == DAMAGE_TYPE_BLEEDING)
	{
		if (GetHP() - dam <= 0)
		{
			dam = GetHP();
		}
	}
#endif

	if (pAttacker && pAttacker->IsPC())
	{
		int iDmgPct = CHARACTER_MANAGER::instance().GetUserDamageRate(pAttacker);
		dam = dam * iDmgPct / 100;
	}

	if (IsMonster() && IsStoneSkinner())
	{
		if (GetHPPct() < GetMobTable().bStoneSkinPoint)
			dam /= 2;
	}

	if (pAttacker)
	{
		if (pAttacker->IsMonster() && pAttacker->IsDeathBlower())
		{
			if (pAttacker->IsDeathBlow())
			{
#ifdef ENABLE_DEATHBLOW_ALL_RACES_FIX
				if (number(JOB_WARRIOR, JOB_MAX_NUM - 1) == GetJob())
#else
				if (number(1, 4) == GetJob())
#endif
				{
					dam = dam * 4;
				}
			}
		}

		BYTE damageFlag = 0;

		if (type == DAMAGE_TYPE_POISON)
			damageFlag = DAMAGE_POISON;
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
		else if (type == DAMAGE_TYPE_BLEEDING)
			damageFlag = DAMAGE_BLEEDING;
#elif defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_MOB_BLEEDING_AS_POISON)
		else if (type == DAMAGE_TYPE_BLEEDING)
			damageFlag = DAMAGE_POISON;
#endif
		else
			damageFlag = DAMAGE_NORMAL;

		if (IsCritical == true)
			damageFlag |= DAMAGE_CRITICAL;

		if (IsPenetrate == true)
			damageFlag |= DAMAGE_PENETRATE;

		DAM_DOUBLE damMul = this->GetDamMul();
		DAM_DOUBLE tempDam = dam;
		dam = tempDam * damMul + 0.5f;
		
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
        if (const auto is_boss_in_ranking{
                bossdamageranking::boss_dmg_ranking_manager().is_boss_in_ranking(GetRaceNum())};
            is_boss_in_ranking)
        {
            bossdamageranking::boss_dmg_ranking_manager().damage_process(
                pAttacker, {GetRaceNum(), static_cast<DWORD>(GetVID())}, dam);
        }
#endif

		// @fixme187 BEGIN
		if (pAttacker->IsPC())
		{
			if (pAttacker->IsAffectFlag(AFF_REVIVE_INVISIBLE) || IsAffectFlag(AFF_REVIVE_INVISIBLE))
			{
				dam = 0;
				damageFlag = DAMAGE_DODGE;
			}
		}
		// @fixme187 END

		if (pAttacker)
			SendDamagePacket(pAttacker, dam, damageFlag);
	}

	if (!cannot_dead)
	{
		if (GetHP() - dam <= 0) // @fixme137
			dam = GetHP();
		PointChange(POINT_HP, -dam, false);
	}

	if (pAttacker && dam > 0 && IsNPC())
	{
		TDamageMap::iterator it = m_map_kDamage.find(pAttacker->GetVID());

		if (it == m_map_kDamage.end())
		{
			m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(dam, 0)));
			it = m_map_kDamage.find(pAttacker->GetVID());
		}
		else
		{
			it->second.iTotalDamage += dam;
		}
		StartRecoveryEvent();

		if (type != DAMAGE_TYPE_POISON) //@Lightwork#66
			UpdateAggrPointEx(pAttacker, type, dam, it->second);
	}

	if (GetHP() <= 0)
	{
#ifdef ENABLE_BLOW_FIX
		if (IsPC())
			Stun();
		else
			Dead(pAttacker);
#else
		Stun();
#endif

		if (pAttacker && !pAttacker->IsNPC())
			m_dwKillerPID = pAttacker->GetPlayerID();
		else
			m_dwKillerPID = 0;
	}

	return false;
}

void CHARACTER::DistributeHP(LPCHARACTER pkKiller)
{
	if (pkKiller->GetDungeon())
		return;
}

#define NEW_GET_LVDELTA(me, victim) aiPercentByDeltaLev[MINMAX(0, (victim + 15) - me, MAX_EXP_DELTA_OF_LEV - 1)]
typedef long double rate_t;
static void GiveExp(LPCHARACTER from, LPCHARACTER to, int iExp)
{
	// decrease/increase exp based on player<>mob level
	rate_t lvFactor = static_cast<rate_t>(NEW_GET_LVDELTA(to->GetLevel(), from->GetLevel())) / 100.0L;
	iExp *= lvFactor;
	// start calculating rate exp bonus
	rate_t rateFactor = 100;

	rateFactor += CPrivManager::instance().GetPriv(to, PRIV_EXP_PCT);

#ifdef ENABLE_EVENT_MANAGER
	const auto event = CHARACTER_MANAGER::Instance().CheckEventIsActive(EXP_EVENT, to->GetEmpire());

	if(event != NULL)
		rateFactor += event->value[0];
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
	if (to->GetNewPetSystem() && to->GetNewPetSystem()->IsSummon())
	{
		if (to->GetNewPetSystem()->GetLevel() < 120 && to->GetNewPetSystem()->CanExp())
		{
			to->GetNewPetSystem()->SetExp(iExp);
			from->CreateFly(FLY_EXP, to->GetNewPetSystem()->GetPetChar());
		}
	}
#endif

	if (to->IsEquipUniqueItem(UNIQUE_ITEM_LARBOR_MEDAL))
		rateFactor += 20;
	if (to->GetMapIndex() >= 660000 && to->GetMapIndex() < 670000)
		rateFactor += 20;

	if (to->GetPoint(POINT_EXP_DOUBLE_BONUS))
		if (number(1, 100) <= to->GetPoint(POINT_EXP_DOUBLE_BONUS))
			rateFactor += 30;

	if (to->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_EXP))
		rateFactor += 50;
	
	if (to->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_EXP_PLUS))
		rateFactor += 100;
		
	if (to->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		rateFactor += 50;

	if (to->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_EXP))
		rateFactor += 50;

	rateFactor += to->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_EXP_BONUS);
	rateFactor += to->GetPoint(POINT_RAMADAN_CANDY_BONUS_EXP);
	rateFactor += to->GetPoint(POINT_MALL_EXPBONUS);
	rateFactor += to->GetPoint(POINT_EXP);
	rateFactor = rateFactor * static_cast<rate_t>(CHARACTER_MANAGER::instance().GetMobExpRate(to)) / 100.0L;
	// fix underflow formula
	iExp = std::max<int>(0, iExp);
	rateFactor = std::max<rate_t>(100.0L, rateFactor);
	// apply calculated rate bonus
	iExp *= (rateFactor / 100.0L);
	// you can get at maximum only 10% of the total required exp at once (so, you need to kill at least 10 mobs to level up) (useless)
	iExp = MIN(to->GetNextExp() / 10, iExp);

	// it recalculate the given exp if the player level is greater than the exp_table size (useless)
	iExp = AdjustExpByLevel(to, iExp);

	to->PointChange(POINT_EXP, iExp, true);
	from->CreateFly(FLY_EXP, to);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	to->UpdateExtBattlePassMissionProgress(EXP_COLLECT, iExp, from->GetRaceNum());
#endif
}


namespace NPartyExpDistribute
{
	struct FPartyTotaler
	{
		int		total;
		int		member_count;
		int		x, y;

		FPartyTotaler(LPCHARACTER center)
			: total(0), member_count(0), x(center->GetX()), y(center->GetY())
		{};

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
			{
				total += __GetPartyExpNP(ch->GetLevel());

				++member_count;
			}
		}
	};

	struct FPartyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		DWORD		_iExp;
		int		m_iMode;
		int		m_iMemberCount;

		FPartyDistributor(LPCHARACTER center, int member_count, int total, DWORD iExp, int iMode)
			: total(total), c(center), x(center->GetX()), y(center->GetY()), _iExp(iExp), m_iMode(iMode), m_iMemberCount(member_count)
		{
			if (m_iMemberCount == 0)
				m_iMemberCount = 1;
		};

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
			{
				DWORD iExp2 = 0;

				switch (m_iMode)
				{
				case PARTY_EXP_DISTRIBUTION_NON_PARITY:
					iExp2 = (DWORD)(_iExp * (float)__GetPartyExpNP(ch->GetLevel()) / total);
					break;

				case PARTY_EXP_DISTRIBUTION_PARITY:
					iExp2 = _iExp / m_iMemberCount;
					break;

				default:
					sys_err("Unknown party exp distribution mode %d", m_iMode);
					return;
				}

				GiveExp(c, ch, iExp2);
			}
		}
	};
}

typedef struct SDamageInfo
{
	DAM_LL iDam;
	LPCHARACTER pAttacker;
	LPPARTY pParty;

	void Clear()
	{
		pAttacker = NULL;
		pParty = NULL;
	}

	inline void Distribute(LPCHARACTER ch, int iExp)
	{
		if (pAttacker)
			GiveExp(ch, pAttacker, iExp);
		else if (pParty)
		{
			NPartyExpDistribute::FPartyTotaler f(ch);
			pParty->ForEachOnlineMember(f);

			if (pParty->IsPositionNearLeader(ch))
				iExp = iExp * (100 + pParty->GetExpBonusPercent()) / 100;

			if (pParty->GetExpCentralizeCharacter())
			{
				LPCHARACTER tch = pParty->GetExpCentralizeCharacter();

				if (DISTANCE_APPROX(ch->GetX() - tch->GetX(), ch->GetY() - tch->GetY()) <= PARTY_DEFAULT_RANGE)
				{
					int iExpCenteralize = (int)(iExp * 0.05f);
					iExp -= iExpCenteralize;

					GiveExp(ch, pParty->GetExpCentralizeCharacter(), iExpCenteralize);
				}
			}

			NPartyExpDistribute::FPartyDistributor fDist(ch, f.member_count, f.total, iExp, pParty->GetExpDistributionMode());
			pParty->ForEachOnlineMember(fDist);
		}
	}
} TDamageInfo;

#ifdef ENABLE_KILL_EVENT_FIX
LPCHARACTER CHARACTER::GetMostAttacked() {
	DAM_LL iMostDam = -1;
	LPCHARACTER pkChrMostAttacked = NULL;
	auto it = m_map_kDamage.begin();

	while (it != m_map_kDamage.end()) {
		//* getting information from the iterator
		const VID& c_VID = it->first;
		const DAM_LL iDam = it->second.iTotalDamage;

		//* increasing the iterator
		++it;

		//* finding the character from his vid
		LPCHARACTER pAttacker = CHARACTER_MANAGER::instance().Find(c_VID);

		//* if the attacked is now offline
		if (!pAttacker)
			continue;

		//* if the attacker is not a player
		if (pAttacker->IsNPC())
			continue;

		//* if the player is too far
		if (DISTANCE_APPROX(GetX() - pAttacker->GetX(), GetY() - pAttacker->GetY()) > 5000)
			continue;

		if (iDam > iMostDam) {
			pkChrMostAttacked = pAttacker;
			iMostDam = iDam;
		}
	}

	return pkChrMostAttacked;
}
#endif

LPCHARACTER CHARACTER::DistributeExp()
{
	int iExpToDistribute = GetExp();

	if (iExpToDistribute <= 0)
		return NULL;

	DAM_LL	iTotalDam = 0;
	LPCHARACTER pkChrMostAttacked = NULL;
	DAM_LL iMostDam = 0;

	typedef std::vector<TDamageInfo> TDamageInfoTable;
	TDamageInfoTable damage_info_table;
	std::map<LPPARTY, TDamageInfo> map_party_damage;

	damage_info_table.reserve(m_map_kDamage.size());

	TDamageMap::iterator it = m_map_kDamage.begin();

	while (it != m_map_kDamage.end())
	{
		const VID& c_VID = it->first;
		DAM_LL iDam = it->second.iTotalDamage;

		++it;

#ifdef ENABLE_PARTY_EXP_FIX
		LPCHARACTER pAttacker = CHARACTER_MANAGER::instance().Find(c_VID);

		if (!pAttacker || !pAttacker->IsPC() || pAttacker->IsNPC())// @fixme205
			continue;

		// Block exp award based on distance
		// Groups have DOUBLE the exp distance to prevent the "exploit" where the party leader gets full exp bonus and some party members don't get exp.
		int dist = DISTANCE_APPROX(GetX() - pAttacker->GetX(), GetY() - pAttacker->GetY());
		if (dist > 10000 || (dist > 5000 && !pAttacker->GetParty()))
			continue;
#else
		LPCHARACTER pAttacker = CHARACTER_MANAGER::instance().Find(c_VID);

		if (!pAttacker || pAttacker->IsNPC() || DISTANCE_APPROX(GetX() - pAttacker->GetX(), GetY() - pAttacker->GetY()) > 5000)// @fixme205
			continue;
#endif

		iTotalDam += iDam;
		if (!pkChrMostAttacked || iDam > iMostDam)
		{
			pkChrMostAttacked = pAttacker;
			iMostDam = iDam;
		}

		if (pAttacker->GetParty())
		{
			std::map<LPPARTY, TDamageInfo>::iterator it = map_party_damage.find(pAttacker->GetParty());
			if (it == map_party_damage.end())
			{
				TDamageInfo di;
				di.iDam = iDam;
				di.pAttacker = NULL;
				di.pParty = pAttacker->GetParty();
				map_party_damage.insert(std::make_pair(di.pParty, di));
			}
			else
			{
				it->second.iDam += iDam;
			}
		}
		else
		{
			TDamageInfo di;

			di.iDam = iDam;
			di.pAttacker = pAttacker;
			di.pParty = NULL;
			damage_info_table.push_back(di);
		}
	}

	for (std::map<LPPARTY, TDamageInfo>::iterator it = map_party_damage.begin(); it != map_party_damage.end(); ++it)
	{
		damage_info_table.push_back(it->second);
	}

	SetExp(0);

	if (iTotalDam == 0)
		return NULL;

	if (m_pkChrStone)
	{
		int iExp = iExpToDistribute >> 1;
		m_pkChrStone->SetExp(m_pkChrStone->GetExp() + iExp);
		iExpToDistribute -= iExp;
	}

	if (damage_info_table.empty())
		return NULL;

	DistributeHP(pkChrMostAttacked);

	{
		TDamageInfoTable::iterator di = damage_info_table.begin();
		{
			TDamageInfoTable::iterator it;

			for (it = damage_info_table.begin(); it != damage_info_table.end(); ++it)
			{
				if (it->iDam > di->iDam)
					di = it;
			}
		}

		int	iExp = iExpToDistribute / 5;
		iExpToDistribute -= iExp;

		DAM_DOUBLE fPercent = (DAM_DOUBLE)di->iDam / iTotalDam;

		if (fPercent > 1.0f)
		{
			sys_err("DistributeExp percent over 1.0 (fPercent %f name %s)", fPercent, di->pAttacker->GetName());
			fPercent = 1.0f;
		}

		iExp += (int)(iExpToDistribute * fPercent);
		di->Distribute(this, iExp);

		if (fPercent == 1.0f)
			return pkChrMostAttacked;

		di->Clear();
	}

	{
		TDamageInfoTable::iterator it;

		for (it = damage_info_table.begin(); it != damage_info_table.end(); ++it)
		{
			TDamageInfo& di = *it;

			DAM_DOUBLE fPercent = (DAM_DOUBLE)di.iDam / iTotalDam;

			if (fPercent > 1.0f)
			{
				sys_err("DistributeExp percent over 1.0 (fPercent %f name %s)", fPercent, di.pAttacker->GetName());
				fPercent = 1.0f;
			}
			di.Distribute(this, (int)(iExpToDistribute * fPercent));
		}
	}

	return pkChrMostAttacked;
}

bool CHARACTER::GetArrowAndBow(LPITEM* ppkBow, LPITEM* ppkArrow)
{
	LPITEM pkBow;

	if (!(pkBow = GetWear(WEAR_WEAPON)) || pkBow->GetProto()->bSubType != WEAPON_BOW)
	{
		return false;
	}

	LPITEM pkArrow;

	if (!(pkArrow = GetWear(WEAR_ARROW)) || pkArrow->GetType() != ITEM_WEAPON ||
		pkArrow->GetProto()->bSubType != WEAPON_ARROW)
	{
		return false;
	}

	*ppkBow = pkBow;
	*ppkArrow = pkArrow;

	return true;
}

class CFuncShoot
{
public:
	LPCHARACTER	m_me;
	BYTE		m_bType;
	bool		m_bSucceed;

	CFuncShoot(LPCHARACTER ch, BYTE bType) : m_me(ch), m_bType(bType), m_bSucceed(FALSE)
	{
	}

	void operator () (DWORD dwTargetVID)
	{
		if (m_bType > 1)
		{
			if (g_bSkillDisable)
				return;

			m_me->m_SkillUseInfo[m_bType].SetMainTargetVID(dwTargetVID);
			/*if (m_bType == SKILL_BIPABU || m_bType == SKILL_KWANKYEOK)
			  m_me->m_SkillUseInfo[m_bType].ResetHitCount();*/
		}

		LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(dwTargetVID);

		if (!pkVictim)
			return;

		if (!battle_is_attackable(m_me, pkVictim))
			return;

		if (m_me->IsNPC())
		{
			if (DISTANCE_APPROX(m_me->GetX() - pkVictim->GetX(), m_me->GetY() - pkVictim->GetY()) > 5000)
				return;
		}

#ifdef ENABLE_LMW_PROTECTION
		if (m_me->IsPC() && m_bType > 0 && m_me->IsSkillCooldown(m_bType, static_cast<float> (m_me->GetSkillPower(m_bType) / 100.0f)))
		{
			return;
		}
#endif

#ifdef ENABLE_AFTERDEATH_SHIELD
			if(m_me->IsAffectFlag(AFF_AFTERDEATH_SHIELD))
				m_me->RemoveShieldAffect();
#endif

		LPITEM pkBow, pkArrow;

		switch (m_bType)
		{
		case 0:
		{
			DAM_LL iDam = 0;

			if (m_me->IsPC())
			{
				if (m_me->GetJob() != JOB_ASSASSIN)
					return;

				if (!m_me->GetArrowAndBow(&pkBow, &pkArrow))
					return;

#ifdef ENABLE_CRASH_CORE_ARROW_FIX
				if (!pkBow)
					break;
#endif
#ifdef ENABLE_BOT_CONTROL
				if (m_me->IsBotControl())
					return;
				if (pkVictim->IsPC() && pkVictim->IsBotControl())
					return;
#endif
				if (m_me->GetSkillGroup() != 0)
					if (!m_me->IsNPC() && m_me->GetSkillGroup() != 2)
					{
						if (m_me->GetSP() < 5)
							return;

						m_me->PointChange(POINT_SP, -5);
					}
#ifdef ENABLE_LMW_PROTECTION
				if (pkVictim->IsStone() || pkVictim->GetMobRank() >= MOB_RANK_BOSS)
				{
					m_me->SetAttackCount(pkVictim->GetVID());
				}
#endif
				iDam = CalcArrowDamage(m_me, pkVictim, pkBow, pkArrow);
			}
			else
				iDam = CalcMeleeDamage(m_me, pkVictim);

			NormalAttackAffect(m_me, pkVictim);

			iDam = iDam * (100 - pkVictim->GetPoint(POINT_RESIST_BOW)) / 100;
			m_me->OnMove(true);
			pkVictim->OnMove();

			if (pkVictim->CanBeginFight())
				pkVictim->BeginFight(m_me);

			pkVictim->Damage(m_me, iDam, DAMAGE_TYPE_NORMAL_RANGE);

		}
		break;

		case 1:
		{
			DAM_LL iDam;

			if (m_me->IsPC())
				return;

			iDam = CalcMagicDamage(m_me, pkVictim);

			NormalAttackAffect(m_me, pkVictim);

#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
			const DAM_LL resist_magic = MINMAX(0, pkVictim->GetPoint(POINT_RESIST_MAGIC), 100);
			const DAM_LL resist_magic_reduction = MINMAX(0, (m_me->GetJob() == JOB_SURA) ? m_me->GetPoint(POINT_RESIST_MAGIC_REDUCTION) / 2 : m_me->GetPoint(POINT_RESIST_MAGIC_REDUCTION), 50);
			const DAM_LL total_res_magic = MINMAX(0, resist_magic - resist_magic_reduction, 100);
			iDam = iDam * (100 - total_res_magic) / 100;
#else
			iDam = iDam * (100 - pkVictim->GetPoint(POINT_RESIST_MAGIC)) / 100;
#endif
			m_me->OnMove(true);
			pkVictim->OnMove();

			if (pkVictim->CanBeginFight())
				pkVictim->BeginFight(m_me);

			pkVictim->Damage(m_me, iDam, DAMAGE_TYPE_MAGIC);
		}
		break;

		case SKILL_YEONSA:
		{
			{
				if (m_me->GetArrowAndBow(&pkBow, &pkArrow))
				{
#ifdef ENABLE_CRASH_CORE_ARROW_FIX
					if (!pkBow)
						break;
#endif
					m_me->OnMove(true);
					pkVictim->OnMove();

					if (pkVictim->CanBeginFight())
						pkVictim->BeginFight(m_me);

					m_me->ComputeSkill(m_bType, pkVictim);

					if (pkVictim->IsDead())
						break;
				}
				else
					break;
			}
		}
		break;

		case SKILL_KWANKYEOK:
		{
			if (m_me->GetArrowAndBow(&pkBow, &pkArrow))
			{
#ifdef ENABLE_CRASH_CORE_ARROW_FIX
				if (!pkBow)
					break;
#endif
				m_me->OnMove(true);
				pkVictim->OnMove();

				if (pkVictim->CanBeginFight())
					pkVictim->BeginFight(m_me);
				m_me->ComputeSkill(m_bType, pkVictim);
			}
		}
		break;

		case SKILL_GIGUNG:
		{
			if (m_me->GetArrowAndBow(&pkBow, &pkArrow))
			{
#ifdef ENABLE_CRASH_CORE_ARROW_FIX
				if (!pkBow)
					break;
#endif
				m_me->OnMove(true);
				pkVictim->OnMove();

				if (pkVictim->CanBeginFight())
					pkVictim->BeginFight(m_me);
				m_me->ComputeSkill(m_bType, pkVictim);
			}
		}

		break;
		case SKILL_HWAJO:
		{
			if (m_me->GetArrowAndBow(&pkBow, &pkArrow))
			{
#ifdef ENABLE_CRASH_CORE_ARROW_FIX
				if (!pkBow)
					break;
#endif
				m_me->OnMove(true);
				pkVictim->OnMove();

				if (pkVictim->CanBeginFight())
					pkVictim->BeginFight(m_me);
				m_me->ComputeSkill(m_bType, pkVictim);
			}
		}

		break;

		case SKILL_HORSE_WILDATTACK_RANGE:
		{
			if (m_me->GetArrowAndBow(&pkBow, &pkArrow))
			{
#ifdef ENABLE_CRASH_CORE_ARROW_FIX
				if (!pkBow)
					break;
#endif
				m_me->OnMove(true);
				pkVictim->OnMove();

				if (pkVictim->CanBeginFight())
					pkVictim->BeginFight(m_me);

				m_me->ComputeSkill(m_bType, pkVictim);
			}
		}

		break;

		case SKILL_MARYUNG:
			//case SKILL_GUMHWAN:
		case SKILL_TUSOK:
		case SKILL_BIPABU:
		case SKILL_NOEJEON:
			//@Lightwork#22
		case SKILL_YONGBI:
		case SKILL_GEOMPUNG:
		case SKILL_SANGONG:
		case SKILL_MAHWAN:
		case SKILL_PABEOB:
			//case SKILL_CURSE:
		{
			m_me->OnMove(true);
			pkVictim->OnMove();

			if (pkVictim->CanBeginFight())
				pkVictim->BeginFight(m_me);
			m_me->ComputeSkill(m_bType, pkVictim);
		}
		break;

		case SKILL_CHAIN:
		{
			m_me->OnMove(true);
			pkVictim->OnMove();

			if (pkVictim->CanBeginFight())
				pkVictim->BeginFight(m_me);
			m_me->ComputeSkill(m_bType, pkVictim);
		}
		break;
		default:
			sys_err("CFuncShoot: I don't know this type [%d] of range attack.", (int)m_bType);
			break;
		}

		m_bSucceed = TRUE;
	}
};

bool CHARACTER::Shoot(BYTE bType)
{
	if (!CanMove())
	{
		return false;
	}

	CFuncShoot f(this, bType);

	if (m_dwFlyTargetID != 0)
	{
		f(m_dwFlyTargetID);
		m_dwFlyTargetID = 0;
	}

	f = std::for_each(m_vec_dwFlyTargets.begin(), m_vec_dwFlyTargets.end(), f);
	m_vec_dwFlyTargets.clear();

	return f.m_bSucceed;
}

void CHARACTER::FlyTarget(DWORD dwTargetVID, long x, long y, BYTE bHeader)
{
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(dwTargetVID);
	TPacketGCFlyTargeting pack;

	//pack.bHeader	= HEADER_GC_FLY_TARGETING;
	pack.bHeader = (bHeader == HEADER_CG_FLY_TARGETING) ? HEADER_GC_FLY_TARGETING : HEADER_GC_ADD_FLY_TARGETING;
	pack.dwShooterVID = GetVID();

	if (pkVictim)
	{
		pack.dwTargetVID = pkVictim->GetVID();
		pack.x = pkVictim->GetX();
		pack.y = pkVictim->GetY();

		if (bHeader == HEADER_CG_FLY_TARGETING)
			m_dwFlyTargetID = dwTargetVID;
		else
			m_vec_dwFlyTargets.push_back(dwTargetVID);
	}
	else
	{
		pack.dwTargetVID = 0;
		pack.x = x;
		pack.y = y;
	}
	PacketAround(&pack, sizeof(pack), this);
}

LPCHARACTER CHARACTER::GetNearestVictim(LPCHARACTER pkChr)
{
	if (NULL == pkChr)
		pkChr = this;

	float fMinDist = 99999.0f;
	LPCHARACTER pkVictim = NULL;

	TDamageMap::iterator it = m_map_kDamage.begin();

	while (it != m_map_kDamage.end())
	{
		const VID& c_VID = it->first;
		++it;

		LPCHARACTER pAttacker = CHARACTER_MANAGER::instance().Find(c_VID);

		if (!pAttacker)
			continue;

		if (pAttacker->IsAffectFlag(AFF_EUNHYUNG) ||
			pAttacker->IsAffectFlag(AFF_INVISIBILITY) ||
			pAttacker->IsAffectFlag(AFF_REVIVE_INVISIBLE))
			continue;

		float fDist = DISTANCE_APPROX(pAttacker->GetX() - pkChr->GetX(), pAttacker->GetY() - pkChr->GetY());

		if (fDist < fMinDist)
		{
			pkVictim = pAttacker;
			fMinDist = fDist;
		}
	}

	return pkVictim;
}

void CHARACTER::SetVictim(LPCHARACTER pkVictim)
{
	if (!pkVictim)
	{
		m_kVIDVictim.Reset();
		battle_end(this);
	}
	else
	{
		m_kVIDVictim = pkVictim->GetVID();
		m_dwLastVictimSetTime = get_dword_time();
	}
}

LPCHARACTER CHARACTER::GetVictim() const
{
	return CHARACTER_MANAGER::instance().Find(m_kVIDVictim);
}

LPCHARACTER CHARACTER::GetProtege() const
{
	if (m_pkChrStone)
		return m_pkChrStone;

	if (m_pkParty)
		return m_pkParty->GetLeader();

	return NULL;
}

int CHARACTER::GetAlignment() const
{
	return m_iAlignment;
}

int CHARACTER::GetRealAlignment() const
{
	return m_iRealAlignment;
}

void CHARACTER::ShowAlignment(bool bShow)
{
	if (bShow)
	{
		if (m_iAlignment != m_iRealAlignment)
		{
			m_iAlignment = m_iRealAlignment;
			UpdatePacket();
		}
	}
	else
	{
		if (m_iAlignment != 0)
		{
			m_iAlignment = 0;
			UpdatePacket();
		}
	}
}

void CHARACTER::UpdateAlignment(int iAmount)
{
	bool bShow = false;

	if (m_iAlignment == m_iRealAlignment)
		bShow = true;

	int i = m_iAlignment / 10;

#ifdef ENABLE_ALIGNMENT_SYSTEM
	m_iRealAlignment = MINMAX(-200000, m_iRealAlignment + iAmount, g_maxAlignment * 10);
#ifndef DISABLE_ALIGNMENT_SYSTEM
	if (CAlignmentProto::instance().IsAlignmentUpdate(m_iRealAlignment, m_AlignmentLevel))
	{
		AlignmentLevel(m_iRealAlignment,true);
	}
#endif
#else
	m_iRealAlignment = MINMAX(-200000, m_iRealAlignment + iAmount, 200000);
#endif

	if (bShow)
	{
		m_iAlignment = m_iRealAlignment;

		if (i != m_iAlignment / 10)
			UpdatePacket();
	}
}

void CHARACTER::SetKillerMode(bool isOn)
{
	if ((isOn ? ADD_CHARACTER_STATE_KILLER : 0) == IS_SET(m_bAddChrState, ADD_CHARACTER_STATE_KILLER))
		return;

	if (isOn)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);
	else
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);

	m_iKillerModePulse = thecore_pulse();
	UpdatePacket();
}

bool CHARACTER::IsKillerMode() const
{
	return IS_SET(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);
}

void CHARACTER::UpdateKillerMode()
{
	if (!IsKillerMode())
		return;

	if (thecore_pulse() - m_iKillerModePulse >= PASSES_PER_SEC(30))
		SetKillerMode(false);
}

void CHARACTER::SetPKMode(BYTE bPKMode)
{
	if (bPKMode >= PK_MODE_MAX_NUM)
		return;

	if (m_bPKMode == bPKMode)
		return;

	if (bPKMode == PK_MODE_GUILD && !GetGuild())
		bPKMode = PK_MODE_FREE;

	m_bPKMode = bPKMode;
	UpdatePacket();
}

BYTE CHARACTER::GetPKMode() const
{
	return m_bPKMode;
}

struct FuncForgetMyAttacker
{
	LPCHARACTER m_ch;
	FuncForgetMyAttacker(LPCHARACTER ch)
	{
		m_ch = ch;
	}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			if (ch->IsPC())
				return;
			if (ch->m_kVIDVictim == m_ch->GetVID())
				ch->SetVictim(NULL);
		}
	}
};

struct FuncAggregateMonster
{
	LPCHARACTER m_ch;
	FuncAggregateMonster(LPCHARACTER ch)
	{
		m_ch = ch;
	}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			if (!ch)
				return;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			if (ch->GetVictim())
				return;

			unsigned short summonDistance{ 10000 };

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
			if (m_ch->IsPremium())
			{
				summonDistance = 13000;
			}	
#endif

			if (number(1, 100) <= 50)
			{
				if (DISTANCE_APPROX(ch->GetX() - m_ch->GetX(), ch->GetY() - m_ch->GetY()) < summonDistance)//@Lightwork#18
				{
					if (ch->CanBeginFight())
					{
						ch->BeginFight(m_ch);
					}
				}
			}
		}
	}
};

struct FuncAttractRanger
{
	LPCHARACTER m_ch;
	FuncAttractRanger(LPCHARACTER ch)
	{
		m_ch = ch;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			if (ch->GetVictim() && ch->GetVictim() != m_ch)
				return;
			if (ch->GetMobAttackRange() > 150)
			{
				int iNewRange = 150;//(int)(ch->GetMobAttackRange() * 0.2);
				if (iNewRange < 150)
					iNewRange = 150;

				ch->AddAffect(AFFECT_BOW_DISTANCE, POINT_BOW_DISTANCE, iNewRange - ch->GetMobAttackRange(), AFF_NONE, 3 * 60, 0, false);
			}
		}
	}
};

struct FuncPullMonster
{
	LPCHARACTER m_ch;
	int m_iLength;
	FuncPullMonster(LPCHARACTER ch, int iLength = 300)
	{
		m_ch = ch;
		m_iLength = iLength;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			//if (ch->GetVictim() && ch->GetVictim() != m_ch)
			//return;
			float fDist = DISTANCE_APPROX(m_ch->GetX() - ch->GetX(), m_ch->GetY() - ch->GetY());
			if (fDist > 3000 || fDist < 100)
				return;

			float fNewDist = fDist - m_iLength;
			if (fNewDist < 100)
				fNewDist = 100;

			float degree = GetDegreeFromPositionXY(ch->GetX(), ch->GetY(), m_ch->GetX(), m_ch->GetY());
			float fx;
			float fy;

			GetDeltaByDegree(degree, fDist - fNewDist, &fx, &fy);
			long tx = (long)(ch->GetX() + fx);
			long ty = (long)(ch->GetY() + fy);

			ch->Sync(tx, ty);
			ch->Goto(tx, ty);
			ch->CalculateMoveDuration();

			ch->SyncPacket();
		}
	}
};

void CHARACTER::ForgetMyAttacker()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncForgetMyAttacker f(this);
		pSec->ForEachAround(f);
	}
	ReviveInvisible(5);
}

void CHARACTER::AggregateMonster()
{
	LPSECTREE pSec = GetSectree();

	if (pSec)
	{
		FuncAggregateMonster f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::AttractRanger()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncAttractRanger f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::PullMonster()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncPullMonster f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::UpdateAggrPointEx(LPCHARACTER pAttacker, EDamageType type, DAM_LL dam, CHARACTER::TBattleInfo& info)
{
	switch (type)
	{
	case DAMAGE_TYPE_NORMAL_RANGE:
		dam = (DAM_LL)(dam * 1.2f);
		break;

	case DAMAGE_TYPE_RANGE:
		dam = (DAM_LL)(dam * 1.5f);
		break;

	case DAMAGE_TYPE_MAGIC:
		dam = (DAM_LL)(dam * 1.2f);
		break;

	default:
		break;
	}

	if (pAttacker == GetVictim())
		dam = (DAM_LL)(dam * 1.2f);

	info.iAggro += dam;

	if (info.iAggro < 0)
		info.iAggro = 0;

	if (GetParty() && dam > 0 && type != DAMAGE_TYPE_SPECIAL)
	{
		LPPARTY pParty = GetParty();

		DAM_LL iPartyAggroDist = dam;

		if (pParty->GetLeaderPID() == GetVID())
			iPartyAggroDist /= 2;
		else
			iPartyAggroDist /= 3;

		pParty->SendMessage(this, PM_AGGRO_INCREASE, iPartyAggroDist, pAttacker->GetVID());
	}

	if (type != DAMAGE_TYPE_POISON)//@Lightwork#66
		ChangeVictimByAggro(info.iAggro, pAttacker);
}

void CHARACTER::UpdateAggrPoint(LPCHARACTER pAttacker, EDamageType type, DAM_LL dam)
{
	if (IsDead() || IsStun())
		return;

	TDamageMap::iterator it = m_map_kDamage.find(pAttacker->GetVID());

	if (it == m_map_kDamage.end())
	{
		m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(0, dam)));
		it = m_map_kDamage.find(pAttacker->GetVID());
	}

	UpdateAggrPointEx(pAttacker, type, dam, it->second);
}

void CHARACTER::ChangeVictimByAggro(DAM_LL iNewAggro, LPCHARACTER pNewVictim)
{
	if (get_dword_time() - m_dwLastVictimSetTime < 3000)
		return;

	if (pNewVictim == GetVictim())
	{
		if (m_iMaxAggro < iNewAggro)
		{
			m_iMaxAggro = iNewAggro;
			return;
		}

		TDamageMap::iterator it;
		TDamageMap::iterator itFind = m_map_kDamage.end();

		for (it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
		{
			if (it->second.iAggro > iNewAggro)
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(it->first);

				if (ch && !ch->IsDead() && DISTANCE_APPROX(ch->GetX() - GetX(), ch->GetY() - GetY()) < 5000)
				{
					itFind = it;
					iNewAggro = it->second.iAggro;
				}
			}
		}

		if (itFind != m_map_kDamage.end())
		{
			m_iMaxAggro = iNewAggro;
			SetVictim(CHARACTER_MANAGER::instance().Find(itFind->first));
			m_dwStateDuration = 1;
		}
	}
	else
	{
		if (m_iMaxAggro < iNewAggro)
		{
			m_iMaxAggro = iNewAggro;
			SetVictim(pNewVictim);
			m_dwStateDuration = 1;
		}
	}
}

#ifdef ENABLE_LMW_PROTECTION
void CHARACTER::SetAttackCount(DWORD victimVID)
{
	std::map<DWORD, BYTE>::iterator it = m_mapAttackCount.find(victimVID);
	if (it == m_mapAttackCount.end())
	{
		m_mapAttackCount.insert(std::make_pair(victimVID, 1));
	}
	else
	{
		it->second++;
	}

	if (!m_attackHackControl)
	{
		AttackHackControlEventStart();
	}
}

bool CHARACTER::GetAttackHack()
{
	if (m_mapAttackCount.empty())
	{
		return true;
	}

	if (m_mapAttackCount.size() > 4)
	{
		HackLogAdd();
		return true;
	}

	for (std::map<DWORD, BYTE>::iterator it = m_mapAttackCount.begin(); it != m_mapAttackCount.end(); ++it)
	{
		if (it->second > m_maxAttackCount)
		{
			HackLogAdd();
			return true;
		}
	}

	m_mapAttackCount.clear();
	return false;
}

void CHARACTER::CalculateAttackCount()
{
	short attSpeed = GetPoint(POINT_ATT_SPEED);
	BYTE attCount = 0;
	
	switch (GetJob())
	{
		case JOB_ASSASSIN:
		{
			if (IsRiding())
			{
				attCount = (attSpeed / 50) + 1;
				if (attSpeed % 50 > 20)
				{
					attCount++;
				}
			}
			else
			{
				attCount = (attSpeed / 50) + 1;
				if (attSpeed % 50 > 20)
				{
					attCount++;
				}

				LPITEM weapon = GetWear(WEAR_WEAPON);
				if (weapon && weapon->GetSubType() == WEAPON_DAGGER)
				{
					attCount = (attSpeed / 35) + 1;
					if (attSpeed % 35 > 15)
					{
						attCount++;
					}
				}
			}
			break;
		}
		case JOB_SHAMAN:
		{
			if (IsRiding())
			{
				attCount = (attSpeed / 50) + 1;
				if (attSpeed % 50 > 20)
				{
					attCount++;
				}
			}
			else
			{
				attCount = (attSpeed / 40) + 1;
				if (attSpeed % 40 > 20)
				{
					attCount++;
				}
			}
			break;
		}
		default://Warrior Sura Wolfman
		{
			attCount = (attSpeed / 50) + 1;
			if (attSpeed % 50 > 20)
			{
				attCount++;
			}
			break;
		}
		
	}
	m_maxAttackCount = attCount;
}
#endif

#ifdef ENABLE_POWER_RANKING
void CHARACTER::CalcutePowerRank()
{
	DWORD powerPoint{0};
	for (const std::pair<BYTE, BYTE>x : powerPointList)
	{
		powerPoint += GetPoint(x.first) * x.second;
	}
	PointChange(POINT_POWER_RANK, powerPoint);
}
#endif