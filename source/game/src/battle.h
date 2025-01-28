#ifndef __INC_METIN_II_GAME_BATTLE_H__
#define __INC_METIN_II_GAME_BATTLE_H__

#include "char.h"

enum EBattleTypes
{
	BATTLE_NONE,
	BATTLE_DAMAGE,
	BATTLE_DEFENSE,
	BATTLE_DEAD
};

extern DAM_LL	CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, DAM_LL iAtk);
extern DAM_LL	CalcBattleDamage(DAM_LL iDam, int iAttackerLev, int iVictimLev);
extern DAM_LL	CalcMeleeDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim, bool bIgnoreDefense = false, bool bIgnoreTargetRating = false);
extern DAM_LL	CalcMagicDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim);
extern DAM_LL	CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense = false);
extern DAM_DOUBLE	CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating = false);

#ifdef NEW_ICEDAMAGE_SYSTEM
extern bool battle_is_icedamage(LPCHARACTER pAttacker, LPCHARACTER pVictim);
#endif
extern bool	battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim);
extern DAM_LL	battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim);
extern void	battle_end(LPCHARACTER ch);

extern bool	battle_distance_valid_by_xy(long x, long y, long tx, long ty);
extern bool	battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim);

extern void	NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim);

inline void AttackAffect(LPCHARACTER pkAttacker,
	LPCHARACTER pkVictim,
	BYTE att_point,
	DWORD immune_flag,
	DWORD affect_idx,
	BYTE affect_point,
	long affect_amount,
	DWORD affect_flag,
	int time,
	const char* name)
{
	if (pkAttacker->GetPoint(att_point) && !pkVictim->IsAffectFlag(affect_flag))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(att_point) && !pkVictim->IsImmune(immune_flag))
		{
			pkVictim->AddAffect(affect_idx, affect_point, affect_amount, affect_flag, time, 0, true);
		}
	}
}

inline void SkillAttackAffect(LPCHARACTER pkVictim,
	int success_pct,
	DWORD immune_flag,
	DWORD affect_idx,
	BYTE affect_point,
	long affect_amount,
	DWORD affect_flag,
	int time,
	const char* name)
{
	if (success_pct && !pkVictim->IsAffectFlag(affect_flag))
	{
		if (number(1, 1000) <= success_pct && !pkVictim->IsImmune(immune_flag))
		{
			pkVictim->AddAffect(affect_idx, affect_point, affect_amount, affect_flag, time, 0, true);
		}
	}
}
#endif
