#include "stdafx.h"
#ifdef ENABLE_HWID
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "ClientManager.h"

bool CClientManager::SetHwidID(TPacketGDAuthLogin* p, BYTE hwidNum)
{
	CDBManager& rDBmgr = CDBManager::instance();
	char szQuery[256];
	char escape_hwd[64 + 1];
	DWORD hwidID = 0;
	rDBmgr.EscapeString(escape_hwd, p->hwid, strlen(p->hwid));

	snprintf(szQuery, sizeof(szQuery),
		"SELECT id FROM account.hwid_list WHERE hwid = '%s' ", escape_hwd);

	std::unique_ptr<SQLMsg> msg(rDBmgr.DirectQuery(szQuery));
	SQLResult* res = msg->Get();

	if (res && res->uiNumRows > 0)
	{
		MYSQL_ROW row;
		row = mysql_fetch_row(res->pSQLResult);
		str_to_number(hwidID, row[0]);
	}
	else
	{
		snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO account.hwid_list (id, hwid) VALUES (0, '%s')", escape_hwd);
		std::unique_ptr<SQLMsg> msg2(rDBmgr.DirectQuery(szQuery));
		SQLResult* res2 = msg2->Get();
		if (res2 && res2->uiInsertID > 0)
		{
			hwidID = res2->uiInsertID;
			m_hwidIDMap.insert(std::make_pair(hwidID, p->hwid));
		}
		else
		{
			return false;
		}
	}
	
	if (hwidNum > 0)
	{
		snprintf(szQuery, sizeof(szQuery),
			"UPDATE account.account SET hwidID2 = %u WHERE id = %u", hwidID, p->dwID);
	}
	else
	{
		snprintf(szQuery, sizeof(szQuery),
			"UPDATE account.account SET hwidID = %u WHERE id = %u", hwidID, p->dwID);
	}

	rDBmgr.DirectQuery(szQuery);
	p->hwidID[hwidNum] = hwidID;
	p->descHwidID = hwidID;
	return true;
}

BYTE CClientManager::CheckHwid(TPacketGDAuthLogin* p)
{
	{
		std::set<std::string>::const_iterator it = m_hwidBanList.find(p->hwid);
		if (it != m_hwidBanList.end())
			return 4;
	}

	HWIDIDMAP::const_iterator it = m_hwidIDMap.find(p->hwidID[0]);
	if (it == m_hwidIDMap.end())
	{
		sys_err("hwid ID map not find ID:%u", p->hwidID[0]);
		return 2;
	}

	char szHwid[64 + 1];
	snprintf(szHwid, sizeof(szHwid), it->second.c_str());

	if (strcmp(p->hwid, szHwid) != 0)
	{
		if (p->hwidID[1] == 0)
		{
			if (!SetHwidID(p, 1))
			{
				return 2;
			}
		}
		else
		{
			HWIDIDMAP::const_iterator it2 = m_hwidIDMap.find(p->hwidID[1]);
			if (it2 == m_hwidIDMap.end())
			{
				sys_err("hwid ID map not find ID:%u", p->hwidID[1]);
				return 2;
			}
			snprintf(szHwid, sizeof(szHwid), it2->second.c_str());
			if (strcmp(p->hwid, szHwid) != 0)
			{
				return 3;
			}
			p->descHwidID = p->hwidID[1];
		}
	}
	else
	{
		p->descHwidID = p->hwidID[0];
	}
	return 0;
}

void CClientManager::InitializeHwid()
{
	MYSQL_ROW row;
	std::unique_ptr<SQLMsg> hwidBan(CDBManager::instance().DirectQuery
		("SELECT hwid FROM account.hwid_ban_list "));

	if (hwidBan->uiSQLErrno == 0 && hwidBan->Get())
	{
		while ((row = mysql_fetch_row(hwidBan->Get()->pSQLResult)))
		{
			m_hwidBanList.insert(row[0]);
		}
	}

	std::unique_ptr<SQLMsg> hwidList(CDBManager::instance().DirectQuery
		("SELECT id, hwid FROM account.hwid_list "));

	if (hwidList->uiSQLErrno == 0 && hwidList->Get())
	{
		while ((row = mysql_fetch_row(hwidList->Get()->pSQLResult)))
		{
			BYTE col = 0;
			DWORD hwidID = 0;
			str_to_number(hwidID, row[col++]);
			m_hwidIDMap.insert(std::make_pair(hwidID, row[col++]));
		}
	}
}

void CClientManager::RecvHwidPacketGD(CPeer* peer, const char* data)
{
	THwidGDPacket* pack;
	data = InGameLog::Decode(pack, data);

	if (!pack)
		return;

	switch (pack->subHeader)
	{
		case SUB_HEADER_GD_HWID_BAN: { RecvHwidBanGD(data); break; }
#ifdef ENABLE_FARM_BLOCK
		case SUB_HEADER_GD_GET_FARM_BLOCK: { RecvGetFarmBlockGD(peer, data); break; }
		case SUB_HEADER_GD_SET_FARM_BLOCK: { RecvSetFarmBlockGD(peer, data); break; }
		case SUB_HEADER_GD_FARM_BLOCK_LOGOUT: { RecvFarmBlockLogout(data); break; }
#endif
		default: { sys_err("Unknow recovery packet GD %d", pack->subHeader); break; }
	}
}

void CClientManager::RecvHwidBanGD(const char* data)
{
	THwidString* subPack;
	data = InGameLog::Decode(subPack, data);
	m_hwidBanList.insert(subPack->hwid);
}

#ifdef ENABLE_FARM_BLOCK
void CClientManager::RecvGetFarmBlockGD(CPeer* peer, const char* data)
{
	TFarmBlockGet* subPack;
	data = InGameLog::Decode(subPack, data);

	bool farmBlock = false;
	FARM_BLOCK_MAP::iterator it = m_mapFarmBlock.find(subPack->descHwidID);
	if (it != m_mapFarmBlock.end())
	{
		FARM_BLOCK_INFO::iterator it2 = it->second.find(subPack->pid);
		if (it2 != it->second.end())
		{
			farmBlock = it2->second;
		}
		else
		{
			if (it->second.size() > 1)
			{
				BYTE deactivateBlock = 0;
				for (it2 = it->second.begin(); it2 != it->second.end(); it2++)
				{
					if (it2->second == false)
					{
						deactivateBlock++;
						if (deactivateBlock == 2)
						{
							farmBlock = true;
							break;
						}
					}
				}
			}
			it->second.insert(std::make_pair(subPack->pid, farmBlock));
		}
	}
	else
	{
		FARM_BLOCK_INFO info;
		info.insert(std::make_pair(subPack->pid, farmBlock));
		m_mapFarmBlock.insert(std::make_pair(subPack->descHwidID, info));
	}

	{
		THwidDGPacket sendPack;
		sendPack.subHeader = SUB_HEADER_DG_GET_FARM_BLOCK;

		TFarmBlockGD sendSubPack;
		sendSubPack.farmBlock = farmBlock;
		sendSubPack.pid = subPack->pid;
		sendSubPack.result = 0;
		peer->EncodeHeader(HEADER_DG_HWID, 0, sizeof(THwidDGPacket) + sizeof(TFarmBlockGD));
		peer->Encode(&sendPack, sizeof(THwidDGPacket));
		peer->Encode(&sendSubPack, sizeof(TFarmBlockGD));
	}
}

void CClientManager::RecvSetFarmBlockGD(CPeer* peer, const char* data)
{
	TChangeFarmBlock* subPack;
	data = InGameLog::Decode(subPack, data);

	BYTE result = 4;

	FARM_BLOCK_MAP::iterator it = m_mapFarmBlock.find(subPack->info.descHwidID);
	if (it != m_mapFarmBlock.end())
	{
		FARM_BLOCK_INFO::iterator it2 = it->second.find(subPack->info.pid);
		if (it2 != it->second.end())
		{
			if (subPack->farmBlock)
			{
				it2->second = true;
				result = 1;
			}
			else
			{
				if (it->second.size() > 1)
				{
					BYTE deactivateBlock = 0;
					for (FARM_BLOCK_INFO::iterator it3 = it->second.begin(); it3 != it->second.end(); it3++)
					{
						if (!it3->second)
						{
							deactivateBlock++;
							if (deactivateBlock == 2)
							{
								result = 3;
								break;
							}
						}
					}
				}
				if (result != 3)
				{
					it2->second = false;
					result = 2;
				}
			}
		}
	}

	{
		THwidDGPacket sendPack;
		sendPack.subHeader = SUB_HEADER_DG_GET_FARM_BLOCK;

		TFarmBlockGD sendSubPack;
		sendSubPack.farmBlock = subPack->farmBlock;
		sendSubPack.pid = subPack->info.pid;
		sendSubPack.result = result;
		peer->EncodeHeader(HEADER_DG_HWID, 0, sizeof(THwidDGPacket) + sizeof(TFarmBlockGD));
		peer->Encode(&sendPack, sizeof(THwidDGPacket));
		peer->Encode(&sendSubPack, sizeof(TFarmBlockGD));
	}
}

void CClientManager::RecvFarmBlockLogout(const char* data)
{
	TFarmBlockGet* subPack;
	data = InGameLog::Decode(subPack, data);

	FARM_BLOCK_MAP::iterator it = m_mapFarmBlock.find(subPack->descHwidID);
	if (it != m_mapFarmBlock.end())
	{
		FARM_BLOCK_INFO::iterator it2 = it->second.find(subPack->pid);
		if (it2 != it->second.end())
		{
			it->second.erase(subPack->pid);
		}
	}
}
#endif

#endif