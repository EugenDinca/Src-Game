#ifndef __INCLUDE_NEW_OFFLINESHOP_HEADER__
#define __INCLUDE_NEW_OFFLINESHOP_HEADER__
#ifdef ENABLE_OFFLINE_SHOP

#define ENABLE_SHOP_GUESTLIST_FIX
#ifdef ENABLE_SHOP_GUESTLIST_FIX
#define GUEST_PTR(id)  (CHARACTER_MANAGER::instance().FindByPID(id))
#define GUEST_FROM_CHARACTER(id) (id->GetPlayerID())
#else
#define GUEST_PTR(i)  (i)
#define GUEST_FROM_CHARACTER(i) (i)
#endif

template <class T>
void ZeroObject(T& obj)
{
	obj = {};
	// memset(&obj, 0 , sizeof(obj)); // Volkana sor
}

template <class T>
void CopyObject(T& objDest, const T& objSrc)
{
	memcpy(&objDest, &objSrc, sizeof(objDest));
}

template <class T>
void CopyContainer(T& objDest, const T& objSrc)
{
	objDest = objSrc;
}

template <class T, size_t size>
void CopyArray(T(&objDest)[size], const T(&objSrc)[size])
{
	if (size == 0)
		return;
	memcpy(&objDest[0], &objSrc[0], sizeof(T) * size);
}

template <class T>
void DeletePointersContainer(T& obj)
{
	typename T::iterator it = obj.begin();
	for (; it != obj.end(); it++)
		delete(*it);
}

namespace offlineshop
{
	inline offlineshop::ExpirationType GetItemExpiration(LPITEM item) 
	{
		auto proto = item->GetProto();
		for (const auto limit : proto->aLimits) 
		{
			if (limit.bType == LIMIT_REAL_TIME)
				return offlineshop::ExpirationType::EXPIRE_REAL_TIME;
			else if (limit.bType == LIMIT_REAL_TIME_START_FIRST_USE && item->GetSocket(1) != 0)
				return offlineshop::ExpirationType::EXPIRE_REAL_TIME_FIRST_USE;
		} return offlineshop::ExpirationType::EXPIRE_NONE;
	}

	enum eOffshopChatPacket
	{
		CHAT_PACKET_CANNOT_CREATE_SHOP,
		CHAT_PACKET_CANNOT_CHANGE_NAME,
		CHAT_PACKET_CANNOT_FORCE_CLOSE,
		CHAT_PACKET_CANNOT_OPEN_SHOP,
		CHAT_PACKET_CANNOT_OPEN_SHOP_OWNER,
		CHAT_PACKET_CANNOT_ADD_ITEM,
		CHAT_PACKET_CANNOT_BUY_ITEM,
		CHAT_PACKET_CANNOT_REMOVE_ITEM,
		CHAT_PACKET_CANNOT_EDIT_ITEM,
		CHAT_PACKET_CANNOT_REMOVE_LAST_ITEM,
		CHAT_PACKET_CANNOT_FILTER,
		CHAT_PACKET_CANNOT_SEARCH_YET,
		CHAT_PACKET_CANNOT_OPEN_SAFEBOX,
		CHAT_PACKET_CANNOT_SAFEBOX_GET_ITEM,
		CHAT_PACKET_CANNOT_SAFEBOX_GET_VALUTES,
		CHAT_PACKET_CANNOT_SAFEBOX_CLOSE,
		CHAT_PACKET_RECV_ITEM_SAFEBOX,
		CHAT_PACKET_CANNOT_DO_NOW,
	};

	inline void SendChatPacket(LPCHARACTER ch, BYTE type)
	{
#define CASESEND(var) case var: ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(#var)); break;
		switch (type)
		{
			CASESEND(CHAT_PACKET_CANNOT_CREATE_SHOP)
				CASESEND(CHAT_PACKET_CANNOT_CHANGE_NAME)
				CASESEND(CHAT_PACKET_CANNOT_FORCE_CLOSE)
				CASESEND(CHAT_PACKET_CANNOT_OPEN_SHOP)
				CASESEND(CHAT_PACKET_CANNOT_OPEN_SHOP_OWNER)
				CASESEND(CHAT_PACKET_CANNOT_ADD_ITEM)
				CASESEND(CHAT_PACKET_CANNOT_BUY_ITEM)
				CASESEND(CHAT_PACKET_CANNOT_REMOVE_ITEM)
				CASESEND(CHAT_PACKET_CANNOT_EDIT_ITEM)
				CASESEND(CHAT_PACKET_CANNOT_REMOVE_LAST_ITEM)
				CASESEND(CHAT_PACKET_CANNOT_FILTER)
				CASESEND(CHAT_PACKET_CANNOT_SEARCH_YET)
				CASESEND(CHAT_PACKET_CANNOT_OPEN_SAFEBOX)
				CASESEND(CHAT_PACKET_CANNOT_SAFEBOX_GET_ITEM)
				CASESEND(CHAT_PACKET_CANNOT_SAFEBOX_GET_VALUTES)
				CASESEND(CHAT_PACKET_CANNOT_SAFEBOX_CLOSE)
				CASESEND(CHAT_PACKET_RECV_ITEM_SAFEBOX)
				CASESEND(CHAT_PACKET_CANNOT_DO_NOW)
		default:
			sys_err("unknown chat packet %d", type);
			break;
		}
	}

#pragma pack(1)
	class CShopItem
	{
	public:
		CShopItem(DWORD dwID);
		CShopItem(const CShopItem& rCopy);
		CShopItem(LPITEM pItem, const TPriceInfo& sPrice, BYTE byWindowType, DWORD dwID = 0);

		~CShopItem();

		bool			GetTable(TItemTable** ppTable) const;
		TPriceInfo* GetPrice() const;
		LPITEM			CreateItem() const;
		TItemInfoEx* GetInfo() const;
		void			SetInfo(LPITEM pItem);
		void			SetInfo(const TItemInfoEx& info);
		void			SetPrice(const TPriceInfo& sPrice);
		void			SetWindow(BYTE byWin);
		BYTE			GetWindow() const;
		DWORD			GetID() const;
		void			SetOwnerID(DWORD dwOwnerID);
		bool			CanBuy(LPCHARACTER ch);
		void operator = (const CShopItem& rItem);

	protected:
		TItemInfoEx		m_itemInfo;
		TPriceInfo		m_priceInfo;
		BYTE			m_byWindow;
		DWORD			m_dwID;
		DWORD			m_dwOwnerID;
	};

#pragma pack()
	class CShop
	{
	public:
		typedef std::vector<CShopItem>  VECSHOPITEM;
#ifdef ENABLE_SHOP_GUESTLIST_FIX
		typedef std::list<DWORD>		LISTGUEST;
#else
		typedef std::list<LPCHARACTER>	LISTGUEST;
#endif

	public:
		CShop();
		CShop(const CShop& rCopy);
		~CShop();

		VECSHOPITEM* GetItems() const;
		LISTGUEST* GetGuests() const;

		void				SetDuration(DWORD dwDuration);
		DWORD				DecreaseDuration();

		DWORD				GetDuration() const;

		void				SetOwnerPID(DWORD dwOwnerPID);
		DWORD				GetOwnerPID() const;

		bool				AddGuest(LPCHARACTER ch);
		bool				RemoveGuest(LPCHARACTER ch);

		void				SetItems(VECSHOPITEM* pVec);
		bool				AddItem(CShopItem& rItem);
		bool				RemoveItem(DWORD dwItemID);
		bool				ModifyItem(DWORD dwItemID, CShopItem& rItem);
		bool				BuyItem(DWORD dwItem);
		bool				GetItem(DWORD dwItem, CShopItem** ppItem);

		LPCHARACTER			FindOwnerCharacter();
		void				Clear();

		const char* GetName() const;
		void				SetName(const char* pcszName);

		void				RefreshToOwner();
#ifdef ENABLE_SHOP_DECORATION
		void				SetRace(DWORD dwRace);
		DWORD				GetRace() const;
#endif
#ifdef ENABLE_IRA_REWORK
		void				SetPosInfo(TShopPosition& pos);
#endif

#ifdef ENABLE_PREMIUM_MEMBER_SYSTEM
		TShopPosition		GetShopPositions();
#endif

	private:
		void				__RefreshItems(LPCHARACTER ch = NULL);
	private:
		VECSHOPITEM			m_vecItems;
		LISTGUEST			m_listGuests;

		DWORD				m_dwPID;
		DWORD				m_dwDuration;
		std::string			m_stName;
#ifdef ENABLE_IRA_REWORK
		TShopPosition		m_posInfo;
#endif
#ifdef ENABLE_SHOP_DECORATION
		DWORD				m_Race;
#endif
	};

	class CShopSafebox
	{
	public:
		typedef std::vector<CShopItem> VECITEM;
	public:
		CShopSafebox(LPCHARACTER chOwner);
		CShopSafebox();
		CShopSafebox(const CShopSafebox& rCopy);
		~CShopSafebox();

		void			SetOwner(LPCHARACTER ch);
		void			SetItems(VECITEM* pVec);
		void			SetValuteAmount(unsigned long long val);

		bool			AddItem(CShopItem* pItem);
		bool			RemoveItem(DWORD dwItemID);
		bool			RemoveItemControl(DWORD dwItemID);

		void			AddValute(unsigned long long val);
		bool			RemoveValute(unsigned long long val);

		VECITEM* GetItems();
		unsigned long long	GetValutes();

		bool			GetItem(DWORD dwItemID, CShopItem** ppItem);
		LPCHARACTER		GetOwner();

		bool			RefreshToOwner(LPCHARACTER ch = nullptr);

	private:
		VECITEM			m_vecItems;
		LPCHARACTER		m_pkOwner;
		unsigned long long	m_safebox_gold;
	};

#ifdef ENABLE_SHOPS_IN_CITIES
	class ShopEntity : public CEntity
	{
	public:
		ShopEntity();
		~ShopEntity() {  }

		void	EncodeInsertPacket(LPENTITY entity);
		void	EncodeRemovePacket(LPENTITY entity);

		const char* GetShopName() const;
		void				SetShopName(const char* name);
#ifdef ENABLE_SHOP_DECORATION
		void				SetShopRace(DWORD dwRace);
		DWORD				GetShopRace();
#endif
		int					GetShopType();
		void				SetShopType(int iType);
		void				SetShop(offlineshop::CShop* pOfflineShop);

		offlineshop::CShop* GetShop() { return m_pkShop; }

		static DWORD		AllocID()
		{
			static DWORD dwID = 1;
			return dwID++;
		}

		DWORD				GetVID() { return m_dwVID; }
		void  Destroy() { CEntity::Destroy(); }

	private:
		char					m_szName[OFFLINE_SHOP_NAME_MAX_LEN];
#ifdef ENABLE_SHOP_DECORATION
		DWORD					m_Race;
#endif
		int						m_iType;
		DWORD					m_dwVID;
		offlineshop::CShop* m_pkShop;
	};
#endif
}
#endif
#endif