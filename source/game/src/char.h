#ifndef __INC_METIN_II_CHAR_H__
#define __INC_METIN_II_CHAR_H__

#include <boost/unordered_map.hpp>
#include "../../common/stl.h"
#include "entity.h"
#include "FSM.h"
#include "horse_rider.h"
#include "vid.h"
#include "constants.h"
#include "affect.h"
#include "affect_flag.h"
#ifndef ENABLE_CUBE_RENEWAL_WORLDARD
#include "cube.h"
#else
#include "cuberenewal.h"
#endif
#include "mining.h"
#include "../../common/Controls.h"
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
#include <vector>
#endif

#define ENABLE_ANTI_CMD_FLOOD
#define ENABLE_OPEN_SHOP_WITH_ARMOR
enum eMountType { MOUNT_TYPE_NONE = 0, MOUNT_TYPE_NORMAL = 1, MOUNT_TYPE_COMBAT = 2, MOUNT_TYPE_MILITARY = 3 };
eMountType GetMountLevelByVnum(DWORD dwMountVnum, bool IsNew);
const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);

class CBuffOnAttributes;
class CPetSystem;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
class CMountSystem;
#endif

#ifdef ENABLE_OFFLINE_SHOP
namespace offlineshop
{
	class CShop;
	class CShopSafebox;
}
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
#include "biolog_system.h"
class BiologSystem;
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
class CNewPet;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
class CBuffiSystem;
#endif

#define INSTANT_FLAG_DEATH_PENALTY		(1 << 0)
#define INSTANT_FLAG_SHOP			(1 << 1)
#define INSTANT_FLAG_EXCHANGE			(1 << 2)
#define INSTANT_FLAG_STUN			(1 << 3)
#define INSTANT_FLAG_NO_REWARD			(1 << 4)

#define AI_FLAG_NPC				(1 << 0)
#define AI_FLAG_AGGRESSIVE			(1 << 1)
#define AI_FLAG_HELPER				(1 << 2)
#define AI_FLAG_STAYZONE			(1 << 3)

extern int g_nPortalLimitTime;

enum
{
	MAIN_RACE_WARRIOR_M,
	MAIN_RACE_ASSASSIN_W,
	MAIN_RACE_SURA_M,
	MAIN_RACE_SHAMAN_W,
	MAIN_RACE_WARRIOR_W,
	MAIN_RACE_ASSASSIN_M,
	MAIN_RACE_SURA_W,
	MAIN_RACE_SHAMAN_M,
#ifdef ENABLE_WOLFMAN_CHARACTER
	MAIN_RACE_WOLFMAN_M,
#endif
	MAIN_RACE_MAX_NUM,
};

enum
{
	POISON_LENGTH = 30,
#ifdef ENABLE_WOLFMAN_CHARACTER
	BLEEDING_LENGTH = 30,
#endif
	STAMINA_PER_STEP = 1,
	SAFEBOX_PAGE_SIZE = 9,
	AI_CHANGE_ATTACK_POISITION_TIME_NEAR = 10000,
	AI_CHANGE_ATTACK_POISITION_TIME_FAR = 1000,
	AI_CHANGE_ATTACK_POISITION_DISTANCE = 100,
	SUMMON_MONSTER_COUNT = 3,
};

enum
{
	FLY_NONE,
	FLY_EXP,
	FLY_HP_MEDIUM,
	FLY_HP_BIG,
	FLY_SP_SMALL,
	FLY_SP_MEDIUM,
	FLY_SP_BIG,
	FLY_FIREWORK1,
	FLY_FIREWORK2,
	FLY_FIREWORK3,
	FLY_FIREWORK4,
	FLY_FIREWORK5,
	FLY_FIREWORK6,
	FLY_FIREWORK_CHRISTMAS,
	FLY_CHAIN_LIGHTNING,
	FLY_HP_SMALL,
	FLY_SKILL_MUYEONG,
};

enum EDamageType
{
	DAMAGE_TYPE_NONE,
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_NORMAL_RANGE,
	DAMAGE_TYPE_MELEE,
	DAMAGE_TYPE_RANGE,
	DAMAGE_TYPE_FIRE,
	DAMAGE_TYPE_ICE,
	DAMAGE_TYPE_ELEC,
	DAMAGE_TYPE_MAGIC,
	DAMAGE_TYPE_POISON,
	DAMAGE_TYPE_SPECIAL,
#ifdef ENABLE_WOLFMAN_CHARACTER
	DAMAGE_TYPE_BLEEDING,
#endif
};

enum DamageFlag
{
	DAMAGE_NORMAL = (1 << 0),
	DAMAGE_POISON = (1 << 1),
	DAMAGE_DODGE = (1 << 2),
	DAMAGE_BLOCK = (1 << 3),
	DAMAGE_PENETRATE = (1 << 4),
	DAMAGE_CRITICAL = (1 << 5),
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	DAMAGE_BLEEDING = (1 << 6),
#endif
};

enum EPointTypes
{
	POINT_NONE					= 0,
	POINT_LEVEL					= 1,
	POINT_VOICE					= 2,
	POINT_EXP					= 3,
	POINT_NEXT_EXP				= 4,
	POINT_HP					= 5,
	POINT_MAX_HP				= 6,
	POINT_SP					= 7,
	POINT_MAX_SP				= 8,
	POINT_STAMINA				= 9,
	POINT_MAX_STAMINA			= 10,
	POINT_GOLD					= 11,
	POINT_ST					= 12,
	POINT_HT					= 13,
	POINT_DX					= 14,
	POINT_IQ					= 15,
	POINT_DEF_GRADE				= 16,
	POINT_ATT_SPEED				= 17,
	POINT_ATT_GRADE				= 18,
	POINT_MOV_SPEED				= 19,
	POINT_CLIENT_DEF_GRADE		= 20,
	POINT_CASTING_SPEED			= 21,
	POINT_MAGIC_ATT_GRADE		= 22,
	POINT_MAGIC_DEF_GRADE		= 23,
	POINT_EMPIRE_POINT			= 24,
	POINT_LEVEL_STEP			= 25,
	POINT_STAT					= 26,
	POINT_SUB_SKILL				= 27,
	POINT_SKILL					= 28,
	POINT_WEAPON_MIN			= 29,
	POINT_WEAPON_MAX			= 30,
	POINT_PLAYTIME				= 31,
	POINT_HP_REGEN				= 32,
	POINT_SP_REGEN				= 33,
	POINT_BOW_DISTANCE			= 34,
	POINT_HP_RECOVERY			= 35,
	POINT_SP_RECOVERY			= 36,
	POINT_POISON_PCT			= 37,
	POINT_STUN_PCT				= 38,
	POINT_SLOW_PCT				= 39,
	POINT_CRITICAL_PCT			= 40,
	POINT_PENETRATE_PCT			= 41,
	POINT_CURSE_PCT				= 42,
	POINT_ATTBONUS_HUMAN		= 43,
	POINT_ATTBONUS_ANIMAL		= 44,
	POINT_ATTBONUS_ORC			= 45,
	POINT_ATTBONUS_MILGYO		= 46,
	POINT_ATTBONUS_UNDEAD		= 47,
	POINT_ATTBONUS_DEVIL		= 48,
	POINT_ATTBONUS_INSECT		= 49,
	POINT_ATTBONUS_FIRE			= 50,
	POINT_ATTBONUS_ICE			= 51,
	POINT_ATTBONUS_DESERT		= 52,
	POINT_ATTBONUS_MONSTER		= 53,
	POINT_ATTBONUS_WARRIOR		= 54,
	POINT_ATTBONUS_ASSASSIN		= 55,
	POINT_ATTBONUS_SURA			= 56,
	POINT_ATTBONUS_SHAMAN		= 57,
	POINT_ATTBONUS_TREE			= 58,
	POINT_RESIST_WARRIOR		= 59,
	POINT_RESIST_ASSASSIN		= 60,
	POINT_RESIST_SURA			= 61,
	POINT_RESIST_SHAMAN			= 62,
	POINT_STEAL_HP				= 63,
	POINT_STEAL_SP				= 64,
	POINT_MANA_BURN_PCT			= 65,
	POINT_DAMAGE_SP_RECOVER		= 66,
	POINT_BLOCK					= 67,
	POINT_DODGE					= 68,
	POINT_RESIST_SWORD			= 69,
	POINT_RESIST_TWOHAND		= 70,
	POINT_RESIST_DAGGER			= 80,
	POINT_RESIST_BELL			= 81,
	POINT_RESIST_FAN			= 82,
	POINT_RESIST_BOW			= 83,
	POINT_RESIST_FIRE			= 84,
	POINT_RESIST_ELEC			= 85,
	POINT_RESIST_MAGIC			= 86,
	POINT_RESIST_WIND			= 87,
	POINT_REFLECT_MELEE			= 88,
	POINT_REFLECT_CURSE			= 89,
	POINT_POISON_REDUCE			= 90,
	POINT_KILL_SP_RECOVER		= 91,
	POINT_EXP_DOUBLE_BONUS		= 92,
	POINT_GOLD_DOUBLE_BONUS		= 93,
	POINT_ITEM_DROP_BONUS		= 94,
	POINT_POTION_BONUS			= 95,
	POINT_KILL_HP_RECOVERY		= 96,
	POINT_IMMUNE_STUN			= 97,
	POINT_IMMUNE_SLOW			= 98,
	POINT_IMMUNE_FALL			= 99,
	POINT_PARTY_ATTACKER_BONUS	= 100,
	POINT_PARTY_TANKER_BONUS	= 101,
	POINT_ATT_BONUS				= 102,
	POINT_DEF_BONUS				= 103,
	POINT_ATT_GRADE_BONUS		= 104,
	POINT_DEF_GRADE_BONUS		= 105,
	POINT_MAGIC_ATT_GRADE_BONUS	= 106,
	POINT_MAGIC_DEF_GRADE_BONUS	= 107,
	POINT_RESIST_NORMAL_DAMAGE	= 108,
	POINT_HIT_HP_RECOVERY		= 109,
	POINT_HIT_SP_RECOVERY		= 110,
	POINT_MANASHIELD			= 111,
	POINT_PARTY_BUFFER_BONUS	= 112,
	POINT_PARTY_SKILL_MASTER_BONUS = 113,
	POINT_HP_RECOVER_CONTINUE	= 114,
	POINT_SP_RECOVER_CONTINUE	= 115,
	POINT_STEAL_GOLD			= 116,
	POINT_POLYMORPH				= 117,
	POINT_MOUNT					= 118,
	POINT_PARTY_HASTE_BONUS		= 119,
	POINT_PARTY_DEFENDER_BONUS	= 120,
	POINT_STAT_RESET_COUNT		= 121,
	POINT_HORSE_SKILL			= 122,
	POINT_MALL_ATTBONUS			= 123,
	POINT_MALL_DEFBONUS			= 124,
	POINT_MALL_EXPBONUS			= 125,
	POINT_MALL_ITEMBONUS		= 126,
	POINT_MALL_GOLDBONUS		= 127,
	POINT_MAX_HP_PCT			= 128,
	POINT_MAX_SP_PCT			= 129,
	POINT_SKILL_DAMAGE_BONUS	= 130,
	POINT_NORMAL_HIT_DAMAGE_BONUS = 131,
	POINT_SKILL_DEFEND_BONUS	= 132,
	POINT_NORMAL_HIT_DEFEND_BONUS = 133,
	POINT_RAMADAN_CANDY_BONUS_EXP = 134,
	POINT_ENERGY				= 135,
	POINT_ENERGY_END_TIME		= 136,
	POINT_COSTUME_ATTR_BONUS	= 137,
	POINT_MAGIC_ATT_BONUS_PER	= 138,
	POINT_MELEE_MAGIC_ATT_BONUS_PER = 139,
	POINT_RESIST_ICE			= 140,
	POINT_RESIST_EARTH			= 141,
	POINT_RESIST_DARK			= 142,
	POINT_RESIST_CRITICAL		= 143,
	POINT_RESIST_PENETRATE		= 144,

#ifdef ENABLE_WOLFMAN_CHARACTER
	POINT_BLEEDING_REDUCE		= 145,
	POINT_BLEEDING_PCT			= 146,
	POINT_ATTBONUS_WOLFMAN		= 147,
	POINT_RESIST_WOLFMAN		= 148,
	POINT_RESIST_CLAW			= 149,
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	POINT_ACCEDRAIN_RATE		= 150,
#endif

#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	POINT_RESIST_MAGIC_REDUCTION = 151,
#endif
#ifdef ENABLE_EXTRA_APPLY_BONUS
	POINT_ATTBONUS_STONE		= 152,
	POINT_ATTBONUS_BOSS			= 153,
	POINT_ATTBONUS_PVM_STR		= 154,
	POINT_ATTBONUS_PVM_AVG		= 155,
	POINT_ATTBONUS_PVM_BERSERKER= 156,
	POINT_ATTBONUS_ELEMENT		= 157,
	POINT_DEFBONUS_PVM			= 158,
	POINT_DEFBONUS_ELEMENT		= 159,
	POINT_ATTBONUS_PVP			= 160,
	POINT_DEFBONUS_PVP			= 161,

	POINT_ATT_FIRE				= 162,
	POINT_ATT_ICE				= 163,
	POINT_ATT_WIND				= 164,
	POINT_ATT_EARTH				= 165,
	POINT_ATT_DARK				= 166,
	POINT_ATT_ELEC				= 167,
#endif

	POINT_REFRESH_MAX,

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	POINT_BATTLE_PASS_PREMIUM_ID,
#endif
#ifdef ENABLE_PLAYER_STATS_SYSTEM
#ifdef ENABLE_POWER_RANKING
	POINT_POWER_RANK,
#endif

	POINT_MONSTER_KILLED,
	POINT_STONE_KILLED,
	POINT_BOSS_KILLED,
	POINT_PLAYER_KILLED,

	POINT_MONSTER_DAMAGE,
	POINT_STONE_DAMAGE,
	POINT_BOSS_DAMAGE,
	POINT_PLAYER_DAMAGE,

	POINT_OPENED_CHEST,
	POINT_FISHING,
	POINT_MINING,
	POINT_COMPLATE_DUNGEON,
	POINT_UPGRADE_ITEM,
	POINT_USE_ENCHANTED_ITEM,
#endif
};

enum EPKModes
{
	PK_MODE_PEACE,
	PK_MODE_REVENGE,
	PK_MODE_FREE,
	PK_MODE_PROTECT,
	PK_MODE_GUILD,
	PK_MODE_MAX_NUM
};

enum EPositions
{
	POS_DEAD,
	POS_SLEEPING,
	POS_RESTING,
	POS_SITTING,
	POS_FISHING,
	POS_FIGHTING,
	POS_MOUNTING,
	POS_STANDING
};

enum EBlockAction
{
	BLOCK_EXCHANGE = (1 << 0),
	BLOCK_PARTY_INVITE = (1 << 1),
	BLOCK_GUILD_INVITE = (1 << 2),
	BLOCK_WHISPER = (1 << 3),
	BLOCK_MESSENGER_INVITE = (1 << 4),
	BLOCK_PARTY_REQUEST = (1 << 5),
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	BLOCK_WARP_REQUEST		= (1 << 6),
#endif
};


// <Factor> Dynamically evaluated CHARACTER* equivalent.
// Referring to SCharDeadEventInfo.
struct DynamicCharacterPtr {
	DynamicCharacterPtr() : is_pc(false), id(0) {}
	DynamicCharacterPtr(const DynamicCharacterPtr& o)
		: is_pc(o.is_pc), id(o.id) {}

	// Returns the LPCHARACTER found in CHARACTER_MANAGER.
	LPCHARACTER Get() const;
	// Clears the current settings.
	void Reset() {
		is_pc = false;
		id = 0;
	}

	// Basic assignment operator.
	DynamicCharacterPtr& operator=(const DynamicCharacterPtr& rhs) {
		is_pc = rhs.is_pc;
		id = rhs.id;
		return *this;
	}
	// Supports assignment with LPCHARACTER type.
	DynamicCharacterPtr& operator=(LPCHARACTER character);
	// Supports type casting to LPCHARACTER.
	operator LPCHARACTER() const {
		return Get();
	}

	bool is_pc;
	uint32_t id;
};

typedef struct character_point
{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t			points[POINT_MAX_NUM];
#else
	long			points[POINT_MAX_NUM];
#endif

	BYTE			job;
	BYTE			voice;

	BYTE			level;
	DWORD			exp;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t			gold;
#else
	long			gold;
#endif

	HP_LL			hp;
	int				sp;

	HP_LL			iRandomHP;
	int				iRandomSP;

	int				stamina;

	BYTE			skill_group;
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	TPlayerStats	player_stats;
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	int				battle_pass_premium_id;
#endif
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	BYTE chatColor;
#endif
} CHARACTER_POINT;

typedef struct character_point_instant
{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t			points[POINT_MAX_NUM];
#else
	long			points[POINT_MAX_NUM];
#endif

	float			fRot;

	HP_LL			iMaxHP;
	int				iMaxSP;

	long			position;

	long			instant_flag;
	DWORD			dwAIFlag;
	DWORD			dwImmuneFlag;
	DWORD			dwLastShoutPulse;

	DWORD			parts[PART_MAX_NUM];

	LPITEM			pItems[INVENTORY_AND_EQUIP_SLOT_MAX];
	CELL_UINT			bItemGrid[INVENTORY_AND_EQUIP_SLOT_MAX];

	LPITEM			pDSItems[DRAGON_SOUL_INVENTORY_MAX_NUM];
	CELL_UINT			wDSItemGrid[DRAGON_SOUL_INVENTORY_MAX_NUM];
#ifdef ENABLE_SPECIAL_STORAGE
	LPITEM			pSSUItems[SPECIAL_INVENTORY_MAX_NUM];
	CELL_UINT			wSSUItemGrid[SPECIAL_INVENTORY_MAX_NUM];
	LPITEM			pSSBItems[SPECIAL_INVENTORY_MAX_NUM];
	CELL_UINT			wSSBItemGrid[SPECIAL_INVENTORY_MAX_NUM];
	LPITEM			pSSSItems[SPECIAL_INVENTORY_MAX_NUM];
	CELL_UINT			wSSSItemGrid[SPECIAL_INVENTORY_MAX_NUM];
	LPITEM			pSSCItems[SPECIAL_INVENTORY_MAX_NUM];
	CELL_UINT			wSSCItemGrid[SPECIAL_INVENTORY_MAX_NUM];
#endif
#ifdef ENABLE_SWITCHBOT
	LPITEM			pSwitchbotItems[SWITCHBOT_SLOT_COUNT];
#endif
	// by mhh
	LPITEM			pCubeItems[CUBE_MAX_NUM];
	LPCHARACTER		pCubeNpc;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	TItemPosEx				pAcceMaterials[ACCE_WINDOW_MAX_MATERIALS];
#endif
#ifdef ENABLE_AURA_SYSTEM
	LPITEM					pAuraMaterials[AURA_WINDOW_MAX_MATERIALS];
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	LPITEM			pAttrItems;
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	LPITEM			pBuffiItems[BUFFI_MAX_SLOT];
#endif
	LPCHARACTER			battle_victim;

	BYTE			gm_level;

	BYTE			bBasePart;

	int				iMaxStamina;

	BYTE			bBlockMode;

	int				iDragonSoulActiveDeck;
	LPENTITY		m_pDragonSoulRefineWindowOpener;
} CHARACTER_POINT_INSTANT;

#define TRIGGERPARAM		LPCHARACTER ch, LPCHARACTER causer

typedef struct trigger
{
	BYTE	type;
	int		(*func) (TRIGGERPARAM);
	long	value;
} TRIGGER;

class CTrigger
{
public:
	CTrigger() : bType(0), pFunc(NULL)
	{
	}

	BYTE	bType;
	int	(*pFunc) (TRIGGERPARAM);
};

EVENTINFO(char_event_info)
{
	DynamicCharacterPtr ch;
};

typedef std::map<VID, size_t> target_map;
struct TSkillUseInfo
{
	int	    iHitCount;
	int	    iMaxHitCount;
	int	    iSplashCount;
	DWORD   dwNextSkillUsableTime;
	int	    iRange;
	bool    bUsed;
	DWORD   dwVID;
	bool    isGrandMaster;
#ifdef ENABLE_LMW_PROTECTION
	bool	bSkillCD;
	DWORD	dwHitCount;
#endif

	target_map TargetVIDMap;

	TSkillUseInfo()
		: iHitCount(0), iMaxHitCount(0), iSplashCount(0), dwNextSkillUsableTime(0), iRange(0), bUsed(false),
		dwVID(0), isGrandMaster(false)
#ifdef ENABLE_LMW_PROTECTION
		, bSkillCD(false), dwHitCount(0)
#endif
	{}

	bool    HitOnce(DWORD dwVnum = 0);
#ifdef ENABLE_LMW_PROTECTION
	bool    IsSkillCooldown(DWORD dwVnum, float fSkillPower);
#endif

	bool    UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount = 1, int hitcount = -1, int range = -1);
	DWORD   GetMainTargetVID() const { return dwVID; }
	void    SetMainTargetVID(DWORD vid) { dwVID = vid; }
	void    ResetHitCount() { if (iSplashCount) { iHitCount = iMaxHitCount; iSplashCount--; } }
};

enum ActivateTimeCheckList
{
	REFINE_CHECK_TIME,
	EXCHANGE_CHECK_TIME,
	SAFEBOX_CHECK_TIME,
	MYSHOP_CHECK_TIME,
	OPEN_CHEST_CHECK_TIME,
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	FRIEND_TELEPORT_CHECK_TIME,
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	BUFFI_SUMMON_TIME,
#endif
	MAX_ACTIVATE_CHECK
};

typedef struct packet_party_update TPacketGCPartyUpdate;
class CExchange;
class CSkillProto;
class CParty;
class CDungeon;
class CWarMap;
class CAffect;
class CGuild;
class CSafebox;
class CArena;

class CShop;
typedef class CShop* LPSHOP;

class CMob;
class CMobInstance;
typedef struct SMobSkillInfo TMobSkillInfo;

//SKILL_POWER_BY_LEVEL
extern int GetSkillPowerByLevelFromType(int job, int skillgroup, int skilllevel);
//END_SKILL_POWER_BY_LEVEL

namespace marriage
{
	class WeddingMap;
}

#define NEW_ICEDAMAGE_SYSTEM
class CHARACTER : public CEntity, public CFSM, public CHorseRider
{
#ifdef __AUTO_HUNT__
public:
	void			GetAutoHuntCommand(const char* szArgument);
	void			SetAutoHuntStatus(bool bStatus, bool bMobFarm = false, bool bMetinFarm = false);
	bool			IsAutoHuntAffectHas();
protected:
	bool			m_bAutoHuntStatus;
#endif

#ifdef ENABLE_RENEWAL_PVP
public:
	bool				IsInFight();
	void				CheckPvPBonus(bool isAdd, bool* pvpSettingNew);
	bool				CheckPvPUse(DWORD itemVnum);
	bool				IsBlockPvP(DWORD itemVnum);
	bool				CheckPvPSetting(BYTE settingIndex);
protected:
	bool				pvpSettings[PVP_BET];
private: 
	std::vector<DWORD> m_pendingAffects; // Store affects to apply after fight
#endif

protected:
	//////////////////////////////////////////////////////////////////////////////////
	virtual void	EncodeInsertPacket(LPENTITY entity);
	virtual void	EncodeRemovePacket(LPENTITY entity);
	//////////////////////////////////////////////////////////////////////////////////

public:
	LPCHARACTER			FindCharacterInView(const char* name, bool bFindPCOnly);
	void				UpdatePacket();

	//////////////////////////////////////////////////////////////////////////////////
protected:
	CStateTemplate<CHARACTER>	m_stateMove;
	CStateTemplate<CHARACTER>	m_stateBattle;
	CStateTemplate<CHARACTER>	m_stateIdle;

public:
	virtual void		StateMove();
	virtual void		StateBattle();
	virtual void		StateIdle();
	virtual void		StateFlag();
	virtual void		StateFlagBase();
	void				StateHorse();

protected:
	// STATE_IDLE_REFACTORING
	void				__StateIdle_Monster();
	void				__StateIdle_Stone();
	void				__StateIdle_NPC();
	// END_OF_STATE_IDLE_REFACTORING

public:
	DWORD GetAIFlag() const { return m_pointsInstant.dwAIFlag; }

	void				SetAggressive();
	bool				IsAggressive() const;

	void				SetCoward();
	bool				IsCoward() const;
	void				CowardEscape();

	void				SetNoAttackShinsu();
	bool				IsNoAttackShinsu() const;

	void				SetNoAttackChunjo();
	bool				IsNoAttackChunjo() const;

	void				SetNoAttackJinno();
	bool				IsNoAttackJinno() const;

	void				SetAttackMob();
	bool				IsAttackMob() const;

	virtual void			BeginStateEmpty();
	virtual void			EndStateEmpty() {}

	void				RestartAtSamePos();

protected:
	DWORD				m_dwStateDuration;
	//////////////////////////////////////////////////////////////////////////////////

public:
	CHARACTER();
	virtual ~CHARACTER();

	void			Create(const char* c_pszName, DWORD vid, bool isPC);
	void			Destroy();

	void			Disconnect(const char* c_pszReason);

protected:
	void			Initialize();

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
private:
	DWORD			dwLastTargetInfoPulse;

public:
	DWORD			GetLastTargetInfoPulse() const { return dwLastTargetInfoPulse; }
	void			SetLastTargetInfoPulse(DWORD pulse) { dwLastTargetInfoPulse = pulse; }
#endif

#ifdef ENABLE_AFTERDEATH_SHIELD
	public:
		void			StartShieldCountdownEvent(LPCHARACTER ch, int iSec);
		void			SetShieldAffect() {	m_afAffectFlag.Set(AFF_AFTERDEATH_SHIELD);	};
		void			RemoveShieldAffect() {	return m_afAffectFlag.Reset(AFF_AFTERDEATH_SHIELD);	};
#endif

	//////////////////////////////////////////////////////////////////////////////////
	// Basic Points
public:
	DWORD			GetPlayerID() const { return m_dwPlayerID; }

	void			SetPlayerProto(const TPlayerTable* table);
	void			CreatePlayerProto(TPlayerTable& tab);

	void			SetProto(const CMob* c_pkMob);
	WORD			GetRaceNum() const;

	void			Save();		// DelayedSave
	void			SaveReal();
	void			FlushDelayedSaveItem();

	const char* GetName() const;
	const VID& GetVID() const { return m_vid; }

	void			SetName(const std::string& name) { m_stName = name; }

	void			SetRace(BYTE race);
	bool			ChangeSex();

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
	bool			UseExtendedBlendAffect(LPITEM item, int affect_type, int apply_type, int apply_value, int apply_duration);
#endif
#if defined(ENABLE_BLEND_AFFECT)
	bool			SetBlendAffect(LPITEM item);
#endif

	DWORD			GetAID() const;
	int				GetChangeEmpireCount() const;
	void			SetChangeEmpireCount();
	int				ChangeEmpire(BYTE empire);

	BYTE			GetJob() const;
	BYTE			GetCharType() const;

	bool			IsPC() const { return GetDesc() ? true : false; }
	bool			IsNPC()	const { return m_bCharType != CHAR_TYPE_PC; }
	bool			IsMonster()	const { return m_bCharType == CHAR_TYPE_MONSTER; }
	bool			IsStone() const { return m_bCharType == CHAR_TYPE_STONE; }
	bool			IsDoor() const { return m_bCharType == CHAR_TYPE_DOOR; }
	bool			IsBuilding() const { return m_bCharType == CHAR_TYPE_BUILDING; }
	bool			IsWarp() const { return m_bCharType == CHAR_TYPE_WARP; }
	bool			IsGoto() const { return m_bCharType == CHAR_TYPE_GOTO; }
	bool			IsPvM() const { return (m_bCharType == CHAR_TYPE_MONSTER) || (m_bCharType == CHAR_TYPE_STONE) || GetMobRank() >= MOB_RANK_BOSS; }
	//		bool			IsPet() const		{ return m_bCharType == CHAR_TYPE_PET; }

	DWORD			GetLastShoutPulse() const { return m_pointsInstant.dwLastShoutPulse; }
	void			SetLastShoutPulse(DWORD pulse) { m_pointsInstant.dwLastShoutPulse = pulse; }
	int				GetLevel() const { return m_points.level; }
	void			SetLevel(BYTE level);

	BYTE			GetGMLevel() const;
	BOOL 			IsGM() const;
	void			SetGMLevel();

	DWORD			GetExp() const { return m_points.exp; }
	void			SetExp(DWORD exp) { m_points.exp = exp; }
	DWORD			GetNextExp() const;
	LPCHARACTER		DistributeExp();
	void			DistributeHP(LPCHARACTER pkKiller);
	void			DistributeSP(LPCHARACTER pkKiller, int iMethod = 0);

#ifdef ENABLE_KILL_EVENT_FIX
	LPCHARACTER		GetMostAttacked();
#endif

	void			SetPosition(int pos);
	bool			IsPosition(int pos) const { return m_pointsInstant.position == pos ? true : false; }
	int				GetPosition() const { return m_pointsInstant.position; }

	void			SetPart(BYTE bPartPos, DWORD wVal);
	DWORD			GetPart(BYTE bPartPos) const;
	DWORD			GetOriginalPart(BYTE bPartPos) const;

	void			SetHP(HP_LL hp) { m_points.hp = hp; }
	HP_LL			GetHP() const { return m_points.hp; }

	void			SetSP(int sp) { m_points.sp = sp; }
	int				GetSP() const { return m_points.sp; }

	void			SetStamina(int stamina) { m_points.stamina = stamina; }
	int				GetStamina() const { return m_points.stamina; }

	void			SetMaxHP(HP_LL iVal) { m_pointsInstant.iMaxHP = iVal; }
	HP_LL			GetMaxHP() const { return m_pointsInstant.iMaxHP; }

	void			SetMaxSP(int iVal) { m_pointsInstant.iMaxSP = iVal; }
	int				GetMaxSP() const { return m_pointsInstant.iMaxSP; }

	void			SetMaxStamina(int iVal) { m_pointsInstant.iMaxStamina = iVal; }
	int				GetMaxStamina() const { return m_pointsInstant.iMaxStamina; }

	void			SetRandomHP(HP_LL v) { m_points.iRandomHP = v; }
	void			SetRandomSP(int v) { m_points.iRandomSP = v; }

	HP_LL			GetRandomHP() const { return m_points.iRandomHP; }
	int				GetRandomSP() const { return m_points.iRandomSP; }

	HP_LL			GetHPPct() const;

	void			SetRealPoint(BYTE idx, HP_LL val);
	HP_LL			GetRealPoint(BYTE idx) const;

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	void			SetPoint(BYTE idx, int64_t val);
	int64_t			GetPoint(BYTE idx) const;
#else
	void			SetPoint(BYTE idx, int val);
	int				GetPoint(BYTE idx) const;
#endif
	HP_LL			GetLimitPoint(BYTE idx) const;
	int				GetPolymorphPoint(BYTE idx) const;

	const TMobTable& GetMobTable() const;
	BYTE				GetMobRank() const;
	BYTE				GetMobBattleType() const;
	BYTE				GetMobSize() const;
	DWORD				GetMobDamageMin() const;
	DWORD				GetMobDamageMax() const;
	WORD				GetMobAttackRange() const;
	DWORD				GetMobDropItemVnum() const;
	float				GetMobDamageMultiply() const;

	// NEWAI
	bool			IsBerserker() const;
	bool			IsBerserk() const;
	void			SetBerserk(bool mode);

	bool			IsStoneSkinner() const;

	bool			IsGodSpeeder() const;
	bool			IsGodSpeed() const;
	void			SetGodSpeed(bool mode);

	bool			IsDeathBlower() const;
	bool			IsDeathBlow() const;

	bool			IsReviver() const;
	bool			HasReviverInParty() const;
	bool			IsRevive() const;
	void			SetRevive(bool mode);
	// NEWAI END

	bool			IsRaceFlag(DWORD dwBit) const;
	bool			IsSummonMonster() const;
	DWORD			GetSummonVnum() const;

	DWORD			GetPolymorphItemVnum() const;
	DWORD			GetMonsterDrainSPPoint() const;

	void			MainCharacterPacket();

	void			ComputePoints();
	void			ComputeBattlePoints();
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	void			PointChange(BYTE type, int64_t amount, bool bAmount = false, bool bBroadcast = false);
#else
	void			PointChange(BYTE type, int amount, bool bAmount = false, bool bBroadcast = false);
#endif
	void			PointsPacket();
	void			ApplyPoint(BYTE bApplyType, HP_LL iVal);
	void			CheckMaximumPoints();

	bool			Show(long lMapIndex, long x, long y, long z = LONG_MAX, bool bShowSpawnMotion = false);

	void			Sitdown(int is_ground);
	void			Standup();

	void			SetRotation(float fRot);
	void			SetRotationToXY(long x, long y);
	float			GetRotation() const { return m_pointsInstant.fRot; }

	void			MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion* packet);
	void			Motion(BYTE motion, LPCHARACTER victim = NULL);

	void			ChatPacket(BYTE type, const char* format, ...);
#ifdef ENABLE_NEW_CHAT
	void			NewChatPacket(WORD chatID, const char* format, ...);
	void			NewChatPacket(WORD chatID);
	void			QuestChatPacket(WORD chatID, BYTE type);
	void			QuestChatPacket(WORD chatID, BYTE type, const char* format, ...);
#endif	
	void			MonsterChat(BYTE bMonsterChatType);

	void			ResetPoint(int iLv);

	void			SetBlockMode(BYTE bFlag);
	void			SetBlockModeForce(BYTE bFlag);
	bool			IsBlockMode(BYTE bFlag) const { return (m_pointsInstant.bBlockMode & bFlag) ? true : false; }

	bool			IsPolymorphed() const { return m_dwPolymorphRace > 0; }
	bool			IsPolyMaintainStat() const { return m_bPolyMaintainStat; }
	void			SetPolymorph(DWORD dwRaceNum, bool bMaintainStat = false);
	DWORD			GetPolymorphVnum() const { return m_dwPolymorphRace; }
	int				GetPolymorphPower() const;
	bool			IsNearWater() const;// @fixme219

#ifdef ENABLE_INVENTORY_REWORK
	void			ItemWinnerChat(DWORD count, BYTE window_type, const char* itemname);
	BYTE			VnumGetWindowType(DWORD vnum) const;
	int				WindowTypeToGetEmpty(BYTE window_type, LPITEM item);
	LPITEM			WindowTypeGetItem(WORD wCell, BYTE window_type) const;
#endif

	// FISING
	void			fishing();
	void			fishing_take();
	// END_OF_FISHING

	// MINING
	void			mining(LPCHARACTER chLoad);
	void			mining_cancel();
	void			mining_take();
	// END_OF_MINING

	void			ResetPlayTime(DWORD dwTimeRemain = 0);

	void			CreateFly(BYTE bType, LPCHARACTER pkVictim);

	void			ResetChatCounter();
	BYTE			IncreaseChatCounter();
	BYTE			GetChatCounter() const;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	void			ResetMountCounter();
	BYTE			IncreaseMountCounter();
	BYTE			GetMountCounter() const;
#endif

protected:
	DWORD			m_dwPolymorphRace;
	bool			m_bPolyMaintainStat;
	DWORD			m_dwLoginPlayTime;
	DWORD			m_dwPlayerID;
	VID				m_vid;
	std::string		m_stName;
	BYTE			m_bCharType;

	CHARACTER_POINT		m_points;
	CHARACTER_POINT_INSTANT	m_pointsInstant;

	int				m_iMoveCount;
	DWORD			m_dwPlayStartTime;
	BYTE			m_bAddChrState;
	bool			m_bSkipSave;
	BYTE			m_bChatCounter;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	BYTE			m_bMountCounter;
#endif

	// End of Basic Points

	//////////////////////////////////////////////////////////////////////////////////
	// Move & Synchronize Positions
	//////////////////////////////////////////////////////////////////////////////////
public:
	bool			IsStateMove() const { return IsState((CState&)m_stateMove); }
	bool			IsStateIdle() const { return IsState((CState&)m_stateIdle); }
	bool			IsWalking() const { return m_bNowWalking || GetStamina() <= 0; }
	void			SetWalking(bool bWalkFlag) { m_bWalking = bWalkFlag; }
	void			SetNowWalking(bool bWalkFlag);
	void			ResetWalking() { SetNowWalking(m_bWalking); }

	bool			Goto(long x, long y);
	void			Stop();

	bool			CanMove() const;

	void			SyncPacket();
	bool			Sync(long x, long y);
	bool			Move(long x, long y);
	void			OnMove(bool bIsAttack = false);
	DWORD			GetMotionMode() const;
	float			GetMoveMotionSpeed() const;
	float			GetMoveSpeed() const;
	void			CalculateMoveDuration();
	void			SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime = 0, int iRot = -1);
	DWORD			GetCurrentMoveDuration() const { return m_dwMoveDuration; }
	DWORD			GetWalkStartTime() const { return m_dwWalkStartTime; }
	DWORD			GetLastMoveTime() const { return m_dwLastMoveTime; }
	DWORD			GetLastAttackTime() const { return m_dwLastAttackTime; }

	void			SetLastAttacked(DWORD time);

	bool			SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList = true);
	bool			IsSyncOwner(LPCHARACTER ch) const;

	bool			WarpSet(long x, long y, long lRealMapIndex = 0);
#ifdef ENABLE_P2P_WARP
	bool			WarpSet(long x, long y, long lPrivateMapIndex, long lMapIndex, long lAddr, WORD wPort);
#endif
	void			SetWarpLocation(long lMapIndex, long x, long y);
	void			WarpEnd();
	const PIXEL_POSITION& GetWarpPosition() const { return m_posWarp; }
	bool			WarpToPID(DWORD dwPID);

	void			SaveExitLocation();
	void			ExitToSavedLocation();

	void			StartStaminaConsume();
	void			StopStaminaConsume();
	bool			IsStaminaConsume() const;
	bool			IsStaminaHalfConsume() const;

	void			ResetStopTime();
	DWORD			GetStopTime() const;
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	bool			WarpToPlayer(LPCHARACTER victim);
	bool			WarpToPlayer(LPDESC desc, DWORD pid, BYTE channel, char* targetName);
	bool			WarpToPlayerMapLevelControl(long mapIndex);
#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
public:
	bool			ChangeLanguage(BYTE bLanguage);
	void			SetLanguage(BYTE language) { m_language = language; }
	BYTE			GetLanguage() { return m_language; }
private:
	BYTE			m_language;
#endif

protected:
	void			ClearSync();

	float			m_fSyncTime;
	LPCHARACTER		m_pkChrSyncOwner;
	CHARACTER_LIST	m_kLst_pkChrSyncOwned;

	PIXEL_POSITION	m_posDest;
	PIXEL_POSITION	m_posStart;
	PIXEL_POSITION	m_posWarp;
	long			m_lWarpMapIndex;

	PIXEL_POSITION	m_posExit;
	long			m_lExitMapIndex;

	DWORD			m_dwMoveStartTime;
	DWORD			m_dwMoveDuration;

	DWORD			m_dwLastMoveTime;
	DWORD			m_dwLastAttackTime;
	DWORD			m_dwWalkStartTime;
	DWORD			m_dwStopTime;

	bool			m_bWalking;
	bool			m_bNowWalking;
	bool			m_bStaminaConsume;
	// End

public:
	void			SyncQuickslot(BYTE bType, QS_USHORT bOldPos, QS_USHORT bNewPos);
	bool			GetQuickslot(QS_USHORT pos, TQuickslot** ppSlot);
	bool			SetQuickslot(QS_USHORT pos, TQuickslot& rSlot);
	bool			DelQuickslot(QS_USHORT pos);
	bool			SwapQuickslot(QS_USHORT a, QS_USHORT b);
	void			ChainQuickslotItem(LPITEM pItem, BYTE bType, QS_USHORT bOldPos);

protected:
	TQuickslot		m_quickslot[QUICKSLOT_MAX_NUM];

	////////////////////////////////////////////////////////////////////////////////////////
	// Affect
public:
	void			StartAffectEvent();
	void			ClearAffect(bool bSave = false
#ifdef ENABLE_NO_CLEAR_BUFF_WHEN_MONSTER_KILL
		, bool letBuffs = false
#endif
	);
	void			ComputeAffect(CAffect* pkAff, bool bAdd);
	bool			AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false);
	void			RefreshAffect();
	bool			RemoveAffect(DWORD dwType);
	bool			IsAffectFlag(DWORD dwAff) const;

	bool			UpdateAffect();	// called from EVENT
	int				ProcessAffect();

	void			LoadAffect(DWORD dwCount, TPacketAffectElement* pElements);
	void			SaveAffect();

	bool			IsLoadedAffect() const { return m_bIsLoadedAffect; }

	bool			IsGoodAffect(BYTE bAffectType) const;

	void			RemoveGoodAffect();
	void			RemoveBadAffect();

	CAffect* FindAffect(DWORD dwType, BYTE bApply = APPLY_NONE) const;
	const std::list<CAffect*>& GetAffectContainer() const { return m_list_pkAffect; }
	bool			RemoveAffect(CAffect* pkAff);

protected:
	bool			m_bIsLoadedAffect;
	TAffectFlag		m_afAffectFlag;
	std::list<CAffect*>	m_list_pkAffect;

public:
	// PARTY_JOIN_BUG_FIX
	void			SetParty(LPPARTY pkParty);
	LPPARTY			GetParty() const { return m_pkParty; }

	bool			RequestToParty(LPCHARACTER leader);
	void			DenyToParty(LPCHARACTER member);
	void			AcceptToParty(LPCHARACTER member);

	void			PartyInvite(LPCHARACTER pchInvitee);

	void			PartyInviteAccept(LPCHARACTER pchInvitee);

	void			PartyInviteDeny(DWORD dwPID);

	bool			BuildUpdatePartyPacket(TPacketGCPartyUpdate& out);
	int				GetLeadershipSkillLevel() const;

	bool			CanSummon(int iLeaderShip);

	void			SetPartyRequestEvent(LPEVENT pkEvent) { m_pkPartyRequestEvent = pkEvent; }

protected:

	void			PartyJoin(LPCHARACTER pkLeader);

	enum PartyJoinErrCode {
		PERR_NONE = 0,
		PERR_SERVER,
		PERR_DUNGEON,
		PERR_OBSERVER,
		PERR_LVBOUNDARY,
		PERR_LOWLEVEL,
		PERR_HILEVEL,
		PERR_ALREADYJOIN,
		PERR_PARTYISFULL,
		PERR_SEPARATOR,			///< Error type separator.
		PERR_DIFFEMPIRE,
		PERR_MAX
	};

	static PartyJoinErrCode	IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

	static PartyJoinErrCode	IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

	LPPARTY			m_pkParty;
	DWORD			m_dwLastDeadTime;
	LPEVENT			m_pkPartyRequestEvent;

	typedef std::map< DWORD, LPEVENT >	EventMap;
	EventMap		m_PartyInviteEventMap;

	// END_OF_PARTY_JOIN_BUG_FIX

	////////////////////////////////////////////////////////////////////////////////////////
	// Dungeon
public:
	void			SetDungeon(LPDUNGEON pkDungeon);
	LPDUNGEON		GetDungeon() const { return m_pkDungeon; }
	LPDUNGEON		GetDungeonForce() const;
protected:
	LPDUNGEON	m_pkDungeon;
	int			m_iEventAttr;

	////////////////////////////////////////////////////////////////////////////////////////
	// Guild
public:
	void			SetGuild(CGuild* pGuild);
	CGuild* GetGuild() const { return m_pGuild; }

	void			SetWarMap(CWarMap* pWarMap);
	CWarMap* GetWarMap() const { return m_pWarMap; }

protected:
	CGuild* m_pGuild;
	DWORD			m_dwUnderGuildWarInfoMessageTime;
	CWarMap* m_pWarMap;

	////////////////////////////////////////////////////////////////////////////////////////
	// Item related
public:
	bool			CanHandleItem(bool bSkipRefineCheck = false, bool bSkipObserver = false);

	bool			IsItemLoaded() const { return m_bItemLoaded; }
	void			SetItemLoaded() { m_bItemLoaded = true; }

	void			ClearItem();
#ifdef ENABLE_HIGHLIGHT_SYSTEM
	void			SetItem(TItemPos Cell, LPITEM item, bool isHighLight = false);
#else
	void			SetItem(TItemPos Cell, LPITEM item);
#endif
	LPITEM			GetItem(TItemPos Cell) const;
	LPITEM			GetInventoryItem(WORD wCell) const;
#ifdef ENABLE_SPECIAL_STORAGE
	LPITEM			GetUpgradeInventoryItem(WORD wCell) const;
	LPITEM			GetBookInventoryItem(WORD wCell) const;
	LPITEM			GetStoneInventoryItem(WORD wCell) const;
	LPITEM			GetChestInventoryItem(WORD wCell) const;
#endif

	bool			IsEmptyItemGrid(TItemPos Cell, BYTE size, int iExceptionCell = -1) const;

	void			SetWear(CELL_UINT bCell, LPITEM item);
	LPITEM			GetWear(CELL_UINT bCell) const;

	// MYSHOP_PRICE_LIST
	void			UseSilkBotary(void);

	void			UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p);
	// END_OF_MYSHOP_PRICE_LIST

	bool			UseItemEx(LPITEM item, TItemPos DestCell
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		, MAX_COUNT item_open_count = 1
#endif
	);
	bool			UseItem(TItemPos Cell, TItemPos DestCell = NPOS
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		, MAX_COUNT item_open_count = 1
#endif
	);

	// ADD_REFINE_BUILDING
	bool			IsRefineThroughGuild() const;
	CGuild* GetRefineGuild() const;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t			ComputeRefineFee(int64_t iCost, int iMultiply = 5) const;
#else
	int				ComputeRefineFee(int iCost, int iMultiply = 5) const;
#endif
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	void			PayRefineFee(int64_t iTotalMoney);
#else
	void			PayRefineFee(int iTotalMoney);
#endif
	void			SetRefineNPC(LPCHARACTER ch);
	// END_OF_ADD_REFINE_BUILDING

	bool			RefineItem(LPITEM pkItem, LPITEM pkTarget);
#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	bool			DeleteItem(TItemPos Cell);
	bool			SellItem(TItemPos Cell);
#else
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	bool			DropItem(TItemPos Cell, MAX_COUNT bCount = 0);
#else
	bool			DropItem(TItemPos Cell, BYTE bCount = 0);
#endif
#endif

	bool			GiveRecallItem(LPITEM item);
	void			ProcessRecallItem(LPITEM item);

	//	void			PotionPacket(int iPotionType);
	void			EffectPacket(int enumEffectType);
	void			SpecificEffectPacket(const char filename[128]);
	void			SpecificEffectPacket(BYTE type);

	// ADD_MONSTER_REFINE
	bool			DoRefine(LPITEM item, bool bMoneyOnly = false);
	// END_OF_ADD_MONSTER_REFINE

	bool			DoRefineWithScroll(LPITEM item);
	bool			RefineInformation(CELL_UINT bCell, BYTE bType, int iAdditionalCell = -1);

	void			SetRefineMode(int iAdditionalCell = -1);
	void			ClearRefineMode();

	bool			GiveItem(LPCHARACTER victim, TItemPos Cell);
	bool			CanReceiveItem(LPCHARACTER from, LPITEM item) const;
	void			ReceiveItem(LPCHARACTER from, LPITEM item);
	bool			GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector <DWORD>& dwItemVnums,
		std::vector <DWORD>& dwItemCounts, std::vector <LPITEM>& item_gets, int& count);

	bool			MoveItem(TItemPos pos, TItemPos change_pos,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT num
#else
		BYTE num
#endif
#ifdef ENABLE_SPLIT_ITEMS_FAST
		, bool isSplitItems = false
#endif
	);
	bool			PickupItem(DWORD vid);
	bool			EquipItem(LPITEM item, int iCandidateCell = -1);
	bool			UnequipItem(LPITEM item);

	bool			CanEquipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

	bool			CanUnequipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

	bool			SwapItem(CELL_UINT bCell, CELL_UINT bDestCell);

	LPITEM			AutoGiveItem(DWORD dwItemVnum,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT bCount = 1,
#else
		BYTE bCount = 1,
#endif
		int iRarePct = -1, bool bMsg = true);

	void			AutoGiveItem(LPITEM item, bool longOwnerShip = false);

	int				GetEmptyInventory(BYTE size) const;
	int				GetEmptyDragonSoulInventory(LPITEM pItem) const;
#ifdef ENABLE_SPECIAL_STORAGE
	int				GetEmptyUpgradeInventory(LPITEM pItem) const;
	int				GetEmptyBookInventory(LPITEM pItem) const;
	int				GetEmptyStoneInventory(LPITEM pItem) const;
	int				GetEmptyChestInventory(LPITEM pItem) const;
#endif
#ifdef ENABLE_SPLIT_ITEMS_FAST
	int				GetEmptyInventoryFromIndex(WORD index, BYTE itemSize, BYTE windowtype = INVENTORY) const; //SPLIT ITEMS
#endif
	void			CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const;

	int				CountEmptyInventory() const;

	int				CountSpecifyItem(DWORD vnum) const;
	bool			CountSpecifyItemText(DWORD vnum, int reqCount);
	void			RemoveSpecifyItem(DWORD vnum, DWORD count = 1);
	LPITEM			FindSpecifyItem(DWORD vnum) const;
	LPITEM			FindItemByID(DWORD id) const;

	int				CountSpecifyTypeItem(BYTE type) const;
	void			RemoveSpecifyTypeItem(BYTE type, DWORD count = 1);

	bool			IsEquipUniqueItem(DWORD dwItemVnum) const;

	// CHECK_UNIQUE_GROUP
	bool			IsEquipUniqueGroup(DWORD dwGroupVnum) const;
	// END_OF_CHECK_UNIQUE_GROUP

	void			SendEquipment(LPCHARACTER ch);
	// End of Item

#ifdef ENABLE_OFFLINE_SHOP
public:
	bool CanTakeInventoryItem(LPITEM item, TItemPos* cell);
	offlineshop::CShop* GetOfflineShop() { return m_pkOfflineShop; }
	void						SetOfflineShop(offlineshop::CShop* pkShop) { m_pkOfflineShop = pkShop; }

	offlineshop::CShop* GetOfflineShopGuest() const { return m_pkOfflineShopGuest; }
	void						SetOfflineShopGuest(offlineshop::CShop* pkShop) { m_pkOfflineShopGuest = pkShop; }

	offlineshop::CShopSafebox*	GetShopSafebox() { return m_pkShopSafebox; }
	void						SetShopSafebox(offlineshop::CShopSafebox* pk);

	int GetOfflineShopUseTime() const { return m_iOfflineShopUseTime; }
	void SetOfflineShopUseTime() { m_iOfflineShopUseTime = thecore_pulse(); }

private:
	offlineshop::CShop* m_pkOfflineShop;
	offlineshop::CShop* m_pkOfflineShopGuest;
	offlineshop::CShopSafebox* m_pkShopSafebox;
	int m_iOfflineShopUseTime = 0;
#endif

protected:

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	void			SendMyShopPriceListCmd(DWORD dwItemVnum, int64_t dwItemPrice);
#else
	void			SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice);
#endif

	bool			m_bNoOpenedShop;

	bool			m_bItemLoaded;
	int				m_iRefineAdditionalCell;
	bool			m_bUnderRefine;
	DWORD			m_dwRefineNPCVID;

public:
	////////////////////////////////////////////////////////////////////////////////////////
	// Money related
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t			GetGold() const { return m_points.gold; }
	void			SetGold(int64_t gold) { m_points.gold = gold; }
	bool			DropGold(INT gold);
	int64_t			GetAllowedGold() const;
	void			GiveGold(int64_t iAmount);
#else
	INT				GetGold() const { return m_points.gold; }
	void			SetGold(INT gold) { m_points.gold = gold; }
	bool			DropGold(INT gold);
	INT				GetAllowedGold() const;
	void			GiveGold(INT iAmount);
#endif
	// End of Money

	////////////////////////////////////////////////////////////////////////////////////////
	// Shop related
public:
	void			SetShop(LPSHOP pkShop);
	LPSHOP			GetShop() const { return m_pkShop; }
	void			ShopPacket(BYTE bSubHeader);

	void			SetShopOwner(LPCHARACTER ch) { m_pkChrShopOwner = ch; }
	LPCHARACTER		GetShopOwner() const { return m_pkChrShopOwner; }

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	void			OpenMyShop(const char* c_pszSign, TShopItemTable* pTable, MAX_COUNT bItemCount);
#else
	void			OpenMyShop(const char* c_pszSign, TShopItemTable* pTable, BYTE bItemCount);
#endif
	LPSHOP			GetMyShop() const { return m_pkMyShop; }
	void			CloseMyShop();

protected:

	LPSHOP			m_pkShop;
	LPSHOP			m_pkMyShop;
	std::string		m_stShopSign;
	LPCHARACTER		m_pkChrShopOwner;
	// End of shop

	////////////////////////////////////////////////////////////////////////////////////////
	// Exchange related
public:
	bool			ExchangeStart(LPCHARACTER victim);
	void			SetExchange(CExchange* pkExchange);
	CExchange* GetExchange() const { return m_pkExchange; }

protected:
	CExchange* m_pkExchange;
	// End of Exchange

	////////////////////////////////////////////////////////////////////////////////////////
	// Battle
public:
	struct TBattleInfo
	{
		DAM_LL iTotalDamage;
		DAM_LL iAggro;

		TBattleInfo(DAM_LL iTot, DAM_LL iAggr)
			: iTotalDamage(iTot), iAggro(iAggr)
		{}
	};
	typedef std::map<VID, TBattleInfo>	TDamageMap;

	typedef struct SAttackLog
	{
		DWORD	dwVID;
		DWORD	dwTime;
	} AttackLog;

	bool				Damage(LPCHARACTER pAttacker, DAM_LL dam, EDamageType type = DAMAGE_TYPE_NORMAL);
	void				DeathPenalty(BYTE bExpLossPercent);
	void				ReviveInvisible(int iDur);

	bool				Attack(LPCHARACTER pkVictim, BYTE bType = 0);
	bool				IsAlive() const { return m_pointsInstant.position == POS_DEAD ? false : true; }
	bool				CanFight() const;

	bool				CanBeginFight() const;
	void				BeginFight(LPCHARACTER pkVictim);

	bool				CounterAttack(LPCHARACTER pkChr);

	bool				IsStun() const;
	void				Stun();
	bool				IsDead() const;
	void				Dead(LPCHARACTER pkKiller = NULL, bool bImmediateDead = true); // @fixme188 instant death false to true

	void				Reward(bool bItemDrop);
	void				RewardGold(LPCHARACTER pkAttacker);

	bool				Shoot(BYTE bType);
	void				FlyTarget(DWORD dwTargetVID, long x, long y, BYTE bHeader);

	void				ForgetMyAttacker();
	void				AggregateMonster();
	void				AttractRanger();
	void				PullMonster();

	bool				GetArrowAndBow(LPITEM* ppkBow, LPITEM* ppkArrow);

	void				AttackedByPoison(LPCHARACTER pkAttacker);
	void				RemovePoison();
#ifdef ENABLE_WOLFMAN_CHARACTER
	void				AttackedByBleeding(LPCHARACTER pkAttacker);
	void				RemoveBleeding();
#endif
	void				AttackedByFire(LPCHARACTER pkAttacker, DAM_LL amount, int count);
	void				RemoveFire();

#ifdef ENABLE_ALIGNMENT_SYSTEM
#ifndef DISABLE_ALIGNMENT_SYSTEM
	void				AlignmentLevel(int Alignment, bool update = false);
	void				AlignmentBonus();
	void				AlignmentBonusDelete(BYTE oldlevel);
#endif
#endif

	void				UpdateAlignment(int iAmount);
	int					GetAlignment() const;

	int					GetRealAlignment() const;
	void				ShowAlignment(bool bShow);

	void				SetKillerMode(bool bOn);
	bool				IsKillerMode() const;
	void				UpdateKillerMode();

	BYTE				GetPKMode() const;
	void				SetPKMode(BYTE bPKMode);

	void				UpdateAggrPoint(LPCHARACTER ch, EDamageType type, DAM_LL dam);
	
#ifdef ENABLE_AFK_MODE_SYSTEM
	public:
		void SetAway(bool f) { m_isAway = f; }
		bool IsAway() const { return m_isAway; }
		void StartUpdateCharacterEvent();
#endif

protected:
	void				UpdateAggrPointEx(LPCHARACTER ch, EDamageType type, DAM_LL dam, TBattleInfo& info);
	void				ChangeVictimByAggro(DAM_LL iNewAggro, LPCHARACTER pNewVictim);

	DWORD				m_dwFlyTargetID;
	std::vector<DWORD>	m_vec_dwFlyTargets;
	TDamageMap			m_map_kDamage;
	//		AttackLog			m_kAttackLog;
	DWORD				m_dwKillerPID;
#ifdef ENABLE_ALIGNMENT_SYSTEM
#ifndef DISABLE_ALIGNMENT_SYSTEM
	BYTE				m_AlignmentLevel;
	std::vector<std::pair<BYTE, long long>> m_alignment_bonus;
#endif
#endif
	int					m_iAlignment;		// Lawful/Chaotic value -200000 ~ 200000
	int					m_iRealAlignment;
	int					m_iKillerModePulse;
	BYTE				m_bPKMode;

	// Aggro
	DWORD				m_dwLastVictimSetTime;
	DAM_LL					m_iMaxAggro;
	// End of Battle

	// Stone
public:
	void				SetStone(LPCHARACTER pkChrStone);
	void				ClearStone();
	void				DetermineDropMetinStone();
	DWORD				GetDropMetinStoneVnum() const { return m_dwDropMetinStone; }
	BYTE				GetDropMetinStonePct() const { return m_bDropMetinStonePct; }

protected:
	LPCHARACTER			m_pkChrStone;
	CHARACTER_SET		m_set_pkChrSpawnedBy;
	DWORD				m_dwDropMetinStone;
	BYTE				m_bDropMetinStonePct;
	// End of Stone

public:
	enum
	{
		SKILL_UP_BY_POINT,
		SKILL_UP_BY_BOOK,
		SKILL_UP_BY_TRAIN,

		// ADD_GRANDMASTER_SKILL
		SKILL_UP_BY_QUEST,
		// END_OF_ADD_GRANDMASTER_SKILL
	};

	void				SkillLevelPacket();
	void				SkillLevelUp(DWORD dwVnum, BYTE bMethod = SKILL_UP_BY_POINT);
	bool				SkillLevelDown(DWORD dwVnum);
	// ADD_GRANDMASTER_SKILL
	bool				UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster = true);
	void				ResetSkill();
#ifdef ENABLE_LMW_PROTECTION
	bool				IsSkillCooldown(DWORD dwVnum, float fSkillPower) { return m_SkillUseInfo[dwVnum].IsSkillCooldown(dwVnum, fSkillPower) ? true : false; }
#endif
	void				SetSkillLevel(DWORD dwVnum, BYTE bLev);
	int					GetUsedSkillMasterType(DWORD dwVnum);

	bool				IsLearnableSkill(DWORD dwSkillVnum) const;
	// END_OF_ADD_GRANDMASTER_SKILL

	bool				CheckSkillHitCount(const BYTE SkillID, const VID dwTargetVID);
	bool				CanUseSkill(DWORD dwSkillVnum) const;
	bool				IsUsableSkillMotion(DWORD dwMotionIndex) const;
	int					GetSkillLevel(DWORD dwVnum) const;
	int					GetSkillMasterType(DWORD dwVnum) const;
	int					GetSkillPower(DWORD dwVnum, BYTE bLevel = 0) const;

	void				SkillLearnWaitMoreTimeMessage(DWORD dwVnum);

	void				ComputePassiveSkill(DWORD dwVnum);
	int					ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
#ifdef ENABLE_WOLFMAN_CHARACTER
	int					ComputeSkillParty(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
	void				WolfmanSkillSet();
#endif
	int					ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel = 0);

	void				SetSkillGroup(BYTE bSkillGroup);
	BYTE				GetSkillGroup() const { return m_points.skill_group; }

	int					ComputeCooltime(int time);

	void				GiveRandomSkillBook();

	void				DisableCooltime();
	bool				LearnSkillByBook(DWORD dwSkillVnum, BYTE bProb = 0);
	bool				LearnGrandMasterSkill(DWORD dwSkillVnum);
#ifdef ENABLE_SKILLS_LEVEL_OVER_P
	bool				LearnSageMasterSkill(DWORD dwSkillVnum);
	bool				LearnLegendaryMasterSkill(DWORD dwSkillVnum);
	bool				LearnPerfectMasterSkill(DWORD dwSkillVnum);
#endif
#ifdef ENABLE_PASSIVE_SKILLS
	void				ComputeNewPassiveSkills();
	void				PassiveSkillUpgrade(BYTE skillIdx, bool allgive);
	DWORD				GetPassiveReadBookVnum(BYTE skillIdx);

	long				GetPassiveCooldDown(BYTE skillIdx);
#endif
#ifdef ENABLE_SKILL_SET_BONUS
	void	SkillSetBonusCalcute(bool update);
	void	SkillSetBonusGiveBuff();
	int		GetSkillSetBonus() const { return m_SkillSetBonus; }
	void	SetSkillSetBonus(int value) { m_SkillSetBonus = value; }
protected:
	int		m_SkillSetBonus;
#endif
private:
	bool				m_bDisableCooltime;
	DWORD				m_dwLastSkillTime;
	// End of Skill

	// MOB_SKILL
public:
	bool				HasMobSkill() const;
	size_t				CountMobSkill() const;
	const TMobSkillInfo* GetMobSkill(unsigned int idx) const;
	bool				CanUseMobSkill(unsigned int idx) const;
	bool				UseMobSkill(unsigned int idx);
	void				ResetMobSkillCooltime();
protected:
	DWORD				m_adwMobSkillCooltime[MOB_SKILL_MAX_NUM];
	// END_OF_MOB_SKILL

	// for SKILL_MUYEONG
public:
	void				StartMuyeongEvent();
	void				StopMuyeongEvent();

private:
	LPEVENT				m_pkMuyeongEvent;

	// for SKILL_CHAIN lighting
public:
	int					GetChainLightningIndex() const { return m_iChainLightingIndex; }
	void				IncChainLightningIndex() { ++m_iChainLightingIndex; }
	void				AddChainLightningExcept(LPCHARACTER ch) { m_setExceptChainLighting.insert(ch); }
	void				ResetChainLightningIndex() { m_iChainLightingIndex = 0; m_setExceptChainLighting.clear(); }
	int					GetChainLightningMaxCount() const;
	const CHARACTER_SET& GetChainLightingExcept() const { return m_setExceptChainLighting; }

private:
	int					m_iChainLightingIndex;
	CHARACTER_SET m_setExceptChainLighting;

	// for SKILL_EUNHYUNG
public:
	void				SetAffectedEunhyung();
	void				ClearAffectedEunhyung() { m_dwAffectedEunhyungLevel = 0; }
	bool				GetAffectedEunhyung() const { return m_dwAffectedEunhyungLevel; }

private:
	DWORD				m_dwAffectedEunhyungLevel;

	//
	// Skill levels
	//
protected:
	TPlayerSkill* m_pSkillLevels;
	boost::unordered_map<BYTE, int>		m_SkillDamageBonus;
	std::map<int, TSkillUseInfo>	m_SkillUseInfo;

	////////////////////////////////////////////////////////////////////////////////////////
	// AI related
public:
	void			AssignTriggers(const TMobTable* table);
	LPCHARACTER		GetVictim() const;
	void			SetVictim(LPCHARACTER pkVictim);
	LPCHARACTER		GetNearestVictim(LPCHARACTER pkChr);
	LPCHARACTER		GetProtege() const;

	bool			Follow(LPCHARACTER pkChr, float fMinimumDistance = 150.0f);
	bool			Return();
	bool			IsGuardNPC() const;
	bool			IsChangeAttackPosition(LPCHARACTER target) const;
	void			ResetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time() - AI_CHANGE_ATTACK_POISITION_TIME_NEAR; }
	void			SetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time(); }

	bool			OnIdle();

	void			OnAttack(LPCHARACTER pkChrAttacker);
	void			OnClick(LPCHARACTER pkChrCauser);

	VID				m_kVIDVictim;

protected:
	DWORD			m_dwLastChangeAttackPositionTime;
	CTrigger		m_triggerOnClick;
	// End of AI

	////////////////////////////////////////////////////////////////////////////////////////
	// Target
protected:
	LPCHARACTER				m_pkChrTarget;
	CHARACTER_SET	m_set_pkChrTargetedBy;

public:
	void				SetTarget(LPCHARACTER pkChrTarget);
	void				BroadcastTargetPacket();
	void				ClearTarget();
	void				CheckTarget();
	LPCHARACTER			GetTarget() const { return m_pkChrTarget; }

	////////////////////////////////////////////////////////////////////////////////////////
	// Safebox
public:
	int					GetSafeboxSize() const;
	void				QuerySafeboxSize();
	void				SetSafeboxSize(int size);

	CSafebox* GetSafebox() const;
	void				LoadSafebox(int iSize, DWORD dwGold, int iItemCount, TPlayerItem* pItems);
	void				ChangeSafeboxSize(BYTE bSize);
	void				CloseSafebox();

	void				ReqSafeboxLoad(const char* pszPassword);

	void				CancelSafeboxLoad(void) { m_bOpeningSafebox = false; }

	void				SetMallLoadTime(int t) { m_iMallLoadTime = t; }
	int					GetMallLoadTime() const { return m_iMallLoadTime; }

	CSafebox* GetMall() const;
	void				LoadMall(int iItemCount, TPlayerItem* pItems);
	void				CloseMall();

	void				SetSafeboxOpenPosition();
	float				GetDistanceFromSafeboxOpen() const;

protected:
	CSafebox* m_pkSafebox;
	int					m_iSafeboxSize;
	bool				m_bOpeningSafebox;

	CSafebox* m_pkMall;
	int					m_iMallLoadTime;

	PIXEL_POSITION		m_posSafeboxOpen;

	////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////
	// Mounting
public:
	void				MountVnum(DWORD vnum);
	DWORD				GetMountVnum() const { return m_dwMountVnum; }
	DWORD				GetLastMountTime() const { return m_dwMountTime; }

	bool				CanUseHorseSkill();

	// Horse
	virtual	void		SetHorseLevel(int iLevel);

	virtual	bool		StartRiding();
	virtual	bool		StopRiding();

	virtual	DWORD		GetMyHorseVnum() const;

	virtual	void		HorseDie();
	virtual bool		ReviveHorse();

	virtual void		SendHorseInfo();
	virtual	void		ClearHorseInfo();

	void				HorseSummon(bool bSummon, bool bFromFar = false, DWORD dwVnum = 0, const char* name = 0);

	LPCHARACTER			GetHorse() const { return m_chHorse; }
	LPCHARACTER			GetRider() const; // rider on horse
	void				SetRider(LPCHARACTER ch);

	bool				IsRiding() const;

#ifdef __PET_SYSTEM__
public:
	CPetSystem* GetPetSystem() { return m_petSystem; }

protected:
	CPetSystem* m_petSystem;

public:
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
public:
	CMountSystem* GetMountSystem() { return m_mountSystem; }

	void 				MountSummon(LPITEM mountItem);
	void 				MountUnsummon(LPITEM mountItem);
	void 				CheckMount();
	bool 				IsRidingMount();
protected:
	CMountSystem* m_mountSystem;
#endif

#ifdef ENABLE_PET_COSTUME_SYSTEM
public:
	void 				PetSummon(LPITEM PetItem);
	void 				PetUnsummon(LPITEM PetItem);
	void 				CheckPet();
#endif

protected:
	LPCHARACTER			m_chHorse;
	LPCHARACTER			m_chRider;

	DWORD				m_dwMountVnum;
	DWORD				m_dwMountTime;

	BYTE				m_bSendHorseLevel;
	BYTE				m_bSendHorseHealthGrade;
	BYTE				m_bSendHorseStaminaGrade;
	
#ifdef ENABLE_AFK_MODE_SYSTEM
	bool m_isAway;
#endif


public:
	void 				SetEmpire(BYTE bEmpire);
	BYTE				GetEmpire() const { return m_bEmpire; }

protected:
	BYTE				m_bEmpire;

	////////////////////////////////////////////////////////////////////////////////////////
	// Regen
public:
	void				SetRegen(LPREGEN pkRegen);

protected:
	PIXEL_POSITION			m_posRegen;
	float				m_fRegenAngle;
	LPREGEN				m_pkRegen;
	size_t				regen_id_; // to help dungeon regen identification

public:
	bool				CannotMoveByAffect() const;
	bool				IsImmune(DWORD dwImmuneFlag);
	void				SetImmuneFlag(DWORD dw) { m_pointsInstant.dwImmuneFlag = dw; }

protected:
	void				ApplyMobAttribute(const TMobTable* table);

public:
	void				SetQuestNPCID(DWORD vid);
	DWORD				GetQuestNPCID() const { return m_dwQuestNPCVID; }
	LPCHARACTER			GetQuestNPC() const;

	void				SetQuestItemPtr(LPITEM item);
	void				ClearQuestItemPtr();
	LPITEM				GetQuestItemPtr() const;

#ifdef ENABLE_QUEST_DND_EVENT
	void				SetQuestDNDItemPtr(LPITEM item);
	void				ClearQuestDNDItemPtr();
	LPITEM				GetQuestDNDItemPtr() const;
#endif

	void				SetQuestBy(DWORD dwQuestVnum) { m_dwQuestByVnum = dwQuestVnum; }
	DWORD				GetQuestBy() const { return m_dwQuestByVnum; }

	int					GetQuestFlag(const std::string& flag) const;
	void				SetQuestFlag(const std::string& flag, int value);

	void				ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID);

private:
	DWORD				m_dwQuestNPCVID;
	DWORD				m_dwQuestByVnum;
	LPITEM				m_pQuestItem;
#ifdef ENABLE_QUEST_DND_EVENT
	LPITEM				m_pQuestDNDItem{ nullptr };
#endif

	// Events
public:
	bool				StartStateMachine(int iPulse = 1);
	void				StopStateMachine();
	void				UpdateStateMachine(DWORD dwPulse);
	void				SetNextStatePulse(int iPulseNext);

	void				UpdateCharacter(DWORD dwPulse);

protected:
	DWORD				m_dwNextStatePulse;

	// Marriage
public:
	LPCHARACTER			GetMarryPartner() const;
	void				SetMarryPartner(LPCHARACTER ch);
	int					GetMarriageBonus(DWORD dwItemVnum, bool bSum = true);

	void				SetWeddingMap(marriage::WeddingMap* pMap);
	marriage::WeddingMap* GetWeddingMap() const { return m_pWeddingMap; }

private:
	marriage::WeddingMap* m_pWeddingMap;
	LPCHARACTER			m_pkChrMarried;

	// Warp Character
public:
	void				StartWarpNPCEvent();

public:
	void				StartSaveEvent();
	void				StartRecoveryEvent();
	void				StartDestroyWhenIdleEvent();

	LPEVENT				m_pkDeadEvent;
	LPEVENT				m_pkStunEvent;
	LPEVENT				m_pkSaveEvent;
	LPEVENT				m_pkRecoveryEvent;
	LPEVENT				m_pkTimedEvent;
	LPEVENT				m_pkFishingEvent;
	LPEVENT				m_pkAffectEvent;
	LPEVENT				m_pkPoisonEvent;
#ifdef ENABLE_WOLFMAN_CHARACTER
	LPEVENT				m_pkBleedingEvent;
#endif
	LPEVENT				m_pkFireEvent;
	LPEVENT				m_pkWarpNPCEvent;
#ifdef ENABLE_AFK_MODE_SYSTEM
	LPEVENT				m_pkUpdateCharacter;
#endif
	//DELAYED_WARP
	//END_DELAYED_WARP

	// MINING
	LPEVENT				m_pkMiningEvent;
	// END_OF_MINING
	LPEVENT				m_pkDestroyWhenIdleEvent;
	LPEVENT				m_pkPetSystemUpdateEvent;

	bool				m_bHasPoisoned;
#ifdef ENABLE_WOLFMAN_CHARACTER
	bool				m_bHasBled;
#endif

	const CMob* m_pkMobData;
	CMobInstance* m_pkMobInst;

	std::map<int, LPEVENT> m_mapMobSkillEvent;

	friend struct FuncSplashDamage;
	friend struct FuncSplashAffect;
	friend class CFuncShoot;

public:
	int				GetPremiumRemainSeconds(BYTE bType) const;

private:
	int				m_aiPremiumTimes[PREMIUM_MAX_NUM];

	// CHANGE_ITEM_ATTRIBUTES
	static const char		msc_szLastChangeItemAttrFlag[];
	// END_OF_CHANGE_ITEM_ATTRIBUTES

	// NEW_HAIR_STYLE_ADD
public:
	bool ItemProcess_Hair(LPITEM item, int iDestCell);
	// END_NEW_HAIR_STYLE_ADD

public:
	void ClearSkill();
	void ClearSubSkill();

	// RESET_ONE_SKILL
	bool ResetOneSkill(DWORD dwVnum);
	// END_RESET_ONE_SKILL

private:
	void SendDamagePacket(LPCHARACTER pAttacker, DAM_LL Damage, BYTE DamageFlag);

	// ARENA
private:
	CArena* m_pArena;
	bool m_ArenaObserver;
	int m_nPotionLimit;

public:
	void 	SetArena(CArena* pArena) { m_pArena = pArena; }
	void	SetArenaObserverMode(bool flag) { m_ArenaObserver = flag; }

	CArena* GetArena() const { return m_pArena; }
	bool	GetArenaObserverMode() const { return m_ArenaObserver; }

	void	SetPotionLimit(int count) { m_nPotionLimit = count; }
	int		GetPotionLimit() const { return m_nPotionLimit; }
	// END_ARENA
	void Say(const std::string& s);

	bool ItemProcess_Polymorph(LPITEM item);

	LPITEM* GetCubeItem() { return m_pointsInstant.pCubeItems; }
	bool IsCubeOpen() const { return (m_pointsInstant.pCubeNpc ? true : false); }
	void SetCubeNpc(LPCHARACTER npc) { m_pointsInstant.pCubeNpc = npc; }
	LPCHARACTER m_newCubeNpc;
	bool CanDoCube() const;

private:
	int		m_deposit_pulse;

public:
	void	UpdateDepositPulse();
	bool	CanDeposit() const;

private:
	void	__OpenPrivateShop();
private:
	std::string m_strNewName;

public:
	const std::string GetNewName() const { return this->m_strNewName; }
	void SetNewName(const std::string name) { this->m_strNewName = name; }

public:
	void GoHome();

private:
	std::set<DWORD>	m_known_guild;

public:
	void SendGuildName(CGuild* pGuild);
	void SendGuildName(DWORD dwGuildID);

private:
	DWORD m_dwLogOffInterval;

public:
	DWORD GetLogOffInterval() const { return m_dwLogOffInterval; }

public:
	bool CanWarp(bool notquest = false, bool dead = false) const;

public:
	void AutoRecoveryItemProcess(const EAffectTypes);

public:
	void BuffOnAttr_AddBuffsFromItem(LPITEM pItem);
	void BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem);

private:
	void BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue);
	void BuffOnAttr_ClearAll();

	typedef std::map <BYTE, CBuffOnAttributes*> TMapBuffOnAttrs;
	TMapBuffOnAttrs m_map_buff_on_attrs;

public:
	void SetArmada() { cannot_dead = true; }
	void ResetArmada() { cannot_dead = false; }
private:
	bool cannot_dead;

#ifdef __PET_SYSTEM__
private:
	bool m_bIsPet;
public:
	void SetPet() { m_bIsPet = true; }
	bool IsPet() { return m_bIsPet; }
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
private:
	bool m_bIsMount;
public:
	void SetMount() { m_bIsMount = true; }
	bool IsMount() { return m_bIsMount; }
#endif

#ifdef NEW_ICEDAMAGE_SYSTEM
private:
	DWORD m_dwNDRFlag;
	std::set<DWORD> m_setNDAFlag;
public:
	const DWORD GetNoDamageRaceFlag();
	void SetNoDamageRaceFlag(DWORD dwRaceFlag);
	void UnsetNoDamageRaceFlag(DWORD dwRaceFlag);
	void ResetNoDamageRaceFlag();
	const std::set<DWORD>& GetNoDamageAffectFlag();
	void SetNoDamageAffectFlag(DWORD dwAffectFlag);
	void UnsetNoDamageAffectFlag(DWORD dwAffectFlag);
	void ResetNoDamageAffectFlag();
#endif

private:
	DAM_DOUBLE m_fAttMul;
	DAM_DOUBLE m_fDamMul;
public:
	DAM_DOUBLE GetAttMul() { return this->m_fAttMul; }
	void SetAttMul(DAM_DOUBLE newAttMul) { this->m_fAttMul = newAttMul; }
	DAM_DOUBLE GetDamMul() { return this->m_fDamMul; }
	void SetDamMul(DAM_DOUBLE newDamMul) { this->m_fDamMul = newDamMul; }

private:
	bool IsValidItemPosition(TItemPos Pos) const;

public:

	void	DragonSoul_Initialize();

	bool	DragonSoul_IsQualified() const;
	void	DragonSoul_GiveQualification();

	int		DragonSoul_GetActiveDeck() const;
	bool	DragonSoul_IsDeckActivated() const;
	bool	DragonSoul_ActivateDeck(int deck_idx);

	void	DragonSoul_DeactivateAll();
#ifdef ENABLE_DS_SET_BONUS
	void	DragonSoul_SetRefresh();
#endif
	//

	void	DragonSoul_CleanUp();

public:
	bool		DragonSoul_RefineWindow_Open(LPENTITY pEntity);
	bool		DragonSoul_RefineWindow_Close();
	LPENTITY	DragonSoul_RefineWindow_GetOpener() { return  m_pointsInstant.m_pDragonSoulRefineWindowOpener; }
	bool		DragonSoul_RefineWindow_CanRefine();

private:
	unsigned int itemAward_vnum;
	char		 itemAward_cmd[20];
	//bool		 itemAward_flag;
public:
	unsigned int GetItemAward_vnum() { return itemAward_vnum; }
	char* GetItemAward_cmd() { return itemAward_cmd; }
	//bool		 GetItemAward_flag() { return itemAward_flag; }
	void		 SetItemAward_vnum(unsigned int vnum) { itemAward_vnum = vnum; }
	void		 SetItemAward_cmd(char* cmd) { strcpy(itemAward_cmd, cmd); }
	//void		 SetItemAward_flag(bool flag) { itemAward_flag = flag; }
#ifdef ENABLE_ANTI_CMD_FLOOD
private:
	int m_dwCmdAntiFloodPulse;
	DWORD m_dwCmdAntiFloodCount;
public:
	int GetCmdAntiFloodPulse() { return m_dwCmdAntiFloodPulse; }
	DWORD GetCmdAntiFloodCount() { return m_dwCmdAntiFloodCount; }
	DWORD IncreaseCmdAntiFloodCount() { return ++m_dwCmdAntiFloodCount; }
	void SetCmdAntiFloodPulse(int dwPulse) { m_dwCmdAntiFloodPulse = dwPulse; }
	void SetCmdAntiFloodCount(DWORD dwCount) { m_dwCmdAntiFloodCount = dwCount; }
#endif
private:

	timeval		m_tvLastSyncTime;
	int			m_iSyncHackCount;
	bool	m_isOpenSafebox;
public:
	void			SetLastSyncTime(const timeval& tv) { memcpy(&m_tvLastSyncTime, &tv, sizeof(timeval)); }
	const timeval& GetLastSyncTime() { return m_tvLastSyncTime; }
	void			SetSyncHackCount(int iCount) { m_iSyncHackCount = iCount; }
	int				GetSyncHackCount() { return m_iSyncHackCount; }

	int		GetSkillPowerByLevel(int level, bool bMob = false) const;
	bool	IsOpenSafebox() const { return m_isOpenSafebox ? true : false; }
	void 	SetOpenSafebox(bool b) { m_isOpenSafebox = b; }

	// @fixme188 BEGIN
public:
	void ResetRewardInfo() {
		m_map_kDamage.clear();
		if (!IsPC())
			SetExp(0);
	}
	void DeadNoReward() {
		if (!IsDead())
			ResetRewardInfo();
		Dead();
	}
	// @fixme188 END

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
protected:
	bool	m_bAcceCombination, m_bAcceAbsorption;

public:
	bool	IsAcceOpened(bool bCombination) { return bCombination ? m_bAcceCombination : m_bAcceAbsorption; }
	bool	IsAcceOpened() const { return (m_bAcceCombination || m_bAcceAbsorption); }
	void	OpenAcce(bool bCombination);
	void	CloseAcce();
	void	ClearAcceMaterials();
	bool	CleanAcceAttr(LPITEM pkItem, LPITEM pkTarget);
	std::vector<LPITEM>	GetAcceMaterials();
	const TItemPosEx* GetAcceMaterialsInfo();
	void	SetAcceMaterial(int pos, LPITEM ptr);
	bool	AcceIsSameGrade(long lGrade);
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	GetAcceCombinePrice(long lGrade);
#else
	DWORD	GetAcceCombinePrice(long lGrade);
#endif
	void	GetAcceCombineResult(DWORD& dwItemVnum, DWORD& dwMinAbs, DWORD& dwMaxAbs);
	BYTE	CheckEmptyMaterialSlot();
	void	AddAcceMaterial(TItemPos tPos, BYTE bPos);
	void	RemoveAcceMaterial(BYTE bPos);
	BYTE	CanRefineAcceMaterials();
	void	RefineAcceMaterials();
#endif

#ifdef ENABLE_FALL_FIX
public:
	bool	CanFall();
#endif

#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
public:
	bool			SwitchChannel(long newAddr, WORD newPort);
	bool			StartChannelSwitch(long newAddr, WORD newPort);
#endif

//CHARACTER ACTIVATE CHECK BEGIN
public:
	void			WindowCloseAll();
	bool			WindowOpenCheck() const;
	bool			ActivateCheck(bool notquest = false, bool dead = false) const;
	bool			OfflineShopActivateCheck() const;
	bool			IsOpenedRefine() const { return m_bUnderRefine; }
	int				GetActivateTime(BYTE type) const { return m_ActivateTime[type]; }
	bool			IsActivateTime(BYTE type,BYTE second) const;
	void			SetActivateTime(BYTE type) { m_ActivateTime[type] = thecore_pulse();}

	bool			IsOpenOfflineShop() const;
	bool			IsOfflineShopActivateTime(BYTE second) const;
private:
	int m_ActivateTime[MAX_ACTIVATE_CHECK];
//CHARACTER ACTIVATE CHECK END

public:
#ifdef ENABLE_SORT_INVENTORIES
	int				m_sortLastUsed;
#endif

#ifdef ENABLE_CHEST_OPEN_RENEWAL
public:
	int				IsEmptyUpgradeInventory(BYTE freeSize) const;
	int				IsEmptyBookInventory(BYTE freeSize) const;
	int				IsEmptyStoneInventory(BYTE freeSize) const;
	int				IsEmptyChestInventory(BYTE freeSize) const;

	int				GiveItemEmpetyCalcute(DWORD dwItemVnum, DWORD wCount, BYTE window_type);
	bool			GiveItemEmpetyCalcute2(TChestEmpetInventory* empety);
	bool			GiveItemEmpetyCalcute2Pct(TChestEmpetInventory* empety, TChestEmpetInventorySize* itemsize);
	void			ChestOpenConst(LPITEM item);
	void			ChestOpenConstSingle(LPITEM item);
	void			ChestOpenNewPct(LPITEM item, BYTE chesttype);

	void			ChestOpenDragonSoul(LPITEM item, bool multiopen);
	bool			ChestDSEmpty(DWORD itemcount, int type, DWORD chestVnum);
	int				ChestDSEmptyCount(DWORD chestVnum);
	bool			ChestDSInventoryEmpety(DWORD chestvnum);
	int				ChestDSType(DWORD vnum, bool chest = false);

	void			ChestOpenNewPctSingle(LPITEM item);
	bool			GiveCalcSpecialGroup(DWORD dwGroupNum, std::vector <DWORD>& dwItemVnums,
		std::vector <DWORD>& dwItemCounts, int& count);
	bool			ChestInventoryEmpetyFull();
#endif

#ifdef ENABLE_REFRESH_CONTROL
public:
	void			RefreshControl(BYTE SubHeader, bool state, bool update = false);
#endif

#ifdef ENABLE_LOADING_RENEWAL
public:
	void SetLoadingState(BYTE state) { m_loading_state = state; }
	BYTE GetLoadingState() { return m_loading_state; }
private:
	BYTE m_loading_state;
#endif


#ifdef ENABLE_PLAYER_STATS_SYSTEM
public:
	DWORD GetStats(BYTE statsType) const { return m_points.player_stats.stat[statsType]; }
	DWORD SetStats(BYTE statsType, DWORD value)	{ m_points.player_stats.stat[statsType] = value; return m_points.player_stats.stat[statsType];}

#ifdef ENABLE_POWER_RANKING
	void CalcutePowerRank();
#endif
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
public:
	BiologSystem* GetBiolog() { return m_biologsystem; }
	TPacketGCBiologUpdate GetBiologInfo() const { return m_BiologInfo; }

	void SetBiologInfo(TPacketGCBiologUpdate data) { m_BiologInfo = data; }
	void SetBiologLevel(BYTE level) { m_BiologInfo.level = level; }
	void SetBiologTime(DWORD time) { m_BiologInfo.time = time; }
	void SetBiologCount(MAX_COUNT count) { m_BiologInfo.given_count = count; }
	void SetBiologAlert(bool state) { m_BiologInfo.alert = state; }
	void SetBiologState(bool state) { m_BiologInfo.state = state; }

	void BiologAlertEventStart();
	void BiologAlertEventStop();
	LPEVENT m_biolog_alert;

private:
	BiologSystem* m_biologsystem;
	TPacketGCBiologUpdate m_BiologInfo;
#endif

#ifdef ENABLE_CHAR_SETTINGS
public:
	TCharSettings GetCharSettings() { return m_char_settings; }
	void SetCharSettings(TCharSettings data) { m_char_settings = data; }
	void SetSettingsAntiExp(bool state) { m_char_settings.antiexp = state; }

	void SendCharSettingsPacket(BYTE type = 31);
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	void SetHideCostumeState(BYTE hideType, bool hideState);
	bool IsCostumeHairHidden()		 const { return m_char_settings.HIDE_COSTUME_HAIR; };
	bool IsCostumeBodyHidden()		 const { return m_char_settings.HIDE_COSTUME_BODY; };
	bool IsCostumeWeaponHidden()	 const { return m_char_settings.HIDE_COSTUME_WEAPON; };
	bool IsCostumeAcceHidden()		 const { return m_char_settings.HIDE_COSTUME_ACCE; };
	bool IsCostumeAuraHidden()		 const { return m_char_settings.HIDE_COSTUME_AURA; };
	bool IsCrownHidden()			 const { return m_char_settings.HIDE_COSTUME_CROWN; };
#endif

private:
	TCharSettings m_char_settings;
#endif

#ifdef ENABLE_LMW_PROTECTION
public:
	void CalculateAttackCount();
	void SetAttackCount(DWORD victimID);
	bool GetAttackHack();
	void AttackHackControlEventStart();
	void AttackHackControlEventStop();
	void HackLogAdd(bool botControl = false);
	LPEVENT m_attackHackControl;
private:
	std::map<DWORD, BYTE>m_mapAttackCount;
	BYTE m_maxAttackCount;
#endif

#ifdef ENABLE_BOT_CONTROL
public:
	void SetBotControl(bool state) { m_isBotControl = state; }
	bool IsBotControl() { return m_isBotControl; }
private:
	bool m_isBotControl;
#endif

#ifdef ENABLE_FISHING_RENEWAL
public:
	BYTE m_bFishGameFishCounter;
	int m_fishEventTime;
#endif

#ifdef ENABLE_STOP_CHAT
public:
	void SetStopChatState(bool state) { m_char_settings.STOP_SHOUT = state; }
	bool GetStopChatState()		 const { return m_char_settings.STOP_SHOUT; };
	int	 m_stopChatCooldown;
#endif

#ifdef ENABLE_ITEM_TRANSACTIONS
public:
	void ItemTransactionDelete(std::vector<TItemTransactionsInfo>& v_item);
	void ItemTransactionSell(std::vector<TItemTransactionsInfo>& v_item);
#endif

#ifdef ENABLE_FARM_BLOCK
public:
	bool IsFarmBlock() { return m_farmBlock; }
	void SetFarmBlock(bool block, BYTE result);
	int GetFarmBlockSetTime() { return m_farmBlockSetTime; }
	void SetWarpCheck(bool state) { m_warpEvent = state; }
	bool IsWarp() { return m_warpEvent; }
private:
	bool m_farmBlock;
	int m_farmBlockSetTime;
	bool m_warpEvent;
#endif

#ifdef ENABLE_DUNGEON_INFO
public:
	void SendDungeonInfoTime(BYTE idx = 0, bool update = false);
	int GetDungeonReJoinTime(BYTE idx);
	void SetDungeonReJoinTime(BYTE idx, int coolDown);
	void DungeonJoinBegin(BYTE idx);
	void DungeonJoinEnd(BYTE idx, long mapIdx);
	void DungeonSendMessage(BYTE messageID);
#endif


#ifdef ENABLE_ITEMSHOP
public:
	long long			GetDragonCoin();
	void				SetDragonCoin(long long amount);
	void				SetProtectTime(const std::string& flagname, int value);
	int					GetProtectTime(const std::string& flagname) const;
protected:
	std::map<std::string, int>  m_protection_Time;
#endif

public:
	int				m_potionUseTime;
	int				m_campFireUseTime;//lightwork campfire spam fixed

#ifdef ENABLE_DROP_ITEM_TO_INVENTORY
public:
	bool DropItemToInventory(LPITEM item);
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
public:
	CNewPet* GetNewPetSystem() { return m_newpetSystem; }
	void	SendPetLevelUpEffect();
	void SetNewPet() { m_bIsNewPet = true; }
	bool IsNewPet() const { return m_bIsNewPet ? true : false; }
	void SummonNewPet();
protected:
	CNewPet* m_newpetSystem;
private:
	bool m_bIsNewPet;
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
public:
	void SetSkillColor(DWORD* dwSkillColor);
	DWORD* GetSkillColor() { return m_dwSkillColor[0]; }

protected:
	DWORD m_dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif

#ifdef ENABLE_AURA_SYSTEM
protected:
	bool	m_bAuraRefine, m_bAuraAbsorption;

public:
	bool	isAuraOpened(bool bCombination) const { return bCombination ? m_bAuraRefine : m_bAuraAbsorption; }
	void    OpenAura(bool bCombination);
	void    CloseAura();
	void    ClearAuraMaterials();
	bool    CleanAuraAttr(LPITEM pkItem, LPITEM pkTarget);
	LPITEM* GetAuraMaterials() { return m_pointsInstant.pAuraMaterials; }
	bool    AuraIsSameGrade(long lGrade);
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	unsigned long long    GetAuraCombinePrice(long long lGrade);
#else
	DWORD    GetAuraCombinePrice(long lGrade);
#endif
	void    GetAuraCombineResult(DWORD& dwItemVnum, DWORD& dwMinAbs, DWORD& dwMaxAbs);
	BYTE    CheckAuraEmptyMaterialSlot();
	void    AddAuraMaterial(TItemPos tPos, BYTE bPos);
	void    RemoveAuraMaterial(BYTE bPos);
	BYTE    CanRefineAuraMaterials();
	void    RefineAuraMaterials();
#endif

#ifdef ENABLE_NAMING_SCROLL
public:
	const char* GetMobNameScroll(BYTE type);
	void NamingScrollNameClear(BYTE type) { m_namingScroll[type] = ""; }
private:
	std::string m_namingScroll[NAME_SCROLL_MAX_NUM];
#endif

#ifdef ENABLE_JEWELS_RENEWAL
public:
	void UseAddJewels(LPITEM item, LPITEM targetItem);
	void UsePutIntoJewels(LPITEM item, LPITEM targetItem);
#endif

#ifdef ENABLE_MINING_RENEWAL
public:
	void AddMiningTotalPoints(const BYTE addThis) { m_bMiningTotalPoints += addThis; }
	int GetMiningTotalPoints() const { return m_bMiningTotalPoints; }

	void SetMiningNPC(const LPCHARACTER npc) { lodeNpc = npc; }
	LPCHARACTER GetMiningNPC() const { return lodeNpc; }
	WORD GetMiningNPCVnum() const;
	BYTE ComputePickaxePoint();

	// Packet 
	void SendDiggingMotion();
	void CreateMiningEvent();
	void EndMiningEvent();

private:
	BYTE m_bMiningTotalPoints;
	LPCHARACTER lodeNpc;
#endif

public:
	void TestYapcam31(BYTE value);

public:
	bool IsInVillage();

#ifdef ENABLE_VOTE4BUFF
public:
	void	SelectVoteBuff(BYTE buffType);
	void	AddVoteBuff(BYTE buffType, time_t time);
	void	CheckVoteBuff();
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
private:
	typedef std::list<TPlayerExtBattlePassMission*> ListExtBattlePassMap;
public:
	void LoadExtBattlePass(DWORD dwCount, TPlayerExtBattlePassMission* data);
	DWORD GetExtBattlePassMissionProgress(DWORD dwBattlePassType, BYTE bMissionIndex, BYTE bMissionType);
	bool IsExtBattlePassCompletedMission(DWORD dwBattlePassType, BYTE bMissionIndex, BYTE bMissionType);
	bool IsExtBattlePassRegistered(BYTE bBattlePassType, DWORD dwBattlePassID);
	void UpdateExtBattlePassMissionProgress(DWORD dwMissionID, DWORD dwUpdateValue, DWORD dwCondition, bool isOverride = false);
	void SetExtBattlePassMissionProgress(BYTE bBattlePassType, DWORD dwMissionIndex, DWORD dwMissionType, DWORD dwUpdateValue);

	bool		IsLoadedExtBattlePass()		const { return m_bIsLoadedExtBattlePass; }
	int			GetExtBattlePassPremiumID()	const { return m_points.battle_pass_premium_id; }
	void		SetExtBattlePassPremiumID(int battle_pass_premium_id) { m_points.battle_pass_premium_id = battle_pass_premium_id; }

	void				SetLastReciveExtBattlePassInfoTime(DWORD time);
	DWORD			GetLastReciveExtBattlePassInfoTime() const { return m_dwLastReciveExtBattlePassInfoTime; }
	void				SetLastReciveExtBattlePassOpenRanking(DWORD time);
	DWORD			GetLastReciveExtBattlePassOpenRanking() const { return m_dwLastExtBattlePassOpenRankingTime; }
protected:
	DWORD	m_dwLastReciveExtBattlePassInfoTime;
	DWORD	m_dwLastExtBattlePassOpenRankingTime;

private:
	bool m_bIsLoadedExtBattlePass;
	ListExtBattlePassMap m_listExtBattlePass;
#endif

#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
public:
	int GetBookVnumBySkillIndex(int skillIndex);
	int	m_readBookCooldown;
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
public:
	void CheckPartyBonus();
	int	m_partyAddBonusPulse;
#endif

#ifdef ENABLE_6TH_7TH_ATTR
public:
	LPITEM			GetAttrInventoryItem() const;
	void			Give67AttrItem();
	void Open67Attr();
	void Set67Attr(bool b) { b67Attr = b; }
	bool Is67AttrOpen() const { return b67Attr; }
	void Change67Attr(LPITEM changer, LPITEM item);

	void Decrease67AttrCooldown(LPITEM cooldownItem);

private:
	bool b67Attr;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
public:
	CBuffiSystem* GetBuffiSystem() { return m_buffiSystem; }
	void SetBuffi(bool val) { m_isBuffi = val; }
	bool IsBuffi() const { return m_isBuffi; }
	void SendBuffiSkillPacket(DWORD skill_vnum);
	bool CanBuffiEquipItem(TItemPos& cell, LPITEM item);

	DWORD GetBuffiItem(BYTE idx) { return m_buffiItem[idx]; }
	void SetBuffiItem(BYTE idx, DWORD vnum) { m_buffiItem[idx] = vnum; }
protected:
	CBuffiSystem* m_buffiSystem;
private:
	bool m_isBuffi;
	int m_buffiItem[BUFFI_MAX_SLOT];
#endif

#ifdef ENABLE_CHAT_COLOR_SYSTEM
public:
	void SetChatColor(BYTE newColor) { m_points.chatColor = newColor; }
	BYTE GetChatColor() const { return m_points.chatColor; }
#endif

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
public:
	bool	IsPremium() const { return m_isPremium; }
private:
	bool	m_isPremium;
#endif

#ifdef ENABLE_AUTO_PICK_UP
public:
	bool	IsAutoPickUp() { return m_isAutoPickUp; }
	void	SetAutoPickUp(bool state) { m_isAutoPickUp = state; }
private:
	bool	m_isAutoPickUp;
#endif
#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
public:
	void SetFastReadEvent(LPEVENT event) { m_fastReadEvent = event; }
	LPEVENT GetFastReadEvent() { return m_fastReadEvent; }
	LPEVENT m_fastReadEvent;
#endif

#ifdef ENABLE_DUNGEON_TURN_BACK
public:
	void SetPlayerDungeonInfo(TDungeonPlayerInfo data) { m_dungeonInfo = data; }
	TDungeonPlayerInfo GetPlayerDungeonInfo() { return m_dungeonInfo; }
private:
	TDungeonPlayerInfo m_dungeonInfo;
#endif

#ifdef ENABLE_BOOSTER_ITEM
public:
	void CalcuteBoosterSetBonus();
	bool GetBoosterSetBonus() { return m_boosterSetBonus; }
	void BoosterGiveBuff();
private:
	bool m_boosterSetBonus;
#endif

#ifdef ENABLE_COSTUME_ATTR
public:
	void ChangeCostumeAttr(LPITEM costume, LPITEM changer);
#endif

#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
public:
	struct S_CARD
	{
		uint32_t type;
		uint32_t value;
	};

	struct CARDS_INFO
	{
		S_CARD cards_in_hand[EMonsterOkeyCardEvent::HAND_CARD_INDEX_MAX];
		S_CARD cards_in_field[EMonsterOkeyCardEvent::FIELD_CARD_INDEX_MAX];
		uint32_t cards_left;
		uint32_t field_points;
		uint32_t points;
	};

	void Cards_open(uint8_t safemode);
	void Cards_clean_list();
	uint32_t GetEmptySpaceInHand();
	void Cards_pullout();
	void RandomizeCards();
	bool CardWasRandomized(uint32_t type, uint32_t value);
	void SendUpdatedInformations();
	void SendReward();
	void CardsDestroy(uint32_t reject_index);
	void CardsAccept(uint32_t accept_index);
	void CardsRestore(uint32_t restore_index);
	uint32_t GetEmptySpaceInField();
	uint32_t GetAllCardsCount();
	bool TypesAreSame();
	bool ValuesAreSame();
	bool CardsMatch();
	uint32_t GetLowestCard();
	bool CheckReward();
	void CheckCards();
	void RestoreField();
	void ResetField();
	void CardsEnd();

protected:
	CARDS_INFO character_cards;
	S_CARD randomized_cards[EMonsterOkeyCardEvent::DECK_COUNT_MAX];
#endif // ENABLE_MINI_GAME_OKEY_NORMAL

#ifdef ENABLE_MINI_GAME_BNW
	public:
		DWORD m_dwFishingBarTimer;	

	public:
		bool IsMinigameBnwStarted() { return m_bMinigameBnwIsStarted; }
		void SetMinigameBnwStarted(bool bIsStarted) { m_bMinigameBnwIsStarted = bIsStarted; }
		std::vector<BYTE> m_vMinigameBnwMyCards;
		std::vector<BYTE> m_vMinigameBnwEnemyCards;
		void MinigameBnwSetPlayerPoints(BYTE bPoints) { m_bMinigameBnwPlayerPoints = bPoints; }
		BYTE MinigameBnwGetPlayerPoints() { return m_bMinigameBnwPlayerPoints; }
		void MinigameBnwSetEnemyPoints(BYTE bPoints) { m_bMinigameBnwEnemyPoints = bPoints; }
		BYTE MinigameBnwGetEnemyPoints() { return m_bMinigameBnwEnemyPoints; }

	private:
		bool m_bMinigameBnwIsStarted;
		BYTE m_bMinigameBnwPlayerPoints;
		BYTE m_bMinigameBnwEnemyPoints;
#endif

#ifdef ENABLE_MINI_GAME_CATCH_KING
public:
	void MiniGameCatchKingSetFieldCards(std::vector<TCatchKingCard> vec) { m_vecCatchKingFieldCards = vec; }

	uint32_t MiniGameCatchKingGetScore() const noexcept { return dwCatchKingTotalScore; }
	void MiniGameCatchKingSetScore(uint32_t dwScore) noexcept { dwCatchKingTotalScore = dwScore; }

	uint32_t MiniGameCatchKingGetBetNumber() const noexcept { return bCatchKingBetSetNumber; }
	void MiniGameCatchKingSetBetNumber(uint8_t bSetNr) noexcept { bCatchKingBetSetNumber = bSetNr; }

	uint8_t MiniGameCatchKingGetHandCard() const noexcept { return bCatchKingHandCard; }
	void MiniGameCatchKingSetHandCard(uint8_t bKingCard) noexcept { bCatchKingHandCard = bKingCard; }

	uint8_t MiniGameCatchKingGetHandCardLeft() const noexcept { return bCatchKingHandCardLeft; }
	void MiniGameCatchKingSetHandCardLeft(uint8_t bHandCard) noexcept { bCatchKingHandCardLeft = bHandCard; }

	bool MiniGameCatchKingGetGameStatus() const noexcept { return dwCatchKingGameStatus; }
	void MiniGameCatchKingSetGameStatus(bool bStatus) noexcept { dwCatchKingGameStatus = bStatus; }

	std::vector<TCatchKingCard> m_vecCatchKingFieldCards;

protected:
	uint8_t bCatchKingHandCard;
	uint8_t bCatchKingHandCardLeft;
	bool dwCatchKingGameStatus;
	uint8_t bCatchKingBetSetNumber;
	uint32_t dwCatchKingTotalScore;
#endif // ENABLE_MINI_GAME_CATCH_KING

#ifdef ENABLE_EVENT_MANAGER
	void ConvertEventItems(LPITEM item);
#endif

#ifdef ENABLE_OFFICIAL_STUFF
protected:
	int				m_iCapeEffectPulse;

public:
	void			SetCapeEffectPulse(const int i) { m_iCapeEffectPulse = i; }
	int				GetCapeEffectPulse() { return m_iCapeEffectPulse; }
#endif

public:
#ifdef ENABLE_INCREASED_MOV_SPEED_MOBS
	int GetIncreasedSpeed() const;
#endif

#ifdef ENABLE_FURKANA_GOTTEN
	void DropCalculator(DWORD mobVnum, int mobCount);
#endif

#if defined(__GAME_OPTION_ESCAPE__)
public:
	void SetEscapeCooltime(const DWORD dwTime) { m_dwEscapeCooltime = dwTime; }
	DWORD GetEscapeCooltime() const { return m_dwEscapeCooltime; }
private:
	DWORD m_dwEscapeCooltime;
#endif

#ifdef ENABLE_EXPRESSING_EMOTION
	struct EmotionsValues
	{
		int 	id_emotion;
		DWORD 	tiempo_emotion;

	}save_info_emotion,copy_info_emotion;

	public:

		void LoadingInfoEmotions();
		void LoadInfoEmotions();
		void LoadEmotions();
		bool CheckEmotionList(int emotion);
		int EmotionsList();
		void InsertEmotion();
		int CountEmotion();
		void StartCheckTimeEmotion();
		int get_time_emotion(int value);
		int get_id_emotion(int value);


	private:
		LPEVENT	TimeEmotionUpdateTime;
		std::vector<EmotionsValues> load_info_emotion;	
#endif

};

ESex GET_SEX(LPCHARACTER ch);

#endif
