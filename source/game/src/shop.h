#ifndef __INC_METIN_II_GAME_SHOP_H__
#define __INC_METIN_II_GAME_SHOP_H__

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

/* ---------------------------------------------------------------------------------- */
class CShop
{
public:
	typedef struct shop_item
	{
		DWORD	vnum;
#ifdef ENABLE_EXTENDED_YANG_LIMIT
		int64_t	price;
#else
		long	price;
#endif
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
		MAX_COUNT	count;
#else
		BYTE	count;
#endif

		LPITEM	pkItem;
		int		itemid;
#ifdef ENABLE_BUY_WITH_ITEMS
		TShopItemPrice	itemPrice[MAX_SHOP_PRICES];
#endif

		shop_item()
		{
			vnum = 0;
			price = 0;
			count = 0;
			itemid = 0;
#ifdef ENABLE_BUY_WITH_ITEMS
			memset(itemPrice, 0, sizeof(itemPrice));
#endif
			pkItem = NULL;
		}
	} SHOP_ITEM;

	CShop();
	virtual ~CShop(); // @fixme139 (+virtual)

	bool	Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable* pItemTable);
#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	void	SetShopItems(TShopItemTable* pItemTable, MAX_COUNT bItemCount);
#else
	void	SetShopItems(TShopItemTable* pItemTable, BYTE bItemCount);
#endif

	virtual void	SetPCShop(LPCHARACTER ch);
	virtual bool	IsPCShop() { return m_pkPC ? true : false; }

	virtual bool	AddGuest(LPCHARACTER ch, DWORD owner_vid, bool bOtherEmpire);
	void	RemoveGuest(LPCHARACTER ch);

#ifdef ENABLE_EXTENDED_YANG_LIMIT
	virtual int64_t	Buy(LPCHARACTER ch, BYTE pos
#ifdef ENABLE_MULTIPLE_BUY_ITEMS
		, bool multiple = false
#endif
	);
#else
	virtual int	Buy(LPCHARACTER ch, BYTE pos);
#endif
#ifdef ENABLE_MULTIPLE_BUY_ITEMS
	virtual uint8_t MultipleBuy(LPCHARACTER ch, uint8_t p, uint8_t c);
#endif
	void	BroadcastUpdateItem(BYTE pos);

	int		GetNumberByVnum(DWORD dwVnum);

	virtual bool	IsSellingItem(DWORD itemID);

	DWORD	GetVnum() { return m_dwVnum; }
	DWORD	GetNPCVnum() { return m_dwNPCVnum; }

protected:
	void	Broadcast(const void* data, int bytes);

protected:
	DWORD				m_dwVnum;
	DWORD				m_dwNPCVnum;

	CGrid* m_pGrid;

	typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
	GuestMapType m_map_guest;
	std::vector<SHOP_ITEM>		m_itemVector;

	LPCHARACTER			m_pkPC;
};

#endif
