#ifndef __INC_METIN_II_GAME_CONFIG_H__
#define __INC_METIN_II_GAME_CONFIG_H__

enum
{
	ADDRESS_MAX_LEN = 15
};

enum ItemDestroyTime { ITEM_DESTROY_TIME_AUTOGIVE, ITEM_DESTROY_TIME_DROPGOLD, ITEM_DESTROY_TIME_DROPITEM, ITEM_DESTROY_TIME_MAX };

void config_init(const std::string& st_localeServiceName); // default "" is CONFIG

extern char sql_addr[256];

extern WORD mother_port;
extern WORD p2p_port;

extern char db_addr[ADDRESS_MAX_LEN + 1];
extern WORD db_port;

extern int passes_per_sec;
extern int save_event_second_cycle;
extern int ping_event_second_cycle;
extern int test_server;
extern bool	guild_mark_server;
extern BYTE guild_mark_min_level;

extern bool	g_bNoMoreClient;
extern bool	g_bNoRegen;

// #ifdef ENABLE_NEWSTUFF
extern bool	g_bEmpireShopPriceTripleDisable;
extern bool g_bGlobalShoutEnable;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
extern MAX_COUNT g_bItemCountLimit;
#else
extern BYTE g_bItemCountLimit;
#endif
extern bool	g_bGMHostCheck;
extern int g_iStatusPointGetLevelLimit;
extern int g_iStatusPointSetMaxValue;
extern int g_iShoutLimitLevel;
extern DWORD g_dwSkillBookNextReadMin;
extern DWORD g_dwSkillBookNextReadMax;
extern std::string g_stProxyIP;
// extern int g_iShoutLimitTime;
extern int g_aiItemDestroyTime[ITEM_DESTROY_TIME_MAX];
// #endif

extern BYTE	g_bChannel;

extern bool	map_allow_find(int index);
extern void	map_allow_copy(long* pl, int size);

extern int		g_iUserLimit;
extern time_t	g_global_time;

const char* get_table_postfix();

extern std::string	g_stHostname;
extern std::string	g_stLocale;
extern std::string	g_stLocaleFilename;

extern char		g_szPublicIP[16];
extern char		g_szInternalIP[16];

extern int (*is_twobyte) (const char* str);
extern int (*check_name) (const char* str);

extern bool		g_bSkillDisable;

extern int		g_iFullUserCount;
extern int		g_iBusyUserCount;
extern void		LoadStateUserCount();

extern BYTE	g_bAuthServer;

extern BYTE	PK_PROTECT_LEVEL;

extern void	LoadValidCRCList();
extern bool	IsValidProcessCRC(DWORD dwCRC);
extern bool	IsValidFileCRC(DWORD dwCRC);

extern std::string	g_stAuthMasterIP;
extern WORD		g_wAuthMasterPort;

extern std::string	g_stQuestDir;
//extern std::string	g_stQuestObjectDir;
extern std::set<std::string> g_setQuestObjectDir;

extern std::vector<std::string>	g_stAdminPageIP;
extern std::string	g_stAdminPagePassword;

extern int g_iSyncHackLimitCount;

extern int g_server_id;
extern std::string g_strWebMallURL;

extern int VIEW_RANGE;
extern int VIEW_BONUS_RANGE;

extern bool g_protectNormalPlayer;

extern int gPlayerMaxLevel;
extern int gShutdownAge;
extern int gShutdownEnable;

extern bool gHackCheckEnable;
#ifdef ENABLE_ALIGNMENT_SYSTEM
extern long long g_maxAlignment;
#endif
// missing begin
extern std::string g_stBlockDate;

extern int g_iSpamBlockMaxLevel;
extern unsigned int g_uiSpamBlockScore;
extern unsigned int g_uiSpamBlockDuration;
extern unsigned int g_uiSpamReloadCycle;

extern void map_allow_log();
// missing end

#ifdef ENABLE_AVERAGE_PRICE
extern int g_AveragePriceUpdateTime;
#endif

#ifdef ENABLE_CLIENT_VERSION_AND_ONLY_GM
extern void ClientVersionUpdate();
extern bool g_serverState;
extern int g_clientVersion[2];
#endif

#endif /* __INC_METIN_II_GAME_CONFIG_H__ */
