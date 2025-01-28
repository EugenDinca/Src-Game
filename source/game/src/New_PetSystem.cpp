#include "stdafx.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "New_PetSystem.h"
#include "packet.h"
#include "item_manager.h"
#include "item.h"
#include "desc.h"
#ifdef ENABLE_NEW_PET_SYSTEM
extern int passes_per_sec;
EVENTINFO(newpetsystem_event_info)
{
	CNewPet* pPetSystem;
};

EVENTFUNC(newpetsystem_update_event)
{
	newpetsystem_event_info* info = dynamic_cast<newpetsystem_event_info*>(event->info);
	if (!info)
	{
		sys_err("check_speedhack_event> <Factor> Null pointer");
		return 0;
	}

	CNewPet* pPetSystem = info->pPetSystem;

	if (!pPetSystem)
		return 0;

	if (!pPetSystem->Update())
		return 0;

	return PASSES_PER_SEC(1) / 4;
}

CNewPet::CNewPet(LPCHARACTER owner)
{
	m_pkOwner = owner;
	Destroy();
}

CNewPet::~CNewPet()
{
	if (IsSummon())
	{
		event_cancel(&m_petUpdateEvent);

		if (m_pkChar || CHARACTER_MANAGER::Instance().Find(m_petVid) != nullptr)
			M2_DESTROY_CHARACTER(m_pkChar);

		Destroy();
	}
}

void CNewPet::Destroy()
{
	m_petUpdateEvent = nullptr;

	m_pkChar = nullptr;
	m_petItem = nullptr;

	memset(&m_skill, 0, sizeof(m_skill));
	memset(&m_bonus, 0, sizeof(m_bonus));

	m_level = 1;
	m_exp = 0;
	m_nextExp = 1;
	m_evolution = 0;
	m_type = 0;
	m_star = 0;
	m_skinCount = 0;
	m_canExp = true;
	m_expRing = false;
	m_petVid = 0;
}

void CNewPet::Save()
{
	if (!m_petItem)
		return;

	m_petItem->Lock(false);
	m_petItem->SetSocket(EPetItemInfo::PET_SOCKET_EXP, m_exp);
	m_petItem->Save();
}

void CNewPet::PetEquip(LPITEM item, bool isEquip)
{
	if (isEquip)
	{
		if (Summon(item))
		{
			m_pkOwner->ComputePoints();
		}
		else
		{
			Destroy();
		}
	}
	else
	{
		if (IsSummon())
		{
			Unsummon();
		}
		else
		{
			Destroy();
		}
		m_pkOwner->ComputePoints();
	}
}

void CNewPet::EggOpen(LPITEM item)
{
	if (!item)
		return;

	LPITEM petItem = ITEM_MANAGER::instance().CreateItem(55701);

	if (petItem)
	{
		int iEmptyCell = item->GetCell();
		ITEM_MANAGER::instance().RemoveItem(item);
		if (m_pkOwner->IsEmptyItemGrid(TItemPos(INVENTORY, iEmptyCell), 1))
		{
			petItem->AddToCharacter(m_pkOwner, TItemPos(INVENTORY, iEmptyCell));
		}
		else
		{
			iEmptyCell = m_pkOwner->WindowTypeToGetEmpty(INVENTORY, petItem);
			if (iEmptyCell != -1)
			{
				petItem->AddToCharacter(m_pkOwner, TItemPos(INVENTORY, iEmptyCell));
			}
			else
			{
				m_pkOwner->AutoGiveItem(55401);
				m_pkOwner->NewChatPacket(NO_EMPTY_SPACE_IN_INVENTORY);
				return;
			}
		}

		petItem->SetSocket(EPetItemInfo::PET_SOCKET_HP, number(1, 30));
		petItem->SetSocket(EPetItemInfo::PET_SOCKET_STR, number(1, 30));
		petItem->SetSocket(EPetItemInfo::PET_SOCKET_MONSTER, number(1, 30));

		petItem->SetSocket(EPetItemInfo::PET_SOCKET_EXP, 0);

		petItem->SetSocket(EPetItemInfo::PET_SOCKET_SKIN_COUNT, 0);
		petItem->SetSocket(EPetItemInfo::PET_SOCKET_SKIN_VNUM, 34045);

		petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_EVOLUTION_AND_LEVEL, 0, 1);
		petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_STAR_AND_TYPE, 0, number(0, 5));

		for (int i = 3; i < 8; i++)
		{
			petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_SKILL_START + i, 0, 62);
		}

	}
}

void CNewPet::Feed(LPITEM item)
{
	if (!item || !m_petItem || m_level == PET_MAX_LEVEL)
		return;

	SetLevel(m_level + 1);
	return;

	const TItemTable* item_table = item->GetProto();

	if (!item_table)
		return;

	int increaseExp = static_cast<int>(item_table->alValues[0]);

	if ((m_level < 120 && item_table->alValues[1] != 0) || (m_level >= 120 && item_table->alValues[1] == 0))
	{
		m_pkOwner->ChatPacket(1, "Kullanilan nesne petin'in seviyesi icin uygun degil");
		return;
	}

	if (m_exp + increaseExp >= 100 && !EvolutionLevelCheck(m_level, m_evolution))
	{
		m_pkOwner->ChatPacket(1, "Petin seviyisini daha fazla gelistirebilmek icin evrimini yukseltmen gerek.");
		return;
	}

	SetExp(increaseExp);

	item->SetCount(item->GetCount() - 1);
}

bool CNewPet::Summon(LPITEM item)
{
	long x = m_pkOwner->GetX() + number(-100, 100);
	long y = m_pkOwner->GetY() + number(-100, 100);
	long z = m_pkOwner->GetZ();

	if (m_pkChar != nullptr || CHARACTER_MANAGER::Instance().Find(m_petVid) != nullptr)
	{
		Save();
		Destroy();
	}

	m_pkChar = CHARACTER_MANAGER::instance().SpawnMob(
		item->GetSocket(EPetItemInfo::PET_SOCKET_SKIN_VNUM),
		m_pkOwner->GetMapIndex(),
		x, y, z,
		false, (int)(m_pkOwner->GetRotation() + 180), false);

	if (!m_pkChar)
	{
		sys_err("[CPetSystem::Summon] Failed to summon the pet. (vnum: %d)", item->GetSocket(EPetItemInfo::PET_SOCKET_SKIN_VNUM));
		return false;
	}

	m_petVid = m_pkChar->GetVID();
	m_pkChar->SetNewPet();
	m_pkChar->SetEmpire(m_pkOwner->GetEmpire());
	SetName();

	m_petItem = item;

	SetLevel(static_cast<BYTE>(item->GetAttribute(EPetItemInfo::PET_ATTR_EVOLUTION_AND_LEVEL).sValue));
	SetExp(static_cast<int>(item->GetSocket(EPetItemInfo::PET_SOCKET_EXP)));
	

	m_evolution = static_cast<BYTE>(item->GetAttribute(EPetItemInfo::PET_ATTR_EVOLUTION_AND_LEVEL).bType);
	m_skinCount = static_cast<BYTE>(item->GetSocket(EPetItemInfo::PET_SOCKET_SKIN_COUNT));

	m_star = static_cast<BYTE>(item->GetAttribute(EPetItemInfo::PET_ATTR_STAR_AND_TYPE).bType);
	m_type = static_cast<BYTE>(item->GetAttribute(EPetItemInfo::PET_ATTR_STAR_AND_TYPE).sValue);

	m_bonus[EPetItemInfo::BONUS_HP] = static_cast<WORD>(item->GetSocket(EPetItemInfo::PET_SOCKET_HP));
	m_bonus[EPetItemInfo::BONUS_STR] = static_cast<WORD>(item->GetSocket(EPetItemInfo::PET_SOCKET_STR));
	m_bonus[EPetItemInfo::BONUS_MONSTER] = static_cast<WORD>(item->GetSocket(EPetItemInfo::PET_SOCKET_MONSTER));

	for (int i = 0; i < EPetItemInfo::SKILL_MAX_COUNT; i++)
	{
		TPlayerItemAttribute attr = item->GetAttribute(i + EPetItemInfo::PET_ATTR_SKILL_START);
		m_skill[i].type = static_cast<BYTE>(attr.bType);
		m_skill[i].level = static_cast<BYTE>(attr.sValue);
	}

	m_pkChar->Show(m_pkOwner->GetMapIndex(), x, y, z);

	SendInformaitonsPacket();

	m_petItem->Lock(true);

	if (m_pkOwner->IsEquipUniqueItem(55140))
		m_expRing = true;

	newpetsystem_event_info* info = AllocEventInfo<newpetsystem_event_info>();
	info->pPetSystem = this;
	m_petUpdateEvent = event_create(newpetsystem_update_event, info, PASSES_PER_SEC(1) / 3);

	return true;
}

void CNewPet::Unsummon()
{
	if (m_pkChar)
		M2_DESTROY_CHARACTER(m_pkChar);

	event_cancel(&m_petUpdateEvent);

	Save();
	Destroy();

	if (m_pkOwner->GetDesc())
	{
		TNewPetGCPacket p;
		p.header = HEADER_GC_NEW_PET;
		p.size = sizeof(TNewPetGCPacket);
		p.subHeader = SUB_GC_UNSUMMON;

		TEMP_BUFFER buf;
		buf.write(&p, sizeof(TNewPetGCPacket));

		m_pkOwner->GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}

void CNewPet::SendInformaitonsPacket()
{
	if (m_pkOwner->GetDesc())
	{
		TNewPetGCPacket p;
		p.header = HEADER_GC_NEW_PET;
		p.size = sizeof(TNewPetGCPacket) + sizeof(TNewPetInfo);
		p.subHeader = SUB_GC_UPDATE;

		TNewPetInfo info;
		memcpy(info.bonus, m_bonus, sizeof(m_bonus));
		memcpy(info.skill, m_skill, sizeof(m_skill));
		info.level = m_level;
		info.exp = m_exp;
		info.nextExp = m_nextExp;
		info.evolution = m_evolution;
		info.type = m_type;
		info.star = m_star;
		info.skinVnum = m_pkChar->GetRaceNum();
		info.skinCount = m_petItem->GetSocket(EPetItemInfo::PET_SOCKET_SKIN_COUNT);

		TEMP_BUFFER buf;
		buf.write(&p, sizeof(TNewPetGCPacket));
		buf.write(&info, sizeof(TNewPetInfo));
		m_pkOwner->GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}

void CNewPet::SetExp(DWORD exp)
{
	if (m_expRing)
	{
		exp += exp / 2;
	}
		
	if (m_exp + exp >= m_nextExp)
	{
		if (!EvolutionLevelCheck(m_level, m_evolution))
		{
			m_canExp = false;
			return;
		}
		m_exp += exp;
		m_exp -= m_nextExp;
		SetLevel(m_level + 1);
	}
	else
	{
		m_exp += exp;
		if (m_pkOwner->GetDesc())
		{
			TNewPetGCPacket p;
			p.header = HEADER_GC_NEW_PET;
			p.size = sizeof(TNewPetGCPacket) + sizeof(DWORD);
			p.subHeader = SUB_GC_UPDATE_EXP;

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(TNewPetGCPacket));
			buf.write(&m_exp, sizeof(DWORD));

			m_pkOwner->GetDesc()->Packet(buf.read_peek(), buf.size());
		}
	}
}

void CNewPet::SetLevel(BYTE level)
{
	if (m_pkChar->GetLevel() >= EPetItemInfo::PET_MAX_LEVEL)
		return;

	m_pkChar->SetLevel(level);

	m_nextExp = m_pkChar->GetNextExp();

	if (m_level > 1 && level > 1)
	{
		m_level = level;
		m_petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_EVOLUTION_AND_LEVEL, m_evolution, m_level);

		if (m_level % 4 == 0)
		{
			IncreaseBonus();
			m_pkOwner->ComputePoints();
		}

		SendInformaitonsPacket();
		m_pkChar->SendPetLevelUpEffect();
	}
	else
		m_level = level;

	m_pkChar->UpdatePacket();
}

void CNewPet::IncreaseBonus()
{
	for (int i = 0; i < 3; i++)
	{
		BYTE increamentValue = number(Pet_Change_Table[m_type][0], Pet_Change_Table[m_type][1]);
		m_bonus[i] += increamentValue;
		m_petItem->SetSocket(i, m_bonus[i]);
	}
}

void CNewPet::SetName()
{
	std::string petName = "";

	if (0 != m_pkOwner &&
		0 != m_pkOwner->GetName())
	{
		petName += m_pkOwner->GetName();
	}

	petName += "'s Pet";

	if (true == IsSummon())
		m_pkChar->SetName(petName);
}

void CNewPet::GiveBuff()
{
	if (!m_pkChar || !m_petItem)
		return;

	if (GetLevel() >= 80)
	{
		for (int i = 0; i < 8; i++)
		{
			if (m_skill[i].type == 0)
				break;

			switch (m_skill[i].type)
			{
				case 1:	//savasci"1"
				case 2:	//sura"2"
				case 3:	//ninja"3"
				case 4:	//shaman"4"
				case 5:	//Lycan"5"
				case 6:	//Berserker"6"
				case 7:	//Buyu Bozma"7"
				case 8: //Yari insan"8"
				case 9: //Delici"9"
				case 10: //Beceri"10"
				case 11: //Hp Absorbe"11"
				case 12: //Ortalama"12"
				case 13: //Bloklama"13"
				case 14: //Canavar"14"
				case 15: //Metin"15"
				{
					long long skillBonusValue = Pet_Skill_Table[m_skill[i].type - 1][m_skill[i].level] + ((Pet_Skill_Table[m_skill[i].type - 1][m_skill[i].level] * (m_star * 5)) / 100);
					m_pkOwner->PointChange(Pet_Skill_Table[m_skill[i].type - 1][0], skillBonusValue);
					break;
				}
				default:
					break;
			}
		}
	}

	long long bonus[3] = { m_pkOwner->GetMaxHP(), m_pkOwner->GetPoint(POINT_ATT_GRADE) , m_pkOwner->GetPoint(POINT_DEF_GRADE) };
	for (int i = 0; i < 3; i++)
	{
		long long giveBonus = ((m_bonus[i] + ((m_bonus[i] * Pet_Skin_Bonus_Value[m_skinCount]) / 100)) * bonus[i]) / 1000;
		m_pkOwner->PointChange(Pet_Bonus_Type[i], giveBonus);
	}
}

void CNewPet::IncreaseEvolution()
{
	if (GetEvolution() == 3)
		return;

	if (EvolutionLevelCheck(GetLevel(), GetEvolution()))
		return;

	int evoTabIdx = -1;

	for (auto i : Pet_Evolution_Table)
	{
		if (i[0] == GetLevel())
		{
			evoTabIdx = i[1] - 1;
			break;
		}
	}

	if (evoTabIdx == -1)
		return;

	if (m_pkOwner->CountSpecifyItem(Pet_Evolution_Table[evoTabIdx][2]) < Pet_Evolution_Table[evoTabIdx][3])
		return;

	m_evolution++;
	m_petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_EVOLUTION_AND_LEVEL, m_evolution, m_level);
	m_pkOwner->RemoveSpecifyItem(Pet_Evolution_Table[evoTabIdx][2], Pet_Evolution_Table[evoTabIdx][3]);
	SendInformaitonsPacket();
	m_canExp = true;
}

void CNewPet::IncreaseStar()
{
	if (GetEvolution() != 3 || GetStar() == 10 || GetLevel() < 80 || m_pkOwner->CountSpecifyItem(55128) < 1)
		return;

	if (number(0, 100) >= 70)
	{
		m_star++;
		m_petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_STAR_AND_TYPE, m_star, m_type);
		m_pkOwner->ChatPacket(1, LC_TEXT("Success."));
		SendInformaitonsPacket();
		m_pkOwner->ComputePoints();
	}
	else
	{
		m_pkOwner->ChatPacket(1, LC_TEXT("Failed."));
	}

	m_pkOwner->RemoveSpecifyItem(55128);
}

void CNewPet::IncreaseType()
{
	if (GetType() == 7)
		return;

	if (m_pkOwner->CountSpecifyItem(55125) < 1)
		return;

	int newPetType = 0;
	int change = number(0, 100);

	if (change >= 90)
		newPetType = 7;
	else if (change >= 80)
		newPetType = 6;
	else if (change >= 70)
		newPetType = 5;
	else if (change >= 60)
		newPetType = 4;
	else if (change >= 50)
		newPetType = 3;
	else if (change >= 35)
		newPetType = 2;
	else if (change >= 20)
		newPetType = 1;

	m_pkOwner->RemoveSpecifyItem(55125);

	m_type = newPetType;
	m_petItem->SetForceAttribute(EPetItemInfo::PET_ATTR_STAR_AND_TYPE, m_star, m_type);
	m_pkOwner->ChatPacket(1, LC_TEXT("Pet type changed new type : %d"), newPetType + 1);
	SendInformaitonsPacket();
}

void CNewPet::BonusChange()
{
	if (m_pkOwner->CountSpecifyItem(55127) < 1)
		return;

	WORD new_bonus[3];
	memset(new_bonus, 0, sizeof(new_bonus));

	if (GetLevel() < 4)
	{
		for (int i = 0; i < 3; i++)
		{
			BYTE increamentValue = number(1,30);
			new_bonus[i] += increamentValue;
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			BYTE increamentValue = number(5, 30);
			new_bonus[i] += increamentValue;
		}

		BYTE increamentCount = GetLevel() / 4;

		while (increamentCount > 0)
		{
			for (int i = 0; i < 3; i++)
			{
				BYTE increamentValue = number(Pet_Change_Table[m_type][0], Pet_Change_Table[m_type][1]);
				new_bonus[i] += increamentValue;
			}
			increamentCount--;
		}
	}

	memcpy(m_bonus, new_bonus, sizeof(new_bonus));
	for (int i = 0; i < 3; i++)
	{
		m_petItem->SetSocket(i, m_bonus[i]);
	}

	m_pkOwner->RemoveSpecifyItem(55127);
	m_pkOwner->ComputePoints();
	SendInformaitonsPacket();
}

void CNewPet::SkinChange(BYTE skinIdx)
{
	if (m_skinCount < skinIdx)
		return;

	m_petItem->SetSocket(EPetItemInfo::PET_SOCKET_SKIN_VNUM, Pet_Skin_Mob_Vnum_List[skinIdx]);
	Unsummon();

	if (Summon(m_pkOwner->GetWear(WEAR_NEW_PET)))
		m_pkOwner->ComputePoints();
	else
		sys_err("Skin change after summon error");
}

void CNewPet::SetSkill(LPITEM book)
{
	if (GetEvolution() < 3)
		return;

	if (!book || !book->GetProto())
		return;

	BYTE skillIndex = book->GetProto()->alValues[0];

	for (int i = 0; i < 8; i++)
	{
		if (m_skill[i].type == 0 || m_skill[i].type == skillIndex)
		{
			if (m_skill[i].level == 30 || m_skill[i].level == 62)
				return;

			if (book->GetCount() < 1)
				return;

			m_skill[i].level++;
			m_skill[i].type = skillIndex;

			m_petItem->SetForceAttribute(i + PET_ATTR_SKILL_START, skillIndex, m_skill[i].level);
			book->SetCount(book->GetCount() - 1);

			SendInformaitonsPacket();
			m_pkOwner->ComputePoints();
			break;
		}
	}
}

void CNewPet::ActivateSkin()
{
	if (m_skinCount == 5)
		return;

	if (m_pkOwner->CountSpecifyItem(Pet_Skin_Item_Vnum_List[m_skinCount]) < 1)
		return;

	m_pkOwner->RemoveSpecifyItem(Pet_Skin_Item_Vnum_List[m_skinCount]);

	m_skinCount++;
	m_petItem->SetSocket(EPetItemInfo::PET_SOCKET_SKIN_COUNT, m_skinCount);
	SendInformaitonsPacket();

	m_pkOwner->ComputePoints();
}

void CNewPet::IncreaseSkillSlot(LPITEM item)
{
	if (!item)
		return;

	if (!IsSummon())
	{
		m_pkOwner->NewChatPacket(PET_TEXT_1);
		return;
	}

	if (GetEvolution() != 3)
	{
		m_pkOwner->NewChatPacket(PET_TEXT_2, "80");
		return;
	}

	for (int i = 3; i < 8; i++)
	{
		if (m_skill[i].level == 62)
		{
			if (item->GetCount() < 1)
				return;

			m_skill[i].level = 0;
			m_petItem->SetForceAttribute(i + PET_ATTR_SKILL_START, 0, 0);
			m_pkOwner->NewChatPacket(PET_TEXT_3);
			item->SetCount(item->GetCount() - 1);
			SendInformaitonsPacket();
			break;
		}
	}
}

/**************************UPDATE*****************************************/
bool CNewPet::Update()
{
	if (IsSummon() && m_pkOwner && CHARACTER_MANAGER::instance().Find(m_petVid))
	{
		if (_UpdateFollowAI())
			return true;
	}

	event_cancel(&m_petUpdateEvent);
	m_petUpdateEvent = nullptr;
	return false;
}

bool CNewPet::Follow(float fMinDistance)
{
	if (!m_pkOwner || !m_pkChar)
		return false;

	float fOwnerX = m_pkOwner->GetX();
	float fOwnerY = m_pkOwner->GetY();

	float fPetX = m_pkChar->GetX();
	float fPetY = m_pkChar->GetY();

	float fDist = DISTANCE_SQRT(fOwnerX - fPetX, fOwnerY - fPetY);
	if (fDist <= fMinDistance)
		return false;

	m_pkChar->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy;

	float fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_pkChar->GetRotation(), fDistToGo, &fx, &fy);

	if (!m_pkChar->Goto((int)(fPetX + fx + 0.5f), (int)(fPetY + fy + 0.5f)))
		return false;

	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

bool CNewPet::_UpdateFollowAI()
{
	if (!m_pkOwner || !m_pkChar)
		return false;

	if (0 == m_pkChar->m_pkMobData)
	{
		return false;
	}

	float	START_FOLLOW_DISTANCE = 300.0f;
	float	START_RUN_DISTANCE = 900.0f;

	float	RESPAWN_DISTANCE = 4500.f;
	int		APPROACH = 290;
	bool bRun = false;

	DWORD currentTime = get_dword_time();

	long ownerX = m_pkOwner->GetX();
	long ownerY = m_pkOwner->GetY();
	long charX = m_pkChar->GetX();
	long charY = m_pkChar->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_pkOwner->GetRotation() * 3.141592f / 180.f;
		float fx = -APPROACH * cos(fOwnerRot);
		float fy = -APPROACH * sin(fOwnerRot);
		if (m_pkChar->Show(m_pkOwner->GetMapIndex(), ownerX + fx, ownerY + fy))
		{
			return true;
		}
	}

	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if (fDist >= START_RUN_DISTANCE)
		{
			bRun = true;
		}

		m_pkChar->SetNowWalking(!bRun);

		Follow(APPROACH);

		m_pkChar->SetLastAttacked(currentTime);
	}
	else
		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	return true;
}
#endif