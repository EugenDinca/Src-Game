#ifndef __INC_PACKET_H__
#define __INC_PACKET_H__
#include "stdafx.h"

enum ClientToGamePackets
{
	HEADER_CG_LOGIN 					= 1,
	HEADER_CG_ATTACK 					= 2,
	HEADER_CG_CHAT					    = 3,
	HEADER_CG_CHARACTER_CREATE 			= 4,
	HEADER_CG_CHARACTER_DELETE 			= 5,
	HEADER_CG_CHARACTER_SELECT		 	= 6,
	HEADER_CG_MOVE 						= 7,
	HEADER_CG_SYNC_POSITION 			= 8,
	HEADER_CG_ENTERGAME 				= 9,
	HEADER_CG_ITEM_USE 					= 10,
	HEADER_CG_ITEM_MOVE 				= 11,
	HEADER_CG_ITEM_PICKUP 				= 12,
	HEADER_CG_QUICKSLOT_ADD 			= 13,
	HEADER_CG_QUICKSLOT_DEL 			= 14,
	HEADER_CG_QUICKSLOT_SWAP 			= 15,
	HEADER_CG_WHISPER 					= 16,
#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	HEADER_CG_ITEM_DELETE 				= 17,
	HEADER_CG_ITEM_SELL 				= 18,
#else
	HEADER_CG_ITEM_DROP 				= 17,
	HEADER_CG_ITEM_DROP2 				= 18,
#endif
	HEADER_CG_ON_CLICK 					= 19,
	HEADER_CG_EXCHANGE 					= 20,
	HEADER_CG_CHARACTER_POSITION 		= 21,
	HEADER_CG_SCRIPT_ANSWER 			= 22,
	HEADER_CG_QUEST_INPUT_STRING 		= 23,
	HEADER_CG_QUEST_CONFIRM 			= 24,
	HEADER_CG_SHOP 						= 25,
	HEADER_CG_FLY_TARGETING 			= 26,
	HEADER_CG_USE_SKILL 				= 27,
	HEADER_CG_ADD_FLY_TARGETING 		= 28,
	HEADER_CG_SHOOT 					= 29,
	HEADER_CG_MYSHOP 					= 30,
	HEADER_CG_ITEM_USE_TO_ITEM			= 31,
	HEADER_CG_TARGET 					= 32,
	HEADER_CG_WARP 						= 33,
	HEADER_CG_SCRIPT_BUTTON 			= 34,
	HEADER_CG_MESSENGER 				= 35,
	HEADER_CG_MALL_CHECKOUT 			= 36,
	HEADER_CG_SAFEBOX_CHECKIN 			= 37,
	HEADER_CG_SAFEBOX_CHECKOUT 			= 38,
	HEADER_CG_PARTY_INVITE 				= 39,
	HEADER_CG_PARTY_INVITE_ANSWER 		= 40,
	HEADER_CG_PARTY_REMOVE 				= 41,
	HEADER_CG_PARTY_SET_STATE			= 42,
	HEADER_CG_PARTY_USE_SKILL 			= 43,
	HEADER_CG_SAFEBOX_ITEM_MOVE 		= 44,
	HEADER_CG_PARTY_PARAMETER 			= 45,
	HEADER_CG_GUILD 					= 46,
	HEADER_CG_ANSWER_MAKE_GUILD 		= 47,
	HEADER_CG_FISHING 					= 48,
	HEADER_CG_ITEM_GIVE 				= 49,
	HEADER_CG_EMPIRE 					= 50,
	HEADER_CG_REFINE					= 51,
	HEADER_CG_MARK_LOGIN 				= 52,
	HEADER_CG_MARK_CRCLIST 				= 53,
	HEADER_CG_MARK_UPLOAD 				= 54,
	HEADER_CG_MARK_IDXLIST 				= 55,
	HEADER_CG_HACK 						= 56,
	HEADER_CG_CHANGE_NAME 				= 57,
	HEADER_CG_LOGIN2 					= 58,
	HEADER_CG_DUNGEON 					= 59,
	HEADER_CG_LOGIN3 					= 60,
	HEADER_CG_GUILD_SYMBOL_UPLOAD 		= 61,
	HEADER_CG_SYMBOL_CRC 				= 62,
	HEADER_CG_SCRIPT_SELECT_ITEM 		= 63,
	HEADER_CG_DRAGON_SOUL_REFINE 		= 64,
	HEADER_CG_STATE_CHECKER 			= 65,
#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	HEADER_CG_TARGET_INFO_LOAD 			= 66,
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	HEADER_CG_CUBE_RENEWAL 				= 67,
#endif
#ifdef ENABLE_OFFLINE_SHOP
	HEADER_CG_NEW_OFFLINESHOP 			= 68,
#endif
#ifdef ENABLE_SWITCHBOT
	HEADER_CG_SWITCHBOT 				= 69,
#endif
#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	HEADER_CG_BULK_WHISPER				= 71,
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	HEADER_CG_BIOLOG					= 73,
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	HEADER_CG_IN_GAME_LOG				= 75,
#endif
#ifdef ENABLE_ITEM_TRANSACTIONS
	HEADER_CG_ITEM_TRANSACTIONS			= 76,
#endif
#ifdef ENABLE_DUNGEON_INFO
	HEADER_CG_DUNGEON_INFO				= 77,
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	HEADER_CG_SKILL_COLOR				= 79,
#endif
#ifdef ENABLE_AURA_SYSTEM
	HEADER_CG_AURA						= 80,
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	HEADER_CG_NEW_PET					= 81,
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	HEADER_CG_CHANGE_LANGUAGE			= 82,
#endif
#ifdef ENABLE_EXTENDED_WHISPER_DETAILS
	HEADER_CG_WHISPER_DETAILS			= 83,
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	HEADER_CG_EXT_BATTLE_PASS_ACTION	= 84,
	HEADER_CG_EXT_SEND_BP_PREMIUM		= 85,
#endif
#ifdef ENABLE_LEADERSHIP_EXTENSION
	HEADER_CG_REQUEST_PARTY_BONUS		= 86,
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	HEADER_CG_67_ATTR 					= 87,
	HEADER_CG_CLOSE_67_ATTR 			= 88,
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	HEADER_CG_BUFFI						= 89,
#endif

#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
	HEADER_CG_OKEY_CARD					= 90,
#endif
#ifdef ENABLE_MINI_GAME_BNW
	HEADER_CG_MINIGAME_BNW				= 91,
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	HEADER_CG_MINI_GAME_CATCH_KING		= 92,
#endif
#ifdef ENABLE_VOICE_CHAT
	HEADER_CG_VOICE_CHAT = 207,
#endif
	HEADER_CG_KEY_AGREEMENT				= 252,
	HEADER_CG_TIME_SYNC					= 253,
	HEADER_CG_PONG						= 254,
	HEADER_CG_HANDSHAKE					= 255,
};

enum GameToClientPackets
{
	HEADER_GC_CHARACTER_ADD				= 1,
	HEADER_GC_CHARACTER_DEL				= 2,
	HEADER_GC_MOVE						= 3,
	HEADER_GC_CHAT						= 4,
	HEADER_GC_SYNC_POSITION				= 5,
	HEADER_GC_LOGIN_SUCCESS				= 6,
	HEADER_GC_LOGIN_FAILURE				= 7,
	HEADER_GC_CHARACTER_CREATE_SUCCESS	= 8,
	HEADER_GC_CHARACTER_CREATE_FAILURE	= 9,
	HEADER_GC_CHARACTER_DELETE_SUCCESS	= 10,
	HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID = 11,
	HEADER_GC_STUN						= 12,
	HEADER_GC_DEAD						= 13,
	HEADER_GC_MAIN_CHARACTER_OLD		= 14,
	HEADER_GC_CHARACTER_POINTS			= 15,
	HEADER_GC_CHARACTER_POINT_CHANGE	= 16,
	HEADER_GC_CHANGE_SPEED				= 17,
	HEADER_GC_CHARACTER_UPDATE			= 18,
	HEADER_GC_ITEM_DEL					= 19,
	HEADER_GC_ITEM_SET					= 20,
	HEADER_GC_ITEM_USE					= 21,
	HEADER_GC_ITEM_DROP					= 22,
	HEADER_GC_ITEM_UPDATE				= 23,
	HEADER_GC_ITEM_GROUND_ADD			= 24,
	HEADER_GC_ITEM_GROUND_DEL			= 25,
	HEADER_GC_QUICKSLOT_ADD				= 26,
	HEADER_GC_QUICKSLOT_DEL				= 27,
	HEADER_GC_QUICKSLOT_SWAP			= 28,
	HEADER_GC_ITEM_OWNERSHIP			= 29,
	HEADER_GC_LOGIN_SUCCESS_NEWSLOT		= 30,
	HEADER_GC_WHISPER					= 31,
	HEADER_GC_MOTION					= 32,
	HEADER_GC_SHOP						= 33,
	HEADER_GC_SHOP_SIGN					= 34,
	HEADER_GC_DUEL_START				= 35,
	HEADER_GC_PVP						= 36,
	HEADER_GC_EXCHANGE					= 37,
	HEADER_GC_CHARACTER_POSITION		= 38,
	HEADER_GC_PING						= 39,
	HEADER_GC_SCRIPT					= 40,
	HEADER_GC_QUEST_CONFIRM				= 41,
	HEADER_GC_MOUNT						= 42,
	HEADER_GC_OWNERSHIP					= 43,
	HEADER_GC_TARGET					= 44,
	HEADER_GC_WARP						= 45,
	HEADER_GC_ADD_FLY_TARGETING			= 46,
	HEADER_GC_CREATE_FLY				= 47,
	HEADER_GC_FLY_TARGETING				= 48,
	HEADER_GC_SKILL_LEVEL_OLD			= 49,
	HEADER_GC_MESSENGER					= 50,
	HEADER_GC_GUILD						= 51,
	HEADER_GC_SKILL_LEVEL				= 52,
	HEADER_GC_PARTY_INVITE				= 53,
	HEADER_GC_PARTY_ADD					= 54,
	HEADER_GC_PARTY_UPDATE				= 55,
	HEADER_GC_PARTY_REMOVE				= 56,
	HEADER_GC_QUEST_INFO				= 57,
	HEADER_GC_REQUEST_MAKE_GUILD		= 58,
	HEADER_GC_PARTY_PARAMETER			= 59,
	HEADER_GC_SAFEBOX_SET				= 60,
	HEADER_GC_SAFEBOX_DEL				= 61,
	HEADER_GC_SAFEBOX_WRONG_PASSWORD	= 62,
	HEADER_GC_SAFEBOX_SIZE				= 63,
	HEADER_GC_FISHING					= 64,
	HEADER_GC_EMPIRE					= 65,
	HEADER_GC_PARTY_LINK				= 66,
	HEADER_GC_PARTY_UNLINK				= 67,
	HEADER_GC_REFINE_INFORMATION_OLD	= 68,
	HEADER_GC_VIEW_EQUIP				= 69,
	HEADER_GC_MARK_BLOCK				= 70,
	HEADER_GC_MARK_IDXLIST				= 71,
	HEADER_GC_TIME						= 72,
	HEADER_GC_CHANGE_NAME				= 73,
	HEADER_GC_DUNGEON					= 74,
	HEADER_GC_WALK_MODE					= 75,
	HEADER_GC_SKILL_GROUP				= 76,
	HEADER_GC_MAIN_CHARACTER			= 77,
	HEADER_GC_SEPCIAL_EFFECT			= 78,
	HEADER_GC_NPC_POSITION				= 79,
	HEADER_GC_LOGIN_KEY					= 80,
	HEADER_GC_REFINE_INFORMATION		= 81,
	HEADER_GC_CHANNEL					= 82,
	HEADER_GC_MALL_OPEN					= 83,
	HEADER_GC_TARGET_UPDATE				= 84,
	HEADER_GC_TARGET_DELETE				= 85,
	HEADER_GC_TARGET_CREATE				= 86,
	HEADER_GC_AFFECT_ADD				= 87,
	HEADER_GC_AFFECT_REMOVE				= 88,
	HEADER_GC_MALL_SET					= 89,
	HEADER_GC_MALL_DEL					= 90,
	HEADER_GC_LAND_LIST					= 91,
	HEADER_GC_LOVER_INFO				= 92,
	HEADER_GC_LOVE_POINT_UPDATE			= 93,
	HEADER_GC_SYMBOL_DATA				= 94,
	HEADER_GC_DIG_MOTION				= 95,
	HEADER_GC_DAMAGE_INFO				= 96,
	HEADER_GC_CHAR_ADDITIONAL_INFO		= 97,
	HEADER_GC_AUTH_SUCCESS				= 100,
	HEADER_GC_SPECIFIC_EFFECT			= 101,
	HEADER_GC_DRAGON_SOUL_REFINE		= 102,
	HEADER_GC_RESPOND_CHANNELSTATUS		= 103,
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	HEADER_GC_CUBE_RENEWAL				= 104,
#endif
#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	HEADER_GC_TARGET_INFO				= 105,
#endif
#ifdef ENABLE_DS_SET_BONUS
	HEADER_GC_DS_TABLE					= 106,
#endif
#ifdef ENABLE_OFFLINE_SHOP
	HEADER_GC_NEW_OFFLINESHOP			= 107,
#endif
#ifdef ENABLE_SWITCHBOT
	HEADER_GC_SWITCHBOT					= 108,
#endif
#ifdef ENABLE_REFRESH_CONTROL
	HEADER_GC_REFRESH_CONTROL			= 110,
#endif
#ifdef ENABLE_GLOBAL_RANKING
	HEADER_GC_GLOBAL_RANKING			= 111,
#endif
#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	HEADER_GC_BULK_WHISPER				= 112,
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	HEADER_GC_BIOLOG					= 114,
#endif
#ifdef ENABLE_CHAR_SETTINGS
	HEADER_GC_CHAR_SETTINGS				= 116,
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	HEADER_GC_IN_GAME_LOG				= 117,
#endif
#ifdef ENABLE_DUNGEON_INFO
	HEADER_GC_DUNGEON_INFO				= 118,
#endif
#ifdef ENABLE_EVENT_MANAGER
	HEADER_GC_EVENT_MANAGER				= 119,
#endif
#ifdef ENABLE_ITEMSHOP
	HEADER_GC_ITEMSHOP					= 120,
#endif
#ifdef ENABLE_AURA_SYSTEM
	HEADER_GC_AURA						= 121,
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	HEADER_GC_NEW_PET					= 122,
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	HEADER_GC_REQUEST_CHANGE_LANGUAGE	= 123,
#endif
#ifdef ENABLE_EXTENDED_WHISPER_DETAILS
	HEADER_GC_WHISPER_DETAILS			= 124,
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	HEADER_GC_EXT_BATTLE_PASS_OPEN				= 125,
	HEADER_GC_EXT_BATTLE_PASS_GENERAL_INFO		= 126,
	HEADER_GC_EXT_BATTLE_PASS_MISSION_INFO		= 127,
	HEADER_GC_EXT_BATTLE_PASS_MISSION_UPDATE	= 128,
	HEADER_GC_EXT_BATTLE_PASS_SEND_RANKING		= 129,
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	HEADER_GC_OPEN_67_ATTR 						= 130,
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	HEADER_GC_BUFFI						= 131,
#endif
#ifdef ENABLE_NEW_CHAT
	HEADER_GC_NEW_CHAT					= 132,
#endif
#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
	HEADER_GC_OKEY_CARD					= 133,
#endif
#ifdef ENABLE_MINI_GAME_BNW
	HEADER_GC_MINIGAME_BNW				= 134,
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	HEADER_GC_MINI_GAME_CATCH_KING		= 135,
#endif
	HEADER_GC_SPECIFIC_EFFECT2			= 136,
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
	HEADER_GC_BOSS_DMG_RANKING			= 150,
#endif
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	HEADER_GC_PLAYER_BLOCK 				= 161,
#endif
#ifdef ENABLE_VOICE_CHAT
	HEADER_GC_VOICE_CHAT = 182,
#endif
#if defined(__BL_KILL_BAR__)
	HEADER_GC_KILLBAR 					= 218,
#endif

	HEADER_GC_KEY_AGREEMENT_COMPLETED	= 250,
	HEADER_GC_KEY_AGREEMENT				= 251,
	HEADER_GC_TIME_SYNC					= 252,
	HEADER_GC_PHASE						= 253,
	HEADER_GC_BINDUDP					= 254,
	HEADER_GC_HANDSHAKE					= 255,
};

enum GameToGamePackets
{
	HEADER_GG_LOGIN					= 1,
	HEADER_GG_LOGOUT				= 2,
	HEADER_GG_RELAY					= 3,
	HEADER_GG_NOTICE				= 4,
	HEADER_GG_SHUTDOWN				= 5,
	HEADER_GG_GUILD					= 6,
	HEADER_GG_DISCONNECT			= 7,
	HEADER_GG_SHOUT					= 8,
	HEADER_GG_SETUP					= 9,
	HEADER_GG_MESSENGER_ADD			= 10,
	HEADER_GG_MESSENGER_REMOVE		= 11,
	HEADER_GG_FIND_POSITION			= 12,
	HEADER_GG_WARP_CHARACTER		= 13,
	HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX = 14,
	HEADER_GG_TRANSFER				= 15,
	HEADER_GG_RELOAD_CRC_LIST		= 16,
	HEADER_GG_LOGIN_PING			= 17,
	HEADER_GG_BLOCK_CHAT			= 18,
	HEADER_GG_CHECK_AWAKENESS		= 19,
#ifdef ENABLE_FULL_NOTICE
	HEADER_GG_BIG_NOTICE			= 20,
#endif
#ifdef ENABLE_SWITCHBOT
	HEADER_GG_SWITCHBOT				= 21,
#endif
#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	HEADER_GG_BULK_WHISPER			= 22,
#endif
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	HEADER_GG_TELEPORT_REQUEST		= 23,
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	HEADER_GG_LOCALE_NOTICE			= 24,
#endif
#ifdef ENABLE_MAINTENANCE_SYSTEM
	HEADER_GG_MAINTENANCE			= 25,
#endif
#ifdef ENABLE_DUNGEON_P2P
	HEADER_GG_DUNGEON				= 26,
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	HEADER_GG_DUNGEON_TURN_BACK		= 27,
#endif
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
	HEADER_GG_UPDATE_BOSS_DAMAGE_RANKING,
#endif
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	HEADER_GG_PLAYER_BLOCK = 34,
#endif

};

#pragma pack(1)
typedef struct SPacketGGSetup
{
	BYTE	bHeader;
	WORD	wPort;
	BYTE	bChannel;
} TPacketGGSetup;

typedef struct SPacketGGLogin
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	DWORD	dwPID;
	BYTE	bEmpire;
	long	lMapIndex;
	BYTE	bChannel;
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketGGLogin;

typedef struct SPacketGGLogout
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGLogout;

typedef struct SPacketGGRelay
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lSize;
} TPacketGGRelay;

typedef struct SPacketGGNotice
{
	BYTE	bHeader;
	long	lSize;
} TPacketGGNotice;

typedef struct SPacketGGShutdown
{
	BYTE	bHeader;
} TPacketGGShutdown;

typedef struct SPacketGGGuild
{
	BYTE	bHeader;
	BYTE	bSubHeader;
	DWORD	dwGuild;
} TPacketGGGuild;

enum
{
	GUILD_SUBHEADER_GG_CHAT,
	GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS,
};

typedef struct SPacketGGGuildChat
{
	BYTE	bHeader;
	BYTE	bSubHeader;
	DWORD	dwGuild;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGGuildChat;

typedef struct SPacketGGParty
{
	BYTE	header;
	BYTE	subheader;
	DWORD	pid;
	DWORD	leaderpid;
} TPacketGGParty;

enum
{
	PARTY_SUBHEADER_GG_CREATE,
	PARTY_SUBHEADER_GG_DESTROY,
	PARTY_SUBHEADER_GG_JOIN,
	PARTY_SUBHEADER_GG_QUIT,
};

typedef struct SPacketGGDisconnect
{
	BYTE	bHeader;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGDisconnect;

typedef struct SPacketGGShout
{
	BYTE	bHeader;
	BYTE	bEmpire;
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
#endif
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGShout;

typedef struct SPacketGGMessenger
{
	BYTE        bHeader;
	char        szAccount[CHARACTER_NAME_MAX_LEN + 1];
	char        szCompanion[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGMessenger;

typedef struct SPacketGGFindPosition
{
	BYTE header;
	DWORD dwFromPID;
	DWORD dwTargetPID;
} TPacketGGFindPosition;

typedef struct SPacketGGWarpCharacter
{
	BYTE header;
	DWORD pid;
	long x;
	long y;
#ifdef ENABLE_P2P_WARP
	long real_map_index;
	long map_index;
	long addr;
	WORD port;
#endif
} TPacketGGWarpCharacter;

//  HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX	    = 15,

typedef struct SPacketGGGuildWarMapIndex
{
	BYTE bHeader;
	DWORD dwGuildID1;
	DWORD dwGuildID2;
	long lMapIndex;
} TPacketGGGuildWarMapIndex;

typedef struct SPacketGGTransfer
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lX, lY;
} TPacketGGTransfer;

typedef struct SPacketGGLoginPing
{
	BYTE	bHeader;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGLoginPing;

typedef struct SPacketGGBlockChat
{
	BYTE	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lBlockDuration;
} TPacketGGBlockChat;

typedef struct command_handshake
{
	BYTE	bHeader;
	DWORD	dwHandshake;
	DWORD	dwTime;
	long	lDelta;
} TPacketCGHandshake;

typedef struct command_login
{
	BYTE	header;
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TPacketCGLogin;

typedef struct command_login2
{
	BYTE	header;
	char	login[LOGIN_MAX_LEN + 1];
	DWORD	dwLoginKey;
	DWORD	adwClientKey[4];
} TPacketCGLogin2;

typedef struct command_login3
{
	BYTE	header;
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
	DWORD	adwClientKey[4];
#ifdef ENABLE_HWID
	char hwid[64 + 1];
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketCGLogin3;

typedef struct packet_login_key
{
	BYTE	bHeader;
	DWORD	dwLoginKey;
} TPacketGCLoginKey;

typedef struct command_player_select
{
	BYTE	header;
	BYTE	index;
} TPacketCGPlayerSelect;

typedef struct command_player_delete
{
	BYTE	header;
	BYTE	index;
	char	private_code[8];
} TPacketCGPlayerDelete;

typedef struct command_player_create
{
	BYTE        header;
	BYTE        index;
	char        name[CHARACTER_NAME_MAX_LEN + 1];
	WORD        job;
	BYTE	shape;
	BYTE	Con;
	BYTE	Int;
	BYTE	Str;
	BYTE	Dex;
} TPacketCGPlayerCreate;

typedef struct command_player_create_success
{
	BYTE		header;
	BYTE		bAccountCharacterIndex;
	TSimplePlayer	player;
} TPacketGCPlayerCreateSuccess;

typedef struct command_attack
{
	BYTE	bHeader;
	BYTE	bType;
	DWORD	dwVID;
	BYTE	bCRCMagicCubeProcPiece;
	BYTE	bCRCMagicCubeFilePiece;
} TPacketCGAttack;

enum EMoveFuncType
{
	FUNC_WAIT,
	FUNC_MOVE,
	FUNC_ATTACK,
	FUNC_COMBO,
	FUNC_MOB_SKILL,
	_FUNC_SKILL,
	FUNC_MAX_NUM,
	FUNC_SKILL = 0x80,
};

typedef struct command_move
{
	BYTE	bHeader;
	BYTE	bFunc;
	BYTE	bArg;
	BYTE	bRot;
	long	lX;
	long	lY;
	DWORD	dwTime;
} TPacketCGMove;

typedef struct command_sync_position_element
{
	DWORD	dwVID;
	long	lX;
	long	lY;
} TPacketCGSyncPositionElement;

typedef struct command_sync_position
{
	BYTE	bHeader;
	WORD	wSize;
} TPacketCGSyncPosition;

typedef struct command_chat
{
	BYTE	header;
	WORD	size;
	BYTE	type;
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	bool bEmoticon;
#endif
} TPacketCGChat;

typedef struct command_whisper
{
	BYTE	bHeader;
	WORD	wSize;
	char 	szNameTo[CHARACTER_NAME_MAX_LEN + 1];
} TPacketCGWhisper;

typedef struct command_entergame
{
	BYTE	header;
} TPacketCGEnterGame;

typedef struct command_item_use
{
	BYTE 	header;
	TItemPos 	Cell;
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	MAX_COUNT item_open_count;
#endif
} TPacketCGItemUse;

typedef struct command_item_use_to_item
{
	BYTE	header;
	TItemPos	Cell;
	TItemPos	TargetCell;
} TPacketCGItemUseToItem;

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
typedef struct command_item_delete
{
	BYTE	header;
	TItemPos	Cell;
} TPacketCGItemDelete;

typedef struct command_item_sell
{
	BYTE		header;
	TItemPos	Cell;
} TPacketCGItemSell;
#else
typedef struct command_item_drop
{
	BYTE 	header;
	TItemPos 	Cell;
	DWORD	gold;
} TPacketCGItemDrop;

typedef struct command_item_drop2
{
	BYTE 	header;
	TItemPos 	Cell;
	DWORD	gold;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE	count;
#endif
} TPacketCGItemDrop2;
#endif

typedef struct command_item_move
{
	BYTE 	header;
	TItemPos	Cell;
	TItemPos	CellTo;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE	count;
#endif
} TPacketCGItemMove;

typedef struct command_item_pickup
{
	BYTE 	header;
	DWORD	vid;
} TPacketCGItemPickup;

typedef struct command_quickslot_add
{
	BYTE	header;
	QS_USHORT	pos;
	TQuickslot	slot;
} TPacketCGQuickslotAdd;

typedef struct command_quickslot_del
{
	BYTE	header;
	QS_USHORT	pos;
} TPacketCGQuickslotDel;

typedef struct command_quickslot_swap
{
	BYTE	header;
	QS_USHORT	pos;
	QS_USHORT	change_pos;
} TPacketCGQuickslotSwap;

enum
{
	SHOP_SUBHEADER_CG_END,
	SHOP_SUBHEADER_CG_BUY,
	SHOP_SUBHEADER_CG_SELL,
	SHOP_SUBHEADER_CG_SELL2
#ifdef ENABLE_MULTIPLE_BUY_ITEMS
	, SHOP_SUBHEADER_CG_MULTIBUY
#endif
};

typedef struct command_shop_buy
{
	BYTE	count;
} TPacketCGShopBuy;

typedef struct command_shop_sell
{
	UINT	bySlot;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	byCount;
#else
	BYTE	byCount;
#endif
#ifdef ENABLE_SPECIAL_STORAGE
	BYTE	byType;
#endif
} TPacketCGShopSell;

typedef struct command_shop
{
	BYTE	header;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	subheader;
#else
	BYTE	subheader;
#endif
} TPacketCGShop;

typedef struct command_on_click
{
	BYTE	header;
	DWORD	vid;
} TPacketCGOnClick;

enum
{
	EXCHANGE_SUBHEADER_CG_START,	/* arg1 == vid of target character */
	EXCHANGE_SUBHEADER_CG_ITEM_ADD,	/* arg1 == position of item */
	EXCHANGE_SUBHEADER_CG_ITEM_DEL,	/* arg1 == position of item */
	EXCHANGE_SUBHEADER_CG_ELK_ADD,	/* arg1 == amount of gold */
	EXCHANGE_SUBHEADER_CG_ACCEPT,	/* arg1 == not used */
	EXCHANGE_SUBHEADER_CG_CANCEL,	/* arg1 == not used */
};

typedef struct command_exchange
{
	BYTE	header;
	BYTE	sub_header;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	arg1;
#else
	DWORD	arg1;
#endif
	BYTE	arg2;
	TItemPos	Pos;
#if defined(ENABLE_CHECKINOUT_UPDATE)
	bool	SelectPosAuto;
#endif
} TPacketCGExchange;

typedef struct command_position
{
	BYTE	header;
	BYTE	position;
} TPacketCGPosition;

typedef struct command_script_answer
{
	BYTE	header;
	BYTE	answer;
	//char	file[32 + 1];
	//BYTE	answer[16 + 1];
} TPacketCGScriptAnswer;

typedef struct command_script_button
{
	BYTE        header;
	unsigned int	idx;
} TPacketCGScriptButton;

typedef struct command_quest_input_string
{
	BYTE header;
	char msg[64 + 1];
} TPacketCGQuestInputString;

typedef struct command_quest_confirm
{
	BYTE header;
	BYTE answer;
	DWORD requestPID;
} TPacketCGQuestConfirm;

typedef struct packet_quest_confirm
{
	BYTE header;
	char msg[64 + 1];
	long timeout;
	DWORD requestPID;
} TPacketGCQuestConfirm;

typedef struct packet_handshake
{
	BYTE	bHeader;
	DWORD	dwHandshake;
	DWORD	dwTime;
	long	lDelta;
} TPacketGCHandshake;

enum EPhase
{
	PHASE_CLOSE,
	PHASE_HANDSHAKE,
	PHASE_LOGIN,
	PHASE_SELECT,
	PHASE_LOADING,
	PHASE_GAME,
	PHASE_DEAD,

	PHASE_CLIENT_CONNECTING,
	PHASE_DBCLIENT,
	PHASE_P2P,
	PHASE_AUTH,
};

typedef struct packet_phase
{
	BYTE	header;
	BYTE	phase;
} TPacketGCPhase;

typedef struct packet_bindudp
{
	BYTE	header;
	DWORD	addr;
	WORD	port;
} TPacketGCBindUDP;

enum
{
	LOGIN_FAILURE_ALREADY = 1,
	LOGIN_FAILURE_ID_NOT_EXIST = 2,
	LOGIN_FAILURE_WRONG_PASS = 3,
	LOGIN_FAILURE_FALSE = 4,
	LOGIN_FAILURE_NOT_TESTOR = 5,
	LOGIN_FAILURE_NOT_TEST_TIME = 6,
	LOGIN_FAILURE_FULL = 7
};

typedef struct packet_login_success
{
	BYTE		bHeader;
	TSimplePlayer	players[PLAYER_PER_ACCOUNT];
	DWORD		guild_id[PLAYER_PER_ACCOUNT];
	char		guild_name[PLAYER_PER_ACCOUNT][GUILD_NAME_MAX_LEN + 1];

	DWORD		handle;
	DWORD		random_key;
} TPacketGCLoginSuccess;

typedef struct packet_auth_success
{
	BYTE	bHeader;
	DWORD	dwLoginKey;
	BYTE	bResult;
} TPacketGCAuthSuccess;

typedef struct packet_login_failure
{
	BYTE	header;
	char	szStatus[ACCOUNT_STATUS_MAX_LEN + 1];
} TPacketGCLoginFailure;

typedef struct packet_create_failure
{
	BYTE	header;
	BYTE	bType;
} TPacketGCCreateFailure;

enum
{
	ADD_CHARACTER_STATE_DEAD = (1 << 0),
	ADD_CHARACTER_STATE_SPAWN = (1 << 1),
	ADD_CHARACTER_STATE_GUNGON = (1 << 2),
	ADD_CHARACTER_STATE_KILLER = (1 << 3),
	ADD_CHARACTER_STATE_PARTY = (1 << 4),
};

enum ECharacterEquipmentPart
{
	CHR_EQUIPPART_ARMOR,
	CHR_EQUIPPART_WEAPON,
	CHR_EQUIPPART_HEAD,
	CHR_EQUIPPART_HAIR,
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CHR_EQUIPPART_ACCE,
#endif
#ifdef ENABLE_AURA_SYSTEM
	CHR_EQUIPPART_AURA,
#endif
#ifdef ENABLE_CROWN_SYSTEM
	CHR_EQUIPPART_CROWN,
#endif
	CHR_EQUIPPART_NUM,
};

#ifdef ENABLE_ITEMS_SHINING
enum EShiningParts
{
	CHR_SHINING_WRIST_LEFT,
	CHR_SHINING_WRIST_RIGHT,
	CHR_SHINING_ARMOR,
	CHR_SHINING_NUM,
};
#endif

typedef struct packet_add_char
{
	BYTE	header;
	DWORD	dwVID;

	float	angle;
	long	x;
	long	y;
	long	z;

	BYTE	bType;
	WORD	wRaceNum;
	BYTE	bMovingSpeed;
	BYTE	bAttackSpeed;

	BYTE	bStateFlag;
	DWORD	dwAffectFlag[2];
} TPacketGCCharacterAdd;

typedef struct packet_char_additional_info
{
	BYTE    header;
	DWORD   dwVID;
	char    name[CHARACTER_NAME_MAX_LEN + 1];
	DWORD    awPart[CHR_EQUIPPART_NUM];
#ifdef ENABLE_ITEMS_SHINING
	DWORD	adwShining[CHR_SHINING_NUM];
#endif
	BYTE	bEmpire;
	DWORD   dwGuildID;
	DWORD   dwLevel;
#ifdef ENABLE_ALIGNMENT_SYSTEM
	int		sAlignment;
#else
	short	sAlignment;
#endif
	BYTE	bPKMode;
	DWORD	dwMountVnum;
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	DWORD dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketGCCharacterAdditionalInfo;

typedef struct packet_update_char
{
	BYTE	header;
	DWORD	dwVID;

	DWORD	awPart[CHR_EQUIPPART_NUM];
#ifdef ENABLE_ITEMS_SHINING
	DWORD	adwShining[CHR_SHINING_NUM];
#endif
	BYTE	bMovingSpeed;
	BYTE	bAttackSpeed;

	BYTE	bStateFlag;
	DWORD	dwAffectFlag[2];

	DWORD	dwGuildID;
#ifdef ENABLE_ALIGNMENT_SYSTEM
	int		sAlignment;
#else
	short	sAlignment;
#endif
	BYTE	bPKMode;
	DWORD	dwMountVnum;
#if defined(ENABLE_NEW_PET_SYSTEM)
	DWORD	dwLevel;
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	DWORD dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketGCCharacterUpdate;

typedef struct packet_del_char
{
	BYTE	header;
	DWORD	id;
} TPacketGCCharacterDelete;

typedef struct packet_chat
{
	BYTE	header;
	WORD	size;
	BYTE	type;
	DWORD	id;
	BYTE	bEmpire;
} TPacketGCChat;

typedef struct packet_whisper
{
	BYTE	bHeader;
	WORD	wSize;
	BYTE	bType;
	char	szNameFrom[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCWhisper;

typedef struct packet_main_character
{
	BYTE        header;
	DWORD	dwVID;
	WORD	wRaceNum;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	long	lx, ly, lz;
	BYTE	empire;
	BYTE	skill_group;
} TPacketGCMainCharacter;

typedef struct packet_points
{
	BYTE	header;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	points[POINT_MAX_NUM];
#else
	INT		points[POINT_MAX_NUM];
#endif
} TPacketGCPoints;

typedef struct packet_skill_level
{
	BYTE		bHeader;
	TPlayerSkill	skills[SKILL_MAX_NUM];
} TPacketGCSkillLevel;

typedef struct packet_point_change
{
	int		header;
	DWORD	dwVID;
	BYTE	type;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	amount;
	int64_t	value;
#else
	long	amount;
	long	value;
#endif
} TPacketGCPointChange;

typedef struct packet_stun
{
	BYTE	header;
	DWORD	vid;
} TPacketGCStun;

typedef struct packet_dead
{
	BYTE	header;
	DWORD	vid;
} TPacketGCDead;

#if defined(GUILD_RANK_SYSTEM)
enum GUILD_RANK
{
	SUBHEADER_GUILD_RANKING_SEND,
	SUBHEADER_GUILD_RANKING_OPEN,
	GUILD_RANKING_MAX_NUM = 100,
	HEADER_GC_GUILD_RANKING = 161,
};
typedef struct packet_guild_ranking_send
{
	packet_guild_ranking_send()
	{
		strncpy(szOwnerName, "", sizeof(szOwnerName));
		strncpy(szGuildName, "", sizeof(szGuildName));
	}
	packet_guild_ranking_send(int32_t c_id, const char* c_guild, const char* c_name, int32_t c_level, int32_t c_point, int32_t c_win, int32_t c_draw, int32_t c_loss)
		: subheader(GUILD_RANK::SUBHEADER_GUILD_RANKING_SEND), id(c_id), level(c_level), point(c_point), win(c_win), draw(c_draw), loss(c_loss)
	{
		strncpy(szOwnerName, c_name, sizeof(szOwnerName));
		strncpy(szGuildName, c_guild, sizeof(szGuildName));
	}
	uint8_t	header = HEADER_GC_GUILD_RANKING;
	uint8_t	subheader = SUBHEADER_GUILD_RANKING_OPEN;
	int32_t		id = 0;
	char	szGuildName[GUILD_NAME_MAX_LEN + 1];
	char	szOwnerName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t		level = 0;
	int32_t		point = 0;
	int32_t		win = 0;
	int32_t		draw = 0;
	int32_t		loss = 0;
} TPacketGCGuildRankingSend;
#endif

struct TPacketGCItemDelDeprecated
{
	BYTE	header;
	TItemPos Cell;
	DWORD	vnum;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE	count;
#endif
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};

typedef struct packet_item_set
{
	BYTE	header;
	TItemPos Cell;
	DWORD	vnum;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE	count;
#endif
	DWORD	flags;
	DWORD	anti_flags;
	bool	highlight;
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
} TPacketGCItemSet;

typedef struct packet_item_del
{
	BYTE	header;
	BYTE	pos;
} TPacketGCItemDel;

struct packet_item_use
{
	BYTE	header;
	TItemPos Cell;
	DWORD	ch_vid;
	DWORD	victim_vid;
	DWORD	vnum;
};

struct packet_item_move
{
	BYTE	header;
	TItemPos Cell;
	TItemPos CellTo;
};

typedef struct packet_item_update
{
	BYTE	header;
	TItemPos Cell;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE	count;
#endif
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
} TPacketGCItemUpdate;

typedef struct packet_item_ground_add
{
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	packet_item_ground_add()
	{
		memset(&alSockets, 0, sizeof(alSockets));
		memset(&aAttrs, 0, sizeof(aAttrs));
	}
#endif
	BYTE	bHeader;
	long 	x, y, z;
	DWORD	dwVID;
	DWORD	dwVnum;
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttrs[ITEM_ATTRIBUTE_MAX_NUM];
#endif
} TPacketGCItemGroundAdd;

typedef struct packet_item_ownership
{
	BYTE	bHeader;
	DWORD	dwVID;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCItemOwnership;

typedef struct packet_item_ground_del
{
	BYTE	bHeader;
	DWORD	dwVID;
} TPacketGCItemGroundDel;

struct packet_quickslot_add
{
	BYTE	header;
	QS_USHORT	pos;
	TQuickslot	slot;
};

struct packet_quickslot_del
{
	BYTE	header;
	QS_USHORT	pos;
};

struct packet_quickslot_swap
{
	BYTE	header;
	QS_USHORT	pos;
	QS_USHORT	pos_to;
};

struct packet_motion
{
	BYTE	header;
	DWORD	vid;
	DWORD	victim_vid;
	WORD	motion;
};

enum EPacketShopSubHeaders
{
	SHOP_SUBHEADER_GC_START,
	SHOP_SUBHEADER_GC_END,
	SHOP_SUBHEADER_GC_UPDATE_ITEM,
	SHOP_SUBHEADER_GC_UPDATE_PRICE,
	SHOP_SUBHEADER_GC_OK,
	SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY,
#ifdef ENABLE_BUY_WITH_ITEMS
	SHOP_SUBHEADER_GC_NOT_ENOUGH_ITEM,
#endif
	SHOP_SUBHEADER_GC_SOLDOUT,
	SHOP_SUBHEADER_GC_INVENTORY_FULL,
	SHOP_SUBHEADER_GC_INVALID_POS,
	SHOP_SUBHEADER_GC_SOLD_OUT,
	SHOP_SUBHEADER_GC_START_EX,
	SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX,
};

struct packet_shop_item
{
	DWORD		vnum;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t		price;
#else
	DWORD		price;
#endif
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE		count;
#endif
#ifdef ENABLE_BUY_WITH_ITEMS
	TShopItemPrice	itemPrice[MAX_SHOP_PRICES];
#endif
	BYTE		display_pos;
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};

typedef struct packet_shop_start
{
	DWORD   owner_vid;
	struct packet_shop_item	items[SHOP_HOST_ITEM_MAX_NUM];
} TPacketGCShopStart;

typedef struct packet_shop_start_ex
{
	typedef struct sub_packet_shop_tab
	{
		char name[SHOP_TAB_NAME_MAX];
		BYTE coin_type;
		packet_shop_item items[SHOP_HOST_ITEM_MAX_NUM];
	} TSubPacketShopTab;
	DWORD owner_vid;
	BYTE shop_tab_count;
} TPacketGCShopStartEx;

typedef struct packet_shop_update_item
{
	BYTE			pos;
	struct packet_shop_item	item;
} TPacketGCShopUpdateItem;

typedef struct packet_shop_update_price
{
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t			iPrice;
#else
	int				iPrice;
#endif
} TPacketGCShopUpdatePrice;

typedef struct packet_shop
{
	BYTE        header;
	WORD	size;
	BYTE        subheader;
} TPacketGCShop;

struct packet_exchange
{
	BYTE	header;
	BYTE	sub_header;
	BYTE	is_me;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	arg1;
#else
	DWORD	arg1;	// vnum
#endif
	TItemPos	arg2;	// cell
	DWORD	arg3;	// count
#ifdef WJ_ENABLE_TRADABLE_ICON
	TItemPos	arg4;
#endif
	long	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};

enum EPacketTradeSubHeaders
{
	EXCHANGE_SUBHEADER_GC_START,	/* arg1 == vid */
	EXCHANGE_SUBHEADER_GC_ITEM_ADD,	/* arg1 == vnum  arg2 == pos  arg3 == count */
	EXCHANGE_SUBHEADER_GC_ITEM_DEL,
	EXCHANGE_SUBHEADER_GC_GOLD_ADD,	/* arg1 == gold */
	EXCHANGE_SUBHEADER_GC_ACCEPT,	/* arg1 == accept */
	EXCHANGE_SUBHEADER_GC_END,		/* arg1 == not used */
	EXCHANGE_SUBHEADER_GC_ALREADY,	/* arg1 == not used */
	EXCHANGE_SUBHEADER_GC_LESS_GOLD,	/* arg1 == not used */
};

struct packet_position
{
	BYTE	header;
	DWORD	vid;
	BYTE	position;
};

typedef struct packet_ping
{
	BYTE	header;
} TPacketGCPing;

struct packet_script
{
	BYTE	header;
	WORD	size;
	BYTE	skin;
	WORD	src_size;
};

typedef struct packet_change_speed
{
	BYTE		header;
	DWORD		vid;
	WORD		moving_speed;
} TPacketGCChangeSpeed;

struct packet_mount
{
	BYTE	header;
	DWORD	vid;
	DWORD	mount_vid;
	BYTE	pos;
	DWORD	x, y;
};

typedef struct packet_move
{
	BYTE		bHeader;
	BYTE		bFunc;
	BYTE		bArg;
	BYTE		bRot;
	DWORD		dwVID;
	long		lX;
	long		lY;
	DWORD		dwTime;
	DWORD		dwDuration;
} TPacketGCMove;

typedef struct packet_ownership
{
	BYTE		bHeader;
	DWORD		dwOwnerVID;
	DWORD		dwVictimVID;
} TPacketGCOwnership;

typedef struct packet_sync_position_element
{
	DWORD	dwVID;
	long	lX;
	long	lY;
} TPacketGCSyncPositionElement;

typedef struct packet_sync_position
{
	BYTE	bHeader;
	WORD	wSize;
} TPacketGCSyncPosition;

typedef struct packet_fly
{
	BYTE	bHeader;
	BYTE	bType;
	DWORD	dwStartVID;
	DWORD	dwEndVID;
} TPacketGCCreateFly;

typedef struct command_fly_targeting
{
	BYTE		bHeader;
	DWORD		dwTargetVID;
	long		x, y;
} TPacketCGFlyTargeting;

typedef struct packet_fly_targeting
{
	BYTE		bHeader;
	DWORD		dwShooterVID;
	DWORD		dwTargetVID;
	long		x, y;
} TPacketGCFlyTargeting;

typedef struct packet_shoot
{
	BYTE		bHeader;
	BYTE		bType;
} TPacketCGShoot;

typedef struct packet_duel_start
{
	BYTE	header;
	WORD	wSize;
} TPacketGCDuelStart;

enum EPVPModes
{
	PVP_MODE_NONE,
	PVP_MODE_AGREE,
	PVP_MODE_FIGHT,
	PVP_MODE_REVENGE
};

typedef struct packet_pvp
{
	BYTE        bHeader;
	DWORD       dwVIDSrc;
	DWORD       dwVIDDst;
	BYTE        bMode;
} TPacketGCPVP;

typedef struct command_use_skill
{
	BYTE	bHeader;
	DWORD	dwVnum;
	DWORD	dwVID;
} TPacketCGUseSkill;

typedef struct command_target
{
	BYTE	header;
	DWORD	dwVID;
} TPacketCGTarget;

typedef struct packet_target
{
	BYTE	header;
	DWORD	dwVID;
	BYTE	bHPPercent;
#ifdef ENABLE_SHOW_TARGET_HP
	int64_t		hpTarget;
#endif
#if defined(ENABLE_TARGET_ELEMENT_SYSTEM)
	BYTE bElement;
#endif
} TPacketGCTarget;

typedef struct packet_warp
{
	BYTE	bHeader;
	long	lX;
	long	lY;
	long	lAddr;
	WORD	wPort;
} TPacketGCWarp;

typedef struct command_warp
{
	BYTE	bHeader;
} TPacketCGWarp;

struct packet_quest_info
{
	BYTE header;
	WORD size;
	WORD index;
#ifdef ENABLE_QUEST_CATEGORY
	WORD c_index;
#endif
	BYTE flag;
};

enum
{
	MESSENGER_SUBHEADER_GC_LIST,
	MESSENGER_SUBHEADER_GC_LOGIN,
	MESSENGER_SUBHEADER_GC_LOGOUT,
	MESSENGER_SUBHEADER_GC_INVITE,
#ifdef ENABLE_MESSENGER_TEAM
	MESSENGER_SUBHEADER_GC_TEAM_LIST,
	MESSENGER_SUBHEADER_GC_TEAM_LOGIN,
	MESSENGER_SUBHEADER_GC_TEAM_LOGOUT,
#endif
};

typedef struct packet_messenger
{
	BYTE header;
	WORD size;
	BYTE subheader;
} TPacketGCMessenger;

typedef struct packet_messenger_guild_list
{
	BYTE connected;
	BYTE length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildList;

typedef struct packet_messenger_guild_login
{
	BYTE length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildLogin;

typedef struct packet_messenger_guild_logout
{
	BYTE length;

	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildLogout;

typedef struct packet_messenger_list_offline
{
	BYTE connected; // always 0
	BYTE length;
} TPacketGCMessengerListOffline;

typedef struct packet_messenger_list_online
{
	BYTE connected; // always 1
	BYTE length;
} TPacketGCMessengerListOnline;

#ifdef ENABLE_MESSENGER_TEAM
typedef struct packet_messenger_team_list_offline
{
	BYTE connected;
	BYTE length;
} TPacketGCMessengerTeamListOffline;

typedef struct packet_messenger_team_list_online
{
	BYTE connected;
	BYTE length;
} TPacketGCMessengerTeamListOnline;
#endif

enum
{
	MESSENGER_SUBHEADER_CG_ADD_BY_VID,
	MESSENGER_SUBHEADER_CG_ADD_BY_NAME,
	MESSENGER_SUBHEADER_CG_REMOVE,
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	MESSENGER_SUBHEADER_CG_REQUEST_WARP_BY_NAME,
	MESSENGER_SUBHEADER_CG_SUMMON_BY_NAME,	
#endif
};

typedef struct command_messenger
{
	BYTE header;
	BYTE subheader;
} TPacketCGMessenger;

typedef struct command_messenger_add_by_vid
{
	DWORD vid;
} TPacketCGMessengerAddByVID;

typedef struct command_messenger_add_by_name
{
	BYTE length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketCGMessengerAddByName;

typedef struct command_messenger_remove
{
	char login[LOGIN_MAX_LEN + 1];
	//DWORD account;
} TPacketCGMessengerRemove;

typedef struct command_safebox_checkout
{
	BYTE	bHeader;
	BYTE	bSafePos;
	TItemPos	ItemPos;
} TPacketCGSafeboxCheckout;

typedef struct command_safebox_checkin
{
	BYTE	bHeader;
	BYTE	bSafePos;
	TItemPos	ItemPos;
} TPacketCGSafeboxCheckin;

///////////////////////////////////////////////////////////////////////////////////
// Party

typedef struct command_party_parameter
{
	BYTE	bHeader;
	BYTE	bDistributeMode;
} TPacketCGPartyParameter;

typedef struct paryt_parameter
{
	BYTE	bHeader;
	BYTE	bDistributeMode;
} TPacketGCPartyParameter;

typedef struct packet_party_add
{
	BYTE	header;
	DWORD	pid;
	char	name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCPartyAdd;

typedef struct command_party_invite
{
	BYTE	header;
	DWORD	vid;
} TPacketCGPartyInvite;

typedef struct packet_party_invite
{
	BYTE	header;
	DWORD	leader_vid;
} TPacketGCPartyInvite;

typedef struct command_party_invite_answer
{
	BYTE	header;
	DWORD	leader_vid;
	BYTE	accept;
} TPacketCGPartyInviteAnswer;

typedef struct packet_party_update
{
	BYTE	header;
	DWORD	pid;
	BYTE	role;
	BYTE	percent_hp;
	short	affects[7];
} TPacketGCPartyUpdate;

typedef struct packet_party_remove
{
	BYTE header;
	DWORD pid;
} TPacketGCPartyRemove;

typedef struct packet_party_link
{
	BYTE header;
	DWORD pid;
	DWORD vid;
} TPacketGCPartyLink;

typedef struct packet_party_unlink
{
	BYTE header;
	DWORD pid;
	DWORD vid;
} TPacketGCPartyUnlink;

typedef struct command_party_remove
{
	BYTE header;
	DWORD pid;
} TPacketCGPartyRemove;

typedef struct command_party_set_state
{
	BYTE header;
	DWORD pid;
	BYTE byRole;
	BYTE flag;
} TPacketCGPartySetState;

enum
{
	PARTY_SKILL_HEAL = 1,
	PARTY_SKILL_WARP = 2
};

typedef struct command_party_use_skill
{
	BYTE header;
	BYTE bySkillIndex;
	DWORD vid;
} TPacketCGPartyUseSkill;

typedef struct packet_safebox_size
{
	BYTE bHeader;
	BYTE bSize;
} TPacketCGSafeboxSize;

typedef struct packet_safebox_wrong_password
{
	BYTE	bHeader;
} TPacketCGSafeboxWrongPassword;

typedef struct command_empire
{
	BYTE	bHeader;
	BYTE	bEmpire;
} TPacketCGEmpire;

typedef struct packet_empire
{
	BYTE	bHeader;
	BYTE	bEmpire;
} TPacketGCEmpire;

enum
{
	SAFEBOX_MONEY_STATE_SAVE,
	SAFEBOX_MONEY_STATE_WITHDRAW,
};

typedef struct command_safebox_money
{
	BYTE        bHeader;
	BYTE        bState;
	long	lMoney;
} TPacketCGSafeboxMoney;

typedef struct packet_safebox_money_change
{
	BYTE	bHeader;
	long	lMoney;
} TPacketGCSafeboxMoneyChange;

// Guild

enum
{
	GUILD_SUBHEADER_GC_LOGIN,
	GUILD_SUBHEADER_GC_LOGOUT,
	GUILD_SUBHEADER_GC_LIST,
	GUILD_SUBHEADER_GC_GRADE,
	GUILD_SUBHEADER_GC_ADD,
	GUILD_SUBHEADER_GC_REMOVE,
	GUILD_SUBHEADER_GC_GRADE_NAME,
	GUILD_SUBHEADER_GC_GRADE_AUTH,
	GUILD_SUBHEADER_GC_INFO,
	GUILD_SUBHEADER_GC_COMMENTS,
	GUILD_SUBHEADER_GC_CHANGE_EXP,
	GUILD_SUBHEADER_GC_CHANGE_MEMBER_GRADE,
	GUILD_SUBHEADER_GC_SKILL_INFO,
	GUILD_SUBHEADER_GC_CHANGE_MEMBER_GENERAL,
	GUILD_SUBHEADER_GC_GUILD_INVITE,
	GUILD_SUBHEADER_GC_WAR,
	GUILD_SUBHEADER_GC_GUILD_NAME,
	GUILD_SUBHEADER_GC_GUILD_WAR_LIST,
	GUILD_SUBHEADER_GC_GUILD_WAR_END_LIST,
	GUILD_SUBHEADER_GC_WAR_SCORE,
	GUILD_SUBHEADER_GC_MONEY_CHANGE,
};

enum GUILD_SUBHEADER_CG
{
	GUILD_SUBHEADER_CG_ADD_MEMBER,
	GUILD_SUBHEADER_CG_REMOVE_MEMBER,
	GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME,
	GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY,
	GUILD_SUBHEADER_CG_OFFER,
	GUILD_SUBHEADER_CG_POST_COMMENT,
	GUILD_SUBHEADER_CG_DELETE_COMMENT,
	GUILD_SUBHEADER_CG_REFRESH_COMMENT,
	GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE,
	GUILD_SUBHEADER_CG_USE_SKILL,
	GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL,
	GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER,
	GUILD_SUBHEADER_CG_CHARGE_GSP,
	GUILD_SUBHEADER_CG_DEPOSIT_MONEY,
	GUILD_SUBHEADER_CG_WITHDRAW_MONEY,
};

typedef struct packet_guild
{
	BYTE header;
	WORD size;
	BYTE subheader;
} TPacketGCGuild;

typedef struct packet_guild_name_t
{
	BYTE header;
	WORD size;
	BYTE subheader;
	DWORD	guildID;
	char	guildName[GUILD_NAME_MAX_LEN];
} TPacketGCGuildName;

typedef struct packet_guild_war
{
	DWORD	dwGuildSelf;
	DWORD	dwGuildOpp;
	BYTE	bType;
	BYTE 	bWarState;
} TPacketGCGuildWar;

typedef struct command_guild
{
	BYTE header;
	BYTE subheader;
} TPacketCGGuild;

typedef struct command_guild_answer_make_guild
{
	BYTE header;
	char guild_name[GUILD_NAME_MAX_LEN + 1];
} TPacketCGAnswerMakeGuild;

typedef struct command_guild_use_skill
{
	DWORD	dwVnum;
	DWORD	dwPID;
} TPacketCGGuildUseSkill;

// Guild Mark
typedef struct command_mark_login
{
	BYTE    header;
	DWORD   handle;
	DWORD   random_key;
} TPacketCGMarkLogin;

typedef struct command_mark_upload
{
	BYTE	header;
	DWORD	gid;
	BYTE	image[16 * 12 * 4];
} TPacketCGMarkUpload;

typedef struct command_mark_idxlist
{
	BYTE	header;
} TPacketCGMarkIDXList;

typedef struct command_mark_crclist
{
	BYTE	header;
	BYTE	imgIdx;
	DWORD	crclist[80];
} TPacketCGMarkCRCList;

typedef struct packet_mark_idxlist
{
	BYTE    header;
	DWORD	bufSize;
	WORD	count;
} TPacketGCMarkIDXList;

typedef struct packet_mark_block
{
	BYTE	header;
	DWORD	bufSize;
	BYTE	imgIdx;
	DWORD	count;
} TPacketGCMarkBlock;

typedef struct command_symbol_upload
{
	BYTE	header;
	WORD	size;
	DWORD	guild_id;
} TPacketCGGuildSymbolUpload;

typedef struct command_symbol_crc
{
	BYTE header;
	DWORD guild_id;
	DWORD crc;
	DWORD size;
} TPacketCGSymbolCRC;

typedef struct packet_symbol_data
{
	BYTE header;
	WORD size;
	DWORD guild_id;
} TPacketGCGuildSymbolData;

// Fishing

typedef struct command_fishing
{
	BYTE header;
	BYTE dir;
} TPacketCGFishing;

typedef struct packet_fishing
{
	BYTE header;
	BYTE subheader;
	DWORD info;
	BYTE dir;
} TPacketGCFishing;

enum
{
	FISHING_SUBHEADER_GC_START,
	FISHING_SUBHEADER_GC_STOP,
	FISHING_SUBHEADER_GC_REACT,
	FISHING_SUBHEADER_GC_SUCCESS,
	FISHING_SUBHEADER_GC_FAIL,
	FISHING_SUBHEADER_GC_FISH,
};

typedef struct command_give_item
{
	BYTE byHeader;
	DWORD dwTargetVID;
	TItemPos ItemPos;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT byItemCount;
#else
	BYTE byItemCount;
#endif
} TPacketCGGiveItem;

typedef struct SPacketCGHack
{
	BYTE	bHeader;
	char	szBuf[255 + 1];
} TPacketCGHack;

// SubHeader - Dungeon
enum
{
	DUNGEON_SUBHEADER_GC_TIME_ATTACK_START = 0,
	DUNGEON_SUBHEADER_GC_DESTINATION_POSITION = 1,
};

typedef struct packet_dungeon
{
	BYTE bHeader;
	WORD size;
	BYTE subheader;
} TPacketGCDungeon;

typedef struct packet_dungeon_dest_position
{
	long x;
	long y;
} TPacketGCDungeonDestPosition;

typedef struct SPacketGCShopSign
{
	BYTE	bHeader;
	DWORD	dwVID;
	char	szSign[SHOP_SIGN_MAX_LEN + 1];
} TPacketGCShopSign;

typedef struct SPacketCGMyShop
{
	BYTE	bHeader;
	char	szSign[SHOP_SIGN_MAX_LEN + 1];
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	bCount;
#else
	BYTE	bCount;
#endif
} TPacketCGMyShop;

typedef struct SPacketGCTime
{
	BYTE	bHeader;
	time_t	time;
} TPacketGCTime;

enum
{
	WALKMODE_RUN,
	WALKMODE_WALK,
};

typedef struct SPacketGCWalkMode
{
	BYTE	header;
	DWORD	vid;
	BYTE	mode;
} TPacketGCWalkMode;

typedef struct SPacketGCChangeSkillGroup
{
	BYTE        header;
	BYTE        skill_group;
} TPacketGCChangeSkillGroup;

typedef struct SPacketCGRefine
{
	BYTE	header;
	BYTE	pos;
	BYTE	type;
} TPacketCGRefine;

typedef struct SPacketCGRequestRefineInfo
{
	BYTE	header;
	BYTE	pos;
} TPacketCGRequestRefineInfo;

typedef struct SPacketGCRefineInformaion
{
	BYTE	header;
	BYTE	type;
	BYTE	pos;
	DWORD	src_vnum;
	DWORD	result_vnum;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	material_count;
#else
	BYTE	material_count;
#endif
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	cost;
#else
	int		cost;
#endif
	int		prob;
	TRefineMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TPacketGCRefineInformation;

struct TNPCPosition
{
	BYTE bType;
	char name[CHARACTER_NAME_MAX_LEN + 1];
	long x;
	long y;
};

typedef struct SPacketGCNPCPosition
{
	BYTE header;
	WORD size;
	WORD count;

	// array of TNPCPosition
} TPacketGCNPCPosition;

typedef struct SPacketGCSpecialEffect
{
	BYTE header;
	BYTE type;
	DWORD vid;
} TPacketGCSpecialEffect;

typedef struct SPacketCGChangeName
{
	BYTE header;
	BYTE index;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketCGChangeName;

typedef struct SPacketGCChangeName
{
	BYTE header;
	DWORD pid;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCChangeName;

typedef struct packet_channel
{
	BYTE header;
	BYTE channel;
} TPacketGCChannel;

#if defined(__BL_KILL_BAR__)
typedef struct command_kill_bar
{
	BYTE	bHeader;
	BYTE	bKillerRace;
	BYTE	bKillerWeaponType;
	BYTE	bVictimRace;
	char	szKiller[CHARACTER_NAME_MAX_LEN + 1];
	char	szVictim[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCKillBar;
#endif

typedef struct SEquipmentItemSet
{
	DWORD   vnum;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE    count;
#endif
	long    alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
} TEquipmentItemSet;

typedef struct pakcet_view_equip
{
	BYTE  header;
	DWORD vid;
	TEquipmentItemSet equips[WEAR_MAX_NUM];
} TPacketViewEquip;

typedef struct
{
	DWORD	dwID;
	long	x, y;
	long	width, height;
	DWORD	dwGuildID;
} TLandPacketElement;

typedef struct packet_land_list
{
	BYTE	header;
	WORD	size;
} TPacketGCLandList;

typedef struct
{
	BYTE	bHeader;
	long	lID;
	char	szName[32 + 1];
	DWORD	dwVID;
	BYTE	bType;
} TPacketGCTargetCreate;

typedef struct
{
	BYTE	bHeader;
	long	lID;
	long	lX, lY;
} TPacketGCTargetUpdate;

typedef struct
{
	BYTE	bHeader;
	long	lID;
} TPacketGCTargetDelete;

typedef struct
{
	BYTE		bHeader;
	TPacketAffectElement elem;
} TPacketGCAffectAdd;

typedef struct
{
	BYTE	bHeader;
	DWORD	dwType;
	BYTE	bApplyOn;
} TPacketGCAffectRemove;

typedef struct packet_lover_info
{
	BYTE header;
	char name[CHARACTER_NAME_MAX_LEN + 1];
	BYTE love_point;
} TPacketGCLoverInfo;

typedef struct packet_love_point_update
{
	BYTE header;
	BYTE love_point;
} TPacketGCLovePointUpdate;

// MINING
typedef struct packet_dig_motion
{
	BYTE header;
	DWORD vid;
	DWORD target_vid;
	BYTE count;
} TPacketGCDigMotion;
// END_OF_MINING

// SCRIPT_SELECT_ITEM
typedef struct command_script_select_item
{
	BYTE header;
	DWORD selection;
} TPacketCGScriptSelectItem;
// END_OF_SCRIPT_SELECT_ITEM

typedef struct packet_damage_info
{
	BYTE header;
	DWORD dwVID;
	BYTE flag;
	DAM_LL damage;
} TPacketGCDamageInfo;

typedef struct SPacketGGCheckAwakeness
{
	BYTE bHeader;
} TPacketGGCheckAwakeness;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
struct TPacketKeyAgreement
{
	static const int MAX_DATA_LEN = 256;
	BYTE bHeader;
	WORD wAgreedLength;
	WORD wDataLength;
	BYTE data[MAX_DATA_LEN];
};

struct TPacketKeyAgreementCompleted
{
	BYTE bHeader;
	BYTE data[3]; // dummy (not used)
};

#endif // _IMPROVED_PACKET_ENCRYPTION_

#define MAX_EFFECT_FILE_NAME 128
typedef struct SPacketGCSpecificEffect
{
	BYTE header;
	DWORD vid;
	char effect_file[MAX_EFFECT_FILE_NAME];
} TPacketGCSpecificEffect;

typedef struct SPacketGCSpecificEffect2
{
	BYTE header;
	DWORD vid;
	BYTE type;
} TPacketGCSpecificEffect2;

enum EDragonSoulRefineWindowRefineType
{
	DragonSoulRefineWindow_UPGRADE,
	DragonSoulRefineWindow_IMPROVEMENT,
	DragonSoulRefineWindow_REFINE,
};

enum EPacketCGDragonSoulSubHeaderType
{
	DS_SUB_HEADER_OPEN,
	DS_SUB_HEADER_CLOSE,
	DS_SUB_HEADER_DO_REFINE_GRADE,
	DS_SUB_HEADER_DO_REFINE_STEP,
	DS_SUB_HEADER_DO_REFINE_STRENGTH,
	DS_SUB_HEADER_REFINE_FAIL,
	DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE,
	DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL,
	DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY,
	DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL,
	DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL,
	DS_SUB_HEADER_REFINE_SUCCEED,
};
typedef struct SPacketCGDragonSoulRefine
{
	SPacketCGDragonSoulRefine() : header(HEADER_CG_DRAGON_SOUL_REFINE)
	{}
	BYTE header;
	BYTE bSubType;
	TItemPos ItemGrid[DRAGON_SOUL_REFINE_GRID_SIZE];
} TPacketCGDragonSoulRefine;

typedef struct SPacketGCDragonSoulRefine
{
	SPacketGCDragonSoulRefine() : header(HEADER_GC_DRAGON_SOUL_REFINE)
	{}
	BYTE header;
	BYTE bSubType;
	TItemPos Pos;
} TPacketGCDragonSoulRefine;

typedef struct SPacketCGStateCheck
{
	BYTE header;
	unsigned long key;
	unsigned long index;
} TPacketCGStateCheck;

typedef struct SPacketGCStateCheck
{
	BYTE header;
	unsigned long key;
	unsigned long index;
	unsigned char state;
} TPacketGCStateCheck;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
enum
{
	HEADER_CG_ACCE = 211,
	HEADER_GC_ACCE = 215,
	ACCE_SUBHEADER_GC_OPEN = 0,
	ACCE_SUBHEADER_GC_CLOSE,
	ACCE_SUBHEADER_GC_ADDED,
	ACCE_SUBHEADER_GC_REMOVED,
	ACCE_SUBHEADER_CG_REFINED,
	ACCE_SUBHEADER_CG_CLOSE = 0,
	ACCE_SUBHEADER_CG_ADD,
	ACCE_SUBHEADER_CG_REMOVE,
	ACCE_SUBHEADER_CG_REFINE,
};

typedef struct SPacketAcce
{
	BYTE	header;
	BYTE	subheader;
	bool	bWindow;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	int64_t	dwPrice;
#else
	DWORD	dwPrice;
#endif
	BYTE	bPos;
	TItemPos	tPos;
	DWORD	dwItemVnum;
	DWORD	dwMinAbs;
	DWORD	dwMaxAbs;
} TPacketAcce;
#endif

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
typedef struct packet_target_info
{
	BYTE	header;
	DWORD	dwVID;
	DWORD	race;
	DWORD	dwVnum;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	count;
#else
	BYTE	count;
#endif
} TPacketGCTargetInfo;

typedef struct packet_target_info_load
{
	BYTE header;
	DWORD dwVID;
} TPacketCGTargetInfoLoad;
#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
enum
{
	CUBE_RENEWAL_SUB_HEADER_OPEN_RECEIVE,
	CUBE_RENEWAL_SUB_HEADER_CLEAR_DATES_RECEIVE,
	CUBE_RENEWAL_SUB_HEADER_DATES_RECEIVE,
	CUBE_RENEWAL_SUB_HEADER_DATES_LOADING,
	CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM,
	CUBE_RENEWAL_SUB_HEADER_CLOSE,
};

typedef struct  packet_send_cube_renewal
{
	BYTE header;
	BYTE subheader;
	DWORD index_item;
	DWORD count_item;
}TPacketCGCubeRenewalSend;

typedef struct dates_cube_renewal
{
	DWORD npc_vnum;
	DWORD index;

	DWORD	vnum_reward;
	int		count_reward;

	bool 	item_reward_stackable;

	DWORD	vnum_material_1;
	int		count_material_1;

	DWORD	vnum_material_2;
	int		count_material_2;

	DWORD	vnum_material_3;
	int		count_material_3;

	DWORD	vnum_material_4;
	int		count_material_4;

	DWORD	vnum_material_5;
	int		count_material_5;

	DWORD	vnum_material_6;
	int		count_material_6;

	DWORD	vnum_material_7;
	int		count_material_7;

	DWORD	vnum_material_8;
	int		count_material_8;

	DWORD	vnum_material_9;
	int		count_material_9;

	DWORD	vnum_material_10;
	int		count_material_10;

	long long 	gold;
	int 	percent;

	char 	category[100];
}TInfoDateCubeRenewal;

typedef struct packet_receive_cube_renewal
{
	packet_receive_cube_renewal() : header(HEADER_GC_CUBE_RENEWAL)
	{}

	BYTE header;
	BYTE subheader;
	TInfoDateCubeRenewal	date_cube_renewal;
}TPacketGCCubeRenewalReceive;
#endif

#ifdef ENABLE_DS_SET_BONUS
typedef struct STPacketDSTable {
	BYTE	bHeader;
	int		iType, iApplyCount, iBasicApplyValue[255], iAditionalApplyValue[255];
	float	fWeight;
} TPacketDSTable;
#endif

#ifdef ENABLE_OFFLINE_SHOP
typedef struct SPacketGCNewOfflineshop
{
	BYTE bHeader;
#ifdef ENABLE_LARGE_DYNAMIC_PACKET
	int wSize;
#else
	WORD wSize;
#endif
	BYTE bSubHeader;
} TPacketGCNewOfflineshop;

typedef struct SPacketCGNewOfflineShop
{
	BYTE bHeader;
	uint16_t wSize;
	BYTE bSubHeader;
} TPacketCGNewOfflineShop;

namespace offlineshop
{
	typedef struct SFilterInfo
	{
		BYTE bType;
		BYTE bSubType;
		char szName[OFFLINE_SHOP_ITEM_MAX_LEN];
		TPriceInfo priceStart, priceEnd;
		BYTE iLevelStart, iLevelEnd;
		DWORD dwWearFlag;
		bool FindCharacter;//Finding by character name
		MAX_COUNT minCount, maxCount;
		BYTE minAbs, maxAbs;
		BYTE minAvg;
		BYTE alchemyGrade;
		BYTE alchemyPurity;
		BYTE sashLevel;
	} TFilterInfo;

	typedef struct SShopItemInfo
	{
		TItemPos pos;
		TPriceInfo price;
#ifdef ENABLE_OFFLINE_SHOP_GRID
		TItemDisplayPos display_pos;
#endif
	} TShopItemInfo;

	enum eSubHeaderGC
	{
		SUBHEADER_GC_SHOP_LIST,
		SUBHEADER_GC_SHOP_OPEN,
		SUBHEADER_GC_SHOP_OPEN_OWNER,
		SUBHEADER_GC_SHOP_OPEN_OWNER_NO_SHOP,
		SUBHEADER_GC_SHOP_CLOSE,
		SUBHEADER_GC_SHOP_BUY_ITEM_FROM_SEARCH,
		SUBHEADER_GC_SHOP_FILTER_RESULT,
		SUBHEADER_GC_SHOP_SAFEBOX_REFRESH,
#ifdef ENABLE_SHOPS_IN_CITIES
		SUBHEADER_GC_INSERT_SHOP_ENTITY,
		SUBHEADER_GC_REMOVE_SHOP_ENTITY,
#endif
#ifdef ENABLE_OFFLINESHOP_NOTIFICATION
		SUBHEADER_GC_NOTIFICATION,
#endif
#ifdef ENABLE_AVERAGE_PRICE
		SUBHEADER_GC_AVERAGE_PRICE,
#endif
	};

	typedef struct SSubPacketGCShopList
	{
		DWORD dwShopCount;
	} TSubPacketGCShopList;

	typedef struct SSubPacketGCShopOpen
	{
		TShopInfo shop;
	} TSubPacketGCShopOpen;

	typedef struct SSubPacketGCShopOpenOwner
	{
		TShopInfo shop;
	} TSubPacketGCShopOpenOwner;

	typedef struct SSubPacketGCShopFilterResult
	{
		DWORD dwCount;
	} TSubPacketGCShopFilterResult;

	typedef struct SSubPacketGCShopSafeboxRefresh
	{
		unsigned long long valute;
		DWORD dwItemCount;
	} TSubPacketGCShopSafeboxRefresh;

	typedef struct SSubPacketGCShopBuyItemFromSearch
	{
		DWORD dwOwnerID;
		DWORD dwItemID;
	} TSubPacketGCShopBuyItemFromSearch;

#ifdef ENABLE_SHOPS_IN_CITIES
	typedef struct SSubPacketGCInsertShopEntity
	{
		DWORD dwVID;
		char szName[OFFLINE_SHOP_NAME_MAX_LEN];
		int iType;
		long x, y, z;
#ifdef ENABLE_SHOP_DECORATION
		DWORD	dwShopDecoration;
#endif
	} TSubPacketGCInsertShopEntity;

	typedef struct SSubPacketGCRemoveShopEntity
	{
		DWORD dwVID;
	} TSubPacketGCRemoveShopEntity;
#endif

#ifdef ENABLE_OFFLINESHOP_NOTIFICATION
	typedef struct NotifTable 
	{
		DWORD dwItemID;
		int64_t dwItemPrice;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT dwItemCount;
#else
		WORD dwItemCount;
#endif
	} TSubPacketGCNotification;
#endif

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	typedef struct sExtendTime
	{
		DWORD dwTime;
	}TSubPacketCGShopExtendTime;
#endif


#ifdef ENABLE_AVERAGE_PRICE
	typedef struct SSubPacketAveragePrice
	{
		SSubPacketAveragePrice(DWORD vnm, long long prc)
		{
			vnum = vnm;
			price = prc;
		}
		DWORD vnum;
		long long price;
	}TSubPacketAveragePrice;
#endif

	enum eSubHeaderCG
	{
		SUBHEADER_CG_SHOP_CREATE_NEW,
		SUBHEADER_CG_SHOP_CHANGE_NAME,
		SUBHEADER_CG_SHOP_FORCE_CLOSE,
		SUBHEADER_CG_SHOP_REQUEST_SHOPLIST,
		SUBHEADER_CG_SHOP_OPEN,
		SUBHEADER_CG_SHOP_OPEN_OWNER,
		SUBHEADER_CG_SHOP_BUY_ITEM,
		SUBHEADER_CG_SHOP_ADD_ITEM,
		SUBHEADER_CG_SHOP_REMOVE_ITEM,
		SUBHEADER_CG_SHOP_EDIT_ITEM,
		SUBHEADER_CG_SHOP_FILTER_REQUEST,
		SUBHEADER_CG_SHOP_SAFEBOX_OPEN,
		SUBHEADER_CG_SHOP_SAFEBOX_GET_ITEM,
		SUBHEADER_CG_SHOP_SAFEBOX_GET_VALUTES,
		SUBHEADER_CG_SHOP_SAFEBOX_CLOSE,
		SUBHEADER_CG_CLOSE_BOARD,
#ifdef ENABLE_SHOPS_IN_CITIES
		SUBHEADER_CG_CLICK_ENTITY,
#endif
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
		SUBHEADER_CG_SHOP_EXTEND_TIME,
#endif
#ifdef ENABLE_AVERAGE_PRICE
		SUBHEADER_CG_AVERAGE_PRICE,
#endif
	};

	typedef struct SSubPacketCGShopCreate
	{
		TShopInfo shop;
	} TSubPacketCGShopCreate;

	typedef struct SSubPacketCGShopChangeName
	{
		char szName[OFFLINE_SHOP_NAME_MAX_LEN];
	} TSubPacketCGShopChangeName;

	typedef struct SSubPacketCGShopOpen
	{
		DWORD dwOwnerID;
	} TSubPacketCGShopOpen;

	typedef struct SSubPacketCGAddItem
	{
		TItemPos pos;
		TPriceInfo price;
#ifdef ENABLE_OFFLINE_SHOP_GRID
		TItemDisplayPos display_pos;
#endif
	} TSubPacketCGAddItem;

	typedef struct SSubPacketCGRemoveItem
	{
		DWORD dwItemID;
	} TSubPacketCGRemoveItem;

	typedef struct SSubPacketCGEditItem
	{
		DWORD dwItemID;
		TPriceInfo price;
		bool allEdit;
	} TSubPacketCGEditItem;

	typedef struct SSubPacketCGFilterRequest
	{
		TFilterInfo filter;
	} TSubPacketCGFilterRequest;

	typedef struct SSubPacketCGShopSafeboxGetItem
	{
		DWORD dwItemID;
	} TSubPacketCGShopSafeboxGetItem;

	typedef struct SSubPacketCGShopSafeboxGetValutes
	{
		unsigned long long gold;
	}TSubPacketCGShopSafeboxGetValutes;

	typedef struct SSubPacketCGShopBuyItem
	{
		DWORD dwOwnerID;
		DWORD dwItemID;
		bool bIsSearch;
	} TSubPacketCGShopBuyItem;

#ifdef ENABLE_SHOPS_IN_CITIES
	typedef struct SSubPacketCGShopClickEntity
	{
		DWORD dwShopVID;
	} TSubPacketCGShopClickEntity;
#endif
}
#endif

#ifdef ENABLE_SWITCHBOT
struct TPacketGGSwitchbot
{
	uint8_t bHeader;
	uint32_t wPort;
	TSwitchbotTable table;

	TPacketGGSwitchbot() : bHeader(HEADER_GG_SWITCHBOT), wPort(0)
	{
		table = {};
	}
};

enum ECGSwitchbotSubheader
{
	SUBHEADER_CG_SWITCHBOT_START,
	SUBHEADER_CG_SWITCHBOT_STOP,
};

struct TPacketCGSwitchbot
{
	uint8_t header;
	int32_t size;
	uint8_t subheader;
	uint8_t slot;
};

enum EGCSwitchbotSubheader
{
	SUBHEADER_GC_SWITCHBOT_UPDATE,
	SUBHEADER_GC_SWITCHBOT_UPDATE_ITEM,
	SUBHEADER_GC_SWITCHBOT_SEND_ATTRIBUTE_INFORMATION,
};

struct TPacketGCSwitchbot
{
	uint8_t header;
	int32_t size;
	uint8_t subheader;
	uint8_t slot;
};

struct TSwitchbotUpdateItem
{
	uint8_t	slot;
	uint8_t	vnum;
	uint8_t	count;
	uint32_t	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};
#endif

#ifdef ENABLE_REFRESH_CONTROL
typedef struct SPacketGCRefreshControl
{
	BYTE Header;
	BYTE SubHeader;
	bool state;
	bool update;
}TPacketGCRefreshControl;

enum RefreshControlSubPackets
{
	REFRESH_INVENTORY,
	REFRESH_SKILL,
	REFRESH_POINT
};
#endif

#ifdef ENABLE_GLOBAL_RANKING
typedef struct SRankingGCPacket
{
	BYTE	bHeader;
	WORD	wSize;
	DWORD	next_update_second;
}TRankingGCPacket;
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
enum BiologSubPacket
{
	SUBHEADER_GC_BIOLOG_OPEN,
	SUBHEADER_GC_BIOLOG_LEVEL_UPDATE,
	SUBHEADER_GC_BIOLOG_COUNT_UPDATE,
	SUBHEADER_GC_BIOLOG_MAX_LEVEL,
	SUBHEADER_CG_BIOLOG_OPEN = 0,
	SUBHEADER_CG_BIOLOG_UPGRADE,
};

typedef struct SPacketCGBiologUpgrade
{
	SPacketCGBiologUpgrade(bool a, bool b, bool c)
	{
		time_reset = a;
		chance = b;
		allgive = c;
	};
	bool time_reset;
	bool chance;
	bool allgive;
}TPacketCGBiologUpgrade;

typedef struct SPacketGCBiologInformations
{
	BYTE mission_level;
	DWORD mission_vnum;
	DWORD mission_stone_vnum;
	MAX_COUNT mission_count;
	DWORD mission_cooldown;
	TBiologBonus mission_bonus[4];
}TPacketGCBiologInformations;

typedef struct SPacketGCBiolog
{
	BYTE header;
	WORD size;
	BYTE sub_header;
}TPacketGCBiolog;

typedef struct SPacketCGBiolog
{
	BYTE header;
	BYTE sub_header;
}TPacketCGBiolog;
#endif

#ifdef ENABLE_CHAR_SETTINGS
typedef struct SPacketCharSettings
{
	BYTE header;
	TCharSettings data;
	BYTE type;
}TPacketGCCharSettings;
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
namespace InGameLog
{
	enum InGameLogGCSubHeader
	{
		SUBHEADER_GC_OFFLINESHOP_LOG_OPEN,
		SUBHEADER_GC_OFFLINESHOP_LOG_ADD,
		SUBHEADER_GC_TRADE_LOG_OPEN,
		SUBHEADER_GC_TRADE_LOG_DETAILS_OPEN
	};

	enum InGameLogCGSubHeader
	{
		SUBHEADER_CG_OFFLINESHOP_LOG_OPEN,
		SUBHEADER_CG_TRADE_LOG_OPEN,
		SUBHEADER_CG_TRADE_LOG_DETAILS_OPEN,
	};

	typedef struct SPacketGCInGameLog
	{
		BYTE header;
		WORD size;
		BYTE subHeader;
	}TPacketGCInGameLog;

	typedef struct SPacketCGInGameLog
	{
		BYTE header;
		BYTE subHeader;
		DWORD logID;
	}TPacketCGInGameLog;
}
#endif

#ifdef ENABLE_ITEM_TRANSACTIONS
typedef struct SItemTransactionsCGPacket
{
	BYTE header;
	BYTE transaction;
	WORD itemCount;
}TItemTransactionsCGPacket;

typedef struct SItemTransactionsInfo
{
	WORD pos;
	BYTE window;
}TItemTransactionsInfo;
#endif


#ifdef ENABLE_PM_ALL_SEND_SYSTEM
typedef struct SPacketCGBulkWhisper
{
	BYTE	header;
	char	szText[512 + 1];
} TPacketCGBulkWhisper;

typedef struct packet_bulk_whisper
{
	BYTE	header;
	WORD	size;
} TPacketGCBulkWhisper;

typedef struct SPacketGGBulkWhisper
{
	BYTE	bHeader;
	long	lSize;
} TPacketGGBulkWhisper;
#endif

#ifdef ENABLE_DUNGEON_INFO
enum EDungeonInfoSubHeader
{
	SUB_HEADER_CG_DUNGEON_LOGIN_REQUEST,
	SUB_HEADER_CG_DUNGEON_TIME_RESET,
	SUB_HEADER_CG_TIME_REQUEST,

	SUB_HEADER_GC_TIME_REQUEST = 0,
	SUB_HEADER_GC_TIME_UPDATE,
	SUB_HEADER_GC_DUNGEON_MESSAGE,
};

typedef struct SPacketDungeonInfoCG
{
	BYTE header;
	BYTE subHeader;
}TPacketDungeonInfoCG;

typedef struct SPacketDungeonInfoGC
{
	BYTE header;
	WORD size;
	BYTE subHeader;
}TPacketDungeonInfoGC;
#endif

#ifdef ENABLE_EVENT_MANAGER
typedef struct SPacketGCEventManager
{
	BYTE	header;
	DWORD	size;
} TPacketGCEventManager;
#endif

#ifdef ENABLE_ITEMSHOP
typedef struct SPacketGCItemShop
{
	BYTE	header;
	DWORD	size;
} TPacketGCItemShop;
#endif

#ifdef ENABLE_TELEPORT_TO_A_FRIEND
enum EFriendTeleportGGSubHeader
{
	SUBHEADER_GG_TELEPORT_REQUEST,
	SUBHEADER_GG_TELEPORT_ANSWER
};
typedef struct SPacketGGTeleportRequest
{
	BYTE	header;
	BYTE	subHeader;
	char	sender[CHARACTER_NAME_MAX_LEN + 1];
	char	target[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGTeleportRequest;
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
typedef struct SPacketCGSkillColor
{
	BYTE bheader;
	BYTE skill;
	DWORD col1;
	DWORD col2;
	DWORD col3;
	DWORD col4;
	DWORD col5;
} TPacketCGSkillColor;
#endif

#ifdef ENABLE_AURA_SYSTEM
enum
{
	AURA_SUBHEADER_GC_OPEN = 0,
	AURA_SUBHEADER_GC_CLOSE,
	AURA_SUBHEADER_GC_ADDED,
	AURA_SUBHEADER_GC_REMOVED,
	AURA_SUBHEADER_GC_REFINED,
	AURA_SUBHEADER_CG_CLOSE = 0,
	AURA_SUBHEADER_CG_ADD,
	AURA_SUBHEADER_CG_REMOVE,
	AURA_SUBHEADER_CG_REFINE,
};

typedef struct SPacketAura
{
	BYTE    header;
	BYTE    subheader;
	bool    bWindow;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
	unsigned long long    dwPrice;
#else
	DWORD    dwPrice;
#endif
	BYTE    bPos;
	TItemPos    tPos;
	DWORD    dwItemVnum;
	DWORD    dwMinAbs;
	DWORD    dwMaxAbs;
} TPacketAura;
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
enum ENewPetPacket
{
	SUB_GC_UNSUMMON,
	SUB_GC_UPDATE,
	SUB_GC_UPDATE_EXP,

	SUB_CG_STAR_INCREASE = 0,
	SUB_CG_TYPE_INCREASE,
	SUB_CG_EVOLUTION_INCREASE,
	SUB_CG_SKIN_CHANGE,
	SUB_CG_ACTIVATE_SKIN,
	SUB_CG_BONUS_CHANGE,
	SUB_CG_RESET_SKILL,
};

typedef struct SNewPetGCPacket
{
	BYTE header;
	WORD size;
	BYTE subHeader;
}TNewPetGCPacket;

typedef struct SNewPetCGPacket
{
	BYTE header;
	BYTE subHeader;
}TNewPetCGPacket;
#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
typedef struct SPacketGGLocaleNotice
{
	BYTE bHeader;
	bool bBigFont;
	char szNotice[MAX_QUEST_NOTICE_ARGS + 1][CHAT_MAX_LEN + 1];
} TPacketGGLocaleNotice;

typedef struct SPacketChangeLanguage
{
	BYTE bHeader;
	BYTE bLanguage;
} TPacketChangeLanguage;
#endif

#ifdef ENABLE_EXTENDED_WHISPER_DETAILS
typedef struct SPacketCGWhisperDetails
{
	BYTE header;
	char name[CHARACTER_NAME_MAX_LEN + 1];
} TPacketCGWhisperDetails;

typedef struct SPacketGCWhisperDetails
{
	BYTE header;
	char name[CHARACTER_NAME_MAX_LEN + 1];
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	BYTE bLanguage;
#endif
} TPacketGCWhisperDetails;
#endif

#ifdef ENABLE_MAINTENANCE_SYSTEM
typedef struct SPacketGGMaintenance
{
	BYTE	bHeader;
	bool	bMaintenance;
	DWORD	dwRemainingTime;
	DWORD	dwMaintenanceTime;
} TPacketGGMaintenance;
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
typedef struct SPacketCGExtBattlePassAction
{
	BYTE bHeader;
	BYTE bAction;
} TPacketCGExtBattlePassAction;

typedef struct SPacketCGExtBattlePassSendPremium
{
	uint8_t bHeader;
	bool premium;
} TPacketCGExtBattlePassSendPremium;

typedef struct SPacketGCExtBattlePassOpen
{
	BYTE bHeader;
} TPacketGCExtBattlePassOpen;

typedef struct SPacketGCExtBattlePassGeneralInfo
{
	BYTE bHeader;
	BYTE bBattlePassType;
	char	szSeasonName[64 + 1];
	DWORD dwBattlePassID;
	DWORD dwBattlePassStartTime;
	DWORD dwBattlePassEndTime;
} TPacketGCExtBattlePassGeneralInfo;

typedef struct SPacketGCExtBattlePassMissionInfo
{
	BYTE bHeader;
	WORD wSize;
	WORD wRewardSize;
	BYTE bBattlePassType;
	DWORD dwBattlePassID;
} TPacketGCExtBattlePassMissionInfo;

typedef struct SPacketGCExtBattlePassMissionUpdate
{
	BYTE bHeader;
	BYTE bBattlePassType;
	BYTE bMissionIndex;
	BYTE bMissionType;
	DWORD dwNewProgress;
} TPacketGCExtBattlePassMissionUpdate;

typedef struct SPacketGCExtBattlePassRanking
{
	BYTE bHeader;
	char	szPlayerName[CHARACTER_NAME_MAX_LEN + 1];
	BYTE bBattlePassType;
	BYTE	bBattlePassID;
	DWORD	dwStartTime;
	DWORD	dwEndTime;
} TPacketGCExtBattlePassRanking;
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
typedef struct SPacketCGRequestPartyBonus 
{
	BYTE		bHeader;
	BYTE		bBonusID;
} TPacketCGRequestPartyBonus;
#endif

#ifdef ENABLE_6TH_7TH_ATTR
typedef struct command_67_attr
{
	BYTE			bHeader;
	BYTE			bMaterialCount;
	BYTE			bSupportCount;
	short			sSupportPos;
	short			sItemPos;
} TPacketCG67Attr;

typedef struct command_67_attr_open_close
{
	BYTE			bHeader;
} TPacket67AttrOpenClose;
#endif

#ifdef ENABLE_NEW_CHAT
typedef struct SPacketNewChat
{
	BYTE header;
	WORD size;
	WORD chatID;
	BYTE chatType;
}TPacketNewChat;
#endif

#ifdef ENABLE_BUFFI_SYSTEM
typedef struct SPGCBuffiSkill
{
	BYTE header;
	BYTE skillVnum;
	DWORD vid;
}TPGCBuffiSkill;

typedef struct SPCGBuffi
{
	BYTE header;
	BYTE subHeader;
}TPCGBuffi;
#endif

#ifdef ENABLE_DUNGEON_P2P
enum EP2PDungeon
{
	SUBHEADER_GG_DUNGEON_REQ,
	SUBHEADER_GG_DUNGEON_SEND,
#ifdef ENABLE_DUNGEON_TURN_BACK
	SUBHEADER_GG_DUNGEON_TIME_OUT,
#endif
};

typedef struct SPGGDungeon
{
	BYTE header;
	BYTE subHeader;
	DWORD pid;
	DWORD mapIdx;
	BYTE channel;
	BYTE dungeonId;
}TPGGDungeon;
#endif

#ifdef ENABLE_DUNGEON_TURN_BACK
typedef struct SPGGDungeonTurnBack
{
	BYTE header;
	BYTE subHeader;
	DWORD pid;
	BYTE requestChannel;
	BYTE dungeonChannel;
	DWORD mapIdx;
	long x, y;
	long addr;
	WORD port;
}TPGGDungeonTurnBack;
#endif

#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
/*Client -> Server*/
enum EPacketCGMiniGameSubHeaderOkeyNormal
{
	SUBHEADER_CG_RUMI_START,
	SUBHEADER_CG_RUMI_EXIT,
	SUBHEADER_CG_RUMI_DECKCARD_CLICK,
	SUBHEADER_CG_RUMI_HANDCARD_CLICK,
	SUBHEADER_CG_RUMI_FIELDCARD_CLICK,
	SUBHEADER_CG_RUMI_DESTROY,
};

typedef struct SPacketCGMiniGameOkeyCard
{
	SPacketCGMiniGameOkeyCard() : bHeader(HEADER_CG_OKEY_CARD) {}
	uint8_t bHeader;
	uint16_t wSize;
	uint8_t bSubHeader;
} TPacketCGMiniGameOkeyCard;

typedef struct SSubPacketCGMiniGameCardOpenClose
{
	uint8_t bSafeMode;
} TSubPacketCGMiniGameCardOpenClose;

typedef struct SSubPacketCGMGHandCardClick
{
	int index;
} TSubPacketCGMiniGameHandCardClick;

typedef struct SSubPacketCGMGFieldCardClick
{
	int index;
} TSubPacketCGMiniGameFieldCardClick;

typedef struct SSubPacketCGMGDestroy
{
	int index;
} TSubPacketCGMiniGameDestroy;

/*Server -> Client*/
enum EPacketGCMiniGameSubHeaderOkeyNormal
{
	SUBHEADER_GC_RUMI_OPEN,
	SUBHEADER_GC_RUMI_CARD_UPDATE,
	SUBHEADER_GC_RUMI_CARD_REWARD,
};

typedef struct SPacketGCMiniGameOkeyCard
{
	SPacketGCMiniGameOkeyCard() : bHeader(HEADER_GC_OKEY_CARD) {}
	uint8_t bHeader;
	uint16_t wSize;
	uint8_t bSubHeader;
} TPacketGCMiniGameOkeyCard;

typedef struct SSubPacketGCMiniGameCardOpenClose
{
	uint8_t bSafeMode;
} TSubPacketGCMiniGameCardOpenClose;

typedef struct SSubPacketGCMiniGameCardsInfo
{
	uint32_t cardHandType[5];
	uint32_t cardHandValue[5];
	uint8_t cardHandLeft;
	uint32_t cHandPoint;

	uint32_t cardFieldType[3];
	uint32_t cardFieldValue[3];
	uint32_t cFieldPoint;
} TSubPacketGCMiniGameCardsInfo;

typedef struct SSubPacketGCMiniGameCardsReward
{
	uint32_t cardType[3];
	uint32_t cardValue[3];
	uint32_t cPoint;
} TSubPacketGCMiniGameCardsReward;
#endif

#ifdef ENABLE_MINI_GAME_BNW
typedef struct SPacketCGMinigameBnw
{
	BYTE header;
	BYTE subheader;
} TPacketCGMinigameBnw;

enum 
{
	MINIGAME_BNW_SUBHEADER_CG_START,
	MINIGAME_BNW_SUBHEADER_CG_SELECTED_CARD,
	MINIGAME_BNW_SUBHEADER_CG_FINISHED,
};

typedef struct SPacketGCMinigameBnw
{
	BYTE header;
	BYTE subheader;
} TPacketGCMinigameBnw;

enum 
{
	MINIGAME_BNW_SUBHEADER_GC_START,
	MINIGAME_BNW_SUBHEADER_GC_DRAW_RESULT,
};

typedef struct SPacketGCMinigameBnwDrawResult
{
	BYTE result;
	BYTE playerPoints;
	BYTE enemyPoints;
} TPacketGCMinigameBnwDrawResult;

#endif

#ifdef ENABLE_MINI_GAME_CATCH_KING
typedef struct SCatchKingCard
{
	SCatchKingCard()
	{
		bIndex = 0;
		bIsExposed = false;
	}

	SCatchKingCard(uint8_t index, bool isExposed)
	{
		bIndex = index;
		bIsExposed = isExposed;
	}

	uint8_t bIndex;
	bool bIsExposed;
} TCatchKingCard;

/*Client -> Server*/
enum EPacketCGMiniGameSubHeaderCatchKing
{
	SUBHEADER_CG_CATCH_KING_START,
	SUBHEADER_CG_CATCH_KING_DECKCARD_CLICK,
	SUBHEADER_CG_CATCH_KING_FIELDCARD_CLICK,
	SUBHEADER_CG_CATCH_KING_REWARD,
};

typedef struct SPacketCGMiniGameCatchKing
{
	SPacketCGMiniGameCatchKing() : bHeader(HEADER_CG_MINI_GAME_CATCH_KING) {}
	uint8_t bHeader;
	uint16_t wSize;
	uint8_t bSubheader;
} TPacketCGMiniGameCatchKing;

typedef struct SSubPacketCGMiniGameCatchKingStart {
	uint8_t betNumber;
} TSubPacketCGMiniGameCatchKingStart;

typedef struct SSubPacketCGMiniGameCatchKingFieldCardClick {
	uint8_t cardNumber;
} TSubPacketCGMiniGameCatchKingFieldCardClick;

/*Server -> Client*/
enum EPacketGCMiniGameSubHeaderCatchKing
{
	SUBHEADER_GC_CATCH_KING_EVENT_INFO,
	SUBHEADER_GC_CATCH_KING_START,
	SUBHEADER_GC_CATCH_KING_SET_CARD,
	SUBHEADER_GC_CATCH_KING_RESULT_FIELD,
	SUBHEADER_GC_CATCH_KING_SET_END_CARD,
	SUBHEADER_GC_CATCH_KING_REWARD,
};

typedef struct SPacketGCMiniGameCatchKing
{
	SPacketGCMiniGameCatchKing() : bHeader(HEADER_GC_MINI_GAME_CATCH_KING) {}
	uint8_t bHeader;
	uint16_t wSize;
	uint8_t bSubheader;
} TPacketGCMiniGameCatchKing;

typedef struct SSubPacketGCCatchKingStart
{
	uint32_t dwBigScore;
} TSubPacketGCCatchKingStart;

typedef struct SSubPacketGCCatchKingSetCard
{
	uint8_t bCardInHand;
} TSubPacketGCCatchKingSetCard;

typedef struct SSubPacketGCCatchKingResult
{
	uint32_t dwPoints;
	uint8_t bRowType;
	uint8_t bCardPos;
	uint8_t bCardValue;
	bool bKeepFieldCard;
	bool bDestroyHandCard;
	bool bGetReward;
	bool bIsFiveNearBy;
} TSubPacketGCCatchKingResult;

typedef struct SSubPacketGCCatchKingSetEndCard
{
	uint8_t bCardPos;
	uint8_t bCardValue;
} TSubPacketGCCatchKingSetEndCard;

typedef struct SSubPacketGCCatchKingReward
{
	uint8_t bReturnCode;
} TSubPacketGCCatchKingReward;

typedef struct SSubPacketGCCatchKingEventInfo
{
	uint8_t bIsEnable;
} TSubPacketGCCatchKingEventInfo;
#endif

struct DynamicPacketInfo
{
    uint8_t header;
    uint8_t sub_header;
};

#ifdef BOSS_DAMAGE_RANKING_PLUGIN
enum class EPacketCGBossDamageRankingSubHeaderType : uint8_t
{
    BOSS_DMG_RANKING_RANK_INFO,
    BOSS_DMG_RANKING_DAMAGE_INFO,
};

struct SPacketCGBossDamageRanking
{
    SPacketCGBossDamageRanking() : header(HEADER_GC_BOSS_DMG_RANKING) {}
    uint8_t header;
};

struct SPacketGCRankingGeneralInfo
{
    uint8_t rank_size;
};

struct SPacketGCBossDamageRankingInfo
{
    uint8_t race;
    char name[CHARACTER_NAME_MAX_LEN + 1];
    uint8_t percent_damage;
    uint8_t bad_affect_flag;
};
#endif

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
enum EPlayerBlockType
{
	PLAYER_BLOCK_TYPE_ADD,
	PLAYER_BLOCK_TYPE_REMOVE,
	PLAYER_BLOCK_TYPE_LOAD
};

typedef struct SPacketGCPlayerBlock
{
	BYTE header;
	WORD size;
	BYTE type;
} TPacketGCPlayerBlock;

typedef struct SPacketGCLoadPlayerBlock
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCLoadPlayerBlock;

typedef struct SPacketGGPlayerBlock
{
	BYTE header;
	BYTE type;
	char szBlockingPlayerName[CHARACTER_NAME_MAX_LEN + 1];
	char szBlockedPlayerName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGPlayerBlock;
#endif

#if defined(ENABLE_VOICE_CHAT)
enum EVoiceChatTypes : uint8_t
{
	VOICE_CHAT_TYPE_NONE,
	VOICE_CHAT_TYPE_LOCAL,
	VOICE_CHAT_TYPE_PARTY,
	VOICE_CHAT_TYPE_GUILD,
	VOICE_CHAT_TYPE_MAX_NUM,
};

typedef struct SVoiceChatPacket
{
	uint8_t header;
	uint16_t size;
	uint16_t dataSize;
	uint16_t distance;
	uint8_t type;
	bool isEncoded;
	char name[CHARACTER_NAME_MAX_LEN + 1];
	uint32_t vid;
} TVoiceChatPacket;
#endif

#pragma pack()
#endif
