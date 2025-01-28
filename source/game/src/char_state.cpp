#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "battle.h"
#include "char_manager.h"
#include "packet.h"
#include "motion.h"
#include "party.h"
#include "affect.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "p2p.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "exchange.h"
#include "sectree_manager.h"
#include "guild_manager.h"
#include "war_map.h"
#include "locale_service.h"

#include "../../common/VnumHelper.h"

extern LPCHARACTER FindVictim(LPCHARACTER pkChr, int iMaxDistance);

namespace
{
	class FuncFindChrForFlag
	{
	public:
		FuncFindChrForFlag(LPCHARACTER pkChr) :
			m_pkChr(pkChr), m_pkChrFind(NULL), m_iMinDistance(INT_MAX)
		{
		}

		void operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return;

			if (ent->IsObserverMode())
				return;

			LPCHARACTER pkChr = (LPCHARACTER)ent;

			if (!pkChr->IsPC())
				return;

			if (!pkChr->GetGuild())
				return;

			if (pkChr->IsDead())
				return;

			int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkChr->GetX(), pkChr->GetY() - m_pkChr->GetY());

			if (iDist <= 500 && m_iMinDistance > iDist &&
				!pkChr->IsAffectFlag(AFF_WAR_FLAG1) &&
				!pkChr->IsAffectFlag(AFF_WAR_FLAG2) &&
				!pkChr->IsAffectFlag(AFF_WAR_FLAG3))
			{
				if ((DWORD)m_pkChr->GetPoint(POINT_STAT) == pkChr->GetGuild()->GetID())
				{
					CWarMap* pMap = pkChr->GetWarMap();
					BYTE idx;

					if (!pMap || !pMap->GetTeamIndex(pkChr->GetGuild()->GetID(), idx))
						return;

					if (!pMap->IsFlagOnBase(idx))
					{
						m_pkChrFind = pkChr;
						m_iMinDistance = iDist;
					}
				}
				else
				{
					m_pkChrFind = pkChr;
					m_iMinDistance = iDist;
				}
			}
		}

		LPCHARACTER	m_pkChr;
		LPCHARACTER m_pkChrFind;
		int		m_iMinDistance;
	};

	class FuncFindChrForFlagBase
	{
	public:
		FuncFindChrForFlagBase(LPCHARACTER pkChr) : m_pkChr(pkChr)
		{
		}

		void operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return;

			if (ent->IsObserverMode())
				return;

			LPCHARACTER pkChr = (LPCHARACTER)ent;

			if (!pkChr->IsPC())
				return;

			CGuild* pkGuild = pkChr->GetGuild();

			if (!pkGuild)
				return;

			int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkChr->GetX(), pkChr->GetY() - m_pkChr->GetY());

			if (iDist <= 500 &&
				(pkChr->IsAffectFlag(AFF_WAR_FLAG1) ||
					pkChr->IsAffectFlag(AFF_WAR_FLAG2) ||
					pkChr->IsAffectFlag(AFF_WAR_FLAG3)))
			{
				CAffect* pkAff = pkChr->FindAffect(AFFECT_WAR_FLAG);
				if (pkAff)
				{
					if ((DWORD)m_pkChr->GetPoint(POINT_STAT) == pkGuild->GetID() &&
						m_pkChr->GetPoint(POINT_STAT) != pkAff->lApplyValue)
					{
						CWarMap* pMap = pkChr->GetWarMap();
						BYTE idx;

						if (!pMap || !pMap->GetTeamIndex(pkGuild->GetID(), idx))
							return;
						{
							SendGuildWarScore(m_pkChr->GetPoint(POINT_STAT), pkAff->lApplyValue, 1);
							pMap->ResetFlag();
						}
					}
				}
			}
		}

		LPCHARACTER m_pkChr;
	};

	class FuncFindGuardVictim
	{
	public:
		FuncFindGuardVictim(LPCHARACTER pkChr, int iMaxDistance) :
			m_pkChr(pkChr),
			m_iMinDistance(INT_MAX),
			m_iMaxDistance(iMaxDistance),
			m_lx(pkChr->GetX()),
			m_ly(pkChr->GetY()),
			m_pkChrVictim(NULL)
		{
		};

		void operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return;

			LPCHARACTER pkChr = (LPCHARACTER)ent;

			if (pkChr->IsPC())
				return;

			if (pkChr->IsNPC() && !pkChr->IsMonster())
				return;

			if (pkChr->IsDead())
				return;

			if (pkChr->IsAffectFlag(AFF_EUNHYUNG) ||
				pkChr->IsAffectFlag(AFF_INVISIBILITY) ||
				pkChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
				return;

			int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

			if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
			{
				m_pkChrVictim = pkChr;
				m_iMinDistance = iDistance;
			}
		}

		LPCHARACTER GetVictim()
		{
			return (m_pkChrVictim);
		}

	private:
		LPCHARACTER	m_pkChr;

		int		m_iMinDistance;
		int		m_iMaxDistance;
		long	m_lx;
		long	m_ly;

		LPCHARACTER	m_pkChrVictim;
	};
}

bool CHARACTER::IsAggressive() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE);
}

void CHARACTER::SetAggressive()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE);
}

bool CHARACTER::IsCoward() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_COWARD);
}

void CHARACTER::SetCoward()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_COWARD);
}

bool CHARACTER::IsBerserker() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_BERSERK);
}

bool CHARACTER::IsStoneSkinner() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_STONESKIN);
}

bool CHARACTER::IsGodSpeeder() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_GODSPEED);
}

bool CHARACTER::IsDeathBlower() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_DEATHBLOW);
}

bool CHARACTER::IsReviver() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_REVIVE);
}

void CHARACTER::CowardEscape()
{
	int iDist[4] = { 500, 1000, 3000, 5000 };

	for (int iDistIdx = 2; iDistIdx >= 0; --iDistIdx)
		for (int iTryCount = 0; iTryCount < 8; ++iTryCount)
		{
			SetRotation(number(0, 359));

			float fx, fy;
			float fDist = number(iDist[iDistIdx], iDist[iDistIdx + 1]);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			bool bIsWayBlocked = false;
			for (int j = 1; j <= 100; ++j)
			{
				if (!SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx * j / 100, GetY() + (int)fy * j / 100))
				{
					bIsWayBlocked = true;
					break;
				}
			}

			if (bIsWayBlocked)
				continue;

			m_dwStateDuration = PASSES_PER_SEC(1);

			int iDestX = GetX() + (int)fx;
			int iDestY = GetY() + (int)fy;

			if (Goto(iDestX, iDestY))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
			return;
		}
}

void  CHARACTER::SetNoAttackShinsu()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKSHINSU);
}
bool CHARACTER::IsNoAttackShinsu() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKSHINSU);
}

void CHARACTER::SetNoAttackChunjo()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKCHUNJO);
}

bool CHARACTER::IsNoAttackChunjo() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKCHUNJO);
}

void CHARACTER::SetNoAttackJinno()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKJINNO);
}

bool CHARACTER::IsNoAttackJinno() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKJINNO);
}

void CHARACTER::SetAttackMob()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_ATTACKMOB);
}

bool CHARACTER::IsAttackMob() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_ATTACKMOB);
}

// STATE_IDLE_REFACTORING
void CHARACTER::StateIdle()
{
	if (IsStone())
	{
		__StateIdle_Stone();
		return;
	}
	else if (IsWarp() || IsGoto())
	{
		m_dwStateDuration = 60 * passes_per_sec;
		return;
	}

	if (IsPC())
		return;

	if (!IsMonster())
	{
		__StateIdle_NPC();
		return;
	}

	__StateIdle_Monster();
}

void CHARACTER::__StateIdle_Stone()
{
	m_dwStateDuration = PASSES_PER_SEC(1);

	int iPercent = 0; // @fixme136
	if (GetMaxHP() >= 0)
		iPercent = (GetHP() * 100) / GetMaxHP();
	DWORD dwVnum = number(MIN(GetMobTable().sAttackSpeed, GetMobTable().sMovingSpeed), MAX(GetMobTable().sAttackSpeed, GetMobTable().sMovingSpeed));

	if (iPercent <= 10 && GetMaxSP() < 10)
	{
		SetMaxSP(10);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 20 && GetMaxSP() < 9)
	{
		SetMaxSP(9);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 30 && GetMaxSP() < 8)
	{
		SetMaxSP(8);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 40 && GetMaxSP() < 7)
	{
		SetMaxSP(7);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 50 && GetMaxSP() < 6)
	{
		SetMaxSP(6);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 60 && GetMaxSP() < 5)
	{
		SetMaxSP(5);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 70 && GetMaxSP() < 4)
	{
		SetMaxSP(4);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 80 && GetMaxSP() < 3)
	{
		SetMaxSP(3);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 90 && GetMaxSP() < 2)
	{
		SetMaxSP(2);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 99 && GetMaxSP() < 1)
	{
		SetMaxSP(1);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else
		return;

	UpdatePacket();
	return;
}

void CHARACTER::__StateIdle_NPC()
{
	// MonsterChat(MONSTER_CHAT_WAIT);
	m_dwStateDuration = PASSES_PER_SEC(5);


	if (IsPet()
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		|| IsMount()
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		|| IsNewPet()
#endif
#ifdef ENABLE_BUFFI_SYSTEM
		|| IsBuffi()
#endif
		)
		return;

	else if (IsGuardNPC())
	{
		FuncFindGuardVictim f(this, 50000);

		if (GetSectree())
			GetSectree()->ForEachAround(f);

		LPCHARACTER victim = f.GetVictim();

		if (victim)
		{
			m_dwStateDuration = passes_per_sec / 2;

			if (CanBeginFight())
				BeginFight(victim);
		}
	}
	else
	{
		if (!IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		{
			LPCHARACTER pkChrProtege = GetProtege();

			if (pkChrProtege)
			{
				if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 500)
				{
					if (Follow(pkChrProtege, number(100, 300)))
						return;
				}
			}

			if (!number(0, 6))
			{
				SetRotation(number(0, 359));

				float fx, fy;
				float fDist = number(200, 400);

				GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

				if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx, GetY() + (int)fy)
					&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx / 2, GetY() + (int)fy / 2)))
					return;

				SetNowWalking(true);

				if (Goto(GetX() + (int)fx, GetY() + (int)fy))
					SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

				return;
			}
		}
	}
}

void CHARACTER::__StateIdle_Monster()
{
	if (IsStun())
		return;

	if (!CanMove())
		return;

	if (IsCoward())
	{
		if (!IsDead())
			CowardEscape();

		return;
	}

	if (IsBerserker())
		if (IsBerserk())
			SetBerserk(false);

	if (IsGodSpeeder())
		if (IsGodSpeed())
			SetGodSpeed(false);

	LPCHARACTER victim = GetVictim();

	if (!victim || victim->IsDead())
	{
		SetVictim(NULL);
		victim = NULL;
		m_dwStateDuration = PASSES_PER_SEC(1);
	}

	if (!victim || victim->IsBuilding())
	{
		if (m_pkChrStone)
		{
			victim = m_pkChrStone->GetNearestVictim(m_pkChrStone);
		}

		else if (IsAggressive())
		{
			victim = FindVictim(this, m_pkMobData->m_table.wAggressiveSight);
		}
	}

	if (victim && !victim->IsDead())
	{
		if (CanBeginFight())
			BeginFight(victim);

		return;
	}

	if (IsAggressive() && !victim)
		m_dwStateDuration = PASSES_PER_SEC(number(1, 3));
	else
		m_dwStateDuration = PASSES_PER_SEC(number(3, 5));

	LPCHARACTER pkChrProtege = GetProtege();

	if (pkChrProtege)
	{
		if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
		{
			if (Follow(pkChrProtege, number(150, 400)))
			{
				return;
			}
		}
	}

	if (!IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (!number(0, 6))
		{
			SetRotation(number(0, 359));

			float fx, fy;
			float fDist = number(300, 700);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx, GetY() + (int)fy)
				&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx / 2, GetY() + (int)fy / 2)))
				return;

			if (Goto(GetX() + (int)fx, GetY() + (int)fy))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			return;
		}
	}

	// MonsterChat(MONSTER_CHAT_WAIT);
}
// END_OF_STATE_IDLE_REFACTORING

bool __CHARACTER_GotoNearTarget(LPCHARACTER self, LPCHARACTER victim)
{
	if (IS_SET(self->GetAIFlag(), AIFLAG_NOMOVE))
		return false;

	switch (self->GetMobBattleType())
	{
	case BATTLE_TYPE_RANGE:
	case BATTLE_TYPE_MAGIC:

		if (self->Follow(victim, self->GetMobAttackRange() * 8 / 10))
			return true;
		break;

	default:

		if (self->Follow(victim, self->GetMobAttackRange() * 9 / 10))
			return true;
	}

	return false;
}

void CHARACTER::StateMove()
{
	DWORD dwElapsedTime = get_dword_time() - m_dwMoveStartTime;
	float fRate = (float)dwElapsedTime / (float)m_dwMoveDuration;

	if (fRate > 1.0f)
		fRate = 1.0f;

	int x = (int)((float)(m_posDest.x - m_posStart.x) * fRate + m_posStart.x);
	int y = (int)((float)(m_posDest.y - m_posStart.y) * fRate + m_posStart.y);

	Move(x, y);

	if (IsPC() && (thecore_pulse() & 15) == 0)
	{
		UpdateSectree();

		if (GetExchange())
		{
			LPCHARACTER victim = GetExchange()->GetCompany()->GetOwner();
			int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

			if (iDist >= EXCHANGE_MAX_DISTANCE)
			{
				GetExchange()->Cancel();
			}
		}
	}

	if (IsPC())
	{
		if (IsWalking() && GetStamina() < GetMaxStamina())
		{
			if (get_dword_time() - GetWalkStartTime() > 5000)
				PointChange(POINT_STAMINA, GetMaxStamina() / 1);
		}

		if (!IsWalking() && !IsRiding()) {
			if ((get_dword_time() - GetLastAttackTime()) < 20000)
			{
				StartAffectEvent();

				if (IsStaminaHalfConsume())
				{
					if (thecore_pulse() & 1)
						PointChange(POINT_STAMINA, -STAMINA_PER_STEP);
				}
				else
					PointChange(POINT_STAMINA, -STAMINA_PER_STEP);

				StartStaminaConsume();

				if (GetStamina() <= 0)
				{
					SetStamina(0);
					SetNowWalking(true);
					StopStaminaConsume();
				}
			}
			else if (IsStaminaConsume())
			{
				StopStaminaConsume();
			}
		}
	}
	else
	{
		// XXX AGGRO
		if (IsMonster() && GetVictim())
		{
			LPCHARACTER victim = GetVictim();
			UpdateAggrPoint(victim, DAMAGE_TYPE_NORMAL, -(victim->GetLevel() / 3 + 1));
		}

		if (IsMonster() && GetMobRank() >= MOB_RANK_BOSS && GetVictim())
		{
			LPCHARACTER victim = GetVictim();

			if (GetRaceNum() == 2191 && number(1, 20) == 1 && get_dword_time() - m_pkMobInst->m_dwLastWarpTime > 1000)
			{
				float fx, fy;
				GetDeltaByDegree(victim->GetRotation(), 400, &fx, &fy);
				long new_x = victim->GetX() + (long)fx;
				long new_y = victim->GetY() + (long)fy;
				SetRotation(GetDegreeFromPositionXY(new_x, new_y, victim->GetX(), victim->GetY()));
				Show(victim->GetMapIndex(), new_x, new_y, 0, true);
				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
				ResetMobSkillCooltime();
				m_pkMobInst->m_dwLastWarpTime = get_dword_time();
				return;
			}

			if (number(0, 3) == 0)
			{
				if (__CHARACTER_GotoNearTarget(this, victim))
					return;
			}
		}
	}

	if (1.0f == fRate)
	{
		if (IsPC())
		{
			GotoState(m_stateIdle);
			StopStaminaConsume();
		}
		else
		{
			if (GetVictim() && !IsCoward())
			{
				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
			}
			else
			{
				GotoState(m_stateIdle);
				m_dwStateDuration = PASSES_PER_SEC(number(1, 3));
			}
		}
	}
}

void CHARACTER::StateBattle()
{
	if (IsStone())
	{
		sys_err("Stone must not use battle state (name %s)", GetName());
		return;
	}

	if (IsPC())
		return;

	if (!CanMove())
		return;

	if (IsStun())
		return;

	LPCHARACTER victim = GetVictim();

	if (victim)// @fixme235
	{
		if (victim->IsPC() && victim->IsObserverMode())
		{
			return;
		}
	}

	if (IsCoward())
	{
		if (IsDead())
			return;

		SetVictim(NULL);

		if (number(1, 50) != 1)
		{
			GotoState(m_stateIdle);
			m_dwStateDuration = 1;
		}
		else
			CowardEscape();

		return;
	}

	if (!victim || (victim->IsStun() && IsGuardNPC()) || victim->IsDead())
	{
		if (victim && victim->IsDead() &&
			IsAggressive() && (!GetParty() || GetParty()->GetLeader() == this))
		{
			LPCHARACTER new_victim = FindVictim(this, m_pkMobData->m_table.wAggressiveSight);

			SetVictim(new_victim);
			m_dwStateDuration = PASSES_PER_SEC(1);

			if (!new_victim)
			{
				switch (GetMobBattleType())
				{
				case BATTLE_TYPE_MELEE:
				case BATTLE_TYPE_SUPER_POWER:
				case BATTLE_TYPE_SUPER_TANKER:
				case BATTLE_TYPE_POWER:
				case BATTLE_TYPE_TANKER:
				{
					float fx, fy;
					float fDist = number(400, 1500);

					GetDeltaByDegree(number(0, 359), fDist, &fx, &fy);

					if (SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(),
						victim->GetX() + (int)fx,
						victim->GetY() + (int)fy) &&
						SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(),
							victim->GetX() + (int)fx / 2,
							victim->GetY() + (int)fy / 2))
					{
						float dx = victim->GetX() + fx;
						float dy = victim->GetY() + fy;

						SetRotation(GetDegreeFromPosition(dx, dy));

						if (Goto((long)dx, (long)dy))
						{
							SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
						}
					}
				}
				}
			}
			return;
		}

		SetVictim(NULL);

		if (IsGuardNPC())
			Return();

		m_dwStateDuration = PASSES_PER_SEC(1);
		return;
	}

	if (IsSummonMonster() && !IsDead() && !IsStun())
	{
		if (!GetParty())
		{
			CPartyManager::instance().CreateParty(this);
		}

		LPPARTY pParty = GetParty();
		bool bPct = !number(0, 3);

		if (bPct && pParty->CountMemberByVnum(GetSummonVnum()) < SUMMON_MONSTER_COUNT)
		{
			int sx = GetX() - 300;
			int sy = GetY() - 300;
			int ex = GetX() + 300;
			int ey = GetY() + 300;

			LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(GetSummonVnum(), GetMapIndex(), sx, sy, ex, ey, true, true);

			if (tch)
			{
				pParty->Join(tch->GetVID());
				pParty->Link(tch);
			}
		}
	}

	LPCHARACTER pkChrProtege = GetProtege();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= 4000.0f)
	{
		SetVictim(NULL);

		if (pkChrProtege)
			if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
				Follow(pkChrProtege, number(150, 400));

		return;
	}

	if (fDist >= GetMobAttackRange() * 1.15)
	{
		__CHARACTER_GotoNearTarget(this, victim);
		return;
	}

	if (m_pkParty)
		m_pkParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);


	DWORD dwCurTime = get_dword_time();
	DWORD dwDuration = CalculateDuration(GetLimitPoint(POINT_ATT_SPEED), 2000);

	if ((dwCurTime - m_dwLastAttackTime) < dwDuration)
	{
		m_dwStateDuration = MAX(1, (passes_per_sec * (dwDuration - (dwCurTime - m_dwLastAttackTime)) / 1000));
		return;
	}

	if (IsBerserker() == true)
		if (GetHPPct() < m_pkMobData->m_table.bBerserkPoint)
			if (IsBerserk() != true)
				SetBerserk(true);

	if (IsGodSpeeder() == true)
		if (GetHPPct() < m_pkMobData->m_table.bGodSpeedPoint)
			if (IsGodSpeed() != true)
				SetGodSpeed(true);

	if (HasMobSkill())
	{
		for (unsigned int iSkillIdx = 0; iSkillIdx < MOB_SKILL_MAX_NUM; ++iSkillIdx)
		{
			if (CanUseMobSkill(iSkillIdx))
			{
				SetRotationToXY(victim->GetX(), victim->GetY());

				if (UseMobSkill(iSkillIdx))
				{
					SendMovePacket(FUNC_MOB_SKILL, iSkillIdx, GetX(), GetY(), 0, dwCurTime);

					float fDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_SPECIAL_1 + iSkillIdx));
					m_dwStateDuration = (DWORD)(fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));
					return;
				}
			}
		}
	}

	if (!Attack(victim))
		m_dwStateDuration = passes_per_sec / 2;
	else
	{
		SetRotationToXY(victim->GetX(), victim->GetY());

		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0, dwCurTime);

		float fDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK));
		m_dwStateDuration = (DWORD)(fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));
	}
}

void CHARACTER::StateFlag()
{
	m_dwStateDuration = (DWORD)PASSES_PER_SEC(0.5);

	CWarMap* pMap = GetWarMap();

	if (!pMap)
		return;

	FuncFindChrForFlag f(this);
	GetSectree()->ForEachAround(f);

	if (!f.m_pkChrFind)
		return;

	if (NULL == f.m_pkChrFind->GetGuild())
		return;

	char buf[256];
	BYTE idx;

	if (!pMap->GetTeamIndex(GetPoint(POINT_STAT), idx))
		return;

	f.m_pkChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_NONE, GetPoint(POINT_STAT), idx == 0 ? AFF_WAR_FLAG1 : AFF_WAR_FLAG2, INFINITE_AFFECT_DURATION, 0, false);
	f.m_pkChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_MOV_SPEED, 50 - f.m_pkChrFind->GetPoint(POINT_MOV_SPEED), 0, INFINITE_AFFECT_DURATION, 0, false);

	pMap->RemoveFlag(idx);

	snprintf(buf, sizeof(buf), LC_TEXT("CMD_TALK_3 %s %s"), pMap->GetGuild(idx)->GetName(), f.m_pkChrFind->GetName());
	pMap->Notice(buf);
}

void CHARACTER::StateFlagBase()
{
	m_dwStateDuration = (DWORD)PASSES_PER_SEC(0.5);

	FuncFindChrForFlagBase f(this);
	GetSectree()->ForEachAround(f);
}

void CHARACTER::StateHorse()
{
	float	START_FOLLOW_DISTANCE = 400.0f;
	float	START_RUN_DISTANCE = 700.0f;
	int		MIN_APPROACH = 150;
	int		MAX_APPROACH = 300;

	DWORD	STATE_DURATION = (DWORD)PASSES_PER_SEC(0.5);

	bool bDoMoveAlone = true;
	bool bRun = true;

	if (IsDead())
		return;

	m_dwStateDuration = STATE_DURATION;

	LPCHARACTER victim = GetRider();

	if (!victim)
	{
		M2_DESTROY_CHARACTER(this);
		return;
	}

	m_pkMobInst->m_posLastAttacked = GetXYZ();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if (fDist > START_RUN_DISTANCE)
			SetNowWalking(!bRun);

		Follow(victim, number(MIN_APPROACH, MAX_APPROACH));

		m_dwStateDuration = STATE_DURATION;
	}
	else if (bDoMoveAlone && (get_dword_time() > m_dwLastAttackTime))
	{
		// wondering-.-
		m_dwLastAttackTime = get_dword_time() + number(5000, 12000);

		SetRotation(number(0, 359));

		float fx, fy;
		float fDist = number(200, 400);

		GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

		if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx, GetY() + (int)fy)
			&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int)fx / 2, GetY() + (int)fy / 2)))
			return;

		SetNowWalking(true);

		if (Goto(GetX() + (int)fx, GetY() + (int)fy))
			SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}
}