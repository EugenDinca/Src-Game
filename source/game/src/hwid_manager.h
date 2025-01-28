#pragma once
#include "stdafx.h"
#ifdef ENABLE_HWID
class CHwidManager : public singleton<CHwidManager>
{
public:
	CHwidManager();
	~CHwidManager();
	void SetHwidBanGD(const char* c_hwid);
	void RecvPacketDG(const char* data);
#ifdef ENABLE_FARM_BLOCK
	void GetFarmBlock(LPCHARACTER ch);
	void SetFarmBlockGD(LPCHARACTER ch);
	void FarmBlockLogout(DWORD hwidID, DWORD pid, bool isWarp);
	void RecvFarmBlockGetDG(const char* data);
#endif
};
#endif