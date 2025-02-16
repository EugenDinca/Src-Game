#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "locale_service.h"

#include "desc.h"

ACMD(do_user_horse_ride);
ACMD(do_user_horse_back);
ACMD(do_user_horse_feed);

// ADD_COMMAND_SLOW_STUN
ACMD(do_slow);
ACMD(do_stun);
// END_OF_ADD_COMMAND_SLOW_STUN

ACMD(do_warp);
ACMD(do_goto);
ACMD(do_item);
ACMD(do_mob);
ACMD(do_mob_ld);
ACMD(do_mob_aggresive);
ACMD(do_mob_coward);
ACMD(do_mob_map);
ACMD(do_purge);
ACMD(do_weaken);
ACMD(do_item_purge);
ACMD(do_state);
ACMD(do_notice);
ACMD(do_map_notice);
ACMD(do_big_notice);
#ifdef ENABLE_FULL_NOTICE
ACMD(do_notice_test);
ACMD(do_big_notice_test);
ACMD(do_map_big_notice);
#endif
ACMD(do_who);
ACMD(do_user);
ACMD(do_disconnect);
ACMD(do_kill);
ACMD(do_emotion_allow);
ACMD(do_emotion);
ACMD(do_transfer);
ACMD(do_set);
ACMD(do_cmd);
ACMD(do_reset);
ACMD(do_greset);
ACMD(do_mount);
ACMD(do_fishing);
ACMD(do_refine_rod);
#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
ACMD(do_ruhoku);
#endif
#ifdef ENABLE_PASSIVE_SKILLS
ACMD(do_passiveread);
#endif
// REFINE_PICK
ACMD(do_max_pick);
ACMD(do_refine_pick);
// END_OF_REFINE_PICK

ACMD(do_console);
ACMD(do_restart);
ACMD(do_advance);
ACMD(do_stat);
ACMD(do_respawn);
ACMD(do_skillup);
ACMD(do_guildskillup);
ACMD(do_pvp);
ACMD(do_point_reset);
ACMD(do_safebox_size);
ACMD(do_safebox_close);
ACMD(do_safebox_password);
ACMD(do_safebox_change_password);
ACMD(do_mall_password);
ACMD(do_mall_close);
ACMD(do_ungroup);
ACMD(do_makeguild);
ACMD(do_deleteguild);
ACMD(do_shutdown);
ACMD(do_group);
ACMD(do_group_random);
ACMD(do_invisibility);
ACMD(do_event_flag);
ACMD(do_get_event_flag);
ACMD(do_private);
ACMD(do_qf);
ACMD(do_clear_quest);
ACMD(do_book);
ACMD(do_reload);
ACMD(do_war);
ACMD(do_nowar);
ACMD(do_setskill);
ACMD(do_setskillother);
ACMD(do_level);
ACMD(do_polymorph);
ACMD(do_polymorph_item);

ACMD(do_close_shop);
ACMD(do_set_walk_mode);
ACMD(do_set_run_mode);
ACMD(do_set_skill_group);
ACMD(do_set_skill_point);
ACMD(do_cooltime);

ACMD(do_gwlist);
ACMD(do_stop_guild_war);
ACMD(do_cancel_guild_war);
ACMD(do_guild_state);

ACMD(do_pkmode);
ACMD(do_messenger_auth);

ACMD(do_getqf);
ACMD(do_setqf);
ACMD(do_delqf);
ACMD(do_set_state);

ACMD(do_forgetme);
ACMD(do_aggregate);
ACMD(do_attract_ranger);
ACMD(do_pull_monster);
ACMD(do_setblockmode);
ACMD(do_priv_empire);
ACMD(do_priv_guild);
ACMD(do_mount_test);
ACMD(do_unmount);
ACMD(do_observer);
ACMD(do_observer_exit);
ACMD(do_socket_item);
ACMD(do_stat_minus);
ACMD(do_stat_reset);
ACMD(do_view_equip);
ACMD(do_block_chat);
ACMD(do_vote_block_chat);

// BLOCK_CHAT
ACMD(do_block_chat_list);
// END_OF_BLOCK_CHAT

ACMD(do_party_request);
ACMD(do_party_request_deny);
ACMD(do_party_request_accept);
ACMD(do_build);
ACMD(do_clear_land);

ACMD(do_horse_state);
ACMD(do_horse_level);
ACMD(do_horse_ride);
ACMD(do_horse_summon);
ACMD(do_horse_unsummon);
ACMD(do_horse_set_stat);

ACMD(do_save_attribute_to_image);
ACMD(do_affect_remove);

ACMD(do_change_attr);
ACMD(do_add_attr);
ACMD(do_add_socket);

ACMD(do_inputall)
{
	ch->NewChatPacket(CMD_TALK_2);
}

ACMD(do_show_arena_list);
ACMD(do_end_all_duel);
ACMD(do_end_duel);
ACMD(do_duel);

ACMD(do_stat_plus_amount);

ACMD(do_break_marriage);

ACMD(do_oxevent_show_quiz);
ACMD(do_oxevent_get_attender);

ACMD(do_effect);

//gift notify quest command
ACMD(do_gift);

ACMD(do_inventory);
ACMD(do_cube);

ACMD(do_reset_subskill);
ACMD(do_flush);

ACMD(do_weeklyevent);

ACMD(do_in_game_mall);

ACMD(do_get_mob_count);

ACMD(do_dice);
ACMD(do_special_item);

ACMD(do_click_mall);

ACMD(do_ride);
ACMD(do_get_item_id_list);
ACMD(do_set_socket);

ACMD(do_costume);
ACMD(do_set_stat);

ACMD(do_can_dead);

ACMD(do_full_set);

ACMD(do_item_full_set);

ACMD(do_attr_full_set);

ACMD(do_all_skill_master);

ACMD(do_use_item);
ACMD(do_dragon_soul);
ACMD(do_ds_list);
ACMD(do_clear_affect);

#ifdef ENABLE_NEWSTUFF
ACMD(do_change_rare_attr);
ACMD(do_add_rare_attr);

ACMD(do_click_safebox);
ACMD(do_force_logout);

ACMD(do_poison);
ACMD(do_rewarp);
#endif
#ifdef ENABLE_WOLFMAN_CHARACTER
ACMD(do_bleeding);
#endif
#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
ACMD(do_change_channel);
#endif
#ifdef ENABLE_AFFECT_BUFF_REMOVE
ACMD(do_remove_buff);
#endif
#ifdef ENABLE_SPLIT_ITEMS_FAST
ACMD(do_split_selected_item);
#ifdef ENABLE_SPECIAL_STORAGE
ACMD(do_split_storage_item);
#endif
#endif
#ifdef ENABLE_SORT_INVENTORIES
ACMD(do_sort_items);
ACMD(do_sort_special_storage);
#endif
#ifdef ENABLE_OFFLINE_SHOP
ACMD(do_offshop_change_shop_name);
ACMD(do_offshop_force_close_shop);
#endif
#ifdef ENABLE_GLOBAL_RANKING
ACMD(do_request_ranking_list);
#endif
#ifdef ENABLE_CHAR_SETTINGS
ACMD(do_SetCharSettings);
#endif
#ifdef ENABLE_FISHING_RENEWAL
ACMD(do_fishingrenewal);
#endif
#ifdef ENABLE_DISTANCE_NPC
ACMD(do_distance_npc);
#endif
#ifdef ENABLE_HWID
ACMD(do_hwidban);
ACMD(do_ban);
#endif
#ifdef ENABLE_FARM_BLOCK
ACMD(do_set_farm_block);
#endif
#ifdef ENABLE_EVENT_MANAGER
ACMD(do_event_manager);
#endif
#ifdef ENABLE_ITEMSHOP
ACMD(do_ishop);
#endif

#ifdef ENABLE_NAMING_SCROLL
ACMD(do_namingscroll);
#endif

#ifdef ENABLE_MINING_RENEWAL
ACMD(do_miningrenewal);
#endif

#ifdef ENABLE_SPECIAL_STORAGE
ACMD(do_transfer_inv_storage);
ACMD(do_transfer_storage_inv);
#endif

#ifdef ENABLE_GET_TOTAL_ONLINE_COUNT
ACMD(do_get_total_online);
#endif
#ifdef ENABLE_MAINTENANCE_SYSTEM
ACMD(do_update_maintenance);
#endif

#ifdef ENABLE_VOTE4BUFF
ACMD(do_vote_bonus);
#endif

#ifdef ENABLE_EXTENDED_BATTLE_PASS
ACMD(do_battlepass_get_info);
ACMD(do_battlepass_set_mission);
ACMD(do_battlepass_premium_activate);
#endif
#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
ACMD(do_read_books_fast);
#endif
#ifdef ENABLE_CHAT_COLOR_SYSTEM
ACMD(do_set_chat_color);
#endif
#ifdef ENABLE_BOT_CONTROL
ACMD(do_bot_control);
ACMD(do_bot_control_open);
#endif

#ifdef ENABLE_DISTANCE_SKILL_SELECT
ACMD(do_skill_select);
#endif

#ifdef __SKILL_TRAINING__
ACMD(do_skill_training);
#endif

ACMD(do_new_full);

#ifdef ENABLE_FURKANA_GOTTEN
ACMD(do_drop_calc);
#endif

#if defined(__GAME_OPTION_ESCAPE__)
ACMD(do_escape);
#endif

#ifdef ENABLE_DAILY_REWARD
ACMD(do_dailyreward_open);
ACMD(do_dailyreward_process);
#endif

#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
ACMD(do_block_player);
ACMD(do_unblock_player);
#endif

#ifdef __STONE_SCROLL__
ACMD(do_stone_scroll);
#endif

struct command_info cmd_info[] =
{
	{ "!RESERVED!",	NULL,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "who",		do_who,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "war",		do_war,			0,			POS_DEAD,	GM_PLAYER	},
	{ "warp",		do_warp,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "user",		do_user,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "notice",		do_notice,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "notice_map",	do_map_notice,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "big_notice",	do_big_notice,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
#ifdef ENABLE_FULL_NOTICE
	{ "big_notice_map",	do_map_big_notice,	0,	POS_DEAD,	GM_HIGH_WIZARD	},
	{ "notice_test",	do_notice_test,		0,	POS_DEAD,	GM_HIGH_WIZARD	},
	{ "big_notice_test",do_big_notice_test,	0,	POS_DEAD,	GM_HIGH_WIZARD	},
#endif
	{ "nowar",		do_nowar,		0,			POS_DEAD,	GM_PLAYER	},
	{ "purge",		do_purge,		0,			POS_DEAD,	GM_WIZARD	},
	{ "weaken",		do_weaken,		0,			POS_DEAD,	GM_GOD		},
	{ "dc",		do_disconnect,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "transfer",	do_transfer,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "goto",		do_goto,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "level",		do_level,		0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "eventflag",	do_event_flag,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "geteventflag",	do_get_event_flag,	0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "item",		do_item,		0,			POS_DEAD,	GM_IMPLEMENTOR		},

	{ "mob",		do_mob,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "mob_ld",		do_mob_ld,			0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "ma",		do_mob_aggresive,	0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "mc",		do_mob_coward,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "mm",		do_mob_map,		0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "kill",		do_kill,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "ipurge",		do_item_purge,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "group",		do_group,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "grrandom",	do_group_random,	0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "set",		do_set,			0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "reset",		do_reset,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "greset",		do_greset,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "advance",	do_advance,		0,			POS_DEAD,	GM_IMPLEMENTOR		},
	{ "book",		do_book,		0,			POS_DEAD,	GM_IMPLEMENTOR  },

	{ "console",	do_console,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "shutdow",	do_inputall,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "shutdown",	do_shutdown,		0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "stat",		do_stat,		0,			POS_DEAD,	GM_PLAYER	},
	{ "stat-",		do_stat_minus,		0,			POS_DEAD,	GM_PLAYER	},
	{ "stat_reset",	do_stat_reset,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "state",		do_state,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	// ADD_COMMAND_SLOW_STUN
	{ "stun",		do_stun,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "slow",		do_slow,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	// END_OF_ADD_COMMAND_SLOW_STUN

	{ "respawn",	do_respawn,		0,			POS_DEAD,	GM_WIZARD	},

	{ "makeguild",	do_makeguild,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "deleteguild",	do_deleteguild,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "mount",		do_mount,		0,			POS_MOUNTING,	GM_PLAYER	},
	{ "restart_here",	do_restart,		SCMD_RESTART_HERE,	POS_DEAD,	GM_PLAYER	},
	{ "restart_town",	do_restart,		SCMD_RESTART_TOWN,	POS_DEAD,	GM_PLAYER	},
	{ "phase_selec",	do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "phase_select",	do_cmd,			SCMD_PHASE_SELECT,	POS_DEAD,	GM_PLAYER	},
	{ "qui",		do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "quit",		do_cmd,			SCMD_QUIT,		POS_DEAD,	GM_PLAYER	},
	{ "logou",		do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "logout",		do_cmd,			SCMD_LOGOUT,		POS_DEAD,	GM_PLAYER	},
	{ "skillup",	do_skillup,		0,			POS_DEAD,	GM_PLAYER	},
	{ "gskillup",	do_guildskillup,	0,			POS_DEAD,	GM_PLAYER	},
	{ "pvp",		do_pvp,			0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox",	do_safebox_size,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "safebox_close",	do_safebox_close,	0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_passwor",do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_password",do_safebox_password,	0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_change_passwor", do_inputall,	0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_change_password", do_safebox_change_password,	0,	POS_DEAD,	GM_PLAYER	},
	{ "mall_passwor",	do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "mall_password",	do_mall_password,	0,			POS_DEAD,	GM_PLAYER	},
	{ "mall_close",	do_mall_close,		0,			POS_DEAD,	GM_PLAYER	},

	// Group Command
	{ "ungroup",	do_ungroup,		0,			POS_DEAD,	GM_PLAYER	},

	// REFINE_ROD_HACK_BUG_FIX
	{ "refine_rod",	do_refine_rod,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	// END_OF_REFINE_ROD_HACK_BUG_FIX

	// REFINE_PICK
	{ "refine_pick",	do_refine_pick,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "max_pick",	do_max_pick,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	// END_OF_REFINE_PICK

	{ "invisible",	do_invisibility,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "qf",		do_qf,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "clear_quest",	do_clear_quest,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "close_shop",	do_close_shop,		0,			POS_DEAD,	GM_PLAYER	},

	{ "set_walk_mode",	do_set_walk_mode,	0,			POS_DEAD,	GM_PLAYER	},
	{ "set_run_mode",	do_set_run_mode,	0,			POS_DEAD,	GM_PLAYER	},
	{ "setjob",do_set_skill_group,	0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "setskill",	do_setskill,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "setskillother",	do_setskillother,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "setskillpoint",  do_set_skill_point,	0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "reload",		do_reload,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "cooltime",	do_cooltime,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "gwlist",		do_gwlist,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "gwstop",		do_stop_guild_war,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "gwcancel",	do_cancel_guild_war, 0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "gstate",		do_guild_state,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "pkmode",		do_pkmode,		0,			POS_DEAD,	GM_PLAYER	},
	{ "messenger_auth",	do_messenger_auth,	0,			POS_DEAD,	GM_PLAYER	},

	{ "getqf",		do_getqf,		0,			POS_DEAD,	GM_IMPLEMENTOR },
	{ "setqf",		do_setqf,		0,			POS_DEAD,	GM_IMPLEMENTOR },

	{ "delqf",		do_delqf,		0,			POS_DEAD,	GM_IMPLEMENTOR },
	{ "set_state",	do_set_state,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "forgetme",	do_forgetme,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "aggregate",	do_aggregate,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "attract_ranger",	do_attract_ranger,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "pull_monster",	do_pull_monster,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "setblockmode",	do_setblockmode,	0,			POS_DEAD,	GM_PLAYER	},
	{ "polymorph",	do_polymorph,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "polyitem",	do_polymorph_item,	0,			POS_DEAD,	GM_HIGH_WIZARD },
	{ "priv_empire",	do_priv_empire,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "priv_guild",	do_priv_guild,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "mount_test",	do_mount_test,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "unmount",	do_unmount,		0,			POS_DEAD,	GM_PLAYER	},
	{ "private",	do_private,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "party_request",	do_party_request,	0,			POS_DEAD,	GM_PLAYER	},
	{ "party_request_accept", do_party_request_accept,0,		POS_DEAD,	GM_PLAYER	},
	{ "party_request_deny", do_party_request_deny,0,			POS_DEAD,	GM_PLAYER	},
	{ "observer",	do_observer,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "observer_exit",	do_observer_exit,	0,			POS_DEAD,	GM_PLAYER	},
	{ "socketitem",	do_socket_item,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "saveati",	do_save_attribute_to_image, 0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "view_equip",	do_view_equip,		0,			POS_DEAD,	GM_PLAYER   	},
	{ "jy",				do_block_chat,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	// BLOCK_CHAT
	{ "vote_block_chat", do_vote_block_chat,		0,			POS_DEAD,	GM_IMPLEMENTOR },
	{ "block_chat",		do_block_chat,		0,			POS_DEAD,	GM_IMPLEMENTOR },
	{ "block_chat_list",do_block_chat_list,	0,			POS_DEAD,	GM_IMPLEMENTOR },
	// END_OF_BLOCK_CHAT

	{ "build",		do_build,		0,		POS_DEAD,	GM_PLAYER	},
	{ "clear_land", do_clear_land,	0,		POS_DEAD,	GM_HIGH_WIZARD	},

	{ "affect_remove",	do_affect_remove,	0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "horse_state",	do_horse_state,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_level",	do_horse_level,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_ride",	do_horse_ride,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_summon",	do_horse_summon,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_unsummon",	do_horse_unsummon,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_set_stat", do_horse_set_stat,	0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "emotion_allow",	do_emotion_allow,	0,			POS_FIGHTING,	GM_PLAYER	},
	{ "kiss",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "slap",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "french_kiss",	do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "clap",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "cheer1",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "cheer2",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},

	// DANCE
	{ "dance1",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance2",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance3",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance4",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance5",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance6",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	// END_OF_DANCE

	{ "congratulation",	do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "forgive",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "angry",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "attractive",	do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "sad",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "shy",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "cheerup",	do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "banter",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "joy",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},

	{ "change_attr",	do_change_attr,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "add_attr",	do_add_attr,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "add_socket",	do_add_socket,		0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "user_horse_ride",	do_user_horse_ride,		0,		POS_FISHING,	GM_PLAYER	},
	{ "user_horse_back",	do_user_horse_back,		0,		POS_FISHING,	GM_PLAYER	},
	{ "user_horse_feed",	do_user_horse_feed,		0,		POS_FISHING,	GM_PLAYER	},

	{ "show_arena_list",	do_show_arena_list,		0,		POS_DEAD,	GM_LOW_WIZARD	},
	{ "end_all_duel",		do_end_all_duel,		0,		POS_DEAD,	GM_LOW_WIZARD	},
	{ "end_duel",			do_end_duel,			0,		POS_DEAD,	GM_LOW_WIZARD	},
	{ "duel",				do_duel,				0,		POS_DEAD,	GM_LOW_WIZARD	},

	{ "con+",			do_stat_plus_amount,	POINT_HT,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "int+",			do_stat_plus_amount,	POINT_IQ,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "str+",			do_stat_plus_amount,	POINT_ST,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "dex+",			do_stat_plus_amount,	POINT_DX,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "break_marriage",	do_break_marriage,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "show_quiz",			do_oxevent_show_quiz,	0,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "get_oxevent_att",	do_oxevent_get_attender,0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "effect",				do_effect,				0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "inventory",			do_inventory,			0,	POS_DEAD,	GM_IMPLEMENTOR },
	{ "cube",				do_cube,				0,	POS_DEAD,	GM_PLAYER	},

	{ "reset_subskill",		do_reset_subskill,		0,	POS_DEAD,	GM_HIGH_WIZARD },
	{ "flush",				do_flush,				0,	POS_DEAD,	GM_IMPLEMENTOR },

	{ "weeklyevent",		do_weeklyevent,			0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "in_game_mall",		do_in_game_mall,		0,	POS_DEAD,	GM_PLAYER	},

	{ "get_mob_count",		do_get_mob_count,		0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "dice",				do_dice,				0,	POS_DEAD,	GM_PLAYER		},
	{ "special_item",			do_special_item,	0,	POS_DEAD,	GM_IMPLEMENTOR		},

	{ "click_mall",			do_click_mall,			0,	POS_DEAD,	GM_PLAYER		},

	{ "ride",				do_ride,				0,	POS_DEAD,	GM_PLAYER	},

	{ "item_id_list",	do_get_item_id_list,	0,	POS_DEAD,	GM_IMPLEMENTOR },
	{ "set_socket",		do_set_socket,			0,	POS_DEAD,	GM_IMPLEMENTOR },

	{ "tcon",			do_set_stat,	POINT_HT,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "tint",			do_set_stat,	POINT_IQ,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "tstr",			do_set_stat,	POINT_ST,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "tdex",			do_set_stat,	POINT_DX,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "cannot_dead",			do_can_dead,	1,	POS_DEAD,		GM_LOW_WIZARD},
	{ "can_dead",				do_can_dead,	0,	POS_DEAD,		GM_LOW_WIZARD},

	{ "full_set",	do_full_set, 0, POS_DEAD,		GM_IMPLEMENTOR },
	{ "item_full_set",	do_item_full_set, 0, POS_DEAD,		GM_IMPLEMENTOR },
	{ "attr_full_set",	do_attr_full_set, 0, POS_DEAD,		GM_IMPLEMENTOR },
	{ "all_skill_master",	do_all_skill_master,	0,	POS_DEAD,	GM_IMPLEMENTOR },
	{ "use_item",		do_use_item,	0, POS_DEAD,		GM_IMPLEMENTOR },

	{ "dragon_soul",				do_dragon_soul,				0,	POS_DEAD,	GM_PLAYER	},
	{ "ds_list",				do_ds_list,				0,	POS_DEAD,	GM_PLAYER	},
	{ "do_clear_affect", do_clear_affect, 	0, POS_DEAD,		GM_LOW_WIZARD},

#ifdef ENABLE_NEWSTUFF
	//item
	{ "add_rare_attr",		do_add_rare_attr,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "change_rare_attr",	do_change_rare_attr,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	//player
	{ "click_safebox",		do_click_safebox,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "force_logout",		do_force_logout,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "poison",				do_poison,					0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "rewarp",				do_rewarp,					0,			POS_DEAD,	GM_LOW_WIZARD	},
#endif
#ifdef ENABLE_WOLFMAN_CHARACTER
	{ "bleeding",			do_bleeding,				0,			POS_DEAD,	GM_IMPLEMENTOR	},
#endif
#ifdef ENABLE_CHANNEL_SWITCH_SYSTEM
	{ "change_current_channel_to",			do_change_channel,			0,			POS_DEAD,	GM_PLAYER		},
#endif
#ifdef ENABLE_AFFECT_BUFF_REMOVE
	{	"remove_this_buff",					do_remove_buff,					0,						POS_DEAD,			GM_PLAYER	},
#endif
#ifdef ENABLE_SPLIT_ITEMS_FAST
	{ "split_normal_item",						do_split_selected_item,					0,						POS_DEAD,			GM_PLAYER },
#ifdef ENABLE_SPECIAL_STORAGE
	{ "split_special_inv_item",					do_split_storage_item,					0,						POS_DEAD,			GM_PLAYER },
#endif
#endif
#ifdef ENABLE_SORT_INVENTORIES
	{ "l05_sort_normal_items",					do_sort_items,							0,						POS_DEAD,			GM_PLAYER	},
	{ "l05_sort_special_inv_items",				do_sort_special_storage,				0,						POS_DEAD,			GM_PLAYER	},
#endif
#ifdef ENABLE_OFFLINE_SHOP
	{ "offshop_change_shop_name", do_offshop_change_shop_name, 0,  POS_DEAD, GM_IMPLEMENTOR },
	{ "offshop_force_close_shop", do_offshop_force_close_shop, 0,  POS_DEAD, GM_IMPLEMENTOR },
#endif
#ifdef ENABLE_GLOBAL_RANKING
	{ "request_best_player_list", do_request_ranking_list, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_CHAR_SETTINGS
	{ "set_char_settings", do_SetCharSettings, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_FISHING_RENEWAL
	{ "fishing_renewal",	do_fishingrenewal, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_DISTANCE_NPC
	{ "open_this_distance_npc",					do_distance_npc,						0,			POS_DEAD,	GM_PLAYER	},
#endif
#ifdef ENABLE_HWID
	{ "hwid_ban",				do_hwidban,				0,			POS_DEAD,	GM_IMPLEMENTOR },
	{ "user_ban",					do_ban,					0,			POS_DEAD,	GM_IMPLEMENTOR },
#endif
#ifdef ENABLE_FARM_BLOCK
	{ "set_farm_block",	do_set_farm_block, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_FAST_SOUL_STONE_SYSTEM
	{ "soulstone_read_event", do_ruhoku, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_PASSIVE_SKILLS
	{ "passive_read", do_passiveread, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_EVENT_MANAGER
	{ "event_manager",	do_event_manager,		0,		POS_DEAD,	GM_IMPLEMENTOR },
#endif
#ifdef ENABLE_ITEMSHOP
	{ "ingame_item_shop_arg",	do_ishop,		0,		POS_DEAD,	GM_PLAYER },
#endif
#ifdef ENABLE_NAMING_SCROLL
	{ "naming_scroll_set_name",		do_namingscroll,				0,				POS_DEAD,	GM_PLAYER },
#endif
#ifdef ENABLE_MINING_RENEWAL
	{ "mining_event_renewal",		do_miningrenewal,				0,				POS_DEAD,	GM_PLAYER },
#endif

#ifdef ENABLE_SPECIAL_STORAGE
	{ "transfer_inv_to_specialinventory",					do_transfer_inv_storage,		0,						POS_DEAD,			GM_PLAYER },
	{ "transfer_specialinv_to_inventory",			do_transfer_storage_inv,		0,						POS_DEAD,			GM_PLAYER },
#endif
#ifdef ENABLE_GET_TOTAL_ONLINE_COUNT
	{ "total_players",						do_get_total_online,						0,				POS_DEAD,		GM_IMPLEMENTOR	},
#endif

#ifdef ENABLE_MAINTENANCE_SYSTEM
	{ "maintenance_update",		do_update_maintenance,		0,				POS_DEAD,	GM_IMPLEMENTOR		},
#endif
#ifdef ENABLE_VOTE4BUFF
	{ "vote_bonus",			do_vote_bonus,		0,				POS_DEAD,	GM_PLAYER },
#endif
#ifdef ENABLE_EXTENDED_BATTLE_PASS
	{ "battlepass_get_info",			do_battlepass_get_info,			0,	POS_DEAD,	RESTRICT_COMMAND_GET_INFO },
	{ "battlepass_set_mission",			do_battlepass_set_mission,		0,	POS_DEAD,	RESTRICT_COMMAND_SET_MISSION },
	{ "battlepass_premium_activate",	do_battlepass_premium_activate,	0,	POS_DEAD,	RESTRICT_COMMAND_PREMIUM_ACTIVATE },
#endif
#ifdef ENABLE_FAST_SKILL_BOOK_SYSTEM
	{"read_book_command", do_read_books_fast,	0, POS_DEAD, GM_PLAYER},
#endif
#ifdef ENABLE_CHAT_COLOR_SYSTEM
	{ "set_char_color_arg", do_set_chat_color, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_BOT_CONTROL
	{ "bot31_control_fail", do_bot_control,	0,	POS_DEAD, GM_PLAYER },
	{ "bot31_control_open", do_bot_control_open,	0,	POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_DISTANCE_SKILL_SELECT
	{ "31cekenkaplumbaga", do_skill_select,	0,	POS_DEAD, GM_PLAYER },
#endif
#ifdef __SKILL_TRAINING__
	{ "skill_training",	do_skill_training,		0,		POS_DEAD,	GM_PLAYER },
#endif
	{ "newfull",	do_new_full, 0, POS_DEAD,		GM_IMPLEMENTOR },
#ifdef ENABLE_FURKANA_GOTTEN
	{ "drop_calcute", do_drop_calc, 0, POS_DEAD,		GM_IMPLEMENTOR },
#endif
#if defined(__GAME_OPTION_ESCAPE__)
	{ "escape", do_escape, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef ENABLE_DAILY_REWARD
	{ "dailyreward_open",	do_dailyreward_open,	0,	POS_DEAD,	GM_PLAYER	},
	{ "dailyreward_process",		do_dailyreward_process,			0,	POS_DEAD,	GM_PLAYER	},	
#endif
#ifdef ENABLE_PLAYER_BLOCK_SYSTEM
	{ "block_player", do_block_player, 0, POS_DEAD, GM_PLAYER },
	{ "unblock_player", do_unblock_player, 0, POS_DEAD, GM_PLAYER },
#endif
#ifdef __STONE_SCROLL__
	{ "stone_scroll", do_stone_scroll, 0, POS_DEAD, GM_PLAYER },
#endif
	{ "\n",		NULL,			0,			POS_DEAD,	GM_IMPLEMENTOR	}
};

void interpreter_set_privilege(const char* cmd, int lvl)
{
	int i;

	for (i = 0; *cmd_info[i].command != '\n'; ++i)
	{
		if (!str_cmp(cmd, cmd_info[i].command))
		{
			cmd_info[i].gm_level = lvl;
			break;
		}
	}
}

void double_dollar(const char* src, size_t src_len, char* dest, size_t dest_len)
{
	const char* tmp = src;
	size_t cur_len = 0;

	dest_len -= 1;

	while (src_len-- && *tmp)
	{
		if (*tmp == '$')
		{
			if (cur_len + 1 >= dest_len)
				break;

			*(dest++) = '$';
			*(dest++) = *(tmp++);
			cur_len += 2;
		}
		else
		{
			if (cur_len >= dest_len)
				break;

			*(dest++) = *(tmp++);
			cur_len += 1;
		}
	}

	*dest = '\0';
}
// #define ENABLE_BLOCK_CMD_SHORTCUT
void interpret_command(LPCHARACTER ch, const char* argument, size_t len)
{
#ifdef ENABLE_ANTI_CMD_FLOOD
	if (ch && !ch->IsGM())
	{
		if (thecore_pulse() > ch->GetCmdAntiFloodPulse() + PASSES_PER_SEC(1))
		{
			ch->SetCmdAntiFloodCount(0);
			ch->SetCmdAntiFloodPulse(thecore_pulse());
		}
		if (ch->IncreaseCmdAntiFloodCount() >= 10)
		{
			ch->GetDesc()->DelayedDisconnect(0);
			return;
		}
	}
#endif
	if (NULL == ch)
	{
		sys_err("NULL CHRACTER");
		return;
	}

	char cmd[128 + 1];
	char new_line[256 + 1];
	const char* line;
	int icmd;

	if (len == 0 || !*argument)
		return;

	double_dollar(argument, len, new_line, sizeof(new_line));

	size_t cmdlen;
	line = first_cmd(new_line, cmd, sizeof(cmd), &cmdlen);

	for (icmd = 1; *cmd_info[icmd].command != '\n'; ++icmd)
	{
		if (cmd_info[icmd].command_pointer == do_cmd)
		{
			if (!strcmp(cmd_info[icmd].command, cmd))
				break;
		}
#ifdef ENABLE_BLOCK_CMD_SHORTCUT
		else if (!strcmp(cmd_info[icmd].command, cmd))
#else
		else if (!strncmp(cmd_info[icmd].command, cmd, cmdlen))
#endif
			break;
	}

	if (ch->GetPosition() < cmd_info[icmd].minimum_position)
	{
		return;
	}

	if (*cmd_info[icmd].command == '\n')
	{
		ch->NewChatPacket(CMD_TALK_1);
		return;
	}

	if (cmd_info[icmd].gm_level && (cmd_info[icmd].gm_level > ch->GetGMLevel() || cmd_info[icmd].gm_level == GM_DISABLE))
	{
		ch->NewChatPacket(CMD_TALK_1);
		return;
	}

	((*cmd_info[icmd].command_pointer) (ch, line, icmd, cmd_info[icmd].subcmd));
}