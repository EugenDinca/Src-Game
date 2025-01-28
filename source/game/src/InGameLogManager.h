#pragma once
#include "stdafx.h"
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

	class InGameLogManager : public singleton<InGameLogManager>
	{
		public:
			InGameLogManager();
			~InGameLogManager();

			//GD
			void SendOfflineShopRequestGD(DWORD ownerID);
			void SendOfflineShopAddItemGD(const TOfflineShopSoldLog& soldlog, DWORD ownerID);
			void SendTradeLogAddGD(const std::vector<TTradeLogItemInfo>& v_itemLog, const TSubPackTradeAddGD& info);
			void SendTradeLogRequestGD(DWORD ownerID);
			void SendTradeLogDetailsRequestGD(DWORD ownerID, DWORD logID);

			//DG
			void RecvInGameLogPacketDG(const char* data);
			void RecvOfflineShopLogRequestDG(const char* data);
			void RecvTradeLogRequestDG(const char* data);
			void RecvTradeLogRequestDetailsDG(const char* data);

			//GC
			void SendOfflineShopLogRequestGC(LPCHARACTER ch, const std::vector<TOfflineShopSoldLog> vLog);
			void SendOfflineShopLogAddGC(LPDESC d, const TOfflineShopSoldLog& soldLog);
			void SendTradeLogRequestGC(LPCHARACTER ch, const std::vector<TTradeLogRequestInfo> vLog);
			void SendTradeLogDetailsRequestGC(LPCHARACTER ch, const TTradeLogDetailsRequest tradeInfo, const std::vector<TTradeLogRequestItem> vLog);

			//CG
			int RecvPacketCG(LPCHARACTER ch, BYTE SubHeader, DWORD logID);
	};

	InGameLog::InGameLogManager& GetManager();
}
#endif
