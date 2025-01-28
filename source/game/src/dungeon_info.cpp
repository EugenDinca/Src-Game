#include "dungeon_info.h"
#ifdef ENABLE_DUNGEON_INFO
#include <fstream>
#include <rapidjson/json.hpp>
#include "locale_service.h"
#include "entity.h"
#include "char.h"
#include "buffer_manager.h"
#include "desc.h"
#include "item_manager.h"
#include "desc_client.h"
#include "locale.hpp"
#include "char_manager.h"
#include "utils.h"
#include "sectree_manager.h"
#include "dungeon.h"
#include "config.h"
#include "p2p.h"

CDungeonInfo::CDungeonInfo()
{
	m_MaxDungeon = 0;
	m_InfoMap.clear();
}

CDungeonInfo::~CDungeonInfo()
{
	m_InfoMap.clear();
}

bool CDungeonInfo::Initialize()
{
	using js = nlohmann::json;
	char file_name[256 + 1];
	snprintf(file_name, sizeof(file_name), "%s/dungeon_info.json", LocaleService_GetBasePath().c_str());
	std::ifstream ifs(file_name);
	if (!ifs.is_open()) { return false; }

	try
	{
		sys_log(0, "---Dungeon Info Read Start------");
		js jf = js::parse(ifs);
		js& arr = jf["dungeon_info"];
		for (const auto& i : arr)
		{
			BYTE dungeonIndex = 0;
			TDungeonInfoTable info = {};
			i.at("dungeon_index").get_to(dungeonIndex);
			i.at("cooldown").get_to(info.cooldown);
			i.at("boss_vnum").get_to(info.bossVnum);
			i.at("ticket_vnum").get_to(info.ticketVnum);
			i.at("min_level").get_to(info.minLevel);
			i.at("max_level").get_to(info.maxLevel);
			i.at("map_index").get_to(info.mapIdx);
			i.at("cord_x").get_to(info.x);
			i.at("cord_y").get_to(info.y);
			
			sys_log(0, "Dungeon Index: %d cooldown: %d boss vnum: %u ticket vnum: %u Level: %d - %d map: %d x %d y %d ",
				dungeonIndex, info.cooldown, info.bossVnum, info.ticketVnum, info.minLevel, info.maxLevel, info.mapIdx, info.x, info.y);

			m_InfoMap.insert(std::make_pair(dungeonIndex, info));
		}
		ifs.close();
		m_MaxDungeon = m_InfoMap.size();
		sys_log(0, "----Dungeon Info Read End------");
	}
	catch (const std::exception& e)
	{
		sys_err("Dungeon Info error : %s - file : %s", e.what(), file_name);
		return false;
	}
	return true;
}

TDungeonInfoTable* CDungeonInfo::GetDungeonInfo(BYTE dungeonIndex)
{
	InfoMap::iterator it = m_InfoMap.find(dungeonIndex);
	if (it != m_InfoMap.end())
		return &it->second;
	return nullptr;
}


/********************CHARACTER CLASS**************************/
void CHARACTER::SendDungeonInfoTime(BYTE idx, bool update)
{
	LPDESC d = GetDesc();
	if (!d || d == nullptr)
		return;

	std::vector<int>v_time;
	BYTE dungeonCount = update ? 1 : CDungeonInfo::instance().GetMaxDungeon();

	TPacketDungeonInfoGC pack;
	pack.header = HEADER_GC_DUNGEON_INFO;
	pack.size = sizeof(TPacketDungeonInfoGC) + sizeof(BYTE) + (sizeof(int) * dungeonCount) + (update ? sizeof(BYTE) : 0);

	if (update)
	{
		pack.subHeader = SUB_HEADER_GC_TIME_UPDATE;
		int time = GetDungeonReJoinTime(idx);
		time < get_global_time() ? time = 0 : time -= get_global_time();
		v_time.push_back(time);
	}
	else
	{
		pack.subHeader = SUB_HEADER_GC_TIME_REQUEST;
		for (int i = 0; i < dungeonCount; i++)
		{
			int time = GetDungeonReJoinTime(i);
			time < get_global_time() ? time = 0 : time -= get_global_time();
			v_time.push_back(time);
		}
	}

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(TPacketDungeonInfoGC));
	buff.write(&dungeonCount, sizeof(BYTE));
	buff.write(&v_time[0], sizeof(int) * dungeonCount);
	if (update)
		buff.write(&idx, sizeof(BYTE));

	d->Packet(buff.read_peek(), buff.size());
}

int CHARACTER::GetDungeonReJoinTime(BYTE idx)
{
	std::string dungeonFlag = "dungeon.time_";
	dungeonFlag += std::to_string(idx);
	return GetQuestFlag(dungeonFlag);
}

void CHARACTER::SetDungeonReJoinTime(BYTE idx, int coolDown)
{
	std::string dungeonFlag = "dungeon.time_";
	dungeonFlag += std::to_string(idx);
	SetQuestFlag(dungeonFlag, (get_global_time() + coolDown));
}

void CHARACTER::DungeonJoinBegin(BYTE idx)
{
	TDungeonInfoTable* info = CDungeonInfo::instance().GetDungeonInfo(idx);
	if (!info)
		return;

	if (!CanWarp())
	{
		NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	if (!info->levelControl(GetLevel()))
	{
		DungeonSendMessage(DungeonInfoError::DUNGEON_ERROR_LEVEL);
		return;
	}

	if (CountSpecifyItem(info->ticketVnum) < 1)
	{
		DungeonSendMessage(DungeonInfoError::DUNGEON_ERROR_TICKET);
		return;
	}

	if (GetDungeonReJoinTime(idx) > get_global_time())
	{
		DungeonSendMessage(DungeonInfoError::DUNGEON_ERROR_QTIME);
		return;
	}

	LPSECTREE_MAP pkMapSectree = SECTREE_MANAGER::instance().GetMap(info->mapIdx);
	if (pkMapSectree)
	{
		LPDUNGEON dungeon = CDungeonManager::instance().Create(info->mapIdx);
		if (!dungeon)
		{
			DungeonSendMessage(DungeonInfoError::DUNGEON_ERROR_UNKNOWN);
			return;
		}
		DungeonJoinEnd(idx, dungeon->GetMapIndex());
	}
	else
	{
		TPGGDungeon p;
		p.header = HEADER_GG_DUNGEON;
		p.subHeader = SUBHEADER_GG_DUNGEON_REQ;
		p.pid = GetPlayerID();
		p.channel = g_bChannel;
		p.mapIdx = info->mapIdx;
		p.dungeonId = idx;
		P2P_MANAGER::instance().Send(&p, sizeof(TPGGDungeon));
	}
}

void CHARACTER::DungeonJoinEnd(BYTE idx, long mapIdx)
{
	TDungeonInfoTable* info = CDungeonInfo::instance().GetDungeonInfo(idx);
	if (!info)
	{
		DungeonSendMessage(DungeonInfoError::DUNGEON_ERROR_UNKNOWN);
		return;
	}

	RemoveSpecifyItem(info->ticketVnum);
	SetDungeonReJoinTime(idx, info->cooldown);
	SendDungeonInfoTime(idx, true);

	WarpSet(info->x, info->y, mapIdx);
}

void CHARACTER::DungeonSendMessage(BYTE messageID)
{
	LPDESC d = GetDesc();

	if (!d)
		return;

	TPacketDungeonInfoGC pack;
	pack.header = HEADER_GC_DUNGEON_INFO;
	pack.size = sizeof(TPacketDungeonInfoGC) + sizeof(BYTE);
	pack.subHeader = SUB_HEADER_GC_DUNGEON_MESSAGE;

	TEMP_BUFFER buf;
	buf.write(&pack, sizeof(TPacketDungeonInfoGC));
	buf.write(&messageID, sizeof(BYTE));

	d->Packet(buf.read_peek(), buf.size());
}
#endif
