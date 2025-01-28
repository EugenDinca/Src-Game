#include "stdafx.h"
#include "utils.h"
#include "minigame_bnw.h"
#include "char.h"
#include <algorithm>
#include <random>
#include "char_manager.h"
#include "desc_client.h"

enum
{
	BNW_REQUIRED_VNUM = 79513,
	BNW_REQUIRED_YANG = 30000,

	BNW_REWARD_S = 83043,
	BNW_REWARD_M = 83042,
	BNW_REWARD_L = 83041,

	BNW_LARGE_POINT = 7,
	BNW_MEDIUM_POINT = 4,
};

void MinigameBnwStart(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->GetGold() < BNW_REQUIRED_YANG)
	{
		ch->NewChatPacket(NOT_ENOUGH_YANG);
		return;
	}

	if (ch->CountSpecifyItem(BNW_REQUIRED_VNUM) <= 0)
	{
		ch->NewChatPacket(NOT_ENOUGH_MATERIAL);
		return;
	}
		
	LPDESC pDesc{ ch->GetDesc() };

	if (!pDesc)
		return;

	const auto event = CHARACTER_MANAGER::Instance().CheckEventIsActive(BLACK_N_WHITE_EVENT, ch->GetEmpire());

	if (!event)
	{
		ch->NewChatPacket(STRING_D193);
		return;
	}

	ch->RemoveSpecifyItem(BNW_REQUIRED_VNUM, 1);
	ch->PointChange(POINT_GOLD, -BNW_REQUIRED_YANG, true);

	ch->SetMinigameBnwStarted(true);
	ch->m_vMinigameBnwMyCards.clear();
	ch->m_vMinigameBnwEnemyCards.clear();
	ch->MinigameBnwSetPlayerPoints(0);
	ch->MinigameBnwSetEnemyPoints(0);

	for (BYTE i = 0; i < 9; i++)
	{
		ch->m_vMinigameBnwEnemyCards.push_back(i);
	}

	auto rd = std::random_device {}; 
	auto rng = std::default_random_engine { rd() };
	std::shuffle(std::begin(ch->m_vMinigameBnwEnemyCards), std::end(ch->m_vMinigameBnwEnemyCards), rng);

	//Use with c++98
	//std::srand(get_dword_time());
	//std::random_shuffle(ch->m_vMinigameBnwEnemyCards.begin(), ch->m_vMinigameBnwEnemyCards.end());

	TPacketGCMinigameBnw p;
	p.header		= HEADER_GC_MINIGAME_BNW;
	p.subheader		= MINIGAME_BNW_SUBHEADER_GC_START;

	pDesc->Packet(&p, sizeof(TPacketGCMinigameBnw));
}

void MinigameBnwSelectedCard(LPCHARACTER ch, BYTE bCard)
{
	if (!ch)
		return;

	if (!ch->IsMinigameBnwStarted())
		return;

	if (std::find(ch->m_vMinigameBnwMyCards.begin(), ch->m_vMinigameBnwMyCards.end(), bCard) != ch->m_vMinigameBnwMyCards.end())
		return;

	LPDESC pDesc{ ch->GetDesc() };

	if (!pDesc)
		return;

	ch->m_vMinigameBnwMyCards.push_back(bCard);

	const BYTE enemyCard = ch->m_vMinigameBnwEnemyCards.back();
	ch->m_vMinigameBnwEnemyCards.pop_back();

	BYTE bState = 0;
	if (bCard < enemyCard)
	{
		ch->MinigameBnwSetEnemyPoints(ch->MinigameBnwGetEnemyPoints() + 1);
		bState = 1;
	}
	else if (bCard > enemyCard)
	{
		ch->MinigameBnwSetPlayerPoints(ch->MinigameBnwGetPlayerPoints() + 1);
		bState = 2;
	}

	TPacketGCMinigameBnw p;
	p.header		= HEADER_GC_MINIGAME_BNW;
	p.subheader		= MINIGAME_BNW_SUBHEADER_GC_DRAW_RESULT;

	TPacketGCMinigameBnwDrawResult p2;
	p2.result = bState;
	p2.playerPoints = ch->MinigameBnwGetPlayerPoints();
	p2.enemyPoints = ch->MinigameBnwGetEnemyPoints();

	pDesc->BufferedPacket(&p, sizeof(TPacketGCMinigameBnw));
	pDesc->Packet(&p2, sizeof(TPacketGCMinigameBnwDrawResult));
}

void CalculateReward(LPCHARACTER ch)
{
	const BYTE playerPoints{ ch->MinigameBnwGetPlayerPoints() };
	const BYTE enemyPoints{ ch->MinigameBnwGetPlayerPoints() };

	BYTE totalPoint{ playerPoints };

	if (playerPoints > enemyPoints) // adding the diff
		totalPoint += playerPoints - enemyPoints;

	DWORD rewardVnum{ BNW_REWARD_S };

	if (totalPoint >= BNW_LARGE_POINT)
		rewardVnum = BNW_REWARD_L;
	else if (totalPoint >= BNW_MEDIUM_POINT)
		rewardVnum = BNW_REWARD_M;

	ch->AutoGiveItem(rewardVnum, 1);
}

void MinigameBnwFinished(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!ch->IsMinigameBnwStarted())
		return;

	if (ch->m_vMinigameBnwEnemyCards.size() != 0)
		return;

	ch->SetMinigameBnwStarted(false);

	ch->m_vMinigameBnwMyCards.clear();

	CalculateReward(ch);
}

