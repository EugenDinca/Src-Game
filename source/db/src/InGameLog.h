#pragma once
#include "stdafx.h"
#include "../../common/tables.h"
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
namespace InGameLog
{
	template <class T>
	void ZeroObject(T& obj)
	{
		memset(&obj, 0, sizeof(obj));
	}

	template <class T>
	void CopyObject(T& objDest, const T& objSrc)
	{
		memcpy(&objDest, &objSrc, sizeof(objDest));
	}

	template <class T>
	const char* Decode(T*& pObj, const char* data)
	{
		pObj = (T*)data;
		return data + sizeof(T);
	}

	class CacheOfflineShopLog
	{
		public:
			typedef std::vector<TOfflineShopSoldLog> SOLD_ITEM_VEC;
			typedef std::map<DWORD, SOLD_ITEM_VEC> SOLD_MAP;

		public:
			CacheOfflineShopLog();
			~CacheOfflineShopLog();

			void PutItem(DWORD ownerID, const TOfflineShopSoldLog& log);
			bool GetLog(DWORD ownerID, SOLD_ITEM_VEC& soldItems) const;
			void AddItem(DWORD ownerID, const TOfflineShopSoldLog& log);

			std::string InsertSoldItemLogQuery(DWORD ownerID, const TOfflineShopSoldLog& log);
			std::string SelectSoldItemLogQuery();

		private:
			SOLD_MAP m_soldMap;
			typedef SOLD_MAP::iterator SOLD_ITER;
			typedef SOLD_MAP::const_iterator SOLD_CONST_ITER;
	};

	class CacheTradeLog
	{
		public:
			typedef std::vector<TTradeLogItemInfo> TRADE_ITEM_VEC;
			typedef std::vector<DWORD> TRADE_LOG_ID_VEC;

			typedef std::map<DWORD, TRADE_LOG_ID_VEC> TRADE_LOG_ID_MAP;//first owner or victim pid
			typedef std::map<DWORD, TTradeLogInfo> TRADE_INFO_MAP;//first logID
			typedef std::map<DWORD, TRADE_ITEM_VEC> TRADE_ITEM_MAP;//first logID

		public:
			CacheTradeLog();
			~CacheTradeLog();
			
			void AddNewLog(const TTradeLogInfoAdd& log, const TRADE_ITEM_VEC& item);

			void PutInfoLog(const TTradeLogInfo& log);
			void PutPIDtoLogID(const DWORD ownerPID, const DWORD victimPID, const DWORD logID);
			void PutItemLog(DWORD logID, const TTradeLogItemInfo& log);

			bool GetInfoLog(DWORD requestPID, std::vector<TTradeLogRequestInfo>& log);
			bool GetDetailsLog(DWORD requestPID, TTradeLogDetailsRequest& tradeInfo, std::vector<TTradeLogRequestItem>& tradeItem);

			std::string SelectTradeInfoLogQuery();
			std::string SelectTradeItemLogQuery();

			std::string InsertTradeInfoLogQuery(const TTradeLogInfoAdd& log);
			std::string InsertTradeItemLogQuery(const TTradeLogItemInfo& log, UINT logID);

			const char* GetNewLogDate(const DWORD logID);
		private:
			TRADE_INFO_MAP m_tradeInfoMap;
			typedef TRADE_INFO_MAP::iterator TRADE_INFO_ITER;
			typedef TRADE_INFO_MAP::const_iterator TRADE_INFO_CONST_ITER;
			
			TRADE_ITEM_MAP m_tradeItemMap;
			typedef TRADE_ITEM_MAP::iterator TRADE_ITEM_ITER;
			typedef TRADE_ITEM_MAP::const_iterator TRADE_ITEM_CONST_ITER;

			TRADE_LOG_ID_MAP m_tradeLogIdMap;
			typedef TRADE_LOG_ID_MAP::iterator TRADE_LOG_ID_ITER;
			typedef TRADE_LOG_ID_MAP::const_iterator TRADE_LOG_ID_CONST_ITER;
	};
}
#endif