#include "stdafx.h"
#include "../../common/VnumHelper.h"
#include "char.h"
#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "char_manager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "item_manager.h"
#include "motion.h"
#include "vector.h"
#include "packet.h"
#include "cmd.h"
#include "fishing.h"
#include "exchange.h"
#include "battle.h"
#include "affect.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "pvp.h"
#include "party.h"
#include "start_position.h"
#include "questmanager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "war_map.h"
#include "banword.h"
#include "target.h"
#include "wedding.h"
#include "mob_manager.h"
#include "mining.h"
#include "arena.h"
#include "dev_log.h"
#include "horsename_manager.h"
#include "gm.h"
#include "map_location.h"
#include "skill_power.h"
#include "buff_on_attributes.h"
#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
#include "bossdamagerankingmanager.hpp"
#endif
#include "DragonSoul.h"
#include "../../common/Controls.h"
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#include "MountSystem.h"
#endif
#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#include <algorithm>
#include <iterator>
using namespace std;
#endif
#ifdef ENABLE_OFFLINE_SHOP
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
#endif
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif

#ifdef ENABLE_FARM_BLOCK
#include "hwid_manager.h"
#endif

#ifdef ENABLE_ALIGNMENT_SYSTEM
#include "alignment_proto.h"
#endif

#ifdef ENABLE_DUNGEON_INFO
#include "dungeon_info.h"
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif

#ifdef ENABLE_MINING_RENEWAL
#include "mining.h"
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
#include "battlepass_manager.h"
#endif
#include "profiler.h"

#ifdef ENABLE_BUFFI_SYSTEM
#include "BuffiSystem.h"
#endif

#ifdef OX_EVENT_DOUBLE_CONNECT_DISABLE
#include "OXEvent.h"
#endif

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
#include "player_block.h"
#endif


extern const BYTE g_aBuffOnAttrPoints;
extern bool RaceToJob(unsigned race, unsigned* ret_job);

extern bool IS_SUMMONABLE_ZONE(int map_index); // char_item.cpp
bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index);

bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index)
{
	switch (map_index)
	{
	case 301:
	case 302:
	case 303:
	case 304:
		if (ch->GetLevel() < 90)
			return false;
	}
	return true;
}

#ifdef NEW_ICEDAMAGE_SYSTEM
const DWORD CHARACTER::GetNoDamageRaceFlag()
{
	return m_dwNDRFlag;
}

void CHARACTER::SetNoDamageRaceFlag(DWORD dwRaceFlag)
{
	if (dwRaceFlag >= MAIN_RACE_MAX_NUM) return;
	if (IS_SET(m_dwNDRFlag, 1 << dwRaceFlag)) return;
	SET_BIT(m_dwNDRFlag, 1 << dwRaceFlag);
}

void CHARACTER::UnsetNoDamageRaceFlag(DWORD dwRaceFlag)
{
	if (dwRaceFlag >= MAIN_RACE_MAX_NUM) return;
	if (!IS_SET(m_dwNDRFlag, 1 << dwRaceFlag)) return;
	REMOVE_BIT(m_dwNDRFlag, 1 << dwRaceFlag);
}

void CHARACTER::ResetNoDamageRaceFlag()
{
	m_dwNDRFlag = 0;
}

const std::set<DWORD>& CHARACTER::GetNoDamageAffectFlag()
{
	return m_setNDAFlag;
}

void CHARACTER::SetNoDamageAffectFlag(DWORD dwAffectFlag)
{
	m_setNDAFlag.insert(dwAffectFlag);
}

void CHARACTER::UnsetNoDamageAffectFlag(DWORD dwAffectFlag)
{
	m_setNDAFlag.erase(dwAffectFlag);
}

void CHARACTER::ResetNoDamageAffectFlag()
{
	m_setNDAFlag.clear();
}
#endif

// <Factor> DynamicCharacterPtr member function definitions

LPCHARACTER DynamicCharacterPtr::Get() const {
	LPCHARACTER p = NULL;
	if (is_pc) {
		p = CHARACTER_MANAGER::instance().FindByPID(id);
	}
	else {
		p = CHARACTER_MANAGER::instance().Find(id);
	}
	return p;
}

DynamicCharacterPtr& DynamicCharacterPtr::operator=(LPCHARACTER character) {
	if (character == NULL) 
	{
		Reset();
		return *this;
	}
	if (character->IsPC()) 
	{
		is_pc = true;
		id = character->GetPlayerID();
	}
	else 
	{
		is_pc = false;
		id = character->GetVID();
	}
	return *this;
}

CHARACTER::CHARACTER()
{
	m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateIdle, &CHARACTER::EndStateEmpty);
	m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
	m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateBattle, &CHARACTER::EndStateEmpty);

	Initialize();
}

CHARACTER::~CHARACTER()
{
	Destroy();
}

void CHARACTER::Initialize()
{
	CEntity::Initialize(ENTITY_CHARACTER);

	m_bNoOpenedShop = true;

	m_bOpeningSafebox = false;

	m_fSyncTime = get_float_time() - 3;
	m_dwPlayerID = 0;
#ifdef ENABLE_RENEWAL_PVP
	memset(&pvpSettings, false, sizeof(pvpSettings));
#endif
	m_dwKillerPID = 0;
	
#if defined(__GAME_OPTION_ESCAPE__)
	m_dwEscapeCooltime = 0;
#endif

	m_iMoveCount = 0;

	m_pkRegen = NULL;
#ifdef __AUTO_HUNT__
	m_bAutoHuntStatus = false;
#endif
	regen_id_ = 0;
	m_posRegen.x = m_posRegen.y = m_posRegen.z = 0;
	m_posStart.x = m_posStart.y = 0;
	m_posDest.x = m_posDest.y = 0;
	m_fRegenAngle = 0.0f;

	m_pkMobData = NULL;
	m_pkMobInst = NULL;

	m_pkShop = NULL;
	m_pkChrShopOwner = NULL;
	m_pkMyShop = NULL;
	m_pkExchange = NULL;
	m_pkParty = NULL;
	m_pkPartyRequestEvent = NULL;

	m_pGuild = NULL;

	m_pkChrTarget = NULL;

	m_pkMuyeongEvent = NULL;

	m_pkWarpNPCEvent = NULL;
	m_pkDeadEvent = NULL;
	m_pkStunEvent = NULL;
	m_pkSaveEvent = NULL;
	m_pkRecoveryEvent = NULL;
	m_pkTimedEvent = NULL;
	m_pkFishingEvent = nullptr;
	// MINING
	m_pkMiningEvent = nullptr;
	// END_OF_MINING

	m_pkPoisonEvent = NULL;
#ifdef ENABLE_WOLFMAN_CHARACTER
	m_pkBleedingEvent = NULL;
#endif
	m_pkFireEvent = NULL;

	m_pkAffectEvent = NULL;
	m_afAffectFlag = TAffectFlag(0, 0);

	m_pkDestroyWhenIdleEvent = NULL;

	m_pkChrSyncOwner = NULL;

	m_points = {};
	m_pointsInstant = {};
	memset(&m_quickslot, 0, sizeof(m_quickslot));

	m_bCharType = CHAR_TYPE_MONSTER;

	SetPosition(POS_STANDING);

	m_dwPlayStartTime = m_dwLastMoveTime = get_dword_time();

	GotoState(m_stateIdle);
	m_dwStateDuration = 1;

	m_dwLastAttackTime = get_dword_time() - 20000;

	m_bAddChrState = 0;

	m_pkChrStone = NULL;

	m_pkSafebox = NULL;
	m_iSafeboxSize = -1;

	m_pkMall = NULL;
	m_iMallLoadTime = 0;

	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;
	m_lWarpMapIndex = 0;

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;

	m_pSkillLevels = NULL;

	m_dwMoveStartTime = 0;
	m_dwMoveDuration = 0;

	m_dwFlyTargetID = 0;

	m_dwNextStatePulse = 0;

	m_dwLastDeadTime = get_dword_time() - 180000;

	m_bSkipSave = false;

	m_bItemLoaded = false;
	
#ifdef ENABLE_AFK_MODE_SYSTEM
	m_pkUpdateCharacter = NULL;
	m_isAway = false;
#endif

	m_bHasPoisoned = false;
#ifdef ENABLE_WOLFMAN_CHARACTER
	m_bHasBled = false;
#endif
	m_pkDungeon = NULL;
	m_iEventAttr = 0;

	m_bNowWalking = m_bWalking = false;
	ResetChangeAttackPositionTime();
	m_bDisableCooltime = false;

	m_iAlignment = 0;
	m_iRealAlignment = 0;

	m_iKillerModePulse = 0;
	m_bPKMode = PK_MODE_PEACE;

	m_dwQuestNPCVID = 0;
	m_dwQuestByVnum = 0;
	m_pQuestItem = NULL;

	m_dwUnderGuildWarInfoMessageTime = get_dword_time() - 60000;

	m_bUnderRefine = false;

	// REFINE_NPC
	m_dwRefineNPCVID = 0;
	// END_OF_REFINE_NPC

	m_dwPolymorphRace = 0;

	m_bStaminaConsume = false;

	ResetChainLightningIndex();

	m_dwMountVnum = 0;
	m_chHorse = NULL;
	m_chRider = NULL;

	m_pWarMap = NULL;
	m_pWeddingMap = NULL;
	m_bChatCounter = 0;

	ResetStopTime();

	m_dwLastVictimSetTime = get_dword_time() - 3000;
	m_iMaxAggro = -100;

	m_bSendHorseLevel = 0;
	m_bSendHorseHealthGrade = 0;
	m_bSendHorseStaminaGrade = 0;

	m_dwLoginPlayTime = 0;

	m_pkChrMarried = NULL;

	m_posSafeboxOpen.x = -1000;
	m_posSafeboxOpen.y = -1000;

	// EQUIP_LAST_SKILL_DELAY
	m_dwLastSkillTime = get_dword_time();
	// END_OF_EQUIP_LAST_SKILL_DELAY

	// MOB_SKILL_COOLTIME
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
	// END_OF_MOB_SKILL_COOLTIME

	// ARENA
	m_pArena = NULL;
	m_nPotionLimit = quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count");
	// END_ARENA

	//PREVENT_TRADE_WINDOW
	m_isOpenSafebox = 0;
	//END_PREVENT_TRADE_WINDOW
	m_deposit_pulse = 0;

	m_strNewName = "";

	m_known_guild.clear();

	m_dwLogOffInterval = 0;
	m_dwMountTime = 0;

	m_bIsLoadedAffect = false;
	cannot_dead = false;

#ifdef __PET_SYSTEM__
	m_petSystem = 0;
	m_bIsPet = false;
#endif
#ifdef NEW_ICEDAMAGE_SYSTEM
	m_dwNDRFlag = 0;
	m_setNDAFlag.clear();
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	m_newpetSystem = 0;
	m_bIsNewPet = false;
#endif
	m_fAttMul = 1.0f;
	m_fDamMul = 1.0f;

	m_pointsInstant.iDragonSoulActiveDeck = -1;
#ifdef ENABLE_ANTI_CMD_FLOOD
	m_dwCmdAntiFloodCount = 0;
	m_dwCmdAntiFloodPulse = 0;
#endif
	memset(&m_tvLastSyncTime, 0, sizeof(m_tvLastSyncTime));
	m_iSyncHackCount = 0;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_bAcceCombination = false;
	m_bAcceAbsorption = false;
#endif
#ifdef ENABLE_AURA_SYSTEM
	m_bAuraRefine = false;
	m_bAuraAbsorption = false;
#endif
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	m_mountSystem = 0;
	m_bIsMount = false;
	m_bMountCounter = 0;
#endif
#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	dwLastTargetInfoPulse = 0;
#endif
#ifdef ENABLE_SORT_INVENTORIES
	m_sortLastUsed = 0;
#endif

#ifdef ENABLE_OFFLINE_SHOP
	m_pkOfflineShop = NULL;
	m_pkShopSafebox = NULL;
	m_pkOfflineShopGuest = NULL;
#endif
#ifdef ENABLE_LOADING_RENEWAL
	m_loading_state = 2;
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	m_biologsystem = nullptr;
	m_biolog_alert = nullptr;
	memset(&m_BiologInfo, 0, sizeof(m_BiologInfo));
#endif
#ifdef ENABLE_CHAR_SETTINGS
	memset(&m_char_settings, 0, sizeof(m_char_settings));
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	memset(&m_dungeonInfo, 0, sizeof(m_dungeonInfo));
#endif
	memset(&m_ActivateTime, 0, sizeof(m_ActivateTime));

#ifdef ENABLE_FISHING_RENEWAL
	m_bFishGameFishCounter = 0;
	m_fishEventTime = 0;
#endif

#ifdef ENABLE_LMW_PROTECTION
	m_attackHackControl = nullptr;
	m_mapAttackCount.clear();
	m_maxAttackCount = 0;
#ifdef ENABLE_BOT_CONTROL
	m_isBotControl = false;
#endif
#endif

#ifdef ENABLE_STOP_CHAT
	m_stopChatCooldown = 0;
#endif
#ifdef ENABLE_FARM_BLOCK
	m_farmBlock = true;
	m_farmBlockSetTime = 0;
	m_warpEvent = false;
#endif
#ifdef ENABLE_ALIGNMENT_SYSTEM
#ifndef DISABLE_ALIGNMENT_SYSTEM
	m_AlignmentLevel = 0;
	m_alignment_bonus.clear();
#endif
#endif
#ifdef ENABLE_SKILL_SET_BONUS
	m_SkillSetBonus = 0;
#endif

#ifdef ENABLE_ITEMSHOP
	m_protection_Time.clear();
#endif
	/* Potion Cooldown */
	m_potionUseTime = 0;
	/* Potion Cooldown */
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	memset(&m_dwSkillColor, 0, sizeof(m_dwSkillColor));
#endif
	m_campFireUseTime=0;//lightwork campfire spam fixed
#ifdef ENABLE_NAMING_SCROLL
	m_namingScroll[MOUNT_NAME_NUM] = "";
	m_namingScroll[PET_NAME_NUM] = "";
	m_namingScroll[BUFFI_NAME_NUM] = "";
#endif
#ifdef ENABLE_MINING_RENEWAL
	m_bMiningTotalPoints = 0;
	lodeNpc = nullptr;
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	m_listExtBattlePass.clear();
	m_bIsLoadedExtBattlePass = false;
	m_dwLastReciveExtBattlePassInfoTime = 0;
#endif
#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
	m_readBookCooldown = 0;
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	b67Attr = false;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	m_buffiSystem = nullptr;
	m_isBuffi = false;
	memset(&m_buffiItem, 0, sizeof(m_buffiItem));
#endif

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
	m_isPremium = false;
#endif
#ifdef ENABLE_AUTO_PICK_UP
	m_isAutoPickUp = false;
#endif
#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
	m_fastReadEvent = nullptr;
#endif
#ifdef ENABLE_BOOSTER_ITEM
	m_boosterSetBonus = false;
#endif
#ifdef ENABLE_MINI_GAME_BNW
	m_bMinigameBnwIsStarted = false;
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	m_vecCatchKingFieldCards.clear();
	bCatchKingHandCard = 0;
	bCatchKingHandCardLeft = 0;
	bCatchKingBetSetNumber = 0;
	dwCatchKingTotalScore = 0;
	dwCatchKingGameStatus = false;
#endif
#ifdef ENABLE_OFFICIAL_STUFF
	m_iCapeEffectPulse = 0;
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	m_language = 0;
#endif
	m_newCubeNpc = nullptr;
}

void CHARACTER::Create(const char* c_pszName, DWORD vid, bool isPC)
{
	static int s_crc = 172814;

	char crc_string[128 + 1];
	snprintf(crc_string, sizeof(crc_string), "%s%p%d", c_pszName, this, ++s_crc);
	m_vid = VID(vid, GetCRC32(crc_string, strlen(crc_string)));

	if (isPC)
		m_stName = c_pszName;
}

void CHARACTER::Destroy()
{
	CloseMyShop();

	if (m_pkRegen)
	{
		if (m_pkDungeon) 
		{
			// Dungeon regen may not be valid at this point
			if (m_pkDungeon->IsValidRegen(m_pkRegen, regen_id_)) {
				--m_pkRegen->count;
			}
		}
		else 
		{
			// Is this really safe? NO IT ISNT! F*CK THAT SH!T!
#ifdef ENABLE_RELOAD_REGEN
			if (is_valid_regen(m_pkRegen))
				--m_pkRegen->count;
#endif
		}
		m_pkRegen = NULL;
	}

	if (m_pkDungeon)
	{
		SetDungeon(NULL);
	}

#ifdef ENABLE_NEW_PET_SYSTEM
	if (m_newpetSystem)
	{
		m_newpetSystem->Save();
		delete m_newpetSystem;
		m_newpetSystem = nullptr;
	}
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (m_mountSystem)
	{
		m_mountSystem->Destroy();
		delete m_mountSystem;

		m_mountSystem = nullptr;
	}

	HorseSummon(false);
#endif

#ifdef __PET_SYSTEM__
	if (m_petSystem)
	{
		m_petSystem->Destroy();
		delete m_petSystem;
		m_petSystem = nullptr;
	}
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
	if (m_biologsystem)
	{
		m_biologsystem->Destroy();
		delete m_biologsystem;
		m_biologsystem = nullptr;
	}
	event_cancel(&m_biolog_alert);
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	if (m_buffiSystem)
	{
		if (m_buffiSystem->IsSummon())
			m_buffiSystem->UnSummon();
		m_buffiSystem->Destroy();
		delete m_buffiSystem;
		m_buffiSystem = nullptr;
	}
#endif

	if (GetRider())
		GetRider()->ClearHorseInfo();

	if (GetDesc())
	{
		GetDesc()->BindCharacter(NULL);
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();

	SetVictim(NULL);

	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

	ClearStone();
	ClearSync();
	ClearTarget();

	if (NULL == m_pkMobData)
	{
		DragonSoul_CleanUp();
		ClearItem();
	}

	// <Factor> m_pkParty becomes NULL after CParty destructor call!
	LPPARTY party = m_pkParty;
	if (party)
	{
		if (party->GetLeaderPID() == GetVID() && !IsPC())
		{
			M2_DELETE(party);
		}
		else
		{
			party->Unlink(this);

			if (!IsPC())
				party->Quit(GetVID());
		}

		SetParty(NULL);
	}

	if (m_pkMobInst)
	{
		M2_DELETE(m_pkMobInst);
		m_pkMobInst = NULL;
	}

	m_pkMobData = NULL;

	if (m_pkSafebox)
	{
		M2_DELETE(m_pkSafebox);
		m_pkSafebox = NULL;
	}

	if (m_pkMall)
	{
		M2_DELETE(m_pkMall);
		m_pkMall = NULL;
	}

	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		if (NULL != it->second)
		{
			M2_DELETE(it->second);
		}
	}
	m_map_buff_on_attrs.clear();

	m_set_pkChrSpawnedBy.clear();

	StopMuyeongEvent();
	event_cancel(&m_pkWarpNPCEvent);
	event_cancel(&m_pkRecoveryEvent);
	event_cancel(&m_pkDeadEvent);
	event_cancel(&m_pkSaveEvent);
	event_cancel(&m_pkTimedEvent);
	event_cancel(&m_pkStunEvent);
	event_cancel(&m_pkFishingEvent);
	event_cancel(&m_pkPoisonEvent);
#ifdef ENABLE_WOLFMAN_CHARACTER
	event_cancel(&m_pkBleedingEvent);
#endif
	event_cancel(&m_pkFireEvent);
	event_cancel(&m_pkPartyRequestEvent);
	//DELAYED_WARP
	//END_DELAYED_WARP
#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
	event_cancel(&m_fastReadEvent);
#endif
	// MINING
	event_cancel(&m_pkMiningEvent);
	// END_OF_MINING

	for (itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.begin(); it != m_mapMobSkillEvent.end(); ++it)
	{
		LPEVENT pkEvent = it->second;
		event_cancel(&pkEvent);
	}
	m_mapMobSkillEvent.clear();

	ClearAffect();

	event_cancel(&m_pkDestroyWhenIdleEvent);

	if (m_pSkillLevels)
	{
		M2_DELETE_ARRAY(m_pSkillLevels);
		m_pSkillLevels = NULL;
	}

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);
}

const char* CHARACTER::GetName() const
{
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	return m_stName.empty() ? (m_pkMobData ? LC_LOCALE_MOB_TEXT(GetRaceNum(), m_language) : "") : m_stName.c_str();
#else
	return m_stName.empty() ? (m_pkMobData ? m_pkMobData->m_table.szLocaleName : "") : m_stName.c_str();
#endif
}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
void CHARACTER::OpenMyShop(const char* c_pszSign, TShopItemTable* pTable, MAX_COUNT bItemCount)
#else
void CHARACTER::OpenMyShop(const char* c_pszSign, TShopItemTable* pTable, BYTE bItemCount)
#endif
{
	if (!CanHandleItem()) // @fixme149
	{
		NewChatPacket(YOU_CANNOT_DO_THIS_WHILE_ANOTHER_WINDOW_IS_OPEN);
		return;
	}

#ifndef ENABLE_OPEN_SHOP_WITH_ARMOR
	if (GetPart(PART_MAIN) > 2)
	{
		NewChatPacket(TAKE_OFF_YOUR_ARMOR_TO_OPEN_SHOP);
		return;
	}
#endif

	if (GetMyShop())
	{
		CloseMyShop();
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	if (pPC->IsRunning())
	{
		return;
	}

	if (bItemCount == 0)
	{
		return;
	}

	int64_t nTotalMoney = 0;

	for (int n = 0; n < bItemCount; ++n)
	{
		nTotalMoney += static_cast<int64_t>((pTable + n)->price);
	}

	nTotalMoney += static_cast<int64_t>(GetGold());

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] Overflow (GOLD_MAX) id %u name %s", GetPlayerID(), GetName());
		NewChatPacket(TAKE_OFF_YOUR_ARMOR_TO_OPEN_SHOP);
		return;
	}

	char szSign[SHOP_SIGN_MAX_LEN + 1];
	strlcpy(szSign, c_pszSign, sizeof(szSign));

	m_stShopSign = szSign;

	if (m_stShopSign.length() == 0)
	{
		return;
	}

	if (CBanwordManager::instance().CheckString(m_stShopSign.c_str(), m_stShopSign.length()))
	{
		NewChatPacket(INVALID_SHOP_NAME);
		return;
	}

	// MYSHOP_PRICE_LIST
	std::map<DWORD, DWORD> itemkind;
	// END_OF_MYSHOP_PRICE_LIST

	std::set<TItemPos> cont;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	for (MAX_COUNT i = 0; i < bItemCount; ++i)
#else
	for (BYTE i = 0; i < bItemCount; ++i)
#endif
	{
		if (cont.find((pTable + i)->pos) != cont.end())
		{
			sys_err("MYSHOP: duplicate shop item detected! (name: %s)", GetName());
			return;
		}

		// ANTI_GIVE, ANTI_MYSHOP check
		LPITEM pkItem = GetItem((pTable + i)->pos);

		if (pkItem)
		{
			const TItemTable* item_table = pkItem->GetProto();

			if (item_table && (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP)))
			{
				NewChatPacket(CANT_TRADE_WITH_THIS_ITEM);
				return;
			}

			if (pkItem->IsEquipped() == true)
			{
				NewChatPacket(CANT_TRADE_WITH_EQUIPPED_ITEMS);
				return;
			}

			if (true == pkItem->isLocked())
			{
				NewChatPacket(CANT_TRADE_WITH_THIS_ITEM);
				return;
			}

			// MYSHOP_PRICE_LIST
			itemkind[pkItem->GetVnum()] = (pTable + i)->price / pkItem->GetCount();
			// END_OF_MYSHOP_PRICE_LIST
		}

		cont.insert((pTable + i)->pos);
	}

	// MYSHOP_PRICE_LIST

	if (CountSpecifyItem(71049)) 
	{
		// @fixme403 BEGIN
		TItemPriceListTable header;
		memset(&header, 0, sizeof(TItemPriceListTable));

		header.dwOwnerID = GetPlayerID();
		header.byCount = itemkind.size();

		size_t idx = 0;
		for (itertype(itemkind) it = itemkind.begin(); it != itemkind.end(); ++it)
		{
			header.aPriceInfo[idx].dwVnum = it->first;
			header.aPriceInfo[idx].dwPrice = it->second;
			idx++;
		}

		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_UPDATE, GetDesc()->GetHandle(), &header, sizeof(TItemPriceListTable));
		// @fixme403 END
	}
	// END_OF_MYSHOP_PRICE_LIST
	else if (CountSpecifyItem(50200))
		RemoveSpecifyItem(50200, 1);
	else
		return;

	if (m_pkExchange)
		m_pkExchange->Cancel();

	TPacketGCShopSign p;

	p.bHeader = HEADER_GC_SHOP_SIGN;
	p.dwVID = GetVID();
	strlcpy(p.szSign, c_pszSign, sizeof(p.szSign));

	PacketAround(&p, sizeof(TPacketGCShopSign));

	m_pkMyShop = CShopManager::instance().CreatePCShop(this, pTable, bItemCount);

	if (IsPolymorphed() == true)
	{
		RemoveAffect(AFFECT_POLYMORPH);
	}

	if (GetHorse())
	{
		HorseSummon(false, true);
	}

	SetPolymorph(30000, true);
}

void CHARACTER::CloseMyShop()
{
	if (GetMyShop())
	{
		m_stShopSign.clear();
		CShopManager::instance().DestroyPCShop(this);
		m_pkMyShop = NULL;

		TPacketGCShopSign p;

		p.bHeader = HEADER_GC_SHOP_SIGN;
		p.dwVID = GetVID();
		p.szSign[0] = '\0';

		PacketAround(&p, sizeof(p));
#ifdef ENABLE_WOLFMAN_CHARACTER
		SetPolymorph(m_points.job, true);
		// SetPolymorph(0, true);
#else
		SetPolymorph(GetJob(), true);
#endif
	}
}

void EncodeMovePacket(TPacketGCMove& pack, DWORD dwVID, BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, BYTE bRot)
{
	pack.bHeader = HEADER_GC_MOVE;
	pack.bFunc = bFunc;
	pack.bArg = bArg;
	pack.dwVID = dwVID;
	pack.dwTime = dwTime ? dwTime : get_dword_time();
	pack.bRot = bRot;
	pack.lX = x;
	pack.lY = y;
	pack.dwDuration = dwDuration;
}

void CHARACTER::RestartAtSamePos()
{
	if (m_bIsObserver)
	{
		return;
	}

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		if (!m_bIsObserver)
			EncodeInsertPacket(entity);

		if (entity->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER lpChar = (LPCHARACTER)entity;
			if (lpChar->IsPC() || lpChar->IsNPC() || lpChar->IsMonster())
			{
				if (!entity->IsObserverMode())
					entity->EncodeInsertPacket(this);
			}
		}
		else
		{
			if (!entity->IsObserverMode())
			{
				entity->EncodeInsertPacket(this);
			}
		}
	}
}

// #define ENABLE_SHOWNPCLEVEL
void CHARACTER::EncodeInsertPacket(LPENTITY entity)
{
	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	LPCHARACTER ch = (LPCHARACTER)entity;
	ch->SendGuildName(GetGuild());
	if (!ch)
		return;

	TPacketGCCharacterAdd pack;

	pack.header = HEADER_GC_CHARACTER_ADD;
	pack.dwVID = m_vid;
	pack.bType = GetCharType();
	pack.angle = GetRotation();
	pack.x = GetX();
	pack.y = GetY();
	pack.z = GetZ();
	pack.wRaceNum = GetRaceNum();

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
	{
		pack.bMovingSpeed = 200;
	}
	else
	{
		pack.bMovingSpeed = GetLimitPoint(POINT_MOV_SPEED);
	}
	pack.bAttackSpeed = GetLimitPoint(POINT_ATT_SPEED);
	pack.dwAffectFlag[0] = m_afAffectFlag.bits[0];
	pack.dwAffectFlag[1] = m_afAffectFlag.bits[1];

	pack.bStateFlag = m_bAddChrState;

	int iDur = 0;

	if (m_posDest.x != pack.x || m_posDest.y != pack.y)
	{
		iDur = (m_dwMoveStartTime + m_dwMoveDuration) - get_dword_time();

		if (iDur <= 0)
		{
			pack.x = m_posDest.x;
			pack.y = m_posDest.y;
		}
	}

	d->Packet(&pack, sizeof(pack));

	if (IsPC() == true || m_bCharType == CHAR_TYPE_NPC)
	{
		TPacketGCCharacterAdditionalInfo addPacket;
		memset(&addPacket, 0, sizeof(TPacketGCCharacterAdditionalInfo));

		addPacket.header = HEADER_GC_CHAR_ADDITIONAL_INFO;
		addPacket.dwVID = m_vid;

		addPacket.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
		addPacket.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		addPacket.awPart[CHR_EQUIPPART_ACCE] = GetPart(PART_ACCE);
#endif
		addPacket.awPart[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);

		addPacket.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
#ifdef ENABLE_AURA_SYSTEM
		addPacket.awPart[CHR_EQUIPPART_AURA] = GetPart(PART_AURA);
#endif
#ifdef ENABLE_CROWN_SYSTEM
		addPacket.awPart[CHR_EQUIPPART_CROWN] = GetPart(PART_CROWN);
#endif
#ifdef ENABLE_ITEMS_SHINING
		addPacket.adwShining[CHR_SHINING_WRIST_LEFT] = GetWear(WEAR_SHINING_WRIST_LEFT) ? GetWear(WEAR_SHINING_WRIST_LEFT)->GetVnum() : 0;
		addPacket.adwShining[CHR_SHINING_WRIST_RIGHT] = GetWear(WEAR_SHINING_WRIST_RIGHT) ? GetWear(WEAR_SHINING_WRIST_RIGHT)->GetVnum() : 0;
		addPacket.adwShining[CHR_SHINING_ARMOR] = GetWear(WEAR_SHINING_ARMOR) ? GetWear(WEAR_SHINING_ARMOR)->GetVnum() : 0;
#endif

		addPacket.bPKMode = m_bPKMode;
		addPacket.dwMountVnum = GetMountVnum();
		addPacket.bEmpire = m_bEmpire;
#ifdef ENABLE_SKILL_COLOR_SYSTEM
		memcpy(addPacket.dwSkillColor, GetSkillColor(), sizeof(addPacket.dwSkillColor));
#endif
		strlcpy(addPacket.name, GetName(), sizeof(addPacket.name));

		if (IsPC() == true
#ifdef ENABLE_NEW_PET_SYSTEM
			|| IsNewPet() == true
#endif
			)
		{
			addPacket.dwLevel = GetLevel();
		}
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		addPacket.bLanguage = IsPC() ? GetLanguage() : 0;
#endif
		if (GetGuild())
		{
			addPacket.dwGuildID = GetGuild()->GetID();
		}
		addPacket.sAlignment = m_iAlignment / 10;

	
#ifdef ENABLE_BUFFI_SYSTEM
		if (IsBuffi())
		{
			addPacket.awPart[CHR_EQUIPPART_ARMOR] = GetBuffiItem(BUFFI_BODY_SLOT);
			addPacket.awPart[CHR_EQUIPPART_WEAPON] = GetBuffiItem(BUFFI_WEAPON_SLOT);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			addPacket.awPart[CHR_EQUIPPART_ACCE] = GetBuffiItem(BUFFI_ACCE_SLOT);
#endif
			addPacket.awPart[CHR_EQUIPPART_HAIR] = GetBuffiItem(BUFFI_HAIR_SLOT);
#ifdef ENABLE_AURA_SYSTEM
			addPacket.awPart[CHR_EQUIPPART_AURA] = GetBuffiItem(BUFFI_AURA_SLOT);
#endif
		}
#endif

		d->Packet(&addPacket, sizeof(TPacketGCCharacterAdditionalInfo));
	}

	if (iDur)
	{
		TPacketGCMove pack;
		EncodeMovePacket(pack, GetVID(), FUNC_MOVE, 0, m_posDest.x, m_posDest.y, iDur, 0, (BYTE)(GetRotation() / 5));
		d->Packet(&pack, sizeof(pack));

		TPacketGCWalkMode p;
		p.vid = GetVID();
		p.header = HEADER_GC_WALK_MODE;
		p.mode = m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;

		d->Packet(&p, sizeof(p));
	}

	if (entity->IsType(ENTITY_CHARACTER) && GetDesc())
	{
		LPCHARACTER ch = (LPCHARACTER)entity;
		if (ch->IsWalking())
		{
			TPacketGCWalkMode p;
			p.vid = ch->GetVID();
			p.header = HEADER_GC_WALK_MODE;
			p.mode = ch->m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;
			GetDesc()->Packet(&p, sizeof(p));
		}
	}

	if (GetMyShop())
	{
		TPacketGCShopSign p;

		p.bHeader = HEADER_GC_SHOP_SIGN;
		p.dwVID = GetVID();
		strlcpy(p.szSign, m_stShopSign.c_str(), sizeof(p.szSign));

		d->Packet(&p, sizeof(TPacketGCShopSign));
	}
}

void CHARACTER::EncodeRemovePacket(LPENTITY entity)
{
	if (entity->GetType() != ENTITY_CHARACTER)
		return;

	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	TPacketGCCharacterDelete pack;

	pack.header = HEADER_GC_CHARACTER_DEL;
	pack.id = m_vid;

	d->Packet(&pack, sizeof(TPacketGCCharacterDelete));
}

void CHARACTER::UpdatePacket()
{
	if (IsPC() && (!GetDesc() || !GetDesc()->GetCharacter()))
		return;


#ifdef ENABLE_LOADING_RENEWAL
	if (IsPC() && GetLoadingState() == 1)
		return;
#endif

	if (GetSectree() == NULL) return;

	TPacketGCCharacterUpdate pack;
	pack.header = HEADER_GC_CHARACTER_UPDATE;
	pack.dwVID = m_vid;

#ifdef ENABLE_BUFFI_SYSTEM
	if (IsBuffi())
	{
		pack.awPart[CHR_EQUIPPART_ARMOR] = GetBuffiItem(BUFFI_BODY_SLOT);
		pack.awPart[CHR_EQUIPPART_WEAPON] = GetBuffiItem(BUFFI_WEAPON_SLOT);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		pack.awPart[CHR_EQUIPPART_ACCE] = GetBuffiItem(BUFFI_ACCE_SLOT);
#endif
		pack.awPart[CHR_EQUIPPART_HAIR] = GetBuffiItem(BUFFI_HAIR_SLOT);
#ifdef ENABLE_AURA_SYSTEM
		pack.awPart[CHR_EQUIPPART_AURA] = GetBuffiItem(BUFFI_AURA_SLOT);
#endif
	}
	else
	{
		pack.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
		pack.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		pack.awPart[CHR_EQUIPPART_ACCE] = GetPart(PART_ACCE);
#endif
		pack.awPart[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);
#ifdef ENABLE_AURA_SYSTEM
		pack.awPart[CHR_EQUIPPART_AURA] = GetPart(PART_AURA);
#endif
	}
#else
	pack.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
	pack.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
	pack.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	pack.awPart[CHR_EQUIPPART_ACCE] = GetPart(PART_ACCE);
#endif
#ifdef ENABLE_AURA_SYSTEM
	pack.awPart[CHR_EQUIPPART_AURA] = GetPart(PART_AURA);
#endif
#endif
	pack.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);

#ifdef ENABLE_CROWN_SYSTEM
	pack.awPart[CHR_EQUIPPART_CROWN] = GetPart(PART_CROWN);
#endif
#ifdef ENABLE_ITEMS_SHINING
	pack.adwShining[CHR_SHINING_WRIST_LEFT] = GetWear(WEAR_SHINING_WRIST_LEFT) ? GetWear(WEAR_SHINING_WRIST_LEFT)->GetVnum() : 0;
	pack.adwShining[CHR_SHINING_WRIST_RIGHT] = GetWear(WEAR_SHINING_WRIST_RIGHT) ? GetWear(WEAR_SHINING_WRIST_RIGHT)->GetVnum() : 0;
	pack.adwShining[CHR_SHINING_ARMOR] = GetWear(WEAR_SHINING_ARMOR) ? GetWear(WEAR_SHINING_ARMOR)->GetVnum() : 0;
#endif
	pack.bMovingSpeed = GetLimitPoint(POINT_MOV_SPEED);
	pack.bAttackSpeed = GetLimitPoint(POINT_ATT_SPEED);
	pack.bStateFlag = m_bAddChrState;
	pack.dwAffectFlag[0] = m_afAffectFlag.bits[0];
	pack.dwAffectFlag[1] = m_afAffectFlag.bits[1];
	pack.dwGuildID = 0;
	pack.sAlignment = m_iAlignment / 10;
	pack.bPKMode = m_bPKMode;

	pack.dwGuildID = GetGuild() ? GetGuild()->GetID() : 0;
	pack.dwMountVnum = GetMountVnum();
#ifdef ENABLE_NEW_PET_SYSTEM
	pack.dwLevel = GetLevel();
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	memcpy(pack.dwSkillColor, GetSkillColor(), sizeof(pack.dwSkillColor));
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	pack.bLanguage = IsPC() ? GetLanguage() : 0;
#endif
	PacketAround(&pack, sizeof(pack));
}

LPCHARACTER CHARACTER::FindCharacterInView(const char* c_pszName, bool bFindPCOnly)
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	for (; it != m_map_view.end(); ++it)
	{
		if (!it->first->IsType(ENTITY_CHARACTER))
			continue;

		LPCHARACTER tch = (LPCHARACTER)it->first;

		if (bFindPCOnly && tch->IsNPC())
			continue;

		if (!strcasecmp(tch->GetName(), c_pszName))
			return (tch);
	}

	return NULL;
}

void CHARACTER::SetPosition(int pos)
{
	if (pos == POS_STANDING)
	{
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

		event_cancel(&m_pkDeadEvent);
		event_cancel(&m_pkStunEvent);
	}
	else if (pos == POS_DEAD)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);

	if (!IsStone())
	{
		switch (pos)
		{
		case POS_FIGHTING:
			GotoState(m_stateBattle);
			break;

		default:
			GotoState(m_stateIdle);
			break;
		}
	}

	m_pointsInstant.position = pos;
}

void CHARACTER::Save()
{
	if (!m_bSkipSave)
		CHARACTER_MANAGER::instance().DelayedSave(this);
}

void CHARACTER::CreatePlayerProto(TPlayerTable& tab)
{
	memset(&tab, 0, sizeof(TPlayerTable));

	if (GetNewName().empty())
	{
		strlcpy(tab.name, GetName(), sizeof(tab.name));
	}
	else
	{
		strlcpy(tab.name, GetNewName().c_str(), sizeof(tab.name));
	}

	strlcpy(tab.ip, GetDesc()->GetHostName(), sizeof(tab.ip));

	tab.id = m_dwPlayerID;
	tab.voice = GetPoint(POINT_VOICE);
	tab.level = GetLevel();
	tab.level_step = GetPoint(POINT_LEVEL_STEP);
	tab.exp = GetExp();
	tab.gold = GetGold();
	tab.job = m_points.job;
	tab.part_base = m_pointsInstant.bBasePart;
	tab.skill_group = m_points.skill_group;

	DWORD dwPlayedTime = (get_dword_time() - m_dwPlayStartTime);

	if (dwPlayedTime > 60000)
	{
		SetRealPoint(POINT_PLAYTIME, GetRealPoint(POINT_PLAYTIME) + dwPlayedTime / 60000);
		ResetPlayTime(dwPlayedTime % 60000);
	}

	tab.playtime = GetRealPoint(POINT_PLAYTIME);
	tab.lAlignment = m_iRealAlignment;

	if (m_posWarp.x != 0 || m_posWarp.y != 0)
	{
		tab.x = m_posWarp.x;
		tab.y = m_posWarp.y;
		tab.z = 0;
		tab.lMapIndex = m_lWarpMapIndex;
	}
	else
	{
		tab.x = GetX();
		tab.y = GetY();
		tab.z = GetZ();
		tab.lMapIndex = GetMapIndex();
	}

	if (m_lExitMapIndex == 0)
	{
		tab.lExitMapIndex = tab.lMapIndex;
		tab.lExitX = tab.x;
		tab.lExitY = tab.y;
	}
	else
	{
		tab.lExitMapIndex = m_lExitMapIndex;
		tab.lExitX = m_posExit.x;
		tab.lExitY = m_posExit.y;
	}

	tab.st = GetRealPoint(POINT_ST);
	tab.ht = GetRealPoint(POINT_HT);
	tab.dx = GetRealPoint(POINT_DX);
	tab.iq = GetRealPoint(POINT_IQ);

	tab.stat_point = GetPoint(POINT_STAT);
	tab.skill_point = GetPoint(POINT_SKILL);
	tab.sub_skill_point = GetPoint(POINT_SUB_SKILL);
	tab.horse_skill_point = GetPoint(POINT_HORSE_SKILL);

	tab.stat_reset_count = GetPoint(POINT_STAT_RESET_COUNT);

	tab.hp = GetHP();
	tab.sp = GetSP();

	tab.stamina = GetStamina();

	tab.sRandomHP = m_points.iRandomHP;
	tab.sRandomSP = m_points.iRandomSP;

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
		tab.quickslot[i] = m_quickslot[i];

	thecore_memcpy(tab.parts, m_pointsInstant.parts, sizeof(tab.parts));

	// REMOVE_REAL_SKILL_LEVLES
	thecore_memcpy(tab.skills, m_pSkillLevels, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	// END_OF_REMOVE_REAL_SKILL_LEVLES

	tab.horse = GetHorseData();
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	thecore_memcpy(&tab.player_stats, &m_points.player_stats, sizeof(tab.player_stats));
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	thecore_memcpy(&tab.biolog, &m_BiologInfo, sizeof(tab.biolog));
#endif
#ifdef ENABLE_CHAR_SETTINGS
	thecore_memcpy(&tab.char_settings, &m_char_settings, sizeof(tab.char_settings));
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	memcpy(&tab.dungeonInfo, &m_dungeonInfo, sizeof(tab.dungeonInfo));
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	tab.battle_pass_premium_id = GetExtBattlePassPremiumID();
#endif
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	tab.chatColor = GetChatColor();
#endif
}

void CHARACTER::SaveReal()
{
	if (m_bSkipSave)
		return;

	if (!GetDesc())
	{
		sys_err("Character::Save : no descriptor when saving (name: %s)", GetName());
		return;
	}

	TPlayerTable table;
	CreatePlayerProto(table);

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_SAVE, GetDesc()->GetHandle(), &table, sizeof(TPlayerTable));

	quest::PC* pkQuestPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	if (!pkQuestPC)
		sys_err("CHARACTER::Save : null quest::PC pointer! (name %s)", GetName());
	else
	{
		pkQuestPC->Save();
	}

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
	if (pMarriage)
		pMarriage->Save();
}

void CHARACTER::FlushDelayedSaveItem()
{
	LPITEM item;

	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
		if ((item = GetInventoryItem(i)))
			ITEM_MANAGER::instance().FlushDelayedSave(item);
}

void CHARACTER::Disconnect(const char* c_pszReason)
{
	assert(GetDesc() != NULL);
	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

	if (GetArena() != NULL)
	{
		GetArena()->OnDisconnect(GetPlayerID());
	}

	if (GetParty() != NULL)
	{
		GetParty()->UpdateOfflineState(GetPlayerID());
	}

	marriage::CManager::instance().Logout(this);

	// P2P Logout
	TPacketGGLogout p;
	p.bHeader = HEADER_GG_LOGOUT;
	strlcpy(p.szName, GetName(), sizeof(p.szName));
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogout));

	if (m_pWarMap)
		SetWarMap(NULL);

	if (m_pWeddingMap)
	{
		SetWeddingMap(NULL);
	}

	if (GetGuild())
		GetGuild()->LogoutMember(this);

	quest::CQuestManager::instance().LogoutPC(this);

	if (GetParty())
		GetParty()->Unlink(this);

	if (IsStun() || IsDead())
	{
		DeathPenalty(0);
		PointChange(POINT_HP, 50 - GetHP());
	}

	if (!CHARACTER_MANAGER::instance().FlushDelayedSave(this))
	{
		SaveReal();
	}

#ifdef ENABLE_OFFLINE_SHOP
	offlineshop::GetManager().RemoveSafeboxFromCache(GetPlayerID());
	offlineshop::GetManager().RemoveGuestFromShops(this);

	if (GetOfflineShop())
	{
		SetOfflineShop(NULL);
	}

	SetShopSafebox(NULL);
#endif

	FlushDelayedSaveItem();

	SaveAffect();
	m_bIsLoadedAffect = false;

	m_bSkipSave = true;

	quest::CQuestManager::instance().DisconnectPC(this);

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	ListExtBattlePassMap::iterator itext = m_listExtBattlePass.begin();
	while (itext != m_listExtBattlePass.end())
	{
		TPlayerExtBattlePassMission* pkMission = *itext++;

		if (!pkMission->bIsUpdated)
			continue;

		db_clientdesc->DBPacket(HEADER_GD_SAVE_EXT_BATTLE_PASS, 0, pkMission, sizeof(TPlayerExtBattlePassMission));
	}
	m_bIsLoadedExtBattlePass = false;
#endif

	CloseSafebox();

	CloseMall();

	CPVPManager::instance().Disconnect(this);

	CTargetManager::instance().Logout(GetPlayerID());

	MessengerManager::instance().Logout(GetName());
	
#ifdef OX_EVENT_DOUBLE_CONNECT_DISABLE
	if (GetMapIndex() == OXEVENT_MAP_INDEX)
		COXEventManager::Instance().RemoveFromAttenderList(GetPlayerID());
#endif

	if (GetDesc())
	{
#ifdef ENABLE_FARM_BLOCK
		if (GetDesc()->GetAccountTable().descHwidID)
			CHwidManager::instance().FarmBlockLogout(GetDesc()->GetAccountTable().descHwidID, GetPlayerID(), IsWarp());
#endif
		GetDesc()->BindCharacter(NULL);
	}
	else
	{
		sys_err("desc tinne");
	}

	M2_DESTROY_CHARACTER(this);
}

bool CHARACTER::Show(long lMapIndex, long x, long y, long z, bool bShowSpawnMotion/* = false */)
{
	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		return false;
	}

	SetMapIndex(lMapIndex);

	bool bChangeTree = false;

	if (!GetSectree() || GetSectree() != sectree)
		bChangeTree = true;

	if (bChangeTree)
	{
		if (GetSectree())
			GetSectree()->RemoveEntity(this);

		ViewCleanup();
	}

	if (!IsNPC())
	{
		if (GetStamina() < GetMaxStamina())
			StartAffectEvent();
	}
	else if (m_pkMobData)
	{
		m_pkMobInst->m_posLastAttacked.x = x;
		m_pkMobInst->m_posLastAttacked.y = y;
		m_pkMobInst->m_posLastAttacked.z = z;
	}

	if (bShowSpawnMotion)
	{
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
		m_afAffectFlag.Set(AFF_SPAWN);
	}

	SetXYZ(x, y, z);

	m_posDest.x = x;
	m_posDest.y = y;
	m_posDest.z = z;

	m_posStart.x = x;
	m_posStart.y = y;
	m_posStart.z = z;

	if (bChangeTree)
	{
		EncodeInsertPacket(this);
		sectree->InsertEntity(this);

		UpdateSectree();
	}
	else
	{
		ViewReencode();
	}

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
	return true;
}


void CHARACTER::MainCharacterPacket()
{
	TPacketGCMainCharacter pack;

	pack.header = HEADER_GC_MAIN_CHARACTER;
	pack.dwVID = m_vid;
	pack.wRaceNum = GetRaceNum();
	pack.lx = GetX();
	pack.ly = GetY();
	pack.lz = GetZ();
	pack.empire = GetDesc()->GetEmpire();
	pack.skill_group = GetSkillGroup();
	strlcpy(pack.szName, GetName(), sizeof(pack.szName));

	GetDesc()->Packet(&pack, sizeof(TPacketGCMainCharacter));
}

void CHARACTER::PointsPacket()
{
	if (!GetDesc())
		return;

	TPacketGCPoints pack;

	pack.header = HEADER_GC_CHARACTER_POINTS;

	memset(&pack.points, 0, sizeof(pack.points));

	pack.points[POINT_LEVEL] = GetLevel();
	pack.points[POINT_EXP] = GetExp();
	pack.points[POINT_NEXT_EXP] = GetNextExp();
	pack.points[POINT_HP] = GetHP();
	pack.points[POINT_MAX_HP] = GetMaxHP();
	pack.points[POINT_SP] = GetSP();
	pack.points[POINT_MAX_SP] = GetMaxSP();
	pack.points[POINT_GOLD] = GetGold();
	pack.points[POINT_STAMINA] = GetStamina();
	pack.points[POINT_MAX_STAMINA] = GetMaxStamina();

	for (int i = POINT_ST; i < POINT_REFRESH_MAX; ++i)
		pack.points[i] = GetPoint(i);

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	pack.points[POINT_BATTLE_PASS_PREMIUM_ID] = GetExtBattlePassPremiumID();
#endif
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	for (int i = 0; i < STATS_MAX; i++)
	{
		pack.points[i + POINT_POWER_RANK] = static_cast<long long>(GetStats(i));
	}
#endif

	GetDesc()->Packet(&pack, sizeof(TPacketGCPoints));
}

bool CHARACTER::ChangeSex()
{
	int src_race = GetRaceNum();

	switch (src_race)
	{
	case MAIN_RACE_WARRIOR_M:
		m_points.job = MAIN_RACE_WARRIOR_W;
		break;

	case MAIN_RACE_WARRIOR_W:
		m_points.job = MAIN_RACE_WARRIOR_M;
		break;

	case MAIN_RACE_ASSASSIN_M:
		m_points.job = MAIN_RACE_ASSASSIN_W;
		break;

	case MAIN_RACE_ASSASSIN_W:
		m_points.job = MAIN_RACE_ASSASSIN_M;
		break;

	case MAIN_RACE_SURA_M:
		m_points.job = MAIN_RACE_SURA_W;
		break;

	case MAIN_RACE_SURA_W:
		m_points.job = MAIN_RACE_SURA_M;
		break;

	case MAIN_RACE_SHAMAN_M:
		m_points.job = MAIN_RACE_SHAMAN_W;
		break;

	case MAIN_RACE_SHAMAN_W:
		m_points.job = MAIN_RACE_SHAMAN_M;
		break;
#ifdef ENABLE_WOLFMAN_CHARACTER
	case MAIN_RACE_WOLFMAN_M:
		m_points.job = MAIN_RACE_WOLFMAN_M;
		break;
#endif
	default:
		sys_err("CHANGE_SEX: %s unknown race %d", GetName(), src_race);
		return false;
	}

	return true;
}

WORD CHARACTER::GetRaceNum() const
{
	if (m_dwPolymorphRace)
		return m_dwPolymorphRace;

	if (m_pkMobData)
		return m_pkMobData->m_table.dwVnum;

	return m_points.job;
}

void CHARACTER::SetRace(BYTE race)
{
	if (race >= MAIN_RACE_MAX_NUM)
	{
		sys_err("CHARACTER::SetRace(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
		return;
	}

	m_points.job = race;
}

BYTE CHARACTER::GetJob() const
{
	unsigned race = m_points.job;
	unsigned job;

	if (RaceToJob(race, &job))
		return job;

	sys_err("CHARACTER::GetJob(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
	return JOB_WARRIOR;
}

void CHARACTER::SetLevel(BYTE level)
{
	m_points.level = level;

	if (IsPC())
	{
		if (level < PK_PROTECT_LEVEL)
			SetPKMode(PK_MODE_PROTECT);
		else if (GetGMLevel() != GM_PLAYER)
			SetPKMode(PK_MODE_PROTECT);
		else if (m_bPKMode == PK_MODE_PROTECT)
			SetPKMode(PK_MODE_PEACE);
	}
}

void CHARACTER::SetEmpire(BYTE bEmpire)
{
	m_bEmpire = bEmpire;
}

void CHARACTER::SetPlayerProto(const TPlayerTable* t)
{
	if (!GetDesc() || !*GetDesc()->GetHostName())
		sys_err("cannot get desc or hostname");
	else
		SetGMLevel();

	m_bCharType = CHAR_TYPE_PC;

	m_dwPlayerID = t->id;

	m_iAlignment = t->lAlignment;
	m_iRealAlignment = t->lAlignment;

	m_points.voice = t->voice;

	m_points.skill_group = t->skill_group;

	m_pointsInstant.bBasePart = t->part_base;
	SetPart(PART_HAIR, t->parts[PART_HAIR]);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	SetPart(PART_ACCE, t->parts[PART_ACCE]);
#endif
#ifdef ENABLE_AURA_SYSTEM
	SetPart(PART_AURA, t->parts[PART_AURA]);
#endif
#ifdef ENABLE_CROWN_SYSTEM
	SetPart(PART_CROWN, t->parts[PART_CROWN]);
#endif

	m_points.iRandomHP = t->sRandomHP;
	m_points.iRandomSP = t->sRandomSP;

	// REMOVE_REAL_SKILL_LEVLES
	if (m_pSkillLevels)
		M2_DELETE_ARRAY(m_pSkillLevels);

	m_pSkillLevels = M2_NEW TPlayerSkill[SKILL_MAX_NUM];
	thecore_memcpy(m_pSkillLevels, t->skills, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	// END_OF_REMOVE_REAL_SKILL_LEVLES

	if (t->lMapIndex >= 10000)
	{
		m_posWarp.x = t->lExitX;
		m_posWarp.y = t->lExitY;
		m_lWarpMapIndex = t->lExitMapIndex;
	}

	SetRealPoint(POINT_PLAYTIME, t->playtime);
	m_dwLoginPlayTime = t->playtime;
	SetRealPoint(POINT_ST, t->st);
	SetRealPoint(POINT_HT, t->ht);
	SetRealPoint(POINT_DX, t->dx);
	SetRealPoint(POINT_IQ, t->iq);

	SetPoint(POINT_ST, t->st);
	SetPoint(POINT_HT, t->ht);
	SetPoint(POINT_DX, t->dx);
	SetPoint(POINT_IQ, t->iq);

	SetPoint(POINT_STAT, t->stat_point);
	SetPoint(POINT_SKILL, t->skill_point);
	SetPoint(POINT_SUB_SKILL, t->sub_skill_point);
	SetPoint(POINT_HORSE_SKILL, t->horse_skill_point);

	SetPoint(POINT_STAT_RESET_COUNT, t->stat_reset_count);

	SetPoint(POINT_LEVEL_STEP, t->level_step);
	SetRealPoint(POINT_LEVEL_STEP, t->level_step);

#ifdef ENABLE_PLAYER_STATS_SYSTEM
	memcpy(&m_points.player_stats, &t->player_stats, sizeof(t->player_stats));
#endif

	SetRace(t->job);

	SetLevel(t->level);
	SetExp(t->exp);
	SetGold(t->gold);
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	SetChatColor(t->chatColor);
#endif
#ifdef ENABLE_ALIGNMENT_SYSTEM
#ifndef DISABLE_ALIGNMENT_SYSTEM
	AlignmentLevel(m_iRealAlignment, false);
#endif
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	SetExtBattlePassPremiumID(t->battle_pass_premium_id);
#endif
	SetMapIndex(t->lMapIndex);
	SetXYZ(t->x, t->y, t->z);

#ifdef ENABLE_CHAR_SETTINGS
	SetCharSettings(t->char_settings);
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	memcpy(&m_dungeonInfo, &t->dungeonInfo, sizeof(m_dungeonInfo));
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	SetBiologInfo(t->biolog);
	if (m_biologsystem)
	{
		m_biologsystem->Destroy();
		delete m_biologsystem;
		sys_err("biolog system not delete %s", GetName());
	}
	m_biologsystem = new BiologSystem(this);
#endif
#ifdef ENABLE_SKILL_SET_BONUS
	SkillSetBonusCalcute(false);
#endif
	ComputePoints();

	SetHP(t->hp);
	SetSP(t->sp);
	SetStamina(t->stamina);


	if (GetGMLevel() > GM_LOW_WIZARD)
	{
		m_afAffectFlag.Set(AFF_YMIR);
		m_bPKMode = PK_MODE_PROTECT;
	}

	if (GetLevel() < PK_PROTECT_LEVEL)
		m_bPKMode = PK_MODE_PROTECT;

	SetHorseData(t->horse);

	if (GetHorseLevel() > 0)
		UpdateHorseDataByLogoff(t->logoff_interval);

	thecore_memcpy(m_aiPremiumTimes, t->aiPremiumTimes, sizeof(t->aiPremiumTimes));

	m_dwLogOffInterval = t->logoff_interval;

#ifdef __PET_SYSTEM__
	if (m_petSystem)
	{
		m_petSystem->Destroy();
		delete m_petSystem;
	}

	m_petSystem = M2_NEW CPetSystem(this);
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (m_mountSystem)
	{
		m_mountSystem->Destroy();
		delete m_mountSystem;
	}

	m_mountSystem = M2_NEW CMountSystem(this);
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
	if (m_newpetSystem)
	{
		m_newpetSystem->Destroy();
		delete m_newpetSystem;
	}

	m_newpetSystem = M2_NEW CNewPet(this);
#endif

#ifdef ENABLE_BUFFI_SYSTEM
	if (m_buffiSystem)
	{
		delete m_buffiSystem;
		sys_err("buffi system not delete %s ", GetName());
	}
	m_buffiSystem = new CBuffiSystem(this);
#endif
}

EVENTFUNC(kill_ore_load_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("kill_ore_load_even> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) 
	{ // <Factor>
		return 0;
	}

	ch->m_pkMiningEvent = nullptr;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::SetProto(const CMob* pkMob)
{
	if (m_pkMobInst)
		M2_DELETE(m_pkMobInst);

	m_pkMobData = pkMob;
	m_pkMobInst = M2_NEW CMobInstance;

	m_bPKMode = PK_MODE_FREE;

	const TMobTable* t = &m_pkMobData->m_table;

	m_bCharType = t->bType;

	SetLevel(t->bLevel);
	SetEmpire(t->bEmpire);

	SetExp(t->dwExp);
	SetRealPoint(POINT_ST, t->bStr);
	SetRealPoint(POINT_DX, t->bDex);
	SetRealPoint(POINT_HT, t->bCon);
	SetRealPoint(POINT_IQ, t->bInt);

	ComputePoints();

	SetHP(GetMaxHP());
	SetSP(GetMaxSP());

	////////////////////
	m_pointsInstant.dwAIFlag = t->dwAIFlag;
	SetImmuneFlag(t->dwImmuneFlag);

	AssignTriggers(t);

	ApplyMobAttribute(t);

	if (IsStone())
	{
		DetermineDropMetinStone();
	}

	if (IsWarp() || IsGoto())
	{
		StartWarpNPCEvent();
	}

	CHARACTER_MANAGER::instance().RegisterRaceNumMap(this);

	// XXX CTF GuildWar hardcoding
	if (warmap::IsWarFlag(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
	}

	if (warmap::IsWarFlagBase(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
	}

	if (m_bCharType == CHAR_TYPE_HORSE ||
		GetRaceNum() == 20101 ||
		GetRaceNum() == 20102 ||
		GetRaceNum() == 20103 ||
		GetRaceNum() == 20104 ||
		GetRaceNum() == 20105 ||
		GetRaceNum() == 20106 ||
		GetRaceNum() == 20107 ||
		GetRaceNum() == 20108 ||
		GetRaceNum() == 20109
		)
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
	}

	// MINING
	if (mining::IsVeinOfOre(GetRaceNum()))
	{
		char_event_info* info = AllocEventInfo<char_event_info>();

		info->ch = this;

		m_pkMiningEvent = event_create(kill_ore_load_event, info, PASSES_PER_SEC(number(7 * 60, 15 * 60)));
	}
	// END_OF_MINING
}

const TMobTable& CHARACTER::GetMobTable() const
{
	return m_pkMobData->m_table;
}

bool CHARACTER::IsRaceFlag(DWORD dwBit) const
{
	return m_pkMobData ? IS_SET(m_pkMobData->m_table.dwRaceFlag, dwBit) : 0;
}

DWORD CHARACTER::GetMobDamageMin() const
{
	return m_pkMobData->m_table.dwDamageRange[0];
}

DWORD CHARACTER::GetMobDamageMax() const
{
	return m_pkMobData->m_table.dwDamageRange[1];
}

float CHARACTER::GetMobDamageMultiply() const
{
	float fDamMultiply = GetMobTable().fDamMultiply;

	if (IsBerserk())
		fDamMultiply = fDamMultiply * 2.0f;

	return fDamMultiply;
}

DWORD CHARACTER::GetMobDropItemVnum() const
{
	return m_pkMobData->m_table.dwDropItemVnum;
}

bool CHARACTER::IsSummonMonster() const
{
	return GetSummonVnum() != 0;
}

DWORD CHARACTER::GetSummonVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwSummonVnum : 0;
}

DWORD CHARACTER::GetPolymorphItemVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwPolymorphItemVnum : 0;
}

DWORD CHARACTER::GetMonsterDrainSPPoint() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwDrainSP : 0;
}

BYTE CHARACTER::GetMobRank() const
{
	if (!m_pkMobData)
		return MOB_RANK_KNIGHT;

	return m_pkMobData->m_table.bRank;
}

BYTE CHARACTER::GetMobSize() const
{
	if (!m_pkMobData)
		return MOBSIZE_MEDIUM;

	return m_pkMobData->m_table.bSize;
}

WORD CHARACTER::GetMobAttackRange() const
{
	switch (GetMobBattleType())
	{
	case BATTLE_TYPE_RANGE:
	case BATTLE_TYPE_MAGIC:
		return m_pkMobData->m_table.wAttackRange + GetPoint(POINT_BOW_DISTANCE);
	default:
		return m_pkMobData->m_table.wAttackRange;
	}
}

BYTE CHARACTER::GetMobBattleType() const
{
	if (!m_pkMobData)
		return BATTLE_TYPE_MELEE;

	return (m_pkMobData->m_table.bBattleType);
}

void CHARACTER::ComputeBattlePoints()
{
	if (IsPolymorphed())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob* pMob = CMobManager::instance().Get(dwMobVnum);
		DAM_LL iAtt = 0;
		DAM_LL iDef = 0;

		if (pMob)
		{
			iAtt = static_cast<DAM_LL>(GetLevel()) * 2 + static_cast<DAM_LL>(GetPolymorphPoint(POINT_ST)) * 2;
			// lev + con
			iDef = static_cast<DAM_LL>(GetLevel()) + static_cast<DAM_LL>(GetPolymorphPoint(POINT_HT) + pMob->m_table.wDef);
		}

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
	else if (IsPC())
	{
		SetPoint(POINT_ATT_GRADE, 0);
		SetPoint(POINT_DEF_GRADE, 0);
		SetPoint(POINT_CLIENT_DEF_GRADE, 0);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));

		DAM_LL iAtk = GetLevel() * 2;
		DAM_LL iStatAtk = 0;

		switch (GetJob())
		{
		case JOB_WARRIOR:
		case JOB_SURA:
			iStatAtk = (2 * GetPoint(POINT_ST));
			break;

		case JOB_ASSASSIN:
			iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_DX)) / 3;
			break;

		case JOB_SHAMAN:
			iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_IQ)) / 3;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			iStatAtk = (2 * GetPoint(POINT_ST));
			break;
#endif
		default:
			sys_err("invalid job %d", GetJob());
			iStatAtk = (2 * GetPoint(POINT_ST));
			break;
		}

		if (GetMountVnum() && iStatAtk < 2 * GetPoint(POINT_ST))
			iStatAtk = (2 * GetPoint(POINT_ST));

		iAtk += iStatAtk;

		if (GetMountVnum())
		{
			if (GetJob() == JOB_SURA && GetSkillGroup() == 1)
			{
				iAtk += (iAtk * GetHorseLevel()) / 60;
			}
			else
			{
				iAtk += (iAtk * GetHorseLevel()) / 30;
			}
		}

		iAtk += GetPoint(POINT_ATT_GRADE_BONUS);

		PointChange(POINT_ATT_GRADE, iAtk);

		// DEF = LEV + CON + ARMOR
		DAM_LL iShowDef = GetLevel() + GetPoint(POINT_HT);
		DAM_LL iDef = GetLevel() + (DAM_LL)(GetPoint(POINT_HT) / 1.25); // For Other
		DAM_LL iArmor = 0;

		LPITEM pkItem;

		for (int i = 0; i < WEAR_MAX_NUM; ++i)
		{
			if ((pkItem = GetWear(i)) && pkItem->GetType() == ITEM_ARMOR)
			{
				if (pkItem->GetSubType() == ARMOR_BODY || pkItem->GetSubType() == ARMOR_HEAD || pkItem->GetSubType() == ARMOR_FOOTS || pkItem->GetSubType() == ARMOR_SHIELD)
				{

					iArmor += pkItem->GetValue(1);
					iArmor += (2 * pkItem->GetValue(5));
				}
			}
		}

		if (true == IsHorseRiding())
		{
			if (iArmor < GetHorseArmor())
				iArmor = GetHorseArmor();

			const char* pHorseName = CHorseNameManager::instance().GetHorseName(GetPlayerID());

			if (pHorseName != NULL && strlen(pHorseName))
			{
				iArmor += 20;
			}
		}

		iArmor += GetPoint(POINT_DEF_GRADE_BONUS);
		iArmor += GetPoint(POINT_PARTY_DEFENDER_BONUS);

		// INTERNATIONAL_VERSION
		PointChange(POINT_DEF_GRADE, iDef + iArmor);
		PointChange(POINT_CLIENT_DEF_GRADE, (iShowDef + iArmor) - GetPoint(POINT_DEF_GRADE));
		// END_OF_INTERNATIONAL_VERSION

		PointChange(POINT_MAGIC_ATT_GRADE, GetLevel() * 2 + GetPoint(POINT_IQ) * 2 + GetPoint(POINT_MAGIC_ATT_GRADE_BONUS));
		PointChange(POINT_MAGIC_DEF_GRADE, GetLevel() + (GetPoint(POINT_IQ) * 3 + GetPoint(POINT_HT)) / 3 + iArmor / 2 + GetPoint(POINT_MAGIC_DEF_GRADE_BONUS));
	}
	else
	{
		DAM_LL iAtt = GetLevel() * 2 + GetPoint(POINT_ST) * 2;
		DAM_LL iDef = GetLevel() + GetPoint(POINT_HT) + GetMobTable().wDef;

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
}

void CHARACTER::ComputePoints()
{
#ifdef ENABLE_LOADING_RENEWAL
	if (IsPC() && GetLoadingState() == 1)
	{
		return;
	}
#endif

#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_POINT, false);
#endif
	long lStat = GetPoint(POINT_STAT);
	long lStatResetCount = GetPoint(POINT_STAT_RESET_COUNT);
	long lSkillActive = GetPoint(POINT_SKILL);
	long lSkillSub = GetPoint(POINT_SUB_SKILL);
	long lSkillHorse = GetPoint(POINT_HORSE_SKILL);
	long lLevelStep = GetPoint(POINT_LEVEL_STEP);

	long lAttackerBonus = GetPoint(POINT_PARTY_ATTACKER_BONUS);
	long lTankerBonus = GetPoint(POINT_PARTY_TANKER_BONUS);
	long lBufferBonus = GetPoint(POINT_PARTY_BUFFER_BONUS);
	long lSkillMasterBonus = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
	long lHasteBonus = GetPoint(POINT_PARTY_HASTE_BONUS);
	long lDefenderBonus = GetPoint(POINT_PARTY_DEFENDER_BONUS);

	HP_LL lHPRecovery = GetPoint(POINT_HP_RECOVERY);
	long lSPRecovery = GetPoint(POINT_SP_RECOVERY);

	memset(m_pointsInstant.points, 0, sizeof(m_pointsInstant.points));
	BuffOnAttr_ClearAll();
	m_SkillDamageBonus.clear();

	SetPoint(POINT_STAT, lStat);
	SetPoint(POINT_SKILL, lSkillActive);
	SetPoint(POINT_SUB_SKILL, lSkillSub);
	SetPoint(POINT_HORSE_SKILL, lSkillHorse);
	SetPoint(POINT_LEVEL_STEP, lLevelStep);
	SetPoint(POINT_STAT_RESET_COUNT, lStatResetCount);

	SetPoint(POINT_ST, GetRealPoint(POINT_ST));
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	SetPart(PART_MAIN, GetOriginalPart(PART_MAIN));
	SetPart(PART_WEAPON, GetOriginalPart(PART_WEAPON));
	SetPart(PART_HEAD, GetOriginalPart(PART_HEAD));
	SetPart(PART_HAIR, GetOriginalPart(PART_HAIR));
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	SetPart(PART_ACCE, GetOriginalPart(PART_ACCE));
#endif
#ifdef ENABLE_AURA_SYSTEM
	SetPart(PART_AURA, GetOriginalPart(PART_AURA));
#endif
#ifdef ENABLE_CROWN_SYSTEM
	SetPart(PART_CROWN, GetOriginalPart(PART_CROWN));
#endif

	SetPoint(POINT_PARTY_ATTACKER_BONUS, lAttackerBonus);
	SetPoint(POINT_PARTY_TANKER_BONUS, lTankerBonus);
	SetPoint(POINT_PARTY_BUFFER_BONUS, lBufferBonus);
	SetPoint(POINT_PARTY_SKILL_MASTER_BONUS, lSkillMasterBonus);
	SetPoint(POINT_PARTY_HASTE_BONUS, lHasteBonus);
	SetPoint(POINT_PARTY_DEFENDER_BONUS, lDefenderBonus);

	SetPoint(POINT_HP_RECOVERY, lHPRecovery);
	SetPoint(POINT_SP_RECOVERY, lSPRecovery);

	HP_LL iMaxHP;
	int iMaxSP;
	int iMaxStamina;

	if (IsPC())
	{
		iMaxHP = JobInitialPoints[GetJob()].max_hp + m_points.iRandomHP + GetPoint(POINT_HT) * JobInitialPoints[GetJob()].hp_per_ht;
		iMaxSP = JobInitialPoints[GetJob()].max_sp + m_points.iRandomSP + GetPoint(POINT_IQ) * JobInitialPoints[GetJob()].sp_per_iq;
		iMaxStamina = JobInitialPoints[GetJob()].max_stamina + GetPoint(POINT_HT) * JobInitialPoints[GetJob()].stamina_per_con;

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_ADD_HP);

			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_ADD_HP) / 100.0f);

				iMaxHP += static_cast<int>(pkSk->kPointPoly.Eval());
			}
		}

		SetPoint(POINT_MOV_SPEED, 200);
		SetPoint(POINT_ATT_SPEED, 100);
		PointChange(POINT_ATT_SPEED, GetPoint(POINT_PARTY_HASTE_BONUS));
		SetPoint(POINT_CASTING_SPEED, 100);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		for (int i = STATS_MONSTER_KILLED; i < STATS_MAX; i++)
		{
			SetPoint(i + POINT_POWER_RANK, static_cast<long long>(GetStats(i)));
		}
#endif
	}
	else
	{
		iMaxHP = m_pkMobData->m_table.dwMaxHP;
		iMaxSP = 0;
		iMaxStamina = 0;

		SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.sAttackSpeed);
		SetPoint(POINT_MOV_SPEED, m_pkMobData->m_table.sMovingSpeed
#ifdef ENABLE_INCREASED_MOV_SPEED_MOBS
		+ GetIncreasedSpeed()
#endif
		);
		SetPoint(POINT_CASTING_SPEED, m_pkMobData->m_table.sAttackSpeed);
	}

	ComputeBattlePoints();


	if (iMaxHP != GetMaxHP())
	{
		SetRealPoint(POINT_MAX_HP, iMaxHP);
	}
	PointChange(POINT_MAX_HP, 0);
	if (iMaxSP != GetMaxSP())
	{
		SetRealPoint(POINT_MAX_SP, iMaxSP);
	}
	PointChange(POINT_MAX_SP, 0);
	SetMaxStamina(iMaxStamina);
	// @fixme118 part1
	int iCurHP = this->GetHP();
	int iCurSP = this->GetSP();

	m_pointsInstant.dwImmuneFlag = 0;

	if (IsPC())
	{
		for (int i = 0; i < WEAR_MAX_NUM; i++)
		{
			LPITEM pItem = GetWear(i);
			if (pItem)
			{
				pItem->ModifyPoints(true);
				SET_BIT(m_pointsInstant.dwImmuneFlag, GetWear(i)->GetImmuneFlag());
			}
		}

		if (DragonSoul_IsDeckActivated())
		{
			for (int i = WEAR_MAX_NUM + DS_SLOT_MAX * DragonSoul_GetActiveDeck();
				i < WEAR_MAX_NUM + DS_SLOT_MAX * (DragonSoul_GetActiveDeck() + 1); i++)
			{
				LPITEM pItem = GetWear(i);
				if (pItem)
				{
					if (DSManager::instance().IsTimeLeftDragonSoul(pItem))
						pItem->ModifyPoints(true);
				}
			}
		}
#ifdef ENABLE_SKILL_SET_BONUS
		if (GetSkillSetBonus() != 0)
			SkillSetBonusGiveBuff();
#endif
#ifdef ENABLE_PASSIVE_SKILLS
		ComputeNewPassiveSkills();
#endif
#ifdef ENABLE_BOOSTER_ITEM
		BoosterGiveBuff();
#endif
#ifdef ENABLE_ALIGNMENT_SYSTEM
#ifndef DISABLE_ALIGNMENT_SYSTEM
		AlignmentBonus();
#endif
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
		if (GetBiologInfo().level > 0)
		{
			if (GetBiolog())
			{
				GetBiolog()->GiveBonus();
			}
		}
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		if (GetWear(WEAR_NEW_PET) && GetNewPetSystem())
			GetNewPetSystem()->GiveBuff();
#endif
	}


	if (GetHP() > GetMaxHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetSP() > GetMaxSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());

	RefreshAffect();

	if (IsPC())
	{
		if (this->GetHP() != iCurHP)
			this->PointChange(POINT_HP, iCurHP - this->GetHP());
		if (this->GetSP() != iCurSP)
			this->PointChange(POINT_SP, iCurSP - this->GetSP());
	}

	UpdatePacket();
	if (IsPC())
	{
#ifdef ENABLE_POWER_RANKING
		CalcutePowerRank();
#endif
#ifdef ENABLE_REFRESH_CONTROL
		RefreshControl(REFRESH_POINT, true, true);
#endif
#ifdef ENABLE_LOADING_RENEWAL
		if (GetLoadingState() == 2)
		{
			SetLoadingState(1);
		}
#endif
	}
}

void CHARACTER::ResetPlayTime(DWORD dwTimeRemain)
{
	m_dwPlayStartTime = get_dword_time() - dwTimeRemain;
}

const int aiRecoveryPercents[10] = { 1, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

EVENTFUNC(recovery_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("recovery_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->IsPC())
	{
		if (ch->IsAffectFlag(AFF_POISON))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
#ifdef ENABLE_WOLFMAN_CHARACTER
		if (ch->IsAffectFlag(AFF_BLEEDING))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
#endif
		if (!ch->IsDoor())
		{
			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * ch->GetMobTable().bRegenPercent) / 100));
		}

		if (ch->GetHP() >= ch->GetMaxHP())
		{
			ch->m_pkRecoveryEvent = NULL;
			return 0;
		}

		return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
	}
	else
	{
		ch->CheckTarget();
		ch->UpdateKillerMode();

		if (ch->IsAffectFlag(AFF_POISON) == true)
		{
			return 3;
		}
#ifdef ENABLE_WOLFMAN_CHARACTER
		if (ch->IsAffectFlag(AFF_BLEEDING))
			return 3;
#endif
		int iSec = (get_dword_time() - ch->GetLastMoveTime()) / 3000;

		ch->DistributeSP(ch);

		if (ch->GetMaxHP() <= ch->GetHP())
			return PASSES_PER_SEC(3);

		int iPercent = 0;
		int iAmount = 0;

		{
			iPercent = aiRecoveryPercents[MIN(9, iSec)];
			iAmount = 15 + (ch->GetMaxHP() * iPercent) / 100;
		}

		iAmount += (iAmount * ch->GetPoint(POINT_HP_REGEN)) / 100;
		ch->PointChange(POINT_HP, iAmount, false);
		return PASSES_PER_SEC(3);
	}
}

void CHARACTER::StartRecoveryEvent()
{
	if (m_pkRecoveryEvent)
		return;

	if (IsDead() || IsStun())
		return;

	if (IsNPC() && GetHP() >= GetMaxHP())
		return;
	
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
    if (auto& boss_dmg_mng{bossdamageranking::boss_dmg_ranking_manager()};
        !IsPC() && boss_dmg_mng.is_boss_in_ranking(GetRaceNum()))
    {
        return;
    }
#endif

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	int iSec = IsPC() ? 3 : (MAX(1, GetMobTable().bRegenCycle));
	m_pkRecoveryEvent = event_create(recovery_event, info, PASSES_PER_SEC(iSec));
}

void CHARACTER::Standup()
{
	struct packet_position pack_position;

	if (!IsPosition(POS_SITTING))
		return;

	SetPosition(POS_STANDING);

	pack_position.header = HEADER_GC_CHARACTER_POSITION;
	pack_position.vid = GetVID();
	pack_position.position = POSITION_GENERAL;

	PacketAround(&pack_position, sizeof(pack_position));
}

void CHARACTER::Sitdown(int is_ground)
{
	struct packet_position pack_position;

	if (IsPosition(POS_SITTING))
		return;

	SetPosition(POS_SITTING);

	pack_position.header = HEADER_GC_CHARACTER_POSITION;
	pack_position.vid = GetVID();
	pack_position.position = POSITION_SITTING_GROUND;
	PacketAround(&pack_position, sizeof(pack_position));
}

void CHARACTER::SetRotation(float fRot)
{
	m_pointsInstant.fRot = fRot;
}

void CHARACTER::SetRotationToXY(long x, long y)
{
	SetRotation(GetDegreeFromPositionXY(GetX(), GetY(), x, y));
}

bool CHARACTER::CannotMoveByAffect() const
{
	return (IsAffectFlag(AFF_STUN));
}

bool CHARACTER::CanMove() const
{
	if (CannotMoveByAffect())
		return false;

	if (GetMyShop())
		return false;

	/*
	   if (get_float_time() - m_fSyncTime < 0.2f)
	   return false;
	 */
	return true;
}

bool CHARACTER::Sync(long x, long y)
{
	if (!GetSectree())
		return false;
	if (IsPC() && IsDead())// @fixme192
		return false;

	LPSECTREE new_tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), x, y);

	if (!new_tree)
	{
		if (GetDesc())
		{
			sys_err("cannot find tree at %d %d (name: %s)", x, y, GetName());
#ifdef ENABLE_KICK_SYNC_FIX
			x = GetX();
			y = GetY();
			new_tree = GetSectree();
#else
			GetDesc()->SetPhase(PHASE_CLOSE);
#endif
		}
		else
		{
			sys_err("no tree: %s %d %d %d", GetName(), x, y, GetMapIndex());
			Dead();
		}

		return false;
	}

	SetRotationToXY(x, y);
	SetXYZ(x, y, 0);

	if (GetDungeon())
	{
		int iLastEventAttr = m_iEventAttr;
		m_iEventAttr = new_tree->GetEventAttribute(x, y);

		if (m_iEventAttr != iLastEventAttr)
		{
			if (GetParty())
			{
				quest::CQuestManager::instance().AttrOut(GetParty()->GetLeaderPID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetParty()->GetLeaderPID(), this, m_iEventAttr);
			}
			else
			{
				quest::CQuestManager::instance().AttrOut(GetPlayerID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetPlayerID(), this, m_iEventAttr);
			}
		}
	}

	if (GetSectree() != new_tree)
	{
		new_tree->InsertEntity(this);
	}

	return true;
}

void CHARACTER::Stop()
{
	GotoState(m_stateIdle);

	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
}

bool CHARACTER::Goto(long x, long y)
{
	if (GetX() == x && GetY() == y)
		return false;

	if (m_posDest.x == x && m_posDest.y == y)
	{
		if (!IsState(m_stateMove))
		{
			m_dwStateDuration = 4;
			GotoState(m_stateMove);
		}
		return false;
	}

	m_posDest.x = x;
	m_posDest.y = y;

	CalculateMoveDuration();

	m_dwStateDuration = 4;

	if (!IsState(m_stateMove))
	{
		if (GetVictim())
		{
			//MonsterChat(MONSTER_CHAT_CHASE);
			// MonsterChat(MONSTER_CHAT_ATTACK);
		}
	}

	GotoState(m_stateMove);

	return true;
}

DWORD CHARACTER::GetMotionMode() const
{
	DWORD dwMode = MOTION_MODE_GENERAL;

	if (IsPolymorphed())
		return dwMode;

	LPITEM pkItem;

	if ((pkItem = GetWear(WEAR_WEAPON)))
	{
		switch (pkItem->GetProto()->bSubType)
		{
		case WEAPON_SWORD:
			dwMode = MOTION_MODE_ONEHAND_SWORD;
			break;

		case WEAPON_TWO_HANDED:
			dwMode = MOTION_MODE_TWOHAND_SWORD;
			break;

		case WEAPON_DAGGER:
			dwMode = MOTION_MODE_DUALHAND_SWORD;
			break;

		case WEAPON_BOW:
			dwMode = MOTION_MODE_BOW;
			break;

		case WEAPON_BELL:
			dwMode = MOTION_MODE_BELL;
			break;

		case WEAPON_FAN:
			dwMode = MOTION_MODE_FAN;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case WEAPON_CLAW:
			dwMode = MOTION_MODE_CLAW;
			break;
#endif
		}
	}
	return dwMode;
}

float CHARACTER::GetMoveMotionSpeed() const
{
	DWORD dwMode = GetMotionMode();

	const CMotion* pkMotion = NULL;

	if (!GetMountVnum())
		pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(dwMode, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));
	else
	{
		pkMotion = CMotionManager::instance().GetMotion(GetMountVnum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));

		if (!pkMotion)
			pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_HORSE, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));
	}

	if (pkMotion)
	{
		return -pkMotion->GetAccumVector().y / pkMotion->GetDuration();
	}
		
	else
	{
		sys_err("cannot find motion (name %s race %d mode %d)", GetName(), GetRaceNum(), dwMode);
		return 300.0f;
	}
}

float CHARACTER::GetMoveSpeed() const
{
	return GetMoveMotionSpeed() * 10000 / CalculateDuration(GetLimitPoint(POINT_MOV_SPEED), 10000);
}

void CHARACTER::CalculateMoveDuration()
{
	m_posStart.x = GetX();
	m_posStart.y = GetY();

	float fDist = DISTANCE_SQRT(m_posStart.x - m_posDest.x, m_posStart.y - m_posDest.y);

	float motionSpeed = GetMoveMotionSpeed();

	m_dwMoveDuration = CalculateDuration(GetLimitPoint(POINT_MOV_SPEED),
		(int)((fDist / motionSpeed) * 1000.0f));

	m_dwMoveStartTime = get_dword_time();
}

bool CHARACTER::Move(long x, long y)
{
	if (IsPC() && IsDead())// @fixme226
		return false;
	if (GetX() == x && GetY() == y)
		return true;

	OnMove();
	return Sync(x, y);
}

void CHARACTER::SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, int iRot)
{
	TPacketGCMove pack;

	if (bFunc == FUNC_WAIT)
	{
		x = m_posDest.x;
		y = m_posDest.y;
		dwDuration = m_dwMoveDuration;
	}

	EncodeMovePacket(pack, GetVID(), bFunc, bArg, x, y, dwDuration, dwTime, iRot == -1 ? (int)GetRotation() / 5 : iRot);
	PacketView(&pack, sizeof(TPacketGCMove), this);
}

HP_LL CHARACTER::GetRealPoint(BYTE type) const
{
	return m_points.points[type];
}

void CHARACTER::SetRealPoint(BYTE type, HP_LL val)
{
	m_points.points[type] = val;
}

int CHARACTER::GetPolymorphPoint(BYTE type) const
{
	if (IsPolymorphed() && !IsPolyMaintainStat())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob* pMob = CMobManager::instance().Get(dwMobVnum);
		int iPower = GetPolymorphPower();

		if (pMob)
		{
			switch (type)
			{
			case POINT_ST:
				if ((GetJob() == JOB_SHAMAN) || ((GetJob() == JOB_SURA) && (GetSkillGroup() == 2)))
					return pMob->m_table.bStr * iPower / 100 + GetPoint(POINT_IQ);
				return pMob->m_table.bStr * iPower / 100 + GetPoint(POINT_ST);

			case POINT_HT:
				return pMob->m_table.bCon * iPower / 100 + GetPoint(POINT_HT);

			case POINT_IQ:
				return pMob->m_table.bInt * iPower / 100 + GetPoint(POINT_IQ);

			case POINT_DX:
				return pMob->m_table.bDex * iPower / 100 + GetPoint(POINT_DX);
			}
		}
	}

	return GetPoint(type);
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
int64_t CHARACTER::GetPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	int64_t val = m_pointsInstant.points[type];
	int64_t max_val = LLONG_MAX;

	switch (type)
	{
	case POINT_STEAL_HP:
	case POINT_STEAL_SP:
		max_val = 50;
		break;
	case POINT_GOLD:
		max_val = GOLD_MAX;
		break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR: %s type %d val %lld (max: %lld)", GetName(), val, max_val);

	return (val);
}
#else
int CHARACTER::GetPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	int val = m_pointsInstant.points[type];
	int max_val = INT_MAX;

	switch (type)
	{
	case POINT_STEAL_HP:
	case POINT_STEAL_SP:
		max_val = 50;
		break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR: %s type %d val %d (max: %d)", GetName(), type, val, max_val);//@Lightwork#140

	return (val);
}
#endif

HP_LL CHARACTER::GetLimitPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	HP_LL val = m_pointsInstant.points[type];
	HP_LL max_val = LLONG_MAX;
	HP_LL limit = LLONG_MAX;
	HP_LL min_limit = -LLONG_MAX;

	switch (type)
	{
	case POINT_ATT_SPEED:
	{
		min_limit = 0;

		if (IsPC())
		{
			if (IsRiding())
			{
				limit = 210;
			}
			else
			{
				limit = 180;
			}
		}
		else
		{
			limit = 250;
		}
		break;
	}

	case POINT_MOV_SPEED:
		min_limit = 0;

		if (IsPC())
			limit = 200;
		else
			limit = 250;
		break;

	case POINT_STEAL_HP:
	case POINT_STEAL_SP:
		limit = 50;
		max_val = 50;
		break;

	case POINT_MALL_ATTBONUS:
	case POINT_MALL_DEFBONUS:
		limit = 20;
		max_val = 50;
		break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR: %s type %d val %d (max: %lld)", GetName(), type, val, max_val);//@Light5#15

	if (val > limit)
		val = limit;

	if (val < min_limit)
		val = min_limit;

	return (val);
}
#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::SetPoint(BYTE type, int64_t val)
#else
void CHARACTER::SetPoint(BYTE type, int val)
#endif
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return;
	}

	m_pointsInstant.points[type] = val;

	if (type == POINT_MOV_SPEED && get_dword_time() < m_dwMoveStartTime + m_dwMoveDuration)
	{
		CalculateMoveDuration();
	}
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
int64_t CHARACTER::GetAllowedGold() const
#else
INT CHARACTER::GetAllowedGold() const
#endif
{
	if (GetLevel() <= 10)
		return 100000;
	else if (GetLevel() <= 20)
		return 500000;
	else
		return 50000000;
}

void CHARACTER::CheckMaximumPoints()
{
	if (GetMaxHP() < GetHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetMaxSP() < GetSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::PointChange(BYTE type, int64_t amount, bool bAmount, bool bBroadcast)
{
	int64_t val = 0;
#else
void CHARACTER::PointChange(BYTE type, int amount, bool bAmount, bool bBroadcast)
{
	int val = 0;
#endif

	switch (type)
	{
	case POINT_NONE:
		return;

	case POINT_LEVEL:
		if ((GetLevel() + amount) > gPlayerMaxLevel)
			return;

		SetLevel(GetLevel() + amount);
		val = GetLevel();
#ifdef ENABLE_WOLFMAN_CHARACTER
		if (GetJob() == JOB_WOLFMAN)
		{
			if ((5 <= val) && (GetSkillGroup() != 1))
			{
				ClearSkill();
				SetSkillGroup(1);
				SetRealPoint(POINT_SKILL, GetLevel() - 1);
				SetPoint(POINT_SKILL, GetRealPoint(POINT_SKILL));
				PointChange(POINT_SKILL, 0);
				WolfmanSkillSet();
			}
		}
#endif
		PointChange(POINT_NEXT_EXP, GetNextExp(), false);

		if (amount)
		{
			quest::CQuestManager::instance().LevelUp(GetPlayerID());
			
#ifdef __SKILL_TRAINING__
			if (GetLevel() >= 5 && GetSkillGroup() == 0)
				ChatPacket(CHAT_TYPE_COMMAND, "SkillTrainingShow");
#endif

			if (GetGuild())
			{
				GetGuild()->LevelChange(GetPlayerID(), GetLevel());
			}

			if (GetParty())
			{
				GetParty()->RequestSetMemberLevel(GetPlayerID(), GetLevel());
			}
		}

#ifdef ENABLE_DISTANCE_SKILL_SELECT
		if (GetLevel() >= 5 && GetSkillGroup() == 0)
		{
			ChatPacket(CHAT_TYPE_COMMAND, "SELECT_JOB");
		}
#endif
		break;

	case POINT_NEXT_EXP:
		val = GetNextExp();
		bAmount = false;
		break;

	case POINT_EXP:
	{
#ifdef ENABLE_CHAR_SETTINGS
		if (m_char_settings.antiexp)
			return;
#endif
		DWORD exp = GetExp();
		DWORD next_exp = GetNextExp();

		if ((amount < 0) && (exp < (DWORD)(-amount)))
		{
			amount = -exp;

			SetExp(exp + amount);
			val = GetExp();
		}
		else
		{
			if (gPlayerMaxLevel <= GetLevel())
				return;

			DWORD iExpBalance = 0;

			if (exp + amount >= next_exp)
			{
				iExpBalance = (exp + amount) - next_exp;
				amount = next_exp - exp;

				SetExp(0);
				exp = next_exp;
			}
			else
			{
				SetExp(exp + amount);
				exp = GetExp();
			}

			DWORD q = DWORD(next_exp / 4.0f);
			int iLevStep = GetRealPoint(POINT_LEVEL_STEP);

			if (iLevStep >= 4)
			{
				sys_err("%s LEVEL_STEP bigger than 4! (%d)", GetName(), iLevStep);
				iLevStep = 4;
			}

			if (exp >= next_exp && iLevStep < 4)
			{
				for (int i = 0; i < 4 - iLevStep; ++i)
					PointChange(POINT_LEVEL_STEP, 1, false, true);
			}
			else if (exp >= q * 3 && iLevStep < 3)
			{
				for (int i = 0; i < 3 - iLevStep; ++i)
					PointChange(POINT_LEVEL_STEP, 1, false, true);
			}
			else if (exp >= q * 2 && iLevStep < 2)
			{
				for (int i = 0; i < 2 - iLevStep; ++i)
					PointChange(POINT_LEVEL_STEP, 1, false, true);
			}
			else if (exp >= q && iLevStep < 1)
				PointChange(POINT_LEVEL_STEP, 1);

			if (iExpBalance)
			{
				PointChange(POINT_EXP, iExpBalance);
			}

			val = GetExp();
		}
	}
	break;

	case POINT_LEVEL_STEP:
		if (amount > 0)
		{
			val = GetPoint(POINT_LEVEL_STEP) + amount;

			switch (val)
			{
			case 1:
			case 2:
			case 3:
				if ((GetLevel() <= g_iStatusPointGetLevelLimit) &&
					(GetLevel() <= gPlayerMaxLevel)) // @fixme104
					PointChange(POINT_STAT, 1);
				break;

			case 4:
			{
				HP_LL iHP = number(JobInitialPoints[GetJob()].hp_per_lv_begin, JobInitialPoints[GetJob()].hp_per_lv_end);
				int iSP = number(JobInitialPoints[GetJob()].sp_per_lv_begin, JobInitialPoints[GetJob()].sp_per_lv_end);

				m_points.iRandomHP += iHP;
				m_points.iRandomSP += iSP;

				if (GetSkillGroup())
				{
					if (GetLevel() >= 5)
						PointChange(POINT_SKILL, 1);

					if (GetLevel() >= 9)
						PointChange(POINT_SUB_SKILL, 1);
				}

				PointChange(POINT_MAX_HP, iHP);
				PointChange(POINT_MAX_SP, iSP);
				PointChange(POINT_LEVEL, 1, false, true);

				val = 0;
			}
			break;
			}
			PointChange(POINT_HP, GetMaxHP() - GetHP());
			PointChange(POINT_SP, GetMaxSP() - GetSP());
			PointChange(POINT_STAMINA, GetMaxStamina() - GetStamina());

			SetPoint(POINT_LEVEL_STEP, val);
			SetRealPoint(POINT_LEVEL_STEP, val);

			Save();
		}
		else
			val = GetPoint(POINT_LEVEL_STEP);

		break;

	case POINT_HP:
	{
		if (IsDead() || IsStun())
			return;

		HP_LL prev_hp = GetHP();

		amount = MIN(GetMaxHP() - GetHP(), amount);
		SetHP(GetHP() + amount);
		val = GetHP();

		BroadcastTargetPacket();

		if (GetParty() && IsPC() && val != prev_hp)
			GetParty()->SendPartyInfoOneToAll(this);
	}
	break;

	case POINT_SP:
	{
		if (IsDead() || IsStun())
			return;

		amount = MIN(GetMaxSP() - GetSP(), amount);
		SetSP(GetSP() + amount);
		val = GetSP();
	}
	break;

	case POINT_STAMINA:
	{
		if (IsDead() || IsStun())
			return;

		int prev_val = GetStamina();
		amount = MIN(GetMaxStamina() - GetStamina(), amount);
		SetStamina(GetStamina() + amount);
		val = GetStamina();

		if (val == 0)
		{
			// Stamina
			SetNowWalking(true);
		}
		else if (prev_val == 0)
		{
			ResetWalking();
		}

		if (amount < 0 && val != 0)
			return;
	}
	break;

	case POINT_MAX_HP:
	{
		SetPoint(type, GetPoint(type) + amount);

		//SetMaxHP(GetMaxHP() + amount);
		HP_LL hp = GetRealPoint(POINT_MAX_HP);
		HP_LL add_hp = MIN(3500, hp * GetPoint(POINT_MAX_HP_PCT) / 100);
		add_hp += GetPoint(POINT_MAX_HP);
		add_hp += GetPoint(POINT_PARTY_TANKER_BONUS);

		SetMaxHP(hp + add_hp);

		val = GetMaxHP();
	}
	break;

	case POINT_MAX_SP:
	{
		SetPoint(type, GetPoint(type) + amount);

		//SetMaxSP(GetMaxSP() + amount);
		int sp = GetRealPoint(POINT_MAX_SP);
		int add_sp = MIN(800, sp * GetPoint(POINT_MAX_SP_PCT) / 100);
		add_sp += GetPoint(POINT_MAX_SP);
		add_sp += GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);

		SetMaxSP(sp + add_sp);

		val = GetMaxSP();
	}
	break;

	case POINT_MAX_HP_PCT:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);

		PointChange(POINT_MAX_HP, 0);
		break;

	case POINT_MAX_SP_PCT:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);

		PointChange(POINT_MAX_SP, 0);
		break;

	case POINT_MAX_STAMINA:
		SetMaxStamina(GetMaxStamina() + amount);
		val = GetMaxStamina();
		break;

	case POINT_GOLD:
	{
		const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(amount);

		if (GOLD_MAX <= nTotalMoney)
		{
			sys_err("[OVERFLOW_GOLD] OriGold %lld AddedGold %lld id %u Name %s ", GetGold(), amount, GetPlayerID(), GetName());
			return;
		}

		SetGold(GetGold() + amount);
		val = GetGold();
	}
	break;

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	case POINT_BATTLE_PASS_PREMIUM_ID:
	{
		SetExtBattlePassPremiumID(amount);
		val = GetExtBattlePassPremiumID();
	}
	break;
#endif

	case POINT_SKILL:
	case POINT_STAT:
	case POINT_SUB_SKILL:
	case POINT_STAT_RESET_COUNT:
	case POINT_HORSE_SKILL:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);

		SetRealPoint(type, val);
		break;

	case POINT_DEF_GRADE:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);

		PointChange(POINT_CLIENT_DEF_GRADE, amount);
		break;

	case POINT_CLIENT_DEF_GRADE:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		break;

#ifdef ENABLE_LMW_PROTECTION
	case POINT_ATT_SPEED:
	{
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		CalculateAttackCount();
		break;
	}
#else
	case POINT_ATT_SPEED:
#endif

	case POINT_ST:
	case POINT_HT:
	case POINT_DX:
	case POINT_IQ:
	case POINT_HP_REGEN:
	case POINT_SP_REGEN:
	case POINT_ATT_GRADE:
	case POINT_MOV_SPEED:
	case POINT_CASTING_SPEED:
	case POINT_MAGIC_ATT_GRADE:
	case POINT_MAGIC_DEF_GRADE:
	case POINT_BOW_DISTANCE:
	case POINT_HP_RECOVERY:
	case POINT_SP_RECOVERY:

	case POINT_ATTBONUS_HUMAN:	// 42
	case POINT_ATTBONUS_ANIMAL:	// 43
	case POINT_ATTBONUS_ORC:	// 44
	case POINT_ATTBONUS_MILGYO:	// 45
	case POINT_ATTBONUS_UNDEAD:	// 46
	case POINT_ATTBONUS_DEVIL:	// 47

	case POINT_ATTBONUS_MONSTER:
	case POINT_ATTBONUS_SURA:
	case POINT_ATTBONUS_ASSASSIN:
	case POINT_ATTBONUS_WARRIOR:
	case POINT_ATTBONUS_SHAMAN:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case POINT_ATTBONUS_WOLFMAN:
#endif

	case POINT_POISON_PCT:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case POINT_BLEEDING_PCT:
#endif
	case POINT_STUN_PCT:
	case POINT_SLOW_PCT:

	case POINT_BLOCK:
	case POINT_DODGE:

	case POINT_CRITICAL_PCT:
	case POINT_RESIST_CRITICAL:
	case POINT_PENETRATE_PCT:
	case POINT_RESIST_PENETRATE:
	case POINT_CURSE_PCT:

	case POINT_STEAL_HP:		// 48
	case POINT_STEAL_SP:		// 49

	case POINT_MANA_BURN_PCT:	// 50
	case POINT_DAMAGE_SP_RECOVER:	// 51
	case POINT_RESIST_NORMAL_DAMAGE:
	case POINT_RESIST_SWORD:
	case POINT_RESIST_TWOHAND:
	case POINT_RESIST_DAGGER:
	case POINT_RESIST_BELL:
	case POINT_RESIST_FAN:
	case POINT_RESIST_BOW:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case POINT_RESIST_CLAW:
#endif
	case POINT_RESIST_FIRE:
	case POINT_RESIST_ELEC:
	case POINT_RESIST_MAGIC:
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	case POINT_ACCEDRAIN_RATE:
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	case POINT_RESIST_MAGIC_REDUCTION:
#endif
#ifdef ENABLE_EXTRA_APPLY_BONUS
	case POINT_ATTBONUS_STONE:
	case POINT_ATTBONUS_BOSS:
	case POINT_ATTBONUS_PVM_STR:
	case POINT_ATTBONUS_PVM_AVG:
	case POINT_ATTBONUS_PVM_BERSERKER:
	case POINT_ATTBONUS_ELEMENT:
	case POINT_DEFBONUS_PVM:
	case POINT_DEFBONUS_ELEMENT:
	case POINT_ATTBONUS_PVP:
	case POINT_DEFBONUS_PVP:
	case POINT_ATT_FIRE:
	case POINT_ATT_ICE:
	case POINT_ATT_WIND:
	case POINT_ATT_EARTH:
	case POINT_ATT_DARK:
	case POINT_ATT_ELEC:
#endif
	case POINT_RESIST_WIND:
	case POINT_RESIST_ICE:
	case POINT_RESIST_EARTH:
	case POINT_RESIST_DARK:
	case POINT_REFLECT_MELEE:	// 67
	case POINT_REFLECT_CURSE:	// 68
	case POINT_POISON_REDUCE:	// 69
#ifdef ENABLE_WOLFMAN_CHARACTER
	case POINT_BLEEDING_REDUCE:
#endif
	case POINT_KILL_SP_RECOVER:	// 70
	case POINT_KILL_HP_RECOVERY:	// 75
	case POINT_HIT_HP_RECOVERY:
	case POINT_HIT_SP_RECOVERY:
	case POINT_MANASHIELD:
	case POINT_ATT_BONUS:
	case POINT_DEF_BONUS:
	case POINT_SKILL_DAMAGE_BONUS:
	case POINT_NORMAL_HIT_DAMAGE_BONUS:

		// DEPEND_BONUS_ATTRIBUTES
	case POINT_SKILL_DEFEND_BONUS:
	case POINT_NORMAL_HIT_DEFEND_BONUS:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		break;
		// END_OF_DEPEND_BONUS_ATTRIBUTES

	case POINT_PARTY_ATTACKER_BONUS:
	case POINT_PARTY_TANKER_BONUS:
	case POINT_PARTY_BUFFER_BONUS:
	case POINT_PARTY_SKILL_MASTER_BONUS:
	case POINT_PARTY_HASTE_BONUS:
	case POINT_PARTY_DEFENDER_BONUS:

	case POINT_RESIST_WARRIOR:
	case POINT_RESIST_ASSASSIN:
	case POINT_RESIST_SURA:
	case POINT_RESIST_SHAMAN:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case POINT_RESIST_WOLFMAN:
#endif

		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		break;

	case POINT_MALL_ATTBONUS:
	case POINT_MALL_DEFBONUS:
	case POINT_MALL_EXPBONUS:
	case POINT_MALL_ITEMBONUS:
	case POINT_MALL_GOLDBONUS:
	case POINT_MELEE_MAGIC_ATT_BONUS_PER:
		if (GetPoint(type) + amount > 100)
		{
			sys_err("MALL_BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
			amount = 100 - GetPoint(type);
		}

		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		break;

	case POINT_RAMADAN_CANDY_BONUS_EXP:
		SetPoint(type, amount);
		val = GetPoint(type);
		break;

	case POINT_EXP_DOUBLE_BONUS:	// 71
	case POINT_GOLD_DOUBLE_BONUS:	// 72
	case POINT_ITEM_DROP_BONUS:	// 73
	case POINT_POTION_BONUS:	// 74
		if (GetPoint(type) + amount > 100)
		{
			sys_err("BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
			amount = 100 - GetPoint(type);
		}

		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		break;

	case POINT_IMMUNE_STUN:		// 76
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		if (val)
		{
			SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
		}
		else
		{
			REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
		}
		break;

	case POINT_IMMUNE_SLOW:		// 77
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		if (val)
		{
			SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
		}
		else
		{
			REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
		}
		break;

	case POINT_IMMUNE_FALL:	// 78
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		if (val)
		{
			SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
		}
		else
		{
			REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
		}
		break;

	case POINT_ATT_GRADE_BONUS:
		SetPoint(type, GetPoint(type) + amount);
		PointChange(POINT_ATT_GRADE, amount);
		val = GetPoint(type);
		break;

	case POINT_DEF_GRADE_BONUS:
		SetPoint(type, GetPoint(type) + amount);
		PointChange(POINT_DEF_GRADE, amount);
		val = GetPoint(type);
		break;

	case POINT_MAGIC_ATT_GRADE_BONUS:
		SetPoint(type, GetPoint(type) + amount);
		PointChange(POINT_MAGIC_ATT_GRADE, amount);
		val = GetPoint(type);
		break;

	case POINT_MAGIC_DEF_GRADE_BONUS:
		SetPoint(type, GetPoint(type) + amount);
		PointChange(POINT_MAGIC_DEF_GRADE, amount);
		val = GetPoint(type);
		break;

	case POINT_VOICE:
	case POINT_EMPIRE_POINT:
		//sys_err("CHARACTER::PointChange: %s: point cannot be changed. use SetPoint instead (type: %d)", GetName(), type);
		val = GetRealPoint(type);
		break;

	case POINT_POLYMORPH:
		SetPoint(type, GetPoint(type) + amount);
		val = GetPoint(type);
		SetPolymorph(val);
		break;

	case POINT_MOUNT:
		SetPoint(type, amount);
		val = GetPoint(type);
		//MountVnum(val);
		break;

	case POINT_ENERGY:
	case POINT_COSTUME_ATTR_BONUS:
	{
		int old_val = GetPoint(type);
		SetPoint(type, old_val + amount);
		val = GetPoint(type);
		BuffOnAttr_ValueChange(type, old_val, val);
	}
	break;

#ifdef ENABLE_PLAYER_STATS_SYSTEM

#ifdef ENABLE_POWER_RANKING
	case POINT_POWER_RANK:			{val = static_cast<int64_t> (SetStats(STATS_POWER, amount));												break; }
#endif

	case POINT_MONSTER_KILLED:		{val = static_cast<int64_t>(SetStats(STATS_MONSTER_KILLED, GetStats(STATS_MONSTER_KILLED) + amount));		break; }
	case POINT_STONE_KILLED:		{val = static_cast<int64_t>(SetStats(STATS_STONE_KILLED, GetStats(STATS_STONE_KILLED) + amount));			break; }
	case POINT_BOSS_KILLED:			{val = static_cast<int64_t>(SetStats(STATS_BOSS_KILLED, GetStats(STATS_BOSS_KILLED) + amount));				break; }
	case POINT_PLAYER_KILLED:		{val = static_cast<int64_t>(SetStats(STATS_PLAYER_KILLED, GetStats(STATS_PLAYER_KILLED) + amount));			break; }

	case POINT_MONSTER_DAMAGE:		{val = static_cast<int64_t>(SetStats(STATS_MONSTER_DAMAGE, amount));										break; }
	case POINT_STONE_DAMAGE:		{val = static_cast<int64_t>(SetStats(STATS_STONE_DAMAGE, amount));											break; }
	case POINT_BOSS_DAMAGE:			{val = static_cast<int64_t>(SetStats(STATS_BOSS_DAMAGE, amount));											break; }
	case POINT_PLAYER_DAMAGE:		{val = static_cast<int64_t>(SetStats(STATS_PLAYER_DAMAGE, amount));											break; }

	case POINT_OPENED_CHEST:		{val = static_cast<int64_t>(SetStats(STATS_OPENED_CHEST, GetStats(STATS_OPENED_CHEST) + amount));			break; }
	case POINT_FISHING:				{val = static_cast<int64_t>(SetStats(STATS_FISHING, GetStats(STATS_FISHING) + amount));						break; }
	case POINT_MINING:				{val = static_cast<int64_t>(SetStats(STATS_MINING, GetStats(STATS_MINING) + amount));						break; }
	case POINT_COMPLATE_DUNGEON:	{val = static_cast<int64_t>(SetStats(STATS_COMPLATE_DUNGEON, GetStats(STATS_COMPLATE_DUNGEON) + amount));	break; }
	case POINT_UPGRADE_ITEM:		{val = static_cast<int64_t>(SetStats(STATS_UPGRADE_ITEM, GetStats(STATS_UPGRADE_ITEM) + amount));			break; }
	case POINT_USE_ENCHANTED_ITEM:	{val = static_cast<int64_t>(SetStats(STATS_USE_ENCHANTED_ITEM, GetStats(STATS_USE_ENCHANTED_ITEM) + amount));break; }

#endif

	default:
		sys_err("CHARACTER::PointChange: %s: unknown point change type %d", GetName(), type);
		return;
	}

	switch (type)
	{
	case POINT_LEVEL:
	case POINT_ST:
	case POINT_DX:
	case POINT_IQ:
	case POINT_HT:
		ComputeBattlePoints();
		break;
	case POINT_MAX_HP:
	case POINT_MAX_SP:
	case POINT_MAX_STAMINA:
		break;
	}

	if (type == POINT_HP && amount == 0)
		return;

	if (GetDesc())
	{
		struct packet_point_change pack;

		pack.header = HEADER_GC_CHARACTER_POINT_CHANGE;
		pack.dwVID = m_vid;
		pack.type = type;
		pack.value = val;

		if (bAmount)
			pack.amount = amount;
		else
			pack.amount = 0;

		if (!bBroadcast)
			GetDesc()->Packet(&pack, sizeof(struct packet_point_change));
		else
			PacketAround(&pack, sizeof(pack));
	}
}

void CHARACTER::ApplyPoint(BYTE bApplyType, HP_LL iVal)
{
	switch (bApplyType)
	{
	case APPLY_NONE:
		break;

	case APPLY_CON:
		PointChange(POINT_HT, iVal);
		PointChange(POINT_MAX_HP, (iVal * JobInitialPoints[GetJob()].hp_per_ht));
		PointChange(POINT_MAX_STAMINA, (iVal * JobInitialPoints[GetJob()].stamina_per_con));
		break;

	case APPLY_INT:
		PointChange(POINT_IQ, iVal);
		PointChange(POINT_MAX_SP, (iVal * JobInitialPoints[GetJob()].sp_per_iq));
		break;

	case APPLY_SKILL:
		// SKILL_DAMAGE_BONUS
	{
		BYTE bSkillVnum = (BYTE)(((DWORD)iVal) >> 24);
		HP_LL iAdd = iVal & 0x00800000;
		HP_LL iChange = iVal & 0x007fffff;
		if (0 == iAdd)
			iChange = -iChange;

		boost::unordered_map<BYTE, int>::iterator iter = m_SkillDamageBonus.find(bSkillVnum);

		if (iter == m_SkillDamageBonus.end())
			m_SkillDamageBonus.insert(std::make_pair(bSkillVnum, iChange));
		else
			iter->second += iChange;
	}
	// END_OF_SKILL_DAMAGE_BONUS
	break;

	case APPLY_MAX_HP:
	case APPLY_MAX_HP_PCT:
	{
		int i = GetMaxHP(); if (i == 0) break;
		PointChange(aApplyInfo[bApplyType].bPointType, iVal);
		float fRatio = (float)GetMaxHP() / (float)i;
		PointChange(POINT_HP, GetHP() * fRatio - GetHP());
	}
	break;

	case APPLY_MAX_SP:
	case APPLY_MAX_SP_PCT:
	{
		int i = GetMaxSP(); if (i == 0) break;
		PointChange(aApplyInfo[bApplyType].bPointType, iVal);
		float fRatio = (float)GetMaxSP() / (float)i;
		PointChange(POINT_SP, GetSP() * fRatio - GetSP());
	}
	break;

	case APPLY_STR:
	case APPLY_DEX:
	case APPLY_ATT_SPEED:
	case APPLY_MOV_SPEED:
	case APPLY_CAST_SPEED:
	case APPLY_HP_REGEN:
	case APPLY_SP_REGEN:
	case APPLY_POISON_PCT:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case APPLY_BLEEDING_PCT:
#endif
	case APPLY_STUN_PCT:
	case APPLY_SLOW_PCT:
	case APPLY_CRITICAL_PCT:
	case APPLY_PENETRATE_PCT:
	case APPLY_ATTBONUS_HUMAN:
	case APPLY_ATTBONUS_ANIMAL:
	case APPLY_ATTBONUS_ORC:
	case APPLY_ATTBONUS_MILGYO:
	case APPLY_ATTBONUS_UNDEAD:
	case APPLY_ATTBONUS_DEVIL:
	case APPLY_ATTBONUS_WARRIOR:	// 59
	case APPLY_ATTBONUS_ASSASSIN:	// 60
	case APPLY_ATTBONUS_SURA:	// 61
	case APPLY_ATTBONUS_SHAMAN:	// 62
#ifdef ENABLE_WOLFMAN_CHARACTER
	case APPLY_ATTBONUS_WOLFMAN:
#endif
	case APPLY_ATTBONUS_MONSTER:	// 63
	case APPLY_STEAL_HP:
	case APPLY_STEAL_SP:
	case APPLY_MANA_BURN_PCT:
	case APPLY_DAMAGE_SP_RECOVER:
	case APPLY_BLOCK:
	case APPLY_DODGE:
	case APPLY_RESIST_SWORD:
	case APPLY_RESIST_TWOHAND:
	case APPLY_RESIST_DAGGER:
	case APPLY_RESIST_BELL:
	case APPLY_RESIST_FAN:
	case APPLY_RESIST_BOW:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case APPLY_RESIST_CLAW:
#endif
	case APPLY_RESIST_FIRE:
	case APPLY_RESIST_ELEC:
	case APPLY_RESIST_MAGIC:
	case APPLY_RESIST_WIND:
	case APPLY_RESIST_ICE:
	case APPLY_RESIST_EARTH:
	case APPLY_RESIST_DARK:
	case APPLY_REFLECT_MELEE:
	case APPLY_REFLECT_CURSE:
	case APPLY_ANTI_CRITICAL_PCT:
	case APPLY_ANTI_PENETRATE_PCT:
	case APPLY_POISON_REDUCE:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case APPLY_BLEEDING_REDUCE:
#endif
	case APPLY_KILL_SP_RECOVER:
	case APPLY_EXP_DOUBLE_BONUS:
	case APPLY_GOLD_DOUBLE_BONUS:
	case APPLY_ITEM_DROP_BONUS:
	case APPLY_POTION_BONUS:
	case APPLY_KILL_HP_RECOVER:
	case APPLY_IMMUNE_STUN:
	case APPLY_IMMUNE_SLOW:
	case APPLY_IMMUNE_FALL:
	case APPLY_BOW_DISTANCE:
	case APPLY_ATT_GRADE_BONUS:
	case APPLY_DEF_GRADE_BONUS:
	case APPLY_MAGIC_ATT_GRADE:
	case APPLY_MAGIC_DEF_GRADE:
	case APPLY_CURSE_PCT:
	case APPLY_MAX_STAMINA:
	case APPLY_MALL_ATTBONUS:
	case APPLY_MALL_DEFBONUS:
	case APPLY_MALL_EXPBONUS:
	case APPLY_MALL_ITEMBONUS:
	case APPLY_MALL_GOLDBONUS:
	case APPLY_SKILL_DAMAGE_BONUS:
	case APPLY_NORMAL_HIT_DAMAGE_BONUS:

		// DEPEND_BONUS_ATTRIBUTES
	case APPLY_SKILL_DEFEND_BONUS:
	case APPLY_NORMAL_HIT_DEFEND_BONUS:
		// END_OF_DEPEND_BONUS_ATTRIBUTES
	case APPLY_RESIST_WARRIOR:
	case APPLY_RESIST_ASSASSIN:
	case APPLY_RESIST_SURA:
	case APPLY_RESIST_SHAMAN:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case APPLY_RESIST_WOLFMAN:
#endif
	case APPLY_ENERGY:					// 82
	case APPLY_DEF_GRADE:				// 83
	case APPLY_COSTUME_ATTR_BONUS:		// 84
	case APPLY_MAGIC_ATTBONUS_PER:		// 85
	case APPLY_MELEE_MAGIC_ATTBONUS_PER:			// 86
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	case APPLY_ACCEDRAIN_RATE:			//97
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	case APPLY_RESIST_MAGIC_REDUCTION:	//98
#endif
#ifdef ENABLE_EXTRA_APPLY_BONUS
	case APPLY_ATTBONUS_STONE:
	case APPLY_ATTBONUS_BOSS:
	case APPLY_ATTBONUS_PVM_STR:
	case APPLY_ATTBONUS_PVM_AVG:
	case APPLY_ATTBONUS_PVM_BERSERKER:
	case APPLY_ATTBONUS_ELEMENT:
	case APPLY_DEFBONUS_PVM:
	case APPLY_DEFBONUS_ELEMENT:
	case APPLY_ATTBONUS_PVP:
	case APPLY_DEFBONUS_PVP:
	case APPLY_ATTBONUS_FIRE:
	case APPLY_ATTBONUS_ICE:
	case APPLY_ATTBONUS_WIND:
	case APPLY_ATTBONUS_EARTH:
	case APPLY_ATTBONUS_DARK:
	case APPLY_ATTBONUS_ELEC:
#endif
		PointChange(aApplyInfo[bApplyType].bPointType, iVal);
		break;

	default:
		sys_err("Unknown apply type %d name %s", bApplyType, GetName());
		break;
	}
}

void CHARACTER::MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion* packet)
{
	packet->header = HEADER_GC_MOTION;
	packet->vid = m_vid;
	packet->motion = motion;

	if (victim)
		packet->victim_vid = victim->GetVID();
	else
		packet->victim_vid = 0;
}

void CHARACTER::Motion(BYTE motion, LPCHARACTER victim)
{
	struct packet_motion pack_motion;
	MotionPacketEncode(motion, victim, &pack_motion);
	PacketAround(&pack_motion, sizeof(struct packet_motion));
}

#ifdef ENABLE_AFK_MODE_SYSTEM
EVENTFUNC(update_character_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "update_character_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;
	
	if (get_dword_time() - ch->GetLastMoveTime() > 60000*3)
	{		
		if (!ch->IsAway())
			ch->SetAway(true);
		
		if (!ch->IsAffectFlag(AFF_AFK))
			ch->AddAffect(AFFECT_AFK, POINT_NONE, 0, AFF_AFK, INFINITE_AFFECT_DURATION, 0, true, true);
		
	}
	
	return PASSES_PER_SEC(15);
}

void CHARACTER::StartUpdateCharacterEvent()
{
	if (m_pkUpdateCharacter)
		return;
	
	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkUpdateCharacter = event_create(update_character_event, info, PASSES_PER_SEC(15));
}
#endif

EVENTFUNC(save_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("save_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) {
		return 0;
	}

	ch->Save();
	ch->FlushDelayedSaveItem();
	return (save_event_second_cycle);
}

void CHARACTER::StartSaveEvent()
{
	if (m_pkSaveEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkSaveEvent = event_create(save_event, info, save_event_second_cycle);
}



void CHARACTER::ChatPacket(BYTE type, const char* format, ...)
{
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	const char* localeFormat;
	if (type != CHAT_TYPE_COMMAND)
		localeFormat = LC_LOCALE_TEXT(format, GetLanguage());
	else
		localeFormat = format;

	if (!localeFormat)
		return;
#endif

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), localeFormat, args);
	va_end(args);
#else
	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);
#endif

	struct packet_chat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(struct packet_chat) + len;
	pack_chat.type = type;
	pack_chat.id = 0;
	pack_chat.bEmpire = d->GetEmpire();

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(chatbuf, len);

	d->Packet(buf.read_peek(), buf.size());
}

#ifdef ENABLE_NEW_CHAT
void CHARACTER::NewChatPacket(WORD chatID, const char* format, ...)
{
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	TPacketNewChat p;
	p.header = HEADER_GC_NEW_CHAT;
	p.size = sizeof(TPacketNewChat) + len;
	p.chatType = 1;
	p.chatID = chatID;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketNewChat));
	buf.write(chatbuf, len);

	d->Packet(buf.read_peek(), buf.size());
}

void CHARACTER::NewChatPacket(WORD chatID)
{
	LPDESC d = GetDesc();

	if (!d)
		return;

	TPacketNewChat p;
	p.header = HEADER_GC_NEW_CHAT;
	p.size = sizeof(TPacketNewChat);
	p.chatType = 1;
	p.chatID = chatID;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketNewChat));

	d->Packet(buf.read_peek(), buf.size());
}

void CHARACTER::QuestChatPacket(WORD chatID, BYTE type, const char* format, ...)
{
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	TPacketNewChat p;
	p.header = HEADER_GC_NEW_CHAT;
	p.size = sizeof(TPacketNewChat) + len;
	p.chatType = type;
	p.chatID = chatID;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketNewChat));
	buf.write(chatbuf, len);

	d->Packet(buf.read_peek(), buf.size());
}

void CHARACTER::QuestChatPacket(WORD chatID, BYTE type)
{
	LPDESC d = GetDesc();

	if (!d)
		return;

	TPacketNewChat p;
	p.header = HEADER_GC_NEW_CHAT;
	p.size = sizeof(TPacketNewChat);
	p.chatType = type;
	p.chatID = chatID;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketNewChat));

	d->Packet(buf.read_peek(), buf.size());
}
#endif

// MINING
void CHARACTER::mining_take()
{
	m_pkMiningEvent = nullptr;
}

void CHARACTER::mining_cancel()
{
	if (m_pkMiningEvent)
	{
		event_cancel(&m_pkMiningEvent);
		NewChatPacket(MINING_STOPPED);
#ifdef ENABLE_MINING_RENEWAL
		EndMiningEvent();
#endif
	}
}

void CHARACTER::mining(LPCHARACTER chLoad)
{
	if (m_pkMiningEvent)
	{
		mining_cancel();
		return;
	}

	if (!chLoad)
	{
		return;
	}

	// @fixme128
	if (GetMapIndex() != chLoad->GetMapIndex() || DISTANCE_APPROX(GetX() - chLoad->GetX(), GetY() - chLoad->GetY()) > 1000)
		return;

	if (mining::GetDropItemFromLod(chLoad->GetRaceNum()) == 0)
		return;

	LPITEM pick = GetWear(WEAR_WEAPON);

	if (!pick || pick->GetType() != ITEM_PICK)
	{
		NewChatPacket(EQUIP_YOUR_PICKAXE_FIRST);
		return;
	}

	if (DISTANCE_APPROX(GetX() - chLoad->GetX(), GetY() - chLoad->GetY()) > 2500)
		return; //fixme@333 pickaxe distance fix

#ifdef ENABLE_MINING_RENEWAL
	const BYTE neededPoint{ mining::GetMiningLodNeededPoint(chLoad->GetRaceNum()) };

	if (neededPoint == 0)
		return;

	ChatPacket(CHAT_TYPE_COMMAND, "MiningRenewalWindow 1 %u", neededPoint);

	int count{15}; // 30 secs

#else
	int count = number(5, 15);
#endif

	TPacketGCDigMotion p;
	p.header = HEADER_GC_DIG_MOTION;
	p.vid = GetVID();
	p.target_vid = chLoad->GetVID();
	p.count = count;

	PacketAround(&p, sizeof(p));

	m_pkMiningEvent = mining::CreateMiningEvent(this, chLoad, count);
}
// END_OF_MINING

// @fixme219 BEGIN
bool CHARACTER::IsNearWater() const
{
	if (!GetSectree())
		return false;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			if (IS_SET(GetSectree()->GetAttribute(GetX() + x * 100, GetY() + y * 100), ATTR_WATER))
				return true;
		}
	}

	return false;
}
// @fixme219 END

void CHARACTER::fishing()
{
	if (m_pkFishingEvent)
	{
		fishing_take();
		return;
	}

	if (!IsNearWater())// @fixme219
		return;

	{
		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

		int	x = GetX();
		int y = GetY();

		LPSECTREE tree = pkSectreeMap->Find(x, y);
		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK))
		{
			NewChatPacket(CANNOT_FISH_HERE);
			return;
		}
	}

	LPITEM rod = GetWear(WEAR_WEAPON);

	if (!rod || rod->GetType() != ITEM_ROD)
	{
		NewChatPacket(EQUIP_YOUR_ROD_FIRST);
		return;
	}

	if (0 == rod->GetSocket(2))
	{
		NewChatPacket(PUT_A_BAIT_TO_YOUR_ROD);
		return;
	}

	float fx, fy;
	GetDeltaByDegree(GetRotation(), 400.0f, &fx, &fy);

	m_pkFishingEvent = fishing::CreateFishingEvent(this);
}

void CHARACTER::fishing_take()
{
	LPITEM rod = GetWear(WEAR_WEAPON);
	if (rod && rod->GetType() == ITEM_ROD)
	{
		using fishing::fishing_event_info;

		if (m_pkFishingEvent)
		{
			struct fishing_event_info* info = dynamic_cast<struct fishing_event_info*>(m_pkFishingEvent->info);

			if (info)
				fishing::Take(info, this);
		}
	}
	else
	{
		NewChatPacket(CANT_FISH_WITHOUT_ROD);
	}

	event_cancel(&m_pkFishingEvent);
}

bool CHARACTER::StartStateMachine(int iNextPulse)
{
	if (CHARACTER_MANAGER::instance().AddToStateList(this))
	{
		m_dwNextStatePulse = thecore_heart->pulse + iNextPulse;
		return true;
	}

	return false;
}

void CHARACTER::StopStateMachine()
{
	CHARACTER_MANAGER::instance().RemoveFromStateList(this);
}

void CHARACTER::UpdateStateMachine(DWORD dwPulse)
{
	if (dwPulse < m_dwNextStatePulse)
		return;

	if (IsDead())
		return;

	Update();
	m_dwNextStatePulse = dwPulse + m_dwStateDuration;
}

void CHARACTER::SetNextStatePulse(int iNextPulse)
{
	CHARACTER_MANAGER::instance().AddToStateList(this);
	m_dwNextStatePulse = iNextPulse;
}

void CHARACTER::UpdateCharacter(DWORD dwPulse)
{
	CFSM::Update();
}

void CHARACTER::SetShop(LPSHOP pkShop)
{
	if ((m_pkShop = pkShop))
		SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
	else
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
		SetShopOwner(NULL);
	}
}

void CHARACTER::SetExchange(CExchange * pkExchange)
{
	m_pkExchange = pkExchange;
}

void CHARACTER::SetPart(BYTE bPartPos, DWORD wVal)
{
	assert(bPartPos < PART_MAX_NUM);
	m_pointsInstant.parts[bPartPos] = wVal;
}

DWORD CHARACTER::GetPart(BYTE bPartPos) const
{
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	if (bPartPos == PART_MAIN && GetWear(WEAR_COSTUME_BODY) && IsCostumeBodyHidden())
	{
		if (const LPITEM pArmor = GetWear(WEAR_BODY))
#ifdef ENABLE_CHANGE_LOOK
			return pArmor->GetTransmutation() != 0 ? pArmor->GetTransmutation() : pArmor->GetVnum();
#else
			return pArmor->GetVnum();
#endif
		else
			return 0;
	}
	else if (bPartPos == PART_HAIR && GetWear(WEAR_COSTUME_HAIR) && IsCostumeHairHidden())
		return 0;
	else if (bPartPos == PART_ACCE && GetWear(WEAR_COSTUME_ACCE) && IsCostumeAcceHidden())
		return 0;
	else if (bPartPos == PART_WEAPON && GetWear(WEAR_COSTUME_WEAPON) && IsCostumeWeaponHidden())
	{
		if (const LPITEM pWeapon = GetWear(WEAR_WEAPON))
#ifdef ENABLE_CHANGE_LOOK
			return pWeapon->GetTransmutation() != 0 ? pWeapon->GetTransmutation() : pWeapon->GetVnum();
#else
			return pWeapon->GetVnum();
#endif
		else
			return 0;
	}
#ifdef ENABLE_AURA_SYSTEM
	else if (bPartPos == PART_AURA && GetWear(WEAR_COSTUME_AURA) && IsCostumeAuraHidden())
		return 0;
#endif
#endif

#ifdef ENABLE_CROWN_SYSTEM
	else if (bPartPos == PART_CROWN && GetWear(WEAR_CROWN) && IsCrownHidden())
		return 0;
#endif

	return m_pointsInstant.parts[bPartPos];
}

DWORD CHARACTER::GetOriginalPart(BYTE bPartPos) const
{
	switch (bPartPos)
	{
	case PART_MAIN:
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		if (GetWear(WEAR_COSTUME_BODY) && IsCostumeBodyHidden())
			if (const LPITEM pArmor = GetWear(WEAR_BODY))
				return pArmor->GetVnum();
#endif
		if (!IsPC())
			return GetPart(PART_MAIN);
		else
			return m_pointsInstant.bBasePart;

	case PART_HAIR:
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		if (GetWear(WEAR_COSTUME_HAIR) && IsCostumeHairHidden())
			return 0;
#endif
		return GetPart(PART_HAIR);

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	case PART_ACCE:
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		if (GetWear(WEAR_COSTUME_ACCE) && IsCostumeAcceHidden())
			return 0;
#endif
		return GetPart(PART_ACCE);
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	case PART_WEAPON:
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		if (GetWear(WEAR_COSTUME_WEAPON) && IsCostumeWeaponHidden())
			if (const LPITEM pWeapon = GetWear(WEAR_WEAPON))
				return pWeapon->GetVnum();
#endif
		return GetPart(PART_WEAPON);
#endif

#ifdef ENABLE_AURA_SYSTEM
		case PART_AURA:
		{
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
			if (GetWear(WEAR_COSTUME_AURA) && IsCostumeAuraHidden())
				return 0;
#endif
			return GetPart(PART_AURA);
		}
#endif
#ifdef ENABLE_CROWN_SYSTEM
		case PART_CROWN:
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
			if (GetWear(WEAR_CROWN) && IsCrownHidden())
				return 0;
#endif
			return GetPart(PART_CROWN);
#endif
	default:
		return 0;
	}
}

BYTE CHARACTER::GetCharType() const
{
	return m_bCharType;
}

bool CHARACTER::SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList)
{
	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		return false;
	// END_OF_TRENT_MONSTER

	if (ch) // @fixme131
	{
		if (!battle_is_attackable(ch, this))
		{
			SendDamagePacket(ch, 0, DAMAGE_BLOCK);
			return false;
		}
	}

	if (ch == this)
	{
		sys_err("SetSyncOwner owner == this (%p)", this);
		return false;
	}

	if (!ch)
	{
		if (bRemoveFromList && m_pkChrSyncOwner)
		{
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
		}
		m_pkChrSyncOwner = NULL;
	}
	else
	{
		if (!IsSyncOwner(ch))
			return false;

		if (DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY()) > 250)
		{
			if (m_pkChrSyncOwner == ch)
				return true;

			return false;
		}

		if (m_pkChrSyncOwner != ch)
		{
			if (m_pkChrSyncOwner)
			{
				m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
			}

			m_pkChrSyncOwner = ch;
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.push_back(this);

			static const timeval zero_tv = { 0, 0 };
			SetLastSyncTime(zero_tv);
		}

		m_fSyncTime = get_float_time();
	}

	TPacketGCOwnership pack;

	pack.bHeader = HEADER_GC_OWNERSHIP;
	pack.dwOwnerVID = ch ? ch->GetVID() : 0;
	pack.dwVictimVID = GetVID();

	PacketAround(&pack, sizeof(TPacketGCOwnership));
	return true;
}

struct FuncClearSync
{
	void operator () (LPCHARACTER ch)
	{
		assert(ch != NULL);
		ch->SetSyncOwner(NULL, false);
	}
};

void CHARACTER::ClearSync()
{
	SetSyncOwner(NULL);

	std::for_each(m_kLst_pkChrSyncOwned.begin(), m_kLst_pkChrSyncOwned.end(), FuncClearSync());
	m_kLst_pkChrSyncOwned.clear();
}

bool CHARACTER::IsSyncOwner(LPCHARACTER ch) const
{
	if (m_pkChrSyncOwner == ch)
		return true;

	if (get_float_time() - m_fSyncTime >= 3.0f)
		return true;

	return false;
}

void CHARACTER::SetParty(LPPARTY pkParty)
{
	if (pkParty == m_pkParty)
		return;

	if (pkParty && m_pkParty)
		sys_err("%s is trying to reassigning party (current %p, new party %p)", GetName(), get_pointer(m_pkParty), get_pointer(pkParty));

	if (m_pkDungeon && IsPC() && !pkParty)
		SetDungeon(NULL);//lightworkcore fix

	m_pkParty = pkParty;

	if (IsPC())
	{
		if (m_pkParty)
			SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);
		else
			REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);

		UpdatePacket();
	}
}

// PARTY_JOIN_BUG_FIX
EVENTINFO(TPartyJoinEventInfo)
{
	DWORD	dwGuestPID;
	DWORD	dwLeaderPID;

	TPartyJoinEventInfo()
		: dwGuestPID(0)
		, dwLeaderPID(0)
	{
	}
};

EVENTFUNC(party_request_event)
{
	TPartyJoinEventInfo* info = dynamic_cast<TPartyJoinEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("party_request_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->dwGuestPID);

	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
		ch->SetPartyRequestEvent(NULL);
	}

	return 0;
}

bool CHARACTER::RequestToParty(LPCHARACTER leader)
{
	if (leader->GetParty())
		leader = leader->GetParty()->GetLeaderCharacter();

	if (!leader)
	{
		NewChatPacket(GROUP_LEADER_IS_NOT_ACTIVE_CANT_DO_THIS);
		return false;
	}

	if (m_pkPartyRequestEvent)
		return false;

	if (!IsPC() || !leader->IsPC())
		return false;

	if (leader->IsBlockMode(BLOCK_PARTY_REQUEST))
		return false;
	
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
	if (rkPlayerBlockMgr.IsPlayerBlock(GetName(), leader->GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, "You cannot send a group request to the player you have blocked.");
		return false;
	}
	else if (rkPlayerBlockMgr.IsPlayerBlock(leader->GetName(), GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, "The player has blocked you.");
		return false;
	}
#endif

	PartyJoinErrCode errcode = IsPartyJoinableCondition(leader, this);

	switch (errcode)
	{
	case PERR_NONE:
		break;

	case PERR_SERVER:
		NewChatPacket(PROBLEM_OCCURED_TRY_AGAIN_LATER);
		return false;

	case PERR_DIFFEMPIRE:
		NewChatPacket(PARTY_REQ_ERR_0);
		return false;

	case PERR_DUNGEON:
		NewChatPacket(PARTY_REQ_ERR_1);
		return false;

	case PERR_OBSERVER:
		NewChatPacket(PARTY_REQ_ERR_2);
		return false;

	case PERR_LVBOUNDARY:
		NewChatPacket(PARTY_REQ_ERR_3);
		return false;

	case PERR_LOWLEVEL:
	case PERR_HILEVEL:
		NewChatPacket(PARTY_REQ_ERR_4);
		return false;

	case PERR_ALREADYJOIN:
		return false;

	case PERR_PARTYISFULL:
		NewChatPacket(PARTY_REQ_ERR_6);
		return false;

	default:
		sys_err("Do not process party join error(%d)", errcode);
		return false;
	}

	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = GetPlayerID();
	info->dwLeaderPID = leader->GetPlayerID();

	SetPartyRequestEvent(event_create(party_request_event, info, PASSES_PER_SEC(10)));

	leader->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequest %u", (DWORD)GetVID());
	NewChatPacket(PARTY_REQ_SENT, "%s", leader->GetName());
	return true;
}

void CHARACTER::DenyToParty(LPCHARACTER member)
{
	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo* info = dynamic_cast<TPartyJoinEventInfo*>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err("CHARACTER::DenyToParty> <Factor> Null pointer");
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

void CHARACTER::AcceptToParty(LPCHARACTER member)
{
	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo* info = dynamic_cast<TPartyJoinEventInfo*>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err("CHARACTER::AcceptToParty> <Factor> Null pointer");
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	if (!GetParty())
		member->NewChatPacket(PARTY_REQ_ERR_8);
	else
	{
		if (GetPlayerID() != GetParty()->GetLeaderPID())
			return;

		PartyJoinErrCode errcode = IsPartyJoinableCondition(this, member);
		switch (errcode)
		{
		case PERR_NONE: 		member->PartyJoin(this); return;
		case PERR_SERVER:		member->NewChatPacket(PROBLEM_OCCURED_TRY_AGAIN_LATER); break;
		case PERR_DUNGEON:		member->NewChatPacket(PARTY_REQ_ERR_1); break;
		case PERR_OBSERVER: 	member->NewChatPacket(PARTY_REQ_ERR_2); break;
		case PERR_LVBOUNDARY:	member->NewChatPacket(PARTY_REQ_ERR_3); break;
		case PERR_LOWLEVEL: 	member->NewChatPacket(PARTY_REQ_ERR_4); break;
		case PERR_HILEVEL: 		member->NewChatPacket(PARTY_REQ_ERR_4); break;
		case PERR_ALREADYJOIN: 	break;
		case PERR_PARTYISFULL: {
			NewChatPacket(PARTY_REQ_ERR_6);
			member->NewChatPacket(PARTY_REQ_ERR_7);
			break;
		}
		default: sys_err("Do not process party join error(%d)", errcode);
		}
	}

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

EVENTFUNC(party_invite_event)
{
	TPartyJoinEventInfo* pInfo = dynamic_cast<TPartyJoinEventInfo*>(event->info);

	if (pInfo == NULL)
	{
		sys_err("party_invite_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER pchInviter = CHARACTER_MANAGER::instance().FindByPID(pInfo->dwLeaderPID);

	if (pchInviter)
	{
		pchInviter->PartyInviteDeny(pInfo->dwGuestPID);
	}

	return 0;
}

void CHARACTER::PartyInvite(LPCHARACTER pchInvitee)
{
	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		NewChatPacket(PARTY_REQ_ERR_9);
		return;
	}
	else if (pchInvitee->IsBlockMode(BLOCK_PARTY_INVITE))
	{
		NewChatPacket(PARTY_REQ_REJECTED, "%s", pchInvitee->GetName());
		return;
	}
	
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
	if (rkPlayerBlockMgr.IsPlayerBlock(GetName(), pchInvitee->GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, "You cannot send a group invite to the player you have blocked.");
		return;
	}
	else if (rkPlayerBlockMgr.IsPlayerBlock(pchInvitee->GetName(), GetName()))
	{
		ChatPacket(CHAT_TYPE_INFO, "The player has blocked you.");
		return;
	}
#endif

	PartyJoinErrCode errcode = IsPartyJoinableCondition(this, pchInvitee);

	switch (errcode)
	{
	case PERR_NONE:
		break;

	case PERR_SERVER:
		NewChatPacket(PROBLEM_OCCURED_TRY_AGAIN_LATER);
		return;

	case PERR_DIFFEMPIRE:
		NewChatPacket(PARTY_REQ_ERR_0);
		return;

	case PERR_DUNGEON:
		NewChatPacket(PARTY_REQ_ERR_1);
		return;

	case PERR_OBSERVER:
		NewChatPacket(PARTY_REQ_ERR_2);
		return;

	case PERR_LVBOUNDARY:
		NewChatPacket(PARTY_REQ_ERR_3);
		return;

	case PERR_LOWLEVEL:
	case PERR_HILEVEL:
		NewChatPacket(PARTY_REQ_ERR_4);
		return;

	case PERR_ALREADYJOIN:
		NewChatPacket(PARTY_REQ_ALREADY_IN_PARTY, "%s", pchInvitee->GetName());
		return;

	case PERR_PARTYISFULL:
		NewChatPacket(PARTY_REQ_ERR_6);
		return;

	default:
		sys_err("Do not process party join error(%d)", errcode);
		return;
	}

	if (m_PartyInviteEventMap.end() != m_PartyInviteEventMap.find(pchInvitee->GetPlayerID()))
		return;

	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = pchInvitee->GetPlayerID();
	info->dwLeaderPID = GetPlayerID();

	m_PartyInviteEventMap.insert(EventMap::value_type(pchInvitee->GetPlayerID(), event_create(party_invite_event, info, PASSES_PER_SEC(10))));

	TPacketGCPartyInvite p;
	p.header = HEADER_GC_PARTY_INVITE;
	p.leader_vid = GetVID();
	pchInvitee->GetDesc()->Packet(&p, sizeof(p));
}

void CHARACTER::PartyInviteAccept(LPCHARACTER pchInvitee)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(pchInvitee->GetPlayerID());

	if (itFind == m_PartyInviteEventMap.end())
	{
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		NewChatPacket(PARTY_REQ_ERR_9);
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableMutableCondition(this, pchInvitee);

	switch (errcode)
	{
	case PERR_NONE:
		break;

	case PERR_SERVER:
		pchInvitee->NewChatPacket(PROBLEM_OCCURED_TRY_AGAIN_LATER);
		return;

	case PERR_DUNGEON:
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_10);
		return;

	case PERR_OBSERVER:
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_2);
		return;

	case PERR_LVBOUNDARY:
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_3);
		return;

	case PERR_LOWLEVEL:
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_4);
		return;

	case PERR_HILEVEL:
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_4);
		return;

	case PERR_ALREADYJOIN:
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_5);
		return;

	case PERR_PARTYISFULL:
		NewChatPacket(PARTY_REQ_ERR_6);
		pchInvitee->NewChatPacket(PARTY_REQ_ERR_7);
		return;

	default:
		sys_err("ignore party join error(%d)", errcode);
		return;
	}

	if (GetParty())
		pchInvitee->PartyJoin(this);
	else
	{
		LPPARTY pParty = CPartyManager::instance().CreateParty(this);

		pParty->Join(pchInvitee->GetPlayerID());
		pParty->Link(pchInvitee);
		pParty->SendPartyInfoAllToOne(this);
	}
}

void CHARACTER::PartyInviteDeny(DWORD dwPID)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(dwPID);

	if (itFind == m_PartyInviteEventMap.end())
	{
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	LPCHARACTER pchInvitee = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (pchInvitee)
		NewChatPacket(PARTY_REQ_REJECTED2, "%s", pchInvitee->GetName());
}

void CHARACTER::PartyJoin(LPCHARACTER pLeader)
{
	pLeader->NewChatPacket(PARTY_JOINED_THE_PARTY, "%s", GetName());
	NewChatPacket(PARTY_JOINED_THE_PARTY2, "%s", pLeader->GetName());

	pLeader->GetParty()->Join(GetPlayerID());
	pLeader->GetParty()->Link(this);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (pchLeader->GetEmpire() != pchGuest->GetEmpire())
		return PERR_DIFFEMPIRE;

	return IsPartyJoinableMutableCondition(pchLeader, pchGuest);
}

static bool __party_can_join_by_level(LPCHARACTER leader, LPCHARACTER quest)
{
	int	level_limit = 30;
	return (abs(leader->GetLevel() - quest->GetLevel()) <= level_limit);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (!CPartyManager::instance().IsEnablePCParty())
		return PERR_SERVER;
	else if (pchLeader->GetDungeon())
		return PERR_DUNGEON;
	else if (pchGuest->IsObserverMode())
		return PERR_OBSERVER;
	else if (false == __party_can_join_by_level(pchLeader, pchGuest))
		return PERR_LVBOUNDARY;
	else if (pchGuest->GetParty())
		return PERR_ALREADYJOIN;
	else if (pchLeader->GetParty())
	{
		if (pchLeader->GetParty()->GetMemberCount() == PARTY_MAX_MEMBER)
			return PERR_PARTYISFULL;
	}

	return PERR_NONE;
}
// END_OF_PARTY_JOIN_BUG_FIX

void CHARACTER::SetDungeon(LPDUNGEON pkDungeon)
{
	if (pkDungeon && m_pkDungeon)
		sys_err("%s is trying to reassigning dungeon (current %p, new party %p)", GetName(), get_pointer(m_pkDungeon), get_pointer(pkDungeon));

	if (m_pkDungeon == pkDungeon) {
		return;
	}

	if (m_pkDungeon)
	{
		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->DecPartyMember(GetParty(), this);
			else
				m_pkDungeon->DecMember(this);
		}
		else if (IsMonster() || IsStone())
		{
			m_pkDungeon->DecMonster();
		}
	}

	m_pkDungeon = pkDungeon;

	if (pkDungeon)
	{
		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->IncPartyMember(GetParty(), this);
			else
				m_pkDungeon->IncMember(this);
		}
		else if (IsMonster() || IsStone())
		{
			m_pkDungeon->IncMonster();
		}
	}
}

void CHARACTER::SetWarMap(CWarMap * pWarMap)
{
	if (m_pWarMap)
		m_pWarMap->DecMember(this);

	m_pWarMap = pWarMap;

	if (m_pWarMap)
		m_pWarMap->IncMember(this);
}

void CHARACTER::SetWeddingMap(marriage::WeddingMap * pMap)
{
	if (m_pWeddingMap)
		m_pWeddingMap->DecMember(this);

	m_pWeddingMap = pMap;

	if (m_pWeddingMap)
		m_pWeddingMap->IncMember(this);
}

void CHARACTER::SetRegen(LPREGEN pkRegen)
{
	m_pkRegen = pkRegen;
	if (pkRegen != NULL) {
		regen_id_ = pkRegen->id;
	}
	m_fRegenAngle = GetRotation();
	m_posRegen = GetXYZ();
}

bool CHARACTER::OnIdle()
{
	return false;
}

void CHARACTER::OnMove(bool bIsAttack)
{
	m_dwLastMoveTime = get_dword_time();

	if (bIsAttack)
	{
		m_dwLastAttackTime = m_dwLastMoveTime;

		if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
			RemoveAffect(AFFECT_REVIVE_INVISIBLE);

		if (IsAffectFlag(AFF_EUNHYUNG))
		{
			RemoveAffect(SKILL_EUNHYUNG);
			SetAffectedEunhyung();
		}
		else
		{
			ClearAffectedEunhyung();
		}

		/*if (IsAffectFlag(AFF_JEONSIN))
		  RemoveAffect(SKILL_JEONSINBANGEO);*/
	}

	/*if (IsAffectFlag(AFF_GUNGON))
	  RemoveAffect(SKILL_GUNGON);*/

	  // MINING
	mining_cancel();
	// END_OF_MINING
}

void CHARACTER::OnClick(LPCHARACTER pkChrCauser)
{
	if (!pkChrCauser)
	{
		sys_err("OnClick %s by NULL", GetName());
		return;
	}

	if (pkChrCauser->GetMyShop() && pkChrCauser != this)
	{
		sys_err("OnClick Fail (%s->%s) - pc has shop", pkChrCauser->GetName(), GetName());
		return;
	}

	if (pkChrCauser->GetExchange())
	{
		sys_err("OnClick Fail (%s->%s) - pc is exchanging", pkChrCauser->GetName(), GetName());
		return;
	}

	if (IsPC())
	{
		if (!CTargetManager::instance().GetTargetInfo(pkChrCauser->GetPlayerID(), TARGET_TYPE_VID, GetVID()))
		{
			if (GetMyShop())
			{
				if (pkChrCauser->IsDead() == true) return;

				//PREVENT_TRADE_WINDOW
				if (pkChrCauser == this)
				{
					if ((GetExchange() || IsOpenSafebox() || GetShopOwner()) || IsCubeOpen() || IsAcceOpened() || IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
						|| isAuraOpened(true) || isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
						|| Is67AttrOpen()
#endif

						)
					{
						pkChrCauser->NewChatPacket(YOU_CANNOT_DO_THIS_WHILE_ANOTHER_WINDOW_IS_OPEN);
						return;
					}
				}
				else
				{
					if ((pkChrCauser->GetExchange() || pkChrCauser->IsOpenSafebox() || pkChrCauser->GetMyShop() || pkChrCauser->GetShopOwner()) || pkChrCauser->IsCubeOpen() || pkChrCauser->IsAcceOpened() || pkChrCauser->IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
						|| isAuraOpened(true) || isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
						|| Is67AttrOpen()
#endif
						)
					{
						pkChrCauser->NewChatPacket(YOU_CANNOT_DO_THIS_WHILE_ANOTHER_WINDOW_IS_OPEN);
						return;
					}

					if ((GetExchange() || IsOpenSafebox() || IsCubeOpen() || IsAcceOpened() || IsOpenOfflineShop())
#ifdef ENABLE_AURA_SYSTEM
						|| isAuraOpened(true) || isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
						|| Is67AttrOpen()
#endif
						)
					{
						pkChrCauser->NewChatPacket(PLAYER_BUSY_RN);
						return;
					}
				}
				//END_PREVENT_TRADE_WINDOW

				if (pkChrCauser->GetShop())
				{
					pkChrCauser->GetShop()->RemoveGuest(pkChrCauser);
					pkChrCauser->SetShop(NULL);
				}

				GetMyShop()->AddGuest(pkChrCauser, GetVID(), false);
				pkChrCauser->SetShopOwner(this);
				return;
			}
			return;
		}
	}

	pkChrCauser->SetQuestNPCID(GetVID());

	if (quest::CQuestManager::instance().Click(pkChrCauser->GetPlayerID(), this))
	{
		return;
	}

	if (!IsPC())
	{
		if (!m_triggerOnClick.pFunc)
		{
			return;
		}
		m_triggerOnClick.pFunc(this, pkChrCauser);
	}
}

BYTE CHARACTER::GetGMLevel() const
{
	if (test_server)
		return GM_IMPLEMENTOR;
	return m_pointsInstant.gm_level;
}

void CHARACTER::SetGMLevel()
{
	if (GetDesc())
	{
		m_pointsInstant.gm_level = gm_get_level(GetName(), GetDesc()->GetHostName(), GetDesc()->GetAccountTable().login);
	}
	else
	{
		m_pointsInstant.gm_level = GM_PLAYER;
	}
}

BOOL CHARACTER::IsGM() const
{
	if (m_pointsInstant.gm_level != GM_PLAYER)
		return true;
	if (test_server)
		return true;
	return false;
}


void CHARACTER::SetStone(LPCHARACTER pkChrStone)
{
	m_pkChrStone = pkChrStone;

	if (m_pkChrStone)
	{
		if (pkChrStone->m_set_pkChrSpawnedBy.find(this) == pkChrStone->m_set_pkChrSpawnedBy.end())
			pkChrStone->m_set_pkChrSpawnedBy.insert(this);
	}
}

struct FuncDeadSpawnedByStone
{
	void operator () (LPCHARACTER ch)
	{
		ch->Dead(NULL);
		ch->SetStone(NULL);
	}
};

void CHARACTER::ClearStone()
{
	if (!m_set_pkChrSpawnedBy.empty())
	{
		FuncDeadSpawnedByStone f;
		std::for_each(m_set_pkChrSpawnedBy.begin(), m_set_pkChrSpawnedBy.end(), f);
		m_set_pkChrSpawnedBy.clear();
	}

	if (!m_pkChrStone)
		return;

	m_pkChrStone->m_set_pkChrSpawnedBy.erase(this);
	m_pkChrStone = NULL;
}

#ifdef ENABLE_SHOW_TARGET_HP
void CHARACTER::ClearTarget()
{
	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);
		m_pkChrTarget = NULL;
	}

	TPacketGCTarget p;
	p.header = HEADER_GC_TARGET;
	p.dwVID = 0;
	p.bHPPercent = 0;
	p.hpTarget = 0;
#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	p.bElement = 0;
#endif
	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();
	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *(it++);
		pkChr->m_pkChrTarget = NULL;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
	}

	m_set_pkChrTargetedBy.clear();
}
#else
void CHARACTER::ClearTarget()
{
	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);
		m_pkChrTarget = NULL;
	}

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = 0;
	p.bHPPercent = 0;
#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	p.bElement = 0;
#endif
	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *(it++);
		pkChr->m_pkChrTarget = NULL;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
	}

	m_set_pkChrTargetedBy.clear();
}
#endif

#ifdef ENABLE_SHOW_TARGET_HP
void CHARACTER::SetTarget(LPCHARACTER pkChrTarget)
{
	if (m_pkChrTarget == pkChrTarget)
		return;

	if (m_pkChrTarget)
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);

	m_pkChrTarget = pkChrTarget;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;

	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.insert(this);

		p.dwVID = m_pkChrTarget->GetVID();

		if ((m_pkChrTarget->IsPC() && !m_pkChrTarget->IsPolymorphed()) || (m_pkChrTarget->GetMaxHP() <= 0)) {
			p.bHPPercent = 0;
			p.hpTarget = 0;
		}
		else
		{
			if (m_pkChrTarget->GetRaceNum() == 20101 ||
				m_pkChrTarget->GetRaceNum() == 20102 ||
				m_pkChrTarget->GetRaceNum() == 20103 ||
				m_pkChrTarget->GetRaceNum() == 20104 ||
				m_pkChrTarget->GetRaceNum() == 20105 ||
				m_pkChrTarget->GetRaceNum() == 20106 ||
				m_pkChrTarget->GetRaceNum() == 20107 ||
				m_pkChrTarget->GetRaceNum() == 20108 ||
				m_pkChrTarget->GetRaceNum() == 20109)
			{
				LPCHARACTER owner = m_pkChrTarget->GetVictim();

				if (owner)
				{
					int iHorseHealth = owner->GetHorseHealth();
					int iHorseMaxHealth = owner->GetHorseMaxHealth();

					if (iHorseMaxHealth) {
						p.bHPPercent = MINMAX(0, iHorseHealth * 100 / iHorseMaxHealth, 100);
						p.hpTarget = iHorseHealth;
					}
					else {
						p.bHPPercent = 100;
						p.hpTarget = 100;
					}
				}
				else {
					p.bHPPercent = 100;
					p.hpTarget = 0;
				}
			}
			else
			{
				if (m_pkChrTarget->GetMaxHP() <= 0) {
					p.bHPPercent = 0;
					p.hpTarget = 0;
				}
				else {
					p.bHPPercent = MINMAX(0, (m_pkChrTarget->GetHP() * 100) / m_pkChrTarget->GetMaxHP(), 100);
					p.hpTarget = m_pkChrTarget->GetHP();
				}
			}
		}
	}
	else
	{
		p.dwVID = 0;
		p.bHPPercent = 0;
		p.hpTarget = 0;
	}

#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	p.bElement = 0;

	if (m_pkChrTarget)
	{
		if (m_pkChrTarget->IsMonster() && m_pkChrTarget->GetMobTable().dwRaceFlag >= RACE_FLAG_ATT_ELEC)
		{
			if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_ELEC)) p.bElement = 1;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_FIRE)) p.bElement = 2;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_ICE)) p.bElement = 3;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_WIND)) p.bElement = 4;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_EARTH)) p.bElement = 5;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_DARK)) p.bElement = 6;
			else p.bElement = 0;
		}
#ifdef ENABLE_PENDANT_SYSTEM
		else if (m_pkChrTarget->IsPC())
		{
			LPITEM pkElement = m_pkChrTarget->GetWear(WEAR_PENDANT);
			if (pkElement)
			{
				TItemTable* pItem = ITEM_MANAGER::instance().GetTable(pkElement->GetVnum());
				const int BASE_ELEMENT = APPLY_ENCHANT_ELECT; // 99
				for (int i = 0; i < ITEM_APPLY_MAX_NUM; i++)
				{
					if (pItem->aApplies[i].bType >= APPLY_ENCHANT_ELECT && pItem->aApplies[i].bType <= APPLY_ENCHANT_DARK)
					{
						p.bElement = pItem->aApplies[i].bType - BASE_ELEMENT + 1;
						break;
					}
				}
			}
		}
		else
		{
			p.bElement = 0;
		}
#endif
	}
#endif

	GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
}
#else
void CHARACTER::SetTarget(LPCHARACTER pkChrTarget)
{
	if (m_pkChrTarget == pkChrTarget)
		return;

	if (m_pkChrTarget)
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);

	m_pkChrTarget = pkChrTarget;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;

	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.insert(this);

		p.dwVID = m_pkChrTarget->GetVID();

		if ((m_pkChrTarget->IsPC() && !m_pkChrTarget->IsPolymorphed()) || (m_pkChrTarget->GetMaxHP() <= 0))
			p.bHPPercent = 0;
		else
		{
			if (m_pkChrTarget->GetRaceNum() == 20101 ||
				m_pkChrTarget->GetRaceNum() == 20102 ||
				m_pkChrTarget->GetRaceNum() == 20103 ||
				m_pkChrTarget->GetRaceNum() == 20104 ||
				m_pkChrTarget->GetRaceNum() == 20105 ||
				m_pkChrTarget->GetRaceNum() == 20106 ||
				m_pkChrTarget->GetRaceNum() == 20107 ||
				m_pkChrTarget->GetRaceNum() == 20108 ||
				m_pkChrTarget->GetRaceNum() == 20109)
			{
				LPCHARACTER owner = m_pkChrTarget->GetVictim();

				if (owner)
				{
					int iHorseHealth = owner->GetHorseHealth();
					int iHorseMaxHealth = owner->GetHorseMaxHealth();

					if (iHorseMaxHealth)
						p.bHPPercent = MINMAX(0, iHorseHealth * 100 / iHorseMaxHealth, 100);
					else
						p.bHPPercent = 100;
				}
				else
					p.bHPPercent = 100;
			}
			else
			{
				if (m_pkChrTarget->GetMaxHP() <= 0) // @fixme136
					p.bHPPercent = 0;
				else
					p.bHPPercent = MINMAX(0, (m_pkChrTarget->GetHP() * 100) / m_pkChrTarget->GetMaxHP(), 100);
			}
		}
	}
	else
	{
		p.dwVID = 0;
		p.bHPPercent = 0;
	}

#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	p.bElement = 0;

	if (m_pkChrTarget)
	{
		if (m_pkChrTarget->IsMonster() && m_pkChrTarget->GetMobTable().dwRaceFlag >= RACE_FLAG_ATT_ELEC)
		{
			if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_ELEC)) p.bElement = 1;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_FIRE)) p.bElement = 2;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_ICE)) p.bElement = 3;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_WIND)) p.bElement = 4;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_EARTH)) p.bElement = 5;
			else if (m_pkChrTarget->IsRaceFlag(RACE_FLAG_ATT_DARK)) p.bElement = 6;
			else p.bElement = 0;
		}
#if defined(__PENDANT_SYSTEM__)
		else if (m_pkChrTarget->IsPC())
		{
			LPITEM pkElement = m_pkChrTarget->GetWear(WEAR_PENDANT);
			if (pkElement)
			{
				TItemTable* pItem = ITEM_MANAGER::instance().GetTable(pkElement->GetVnum());
				const int BASE_ELEMENT = APPLY_ENCHANT_ELECT; // 99
				for (int i = 0; i < ITEM_APPLY_MAX_NUM; i++)
				{
					if (pItem->aApplies[i].bType >= APPLY_ENCHANT_ELECT && pItem->aApplies[i].bType <= APPLY_ENCHANT_DARK)
					{
						p.bElement = pItem->aApplies[i].bType - BASE_ELEMENT + 1;
						break;
					}
				}
			}
		}
		else
		{
			p.bElement = 0;
		}
#endif
	}
#endif

	GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
}
#endif

#ifdef ENABLE_SHOW_TARGET_HP
void CHARACTER::BroadcastTargetPacket()
{
	if (m_set_pkChrTargetedBy.empty())
		return;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = GetVID();

	if (GetMaxHP() <= 0)
	{
		p.bHPPercent = 0;
		p.hpTarget = 0;
	}
	else
	{
		if (IsPC())
		{
			p.bHPPercent = 0;
			p.hpTarget = 0;
		}
		else
		{
			//@fix overflow int data type
			p.bHPPercent = MINMAX(0, (GetHP() * 100) / GetMaxHP(), 100);
			p.hpTarget = GetHP();
		}
	}

#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	p.bElement = 0;
#endif

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *it++;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
	}
}
#else
void CHARACTER::BroadcastTargetPacket()
{
	if (m_set_pkChrTargetedBy.empty())
		return;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = GetVID();

	if (IsPC())
		p.bHPPercent = 0;
	else if (GetMaxHP() <= 0) // @fixme136
		p.bHPPercent = 0;
	else
		p.bHPPercent = MINMAX(0, (GetHP() * 100) / GetMaxHP(), 100);

#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	p.bElement = 0;
#endif

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *it++;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
	}
}
#endif

void CHARACTER::CheckTarget()
{
	if (!m_pkChrTarget)
		return;

	if (DISTANCE_APPROX(GetX() - m_pkChrTarget->GetX(), GetY() - m_pkChrTarget->GetY()) >= 4800)
		SetTarget(NULL);
}

void CHARACTER::SetWarpLocation(long lMapIndex, long x, long y)
{
	m_posWarp.x = x * 100;
	m_posWarp.y = y * 100;
	m_lWarpMapIndex = lMapIndex;
}

void CHARACTER::SaveExitLocation()
{
	m_posExit = GetXYZ();
	m_lExitMapIndex = GetMapIndex();
}

void CHARACTER::ExitToSavedLocation()
{
	WarpSet(m_posWarp.x, m_posWarp.y, m_lWarpMapIndex);

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;
}

#ifdef ENABLE_P2P_WARP
bool CHARACTER::WarpSet(long x, long y, long lPrivateMapIndex)
{
	if (!IsPC())
	{
		return false;
	}

	long lAddr;
	long lMapIndex;
	uint16_t wPort;

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
	{
		sys_err("cannot find map location index %d x %d y %d name %s", lMapIndex, x, y, GetName());
		return false;
	}

	return WarpSet(x, y, lPrivateMapIndex, lMapIndex, lAddr, wPort);
}

bool CHARACTER::WarpSet(long x, long y, long lPrivateMapIndex, long lMapIndex, long lAddr, WORD wPort)
{
	{
		long lCurAddr;
		long lCurMapIndex = 0;
		WORD wCurPort;

		CMapLocation::instance().Get(GetX(), GetY(), lCurMapIndex, lCurAddr, wCurPort);
	}

	if (lPrivateMapIndex >= 10000)
	{
		if (lPrivateMapIndex / 10000 != lMapIndex)
		{
			sys_err ("Invalid map index %d, must be child of %d", lPrivateMapIndex, lMapIndex);
			return false;
		}

		lMapIndex = lPrivateMapIndex;
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	TPacketGCWarp p;
	p.bHeader = HEADER_GC_WARP;
	p.lX = x;
	p.lY = y;
	p.lAddr = lAddr;
	p.wPort = wPort;

#ifdef ENABLE_SWITCHBOT
	CSwitchbotManager::Instance().SetIsWarping(GetPlayerID(), true);

	if (p.wPort != mother_port)
	{
		CSwitchbotManager::Instance().P2PSendSwitchbot(GetPlayerID(), p.wPort);
	}
#endif
#ifdef ENABLE_FARM_BLOCK
	SetWarpCheck(true);
#endif
	GetDesc()->Packet(&p, sizeof(TPacketGCWarp));
	return true;
}

#else
bool CHARACTER::WarpSet(long x, long y, long lPrivateMapIndex)
{
	if (!IsPC())
		return false;

	long lAddr;
	long lMapIndex;
	WORD wPort;

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
	{
		sys_err("cannot find map location index %d x %d y %d name %s", lMapIndex, x, y, GetName());
		return false;
	}

	{
		long lCurAddr;
		long lCurMapIndex = 0;
		WORD wCurPort;

		CMapLocation::instance().Get(GetX(), GetY(), lCurMapIndex, lCurAddr, wCurPort);
	}

	if (lPrivateMapIndex >= 10000)
	{
		if (lPrivateMapIndex / 10000 != lMapIndex)
		{
			sys_err("Invalid map index %d, must be child of %d", lPrivateMapIndex, lMapIndex);
			return false;
		}

		lMapIndex = lPrivateMapIndex;
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();

		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	TPacketGCWarp p;

	p.bHeader = HEADER_GC_WARP;
	p.lX = x;
	p.lY = y;
	p.lAddr = lAddr;
#ifdef ENABLE_NEWSTUFF
	if (!g_stProxyIP.empty())
		p.lAddr = inet_addr(g_stProxyIP.c_str());
#endif
	p.wPort = wPort;

#ifdef ENABLE_SWITCHBOT
	CSwitchbotManager::Instance().SetIsWarping(GetPlayerID(), true);

	if (p.wPort != mother_port)
	{
		CSwitchbotManager::Instance().P2PSendSwitchbot(GetPlayerID(), p.wPort);
	}
#endif
#ifdef ENABLE_FARM_BLOCK
	SetWarpCheck(true);
#endif

	GetDesc()->Packet(&p, sizeof(TPacketGCWarp));
	return true;
}
#endif

#define ENABLE_GOHOME_IF_MAP_NOT_ALLOWED
void CHARACTER::WarpEnd()
{
	if (m_posWarp.x == 0 && m_posWarp.y == 0)
		return;

	int index = m_lWarpMapIndex;

	if (index > 10000)
		index /= 10000;

	if (!map_allow_find(index))
	{
		sys_err("location %d %d not allowed to login this server", m_posWarp.x, m_posWarp.y);
#ifdef ENABLE_GOHOME_IF_MAP_NOT_ALLOWED
		GoHome();
#else
		GetDesc()->SetPhase(PHASE_CLOSE);
#endif
		return;
	}

	Show(m_lWarpMapIndex, m_posWarp.x, m_posWarp.y, 0);
	Stop();

	m_lWarpMapIndex = 0;
	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;

#ifdef ENABLE_FARM_BLOCK
	SetWarpCheck(false);
#endif

	{
		// P2P Login
		TPacketGGLogin p;

		p.bHeader = HEADER_GG_LOGIN;
		strlcpy(p.szName, GetName(), sizeof(p.szName));
		p.dwPID = GetPlayerID();
		p.bEmpire = GetEmpire();
		p.lMapIndex = SECTREE_MANAGER::instance().GetMapIndex(GetX(), GetY());
		p.bChannel = g_bChannel;
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		p.bLanguage = GetLanguage();
#endif
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogin));
	}
}

bool CHARACTER::Return()
{
	if (!IsNPC())
		return false;

	int x, y;
	SetVictim(NULL);

	x = m_pkMobInst->m_posLastAttacked.x;
	y = m_pkMobInst->m_posLastAttacked.y;

	SetRotationToXY(x, y);

	if (!Goto(x, y))
		return false;

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	if (GetParty())
		GetParty()->SendMessage(this, PM_RETURN, x, y);

	return true;
}

bool CHARACTER::Follow(LPCHARACTER pkChr, float fMinDistance)
{
	if (IsPC())
	{
		sys_err("CHARACTER::Follow : PC cannot use this method", GetName());
		return false;
	}


	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (pkChr->IsPC())
		{
			// If i'm in a party. I must obey party leader's AI.
			if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
			{
				if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000)
				{
					if (m_pkMobData->m_table.wAttackRange < DISTANCE_APPROX(pkChr->GetX() - GetX(), pkChr->GetY() - GetY()))
						if (Return())
							return true;
				}
			}
		}
		return false;
	}
	// END_OF_TRENT_MONSTER

	long x = pkChr->GetX();
	long y = pkChr->GetY();

	if (pkChr->IsPC())
	{
		// If i'm in a party. I must obey party leader's AI.
		if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
		{
			if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000)
			{
				if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
					if (Return())
						return true;
			}
		}
	}

	if (pkChr->IsState(pkChr->m_stateMove) &&
		GetMobBattleType() != BATTLE_TYPE_RANGE &&
		GetMobBattleType() != BATTLE_TYPE_MAGIC &&
		false == IsPet()
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		&& false == IsMount()
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		&& false == IsNewPet()
#endif
#ifdef ENABLE_BUFFI_SYSTEM
		&& false == IsBuffi()
#endif
		)
	{
		float rot = pkChr->GetRotation();
		float rot_delta = GetDegreeDelta(rot, GetDegreeFromPositionXY(GetX(), GetY(), pkChr->GetX(), pkChr->GetY()));

		float yourSpeed = pkChr->GetMoveSpeed();
		float mySpeed = GetMoveSpeed();

		float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());
		float fFollowSpeed = mySpeed - yourSpeed * cos(rot_delta * M_PI / 180);

		if (fFollowSpeed >= 0.1f)
		{
			float fMeetTime = fDist / fFollowSpeed;
			float fYourMoveEstimateX, fYourMoveEstimateY;

			if (fMeetTime * yourSpeed <= 100000.0f)
			{
				GetDeltaByDegree(pkChr->GetRotation(), fMeetTime * yourSpeed, &fYourMoveEstimateX, &fYourMoveEstimateY);

				x += (long)fYourMoveEstimateX;
				y += (long)fYourMoveEstimateY;

				float fDistNew = sqrt(((double)x - GetX()) * (x - GetX()) + ((double)y - GetY()) * (y - GetY()));
				if (fDist < fDistNew)
				{
					x = (long)(GetX() + (x - GetX()) * fDist / fDistNew);
					y = (long)(GetY() + (y - GetY()) * fDist / fDistNew);
				}
			}
		}
	}

	SetRotationToXY(x, y);

	float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());

	if (fDist <= fMinDistance)
		return false;

	float fx, fy;

	if (IsChangeAttackPosition(pkChr) && GetMobRank() < MOB_RANK_BOSS)
	{
		SetChangeAttackPositionTime();

		int retry = 16;
		int dx, dy;
		int rot = (int)GetDegreeFromPositionXY(x, y, GetX(), GetY());

		while (--retry)
		{
			if (fDist < 500.0f)
				GetDeltaByDegree((rot + number(-90, 90) + number(-90, 90)) % 360, fMinDistance, &fx, &fy);
			else
				GetDeltaByDegree(number(0, 359), fMinDistance, &fx, &fy);

			dx = x + (int)fx;
			dy = y + (int)fy;

			LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), dx, dy);

			if (NULL == tree)
				break;

			if (0 == (tree->GetAttribute(dx, dy) & (ATTR_BLOCK | ATTR_OBJECT)))
				break;
		}

		if (!Goto(dx, dy))
			return false;
	}
	else
	{
		float fDistToGo = fDist - fMinDistance;
		GetDeltaByDegree(GetRotation(), fDistToGo, &fx, &fy);

		if (!Goto(GetX() + (int)fx, GetY() + (int)fy))
			return false;
	}

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	return true;
}

float CHARACTER::GetDistanceFromSafeboxOpen() const
{
	return DISTANCE_APPROX(GetX() - m_posSafeboxOpen.x, GetY() - m_posSafeboxOpen.y);
}

void CHARACTER::SetSafeboxOpenPosition()
{
	m_posSafeboxOpen = GetXYZ();
}

CSafebox* CHARACTER::GetSafebox() const
{
	return m_pkSafebox;
}

void CHARACTER::ReqSafeboxLoad(const char* pszPassword)
{
	if (!*pszPassword || strlen(pszPassword) > SAFEBOX_PASSWORD_MAX_LEN)
	{
		NewChatPacket(SAFEBOX_ERR);
		return;
	}
	else if (m_pkSafebox)
	{
		NewChatPacket(SAFEBOX_ERR_1);
		return;
	}

	int iPulse = thecore_pulse();

	if (iPulse - GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(10))
	{
		NewChatPacket(SAFEBOX_ERR_2);
		return;
	}
	else if (m_bOpeningSafebox)
	{
		return;
	}

	SetActivateTime(SAFEBOX_CHECK_TIME);
	m_bOpeningSafebox = true;

	TSafeboxLoadPacket p;
	p.dwID = GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, pszPassword, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_LOAD, GetDesc()->GetHandle(), &p, sizeof(p));
}

void CHARACTER::LoadSafebox(int iSize, DWORD dwGold, int iItemCount, TPlayerItem * pItems)
{
	bool bLoaded = false;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(true);
	//END_PREVENT_TRADE_WINDOW

	if (m_pkSafebox)
		bLoaded = true;

	if (!m_pkSafebox)
		m_pkSafebox = M2_NEW CSafebox(this, iSize, dwGold);
	else
		m_pkSafebox->ChangeSize(iSize);

	m_iSafeboxSize = iSize;

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_SAFEBOX_SIZE;
	p.bSize = iSize;

	GetDesc()->Packet(&p, sizeof(TPacketCGSafeboxSize));

	if (!bLoaded)
	{
		for (int i = 0; i < iItemCount; ++i, ++pItems)
		{
			if (!m_pkSafebox->IsValidPosition(pItems->pos))
				continue;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id);

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", pItems->vnum, pItems->id, GetName());
				continue;
			}

			item->SetSkipSave(true);
			item->SetSockets(pItems->alSockets);
			item->SetAttributes(pItems->aAttr);

			if (!m_pkSafebox->Add(pItems->pos, item))
			{
				M2_DESTROY_ITEM(item);
			}
			else
			{
				item->OnAfterCreatedItem();
				item->SetSkipSave(false);
			}
		}
	}
}

void CHARACTER::ChangeSafeboxSize(BYTE bSize)
{
	//if (!m_pkSafebox)
	//return;
	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_SAFEBOX_SIZE;
	p.bSize = bSize;

	GetDesc()->Packet(&p, sizeof(TPacketCGSafeboxSize));

	if (m_pkSafebox)
		m_pkSafebox->ChangeSize(bSize);

	m_iSafeboxSize = bSize;
}

void CHARACTER::CloseSafebox()
{
	if (!m_pkSafebox)
		return;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(false);
	//END_PREVENT_TRADE_WINDOW

	m_pkSafebox->Save();

	M2_DELETE(m_pkSafebox);
	m_pkSafebox = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseSafebox");

	SetActivateTime(SAFEBOX_CHECK_TIME);
	m_bOpeningSafebox = false;

	Save();
}

CSafebox* CHARACTER::GetMall() const
{
	return m_pkMall;
}

void CHARACTER::LoadMall(int iItemCount, TPlayerItem * pItems)
{
	bool bLoaded = false;

	if (m_pkMall)
		bLoaded = true;

	if (!m_pkMall)
		m_pkMall = M2_NEW CSafebox(this, 3 * SAFEBOX_PAGE_SIZE, 0);
	else
		m_pkMall->ChangeSize(3 * SAFEBOX_PAGE_SIZE);

	m_pkMall->SetWindowMode(MALL);

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_MALL_OPEN;
	p.bSize = 3 * SAFEBOX_PAGE_SIZE;

	GetDesc()->Packet(&p, sizeof(TPacketCGSafeboxSize));

	if (!bLoaded)
	{
		for (int i = 0; i < iItemCount; ++i, ++pItems)
		{
			if (!m_pkMall->IsValidPosition(pItems->pos))
				continue;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id);

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", pItems->vnum, pItems->id, GetName());
				continue;
			}

			item->SetSkipSave(true);
			item->SetSockets(pItems->alSockets);
			item->SetAttributes(pItems->aAttr);

			if (!m_pkMall->Add(pItems->pos, item))
				M2_DESTROY_ITEM(item);
			else
				item->SetSkipSave(false);
		}
	}
}

void CHARACTER::CloseMall()
{
	if (!m_pkMall)
		return;

	m_pkMall->Save();

	M2_DELETE(m_pkMall);
	m_pkMall = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseMall");
}

bool CHARACTER::BuildUpdatePartyPacket(TPacketGCPartyUpdate & out)
{
	if (!GetParty())
		return false;

	memset(&out, 0, sizeof(out));

	out.header = HEADER_GC_PARTY_UPDATE;
	out.pid = GetPlayerID();
	if (GetMaxHP() <= 0) // @fixme136
		out.percent_hp = 0;
	else
		out.percent_hp = MINMAX(0, GetHP() * 100 / GetMaxHP(), 100);
	out.role = GetParty()->GetRole(GetPlayerID());

	LPCHARACTER l = GetParty()->GetLeaderCharacter();

	if (l && DISTANCE_APPROX(GetX() - l->GetX(), GetY() - l->GetY()) < PARTY_DEFAULT_RANGE)
	{
		out.affects[0] = GetParty()->GetPartyBonusExpPercent();
		out.affects[1] = GetPoint(POINT_PARTY_ATTACKER_BONUS);
		out.affects[2] = GetPoint(POINT_PARTY_TANKER_BONUS);
		out.affects[3] = GetPoint(POINT_PARTY_BUFFER_BONUS);
		out.affects[4] = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
		out.affects[5] = GetPoint(POINT_PARTY_HASTE_BONUS);
		out.affects[6] = GetPoint(POINT_PARTY_DEFENDER_BONUS);
	}

	return true;
}

int CHARACTER::GetLeadershipSkillLevel() const
{
	return GetSkillLevel(SKILL_LEADERSHIP);
}

void CHARACTER::QuerySafeboxSize()
{
	if (m_iSafeboxSize == -1)
	{
		DBManager::instance().ReturnQuery(QID_SAFEBOX_SIZE,
			GetPlayerID(),
			NULL,
			"SELECT size FROM safebox%s WHERE account_id = %u",
			get_table_postfix(),
			GetDesc()->GetAccountTable().id);
	}
}

void CHARACTER::SetSafeboxSize(int iSize)
{
	m_iSafeboxSize = iSize;
	DBManager::instance().Query("UPDATE safebox%s SET size = %d WHERE account_id = %u", get_table_postfix(), iSize / SAFEBOX_PAGE_SIZE, GetDesc()->GetAccountTable().id);
}

int CHARACTER::GetSafeboxSize() const
{
	return m_iSafeboxSize;
}

void CHARACTER::SetNowWalking(bool bWalkFlag)
{
	//if (m_bNowWalking != bWalkFlag || IsNPC())
	if (m_bNowWalking != bWalkFlag)
	{
		if (bWalkFlag)
		{
			m_bNowWalking = true;
			m_dwWalkStartTime = get_dword_time();
		}
		else
		{
			m_bNowWalking = false;
		}

		//if (m_bNowWalking)
		{
			TPacketGCWalkMode p;
			p.vid = GetVID();
			p.header = HEADER_GC_WALK_MODE;
			p.mode = m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;

			PacketView(&p, sizeof(p));
		}
	}
}

void CHARACTER::StartStaminaConsume()
{
	if (m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = true;
	if (IsStaminaHalfConsume())
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec / 2, GetStamina());
	else
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
}

void CHARACTER::StopStaminaConsume()
{
	if (!m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = false;
	ChatPacket(CHAT_TYPE_COMMAND, "StopStaminaConsume %d", GetStamina());
}

bool CHARACTER::IsStaminaConsume() const
{
	return m_bStaminaConsume;
}

bool CHARACTER::IsStaminaHalfConsume() const
{
	return IsEquipUniqueItem(UNIQUE_ITEM_HALF_STAMINA);
}

void CHARACTER::ResetStopTime()
{
	m_dwStopTime = get_dword_time();
}

DWORD CHARACTER::GetStopTime() const
{
	return m_dwStopTime;
}

void CHARACTER::ResetPoint(int iLv)
{
	BYTE bJob = GetJob();

	PointChange(POINT_LEVEL, iLv - GetLevel());

	SetRealPoint(POINT_ST, JobInitialPoints[bJob].st);
	SetPoint(POINT_ST, GetRealPoint(POINT_ST));

	SetRealPoint(POINT_HT, JobInitialPoints[bJob].ht);
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));

	SetRealPoint(POINT_DX, JobInitialPoints[bJob].dx);
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));

	SetRealPoint(POINT_IQ, JobInitialPoints[bJob].iq);
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	SetRandomHP((iLv - 1) * number(JobInitialPoints[GetJob()].hp_per_lv_begin, JobInitialPoints[GetJob()].hp_per_lv_end));
	SetRandomSP((iLv - 1) * number(JobInitialPoints[GetJob()].sp_per_lv_begin, JobInitialPoints[GetJob()].sp_per_lv_end));

	// @fixme104
	PointChange(POINT_STAT, (MINMAX(1, iLv, g_iStatusPointGetLevelLimit) * 3) + GetPoint(POINT_LEVEL_STEP) - GetPoint(POINT_STAT));

	ComputePoints();

	PointChange(POINT_HP, GetMaxHP() - GetHP());
	PointChange(POINT_SP, GetMaxSP() - GetSP());

	PointsPacket();
}

bool CHARACTER::IsChangeAttackPosition(LPCHARACTER target) const
{
	if (!IsNPC())
		return true;

	DWORD dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_NEAR;

	if (DISTANCE_APPROX(GetX() - target->GetX(), GetY() - target->GetY()) >
		AI_CHANGE_ATTACK_POISITION_DISTANCE + GetMobAttackRange())
		dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_FAR;

	return get_dword_time() - m_dwLastChangeAttackPositionTime > dwChangeTime;
}

void CHARACTER::GiveRandomSkillBook()
{
	LPITEM item = AutoGiveItem(50300);

	if (NULL != item)
	{
		extern const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);
		DWORD dwSkillVnum = 0;
		// 50% of getting random books or getting one of the same player's race
		if (!number(0, 1))
			dwSkillVnum = GetRandomSkillVnum(GetJob());
		else
			dwSkillVnum = GetRandomSkillVnum();
		item->SetSocket(0, dwSkillVnum);
	}
}

void CHARACTER::ReviveInvisible(int iDur)
{
	AddAffect(AFFECT_REVIVE_INVISIBLE, POINT_NONE, 0, AFF_REVIVE_INVISIBLE, iDur, 0, true);
}

void CHARACTER::SetGuild(CGuild * pGuild)
{
	if (m_pGuild != pGuild)
	{
		m_pGuild = pGuild;
		UpdatePacket();
	}
}

void CHARACTER::BeginStateEmpty()
{
	return;
}

void CHARACTER::EffectPacket(int enumEffectType)
{
	TPacketGCSpecialEffect p;

	p.header = HEADER_GC_SEPCIAL_EFFECT;
	p.type = enumEffectType;
	p.vid = GetVID();

	PacketAround(&p, sizeof(TPacketGCSpecialEffect));
}

void CHARACTER::SpecificEffectPacket(const char filename[MAX_EFFECT_FILE_NAME])
{
	TPacketGCSpecificEffect p;

	p.header = HEADER_GC_SPECIFIC_EFFECT;
	p.vid = GetVID();
	memcpy(p.effect_file, filename, MAX_EFFECT_FILE_NAME);

	PacketAround(&p, sizeof(TPacketGCSpecificEffect));
}

void CHARACTER::SpecificEffectPacket(BYTE type)
{
	TPacketGCSpecificEffect2 p;

	p.header = HEADER_GC_SPECIFIC_EFFECT2;
	p.vid = GetVID();
	p.type = type;

	PacketAround(&p, sizeof(TPacketGCSpecificEffect2));
}

void CHARACTER::MonsterChat(BYTE bMonsterChatType)
{
	if (IsPC())
		return;

	char sbuf[256 + 1];

	if (IsMonster())
	{
		if (number(0, 60))
			return;

		snprintf(sbuf, sizeof(sbuf),
			"(locale.monster_chat[%i] and locale.monster_chat[%i][%lld] or '')",
			GetRaceNum(), GetRaceNum(), bMonsterChatType * 3 + number(1, 3));
	}
	else
	{
		if (bMonsterChatType != MONSTER_CHAT_WAIT)
			return;

		if (IsGuardNPC())
		{
			if (number(0, 6))
				return;
		}
		else
		{
			if (number(0, 30))
				return;
		}

		snprintf(sbuf, sizeof(sbuf), "(locale.monster_chat[%i] and locale.monster_chat[%i][number(1, table.getn(locale.monster_chat[%i]))] or '')", GetRaceNum(), GetRaceNum(), GetRaceNum());
	}

	std::string text = quest::ScriptToString(sbuf);

	if (text.empty())
		return;

	struct packet_chat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(struct packet_chat) + text.size() + 1;
	pack_chat.type = CHAT_TYPE_TALKING;
	pack_chat.id = GetVID();
	pack_chat.bEmpire = 0;

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(text.c_str(), text.size() + 1);

	PacketAround(buf.read_peek(), buf.size());
}

void CHARACTER::SetQuestNPCID(DWORD vid)
{
	m_dwQuestNPCVID = vid;
}

LPCHARACTER CHARACTER::GetQuestNPC() const
{
	return CHARACTER_MANAGER::instance().Find(m_dwQuestNPCVID);
}

void CHARACTER::SetQuestItemPtr(LPITEM item)
{
	m_pQuestItem = item;
}

void CHARACTER::ClearQuestItemPtr()
{
	m_pQuestItem = NULL;
}

LPITEM CHARACTER::GetQuestItemPtr() const
{
	return m_pQuestItem;
}

#ifdef ENABLE_QUEST_DND_EVENT
void CHARACTER::SetQuestDNDItemPtr(LPITEM item)
{
	m_pQuestDNDItem = item;
}

void CHARACTER::ClearQuestDNDItemPtr()
{
	m_pQuestDNDItem = NULL;
}

LPITEM CHARACTER::GetQuestDNDItemPtr() const
{
	return m_pQuestDNDItem;
}
#endif

LPDUNGEON CHARACTER::GetDungeonForce() const
{
	if (m_lWarpMapIndex > 10000)
		return CDungeonManager::instance().FindByMapIndex(m_lWarpMapIndex);

	return m_pkDungeon;
}

void CHARACTER::SetBlockMode(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;

	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);

	SetQuestFlag("game_option.block_exchange", bFlag & BLOCK_EXCHANGE ? 1 : 0);
	SetQuestFlag("game_option.block_party_invite", bFlag & BLOCK_PARTY_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_guild_invite", bFlag & BLOCK_GUILD_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_whisper", bFlag & BLOCK_WHISPER ? 1 : 0);
	SetQuestFlag("game_option.block_messenger_invite", bFlag & BLOCK_MESSENGER_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_party_request", bFlag & BLOCK_PARTY_REQUEST ? 1 : 0);
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	SetQuestFlag("game_option.block_warp_request", bFlag & BLOCK_WARP_REQUEST ? 1 : 0);
#endif
}

void CHARACTER::SetBlockModeForce(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;
	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);
}

bool CHARACTER::IsGuardNPC() const
{
	return IsNPC() && (GetRaceNum() == 11000 || GetRaceNum() == 11002 || GetRaceNum() == 11004);
}

int CHARACTER::GetPolymorphPower() const
{
	return aiPolymorphPowerByLevel[MINMAX(0, GetSkillLevel(SKILL_POLYMORPH), 40)];
}

void CHARACTER::SetPolymorph(DWORD dwRaceNum, bool bMaintainStat)
{
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (dwRaceNum < MAIN_RACE_MAX_NUM)
#else
	if (dwRaceNum < JOB_MAX_NUM)
#endif
	{
		dwRaceNum = 0;
		bMaintainStat = false;
	}

	if (m_dwPolymorphRace == dwRaceNum)
		return;

	m_bPolyMaintainStat = bMaintainStat;
	m_dwPolymorphRace = dwRaceNum;

	if (dwRaceNum != 0)
		StopRiding();

	SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
	m_afAffectFlag.Set(AFF_SPAWN);

	ViewReencode();

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	if (!bMaintainStat)
	{
		PointChange(POINT_ST, 0);
		PointChange(POINT_DX, 0);
		PointChange(POINT_IQ, 0);
		PointChange(POINT_HT, 0);
	}
	ComputeBattlePoints();
}

int CHARACTER::GetQuestFlag(const std::string & flag) const
{
	// @Lightwork#94 BEGIN
	if (!IsPC())
	{
		sys_err("Trying to get qf %s from non player character", flag.c_str());
		return 0;
	}
	// @Lightwork#94 END
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());

	// @Lightwork#94 BEGIN
	if (!pPC)
	{
		//sys_err("Nullpointer when trying to access questflag %s for player with pid %u", flag.c_str(), pid);
		return 0;
	}
	// @Lightwork#94 END

	return pPC->GetFlag(flag);
}

void CHARACTER::SetQuestFlag(const std::string & flag, int value)
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	// @Lightwork#94 BEGIN
	if (!pPC)
	{
		//sys_err("Nullpointer when trying to set questflag %s for player with pid %u", flag.c_str(), pid);
		return;
	}
	// @Lightwork#94 END
	pPC->SetFlag(flag, value);
}

void CHARACTER::DetermineDropMetinStone()
{
	static const DWORD c_adwMetin[] =
	{
		28012,
		28030,
		28031,
		28032,
		28033,
		28034,
		28035,
		28036,
		28037,
		28038,
		28039,
		28040,
		28041,
		28042,
		28043,
#if defined(ENABLE_MAGIC_REDUCTION_SYSTEM) && defined(USE_MAGIC_REDUCTION_STONES)
		28044,
		28045,
#endif
	};
	DWORD stone_num = GetRaceNum();
	int idx = std::lower_bound(aStoneDrop, aStoneDrop + STONE_INFO_MAX_NUM, stone_num) - aStoneDrop;
	if (idx >= STONE_INFO_MAX_NUM || aStoneDrop[idx].dwMobVnum != stone_num)
	{
		m_dwDropMetinStone = 0;
	}
	else
	{
		const SStoneDropInfo& info = aStoneDrop[idx];
		m_bDropMetinStonePct = info.iDropPct;
		{
			m_dwDropMetinStone = c_adwMetin[number(0, sizeof(c_adwMetin) / sizeof(DWORD) - 1)];
			int iGradePct = number(1, 100);
			for (int iStoneLevel = 0; iStoneLevel < STONE_LEVEL_MAX_NUM; iStoneLevel++)
			{
				int iLevelGradePortion = info.iLevelPct[iStoneLevel];
				if (iGradePct <= iLevelGradePortion)
				{
					break;
				}
				else
				{
					iGradePct -= iLevelGradePortion;
					m_dwDropMetinStone += 100;
				}
			}
		}
	}
}

void CHARACTER::SendEquipment(LPCHARACTER ch)
{
	TPacketViewEquip p;
	p.header = HEADER_GC_VIEW_EQUIP;
	p.vid = GetVID();
	for (int i = 0; i < WEAR_MAX_NUM; i++)
	{
		LPITEM item = GetWear(i);
		if (item)
		{
			p.equips[i].vnum = item->GetVnum();
			p.equips[i].count = item->GetCount();

			thecore_memcpy(p.equips[i].alSockets, item->GetSockets(), sizeof(p.equips[i].alSockets));
			thecore_memcpy(p.equips[i].aAttr, item->GetAttributes(), sizeof(p.equips[i].aAttr));
		}
		else
		{
			p.equips[i].vnum = 0;
		}
	}
	ch->GetDesc()->Packet(&p, sizeof(p));
}

bool CHARACTER::CanSummon(int iLeaderShip)
{
	return ((iLeaderShip >= 20) || ((iLeaderShip >= 12) && ((m_dwLastDeadTime + 180) > get_dword_time())));
}

void CHARACTER::MountVnum(DWORD vnum)
{
	if (m_dwMountVnum == vnum)
		return;
	if ((m_dwMountVnum != 0) && (vnum != 0)) //@fixme108 set recursively to 0 for eventuality
		MountVnum(0);

	m_dwMountVnum = vnum;
	m_dwMountTime = get_dword_time();

	if (m_bIsObserver)
		return;

	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;
		EncodeInsertPacket(entity);
	}
	ComputePoints();
}

namespace {
	class FuncCheckWarp
	{
	public:
		FuncCheckWarp(LPCHARACTER pkWarp)
		{
			m_lTargetY = 0;
			m_lTargetX = 0;

			m_lX = pkWarp->GetX();
			m_lY = pkWarp->GetY();

			m_bInvalid = false;
			m_bEmpire = pkWarp->GetEmpire();

			char szTmp[64];

			if (3 != sscanf(pkWarp->GetName(), " %s %ld %ld ", szTmp, &m_lTargetX, &m_lTargetY))
			{
				if (number(1, 100) < 5)
					sys_err("Warp NPC name wrong : vnum(%d) name(%s)", pkWarp->GetRaceNum(), pkWarp->GetName());

				m_bInvalid = true;

				return;
			}

			m_lTargetX *= 100;
			m_lTargetY *= 100;

			m_bUseWarp = true;

			if (pkWarp->IsGoto())
			{
				LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(pkWarp->GetMapIndex());
				m_lTargetX += pkSectreeMap->m_setting.iBaseX;
				m_lTargetY += pkSectreeMap->m_setting.iBaseY;
				m_bUseWarp = false;
			}
		}

		bool Valid()
		{
			return !m_bInvalid;
		}

		void operator () (LPENTITY ent)
		{
			if (!Valid())
				return;

			if (!ent->IsType(ENTITY_CHARACTER))
				return;

			LPCHARACTER pkChr = (LPCHARACTER)ent;

			if (!pkChr->IsPC())
				return;

			int iDist = DISTANCE_APPROX(pkChr->GetX() - m_lX, pkChr->GetY() - m_lY);

			if (iDist > 300)
				return;

			if (m_bEmpire && pkChr->GetEmpire() && m_bEmpire != pkChr->GetEmpire())
				return;

			if (!pkChr->CanWarp())
				return;

			if (!pkChr->CanHandleItem(false, true))
				return;

			if (m_bUseWarp)
				pkChr->WarpSet(m_lTargetX, m_lTargetY);
			else
			{
				pkChr->Show(pkChr->GetMapIndex(), m_lTargetX, m_lTargetY);
				pkChr->Stop();
			}
		}

		bool m_bInvalid;
		bool m_bUseWarp;

		long m_lX;
		long m_lY;
		long m_lTargetX;
		long m_lTargetY;

		BYTE m_bEmpire;
	};
}

EVENTFUNC(warp_npc_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("warp_npc_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->GetSectree())
	{
		ch->m_pkWarpNPCEvent = NULL;
		return 0;
	}

	FuncCheckWarp f(ch);
	if (f.Valid())
		ch->GetSectree()->ForEachAround(f);

	return passes_per_sec / 2;
}

void CHARACTER::StartWarpNPCEvent()
{
	if (m_pkWarpNPCEvent)
		return;

	if (!IsWarp() && !IsGoto())
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkWarpNPCEvent = event_create(warp_npc_event, info, passes_per_sec / 2);
}

void CHARACTER::SyncPacket()
{
	TEMP_BUFFER buf;

	TPacketCGSyncPositionElement elem;

	elem.dwVID = GetVID();
	elem.lX = GetX();
	elem.lY = GetY();

	TPacketGCSyncPosition pack;

	pack.bHeader = HEADER_GC_SYNC_POSITION;
	pack.wSize = sizeof(TPacketGCSyncPosition) + sizeof(elem);

	buf.write(&pack, sizeof(pack));
	buf.write(&elem, sizeof(elem));

	PacketAround(buf.read_peek(), buf.size());
}

LPCHARACTER CHARACTER::GetMarryPartner() const
{
	return m_pkChrMarried;
}

void CHARACTER::SetMarryPartner(LPCHARACTER ch)
{
	m_pkChrMarried = ch;
}

int CHARACTER::GetMarriageBonus(DWORD dwItemVnum, bool bSum)
{
	if (IsNPC())
		return 0;

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());

	if (!pMarriage)
		return 0;

	return pMarriage->GetBonus(dwItemVnum, bSum, this);
}

void CHARACTER::ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID)
{
	if (!IsPC())
		return;

	TPacketGCQuestConfirm p;

	p.header = HEADER_GC_QUEST_CONFIRM;
	p.requestPID = dwRequestPID;
	p.timeout = iTimeout;
	strlcpy(p.msg, szMsg, sizeof(p.msg));

	GetDesc()->Packet(&p, sizeof(p));
}

int CHARACTER::GetPremiumRemainSeconds(BYTE bType) const
{
	if (bType >= PREMIUM_MAX_NUM)
		return 0;

	return m_aiPremiumTimes[bType] - get_global_time();
}

bool CHARACTER::WarpToPID(DWORD dwPID)
{
	LPCHARACTER victim;
	if ((victim = (CHARACTER_MANAGER::instance().FindByPID(dwPID))))
	{
		int mapIdx = victim->GetMapIndex();
		if (IS_SUMMONABLE_ZONE(mapIdx))
		{
			if (CAN_ENTER_ZONE(this, mapIdx))
			{
				WarpSet(victim->GetX(), victim->GetY());
			}
			else
			{
				NewChatPacket(CANT_TELEPORT_THIS_PLAYER);
				return false;
			}
		}
		else
		{
			NewChatPacket(CANT_TELEPORT_THIS_PLAYER);
			return false;
		}
	}
	else
	{
		CCI* pcci = P2P_MANAGER::instance().FindByPID(dwPID);

		if (!pcci)
		{
			NewChatPacket(THIS_PLAYER_IS_OFFLINE);
			return false;
		}
#ifndef ENABLE_P2P_WARP
		if (pcci->bChannel != g_bChannel)
		{
			NewChatPacket(OTHER_CHANNEL_ERR, "%d|%d", pcci->bChannel, g_bChannel);
			return false;
		}
#endif
		if (false == IS_SUMMONABLE_ZONE(pcci->lMapIndex))
		{
			NewChatPacket(CANT_TELEPORT_THIS_PLAYER);
			return false;
		}
		else
		{
			if (!CAN_ENTER_ZONE(this, pcci->lMapIndex))
			{
				NewChatPacket(CANT_TELEPORT_THIS_PLAYER);
				return false;
			}

			TPacketGGFindPosition p;
			p.header = HEADER_GG_FIND_POSITION;
			p.dwFromPID = GetPlayerID();
			p.dwTargetPID = dwPID;
			pcci->pkDesc->Packet(&p, sizeof(TPacketGGFindPosition));
		}
	}
	return true;
}

// ADD_REFINE_BUILDING
CGuild* CHARACTER::GetRefineGuild() const
{
	LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);

	return (chRefineNPC ? chRefineNPC->GetGuild() : NULL);
}

bool CHARACTER::IsRefineThroughGuild() const
{
	return GetRefineGuild() != NULL;
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
int64_t CHARACTER::ComputeRefineFee(int64_t iCost, int iMultiply) const
#else
int CHARACTER::ComputeRefineFee(int iCost, int iMultiply) const
#endif
{
	CGuild* pGuild = GetRefineGuild();
	if (pGuild)
	{
		if (pGuild == GetGuild())
			return iCost * iMultiply * 9 / 10;

		LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);
		if (chRefineNPC && chRefineNPC->GetEmpire() != GetEmpire())
			return iCost * iMultiply * 3;

		return iCost * iMultiply;
	}
	else
		return iCost;
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
void CHARACTER::PayRefineFee(int64_t iTotalMoney)
#else
void CHARACTER::PayRefineFee(int iTotalMoney)
#endif
{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t iFee = iTotalMoney / 10;
#else
	int iFee = iTotalMoney / 10;
#endif
	CGuild* pGuild = GetRefineGuild();

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t iRemain = iTotalMoney;
#else
	int iRemain = iTotalMoney;
#endif

	if (pGuild)
	{
		if (pGuild != GetGuild())
		{
			pGuild->RequestDepositMoney(this, iFee);
			iRemain -= iFee;
		}
	}

	PointChange(POINT_GOLD, -iRemain);
}
// END_OF_ADD_REFINE_BUILDING


void CHARACTER::Say(const std::string & s)
{
	struct ::packet_script packet_script;

	packet_script.header = HEADER_GC_SCRIPT;
	packet_script.skin = 1;
	packet_script.src_size = s.size();
	packet_script.size = packet_script.src_size + sizeof(struct packet_script);

	TEMP_BUFFER buf;

	buf.write(&packet_script, sizeof(struct packet_script));
	buf.write(&s[0], s.size());

	if (IsPC())
	{
		GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}

void CHARACTER::UpdateDepositPulse()
{
	m_deposit_pulse = thecore_pulse() + PASSES_PER_SEC(60 * 5);
}

bool CHARACTER::CanDeposit() const
{
	return (m_deposit_pulse == 0 || (m_deposit_pulse < thecore_pulse()));
}

ESex GET_SEX(LPCHARACTER ch)
{
	switch (ch->GetRaceNum())
	{
	case MAIN_RACE_WARRIOR_M:
	case MAIN_RACE_SURA_M:
	case MAIN_RACE_ASSASSIN_M:
	case MAIN_RACE_SHAMAN_M:
#ifdef ENABLE_WOLFMAN_CHARACTER
	case MAIN_RACE_WOLFMAN_M:
#endif
		return SEX_MALE;

	case MAIN_RACE_ASSASSIN_W:
	case MAIN_RACE_SHAMAN_W:
	case MAIN_RACE_WARRIOR_W:
	case MAIN_RACE_SURA_W:
		return SEX_FEMALE;
	}

	/* default sex = male */
	return SEX_MALE;
}

HP_LL CHARACTER::GetHPPct() const
{
	if (GetMaxHP() <= 0) // @fixme136
		return 0;
	return (GetHP() * 100) / GetMaxHP();
}

bool CHARACTER::IsBerserk() const
{
	if (m_pkMobInst != NULL)
		return m_pkMobInst->m_IsBerserk;
	else
		return false;
}

void CHARACTER::SetBerserk(bool mode)
{
	if (m_pkMobInst != NULL)
		m_pkMobInst->m_IsBerserk = mode;
}

bool CHARACTER::IsGodSpeed() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsGodSpeed;
	}
	else
	{
		return false;
	}
}

void CHARACTER::SetGodSpeed(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsGodSpeed = mode;

		if (mode == true)
		{
			SetPoint(POINT_ATT_SPEED, 250);
		}
		else
		{
			SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.sAttackSpeed);
		}
	}
}

bool CHARACTER::IsDeathBlow() const
{
	if (number(1, 100) <= m_pkMobData->m_table.bDeathBlowPoint)
	{
		return true;
	}
	else
	{
		return false;
	}
}

struct FFindReviver
{
	FFindReviver()
	{
		pChar = NULL;
		HasReviver = false;
	}

	void operator() (LPCHARACTER ch)
	{
		if (ch->IsMonster() != true)
		{
			return;
		}

		if (ch->IsReviver() == true && pChar != ch && ch->IsDead() != true)
		{
			if (number(1, 100) <= ch->GetMobTable().bRevivePoint)
			{
				HasReviver = true;
				pChar = ch;
			}
		}
	}

	LPCHARACTER pChar;
	bool HasReviver;
};

bool CHARACTER::HasReviverInParty() const
{
	LPPARTY party = GetParty();

	if (party != NULL)
	{
		if (party->GetMemberCount() == 1) return false;

		FFindReviver f;
		party->ForEachMemberPtr(f);
		return f.HasReviver;
	}

	return false;
}

bool CHARACTER::IsRevive() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsRevive;
	}

	return false;
}

void CHARACTER::SetRevive(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsRevive = mode;
	}
}

void CHARACTER::GoHome()
{
	WarpSet(EMPIRE_START_X(GetEmpire()), EMPIRE_START_Y(GetEmpire()));
}

void CHARACTER::SendGuildName(CGuild * pGuild)
{
	if (NULL == pGuild) return;

	DESC* desc = GetDesc();

	if (NULL == desc) return;
	if (m_known_guild.find(pGuild->GetID()) != m_known_guild.end()) return;

	m_known_guild.insert(pGuild->GetID());

	TPacketGCGuildName	pack;
	memset(&pack, 0x00, sizeof(pack));

	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_GUILD_NAME;
	pack.size = sizeof(TPacketGCGuildName);
	pack.guildID = pGuild->GetID();
	memcpy(pack.guildName, pGuild->GetName(), GUILD_NAME_MAX_LEN);

	desc->Packet(&pack, sizeof(pack));
}

void CHARACTER::SendGuildName(DWORD dwGuildID)
{
	SendGuildName(CGuildManager::instance().FindGuild(dwGuildID));
}

EVENTFUNC(destroy_when_idle_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("destroy_when_idle_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (ch->GetVictim())
	{
		return PASSES_PER_SEC(300);
	}
	ch->m_pkDestroyWhenIdleEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::StartDestroyWhenIdleEvent()
{
	if (m_pkDestroyWhenIdleEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkDestroyWhenIdleEvent = event_create(destroy_when_idle_event, info, PASSES_PER_SEC(300));
}

void CHARACTER::ResetChatCounter()
{
	m_bChatCounter = 0;
}

BYTE CHARACTER::IncreaseChatCounter()
{
	return ++m_bChatCounter;
}

BYTE CHARACTER::GetChatCounter() const
{
	return m_bChatCounter;
}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
BYTE CHARACTER::GetMountCounter() const
{
	return m_bMountCounter;
}

void CHARACTER::ResetMountCounter()
{
	m_bMountCounter = 0;
}

BYTE CHARACTER::IncreaseMountCounter()
{
	return ++m_bMountCounter;
}
#endif

bool CHARACTER::IsRiding() const
{
	return IsHorseRiding() || GetMountVnum();
}

DWORD CHARACTER::GetNextExp() const
{
	if (PLAYER_MAX_LEVEL_CONST < GetLevel())
		return 2500000000u;
	else
		return exp_table[GetLevel()];
}

int	CHARACTER::GetSkillPowerByLevel(int level, bool bMob) const
{
	return CTableBySkill::instance().GetSkillPowerByLevelFromType(GetJob(), GetSkillGroup(), MINMAX(0, level, SKILL_MAX_LEVEL), bMob);
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
std::vector<LPITEM> CHARACTER::GetAcceMaterials()
{
	return std::vector<LPITEM>{ITEM_MANAGER::instance().Find(m_pointsInstant.pAcceMaterials[0].id), ITEM_MANAGER::instance().Find(m_pointsInstant.pAcceMaterials[1].id)};
}

const TItemPosEx* CHARACTER::GetAcceMaterialsInfo()
{
	return m_pointsInstant.pAcceMaterials;
}

void CHARACTER::SetAcceMaterial(int pos, LPITEM ptr)
{
	if (pos < 0 || pos >= ACCE_WINDOW_MAX_MATERIALS)
		return;
	if (!ptr)
		m_pointsInstant.pAcceMaterials[pos] = {};
	else
	{
		m_pointsInstant.pAcceMaterials[pos].id = ptr->GetID();
		m_pointsInstant.pAcceMaterials[pos].pos.cell = ptr->GetCell();
		m_pointsInstant.pAcceMaterials[pos].pos.window_type = ptr->GetWindow();
	}
}

void CHARACTER::OpenAcce(bool bCombination)
{
	if (IsAcceOpened(bCombination))
	{
		NewChatPacket(THIS_WINDOW_IS_ALREADY_OPEN);
		return;
	}

	if (bCombination)
	{
		if (m_bAcceAbsorption)
		{
			NewChatPacket(CLOSE_OTHER_WINDOWS_TO_DO_THIS);
			return;
		}

		m_bAcceCombination = true;
	}
	else
	{
		if (m_bAcceCombination)
		{
			NewChatPacket(CLOSE_OTHER_WINDOWS_TO_DO_THIS);
			return;
		}

		m_bAcceAbsorption = true;
	}

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_OPEN;
	sPacket.bWindow = bCombination;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAcce));

	ClearAcceMaterials();
}

void CHARACTER::CloseAcce()
{
	if ((!m_bAcceCombination) && (!m_bAcceAbsorption))
		return;

	bool bWindow = m_bAcceCombination;

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_CLOSE;
	sPacket.bWindow = bWindow;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAcce));

	if (bWindow)
		m_bAcceCombination = false;
	else
		m_bAcceAbsorption = false;

	ClearAcceMaterials();
}

void CHARACTER::ClearAcceMaterials()
{
	auto pkItemMaterial = GetAcceMaterials();
	for (int i = 0; i < ACCE_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			continue;

		pkItemMaterial[i]->Lock(false);
		pkItemMaterial[i] = NULL;
		SetAcceMaterial(i, nullptr);
	}
}

bool CHARACTER::AcceIsSameGrade(long lGrade)
{
	auto pkItemMaterial = GetAcceMaterials();
	if (!pkItemMaterial[0])
		return false;
	return pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD) == lGrade;
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
int64_t CHARACTER::GetAcceCombinePrice(long lGrade)
#else
DWORD CHARACTER::GetAcceCombinePrice(long lGrade)
#endif
{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t dwPrice = 0;
#else
	DWORD dwPrice = 0;
#endif
	switch (lGrade)
	{
	case 2:
	{
		dwPrice = ACCE_GRADE_2_PRICE;
	}
	break;
	case 3:
	{
		dwPrice = ACCE_GRADE_3_PRICE;
	}
	break;
	case 4:
	{
		dwPrice = ACCE_GRADE_4_PRICE;
	}
	break;
	default:
	{
		dwPrice = ACCE_GRADE_1_PRICE;
	}
	break;
	}

	return dwPrice;
}

BYTE CHARACTER::CheckEmptyMaterialSlot()
{
	auto pkItemMaterial = GetAcceMaterials();
	for (int i = 0; i < ACCE_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			return i;
	}

	return 255;
}

void CHARACTER::GetAcceCombineResult(DWORD & dwItemVnum, DWORD & dwMinAbs, DWORD & dwMaxAbs)
{
	auto pkItemMaterial = GetAcceMaterials();

	if (m_bAcceCombination)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			long lVal = pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD);
			if (lVal == 4)
			{
				dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
				dwMinAbs = pkItemMaterial[0]->GetSocket(ACCE_ABSORPTION_SOCKET);
				DWORD dwMaxAbsCalc = (dwMinAbs + ACCE_GRADE_4_ABS_RANGE > ACCE_GRADE_4_ABS_MAX ? ACCE_GRADE_4_ABS_MAX : (dwMinAbs + ACCE_GRADE_4_ABS_RANGE));
				dwMaxAbs = dwMaxAbsCalc;
			}
			else
			{
				DWORD dwMaskVnum = pkItemMaterial[0]->GetOriginalVnum();
				TItemTable* pTable = ITEM_MANAGER::instance().GetTable(dwMaskVnum + 1);
				if (pTable)
					dwMaskVnum += 1;

				dwItemVnum = dwMaskVnum;
				switch (lVal)
				{
				case 2:
				{
					dwMinAbs = ACCE_GRADE_3_ABS;
					dwMaxAbs = ACCE_GRADE_3_ABS;
				}
				break;
				case 3:
				{
					dwMinAbs = ACCE_GRADE_4_ABS_MIN;
					dwMaxAbs = ACCE_GRADE_4_ABS_MAX_COMB;
				}
				break;
				default:
				{
					dwMinAbs = ACCE_GRADE_2_ABS;
					dwMaxAbs = ACCE_GRADE_2_ABS;
				}
				break;
				}
			}
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
	else
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
			dwMinAbs = pkItemMaterial[0]->GetSocket(ACCE_ABSORPTION_SOCKET);
			dwMaxAbs = dwMinAbs;
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
}

void CHARACTER::AddAcceMaterial(TItemPos tPos, BYTE bPos)
{
	if (bPos >= ACCE_WINDOW_MAX_MATERIALS)
	{
		if (bPos == 255)
		{
			bPos = CheckEmptyMaterialSlot();
			if (bPos >= ACCE_WINDOW_MAX_MATERIALS)
				return;
		}
		else
			return;
	}

	LPITEM pkItem = GetItem(tPos);
	if (!pkItem)
		return;
	else if ((pkItem->GetCell() >= INVENTORY_MAX_NUM) || (pkItem->IsEquipped()) || (tPos.IsBeltInventoryPosition()) || (pkItem->IsDragonSoul()))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME) && (m_bAcceCombination))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME) && (bPos == 0) && (m_bAcceAbsorption))
		return;
	else if (pkItem->isLocked())
	{
		NewChatPacket(CANT_ADD_LOCKED_ITEMS);
		return;
	}
	else if ((m_bAcceCombination) && (bPos == 1) && (!AcceIsSameGrade(pkItem->GetValue(ACCE_GRADE_VALUE_FIELD))))
	{
		NewChatPacket(ONLY_SAME_GRADE_CAN_COMBINED);
		return;
	}
	else if ((m_bAcceCombination) && (pkItem->GetSocket(ACCE_ABSORPTION_SOCKET) >= ACCE_GRADE_4_ABS_MAX))
	{
		NewChatPacket(ABSORBPTION_CHANGE_IS_MAX);
		return;
	}
	else if ((bPos == 1) && (m_bAcceAbsorption))
	{
		if ((pkItem->GetType() != ITEM_WEAPON) && (pkItem->GetType() != ITEM_ARMOR))
		{
			NewChatPacket(ONLY_ARMOR_WEARPON_CAN_BE_ABSORBED);
			return;
		}
		else if ((pkItem->GetType() == ITEM_ARMOR) && (pkItem->GetSubType() != ARMOR_BODY))
		{
			NewChatPacket(ONLY_ARMOR_WEARPON_CAN_BE_ABSORBED);
			return;
		}
	}
	else if ((pkItem->GetSubType() != COSTUME_ACCE) && (m_bAcceCombination))
		return;
	else if ((pkItem->GetSubType() != COSTUME_ACCE) && (bPos == 0) && (m_bAcceAbsorption))
		return;
	else if ((pkItem->GetSocket(ACCE_ABSORBED_SOCKET) > 0) && (bPos == 0) && (m_bAcceAbsorption))
		return;

	auto pkItemMaterial = GetAcceMaterials();
	if ((bPos == 1) && (!pkItemMaterial[0]))
		return;

	if (pkItemMaterial[bPos])
		return;

	SetAcceMaterial(bPos, pkItem);
	pkItemMaterial[bPos] = pkItem;
	pkItemMaterial[bPos]->Lock(true);

	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetAcceCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_ADDED;
	sPacket.bWindow = m_bAcceCombination;
	sPacket.dwPrice = GetAcceCombinePrice(pkItem->GetValue(ACCE_GRADE_VALUE_FIELD));
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = dwItemVnum;
	sPacket.dwMinAbs = dwMinAbs;
	sPacket.dwMaxAbs = dwMaxAbs;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAcce));
}

void CHARACTER::RemoveAcceMaterial(BYTE bPos)
{
	if (bPos >= ACCE_WINDOW_MAX_MATERIALS)
		return;

	auto pkItemMaterial = GetAcceMaterials();

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t dwPrice = 0;
#else
	DWORD dwPrice = 0;
#endif

	if (bPos == 1)
	{
		if (pkItemMaterial[bPos])
		{
			pkItemMaterial[bPos]->Lock(false);
			pkItemMaterial[bPos] = NULL;
			SetAcceMaterial(bPos, nullptr);
		}

		if (pkItemMaterial[0])
			dwPrice = GetAcceCombinePrice(pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD));
	}
	else
		ClearAcceMaterials();

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAcce sPacket;
	sPacket.header = HEADER_GC_ACCE;
	sPacket.subheader = ACCE_SUBHEADER_GC_REMOVED;
	sPacket.bWindow = m_bAcceCombination;
	sPacket.dwPrice = dwPrice;
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAcce));
}

BYTE CHARACTER::CanRefineAcceMaterials()
{
	BYTE bReturn = 0;
	if (!GetDesc())
		return bReturn;

	if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || IsOpenOfflineShop() || ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
		|| isAuraOpened(true) || isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| Is67AttrOpen()
#endif
		)
		return bReturn;

	auto materialInfo = GetAcceMaterialsInfo();
	auto pkItemMaterial = GetAcceMaterials();
	if (!pkItemMaterial[0] || !pkItemMaterial[1])
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial null");
		return bReturn;
	}
	else if (pkItemMaterial[0]->GetOwner() != this || pkItemMaterial[1]->GetOwner() != this)
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial different ownership");
		return bReturn;
	}
	else if (pkItemMaterial[0]->IsEquipped() || pkItemMaterial[1]->IsEquipped())
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial equipped");
		return bReturn;
	}
	else if (pkItemMaterial[0]->GetWindow() != INVENTORY || pkItemMaterial[1]->GetWindow() != INVENTORY)
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial not in INVENTORY");
		return bReturn;
	}
	else if (!materialInfo[0].id || !materialInfo[1].id)
	{
		sys_err("CanRefineAcceMaterials: materialInfo id 0");
		return bReturn;
	}
	else if (materialInfo[0].pos.cell != pkItemMaterial[0]->GetCell() || materialInfo[1].pos.cell != pkItemMaterial[1]->GetCell())
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial wrong cell");
		return bReturn;
	}
	else if (materialInfo[0].pos.window_type != pkItemMaterial[0]->GetWindow() || materialInfo[1].pos.window_type != pkItemMaterial[1]->GetWindow())
	{
		sys_err("CanRefineAcceMaterials: pkItemMaterial wrong window_type");
		return bReturn;
	}

	if (m_bAcceCombination)
	{
		if (!AcceIsSameGrade(pkItemMaterial[1]->GetValue(ACCE_GRADE_VALUE_FIELD)))
		{
			sys_err("CanRefineAcceMaterials: pkItemMaterial different acce grade");
			return bReturn;
		}

		for (int i = 0; i < ACCE_WINDOW_MAX_MATERIALS; ++i)
		{
			if (pkItemMaterial[i])
			{
				if ((pkItemMaterial[i]->GetType() == ITEM_COSTUME) && (pkItemMaterial[i]->GetSubType() == COSTUME_ACCE))
					bReturn = 1;
				else
				{
					bReturn = 0;
					break;
				}
			}
			else
			{
				bReturn = 0;
				break;
			}
		}
	}
	else if (m_bAcceAbsorption)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			if ((pkItemMaterial[0]->GetType() == ITEM_COSTUME) && (pkItemMaterial[0]->GetSubType() == COSTUME_ACCE))
				bReturn = 2;
			else
				bReturn = 0;

			if ((pkItemMaterial[1]->GetType() == ITEM_WEAPON) || ((pkItemMaterial[1]->GetType() == ITEM_ARMOR) && (pkItemMaterial[1]->GetSubType() == ARMOR_BODY)))
				bReturn = 2;
			else
				bReturn = 0;

			if (pkItemMaterial[0]->GetSocket(ACCE_ABSORBED_SOCKET) > 0)
				bReturn = 0;
		}
		else
			bReturn = 0;
	}

	return bReturn;
}

void CHARACTER::RefineAcceMaterials()
{
	BYTE bCan = CanRefineAcceMaterials();
	if (bCan == 0)
		return;

	if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen() || IsOpenOfflineShop() || ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
		|| isAuraOpened(true) || isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| Is67AttrOpen()
#endif
		)
		return;

	auto pkItemMaterial = GetAcceMaterials();

	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetAcceCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t dwPrice = GetAcceCombinePrice(pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD));
#else
	DWORD dwPrice = GetAcceCombinePrice(pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD));
#endif

	if (bCan == 1)
	{
		int iSuccessChance = 0;
		long lVal = pkItemMaterial[0]->GetValue(ACCE_GRADE_VALUE_FIELD);
		switch (lVal)
		{
		case 2:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_2;
		}
		break;
		case 3:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_3;
		}
		break;
		case 4:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_4;
		}
		break;
		default:
		{
			iSuccessChance = ACCE_COMBINE_GRADE_1;
		}
		break;
		}

		if (GetGold() < dwPrice)
		{
			NewChatPacket(NOT_ENOUGH_YANG);
			return;
		}

		int iChance = number(1, 100);
		bool bSucces = (iChance <= iSuccessChance ? true : false);

		if (bSucces)
		{
			LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(dwItemVnum, 1, 0, false);
			if (!pkItem)
			{
				sys_err("%d can't be created.", dwItemVnum);
				return;
			}

			ITEM_MANAGER::CopyAllAttrTo(pkItemMaterial[0], pkItem);
			DWORD dwAbs = (dwMinAbs == dwMaxAbs ? dwMinAbs : number(dwMinAbs + 1, dwMaxAbs));
			pkItem->SetSocket(ACCE_ABSORPTION_SOCKET, dwAbs);
			pkItem->SetSocket(ACCE_ABSORBED_SOCKET, pkItemMaterial[0]->GetSocket(ACCE_ABSORBED_SOCKET));

			PointChange(POINT_GOLD, -dwPrice);

			WORD wCell = pkItemMaterial[0]->GetCell();
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[0], "COMBINE (REFINE SUCCESS)");
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "COMBINE (REFINE SUCCESS)");

			pkItem->AddToCharacter(this, TItemPos(INVENTORY, wCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
			if (lVal == 4)
				NewChatPacket(NEW_ABSORB_RATE, "%d", dwAbs);
			else
				NewChatPacket(IT_WAS_SUCCESFULL);

			EffectPacket(SE_EFFECT_ACCE_SUCCEDED);
			ClearAcceMaterials();

#ifdef ENABLE_EXTENDED_BATTLE_PASS
			UpdateExtBattlePassMissionProgress(BP_REFINE_SASH, 1, pkItem->GetVnum());
#endif

		}
		else
		{
			PointChange(POINT_GOLD, -dwPrice);
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "COMBINE (REFINE FAIL)");

			if (lVal == 4)
				NewChatPacket(NEW_ABSORB_RATE, "%d", pkItemMaterial[0]->GetSocket(ACCE_ABSORPTION_SOCKET));
			else
				NewChatPacket(IT_WAS_FAILED);

			pkItemMaterial[1] = NULL;
		}

		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;

		TPacketAcce sPacket;
		sPacket.header = HEADER_GC_ACCE;
		sPacket.subheader = ACCE_SUBHEADER_CG_REFINED;
		sPacket.bWindow = m_bAcceCombination;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 0;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		if (bSucces)
			sPacket.dwMaxAbs = 100;
		else
			sPacket.dwMaxAbs = 0;

		GetDesc()->Packet(&sPacket, sizeof(TPacketAcce));
	}
	else
	{
		pkItemMaterial[1]->CopyAttributeTo(pkItemMaterial[0]);
		pkItemMaterial[0]->SetSocket(ACCE_ABSORBED_SOCKET, pkItemMaterial[1]->GetOriginalVnum());
#ifdef USE_ACCE_ABSORB_WITH_NO_NEGATIVE_BONUS
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (pkItemMaterial[0]->GetAttributeValue(i) < 0)
				pkItemMaterial[0]->SetForceAttribute(i, pkItemMaterial[0]->GetAttributeType(i), 0);
		}
#endif
		ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "ABSORBED (REFINE SUCCESS)");

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemMaterial[0]);
		NewChatPacket(IT_WAS_SUCCESFULL);
		EffectPacket(SE_EFFECT_ACCE_SUCCEDED);

		ClearAcceMaterials();

		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;

		TPacketAcce sPacket;
		sPacket.header = HEADER_GC_ACCE;
		sPacket.subheader = ACCE_SUBHEADER_CG_REFINED;
		sPacket.bWindow = m_bAcceCombination;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 255;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		sPacket.dwMaxAbs = 1;
		GetDesc()->Packet(&sPacket, sizeof(TPacketAcce));
	}
}

bool CHARACTER::CleanAcceAttr(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
	{
		NewChatPacket(CLOSE_OTHER_WINDOWS_TO_DO_THIS);
		return false;
	}
	else if ((!pkItem) || (!pkTarget))
	{
		NewChatPacket(INVALID_ACTION);
		return false;
	}
	else if ((pkTarget->GetType() != ITEM_COSTUME) && (pkTarget->GetSubType() != COSTUME_ACCE))
	{
		NewChatPacket(CANT_USE_THIS_WITH_NON_SASH_ITEMS);
		return false;
	}

	if (pkTarget->GetSocket(ACCE_ABSORBED_SOCKET) <= 0)
	{
		NewChatPacket(ITEM_HAS_NO_ABSORBTION);
		return false;
	}

	pkTarget->SetSocket(ACCE_ABSORBED_SOCKET, 0);
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		pkTarget->SetForceAttribute(i, 0, 0);
	return true;
}
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
void CHARACTER::MountSummon(LPITEM mountItem)
{
#ifdef ENABLE_LOADING_RENEWAL
	if (IsPC() && GetLoadingState() != 0)
	{
		return;
	}
#endif

	if (IsPolymorphed() == true)
	{
		NewChatPacket(POLY_BLOCKED_WITH_THIS);
		return;
	}

	if (GetMapIndex() == 113)
		return;

	if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
		return;

	CMountSystem* mountSystem = GetMountSystem();
	DWORD mobVnum = 0;

	if (!mountSystem || !mountItem)
		return;

#ifdef ENABLE_MOUNT_SKIN
	if (mountItem->GetType() == ITEM_MOUNT_SKIN)//Skin takilirsa normal binek gorunumunu siler
	{
		LPITEM mount = GetWear(WEAR_COSTUME_MOUNT);
		if (mount)
			MountUnsummon(mount);
	}
	if (mountItem->GetType() == ITEM_COSTUME && GetWear(WEAR_MOUNT_SKIN))//binek skin takili ve normal binegi hesapliyorsa
		return;
#endif

	if (mountItem->GetValue(1) != 0)
		mobVnum = mountItem->GetValue(1);

	if (IsHorseRiding())
		StopRiding();

	if (GetHorse())
		HorseSummon(false);

	mountSystem->Summon(mobVnum, mountItem, false);
}

void CHARACTER::MountUnsummon(LPITEM mountItem)
{
#ifdef ENABLE_LOADING_RENEWAL
	if (IsPC() && GetLoadingState() != 0)
	{
		return;
	}
#endif

	CMountSystem* mountSystem = GetMountSystem();
	DWORD mobVnum = 0;

	if (!mountSystem || !mountItem)
		return;

	if (mountItem->GetValue(1) != 0)
		mobVnum = mountItem->GetValue(1);

	if (GetMountVnum() == mobVnum)
		mountSystem->Unmount(mobVnum);

	mountSystem->Unsummon(mobVnum);
#ifdef ENABLE_MOUNT_SKIN
	if (mountItem->GetType() == ITEM_MOUNT_SKIN)//binek skin cikinca normal binegi cagirir fonksyon sonunda kalmasi gerek
	{
		LPITEM mount = GetWear(WEAR_COSTUME_MOUNT);
		if (mount)
			MountSummon(mount);
#ifdef ENABLE_HIDE_COSTUME
		SetMountSkinHidden(false);
#endif
	}
#endif
}

void CHARACTER::CheckMount()
{
	if (GetMapIndex() == 113)
		return;

	if (CWarMapManager::instance().IsWarMap(GetMapIndex()))
		return;

#ifdef ENABLE_MOUNT_SKIN
	if (GetWear(WEAR_MOUNT_SKIN))
	{
		LPITEM mountskin = GetWear(WEAR_MOUNT_SKIN);
		CMountSystem* mountSystem = GetMountSystem();
		DWORD mobVnum = 0;

		if (!mountSystem || !mountskin)
			return;

		if (mountskin->GetValue(1) != 0)
			mobVnum = mountskin->GetValue(1);

		if (mountSystem->CountSummoned() == 0)
		{
			mountSystem->Summon(mobVnum, mountskin, false);
		}
	}
	else if (GetWear(WEAR_COSTUME_MOUNT))
	{
		LPITEM mountItem = GetWear(WEAR_COSTUME_MOUNT);
		CMountSystem* mountSystem = GetMountSystem();
		DWORD mobVnum = 0;

		if (!mountSystem || !mountItem)
			return;

		if (mountItem->GetValue(1) != 0)
			mobVnum = mountItem->GetValue(1);

		if (mountSystem->CountSummoned() == 0)
		{
			mountSystem->Summon(mobVnum, mountItem, false);
		}
	}
#else
	if (GetWear(WEAR_COSTUME_MOUNT))
	
		if (mountItem->GetValue(1) != 0)
			mobVnum = mountItem->GetValue(1);

		if (mountSystem->CountSummoned() == 0)
		{
			mountSystem->Summon(mobVnum, mountItem, false);
		}
	}
#endif
}

bool CHARACTER::IsRidingMount()
{
	return (GetWear(WEAR_COSTUME_MOUNT));
}
#endif

#ifdef ENABLE_PET_COSTUME_SYSTEM
void CHARACTER::PetSummon(LPITEM PetItem)
{
	if (GetMapIndex() == 113)
		return;

#ifdef ENABLE_LOADING_RENEWAL
	if (IsPC() && GetLoadingState() != 0)
	{
		return;
	}
#endif

	CPetSystem* petSystem = GetPetSystem();
	DWORD mobVnum = 0;

	if (!petSystem || !PetItem)
		return;

	if (PetItem->GetType() == ITEM_PET && GetWear(WEAR_PET_SKIN))//pet skin takili ve normal peti hesapliyorsa
		return;

	if (PetItem->GetType() == ITEM_PET_SKIN)//Skin takilarsa normal pet gorunumunu siler
	{
		LPITEM mainPet = GetWear(WEAR_PET);
		if (mainPet)
			PetUnsummon(mainPet);
	}

	if (PetItem->GetValue(1) != 0)
		mobVnum = PetItem->GetValue(1);

	petSystem->Summon(mobVnum, PetItem, false);
}

void CHARACTER::PetUnsummon(LPITEM PetItem)
{
#ifdef ENABLE_LOADING_RENEWAL
	if (IsPC() && GetLoadingState() != 0)
	{
		return;
	}
#endif

	CPetSystem* PetSystem = GetPetSystem();
	DWORD mobVnum = 0;

	if (!PetSystem || !PetItem)
		return;

	if (PetItem->GetValue(1) != 0)
		mobVnum = PetItem->GetValue(1);

	PetSystem->Unsummon(mobVnum);

	if (PetItem->GetType() == ITEM_PET_SKIN)//pet skin ciktiginnda normal pet gorunumunu getirir
	{
		LPITEM mainPet = GetWear(WEAR_PET);
		PetSummon(mainPet);
	}

}

void CHARACTER::CheckPet()
{
	if (GetMapIndex() == 113)
		return;

	CPetSystem* petSystem = GetPetSystem();
	LPITEM petItem = GetWear(WEAR_PET);

	if (!petSystem || !petItem)
		return;

	DWORD mobVnum = 0;

#ifdef ENABLE_PET_SKIN
	if (GetWear(WEAR_PET_SKIN))
		petItem = GetWear(WEAR_PET_SKIN);
#endif

	if (petItem->GetValue(1) != 0)
		mobVnum = petItem->GetValue(1);

	if (petSystem->CountSummoned() == 0)
		petSystem->Summon(mobVnum, petItem, false);
}
#endif

#ifdef ENABLE_FALL_FIX
bool CHARACTER::CanFall()
{
	if (IsAffectFlag(AFF_CHEONGEUN) && !IsAffectFlag(AFF_CHEONGEUN_WITH_FALL)) //Taichi skill
		return false;

	if (IsImmune(IMMUNE_FALL)) //Immune flag
		return false;

	if (!IsPC() &&
		GetRaceNum() == 1097 &&
		GetRaceNum() == 1098 &&
		GetRaceNum() == 1099 &&
		GetRaceNum() == 2496 &&
		GetRaceNum() == 2497 &&
		GetRaceNum() == 2498
		)
		return false;

	return true;
}
#endif

#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
bool CHARACTER::SwitchChannel(long newAddr, WORD newPort)
{
	if (!IsPC() || !GetDesc() || !CanWarp())
		return false;

	long x = GetX();
	long y = GetY();

	long lAddr = newAddr;
	long lMapIndex = GetMapIndex();
	WORD wPort = newPort;

	// If we currently are in a dungeon.
	if (lMapIndex >= 10000)
	{
		sys_err("Invalid change channel request from dungeon %d!", lMapIndex);
		return false;
	}

	// If we are on CH99.
	if (g_bChannel == 99)
	{
		sys_err("%s attempted to change channel from CH99, ignoring req.", GetName());
		return false;
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	TPacketGCWarp p;

	p.bHeader = HEADER_GC_WARP;
	p.lX = x;
	p.lY = y;
	p.lAddr = lAddr;
	p.wPort = wPort;
#ifdef ENABLE_FARM_BLOCK
	SetWarpCheck(true);
#endif
	GetDesc()->Packet(&p, sizeof(p));
	return true;
}

EVENTINFO(switch_channel_info)
{
	DynamicCharacterPtr ch;
	int secs;
	long newAddr;
	WORD newPort;
	switch_channel_info()
		: ch(),
		secs(0),
		newAddr(0),
		newPort(0)
	{
	}
};

EVENTFUNC(switch_channel)
{
	switch_channel_info* info = dynamic_cast<switch_channel_info*>(event->info);
	if (!info)
	{
		sys_err("No switch channel event info!");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (!ch)
	{
		sys_err("No char to work on for the switch.");
		return 0;
	}

	if (!ch->GetDesc())
		return 0;

	if (info->secs > 0)
	{
		ch->NewChatPacket(CHANNEL_SWITCH_0, "%d", info->secs);
		--info->secs;
		return PASSES_PER_SEC(1);
	}
	ch->m_pkTimedEvent = nullptr;
	ch->SwitchChannel(info->newAddr, info->newPort);
	return 0;
}

bool CHARACTER::StartChannelSwitch(long newAddr, WORD newPort)
{
	if (!CanWarp())
		return false;

	switch_channel_info* info = AllocEventInfo<switch_channel_info>();
	info->ch = this;
	info->secs = CanWarp() && !IsPosition(POS_FIGHTING) ? 3 : 10;
	info->newAddr = newAddr;
	info->newPort = newPort;

	m_pkTimedEvent = event_create(switch_channel, info, 1);
	NewChatPacket(CHANNEL_SWITCH_1);
	return true;
}
#endif

//CHARACTER ACTIVATE CHECK BEGIN
bool CHARACTER::WindowOpenCheck() const
{
	if (GetExchange() ||
		GetMyShop() ||
		GetShopOwner() ||
		IsOpenSafebox() ||
		IsCubeOpen() ||
		IsAcceOpened() ||
#ifdef ENABLE_AURA_SYSTEM
		isAuraOpened(true) || isAuraOpened(false) ||
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		Is67AttrOpen() ||
#endif
		IsOpenOfflineShop()
		)
		return true;

	return false;
}

void CHARACTER::WindowCloseAll()
{
	if (!GetDesc())
		return;

	SetCubeNpc(nullptr);
	CloseAcce();
	CloseAura();

	CShopManager::instance().StopShopping(this);
	CloseMyShop();
	CloseSafebox();

	SetBotControl(false);

	SetOfflineShopGuest(nullptr);

	ChatPacket(CHAT_TYPE_COMMAND, "CloseAllWindows");
}

bool CHARACTER::ActivateCheck(bool notquest, bool dead) const
{
	if (!dead && IsDead())
		return true;

	if (IsStun() ||
		m_pkTimedEvent
		)
		return true;

	if (!notquest)
	{
		if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			return true;
	}

	return false;
}

bool CHARACTER::OfflineShopActivateCheck() const
{
	if (ActivateCheck())
		return false;

	if (GetExchange() ||
		GetMyShop() ||
		GetShopOwner() ||
		IsOpenSafebox() ||
		IsCubeOpen() ||
		IsAcceOpened() ||
#ifdef ENABLE_AURA_SYSTEM
		isAuraOpened(true) || isAuraOpened(false) ||
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		Is67AttrOpen() ||
#endif
		IsOpenedRefine() ||
		IsObserverMode()
		)
		return false;

	const int iPulse = thecore_pulse();

	const int limit_time = PASSES_PER_SEC(g_nPortalLimitTime);

	for (int i = 0; i < MAX_ACTIVATE_CHECK; i++)
	{
		if ((iPulse - GetActivateTime(i)) < limit_time)
			return false;
	}
	return true;
}

bool CHARACTER::CanWarp(bool notquest, bool dead) const
{
	if (ActivateCheck(notquest, dead))
		return false;

	if (WindowOpenCheck())
		return false;

	const int iPulse = thecore_pulse();
	const int limit_time = PASSES_PER_SEC(g_nPortalLimitTime);

	for (int i = 0; i < MAX_ACTIVATE_CHECK; i++)
	{
		if ((iPulse - GetActivateTime(i)) < limit_time)
			return false;
	}

	if ((iPulse - GetOfflineShopUseTime()) < limit_time)
		return false;

	return true;
}

bool CHARACTER::IsActivateTime(BYTE type, BYTE second) const
{
	const int iPulse = thecore_pulse();
	const int limit_time = PASSES_PER_SEC(second);

	if ((iPulse - GetActivateTime(type)) < limit_time)
		return false;

	return true;
}

bool CHARACTER::IsOfflineShopActivateTime(BYTE second) const
{
	const int iPulse = thecore_pulse();
	const int limit_time = PASSES_PER_SEC(second);

	if ((iPulse - GetOfflineShopUseTime()) < limit_time)
		return true;

	return false;
}

bool CHARACTER::IsOpenOfflineShop() const
{
	if (m_pkShopSafebox || m_pkShopSafebox != NULL)
		return true;

	if (m_pkOfflineShopGuest || m_pkOfflineShopGuest != NULL)
		return true;

	if (IsOfflineShopActivateTime(3))
		return true;

	return false;
}
//CHARACTER ACTIVATE CHECK END

#ifdef ENABLE_OFFLINE_SHOP
bool CHARACTER::CanTakeInventoryItem(LPITEM item, TItemPos * cell)
{
	if (!item || !cell)
		return false;

	int iEmpty = -1;
	BYTE windowtype = VnumGetWindowType(item->GetVnum());
	cell->window_type = windowtype;
	cell->cell = iEmpty = WindowTypeToGetEmpty(windowtype, item);

	return iEmpty != -1;
}

void CHARACTER::SetShopSafebox(offlineshop::CShopSafebox * pk)
{
	if (m_pkShopSafebox && pk == NULL)
	{
		m_pkShopSafebox->SetOwner(NULL);
	}

	else if (m_pkShopSafebox == NULL && pk)
	{
		pk->SetOwner(this);
	}

	m_pkShopSafebox = pk;
}
#endif

#ifdef ENABLE_REFRESH_CONTROL
void CHARACTER::RefreshControl(BYTE SubHeader, bool state, bool update)
{
	LPDESC d = GetDesc();
	if (d && (d->IsPhase(PHASE_GAME) || d->IsPhase(PHASE_DEAD)))
	{
		TPacketGCRefreshControl pack;
		pack.Header = HEADER_GC_REFRESH_CONTROL;
		pack.SubHeader = SubHeader;
		pack.update = update;
		pack.state = state;
		d->Packet(&pack, sizeof(TPacketGCRefreshControl));
	}
}
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
EVENTFUNC(biolog_alert_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("recovery_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL)
	{
		return 0;
	}

	TPacketGCBiologUpdate bioinfo = ch->GetBiologInfo();
	if (!bioinfo.alert)
	{
		ch->BiologAlertEventStop();
		return 0;
	}

	if (bioinfo.time <= get_global_time())
	{
		if (!ch->GetBiolog())
		{
			sys_err("Biolog alert event biolog pointer null");
			return 0;
		}

		LPDESC d = ch->GetDesc();
		if (d)
		{
			char buf[255];
			int len = snprintf(buf, sizeof(buf), LC_TEXT("Your time is up, can deliver the next item."));
			TPacketGCWhisper p;
			p.bHeader = HEADER_GC_WHISPER;
			p.bType = WHISPER_TYPE_SYSTEM;
			p.wSize = sizeof(TPacketGCWhisper) + len;
			strlcpy(p.szNameFrom, "[BIOLOG]", sizeof(p.szNameFrom));
			d->BufferedPacket(&p, sizeof(p));
			d->Packet(buf, len);
		}
		else
		{
			sys_err("Biolog alert event desc null");
		}
		ch->SetBiologAlert(false);
		ch->m_biolog_alert = nullptr;
		return 0;
	}
	return PASSES_PER_SEC((bioinfo.time - get_global_time()) + 1);
}
void CHARACTER::BiologAlertEventStart()
{
	if (!m_BiologInfo.alert)
		return;

	if (m_biolog_alert)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_biolog_alert = event_create(biolog_alert_event, info, PASSES_PER_SEC(7));
}

void CHARACTER::BiologAlertEventStop()
{
	if (m_biolog_alert)
	{
		event_cancel(&m_biolog_alert);
		m_biolog_alert = nullptr;
	}
}
#endif

#ifdef ENABLE_CHAR_SETTINGS
void CHARACTER::SendCharSettingsPacket(BYTE type)
{
	LPDESC d = GetDesc();
	if (!d)
	{
		return;
	}

	TPacketGCCharSettings p;
	p.header = HEADER_GC_CHAR_SETTINGS;
	memcpy(&p.data, &m_char_settings, sizeof(p.data));
	p.type = type;
	d->Packet(&p, sizeof(p));
}
#endif

#ifdef ENABLE_LMW_PROTECTION
EVENTFUNC(attack_hack_control_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("recovery_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL)
	{
		return 0;
	}

	if (ch->GetAttackHack())
	{
		ch->m_attackHackControl = nullptr;
		return 0;
	}

	return PASSES_PER_SEC(1);
}

void CHARACTER::AttackHackControlEventStart()
{
	if (m_attackHackControl)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_attackHackControl = event_create(attack_hack_control_event, info, PASSES_PER_SEC(1));
}

void CHARACTER::AttackHackControlEventStop()
{
	if (m_attackHackControl)
	{
		event_cancel(&m_attackHackControl);
		m_attackHackControl = nullptr;
	}
}

void CHARACTER::HackLogAdd(bool botControl)
{
	char query[128];
	if (botControl)
	{
		snprintf(query, sizeof(query), "UPDATE log.player_hack SET bot_count = bot_count + 1 WHERE pid = %d",
			GetPlayerID());
	}
	else
	{
		snprintf(query, sizeof(query), "UPDATE log.player_hack SET hack_count = hack_count + 1 WHERE pid = %d",
			GetPlayerID());
	}
	DBManager::instance().DirectQuery(query);

	Save();
	if (GetDesc()->IsPhase(PHASE_GAME))
	{
		GetDesc()->SetPhase(PHASE_CLOSE);
	}
}
#endif

#ifdef ENABLE_HIDE_COSTUME_SYSTEM
void CHARACTER::SetHideCostumeState(BYTE hideType, bool hideState)
{
	switch (hideType)
	{
		case SETTINGS_HIDE_COSTUME_HAIR:
			m_char_settings.HIDE_COSTUME_HAIR = hideState;
			break;
		case SETTINGS_HIDE_COSTUME_BODY:
			m_char_settings.HIDE_COSTUME_BODY = hideState;
			break;
		case SETTINGS_HIDE_COSTUME_WEAPON:
			m_char_settings.HIDE_COSTUME_WEAPON = hideState;
			break;
		case SETTINGS_HIDE_COSTUME_ACCE:
			m_char_settings.HIDE_COSTUME_ACCE = hideState;
			break;
		case SETTINGS_HIDE_COSTUME_AURA:
			m_char_settings.HIDE_COSTUME_AURA = hideState;
			break;
		case SETTINGS_HIDE_COSTUME_CROWN:
			m_char_settings.HIDE_COSTUME_CROWN = hideState;
			break;
		default: break;
	}
}
#endif

#ifdef ENABLE_FARM_BLOCK
void CHARACTER::SetFarmBlock(bool block, BYTE result)
{
	switch (result)
	{
		case 0:
		{
			m_farmBlock = block;
			ChatPacket(CHAT_TYPE_COMMAND, "SetFarmBlock %d", block);
			break;
		}
		case 1:
		{
			m_farmBlock = block;
			ChatPacket(CHAT_TYPE_COMMAND, "SetFarmBlock %d", block);
			NewChatPacket(MULTIFARM_DEACTIVATED);
			m_farmBlockSetTime = thecore_pulse();
			break;
		}
		case 2:
		{
			m_farmBlock = block;
			ChatPacket(CHAT_TYPE_COMMAND, "SetFarmBlock %d", block);
			NewChatPacket(MULTIFARM_ACTIVATED);
			m_farmBlockSetTime = thecore_pulse();
			break;
		}
		case 3:
		{
			NewChatPacket(MULTIFARM_MULTIACC_LIMIT);
			m_farmBlockSetTime = thecore_pulse();
			break;
		}
		case 4:
		{
			NewChatPacket(GLOBAL_ERR_OCCURED);
			break;
		}
		default:
			break;
	}

}
#endif

#ifdef ENABLE_ALIGNMENT_SYSTEM
#ifndef DISABLE_ALIGNMENT_SYSTEM
void CHARACTER::AlignmentBonusDelete(BYTE oldlevel)
{
	std::vector<std::pair<BYTE, long long>> deleteBonus;
	CAlignmentProto::instance().GetBonus(oldlevel, deleteBonus);
	for (const auto& i : deleteBonus)
	{
		PointChange(i.first, -i.second);
	}
}

void CHARACTER::AlignmentLevel(int Alignment,bool update)
{
	int alg = Alignment / 10;
	BYTE oldlevel = m_AlignmentLevel;

	if (alg < 1000)
	{
		m_AlignmentLevel = 0;
		if (update)
		{
			if (oldlevel != m_AlignmentLevel)
			{
				if (oldlevel > 0)
				{
					AlignmentBonusDelete(oldlevel);
				}
			}
		}
		return;
	}

	CAlignmentProto& ralig = CAlignmentProto::instance();
	m_AlignmentLevel = ralig.GetAlignmentLevel(alg);

	if (update)
	{
		if (oldlevel != m_AlignmentLevel)
		{
			AlignmentBonusDelete(oldlevel);
			ralig.GetBonus(m_AlignmentLevel, m_alignment_bonus);
			AlignmentBonus();
		}
	}
	else
	{
		ralig.GetBonus(m_AlignmentLevel, m_alignment_bonus);
	}
}

void CHARACTER::AlignmentBonus()
{
	if (m_AlignmentLevel < 1)
		return;

	for (const auto& i : m_alignment_bonus)
	{
		PointChange(i.first, i.second);
	}
}
#endif
#endif

#ifdef ENABLE_ITEMSHOP
long long CHARACTER::GetDragonCoin()
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT cash FROM account.account WHERE id = '%d';", GetDesc()->GetAccountTable().id));
	if (pMsg->Get()->uiNumRows == 0)
		return 0;
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	long long dc = 0;
	str_to_number(dc, row[0]);
	return dc;
}

void CHARACTER::SetDragonCoin(long long amount)
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("UPDATE account.account SET cash = '%lld' WHERE id = '%d';", amount, GetDesc()->GetAccountTable().id));
}

void CHARACTER::SetProtectTime(const std::string& flagname, int value)
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		it->second = value;
	else
		m_protection_Time.insert(make_pair(flagname, value));
}
int CHARACTER::GetProtectTime(const std::string& flagname) const
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		return it->second;
	return 0;
}

#endif

#ifdef ENABLE_TELEPORT_TO_A_FRIEND
bool CHARACTER::WarpToPlayer(LPCHARACTER victim)
{
	if(99 == g_bChannel)
	{
		victim->NewChatPacket(CANT_TELEPORT_THIS_PLAYER);
		return false;
	}

	if (GetDungeon())
	{
		victim->NewChatPacket(PARTY_REQ_ERR_10);
		return false;
	}

	WarpToPID(victim->GetPlayerID());
	return true;
}

bool CHARACTER::WarpToPlayer(LPDESC desc, DWORD pid, BYTE channel, char* targetName)
{
	if (!desc)
		return false;

	if (99 == g_bChannel || channel == 99)
	{
		desc->SetRelay(targetName);
		desc->NewChatPacket(CANT_TELEPORT_THIS_PLAYER);
		return false;
	}

	if (GetDungeon())
	{
		desc->SetRelay(targetName);
		desc->NewChatPacket(PARTY_REQ_ERR_10);
		return false;
	}

	WarpToPID(pid);
	return true;
}

bool CHARACTER::WarpToPlayerMapLevelControl(long mapIndex)
{
	switch (mapIndex)
	{
		case 1:
		case 3:
		case 21:
		case 23:
		case 41:
		case 43:
			return true;
	}

	return false;
}
#endif


#ifdef ENABLE_NEW_PET_SYSTEM
void CHARACTER::SendPetLevelUpEffect()
{
	struct packet_point_change pack;

	pack.header = HEADER_GC_CHARACTER_POINT_CHANGE;
	pack.dwVID = GetVID();
	pack.type = 1;
	pack.value = GetLevel();
	pack.amount = 1;
	PacketAround(&pack, sizeof(pack));
}

void CHARACTER::SummonNewPet()
{
	if (GetNewPetSystem())
	{
		GetNewPetSystem()->Summon(GetWear(WEAR_NEW_PET));
	}
}
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
void CHARACTER::SetSkillColor(DWORD * dwSkillColor)
{
	memcpy(m_dwSkillColor, dwSkillColor, sizeof(m_dwSkillColor));
	UpdatePacket();
}
#endif

#ifdef ENABLE_AURA_SYSTEM
void CHARACTER::OpenAura(bool bCombination)
{
	if ((GetExchange() || IsOpenSafebox() || GetShopOwner()) || IsCubeOpen() || IsAcceOpened() || IsOpenOfflineShop()
#ifdef ENABLE_6TH_7TH_ATTR
		|| Is67AttrOpen()
#endif
		)
		return;

	if (isAuraOpened(bCombination))
	{
		NewChatPacket(THIS_WINDOW_IS_ALREADY_OPEN);
		return;
	}

	if (bCombination)
	{
		if (m_bAuraAbsorption)
		{
			NewChatPacket(THIS_WINDOW_IS_ALREADY_OPEN);
			return;
		}

		m_bAuraRefine = true;
	}
	else
	{
		if (m_bAuraRefine)
		{
			NewChatPacket(THIS_WINDOW_IS_ALREADY_OPEN);
			return;
		}

		m_bAuraAbsorption = true;
	}

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAura sPacket;
	sPacket.header = HEADER_GC_AURA;
	sPacket.subheader = AURA_SUBHEADER_GC_OPEN;
	sPacket.bWindow = bCombination;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAura));

	ClearAuraMaterials();
}

void CHARACTER::CloseAura()
{
	if ((!m_bAuraRefine) && (!m_bAuraAbsorption))
		return;

	bool bWindow = (m_bAuraRefine == true ? true : false);

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAura sPacket;
	sPacket.header = HEADER_GC_AURA;
	sPacket.subheader = AURA_SUBHEADER_GC_CLOSE;
	sPacket.bWindow = bWindow;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAura));

	if (bWindow)
		m_bAuraRefine = false;
	else
		m_bAuraAbsorption = false;

	ClearAuraMaterials();
}

void CHARACTER::ClearAuraMaterials()
{
	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();
	for (int i = 0; i < AURA_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			continue;

		pkItemMaterial[i]->Lock(false);
		pkItemMaterial[i] = nullptr;
	}
}

bool CHARACTER::AuraIsSameGrade(long lGrade)
{
	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();
	if (!pkItemMaterial[0])
		return false;

	bool bReturn = (pkItemMaterial[0]->GetValue(AURA_GRADE_VALUE_FIELD) == lGrade ? true : false);
	return bReturn;
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
unsigned long long CHARACTER::GetAuraCombinePrice(long long lGrade)
{
	unsigned long long dwPrice = AURA_REFINE_PRICE;
	return dwPrice;
}
#else
DWORD CHARACTER::GetAuraCombinePrice(long lGrade)
{
	DWORD dwPrice = AURA_REFINE_PRICE;
	return dwPrice;
}
#endif

BYTE CHARACTER::CheckAuraEmptyMaterialSlot()
{
	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();
	for (int i = 0; i < AURA_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			return i;
	}

	return 255;
}

void CHARACTER::GetAuraCombineResult(DWORD& dwItemVnum, DWORD& dwMinAbs, DWORD& dwMaxAbs)
{
	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();

	if (m_bAuraRefine)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			long lVal = pkItemMaterial[0]->GetValue(AURA_TYPE_VALUE_FIELD);
			if (lVal == 6)
			{
				dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
			}
			else
			{
				DWORD dwMaskVnum = pkItemMaterial[0]->GetOriginalVnum();
				TItemTable* pTable = ITEM_MANAGER::instance().GetTable(dwMaskVnum + 1);
				if (pTable)
					dwMaskVnum += 1;

				dwItemVnum = dwMaskVnum;
			}
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
	else
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
			dwMinAbs = pkItemMaterial[0]->GetSocket(AURA_ABSORPTION_SOCKET);
			dwMaxAbs = dwMinAbs;
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
}

void CHARACTER::AddAuraMaterial(TItemPos tPos, BYTE bPos)
{
	if (bPos >= AURA_WINDOW_MAX_MATERIALS)
	{
		if (bPos == 255)
		{
			bPos = CheckEmptyMaterialSlot();
			if (bPos >= AURA_WINDOW_MAX_MATERIALS)
				return;
		}
		else
			return;
	}

	LPITEM pkItem = GetItem(tPos);
	if (!pkItem)
		return;
	else if ((pkItem->GetCell() >= INVENTORY_MAX_NUM) || (pkItem->IsEquipped()) || (pkItem->IsDragonSoul()))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME && pkItem->GetType() != COSTUME_AURA) && (bPos == 0) && (m_bAuraRefine))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME && pkItem->GetType() != COSTUME_AURA) && (bPos == 0) && (m_bAuraAbsorption))
		return;
	else if (pkItem->isLocked())
	{
		NewChatPacket(CANT_ADD_LOCKED_ITEMS);
		return;
	}
	else if ((m_bAuraRefine) && (pkItem->GetSocket(AURA_ABSORPTION_SOCKET) >= AURA_MAX_ABS))
	{
		NewChatPacket(ABSORBPTION_CHANGE_IS_MAX);
		return;
	}
	else if ((bPos == 1) && (m_bAuraAbsorption))
	{
		if ((pkItem->GetType() != ITEM_ARMOR))
		{
			NewChatPacket(AURA_INVALID_ITEM_TYPE);
			return;
		}
		else if ((pkItem->GetType() == ITEM_ARMOR) && (pkItem->GetSubType() != ARMOR_SHIELD) && (pkItem->GetSubType() != ARMOR_EAR) && (pkItem->GetSubType() != ARMOR_WRIST) && (pkItem->GetSubType() != ARMOR_NECK))
		{
			NewChatPacket(AURA_INVALID_ITEM_TYPE);
			return;
		}
	}
	else if ((bPos == 1) && (m_bAuraRefine))
	{
		if ((pkItem->GetVnum() != AURA_ICE_RUNIC && pkItem->GetVnum() != AURA_ICE_RUNIC2))
		{
			NewChatPacket(AURA_RUNIC_ITEM_NOT_FOUND);
			return;
		}
	}
	else if ((pkItem->GetSubType() != COSTUME_AURA) && (m_bAuraAbsorption))
		return;
	else if ((pkItem->GetSubType() != COSTUME_AURA) && (bPos == 0) && (m_bAuraAbsorption))
		return;
	else if ((pkItem->GetSocket(AURA_ABSORBED_SOCKET) > 0) && (bPos == 0) && (m_bAuraAbsorption))
		return;

	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();
	if ((bPos == 1) && (!pkItemMaterial[0]))
		return;

	if (pkItemMaterial[bPos])
		return;

	pkItemMaterial[bPos] = pkItem;
	pkItemMaterial[bPos]->Lock(true);

	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetAuraCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);

	TPacketAura sPacket;
	sPacket.header = HEADER_GC_AURA;
	sPacket.subheader = AURA_SUBHEADER_GC_ADDED;
	sPacket.bWindow = m_bAuraRefine == true ? true : false;
	sPacket.dwPrice = GetAuraCombinePrice(pkItem->GetValue(AURA_GRADE_VALUE_FIELD));
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = dwItemVnum;
	sPacket.dwMinAbs = dwMinAbs;
	sPacket.dwMaxAbs = dwMaxAbs;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAura));
}

void CHARACTER::RemoveAuraMaterial(BYTE bPos)
{
	if (bPos >= AURA_WINDOW_MAX_MATERIALS)
		return;

	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	unsigned long long dwPrice = 0;
#else
	DWORD dwPrice = 0;
#endif

	if (bPos == 1)
	{
		if (pkItemMaterial[bPos])
		{
			pkItemMaterial[bPos]->Lock(false);
			pkItemMaterial[bPos] = nullptr;
		}

		if (pkItemMaterial[0])
			dwPrice = GetAuraCombinePrice(pkItemMaterial[0]->GetValue(AURA_GRADE_VALUE_FIELD));
	}
	else
		ClearAuraMaterials();

	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;

	TPacketAura sPacket;
	sPacket.header = HEADER_GC_AURA;
	sPacket.subheader = AURA_SUBHEADER_GC_REMOVED;
	sPacket.bWindow = m_bAuraRefine == true ? true : false;
	sPacket.dwPrice = dwPrice;
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketAura));
}

BYTE CHARACTER::CanRefineAuraMaterials()
{
	BYTE bReturn = 0;
	LPITEM* pkItemMaterial = GetAuraMaterials();
	if (m_bAuraRefine)
	{
		for (int i = 0; i < AURA_WINDOW_MAX_MATERIALS; ++i)
		{
			if (pkItemMaterial[i] != nullptr)
			{
				if ((pkItemMaterial[i]->GetType() == ITEM_COSTUME) && (pkItemMaterial[i]->GetSubType() == COSTUME_AURA))
					bReturn = 1;
				else if ((pkItemMaterial[i]->GetVnum() == AURA_ICE_RUNIC || pkItemMaterial[i]->GetVnum() == AURA_ICE_RUNIC2))
					bReturn = 1;
				else
				{
					bReturn = 0;
					break;
				}
			}
			else
			{
				bReturn = 0;
				break;
			}
		}
	}
	else if (m_bAuraAbsorption)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			if ((pkItemMaterial[0]->GetType() == ITEM_COSTUME) && (pkItemMaterial[0]->GetSubType() == COSTUME_AURA))
				bReturn = 2;
			else
				bReturn = 0;

			if ((pkItemMaterial[1]->GetType() == ITEM_ARMOR) && ((pkItemMaterial[1]->GetSubType() == ARMOR_EAR) || (pkItemMaterial[1]->GetSubType() == ARMOR_WRIST) || (pkItemMaterial[1]->GetSubType() == ARMOR_NECK) || (pkItemMaterial[1]->GetSubType() == ARMOR_SHIELD)))
				bReturn = 2;
			else
				bReturn = 0;

			if (pkItemMaterial[0]->GetSocket(AURA_ABSORBED_SOCKET) > 0)
				bReturn = 0;
		}
		else
			bReturn = 0;
	}

	return bReturn;
}

void CHARACTER::RefineAuraMaterials()
{
	BYTE bCan = CanRefineAuraMaterials();
	if (bCan == 0)
		return;

	if ((GetExchange() || IsOpenSafebox() || GetShopOwner()) || IsCubeOpen() || IsAcceOpened() || IsOpenOfflineShop())
	{
		NewChatPacket(CLOSE_OTHER_WINDOWS_TO_DO_THIS);
		return;
	}

	LPITEM* pkItemMaterial;
	pkItemMaterial = GetAuraMaterials();

	if (!pkItemMaterial[0] || !pkItemMaterial[1])
		return;

	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetAuraCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	unsigned long long dwPrice = 0;
	if (pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) < 250)
		dwPrice = AURA_REFINE_PRICE;
	else
		dwPrice = AURA_REFINE_PRICE2;
#else
	DWORD dwPrice = AURA_REFINE_PRICE;
#endif

	if (bCan == 1)
	{
		if (GetGold() < dwPrice)
		{
			NewChatPacket(NOT_ENOUGH_YANG);
			return;
		}
		int oldGradeValue = pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD);
		bool bPacket = true;
		if (pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) >= 500)
		{
			NewChatPacket(ABSORBPTION_CHANGE_IS_MAX);
			return;
		}
		if (pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) < 250)
		{
			if (pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) != 49 && pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) != 99 && pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) != 149 && pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) != 199 && pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) != 249)
			{
				if (pkItemMaterial[1]->GetVnum() == AURA_ICE_RUNIC2)
				{
					NewChatPacket(AURA_RUNIC_ICE_SHOULD_BE_USED);
					return;
				}
				else
				{
					PointChange(POINT_GOLD, -dwPrice);
					pkItemMaterial[0]->SetSocket(AURA_ABSORPTION_SOCKET, pkItemMaterial[0]->GetSocket(AURA_ABSORPTION_SOCKET) + 1);
					pkItemMaterial[0]->SetSocket(AURA_GRADE_VALUE_FIELD, pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) + 1);
					bool bDelete = false;
					if (pkItemMaterial[1]->GetCount() - 1 < 1)
						bDelete = true;
					pkItemMaterial[1]->SetCount(pkItemMaterial[1]->GetCount() - 1);
					if (bDelete == true)
						pkItemMaterial[1] = NULL;
					NewChatPacket(IT_WAS_SUCCESFULL);
				}
			}
			else
			{
				LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(dwItemVnum, 1, 0, false);
				if (!pkItem)
				{
					sys_err("%d can't be created.", dwItemVnum);
					return;
				}
				if (pkItemMaterial[1]->GetVnum() == AURA_ICE_RUNIC2)
				{
					NewChatPacket(AURA_RUNIC_ICE_SHOULD_BE_USED);
					return;
				}
				else
				{

					ITEM_MANAGER::CopyAllAttrTo(pkItemMaterial[0], pkItem);
					pkItem->SetSocket(AURA_ABSORPTION_SOCKET, pkItem->GetSocket(AURA_ABSORPTION_SOCKET) + 1);
					pkItem->SetSocket(AURA_ABSORBED_SOCKET, pkItemMaterial[0]->GetSocket(AURA_ABSORBED_SOCKET));
					pkItem->SetSocket(AURA_GRADE_VALUE_FIELD, pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) + 1);

					PointChange(POINT_GOLD, -dwPrice);
					WORD wCell = pkItemMaterial[0]->GetCell();
					ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[0]);
					pkItemMaterial[1]->SetCount(pkItemMaterial[1]->GetCount() - 1);

					pkItem->AddToCharacter(this, TItemPos(INVENTORY, wCell));
					ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
					NewChatPacket(IT_WAS_SUCCESFULL);

					ClearAuraMaterials();
					bPacket = false;
				}
			}
		}
		else
		{
			if (pkItemMaterial[1]->GetVnum() == AURA_ICE_RUNIC)
			{
				NewChatPacket(AURA_RUNIC_FIRE_SHOULD_BE_USED);
				return;
			}
			else
			{
				PointChange(POINT_GOLD, -dwPrice);
				pkItemMaterial[0]->SetSocket(AURA_ABSORPTION_SOCKET, pkItemMaterial[0]->GetSocket(AURA_ABSORPTION_SOCKET) + 1);
				pkItemMaterial[0]->SetSocket(AURA_GRADE_VALUE_FIELD, pkItemMaterial[0]->GetSocket(AURA_GRADE_VALUE_FIELD) + 1);
				bool bDelete = false;
				if (pkItemMaterial[1]->GetCount() - 1 < 1)
					bDelete = true;
				pkItemMaterial[1]->SetCount(pkItemMaterial[1]->GetCount() - 1);
				if (bDelete == true)
					pkItemMaterial[1] = NULL;
				NewChatPacket(IT_WAS_SUCCESFULL);
			}
		}

		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;

		TPacketAura sPacket;
		sPacket.header = HEADER_GC_AURA;
		sPacket.subheader = AURA_SUBHEADER_GC_REFINED;
		sPacket.bWindow = m_bAuraRefine == true ? true : false;
		sPacket.dwPrice = dwPrice;
		if (pkItemMaterial[0])
		{
			if (bPacket)
				if (oldGradeValue != 49 && oldGradeValue != 99 && oldGradeValue != 149 && oldGradeValue != 199 && oldGradeValue != 249)
					sPacket.bPos = 255;
				else
					sPacket.bPos = 0;
			else
				sPacket.bPos = 0;
		}
		else
			sPacket.bPos = 0;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		sPacket.dwMaxAbs = 100;
		GetDesc()->Packet(&sPacket, sizeof(TPacketAura));
	}
	else
	{
		pkItemMaterial[1]->CopyAttributeTo(pkItemMaterial[0]);
		pkItemMaterial[0]->SetSocket(AURA_ABSORBED_SOCKET, pkItemMaterial[1]->GetOriginalVnum());
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (pkItemMaterial[0]->GetAttributeValue(i) < 0)
				pkItemMaterial[0]->SetForceAttribute(i, pkItemMaterial[0]->GetAttributeType(i), 0);
		}

		ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1]);

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemMaterial[0]);

		NewChatPacket(IT_WAS_SUCCESFULL);

		ClearAuraMaterials();

		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;

		TPacketAura sPacket;
		sPacket.header = HEADER_GC_AURA;
		sPacket.subheader = AURA_SUBHEADER_GC_REFINED;
		sPacket.bWindow = m_bAuraRefine == true ? true : false;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 255;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		sPacket.dwMaxAbs = 1;
		GetDesc()->Packet(&sPacket, sizeof(TPacketAura));
	}
}

bool CHARACTER::CleanAuraAttr(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
	{
		return false;
	}
	else if ((!pkItem) || (!pkTarget))
	{
		return false;
	}		
	else if ((pkTarget->GetType() != ITEM_COSTUME) && (pkTarget->GetSubType() != COSTUME_AURA))
	{
		NewChatPacket(CANT_USE_THIS_WITH_NON_AURA_ITEMS);
		return false;
	}
	if (pkTarget->GetSocket(AURA_ABSORBED_SOCKET) <= 0)
	{
		NewChatPacket(ITEM_HAS_NO_ABSORBTION);
		return false;
	}

	pkTarget->SetSocket(AURA_ABSORBED_SOCKET, 0);
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		pkTarget->SetForceAttribute(i, 0, 0);

	pkItem->SetCount(pkItem->GetCount() - 1);
	NewChatPacket(AURA_ABS_REMOVE_ATTR);
	return true;
}
#endif

#ifdef ENABLE_NAMING_SCROLL
const char* CHARACTER::GetMobNameScroll(BYTE type)
{
	if (type == MOUNT_NAME_NUM)
	{
		if (m_namingScroll[MOUNT_NAME_NUM].empty())
		{
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT naming_scroll_mount FROM player.player WHERE id = %u", GetPlayerID()));
			if (pMsg->Get()->uiNumRows == 0)
				return "";

			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			m_namingScroll[MOUNT_NAME_NUM] = row[0];
		}
	}
	else if (type == PET_NAME_NUM)
	{
		if (m_namingScroll[PET_NAME_NUM].empty())
		{
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT naming_scroll_pet FROM player.player WHERE id = %u", GetPlayerID()));

			if (pMsg->Get()->uiNumRows == 0)
				return "";

			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			m_namingScroll[PET_NAME_NUM] = row[0];
		}
	}
	else
	{
		if (m_namingScroll[BUFFI_NAME_NUM].empty())
		{
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT naming_scroll_buffi FROM player.player WHERE id = %u", GetPlayerID()));

			if (pMsg->Get()->uiNumRows == 0)
				return "";

			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			m_namingScroll[BUFFI_NAME_NUM] = row[0];
		}
	}

	return m_namingScroll[type].c_str();
}

#endif


#ifdef ENABLE_MINING_RENEWAL
BYTE CHARACTER::ComputePickaxePoint()
{
	const LPITEM wearPickaxe{ GetWear(WEAR_WEAPON) };

	if (!wearPickaxe)
		return -1;

	if (wearPickaxe->GetType() != ITEM_PICK)
		return -1;

	const DWORD pickAxeVnum{ wearPickaxe->GetVnum() };
	const int miningSkillLevel{ GetSkillLevel(SKILL_MINING) };
	int pickaxePoint{-1};

	const int incrementRatePerSkill{ IncreasePointPerSkillLevel[miningSkillLevel] };

	switch (pickAxeVnum)
	{
		case 29101:
			pickaxePoint = 10;
			break;

		case 29102:
			pickaxePoint = 12;
			break;
		case 29103:
			pickaxePoint = 14;
			break;
		case 29104:
			pickaxePoint = 20;
			break;

		default: pickaxePoint = 10; break;
	}

	pickaxePoint += incrementRatePerSkill;

	return pickaxePoint;
}

void CHARACTER::SendDiggingMotion()
{
	const LPCHARACTER lode{ GetMiningNPC() };
	const LPITEM pick{ GetWear(WEAR_WEAPON) };

	if (!m_pkMiningEvent)
		return;

	if (!lode)
		return;

	// @fixme128
	if (GetMapIndex() != lode->GetMapIndex() || DISTANCE_APPROX(GetX() - lode->GetX(), GetY() - lode->GetY()) > 1000)
		return;

	if (mining::GetDropItemFromLod(lode->GetRaceNum()) == 0)
		return;

	if (!pick || pick->GetType() != ITEM_PICK)
		return;

	if (DISTANCE_APPROX(GetX() - lode->GetX(), GetY() - lode->GetY()) > 2500)
		return;

	const BYTE neededPoint{ mining::GetMiningLodNeededPoint(lode->GetRaceNum()) };

	if (neededPoint == 0)
		return;

	TPacketGCDigMotion dig{};
	dig.header = HEADER_GC_DIG_MOTION;
	dig.vid = GetVID();
	dig.target_vid = lode->GetVID();
	dig.count = 1;

	PacketAround(&dig, sizeof(dig));

	if (GetMiningTotalPoints() >= neededPoint)
	{
		mining::OreDrop(this, lode->GetRaceNum());
		NewChatPacket(MINING_SUCCESSFULL);

#ifdef ENABLE_MINING_RENEWAL
		EndMiningEvent();
#endif
	}

}

void CHARACTER::CreateMiningEvent()
{
	LPCHARACTER lode = GetMiningNPC();
	LPITEM pick = GetWear(WEAR_WEAPON);
	int eventTime = 15;

	if (m_pkMiningEvent)
	{
		mining_cancel();
		return;
	}

	if (!lode)
		return;

	// @fixme128
	if (GetMapIndex() != lode->GetMapIndex() || DISTANCE_APPROX(GetX() - lode->GetX(), GetY() - lode->GetY()) > 1000)
		return;

	if (mining::GetDropItemFromLod(lode->GetRaceNum()) == 0)
		return;

	if (!pick || pick->GetType() != ITEM_PICK)
		return;

	if (DISTANCE_APPROX(GetX() - lode->GetX(), GetY() - lode->GetY()) > 2500)
		return;

	const BYTE neededPoint{ mining::GetMiningLodNeededPoint(lode->GetRaceNum()) };

	if (neededPoint == 0)
		return;

	ChatPacket(CHAT_TYPE_COMMAND, "MiningRenewalWindow 1 %u", neededPoint);
	m_pkMiningEvent = mining::CreateMiningEvent(this, lode, eventTime);
}

void CHARACTER::EndMiningEvent()
{
	m_bMiningTotalPoints = 0; 
	lodeNpc = nullptr;
	ChatPacket(CHAT_TYPE_COMMAND, "MiningRenewalWindow 0 -1");
}

WORD CHARACTER::GetMiningNPCVnum() const
{
	if (lodeNpc == nullptr)
		return 0;

	return lodeNpc->GetRaceNum();
}

#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
bool CHARACTER::ChangeLanguage(BYTE bLanguage)
{
	if (!IsPC())
		return false;

	if (!CanWarp())
		return false;

	TPacketChangeLanguage packet;
	packet.bHeader = HEADER_GC_REQUEST_CHANGE_LANGUAGE;
	packet.bLanguage = bLanguage;
	GetDesc()->Packet(&packet, sizeof(packet));

	GetDesc()->SetLanguage(bLanguage);
	UpdatePacket();
	return true;
}
#endif

bool CHARACTER::IsInVillage()
{
	switch (GetMapIndex())
	{
		case 1:
		case 3:
		case 21:
		case 23:
		case 41:
		case 43:
		{
			return true;
		}
		default:
			return false;
	}
	return false;
}

#ifdef ENABLE_VOTE4BUFF
void CHARACTER::CheckVoteBuff()
{
	if (FindAffect(AFFECT_VOTE4BUFF))
		return;

	if (!GetDesc() && !GetDesc()->GetAccountTable().descHwidID)
		return;

	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery("SELECT vote_buff_time, vote_buff_type FROM account.hwid_list WHERE id = %u", GetDesc()->GetAccountTable().descHwidID));
	if (msg->Get()->uiNumRows == 0)
		return;

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

	time_t buffTime = 0;
	str_to_number(buffTime, row[0]);

	if (buffTime > get_global_time())
	{
		BYTE buffType = 0;
		str_to_number(buffType, row[1]);

		buffTime = buffTime - get_global_time();
		AddVoteBuff(buffType, buffTime);
	}
	else
	{
		ChatPacket(CHAT_TYPE_COMMAND, "Vote4Buff %u", 1);
	}
}

void CHARACTER::AddVoteBuff(BYTE buffType, time_t time)
{
	switch (buffType)
	{
		case 1:
		{
			AddAffect(AFFECT_VOTE4BUFF, POINT_MAX_HP, 1000, AFF_NONE, time, 0, false);
			break;
		}
		case 2:
		{
			AddAffect(AFFECT_VOTE4BUFF, POINT_ATTBONUS_MONSTER, 10, AFF_NONE, time, 0, false);
			break;
		}
		case 3:
		{
			AddAffect(AFFECT_VOTE4BUFF, POINT_EXP_DOUBLE_BONUS, 20, AFF_NONE, time, 0, false);
			break;
		}
	}
}

void CHARACTER::SelectVoteBuff(BYTE buffType)
{
	if (!GetDesc() && !GetDesc()->GetAccountTable().descHwidID)
		return;

	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery("SELECT vote_buff_time FROM account.hwid_list WHERE id = %u", GetDesc()->GetAccountTable().descHwidID));
	if (msg->Get()->uiNumRows == 0)
		return;

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

	time_t bonusTime = 0;
	str_to_number(bonusTime, row[0]);

	if (bonusTime > get_global_time())
		return;

	bonusTime = 86400;
	AddVoteBuff(buffType, bonusTime);

	DBManager::instance().DirectQuery("UPDATE account.hwid_list SET vote_buff_time = %u, vote_buff_type = %u WHERE id = %u", (get_global_time() + bonusTime), buffType, GetDesc()->GetAccountTable().descHwidID);
}
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
void CHARACTER::SetLastReciveExtBattlePassInfoTime(DWORD time)
{
	m_dwLastReciveExtBattlePassInfoTime = time;
}

void CHARACTER::SetLastReciveExtBattlePassOpenRanking(DWORD time)
{
	m_dwLastExtBattlePassOpenRankingTime = time;
}

void CHARACTER::LoadExtBattlePass(DWORD dwCount, TPlayerExtBattlePassMission* data)
{
	m_bIsLoadedExtBattlePass = false;

	for (int i = 0; i < dwCount; ++i, ++data)
	{
		TPlayerExtBattlePassMission* newMission = new TPlayerExtBattlePassMission;
		newMission->dwPlayerId = data->dwPlayerId;
		newMission->dwBattlePassType = data->dwBattlePassType;
		newMission->dwMissionIndex = data->dwMissionIndex;
		newMission->dwMissionType = data->dwMissionType;
		newMission->dwBattlePassId = data->dwBattlePassId;
		newMission->dwExtraInfo = data->dwExtraInfo;
		newMission->bCompleted = data->bCompleted;
		newMission->bIsUpdated = data->bIsUpdated;

		m_listExtBattlePass.push_back(newMission);
	}

	m_bIsLoadedExtBattlePass = true;
}

DWORD CHARACTER::GetExtBattlePassMissionProgress(DWORD dwBattlePassType, BYTE bMissionIndex, BYTE bMissionType)
{
	DWORD BattlePassID;
	if (dwBattlePassType == 1)
		BattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
	else if (dwBattlePassType == 2)
		BattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
	else if (dwBattlePassType == 3)
		BattlePassID = CBattlePassManager::instance().GetEventBattlePassID();
	else {
		sys_err("Unknown BattlePassType (%d)", dwBattlePassType);
		return 0;
	}

	ListExtBattlePassMap::iterator it = m_listExtBattlePass.begin();
	while (it != m_listExtBattlePass.end())
	{
		TPlayerExtBattlePassMission* pkMission = *it++;
		if (pkMission->dwBattlePassType == dwBattlePassType && pkMission->dwMissionIndex == bMissionIndex && pkMission->dwMissionType == bMissionType && pkMission->dwBattlePassId == BattlePassID)
			return pkMission->dwExtraInfo;
	}
	return 0;
}

bool CHARACTER::IsExtBattlePassCompletedMission(DWORD dwBattlePassType, BYTE bMissionIndex, BYTE bMissionType)
{
	DWORD BattlePassID;
	if (dwBattlePassType == 1)
		BattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
	else if (dwBattlePassType == 2)
		BattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
	else if (dwBattlePassType == 3)
		BattlePassID = CBattlePassManager::instance().GetEventBattlePassID();
	else {
		sys_err("Unknown BattlePassType (%d)", dwBattlePassType);
		return false;
	}
	ListExtBattlePassMap::iterator it = m_listExtBattlePass.begin();
	while (it != m_listExtBattlePass.end())
	{
		TPlayerExtBattlePassMission* pkMission = *it++;
		if (pkMission->dwBattlePassType == dwBattlePassType && pkMission->dwMissionIndex == bMissionIndex && pkMission->dwMissionType == bMissionType && pkMission->dwBattlePassId == BattlePassID)
			return (pkMission->bCompleted ? true : false);
	}
	return false;
}

bool CHARACTER::IsExtBattlePassRegistered(BYTE bBattlePassType, DWORD dwBattlePassID)
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT * FROM player.battlepass_playerindex WHERE player_id = %d and battlepass_type = %d and battlepass_id = %d", GetPlayerID(), bBattlePassType, dwBattlePassID));
	if (pMsg->Get()->uiNumRows != 0)
		return true;

	return false;
}

void CHARACTER::UpdateExtBattlePassMissionProgress(DWORD dwMissionType, DWORD dwUpdateValue, DWORD dwCondition, bool isOverride)
{
	if (!GetDesc())
		return;

	if (!m_bIsLoadedExtBattlePass)
		return;

	DWORD dwSafeCondition = dwCondition;
	for (BYTE bBattlePassType = 1; bBattlePassType <= 3; ++bBattlePassType)
	{
		bool foundMission = false;
		DWORD dwSaveProgress = 0;
		dwCondition = dwSafeCondition;

		BYTE bBattlePassID;
		BYTE bMissionIndex = CBattlePassManager::instance().GetMissionIndex(bBattlePassType, dwMissionType, dwCondition);

		if (bBattlePassType == 1)
			bBattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
		if (bBattlePassType == 2)
		{
			bBattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
			//CAffect* pAffect = FindAffect(AFFECT_BATTLEPASS);
			if (!FindAffect(AFFECT_BATTLEPASS))
				continue;

			/*if (bBattlePassID != GetExtBattlePassPremiumID())
				continue;*/
		}

		if (bBattlePassType == 3)
			bBattlePassID = CBattlePassManager::instance().GetEventBattlePassID();

		DWORD dwFirstInfo, dwSecondInfo;
		if (CBattlePassManager::instance().BattlePassMissionGetInfo(bBattlePassType, bMissionIndex, bBattlePassID, dwMissionType, &dwFirstInfo, &dwSecondInfo))
		{
			if (dwFirstInfo == 0)
				dwCondition = 0;

			if ((dwMissionType == 2 && dwFirstInfo <= dwCondition) || (dwMissionType == 4 && dwFirstInfo <= dwCondition))  /*|| dwMissionType == 20 && dwFirstInfo <= dwCondition*/
				dwCondition = dwFirstInfo;

			if (dwFirstInfo == dwCondition && GetExtBattlePassMissionProgress(bBattlePassType, bMissionIndex, dwMissionType) < dwSecondInfo)
			{
				ListExtBattlePassMap::iterator it = m_listExtBattlePass.begin();
				while (it != m_listExtBattlePass.end())
				{
					TPlayerExtBattlePassMission* pkMission = *it++;

					if (pkMission->dwBattlePassType == bBattlePassType && pkMission->dwMissionIndex == bMissionIndex && pkMission->dwMissionType == dwMissionType && pkMission->dwBattlePassId == bBattlePassID)
					{
						pkMission->bIsUpdated = 1;

						if (isOverride)
							pkMission->dwExtraInfo = dwUpdateValue;
						else
							pkMission->dwExtraInfo += dwUpdateValue;

						if (pkMission->dwExtraInfo >= dwSecondInfo)
						{
							pkMission->dwExtraInfo = dwSecondInfo;
							pkMission->bCompleted = 1;

							std::string stMissionName = CBattlePassManager::instance().GetMissionNameByType(pkMission->dwMissionType);
							std::string stBattlePassName = CBattlePassManager::instance().GetNormalBattlePassNameByID(pkMission->dwBattlePassId);

							CBattlePassManager::instance().BattlePassRewardMission(this, bBattlePassType, bBattlePassID, bMissionIndex);
							if (bBattlePassType == 1) 
							{
								ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_NORMAL_MISSION"));
							}
							if (bBattlePassType == 2) 
							{
								ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT( "BATTLEPASS_COMPLETE_PREMIUM_MISSION"));
							}
							if (bBattlePassType == 3) 
							{
								ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_EVENT_MISSION"));
							}

							TPacketGCExtBattlePassMissionUpdate packet;
							packet.bHeader = HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE;
							packet.bBattlePassType = bBattlePassType;
							packet.bMissionIndex = bMissionIndex;
							packet.dwNewProgress = pkMission->dwExtraInfo;
							GetDesc()->Packet(&packet, sizeof(TPacketGCExtBattlePassMissionUpdate));
						}

						dwSaveProgress = pkMission->dwExtraInfo;
						foundMission = true;

						if (pkMission->bCompleted != 1) {
							TPacketGCExtBattlePassMissionUpdate packet;
							packet.bHeader = HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE;
							packet.bBattlePassType = bBattlePassType;
							packet.bMissionIndex = bMissionIndex;
							packet.dwNewProgress = dwSaveProgress;
							GetDesc()->Packet(&packet, sizeof(TPacketGCExtBattlePassMissionUpdate));
						}
						break;
					}

				}

				if (!foundMission)
				{
					if (!IsExtBattlePassRegistered(bBattlePassType, bBattlePassID))
						DBManager::instance().DirectQuery("INSERT INTO player.battlepass_playerindex SET player_id=%d, player_name='%s', battlepass_type=%d, battlepass_id=%d, start_time=NOW()", GetPlayerID(), GetName(), bBattlePassType, bBattlePassID);

					TPlayerExtBattlePassMission* newMission = new TPlayerExtBattlePassMission;
					newMission->dwPlayerId = GetPlayerID();
					newMission->dwBattlePassType = bBattlePassType;
					newMission->dwMissionType = dwMissionType;
					newMission->dwBattlePassId = bBattlePassID;

					if (dwUpdateValue >= dwSecondInfo)
					{
						newMission->dwMissionIndex = CBattlePassManager::instance().GetMissionIndex(bBattlePassType, dwMissionType, dwCondition);
						newMission->dwExtraInfo = dwSecondInfo;
						newMission->bCompleted = 1;

						CBattlePassManager::instance().BattlePassRewardMission(this, bBattlePassType, bBattlePassID, bMissionIndex);
						if (bBattlePassType == 1) {
							EffectPacket(SE_EFFECT_BP_NORMAL_MISSION_COMPLETED);
							ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_NORMAL_MISSION"));
						}
						if (bBattlePassType == 2) {
							EffectPacket(SE_EFFECT_BP_PREMIUM_MISSION_COMPLETED);
							ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_PREMIUM_MISSION"));
						}
						if (bBattlePassType == 3) {
							EffectPacket(SE_EFFECT_BP_EVENT_MISSION_COMPLETED);
							ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_EVENT_MISSION"));
						}

						dwSaveProgress = dwSecondInfo;
					}
					else
					{
						newMission->dwMissionIndex = CBattlePassManager::instance().GetMissionIndex(bBattlePassType, dwMissionType, dwCondition);
						newMission->dwExtraInfo = dwUpdateValue;
						newMission->bCompleted = 0;

						dwSaveProgress = dwUpdateValue;
					}

					newMission->bIsUpdated = 1;

					m_listExtBattlePass.push_back(newMission);

					TPacketGCExtBattlePassMissionUpdate packet;
					packet.bHeader = HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE;
					packet.bBattlePassType = bBattlePassType;
					packet.bMissionIndex = bMissionIndex;
					packet.dwNewProgress = dwSaveProgress;
					GetDesc()->Packet(&packet, sizeof(TPacketGCExtBattlePassMissionUpdate));
				}
			}
		}
	}
}

void CHARACTER::SetExtBattlePassMissionProgress(BYTE bBattlePassType, DWORD dwMissionIndex, DWORD dwMissionType, DWORD dwUpdateValue)
{
	if (!GetDesc())
		return;

	if (!m_bIsLoadedExtBattlePass)
		return;

	bool foundMission = false;
	DWORD dwSaveProgress = 0;

	BYTE bBattlePassID;
	if (bBattlePassType == 1)
		bBattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
	else if (bBattlePassType == 2)
		bBattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
	else if (bBattlePassType == 3)
		bBattlePassID = CBattlePassManager::instance().GetEventBattlePassID();
	else {
		sys_err("Unknown BattlePassType (%d)", bBattlePassType);
		return;
	}

	DWORD dwFirstInfo, dwSecondInfo;
	if (CBattlePassManager::instance().BattlePassMissionGetInfo(bBattlePassType, dwMissionIndex, bBattlePassID, dwMissionType, &dwFirstInfo, &dwSecondInfo))
	{
		ListExtBattlePassMap::iterator it = m_listExtBattlePass.begin();
		while (it != m_listExtBattlePass.end())
		{
			TPlayerExtBattlePassMission* pkMission = *it++;

			if (pkMission->dwBattlePassType == bBattlePassType && pkMission->dwMissionIndex == dwMissionIndex && pkMission->dwMissionType == dwMissionType && pkMission->dwBattlePassId == bBattlePassID)
			{
				pkMission->bIsUpdated = 1;
				pkMission->bCompleted = 0;

				pkMission->dwExtraInfo = dwUpdateValue;

				if (pkMission->dwExtraInfo >= dwSecondInfo)
				{
					pkMission->dwExtraInfo = dwSecondInfo;
					pkMission->bCompleted = 1;

					std::string stMissionName = CBattlePassManager::instance().GetMissionNameByType(pkMission->dwMissionType);
					std::string stBattlePassName = CBattlePassManager::instance().GetNormalBattlePassNameByID(pkMission->dwBattlePassId);
					//ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("New Value : %d"), pkMission->dwExtraInfo);

					CBattlePassManager::instance().BattlePassRewardMission(this, bBattlePassType, bBattlePassID, dwMissionIndex);
					if (bBattlePassType == 1) {
						EffectPacket(SE_EFFECT_BP_NORMAL_MISSION_COMPLETED);
						ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_NORMAL_MISSION"));
					}
					if (bBattlePassType == 2) {
						EffectPacket(SE_EFFECT_BP_PREMIUM_MISSION_COMPLETED);
						ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_PREMIUM_MISSION"));
					}
					if (bBattlePassType == 3) {
						EffectPacket(SE_EFFECT_BP_EVENT_MISSION_COMPLETED);
						ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_EVENT_MISSION"));
					}

					TPacketGCExtBattlePassMissionUpdate packet;
					packet.bHeader = HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE;
					packet.bBattlePassType = bBattlePassType;
					packet.bMissionIndex = dwMissionIndex;
					packet.dwNewProgress = pkMission->dwExtraInfo;
					GetDesc()->Packet(&packet, sizeof(TPacketGCExtBattlePassMissionUpdate));
				}

				dwSaveProgress = pkMission->dwExtraInfo;
				foundMission = true;

				if (pkMission->bCompleted != 1) {
					TPacketGCExtBattlePassMissionUpdate packet;
					packet.bHeader = HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE;
					packet.bBattlePassType = bBattlePassType;
					packet.bMissionIndex = dwMissionIndex;
					packet.dwNewProgress = dwSaveProgress;
					GetDesc()->Packet(&packet, sizeof(TPacketGCExtBattlePassMissionUpdate));
				}
				break;
			}
		}

		if (!foundMission)
		{
			if (!IsExtBattlePassRegistered(bBattlePassType, bBattlePassID))
				DBManager::instance().DirectQuery("INSERT INTO player.battlepass_playerindex SET player_id=%d, player_name='%s', battlepass_type=%d, battlepass_id=%d, start_time=NOW()", GetPlayerID(), GetName(), bBattlePassType, bBattlePassID);

			TPlayerExtBattlePassMission* newMission = new TPlayerExtBattlePassMission;
			newMission->dwPlayerId = GetPlayerID();
			newMission->dwBattlePassType = bBattlePassType;
			newMission->dwMissionType = dwMissionType;
			newMission->dwBattlePassId = bBattlePassID;

			if (dwUpdateValue >= dwSecondInfo)
			{
				newMission->dwMissionIndex = dwMissionIndex;
				newMission->dwExtraInfo = dwSecondInfo;
				newMission->bCompleted = 1;

				CBattlePassManager::instance().BattlePassRewardMission(this, bBattlePassType, bBattlePassID, dwMissionIndex);
				if (bBattlePassType == 1) {
					EffectPacket(SE_EFFECT_BP_NORMAL_MISSION_COMPLETED);
					ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_NORMAL_MISSION"));
				}
				if (bBattlePassType == 2) {
					EffectPacket(SE_EFFECT_BP_PREMIUM_MISSION_COMPLETED);
					ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("BATTLEPASS_COMPLETE_PREMIUM_MISSION"));
				}
				if (bBattlePassType == 3) {
					EffectPacket(SE_EFFECT_BP_EVENT_MISSION_COMPLETED);
					ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT( "BATTLEPASS_COMPLETE_EVENT_MISSION"));
				}

				dwSaveProgress = dwSecondInfo;
			}
			else
			{
				newMission->dwMissionIndex = dwMissionIndex;
				newMission->dwExtraInfo = dwUpdateValue;
				newMission->bCompleted = 0;

				dwSaveProgress = dwUpdateValue;
			}

			newMission->bIsUpdated = 1;

			m_listExtBattlePass.push_back(newMission);

			TPacketGCExtBattlePassMissionUpdate packet;
			packet.bHeader = HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE;
			packet.bBattlePassType = bBattlePassType;
			packet.bMissionIndex = dwMissionIndex;
			packet.dwNewProgress = dwSaveProgress;
			GetDesc()->Packet(&packet, sizeof(TPacketGCExtBattlePassMissionUpdate));
		}
	}
}
#endif

#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
int CHARACTER::GetBookVnumBySkillIndex(int skillIndex)
{
	switch (skillIndex)
	{
		case 180: {skillIndex = 94; break; }
		case 181: {skillIndex = 95; break; }
		case 182: {skillIndex = 96; break; }
		case 183: {skillIndex = 110; break; }
		case 184: {skillIndex = 111; break; }
	}
	if (skillIndex)
		return (50400 + skillIndex);
	return -1;
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
void CHARACTER::Open67Attr()
{
	LPDESC d = GetDesc();
	if (!d)
		return;
	
	if ((GetExchange() || IsOpenSafebox() || GetShopOwner()) || IsCubeOpen() || IsAcceOpened() || IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
		|| isAuraOpened(true) || isAuraOpened(false)
#endif
		)
		return;

	Set67Attr(true);

	TPacket67AttrOpenClose p;
	p.bHeader = HEADER_GC_OPEN_67_ATTR;
	d->Packet(&p, sizeof(p));
}
#endif

#ifdef ENABLE_BUFFI_SYSTEM
void CHARACTER::SendBuffiSkillPacket(DWORD skill_vnum)
{
	TPGCBuffiSkill pack;
	pack.header = HEADER_GC_BUFFI;
	pack.skillVnum = skill_vnum;
	pack.vid = GetVID();
	PacketView(&pack, sizeof(TPGCBuffiSkill), this);
}
#endif

#ifdef ENABLE_BOOSTER_ITEM
void CHARACTER::CalcuteBoosterSetBonus()
{
	if (GetWear(WEAR_BOOSTER_BODY) && GetWear(WEAR_BOOSTER_WEAPON) && GetWear(WEAR_BOOSTER_HEAD))
	{
		m_boosterSetBonus = true;
	}
	else
	{
		m_boosterSetBonus = false;
	}
	ComputePoints();
}

void CHARACTER::BoosterGiveBuff()
{
	LPITEM item = nullptr;
	if ((item = GetWear(WEAR_WEAPON)))
	{
		const int rate = item->BonusRate();
		if (rate != -1)
		{
			int gradeValue = item->GetValue(4) + item->GetValue(5);
			if (gradeValue > 0)
			{
				PointChange(POINT_ATT_GRADE_BONUS, ((gradeValue * rate) / 100));
			}
		}
	}

	if ((item = GetWear(WEAR_BODY)))
	{
		const int rate = item->BonusRate();
		if (rate != -1)
		{
			int gradeValue = item->GetValue(1);
			gradeValue += (2 * item->GetValue(5));
			if (gradeValue > 0)
			{
				PointChange(POINT_DEF_GRADE_BONUS, ((gradeValue * rate) / 100));
			}		
		}
	}

	if ((item = GetWear(WEAR_HEAD)))
	{
		const int rate = item->BonusRate();
		if (rate != -1)
		{
			int gradeValue = item->GetValue(1);
			gradeValue += (2 * item->GetValue(5));
			if (gradeValue > 0)
			{
				PointChange(POINT_DEF_GRADE_BONUS, ((gradeValue * rate) / 100));
			}
		}
	}
}
#endif

#ifdef ENABLE_INCREASED_MOV_SPEED_MOBS
int CHARACTER::GetIncreasedSpeed() const
{
	if (IsMonster() && GetMobRank() < MOB_RANK_BOSS)
		return 250;

	else if (GetMobRank() == MOB_RANK_BOSS)
		return 25;

	return 0;
}
#endif


#ifdef ENABLE_FURKANA_GOTTEN
void CHARACTER::DropCalculator(DWORD mobVnum, int mobCount)
{
	LPCHARACTER mob = CHARACTER_MANAGER::instance().SpawnMob(mobVnum, GetMapIndex(), GetX() + 50, GetY() + 50, 0);
	static std::map<DWORD, int> map_drop;
	map_drop.clear();
	ITEM_MANAGER::Instance().DropCalculator(mob, this, map_drop, mobCount);
	ChatPacket(1, "%d mob vnum drop begin", mobVnum);

	for (const auto& it : map_drop)
	{
		TItemTable* item = ITEM_MANAGER::instance().GetTable(it.first);
		if (!item)
			continue;

		ChatPacket(1, "%s x %d", item->szLocaleName, it.second);
	}
	
	ChatPacket(1, "%d mob vnum drop end", mobVnum);
	M2_DESTROY_CHARACTER(mob);
}
#endif
void CHARACTER::TestYapcam31(BYTE value)
{
	ChatPacket(1, "%d POINT_RESIST_NORMAL_DAMAGE", GetPoint(POINT_RESIST_NORMAL_DAMAGE));

}

#ifdef ENABLE_AFTERDEATH_SHIELD
EVENTINFO(shield_countdown_data)
{
	int seconds;
	LPCHARACTER ch;
};

static LPEVENT timer_shieldcountdown_check = NULL;

EVENTFUNC(ShieldCountdown_Timer_Event)
{
	shield_countdown_data* info = dynamic_cast<shield_countdown_data*>(event->info);

	if (info == NULL)
	{
		sys_err("Afterdeath Shield - ERROR: ShieldCountdown_Timer_Event> <Factor> Time 0 - Error");
		return 0;
	}

	int * pTimer = &(info->seconds);
	LPCHARACTER ch = info->ch;
	
	if(ch == NULL || ch->GetDesc() == NULL || !ch->IsAffectFlag(AFF_AFTERDEATH_SHIELD))
	{
		if(!ch->IsAffectFlag(AFF_AFTERDEATH_SHIELD))
			ch->UpdatePacket();
		timer_shieldcountdown_check = NULL;
		return 0;
	}

	if (*pTimer == 0)
	{
		ch->RemoveShieldAffect();
		ch->UpdatePacket();
	}
	else
	{
		--*pTimer;
		return passes_per_sec;
	}

	timer_shieldcountdown_check = NULL;
	return 0;
}

void CHARACTER::StartShieldCountdownEvent(LPCHARACTER ch, int iSec)
{

	if(ch == NULL || ch->GetDesc() == NULL)
		return;

	shield_countdown_data* info = AllocEventInfo<shield_countdown_data>();
	info->seconds = iSec;
	info->ch = ch;
	timer_shieldcountdown_check = event_create(ShieldCountdown_Timer_Event, info, 1);
}
#endif

#ifdef __AUTO_HUNT__
void CHARACTER::SetAutoHuntStatus(bool bStatus, bool bMobFarm, bool bMetinFarm)
{
	m_bAutoHuntStatus = bStatus;
	RemoveAffect(AFFECT_AUTO_HUNT_AFFECT);
	if (bStatus)
		AddAffect(AFFECT_AUTO_HUNT_AFFECT, POINT_NONE, 0, AFF_AUTO_HUNT, INFINITE_AFFECT_DURATION, 0, false);
	ChatPacket(CHAT_TYPE_COMMAND, "AutoHuntStatus %d %d %d", bStatus, bMobFarm, bMetinFarm);
}
void CHARACTER::GetAutoHuntCommand(const char* szArgument)
{
	std::vector<std::string> vecArgs;
	split_argument(szArgument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	else if (vecArgs[1] == "end")
	{
		SetAutoHuntStatus(false);
	}
	else if (vecArgs[1] == "start")
	{
		if (vecArgs.size() < 7) { return; }
		if (!IsAutoHuntAffectHas())
		{
			ChatPacket(CHAT_TYPE_INFO, "You don't have auto hunt affect.");
			return;
		}
		SetAutoHuntStatus(true, vecArgs[5] == "1" ? true : false, vecArgs[6] == "1" ? true : false);
	}
	ChatPacket(CHAT_TYPE_COMMAND, "auto_hunt %s", szArgument);
}
bool CHARACTER::IsAutoHuntAffectHas()
{
	return FindAffect(AFFECT_AUTO_HUNT) != NULL ? true : false;
}
#endif

#ifdef ENABLE_RENEWAL_PVP
bool CHARACTER::IsInFight()
{
	return CPVPManager::Instance().IsFighting(this);
}
bool CHARACTER::IsBlockPvP(DWORD itemVnum)
{
	if (itemVnum >= 53001 && itemVnum <= 53100 && pvpSettings[PVP_PET] == false)
		return true;
	else if (itemVnum >= 55700 && itemVnum <= 55750 && pvpSettings[PVP_NEW_PET] == false)
		return true;

	std::map<DWORD, DWORD> pvp_data = {
		{72723,PVP_HP_ELIXIR},
		{72724,PVP_HP_ELIXIR},
		{72725,PVP_HP_ELIXIR},
		{72726,PVP_HP_ELIXIR},
		{50826,PVP_WHITE_DEW},
		{50823,PVP_YELLOW_DEW},
		{50822,PVP_ORANGE_DEW},

		{50821,PVP_RED_DEW},
		{50825,PVP_BLUE_DEW},
		{50824,PVP_GREEN_DEW},

		{39018,PVP_DRAGON_GOD_ATTACK},
		{71028,PVP_DRAGON_GOD_ATTACK},

		{39020,PVP_DRAGON_GOD_DEFENCE},
		{71030,PVP_DRAGON_GOD_DEFENCE},

		{39017,PVP_DRAGON_GOD_LIFE},
		{71027,PVP_DRAGON_GOD_LIFE},

		{39025,PVP_PIERCING_STRIKE},
		{71045,PVP_PIERCING_STRIKE},

		{39024,PVP_CRITICAL_STRIKE},
		{71044,PVP_CRITICAL_STRIKE},

		{50817,PVP_ZIN_WATER},
		{50818,PVP_SAMBO_WATER},

		{27868,PVP_ATTACKSPEED_FISH},
		{51002,PVP_ENERGY},
	};
	auto it = pvp_data.find(itemVnum);
	if (it != pvp_data.end())
	{
		if (pvpSettings[it->second] == false)
			return true;
	}
	return false;
}
bool CHARACTER::CheckPvPSetting(BYTE settingIndex)
{
	if (!IsInFight())
		return false;
	return pvpSettings[settingIndex] == false;
}
bool CHARACTER::CheckPvPUse(DWORD itemVnum)
{
	if (!IsInFight())
		return false;
	return IsBlockPvP(itemVnum);
}
void CHARACTER::CheckPvPBonus(bool isAdd, bool* pvpSettingNew)
{
	if (isAdd)
	{
		thecore_memcpy(&pvpSettings, pvpSettingNew, sizeof(pvpSettings));
		//Close items
		for (DWORD j = 0; j < INVENTORY_MAX_NUM; ++j)
		{
			LPITEM item = GetInventoryItem(j);
			if (item != NULL)
			{
				if (item->GetSocket(0) != 1)
					continue;
				else if(IsBlockPvP(item->GetVnum()))
					UseItem(TItemPos(INVENTORY, item->GetCell()));
			}
		}

		LPITEM item = NULL;

		if (pvpSettings[PVP_NEW_PET] == false)
		{
			item = GetWear(WEAR_PET);
			if (item != NULL)
				UnequipItem(item);
		}

		if (pvpSettings[PVP_PET] == false)
		{
			item = GetWear(WEAR_PET_SKIN);
			if (item != NULL)
				UnequipItem(item);
		}

		if (pvpSettings[PVP_BUFFI_SKILLS] == false)
		{
			RemoveAffect(SKILL_PAERYONG);
			RemoveAffect(SKILL_REFLECT);
			RemoveAffect(SKILL_GICHEON);
			RemoveAffect(SKILL_JEONGEOP);
			RemoveAffect(SKILL_HOSIN);
			RemoveAffect(SKILL_KWAESOK);
			RemoveAffect(SKILL_JEUNGRYEOK);
		}

		CAffect* affect = NULL;
		
		if (pvpSettings[PVP_HP_ELIXIR] == false)
{
    affect = FindAffect(AFFECT_AUTO_HP_RECOVERY);
    if (affect != NULL)
    {
        affect->bActive = false; // Disable it instead of removing
    }
}

		if (pvpSettings[PVP_POISONING] == false)
		{
			affect = FindAffect(AFFECT_POISON);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_WHITE_DEW] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_DEF_GRADE_BONUS].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_YELLOW_DEW] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_ATT_SPEED].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_ORANGE_DEW] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_PENETRATE_PCT].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_RED_DEW] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_CRITICAL_PCT].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_BLUE_DEW] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_ATT_GRADE_BONUS].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_ATTACKSPEED_FISH] == false)
		{
			affect = FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[APPLY_ATT_SPEED].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}
		
		if (pvpSettings[PVP_GREEN_DEW] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_RESIST_MAGIC].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_ZIN_WATER] == false)
		{
			affect = FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[APPLY_ATT_GRADE_BONUS].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_SAMBO_WATER] == false)
		{
			affect = FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[APPLY_DEF_GRADE_BONUS].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_ATTACKSPEED_FISH] == false)
		{
			affect = FindAffect(AFFECT_ATT_SPEED, aApplyInfo[APPLY_ATT_SPEED].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_ENERGY] == false)
		{
			affect = FindAffect(AFFECT_BLEND, aApplyInfo[APPLY_ENERGY].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_DRAGON_GOD_ATTACK] == false)
		{
			affect = FindAffect(510, aApplyInfo[86].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_DRAGON_GOD_DEFENCE] == false)
		{
			affect = FindAffect(510, aApplyInfo[65].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_DRAGON_GOD_LIFE] == false)
		{
			affect = FindAffect(510, aApplyInfo[69].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}


		if (pvpSettings[PVP_PIERCING_STRIKE] == false)
		{
			affect = FindAffect(510, aApplyInfo[16].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_CRITICAL_STRIKE] == false)
		{
			affect = FindAffect(510, aApplyInfo[15].bPointType);
			if (affect != NULL)
				RemoveAffect(affect);
		}

		if (pvpSettings[PVP_HALF_HUMAN] == true)
		{
			auto affect = FindAffect(AFFECT_PVP_SETTINGS);
			if (affect)
				RemoveAffect(affect);
			AddAffect(AFFECT_PVP_SETTINGS, POINT_ATTBONUS_HUMAN, -50, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true);
		}
	}
	else
	{
		auto affect = FindAffect(AFFECT_PVP_SETTINGS);
		if (affect)
			RemoveAffect(affect);
	}
}
#endif