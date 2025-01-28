#ifndef __INC_METIN_II_GAME_SAFEBOX_H__
#define __INC_METIN_II_GAME_SAFEBOX_H__

class CHARACTER;
class CItem;
class CGrid;

class CSafebox
{
public:
	CSafebox(LPCHARACTER pkChrOwner, int iSize, DWORD dwGold);
	~CSafebox();

	bool		Add(DWORD dwPos, LPITEM pkItem);
	LPITEM		Get(DWORD dwPos);
	LPITEM		Remove(DWORD dwPos);
	void		ChangeSize(int iSize);

#ifdef ENABLE_EXTENDED_COUNT_ITEMS
	bool		MoveItem(CELL_UINT bCell, CELL_UINT bDestCell, MAX_COUNT count);
#else
	bool		MoveItem(CELL_UINT bCell, CELL_UINT bDestCell, BYTE count);
#endif
	LPITEM		GetItem(CELL_UINT bCell);

	void		Save();

	bool		IsEmpty(DWORD dwPos, BYTE bSize);
	bool		IsValidPosition(DWORD dwPos);

	void		SetWindowMode(BYTE bWindowMode);

protected:
	void		__Destroy();

	LPCHARACTER	m_pkChrOwner;
	LPITEM		m_pkItems[SAFEBOX_MAX_NUM];
	CGrid* m_pkGrid;
	int		m_iSize;
	long		m_lGold;

	BYTE		m_bWindowMode;
};

#endif
