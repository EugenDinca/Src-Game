#ifndef __INCLUDE_HEADER_OFFLINESHOP_MANAGER__
#define __INCLUDE_HEADER_OFFLINESHOP_MANAGER__

#ifdef ENABLE_OFFLINE_SHOP
#define SUBTYPE_NOSET 255
#define OFFLINESHOP_DURATION_UPDATE_TIME PASSES_PER_SEC(60)
namespace offlineshop
{
	class CShopManager : public singleton<CShopManager>
	{
	public:
#ifdef ENABLE_SHOPS_IN_CITIES
		typedef std::map<DWORD, ShopEntity*> SHOPENTITIES_MAP;
		typedef struct SCityShopInfo {
			SHOPENTITIES_MAP	entitiesByPID;
			SHOPENTITIES_MAP	entitiesByVID;

			void Clear()
			{
				entitiesByPID.clear();
				entitiesByVID.clear();
			}

			SCityShopInfo()
			{
				Clear();
			}

			SCityShopInfo(const SCityShopInfo& rCopy)
			{
				CopyContainer(entitiesByPID, rCopy.entitiesByPID);
				CopyContainer(entitiesByVID, rCopy.entitiesByVID);
			}
		} TCityShopInfo;
#endif

		typedef std::map<DWORD, CShop>					 SHOPMAP;
		typedef std::map<DWORD, CShopSafebox>			 SAFEBOXMAP;
		typedef std::map<DWORD, DWORD>					 SEARCHTIMEMAP;

#ifdef ENABLE_SHOPS_IN_CITIES
		typedef std::vector<TCityShopInfo>				 CITIESVEC;
#endif

		CShopManager();

#ifdef ENABLE_IRA_REWORK
		offlineshop::CShop* PutsNewShop(TShopInfo* pInfo, TShopPosition* pPosInfo);
#else
		offlineshop::CShop* PutsNewShop(TShopInfo* pInfo);
#endif
		offlineshop::CShop* GetShopByOwnerID(DWORD dwPID);
		CShopSafebox* GetShopSafeboxByOwnerID(DWORD dwPID);

		void					RemoveSafeboxFromCache(DWORD dwOwnerID);
		void					RemoveGuestFromShops(LPCHARACTER ch);

#ifdef ENABLE_SHOPS_IN_CITIES
	public:
#ifdef ENABLE_IRA_REWORK
		void		CreateNewShopEntities(offlineshop::CShop& rShop, TShopPosition& pos);
#else
		void		CreateNewShopEntities(offlineshop::CShop& rShop);
#endif
		void		DestroyNewShopEntities(const offlineshop::CShop& rShop);

		void		EncodeInsertShopEntity(ShopEntity& shop, LPCHARACTER ch);
		void		EncodeRemoveShopEntity(ShopEntity& shop, LPCHARACTER ch);

	private:
#ifndef ENABLE_IRA_REWORK
		bool		__CanUseCity(size_t index);
		bool		__CheckEntitySpawnPos(const long x, const long y, const TCityShopInfo& city);
#endif
		void		__UpdateEntity(const offlineshop::CShop& rShop);
#endif

	public:
		/*db*/	void		SendShopBuyDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID);
		/*db*/	void		SendShopLockBuyItemDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID);

		/*db*/	bool		RecvShopLockedBuyItemDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID);
		/*db*/	bool		RecvShopBuyDBPacket(DWORD dwBuyerID, DWORD dwOwnerID, DWORD dwItemID);
		/*db*/	void		SendShopCannotBuyLockedItemDBPacket(DWORD dwOwnerID, DWORD dwItemID);

		/*db*/	void		SendShopEditItemDBPacket(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& rPrice, bool allEdit);
		/*db*/	bool		RecvShopEditItemDBPacket(DWORD dwOwnerID, DWORD dwItemID, const TPriceInfo& rPrice);

		/*db*/	void		SendShopRemoveItemDBPacket(DWORD dwOwnerID, DWORD dwItemID);
		/*db*/	bool		RecvShopRemoveItemDBPacket(DWORD dwOwnerID, DWORD dwItemID);

		/*db*/	void		SendShopAddItemDBPacket(DWORD dwOwnerID, const TItemInfo& rItemInfo);
		/*db*/	bool		RecvShopAddItemDBPacket(DWORD dwOwnerID, const TItemInfo& rItemInfo);

		/*db*/	void		SendShopForceCloseDBPacket(DWORD dwPID);
		/*db*/	bool		RecvShopForceCloseDBPacket(DWORD dwPID);
		/*db*/	bool		RecvShopExpiredDBPacket(DWORD dwPID);

#ifdef ENABLE_IRA_REWORK
		/*db*/	void		SendShopCreateNewDBPacket(const TShopInfo&, const TShopPosition& pos, std::vector<TItemInfo>& vec);
		/*db*/	bool		RecvShopCreateNewDBPacket(const TShopInfo&, TShopPosition& pos, std::vector<TItemInfo>& vec);
#else
		/*db*/	void		SendShopCreateNewDBPacket(const TShopInfo&, std::vector<TItemInfo>& vec);
		/*db*/	bool		RecvShopCreateNewDBPacket(const TShopInfo&, std::vector<TItemInfo>& vec);
#endif

		/*db*/	void		SendShopChangeNameDBPacket(DWORD dwOwnerID, const char* szName);
		/*db*/	bool		RecvShopChangeNameDBPacket(DWORD dwOwnerID, const char* szName);

#ifdef ENABLE_OFFLINESHOP_EXTEND_TIME
				bool		RecvShopExtendTimeClientPacket(LPCHARACTER ch, DWORD dwTime);
				void		SendShopExtendTimeDBPacket(DWORD dwOwnerID, DWORD dwTime);
				bool		RecvShopExtendTimeDBPacket(DWORD dwOwnerID, DWORD dwTime);
#endif

//offlineshop-updated 05/08/19

		/*db*/	void		SendShopSafeboxGetItemDBPacket(DWORD dwOwnerID, DWORD dwItemID);
		/*db*/	void		SendShopSafeboxGetValutesDBPacket(DWORD dwOwnerID, const unsigned long long& valutes);
		/*db*/  bool		SendShopSafeboxAddItemDBPacket(DWORD dwOwnerID, const CShopItem& item);
		/*db*/	bool		RecvShopSafeboxAddItemDBPacket(DWORD dwOwnerID, DWORD dwItemID, const TItemInfoEx& item);
		/*db*/	bool		RecvShopSafeboxAddValutesDBPacket(DWORD dwOwnerID, const unsigned long long& valute);
		/*db*/	bool		RecvShopSafeboxLoadDBPacket(DWORD dwOwnerID, const unsigned long long& valute, const std::vector<DWORD>& ids, const std::vector<TItemInfoEx>& items);
		/*db*/  bool		RecvShopSafeboxExpiredItemDBPacket(DWORD dwOwnerID, DWORD dwItemID);

		/*cli.*/bool		RecvShopCreateNewClientPacket(LPCHARACTER ch, TShopInfo& rShopInfo, std::vector<TShopItemInfo>& vec);
		/*cli.*/bool		RecvShopChangeNameClientPacket(LPCHARACTER ch, const char* szName);
		/*cli.*/bool		RecvShopForceCloseClientPacket(LPCHARACTER ch);
		/*cli.*/bool		RecvShopRequestListClientPacket(LPCHARACTER ch);
		/*cli.*/bool		RecvShopOpenClientPacket(LPCHARACTER ch, DWORD dwOwnerID);
		/*cli.*/bool		RecvShopOpenMyShopClientPacket(LPCHARACTER ch);
		/*cli.*/bool		RecvShopBuyItemClientPacket(LPCHARACTER ch, DWORD dwOwnerID, DWORD dwItemID, bool isSearch);
#ifdef	ENABLE_OFFLINESHOP_NOTIFICATION
		void				SendNotificationClientPacket(LPDESC d, DWORD dwItemID, long long dwItemPrice, WORD dwItemCount);
#endif
#ifdef ENABLE_SHOPS_IN_CITIES
		/*cli.*/bool		RecvShopClickEntity(LPCHARACTER ch, DWORD dwShopEntityVID);
#endif

		/*cli.*/void		SendShopListClientPacket(LPCHARACTER ch);
		/*cli.*/void		SendShopOpenClientPacket(LPCHARACTER ch, CShop* pkShop);
		/*cli.*/void		SendShopOpenMyShopClientPacket(LPCHARACTER ch);
		/*cli.*/void		SendShopOpenMyShopNoShopClientPacket(LPCHARACTER ch);
		/*cli.*/void		SendShopBuyItemFromSearchClientPacket(LPCHARACTER ch, DWORD dwOwnerID, DWORD dwItemID);

		/*cli.*/void		SendShopForceClosedClientPacket(DWORD dwOwnerID);

		/*cli.*/bool		RecvShopAddItemClientPacket(LPCHARACTER ch, const TItemPos& item, const TPriceInfo& price
#ifdef ENABLE_OFFLINE_SHOP_GRID
								,const TItemDisplayPos& display_pos
#endif
								);

		/*cli.*/bool		RecvShopRemoveItemClientPacket(LPCHARACTER ch, DWORD dwItemID);
		/*cli.*/bool		RecvShopEditItemClientPacket(LPCHARACTER ch, DWORD dwItemID, const TPriceInfo& price, bool allEdit);

		/*cli.*/bool		RecvShopFilterRequestClientPacket(LPCHARACTER ch, const TFilterInfo& filter);
		/*cli.*/void		SendShopFilterResultClientPacket(LPCHARACTER ch, const std::vector<TItemInfo>& items);

		/*cli.*/bool		RecvShopSafeboxOpenClientPacket(LPCHARACTER ch);
		/*cli.*/bool		RecvShopSafeboxGetItemClientPacket(LPCHARACTER ch, DWORD dwItemID);
		/*cli.*/bool		RecvShopSafeboxGetValutesClientPacket(LPCHARACTER ch, const unsigned long long& valutes);
		/*cli.*/bool		RecvShopSafeboxCloseClientPacket(LPCHARACTER ch);

		/*cli.*/void		SendShopSafeboxRefresh(LPCHARACTER ch, const unsigned long long& valute, const std::vector<CShopItem>& vec);

		/*cli.*/void		RecvCloseBoardClientPacket(LPCHARACTER ch);

		void		UpdateShopsDuration();

		void		ClearSearchTimeMap();
		bool		CheckSearchTime(DWORD dwPID);

		void		Destroy();
#ifdef ENABLE_IRA_REWORK
		int			GetMapIndexAllowsList(int iMapIndex);
#endif
#ifdef ENABLE_OFFLINE_SHOP_LOG
		void		OfflineShopLogs(DWORD ownerid, const char* log_type, const char* log);
		void		SafeboxRequestGoldLogs(DWORD ownerid, long long oldgold, long long newgold, unsigned long long requestgold);
		void		SafeboxRequestItemLogs(DWORD ownerid, DWORD itemid, DWORD itemvnum);
#endif
#ifdef ENABLE_AVERAGE_PRICE
		void		AveragePriceUpdate();
		void		RecvAveragePrice(LPCHARACTER ch, DWORD vnum);
		long long	GetAveragePrice(DWORD vnum);
#endif
	private:
		SHOPMAP			m_mapShops;
		SAFEBOXMAP		m_mapSafeboxs;

		LPEVENT			m_eventShopDuration;
		SEARCHTIMEMAP	m_searchTimeMap;

#ifdef ENABLE_SHOPS_IN_CITIES
		CITIESVEC		m_vecCities;
#endif
#ifdef ENABLE_IRA_REWORK
		std::set<int> s_set_offlineshop_map_allows;
#endif
		std::unordered_map<DWORD, long long> m_mapAveragePrice;

		const std::map<BYTE, BYTE> acceGradeMap{
			{1, 1},
			{2, 5},
			{3, 10},
			{4, 11},
		};

	};

	offlineshop::CShopManager& GetManager();

}

#endif
#endif