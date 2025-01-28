/************************************************************
* title_name		: Challenge System
* author			: SkyOfDance
* date				: 15.08.2022
************************************************************/
#include "stdafx.h"
#ifdef ENABLE_CHALLENGE_SYSTEM
#include "ChallengeSystem.h"
#include "char.h"
#include "db.h"
#include "desc.h"
#include "p2p.h"

CChallenge::CChallenge()
{
	Destroy();
}

CChallenge::~CChallenge()
{
	Destroy();
}

void CChallenge::Destroy()
{
	memset(&m_ChallengeWinnerList, 0, sizeof(m_ChallengeWinnerList));
	m_Initialize = true;
}

void CChallenge::Initialize()
{
	for (int i = 0; i < ChallengeConf::MAX_QUEST_LIMIT; i++)
	{
		m_ChallengeWinnerList[i] = GetInfoDB(i);
	}
	m_Initialize = false;
}

TChallengeInfo CChallenge::GetInfoDB(BYTE id)
{
	TChallengeInfo s_Winner;
	memset(&s_Winner, 0, sizeof(s_Winner));
	s_Winner.bId = id;
	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery("SELECT Winner FROM challenge_system WHERE QuestId = %d", id));
	if (msg->Get()->uiNumRows != 0)//sqlde veri varsa
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
		s_Winner.bState = true;
		snprintf(s_Winner.szWinner, sizeof(s_Winner.szWinner), "%s", row[0]);
	}
	else
	{
		s_Winner.bState = false;
	}

	return s_Winner;
}

void CChallenge::SetWinner(BYTE id, LPCHARACTER ch)
{
	if (!ch || ch == NULL || ch == nullptr || !ch->GetDesc()) { return; }

	//Ne olur ne olmaz son bir sql kontrolü
	TChallengeInfo s_Winner = GetInfoDB(id);
	if (s_Winner.bState == true)
	{
		sys_err("state error challenge id %d character name %s", id, ch->GetName());
		return;
	}
	else
	{
		char query[512];
		snprintf(query, sizeof(query), "INSERT INTO challenge_system VALUES('%s', %d)", ch->GetName(), id);
		std::unique_ptr<SQLMsg> updatemsg(DBManager::instance().DirectQuery(query));
		s_Winner = GetInfoDB(id);
		if (s_Winner.bState == false)//yazýlýp yazýlmadýðýný kontrol ettik
		{
			sys_err("state error challenge id %d character name %s", id, ch->GetName());
			return;
		}
	}

	m_ChallengeWinnerList[id] = GetInfoDB(id);//cacheyi yeniledik

	//P2P
	TPacketGGChallengeUpdate pgg;
	pgg.bHeader = HEADER_GG_UPDATE_CHALLENGE;
	pgg.info = m_ChallengeWinnerList[id];
	P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGChallengeUpdate));

	//Ep ödülü
	char szQuery[512];
	sprintf(szQuery, "update account.account set cash = cash + %d where id = %d limit 1", m_Challenge_GiftValue[id], ch->GetDesc()->GetAccountTable().id);
	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));

	//Duyuru
	char szMsg[1024] = "<Challenge Duyurusu>";
	char szMsg2[256];
	snprintf(szMsg2, sizeof(szMsg2), "CHALLENGE_QUEST_NAME_%d", id + 1);

	size_t len = strlen(szMsg);
	len += snprintf(szMsg + len, sizeof(szMsg) - len,
		LC_TEXT(szMsg2));

	len += snprintf(szMsg + len, sizeof(szMsg) - len, " ");
	len += snprintf(szMsg + len, sizeof(szMsg) - len,
		LC_TEXT("CHALLENGE_WINNER %s %d"), ch->GetName(), m_Challenge_GiftValue[id]);
	RecvWinnerList(ch);//kazananan kiþiye update yolladýk

	BroadcastNotice(szMsg, true);

}

void CChallenge::RecvWinnerList(LPCHARACTER ch)
{
	if (!ch || ch == NULL || ch == nullptr) { return; }

	if (m_Initialize)//Verileri kontrol eder
	{
		Initialize();//yoksa yükler
		if (m_Initialize) { return; }
	}
	for (int i = 0; i < ChallengeConf::MAX_QUEST_LIMIT; i++)
	{
		if (m_ChallengeWinnerList[i].bState)//görevi tamamlanmýþlarý yollar
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "BINARY_SetWinnerChallenge %d %s",
				m_ChallengeWinnerList[i].bId, m_ChallengeWinnerList[i].szWinner);
		}
	}
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ChallengeUpdate");
}

bool CChallenge::GetWinner(BYTE id)
{
	if (m_Initialize)//Verileri kontrol eder
	{
		Initialize();//yoksa yükler
	}
	return m_ChallengeWinnerList[id].bState;
}

void CChallenge::RefinedChallenge(DWORD vnum, LPCHARACTER ch)
{
	if (!ch || ch == NULL || ch == nullptr || !ch->GetDesc()) { return; }
	BYTE questid = MAX_QUEST_LIMIT;
	switch (vnum)
	{
	case 68225: {questid = CHALLENGE_QUEST_NAME_7; break; }
	case 68025: {questid = CHALLENGE_QUEST_NAME_8; break; }
	case 46825: {questid = CHALLENGE_QUEST_NAME_9; break; }
	case 46625: {questid = CHALLENGE_QUEST_NAME_10; break; }
	case 41600: {questid = CHALLENGE_QUEST_NAME_18; break; }
	case 41590: {questid = CHALLENGE_QUEST_NAME_19; break; }
	case 10960: {questid = CHALLENGE_QUEST_NAME_20; break; }
	case 68250: {questid = CHALLENGE_QUEST_NAME_22; break; }
	case 68050: {questid = CHALLENGE_QUEST_NAME_23; break; }
	case 46850: {questid = CHALLENGE_QUEST_NAME_24; break; }
	case 46650: {questid = CHALLENGE_QUEST_NAME_25; break; }
	case 68275: {questid = CHALLENGE_QUEST_NAME_34; break; }
	case 68075: {questid = CHALLENGE_QUEST_NAME_35; break; }
	case 46875: {questid = CHALLENGE_QUEST_NAME_36; break; }
	case 46675: {questid = CHALLENGE_QUEST_NAME_37; break; }
	case 60760: {questid = CHALLENGE_QUEST_NAME_40; break; }
	case 28451: {questid = CHALLENGE_QUEST_NAME_47; break; }
	case 28457: {questid = CHALLENGE_QUEST_NAME_48; break; }
	case 28469: {questid = CHALLENGE_QUEST_NAME_49; break; }
	case 28463: {questid = CHALLENGE_QUEST_NAME_50; break; }
	case 68300: {questid = CHALLENGE_QUEST_NAME_55; break; }
	case 68100: {questid = CHALLENGE_QUEST_NAME_56; break; }
	case 46900: {questid = CHALLENGE_QUEST_NAME_57; break; }
	case 46700: {questid = CHALLENGE_QUEST_NAME_58; break; }

	case 24028: {questid = CHALLENGE_QUEST_NAME_61; break; }
	case 24031: {questid = CHALLENGE_QUEST_NAME_62; break; }
	case 24034: {questid = CHALLENGE_QUEST_NAME_63; break; }
	case 24037: {questid = CHALLENGE_QUEST_NAME_64; break; }
	case 24040: {questid = CHALLENGE_QUEST_NAME_65; break; }
	case 24043: {questid = CHALLENGE_QUEST_NAME_66; break; }
	case 24046: {questid = CHALLENGE_QUEST_NAME_67; break; }
	case 24049: {questid = CHALLENGE_QUEST_NAME_68; break; }
	case 24052: {questid = CHALLENGE_QUEST_NAME_69; break; }
	case 60170: {questid = CHALLENGE_QUEST_NAME_70; break; }
	case 11160: {questid = CHALLENGE_QUEST_NAME_72; break; }

	case 45425:
	case 45625:
	case 45825:
	case 46025:
	case 46225:
	case 46425:
	{
		questid = CHALLENGE_QUEST_NAME_11;
		break;
	}

	case 45650:
	case 45450:
	case 45850:
	case 46050:
	case 46250:
	case 46450:
	{
		questid = CHALLENGE_QUEST_NAME_26;
		break;
	}

	case 45675:
	case 45475:
	case 45875:
	case 46075:
	case 46275:
	case 46475:
	{
		questid = CHALLENGE_QUEST_NAME_38;
		break;
	}
	case 45500:
	case 45700:
	case 45900:
	case 46100:
	case 46300:
	case 46500:
	{
		questid = CHALLENGE_QUEST_NAME_59;
		break;
	}


	case 66215:
	case 66285:
	case 66355:
	case 66425:
	case 66495:
	case 66565:
	{
		questid = CHALLENGE_QUEST_NAME_17;
		break;
	}

	case 65414:
	case 65430:
	case 65446:
	case 65462:
	case 65478:
	case 65494:
	case 65510:
	{
		questid = CHALLENGE_QUEST_NAME_30;
		break;
	}

	case 65420:
	case 65436:
	case 65452:
	case 65468:
	case 65484:
	case 65500:
	case 65516:
	{
		questid = CHALLENGE_QUEST_NAME_42;
		break;
	}


	case 65015:
	case 65031:
	case 65047:
	case 65063:
	case 65079:
	case 65095:
	{
		questid = CHALLENGE_QUEST_NAME_31;
		break;
	}

	case 65021:
	case 65037:
	case 65053:
	case 65069:
	case 65085:
	case 65101:
	{
		questid = CHALLENGE_QUEST_NAME_43;
		break;
	}

	case 67309:
	case 67319:
	case 67329:
	case 67339:
	case 67349:
	{
		questid = CHALLENGE_QUEST_NAME_44;
		break;
	}

	default:
		return;
		break;
	}

	if (questid == MAX_QUEST_LIMIT)
		return;

	if (m_Initialize)//Verileri kontrol eder
	{
		Initialize();//yoksa yükler
		if (m_Initialize) { return; }
	}

	if (GetWinner(questid) == true)//challenge kazanýlmýþmý kontrol eder
	{
		return;
	}
	else
	{
		SetWinner(questid, ch);
	}

}

void CChallenge::UpdateP2P(TChallengeInfo pgg)
{
	m_ChallengeWinnerList[pgg.bId] = pgg;
}

#endif