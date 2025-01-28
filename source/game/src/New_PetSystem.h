#pragma once
#ifdef ENABLE_NEW_PET_SYSTEM
class CNewPet
{
public:
	enum EPetItemInfo
	{
		BONUS_HP = 0,
		BONUS_STR = 1,
		BONUS_MONSTER = 2,
		
		PET_SOCKET_HP = 0,
		PET_SOCKET_STR = 1,
		PET_SOCKET_MONSTER = 2,
		PET_SOCKET_EXP = 3,
		PET_SOCKET_SKIN_COUNT = 4,
		PET_SOCKET_SKIN_VNUM = 5,

		PET_ATTR_EVOLUTION_AND_LEVEL = 0,//type evo value level
		PET_ATTR_STAR_AND_TYPE = 1,//type star value type

		PET_ATTR_SKILL_START,
		SKILL_MAX_COUNT = 8,

		PET_MAX_LEVEL = 120,
	};

	CNewPet(LPCHARACTER owner);
	~CNewPet();
	
	void Destroy();

	void Unsummon();
	bool Summon(LPITEM item);

	void Save();
	bool Update();
	bool IsSummon() { return m_pkChar ? true : false; }

	void EggOpen(LPITEM item);
	void Feed(LPITEM item);

	void PetEquip(LPITEM item, bool isEquip);
	void GiveBuff();
	void SendInformaitonsPacket();

protected:
	bool _UpdateFollowAI();

private:
	bool Follow(float fMinDistance = 50.f);

public:
	void SetLevel(BYTE level);
	BYTE GetLevel() { return m_level; }

	void SetExp(DWORD exp);
	DWORD GetExp() { return m_exp; }

	void IncreaseEvolution();
	BYTE GetEvolution() { return m_evolution; }

	void IncreaseType();
	BYTE GetType() { return m_type; }

	void IncreaseStar();
	BYTE GetStar() { return m_star; }

	void SetName();

	void IncreaseBonus();
	void BonusChange();
	void SkinChange(BYTE skinIdx);
	void ActivateSkin();

	void IncreaseSkillSlot(LPITEM item);

	void SetSkill(LPITEM book);

	LPCHARACTER GetPetChar() { return m_pkChar; }
	bool CanExp() { return m_canExp; }

	void SetExpRing(bool state) { m_expRing = state; }
private:
	BYTE m_evolution;
	BYTE m_type;
	BYTE m_star;
	BYTE m_level;
	DWORD m_exp;
	DWORD	m_nextExp;
	TPetSkill m_skill[EPetItemInfo::SKILL_MAX_COUNT];

	WORD			m_bonus[3];
	BYTE			m_skinCount;

	bool			m_canExp;
	bool			m_expRing;

	LPITEM			m_petItem;
	LPCHARACTER		m_pkChar;
	LPCHARACTER		m_pkOwner;
	LPEVENT			m_petUpdateEvent;
	DWORD			m_petVid;
};
#endif