#include "stdafx.h"
#include "utils.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "wedding.h"

#define NEED_TARGET	(1 << 0)
#define NEED_PC		(1 << 1)
#define WOMAN_ONLY	(1 << 2)
#define OTHER_SEX_ONLY	(1 << 3)
#define SELF_DISARM	(1 << 4)
#define TARGET_DISARM	(1 << 5)
#define BOTH_DISARM	(SELF_DISARM | TARGET_DISARM)

struct emotion_type_s
{
	const char* command;
	const char* command_to_client;
	long	flag;
	float	extra_delay;
} emotion_types[] = {
	{ "Å°½º",	"french_kiss",	NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		2.0f },
	{ "»Ç»Ç",	"kiss",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		1.5f },
	{ "µû±Í",	"slap",		NEED_PC | SELF_DISARM,				1.5f },
	{ "¹Ú¼ö",	"clap",		0,						1.0f },
	{ "¿Í",		"cheer1",	0,						1.0f },
	{ "¸¸¼¼",	"cheer2",	0,						1.0f },

	// DANCE
	{ "´í½º1",	"dance1",	0,						1.0f },
	{ "´í½º2",	"dance2",	0,						1.0f },
	{ "´í½º3",	"dance3",	0,						1.0f },
	{ "´í½º4",	"dance4",	0,						1.0f },
	{ "´í½º5",	"dance5",	0,						1.0f },
	{ "´í½º6",	"dance6",	0,						1.0f },
	// END_OF_DANCE
	{ "ÃàÇÏ",	"congratulation",	0,				1.0f	},
	{ "¿ë¼­",	"forgive",			0,				1.0f	},
	{ "È­³²",	"angry",			0,				1.0f	},
	{ "À¯È¤",	"attractive",		0,				1.0f	},
	{ "½½ÇÄ",	"sad",				0,				1.0f	},
	{ "ºê²ô",	"shy",				0,				1.0f	},
	{ "ÀÀ¿ø",	"cheerup",			0,				1.0f	},
	{ "ÁúÅõ",	"banter",			0,				1.0f	},
	{ "±â»Ý",	"joy",				0,				1.0f	},
	{ "\n",	"\n",		0,						0.0f },
};

std::set<std::pair<DWORD, DWORD> > s_emotion_set;

ACMD(do_emotion_allow)
{
	if (!ch) return;//@Lightwork#39

	if (ch->GetArena())
	{
		ch->NewChatPacket(CMD_TALK_12);
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD	val = 0; str_to_number(val, arg1);
	s_emotion_set.insert(std::make_pair(ch->GetVID(), val));
}

ACMD(do_emotion)
{
	int i;
	{
		if (ch->IsRiding())
		{
			ch->NewChatPacket(CMD_TALK_13);
			return;
		}
	}

	for (i = 0; *emotion_types[i].command != '\n'; ++i)
	{
		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command))
			break;

		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command_to_client))
			break;
	}

	if (*emotion_types[i].command == '\n')
	{
		sys_err("cannot find emotion");
		return;
	}

	if (IS_SET(emotion_types[i].flag, WOMAN_ONLY) && SEX_MALE == GET_SEX(ch))
	{
		ch->NewChatPacket(CMD_TALK_15);
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER victim = NULL;

	if (*arg1)
		victim = ch->FindCharacterInView(arg1, IS_SET(emotion_types[i].flag, NEED_PC));

	if (IS_SET(emotion_types[i].flag, NEED_TARGET | NEED_PC))
	{
		if (!victim)
		{
			ch->NewChatPacket(CMD_TALK_16);
			return;
		}
	}

	if (victim)
	{
		if (!victim->IsPC() || victim == ch)
			return;

		if (victim->IsRiding())
		{
			ch->NewChatPacket(CMD_TALK_17);
			return;
		}

		long distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

		if (distance < 10)
		{
			ch->NewChatPacket(CMD_TALK_18);
			return;
		}

		if (distance > 500)
		{
			ch->NewChatPacket(CMD_TALK_19);
			return;
		}

		if (IS_SET(emotion_types[i].flag, OTHER_SEX_ONLY))
		{
			if (GET_SEX(ch) == GET_SEX(victim))
			{
				ch->NewChatPacket(CMD_TALK_11);
				return;
			}
		}

		if (IS_SET(emotion_types[i].flag, NEED_PC))
		{
			if (s_emotion_set.find(std::make_pair(victim->GetVID(), ch->GetVID())) == s_emotion_set.end())
			{
				if (true == marriage::CManager::instance().IsMarried(ch->GetPlayerID()))
				{
					const marriage::TMarriage* marriageInfo = marriage::CManager::instance().Get(ch->GetPlayerID());

					const DWORD other = marriageInfo->GetOther(ch->GetPlayerID());

					if (0 == other || other != victim->GetPlayerID())
					{
						ch->NewChatPacket(CMD_TALK_10);
						return;
					}
				}
				else
				{
					ch->NewChatPacket(CMD_TALK_10);
					return;
				}
			}

			s_emotion_set.insert(std::make_pair(ch->GetVID(), victim->GetVID()));
		}
	}

	char chatbuf[256 + 1];
	int len = snprintf(chatbuf, sizeof(chatbuf), "%s %u %u",
		emotion_types[i].command_to_client,
		(DWORD)ch->GetVID(), victim ? (DWORD)victim->GetVID() : 0);

	if (len < 0 || len >= (int)sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	++len;

	TPacketGCChat pack_chat;
	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = CHAT_TYPE_COMMAND;
	pack_chat.id = 0;
	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(TPacketGCChat));
	buf.write(chatbuf, len);

	ch->PacketAround(buf.read_peek(), buf.size());
}