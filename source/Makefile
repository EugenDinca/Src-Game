CC = gcc10
CXX = g++10

PLATFORM = $(shell file /bin/ls | cut -d' ' -f3 | cut -d'-' -f1)
BSD_VERSION = $(shell uname -v 2>&1 | cut -d' ' -f2 | cut -d'.' -f1)
SVR_VERSION = 52000

.PHONY:  liblua libsql libgame libpoly libthecore game db

default: liblua libsql libgame libpoly libthecore game db
	@echo "--------------------------------------"
	@echo "Build Done"
	@echo "--------------------------------------"

liblua: .
	$(MAKE) -C $@/5.0 clean
	$(MAKE) -C $@/5.0

libsql: .
	@touch $@/Depend
	$(MAKE) -C $@ dep
	$(MAKE) -C $@ clean
	$(MAKE) -C $@

libgame: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src

libpoly: .
	@touch $@/Depend
	$(MAKE) -C $@ dep
	$(MAKE) -C $@ clean
	$(MAKE) -C $@

libthecore: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src

game: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src

db: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src

gamedb:
	@touch game/src/Depend
	$(MAKE) -C game/src dep
	$(MAKE) -C game/src

	@touch db/src/Depend
	$(MAKE) -C db/src dep
	$(MAKE) -C db/src

gamedbclean:
	$(MAKE) -C game/src clean
	$(MAKE) -C db/src clean

ver:
	@$(CC) -v
ver2:
	@$(CC) -v
	$(MAKE) -C game/src ver

strip:
	$(MAKE) -C game/src strip
	$(MAKE) -C db/src strip

clean:
	$(MAKE) -C libs/liblua/5.0 clean
	$(MAKE) -C libs/libsql clean
	$(MAKE) -C libs/libgame/src clean
	$(MAKE) -C libs/libpoly clean
	$(MAKE) -C libs/libthecore/src clean
	$(MAKE) -C game/src clean
	$(MAKE) -C db/src clean

libs: 
	@echo "--------------------------------------"
	@echo "Library Build Start"
	@echo "--------------------------------------"
	$(MAKE) -C libs/liblua/5.0 clean
	$(MAKE) -C libs/liblua/5.0
	@touch libs/libsql/Depend
	$(MAKE) -C libs/libsql dep
	$(MAKE) -C libs/libsql clean
	$(MAKE) -C libs/libsql
	@touch libs/ibgame/src/Depend
	$(MAKE) -C libs/libgame/src dep
	$(MAKE) -C libs/libgame/src clean
	$(MAKE) -C libs/libgame/src
	@touch libs/libpoly/Depend
	$(MAKE) -C libs/libpoly dep
	$(MAKE) -C libs/libpoly clean
	$(MAKE) -C libs/libpoly
	@touch libs/libthecore/src/Depend
	$(MAKE) -C libs/libthecore/src dep
	$(MAKE) -C libs/libthecore/src clean
	$(MAKE) -C libs/libthecore/src
	@echo "--------------------------------------"
	@echo "Library is Builded!"
	@echo "--------------------------------------"
all:
	@echo "--------------------------------------"
	@echo "Update Revision"
	@echo "--------------------------------------"

	@echo "--------------------------------------"
	@echo "Full Build Start"
	@echo "--------------------------------------"

	$(MAKE) -C libs/liblua/5.0 clean
	$(MAKE) -C libs/liblua/5.0

	@touch libs/libsql/Depend
	$(MAKE) -C libs/libsql dep
	$(MAKE) -C libs/libsql clean
	$(MAKE) -C libs/libsql

	@touch libs/libgame/src/Depend
	$(MAKE) -C libs/libgame/src dep
	$(MAKE) -C libs/libgame/src clean
	$(MAKE) -C libs/libgame/src

	@touch libs/libpoly/Depend
	$(MAKE) -C libs/libpoly dep
	$(MAKE) -C libs/libpoly clean
	$(MAKE) -C libs/libpoly

	@touch libs/libthecore/src/Depend
	$(MAKE) -C libs/libthecore/src dep
	$(MAKE) -C libs/libthecore/src clean
	$(MAKE) -C libs/libthecore/src

	@touch game/src/Depend
	$(MAKE) -C game/src dep
	$(MAKE) -C game/src clean
	$(MAKE) -C game/src

	@touch db/src/Depend
	$(MAKE) -C db/src dep
	$(MAKE) -C db/src clean
	$(MAKE) -C db/src
	@echo "--------------------------------------"
	@echo "Full Build End"
	@echo "--------------------------------------"
