#include "stdafx.h"

#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#include "desc.h"
#include "item.h"
#include "item_manager.h"
#include "char_manager.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "char.h"
#include "config.h"
#include "p2p.h"

bool ValidPosition(uint32_t wCell)
{
	return wCell < SWITCHBOT_SLOT_COUNT;
}


const float c_fSpeed = 0.15f;
#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
const float c_fPremiumSpeed = 0.10f;
#endif

bool SwitchbotHelper::IsValidItem(LPITEM pkItem)
{
	if (!pkItem)
	{
		return false;
	}
#ifdef ENABLE_SOULBIND_SYSTEM
	if (pkItem->IsSealed())
	{
		return false;
	}
#endif
	if (pkItem->IsEquipped()) {
		return false; // lightwork equipped items cant be added and also window cheks
	}
	switch (pkItem->GetType())
	{
	case ITEM_WEAPON:
		switch (pkItem->GetSubType())
		{
		case WEAPON_SWORD:
		case WEAPON_DAGGER:
		case WEAPON_BOW:
		case WEAPON_TWO_HANDED:
		case WEAPON_BELL:
		case WEAPON_FAN:
		case WEAPON_CLAW:
			return true;
		}

	case ITEM_ARMOR:
		switch (pkItem->GetSubType())
		{
		case ARMOR_BODY:
		case ARMOR_HEAD:
		case ARMOR_SHIELD:
		case ARMOR_WRIST:
		case ARMOR_FOOTS:
		case ARMOR_NECK:
		case ARMOR_EAR:
			return true;
		}

	default:
		return false;
	}
}

CSwitchbot::CSwitchbot()
{
	m_pkSwitchEvent = nullptr;
	m_table = {};
	m_isWarping = false;
}

CSwitchbot::~CSwitchbot()
{
	if (m_pkSwitchEvent)
	{
		event_cancel(&m_pkSwitchEvent);
		m_pkSwitchEvent = nullptr;
	}

	m_table = {};
	m_isWarping = false;
}

void CSwitchbot::SetTable(TSwitchbotTable table)
{
	m_table = table;
}

TSwitchbotTable CSwitchbot::GetTable()
{
	return m_table;
}

void CSwitchbot::SetPlayerId(uint32_t player_id)
{
	m_table.player_id = player_id;
}

uint32_t CSwitchbot::GetPlayerId(uint32_t player_id)
{
	return m_table.player_id;
}

void CSwitchbot::RegisterItem(WORD wCell, uint32_t item_id)
{
	if (!ValidPosition(wCell))
	{
		return;
	}

	m_table.items[wCell] = item_id;
}

void CSwitchbot::UnregisterItem(WORD wCell)
{
	if (!ValidPosition(wCell))
	{
		return;
	}

	m_table.items[wCell] = 0;
	m_table.active[wCell] = false;
	m_table.finished[wCell] = false;
	memset(&m_table.alternatives[wCell], 0, sizeof(m_table.alternatives[wCell]));
}

void CSwitchbot::SetAttributes(uint8_t slot, std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives)
{
	if (!ValidPosition(slot))
	{
		return;
	}

	for (uint8_t alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
	{
		for (uint8_t attrIdx = 0; attrIdx < MAX_NORM_ATTR_NUM; ++attrIdx)
		{
			m_table.alternatives[slot][alternative].attributes[attrIdx].bType = vec_alternatives[alternative].attributes[attrIdx].bType;
			m_table.alternatives[slot][alternative].attributes[attrIdx].sValue = vec_alternatives[alternative].attributes[attrIdx].sValue;
		}
	}
}

void CSwitchbot::SetActive(uint8_t slot, bool active)
{
	if (!ValidPosition(slot))
	{
		return;
	}

	m_table.active[slot] = active;
	m_table.finished[slot] = false;

	if (active)
	{
		const uint32_t item_id = m_table.items[slot];
		LPITEM pkItem = ITEM_MANAGER::Instance().Find(item_id);
		if (!pkItem)
		{
			return;
		}
		pkItem->ChangeAttribute();
	}
}

EVENTINFO(TSwitchbotEventInfo)
{
	CSwitchbot* pkSwitchbot;
	uint32_t player_id;
	int32_t oyuncuYok;

	TSwitchbotEventInfo() : pkSwitchbot(nullptr), player_id(0), oyuncuYok(0)
	{
	}
};

EVENTFUNC(switchbot_event)
{
	TSwitchbotEventInfo* info = dynamic_cast<TSwitchbotEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("switchbot_event> <Factor> Info Null pointer");
		return 0;
	}

	if (!info->pkSwitchbot)
	{
		sys_err("switchbot_event> <Factor> Switchbot Null pointer");
		return 0;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(info->player_id);

	if (ch)
	{
		info->oyuncuYok = 0;
		info->pkSwitchbot->SwitchItems();
#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		if (ch->IsPremium())
			return PASSES_PER_SEC(c_fPremiumSpeed);
#endif
	}
	else
	{
		info->oyuncuYok++;
		if (info->oyuncuYok >= 100)
		{
			CSwitchbotManager::Instance().Destroy(info->player_id);
			return 0;
		}
	}
	
	return PASSES_PER_SEC(c_fSpeed);
}

void CSwitchbot::Start()
{
	TSwitchbotEventInfo* info = AllocEventInfo<TSwitchbotEventInfo>();
	info->pkSwitchbot = this;
	info->player_id = m_table.player_id;

	m_pkSwitchEvent = event_create(switchbot_event, info, c_fSpeed);

	CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
}

void CSwitchbot::Stop()
{
	if (m_pkSwitchEvent)
	{
		event_cancel(&m_pkSwitchEvent);
		m_pkSwitchEvent = nullptr;
	}

	memset(&m_table.active, 0, sizeof(m_table.active));

	CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
}

void CSwitchbot::Pause()
{
	if (m_pkSwitchEvent)
	{
		event_cancel(&m_pkSwitchEvent);
		m_pkSwitchEvent = nullptr;
	}
}

bool CSwitchbot::IsActive(uint8_t slot)
{
	if (!ValidPosition(slot))
	{
		return false;
	}

	return m_table.active[slot];
}

bool CSwitchbot::HasActiveSlots()
{
	for (const auto& it : m_table.active)
	{
		if (it)
		{
			return true;
		}
	}

	return false;
}

bool CSwitchbot::IsSwitching()
{
	return m_pkSwitchEvent != nullptr;
}

bool CSwitchbot::IsWarping()
{
	return m_isWarping;
}

void CSwitchbot::SetIsWarping(bool warping)
{
	m_isWarping = warping;
}

void CSwitchbot::SwitchItems()
{
	for (uint8_t bSlot = 0; bSlot < SWITCHBOT_SLOT_COUNT; ++bSlot)
	{
		if (!m_table.active[bSlot])
		{
			continue;
		}

		m_table.finished[bSlot] = false;

		const uint32_t item_id = m_table.items[bSlot];

		LPITEM pkItem = ITEM_MANAGER::Instance().Find(item_id);
		if (!pkItem)
		{
			continue;
		}

		LPCHARACTER pkOwner = pkItem->GetOwner();
		if (!pkOwner)
		{
			return;
		}

		if (pkOwner->WindowOpenCheck() || pkOwner->ActivateCheck())
		{
			return;
		}

		if (CheckItem(pkItem, bSlot))
		{
			LPDESC desc = pkOwner->GetDesc();
			if (desc)
			{
				char buf[255];
				int32_t len = snprintf(buf, sizeof(buf), LC_TEXT("Bonuschange of %s (Slot: %d) successfully finished."), pkItem->GetName(), bSlot + 1);

				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_SYSTEM;
				pack.wSize = sizeof(TPacketGCWhisper) + len;
				strlcpy(pack.szNameFrom, "[Switchbot]", sizeof(pack.szNameFrom));
				pkOwner->GetDesc()->BufferedPacket(&pack, sizeof(pack));
				pkOwner->GetDesc()->Packet(buf, len);
				SendItemUpdate(pkOwner, bSlot, pkItem);
			}
			SetActive(bSlot, false);

			m_table.finished[bSlot] = true;

			if (!HasActiveSlots())
			{
				Stop();
			}
			else
			{
				CSwitchbotManager::Instance().SendSwitchbotUpdate(m_table.player_id);
			}
		}
		else
		{
			bool stop = true;
			if (SWITCHBOT_PRICE_TYPE == 1)
			{
				for (const auto& itemVnum : c_arSwitchingItems)
				{
					//CHECK_LIMITED_ITEM START
					if (itemVnum == 71151) // Yesil efsunlar
					{
						if ((pkItem->GetType() == ITEM_WEAPON) || (pkItem->GetType() == ITEM_ARMOR && pkItem->GetSubType() == ARMOR_BODY))
						{
							bool bCanUse = true;
							for (int32_t i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
							{
								if (pkItem->GetLimitType(i) == LIMIT_LEVEL && pkItem->GetLimitValue(i) > 40)
								{
									bCanUse = false;
									break;
								}
							}
							if (false == bCanUse)
							{
								continue;
							}
						}
						else
						{
							continue;
						}
					}
					//CHECK_LIMITED_ITEM END

					if (pkOwner->CountSpecifyItem(itemVnum) >= SWITCHBOT_PRICE_AMOUNT)
					{
						stop = false;
						pkOwner->RemoveSpecifyItem(itemVnum, SWITCHBOT_PRICE_AMOUNT);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
						pkOwner->PointChange(POINT_USE_ENCHANTED_ITEM, 1);
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
						pkOwner->UpdateExtBattlePassMissionProgress(BP_ITEM_USE, 1, 71084);
#endif
						break;
					}
				}
			}
			else if (SWITCHBOT_PRICE_TYPE == 2)
			{
				if (pkOwner->GetGold() >= SWITCHBOT_PRICE_AMOUNT)
				{
					stop = false;
					pkOwner->PointChange(POINT_GOLD, -static_cast<long long>(SWITCHBOT_PRICE_AMOUNT));
				}
			}

			if (stop)
			{
				SendItemUpdate(pkOwner, bSlot, pkItem);
				Stop();

				if (SWITCHBOT_PRICE_TYPE == 1)
				{
					pkOwner->NewChatPacket(SWITCH_BOT_STOP_INFO_1);
				}
				else
				{
					pkOwner->NewChatPacket(SWITCH_BOT_STOP_INFO_2);
				}

				return;
			}

			pkItem->ChangeAttribute();
			//SendItemUpdate(pkOwner, bSlot, pkItem);
		}
	}
}

bool CSwitchbot::CheckItem(LPITEM pkItem, uint8_t slot)
{
	if (!ValidPosition(slot))
	{
		return false;
	}

	if (!pkItem)
	{
		return false;
	}

	bool checked = 0;

	for (const auto& alternative : m_table.alternatives[slot])
	{
		if (!alternative.IsConfigured())
		{
			continue;
		}

		uint8_t configuredAttrCount = 0;
		uint8_t correctAttrCount = 0;

		for (const auto& destAttr : alternative.attributes)
		{
			if (!destAttr.bType || !destAttr.sValue)
			{
				continue;
			}

			++configuredAttrCount;

			for (uint8_t attrIdx = 0; attrIdx < MAX_NORM_ATTR_NUM; ++attrIdx)
			{
				const TPlayerItemAttribute& curAttr = pkItem->GetAttribute(attrIdx);

				if (curAttr.bType != destAttr.bType || curAttr.sValue < destAttr.sValue)
				{
					continue;
				}

				++correctAttrCount;
				break;
			}
		}

		checked = true;

		if (configuredAttrCount == correctAttrCount)
		{
			return true;
		}
	}

	if (!checked)
	{
		return true;
	}

	return false;
}

void CSwitchbot::SendItemUpdate(LPCHARACTER ch, uint8_t slot, LPITEM item)
{
	LPDESC desc = ch->GetDesc();
	if (!desc)
	{
		return;
	}

	TPacketGCSwitchbot pack;
	pack.header = HEADER_GC_SWITCHBOT;
	pack.subheader = SUBHEADER_GC_SWITCHBOT_UPDATE_ITEM;
	pack.size = sizeof(TPacketGCSwitchbot) + sizeof(TSwitchbotUpdateItem);

	TSwitchbotUpdateItem update = {};
	update.slot = slot;
	update.vnum = item->GetVnum();
	update.count = item->GetCount();

	thecore_memcpy(update.alSockets, item->GetSockets(), sizeof(update.alSockets));
	thecore_memcpy(update.aAttr, item->GetAttributes(), sizeof(update.aAttr));

	desc->BufferedPacket(&pack, sizeof(pack));
	desc->Packet(&update, sizeof(TSwitchbotUpdateItem));
}

CSwitchbotManager::CSwitchbotManager()
{
	Initialize();
}

CSwitchbotManager::~CSwitchbotManager()
{
	Initialize();
}

void CSwitchbotManager::Initialize()
{
	for (decltype(m_map_Switchbots)::const_iterator iter = m_map_Switchbots.begin(); iter != m_map_Switchbots.end(); ++iter)
		delete iter->second;
	m_map_Switchbots.clear();
}

void CSwitchbotManager::RegisterItem(uint32_t player_id, uint32_t item_id, WORD wCell)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		pkSwitchbot = new CSwitchbot();
		pkSwitchbot->SetPlayerId(player_id);
		m_map_Switchbots.insert(std::make_pair(player_id, pkSwitchbot));
	}

	if (pkSwitchbot->IsWarping())
	{
		return;
	}

	pkSwitchbot->RegisterItem(wCell, item_id);
	SendSwitchbotUpdate(player_id);
}

void CSwitchbotManager::UnregisterItem(uint32_t player_id, WORD wCell)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		return;
	}

	if (pkSwitchbot->IsWarping())
	{
		return;
	}

	pkSwitchbot->UnregisterItem(wCell);
	SendSwitchbotUpdate(player_id);
}

void CSwitchbotManager::Start(uint32_t player_id, uint8_t slot, std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives)
{
	if (!ValidPosition(slot))
	{
		return;
	}

	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		sys_err("No Switchbot found for player_id %u slot %d", player_id, slot);
		return;
	}

	if (pkSwitchbot->IsActive(slot))
	{
		sys_err("Switchbot slot %d already running for player_id %u", slot, player_id);
		return;
	}

	pkSwitchbot->SetActive(slot, true);
	pkSwitchbot->SetAttributes(slot, vec_alternatives);

	if (pkSwitchbot->HasActiveSlots() && !pkSwitchbot->IsSwitching())
	{
		pkSwitchbot->Start();
	}
	else
	{
		SendSwitchbotUpdate(player_id);
	}
}

void CSwitchbotManager::Stop(uint32_t player_id, uint8_t slot)
{
	if (!ValidPosition(slot))
	{
		return;
	}

	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		sys_err("No Switchbot found for player_id %u slot %d", player_id, slot);
		return;
	}

	if (!pkSwitchbot->IsActive(slot))
	{
		sys_err("Switchbot slot %d is not running for player_id %u", slot, player_id);
		return;
	}

	pkSwitchbot->SetActive(slot, false);

	if (!pkSwitchbot->HasActiveSlots() && pkSwitchbot->IsSwitching())
	{
		pkSwitchbot->Stop();
	}
	else
	{
		SendSwitchbotUpdate(player_id);
	}
}

bool CSwitchbotManager::IsActive(uint32_t player_id, uint8_t slot)
{
	if (!ValidPosition(slot))
	{
		return false;
	}

	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		return false;
	}

	return pkSwitchbot->IsActive(slot);
}

bool CSwitchbotManager::IsWarping(uint32_t player_id)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		return false;
	}

	return pkSwitchbot->IsWarping();
}

void CSwitchbotManager::SetIsWarping(uint32_t player_id, bool warping)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		return;
	}

	pkSwitchbot->SetIsWarping(warping);
}

CSwitchbot* CSwitchbotManager::FindSwitchbot(uint32_t player_id)
{
	const auto& it = m_map_Switchbots.find(player_id);
	if (it == m_map_Switchbots.end())
	{
		return nullptr;
	}

	return it->second;
}

void CSwitchbotManager::P2PSendSwitchbot(uint32_t player_id, WORD wTargetPort)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		//sys_err("No switchbot found to transfer. (pid %d source_port %d target_port %d)", player_id, mother_port, wTargetPort);
		return;
	}

	pkSwitchbot->Pause();
	TPacketGGSwitchbot pack;
	pack.wPort = wTargetPort;
	pack.table = pkSwitchbot->GetTable();

	m_map_Switchbots.erase(player_id);
	delete pkSwitchbot;
	pkSwitchbot = NULL;

	P2P_MANAGER::Instance().Send(&pack, sizeof(pack));
}

void CSwitchbotManager::P2PReceiveSwitchbot(TSwitchbotTable table)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(table.player_id);
	if (!pkSwitchbot)
	{
		pkSwitchbot = new CSwitchbot();
		m_map_Switchbots.insert(std::make_pair(table.player_id, pkSwitchbot));
	}

	pkSwitchbot->SetTable(table);
}

void CSwitchbotManager::SendItemAttributeInformations(LPCHARACTER ch)
{
	if (!ch)
	{
		return;
	}

	LPDESC desc = ch->GetDesc();
	if (!desc)
	{
		return;
	}

	TPacketGCSwitchbot pack;
	pack.header = HEADER_GC_SWITCHBOT;
	pack.subheader = SUBHEADER_GC_SWITCHBOT_SEND_ATTRIBUTE_INFORMATION;
	pack.size = sizeof(TPacketGCSwitchbot);

	TEMP_BUFFER buf;
	for (uint8_t bAttributeSet = 0; bAttributeSet < ATTRIBUTE_SET_MAX_NUM; ++bAttributeSet)
	{
		for (int32_t iApplyNum = 0; iApplyNum < MAX_APPLY_NUM; ++iApplyNum)
		{
			const TItemAttrTable& r = g_map_itemAttr[iApplyNum];

			uint8_t max = r.bMaxLevelBySet[bAttributeSet];
			if (max > 0)
			{
				TSwitchbottAttributeTable table = {};
				table.attribute_set = bAttributeSet;
				table.apply_num = iApplyNum;
				table.max_value = r.lValues[max - 1];

				buf.write(&table, sizeof(table));
			}
		}
	}

	if (buf.size())
	{
		pack.size += buf.size();
		desc->BufferedPacket(&pack, sizeof(pack));
		desc->Packet(buf.read_peek(), buf.size());
	}
	else
	{
		desc->Packet(&pack, sizeof(pack));
	}
}

void CSwitchbotManager::SendSwitchbotUpdate(uint32_t player_id)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (!pkSwitchbot)
	{
		return;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindByPID(player_id);
	if (!ch)
	{
		return;
	}

	LPDESC desc = ch->GetDesc();
	if (!desc)
	{
		return;
	}

	TSwitchbotTable table = pkSwitchbot->GetTable();

	TPacketGCSwitchbot pack;
	pack.header = HEADER_GC_SWITCHBOT;
	pack.subheader = SUBHEADER_GC_SWITCHBOT_UPDATE;
	pack.size = sizeof(TPacketGCSwitchbot) + sizeof(TSwitchbotTable);

	desc->BufferedPacket(&pack, sizeof(pack));
	desc->Packet(&table, sizeof(table));

	for (int i = 0; i < SWITCHBOT_SLOT_COUNT; i++)
	{
		const uint32_t item_id = table.items[i];

		LPITEM pkItem = ITEM_MANAGER::Instance().Find(item_id);
		if (!pkItem)
		{
			continue;
		}

		pkSwitchbot->SendItemUpdate(ch, i, pkItem);
	}

}

void CSwitchbotManager::EnterGame(LPCHARACTER ch)
{
	SendItemAttributeInformations(ch);
	SetIsWarping(ch->GetPlayerID(), false);
	SendSwitchbotUpdate(ch->GetPlayerID());

	CSwitchbot* pkSwitchbot = FindSwitchbot(ch->GetPlayerID());
	if (pkSwitchbot && pkSwitchbot->HasActiveSlots() && !pkSwitchbot->IsSwitching())
	{
		pkSwitchbot->Start();
	}
}

void CSwitchbotManager::Destroy(uint32_t player_id)
{
	CSwitchbot* pkSwitchbot = FindSwitchbot(player_id);
	if (pkSwitchbot)
	{
		pkSwitchbot->Pause();
		M2_DELETE(pkSwitchbot);
		m_map_Switchbots.erase(player_id);
	}
}

#endif