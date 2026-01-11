#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libs/libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"

#include "../../common/VnumHelper.h"
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#include "MountSystem.h"
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
#include "PetSystem.h"
#endif
#ifdef ENABLE_GLOBAL_RANKING
#include "statistics_rank.h"
#endif
#ifdef ENABLE_FISHING_RENEWAL
#include "fishing.h"
#endif
#ifdef ENABLE_HWID
#include "hwid_manager.h"
#endif
#ifdef ENABLE_MINING_RENEWAL
#include "mining.h"
#endif
#ifdef ENABLE_BUFFI_SYSTEM
#include "BuffiSystem.h"
#endif
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
#include "player_block.h"
#endif

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		if (ch->GetMountVnum())
		{
			ch->NewChatPacket(STRING_D1);
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->NewChatPacket(STRING_D2);
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

//ACMD(do_user_horse_back)
//{
//	if (ch->GetHorse() != NULL)
//	{
//		ch->HorseSummon(false);
//		ch->NewChatPacket(STRING_D3);
//	}
//	else if (ch->IsHorseRiding() == true)
//	{
//		ch->NewChatPacket(STRING_D4);
//	}
//	else
//	{
//		ch->NewChatPacket(STRING_D2);
//	}
//}

//fix
ACMD(do_user_horse_back)
{
    // 1. Dacă este călare → trebuie să coboare
    if (ch->IsHorseRiding())
    {
        ch->StopRiding();
        ch->NewChatPacket(STRING_D4); // mesajul tău (ex: "Ai coborât de pe mount")
        return;
    }

    // 2. Dacă mount-ul este chemat → îl trimitem înapoi
    if (ch->GetHorse() != NULL)
    {
        ch->HorseSummon(false);
        ch->NewChatPacket(STRING_D3); // mesajul tău (ex: "Mount trimis")
        return;
    }

    // 3. Dacă nu există mount
    ch->NewChatPacket(STRING_D2); // mesajul tău (ex: "Nu ai mount")
}

ACMD(do_user_horse_feed)
{
	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
			ch->NewChatPacket(STRING_D2);
		else
			ch->NewChatPacket(STRING_D5);
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->NewChatPacket(STRING_D6 ,"%s", ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName);
	}
	else
	{
		ch->NewChatPacket(STRING_D7 ,"%s", ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName);
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int         	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo()
		: ch()
		, subcmd(0)
		, left_second(0)
	{
		::memset(szReason, 0, MAX_REASON_LEN);
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
		: seconds(0)
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>(event->info);

	if (info == NULL)
	{
		sys_err("shutdown_event> <Factor> Null pointer");
		return 0;
	}

	int* pSec = &(info->seconds);

	if (*pSec < 0)
	{
		if (-- * pSec == -10)
		{
			const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
        {
            CHARACTER_MANAGER::Instance().ProcessDelayedSave();
            ITEM_MANAGER::Instance().FlushDelayedSave();

            if (db_clientdesc)
                db_clientdesc->DBPacketHeader(HEADER_GD_FORCE_FLUSH_CACHE, 0, 0);
            
            return 0;
        }

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--* pSec;
		return passes_per_sec;
	}
	else
	{
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		SendNotice(LC_TEXT("Shutdown in %d"), true, *pSec);
#else
		char buf[64];
		snprintf(buf, sizeof(buf), LC_TEXT("Shutdown in %d"), *pSec);
		SendNotice(buf, true);
#endif

		--* pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
    {
        CHARACTER_MANAGER::Instance().ProcessDelayedSave();
        ITEM_MANAGER::Instance().FlushDelayedSave();

        if (db_clientdesc)
            db_clientdesc->DBPacketHeader(HEADER_GD_FORCE_FLUSH_CACHE, 0, 0);

        thecore_shutdown();
        return;
    }

	CWarMapManager::instance().OnShutdown();

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	SendNotice(LC_TEXT("The game will shut down after %d seconds."), true, iSec);
#else
	char buf[64];
	snprintf(buf, sizeof(buf), LC_TEXT("The game will shut down after %d seconds."), iSec);
	SendNotice(buf,true);
#endif
	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	if (NULL == ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}

	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));
	Shutdown(10);
#ifdef ENABLE_CLIENT_VERSION_AND_ONLY_GM
	DBManager::instance().DirectQuery("UPDATE common.game_settings SET player_mode = %u;", 1);
	ClientVersionUpdate();
#endif
}

EVENTFUNC(timed_event)
{
	TimedEventInfo* info = dynamic_cast<TimedEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("timed_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

#ifdef ENABLE_DIZCIYA_GOTTEN
	if (1 < 2)
#else
	if (info->left_second <= 0)
#endif
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
		{
			TPacketNeedLoginLogInfo acc_info;
			acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
			acc_info.bLanguage = ch->GetDesc()->GetAccountTable().bLanguage;
#endif
			db_clientdesc->DBPacket(HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info));
		}
		break;
		}

		switch (info->subcmd)
		{
		case SCMD_LOGOUT:
			if (d)
				d->SetPhase(PHASE_CLOSE);
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
			break;

		case SCMD_PHASE_SELECT:
		{
			ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

			if (d)
			{
				d->SetPhase(PHASE_SELECT);
			}
		}
		break;
		}

		return 0;
	}
	else
	{
		ch->NewChatPacket(STRING_D8,"%d", info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	if (ch->m_pkTimedEvent)
	{
		ch->NewChatPacket(STRING_D9);
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
	case SCMD_LOGOUT:
		ch->NewChatPacket(STRING_D10);
		break;

	case SCMD_QUIT:
		ch->NewChatPacket(STRING_D11);
		break;

	case SCMD_PHASE_SELECT:
		ch->NewChatPacket(STRING_D12);
		break;
	}

	if (!ch->CanWarp() && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
	case SCMD_LOGOUT:
	case SCMD_QUIT:
	case SCMD_PHASE_SELECT:
	{
		TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

		{
			if (ch->IsPosition(POS_FIGHTING))
				info->left_second = 10;
			else
				info->left_second = 3;
		}

		info->ch = ch;
		info->subcmd = subcmd;
		strlcpy(info->szReason, argument, sizeof(info->szReason));

		ch->m_pkTimedEvent = event_create(timed_event, info, 1);
	}
	break;
	}
}

ACMD(do_mount)
{
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
#define eFRS_HERESEC	170
		if (iTimeToDead > eFRS_HERESEC)
		{
			ch->NewChatPacket(STRING_D13, "%d", (iTimeToDead - eFRS_HERESEC));
			return;
		}
	}

	//PREVENT_HACK

	if (subcmd == SCMD_RESTART_TOWN)
	{
		if (!ch->CanWarp(false, true))
		{
			if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
			{
				ch->NewChatPacket(STRING_D13, "%d", (iTimeToDead - (180 - g_nPortalLimitTime)));
				return;
			}
		}

#define eFRS_TOWNSEC	173
		if (iTimeToDead > eFRS_TOWNSEC)
		{
			ch->NewChatPacket(STRING_D14, "%d", (iTimeToDead - eFRS_TOWNSEC));
			return;
		}
	}
	//END_PREVENT_HACK

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
#ifdef ENABLE_AFTERDEATH_SHIELD
	ch->SetShieldAffect();
	ch->StartShieldCountdownEvent(ch, AFTERDEATH_SHIELD_DURATION);
#endif
	ch->StartRecoveryEvent();

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap* pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
			case SCMD_RESTART_TOWN:
				PIXEL_POSITION pos;

				if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
					ch->Show(ch->GetMapIndex(), pos.x, pos.y);
				else
					ch->ExitToSavedLocation();

				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				ch->CheckMount();
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
				ch->CheckPet();
#endif
				break;

			case SCMD_RESTART_HERE:
				ch->RestartAtSamePos();
				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				ch->CheckMount();
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
				ch->CheckPet();
#endif
				break;
			}

			return;
		}
	}
	switch (subcmd)
	{
	case SCMD_RESTART_TOWN:
		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
			ch->WarpSet(pos.x, pos.y);
		else
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		ch->PointChange(POINT_HP, 50 - ch->GetHP());
		ch->DeathPenalty(1);
		break;

	case SCMD_RESTART_HERE:
		ch->RestartAtSamePos();
		ch->PointChange(POINT_HP, 50 - ch->GetHP());
		ch->DeathPenalty(0);
		ch->ReviveInvisible(5);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		ch->CheckMount();
#endif
#ifdef ENABLE_PET_COSTUME_SYSTEM
		ch->CheckPet();
#endif
		break;
	}
}

#define MAX_STAT g_iStatusPointSetMaxValue

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->NewChatPacket(STRING_D15);
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	if (ch->IsPolymorphed())
	{
		ch->NewChatPacket(STRING_D15);
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;

	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;

	int point_incr = 1;
	int deline = MAX_STAT - ch->GetRealPoint(idx);
	str_to_number(point_incr, arg2);
	point_incr = MIN(point_incr, deline); //@Check the real point to add (check ch->GetRealPoint(idx) + point_incr >= MAX_STAT)

	if (point_incr <= 0)
		return;
	else if (point_incr > ch->GetPoint(POINT_STAT))
		point_incr = ch->GetPoint(POINT_STAT);

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + point_incr);
	ch->SetPoint(idx, ch->GetPoint(idx) + point_incr);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -point_incr);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->NewChatPacket(STRING_D16);
		return;
	}

	if (ch->GetMapIndex() == 113) // @fixme212
	{
		ch->NewChatPacket(STRING_D17);
		return;
	}

	#ifdef ENABLE_RENEWAL_PVP
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 3) { return; }

	DWORD vid = 0;
	str_to_number(vid, vecArgs[2].c_str());

	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);
	if (!pkVictim)
		return;
	if (pkVictim->IsNPC())
		return;
	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->NewChatPacket(STRING_D18);
		return;
	}

	if (vecArgs[1] == "revenge")
	{
		if (!CPVPManager::Instance().HasPvP(ch, pkVictim))
			return;
		long long betPrice = 0;
		bool pvpData[PVP_BET];
		CPVPManager::instance().Insert(ch, pkVictim, pvpData, betPrice);
	}
	else if (vecArgs[1] == "pvp")
	{
		if (vecArgs.size() != 28) { return; }

		long long betPrice = 0;
		bool pvpData[PVP_BET];
		memset(&pvpData, false, sizeof(pvpData));

		for (DWORD j = 3; j < vecArgs.size() - 1; ++j)
			pvpData[j - 3] = vecArgs[j] == "1" ? true : false;
		str_to_number(betPrice, vecArgs[vecArgs.size() - 1].c_str());

		if (CPVPManager::Instance().IsFighting(ch))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1001"));
			return;
		}
		else if (CPVPManager::Instance().IsFighting(pkVictim))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1002"));
			return;
		}
		else if (betPrice > ch->GetGold())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1000"));
			return;
		}
		else if (betPrice > pkVictim->GetGold())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1003"));
			return;
		}

		CPVPManager::instance().Insert(ch, pkVictim, pvpData, betPrice);
	}
	else if (vecArgs[1] == "close")
	{
		CPVPManager::instance().RemoveCharactersPvP(ch, pkVictim);
	}
#else
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->NewChatPacket(STRING_D18);
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
#endif

}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->NewChatPacket(STRING_D19);
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->NewChatPacket(STRING_D20);
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch (vnum)
		{
		case SKILL_HORSE_WILDATTACK:
		case SKILL_HORSE_CHARGE:
		case SKILL_HORSE_ESCAPE:
		case SKILL_HORSE_WILDATTACK_RANGE:

		case SKILL_7_A_ANTI_TANHWAN:
		case SKILL_7_B_ANTI_AMSEOP:
		case SKILL_7_C_ANTI_SWAERYUNG:
		case SKILL_7_D_ANTI_YONGBI:

		case SKILL_8_A_ANTI_GIGONGCHAM:
		case SKILL_8_B_ANTI_YEONSA:
		case SKILL_8_C_ANTI_MAHWAN:
		case SKILL_8_D_ANTI_BYEURAK:

		case SKILL_ADD_HP:
		case SKILL_RESIST_PENETRATE:
#ifdef ENABLE_PASSIVE_SKILLS
		case SKILL_PASSIVE_MONSTER:
		case SKILL_PASSIVE_STONE:
		case SKILL_PASSIVE_BERSERKER:
		case SKILL_PASSIVE_HUMAN:
		case SKILL_PASSIVE_SKILL_COOLDOWN:
		case SKILL_PASSIVE_DRAGON_HEARTH:
		case SKILL_PASSIVE_HERBOLOGY:
		case SKILL_PASSIVE_UPGRADING:
		case SKILL_PASSIVE_FISHING:
#endif

#ifdef ENABLE_BUFFI_SYSTEM
		case SKILL_BUFFI_1:
		case SKILL_BUFFI_2:
		case SKILL_BUFFI_3:
		case SKILL_BUFFI_4:
		case SKILL_BUFFI_5:
#endif
			ch->SkillLevelUp(vnum);
			break;
		}
	}
}

ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	return;

	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->NewChatPacket(STRING_D21);
		return;
	}

	if (!*arg2 || strlen(arg2) > 6)
	{
		ch->NewChatPacket(STRING_D21);
		return;
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->NewChatPacket(STRING_D21);
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->NewChatPacket(STRING_D22);
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10)
	{
		ch->NewChatPacket(STRING_D23);
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->NewChatPacket(STRING_D24);
		return;
	}

	if (ch->GetDungeon())
	{
		ch->NewChatPacket(STRING_D25);
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->NewChatPacket(STRING_D26);
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

ACMD(do_war)
{
	CGuild* g = ch->GetGuild();

	if (!g)
		return;

	if (g->UnderAnyWar())
	{
		ch->NewChatPacket(STRING_D27);
		return;
	}

	char arg1[256], arg2[256];
	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type < 0)// @fixme20
		{
			return;
		}

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	}

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->NewChatPacket(STRING_D28);
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->NewChatPacket(STRING_D29);
		return;
	}

	switch (g->GetGuildWarState(opp_g->GetID()))
	{
	case GUILD_WAR_NONE:
	{
		if (opp_g->UnderAnyWar())
		{
			ch->NewChatPacket(STRING_D30);
			return;
		}

		int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

		if (g->GetGuildMoney() < iWarPrice)
		{
			ch->NewChatPacket(STRING_D31);
			return;
		}

		if (opp_g->GetGuildMoney() < iWarPrice)
		{
			ch->NewChatPacket(STRING_D32);
			return;
		}
	}
	break;

	case GUILD_WAR_SEND_DECLARE:
	{
		ch->NewChatPacket(STRING_D33);
		return;
	}
	break;

	case GUILD_WAR_RECV_DECLARE:
	{
		if (opp_g->UnderAnyWar())
		{
			ch->NewChatPacket(STRING_D30);
			g->RequestRefuseWar(opp_g->GetID());
			return;
		}
	}
	break;

	case GUILD_WAR_RESERVE:
	{
		ch->NewChatPacket(STRING_D34);
		return;
	}
	break;

	case GUILD_WAR_END:
		return;

	default:
		ch->NewChatPacket(STRING_D35);
		g->RequestRefuseWar(opp_g->GetID());
		return;
	}

	if (!g->CanStartWar(type))
	{
		if (g->GetLadderPoint() == 0)
		{
			ch->NewChatPacket(STRING_D36);
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->NewChatPacket(STRING_D37,"%d", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		return;
	}

	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->NewChatPacket(STRING_D38);
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->NewChatPacket(STRING_D39);
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI* pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->NewChatPacket(STRING_D40);
		g->RequestRefuseWar(opp_g->GetID());
		return;
	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI* pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->NewChatPacket(STRING_D40);
		g->RequestRefuseWar(opp_g->GetID());
		return;
	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->NewChatPacket(STRING_D28);
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->NewChatPacket(STRING_D29);
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->NewChatPacket(STRING_D16);
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);
	// @fixme130 AuthToAdd void -> bool
	bool bIsDenied = answer != 'y';
	bool bIsAdded = MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, bIsDenied); // DENY
	if (bIsAdded && bIsDenied)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->NewChatPacket(STRING_D41,"%s", ch->GetName());
	}
}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{

#ifdef ENABLE_MOUNT_SKIN
#ifdef ENABLE_HIDE_COSTUME
	if (ch->GetWear(WEAR_MOUNT_SKIN) && !ch->IsMountSkinHidden())
#else
	if (ch->GetWear(WEAR_MOUNT_SKIN))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_MOUNT_SKIN);
		DWORD mobVnum = 0;

		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		return;
	}
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
#ifdef ENABLE_MOUNT_SKIN
	else if (ch->GetWear(WEAR_COSTUME_MOUNT))
#else
	if (ch->GetWear(WEAR_COSTUME_MOUNT))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		DWORD mobVnum = 0;

		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
			{
				mountSystem->Unmount(mobVnum);
			}
		}
		return;
	}
#endif

	LPITEM item = ch->GetWear(WEAR_UNIQUE1);
	LPITEM item2 = ch->GetWear(WEAR_UNIQUE2);
	LPITEM item3 = ch->GetWear(WEAR_COSTUME_MOUNT);

	if (item && item->IsRideItem())
		ch->UnequipItem(item);

	if (item2 && item2->IsRideItem())
		ch->UnequipItem(item2);

	if (item3 && item3->IsRideItem())
		ch->UnequipItem(item3);

	if (ch->IsHorseRiding())
	{
		ch->StopRiding();
	}
}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;

		tch->SendEquipment(ch);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->NewChatPacket(STRING_D16);
		return;
	}

	if (ch->GetParty())
	{
		ch->NewChatPacket(STRING_D43);
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name = "";
		empire = 0;
		mapIndex = 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name = c_src.st_name;
		empire = c_src.empire;
		mapIndex = c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

ACMD(do_inventory)
{
	int	index = 0;
	int	count = 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s",
			index, item ? item->GetName() : "<NONE>");
		++index;
	}
}

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
ACMD(do_cube)
{
	const char* line;
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		return;
	}

	switch (LOWER(arg1[0]))
	{
	case 'o':	// open
		Cube_open(ch);
		break;

	default:
		return;
	}
}
#endif

ACMD(do_in_game_mall)
{
	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
		case LC_GERMANY:	country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
		case LC_FRANCE:		country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_ITALY:		country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
		case LC_SPAIN:		country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
		case LC_UK:			country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
		case LC_TURKEY:		country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_POLAND:		country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
		case LC_PORTUGAL:	country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
		case LC_GREEK:		country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_RUSSIA:		country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
		case LC_DENMARK:	country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
		case LC_BULGARIA:	country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
		case LC_CROATIA:	country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
		case LC_MEXICO:		country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
		case LC_ARABIA:		country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
		case LC_CZECH:		country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
		case LC_ROMANIA:	country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
		case LC_HUNGARY:	country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
		case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
		case LC_USA:		country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
		case LC_CANADA:	country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
		case LC_EUROPE: country_code[0] = 'e'; country_code[1] = 'u'; country_code[2] = '\0'; break;
#endif
		default:
			break;
		}

		char buf[512 + 1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF9001";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char*)buf, strlen(buf));
#ifdef __FreeBSD__
		MD5End(&ctx, sas);
#else
		static const char hex[] = "0123456789abcdef";
		unsigned char digest[16];
		MD5Final(digest, &ctx);
		int i;
		for (i = 0; i < 16; ++i) {
			sas[i + i] = hex[digest[i] >> 4];
			sas[i + i + 1] = hex[digest[i] & 0x0f];
		}
		sas[i + i] = '\0';
#endif

		snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
			g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}

ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

#ifdef ENABLE_DICE_SYSTEM
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_DICE_INFO, LC_TEXT("%s rolled the dice, resulting in %d. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_DICE_INFO, LC_TEXT("You roll %d. (%d-%d)"), n, start, end);
#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_TEXT("%s rolled the dice, resulting in %d. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->NewChatPacket(STRING_D44, "%d|%d|%d", n, start, end);
#endif
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_click_safebox)
{
	if ((ch->GetGMLevel() <= GM_PLAYER) && (ch->GetDungeon() || ch->GetWarMap()))
	{
		ch->NewChatPacket(STRING_D45);
		return;
	}

	ch->SetSafeboxOpenPosition();
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
}
ACMD(do_force_logout)
{
	LPDESC pDesc = DESC_MANAGER::instance().FindByCharacterName(ch->GetName());
	if (!pDesc)
		return;
	pDesc->DelayedDisconnect(0);
}
#endif

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
	if (ch->IsDead() || ch->IsStun())
		return;

#ifdef ENABLE_COSTUME_RELATED_FIXES
	LPITEM armor = ch->GetWear(WEAR_BODY);
	int vnumList[4] = { 12000, 12001, 12002, 12003 };

	for (int i = 0; i < _countof(vnumList); i++)
	{
		if (armor && armor->GetVnum() == vnumList[i])
		{
			if (ch->GetEmptyInventory(armor->GetSize()) == -1)
				return;
			else
				ch->UnequipItem(armor);
		}
	}
#endif

	if (ch->GetMapIndex() == 113) 
	{
		ch->NewChatPacket(STRING_D46);
		return;
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ch->IsPolymorphed() == true) 
	{
		ch->NewChatPacket(STRING_D15);
		return;
	}

	if (ch->GetWear(WEAR_BODY)) 
	{
		LPITEM armor = ch->GetWear(WEAR_BODY);

		if (armor && (armor->GetVnum() >= 11901 && armor->GetVnum() <= 11904)) {
			ch->NewChatPacket(STRING_D47);
			return;
		}
	}

#ifdef ENABLE_MOUNT_SKIN
#ifdef ENABLE_HIDE_COSTUME
	if (ch->GetWear(WEAR_MOUNT_SKIN) && !ch->IsMountSkinHidden())
#else
	if (ch->GetWear(WEAR_MOUNT_SKIN))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_MOUNT_SKIN);
		DWORD mobVnum = 0;
	
		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
				mountSystem->Unmount(mobVnum);
		}
		else
		{
			if (mountSystem->CountSummoned() == 1)
				mountSystem->Mount(mobVnum, mount);
		}

		return;
	}
#endif

#ifdef ENABLE_MOUNT_SKIN
	else if (ch->GetWear(WEAR_COSTUME_MOUNT))
#else
	if (ch->GetWear(WEAR_COSTUME_MOUNT))
#endif
	{
		CMountSystem* mountSystem = ch->GetMountSystem();
		LPITEM mount = ch->GetWear(WEAR_COSTUME_MOUNT);
		DWORD mobVnum = 0;

		if (!mountSystem && !mount)
			return;

		if (mount->GetValue(1) != 0)
			mobVnum = mount->GetValue(1);

		if (ch->GetMountVnum())
		{
			if (mountSystem->CountSummoned() == 0)
				mountSystem->Unmount(mobVnum);
		}
		else
		{
			if (mountSystem->CountSummoned() == 1)
			{
				mountSystem->Mount(mobVnum, mount);
			}
		}

		return;
	}
#endif

	if (ch->IsHorseRiding())
	{
		ch->StopRiding();
		return;
	}

	if (ch->GetHorse() != NULL)
	{
		ch->StartRiding();
		return;
	}

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (NULL == item)
			continue;

		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_MOUNT) {
			ch->UseItem(TItemPos(INVENTORY, i));
			return;
		}
	}

	ch->NewChatPacket(STRING_D2);
}

#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
ACMD(do_change_channel)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->NewChatPacket(STRING_D48);
		return;
	}

	short channel;
	str_to_number(channel, arg1);

	if (channel < 0 || channel > 6)
	{
		ch->NewChatPacket(STRING_D49);
		return;
	}

	if (channel == g_bChannel)
	{
		ch->NewChatPacket(STRING_D50, "%d", g_bChannel);
		return;
	}

	if (g_bChannel == 99)
	{
		ch->NewChatPacket(STRING_D51);
		return;
	}

	if (ch->GetDungeon())
	{
		ch->NewChatPacket(STRING_D52);
		return;
	}

	if (ch->IsObserverMode())
	{
		ch->NewChatPacket(STRING_D53);
		return;
	}

	if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
	{
		ch->NewChatPacket(STRING_D54);
		return;
	}

	TPacketChangeChannel p;
	p.channel = channel;
	p.lMapIndex = ch->GetMapIndex();

	db_clientdesc->DBPacket(HEADER_GD_FIND_CHANNEL, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}
#endif

#ifdef ENABLE_AFFECT_BUFF_REMOVE
ACMD(do_remove_buff)
{
	if (!ch)
		return;
	
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	int affect = 0;
	str_to_number(affect, arg1);

	if (affect == 66)
		return;

	CAffect* pAffect = ch->FindAffect(affect);

	if (pAffect)
	{
		if ((ch->m_potionUseTime + 10) > get_global_time())
		{
			ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d",(ch->m_potionUseTime + 10) - get_global_time());
			return;
		}
		if (affect == AFFECT_AUTO_HP_RECOVERY || affect == AFFECT_AUTO_SP_RECOVERY)
		{
			if (pAffect->lSPCost == 0)
			{
				pAffect->lSPCost = 1;
				ch->NewChatPacket(STRING_D55);
			}
			else
			{
				pAffect->lSPCost = 0;
				ch->NewChatPacket(STRING_D56);
			}
			ch->m_potionUseTime = get_global_time();
		}
#ifdef ENABLE_AUTO_PICK_UP
		else if (affect == AFFECT_AUTO_PICK_UP)
		{
			if (pAffect->lSPCost == 0)
			{
				pAffect->lSPCost = 1;
				ch->NewChatPacket(STRING_D182);
				ch->SetAutoPickUp(true);
			}
			else
			{
				pAffect->lSPCost = 0;
				ch->NewChatPacket(STRING_D183);
				ch->SetAutoPickUp(false);
			}
			ch->m_potionUseTime = get_global_time();
		}
#endif
		else
		{
			ch->RemoveAffect(affect);
		}
	}
		
}
#endif

#ifdef ENABLE_SPLIT_ITEMS_FAST
ACMD(do_split_selected_item)
{
	if (!ch) 
		return;

	if (quest::CQuestManager::instance().GetEventFlag("split_item") == 1)
	{
		ch->NewChatPacket(STRING_D57);
		return;
	}

	const char* line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->NewChatPacket(CMD_TALK_1);
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT count = 0;
#endif
	WORD cell = 0;
	WORD destCell = 0;

	str_to_number(cell, arg1);
	str_to_number(count, arg2);
	str_to_number(destCell, arg3);

	LPITEM item = ch->GetInventoryItem(cell);
	if (item != NULL)
	{
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT itemCount = item->GetCount();
#endif
		while (itemCount > 0)
		{
			if (count > itemCount)
				count = itemCount;

			int iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize());
			if (iEmptyPosition == -1)
				break;

			itemCount -= count;
			ch->MoveItem(TItemPos(INVENTORY, cell), TItemPos(INVENTORY, iEmptyPosition), count
#ifdef ENABLE_HIGHLIGHT_SYSTEM
				, true
#endif
			);
		}
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, true, true);
#endif
	}
}

#ifdef ENABLE_SPECIAL_STORAGE
ACMD(do_split_storage_item)
{
	if (!ch) 
		return;

	if (quest::CQuestManager::instance().GetEventFlag("special_split_item") == 1)
	{
		ch->NewChatPacket(STRING_D57);
		return;
	}

	char arg1[256], arg2[256], arg3[256], arg4[256];
	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
	{
		ch->NewChatPacket(CMD_TALK_1);
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT count = 0;
#endif
	WORD cell = 0;
	WORD destCell = 0;
	BYTE windowtype = 0;

	str_to_number(cell, arg1);
	str_to_number(count, arg2);
	str_to_number(destCell, arg3);
	str_to_number(windowtype, arg4);

	LPITEM item = ch->WindowTypeGetItem(cell, windowtype);
	if (item != NULL)
	{
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		uint16_t itemCount = item->GetCount();
#endif
		while (itemCount > 0)
		{
			if (count > itemCount)
				count = itemCount;

			int iEmptyPosition = ch->GetEmptyInventoryFromIndex(destCell, item->GetSize(), windowtype);
			if (iEmptyPosition == -1)
				break;

			itemCount -= count;
			ch->MoveItem(TItemPos(windowtype, cell), TItemPos(windowtype, iEmptyPosition), count
#ifdef ENABLE_HIGHLIGHT_SYSTEM
				, true
#endif
			);
		}
#ifdef ENABLE_REFRESH_CONTROL
		ch->RefreshControl(REFRESH_INVENTORY, true, true);
#endif
	}
}
#endif

#ifdef ENABLE_SORT_INVENTORIES
ACMD(do_sort_items)
{
	if (!ch)
		return;

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	if ((ch->m_sortLastUsed + 30) > get_global_time())
	{
		ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_sortLastUsed + 30) - get_global_time());
		return;
	}

	ch->m_sortLastUsed = get_global_time();
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);

		if (!item)
			continue;

		if (item->isLocked())
			continue;

		if (item->GetCount() == g_bItemCountLimit)
			continue;

		if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			for (int j = i; j < INVENTORY_MAX_NUM; ++j)
			{
				LPITEM item2 = ch->GetInventoryItem(j);

				if (!item2)
					continue;

				if (item2->isLocked())
					continue;

				if (item2->GetVnum() == item->GetVnum())
				{
					bool bStopSockets = false;

					for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
					{
						if (item2->GetSocket(k) != item->GetSocket(k))
						{
							bStopSockets = true;
							break;
						}
					}

					if (bStopSockets)
						continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#else
					BYTE bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#endif

					item->SetCount(item->GetCount() + bAddCount);
					item2->SetCount(item2->GetCount() - bAddCount);
					continue;
				}
			}
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, true,true);
#endif
}
ACMD(do_sort_special_storage)
{

	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;


	if ((ch->m_sortLastUsed + 30) > get_global_time())
	{
		ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_sortLastUsed + 30) - get_global_time());
		return;
	}

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	BYTE windowtype = 0;
	str_to_number(windowtype, arg1);
	ch->m_sortLastUsed = get_global_time();
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		LPITEM item = ch->WindowTypeGetItem(i,windowtype);

		if (!item)
			continue;

		if (item->isLocked())
			continue;

		if (item->GetCount() == g_bItemCountLimit)
			continue;

		if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			for (int j = i; j < SPECIAL_INVENTORY_MAX_NUM; ++j)
			{
				LPITEM item2 = ch->WindowTypeGetItem(j, windowtype);

				if (!item2)
					continue;

				if (item2->isLocked())
					continue;

				if (item2->GetVnum() == item->GetVnum())
				{
					bool bStopSockets = false;

					for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
					{
						if (item2->GetSocket(k) != item->GetSocket(k))
						{
							bStopSockets = true;
							break;
						}
					}

					if (bStopSockets)
						continue;

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
					MAX_COUNT bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#else
					BYTE bAddCount = MIN(g_bItemCountLimit - item->GetCount(), item2->GetCount());
#endif

					item->SetCount(item->GetCount() + bAddCount);
					item2->SetCount(item2->GetCount() - bAddCount);

					continue;
				}
			}
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	ch->RefreshControl(REFRESH_INVENTORY, true, true);
#endif
}
#endif

#endif

#ifdef ENABLE_GLOBAL_RANKING
ACMD(do_request_ranking_list)
{
	if (!ch)
		return;
	
	CStatisticsRanking::Instance().RequestRankingList(ch);
}
#endif

#ifdef ENABLE_CHAR_SETTINGS
ACMD(do_SetCharSettings)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;
	BYTE settingtype = 0;
	str_to_number(settingtype, arg1);

	switch (settingtype)
	{
		case SETTINGS_ANTI_EXP:
		{
			if (ch->GetCharSettings().antiexp)
			{
				ch->SetSettingsAntiExp(false);
			}
			else
			{
				ch->SetSettingsAntiExp(true);
			}
			break;
		}
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		case SETTINGS_HIDE_COSTUME_HAIR:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_HAIR, ch->IsCostumeHairHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_BODY:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_BODY, ch->IsCostumeBodyHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_WEAPON:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_WEAPON, ch->IsCostumeWeaponHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_ACCE:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_ACCE, ch->IsCostumeAcceHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_AURA:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_AURA, ch->IsCostumeAuraHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
		case SETTINGS_HIDE_COSTUME_CROWN:
		{
			ch->SetHideCostumeState(SETTINGS_HIDE_COSTUME_CROWN, ch->IsCrownHidden() ? 0 : 1);
			ch->UpdatePacket();
			break;
		}
#endif
#ifdef ENABLE_STOP_CHAT
		case SETTINGS_STOP_CHAT:
		{
			if ((ch->m_stopChatCooldown + 30) > get_global_time())
			{
				ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_stopChatCooldown + 30) - get_global_time());
				return;
			}

			ch->SetStopChatState(ch->GetStopChatState() ? 0 : 1);
			ch->NewChatPacket(STRING_D58);

			ch->m_stopChatCooldown = get_global_time();
			break;
		}
#endif
		default: return;
	}
	ch->SendCharSettingsPacket(settingtype);
}
#endif

#ifdef ENABLE_FISHING_RENEWAL
ACMD(do_fishingrenewal)
{
	if (!ch)
		return;

	if (ch->IsDead() || ch->IsRiding() || ch->IsStun())
		return;

	LPITEM rod = ch->GetWear(WEAR_WEAPON);

	if (!rod)
		return;

	if (!(rod && rod->GetType() == ITEM_ROD))
	{
		ch->m_pkFishingEvent = nullptr;
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		switch (LOWER(*arg1))
		{

			case 'f': // fail
			{
				if ((ch->m_fishEventTime + 5) > get_global_time()) //lightwork campfire spam fixed
					return;
				ch->m_fishEventTime = get_global_time(); //lightwork campfire spam fixed

				fishing::FishingFail(ch);
				rod->SetSocket(2, 0);
				break;
			}

			case 'a': // add
			{
				if (ch->m_bFishGameFishCounter < 3)
					ch->m_bFishGameFishCounter += 1;
				else
					ch->fishing_take();
		
				break;
			}
			case 's': // success
			{
				if ((ch->m_fishEventTime + 5) > get_global_time())
					return;

				ch->m_fishEventTime = get_global_time();

				if (ch->m_bFishGameFishCounter == 3)
				{
					ch->fishing_take();
					ch->m_bFishGameFishCounter = 0;
				}
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("An error has occured, report this error to authoroties. Fish #s parameter"));
				break;
			}
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("An error has occured, report this error to authoroties. Fish #not arg"));
	}

}
#endif

#ifdef ENABLE_DISTANCE_NPC
#include "shop.h"
#include "shop_manager.h"
ACMD(do_distance_npc)
{
	if (quest::CQuestManager::instance().GetEventFlag("distance_shop_disable") == 1)
	{
		ch->NewChatPacket(STRING_D57);
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1) return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (!ch->IsPC()) return;

	if (ch->WindowOpenCheck() || ch->ActivateCheck())
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	if (ch->GetDungeon())
	{
		ch->NewChatPacket(STRING_D45);
		return;
	}

	LPSHOP shop = CShopManager::instance().Get(vnum);
	if (!shop) return;

	ch->SetShopOwner(ch);
	shop->AddGuest(ch, 0, false);
}
#endif

#ifdef ENABLE_FARM_BLOCK
ACMD(do_set_farm_block)
{
	if (!ch)
		return;

	const int pulse = thecore_pulse();
	const int limit_time = 30 * passes_per_sec;

	if ((pulse - ch->GetFarmBlockSetTime()) < limit_time)
	{
		ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", 30);
		return;
	}
	CHwidManager::instance().SetFarmBlockGD(ch);
}
#endif

#ifdef ENABLE_PASSIVE_SKILLS
ACMD(do_passiveread)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (!ch->CanWarp())
	{
		ch->NewChatPacket(STRING_D61);
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (!*arg1 || !*arg2)
		return;

	int skillIdx = atoi(arg2);
	bool allRead = atoi(arg1);

	ch->PassiveSkillUpgrade(skillIdx, allRead);
}
#endif

#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
EVENTINFO(SoulStoneEvent)
{
	LPCHARACTER	ch;
	long skillindex;
	bool sage;
	bool align;

	SoulStoneEvent()
		: ch(nullptr)
		, skillindex(0)
		, sage(false)
		, align(false)
	{
	}
};

EVENTFUNC(soulStone_Event)
{
	SoulStoneEvent* info = dynamic_cast<SoulStoneEvent*>(event->info);

	if (info == nullptr)
	{
		sys_err("soulStone_Event> <Factor> nullptr pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (!ch)
		return 0;

	bool sage = info->sage;
	bool alignment = info->align;
	long skillindex = info->skillindex;


	int soulStoneVnum = sage ? 24000 : 50513;
	int ruhVnum = sage ? 24013 : 71001;
	int munzeviVnum = sage ? 24014 : 71094;

	if (!ch->CountSpecifyItemText(soulStoneVnum, 1))
		return 0;
	if (!ch->CountSpecifyItemText(ruhVnum, 1))
		return 0;
	if (!ch->CountSpecifyItemText(munzeviVnum, 1))
		return 0;

	int zenCount = 50;
	int alignmentDownPoint = 0;
	if (!alignment)
	{
		int alignmentPoint = ch->GetAlignment() / 10;
		zenCount = MIN(9, ch->GetSkillLevel(skillindex) - (sage ? 40 : 30));

		if (alignmentPoint > 0)
		{
			if (alignmentPoint > zenCount * 500)
			{
				alignmentDownPoint = (zenCount * 500) * (-10);
				zenCount = 0;
			}
			else
			{
				alignmentDownPoint = alignmentPoint * (-10);
				zenCount -= alignmentPoint / 500;
			}
		}
	}

	if (zenCount > 0 && !ch->CountSpecifyItemText(70102, zenCount))
		return 0;

	if (sage)
	{
		if (!ch->LearnSageMasterSkill(skillindex))
			return 0;
	}
	else
	{
		if (!ch->LearnGrandMasterSkill(skillindex))
			return 0;
	}

	ch->RemoveSpecifyItem(munzeviVnum, 1);
	ch->RemoveSpecifyItem(ruhVnum, 1);
	ch->RemoveSpecifyItem(soulStoneVnum, 1);
	ch->RemoveSpecifyItem(70102, zenCount);

	if (alignmentDownPoint != 0)
		ch->UpdateAlignment(alignmentDownPoint);

#ifdef ENABLE_BUFFI_SYSTEM
	if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
	{
		if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
		{
			ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
		}
	}
#endif
	return 1;
}

ACMD(do_ruhoku)
{
	char arg1[256], arg2[256], arg3[256], arg4[256];

	four_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !*arg4)
		return;

	bool allRead = false;
	bool sage = false;
	bool alignment = false;
	long skillindex = 0;

	str_to_number(allRead, arg1);
	str_to_number(sage, arg2);
	str_to_number(skillindex, arg3);
	str_to_number(alignment, arg4);

	CSkillProto* pkSk = CSkillManager::instance().Get(skillindex);
	if (!pkSk)
		return;

	if (!ch)
		return;

	if (!ch->CanWarp())
	{
		ch->NewChatPacket(STRING_D61);
		return;
	}

	if (ch->GetSkillGroup() == 0)
	{
		ch->NewChatPacket(STRING_D79);
		return;
	}

	if (ch->GetSkillLevel(skillindex) >= (sage ? 50 : 40))
	{
		ch->NewChatPacket(STRING_D78);
		return;
	}

	if (allRead == 1)
	{
		int soulStoneVnum = sage ? 24000 : 50513;

		int ruhVnum = sage ? 24013 : 71001;
		int munzeviVnum = sage ? 24014 : 71094;

		if (!ch->CountSpecifyItemText(soulStoneVnum, 1))
			return;
		if (!ch->CountSpecifyItemText(ruhVnum, 1))
			return;
		if (!ch->CountSpecifyItemText(munzeviVnum, 1))
			return;

		int zenCount = 50;
		int alignmentDownPoint = 0;
		if (!alignment)
		{
			int alignmentPoint = ch->GetAlignment() / 10;
			zenCount = MIN(9, ch->GetSkillLevel(skillindex) - (sage ? 40 : 30));

			if (alignmentPoint > 0)
			{
				if (alignmentPoint > zenCount * 500)
				{
					alignmentDownPoint = (zenCount * 500) * (-10);
					zenCount = 0;
				}
				else
				{
					alignmentDownPoint = alignmentPoint * (-10);
					zenCount -= alignmentPoint / 500;
				}
			}
		}

		if (zenCount > 0 && !ch->CountSpecifyItemText(70102, zenCount))
			return;
			
		if (sage)
		{
			if (!ch->LearnSageMasterSkill(skillindex))
				return;
		}
		else
		{
			if (!ch->LearnGrandMasterSkill(skillindex))
				return;
		}

		ch->RemoveSpecifyItem(ruhVnum, 1);
		ch->RemoveSpecifyItem(munzeviVnum, 1);
		ch->RemoveSpecifyItem(soulStoneVnum, 1);
		ch->RemoveSpecifyItem(70102, zenCount);

		if (alignmentDownPoint != 0)
			ch->UpdateAlignment(alignmentDownPoint);
#ifdef ENABLE_BUFFI_SYSTEM
		if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
		{
			if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
			{
				ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
			}
		}
#endif
	}

	else
	{
		LPEVENT fastEvent = ch->GetFastReadEvent();
		if (fastEvent)
			event_cancel(&fastEvent);

		SoulStoneEvent* info = AllocEventInfo<SoulStoneEvent>();
		info->ch = ch;
		info->skillindex = skillindex;
		info->sage = sage;
		info->align = alignment;
		ch->SetFastReadEvent(event_create(soulStone_Event, info, PASSES_PER_SEC(1)));
	}
	return;
}
#endif


#ifdef ENABLE_EVENT_MANAGER
ACMD(do_event_manager)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);

	if (vecArgs.size() < 2)
	{
		return;
	}
	else if (vecArgs[1] == "info")
	{
		CHARACTER_MANAGER::Instance().SendDataPlayer(ch);
	}
	else if (vecArgs[1] == "remove")
	{
		if (!ch->IsGM())
			return;

		if (vecArgs.size() < 3)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "put the event index!!");
			return; 
		}

		BYTE removeIndex = 0;
		str_to_number(removeIndex, vecArgs[2].c_str());

		if(CHARACTER_MANAGER::Instance().CloseEventManuel(removeIndex))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "successfuly remove!");
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "dont has any event!");
		}
	}
	else if (vecArgs[1] == "update")
	{
		if (!ch->IsGM())
			return;

		const BYTE subHeader = EVENT_MANAGER_UPDATE;
		db_clientdesc->DBPacket(HEADER_GD_EVENT_MANAGER, 0, &subHeader, sizeof(BYTE));
		ch->ChatPacket(CHAT_TYPE_INFO, "successfully update!");
	}
}
#endif

#ifdef ENABLE_ITEMSHOP
ACMD(do_ishop)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);

	if (vecArgs.size() < 2)
	{
		return;
	}
	else if (vecArgs[1] == "data")
	{
		if (ch->GetProtectTime("itemshop.load") == 1)
			return;
		ch->SetProtectTime("itemshop.load", 1);

		if (vecArgs.size() < 3)
			return;

		int updateTime = 0;
		str_to_number(updateTime, vecArgs[2].c_str());
		CHARACTER_MANAGER::Instance().LoadItemShopData(ch, CHARACTER_MANAGER::Instance().GetItemShopUpdateTime() != updateTime);
	}
	else if (vecArgs[1] == "log")
	{
		if (ch->GetProtectTime("itemshop.log") == 1)
			return;
	
		ch->SetProtectTime("itemshop.log", 1);

		CHARACTER_MANAGER::Instance().LoadItemShopLog(ch);
	}
	else if (vecArgs[1] == "buy")
	{
		if (vecArgs.size() < 4)
			return;

		int itemID = 0;
		str_to_number(itemID, vecArgs[2].c_str());
		int itemCount = 0;
		str_to_number(itemCount, vecArgs[3].c_str());
		CHARACTER_MANAGER::Instance().LoadItemShopBuy(ch, itemID, itemCount);
	}	
	else if (vecArgs[1] == "u_buy")
	{
		if (vecArgs.size() < 4)
			return;

		int itemID = 0;
		str_to_number(itemID, vecArgs[2].c_str());
		int itemCount = 0;
		str_to_number(itemCount, vecArgs[3].c_str());
		CHARACTER_MANAGER::Instance().LoadItemShopBuy(ch, itemID, itemCount, true);
	}
	else if (vecArgs[1] == "wheel")
	{
		if (vecArgs.size() < 3)
		{
			return;
		}
		else if (vecArgs[2] == "start")
		{
			if (vecArgs.size() < 4)
				return;

			BYTE ticketType = 0;

			str_to_number(ticketType, vecArgs[3].c_str());
			if (ticketType > 1)
				return;

			else if (ch->GetProtectTime("WheelWorking") != 0)
			{
				ch->NewChatPacket(STRING_D67);
				return;
			}
			if (ticketType == 0)
			{
				if (ch->CountSpecifyItem(wheelSpinTicketVnum) - wheelSpinWithTicketPrice < 0)
				{
					ch->NewChatPacket(STRING_D68);
					return;
				}
				ch->RemoveSpecifyItem(wheelSpinTicketVnum, wheelSpinWithTicketPrice);
			}
			else if (ticketType == 1)
			{
				long long dragonCoin = ch->GetDragonCoin();

				if(dragonCoin - wheelDCPrice < 0)
				{
					ch->NewChatPacket(STRING_D69);
					return;
				}
		
				ch->SetDragonCoin(dragonCoin- wheelDCPrice);

				ch->ChatPacket(CHAT_TYPE_COMMAND, "SetDragonCoin %lld", dragonCoin - wheelDCPrice);

				BYTE subIndex = ITEMSHOP_LOG_ADD;

				DWORD accountID = ch->GetDesc()->GetAccountTable().id;
				char playerName[CHARACTER_NAME_MAX_LEN+1];
				char ipAdress[16];

				strlcpy(playerName,ch->GetName(),sizeof(playerName));
				strlcpy(ipAdress,ch->GetDesc()->GetHostName(),sizeof(ipAdress));
				db_clientdesc->DBPacketHeader(HEADER_GD_ITEMSHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE)+sizeof(DWORD)+sizeof(playerName)+sizeof(ipAdress));
				db_clientdesc->Packet(&subIndex, sizeof(BYTE));
				db_clientdesc->Packet(&accountID, sizeof(DWORD));
				db_clientdesc->Packet(&playerName, sizeof(playerName));
				db_clientdesc->Packet(&ipAdress, sizeof(ipAdress));

				if (ch->GetProtectTime("itemshop.log") == 1)
				{
					char       timeText[21];
					time_t     now = time(0);
					struct tm  tstruct = *localtime(&now);
					strftime(timeText, sizeof(timeText), "%Y-%m-%d %X", &tstruct);
					ch->ChatPacket(CHAT_TYPE_COMMAND, "ItemShopAppendLog %s %d %s %s 1 1 %u", timeText, time(0), playerName, ipAdress, wheelDCPrice);
				}
			}
			// Important items
			std::vector<std::pair<long, long>> m_important_item = {
				 {39067,1},
				 {36020,1},
				 {36024,1},
				 {36028,1},
				 {36032,1},
				 {71992,1},
				 {24050,1},
				 {24054,1},
				 {24058,1},
				 {37000,1},
				 {37008,1},
				 {37016,1},
				 {25041,1},
				 {25042,1},
			};
			// normal items
			std::map<std::pair<long, long>, int> m_normal_item = {
				 {{72351,1},80},
				 {{24601,1},80},
				 {{71035,1},80},
				 {{30629,1},80},
				 {{50513,1},80},
				 {{25040,3},80},
				 {{80019,3},80},
				 {{50513,1},40},
				 {{71084,100},80},
				 {{200210,2},80},
				 {{200011,3},80},
				 {{70005,1},50},
				 {{70043,1},50},
				 {{970005,1},20},
				 {{970043,1},20},
				 {{200014,5},80},
				 {{100300,1},60},
				 {{100400,1},50},
				 {{100500,1},40},
				 {{72346,1},20},
				 {{24602,1},20},
				 {{24155,1},1},
				 {{24156,1},1},
				 {{24157,1},1},
				 {{24158,1},1},
				 {{24159,1},1},
				 {{24160,1},1},
				 {{24161,1},1},
				 {{24162,1},1},
				 {{24163,1},1},
				 {{24164,1},1},
				 {{24175,1},1},
				 {{24176,1},1},
				 {{24177,1},1},
				 {{24178,1},1},
				 {{24179,1},1},
				 {{24180,1},1},
				 {{24181,1},1},
				 {{24182,1},1},
				 {{24183,1},1},
				 {{24184,1},1},
				 {{24195,1},1},
				 {{24196,1},1},
				 {{24197,1},1},
				 {{24198,1},1},
				 {{24199,1},1},
				 {{24225,1},1},
				 {{24226,1},1},
				 {{24227,1},1},
				 {{24228,1},1},
				 {{24229,1},1},
			};

			std::vector<std::pair<long, long>> m_send_items;
			if (m_important_item.size())
			{
				int random = number(0,m_important_item.size()-1);
				m_send_items.emplace_back(m_important_item[random].first, m_important_item[random].second);
			}

			while (true)
			{
				for (auto it = m_normal_item.begin(); it != m_normal_item.end(); ++it)
				{
					int randomEx = number(0, 4);
					if (randomEx == 4)
					{
						int random = number(0, 100);
						if (it->second >= random)
						{
							auto itFind = std::find(m_send_items.begin(), m_send_items.end(), it->first);
							if (itFind == m_send_items.end())
							{
								m_send_items.emplace_back(it->first.first, it->first.second);
								if (m_send_items.size() >= 10)
									break;
							}
						}
					}
				}
				if (m_send_items.size() >= 10)
					break;
			}

			std::string cmd_wheel = "";

			if (m_send_items.size())
			{
				for (auto it = m_send_items.begin(); it != m_send_items.end(); ++it)
				{
					cmd_wheel += std::to_string(it->first);
					cmd_wheel += "|";
					cmd_wheel += std::to_string(it->second);
					cmd_wheel += "#";
				}
			}

			int luckyWheel = number(0, 9);
			if (luckyWheel == 0)
				if (number(0, 1) == 0)
					luckyWheel = number(0, 9);

			ch->SetProtectTime("WheelLuckyIndex", luckyWheel);
			ch->SetProtectTime("WheelLuckyItemVnum", m_send_items[luckyWheel].first);
			ch->SetProtectTime("WheelLuckyItemCount", m_send_items[luckyWheel].second);

			ch->SetProtectTime("WheelWorking", 1);

			ch->ChatPacket(CHAT_TYPE_COMMAND, "SetWheelItemData %s", cmd_wheel.c_str());
			ch->ChatPacket(CHAT_TYPE_COMMAND, "OnSetWhell %d", luckyWheel);
		}
		else if (vecArgs[2] == "done")
		{
			if (ch->GetProtectTime("WheelWorking") == 0)
				return;

			ch->AutoGiveItem(ch->GetProtectTime("WheelLuckyItemVnum"), ch->GetProtectTime("WheelLuckyItemCount"));
			ch->ChatPacket(CHAT_TYPE_COMMAND, "GetWheelGiftData %d %d", ch->GetProtectTime("WheelLuckyItemVnum"), ch->GetProtectTime("WheelLuckyItemCount"));
			ch->SetProtectTime("WheelLuckyIndex", 0);
			ch->SetProtectTime("WheelLuckyItemVnum", 0);
			ch->SetProtectTime("WheelLuckyItemCount", 0);
			ch->SetProtectTime("WheelWorking", 0);
		}

	}
}
#endif

#ifdef ENABLE_NAMING_SCROLL
ACMD(do_namingscroll)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (!ch->CanWarp())
	{
		ch->NewChatPacket(STRING_D61);
		return;
	}

	const char* line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3) // arg 1 = pos in inv, arg 2 = scrolltype, arg3 = name
		return;

	BYTE scrollType = 0;

	int bCell = 0;
	str_to_number(bCell, arg1); // for checking the item in the inventory

	LPITEM namingScroll = ch->GetInventoryItem(bCell);

	if (!namingScroll)
		return;

	str_to_number(scrollType, arg2);

	for (int i = 0; i < strlen(arg3); ++i)
	{
		if (arg3[i] == '%' ||
			arg3[i] == '/' ||
			arg3[i] == '>' ||
			arg3[i] == '|' ||
			arg3[i] == ';' ||
			arg3[i] == ':' ||
			arg3[i] == '}' ||
			arg3[i] == '{' ||
			arg3[i] == '[' ||
			arg3[i] == ']' ||
			arg3[i] == '#' ||
			arg3[i] == '@' ||
			arg3[i] == '^' ||
			arg3[i] == '&' ||
			arg3[i] == '\\')
		{
			ch->NewChatPacket(STRING_D70);
			return;
		}
	}

	if (scrollType == MOUNT_NAME_NUM+1)
	{
		if (ch->IsRiding())
		{
			ch->NewChatPacket(STRING_D71);
			return;
		}

		if (namingScroll->GetVnum() != 24050 && namingScroll->GetVnum() != 24051 && namingScroll->GetVnum() != 24052 && namingScroll->GetVnum() != 24053 && namingScroll->GetVnum() != 64000)
		{
			ch->NewChatPacket(STRING_D60);
			return;
		}

		CAffect* mountAffect = ch->FindAffect(AFFECT_NAMING_SCROLL_MOUNT);
		if (mountAffect)
		{
			if (mountAffect->lApplyValue == namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE))
			{
				ch->NewChatPacket(STRING_D72);
				return;
			}
			ch->RemoveAffect(AFFECT_NAMING_SCROLL_MOUNT);
		}

		ch->AddAffect(AFFECT_NAMING_SCROLL_MOUNT, 0, namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE),
			0, namingScroll->GetValue(NAMING_SCROLL_TIME_VALUE), 0, false);

		LPITEM wearMount = ch->GetWear(WEAR_COSTUME_MOUNT);
		if (wearMount)
		{
			if (ch->GetWear(WEAR_MOUNT_SKIN))
				wearMount = ch->GetWear(WEAR_MOUNT_SKIN);

			ch->MountUnsummon(wearMount);
		}

		char szName[CHARACTER_NAME_MAX_LEN + 1];
		DBManager::instance().EscapeString(szName, sizeof(szName), arg3, strlen(arg3));

		DBManager::instance().DirectQuery("UPDATE player SET naming_scroll_mount = '%s' WHERE id = %u", szName, ch->GetPlayerID()); // setting name for playerid cuz we want only one player have this shit
		ch->NewChatPacket(STRING_D73);
		ITEM_MANAGER::Instance().RemoveItem(namingScroll);
		ch->NamingScrollNameClear(MOUNT_NAME_NUM);
		ch->MountSummon(wearMount);
		ch->ComputePoints();
	}
	else if (scrollType == PET_NAME_NUM+1)
	{

		if (namingScroll->GetVnum() != 24054 && namingScroll->GetVnum() != 24055 && namingScroll->GetVnum() != 24056 && namingScroll->GetVnum() != 24057 && namingScroll->GetVnum() != 64001)
		{
			ch->NewChatPacket(STRING_D60);
			return;
		}

		CAffect* mountAffect = ch->FindAffect(AFFECT_NAMING_SCROLL_PET);
		if (mountAffect)
		{
			if (mountAffect->lApplyValue == namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE))
			{
				ch->NewChatPacket(STRING_D72);
				return;
			}
			ch->RemoveAffect(AFFECT_NAMING_SCROLL_PET);
		}

		ch->AddAffect(AFFECT_NAMING_SCROLL_PET, 0, namingScroll->GetValue(NAMING_SCROLL_BONUS_RATE_VALUE),
			0, namingScroll->GetValue(NAMING_SCROLL_TIME_VALUE), 0, false);

		LPITEM wearPet = ch->GetWear(WEAR_PET);
		if (wearPet)
		{
			if (ch->GetWear(WEAR_PET_SKIN))
				wearPet = ch->GetWear(WEAR_PET_SKIN);

			ch->PetUnsummon(wearPet);
		}

		char szName[CHARACTER_NAME_MAX_LEN + 1];
		DBManager::instance().EscapeString(szName, sizeof(szName), arg3, strlen(arg3));
		DBManager::instance().DirectQuery("UPDATE player SET naming_scroll_pet = '%s' WHERE id = %u", szName, ch->GetPlayerID()); // setting name for playerid cuz we want only one player have this shit
		
		ch->NewChatPacket(STRING_D74);
		ITEM_MANAGER::Instance().RemoveItem(namingScroll);
		ch->NamingScrollNameClear(PET_NAME_NUM);
		ch->PetSummon(wearPet);
		ch->ComputePoints();
	}
	else if (scrollType == BUFFI_NAME_NUM + 1)
	{

		if (namingScroll->GetVnum() != 24058 && namingScroll->GetVnum() != 24059 && namingScroll->GetVnum() != 24060 && namingScroll->GetVnum() != 24061 && namingScroll->GetVnum() != 64002)
			return;

		CAffect* mountAffect = ch->FindAffect(AFFECT_NAMING_SCROLL_BUFFI);
		if (mountAffect)
		{
			ch->NewChatPacket(STRING_D72);
			return;
		}

		ch->AddAffect(AFFECT_NAMING_SCROLL_BUFFI, 0, 0,
			0, namingScroll->GetValue(NAMING_SCROLL_TIME_VALUE), 0, false);

		if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
		{
			ch->GetBuffiSystem()->UnSummon();
			ch->GetBuffiSystem()->Summon();
		}

		char szName[CHARACTER_NAME_MAX_LEN + 1];
		DBManager::instance().EscapeString(szName, sizeof(szName), arg3, strlen(arg3));
		DBManager::instance().DirectQuery("UPDATE player SET naming_scroll_buffi = '%s' WHERE id = %u", szName, ch->GetPlayerID()); // setting name for playerid cuz we want only one player have this shit

		ITEM_MANAGER::Instance().RemoveItem(namingScroll);
		ch->NamingScrollNameClear(BUFFI_NAME_NUM);
	}
}
#endif

#ifdef ENABLE_MINING_RENEWAL
ACMD(do_miningrenewal)
{
	if (!ch)
		return;

	if (ch->IsDead() || ch->IsRiding() || ch->IsStun())
		return;

	const LPITEM wearPickaxe{ ch->GetWear(WEAR_WEAPON) };

	if (!wearPickaxe)
		return;

	if (!(wearPickaxe && wearPickaxe->GetType() == ITEM_PICK))
	{
		ch->mining_cancel();
		return;
	}

	char arg1[256]{};
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE event{};
	str_to_number(event, arg1);

	const BYTE neededPoint = mining::GetMiningLodNeededPoint(ch->GetMiningNPCVnum());

	if (neededPoint == 0)
	{
		if (ch->m_pkMiningEvent)
		{
			ch->mining_cancel();
			ch->EndMiningEvent();
			return;
		}
	}

	switch (event)
	{
		case MINING_EVENT_ADD_POINTS: // add
		{
			if (ch->m_pkMiningEvent)
			{
				if (ch->GetMiningTotalPoints() < neededPoint)
				{
					ch->AddMiningTotalPoints(ch->ComputePickaxePoint());
					ch->SendDiggingMotion();
				}
			}
			break;
		}
		case MINING_EVENT_REQUEST_POINTS: // request pickaxe point
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "SendPickaxePoint %d", ch->ComputePickaxePoint());
			break;
		}
		case MINING_EVENT_CANCEL:
		{
			if (ch->m_pkMiningEvent)
			{
				ch->mining_cancel();
				ch->EndMiningEvent();
			}
			break;
		}
	}

}
#endif

#ifdef ENABLE_SPECIAL_STORAGE
ACMD(do_transfer_inv_storage)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD dwPos = 0;

	str_to_number(dwPos, arg1);

	if ((ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner()) || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
		{
			ch->NewChatPacket(YOU_CANNOT_DO_THIS_WHILE_ANOTHER_WINDOW_IS_OPEN);
			return;
		}

	if (dwPos >= 0 && dwPos <= INVENTORY_MAX_NUM)
	{
		LPITEM item = ch->GetInventoryItem(dwPos);

		if (item)
		{
			if (item->IsUpgradeItem() || item->IsBook() || item->IsStone() || item->IsChest())
			{
				BYTE window_type = ch->VnumGetWindowType(item->GetVnum());
				int iEmptyPos = ch->WindowTypeToGetEmpty(window_type, item);

				if (iEmptyPos != -1)
					ch->MoveItem(TItemPos(INVENTORY, item->GetCell()), TItemPos(window_type, iEmptyPos), item->GetCount()
#ifdef ENABLE_HIGHLIGHT_SYSTEM
						, true
#endif
					);
				else
					ch->NewChatPacket(STRING_D75);
			}
		}
	}
}

ACMD(do_transfer_storage_inv)
{
	if (!ch)
		return;

	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	DWORD dwPos = 0;
	BYTE window_type = 0;

	str_to_number(dwPos, arg1);
	str_to_number(window_type, arg2);

	if ((ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner()) || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
		{
			ch->NewChatPacket(YOU_CANNOT_DO_THIS_WHILE_ANOTHER_WINDOW_IS_OPEN);
			return;
		}

	if (dwPos >= 0 && dwPos <= INVENTORY_MAX_NUM && window_type >= UPGRADE_INVENTORY && window_type <= CHEST_INVENTORY)
	{
		LPITEM item = ch->WindowTypeGetItem(dwPos, window_type);

		if (item)
		{
			if (item->IsUpgradeItem() || item->IsStone() || item->IsBook() || item->IsChest())
			{
				int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

				if (iEmptyPos != -1)
					ch->MoveItem(TItemPos(ch->VnumGetWindowType(item->GetVnum()), item->GetCell()), TItemPos(INVENTORY, iEmptyPos), item->GetCount()
#ifdef ENABLE_HIGHLIGHT_SYSTEM
						, true
#endif
					);
				else
					ch->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
			}
		}
	}
}
#endif

#ifdef ENABLE_VOTE4BUFF
ACMD(do_vote_bonus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch)
		return;

	BYTE buffType = 0;
	str_to_number(buffType, arg1);
	if (buffType > 3 && buffType < 1)
		return;

	ch->SelectVoteBuff(buffType);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "Vote4Buff %u", 0);
}
#endif

#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
EVENTINFO(TMainEventInfo2)
{
	LPCHARACTER	kim;
	long skillindexx;

	TMainEventInfo2()
		: kim(nullptr)
		, skillindexx(0)
	{
	}
};

EVENTFUNC(bk_event)
{
	TMainEventInfo2* info = dynamic_cast<TMainEventInfo2*>(event->info);

	if (info == nullptr)
	{
		sys_err("bk_event> <Factor> nullptr pointer");
		return 0;
	}

	long skillindex = 0;

	LPCHARACTER	ch = info->kim;
	skillindex = info->skillindexx;

	if (nullptr == ch || skillindex == 0)
		return 0;

	if (!ch->CanWarp())
	{
		ch->NewChatPacket(STRING_D61);
		return 0;
	}

	int skilllevel = ch->GetSkillLevel(skillindex);
	if (skilllevel >= 30)
	{
		ch->NewChatPacket(STRING_D78);
		return 0;
	}

	int bookVnum = ch->GetBookVnumBySkillIndex(skillindex);
	if (bookVnum == -1)
		return 0;

	if (!ch->CountSpecifyItemText(bookVnum, 1))
		return 0;
	if (!ch->CountSpecifyItemText(71001, 1))
		return 0;
	if (!ch->CountSpecifyItemText(71094, 1))
		return 0;

	if (!ch->LearnSkillByBook(skillindex))
		return 0;

	ch->RemoveSpecifyItem(71001, 1);
	ch->RemoveSpecifyItem(71094, 1);
	ch->RemoveSpecifyItem(bookVnum, 1);
#ifdef ENABLE_BUFFI_SYSTEM
	if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
	{
		if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
		{
			ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
		}
	}
#endif
	return 1;
}

ACMD(do_read_books_fast)
{
	int gelen;
	long skillindex = 0;
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	str_to_number(gelen, arg1);
	str_to_number(skillindex, arg2);

	if (gelen < 0 || skillindex < 0)
		return;

	if (!ch)
		return;

	if (!ch->CanWarp())
	{
		ch->NewChatPacket(STRING_D61);
		return;
	}

	int skillgrup = ch->GetSkillGroup();
	if (skillgrup == 0)
	{
		ch->NewChatPacket(STRING_D79);
		return;
	}

	if (ch->GetSkillLevel(skillindex) >= 30)
	{
		ch->NewChatPacket(STRING_D78);
		return;
	}

	if (gelen == 1) ///tek
	{
		int bookVnum = ch->GetBookVnumBySkillIndex(skillindex);
		if (bookVnum == -1)
			return;
		if (!ch->CountSpecifyItemText(bookVnum, 1))
			return;
		if (!ch->CountSpecifyItemText(71001, 1))
			return;
		if (!ch->CountSpecifyItemText(71094, 1))
			return;

		if (!ch->LearnSkillByBook(skillindex))
			return;

		ch->RemoveSpecifyItem(71001, 1);
		ch->RemoveSpecifyItem(71094, 1);
		ch->RemoveSpecifyItem(bookVnum, 1);
#ifdef ENABLE_BUFFI_SYSTEM
		if (skillindex >= SKILL_BUFFI_1 && skillindex <= SKILL_BUFFI_5)
		{
			if (ch->GetBuffiSystem() && ch->GetBuffiSystem()->IsSummon())
			{
				ch->GetBuffiSystem()->UpdateSkillLevel(skillindex);
			}
		}
#endif
	}

	else if (gelen == 0) ///hepsi
	{
		LPEVENT fastEvent = ch->GetFastReadEvent();
		if (fastEvent)
			event_cancel(&fastEvent);

		TMainEventInfo2* info = AllocEventInfo<TMainEventInfo2>();
		info->kim = ch;
		info->skillindexx = skillindex;
		ch->SetFastReadEvent(event_create(bk_event, info, PASSES_PER_SEC(1)));
	}
	return;
}
#endif

#ifdef ENABLE_CHAT_COLOR_SYSTEM
ACMD(do_set_chat_color)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch)
		return;

	BYTE color = 0;
	str_to_number(color, arg1);
	ch->SetChatColor(color);
}
#endif

#ifdef ENABLE_BOT_CONTROL
ACMD(do_bot_control)
{
	if (!ch)
		return;
#ifdef ENABLE_LMW_PROTECTION
	ch->HackLogAdd(true);
#endif
}

ACMD(do_bot_control_open)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	bool state = false;
	str_to_number(state, arg1);

	ch->SetBotControl(state);
}
#endif

#ifdef ENABLE_DISTANCE_SKILL_SELECT
ACMD(do_skill_select)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch)
		return;

	if (ch->GetSkillGroup() != 0)
		return;

	BYTE skillGroup = 0;
	str_to_number(skillGroup, arg1);

	if (skillGroup == 0)
		return;

	ch->SetSkillGroup(skillGroup);
	ch->ClearSkill();
	ch->RemoveGoodAffect();

	for (const auto& skillTable : skillSelectTable[ch->GetJob()][skillGroup - 1])
	{
		ch->SetSkillLevel(skillTable[0], skillTable[1]);
	}

	ch->Save();
	
	ch->WarpSet(ch->GetX(), ch->GetY());
	ch->Stop();
}
#endif

#ifdef ENABLE_FURKANA_GOTTEN
ACMD(do_drop_calc)
{
	if (!ch)
		return;

	DWORD mobVnum;
	int mobCount = 0;
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	return;

	str_to_number(mobVnum, arg1);
	str_to_number(mobCount, arg2);
	ch->DropCalculator(mobVnum, mobCount);

}
#endif

#ifdef __SKILL_TRAINING__
ACMD(do_skill_training)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 2) { return; }
	if (vecArgs[1] == "do")
	{
		if (vecArgs.size() < 3) { return; }
		BYTE bGroupIdx;
		if (!str_to_number(bGroupIdx, vecArgs[2].c_str()))
			return;
		else if (bGroupIdx != 1 && bGroupIdx != 2 && ch->GetSkillGroup() != 0)
			return;
		ch->SetSkillGroup(bGroupIdx);
		ch->ClearSkill();
		//passive-skills
		ch->SetSkillLevel(SKILL_LEADERSHIP, 50);
		ch->SetSkillLevel(SKILL_COMBO, 2);
		ch->SetSkillLevel(SKILL_MINING, 50);
		ch->SetSkillLevel(SKILL_LANGUAGE1, 50);
		ch->SetSkillLevel(SKILL_LANGUAGE2, 50);
		ch->SetSkillLevel(SKILL_LANGUAGE3, 50);
		ch->SetSkillLevel(SKILL_POLYMORPH, 50);
		//horse
		ch->SetSkillLevel(SKILL_HORSE, 21);
		ch->SetSkillLevel(SKILL_HORSE_SUMMON, 20);
		ch->SetSkillLevel(SKILL_HORSE_WILDATTACK, 20);
		ch->SetSkillLevel(SKILL_HORSE_CHARGE, 20);
		ch->SetSkillLevel(SKILL_HORSE_ESCAPE, 20);
		ch->SetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE, 20);
	}
}
#endif

#if defined(__GAME_OPTION_ESCAPE__)
ACMD(do_escape)
{
	if (ch->IsDead() || ch->IsPolymorphed())
		return;

	if (ch->GetEscapeCooltime() > thecore_pulse() && !ch->IsGM())
		return;

	const BYTE bEmpire = ch->GetEmpire();
	const long lMapIndex = ch->GetMapIndex();
	const PIXEL_POSITION& rkPos = ch->GetXYZ();

	const LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pSectreeMap == nullptr)
	{
		ch->WarpSet(g_start_position[bEmpire][0], g_start_position[bEmpire][1]);
		return;
	}

	const LPSECTREE pSectree = pSectreeMap->Find(rkPos.x, rkPos.y);
	const DWORD dwAttr = pSectree->GetAttribute(rkPos.x, rkPos.y);

	int iEscapeDistance = quest::CQuestManager::instance().GetEventFlag("escape_distance");
	iEscapeDistance = (iEscapeDistance > 0) ? iEscapeDistance : 300;

	int iEscapeCooltime = quest::CQuestManager::instance().GetEventFlag("escape_cooltime");
	iEscapeCooltime = (iEscapeCooltime > 0) ? iEscapeCooltime : 10;

	if (IS_SET(dwAttr, ATTR_BLOCK /*| ATTR_OBJECT*/))
	{
		/*
		* NOTE : If an object doesn't have a blocked area, players can still be blocked if they get inside it.
		* The problem is that bridges are treated as objects too, and we don't want players to use the escape feature through them.
		* The tricky part is figuring out whether a specific object is a bridge or not.
		* In the current state, we are only checking blocked areas.
		*
		* 2021.01.17.Owsap
		*/

		PIXEL_POSITION kNewPos;
		if (SECTREE_MANAGER::instance().GetRandomLocation(lMapIndex, kNewPos, rkPos.x, rkPos.y, iEscapeDistance))
		{
			char szBuf[255 + 1];
			snprintf(szBuf, sizeof(szBuf), "%ld, (%d, %d) -> (%d, %d)",
				lMapIndex, rkPos.x, rkPos.y, kNewPos.x, kNewPos.y);
			//LogManager::instance().CharLog(ch, lMapIndex, "ESCAPE", szBuf);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have successfully freed yourself."));
			ch->Show(lMapIndex, kNewPos.x, kNewPos.y, rkPos.z);
		}
		else
		{
			char szBuf[255 + 1];
			snprintf(szBuf, sizeof(szBuf), "%ld, (%d, %d) -> EMPIRE START POSITION",
				lMapIndex, rkPos.x, rkPos.y);
			//LogManager::instance().CharLog(ch, lMapIndex, "ESCAPE", szBuf);

			ch->WarpSet(g_start_position[bEmpire][0], g_start_position[bEmpire][1]);
		}
	}

	ch->SetEscapeCooltime(thecore_pulse() + PASSES_PER_SEC(10));
}
#endif

#ifdef ENABLE_DAILY_REWARD
ACMD(do_dailyreward_open)
{
	if (!ch)
		return;
	
	if (ch->GetQuestFlag("daily_reward.can_open_dreward") < get_global_time())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "dailyReward_open");
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("daily_reward_not_yet"));
		return;
	}
}

ACMD(do_dailyreward_process)
{
	if (!ch)
		return;
	
	int REWARDS[][2] = { //Here goes rewards. 
		{200221, 1}, // {vnum, count}
		{200222, 1}, // {vnum, count}
	};
	
	if (ch->GetQuestFlag("daily_reward.can_open_dreward") < get_global_time())
	{
		int random = number(0, sizeof(REWARDS)/sizeof(*REWARDS)-1);
		
		int dr_vnum = REWARDS[random][0];
		int dr_vnum_count = REWARDS[random][1];	

		ch->SetQuestFlag("daily_reward.can_open_dreward", get_global_time()+86400);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "dailyReward_set_reward %i %i", dr_vnum, dr_vnum_count);
		ch->AutoGiveItem(dr_vnum, dr_vnum_count);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("daily_reward_not_yet"));
		return;
	}
}
#endif

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
ACMD(do_block_player)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	auto name = ch->GetName();
	if (name == arg1)
		return;

	auto tch = CHARACTER_MANAGER::Instance().FindPC(arg1);
	auto pkCCI = P2P_MANAGER::Instance().Find(arg1);
	auto& rkPlayerBlockMgr = CPlayerBlock::Instance();

	if (tch || pkCCI)
	{
		if (rkPlayerBlockMgr.IsPlayerBlock(name, arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You have already blocked this player.");
			return;
		}

		rkPlayerBlockMgr.BlockPlayer(name, arg1);

		// Go P2P
		TPacketGGPlayerBlock p2p_packet;
		p2p_packet.header = HEADER_GG_PLAYER_BLOCK;
		p2p_packet.type = PLAYER_BLOCK_TYPE_ADD;
		strlcpy(p2p_packet.szBlockingPlayerName, name, sizeof(p2p_packet.szBlockingPlayerName));
		strlcpy(p2p_packet.szBlockedPlayerName, arg1, sizeof(p2p_packet.szBlockedPlayerName));
		P2P_MANAGER::Instance().Send(&p2p_packet, sizeof(p2p_packet));
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ´ÔÀº Á¢¼ÓµÇ ÀÖÁö ¾Ê½À´Ï´Ù."), arg1);
}

ACMD(do_unblock_player)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	auto name = ch->GetName();
	if (name == arg1)
		return;

	auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
	if (!rkPlayerBlockMgr.IsPlayerBlock(name, arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "The player has not already been blocked.");
		return;
	}

	rkPlayerBlockMgr.UnBlockPlayer(name, arg1);

	// Go P2P
	TPacketGGPlayerBlock p2p_packet;
	p2p_packet.header = HEADER_GG_PLAYER_BLOCK;
	p2p_packet.type = PLAYER_BLOCK_TYPE_REMOVE;
	strlcpy(p2p_packet.szBlockingPlayerName, name, sizeof(p2p_packet.szBlockingPlayerName));
	strlcpy(p2p_packet.szBlockedPlayerName, arg1, sizeof(p2p_packet.szBlockedPlayerName));
	P2P_MANAGER::Instance().Send(&p2p_packet, sizeof(p2p_packet));
}
#endif

#ifdef __STONE_SCROLL__
ACMD(do_stone_scroll)
{
	std::vector<std::string> vecArgs;
	split_argument(argument, vecArgs);
	if (vecArgs.size() < 4) { return; }

	WORD scrollPos, itemPos;
	BYTE socketIdx;

	if (!str_to_number(scrollPos, vecArgs[1].c_str()) || !str_to_number(itemPos, vecArgs[2].c_str()) || !str_to_number(socketIdx, vecArgs[3].c_str()))
		return;

	LPITEM scrollItem = ch->GetInventoryItem(scrollPos), item = ch->GetInventoryItem(itemPos);
	if((!scrollItem&&!ch->CountSpecifyItem(25100)) || !item || !((item->GetType() == ITEM_WEAPON && item->GetSubType() != WEAPON_ARROW) || (item->GetType() == ITEM_ARMOR && item->GetSubType() == ARMOR_BODY)) || socketIdx >= 3)
		return;

	if (scrollItem)
	{
		if (scrollItem->GetVnum() != 25100 && !ch->CountSpecifyItem(25100))
			return;
	}
	else
	{
		if (!ch->CountSpecifyItem(25100))
			return;
	}

	const DWORD stoneItemIdx = item->GetSocket(socketIdx);
	const TItemTable* p = ITEM_MANAGER::instance().GetTable(stoneItemIdx);
	if (!p || p->bType != ITEM_METIN)
		return;
	
	if(28960 == stoneItemIdx)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't get back broke stone!");
		return;
	}

	BYTE socketCount = 0;
	for (BYTE i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) >= 1)
			socketCount += 1;
	}

	ch->AutoGiveItem(stoneItemIdx);
	item->SetSocket(socketIdx, 0);

	if (scrollItem)
	{
		if (scrollItem->GetVnum() != 25100)
			ch->RemoveSpecifyItem(25100, 1);
		else
			scrollItem->SetCount(scrollItem->GetCount() - 1);
	}
	else
		ch->RemoveSpecifyItem(25100, 1);
	
	
	const std::vector<long> m_lsockets = { item->GetSocket(0), item->GetSocket(1), item->GetSocket(2)};
	for (BYTE i = 0; i < socketCount; ++i)
		item->SetSocket(i, 1);
	if (socketCount > 1)
	{
		for (BYTE i = 0, j=0; i < socketCount; ++i)
		{
			if (m_lsockets[i] > 0)
				item->SetSocket(j++, m_lsockets[i]);
				
		}
	}

	item->UpdatePacket();
	item->Save();
}
#endif

#ifdef __AUTO_HUNT__
ACMD(do_auto_hunt)
{
	ch->GetAutoHuntCommand(argument);
}
#endif

#ifdef ENABLE_VOICE_CHAT
ACMD(do_voice_chat_config)
{
	char arg[256];
	one_argument(argument, arg, sizeof(arg));
	
	uint8_t type;
	str_to_number(type, arg);
	
	if (!type || type >= VOICE_CHAT_TYPE_MAX_NUM)
		return;
	
	ch->VoiceChatSetConfig(type, !ch->VoiceChatIsBlock(type));
}
#endif