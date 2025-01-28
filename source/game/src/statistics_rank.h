#pragma once
#ifdef ENABLE_GLOBAL_RANKING
class CStatisticsRanking : public singleton<CStatisticsRanking>
{
	public:
		CStatisticsRanking();
		virtual ~CStatisticsRanking();
		void Destroy();

		void RequestRankingList(LPCHARACTER ch);
		void UpdateRanking(const char* c_pData);

	protected:
		DWORD m_nextUpdateTime;
		bool m_isInitialize;
		TRankingData m_rankingTop;

		std::map<DWORD, TRankingList>m_rankingList;
};
#endif