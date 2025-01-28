#include "stdafx.h"
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLog.h"
#include "QID.h"
#include "DBManager.h"
#include "ClientManager.h"
#include "Peer.h"
#include <string.h>
#define DIRECT_QUERY(query) CDBManager::instance().DirectQuery((query))
#pragma GCC diagnostic ignored "-Wstringop-truncation"
namespace InGameLog
{
/**************************************************************
					CLASS: CacheOfflineShopLog 
**************************************************************/
	CacheOfflineShopLog::CacheOfflineShopLog()
	{
		m_soldMap.clear();
	}

	CacheOfflineShopLog::~CacheOfflineShopLog()
	{
		m_soldMap.clear();
	}

	void CacheOfflineShopLog::PutItem(DWORD ownerID, const TOfflineShopSoldLog& log)
	{
		SOLD_ITER it = m_soldMap.find(ownerID);
		if (it == m_soldMap.end())
		{
			SOLD_ITEM_VEC vec;
			vec.push_back(log);
			m_soldMap.insert(std::make_pair(ownerID, vec));
		}
		else
		{
			SOLD_ITEM_VEC& vec = it->second;
			vec.push_back(log);
		}
	}

	bool CacheOfflineShopLog::GetLog(DWORD ownerID, SOLD_ITEM_VEC& soldItems) const
	{
		SOLD_CONST_ITER it = m_soldMap.find(ownerID);
		if (it == m_soldMap.end())
			return false;

		soldItems = it->second;
		return true;
	}

	void CacheOfflineShopLog::AddItem(DWORD ownerID, const TOfflineShopSoldLog& log)
	{
		SOfflineShopSoldQuery* qi = new SOfflineShopSoldQuery;//erased in the RecvOfflineShopRequestLog
		qi->ownerID = ownerID;
		CopyObject(qi->log, log);
		std::unique_ptr<SQLMsg> msg(DIRECT_QUERY("SELECT NOW()"));
		if (msg->Get())
		{
			MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
			strlcpy(qi->log.soldDate, row[0], sizeof(qi->log.soldDate));
		}
		std::string query = InsertSoldItemLogQuery(ownerID, log);
		CDBManager::instance().ReturnQuery(query.c_str(), QID_IN_GAME_LOG_OFFLINESHOP_ADD_ITEM, 0, qi);
	}

	std::string CacheOfflineShopLog::InsertSoldItemLogQuery(DWORD ownerID, const TOfflineShopSoldLog& log)
	{
		char szQuery[1024] = "INSERT INTO log.offlineshop_sold (owner_id, buyer_name, sell_price, sell_date, vnum, count ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",socket%d ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",attr%d ,attrval%d ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, ") VALUES ( ");

		len += snprintf(szQuery + len, sizeof(szQuery) - len, "%u, '%s', %llu, NOW(), %u, %u ",
			ownerID, log.buyerName, log.sellPrice, log.item.dwVnum, log.item.dwCount);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",%ld ", log.item.alSockets[i]);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",%d ,%d ", log.item.aAttr[i].bType, log.item.aAttr[i].sValue);

		std::string query = szQuery;
		query += ")";

		return query;
	}

	std::string CacheOfflineShopLog::SelectSoldItemLogQuery()
	{
		char szQuery[512] = "SELECT owner_id, buyer_name, sell_date, sell_price, vnum, count ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",socket%d ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",attr%d ,attrval%d ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, " FROM log.offlineshop_sold ");
		return szQuery;
	}


/**************************************************************
					CLASS: CacheTradeLog
**************************************************************/

	CacheTradeLog::CacheTradeLog()
	{
		m_tradeInfoMap.clear();
		m_tradeItemMap.clear();
		m_tradeLogIdMap.clear();
	}

	CacheTradeLog::~CacheTradeLog()
	{
		m_tradeInfoMap.clear();
		m_tradeItemMap.clear();
		m_tradeLogIdMap.clear();
	}

	void CacheTradeLog::PutInfoLog(const TTradeLogInfo& log)
	{
		PutPIDtoLogID(log.owner.pid, log.victim.pid, log.logID);
		m_tradeInfoMap.insert(std::make_pair(log.logID, log));
	}

	void CacheTradeLog::PutItemLog(DWORD logID, const TTradeLogItemInfo& log)
	{
		TRADE_ITEM_ITER it = m_tradeItemMap.find(logID);
		if (it == m_tradeItemMap.end())
		{
			TRADE_ITEM_VEC vec;
			vec.push_back(log);
			m_tradeItemMap.insert(std::make_pair(logID, vec));
		}
		else
		{
			TRADE_ITEM_VEC& vec = it->second;
			vec.push_back(log);
		}
	}

	void CacheTradeLog::PutPIDtoLogID(DWORD ownerPID, DWORD victimPID, DWORD logID)
	{
		TRADE_LOG_ID_ITER it = m_tradeLogIdMap.find(ownerPID);
		if (it == m_tradeLogIdMap.end())
		{
			TRADE_LOG_ID_VEC vec;
			vec.push_back(logID);
			m_tradeLogIdMap.insert(std::make_pair(ownerPID, vec));
		}
		else
		{
			TRADE_LOG_ID_VEC& vec = it->second;
			vec.push_back(logID);
		}

		TRADE_LOG_ID_ITER it2 = m_tradeLogIdMap.find(victimPID);
		if (it2 == m_tradeLogIdMap.end())
		{
			TRADE_LOG_ID_VEC vec;
			vec.push_back(logID);
			m_tradeLogIdMap.insert(std::make_pair(victimPID, vec));
		}
		else
		{
			TRADE_LOG_ID_VEC& vec = it2->second;
			vec.push_back(logID);
		}
	}

	bool CacheTradeLog::GetInfoLog(DWORD requestPID, std::vector<TTradeLogRequestInfo>& log)
	{
		TRADE_LOG_ID_CONST_ITER it = m_tradeLogIdMap.find(requestPID);
		if (it == m_tradeLogIdMap.end())
			return false;
		
		log.reserve(it->second.size());
		for (auto& rLogId : it->second)
		{
			TRADE_INFO_CONST_ITER it2 = m_tradeInfoMap.find(rLogId);
			if (it2 == m_tradeInfoMap.end())
				break;

			TTradeLogRequestInfo requestInfo;
			requestInfo.logID = rLogId;
			snprintf(requestInfo.tradingDate, sizeof(requestInfo.tradingDate), it2->second.tradingDate);
			
			if (requestPID != it2->second.owner.pid)
			{
				snprintf(requestInfo.victimName, sizeof(requestInfo.victimName), it2->second.owner.name);
			}
			else
			{
				snprintf(requestInfo.victimName, sizeof(requestInfo.victimName), it2->second.victim.name);
			}
			log.push_back(requestInfo);
		}

		if (log.size() < 1)
			return false;

		return true;
	}

	bool CacheTradeLog::GetDetailsLog(DWORD requestPID, TTradeLogDetailsRequest& tradeInfo, std::vector<TTradeLogRequestItem>& tradeItem)
	{
		TRADE_INFO_CONST_ITER it = m_tradeInfoMap.find(tradeInfo.logID);
		if (it == m_tradeInfoMap.end())
			return false;

		if (requestPID != it->second.owner.pid)
		{
			tradeInfo.ownerGold = it->second.victim.gold;
			tradeInfo.victimGold = it->second.owner.gold;
		}
		else
		{
			tradeInfo.victimGold = it->second.victim.gold;
			tradeInfo.ownerGold = it->second.owner.gold;
		}

		TRADE_ITEM_CONST_ITER it2 = m_tradeItemMap.find(tradeInfo.logID);
		if (it2 == m_tradeItemMap.end())
			return false;

		tradeInfo.itemCount = it2->second.size();
		tradeItem.reserve(tradeInfo.itemCount);

		for (auto& rItem : it2->second)
		{
			TTradeLogRequestItem requestItem;
			CopyObject(requestItem.item, rItem.item);
			requestItem.pos = rItem.pos;
			if (rItem.ownerID == requestPID)
			{
				requestItem.isOwner = true;
			}
			else
			{
				requestItem.isOwner = false;
			}
			tradeItem.push_back(requestItem);
		}
		return true;
	}

	void CacheTradeLog::AddNewLog(const TTradeLogInfoAdd& log, const TRADE_ITEM_VEC& item)
	{
		std::string query = InsertTradeInfoLogQuery(log);
		std::unique_ptr<SQLMsg> msg (DIRECT_QUERY(query.c_str()));

		if (msg->uiSQLErrno != 0)
			return;

		if (msg->Get())
		{
			UINT logID = msg->Get()->uiInsertID;
			TTradeLogInfo loginfo;
			ZeroObject(loginfo);
			CopyObject(loginfo.owner, log.owner);
			CopyObject(loginfo.victim, log.victim);

			loginfo.logID = logID;
			strncpy(loginfo.tradingDate, GetNewLogDate(logID), sizeof(loginfo.tradingDate));
			PutInfoLog(loginfo);

			for (const TTradeLogItemInfo& it : item)
			{
				std::string itemquery = InsertTradeItemLogQuery(it, logID);
				PutItemLog(logID, it);
				DIRECT_QUERY(itemquery.c_str());
			}
		}
	}

	std::string CacheTradeLog::SelectTradeInfoLogQuery()
	{
		char szQuery[512] = "SELECT log_id, trade_date, owner_id, owner_name, owner_gold, victim_id, victim_name, victim_gold FROM log.trade_info_log";
		return szQuery;
	}

	std::string CacheTradeLog::SelectTradeItemLogQuery()
	{
		char szQuery[512] = "SELECT log_id, owner_id, pos, vnum, count ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",socket%d ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",attr%d ,attrval%d ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, " FROM log.trade_item_log ");

		return szQuery;
	}

	std::string CacheTradeLog::InsertTradeInfoLogQuery(const TTradeLogInfoAdd& log)
	{
		char szQuery[512] = "INSERT INTO log.trade_info_log (log_id, trade_date, owner_id, owner_name, owner_gold, victim_id, victim_name, victim_gold ";
		size_t len = strlen(szQuery);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, ") VALUES ( ");

		len += snprintf(szQuery + len, sizeof(szQuery) - len, "0, NOW(), %u, '%s', %llu, %u, '%s', %llu ",
			log.owner.pid, log.owner.name, log.owner.gold, log.victim.pid, log.victim.name, log.victim.gold);

		std::string query = szQuery;
		query += ")";

		return query;
	}

	std::string CacheTradeLog::InsertTradeItemLogQuery(const TTradeLogItemInfo& log, UINT logID)
	{
		char szQuery[1024] = "INSERT INTO log.trade_item_log (log_id, owner_id, pos, vnum, count ";
		size_t len = strlen(szQuery);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",socket%d ", i);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",attr%d ,attrval%d ", i, i);

		len += snprintf(szQuery + len, sizeof(szQuery) - len, ") VALUES ( ");

		len += snprintf(szQuery + len, sizeof(szQuery) - len, "%u, %u, %d, %u, %u ",
			logID,log.ownerID, log.pos, log.item.dwVnum, log.item.dwCount);

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",%ld ", log.item.alSockets[i]);

		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
			len += snprintf(szQuery + len, sizeof(szQuery) - len,
				",%d ,%d ", log.item.aAttr[i].bType, log.item.aAttr[i].sValue);

		std::string query = szQuery;
		query += ")";

		return query;
	}

	const char* CacheTradeLog::GetNewLogDate(const DWORD logID)
	{
		char szQuery[128];
		snprintf(szQuery, sizeof(szQuery), "SELECT trade_date FROM log.trade_info_log WHERE log_id = %u", logID);
		std::unique_ptr<SQLMsg> msg (DIRECT_QUERY(szQuery));
		if (msg->Get())
		{
			MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
			return row[0];
		}
		return " ";
	}
}//namespace


/**************************************************************
						CLIENT MANAGER
**************************************************************/
bool CClientManager::InitializeInGameLogTable()
{
	MYSQL_ROW row;
	//OFFLINE SHOP SOLD LOG
	{
		DIRECT_QUERY("DELETE FROM log.offlineshop_sold WHERE DATE_SUB(NOW(), INTERVAL 5 DAY) > sell_date");

		std::string query = m_ingamelogOfflineShop.SelectSoldItemLogQuery();
		std::unique_ptr<SQLMsg> pMsg(DIRECT_QUERY(query.c_str()));

		if (pMsg->uiSQLErrno != 0)
		{
			sys_err("CANNOT LOAD offlineshop_sold TABLE , errorcode %d ", pMsg->uiSQLErrno);
			return false;
		}

		if (pMsg->Get())
		{
			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int col = 0;
				InGameLog::TOfflineShopSoldLog log;
				InGameLog::ZeroObject(log);

				//owner_id, buyer_name, sell_price, sell_date, vnum, count
				DWORD ownerID = 0;
				str_to_number(ownerID, row[col++]);
				strlcpy(log.buyerName, row[col++], sizeof(log.buyerName));
				strlcpy(log.soldDate, row[col++], sizeof(log.soldDate));
				str_to_number(log.sellPrice, row[col++]);
				str_to_number(log.item.dwVnum, row[col++]);
				str_to_number(log.item.dwCount, row[col++]);

				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
					str_to_number(log.item.alSockets[i], row[col++]);

				for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
				{
					str_to_number(log.item.aAttr[i].bType, row[col++]);
					str_to_number(log.item.aAttr[i].sValue, row[col++]);
				}
				m_ingamelogOfflineShop.PutItem(ownerID, log);
			}
		}
	}

	//TRADE INFO LOG
	{
		std::unique_ptr<SQLMsg> pDeleteMsg(DIRECT_QUERY
			("SELECT log_id FROM log.trade_info_log WHERE DATE_SUB(NOW(), INTERVAL 5 DAY) > trade_date"));

		if (pDeleteMsg->uiSQLErrno == 0 && pDeleteMsg->Get())
		{
			std::vector<DWORD> deleteLogID;
			deleteLogID.clear();

			while ((row = mysql_fetch_row(pDeleteMsg->Get()->pSQLResult)))
			{
				DWORD logid = 0;
				str_to_number(logid, row[0]);
				deleteLogID.push_back(logid);

			}

			std::unique_ptr<SQLMsg> pDeleteMsg2(DIRECT_QUERY
				("DELETE FROM log.trade_info_log WHERE DATE_SUB(NOW(), INTERVAL 5 DAY) > trade_date"));

			char query[256];
			for (auto& it : deleteLogID)
			{
				snprintf(query, sizeof(query), "DELETE FROM log.trade_item_log WHERE log_id = % d", it);
				std::unique_ptr<SQLMsg> pDeleteMsg3(DIRECT_QUERY(query));
			}
		}

		std::string query = m_ingamelogTrade.SelectTradeInfoLogQuery();
		std::unique_ptr<SQLMsg> pMsg(DIRECT_QUERY(query.c_str()));

		if (pMsg->uiSQLErrno != 0)
		{
			sys_err("CANNOT LOAD trade_info_log TABLE , errorcode %d ", pMsg->uiSQLErrno);
			return false;
		}

		if (pMsg->Get())
		{
			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int col = 0;
				InGameLog::TTradeLogInfo log;
				InGameLog::ZeroObject(log);

				//log_id, trade_date, owner_id, owner_name, owner_gold, victim_id, victim_name
				str_to_number(log.logID, row[col++]);
				strlcpy(log.tradingDate, row[col++], sizeof(log.tradingDate));

				str_to_number(log.owner.pid, row[col++]);
				strlcpy(log.owner.name, row[col++], sizeof(log.owner.name));
				str_to_number(log.owner.gold, row[col++]);

				str_to_number(log.victim.pid, row[col++]);
				strlcpy(log.victim.name, row[col++], sizeof(log.victim.name));
				str_to_number(log.victim.gold, row[col++]);

				m_ingamelogTrade.PutInfoLog(log);
			}
		}
	}

	//TRADE ITEM LOG
	{
		std::string query = m_ingamelogTrade.SelectTradeItemLogQuery();
		std::unique_ptr<SQLMsg> pMsg(DIRECT_QUERY(query.c_str()));

		if (pMsg->uiSQLErrno != 0)
		{
			sys_err("CANNOT LOAD trade_item_log TABLE , errorcode %d ", pMsg->uiSQLErrno);
			return false;
		}

		if (pMsg->Get())
		{
			while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
			{
				int col = 0;
				InGameLog::TTradeLogItemInfo log;
				InGameLog::ZeroObject(log);

				//log_id, owner_id, pos, vnum, count
				DWORD logID = 0;
				str_to_number(logID, row[col++]);
				str_to_number(log.ownerID, row[col++]);
				str_to_number(log.pos, row[col++]);

				str_to_number(log.item.dwVnum, row[col++]);
				str_to_number(log.item.dwCount, row[col++]);

				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
					str_to_number(log.item.alSockets[i], row[col++]);

				for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
				{
					str_to_number(log.item.aAttr[i].bType, row[col++]);
					str_to_number(log.item.aAttr[i].sValue, row[col++]);
				}
				m_ingamelogTrade.PutItemLog(logID, log);
			}
		}
	}
	return true;
}

/**************************************************************
							QID
**************************************************************/
void CClientManager::ResultOfflineShopLog(CQueryInfo* pQueryInfo)
{
	InGameLog::SOfflineShopSoldQuery* qi = (InGameLog::SOfflineShopSoldQuery*)pQueryInfo->pvData;
	m_ingamelogOfflineShop.PutItem(qi->ownerID, qi->log);
	delete(qi);
}

/**************************************************************
						GD PACKET
**************************************************************/
void CClientManager::RecvInGameLogPacketGD(CPeer* peer, const char* data)
{
	using namespace InGameLog;
	TPacketGDInGameLog* pack;
	data = Decode(pack, data);

	if (!pack)
		return;

	switch (pack->subHeader)
	{
		case SUBHEADER_IGL_GD_OFFLINESHOP_ADDLOG: { RecvOfflineShopAddLogGD(data); break; }
		case SUBHEADER_IGL_GD_OFFLINESHOP_REQUESTLOG: { RecvOfflineShopRequestLogGD(peer, data); break; }
		case SUBHEADER_IGL_GD_TRADE_ADDLOG: { RecvTradeAddLogGD(data); break; }
		case SUBHEADER_IGL_GD_TRADE_REQUESTLOG: { RecvTradeLogRequestGD(peer, data); break; }
		case SUBHEADER_IGL_GD_TRADE_DETAILS_REQUESTLOG: { RecvTradeLogDetailsRequestGD(peer, data); break; }

		default: { sys_err("Unknow recovery packet GD %d", pack->subHeader); break; }
	}
}

void CClientManager::RecvOfflineShopAddLogGD(const char* data)
{
	InGameLog::TSubPackOfflineShopAddGD* subPack;
	data = InGameLog::Decode(subPack, data);
	m_ingamelogOfflineShop.AddItem(subPack->ownerID, subPack->log);
}

void CClientManager::RecvOfflineShopRequestLogGD(CPeer* peer, const char* data)
{
	InGameLog::TOfflineShopRequestGD* subPack;
	data = InGameLog::Decode(subPack, data);
	SendOfflineShopRequestLogDG(peer, subPack->ownerID);
}

void CClientManager::RecvTradeAddLogGD(const char* data)
{
	InGameLog::TSubPackTradeAddGD* subPack;
	data = InGameLog::Decode(subPack, data);

	InGameLog::TTradeLogInfoAdd loginfo;
	InGameLog::CopyObject(loginfo.owner, subPack->owner);
	InGameLog::CopyObject(loginfo.victim, subPack->victim);

	InGameLog::CacheTradeLog::TRADE_ITEM_VEC v_item;
	v_item.clear();

	for (int i = 0; i < subPack->itemCount; i++)
	{
		InGameLog::TTradeLogItemInfo* itempack;
		data = InGameLog::Decode(itempack, data);
		v_item.push_back(*itempack);
	}

	m_ingamelogTrade.AddNewLog(loginfo, v_item);
}

void CClientManager::RecvTradeLogRequestGD(CPeer* peer, const char* data)
{
	InGameLog::TTradeRequestGD* subPack;
	data = InGameLog::Decode(subPack, data);
	SendTradeLogRequestDG(peer, subPack->ownerID);
}

void CClientManager::RecvTradeLogDetailsRequestGD(CPeer* peer, const char* data)
{
	InGameLog::TTradeDetailsRequestGD* subPack;
	data = InGameLog::Decode(subPack, data);
	SendTradeLogDetailsRequestDG(peer, *subPack);
}

/**************************************************************
						DG PACKET
**************************************************************/
void CClientManager::SendOfflineShopRequestLogDG(CPeer* peer, DWORD ownerID)
{
	using namespace InGameLog;
	CacheOfflineShopLog::SOLD_ITEM_VEC pvSolds;
	pvSolds.clear();

	TPacketDGInGameLog pack;
	pack.subHeader = SUBHEADER_IGL_DG_OFFLINESHOP_REQUESTLOG;

	TInGameLogRequestDG subpack;
	subpack.ownerID = ownerID;

	if (!m_ingamelogOfflineShop.GetLog(ownerID, pvSolds))
	{
		subpack.logCount = 0;
		peer->EncodeHeader(HEADER_DG_IN_GAME_LOG, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
		return;
	}

	subpack.logCount = pvSolds.size();

	size_t buffersize = sizeof(pack) + sizeof(subpack) + (pvSolds.size() * sizeof(TOfflineShopSoldLog));
	peer->EncodeHeader(HEADER_DG_IN_GAME_LOG, 0, buffersize);
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));

	for (WORD i = 0; i < pvSolds.size(); i++)
	{
		peer->Encode(&pvSolds[i], sizeof(TOfflineShopSoldLog));
	}
}

void CClientManager::SendTradeLogRequestDG(CPeer* peer, DWORD ownerID)
{
	using namespace InGameLog;
	TPacketDGInGameLog pack;
	pack.subHeader = SUBHEADER_IGL_DG_TRADE_REQUESTLOG;

	TInGameLogRequestDG subpack;
	subpack.ownerID = ownerID;

	std::vector<TTradeLogRequestInfo> pvLog;

	if (!m_ingamelogTrade.GetInfoLog(ownerID, pvLog))
	{
		subpack.logCount = 0;
		peer->EncodeHeader(HEADER_DG_IN_GAME_LOG, 0, sizeof(pack) + sizeof(subpack));
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&subpack, sizeof(subpack));
		return;
	}

	subpack.logCount = pvLog.size();

	size_t buffersize = sizeof(pack) + sizeof(subpack) + (pvLog.size() * sizeof(TTradeLogRequestInfo));
	peer->EncodeHeader(HEADER_DG_IN_GAME_LOG, 0, buffersize);
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&subpack, sizeof(subpack));

	for (auto& rvInfo: pvLog)
	{
		peer->Encode(&rvInfo, sizeof(TTradeLogRequestInfo));
	}
}

void CClientManager::SendTradeLogDetailsRequestDG(CPeer* peer, InGameLog::TTradeDetailsRequestGD data)
{
	using namespace InGameLog;
	TPacketDGInGameLog pack;
	pack.subHeader = SUBHEADER_IGL_DG_TRADE_DETAILS_REQUESTLOG;

	DWORD ownerID = data.ownerID;
	TTradeLogDetailsRequest subpack;
	ZeroObject(subpack);
	subpack.logID = data.logID;
	std::vector<TTradeLogRequestItem> pvLog;

	size_t buffersize = sizeof(pack) + sizeof(DWORD) + sizeof(subpack);

	if (!m_ingamelogTrade.GetDetailsLog(ownerID, subpack, pvLog))
	{
		peer->EncodeHeader(HEADER_DG_IN_GAME_LOG, 0, buffersize);
		peer->Encode(&pack, sizeof(pack));
		peer->Encode(&ownerID, sizeof(DWORD));
		peer->Encode(&subpack, sizeof(subpack));
		return;
	}

	buffersize += pvLog.size() * sizeof(TTradeLogRequestItem);

	peer->EncodeHeader(HEADER_DG_IN_GAME_LOG, 0, buffersize);
	peer->Encode(&pack, sizeof(pack));
	peer->Encode(&ownerID, sizeof(DWORD));
	peer->Encode(&subpack, sizeof(subpack));

	for (auto& rvInfo : pvLog)
	{
		peer->Encode(&rvInfo, sizeof(TTradeLogRequestItem));
	}
}
#endif