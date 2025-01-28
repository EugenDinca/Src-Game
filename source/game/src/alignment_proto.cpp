#include "stdafx.h"
#ifdef ENABLE_ALIGNMENT_SYSTEM
#include <fstream>
#include <rapidjson/json.hpp>
#include "locale_service.h"
#include "alignment_proto.h"
#include "constants.h"
#include "char.h"
#include "config.h"
CAlignmentProto::CAlignmentProto()
{
	m_alignMap.clear();
}

CAlignmentProto::~CAlignmentProto()
{
	m_alignMap.clear();
}

bool CAlignmentProto::Initialize()
{
	using js = nlohmann::json;
	char file_name[256 + 1];
	snprintf(file_name, sizeof(file_name), "%s/alignment_proto.json", LocaleService_GetBasePath().c_str());
	std::ifstream ifs(file_name);
	if (!ifs.is_open()) { return false; }

	try
	{
		sys_log(0,"----Alignment Proto Read Start------");
		js jf = js::parse(ifs);
		js& arr = jf["alignment"];
		for (const auto& i : arr)
		{
			BYTE level = 0;
			TAlignmentTable info = {};

			i.at("level").get_to(level);
			i.at("min_point").get_to(info.minPoint);
			i.at("max_point").get_to(info.maxPoint);
			sys_log(0, "Alignment Level : %d, Min_Point : %d, Max_Point : %d", level, info.minPoint, info.maxPoint);

			for (const auto& y : i["bonus"])
			{
				std::string str_applyType;
				WORD pointValue;
				y.at("type").get_to(str_applyType);
				y.at("value").get_to(pointValue);

				std::pair<BYTE, BYTE> bonus = StringApplyToPoint(str_applyType.c_str());
				const BYTE pointType = bonus.second;
				if (pointType == POINT_NONE)
				{
					sys_err("Unknow apply type : %s", str_applyType.c_str());
					break;
				}

				info.bonus.push_back(std::make_pair(pointType, pointValue));
				sys_log(0, "Alignment Bonus Type: %s Value: %d Point Type: %d ", str_applyType.c_str(), pointValue, pointType);
			}
			m_alignMap.insert(std::make_pair(level, info));
		}
		ifs.close();

		itertype(m_alignMap) it = m_alignMap.find(m_alignMap.size());
		if (it == m_alignMap.end()) { return false; }
		g_maxAlignment = it->second.maxPoint - 1;
		it->second.maxPoint += 1;
		sys_log(0,"Maximun alignment max point %d", g_maxAlignment);

		sys_log(0, "----Alignment Proto Read End------");
	}
	catch (const std::exception& e) 
	{
		sys_err("ReadAlignmentProto error : %s - file : %s", e.what(), file_name);
		return false;
	}
	return true;
}

BYTE CAlignmentProto::GetAlignmentLevel(int alignPoint)
{
	for (itertype(m_alignMap)it = m_alignMap.begin(); it != m_alignMap.end(); ++it)
	{
		if (alignPoint >= it->second.minPoint && alignPoint < it->second.maxPoint)
		{
			return it->first;
		}
	}
	return 0;
}

void CAlignmentProto::GetBonus(BYTE level, std::vector<std::pair<BYTE, long long>>& bonus)
{
	bonus.clear();
	itertype(m_alignMap)it = m_alignMap.find(level);

	if (it != m_alignMap.end())
	{
		bonus = it->second.bonus;
	}
}

bool CAlignmentProto::IsAlignmentUpdate(int alignPoint, BYTE level)
{
	itertype(m_alignMap)it = m_alignMap.find(level);
	if (it != m_alignMap.end())
	{
		if (alignPoint < it->second.minPoint || alignPoint > it->second.maxPoint)
			return true;
	}
	return false;
}
#endif