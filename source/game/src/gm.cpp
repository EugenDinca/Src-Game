#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "locale_service.h"
#include "config.h"

//ADMIN_MANAGER
std::set<std::string> g_set_Host;
std::map<std::string, tGM> g_map_GM;

void gm_new_clear()
{
	g_set_Host.clear();
	g_map_GM.clear();
}

void gm_new_insert(const tAdminInfo& rAdminInfo)
{
	tGM t;

	if (strlen(rAdminInfo.m_szContactIP) == 0)
	{
		t.pset_Host = &g_set_Host;
	}
	else
	{
		t.pset_Host = NULL;
	}

	memcpy(&t.Info, &rAdminInfo, sizeof(rAdminInfo));

	g_map_GM[rAdminInfo.m_szName] = t;
}

void gm_new_host_inert(const char* host)
{
	g_set_Host.insert(host);
}

BYTE gm_new_get_level(const char* name, const char* host, const char* account)
{
	if (test_server) return GM_IMPLEMENTOR;

	std::map<std::string, tGM >::iterator it = g_map_GM.find(name);

	if (g_map_GM.end() == it)
		return GM_PLAYER;

	// GERMAN_GM_NOT_CHECK_HOST

#ifdef ENABLE_NEWSTUFF
	if (!g_bGMHostCheck)
#else
	if (true)
#endif
	{
		if (account)
		{
			if (strcmp(it->second.Info.m_szAccount, account) != 0)
			{
				return GM_PLAYER;
			}
		}
		return it->second.Info.m_Authority;
	}
	// END_OF_GERMAN_GM_NOT_CHECK_HOST
	else
	{
		if (host)
		{
			if (it->second.pset_Host)
			{
				if (it->second.pset_Host->end() == it->second.pset_Host->find(host))
				{
					return GM_PLAYER;
				}
			}
			else
			{
				if (strcmp(it->second.Info.m_szContactIP, host) != 0)
				{
					return GM_PLAYER;
				}
			}
		}

		return it->second.Info.m_Authority;
	}
	return GM_PLAYER;
}

//END_ADMIN_MANAGER
BYTE gm_get_level(const char* name, const char* host, const char* account)
{
	return gm_new_get_level(name, host, account);
}