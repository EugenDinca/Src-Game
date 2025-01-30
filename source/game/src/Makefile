CXX = g++10

GAME_VERSION := 52000

GccMajorVersion := $(shell expr `$(CXX) -dumpversion | cut -f1 -d.`)
GccMinorVersion := $(shell expr `$(CXX) -dumpversion | cut -f2 -d.`)
GccMinorEGT8 := $(shell expr $(GccMinorVersion) \>= 8)

INCDIR =
LIBDIR =
BINDIR = /usr/Src-Game/source/game
OBJDIR = .obj
$(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

## LIST OF CONSTANTS BEGIN
ENABLE_LUA_5_VERSION = 0
ENABLE_GCC_AUTODEPEND = 1
ENABLE_STATIC = 0
## LIST OF CONSTANTS END

# Depend Path File
ifneq ($(ENABLE_GCC_AUTODEPEND), 1)
DEPFILE = Depend
endif

# Standard Libraries
LIBS = -lm -lmd

# Project Flags
CFLAGS  = -m32 -g -Wall -O2 -pipe -fexceptions -fno-strict-aliasing -pthread -D_THREAD_SAFE -DNDEBUG
CFLAGS += -Wno-deprecated-declarations -Wno-nonnull-compare -Wno-deprecated-declarations -Wno-array-bounds -Wno-address
CFLAGS += -Wno-int-in-bool-context -Wno-format-truncation -Wno-stringop-truncation -Wno-sign-compare
CXXFLAGS = -std=c++20 -Wl,-rpath=/usr/local/lib/gcc10

ifeq ($(ENABLE_STATIC), 1)
CFLAGS += -static
endif

ifeq ($(GccMinorEGT8), 1)
CFLAGS += -Wno-unused-local-typedefs
endif

# FreeBSD stack protector
CFLAGS += -fstack-protector-all

# Version defines
CFLAGS += -D__USER__=\"$(USER)\" -D__HOSTNAME__=\"$(HOSTNAME)\" -D__PWD__=\"$(PWD)\" -D__GAME_VERSION__=\"$(GAME_VERSION)\"

# Boost
INCDIR += -I../../../external/include/boost

# DevIL
INCDIR += -I../../../external/include/IL
LIBS += ../../../external/lib/libIL.a\
		../../../external/lib/libjasper.a\
		../../../external/lib/libpng.a\
		../../../external/lib/libtiff.a\
		../../../external/lib/libjbig.a\
		../../../external/lib/libmng.a\
		/usr/lib/liblzma.a\
		../../../external/lib/liblcms.a\
		../../../external/lib/libjpeg.a

# MySQL

# INCDIR += -I../../libs/mysql/
INCDIR += -I../../libs/mysql/
# INCDIR += -I/usr/local/include/mysql/server/
LIBS += /usr/local/lib/mysql/libmariadbclient.a /usr/lib/libz.a

# CryptoPP
LIBS += ../../../external/lib/libcryptopp.a

# OpenSSL
INCDIR += -I/usr/include
LIBS += -lssl -lcrypto
# LIBS += /usr/lib/libssl.a

# Lua
CFLAGS += -DENABLE_LUA_5_VERSION=ENABLE_LUA_5_VERSION
ifeq ($(ENABLE_LUA_5_VERSION), 2)
INCDIR += -I../../libs/liblua/5.2/install/include
LIBDIR += -L../../libs/liblua/5.2/install/lib
LIBS += ../../libs/liblua/5.2/install/lib/liblua.a
else
INCDIR += -I../../libs/liblua/5.0/include
LIBDIR += -L../../libs/liblua/5.0/lib
LIBS += ../../libs/liblua/5.0/lib/liblua.a ../../libs/liblua/5.0/lib/liblualib.a
endif

#json
INCDIR += -I../../../external/include/rapidjson

# Project Libraries
INCDIR += -I../../../external/include
INCDIR += -I/usr/local/include
LIBDIR += -L/usr/local/lib

LIBDIR += -L../../libs/libthecore/lib -L../../libs/libpoly -L../../libs/libsql -L../../libs/libgame/lib
LIBS += -lthecore -lpoly -lsql -lgame

# PROJECT_SRC_FILES BEGIN
MAINCPP = main.cpp

CFILE	= minilzo.c

CPPFILE = BattleArena.cpp\
		  FSM.cpp\
		  MarkConvert.cpp\
		  MarkImage.cpp\
		  MarkManager.cpp\
		  OXEvent.cpp\
		  ani.cpp\
		  arena.cpp\
		  banword.cpp\
		  battle.cpp\
		  blend_item.cpp\
		  buffer_manager.cpp\
		  building.cpp\
		  char.cpp\
		  char_affect.cpp\
		  char_battle.cpp\
		  char_change_empire.cpp\
		  char_horse.cpp\
		  char_item.cpp\
		  char_manager.cpp\
		  char_quickslot.cpp\
		  char_resist.cpp\
		  char_skill.cpp\
		  char_state.cpp\
		  PetSystem.cpp\
		  cmd.cpp\
		  cmd_emotion.cpp\
		  cmd_general.cpp\
		  cmd_gm.cpp\
		  cmd_oxevent.cpp\
		  config.cpp\
		  constants.cpp\
		  crc32.cpp\
		  cuberenewal.cpp\
		  db.cpp\
		  desc.cpp\
		  desc_client.cpp\
		  desc_manager.cpp\
		  desc_p2p.cpp\
		  dev_log.cpp\
		  dungeon.cpp\
		  entity.cpp\
		  entity_view.cpp\
		  event.cpp\
		  event_queue.cpp\
		  exchange.cpp\
		  file_loader.cpp\
		  fishing.cpp\
		  gm.cpp\
		  guild.cpp\
		  guild_manager.cpp\
		  guild_war.cpp\
		  horse_rider.cpp\
		  horsename_manager.cpp\
		  input.cpp\
		  input_auth.cpp\
		  input_db.cpp\
		  input_login.cpp\
		  input_main.cpp\
		  input_p2p.cpp\
		  input_udp.cpp\
		  ip_ban.cpp\
		  item.cpp\
		  item_addon.cpp\
		  item_attribute.cpp\
		  item_manager.cpp\
		  item_manager_idrange.cpp\
		  locale.cpp\
		  locale_service.cpp\
		  login_data.cpp\
		  lzo_manager.cpp\
		  marriage.cpp\
		  messenger_manager.cpp\
		  mining.cpp\
		  mob_manager.cpp\
		  motion.cpp\
		  p2p.cpp\
		  packet_info.cpp\
		  party.cpp\
		  polymorph.cpp\
		  priv_manager.cpp\
		  pvp.cpp\
		  questevent.cpp\
		  questlua.cpp\
		  questlua_affect.cpp\
		  questlua_arena.cpp\
		  questlua_ba.cpp\
		  questlua_building.cpp\
		  questlua_danceevent.cpp\
		  questlua_dungeon.cpp\
		  questlua_game.cpp\
		  questlua_global.cpp\
		  questlua_guild.cpp\
		  questlua_horse.cpp\
		  questlua_pet.cpp\
		  questlua_item.cpp\
		  questlua_marriage.cpp\
		  questlua_npc.cpp\
		  questlua_oxevent.cpp\
		  questlua_party.cpp\
		  questlua_pc.cpp\
		  questlua_quest.cpp\
		  questlua_target.cpp\
		  questmanager.cpp\
		  questnpc.cpp\
		  questpc.cpp\
		  refine.cpp\
		  regen.cpp\
		  safebox.cpp\
		  sectree.cpp\
		  sectree_manager.cpp\
		  shop.cpp\
		  skill.cpp\
		  start_position.cpp\
		  target.cpp\
		  text_file_loader.cpp\
		  trigger.cpp\
		  utils.cpp\
		  vector.cpp\
		  war_map.cpp\
		  wedding.cpp\
		  version.cpp\
		  map_location.cpp\
		  skill_power.cpp\
		  affect.cpp\
		  cipher.cpp\
		  buff_on_attributes.cpp\
		  dragon_soul_table.cpp\
		  DragonSoul.cpp\
		  group_text_parse_tree.cpp\
		  char_dragonsoul.cpp\
		  questlua_dragonsoul.cpp\
		  shop_manager.cpp\
		  shopEx.cpp\
		  item_manager_read_tables.cpp\
		  shutdown_manager.cpp\
		  questlua_dnd.cpp\
		  MountSystem.cpp\
		  new_offlineshop.cpp\
		  new_offlineshop_manager.cpp\
		  new_offlineshop_safebox.cpp\
		  new_switchbot.cpp\
		  char_special_group_rework.cpp\
		  statistics_rank.cpp\
		  Inventory_Rework.cpp\
		  biolog_system.cpp\
		  hwid_manager.cpp\
		  alignment_proto.cpp\
		  dungeon_info.cpp\
		  New_PetSystem.cpp\
		  CsvReader.cpp\
		  maintenance.cpp\
		  battlepass_manager.cpp\
		  BuffiSystem.cpp\
		  BossRegen.cpp\
		  InGameLogManager.cpp\
		  minigame_bnw.cpp\
		  minigame_catchking.cpp\
		  minigame_okeycard.cpp\
		  player_block.cpp
# PROJECT_SRC_FILES END

BOSS_DAMAGE_RANKING_PLUGIN = 1
ifeq ($(BOSS_DAMAGE_RANKING_PLUGIN), 1)
CPPFILE += bossdamageranking.cpp
CPPFILE += bossdamagerankingmanager.cpp
endif

# PROJECT_OBJ_FILES BEGIN
COBJS	= $(CFILE:%.c=$(OBJDIR)/%.o)
CPPOBJS	= $(CPPFILE:%.cpp=$(OBJDIR)/%.o)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
CDEPS	= $(COBJS:%.o=%.d)
CPPDEPS	= $(CPPOBJS:%.o=%.d)
endif

MAINOBJ = $(MAINCPP:%.cpp=$(OBJDIR)/%.o)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
MAINDEPS = $(MAINOBJ:%.o=%.d)
endif

# PROJECT_OBJ_FILES END

# Target Paths
MAIN_TARGET = $(BINDIR)/game

default: $(MAIN_TARGET)
test: $(TEST_TARGET)

$(OBJDIR)/%.o: %.c
	@echo compiling $<
	@$(CXX) $(CFLAGS) $(INCDIR) -c $< -o $@
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@$(CXX) -MM -MG -MP $(CFLAGS) $(INCDIR) -c $< -o $(OBJDIR)/$*.d
	@sed -i '' -e's/$*.o:/$(OBJDIR)\/$*.o:/g' $(OBJDIR)/$*.d
endif

$(OBJDIR)/%.o: %.cpp
	@echo compiling $<
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCDIR) -c $< -o $@
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@$(CXX) -MM -MG -MP $(CFLAGS) $(CXXFLAGS) $(INCDIR) -c $< -o $(OBJDIR)/$*.d
	@sed -i '' -e's/$*.o:/$(OBJDIR)\/$*.o:/g' $(OBJDIR)/$*.d
endif

$(MAIN_TARGET): $(CPPOBJS) $(COBJS) $(MAINOBJ)
	@echo linking $(MAIN_TARGET)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(LIBDIR) $(COBJS) $(CPPOBJS) $(MAINOBJ) $(LIBS) -o $(MAIN_TARGET)

strip:
	@cp $(MAIN_TARGET) $(BINDIR)/game_r
	@strip $(BINDIR)/game_r

clean:
	@echo cleaning $(MAIN_TARGET) $(OBJDIR)
	@rm -f $(COBJS) $(CPPOBJS) $(MAINOBJ)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@rm -f $(CDEPS) $(CPPDEPS) $(MAINDEPS)
endif
	@rm -f $(BINDIR)/game_r* $(BINDIR)/test_r* $(BINDIR)/conv

dep:
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@echo "Note: gcc autodepend is autodetected, so target dep skipped"
else
	makedepend -f $(DEPFILE) $(INCDIR) -I/usr/include/c++/3.3 -I/usr/include/c++/4.2 -p$(OBJDIR)/ $(CPPFILE) $(CFILE) $(MAINCPP) $(TESTCPP) 2> /dev/null > $(DEPFILE)
endif

# AUTO_DEPEND_CHECK BEGIN
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
sinclude $(CDEPS)
sinclude $(CPPDEPS)
sinclude $(MAINDEPS)
else
sinclude $(DEPFILE)
endif
# AUTO_DEPEND_CHECK END
