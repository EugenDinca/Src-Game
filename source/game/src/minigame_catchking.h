#pragma once

#include "../../common/length.h"
#include "packet.h"

class CatchKing: public singleton<CatchKing>
{
#ifdef ENABLE_MINI_GAME_CATCH_KING
public:
	int	 MiniGameCatchKing(LPCHARACTER ch, const char* data, size_t uiBytes);
	void MiniGameCatchKingStartGame(LPCHARACTER pkChar, uint8_t bSetCount);
	void MiniGameCatchKingDeckCardClick(LPCHARACTER pkChar);
	void MiniGameCatchKingFieldCardClick(LPCHARACTER pkChar, uint8_t bFieldPos);
	void MiniGameCatchKingGetReward(LPCHARACTER pkChar);
protected:
	int iCatchKingEndTime;
#endif // ENABLE_MINI_GAME_CATCH_KING

};