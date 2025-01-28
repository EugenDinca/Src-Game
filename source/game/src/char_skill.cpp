#include "stdafx.h"
#include <sstream>

#include "utils.h"
#include "config.h"
#include "vector.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "desc.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "item.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "start_position.h"
#include "party.h"
#include "buffer_manager.h"
#include "guild.h"

#include "unique_item.h"
#include "questmanager.h"
#include "../../common/Controls.h"

#ifdef ENABLE_SKILL_COLOR_SYSTEM
#include "desc_client.h"
#endif

#define ENABLE_FORCE2MASTERSKILL
#define ENABLE_NO_MOUNT_CHECK
// #define ENABLE_NULLIFYAFFECT_LIMIT

static const DWORD s_adwSubSkillVnums[] =
{
	SKILL_LEADERSHIP,
	SKILL_COMBO,
	SKILL_MINING,
	SKILL_LANGUAGE1,
	SKILL_LANGUAGE2,
	SKILL_LANGUAGE3,
	SKILL_POLYMORPH,
	SKILL_HORSE,
	SKILL_HORSE_SUMMON,
	SKILL_HORSE_WILDATTACK,
	SKILL_HORSE_CHARGE,
	SKILL_HORSE_ESCAPE,
	SKILL_HORSE_WILDATTACK_RANGE,
	SKILL_ADD_HP,
	SKILL_RESIST_PENETRATE,
#ifdef ENABLE_BUFFI_SYSTEM
	SKILL_BUFFI_1,
	SKILL_BUFFI_2,
	SKILL_BUFFI_3,
	SKILL_BUFFI_4,
	SKILL_BUFFI_5,
#endif
#ifdef ENABLE_PASSIVE_SKILLS
	SKILL_PASSIVE_MONSTER,
	SKILL_PASSIVE_STONE,
	SKILL_PASSIVE_BERSERKER,
	SKILL_PASSIVE_HUMAN,
	SKILL_PASSIVE_SKILL_COOLDOWN,
	SKILL_PASSIVE_DRAGON_HEARTH,
	SKILL_PASSIVE_HERBOLOGY,
	SKILL_PASSIVE_UPGRADING,
	SKILL_PASSIVE_FISHING,
#endif
};

bool TSkillUseInfo::HitOnce(DWORD dwVnum)
{
	if (!bUsed)
		return false;

	if (dwNextSkillUsableTime && dwNextSkillUsableTime < get_dword_time() && dwVnum != SKILL_MUYEONG && dwVnum != SKILL_HORSE_WILDATTACK)
	{
		return false;
	}

	if (iHitCount == -1)
	{
		return true;
	}

	if (iHitCount)
	{
		iHitCount--;
		return true;
	}
	return false;
}

bool TSkillUseInfo::UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount, int hitcount, int range)
{
	this->isGrandMaster = isGrandMaster;
	DWORD dwCur = get_dword_time();

	if (bUsed && dwNextSkillUsableTime > dwCur)
	{
		iHitCount = 0;
		return false;
	}

	bUsed = true;
#ifdef ENABLE_LMW_PROTECTION
	bSkillCD = true;
	dwHitCount = 0;
#endif
	if (dwCooltime)
		dwNextSkillUsableTime = dwCur + dwCooltime;
	else
		dwNextSkillUsableTime = 0;

	iRange = range;
	iMaxHitCount = iHitCount = hitcount;

	dwVID = vid;
	iSplashCount = splashcount;
	return true;
}

#ifdef ENABLE_LMW_PROTECTION
bool TSkillUseInfo::IsSkillCooldown(DWORD dwVnum, float fSkillPower)
{
	DWORD maxHitCount = 0;
	if (dwVnum == SKILL_KWANKYEOK || dwVnum == SKILL_HORSE_WILDATTACK_RANGE)
	{
		if (dwVnum == SKILL_KWANKYEOK)
			maxHitCount = static_cast<DWORD>(2 + floor(6 * fSkillPower));
		else if (dwVnum == SKILL_HORSE_WILDATTACK_RANGE)
			maxHitCount = 5;

		dwHitCount++;

		if (dwHitCount > maxHitCount)
			return true;
		return false;
	}

	if (!bSkillCD)
		return true;

	bSkillCD = false;
	return bSkillCD;
}
#endif

int CHARACTER::GetChainLightningMaxCount() const
{
	return aiChainLightningCountBySkillLevel[MIN(SKILL_MAX_LEVEL, GetSkillLevel(SKILL_CHAIN))];
}

void CHARACTER::SetAffectedEunhyung()
{
	m_dwAffectedEunhyungLevel = GetSkillPower(SKILL_EUNHYUNG);
}

void CHARACTER::SetSkillGroup(BYTE bSkillGroup)
{
	if (bSkillGroup > 2)
		return;

	if (GetLevel() < 5)
		return;

	m_points.skill_group = bSkillGroup;

	TPacketGCChangeSkillGroup p;
	p.header = HEADER_GC_SKILL_GROUP;
	p.skill_group = m_points.skill_group;

	GetDesc()->Packet(&p, sizeof(TPacketGCChangeSkillGroup));
}

int CHARACTER::ComputeCooltime(int time)
{
	return CalculateDuration(GetPoint(POINT_CASTING_SPEED), time);
}

void CHARACTER::SkillLevelPacket()
{
	if (!GetDesc())
		return;

	TPacketGCSkillLevel pack;

	pack.bHeader = HEADER_GC_SKILL_LEVEL;
	thecore_memcpy(&pack.skills, m_pSkillLevels, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	GetDesc()->Packet(&pack, sizeof(TPacketGCSkillLevel));
}

void CHARACTER::SetSkillLevel(DWORD dwVnum, BYTE bLev)
{
	if (NULL == m_pSkillLevels)
		return;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (vnum %u)", dwVnum);
		return;
	}

	m_pSkillLevels[dwVnum].bLevel = MIN(SKILL_MAX_LEVEL, bLev);

#ifdef ENABLE_SKILLS_LEVEL_OVER_P
	if (bLev >= 60)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_LEGENDARY_MASTER;
	else if (bLev >= 50)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_SAGE_MASTER;
	else if (bLev >= 40)
#else
	if (bLev >= 40)
#endif
		m_pSkillLevels[dwVnum].bMasterType = SKILL_PERFECT_MASTER;
	else if (bLev >= 30)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_GRAND_MASTER;
	else if (bLev >= 20)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_MASTER;
	else
		m_pSkillLevels[dwVnum].bMasterType = SKILL_NORMAL;
}

bool CHARACTER::IsLearnableSkill(DWORD dwSkillVnum) const
{
	const CSkillProto* pkSkill = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSkill)
		return false;

	if (GetSkillLevel(dwSkillVnum) >= SKILL_MAX_LEVEL)
		return false;

	if (pkSkill->dwType == 0)
	{
		if (GetSkillLevel(dwSkillVnum) >= pkSkill->bMaxLevel)
			return false;

		return true;
	}

	if (pkSkill->dwType == 5)
	{
		if (dwSkillVnum == SKILL_HORSE_WILDATTACK_RANGE && GetJob() != JOB_ASSASSIN)
			return false;

		return true;
	}

	if (GetSkillGroup() == 0)
		return false;

	if (pkSkill->dwType - 1 == GetJob())
		return true;
#ifdef ENABLE_WOLFMAN_CHARACTER

	if (7 == pkSkill->dwType && JOB_WOLFMAN == GetJob())
		return true;
#endif
	if (6 == pkSkill->dwType)
	{
		if (SKILL_7_A_ANTI_TANHWAN <= dwSkillVnum && dwSkillVnum <= SKILL_7_D_ANTI_YONGBI)
		{
			for (int i = 0; i < 4; i++)
			{
				if (unsigned(SKILL_7_A_ANTI_TANHWAN + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_7_A_ANTI_TANHWAN + i))
					{
						return false;
					}
				}
			}

			return true;
		}

		if (SKILL_8_A_ANTI_GIGONGCHAM <= dwSkillVnum && dwSkillVnum <= SKILL_8_D_ANTI_BYEURAK)
		{
			for (int i = 0; i < 4; i++)
			{
				if (unsigned(SKILL_8_A_ANTI_GIGONGCHAM + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_8_A_ANTI_GIGONGCHAM + i))
						return false;
				}
			}

			return true;
		}
	}

	return false;
}

// ADD_GRANDMASTER_SKILL


static bool FN_should_check_exp(LPCHARACTER ch)
{
	// @warme005
	return ch->GetLevel() < gPlayerMaxLevel;
}

// #define ENABLE_MASTER_SKILLBOOK_NO_STEPS
bool CHARACTER::LearnSkillByBook(DWORD dwSkillVnum, BYTE bProb)
{
	const CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		NewChatPacket(SKILL_TALK_1);
		return false;
	}

	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	if (!pPC)
		return false;

	DWORD need_exp = 0;

	if (FN_should_check_exp(this))
	{
		//need_exp = 20000;
		if (GetLevel() < PLAYER_MAX_LEVEL_CONST)
		{
			need_exp = 0;
		}//@Lightwork#11

		if (GetExp() < need_exp)
		{
			NewChatPacket(SKILL_TALK_8);
			return false;
		}
	}

	if (pkSk->dwType != 0)
	{
		if (GetSkillMasterType(dwSkillVnum) != SKILL_MASTER)
		{
			if (GetSkillMasterType(dwSkillVnum) > SKILL_MASTER)
				NewChatPacket(SKILL_TALK_9);
			else
				NewChatPacket(SKILL_TALK_10);
			return false;
		}
	}

	int need_bookcount = GetSkillLevel(dwSkillVnum) - 20;

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	PointChange(POINT_EXP, -static_cast<int64_t>(need_exp));
#else
	PointChange(POINT_EXP, -need_exp);
#endif

#ifdef ENABLE_SKILL_DESCRIPTION_RENEWAL
				int read_count = m_pSkillLevels[dwSkillVnum].bBooks;
#else
				char flag[128+1];
				memset(flag, 0, sizeof(flag));
				snprintf(flag, sizeof(flag), "traning_master_skill.%u.read_count", dwSkillVnum);

				int read_count = pPC->GetFlag(flag);
#endif

	if (read_count >= need_bookcount)
	{
#ifdef ENABLE_SKILL_DESCRIPTION_RENEWAL
						m_pSkillLevels[dwSkillVnum].bBooks = 0;
						SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
#else
						SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
						pPC->DeleteFlag(flag);
#endif
		//pPC->DeleteFlag(flag);
		NewChatPacket(SKILL_TALK_6);
	}
	else
	{
#ifdef ENABLE_SKILL_DESCRIPTION_RENEWAL
						m_pSkillLevels[dwSkillVnum].bBooks += 1;
#else
						pPC->SetFlag(flag, read_count + 1);
#endif
		NewChatPacket(SKILL_TALK_12, "%d", need_bookcount - read_count);
#ifdef ENABLE_SKILL_DESCRIPTION_RENEWAL
						SkillLevelPacket();
#endif
	}

	return true;
}

bool CHARACTER::SkillLevelDown(DWORD dwVnum)
{
	if (NULL == m_pSkillLevels)
		return false;

	if (g_bSkillDisable)
		return false;

	if (IsPolymorphed())
		return false;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such skill by number %u", dwVnum);
		return false;
	}

	if (!IsLearnableSkill(dwVnum))
		return false;

	if (GetSkillMasterType(pkSk->dwVnum) != SKILL_NORMAL)
		return false;

	if (!GetSkillGroup())
		return false;

	if (pkSk->dwVnum >= SKILL_MAX_NUM)
		return false;

	if (m_pSkillLevels[pkSk->dwVnum].bLevel == 0)
		return false;

	int idx = POINT_SKILL;
	switch (pkSk->dwType)
	{
	case 0:
		idx = POINT_SUB_SKILL;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 6:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case 7:
#endif
		idx = POINT_SKILL;
		break;
	case 5:
		idx = POINT_HORSE_SKILL;
		break;
	default:
		sys_err("Wrong skill type %d skill vnum %d", pkSk->dwType, pkSk->dwVnum);
		return false;
	}

	PointChange(idx, +1);
	SetSkillLevel(pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bLevel - 1);

	Save();

	ComputePoints();
	SkillLevelPacket();
	return true;
}

void CHARACTER::SkillLevelUp(DWORD dwVnum, BYTE bMethod)
{
	if (NULL == m_pSkillLevels)
		return;

	if (g_bSkillDisable)
		return;

	if (IsPolymorphed())
	{
		return;
	}

	if (SKILL_7_A_ANTI_TANHWAN <= dwVnum && dwVnum <= SKILL_8_D_ANTI_BYEURAK)
	{
		if (0 == GetSkillLevel(dwVnum))
			return;
	}

	const CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such skill by number (vnum %u)", dwVnum);
		return;
	}

	if (pkSk->dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("Skill Vnum overflow (vnum %u)", dwVnum);
		return;
	}

	if (!IsLearnableSkill(dwVnum))
		return;

	if (pkSk->dwType != 0)
	{
		switch (GetSkillMasterType(pkSk->dwVnum))
		{
		case SKILL_GRAND_MASTER:
		{
			if (bMethod != SKILL_UP_BY_QUEST)
				return;
			break;
		}

		case SKILL_PERFECT_MASTER:
		{
#ifdef ENABLE_SKILLS_LEVEL_OVER_P
			if (bMethod != SKILL_UP_BY_QUEST)
				return;
			break;
		}
		case SKILL_SAGE_MASTER:
		{
			if (bMethod != SKILL_UP_BY_QUEST)
				return;
			break;
		}
		case SKILL_LEGENDARY_MASTER:
		{
			return; break;
		}
#else
			return; break;
		}
#endif
	}
}

	if (bMethod == SKILL_UP_BY_POINT)
	{
		if (GetSkillMasterType(pkSk->dwVnum) != SKILL_NORMAL)
			return;

		if (IS_SET(pkSk->dwFlag, SKILL_FLAG_DISABLE_BY_POINT_UP))
			return;
	}
	else if (bMethod == SKILL_UP_BY_BOOK)
	{
		if (pkSk->dwType != 0)
			if (GetSkillMasterType(pkSk->dwVnum) != SKILL_MASTER)
				return;
	}

	if (GetLevel() < pkSk->bLevelLimit)
		return;

	if (pkSk->preSkillVnum)
		if (GetSkillMasterType(pkSk->preSkillVnum) == SKILL_NORMAL &&
			GetSkillLevel(pkSk->preSkillVnum) < pkSk->preSkillLevel)
			return;

	if (!GetSkillGroup())
		return;

	if (bMethod == SKILL_UP_BY_POINT)
	{
		int idx;

		switch (pkSk->dwType)
		{
		case 0:
			idx = POINT_SUB_SKILL;
			break;

		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case 7:
#endif
			idx = POINT_SKILL;
			break;

		case 5:
			idx = POINT_HORSE_SKILL;
			break;

		default:
			sys_err("Wrong skill type %d skill vnum %d", pkSk->dwType, pkSk->dwVnum);
			return;
		}

		if (GetPoint(idx) < 1)
			return;

		PointChange(idx, -1);
	}

	int SkillPointBefore = GetSkillLevel(pkSk->dwVnum);
	SetSkillLevel(pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bLevel + 1);

	if (pkSk->dwType != 0)
	{
		switch (GetSkillMasterType(pkSk->dwVnum))
		{
		case SKILL_NORMAL:
		{

			if (GetSkillLevel(pkSk->dwVnum) >= 17)
			{
#ifdef ENABLE_FORCE2MASTERSKILL
				SetSkillLevel(pkSk->dwVnum, 20);
#else
				if (GetQuestFlag("reset_scroll.force_to_master_skill") > 0)
				{
					SetSkillLevel(pkSk->dwVnum, 20);
					SetQuestFlag("reset_scroll.force_to_master_skill", 0);
				}
				else
				{
					if (number(1, 21 - MIN(20, GetSkillLevel(pkSk->dwVnum))) == 1)
						SetSkillLevel(pkSk->dwVnum, 20);
				}
#endif
			}
			break;
		}

		case SKILL_MASTER:
		{
			if (GetSkillLevel(pkSk->dwVnum) >= 30)
			{
				if (number(1, 31 - MIN(30, GetSkillLevel(pkSk->dwVnum))) == 1)
					SetSkillLevel(pkSk->dwVnum, 30);
			}
			break;
		}
		case SKILL_GRAND_MASTER:
		{
			if (GetSkillLevel(pkSk->dwVnum) >= 40)
			{
				SetSkillLevel(pkSk->dwVnum, 40);
			}
			break;
		}
#ifdef ENABLE_SKILLS_LEVEL_OVER_P
		case SKILL_PERFECT_MASTER:
		{
			if (GetSkillLevel(pkSk->dwVnum) >= 49)
			{
				SetSkillLevel(pkSk->dwVnum, 49);
			}
			break;
		}
		case SKILL_SAGE_MASTER:
		{
			if (GetSkillLevel(pkSk->dwVnum) >= 59)
			{
				SetSkillLevel(pkSk->dwVnum, 59);
			}
			break;
		}
#endif
		}
	}

	char szSkillUp[1024];

	snprintf(szSkillUp, sizeof(szSkillUp), "SkillUp: %s %u %d %d[Before:%d] type %u",
		GetName(), pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bMasterType, m_pSkillLevels[pkSk->dwVnum].bLevel, SkillPointBefore, pkSk->dwType);

	sys_log(0, "%s", szSkillUp);

	Save();

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ResetSkill()
{
	if (NULL == m_pSkillLevels)
		return;

	std::vector<std::pair<DWORD, TPlayerSkill> > vec;
	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		vec.push_back(std::make_pair(s_adwSubSkillVnums[i], m_pSkillLevels[s_adwSubSkillVnums[i]]));
	}

	memset(m_pSkillLevels, 0, sizeof(TPlayerSkill) * SKILL_MAX_NUM);

	std::vector<std::pair<DWORD, TPlayerSkill> >::const_iterator iter = vec.begin();

	while (iter != vec.end())
	{
		const std::pair<DWORD, TPlayerSkill>& pair = *(iter++);
		m_pSkillLevels[pair.first] = pair.second;
	}

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputePassiveSkill(DWORD dwVnum)
{
	if (g_bSkillDisable)
		return;

	if (GetSkillLevel(dwVnum) == 0)
		return;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);
	pkSk->SetPointVar("k", GetSkillLevel(dwVnum));
	int iAmount = (int)pkSk->kPointPoly.Eval();
	PointChange(pkSk->bPointOn, iAmount);
}

struct FFindNearVictim
{
	FFindNearVictim(LPCHARACTER center, LPCHARACTER attacker, const CHARACTER_SET& excepts_set = empty_set_)
		: m_pkChrCenter(center),
		m_pkChrNextTarget(NULL),
		m_pkChrAttacker(attacker),
		m_count(0),
		m_excepts_set(excepts_set)
	{
	}

	void operator ()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		if (!m_excepts_set.empty()) {
			if (m_excepts_set.find(pkChr) != m_excepts_set.end())
				return;
		}

		if (m_pkChrCenter == pkChr)
			return;

		if (!battle_is_attackable(m_pkChrAttacker, pkChr))
		{
			return;
		}

		if (abs(m_pkChrCenter->GetX() - pkChr->GetX()) > 1000 || abs(m_pkChrCenter->GetY() - pkChr->GetY()) > 1000)
			return;

		float fDist = DISTANCE_APPROX(m_pkChrCenter->GetX() - pkChr->GetX(), m_pkChrCenter->GetY() - pkChr->GetY());

		if (fDist < 1000)
		{
			++m_count;

			if ((m_count == 1) || number(1, m_count) == 1)
				m_pkChrNextTarget = pkChr;
		}
	}

	LPCHARACTER GetVictim()
	{
		return m_pkChrNextTarget;
	}

	LPCHARACTER m_pkChrCenter;
	LPCHARACTER m_pkChrNextTarget;
	LPCHARACTER m_pkChrAttacker;
	int		m_count;
	const CHARACTER_SET& m_excepts_set;
private:
	static CHARACTER_SET empty_set_;
};

CHARACTER_SET FFindNearVictim::empty_set_;

EVENTINFO(chain_lightning_event_info)
{
	DWORD			dwVictim;
	DWORD			dwChr;

	chain_lightning_event_info()
		: dwVictim(0)
		, dwChr(0)
	{
	}
};

EVENTFUNC(ChainLightningEvent)
{
	chain_lightning_event_info* info = dynamic_cast<chain_lightning_event_info*>(event->info);

	LPCHARACTER pkChrVictim = CHARACTER_MANAGER::instance().Find(info->dwVictim);
	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().Find(info->dwChr);
	LPCHARACTER pkTarget = NULL;

	if (!pkChr || !pkChrVictim)
	{
		sys_log(1, "use chainlighting, but no character");
		return 0;
	}

	sys_log(1, "chainlighting event %s", pkChr->GetName());

	if (pkChrVictim->GetParty())
	{
		pkTarget = pkChrVictim->GetParty()->GetNextOwnership(NULL, pkChrVictim->GetX(), pkChrVictim->GetY());
		if (pkTarget == pkChrVictim || !number(0, 2) || pkChr->GetChainLightingExcept().find(pkTarget) != pkChr->GetChainLightingExcept().end())
			pkTarget = NULL;
	}

	if (!pkTarget)
	{
		// 1. Find Next victim
		FFindNearVictim f(pkChrVictim, pkChr, pkChr->GetChainLightingExcept());

		if (pkChrVictim->GetSectree())
		{
			pkChrVictim->GetSectree()->ForEachAround(f);
			// 2. If exist, compute it again
			pkTarget = f.GetVictim();
		}
	}

	if (pkTarget)
	{
		pkChrVictim->CreateFly(FLY_CHAIN_LIGHTNING, pkTarget);
		pkChr->ComputeSkill(SKILL_CHAIN, pkTarget);
		pkChr->AddChainLightningExcept(pkTarget);
	}
	else
	{
		sys_log(1, "%s use chainlighting, but find victim failed near %s", pkChr->GetName(), pkChrVictim->GetName());
	}

	return 0;
}

void SetPolyVarForAttack(LPCHARACTER ch, CSkillProto* pkSk, LPITEM pkWeapon)
{
	if (ch->IsPC())
	{
		if (pkWeapon && pkWeapon->GetType() == ITEM_WEAPON)
		{
			int iWep = number(pkWeapon->GetValue(3), pkWeapon->GetValue(4));
			iWep += pkWeapon->GetValue(5);

			int iMtk = number(pkWeapon->GetValue(1), pkWeapon->GetValue(2));
			iMtk += pkWeapon->GetValue(5);

			pkSk->SetPointVar("wep", iWep);
			pkSk->SetPointVar("mtk", iMtk);
			pkSk->SetPointVar("mwep", iMtk);
		}
		else
		{
			pkSk->SetPointVar("wep", 0);
			pkSk->SetPointVar("mtk", 0);
			pkSk->SetPointVar("mwep", 0);
		}
	}
	else
	{
		int iWep = number(ch->GetMobDamageMin(), ch->GetMobDamageMax());
		pkSk->SetPointVar("wep", iWep);
		pkSk->SetPointVar("mwep", iWep);
		pkSk->SetPointVar("mtk", iWep);
	}
}

struct FuncSplashDamage
{
	FuncSplashDamage(int x, int y, CSkillProto* pkSk, LPCHARACTER pkChr, int iAmount, int iAG, int iMaxHit, LPITEM pkWeapon, bool bDisableCooltime, TSkillUseInfo* pInfo, BYTE bUseSkillPower)
		:
		m_x(x), m_y(y), m_pkSk(pkSk), m_pkChr(pkChr), m_iAmount(iAmount), m_iAG(iAG), m_iCount(0), m_iMaxHit(iMaxHit), m_pkWeapon(pkWeapon), m_bDisableCooltime(bDisableCooltime), m_pInfo(pInfo), m_bUseSkillPower(bUseSkillPower)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
		{
			//if (m_pkSk->dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN target not character %s", m_pkChr->GetName());
			return;
		}

		LPCHARACTER pkChrVictim = (LPCHARACTER)ent;

		if (DISTANCE_APPROX(m_x - pkChrVictim->GetX(), m_y - pkChrVictim->GetY()) > m_pkSk->iSplashRange)
		{
			return;
		}

		if (!battle_is_attackable(m_pkChr, pkChrVictim))
		{
			return;
		}
		
#ifdef ENABLE_AFTERDEATH_SHIELD
		if(m_pkChr->IsAffectFlag(AFF_AFTERDEATH_SHIELD))
			m_pkChr->RemoveShieldAffect();
#endif

		if (m_pkChr->IsPC())

			if (!(m_pkSk->dwVnum >= GUILD_SKILL_START && m_pkSk->dwVnum <= GUILD_SKILL_END))
				if (!m_bDisableCooltime && m_pInfo && !m_pInfo->HitOnce(m_pkSk->dwVnum) && m_pkSk->dwVnum != SKILL_MUYEONG)
				{
					return;
				}

		++m_iCount;

		DAM_LL iDam;

		////////////////////////////////////////////////////////////////////////////////
		//float k = 1.0f * m_pkChr->GetSkillPower(m_pkSk->dwVnum) * m_pkSk->bMaxLevel / 100;
		//m_pkSk->kPointPoly2.SetVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
		m_pkSk->SetPointVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
		m_pkSk->SetPointVar("lv", m_pkChr->GetLevel());
		m_pkSk->SetPointVar("iq", m_pkChr->GetPoint(POINT_IQ));
		m_pkSk->SetPointVar("str", m_pkChr->GetPoint(POINT_ST));
		m_pkSk->SetPointVar("dex", m_pkChr->GetPoint(POINT_DX));
		m_pkSk->SetPointVar("con", m_pkChr->GetPoint(POINT_HT));
		m_pkSk->SetPointVar("def", m_pkChr->GetPoint(POINT_DEF_GRADE));
		m_pkSk->SetPointVar("odef", m_pkChr->GetPoint(POINT_DEF_GRADE) - m_pkChr->GetPoint(POINT_DEF_GRADE_BONUS));
		m_pkSk->SetPointVar("horse_level", m_pkChr->GetHorseLevel());

		//int iPenetratePct = (int)(1 + k*4);
		bool bIgnoreDefense = false;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_PENETRATE))
		{
			int iPenetratePct = (int)m_pkSk->kPointPoly2.Eval();

			if (number(1, 100) <= iPenetratePct)
				bIgnoreDefense = true;
		}

		bool bIgnoreTargetRating = false;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_IGNORE_TARGET_RATING))
		{
			int iPct = (int)m_pkSk->kPointPoly2.Eval();

			if (number(1, 100) <= iPct)
				bIgnoreTargetRating = true;
		}

		m_pkSk->SetPointVar("ar", CalcAttackRating(m_pkChr, pkChrVictim, bIgnoreTargetRating));

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
		{
			m_pkSk->SetPointVar("atk", CalcMeleeDamage(m_pkChr, pkChrVictim, true, bIgnoreTargetRating));
		}		
		else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
		{
			LPITEM pkBow, pkArrow;

			if (m_pkChr->GetArrowAndBow(&pkBow, &pkArrow))
				m_pkSk->SetPointVar("atk", CalcArrowDamage(m_pkChr, pkChrVictim, pkBow, pkArrow, true));
			else
				m_pkSk->SetPointVar("atk", 0);
		}

		if (m_pkSk->bPointOn == POINT_MOV_SPEED)
			m_pkSk->kPointPoly.SetVar("maxv", pkChrVictim->GetLimitPoint(POINT_MOV_SPEED));

		m_pkSk->SetPointVar("maxhp", pkChrVictim->GetMaxHP());
		m_pkSk->SetPointVar("maxsp", pkChrVictim->GetMaxSP());

		m_pkSk->SetPointVar("chain", m_pkChr->GetChainLightningIndex());
		m_pkChr->IncChainLightningIndex();

		bool bUnderEunhyung = m_pkChr->GetAffectedEunhyung() > 0;

		m_pkSk->SetPointVar("ek", m_pkChr->GetAffectedEunhyung() * 1. / 100);
		//m_pkChr->ClearAffectedEunhyung();
		SetPolyVarForAttack(m_pkChr, m_pkSk, m_pkWeapon);

		DAM_LL iAmount = 0;

		if (m_pkChr->GetUsedSkillMasterType(m_pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			iAmount = (DAM_LL)m_pkSk->kMasterBonusPoly.Eval();
		}
		else
		{
			iAmount = (DAM_LL)m_pkSk->kPointPoly.Eval();
		}

		////////////////////////////////////////////////////////////////////////////////
		iAmount = -iAmount;

		if (m_pkSk->dwVnum == SKILL_AMSEOP)
		{
			float fDelta = GetDegreeDelta(m_pkChr->GetRotation(), pkChrVictim->GetRotation());
			float adjust;

			if (fDelta < 35.0f)
			{
				adjust = 1.5f;

				if (bUnderEunhyung)
					adjust += 0.5f;

				if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
				{
					adjust += 0.5f;
				}
			}
			else
			{
				adjust = 1.0f;

				if (bUnderEunhyung)
					adjust += 0.5f;

				if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
					adjust += 0.5f;
			}

			iAmount = (DAM_LL)(iAmount * adjust);
		}
		else if (m_pkSk->dwVnum == SKILL_GUNGSIN)
		{
			float adjust = 1.0;

			if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			{
				adjust = 1.35f;
			}

			iAmount = (DAM_LL)(iAmount * adjust);
		}
#ifdef ENABLE_WOLFMAN_CHARACTER
		else if (m_pkSk->dwVnum == SKILL_GONGDAB)
		{
			float adjust = 1.0;

			if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			{
				adjust = 1.35f;
			}

			iAmount = (DAM_LL)(iAmount * adjust);
		}
#endif

		iDam = CalcBattleDamage(iAmount, m_pkChr->GetLevel(), pkChrVictim->GetLevel());

		if (m_pkChr->IsPC() && m_pkChr->m_SkillUseInfo[m_pkSk->dwVnum].GetMainTargetVID() != (DWORD)pkChrVictim->GetVID())
		{
			iDam = (DAM_LL)(iDam * m_pkSk->kSplashAroundDamageAdjustPoly.Eval());
		}

		EDamageType dt = DAMAGE_TYPE_NONE;

		switch (m_pkSk->bSkillAttrType)
		{
		case SKILL_ATTR_TYPE_NORMAL:
			break;

		case SKILL_ATTR_TYPE_MELEE:
		{
			dt = DAMAGE_TYPE_MELEE;

			LPITEM pkWeapon = m_pkChr->GetWear(WEAR_WEAPON);

			if (pkWeapon)
				switch (pkWeapon->GetSubType())
				{
				case WEAPON_SWORD:
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_SWORD)) / 100;
					break;

				case WEAPON_TWO_HANDED:
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_TWOHAND)) / 100;
					break;

				case WEAPON_DAGGER:
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_DAGGER)) / 100;
					break;

				case WEAPON_BELL:
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_BELL)) / 100;
					break;

				case WEAPON_FAN:
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_FAN)) / 100;
					break;
#ifdef ENABLE_WOLFMAN_CHARACTER
				case WEAPON_CLAW:
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_CLAW)) / 100;
#if defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_ITEM_CLAW_AS_DAGGER)
					iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_DAGGER)) / 100;
#endif
					break;
#endif
				}

			if (!bIgnoreDefense)
				iDam -= pkChrVictim->GetPoint(POINT_DEF_GRADE);
		}
		break;

		case SKILL_ATTR_TYPE_RANGE:
			dt = DAMAGE_TYPE_RANGE;
			iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_BOW)) / 100;
			break;

		case SKILL_ATTR_TYPE_MAGIC:
			dt = DAMAGE_TYPE_MAGIC;
			iDam = CalcAttBonus(m_pkChr, pkChrVictim, iDam);
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
			{
				const DAM_LL resist_magic = MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_MAGIC), 100);
				const DAM_LL resist_magic_reduction = MINMAX(0, (m_pkChr->GetJob() == JOB_SURA) ? m_pkChr->GetPoint(POINT_RESIST_MAGIC_REDUCTION) / 2 : m_pkChr->GetPoint(POINT_RESIST_MAGIC_REDUCTION), 50);
				const DAM_LL total_res_magic = MINMAX(0, resist_magic - resist_magic_reduction, 100);
				iDam = iDam * (100 - total_res_magic) / 100;
			}
#else
			iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_MAGIC)) / 100;
#endif
			break;

		default:
			sys_err("Unknown skill attr type %u vnum %u", m_pkSk->bSkillAttrType, m_pkSk->dwVnum);
			break;
		}

		if (pkChrVictim->IsNPC())
		{
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_WIND))
			{
				iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_WIND)) / 100;
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_ELEC))
			{
				iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_ELEC)) / 100;
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_FIRE))
			{
				iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_FIRE)) / 100;
			}
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_COMPUTE_MAGIC_DAMAGE))
			dt = DAMAGE_TYPE_MAGIC;

		if (pkChrVictim->CanBeginFight())
			pkChrVictim->BeginFight(m_pkChr);

		{
			BYTE AntiSkillID = 0;

			switch (m_pkSk->dwVnum)
			{
			case SKILL_TANHWAN:		AntiSkillID = SKILL_7_A_ANTI_TANHWAN;		break;
			case SKILL_AMSEOP:		AntiSkillID = SKILL_7_B_ANTI_AMSEOP;		break;
			case SKILL_SWAERYUNG:	AntiSkillID = SKILL_7_C_ANTI_SWAERYUNG;		break;
			case SKILL_YONGBI:		AntiSkillID = SKILL_7_D_ANTI_YONGBI;		break;
			case SKILL_GIGONGCHAM:	AntiSkillID = SKILL_8_A_ANTI_GIGONGCHAM;	break;
			case SKILL_YEONSA:		AntiSkillID = SKILL_8_B_ANTI_YEONSA;		break;
			case SKILL_MAHWAN:		AntiSkillID = SKILL_8_C_ANTI_MAHWAN;		break;
			case SKILL_BYEURAK:		AntiSkillID = SKILL_8_D_ANTI_BYEURAK;		break;
			}

			if (0 != AntiSkillID)
			{
				BYTE AntiSkillLevel = pkChrVictim->GetSkillLevel(AntiSkillID);

				if (0 != AntiSkillLevel)
				{
					CSkillProto* pkSk = CSkillManager::instance().Get(AntiSkillID);
					if (!pkSk)
					{
						sys_err("There is no anti skill(%d) in skill proto", AntiSkillID);
					}
					else
					{
						pkSk->SetPointVar("k", 1.0f * pkChrVictim->GetSkillPower(AntiSkillID) * pkSk->bMaxLevel / 100);
						double ResistAmount = pkSk->kPointPoly.Eval();
						iDam -= iDam * (ResistAmount / 100.0);
					}
				}
			}
		}

		if (!pkChrVictim->Damage(m_pkChr, iDam, dt) && !pkChrVictim->IsStun())
		{
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_REMOVE_GOOD_AFFECT))
			{
#ifdef ENABLE_NULLIFYAFFECT_LIMIT
				int iLevel = m_pkChr->GetLevel();
				int yLevel = pkChrVictim->GetLevel();
				// const float k = 1.0 * m_pkChr->GetSkillPower(m_pkSk->dwVnum, bSkillLevel) * m_pkSk->bMaxLevel / 100;
				int iDifLev = 9;
				if ((iLevel - iDifLev <= yLevel) && (iLevel + iDifLev >= yLevel))
#endif
				{
					int iAmount2 = (int)m_pkSk->kPointPoly2.Eval();
					int iDur2 = (int)m_pkSk->kDurationPoly2.Eval();
					iDur2 += m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

					if (number(1, 100) <= iAmount2)
					{
						pkChrVictim->RemoveGoodAffect();
						pkChrVictim->AddAffect(m_pkSk->dwVnum, POINT_NONE, 0, AFF_PABEOP, iDur2, 0, true);
					}
				}
			}
#ifdef ENABLE_WOLFMAN_CHARACTER
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW | SKILL_FLAG_STUN | SKILL_FLAG_FIRE_CONT | SKILL_FLAG_POISON | SKILL_FLAG_BLEEDING))
#else
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW | SKILL_FLAG_STUN | SKILL_FLAG_FIRE_CONT | SKILL_FLAG_POISON))
#endif
			{
				int iPct = (int)m_pkSk->kPointPoly2.Eval();
				int iDur = (int)m_pkSk->kDurationPoly2.Eval();

				iDur += m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_STUN))
				{
					SkillAttackAffect(pkChrVictim, iPct, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, iDur, m_pkSk->szName);
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW))
				{
					SkillAttackAffect(pkChrVictim, iPct, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, iDur, m_pkSk->szName);
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_FIRE_CONT))
				{
					m_pkSk->SetDurationVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
					m_pkSk->SetDurationVar("iq", m_pkChr->GetPoint(POINT_IQ));

					iDur = (int)m_pkSk->kDurationPoly2.Eval();
					int bonus = m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

					if (bonus != 0)
					{
						iDur += bonus / 2;
					}

					if (number(1, 100) <= iDur)
					{
						pkChrVictim->AttackedByFire(m_pkChr, iPct, 5);
					}
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_POISON))
				{
					if (number(1, 100) <= iPct)
						pkChrVictim->AttackedByPoison(m_pkChr);
				}
#ifdef ENABLE_WOLFMAN_CHARACTER
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_BLEEDING))
				{
					if (number(1, 100) <= iPct)
						pkChrVictim->AttackedByBleeding(m_pkChr);
				}
#endif
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH | SKILL_FLAG_CRUSH_LONG) &&
				!IS_SET(pkChrVictim->GetAIFlag(), AIFLAG_NOMOVE))
			{
				float fCrushSlidingLength = 200;

				if (m_pkChr->IsNPC())
					fCrushSlidingLength = 400;

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH_LONG))
					fCrushSlidingLength *= 2;

				float fx, fy;
				float degree = GetDegreeFromPositionXY(m_pkChr->GetX(), m_pkChr->GetY(), pkChrVictim->GetX(), pkChrVictim->GetY());

				if (m_pkSk->dwVnum == SKILL_HORSE_WILDATTACK)
				{
					degree -= m_pkChr->GetRotation();
					degree = fmod(degree, 360.0f) - 180.0f;

					if (degree > 0)
						degree = m_pkChr->GetRotation() + 90.0f;
					else
						degree = m_pkChr->GetRotation() - 90.0f;
				}
				GetDeltaByDegree(degree, fCrushSlidingLength, &fx, &fy);
#ifdef ENABLE_FALL_FIX
				bool victimCanFall = pkChrVictim->CanFall();
				sys_log(0, "CRUSH! %s -> %s (%d %d) -> (%d %d) executed (fall): %d", m_pkChr->GetName(), pkChrVictim->GetName(), pkChrVictim->GetX(), pkChrVictim->GetY(), static_cast<int>(pkChrVictim->GetX() + fx), static_cast<int>(pkChrVictim->GetY() + fy), victimCanFall);
				if (victimCanFall)
				{
					int tx = static_cast<int>(pkChrVictim->GetX() + fx);
					int ty = static_cast<int>(pkChrVictim->GetY() + fy);
					if (true == SECTREE_MANAGER::instance().IsMovablePosition(pkChrVictim->GetMapIndex(), tx, ty))
						return;
					pkChrVictim->Sync(tx, ty);
					pkChrVictim->Goto(tx, ty);
					pkChrVictim->CalculateMoveDuration();
				}
#else
				sys_log(0, "CRUSH! %s -> %s (%d %d) -> (%d %d)", m_pkChr->GetName(), pkChrVictim->GetName(), pkChrVictim->GetX(), pkChrVictim->GetY(), (long)(pkChrVictim->GetX() + fx), (long)(pkChrVictim->GetY() + fy));
				long tx = (long)(pkChrVictim->GetX() + fx);
				long ty = (long)(pkChrVictim->GetY() + fy);

				pkChrVictim->Sync(tx, ty);
				pkChrVictim->Goto(tx, ty);
				pkChrVictim->CalculateMoveDuration();
#endif
				if (m_pkChr->IsPC() && m_pkChr->m_SkillUseInfo[m_pkSk->dwVnum].GetMainTargetVID() == (DWORD)pkChrVictim->GetVID())
				{
					SkillAttackAffect(pkChrVictim, 1000, IMMUNE_STUN, m_pkSk->dwVnum, POINT_NONE, 0, AFF_STUN, 4, m_pkSk->szName);
				}
				else
				{
					pkChrVictim->SyncPacket();
				}
			}
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_HP_ABSORB))
		{
			int iPct = (int)m_pkSk->kPointPoly2.Eval();
			m_pkChr->PointChange(POINT_HP, iDam * iPct / 100);
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SP_ABSORB))
		{
			int iPct = (int)m_pkSk->kPointPoly2.Eval();
			m_pkChr->PointChange(POINT_SP, iDam * iPct / 100);
		}

		if (m_pkSk->dwVnum == SKILL_CHAIN && m_pkChr->GetChainLightningIndex() < m_pkChr->GetChainLightningMaxCount())
		{
			chain_lightning_event_info* info = AllocEventInfo<chain_lightning_event_info>();

			info->dwVictim = pkChrVictim->GetVID();
			info->dwChr = m_pkChr->GetVID();

			event_create(ChainLightningEvent, info, passes_per_sec / 5);
		}
	}

	int		m_x;
	int		m_y;
	CSkillProto* m_pkSk;
	LPCHARACTER	m_pkChr;
	int		m_iAmount;
	int		m_iAG;
	int		m_iCount;
	int		m_iMaxHit;
	LPITEM	m_pkWeapon;
	bool m_bDisableCooltime;
	TSkillUseInfo* m_pInfo;
	BYTE m_bUseSkillPower;
};

struct FuncSplashAffect
{
	FuncSplashAffect(LPCHARACTER ch, int x, int y, int iDist, DWORD dwVnum, BYTE bPointOn, int iAmount, DWORD dwAffectFlag, int iDuration, int iSPCost, bool bOverride, int iMaxHit)
	{
		m_x = x;
		m_y = y;
		m_iDist = iDist;
		m_dwVnum = dwVnum;
		m_bPointOn = bPointOn;
		m_iAmount = iAmount;
		m_dwAffectFlag = dwAffectFlag;
		m_iDuration = iDuration;
		m_iSPCost = iSPCost;
		m_bOverride = bOverride;
		m_pkChrAttacker = ch;
		m_iMaxHit = iMaxHit;
		m_iCount = 0;
	}

	void operator () (LPENTITY ent)
	{
		if (m_iMaxHit && m_iMaxHit <= m_iCount)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pkChr = (LPCHARACTER)ent;

			if (DISTANCE_APPROX(m_x - pkChr->GetX(), m_y - pkChr->GetY()) < m_iDist)
			{
				if (m_dwVnum == SKILL_TUSOK)
					if (pkChr->CanBeginFight())
						pkChr->BeginFight(m_pkChrAttacker);

				if (pkChr->IsPC() && m_dwVnum == SKILL_TUSOK)
					pkChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration / 3, m_iSPCost, m_bOverride);
				else
					pkChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration, m_iSPCost, m_bOverride);

				m_iCount++;
			}
		}
	}

	LPCHARACTER m_pkChrAttacker;
	int		m_x;
	int		m_y;
	int		m_iDist;
	DWORD	m_dwVnum;
	BYTE	m_bPointOn;
	int		m_iAmount;
	DWORD	m_dwAffectFlag;
	int		m_iDuration;
	int		m_iSPCost;
	bool	m_bOverride;
	int         m_iMaxHit;
	int         m_iCount;
};

EVENTINFO(skill_gwihwan_info)
{
	DWORD pid;
	BYTE bsklv;

	skill_gwihwan_info()
		: pid(0)
		, bsklv(0)
	{
	}
};

EVENTFUNC(skill_gwihwan_event)
{
	skill_gwihwan_info* info = dynamic_cast<skill_gwihwan_info*>(event->info);

	if (info == NULL)
	{
		sys_err("skill_gwihwan_event> <Factor> Null pointer");
		return 0;
	}

	DWORD pid = info->pid;
	BYTE sklv = info->bsklv;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (!ch)
		return 0;

	int percent = 20 * sklv - 1;

	if (number(1, 100) <= percent)
	{
		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
		{
			sys_log(1, "Recall: %s %d %d -> %d %d", ch->GetName(), ch->GetX(), ch->GetY(), pos.x, pos.y);
			ch->WarpSet(pos.x, pos.y);
		}
		else
		{
			sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", ch->GetName(), ch->GetX(), ch->GetY());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	else
	{
		ch->NewChatPacket(SKILL_TALK_14);
	}
	return 0;
}

int CHARACTER::ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel)
{
	if (GetMountVnum())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	if (g_bSkillDisable)
		return BATTLE_NONE;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return BATTLE_NONE;

	if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
		return BATTLE_NONE;

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pkSk->dwVnum)) == 0)
		{
			return BATTLE_NONE;
		}
	}

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetPointVar("k", k);
	pkSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMeleeDamage(this, this, true, false));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMagicDamage(this, this));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pkBow, pkArrow;
		if (GetArrowAndBow(&pkBow, &pkArrow))
		{
			pkSk->SetPointVar("atk", CalcArrowDamage(this, this, pkBow, pkArrow, true));
		}
		else
		{
			pkSk->SetPointVar("atk", 0);
		}
	}

	if (pkSk->bPointOn == POINT_MOV_SPEED)
	{
		pkSk->SetPointVar("maxv", this->GetLimitPoint(POINT_MOV_SPEED));
	}

	pkSk->SetPointVar("lv", GetLevel());
	pkSk->SetPointVar("iq", GetPoint(POINT_IQ));
	pkSk->SetPointVar("str", GetPoint(POINT_ST));
	pkSk->SetPointVar("dex", GetPoint(POINT_DX));
	pkSk->SetPointVar("con", GetPoint(POINT_HT));
	pkSk->SetPointVar("maxhp", this->GetMaxHP());
	pkSk->SetPointVar("maxsp", this->GetMaxSP());
	pkSk->SetPointVar("chain", 0);
	pkSk->SetPointVar("ar", CalcAttackRating(this, this));
	pkSk->SetPointVar("def", GetPoint(POINT_DEF_GRADE));
	pkSk->SetPointVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pkSk->SetPointVar("horse_level", GetHorseLevel());

	if (pkSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true);

	LPITEM pkWeapon = GetWear(WEAR_WEAPON);

	SetPolyVarForAttack(this, pkSk, pkWeapon);

	pkSk->SetDurationVar("k", k/*bSkillLevel*/);

	int iAmount = (int)pkSk->kPointPoly.Eval();
	int iAmount2 = (int)pkSk->kPointPoly2.Eval();

	// ADD_GRANDMASTER_SKILL
	int iAmount3 = (int)pkSk->kPointPoly3.Eval();

	if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		/*
		   if (iAmount >= 0)
		   iAmount += (int) m_pkSk->kMasterBonusPoly.Eval();
		   else
		   iAmount -= (int) m_pkSk->kMasterBonusPoly.Eval();
		 */
		iAmount = (int)pkSk->kMasterBonusPoly.Eval();
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (number(1, 100) <= iAmount2)
		{
			RemoveBadAffect();
		}
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		//

		//
		bool bAdded = false;

		if (pkSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int iAG = 0;

			FuncSplashDamage f(posTarget.x, posTarget.y, pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, m_bDisableCooltime, IsPC() ? &m_SkillUseInfo[dwVnum] : NULL, GetSkillPower(dwVnum, bSkillLevel));

			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (GetSectree())
					GetSectree()->ForEachAround(f);
			}
			else
			{
				//if (dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN skill call FuncSplashDamage %s", GetName());
				f(this);
			}
		}
		else
		{
			//if (dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN skill no damage %d %s", iAmount, GetName());
			int iDur = (int)pkSk->kDurationPoly.Eval();

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END))
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_MUYEONG)
					{
						//if (dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN skill cannot hit %s", GetName());
						return BATTLE_NONE;
					}

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pkSk->bPointOn2 != POINT_NONE)
		{
			int iDur = (int)pkSk->kDurationPoly2.Eval();

			sys_log(1, "try second %u %d %d", pkSk->dwVnum, pkSk->bPointOn2, iDur);

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
			else
			{
				PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER && pkSk->bPointOn3 != POINT_NONE)
		{
			int iDur = (int)pkSk->kDurationPoly3.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, 0 /*pkSk->dwAffectFlag3*/, iDur, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, 0 /*pkSk->dwAffectFlag3*/, iDur, 0, !bAdded, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
			}
			else
			{
				PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		bool bAdded = false;
		int iDur = (int)pkSk->kDurationPoly.Eval();

		if (iDur > 0)
		{
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

			pkSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			AddAffect(pkSk->dwVnum,
				pkSk->bPointOn,
				iAmount,
				pkSk->dwAffectFlag,
				iDur,
				(long)pkSk->kDurationSPCostPoly.Eval(),
				!bAdded);

			bAdded = true;
		}
		else
		{
			PointChange(pkSk->bPointOn, iAmount);
		}

		if (pkSk->bPointOn2 != POINT_NONE)
		{
			int iDur = (int)pkSk->kDurationPoly2.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
				AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded);
				bAdded = true;
			}
			else
			{
				PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER && pkSk->bPointOn3 != POINT_NONE)
		{
			int iDur = (int)pkSk->kDurationPoly3.Eval();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
				AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, 0 /*pkSk->dwAffectFlag3*/, iDur, 0, !bAdded);
			}
			else
			{
				PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_NONE;
	}
}

#ifdef ENABLE_WOLFMAN_CHARACTER
struct FComputeSkillParty
{
	FComputeSkillParty(DWORD dwVnum, LPCHARACTER pkAttacker, BYTE bSkillLevel = 0)
		: m_dwVnum(dwVnum), m_pkAttacker(pkAttacker), m_bSkillLevel(bSkillLevel)
	{
	}

	void operator () (LPCHARACTER ch)
	{
		m_pkAttacker->ComputeSkill(m_dwVnum, ch, m_bSkillLevel);
	}

	DWORD m_dwVnum;
	LPCHARACTER m_pkAttacker;
	BYTE m_bSkillLevel;
};

int CHARACTER::ComputeSkillParty(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel)
{
	FComputeSkillParty f(dwVnum, pkVictim, bSkillLevel);
	if (GetParty() && GetParty()->GetNearMemberCount())
		GetParty()->ForEachNearMember(f);
	else
		f(this);

	return BATTLE_NONE;
}
#endif

int CHARACTER::ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel)
{
	const bool bCanUseHorseSkill = CanUseHorseSkill();

	if (dwVnum != SKILL_MUYEONG)//@Lightwork#2
	{
		if (false == bCanUseHorseSkill && true == IsRiding()
			)
			return BATTLE_NONE;
	}

	if (IsPolymorphed())
		return BATTLE_NONE;

	if (g_bSkillDisable)
		return BATTLE_NONE;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return BATTLE_NONE;

	if (dwVnum != SKILL_MUYEONG)//@Lightwork#2
	{
		if (bCanUseHorseSkill && pkSk->dwType != SKILL_TYPE_HORSE)
			return BATTLE_NONE;

		if (!bCanUseHorseSkill && pkSk->dwType == SKILL_TYPE_HORSE)
			return BATTLE_NONE;
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		pkVictim = this;
	// #ifdef ENABLE_WOLFMAN_CHARACTER
		// else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY))
			// pkVictim = this;
	// #endif

	if (!pkVictim)
	{
		return BATTLE_NONE;
	}

	if (pkSk->dwTargetRange && DISTANCE_SQRT(GetX() - pkVictim->GetX(), GetY() - pkVictim->GetY()) >= pkSk->dwTargetRange + 50)
	{
		return BATTLE_NONE;
	}

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pkSk->dwVnum)) == 0)
		{
			return BATTLE_NONE;
		}
	}

	if (pkVictim->IsAffectFlag(AFF_PABEOP) && pkVictim->IsGoodAffect(dwVnum))
	{
		return BATTLE_NONE;
	}

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetPointVar("k", k);
	pkSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	if (pkSk->dwType == SKILL_TYPE_HORSE)
	{
		LPITEM pkBow, pkArrow;
		if (GetArrowAndBow(&pkBow, &pkArrow))
		{
			pkSk->SetPointVar("atk", CalcArrowDamage(this, pkVictim, pkBow, pkArrow, true));
		}
		else
		{
			pkSk->SetPointVar("atk", CalcMeleeDamage(this, pkVictim, true, false));
		}
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMeleeDamage(this, pkVictim, true, false));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMagicDamage(this, pkVictim));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pkBow, pkArrow;
		if (GetArrowAndBow(&pkBow, &pkArrow))
		{
			pkSk->SetPointVar("atk", CalcArrowDamage(this, pkVictim, pkBow, pkArrow, true));
		}
		else
		{
			pkSk->SetPointVar("atk", 0);
		}
	}

	if (pkSk->bPointOn == POINT_MOV_SPEED)
	{
		pkSk->SetPointVar("maxv", pkVictim->GetLimitPoint(POINT_MOV_SPEED));
	}

	pkSk->SetPointVar("lv", GetLevel());
	pkSk->SetPointVar("iq", GetPoint(POINT_IQ));
	pkSk->SetPointVar("str", GetPoint(POINT_ST));
	pkSk->SetPointVar("dex", GetPoint(POINT_DX));
	pkSk->SetPointVar("con", GetPoint(POINT_HT));
	pkSk->SetPointVar("maxhp", pkVictim->GetMaxHP());
	pkSk->SetPointVar("maxsp", pkVictim->GetMaxSP());
	pkSk->SetPointVar("chain", 0);
	pkSk->SetPointVar("ar", CalcAttackRating(this, pkVictim));
	pkSk->SetPointVar("def", GetPoint(POINT_DEF_GRADE));
	pkSk->SetPointVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pkSk->SetPointVar("horse_level", GetHorseLevel());

	if (pkSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true);

	LPITEM pkWeapon = GetWear(WEAR_WEAPON);

	SetPolyVarForAttack(this, pkSk, pkWeapon);

	pkSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
	pkSk->kDurationPoly2.SetVar("k", k/*bSkillLevel*/);

	int iAmount = (int)pkSk->kPointPoly.Eval();
	int iAmount2 = (int)pkSk->kPointPoly2.Eval();
	int iAmount3 = (int)pkSk->kPointPoly3.Eval();

	// ADD_GRANDMASTER_SKILL
	if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		iAmount = (int)pkSk->kMasterBonusPoly.Eval();
	}

	// END_OF_ADD_GRANDMASTER_SKILL

	// REMOVE_BAD_AFFECT_BUG_FIX
	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (number(1, 100) <= iAmount2)
		{
			pkVictim->RemoveBadAffect();
		}
	}
	// END_OF_REMOVE_BAD_AFFECT_BUG_FIX

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE) &&
		!(pkSk->dwVnum == SKILL_MUYEONG && pkVictim == this) && !(pkSk->IsChargeSkill() && pkVictim == this))
	{
		bool bAdded = false;

		if (pkSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int iAG = 0;

			FuncSplashDamage f(pkVictim->GetX(), pkVictim->GetY(), pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, m_bDisableCooltime, IsPC() ? &m_SkillUseInfo[dwVnum] : NULL, GetSkillPower(dwVnum, bSkillLevel));
			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (pkVictim->GetSectree())
					pkVictim->GetSectree()->ForEachAround(f);
			}
			else
			{
				f(pkVictim);
			}
		}
		else
		{
			pkSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
			int iDur = (int)pkSk->kDurationPoly.Eval();

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END))
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_MUYEONG)
					{
						return BATTLE_NONE;
					}

			if (iDur > 0)
			{

				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef ENABLE_PASSIVE_SKILLS
				iDur += GetPassiveCooldDown(pkSk->dwVnum);
#endif
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pkSk->bPointOn2 != POINT_NONE && !pkSk->IsChargeSkill())
		{
			pkSk->kDurationPoly2.SetVar("k", k/*bSkillLevel*/);
			int iDur = (int)pkSk->kDurationPoly2.Eval();
#ifdef ENABLE_PASSIVE_SKILLS
			iDur += GetPassiveCooldDown(pkSk->dwVnum);
#endif
			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE && !pkSk->IsChargeSkill() && GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			pkSk->kDurationPoly3.SetVar("k", k/*bSkillLevel*/);
			int iDur = (int)pkSk->kDurationPoly3.Eval();
#ifdef ENABLE_PASSIVE_SKILLS
			iDur += GetPassiveCooldDown(pkSk->dwVnum);
#endif
			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		if (dwVnum == SKILL_MUYEONG)
		{
			pkSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
			pkSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			int iDur = (long)pkSk->kDurationPoly.Eval();
#ifdef ENABLE_PASSIVE_SKILLS
			iDur += GetPassiveCooldDown(pkSk->dwVnum);
#endif
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
			if (pkVictim == this)
				AddAffect(dwVnum,
					POINT_NONE, 0,
					AFF_MUYEONG,
					iDur,
					(long)pkSk->kDurationSPCostPoly.Eval(),
					true);

			return BATTLE_NONE;
		}

		bool bAdded = false;
		pkSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
		int iDur = (int)pkSk->kDurationPoly.Eval();

		if (iDur > 0)
		{
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef ENABLE_PASSIVE_SKILLS
			iDur += GetPassiveCooldDown(pkSk->dwVnum);
#endif
			pkSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			if (pkSk->bPointOn2 != POINT_NONE)
			{
				pkVictim->RemoveAffect(pkSk->dwVnum);

				int iDur2 = (int)pkSk->kDurationPoly2.Eval();

				if (iDur2 > 0)
				{
					iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, false);
				}
				else
				{
					pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
				}

				DWORD affact_flag = pkSk->dwAffectFlag;

				// ADD_GRANDMASTER_SKILL
				if ((pkSk->dwVnum == SKILL_CHUNKEON && GetUsedSkillMasterType(pkSk->dwVnum) < SKILL_GRAND_MASTER))
					affact_flag = AFF_CHEONGEUN_WITH_FALL;
				// END_OF_ADD_GRANDMASTER_SKILL

				pkVictim->AddAffect(pkSk->dwVnum,
					pkSk->bPointOn,
					iAmount,
					affact_flag,
					iDur,
					(long)pkSk->kDurationSPCostPoly.Eval(),
					false);
			}
			else
			{
				pkVictim->AddAffect(pkSk->dwVnum,
					pkSk->bPointOn,
					iAmount,
					pkSk->dwAffectFlag,
					iDur,
					(long)pkSk->kDurationSPCostPoly.Eval(),
					// ADD_GRANDMASTER_SKILL
					!bAdded);
				// END_OF_ADD_GRANDMASTER_SKILL
			}

			bAdded = true;
		}
		else
		{
			if (!pkSk->IsChargeSkill())
				pkVictim->PointChange(pkSk->bPointOn, iAmount);

			if (pkSk->bPointOn2 != POINT_NONE)
			{
				pkVictim->RemoveAffect(pkSk->dwVnum);

				int iDur2 = (int)pkSk->kDurationPoly2.Eval();
#ifdef ENABLE_PASSIVE_SKILLS
				iDur2 += GetPassiveCooldDown(pkSk->dwVnum);
#endif
				if (iDur2 > 0)
				{
					iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
					if (pkSk->IsChargeSkill())
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, AFF_TANHWAN_DASH, iDur2, 0, false);
					else
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, false);
				}
				else
				{
					pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
				}
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE && !pkSk->IsChargeSkill() && GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			pkSk->kDurationPoly3.SetVar("k", k/*bSkillLevel*/);
			int iDur = (int)pkSk->kDurationPoly3.Eval();
#ifdef ENABLE_PASSIVE_SKILLS
			iDur += GetPassiveCooldDown(pkSk->dwVnum);
#endif
			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_NONE;
	}
}

bool CHARACTER::UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster)
{
	if (false == CanUseSkill(dwVnum))
		return false;

	if ((dwVnum == SKILL_GEOMKYUNG || dwVnum == SKILL_GWIGEOM) && !GetWear(WEAR_WEAPON))//@Lightwork#88
		return false;

	if (g_bSkillDisable)
		return false;

	if (IsObserverMode())
		return false;

	if (!CanMove())
		return false;

	if (IsPolymorphed())
		return false;

	const bool bCanUseHorseSkill = CanUseHorseSkill();

	if (dwVnum == SKILL_HORSE_SUMMON)
	{
		if (GetSkillLevel(dwVnum) == 0)
			return false;
		return true;
	}

	if (false == bCanUseHorseSkill && true == IsRiding())
		return false;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return false;

	if (bCanUseHorseSkill && pkSk->dwType != SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (!bCanUseHorseSkill && pkSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (GetSkillLevel(dwVnum) == 0)
		return false;

	// NO_GRANDMASTER
	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		bUseGrandMaster = false;
	// END_OF_NO_GRANDMASTER

	// MINING
	if (GetWear(WEAR_WEAPON) && (GetWear(WEAR_WEAPON)->GetType() == ITEM_ROD || GetWear(WEAR_WEAPON)->GetType() == ITEM_PICK))
		return false;
	// END_OF_MINING

	m_SkillUseInfo[dwVnum].TargetVIDMap.clear();

	if (pkSk->IsChargeSkill())
	{
		if ((IsAffectFlag(AFF_TANHWAN_DASH)) || (pkVictim && (pkVictim != this)))
		{
			if (!pkVictim)
				return false;

			if (!IsAffectFlag(AFF_TANHWAN_DASH))
			{
				if (!UseSkill(dwVnum, this))
					return false;
			}

			m_SkillUseInfo[dwVnum].SetMainTargetVID(pkVictim->GetVID());

			ComputeSkill(dwVnum, pkVictim);
			RemoveAffect(dwVnum);
			return true;
		}
	}

	if (dwVnum == SKILL_COMBO)
	{
		ChatPacket(CHAT_TYPE_COMMAND, "combo %d", GetSkillLevel(SKILL_COMBO));
		return true;
	}

	if ((0 != pkSk->dwAffectFlag || pkSk->dwVnum == SKILL_MUYEONG) && (pkSk->dwFlag & SKILL_FLAG_TOGGLE) && RemoveAffect(pkSk->dwVnum))
	{
		return true;
	}

	if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
		RemoveAffect(AFFECT_REVIVE_INVISIBLE);

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum) * pkSk->bMaxLevel / 100;

	pkSk->SetPointVar("k", k);
	pkSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	pkSk->kCooldownPoly.SetVar("k", k);
	int iCooltime = (int)pkSk->kCooldownPoly.Eval();
	int lMaxHit = pkSk->lMaxHit ? pkSk->lMaxHit : -1;

	pkSk->SetSPCostVar("k", k);

	DWORD dwCur = get_dword_time();

	if (dwVnum == SKILL_TERROR && m_SkillUseInfo[dwVnum].bUsed && m_SkillUseInfo[dwVnum].dwNextSkillUsableTime > dwCur)
	{
		sys_log(0, " SKILL_TERROR's Cooltime is not delta over %u", m_SkillUseInfo[dwVnum].dwNextSkillUsableTime - dwCur);
		return false;
	}

	int iNeededSP = 0;

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_HP_AS_COST))
	{
		pkSk->SetSPCostVar("maxhp", GetMaxHP());
		pkSk->SetSPCostVar("v", GetHP());
		iNeededSP = (int)pkSk->kSPCostPoly.Eval();

		// ADD_GRANDMASTER_SKILL
		if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
		{
			iNeededSP = (int)pkSk->kGrandMasterAddSPCostPoly.Eval();
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		if (GetHP() < iNeededSP)
			return false;

		PointChange(POINT_HP, -iNeededSP);
	}
	else
	{
		// SKILL_FOMULA_REFACTORING
		pkSk->SetSPCostVar("maxhp", GetMaxHP());
		pkSk->SetSPCostVar("maxv", GetMaxSP());
		pkSk->SetSPCostVar("v", GetSP());

		iNeededSP = (int)pkSk->kSPCostPoly.Eval();

		if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
		{
			iNeededSP = (int)pkSk->kGrandMasterAddSPCostPoly.Eval();
		}
		// END_OF_SKILL_FOMULA_REFACTORING

		if (GetSP() < iNeededSP)
			return false;

		PointChange(POINT_SP, -iNeededSP);
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		pkVictim = this;
#ifdef ENABLE_WOLFMAN_CHARACTER
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY))
		pkVictim = this;
#endif

	if ((pkSk->dwVnum == SKILL_MUYEONG) || (pkSk->IsChargeSkill() && !IsAffectFlag(AFF_TANHWAN_DASH) && !pkVictim))
	{
		pkVictim = this;
	}

	int iSplashCount = 1;

	if (false == m_bDisableCooltime)
	{
		if (false ==
			m_SkillUseInfo[dwVnum].UseSkill(
				bUseGrandMaster,
				(NULL != pkVictim && SKILL_HORSE_WILDATTACK != dwVnum) ? pkVictim->GetVID() : NULL,
				ComputeCooltime(iCooltime * 1000),
				iSplashCount,
				lMaxHit))
		{
			return false;
		}
	}

	if (dwVnum == SKILL_CHAIN)
	{
		ResetChainLightningIndex();
		AddChainLightningExcept(pkVictim);
	}

#ifdef ENABLE_SKILL_COLOR_SYSTEM
	if (pkVictim != NULL && (dwVnum == 94 || dwVnum == 95 || dwVnum == 96 || dwVnum == 110 || dwVnum == 111))
	{
		BYTE skill = 0;
		BYTE id = 0;
		switch (dwVnum)
		{
		case 94:
			skill = ESkillColorLength::BUFF_BEGIN + 0;
			id = 3;
			break;
		case 95:
			skill = ESkillColorLength::BUFF_BEGIN + 1;
			id = 4;
			break;
		case 96:
			skill = ESkillColorLength::BUFF_BEGIN + 2;
			id = 5;
			break;
		case 110:
			skill = ESkillColorLength::BUFF_BEGIN + 3;
			id = 4;
			break;
		case 111:
			skill = ESkillColorLength::BUFF_BEGIN + 4;
			id = 5;
			break;
		default:
			break;
		}

		DWORD data[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
		memcpy(data, pkVictim->GetSkillColor(), sizeof(data));

		DWORD dataAttacker[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
		memcpy(dataAttacker, this->GetSkillColor(), sizeof(dataAttacker));

		data[skill][0] = dataAttacker[id][0];
		data[skill][1] = dataAttacker[id][1];
		data[skill][2] = dataAttacker[id][2];
		data[skill][3] = dataAttacker[id][3];
		data[skill][4] = dataAttacker[id][4];

		pkVictim->SetSkillColor(data[0]);

		TSkillColor db_pack;
		memcpy(db_pack.dwSkillColor, data, sizeof(data));
		db_pack.player_id = pkVictim->GetPlayerID();
		db_clientdesc->DBPacketHeader(HEADER_GD_SKILL_COLOR_SAVE, 0, sizeof(TSkillColor));
		db_clientdesc->Packet(&db_pack, sizeof(TSkillColor));
	}
#endif

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		ComputeSkill(dwVnum, this);
#ifdef ENABLE_WOLFMAN_CHARACTER
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY))
		ComputeSkillParty(dwVnum, this);
#endif
	else if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK))
		ComputeSkill(dwVnum, pkVictim);
	else if (dwVnum == SKILL_BYEURAK)
		ComputeSkill(dwVnum, pkVictim);
	else if (dwVnum == SKILL_PAERYONG)//@Lightwork#23
		ComputeSkill(dwVnum, pkVictim);//@Lightwork#23
	else if (dwVnum == SKILL_MUYEONG || pkSk->IsChargeSkill())
		ComputeSkill(dwVnum, pkVictim);

	m_dwLastSkillTime = get_dword_time();

	return true;
}

int CHARACTER::GetUsedSkillMasterType(DWORD dwVnum)
{
	const TSkillUseInfo& rInfo = m_SkillUseInfo[dwVnum];

	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		return GetSkillMasterType(dwVnum);

	if (rInfo.isGrandMaster)
		return GetSkillMasterType(dwVnum);

	return MIN(GetSkillMasterType(dwVnum), SKILL_MASTER);
}

int CHARACTER::GetSkillMasterType(DWORD dwVnum) const
{
	if (!IsPC())
		return 0;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow %u", GetName(), dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].bMasterType : SKILL_NORMAL;
}

int CHARACTER::GetSkillPower(DWORD dwVnum, BYTE bLevel) const
{
	if (dwVnum >= SKILL_LANGUAGE1 && dwVnum <= SKILL_LANGUAGE3 && IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
	{
		return 100;
	}

	if (dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)
	{
		if (GetGuild())
			return 100 * GetGuild()->GetSkillLevel(dwVnum) / 7 / 7;
		else
			return 0;
	}

	if (bLevel)
	{
		//SKILL_POWER_BY_LEVEL
		return GetSkillPowerByLevel(bLevel, true);
		//END_SKILL_POWER_BY_LEVEL;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow %u", GetName(), dwVnum);
		return 0;
	}

	//SKILL_POWER_BY_LEVEL
	return GetSkillPowerByLevel(GetSkillLevel(dwVnum));
	//SKILL_POWER_BY_LEVEL
}

int CHARACTER::GetSkillLevel(DWORD dwVnum) const
{
	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow %u", GetName(), dwVnum);
		sys_log(0, "%s skill vnum overflow %u", GetName(), dwVnum);
		return 0;
	}

	return MIN(SKILL_MAX_LEVEL, m_pSkillLevels ? m_pSkillLevels[dwVnum].bLevel : 0);
}

EVENTFUNC(skill_muyoung_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("skill_muyoung_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->IsAffectFlag(AFF_MUYEONG))
	{
		ch->StopMuyeongEvent();
		return 0;
	}

	// 1. Find Victim
	FFindNearVictim f(ch, ch);
	if (ch->GetSectree())
	{
		ch->GetSectree()->ForEachAround(f);
		// 2. Shoot!
		if (f.GetVictim())
		{
			ch->CreateFly(FLY_SKILL_MUYEONG, f.GetVictim());
			ch->ComputeSkill(SKILL_MUYEONG, f.GetVictim());
		}
	}

	return PASSES_PER_SEC(3);
}

void CHARACTER::StartMuyeongEvent()
{
	if (m_pkMuyeongEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkMuyeongEvent = event_create(skill_muyoung_event, info, PASSES_PER_SEC(1));
}

void CHARACTER::StopMuyeongEvent()
{
	event_cancel(&m_pkMuyeongEvent);
}

void CHARACTER::SkillLearnWaitMoreTimeMessage(DWORD ms)
{
	ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("SKILL_TALKING_2"));
}

void CHARACTER::DisableCooltime()
{
	m_bDisableCooltime = true;
}

bool CHARACTER::HasMobSkill() const
{
	return CountMobSkill() > 0;
}

size_t CHARACTER::CountMobSkill() const
{
	if (!m_pkMobData)
		return 0;

	size_t c = 0;

	for (size_t i = 0; i < MOB_SKILL_MAX_NUM; ++i)
		if (m_pkMobData->m_table.Skills[i].dwVnum)
			++c;

	return c;
}

const TMobSkillInfo* CHARACTER::GetMobSkill(unsigned int idx) const
{
	if (idx >= MOB_SKILL_MAX_NUM)
		return NULL;

	if (!m_pkMobData)
		return NULL;

	if (0 == m_pkMobData->m_table.Skills[idx].dwVnum)
		return NULL;

	return &m_pkMobData->m_mobSkillInfo[idx];
}

bool CHARACTER::CanUseMobSkill(unsigned int idx) const
{
	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	if (m_adwMobSkillCooltime[idx] > get_dword_time())
		return false;

	if (number(0, 1))
		return false;

	return true;
}

EVENTINFO(mob_skill_event_info)
{
	DynamicCharacterPtr ch;
	PIXEL_POSITION pos;
	DWORD vnum;
	int index;
	BYTE level;

	mob_skill_event_info()
		: ch()
		, pos()
		, vnum(0)
		, index(0)
		, level(0)
	{
	}
};

EVENTFUNC(mob_skill_hit_event)
{
	mob_skill_event_info* info = dynamic_cast<mob_skill_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("mob_skill_event_info> <Factor> Null pointer");
		return 0;
	}

	// <Factor>
	LPCHARACTER ch = info->ch;
	if (ch == NULL) {
		return 0;
	}

	ch->ComputeSkillAtPosition(info->vnum, info->pos, info->level);
	ch->m_mapMobSkillEvent.erase(info->index);

	return 0;
}

bool CHARACTER::UseMobSkill(unsigned int idx)
{
	if (IsPC())
		return false;

	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	DWORD dwVnum = pInfo->dwSkillVnum;
	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return false;

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, pInfo->bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->kCooldownPoly.SetVar("k", k);
	int iCooltime = (int)(pkSk->kCooldownPoly.Eval() * 1000);

	m_adwMobSkillCooltime[idx] = get_dword_time() + iCooltime;

	if (m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack.empty())
	{
		sys_err("No skill hit data for mob %s index %d", GetName(), idx);
		return false;
	}

	for (size_t i = 0; i < m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack.size(); i++)
	{
		PIXEL_POSITION pos = GetXYZ();
		const TMobSplashAttackInfo& rInfo = m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack[i];

		if (rInfo.dwHitDistance)
		{
			float fx, fy;
			GetDeltaByDegree(GetRotation(), rInfo.dwHitDistance, &fx, &fy);
			pos.x += (long)fx;
			pos.y += (long)fy;
		}

		if (rInfo.dwTiming)
		{
			mob_skill_event_info* info = AllocEventInfo<mob_skill_event_info>();

			info->ch = this;
			info->pos = pos;
			info->level = pInfo->bSkillLevel;
			info->vnum = dwVnum;
			info->index = i;

			// <Factor> Cancel existing event first
			itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.find(i);
			if (it != m_mapMobSkillEvent.end()) {
				LPEVENT existing = it->second;
				event_cancel(&existing);
				m_mapMobSkillEvent.erase(it);
			}

			m_mapMobSkillEvent.insert(std::make_pair(i, event_create(mob_skill_hit_event, info, PASSES_PER_SEC(rInfo.dwTiming) / 1000)));
		}
		else
		{
			ComputeSkillAtPosition(dwVnum, pos, pInfo->bSkillLevel);
		}
	}

	return true;
}

void CHARACTER::ResetMobSkillCooltime()
{
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
}

bool CHARACTER::IsUsableSkillMotion(DWORD dwMotionIndex) const
{
	DWORD selfJobGroup = (GetJob() + 1) * 10 + GetSkillGroup();
#ifdef ENABLE_WOLFMAN_CHARACTER
	const DWORD SKILL_NUM = 176;
#else
	const DWORD SKILL_NUM = 158;
#endif
	static DWORD s_anSkill2JobGroup[SKILL_NUM] = {
		0, // common_skill 0
		11, // job_skill 1
		11, // job_skill 2
		11, // job_skill 3
		11, // job_skill 4
		11, // job_skill 5
		11, // job_skill 6
		0, // common_skill 7
		0, // common_skill 8
		0, // common_skill 9
		0, // common_skill 10
		0, // common_skill 11
		0, // common_skill 12
		0, // common_skill 13
		0, // common_skill 14
		0, // common_skill 15
		12, // job_skill 16
		12, // job_skill 17
		12, // job_skill 18
		12, // job_skill 19
		12, // job_skill 20
		12, // job_skill 21
		0, // common_skill 22
		0, // common_skill 23
		0, // common_skill 24
		0, // common_skill 25
		0, // common_skill 26
		0, // common_skill 27
		0, // common_skill 28
		0, // common_skill 29
		0, // common_skill 30
		21, // job_skill 31
		21, // job_skill 32
		21, // job_skill 33
		21, // job_skill 34
		21, // job_skill 35
		21, // job_skill 36
		0, // common_skill 37
		0, // common_skill 38
		0, // common_skill 39
		0, // common_skill 40
		0, // common_skill 41
		0, // common_skill 42
		0, // common_skill 43
		0, // common_skill 44
		0, // common_skill 45
		22, // job_skill 46
		22, // job_skill 47
		22, // job_skill 48
		22, // job_skill 49
		22, // job_skill 50
		22, // job_skill 51
		0, // common_skill 52
		0, // common_skill 53
		0, // common_skill 54
		0, // common_skill 55
		0, // common_skill 56
		0, // common_skill 57
		0, // common_skill 58
		0, // common_skill 59
		0, // common_skill 60
		31, // job_skill 61
		31, // job_skill 62
		31, // job_skill 63
		31, // job_skill 64
		31, // job_skill 65
		31, // job_skill 66
		0, // common_skill 67
		0, // common_skill 68
		0, // common_skill 69
		0, // common_skill 70
		0, // common_skill 71
		0, // common_skill 72
		0, // common_skill 73
		0, // common_skill 74
		0, // common_skill 75
		32, // job_skill 76
		32, // job_skill 77
		32, // job_skill 78
		32, // job_skill 79
		32, // job_skill 80
		32, // job_skill 81
		0, // common_skill 82
		0, // common_skill 83
		0, // common_skill 84
		0, // common_skill 85
		0, // common_skill 86
		0, // common_skill 87
		0, // common_skill 88
		0, // common_skill 89
		0, // common_skill 90
		41, // job_skill 91
		41, // job_skill 92
		41, // job_skill 93
		41, // job_skill 94
		41, // job_skill 95
		41, // job_skill 96
		0, // common_skill 97
		0, // common_skill 98
		0, // common_skill 99
		0, // common_skill 100
		0, // common_skill 101
		0, // common_skill 102
		0, // common_skill 103
		0, // common_skill 104
		0, // common_skill 105
		42, // job_skill 106
		42, // job_skill 107
		42, // job_skill 108
		42, // job_skill 109
		42, // job_skill 110
		42, // job_skill 111
		0, // common_skill 112
		0, // common_skill 113
		0, // common_skill 114
		0, // common_skill 115
		0, // common_skill 116
		0, // common_skill 117
		0, // common_skill 118
		0, // common_skill 119
		0, // common_skill 120
		0, // common_skill 121
		0, // common_skill 122
		0, // common_skill 123
		0, // common_skill 124
		0, // common_skill 125
		0, // common_skill 126
		0, // common_skill 127
		0, // common_skill 128
		0, // common_skill 129
		0, // common_skill 130
		0, // common_skill 131
		0, // common_skill 132
		0, // common_skill 133
		0, // common_skill 134
		0, // common_skill 135
		0, // common_skill 136
		0, // job_skill 137
		0, // job_skill 138
		0, // job_skill 139
		0, // job_skill 140
		0, // common_skill 141
		0, // common_skill 142
		0, // common_skill 143
		0, // common_skill 144
		0, // common_skill 145
		0, // common_skill 146
		0, // common_skill 147
		0, // common_skill 148
		0, // common_skill 149
		0, // common_skill 150
		0, // common_skill 151
		0, // job_skill 152
		0, // job_skill 153
		0, // job_skill 154
		0, // job_skill 155
		0, // job_skill 156
		0, // job_skill 157
#ifdef ENABLE_WOLFMAN_CHARACTER
		0, // empty(reserved) 158
		0, // empty(reserved) 159
		0, // empty(reserved) 160
		0, // empty(reserved) 161
		0, // empty(reserved) 162
		0, // empty(reserved) 163
		0, // empty(reserved) 164
		0, // empty(reserved) 165
		0, // empty(reserved) 166
		0, // empty(reserved) 167
		0, // empty(reserved) 168
		0, // empty(reserved) 169
		51, // job_skill(WOLFMAN SKILL) 170
		51, // job_skill(WOLFMAN SKILL) 171
		51, // job_skill(WOLFMAN SKILL) 172
		51, // job_skill(WOLFMAN SKILL) 173
		51, // job_skill(WOLFMAN SKILL) 174
		51, // job_skill(WOLFMAN SKILL) 175
#endif
	}; // s_anSkill2JobGroup

#ifdef ENABLE_SKILLS_LEVEL_OVER_P
	const DWORD MOTION_MAX_NUM = 174;
#else
	const DWORD MOTION_MAX_NUM = 124;
#endif
#ifdef ENABLE_WOLFMAN_CHARACTER
	const DWORD SKILL_LIST_MAX_COUNT = 6;
#else
	const DWORD SKILL_LIST_MAX_COUNT = 5;
#endif
	static DWORD s_anMotion2SkillVnumList[MOTION_MAX_NUM][SKILL_LIST_MAX_COUNT] =
	{
		{   0,		0,			0,			0,			0		}, //  0

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  1
		{   5,		2,			32,			62,			92,	171		}, //  2
		{   5,		3,			33,			63,			93,	172		}, //  3
		{   5,		4,			34,			64,			94,	173		}, //  4
		{   5,		5,			35,			65,			95,	174		}, //  5
		{   5,		6,			36,			66,			96,	175		}, //  6
#else
		{   4,		1,			31,			61,			91		}, //  1
		{   4,		2,			32,			62,			92		}, //  2
		{   4,		3,			33,			63,			93		}, //  3
		{   4,		4,			34,			64,			94		}, //  4
		{   4,		5,			35,			65,			95		}, //  5
		{   4,		6,			36,			66,			96		}, //  6
#endif
		{   0,		0,			0,			0,			0		}, //  7
		{   0,		0,			0,			0,			0		}, //  8

		{   0,		0,			0,			0,			0		}, //  9
		{   0,		0,			0,			0,			0		}, //  10
		{   0,		0,			0,			0,			0		}, //  11
		{   0,		0,			0,			0,			0		}, //  12
		{   0,		0,			0,			0,			0		}, //  13
		{   0,		0,			0,			0,			0		}, //  14
		{   0,		0,			0,			0,			0		}, //  15

		{   4,		16,			46,			76,			106		}, //  16
		{   4,		17,			47,			77,			107		}, //  17
		{   4,		18,			48,			78,			108		}, //  18
		{   4,		19,			49,			79,			109		}, //  19
		{   4,		20,			50,			80,			110		}, //  20
		{   4,		21,			51,			81,			111		}, //  21
		{   0,		0,			0,			0,			0		}, //  22
		{   0,		0,			0,			0,			0		}, //  23

		{   0,		0,			0,			0,			0		}, //  24
		{   0,		0,			0,			0,			0		}, //  25

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  26
		{   5,		2,			32,			62,			92,	171		}, //  27
		{   5,		3,			33,			63,			93,	172		}, //  28
		{   5,		4,			34,			64,			94,	173		}, //  29
		{   5,		5,			35,			65,			95,	174		}, //  30
		{   5,		6,			36,			66,			96,	175		}, //  31
#else
		{   4,		1,			31,			61,			91		}, //  26
		{   4,		2,			32,			62,			92		}, //  27
		{   4,		3,			33,			63,			93		}, //  28
		{   4,		4,			34,			64,			94		}, //  29
		{   4,		5,			35,			65,			95		}, //  30
		{   4,		6,			36,			66,			96		}, //  31
#endif
		{   0,		0,			0,			0,			0		}, //  32
		{   0,		0,			0,			0,			0		}, //  33

		{   0,		0,			0,			0,			0		}, //  34
		{   0,		0,			0,			0,			0		}, //  35
		{   0,		0,			0,			0,			0		}, //  36
		{   0,		0,			0,			0,			0		}, //  37
		{   0,		0,			0,			0,			0		}, //  38
		{   0,		0,			0,			0,			0		}, //  39
		{   0,		0,			0,			0,			0		}, //  40

		{   4,		16,			46,			76,			106		}, //  41
		{   4,		17,			47,			77,			107		}, //  42
		{   4,		18,			48,			78,			108		}, //  43
		{   4,		19,			49,			79,			109		}, //  44
		{   4,		20,			50,			80,			110		}, //  45
		{   4,		21,			51,			81,			111		}, //  46
		{   0,		0,			0,			0,			0		}, //  47
		{   0,		0,			0,			0,			0		}, //  48

		{   0,		0,			0,			0,			0		}, //  49
		{   0,		0,			0,			0,			0		}, //  50

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  51
		{   5,		2,			32,			62,			92,	171		}, //  52
		{   5,		3,			33,			63,			93,	172		}, //  53
		{   5,		4,			34,			64,			94,	173		}, //  54
		{   5,		5,			35,			65,			95,	174		}, //  55
		{   5,		6,			36,			66,			96,	175		}, //  56
#else
		{   4,		1,			31,			61,			91		}, //  51
		{   4,		2,			32,			62,			92		}, //  52
		{   4,		3,			33,			63,			93		}, //  53
		{   4,		4,			34,			64,			94		}, //  54
		{   4,		5,			35,			65,			95		}, //  55
		{   4,		6,			36,			66,			96		}, //  56
#endif
		{   0,		0,			0,			0,			0		}, //  57
		{   0,		0,			0,			0,			0		}, //  58

		{   0,		0,			0,			0,			0		}, //  59
		{   0,		0,			0,			0,			0		}, //  60
		{   0,		0,			0,			0,			0		}, //  61
		{   0,		0,			0,			0,			0		}, //  62
		{   0,		0,			0,			0,			0		}, //  63
		{   0,		0,			0,			0,			0		}, //  64
		{   0,		0,			0,			0,			0		}, //  65

		{   4,		16,			46,			76,			106		}, //  66
		{   4,		17,			47,			77,			107		}, //  67
		{   4,		18,			48,			78,			108		}, //  68
		{   4,		19,			49,			79,			109		}, //  69
		{   4,		20,			50,			80,			110		}, //  70
		{   4,		21,			51,			81,			111		}, //  71
		{   0,		0,			0,			0,			0		}, //  72
		{   0,		0,			0,			0,			0		}, //  73

		{   0,		0,			0,			0,			0		}, //  74
		{   0,		0,			0,			0,			0		}, //  75

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  76
		{   5,		2,			32,			62,			92,	171		}, //  77
		{   5,		3,			33,			63,			93,	172		}, //  78
		{   5,		4,			34,			64,			94,	173		}, //  79
		{   5,		5,			35,			65,			95,	174		}, //  80
		{   5,		6,			36,			66,			96,	175		}, //  81
#else
		{   4,		1,			31,			61,			91		}, //  76
		{   4,		2,			32,			62,			92		}, //  77
		{   4,		3,			33,			63,			93		}, //  78
		{   4,		4,			34,			64,			94		}, //  79
		{   4,		5,			35,			65,			95		}, //  80
		{   4,		6,			36,			66,			96		}, //  81
#endif
		{   0,		0,			0,			0,			0		}, //  82
		{   0,		0,			0,			0,			0		}, //  83

		{   0,		0,			0,			0,			0		}, //  84
		{   0,		0,			0,			0,			0		}, //  85
		{   0,		0,			0,			0,			0		}, //  86
		{   0,		0,			0,			0,			0		}, //  87
		{   0,		0,			0,			0,			0		}, //  88
		{   0,		0,			0,			0,			0		}, //  89
		{   0,		0,			0,			0,			0		}, //  90

		{   4,		16,			46,			76,			106		}, //  91
		{   4,		17,			47,			77,			107		}, //  92
		{   4,		18,			48,			78,			108		}, //  93
		{   4,		19,			49,			79,			109		}, //  94
		{   4,		20,			50,			80,			110		}, //  95
		{   4,		21,			51,			81,			111		}, //  96
		{   0,		0,			0,			0,			0		}, //  97
		{   0,		0,			0,			0,			0		}, //  98

		{   0,		0,			0,			0,			0		}, //  99
		{   0,		0,			0,			0,			0		}, //  100

#ifdef ENABLE_SKILLS_LEVEL_OVER_P
		//Sage
		#ifdef ENABLE_WOLFMAN_CHARACTER
				{ 5, 1, 31, 61, 91, 170 },//101
				{ 5, 2, 32, 62, 92, 171 },//102
				{ 5, 3, 33, 63, 93, 172 },//103
				{ 5, 4, 34, 64, 94, 173 },//104
				{ 5, 5, 35, 65, 95, 174 },//105
				{ 5, 6, 36, 66, 96, 175 },//106
		#else
				{ 4, 1, 31, 61, 91 },
				{ 4, 2, 32, 62, 92 },
				{ 4, 3, 33, 63, 93 },
				{ 4, 4, 34, 64, 94 },
				{ 4, 5, 35, 65, 95 },
				{ 4, 6, 36, 66, 96 },
		#endif
				{ 0, 0, 0, 0, 0 },//107
				{ 0, 0, 0, 0, 0 },//108
				{ 0, 0, 0, 0, 0 },//109
				{ 0, 0, 0, 0, 0 },//110
				{ 0, 0, 0, 0, 0 },//111
				{ 0, 0, 0, 0, 0 },//112
				{ 0, 0, 0, 0, 0 },//113
				{ 0, 0, 0, 0, 0 },//114
				{ 0, 0, 0, 0, 0 },//115

				{ 4, 16, 46, 76, 106 },//116
				{ 4, 17, 47, 77, 107 },//117
				{ 4, 18, 48, 78, 108 },//118
				{ 4, 19, 49, 79, 109 },//119
				{ 4, 20, 50, 80, 110 },//120
				{ 4, 21, 51, 81, 111 },//121

				{ 0, 0, 0, 0, 0 },//122
				{ 0, 0, 0, 0, 0 },//123
				{ 0, 0, 0, 0, 0 },//124
				{ 0, 0, 0, 0, 0 },//125
				//legendary
	#ifdef ENABLE_WOLFMAN_CHARACTER
			{ 5, 1, 31, 61, 91, 170 },//126
			{ 5, 2, 32, 62, 92, 171 },//127
			{ 5, 3, 33, 63, 93, 172 },//128
			{ 5, 4, 34, 64, 94, 173 },//129
			{ 5, 5, 35, 65, 95, 174 },//130
			{ 5, 6, 36, 66, 96, 175 },//131
	#else
			{ 4, 1, 31, 61, 91 },
			{ 4, 2, 32, 62, 92 },
			{ 4, 3, 33, 63, 93 },
			{ 4, 4, 34, 64, 94 },
			{ 4, 5, 35, 65, 95 },
			{ 4, 6, 36, 66, 96 },
	#endif
			{ 0, 0, 0, 0, 0 },//132
			{ 0, 0, 0, 0, 0 },//133
			{ 0, 0, 0, 0, 0 },//134
			{ 0, 0, 0, 0, 0 },//135
			{ 0, 0, 0, 0, 0 },//136
			{ 0, 0, 0, 0, 0 },//137
			{ 0, 0, 0, 0, 0 },//138
			{ 0, 0, 0, 0, 0 },//139
			{ 0, 0, 0, 0, 0 },//140

			{ 4, 16, 46, 76, 106 },//141
			{ 4, 17, 47, 77, 107 },//142
			{ 4, 18, 48, 78, 108 },//143
			{ 4, 19, 49, 79, 109 },//144
			{ 4, 20, 50, 80, 110 },//145
			{ 4, 21, 51, 81, 111 },//146

			{ 0, 0, 0, 0, 0 },//147
			{ 0, 0, 0, 0, 0 },//148
			{ 0, 0, 0, 0, 0 },//149
			{ 0, 0, 0, 0, 0 },//150
	#endif

			{   1,  152,    0,    0,    0}, //  101
			{   1,  153,    0,    0,    0}, //  102
			{   1,  154,    0,    0,    0}, //  103
			{   1,  155,    0,    0,    0}, //  104
			{   1,  156,    0,    0,    0}, //  105
			{   1,  157,    0,    0,    0}, //  106

			{   0,    0,    0,    0,    0}, //  107
			{   0,    0,    0,    0,    0}, //  108
			{   0,    0,    0,    0,    0}, //  109
			{   0,    0,    0,    0,    0}, //  110
			{   0,    0,    0,    0,    0}, //  111
			{   0,    0,    0,    0,    0}, //  112
			{   0,    0,    0,    0,    0}, //  113
			{   0,    0,    0,    0,    0}, //  114
			{   0,    0,    0,    0,    0}, //  115
			{   0,    0,    0,    0,    0}, //  116
			{   0,    0,    0,    0,    0}, //  117
			{   0,    0,    0,    0,    0}, //  118
			{   0,    0,    0,    0,    0}, //  119
			{   0,    0,    0,    0,    0}, //  120

			{   2,  137,  140,    0,    0}, //  121
			{   1,  138,    0,    0,    0}, //  122
			{   1,  139,    0,    0,    0}, //  123
	};

	if (dwMotionIndex >= MOTION_MAX_NUM)
	{
		sys_err("OUT_OF_MOTION_VNUM: name=%s, motion=%d/%d", GetName(), dwMotionIndex, MOTION_MAX_NUM);
		return false;
	}

	DWORD* skillVNums = s_anMotion2SkillVnumList[dwMotionIndex];

	DWORD skillCount = *skillVNums++;
	if (skillCount >= SKILL_LIST_MAX_COUNT)
	{
		sys_err("OUT_OF_SKILL_LIST: name=%s, count=%d/%d", GetName(), skillCount, SKILL_LIST_MAX_COUNT);
		return false;
	}

	for (DWORD skillIndex = 0; skillIndex != skillCount; ++skillIndex)
	{
		if (skillIndex >= SKILL_MAX_NUM)
		{
			sys_err("OUT_OF_SKILL_VNUM: name=%s, skill=%d/%d", GetName(), skillIndex, SKILL_MAX_NUM);
			return false;
		}

		DWORD eachSkillVNum = skillVNums[skillIndex];
		if (eachSkillVNum != 0)
		{
			DWORD eachJobGroup = s_anSkill2JobGroup[eachSkillVNum];

			if (0 == eachJobGroup || eachJobGroup == selfJobGroup)
			{
				// GUILDSKILL_BUG_FIX
				DWORD eachSkillLevel = 0;

				if (eachSkillVNum >= GUILD_SKILL_START && eachSkillVNum <= GUILD_SKILL_END)
				{
					if (GetGuild())
						eachSkillLevel = GetGuild()->GetSkillLevel(eachSkillVNum);
					else
						eachSkillLevel = 0;
				}
				else
				{
					eachSkillLevel = GetSkillLevel(eachSkillVNum);
				}

				if (eachSkillLevel > 0)
				{
					return true;
				}
				// END_OF_GUILDSKILL_BUG_FIX
			}
		}
	}

	return false;
}

void CHARACTER::ClearSkill()
{
	PointChange(POINT_SKILL, 4 + (GetLevel() - 5) - GetPoint(POINT_SKILL));
	// @fixme234 BEGIN
	RemoveAffect(SKILL_JEONGWI);
	RemoveAffect(SKILL_GEOMKYUNG);
	RemoveAffect(SKILL_CHUNKEON);
	RemoveAffect(SKILL_EUNHYUNG);
	RemoveAffect(SKILL_GYEONGGONG);
	RemoveAffect(SKILL_GWIGEOM);
	RemoveAffect(SKILL_TERROR);
	RemoveAffect(SKILL_JUMAGAP);
	RemoveAffect(SKILL_MANASHILED);
	RemoveAffect(SKILL_HOSIN);
	RemoveAffect(SKILL_REFLECT);
	RemoveAffect(SKILL_KWAESOK);
	RemoveAffect(SKILL_JEUNGRYEOK);
	RemoveAffect(SKILL_GICHEON);
	RemoveAffect(SKILL_JEOKRANG);
	RemoveAffect(SKILL_CHEONGRANG);
	// @fixme234 END
	ResetSkill();
}

void CHARACTER::ClearSubSkill()
{
	PointChange(POINT_SUB_SKILL, GetLevel() < 10 ? 0 : (GetLevel() - 9) - GetPoint(POINT_SUB_SKILL));

	if (m_pSkillLevels == NULL)
	{
		sys_err("m_pSkillLevels nil (name: %s)", GetName());
		return;
	}

	TPlayerSkill CleanSkill;
	memset(&CleanSkill, 0, sizeof(TPlayerSkill));

	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		m_pSkillLevels[s_adwSubSkillVnums[i]] = CleanSkill;
	}

	ComputePoints();
	SkillLevelPacket();
}

bool CHARACTER::ResetOneSkill(DWORD dwVnum)
{
	if (NULL == m_pSkillLevels)
	{
		sys_err("m_pSkillLevels nil (name %s, vnum %u)", GetName(), dwVnum);
		return false;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (name %s, vnum %u)", GetName(), dwVnum);
		return false;
	}

	BYTE level = m_pSkillLevels[dwVnum].bLevel;

	m_pSkillLevels[dwVnum].bLevel = 0;
	m_pSkillLevels[dwVnum].bMasterType = 0;
#ifdef ENABLE_SKILL_DESCRIPTION_RENEWAL
	m_pSkillLevels[dwVnum].bBooks = 0;
#endif
	if (level > 17)
		level = 17;

	PointChange(POINT_SKILL, level);

	ComputePoints();
	SkillLevelPacket();

	return true;
}

eMountType GetMountLevelByVnum(DWORD dwMountVnum, bool IsNew) // updated to 2014/12/10
{
	if (!dwMountVnum)
		return MOUNT_TYPE_NONE;

	switch (dwMountVnum)
	{
		// ### YES SKILL
		// @fixme116 begin
	case 20107: // normal military horse (no guild)
	case 20108: // normal military horse (guild member)
	case 20109: // normal military horse (guild master)
		if (IsNew)
			return MOUNT_TYPE_NONE;
		// @fixme116 end
		// Classic
	case 20110: // Classic Boar
	case 20111: // Classic Wolf
	case 20112: // Classic Tiger
	case 20113: // Classic Lion
	case 20114: // White Lion
		// Special Lv2
	case 20115: // Wild Battle Boar
	case 20116: // Fight Wolf
	case 20117: // Storm Tiger
	case 20118: // Battle Lion (bugged)
	case 20205: // Wild Battle Boar (alternative)
	case 20206: // Fight Wolf (alternative)
	case 20207: // Storm Tiger (alternative)
	case 20208: // Battle Lion (bugged) (alternative)
		// Royal Tigers
	case 20120: // blue
	case 20121: // dark red
	case 20122: // gold
	case 20123: // green
	case 20124: // pied
	case 20125: // white
		// Royal mounts (Special Lv3)
	case 20209: // Royal Boar
	case 20210: // Royal Wolf
	case 20211: // Royal Tiger
	case 20212: // Royal Lion
		//
	case 20215: // Rudolph m Lv3 (yes skill, yes atk)
	case 20218: // Rudolph f Lv3 (yes skill, yes atk)
	case 20225: // Dyno Lv3 (yes skill, yes atk)
	case 20230: // Turkey Lv3 (yes skill, yes atk)
		return MOUNT_TYPE_MILITARY;
		break;
		// ### NO SKILL YES ATK
		// @fixme116 begin
	case 20104: // normal combat horse (no guild)
	case 20105: // normal combat horse (guild member)
	case 20106: // normal combat horse (guild master)
		if (IsNew)
			return MOUNT_TYPE_NONE;
		// @fixme116 end
	case 20119: // Black Horse (no skill, yes atk)
	case 20214: // Rudolph m Lv2 (no skill, yes atk)
	case 20217: // Rudolph f Lv2 (no skill, yes atk)
	case 20219: // Equus Porphyreus (no skill, yes atk)
	case 20220: // Comet (no skill, yes atk)
	case 20221: // Polar Predator (no skill, yes atk)
	case 20222: // Armoured Panda (no skill, yes atk)
	case 20224: // Dyno Lv2 (no skill, yes atk)
	case 20226: // Nightmare (no skill, yes atk)
	case 20227: // Unicorn (no skill, yes atk)
	case 20229: // Turkey Lv2 (no skill, yes atk)
	case 20231: // Leopard (no skill, yes atk)
	case 20232: // Black Panther (no skill, yes atk)
		return MOUNT_TYPE_COMBAT;
		break;
		// ### NO SKILL NO ATK
		// @fixme116 begin
	case 20101: // normal beginner horse (no guild)
	case 20102: // normal beginner horse (guild member)
	case 20103: // normal beginner horse (guild master)
		if (IsNew)
			return MOUNT_TYPE_NONE;
		// @fixme116 end
	case 20213: // Rudolph m Lv1 (no skill, no atk)
	case 20216: // Rudolph f Lv1 (no skill, no atk)
		// Special Lv1
	case 20201: // Boar Lv1 (no skill, no atk)
	case 20202: // Wolf Lv1 (no skill, no atk)
	case 20203: // Tiger Lv1 (no skill, no atk)
	case 20204: // Lion Lv1 (no skill, no atk)
		//
	case 20223: // Dyno Lv1 (no skill, no atk)
	case 20228: // Turkey Lv1 (no skill, no atk)
		return MOUNT_TYPE_NORMAL;
		break;
	default:
		return MOUNT_TYPE_NONE;
		break;
	}
}

const int SKILL_COUNT = 6;
static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][SKILL_COUNT] =
{
	{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} },
	{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} },
	{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} },
	{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} },
#ifdef ENABLE_WOLFMAN_CHARACTER
	{ {	170,171,172,173,174,175	}, {	0,	0,	0,	0,	0,	0	} },
#endif
};

const DWORD GetRandomSkillVnum(BYTE bJob)
{
	// the chosen skill
	DWORD dwSkillVnum = 0;
	do
	{
		// tmp stuff
		DWORD tmpJob = (bJob != JOB_MAX_NUM) ? MINMAX(0, bJob, JOB_MAX_NUM - 1) : number(0, JOB_MAX_NUM - 1);
		DWORD tmpSkillGroup = number(0, SKILL_GROUP_MAX_NUM - 1);
		DWORD tmpSkillCount = number(0, SKILL_COUNT - 1);
		// set skill
		dwSkillVnum = SkillList[tmpJob][tmpSkillGroup][tmpSkillCount];

#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_WOLFMAN_BOOKS)
		if (tmpJob == JOB_WOLFMAN)
			continue;
#endif

		if (dwSkillVnum != 0 && NULL != CSkillManager::instance().Get(dwSkillVnum))
			break;
	} while (true);
	return dwSkillVnum;
}

bool CHARACTER::CanUseSkill(DWORD dwSkillVnum) const
{
	if (0 == dwSkillVnum) return false;

	if (0 < GetSkillGroup())
	{
		const DWORD* pSkill = SkillList[GetJob()][GetSkillGroup() - 1];

		for (int i = 0; i < SKILL_COUNT; ++i)
		{
			if (pSkill[i] == dwSkillVnum) return true;
		}
	}

	//if (true == IsHorseRiding())

	if (true == IsRiding())
	{
#ifndef ENABLE_NO_MOUNT_CHECK
		eMountType eIsMount = GetMountLevelByVnum(GetMountVnum(), false);
		if (eIsMount != MOUNT_TYPE_MILITARY)
		{
			return false;
		}
#endif
		switch (dwSkillVnum)
		{
		case SKILL_HORSE_WILDATTACK:
		case SKILL_HORSE_CHARGE:
		case SKILL_HORSE_ESCAPE:
		case SKILL_HORSE_WILDATTACK_RANGE:
			return true;
		}
	}

	switch (dwSkillVnum)
	{
	case 121: case 122: case 124: case 126: case 127: case 128: case 129: case 130:
	case 131:
	case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
		return true;
	}

	return false;
}

bool CHARACTER::CheckSkillHitCount(const BYTE SkillID, const VID TargetVID)
{
	std::map<int, TSkillUseInfo>::iterator iter = m_SkillUseInfo.find(SkillID);

	if (iter == m_SkillUseInfo.end())
	{
		return false;
	}

	TSkillUseInfo& rSkillUseInfo = iter->second;

	if (false == rSkillUseInfo.bUsed)
	{
		return false;
	}

	switch (SkillID)
	{
	case SKILL_YONGKWON:
	case SKILL_HWAYEOMPOK:
	case SKILL_DAEJINGAK:
	case SKILL_PAERYONG:
		return false;
	}

	target_map::iterator iterTargetMap = rSkillUseInfo.TargetVIDMap.find(TargetVID);

	if (rSkillUseInfo.TargetVIDMap.end() != iterTargetMap)
	{
		size_t MaxAttackCountPerTarget = 1;

		switch (SkillID)
		{
		case SKILL_SAMYEON:
		case SKILL_CHARYUN:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case SKILL_CHAYEOL:
#endif
			MaxAttackCountPerTarget = 3;
			break;

		case SKILL_HORSE_WILDATTACK_RANGE:
			MaxAttackCountPerTarget = 5;
			break;

		case SKILL_YEONSA:
			MaxAttackCountPerTarget = 7;
			break;

		case SKILL_HORSE_ESCAPE:
			MaxAttackCountPerTarget = 10;
			break;
		}

		if (iterTargetMap->second >= MaxAttackCountPerTarget)
		{
			return false;
		}

		iterTargetMap->second++;
	}
	else
	{
		rSkillUseInfo.TargetVIDMap.insert(std::make_pair(TargetVID, 1));
	}

	return true;
}

bool CHARACTER::LearnGrandMasterSkill(DWORD dwSkillVnum)
{
	CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);
	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		NewChatPacket(SKILL_NOT_UPGRADE_ERROR);
		return false;
	}


	if (GetSkillMasterType(dwSkillVnum) != SKILL_GRAND_MASTER)
		return false;

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "skill.grand" << dwSkillVnum;
		strTrainSkill = os.str();
	}
	int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 30);
	int iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
	SetQuestFlag(strTrainSkill, iTotalReadCount);

	int iMinReadCount = MinReadSoulStone[idx];
	int iMaxReadCount = MaxReadSoulStone[idx];
	int iBookCount = ReadSoulStone[idx];
	bool bSuccess = false;
	int n = 0;

	if (idx >= 7)
		n = number(iMinReadCount, iMaxReadCount);
	else
		n = number(iMinReadCount, iMaxReadCount - 2);

	if (n == iBookCount)
		bSuccess = true;

	if (iTotalReadCount < iMinReadCount)
		bSuccess = false;
	if (iTotalReadCount > iMaxReadCount)
		bSuccess = true;

	if (bSuccess)
	{
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
		if (GetSkillLevel(dwSkillVnum) == 40)//delete flag
			SetQuestFlag(strTrainSkill, 0);
		NewChatPacket(SKILL_UPGRADE_SUCCESS);
	}
	else
		NewChatPacket(SKILL_UPGRADE_FAIL);

	return true;
}
// END_OF_ADD_GRANDMASTER_SKILL

#ifdef ENABLE_SKILLS_LEVEL_OVER_P
bool CHARACTER::LearnSageMasterSkill(DWORD dwSkillVnum)
{
	CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);
	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		NewChatPacket(SKILL_NOT_UPGRADE_ERROR);
		return false;
	}


	if (GetSkillMasterType(dwSkillVnum) != SKILL_PERFECT_MASTER)
		return false;

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "skill.sage" << dwSkillVnum;
		strTrainSkill = os.str();
	}
	int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 40);
	int iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
	SetQuestFlag(strTrainSkill, iTotalReadCount);

	int iMinReadCount = MinReadSoulStone[idx];
	int iMaxReadCount = MaxReadSoulStone[idx];
	int iBookCount = ReadSoulStone[idx];
	bool bSuccess = false;
	int n = 0;

	if (idx >= 7)
		n = number(iMinReadCount, iMaxReadCount);
	else
		n = number(iMinReadCount, iMaxReadCount - 2);

	if (n == iBookCount)
		bSuccess = true;

	if (iTotalReadCount < iMinReadCount)
		bSuccess = false;
	if (iTotalReadCount > iMaxReadCount)
		bSuccess = true;

	if (bSuccess)
	{
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
		if (GetSkillLevel(dwSkillVnum) == 50)//delete flag
		{
			SetQuestFlag(strTrainSkill, 0);
#ifdef ENABLE_SKILL_SET_BONUS
			SkillSetBonusCalcute(true);
#endif
		}
		NewChatPacket(SKILL_UPGRADE_SUCCESS);
	}
	else
		NewChatPacket(SKILL_UPGRADE_FAIL);

	return true;
}

bool CHARACTER::LearnLegendaryMasterSkill(DWORD dwSkillVnum)
{
	CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);
	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		NewChatPacket(SKILL_NOT_UPGRADE_ERROR);
		return false;
	}

	if (GetSkillMasterType(dwSkillVnum) != SKILL_SAGE_MASTER)
		return false;

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "skill.legendary" << dwSkillVnum;
		strTrainSkill = os.str();
	}

	int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 50);
	int iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
	SetQuestFlag(strTrainSkill, iTotalReadCount);

	int iMinReadCount = MinReadSoulStone[idx];
	int iMaxReadCount = MaxReadSoulStone[idx];
	int iBookCount = ReadSoulStone[idx];
	bool bSuccess = false;
	int n = 0;

	if (idx >= 7)
		n = number(iMinReadCount, iMaxReadCount);
	else
		n = number(iMinReadCount, iMaxReadCount - 2);

	if (n == iBookCount)
		bSuccess = true;

	if (iTotalReadCount < iMinReadCount)
		bSuccess = false;
	if (iTotalReadCount > iMaxReadCount)
		bSuccess = true;

	if (bSuccess)
	{
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
		if (GetSkillLevel(dwSkillVnum) == 60)//delete flag
		{
			SetQuestFlag(strTrainSkill, 0);
#ifdef ENABLE_SKILL_SET_BONUS
			SkillSetBonusCalcute(true);
#endif
#ifdef ENABLE_CHALLENGE_SYSTEM
			if (!(dwSkillVnum >= 200 && dwSkillVnum <= 207))
			{
				if (!CChallenge::Instance().GetWinner(CHALLENGE_QUEST_NAME_39))
				{
					CChallenge::Instance().SetWinner(CHALLENGE_QUEST_NAME_39, this);
				}
			}
#endif
		}

		NewChatPacket(SKILL_UPGRADE_SUCCESS);
	}
	else
		NewChatPacket(SKILL_UPGRADE_FAIL);

	return true;
}

bool CHARACTER::LearnPerfectMasterSkill(DWORD dwSkillVnum)
{
	CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);
	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		NewChatPacket(SKILL_NOT_UPGRADE_ERROR);
		return false;
	}

	if (GetSkillMasterType(dwSkillVnum) != SKILL_GRAND_MASTER)
		return false;

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "skill.perfect" << dwSkillVnum;
		strTrainSkill = os.str();
	}

	int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 30);
	int iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
	SetQuestFlag(strTrainSkill, iTotalReadCount);

	int iMinReadCount = MinReadSoulStone[idx];
	int iMaxReadCount = MaxReadSoulStone[idx];
	int iBookCount = ReadSoulStone[idx];
	bool bSuccess = false;
	int n = 0;

	if (idx >= 7)
		n = number(iMinReadCount, iMaxReadCount);
	else
		n = number(iMinReadCount, iMaxReadCount - 1);

	if (n == iBookCount || n == (iBookCount + 1) || n == (iBookCount - 1))
		bSuccess = true;

	if (iTotalReadCount < iMinReadCount)
		bSuccess = false;
	if (iTotalReadCount > iMaxReadCount)
		bSuccess = true;

	if (bSuccess)
	{
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
		if (GetSkillLevel(dwSkillVnum) == 40)//delete flag
		{
			SetQuestFlag(strTrainSkill, 0);
#ifdef ENABLE_SKILL_SET_BONUS
			SkillSetBonusCalcute(true);
#endif
		}
		NewChatPacket(SKILL_UPGRADE_SUCCESS);
	}
	else
		NewChatPacket(SKILL_UPGRADE_FAIL);

	return true;
}
#endif

#ifdef ENABLE_PASSIVE_SKILLS
DWORD CHARACTER::GetPassiveReadBookVnum(BYTE skillIdx)
{
	switch (skillIdx)
	{
		case 121: {return 24010; }//liderlik
		case 124: {return 24011; }//maden
		case 200: {return 24002; }//canavar
		case 201: {return 24003; }//metin
		case 202: {return 24004; }//berserker
		case 203: {return 24005; }//insan
		case 204: {return 24006; }//beceri sure
		case 205: {return 24007; }//hp
		case 206: {return 24008; }//sebnem bonus
		case 207: {return 24009; }//nesne yaratma
		case 208: {return 24012; }//balik
	}
	return 0;
}

void CHARACTER::PassiveSkillUpgrade(BYTE skillIdx, bool allgive)
{
	if (IsPolymorphed())
	{
		NewChatPacket(POLY_BLOCKED_WITH_THIS);
		return;
	}

	if (!GetSkillGroup())//lightwork skillbook with no skillgroup fixed
	{
		NewChatPacket(YOU_HAVE_NO_SKILL_GROUP);
		return;
	}

	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	if (!pPC)
		return;

	BYTE skilllevel = GetSkillLevel(skillIdx);
	bool update = false;
	if (skilllevel == 50)
		return;

	BYTE readCount = PassiveReadBook[skilllevel];
	DWORD bookVnum = GetPassiveReadBookVnum(skillIdx);
	if (bookVnum == 0)
		return;

	bool sage = GetSkillMasterType(skillIdx) == SKILL_SAGE_MASTER;
	int ruhVnum = sage ? 24013 : 71001;
	int munzeviVnum = sage ? 24014 : 71094;

	if (!CountSpecifyItemText(bookVnum, 1))
		return;
	if (!CountSpecifyItemText(ruhVnum, 1))
		return;
	if (!CountSpecifyItemText(munzeviVnum, 1))
		return;

	std::string skillFlag;
	{
		std::ostringstream os;
		os << "skill.passive" << skillIdx;
		skillFlag = os.str();
	}

	if (allgive)
	{
		int totalReadingCount = 0;
		do
		{
			bookVnum = GetPassiveReadBookVnum(skillIdx);
			if (!CountSpecifyItemText(bookVnum, 1))
				break;
			if (!CountSpecifyItemText(munzeviVnum, 1))
				break;
			if (!CountSpecifyItemText(ruhVnum, 1))
				break;

			int totalReadCount = pPC->GetFlag(skillFlag);
			readCount = PassiveReadBook[skilllevel];
			
			int increasseReading = readCount - totalReadCount;

			{
				int count1 = CountSpecifyItem(71001);
				int count2 = CountSpecifyItem(71094);
				int bookCount = CountSpecifyItem(bookVnum);

				int itemCount = count1 > count2 ? count2 : count1;
				itemCount = itemCount > bookCount ? bookCount : itemCount;

				increasseReading = itemCount > increasseReading ? increasseReading : itemCount;
				totalReadCount += increasseReading;
				totalReadingCount += increasseReading;
			}

			RemoveSpecifyItem(ruhVnum, increasseReading);
			RemoveSpecifyItem(munzeviVnum, increasseReading);
			RemoveSpecifyItem(bookVnum, increasseReading);
			
			if (readCount <= totalReadCount)
			{
				SetSkillLevel(skillIdx, GetSkillLevel(skillIdx) + 1);
				pPC->DeleteFlag(skillFlag);
				skilllevel++;
				update = true;
			}
			else
			{
				pPC->SetFlag(skillFlag, totalReadCount);
			}
		} while (skilllevel < 50);

		NewChatPacket(PASSIVE_SKILL_TALK_5, "%d", totalReadingCount);
	}
	else
	{
		RemoveSpecifyItem(munzeviVnum, 1);
		RemoveSpecifyItem(ruhVnum, 1);
		RemoveSpecifyItem(bookVnum, 1);

		int totalReadCount = pPC->GetFlag(skillFlag) + 1;

		if (readCount <= totalReadCount)
		{
			SetSkillLevel(skillIdx, GetSkillLevel(skillIdx) + 1);
			pPC->DeleteFlag(skillFlag);
			update = true;
		}
		else
		{
			pPC->SetFlag(skillFlag, totalReadCount);
			int readingcount = readCount - totalReadCount;
			NewChatPacket(PASSIVE_SKILL_TALK_4, "%d", readingcount);
		}
	}
	if (update)
	{
		Save();
		ComputePoints();
		SkillLevelPacket();
	}
}

void CHARACTER::ComputeNewPassiveSkills()
{
	for (BYTE skillIndex = SKILL_PASSIVE_MONSTER; skillIndex <= SKILL_PASSIVE_FISHING; ++skillIndex)
		ComputePassiveSkill(skillIndex);
}
#endif

#ifdef ENABLE_SKILL_SET_BONUS
void CHARACTER::SkillSetBonusCalcute(bool update)
{
	int oldsetbonus = 0;
	if (update == true)
	{
		oldsetbonus = GetSkillSetBonus();
	}
	int skillgrup = GetSkillGroup();
	if (skillgrup == 0)
		return;
	std::vector <int> SkillIndex;
	BYTE job = GetJob();
	int sagecount = 0;
	int legendarycount = 0;
	switch (job)
	{
	case JOB_WARRIOR:
	{
		if (skillgrup == 1)
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(1 + i);
			}
		}
		else
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(16 + i);
			}
		}
		break;
	}
	case JOB_ASSASSIN:
	{
		if (skillgrup == 1)
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(31 + i);
			}
		}
		else
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(46 + i);
			}
		}
		break;
	}
	case JOB_SURA:
	{
		if (skillgrup == 1)
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(61 + i);
			}
		}
		else
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(76 + i);
			}
		}
		break;
	}
	case JOB_SHAMAN:
	{
		if (skillgrup == 1)
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(91 + i);
			}
		}
		else
		{
			for (int i = 0; i < 6; i++)
			{
				SkillIndex.push_back(106 + i);
			}
		}
		break;
	}
	case JOB_WOLFMAN:
	{
		for (int i = 0; i < 6; i++)
		{
			SkillIndex.push_back(170 + i);
		}
		break;
	}
	default: break;
	}

	for (int i = 0; i < 6; i++)
	{
		int mastertype = GetSkillMasterType(SkillIndex[i]);
		if (mastertype == SKILL_SAGE_MASTER)
			sagecount++;
		else if (mastertype == SKILL_LEGENDARY_MASTER)
			legendarycount++;
	}

	if (legendarycount == 6)
	{
		SetSkillSetBonus(2);
	}
	else if ((sagecount + legendarycount) == 6)
	{
		SetSkillSetBonus(1);
	}
	else
	{
		SetSkillSetBonus(0);
	}
	if (update == true)
	{
		if (oldsetbonus != GetSkillSetBonus())
			ComputePoints();
	}
}

void CHARACTER::SkillSetBonusGiveBuff()
{
	if (GetSkillSetBonus() == 0)
		return;

	else if (GetSkillSetBonus() == 1)
	{
		PointChange(POINT_MAX_HP, 1000);
		PointChange(POINT_ATTBONUS_HUMAN, 10);

		PointChange(POINT_ATTBONUS_MONSTER, 10);
		PointChange(POINT_ATTBONUS_STONE, 5);
		PointChange(POINT_ATTBONUS_BOSS, 5);
	}
	else if (GetSkillSetBonus() == 2)
	{
		PointChange(POINT_ATTBONUS_MONSTER, 1000);
		PointChange(POINT_ATTBONUS_WARRIOR, 250);
		PointChange(POINT_MELEE_MAGIC_ATT_BONUS_PER, 10);
	}
}
#endif

#ifdef ENABLE_WOLFMAN_CHARACTER
void CHARACTER::WolfmanSkillSet()
{
	SetSkillLevel(121, 20);
	SetSkillLevel(124, 20);
	SetSkillLevel(125, 20);
	SetSkillLevel(129, 20);
	SetSkillLevel(132, 20);
	SetSkillLevel(133, 20);
	SetSkillLevel(134, 20);
	SetSkillLevel(135, 20);
	SetSkillLevel(136, 20);
	SetSkillLevel(137, 20);
	SetSkillLevel(138, 20);
	SetSkillLevel(130, 20);
	SetSkillLevel(131, 10);
	SetSkillLevel(122, 2);

	for (int i = 170; i < 176; i++)
	{
		SetSkillLevel(i, 30);
	}

	SkillLevelPacket();

}
#endif

#ifdef ENABLE_PASSIVE_SKILLS
long CHARACTER::GetPassiveCooldDown(BYTE skillIdx)
{
	long duration = 0;
	switch (skillIdx)
	{
		case SKILL_JEONGWI: // ofke
		case SKILL_GEOMKYUNG: // hava kilici
		case SKILL_CHUNKEON: // guclu beden

		case SKILL_GWIGEOM: // keskinlik
		case SKILL_TERROR: // dehset
		case SKILL_JUMAGAP: // buyulu zirh

		case SKILL_MUYEONG: // ates hayaleti
		case SKILL_MANASHILED: // karanlik koruma

		case SKILL_GYEONGGONG: // hafif adim
		case SKILL_EUNHYUNG: // kamuflaj

	#if defined(ENABLE_WOLFMAN_CHARACTER)
		case SKILL_CHEONGRANG: // kirmizi kurt ruhu
	#endif
		case SKILL_REFLECT: // yansitma
		case SKILL_GICHEON: // ejderha yardimi
		case SKILL_KWAESOK: // cabukluk
		case SKILL_JEUNGRYEOK: // yuksek saldiri
		{
			duration = GetSkillLevel(SKILL_PASSIVE_SKILL_COOLDOWN) * 72;
			break;
		}
		default: break;
	}
	duration += 1800;
	return duration;
}

#endif