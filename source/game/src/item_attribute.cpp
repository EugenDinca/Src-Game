#include "stdafx.h"
#include "constants.h"

#include "item.h"
#include "char.h"
#include "desc.h"
#include "item_manager.h"
#ifdef ENABLE_NEWSTUFF
#include "config.h"
#endif

#ifndef ENABLE_SWITCHBOT
const int32_t MAX_NORM_ATTR_NUM = ITEM_MANAGER::MAX_NORM_ATTR_NUM;
const int32_t MAX_RARE_ATTR_NUM = ITEM_MANAGER::MAX_RARE_ATTR_NUM;
#endif

#ifdef ENABLE_ATTR_RARE_RENEWAL
int CItem::GetAttributeSetIndex(bool rare)
#else
int CItem::GetAttributeSetIndex()
#endif
{
#ifdef ENABLE_ATTR_RARE_RENEWAL
	if (rare)
	{
		switch (GetType())
		{
#ifdef ENABLE_PET_SKIN_ATTR
			case ITEM_PET_SKIN:
#endif
#ifdef ENABLE_HERSEYE_ATTIR
			case ITEM_MOUNT_SKIN:
			case ITEM_AURA_SKIN:
#endif
			{
				return ATTRIBUTE_SET_RARE_SKIN;
			}
	
#ifdef ENABLE_HERSEYE_ATTIR
			case ITEM_COSTUME:
			{
				switch(GetSubType())
				{
					case COSTUME_BODY:
					case COSTUME_HAIR:
					case COSTUME_WEAPON:
					{
						return ATTRIBUTE_SET_RARE_COSTUME;
					}
					case COSTUME_MOUNT:
					{
						return ATTRIBUTE_SET_RARE_PET_AND_MOUNT;
					}
					case COSTUME_WING:
					{
						return ATTRIBUTE_SET_RARE_SKIN;
					}
					default: return -1;
				}
			}
			case ITEM_PET:
			{
				return ATTRIBUTE_SET_RARE_PET_AND_MOUNT;
			}
			case ITEM_GLOVE:
			{
				return ATTRIBUTE_SET_RARE_GLOVE;
			}
			case ITEM_CROWN:
			{
				return ATTRIBUTE_SET_RARE_CROWN;
			}
			case ITEM_BELT:
			{
				return ATTRIBUTE_SET_RARE_BELT;
			}
			case ITEM_DREAMSOUL:
			{
				return ATTRIBUTE_SET_RARE_DREAMSOUL;
			}
			case ITEM_PENDANT:
			{
				return ATTRIBUTE_SET_RARE_PENDANT;
			}
			case ITEM_SHINING:
			{
				return ATTRIBUTE_SET_RARE_SHININGS;
			}
			case ITEM_BOOSTER:
			{
				return ATTRIBUTE_SET_RARE_BOOSTERS;
			}
			case ITEM_RINGS:
			{
				return ATTRIBUTE_SET_RARE_RINGS;
			}
#endif

			default:
				break;
		}
		return -1;
	}
#endif
	if (GetType() == ITEM_WEAPON)
	{
		if (GetSubType() == WEAPON_ARROW)
			return -1;

		return ATTRIBUTE_SET_WEAPON;
	}

	if (GetType() == ITEM_ARMOR)
	{
		switch (GetSubType())
		{
		case ARMOR_BODY:
			return ATTRIBUTE_SET_BODY;

		case ARMOR_WRIST:
			return ATTRIBUTE_SET_WRIST;

		case ARMOR_FOOTS:
			return ATTRIBUTE_SET_FOOTS;

		case ARMOR_NECK:
			return ATTRIBUTE_SET_NECK;

		case ARMOR_HEAD:
			return ATTRIBUTE_SET_HEAD;

		case ARMOR_SHIELD:
			return ATTRIBUTE_SET_SHIELD;

		case ARMOR_EAR:
			return ATTRIBUTE_SET_EAR;
		}
	}
	else if (GetType() == ITEM_COSTUME)
	{
		switch (GetSubType())
		{
		case COSTUME_BODY:
#ifdef ENABLE_ITEM_ATTR_COSTUME
			return ATTRIBUTE_SET_COSTUME_BODY;
#else
			return ATTRIBUTE_SET_BODY;
#endif

		case COSTUME_HAIR:
#ifdef ENABLE_ITEM_ATTR_COSTUME
			return ATTRIBUTE_SET_COSTUME_HAIR;
#else
			return ATTRIBUTE_SET_HEAD;
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		case COSTUME_MOUNT:
			break;
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		case COSTUME_WEAPON:
#ifdef ENABLE_ITEM_ATTR_COSTUME
			return ATTRIBUTE_SET_COSTUME_WEAPON;
#else
			return ATTRIBUTE_SET_WEAPON;
#endif
#endif
		}
	}

	return -1;
}

bool CItem::HasAttr(BYTE bApply)
{
	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		if (m_pProto->aApplies[i].bType == bApply)
			return true;

	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
		if (GetAttributeType(i) == bApply)
			return true;

	return false;
}

bool CItem::HasRareAttr(BYTE bApply)
{
#ifdef ENABLE_ATTR_RARE_RENEWAL
	for (int i = 0; i < 5; ++i)
		if (GetAttributeType(i) == bApply)
			return true;
#else
	for (int i = 0; i < MAX_RARE_ATTR_NUM; ++i)
		if (GetAttributeType(i + 5) == bApply)
			return true;
#endif

	return false;
}

void CItem::AddAttribute(BYTE bApply, short sValue)
{
	if (HasAttr(bApply))
		return;

	int i = GetAttributeCount();

	if (i >= MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		if (sValue)
			SetAttribute(i, bApply, sValue);
	}
}

void CItem::AddAttr(BYTE bApply, BYTE bLevel)
{
	if (HasAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = GetAttributeCount();

	if (i == MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		const TItemAttrTable& r = g_map_itemAttr[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetAttribute(i, bApply, lVal);
	}
}

void CItem::PutAttributeWithLevel(BYTE bLevel)
{
	int iAttributeSet = GetAttributeSetIndex();
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable& r = g_map_itemAttr[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasAttr(i))
		{
			avail.push_back(i);
			total += r.dwProb;
		}
	}

	unsigned int prob = number(1, total);
	int attr_idx = APPLY_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable& r = g_map_itemAttr[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable& r = g_map_itemAttr[attr_idx];

	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddAttr(attr_idx, bLevel);
}

void CItem::PutAttribute(const int* aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutAttributeWithLevel(i + 1);
}

void CItem::ChangeAttribute(const int* aiChangeProb)
{
	int iAttributeCount = GetAttributeCount();

	ClearAttribute();

	if (iAttributeCount == 0)
		return;

	TItemTable const* pProto = GetProto();

	if (pProto && pProto->sAddonType)
	{
		ApplyAddon(pProto->sAddonType);
	}

	static const int tmpChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		0, 10, 40, 35, 15,
	};

	for (int i = GetAttributeCount(); i < iAttributeCount; ++i)
	{
		if (aiChangeProb == NULL)
		{
			PutAttribute(tmpChangeProb);
		}
		else
		{
			PutAttribute(aiChangeProb);
		}
	}
}

void CItem::AddAttribute()
{
	static const int aiItemAddAttributePercent[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		40, 50, 10, 0, 0
	};

	if (GetAttributeCount() < MAX_NORM_ATTR_NUM)
		PutAttribute(aiItemAddAttributePercent);
}

void CItem::ClearAttribute()
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		m_aAttr[i].bType = 0;
		m_aAttr[i].sValue = 0;
	}
}

int CItem::GetAttributeCount()
{
	int i;

	for (i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == 0)
			break;
	}

	return i;
}

int CItem::FindAttribute(BYTE bType)
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == bType)
			return i;
	}

	return -1;
}

bool CItem::RemoveAttributeAt(int index)
{
	if (GetAttributeCount() <= index)
		return false;

	for (int i = index; i < MAX_NORM_ATTR_NUM - 1; ++i)
	{
		SetAttribute(i, GetAttributeType(i + 1), GetAttributeValue(i + 1));
	}

	SetAttribute(MAX_NORM_ATTR_NUM - 1, APPLY_NONE, 0);
	return true;
}

bool CItem::RemoveAttributeType(BYTE bType)
{
	int index = FindAttribute(bType);
	return index != -1 && RemoveAttributeType(index);
}

void CItem::SetAttributes(const TPlayerItemAttribute* c_pAttribute)
{
	thecore_memcpy(m_aAttr, c_pAttribute, sizeof(m_aAttr));
	Save();
}

void CItem::SetAttribute(int i, BYTE bType, short sValue)
{
	assert(i < MAX_NORM_ATTR_NUM);

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();
}

void CItem::SetForceAttribute(int i, BYTE bType, short sValue)
{
	assert(i < ITEM_ATTRIBUTE_MAX_NUM);

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();
}

void CItem::CopyAttributeTo(LPITEM pItem)
{
	pItem->SetAttributes(m_aAttr);
}

int CItem::GetRareAttrCount()
{
	int ret = 0;

	for (DWORD dwIdx = ITEM_ATTRIBUTE_RARE_START; dwIdx < ITEM_ATTRIBUTE_RARE_END; dwIdx++)
	{
		if (m_aAttr[dwIdx].bType != 0)
			ret++;
	}

	return ret;
}

bool CItem::ChangeRareAttribute()
{
	if (GetRareAttrCount() == 0)
		return false;

	int count = GetRareAttrCount();

	for (int i = 0; i < count; ++i)
	{
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].bType = 0;
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].sValue = 0;
	}

	for (int i = 0; i < count; ++i)
	{
		AddRareAttribute();
	}

	return true;
}

bool CItem::AddRareAttribute()
{
	int count = GetRareAttrCount();

#ifdef ENABLE_ATTR_RARE_RENEWAL
	if (count >= 5)
#else
	if (count >= ITEM_ATTRIBUTE_RARE_NUM)
#endif
		return false;

	
#ifdef ENABLE_ATTR_RARE_RENEWAL
	int nAttrSet = GetAttributeSetIndex(true);
#else
	int nAttrSet = GetAttributeSetIndex();
#endif
	std::vector<int> avail;

	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const auto& it = g_map_itemRare.find(i);
		if (it == g_map_itemRare.end())
			continue;

		const auto& r = it->second;

		if (r.dwApplyIndex != 0 && r.bMaxLevelBySet[nAttrSet] > 0 && HasRareAttr(i) != true)
		{
			avail.push_back(i);
		}
	}

	const auto& it = g_map_itemRare.find(avail[number(0, avail.size() - 1)]);
	if (it == g_map_itemRare.end())
		return false;

	const auto& r = it->second;

	int nAttrLevel = number(1,5);

	if (nAttrLevel > r.bMaxLevelBySet[nAttrSet])
		nAttrLevel = r.bMaxLevelBySet[nAttrSet];

	TPlayerItemAttribute& attr = m_aAttr[count];
	attr.bType = r.dwApplyIndex;
	attr.sValue = r.lValues[nAttrLevel - 1];

	UpdatePacket();
	Save();

	return true;
}

void CItem::PutRareAttribute(const int* aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutRareAttributeWithLevel(i + 1);
}

void CItem::PutRareAttributeWithLevel(BYTE bLevel)
{
#ifdef ENABLE_ATTR_RARE_RENEWAL
	int iAttributeSet = GetAttributeSetIndex(true);
#else
	int iAttributeSet = GetAttributeSetIndex();
#endif
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	for (int i = 0; i < MAX_APPLY_NUM; ++i)
	{
		const TItemAttrRareTable& r = g_map_itemRare[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasRareAttr(i))
		{
			avail.push_back(i);
			total += r.dwProb;
		}
	}

	unsigned int prob = number(1, total);
	int attr_idx = APPLY_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrRareTable& r = g_map_itemRare[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item rare attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrRareTable& r = g_map_itemRare[attr_idx];

	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddRareAttr(attr_idx, bLevel);
}

void CItem::AddRareAttr(BYTE bApply, BYTE bLevel)
{
	if (HasRareAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = ITEM_ATTRIBUTE_RARE_START + GetRareAttrCount();

	if (i == ITEM_ATTRIBUTE_RARE_END)
		sys_err("item rare attribute overflow!");
	else
	{
		const TItemAttrRareTable& r = g_map_itemRare[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetForceAttribute(i, bApply, lVal);
	}
}

#ifdef ENABLE_6TH_7TH_ATTR
int CItem::Get67AttrIdx()
{
	switch (GetType())
	{
	case ITEM_ARMOR:
	{
		switch (GetSubType())
		{
			case ARMOR_BODY:
				return ATTRIBUTE_67TH_BODY;

			case ARMOR_HEAD:
				return ATTRIBUTE_67TH_HEAD;

			case ARMOR_SHIELD:
				return ATTRIBUTE_67TH_SHIELD;

			case ARMOR_WRIST:
				return ATTRIBUTE_67TH_WRIST;

			case ARMOR_FOOTS:
				return ATTRIBUTE_67TH_FOOTS;

			case ARMOR_NECK:
				return ATTRIBUTE_67TH_NECK;

			case ARMOR_EAR:
				return ATTRIBUTE_67TH_EAR;
		}
	}

	case ITEM_WEAPON:
		return ATTRIBUTE_67TH_WEAPON;

	case ITEM_PENDANT:
		return ATTRIBUTE_67TH_PENDANT;

	case ITEM_GLOVE:
		return ATTRIBUTE_67TH_GLOVE;

	}

	return -1;
}

int CItem::Get67AttrCount()
{
	int ret = 0;

	for (DWORD dwIdx = 5; dwIdx < 7; dwIdx++)
	{
		if (m_aAttr[dwIdx].bType != 0)
			ret++;
	}

	return ret;
}

int CItem::Get67MaterialVnum()
{
	switch (GetType())
	{
		case ITEM_ARMOR:
		{
			switch (GetSubType())
			{
			case ARMOR_BODY:
				return 39070;

			case ARMOR_HEAD:
				return 39071;

			case ARMOR_SHIELD:
				return 39072;

			case ARMOR_WRIST:
				return 39073;

			case ARMOR_FOOTS:
				return 39074;

			case ARMOR_NECK:
				return 39075;

			case ARMOR_EAR:
				return 39076;
			}
		}

		case ITEM_WEAPON:
			return 39077;

		case ITEM_PENDANT:
			return 39078;

		case ITEM_GLOVE:
			return 39079;
	}

	return 0;
}

void CItem::Add67Attr()
{
	const map67Attr::iterator it = map_item67ThAttr.find(Get67AttrIdx());
	if (it == map_item67ThAttr.end())
		return;

	const std::vector<T67AttrTable>& attrVec = it->second;

	BYTE attrLevel = number(0, 4);
	while (true)
	{
		size_t attrIdx = number(0, (attrVec.size() - 1));
		if (GetAttributeType(5) == attrVec[attrIdx].attrType)
			continue;

		if (GetAttributeType(5) == 0)
			SetForceAttribute(5, attrVec[attrIdx].attrType, attrVec[attrIdx].attrValue[attrLevel]);
		else
			SetForceAttribute(6, attrVec[attrIdx].attrType, attrVec[attrIdx].attrValue[attrLevel]);
		break;
	}
}

void CItem::Clear67Attribute()
{
	for (int i = 5; i < 7; ++i)
	{
		m_aAttr[i].bType = 0;
		m_aAttr[i].sValue = 0;
	}
}

void CItem::Change67Attr()
{
	const map67Attr::iterator it = map_item67ThAttr.find(Get67AttrIdx());
	if (it == map_item67ThAttr.end())
		return;

	const std::vector<T67AttrTable>& attrVec = it->second;

	
	Clear67Attribute();
	do
	{
		BYTE attrLevel = number(0, 4);
		size_t attrIdx = number(0, attrVec.size() - 1);

		if (GetAttributeType(5) == attrVec[attrIdx].attrType)
			continue;

		if (GetAttributeType(5) == 0)
			SetForceAttribute(5, attrVec[attrIdx].attrType, attrVec[attrIdx].attrValue[attrLevel]);
		else
			SetForceAttribute(6, attrVec[attrIdx].attrType, attrVec[attrIdx].attrValue[attrLevel]);

	} while (GetAttributeType(6) == 0);
}
#endif