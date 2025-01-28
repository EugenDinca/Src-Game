#include "stdafx.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "unique_item.h"
#include "affect.h"
#include "dev_log.h"
#include "locale_service.h"
#include "questmanager.h"
#include "skill.h"
#ifdef ENABLE_P2P_WARP
#include "map_location.h"
#endif
#ifdef ENABLE_DUNGEON_P2P
#include "dungeon.h"
#endif
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
#include "bossdamagerankingmanager.hpp"
#endif

////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char* c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin*)c_pData);
}

void CInputP2P::Logout(LPDESC d, const char* c_pData)
{
	TPacketGGLogout* p = (TPacketGGLogout*)c_pData;
	P2P_MANAGER::instance().Logout(p->szName);
}

int CInputP2P::Relay(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGRelay* p = (TPacketGGRelay*)c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const BYTE* c_pbData = (const BYTE*)(c_pData + sizeof(TPacketGGRelay));

	if (!pkChr)
		return p->lSize;

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		if (pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			return p->lSize;
		}

		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*)buf;

		p2->bType = p2->bType & 0x0F;
		if (p2->bType == 0x0F) 
		{
			p2->bType = WHISPER_TYPE_SYSTEM;
		}
		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

#ifdef ENABLE_FULL_NOTICE
int CInputP2P::Notice(LPDESC d, const char* c_pData, size_t uiBytes, bool bBigFont)
#else
int CInputP2P::Notice(LPDESC d, const char* c_pData, size_t uiBytes)
#endif
{
	TPacketGGNotice* p = (TPacketGGNotice*)c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256 + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
#ifdef ENABLE_FULL_NOTICE
	SendNotice(szBuf, bBigFont);
#else
	SendNotice(szBuf);
#endif
	return (p->lSize);
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild* p = (TPacketGGGuild*)c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild* g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
	case GUILD_SUBHEADER_GG_CHAT:
	{
		if (uiBytes < sizeof(TPacketGGGuildChat))
			return -1;

		TPacketGGGuildChat* p = (TPacketGGGuildChat*)c_pData;

		if (g)
			g->P2PChat(p->szText);

		return sizeof(TPacketGGGuildChat);
	}

	case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
	{
		if (uiBytes < sizeof(int))
			return -1;

		int iBonus = *((int*)c_pData);
		CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
		if (pGuild)
		{
			pGuild->SetMemberCountBonus(iBonus);
		}
		return sizeof(int);
	}
	default:
		sys_err("UNKNOWN GUILD SUB PACKET");
		break;
	}
	return 0;
}

struct FuncShout
{
	const char* m_str;
	BYTE m_bEmpire;

	FuncShout(const char* str, BYTE bEmpire) : m_str(str), m_bEmpire(bEmpire)
	{
	}

	void operator () (LPDESC d)
	{
#ifdef ENABLE_NEWSTUFF
		if (!d->GetCharacter() || (!g_bGlobalShoutEnable && d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;
#else
		if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;
#endif
#ifdef ENABLE_STOP_CHAT
		if (!d->GetCharacter()->GetCharSettings().STOP_SHOUT)
		{
			d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
		}
#else
		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
#endif

	}
};

void SendShout(const char* szText, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire));
}

void CInputP2P::Shout(const char* c_pData)
{
	TPacketGGShout* p = (TPacketGGShout*)c_pData;
	SendShout(p->szText, p->bEmpire);
}

void CInputP2P::Disconnect(const char* c_pData)
{
	TPacketGGDisconnect* p = (TPacketGGDisconnect*)c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
void SendLCNotice(const char* szNotice, bool bBigFont, ...)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = c_ref_set.begin();

	while (it != c_ref_set.end())
	{
		LPDESC d = *(it++);
		if (d->GetCharacter())
		{
			std::string strMsg = szNotice;
			const char* c_pszBuf;

			if (!strMsg.empty() && std::all_of(strMsg.begin(), strMsg.end(), ::isdigit))
			{
				DWORD dwKey = atoi(szNotice);
				BYTE bLanguage = (d ? d->GetLanguage() : LOCALE_YMIR);

				c_pszBuf = LC_LOCALE_QUEST_TEXT(dwKey, bLanguage);
			}
			else
			{
				c_pszBuf = szNotice;
			}

			std::string strBuffFilter = c_pszBuf;
			std::string strReplace("%d");

			size_t pos = 0;
			while ((pos = strBuffFilter.find(strReplace)) != std::string::npos)
			{
				strBuffFilter.replace(pos, strReplace.length(), "%s");
			}

			const char* c_pszConvBuf = strBuffFilter.c_str();
			char szNoticeBuf[CHAT_MAX_LEN + 1];

			va_list args;
			va_start(args, bBigFont);
			vsnprintf(szNoticeBuf, sizeof(szNoticeBuf), c_pszConvBuf, args);
			va_end(args);

			const char* c_pszToken;
			const char* c_pszLast = szNoticeBuf;

			std::string strBuff = szNoticeBuf;
			std::string strDelim = "[ENTER]";
			std::string strToken;

			while ((pos = strBuff.find(strDelim)) != std::string::npos)
			{
				strToken = strBuff.substr(0, pos);
				c_pszToken = strToken.c_str();
				d->GetCharacter()->ChatPacket(bBigFont ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", c_pszToken);

				c_pszLast = strBuff.erase(0, pos + strDelim.length()).c_str();
			}
			d->GetCharacter()->ChatPacket(bBigFont ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", c_pszLast);
		}
	}
}

void CInputP2P::LocaleNotice(const char* c_pData)
{
	TPacketGGLocaleNotice* p = (TPacketGGLocaleNotice*)c_pData;
	SendLCNotice(p->szNotice[0], p->bBigFont, p->szNotice[1], p->szNotice[2], p->szNotice[3], p->szNotice[4], p->szNotice[5]);
}
#endif

void CInputP2P::Setup(LPDESC d, const char* c_pData)
{
	TPacketGGSetup* p = (TPacketGGSetup*)c_pData;
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerAdd(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger*)c_pData;
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger*)c_pData;
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
}

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*)c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (ch && ch->GetMapIndex() < 10000)
	{
		TPacketGGWarpCharacter pw;
		pw.header = HEADER_GG_WARP_CHARACTER;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();

#ifdef ENABLE_P2P_WARP
		pw.real_map_index = ch->GetMapIndex();
		if (!CMapLocation::instance().Get(pw.x, pw.y, pw.map_index, pw.addr, pw.port))
		{
			sys_err("cannot find map location for FindPosition index %d x %d y %d name %s", pw.map_index, pw.x, pw.y, ch->GetName());
			return;
		}
#endif

		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*)c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
	if (ch)
	{
#ifdef ENABLE_P2P_WARP
		ch->WarpSet(p->x, p->y, p->real_map_index, p->map_index, p->addr, p->port);
#else
		ch->WarpSet(p->x, p->y, p->mapIndex);
#endif
	}
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex* p = (TPacketGGGuildWarMapIndex*)c_pData;
	CGuildManager& gm = CGuildManager::instance();

	CGuild* g1 = gm.FindGuild(p->dwGuildID1);
	CGuild* g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char* c_pData)
{
	TPacketGGTransfer* p = (TPacketGGTransfer*)c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::LoginPing(LPDESC d, const char* c_pData)
{
	TPacketGGLoginPing* p = (TPacketGGLoginPing*)c_pData;

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char* c_pData)
{
	TPacketGGBlockChat* p = (TPacketGGBlockChat*)c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
}

void CInputP2P::IamAwake(LPDESC d, const char* c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

int CInputP2P::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_GG_SETUP:
		Setup(d, c_pData);
		break;

	case HEADER_GG_LOGIN:
		Login(d, c_pData);
		break;

	case HEADER_GG_LOGOUT:
		Logout(d, c_pData);
		break;

	case HEADER_GG_RELAY:
		if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#ifdef ENABLE_FULL_NOTICE
	case HEADER_GG_BIG_NOTICE:
		if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft, true)) < 0)
			return -1;
		break;
#endif
	case HEADER_GG_NOTICE:
		if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_SHUTDOWN:
		sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
		Shutdown(10);
		break;

	case HEADER_GG_GUILD:
		if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_SHOUT:
		Shout(c_pData);
		break;

	case HEADER_GG_DISCONNECT:
		Disconnect(c_pData);
		break;

	case HEADER_GG_MESSENGER_ADD:
		MessengerAdd(c_pData);
		break;

	case HEADER_GG_MESSENGER_REMOVE:
		MessengerRemove(c_pData);
		break;

	case HEADER_GG_FIND_POSITION:
		FindPosition(d, c_pData);
		break;

	case HEADER_GG_WARP_CHARACTER:
		WarpCharacter(c_pData);
		break;

	case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
		GuildWarZoneMapIndex(c_pData);
		break;

	case HEADER_GG_TRANSFER:
		Transfer(c_pData);
		break;

	case HEADER_GG_RELOAD_CRC_LIST:
		LoadValidCRCList();
		break;

	case HEADER_GG_LOGIN_PING:
		LoginPing(d, c_pData);
		break;

	case HEADER_GG_BLOCK_CHAT:
		BlockChat(c_pData);
		break;

	case HEADER_GG_CHECK_AWAKENESS:
		IamAwake(d, c_pData);
		break;
		
#ifdef BOSS_DAMAGE_RANKING_PLUGIN
	    case HEADER_GG_UPDATE_BOSS_DAMAGE_RANKING:
	    {
	        bossdamageranking::boss_dmg_ranking_manager().initialize();
	        break;
	    }
#endif

#ifdef ENABLE_SWITCHBOT
	case HEADER_GG_SWITCHBOT:
		Switchbot(d, c_pData);
		break;
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	case HEADER_GG_BULK_WHISPER:
		if ((iExtraLen = BulkWhisperSend(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	case HEADER_GG_TELEPORT_REQUEST:
		SendTPRequest(c_pData);
		break;
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	case HEADER_GG_LOCALE_NOTICE:
		LocaleNotice(c_pData);
		break;
#endif
#ifdef ENABLE_MAINTENANCE_SYSTEM
	case HEADER_GG_MAINTENANCE:
		Maintenance(c_pData);
		break;
#endif

#ifdef ENABLE_DUNGEON_P2P
	case HEADER_GG_DUNGEON:
		Dungeon(c_pData);
		break;
#endif
#ifdef ENABLE_DUNGEON_TURN_BACK
	case HEADER_GG_DUNGEON_TURN_BACK:
		DungeonTurnBack(c_pData);
		break;
#endif
	}

	return (iExtraLen);
}

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
void CInputP2P::Switchbot(LPDESC d, const char* c_pData)
{
	const TPacketGGSwitchbot* p = reinterpret_cast<const TPacketGGSwitchbot*>(c_pData);
	if (p->wPort != mother_port)
	{
		return;
	}

	CSwitchbotManager::Instance().P2PReceiveSwitchbot(p->table);
}
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
int CInputP2P::BulkWhisperSend(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGBulkWhisper* p = (TPacketGGBulkWhisper*)c_pData;

	if (uiBytes < sizeof(TPacketGGBulkWhisper) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[CHAT_MAX_LEN + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGBulkWhisper), MIN(p->lSize + 1, sizeof(szBuf)));
	SendBulkWhisper(szBuf);

	return (p->lSize);
}
#endif


#ifdef ENABLE_TELEPORT_TO_A_FRIEND
void CInputP2P::SendTPRequest(const char* c_pData)
{
	if (!c_pData)
		return;

	TPacketGGTeleportRequest* p = (TPacketGGTeleportRequest*)c_pData;

	LPCHARACTER tch = CHARACTER_MANAGER::Instance().FindPC(p->target);

	if (!tch)
		return;

	if (p->subHeader == SUBHEADER_GG_TELEPORT_REQUEST)
	{
		if (tch->IsBlockMode(BLOCK_WARP_REQUEST))
		{
			CCI* sendch = P2P_MANAGER::Instance().Find(p->sender);
			if (sendch)
			{
				LPDESC senddesc;
				if (!(senddesc = sendch->pkDesc))
					return;

				sendch->pkDesc->NewChatPacket(USER_BLOCKED_WARP_REQUESTS, "%s",
					p->target);
			}
			return;
		}

		tch->ChatPacket(CHAT_TYPE_COMMAND, "RequestWarpToCharacter %s", p->sender);
	}
	else
	{
		CCI* pch = P2P_MANAGER::Instance().Find(p->sender);
		if (!pch)
		{
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is not online."), p->sender);
			return;
		}
		tch->WarpToPlayer(pch->pkDesc, pch->dwPID, pch->bChannel, pch->szName);
	}
}
#endif

#ifdef ENABLE_DUNGEON_P2P
void CInputP2P::Dungeon(const char* c_pData)
{
	if (!c_pData)
		return;

	TPGGDungeon* p = (TPGGDungeon*)c_pData;


	if (p->channel != 99 || p->subHeader == SUBHEADER_GG_DUNGEON_SEND)
	{
		if (p->channel != g_bChannel)
			return;			
	}
	else if (g_bChannel != 1)
	{
		return;
	}


	if (p->subHeader == SUBHEADER_GG_DUNGEON_REQ)
	{
		LPSECTREE_MAP pkMapSectree = SECTREE_MANAGER::instance().GetMap(p->mapIdx);
		if (!pkMapSectree)
			return;

		LPDUNGEON dungeon = CDungeonManager::instance().Create(p->mapIdx);
		if (!dungeon)
			return;

		TPGGDungeon newP = *p;
		newP.subHeader = SUBHEADER_GG_DUNGEON_SEND;
		newP.mapIdx = dungeon->GetMapIndex();

		P2P_MANAGER::instance().Send(&newP, sizeof(TPGGDungeon));
	}
	else
	{
		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(p->pid);
		if (!ch)
			return;

		ch->DungeonJoinEnd(p->dungeonId, p->mapIdx);
	}
}
#endif

#ifdef ENABLE_DUNGEON_TURN_BACK
void CInputP2P::DungeonTurnBack(const char* c_pData)
{
	if (!c_pData)
		return;

	TPGGDungeonTurnBack* p = (TPGGDungeonTurnBack*)c_pData;

	if (p->subHeader == SUBHEADER_GG_DUNGEON_REQ)
	{
		if (p->dungeonChannel != g_bChannel)
			return;

		LPSECTREE_MAP pkMapSectree = SECTREE_MANAGER::instance().GetMap(p->mapIdx / 10000);
		if (!pkMapSectree)
			return;

		TPGGDungeonTurnBack newP = *p;
		newP.subHeader = SUBHEADER_GG_DUNGEON_TIME_OUT;

		LPDUNGEON dungeon = CDungeonManager::instance().FindByMapIndex(p->mapIdx);
		if (dungeon)
		{
			long mapIdx;
			if (CMapLocation::instance().Get(newP.x, newP.y, mapIdx, newP.addr, newP.port))
				newP.subHeader = SUBHEADER_GG_DUNGEON_SEND;
		}

		P2P_MANAGER::instance().Send(&newP, sizeof(TPGGDungeonTurnBack));
	}
	else
	{
		if (p->requestChannel != g_bChannel)
			return;

		LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(p->pid);
		if (!ch)
			return;

		if (p->subHeader == SUBHEADER_GG_DUNGEON_SEND)
		{
			ch->WarpSet(p->x, p->y, p->mapIdx, (p->mapIdx / 10000), p->addr, p->port);
		}
		else
		{
			ch->NewChatPacket(STRING_D185);
		}
	}
}
#endif

#ifdef ENABLE_MAINTENANCE_SYSTEM
#include "maintenance.h"
void CInputP2P::Maintenance(const char* c_pData)
{
	auto p = (TPacketGGMaintenance*)c_pData;
	auto& rkMaintenanceMgr = CMaintenance::Instance();

	if (!p->bMaintenance)
		rkMaintenanceMgr.CancelMaintenanceTimer();

	rkMaintenanceMgr.SetMaintenance(p->bMaintenance);
	rkMaintenanceMgr.SetRemainingTime(p->dwRemainingTime);
	rkMaintenanceMgr.SetMaintenanceTime(p->dwMaintenanceTime);
	rkMaintenanceMgr.MaintenancePacket(p->dwRemainingTime, p->dwMaintenanceTime);
}
#endif