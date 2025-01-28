#pragma once

#ifdef ENABLE_SWITCHBOT
#include "../../common/tables.h"

class SwitchbotHelper
{
public:
	static bool IsValidItem(LPITEM pkItem);
};

class CSwitchbot
{
public:
	CSwitchbot();
	~CSwitchbot();

	void SetTable(TSwitchbotTable table);
	TSwitchbotTable GetTable();

	void SetPlayerId(uint32_t player_id);
	uint32_t GetPlayerId(uint32_t player_id);

	void RegisterItem(WORD wCell, uint32_t item_id);
	void UnregisterItem(WORD wCell);
	void SetAttributes(uint8_t slot, std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives);

	void SetActive(uint8_t slot, bool active);
	bool IsActive(uint8_t slot);
	bool HasActiveSlots();
	bool IsSwitching();
	bool IsWarping();
	void SetIsWarping(bool warping);

	void Start();
	void Stop();
	void Pause();

	/*void StartEndEvent(uint32_t player_id);*/

	void SwitchItems();
	bool CheckItem(LPITEM pkItem, uint8_t slot);

	void SendItemUpdate(LPCHARACTER ch, uint8_t slot, LPITEM item);

protected:
	TSwitchbotTable m_table;
	LPEVENT m_pkSwitchEvent;
	bool m_isWarping;
};

class CSwitchbotManager : public singleton<CSwitchbotManager>
{
public:
	CSwitchbotManager();
	virtual ~CSwitchbotManager();

	void Initialize();
	void RegisterItem(uint32_t player_id, uint32_t item_id, WORD wCell);
	void UnregisterItem(uint32_t player_id, WORD wCell);
	void Start(uint32_t player_id, uint8_t slot, std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives);
	void Stop(uint32_t player_id, uint8_t slot);

	bool IsActive(uint32_t player_id, uint8_t slot);
	bool IsWarping(uint32_t player_id);
	void SetIsWarping(uint32_t player_id, bool warping);

	CSwitchbot* FindSwitchbot(uint32_t player_id);

	void P2PSendSwitchbot(uint32_t player_id, WORD wTargetPort);
	void P2PReceiveSwitchbot(TSwitchbotTable table);

	void SendItemAttributeInformations(LPCHARACTER ch);
	void SendSwitchbotUpdate(uint32_t player_id);

	void EnterGame(LPCHARACTER ch);
	void Destroy(uint32_t player_id);

protected:
	std::map<uint32_t, CSwitchbot*> m_map_Switchbots;
};
#endif