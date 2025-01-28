#pragma once
#include "stdafx.h"
#ifdef ENABLE_DUNGEON_INFO
class CDungeonInfo : public singleton<CDungeonInfo>
{
public:
	CDungeonInfo();
	~CDungeonInfo();
	bool Initialize();
	TDungeonInfoTable* GetDungeonInfo(BYTE dungeonIndex);
	using InfoMap = std::unordered_map<BYTE, TDungeonInfoTable>;
	BYTE GetMaxDungeon() { return m_MaxDungeon; }
private:
	InfoMap m_InfoMap;
	BYTE m_MaxDungeon;
};
#endif
