#ifndef __INC_TABLES_H__
#define __INC_TABLES_H__

#include "length.h"
#include "item_length.h"
#include "Controls.h"
#include <vector>
typedef	DWORD IDENT;

enum GameToDBPackets
{
	HEADER_GD_LOGIN							= 1,
	HEADER_GD_LOGOUT						= 2,
	HEADER_GD_PLAYER_LOAD					= 3,
	HEADER_GD_PLAYER_SAVE					= 4,
	HEADER_GD_PLAYER_CREATE					= 5,
	HEADER_GD_PLAYER_DELETE					= 6,
	HEADER_GD_LOGIN_KEY						= 7,
	HEADER_GD_BOOT							= 8,
	HEADER_GD_PLAYER_COUNT					= 9,
	HEADER_GD_QUEST_SAVE					= 10,
	HEADER_GD_SAFEBOX_LOAD					= 11,
	HEADER_GD_SAFEBOX_SAVE					= 12,
	HEADER_GD_SAFEBOX_CHANGE_SIZE			= 13,
	HEADER_GD_EMPIRE_SELECT					= 14,
	HEADER_GD_SAFEBOX_CHANGE_PASSWORD		= 15,
	HEADER_GD_SAFEBOX_CHANGE_PASSWORD_SECOND = 16,
	HEADER_GD_DIRECT_ENTER					= 17,
	HEADER_GD_GUILD_SKILL_UPDATE			= 18,
	HEADER_GD_GUILD_EXP_UPDATE				= 19,
	HEADER_GD_GUILD_ADD_MEMBER				= 20,
	HEADER_GD_GUILD_REMOVE_MEMBER			= 21,
	HEADER_GD_GUILD_CHANGE_GRADE			= 22,
	HEADER_GD_GUILD_CHANGE_MEMBER_DATA		= 23,
	HEADER_GD_GUILD_DISBAND					= 24,
	HEADER_GD_GUILD_WAR						= 25,
	HEADER_GD_GUILD_WAR_SCORE				= 26,
	HEADER_GD_GUILD_CREATE					= 27,
	HEADER_GD_ITEM_SAVE						= 28,
	HEADER_GD_ITEM_DESTROY					= 29,
	HEADER_GD_ADD_AFFECT					= 30,
	HEADER_GD_REMOVE_AFFECT					= 31,
	HEADER_GD_HIGHSCORE_REGISTER			= 32,
	HEADER_GD_ITEM_FLUSH					= 33,
	HEADER_GD_PARTY_CREATE					= 34,
	HEADER_GD_PARTY_DELETE					= 35,
	HEADER_GD_PARTY_ADD						= 36,
	HEADER_GD_PARTY_REMOVE					= 37,
	HEADER_GD_PARTY_STATE_CHANGE			= 38,
	HEADER_GD_PARTY_HEAL_USE				= 39,
	HEADER_GD_FLUSH_CACHE					= 40,
	HEADER_GD_RELOAD_PROTO					= 41,
	HEADER_GD_CHANGE_NAME					= 42,
	HEADER_GD_GUILD_CHANGE_LADDER_POINT		= 43,
	HEADER_GD_GUILD_USE_SKILL				= 44,
	HEADER_GD_REQUEST_EMPIRE_PRIV			= 45,
	HEADER_GD_REQUEST_GUILD_PRIV			= 46,
	HEADER_GD_GUILD_DEPOSIT_MONEY			= 48,
	HEADER_GD_GUILD_WITHDRAW_MONEY			= 49,
	HEADER_GD_GUILD_WITHDRAW_MONEY_GIVE_REPLY = 50,
	HEADER_GD_REQUEST_CHARACTER_PRIV		= 51,
	HEADER_GD_SET_EVENT_FLAG				= 52,
	HEADER_GD_PARTY_SET_MEMBER_LEVEL		= 53,
	HEADER_GD_GUILD_WAR_BET					= 54,
	HEADER_GD_CREATE_OBJECT					= 55,
	HEADER_GD_DELETE_OBJECT					= 56,
	HEADER_GD_UPDATE_LAND					= 57,
	HEADER_GD_MARRIAGE_ADD					= 58,
	HEADER_GD_MARRIAGE_UPDATE				= 59,
	HEADER_GD_MARRIAGE_REMOVE				= 60,
	HEADER_GD_WEDDING_REQUEST				= 61,
	HEADER_GD_WEDDING_READY					= 62,
	HEADER_GD_WEDDING_END					= 63,
	HEADER_GD_AUTH_LOGIN					= 64,
	HEADER_GD_LOGIN_BY_KEY					= 65,
	HEADER_GD_MALL_LOAD						= 66,
	HEADER_GD_MYSHOP_PRICELIST_UPDATE		= 67,
	HEADER_GD_MYSHOP_PRICELIST_REQ			= 68,
	HEADER_GD_BLOCK_CHAT					= 69,
	HEADER_GD_RELOAD_ADMIN					= 70,
	HEADER_GD_BREAK_MARRIAGE				= 71,
	HEADER_GD_REQ_CHANGE_GUILD_MASTER		= 72,
	HEADER_GD_REQ_SPARE_ITEM_ID_RANGE		= 73,
	HEADER_GD_UPDATE_HORSE_NAME				= 74,
	HEADER_GD_REQ_HORSE_NAME				= 75,
	HEADER_GD_DC							= 76,
	HEADER_GD_VALID_LOGOUT					= 77,
	HEADER_GD_REQUEST_CHARGE_CASH			= 78,
	HEADER_GD_DELETE_AWARDID				= 79,
	HEADER_GD_UPDATE_CHANNELSTATUS			= 80,
	HEADER_GD_REQUEST_CHANNELSTATUS			= 81,
#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
	HEADER_GD_FIND_CHANNEL					= 82,
#endif
#ifdef ENABLE_OFFLINE_SHOP
	HEADER_GD_NEW_OFFLINESHOP				= 83,
#endif
#ifdef ENABLE_GLOBAL_RANKING
	HEADER_GD_GLOBAL_RANKING				= 85,
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	HEADER_GD_IN_GAME_LOG					= 86,
#endif
#ifdef ENABLE_HWID
	HEADER_GD_HWID							= 87,
#endif
#ifdef ENABLE_EVENT_MANAGER
	HEADER_GD_EVENT_MANAGER					= 88,
#endif
#ifdef ENABLE_ITEMSHOP
	HEADER_GD_ITEMSHOP						= 89,
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	HEADER_GD_SKILL_COLOR_SAVE				= 90,
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	HEADER_GD_REQUEST_CHANGE_LANGUAGE		= 91,
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	HEADER_GD_SAVE_EXT_BATTLE_PASS			= 92,
#endif
	HEADER_GD_FORCE_FLUSH_CACHE				= 250,
	HEADER_GD_SETUP							= 0xff,

};
enum DbToGamePackets
{
	HEADER_DG_NOTICE						= 1,
	HEADER_DG_LOGIN_SUCCESS					= 2,
	HEADER_DG_LOGIN_NOT_EXIST				= 3,
	HEADER_DG_LOGIN_WRONG_PASSWD			= 4,
	HEADER_DG_LOGIN_ALREADY					= 5,
	HEADER_DG_PLAYER_LOAD_SUCCESS			= 6,
	HEADER_DG_PLAYER_LOAD_FAILED			= 7,
	HEADER_DG_PLAYER_CREATE_SUCCESS			= 8,
	HEADER_DG_PLAYER_CREATE_ALREADY			= 9,
	HEADER_DG_PLAYER_CREATE_FAILED			= 10,
	HEADER_DG_PLAYER_DELETE_SUCCESS			= 11,
	HEADER_DG_PLAYER_DELETE_FAILED			= 12,
	HEADER_DG_ITEM_LOAD						= 13,
	HEADER_DG_BOOT							= 14,
	HEADER_DG_QUEST_LOAD					= 15,
	HEADER_DG_SAFEBOX_LOAD					= 16,
	HEADER_DG_SAFEBOX_CHANGE_SIZE			= 17,
	HEADER_DG_SAFEBOX_WRONG_PASSWORD		= 18,
	HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER = 19,
	HEADER_DG_EMPIRE_SELECT					= 20,
	HEADER_DG_AFFECT_LOAD					= 21,
	HEADER_DG_MALL_LOAD						= 22,
	HEADER_DG_DIRECT_ENTER					= 23,
	HEADER_DG_GUILD_SKILL_UPDATE			= 24,
	HEADER_DG_GUILD_SKILL_RECHARGE			= 25,
	HEADER_DG_GUILD_EXP_UPDATE				= 26,
	HEADER_DG_PARTY_CREATE					= 27,
	HEADER_DG_PARTY_DELETE					= 28,
	HEADER_DG_PARTY_ADD						= 29,
	HEADER_DG_PARTY_REMOVE					= 30,
	HEADER_DG_PARTY_STATE_CHANGE			= 31,
	HEADER_DG_PARTY_HEAL_USE				= 32,
	HEADER_DG_PARTY_SET_MEMBER_LEVEL		= 33,
	HEADER_DG_TIME							= 34,
	HEADER_DG_ITEM_ID_RANGE					= 35,
	HEADER_DG_GUILD_ADD_MEMBER				= 36,
	HEADER_DG_GUILD_REMOVE_MEMBER			= 37,
	HEADER_DG_GUILD_CHANGE_GRADE			= 38,
	HEADER_DG_GUILD_CHANGE_MEMBER_DATA		= 39,
	HEADER_DG_GUILD_DISBAND					= 40,
	HEADER_DG_GUILD_WAR						= 41,
	HEADER_DG_GUILD_WAR_SCORE				= 42,
	HEADER_DG_GUILD_TIME_UPDATE				= 43,
	HEADER_DG_GUILD_LOAD					= 44,
	HEADER_DG_GUILD_LADDER					= 45,
	HEADER_DG_GUILD_SKILL_USABLE_CHANGE		= 46,
	HEADER_DG_GUILD_MONEY_CHANGE			= 47,
	HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE		= 48,
	HEADER_DG_SET_EVENT_FLAG				= 49,
	HEADER_DG_GUILD_WAR_RESERVE_ADD			= 50,
	HEADER_DG_GUILD_WAR_RESERVE_DEL			= 51,
	HEADER_DG_GUILD_WAR_BET					= 52,
	HEADER_DG_RELOAD_PROTO					= 53,
	HEADER_DG_CHANGE_NAME					= 54,
	HEADER_DG_AUTH_LOGIN					= 55,
	HEADER_DG_CHANGE_EMPIRE_PRIV			= 56,
	HEADER_DG_CHANGE_GUILD_PRIV				= 57,
	HEADER_DG_CHANGE_CHARACTER_PRIV			= 59,
	HEADER_DG_CREATE_OBJECT					= 60,
	HEADER_DG_DELETE_OBJECT					= 61,
	HEADER_DG_UPDATE_LAND					= 62,
	HEADER_DG_MARRIAGE_ADD					= 63,
	HEADER_DG_MARRIAGE_UPDATE				= 64,
	HEADER_DG_MARRIAGE_REMOVE				= 65,
	HEADER_DG_WEDDING_REQUEST				= 66,
	HEADER_DG_WEDDING_READY					= 67,
	HEADER_DG_WEDDING_START					= 68,
	HEADER_DG_WEDDING_END					= 69,
	HEADER_DG_MYSHOP_PRICELIST_RES			= 70,
	HEADER_DG_RELOAD_ADMIN					= 71,
	HEADER_DG_BREAK_MARRIAGE				= 72,
	HEADER_DG_ACK_CHANGE_GUILD_MASTER		= 73,
	HEADER_DG_ACK_SPARE_ITEM_ID_RANGE		= 74,
	HEADER_DG_UPDATE_HORSE_NAME				= 75,
	HEADER_DG_ACK_HORSE_NAME				= 76,
	HEADER_DG_RESULT_CHARGE_CASH			= 77,
	HEADER_DG_ITEMAWARD_INFORMER			= 78,
	HEADER_DG_RESPOND_CHANNELSTATUS			= 79,
#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
	HEADER_DG_CHANNEL_RESULT				= 80,
#endif
#ifdef ENABLE_OFFLINE_SHOP
	HEADER_DG_NEW_OFFLINESHOP				= 81,
#endif
#ifdef ENABLE_GLOBAL_RANKING
	HEADER_DG_GLOBAL_RANKING				= 82,
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	HEADER_DG_IN_GAME_LOG					= 83,
#endif
#ifdef ENABLE_HWID
	HEADER_DG_HWID							= 84,
#endif
#ifdef ENABLE_EVENT_MANAGER
	HEADER_DG_EVENT_MANAGER					= 85,
#endif
#ifdef ENABLE_ITEMSHOP
	HEADER_DG_ITEMSHOP						= 86,
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	HEADER_DG_SKILL_COLOR_LOAD				= 87,
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	HEADER_DG_EXT_BATTLE_PASS_LOAD			= 88,
#endif
	HEADER_DG_SHUTDOWN_CORE					= 250,
	HEADER_DG_MAP_LOCATIONS					= 0xfe,
	HEADER_DG_P2P							= 0xff,

};

/* game Server -> DB Server */
#pragma pack(1)
enum ERequestChargeType
{
	ERequestCharge_Cash = 0,
	ERequestCharge_Mileage,
};

typedef struct SRequestChargeCash
{
	DWORD		dwAID;		// id(primary key) - Account Table
	DWORD		dwAmount;
	ERequestChargeType	eChargeType;

} TRequestChargeCash;

typedef struct SSimplePlayer
{
	DWORD		dwID;
	char		szName[CHARACTER_NAME_MAX_LEN + 1];
	BYTE		byJob;
	BYTE		byLevel;
	DWORD		dwPlayMinutes;
#ifdef ENABLE_OFFICAL_CHARACTER_SCREEN
	DWORD		dwLastPlayTime;
#endif
	BYTE		byST, byHT, byDX, byIQ;
	DWORD		wMainPart;
	BYTE		bChangeName;
	DWORD		wHairPart;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	DWORD		wAccePart;
#endif
	BYTE		bDummy[4];
	long		x, y;
	long		lAddr;
	WORD		wPort;
	BYTE		skill_group;
} TSimplePlayer;

typedef struct SAccountTable
{
	DWORD		id;
	char		login[LOGIN_MAX_LEN + 1];
	char		passwd[PASSWD_MAX_LEN + 1];
	char		social_id[SOCIAL_ID_MAX_LEN + 1];
	char		status[ACCOUNT_STATUS_MAX_LEN + 1];
	BYTE		bEmpire;
	TSimplePlayer	players[PLAYER_PER_ACCOUNT];
#ifdef ENABLE_HWID
	char hwid[64 + 1];
	DWORD hwidID[2];
	DWORD descHwidID;
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TAccountTable;

typedef struct SPacketDGCreateSuccess
{
	BYTE		bAccountCharacterIndex;
	TSimplePlayer	player;
} TPacketDGCreateSuccess;

typedef struct TPlayerItemAttribute
{
	BYTE	bType;
	short	sValue;
} TPlayerItemAttribute;

typedef struct SPlayerItem
{
	DWORD	id;
	BYTE	window;
	WORD	pos;
	DWORD	count;

	DWORD	vnum;
	long	alSockets[ITEM_SOCKET_MAX_NUM];

	TPlayerItemAttribute    aAttr[ITEM_ATTRIBUTE_MAX_NUM];

	DWORD	owner;
} TPlayerItem;

typedef struct SQuickslot
{
	BYTE	type;
	QS_USHORT	pos;
} TQuickslot;

typedef struct SPlayerSkill
{
	BYTE	bMasterType;
	BYTE	bLevel;
#ifdef ENABLE_SKILL_DESCRIPTION_RENEWAL
	BYTE	bBooks;
#endif
} TPlayerSkill;

struct	THorseInfo
{
	BYTE	bLevel;
	BYTE	bRiding;
	short	sStamina;
	short	sHealth;
	DWORD	dwHorseHealthDropTime;
};

#ifdef ENABLE_PLAYER_STATS_SYSTEM
enum StatsTypes
{
#ifdef ENABLE_POWER_RANKING
	STATS_POWER,
#endif

	STATS_MONSTER_KILLED,
	STATS_STONE_KILLED,
	STATS_BOSS_KILLED,
	STATS_PLAYER_KILLED,

	STATS_MONSTER_DAMAGE,
	STATS_STONE_DAMAGE,
	STATS_BOSS_DAMAGE,
	STATS_PLAYER_DAMAGE,

	STATS_OPENED_CHEST,
	STATS_FISHING,
	STATS_MINING,
	STATS_COMPLATE_DUNGEON,

	STATS_UPGRADE_ITEM,
	STATS_USE_ENCHANTED_ITEM,
	STATS_MAX,

#ifdef ENABLE_GLOBAL_RANKING
	RANKING_ALIGN = STATS_MAX,
	RANKING_PLAY_TIME = STATS_MAX + 1,
	RANKING_MAX
#endif

};
typedef struct SPlayerStats
{
	DWORD stat[STATS_MAX];
}TPlayerStats;
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
enum BiologSupportItemVnum
{
	BIOLOG_CHANCE_ITEM = 71035,
	BIOLOG_TIMERESET_ITEM = 30629,

};
typedef struct SBiologBonusTable
{
	BYTE apply_type;
	BYTE point_type;
	WORD value;
}TBiologBonusTable;

typedef struct SBiologBonus
{
	BYTE type;
	WORD value;
}TBiologBonus;

typedef struct SBiologInformationsSqlFetch
{
	BYTE mission_index;
	BYTE mission_level;
	DWORD mission_vnum;
	DWORD mission_stone_vnum;
	DWORD mission_stone_mob_vnum;
	BYTE mission_stone_mob_change;
	MAX_COUNT mission_count;
	BYTE mission_success_change;
	DWORD mission_cooldown;
	TBiologBonusTable mission_bonus[4];
}TBiologInformationsSqlFetch;

typedef struct SBiologInformationsCache
{
	BYTE mission_level;
	DWORD mission_vnum;
	DWORD mission_stone_vnum;
	DWORD mission_stone_mob_vnum;
	BYTE mission_stone_mob_change;
	MAX_COUNT mission_count;
	BYTE mission_success_change;
	DWORD mission_cooldown;
	TBiologBonus mission_bonus[4];
}TBiologInformationsCache;

typedef struct SPacketGCBiologUpdate
{
	BYTE level;
	MAX_COUNT given_count;
	DWORD time;
	bool alert;
	BYTE state;
}TPacketGCBiologUpdate;

typedef struct SBiologMobInfo
{
	DWORD vnum;
	BYTE chance;
}TBiologMobInfo;
#endif

#ifdef ENABLE_CHAR_SETTINGS
enum CharSettingsList
{
	SETTINGS_ANTI_EXP,
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	SETTINGS_HIDE_COSTUME_HAIR,
	SETTINGS_HIDE_COSTUME_BODY,
	SETTINGS_HIDE_COSTUME_WEAPON,
	SETTINGS_HIDE_COSTUME_ACCE,
	SETTINGS_HIDE_COSTUME_AURA,
	SETTINGS_HIDE_COSTUME_CROWN,
#endif
#ifdef ENABLE_STOP_CHAT
	SETTINGS_STOP_CHAT,
#endif

};
typedef struct SCharSettings
{
	bool antiexp;
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	bool HIDE_COSTUME_HAIR;
	bool HIDE_COSTUME_BODY;
	bool HIDE_COSTUME_WEAPON;
	bool HIDE_COSTUME_ACCE;
	bool HIDE_COSTUME_AURA;
	bool HIDE_COSTUME_CROWN;
#endif
#ifdef ENABLE_STOP_CHAT
	bool STOP_SHOUT;
#endif
}TCharSettings;
#endif

#ifdef ENABLE_DUNGEON_TURN_BACK
typedef struct SDungeonPlayerInfo
{
	DWORD dungeonMapIdx;
	BYTE dungeonChannel;
}TDungeonPlayerInfo;
#endif

typedef struct SPlayerTable
{
	DWORD	id;

	char	name[CHARACTER_NAME_MAX_LEN + 1];
	char	ip[IP_ADDRESS_LENGTH + 1];

	WORD	job;
	BYTE	voice;

	BYTE	level;
	BYTE	level_step;
	short	st, ht, dx, iq;

	DWORD	exp;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	gold;
#else
	INT		gold;
#endif

	BYTE	dir;
	INT		x, y, z;
	INT		lMapIndex;

	long	lExitX, lExitY;
	long	lExitMapIndex;

	// @fixme301
	int		hp;
	int		sp;

	short	sRandomHP;
	short	sRandomSP;

	int		playtime;
#ifdef ENABLE_OFFICAL_CHARACTER_SCREEN
	int		lastplaytime;
#endif
	short	stat_point;
	short	skill_point;
	short	sub_skill_point;
	short	horse_skill_point;

	TPlayerSkill skills[SKILL_MAX_NUM];

	TQuickslot  quickslot[QUICKSLOT_MAX_NUM];

	BYTE	part_base;
	DWORD	parts[PART_MAX_NUM];

	short	stamina;

	BYTE	skill_group;
	long	lAlignment;

	short	stat_reset_count;

	THorseInfo	horse;

	DWORD	logoff_interval;

	int		aiPremiumTimes[PREMIUM_MAX_NUM];

#ifdef ENABLE_PLAYER_STATS_SYSTEM
	TPlayerStats player_stats;
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	TPacketGCBiologUpdate biolog;
#endif
#ifdef ENABLE_CHAR_SETTINGS
	TCharSettings char_settings;
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	int		battle_pass_premium_id;
#endif
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	BYTE chatColor;
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	TDungeonPlayerInfo dungeonInfo;
#endif
} TPlayerTable;

typedef struct SMobSkillLevel
{
	DWORD	dwVnum;
	BYTE	bLevel;
} TMobSkillLevel;

typedef struct SEntityTable
{
	DWORD dwVnum;
} TEntityTable;

typedef struct SMobTable : public SEntityTable
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	char	szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

	BYTE	bType;			// Monster, NPC
	BYTE	bRank;			// PAWN, KNIGHT, KING
	BYTE	bBattleType;		// MELEE, etc..
	BYTE	bLevel;			// Level
	BYTE	bSize;

	DWORD	dwGoldMin;
	DWORD	dwGoldMax;
	DWORD	dwExp;
	HP_ULL	dwMaxHP;
	BYTE	bRegenCycle;
	BYTE	bRegenPercent;
	WORD	wDef;

	DWORD	dwAIFlag;
	DWORD	dwRaceFlag;
	DWORD	dwImmuneFlag;

	BYTE	bStr, bDex, bCon, bInt;
	DWORD	dwDamageRange[2];

	short	sAttackSpeed;
	short	sMovingSpeed;
	BYTE	bAggresiveHPPct;
	WORD	wAggressiveSight;
	WORD	wAttackRange;

	char	cEnchants[MOB_ENCHANTS_MAX_NUM];
	char	cResists[MOB_RESISTS_MAX_NUM];

	DWORD	dwResurrectionVnum;
	DWORD	dwDropItemVnum;

	BYTE	bMountCapacity;
	BYTE	bOnClickType;

	BYTE	bEmpire;
	char	szFolder[64 + 1];

	float	fDamMultiply;

	DWORD	dwSummonVnum;
	DWORD	dwDrainSP;
	DWORD	dwMobColor;
	DWORD	dwPolymorphItemVnum;

	TMobSkillLevel Skills[MOB_SKILL_MAX_NUM];

	BYTE	bBerserkPoint;
	BYTE	bStoneSkinPoint;
	BYTE	bGodSpeedPoint;
	BYTE	bDeathBlowPoint;
	BYTE	bRevivePoint;
} TMobTable;

typedef struct SSkillTable
{
	DWORD	dwVnum;
	char	szName[32 + 1];
	BYTE	bType;
	BYTE	bMaxLevel;
	DWORD	dwSplashRange;

	char	szPointOn[64];
	char	szPointPoly[100 + 1];
	char	szSPCostPoly[100 + 1];
	char	szDurationPoly[100 + 1];
	char	szDurationSPCostPoly[100 + 1];
	char	szCooldownPoly[100 + 1];
	char	szMasterBonusPoly[100 + 1];
	//char	szAttackGradePoly[100 + 1];
	char	szGrandMasterAddSPCostPoly[100 + 1];
	DWORD	dwFlag;
	DWORD	dwAffectFlag;

	// Data for secondary skill
	char 	szPointOn2[64];
	char 	szPointPoly2[100 + 1];
	char 	szDurationPoly2[100 + 1];
	DWORD 	dwAffectFlag2;

	// Data for grand master point
	char 	szPointOn3[64];
	char 	szPointPoly3[100 + 1];
	char 	szDurationPoly3[100 + 1];

	BYTE	bLevelStep;
	BYTE	bLevelLimit;
	DWORD	preSkillVnum;
	BYTE	preSkillLevel;

	long	lMaxHit;
	char	szSplashAroundDamageAdjustPoly[100 + 1];

	BYTE	bSkillAttrType;

	DWORD	dwTargetRange;
} TSkillTable;

#ifdef ENABLE_BUY_WITH_ITEMS
typedef struct SShopItemPrice
{
	uint32_t		vnum;
	uint32_t		count;
} TShopItemPrice;
#endif

typedef struct SShopItemTable
{
	DWORD		vnum;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE		count;
#endif
#ifdef ENABLE_BUY_WITH_ITEMS
	TShopItemPrice	itemPrice[MAX_SHOP_PRICES];
#endif
	TItemPos	pos;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	price;
#else
	DWORD		price;
#endif
	BYTE		display_pos;
} TShopItemTable;

typedef struct SShopTable
{
	DWORD		dwVnum;
	DWORD		dwNPCVnum;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	byItemCount;
#else
	BYTE		byItemCount;
#endif
	TShopItemTable	items[SHOP_HOST_ITEM_MAX_NUM];
} TShopTable;

#define QUEST_NAME_MAX_LEN	32
#define QUEST_STATE_MAX_LEN	64

typedef struct SQuestTable
{
	DWORD		dwPID;
	char		szName[QUEST_NAME_MAX_LEN + 1];
	char		szState[QUEST_STATE_MAX_LEN + 1];
	long		lValue;
} TQuestTable;

typedef struct SItemLimit
{
	BYTE	bType;
	long	lValue;
} TItemLimit;

typedef struct SItemApply
{
	BYTE	bType;
	long	lValue;
} TItemApply;

typedef struct SItemTable : public SEntityTable
{
	DWORD		dwVnumRange;
	char        szName[ITEM_NAME_MAX_LEN + 1];
	char	szLocaleName[ITEM_NAME_MAX_LEN + 1];
	BYTE	bType;
	BYTE	bSubType;

	BYTE        bWeight;
	BYTE	bSize;

	DWORD	dwAntiFlags;
	DWORD	dwFlags;
	DWORD	dwWearFlags;
	DWORD	dwImmuneFlag;

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t		dwGold;
	int64_t		dwShopBuyPrice;
#else
	DWORD		dwGold;
	DWORD		dwShopBuyPrice;
#endif

	TItemLimit	aLimits[ITEM_LIMIT_MAX_NUM];
	TItemApply	aApplies[ITEM_APPLY_MAX_NUM];
	long        alValues[ITEM_VALUES_MAX_NUM];
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	DWORD	dwRefinedVnum;
	WORD	wRefineSet;
	BYTE	bAlterToMagicItemPct;
	BYTE	bSpecular;
	BYTE	bGainSocketPct;

	short int	sAddonType;



	char		cLimitRealTimeFirstUseIndex;
	char		cLimitTimerBasedOnWearIndex;

} TItemTable;

struct TItemAttrTable
{
	TItemAttrTable() :
		dwApplyIndex(0),
		dwProb(0)
	{
		szApply[0] = 0;
		memset(&lValues, 0, sizeof(lValues));
		memset(&bMaxLevelBySet, 0, sizeof(bMaxLevelBySet));
	}

	char    szApply[APPLY_NAME_MAX_LEN + 1];
	DWORD   dwApplyIndex;
	DWORD   dwProb;
	long    lValues[ITEM_ATTRIBUTE_MAX_LEVEL];
	BYTE    bMaxLevelBySet[ATTRIBUTE_SET_MAX_NUM];
};

struct TItemAttrRareTable
{
	TItemAttrRareTable() :
		dwApplyIndex(0),
		dwProb(0)
	{
		szApply[0] = 0;
		memset(&lValues, 0, sizeof(lValues));
		memset(&bMaxLevelBySet, 0, sizeof(bMaxLevelBySet));
	}

	char    szApply[APPLY_NAME_MAX_LEN + 1];
	DWORD   dwApplyIndex;
	DWORD   dwProb;
	long    lValues[ITEM_ATTRIBUTE_MAX_LEVEL];
#ifdef ENABLE_ATTR_RARE_RENEWAL
	BYTE    bMaxLevelBySet[ATTRIBUTE_SET_RARE_MAX_NUM];
#else
	BYTE    bMaxLevelBySet[ATTRIBUTE_SET_MAX_NUM];
#endif
};

typedef struct SConnectTable
{
	char	login[LOGIN_MAX_LEN + 1];
	IDENT	ident;
} TConnectTable;

typedef struct SLoginPacket
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TLoginPacket;

typedef struct SPlayerLoadPacket
{
	DWORD	account_id;
	DWORD	player_id;
	BYTE	account_index;
} TPlayerLoadPacket;

typedef struct SPlayerCreatePacket
{
	char		login[LOGIN_MAX_LEN + 1];
	char		passwd[PASSWD_MAX_LEN + 1];
	DWORD		account_id;
	BYTE		account_index;
	TPlayerTable	player_table;
} TPlayerCreatePacket;

typedef struct SPlayerDeletePacket
{
	char	login[LOGIN_MAX_LEN + 1];
	DWORD	player_id;
	BYTE	account_index;
	//char	name[CHARACTER_NAME_MAX_LEN + 1];
	char	private_code[8];
} TPlayerDeletePacket;

typedef struct SLogoutPacket
{
	char login[LOGIN_MAX_LEN + 1];
	char passwd[PASSWD_MAX_LEN + 1];
	bool bCanUseLoginByKey;
} TLogoutPacket;

typedef struct SPlayerCountPacket
{
	DWORD	dwCount;
} TPlayerCountPacket;

#define SAFEBOX_MAX_NUM			135
#define SAFEBOX_PASSWORD_MAX_LEN	6

typedef struct SSafeboxTable
{
	DWORD	dwID;
	BYTE	bSize;
	DWORD	dwGold;
	WORD	wItemCount;
} TSafeboxTable;

typedef struct SSafeboxChangeSizePacket
{
	DWORD	dwID;
	BYTE	bSize;
} TSafeboxChangeSizePacket;

typedef struct SSafeboxLoadPacket
{
	DWORD	dwID;
	char	szLogin[LOGIN_MAX_LEN + 1];
	char	szPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
} TSafeboxLoadPacket;

typedef struct SSafeboxChangePasswordPacket
{
	DWORD	dwID;
	char	szOldPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
	char	szNewPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
} TSafeboxChangePasswordPacket;

typedef struct SSafeboxChangePasswordPacketAnswer
{
	BYTE	flag;
} TSafeboxChangePasswordPacketAnswer;

typedef struct SEmpireSelectPacket
{
	DWORD	dwAccountID;
	BYTE	bEmpire;
} TEmpireSelectPacket;

typedef struct SPacketGDSetup
{
	char	szPublicIP[16];	// Public IP which listen to users
	BYTE	bChannel;
	WORD	wListenPort;
	WORD	wP2PPort;
	long	alMaps[MAP_ALLOW_LIMIT];
	DWORD	dwLoginCount;
	BYTE	bAuthServer;
} TPacketGDSetup;

typedef struct SPacketDGMapLocations
{
	BYTE	bCount;
} TPacketDGMapLocations;

typedef struct SMapLocation
{
	long	alMaps[MAP_ALLOW_LIMIT];
	char	szHost[MAX_HOST_LENGTH + 1];
	WORD	wPort;
} TMapLocation;

typedef struct SPacketDGP2P
{
	char	szHost[MAX_HOST_LENGTH + 1];
	WORD	wPort;
	BYTE	bChannel;
} TPacketDGP2P;

typedef struct SPacketGDDirectEnter
{
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
	BYTE	index;
} TPacketGDDirectEnter;

typedef struct SPacketDGDirectEnter
{
	TAccountTable accountTable;
	TPlayerTable playerTable;
} TPacketDGDirectEnter;

typedef struct SPacketGuildSkillUpdate
{
	DWORD guild_id;
	int amount;
	BYTE skill_levels[12];
	BYTE skill_point;
	BYTE save;
} TPacketGuildSkillUpdate;

typedef struct SPacketGuildExpUpdate
{
	DWORD guild_id;
	int amount;
} TPacketGuildExpUpdate;

typedef struct SPacketGuildChangeMemberData
{
	DWORD guild_id;
	DWORD pid;
	DWORD offer;
	BYTE level;
	BYTE grade;
} TPacketGuildChangeMemberData;


typedef struct SPacketDGLoginAlready
{
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketDGLoginAlready;

typedef struct TPacketAffectElement
{
	DWORD	dwType;
	BYTE	bApplyOn;
	long	lApplyValue;
	DWORD	dwFlag;
	long	lDuration;
	long	lSPCost;
} TPacketAffectElement;

typedef struct SPacketGDAddAffect
{
	DWORD			dwPID;
	TPacketAffectElement	elem;
} TPacketGDAddAffect;

typedef struct SPacketGDRemoveAffect
{
	DWORD	dwPID;
	DWORD	dwType;
	BYTE	bApplyOn;
} TPacketGDRemoveAffect;

typedef struct SPacketGDHighscore
{
	DWORD	dwPID;
	long	lValue;
	char	cDir;
	char	szBoard[21];
} TPacketGDHighscore;

typedef struct SPacketPartyCreate
{
	DWORD	dwLeaderPID;
} TPacketPartyCreate;

typedef struct SPacketPartyDelete
{
	DWORD	dwLeaderPID;
} TPacketPartyDelete;

typedef struct SPacketPartyAdd
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bState;
} TPacketPartyAdd;

typedef struct SPacketPartyRemove
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
} TPacketPartyRemove;

typedef struct SPacketPartyStateChange
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bRole;
	BYTE	bFlag;
} TPacketPartyStateChange;

typedef struct SPacketPartySetMemberLevel
{
	DWORD	dwLeaderPID;
	DWORD	dwPID;
	BYTE	bLevel;
} TPacketPartySetMemberLevel;

typedef struct SPacketGDBoot
{
    DWORD	dwItemIDRange[2];
	char	szIP[16];
} TPacketGDBoot;

typedef struct SPacketGuild
{
	DWORD	dwGuild;
	DWORD	dwInfo;
} TPacketGuild;

typedef struct SPacketGDGuildAddMember
{
	DWORD	dwPID;
	DWORD	dwGuild;
	BYTE	bGrade;
} TPacketGDGuildAddMember;

typedef struct SPacketDGGuildMember
{
	DWORD	dwPID;
	DWORD	dwGuild;
	BYTE	bGrade;
	BYTE	isGeneral;
	BYTE	bJob;
	BYTE	bLevel;
	DWORD	dwOffer;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketDGGuildMember;

typedef struct SPacketGuildWar
{
	BYTE	bType;
	BYTE	bWar;
	DWORD	dwGuildFrom;
	DWORD	dwGuildTo;
	long	lWarPrice;
	long	lInitialScore;
} TPacketGuildWar;



typedef struct SPacketGuildWarScore
{
	DWORD dwGuildGainPoint;
	DWORD dwGuildOpponent;
	long lScore;
	long lBetScore;
} TPacketGuildWarScore;

typedef struct SRefineMaterial
{
	DWORD vnum;
	int count;
} TRefineMaterial;

typedef struct SRefineTable
{
	//DWORD src_vnum;
	//DWORD result_vnum;
	DWORD id;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT material_count;
#else
	BYTE material_count;
#endif
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t cost;
#else
	int cost;
#endif
	int prob;
	TRefineMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TRefineTable;

typedef struct SBanwordTable
{
	char szWord[BANWORD_MAX_LEN + 1];
} TBanwordTable;

typedef struct SPacketGDChangeName
{
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGDChangeName;

typedef struct SPacketDGChangeName
{
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketDGChangeName;

typedef struct SPacketGuildLadder
{
	DWORD dwGuild;
	long lLadderPoint;
	long lWin;
	long lDraw;
	long lLoss;
} TPacketGuildLadder;

typedef struct SPacketGuildLadderPoint
{
	DWORD dwGuild;
	long lChange;
} TPacketGuildLadderPoint;

typedef struct SPacketGuildUseSkill
{
	DWORD dwGuild;
	DWORD dwSkillVnum;
	DWORD dwCooltime;
} TPacketGuildUseSkill;

typedef struct SPacketGuildSkillUsableChange
{
	DWORD dwGuild;
	DWORD dwSkillVnum;
	BYTE bUsable;
} TPacketGuildSkillUsableChange;

typedef struct SPacketGDLoginKey
{
	DWORD dwAccountID;
	DWORD dwLoginKey;
} TPacketGDLoginKey;

typedef struct SPacketGDAuthLogin
{
	DWORD	dwID;
	DWORD	dwLoginKey;
	char	szLogin[LOGIN_MAX_LEN + 1];
	char	szSocialID[SOCIAL_ID_MAX_LEN + 1];
	DWORD	adwClientKey[4];
	int		iPremiumTimes[PREMIUM_MAX_NUM];
#ifdef ENABLE_HWID
	char hwid[64 + 1];
	DWORD hwidID[2];
	DWORD descHwidID;
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketGDAuthLogin;

typedef struct SPacketGDLoginByKey
{
	char	szLogin[LOGIN_MAX_LEN + 1];
	DWORD	dwLoginKey;
	DWORD	adwClientKey[4];
	char	szIP[MAX_HOST_LENGTH + 1];
} TPacketGDLoginByKey;


typedef struct SPacketGiveGuildPriv
{
	BYTE type;
	int value;
	DWORD guild_id;
	time_t duration_sec;
} TPacketGiveGuildPriv;
typedef struct SPacketGiveEmpirePriv
{
	BYTE type;
	int value;
	BYTE empire;
	time_t duration_sec;
} TPacketGiveEmpirePriv;
typedef struct SPacketGiveCharacterPriv
{
	BYTE type;
	int value;
	DWORD pid;
} TPacketGiveCharacterPriv;
typedef struct SPacketRemoveGuildPriv
{
	BYTE type;
	DWORD guild_id;
} TPacketRemoveGuildPriv;
typedef struct SPacketRemoveEmpirePriv
{
	BYTE type;
	BYTE empire;
} TPacketRemoveEmpirePriv;

typedef struct SPacketDGChangeCharacterPriv
{
	BYTE type;
	int value;
	DWORD pid;
	BYTE bLog;
} TPacketDGChangeCharacterPriv;


typedef struct SPacketDGChangeGuildPriv
{
	BYTE type;
	int value;
	DWORD guild_id;
	BYTE bLog;
	time_t end_time_sec;
} TPacketDGChangeGuildPriv;

typedef struct SPacketDGChangeEmpirePriv
{
	BYTE type;
	int value;
	BYTE empire;
	BYTE bLog;
	time_t end_time_sec;
} TPacketDGChangeEmpirePriv;

typedef struct SPacketGDGuildMoney
{
	DWORD dwGuild;
	INT iGold;
} TPacketGDGuildMoney;

typedef struct SPacketDGGuildMoneyChange
{
	DWORD dwGuild;
	INT iTotalGold;
} TPacketDGGuildMoneyChange;

typedef struct SPacketDGGuildMoneyWithdraw
{
	DWORD dwGuild;
	INT iChangeGold;
} TPacketDGGuildMoneyWithdraw;

typedef struct SPacketGDGuildMoneyWithdrawGiveReply
{
	DWORD dwGuild;
	INT iChangeGold;
	BYTE bGiveSuccess;
} TPacketGDGuildMoneyWithdrawGiveReply;

typedef struct SPacketSetEventFlag
{
	char	szFlagName[EVENT_FLAG_NAME_MAX_LEN + 1];
	long	lValue;
} TPacketSetEventFlag;


typedef struct SPacketLoginOnSetup
{
	DWORD   dwID;
	char    szLogin[LOGIN_MAX_LEN + 1];
	char    szSocialID[SOCIAL_ID_MAX_LEN + 1];
	char    szHost[MAX_HOST_LENGTH + 1];
	DWORD   dwLoginKey;
	DWORD   adwClientKey[4];
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketLoginOnSetup;

typedef struct SPacketGDCreateObject
{
	DWORD	dwVnum;
	DWORD	dwLandID;
	INT		lMapIndex;
	INT	 	x, y;
	float	xRot;
	float	yRot;
	float	zRot;
} TPacketGDCreateObject;

typedef struct SGuildReserve
{
	DWORD       dwID;
	DWORD       dwGuildFrom;
	DWORD       dwGuildTo;
	DWORD       dwTime;
	BYTE        bType;
	long        lWarPrice;
	long        lInitialScore;
	bool        bStarted;
	DWORD	dwBetFrom;
	DWORD	dwBetTo;
	long	lPowerFrom;
	long	lPowerTo;
	long	lHandicap;
} TGuildWarReserve;

typedef struct
{
	DWORD	dwWarID;
	char	szLogin[LOGIN_MAX_LEN + 1];
	DWORD	dwGold;
	DWORD	dwGuild;
} TPacketGDGuildWarBet;

// Marriage

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
	time_t tMarryTime;
	char szName1[CHARACTER_NAME_MAX_LEN + 1];
	char szName2[CHARACTER_NAME_MAX_LEN + 1];
} TPacketMarriageAdd;

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
	INT  iLovePoint;
	BYTE  byMarried;
} TPacketMarriageUpdate;

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketMarriageRemove;

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingRequest;

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
	DWORD dwMapIndex;
} TPacketWeddingReady;

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingStart;

typedef struct
{
	DWORD dwPID1;
	DWORD dwPID2;
} TPacketWeddingEnd;


typedef struct SPacketMyshopPricelistHeader
{
	DWORD	dwOwnerID;
	BYTE	byCount;
} TPacketMyshopPricelistHeader;


typedef struct SItemPriceInfo
{
	DWORD	dwVnum;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	dwPrice;
#else
	DWORD	dwPrice;
#endif
} TItemPriceInfo;

typedef struct SItemPriceListTable
{
	DWORD	dwOwnerID;
	BYTE	byCount;

	TItemPriceInfo	aPriceInfo[SHOP_PRICELIST_MAX_NUM];
} TItemPriceListTable;

typedef struct
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	long lDuration;
} TPacketBlockChat;

//ADMIN_MANAGER
typedef struct TAdminInfo
{
	int m_ID;
	char m_szAccount[32];
	char m_szName[32];
	char m_szContactIP[16];
	char m_szServerIP[16];
	int m_Authority;
} tAdminInfo;
//END_ADMIN_MANAGER

//BOOT_LOCALIZATION
struct tLocale
{
	char szValue[32];
	char szKey[32];
};
//BOOT_LOCALIZATION

//RELOAD_ADMIN
typedef struct SPacketReloadAdmin
{
	char szIP[16];
} TPacketReloadAdmin;
//END_RELOAD_ADMIN


typedef struct tChangeGuildMaster
{
	DWORD dwGuildID;
	DWORD idFrom;
	DWORD idTo;
} TPacketChangeGuildMaster;

typedef struct tItemIDRange
{
	DWORD dwMin;
	DWORD dwMax;
	DWORD dwUsableItemIDMin;
} TItemIDRangeTable;

typedef struct tUpdateHorseName
{
	DWORD dwPlayerID;
	char szHorseName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketUpdateHorseName;

typedef struct tDC
{
	char	login[LOGIN_MAX_LEN + 1];
} TPacketDC;

typedef struct tNeedLoginLogInfo
{
	DWORD dwPlayerID;
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketNeedLoginLogInfo;


typedef struct tItemAwardInformer
{
	char	login[LOGIN_MAX_LEN + 1];
	char	command[20];
	unsigned int vnum;
} TPacketItemAwardInfromer;

typedef struct tDeleteAwardID
{
	DWORD dwID;
} TPacketDeleteAwardID;

typedef struct SChannelStatus
{
	short nPort;
	BYTE bStatus;
} TChannelStatus;

#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
typedef struct
{
	long lMapIndex;
	int channel;
} TPacketChangeChannel;

typedef struct
{
	long lAddr;
	WORD port;
} TPacketReturnChannel;
#endif

#ifdef ENABLE_OFFLINE_SHOP
typedef struct SPacketGDNewOfflineShop
{
	BYTE bSubHeader;
} TPacketGDNewOfflineShop;

typedef struct SPacketDGNewOfflineShop
{
	BYTE bSubHeader;
} TPacketDGNewOfflineShop;

namespace offlineshop
{
	enum class ExpirationType
	{
		EXPIRE_NONE,
		EXPIRE_REAL_TIME,
		EXPIRE_REAL_TIME_FIRST_USE,
	};	

	typedef struct SPriceInfo
	{
		long long illYang;
		SPriceInfo() : illYang(0)
		{}

		bool operator < (const SPriceInfo& rItem) const
		{
			return GetTotalYangAmount() < rItem.GetTotalYangAmount();
		}

		long long GetTotalYangAmount() const
		{
			long long total = illYang;
			return total;
		}
	} TPriceInfo;

#ifdef ENABLE_OFFLINE_SHOP_GRID
	typedef struct SItemDisplayPos
	{
		BYTE pos;
		BYTE page;
	}TItemDisplayPos;
#endif

	typedef struct SItemInfoEx
	{
		DWORD dwVnum;
		DWORD dwCount;
		long alSockets[ITEM_SOCKET_MAX_NUM];
		TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
#ifdef __ENABLE_CHANGELOOK_SYSTEM__
		DWORD dwTransmutation;
#endif
		ExpirationType expiration;
#ifdef ENABLE_OFFLINE_SHOP_GRID
		TItemDisplayPos display_pos;
#endif
	} TItemInfoEx;

	typedef struct SItemInfo
	{
		DWORD dwOwnerID, dwItemID;
		TPriceInfo price;
		TItemInfoEx item;
		char dwOwnerName[CHARACTER_NAME_MAX_LEN + 1];
	} TItemInfo;

	typedef struct SShopInfo
	{
		DWORD dwOwnerID;
		DWORD dwDuration;
		char szName[OFFLINE_SHOP_NAME_MAX_LEN];
#ifdef ENABLE_SHOP_DECORATION
		DWORD	dwShopDecoration;
#endif
		DWORD dwCount;
	} TShopInfo;

#ifdef ENABLE_IRA_REWORK 
	typedef struct SShopPosition
	{
		long lMapIndex;
		long x, y;
		BYTE bChannel;
	} TShopPosition;
#endif

	enum eNewOfflineshopSubHeaderGD
	{
		SUBHEADER_GD_BUY_ITEM = 0,
		SUBHEADER_GD_BUY_LOCK_ITEM,
		SUBHEADER_GD_CANNOT_BUY_LOCK_ITEM,
		SUBHEADER_GD_EDIT_ITEM,
		SUBHEADER_GD_REMOVE_ITEM,
		SUBHEADER_GD_ADD_ITEM,
		SUBHEADER_GD_SHOP_FORCE_CLOSE,
		SUBHEADER_GD_SHOP_CREATE_NEW,
		SUBHEADER_GD_SHOP_CHANGE_NAME,
		SUBHEADER_GD_SAFEBOX_GET_ITEM,
		SUBHEADER_GD_SAFEBOX_GET_VALUTES,
		SUBHEADER_GD_SAFEBOX_ADD_ITEM,
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
		SUBHEADER_GD_SHOP_EXTEND_TIME,
#endif
	};

	typedef struct SSubPacketGDBuyItem
	{
		DWORD dwOwnerID, dwItemID, dwGuestID;
	} TSubPacketGDBuyItem;

	typedef struct SSubPacketGDLockBuyItem
	{
		DWORD dwOwnerID, dwItemID, dwGuestID;
	} TSubPacketGDLockBuyItem;

	typedef struct SSubPacketGDCannotBuyLockItem
	{
		DWORD dwOwnerID, dwItemID;
	} TSubPacketGDCannotBuyLockItem;

	typedef struct SSubPacketGDEditItem
	{
		DWORD dwOwnerID, dwItemID;
		TPriceInfo priceInfo;
		bool allEdit;
	} TSubPacketGDEditItem;

	typedef struct SSubPacketGDRemoveItem
	{
		DWORD dwOwnerID;
		DWORD dwItemID;
	} TSubPacketGDRemoveItem;

	typedef struct SSubPacketGDAddItem
	{
		DWORD dwOwnerID;
		TItemInfo itemInfo;
	} TSubPacketGDAddItem;

	typedef struct SSubPacketGDShopForceClose
	{
		DWORD dwOwnerID;
	} TSubPacketGDShopForceClose;

	typedef struct SSubPacketGDShopCreateNew
	{
		TShopInfo shop;
#ifdef ENABLE_IRA_REWORK
		TShopPosition pos;
#endif
	} TSubPacketGDShopCreateNew;

	typedef struct SSubPacketGDShopChangeName
	{
		DWORD dwOwnerID;
		char szName[OFFLINE_SHOP_NAME_MAX_LEN];
	} TSubPacketGDShopChangeName;

	typedef struct SSubPacketGDSafeboxGetItem
	{
		DWORD dwOwnerID;
		DWORD dwItemID;
	} TSubPacketGDSafeboxGetItem;

	typedef struct SSubPacketGDSafeboxAddItem
	{
		DWORD			dwOwnerID;
		TItemInfoEx		item;
	} TSubPacketGDSafeboxAddItem;

	typedef struct SSubPacketGDSafeboxGetValutes
	{
		DWORD dwOwnerID;
		unsigned long long valute;
	} TSubPacketGDSafeboxGetValutes;

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	typedef struct extendTimeGameDB
	{
		DWORD	dwOwnerID;
		DWORD	dwTime;
	} TSubPacketGDShopExtendTime;

	typedef struct extendTimeDBGame
	{
		DWORD dwOwnerID;
		DWORD dwTime;
	} TSubPacketDGShopExtendTime;
#endif

	enum eSubHeaderDGNewOfflineshop
	{
		SUBHEADER_DG_BUY_ITEM,
		SUBHEADER_DG_LOCKED_BUY_ITEM,
		SUBHEADER_DG_EDIT_ITEM,
		SUBHEADER_DG_REMOVE_ITEM,
		SUBHEADER_DG_ADD_ITEM,
		SUBHEADER_DG_SHOP_FORCE_CLOSE,
		SUBHEADER_DG_SHOP_CREATE_NEW,
		SUBHEADER_DG_SHOP_CHANGE_NAME,
		SUBHEADER_DG_SHOP_EXPIRED,
		SUBHEADER_DG_LOAD_TABLES,
		SUBHEADER_DG_SAFEBOX_ADD_ITEM,
		SUBHEADER_DG_SAFEBOX_ADD_VALUTES,
		SUBHEADER_DG_SAFEBOX_LOAD,
		SUBHEADER_DG_SAFEBOX_EXPIRED_ITEM,
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
		SUBHEADER_DG_SHOP_EXTEND_TIME,
#endif
	};

	typedef struct SSubPacketDGBuyItem
	{
		DWORD dwOwnerID, dwItemID, dwBuyerID;
	} TSubPacketDGBuyItem;

	typedef struct SSubPacketDGLockedBuyItem
	{
		DWORD dwOwnerID, dwItemID, dwBuyerID;
	} TSubPacketDGLockedBuyItem;

	typedef struct SSubPacketDGEditItem
	{
		DWORD dwOwnerID, dwItemID;
		TPriceInfo price;
	} TSubPacketDGEditItem;

	typedef struct SSubPacketDGRemoveItem
	{
		DWORD dwOwnerID, dwItemID;
	} TSubPacketDGRemoveItem;

	typedef struct SSubPacketDGAddItem
	{
		DWORD dwOwnerID, dwItemID;
		TItemInfo item;
	} TSubPacketDGAddItem;

	typedef struct SSubPacketDGShopForceClose
	{
		DWORD dwOwnerID;
	} TSubPacketDGShopForceClose;

	typedef struct SSubPacketDGShopCreateNew
	{
		TShopInfo shop;
#ifdef ENABLE_IRA_REWORK
		TShopPosition pos;
#endif
	} TSubPacketDGShopCreateNew;

	typedef struct SSubPacketDGShopChangeName
	{
		DWORD dwOwnerID;
		char szName[OFFLINE_SHOP_NAME_MAX_LEN];
	} TSubPacketDGShopChangeName;

	typedef struct SSubPacketDGLoadTables
	{
		DWORD dwShopCount;
	} TSubPacketDGLoadTables;

	typedef struct SSubPacketDGShopExpired
	{
		DWORD dwOwnerID;
	} TSubPacketDGShopExpired;

	typedef struct SSubPacketDGSafeboxAddItem
	{
		DWORD dwOwnerID, dwItemID;
		TItemInfoEx item;
	} TSubPacketDGSafeboxAddItem;

	typedef struct SSubPacketDGSafeboxAddValutes
	{
		DWORD dwOwnerID;
		unsigned long long valute;
	} TSubPacketDGSafeboxAddValutes;

	typedef struct SSubPacketDGSafeboxLoad
	{
		DWORD dwOwnerID;
		unsigned long long valute;
		DWORD dwItemCount;
	} TSubPacketDGSafeboxLoad;

	typedef struct SSubPacketDGSafeboxExpiredItem
	{
		DWORD dwOwnerID;
		DWORD dwItemID;
	} TSubPacketDGSafeboxExpiredItem;
}
#endif

#ifdef ENABLE_SWITCHBOT
struct TSwitchbotAttributeAlternativeTable
{
	TPlayerItemAttribute attributes[MAX_NORM_ATTR_NUM];

	bool IsConfigured() const
	{
		for (const auto& it : attributes)
		{
			if (it.bType && it.sValue)
			{
				return true;
			}
		}

		return false;
	}
};

struct TSwitchbotTable
{
	uint32_t player_id;
	bool active[SWITCHBOT_SLOT_COUNT];
	bool finished[SWITCHBOT_SLOT_COUNT];
	uint32_t items[SWITCHBOT_SLOT_COUNT];
	TSwitchbotAttributeAlternativeTable alternatives[SWITCHBOT_SLOT_COUNT][SWITCHBOT_ALTERNATIVE_COUNT];

	TSwitchbotTable() : player_id(0)
	{
		memset(&items, 0, sizeof(items));
		memset(&alternatives, 0, sizeof(alternatives));
		memset(&active, false, sizeof(active));
		memset(&finished, false, sizeof(finished));
	}
};

struct TSwitchbottAttributeTable
{
	uint8_t attribute_set;
	int32_t apply_num;
	uint32_t max_value;
};
#endif

#ifdef ENABLE_CHEST_OPEN_RENEWAL
typedef struct ChestItem
{
	DWORD vnum;
	DWORD count;
} TChestGiveItem;

typedef struct ChestOpenEmpetyInventory
{
	int book = 0;
	int upgrade = 0;
	int stone = 0;
	int chest = 0;
	int norm = 0;
} TChestEmpetInventory;

typedef struct ChestOpenEmpetyInventorySize
{
	int size2 = 0;
	int size3 = 0;
} TChestEmpetInventorySize;

enum ChestRework
{
	CHEST_TYPE_STONE,
	CHEST_TYPE_NEWPCT,
	CHEST_TYPE_NORM,
	CHEST_OPEN_COUNT_CONTROL = 200,
	CHEST_OPEN_COUNT_CONTROL2 = 50,
	MAX_CHEST_OPEN_COUNT = 10000,
	MIN_CHEST_OPEN_COUNT_PCT = 5,
	MIN_CHEST_OPEN_COUNT_NORM = 25,
	MIN_CHEST_OPEN_COUNT_STONE = 20,
	MAX_CHEST_OPEN_PCT = 500,
	MAX_CHEST_OPEN_NORM = 20000,
	MAX_CHEST_OPEN_STONE = 50000,
	OPEN_COUNT_CONTROL_PCT = 10,
	OPEN_COUNT_CONTROL_NORM = 200,
	OPEN_COUNT_CONTROL_STONE = 200,
};
#endif

#ifdef ENABLE_GLOBAL_RANKING
enum RankingConf
{
	RANKING_MAX_PLAYER_COUNT = 10,
#ifdef ENABLE_DIZCIYA_GOTTEN
	CACHE_TIME = 60,
#else
	CACHE_TIME = 60 * 10,
#endif

	RANKING_TOP = 0,
	RANKING_PLAYER
};

typedef struct SRankingInfo
{
	uint8_t bOrder;
	char szPlayerName[12 + 1];
	DWORD llScore;
	BYTE country;
	BYTE playerRace;
}TRankingInfo;

typedef struct SRankingData
{
	TRankingInfo rank[RANKING_MAX][RANKING_MAX_PLAYER_COUNT];
}TRankingData;

typedef struct SRankigDGPacket
{
	BYTE header;
	size_t size;
	BYTE subHeader;
}TRankigDGPacket;

typedef struct SRanking
{
	DWORD pid;
	DWORD value;
}TRanking;

typedef struct SRankingList
{
	WORD order[StatsTypes::RANKING_MAX];
	DWORD value[StatsTypes::RANKING_MAX];
}TRankingList;

typedef struct SRankingMeGC
{
	char playerName[12 + 1];
	BYTE country;
	BYTE playerRace;
	TRankingList rank;
}TRankingMeGC;
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
namespace InGameLog
{
	typedef struct SItemInfo
	{
		DWORD dwVnum;
		DWORD dwCount;
		long alSockets[ITEM_SOCKET_MAX_NUM];
		TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	}TItemInfo;

	typedef struct SOfflineShopSoldLog
	{
		TItemInfo item;
		unsigned long long sellPrice;
		char buyerName[24];
		char soldDate[24];
	}TOfflineShopSoldLog;

	typedef struct STradeLogCharacterInfo
	{
		DWORD pid;
		char name[24];
		uint64_t gold;
	}TTradeLogCharacterInfo;

	typedef struct STradeLogInfo
	{
		DWORD logID;
		TTradeLogCharacterInfo owner;
		TTradeLogCharacterInfo victim;
		char tradingDate[24];
	}TTradeLogInfo;

	typedef struct STradeLogInfoAdd
	{
		TTradeLogCharacterInfo owner;
		TTradeLogCharacterInfo victim;
	}TTradeLogInfoAdd;

	typedef struct STradeLogRequestInfo
	{
		DWORD logID;
		char victimName[24];
		char tradingDate[32];
	}TTradeLogRequestInfo;

	typedef struct STradeLogDetailsRequest
	{
		DWORD logID;
		BYTE itemCount;
		uint64_t ownerGold;
		uint64_t victimGold;
	}TTradeLogDetailsRequest;

	typedef struct STradeLogRequestItem
	{
		BYTE pos;
		bool isOwner;
		TItemInfo item;
	}TTradeLogRequestItem;

	typedef struct STradeLogItemInfo
	{
		DWORD ownerID;
		BYTE pos;
		TItemInfo item;
	}TTradeLogItemInfo;

	//GD
	enum IngameLogGDsubHeader
	{
		SUBHEADER_IGL_GD_OFFLINESHOP_ADDLOG,
		SUBHEADER_IGL_GD_OFFLINESHOP_REQUESTLOG,
		SUBHEADER_IGL_GD_TRADE_ADDLOG,
		SUBHEADER_IGL_GD_TRADE_REQUESTLOG,
		SUBHEADER_IGL_GD_TRADE_DETAILS_REQUESTLOG,
	};

	typedef struct SPacketGDInGameLog
	{
		BYTE subHeader;
	}TPacketGDInGameLog;

	typedef struct SOfflineShopSoldQuery
	{
		DWORD ownerID;
		TOfflineShopSoldLog log;
	}TSubPackOfflineShopAddGD;

	typedef struct SOfflineShopRequestGD
	{
		DWORD ownerID;
	}TOfflineShopRequestGD;

	typedef struct SSubPackTradeAddGD
	{
		BYTE itemCount;
		TTradeLogCharacterInfo owner;
		TTradeLogCharacterInfo victim;
	}TSubPackTradeAddGD;

	typedef struct STradeRequestGD
	{
		DWORD ownerID;
	}TTradeRequestGD;

	typedef struct STradeDetailsRequestGD
	{
		DWORD ownerID;
		DWORD logID;
	}TTradeDetailsRequestGD;
	//DG
	enum IngameLogDGsubHeader
	{
		SUBHEADER_IGL_DG_OFFLINESHOP_REQUESTLOG,
		SUBHEADER_IGL_DG_TRADE_REQUESTLOG,
		SUBHEADER_IGL_DG_TRADE_DETAILS_REQUESTLOG,
	};

	typedef struct SPacketDGInGameLog
	{
		BYTE subHeader;
	}TPacketDGInGameLog;
	
	typedef struct SInGameLogRequestDG
	{
		DWORD ownerID;
		WORD logCount;
	}TInGameLogRequestDG;

}
#endif

#ifdef ENABLE_HWID
enum HwidSubPacketGD
{
	SUB_HEADER_GD_HWID_BAN,
#ifdef ENABLE_FARM_BLOCK
	SUB_HEADER_GD_GET_FARM_BLOCK,
	SUB_HEADER_GD_SET_FARM_BLOCK,
	SUB_HEADER_GD_FARM_BLOCK_LOGOUT,
#endif
};

enum HwidSubPacketDG
{
#ifdef ENABLE_FARM_BLOCK
	SUB_HEADER_DG_GET_FARM_BLOCK,
#endif
};

typedef struct SHwidGDPacket
{
	BYTE subHeader;
}THwidGDPacket;

typedef struct SHwidDGPacket
{
	BYTE subHeader;
}THwidDGPacket;

typedef struct SHwidString
{
	char hwid[64 + 1];
}THwidString;

#ifdef ENABLE_FARM_BLOCK
typedef struct SFarmBlockGet
{
	DWORD descHwidID;
	DWORD pid;
}TFarmBlockGet;

typedef struct SFarmBlockGD
{
	DWORD pid;
	bool farmBlock;
	BYTE result;
}TFarmBlockGD;

typedef struct SChangeFarmBlock
{
	TFarmBlockGet info;
	bool farmBlock;
}TChangeFarmBlock;
#endif

#endif

#ifdef ENABLE_ALIGNMENT_SYSTEM
typedef struct SAlignmentTable
{
	int minPoint, maxPoint;
	std::vector<std::pair<BYTE, long long>>bonus;
	SAlignmentTable()
	{
		bonus.clear();
	}
}TAlignmentTable;
#endif

#ifdef ENABLE_DUNGEON_INFO
enum DungeonInfoError
{
	DUNGEON_ERROR_LEVEL,
	DUNGEON_ERROR_TICKET,
	DUNGEON_ERROR_QTIME,//Quest time
	DUNGEON_ERROR_BUSY,//
	DUNGEON_ERROR_UNKNOWN
};
typedef struct SDungeonInfoTable
{
	int cooldown;
	DWORD bossVnum;
	DWORD ticketVnum;
	BYTE minLevel;
	BYTE maxLevel;
	long mapIdx, x, y;

	SDungeonInfoTable()
	{
		cooldown = 0;
		bossVnum = 0;
		ticketVnum = 0;
		minLevel = 0;
		maxLevel = 0;
		mapIdx = 0;
		x = 0;
		y = 0;
	}

	bool levelControl(BYTE level) const
	{
		if (level < minLevel || level > maxLevel)
			return false;
		return true;
	}

}TDungeonInfoTable;
#endif


#ifdef ENABLE_EVENT_MANAGER
typedef struct event_struct_
{
	WORD	eventID;
	BYTE	eventIndex;
	int		startTime;
	int		endTime;
	BYTE	empireFlag;
	BYTE	channelFlag;
	DWORD	value[4];
	bool	eventStatus;
	bool	eventTypeOnlyStart;
	char	startTimeText[25];
	char	endTimeText[25];
}TEventManagerData;

enum
{
	EVENT_MANAGER_LOAD,
	EVENT_MANAGER_EVENT_STATUS,
	EVENT_MANAGER_REMOVE_EVENT,
	EVENT_MANAGER_UPDATE,

	DOUBLE_METIN_LOOT_EVENT = 1,
	DOUBLE_BOSS_LOOT_EVENT = 2,

	WHELL_OF_FORTUNE_EVENT = 3,
	EXP_EVENT = 4,

	LETTER_EVENT = 5,
	OKEY_CARD_EVENT = 6,
	CATCH_KING_EVENT = 7,

	BLACK_N_WHITE_EVENT = 8,
	YUTNORI_EVENT = 9,

	MOONLIGHT_EVENT = 10,
	SOCCER_BALL_EVENT = 11,

	HALLOWEEN_EVENT = 12,
	RAMADAN_EVENT = 13,
	SUMMER_EVENT = 14,
	CHRISTMAS_EVENT = 15,
	EASTER_EVENT = 16,
	VALENTINES_EVENT = 17,

	BEGGINER_EVENT = 18,

	EMPTY_PERMANENT_EVENT = 19,

};

#endif

#ifdef ENABLE_ITEMSHOP
enum itemShopCommands
{
	ITEMSHOP_LOAD,
	ITEMSHOP_LOG,
	ITEMSHOP_BUY,
	ITEMSHOP_DRAGONCOIN,
	ITEMSHOP_RELOAD,
	ITEMSHOP_LOG_ADD,
	ITEMSHOP_UPDATE_ITEM,
};

typedef struct SIShopData
{
	DWORD	id;
	DWORD	itemVnum;
	long long	itemPrice;
	int		topSellingIndex;
	BYTE	discount;
	int		offerTime;
	int		addedTime;
	long long	sellCount;
	int	week_limit;
	int	month_limit;
	int maxSellCount;
	long long itemUntradeablePrice;
	DWORD itemVnumUntradeable;
}TIShopData;

typedef struct SIShopLogData
{
	DWORD	accountID;
	char	playerName[CHARACTER_NAME_MAX_LEN+1];
	char	buyDate[21];
	int		buyTime;
	char	ipAdress[16];
	DWORD	itemID;
	DWORD	itemVnum;
	int		itemCount;
	long long	itemPrice;
}TIShopLogData;
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
typedef struct SSkillColor
{
	DWORD player_id;
	DWORD dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
} TSkillColor;
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
typedef struct SPetSkill
{
	BYTE type;
	BYTE level;
}TPetSkill;

typedef struct SNewPetInfo
{
	BYTE level;
	DWORD exp;
	DWORD nextExp;
	BYTE evolution;
	BYTE type;
	BYTE star;
	DWORD skinVnum;
	BYTE skinCount;
	WORD bonus[3];
	TPetSkill skill[8];
}TNewPetInfo;
#endif

#ifdef ENABLE_NAMING_SCROLL
enum ENamingScroll
{
	MOUNT_NAME_NUM,
	PET_NAME_NUM,
	BUFFI_NAME_NUM,
	NAME_SCROLL_MAX_NUM,
	NAMING_SCROLL_BONUS_RATE_VALUE = 0,
	NAMING_SCROLL_TIME_VALUE = 1,
	
};
#endif

#ifdef ENABLE_ITEM_SELL_LOG
typedef struct SItemSellLog
{
	DWORD pid;
	char logType[16];
	DWORD dwVnum;
	DWORD dwCount;
	long alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
}TItemSellLog;
#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
typedef struct SRequestChangeLanguage
{
	DWORD dwAID;
	BYTE bLanguage;
} TRequestChangeLanguage;
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
typedef struct SPlayerExtBattlePassMission
{
	DWORD dwPlayerId;
	DWORD dwBattlePassType;
	DWORD dwMissionIndex;
	DWORD dwMissionType;
	DWORD dwBattlePassId;
	DWORD dwExtraInfo;
	BYTE bCompleted;
	BYTE bIsUpdated;
} TPlayerExtBattlePassMission;

typedef struct SExtBattlePassRewardItem
{
	DWORD dwVnum;
	BYTE bCount;
} TExtBattlePassRewardItem;

typedef struct SExtBattlePassMissionInfo
{
	BYTE bMissionIndex;
	BYTE bMissionType;
	DWORD dwMissionInfo[3];
	TExtBattlePassRewardItem aRewardList[3];
} TExtBattlePassMissionInfo;

typedef struct SExtBattlePassTimeTable
{
	BYTE	bBattlePassId;
	DWORD	dwStartTime;
	DWORD	dwEndTime;
} TExtBattlePassTimeTable;
#endif

#ifdef ENABLE_RENEWAL_PVP
enum
{
	PVP_CRITICAL_DAMAGE_SKILLS,
	PVP_POISONING,
	PVP_HALF_HUMAN,
	PVP_BUFFI_SKILLS,
	PVP_MISS_HITS,
	PVP_DISPEL_EFFECTS,
	PVP_HP_ELIXIR,
	PVP_WHITE_DEW,
	PVP_YELLOW_DEW,
	PVP_ORANGE_DEW,
	PVP_RED_DEW,
	PVP_BLUE_DEW,
	PVP_GREEN_DEW,
	PVP_ZIN_WATER,
	PVP_SAMBO_WATER,
	PVP_ATTACKSPEED_FISH,
	PVP_DRAGON_GOD_ATTACK,
	PVP_DRAGON_GOD_DEFENCE,
	PVP_DRAGON_GOD_LIFE,
	PVP_PIERCING_STRIKE,
	PVP_CRITICAL_STRIKE,
	PVP_PET,
	PVP_NEW_PET,
	PVP_ENERGY,
	PVP_BET,
	PVP_MAX,
};
#endif

#pragma pack()
#endif

