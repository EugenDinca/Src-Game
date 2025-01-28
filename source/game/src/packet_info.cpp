#include "stdafx.h"
#include "../../common/stl.h"
#include "constants.h"
#include "packet_info.h"


CPacketInfo::CPacketInfo()
	: m_pCurrentPacket(NULL), m_dwStartTime(0)
{
}

CPacketInfo::~CPacketInfo()
{
	itertype(m_pPacketMap) it = m_pPacketMap.begin();
	for (; it != m_pPacketMap.end(); ++it) {
		M2_DELETE(it->second);
	}
}

void CPacketInfo::Set(int header, int iSize, const char* c_pszName, bool bSeq)
{
	if (m_pPacketMap.find(header) != m_pPacketMap.end())
		return;

	TPacketElement* element = M2_NEW TPacketElement;

	element->iSize = iSize;
	element->stName.assign(c_pszName);
	element->iCalled = 0;
	element->dwLoad = 0;
	m_pPacketMap.insert(std::map<int, TPacketElement*>::value_type(header, element));
}

bool CPacketInfo::Get(int header, int* size, const char** c_ppszName)
{
	std::map<int, TPacketElement*>::iterator it = m_pPacketMap.find(header);

	if (it == m_pPacketMap.end())
		return false;

	*size = it->second->iSize;
	*c_ppszName = it->second->stName.c_str();

	m_pCurrentPacket = it->second;
	return true;
}

TPacketElement* CPacketInfo::GetElement(int header)
{
	std::map<int, TPacketElement*>::iterator it = m_pPacketMap.find(header);

	if (it == m_pPacketMap.end())
		return NULL;

	return it->second;
}

void CPacketInfo::Start()
{
	assert(m_pCurrentPacket != NULL);
	m_dwStartTime = get_dword_time();
}

void CPacketInfo::End()
{
	++m_pCurrentPacket->iCalled;
	m_pCurrentPacket->dwLoad += get_dword_time() - m_dwStartTime;
}

void CPacketInfo::Log(const char* c_pszFileName)
{
	FILE* fp;

	fp = fopen(c_pszFileName, "w");

	if (!fp)
		return;

	std::map<int, TPacketElement*>::iterator it = m_pPacketMap.begin();

	fprintf(fp, "Name             Called     Load       Ratio\n");

	while (it != m_pPacketMap.end())
	{
		TPacketElement* p = it->second;
		++it;

		fprintf(fp, "%-16s %-10d %-10u %.2f\n",
			p->stName.c_str(),
			p->iCalled,
			p->dwLoad,
			p->iCalled != 0 ? (float)p->dwLoad / p->iCalled : 0.0f);
	}

	fclose(fp);
}

CPacketInfoCG::CPacketInfoCG()
{
	Set(HEADER_CG_HANDSHAKE, sizeof(TPacketCGHandshake), "Handshake", false);
	Set(HEADER_CG_TIME_SYNC, sizeof(TPacketCGHandshake), "TimeSync", true);
	Set(HEADER_CG_MARK_LOGIN, sizeof(TPacketCGMarkLogin), "MarkLogin", false);
	Set(HEADER_CG_MARK_IDXLIST, sizeof(TPacketCGMarkIDXList), "MarkIdxList", false);
	Set(HEADER_CG_MARK_CRCLIST, sizeof(TPacketCGMarkCRCList), "MarkCrcList", false);
	Set(HEADER_CG_MARK_UPLOAD, sizeof(TPacketCGMarkUpload), "MarkUpload", false);
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	Set(HEADER_CG_KEY_AGREEMENT, sizeof(TPacketKeyAgreement), "KeyAgreement", false);
#endif
	Set(HEADER_CG_GUILD_SYMBOL_UPLOAD, sizeof(TPacketCGGuildSymbolUpload), "SymbolUpload", false);
	Set(HEADER_CG_SYMBOL_CRC, sizeof(TPacketCGSymbolCRC), "SymbolCRC", false);
	Set(HEADER_CG_LOGIN, sizeof(TPacketCGLogin), "Login", true);
	Set(HEADER_CG_LOGIN2, sizeof(TPacketCGLogin2), "Login2", true);
	Set(HEADER_CG_LOGIN3, sizeof(TPacketCGLogin3), "Login3", true);
	Set(HEADER_CG_ATTACK, sizeof(TPacketCGAttack), "Attack", true);
	Set(HEADER_CG_CHAT, sizeof(TPacketCGChat), "Chat", true);
	Set(HEADER_CG_WHISPER, sizeof(TPacketCGWhisper), "Whisper", true);
	Set(HEADER_CG_CHARACTER_SELECT, sizeof(TPacketCGPlayerSelect), "Select", true);
	Set(HEADER_CG_CHARACTER_CREATE, sizeof(TPacketCGPlayerCreate), "Create", true);
	Set(HEADER_CG_CHARACTER_DELETE, sizeof(TPacketCGPlayerDelete), "Delete", true);
	Set(HEADER_CG_ENTERGAME, sizeof(TPacketCGEnterGame), "EnterGame", true);
	Set(HEADER_CG_ITEM_USE, sizeof(TPacketCGItemUse), "ItemUse", true);
#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	Set(HEADER_CG_ITEM_DELETE, sizeof(TPacketCGItemDelete), "ItemDelete", false);
	Set(HEADER_CG_ITEM_SELL, sizeof(TPacketCGItemSell), "ItemSell", false);
#else
	Set(HEADER_CG_ITEM_DROP, sizeof(TPacketCGItemDrop), "ItemDrop", true);
	Set(HEADER_CG_ITEM_DROP2, sizeof(TPacketCGItemDrop2), "ItemDrop2", true);
#endif
	Set(HEADER_CG_ITEM_MOVE, sizeof(TPacketCGItemMove), "ItemMove", true);
	Set(HEADER_CG_ITEM_PICKUP, sizeof(TPacketCGItemPickup), "ItemPickup", true);
	Set(HEADER_CG_QUICKSLOT_ADD, sizeof(TPacketCGQuickslotAdd), "QuickslotAdd", true);
	Set(HEADER_CG_QUICKSLOT_DEL, sizeof(TPacketCGQuickslotDel), "QuickslotDel", true);
	Set(HEADER_CG_QUICKSLOT_SWAP, sizeof(TPacketCGQuickslotSwap), "QuickslotSwap", true);
	Set(HEADER_CG_SHOP, sizeof(TPacketCGShop), "Shop", true);
	Set(HEADER_CG_ON_CLICK, sizeof(TPacketCGOnClick), "OnClick", true);
	Set(HEADER_CG_EXCHANGE, sizeof(TPacketCGExchange), "Exchange", true);
	Set(HEADER_CG_CHARACTER_POSITION, sizeof(TPacketCGPosition), "Position", true);
	Set(HEADER_CG_SCRIPT_ANSWER, sizeof(TPacketCGScriptAnswer), "ScriptAnswer", true);
	Set(HEADER_CG_SCRIPT_BUTTON, sizeof(TPacketCGScriptButton), "ScriptButton", true);
	Set(HEADER_CG_QUEST_INPUT_STRING, sizeof(TPacketCGQuestInputString), "QuestInputString", true);
	Set(HEADER_CG_QUEST_CONFIRM, sizeof(TPacketCGQuestConfirm), "QuestConfirm", true);
	Set(HEADER_CG_MOVE, sizeof(TPacketCGMove), "Move", true);
	Set(HEADER_CG_SYNC_POSITION, sizeof(TPacketCGSyncPosition), "SyncPosition", true);
	Set(HEADER_CG_FLY_TARGETING, sizeof(TPacketCGFlyTargeting), "FlyTarget", true);
	Set(HEADER_CG_ADD_FLY_TARGETING, sizeof(TPacketCGFlyTargeting), "AddFlyTarget", true);
	Set(HEADER_CG_SHOOT, sizeof(TPacketCGShoot), "Shoot", true);
	Set(HEADER_CG_USE_SKILL, sizeof(TPacketCGUseSkill), "UseSkill", true);
	Set(HEADER_CG_ITEM_USE_TO_ITEM, sizeof(TPacketCGItemUseToItem), "UseItemToItem", true);
	Set(HEADER_CG_TARGET, sizeof(TPacketCGTarget), "Target", true);
	Set(HEADER_CG_WARP, sizeof(TPacketCGWarp), "Warp", true);
	Set(HEADER_CG_MESSENGER, sizeof(TPacketCGMessenger), "Messenger", true);
	Set(HEADER_CG_PARTY_REMOVE, sizeof(TPacketCGPartyRemove), "PartyRemove", true);
	Set(HEADER_CG_PARTY_INVITE, sizeof(TPacketCGPartyInvite), "PartyInvite", true);
	Set(HEADER_CG_PARTY_INVITE_ANSWER, sizeof(TPacketCGPartyInviteAnswer), "PartyInviteAnswer", true);
	Set(HEADER_CG_PARTY_SET_STATE, sizeof(TPacketCGPartySetState), "PartySetState", true);
	Set(HEADER_CG_PARTY_USE_SKILL, sizeof(TPacketCGPartyUseSkill), "PartyUseSkill", true);
	Set(HEADER_CG_PARTY_PARAMETER, sizeof(TPacketCGPartyParameter), "PartyParam", true);
	Set(HEADER_CG_EMPIRE, sizeof(TPacketCGEmpire), "Empire", true);
	Set(HEADER_CG_SAFEBOX_CHECKOUT, sizeof(TPacketCGSafeboxCheckout), "SafeboxCheckout", true);
	Set(HEADER_CG_SAFEBOX_CHECKIN, sizeof(TPacketCGSafeboxCheckin), "SafeboxCheckin", true);
	Set(HEADER_CG_SAFEBOX_ITEM_MOVE, sizeof(TPacketCGItemMove), "SafeboxItemMove", true);
	Set(HEADER_CG_GUILD, sizeof(TPacketCGGuild), "Guild", true);
	Set(HEADER_CG_ANSWER_MAKE_GUILD, sizeof(TPacketCGAnswerMakeGuild), "AnswerMakeGuild", true);
	Set(HEADER_CG_FISHING, sizeof(TPacketCGFishing), "Fishing", true);
	Set(HEADER_CG_ITEM_GIVE, sizeof(TPacketCGGiveItem), "ItemGive", true);
	Set(HEADER_CG_HACK, sizeof(TPacketCGHack), "Hack", true);
	Set(HEADER_CG_MYSHOP, sizeof(TPacketCGMyShop), "MyShop", true);
	Set(HEADER_CG_REFINE, sizeof(TPacketCGRefine), "Refine", true);
	Set(HEADER_CG_CHANGE_NAME, sizeof(TPacketCGChangeName), "ChangeName", true);
	Set(HEADER_CG_PONG, sizeof(BYTE), "Pong", true);
	Set(HEADER_CG_MALL_CHECKOUT, sizeof(TPacketCGSafeboxCheckout), "MallCheckout", true);
	Set(HEADER_CG_SCRIPT_SELECT_ITEM, sizeof(TPacketCGScriptSelectItem), "ScriptSelectItem", true);
	Set(HEADER_CG_DRAGON_SOUL_REFINE, sizeof(TPacketCGDragonSoulRefine), "DragonSoulRefine", false);
	Set(HEADER_CG_STATE_CHECKER, sizeof(BYTE), "ServerStateCheck", false);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	Set(HEADER_CG_ACCE, sizeof(TPacketAcce), "Acce", true);
#endif
#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
	Set(HEADER_CG_TARGET_INFO_LOAD, sizeof(TPacketCGTargetInfoLoad), "TargetInfoLoad", false);
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	Set(HEADER_CG_CUBE_RENEWAL, sizeof(TPacketCGCubeRenewalSend), "CubeRenewalSend", true);
#endif
#ifdef ENABLE_DS_SET_BONUS
	Set(HEADER_GC_DS_TABLE, sizeof(TPacketDSTable), "DSTable", true);
#endif
#ifdef ENABLE_OFFLINE_SHOP
	Set(HEADER_CG_NEW_OFFLINESHOP, sizeof(TPacketCGNewOfflineShop), "NewOfflineshop", true);
#endif
#ifdef ENABLE_SWITCHBOT
	Set(HEADER_CG_SWITCHBOT, sizeof(TPacketGCSwitchbot), "Switchbot", true);
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	Set(HEADER_CG_BIOLOG, sizeof(TPacketCGBiolog), "Biolog", true);
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	Set(HEADER_CG_IN_GAME_LOG, sizeof(InGameLog::TPacketCGInGameLog), "InGameLog", true);
#endif
#ifdef ENABLE_ITEM_TRANSACTIONS
	Set(HEADER_CG_ITEM_TRANSACTIONS, sizeof(TItemTransactionsCGPacket), "itemTransactions", true);
#endif
#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	Set(HEADER_CG_BULK_WHISPER, sizeof(TPacketCGBulkWhisper), "TPacketCGBulkWhisper", true);
#endif
#ifdef ENABLE_DUNGEON_INFO
	Set(HEADER_CG_DUNGEON_INFO, sizeof(TPacketDungeonInfoCG), "DungeonInfo", true);
#endif
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	Set(HEADER_CG_SKILL_COLOR, sizeof(TPacketCGSkillColor), "ChangeSkillColor", true);
#endif
#ifdef ENABLE_AURA_SYSTEM
	Set(HEADER_CG_AURA, sizeof(TPacketAura), "Aura", false);
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	Set(HEADER_CG_NEW_PET, sizeof(TNewPetCGPacket), "NewPet", true);
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	Set(HEADER_CG_CHANGE_LANGUAGE, sizeof(TPacketChangeLanguage), "ChangeLanguage", true);
#endif
#ifdef ENABLE_EXTENDED_WHISPER_DETAILS
	Set(HEADER_CG_WHISPER_DETAILS, sizeof(TPacketCGWhisperDetails), "WhisperDetails", true);
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	Set(HEADER_CG_EXT_BATTLE_PASS_ACTION, sizeof(TPacketCGExtBattlePassAction), "ReciveExtBattlePassActions", true);
	Set(HEADER_CG_EXT_SEND_BP_PREMIUM, sizeof(TPacketCGExtBattlePassSendPremium), "ReciveExtBattlePassPremium", true);
#endif
#ifdef ENABLE_LEADERSHIP_EXTENSION
	Set(HEADER_CG_REQUEST_PARTY_BONUS, sizeof(TPacketCGRequestPartyBonus), "RequestPartyBonus", true);
#endif
#ifdef ENABLE_6TH_7TH_ATTR
	Set(HEADER_CG_67_ATTR, sizeof(TPacketCG67Attr), "67Attr", true);
	Set(HEADER_CG_CLOSE_67_ATTR, sizeof(TPacket67AttrOpenClose), "67AttrClose", true);
#endif
#ifdef ENABLE_BUFFI_SYSTEM
	Set(HEADER_CG_BUFFI, sizeof(TPCGBuffi), "Buffi", true);
#endif
#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
	Set(HEADER_CG_OKEY_CARD, sizeof(TPacketCGMiniGameOkeyCard), "MiniGameOkeyCard", true);
#endif
#ifdef ENABLE_MINI_GAME_BNW
	Set(HEADER_CG_MINIGAME_BNW, sizeof(TPacketCGMinigameBnw), "MinigameBnw", true);
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	Set(HEADER_CG_MINI_GAME_CATCH_KING, sizeof(TPacketCGMiniGameCatchKing), "MiniGameCatchKing", true);
#endif
}

CPacketInfoCG::~CPacketInfoCG()
{
	Log("packet_info.txt");
}

CPacketInfoGG::CPacketInfoGG()
{
	Set(HEADER_GG_SETUP, sizeof(TPacketGGSetup), "Setup", false);
	Set(HEADER_GG_LOGIN, sizeof(TPacketGGLogin), "Login", false);
	Set(HEADER_GG_LOGOUT, sizeof(TPacketGGLogout), "Logout", false);
	Set(HEADER_GG_RELAY, sizeof(TPacketGGRelay), "Relay", false);
	Set(HEADER_GG_NOTICE, sizeof(TPacketGGNotice), "Notice", false);
#ifdef ENABLE_FULL_NOTICE
	Set(HEADER_GG_BIG_NOTICE, sizeof(TPacketGGNotice), "BigNotice", false);
#endif
	Set(HEADER_GG_SHUTDOWN, sizeof(TPacketGGShutdown), "Shutdown", false);
	Set(HEADER_GG_GUILD, sizeof(TPacketGGGuild), "Guild", false);
	Set(HEADER_GG_SHOUT, sizeof(TPacketGGShout), "Shout", false);
	Set(HEADER_GG_DISCONNECT, sizeof(TPacketGGDisconnect), "Disconnect", false);
	Set(HEADER_GG_MESSENGER_ADD, sizeof(TPacketGGMessenger), "MessengerAdd", false);
	Set(HEADER_GG_MESSENGER_REMOVE, sizeof(TPacketGGMessenger), "MessengerRemove", false);
	Set(HEADER_GG_FIND_POSITION, sizeof(TPacketGGFindPosition), "FindPosition", false);
	Set(HEADER_GG_WARP_CHARACTER, sizeof(TPacketGGWarpCharacter), "WarpCharacter", false);
	Set(HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX, sizeof(TPacketGGGuildWarMapIndex), "GuildWarMapIndex", false);
	Set(HEADER_GG_TRANSFER, sizeof(TPacketGGTransfer), "Transfer", false);
	Set(HEADER_GG_RELOAD_CRC_LIST, sizeof(BYTE), "ReloadCRCList", false);
	Set(HEADER_GG_LOGIN_PING, sizeof(TPacketGGLoginPing), "LoginPing", false);
	Set(HEADER_GG_BLOCK_CHAT, sizeof(TPacketGGBlockChat), "BlockChat", false);
	Set(HEADER_GG_CHECK_AWAKENESS, sizeof(TPacketGGCheckAwakeness), "CheckAwakeness", false);
#ifdef ENABLE_SWITCHBOT
	Set(HEADER_GG_SWITCHBOT, sizeof(TPacketGGSwitchbot), "Switchbot", false);
#endif
#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	Set(HEADER_GG_BULK_WHISPER, sizeof(TPacketGGBulkWhisper), "TPacketGGBulkWhisper", false);
#endif
#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	Set(HEADER_GG_TELEPORT_REQUEST, sizeof(TPacketGGTeleportRequest), "SendTPRequest", false);
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	Set(HEADER_GG_LOCALE_NOTICE, sizeof(TPacketGGLocaleNotice), "MultiLanguage", false);
#endif
#ifdef ENABLE_MAINTENANCE_SYSTEM
	Set(HEADER_GG_MAINTENANCE, sizeof(TPacketGGMaintenance), "Maintenance", false);
#endif
#ifdef ENABLE_DUNGEON_P2P
	Set(HEADER_GG_DUNGEON, sizeof(TPGGDungeon), "Dungeon", false);
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	Set(HEADER_GG_DUNGEON_TURN_BACK, sizeof(TPGGDungeonTurnBack), "DungeonTurnBack", false);
#endif
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
	Set(HEADER_GG_UPDATE_BOSS_DAMAGE_RANKING,		sizeof(uint8_t),	"BossDamageRanking",		false);
#endif
}

CPacketInfoGG::~CPacketInfoGG()
{
	Log("p2p_packet_info.txt");
}