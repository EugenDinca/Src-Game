#include "stdafx.h"

#ifdef ENABLE_MINI_GAME_CATCH_KING
#include "config.h"
#include "minigame_catchking.h"

#include "../../common/length.h"
#include "../../common/tables.h"
#include "p2p.h"
#include "locale_service.h"
#include "char.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "questmanager.h"
#include "questlua.h"
#include "start_position.h"
#include "char_manager.h"
#include "item_manager.h"
#include "sectree_manager.h"
#include "regen.h"
#include "db.h"
#include "target.h"
#include "party.h"

#include <random>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <ctime>

size_t GetMiniGameSubPacketLengthCatchKing(const EPacketCGMiniGameSubHeaderCatchKing& SubHeader)
{
	switch (SubHeader)
	{
		case SUBHEADER_CG_CATCH_KING_START:
			return sizeof(TSubPacketCGMiniGameCatchKingStart);
		case SUBHEADER_CG_CATCH_KING_DECKCARD_CLICK:
			return 0;
		case SUBHEADER_CG_CATCH_KING_FIELDCARD_CLICK:
			return sizeof(TSubPacketCGMiniGameCatchKingFieldCardClick);
		case SUBHEADER_CG_CATCH_KING_REWARD:
			return 0;
	}

	return 0;
}


int CatchKing::MiniGameCatchKing(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGMiniGameCatchKing))
		return -1;

	const TPacketCGMiniGameCatchKing* pinfo = reinterpret_cast<const TPacketCGMiniGameCatchKing*>(data);
	const char* c_pData = data + sizeof(TPacketCGMiniGameCatchKing);

	uiBytes -= sizeof(TPacketCGMiniGameCatchKing);

	const EPacketCGMiniGameSubHeaderCatchKing SubHeader = static_cast<EPacketCGMiniGameSubHeaderCatchKing>(pinfo->bSubheader);
	const size_t SubPacketLength = GetMiniGameSubPacketLengthCatchKing(SubHeader);
	if (uiBytes < SubPacketLength)
	{
		sys_err("invalid catchking subpacket length (sublen %d size %u buffer %u)", SubPacketLength, sizeof(TPacketCGMiniGameCatchKing), uiBytes);
		return -1;
	}

	switch (SubHeader)
	{
	case SUBHEADER_CG_CATCH_KING_START:
	{
		const TSubPacketCGMiniGameCatchKingStart* sp = reinterpret_cast<const TSubPacketCGMiniGameCatchKingStart*>(c_pData);
		MiniGameCatchKingStartGame(ch, sp->betNumber);
	}
	return SubPacketLength;

	case SUBHEADER_CG_CATCH_KING_DECKCARD_CLICK:
	{
		MiniGameCatchKingDeckCardClick(ch);
	}
	return SubPacketLength;

	case SUBHEADER_CG_CATCH_KING_FIELDCARD_CLICK:
	{
		const TSubPacketCGMiniGameCatchKingFieldCardClick* sp = reinterpret_cast<const TSubPacketCGMiniGameCatchKingFieldCardClick*>(c_pData);
		MiniGameCatchKingFieldCardClick(ch, sp->cardNumber);
	}
	return SubPacketLength;

	case SUBHEADER_CG_CATCH_KING_REWARD:
	{
		MiniGameCatchKingGetReward(ch);
	}
	return SubPacketLength;
	}

	return 0;
}

void CatchKing::MiniGameCatchKingStartGame(LPCHARACTER pkChar, uint8_t bSetCount)
{
	if (pkChar == nullptr)
		return;

	if (!pkChar->GetDesc())
		return;

	if (pkChar->MiniGameCatchKingGetGameStatus() == true)
		return;

	const auto event = CHARACTER_MANAGER::Instance().CheckEventIsActive(CATCH_KING_EVENT, pkChar->GetEmpire());

	if (!event)
	{
		pkChar->NewChatPacket(STRING_D193);
		return;
	}

	if (bSetCount < 1 || bSetCount > 5)
	{
		pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The number of bets is invalid, please try again."));
		return;
	}

	if (pkChar->GetGold() < (CATCH_KING_PLAY_YANG * bSetCount))
	{
		pkChar->NewChatPacket(NOT_ENOUGH_YANG);
		return;
	}

	if (pkChar->CountSpecifyItem(CATCH_KING_PLAY_ITEM) < bSetCount)
	{
		pkChar->NewChatPacket(NOT_ENOUGH_MATERIAL);
		return;
	}

	pkChar->RemoveSpecifyItem(CATCH_KING_PLAY_ITEM, bSetCount);
	pkChar->PointChange(POINT_GOLD, -(CATCH_KING_PLAY_YANG * bSetCount));

	std::vector<TCatchKingCard> m_vecFieldCards;

	std::srand(unsigned(std::time(0)));

	for (int i = 0; i < 25; i++)
	{
		if (i >= 0 && i < 7)
			m_vecFieldCards.push_back(TCatchKingCard(1, false));
		else if (i >= 7 && i < 11)
			m_vecFieldCards.push_back(TCatchKingCard(2, false));
		else if (i >= 11 && i < 16)
			m_vecFieldCards.push_back(TCatchKingCard(3, false));
		else if (i >= 16 && i < 21)
			m_vecFieldCards.push_back(TCatchKingCard(4, false));
		else if (i >= 21 && i < 24)
			m_vecFieldCards.push_back(TCatchKingCard(5, false));
		else if (i >= 24)
			m_vecFieldCards.push_back(TCatchKingCard(6, false)); // 6 = K
	}

	// std::random_shuffle(m_vecFieldCards.begin(), m_vecFieldCards.end());
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(m_vecFieldCards.begin(), m_vecFieldCards.end(), g);

	pkChar->MiniGameCatchKingSetFieldCards(m_vecFieldCards);

	pkChar->MiniGameCatchKingSetBetNumber(bSetCount);
	pkChar->MiniGameCatchKingSetHandCardLeft(12);
	pkChar->MiniGameCatchKingSetGameStatus(true);

	const uint32_t dwBigScore = 500;

	TEMP_BUFFER buf;
	TPacketGCMiniGameCatchKing packet{};
	TSubPacketGCCatchKingStart sub{};
	packet.wSize = sizeof(TPacketGCMiniGameCatchKing) + sizeof(TSubPacketGCCatchKingStart);
	packet.bSubheader = SUBHEADER_GC_CATCH_KING_START;
	sub.dwBigScore = dwBigScore;

	if (!pkChar->GetDesc())
	{
		sys_err("User(%s)'s DESC is nullptr POINT.", pkChar->GetName());
		return;
	}

	buf.write(&packet, sizeof(TPacketGCMiniGameCatchKing));
	buf.write(&sub, sizeof(TSubPacketGCCatchKingStart));
	pkChar->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CatchKing::MiniGameCatchKingDeckCardClick(LPCHARACTER pkChar)
{
	if (pkChar == nullptr)
		return;

	if (!pkChar->GetDesc())
		return;

	const auto event = CHARACTER_MANAGER::Instance().CheckEventIsActive(CATCH_KING_EVENT, pkChar->GetEmpire());

	if (!event)
	{
		pkChar->NewChatPacket(STRING_D193);
		return;
	}

	if (pkChar->MiniGameCatchKingGetGameStatus() == false)
		return;

	if (pkChar->MiniGameCatchKingGetHandCard())
		return;

	const uint8_t bCardLeft = pkChar->MiniGameCatchKingGetHandCardLeft();

	if (bCardLeft)
	{
		if (bCardLeft <= 12 && bCardLeft > 7)
			pkChar->MiniGameCatchKingSetHandCard(1);
		else if (bCardLeft <= 7 && bCardLeft > 5)
			pkChar->MiniGameCatchKingSetHandCard(2);
		else if (bCardLeft <= 5 && bCardLeft > 3)
			pkChar->MiniGameCatchKingSetHandCard(3);
		else if (bCardLeft == 3)
			pkChar->MiniGameCatchKingSetHandCard(4);
		else if (bCardLeft == 2)
			pkChar->MiniGameCatchKingSetHandCard(5);
		else if (bCardLeft == 1)
			pkChar->MiniGameCatchKingSetHandCard(6);
	}
	else
		return;

	const uint8_t bCardInHand = pkChar->MiniGameCatchKingGetHandCard();

	if (!bCardInHand)
		return;

	pkChar->MiniGameCatchKingSetHandCardLeft(bCardLeft - 1);

	TEMP_BUFFER buf;
	TPacketGCMiniGameCatchKing packet{};
	TSubPacketGCCatchKingSetCard sub{};
	packet.wSize = sizeof(TPacketGCMiniGameCatchKing) + sizeof(TSubPacketGCCatchKingSetCard);
	packet.bSubheader = SUBHEADER_GC_CATCH_KING_SET_CARD;
	sub.bCardInHand = bCardInHand;

	if (!pkChar->GetDesc())
	{
		sys_err("User(%s)'s DESC is nullptr POINT.", pkChar->GetName());
		return;
	}

	buf.write(&packet, sizeof(TPacketGCMiniGameCatchKing));
	buf.write(&sub, sizeof(TSubPacketGCCatchKingSetCard));
	pkChar->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CatchKing::MiniGameCatchKingFieldCardClick(LPCHARACTER pkChar, uint8_t bFieldPos)
{
	if (pkChar == nullptr)
		return;

	if (!pkChar->GetDesc())
		return;

	const auto event = CHARACTER_MANAGER::Instance().CheckEventIsActive(CATCH_KING_EVENT, pkChar->GetEmpire());

	if (!event)
	{
		pkChar->NewChatPacket(STRING_D193);
		return;
	}

	if (pkChar->MiniGameCatchKingGetGameStatus() == false)
		return;

	if (bFieldPos < 0 || bFieldPos > 24)
		return;

	const uint8_t bHandCard = pkChar->MiniGameCatchKingGetHandCard();
	const TCatchKingCard filedCard = pkChar->m_vecCatchKingFieldCards[bFieldPos];

	if (!bHandCard)
		return;

	if (filedCard.bIsExposed == true)
		return;

	uint32_t dwPoints = 0;
	bool bDestroyCard = false;
	bool bKeepFieldCard = false;
	bool bGetReward = false;

	if (bHandCard < 5)
	{
		if (bHandCard < filedCard.bIndex)
		{
			dwPoints = 0;
			bDestroyCard = true;
			bKeepFieldCard = false;
		}
		else if (bHandCard == filedCard.bIndex)
		{
			dwPoints = filedCard.bIndex * 10;
			bDestroyCard = true;
			bKeepFieldCard = true;
		}
		else
		{
			dwPoints = filedCard.bIndex * 10;
			bDestroyCard = false;
			bKeepFieldCard = true;
		}
	}

	int checkPos[8];
	checkPos[0] = bFieldPos - 5;
	checkPos[1] = bFieldPos + 5;
	checkPos[2] = (bFieldPos % 10 == 4 || bFieldPos % 10 == 9) ? -1 : (bFieldPos + 1);
	checkPos[3] = (bFieldPos % 10 == 0 || bFieldPos % 10 == 5) ? -1 : (bFieldPos - 1);
	checkPos[4] = (bFieldPos % 10 == 4 || bFieldPos % 10 == 9) ? -1 : (bFieldPos - 5 + 1);
	checkPos[5] = (bFieldPos % 10 == 4 || bFieldPos % 10 == 9) ? -1 : (bFieldPos + 5 + 1);
	checkPos[6] = (bFieldPos % 10 == 0 || bFieldPos % 10 == 5) ? -1 : (bFieldPos - 5 - 1);
	checkPos[7] = (bFieldPos % 10 == 0 || bFieldPos % 10 == 5) ? -1 : (bFieldPos + 5 - 1);

	bool isFiveNearby = false;

	for (int i = 0; i < 25; i++)
	{
		if (isFiveNearby)
			break;

		for (size_t j = 0; j < (sizeof(checkPos) / sizeof(checkPos[0])); j++)
		{
			if (checkPos[j] < 0 || checkPos[j] >= 25)
				continue;

			if (i == checkPos[j] && pkChar->m_vecCatchKingFieldCards[i].bIndex == 5)
			{
				isFiveNearby = true;
				break;
			}
		}
	}

	if (bHandCard == 5)
	{
		if (isFiveNearby)
		{
			dwPoints = 0;
			bDestroyCard = true;
			bKeepFieldCard = (bHandCard >= filedCard.bIndex) ? true : false;
		}
		else
		{
			dwPoints = (bHandCard >= filedCard.bIndex) ? filedCard.bIndex * 10 : 0;
			bDestroyCard = (bHandCard > filedCard.bIndex) ? false : true;
			bKeepFieldCard = (bHandCard >= filedCard.bIndex) ? true : false;
		}
	}

	if (bHandCard == 6)
	{
		dwPoints = (bHandCard == filedCard.bIndex) ? 100 : 0;
		bDestroyCard = true;
		bKeepFieldCard = (bHandCard == filedCard.bIndex) ? true : false;
	}

	if (bKeepFieldCard)
		pkChar->m_vecCatchKingFieldCards[bFieldPos].bIsExposed = true;

	int checkRowPos[4];
	checkRowPos[0] = 5 * (bFieldPos / 5);
	checkRowPos[1] = 4 + (5 * (bFieldPos / 5));
	checkRowPos[2] = bFieldPos - (5 * (bFieldPos / 5));
	checkRowPos[3] = bFieldPos + 20 - (5 * (bFieldPos / 5));

	bool isHorizontalRow = true;
	bool isVerticalRow = true;

	for (int row = checkRowPos[0]; row <= checkRowPos[1]; row += 1)
	{
		if (!pkChar->m_vecCatchKingFieldCards[row].bIsExposed)
		{
			isHorizontalRow = false;
			break;
		}
	}

	for (int col = checkRowPos[2]; col <= checkRowPos[3]; col += 5)
	{
		if (!pkChar->m_vecCatchKingFieldCards[col].bIsExposed)
		{
			isVerticalRow = false;
			break;
		}
	}

	dwPoints += isHorizontalRow ? 10 : 0;
	dwPoints += isVerticalRow ? 10 : 0;

	if (dwPoints)
		pkChar->MiniGameCatchKingSetScore(pkChar->MiniGameCatchKingGetScore() + dwPoints);

	bool isTheEnd = false;

	if (bDestroyCard)
	{
		bGetReward = (bHandCard == 6 && pkChar->MiniGameCatchKingGetScore() >= 10) ? true : false;
		isTheEnd = (bHandCard == 6) ? true : false;
		pkChar->MiniGameCatchKingSetHandCard(0);
	}

	uint8_t bRowType = 0;
	if (isHorizontalRow && !isVerticalRow)
		bRowType = 1;
	else if (!isHorizontalRow && isVerticalRow)
		bRowType = 2;
	else if (isHorizontalRow && isVerticalRow)
		bRowType = 3;

	TEMP_BUFFER buf;
	TPacketGCMiniGameCatchKing packet{};
	TSubPacketGCCatchKingResult packetSecond{};
	packet.wSize = sizeof(TPacketGCMiniGameCatchKing) + sizeof(TSubPacketGCCatchKingResult);
	packet.bSubheader = SUBHEADER_GC_CATCH_KING_RESULT_FIELD;

	packetSecond.dwPoints = pkChar->MiniGameCatchKingGetScore();
	packetSecond.bRowType = bRowType;
	packetSecond.bCardPos = bFieldPos;
	packetSecond.bCardValue = filedCard.bIndex;
	packetSecond.bKeepFieldCard = bKeepFieldCard;
	packetSecond.bDestroyHandCard = bDestroyCard;
	packetSecond.bGetReward = bGetReward;
	packetSecond.bIsFiveNearBy = isFiveNearby;

	if (!pkChar->GetDesc())
	{
		sys_err("User(%s)'s DESC is nullptr POINT.", pkChar->GetName());
		return;
	}

	buf.write(&packet, sizeof(TPacketGCMiniGameCatchKing));
	buf.write(&packetSecond, sizeof(TSubPacketGCCatchKingResult));
	pkChar->GetDesc()->Packet(buf.read_peek(), buf.size());

	if (isTheEnd)
	{
		for (uint8_t i = 0; i < 25; i++)
		{
			if (!pkChar->m_vecCatchKingFieldCards[i].bIsExposed)
			{
				TEMP_BUFFER buf;
				TPacketGCMiniGameCatchKing packet2{};
				TSubPacketGCCatchKingSetEndCard packetSecond2{};
				packet2.bSubheader = SUBHEADER_GC_CATCH_KING_SET_END_CARD;
				packet2.wSize = sizeof(TPacketGCMiniGameCatchKing) + sizeof(TSubPacketGCCatchKingSetEndCard);

				packetSecond2.bCardPos = i;
				packetSecond2.bCardValue = pkChar->m_vecCatchKingFieldCards[i].bIndex;

				if (!pkChar->GetDesc())
				{
					sys_err("User(%s)'s DESC is nullptr POINT.", pkChar->GetName());
					return;
				}

				buf.write(&packet2, sizeof(TPacketGCMiniGameCatchKing));
				buf.write(&packetSecond2, sizeof(TSubPacketGCCatchKingSetEndCard));
				pkChar->GetDesc()->Packet(buf.read_peek(), buf.size());
			}
		}
	}
}

void CatchKing::MiniGameCatchKingGetReward(LPCHARACTER pkChar)
{
	if (pkChar == nullptr)
		return;

	if (!pkChar->GetDesc())
		return;

	const auto event = CHARACTER_MANAGER::Instance().CheckEventIsActive(CATCH_KING_EVENT, pkChar->GetEmpire());

	if (!event)
	{
		pkChar->NewChatPacket(STRING_D193);
		return;
	}

	if (pkChar->MiniGameCatchKingGetGameStatus() == false)
		return;

	if (pkChar->MiniGameCatchKingGetHandCard())
		return;

	if (pkChar->MiniGameCatchKingGetHandCardLeft())
		return;

	uint32_t dwRewardVnum = 0;
	uint8_t bReturnCode = 0;
	const uint32_t dwScore = pkChar->MiniGameCatchKingGetScore();

	if (dwScore >= 10 && dwScore < 400)
		dwRewardVnum = CATCH_KING_REWARD_S;
	else if (dwScore >= 400 && dwScore < 550)
		dwRewardVnum = CATCH_KING_REWARD_M;
	else if (dwScore >= 550)
		dwRewardVnum = CATCH_KING_REWARD_L;

	if (dwRewardVnum)
	{
		pkChar->MiniGameCatchKingSetScore(0);
		pkChar->AutoGiveItem(dwRewardVnum, static_cast<uint8_t>(pkChar->MiniGameCatchKingGetBetNumber()));
		pkChar->MiniGameCatchKingSetBetNumber(0);
		pkChar->MiniGameCatchKingSetGameStatus(false);
		pkChar->m_vecCatchKingFieldCards.clear();
#ifdef ENABLE_BATTLE_PASS_SYSTEM
		pkChar->UpdateExtBattlePassMissionProgress(COMPLETE_MINIGAME, 1, 2);
#endif

		bReturnCode = 0;
	}
	else
	{
		bReturnCode = 1;
	}

	TEMP_BUFFER buf;
	TPacketGCMiniGameCatchKing packet{};
	TSubPacketGCCatchKingReward sub{};
	packet.wSize = sizeof(TPacketGCMiniGameCatchKing) + sizeof(TSubPacketGCCatchKingReward);
	packet.bHeader = HEADER_GC_MINI_GAME_CATCH_KING;
	packet.bSubheader = SUBHEADER_GC_CATCH_KING_REWARD;
	sub.bReturnCode = bReturnCode;

	if (!pkChar->GetDesc())
	{
		sys_err("User(%s)'s DESC is nullptr POINT.", pkChar->GetName());
		return;
	}

	buf.write(&packet, sizeof(TPacketGCMiniGameCatchKing));
	buf.write(&sub, sizeof(TSubPacketGCCatchKingReward));
	pkChar->GetDesc()->Packet(buf.read_peek(), buf.size());

}
#endif
