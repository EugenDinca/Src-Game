#ifndef __INC_METIN_II_GAME_CONSTANTS_H__
#define __INC_METIN_II_GAME_CONSTANTS_H__

#include "../../common/tables.h"

enum EMonsterChatState
{
	MONSTER_CHAT_WAIT,
	MONSTER_CHAT_ATTACK,
	MONSTER_CHAT_CHASE,
	MONSTER_CHAT_ATTACKED,
};

typedef struct SMobRankStat
{
	int iGoldPercent;
} TMobRankStat;

typedef struct SMobStat
{
	BYTE	byLevel;
	WORD	HP;
	DWORD	dwExp;
	WORD	wDefGrade;
} TMobStat;

typedef struct SBattleTypeStat
{
	int		AttGradeBias;
	int		DefGradeBias;
	int		MagicAttGradeBias;
	int		MagicDefGradeBias;
} TBattleTypeStat;

typedef struct SJobInitialPoints
{
	int		st, ht, dx, iq;
	int		max_hp, max_sp;
	int		hp_per_ht, sp_per_iq;
	int		hp_per_lv_begin, hp_per_lv_end;
	int		sp_per_lv_begin, sp_per_lv_end;
	int		max_stamina;
	int		stamina_per_con;
	int		stamina_per_lv_begin, stamina_per_lv_end;
} TJobInitialPoints;

typedef struct __coord
{
	int		x, y;
} Coord;

typedef struct SApplyInfo
{
	BYTE	bPointType;                          // APPLY -> POINT
} TApplyInfo;

enum {
	FORTUNE_BIG_LUCK,
	FORTUNE_LUCK,
	FORTUNE_SMALL_LUCK,
	FORTUNE_NORMAL,
	FORTUNE_SMALL_BAD_LUCK,
	FORTUNE_BAD_LUCK,
	FORTUNE_BIG_BAD_LUCK,
	FORTUNE_MAX_NUM,
};

const int STONE_INFO_MAX_NUM = 22;
const int STONE_LEVEL_MAX_NUM = 4;

struct SStoneDropInfo
{
	DWORD dwMobVnum;
	int iDropPct;
	int iLevelPct[STONE_LEVEL_MAX_NUM + 1];
};

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
typedef struct SfishInfo
{
	BYTE itemAttribute;
	BYTE attributeValue;
}TfishInfo;
#endif

inline bool operator < (const SStoneDropInfo& l, DWORD r)
{
	return l.dwMobVnum < r;
}

inline bool operator < (DWORD l, const SStoneDropInfo& r)
{
	return l < r.dwMobVnum;
}

inline bool operator < (const SStoneDropInfo& l, const SStoneDropInfo& r)
{
	return l.dwMobVnum < r.dwMobVnum;
}

extern const TApplyInfo		aApplyInfo[MAX_APPLY_NUM];
extern const TMobRankStat       MobRankStats[MOB_RANK_MAX_NUM];

extern TBattleTypeStat		BattleTypeStats[BATTLE_TYPE_MAX_NUM];

extern const DWORD		party_exp_distribute_table[PLAYER_EXP_TABLE_MAX + 1];

extern const DWORD		exp_table_common[PLAYER_MAX_LEVEL_CONST + 1];

extern const DWORD* exp_table;

extern const DWORD		guild_exp_table[GUILD_MAX_LEVEL + 1];
extern const DWORD		guild_exp_table2[GUILD_MAX_LEVEL + 1];

#define MAX_EXP_DELTA_OF_LEV	31
#define PERCENT_LVDELTA(me, victim) aiPercentByDeltaLev[MINMAX(0, (victim + 15) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#define PERCENT_LVDELTA_BOSS(me, victim) aiPercentByDeltaLevForBoss[MINMAX(0, (victim + 15) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#define CALCULATE_VALUE_LVDELTA(me, victim, val) ((val * PERCENT_LVDELTA(me, victim)) / 100)
extern const int		aiPercentByDeltaLev_euckr[MAX_EXP_DELTA_OF_LEV];
extern const int		aiPercentByDeltaLevForBoss_euckr[MAX_EXP_DELTA_OF_LEV];
extern const int* aiPercentByDeltaLev;
extern const int* aiPercentByDeltaLevForBoss;

#define ARROUND_COORD_MAX_NUM	161
extern Coord			aArroundCoords[ARROUND_COORD_MAX_NUM];
extern TJobInitialPoints	JobInitialPoints[JOB_MAX_NUM];

extern const int		aiMobEnchantApplyIdx[MOB_ENCHANTS_MAX_NUM];
extern const int		aiMobResistsApplyIdx[MOB_RESISTS_MAX_NUM];

extern const int		aSkillAttackAffectProbByRank[MOB_RANK_MAX_NUM];

extern const int aiItemMagicAttributePercentHigh[ITEM_ATTRIBUTE_MAX_LEVEL];
extern const int aiItemMagicAttributePercentLow[ITEM_ATTRIBUTE_MAX_LEVEL];

extern const int aiItemAttributeAddPercent[ITEM_ATTRIBUTE_MAX_NUM];

extern const int aiWeaponSocketQty[WEAPON_NUM_TYPES];
extern const int aiArmorSocketQty[ARMOR_NUM_TYPES];
extern const int aiSocketPercentByQty[5][4];

extern const int aiExpLossPercents[PLAYER_EXP_TABLE_MAX + 1];

extern const int* aiSkillPowerByLevel;
extern const int aiSkillPowerByLevel_euckr[SKILL_MAX_LEVEL + 1];

extern const int aiPolymorphPowerByLevel[SKILL_MAX_LEVEL + 1];

extern const int aiSkillBookCountForLevelUp[10];
extern const int aiGrandMasterSkillBookCountForLevelUp[10];
extern const int aiGrandMasterSkillBookMinCount[10];
extern const int aiGrandMasterSkillBookMaxCount[10];
extern const int CHN_aiPartyBonusExpPercentByMemberCount[9];
extern const int KOR_aiPartyBonusExpPercentByMemberCount[9];
extern const int KOR_aiUniqueItemPartyBonusExpPercentByMemberCount[9];

typedef std::map<DWORD, TItemAttrTable> TItemAttrMap;
typedef std::map<DWORD, TItemAttrRareTable> TItemAttrRareMap;
extern TItemAttrMap g_map_itemAttr;
extern TItemAttrRareMap g_map_itemRare;

extern const int* aiChainLightningCountBySkillLevel;
extern const int aiChainLightningCountBySkillLevel_euckr[SKILL_MAX_LEVEL + 1];

extern const char* c_apszEmpireNames[EMPIRE_MAX_NUM];
extern const char* c_apszPrivNames[MAX_PRIV_NUM];
extern const SStoneDropInfo aStoneDrop[STONE_INFO_MAX_NUM];

typedef struct
{
	long lMapIndex;
	int iWarPrice;
	int iWinnerPotionRewardPctToWinner;
	int iLoserPotionRewardPctToWinner;
	int iInitialScore;
	int iEndScore;
} TGuildWarInfo;

extern TGuildWarInfo KOR_aGuildWarInfo[GUILD_WAR_TYPE_MAX_NUM];

// ACCESSORY_REFINE
enum
{
#ifdef ENABLE_JEWELS_RENEWAL
	ITEM_ACCESSORY_SOCKET_MAX_NUM = 3
#else
	ITEM_ACCESSORY_SOCKET_MAX_NUM = 3
#endif
};

extern const int aiAccessorySocketAddPct[ITEM_ACCESSORY_SOCKET_MAX_NUM];
extern const int aiAccessorySocketEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
#ifdef ENABLE_JEWELS_RENEWAL
extern const int aiAccessorySocketPermaEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
#endif
extern const int aiAccessorySocketDegradeTime[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
extern const int aiAccessorySocketPutPct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
long FN_get_apply_type(const char* apply_type_string);


#ifdef ENABLE_MINING_RENEWAL
extern const int IncreasePointPerSkillLevel[SKILL_MAX_LEVEL + 1];
#endif

// END_OF_ACCESSORY_REFINE
#ifdef ENABLE_BIOLOG_SYSTEM
std::pair<BYTE,BYTE> StringApplyToPoint(const char* apply_type_string);
#endif

#ifdef ENABLE_SKILLS_LEVEL_OVER_P
extern const int MinReadSoulStone[10];
extern const int MaxReadSoulStone[10];
extern const int ReadSoulStone[10];
#endif
#ifdef ENABLE_PASSIVE_SKILLS
extern const int PassiveReadBook[50];
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
extern const DWORD		Pet_Skill_Table[15][31];
extern const DWORD		Pet_Change_Table[8][2];
extern const DWORD		Pet_Table[11][2];
extern const BYTE		Pet_Bonus_Type[3];
extern const BYTE		Pet_Skin_Bonus_Value[6];
extern const DWORD		Pet_Skin_Item_Vnum_List[5];
extern const DWORD		Pet_Skin_Mob_Vnum_List[6];
extern const DWORD		Pet_Evolution_Table[3][4];
typedef std::unordered_map<BYTE, BYTE> NEW_PET_EVOLUTION_MAP;
extern NEW_PET_EVOLUTION_MAP mapEvolution;
void SetEvolutionMap();
bool EvolutionLevelCheck(BYTE level, BYTE evolution);
#endif

#ifdef ENABLE_6TH_7TH_ATTR
typedef std::unordered_map<BYTE, std::vector<T67AttrTable>> map67Attr;
extern map67Attr map_item67ThAttr;
extern const WORD New67AttrTable[18][6 + ATTRIBUTE_67TH_MAX];
void Set67AttrTable();
#endif

#ifdef ENABLE_POWER_RANKING
extern const std::pair<BYTE, BYTE> powerPointList[MAX_APPLY_NUM];
#endif

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
extern const int fishItemInfo[12][2];
#endif

#ifdef ENABLE_DISTANCE_SKILL_SELECT
extern const int skillSelectTable[4][2][6][2];
#endif

typedef struct SGiveItemList
{
	DWORD vnum;
	TPlayerItemAttribute attr[7];
	long socket[6];
}TGiveItemList;


extern const TGiveItemList GiveItemTable[5][24];
extern const DWORD GiveItemTableVnumList[30];

bool NotChestWindowItem(DWORD vnum);

#ifdef ENABLE_EVENT_MANAGER
extern std::set<DWORD> eventItemList;
extern std::map<DWORD, int> eventItemEndTime;

void EventItemListTimeLoad();
void EventItemTimeUpdate();
int GetEventItemDestroyTime(DWORD vnum);
#endif

#endif