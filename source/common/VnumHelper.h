#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__

class CItemVnumHelper
{
public:

	static	const bool	IsPhoenix(DWORD vnum)				{ return 53001 == vnum; }
	static	const bool	IsRamadanMoonRing(DWORD vnum)		{ return 71135 == vnum; }
	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }
	static	const bool	IsHappinessRing(DWORD vnum)		{ return 71143 == vnum; }
	static	const bool	IsLovePendant(DWORD vnum)		{ return 71145 == vnum; }

#if defined(ENABLE_EXTENDED_BLEND_AFFECT)
	/// Extended Blend

	static const bool IsExtendedBlend(DWORD vnum)
	{
		switch (vnum)
		{
			
		case 950821:
		case 950822:
		case 950823:
		case 950824:
		case 950825:
		case 950826:

		case 950827:
		case 950828:
		case 950829:

		case 951002: 

		case 939017:
		case 939018:
		case 939019:
		case 939020:

		case 939024:
		case 939025:

		case 927209:
		case 927212:

			return true;

		default:
			return false;
		}
	}
#endif

};

class CMobVnumHelper
{
public:
	static	bool	IsPhoenix(DWORD vnum)				{ return 34001 == vnum; }
	static	bool	IsIcePhoenix(DWORD vnum)				{ return 34003 == vnum; }
	static	bool	IsPetUsingPetSystem(DWORD vnum)	{ return (IsPhoenix(vnum) || IsReindeerYoung(vnum)) || IsIcePhoenix(vnum); }
	static	bool	IsReindeerYoung(DWORD vnum)	{ return 34002 == vnum; }
	static	bool	IsRamadanBlackHorse(DWORD vnum)		{ return 20119 == vnum || 20219 == vnum || 22022 == vnum; }
};

class CVnumHelper
{
};


#endif	//__HEADER_VNUM_HELPER__

