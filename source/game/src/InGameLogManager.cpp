#include "stdafx.h"
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLogManager.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
namespace InGameLog
{
	InGameLog::InGameLogManager& GetManager()
	{
		return InGameLog::InGameLogManager::instance();
	}

	InGameLogManager::InGameLogManager()
	{
	}

	InGameLogManager::~InGameLogManager()
	{
	}

	//GD PACKET BEGIN
	void InGameLogManager::SendOfflineShopAddItemGD(const TOfflineShopSoldLog& soldlog, DWORD ownerID)
	{
		TPacketGDInGameLog pack;
		pack.subHeader = SUBHEADER_IGL_GD_OFFLINESHOP_ADDLOG;

		TSubPackOfflineShopAddGD subpack;
		ZeroObject(subpack);
		subpack.ownerID = ownerID;
		CopyObject(subpack.log, soldlog);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGDInGameLog));
		buff.write(&subpack, sizeof(TSubPackOfflineShopAddGD));
		db_clientdesc->DBPacket(HEADER_GD_IN_GAME_LOG, 0, buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendOfflineShopRequestGD(DWORD ownerID)
	{
		TPacketGDInGameLog pack;
		pack.subHeader = SUBHEADER_IGL_GD_OFFLINESHOP_REQUESTLOG;

		TOfflineShopRequestGD subpack;
		subpack.ownerID = ownerID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGDInGameLog));
		buff.write(&subpack, sizeof(TOfflineShopRequestGD));
		db_clientdesc->DBPacket(HEADER_GD_IN_GAME_LOG, 0, buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendTradeLogAddGD(const std::vector<TTradeLogItemInfo>& v_itemLog, const TSubPackTradeAddGD& info)
	{
		TPacketGDInGameLog pack;
		pack.subHeader = SUBHEADER_IGL_GD_TRADE_ADDLOG;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGDInGameLog));
		buff.write(&info, sizeof(TSubPackTradeAddGD));

		for (const TTradeLogItemInfo& iteminfo : v_itemLog)
		{
			buff.write(&iteminfo, sizeof(TTradeLogItemInfo));
		}

		db_clientdesc->DBPacket(HEADER_GD_IN_GAME_LOG, 0, buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendTradeLogRequestGD(DWORD ownerID)
	{
		TPacketGDInGameLog pack;
		pack.subHeader = SUBHEADER_IGL_GD_TRADE_REQUESTLOG;

		TTradeRequestGD subpack;
		subpack.ownerID = ownerID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGDInGameLog));
		buff.write(&subpack, sizeof(TTradeRequestGD));
		db_clientdesc->DBPacket(HEADER_GD_IN_GAME_LOG, 0, buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendTradeLogDetailsRequestGD(DWORD ownerID, DWORD logID)
	{
		TPacketGDInGameLog pack;
		pack.subHeader = SUBHEADER_IGL_GD_TRADE_DETAILS_REQUESTLOG;

		TTradeDetailsRequestGD subpack;
		subpack.ownerID = ownerID;
		subpack.logID = logID;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGDInGameLog));
		buff.write(&subpack, sizeof(TTradeDetailsRequestGD));
		db_clientdesc->DBPacket(HEADER_GD_IN_GAME_LOG, 0, buff.read_peek(), buff.size());
	}
	//GD PACKET END

	//DG PACKET BEGIN
	void InGameLogManager::RecvInGameLogPacketDG(const char* data)
	{
		TPacketDGInGameLog* pack;
		data = Decode(pack, data);
		switch (pack->subHeader)
		{
			case SUBHEADER_IGL_DG_OFFLINESHOP_REQUESTLOG: { RecvOfflineShopLogRequestDG(data); break; }
			case SUBHEADER_IGL_DG_TRADE_REQUESTLOG: { RecvTradeLogRequestDG(data); break; }
			case SUBHEADER_IGL_DG_TRADE_DETAILS_REQUESTLOG: { RecvTradeLogRequestDetailsDG(data); break; }
			default: { sys_err("Unknow recovery packet DG %d", pack->subHeader); break; }
		}
	}

	void InGameLogManager::RecvOfflineShopLogRequestDG(const char* data)
	{
		TInGameLogRequestDG* subpack;
		data = Decode(subpack, data);

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(subpack->ownerID);

		if (!ch || ch == nullptr)
			return;

		std::vector<TOfflineShopSoldLog> vLog;
		vLog.clear();

		if (subpack->logCount < 1)
		{
			SendOfflineShopLogRequestGC(ch, vLog);
			return;
		}

		vLog.reserve(subpack->logCount);
		TOfflineShopSoldLog* logTemp;
		for (WORD i = 0; i < subpack->logCount; i++)
		{
			data = Decode(logTemp, data);
			vLog.push_back(*logTemp);
			
		}
		SendOfflineShopLogRequestGC(ch, vLog);
	}

	void InGameLogManager::RecvTradeLogRequestDG(const char* data)
	{
		TInGameLogRequestDG* subpack;
		data = Decode(subpack, data);

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(subpack->ownerID);

		if (!ch || ch == nullptr)
			return;

		std::vector<TTradeLogRequestInfo> vLog;
		vLog.clear();

		if (subpack->logCount < 1)
		{
			SendTradeLogRequestGC(ch, vLog);
			return;
		}

		vLog.reserve(subpack->logCount);
		TTradeLogRequestInfo* logTemp;
		for (WORD i = 0; i < subpack->logCount; i++)
		{
			data = Decode(logTemp, data);
			vLog.push_back(*logTemp);

		}
		SendTradeLogRequestGC(ch, vLog);
	}

	void InGameLogManager::RecvTradeLogRequestDetailsDG(const char* data)
	{
		DWORD* ownerID;
		data = Decode(ownerID, data);

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(*ownerID);
		if (!ch || ch == nullptr)
			return;

		TTradeLogDetailsRequest* subpack;
		data = Decode(subpack, data);

		std::vector<TTradeLogRequestItem> vLog;
		if (subpack->itemCount == 0)
		{
			SendTradeLogDetailsRequestGC(ch, *subpack, vLog);
			return;
		}

		vLog.reserve(subpack->itemCount);
		TTradeLogRequestItem* logTemp;
		for (BYTE i = 0; i < subpack->itemCount; i++)
		{
			data = Decode(logTemp, data);
			vLog.push_back(*logTemp);

		}
		SendTradeLogDetailsRequestGC(ch, *subpack, vLog);
	}
	//DG PACKET END

	//GC PACKET BEGIN
	void InGameLogManager::SendOfflineShopLogRequestGC(LPCHARACTER ch, const std::vector<TOfflineShopSoldLog> vLog)
	{
		LPDESC d = ch->GetDesc();
		if (!d || d == nullptr)
			return;

		DWORD LogCount = vLog.size();
		TPacketGCInGameLog pack;
		pack.header = HEADER_GC_IN_GAME_LOG;
		pack.subHeader = InGameLog::InGameLogGCSubHeader::SUBHEADER_GC_OFFLINESHOP_LOG_OPEN;
		pack.size = sizeof(pack) + sizeof(DWORD) + (sizeof(TOfflineShopSoldLog) * LogCount);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGCInGameLog));
		buff.write(&LogCount, sizeof(DWORD));

		if (LogCount < 1)
		{
			d->Packet(buff.read_peek(), buff.size());
			return;
		}

		buff.write(&vLog[0], sizeof(TOfflineShopSoldLog)* LogCount);
		d->Packet(buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendOfflineShopLogAddGC(LPDESC d, const TOfflineShopSoldLog& soldLog)
	{
		if (!d)
			return;

		TPacketGCInGameLog pack;
		pack.header = HEADER_GC_IN_GAME_LOG;
		pack.subHeader = InGameLog::InGameLogGCSubHeader::SUBHEADER_GC_OFFLINESHOP_LOG_ADD;
		pack.size = sizeof(pack) + sizeof(TOfflineShopSoldLog);

		TOfflineShopSoldLog subpack;
		CopyObject(subpack, soldLog);
		snprintf(subpack.soldDate, sizeof(subpack.soldDate), "Updating");

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGCInGameLog));
		buff.write(&subpack, sizeof(TOfflineShopSoldLog));

		d->Packet(buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendTradeLogRequestGC(LPCHARACTER ch, const std::vector<TTradeLogRequestInfo> vLog)
	{
		LPDESC d = ch->GetDesc();
		if (!d || d == nullptr)
			return;

		DWORD LogCount = vLog.size();
		TPacketGCInGameLog pack;
		pack.header = HEADER_GC_IN_GAME_LOG;
		pack.subHeader = InGameLog::InGameLogGCSubHeader::SUBHEADER_GC_TRADE_LOG_OPEN;
		pack.size = sizeof(pack) + sizeof(DWORD) + (sizeof(TTradeLogRequestInfo) * LogCount);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGCInGameLog));
		buff.write(&LogCount, sizeof(DWORD));

		if (LogCount < 1)
		{
			d->Packet(buff.read_peek(), buff.size());
			return;
		}

		buff.write(&vLog[0], sizeof(TTradeLogRequestInfo) * LogCount);
		d->Packet(buff.read_peek(), buff.size());
	}

	void InGameLogManager::SendTradeLogDetailsRequestGC(LPCHARACTER ch, const TTradeLogDetailsRequest tradeInfo, const std::vector<TTradeLogRequestItem> vLog)
	{
		LPDESC d = ch->GetDesc();
		if (!d || d == nullptr)
			return;

		TPacketGCInGameLog pack;
		pack.header = HEADER_GC_IN_GAME_LOG;
		pack.subHeader = InGameLog::InGameLogGCSubHeader::SUBHEADER_GC_TRADE_LOG_DETAILS_OPEN;
		pack.size = sizeof(pack) + sizeof(TTradeLogDetailsRequest) + (sizeof(TTradeLogRequestItem) * tradeInfo.itemCount);

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(TPacketGCInGameLog));
		buff.write(&tradeInfo, sizeof(TTradeLogDetailsRequest));

		if (tradeInfo.itemCount < 1)
		{
			d->Packet(buff.read_peek(), buff.size());
			return;
		}
		buff.write(&vLog[0], sizeof(TTradeLogRequestItem) * tradeInfo.itemCount);
		d->Packet(buff.read_peek(), buff.size());
	}
	//GC PACKET END

	//CG PACKET BEGIN
	int InGameLogManager::RecvPacketCG(LPCHARACTER ch, BYTE SubHeader, DWORD logID)
	{
		switch (SubHeader)
		{
			case SUBHEADER_CG_OFFLINESHOP_LOG_OPEN: { SendOfflineShopRequestGD(ch->GetPlayerID()); break; }
			case SUBHEADER_CG_TRADE_LOG_OPEN: { SendTradeLogRequestGD(ch->GetPlayerID()); break; }
			case SUBHEADER_CG_TRADE_LOG_DETAILS_OPEN: { SendTradeLogDetailsRequestGD(ch->GetPlayerID(), logID); break; }
			default: { sys_err("Sub Header error %u", SubHeader); return -1; break; }
		}
		return 0;
	}
	//CG PACKET END
}

// fix cube
namespace InGameLog
{
    // Hack / Cheat logging
    void InGameLogManager::HackLogEx(LPCHARACTER ch, const char* textLine)
    {
        if (!ch)
            return;

        LPDESC desc = ch->GetDesc();
        if (!desc)
            return;

        // Use db_clientdesc or proper Query function from your project
        // If you have a global Query function:
        Query(
            "INSERT INTO `log`.`log_hack_ex` "
            "(`player`, `player_name`, `account`, `account_name`, `textLine`, `time`) "
            "VALUES (%u, '%s', %u, '%s', '%s', NOW())",
            ch->GetPlayerID(),
            ch->GetName(),
            ch->GetAID(),
            desc->GetAccountTable().login,
            textLine
        );
    }

    void InGameLogManager::HackLogEx(LPDESC desc, const char* textLine)
    {
        if (!desc)
            return;

        DWORD player_id = 0;
        DWORD account_id = 0;
        const char* player_name = "Unknown";

        LPCHARACTER ch = desc->GetCharacter();
        if (ch)
        {
            player_id = ch->GetPlayerID();
            account_id = ch->GetAID();
            player_name = ch->GetName();
        }

        Query(
            "INSERT INTO `log`.`log_hack_ex` "
            "(`player`, `player_name`, `account`, `account_name`, `textLine`, `time`) "
            "VALUES (%u, '%s', %u, '%s', '%s', NOW())",
            player_id,
            player_name,
            account_id,
            desc->GetAccountTable().login,
            textLine
        );
    }

    void InGameLogManager::HackLogEx(const std::string& stLogin, const char* textLine)
    {
        Query(
            "INSERT INTO `log`.`log_hack_auth` "
            "(account, textLine, time) "
            "VALUES ('%s', '%s', NOW())",
            stLogin.c_str(),
            textLine
        );
    }
}
//end fix cube
#endif