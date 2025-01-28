#pragma once
#ifdef ENABLE_BOSS_REGEN
typedef struct SBossRegen
{
	DWORD vnum;
	int x, y;
	time_t spawnTime;
	WORD mapIdx;

	DWORD bossVID;
	time_t nextRegenTime;
}TSBossRegen;

class CBossRegen : public singleton<CBossRegen>
{
public:
	CBossRegen();
	~CBossRegen();

	void LoadRegen(const char* filename, long lMapIndex, int base_x, int base_y);
	void FirstRegen();
	void UpdateRegen();
	void GetLocalTime(tm& t, time_t time);

private:
	std::vector<TSBossRegen> m_BossRegenList;
};

#endif