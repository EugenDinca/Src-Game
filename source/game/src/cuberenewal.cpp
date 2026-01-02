#define _cube_cpp_

#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "char.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "questmanager.h"
#include <sstream>
#include "packet.h"
#include "desc_client.h"

static std::vector<CUBE_RENEWAL_DATA*>	s_cube_proto;

typedef std::vector<CUBE_RENEWAL_VALUE>	TCubeValueVector;

struct SCubeMaterialInfo
{
	SCubeMaterialInfo()
	{
		bHaveComplicateMaterial = false;
	};

	CUBE_RENEWAL_VALUE			reward;
	TCubeValueVector			material;
	long long					gold;
	int 						percent;
	std::string					category;
#ifdef ENABLE_CUBE_SAVE_SOCKET_ATTRS
	bool						allowCopyAttr;
#endif
	TCubeValueVector	complicateMaterial;

	std::string			infoText;
	bool				bHaveComplicateMaterial;
};

struct SItemNameAndLevel
{
	SItemNameAndLevel() { level = 0; }

	std::string		name;
	int				level;
};

typedef std::vector<SCubeMaterialInfo>								TCubeResultList;
typedef boost::unordered_map<DWORD, TCubeResultList>				TCubeMapByNPC;

TCubeMapByNPC cube_info_map;

static bool FN_check_valid_npc(WORD vnum)
{
	for (std::vector<CUBE_RENEWAL_DATA*>::iterator iter = s_cube_proto.begin(); iter != s_cube_proto.end(); iter++)
	{
		if (std::find((*iter)->npc_vnum.begin(), (*iter)->npc_vnum.end(), vnum) != (*iter)->npc_vnum.end())
			return true;
	}

	return false;
}

static bool FN_check_cube_data(CUBE_RENEWAL_DATA* cube_data)
{
	DWORD	i = 0;
	DWORD	end_index = 0;

	end_index = cube_data->npc_vnum.size();
	for (i = 0; i < end_index; ++i)
	{
		if (cube_data->npc_vnum[i] == 0)	return false;
	}

	end_index = cube_data->item.size();
	for (i = 0; i < end_index; ++i)
	{
		if (cube_data->item[i].vnum == 0)		return false;
		if (cube_data->item[i].count == 0)	return false;
	}

	end_index = cube_data->reward.size();
	for (i = 0; i < end_index; ++i)
	{
		if (cube_data->reward[i].vnum == 0)	return false;
		if (cube_data->reward[i].count == 0)	return false;
	}
	return true;
}

static int FN_check_cube_item_vnum_material(const SCubeMaterialInfo& materialInfo, int index)
{
	if (index <= materialInfo.material.size())
	{
		return materialInfo.material[index - 1].vnum;
	}
	return 0;
}

static int FN_check_cube_item_count_material(const SCubeMaterialInfo& materialInfo, int index)
{
	if (index <= materialInfo.material.size())
	{
		return materialInfo.material[index - 1].count;
	}

	return 0;
}

CUBE_RENEWAL_DATA::CUBE_RENEWAL_DATA()
{
	this->gold = 0;
	this->category = "WORLDARD";
#ifdef ENABLE_CUBE_SAVE_SOCKET_ATTRS
	this->allowCopyAttr = false;
#endif
}

void Cube_init()
{
	CUBE_RENEWAL_DATA* p_cube = NULL;
	std::vector<CUBE_RENEWAL_DATA*>::iterator iter;

	char file_name[256 + 1];
	snprintf(file_name, sizeof(file_name), "/lightwork/main/srv1/share/locale/europe/common/cube.txt");

	for (iter = s_cube_proto.begin(); iter != s_cube_proto.end(); iter++)
	{
		p_cube = *iter;
		M2_DELETE(p_cube);
	}

	s_cube_proto.clear();

	if (false == Cube_load(file_name))
		sys_err("Cube_Init failed");
}

bool Cube_load(const char* file)
{
	FILE* fp;

	const char* value_string;

	char	one_line[256];
	long long		value1, value2;
	const char* delim = " \t\r\n";
	char* v, * token_string;
	CUBE_RENEWAL_DATA* cube_data = NULL;
	CUBE_RENEWAL_VALUE	cube_value = { 0,0 };

	if (0 == file || 0 == file[0])
		return false;

	if ((fp = fopen(file, "r")) == 0)
		return false;

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);

		if (NULL == token_string)
			continue;

		// set value1, value2
		if ((v = strtok(NULL, delim)))
			str_to_number(value1, v);
		value_string = v;

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		TOKEN("section")
		{
			cube_data = M2_NEW CUBE_RENEWAL_DATA;
		}
		else TOKEN("npc")
		{
			cube_data->npc_vnum.push_back((WORD)value1);
		}
		else TOKEN("item")
		{
			cube_value.vnum = value1;
			cube_value.count = value2;

			cube_data->item.push_back(cube_value);
		}
		else TOKEN("reward")
		{
			cube_value.vnum = value1;
			cube_value.count = value2;

			cube_data->reward.push_back(cube_value);
		}
		else TOKEN("percent")
		{
			cube_data->percent = value1;
		}

	else TOKEN("category")
	{
		cube_data->category = value_string;
	}

	else TOKEN("gold")
	{
		cube_data->gold = value1;
	}
#ifdef ENABLE_CUBE_SAVE_SOCKET_ATTRS
	else TOKEN("allow_copy")
	{
	cube_data->allowCopyAttr = (value1 == 1 ? true : false);
	}
#endif
		else TOKEN("end")
		{
			// TODO : check cube data
			if (false == FN_check_cube_data(cube_data))
			{
				M2_DELETE(cube_data);
				continue;
			}
			s_cube_proto.push_back(cube_data);
		}
	}

	fclose(fp);
	return true;
}

SItemNameAndLevel SplitItemNameAndLevelFromName(const std::string& name)
{
	int level = 0;
	SItemNameAndLevel info;
	info.name = name;

	size_t pos = name.find("+");

	if (std::string::npos != pos)
	{
		const std::string levelStr = name.substr(pos + 1, name.size() - pos - 1);
		str_to_number(level, levelStr.c_str());

		info.name = name.substr(0, pos);
	}

	info.level = level;

	return info;
};

bool Cube_InformationInitialize()
{
	for (int i = 0; i < s_cube_proto.size(); ++i)
	{
		CUBE_RENEWAL_DATA* cubeData = s_cube_proto[i];

		const std::vector<CUBE_RENEWAL_VALUE>& rewards = cubeData->reward;

		if (1 != rewards.size())
		{
			sys_err("[CubeInfo] WARNING! Does not support multiple rewards (count: %d)", rewards.size());
			continue;
		}

		const CUBE_RENEWAL_VALUE& reward = rewards.at(0);
		const WORD& npcVNUM = cubeData->npc_vnum.at(0);

		TCubeMapByNPC& cubeMap = cube_info_map;
		TCubeResultList& resultList = cubeMap[npcVNUM];
		SCubeMaterialInfo materialInfo;

		materialInfo.reward = reward;
		materialInfo.gold = cubeData->gold;
		materialInfo.percent = cubeData->percent;
		materialInfo.material = cubeData->item;
		materialInfo.category = cubeData->category;

		resultList.push_back(materialInfo);
	}

	return true;
}

void Cube_open(LPCHARACTER ch)
{
	if (!ch || ch == NULL || ch == nullptr || !ch->GetDesc()) { return; } //lightworkfixme012
	LPCHARACTER	npc;
	npc = ch->GetQuestNPC();

	if (!npc)
		return;

	DWORD npcVNUM = npc->GetRaceNum();

	if (NULL == npc)
	{
		return;
	}

	if (FN_check_valid_npc(npcVNUM) == false)
	{
		return;
	}

	ch->m_newCubeNpc = npc;
	ch->WindowCloseAll();

	if (ch->GetExchange() ||
		ch->GetMyShop() ||
		ch->GetShopOwner() ||
		ch->IsOpenSafebox() ||
		ch->IsAcceOpened() ||
#ifdef ENABLE_AURA_SYSTEM
		ch->isAuraOpened(true) || ch->isAuraOpened(false) ||
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		ch->Is67AttrOpen() ||
#endif
		ch->IsOpenOfflineShop() ||
		ch->ActivateCheck(true))
	{
		ch->NewChatPacket(CLOSE_WINDOWS_ERROR);
		return;
	}

	long distance = DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY());

	if (distance >= CUBE_MAX_DISTANCE)
	{
		return;
	}

	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_CLEAR_DATES_RECEIVE);
	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_DATES_RECEIVE, npcVNUM);
	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_DATES_LOADING);
	SendDateCubeRenewalPackets(ch, CUBE_RENEWAL_SUB_HEADER_OPEN_RECEIVE);

	ch->SetCubeNpc(npc);
}

void Cube_close(LPCHARACTER ch)
{
	if (!ch || ch == NULL || ch == nullptr || !ch->GetDesc()) { return; }//lightworkfixme012
	ch->SetCubeNpc(ch->m_newCubeNpc);
	ch->m_newCubeNpc = nullptr;
}

void Cube_Make(LPCHARACTER ch, int index, int count_item)
{
	LPCHARACTER	npc;

	npc = ch->GetQuestNPC();

	if (!npc)
		return;
	
	 // Anti exploit: count_item negativ
    if (count_item < 0)
    {
        sys_err("[CUBE HACK] %s (%u) count_item < 0", ch->GetName(), ch->GetPlayerID());
        return;
    }

    // Anti exploit: index_item_improve modificat
    if (index_item_improve != -1)
    {
        sys_err("[CUBE HACK] %s (%u) index_item_improve != -1", ch->GetName(), ch->GetPlayerID());
        return;
    }

	if (!ch->IsCubeOpen())
	{
		ch->NewChatPacket(STRING_D82);
		return;
	}

	if (NULL == npc)
	{
		return;
	}

	const TCubeResultList& resultList = cube_info_map[npc->GetRaceNum()];

	if (index >= resultList.size())
		return;

	const SCubeMaterialInfo& materialInfo = resultList.at(index);

	bool isStack = true;
	TItemTable* p = ITEM_MANAGER::instance().GetTable(materialInfo.reward.vnum);
	if (p)
	{
		if (!(p->dwFlags & ITEM_FLAG_STACKABLE))
		{
			count_item = 1;
			isStack = false;
		}
	}


	for (int i = 0; i < materialInfo.material.size(); ++i)
	{
		if (ch->CountSpecifyItem(materialInfo.material[i].vnum) < (materialInfo.material[i].count * count_item))
		{
			ch->NewChatPacket(STRING_D68);
			return;
		}
	}

	if (materialInfo.gold != 0) 
	{
		if (ch->GetGold() < (materialInfo.gold * count_item))
		{
			ch->NewChatPacket(STRING_D83);
			return;
		}
	}

	int percent_number;
	int total_items_give = 0;

	for (int i = 0; i < count_item; ++i)
	{
		percent_number = number(1, 100);

		if (percent_number <= materialInfo.percent)
		{
			total_items_give++;
		}
	}

	if (total_items_give <= 0)
	{
		for (int i = 0; i < materialInfo.material.size(); ++i)
		{
			ch->RemoveSpecifyItem(materialInfo.material[i].vnum, (materialInfo.material[i].count * count_item));
		}

		if (materialInfo.gold != 0)
		{
			ch->PointChange(POINT_GOLD, -static_cast<long long>(materialInfo.gold * count_item), false);
		}
		if (total_items_give <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("It has failed."));
			sys_err("[CUBE FAIL] %s (%u) total_items_give <= 0",
				ch->GetName(),
				ch->GetPlayerID());
			return;
		}
		ch->NewChatPacket(STRING_D84);
		return;
	}

	if (!isStack)
	{
		LPITEM pItem = ITEM_MANAGER::instance().CreateItem(materialInfo.reward.vnum, (materialInfo.reward.count * total_items_give));

		BYTE window_type = ch->VnumGetWindowType(pItem->GetVnum());
		int iEmptyPos = ch->WindowTypeToGetEmpty(window_type, pItem);

		if (iEmptyPos < 0)
		{
			ch->NewChatPacket(STRING_D76);
			ITEM_MANAGER::instance().DestroyItem(pItem);
			return;
		}

#ifdef ENABLE_CUBE_SAVE_SOCKET_ATTRS
		if (materialInfo.allowCopyAttr)
		{
			for (int i = 0; i < INVENTORY_MAX_NUM; ++i)//Starting searchinf in inventory for Material Armor
			{
				LPITEM object = ch->GetInventoryItem(i);//Select Item via LPITEM
				if (!object)
					continue;

				if (object->GetType() == ITEM_WEAPON || object->GetType() == ITEM_ARMOR || object->GetType() == ITEM_GLOVE) // Check if is armor or weapon
				{
					if (object->GetVnum() == materialInfo.material[0].vnum) // Check if Select item is same item with crafting item
					{
						//Coppy Attributes
						pItem->ClearAttribute();
						for (int a = 0; a < ITEM_ATTRIBUTE_MAX_NUM; a++)
						{
							if (object->GetAttributeType(a) != 0)
							{
								pItem->SetForceAttribute(a, object->GetAttributeType(a), object->GetAttributeValue(a));
							}	
						}

						if (object->GetType() == ITEM_WEAPON || (object->GetType() == ITEM_ARMOR && object->GetSubType() == ARMOR_BODY))
						{
							for (int a = 0; a < ITEM_SOCKET_MAX_NUM; a++)
							{
								pItem->SetSocket(a, object->GetSocket(a));
							}
						}
						break;
					}
				}
			}
		}
#endif
		pItem->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
		ch->ItemWinnerChat(pItem->GetCount(), window_type, pItem->GetName());
	}
	else
	{
		if (!ch->AutoGiveItem(materialInfo.reward.vnum, (materialInfo.reward.count * total_items_give)))
			return;
	}

	for (int i = 0; i < materialInfo.material.size(); ++i)
	{
		ch->RemoveSpecifyItem(materialInfo.material[i].vnum, (materialInfo.material[i].count * count_item));
	}

	if (materialInfo.gold != 0)
	{
		ch->PointChange(POINT_GOLD, -static_cast<long long>(materialInfo.gold * count_item));
	}

#ifdef ENABLE_EXTENDED_BATTLE_PASS
	ch->UpdateExtBattlePassMissionProgress(BP_ITEM_CRAFT, (materialInfo.reward.count* total_items_give), materialInfo.reward.vnum);
#endif
#ifdef ENABLE_PLAYER_STATS_SYSTEM
	ch->PointChange(POINT_UPGRADE_ITEM, (materialInfo.reward.count* total_items_give));
#endif
}

void SendDateCubeRenewalPackets(LPCHARACTER ch, BYTE subheader, DWORD npcVNUM)
{
	TPacketGCCubeRenewalReceive pack;
	pack.subheader = subheader;

	if (subheader == CUBE_RENEWAL_SUB_HEADER_DATES_RECEIVE)
	{
		const TCubeResultList& resultList = cube_info_map[npcVNUM];
		for (TCubeResultList::const_iterator iter = resultList.begin(); resultList.end() != iter; ++iter)
		{
			const SCubeMaterialInfo& materialInfo = *iter;

			pack.date_cube_renewal.vnum_reward = materialInfo.reward.vnum;
			pack.date_cube_renewal.count_reward = materialInfo.reward.count;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(materialInfo.reward.vnum, materialInfo.reward.count);

			if (item->IsStackable() || !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK)) 
			{
				pack.date_cube_renewal.item_reward_stackable = true;
			}
			else 
			{
				pack.date_cube_renewal.item_reward_stackable = false;
			}

			pack.date_cube_renewal.vnum_material_1 = FN_check_cube_item_vnum_material(materialInfo, 1);
			pack.date_cube_renewal.count_material_1 = FN_check_cube_item_count_material(materialInfo, 1);
			pack.date_cube_renewal.vnum_material_2 = FN_check_cube_item_vnum_material(materialInfo, 2);
			pack.date_cube_renewal.count_material_2 = FN_check_cube_item_count_material(materialInfo, 2);
			pack.date_cube_renewal.vnum_material_3 = FN_check_cube_item_vnum_material(materialInfo, 3);
			pack.date_cube_renewal.count_material_3 = FN_check_cube_item_count_material(materialInfo, 3);
			pack.date_cube_renewal.vnum_material_4 = FN_check_cube_item_vnum_material(materialInfo, 4);
			pack.date_cube_renewal.count_material_4 = FN_check_cube_item_count_material(materialInfo, 4);
			pack.date_cube_renewal.vnum_material_5 = FN_check_cube_item_vnum_material(materialInfo, 5);
			pack.date_cube_renewal.count_material_5 = FN_check_cube_item_count_material(materialInfo, 5);

			pack.date_cube_renewal.vnum_material_6 = FN_check_cube_item_vnum_material(materialInfo, 6);
			pack.date_cube_renewal.count_material_6 = FN_check_cube_item_count_material(materialInfo, 6);
			pack.date_cube_renewal.vnum_material_7 = FN_check_cube_item_vnum_material(materialInfo, 7);
			pack.date_cube_renewal.count_material_7 = FN_check_cube_item_count_material(materialInfo, 7);
			pack.date_cube_renewal.vnum_material_8 = FN_check_cube_item_vnum_material(materialInfo, 8);
			pack.date_cube_renewal.count_material_8 = FN_check_cube_item_count_material(materialInfo, 8);
			pack.date_cube_renewal.vnum_material_9 = FN_check_cube_item_vnum_material(materialInfo, 9);
			pack.date_cube_renewal.count_material_9 = FN_check_cube_item_count_material(materialInfo, 9);
			pack.date_cube_renewal.vnum_material_10 = FN_check_cube_item_vnum_material(materialInfo, 10);
			pack.date_cube_renewal.count_material_10 = FN_check_cube_item_count_material(materialInfo, 10);

			pack.date_cube_renewal.gold = materialInfo.gold;
			pack.date_cube_renewal.percent = materialInfo.percent;

			memcpy(pack.date_cube_renewal.category, materialInfo.category.c_str(), sizeof(pack.date_cube_renewal.category));

			LPDESC d = ch->GetDesc();

			if (NULL == d)
			{
				sys_err("User SendDateCubeRenewalPackets (%s)'s DESC is NULL POINT.", ch->GetName());
				return;
			}

			d->Packet(&pack, sizeof(pack));
		}
	}
	else {
		LPDESC d = ch->GetDesc();

		if (NULL == d)
		{
			sys_err("User SendDateCubeRenewalPackets (%s)'s DESC is NULL POINT.", ch->GetName());
			return;
		}

		d->Packet(&pack, sizeof(pack));
	}
}