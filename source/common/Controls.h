#pragma once
//// TEST
/*
__author__ = MDproject
__discord__ = MDproject
__project__ = MDproject
*/
//#define ENABLE_DIZCIYA_GOTTEN														// Dev Server
#define ENABLE_FURKANA_GOTTEN														// Dev Server

//#ifdef ENABLE_DIZCIYA_GOTTEN
//#define IMPROVED_PACKET_ENCRYPTION_REMOVE											// Dev server Removing Packet Enc
//#else
#define _IMPROVED_PACKET_ENCRYPTION_												// Packed Encryption
//#endif

enum eConfig {
	MAP_ALLOW_LIMIT = 32, // 32 defaultfdead
};
#define GetGoldMultipler() (1)
/********************************************** No Touch Defines Start **********************************************/

#define __PET_SYSTEM__																		// Official Pet System
#define __UDP_BLOCK__																		// UDP Block
#define ENABLE_PORT_SECURITY																// Port Security
/********************************************** End of No Touch **********************************************/


/********************************************** General Defines Start **********************************************/
#define ENABLE_FULL_NOTICE																	// Full Notice
#define ENABLE_NEWSTUFF																		// New Stuff from Martysama
#define ENABLE_BELT_INVENTORY_EX															// Belt Inventory Stuff
#define ENABLE_PLAYER_PER_ACCOUNT5															// Max player per accounts module
#define ENABLE_OBJ_SCALLING																	// Object Scale
#define ENABLE_DICE_SYSTEM																	// Official Dice
#define ENABLE_EXTEND_INVEN_SYSTEM															// 5 Pages Of Inventory
#define ENABLE_D_NJGUILD																	// Guild Dungeon Stuff
#define ENABLE_WOLFMAN_CHARACTER															// Wolfman Stuff
#define ENABLE_HIGHLIGHT_SYSTEM																// Highlight Items
#define ENABLE_KILL_EVENT_FIX																// Kill Lua event 0 exp problem
#define ENABLE_TARGET_INFORMATION_SYSTEM													// Target Information System
#define ENABLE_TARGET_INFORMATION_RENEWAL													// Target Information System
#define ENABLE_SHOW_TARGET_HP																// Target HP Information
#define ENABLE_TARGET_ELEMENT_SYSTEM														// Target Element Information
#define ENABLE_CHANNEL_SWITCH_SYSTEM														// Channel switch
#define ENABLE_DROP_DIALOG_EXTENDED_SYSTEM													// Sell-Delete Dialog
#define ENABLE_CUBE_RENEWAL_WORLDARD														// Official Cube
#define ENABLE_SPECIAL_STORAGE																// Special Storage
#define ENABLE_EXTENDED_COUNT_ITEMS															// Item count renewal
#define ENABLE_EXTENDED_YANG_LIMIT															// Yang limit renewal
#define ENABLE_SPLIT_ITEMS_FAST																// Multiple split items
#define ENABLE_SORT_INVENTORIES																// Sort Inventories
#define ENABLE_QUEST_CATEGORY																// Official Quest Category
#define ENABLE_OFFLINE_SHOP																	// OfflineShop Reworked
#define ENABLE_SHOP_AUTO_STACK																// Buy autostack for all inventories
#define ENABLE_CHECKINOUT_UPDATE															// Storage - Exchange Check-in Out
#define ENABLE_SWITCHBOT																	// Serverside Switchbot
#define ENABLE_CHEST_OPEN_RENEWAL															// Open chest renewal
#define ENABLE_INVENTORY_REWORK																// Inventory rework
#define __GAME_OPTION_ESCAPE__ 																// Game Option (Escape)

#define ENABLE_MULTI_LANGUAGE_SYSTEM														// Multi language system
#define ENABLE_EXTENDED_WHISPER_DETAILS														// Extended whisper details
#define ENABLE_SKILL_DESCRIPTION_RENEWAL													//Description Of Book Remaining

#define OX_EVENT_DOUBLE_CONNECT_DISABLE														//OX DOUBLE CONNECTION REMOVE (IP)
#define BOSS_DAMAGE_RANKING_PLUGIN															// Enable Damage Ranking Boss
#define __SKILL_TRAINING__																	// SKILL SELECT RENDER TARGET
#define ENABLE_PLAYER_BLOCK_SYSTEM															//Player Block System
#define ENABLE_WHISPER_RENEWAL																//Typing Whisper...
#define __STONE_SCROLL__																	// Remove Stone from Item
#define ENABLE_AFK_MODE_SYSTEM																// AFK MODE
#define __AUTO_HUNT__																		// Auto Hunt System
#define ENABLE_RENEWAL_PVP																	// New Duel PvP
#define ENABLE_EXPRESSING_EMOTION															// NEW EMOTION
#define __BL_KILL_BAR__																		// Kill Bar Icon

#define ENABLE_PLAYER_STATS_SYSTEM															// Character Stats
#define ENABLE_GLOBAL_RANKING																// Global Ranking System
#define ENABLE_POWER_RANKING

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	#define MAX_COUNT DWORD	// (uint16_t = 65535)
	#define SET_MAX_COUNT 2000 // This needs to be less or equal to the max value of MAX_COUNT
#endif

#define ENABLE_SOCKET_LIMIT_5																//Socket Limiti

#define ENABLE_MAGIC_REDUCTION_SYSTEM														// Magic Reduction Apply

#define ENABLE_BIOLOG_SYSTEM																// Biolog System
#define ENABLE_CHAR_SETTINGS																//Character settings
#define ENABLE_ITEM_TRANSACTIONS															// Item transactions
#define ENABLE_LMW_PROTECTION																// Anti Cheat Solution
#define ENABLE_BOT_CONTROL
#define ENABLE_ALIGNMENT_SYSTEM																// Alignment System
#define ENABLE_EXTRA_APPLY_BONUS															// New apply bonuses

 /* KraKerYas */

#define ENABLE_EVENT_MANAGER																// Event Manager
#ifdef ENABLE_EVENT_MANAGER

#define ENABLE_MINI_GAME_OKEY_NORMAL
#define ENABLE_MINI_GAME_BNW
#define ENABLE_MINI_GAME_CATCH_KING
#endif

#define ENABLE_ITEMSHOP																		// ItemShop Ingame
#ifdef ENABLE_ITEMSHOP
#define ENABLE_ITEMSHOP_TO_INVENTORY														// buying item directly from inventory


// ITEMSHOP CONFIG
constexpr int wheelDCPrice{15};

constexpr DWORD wheelSpinTicketVnum{ 200300 };
constexpr int wheelSpinWithTicketPrice{ 50 };

#endif


 /* KraKerYas */

#ifdef ENABLE_WOLFMAN_CHARACTER
#define USE_MOB_BLEEDING_AS_POISON															// Enable Bleeding
#define USE_MOB_CLAW_AS_DAGGER																// Claw = Dagger Module
// #define USE_ITEM_BLEEDING_AS_POISON
// #define USE_ITEM_CLAW_AS_DAGGER
//#define USE_WOLFMAN_STONES
#define USE_WOLFMAN_BOOKS
#endif

#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
// #define USE_MAGIC_REDUCTION_STONES
#endif

#define ENABLE_AFTERDEATH_SHIELD
#ifdef ENABLE_AFTERDEATH_SHIELD
#define AFTERDEATH_SHIELD_DURATION 15 //Duration of shield protection (in secs).
#endif

#ifdef ENABLE_OFFLINE_SHOP
#define ENABLE_SHOPS_IN_CITIES
#define ENABLE_OFFLINESHOP_NOTIFICATION
#define ENABLE_SHOP_DECORATION
#define ENABLE_IRA_REWORK
#define ENABLE_LARGE_DYNAMIC_PACKET
#define ENABLE_OFFLINE_SHOP_LOG
#define ENABLE_OFFLINE_SHOP_GRID
#define ENABLE_OFFLINESHOP_EXTEND_TIME														// Extend shop time
#define ENABLE_AVERAGE_PRICE
#endif

#define ENABLE_IN_GAME_LOG_SYSTEM
#define ENABLE_HWID
#define ENABLE_FARM_BLOCK
#define ENABLE_SKILLS_LEVEL_OVER_P										// Sage ve Legendary Skill
#define ENABLE_FAST_SOUL_STONE_SYSTEM									// Fast read soul stone
#define ENABLE_FAST_SKILL_BOOK_SYSTEM									// Fast read book
//#define ENABLE_SKILL_SET_BONUS											// Skill set bonus sage legendary
#define ENABLE_PASSIVE_SKILLS											// Passive Skills
#define ENABLE_SKILL_COLOR_SYSTEM										// Colored Skills
//#define ENABLE_DISTANCE_SKILL_SELECT


#define ENABLE_DUNGEON_INFO												//Zindan takip
#define ENABLE_DUNGEON_P2P
#define ENABLE_DUNGEON_TURN_BACK
#define ENABLE_NEW_PET_SYSTEM											//Gelişmiş Levelli Pet Sistemi
#ifdef ENABLE_NEW_PET_SYSTEM
#define ENABLE_PET_ATTR_DETERMINE										//Pet Efsunlama Ve Tip Belirleme
#define ENABLE_PET_NOT_EXP
#endif
#define ENABLE_ITEM_SELL_LOG
#define ENABLE_BUFFI_SYSTEM
#define ENABLE_NEW_CHAT
#define ENABLE_BOSS_REGEN

#define ENABLE_COSTUME_ATTR
//#define ENABLE_CHALLENGE_SYSTEM
/********************************************** End of General Defines **********************************************/



/********************************************** Item Related Defines Start **********************************************/

#define ENABLE_ACCE_COSTUME_SYSTEM 													// Shoulder Sash Costume
#define ENABLE_MOUNT_COSTUME_SYSTEM 												// Mount Costume
#define ENABLE_PET_COSTUME_SYSTEM 													// Pet Costume
#define ENABLE_WEAPON_COSTUME_SYSTEM 												// Weapon Costume
#define ENABLE_ITEM_ATTR_COSTUME 													// Costume Item Attr
#define ENABLE_PLUS_SCROLL															// Create multiple upgrade scrolls with custom chances
#define ENABLE_DS_GRADE_MYTH														// Myth DS
#define ENABLE_DS_SET_BONUS															// Ds set bonus
#define ENABLE_AURA_SYSTEM															// Aura System

#define ENABLE_MOUNT_SKIN															// Mount Skin

#define ENABLE_PET_SKIN																// Pet Skin

#ifdef ENABLE_PET_SKIN
#define ENABLE_PET_SKIN_ATTR
#define ENABLE_HERSEYE_ATTIR														// Dızcı götten veriyor 23 cm hakan 4k izle hd.mp4
#endif

#define ENABLE_ACCE_COSTUME_SKIN													// Acce Costume Skin
#define ENABLE_AURA_SKIN															// Aura Skin

#define ENABLE_CROWN_SYSTEM															// Crown part
#define ENABLE_ITEMS_SHINING														// Shining Items

#define ENABLE_GREEN_ATTRIBUTE_CHANGER												// Green Attributes

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	// #define USE_ACCE_ABSORB_WITH_NO_NEGATIVE_BONUS
#endif
#define ENABLE_BOOSTER_ITEM
#define ENABLE_PENDANT_ITEM
#define ENABLE_GLOVE_ITEM
#define ENABLE_RINGS
#define ENABLE_DREAMSOUL

#define ENABLE_NAMING_SCROLL														// Naming Scroll for pets and mounts
#define ENABLE_JEWELS_RENEWAL
#define ENABLE_ADDSTONE_NOT_FAIL_ITEM
/********************************************** End of Item Related Defines **********************************************/



/********************************************** Fix Defines Start **********************************************/

#define ENABLE_PARTY_EXP_FIX														// implementation of party exp char_battle
#define ENABLE_FALL_FIX																// Fall fix
#define ENABLE_CRASH_CORE_ARROW_FIX													// Arrow crash fixed
#define ENABLE_COSTUME_RELATED_FIXES												// Costume related tuxedo fix etc
#define ENABLE_KICK_SYNC_FIX														// Cannot find tree/sync fixed
#define ENABLE_EXP_GROUP_FIX														// EXP Group fix
#define ENABLE_HP_GROUP_FIX															// HP Group fix
#define ENABLE_REFRESH_CONTROL
#define ENABLE_LOADING_RENEWAL
/********************************************** End of Fix Defines **********************************************/



/********************************************** Misc Defines Start **********************************************/

#define ENABLE_QUEST_DIE_EVENT														// Quest Die Event ??
#define ENABLE_QUEST_BOOT_EVENT														// Quest Boot Event ??
#define ENABLE_QUEST_DND_EVENT														// Quest Dnd Event ??
// #define ENABLE_SYSLOG_PACKET_SENT												// debug purposes

#define ENABLE_CLEAR_OLD_GUILD_LANDS												// If there is a guild lands, after 3 weeks inactivity it deletes
#define ENABLE_CHAT_FLAG_AND_PM														// Chat PM Flag System
#define ENABLE_NO_RESET_SKILL_WHEN_GM_LEVEL_UP										// Disables reset skilling on giving levels (GM)
#define ENABLE_OBSERVER_DAMAGE_FIX													// observers can not attack
#define ENABLE_BLOW_FIX																// Blow fix
#define ENABLE_EXCHANGE_WINDOW_RENEWAL												// Renewed 24 slot exchange
#define ENABLE_EXTENDED_SHOP_SLOTS													// Renewed 80 slot shop
#define ENABLE_IGNORE_LOW_POWER_BUFF												// Ignore low power buff
#define ENABLE_FIX_ETC_ITEM_FILE_VNUM												// Extended with item vnums
#define ENABLE_NO_CLEAR_BUFF_WHEN_MONSTER_KILL										// No Clear Buff When Monster Kill
#define ENABLE_RANGE_DRAGON_SOUL_UPGRADE											// D.S range upgrade etc
#define ENABLE_OFFICAL_CHARACTER_SCREEN												// Official Charselect
#define ENABLE_EXTENDED_ITEMNAME_ON_GROUND											// See the items on ground
#define ENABLE_AFFECT_BUFF_REMOVE													// Remove affect buff
#define WJ_ENABLE_TRADABLE_ICON														// Tradable icon stuff
#define ENABLE_BUY_WITH_ITEMS														// Buy items with items
#define ENABLE_MULTIPLE_BUY_ITEMS													// Multi Buy
//#define ENABLE_ITEMAWARD_REFRESH													// item award kullan�rsak a��caz
#define ENABLE_DEATHBLOW_ALL_RACES_FIX												// Deathblow damage implemented for lycan aswell.
#define ENABLE_FISHING_RENEWAL														// Official Fishing System
#define ENABLE_HIDE_COSTUME_SYSTEM													// Hide costume parts
#define ENABLE_STOP_CHAT															// Stops the shout receiving when the button is pressed.
#define ENABLE_DISTANCE_NPC															// Distance Shopping from NPC's
#define ENABLE_FAKE_CHAT															// Shout Bot
#define ENABLE_PM_ALL_SEND_SYSTEM													// Send PM to Everyone (gm only)
#define ENABLE_ZODIAC_TEMPLE_CHAT													// Official Zodiac Notice
#define ENABLE_MINING_RENEWAL														// Mining System Reworked
#define ENABLE_ADD_REALTIME_AFFECT													// Realtime Affect
#define ENABLE_DAILY_REWARD															// Reward System

#define ENABLE_CLIENT_VERSION_AND_ONLY_GM											// Only gm mode and client version
#define ENABLE_GET_TOTAL_ONLINE_COUNT												// Shows total online count on all cores
#define ENABLE_PREMIUM_MEMBER_SYSTEM												// Premium Member System
#define ENABLE_AUTO_PICK_UP
#define ENABLE_MAINTENANCE_SYSTEM													// Maintenance System
#define ENABLE_VOTE4BUFF															// Vote 4 Coin & Buff
#define ENABLE_6TH_7TH_ATTR															// 6th 7th attr
#define ENABLE_MESSENGER_TEAM														// Game Team On Messenger
#define ENABLE_LEADERSHIP_EXTENSION													// Leadership
#define ENABLE_EXTENDED_BATTLE_PASS													// Battlepass
#define ENABLE_DUNGEON_MODULES														// New dungeon Modules
#define ENABLE_BLEND_AFFECT															// New Blend System
#define ENABLE_EXTENDED_BLEND_AFFECT												// New Blend Affect System
#define ENABLE_CHAT_COLOR_SYSTEM
#define ENABLE_INCREASED_MOV_SPEED_MOBS
#define ENABLE_METIN_QUEUE


#define ENABLE_P2P_WARP																// P2P Warp
#define ENABLE_TELEPORT_TO_A_FRIEND													// Teleport Friend

#define ENABLE_RELOAD_REGEN															// Reload Regen

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	#define ENABLE_CUBE_SAVE_SOCKET_ATTRS											// Cube save attr / sockets
#endif
#define ENABLE_DROP_ITEM_TO_INVENTORY
#ifdef ENABLE_MULTIPLE_BUY_ITEMS
#define MULTIPLE_BUY_MAX_COUNT 100
#else
#define MULTIPLE_BUY_MAX_COUNT 0
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
#define RESTRICT_COMMAND_GET_INFO					GM_LOW_WIZARD
#define RESTRICT_COMMAND_SET_MISSION				GM_IMPLEMENTOR
#define RESTRICT_COMMAND_PREMIUM_ACTIVATE			GM_IMPLEMENTOR
#endif

#define ENABLE_BASIC_ITEM_SYSTEM
/********************************************** End of Misc Defines **********************************************/

/*
Information: Some fixes were not added with defines to avoid complexity, you can search @lightwork in binary to find them.
*/

/**************VARIABLES**********************/
using CELL_UINT = unsigned short;
using DAM_ULL = unsigned long long;
using DAM_LL = long long;
using DAM_DOUBLE = double;
using HP_ULL = unsigned long long;
using HP_LL = long long;
using QS_USHORT = unsigned short;//SyncQuickslot newpos UINT16_MAX

#define ENABLE_ITEM_PROTO_APPLY_LIMIT
#define ENABLE_ITEM_TABLE_ATTR_LIMIT
#define ENABLE_ATTR_RARE_RENEWAL

#define ENABLE_OFFICIAL_STUFF
