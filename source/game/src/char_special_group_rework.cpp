/************************************************************
* title_name		: Special Group Rework
* author			: SkyOfDance
* date				: 23.04.2022
* update			: 28.04.2022
************************************************************/
#include "stdafx.h"
#ifdef ENABLE_CHEST_OPEN_RENEWAL
#include "char.h"
#include "item.h"
#include "config.h"
#include "item_manager.h"
#include "utils.h"
#include "desc.h"
/************************************************************************************************************************************************
*ChestOpenConst::Sabit droplu sandýk tiplerinde çalýþýr ilk önce karakterde kaç sandýk olduðunu alýr sonra 
*sandýðýn içindeki itemleri ve item sayýlarýný vectore alýr arkasýndan her 500 sandýk açýmý için karakterin
*envanterinde boþluk olup olmadýðýný hesaplar ve karakterin alabilceði item miktarýnda sandýðý açar itemleri tek paket halinde gönderir 
*CheckItemSocket2:: itemin socketinde farklýlýk olup olmadýðýný buluyor
*SetChestOpenUseTime:: sandýk açýmýndan sonra 2 saniye süre koyar
*GiveItemEmpetyCalcute:: envantertde boþluk olup olmadýðýný hesaplar ilk önce stacklenebilcek yerleri kontrol eder
*stacklanan itemleri düþer geri kalan itemlerin kaç slotluk yere ihtiyaç duyduðunu hesaplayýp return eder
*TChestEmpetInventory structýna return edilen deðer yazýlýr ardýndan 
*GiveItemEmpetyCalcute2:: fonksyonu çalýþýr tüm envanter tiplerinde ihtiyaç duyulan boþluðun olup olmadýðýný hesaplar false yada true döner
************************************************************************************************************************************************/


const int DragonSoulPosList[DS_SLOT_MAX][DRAGON_SOUL_GRADE_MAX] =
{
	{0, 32, 64, 96, 128, 160},
	{192, 224, 256, 288, 320, 352},
	{384, 416, 448, 480, 512, 544},
	{576, 608, 640, 672, 704, 736},
	{768, 800, 832, 864, 896, 928},
	{960, 992, 1024, 1056, 1088, 1120}
};


static int DsChestToGrade(DWORD chestVnum)//Simya sandýðý vnumlarý eklenip içinden çýkan simya sýnýfý dönücek
{
	switch (chestVnum)
	{
		case 51511:
		case 51512:
		case 51513:
		case 51514:
		case 51515:
		case 51516:
		{
			return 0;
		}

		case 200020:
		{
			return 4;
		}
	}
	return -1;
}

int CHARACTER::GiveItemEmpetyCalcute(DWORD dwItemVnum, DWORD wCount,BYTE window_type)
{
	TItemTable* p = ITEM_MANAGER::instance().GetTable(dwItemVnum);
	if (!p) { return 0; }
	int stackcount = 0;
	//item stacklene biliyorsa
	if (p->dwFlags & ITEM_FLAG_STACKABLE)
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = WindowTypeGetItem(i, window_type);
			if (!item) { continue; }

			if (item->GetVnum() == dwItemVnum)
			{
				DWORD wCount2 = MIN(g_bItemCountLimit - item->GetCount(), wCount);
				wCount -= wCount2;
				if (wCount == 0) { return 0; }
			}
		}
		if (wCount == g_bItemCountLimit) { stackcount = 1; }
		else { stackcount = (wCount / g_bItemCountLimit) + 1; }	
	}
	else { stackcount = wCount; }

	return stackcount;
}

bool CHARACTER::GiveItemEmpetyCalcute2(TChestEmpetInventory *empety)
{
	if (!empety) { return false; }
	int EmpCount = 0;
	BYTE bSize = 1;

	if (empety->norm > 0)
	{
		EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos(INVENTORY, i), bSize)) 
			{
				++EmpCount;
				if (empety->norm == EmpCount)
					break;
			}
		}
		if (empety->norm > EmpCount) { return false; }
	}

	if (empety->upgrade > 0) 
	{
		EmpCount = 0;
		for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos (UPGRADE_INVENTORY, i), bSize))
			{
				++EmpCount;
				if (empety->upgrade == EmpCount)
					break;
			}
		}
		if (empety->upgrade > EmpCount) { return false; }
	}

	if (empety->chest > 0) 
	{
		EmpCount = 0;
		for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos (CHEST_INVENTORY, i), bSize)) 
			{
				++EmpCount;
				if (empety->chest == EmpCount)
					break;
			}
		}
		if (empety->chest > EmpCount) { return false;}
	}
	if (empety->book > 0)
	{
		EmpCount = 0;
		for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos (BOOK_INVENTORY, i), bSize))
			{
				++EmpCount;
				if (empety->book == EmpCount)
					break;
			}
		}
		if (empety->book > EmpCount) { return false;}
	}

	if (empety->stone > 0)
	{
		EmpCount = 0;
		for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos (STONE_INVENTORY, i), bSize))
			{
				++EmpCount;
				if (empety->stone == EmpCount)
					break;
			}
		}
		if (empety->stone > EmpCount) { return false;}
	}
	return true;
}

void CHARACTER::ChestOpenConst(LPITEM item)
{
	if (!item)
		return;
	//grp sandigin grup bilgisini alir
	const CSpecialItemGroup* grp = ITEM_MANAGER::Instance().GetSpecialItemGroup(item->GetVnum());
	if (!grp)
		return;

	const int n = grp->m_vecProbs.size();
	if (n == 0)
		return;

	const DWORD itemcount = item->GetCount();
	DWORD opencount = 1;
	opencount = itemcount < MAX_CHEST_OPEN_COUNT ? itemcount : MAX_CHEST_OPEN_COUNT;

	DWORD dwVnum = 0;
	DWORD dwCount = 0;
	int multicount = 0;
	DWORD CalcuteCount = 0;
	bool stop_drop = false;
	bool empetycalcute = false;
	BYTE window_type = 0;
	int empetycount = 0;
	bool empetymin = false;
	BYTE empetyminval = 0;
	//Envanterde yeterli yer varmi
	if (opencount > CHEST_OPEN_COUNT_CONTROL)
	{
		multicount = opencount / CHEST_OPEN_COUNT_CONTROL;
		for (int a = 0; a < multicount; a++)
		{
			TChestEmpetInventory p_emp;
			for (int i = 0; i < n; i++)
			{
				dwVnum = grp->GetVnum(i);
				dwCount = grp->GetCount(i) * (CHEST_OPEN_COUNT_CONTROL + (opencount - (multicount * CHEST_OPEN_COUNT_CONTROL)));
				CalcuteCount = dwCount * (a + 1);
				window_type = VnumGetWindowType(dwVnum);
				empetycount = GiveItemEmpetyCalcute(dwVnum, CalcuteCount, window_type);
				if (empetycount > 0)
				{
					switch (window_type)
					{
						case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
						case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
						case INVENTORY: { p_emp.norm += empetycount; break; }
						case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
						case STONE_INVENTORY: { p_emp.stone += empetycount; break; }
						default: break;
					}
					empetycalcute = true;
				}
			}
			if (empetycalcute) 
			{
				if(!GiveItemEmpetyCalcute2(&p_emp)) 
				{
					empetymin = true;
					stop_drop = true;
					empetyminval = 1;
					if (a == 0) { opencount = 0; }
					else { opencount = a * CHEST_OPEN_COUNT_CONTROL; }
					break;
				}
			}
		}
	}
	else
	{
		multicount = opencount / CHEST_OPEN_COUNT_CONTROL2;
		if (multicount < 1)
		{
			empetymin = true;
		}
		else
		{
			for (int a = 0; a < multicount; a++)
			{
				TChestEmpetInventory p_emp;
				for (int i = 0; i < n; i++)
				{
					dwVnum = grp->GetVnum(i);
					dwCount = grp->GetCount(i) * (CHEST_OPEN_COUNT_CONTROL2 + (opencount - (multicount * CHEST_OPEN_COUNT_CONTROL2)));
					CalcuteCount = dwCount * (a + 1);
					window_type = VnumGetWindowType(dwVnum);
					empetycount = GiveItemEmpetyCalcute(dwVnum, CalcuteCount, window_type);
					if (empetycount > 0)
					{
						switch (window_type)
						{
							case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
							case INVENTORY: { p_emp.norm += empetycount; break; }
							case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
							case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
							case STONE_INVENTORY: { p_emp.stone += empetycount; break; }

							default: break;
						}
						empetycalcute = true;
					}
				}
				if (empetycalcute)
				{
					if (!GiveItemEmpetyCalcute2(&p_emp))
					{
						empetymin = true;
						stop_drop = true;
						empetyminval = 2;
						if (a == 0) { opencount = 0; }
						else { opencount = a * CHEST_OPEN_COUNT_CONTROL2; }
						break;
					}
				}
			}
		}
	}

	if (empetymin)
	{
		if (empetyminval == 1)
			opencount += CHEST_OPEN_COUNT_CONTROL2;
		else if (empetyminval == 2)
			opencount += 120;

		TChestEmpetInventory p_emp;
		for (int i = 0; i < n; i++)
		{
			dwVnum = grp->GetVnum(i);
			dwCount = grp->GetCount(i) * opencount;
			window_type = VnumGetWindowType(dwVnum);
			empetycount = GiveItemEmpetyCalcute(dwVnum, dwCount, window_type);
			if (empetycount > 0)
			{
				switch (window_type)
				{
				case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
				case INVENTORY: { p_emp.norm += empetycount; break; }
				case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
				case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
				case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
				default: break;
				}
				empetycalcute = true;
			}
		}
		if (empetycalcute)
		{
			if (!GiveItemEmpetyCalcute2(&p_emp))
			{
				switch (empetyminval)
				{
					case 1: {opencount -= CHEST_OPEN_COUNT_CONTROL2; break; }
					case 2: {opencount -= 120; break; }
					default: 
					{
						NewChatPacket(CHEST_GIVE_STOP_DROP);
						return;
					}
				}
				if (opencount == 0)
				{
					NewChatPacket(CHEST_GIVE_STOP_DROP);
					return;
				}
			}
		}
	}//EmpetyMin
	
	
	if (item->GetCount() >= opencount)
	{
		item->SetCount(itemcount - opencount);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		PointChange(POINT_OPENED_CHEST, opencount);
#endif
	}
	else
	{
		NewChatPacket(GLOBAL_ERR_OCCURED);
		return;
	}

	int stackcount = 0;

#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (int i = 0; i < n; i++)
	{
		dwVnum = grp->GetVnum(i);
		dwCount = grp->GetCount(i) * opencount;

		if (dwCount > g_bItemCountLimit)
		{
			stackcount = dwCount / g_bItemCountLimit;
			for (int istack = 0; istack < stackcount; istack++)
			{
				AutoGiveItem(dwVnum, g_bItemCountLimit,-1,false);
			}
			DWORD dwCount2 = 0;
			dwCount2 = dwCount - (stackcount * g_bItemCountLimit);
			if (dwCount2 > 0)
			{
				AutoGiveItem(dwVnum, dwCount2);
			}
		}
		else
		{
			AutoGiveItem(dwVnum, dwCount);
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, true, true);
#endif

	NewChatPacket(CHEST_OPEN_COUNT, "%d", opencount);
	if (stop_drop)
	{
		NewChatPacket(CHEST_GIVE_STOP_DROP);
	}
}

void CHARACTER::ChestOpenConstSingle(LPITEM item)
{
	if (!item)
		return;
	//grp sandigin grup bilgisini alir
	const CSpecialItemGroup* grp = ITEM_MANAGER::Instance().GetSpecialItemGroup(item->GetVnum());
	if (!grp)
		return;

	const int n = grp->m_vecProbs.size();
	if (n == 0)
		return;

	DWORD itemcount = item->GetCount();
	DWORD opencount = 1;

	DWORD dwVnum = 0;
	DWORD dwCount = 0;
	bool empetycalcute = false;
	BYTE window_type = 0;
	int empetycount = 0;
	//Envanterde yeterli yer varmi
	TChestEmpetInventory p_emp;
	for (int i = 0; i < n; i++)
	{
		dwVnum = grp->GetVnum(i);
		dwCount = grp->GetCount(i) * opencount;
		window_type = VnumGetWindowType(dwVnum);
		empetycount = GiveItemEmpetyCalcute(dwVnum, dwCount, window_type);
		if (empetycount > 0)
		{
			switch (window_type)
			{
				case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
				case INVENTORY: { p_emp.norm += empetycount; break; }
				case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
				case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
				case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
				default: break;
			}
			empetycalcute = true;
		}
	}

	if (empetycalcute)
	{
		if (!GiveItemEmpetyCalcute2(&p_emp))
		{
			NewChatPacket(CHEST_GIVE_STOP_DROP);
			return;
		}
	}

	if (item->GetCount() >= 1)
	{
		item->SetCount(itemcount - 1);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		PointChange(POINT_OPENED_CHEST, opencount);
#endif
	}
	else
	{
		NewChatPacket(GLOBAL_ERR_OCCURED);
		return;
	}

	int stackcount = 0;
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (int i = 0; i < n; i++)
	{
		dwVnum = grp->GetVnum(i);
		dwCount = grp->GetCount(i) * opencount;

		if (dwCount > g_bItemCountLimit)
		{
			stackcount = dwCount / g_bItemCountLimit;
			for (int istack = 0; istack < stackcount; istack++)
			{
				AutoGiveItem(dwVnum, g_bItemCountLimit);
			}
			DWORD dwCount2 = 0;
			dwCount2 = dwCount - (stackcount * g_bItemCountLimit);
			if (dwCount2 > 0)
			{
				AutoGiveItem(dwVnum, dwCount2);
			}
		}
		else
		{
			AutoGiveItem(dwVnum, dwCount);
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, true, true);
#endif
}

/************************************************************************************************************************************************
GiveCalcSpecialGroup sandýðýn vnumunu gönderip vectorle kaç sandýk açtýðýný count u refarans alarak
vnum ve itemcountunu vectore refarans ile pushlucak bir struct ve map oluþturulup dönen deðerler mapte vector içinde toplanýcak
mapte itemvnum sabit deðer olup count pushlancak bu hesaplama her 20 sandýk için yapýlýcak
her 20 sandýkta i++ verilcek mapte envanterde boþ olup olmadýðý hesaplanýcak bu hesap gönderilcek parametler 
(itemvnum,itemcount,itemsize,windowtype,(struct bzsize 2 ve 3 olarak ayrý olucak b size 2 yada 3 olursa refarans olarak dönücek))
fonksyonda stacklenebilirliði ne gerekli hesaplamalar yapýlýp nekadar boþ yer ihtiyacý olduðu hesaplanýp TChestEmpetInventory structýna window tipi ile 
birlikte yazýlýcak her sefer deðer artýcak ayrýca mape yazýlan item vnumlarýn size ý alýnarak yazýcak ona göre
structa toplanan veri en son hesaplamak için bir fonksyona gönderilicek fonksyona giden 
parametreler ( TChestEmpetInventory,TChestEmpetInventorySize)
buraya gelen parametler de ilk önce bir vektör oluþup inventory tipinde hesaplama yapýlýrken bulunan boþ yerler
vectore yazýlýcak daha sonra 2 ve 3 size için ayrý bir hesaplama yapýlarak vector true yada false döndürülcek
************************************************************************************************************************************************/

void CHARACTER::ChestOpenNewPctSingle(LPITEM item)
{
	if (!item) { return; }
	DWORD chestcount = 0;
	chestcount = item->GetCount();

	std::map<DWORD, TChestGiveItem> map_giveitem;
	map_giveitem.clear();
	DWORD dwBoxVnum = item->GetVnum();

	bool stopdrop = false;
	std::vector <DWORD> dwVnums;
	std::vector <DWORD> dwCounts;
	int count = 0;

	if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
	{
		for (int i = 0; i < count; i++)
		{
			TChestGiveItem cItem;
			cItem.count = dwCounts[i];
			cItem.vnum = dwVnums[i];
			std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(dwVnums[i]);
			if (it == map_giveitem.end())
			{
				map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
			}
			else
			{
				it->second.count = it->second.count + cItem.count;
			}
		}
	}

	if (map_giveitem.size() > 0)
	{
		TChestEmpetInventory p_emp;
		TChestEmpetInventorySize p_emp_size;
		bool givecalcute_emp = false;
		//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
		for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_giveitem.begin(); it_calcute != map_giveitem.end(); ++it_calcute)
		{
			TChestGiveItem cItem;
			cItem.count = it_calcute->second.count;
			cItem.vnum = it_calcute->second.vnum;
			TItemTable* p_table = ITEM_MANAGER::instance().GetTable(cItem.vnum);
			if (!p_table) { return; }
			BYTE itemsize = p_table->bSize;

			if (itemsize != 1)
			{
				if (itemsize == 2)
					p_emp_size.size2 += cItem.count;
				else if (itemsize == 3)
					p_emp_size.size3 += cItem.count;
				givecalcute_emp = true;
			}
			else
			{
				int window_type = VnumGetWindowType(cItem.vnum);
				int empetycount = GiveItemEmpetyCalcute(cItem.vnum, cItem.count, window_type);
				if (empetycount > 0)
				{
					switch (window_type)
					{
					case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
					case INVENTORY: { p_emp.norm += empetycount; break; }
					case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
					case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
					case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
					default: break;
					}
					givecalcute_emp = true;
				}
			}
		}
		//Yeterli slot boþluðu olup olmadýðýný hesaplýyor
		if (givecalcute_emp)
		{
			if (!GiveItemEmpetyCalcute2Pct(&p_emp, &p_emp_size))
			{
				stopdrop = true;
			}
		}
	}//mapsize
	else { return; }

	if (item->GetCount() >= 1)
	{
		item->SetCount(chestcount - 1);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		PointChange(POINT_OPENED_CHEST, 1);
#endif
	}
	else
	{
		NewChatPacket(GLOBAL_ERR_OCCURED);
		return;
	}

	if (stopdrop)
	{
		NewChatPacket(CHEST_GIVE_STOP_DROP);
	}

	int stackcount = 0;

	std::map<DWORD, TChestGiveItem> map_GiveSize1;
	std::map<DWORD, TChestGiveItem> map_GiveSize2;
	map_GiveSize1.clear();
	map_GiveSize2.clear();

#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_giveitem.begin(); it_give != map_giveitem.end(); ++it_give)
	{
		DWORD count = it_give->second.count;
		DWORD vnum = it_give->second.vnum;
		TItemTable* p_table = ITEM_MANAGER::instance().GetTable(vnum);
		BYTE itemsize = p_table->bSize;
		if (itemsize == 3)
		{
			for (int i = 0; i < count; ++i)
			{
				AutoGiveItem(vnum, 1);
			}
		}
		else
		{
			TChestGiveItem cItem;
			cItem.count = count;
			cItem.vnum = vnum;
			if (itemsize == 2)
			{
				std::map<DWORD, TChestGiveItem>::iterator it = map_GiveSize2.find(cItem.vnum);
				if (it == map_GiveSize2.end())
				{
					map_GiveSize2.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
				}
				else
				{
					it->second.count = it->second.count + cItem.count;
				}
			}
			else
			{
				std::map<DWORD, TChestGiveItem>::iterator it = map_GiveSize1.find(cItem.vnum);
				if (it == map_GiveSize1.end())
				{
					map_GiveSize1.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
				}
				else
				{
					it->second.count = it->second.count + cItem.count;
				}
			}
		}
	}

	if (map_GiveSize2.size() > 0)
	{
		for (std::map<DWORD, TChestGiveItem>::iterator it_givesize2 = map_GiveSize2.begin(); it_givesize2 != map_GiveSize2.end(); ++it_givesize2)
		{
			DWORD count = it_givesize2->second.count;
			DWORD vnum = it_givesize2->second.vnum;
			for (int i = 0; i < count; ++i)
			{
				AutoGiveItem(vnum, 1);
			}
		}
	}

	if (map_GiveSize1.size() > 0)
	{
		for (std::map<DWORD, TChestGiveItem>::iterator it_givesize1 = map_GiveSize1.begin(); it_givesize1 != map_GiveSize1.end(); ++it_givesize1)
		{
			DWORD count = it_givesize1->second.count;
			DWORD vnum = it_givesize1->second.vnum;
			TItemTable* p_table = ITEM_MANAGER::instance().GetTable(vnum);

			if (p_table->dwFlags & ITEM_FLAG_STACKABLE)
			{
				if (count > g_bItemCountLimit)
				{
					stackcount = count / g_bItemCountLimit;
					for (int istack = 0; istack < stackcount; istack++)
					{
						AutoGiveItem(vnum, g_bItemCountLimit);
					}
					DWORD dwCount2 = 0;
					dwCount2 = count - (stackcount * g_bItemCountLimit);
					if (dwCount2 > 0)
					{
						AutoGiveItem(vnum, dwCount2);
					}
				}
				else
				{
					AutoGiveItem(vnum, count);
				}
			}
			else
			{
				for (int i = 0; i < count; ++i)
				{
					AutoGiveItem(vnum, 1);
				}
			}
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, true, true);
#endif
}

void CHARACTER::ChestOpenNewPct(LPITEM item, BYTE chesttype)
{
	if (!item) { return; }
	int CHEST_OPEN_COUNT_CONTROL_PCT = 0;
	int MAX_CHEST_OPEN_COUNT_PCT = 0;
	int MIN_CHEST_OPEN_COUNT = 0;

	switch (chesttype)
	{
		case CHEST_TYPE_NEWPCT:
		{
			CHEST_OPEN_COUNT_CONTROL_PCT = OPEN_COUNT_CONTROL_PCT;
			MAX_CHEST_OPEN_COUNT_PCT = MAX_CHEST_OPEN_PCT;
			MIN_CHEST_OPEN_COUNT = MIN_CHEST_OPEN_COUNT_PCT;
			break;
		}
		case CHEST_TYPE_STONE:
		{
			CHEST_OPEN_COUNT_CONTROL_PCT = OPEN_COUNT_CONTROL_STONE;
			MAX_CHEST_OPEN_COUNT_PCT = MAX_CHEST_OPEN_STONE;
			MIN_CHEST_OPEN_COUNT = MIN_CHEST_OPEN_COUNT_STONE;
			break;
		}
		case CHEST_TYPE_NORM:
		{
			CHEST_OPEN_COUNT_CONTROL_PCT = OPEN_COUNT_CONTROL_NORM;
			MAX_CHEST_OPEN_COUNT_PCT = MAX_CHEST_OPEN_NORM;
			MIN_CHEST_OPEN_COUNT = MIN_CHEST_OPEN_COUNT_NORM;
			break;
		}
		default: return;	break;
	}

	DWORD chestcount = 0;
	chestcount = item->GetCount();
	DWORD opencount = 1;

	opencount = chestcount < MAX_CHEST_OPEN_COUNT_PCT ? chestcount : MAX_CHEST_OPEN_COUNT_PCT;
	int multicount = 0;
	std::map<DWORD, TChestGiveItem> map_giveitem;
	map_giveitem.clear();
	DWORD dwBoxVnum = item->GetVnum();
	
	bool stopdrop = false;
	int stopdropvalue = 0;

	if (opencount > CHEST_OPEN_COUNT_CONTROL_PCT)
	{
		multicount = opencount / CHEST_OPEN_COUNT_CONTROL_PCT;
		for (int a = 0; a < multicount; a++)
		{
			TChestEmpetInventory p_emp;
			TChestEmpetInventorySize p_emp_size;
			std::map<DWORD, TChestGiveItem> map_givecalcute;
			map_givecalcute.clear();

			//20 sandýk drobu hesaplýyýp map'e yazýyor
			for (int c = 0; c < CHEST_OPEN_COUNT_CONTROL_PCT; c++)
			{
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				int count = 0;

				if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
				{
					for (int i = 0; i < count; i++)
					{
						TChestGiveItem cItem;
						cItem.count = dwCounts[i];
						cItem.vnum = dwVnums[i];
						std::map<DWORD, TChestGiveItem>::iterator it = map_givecalcute.find(dwVnums[i]);
						if (it == map_givecalcute.end())
						{
							map_givecalcute.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
						}
						else 
						{
							it->second.count = it->second.count + cItem.count;
						}
					}
				}
			}

			if (map_givecalcute.size() > 0)
			{
				//Hesaplama için yeni bir map oluþturur
				std::map<DWORD, TChestGiveItem> map_giveitem_copy;
				map_giveitem_copy.clear();
				map_giveitem_copy = map_giveitem;
				for (std::map<DWORD, TChestGiveItem>::iterator it_give_calcute = map_givecalcute.begin(); it_give_calcute != map_givecalcute.end(); ++it_give_calcute)
				{
					TChestGiveItem cItem;
					cItem.count = it_give_calcute->second.count;
					cItem.vnum = it_give_calcute->second.vnum;
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem_copy.find(cItem.vnum);
					if (it == map_giveitem_copy.end())
					{
						map_giveitem_copy.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}

				bool givecalcute_emp = false;
				//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
				for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_giveitem_copy.begin(); it_calcute != map_giveitem_copy.end(); ++it_calcute)
				{
					TChestGiveItem cItem;
					cItem.count = it_calcute->second.count;
					cItem.vnum = it_calcute->second.vnum;
					TItemTable* p_table = ITEM_MANAGER::instance().GetTable(cItem.vnum);
					if (!p_table) { return; }
					BYTE itemsize = p_table->bSize;

					if (itemsize != 1)
					{
						if (itemsize == 2)
							p_emp_size.size2 += cItem.count;
						else if (itemsize == 3)
							p_emp_size.size3 += cItem.count;
						givecalcute_emp = true;
					}
					else
					{
						int window_type = VnumGetWindowType(cItem.vnum);
						int empetycount = GiveItemEmpetyCalcute(cItem.vnum, cItem.count, window_type);
						if (empetycount > 0)
						{
							switch (window_type)
							{
								case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
								case INVENTORY: { p_emp.norm += empetycount; break; }
								case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
								case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
								case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
								default: break;
							}
							givecalcute_emp = true;
						}
					}
				}
				//Yeterli slot boþluðu olup olmadýðýný hesaplýyor
				if (givecalcute_emp)
				{
					if (!GiveItemEmpetyCalcute2Pct(&p_emp, &p_emp_size))
					{
						stopdrop = true;
						
						if (a == 0)
						{
							opencount = MIN_CHEST_OPEN_COUNT;
							stopdropvalue = 1;
						}
						else
						{
							opencount = a * CHEST_OPEN_COUNT_CONTROL_PCT;
						}
						break;
					}
				}
				//Yeterli boþluk varsa yeni hesaplanan itemler itemlerin verilceðis mape aktarýlýr
				for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_givecalcute.begin(); it_give != map_givecalcute.end(); ++it_give)
				{
					TChestGiveItem cItem;
					cItem.count = it_give->second.count;
					cItem.vnum = it_give->second.vnum;
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(cItem.vnum);
					if (it == map_giveitem.end())
					{
						map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}
			}//if map size
			else { return; }
		}//for multicount
		int calcutecount = opencount - (multicount * CHEST_OPEN_COUNT_CONTROL_PCT);
		if (!stopdrop && calcutecount > 0)
		{
			TChestEmpetInventory p_emp;
			TChestEmpetInventorySize p_emp_size;
			std::map<DWORD, TChestGiveItem> map_givecalcute;
			map_givecalcute.clear();

			//multicounttan geriye kalan itemler hesaplanýyor
			
			for (int c = 0; c < calcutecount; c++)
			{
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				int count = 0;

				if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
				{
					for (int i = 0; i < count; i++)
					{
						TChestGiveItem cItem;
						cItem.count = dwCounts[i];
						cItem.vnum = dwVnums[i];
						std::map<DWORD, TChestGiveItem>::iterator it = map_givecalcute.find(dwVnums[i]);
						if (it == map_givecalcute.end())
						{
							map_givecalcute.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
						}
						else
						{
							it->second.count = it->second.count + cItem.count;
						}
					}
				}
			}

			if (map_givecalcute.size() > 0)
			{
				//Hesaplama için yeni bir map oluþturur
				std::map<DWORD, TChestGiveItem> map_giveitem_copy;
				map_giveitem_copy.clear();
				map_giveitem_copy = map_giveitem;
				for (std::map<DWORD, TChestGiveItem>::iterator it_give_calcute = map_givecalcute.begin(); it_give_calcute != map_givecalcute.end(); ++it_give_calcute)
				{
					TChestGiveItem cItem;
					cItem.count = it_give_calcute->second.count;
					cItem.vnum = it_give_calcute->second.vnum;
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem_copy.find(cItem.vnum);
					if (it == map_giveitem_copy.end())
					{
						map_giveitem_copy.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}

				bool givecalcute_emp = false;
				//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
				for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_giveitem_copy.begin(); it_calcute != map_giveitem_copy.end(); ++it_calcute)
				{
					TChestGiveItem cItem;
					cItem.count = it_calcute->second.count;
					cItem.vnum = it_calcute->second.vnum;
					TItemTable* p_table = ITEM_MANAGER::instance().GetTable(cItem.vnum);
					if (!p_table) { return; }
					BYTE itemsize = p_table->bSize;

					if (itemsize != 1)
					{
						if (itemsize == 2)
							p_emp_size.size2 += cItem.count;
						else if (itemsize == 3)
							p_emp_size.size3 += cItem.count;
						givecalcute_emp = true;
					}
					else
					{
						int window_type = VnumGetWindowType(cItem.vnum);
						int empetycount = GiveItemEmpetyCalcute(cItem.vnum, cItem.count, window_type);
						if (empetycount > 0)
						{
							switch (window_type)
							{
							case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
							case INVENTORY: { p_emp.norm += empetycount; break; }
							case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
							case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
							case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
							default: break;
							}
							givecalcute_emp = true;
						}
					}
				}
				//Yeterli slot boþluðu olup olmadýðýný hesaplýyor
				if (givecalcute_emp)
				{
					if (!GiveItemEmpetyCalcute2Pct(&p_emp, &p_emp_size))
					{
						stopdrop = true;
						opencount = multicount * CHEST_OPEN_COUNT_CONTROL_PCT;
					}
				}
				//Yeterli boþluk varsa yeni hesaplanan itemler itemlerin verilceðis mape aktarýlýr
				if (!stopdrop)
				{
					for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_givecalcute.begin(); it_give != map_givecalcute.end(); ++it_give)
					{
						TChestGiveItem cItem;
						cItem.count = it_give->second.count;
						cItem.vnum = it_give->second.vnum;
						std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(cItem.vnum);
						if (it == map_giveitem.end())
						{
							map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
						}
						else
						{
							it->second.count = it->second.count + cItem.count;
						}
					}
				}

			}//if map size
			else { return; }
		}
	}//if opencount > countcontrol
	else
	{
		//Çýkýcak itemleri hesaplayýp mape yazýyor
		for (int c = 0; c < opencount; c++)
		{
			std::vector <DWORD> dwVnums;
			std::vector <DWORD> dwCounts;
			int count = 0;
			if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
			{
				for (int i = 0; i < count; i++)
				{
					TChestGiveItem cItem;
					cItem.count = dwCounts[i];
					cItem.vnum = dwVnums[i];
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(dwVnums[i]);
					if (it == map_giveitem.end())
					{
						map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}
			}
		}

		if (map_giveitem.size() > 0)
		{
			TChestEmpetInventory p_emp;
			TChestEmpetInventorySize p_emp_size;
			bool givecalcute_emp = false;
			//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
			for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_giveitem.begin(); it_calcute != map_giveitem.end(); ++it_calcute)
			{
				TChestGiveItem cItem;
				cItem.count = it_calcute->second.count;
				cItem.vnum = it_calcute->second.vnum;
				TItemTable* p_table = ITEM_MANAGER::instance().GetTable(cItem.vnum);
				if (!p_table) { return; }
				BYTE itemsize = p_table->bSize;

				if (itemsize != 1)
				{
					if (itemsize == 2)
						p_emp_size.size2 += cItem.count;
					else if (itemsize == 3)
						p_emp_size.size3 += cItem.count;
					givecalcute_emp = true;
				}
				else
				{
					int window_type = VnumGetWindowType(cItem.vnum);
					int empetycount = GiveItemEmpetyCalcute(cItem.vnum, cItem.count, window_type);
					if (empetycount > 0)
					{
						switch (window_type)
						{
							case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
							case INVENTORY: { p_emp.norm += empetycount; break; }
							case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
							case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
							case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
							default: break;
						}
						givecalcute_emp = true;
					}
				}
			}
			//Yeterli slot boþluðu olup olmadýðýný hesaplýyor
			if (givecalcute_emp)
			{
				if (!GiveItemEmpetyCalcute2Pct(&p_emp, &p_emp_size))
				{
					opencount = MIN_CHEST_OPEN_COUNT;
					stopdrop = true;
					stopdropvalue = 1;
				}
			}
		}//mapsize
		else { return; }
	}

	if (stopdrop && stopdropvalue == 1)
	{
		std::map<DWORD, TChestGiveItem> map_givecalcute;
		map_givecalcute.clear();

		//minimun sandýk drobu hesaplýyýp map'e yazýyor
		for (int c = 0; c < MIN_CHEST_OPEN_COUNT; c++)
		{
			std::vector <DWORD> dwVnums;
			std::vector <DWORD> dwCounts;
			int count = 0;

			if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
			{
				for (int i = 0; i < count; i++)
				{
					TChestGiveItem cItem;
					cItem.count = dwCounts[i];
					cItem.vnum = dwVnums[i];
					std::map<DWORD, TChestGiveItem>::iterator it = map_givecalcute.find(dwVnums[i]);
					if (it == map_givecalcute.end())
					{
						map_givecalcute.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}
			}
		}

		if (map_givecalcute.size() > 0)
		{
			TChestEmpetInventory p_emp;
			TChestEmpetInventorySize p_emp_size;
			bool givecalcute_emp = false;
			//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
			for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_givecalcute.begin(); it_calcute != map_givecalcute.end(); ++it_calcute)
			{
				TChestGiveItem cItem;
				cItem.count = it_calcute->second.count;
				cItem.vnum = it_calcute->second.vnum;
				TItemTable* p_table = ITEM_MANAGER::instance().GetTable(cItem.vnum);
				if (!p_table) { return; }
				BYTE itemsize = p_table->bSize;

				if (itemsize != 1)
				{
					if (itemsize == 2)
						p_emp_size.size2 += cItem.count;
					else if (itemsize == 3)
						p_emp_size.size3 += cItem.count;
					givecalcute_emp = true;
				}
				else
				{
					int window_type = VnumGetWindowType(cItem.vnum);
					int empetycount = GiveItemEmpetyCalcute(cItem.vnum, cItem.count, window_type);
					if (empetycount > 0)
					{
						switch (window_type)
						{
							case UPGRADE_INVENTORY: { p_emp.upgrade += empetycount; break; }
							case INVENTORY: { p_emp.norm += empetycount; break; }
							case CHEST_INVENTORY: {  p_emp.chest += empetycount; break; }
							case BOOK_INVENTORY: { p_emp.book += empetycount; break; }
							case STONE_INVENTORY: {p_emp.stone += empetycount; break; }
							default: break;
						}
						givecalcute_emp = true;
					}
				}
			}
			//Yeterli slot boþluðu olup olmadýðýný hesaplýyor
			if (givecalcute_emp)
			{
				if (!GiveItemEmpetyCalcute2Pct(&p_emp, &p_emp_size))
				{
					if (stopdropvalue != 0)
					{
						NewChatPacket(CHEST_GIVE_STOP_DROP);
						return;
					}
				}
			}
			map_giveitem.clear();
			for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_givecalcute.begin(); it_give != map_givecalcute.end(); ++it_give)
			{
				TChestGiveItem cItem;
				cItem.count = it_give->second.count;
				cItem.vnum = it_give->second.vnum;
				std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(cItem.vnum);
				if (it == map_giveitem.end())
				{
					map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
				}
				else
				{
					it->second.count = it->second.count + cItem.count;
				}
			}
		}//if map size
		else { return; }
	}//stopdrop

	if (opencount < 1)
	{
		if (stopdrop)
		{
			NewChatPacket(CHEST_GIVE_STOP_DROP);
		}
		return;
	}

	if (item->GetCount() >= opencount)
	{
		item->SetCount(chestcount - opencount);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		PointChange(POINT_OPENED_CHEST, opencount);
#endif
	}	
	else
	{
		NewChatPacket(GLOBAL_ERR_OCCURED);
		return;
	}

	int stackcount = 0;

	std::map<DWORD, TChestGiveItem> map_GiveSize1;
	std::map<DWORD, TChestGiveItem> map_GiveSize2;
	map_GiveSize1.clear();
	map_GiveSize2.clear();
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, false);
#endif
	for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_giveitem.begin(); it_give != map_giveitem.end(); ++it_give)
	{
		DWORD count = it_give->second.count;
		DWORD vnum = it_give->second.vnum;
		TItemTable* p_table = ITEM_MANAGER::instance().GetTable(vnum);
		BYTE itemsize = p_table->bSize;
		if (itemsize == 3)
		{
			for (int i = 0; i < count; ++i)
			{
				AutoGiveItem(vnum, 1);
			}
		}
		else
		{
			TChestGiveItem cItem;
			cItem.count = count;
			cItem.vnum = vnum;
			if (itemsize == 2)
			{
				std::map<DWORD, TChestGiveItem>::iterator it = map_GiveSize2.find(cItem.vnum);
				if (it == map_GiveSize2.end())
				{
					map_GiveSize2.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
				}
				else
				{
					it->second.count = it->second.count + cItem.count;
				}
			}
			else
			{
				std::map<DWORD, TChestGiveItem>::iterator it = map_GiveSize1.find(cItem.vnum);
				if (it == map_GiveSize1.end())
				{
					map_GiveSize1.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
				}
				else
				{
					it->second.count = it->second.count + cItem.count;
				}
			}
		}
	}

	if (map_GiveSize2.size() > 0)
	{
		for (std::map<DWORD, TChestGiveItem>::iterator it_givesize2 = map_GiveSize2.begin(); it_givesize2 != map_GiveSize2.end(); ++it_givesize2)
		{
			DWORD count = it_givesize2->second.count;
			DWORD vnum = it_givesize2->second.vnum;

			for (int i = 0; i < count; ++i)
			{
				AutoGiveItem(vnum, 1);
			}
		}
	}

	if (map_GiveSize1.size() > 0)
	{
		for (std::map<DWORD, TChestGiveItem>::iterator it_givesize1 = map_GiveSize1.begin(); it_givesize1 != map_GiveSize1.end(); ++it_givesize1)
		{
			DWORD count = it_givesize1->second.count;
			DWORD vnum = it_givesize1->second.vnum;
			TItemTable* p_table = ITEM_MANAGER::instance().GetTable(vnum);

			if (p_table->dwFlags & ITEM_FLAG_STACKABLE)
			{
				if (count > g_bItemCountLimit)
				{
					stackcount = count / g_bItemCountLimit;
					for (int istack = 0; istack < stackcount; istack++)
					{
						AutoGiveItem(vnum, g_bItemCountLimit);
					}
					DWORD dwCount2 = 0;
					dwCount2 = count - (stackcount * g_bItemCountLimit);
					if (dwCount2 > 0)
					{
						AutoGiveItem(vnum, dwCount2);
					}
				}
				else
				{
					AutoGiveItem(vnum, count);
				}
			}
			else
			{
				for (int i = 0; i < count; ++i)
				{
					AutoGiveItem(vnum, 1);
				}
			}
		}
	}
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, true, true);
#endif
	NewChatPacket(CHEST_OPEN_COUNT, "%d", opencount);
	if (stopdrop)
	{
		NewChatPacket(CHEST_GIVE_STOP_DROP);
	}
}

bool CHARACTER::GiveCalcSpecialGroup(DWORD dwGroupNum, std::vector<DWORD>& dwItemVnums,
	std::vector<DWORD>& dwItemCounts, int& count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);
	if (!pGroup) { return false; }

	/*
	-idxes vectorü GetMultiIndex fonksyonundan refarans olarak dönüyor
	-içinde þans hesaplamalarýnda çýkan indexleri barýndýrýyor
	-multiindex te return eden deðer ise sandýktan toplamda 
	-kaç item çýkacaðý
	*/
	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);
	if (n < 1)
		return false;

	for (int i = 0; i < n; i++)
	{
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		dwItemCounts.push_back(dwCount);
		dwItemVnums.push_back(dwVnum);
		count++;
	}
	return true;
}

bool CHARACTER::GiveItemEmpetyCalcute2Pct(TChestEmpetInventory* empety, TChestEmpetInventorySize* itemsize)

{
	if (!empety || !itemsize) { return false; }
	int EmpCount;
	BYTE bSize = 1;
	std::vector<int> v_index;
	v_index.clear();

	if (itemsize->size3 > 0)
	{
		EmpCount = 0;
		for (int a = 0; a < SPECIAL_INVENTORY_MAX_NUM; ++a)
		{
			bool contin = false;
			if (IsEmptyItemGrid(TItemPos(INVENTORY, a), 3))
			{
				for (int x = 0; x < v_index.size(); ++x)
				{
					if (v_index[x] == a)
					{
						contin = true;
						break;
					}
				}
				if (contin) { continue; }
				else
				{
					v_index.push_back(a);
					v_index.push_back(a+5);
					v_index.push_back(a+10);
					++EmpCount;
					if (EmpCount == itemsize->size3)
					{
						break;
					}
				}
			}
		}
		if (itemsize->size3 > EmpCount) { return false; }
	}//itemsize 3

	if (itemsize->size2 > 0)
	{
		EmpCount = 0;
		for (int a = 0; a < SPECIAL_INVENTORY_MAX_NUM; ++a)
		{
			bool contin = false;
			if (IsEmptyItemGrid(TItemPos(INVENTORY, a), 2))
			{
				for (int x = 0; x < v_index.size(); ++x)
				{
					if (v_index[x] == a)
					{
						contin = true;
						break;
					}
				}
				if (contin) { continue; }
				else
				{
					++EmpCount;
					v_index.push_back(a);
					v_index.push_back(a + 5);
					if (itemsize->size2 == EmpCount)
					{
						break;
					}

				}
			}
		}
		if (itemsize->size2 > EmpCount) { return false; }
	}//itemsize2
	if (empety->norm > 0)
	{
		EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			bool contin = false;
			if (IsEmptyItemGrid(TItemPos(INVENTORY, i), bSize)) 
			{
				for (int x = 0; x < v_index.size(); ++x)
				{
					if (v_index[x] == i)
					{
						contin = true;
						break;
					}
				}
				if (contin) { continue; }
				else
				{
					++EmpCount;
					v_index.push_back(i);
					if (empety->norm == EmpCount)
					{
						break;
					}
				}
			}
		}
		if (empety->norm > EmpCount) { return false; }
	}//empety norm

	if (empety->upgrade > 0) 
	{
		EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos(UPGRADE_INVENTORY, i), bSize)) 
			{
				++EmpCount;
				if (empety->upgrade == EmpCount)
					break;
			}
		}
		if (empety->upgrade > EmpCount) { return false; }
	}
	if (empety->chest > 0)
	{
		EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (IsEmptyItemGrid(TItemPos(CHEST_INVENTORY, i), bSize))
			{
				++EmpCount;
				if (empety->chest == EmpCount)
					break;
			}
		}
		if (empety->chest > EmpCount) { return false; }
	}
	if (empety->book > 0) 
	{
		EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (IsEmptyItemGrid(TItemPos(BOOK_INVENTORY, i), bSize))
			{
				++EmpCount;
				if (empety->book == EmpCount)
					break;
			}
		}
		if (empety->book > EmpCount) { return false; }
	}

	if (empety->stone > 0)
	{
		EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i) {
			if (IsEmptyItemGrid(TItemPos(STONE_INVENTORY, i), bSize)) {
				++EmpCount;
				if (empety->stone == EmpCount)
					break;
			}
		}
		if (empety->stone > EmpCount) { return false; }
	}
	return true;
}

bool CHARACTER::ChestDSEmpty(DWORD itemcount, int type, DWORD chestVnum)
{
	int dsGrade = DsChestToGrade(chestVnum);
	if (dsGrade == -1)
		return false;

	int posstart = DragonSoulPosList[type][dsGrade];

	if (itemcount > 0)
	{
		int EmpCount = 0;
		for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i) 
		{
			if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, posstart+i), 1))
			{
				++EmpCount;
				if (itemcount == EmpCount)
					break;
			}
		}
		if (itemcount > EmpCount) { return false; }
	}
	return true;
}

int	CHARACTER::ChestDSEmptyCount(DWORD chestVnum)
{
	int dsGrade = DsChestToGrade(chestVnum);
	if (dsGrade == -1)
		return -1;

	int type = ChestDSType(chestVnum, true);

	if (type == -1)
		return -1;

	int posstart = DragonSoulPosList[type][dsGrade];

	int emptyCount = 0;
	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
	{
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, posstart + i), 1))
		{
			emptyCount++;
		}
	}

	if (emptyCount > 0)
		return emptyCount;

	return -1;
}

int CHARACTER::ChestDSType(DWORD vnum, bool chest)
{
	if (chest)
	{
		if (vnum == 51511)
			return 0;
		else if (vnum == 51512)
			return 1;
		else if (vnum == 51513)
			return 2;
		else if (vnum == 51514)
			return 3;
		else if (vnum == 51515)
			return 4;
		else if (vnum == 51516)
			return 5;
	}
	else
	{
		if (vnum >= 110000 && vnum < 120000)
			return 0;
		else if (vnum >= 120000 && vnum < 130000)
			return 1;
		else if (vnum >= 130000 && vnum < 140000)
			return 2;
		else if (vnum >= 140000 && vnum < 150000)
			return 3;
		else if (vnum >= 150000 && vnum < 160000)
			return 4;
		else if (vnum >= 160000 && vnum < 170000)
			return 5;
	}

	return -1;
}

void CHARACTER::ChestOpenDragonSoul(LPITEM item, bool multiopen)
{
	if (!item) { return; }
	int CHEST_OPEN_COUNT_CONTROL_PCT = 10;
	int MAX_CHEST_OPEN_COUNT_PCT = 32;

	DWORD chestcount = 0;
	chestcount = item->GetCount();
	DWORD opencount = 1;

	if (multiopen)
	{
		opencount = chestcount < MAX_CHEST_OPEN_COUNT_PCT ? chestcount : MAX_CHEST_OPEN_COUNT_PCT;
		if (opencount < 1)
			return;

		if (item->GetVnum() != 200020)
		{
			int emptyCount = ChestDSEmptyCount(item->GetVnum());
			if (emptyCount != -1)
				opencount = chestcount < emptyCount ? chestcount : emptyCount;
		}
	}

	DWORD dwBoxVnum = item->GetVnum();
	int multicount = 0;
	std::map<DWORD, TChestGiveItem> map_giveitem;
	map_giveitem.clear();

	bool stopdrop = false;

	if (opencount > CHEST_OPEN_COUNT_CONTROL_PCT)
	{
		multicount = opencount / CHEST_OPEN_COUNT_CONTROL_PCT;
		for (int a = 0; a < multicount; a++)
		{
			std::map<DWORD, TChestGiveItem> map_givecalcute;
			map_givecalcute.clear();

			int controlcount = 0;
			if (a == 0)
			{
				int calcutecount = opencount - (multicount * CHEST_OPEN_COUNT_CONTROL_PCT);
				controlcount = CHEST_OPEN_COUNT_CONTROL_PCT + calcutecount;
			}
			else
				controlcount = CHEST_OPEN_COUNT_CONTROL_PCT;
				
				
			//control adedinde sandýk drobu hesaplýyýp map'e yazýyor
			for (int c = 0; c < controlcount; c++)
			{
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				int count = 0;

				if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
				{
					for (int i = 0; i < count; i++)
					{
						TChestGiveItem cItem;
						cItem.count = dwCounts[i];
						cItem.vnum = dwVnums[i];
						std::map<DWORD, TChestGiveItem>::iterator it = map_givecalcute.find(dwVnums[i]);
						if (it == map_givecalcute.end())
						{
							map_givecalcute.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
						}
						else
						{
							it->second.count = it->second.count + cItem.count;
						}
					}
				}
			}

			if (map_givecalcute.size() > 0)
			{
				//Hesaplama için yeni bir map oluþturur
				std::map<DWORD, TChestGiveItem> map_giveitem_copy;
				map_giveitem_copy.clear();
				map_giveitem_copy = map_giveitem;
				for (std::map<DWORD, TChestGiveItem>::iterator it_give_calcute = map_givecalcute.begin(); it_give_calcute != map_givecalcute.end(); ++it_give_calcute)
				{
					TChestGiveItem cItem;
					cItem.count = it_give_calcute->second.count;
					cItem.vnum = it_give_calcute->second.vnum;
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem_copy.find(cItem.vnum);
					if (it == map_giveitem_copy.end())
					{
						map_giveitem_copy.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}

				//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
				for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_giveitem_copy.begin(); it_calcute != map_giveitem_copy.end(); ++it_calcute)
				{
					int dstype = ChestDSType(it_calcute->second.vnum);
					if (!ChestDSEmpty(it_calcute->second.count, dstype, dwBoxVnum))
					{
						stopdrop = true;

						if (a == 0)
						{
							NewChatPacket(CHEST_GIVE_STOP_DROP);
							return;
						}
						else
						{
							int extracount = opencount - (multicount * CHEST_OPEN_COUNT_CONTROL_PCT);
							opencount = (a * CHEST_OPEN_COUNT_CONTROL_PCT) + extracount;
						}
						break;
					}
				}
				if (stopdrop)
					break;
				//Yeterli boþluk varsa yeni hesaplanan itemler itemlerin verilceðis mape aktarýlýr
				for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_givecalcute.begin(); it_give != map_givecalcute.end(); ++it_give)
				{
					TChestGiveItem cItem;
					cItem.count = it_give->second.count;
					cItem.vnum = it_give->second.vnum;
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(cItem.vnum);
					if (it == map_giveitem.end())
					{
						map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(cItem.vnum, cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}
			}//if map size
			else { return; }
		}//for multicount
	}//if opencount > countcontrol
	else
	{
		//Çýkýcak itemleri hesaplayýp mape yazýyor
		for (int c = 0; c < opencount; c++)
		{
			std::vector <DWORD> dwVnums;
			std::vector <DWORD> dwCounts;
			int count = 0;
			if (GiveCalcSpecialGroup(dwBoxVnum, dwVnums, dwCounts, count))
			{
				for (int i = 0; i < count; i++)
				{
					TChestGiveItem cItem;
					cItem.count = dwCounts[i];
					cItem.vnum = dwVnums[i];
					std::map<DWORD, TChestGiveItem>::iterator it = map_giveitem.find(dwVnums[i]);
					if (it == map_giveitem.end())
					{
						map_giveitem.insert(std::map<DWORD, TChestGiveItem>::value_type(dwVnums[i], cItem));
					}
					else
					{
						it->second.count = it->second.count + cItem.count;
					}
				}
			}
		}

		if (map_giveitem.size() > 0)
		{
			//Nekadar boþ slota ihtiyaç duyduðunu hesaplýyor
			for (std::map<DWORD, TChestGiveItem>::iterator it_calcute = map_giveitem.begin(); it_calcute != map_giveitem.end(); ++it_calcute)
			{
				int dstype = ChestDSType(it_calcute->second.vnum);
				if (!ChestDSEmpty(it_calcute->second.count, dstype, dwBoxVnum))
				{
					NewChatPacket(CHEST_GIVE_STOP_DROP);
					return;
				}
			}
		}//mapsize
		else { return; }
	}

	if (item->GetCount() >= opencount)
	{
		item->SetCount(item->GetCount() - opencount);
#ifdef ENABLE_PLAYER_STATS_SYSTEM
		PointChange(POINT_OPENED_CHEST, opencount);
#endif
	}
	else
	{
		NewChatPacket(GLOBAL_ERR_OCCURED);
		return;
	}
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY,false);
#endif
	for (std::map<DWORD, TChestGiveItem>::iterator it_give = map_giveitem.begin(); it_give != map_giveitem.end(); ++it_give)
	{
		DWORD count = it_give->second.count;
		DWORD vnum = it_give->second.vnum;
		for (int i = 0; i < count; ++i)
		{
			AutoGiveItem(vnum, 1,-1,false);
		}
		TItemTable* itemtable = ITEM_MANAGER::instance().GetTable(vnum);
		const char* itemname = itemtable->szLocaleName;
		ItemWinnerChat(count, DRAGON_SOUL_INVENTORY, itemname);
	}
#ifdef ENABLE_REFRESH_CONTROL
	RefreshControl(REFRESH_INVENTORY, true,true);
#endif
	if (stopdrop)
	{
		NewChatPacket(CHEST_GIVE_STOP_DROP);
	}
}

///Tüm simyalardan boþ yer varmý kontrol ediyoruz
bool CHARACTER::ChestDSInventoryEmpety(DWORD chestvnum)
{
	int dsGrade = DsChestToGrade(chestvnum);
	if (dsGrade == -1)
		return false;

	if (chestvnum == 200020)
	{
		for (int y = 0; y < DS_SLOT_MAX; ++y)
		{
			int EmpCount = 0;
			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
			{
				if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, DragonSoulPosList[y][dsGrade] + i), 1))
				{
					++EmpCount;
					if (2 == EmpCount)
						break;
				}
			}
			if (EmpCount < 2) { return false; }
		}
	}
	else
	{
		int dsType = ChestDSType(chestvnum, true);
		if (dsType == -1)
			return false;

		for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		{
			if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, DragonSoulPosList[dsType][dsGrade] + i), 1))
			{
				return true;
			}
		}
		return false;
	}

	return true;
}

bool CHARACTER::ChestInventoryEmpetyFull()
{
	int inv[5] = { INVENTORY, UPGRADE_INVENTORY,BOOK_INVENTORY,STONE_INVENTORY,CHEST_INVENTORY};

	for (int y = 0; y < 5; ++y)
	{
		int EmpCount = 0;
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			if (IsEmptyItemGrid(TItemPos(inv[y], i), 1))
			{
				++EmpCount;
				if (3 == EmpCount)
					break;
			}
		}
		if (3 > EmpCount) { return false; }
	}

	return true;
}
#endif



