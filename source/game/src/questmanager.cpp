#include "stdafx.h"
#include <fstream>
#include "constants.h"
#include "buffer_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "text_file_loader.h"
#include "lzo_manager.h"
#include "item.h"
#include "config.h"
#include "target.h"
#include "party.h"
#include "locale_service.h"
#include "dungeon.h"
#ifdef ENABLE_QUEST_CATEGORY
#include <boost/tokenizer.hpp>
#endif

namespace quest
{
	using namespace std;

	CQuestManager::CQuestManager()
		: m_pSelectedDungeon(NULL), m_dwServerTimerArg(0), m_iRunningEventIndex(0), L(NULL), m_bNoSend(false),
		m_CurrentRunningState(NULL), m_pCurrentCharacter(NULL), m_pCurrentNPCCharacter(NULL), m_pCurrentPartyMember(NULL),
		m_pCurrentPC(NULL), m_iCurrentSkin(0), m_bError(false), m_pOtherPCBlockRootPC(NULL)
	{
	}

	CQuestManager::~CQuestManager()
	{
		Destroy();
	}

	void CQuestManager::Destroy()
	{
		if (L)
		{
			lua_close(L);
			L = NULL;
		}
	}

	bool CQuestManager::Initialize()
	{
		if (g_bAuthServer)
			return true;

		if (!InitializeLua())
			return false;

		m_pSelectedDungeon = NULL;

		m_mapEventName.emplace("click", QUEST_CLICK_EVENT);
		m_mapEventName.emplace("kill", QUEST_KILL_EVENT);
		m_mapEventName.emplace("timer", QUEST_TIMER_EVENT);
		m_mapEventName.emplace("levelup", QUEST_LEVELUP_EVENT);
		m_mapEventName.emplace("login", QUEST_LOGIN_EVENT);
		m_mapEventName.emplace("logout", QUEST_LOGOUT_EVENT);
		m_mapEventName.emplace("button", QUEST_BUTTON_EVENT);
		m_mapEventName.emplace("info", QUEST_INFO_EVENT);
		m_mapEventName.emplace("chat", QUEST_CHAT_EVENT);
		m_mapEventName.emplace("in", QUEST_ATTR_IN_EVENT);
		m_mapEventName.emplace("out", QUEST_ATTR_OUT_EVENT);
		m_mapEventName.emplace("use", QUEST_ITEM_USE_EVENT);
		m_mapEventName.emplace("server_timer", QUEST_SERVER_TIMER_EVENT);
		m_mapEventName.emplace("enter", QUEST_ENTER_STATE_EVENT);
		m_mapEventName.emplace("leave", QUEST_LEAVE_STATE_EVENT);
		m_mapEventName.emplace("letter", QUEST_LETTER_EVENT);
		m_mapEventName.emplace("take", QUEST_ITEM_TAKE_EVENT);
		m_mapEventName.emplace("target", QUEST_TARGET_EVENT);
		m_mapEventName.emplace("party_kill", QUEST_PARTY_KILL_EVENT);
		m_mapEventName.emplace("unmount", QUEST_UNMOUNT_EVENT);
		m_mapEventName.emplace("pick", QUEST_ITEM_PICK_EVENT);
		m_mapEventName.emplace("sig_use", QUEST_SIG_USE_EVENT);
		m_mapEventName.emplace("item_informer", QUEST_ITEM_INFORMER_EVENT);
#ifdef ENABLE_QUEST_DIE_EVENT
		m_mapEventName.emplace("die", QUEST_DIE_EVENT);
#endif
#ifdef ENABLE_QUEST_BOOT_EVENT
		m_mapEventName.emplace("boot", QUEST_BOOT_EVENT);
#endif
#ifdef ENABLE_QUEST_DND_EVENT
		m_mapEventName.emplace("dnd", QUEST_DND_EVENT);
#endif
		m_bNoSend = false;

		m_iCurrentSkin = QUEST_SKIN_NORMAL;

		{
			ifstream inf((g_stQuestDir + "/questnpc.txt").c_str());
			int line = 0;

			if (!inf.is_open())
				sys_err("QUEST Cannot open 'questnpc.txt'");
			else
				sys_log(0, "QUEST can open 'questnpc.txt' (%s)", g_stQuestDir.c_str());

			while (1)
			{
				unsigned int vnum;

				inf >> vnum;

				line++;

				if (inf.fail())
					break;

				string s;
				getline(inf, s);
				unsigned int li = 0, ri = s.size() - 1;
				while (li < s.size() && isspace(s[li])) li++;
				while (ri > 0 && isspace(s[ri])) ri--;

				if (ri < li)
				{
					sys_err("QUEST questnpc.txt:%d:npc name error", line);
					continue;
				}

				s = s.substr(li, ri - li + 1);

				int	n = 0;
				str_to_number(n, s.c_str());
				if (n)
					continue;

				m_mapNPC[vnum].Set(vnum, s);
				m_mapNPCNameID[s] = vnum;
			}

			// notarget quest
			m_mapNPC[0].Set(0, "notarget");
		}

		SetEventFlag("guild_withdraw_delay", 1);
		SetEventFlag("guild_disband_delay", 1);
#ifdef ENABLE_QUEST_CATEGORY
		ReadQuestCategoryToDict();
#endif
		return true;
	}

	unsigned int CQuestManager::FindNPCIDByName(const string& name)
	{
		map<string, unsigned int>::iterator it = m_mapNPCNameID.find(name);
		return it != m_mapNPCNameID.end() ? it->second : 0;
	}

	void CQuestManager::SelectItem(unsigned int pc, unsigned int selection)
	{
		PC* pPC = GetPC(pc);
		if (pPC && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_SELECT_ITEM)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args = 1;
			lua_pushnumber(pPC->GetRunningQuestState()->co, selection);

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
	}

	void CQuestManager::Confirm(unsigned int pc, EQuestConfirmType confirm, unsigned int pc2)
	{
		PC* pPC = GetPC(pc);

		if (!pPC->IsRunning())
		{
			sys_err("no quest running for pc, cannot process input : %u", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_CONFIRM)
		{
			sys_err("not wait for a confirm : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		if (pc2 && !pPC->IsConfirmWait(pc2))
		{
			sys_err("not wait for a confirm : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->ClearConfirmWait();

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args = 1;
		lua_pushnumber(pPC->GetRunningQuestState()->co, confirm);

		AddScript("[END_CONFIRM_WAIT]");
		SetSkinStyle(QUEST_SKIN_NOWINDOW);
		SendScript();

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}
	}

	int CQuestManager::ReadQuestCategoryFile(WORD q_index)
	{
		ifstream inf((g_stQuestDir + "/questcategory.txt").c_str());
		int line = 0;
		int c_qi = 99;

		if (!inf.is_open())
			sys_err("QUEST Cannot open 'questcategory.txt'");
		else
			sys_log(0, "QUEST can open 'questcategory.txt' (%s)", g_stQuestDir.c_str());

		while (1)
		{
			string qn = CQuestManager::instance().GetQuestNameByIndex(q_index);

			unsigned int category_num;

			//enum
			//{
			//	MAIN_QUEST,		//0
			//	SUB_QUEST,		//1
			//	COLLECT_QUEST,	//2
			//	LEVELUP_QUEST,	//3
			//	SCROLL_QUEST,	//4
			//	SYSTEM_QUEST,	//5
			//};

			inf >> category_num;

			line++;

			if (inf.fail())
				break;

			string s;
			getline(inf, s);
			unsigned int li = 0, ri = s.size() - 1;
			while (li < s.size() && isspace(s[li])) li++;
			while (ri > 0 && isspace(s[ri])) ri--;

			if (ri < li)
			{
				sys_err("QUEST questcategory.txt:%d:npc name error", line);
				continue;
			}

			s = s.substr(li, ri - li + 1);

			int	n = 0;
			str_to_number(n, s.c_str());
			if (n)
				continue;

			if (qn == s)
			{
				c_qi = category_num;
				break;
			}
		}

		// notarget quest
		//m_mapNPC[0].Set(0, "notarget");

		return c_qi;
	}

#ifdef ENABLE_QUEST_CATEGORY
	int CQuestManager::GetQuestCategoryByQuestIndex(WORD q_index)
	{
		if (QuestCategoryIndexMap.find(q_index) != QuestCategoryIndexMap.end())
			return QuestCategoryIndexMap[q_index];
		else
			return 0; /* DEFAULT_QUEST_CATEGORY */
	}

	void CQuestManager::ReadQuestCategoryToDict()
	{
		if (!QuestCategoryIndexMap.empty())
			QuestCategoryIndexMap.clear();

		ifstream inf((g_stQuestDir + "/questcategory.txt").c_str());

		if (!inf.is_open())
		{
			sys_err("QUEST Cannot open 'questcategory.txt'");
			return;
		}

		string lineFromFile;
		while (getline(inf, lineFromFile))
		{
			if (lineFromFile.empty())
				continue;

			boost::tokenizer<boost::escaped_list_separator<char> > token(lineFromFile, boost::escaped_list_separator<char>('\\', '\t', '\"'));
			vector<string> data(token.begin(), token.end());

			int category_num = atoi(data[0].c_str());
			string quest_name = data[1];

			unsigned int quest_index = CQuestManager::instance().GetQuestIndexByName(quest_name);

			if (quest_index != 0)
				QuestCategoryIndexMap[quest_index] = category_num;
			else
				sys_err("QUEST couldnt find QuestIndex for name Quest: %s(%d)", quest_name.c_str(), category_num);
		}
	}
#endif

	void CQuestManager::Input(unsigned int pc, const char* msg)
	{
		PC* pPC = GetPC(pc);
		if (!pPC)
		{
			sys_err("no pc! : %u", pc);
			return;
		}

		if (!pPC->IsRunning())
		{
			sys_err("no quest running for pc, cannot process input : %u", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_INPUT)
		{
			sys_err("not wait for a input : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args = 1;
		lua_pushstring(pPC->GetRunningQuestState()->co, msg);

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}
	}

	void CQuestManager::Select(unsigned int pc, unsigned int selection)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_SELECT)
		{
			pPC->SetSendDoneFlag();

			if (!pPC->GetRunningQuestState()->chat_scripts.empty())
			{
				QuestState& old_qs = *pPC->GetRunningQuestState();
				CloseState(old_qs);

				if (selection >= pPC->GetRunningQuestState()->chat_scripts.size())
				{
					pPC->SetSendDoneFlag();
					GotoEndState(old_qs);
					pPC->EndRunning();
				}
				else
				{
					AArgScript* pas = pPC->GetRunningQuestState()->chat_scripts[selection];
					ExecuteQuestScript(*pPC, pas->quest_index, pas->state_index, pas->script.GetCode(), pas->script.GetSize());
				}
			}
			else
			{
				// on default
				pPC->GetRunningQuestState()->args = 1;
				lua_pushnumber(pPC->GetRunningQuestState()->co, selection + 1);

				if (!RunState(*pPC->GetRunningQuestState()))
				{
					CloseState(*pPC->GetRunningQuestState());
					pPC->EndRunning();
				}
			}
		}
		else
		{
			sys_err("wrong QUEST_SELECT request! : %d", pc);
		}
	}

	void CQuestManager::Resume(unsigned int pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_PAUSE)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args = 0;

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
		else
		{
			//cerr << pPC << endl;
			//cerr << pPC->IsRunning() << endl;
			//cerr << pPC->GetRunningQuestState()->suspend_state;
			//cerr << SUSPEND_STATE_WAIT << endl;
			//cerr << "wrong QUEST_WAIT request! : " << pc << endl;
			sys_err("wrong QUEST_WAIT request! : %d", pc);
		}
	}

	void CQuestManager::EnterState(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnEnterState(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::LeaveState(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLeaveState(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::Letter(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLetter(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::LogoutPC(LPCHARACTER ch)
	{
		PC* pPC = GetPC(ch->GetPlayerID());

		if (pPC && pPC->IsRunning())
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->CancelRunning();
		}

		Logout(ch->GetPlayerID());

		if (ch == m_pCurrentCharacter)
		{
			m_pCurrentCharacter = NULL;
			m_pCurrentPC = NULL;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//
	//
	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::Login(unsigned int pc, const char* c_pszQuest)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLogin(*pPC, c_pszQuest);
		}
		else
		{
			sys_err("QUEST no such pc id : %d", pc);
		}
	}

	void CQuestManager::Logout(unsigned int pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLogout(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

#define ENABLE_PARTYKILL
	void CQuestManager::Kill(unsigned int pc, unsigned int npc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			// kill call script
			if (npc >= MAIN_RACE_MAX_NUM) //@fixme109
				m_mapNPC[npc].OnKill(*pPC); //@warme004
			m_mapNPC[QUEST_NO_NPC].OnKill(*pPC);

#ifdef ENABLE_PARTYKILL
			// party_kill call script
			LPCHARACTER ch = GetCurrentCharacterPtr();
			LPPARTY pParty = ch->GetParty();
			LPCHARACTER leader = pParty ? pParty->GetLeaderCharacter() : ch;
			if (leader)
			{
				m_pCurrentPartyMember = ch;
				if (npc >= MAIN_RACE_MAX_NUM) //@fixme109
					m_mapNPC[npc].OnPartyKill(*GetPC(leader->GetPlayerID())); //@warme004
				m_mapNPC[QUEST_NO_NPC].OnPartyKill(*GetPC(leader->GetPlayerID()));

				pPC = GetPC(pc);
			}
#endif
		}
		else
			sys_err("QUEST: no such pc id : %d", pc);
	}

#ifdef ENABLE_QUEST_DIE_EVENT
	void CQuestManager::Die(unsigned int pc, unsigned int npc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnDie(*pPC);
		}
		else
			sys_err("QUEST: no such pc id : %d", pc);
	}
#endif

#ifdef ENABLE_QUEST_BOOT_EVENT
	bool CQuestManager::Boot()
	{
		m_pCurrentPC = GetPCForce(0);
		m_pCurrentCharacter = NULL;
		m_pSelectedDungeon = NULL;
		return m_mapNPC[QUEST_NO_NPC].OnBoot(*m_pCurrentPC);
	}
#endif

#ifdef ENABLE_QUEST_DND_EVENT
	bool CQuestManager::DND(uint32_t pc, LPITEM item_dnd, LPITEM item_victim, bool bReceiveAll)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			// call script
			SetCurrentItem(item_victim);
			SetCurrentDNDItem(item_dnd);
			return m_mapNPC[item_dnd->GetVnum()].OnDND(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST DND_EVENT no such pc id : %d", pc);
			return false;
		}
	}
#endif

	bool CQuestManager::ServerTimer(unsigned int npc, unsigned int arg)
	{
		SetServerTimerArg(arg);
		m_pCurrentPC = GetPCForce(0);
		m_pCurrentCharacter = NULL;
		m_pSelectedDungeon = NULL;
		return m_mapNPC[npc].OnServerTimer(*m_pCurrentPC);
	}

	bool CQuestManager::Timer(unsigned int pc, unsigned int npc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				return false;
			}
			// call script
			return m_mapNPC[npc].OnTimer(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST TIMER_EVENT no such pc id : %d", pc);
			return false;
		}
		//cerr << "QUEST TIMER" << endl;
	}

	void CQuestManager::LevelUp(unsigned int pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLevelUp(*pPC);
		}
		else
		{
			sys_err("QUEST LEVELUP_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::AttrIn(unsigned int pc, LPCHARACTER ch, int attr)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
				return;

			// call script
			m_mapNPC[attr + QUEST_ATTR_NPC_START].OnAttrIn(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST no such pc id : %d", pc);
		}
	}

	void CQuestManager::AttrOut(unsigned int pc, LPCHARACTER ch, int attr)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			//m_pCurrentCharacter = ch;
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
				return;

			// call script
			m_mapNPC[attr + QUEST_ATTR_NPC_START].OnAttrOut(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST no such pc id : %d", pc);
		}
	}

	bool CQuestManager::Target(unsigned int pc, DWORD dwQuestIndex, const char* c_pszTargetName, const char* c_pszVerb)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			bool bRet;
			return m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, dwQuestIndex, c_pszTargetName, c_pszVerb, bRet);
		}

		return false;
	}

	void CQuestManager::QuestInfo(unsigned int pc, unsigned int quest_index)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
				return;

			m_bQuestInfoFlag = 1;
			m_mapNPC[QUEST_NO_NPC].OnInfo(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST INFO_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::QuestButton(unsigned int pc, unsigned int quest_index)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
				return;
			m_mapNPC[QUEST_NO_NPC].OnButton(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST CLICK_EVENT no such pc id : %d", pc);
		}
	}

	bool CQuestManager::TakeItem(unsigned int pc, unsigned int npc, LPITEM item)
	{
		//m_CurrentNPCRace = npc;
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;
			// call script
			SetCurrentItem(item);
			return m_mapNPC[npc].OnTakeItem(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	bool CQuestManager::UseItem(unsigned int pc, LPITEM item, bool bReceiveAll)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;
			// call script
			SetCurrentItem(item);

			return m_mapNPC[item->GetVnum()].OnUseItem(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	bool CQuestManager::SIGUse(unsigned int pc, DWORD sig_vnum, LPITEM item, bool bReceiveAll)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			// call script
			SetCurrentItem(item);

			return m_mapNPC[sig_vnum].OnSIGUse(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	bool CQuestManager::GiveItemToPC(unsigned int pc, LPCHARACTER pkChr)
	{
		if (!pkChr->IsPC())
			return false;

		PC* pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			TargetInfo* pInfo = CTargetManager::instance().GetTargetInfo(pc, TARGET_TYPE_VID, pkChr->GetVID());

			if (pInfo)
			{
				bool bRet;

				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return true;
			}
		}

		return false;
	}

	bool CQuestManager::Click(unsigned int pc, LPCHARACTER pkChrTarget)
	{
		PC* pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			TargetInfo* pInfo = CTargetManager::instance().GetTargetInfo(pc, TARGET_TYPE_VID, pkChrTarget->GetVID());

			if (pInfo)
			{
				bool bRet;
				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return bRet;
			}

			DWORD dwCurrentNPCRace = pkChrTarget->GetRaceNum();

			if (pkChrTarget->IsNPC())
			{
				map<unsigned int, NPC>::iterator it = m_mapNPC.find(dwCurrentNPCRace);

				if (it == m_mapNPC.end())
				{
					return false;
				}

				// call script
				if (it->second.HasChat())
				{
					// if have chat, give chat

					if (!it->second.OnChat(*pPC))
					{
						return it->second.OnClick(*pPC);
					}

					return true;
				}
				else
				{
					// else click
					return it->second.OnClick(*pPC);
				}
			}
			return false;
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST CLICK_EVENT no such pc id : %d", pc);
			return false;
		}
		//cerr << "QUEST CLICk" << endl;
	}

	void CQuestManager::Unmount(unsigned int pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnUnmount(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::ItemInformer(unsigned int pc, unsigned int vnum)
	{
		PC* pPC;
		pPC = GetPC(pc);

		m_mapNPC[QUEST_NO_NPC].OnItemInformer(*pPC, vnum);
	}
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::LoadStartQuest(const string& quest_name, unsigned int idx)
	{
		for (itertype(g_setQuestObjectDir) it = g_setQuestObjectDir.begin(); it != g_setQuestObjectDir.end(); ++it)
		{
			const string& stQuestObjectDir = *it;
			string full_name = stQuestObjectDir + "/begin_condition/" + quest_name;
			ifstream inf(full_name.c_str());

			if (inf.is_open())
			{
				istreambuf_iterator<char> ib(inf), ie;
				copy(ib, ie, back_inserter(m_hmQuestStartScript[idx]));
			}
		}
	}

	bool CQuestManager::CanStartQuest(unsigned int quest_index, const PC& pc)
	{
		return CanStartQuest(quest_index);
	}

	bool CQuestManager::CanStartQuest(unsigned int quest_index)
	{
		THashMapQuestStartScript::iterator it;

		if ((it = m_hmQuestStartScript.find(quest_index)) == m_hmQuestStartScript.end())
			return true;
		else
		{
			int x = lua_gettop(L);
			lua_dobuffer(L, &(it->second[0]), it->second.size(), "StartScript");
			int bStart = lua_toboolean(L, -1);
			lua_settop(L, x);
			return bStart != 0;
		}
	}

	bool CQuestManager::CanEndQuestAtState(const string& quest_name, const string& state_name)
	{
		return false;
	}

	void CQuestManager::DisconnectPC(LPCHARACTER ch)
	{
		m_mapPC.erase(ch->GetPlayerID());
	}

	PC* CQuestManager::GetPCForce(unsigned int pc)
	{
		PCMap::iterator it;

		if ((it = m_mapPC.find(pc)) == m_mapPC.end())
		{
			PC* pPC = &m_mapPC[pc];
			pPC->SetID(pc);
			return pPC;
		}

		return &it->second;
	}

	PC* CQuestManager::GetPC(unsigned int pc)
	{
		PCMap::iterator it;

		LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(pc);

		if (!pkChr)
			return NULL;

		m_pCurrentPC = GetPCForce(pc);
		m_pCurrentCharacter = pkChr;
		m_pSelectedDungeon = NULL;
		return (m_pCurrentPC);
	}

	void CQuestManager::ClearScript()
	{
		m_strScript.clear();
		m_iCurrentSkin = QUEST_SKIN_NORMAL;
	}

	void CQuestManager::AddScript(const string& str)
	{
		m_strScript += str;
	}

	void CQuestManager::SendScript()
	{
		if (m_bNoSend || !GetCurrentCharacterPtr() || !GetCurrentCharacterPtr()->GetDesc()) // @fixme174 (ch check)
		{
			m_bNoSend = false;
			ClearScript();
			return;
		}

		if (m_strScript == "[DONE]" || m_strScript == "[NEXT]")
		{
			if (m_pCurrentPC && !m_pCurrentPC->GetAndResetDoneFlag() && m_strScript == "[DONE]" && m_iCurrentSkin == QUEST_SKIN_NORMAL && !IsError())
			{
				ClearScript();
				return;
			}
			m_iCurrentSkin = QUEST_SKIN_NOWINDOW;
		}

		//send -_-!
		struct ::packet_script packet_script;

		packet_script.header = HEADER_GC_SCRIPT;
		packet_script.skin = m_iCurrentSkin;
		packet_script.src_size = m_strScript.size();
		packet_script.size = packet_script.src_size + sizeof(struct packet_script);

		TEMP_BUFFER buf;
		buf.write(&packet_script, sizeof(struct packet_script));
		buf.write(&m_strScript[0], m_strScript.size());

		GetCurrentCharacterPtr()->GetDesc()->Packet(buf.read_peek(), buf.size());

		ClearScript();
		m_bQuestInfoFlag = 0;
	}

	const char* CQuestManager::GetQuestStateName(const string& quest_name, const int state_index)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L, -1))
		{
			sys_err("QUEST wrong quest state file %s.%d", quest_name.c_str(), state_index);
			lua_settop(L, x);
			return "";
		}
		lua_pushnumber(L, state_index);
		lua_gettable(L, -2);

		const char* str = lua_tostring(L, -1);
		lua_settop(L, x);
		return str;
	}

	int CQuestManager::GetQuestStateIndex(const string& quest_name, const string& state_name)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L, -1))
		{
			sys_err("QUEST wrong quest state file %s.%s", quest_name.c_str(), state_name.c_str());
			lua_settop(L, x);
			return 0;
		}
		lua_pushstring(L, state_name.c_str());
		lua_gettable(L, -2);

		int v = (int)rint(lua_tonumber(L, -1));
		lua_settop(L, x);
		return v;
	}

	void CQuestManager::SetSkinStyle(int iStyle)
	{
		if (iStyle < 0 || iStyle >= QUEST_SKIN_COUNT)
		{
			m_iCurrentSkin = QUEST_SKIN_NORMAL;
		}
		else
			m_iCurrentSkin = iStyle;
	}

	unsigned int CQuestManager::LoadTimerScript(const string& name)
	{
		map<string, unsigned int>::iterator it;
		if ((it = m_mapTimerID.find(name)) != m_mapTimerID.end())
		{
			return it->second;
		}
		else
		{
			unsigned int new_id = UINT_MAX - m_mapTimerID.size();

			m_mapNPC[new_id].Set(new_id, name);
			m_mapTimerID.insert(make_pair(name, new_id));

			return new_id;
		}
	}

	unsigned int CQuestManager::GetCurrentNPCRace()
	{
		return GetCurrentNPCCharacterPtr() ? GetCurrentNPCCharacterPtr()->GetRaceNum() : 0;
	}

	LPITEM CQuestManager::GetCurrentItem()
	{
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestItemPtr() : NULL;
	}

	void CQuestManager::ClearCurrentItem()
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->ClearQuestItemPtr();
	}

	void CQuestManager::SetCurrentItem(LPITEM item)
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->SetQuestItemPtr(item);
	}

#ifdef ENABLE_QUEST_DND_EVENT
	LPITEM CQuestManager::GetCurrentDNDItem()
	{
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestDNDItemPtr() : NULL;
	}

	void CQuestManager::ClearCurrentDNDItem()
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->ClearQuestDNDItemPtr();
	}

	void CQuestManager::SetCurrentDNDItem(LPITEM item)
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->SetQuestDNDItemPtr(item);
	}
#endif

	LPCHARACTER CQuestManager::GetCurrentNPCCharacterPtr()
	{
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestNPC() : NULL;
	}

	const string& CQuestManager::GetCurrentQuestName()
	{
		return GetCurrentPC()->GetCurrentQuestName();
	}

	LPDUNGEON CQuestManager::GetCurrentDungeon()
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();

		if (!ch)
		{
			if (m_pSelectedDungeon)
				return m_pSelectedDungeon;
			return NULL;
		}

		return ch->GetDungeonForce();
	}

	void CQuestManager::RegisterQuest(const string& stQuestName, unsigned int idx)
	{
		assert(idx > 0);

		itertype(m_hmQuestName) it;

		if ((it = m_hmQuestName.find(stQuestName)) != m_hmQuestName.end())
			return;

		m_hmQuestName.insert(make_pair(stQuestName, idx));
		LoadStartQuest(stQuestName, idx);
		m_mapQuestNameByIndex.insert(make_pair(idx, stQuestName));
	}

	unsigned int CQuestManager::GetQuestIndexByName(const string& name)
	{
		THashMapQuestName::iterator it = m_hmQuestName.find(name);

		if (it == m_hmQuestName.end())
			return 0; // RESERVED

		return it->second;
	}

	const string& CQuestManager::GetQuestNameByIndex(unsigned int idx)
	{
		itertype(m_mapQuestNameByIndex) it;

		if ((it = m_mapQuestNameByIndex.find(idx)) == m_mapQuestNameByIndex.end())
		{
			sys_err("cannot find quest name by index %u", idx);
			assert(!"cannot find quest name by index");

			static std::string st = "";
			return st;
		}

		return it->second;
	}

	void CQuestManager::SendEventFlagList(LPCHARACTER ch)
	{
		itertype(m_mapEventFlag) it;
		for (it = m_mapEventFlag.begin(); it != m_mapEventFlag.end(); ++it)
		{
			const string& flagname = it->first;
			int value = it->second;

			ch->ChatPacket(CHAT_TYPE_INFO, "%s %d", flagname.c_str(), value);
		}
	}

	void CQuestManager::RequestSetEventFlag(const string& name, int value)
	{
		TPacketSetEventFlag p;
		strlcpy(p.szFlagName, name.c_str(), sizeof(p.szFlagName));
		p.lValue = value;
		db_clientdesc->DBPacket(HEADER_GD_SET_EVENT_FLAG, 0, &p, sizeof(TPacketSetEventFlag));
	}

	void CQuestManager::SetEventFlag(const string& name, int value)
	{
		m_mapEventFlag[name] = value;

		if (name == "mob_item")
		{
			CHARACTER_MANAGER::instance().SetMobItemRate(value);
		}
		else if (name == "mob_dam")
		{
			CHARACTER_MANAGER::instance().SetMobDamageRate(value);
		}
		else if (name == "mob_gold")
		{
			CHARACTER_MANAGER::instance().SetMobGoldAmountRate(value);
		}
		else if (name == "mob_gold_pct")
		{
			CHARACTER_MANAGER::instance().SetMobGoldDropRate(value);
		}
		else if (name == "user_dam")
		{
			CHARACTER_MANAGER::instance().SetUserDamageRate(value);
		}
		else if (name == "user_dam_buyer")
		{
			CHARACTER_MANAGER::instance().SetUserDamageRatePremium(value);
		}
		else if (name == "mob_exp")
		{
			CHARACTER_MANAGER::instance().SetMobExpRate(value);
		}
		else if (name == "mob_item_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobItemRatePremium(value);
		}
		else if (name == "mob_exp_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobExpRatePremium(value);
		}
		else if (name == "mob_gold_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobGoldAmountRatePremium(value);
		}
		else if (name == "mob_gold_pct_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobGoldDropRatePremium(value);
		}
#ifdef ENABLE_VOICE_CHAT
		else if (name == "voice_chat_disable")
		{
			bool disabled = (bool)value;
			if (g_bVoiceChatDisabled == disabled)
				return;

			g_bVoiceChatDisabled = disabled;
		}
#endif
	}

	int	CQuestManager::GetEventFlag(const string& name)
	{
		map<string, int>::iterator it = m_mapEventFlag.find(name);

		if (it == m_mapEventFlag.end())
			return 0;

		return it->second;
	}

	void CQuestManager::BroadcastEventFlagOnLogin(LPCHARACTER ch)
	{
		int iEventFlagValue;
		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("day")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
		}
	}

	void CQuestManager::Reload()
	{
		lua_close(L);
		m_mapNPC.clear();
		m_mapNPCNameID.clear();
		m_hmQuestName.clear();
		m_mapTimerID.clear();
		m_hmQuestStartScript.clear();
		m_mapEventName.clear();
		L = NULL;
		Initialize();

		for (itertype(m_registeredNPCVnum) it = m_registeredNPCVnum.begin(); it != m_registeredNPCVnum.end(); ++it)
		{
			char buf[256];
			DWORD dwVnum = *it;
			snprintf(buf, sizeof(buf), "%u", dwVnum);
			m_mapNPC[dwVnum].Set(dwVnum, buf);
		}
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, DWORD quest_index, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		return ExecuteQuestScript(pc, CQuestManager::instance().GetQuestNameByIndex(quest_index), state, code, code_size, pChatScripts, bUseCache);
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, const string& quest_name, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		QuestState qs = CQuestManager::instance().OpenState(quest_name, state);
		if (pChatScripts)
			qs.chat_scripts.swap(*pChatScripts);

		if (bUseCache)
		{
			lua_getglobal(qs.co, "__codecache");
			// stack : __codecache
			lua_pushnumber(qs.co, (long)code);
			// stack : __codecache (codeptr)
			lua_rawget(qs.co, -2);
			// stack : __codecache (compiled-code)
			if (lua_isnil(qs.co, -1))
			{
				// cache miss

				// load code to lua,
				// save it to cache
				// and only function remain in stack
				lua_pop(qs.co, 1);
				// stack : __codecache
				luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());
				// stack : __codecache (compiled-code)
				lua_pushnumber(qs.co, (long)code);
				// stack : __codecache (compiled-code) (codeptr)
				lua_pushvalue(qs.co, -2);
				// stack : __codecache (compiled-code) (codeptr) (compiled_code)
				lua_rawset(qs.co, -4);
				// stack : __codecache (compiled-code)
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
			else
			{
				// cache hit
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
		}
		else
			luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());

		pc.SetQuest(quest_name, qs);

		QuestState& rqs = *pc.GetRunningQuestState();
		if (!CQuestManager::instance().RunState(rqs))
		{
			CQuestManager::instance().CloseState(rqs);
			pc.EndRunning();
			return false;
		}
		return true;
	}

	void CQuestManager::RegisterNPCVnum(DWORD dwVnum)
	{
		if (m_registeredNPCVnum.find(dwVnum) != m_registeredNPCVnum.end())
			return;

		m_registeredNPCVnum.insert(dwVnum);

		char buf[256];
		DIR* dir;

		for (itertype(g_setQuestObjectDir) it = g_setQuestObjectDir.begin(); it != g_setQuestObjectDir.end(); ++it)
		{
			const string& stQuestObjectDir = *it;
			snprintf(buf, sizeof(buf), "%s/%u", stQuestObjectDir.c_str(), dwVnum);

			if ((dir = opendir(buf)))
			{
				closedir(dir);
				snprintf(buf, sizeof(buf), "%u", dwVnum);

				m_mapNPC[dwVnum].Set(dwVnum, buf);
			}
		}
	}

	void CQuestManager::WriteRunningStateToSyserr()
	{
		const char* state_name = GetQuestStateName(GetCurrentQuestName(), GetCurrentState()->st);

		string event_index_name = "";
		for (itertype(m_mapEventName) it = m_mapEventName.begin(); it != m_mapEventName.end(); ++it)
		{
			if (it->second == m_iRunningEventIndex)
			{
				event_index_name = it->first;
				break;
			}
		}

		sys_err("LUA_ERROR: quest %s.%s %s", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str());
	}

#ifndef __WIN32__
	void CQuestManager::QuestError(const char* func, int line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		_sys_err(func, line, "%s", szMsg);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d]", func, line);
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#else
	void CQuestManager::QuestError(const char* func, int line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		_sys_err(func, line, "%s", szMsg);
	}
#endif

	void CQuestManager::AddServerTimer(const std::string& name, DWORD arg, LPEVENT event)
	{
		if (m_mapServerTimer.find(make_pair(name, arg)) != m_mapServerTimer.end())
		{
			sys_err("already registered server timer name:%s arg:%u", name.c_str(), arg);
			return;
		}
		m_mapServerTimer.insert(make_pair(make_pair(name, arg), event));
	}

	void CQuestManager::ClearServerTimerNotCancel(const std::string& name, DWORD arg)
	{
		m_mapServerTimer.erase(make_pair(name, arg));
	}

	void CQuestManager::ClearServerTimer(const std::string& name, DWORD arg)
	{
		itertype(m_mapServerTimer) it = m_mapServerTimer.find(make_pair(name, arg));
		if (it != m_mapServerTimer.end())
		{
			LPEVENT event = it->second;
			event_cancel(&event);
			m_mapServerTimer.erase(it);
		}
	}

	void CQuestManager::CancelServerTimers(DWORD arg)
	{
		itertype(m_mapServerTimer) it = m_mapServerTimer.begin();
		for (; it != m_mapServerTimer.end();) {
			if (it->first.second == arg) {
				LPEVENT event = it->second;
				event_cancel(&event);
				m_mapServerTimer.erase(it++);
			}
			else {
				++it;
			}
		}
	}

	void CQuestManager::SetServerTimerArg(DWORD dwArg)
	{
		m_dwServerTimerArg = dwArg;
	}

	DWORD CQuestManager::GetServerTimerArg()
	{
		return m_dwServerTimerArg;
	}

	void CQuestManager::SelectDungeon(LPDUNGEON pDungeon)
	{
		m_pSelectedDungeon = pDungeon;
	}

	bool CQuestManager::PickupItem(unsigned int pc, LPITEM item)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;
			// call script
			SetCurrentItem(item);

			return m_mapNPC[item->GetVnum()].OnPickupItem(*pPC);
		}
		else
		{
			sys_err("QUEST PICK_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}
	void CQuestManager::BeginOtherPCBlock(DWORD pid)
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();
		if (NULL == ch)
		{
			sys_err("NULL?");
			return;
		}

		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = GetCurrentPC();
		}
		m_vecPCStack.push_back(GetCurrentCharacterPtr()->GetPlayerID());
		GetPC(pid);
	}

	void CQuestManager::EndOtherPCBlock()
	{
		if (m_vecPCStack.size() == 0)
		{
			sys_err("m_vecPCStack is alread empty. CurrentQuest{Name(%s), State(%s)}", GetCurrentQuestName().c_str(), GetCurrentState()->_title.c_str());
			return;
		}
		DWORD pc = m_vecPCStack.back();
		m_vecPCStack.pop_back();
		GetPC(pc);

		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = NULL;
		}
	}

	bool CQuestManager::IsInOtherPCBlock()
	{
		return !m_vecPCStack.empty();
	}

	PC* CQuestManager::GetOtherPCBlockRootPC()
	{
		return m_pOtherPCBlockRootPC;
	}
}