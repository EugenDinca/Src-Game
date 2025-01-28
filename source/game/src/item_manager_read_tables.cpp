#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "db.h"

#include "skill.h"
#include "text_file_loader.h"
#include "priv_manager.h"
#include "questmanager.h"
#include "unique_item.h"
#include "safebox.h"
#include "blend_item.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "item_manager_private_types.h"
#include "group_text_parse_tree.h"

bool ITEM_MANAGER::ReadSpecialDropItemFile(const char* c_pszFileName
#ifdef ENABLE_RELOAD_REGEN
	, bool isReloading
#endif
)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	std::map<DWORD, CSpecialAttrGroup*> tempSpecAttr;
	std::map<DWORD, CSpecialItemGroup*> tempSpecItem;
	std::map<DWORD, CSpecialItemGroup*> tempSpecItemQuest;
	if (isReloading)
		sys_log(0, "RELOADING SpecialDrop");

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("ReadSpecialDropItemFile : Syntax error %s : no vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		TTokenVector* pTok;

		//
		std::string stType;
		int type = CSpecialItemGroup::NORMAL;
		if (loader.GetTokenString("type", &stType))
		{
			stl_lowers(stType);
			if (stType == "pct")
			{
				type = CSpecialItemGroup::PCT;
			}
#ifdef ENABLE_CHEST_OPEN_RENEWAL
			else if (stType == "const") {//Sandýðýn içinde ne varsa atar
				type = CSpecialItemGroup::CR_CONST;
			}
			else if (stType == "stone") {//newpct aynýsýdýr tek fark açýlma sayýsý daha yüksektir
				type = CSpecialItemGroup::CR_STONE;
			}
			else if (stType == "newpct") {//pct türü gibidir
				type = CSpecialItemGroup::CR_NEW_PCT;
			}
			else if (stType == "norm") {//Normal sandýk açýmýyla aynýdýr
				type = CSpecialItemGroup::CR_NORM;
			}
#endif
			else if (stType == "quest")
			{
				type = CSpecialItemGroup::QUEST;
				quest::CQuestManager::instance().RegisterNPCVnum(iVnum);
			}
			else if (stType == "special")
			{
				type = CSpecialItemGroup::SPECIAL;
			}
		}

		if ("attr" == stType)
		{
			CSpecialAttrGroup* pkGroup = M2_NEW CSpecialAttrGroup(iVnum);
			for (int k = 1; k < 1024; ++k) // @fixme148 256 -> 1024
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					DWORD apply_type = 0;
					int	apply_value = 0;
					str_to_number(apply_type, pTok->at(0).c_str());
					if (0 == apply_type)
					{
						apply_type = FN_get_apply_type(pTok->at(0).c_str());
						if (0 == apply_type)
						{
							sys_err("Invalid APPLY_TYPE %s in Special Item Group Vnum %d", pTok->at(0).c_str(), iVnum);
							return false;
						}
					}
					str_to_number(apply_value, pTok->at(1).c_str());
					if (apply_type > MAX_APPLY_NUM)
					{
						sys_err("Invalid APPLY_TYPE %u in Special Item Group Vnum %d", apply_type, iVnum);
						M2_DELETE(pkGroup);
						return false;
					}
					pkGroup->m_vecAttrs.push_back(CSpecialAttrGroup::CSpecialAttrInfo(apply_type, apply_value));
				}
				else
				{
					break;
				}
			}
			if (loader.GetTokenVector("effect", &pTok))
			{
				pkGroup->m_stEffectFileName = pTok->at(0);
			}
			loader.SetParentNode();
#ifdef ENABLE_RELOAD_REGEN
			if (isReloading)
				tempSpecAttr.insert(std::make_pair(iVnum, pkGroup));
			else
				m_map_pkSpecialAttrGroup.insert(std::make_pair(iVnum, pkGroup));
#else
			m_map_pkSpecialAttrGroup.insert(std::make_pair(iVnum, pkGroup));
#endif

		}
		else
		{
			CSpecialItemGroup* pkGroup = M2_NEW CSpecialItemGroup(iVnum, type);
			for (int k = 1; k < 1024; ++k) // @fixme148 256 -> 1024
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					const std::string& name = pTok->at(0);
					DWORD dwVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwVnum))
					{
						if (name == "exp")
						{
							dwVnum = CSpecialItemGroup::EXP;
						}
						else if (name == "mob")
						{
							dwVnum = CSpecialItemGroup::MOB;
						}
						else if (name == "slow")
						{
							dwVnum = CSpecialItemGroup::SLOW;
						}
						else if (name == "drain_hp")
						{
							dwVnum = CSpecialItemGroup::DRAIN_HP;
						}
						else if (name == "poison")
						{
							dwVnum = CSpecialItemGroup::POISON;
						}
#ifdef ENABLE_WOLFMAN_CHARACTER
						else if (name == "bleeding")
						{
							dwVnum = CSpecialItemGroup::BLEEDING;
						}
#endif
						else if (name == "group")
						{
							dwVnum = CSpecialItemGroup::MOB_GROUP;
						}
						else
						{
							str_to_number(dwVnum, name.c_str());
							if (!ITEM_MANAGER::instance().GetTable(dwVnum))
							{
								sys_err("ReadSpecialDropItemFile : there is no item %s : node %s", name.c_str(), stName.c_str());
								M2_DELETE(pkGroup);

								return false;
							}
						}
					}

					int iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());
					int iProb = 0;
					str_to_number(iProb, pTok->at(2).c_str());

					int iRarePct = 0;
					if (pTok->size() > 3)
					{
						str_to_number(iRarePct, pTok->at(3).c_str());
					}

					sys_log(0, "        name %s count %d prob %d rare %d", name.c_str(), iCount, iProb, iRarePct);
					pkGroup->AddItem(dwVnum, iCount, iProb, iRarePct);

					// CHECK_UNIQUE_GROUP
					if (iVnum < 30000)
					{
						m_ItemToSpecialGroup[dwVnum] = iVnum;
					}
					// END_OF_CHECK_UNIQUE_GROUP

					continue;
				}

				break;
			}
			loader.SetParentNode();

#ifdef ENABLE_RELOAD_REGEN
			if (CSpecialItemGroup::QUEST == type)
			{
				if (isReloading)
					tempSpecItemQuest.insert(std::make_pair(iVnum, pkGroup));
				else
					m_map_pkQuestItemGroup.insert(std::make_pair(iVnum, pkGroup));
			}
			else
			{
				if (isReloading)
					tempSpecItem.insert(std::make_pair(iVnum, pkGroup));
				else
					m_map_pkSpecialItemGroup.insert(std::make_pair(iVnum, pkGroup));
					}
#else

			if (CSpecialItemGroup::QUEST == type)
				m_map_pkQuestItemGroup.insert(std::make_pair(iVnum, pkGroup));
			else
				m_map_pkSpecialItemGroup.insert(std::make_pair(iVnum, pkGroup));
#endif
		}
	}

#ifdef ENABLE_RELOAD_REGEN
	if (isReloading)
	{
		m_map_pkQuestItemGroup.clear();
		m_map_pkSpecialItemGroup.clear();
		m_map_pkSpecialAttrGroup.clear();

		for (std::map<DWORD, CSpecialAttrGroup*>::iterator it = tempSpecAttr.begin(); it != tempSpecAttr.end(); it++)
		{
			m_map_pkSpecialAttrGroup[it->first] = it->second;
		}

		for (std::map<DWORD, CSpecialItemGroup*>::iterator it = tempSpecItem.begin(); it != tempSpecItem.end(); it++)
		{
			m_map_pkSpecialItemGroup[it->first] = it->second;
		}

		for (std::map<DWORD, CSpecialItemGroup*>::iterator it = tempSpecItemQuest.begin(); it != tempSpecItemQuest.end(); it++)
		{
			m_map_pkQuestItemGroup[it->first] = it->second;
		}
	}

#endif

	return true;
}

bool ITEM_MANAGER::ConvSpecialDropItemFile()
{
	char szSpecialItemGroupFileName[256];
	snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
		"%s/special_item_group.txt", LocaleService_GetBasePath().c_str());

	FILE* fp = fopen("special_item_group_vnum.txt", "w");
	if (!fp)
	{
		sys_err("could not open file (%s)", "special_item_group_vnum.txt");
		return false;
	}

	CTextFileLoader loader;

	if (!loader.Load(szSpecialItemGroupFileName))
	{
		fclose(fp);
		return false;
	}

	std::string stName;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("ConvSpecialDropItemFile : Syntax error %s : no vnum, node %s", szSpecialItemGroupFileName, stName.c_str());
			loader.SetParentNode();
			fclose(fp);
			return false;
		}

		std::string str;
		int type = 0;
		if (loader.GetTokenString("type", &str))
		{
			stl_lowers(str);
			if (str == "pct")
				type = 1;
			// @fixme148 BEGIN
#ifdef ENABLE_CHEST_OPEN_RENEWAL
			else if (str == "const")
				type = 5;
			else if (str == "stone")
				type = 6;
			else if (str == "newpct")
				type = 7;
			else if (str == "norm")
				type = 8;
#endif
			else if (str == "quest")
				type = 2;
			else if (str == "special")
				type = 3;
			else if (str == "attr")
				type = 4;
			// @fixme148 END
		}

		TTokenVector* pTok;

		fprintf(fp, "Group	%s\n", stName.c_str());
		fprintf(fp, "{\n");
		fprintf(fp, "	Vnum	%i\n", iVnum);
		if (type == 1)
			fprintf(fp, "	Type	Pct\n");
		// @fixme148 BEGIN
#ifdef ENABLE_CHEST_OPEN_RENEWAL
		else if (type == 5)
			fprintf(fp, "	Type	Const\n");
		else if (type == 6)
			fprintf(fp, "	Type	Stone\n");
		else if (type == 7)
			fprintf(fp, "	Type	Newpct\n");
		else if (type == 8)
			fprintf(fp, "	Type	Norm\n");
#endif
		else if (type == 2)
			fprintf(fp, "	Type	Quest\n");
		else if (type == 3)
			fprintf(fp, "	Type	special\n");
		else if (type == 4)
			fprintf(fp, "	Type	ATTR\n");
		// @fixme148 END

		for (int k = 1; k < 1024; ++k) // @fixme148 256 -> 1024
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				std::string& name = pTok->at(0);
				DWORD dwVnum = 0;

				if (!GetVnumByOriginalName(name.c_str(), dwVnum))
				{
					if (
						name == "exp" ||
						name == "mob" ||
						name == "slow" ||
						name == "drain_hp" ||
						name == "poison" ||
#ifdef ENABLE_WOLFMAN_CHARACTER
						name == "bleeding" ||
#endif
						name == "group")
					{
						dwVnum = 0;
					}
					// @fixme148 BEGIN
					else if (name == "°æÇèÄ¡")
					{
						dwVnum = 0;
						name = "exp";
					}
					// @fixme148 END
					else
					{
						str_to_number(dwVnum, name.c_str());
						if (!ITEM_MANAGER::instance().GetTable(dwVnum) && type != 4)
						{
							sys_err("ReadSpecialDropItemFile : there is no item %s : node %s", name.c_str(), stName.c_str());
							fclose(fp);

							return false;
						}
					}
				}

				int iCount = 0;
				str_to_number(iCount, pTok->at(1).c_str());
				int iProb = 0;
				// @fixme148 BEGIN
				if (pTok->size() > 2)
					str_to_number(iProb, pTok->at(2).c_str());
				// @fixme148 END

				int iRarePct = 0;
				if (pTok->size() > 3)
					str_to_number(iRarePct, pTok->at(3).c_str());

				// @fixme148 BEGIN
				if (type == 4)
					fprintf(fp, "	%d	%u	%d\n", k, dwVnum, iCount);
				// @fixme148 END
				else
				{
					if (0 == dwVnum)
						fprintf(fp, "	%d	%s	%d	%d\n", k, name.c_str(), iCount, iProb);
					else
						fprintf(fp, "	%d	%u	%d	%d\n", k, dwVnum, iCount, iProb);
				}

				continue;
			}

			break;
		}
		std::string effect;
		if (loader.GetTokenString("effect", &effect))
			fprintf(fp, "	effect	\"%s\"\n", effect.c_str());
		fprintf(fp, "}\n");
		fprintf(fp, "\n");

		loader.SetParentNode();
	}

	fclose(fp);
	return true;
}

bool ITEM_MANAGER::ReadMonsterDropItemGroup(const char* c_pszFileName
#ifdef ENABLE_RELOAD_REGEN
	, bool isReloading
#endif
)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

#ifdef ENABLE_RELOAD_REGEN
	std::map<DWORD, CDropItemGroup*> tempDropItemGr;
	if (isReloading)
		sys_log(0, "RELOADING MonsterDrop");
#endif

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		std::string stName("");

		loader.GetCurrentNodeName(&stName);

		if (strncmp(stName.c_str(), "kr_", 3) == 0)
			continue;

		loader.SetChildNode(i);

		int iMobVnum = 0;

		std::string strType("");

		if (!loader.GetTokenString("type", &strType))
		{
			sys_err("ReadMonsterDropItemGroup : Syntax error %s : no type (kill|drop), node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		if (!loader.GetTokenInteger("mob", &iMobVnum))
		{
			sys_err("ReadMonsterDropItemGroup : Syntax error %s : no mob vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		sys_log(0, "MOB_ITEM_GROUP %s [%s] %d", stName.c_str(), strType.c_str(), iMobVnum);

		TTokenVector* pTok = NULL;

		if (strType == "drop")
		{
			CDropItemGroup* pkGroup;
			bool bNew = true;

#ifdef ENABLE_RELOAD_REGEN

			if (isReloading)
			{
				itertype(tempDropItemGr) it = tempDropItemGr.find(iMobVnum);
				if (it == tempDropItemGr.end())
				{
					pkGroup = M2_NEW CDropItemGroup(0, iMobVnum, stName);
				}
				else
				{
					bNew = false;
					pkGroup = it->second;
				}
			}
			else
			{
				itertype(m_map_pkDropItemGroup) it = m_map_pkDropItemGroup.find(iMobVnum);
				if (it == m_map_pkDropItemGroup.end())
				{
					pkGroup = M2_NEW CDropItemGroup(0, iMobVnum, stName);
				}
				else
				{
					bNew = false;
					pkGroup = it->second;
				}
			}

#else
			itertype(m_map_pkDropItemGroup) it = m_map_pkDropItemGroup.find(iMobVnum);

			if (it == m_map_pkDropItemGroup.end())
			{
				pkGroup = M2_NEW CDropItemGroup(0, iMobVnum, stName);
			}
			else
			{
				bNew = false;
				pkGroup = it->second;
			}
#endif

			for (int k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					std::string& name = pTok->at(0);
					DWORD dwVnum = 0;

					if (!GetVnumByOriginalName(name.c_str(), dwVnum))
					{
						str_to_number(dwVnum, name.c_str());
						if (!ITEM_MANAGER::instance().GetTable(dwVnum))
						{
							sys_err("ReadDropItemGroup : there is no item %s : node %s", name.c_str(), stName.c_str());
							M2_DELETE(pkGroup);

							return false;
						}
					}

					int iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());

					if (iCount < 1)
					{
						sys_err("ReadMonsterDropItemGroup : there is no count for item %s : node %s", name.c_str(), stName.c_str());
						M2_DELETE(pkGroup);

						return false;
					}

					float fPercent = atof(pTok->at(2).c_str());

					DWORD dwPct = (DWORD)(10000.0f * fPercent);

					sys_log(0, "        name %s vnum %d pct %d count %d", name.c_str(), dwVnum, dwPct, iCount);
					pkGroup->AddItem(dwVnum, dwPct, iCount);

					continue;
				}

				break;
			}
#ifdef ENABLE_RELOAD_REGEN
			if (bNew)
			{
				if (isReloading)
					tempDropItemGr.insert(std::map<DWORD, CDropItemGroup*>::value_type(iMobVnum, pkGroup));
				else
					m_map_pkDropItemGroup.insert(std::map<DWORD, CDropItemGroup*>::value_type(iMobVnum, pkGroup));
			}
#else
			if (bNew)
				m_map_pkDropItemGroup.insert(std::map<DWORD, CDropItemGroup*>::value_type(iMobVnum, pkGroup));
#endif
		}
		else
		{
			sys_err("ReadMonsterDropItemGroup : Syntax error %s : invalid type %s (kill|drop), node %s", c_pszFileName, strType.c_str(), stName.c_str());
			loader.SetParentNode();
			return false;
		}

		loader.SetParentNode();
	}

#ifdef ENABLE_RELOAD_REGEN
	if (isReloading)
	{
		for (std::map<DWORD, CDropItemGroup*>::iterator it = m_map_pkDropItemGroup.begin(); it != m_map_pkDropItemGroup.end(); it++)
			M2_DELETE(it->second);
		m_map_pkDropItemGroup.clear();

		for (std::map<DWORD, CDropItemGroup*>::iterator it = tempDropItemGr.begin(); it != tempDropItemGr.end(); it++)
		{
			m_map_pkDropItemGroup[it->first] = it->second;
		}
	}
#endif

	return true;
}

bool ITEM_MANAGER::ReadItemVnumMaskTable(const char* c_pszFileName)
{
	FILE* fp = fopen(c_pszFileName, "r");
	if (!fp)
	{
		return false;
	}

	int ori_vnum, new_vnum;
	while (fscanf(fp, "%u %u", &ori_vnum, &new_vnum) != EOF)
	{
		m_map_new_to_ori.insert(TMapDW2DW::value_type(new_vnum, ori_vnum));
	}
	fclose(fp);
	return true;
}