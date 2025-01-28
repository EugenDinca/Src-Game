#include "stdafx.h"
#include "mining.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "item.h"
#include "config.h"
#include "db.h"

#include "skill.h"

#define ENABLE_PICKAXE_RENEWAL
namespace mining
{
	enum
	{
		MAX_ORE = 19,
		MAX_FRACTION_COUNT = 9,
		ORE_COUNT_FOR_REFINE = 100,
	};

	typedef struct SInfo
	{
		DWORD dropOreVnum;
		BYTE neededPoint;
	}TInfo;

	const std::unordered_map<DWORD, TInfo> miningLodMap = {
		{ 20047, {200213, 100}, },
		{ 20048, {200213, 100}, },
		{ 20049, {200213, 100}, },
		{ 20050, {200213, 100}, },
		{ 20051, {200213, 100}, },
		{ 20052, {200213, 100}, },
		{ 20053, {200213, 100}, },
		{ 20054, {200213, 100}, },
		{ 20055, {200213, 100}, },
		{ 20056, {200213, 100}, },
		{ 20057, {200213, 100}, },
		{ 20058, {200213, 100}, },
		{ 20059, {200213, 100}, },
		{ 30301, {200213, 100}, },
		{ 30302, {200214, 150} },
		{ 30303, {200214, 150} },
		{ 30304, {200214, 150} },
		{ 30305, {200214, 150} },
		{ 30306, {200214, 150} }
	};

	int fraction_info[MAX_FRACTION_COUNT][3] =
	{
		{ 20,  1, 10 },
		{ 30, 11, 20 },
		{ 20, 21, 30 },
		{ 15, 31, 40 },
		{  5, 41, 50 },
		{  4, 51, 60 },
		{  3, 61, 70 },
		{  2, 71, 80 },
		{  1, 81, 90 },
	};

	int PickGradeAddPct[10] =
	{
		3, 5, 8, 11, 15, 20, 26, 32, 40, 50
	};

	int SkillLevelAddPct[SKILL_MAX_LEVEL + 1] =
	{
		0,
		1, 1, 1, 1,		//  1 - 4
		2, 2, 2, 2,		//  5 - 8
		3, 3, 3, 3,		//  9 - 12
		4, 4, 4, 4,		// 13 - 16
		5, 5, 5, 5,		// 17 - 20
		6, 6, 6, 6,		// 21 - 24
		7, 7, 7, 7,		// 25 - 28
		8, 8, 8, 8,		// 29 - 32
		9, 9, 9, 9,		// 33 - 36
		10, 10, 10, 	// 37 - 39
		11,				// 40
	};

	DWORD GetDropItemFromLod(const DWORD miningLodVnum)
	{
		const auto it{ miningLodMap.find(miningLodVnum) };

		if (it != miningLodMap.end())
			return it->second.dropOreVnum;

		return 0;
	}

	BYTE GetMiningLodNeededPoint(const DWORD miningLodVnum)
	{
		const auto it{ miningLodMap.find(miningLodVnum) };

		if (it != miningLodMap.end())
			return it->second.neededPoint;

		return 0;
	}

	int GetFractionCount(
#ifdef ENABLE_MINING_RENEWAL
		const LPCHARACTER ch
#endif
	)
	{
#ifdef ENABLE_MINING_RENEWAL

		if (!ch)
			return 0;

		const LPITEM wearPickaxe{ ch->GetWear(WEAR_WEAPON) };

		if (!wearPickaxe)
			return 0;

		if (!(wearPickaxe && wearPickaxe->GetType() == ITEM_PICK))
			return 0;

		const DWORD pickAxeVnum{ wearPickaxe->GetVnum() };
		BYTE  minCount = 10; // Default Drop
		BYTE  maxCount = 25; // Default Drop
		BYTE doubleDropChance = 1;

		const int iSkillLevel{ ch->GetSkillLevel(SKILL_MINING) }; // Mining Skill Level

		switch (pickAxeVnum)
		{
		case 29102:
		{
			minCount = 25;
			maxCount = 40;
			doubleDropChance = 5;
			break;
		}

		case 29103:
		{
			minCount = 40;
			maxCount = 55;
			doubleDropChance = 10;
			break;
		}

		case 29104:
		{
			minCount = 55;
			maxCount = 75;
			doubleDropChance = 20;
			break;
		}
		default: break;
		}

		int dropCount = number(minCount, maxCount);
		doubleDropChance += IncreasePointPerSkillLevel[iSkillLevel] * 2;

		if (number(1, 100) <= doubleDropChance)
			dropCount *= 2;

		return dropCount;

#else
		int r = number(1, 100);

		for (int i = 0; i < MAX_FRACTION_COUNT; ++i)
		{
			if (r <= fraction_info[i][0])
				return number(fraction_info[i][1], fraction_info[i][2]);
			else
				r -= fraction_info[i][0];
		}
		return 0;
#endif
	}

	void OreDrop(LPCHARACTER ch, DWORD dwLoadVnum)
	{
		const DWORD dwRawOreVnum = GetDropItemFromLod(dwLoadVnum);

		const int iFractionCount = GetFractionCount(
#ifdef ENABLE_MINING_RENEWAL
			ch
#endif
		);

		if (iFractionCount == 0)
		{
			sys_err("Wrong ore fraction count");
			return;
		}

		const LPITEM item{ ITEM_MANAGER::instance().CreateItem(dwRawOreVnum, iFractionCount) };

#ifdef ENABLE_MINING_RENEWAL
		const LPITEM upgradeItem{ ITEM_MANAGER::instance().CreateItem(MINING_UPGRADE_ITEM_VNUM, iFractionCount) };

		if (!upgradeItem)
		{
			sys_err("cannot create item vnum %d", MINING_UPGRADE_ITEM_VNUM);
			return;
		}

#endif

		if (!item)
		{
			sys_err("cannot create item vnum %d", dwRawOreVnum);
			return;
		}

		PIXEL_POSITION pos;
		pos.x = ch->GetX() + number(-200, 200);
		pos.y = ch->GetY() + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();
		item->SetOwnership(ch, 15);

#ifdef ENABLE_MINING_RENEWAL
		upgradeItem->AddToGround(ch->GetMapIndex(), pos);
		upgradeItem->StartDestroyEvent();
		upgradeItem->SetOwnership(ch, 15);
#endif
		if (number(0, 100) <= MINING_UPGRADE_BOOK_DROP_PCT)
		{
			const LPITEM upgradeBook{ ITEM_MANAGER::instance().CreateItem(MINING_UPGRADE_BOOK_VNUM, 1) };

			if (!upgradeBook)
			{
				sys_err("cannot create item vnum %d", MINING_UPGRADE_BOOK_VNUM);
				return;
			}

			upgradeBook->AddToGround(ch->GetMapIndex(), pos);
			upgradeBook->StartDestroyEvent();
			upgradeBook->SetOwnership(ch, 15);
		}
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		ch->PointChange(POINT_MINING, iFractionCount);
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
		ch->UpdateExtBattlePassMissionProgress(BP_MINING, 1, item->GetVnum());
#endif
	}

	int GetOrePct(LPCHARACTER ch)
	{
		int defaultPct = 20;
		int iSkillLevel = ch->GetSkillLevel(SKILL_MINING);

		LPITEM pick = ch->GetWear(WEAR_WEAPON);

		if (!pick || pick->GetType() != ITEM_PICK)
			return 0;

		return defaultPct + SkillLevelAddPct[MINMAX(0, iSkillLevel, 40)] + PickGradeAddPct[MINMAX(0, pick->GetRefineLevel(), 9)];
	}

	EVENTINFO(mining_event_info)
	{
		DWORD pid;
		DWORD vid_load;

		mining_event_info()
			: pid(0)
			, vid_load(0)
		{
		}
	};

	// REFINE_PICK
	bool Pick_Check(CItem& item)
	{
		if (item.GetType() != ITEM_PICK)
			return false;

		return true;
	}

	int Pick_GetMaxExp(CItem& pick)
	{
		return pick.GetValue(2);
	}

	int Pick_GetCurExp(CItem& pick)
	{
		return pick.GetSocket(0);
	}

	void Pick_IncCurExp(CItem& pick)
	{
		int cur = Pick_GetCurExp(pick);
		pick.SetSocket(0, cur + 1);
	}

#ifdef ENABLE_PICKAXE_RENEWAL
	void Pick_SetPenaltyExp(CItem& pick)
	{
		int cur = Pick_GetCurExp(pick);
		pick.SetSocket(0, (cur > 0) ? (cur - (cur * 10 / 100)) : 0);
	}
#endif

	void Pick_MaxCurExp(CItem& pick)
	{
		int max = Pick_GetMaxExp(pick);
		pick.SetSocket(0, max);
	}

	bool Pick_Refinable(CItem& item)
	{
		if (Pick_GetCurExp(item) < Pick_GetMaxExp(item))
			return false;

		return true;
	}

	bool Pick_IsPracticeSuccess(CItem& pick)
	{
		return (number(1, pick.GetValue(1)) == 1);
	}

	bool Pick_IsRefineSuccess(CItem& pick)
	{
		return (number(1, 100) <= pick.GetValue(3));
	}

	int RealRefinePick(LPCHARACTER ch, LPITEM item)
	{
		if (!ch || !item)
			return 2;

		ITEM_MANAGER& rkItemMgr = ITEM_MANAGER::instance();

		if (!Pick_Check(*item))
		{
			sys_err("REFINE_PICK_HACK pid(%u) item(%s:%d) type(%d)", ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetType());
			return 2;
		}

		CItem& rkOldPick = *item;

		if (!Pick_Refinable(rkOldPick))
			return 2;

		if (rkOldPick.IsEquipped() == true)
			return 2;

		if (Pick_IsRefineSuccess(rkOldPick))
		{

			LPITEM pkNewPick = ITEM_MANAGER::instance().CreateItem(rkOldPick.GetRefinedVnum(), 1);
			if (pkNewPick)
			{
				CELL_UINT bCell = rkOldPick.GetCell();
				rkItemMgr.RemoveItem(item, "REMOVE (REFINE PICK)");
				pkNewPick->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				return 1;
			}

			return 2;
		}
		else
		{

#ifdef ENABLE_PICKAXE_RENEWAL
			{
				Pick_SetPenaltyExp(*item);
				return 0;
			}
#else
			LPITEM pkNewPick = ITEM_MANAGER::instance().CreateItem(rkOldPick.GetValue(4), 1);

			if (pkNewPick)
			{
				CELL_UINT bCell = rkOldPick.GetCell();
				rkItemMgr.RemoveItem(item, "REMOVE (REFINE PICK)");
				pkNewPick->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				return 0;
			}
#endif
			return 2;
		}
	}

	void CHEAT_MAX_PICK(LPCHARACTER ch, LPITEM item)
	{
		if (!item)
			return;

		if (!Pick_Check(*item))
			return;

		CItem& pick = *item;
		Pick_MaxCurExp(pick);

		ch->NewChatPacket(STRING_D108, "%d", Pick_GetCurExp(pick));
	}

	void PracticePick(LPCHARACTER ch, LPITEM item)
	{
		if (!item)
			return;

		if (!Pick_Check(*item))
			return;

		CItem& pick = *item;
		if (pick.GetRefinedVnum() <= 0)
			return;

		if (Pick_IsPracticeSuccess(pick))
		{
			if (Pick_Refinable(pick))
			{
				ch->NewChatPacket(STRING_D109);
				ch->NewChatPacket(STRING_D110);
			}
			else
			{
				Pick_IncCurExp(pick);

				ch->NewChatPacket(STRING_D111, "%d|%d",
					Pick_GetCurExp(pick), Pick_GetMaxExp(pick));

				if (Pick_Refinable(pick))
				{
					ch->NewChatPacket(STRING_D109);
					ch->NewChatPacket(STRING_D110);
				}
			}
		}
	}
	// END_OF_REFINE_PICK

	EVENTFUNC(mining_event)
	{
		mining_event_info* info = dynamic_cast<mining_event_info*>(event->info);

		if (info == nullptr)
		{
			sys_err("mining_event_info> <Factor> Null pointer");
			return 0;
		}

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->pid);
		LPCHARACTER load = CHARACTER_MANAGER::instance().Find(info->vid_load);

		if (!ch)
			return 0;

		ch->mining_take();

		LPITEM pick = ch->GetWear(WEAR_WEAPON);

		// REFINE_PICK
		if (!pick || !Pick_Check(*pick))
		{
			ch->NewChatPacket(STRING_D112);
			return 0;
		}
		// END_OF_REFINE_PICK

		if (!load)
		{
			ch->NewChatPacket(STRING_D113);
			return 0;
		}

#ifdef ENABLE_MINING_RENEWAL
		ch->EndMiningEvent();
#endif

		PracticePick(ch, pick);

		return 0;
	}

	LPEVENT CreateMiningEvent(LPCHARACTER ch, LPCHARACTER load, int count)
	{
		mining_event_info* info = AllocEventInfo<mining_event_info>();
		info->pid = ch->GetPlayerID();
		info->vid_load = load->GetVID();

		return event_create(mining_event, info, PASSES_PER_SEC(2 * count));
	}

	bool IsVeinOfOre(DWORD vnum)
	{
		const auto it{ miningLodMap.find(vnum) };

		if (it != miningLodMap.end())
			return true;

		return false;
	}
}