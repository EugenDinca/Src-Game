#include "stdafx.h"
#ifdef ENABLE_GLOBAL_RANKING
#include "statistics_rank.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "utils.h"
#include "buffer_manager.h"
#include "InGameLogManager.h"
CStatisticsRanking::CStatisticsRanking()
{
	Destroy();
}
CStatisticsRanking::~CStatisticsRanking()
{
	Destroy();
}

void CStatisticsRanking::Destroy()
{
	m_isInitialize = false;
	m_nextUpdateTime = 0;
	memset(&m_rankingTop, 0, sizeof(TRankingData));
}

void CStatisticsRanking::RequestRankingList(LPCHARACTER ch)
{
	if (!m_isInitialize)
	{
		db_clientdesc->DBPacket(HEADER_GD_GLOBAL_RANKING, 0, NULL, 0);
		return;
	}

	if (!ch)
		return;

	LPDESC d = ch->GetDesc();
	if (!d)
		return;
	
	TRankingMeGC playerRank{};
	memset(&playerRank, 0, sizeof(TRankingMeGC));

	const auto& it = m_rankingList.find(ch->GetPlayerID());
	if (it != m_rankingList.end())
	{
		memcpy(&playerRank.rank, &it->second, sizeof(TRankingList));
	}

	playerRank.country = ch->GetLanguage();
	playerRank.playerRace = ch->GetRaceNum();
	snprintf(playerRank.playerName, sizeof(playerRank.playerName), ch->GetName());

	TRankingGCPacket packet;
	packet.bHeader = HEADER_GC_GLOBAL_RANKING;
	packet.wSize = sizeof(TRankingGCPacket) + sizeof(TRankingData) + sizeof(TRankingMeGC);
	packet.next_update_second = m_nextUpdateTime - get_global_time();

	TEMP_BUFFER buff;
	buff.write(&packet, sizeof(TRankingGCPacket));
	buff.write(&m_rankingTop, sizeof(TRankingData));
	buff.write(&playerRank, sizeof(TRankingMeGC));

	d->Packet(buff.read_peek(), buff.size());
}

void CStatisticsRanking::UpdateRanking(const char* c_pData)
{
	TRankigDGPacket* p;
	c_pData = InGameLog::Decode(p, c_pData);
	
	if (p->subHeader == RANKING_TOP)
	{
		TRankingData* topRank;
		c_pData = InGameLog::Decode(topRank, c_pData);

		Destroy();
		memcpy(&m_rankingTop, topRank, sizeof(TRankingData));
		m_nextUpdateTime = get_global_time() + CACHE_TIME;
		m_isInitialize = true;
	}
	else
	{
		BYTE* listIdx;
		c_pData = InGameLog::Decode(listIdx, c_pData);

		int* rankListSize;
		c_pData = InGameLog::Decode(rankListSize, c_pData);

		size_t bufferSize = p->size - (sizeof(TRankigDGPacket) + sizeof(BYTE) + sizeof(int));
		if ((bufferSize % sizeof(TRanking)) != 0)
		{
			sys_err("CRITICAL ERROR: Rank list %d buffer fail", listIdx);
		}

		for (int i = 0; i < *rankListSize; i++)
		{
			TRanking* rankList;
			c_pData = InGameLog::Decode(rankList, c_pData);

			auto it = m_rankingList.find(rankList->pid);
			if (it != m_rankingList.end())
			{
				it->second.order[*listIdx] = i;
				it->second.value[*listIdx] = rankList->value;
			}
			else
			{
				TRankingList mapTemp{};
				memset(&mapTemp, 0, sizeof(mapTemp));
				
				mapTemp.order[*listIdx] = i;
				mapTemp.value[*listIdx] = rankList->value;

				m_rankingList.insert(std::make_pair(rankList->pid, mapTemp));
			}
		}
	}

}
#endif