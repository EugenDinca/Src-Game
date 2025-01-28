#include "biolog_system.h"
#ifdef ENABLE_BIOLOG_SYSTEM
#include "stdafx.h"
#include "char.h"
#include "item_manager.h"
#include "item.h"
#include "desc_client.h"
#include "db.h"
#include "buffer_manager.h"
#include "utils.h"
/***************************************************************************/
/*************************BIOLOG SYSTEM*************************************/
/***************************************************************************/
BiologSystem::BiologSystem(LPCHARACTER ch)
	:m_pkChar(ch)
{
}

BiologSystem::~BiologSystem()
{
	m_pkChar = nullptr;
}

void BiologSystem::Destroy()
{
	m_givebonusload = false;
	m_mapGiveBonus.clear();
	memset(&mobInfo, 0, sizeof(mobInfo));
}

void BiologSystem::GiveBonus()
{
	if (!m_pkChar || m_pkChar == nullptr)
	{
		sys_err("BiologSystem ch empty");
		return;
	}

	if (!m_givebonusload)
	{
		m_mapGiveBonus.clear();
		BiologManager::Instance().GetGiveBonus(m_mapGiveBonus, m_pkChar->GetBiologInfo().level);
		if (m_pkChar->GetBiologInfo().state == 1)
		{
			mobInfo = BiologManager::Instance().GetBiologMobInfo(m_pkChar->GetBiologInfo().level);
		}
		m_givebonusload = true;
	}

	for (itertype(m_mapGiveBonus) it = m_mapGiveBonus.begin(); it != m_mapGiveBonus.end(); it++)
	{
		m_pkChar->PointChange(it->first, it->second);
	}
}

void BiologSystem::SendItem(TPacketCGBiologUpgrade info)
{

	if (m_pkChar->WindowOpenCheck() || m_pkChar->ActivateCheck())
	{
		m_pkChar->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	TPacketGCBiologUpdate chinfo = m_pkChar->GetBiologInfo();

	if (BiologManager::Instance().IsMaxLevel(chinfo.level))
	{
		m_pkChar->NewChatPacket(CANT_INCREASE_YOUR_BIOLOG_LEVEL_MORE);
		return;
	}

	TBiologInformationsCache* upgradeinfo = BiologManager::Instance().GetUpgradeInformations(chinfo.level);
	if (!upgradeinfo)
	{
		return;
	}
	
	if (chinfo.state == 1)
	{
		if (m_pkChar->CountSpecifyItem(upgradeinfo->mission_stone_vnum) < 1)
		{
			return;
		}
		m_pkChar->RemoveSpecifyItem(upgradeinfo->mission_stone_vnum);
		LevelUp();
		return;
	}

	if (m_pkChar->GetLevel() < upgradeinfo->mission_level)
	{
		return;
	}

	if (m_pkChar->CountSpecifyItem(upgradeinfo->mission_vnum) < 1)
	{
		return;
	}

	if ((info.allgive && info.time_reset ) || (info.allgive && (!info.time_reset && upgradeinfo->mission_cooldown == 0)))
	{
		MAX_COUNT remaindercount = upgradeinfo->mission_count - chinfo.given_count;
		MAX_COUNT IncreaseCount = m_pkChar->CountSpecifyItem(upgradeinfo->mission_vnum);

		if (info.chance)
		{
			MAX_COUNT chanceitemcount = m_pkChar->CountSpecifyItem(BIOLOG_CHANCE_ITEM);
			IncreaseCount = IncreaseCount > chanceitemcount ? chanceitemcount : IncreaseCount;
		}
		if (info.time_reset)
		{
			MAX_COUNT timeresetitemcount = m_pkChar->CountSpecifyItem(BIOLOG_TIMERESET_ITEM);
			IncreaseCount = IncreaseCount > timeresetitemcount ? timeresetitemcount : IncreaseCount;
		}

		if (IncreaseCount == 0)
		{
			return;
		}

		MAX_COUNT successcount = 0;
		MAX_COUNT failcount = 0;
		if (info.chance)
		{
			IncreaseCount = IncreaseCount > remaindercount ? remaindercount : IncreaseCount;
			successcount = IncreaseCount;
			m_pkChar->RemoveSpecifyItem(BIOLOG_CHANCE_ITEM, IncreaseCount);
		}
		else
		{
			MAX_COUNT i = 0;
			while(IncreaseCount > i)
			{
				BYTE chance = number(0, 100);
				if (chance <= upgradeinfo->mission_success_change)
				{
					successcount++;
				}
				else
				{
					failcount++;
				}

				i++;

				if (i == remaindercount)
				{
					IncreaseCount = remaindercount;
					break;
				}
			}
		}

		m_pkChar->RemoveSpecifyItem(upgradeinfo->mission_vnum, IncreaseCount);
		if (info.time_reset)
		{
			m_pkChar->RemoveSpecifyItem(BIOLOG_TIMERESET_ITEM, IncreaseCount);
		}

		m_pkChar->SetBiologCount(chinfo.given_count + successcount);
		m_pkChar->NewChatPacket(X_SUCCESSFULL_Y_UNSUCCESSFULL, "%d|%d", successcount, failcount);
		if (m_pkChar->GetBiologInfo().given_count >= upgradeinfo->mission_count)
		{
			LevelUp();
			return;
		}
	}
	else
	{
		BYTE chance = number(0, 100);
		if (info.chance)
		{
			if (m_pkChar->CountSpecifyItem(BIOLOG_CHANCE_ITEM) < 1)
			{
				return;
			}
			chance = 0;
		}

		if (chinfo.time > get_global_time())
		{
			if (info.time_reset)
			{
				if (m_pkChar->CountSpecifyItem(BIOLOG_TIMERESET_ITEM) < 1)
				{
					return;
				}
				m_pkChar->RemoveSpecifyItem(BIOLOG_TIMERESET_ITEM);
			}
			else
			{
				return;
			}
		}
		else if(info.time_reset)
		{
			return;
		}

		m_pkChar->SetBiologTime(get_global_time() + upgradeinfo->mission_cooldown);
		
		if (info.chance)
		{
			m_pkChar->RemoveSpecifyItem(BIOLOG_CHANCE_ITEM);
		}

		m_pkChar->RemoveSpecifyItem(upgradeinfo->mission_vnum);
		if (chance <= upgradeinfo->mission_success_change)
		{
			m_pkChar->SetBiologCount(chinfo.given_count + 1);
			m_pkChar->NewChatPacket(IT_WAS_SUCCESFULL);
			if (m_pkChar->GetBiologInfo().given_count >= upgradeinfo->mission_count)
			{
				LevelUp();
				return;
			}
		}
		else
		{
			m_pkChar->NewChatPacket(IT_WAS_FAILED);
		}
	}

	TPacketGCBiologUpdate chbiologinfo = m_pkChar->GetBiologInfo();
	if (chbiologinfo.time < get_global_time())
	{
		chbiologinfo.time = 0;
	}
	else
	{
		chbiologinfo.time -= get_global_time();
		m_pkChar->SetBiologAlert(true);
		m_pkChar->BiologAlertEventStop();
		m_pkChar->BiologAlertEventStart();
	}

	SendPacket(SUBHEADER_GC_BIOLOG_COUNT_UPDATE, &chbiologinfo, sizeof(chbiologinfo));
}

void BiologSystem::LevelUp()
{
	m_pkChar->SetBiologCount(0);
	m_pkChar->SetBiologTime(0);
	TPacketGCBiologUpdate chbiologinfo = m_pkChar->GetBiologInfo();
	TBiologInformationsCache* bioInfoCache = BiologManager::Instance().GetUpgradeInformations(chbiologinfo.level);

	if (chbiologinfo.state == 1 || bioInfoCache->mission_stone_vnum == 0)
	{
		m_pkChar->SetBiologLevel(chbiologinfo.level + 1);
		m_pkChar->SetBiologState(0);
		memset(&mobInfo, 0, sizeof(mobInfo));
		m_givebonusload = false;
		m_pkChar->ComputePoints();

		if (BiologManager::Instance().IsMaxLevel(chbiologinfo.level))
		{
			SendPacket(SUBHEADER_GC_BIOLOG_MAX_LEVEL, nullptr, 0);
			return;
		}

	}
	else
	{
		m_pkChar->SetBiologState(1);
		mobInfo = BiologManager::Instance().GetBiologMobInfo(m_pkChar->GetBiologInfo().level);
	}

	chbiologinfo = m_pkChar->GetBiologInfo();
	TPacketGCBiologInformations* p_bioinfo = BiologManager::Instance().GetBiologGCInformations(chbiologinfo.level);
	TPacketGCBiologInformations bioinfo{};
	memcpy(&bioinfo, p_bioinfo, sizeof(bioinfo));

	TEMP_BUFFER buffer;
	buffer.write(&bioinfo, sizeof(bioinfo));
	buffer.write(&chbiologinfo, sizeof(chbiologinfo));

	SendPacket(SUBHEADER_GC_BIOLOG_LEVEL_UPDATE, buffer.read_peek(), buffer.size());
	m_pkChar->NewChatPacket(YOUR_BIOLOG_LEVEL_INCREASED);

}

void BiologSystem::KillMob(DWORD vnum)
{
	if (mobInfo.vnum != vnum)
		return;

	if (mobInfo.chance >= number(0, 250))
	{
		TPacketGCBiologInformations* bioinfo = BiologManager::Instance().GetBiologGCInformations(m_pkChar->GetBiologInfo().level);
		if (!bioinfo)
			return;
		m_pkChar->AutoGiveItem(bioinfo->mission_stone_vnum);
	}
}

/******************************************PACKET*********************************************************/
void BiologSystem::SendPacket(BYTE SubHeader, const void* Data, WORD size)
{
	if (!m_pkChar->GetDesc())
		return;

	TPacketGCBiolog packet;
	packet.header = HEADER_GC_BIOLOG;
	packet.size = sizeof(TPacketGCBiolog) + size;
	packet.sub_header = SubHeader;

	TEMP_BUFFER buf;
	buf.write(&packet, sizeof(TPacketGCBiolog));

	if (size > 0)
	{
		buf.write(Data, size);
	}

	m_pkChar->GetDesc()->Packet(buf.read_peek(), buf.size());
}

int BiologSystem::RecvPacket(BYTE SubHeader, const char* Data, UINT uiBytes)
{
	switch (SubHeader)
	{
		case SUBHEADER_CG_BIOLOG_OPEN:
		{
			TPacketGCBiologUpdate chbiologinfo = m_pkChar->GetBiologInfo();
			if (BiologManager::Instance().IsMaxLevel(chbiologinfo.level))
			{
				SendPacket(SUBHEADER_GC_BIOLOG_MAX_LEVEL, nullptr, 0);
				return 0;
			}
			TPacketGCBiologInformations* p_bioinfo = BiologManager::Instance().GetBiologGCInformations(chbiologinfo.level);
			TPacketGCBiologInformations bioinfo{};
			memcpy(&bioinfo, p_bioinfo, sizeof(bioinfo));

			if (chbiologinfo.time < get_global_time())
			{
				chbiologinfo.time = 0;
			}
			else
			{
				chbiologinfo.time -= get_global_time();
			}

			TEMP_BUFFER buffer;
			buffer.write(&bioinfo, sizeof(bioinfo));
			buffer.write(&chbiologinfo, sizeof(chbiologinfo));

			SendPacket(SUBHEADER_GC_BIOLOG_OPEN, buffer.read_peek(),buffer.size());
			return 0;
			break;
		}

		case SUBHEADER_CG_BIOLOG_UPGRADE:
		{
			if (uiBytes < sizeof(TPacketCGBiologUpgrade))
				return -1;

			TPacketCGBiologUpgrade p = *(TPacketCGBiologUpgrade*)Data;
			Data += sizeof(TPacketCGBiologUpgrade);
			uiBytes -= sizeof(TPacketCGBiologUpgrade);

			SendItem(p);
			return sizeof(TPacketCGBiologUpgrade);
			break;
		}
	}
	sys_err("Sub Header error %u", SubHeader);
	return -1;
}

/***************************************************************************/
/*************************BIOLOG MANAGER************************************/
/***************************************************************************/
BiologManager::BiologManager()
{
	Initialize();
}

BiologManager::~BiologManager()
{
	Destroy();
}

void BiologManager::Initialize()
{
	IsLoadedData = false;
	snprintf(FetchQuery, sizeof(FetchQuery), "SELECT * FROM player.biolog_table");
	m_vecBiologGCInformations.clear();
	m_mapBiologInformations.clear();
	m_max_biolog_level = 0;
}

void BiologManager::Destroy()
{
	m_vecBiologGCInformations.clear();
	m_mapBiologInformations.clear();
}

void BiologManager::LoadInformationsData()
{
	std::unique_ptr<SQLMsg> pMsg (DBManager::instance().DirectQuery(FetchQuery));
	if (pMsg->Get()->uiNumRows == 0) { sys_err("Sql data empty"); return; }

	std::map<BYTE, TBiologInformationsSqlFetch> BiologInformationsTable;//geçiçi bir map oluþturduk
	BiologInformationsTable.clear();

	MYSQL_ROW row;
	//Sqldeki tüm verileri çekene kadar iþler
	while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		BYTE col = 0;
		TBiologInformationsSqlFetch informations{};//geçiçi struct oluþturduk mape yazmak için
		str_to_number(informations.mission_index, row[col++]);
		str_to_number(informations.mission_level, row[col++]);
		str_to_number(informations.mission_vnum, row[col++]);
		str_to_number(informations.mission_stone_vnum, row[col++]);
		str_to_number(informations.mission_stone_mob_vnum, row[col++]);
		str_to_number(informations.mission_stone_mob_change, row[col++]);
		str_to_number(informations.mission_count, row[col++]);
		str_to_number(informations.mission_success_change, row[col++]);
		str_to_number(informations.mission_cooldown, row[col++]);
		for (int i = 0; i < 4; i++)
		{
			//verdiðimiz özelliklerin point ve apply lerini aldýk
			std::pair<BYTE,BYTE> variable = StringApplyToPoint(row[col++]);
			informations.mission_bonus[i].apply_type = variable.first;
			informations.mission_bonus[i].point_type = variable.second;

			str_to_number(informations.mission_bonus[i].value, row[col++]);
		}
		BiologInformationsTable.insert(std::make_pair(informations.mission_index, informations));
	}

	//sqlden aldýðýmýz verileri cache alýyoruz
	for (itertype(BiologInformationsTable) it = BiologInformationsTable.begin(); it != BiologInformationsTable.end(); it++)
	{
		TPacketGCBiologInformations temGCpinf{};
		TBiologInformationsCache tempCache{};
		
		tempCache.mission_success_change = it->second.mission_success_change;
		tempCache.mission_stone_mob_vnum = it->second.mission_stone_mob_vnum;
		tempCache.mission_stone_mob_change = it->second.mission_stone_mob_change;

		temGCpinf.mission_level		= tempCache.mission_level		= it->second.mission_level;
		temGCpinf.mission_vnum		= tempCache.mission_vnum		= it->second.mission_vnum;
		temGCpinf.mission_stone_vnum = tempCache.mission_stone_vnum = it->second.mission_stone_vnum;
		temGCpinf.mission_count		= tempCache.mission_count		= it->second.mission_count;
		temGCpinf.mission_cooldown	= tempCache.mission_cooldown	= it->second.mission_cooldown;

		for (int i = 0; i < 4; i++)
		{
			tempCache.mission_bonus[i].type = it->second.mission_bonus[i].point_type;
			temGCpinf.mission_bonus[i].type	= it->second.mission_bonus[i].apply_type;
			tempCache.mission_bonus[i].value = temGCpinf.mission_bonus[i].value = it->second.mission_bonus[i].value;
		}

		m_mapBiologInformations.insert(std::make_pair(it->second.mission_index, tempCache));
		m_vecBiologGCInformations.push_back(temGCpinf);
	}

	m_max_biolog_level = m_vecBiologGCInformations.size();
	IsLoadedData = true;//verinin yüklü olduðunu belirledik
}

//Biolog level geliþtirmesinde gerekli bilgiler için bellek adresini döndürür
TBiologInformationsCache* BiologManager::GetUpgradeInformations(BYTE biolog_level)
{
	itertype(m_mapBiologInformations) it = m_mapBiologInformations.find(biolog_level);
	if (it == m_mapBiologInformations.end())
		return nullptr;

	return &it->second;
}

TPacketGCBiologInformations* BiologManager::GetBiologGCInformations(BYTE biolog_level)
{
	if (!IsLoadedData)
		LoadInformationsData();

	if (m_vecBiologGCInformations.size() < biolog_level)
		return nullptr;

	return &m_vecBiologGCInformations[biolog_level];
}

//Karakterin biolog leveline göre verilecek özelliklerin point ve valuelerini referansla doldurur
void BiologManager::GetGiveBonus(GiveBonusMap& bonusmap, BYTE biolog_level)
{
	if (!IsLoadedData)
		LoadInformationsData();

	itertype(m_mapBiologInformations) it_end = m_mapBiologInformations.find(biolog_level);
	bonusmap.clear();

	for (itertype(m_mapBiologInformations) it = m_mapBiologInformations.begin(); it != it_end; it++)
	{
		for (int i = 0; i < 4; i++)
		{
			TBiologBonus bonus = it->second.mission_bonus[i];
			itertype(bonusmap) it2 = bonusmap.find(bonus.type);
			if (it2 == bonusmap.end())
			{
				bonusmap.insert(std::make_pair(bonus.type, bonus.value));
			}
			else
			{
				it2->second += bonus.value;
			}
		}
	}
}

TBiologMobInfo BiologManager::GetBiologMobInfo(BYTE biolog_level)
{
	TBiologMobInfo info;
	memset(&info, 0, sizeof(info));

	itertype(m_mapBiologInformations) it = m_mapBiologInformations.find(biolog_level);
	if (it != m_mapBiologInformations.end())
	{
		info.vnum = it->second.mission_stone_mob_vnum;
		info.chance = it->second.mission_stone_mob_change;
	}

	return info;
}
#endif




