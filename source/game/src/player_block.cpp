/*
 * author				: Kio
 * file_creation_date	: 12.02.2023
 * file_name			: player_block.cpp
 */

#include "stdafx.h"
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
#include "char.h"
#include "char_manager.h"
#include "messenger_manager.h"
#include "buffer_manager.h"
#include "desc.h"
#include "player_block.h"

CPlayerBlock::CPlayerBlock() = default;
CPlayerBlock::~CPlayerBlock() = default;

auto CPlayerBlock::BlockPlayer(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName, bool isP2P) -> void
{
	m_map_PlayerBlock[strBlockingPlayerName].emplace(strBlockedPlayerName);

	// Send Add Packet
	if (!isP2P)
	{
		auto ch = CHARACTER_MANAGER::Instance().FindPC(strBlockingPlayerName.c_str());
		if (ch)
		{
			PlayerBlockPacket(ch, strBlockedPlayerName, PLAYER_BLOCK_TYPE_ADD);

			// Remove from friend list
			MessengerManager::Instance().RemoveFromList(strBlockingPlayerName, strBlockedPlayerName);
			// Remove from friend list

			ch->ChatPacket(CHAT_TYPE_INFO, "You have blocked %s.", strBlockedPlayerName.c_str());
		}

		// Insert Database
		InsertPlayerBlock(strBlockingPlayerName, strBlockedPlayerName);
	}
}

auto CPlayerBlock::UnBlockPlayer(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName, bool isP2P) -> void
{
	m_map_PlayerBlock[strBlockingPlayerName].erase(strBlockedPlayerName);

	// Send Remove Packet
	if (!isP2P)
	{
		auto ch = CHARACTER_MANAGER::Instance().FindPC(strBlockingPlayerName.c_str());
		if (ch)
		{
			PlayerBlockPacket(ch, strBlockedPlayerName, PLAYER_BLOCK_TYPE_REMOVE);
			ch->ChatPacket(CHAT_TYPE_INFO, "You have unblocked %s.", strBlockedPlayerName.c_str());
		}

		// Delete Database
		DeletePlayerBlock(strBlockingPlayerName, strBlockedPlayerName);
	}
}

auto CPlayerBlock::IsPlayerBlock(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName) -> bool
{
	auto it = m_map_PlayerBlock[strBlockingPlayerName].find(strBlockedPlayerName);
	return it != m_map_PlayerBlock[strBlockingPlayerName].end();
}

auto CPlayerBlock::PlayerBlockPacket(LPCHARACTER ch, const std::string &strBlockedPlayerName, BYTE bType) -> void
{
	auto d = ch->GetDesc();
	if (!d)
		return;

	BYTE bSize = strBlockedPlayerName.size();

	TPacketGCPlayerBlock p;
	p.header = HEADER_GC_PLAYER_BLOCK;
	p.type = bType;
	p.size = sizeof(p) + bSize;

	d->BufferedPacket(&p, sizeof(p));
	d->Packet(strBlockedPlayerName.c_str(), bSize);
}

auto CPlayerBlock::WhisperPacket(LPDESC d, const char *szText, const std::string &strBlockedPlayerName) -> void
{
	if (!d)
		return;

	char buf[64];
	int len = snprintf(buf, sizeof(buf), szText);

	TPacketGCWhisper p;
	p.bHeader = HEADER_GC_WHISPER;
	p.bType = WHISPER_TYPE_SYSTEM;
	p.wSize = sizeof(TPacketGCWhisper) + len;
	strlcpy(p.szNameFrom, strBlockedPlayerName.c_str(), sizeof(p.szNameFrom));

	d->BufferedPacket(&p, sizeof(p));
	d->Packet(buf, len);
}

auto CPlayerBlock::LoadPlayerBlockList(LPCHARACTER ch) -> void
{
	const auto &strName = std::string(ch->GetName());
	if (m_map_PlayerBlock[strName].empty())
		return;

	auto d = ch->GetDesc();
	if (!d)
		return;

	TPacketGCPlayerBlock p;
	p.header = HEADER_GC_PLAYER_BLOCK;
	p.type = PLAYER_BLOCK_TYPE_LOAD;
	p.size = sizeof(p);

	TEMP_BUFFER buf;
	TPacketGCLoadPlayerBlock load_player_block_packet;

	for (const auto &it : m_map_PlayerBlock[strName])
	{
		strlcpy(load_player_block_packet.szName, it.c_str(), sizeof(load_player_block_packet.szName));
		buf.write(&load_player_block_packet, sizeof(load_player_block_packet));
	}

	p.size += buf.size();
	d->BufferedPacket(&p, sizeof(p));
	d->Packet(buf.read_peek(), buf.size());
}

auto CPlayerBlock::InsertPlayerBlock(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName) -> void
{
	DBManager::Instance().DirectQuery("INSERT INTO player.player_block_list VALUES ('%s', '%s')", strBlockingPlayerName.c_str(), strBlockedPlayerName.c_str());
}

auto CPlayerBlock::DeletePlayerBlock(const std::string &strBlockingPlayerName, const std::string &strBlockedPlayerName) -> void
{
	DBManager::Instance().DirectQuery("DELETE FROM player.player_block_list WHERE blockingplayername = '%s' AND blockedplayername = '%s'", strBlockingPlayerName.c_str(), strBlockedPlayerName.c_str());
}

auto CPlayerBlock::ChangeName(const std::string &strOldName, const std::string &strNewName) -> void
{
	auto it = m_map_PlayerBlock.find(strOldName);
	if (it != m_map_PlayerBlock.end())
	{
		m_map_PlayerBlock[strNewName] = std::move(it->second);
		m_map_PlayerBlock.erase(it);
	}

	for (auto &it : m_map_PlayerBlock)
	{
		auto it2 = it.second.find(strOldName);
		if (it2 != it.second.end())
		{
			it.second.erase(it2);
			it.second.emplace(strNewName);
		}
	}

	DBManager::Instance().DirectQuery("UPDATE player.player_block_list SET blockingplayername = '%s' WHERE blockingplayername = '%s'", strNewName.c_str(), strOldName.c_str());
	DBManager::Instance().DirectQuery("UPDATE player.player_block_list SET blockedplayername = '%s' WHERE blockedplayername = '%s'", strNewName.c_str(), strOldName.c_str());

	sys_log(0, "PLAYER_BLOCK: ChangeName: %s -> %s", strOldName.c_str(), strNewName.c_str());
}

auto CPlayerBlock::BootPlayerBlockList() -> void
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery("SELECT * FROM player.player_block_list"));
	if (pMsg->Get()->uiNumRows)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			m_map_PlayerBlock[std::string(row[0])].emplace(std::string(row[1]));
	}

	sys_log(0, "BOOT: PlayerBlock: count: %d", pMsg->Get()->uiNumRows);
}
#endif