#ifndef __INC_CLIENTMANAGER_H__
#define __INC_CLIENTMANAGER_H__

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "../../common/stl.h"
#include "../../common/building.h"
#include "Peer.h"
#include "DBManager.h"
#include "LoginData.h"
#ifdef ENABLE_OFFLINE_SHOP
#include "OfflineshopCache.h"
#endif
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLog.h"
#endif

#define ENABLE_PROTO_FROM_DB


class CPlayerTableCache;
class CItemCache;
class CItemPriceListTableCache;
#ifdef ENABLE_SKILL_COLOR_SYSTEM
class CSKillColorCache;
#endif

class CPacketInfo
{
public:
	void Add(int header);
	void Reset();

	std::map<int, int> m_map_info;
};

size_t CreatePlayerSaveQuery(char* pszQuery, size_t querySize, TPlayerTable* pkTab);

class CClientManager : public CNetBase, public singleton<CClientManager>
{
public:
	typedef std::list<CPeer*>			TPeerList;
	typedef boost::unordered_map<DWORD, CPlayerTableCache*> TPlayerTableCacheMap;
	typedef boost::unordered_map<DWORD, CItemCache*> TItemCacheMap;
	typedef boost::unordered_set<CItemCache*, boost::hash<CItemCache*> > TItemCacheSet;
	typedef boost::unordered_map<DWORD, TItemCacheSet*> TItemCacheSetPtrMap;
	typedef boost::unordered_map<DWORD, CItemPriceListTableCache*> TItemPriceListCacheMap;
	typedef boost::unordered_map<short, BYTE> TChannelStatusMap;
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	typedef std::unordered_map<DWORD, CSKillColorCache *> TSkillColorCacheMap;
#endif

	// MYSHOP_PRICE_LIST

	typedef std::pair< DWORD, DWORD >		TItemPricelistReqInfo;
	// END_OF_MYSHOP_PRICE_LIST

	class ClientHandleInfo
	{
	public:
		DWORD	dwHandle;
		DWORD	account_id;
		DWORD	player_id;
		BYTE	account_index;
		char	login[LOGIN_MAX_LEN + 1];
		char	safebox_password[SAFEBOX_PASSWORD_MAX_LEN + 1];
		char	ip[MAX_HOST_LENGTH + 1];

		TAccountTable* pAccountTable;
		TSafeboxTable* pSafebox;

		ClientHandleInfo(DWORD argHandle, DWORD dwPID = 0)
		{
			dwHandle = argHandle;
			pSafebox = NULL;
			pAccountTable = NULL;
			player_id = dwPID;
		};

		ClientHandleInfo(DWORD argHandle, DWORD dwPID, DWORD accountId)
		{
			dwHandle = argHandle;
			pSafebox = NULL;
			pAccountTable = NULL;
			player_id = dwPID;
			account_id = accountId;
		};

		~ClientHandleInfo()
		{
			if (pSafebox)
			{
				delete pSafebox;
				pSafebox = NULL;
			}
		}
	};

#ifdef ENABLE_OFFLINE_SHOP
public:
	bool InitializeOfflineshopTable();
	bool RecvOfflineShopBuyItemPacket(const char* data);
	bool RecvOfflineShopLockBuyItem(CPeer* peer, const char* data);
	bool RecvOfflineShopCannotBuyLockItem(const char* data);
	bool RecvOfflineShopEditItemPacket(const char* data);
	bool RecvOfflineShopRemoveItemPacket(const char* data);
	bool RecvOfflineShopAddItemPacket(const char* data);
	bool RecvOfflineShopForceClose(const char* data);
	bool RecvOfflineShopCreateNew(const char* data);
	bool RecvOfflineShopChangeName(const char* data);
	bool RecvOfflineShopSafeboxGetItem(const char* data);
	bool RecvOfflineShopSafeboxGetValutes(const char* data);
	bool RecvOfflineShopSafeboxAddItem(const char* data);
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	bool RecvOfflineShopExtendTime(const char* data);
	bool SendOfflineShopExtendTime(DWORD dwOwnerID, DWORD dwTime);
#endif
	bool SendOfflineShopBuyItemPacket(DWORD dwOwner, DWORD dwGuest, DWORD dwItem);
	bool SendOfflineShopBuyLockedItemPacket(CPeer* peer, DWORD dwOwner, DWORD dwGuest, DWORD dwItem);
	bool SendOfflineShopEditItemPacket(DWORD dwOwner, DWORD dwItem, const offlineshop::TPriceInfo& price);
	bool SendOfflineShopRemoveItemPacket(DWORD dwOwner, DWORD dwItem);
	bool SendOfflineShopAddItemPacket(DWORD dwOwner, DWORD dwItemID, const offlineshop::TItemInfo& rInfo);
	bool SendOfflineShopForceClose(DWORD dwOwnerID);
#ifdef ENABLE_IRA_REWORK
	bool SendOfflineShopCreateNew(const offlineshop::TShopInfo& shop, const std::vector<offlineshop::TItemInfo>& vec, offlineshop::TShopPosition& pos);
#else
	bool SendOfflineShopCreateNew(const offlineshop::TShopInfo& shop, const std::vector<offlineshop::TItemInfo>& vec);
#endif
	bool SendOfflineShopChangeName(DWORD dwOwnerID, const char* szName);
	bool SendOfflineshopShopExpired(DWORD dwOwnerID);
	void SendOfflineshopTable(CPeer* peer);
	void RecvOfflineShopPacket(CPeer* peer, const char* data);
	void SendOfflineshopSafeboxAddItem(DWORD dwOwnerID, DWORD dwItem, const offlineshop::TItemInfoEx& item);
	void SendOfflineshopSafeboxAddValutes(DWORD dwOwnerID, const unsigned long long& valute);
	void SendOfflineshopSafeboxLoad(CPeer* peer, DWORD dwOwnerID, const unsigned long long& valute, const std::vector<offlineshop::TItemInfoEx>& items, const std::vector<DWORD>& ids);
	void SendOfflineshopSafeboxExpiredItem(DWORD dwOwnerID, DWORD itemID);
	void OfflineShopResultQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void OfflineShopResultAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void OfflineShopResultCreateShopQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void OfflineShopResultCreateShopAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void OfflineShopResultSafeboxAddItemQuery(CPeer* peer, SQLMsg* msg, CQueryInfo* pQueryInfo);
	void OfflineshopDurationProcess();
	void OfflineshopExpiredShop(DWORD dwID);
	void OfflineshopLoadShopSafebox(CPeer* peer, DWORD dwID);
	bool IsUsingOfflineshopSystem(DWORD dwID);
private:
	offlineshop::CShopCache m_offlineshopShopCache;
	offlineshop::CSafeboxCache m_offlineshopSafeboxCache;
#endif

public:
	CClientManager();
	~CClientManager();

	bool	Initialize();
	time_t	GetCurrentTime();

	void	MainLoop();
	void	Quit();

	void	GetPeerP2PHostNames(std::string& peerHostNames);
	void	SetTablePostfix(const char* c_pszTablePostfix);
	void	SetPlayerIDStart(int iIDStart);
	int	GetPlayerIDStart() { return m_iPlayerIDStart; }

	int	GetPlayerDeleteLevelLimit() { return m_iPlayerDeleteLevelLimit; }

	DWORD	GetUserCount();

	void	SendAllGuildSkillRechargePacket();
	void	SendTime();

	CPlayerTableCache* GetPlayerCache(DWORD id);
	void			PutPlayerCache(TPlayerTable* pNew);

	void			CreateItemCacheSet(DWORD dwID);
	TItemCacheSet* GetItemCacheSet(DWORD dwID);
	void			FlushItemCacheSet(DWORD dwID);

	CItemCache* GetItemCache(DWORD id);
	void			PutItemCache(TPlayerItem* pNew, bool bSkipQuery = false);
	bool			DeleteItemCache(DWORD id);

#ifdef ENABLE_SKILL_COLOR_SYSTEM
	CSKillColorCache* GetSkillColorCache(DWORD id);
	void PutSkillColorCache(const TSkillColor * pNew);
	void UpdateSkillColorCache();
#endif

	void			UpdatePlayerCache();
	void			UpdateItemCache();

	// MYSHOP_PRICE_LIST

	CItemPriceListTableCache* GetItemPriceListCache(DWORD dwID);

	void			PutItemPriceListCache(const TItemPriceListTable* pItemPriceList);

	void			UpdateItemPriceListCache(void);
	// END_OF_MYSHOP_PRICE_LIST

	void			SendGuildSkillUsable(DWORD guild_id, DWORD dwSkillVnum, bool bUsable);

	void			SetCacheFlushCountLimit(int iLimit);

	template <class Func>
	Func		for_each_peer(Func f);

	CPeer* GetAnyPeer();

	void			ForwardPacket(BYTE header, const void* data, int size, BYTE bChannel = 0, CPeer* except = NULL);

	void			SendNotice(const char* c_pszFormat, ...);

	// @fixme203 directly GetCommand instead of strcpy
	char* GetCommand(char* str, char* command);
	void			ItemAward(CPeer* peer, char* login);

protected:
	void	Destroy();

private:
	bool		InitializeTables();
	bool		InitializeShopTable();
	bool		InitializeMobTable();
	bool		InitializeItemTable();
	bool		InitializeQuestItemTable();
	bool		InitializeSkillTable();
	bool		InitializeRefineTable();
	bool		InitializeBanwordTable();
	bool		InitializeItemAttrTable();
	bool		InitializeItemRareTable();
	bool		InitializeLandTable();
	bool		InitializeObjectProto();
	bool		InitializeObjectTable();

	bool		MirrorMobTableIntoDB();
	bool		MirrorItemTableIntoDB();

	void		AddPeer(socket_t fd);
	void		RemovePeer(CPeer* pPeer);
	CPeer*		GetPeer(IDENT ident);

	int		AnalyzeQueryResult(SQLMsg* msg);
	int		AnalyzeErrorMsg(CPeer* peer, SQLMsg* msg);

	int		Process();

	void            ProcessPackets(CPeer* peer);

	CLoginData* GetLoginData(DWORD dwKey);
	CLoginData* GetLoginDataByLogin(const char* c_pszLogin);
	CLoginData* GetLoginDataByAID(DWORD dwAID);

	void		InsertLoginData(CLoginData* pkLD);
	void		DeleteLoginData(CLoginData* pkLD);

	bool		InsertLogonAccount(const char* c_pszLogin, DWORD dwHandle, const char* c_pszIP);
	bool		DeleteLogonAccount(const char* c_pszLogin, DWORD dwHandle);
	bool		FindLogonAccount(const char* c_pszLogin);

	void		GuildCreate(CPeer* peer, DWORD dwGuildID);
	void		GuildSkillUpdate(CPeer* peer, TPacketGuildSkillUpdate* p);
	void		GuildExpUpdate(CPeer* peer, TPacketGuildExpUpdate* p);
	void		GuildAddMember(CPeer* peer, TPacketGDGuildAddMember* p);
	void		GuildChangeGrade(CPeer* peer, TPacketGuild* p);
	void		GuildRemoveMember(CPeer* peer, TPacketGuild* p);
	void		GuildChangeMemberData(CPeer* peer, TPacketGuildChangeMemberData* p);
	void		GuildDisband(CPeer* peer, TPacketGuild* p);
	void		GuildWar(CPeer* peer, TPacketGuildWar* p);
	void		GuildWarScore(CPeer* peer, TPacketGuildWarScore* p);
	void		GuildChangeLadderPoint(TPacketGuildLadderPoint* p);
	void		GuildUseSkill(TPacketGuildUseSkill* p);
	void		GuildDepositMoney(TPacketGDGuildMoney* p);
	void		GuildWithdrawMoney(CPeer* peer, TPacketGDGuildMoney* p);
	void		GuildWithdrawMoneyGiveReply(TPacketGDGuildMoneyWithdrawGiveReply* p);
	void		GuildWarBet(TPacketGDGuildWarBet* p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		SetGuildWarEndTime(DWORD guild_id1, DWORD guild_id2, time_t tEndTime);

	void		QUERY_BOOT(CPeer* peer, TPacketGDBoot* p);

	void		QUERY_LOGIN(CPeer* peer, DWORD dwHandle, SLoginPacket* data);
	void		QUERY_LOGOUT(CPeer* peer, DWORD dwHandle, const char*);

	void		RESULT_LOGIN(CPeer* peer, SQLMsg* msg);

	void		QUERY_PLAYER_LOAD(CPeer* peer, DWORD dwHandle, TPlayerLoadPacket*);
	void		RESULT_COMPOSITE_PLAYER(CPeer* peer, SQLMsg* pMsg, DWORD dwQID);
	void		RESULT_PLAYER_LOAD(CPeer* peer, MYSQL_RES* pRes, ClientHandleInfo* pkInfo);
	void		RESULT_ITEM_LOAD(CPeer* peer, MYSQL_RES* pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_QUEST_LOAD(CPeer* pkPeer, MYSQL_RES* pRes, DWORD dwHandle, DWORD dwPID);
	// @fixme402 (RESULT_AFFECT_LOAD +dwRealPID)
	void		RESULT_AFFECT_LOAD(CPeer* pkPeer, MYSQL_RES* pRes, DWORD dwHandle, DWORD dwRealPID);

	// PLAYER_INDEX_CREATE_BUG_FIX
	void		RESULT_PLAYER_INDEX_CREATE(CPeer* pkPeer, SQLMsg* msg);
	// END_PLAYER_INDEX_CREATE_BUG_FIX

#ifdef ENABLE_SKILL_COLOR_SYSTEM
	void		QUERY_SKILL_COLOR_LOAD(CPeer * peer, DWORD dwHandle, TPlayerLoadPacket * packet);
	void		RESULT_SKILL_COLOR_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle);
	void		QUERY_SKILL_COLOR_SAVE(const char * c_pData);
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	void		RESULT_EXT_BATTLE_PASS_LOAD(CPeer* peer, MYSQL_RES* pRes, DWORD dwHandle, DWORD dwRealPID);
	void		QUERY_SAVE_EXT_BATTLE_PASS(CPeer* peer, DWORD dwHandle, TPlayerExtBattlePassMission* battlePass);
#endif

	// MYSHOP_PRICE_LIST

	void		RESULT_PRICELIST_LOAD(CPeer* peer, SQLMsg* pMsg);

	void		RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg);
	// END_OF_MYSHOP_PRICE_LIST

	void		QUERY_PLAYER_SAVE(CPeer* peer, DWORD dwHandle, TPlayerTable*);

	void		__QUERY_PLAYER_CREATE(CPeer* peer, DWORD dwHandle, TPlayerCreatePacket*);
	void		__QUERY_PLAYER_DELETE(CPeer* peer, DWORD dwHandle, TPlayerDeletePacket*);
	void		__RESULT_PLAYER_DELETE(CPeer* peer, SQLMsg* msg);

	void		QUERY_PLAYER_COUNT(CPeer* pkPeer, TPlayerCountPacket*);

	void		QUERY_ITEM_SAVE(CPeer* pkPeer, const char* c_pData);
	void		QUERY_ITEM_DESTROY(CPeer* pkPeer, const char* c_pData);
	void		QUERY_ITEM_FLUSH(CPeer* pkPeer, const char* c_pData);

	void		QUERY_QUEST_SAVE(CPeer* pkPeer, TQuestTable*, DWORD dwLen);
	void		QUERY_ADD_AFFECT(CPeer* pkPeer, TPacketGDAddAffect* p);
	void		QUERY_REMOVE_AFFECT(CPeer* pkPeer, TPacketGDRemoveAffect* p);

	void		QUERY_SAFEBOX_LOAD(CPeer* pkPeer, DWORD dwHandle, TSafeboxLoadPacket*, bool bMall);
	void		QUERY_SAFEBOX_SAVE(CPeer* pkPeer, TSafeboxTable* pTable);
	void		QUERY_SAFEBOX_CHANGE_SIZE(CPeer* pkPeer, DWORD dwHandle, TSafeboxChangeSizePacket* p);
	void		QUERY_SAFEBOX_CHANGE_PASSWORD(CPeer* pkPeer, DWORD dwHandle, TSafeboxChangePasswordPacket* p);

	void		RESULT_SAFEBOX_LOAD(CPeer* pkPeer, SQLMsg* msg);
	void		RESULT_SAFEBOX_CHANGE_SIZE(CPeer* pkPeer, SQLMsg* msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD(CPeer* pkPeer, SQLMsg* msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(CPeer* pkPeer, SQLMsg* msg);

	void		QUERY_EMPIRE_SELECT(CPeer* pkPeer, DWORD dwHandle, TEmpireSelectPacket* p);
	void		QUERY_SETUP(CPeer* pkPeer, DWORD dwHandle, const char* c_pData);

	void		SendPartyOnSetup(CPeer* peer);
	void		ForceFlushCache(CPeer* pkPeer, const char* c_pData);

	void		QUERY_HIGHSCORE_REGISTER(CPeer* peer, TPacketGDHighscore* data);
	void		RESULT_HIGHSCORE_REGISTER(CPeer* pkPeer, SQLMsg* msg);

	void		QUERY_FLUSH_CACHE(CPeer* pkPeer, const char* c_pData);

	void		QUERY_PARTY_CREATE(CPeer* peer, TPacketPartyCreate* p);
	void		QUERY_PARTY_DELETE(CPeer* peer, TPacketPartyDelete* p);
	void		QUERY_PARTY_ADD(CPeer* peer, TPacketPartyAdd* p);
	void		QUERY_PARTY_REMOVE(CPeer* peer, TPacketPartyRemove* p);
	void		QUERY_PARTY_STATE_CHANGE(CPeer* peer, TPacketPartyStateChange* p);
	void		QUERY_PARTY_SET_MEMBER_LEVEL(CPeer* peer, TPacketPartySetMemberLevel* p);

	void		QUERY_RELOAD_PROTO();

	void		QUERY_CHANGE_NAME(CPeer* peer, DWORD dwHandle, TPacketGDChangeName* p);
	void		GetPlayerFromRes(TPlayerTable* player_table, MYSQL_RES* res);

	void		QUERY_LOGIN_KEY(CPeer* pkPeer, TPacketGDLoginKey* p);

	void		AddGuildPriv(TPacketGiveGuildPriv* p);
	void		AddEmpirePriv(TPacketGiveEmpirePriv* p);
	void		AddCharacterPriv(TPacketGiveCharacterPriv* p);

	void		QUERY_AUTH_LOGIN(CPeer* pkPeer, DWORD dwHandle, TPacketGDAuthLogin* p);

	void		QUERY_LOGIN_BY_KEY(CPeer* pkPeer, DWORD dwHandle, TPacketGDLoginByKey* p);
	void		RESULT_LOGIN_BY_KEY(CPeer* peer, SQLMsg* msg);

	void		ChargeCash(const TRequestChargeCash* p);

	void		LoadEventFlag();
	void		SetEventFlag(TPacketSetEventFlag* p);
	void		SendEventFlagsOnSetup(CPeer* peer);

	void		MarriageAdd(TPacketMarriageAdd* p);
	void		MarriageUpdate(TPacketMarriageUpdate* p);
	void		MarriageRemove(TPacketMarriageRemove* p);

	void		WeddingRequest(TPacketWeddingRequest* p);
	void		WeddingReady(TPacketWeddingReady* p);
	void		WeddingEnd(TPacketWeddingEnd* p);

	// MYSHOP_PRICE_LIST

	void		MyshopPricelistUpdate(const TItemPriceListTable* pPacket); // @fixme403 (TPacketMyshopPricelistHeader to TItemPriceListTable)

	void		MyshopPricelistRequest(CPeer* peer, DWORD dwHandle, DWORD dwPlayerID);
	// END_OF_MYSHOP_PRICE_LIST

	// Building
	void		CreateObject(TPacketGDCreateObject* p);
	void		DeleteObject(DWORD dwID);
	void		UpdateLand(DWORD* pdw);
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	void ChangeLanguage(const TRequestChangeLanguage* p);
#endif
	// BLOCK_CHAT
	void		BlockChat(TPacketBlockChat* p);
	// END_OF_BLOCK_CHAT

private:
	int					m_looping;
	socket_t				m_fdAccept;
	TPeerList				m_peerList;

	CPeer* m_pkAuthPeer;

	// LoginKey, LoginData pair
	typedef boost::unordered_map<DWORD, CLoginData*> TLoginDataByLoginKey;
	TLoginDataByLoginKey			m_map_pkLoginData;

	// Login LoginData pair
	typedef boost::unordered_map<std::string, CLoginData*> TLoginDataByLogin;
	TLoginDataByLogin			m_map_pkLoginDataByLogin;

	// AccountID LoginData pair
	typedef boost::unordered_map<DWORD, CLoginData*> TLoginDataByAID;
	TLoginDataByAID				m_map_pkLoginDataByAID;

	typedef boost::unordered_map<std::string, CLoginData*> TLogonAccountMap;
	TLogonAccountMap			m_map_kLogonAccount;

	int					m_iPlayerIDStart;
	int					m_iPlayerDeleteLevelLimit;
	int					m_iPlayerDeleteLevelLimitLower;

	std::vector<TMobTable>			m_vec_mobTable;
	std::vector<TItemTable>			m_vec_itemTable;
	std::map<DWORD, TItemTable*>		m_map_itemTableByVnum;

	int					m_iShopTableSize;
	TShopTable* m_pShopTable;

	int					m_iRefineTableSize;
	TRefineTable* m_pRefineTable;

	std::vector<TSkillTable>		m_vec_skillTable;
	std::vector<TBanwordTable>		m_vec_banwordTable;
	std::vector<TItemAttrTable>		m_vec_itemAttrTable;
	std::vector<TItemAttrRareTable>		m_vec_itemRareTable;

	std::vector<building::TLand>		m_vec_kLandTable;
	std::vector<building::TObjectProto>	m_vec_kObjectProto;
	std::map<DWORD, building::TObject*>	m_map_pkObjectTable;

	bool					m_bShutdowned;
	bool					m_bForceFlushHandled;

	TPlayerTableCacheMap			m_map_playerCache;

	TItemCacheMap				m_map_itemCache;
	TItemCacheSetPtrMap			m_map_pkItemCacheSetPtr;
#ifdef ENABLE_SKILL_COLOR_SYSTEM
	TSkillColorCacheMap			m_map_SkillColorCache;
#endif

	// MYSHOP_PRICE_LIST

	TItemPriceListCacheMap m_mapItemPriceListCache;
	// END_OF_MYSHOP_PRICE_LIST

	TChannelStatusMap m_mChannelStatus;

	struct TPartyInfo
	{
		BYTE bRole;
		BYTE bLevel;

		TPartyInfo() :bRole(0), bLevel(0)
		{
		}
	};

	typedef std::map<DWORD, TPartyInfo>	TPartyMember;
	typedef std::map<DWORD, TPartyMember>	TPartyMap;
	typedef std::map<BYTE, TPartyMap>	TPartyChannelMap;
	TPartyChannelMap m_map_pkChannelParty;

	typedef std::map<std::string, long>	TEventFlagMap;
	TEventFlagMap m_map_lEventFlag;

	BYTE					m_bLastHeader;
	int					m_iCacheFlushCount;
	int					m_iCacheFlushCountLimit;

private:
	TItemIDRangeTable m_itemRange;

public:
	bool InitializeNowItemID();
	DWORD GetItemID();
	DWORD GainItemID();
	TItemIDRangeTable GetItemRange() { return m_itemRange; }

	//BOOT_LOCALIZATION
public:

	bool InitializeLocalization();

private:
	std::vector<tLocale> m_vec_Locale;
	//END_BOOT_LOCALIZATION
	//ADMIN_MANAGER

	bool __GetAdminInfo(const char* szIP, std::vector<tAdminInfo>& rAdminVec);
	bool __GetHostInfo(std::vector<std::string>& rIPVec);
	//END_ADMIN_MANAGER

	//RELOAD_ADMIN
	void ReloadAdmin(CPeer* peer, TPacketReloadAdmin* p);
	//END_RELOAD_ADMIN
	void BreakMarriage(CPeer* peer, const char* data);

	struct TLogoutPlayer
	{
		DWORD	pid;
		time_t	time;

		bool operator < (const TLogoutPlayer& r)
		{
			return (pid < r.pid);
		}
	};

	typedef boost::unordered_map<DWORD, TLogoutPlayer*> TLogoutPlayerMap;
	TLogoutPlayerMap m_map_logout;

	void InsertLogoutPlayer(DWORD pid);
	void DeleteLogoutPlayer(DWORD pid);
	void UpdateLogoutPlayer();
	void UpdateItemCacheSet(DWORD pid);

	void FlushPlayerCacheSet(DWORD pid);
	void SendSpareItemIDRange(CPeer* peer);

	void UpdateHorseName(TPacketUpdateHorseName* data, CPeer* peer);
	void AckHorseName(DWORD dwPID, CPeer* peer);
	void DeleteLoginKey(TPacketDC* data);
	void ResetLastPlayerID(const TPacketNeedLoginLogInfo* data);
	//delete gift notify icon
	void DeleteAwardId(TPacketDeleteAwardID* data);
	void UpdateChannelStatus(TChannelStatus* pData);
	void RequestChannelStatus(CPeer* peer, DWORD dwHandle);

#ifdef ENABLE_PROTO_FROM_DB
public:
	bool		InitializeMobTableFromDB();
	bool		InitializeItemTableFromDB();
protected:
	bool		bIsProtoReadFromDB;
#endif

#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
	void FindChannel(CPeer* pkPeer, DWORD dwHandle, TPacketChangeChannel* p);
#endif

#ifdef ENABLE_GLOBAL_RANKING
private:
	bool		InitializeRank();
	void		SendRank(CPeer* pkPeer = NULL);
	TRankingData m_rankingData;
	std::vector<TRanking> m_rankList[StatsTypes::RANKING_MAX];
	std::set<DWORD> m_gmListPid;
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
public:
	bool InitializeInGameLogTable();
//QID RESULT
	void ResultOfflineShopLog(CQueryInfo* pQueryInfo);

//GD RECV
	void RecvInGameLogPacketGD(CPeer* peer, const char* data);
	void RecvOfflineShopAddLogGD(const char* data);
	void RecvOfflineShopRequestLogGD(CPeer* peer,const char* data);

	void RecvTradeAddLogGD(const char* data);
	void RecvTradeLogRequestGD(CPeer* peer, const char* data);
	void RecvTradeLogDetailsRequestGD(CPeer* peer, const char* data);

//DG SEND
	void SendOfflineShopRequestLogDG(CPeer* peer, DWORD ownerID);
	void SendTradeLogRequestDG(CPeer* peer, DWORD ownerID);
	void SendTradeLogDetailsRequestDG(CPeer* peer, InGameLog::TTradeDetailsRequestGD data);

private:
	InGameLog::CacheOfflineShopLog m_ingamelogOfflineShop;
	InGameLog::CacheTradeLog m_ingamelogTrade;
#endif

#ifdef ENABLE_HWID
public:
	bool SetHwidID(TPacketGDAuthLogin* p, BYTE hwidNum = 0);
	BYTE CheckHwid(TPacketGDAuthLogin* p);
	typedef std::map<DWORD, std::string> HWIDIDMAP;

	void RecvHwidPacketGD(CPeer* peer, const char* data);
	void RecvHwidBanGD(const char* data);
private:
	void InitializeHwid();
	HWIDIDMAP m_hwidIDMap;
	std::set<std::string> m_hwidBanList;

#ifdef ENABLE_FARM_BLOCK
public:
	typedef std::map<DWORD, bool> FARM_BLOCK_INFO;//first pid second farm block
	typedef std::map<DWORD, FARM_BLOCK_INFO> FARM_BLOCK_MAP; //first hwidID
	void RecvGetFarmBlockGD(CPeer* peer, const char* data);
	void RecvSetFarmBlockGD(CPeer* peer, const char* data);
	void RecvFarmBlockLogout(const char* data);
private:
	FARM_BLOCK_MAP m_mapFarmBlock;

#endif
#endif

#ifdef ENABLE_EVENT_MANAGER
public:
	bool		InitializeEventManager(bool updateFromGameMaster = false);
	void		RecvEventManagerPacket(const char* data);
	void		UpdateEventManager();
	void		SendEventData(CPeer* pkPeer = NULL, bool updateFromGameMaster = false);
protected:
	std::map<BYTE, std::vector<TEventManagerData>> m_EventManager;
#endif

#ifdef ENABLE_ITEMSHOP
public:
	bool		InitializeItemShop();
	void		SendItemShopData(CPeer* pkPeer = NULL, bool isPacket = false);
	void		RecvItemShop(CPeer* pkPeer, DWORD dwHandle, const char* data);
	long long	GetDragonCoin(DWORD id);
	void		SetDragonCoin(DWORD id, long long amount);
	void		ItemShopIncreaseSellCount(DWORD itemID, int itemCount);
	void		SetEventFlag(const char* flag, int value);
	int			GetEventFlag(const char* flag);
protected:
	int			itemShopUpdateTime;
	std::map<BYTE, std::map<BYTE, std::vector<TIShopData>>> m_IShopManager;
	std::map<DWORD, std::vector<TIShopLogData>> m_IShopLogManager;
#endif

};

template<class Func>
Func CClientManager::for_each_peer(Func f)
{
	TPeerList::iterator it;
	for (it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		f(*it);
	}
	return f;
}
#endif
