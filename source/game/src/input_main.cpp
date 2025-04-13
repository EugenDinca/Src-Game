#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "cmd.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"

#include "banword.h"
#include "unique_item.h"
#include "building.h"
#include "locale_service.h"
#include "gm.h"
#include "spam.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "locale_service.h"
#include "DragonSoul.h"
#include "belt_inventory_helper.h" // @fixme119
#include "../../common/Controls.h"
#include "input.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
#include "biolog_system.h"
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLogManager.h"
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
#include "New_PetSystem.h"
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
#include "battlepass_manager.h"
#endif
#ifdef ENABLE_MINI_GAME_BNW
#include "minigame_bnw.h"
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
#include "minigame_catchking.h"
#endif
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
#include "player_block.h"
#endif

#define ENABLE_CHAT_COLOR_SYSTEM
#define ENABLE_CHAT_SPAMLIMIT
#define ENABLE_WHISPER_CHAT_SPAMLIMIT
#define ENABLE_CHECK_GHOSTMODE
static int __deposit_limit()
{
	return (1000 * 10000);
}

void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->NewChatPacket(STRING_D117);
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;

	long min = (sec / 60);
	sec -= min * 60;

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	if (hour > 0 && min > 0)
		ch->NewChatPacket(STRING_D118,"%d|%d|%d", hour, min, sec);
	else if (hour > 0 && min == 0)
		ch->NewChatPacket(STRING_D119,"%d|%d", hour, sec);
	else if (hour == 0 && min > 0)
		ch->NewChatPacket(STRING_D120,"%d|%d", min, sec);
	else
		ch->NewChatPacket(STRING_D121,"%d", sec);
#else
	char buf[128 + 1];
	if (hour > 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT("Chat disabled for %d hours %d minutes %d seconds."), hour, min, sec);
	else if (hour > 0 && min == 0)
		snprintf(buf, sizeof(buf), LC_TEXT("Chat disabled for %d hours %d seconds."), hour, sec);
	else if (hour == 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT("Chat disabled for %d minutes %d seconds."), min, sec);
	else
		snprintf(buf, sizeof(buf), LC_TEXT("Chat disabled for %d seconds."), sec);

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
#endif
}

EVENTINFO(spam_event_info)
{
	char host[MAX_HOST_LENGTH + 1];

	spam_event_info()
	{
		::memset(host, 0, MAX_HOST_LENGTH + 1);
	}
};

typedef boost::unordered_map<std::string, std::pair<unsigned int, LPEVENT> > spam_score_of_ip_t;
spam_score_of_ip_t spam_score_of_ip;

EVENTFUNC(block_chat_by_ip_event)
{
	spam_event_info* info = dynamic_cast<spam_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("block_chat_by_ip_event> <Factor> Null pointer");
		return 0;
	}

	const char* host = info->host;

	spam_score_of_ip_t::iterator it = spam_score_of_ip.find(host);

	if (it != spam_score_of_ip.end())
	{
		it->second.first = 0;
		it->second.second = NULL;
	}

	return 0;
}

bool SpamBlockCheck(LPCHARACTER ch, const char* const buf, const size_t buflen)
{
	if (ch->GetLevel() < g_iSpamBlockMaxLevel)
	{
		spam_score_of_ip_t::iterator it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());

		if (it == spam_score_of_ip.end())
		{
			spam_score_of_ip.insert(std::make_pair(ch->GetDesc()->GetHostName(), std::make_pair(0, (LPEVENT)NULL)));
			it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());
		}

		if (it->second.second)
		{
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);
			return true;
		}

		unsigned int score;
		const char* word = SpamManager::instance().GetSpamScore(buf, buflen, score);

		it->second.first += score;

		if (word)
			sys_log(0, "SPAM_SCORE: %s text: %s score: %u total: %u word: %s", ch->GetName(), buf, score, it->second.first, word);

		if (it->second.first >= g_uiSpamBlockScore)
		{
			spam_event_info* info = AllocEventInfo<spam_event_info>();
			strlcpy(info->host, ch->GetDesc()->GetHostName(), sizeof(info->host));

			it->second.second = event_create(block_chat_by_ip_event, info, PASSES_PER_SEC(g_uiSpamBlockDuration));
			sys_log(0, "SPAM_IP: %s for %u seconds", info->host, g_uiSpamBlockDuration);

			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);

			return true;
		}
	}

	return false;
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char* src, int maxLen, int& tagLen, std::string& extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char* cur = ++src;

	if (*cur == '|')
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H')
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char* src, int src_len, int& hyperlinks, bool& colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

int ProcessTextTag(LPCHARACTER ch, const char* c_pszText, size_t len)
{
	int hyperlinks;
	bool colored;

	GetTextTagInfo(c_pszText, len, hyperlinks, colored);

	if (colored == true && hyperlinks == 0)
		return 4;

	return 0;
}

int CInputMain::Whisper(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGWhisper* pinfo = reinterpret_cast<const TPacketCGWhisper*>(data);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGWhisper);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

#ifdef ENABLE_WHISPER_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 10)
	{
		ch->GetDesc()->DelayedDisconnect(0);
		return (iExtraLen);
	}
#endif

	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->NewChatPacket(STRING_D117);
		return (iExtraLen);
	}

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->szNameTo);

	if (pkChr == ch)
		return (iExtraLen);

	LPDESC pkDesc = NULL;
	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;
			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		return iExtraLen;
	}

	if (!pkChr)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

		if (pkCCI)
		{
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
			auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
			if (rkPlayerBlockMgr.IsPlayerBlock(ch->GetName(), pinfo->szNameTo))
			{
				rkPlayerBlockMgr.WhisperPacket(ch->GetDesc(), "You cannot send messages to the player you have blocked.", pinfo->szNameTo);
				return iExtraLen;
			}
			else if (rkPlayerBlockMgr.IsPlayerBlock(pinfo->szNameTo, ch->GetName()))
			{
				rkPlayerBlockMgr.WhisperPacket(ch->GetDesc(), "The player has blocked you.", pinfo->szNameTo);
				return iExtraLen;
			}
#endif
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->szNameTo);
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;

			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_NOT_EXIST;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
#ifdef ENABLE_WHISPER_RENEWAL			
			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			if (!(std::string(buf).find("|?whisper_renewal>|") != std::string::npos || std::string(buf).find("|?whisper_renewal<|") != std::string::npos)) {
				ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
				sys_log(0, "WHISPER: no player");
			}
#else
			ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
			sys_log(0, "WHISPER: no player");
#endif
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
			pkDesc->SetRelay("");
			return iExtraLen;
#endif
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_TARGET_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
			pkDesc->SetRelay("");
			return iExtraLen;
#endif
		}
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
		auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
		if (pkChr && rkPlayerBlockMgr.IsPlayerBlock(ch->GetName(), pinfo->szNameTo))
			rkPlayerBlockMgr.WhisperPacket(ch->GetDesc(), "You cannot send messages to the player you have blocked.", pinfo->szNameTo);
		else if (pkChr && rkPlayerBlockMgr.IsPlayerBlock(pinfo->szNameTo, ch->GetName()))
			rkPlayerBlockMgr.WhisperPacket(ch->GetDesc(), "The player has blocked you.", pinfo->szNameTo);
#endif
		else
		{
			BYTE bType = WHISPER_TYPE_NORMAL;

			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			const size_t buflen = strlen(buf);

			if (true == SpamBlockCheck(ch, buf, buflen))
			{
				if (!pkChr)
				{
					CCI* pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

					if (pkCCI)
					{
						pkDesc->SetRelay("");
					}
				}
				return iExtraLen;
			}

			CBanwordManager::instance().ConvertString(buf, buflen);

			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;

			if (buflen > 0)
			{
				TPacketGCWhisper pack;

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + buflen;
				pack.bType = bType;
				strlcpy(pack.szNameFrom, ch->GetName(), sizeof(pack.szNameFrom));

				TEMP_BUFFER tmpbuf;

				tmpbuf.write(&pack, sizeof(pack));
				tmpbuf.write(buf, buflen);

				pkDesc->Packet(tmpbuf.read_peek(), tmpbuf.size());
			}
		}
	}
	if (pkDesc)
		pkDesc->SetRelay("");

	return (iExtraLen);
}

struct RawPacketToCharacterFunc
{
	const void* m_buf;
	int	m_buf_len;

	RawPacketToCharacterFunc(const void* buf, int buf_len) : m_buf(buf), m_buf_len(buf_len)
	{
	}

	void operator () (LPCHARACTER c)
	{
		if (!c->GetDesc())
			return;

		c->GetDesc()->Packet(m_buf, m_buf_len);
	}
};

struct FEmpireChatPacket
{
	packet_chat& p;
	const char* orig_msg;
	int orig_len;

	int iMapIndex;
	int namelen;

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	const char* m_szName;
	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, int iMapIndex, int iNameLen, const char* szName)
		: p(p), orig_msg(chat_msg), orig_len(len), iMapIndex(iMapIndex), namelen(iNameLen), m_szName(szName)
#else
	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, int iMapIndex, int iNameLen)
		: p(p), orig_msg(chat_msg), orig_len(len), iMapIndex(iMapIndex), namelen(iNameLen)
#endif//test
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;
		
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
		auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
		auto name = d->GetCharacter()->GetName();
		if (name != m_szName)
		{
			if (rkPlayerBlockMgr.IsPlayerBlock(name, m_szName) || rkPlayerBlockMgr.IsPlayerBlock(m_szName, name))
				return;
		}
#endif

		d->BufferedPacket(&p, sizeof(packet_chat));
		d->Packet(orig_msg, orig_len);
	}
};

int CInputMain::Chat(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGChat* pinfo = reinterpret_cast<const TPacketCGChat*>(data);

	if (uiBytes < pinfo->size)
		return -1;

	const int iExtraLen = pinfo->size - sizeof(TPacketCGChat);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->size, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN + 3) + 1];
	strlcpy(buf, data + sizeof(TPacketCGChat), MIN(iExtraLen + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

	if (buflen > 1 && *buf == '/')
	{
		interpret_command(ch, buf + 1, buflen - 1);
		return iExtraLen;
	}
#ifdef ENABLE_CHAT_SPAMLIMIT
	if (ch->IncreaseChatCounter() >= 4)
	{
		if (ch->GetChatCounter() == 10)
			ch->GetDesc()->DelayedDisconnect(0);
		return iExtraLen;
	}
#else
	if (ch->IncreaseChatCounter() >= 10)
	{
		if (ch->GetChatCounter() == 10)
		{
			sys_log(0, "CHAT_HACK: %s", ch->GetName());
			ch->GetDesc()->DelayedDisconnect(5);
		}

		return iExtraLen;
	}
#endif

	const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

	if (pAffect != NULL)
	{
		SendBlockChatInfo(ch, pAffect->lDuration);
		return iExtraLen;
	}

	if (true == SpamBlockCheck(ch, buf, buflen))
	{
		return iExtraLen;
	}

	// @fixme133 begin
	CBanwordManager::instance().ConvertString(buf, buflen);
	// @fixme133 end

	char chatbuf[CHAT_MAX_LEN + 1];
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	int len = snprintf(chatbuf, sizeof(chatbuf), "|L%s|l %s : %s", LC_LOCALE(ch->GetLanguage()), ch->GetName(), buf);
#else
	int len = snprintf(chatbuf, sizeof(chatbuf), "%s : %s", ch->GetName(), buf);
#endif

	if (len < 0 || len >= (int)sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	if (pinfo->type == CHAT_TYPE_SHOUT)
	{
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	char const* color;

	if (ch->GetChatColor() == 1)
		color = "0080FF";
	else if (ch->GetChatColor() == 2) 
		color = "FF0000";
	else if (ch->GetChatColor() == 3)
		color = "FFFF00";
	else if (ch->GetChatColor() == 4)
		color = "00FF00";
	else if (ch->GetChatColor() == 5)
		color = "FFA500";
	else if (ch->GetChatColor() == 6)
		color = "40E0D0";
	else if (ch->GetChatColor() == 7)
		color = "000000";
	else if (ch->GetChatColor() == 8)
		color = "A020F0";
	else if (ch->GetChatColor() == 9)
		color = "FFC0CB";
	else
	{
		if (pinfo->type == CHAT_TYPE_SHOUT)
			color = "A7FFD4";
		else
			color = "FFFFFF";
	}

	if (pinfo->bEmoticon == false
#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		&& ch->FindAffect(AFFECT_PREMIUM)
#endif
		)
		len = snprintf(chatbuf, sizeof(chatbuf), "|L%s|l %s |H%s%s|h(#)|h|r - Lv.%d|h|r : |cFF%s%s|r", LC_LOCALE(ch->GetLanguage()), ch->GetName(), "whisper:", ch->GetName(), ch->GetLevel(), color, buf);
	else
		len = snprintf(chatbuf, sizeof(chatbuf), "|L%s|l %s |H%s%s|h(#)|h|r - Lv.%d|h|r : %s", LC_LOCALE(ch->GetLanguage()), ch->GetName(), "whisper:", ch->GetName(), ch->GetLevel(), buf);
#else
#ifdef ENABLE_CHAT_FLAG_AND_PM
		len = snprintf(chatbuf, sizeof(chatbuf), "|L%s|l %s |H%s%s|h(#)|h|r - Lv.%d|h|r : %s", LC_LOCALE(ch->GetDesc()->GetLanguage()), ch->GetName(), "whisper:", ch->GetName(), ch->GetLevel(), buf);
#endif
#endif

		if (ch->GetLevel() < g_iShoutLimitLevel)
		{
			ch->NewChatPacket(STRING_D122, "%d", g_iShoutLimitLevel);
			return (iExtraLen);
		}

		if (thecore_heart->pulse - (int)ch->GetLastShoutPulse() < passes_per_sec * 15)
			return (iExtraLen);

		ch->SetLastShoutPulse(thecore_heart->pulse);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		ch->UpdateExtBattlePassMissionProgress(CHAT_SHOUT, 1, 0);
#endif
		TPacketGGShout p;

		p.bHeader = HEADER_GG_SHOUT;
		p.bEmpire = ch->GetEmpire();
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
		strlcpy(p.szName, ch->GetName(), sizeof(p.szName));
#endif
		strlcpy(p.szText, chatbuf, sizeof(p.szText));

		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
		SendShout(chatbuf, ch->GetEmpire(), ch->GetName());
#else
		SendShout(chatbuf, ch->GetEmpire());
#endif

		return (iExtraLen);
	}

	TPacketGCChat pack_chat;

	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = pinfo->type;
	pack_chat.id = ch->GetVID();

	switch (pinfo->type)
	{
	case CHAT_TYPE_TALKING:
	{
		const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_ref_set.begin(), c_ref_set.end(),
			FEmpireChatPacket(pack_chat,
				chatbuf,
				len,
				ch->GetMapIndex(), strlen(ch->GetName())
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
					, ch->GetName()
#endif
					));
	}
	break;

	case CHAT_TYPE_PARTY:
	{
		if (!ch->GetParty())
			ch->NewChatPacket(STRING_D123);
		else
		{
			TEMP_BUFFER tbuf;

			tbuf.write(&pack_chat, sizeof(pack_chat));
			tbuf.write(chatbuf, len);

			RawPacketToCharacterFunc f(tbuf.read_peek(), tbuf.size());
			ch->GetParty()->ForEachOnlineMember(f);
		}
	}
	break;

	case CHAT_TYPE_GUILD:
	{
		if (!ch->GetGuild())
		{
			ch->NewChatPacket(STRING_D124);
		}
		else
		{
			ch->GetGuild()->Chat(chatbuf);
		}
	}
	break;

	default:
		sys_err("Unknown chat type %d", pinfo->type);
		break;
	}

	return (iExtraLen);
}

void CInputMain::ItemUse(LPCHARACTER ch, const char* data)
{
#ifdef ENABLE_CHEST_OPEN_RENEWAL
	if (((struct command_item_use*)data)->item_open_count == 0)
		ch->UseItem(((struct command_item_use*)data)->Cell);
	else
		ch->UseItem(((struct command_item_use*)data)->Cell, NPOS, ((struct command_item_use*)data)->item_open_count);
#else
	ch->UseItem(((struct command_item_use*)data)->Cell);
#endif
}

void CInputMain::ItemToItem(LPCHARACTER ch, const char* pcData)
{
	TPacketCGItemUseToItem* p = (TPacketCGItemUseToItem*)pcData;
	if (ch)
		ch->UseItem(p->Cell, p->TargetCell);
}

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
void CInputMain::ItemDelete(LPCHARACTER ch, const char* data)
{
	struct command_item_delete* pinfo = (struct command_item_delete*)data;

	if (ch)
		ch->DeleteItem(pinfo->Cell);
}

void CInputMain::ItemSell(LPCHARACTER ch, const char* data)
{
	struct command_item_sell* pinfo = (struct command_item_sell*)data;

	if (ch)
		ch->SellItem(pinfo->Cell);
}
#else
void CInputMain::ItemDrop(LPCHARACTER ch, const char* data)
{
	struct command_item_drop* pinfo = (struct command_item_drop*)data;
	if (!ch)
		return;

	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->DropItem(pinfo->Cell);
}

void CInputMain::ItemDrop2(LPCHARACTER ch, const char* data)
{
	TPacketCGItemDrop2* pinfo = (TPacketCGItemDrop2*)data;
	if (!ch)
		return;
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->DropItem(pinfo->Cell, pinfo->count);
}
#endif

void CInputMain::ItemMove(LPCHARACTER ch, const char* data)
{
	struct command_item_move* pinfo = (struct command_item_move*)data;

	if (ch)
		ch->MoveItem(pinfo->Cell, pinfo->CellTo, pinfo->count);
}

void CInputMain::ItemPickup(LPCHARACTER ch, const char* data)
{
	struct command_item_pickup* pinfo = (struct command_item_pickup*)data;
	if (ch)
		ch->PickupItem(pinfo->vid);
}

void CInputMain::QuickslotAdd(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_add* pinfo = (struct command_quickslot_add*)data;

	if (pinfo->slot.type == QUICKSLOT_TYPE_ITEM)// @fixme182
	{
		LPITEM item = NULL;
		TItemPos srcCell(INVENTORY, pinfo->slot.pos);
		if (!(item = ch->GetItem(srcCell)))
			return;
		if (item->GetType() != ITEM_USE && item->GetType() != ITEM_QUEST)
			return;
	}

	ch->SetQuickslot(pinfo->pos, pinfo->slot);
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_del* pinfo = (struct command_quickslot_del*)data;
	ch->DelQuickslot(pinfo->pos);
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_swap* pinfo = (struct command_quickslot_swap*)data;
	ch->SwapQuickslot(pinfo->pos, pinfo->change_pos);
}

int CInputMain::Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMessenger* p = (TPacketCGMessenger*)c_pData;

	if (uiBytes < sizeof(TPacketCGMessenger))
		return -1;

	c_pData += sizeof(TPacketCGMessenger);
	uiBytes -= sizeof(TPacketCGMessenger);

	switch (p->subheader)
	{
	case MESSENGER_SUBHEADER_CG_ADD_BY_VID:
	{
		if (uiBytes < sizeof(TPacketCGMessengerAddByVID))
			return -1;

		TPacketCGMessengerAddByVID* p2 = (TPacketCGMessengerAddByVID*)c_pData;
		LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

		if (!ch_companion)
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch->IsObserverMode())
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
		{
			ch->NewChatPacket(STRING_D127);
			return sizeof(TPacketCGMessengerAddByVID);
		}
		
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
		auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
		if (rkPlayerBlockMgr.IsPlayerBlock(ch->GetName(), ch_companion->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You cannot send a friend request to the player you have blocked.");
			return sizeof(TPacketCGMessengerAddByVID);
		}
		else if (rkPlayerBlockMgr.IsPlayerBlock(ch_companion->GetName(), ch->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "The player has blocked you.");
			return sizeof(TPacketCGMessengerAddByVID);
		}
#endif

		LPDESC d = ch_companion->GetDesc();

		if (!d)
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
		{
			ch->NewChatPacket(STRING_D125);
			return sizeof(TPacketCGMessengerAddByVID);
		}

		if (ch->GetDesc() == d)
			return sizeof(TPacketCGMessengerAddByVID);

		MessengerManager::instance().RequestToAdd(ch, ch_companion);
		//MessengerManager::instance().AddToList(ch->GetName(), ch_companion->GetName());
	}
	return sizeof(TPacketCGMessengerAddByVID);

	case MESSENGER_SUBHEADER_CG_ADD_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));

		if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(name) != GM_PLAYER)
		{
			ch->NewChatPacket(STRING_D125);
			return CHARACTER_NAME_MAX_LEN;
		}

		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (!tch)
			ch->NewChatPacket(STRING_D126, "%s", name);
		else
		{
			if (tch == ch)
				return CHARACTER_NAME_MAX_LEN;

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
			if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
			{
				ch->NewChatPacket(STRING_D127);
				return CHARACTER_NAME_MAX_LEN;
			}

			auto& rkPlayerBlockMgr = CPlayerBlock::Instance();
			if (rkPlayerBlockMgr.IsPlayerBlock(ch->GetName(), tch->GetName()))
				ch->ChatPacket(CHAT_TYPE_INFO, "You cannot send a friend request to the player you have blocked.");
			else if (rkPlayerBlockMgr.IsPlayerBlock(tch->GetName(), ch->GetName()))
				ch->ChatPacket(CHAT_TYPE_INFO, "The player has blocked you.");
#else
			if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ó´ë¹æÀÌ ¸Þ½ÅÁ® Ãß°¡ °ÅºÎ »óÅÂÀÔ´Ï´Ù."));
#endif
			else
			{
				MessengerManager::instance().RequestToAdd(ch, tch);
				//MessengerManager::instance().AddToList(ch->GetName(), tch->GetName());
			}
		}
	}
	return CHARACTER_NAME_MAX_LEN;

	case MESSENGER_SUBHEADER_CG_REMOVE:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char char_name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(char_name, c_pData, sizeof(char_name));
		MessengerManager::instance().RemoveFromList(ch->GetName(), char_name);
		MessengerManager::instance().RemoveFromList(char_name, ch->GetName()); // @fixme183
	}
	return CHARACTER_NAME_MAX_LEN;

#ifdef ENABLE_TELEPORT_TO_A_FRIEND
	case MESSENGER_SUBHEADER_CG_REQUEST_WARP_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
		{
			return -1;
		}

		if (!ch->IsActivateTime(FRIEND_TELEPORT_CHECK_TIME, 5)
			|| ch->WindowOpenCheck()
			|| ch->ActivateCheck()
			)
		{
			ch->NewChatPacket(STRING_D53);
			return CHARACTER_NAME_MAX_LEN;
		}

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));
		
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (!tch)
		{
			CCI* pch = P2P_MANAGER::Instance().Find(name);
			if (!pch)
			{
				ch->NewChatPacket(STRING_D126, "%s", name);
				return CHARACTER_NAME_MAX_LEN;
			}

			if (!ch->WarpToPlayerMapLevelControl(pch->lMapIndex))
			{
				ch->NewChatPacket(STRING_D192);
				return CHARACTER_NAME_MAX_LEN;
			}

			TPacketGGTeleportRequest request{};
			request.header = HEADER_GG_TELEPORT_REQUEST;
			request.subHeader = SUBHEADER_GG_TELEPORT_REQUEST;
			strlcpy(request.sender, ch->GetName(), CHARACTER_NAME_MAX_LEN + 1);
			strlcpy(request.target, name, CHARACTER_NAME_MAX_LEN + 1);

			P2P_MANAGER::Instance().Send(&request, sizeof(TPacketGGTeleportRequest));
		}
		else
		{
			if (tch == ch)
			{
				return CHARACTER_NAME_MAX_LEN;
			}

			if (!ch->WarpToPlayerMapLevelControl(tch->GetMapIndex()))
			{
				ch->NewChatPacket(STRING_D192);
				return CHARACTER_NAME_MAX_LEN;
			}

			if (tch->IsBlockMode(BLOCK_WARP_REQUEST))
			{
				ch->NewChatPacket(STRING_D129, "%s", tch->GetName());
				return CHARACTER_NAME_MAX_LEN;
			}

			tch->ChatPacket(CHAT_TYPE_COMMAND, "RequestWarpToCharacter %s", ch->GetName());
		}
		ch->SetActivateTime(FRIEND_TELEPORT_CHECK_TIME);
	}
	return CHARACTER_NAME_MAX_LEN;
			
	case MESSENGER_SUBHEADER_CG_SUMMON_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
		{
			return -1;
		}

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));

		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (strcmp(name, ch->GetName()) == 0)
		{
			return CHARACTER_NAME_MAX_LEN;
		}

		if (!tch)
		{
			CCI* pch = P2P_MANAGER::Instance().Find(name);
			if (!pch)
			{
				ch->NewChatPacket(STRING_D126, "%s", name);
				return CHARACTER_NAME_MAX_LEN;
			}

			TPacketGGTeleportRequest request{};
			request.header = HEADER_GG_TELEPORT_REQUEST;
			request.subHeader = SUBHEADER_GG_TELEPORT_ANSWER;
			strlcpy(request.sender, ch->GetName(), CHARACTER_NAME_MAX_LEN + 1);
			strlcpy(request.target, name, CHARACTER_NAME_MAX_LEN + 1);

			P2P_MANAGER::Instance().Send(&request, sizeof(TPacketGGTeleportRequest));
		}
		else
		{
			tch->WarpToPlayer(ch);
		}
	}
	return CHARACTER_NAME_MAX_LEN;
#endif


	default:
		sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}

	return 0;
}

#ifdef ENABLE_SPECIAL_STORAGE
typedef struct fckOFF
{
	BYTE		bySlot;
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	MAX_COUNT	byCount;
#else
	BYTE		byCount;
#endif
#ifdef ENABLE_SPECIAL_STORAGE
	BYTE		byType;
#endif
} TfckOFF;
#endif

int CInputMain::Shop(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGShop* p = (TPacketCGShop*)data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	const char* c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);

	switch (p->subheader)
	{
	case SHOP_SUBHEADER_CG_END:
		sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
		CShopManager::instance().StopShopping(ch);
		return 0;

	case SHOP_SUBHEADER_CG_BUY:
	{
		if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
			return -1;

		BYTE bPos = *(c_pData + 1);
		sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), bPos);
		CShopManager::instance().Buy(ch, bPos);
		return (sizeof(BYTE) + sizeof(BYTE));
	}

	case SHOP_SUBHEADER_CG_SELL:
	{
		if (uiBytes < sizeof(BYTE))
			return -1;

		BYTE pos = *c_pData;

		sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
		CShopManager::instance().Sell(ch, pos);
		return sizeof(BYTE);
	}

	case SHOP_SUBHEADER_CG_SELL2:
	{
#ifdef ENABLE_SPECIAL_STORAGE
		if (uiBytes < sizeof(TfckOFF))
			return -1;

		TfckOFF* p2 = (TfckOFF*)c_pData;

		sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
#ifdef ENABLE_SPECIAL_STORAGE
		CShopManager::instance().Sell(ch, p2->bySlot, p2->byCount, p2->byType);
#else
		CShopManager::instance().Sell(ch, p2->bySlot, p2->byCount);
#endif
		return sizeof(TfckOFF);
#else
		if (uiBytes < sizeof(BYTE) + sizeof(WORD))
			return -1;

		BYTE pos = *(c_pData++);
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT count = *(c_pData);
#else
		BYTE count = *(c_pData);
#endif

		sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());
		CShopManager::instance().Sell(ch, pos, count);
		return sizeof(BYTE) + sizeof(WORD);
#endif
	}

#ifdef ENABLE_MULTIPLE_BUY_ITEMS
	case SHOP_SUBHEADER_CG_MULTIBUY:
	{
		size_t size = sizeof(uint8_t) + sizeof(uint8_t);
		if (uiBytes < size) {
			return -1;
		}

		uint8_t p = *(c_pData++);
		uint8_t c = *(c_pData);
		sys_log(1, "INPUT: %s SHOP: MULTIPLE BUY %d COUNT %d", ch->GetName(), p, c);
		CShopManager::instance().MultipleBuy(ch, p, c);
		return size;
	}
#endif

	default:
		sys_err("CInputMain::Shop : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}

	return 0;
}

void CInputMain::OnClick(LPCHARACTER ch, const char* data)
{
	struct command_on_click* pinfo = (struct command_on_click*)data;
	LPCHARACTER			victim;

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid)))
		victim->OnClick(ch);
}

void CInputMain::Exchange(LPCHARACTER ch, const char* data)
{
	struct command_exchange* pinfo = (struct command_exchange*)data;
	LPCHARACTER	to_ch = NULL;

	if (!ch->CanHandleItem())
		return;

	int iPulse = thecore_pulse();

	if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
	{
		if (iPulse - to_ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			to_ch->NewChatPacket(STRING_D130, "%d", g_nPortalLimitTime);
			return;
		}

		if (true == to_ch->IsDead())
		{
			return;
		}
	}

	if (iPulse - ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->NewChatPacket(STRING_D130, "%d", g_nPortalLimitTime);
		return;
	}

	switch (pinfo->sub_header)
	{
	case EXCHANGE_SUBHEADER_CG_START:	// arg1 == vid of target character
		if (!ch->GetExchange())
		{
			if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
			{
				if (iPulse - ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
				{
					ch->NewChatPacket(STRING_D130, "%d", g_nPortalLimitTime);
					return;
				}

				if (iPulse - to_ch->GetActivateTime(SAFEBOX_CHECK_TIME) < PASSES_PER_SEC(g_nPortalLimitTime))
				{
					to_ch->NewChatPacket(STRING_D130, "%d", g_nPortalLimitTime);
					return;
				}

				if (ch->GetGold() >= GOLD_MAX)
				{
					ch->NewChatPacket(STRING_D131);

#ifdef ENABLE_EXTENDED_YANG_LIMIT
					sys_err("[OVERFLOG_GOLD] START (%lld) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
#else
					sys_err("[OVERFLOG_GOLD] START (%u) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
#endif
					return;
				}

				if (to_ch->IsPC())
				{
					if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
					{
						sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
						return;
					}
				}

				if (ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop() || ch->ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
					|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
					|| ch->Is67AttrOpen()
#endif
#ifdef ENABLE_BOT_CONTROL
					|| ch->IsBotControl()
#endif
					)
				{
					ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
					return;
				}
				ch->ExchangeStart(to_ch);
			}
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ITEM_ADD:	// arg1 == position of item, arg2 == position in exchange window
		if (ch->GetExchange())
		{
			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
#if defined(ENABLE_CHECKINOUT_UPDATE)
				ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2, pinfo->SelectPosAuto);
#else
				ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2);
#endif
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ITEM_DEL:	// arg1 == position of item
		if (ch->GetExchange())
		{
			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->RemoveItem(pinfo->arg1);
		}
		break;

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
		if (ch->GetExchange())
		{
			const int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(pinfo->arg1);

			if (GOLD_MAX <= nTotalGold)
			{
				ch->NewChatPacket(STRING_D132);

				sys_err("[OVERFLOW_GOLD] ELK_ADD (%lld) id %u name %s ",
					ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetName());

				return;
			}

			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->AddGold(pinfo->arg1);
		}
		break;
#else
	case EXCHANGE_SUBHEADER_CG_ELK_ADD:	// arg1 == amount of gold
		if (ch->GetExchange())
		{
			const int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(pinfo->arg1);

			if (GOLD_MAX <= nTotalGold)
			{
				ch->NewChatPacket(STRING_D132);

				sys_err("[OVERFLOW_GOLD] ELK_ADD (%u) id %u name %s ",
					ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetName());

				return;
			}

			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->AddGold(pinfo->arg1);
		}
		break;
#endif

	case EXCHANGE_SUBHEADER_CG_ACCEPT:	// arg1 == not used
		if (ch->GetExchange())
		{
			sys_log(0, "CInputMain()::Exchange() ==> ACCEPT ");
			ch->GetExchange()->Accept(true);
		}

		break;

	case EXCHANGE_SUBHEADER_CG_CANCEL:	// arg1 == not used
		if (ch->GetExchange())
			ch->GetExchange()->Cancel();
		break;
	}
}

void CInputMain::Position(LPCHARACTER ch, const char* data)
{
	struct command_position* pinfo = (struct command_position*)data;

	switch (pinfo->position)
	{
	case POSITION_GENERAL:
		ch->Standup();
		break;

	case POSITION_SITTING_CHAIR:
		ch->Sitdown(0);
		break;

	case POSITION_SITTING_GROUND:
		ch->Sitdown(1);
		break;
	}
}

void CInputMain::Move(LPCHARACTER ch, const char* data)
{
	if (!ch->CanMove())
		return;

	struct command_move* pinfo = (struct command_move*)data;

	if (pinfo->bFunc >= FUNC_MAX_NUM && !(pinfo->bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}

	{
#ifdef ENABLE_CHECK_GHOSTMODE
		if (ch->IsPC() && ch->IsDead())
		{
			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
#endif
		const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
		if (((false == ch->IsRiding() && fDist > 600) || fDist > 750) && OXEVENT_MAP_INDEX != ch->GetMapIndex())//@fixme3169 Hareket hizi yukselince diger oyuncularin yuklenmesi
		{
			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}
	}

	if (pinfo->bFunc == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();
		ch->Goto(pinfo->lX, pinfo->lY);
#ifdef ENABLE_AFK_MODE_SYSTEM
		if (ch->IsAway())
		{
			ch->SetAway(false);
			if (ch->IsAffectFlag(AFF_AFK))
				ch->RemoveAffect(AFFECT_AFK);
		}
#endif
	}
	else
	{
		if (pinfo->bFunc == FUNC_ATTACK || pinfo->bFunc == FUNC_COMBO)
			ch->OnMove(true);
		else if (pinfo->bFunc & FUNC_SKILL)
		{
			const int MASK_SKILL_MOTION = 0x7F;
			unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			if (!ch->IsUsableSkillMotion(motion))
			{
				ch->GetDesc()->DelayedDisconnect(number(150, 500));
			}
			ch->OnMove();
		}

		ch->SetRotation(pinfo->bRot * 5);
		ch->ResetStopTime();

		ch->Move(pinfo->lX, pinfo->lY);
		ch->Stop();
		ch->StopStaminaConsume();
#ifdef ENABLE_AFK_MODE_SYSTEM
		if (ch->IsAway())
		{
			ch->SetAway(false);
			if (ch->IsAffectFlag(AFF_AFK))
				ch->RemoveAffect(AFFECT_AFK);
		}
#endif
	}

	TPacketGCMove pack;

	pack.bHeader = HEADER_GC_MOVE;
	pack.bFunc = pinfo->bFunc;
	pack.bArg = pinfo->bArg;
	pack.bRot = pinfo->bRot;
	pack.dwVID = ch->GetVID();
	pack.lX = pinfo->lX;
	pack.lY = pinfo->lY;
	pack.dwTime = pinfo->dwTime;
	pack.dwDuration = (pinfo->bFunc == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0;

	ch->PacketAround(&pack, sizeof(TPacketGCMove), ch);
}

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		switch (type->type)
		{
		case SKILL_GEOMPUNG:
		case SKILL_SANGONG:
		case SKILL_YEONSA:
		case SKILL_KWANKYEOK:
		case SKILL_HWAJO:
		case SKILL_GIGUNG:
		case SKILL_PABEOB:
		case SKILL_MARYUNG:
		case SKILL_TUSOK:
		case SKILL_MAHWAN:
		case SKILL_BIPABU:
		case SKILL_NOEJEON:
		case SKILL_CHAIN:
		case SKILL_HORSE_WILDATTACK_RANGE:
			if (HEADER_CG_SHOOT != type->header)
			{
				return;
			}
			break;
		}
	}

	switch (header)
	{
	case HEADER_CG_ATTACK:
	{
		if (NULL == ch->GetDesc())
			return;

		const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);

		ch->GetDesc()->AssembleCRCMagicCube(packMelee->bCRCMagicCubeProcPiece, packMelee->bCRCMagicCubeFilePiece);

		LPCHARACTER	victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

		if (NULL == victim || ch == victim)
			return;

		switch (victim->GetCharType())
		{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
			return;
		}

		if (packMelee->bType > 0)
		{
			if (false == ch->CheckSkillHitCount(packMelee->bType, victim->GetVID()))
			{
				return;
			}
		}

		ch->Attack(victim, packMelee->bType);
	}
	break;

	case HEADER_CG_SHOOT:
	{
		const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

		ch->Shoot(packShoot->bType);
	}
	break;
	}
}

int CInputMain::SyncPosition(LPCHARACTER ch, const char* c_pcData, size_t uiBytes)
{
	const TPacketCGSyncPosition* pinfo = reinterpret_cast<const TPacketCGSyncPosition*>(c_pcData);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGSyncPosition);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (0 != (iExtraLen % sizeof(TPacketCGSyncPositionElement)))
	{
		sys_err("invalid packet length %d (name: %s)", pinfo->wSize, ch->GetName());
		return iExtraLen;
	}

	int iCount = iExtraLen / sizeof(TPacketCGSyncPositionElement);

	if (iCount <= 0)
		return iExtraLen;

	static const int nCountLimit = 16;

	if (iCount > nCountLimit)
	{
		sys_err("Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName());
		//ch->GetDesc()->SetPhase(PHASE_CLOSE);
		//return -1;
		iCount = nCountLimit;
	}

	TEMP_BUFFER tbuf;
	LPBUFFER lpBuf = tbuf.getptr();

	TPacketGCSyncPosition* pHeader = (TPacketGCSyncPosition*)buffer_write_peek(lpBuf);
	buffer_write_proceed(lpBuf, sizeof(TPacketGCSyncPosition));

	const TPacketCGSyncPositionElement* e =
		reinterpret_cast<const TPacketCGSyncPositionElement*>(c_pcData + sizeof(TPacketCGSyncPosition));

	timeval tvCurTime;
	gettimeofday(&tvCurTime, NULL);

	for (int i = 0; i < iCount; ++i, ++e)
	{
		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e->dwVID);

		if (!victim)
			continue;

		switch (victim->GetCharType())
		{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
			continue;
		}

		if (!victim->SetSyncOwner(ch))
			continue;

		const float fDistWithSyncOwner = DISTANCE_SQRT((victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100);
		static const float fLimitDistWithSyncOwner = 2500.f + 1000.f;

		if (fDistWithSyncOwner > fLimitDistWithSyncOwner)
		{
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				sys_err("Too far SyncPosition DistanceWithSyncOwner(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
					fDistWithSyncOwner, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
					e->lX, e->lY);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}

		const float fDist = DISTANCE_SQRT((victim->GetX() - e->lX) / 100, (victim->GetY() - e->lY) / 100);
		static const long g_lValidSyncInterval = 100 * 1000; // 100ms
		const timeval& tvLastSyncTime = victim->GetLastSyncTime();
		timeval* tvDiff = timediff(&tvCurTime, &tvLastSyncTime);

		if (tvDiff->tv_sec == 0 && tvDiff->tv_usec < g_lValidSyncInterval)
		{
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				sys_err("Too often SyncPosition Interval(%ldms)(%s) from Name(%s) VICTIM(%d,%d) SYNC(%d,%d)",
					tvDiff->tv_sec * 1000 + tvDiff->tv_usec / 1000, victim->GetName(), ch->GetName(), victim->GetX(), victim->GetY(),
					e->lX, e->lY);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}
		else if (fDist > 25.0f)
		{
			sys_err("Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				e->lX, e->lY);

			ch->GetDesc()->SetPhase(PHASE_CLOSE);

			return -1;
		}
		else
		{
			victim->SetLastSyncTime(tvCurTime);
			victim->Sync(e->lX, e->lY);
			buffer_write(lpBuf, e, sizeof(TPacketCGSyncPositionElement));
		}
	}

	if (buffer_size(lpBuf) != sizeof(TPacketGCSyncPosition))
	{
		pHeader->bHeader = HEADER_GC_SYNC_POSITION;
		pHeader->wSize = buffer_size(lpBuf);

		ch->PacketAround(buffer_read_peek(lpBuf), buffer_size(lpBuf), ch);
	}

	return iExtraLen;
}

void CInputMain::FlyTarget(LPCHARACTER ch, const char* pcData, BYTE bHeader)
{
	TPacketCGFlyTargeting* p = (TPacketCGFlyTargeting*)pcData;
	ch->FlyTarget(p->dwTargetVID, p->x, p->y, bHeader);
}

void CInputMain::UseSkill(LPCHARACTER ch, const char* pcData)
{
	TPacketCGUseSkill* p = (TPacketCGUseSkill*)pcData;
	ch->UseSkill(p->dwVnum, CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::ScriptButton(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptButton* p = (TPacketCGScriptButton*)c_pData;

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->idx & 0x80000000)
	{
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->idx & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->idx);
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptAnswer* p = (TPacketCGScriptAnswer*)c_pData;

	if (p->answer > 250)
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(), p->answer);
	}
}

// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptSelectItem* p = (TPacketCGScriptSelectItem*)c_pData;
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection);
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestInputString* p = (TPacketCGQuestInputString*)c_pData;

	char msg[65];
	strlcpy(msg, p->msg, sizeof(msg));

	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), msg);
}

void CInputMain::QuestConfirm(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestConfirm* p = (TPacketCGQuestConfirm*)c_pData;
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->requestPID);
	if (p->answer)
		p->answer = quest::CONFIRM_YES;

	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType)p->answer, ch->GetPlayerID());
	}
}

void CInputMain::Target(LPCHARACTER ch, const char* pcData)
{
	TPacketCGTarget* p = (TPacketCGTarget*)pcData;

	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->dwVID);

	if (pkObj)
	{
		TPacketGCTarget pckTarget;
		pckTarget.header = HEADER_GC_TARGET;
		pckTarget.dwVID = p->dwVID;
		ch->GetDesc()->Packet(&pckTarget, sizeof(TPacketGCTarget));
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::Warp(LPCHARACTER ch, const char* pcData)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, const char* c_pData)
{
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	TPacketCGSafeboxCheckin* p = (TPacketCGSafeboxCheckin*)c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox* pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->ItemPos);

	if (!pkSafebox || !pkItem)
		return;

	if (pkItem->GetCell() >= INVENTORY_MAX_NUM && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
		ch->NewChatPacket(STRING_D133);
		return;
	}

	// @lightworkfix END
	if (pkItem->IsEquipped())
	{
		return;
	}
	// @lightworkfix END

	if (!pkSafebox->IsEmpty(p->bSafePos, pkItem->GetSize()))
	{
		ch->NewChatPacket(STRING_D134);
		return;
	}
#ifdef ENABLE_SPECIAL_STORAGE
	if (pkItem->GetWindow() != INVENTORY)
	{
		if (true == pkItem->IsUpgradeItem() || true == pkItem->IsBook() || true == pkItem->IsStone() || true == pkItem->IsChest())
		{
			ch->NewChatPacket(STRING_D135);
			return;
		}
	}
#endif
	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->NewChatPacket(STRING_D133);
		return;
	}

	if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX))
	{
		ch->NewChatPacket(STRING_D133);
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->NewChatPacket(STRING_D133);
		return;
	}

	// @fixme140 BEGIN
	if (ITEM_BELT == pkItem->GetType() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		ch->NewChatPacket(STRING_D136);
		return;
	}
	// @fixme140 END

	pkItem->RemoveFromCharacter();
#ifdef ENABLE_SPECIAL_STORAGE
	if (!pkItem->IsDragonSoul() && !pkItem->IsUpgradeItem() && !pkItem->IsBook() && !pkItem->IsStone() && !pkItem->IsChest())
#else
	if (!pkItem->IsDragonSoul())
#endif
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->ItemPos.cell, UINT16_MAX);
	pkSafebox->Add(p->bSafePos, pkItem);
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, const char* c_pData, bool bMall)
{
	TPacketCGSafeboxCheckout* p = (TPacketCGSafeboxCheckout*)c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox* pkSafebox;

	if (bMall)
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->bSafePos);

	if (!pkItem)
		return;

	if (!ch->IsEmptyItemGrid(p->ItemPos, pkItem->GetSize()))
		return;

	if (pkItem->IsDragonSoul())
	{
		if (bMall)
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->ItemPos.window_type)
		{
			ch->NewChatPacket(STRING_D137);
			return;
		}

		TItemPos DestPos = p->ItemPos;
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}
			DestPos = TItemPos(DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

#ifdef ENABLE_SPECIAL_STORAGE
	else if (p->ItemPos.window_type != INVENTORY)
	{
		if (pkItem->IsUpgradeItem())
		{
			if (UPGRADE_INVENTORY != p->ItemPos.window_type)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyUpgradeInventory(pkItem);
			if (iCell < 0)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}
			DestPos = TItemPos(UPGRADE_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
		else if (pkItem->IsBook())
		{
			if (BOOK_INVENTORY != p->ItemPos.window_type)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyBookInventory(pkItem);
			if (iCell < 0)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}
			DestPos = TItemPos(BOOK_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
		else if (pkItem->IsStone())
		{
			if (STONE_INVENTORY != p->ItemPos.window_type)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyStoneInventory(pkItem);
			if (iCell < 0)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}
			DestPos = TItemPos(STONE_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
		else if (pkItem->IsChest())
		{
			if (CHEST_INVENTORY != p->ItemPos.window_type)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}

			TItemPos DestPos = p->ItemPos;

			int iCell = ch->GetEmptyChestInventory(pkItem);
			if (iCell < 0)
			{
				ch->NewChatPacket(STRING_D137);
				return;
			}
			DestPos = TItemPos(CHEST_INVENTORY, iCell);

			pkSafebox->Remove(p->bSafePos);
			pkItem->AddToCharacter(ch, DestPos);
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
		}
	}
#endif
	else
	{
#ifdef ENABLE_SPECIAL_STORAGE
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type ||
			UPGRADE_INVENTORY == p->ItemPos.window_type ||
			BOOK_INVENTORY == p->ItemPos.window_type ||
			STONE_INVENTORY == p->ItemPos.window_type ||
			CHEST_INVENTORY == p->ItemPos.window_type)
#else
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type)
#endif
		{
			ch->NewChatPacket(STRING_D137);
			return;
		}
		// @fixme119
		if (p->ItemPos.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(pkItem))
		{
			ch->NewChatPacket(STRING_D138);
			return;
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, p->ItemPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

	DWORD dwID = pkItem->GetID();
	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwID, sizeof(DWORD));
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, const char* data)
{
	struct command_item_move* pinfo = (struct command_item_move*)data;

	if (!ch->CanHandleItem())
		return;

	if (!ch->GetSafebox())
		return;

	ch->GetSafebox()->MoveItem(pinfo->Cell.cell, pinfo->CellTo.cell, pinfo->count);
}

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, const char* c_pData)
{
	if (!ch) //@Lightwork#26
	{
		return;
	}

	if (ch->GetArena())
	{
		ch->NewChatPacket(STRING_D16);
		return;
	}

	TPacketCGPartyInvite* p = (TPacketCGPartyInvite*)c_pData;

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid);

	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc() || !pInvitee->IsPC() || !ch->IsPC()) //@Lightwork#26
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, const char* c_pData)
{
	if (!ch) //@Lightwork#26
	{
		return;
	}

	if (ch->GetArena())
	{
		ch->NewChatPacket(STRING_D16);
		return;
	}

	TPacketCGPartyInviteAnswer* p = (TPacketCGPartyInviteAnswer*)c_pData;

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid);

	//if (!pInviter)
	if (!pInviter || !pInviter->IsPC()) //@Lightwork#26
		ch->NewChatPacket(STRING_D139);
	else if (!p->accept)
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	else
		pInviter->PartyInviteAccept(ch);
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, const char* c_pData)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->NewChatPacket(STRING_D140);
		return;
	}

	TPacketCGPartySetState* p = (TPacketCGPartySetState*)c_pData;

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->NewChatPacket(STRING_D141);
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid))
	{
		ch->NewChatPacket(STRING_D142);
		return;
	}

	DWORD pid = p->pid;

	switch (p->byRole)
	{
	case PARTY_ROLE_NORMAL:
		break;

	case PARTY_ROLE_ATTACKER:
	case PARTY_ROLE_TANKER:
	case PARTY_ROLE_BUFFER:
	case PARTY_ROLE_SKILL_MASTER:
	case PARTY_ROLE_HASTE:
	case PARTY_ROLE_DEFENDER:
		if (ch->GetParty()->SetRole(pid, p->byRole, p->flag))
		{
			TPacketPartyStateChange pack;
			pack.dwLeaderPID = ch->GetPlayerID();
			pack.dwPID = p->pid;
			pack.bRole = p->byRole;
			pack.bFlag = p->flag;
			db_clientdesc->DBPacket(HEADER_GD_PARTY_STATE_CHANGE, 0, &pack, sizeof(pack));
		}

		break;

	default:
		sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->byRole);
		break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, const char* c_pData)
{
	if (ch->GetArena())
	{
		ch->NewChatPacket(STRING_D16);
		return;
	}

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->NewChatPacket(STRING_D140);
		return;
	}

	if (ch->GetDungeon())
	{
		ch->NewChatPacket(STRING_D143);
		return;
	}

	TPacketCGPartyRemove* p = (TPacketCGPartyRemove*)c_pData;

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon())
		{
			ch->NewChatPacket(STRING_D143);
		}
		else
		{
			if (pParty->IsPartyInDungeon(351))
			{
				ch->NewChatPacket(STRING_D144);
				return;
			}

			// leader can remove any member
			if (p->pid == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid);
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->NewChatPacket(STRING_D145);
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid);
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid == ch->GetPlayerID())
		{
			if (ch->GetDungeon())
			{
				ch->NewChatPacket(STRING_D143);
			}
			else
			{
				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->NewChatPacket(STRING_D146);
					pParty->Quit(ch->GetPlayerID());
				}
			}
		}
		else
		{
			ch->NewChatPacket(STRING_D147);
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGAnswerMakeGuild* p = (TPacketCGAnswerMakeGuild*)c_pData;

	if (ch->GetGold() < 200000)
	{
		return;
	}

	if (ch->GetLevel() < 40)
	{
		return;//@fixme241
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
		CGuildManager::instance().GetDisbandDelay())
	{
		ch->NewChatPacket(STRING_D148, "%d",
			quest::CQuestManager::instance().GetEventFlag("guild_disband_delay"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
		CGuildManager::instance().GetWithdrawDelay())
	{
		ch->NewChatPacket(STRING_D149,"%d",
			quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay"));
		return;
	}

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, p->guild_name, sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name))
	{
		ch->NewChatPacket(STRING_D150);
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);

	if (dwGuildID)
	{
		ch->NewChatPacket(STRING_D151, "%s", cp.name);

		int GuildCreateFee = 200000;

		ch->PointChange(POINT_GOLD, -GuildCreateFee);
		//ch->SendGuildName(dwGuildID);
	}
	else
		ch->NewChatPacket(STRING_D152);
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyUseSkill* p = (TPacketCGPartyUseSkill*)c_pData;
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->NewChatPacket(STRING_D153);
		return;
	}

	switch (p->bySkillIndex)
	{
	case PARTY_SKILL_HEAL:
		ch->GetParty()->HealParty();
		break;
	case PARTY_SKILL_WARP:
	{
		LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid);
		if (pch->IsPC()) //@Lightwork#26
			ch->GetParty()->SummonToLeader(pch->GetPlayerID());
		else
			ch->NewChatPacket(STRING_D103);
	}
	break;
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyParameter* p = (TPacketCGPartyParameter*)c_pData;

#ifdef ENABLE_EXP_GROUP_FIX
	if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
#else
	if (ch->GetParty())
#endif
		ch->GetParty()->SetParameter(p->bDistributeMode);
}

size_t GetSubPacketSize(const GUILD_SUBHEADER_CG& header)
{
	switch (header)
	{
	case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:				return sizeof(int);
	case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:				return sizeof(int);
	case GUILD_SUBHEADER_CG_ADD_MEMBER:					return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_REMOVE_MEMBER:				return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:			return 10;
	case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:		return sizeof(BYTE) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_OFFER:						return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_CHARGE_GSP:					return sizeof(int);
	case GUILD_SUBHEADER_CG_POST_COMMENT:				return 1;
	case GUILD_SUBHEADER_CG_DELETE_COMMENT:				return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_REFRESH_COMMENT:			return 0;
	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:		return sizeof(DWORD) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_USE_SKILL:					return sizeof(TPacketCGGuildUseSkill);
	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:		return sizeof(DWORD) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:		return sizeof(DWORD) + sizeof(BYTE);
	}

	return 0;
}

int CInputMain::Guild(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuild))
		return -1;

	const TPacketCGGuild* p = reinterpret_cast<const TPacketCGGuild*>(data);
	const char* c_pData = data + sizeof(TPacketCGGuild);

	uiBytes -= sizeof(TPacketCGGuild);

	const GUILD_SUBHEADER_CG SubHeader = static_cast<GUILD_SUBHEADER_CG>(p->subheader);
	const size_t SubPacketLen = GetSubPacketSize(SubHeader);

	if (uiBytes < SubPacketLen)
	{
		return -1;
	}

	CGuild* pGuild = ch->GetGuild();

	if (NULL == pGuild)
	{
		if (SubHeader != GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER)
		{
			ch->NewChatPacket(STRING_D154);
			return SubPacketLen;
		}
	}

	switch (SubHeader)
	{
	case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:
	{
		return SubPacketLen;

		const int gold = MIN(*reinterpret_cast<const int*>(c_pData), __deposit_limit());

		if (gold < 0)
		{
			ch->NewChatPacket(STRING_D155);
			return SubPacketLen;
		}

		if (ch->GetGold() < gold)
		{
			ch->NewChatPacket(STRING_D83);
			return SubPacketLen;
		}

		pGuild->RequestDepositMoney(ch, gold);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:
	{
		return SubPacketLen;

		const int gold = MIN(*reinterpret_cast<const int*>(c_pData), 500000);

		if (gold < 0)
		{
			ch->NewChatPacket(STRING_D155);
			return SubPacketLen;
		}

		pGuild->RequestWithdrawMoney(ch, gold);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_ADD_MEMBER:
	{
		const DWORD vid = *reinterpret_cast<const DWORD*>(c_pData);
		LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(vid);

		if (!newmember)
		{
			ch->NewChatPacket(CMD_TALK_16);
			return SubPacketLen;
		}

		// @fixme145 BEGIN (+newmember ispc check)
		if (!ch->IsPC() || !newmember->IsPC())
			return SubPacketLen;
		// @fixme145 END

		pGuild->Invite(ch, newmember);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_REMOVE_MEMBER:
	{
		if (pGuild->UnderAnyWar() != 0)
		{
			ch->NewChatPacket(STRING_D156);
			return SubPacketLen;
		}

		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pid);

		if (member)
		{
			if (member->GetGuild() != pGuild)
			{
				ch->NewChatPacket(STRING_D157);
				return SubPacketLen;
			}

			if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
			{
				ch->NewChatPacket(STRING_D158);
				return SubPacketLen;
			}

			member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
			pGuild->RequestRemoveMember(member->GetPlayerID());
		}
		else
		{
			if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
			{
				ch->NewChatPacket(STRING_D158);
				return SubPacketLen;
			}

			if (pGuild->RequestRemoveMember(pid))
				ch->NewChatPacket(STRING_D159);
			else
				ch->NewChatPacket(CMD_TALK_16);
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:
	{
		char gradename[GUILD_GRADE_NAME_MAX_LEN + 1];
		strlcpy(gradename, c_pData + 1, sizeof(gradename));

		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->NewChatPacket(STRING_D160);
		}
		else if (*c_pData == GUILD_LEADER_GRADE)
		{
			ch->NewChatPacket(STRING_D161);
		}
		else if (!check_name(gradename))
		{
			ch->NewChatPacket(STRING_D162);
		}
		else
		{
			pGuild->ChangeGradeName(*c_pData, gradename);
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:
	{
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->NewChatPacket(STRING_D163);
		}
		else if (*c_pData == GUILD_LEADER_GRADE)
		{
			ch->NewChatPacket(STRING_D164);
		}
		else
		{
			pGuild->ChangeGradeAuth(*c_pData, *(c_pData + 1));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_OFFER:
	{
#ifdef ENABLE_CHAR_SETTINGS
		if (ch->GetCharSettings().antiexp)
		{
			ch->NewChatPacket(STRING_D165);
			return SubPacketLen;
		}
#endif
		DWORD offer = *reinterpret_cast<const DWORD*>(c_pData);

		if (pGuild->GetLevel() >= GUILD_MAX_LEVEL)
		{
			ch->NewChatPacket(STRING_D166);
		}
		else
		{
			offer /= 100;
			offer *= 100;

			if (pGuild->OfferExp(ch, offer))
			{
				ch->NewChatPacket(STRING_D167, "%u", offer);
#ifdef ENABLE_EXTENDED_BATTLE_PASS
				ch->UpdateExtBattlePassMissionProgress(GUILD_SPENT_EXP, offer, 0);
#endif
			}
			else
			{
				ch->NewChatPacket(STRING_D168);
			}
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHARGE_GSP:
	{
		const int offer = *reinterpret_cast<const int*>(c_pData);
		const int gold = offer * 100;

		if (offer < 0 || gold < offer || gold < 0 || ch->GetGold() < gold)
		{
			ch->NewChatPacket(STRING_D83);
			return SubPacketLen;
		}

		if (!pGuild->ChargeSP(ch, offer))
		{
			ch->NewChatPacket(STRING_D169);
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_POST_COMMENT:
	{
		const size_t length = *c_pData;

		if (length > GUILD_COMMENT_MAX_LEN)
		{
			sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), length);
			ch->GetDesc()->SetPhase(PHASE_CLOSE);
			return -1;
		}

		if (uiBytes < 1 + length)
			return -1;

		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (length && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE) && *(c_pData + 1) == '!')
		{
			ch->NewChatPacket(STRING_D170);
		}
		else
		{
			std::string str(c_pData + 1, length);
			pGuild->AddComment(ch, str);
		}

		return (1 + length);
	}

	case GUILD_SUBHEADER_CG_DELETE_COMMENT:
	{
		const DWORD comment_id = *reinterpret_cast<const DWORD*>(c_pData);

		pGuild->DeleteComment(ch, comment_id);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_REFRESH_COMMENT:
		pGuild->RefreshComment(ch);
		return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:
	{
		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE grade = *(c_pData + sizeof(DWORD));
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
			ch->NewChatPacket(STRING_D163);
		else if (ch->GetPlayerID() == pid)
			ch->NewChatPacket(STRING_D164);
		else if (grade == 1)
			ch->NewChatPacket(STRING_D171);
		else
			pGuild->ChangeMemberGrade(pid, grade);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_USE_SKILL:
	{
		const TPacketCGGuildUseSkill* p = reinterpret_cast<const TPacketCGGuildUseSkill*>(c_pData);

		pGuild->UseSkill(p->dwVnum, ch, p->dwPID);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:
	{
		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE is_general = *(c_pData + sizeof(DWORD));
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->NewChatPacket(STRING_D172);
		}
		else
		{
			if (!pGuild->ChangeMemberGeneral(pid, is_general))
			{
				ch->NewChatPacket(STRING_D173);
			}
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:
	{
		const DWORD guild_id = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE accept = *(c_pData + sizeof(DWORD));

		CGuild* g = CGuildManager::instance().FindGuild(guild_id);

		if (g)
		{
			if (accept)
				g->InviteAccept(ch);
			else
				g->InviteDeny(ch->GetPlayerID());
		}
	}
	return SubPacketLen;
	}

	return 0;
}

void CInputMain::Fishing(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGFishing* p = (TPacketCGFishing*)c_pData;
	ch->SetRotation(p->dir * 5);
	ch->fishing();
	return;
}

void CInputMain::ItemGive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGGiveItem* p = (TPacketCGGiveItem*)c_pData;
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->dwTargetVID);

	if (to_ch)
		ch->GiveItem(to_ch, p->ItemPos);
	else
		ch->NewChatPacket(STRING_D174);
}

void CInputMain::Hack(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGHack* p = (TPacketCGHack*)c_pData;

	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);

	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}

int CInputMain::MyShop(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMyShop* p = (TPacketCGMyShop*)c_pData;
	int iExtraLen = p->bCount * sizeof(TShopItemTable);

	if (uiBytes < sizeof(TPacketCGMyShop) + iExtraLen)
		return -1;

	if (ch->GetGold() >= GOLD_MAX)
	{
		ch->NewChatPacket(STRING_D175);
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}

	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		|| ch->IsAcceOpened()
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return (iExtraLen);
	}

	sys_log(0, "MyShop count %d", p->bCount);
	ch->OpenMyShop(p->szSign, (TShopItemTable*)(c_pData + sizeof(TPacketCGMyShop)), p->bCount);
	return (iExtraLen);
}

void CInputMain::Refine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRefine* p = reinterpret_cast<const TPacketCGRefine*>(c_pData);

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen() || ch->IsAcceOpened() || ch->IsOpenOfflineShop() || ch->ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| ch->Is67AttrOpen()
#endif
		)
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		ch->ClearRefineMode();
		return;
	}

	if (p->type == 255)
	{
		ch->ClearRefineMode();
		return;
	}

	if (p->pos >= INVENTORY_MAX_NUM)
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->pos);

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}
	
	ch->SetActivateTime(REFINE_CHECK_TIME);

	if (p->type == REFINE_TYPE_NORMAL)
	{
		ch->DoRefine(item);
	}
	else if (p->type == REFINE_TYPE_SCROLL || p->type == REFINE_TYPE_HYUNIRON || p->type == REFINE_TYPE_MUSIN || p->type == REFINE_TYPE_BDRAGON
#ifdef ENABLE_PLUS_SCROLL
		|| p->type == REFINE_TYPE_PLUS_SCROLL
#endif
		)
	{
		ch->DoRefineWithScroll(item);
	}

	ch->ClearRefineMode();
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CInputMain::Acce(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAcce* sPacket = (TPacketAcce*)c_pData;
	switch (sPacket->subheader)
	{
	case ACCE_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAcce();
	}
	break;
	case ACCE_SUBHEADER_CG_ADD:
	{
		pkChar->AddAcceMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAcceMaterial(sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAcceMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#ifndef ENABLE_TARGET_INFORMATION_RENEWAL
void CInputMain::TargetInfoLoad(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGTargetInfoLoad* p = (TPacketCGTargetInfoLoad*)c_pData;
	TPacketGCTargetInfo pInfo;
	pInfo.header = HEADER_GC_TARGET_INFO;
	static std::vector<LPITEM> s_vec_item;
	s_vec_item.clear();
	LPITEM pkInfoItem;
	LPCHARACTER m_pkChrTarget = CHARACTER_MANAGER::instance().Find(p->dwVID);

	if (!ch || !m_pkChrTarget) {
		return;
	}

	if (!ch->GetDesc()) {
		return;
	}

	if (ITEM_MANAGER::instance().CreateDropItemVector(m_pkChrTarget, ch, s_vec_item) && (m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone()))
	{
		if (s_vec_item.size() == 0);
		else if (s_vec_item.size() == 1)
		{
			pkInfoItem = s_vec_item[0];
			pInfo.dwVID = m_pkChrTarget->GetVID();
			pInfo.race = m_pkChrTarget->GetRaceNum();
			pInfo.dwVnum = pkInfoItem->GetVnum();
			pInfo.count = pkInfoItem->GetCount();
			ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
		}
		else
		{
			int iItemIdx = s_vec_item.size() - 1;
			while (iItemIdx >= 0)
			{
				pkInfoItem = s_vec_item[iItemIdx--];
				if (!pkInfoItem)
				{
					sys_err("pkInfoItem null in vector idx %d", iItemIdx + 1);
					continue;
				}
				pInfo.dwVID = m_pkChrTarget->GetVID();
				pInfo.race = m_pkChrTarget->GetRaceNum();
				pInfo.dwVnum = pkInfoItem->GetVnum();
				pInfo.count = pkInfoItem->GetCount();
				ch->GetDesc()->Packet(&pInfo, sizeof(TPacketGCTargetInfo));
			}
		}
	}
}
#endif
#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
void CInputMain::CubeRenewalSend(LPCHARACTER ch, const char* data)
{
	struct packet_send_cube_renewal* pinfo = (struct packet_send_cube_renewal*)data;
	switch (pinfo->subheader)
	{
	case CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM:
	{
		int index_item = pinfo->index_item;
		int count_item = pinfo->count_item;

		Cube_Make(ch, index_item, count_item);
	}
	break;

	case CUBE_RENEWAL_SUB_HEADER_CLOSE:
	{
		Cube_close(ch);
	}
	break;
	}
}
#endif

#ifdef ENABLE_OFFLINE_SHOP
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
template <class T>
bool CanDecode(T* p, int buffleft)
{
	return buffleft >= (int)sizeof(T);
}

template <class T>
const char* Decode(T*& pObj, const char* data, int* pbufferLeng = nullptr, int* piBufferLeft = nullptr)
{
	pObj = (T*)data;

	if (pbufferLeng)
		*pbufferLeng += sizeof(T);

	if (piBufferLeft)
		*piBufferLeft -= sizeof(T);

	return data + sizeof(T);
}

int OfflineshopPacketCreateNewShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopCreate* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::TShopInfo& rShopInfo = pack->shop;

	if (rShopInfo.dwCount > 500)
		return -1;

	std::vector<offlineshop::TShopItemInfo> vec;
	vec.reserve(rShopInfo.dwCount);

	offlineshop::TShopItemInfo* pItem = nullptr;

	for (DWORD i = 0; i < rShopInfo.dwCount; ++i)
	{
		if (!CanDecode(pItem, iBufferLeft))
			return -1;

		data = Decode(pItem, data, &iExtra, &iBufferLeft);
		vec.push_back(*pItem);
	}

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopCreateNewClientPacket(ch, rShopInfo, vec))
		ch->NewChatPacket(STRING_D203);

	return iExtra;
}

int OfflineshopPacketChangeShopName(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopChangeName* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopChangeNameClientPacket(ch, pack->szName))
		ch->NewChatPacket(STRING_D202);

	return iExtra;
}

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
int OfflineshopPacketExtendTime(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopExtendTime* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopExtendTimeClientPacket(ch, pack->dwTime))
		ch->NewChatPacket(STRING_D205);

	return iExtra;
}
#endif

#ifdef ENABLE_AVERAGE_PRICE
int OfflineshopPacketAveragePrice(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	DWORD* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvAveragePrice(ch, *pack);

	return iExtra;
}
#endif

int OfflineshopPacketForceCloseShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopForceCloseClientPacket(ch))
		ch->NewChatPacket(STRING_D204);

	return 0;
}

int OfflineshopPacketRequestShopList(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvShopRequestListClientPacket(ch);
	return 0;
}

int OfflineshopPacketOpenShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopOpen* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopOpenClientPacket(ch, pack->dwOwnerID))
		ch->NewChatPacket(STRING_D206);

	return iExtra;
}

int OfflineshopPacketOpenShowOwner(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopOpenMyShopClientPacket(ch))
		ch->NewChatPacket(STRING_D207);

	return 0;
}

int OfflineshopPacketBuyItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopBuyItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopBuyItemClientPacket(ch, pack->dwOwnerID, pack->dwItemID, pack->bIsSearch))
		ch->NewChatPacket(STRING_D208);

	return iExtra;
}

int OfflineshopPacketAddItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAddItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopAddItemClientPacket(ch, pack->pos, pack->price
#ifdef ENABLE_OFFLINE_SHOP_GRID
		, pack->display_pos
#endif
	))
		ch->NewChatPacket(STRING_D209);

	return iExtra;
}

int OfflineshopPacketRemoveItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGRemoveItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopRemoveItemClientPacket(ch, pack->dwItemID))
		ch->NewChatPacket(STRING_D210);

	return iExtra;
}

int OfflineshopPacketEditItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGEditItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopEditItemClientPacket(ch, pack->dwItemID, pack->price, pack->allEdit))
		ch->NewChatPacket(STRING_D211);

	return iExtra;
}

int OfflineshopPacketFilterRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGFilterRequest* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopFilterRequestClientPacket(ch, pack->filter))
		ch->NewChatPacket(STRING_D212);

	return iExtra;
}

int OfflineshopPacketOpenSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxOpenClientPacket(ch))
		ch->NewChatPacket(STRING_D214);

	return 0;
}

int OfflineshopPacketCloseBoard(LPCHARACTER ch)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvCloseBoardClientPacket(ch);
	return 0;
}

int OfflineshopPacketGetItemSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopSafeboxGetItem* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxGetItemClientPacket(ch, pack->dwItemID))
		ch->NewChatPacket(STRING_D215);

	return iExtra;
}

int OfflineshopPacketGetValutesSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopSafeboxGetValutes *pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxGetValutesClientPacket(ch, pack->gold))
		ch->NewChatPacket(STRING_D216);

	return iExtra;
}

int OfflineshopPacketCloseSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if (!rManager.RecvShopSafeboxCloseClientPacket(ch))
		ch->NewChatPacket(STRING_D217);

	return 0;
}


#ifdef ENABLE_SHOPS_IN_CITIES
int OfflineshopPacketClickEntity(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopClickEntity* pack = nullptr;
	if (!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra = 0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvShopClickEntity(ch, pack->dwShopVID);
	return iExtra;
}
#endif

int OfflineshopPacket(const char* data, LPCHARACTER ch, long iBufferLeft)
{
	if (iBufferLeft < (int)sizeof(TPacketCGNewOfflineShop))
		return -1;

	TPacketCGNewOfflineShop* pPack = nullptr;
	iBufferLeft -= sizeof(TPacketCGNewOfflineShop);
	data = Decode(pPack, data);

	switch (pPack->bSubHeader)
	{
	case offlineshop::SUBHEADER_CG_SHOP_CREATE_NEW:				return OfflineshopPacketCreateNewShop(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_CHANGE_NAME:			return OfflineshopPacketChangeShopName(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_FORCE_CLOSE:			return OfflineshopPacketForceCloseShop(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REQUEST_SHOPLIST:		return OfflineshopPacketRequestShopList(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OPEN:					return OfflineshopPacketOpenShop(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OPEN_OWNER:				return OfflineshopPacketOpenShowOwner(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_BUY_ITEM:				return OfflineshopPacketBuyItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_ADD_ITEM:				return OfflineshopPacketAddItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REMOVE_ITEM:			return OfflineshopPacketRemoveItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_EDIT_ITEM:				return OfflineshopPacketEditItem(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_FILTER_REQUEST:			return OfflineshopPacketFilterRequest(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_OPEN:			return OfflineshopPacketOpenSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_GET_ITEM:		return OfflineshopPacketGetItemSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_GET_VALUTES:	return OfflineshopPacketGetValutesSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_CLOSE:			return OfflineshopPacketCloseSafebox(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_CLOSE_BOARD:					return OfflineshopPacketCloseBoard(ch);
#ifdef ENABLE_SHOPS_IN_CITIES
	case offlineshop::SUBHEADER_CG_CLICK_ENTITY:				return OfflineshopPacketClickEntity(ch, data, iBufferLeft);
#endif
#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
	case offlineshop::SUBHEADER_CG_SHOP_EXTEND_TIME:			return OfflineshopPacketExtendTime(ch, data, iBufferLeft);
#endif
#ifdef ENABLE_AVERAGE_PRICE
	case offlineshop::SUBHEADER_CG_AVERAGE_PRICE:				return OfflineshopPacketAveragePrice(ch, data, iBufferLeft);
#endif

	default:
		sys_err("UNKNOWN SUBHEADER %d ", pPack->bSubHeader);
		return -1;
	}
}
#endif

#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
void CInputMain::ChangeLanguage(LPCHARACTER ch, BYTE bLanguage)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	BYTE bCurrentLanguage = ch->GetLanguage();

	if (bCurrentLanguage == bLanguage)
		return;

	if (bLanguage > LOCALE_YMIR && bLanguage < LOCALE_MAX_NUM)
	{
		TRequestChangeLanguage packet;
		packet.dwAID = ch->GetDesc()->GetAccountTable().id;
		packet.bLanguage = bLanguage;

		db_clientdesc->DBPacketHeader(HEADER_GD_REQUEST_CHANGE_LANGUAGE, 0, sizeof(TRequestChangeLanguage));
		db_clientdesc->Packet(&packet, sizeof(packet));

		ch->ChangeLanguage(bLanguage);
	}
}
#endif

#ifdef ENABLE_EXTENDED_WHISPER_DETAILS
void CInputMain::WhisperDetails(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGWhisperDetails* CGWhisperDetails = (TPacketCGWhisperDetails*)c_pData;

	if (!*CGWhisperDetails->name)
		return;

	TPacketGCWhisperDetails GCWhisperDetails;
	GCWhisperDetails.header = HEADER_GC_WHISPER_DETAILS;
	strncpy(GCWhisperDetails.name, CGWhisperDetails->name, sizeof(GCWhisperDetails.name) - 1);

	BYTE bLanguage = LOCALE_DEFAULT;

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(CGWhisperDetails->name);

	if (!pkChr)
	{
		LPDESC pkDesc = NULL;
		CCI* pkCCI = P2P_MANAGER::instance().Find(CGWhisperDetails->name);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			if (pkDesc)
				bLanguage = pkCCI->bLanguage;
		}
	}
	else
	{
		bLanguage = pkChr->GetLanguage();
	}

	GCWhisperDetails.bLanguage = bLanguage;
	ch->GetDesc()->Packet(&GCWhisperDetails, sizeof(GCWhisperDetails));
}
#endif


#ifdef ENABLE_EXTENDED_BATTLE_PASS
int CInputMain::ReciveExtBattlePassActions(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGExtBattlePassAction* p = (TPacketCGExtBattlePassAction*)data;

	if (uiBytes < sizeof(TPacketCGExtBattlePassAction))
		return -1;

	uiBytes -= sizeof(TPacketCGExtBattlePassAction);

	switch (p->bAction)
	{
	case 1:
		CBattlePassManager::instance().BattlePassRequestOpen(ch);
		return 0;

	case 2:
		if (get_dword_time() < ch->GetLastReciveExtBattlePassOpenRanking()) { // bu isin içinde bir kahpelik olabilir
			//ch->NewChatPacket(STRING_D176), ((ch->GetLastReciveExtBattlePassOpenRanking() - get_dword_time()) / 1000) + 1);
			return 0;
		}
		ch->SetLastReciveExtBattlePassOpenRanking(get_dword_time() + 10000);

		for (BYTE bBattlePassType = 1; bBattlePassType <= 3; ++bBattlePassType)
		{
			BYTE bBattlePassID;
			if (bBattlePassType == 1)
				bBattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
			if (bBattlePassType == 2) {
				bBattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
				//if (bBattlePassID != ch->GetExtBattlePassPremiumID())
				//	continue;
				if (!ch->FindAffect(AFFECT_BATTLEPASS))
					continue;
			}
			if (bBattlePassType == 3)
				bBattlePassID = CBattlePassManager::instance().GetEventBattlePassID();

			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT player_name, battlepass_type+0, battlepass_id, UNIX_TIMESTAMP(start_time), UNIX_TIMESTAMP(end_time) FROM player.battlepass_playerindex WHERE battlepass_type = %d and battlepass_id = %d and battlepass_completed = 1 and not player_name LIKE '[%%' ORDER BY (UNIX_TIMESTAMP(end_time)-UNIX_TIMESTAMP(start_time)) ASC LIMIT 40", bBattlePassType, bBattlePassID));
			if (pMsg->uiSQLErrno)
				return 0;

			MYSQL_ROW row;

			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				TPacketGCExtBattlePassRanking pack;
				pack.bHeader = HEADER_GC_EXT_BATTLE_PASS_SEND_RANKING;
				strlcpy(pack.szPlayerName, row[0], sizeof(pack.szPlayerName));
				pack.bBattlePassType = std::atoi(row[1]);
				pack.bBattlePassID = std::atoll(row[2]);
				pack.dwStartTime = std::atoll(row[3]);
				pack.dwEndTime = std::atoll(row[4]);

				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		break;

	case 10:
		CBattlePassManager::instance().BattlePassRequestReward(ch, 1);
		return 0;

	case 11:
		CBattlePassManager::instance().BattlePassRequestReward(ch, 2);
		return 0;

	case 12:
		CBattlePassManager::instance().BattlePassRequestReward(ch, 3);
		return 0;


	default:
		break;
	}

	return 0;
}

int CInputMain::ReciveExtBattlePassPremium(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGExtBattlePassSendPremium* p = (TPacketCGExtBattlePassSendPremium*)data;
	if (uiBytes < sizeof(TPacketCGExtBattlePassSendPremium))
		return -1;

	uiBytes -= sizeof(TPacketCGExtBattlePassSendPremium);

	if (p->premium && ch->FindAffect(AFFECT_BATTLEPASS))
	{
		ch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, CBattlePassManager::instance().GetPremiumBattlePassID());
		CBattlePassManager::instance().BattlePassRequestOpen(ch);
		ch->NewChatPacket(STRING_D177);
	}

	return 0;
}
#endif

#ifdef ENABLE_6TH_7TH_ATTR
void CInputMain::Attr67(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCG67Attr* p = reinterpret_cast<const TPacketCG67Attr*>(c_pData);
	
	if (ch->IsDead())
		return;

	if (ch->GetAttrInventoryItem())
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
#ifdef ENABLE_AURA_SYSTEM
		|| ch->isAuraOpened(true) || ch->isAuraOpened(false)
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		|| ch->IsAcceOpened()
#endif
		)
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	const LPITEM item = ch->GetInventoryItem(p->sItemPos);
	if (!item)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no item.");
		return;
	}

	switch (item->GetType())
	{
	case ITEM_WEAPON:
	case ITEM_ARMOR:
	case ITEM_PENDANT:
	case ITEM_GLOVE:
		break;

	default:
		ch->ChatPacket(CHAT_TYPE_INFO, "The item type is not suitable for 6-7 attr.");
		return;
	}

	if (item->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You cannot add a bonus to an equipped item.");
		return;
	}

	if (item->IsExchanging())
		return;

	const int norm_attr_count = item->GetAttributeCount();
	const int rare_attr_count = item->Get67AttrCount();
	const int attr_set_index = item->Get67AttrIdx();

	if (attr_set_index == -1 || norm_attr_count < 5 || rare_attr_count >= 2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This item is not suitable for 6-7 attr.");
		return;
	}

	enum
	{
		SUCCESS_PER_MATERIAL = 2,
		MATERIAL_MAX_COUNT = 10,
		SUPPORT_MAX_COUNT = 5,
	};

	if (p->bMaterialCount > MATERIAL_MAX_COUNT || p->bSupportCount > SUPPORT_MAX_COUNT)
		return;

	DWORD dwMaterialVnum = item->Get67MaterialVnum();
	if (dwMaterialVnum == 0 || p->bMaterialCount < 1 
		|| ch->CountSpecifyItem(dwMaterialVnum) < p->bMaterialCount)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You don't have enough material item.");
		return;
	}
	
	BYTE success = SUCCESS_PER_MATERIAL * p->bMaterialCount;
	
	if (p->sSupportPos != -1)
	{
		const LPITEM support_item = ch->GetInventoryItem(p->sSupportPos);
		if (support_item)
		{
			if (ch->CountSpecifyItem(support_item->GetVnum()) < p->bSupportCount)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "You don't have enough support item.");
				return;
			}

			BYTE uSupportSuccess = 0;
			switch (support_item->GetVnum())
			{
			case 72064:
				uSupportSuccess = 1;
				break;
			case 72065:
				uSupportSuccess = 2;
				break;
			case 72066:
				uSupportSuccess = 4;
				break;
			case 72067:
				uSupportSuccess = 10;
				break;
			default:
				ch->ChatPacket(CHAT_TYPE_INFO, "The support item is inappropriate.");
				return;
			}

			success += uSupportSuccess * p->bSupportCount;
			ch->RemoveSpecifyItem(support_item->GetVnum(), p->bSupportCount);
		}
	}

	ch->RemoveSpecifyItem(dwMaterialVnum, p->bMaterialCount);
	
	item->RemoveFromCharacter();
	item->AddToCharacter(ch, TItemPos(ATTR_INVENTORY, 0));

	if (number(1, 100) <= success)
	{
		item->Add67Attr();
		ch->SetQuestFlag("67attr.succes", 1);
	}
	else
	{
		ch->SetQuestFlag("67attr.succes", 0);
	}	
	
	ch->SetQuestFlag("67attr.time", get_global_time() + 21600);
	
}

void CInputMain::Attr67Close(LPCHARACTER ch, const char* c_pData)
{
	ch->Set67Attr(false);
}
#endif

#if defined(ENABLE_VOICE_CHAT)
struct FuncSendVoiceChatPacket
{
	FuncSendVoiceChatPacket(LPCHARACTER ch, const TVoiceChatPacket& packet, const char* audioData)
	{
		m_Character = ch;
		m_Packet = packet;
		m_AudioData = audioData;
		m_PlayerCount = 0;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;
			LPDESC d = ch->GetDesc();

			if (!d)
				return;

			if (ch == m_Character)
			{
				if (!test_server || quest::CQuestManager::Instance().GetEventFlag("voice_chat_hear_yourself") == 0)
					return;
			}

			if (ch->VoiceChatIsBlock(m_Packet.type))
				return;

			// If you have messenger block
			//if (BlocklistManager::instance().IsBlocked(m_Character->GetName(), ch->GetName()))
			//	return;

			//if (BlocklistManager::instance().IsBlocked(ch->GetName(), m_Character->GetName()))
			//	return;

			int distance = DISTANCE_APPROX(m_Character->GetX() - ch->GetX(), m_Character->GetY() - ch->GetY());
			if (m_Packet.type == VOICE_CHAT_TYPE_LOCAL && distance > 5'000)
				return;

			m_Packet.distance = static_cast<uint16_t>(distance);
			d->BufferedPacket(&m_Packet, sizeof(m_Packet));
			d->Packet(m_AudioData, m_Packet.dataSize);
			m_PlayerCount++;
		}
	}

	int m_PlayerCount;

private:
	LPCHARACTER m_Character;
	TVoiceChatPacket m_Packet;
	const char* m_AudioData;
};

int CInputMain::HandleVoiceChatPacket(const LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	LPDESC d = ch->GetDesc();
	TVoiceChatPacket pack = *(TVoiceChatPacket*)c_pData;
	pack.header = HEADER_GC_VOICE_CHAT;
	pack.vid = ch->GetVID();
	strlcpy(pack.name, ch->GetName(), sizeof(pack.name));

	if (!pack.dataSize)
		return 0;

	if (pack.dataSize != pack.size - sizeof(TVoiceChatPacket))
		return -1;

	if (uiBytes < pack.size)
		return -1;

	if (!pack.type || pack.type >= VOICE_CHAT_TYPE_MAX_NUM || ch->FindAffect(AFFECT_BLOCK_CHAT))
		return pack.dataSize;

	if (ch->GetLevel() < VOICE_CHAT_MIN_LEVEL)
	{
		static std::unordered_map<uint32_t, int> toNotify;
		int& nextNotification = toNotify[ch->GetPlayerID()];
		if (nextNotification < thecore_pulse())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need level %d to use the voice chat."), VOICE_CHAT_MIN_LEVEL);
			nextNotification = thecore_pulse() + PASSES_PER_SEC(1);
		}
		return pack.dataSize;
	}

	if (ch->VoiceChatIsBlock(pack.type))
		return pack.dataSize;

	if (g_bVoiceChatDisabled)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "voice_chat_disabled 1");
		return pack.dataSize;
	}

	FuncSendVoiceChatPacket f(ch, pack, c_pData + sizeof(pack));

	switch (pack.type)
	{
	case VOICE_CHAT_TYPE_LOCAL:
	{
		if (LPSECTREE tree = ch->GetSectree())
		{
			ch->EffectPacket(SE_VOICE_CHAT, d);
			tree->ForEachAround(f);
		}
	} break;

	case VOICE_CHAT_TYPE_PARTY:
	{
		if (LPPARTY party = ch->GetParty())
		{
			ch->EffectPacket(SE_VOICE_CHAT, d);
			party->ForEachOnMapMember(f, ch->GetMapIndex());
		}
	} break;

	case VOICE_CHAT_TYPE_GUILD:
	{
		if (CGuild* guild = ch->GetGuild())
		{
			ch->EffectPacket(SE_VOICE_CHAT, d);
			guild->ForEachOnMapMember(f, ch->GetMapIndex());
		}
	} break;

	default:
		sys_err("Unhandled voice type %u", pack.type);
		break;
	}

	return pack.dataSize;
}
#endif

int CInputMain::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_CHAT:
		if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_WHISPER:
		if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_MOVE:
		Move(ch, c_pData);
		// @fixme103 (removed CheckClientVersion since useless in here)
		break;

	case HEADER_CG_CHARACTER_POSITION:
		Position(ch, c_pData);
		break;

	case HEADER_CG_ITEM_USE:
		if (!ch->IsObserverMode())
			ItemUse(ch, c_pData);
		break;

#ifdef ENABLE_DROP_DIALOG_EXTENDED_SYSTEM
	case HEADER_CG_ITEM_DELETE:
		if (!ch->IsObserverMode())
			ItemDelete(ch, c_pData);
		break;

	case HEADER_CG_ITEM_SELL:
		if (!ch->IsObserverMode())
			ItemSell(ch, c_pData);
		break;
#else
	case HEADER_CG_ITEM_DROP:
		if (!ch->IsObserverMode())
		{
			ItemDrop(ch, c_pData);
		}
		break;

	case HEADER_CG_ITEM_DROP2:
		if (!ch->IsObserverMode())
			ItemDrop2(ch, c_pData);
		break;
#endif

	case HEADER_CG_ITEM_MOVE:
		if (!ch->IsObserverMode())
			ItemMove(ch, c_pData);
		break;

	case HEADER_CG_ITEM_PICKUP:
		if (!ch->IsObserverMode())
			ItemPickup(ch, c_pData);
		break;

	case HEADER_CG_ITEM_USE_TO_ITEM:
		if (!ch->IsObserverMode())
			ItemToItem(ch, c_pData);
		break;

	case HEADER_CG_ITEM_GIVE:
		if (!ch->IsObserverMode())
			ItemGive(ch, c_pData);
		break;

	case HEADER_CG_EXCHANGE:
		if (!ch->IsObserverMode())
			Exchange(ch, c_pData);
		break;

	case HEADER_CG_ATTACK:
	case HEADER_CG_SHOOT:
		if (!ch->IsObserverMode())
		{
			Attack(ch, bHeader, c_pData);
		}
		break;

	case HEADER_CG_USE_SKILL:
		if (!ch->IsObserverMode())
			UseSkill(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_ADD:
		QuickslotAdd(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_DEL:
		QuickslotDelete(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_SWAP:
		QuickslotSwap(ch, c_pData);
		break;

	case HEADER_CG_SHOP:
		if ((iExtraLen = Shop(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_MESSENGER:
		if ((iExtraLen = Messenger(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_ON_CLICK:
		OnClick(ch, c_pData);
		break;

	case HEADER_CG_SYNC_POSITION:
		if ((iExtraLen = SyncPosition(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_ADD_FLY_TARGETING:
	case HEADER_CG_FLY_TARGETING:
		FlyTarget(ch, c_pData, bHeader);
		break;

	case HEADER_CG_SCRIPT_BUTTON:
		ScriptButton(ch, c_pData);
		break;

		// SCRIPT_SELECT_ITEM
	case HEADER_CG_SCRIPT_SELECT_ITEM:
		ScriptSelectItem(ch, c_pData);
		break;
		// END_OF_SCRIPT_SELECT_ITEM

	case HEADER_CG_SCRIPT_ANSWER:
		ScriptAnswer(ch, c_pData);
		break;

	case HEADER_CG_QUEST_INPUT_STRING:
		QuestInputString(ch, c_pData);
		break;

	case HEADER_CG_QUEST_CONFIRM:
		QuestConfirm(ch, c_pData);
		break;

	case HEADER_CG_TARGET:
		Target(ch, c_pData);
		break;

	case HEADER_CG_WARP:
		Warp(ch, c_pData);
		break;

	case HEADER_CG_SAFEBOX_CHECKIN:
		SafeboxCheckin(ch, c_pData);
		break;

	case HEADER_CG_SAFEBOX_CHECKOUT:
		SafeboxCheckout(ch, c_pData, false);
		break;

	case HEADER_CG_SAFEBOX_ITEM_MOVE:
		SafeboxItemMove(ch, c_pData);
		break;

	case HEADER_CG_MALL_CHECKOUT:
		SafeboxCheckout(ch, c_pData, true);
		break;

	case HEADER_CG_PARTY_INVITE:
		PartyInvite(ch, c_pData);
		break;

	case HEADER_CG_PARTY_REMOVE:
		PartyRemove(ch, c_pData);
		break;

	case HEADER_CG_PARTY_INVITE_ANSWER:
		PartyInviteAnswer(ch, c_pData);
		break;

	case HEADER_CG_PARTY_SET_STATE:
		PartySetState(ch, c_pData);
		break;

	case HEADER_CG_PARTY_USE_SKILL:
		PartyUseSkill(ch, c_pData);
		break;

	case HEADER_CG_PARTY_PARAMETER:
		PartyParameter(ch, c_pData);
		break;

	case HEADER_CG_ANSWER_MAKE_GUILD:
#ifdef ENABLE_NEWGUILDMAKE
		ch->ChatPacket(CHAT_TYPE_INFO, "<%s> AnswerMakeGuild disabled", __FUNCTION__);
#else
		AnswerMakeGuild(ch, c_pData);
#endif
		break;

	case HEADER_CG_GUILD:
		if ((iExtraLen = Guild(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_FISHING:
		Fishing(ch, c_pData);
		break;

	case HEADER_CG_HACK:
		Hack(ch, c_pData);
		break;

	case HEADER_CG_MYSHOP:
		if ((iExtraLen = MyShop(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	case HEADER_CG_ACCE:
		Acce(ch, c_pData);
		break;
#endif

	case HEADER_CG_REFINE:
		Refine(ch, c_pData);
		break;

	case HEADER_CG_DRAGON_SOUL_REFINE:
	{
		TPacketCGDragonSoulRefine* p = reinterpret_cast <TPacketCGDragonSoulRefine*>((void*)c_pData);
		switch (p->bSubType)
		{
		case DS_SUB_HEADER_CLOSE:
			ch->DragonSoul_RefineWindow_Close();
			break;
		case DS_SUB_HEADER_DO_REFINE_GRADE:
		{
			DSManager::instance().DoRefineGrade(ch, p->ItemGrid);
		}
		break;
		case DS_SUB_HEADER_DO_REFINE_STEP:
		{
			DSManager::instance().DoRefineStep(ch, p->ItemGrid);
		}
		break;
		case DS_SUB_HEADER_DO_REFINE_STRENGTH:
		{
			DSManager::instance().DoRefineStrength(ch, p->ItemGrid);
		}
		break;
		}
	}
	break;

#ifdef ENABLE_TARGET_INFORMATION_SYSTEM
#ifndef ENABLE_TARGET_INFORMATION_RENEWAL
	case HEADER_CG_TARGET_INFO_LOAD:
		TargetInfoLoad(ch, c_pData);
		break;
#endif
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
	case HEADER_CG_CUBE_RENEWAL:
		CubeRenewalSend(ch, c_pData);
		break;
#endif
#ifdef ENABLE_OFFLINE_SHOP
	case HEADER_CG_NEW_OFFLINESHOP:
		if ((iExtraLen = OfflineshopPacket(c_pData, ch, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_SWITCHBOT
	case HEADER_CG_SWITCHBOT:
		if ((iExtraLen = Switchbot(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_BIOLOG_SYSTEM
	case HEADER_CG_BIOLOG:
		if ((iExtraLen = Biolog(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	case HEADER_CG_IN_GAME_LOG:
		if ((iExtraLen = InGameLog(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_ITEM_TRANSACTIONS
	case HEADER_CG_ITEM_TRANSACTIONS:
		if ((iExtraLen = ItemTransactions(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
	case HEADER_CG_BULK_WHISPER:
		BulkWhisperManager(ch, c_pData);
		break;
#endif
#ifdef ENABLE_DUNGEON_INFO
	case HEADER_CG_DUNGEON_INFO:
		if ((iExtraLen = DungeonInfo(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_SKILL_COLOR_SYSTEM
	case HEADER_CG_SKILL_COLOR:
		SetSkillColor(ch, c_pData);
		break;
#endif

#ifdef ENABLE_AURA_SYSTEM
	case HEADER_CG_AURA:
		Aura(ch, c_pData);
		break;
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	case HEADER_CG_NEW_PET:
		if ((iExtraLen = NewPet(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_MULTI_LANGUAGE_SYSTEM
	case HEADER_CG_CHANGE_LANGUAGE:
	{
		TPacketChangeLanguage* p = reinterpret_cast <TPacketChangeLanguage*>((void*)c_pData);
		ChangeLanguage(ch, p->bLanguage);
	}
	break;
#endif

#ifdef ENABLE_EXTENDED_WHISPER_DETAILS
	case HEADER_CG_WHISPER_DETAILS:
		WhisperDetails(ch, c_pData);
		break;
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	case HEADER_CG_EXT_BATTLE_PASS_ACTION:
		if ((iExtraLen = ReciveExtBattlePassActions(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_EXT_SEND_BP_PREMIUM:
		if ((iExtraLen = ReciveExtBattlePassPremium(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
	case HEADER_CG_REQUEST_PARTY_BONUS:
		if ((iExtraLen = RequestPartyBonus(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef ENABLE_6TH_7TH_ATTR
	case HEADER_CG_67_ATTR:
		Attr67(ch, c_pData);
		break;
		
	case HEADER_CG_CLOSE_67_ATTR:
		Attr67Close(ch, c_pData);
		break;
#endif
#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
	case HEADER_CG_OKEY_CARD:
		if ((iExtraLen = MiniGameOkeyCard(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_MINI_GAME_BNW
	case HEADER_CG_MINIGAME_BNW:
		if ((iExtraLen = MinigameBnw(ch, c_pData, m_iBufferLeft))<0)
			return -1;
		break;
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	case HEADER_CG_MINI_GAME_CATCH_KING:
		if ((iExtraLen = CatchKing::Instance().MiniGameCatchKing(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif
#ifdef ENABLE_VOICE_CHAT
		case HEADER_CG_VOICE_CHAT:
			if ((iExtraLen = HandleVoiceChatPacket(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;

			break;
#endif
	}
	return (iExtraLen);
}

int CInputDead::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		return 0;
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_CHAT:
		if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_WHISPER:
		if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_HACK:
		Hack(ch, c_pData);
		break;

	default:
		return (0);
	}

	return (iExtraLen);
}

#ifdef ENABLE_SWITCHBOT
int32_t CInputMain::Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGSwitchbot* p = reinterpret_cast<const TPacketCGSwitchbot*>(data);

	if (uiBytes < sizeof(TPacketCGSwitchbot))
	{
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGSwitchbot);
	uiBytes -= sizeof(TPacketCGSwitchbot);

	switch (p->subheader)
	{
	case SUBHEADER_CG_SWITCHBOT_START:
	{
		size_t extraLen = sizeof(TSwitchbotAttributeAlternativeTable) * SWITCHBOT_ALTERNATIVE_COUNT;
		if (uiBytes < extraLen)
		{
			return -1;
		}

		std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives;

		for (BYTE alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
		{
			const TSwitchbotAttributeAlternativeTable* pAttr = reinterpret_cast<const TSwitchbotAttributeAlternativeTable*>(c_pData);
			c_pData += sizeof(TSwitchbotAttributeAlternativeTable);

			vec_alternatives.emplace_back(*pAttr);
		}

		CSwitchbotManager::Instance().Start(ch->GetPlayerID(), p->slot, vec_alternatives);
		return extraLen;
	}

	case SUBHEADER_CG_SWITCHBOT_STOP:
	{
		CSwitchbotManager::Instance().Stop(ch->GetPlayerID(), p->slot);
		return 0;
	}
	}

	return 0;
}
#endif

#ifdef ENABLE_BIOLOG_SYSTEM
int CInputMain::Biolog(LPCHARACTER ch, const char* data, UINT uiBytes)
{
	if (!ch)
	{
		return -1;
	}

	TPacketCGBiolog* p = (TPacketCGBiolog*)data;
	data += sizeof(TPacketCGBiolog);

	BiologSystem* pkBiolog = ch->GetBiolog();
	if (!pkBiolog)
	{
		return -1;
	}

	return pkBiolog->RecvPacket(p->sub_header,data, uiBytes);
}
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
int CInputMain::InGameLog(LPCHARACTER ch, const char* data, UINT uiBytes)
{
	if (!ch)
	{
		return -1;
	}

	if (uiBytes < sizeof(InGameLog::TPacketCGInGameLog))
	{
		return -1;
	}
	
	InGameLog::TPacketCGInGameLog* p = (InGameLog::TPacketCGInGameLog*)data;
	data += sizeof(InGameLog::TPacketCGInGameLog);
	uiBytes -= sizeof(InGameLog::TPacketCGInGameLog);

	InGameLog::InGameLogManager& rIglMgr = InGameLog::GetManager();

	return rIglMgr.RecvPacketCG(ch,p->subHeader, p->logID);
}
#endif

#ifdef ENABLE_ITEM_TRANSACTIONS
int CInputMain::ItemTransactions(LPCHARACTER ch, const char* data, int BufferLeft)
{
	if (!ch)
		return -1;

	if (!ch->CanWarp(true))
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return BufferLeft;
	}

	if (BufferLeft < (int)sizeof(TItemTransactionsCGPacket))
		return -1;

	TItemTransactionsCGPacket* pack = nullptr;
	BufferLeft -= sizeof(TItemTransactionsCGPacket);
	data = Decode(pack, data);

	int extraBuffer = 0;
	std::vector<TItemTransactionsInfo> v_itemPos;
	v_itemPos.reserve(pack->itemCount);
	TItemTransactionsInfo* pInfo = nullptr;

	for (int i = 0; i < pack->itemCount; i++)
	{
		if (!CanDecode(pInfo, BufferLeft))
			return -1;

		data = Decode(pInfo, data, &extraBuffer, &BufferLeft);
		v_itemPos.push_back(*pInfo);
	}

	if (pack->transaction == 0)
	{
		ch->ItemTransactionSell(v_itemPos);
	}
	else
	{
		ch->ItemTransactionDelete(v_itemPos);
	}

	return extraBuffer;
}
#endif

#ifdef ENABLE_FAKE_CHAT
#define FILE_MAX_LEN 256
#include <iostream>
#include <fstream>
std::vector<std::string> vec_chNames;
std::string textArray[9] = {
	"AS", "as","aleykum selam", "as reis", "a.s", "A.S", "hosgeldin reis as", "A.s", "Aleykum Selam"
};
#define TIMER_TICK_DIVIDING 6
#define MIN_CHAT_COUNT_FOR_A_TICK 4
#define MAX_CHAT_COUNT_FOR_A_TICK 10
EVENTINFO(chat_shout_event_info)
{
	const char* szText;
	int	chatCount;
	int maxCount;
};
EVENTFUNC(chat_shout_update_event)
{
	chat_shout_event_info* info = dynamic_cast<chat_shout_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("mountsystem_update_event> <Factor> Null pointer");
		return 0;
	}
	if (info->chatCount >= info->maxCount) { vec_chNames.clear(); return 0; }

	int startIndex = info->chatCount;
	const int shoutCount = number(MIN_CHAT_COUNT_FOR_A_TICK, MAX_CHAT_COUNT_FOR_A_TICK);
	info->chatCount += shoutCount;

	for (int i = 0; i < shoutCount; i++) 
	{
		const int useIndex = startIndex + i;
		if (useIndex >= vec_chNames.size()) { vec_chNames.clear(); sys_err("for %d create",i);  return 0; }
		char chatbuf_global[CHAT_MAX_LEN + 1];
		std::string chName = vec_chNames[useIndex];
		std::string chatText = textArray[number(0, _countof(textArray) - 1)];
		const BYTE charEmpire = number(1, 5);
		const BYTE charLevel = number(75, 85);

		snprintf(chatbuf_global, sizeof(chatbuf_global), "%s |H%s%s|h(#)|h|r - Lv.%d|h|r : %s", chName.c_str(), "whisper:", chName.c_str(), charLevel, chatText.c_str());
		
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
		SendShout(chatbuf_global, charEmpire, chName.c_str());
#else
		SendShout(chatbuf_global, charEmpire);
#endif
		TPacketGGShout p;
		p.bHeader = HEADER_GG_SHOUT;
		p.bEmpire = charEmpire;
		strlcpy(p.szText, chatbuf_global, sizeof(p.szText));
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));
	}

	return PASSES_PER_SEC(1) / TIMER_TICK_DIVIDING;
}

void CreateShoutEvent() 
{
	FILE* f;
	char line[FILE_MAX_LEN];

	f = fopen("/lightwork/main/srv1/share/locale/europe/common/shout_bot_fake_users.txt", "r");
	if (!f) { return; }

	while (fgets(line, FILE_MAX_LEN, f)) 
	{
		std::string tmpStr = (std::string)line;
		std::string strLine = tmpStr.substr(0, tmpStr.size() - 2);
		vec_chNames.push_back(strLine);
	}

	if (!feof(f)) {
		fclose(f);
		return;
	}
	fclose(f);
	/***/

	chat_shout_event_info* info = AllocEventInfo<chat_shout_event_info>();
	info->maxCount = vec_chNames.size();
	info->chatCount = 0;
	event_create(chat_shout_update_event, info, PASSES_PER_SEC(1));
}
#endif

#ifdef ENABLE_PM_ALL_SEND_SYSTEM
void CInputMain::BulkWhisperManager(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGBulkWhisper* f = (TPacketCGBulkWhisper*)c_pData;

	if (!ch)
		return;

	if (ch->GetGMLevel() != GM_IMPLEMENTOR)
		return;

	TPacketGGBulkWhisper p;
	p.bHeader = HEADER_GG_BULK_WHISPER;
	p.lSize = strlen(f->szText) + 1;

	TEMP_BUFFER tmpbuf;
	tmpbuf.write(&p, sizeof(p));
	tmpbuf.write(f->szText, p.lSize);

	SendBulkWhisper(f->szText);
	P2P_MANAGER::instance().Send(tmpbuf.read_peek(), tmpbuf.size());
}
#endif

#ifdef ENABLE_DUNGEON_INFO
int CInputMain::DungeonInfo(LPCHARACTER ch, const char* data, int BufferLeft)
{
	if (!ch)
		return -1;

	if (BufferLeft < (int)sizeof(TPacketDungeonInfoCG))
		return -1;

	TPacketDungeonInfoCG* pack = nullptr;
	BufferLeft -= sizeof(TPacketDungeonInfoCG);
	data = Decode(pack, data);

	int extraBuffer = 0;

	switch (pack->subHeader)
	{
		case SUB_HEADER_CG_TIME_REQUEST:
		{
			ch->SendDungeonInfoTime();
			break;
		}

		case SUB_HEADER_CG_DUNGEON_LOGIN_REQUEST:
		{
			BYTE* dungeonIdx;
			if (!CanDecode(dungeonIdx, BufferLeft))
				return -1;

			data = Decode(dungeonIdx, data, &extraBuffer, &BufferLeft);
			ch->DungeonJoinBegin(*dungeonIdx);
			break;
		}

		case SUB_HEADER_CG_DUNGEON_TIME_RESET:
		{
			BYTE* dungeonIdx;
			if (!CanDecode(dungeonIdx, BufferLeft))
				return -1;

			data = Decode(dungeonIdx, data, &extraBuffer, &BufferLeft);
			break;
		}

		default: break;
	}

	return extraBuffer;
}
#endif


#ifdef ENABLE_SKILL_COLOR_SYSTEM
void CInputMain::SetSkillColor(LPCHARACTER ch, const char* pcData)
{
	TPacketCGSkillColor* p = (TPacketCGSkillColor*)pcData;

	if (p->skill >= ESkillColorLength::MAX_SKILL_COUNT)
		return;

	DWORD data[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memcpy(data, ch->GetSkillColor(), sizeof(data));

	data[p->skill][0] = p->col1;
	data[p->skill][1] = p->col2;
	data[p->skill][2] = p->col3;
	data[p->skill][3] = p->col4;
	data[p->skill][4] = p->col5;

	ch->NewChatPacket(STRING_D178);
	ch->SetSkillColor(data[0]);

	TSkillColor db_pack;
	memcpy(db_pack.dwSkillColor, data, sizeof(data));
	db_pack.player_id = ch->GetPlayerID();
	db_clientdesc->DBPacketHeader(HEADER_GD_SKILL_COLOR_SAVE, 0, sizeof(TSkillColor));
	db_clientdesc->Packet(&db_pack, sizeof(TSkillColor));
}
#endif

#ifdef ENABLE_AURA_SYSTEM
void CInputMain::Aura(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAura* sPacket = (TPacketAura*)c_pData;
	switch (sPacket->subheader)
	{
	case AURA_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAura();
	}
	break;
	case AURA_SUBHEADER_CG_ADD:
	{
		pkChar->AddAuraMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case AURA_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAuraMaterial(sPacket->bPos);
	}
	break;
	case AURA_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAuraMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

#ifdef ENABLE_NEW_PET_SYSTEM
int CInputMain::NewPet(LPCHARACTER ch, const char* data, int BufferLeft)
{
	if (!ch)
		return -1;

	if (BufferLeft < (int)sizeof(TNewPetCGPacket))
		return -1;

	TNewPetCGPacket* pack = nullptr;
	BufferLeft -= sizeof(TNewPetCGPacket);
	data = Decode(pack, data);

	CNewPet* newPet = ch->GetNewPetSystem();
	if (!newPet || !newPet->IsSummon())
		return false;

	int extraBuffer = 0;

	switch (pack->subHeader)
	{
		case SUB_CG_STAR_INCREASE:
		{
			newPet->IncreaseStar();
			break;
		}

		case SUB_CG_TYPE_INCREASE:
		{
			newPet->IncreaseType();
			break;
		}

		case SUB_CG_EVOLUTION_INCREASE:
		{
			newPet->IncreaseEvolution();
			break;
		}

		case SUB_CG_BONUS_CHANGE:
		{
			newPet->BonusChange();
			break;
		}

		case SUB_CG_ACTIVATE_SKIN:
		{
			newPet->ActivateSkin();
			break;
		}

		case SUB_CG_SKIN_CHANGE:
		{
			BYTE* skinIndex;
			if (!CanDecode(skinIndex, BufferLeft))
				return -1;

			data = Decode(skinIndex, data, &extraBuffer, &BufferLeft);
			newPet->SkinChange(*skinIndex);
			break;
		}
	}

	return extraBuffer;
}
#endif

#ifdef ENABLE_LEADERSHIP_EXTENSION
int CInputMain::RequestPartyBonus(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (1 == quest::CQuestManager::instance().GetEventFlag("disable_party_bonus"))
	{
		ch->NewChatPacket(STRING_D57);
		return -1;
	}

	if (ch->GetParty())
	{
		ch->NewChatPacket(STRING_D179);
		return 0;
	}

	TPacketCGRequestPartyBonus * p = (TPacketCGRequestPartyBonus *) data;

	if (uiBytes < sizeof(TPacketCGRequestPartyBonus))
		return -1;

	if ((ch->m_partyAddBonusPulse + 30) > get_global_time())
	{
		ch->NewChatPacket(WAIT_TO_USE_AGAIN, "%d", (ch->m_partyAddBonusPulse + 30) - get_global_time());
		return 0;
	}
	
	if (ch->GetLeadershipSkillLevel() < 20)
	{
		ch->NewChatPacket(STRING_D180);
		return 0;
	}
	
	ch->m_partyAddBonusPulse = get_global_time();

	float k = (float) ch->GetSkillPowerByLevel( MIN(SKILL_MAX_LEVEL, ch->GetLeadershipSkillLevel() ) )/ 100.0f;
		
	{
		bool bCompute = false;
		if(ch->GetPoint(POINT_PARTY_ATTACKER_BONUS))
		{
			ch->PointChange(POINT_PARTY_ATTACKER_BONUS, -ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
			bCompute = true;
		}
		
		if(ch->GetPoint(POINT_PARTY_TANKER_BONUS))
		{
			ch->PointChange(POINT_PARTY_TANKER_BONUS, -ch->GetPoint(POINT_PARTY_TANKER_BONUS));
			bCompute = true;
		}
			
		if(ch->GetPoint(POINT_PARTY_BUFFER_BONUS))
		{
			ch->PointChange(POINT_PARTY_BUFFER_BONUS, -ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
			bCompute = true;
		}
		
		if(ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS))
		{
			ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, -ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
			bCompute = true;
		}
	
		if(ch->GetPoint(POINT_PARTY_DEFENDER_BONUS))
		{
			ch->PointChange(POINT_PARTY_DEFENDER_BONUS, -ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
			bCompute = true;
		}
	
		if(ch->GetPoint(POINT_PARTY_HASTE_BONUS))
		{
			ch->PointChange(POINT_PARTY_HASTE_BONUS, -ch->GetPoint(POINT_PARTY_HASTE_BONUS));
			bCompute = true;
		}
	
		if(bCompute)
		{
			ch->ComputeBattlePoints();
			ch->ComputePoints();
		}
	}
	
	if(!p->bBonusID)
		return 0;
	
	bool bAdd = false;
	
	switch (p->bBonusID)
	{
		case PARTY_ROLE_ATTACKER:
			{
				int iBonus = (int) (10 + 60 * k);

				if (ch->GetPoint(POINT_PARTY_ATTACKER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_ATTACKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_TANKER:
			{
				int iBonus = (int) (50 + 1450 * k);

				if (ch->GetPoint(POINT_PARTY_TANKER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_TANKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_TANKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_BUFFER:
			{
				int iBonus = (int) (5 + 45 * k);

				if (ch->GetPoint(POINT_PARTY_BUFFER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_BUFFER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_SKILL_MASTER:
			{
				int iBonus = (int) (10*k);

				if (ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_HASTE:
			{
				int iBonus = (int) (1+5*k);
				if (ch->GetPoint(POINT_PARTY_HASTE_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_HASTE_BONUS, iBonus - ch->GetPoint(POINT_PARTY_HASTE_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_DEFENDER:
			{
				int iBonus = (int) (5+30*k);
				if (ch->GetPoint(POINT_PARTY_DEFENDER_BONUS) != iBonus)
				{
					bAdd = true;
					ch->PointChange(POINT_PARTY_DEFENDER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
			
		default:
			break;	
	}
	
	if(bAdd)
	{
		ch->SetQuestFlag("party.bonus_id", p->bBonusID);

		if(ch->FindAffect(AFFECT_PARTY_BONUS))
		{
			ch->RemoveAffect(AFFECT_PARTY_BONUS);
			ch->AddAffect(AFFECT_PARTY_BONUS, p->bBonusID, 0, 0, INFINITE_AFFECT_DURATION, 0, false);
		}
		else
			ch->AddAffect(AFFECT_PARTY_BONUS, p->bBonusID, 0, 0, INFINITE_AFFECT_DURATION, 0, false);
	}
	else
	{
		// No add so remove
		if(ch->FindAffect(AFFECT_PARTY_BONUS))
			ch->RemoveAffect(AFFECT_PARTY_BONUS);
	}
	
	return 0;
}
#endif

#ifdef ENABLE_MINI_GAME_OKEY_NORMAL
size_t GetMiniGameSubPacketLength(const EPacketCGMiniGameSubHeaderOkeyNormal& SubHeader)
{
	switch (SubHeader)
	{
		case SUBHEADER_CG_RUMI_START:
			return sizeof(TSubPacketCGMiniGameCardOpenClose);
		case SUBHEADER_CG_RUMI_EXIT:
			return 0;
		case SUBHEADER_CG_RUMI_DECKCARD_CLICK:
			return 0;
		case SUBHEADER_CG_RUMI_HANDCARD_CLICK:
			return sizeof(TSubPacketCGMiniGameHandCardClick);
		case SUBHEADER_CG_RUMI_FIELDCARD_CLICK:
			return sizeof(TSubPacketCGMiniGameFieldCardClick);
		case SUBHEADER_CG_RUMI_DESTROY:
			return sizeof(TSubPacketCGMiniGameDestroy);
	}

	return 0;
}

int CInputMain::MiniGameOkeyCard(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGMiniGameOkeyCard))
		return -1;

	const TPacketCGMiniGameOkeyCard* pinfo = reinterpret_cast<const TPacketCGMiniGameOkeyCard*>(data);
	const char* c_pData = data + sizeof(TPacketCGMiniGameOkeyCard);

	uiBytes -= sizeof(TPacketCGMiniGameOkeyCard);

	const EPacketCGMiniGameSubHeaderOkeyNormal SubHeader = static_cast<EPacketCGMiniGameSubHeaderOkeyNormal>(pinfo->bSubHeader);
	const size_t SubPacketLength = GetMiniGameSubPacketLength(SubHeader);
	if (uiBytes < SubPacketLength)
	{
		sys_err("invalid minigame subpacket length (sublen %d size %u buffer %u)", SubPacketLength, sizeof(TPacketCGMiniGameOkeyCard), uiBytes);
		return -1;
	}

	switch (SubHeader)
	{
		case SUBHEADER_CG_RUMI_START:
			{
				const TSubPacketCGMiniGameCardOpenClose* sp = reinterpret_cast<const TSubPacketCGMiniGameCardOpenClose*>(c_pData);
				ch->Cards_open(sp->bSafeMode);
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_EXIT:
			{
				ch->CardsEnd();
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_DESTROY:
			{
				const TSubPacketCGMiniGameDestroy* sp = reinterpret_cast<const TSubPacketCGMiniGameDestroy*>(c_pData);
				ch->CardsDestroy(sp->index);
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_DECKCARD_CLICK:
			{
				ch->Cards_pullout();
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_HANDCARD_CLICK:
			{
				const TSubPacketCGMiniGameHandCardClick* sp = reinterpret_cast<const TSubPacketCGMiniGameHandCardClick*>(c_pData);
				ch->CardsAccept(sp->index);
			}
			return SubPacketLength;

		case SUBHEADER_CG_RUMI_FIELDCARD_CLICK:
			{
				const TSubPacketCGMiniGameFieldCardClick* sp = reinterpret_cast<const TSubPacketCGMiniGameFieldCardClick*>(c_pData);
				ch->CardsRestore(sp->index);
			}
			return SubPacketLength;
	}

	return 0;
}
#endif
#ifdef ENABLE_MINI_GAME_BNW
int CInputMain::MinigameBnw(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMinigameBnw* p = (TPacketCGMinigameBnw*) c_pData;
	
	if (uiBytes < sizeof(TPacketCGMinigameBnw))
		return -1;

	c_pData += sizeof(TPacketCGMinigameBnw);
	uiBytes -= sizeof(TPacketCGMinigameBnw);

	switch (p->subheader)
	{
		case MINIGAME_BNW_SUBHEADER_CG_START:
			{
				MinigameBnwStart(ch);
			}
			return 0;
		case MINIGAME_BNW_SUBHEADER_CG_SELECTED_CARD:
			{
				if (uiBytes < sizeof(BYTE))
					return -1;

				const BYTE card = *reinterpret_cast<const BYTE*>(c_pData);
				MinigameBnwSelectedCard(ch, card);
			}
			return sizeof(BYTE);

		case MINIGAME_BNW_SUBHEADER_CG_FINISHED:
			{
				MinigameBnwFinished(ch);
			}
			return 0;
	}
	return 0;
}
#endif