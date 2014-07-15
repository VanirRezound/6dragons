CC = g++
MAKE = make
# Commented out profiling as it could cause crashes in some copyovering scenarios
PROF =
#PROF = -pg
#NOCRYPT =

#Uncomment the line below if you'll be using gdb for debugging information
#USE_GDB = gdb
DEBUG_LEVEL = 2

#The program executable's name
PROG_NAME = 6dragons

#Memory Debugger - comment out to turn the debugger off, uncomment to use it.
MEMDEBUG = 1

#Comment this if you don't want math errors
MATH_LIB = -lm

#Uncomment the line below if compiling on a RedHat (and possibly other Linux) box
LINUX = -DLINUX

#Uncomment the line below if you are getting a line like:
#interp.c:757: warning: int format, time_t arg (arg 7)
#TIME = -DTIMEFORMAT

#I3 - Comment out to disable I3 support
#I3 = 1

#Uncomment the line below if you are getting undefined crypt errors
NEED_CRYPT = -lcrypt

#Uncomment the line below if you are getting undefined references to dlsym, dlopen, and dlclose.
#Comment it out if you get errors about ldl not being found.
#NEED_DL = -ldl

#Volk added this for MYSQL support
#MYSQL = -L/usr/lib/mysql -lmysqlclient

#Bexar added this for Lua support
#LUA = -llua

#Directory locations of important lib files
LIBDIR  = ./lib

#Modify the lines below if you want a performance increase though beware your core
#files may not be as much of a benefit if you do. Suggested OPT_FLAG: -O
OPT_LEVEL = 
OPT_FLAG = -O$(OPT_LEVEL)

#Format check flags
FMT_FLG = -Wformat -Wformat-security -Wmissing-format-attribute

#Warning check flags
W_FLAGS = -Wall -Werror -Wshadow -Wformat-security -Wpointer-arith -Wcast-align -Wredundant-decls -Wconversion 

#Compile and Link flags
CMP_FLG = -g$(USE_GDB)2 $(WRN_FLG) 
C_FLAGS = $(OPT_FLAG) $(CMP_FLG) $(PROF) $(NOCRYPT) $(SOLARIS_FLAG) $(TIME) $(LINUX)
# L_FLAGS = $(PROF) $(SOLARIS_LINK) -lz $(NEED_DL) $(NEED_CRYPT) $(MATH_LIB) $(MYSQL) $(LUA) -lm
L_FLAGS = $(PROF) $(SOLARIS_LINK) -lz $(NEED_DL) $(NEED_CRYPT) $(MATH_LIB) $(MYSQL)

#Listing of all '.c' files DO NOT GET OUT OF ALPHABETICAL ORDER
C_FILES = act_comm.c      act_info.c       act_move.c     act_obj.c    act_wiz.c    alias.c     archery.c  location.c \
          arena.c         automap.c        ban.c          bank.c       boards.c     build.c     buitdy.c   calendar.c  city.c \
          channels.c      clans.c          classes.c      color.c      comm.c       comments.c  companion.c  const.c    copyover.c \
          currency.c      crafts.c         db.c           deity.c      dragon.c     editor.c    events.c   faith.c \
          fight.c         filer.c          ftag.c         gboard.c     genesis.c    handler.c    hint.c       hometowns.c house.c    imm_host.c interp.c  landmark.c \
          magic.c         mccp.c  medit.c     memory.c   misc.c \
          monk.c          mssp.c           mud_comm.c     mud_prog.c   multi.c      pets.c         pfiles.c      mt19937ar.c  new_auth.c  news.c     oedit.c \
          player.c        polymorph.c      portalstone.c  quests.c     races.c      redit.c     renumber.c reset.c \
          save.c          services.c       shops.c        skills.c     slay.c       smell.c      special.c   status.c   symref.c \
          teacher.c       track.c          update.c       vault.c      variables.c  \
          tables.c        weather.c        whereis.c      whois.c      wild_comm.c  wild_mobs.c      

#Listing of all '.h' files necessary to create the program binary

ifdef I3
   C_FILES := i3.c md5.c $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DI3 -DI3CHRONICLES
endif

O_FILES := $(patsubst %.c,o/%.o,$(C_FILES))

H_FILES = $(wildcard h/*.h)

all:
	@$(MAKE) -s $(PROG_NAME)

$(PROG_NAME): $(O_FILES)
	@rm -f $(PROG_NAME)
	@$(CC) $(L_FLAGS) -o $(PROG_NAME) $(O_FILES) $(L_FLAGS)
	@chmod 700 $(C_FILES)
	@chmod 700 $(H_FILES)
	@chmod g+w $(PROG_NAME)
	@chmod a+x $(PROG_NAME)
	@chmod g+w $(O_FILES)
	mv $(PROG_NAME) ../$(PROG_NAME)
	@echo "Done Compiling $(PROG_NAME)"

symref.c: h/mud.h symref_template.c mktables h/landmark.h h/mccp.h
	./mktables symref.c symref_template.c -h h/mud.h -h h/landmark.h -h h/mccp.h

o/%.o: %.c $(H_FILES)
	echo "  Compiling $@";
	$(CC) -c $(C_FLAGS) $< -o $@

.c.o: mud.h
	$(CC) -c $(C_FLAGS) $<

clean:
	rm -f o/*.o ../$(PROG_NAME) *~ h/*~ symref.c
	make all

purge:
	rm -f o/*.o ../$(PROG_NAME) *~ h/*~ symref.c

indent:
	indent -ts2 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -nprs -bap -cbi0 -cli2 -bli0 -l200 -lp -i2 -cdb -c1 -cd1 -sc -pi0 $(C_FILES)
	indent -ts2 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -nprs -bap -cbi0 -cli2 -bli0 -l200 -lp -i2 -cdb -c1 -cd1 -sc -pi0 $(H_FILES)

indentclean:
	rm *.c~ h/*.h~
