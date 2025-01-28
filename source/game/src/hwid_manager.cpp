#include "stdafx.h"
#ifdef ENABLE_HWID
#include "hwid_manager.h"
#include "db.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
#include "InGameLogManager.h"
CHwidManager::CHwidManager()
{
}

CHwidManager::~CHwidManager()
{
}

void CHwidManager::SetHwidBanGD(const char* c_hwid)
{
	THwidGDPacket pack;
	pack.subHeader = SUB_HEADER_GD_HWID_BAN;
	THwidString subPack;
	snprintf(subPack.hwid, sizeof(subPack.hwid), c_hwid);

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(THwidGDPacket));
	buff.write(&subPack, sizeof(THwidString));
	db_clientdesc->DBPacket(HEADER_GD_HWID, 0, buff.read_peek(), buff.size());
}

void CHwidManager::RecvPacketDG(const char* data)
{
	THwidDGPacket* pack;
	data = InGameLog::Decode(pack, data);
	switch (pack->subHeader)
	{
#ifdef ENABLE_FARM_BLOCK
		case SUB_HEADER_DG_GET_FARM_BLOCK: { RecvFarmBlockGetDG(data); break; }
#endif
		default: { sys_err("Unknow recovery packet DG %d", pack->subHeader); break; }
	}
}

#ifdef ENABLE_FARM_BLOCK
void CHwidManager::GetFarmBlock(LPCHARACTER ch)
{
	if (ch->GetDesc())
	{
		THwidGDPacket pack;
		pack.subHeader = SUB_HEADER_GD_GET_FARM_BLOCK;

		TFarmBlockGet subPack;
		subPack.descHwidID = ch->GetDesc()->GetAccountTable().descHwidID;
		subPack.pid = ch->GetPlayerID();

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(THwidGDPacket));
		buff.write(&subPack, sizeof(TFarmBlockGet));
		db_clientdesc->DBPacket(HEADER_GD_HWID, 0, buff.read_peek(), buff.size());
	}
}

void CHwidManager::RecvFarmBlockGetDG(const char* data)
{
	TFarmBlockGD* pack;
	data = InGameLog::Decode(pack, data);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pack->pid);
	if (ch)
	{
		ch->SetFarmBlock(pack->farmBlock, pack->result);
	}

}

void CHwidManager::SetFarmBlockGD(LPCHARACTER ch)
{
	if (ch->GetDesc())
	{
		THwidGDPacket pack;
		pack.subHeader = SUB_HEADER_GD_SET_FARM_BLOCK;

		TChangeFarmBlock subPack;
		subPack.info.descHwidID = ch->GetDesc()->GetAccountTable().descHwidID;
		subPack.info.pid = ch->GetPlayerID();
		ch->IsFarmBlock() ? subPack.farmBlock = false : subPack.farmBlock = true;

		TEMP_BUFFER buff;
		buff.write(&pack, sizeof(THwidGDPacket));
		buff.write(&subPack, sizeof(TChangeFarmBlock));
		db_clientdesc->DBPacket(HEADER_GD_HWID, 0, buff.read_peek(), buff.size());
	}
}

void CHwidManager::FarmBlockLogout(DWORD hwidID, DWORD pid, bool isWarp)
{
	if (isWarp)
		return;

	THwidGDPacket pack;
	pack.subHeader = SUB_HEADER_GD_FARM_BLOCK_LOGOUT;

	TFarmBlockGet subPack;
	subPack.descHwidID = hwidID;
	subPack.pid = pid;

	TEMP_BUFFER buff;
	buff.write(&pack, sizeof(THwidGDPacket));
	buff.write(&subPack, sizeof(TFarmBlockGet));
	db_clientdesc->DBPacket(HEADER_GD_HWID, 0, buff.read_peek(), buff.size());
}
#endif


#endif