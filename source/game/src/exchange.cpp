#include "stdafx.h"
#include "../../libs/libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"

#include "db.h"
#include "locale_service.h"
#include "../../common/length.h"
#include "exchange.h"
#include "DragonSoul.h"
#include "questmanager.h" // @fixme150
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
#include "InGameLogManager.h"
#endif
#ifdef ENABLE_EXTENDED_YANG_LIMIT
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, int64_t arg1, TItemPos arg2, DWORD arg3, void* pvData = NULL);
#else
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, DWORD arg1, TItemPos arg2, DWORD arg3, void* pvData = NULL);
#endif

#ifdef ENABLE_EXTENDED_YANG_LIMIT
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, int64_t arg1, TItemPos arg2, DWORD arg3, void* pvData)
#else
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, DWORD arg1, TItemPos arg2, DWORD arg3, void* pvData)
#endif
{
	if (!ch->GetDesc())
		return;

	struct packet_exchange pack_exchg;

	pack_exchg.header = HEADER_GC_EXCHANGE;
	pack_exchg.sub_header = sub_header;
	pack_exchg.is_me = is_me;
	pack_exchg.arg1 = arg1;
	pack_exchg.arg2 = arg2;
	pack_exchg.arg3 = arg3;

	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD && pvData)
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(((LPITEM)pvData)->GetWindow(), ((LPITEM)pvData)->GetCell());
#endif
		thecore_memcpy(&pack_exchg.alSockets, ((LPITEM)pvData)->GetSockets(), sizeof(pack_exchg.alSockets));
		thecore_memcpy(&pack_exchg.aAttr, ((LPITEM)pvData)->GetAttributes(), sizeof(pack_exchg.aAttr));
	}
	else
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(RESERVED_WINDOW, 0);
#endif
		memset(&pack_exchg.alSockets, 0, sizeof(pack_exchg.alSockets));
		memset(&pack_exchg.aAttr, 0, sizeof(pack_exchg.aAttr));
	}
	ch->GetDesc()->Packet(&pack_exchg, sizeof(pack_exchg));
}

bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	if (this == victim)
		return false;

	if (IsObserverMode())
	{
		return false;
	}

	if (victim->IsNPC())
		return false;

	//PREVENT_TRADE_WINDOW
	if (IsOpenSafebox() || GetShopOwner() || GetMyShop() || IsCubeOpen() || IsAcceOpened() || IsOpenOfflineShop() || ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
		|| isAuraOpened(true) || isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| Is67AttrOpen()
#endif
#ifdef ENABLE_BOT_CONTROL
		|| IsBotControl()
#endif
		)
	{
		NewChatPacket(STRING_D85);
		return false;
	}

	if (victim->IsOpenSafebox() || victim->GetShopOwner() || victim->GetMyShop() || victim->IsCubeOpen() || victim->IsAcceOpened() || victim->IsOpenOfflineShop() || victim->ActivateCheck()
#ifdef ENABLE_AURA_SYSTEM
		|| victim->isAuraOpened(true) || victim->isAuraOpened(false)
#endif
#ifdef ENABLE_6TH_7TH_ATTR
		|| victim->Is67AttrOpen()
#endif
#ifdef ENABLE_BOT_CONTROL
		|| victim->IsBotControl()
#endif
		)
	{
		NewChatPacket(STRING_D86);
		return false;
	}
	//END_PREVENT_TRADE_WINDOW
	int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		exchange_packet(this, EXCHANGE_SUBHEADER_GC_ALREADY, 0, 0, NPOS, 0);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		NewChatPacket(STRING_D87);
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	//
	SetActivateTime(EXCHANGE_CHECK_TIME);
	victim->SetActivateTime(EXCHANGE_CHECK_TIME);

	exchange_packet(victim, EXCHANGE_SUBHEADER_GC_START, 0, GetVID(), NPOS, 0);
	exchange_packet(this, EXCHANGE_SUBHEADER_GC_START, 0, victim->GetVID(), NPOS, 0);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

	m_lGold = 0;

	m_pOwner = pOwner;
	pOwner->SetExchange(this);

#ifdef ENABLE_EXCHANGE_WINDOW_RENEWAL
	m_pGrid = M2_NEW CGrid(6, 4);
#else
	m_pGrid = M2_NEW CGrid(4, 3);
#endif
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

#if defined(ENABLE_CHECKINOUT_UPDATE)
int CExchange::GetEmptyExchange(BYTE size)
{
	for (unsigned int i = 0; m_pGrid && i < m_pGrid->GetSize(); i++)
		if (m_pGrid->IsEmpty(i, 1, size))
			return i;

	return -1;
}
bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos, bool SelectPosAuto)
#else
bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos)
#endif
{
	assert(m_pOwner != NULL && GetCompany());

	if (!item_pos.IsValidItemPosition())
		return false;

	if (item_pos.IsEquipPosition())
		return false;

	LPITEM item;

	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
	{
		m_pOwner->NewChatPacket(STRING_D88);
		return false;
	}

	if (true == item->isLocked())
	{
		return false;
	}

	if (item->IsExchanging())
	{
		return false;
	}

#if defined(ENABLE_CHECKINOUT_UPDATE)
	if (SelectPosAuto)
	{
		int AutoPos = GetEmptyExchange(item->GetSize());
		if (AutoPos == -1)
		{
			GetOwner()->NewChatPacket(STRING_D76);
			return false;
		}
		display_pos = AutoPos;
	}
#endif

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i] = item;
		m_aItemPos[i] = item_pos;
		m_abItemDisplayPos[i] = display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		exchange_packet(m_pOwner,
			EXCHANGE_SUBHEADER_GC_ITEM_ADD,
			true,
			item->GetVnum(),
			TItemPos(RESERVED_WINDOW, display_pos),
			item->GetCount(),
			item);

		exchange_packet(GetCompany()->GetOwner(),
			EXCHANGE_SUBHEADER_GC_ITEM_ADD,
			false,
			item->GetVnum(),
			TItemPos(RESERVED_WINDOW, display_pos),
			item->GetCount(),
			item);
		return true;
	}

	return false;
}

bool CExchange::RemoveItem(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, true, pos, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, false, pos, PosOfInventory, 0);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos] = NULL;
	m_aItemPos[pos] = NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

#ifdef ENABLE_EXTENDED_YANG_LIMIT
bool CExchange::AddGold(int64_t gold)
#else
bool CExchange::AddGold(long gold)
#endif
{
	if (gold <= 0)
		return false;

	if (GetOwner()->GetGold() < gold)
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_GOLD, 0, 0, NPOS, 0);
		return false;
	}

	if (m_lGold > 0)
		return false;

	Accept(false);
	GetCompany()->Accept(false);

	m_lGold = gold;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, true, m_lGold, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, false, m_lGold, NPOS, 0);
	return true;
}

bool CExchange::Check(int* piItemCount)
{
	if (GetOwner()->GetGold() < m_lGold)
		return false;

	int item_count = 0;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

bool CExchange::CheckSpace()
{
	static CGrid s_grid1(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_grid2(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 2
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	static CGrid s_grid3(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_grid4(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 4
	static CGrid s_grid5(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 5
#endif

	s_grid1.Clear();
	s_grid2.Clear();
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	s_grid3.Clear();
	s_grid4.Clear();
	s_grid5.Clear();
#endif

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;

	for (i = 0; i < INVENTORY_PAGE_SIZE * 1; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid1.Put(i, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE * 1; i < INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid2.Put(i - INVENTORY_PAGE_SIZE * 1, 1, item->GetSize());
	}
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	for (i = INVENTORY_PAGE_SIZE * 2; i < INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid3.Put(i - INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE * 3; i < INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid4.Put(i - INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}

	for (i = INVENTORY_PAGE_SIZE * 4; i < INVENTORY_PAGE_SIZE * 5; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;

		s_grid5.Put(i - INVENTORY_PAGE_SIZE * 4, 1, item->GetSize());
	}
#endif

	static std::vector <WORD> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);

	bool bDSInitialized = false;

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}

			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			WORD wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
			{
				WORD wPos = wBasePos + i;
				if (0 == s_vDSGrid[wPos]) // @fixme159 (wBasePos to wPos)
				{
					bool bEmpty = true;
					for (int j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] = wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
				return false;
		}
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
		{
			return true;
		}
		else if (item->IsBook())
		{
			return true;
		}
		else if (item->IsStone())
		{
			return true;
		}
		else if (item->IsChest())
		{
			return true;
		}
#endif
		else
		{
			int iPos;

			if ((iPos = s_grid1.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid1.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid2.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid2.Put(iPos, 1, item->GetSize());
			}
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
			else if ((iPos = s_grid3.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid3.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid4.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid4.Put(iPos, 1, item->GetSize());
			}
			else if ((iPos = s_grid5.FindBlank(1, item->GetSize())) >= 0)
			{
				s_grid5.Put(iPos, 1, item->GetSize());
			}
#endif
			else
				return false;
		}
	}

	return true;
}

#ifdef ENABLE_SPECIAL_STORAGE
bool CExchange::CheckSpaceUpgradeInventory()
{
	static CGrid s_upp_grid1(5, 9);
	static CGrid s_upp_grid2(5, 9);
	static CGrid s_upp_grid3(5, 9);
	static CGrid s_upp_grid4(5, 9);
	static CGrid s_upp_grid5(5, 9);

	s_upp_grid1.Clear();
	s_upp_grid2.Clear();
	s_upp_grid3.Clear();
	s_upp_grid4.Clear();
	s_upp_grid5.Clear();

	LPCHARACTER victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;
	for (i = 0; i < SPECIAL_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetUpgradeInventoryItem(i)))
			continue;

		s_upp_grid1.Put(i, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE; i < SPECIAL_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetUpgradeInventoryItem(i)))
			continue;

		s_upp_grid2.Put(i - SPECIAL_INVENTORY_PAGE_SIZE, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetUpgradeInventoryItem(i)))
			continue;

		s_upp_grid3.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetUpgradeInventoryItem(i)))
			continue;

		s_upp_grid4.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 4; i < SPECIAL_INVENTORY_PAGE_SIZE * 5; ++i)
	{
		if (!(item = victim->GetUpgradeInventoryItem(i)))
			continue;

		s_upp_grid5.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 4, 1, item->GetSize());
	}

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsUpgradeItem())
		{
			int iPos = s_upp_grid1.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_upp_grid1.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_upp_grid2.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_upp_grid2.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_upp_grid3.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_upp_grid3.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_upp_grid4.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_upp_grid4.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_upp_grid5.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_upp_grid5.Put(iPos, 1, item->GetSize());
				continue;
			}

			return false;
		}
	}

	return true;
}

bool CExchange::CheckSpaceBookInventory()
{
	static CGrid s_book_grid1(5, 9);
	static CGrid s_book_grid2(5, 9);
	static CGrid s_book_grid3(5, 9);
	static CGrid s_book_grid4(5, 9);
	static CGrid s_book_grid5(5, 9);

	s_book_grid1.Clear();
	s_book_grid2.Clear();
	s_book_grid3.Clear();
	s_book_grid4.Clear();
	s_book_grid5.Clear();

	LPCHARACTER victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;
	for (i = 0; i < SPECIAL_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetBookInventoryItem(i)))
			continue;

		s_book_grid1.Put(i, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE; i < SPECIAL_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetBookInventoryItem(i)))
			continue;

		s_book_grid2.Put(i - SPECIAL_INVENTORY_PAGE_SIZE, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetBookInventoryItem(i)))
			continue;

		s_book_grid3.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetBookInventoryItem(i)))
			continue;

		s_book_grid4.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 4; i < SPECIAL_INVENTORY_PAGE_SIZE * 5; ++i)
	{
		if (!(item = victim->GetBookInventoryItem(i)))
			continue;

		s_book_grid5.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 4, 1, item->GetSize());
	}

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsBook())
		{
			int iPos = s_book_grid1.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_book_grid1.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_book_grid2.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_book_grid2.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_book_grid3.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_book_grid3.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_book_grid4.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_book_grid4.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_book_grid5.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_book_grid5.Put(iPos, 1, item->GetSize());
				continue;
			}

			return false;
		}
	}

	return true;
}

bool CExchange::CheckSpaceStoneInventory()
{
	static CGrid s_stone_grid1(5, 9);
	static CGrid s_stone_grid2(5, 9);
	static CGrid s_stone_grid3(5, 9);
	static CGrid s_stone_grid4(5, 9);
	static CGrid s_stone_grid5(5, 9);

	s_stone_grid1.Clear();
	s_stone_grid2.Clear();
	s_stone_grid3.Clear();
	s_stone_grid4.Clear();
	s_stone_grid5.Clear();

	LPCHARACTER victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;
	for (i = 0; i < SPECIAL_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetStoneInventoryItem(i)))
			continue;

		s_stone_grid1.Put(i, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE; i < SPECIAL_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetStoneInventoryItem(i)))
			continue;

		s_stone_grid2.Put(i - SPECIAL_INVENTORY_PAGE_SIZE, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetStoneInventoryItem(i)))
			continue;

		s_stone_grid3.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetStoneInventoryItem(i)))
			continue;

		s_stone_grid4.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 4; i < SPECIAL_INVENTORY_PAGE_SIZE * 5; ++i)
	{
		if (!(item = victim->GetStoneInventoryItem(i)))
			continue;

		s_stone_grid5.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 4, 1, item->GetSize());
	}

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsStone())
		{
			int iPos = s_stone_grid1.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_stone_grid1.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_stone_grid2.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_stone_grid2.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_stone_grid3.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_stone_grid3.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_stone_grid4.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_stone_grid4.Put(iPos, 1, item->GetSize());
				continue;
			}
			iPos = s_stone_grid5.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_stone_grid5.Put(iPos, 1, item->GetSize());
				continue;
			}

			return false;
		}
	}

	return true;
}

bool CExchange::CheckSpaceChestInventory()
{
	static CGrid s_chest_grid1(5, 9);
	static CGrid s_chest_grid2(5, 9);
	static CGrid s_chest_grid3(5, 9);
	static CGrid s_chest_grid4(5, 9);
	static CGrid s_chest_grid5(5, 9);

	s_chest_grid1.Clear();
	s_chest_grid2.Clear();
	s_chest_grid3.Clear();
	s_chest_grid4.Clear();
	s_chest_grid5.Clear();

	LPCHARACTER victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;
	for (i = 0; i < SPECIAL_INVENTORY_PAGE_SIZE; ++i)
	{
		if (!(item = victim->GetChestInventoryItem(i)))
			continue;

		s_chest_grid1.Put(i, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE; i < SPECIAL_INVENTORY_PAGE_SIZE * 2; ++i)
	{
		if (!(item = victim->GetChestInventoryItem(i)))
			continue;

		s_chest_grid2.Put(i - SPECIAL_INVENTORY_PAGE_SIZE, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 2; i < SPECIAL_INVENTORY_PAGE_SIZE * 3; ++i)
	{
		if (!(item = victim->GetChestInventoryItem(i)))
			continue;

		s_chest_grid3.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 2, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 3; i < SPECIAL_INVENTORY_PAGE_SIZE * 4; ++i)
	{
		if (!(item = victim->GetChestInventoryItem(i)))
			continue;

		s_chest_grid4.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 3, 1, item->GetSize());
	}
	for (i = SPECIAL_INVENTORY_PAGE_SIZE * 4; i < SPECIAL_INVENTORY_PAGE_SIZE * 5; ++i)
	{
		if (!(item = victim->GetChestInventoryItem(i)))
			continue;

		s_chest_grid5.Put(i - SPECIAL_INVENTORY_PAGE_SIZE * 4, 1, item->GetSize());
	}

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsChest())
		{
			int iPos = s_chest_grid1.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_chest_grid1.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_chest_grid2.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_chest_grid2.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_chest_grid3.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_chest_grid3.Put(iPos, 1, item->GetSize());
				continue;
			}

			iPos = s_chest_grid4.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_chest_grid4.Put(iPos, 1, item->GetSize());
				continue;
			}
			iPos = s_chest_grid5.FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				s_chest_grid5.Put(iPos, 1, item->GetSize());
				continue;
			}

			return false;
		}
	}

	return true;
}
#endif

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
bool CExchange::Done(std::vector<InGameLog::TTradeLogItemInfo>& logitem)
#else
bool CExchange::Done()
#endif
{
	int		empty_pos, i;
	LPITEM	item;

	LPCHARACTER	victim = GetCompany()->GetOwner();
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
	InGameLog::TTradeLogItemInfo loginfo;
#endif
	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;
		BYTE window_type = victim->VnumGetWindowType(item->GetVnum());
		empty_pos = victim->WindowTypeToGetEmpty(window_type,item);

		if (empty_pos < 0)
		{
			sys_err("Exchange::Done : Cannot find blank position in inventory %s <-> %s item %s",
				m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);

		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), UINT16_MAX);

		item->RemoveFromCharacter();
		item->AddToCharacter(victim, TItemPos(window_type, empty_pos));

		ITEM_MANAGER::instance().FlushDelayedSave(item);
		item->SetExchanging(false);

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
		loginfo.ownerID = m_pOwner->GetPlayerID();
		memcpy(loginfo.item.aAttr, item->GetAttributes(), sizeof(loginfo.item.aAttr));
		loginfo.item.dwVnum = item->GetVnum();
		loginfo.item.dwCount = item->GetCount();
		memcpy(loginfo.item.alSockets, item->GetSockets(), sizeof(loginfo.item.alSockets));
		loginfo.pos = m_abItemDisplayPos[i];
		logitem.push_back(loginfo);
#endif
		m_apItems[i] = NULL;
	}

	if (m_lGold)
	{
		GetOwner()->PointChange(POINT_GOLD, -m_lGold, true);
		victim->PointChange(POINT_GOLD, m_lGold, true);
	}

	m_pGrid->Clear();
	return true;
}

bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int	iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		//PREVENT_PORTAL_AFTER_EXCHANGE
		GetOwner()->SetActivateTime(EXCHANGE_CHECK_TIME);
		victim->SetActivateTime(EXCHANGE_CHECK_TIME);
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
		std::vector<InGameLog::TTradeLogItemInfo> v_itemLog;
		v_itemLog.clear();
		InGameLog::TSubPackTradeAddGD logpacket;
#endif

		// @fixme150 BEGIN
		if (quest::CQuestManager::instance().GetPCForce(GetOwner()->GetPlayerID())->IsRunning() == true)
		{
			GetOwner()->NewChatPacket(STRING_D89);
			victim->NewChatPacket(PLAYER_BUSY_RN);
			goto EXCHANGE_END;
		}
		else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
		{
			victim->NewChatPacket(STRING_D89);
			GetOwner()->NewChatPacket(PLAYER_BUSY_RN);
			goto EXCHANGE_END;
		}
		// @fixme150 END

		if (!Check(&iItemCount))
		{
			GetOwner()->NewChatPacket(STRING_D90);
			victim->NewChatPacket(STRING_D90);
			goto EXCHANGE_END;
		}

		if (!CheckSpace())
		{
			GetOwner()->NewChatPacket(STRING_D90);
			victim->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

#ifdef ENABLE_SPECIAL_STORAGE
		if (!CheckSpaceUpgradeInventory())
		{
			GetOwner()->NewChatPacket(STRING_D90);
			victim->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!CheckSpaceBookInventory())
		{
			GetOwner()->NewChatPacket(STRING_D90);
			victim->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!CheckSpaceStoneInventory())
		{
			GetOwner()->NewChatPacket(STRING_D90);
			victim->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!CheckSpaceChestInventory())
		{
			GetOwner()->NewChatPacket(STRING_D90);
			victim->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}
#endif

		if (!GetCompany()->Check(&iItemCount))
		{
			victim->NewChatPacket(STRING_D90);
			GetOwner()->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->NewChatPacket(STRING_D90);
			GetOwner()->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

#ifdef ENABLE_SPECIAL_STORAGE
		if (!GetCompany()->CheckSpaceUpgradeInventory())
		{
			victim->NewChatPacket(STRING_D90);
			GetOwner()->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpaceBookInventory())
		{
			victim->NewChatPacket(STRING_D90);
			GetOwner()->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpaceStoneInventory())
		{
			victim->NewChatPacket(STRING_D90);
			GetOwner()->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpaceChestInventory())
		{
			victim->NewChatPacket(STRING_D90);
			GetOwner()->NewChatPacket(STRING_D42);
			goto EXCHANGE_END;
		}
#endif

		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
		if (Done(v_itemLog))
#else
		if (Done())
#endif
		{
			if (m_lGold)
				GetOwner()->Save();
#ifdef ENABLE_IN_GAME_LOG_SYSTEM
			if (GetCompany()->Done(v_itemLog))
#else
			if (GetCompany()->Done())
#endif
			{
				if (GetCompany()->m_lGold)
					victim->Save();

#ifdef ENABLE_IN_GAME_LOG_SYSTEM
				if (v_itemLog.size() > 0 || m_lGold > 0 || GetCompany()->m_lGold > 0)
				{
					logpacket.itemCount = v_itemLog.size();
					logpacket.owner.pid = m_pOwner->GetPlayerID();
					snprintf(logpacket.owner.name, sizeof(logpacket.owner.name), m_pOwner->GetName());
					logpacket.owner.gold = m_lGold;

					logpacket.victim.pid = GetCompany()->m_pOwner->GetPlayerID();
					snprintf(logpacket.victim.name, sizeof(logpacket.victim.name), GetCompany()->m_pOwner->GetName());
					logpacket.victim.gold = GetCompany()->m_lGold;
					InGameLog::InGameLogManager& rIglMgr = InGameLog::GetManager();
					rIglMgr.SendTradeLogAddGD(v_itemLog, logpacket);
				}
#endif
				// INTERNATIONAL_VERSION
				GetOwner()->NewChatPacket(STRING_D91 ,"%s", victim->GetName());
				victim->NewChatPacket(STRING_D91 ,"%s", GetOwner()->GetName());
				// END_OF_INTERNATIONAL_VERSION
			}
		}

	EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, true, m_bAccept, NPOS, 0);
		exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, false, m_bAccept, NPOS, 0);
		return true;
	}
}

void CExchange::Cancel()
{
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_END, 0, 0, NPOS, 0);
	GetOwner()->SetExchange(NULL);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE(this);
}