#ifndef __INC_METIN_II_GAME_SHOP_MANAGER_H__
#define __INC_METIN_II_GAME_SHOP_MANAGER_H__

class CShop;
typedef class CShop* LPSHOP;

class CShopManager : public singleton<CShopManager>
{
public:
	typedef std::map<DWORD, CShop*> TShopMap;

public:
	CShopManager();
	virtual ~CShopManager();

	bool	Initialize(TShopTable* table, int size);
	void	Destroy();

	LPSHOP	Get(DWORD dwVnum);
	LPSHOP	GetByNPCVnum(DWORD dwVnum);

	bool	StartShopping(LPCHARACTER pkChr, LPCHARACTER pkShopKeeper, int iShopVnum = 0);
	void	StopShopping(LPCHARACTER ch);

	void	Buy(LPCHARACTER ch, BYTE pos);
#ifdef ENABLE_MULTIPLE_BUY_ITEMS
	void	MultipleBuy(LPCHARACTER ch, uint8_t p, uint8_t c);
#endif

	void	Sell(LPCHARACTER ch, CELL_UINT bCell,
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT bCount = 0
#else
		BYTE bCount = 0
#endif
#ifdef ENABLE_SPECIAL_STORAGE
		,BYTE bType = 0
#endif
	);


#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	LPSHOP	CreatePCShop(LPCHARACTER ch, TShopItemTable* pTable, MAX_COUNT bItemCount);
#else
	LPSHOP	CreatePCShop(LPCHARACTER ch, TShopItemTable* pTable, BYTE bItemCount);
#endif
	LPSHOP	FindPCShop(DWORD dwVID);
	void	DestroyPCShop(LPCHARACTER ch);

private:
	TShopMap	m_map_pkShop;
	TShopMap	m_map_pkShopByNPCVnum;
	TShopMap	m_map_pkShopByPC;

	bool	ReadShopTableEx(const char* stFileName);
};

#endif
