#include "stdafx.h"
#ifdef ENABLE_BUFFI_SYSTEM
#include "BuffiSystem.h"
#include "char.h"
#include "packet.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "desc.h"
#include "utils.h"
#include "vector.h"
#include "skill.h"
#include "item.h"

const DWORD DefaultBuffiItem[BUFFI_MAX_SLOT]{
	36009,
	36002,
	25,
	0,
	0
};

CBuffiSystem::CBuffiSystem(LPCHARACTER owner)
	:m_owner(owner)
{
	Initialize();
}

CBuffiSystem::~CBuffiSystem()
{
	Destroy();
	m_owner = nullptr;
}

void CBuffiSystem::Destroy()
{
	if (!m_owner || !m_owner->IsPC())
		return;

	m_buffi = nullptr;
	m_buffiUpdateEvent = nullptr;
}


void CBuffiSystem::Initialize()
{
	memset(m_skillLevel, 0, sizeof(m_skillLevel));
	m_buffi = nullptr;
	m_buffiUpdateEvent = nullptr;
	m_buffiVID = 0;
	m_lastSkillTime = 0;
	m_isNameChanged = false;
}

void CBuffiSystem::Summon()
{
	if (!m_owner)
		return;

	if (m_owner->FindAffect(AFFECT_NAMING_SCROLL_BUFFI))
	{
		m_isNameChanged = true;
	}

	if (m_skillLevel[0] == 0)
	{
		UpdateSkillLevel();
	}

	long x = m_owner->GetX() + number(-100, 100);
	long y = m_owner->GetY() + number(-100, 100);
	long z = m_owner->GetZ();


	m_buffi = CHARACTER_MANAGER::instance().SpawnMob(
		62,
		m_owner->GetMapIndex(),
		x, y, z,
		false, (int)(m_owner->GetRotation() + 180), false);

	if (!m_buffi)
	{
		sys_err("Failed to summon the buffi");
		return;
	}

	m_buffi->SetBuffi(true);
	m_buffi->SetEmpire(m_owner->GetEmpire());
	SetName();
	UpdateBuffiItem();
	m_buffiVID = m_buffi->GetVID();
	m_buffi->Show(m_owner->GetMapIndex(), x, y, z);
	StartUpdateEvent();
}


void CBuffiSystem::UnSummon()
{
	if (!m_owner)
		return;

	StopUpdateEvent();
	if (m_buffi != nullptr && CHARACTER_MANAGER::instance().Find(m_buffiVID))
		M2_DESTROY_CHARACTER(m_buffi);

	m_buffiVID = 0;
}

void CBuffiSystem::SetName()
{
	std::string buffiName = "";

#ifdef ENABLE_NAMING_SCROLL
	if (m_owner->FindAffect(AFFECT_NAMING_SCROLL_BUFFI))
	{
		buffiName = m_owner->GetMobNameScroll(BUFFI_NAME_NUM);
	}
	else
	{
		if (0 != m_owner &&
			0 != m_owner->GetName())
		{
			buffiName += m_owner->GetName();
		}

		buffiName += "'s Buffi";
	}
#else
	if (0 != m_owner &&
		0 != m_owner->GetName())
	{
		buffiName += m_owner->GetName();
	}

	buffiName += "'s Buffi";
#endif

	if (IsSummon())
		m_buffi->SetName(buffiName);
}

void CBuffiSystem::UseSkill()
{
	if (!m_owner || !m_buffi)
		return;

	if (m_owner->IsDead())
		return;

	if (!m_owner->IsAffectFlag(AFF_HOSIN))
	{
		m_buffi->SetPoint(POINT_IQ, 160);
		m_buffi->ComputeSkill(SKILL_HOSIN, m_owner, GetSkillLevel(0));
		m_buffi->SendBuffiSkillPacket(SKILL_HOSIN);
	}
	else if (!m_owner->IsAffectFlag(AFF_BOHO))
	{
		m_buffi->SetPoint(POINT_IQ, 160);
		m_buffi->ComputeSkill(SKILL_REFLECT, m_owner, GetSkillLevel(1));
		m_buffi->SendBuffiSkillPacket(SKILL_REFLECT);
	}
	else if (!m_owner->IsAffectFlag(AFF_GICHEON))
	{
		m_buffi->SetPoint(POINT_IQ, 160);
		m_buffi->ComputeSkill(SKILL_GICHEON, m_owner, GetSkillLevel(2));
		m_buffi->SendBuffiSkillPacket(SKILL_GICHEON);
	}
	else if (m_isNameChanged)
	{
		if (!m_owner->IsAffectFlag(AFF_KWAESOK))
		{
			m_buffi->SetPoint(POINT_IQ, 160);
			m_buffi->ComputeSkill(SKILL_KWAESOK, m_owner, GetSkillLevel(3));
			m_buffi->SendBuffiSkillPacket(SKILL_KWAESOK);
		}
		else if (!m_owner->IsAffectFlag(AFF_JEUNGRYEOK))
		{
			m_buffi->SetPoint(POINT_IQ, 160);
			m_buffi->ComputeSkill(SKILL_JEUNGRYEOK, m_owner, GetSkillLevel(4));
			m_buffi->SendBuffiSkillPacket(SKILL_JEUNGRYEOK);
		}
	}
	SetLastSkillTime(get_dword_time());
}


/*************************** UPDATE **************************/
void CBuffiSystem::UpdateBuffiItem()
{
	if (!m_buffi || !m_owner)
		return;

	for (int i = 0; i < BUFFI_MAX_SLOT; i++)
	{
		LPITEM item = m_owner->GetItem(TItemPos(BUFFI_INVENTORY, i));
		if (item)
		{
			if (BUFFI_HAIR_SLOT == i)
			{
				m_buffi->SetBuffiItem(i, item->GetValue(3));
				continue;
			}

			m_buffi->SetBuffiItem(i, item->GetVnum());
		}
		else
		{
			m_buffi->SetBuffiItem(i, DefaultBuffiItem[i]);
		}
	}
	m_buffi->UpdatePacket();
}

void CBuffiSystem::UpdateSkillLevel(int updateSkillVnum)
{
	if (!m_owner)
		return;

	memset(m_skillLevel, 0, sizeof(m_skillLevel));

	for (int i = 0; i < 5; i++)
	{
		m_skillLevel[i] = static_cast<BYTE>(m_owner->GetSkillLevel(SKILL_BUFFI_1 + i));
	}

	if (updateSkillVnum != 1)
	{
		switch (updateSkillVnum)
		{
			case SKILL_BUFFI_1:
				updateSkillVnum = SKILL_HOSIN;
				break;

			case SKILL_BUFFI_2:
				updateSkillVnum = SKILL_REFLECT;
				break;

			case SKILL_BUFFI_3:
				updateSkillVnum = SKILL_GICHEON;
				break;

			case SKILL_BUFFI_4:
				updateSkillVnum = SKILL_KWAESOK;
				break;

			case SKILL_BUFFI_5:
				updateSkillVnum = SKILL_JEUNGRYEOK;
				break;
		}
		m_owner->RemoveAffect(updateSkillVnum);
	}
}

extern int passes_per_sec;
EVENTINFO(buffiSystem_event_info)
{
	CBuffiSystem* pBuffiSystem;
};

EVENTFUNC(buffisystem_update_event)
{
	buffiSystem_event_info* info = dynamic_cast<buffiSystem_event_info*>(event->info);
	if (!info)
		return 0;

	CBuffiSystem* pBuffiSystem = info->pBuffiSystem;

	if (!pBuffiSystem)
		return 0;

	if (!pBuffiSystem->Update())
	{
		pBuffiSystem->UnSummon();
		return 0;
	}
		

	return PASSES_PER_SEC(1) / 4;
}

void CBuffiSystem::StartUpdateEvent()
{
	buffiSystem_event_info* info = AllocEventInfo<buffiSystem_event_info>();
	info->pBuffiSystem = this;
	m_buffiUpdateEvent = event_create(buffisystem_update_event, info, PASSES_PER_SEC(1) / 3);
}

void CBuffiSystem::StopUpdateEvent()
{
	event_cancel(&m_buffiUpdateEvent);
	m_buffiUpdateEvent = nullptr;
}

bool CBuffiSystem::Update()
{
	if (!m_owner || !m_buffi)
		return false;

	if (!CHARACTER_MANAGER::instance().Find(m_buffiVID))
		return false;

	if (IsSummon())
	{
		if (get_dword_time() - GetLastSkillTime() >= 3000)
		{
			UseSkill();
		}
		if (UpdateFallowAi())
		{
			return true;
		}
	}

	StopUpdateEvent();
	return false;
}

bool CBuffiSystem::Follow(float fMinDistance)
{
	if (!m_owner || !m_buffi)
		return false;

	float fOwnerX = m_owner->GetX();
	float fOwnerY = m_owner->GetY();

	float fBuffiX = m_buffi->GetX();
	float fBuffiY = m_buffi->GetY();

	float fDist = DISTANCE_SQRT(fOwnerX - fBuffiX, fOwnerY - fBuffiY);
	if (fDist <= fMinDistance)
		return false;

	m_buffi->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy;

	float fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_buffi->GetRotation(), fDistToGo, &fx, &fy);

	if (!m_buffi->Goto((int)(fBuffiX + fx + 0.5f), (int)(fBuffiY + fy + 0.5f)))
		return false;

	m_buffi->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

bool CBuffiSystem::UpdateFallowAi()
{
	if (!m_buffi || !m_owner)
		return false;

	if (0 == m_buffi->m_pkMobData)
	{
		return false;
	}

	float	START_FOLLOW_DISTANCE = 300.0f;
	float	START_RUN_DISTANCE = 900.0f;

	float	RESPAWN_DISTANCE = 4500.f;
	int		APPROACH = 290;
	bool bRun = false;

	DWORD currentTime = get_dword_time();

	long ownerX = m_owner->GetX();
	long ownerY = m_owner->GetY();
	long charX = m_buffi->GetX();
	long charY = m_buffi->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_owner->GetRotation() * 3.141592f / 180.f;
		float fx = -APPROACH * cos(fOwnerRot);
		float fy = -APPROACH * sin(fOwnerRot);
		if (m_buffi->Show(m_owner->GetMapIndex(), ownerX + fx, ownerY + fy))
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

		m_buffi->SetNowWalking(!bRun);

		Follow(APPROACH);

		m_buffi->SetLastAttacked(currentTime);
	}
	else
		m_buffi->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	return true;
}
#endif