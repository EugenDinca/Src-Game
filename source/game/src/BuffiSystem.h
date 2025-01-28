#pragma once
#ifdef ENABLE_BUFFI_SYSTEM
class CBuffiSystem
{
public:
	CBuffiSystem(LPCHARACTER owner);
	~CBuffiSystem();
	void Initialize();
	void Destroy();

	void Summon();
	void UnSummon();
	bool IsSummon() { return m_buffi ? true : false; };

	void StartUpdateEvent();
	void StopUpdateEvent();
	bool Update();

	void UseSkill();
	BYTE GetSkillLevel(BYTE skillIdx) { return m_skillLevel[skillIdx]; }
	void UpdateSkillLevel(int updateSkillVnum = -1);

	void SetName();

	void SetLastSkillTime(DWORD time) { m_lastSkillTime = time; }
	DWORD GetLastSkillTime() { return m_lastSkillTime; }

	void UpdateBuffiItem();
protected:
	bool UpdateFallowAi();

private:
	bool Follow(float fMinDistance = 50.f);

	LPCHARACTER	m_buffi;
	LPCHARACTER	m_owner;

	DWORD m_buffiVID;

	LPEVENT		m_buffiUpdateEvent;

	BYTE m_skillLevel[5];
	DWORD m_lastSkillTime;
	bool m_isNameChanged;
};
#endif

