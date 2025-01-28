#include "stdafx.h"
#ifdef ENABLE_BOSS_REGEN
#include "BossRegen.h"
#include "char.h"
#include "char_manager.h"
#include "sectree.h"
#include "utils.h"
#include "sectree_manager.h"
CBossRegen::CBossRegen()
{
	m_BossRegenList.clear();
}

CBossRegen::~CBossRegen()
{
	m_BossRegenList.clear();
}

void CBossRegen::LoadRegen(const char* filename, long lMapIndex, int base_x, int base_y)
{
	FILE* fp = fopen(filename, "r");
	if (!fp)
		return;

	TSBossRegen temp{};
	temp.mapIdx = lMapIndex;
	temp.bossVID = 0;
	temp.nextRegenTime = 0;

	sys_log(0, "Load Boss Regen Begin Map Index %d", lMapIndex);
	while (fscanf(fp, "%u %d %d %d", &temp.vnum, &temp.x, &temp.y, &temp.spawnTime) != EOF)
	{
		temp.x *= 100;
		temp.y *= 100;
		temp.x += base_x;
		temp.y += base_y;
		m_BossRegenList.push_back(temp);

		sys_log(0, "Vnum : %lu x: %d y: %d time: %ld", temp.vnum, temp.x, temp.y, temp.spawnTime);
	}

	sys_log(0, "Load Boss Regen End Map Index %d", lMapIndex);

	fclose(fp);
}

void CBossRegen::FirstRegen()
{
	const time_t ft = time(0);
	const struct tm lt = *localtime(&ft);
	LPCHARACTER boss = nullptr;

	for (int i = 0; i < m_BossRegenList.size(); i++)
	{
		TSBossRegen& regenList = m_BossRegenList.at(i);

		const time_t rt = regenList.spawnTime;
		struct tm rlt {};
		this->GetLocalTime(rlt, rt);

		struct tm nextRT {};

		if (rlt.tm_min > 0)
		{
			if (lt.tm_min > rlt.tm_min)
			{
				nextRT.tm_min = rlt.tm_min - (lt.tm_min - (rlt.tm_min * (lt.tm_min / rlt.tm_min)));
			}
			else
			{
				nextRT.tm_min = rlt.tm_min - lt.tm_min;
			}
		}
		nextRT.tm_hour = rlt.tm_hour;
		nextRT.tm_sec = lt.tm_sec;

		regenList.nextRegenTime = get_global_time() + (((nextRT.tm_hour * 3600) + (nextRT.tm_min * 60)) - nextRT.tm_sec);

		boss = CHARACTER_MANAGER::instance().SpawnMob(regenList.vnum, regenList.mapIdx, regenList.x, regenList.y, 0);
		if (boss)
		{
			regenList.bossVID = boss->GetVID();
		}			
	}
}

void CBossRegen::GetLocalTime(tm& t, time_t time)
{
	if (time >= 3600)
	{
		t.tm_hour = time / 3600;
		time -= t.tm_hour * 3600;
	}

	if (time > 60)
	{
		t.tm_min = time / 60;
		time -= t.tm_min * 60;
	}
	t.tm_sec = time;
}


void CBossRegen::UpdateRegen()
{
	for (int i = 0; i < m_BossRegenList.size(); i++)
	{
		TSBossRegen& regenList = m_BossRegenList.at(i);
		if (regenList.nextRegenTime <= get_global_time())
		{
			LPCHARACTER boss = CHARACTER_MANAGER::Instance().Find(regenList.bossVID);
			if (boss)
				M2_DESTROY_CHARACTER(boss);

			regenList.nextRegenTime = get_global_time() + regenList.spawnTime;

			if ((boss = CHARACTER_MANAGER::instance().SpawnMob(regenList.vnum, regenList.mapIdx, regenList.x, regenList.y, 0)))
				regenList.bossVID = boss->GetVID();
		}
	}
}
#endif