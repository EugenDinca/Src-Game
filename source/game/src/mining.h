#ifndef __MINING_H
#define __MINING_H

namespace mining
{
	LPEVENT CreateMiningEvent(LPCHARACTER ch, LPCHARACTER load, int count);
	DWORD GetDropItemFromLod(const DWORD miningLodVnum);
	BYTE GetMiningLodNeededPoint(const DWORD miningLodVnum);
	int GetFractionCount(
#ifdef ENABLE_MINING_RENEWAL
		const LPCHARACTER ch
#endif
	);
	void OreDrop(LPCHARACTER ch, DWORD dwLoadVnum);

	// REFINE_PICK
	int RealRefinePick(LPCHARACTER ch, LPITEM item);
	void CHEAT_MAX_PICK(LPCHARACTER ch, LPITEM item);
	// END_OF_REFINE_PICK
	bool IsVeinOfOre(DWORD vnum);
}

#endif
