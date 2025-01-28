#pragma once
#include "stdafx.h"
#ifdef ENABLE_BIOLOG_SYSTEM
#include "packet.h"
typedef std::map<BYTE, WORD> GiveBonusMap;
class BiologSystem
{
public:
	BiologSystem(LPCHARACTER m_pkChar);
	virtual ~BiologSystem();
	void Destroy();

	void GiveBonus();
	void SendItem(TPacketCGBiologUpgrade info);
	void LevelUp();

	void KillMob(DWORD vnum);
	void SendPacket(BYTE SubHeader, const void* Data, WORD size);
	int RecvPacket(BYTE SubHeader, const char* Data, UINT uiBytes);
private:
	LPCHARACTER m_pkChar;
	GiveBonusMap m_mapGiveBonus;
	bool m_givebonusload;

	TBiologMobInfo mobInfo;
};

class BiologManager : public singleton<BiologManager>
{
public:
	BiologManager();
	~BiologManager();
	void Initialize();
	void Destroy();
	void LoadInformationsData();
	TBiologInformationsCache* GetUpgradeInformations(BYTE biolog_level);
	TPacketGCBiologInformations* GetBiologGCInformations(BYTE biolog_level);
	void GetGiveBonus(GiveBonusMap& bonusmap, BYTE biolog_level);

	TBiologMobInfo GetBiologMobInfo(BYTE biolog_level);
	bool IsMaxLevel(BYTE level) { return m_max_biolog_level <= level; }
	BYTE GetMaxLevel() { return m_max_biolog_level; }
protected:
	std::vector<TPacketGCBiologInformations> m_vecBiologGCInformations;
	std::map<BYTE, TBiologInformationsCache> m_mapBiologInformations;
private:
	bool IsLoadedData;
	char FetchQuery[64];
	BYTE m_max_biolog_level;
};
#endif