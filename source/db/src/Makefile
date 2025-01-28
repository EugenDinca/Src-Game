CC = gcc10
CXX = g++10

DB_VERSION = 52000

INCDIR =
LIBDIR =
BINDIR = /usr/src/source/db
OBJDIR = .obj
$(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

# LIST_OF_CONSTANTS BEGIN
ENABLE_GCC_AUTODEPEND = 1
ENABLE_STATIC = 0
# LIST_OF_CONSTANTS END

# Depend Path File
ifneq ($(ENABLE_GCC_AUTODEPEND), 1)
DEPFILE = Depend
endif

# Project Flags
CFLAGS  = -m32 -g -Wall -O2 -pipe -fno-rtti -fno-exceptions -Wno-long-long -pthread -D_THREAD_SAFE
CFLAGS += -Wno-deprecated-declarations -Wno-nonnull-compare -Wno-deprecated-declarations -Wno-array-bounds -Wno-address
CFLAGS += -Wno-int-in-bool-context -Wno-format-truncation
CXXFLAGS = -std=c++20 -Wl,-rpath=/usr/local/lib/gcc10

ifeq ($(ENABLE_STATIC), 1)
CFLAGS += -static
endif

# Version defines
CFLAGS += -D__USER__=\"$(USER)\" -D__HOSTNAME__=\"$(HOSTNAME)\" -D__PWD__=\"$(PWD)\" -D__DB_VERSION__=\"$(DB_VERSION)\"

# Boost
INCDIR += -I../../../external/include/boost

# MySQL
# INCDIR += -I/usr/local/include/mysql/server/
INCDIR += -I../../libs/mysql/

LIBS += /usr/local/lib/mysql/libmariadbclient.a /usr/lib/libz.a

# Project Libraries
INCDIR += -I/usr/local/include
INCDIR += -I../../../external/include
LIBDIR += -I../../../external/lib

LIBDIR += -L../../libs/libthecore/lib -L../../libs/libsql -L../../libs/libpoly -L../../libs/libgame/lib
LIBS += -lthecore -lsql -lpoly -lgame -lm

# OpenSSL
LIBS += -lssl -lcrypto

# PROJECT_SRC_FILES BEGIN
CPPFILE = Config.cpp\
		  NetBase.cpp\
		  Peer.cpp\
		  PeerBase.cpp\
		  Main.cpp\
		  Lock.cpp\
		  DBManager.cpp \
		  Cache.cpp\
		  LoginData.cpp\
		  ClientManager.cpp\
		  ClientManagerPlayer.cpp\
		  ClientManagerLogin.cpp\
		  ClientManagerBoot.cpp\
		  ClientManagerParty.cpp\
		  ClientManagerGuild.cpp\
		  GuildManager.cpp\
		  PrivManager.cpp\
		  ItemAwardManager.cpp\
		  ClientManagerEventFlag.cpp\
		  Marriage.cpp\
		  ItemIDRangeManager.cpp\
		  ClientManagerHorseName.cpp\
		  version.cpp\
		  ProtoReader.cpp\
		  CsvReader.cpp\
		  ClientManagerOfflineshop.cpp\
		  InGameLog.cpp\
		  ClientManagerHwid.cpp\
		  OfflineshopCache.cpp\
		  buffer_manager.cpp 
# PROJECT_SRC_FILES END

# PROJECT_OBJ_FILES BEGIN
CPPOBJS = $(CPPFILE:%.cpp=$(OBJDIR)/%.o)
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
CPPDEPS	= $(CPPOBJS:%.o=%.d)
endif
# PROJECT_OBJ_FILES END

# Target Paths
MAIN_TARGET = $(BINDIR)/db

default: $(MAIN_TARGET)

$(MAIN_TARGET): $(CPPOBJS)
	@echo linking $(MAIN_TARGET)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(LIBDIR) $(CPPOBJS) $(LIBS) -o $(MAIN_TARGET)

$(OBJDIR)/%.o: %.cpp
	@echo compiling $<
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCDIR) -c $< -o $@
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@$(CXX) -MM -MG -MP $(CFLAGS) $(CXXFLAGS) $(INCDIR) -c $< -o $(OBJDIR)/$*.d
	@sed -i '' -e's/$*.o:/$(OBJDIR)\/$*.o:/g' $(OBJDIR)/$*.d
endif

$(OBJDIR):
	@mkdir $(OBJDIR)

strip:
	@cp $(BINDIR)/db_r$(DB_VERSION) $(BINDIR)/db_r
	@strip $(BINDIR)/db_r

clean:
	@rm -f $(CPPOBJS) $(BINDIR)/db_r*

dep:
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
	@echo "Note: gcc autodepend is autodetected, so target dep skipped"
else
	makedepend -f $(DEPFILE) $(INCDIR) -I/usr/include/c++/3.3 -I/usr/include/c++/4.2 -p$(OBJDIR)/ $(CPPFILE) 2> /dev/null > $(DEPFILE)
endif

# AUTO_DEPEND_CHECK BEGIN
ifeq ($(ENABLE_GCC_AUTODEPEND), 1)
sinclude $(CPPDEPS)
else
sinclude $(DEPFILE)
endif
# AUTO_DEPEND_CHECK END
