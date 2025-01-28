#pragma once
#include "stdafx.h"
#ifdef ENABLE_ALIGNMENT_SYSTEM
class CAlignmentProto : public singleton<CAlignmentProto>
{
public:
	CAlignmentProto();
	~CAlignmentProto();
	bool Initialize();
	using ALIGN_MAP = std::unordered_map<BYTE, TAlignmentTable>;

	BYTE GetAlignmentLevel(int alignPoint);
	void GetBonus(BYTE level, std::vector<std::pair<BYTE, long long>>& bonus);
	bool IsAlignmentUpdate(int alignPoint, BYTE level);
private:
	ALIGN_MAP m_alignMap;
};
#endif
