/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 * - Win32 port by Nick Gammon                                             *
 ***************************************************************************
 * - Main MUD header                                                       *
 ***************************************************************************/
#include <stdlib.h>
#include <limits.h>
#if defined(__CYGWIN__) || defined(__FreeBSD__)
#include <sys/time.h>
#endif
#ifdef __cplusplus
#if defined(WIN32)
#include <typeinfo.h>
#else
#include <typeinfo>
#endif
#endif
#include <stdio.h>

#include "classes.h"

#ifdef WIN32
#ifdef _CONSOLE                                        /* this is a trick so mingw32
                                                        * builds do not have a warning -
                                                        * MSVC defines this for console
                                                        * - we are using it to detect a
                                                        * build from MSVC++ */
#pragma warning( disable: 4550 4715)
#pragma warning( disable: 4100 4127 4204 4223 4244 4514)
#endif
#pragma warning( disable: 4115 4201 4214)
#include <winsock.h>
#pragma warning( default: 4115 4201 4214)
#include <sys/types.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define NOCRYPT
typedef int             socklen_t;

#define index strchr
#define rindex strrchr
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define lstat stat
#else
#include <unistd.h>
#define closesocket close
#define _snprintf snprintf
/*#ifndef SYSV
  #include <sys/cdefs.h>
#else
  #include <re_comp.h>
#endif*/
#include <sys/time.h>
#endif

#define LOG_I3  LOG_NORMAL


FILE                   *__FileOpen( const char *filename, const char *mode, const char *file,
                                    const char *function, int line );
void                    FileClose( FILE * fp );

#define FileOpen( filename, mode)  __FileOpen(filename, mode, __FILE__, __FUNCTION__, __LINE__)

typedef struct file_data FILE_DATA;

extern FILE_DATA       *first_filedata;
extern FILE_DATA       *last_filedata;
extern int              FilesOpen;
struct file_data
{
    FILE_DATA              *next;
    FILE_DATA              *prev;
    FILE                   *fp;
    char                   *filename;
    char                   *mode;

    // *Where they were called from* //
    char                   *file;
    char                   *function;
    int                     line;
};

typedef int             ch_ret;
typedef int             obj_ret;

/* Accommodate old non-Ansi compilers. */
#if defined(TRADITIONAL)
#define const
#define args(list) ()
#define DECLARE_DO_FUN(fun) void fun()
#define DECLARE_SPEC_FUN(fun) bool fun()
#define DECLARE_SPELL_FUN(fun) ch_ret fun()
#else
#define args(list) list
#define DECLARE_DO_FUN(fun) DO_FUN fun
#define DECLARE_SPEC_FUN(fun) SPEC_FUN fun
#define DECLARE_SPELL_FUN(fun) SPELL_FUN fun
#endif

#define REMOVE_AFFECTED(ch, sn) (xREMOVE_BIT((ch)->affected_by, (sn)))
/* #define REMOVE_AFFECTED2(ch, sn) (xREMOVE_BIT((ch)->affected_by2,(sn))) */
#define deathmsg(format, ch, arg1, arg2, type) act(format, ch, arg1, arg2, type)

#define STARTING_DISTANCE 3
#define MAX_HIT_DISTANCE 8
#define MSP_DEFAULT		    -99
/* Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE 1
#endif

#if !defined(BERR)
#define BERR 255
#endif
/*
#if defined(_AIX)
#if !defined(const)
#define const
#endif
typedef int bool;
#define unix
#else
typedef unsigned char bool;
#endif
*/
#define bool unsigned char
bool                    remove_printf( char *fmt, ... );
void                    remove_file( const char *filename );

/* Structure types. */
typedef struct editor_data EDITOR_DATA;
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct watch_data WATCH_DATA;
typedef struct extracted_char_data EXTRACT_CHAR_DATA;
typedef struct char_data CHAR_DATA;
typedef struct hunt_hate_fear HHF_DATA;
typedef struct fighting_data FIGHT_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct nuisance_data NUISANCE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct id_Name  id_Name;
typedef struct pc_data PC_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct reserve_data RESERVE_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct hour_min_sec HOUR_MIN_SEC;
typedef struct mob_prog_data MPROG_DATA;
typedef struct mob_prog_act_list MPROG_ACT_LIST;
typedef struct mppause_data MPPAUSE_DATA;
typedef struct teleport_data TELEPORT_DATA;
typedef struct timer_data TIMER;
typedef struct system_data SYSTEM_DATA;
typedef struct smaug_affect SMAUG_AFF;
typedef struct who_data WHO_DATA;
typedef struct skill_type SKILLTYPE;
typedef struct social_type SOCIALTYPE;
typedef struct cmd_type CMDTYPE;
typedef struct killed_data KILLED_DATA;
typedef struct pkilled_data PKILLED_DATA;
typedef struct deity_data DEITY_DATA;
typedef struct variable_data VARIABLE_DATA;
typedef struct staffent STAFFENT;
typedef struct ignore_data IGNORE_DATA;
typedef struct immortal_host IMMORTAL_HOST;
typedef struct extended_bitvector EXT_BV;
typedef struct found_area FOUND_AREA;
typedef struct currency_data CURRENCY_DATA;
typedef struct currency_index_data CURR_INDEX_DATA;
typedef struct bug_data BUG_DATA;
typedef struct idea_data IDEA_DATA;
typedef struct typo_data TYPO_DATA;
typedef struct bank_data BANK_DATA;
typedef struct olc_data OLC_DATA;
typedef struct location_data LOCATION;
typedef struct quest_data QUEST_DATA;
typedef struct chquest_data CHQUEST_DATA;
typedef struct chap_data CHAP_DATA;

#define MSL MAX_STRING_LENGTH                          /* Change MAX_STRING_LENGTH in
                                                        * mud.h */
#define MIL MAX_INPUT_LENGTH                           /* Change MAX_INPUT_LENGTH in
                                                        * mud.h */
#define MFL	                 256

/* Function types. */
typedef void DO_FUN     ( CHAR_DATA *ch, char *argument );
typedef bool SPEC_FUN   ( CHAR_DATA *ch );
typedef ch_ret SPELL_FUN ( int sn, int level, CHAR_DATA *ch, void *vo );

#define DUR_CONV 23.333333333333333333333333
#define HIDDEN_TILDE '*'

/* 32bit bitvector defines */
#define BV00 (1 <<  0)
#define BV01 (1 <<  1)
#define BV02 (1 <<  2)
#define BV03 (1 <<  3)
#define BV04 (1 <<  4)
#define BV05 (1 <<  5)
#define BV06 (1 <<  6)
#define BV07 (1 <<  7)
#define BV08 (1 <<  8)
#define BV09 (1 <<  9)
#define BV10 (1 << 10)
#define BV11 (1 << 11)
#define BV12 (1 << 12)
#define BV13 (1 << 13)
#define BV14 (1 << 14)
#define BV15 (1 << 15)
#define BV16 (1 << 16)
#define BV17 (1 << 17)
#define BV18 (1 << 18)
#define BV19 (1 << 19)
#define BV20 (1 << 20)
#define BV21 (1 << 21)
#define BV22 (1 << 22)
#define BV23 (1 << 23)
#define BV24 (1 << 24)
#define BV25 (1 << 25)
#define BV26 (1 << 26)
#define BV27 (1 << 27)
#define BV28 (1 << 28)
#define BV29 (1 << 29)
#define BV30 (1 << 30)
#define BV31 (1 << 31)
/* 32 USED! DO NOT ADD MORE! SB */

/* String and memory management parameters. */
#define MAX_KEY_HASH      2048
#define MAX_STRING_LENGTH 8192  /* buf */              /* Was 4096 */
#define MAX_INPUT_LENGTH  2048                         /* arg - Was 1024 */
#define MAX_INBUF_SIZE    4096                         /* Was 1024 */

/* Maximum socket connections allowed, for logging in. Default is 20. -Orion */
#define MAX_CONNECT_SOCKET  20
#define HASHSTR                                        /* use string hashing */
#define MAX_LAYERS          8                          /* maximum clothing layers */
#define MAX_NEST            100                        /* maximum container nesting */
#define MAX_KILLTRACK      20                          /* track mob vnums killed */
#define MAX_QUEST	   500                             /* Maximum amount of quests *
                                                        * allowed */
#define MAX_CHAPTERS	   30

/* Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define SPELL_SILENT_MARKER "silent"                   /* No OK. or Failed. */
#define MAX_EXP_WORTH 500000
#define MIN_EXP_WORTH 500
#define MAX_VNUM      100000                           /* Maximum room, object, and
                                                        * mobile vnum */
#define MAX_REXITS    20                               /* Maximum exits allowed in 1 room 
                                                        */
#define MAX_SKILL     1000
#define MAX_CLASS     25
#define MAX_NPC_CLASS 24

/* MULTICLASS LEVEL DIFFERENCE */
#define MC_LD		8
#define MAX_LEVEL      108
#define MAX_CLASS_LVL  100                             /* Used to support multi-class
                                                        * levels. -Taon */
#define MAX_CPD        4                               /* Maximum council power level
                                                        * difference */
#define MAX_HERB       20
#define MAX_DISEASE    20
#define MAX_PERSONAL   5                               /* Maximum personal skills */
#define MAX_WHERE_NAME 31
#define ADVDEATHLEVEL  10                              /* Where advanced players stop
                                                        * perma dying! */

#define LEVEL_AJ_GENERAL MAX_LEVEL
#define LEVEL_AJ_COLONEL (MAX_LEVEL - 1)
#define LEVEL_AJ_MAJOR    (MAX_LEVEL - 2)
#define LEVEL_AJ_CAPTAIN (MAX_LEVEL - 3)
#define LEVEL_AJ_LT      (MAX_LEVEL - 4)
#define LEVEL_AJ_SGT     (MAX_LEVEL - 5)
#define LEVEL_AJ_CPL     (MAX_LEVEL - 6)
#define LEVEL_IMMORTAL    (MAX_LEVEL - 7)
#define LEVEL_DEMIGOD     (MAX_LEVEL - 8)
#define LEVEL_AVATAR      LEVEL_DEMIGOD
#define LEVEL_HERO        LEVEL_DEMIGOD
#define LEVEL_LOG         LEVEL_IMMORTAL
#define LEVEL_HIGOD       LEVEL_CAPTAIN

#define IS_PUPPET(ch)	(ch && ch->pcdata && IS_SET((ch)->pcdata->flags, PCFLAG_PUPPET))
#include "memory.h"
#include "color.h"
#include "alias.h"

/* Needed here for board.h and gboard.h */
typedef struct global_board_data GLOBAL_BOARD_DATA;

#define MAX_BOARD 4
extern bool             GlobalAbort;

#include "board.h"
#include "gboard.h"

/* This is to tell if act uses uppercasestring or not --Shaddai */
extern bool             DONT_UPPER;
extern const char      *const wear_locs[];

#define SECONDS_PER_TICK		sysdata.secpertick

#define PULSE_PER_SECOND		sysdata.pulsepersec
#define PULSE_VIOLENCE			sysdata.pulseviolence
#define PULSE_MOBILE			sysdata.pulsemobile
#define PULSE_TICK				sysdata.pulsetick
#define PULSE_AREA				(60 * PULSE_PER_SECOND)
#define PULSE_AUCTION				 (21 * PULSE_PER_SECOND)
#define PULSE_ARENA                             (22 * PULSE_PER_SECOND)
#define PULSE_TUTORIAL                          50

/* Codebase Version Definitions -- Orion */
#define CODEBASE_VERSION_TITLE  "CHRONICLES"
#define CODEBASE_VERSION_MAJOR  "1"
#define CODEBASE_VERSION_MINOR  "05a"
#define CODEBASE_VERSION_INFO  "Chronicles can be downloaded from - http://www.mudplanet.org/chronicles/\n\rWe the Staff of Archaic Journey chose Chronicles code due to the bugfixes\r\nand because Orion took out all the extra code that was wasting memory.\t\nArchaic Journey highly recommends this codebase."
#define CODEBASE_VERSION_BAR   "--=--=--=--=--=--"
#define CODEBASE_VERSION_CBAR   AT_BLUE
#define CODEBASE_VERSION_COLOR  AT_WHITE

/* Stuff for area versions --Shaddai */
extern int              area_version;

#define HAS_SPELL_INDEX     -1
#define AREA_VERSION_WRITE 7

/* Command logging types. */
typedef enum
{
    LOG_NORMAL, LOG_ALWAYS, LOG_NEVER, LOG_BUILD, LOG_HIGH, LOG_COMM,
    LOG_WARN, LOG_ALL
} log_types;

struct location_data
{
    LOCATION               *next,
                           *prev;
    char                   *name;
    int                     vnum;
};

/* short cut crash bug fix provided by gfinello@mail.karmanet.it*/
typedef enum
{
    relMSET_ON, relOSET_ON
} relation_type;

typedef struct rel_data REL_DATA;

struct rel_data
{
    void                   *Actor;
    void                   *Subject;
    REL_DATA               *next;
    REL_DATA               *prev;
    relation_type           Type;
};

/* Return types for move_char, damage, greet_trigger, etc, etc
 * Added by Thoric to get rid of bugs
 */
typedef enum
{
    rNONE, rCHAR_DIED, rVICT_DIED, rBOTH_DIED, rCHAR_QUIT, rVICT_QUIT,
    rBOTH_QUIT, rSPELL_FAILED, rOBJ_SCRAPPED, rOBJ_EATEN, rOBJ_EXPIRED,
    rOBJ_TIMER, rOBJ_TRASHED, rOBJ_QUAFFED, rOBJ_USED, rOBJ_EXTRACTED,
    rOBJ_DRUNK, rCHAR_IMMUNE, rVICT_IMMUNE,
    rCHAR_AND_OBJ_EXTRACTED = 128,
    rERROR = 255
} ret_types;

/* Echo types for echo_to_all */
#define ECHOTAR_ALL  0
#define ECHOTAR_PC  1
#define ECHOTAR_IMM  2

/* defines for new do_who */
#define WT_MORTAL  0
#define WT_DEADLY  1
#define WT_IMM  2
#define WT_GROUPED  3
#define WT_GROUPWHO  4

/* Defines for extended bitvectors */
#ifndef INTBITS
#define INTBITS  32
#endif
#define XBM  31                                        /* extended bitmask (INTBITS - 1) */
#define RSV  5                                         /* right-shift value (sqrt(XBM+1)) 
                                                        */
#define XBI  4                                         /* integers in an extended
                                                        * bitvector */
#define MAX_BITS  XBI * INTBITS

/* Structure for extended bitvectors -- Thoric */
struct extended_bitvector
{
    unsigned int            bits[XBI];                 /* Needs to be unsigned to compile 
                                                        * in Redhat 6 - Samson */
};

// #include "weather.h"
#include "calendar.h"                                  /* AFKMud Calendar Replacement -
                                                        * Samson */
#include "weather.h"

#ifdef I3
#include "i3.h"
#endif

typedef enum
{
    vtNONE, vtINT, vtXBIT, vtSTR
} variable_types;

/*
 * Variable structure used for putting variable tags on players, mobs
 * or anything else.  Will be persistant (save) for players.
 */
struct variable_data
{
    VARIABLE_DATA          *next;
    char                    type;                      /* type of data */
    int                     flags;                     /* flags for future use */
    int                     vnum;                      /* vnum of mob that set this */
    time_t                  c_time;                    /* time created */
    time_t                  m_time;                    /* time last modified */
    time_t                  r_time;                    /* time last read */
    time_t                  expires;                   /* expiry date */
    int                     timer;                     /* expiry timer */
    char                   *tag;                       /* variable name */
    void                   *data;                      /* data pointer */
};

/* do_who output structure -- Narn */
struct who_data
{
    WHO_DATA               *prev;
    WHO_DATA               *next;
    char                   *text;
    int                     type;
};

#define is_full_name is_name

/*
 * Player watch data structure  --Gorog
 */
struct watch_data
{
    WATCH_DATA             *next;
    WATCH_DATA             *prev;
    short                   imm_level;
    char                   *imm_name;                  /* imm doing the watching */
    char                   *target_name;               /* player or command being watched 
                                                        */
    char                   *player_site;               /* site being watched */
};

/* Nuisance structure */
#define MAX_NUISANCE_STAGE 10                          /* How many nuisance stages */
struct nuisance_data
{
    long int                time;                      /* The time nuisance flag was set */
    long int                max_time;                  /* Time for max penalties */
    int                     flags;                     /* Stage of nuisance */
    int                     power;                     /* Power of nuisance */
};

/* Yeesh.. remind us of the old MERC ban structure? :) */
struct reserve_data
{
    RESERVE_DATA           *next;
    RESERVE_DATA           *prev;
    char                   *name;
};

/* Currency stuff */
typedef enum
{
    CURR_NONE = 0, CURR_GOLD, CURR_SILVER, CURR_BRONZE, CURR_COPPER,
    MAX_CURR_TYPE
} currency_types;

#define FIRST_CURR   CURR_NONE+1
#define LAST_CURR    MAX_CURR_TYPE-1
#define DEFAULT_CURR CURR_GOLD

#define GET_MONEY(ch,type)   ((ch)->money[(type)])
#define GET_VALUE(obj,type) (obj->currtype)

#define GET_TITLE(ch)           ((ch)->pcdata->title)

#define GET_POS(ch)             ((ch)->position)

typedef struct race_type RACE_TYPE;

#define MAX_RACE      18
#define MAX_NPC_RACE  92
#define GET_RACE(ch) (race_table[ch->race]->race_name)
#define GET_CLASS(ch) (class_table[ch->class]->who_name)
#define MAX_DEITY 10

/* the races */
typedef enum
{
    RACE_HUMAN, RACE_ELF, RACE_DWARF, RACE_HALFLING, RACE_PIXIE, RACE_OGRE,
    RACE_ORC, RACE_TROLL, RACE_SHADE, RACE_GOBLIN, RACE_DROW, RACE_GNOME,
    RACE_CENTAUR, RACE_DRAGON, RACE_CELESTIAL, RACE_VAMPIRE, RACE_DEMON,
    RACE_MINDFLAYER
} race_types;

/* npc races */
#define RACE_HUMAN          0
#define RACE_ELF            1
#define RACE_DWARF          2
#define RACE_HALFLING       3
#define RACE_PIXIE          4
#define RACE_OGRE           5
#define RACE_ORC            6
#define RACE_TROLL          7
#define RACE_SHADE          8
#define RACE_GOBLIN         9
#define RACE_DROW           10
#define RACE_GNOME          11
#define RACE_CENTAUR        12
#define RACE_DRAGON         13
#define RACE_CELESTIAL      14
#define RACE_VAMPIRE        15
#define RACE_DEMON          16
#define RACE_MINDFLAYER     17

/* race dedicated stuff */
struct race_type
{
    char                    race_name[16];             /* Race name */
    EXT_BV                  affected;                  /* Default affect bitvectors */
    EXT_BV                  flags;                     /* Special flags for Advanced
                                                        * races etc */
    short                   base_str;                  /* Str bonus/penalty */
    short                   base_dex;                  /* Dex " */
    short                   base_wis;                  /* Wis " */
    short                   base_int;                  /* Int " */
    short                   base_con;                  /* Con " */
    short                   base_cha;                  /* Cha " */
    short                   base_lck;                  /* Lck " */
    int                     hit;
    int                     mana;
    int                     resist;
    int                     suscept;
    int                     language;                  /* Default racial language */
    int                     sdbonus;
    short                   ac_plus;
    short                   alignment;
    EXT_BV                  attacks;
    EXT_BV                  defenses;
    short                   minalign;
    short                   maxalign;
    short                   exp_multiplier;
    short                   height;
    short                   weight;
    short                   hunger_mod;
    short                   thirst_mod;
    short                   saving_poison_death;
    short                   saving_wand;
    short                   saving_para_petri;
    short                   saving_breath;
    short                   saving_spell_staff;
    char                   *where_name[MAX_WHERE_NAME];
    short                   mana_regen;
    short                   hp_regen;
    short                   race_recall;
};

extern const struct race_type _race_table[MAX_RACE];
extern struct race_type *race_table[MAX_RACE];
extern const char      *const npc_race[];

typedef enum
{
    RACE_ADVANCED, MAX_RACE_FLAG
}
race_flags;

/*
 * Time and weather stuff.
 */
typedef enum
{
    SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET
} sun_positions;

typedef enum
{
    SKY_CLOUDLESS, SKY_CLOUDY, SKY_RAINING, SKY_LIGHTNING
} sky_conditions;

struct time_info_data
{
    int                     hour;
    int                     day;
    int                     month;
    int                     year;
    int                     sunlight;
    int                     season;                    /* Samson 5-6-99 */
};

struct hour_min_sec
{
    int                     hour;
    int                     min;
    int                     sec;
    int                     manual;
};

/* Structure used to build stafflist */
struct staffent
{
    STAFFENT               *next;
    STAFFENT               *last;
    char                   *name;
    short                   level;
};

struct idName
{
    char                   *name;
    long                    id;
    struct idName          *next;
};

/* Structure to only allow immortals domains to access their chars. */
struct immortal_host
{
    IMMORTAL_HOST          *next;
    IMMORTAL_HOST          *prev;
    char                   *name;
    char                   *host;
    bool                    prefix;
    bool                    suffix;
};

/* Connected state for a channel. */
typedef enum
{
    CON_PLAYING = 0, CON_ACCOUNT_MENU = -99,
    CON_GET_NAME, CON_GET_OLD_PASSWORD,
    CON_CONFIRM_NEW_NAME, CON_GET_NEW_PASSWORD, CON_CONFIRM_NEW_PASSWORD,
    CON_GET_NEW_SEX, CON_GET_NEW_RACE, CON_GET_NEW_CLASS,
    CON_GET_FIRST_CHOICE, CON_GET_SECOND_CHOICE,
    CON_GET_SECOND_CLASS, CON_GET_THIRD_CLASS, CON_PRESS_ENTER,
    CON_EDITING, CON_GET_ANSI,
    CON_ROLL_STATS, CON_COPYOVER_RECOVER,
    CON_PLOADED, CON_DELETE, CON_NOTE_TO,
    CON_NOTE_SUBJECT, CON_NOTE_EXPIRE, CON_MENU,
    CON_ENTER_MENU, CON_ENTER_GAME,
    CON_REDIT, CON_OEDIT, CON_MEDIT, CON_GET_BLIND
} connection_types;

/* Character substates */
typedef enum
{
    SUB_NONE, SUB_PAUSE, SUB_PERSONAL_DESC, SUB_BAN_DESC, SUB_OBJ_SHORT,
    SUB_OBJ_LONG, SUB_OBJ_EXTRA, SUB_MOB_LONG, SUB_MOB_DESC, SUB_ROOM_DESC,
    SUB_ROOM_EXTRA, SUB_ROOM_EXIT_DESC, SUB_WRITING_NOTE, SUB_MPROG_EDIT,
    SUB_HELP_EDIT, SUB_WRITING_MAP, SUB_PERSONAL_BIO, SUB_REPEATCMD,
    SUB_RESTRICTED, SUB_DEITYDESC, SUB_MORPH_DESC, SUB_MORPH_HELP, SUB_QUEST_DESC,
    SUB_PROJ_DESC, SUB_SLAYCMSG, SUB_SLAYVMSG, SUB_SLAYRMSG, SUB_GNOTE,
    SUB_BUG_DESC, SUB_BUG_FIXDESC, SUB_IDEA_DESC, SUB_IDEA_USEDESC,
    SUB_TYPO_DESC,

    /*
     * timer types ONLY below this point 
     */
    SUB_TIMER_DO_ABORT = 128, SUB_TIMER_CANT_ABORT
} char_substates;

/* Descriptor (channel) structure. */
struct descriptor_data
{
    DESCRIPTOR_DATA        *next;
    DESCRIPTOR_DATA        *prev;
    DESCRIPTOR_DATA        *snoop_by;
    CHAR_DATA              *character;
    CHAR_DATA              *original;
    char                   *host;
    int                     port;
#if defined(WIN32)
    SOCKET                  descriptor;
#else
    int                     descriptor;
#endif
    short                   connected;
    short                   idle;
    short                   lines;
    short                   scrlen;
    short                   mxp;
    bool                    fcommand;
    char                    inbuf[MAX_INBUF_SIZE];
    char                    incomm[MIL];
    char                    inlast[MIL];
    int                     repeat;
    char                   *outbuf;
    unsigned long           outsize;
    int                     outtop;
    char                   *pagebuf;
    unsigned long           pagesize;
    int                     pagetop;
    char                   *pagepoint;
    char                    pagecmd;
    char                    pagecolor;
    int                     newstate;
    unsigned char           prevcolor;
    bool                    mxp_detected;
    short                   speed;                     /* descriptor speed settings */
    struct mccp_data       *mccp;                      /* Mud Client Compression Protocol 
                                                        */
    bool                    can_compress;
    OLC_DATA               *olc;
    bool                    link_dead;
};

/* Attribute bonus structures. */
struct str_app_type
{
    short                   tohit;
    short                   todam;
    short                   carry;
    short                   wield;
};

struct int_app_type
{
    short                   learn;
};

struct wis_app_type
{
    short                   practice;
};

struct dex_app_type
{
    short                   defensive;
};

struct con_app_type
{
    short                   hitp;
    short                   shock;
};

struct cha_app_type
{
    short                   charm;
    short                   reaction;
};

struct lck_app_type
{
    short                   luck;
};

/* TO types for act. */
typedef enum
{ TO_ROOM, TO_NOTVICT, TO_VICT, TO_CHAR, TO_THIRD, TO_CANSEE } to_types;

/* Real action "TYPES" for act. */
#define AT_COLORIZE     -1                             /* Color sequence to interpret
                                                        * color codes */

#define INIT_WEAPON_CONDITION    12
#define MAX_ITEM_IMPACT   30

/* Help table types. */
struct help_data
{
    HELP_DATA              *next;
    HELP_DATA              *prev;
    short                   level;
    char                   *keyword;
    char                   *text;
    time_t                  modified_time;
};

/* Mob program structures and defines */
/* Moved these defines here from mud_prog.c as I need them -rkb */
#define MAX_IFS 20                                     /* should always be generous */
#define IN_IF 0
#define IN_ELSE 1
#define DO_IF 2
#define DO_ELSE 3

#define MAX_PROG_NEST 20

struct act_prog_data
{
    struct act_prog_data   *next;
    void                   *vo;
};

struct mob_prog_act_list
{
    MPROG_ACT_LIST         *next;
    char                   *buf;
    CHAR_DATA              *ch;
    OBJ_DATA               *obj;
    void                   *vo;
};

struct mob_prog_data
{
    MPROG_DATA             *next;
    short                   type;
    bool                    triggered;
    int                     resetdelay;
    char                   *arglist;
    char                   *comlist;
};

/* Used to store pauseing mud progs. -rkb */
typedef enum
{ MP_MOB, MP_ROOM, MP_OBJ } mp_types;
struct mppause_data
{
    MPPAUSE_DATA           *next;
    MPPAUSE_DATA           *prev;

    int                     timer;                     /* Pulses to pause */
    mp_types                type;                      /* Mob, Room or Obj prog */
    ROOM_INDEX_DATA        *room;                      /* Room when type is MP_ROOM */

    /*
     * mprog_driver state variables 
     */
    int                     ignorelevel;
    int                     iflevel;
    bool                    ifstate[MAX_IFS][DO_ELSE + 1];

    /*
     * mprog_driver arguments 
     */
    char                   *com_list;
    CHAR_DATA              *mob;
    CHAR_DATA              *actor;
    OBJ_DATA               *obj;
    void                   *vo;
    bool                    single_step;
};

extern bool             MOBtrigger;

/* Per-class stuff. */
struct class_type
{
    char                   *who_name;                  /* Name for 'who' */
    char                   *filename;
    EXT_BV                  affected;
    short                   attr_prime;                /* Prime attribute */
    short                   attr_second;               /* Second attribute */
    short                   attr_deficient;            /* Deficient attribute */
    int                     race_restriction;          /* Flags for illegal races -Lews */
    int                     combo_restriction;         // Taon
    int                     reclass1;                  /* What can this class reclass to */
    int                     reclass2;
    int                     reclass3;
    int                     resist;
    int                     suscept;
    short                   skill_adept;               /* Maximum skill level */
    short                   thac0_00;                  /* Thac0 for level 0 */
    short                   thac0_32;                  /* Thac0 for level 32 */
    short                   hp_min;                    /* Min hp gained on leveling */
    short                   hp_max;                    /* Max hp gained on leveling */
    short                   mana_min;
    short                   mana_max;                  /* Class gains mana on level */
    short                   exp_base;                  /* Class base exp */
    short                   craft_base;                /* Craft base exp */
    bool                    starting;                  /* Starting class or not */
};

struct at_color_type
{
    char                   *name;
    short                   def_color;
};

/* An affect.
 *
 * So limited... so few fields... should we add more?
 */
struct affect_data
{
    AFFECT_DATA            *next;
    AFFECT_DATA            *prev;
    short                   type;
    int                     duration;
    short                   location;
    int                     modifier;
    int                     level;
    EXT_BV                  bitvector;
};

/*
 * A SMAUG spell
 */
struct smaug_affect
{
    SMAUG_AFF              *next;
    char                   *duration;
    short                   location;
    char                   *modifier;
    int                     bitvector;                 /* this is the bit number */
};

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_PREY            41035
#define MOB_VNUM_DEITY		   17
#define MOB_VNUM_CITYGUARD       3060
#define MOB_VNUM_VAMPIRE         80
#define MOB_VNUM_ANIMATED_CORPSE 5
#define MOB_VNUM_ANIMATED_SKEL   6
#define MOB_VNUM_POLY_WOLF       10
#define MOB_VNUM_POLY_MIST       11
#define MOB_VNUM_POLY_BAT        12
#define MOB_VNUM_POLY_HAWK       13
#define MOB_VNUM_POLY_CAT        14
#define MOB_VNUM_POLY_DOVE       15
#define MOB_VNUM_POLY_FISH       16
#define MOB_VNUM_TANZEANAL       30036
#define MOB_VNUM_WIZARD_EYE      30037                 /* For the wizard eye spell */
#define MOB_VNUM_PASSAGE         30048                 /* For the passage skill */
#define MOB_VNUM_IMP             11203                 /* Evil homeland tutorial mob */
#define MOB_VNUM_LSKELLIE        41036
#define MOB_VNUM_SOLDIERS        41038
#define MOB_VNUM_ARCHERS         41039

/* Teacher vnums */
/* Basic Teachers 13 total One for each homeland */
#define MOB_VNUM_THALION         21499                 /* Paleon City - Humans */
#define MOB_VNUM_RAEAKAN         27009                 /* Valnaes - Teacher */
#define MOB_VNUM_CHROMATIC       40004                 /* Drakkel Shadar Dragon Teacher */
#define MOB_VNUM_ORTHA           30025                 /* Demon Homeland Teacher */
#define MOB_VNUM_DRATH           31000                 /* Celestial homeland teacher */
#define MOB_VNUM_DWARF           31378                 /* Manoake Dwarven homeland
                                                        * teacher */

#define MOB_VNUM_MARLENA         1254                  /* fake player */

/* Mid-level Teachers */
#define MOB_VNUM_DRAGON          26001                 /* Nasty red dragon north of
                                                        * Paleon City */
#define MOB_VNUM_MAZEKEEPER      6516                  /* Dwarven Area */
#define MOB_VNUM_CHIEF           1505                  /* Gnome Srefuge area */
#define MOB_VNUM_FROGHEMOTH      6117                  /* Haon Area */
#define MOB_VNUM_GOBLINKING      3507                  /* Mount Doom Goblin area */
#define MOB_VNUM_NYCADAEMON      14017                 /* Demon in Drow City */

/* High-level Teachers */
#define MOB_VNUM_REDDRAGON       7040

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
typedef enum
{
    ACT_IS_NPC, ACT_SENTINEL, ACT_SCAVENGER,
    ACT_WATER, ACT_NOQUEST, ACT_AGGRESSIVE,
    ACT_STAY_AREA, ACT_WIMPY, ACT_PET,
    ACT_BANKER, ACT_PRACTICE, ACT_IMMORTAL,
    ACT_DEADLY, ACT_POLYSELF, ACT_WILD_AGGR,
    ACT_GUARDIAN, ACT_RUNNING, ACT_NOWANDER,
    ACT_MOUNTABLE, ACT_MOUNTED, ACT_SCHOLAR,
    ACT_SECRETIVE, ACT_HARDHAT, ACT_MOBINVIS,
    ACT_NOASSIST, ACT_AUTONOMOUS, ACT_PACIFIST,
    ACT_NOATTACK, ACT_ANNOYING, ACT_STATSHIELD,
    ACT_PROTOTYPE, ACT_HEALER, ACT_CARRY,
    ACT_GRALLOC, ACT_GROUP, ACT_QUESTMASTER,
    ACT_UNDERTAKER, ACT_OVERRIDE, ACT_ACLAN_LEADER,
    ACT_HCLAN_LEADER, ACT_PLAYER, ACT_NEWBPRACTICE,
    ACT_RESIZER, ACT_NCLAN_LEADER, ACT_QUESTGIVER,
    ACT_SIGILIST, ACT_CRAFTSHOP, MAX_ACT
} act_types;

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
typedef enum
{
    AFF_BLINDNESS, AFF_INVISIBLE, AFF_DETECT_EVIL,
    AFF_DETECT_INVIS, AFF_DETECT_MAGIC, AFF_DETECT_HIDDEN,
    AFF_FASCINATE, AFF_SANCTUARY, AFF_FAERIE_FIRE,
    AFF_INFRARED, AFF_CURSE, AFF_SHIELD,
    AFF_POISON, AFF_PROTECT, AFF_PARALYSIS,
    AFF_SNEAK, AFF_HIDE, AFF_SLEEP,
    AFF_CHARM, AFF_FLYING, AFF_PASS_DOOR,
    AFF_FLOATING, AFF_TRUESIGHT, AFF_DETECTTRAPS,
    AFF_SCRYING, AFF_FIRESHIELD, AFF_SHOCKSHIELD,
    AFF_DETECT_SNEAK, AFF_ICESHIELD, AFF_POSSESS,
    AFF_BERSERK, AFF_AQUA_BREATH, AFF_RECURRINGSPELL,
    AFF_CONTAGIOUS, AFF_ACIDMIST, AFF_VENOMSHIELD,
    AFF_SHAPESHIFT, AFF_DEMONIC_SIGHT, AFF_NOSIGHT,
    AFF_SPIKE, AFF_UNHOLY_SPHERE, AFF_WIZARD_EYE,
    AFF_THAITIN, AFF_SHRINK, AFF_HEAVENS_BLESS,
    AFF_VAMPIRIC_STRENGTH, AFF_SURREAL_SPEED, AFF_BURROW,
    AFF_AGE, AFF_SILENCE, AFF_FEIGN, AFF_MANA_POOL, AFF_MAIM,
    AFF_REACTIVE, AFF_NETTLE, AFF_SLOW, AFF_WARD,
    AFF_ANOINT, AFF_FURY, AFF_BOOST, AFF_PRAYER,
    AFF_KINETIC,
    AFF_UNSEARING_SKIN,
    AFF_IRON_SKIN, AFF_DEFENSIVE_POSTURING, AFF_RECOIL, AFF_SIDESTEP,
    AFF_THICKEN_SKIN, AFF_ROOT, AFF_KEEN_EYE, AFF_MURAZOR, AFF_BODY,
    AFF_STIRRING, AFF_SUSTAIN_SELF, AFF_SNARE, AFF_RITUAL,
    AFF_SIPHON_STRENGTH, AFF_DECREE_DECAY, AFF_GIANT_STRENGTH,
    AFF_WIZARD_SIGHT, AFF_GRENDALS_STANCE, AFF_AURA_LIFE,
    AFF_ACIDIC_TOUCH, AFF_TANGLED, AFF_FLAMING_SHIELD,
    AFF_DIVINE_INTERVENTION, AFF_ENERGY_CONTAINMENT,   // afflictions
    AFF_FUNGAL_TOXIN, AFF_ASTHMA, AFF_CLUMSINESS, AFF_CONFUSION, AFF_DIZZINESS,
    AFF_NAUSEA,
    AFF_MIGRAINE, AFF_FROSTBITE, AFF_ROTTEN_GUT, AFF_BRITTLE_BONES,
    AFF_SWELLING, AFF_BLISTERING, AFF_NEUROTOXIN, AFF_TOXIN, AFF_CORROSIVE,
    AFF_VENOM_TOXIN, AFF_HIGHER_MAGIC,
    AFF_PTALONS, AFF_BATTLEFIELD, AFF_DISEASED, AFF_DRAGONLORD, AFF_HATE,
    AFF_CRAFTED_FOOD1, AFF_CRAFTED_FOOD2, AFF_CRAFTED_FOOD3,
    AFF_CRAFTED_DRINK1, AFF_CRAFTED_DRINK2, AFF_CRAFTED_DRINK3, AFF_NOSCENT, AFF_VICTIM,
    AFF_GRAZE,
    AFF_GRAFT, AFF_OTTOS_DANCE,
    AFF_SOUND_WAVES, AFF_MOUNTED,
    AFF_PHASE, AFF_TANGLE, AFF_R119, AFF_R120, AFF_R121, AFF_R122, AFF_R123,
    AFF_R124, AFF_R125,
    AFF_R126, AFF_R127,
    MAX_AFFECTED_BY
} affected_by_types;

/* Resistant Immune Susceptible flags */
#define RIS_FIRE         BV00                          /* 1 */
#define RIS_COLD         BV01                          /* 2 */
#define RIS_ELECTRICITY  BV02                          /* 4 */
#define RIS_ENERGY       BV03                          /* 8 */
#define RIS_BLUNT        BV04                          /* 16 */
#define RIS_PIERCE       BV05                          /* 32 */
#define RIS_SLASH        BV06                          /* 64 */
#define RIS_ACID         BV07                          /* 128 */
#define RIS_POISON       BV08                          /* 256 */
#define RIS_DRAIN        BV09                          /* 512 */
#define RIS_SLEEP        BV10                          /* 1024 */
#define RIS_CHARM        BV11                          /* 2048 */
#define RIS_HOLD         BV12                          /* 4096 */
#define RIS_NONMAGIC     BV13                          /* 8192 */
#define RIS_PLUS1        BV14                          /* 16384 */
#define RIS_PLUS2        BV15                          /* 32768 */
#define RIS_PLUS3        BV16                          /* 65536 */
#define RIS_PLUS4        BV17                          /* 131072 */
#define RIS_PLUS5        BV18                          /* 262144 */
#define RIS_PLUS6        BV19                          /* 524288 */
#define RIS_MAGIC        BV20                          /* 1048576 */
#define RIS_PARALYSIS    BV21
#define RIS_HACK         BV22
#define RIS_LASH         BV23
#define RIS_HIT          BV24
#define RIS_R4           BV25
#define RIS_R5           BV26
#define RIS_R6           BV27
#define RIS_R7           BV28
#define RIS_R8           BV29
#define RIS_R9           BV30
#define RIS_R10          BV31

/* Attack types */
typedef enum
{
    ATCK_BITE, ATCK_CLAWS, ATCK_TAIL_SWIPE, ATCK_STING, ATCK_PUNCH, ATCK_KICK,
    ATCK_TRIP, ATCK_BASH, ATCK_STUN, ATCK_GOUGE, ATCK_BACKSTAB, ATCK_FEED,
    ATCK_DRAIN, ATCK_FIREBREATH, ATCK_FROSTBREATH, ATCK_ACIDBREATH,
    ATCK_LIGHTNBREATH, ATCK_GASBREATH, ATCK_POISON, ATCK_NASTYPOISON, ATCK_GAZE,
    ATCK_BLINDNESS, ATCK_CAUSESERIOUS, ATCK_EARTHQUAKE, ATCK_CAUSECRITICAL,
    ATCK_CURSE, ATCK_FLAMESTRIKE, ATCK_HARM, ATCK_FIREBALL, ATCK_COLORSPRAY,
    ATCK_WEAKEN, ATCK_SPIRALBLAST, ATCK_INFERNO, ATCK_BLIZZARD, ATCK_EBOLT,
    ATCK_BLIGHTNING, ATCK_BRAINBOIL, ATCK_GSMITE, ATCK_HHANDS, ATCK_LTOUCH,
    MAX_ATTACK_TYPE
} attack_types;

/*
 * Defense types
 */
typedef enum
{
    DFND_PARRY, DFND_DODGE, DFND_HEAL, DFND_MINORHEALING, DFND_ARCHHEALING,
    DFND_SYLVANWIND, DFND_DISPELMAGIC, DFND_DISPELEVIL, DFND_SANCTUARY,
    DFND_FIRESHIELD, DFND_SHOCKSHIELD, DFND_SHIELD, DFND_BLESS, DFND_STONESKIN,
    DFND_TELEPORT, DFND_ACOMPANION, DFND_LSKELLIE, DFND_CELEMENTAL, DFND_SWARD,
    DFND_DISARM, DFND_ICESHIELD, DFND_GRIP, DFND_TRUESIGHT, DFND_ACIDMIST,
    DFND_VENOMSHIELD, DFND_BESTOWVITAE, DFND_PHASE,
    DFND_DISPLACEMENT, DFND_GHEAL, DFND_COUNTERSTRIKE, DFND_BLADEMASTER,
    DFND_DBLESS, DFND_R13, MAX_DEFENSE_TYPE
} defense_types;

/*
 * Body parts
 */
typedef enum
{
    PART_HEAD, PART_ARMS, PART_LEGS,
    PART_HEART, PART_BRAINS, PART_GUTS,
    PART_HANDS, PART_FEET, PART_FINGERS,
    PART_EAR, PART_EYE, PART_LONG_TONGUE,
    PART_EYESTALKS, PART_TENTACLES, PART_FINS,
    PART_WINGS, PART_TAIL, PART_SCALES,
    PART_CLAWS, PART_FANGS, PART_HORNS,
    PART_TUSKS, PART_TAILATTACK, PART_SHARPSCALES,
    PART_BEAK, PART_HAUNCH, PART_HOOVES,
    PART_PAWS, PART_FORELEGS, PART_FEATHERS,
    MAX_PARTS
} part_types;

/*
 * Autosave flags
 */
typedef enum
{
    SV_DEATH, SV_KILL, SV_PASSCHG,
    SV_DROP, SV_PUT, SV_GIVE,
    SV_AUTO, SV_ZAPDROP, SV_AUCTION,
    SV_GET, SV_RECEIVE, SV_IDLE,
    SV_BACKUP, SV_QUITBACKUP, SV_FILL,
    SV_EMPTY,
    MAX_SAVE
} save_flags;

extern const char      *obj_sizes[];

/*
 * Pipe flags
 */
#define PIPE_TAMPED    BV01
#define PIPE_LIT    BV02
#define PIPE_HOT    BV03
#define PIPE_DIRTY    BV04
#define PIPE_FILTHY    BV05
#define PIPE_GOINGOUT    BV06
#define PIPE_BURNT    BV07
#define PIPE_FULLOFASH    BV08

/*
 * Flags for act_string -- Shaddai
 */
#define STRING_NONE               0
#define STRING_IMM                BV01

/*
 * Skill/Spell flags  The minimum BV *MUST* be 11!
 */
typedef enum
{
    SF_WATER, SF_EARTH, SF_AIR,
    SF_ASTRAL, SF_AREA, SF_DISTANT,
    SF_REVERSE, SF_NOSELF, SF_HEAL,
    SF_ACCUMULATIVE, SF_RECASTABLE, SF_NOSCRIBE,
    SF_NOBREW, SF_GROUPSPELL, SF_OBJECT,
    SF_CHARACTER, SF_SECRETSKILL, SF_PKSENSITIVE,
    SF_STOPONFAIL, SF_NOFIGHT, SF_NODISPEL,
    SF_RANDOMTARGET,
    MAX_SKELL_FLAG
} skell_flags;

typedef enum
{
    SS_NONE, SS_POISON_DEATH, SS_ROD_WANDS,
    SS_PARA_PETRI, SS_BREATH, SS_SPELL_STAFF
} save_types;

#define ALL_BITS  INT_MAX
#define SDAM_MASK  ALL_BITS & ~(BV00 | BV01 | BV02)
#define SACT_MASK  ALL_BITS & ~(BV03 | BV04 | BV05)
#define SCLA_MASK  ALL_BITS & ~(BV06 | BV07 | BV08)
#define SPOW_MASK  ALL_BITS & ~(BV09 | BV10)
#define SSAV_MASK  ALL_BITS & ~(BV11 | BV12 | BV13)

typedef enum
{
    SD_NONE, SD_FIRE, SD_COLD,
    SD_ELECTRICITY, SD_ENERGY, SD_ACID,
    SD_POISON, SD_DRAIN
} spell_dam_types;

typedef enum
{
    SA_NONE, SA_CREATE, SA_DESTROY,
    SA_RESIST, SA_SUSCEPT, SA_DIVINATE,
    SA_OBSCURE, SA_CHANGE
} spell_act_types;

typedef enum
{
    SP_NONE, SP_MINOR, SP_GREATER,
    SP_MAJOR
} spell_power_types;

typedef enum
{
    SC_NONE, SC_LUNAR, SC_SOLAR,
    SC_TRAVEL, SC_SUMMON, SC_LIFE,
    SC_DEATH, SC_ILLUSION
} spell_class_types;

typedef enum
{
    SE_NONE, SE_NEGATE, SE_EIGHTHDAM,
    SE_QUARTERDAM, SE_HALFDAM, SE_3QTRDAM,
    SE_REFLECT, SE_ABSORB
} spell_save_effects;

/*
 * Sex.
 * Used in #MOBILES.
 */
typedef enum
{
    SEX_NEUTRAL, SEX_MALE, SEX_FEMALE
} sex_types;

typedef enum
{
    TRAP_TYPE_POISON_GAS = 1, TRAP_TYPE_POISON_DART, TRAP_TYPE_POISON_NEEDLE,
    TRAP_TYPE_POISON_DAGGER, TRAP_TYPE_POISON_ARROW, TRAP_TYPE_BLINDNESS_GAS,
    TRAP_TYPE_SLEEPING_GAS, TRAP_TYPE_FLAME, TRAP_TYPE_EXPLOSION,
    TRAP_TYPE_ACID_SPRAY, TRAP_TYPE_ELECTRIC_SHOCK, TRAP_TYPE_BLADE,
    TRAP_TYPE_SEX_CHANGE,
    MAX_TRAPTYPE
} trap_types;

#define TRAP_ROOM        BV00
#define TRAP_OBJ         BV01
#define TRAP_ENTER_ROOM  BV02
#define TRAP_LEAVE_ROOM  BV03
#define TRAP_OPEN        BV04
#define TRAP_CLOSE       BV05
#define TRAP_GET         BV06
#define TRAP_PUT         BV07
#define TRAP_PICK        BV08
#define TRAP_UNLOCK      BV09
#define TRAP_N           BV10
#define TRAP_S           BV11
#define TRAP_E           BV12
#define TRAP_W           BV13
#define TRAP_U           BV14
#define TRAP_D           BV15
#define TRAP_EXAMINE     BV16
#define TRAP_NE          BV17
#define TRAP_NW          BV18
#define TRAP_SE          BV19
#define TRAP_SW          BV20

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
// PALEON EQUIP
#define OBJ_VNUM_PCATAPULT          41008
#define OBJ_VNUM_PBALLISTA          41009
#define OBJ_VNUM_PPITCH             41010
#define OBJ_VNUM_PPLATFORM          41011
#define OBJ_VNUM_PRAM               41012
#define OBJ_VNUM_PARROWS            41023
// DAKAR EQUIP
#define OBJ_VNUM_DCATAPULT          41013
#define OBJ_VNUM_DBALLISTA          41014
#define OBJ_VNUM_DPITCH             41015
#define OBJ_VNUM_DPLATFORM          41016
#define OBJ_VNUM_DRAM               41017
#define OBJ_VNUM_DARROWS            41024
// FORBIDDEN EQUIP
#define OBJ_VNUM_FCATAPULT          41018
#define OBJ_VNUM_FBALLISTA          41019
#define OBJ_VNUM_FPITCH             41020
#define OBJ_VNUM_FPLATFORM          41021
#define OBJ_VNUM_FRAM               41022
#define OBJ_VNUM_FARROWS            41025

#define OBJ_VNUM_OPORTAL         63
#define OBJ_VNUM_DEITY           64
#define OBJ_VNUM_MONEY_ONE        2
#define OBJ_VNUM_MONEY_SOME       3
#define OBJ_VNUM_MONEY_LOTS     128
#define OBJ_VNUM_MONEY_HEAPS    129
#define OBJ_VNUM_MONEY_PILE     130
#define OBJ_VNUM_MONEY_MASS     131
#define OBJ_VNUM_MONEY_MILLS    132
#define OBJ_VNUM_MONEY_INF      133
#define OBJ_VNUM_RUNE            16285
#define OBJ_VNUM_VAULT           41005
#define OBJ_VNUM_SKELLY          41006

#define OBJ_VNUM_CORPSE_NPC       10
#define OBJ_VNUM_CORPSE_PC        11
#define OBJ_VNUM_SKELETON        140
#define OBJ_VNUM_SEVERED_HEAD     12
#define OBJ_VNUM_TORN_HEART       13
#define OBJ_VNUM_SLICED_ARM       14
#define OBJ_VNUM_SLICED_LEG       15
#define OBJ_VNUM_SPILLED_GUTS       16
#define OBJ_VNUM_BLOOD       17
#define OBJ_VNUM_BLOODSTAIN       18
#define OBJ_VNUM_SCRAPS       19
#define OBJ_VNUM_SUMMONLIGHT	77

#define OBJ_VNUM_FOOD1		92
#define OBJ_VNUM_FOOD2		97
//Used in create food to invoke a random meal  -  Volk
#define OBJ_VNUM_VOMIT		102
#define OBJ_VNUM_PICK           1298

/* Raw Materials */
#define OBJ_VNUM_GATHER        41003                   /* gathered food items */
#define OBJ_VNUM_CRAFTFOOD     41004                   /* crafted player food */
#define OBJ_VNUM_ORE           1239                    /* Metal Objects */
#define OBJ_VNUM_ORE2          1240                    /* Wooden Objects */
#define OBJ_VNUM_ORE3          1241                    /* CLOTHES */
#define OBJ_VNUM_ORE4          1242                    /* Jewlery */
#define OBJ_VNUM_GARDENS       1243                    /* Food */

/* End of Raw Materials */

#define OBJ_VNUM_LIGHT_BALL       21
#define OBJ_VNUM_SPRING       22

#define OBJ_VNUM_SKIN       23
#define OBJ_VNUM_SLICE       24
#define OBJ_VNUM_SHOPPING_BAG       25

#define OBJ_VNUM_BLOODLET       26

#define OBJ_VNUM_FIRE       30
#define OBJ_VNUM_TRAP       31
#define OBJ_VNUM_PORTAL       32

#define OBJ_VNUM_BLACK_POWDER       33
#define OBJ_VNUM_SCROLL_SCRIBING     34
#define OBJ_VNUM_FLASK_BREWING       35
#define OBJ_VNUM_NOTE       36

/* Academy eq */
#define OBJ_VNUM_SCHOOL_MACE    10315
#define OBJ_VNUM_SCHOOL_DAGGER    10312
#define OBJ_VNUM_SCHOOL_SWORD    10313
#define OBJ_VNUM_SCHOOL_VEST    10308
#define OBJ_VNUM_SCHOOL_SHIELD    10310
#define OBJ_VNUM_SCHOOL_BANNER    10311

#define OBJ_VNUM_FIREBALL         30016
#define OBJ_VNUM_JUDGE            30027
#define OBJ_VNUM_FLAMING_WHIP     1265
#define OBJ_VNUM_BONE             30032
#define OBJ_VNUM_BOOK             1224
/*
 * Item types.
 * Used in #OBJECTS.
 */
typedef enum
{
    ITEM_NONE, ITEM_LIGHT, ITEM_SCROLL,
    ITEM_WAND, ITEM_STAFF, ITEM_WEAPON,
    ITEM_FIREWEAPON, ITEM_MISSILE, ITEM_TREASURE,
    ITEM_ARMOR, ITEM_POTION, ITEM_WORN,
    ITEM_FURNITURE, ITEM_TRASH, ITEM_OLDTRAP,
    ITEM_CONTAINER, ITEM_NOTE, ITEM_DRINK_CON,
    ITEM_KEY, ITEM_FOOD, ITEM_MONEY,
    ITEM_PEN, ITEM_BOAT, ITEM_CORPSE_NPC,
    ITEM_CORPSE_PC, ITEM_FOUNTAIN, ITEM_PILL,
    ITEM_BLOOD, ITEM_BLOODSTAIN, ITEM_SCRAPS,
    ITEM_PIPE, ITEM_HERB_CON, ITEM_HERB,
    ITEM_INCENSE, ITEM_FIRE, ITEM_BOOK,
    ITEM_SWITCH, ITEM_LEVER, ITEM_PULLCHAIN,
    ITEM_BUTTON, ITEM_DIAL, ITEM_RUNE,
    ITEM_RUNEPOUCH, ITEM_MATCH, ITEM_TRAP,
    ITEM_MAP, ITEM_PORTAL, ITEM_PAPER,
    ITEM_TINDER, ITEM_LOCKPICK, ITEM_SPIKE,
    ITEM_DISEASE, ITEM_OIL, ITEM_FUEL,
    ITEM_PIECE, ITEM_THROWING, ITEM_MISSILE_WEAPON,
    ITEM_PROJECTILE, ITEM_QUIVER, ITEM_SHOVEL,
    ITEM_SALVE, ITEM_COOK, ITEM_KEYRING,
    ITEM_ODOR, ITEM_CHANCE, ITEM_SHARPEN,
    ITEM_SHACKLE, ITEM_RAW, ITEM_INSTRUMENT,
    ITEM_SKELETON, ITEM_RESOURCE, ITEM_DYE,
    ITEM_STONE, ITEM_TOOL, ITEM_STOVE, ITEM_COAL,
    ITEM_SHEATH, ITEM_SABOTAGE, ITEM_FORGE,
    MAX_ITEM_TYPE
} item_types;

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
typedef enum
{
    ITEM_GLOW, ITEM_HUM, ITEM_DARK,
    ITEM_LOYAL, ITEM_EVIL, ITEM_INVIS,
    ITEM_MAGIC, ITEM_NODROP, ITEM_BLESS,
    ITEM_ANTI_GOOD, ITEM_ANTI_EVIL, ITEM_ANTI_NEUTRAL,
    ITEM_NOREMOVE, ITEM_INVENTORY, ITEM_ANTI_MAGE,
    ITEM_ANTI_THIEF, ITEM_ANTI_WARRIOR, ITEM_ANTI_PRIEST,
    ITEM_ORGANIC, ITEM_METAL, ITEM_DONATION,
    ITEM_CLANOBJECT, ITEM_CLANCORPSE, ITEM_ANTI_VAMPIRE,
    ITEM_ANTI_DRUID, ITEM_HIDDEN, ITEM_POISONED,
    ITEM_COVERING, ITEM_R22, ITEM_BURIED,
    ITEM_PROTOTYPE, ITEM_NOLOCATE, ITEM_GROUNDROT,
    ITEM_LOOTABLE, ITEM_PKDISARMED, ITEM_LODGED,
    ITEM_SKELEROT, ITEM_OVERRIDE, ITEM_CLANCONTAINER,
    ITEM_NOGROUP, MAX_ITEM_FLAG
} item_extra_flags;

/* Lever/dial/switch/button/pullchain flags */
#define TRIG_UP   BV00
#define TRIG_UNLOCK  BV01
#define TRIG_LOCK  BV02
#define TRIG_D_NORTH  BV03
#define TRIG_D_SOUTH  BV04
#define TRIG_D_EAST  BV05
#define TRIG_D_WEST  BV06
#define TRIG_D_UP  BV07
#define TRIG_D_DOWN  BV08
#define TRIG_DOOR  BV09
#define TRIG_CONTAINER  BV10
#define TRIG_OPEN  BV11
#define TRIG_CLOSE  BV12
#define TRIG_PASSAGE  BV13
#define TRIG_OLOAD  BV14
#define TRIG_MLOAD  BV15
#define TRIG_TELEPORT  BV16
#define TRIG_TELEPORTALL  BV17
#define TRIG_TELEPORTPLUS  BV18
#define TRIG_DEATH  BV19
#define TRIG_CAST  BV20
#define TRIG_FAKEBLADE  BV21
#define TRIG_RAND4  BV22
#define TRIG_RAND6  BV23
#define TRIG_TRAPDOOR  BV24
#define TRIG_ANOTHEROOM  BV25
#define TRIG_USEDIAL  BV26
#define TRIG_ABSOLUTEVNUM  BV27
#define TRIG_SHOWROOMDESC  BV28
#define TRIG_AUTORETURN  BV29

typedef enum
{
    TELE_SHOWDESC, TELE_TRANSALL, TELE_TRANSALLPLUS,
    MAX_TELE_FLAG
} tele_flags;

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE               BV00
#define ITEM_WEAR_FINGER        BV01
#define ITEM_WEAR_NECK          BV02
#define ITEM_WEAR_BODY          BV03
#define ITEM_WEAR_HEAD          BV04
#define ITEM_WEAR_LEGS          BV05
#define ITEM_WEAR_FEET          BV06
#define ITEM_WEAR_HANDS         BV07
#define ITEM_WEAR_ARMS          BV08
#define ITEM_WEAR_SHIELD        BV09
#define ITEM_WEAR_ABOUT         BV10
#define ITEM_WEAR_WAIST         BV11
#define ITEM_WEAR_WRIST         BV12
#define ITEM_WIELD              BV13
#define ITEM_HOLD               BV14
#define ITEM_DUAL_WIELD         BV15
#define ITEM_WEAR_EARS          BV16
#define ITEM_WEAR_EYES          BV17
#define ITEM_MISSILE_WIELD      BV18
#define ITEM_WEAR_BACK          BV19
#define ITEM_WEAR_FACE          BV20
#define ITEM_WEAR_ANKLE         BV21
#define ITEM_LODGE_RIB          BV22
#define ITEM_LODGE_ARM          BV23
#define ITEM_LODGE_LEG          BV24
#define ITEM_WEAR_SHEATH        BV25
#define ITEM_WEAR_SHOULDERS     BV26
#define ITEM_WEAR_MAX           26

/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
typedef enum
{
    APPLY_NONE, APPLY_STR, APPLY_DEX,
    APPLY_INT, APPLY_WIS, APPLY_CON,
    APPLY_SEX, APPLY_CLASS, APPLY_RACE,
    APPLY_AGE, APPLY_HEIGHT, APPLY_WEIGHT,
    APPLY_MANA, APPLY_HIT, APPLY_MOVE,
    APPLY_R1, APPLY_R2, APPLY_AC,                      // R1 + R2
    APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_POISON,
    APPLY_SAVING_ROD, APPLY_SAVING_PARA, APPLY_SAVING_BREATH,
    APPLY_SAVING_SPELL, APPLY_CHA, APPLY_AFFECT,
    APPLY_RESISTANT, APPLY_IMMUNE, APPLY_SUSCEPTIBLE,
    APPLY_WEAPONSPELL, APPLY_LCK, APPLY_R3,            // R3
    APPLY_R4, APPLY_R5, APPLY_R6,                      // TO R6
    APPLY_R7, APPLY_R8, APPLY_R9,                      // TO R9
    APPLY_R10, APPLY_R11, APPLY_R12,
    APPLY_R13, APPLY_R14, APPLY_R15,
    APPLY_R16, APPLY_R17, APPLY_R18,                   // TO R18
    APPLY_R19, APPLY_R20, APPLY_R21,
    APPLY_R22, APPLY_R23, APPLY_R24,                   // TO R24
    APPLY_R25, APPLY_R26, APPLY_WEARSPELL,             // TO R26
    APPLY_REMOVESPELL, APPLY_EMOTION, APPLY_MENTALSTATE,
    APPLY_STRIPSN, APPLY_REMOVE, APPLY_R27,            // TO R27
    APPLY_FULL, APPLY_THIRST, APPLY_DRUNK,
    APPLY_BLOOD, APPLY_R28, APPLY_RECURRINGSPELL,      // TO R28
    APPLY_CONTAGIOUS, APPLY_EXT_AFFECT, APPLY_ODOR,
    APPLY_ROOMFLAG, APPLY_SECTORTYPE, APPLY_ROOMLIGHT,
    APPLY_TELEVNUM, APPLY_TELEDELAY, APPLY_R29,        // TO R29
    APPLY_R30, APPLY_R31, APPLY_PERCENTAGE,            // TO R31
    MAX_APPLY_TYPE
} apply_types;

#define REVERSE_APPLY     1000

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE     BV00
#define CONT_PICKPROOF     BV01
#define CONT_CLOSED     BV02
#define CONT_LOCKED     BV03
#define CONT_EATKEY     BV04

/*
 * Sitting/Standing/Sleeping/Sitting on/in/at Objects - Xerves
 * Used for furniture (value[2]) in the #OBJECTS Section
 */
#define SIT_ON     BV00
#define SIT_IN     BV01
#define SIT_AT     BV02

#define STAND_ON   BV03
#define STAND_IN   BV04
#define STAND_AT   BV05

#define SLEEP_ON   BV06
#define SLEEP_IN   BV07
#define SLEEP_AT   BV08

#define REST_ON     BV09
#define REST_IN     BV10
#define REST_AT     BV11

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO        2
#define ROOM_VNUM_POLY        3
#define ROOM_VNUM_HELL        6
#define ROOM_VNUM_CHAT     1200
#define ROOM_VNUM_TEMPLE    16007
#define ROOM_VNUM_ALTAR    16107
#define ROOM_VNUM_SCHOOL    16007
#define ROOM_VNUM_MUDSCHOOL  5109
#define ROOM_AUTH_START      5109
#define ROOM_AUTH_START2     5199
#define ROOM_AUTH_START3     5200
#define ROOM_AUTH_START4     33268
#define ROOM_AUTH_START5     33296
#define ROOM_AUTH_START6     33297
#define ROOM_AUTH_START7     19023
#define ROOM_AUTH_START8     19098
#define ROOM_AUTH_START9     19099
#define PALEON_SIEGE         3503
#define PALEON_SIEGE2        16142
#define PALEON_SIEGE3        16140
#define DAKAR_SIEGE          27013
#define DAKAR_SIEGE2         27006
#define DAKAR_SIEGE3         27012
#define FORBIDDEN_SIEGE      35509
#define FORBIDDEN_SIEGE2     35511
#define FORBIDDEN_SIEGE3     35512
#define PALEON_DEFEND        16019
#define PALEON_DEFEND2       16119
#define PALEON_DEFEND3       16018
#define DAKAR_DEFEND         11205
#define DAKAR_DEFEND2        11204
#define DAKAR_DEFEND3        11228
#define FORBIDDEN_DEFEND     35516
#define FORBIDDEN_DEFEND2    35537
#define FORBIDDEN_DEFEND3    35530

#define ROOM_VNUM_HALLOFFALLEN    16007
#define WILDERNESS_MIN_VNUM  41000
#define WILDERNESS_MAX_VNUM  44000
#define RIFT_MIN_VNUM 6500
#define RIFT_MAX_VNUM 6900
#define XWILD_MIN_VNUM  60000
#define XWILD_MAX_VNUM  62999
#define YWILD_MIN_VNUM  48001
#define YWILD_MAX_VNUM  53801
#define IN_WILDERNESS(ch) ((ch) && (ch)->in_room && (((ch)->in_room->vnum >= WILDERNESS_MIN_VNUM && (ch)->in_room->vnum <= WILDERNESS_MAX_VNUM ) || ((ch)->in_room->vnum >= XWILD_MIN_VNUM && (ch)->in_room->vnum <= XWILD_MAX_VNUM || (ch)->in_room->vnum >= YWILD_MIN_VNUM && (ch)->in_room->vnum <= YWILD_MAX_VNUM)))
#define IN_RIFT(ch)       ((ch) && (ch)->in_room && (ch)->in_room->vnum >= RIFT_MIN_VNUM && (ch)->in_room->vnum <= RIFT_MAX_VNUM )
#define WILD_MAP_ON(ch)   ((ch)->map_toggle == 0)
#define WILD_DESC_ON(ch)  ((ch)->map_desc_toggle == 0)
#define WILD_NAME_ON(ch)  ((ch)->map_name_toggle == 0)

void get_wilderness_name args( ( CHAR_DATA *ch ) );

/* Room flags. Holy cow!  Talked about stripped away..
 * Used in #ROOMS. Those merc guys know how to strip code down.
 *                 Lets put it all back... ;)
 */
#define ROOM_DARK          BV00
#define ROOM_DEATH         BV01
#define ROOM_NO_MOB        BV02
#define ROOM_INDOORS       BV03
#define ROOM_PET_STOREROOM BV04
#define ROOM_NOMISSILE     BV05
/* BV06 are unused currently */
#define ROOM_NO_MAGIC      BV07
#define ROOM_TUNNEL        BV08
#define ROOM_PRIVATE       BV09
#define ROOM_SAFE          BV10
#define ROOM_SOLITARY      BV11
#define ROOM_PET_SHOP      BV12
#define ROOM_NO_RECALL     BV13
#define ROOM_TRADESKILLS   BV14
#define ROOM_NODROPALL     BV15
#define ROOM_SILENCE       BV16
#define ROOM_LOGSPEECH     BV17
#define ROOM_NODROP        BV18
#define ROOM_CLANSTOREROOM BV19
#define ROOM_NO_SUMMON     BV20
#define ROOM_NO_ASTRAL     BV21
#define ROOM_TELEPORT      BV22
#define ROOM_TELESHOWDESC  BV23
#define ROOM_NOFLOOR       BV24
#define ROOM_NOSUPPLICATE  BV25
#define ROOM_ARENA         BV26
#define ROOM_LIGHTED       BV27
#define ROOM_BANK          BV28
#define ROOM_QUEST         BV29
#define ROOM_PROTOTYPE     BV30
#define ROOM_DND           BV31
/* You can't go above BV31 */

/* Directions.
 * Used in #ROOMS.
 */
typedef enum
{
    DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST,
    DIR_UP, DIR_DOWN, DIR_NORTHEAST, DIR_NORTHWEST,
    DIR_SOUTHEAST, DIR_SOUTHWEST, DIR_SOMEWHERE, DIR_EXPLORE
} dir_types;

#define PT_WATER  100
#define PT_AIR  200
#define PT_EARTH  300
#define PT_FIRE  400

/*
 * Push/pull types for exits     -Thoric
 * To differentiate between the current of a river, or a strong gust of wind
 */
typedef enum
{
    PULL_UNDEFINED, PULL_VORTEX, PULL_VACUUM, PULL_SLIP, PULL_ICE,
    PULL_MYSTERIOUS,
    PULL_CURRENT = PT_WATER, PULL_WAVE, PULL_WHIRLPOOL, PULL_GEYSER,
    PULL_WIND = PT_AIR, PULL_STORM, PULL_COLDWIND, PULL_BREEZE,
    PULL_LANDSLIDE = PT_EARTH, PULL_SINKHOLE, PULL_QUICKSAND, PULL_EARTHQUAKE,
    PULL_LAVA = PT_FIRE, PULL_HOTAIR
} dir_pulltypes;

#define MAX_DIR   DIR_SOUTHWEST                        /* max for normal walking */
#define DIR_PORTAL  DIR_SOMEWHERE                      /* portal direction */

/*
 * Exit flags.   EX_RES# are reserved for use by the
 * Used in #ROOMS.  SMAUG development team
 */
#define EX_ISDOOR       BV00
#define EX_CLOSED       BV01
#define EX_LOCKED       BV02
#define EX_SECRET       BV03
#define EX_SWIM         BV04
#define EX_PICKPROOF    BV05
#define EX_FLY          BV06
#define EX_CLIMB        BV07
#define EX_DIG          BV08
#define EX_EATKEY       BV09
#define EX_NOPASSDOOR   BV10
#define EX_HIDDEN       BV11
#define EX_PASSAGE      BV12
#define EX_PORTAL       BV13
#define EX_RES1         BV14
#define EX_RES2         BV15
#define EX_xCLIMB       BV16
#define EX_xENTER       BV17
#define EX_xLEAVE       BV18
#define EX_xAUTO        BV19
#define EX_NOFLEE       BV20
#define EX_xSEARCHABLE  BV21
#define EX_BASHED       BV22
#define EX_BASHPROOF    BV23
#define EX_NOMOB        BV24
#define EX_WINDOW       BV25
#define EX_xLOOK        BV26
#define EX_ISBOLT       BV27
#define EX_BOLTED       BV28
#define EX_AUTOFULL     BV29
#define MAX_EXFLAG    29

/*
 * Sector types.
 * Used in #ROOMS.
 */
typedef enum
{
    SECT_INSIDE, SECT_ROAD, SECT_FIELD, SECT_FOREST,
    SECT_HILLS, SECT_MOUNTAIN, SECT_WATER_SWIM, SECT_WATER_NOSWIM,
    SECT_UNDERWATER, SECT_AIR, SECT_DESERT, SECT_AREA_ENT,
    SECT_OCEANFLOOR, SECT_UNDERGROUND, SECT_LAVA, SECT_SWAMP,
    SECT_CITY, SECT_VROAD, SECT_HROAD, SECT_OCEAN,
    SECT_JUNGLE, SECT_GRASSLAND, SECT_CROSSROAD, SECT_THICKFOREST,
    SECT_HIGHMOUNTAIN, SECT_ARCTIC, SECT_WATERFALL, SECT_RIVER,
    SECT_DOCK, SECT_LAKE, SECT_CAMPSITE, SECT_PORTALSTONE,
    SECT_DEEPMUD, SECT_QUICKSAND, SECT_PASTURELAND, SECT_VALLEY,
    SECT_MOUNTAINPASS, SECT_BEACH, SECT_FOG, SECT_NOCHANGE,
    SECT_SKY, SECT_CLOUD, SECT_SNOW, SECT_ORE,
    SECT_MAX
} sector_types;

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
typedef enum
{
    WEAR_NONE = -1, WEAR_LIGHT = 0, WEAR_FINGER_L, WEAR_FINGER_R, WEAR_NECK_1,
    WEAR_NECK_2, WEAR_BODY, WEAR_HEAD, WEAR_LEGS, WEAR_FEET, WEAR_HANDS,
    WEAR_ARMS, WEAR_SHIELD, WEAR_ABOUT, WEAR_WAIST, WEAR_WRIST_L, WEAR_WRIST_R,
    WEAR_WIELD, WEAR_HOLD, WEAR_DUAL_WIELD, WEAR_EARS, WEAR_EYES,
    WEAR_MISSILE_WIELD, WEAR_BACK, WEAR_FACE, WEAR_ANKLE_L, WEAR_ANKLE_R,
    WEAR_LODGE_RIB, WEAR_LODGE_ARM, WEAR_LODGE_LEG, WEAR_SHEATH, WEAR_SHOULDERS,
    MAX_WEAR
} wear_locations;

/* Auth Flags */
#define FLAG_WRAUTH        1
#define FLAG_AUTH        2

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
typedef enum
{
    COND_DRUNK, COND_FULL, COND_THIRST, COND_BLOODTHIRST, MAX_CONDS
} conditions;

/*
 * Positions.
 */
typedef enum
{
    POS_DEAD, POS_MORTAL, POS_INCAP, POS_STUNNED, POS_SLEEPING, POS_MEDITATING,
    POS_BERSERK, POS_RESTING, POS_AGGRESSIVE, POS_SITTING, POS_CROUCH,
    POS_FIGHTING,
    POS_DEFENSIVE, POS_EVASIVE, POS_CRAWL, POS_STANDING, POS_MOUNTED, POS_SHOVE,
    POS_DRAG
} positions;

/*
 * Styles.
 */
typedef enum
{
    STYLE_BERSERK, STYLE_AGGRESSIVE, STYLE_FIGHTING, STYLE_DEFENSIVE,
    STYLE_EVASIVE,
} styles;

/*
 * ACT bits for players.
 */
typedef enum
{
    PLR_IS_NPC, PLR_BOUGHT_PET, PLR_SHOVEDRAG, PLR_AUTOEXIT, PLR_AUTOLOOT,
    PLR_AUTOTRASH, PLR_BLANK, PLR_OUTCAST, PLR_BRIEF, PLR_COMBINE,
    PLR_PROMPT, PLR_TELNET_GA, PLR_HOLYLIGHT, PLR_WIZINVIS, PLR_ROOMVNUM,
    PLR_SILENCE, PLR_NO_EMOTE, PLR_ATTACKER, PLR_NO_TELL, PLR_LOG,
    PLR_DENY, PLR_FREEZE, PLR_THIEF, PLR_KILLER, PLR_LITTERBUG,
    PLR_ANSI, PLR_RIP, PLR_NICE, PLR_FLEE, PLR_AUTOMONEY,
    PLR_STAFF, PLR_AFK, PLR_INVISPROMPT, PLR_AUTOGLANCE,
    PLR_QUESTING, PLR_MXP, PLR_SHADOWFORM, PLR_EXEMPT, PLR_AUTOMAP,
    PLR_R40, PLR_TUTORIAL, PLR_ACTIVATE, PLR_CARTOGRAPHER,
    PLR_BLIND, PLR_AUTODOOR, PLR_PKSAFE, PLR_RED, PLR_BLUE, PLR_FROZEN,
    PLR_PLAYING, PLR_WAITING,
    PLR_LIFE, PLR_RP, PLR_R54, PLR_TEASE, PLR_BOAT, PLR_MUSIC, PLR_SOUND, PLR_R59,
    PLR_EXTREME, PLR_R61, PLR_R62, PLR_R63, PLR_R64, PLR_R65, PLR_R66, PLR_R67,
    PLR_R68,
    PLR_R69, PLR_R70, PLR_R71, PLR_R72, PLR_R73, PLR_R74, PLR_R75, PLR_R76,
    PLR_R77,
    PLR_R78, PLR_R79, PLR_R80, PLR_R81
} player_flags;

/* Bits for pc_data->flags. */
#define PCFLAG_R1         BV00
#define PCFLAG_DEADLY     BV01
#define PCFLAG_UNAUTHED   BV02
#define PCFLAG_NORECALL   BV03
#define PCFLAG_NOINTRO    BV04
#define PCFLAG_GAG        BV05
#define PCFLAG_RETIRED    BV06
#define PCFLAG_GUEST      BV07
#define PCFLAG_NOSUMMON   BV08
#define PCFLAG_PAGERON    BV09
#define PCFLAG_NOTITLE    BV10
#define PCFLAG_GROUPWHO   BV11
#define PCFLAG_DIAGNOSE   BV12
#define PCFLAG_HIGHGAG    BV13
#define PCFLAG_WATCH      BV14                         /* see function "do_watch" */
#define PCFLAG_HINTS      BV15                         /* Force new players to help start 
                                                        */
#define PCFLAG_DND        BV16                         /* Do Not Disturb flage */
#define PCFLAG_IDLE       BV17                         /* Player is Linkdead */
#define PCFLAG_SHOWSLOTS  BV18                         /* Show eq slots */
#define PCFLAG_ADMIN      BV19                         /* Set an Immortal up as an Admin */
#define PCFLAG_ENFORCER   BV20                         /* Set an Immortal up as an
                                                        * Enforcer */
#define PCFLAG_BUILDER    BV21                         /* Set an Immortal up as an
                                                        * Builder */
#define PCFLAG_TEMP       BV22                         /* Set an Immortal up as an
                                                        * Roleplayer */
#define PCFLAG_PRIVACY    BV23                         /* For do_whois command */
#define PCFLAG_WIZHCOLOR  BV24
#define PCFLAG_AIDLE      BV25                         /* For auto idle */
#define PCFLAG_PUPPET	  BV26
#define PCFLAG_NOBEEP     BV27                         /* Samson Snippet 2-15-98 */
#define PCFLAG_DEMIGOD    BV28
#define PCFLAG_BUILDWALK  BV29

typedef enum
{
    TIMER_NONE, TIMER_RECENTFIGHT, TIMER_SHOVEDRAG, TIMER_DO_FUN,
    TIMER_APPLIED, TIMER_PKILLED, TIMER_ASUPRESSED, TIMER_NUISANCE
} timer_types;

struct timer_data
{
    TIMER                  *prev;
    TIMER                  *next;
    DO_FUN                 *do_fun;
    int                     value;
    short                   type;
    int                     count;
};

/*
 * Channel bits.
 */

#define MAX_CHANNEL 40

typedef enum
{
    CHANNEL_AUCTION, CHANNEL_OOC, CHANNEL_QUEST, CHANNEL_SOOC, CHANNEL_MUSIC,
    CHANNEL_ASK, CHANNEL_SHOUT, CHANNEL_YELL, CHANNEL_MONITOR, CHANNEL_LOG,
    CHANNEL_HIGHGOD, CHANNEL_CLAN, CHANNEL_BUILD, CHANNEL_HIGH, CHANNEL_AVTALK,
    CHANNEL_PRAY, CHANNEL_COUNCIL, CHANNEL_COMM, CHANNEL_TELLS,
    CHANNEL_NEWBIE, CHANNEL_WARTALK, CHANNEL_RACETALK,
    CHANNEL_WARN, CHANNEL_WHISPER, CHANNEL_AUTH, CHANNEL_TRAFFIC, CHANNEL_ICC,
    CHANNEL_SWEAR, CHANNEL_TELEPATHY, CHANNEL_6D
} channel_bits;

/* Area defines - Scryn 8/11 */
#define AREA_DELETED     BV00
#define AREA_LOADED                BV01

/* Area flags - Narn Mar/96 */
#define AFLAG_ANTIPKILL   BV00
#define AFLAG_FREEKILL    BV01
#define AFLAG_NOTELEPORT  BV02
#define AFLAG_SPELLLIMIT  BV03
#define AFLAG_DARKNESS    BV04
#define AFLAG_NOPKILL     BV05
#define AFLAG_NOASTRAL    BV06
#define AFLAG_NOSUMMON    BV07
#define AFLAG_NOPORTAL    BV08
#define AFLAG_INDOORS     BV09
#define AFLAG_UNOTSEE     BV10
#define AFLAG_NOQUEST     BV11
#define AFLAG_QUEST       BV12
#define AFLAG_PROTOTYPE   BV13
#define AFLAG_NODISCOVERY BV14
#define AFLAG_LIGHTED     BV15
#define AFLAG_GROUP       BV16
#define AFLAG_NOINFLUENCE BV17
#define AFLAG_ARENA       BV18

struct deity_data
{
    DEITY_DATA             *next;
    DEITY_DATA             *prev;
    char                   *filename;
    char                   *name;
    char                   *description;
    short                   alignment;
    short                   worshippers;
    short                   scorpse;
    short                   sdeityobj;
    short                   savatar;
    short                   srecall;
    short                   flee;
    short                   flee_npcrace;
    short                   flee_npcfoe;
    short                   kill;
    short                   kill_magic;
    short                   kill_npcrace;
    short                   kill_npcfoe;
    short                   sac;
    short                   bury_corpse;
    short                   aid_spell;
    short                   aid;
    short                   backstab;
    short                   steal;
    short                   die;
    short                   die_npcrace;
    short                   die_npcfoe;
    short                   spell_aid;
    short                   dig_corpse;
    int                     race;
    int                     race2;
    int                     Class;
    int                     sex;
    int                     npcrace;
    int                     npcfoe;
    int                     suscept;
    int                     element;
    EXT_BV                  affected;
    int                     susceptnum;
    int                     elementnum;
    int                     affectednum;
    int                     objstat;
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data
{
    MOB_INDEX_DATA         *next;
    MOB_INDEX_DATA         *next_sort;
    AREA_DATA              *area;
    SPEC_FUN               *spec_fun;
    struct shop_data       *pShop;
    struct repairshop_data *rShop;
    MPROG_DATA             *mudprogs;
    EXT_BV                  progtypes;
    char                   *player_name;
    char                   *short_descr;
    char                   *long_descr;
    char                   *description;
    int                     vnum;
    short                   count;
    short                   killed;
    short                   sex;
    short                   level;
    EXT_BV                  act;
    EXT_BV                  affected_by;
    short                   alignment;
    short                   mobthac0;                  /* Unused */
    short                   ac;
    short                   hitnodice;
    short                   hitsizedice;
    short                   hitplus;
    short                   damnodice;
    short                   damsizedice;
    short                   damplus;
    short                   numattacks;
    int                     money[MAX_CURR_TYPE];
    int                     gold;
    int                     exp;
    EXT_BV                  xflags;
    int                     immune;
    int                     resistant;
    int                     susceptible;
    EXT_BV                  attacks;
    EXT_BV                  defenses;
    int                     speaks;
    int                     speaking;
    int                     slicevnum;
    short                   position;
    short                   defposition;
    short                   height;
    short                   weight;
    short                   race;
    short                   Class;
    short                   hitroll;
    short                   damroll;
    short                   perm_str;
    short                   perm_int;
    short                   perm_wis;
    short                   perm_dex;
    short                   perm_con;
    short                   perm_cha;
    short                   perm_lck;
    short                   saving_poison_death;
    short                   saving_wand;
    short                   saving_para_petri;
    short                   saving_breath;
    short                   saving_spell_staff;
    short                   color;
    short                   influence;                 // chance a clan might gain
    // influence over a mob
    char                   *clanname;
};

struct hunt_hate_fear
{
    char                   *name;
    CHAR_DATA              *who;
};

struct fighting_data
{
    CHAR_DATA              *who;
    int                     xp;
    short                   align;
    short                   duration;
    short                   timeskilled;
    int                     distance;
};

struct extracted_char_data
{
    EXTRACT_CHAR_DATA      *next;
    CHAR_DATA              *ch;
    ROOM_INDEX_DATA        *room;
    ch_ret                  retcode;
    bool                    extract;
};

/*
 * One character (PC or NPC).
 * (Shouldn't most of that build interface stuff use substate, dest_buf,
 * spare_ptr and tempnum?  Seems a little redundant)
 */
struct char_data
{
    CHAR_DATA              *next;
    CHAR_DATA              *prev;
    CHAR_DATA              *next_in_room;
    CHAR_DATA              *prev_in_room;
    CHAR_DATA              *master;
    CHAR_DATA              *leader;
    FIGHT_DATA             *fighting;
    CHAR_DATA              *reply;
    CHAR_DATA              *retell;
    CHAR_DATA              *switched;                  /* PC_DATA ? */
    CHAR_DATA              *mount;
    EDITOR_DATA            *editor;
    HHF_DATA               *hunting;                   /* NPC */
    HHF_DATA               *fearing;                   /* NPC */
    HHF_DATA               *hating;                    /* NPC */
    VARIABLE_DATA          *variables;
    SPEC_FUN               *spec_fun;                  /* NPC */
    MPROG_ACT_LIST         *mpact;                     /* NPC */
    RESET_DATA             *reset;
    int                     mpactnum;                  /* NPC */
    short                   mpscriptpos;               /* NPC */
    MOB_INDEX_DATA         *pIndexData;                /* NPC */
    DESCRIPTOR_DATA        *desc;
    CHAR_DATA              *redirect;
    AFFECT_DATA            *first_affect;
    AFFECT_DATA            *last_affect;
    OBJ_DATA               *first_carrying;
    OBJ_DATA               *last_carrying;
    OBJ_DATA               *on;
    ROOM_INDEX_DATA        *in_room;
    ROOM_INDEX_DATA        *was_in_room;
    PC_DATA                *pcdata;
    DO_FUN                 *last_cmd;
    void                   *dest_buf;                  /* This one is to assign to
                                                        * differen things */
    char                   *alloc_ptr;                 /* Must str_dup and free this one */
    void                   *spare_ptr;
    int                     tempnum;
    TIMER                  *first_timer;
    TIMER                  *last_timer;
    struct char_morph      *morph;
    char                   *name;
    char                   *short_descr;               /* NPC */
    char                   *long_descr;                /* NPC */
    char                   *description;
    char                   *landmark;                  /* Used for auto landmark */
    short                   num_fighting;
    short                   substate;
    short                   sex;
    short                   Class;
    short                   secondclass;
    short                   thirdclass;
    short                   race;
    short                   trust;                     /* PC DATA ? */
    int                     played;                    /* PC DATA ? */
    time_t                  logon;                     /* PC DATA ? */
    time_t                  save_time;                 /* PC DATA ? */
    short                   timer;
    short                   wait;
    int                     blood;
    int                     max_blood;
    int                     hit;
    int                     max_hit;
    int                     mana;
    int                     max_mana;
    int                     move;
    int                     max_move;
    int                     slicevnum;
    short                   practice;                  /* PC DATA ? */
    short                   numattacks;
    int                     money[MAX_CURR_TYPE];
    int                     gold;
    int                     exp;                       /* Should be in PC_DATA, but I
                                                        * plan on mobs needing soon
                                                        * -Orion */
    int                     firstexp;
    int                     secondexp;
    int                     thirdexp;

    short                   firstexpratio;
    short                   secondexpratio;
    short                   thirdexpratio;

    short                   level;
    short                   firstlevel;
    short                   secondlevel;
    short                   thirdlevel;

    int                     temp_base_hit;             // Taon
    int                     quest_curr;                // Taon 
    int                     quest_accum;               // Taon 
    int                     used_trade;                // Taon 
    short                   focus_level;               // Taon 
    short                   faith;                     // Taon
    short                   chan_invite;               // Taon 
    short                   arena_mob_count;           // Taon
    short                   arena_obj_count;           // Taon
    int                     arena_wins;                // Taon
    int                     arena_loss;                // Taon
    short                   map_toggle;                // Taon
    short                   map_size;                  // Taon
    short                   map_desc_toggle;           // Taon
    short                   map_name_toggle;           // Taon
    short                   map_type;                  // Taon

    EXT_BV                  act;                       /* NPC */
    EXT_BV                  affected_by;
/*  EXT_BV affected_by2; */
    EXT_BV                  no_affected_by;
/*  EXT_BV no_affected_by2; */
    int                     carry_weight;
    int                     carry_number;
    EXT_BV                  xflags;
    int                     no_immune;
    int                     no_resistant;
    int                     no_susceptible;
    int                     immune;
    int                     resistant;
    int                     susceptible;
    EXT_BV                  attacks;                   /* NPC */
    EXT_BV                  defenses;                  /* NPC */
    int                     speaks;
    int                     speaking;
    short                   saving_poison_death;
    short                   saving_wand;
    short                   saving_para_petri;
    short                   saving_breath;
    short                   saving_spell_staff;
    short                   alignment;
    short                   barenumdie;
    short                   baresizedie;
    short                   mobthac0;                  /* NPC ? */
    short                   hitroll;
    short                   damroll;
    short                   hitplus;
    short                   damplus;
    short                   position;
    short                   defposition;
    short                   style;
    short                   height;
    short                   weight;
    short                   armor;
    short                   wimpy;
    EXT_BV                  deaf;
    short                   perm_str;
    short                   perm_int;
    short                   perm_wis;
    short                   perm_dex;
    short                   perm_con;
    short                   perm_cha;
    short                   perm_lck;
    short                   mod_str;
    short                   mod_int;
    short                   mod_wis;
    short                   mod_dex;
    short                   mod_con;
    short                   mod_cha;
    short                   mod_lck;
    short                   statpoints;
    short                   mental_state;              /* simplified PC DATA ? */
    short                   emotional_state;           /* simplified PC DATA ? */
    int                     retran;
    int                     regoto;
    short                   mobinvis;                  /* Mobinvis level SB NPC */
    int                     home_vnum;                 /* copyover tracker NPC ? */
    short                   colors[MAX_COLORS];
    CHAR_DATA              *challenged;                /* Who challenged them? */
    CHAR_DATA              *challenge;                 /* Who they challenged? */
    CHAR_DATA              *betted_on;
    int                     bet_amt;
    short                   cmd_recurse;
    struct note_data       *pnote;
    NOTE_DATA              *comments;
    short                   hate_level;                /* Used for mobs to switch to ones 
                                                        * with higher number */
    short                   ward_dam;                  /* Used to determine if a ward has 
                                                        * absorbed max damage */
    short                   kinetic_dam;
    short                   color;
    short                   influence;                 // chance a clan might gain
    // influence over a mob
    char                   *clanname;
    char                   *clan;
    short                   degree;
    int                     questcountdown;
    int                     questvnum;
    int                     questtype;
    int                     questgiver;
    int                     success_attack;
    int                     damage_amount;
};

struct killed_data
{
    int                     vnum;
    char                    count;
};

struct pkilled_data
{
    PKILLED_DATA           *next;
    PKILLED_DATA           *prev;
    char                   *name;
    time_t                  timekilled;
};

struct quest_data
{
    QUEST_DATA             *next,
                           *prev;
    CHAP_DATA              *first_chapter;
    CHAP_DATA              *last_chapter;
    int                     number;                    /* Number */
    char                   *name;                      /* Name of the quest */
    char                   *desc;
    int                     timelimit;                 /* Time limit on the full quest */
    int                     level;                     /* Minimum level needed to start
                                                        * the quest */
    int                     chapters;                  /* How many chapters? Big quest or 
                                                        * small quest? */
    int                     svnum;                     /* What vnum started this quest */
    short                   stype;                     /* What type started it (0 =
                                                        * Mobile, 1 = Object, 2 = Room) */
    bool                    skipchapters;              /* Are they allowed to skip
                                                        * chapters? */
};

struct chap_data
{
    CHAP_DATA              *next;
    CHAP_DATA              *prev;
    int                     number;
    char                   *desc;
    char                   *bio;
    int                     timelimit;                 /* Time limit for this chapter of
                                                        * the quest */
    int                     level;                     /* Minimum level for this chapter
                                                        * of the quest */
    int                     kamount;                   /* Amount they have to kill to
                                                        * complete this part of the quest 
                                                        */
};

struct chquest_data
{
    CHQUEST_DATA           *next,
                           *prev;
    int                     questnum;                  /* What quest is this? */
    int                     questlimit;                /* Current time limit on quest */
    int                     chaplimit;                 /* What the hell is this for? */
    int                     progress;                  /* Current quest progress */
    int                     times;                     /* Number of times player has
                                                        * attempted this quest */
    int                     kamount;                   /* How many do they still have to
                                                        * kill */
};

int                     get_number_from_quest( QUEST_DATA * quest );
QUEST_DATA             *get_quest_from_name( const char *name );
QUEST_DATA             *get_quest_from_number( int x );
int                     get_chapter( CHAR_DATA *ch, QUEST_DATA * quest );
int                     get_chkamount( CHAR_DATA *ch, QUEST_DATA * quest );

#define GET_CHAPTER( ch, quest ) ( get_chapter( (ch), (quest) ) )
#define GET_CHKAMOUNT( ch, quest ) ( get_chkamount( (ch), (quest) ) )

/* Structure for link list of ignored players */
struct ignore_data
{
    IGNORE_DATA            *next;
    IGNORE_DATA            *prev;
    char                   *name;
};

struct bank_data
{
    BANK_DATA              *next;
    char                   *name;
    char                   *password;
    time_t                  lastused;                  /* Lets keep track of when this
                                                        * bank last got used */
    int                     bronze;
    int                     copper;
    int                     gold;
    int                     silver;
    int                     amount;
};

/* Max number of people you can ignore at once */
#define MAX_IGN 6

/* a linked list of all of the areas a player has found */
struct found_area
{
    FOUND_AREA             *next,
                           *prev;
    char                   *area_name;
};

/* Data which only PC's have. */
struct pc_data
{
    int                     sqlnumber;                 /* Volk - if player's name and
                                                        * password match on forums,
                                                        * sqlnumber is set to their ID */
    char                   *sqlpass;                   /* another quick one - if their
                                                        * forums password changes, we
                                                        * want to know about it */
    CHAR_DATA              *pet;
    DEITY_DATA             *deity;
    FOUND_AREA             *first_area,
                           *last_area;
    CHQUEST_DATA           *first_quest,
                           *last_quest;
    struct clan_data       *clan;
    struct council_data    *council;
    struct htown_data      *htown;
    struct city_data       *city;
    AREA_DATA              *area;
    char                   *homepage;
    char                   *clan_name;
    int                     clan_timer;
    char                   *council_name;
    char                   *deity_name;
    char                   *htown_name;
    char                   *city_name;
    char                   *pwd;
    char                   *bamfin;
    char                   *bamfout;
    char                   *filename;                  /* For the safe mset name -Shaddai 
                                                        */
    char                   *rank;
    char                   *title;
    char                   *bestowments;               /* Special bestowed commands */
    int                     flags;                     /* Whether the player is deadly
                                                        * and whatever else we add.  */
    int                     pkills;                    /* Number of pkills on behalf of
                                                        * clan */
    int                     pdeaths;                   /* Number of times pkilled
                                                        * (legally) */
    int                     mkills;                    /* Number of mobs killed */
    int                     mdeaths;                   /* Number of deaths due to mobs */
    int                     illegal_pk;                /* Number of illegal pk's
                                                        * committed */
    int                     apply_blood;               /* Amount of blood to add to max
                                                        * blood */
    NUISANCE_DATA          *nuisance;                  /* New Nuisance structure */
    long int                restore_time;              /* The last time the char did a
                                                        * restore all */
    long int                heaven_time;               /* The last time the char did a
                                                        * heavens blessing */
    int                     r_range_lo;                /* room range */
    int                     r_range_hi;
    int                     m_range_lo;                /* mob range */
    int                     m_range_hi;
    int                     o_range_lo;                /* obj range */
    int                     o_range_hi;
    short                   wizinvis;                  /* wizinvis level */
    short                   min_snoop;                 /* minimum snoop level */
    short                   condition[MAX_CONDS];
    short                   learned[MAX_SKILL];
    short                   dlearned[MAX_SKILL];       /* This is for dragon's HUMAN
                                                        * FORM, to save a new set of
                                                        * skills heh */
    bool                    dshowlearned[MAX_SKILL];   /* Used for showing/not showing
                                                        * stuff thats learned */
    bool                    dshowdlearned[MAX_SKILL];  /* Same as above for dragon's
                                                        * human form */
    KILLED_DATA             killed[MAX_KILLTRACK];
    PKILLED_DATA           *first_pkill;
    PKILLED_DATA           *last_pkill;
    short                   favor;                     /* deity favor */
    short                   charmies;                  /* Number of Charmies */
    int                     auth_state;
    time_t                  release_date;              /* Auto-helling.. Altrag */
    char                   *helled_by;
    char                   *bio;                       /* Personal Bio */
    char                   *authed_by;                 /* what crazy imm authed this name 
                                                        * ;) */
    SKILLTYPE              *special_skills[MAX_PERSONAL];   /* personalized skills/spells 
                                                             */
    char                   *prompt;                    /* User config prompts */
    char                   *fprompt;                   /* Fight prompts */
    char                   *subprompt;                 /* Substate prompt */
    short                   pagerlen;                  /* For pager (NOT menus) */
    IGNORE_DATA            *first_ignored;             /* keep track of who to ignore */
    IGNORE_DATA            *last_ignored;
    short                   colors[MAX_COLORS];        /* Custom color codes - Samson
                                                        * 9-28-98 */
#ifdef I3
    I3_CHARDATA           *i3chardata;
#endif
    bool                    copyover;                  /* copyover tracker */
    char                   *say_history[20];
    char                   *tell_history[20];
    char                   *chan_listen;               /* For dynamic channels - Samson
                                                        * 3-2-02 */
    char                   *email;                     /* Email address for whois -
                                                        * Samson */
    int                     icq;                       /* ICQ# for whois - Samson 1-4-99 */
    short                   banish;                    /* letting warmaster and chieftain 
                                                        * banish players if they both
                                                        * agree. char *lasthost; /*
                                                        * Stores host info so it doesn't 
                                                        * have to depend on descriptor,
                                                        * for things like finger */
    char                   *afkbuf;
    ALIAS_DATA             *first_alias;
    ALIAS_DATA             *last_alias;
    int                     lair;
    int                     palace;
    char                   *vamp_status;
    short                   tmptrust;
    short                   tmplevel;
    short                   extlevel;
    int                     tmpmax_mana;
    int                     tmpmax_move;
    int                     tmpmax_hit;
    int                     preglory_hit;
    int                     preglory_mana;
    int                     preglory_move;
    short                   tmpheight;
    short                   tmpcrawl;
    short                   tmpweight;
    short                   tmpclass;
    short                   tmprace;
    int                     tmproom;
    struct editor_data     *editor;
    NOTE_DATA              *in_progress;
    GLOBAL_BOARD_DATA      *board;
    time_t                  last_note[MAX_BOARD];
    time_t                  last2_note[MAX_BOARD];
    time_t                  boards;
    /*
     * Double exp stuff 
     */
    int                     double_exp_timer;
    time_t                  last_dexpupdate;
    char                   *authexp;
    bool                    getsdoubleexp;
    long int                last_read_news;            /* Timestamp of last news read */
    int                     holdbreath;                /* How long a player has held
                                                        * their breath for */
    int                     frostbite;                 /* how long before frost bite */
    short                   adjust;
    short                   textspeed;                 /* 1-10, how fast mppauses go */
    char                   *alt1;
    char                   *alt2;                      /* Basic puppet code */
    char                   *alt3;
    char                   *alt4;
    char                   *alt5;
    char                   *rand;
    short                   hp_balance;
    int                     potionsoob;
    int                     potionspvp;
    int                     potionspve;
    short                   age_bonus;
    short                   age;
    short                   day;
    short                   month;
    short                   year;
    int                     timezone;
    long                    tag_flags;
    short                   tutorialtimer;
    short                   num_bugs;
    short                   bugReward1;
    short                   bugReward2;
    short                   num_ideas;
    short                   ideaReward1;
    short                   ideaReward2;
    short                   num_typos;
    short                   typoReward1;
    short                   typoReward2;
    int                     lastaccountcreated;
    int                     warn;
    BANK_DATA              *bank;
    short                   tradelevel;                // keep track of blacksmith level
    short                   tradeclass;                // what tradeskill class are they
    int                     craftpoints;               // how many craft points to gain a 
                                                       // level
    /*
     * Volk - BARDS! Need to have an instrument timer 
     */
    int                     bard;
    int                     bardsn;
    int                     clanpoints;                // keep track how many clan points 
                                                       // a player earns
    // for clan
    void                   *dest_buf;                  /* This one is to assign to
                                                        * differen things */
    void                   *spare_ptr;
    int                     score;
    unsigned int            questcompleted;
    short                   voted;
    short                   rprate;                    // How good a Role player is voted 
                                                       // by peers
    time_t                  lastrated;                 // To allow for ratings every 24
                                                       // hrs 
};

/* Liquids. */
#define LIQ_WATER   0
#define LIQ_MAX    21                                  /* Added vomit and squishee,
                                                        * holywater */

struct liq_type
{
    const char             *liq_name;
    const char             *liq_color;
    short                   liq_affect[3];
};

/* modified for new weapon_types - Grimm */
/* Trimmed down to reduce duplicated types - Samson 1-9-00 */
typedef enum
{
    DAM_HIT, DAM_SLASH, DAM_STAB, DAM_HACK, DAM_CRUSH, DAM_LASH,
    DAM_PIERCE, DAM_THRUST, DAM_MAX_TYPE
} damage_types;

extern const struct obj_color_type freak_color[];

struct obj_color_type
{
    const char             *name;
};

#define MAX_NUMBER 15

/* New Weapon type array for profficiency checks - Samson 11-20-99
 * Changed by Volk - 4/Sep/06 */
typedef enum
{
    WEP_2H_LONG_BLADE, WEP_1H_LONG_BLADE, WEP_1H_SHORT_BLADE,
    WEP_WHIP, WEP_2H_BLUDGEON, WEP_1H_BLUDGEON, WEP_ARCHERY,
    WEP_BLOWGUN, WEP_2H_AXE, WEP_1H_AXE, WEP_SPEAR, WEP_STAFF,
    WEP_LANCE, WEP_FLAIL, WEP_TALON, WEP_POLEARM, WEP_MAX
} weapon_types;

/* New projectile type array for archery weapons - Samson 1-9-00 */
typedef enum
{
    PROJ_BOLT, PROJ_ARROW, PROJ_DART, PROJ_STONE, PROJ_MAX
} projectile_types;

/*
 * Extra description data for a room or object.
 */
struct extra_descr_data
{
    EXTRA_DESCR_DATA       *next;                      /* Next in list */
    EXTRA_DESCR_DATA       *prev;                      /* Previous in list */
    char                   *keyword;                   /* Keyword in look/examine */
    char                   *description;               /* What to see */
};

/*
 * Prototype for an object.
 */
struct obj_index_data
{
    OBJ_INDEX_DATA         *next;
    OBJ_INDEX_DATA         *next_sort;
    AREA_DATA              *area;
    EXTRA_DESCR_DATA       *first_extradesc;
    EXTRA_DESCR_DATA       *last_extradesc;
    AFFECT_DATA            *first_affect;
    AFFECT_DATA            *last_affect;
    MPROG_DATA             *mudprogs;                  /* objprogs */
    EXT_BV                  progtypes;                 /* objprogs */
    char                   *name;
    char                   *short_descr;
    char                   *description;
    char                   *action_desc;
    int                     vnum;
    short                   level;
    short                   item_type;
    EXT_BV                  extra_flags;
    int                     wear_flags;
    short                   count;
    short                   weight;
    int                     cost;
    int                     value[7];
    int                     serial;
    short                   layers;
    int                     rent;                      /* Unused */
    short                   currtype;
    short                   color;
    short                   size;
    short                   rating;
};

/*
 * One object.
 */
struct obj_data
{
    OBJ_DATA               *next;
    OBJ_DATA               *prev;
    OBJ_DATA               *next_content;
    OBJ_DATA               *prev_content;
    OBJ_DATA               *first_content;
    OBJ_DATA               *last_content;
    OBJ_DATA               *in_obj;
    CHAR_DATA              *carried_by;
    EXTRA_DESCR_DATA       *first_extradesc;
    EXTRA_DESCR_DATA       *last_extradesc;
    AFFECT_DATA            *first_affect;
    AFFECT_DATA            *last_affect;
    OBJ_INDEX_DATA         *pIndexData;
    ROOM_INDEX_DATA        *in_room;
    RESET_DATA             *reset;
    char                   *name;
    char                   *short_descr;
    char                   *description;
    char                   *action_desc;
    short                   item_type;
    short                   mpscriptpos;
    EXT_BV                  extra_flags;
    int                     wear_flags;
    MPROG_ACT_LIST         *mpact;                     /* mudprogs */
    int                     mpactnum;                  /* mudprogs */
    short                   wear_loc;
    short                   weight;
    int                     cost;
    short                   level;
    short                   timer;
    int                     value[7];
    short                   count;                     /* support for object grouping */
    int                     serial;                    /* serial number */
    int                     room_vnum;                 /* copyover tracker */
    short                   currtype;
    short                   color;
    char                   *owner;
    short                   size;
    int                     glory;
};

/*
 * Exit data.
 */
struct exit_data
{
    EXIT_DATA              *prev;                      /* previous exit in linked list */
    EXIT_DATA              *next;                      /* next exit in linked list */
    EXIT_DATA              *rexit;                     /* Reverse exit pointer */
    ROOM_INDEX_DATA        *to_room;                   /* Pointer to destination room */
    char                   *keyword;                   /* Keywords for exit or door */
    char                   *description;               /* Description of exit */
    int                     vnum;                      /* Vnum of room exit leads to */
    int                     rvnum;                     /* Vnum of room in opposite dir */
    int                     exit_info;                 /* door states & other flags */
    int                     key;                       /* Key vnum */
    short                   vdir;                      /* Physical "direction" */
    short                   pull;                      /* pull of direction (current) */
    short                   pulltype;                  /* type of pull (current, wind) */
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'H': hide an object
 *   'B': set a bitvector
 *   'T': trap an object
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct reset_data
{
    RESET_DATA             *next;
    RESET_DATA             *prev;
    RESET_DATA             *first_reset;
    RESET_DATA             *last_reset;
    RESET_DATA             *next_reset;
    RESET_DATA             *prev_reset;
    CHAR_DATA              *ch;
    OBJ_DATA               *obj;
    char                    command;
    int                     extra;
    int                     arg1;
    int                     arg2;
    int                     arg3;
};

/* Constants for arg2 of 'B' resets. */
#define BIT_RESET_DOOR 0
#define BIT_RESET_OBJECT 1
#define BIT_RESET_MOBILE 2
#define BIT_RESET_ROOM 3
#define BIT_RESET_TYPE_MASK 0xFF                       /* 256 should be enough */
#define BIT_RESET_DOOR_THRESHOLD  8
#define BIT_RESET_DOOR_MASK 0xFF00                     /* 256 should be enough */
#define BIT_RESET_SET BV30
#define BIT_RESET_TOGGLE BV31
#define BIT_RESET_FREEBITS 0x3FFF0000                  /* For reference */

/* Area definition. */
struct area_data
{
    AREA_DATA              *next_area;
    AREA_DATA              *prev_area;
    AREA_DATA              *next;
    AREA_DATA              *prev;
    AREA_DATA              *next_sort;
    AREA_DATA              *prev_sort;
    AREA_DATA              *next_sort_name;            /* Used for alphanum. sort */
    AREA_DATA              *prev_sort_name;            /* Ditto, Fireblade */
    struct clan_data       *influencer;                /* What clan is influencing it */
    char                   *name;
    char                   *filename;
    int                     flags;
    short                   status;                    /* h, 8/11 */
    short                   age;
    short                   nplayer;
    short                   reset_frequency;
    int                     low_r_vnum;
    int                     hi_r_vnum;
    int                     low_o_vnum;
    int                     hi_o_vnum;
    int                     low_m_vnum;
    int                     hi_m_vnum;
    int                     low_soft_range;
    int                     hi_soft_range;
    int                     low_hard_range;
    int                     hi_hard_range;
    int                     spelllimit;
    int                     curr_spell_count;
    char                   *author;                    /* Scryn */
    char                   *derivatives;               /* Orion */
    char                   *htown;                     /* Vladaar */
    char                   *city;
    char                   *desc;                      /* Vladaar */
    char                   *resetmsg;                  /* Rennard */
    ROOM_INDEX_DATA        *first_room;
    ROOM_INDEX_DATA        *last_room;
    short                   max_players;
    int                     mkills;
    int                     mdeaths;
    int                     pkills;
    int                     pdeaths;
    int                     looted[MAX_CURR_TYPE];
    int                     high_economy[MAX_CURR_TYPE];
    int                     low_economy[MAX_CURR_TYPE];
    short                   weatherx;                  /* Weather Cell Assignment for the 
                                                        * X-Axis */
    short                   weathery;                  /* Weather Cell Assignment for the 
                                                        * Y-Axis */
    short                   currvnum;
    CURR_INDEX_DATA        *currindex;
    int                     illegal_pk;
    short                   color;
    short                   influence;                 // chance a clan might gain
    // influence over a area
    char                   *clanname;
};

/*
 * Used to keep track of system settings and statistics  -Thoric
 */
struct system_data
{
    int                     maxplayers;                /* Maximum players this boot */
    int                     alltimemax;                /* Maximum players ever */
    int                     global_looted;             /* Gold looted this boot */
    int                     upill_val;                 /* Used pill value */
    int                     upotion_val;               /* Used potion value */
    int                     brewed_used;               /* Brewed potions used */
    int                     scribed_used;              /* Scribed scrolls used */
    int                     numquotes;                 /* Keep track of number of quotes
                                                        * here */
    char                   *time_of_max;               /* Time of max ever */
    char                   *mud_name;                  /* Name of mud */
    bool                    NO_NAME_RESOLVING;         /* Hostnames are not resolved */
    bool                    DENY_NEW_PLAYERS;          /* New players cannot connect */
    bool                    WAIT_FOR_AUTH;             /* New players must be auth'ed */
    bool                    Advanced_Player;
    short                   read_all_mail;             /* Read all player mail(was 54) */
    short                   read_mail_free;            /* Read mail for free (was 51) */
    short                   write_mail_free;           /* Write mail for free(was 51) */
    short                   take_others_mail;          /* Take others mail (was 54) */
    short                   muse_level;                /* Level of muse channel */
    short                   think_level;               /* Level of think channel
                                                        * LEVEL_MASTER */
    short                   build_level;               /* Level of build channel
                                                        * LEVEL_WORKER */
    short                   log_level;                 /* Level of log channel LEVEL LOG */
    short                   level_modify_proto;        /* Level to modify prototype stuff
                                                        * LEVEL_WORKER */
    short                   level_override_private;    /* override private flag */
    short                   level_mset_player;         /* Level to mset a player */
    short                   bash_plr_vs_plr;           /* Bash mod player vs. player */
    short                   bash_nontank;              /* Bash mod basher != primary
                                                        * attacker */
    short                   gouge_plr_vs_plr;          /* Gouge mod player vs. player */
    short                   gouge_nontank;             /* Gouge mod player != primary
                                                        * attacker */
    short                   stun_plr_vs_plr;           /* Stun mod player vs. player */
    short                   stun_regular;              /* Stun difficult */
    short                   dodge_mod;                 /* Divide dodge chance by */
    short                   parry_mod;                 /* Divide parry chance by */
    short                   tumble_mod;                /* Divide tumble chance by */
    short                   phase_mod;
    short                   displacement_mod;
    short                   dam_plr_vs_plr;            /* Damage mod player vs. player */
    short                   dam_plr_vs_mob;            /* Damage mod player vs. mobile */
    short                   dam_mob_vs_plr;            /* Damage mod mobile vs. player */
    short                   dam_mob_vs_mob;            /* Damage mod mobile vs. mobile */
    short                   level_getobjnotake;        /* Get objects without take flag */
    short                   level_forcepc;             /* The level at which you can use
                                                        * force on players. */
    short                   bestow_dif;                /* Max # of levels between trust
                                                        * and command level for a bestow
                                                        * to work --Blodkai */
    char                   *clan_overseer;             /* Pointer to char containing the
                                                        * name of the */
    char                   *clan_advisor;              /* clan overseer and advisor. */
    short                   clan_timer;
    short                   clanlog;
    EXT_BV                  save_flags;                /* Toggles for saving conditions */
    short                   save_frequency;            /* How old to autosave someone */
    short                   check_imm_host;            /* Do we check immortal's hosts? */
    short                   morph_opt;                 /* Do we optimize morph's? */
    short                   save_pets;                 /* Do pets save? */
    short                   ban_site_level;            /* Level to ban sites */
    short                   ban_class_level;           /* Level to ban classes */
    short                   ban_race_level;            /* Level to ban races */
    short                   ident_retries;             /* Number of times to retry broken 
                                                        * pipes. */
    short                   pk_loot;                   /* Pkill looting allowed? */
    short                   reqwho_arg;                /* Who argument required? */
    short                   wiz_lock;                  /* Game wizlocked? */
    short                   autoboot_hour;             /* What hour does the autoboot
                                                        * happen? */
    short                   autoboot_minute;           /* What minute does the autoboot
                                                        * happen? */
    short                   autoboot_period;           /* What period of days pass
                                                        * between autoboots? */
    short                   newbie_purge;              /* Level to auto-purge newbies at
                                                        * - Samson 12-27-98 */
    short                   regular_purge;             /* Level to purge normal players
                                                        * at - Samson 12-27-98 */
    bool                    CLEANPFILES;               /* Should the mud clean up pfiles
                                                        * daily? - Samson 12-27-98 */
    char                   *http;                      /* Store web address for
                                                        * who/webwho */
//Volk: Added support for max adepts for each class
    short                   class3maxadept;
    short                   class2maxadept;
    short                   class1maxadept;
    short                   potionsoob;
    short                   potionspvp;
    short                   potionspve;
    int                     maxholiday;                /* Maximum Number of Holidays
                                                        * settable. */
    /*
     * Settings Things for calendar - Most changable in cset 
     */
    int                     secpertick;
    int                     pulsepersec;
    int                     pulsetick;
    int                     pulseviolence;
    int                     pulsemobile;
    int                     pulsecalendar;
    int                     pulseweather;
    int                     pulseaffect;
    /*
     * direct influence over the calendar 
     */
    int                     hoursperday;
    int                     daysperweek;
    int                     dayspermonth;
    int                     monthsperyear;
    int                     daysperyear;
    int                     hoursunrise;
    int                     hourdaybegin;
    int                     hournoon;
    int                     hoursunset;
    int                     hournightbegin;
    int                     hourmidnight;
    int                     bcount;
    int                     scount;
    bool                    dexp;
    bool                    daydexp;
    short                   gmb;
};

/* Room type. */
struct room_index_data
{
    ROOM_INDEX_DATA        *next;
    ROOM_INDEX_DATA        *next_sort;
    CHAR_DATA              *first_person;
    CHAR_DATA              *last_person;
    OBJ_DATA               *first_content;
    OBJ_DATA               *last_content;
    EXTRA_DESCR_DATA       *first_extradesc;
    EXTRA_DESCR_DATA       *last_extradesc;
    AREA_DATA              *area;
    EXIT_DATA              *first_exit;
    EXIT_DATA              *last_exit;
    AFFECT_DATA            *first_affect;
    AFFECT_DATA            *last_affect;
    MPROG_ACT_LIST         *mpact;
    int                     mpactnum;
    MPROG_DATA             *mudprogs;
    short                   mpscriptpos;
    char                   *name;
    char                   *description;
    int                     vnum;
    int                     room_flags;
    EXT_BV                  progtypes;
    short                   light;
    short                   sector_type;
    short                   winter_sector;
    int                     tele_vnum;
    short                   tele_delay;
    short                   tunnel;
    short                   height;
    CURR_INDEX_DATA        *currindex;
    short                   currvnum;
/*
  short curresources;
  short maxresources;
  short rarity;
*/
    RESET_DATA             *first_reset;
    RESET_DATA             *last_reset;
    RESET_DATA             *last_mob_reset;
    RESET_DATA             *last_obj_reset;
    ROOM_INDEX_DATA        *next_aroom;                /* Rooms within an area */
    ROOM_INDEX_DATA        *prev_aroom;
};

/* Delayed teleport type. */
struct teleport_data
{
    TELEPORT_DATA          *next;
    TELEPORT_DATA          *prev;
    ROOM_INDEX_DATA        *room;
    short                   timer;
};

/* Types of skill numbers.  Used to keep separate lists of sn's
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED -1
#define TYPE_HIT      1000                             /* allows for 1000 skills/spells */
#define TYPE_HERB     2000                             /* allows for 1000 attack types */
#define TYPE_PERSONAL 3000                             /* allows for 1000 herb types */
#define TYPE_RACIAL   4000                             /* allows for 1000 personal types */
#define TYPE_DISEASE  5000                             /* allows for 1000 racial types */
#define TYPE_MAX      6000                             /* ditto for disease */

/* Target types. */
typedef enum
{
    TAR_IGNORE, TAR_CHAR_OFFENSIVE, TAR_CHAR_DEFENSIVE, TAR_CHAR_SELF,
    TAR_OBJ_INV,
} target_types;

typedef enum
{
    SKILL_UNKNOWN, SKILL_SPELL, SKILL_SKILL, SKILL_WEAPON, SKILL_TONGUE,
    SKILL_RACIAL, SKILL_UNUSED, SKILL_SONG
} skill_types;

struct timerset
{
    int                     num_uses;
    struct timeval          total_time;
    struct timeval          min_time;
    struct timeval          max_time;
};

/* Skills include spells as a particular case. */
struct skill_type
{
    char                   *name;                      /* Name of skill */
    short                   skill_level[MAX_CLASS];    /* Level needed by class */
    short                   skill_adept[MAX_CLASS];    /* Max attainable % in this skill */
    short                   race_level[MAX_RACE];      /* Racial abilities: level */
    short                   race_adept[MAX_RACE];      /* Racial abilities: adept */
    SPELL_FUN              *spell_fun;                 /* Spell pointer (for spells) */
    DO_FUN                 *skill_fun;                 /* Skill pointer (for skills) */
    short                   target;                    /* Legal targets */
    short                   minimum_position;          /* Position for caster / user */
    short                   slot;                      /* Slot for #OBJECT loading */
    short                   min_mana;                  /* Minimum mana used */
    short                   beats;                     /* Rounds required to use skill */
    char                   *noun_damage;               /* Damage message */
    char                   *msg_off;                   /* Wear off message */
    short                   min_level;                 /* Minimum level to be able to
                                                        * cast */
    short                   min_dist;                  // for range combat
    short                   max_dist;                  // for range combat
    short                   type;                      /* Spell/Skill/Weapon/Tongue */
    short                   range;                     /* Range of spell (rooms) */
    int                     info;                      /* Spell action/class/etc */
    EXT_BV                  flags;                     /* Flags */
    char                   *hit_char;                  /* Success message to caster */
    char                   *hit_vict;                  /* Success message to victim */
    char                   *hit_room;                  /* Success message to room */
    char                   *hit_dest;                  /* Success message to dest room */
    char                   *miss_char;                 /* Failure message to caster */
    char                   *miss_vict;                 /* Failure message to victim */
    char                   *miss_room;                 /* Failure message to room */
    char                   *die_char;                  /* Victim death msg to caster */
    char                   *die_vict;                  /* Victim death msg to victim */
    char                   *die_room;                  /* Victim death msg to room */
    char                   *imm_char;                  /* Victim immune msg to caster */
    char                   *imm_vict;                  /* Victim immune msg to victim */
    char                   *imm_room;                  /* Victim immune msg to room */
    char                   *dice;                      /* Dice roll */
    int                     value;                     /* Misc value */
    int                     spell_sector;              /* Sector Spell work */
    char                    saves;                     /* What saving spell applies */
    char                    difficulty;                /* Difficulty of casting/learning */
//  short trade; /* Setting skills to tradeskill professions */
    SMAUG_AFF              *affects;                   /* Spell affects, if any */
    char                   *components;                /* Spell components, if any */
    char                   *teachers;                  /* Skill requires a special
                                                        * teacher */
    char                    participants;              /* # of required participants */
    struct timerset         userec;                    /* Usage record */
    bool                    somatic;
    bool                    verbal;
};

/* So we can have different configs for different ports -- Shaddai */
extern int              port;
extern BANK_DATA       *bank_index[];

/* These are skill_lookup return values for common skills and spells. */
extern short            gsn_body_drop;
extern short            gsn_cannibalize;
extern short            gsn_marble_skin;
extern short            gsn_granite_skin;
extern short            gsn_break;
extern short            gsn_brain_boil;
extern short            gsn_death_embrace;
extern short            gsn_heart_grab;
extern short            gsn_hallowed_blow;
extern short            gsn_mitigate;
extern short            gsn_glacial_armor;
extern short            gsn_behead;
extern short            gsn_remember;
extern short            gsn_flint_fire;
extern short            gsn_brittle_bone;
extern short            gsn_festering_wound;
extern short            gsn_dehydrate;
extern short            gsn_druidic_hymn;
extern short            gsn_shriek;
extern short            gsn_shrieking_note;
extern short            gsn_harmonic_melody;
extern short            gsn_human_form;
extern short            gsn_evac_crescendo;
extern short            gsn_thunderous_hymn;
extern short            gsn_stirring_ballad;
extern short            gsn_ottos_dance;
extern short            gsn_unholy_melody;
extern short            gsn_sound_waves;
extern short            gsn_rousing_tune;
extern short            gsn_telepathy;
extern short            gsn_heavens_blessing;
extern short            gsn_silence;
extern short            gsn_shapeshift;
extern short            gsn_shield;
extern short            gsn_higher_magic;
extern short            gsn_sanctuary;
extern short            gsn_wings;
extern short            gsn_demonic_sight;
extern short            gsn_dye;
extern short            gsn_assassinate;
extern short            gsn_breath;
extern short            gsn_lick;
extern short            gsn_tears;
extern short            gsn_eldritch_bolt;
extern short            gsn_taunt;
extern short            gsn_mortify;
extern short            gsn_mine;
extern short            gsn_assault;
extern short            gsn_enrage;
extern short            gsn_pawn;
extern short            gsn_wailing;
extern short            gsn_detrap;
extern short            gsn_backstab;
extern short            gsn_arteries;
extern short            gsn_circle;
extern short            gsn_cone;
extern short            gsn_defend;
extern short            gsn_cook;
extern short            gsn_dodge;
extern short            gsn_phase;                     // Taon
extern short            gsn_displacement;              // Taon
extern short            gsn_hide;
extern short            gsn_smuggle;
extern short            gsn_peek;
extern short            gsn_pick_lock;
extern short            gsn_scan;
extern short            gsn_sneak;
extern short            gsn_steal;
extern short            gsn_gouge;
extern short            gsn_track;
extern short            gsn_smell;
extern short            gsn_search;
extern short            gsn_earthspeak;                // Volk
extern short            gsn_mount;
extern short            gsn_snatch;
extern short            gsn_shelter;
extern short            gsn_gauge;
extern short            gsn_burrow;
extern short            gsn_vampiric_strength;
extern short            gsn_surreal_speed;
extern short            gsn_shrink;
extern short            gsn_passage;
extern short            gsn_conjure_elemental;
extern short            gsn_animal_companion;
extern short            gsn_lesser_skeleton;
extern short            gsn_extract_skeleton;
extern short            gsn_paralyze;
extern short            gsn_judo;
extern short            gsn_judge;
extern short            gsn_bone;
extern short            gsn_angelfire;
extern short            gsn_unholy_sphere;
extern short            gsn_charge;
extern short            gsn_possess;
extern short            gsn_meditate;
extern short            gsn_trance;
extern short            gsn_pluck;
extern short            gsn_drop;
extern short            gsn_rage;
extern short            gsn_fly_home;
extern short            gsn_draw_mana;
extern short            gsn_harm_touch;
extern short            gsn_devour;
extern short            gsn_spike;
extern short            gsn_gut;
extern short            gsn_hellfire;                  // Taon
extern short            gsn_deathroll;                 // Taon
extern short            gsn_ballistic;                 // Taon
extern short            gsn_flaming_shield;            // Taon
extern short            gsn_tangle;
extern short            gsn_entangle;                  // Taon
extern short            gsn_submerged;
extern short            gsn_thaitin;
extern short            gsn_truesight;
extern short            gsn_nosight;
extern short            gsn_tail_swipe;
extern short            gsn_gust_of_wind;
extern short            gsn_stomp;
extern short            gsn_bashdoor;
extern short            gsn_berserk;
extern short            gsn_hitall;
extern short            gsn_feign_death;
extern short            gsn_blizzard;
extern short            gsn_inferno;
extern short            gsn_flamebreath;
extern short            gsn_flaming_whip;
extern short            gsn_energy_containment;
extern short            gsn_rotten_gut;                // Aurin

/* Volk - makeover bards */
extern short            gsn_play;
extern short            gsn_vocals;
extern short            gsn_woodwinds;
extern short            gsn_strings;
extern short            gsn_brass;
extern short            gsn_drums;

/* Brain guy skills */
extern short            gsn_choke;
extern short            gsn_thicken_skin;
extern short            gsn_mental_assault;
extern short            gsn_kinetic_barrier;
extern short            gsn_leech;
extern short            gsn_dream_walk;
extern short            gsn_psionic_blast;
extern short            gsn_graft_weapon;
extern short            gsn_chameleon;
extern short            gsn_heal;
extern short            gsn_healing_thoughts;
extern short            gsn_object_reading;
extern short            gsn_lore;
extern short            gsn_whip_of_murazor;
extern short            gsn_astral_body;
extern short            gsn_sustenance;
extern short            gsn_fear;
extern short            gsn_drowsy;
extern short            gsn_torture_mind;

extern short            gsn_disarm;
extern short            gsn_enhanced_damage;
extern short            gsn_grendals_stance;
extern short            gsn_masters_eye;               // Taon
extern short            gsn_siphon_strength;           // Vladaar
extern short            gsn_kick;
extern short            gsn_know_enemy;
extern short            gsn_quivering_palm;
extern short            gsn_tackle;
extern short            gsn_backhand;
extern short            gsn_righteous_blow;
extern short            gsn_maim;
extern short            gsn_parry;
extern short            gsn_second_attack;
extern short            gsn_third_attack;
extern short            gsn_fourth_attack;
extern short            gsn_fifth_attack;
extern short            gsn_sixth_attack;
extern short            gsn_seventh_attack;
extern short            gsn_eighth_attack;
extern short            gsn_dual_wield;
extern short            gsn_rapid_healing;             // Taon
extern short            gsn_combat_mind;               // Taon
extern short            gsn_find_weakness;             // Taon
extern short            gsn_battle_trance;             // Taon
extern short            gsn_counterstrike;             // Taon
extern short            gsn_blade_master;              // Taon
extern short            gsn_heavy_hands;
extern short            gsn_boost;                     // Taon
extern short            gsn_prayer;                    // Taon
extern short            gsn_desecrate;
extern short            gsn_focus;                     // Taon
extern short            gsn_defensive_posturing;       // Taon
extern short            gsn_feed;
extern short            gsn_bloodlet;
extern short            gsn_broach;
extern short            gsn_mistwalk;
extern short            gsn_forge;
extern short            gsn_tan;
extern short            gsn_hunt;
extern short            gsn_mix;
extern short            gsn_crafted_food;
extern short            gsn_crafted_drink;
extern short            gsn_gather;
extern short            gsn_bake;
extern short            gsn_root;                      // Taon
extern short            gsn_untangle;                  // Taon
extern short            gsn_iron_skin;                 // Taon
extern short            gsn_impale;
extern short            gsn_ritual;                    // Taon
extern short            gsn_sustain_self;              // Taon
extern short            gsn_biofeedback;               // Taon
extern short            gsn_recoil;                    // Taon
extern short            gsn_decree_decay;              // Vladaar
extern short            gsn_keen_eye;                  // Taon
extern short            gsn_daze;                      // Taon
extern short            gsn_fury;                      // Taon
extern short            gsn_ensnare;                   // Taon
extern short            gsn_layhands;                  // Taon
extern short            gsn_endurance;                 // Taon
extern short            gsn_sidestep;                  // Taon
extern short            gsn_dominate;                  // Taon
extern short            gsn_acidic_touch;              // Taon
extern short            gsn_martial_arts;              // Taon
extern short            gsn_faith;                     // Taon
extern short            gsn_aid;

/* used to do specific lookups */
extern short            gsn_first_spell;
extern short            gsn_first_skill;
extern short            gsn_first_weapon;
extern short            gsn_first_tongue;

//extern short gsn_first_trade;
extern short            gsn_first_song;
extern short            gsn_top_sn;

/* spells */
extern short            gsn_blindness;
extern short            gsn_charm_person;
extern short            gsn_aqua_breath;
extern short            gsn_curse;
extern short            gsn_invis;
extern short            gsn_spectral_ward;
extern short            gsn_spirits_ward;
extern short            gsn_mass_invis;
extern short            gsn_poison;
extern short            gsn_sleep;
extern short            gsn_fireball;                  /* for fireshield */

/* newer attack skills */
extern short            gsn_punch;
extern short            gsn_bash;
extern short            gsn_stun;
extern short            gsn_bite;
extern short            gsn_claw;
extern short            gsn_sting;

extern short            gsn_poison_weapon;
extern short            gsn_ball_of_fire;
extern short            gsn_scribe;
extern short            gsn_brew;
extern short            gsn_climb;

/* changed to new weapon types - Grimm */
extern short            gsn_pugilism;
extern short            gsn_2h_long_blades;
extern short            gsn_1h_long_blades;
extern short            gsn_1h_short_blades;
extern short            gsn_whips;
extern short            gsn_2h_bludgeons;
extern short            gsn_1h_bludgeons;
extern short            gsn_archery;
extern short            gsn_restring;
extern short            gsn_spears;
extern short            gsn_staves;
extern short            gsn_blowguns;
extern short            gsn_1h_axes;
extern short            gsn_2h_axes;
extern short            gsn_lances;
extern short            gsn_flails;
extern short            gsn_talons;
extern short            gsn_polearms;

extern short            gsn_grip;
extern short            gsn_slice;

extern short            gsn_tumble;

extern short            gsn_swim;

extern short            gsn_sharpen;                   /* Sharpen skill --Cronel */

/* PRIESTS */
extern short            gsn_anoint;

/*
 * Cmd flag names --Shaddai
 */
extern const char      *const cmd_flags[];

/*
 * Utility macros.
 */
#define UMIN(a, b)  ((a) < (b) ? (a) : (b))
#define UMAX(a, b)  ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)  ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)  ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)  ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))

#define WAS_IMMORTAL(ch)  (!IS_NPC((ch)) && (ch)->pcdata->tmplevel > LEVEL_DEMIGOD)

#define NULLSTR( str ) ( !( str ) || ( str )[0] == '\0' )

/*
 * Old-style Bit manipulation macros
 *
 * The bit passed is the actual value of the bit (Use the BV## defines)
 */
#define IS_SET(flag, bit)  ((flag) & (bit))
#define SET_BIT(var, bit)  ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)  ((var) ^= (bit))

#define IS_PLR_FLAG(var, bit)         (!IS_NPC(var) && xIS_SET((var)->act, (bit)))
#define SET_PLR_FLAG(var, bit)         xSET_BIT((var)->act, (bit))
#define REMOVE_PLR_FLAG(var, bit)      xREMOVE_BIT((var)->act, (bit))

#define IS_BLIND(ch)    (xIS_SET((ch)->act, PLR_BLIND))

/* Multi-class prototypes */
int find_skill_level    args( ( CHAR_DATA *ch, int sn ) );

/*
 * Macros for accessing virtually unlimited bitvectors.  -Thoric
 *
 * Note that these macros use the bit number rather than the bit value
 * itself -- which means that you can only access _one_ bit at a time
 *
 * This code uses an array of integers
 */

/* The functions for these prototypes can be found in misc.c
 * They are up here because they are used by the macros below
 */
bool                    ext_is_empty( EXT_BV * bits );
void                    ext_clear_bits( EXT_BV * bits );
int                     ext_has_bits( EXT_BV * var, EXT_BV * bits );
bool                    ext_same_bits( EXT_BV * var, EXT_BV * bits );
void                    ext_set_bits( EXT_BV * var, EXT_BV * bits );
void                    ext_remove_bits( EXT_BV * var, EXT_BV * bits );
void                    ext_toggle_bits( EXT_BV * var, EXT_BV * bits );

/* Here are the extended bitvector macros: */
#define xIS_SET(var, bit)  ((var).bits[(bit) >> RSV] & 1 << ((bit) & XBM))
#define xSET_BIT(var, bit)  ((var).bits[(bit) >> RSV] |= 1 << ((bit) & XBM))
#define xSET_BITS(var, bit)  (ext_set_bits(&(var), &(bit)))
#define xREMOVE_BIT(var, bit)  ((var).bits[(bit) >> RSV] &= ~(1 << ((bit) & XBM)))
#define xREMOVE_BITS(var, bit)  (ext_remove_bits(&(var), &(bit)))
#define xTOGGLE_BIT(var, bit)  ((var).bits[(bit) >> RSV] ^= 1 << ((bit) & XBM))
#define xTOGGLE_BITS(var, bit)  (ext_toggle_bits(&(var), &(bit)))
#define xCLEAR_BITS(var)  (ext_clear_bits(&(var)))
#define xIS_EMPTY(var)  (ext_is_empty(&(var)))
#define xHAS_BITS(var, bit)  (ext_has_bits(&(var), &(bit)))
#define xSAME_BITS(var, bit)  (ext_same_bits(&(var), &(bit)))

/*
 * Memory allocation macros.
 */
#define VLD_STR(data) ((data) != NULL && (data)[0] != '\0')

#define CREATE(result, type, number)                                    \
do                                                                      \
{                                                                       \
   if (!((result) = (type *) calloc ((number), sizeof(type))))          \
   {                                                                    \
      perror("malloc failure");                                         \
      fprintf(stderr, "Malloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
      abort();                                                          \
   }                                                                    \
} while(0)

#define RECREATE(result,type,number)                                    \
do                                                                      \
{                                                                       \
   if(!((result) = (type *)realloc((result), sizeof(type) * (number)))) \
   {                                                                    \
      perror("realloc failure");                                        \
      fprintf(stderr, "Realloc failure @ %s:%d\n", __FILE__, __LINE__); \
      abort();                                                          \
   }                                                                    \
} while(0)

bool                    in_hash_table( const char *str );

#define DISPOSE(point)                      \
do                                          \
{                                           \
   if( (point) )                            \
   {                                        \
      free( (point) );                      \
      (point) = NULL;                       \
   }                                        \
} while(0)

/* Use this if it is a string and you want to run it through the stuff */
#define STRDISPOSE(point)                      \
do                                             \
{                                              \
   if( (point) )                               \
   {                                           \
      if( typeid((point)) == typeid(char*) || typeid((point)) == typeid(const char*) ) \
      {                                        \
         if( in_hash_table( (point) ) ) \
         {                                     \
            log_printf( "&RDISPOSE called on STRALLOC pointer: %s, line %d\n", __FILE__, __LINE__ ); \
            log_string( "Attempting to correct." ); \
            if( str_free( (point) ) == -1 ) \
               log_printf( "&RSTRFREEing bad pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         }                                     \
         else                                  \
            free( (point) );                   \
      }                                        \
      else                                     \
         free( (point) );                      \
      (point) = NULL;                          \
   }                                           \
   else                                          \
      (point) = NULL;                            \
} while(0)

#define STRALLOC(point)		str_alloc((point))
#define QUICKLINK(point)	quick_link((point))

#if defined(__FreeBSD__)
#define STRFREE(point)                          \
do                                              \
{                                               \
   if((point))                                  \
   {                                            \
      if( str_free((point)) == -1 )             \
         bug( "&RSTRFREEing bad pointer: %s, line %d", __FILE__, __LINE__ ); \
      (point) = NULL;                           \
   }                                            \
} while(0)
#else
#define STRFREE(point)                           \
do                                               \
{                                                \
   if((point))                                   \
   {                                             \
      if( !in_hash_table( (point) ) )            \
      {                                          \
         log_printf( "&RSTRFREE called on str_dup pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         log_string( "Attempting to correct." ); \
         free( (point) );                        \
      }                                          \
      else if( str_free((point)) == -1 )         \
         log_printf( "&RSTRFREEing bad pointer: %s, line %d\n", __FILE__, __LINE__ ); \
      (point) = NULL;                            \
   }                                             \
   else                                          \
      (point) = NULL;                            \
} while(0)
#endif

#define STRSET( point, new )    \
do                              \
{                               \
   STRFREE( (point) );          \
   (point) = STRALLOC( (new) ); \
} while(0)

/* double-linked list handling macros -Thoric */
/* Updated by Scion 8/6/1999 */
#define LINK( link, first, last, next, prev ) \
do                                          \
{                                           \
   if ( !(first) )                          \
   {                                        \
      (first) = (link);                     \
      (last) = (link);                      \
   }                                        \
   else                                     \
      (last)->next = (link);                \
   (link)->next = NULL;                     \
   if ((first) == (link))                   \
      (link)->prev = NULL;                  \
   else                                     \
      (link)->prev = (last);                \
   (last) = (link);                         \
} while(0)

#define INSERT(link, insert, first, next, prev) \
do                                              \
{                                               \
   (link)->prev = (insert)->prev;               \
   if ( !(insert)->prev )                       \
      (first) = (link);                         \
   else                                         \
      (insert)->prev->next = (link);            \
   (insert)->prev = (link);                     \
   (link)->next = (insert);                     \
} while(0)

#define UNLINK(link, first, last, next, prev)   \
do                                              \
{                                               \
   if ( !(link)->prev )                         \
   {                                            \
      (first) = (link)->next;                   \
      if ((first))                              \
         (first)->prev = NULL;                  \
   }                                            \
   else                                         \
   {                                            \
      (link)->prev->next = (link)->next;        \
   }                                            \
   if ( !(link)->next )                         \
   {                                            \
      (last) = (link)->prev;                    \
      if((last))                                \
         (last)->next = NULL;                   \
   }                                            \
   else                                         \
   {                                            \
      (link)->next->prev = (link)->prev;        \
   }                                            \
} while(0)

#define fclose(fp) \
do \
{ \
    fclose((fp)); \
    (fp) = NULL; \
} while(0)

#define ASSIGN_GSN(gsn, skill) \
do \
{ \
    if(((gsn) = skill_lookup((skill))) == -1) \
  fprintf(stderr, "ASSIGN_GSN: Skill %s not found.\n", \
  (skill)); \
} while(0)

#define CHECK_SUBRESTRICTED(ch) \
do \
{ \
    if((ch)->substate == SUB_RESTRICTED) \
    { \
  send_to_char("You cannot use this command from within another command.\n\r", ch); \
  return; \
    } \
} while(0)

#define error(ch) send_to_char("&WHuh? - Make sure you properly spelled your command.&D\n\r", (ch));

//Added a couple new character macros, they should prove useful. -Taon
#define IS_WIELDING(ch) (get_eq_char((ch), WEAR_WIELD) != NULL || get_eq_char((ch), WEAR_DUAL_WIELD) != NULL)
#define IS_PC_CHARMED(ch) (!IS_NPC((ch)) && IS_AFFECTED((ch), AFF_CHARM))

/*
 * Character macros.
 */
#define IS_NPC(ch)  (xIS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)  (get_trust((ch)) >= LEVEL_IMMORTAL && !IS_NPC(ch))
#define IS_DEMIGOD(ch)  (get_trust((ch)) >= LEVEL_DEMIGOD)
#define IS_AVATAR(ch)  ( (ch)->level >= LEVEL_AVATAR && (ch)->secondclass == -1 )
#define IS_DUALAVATAR(ch) ( (ch)->level >= LEVEL_AVATAR && (ch)->secondlevel >= LEVEL_AVATAR && (ch)->thirdclass == -1 )
#define IS_TRIAVATAR(ch) ( (ch)->level >= LEVEL_AVATAR && (ch)->secondlevel >= LEVEL_AVATAR && (ch)->thirdlevel >= LEVEL_AVATAR )
#define IS_HERO(ch)  (get_trust((ch)) >= LEVEL_HERO)
#define HAS_BODYPART(ch, part)  (!xIS_EMPTY((ch)->xflags) && xIS_SET((ch)->xflags, (part)))
#define IS_QUESTING(ch)  (xIS_SET((ch)->act, PLR_QUESTING))
#define IS_GOOD(ch)  ((ch)->alignment >= 350)
#define IS_EVIL(ch)  ((ch)->alignment <= -350)
#define IS_NEUTRAL(ch)  (!IS_GOOD((ch)) && !IS_EVIL((ch)))
#define GET_TIME_PLAYED(ch)     (((ch)->played + (current_time - (ch)->logon)) / 3600)
#define IS_ACT_FLAG(ch, flag)  (xIS_SET((ch)->act, flag))

#define IS_RESIS(ch, ris)   (IS_SET((ch)->resistant, ris))
#define IS_IMMUNE(ch, ris)  (IS_SET((ch)->immune, ris))
#define IS_SUSCEP(ch, ris)  (IS_SET((ch)->susceptible, ris))
#define IS_ATTACK(ch, atk)  (xIS_SET((ch)->attacks, atk))
#define IS_DEFENSE(ch, atk) (xIS_SET((ch)->defenses, atk))
#define GET_ALIGN(ch)       ((ch)->alignment)

#define IS_TWOHANDS(obj) ((obj)->value[4] == WEP_2H_LONG_BLADE || (obj)->value[4] == WEP_2H_BLUDGEON || (obj)->value[4] == WEP_2H_AXE)
#define IS_AWAKE(ch)  ((ch)->position > POS_SLEEPING)
#define GET_AC(ch)  ((ch)->armor + (IS_AWAKE((ch)) ? dex_app[get_curr_dex((ch))].defensive : 0) + VAMP_AC((ch)) + DRAGON_AC((ch)) + MONK_AC((ch)) + (ch)->level*2 )
#define GET_HITROLL(ch) (((ch)->race == RACE_DRAGON ? ((ch)->height / 48) : 0) + (ch)->hitroll + str_app[get_curr_str((ch))].tohit + (2-(abs((ch)->mental_state)/10)))

#define GET_DAMROLL(ch) ((((ch)->race == RACE_DRAGON && !IS_NPC((ch))) ? ((ch)->height / 48) : 0) \
+ (ch)->damroll + (ch)->damplus + str_app[get_curr_str((ch))].todam \
+ (((ch)->mental_state &&(ch)->mental_state < 15) ? 1 : 0))

#define IS_OUTSIDE(ch)          (!IS_SET(\
                                    (ch)->in_room->room_flags, \
                                    ROOM_INDOORS) && !IS_SET(\
                                    (ch)->in_room->room_flags, \
                                    ROOM_TUNNEL) && \
                                  !NO_WEATHER_SECT((ch)->in_room->sector_type) && \
                                !IS_SET((ch)->in_room->area->flags, AFLAG_INDOORS))

#define OUTSIDE(to_room) ( !IS_SET( (to_room)->room_flags, ROOM_INDOORS ) && !IS_SET( (to_room)->room_flags, ROOM_TUNNEL ) \
                         && !NO_WEATHER_SECT( (to_room)->sector_type ) )

#define NO_WEATHER_SECT(sect) ( (sect) == SECT_INSIDE || \
                              (sect) == SECT_UNDERWATER || \
                              (sect) == SECT_OCEANFLOOR || \
                              (sect) == SECT_UNDERGROUND )

#define IS_WATER_SECT(sect) ( (sect) == SECT_UNDERWATER || \
			    (sect) == SECT_OCEANFLOOR || \
                            (sect) == SECT_OCEAN      || \
                            (sect) == SECT_RIVER      || \
			    (sect) == SECT_WATER_SWIM || \
			    (sect) == SECT_WATER_NOSWIM || \
                            (sect) == SECT_LAKE )

#define IS_DRUNK(ch, drunk) ( number_percent() < ( (ch)->pcdata->condition[COND_DRUNK] * 2 / (drunk) ) )

#define IS_DEVOTED(ch) ( !IS_NPC((ch)) && (ch)->pcdata->deity)

#define IS_IDLE(ch) ( (ch)->pcdata && IS_SET( (ch)->pcdata->flags, PCFLAG_IDLE ) )
#define IS_PKILL(ch) ( (ch)->pcdata && IS_SET( (ch)->pcdata->flags, PCFLAG_DEADLY ) )

#define CAN_PKILL(ch) ( IS_PKILL((ch)) && (ch)->level >= 5 )

#define WAIT_EVAL(ch, npulse)  ((ch)->wait = (!IS_NPC(ch) && (ch)->pcdata->nuisance && \
     ((ch)->pcdata->nuisance->flags>4)) ? UMAX((ch)->wait, \
     (npulse+((ch)->pcdata->nuisance->flags-4) + \
     (ch)->pcdata->nuisance->power)) : \
     UMAX((ch)->wait, (npulse)))

#define WAIT_STATE(ch, npulse) ((ch)->wait=(!IS_NPC(ch)&&ch->pcdata->nuisance&&\
                              (ch->pcdata->nuisance->flags>4))?UMAX((ch)->wait,\
                              (npulse+((ch)->pcdata->nuisance->flags-4)+ \
                              ch->pcdata->nuisance->power)): \
                              UMAX((ch)->wait, (IS_IMMORTAL(ch) ? 0 : \
			      (!str_cmp((ch)->name, "Kidara") || !str_cmp((ch)->name, "Jordan")) ? 0 : (npulse))))

#define EXIT(ch, door) ( get_exit( (ch)->in_room, (door) ) )

#define CAN_GO(ch, door) ( EXIT( (ch),(door) ) && ( EXIT( (ch), (door) )->to_room != NULL ) && !IS_SET( EXIT( (ch), (door) )->exit_info, EX_CLOSED ) )

#define IS_FLOATING(ch) ( IS_AFFECTED( (ch), AFF_FLYING ) || IS_AFFECTED( (ch), AFF_FLOATING ) )

#define IS_VALID_SN(sn) ( (sn) >= 0 && (sn) < MAX_SKILL && skill_table[(sn)] && skill_table[(sn)]->name )
#define IS_VALID_HERB(sn) ( (sn) >= 0 && (sn) < MAX_HERB && herb_table[(sn)] && herb_table[(sn)]->name )
#define IS_VALID_DISEASE(sn) ( (sn) >= 0 && (sn) < MAX_DISEASE && disease_table[(sn)] && disease_table[(sn)]->name )

#define IS_PACIFIST(ch)  (IS_NPC((ch)) && xIS_SET((ch)->act, ACT_PACIFIST))

#define SPELL_FLAG(skill, flag) (xIS_SET((skill)->flags, (flag)))
#define SPELL_DAMAGE(skill)     (((skill)->info) & 7)
#define SPELL_ACTION(skill)     (((skill)->info >>  3) & 7)
#define SPELL_CLASS(skill)      (((skill)->info >>  6) & 7)
#define SPELL_POWER(skill)      (((skill)->info >>  9) & 3)
#define SPELL_SAVE(skill)       (((skill)->info >> 11) & 7)
#define SET_SDAM(skill, val)    ((skill)->info =  ((skill)->info & SDAM_MASK) + ((val) & 7))
#define SET_SACT(skill, val)    ((skill)->info =  ((skill)->info & SACT_MASK) + (((val) & 7) << 3))
#define SET_SCLA(skill, val)    ((skill)->info =  ((skill)->info & SCLA_MASK) + (((val) & 7) << 6))
#define SET_SPOW(skill, val)    ((skill)->info =  ((skill)->info & SPOW_MASK) + (((val) & 3) << 9))
#define SET_SSAV(skill, val)    ((skill)->info =  ((skill)->info & SSAV_MASK) + (((val) & 7) << 11))

/* Retired and guest imms. */
#define IS_RETIRED(ch) ((ch)->pcdata && IS_SET((ch)->pcdata->flags, PCFLAG_RETIRED))
#define IS_GUEST(ch) ((ch)->pcdata && IS_SET((ch)->pcdata->flags, PCFLAG_GUEST))
#define IS_TEMP(ch) ((ch)->pcdata && IS_SET((ch)->pcdata->flags, PCFLAG_TEMP))

/* RIS by gsn lookups. -- Altrag.
   Will need to add some || stuff for spells that need a special GSN. */

#define IS_FIRE(dt)        (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_FIRE)
#define IS_COLD(dt)        (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_COLD)
#define IS_ACID(dt)        (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ACID)
#define IS_ELECTRICITY(dt) (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ELECTRICITY)
#define IS_ENERGY(dt)      (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ENERGY)
#define IS_DRAIN(dt)       (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_DRAIN)
#define IS_POISON(dt)      (IS_VALID_SN((dt)) && SPELL_DAMAGE(skill_table[(dt)]) == SD_POISON)

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)  (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)  (xIS_SET((obj)->extra_flags, (stat)))
#define CAN_SHARPEN(obj) ((obj)->value[3] == DAM_HACK || (obj)->value[3] == DAM_STAB || (obj)->value[3] == DAM_SLASH \
   || (obj)->value[3] == DAM_LASH || (obj)->value[3] == DAM_PIERCE || (obj)->value[3] == DAM_THRUST \
   || ((obj)->item_type == ITEM_PROJECTILE && ((obj)->value[6] == 0 || (obj)->value[6] == 1 || (obj)->value[6] == 2 )))

/*
 * MudProg macros.      -Thoric
 */
#define HAS_PROG(what, prog)  (xIS_SET((what)->progtypes, (prog)))

const char             *PERS( CHAR_DATA *ch, CHAR_DATA *looker );

/* Volk - support for enhanced spells! */
#define can_speak(ch)         (!IS_AFFECTED((ch), AFF_SILENCE))
#define can_move(ch)          (!IS_AFFECTED((ch), AFF_PARALYSIS))
#define can_move_dir(ch)      (!IS_AFFECTED((ch), AFF_ROOT))

void                    log_string( const char *fmt, ... );

#define dam_message(ch, victim, dam, dt)  (new_dam_message((ch), (victim), (dam), (dt), NULL))

/*
 *  Defines for the command flags. --Shaddai
 */
#define  CMD_FLAG_POSSESS  BV00
#define CMD_FLAG_POLYMORPHED  BV01
#define CMD_WATCH  BV02                                /* FB */
#define CMD_ADMIN  BV03                                /* Allow Admin their commands */
#define CMD_ENFORCER  BV04                             /* Allow Enforcers their commands */
#define CMD_BUILDER  BV05                              /* Allow Builders their commands */
#define CMD_ROLEPLAYER  BV06                           /* Allow Rolplayers their commands 
                                                        */
#define CMD_FULLNAME  BV07                             /* Commands flagged with this must 
                                                        * be typed entirely */

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type
{
    CMDTYPE                *next;
    char                   *name;
    DO_FUN                 *do_fun;
    int                     flags;                     /* Added for Checking interpret
                                                        * stuff -Shaddai */
    short                   position;
    short                   level;
    short                   log;
    struct timerset         userec;
    int                     lag_count;                 /* count lag flags for this cmd -
                                                        * FB */
    short                   cshow;                     /* if command is shown in command
                                                        * list - Zarius */
};

/*
 * Structure for a social in the socials table.
 */
struct social_type
{
    SOCIALTYPE             *next;
    char                   *name;
    char                   *char_no_arg;
    char                   *others_no_arg;
    char                   *char_found;
    char                   *others_found;
    char                   *vict_found;
    char                   *char_auto;
    char                   *others_auto;
};

/*
 * Global constants.
 */

extern time_t           last_restore_all_time;
extern time_t           last_heaven_all_time;
extern time_t           boot_time;                     /* this should be moved down */
extern HOUR_MIN_SEC    *set_boot_time;
extern struct tm       *new_boot_time;
extern time_t           new_boot_time_t;

extern const struct str_app_type str_app[26];
extern const struct int_app_type int_app[26];
extern const struct wis_app_type wis_app[26];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];
extern const struct cha_app_type cha_app[26];
extern const struct lck_app_type lck_app[26];

extern struct at_color_type at_color_table[MAX_COLORS];
extern const struct liq_type liq_table[LIQ_MAX];

extern const char      *attack_table[DAM_MAX_TYPE];
extern const char      *attack_table_plural[DAM_MAX_TYPE];
extern const char     **const s_message_table[DAM_MAX_TYPE];
extern const char     **const p_message_table[DAM_MAX_TYPE];
extern const char      *weapon_skills[WEP_MAX];        /* Used in spell_identify */
extern const char      *projectiles[PROJ_MAX];         /* For archery weapons */

extern const char      *const skill_tname[];
extern short const      movement_loss[SECT_MAX];
extern const char      *const dir_name[];
extern const char      *const where_name[MAX_WHERE_NAME];
extern const short      rev_dir[];
extern const int        trap_door[];
extern const char      *const r_flags[];
extern const char      *const ex_flags[];
extern const char      *const w_flags[];
extern const char      *const sec_flags[];
extern const char      *const item_w_flags[];
extern const char      *const o_flags[];
extern const char      *const a_flags[];
extern const char      *const a2_flags[];
extern const char      *const o_types[];
extern const char      *const a_types[];
extern const char      *const act_flags[];
extern const char      *const plr_flags[];
extern const char      *const pc_flags[];
extern const char      *const trap_flags[];
extern const char      *const ris_flags[];
extern const char      *const trig_flags[];
extern const char      *const part_flags[];
extern const char      *const npc_class[];
extern const char      *const defense_flags[];
extern const char      *const attack_flags[];
extern const char      *const area_flags[];
extern const char      *const container_flags[];       /* OasisOLC */
extern const char      *const ex_pmisc[];
extern const char      *const ex_pwater[];
extern const char      *const ex_pair[];
extern const char      *const ex_pearth[];
extern const char      *const ex_pfire[];

extern char            *const temp_settings[];         /* FB */
extern char            *const precip_settings[];
extern char            *const wind_settings[];
extern char            *const preciptemp_msg[6][6];
extern char            *const windtemp_msg[6][6];
extern char            *const precip_msg[];
extern char            *const wind_msg[];

extern const char      *const curr_types[];
extern const char      *const cap_curr_types[];

extern char            *const raceflags[];

extern MPPAUSE_DATA    *first_mpwait;                  /* Storing pauseing mud progs */
extern MPPAUSE_DATA    *last_mpwait;                   /* - */
extern MPPAUSE_DATA    *current_mpwait;                /* - */

extern char            *target_name;
extern char            *ranged_target_name;
extern int              numobjsloaded;
extern int              nummobsloaded;
extern int              physicalobjects;
extern int              last_pkroom;
extern int              num_descriptors;
extern struct system_data sysdata;
extern int              top_sn;
extern int              top_herb;

extern CMDTYPE         *command_hash[126];

extern struct class_type *class_table[MAX_CLASS];
extern char            *title_table[MAX_CLASS][MAX_LEVEL + 1][2];

extern SKILLTYPE       *skill_table[MAX_SKILL];
extern SOCIALTYPE      *social_index[27];
extern CHAR_DATA       *cur_char;
extern ROOM_INDEX_DATA *cur_room;
extern bool             cur_char_died;
extern ch_ret           global_retcode;
extern SKILLTYPE       *herb_table[MAX_HERB];
extern SKILLTYPE       *disease_table[MAX_DISEASE];
extern DEITY_DATA      *first_deity;
extern DEITY_DATA      *last_deity;
extern int              cur_obj;
extern int              cur_obj_serial;
extern bool             cur_obj_extracted;
extern obj_ret          global_objcode;

extern HELP_DATA       *first_help;
extern HELP_DATA       *last_help;

extern WATCH_DATA      *first_watch;
extern WATCH_DATA      *last_watch;
extern RESERVE_DATA    *first_reserved;
extern RESERVE_DATA    *last_reserved;
extern CHAR_DATA       *first_char;
extern CHAR_DATA       *last_char;
extern DESCRIPTOR_DATA *first_descriptor;
extern DESCRIPTOR_DATA *last_descriptor;
extern OBJ_DATA        *first_object;
extern OBJ_DATA        *last_object;
extern AREA_DATA       *first_full_area;
extern AREA_DATA       *last_full_area;
extern AREA_DATA       *first_area;
extern AREA_DATA       *last_area;
extern AREA_DATA       *first_build;
extern AREA_DATA       *last_build;
extern AREA_DATA       *first_asort;
extern AREA_DATA       *last_asort;
extern AREA_DATA       *first_bsort;
extern AREA_DATA       *last_bsort;
extern AREA_DATA       *first_area_name;               /* alphanum. sort */
extern AREA_DATA       *last_area_name;                /* Fireblade */

extern TELEPORT_DATA   *first_teleport;
extern TELEPORT_DATA   *last_teleport;
extern OBJ_DATA        *extracted_obj_queue;
extern EXTRACT_CHAR_DATA *extracted_char_queue;
extern OBJ_DATA        *save_equipment[MAX_WEAR][MAX_LAYERS];
extern CHAR_DATA       *quitting_char;
extern CHAR_DATA       *loading_char;
extern CHAR_DATA       *saving_char;
extern OBJ_DATA        *all_obj;

extern time_t           current_time;
extern bool             fLogAll;
extern char             log_buf[];
extern TIME_INFO_DATA   time_info;

// extern WEATHER_DATA weather_info;
extern IMMORTAL_HOST   *immortal_host_start;
extern IMMORTAL_HOST   *immortal_host_end;

extern struct act_prog_data *mob_act_list;

extern bool             happyhouron;                   /* For happy hour! */

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */

 /*
  * OasisOLC Declarations
  */

DECLARE_DO_FUN( do_mphtown );
DECLARE_DO_FUN( do_mprmflag );
DECLARE_DO_FUN( do_mpflag );
DECLARE_DO_FUN( do_mprmtag );
DECLARE_DO_FUN( do_mptag );
DECLARE_DO_FUN( do_omedit );
DECLARE_DO_FUN( do_oredit );
DECLARE_DO_FUN( do_ooedit );
DECLARE_DO_FUN( do_ocopy );
DECLARE_DO_FUN( do_oclaim );
DECLARE_DO_FUN( do_fileio );
DECLARE_DO_FUN( do_addaffects );
DECLARE_DO_FUN( do_remember );
DECLARE_DO_FUN( do_remort );
DECLARE_DO_FUN( do_behead );
DECLARE_DO_FUN( do_fixskywild );
DECLARE_DO_FUN( do_vote );
DECLARE_DO_FUN( do_aobjsize );
DECLARE_DO_FUN( do_resize );
DECLARE_DO_FUN( do_showholiday );
DECLARE_DO_FUN( do_global_note );
DECLARE_DO_FUN( do_nread );
DECLARE_DO_FUN( do_global_boards );
DECLARE_DO_FUN( do_setmssp );
DECLARE_DO_FUN( do_buildwalk );
DECLARE_DO_FUN( do_rgrid );
DECLARE_DO_FUN( do_send );
DECLARE_DO_FUN( do_body_drop );
DECLARE_DO_FUN( do_astral_body );
DECLARE_DO_FUN( do_makeboard );
DECLARE_DO_FUN( do_bset );
DECLARE_DO_FUN( do_bstat );

DECLARE_DO_FUN( skill_notfound );
DECLARE_DO_FUN( do_aassign );
DECLARE_DO_FUN( do_activate );
DECLARE_DO_FUN( do_add_imm_host );
DECLARE_DO_FUN( do_advance );
DECLARE_DO_FUN( do_aexit );
DECLARE_DO_FUN( do_affected );
DECLARE_DO_FUN( do_afk );
DECLARE_DO_FUN( do_aid );
DECLARE_DO_FUN( do_alias );
DECLARE_DO_FUN( do_allow );
DECLARE_DO_FUN( do_angelfire );
DECLARE_DO_FUN( do_anoint );
DECLARE_DO_FUN( do_ansi );
DECLARE_DO_FUN( do_apply );
DECLARE_DO_FUN( do_appraise );
DECLARE_DO_FUN( do_areas );
DECLARE_DO_FUN( do_aset );
DECLARE_DO_FUN( do_assassinate );
DECLARE_DO_FUN( do_assault );
DECLARE_DO_FUN( do_astat );
DECLARE_DO_FUN( do_apurge );
DECLARE_DO_FUN( do_at );
DECLARE_DO_FUN( do_atobj );
DECLARE_DO_FUN( do_auction );
DECLARE_DO_FUN( do_authorize );
DECLARE_DO_FUN( do_backhand );
DECLARE_DO_FUN( do_arteries );
DECLARE_DO_FUN( do_backstab );
DECLARE_DO_FUN( do_balance );
DECLARE_DO_FUN( do_ball_of_fire );
DECLARE_DO_FUN( do_balzhur );
DECLARE_DO_FUN( do_bamfin );
DECLARE_DO_FUN( do_bamfout );
DECLARE_DO_FUN( do_banish );
DECLARE_DO_FUN( do_bankedit );
DECLARE_DO_FUN( do_contribute );
DECLARE_DO_FUN( do_cleararena );
DECLARE_DO_FUN( do_fortify );
DECLARE_DO_FUN( do_siege );
DECLARE_DO_FUN( do_tax );
DECLARE_DO_FUN( do_sabotage );
DECLARE_DO_FUN( do_bankinfo );
DECLARE_DO_FUN( do_ban );
DECLARE_DO_FUN( do_bash );
DECLARE_DO_FUN( do_bashdoor );
DECLARE_DO_FUN( do_beckon );
DECLARE_DO_FUN( do_graze );
DECLARE_DO_FUN( do_distract );
DECLARE_DO_FUN( do_berserk );
DECLARE_DO_FUN( do_bestow );
DECLARE_DO_FUN( do_bestowarea );
DECLARE_DO_FUN( do_bio );
DECLARE_DO_FUN( do_bite );
DECLARE_DO_FUN( do_blizzard );
DECLARE_DO_FUN( do_bloodlet );
DECLARE_DO_FUN( do_blue );
DECLARE_DO_FUN( do_bodybag );
DECLARE_DO_FUN( do_bolt );
DECLARE_DO_FUN( do_bone );
DECLARE_DO_FUN( do_brandish );
DECLARE_DO_FUN( do_breathe );
DECLARE_DO_FUN( do_brew );
DECLARE_DO_FUN( do_brittle_bone );
DECLARE_DO_FUN( do_broach );
DECLARE_DO_FUN( do_bug );
DECLARE_DO_FUN( do_burrow );
DECLARE_DO_FUN( do_bury );
DECLARE_DO_FUN( do_buy );
DECLARE_DO_FUN( do_calm );
DECLARE_DO_FUN( do_chameleon );
DECLARE_DO_FUN( do_cast );
DECLARE_DO_FUN( do_play );
DECLARE_DO_FUN( do_cedit );
DECLARE_DO_FUN( do_channels );
DECLARE_DO_FUN( do_charge );
DECLARE_DO_FUN( do_check_vnums );
DECLARE_DO_FUN( do_choke );
DECLARE_DO_FUN( do_circle );
DECLARE_DO_FUN( do_flamebreath );
DECLARE_DO_FUN( do_energy_containment );

DECLARE_DO_FUN( do_clans );
DECLARE_DO_FUN( do_makeclan );
DECLARE_DO_FUN( do_makecouncil );
DECLARE_DO_FUN( do_call );
DECLARE_DO_FUN( do_makedeity );
DECLARE_DO_FUN( do_setclan );
DECLARE_DO_FUN( do_setcouncil );
DECLARE_DO_FUN( do_showclan );
DECLARE_DO_FUN( do_showcouncil );
DECLARE_DO_FUN( do_setdeity );
DECLARE_DO_FUN( do_victories );
DECLARE_DO_FUN( do_classes );
DECLARE_DO_FUN( do_claw );
DECLARE_DO_FUN( do_clear );
DECLARE_DO_FUN( do_climb );
DECLARE_DO_FUN( do_close );
DECLARE_DO_FUN( do_cmdtable );
DECLARE_DO_FUN( do_color );
DECLARE_DO_FUN( do_commands );
DECLARE_DO_FUN( do_companion );
DECLARE_DO_FUN( do_comment );
DECLARE_DO_FUN( do_compare );
DECLARE_DO_FUN( do_cone );
DECLARE_DO_FUN( do_config );
DECLARE_DO_FUN( do_connect );
DECLARE_DO_FUN( do_consider );
DECLARE_DO_FUN( do_cook );
DECLARE_DO_FUN( do_copyover );                         /* Copyover command - Samson
                                                        * 3-31-01 */
DECLARE_DO_FUN( do_councils );
DECLARE_DO_FUN( do_forge );
DECLARE_DO_FUN( do_tan );
DECLARE_DO_FUN( do_hunt );
DECLARE_DO_FUN( do_bake );
DECLARE_DO_FUN( do_mix );
DECLARE_DO_FUN( do_gather );
DECLARE_DO_FUN( do_crawl );
DECLARE_DO_FUN( do_credits );
DECLARE_DO_FUN( do_crouch );
DECLARE_DO_FUN( do_cset );
DECLARE_DO_FUN( do_deadly );
DECLARE_DO_FUN( do_declare );
DECLARE_DO_FUN( do_defend );
DECLARE_DO_FUN( do_dehydrate );
DECLARE_DO_FUN( do_deities );
DECLARE_DO_FUN( do_delay );
DECLARE_DO_FUN( do_delete );
DECLARE_DO_FUN( do_demonic_sight );
DECLARE_DO_FUN( do_deny );
DECLARE_DO_FUN( do_dye );
DECLARE_DO_FUN( do_deposit );
DECLARE_DO_FUN( do_description );
DECLARE_DO_FUN( do_desecrate );
DECLARE_DO_FUN( do_destroy );
DECLARE_DO_FUN( do_destroychannel );
DECLARE_DO_FUN( do_destroyslay );                      /* New destroyslay command -
                                                        * Samson 8-3-98 */
DECLARE_DO_FUN( do_detrap );
DECLARE_DO_FUN( do_devote );
DECLARE_DO_FUN( do_devour );
DECLARE_DO_FUN( do_dig );
DECLARE_DO_FUN( do_sigilist );
DECLARE_DO_FUN( do_disarm );
DECLARE_DO_FUN( do_disconnect );
DECLARE_DO_FUN( do_dislodge );
DECLARE_DO_FUN( do_dismiss );
DECLARE_DO_FUN( do_dismount );
DECLARE_DO_FUN( do_dmesg );
DECLARE_DO_FUN( do_dnd );
DECLARE_DO_FUN( do_down );
DECLARE_DO_FUN( do_defensive_posturing );              // Taon
DECLARE_DO_FUN( do_cexp );
DECLARE_DO_FUN( do_dexp );                             /* Double exp */
DECLARE_DO_FUN( do_drag );
DECLARE_DO_FUN( do_draw );
DECLARE_DO_FUN( do_dream_walk );
DECLARE_DO_FUN( do_drink );
DECLARE_DO_FUN( do_drop );
DECLARE_DO_FUN( do_drowsy );
DECLARE_DO_FUN( do_druidic_hymn );

DECLARE_DO_FUN( do_east );
DECLARE_DO_FUN( do_eat );
DECLARE_DO_FUN( do_echo );
DECLARE_DO_FUN( do_eldritch_bolt );
DECLARE_DO_FUN( do_elevate );
DECLARE_DO_FUN( do_emote );
DECLARE_DO_FUN( do_semote );
DECLARE_DO_FUN( do_empty );
DECLARE_DO_FUN( do_enrage );
DECLARE_DO_FUN( do_enter );
DECLARE_DO_FUN( do_equipment );
DECLARE_DO_FUN( do_evac_crescendo );
DECLARE_DO_FUN( do_exchange );
DECLARE_DO_FUN( do_examine );
DECLARE_DO_FUN( do_exits );
DECLARE_DO_FUN( do_expire );
DECLARE_DO_FUN( do_explore );
DECLARE_DO_FUN( do_expratio );
DECLARE_DO_FUN( do_extreme );
DECLARE_DO_FUN( do_eye );
DECLARE_DO_FUN( do_faith );                            // Taon
DECLARE_DO_FUN( do_wilderness );                       // Taon
DECLARE_DO_FUN( do_fclear );
DECLARE_DO_FUN( do_fear );
DECLARE_DO_FUN( do_feed );
//DECLARE_DO_FUN(do_flamebreath); //Taon
DECLARE_DO_FUN( do_break );
DECLARE_DO_FUN( do_cannibalize );
DECLARE_DO_FUN( do_brain_boil );
DECLARE_DO_FUN( do_death_embrace );
DECLARE_DO_FUN( do_feign_death );
DECLARE_DO_FUN( do_festering_wound );
DECLARE_DO_FUN( do_fill );
DECLARE_DO_FUN( do_findnote );
DECLARE_DO_FUN( do_find_trap );
DECLARE_DO_FUN( do_fire );
DECLARE_DO_FUN( do_fixchar );
DECLARE_DO_FUN( do_flee );
DECLARE_DO_FUN( do_flint_fire );
DECLARE_DO_FUN( do_fly );
DECLARE_DO_FUN( do_fly_home );
DECLARE_DO_FUN( do_foldarea );
DECLARE_DO_FUN( do_follow );
DECLARE_DO_FUN( do_for );
DECLARE_DO_FUN( do_force );
DECLARE_DO_FUN( do_forceclose );
DECLARE_DO_FUN( do_fprompt );
DECLARE_DO_FUN( do_fquit );                            /* Gorog */
DECLARE_DO_FUN( do_form_password );
DECLARE_DO_FUN( do_freeze );
DECLARE_DO_FUN( do_fshow );
DECLARE_DO_FUN( do_ftick );
DECLARE_DO_FUN( do_harm_touch );                       // Vladaar
DECLARE_DO_FUN( do_human_form );
DECLARE_DO_FUN( do_hallowed_blow );
DECLARE_DO_FUN( do_heart_grab );
DECLARE_DO_FUN( do_decree_decay );                     // Vladaar
DECLARE_DO_FUN( do_recoil );                           // Taon
DECLARE_DO_FUN( do_red );
DECLARE_DO_FUN( do_remains );
DECLARE_DO_FUN( do_renumber );
DECLARE_DO_FUN( do_retreat );
DECLARE_DO_FUN( do_reward );
DECLARE_DO_FUN( do_rush );
DECLARE_DO_FUN( do_keen_eye );                         // Taon
DECLARE_DO_FUN( do_fury );                             // Taon
DECLARE_DO_FUN( do_ftag );
DECLARE_DO_FUN( do_zot );                              // Taon
DECLARE_DO_FUN( do_ensnare );                          // Taon
DECLARE_DO_FUN( do_layhands );                         // Taon
DECLARE_DO_FUN( do_laysiege );
DECLARE_DO_FUN( do_sidestep );                         // Taon
DECLARE_DO_FUN( do_daze );                             // Taon
DECLARE_DO_FUN( do_root );                             // Taon
DECLARE_DO_FUN( do_rollcall );
DECLARE_DO_FUN( do_roster );
DECLARE_DO_FUN( do_rousing_tune );
DECLARE_DO_FUN( do_iron_skin );                        // Taon
DECLARE_DO_FUN( do_impale );
DECLARE_DO_FUN( do_ritual );                           // Taon
DECLARE_DO_FUN( do_boost );                            // Taon
DECLARE_DO_FUN( do_untangle );                         // Taon
DECLARE_DO_FUN( do_gauge );
DECLARE_DO_FUN( do_get );
DECLARE_DO_FUN( do_gfighting );
DECLARE_DO_FUN( do_give );
DECLARE_DO_FUN( do_aglance );
DECLARE_DO_FUN( do_glance );
DECLARE_DO_FUN( do_goto );
DECLARE_DO_FUN( do_gouge );
DECLARE_DO_FUN( do_graft_weapon );
DECLARE_DO_FUN( do_gratz );
DECLARE_DO_FUN( do_grendals_stance );
DECLARE_DO_FUN( do_group );
DECLARE_DO_FUN( do_gtell );
DECLARE_DO_FUN( do_gust_of_wind );
DECLARE_DO_FUN( do_gut );
DECLARE_DO_FUN( do_deathroll );
DECLARE_DO_FUN( do_ballistic );
DECLARE_DO_FUN( do_entangle );
DECLARE_DO_FUN( do_tangle );
DECLARE_DO_FUN( do_dominate );
DECLARE_DO_FUN( do_hellfire );
DECLARE_DO_FUN( do_submerged );
DECLARE_DO_FUN( do_gwhere );
DECLARE_DO_FUN( do_happyhour );
DECLARE_DO_FUN( do_harmonic_melody );
DECLARE_DO_FUN( do_hate );
DECLARE_DO_FUN( do_healing_thoughts );
DECLARE_DO_FUN( do_heavens_blessing );
DECLARE_DO_FUN( do_hedit );
DECLARE_DO_FUN( do_hell );
DECLARE_DO_FUN( do_help );
DECLARE_DO_FUN( do_helpcheck );
DECLARE_DO_FUN( do_hide );
DECLARE_DO_FUN( do_higher_magic );
DECLARE_DO_FUN( do_hintedit );
DECLARE_DO_FUN( do_hitall );
DECLARE_DO_FUN( do_hl );
DECLARE_DO_FUN( do_hlist );
DECLARE_DO_FUN( do_holylight );
DECLARE_DO_FUN( do_homepage );
DECLARE_DO_FUN( do_accessories );                      // Only command for housing
                                                       // accessories.
DECLARE_DO_FUN( do_account );
DECLARE_DO_FUN( do_gohome );                           // Command for a player to be
                                                       // instantly transported to their
                                                       // residence.
DECLARE_DO_FUN( do_homebuy );                          // Main housing auction command.
DECLARE_DO_FUN( do_house );                            // House personalization command
                                                       // and also immortal command to
                                                       // manually
                                 // edit residences.
DECLARE_DO_FUN( do_residence );                        // Command to list current houses
                                                       // and their stats.
DECLARE_DO_FUN( do_sellhouse );                        // Player command to sell their
                                                       // residence and place it on
                                                       // auction.

DECLARE_DO_FUN( do_hset );
DECLARE_DO_FUN( do_htowns );
DECLARE_DO_FUN( do_city );
DECLARE_DO_FUN( do_htown_outcast );
DECLARE_DO_FUN( do_ide );
DECLARE_DO_FUN( do_idea );
DECLARE_DO_FUN( do_ignore );
DECLARE_DO_FUN( do_immortalize );
DECLARE_DO_FUN( do_imm_morph );
DECLARE_DO_FUN( do_imm_unmorph );
DECLARE_DO_FUN( do_induct );
DECLARE_DO_FUN( do_intro );
DECLARE_DO_FUN( do_influence );
DECLARE_DO_FUN( do_inferno );
DECLARE_DO_FUN( do_installarea );
DECLARE_DO_FUN( do_instaroom );
DECLARE_DO_FUN( do_instazone );
DECLARE_DO_FUN( do_invade );
DECLARE_DO_FUN( do_inventory );
DECLARE_DO_FUN( do_invis );
DECLARE_DO_FUN( do_ipcompare );
DECLARE_DO_FUN( do_journal );
DECLARE_DO_FUN( do_judge );
DECLARE_DO_FUN( do_judo );
DECLARE_DO_FUN( do_khistory );
DECLARE_DO_FUN( do_kick );
DECLARE_DO_FUN( do_heavy_hands );
DECLARE_DO_FUN( do_phase );
DECLARE_DO_FUN( do_know_enemy );
DECLARE_DO_FUN( do_kill );
DECLARE_DO_FUN( do_kinetic_barrier );
DECLARE_DO_FUN( do_lair );
DECLARE_DO_FUN( do_land );
DECLARE_DO_FUN( do_languages );
DECLARE_DO_FUN( do_last );
DECLARE_DO_FUN( do_laws );
DECLARE_DO_FUN( do_leave );
DECLARE_DO_FUN( do_leech );
DECLARE_DO_FUN( do_extract_skeleton );
DECLARE_DO_FUN( do_lesser_skeleton );
DECLARE_DO_FUN( do_animal_companion );
DECLARE_DO_FUN( do_conjure_elemental );
DECLARE_DO_FUN( do_level );
DECLARE_DO_FUN( do_lick );
DECLARE_DO_FUN( do_life );
DECLARE_DO_FUN( do_light );
DECLARE_DO_FUN( do_list );
DECLARE_DO_FUN( do_listcurrency );
DECLARE_DO_FUN( do_listen );
DECLARE_DO_FUN( do_litterbug );
DECLARE_DO_FUN( do_loadup );
DECLARE_DO_FUN( do_locations );
DECLARE_DO_FUN( do_prefresh );
DECLARE_DO_FUN( do_proclaim );
DECLARE_DO_FUN( do_lock );
DECLARE_DO_FUN( do_log );
DECLARE_DO_FUN( do_look );
// Volk added automap function!
DECLARE_DO_FUN( do_lookmap );
DECLARE_DO_FUN( do_low_purge );
DECLARE_DO_FUN( do_maim );
DECLARE_DO_FUN( do_makechannel );
DECLARE_DO_FUN( do_makehtown );
DECLARE_DO_FUN( do_makecity );
DECLARE_DO_FUN( do_makerepair );
DECLARE_DO_FUN( do_makerooms );
DECLARE_DO_FUN( do_makeshop );
DECLARE_DO_FUN( do_makeslay );                         /* New makeslay command - Samson
                                                        * 8-3-98 */
DECLARE_DO_FUN( do_makestafflist );
DECLARE_DO_FUN( do_massign );
DECLARE_DO_FUN( do_meditate );
DECLARE_DO_FUN( do_trance );
DECLARE_DO_FUN( do_memory );
DECLARE_DO_FUN( do_mental_assault );
DECLARE_DO_FUN( do_mcreate );
DECLARE_DO_FUN( do_mdelete );
DECLARE_DO_FUN( do_mfind );
DECLARE_DO_FUN( do_mine );
DECLARE_DO_FUN( do_minvoke );
DECLARE_DO_FUN( do_mistwalk );
DECLARE_DO_FUN( do_mlist );
DECLARE_DO_FUN( do_money );
DECLARE_DO_FUN( do_morphcreate );
DECLARE_DO_FUN( do_morphdestroy );
DECLARE_DO_FUN( do_morphset );
DECLARE_DO_FUN( do_morphstat );
DECLARE_DO_FUN( do_mortalize );
DECLARE_DO_FUN( do_mortify );
DECLARE_DO_FUN( do_mount );
DECLARE_DO_FUN( do_mset );
DECLARE_DO_FUN( do_mstat );
DECLARE_DO_FUN( do_vstat );
DECLARE_DO_FUN( do_vclear );
DECLARE_DO_FUN( do_murder );
DECLARE_DO_FUN( do_mwhere );
DECLARE_DO_FUN( do_name );
DECLARE_DO_FUN( do_newbierecall );
DECLARE_DO_FUN( do_newbieset );
DECLARE_DO_FUN( do_news );
DECLARE_DO_FUN( do_newzones );
DECLARE_DO_FUN( do_noemote );
DECLARE_DO_FUN( do_noresolve );
DECLARE_DO_FUN( do_north );
DECLARE_DO_FUN( do_northeast );
DECLARE_DO_FUN( do_northwest );
DECLARE_DO_FUN( do_noteroom );
DECLARE_DO_FUN( do_mailroom );
DECLARE_DO_FUN( do_notell );
DECLARE_DO_FUN( do_notitle );
DECLARE_DO_FUN( do_nuisance );
DECLARE_DO_FUN( do_oassign );
DECLARE_DO_FUN( do_lore );
DECLARE_DO_FUN( do_object_reading );
DECLARE_DO_FUN( do_ocreate );
DECLARE_DO_FUN( do_odelete );
DECLARE_DO_FUN( do_ofind );
DECLARE_DO_FUN( do_oinvade );
DECLARE_DO_FUN( do_oinvoke );
DECLARE_DO_FUN( do_olist );
DECLARE_DO_FUN( do_opcopy );
DECLARE_DO_FUN( do_open );
DECLARE_DO_FUN( do_order );
DECLARE_DO_FUN( do_oset );
DECLARE_DO_FUN( do_ostat );
DECLARE_DO_FUN( do_ouch );
DECLARE_DO_FUN( do_outcast );
DECLARE_DO_FUN( do_revoke );
DECLARE_DO_FUN( do_owhere );
DECLARE_DO_FUN( do_pager );
DECLARE_DO_FUN( do_pardon );
DECLARE_DO_FUN( do_passage );
DECLARE_DO_FUN( do_password );
DECLARE_DO_FUN( do_pawn );
DECLARE_DO_FUN( do_pcrename );
DECLARE_DO_FUN( do_peace );
DECLARE_DO_FUN( do_pfiles );
DECLARE_DO_FUN( do_pfreload );
DECLARE_DO_FUN( do_pick );
DECLARE_DO_FUN( do_pluck );
DECLARE_DO_FUN( do_poison_weapon );
DECLARE_DO_FUN( do_practice );
DECLARE_DO_FUN( do_prayer );                           // Taon
DECLARE_DO_FUN( do_forfeit );                          // Taon
DECLARE_DO_FUN( do_arena );                            // Taon
DECLARE_DO_FUN( do_decline );                          // Taon
DECLARE_DO_FUN( do_accept );                           // Taon
DECLARE_DO_FUN( do_challenge );                        // Taon
DECLARE_DO_FUN( do_tears );
DECLARE_DO_FUN( do_textspeed );                        // Volk :P
DECLARE_DO_FUN( do_closearena );                       // Taon
DECLARE_DO_FUN( do_flaming_whip );                     // Taon
DECLARE_DO_FUN( do_promote );
DECLARE_DO_FUN( do_prompt );
DECLARE_DO_FUN( do_pull );
DECLARE_DO_FUN( do_punch );
DECLARE_DO_FUN( do_purge );
DECLARE_DO_FUN( do_push );
DECLARE_DO_FUN( do_put );
DECLARE_DO_FUN( do_psionic_blast );
DECLARE_DO_FUN( do_quaff );
DECLARE_DO_FUN( do_qui );
DECLARE_DO_FUN( do_quit );
DECLARE_DO_FUN( do_quivering_palm );
DECLARE_DO_FUN( do_races );
DECLARE_DO_FUN( do_rage );
DECLARE_DO_FUN( do_rank );
DECLARE_DO_FUN( do_ratings );
DECLARE_DO_FUN( do_rap );
DECLARE_DO_FUN( do_rassign );
DECLARE_DO_FUN( do_rat );
DECLARE_DO_FUN( do_rdelete );
DECLARE_DO_FUN( do_reboot );
DECLARE_DO_FUN( do_recall );
DECLARE_DO_FUN( do_cityrecall );
DECLARE_DO_FUN( do_clanrecall );                       // Taon
DECLARE_DO_FUN( do_recho );
DECLARE_DO_FUN( do_recite );
DECLARE_DO_FUN( do_redit );
DECLARE_DO_FUN( do_regoto );
DECLARE_DO_FUN( do_remove );
DECLARE_DO_FUN( do_rent );
DECLARE_DO_FUN( do_repair );
DECLARE_DO_FUN( do_repairset );
DECLARE_DO_FUN( do_repairshops );
DECLARE_DO_FUN( do_repairstat );
DECLARE_DO_FUN( do_reply );
DECLARE_DO_FUN( do_report );
DECLARE_DO_FUN( do_reserve );
DECLARE_DO_FUN( do_reset );
DECLARE_DO_FUN( do_rest );
DECLARE_DO_FUN( do_restore );
DECLARE_DO_FUN( do_restoretime );
DECLARE_DO_FUN( do_restrict );
DECLARE_DO_FUN( do_retire );
DECLARE_DO_FUN( do_retran );
DECLARE_DO_FUN( do_return );
DECLARE_DO_FUN( do_righteous_blow );
DECLARE_DO_FUN( do_rip );
DECLARE_DO_FUN( do_rlist );
DECLARE_DO_FUN( do_roll );
DECLARE_DO_FUN( do_rset );
DECLARE_DO_FUN( do_rstat );
DECLARE_DO_FUN( do_save );
DECLARE_DO_FUN( do_silentsave );
DECLARE_DO_FUN( do_saveall );
DECLARE_DO_FUN( do_savearea );
DECLARE_DO_FUN( do_scan );
DECLARE_DO_FUN( do_scatter );
DECLARE_DO_FUN( do_oscatter );                         // Taon
DECLARE_DO_FUN( do_bscore );
DECLARE_DO_FUN( do_score );
DECLARE_DO_FUN( do_addlandmark );
DECLARE_DO_FUN( do_removelandmark );
DECLARE_DO_FUN( do_setlandmark );
DECLARE_DO_FUN( do_landmark );
DECLARE_DO_FUN( do_scribe );
DECLARE_DO_FUN( do_search );
DECLARE_DO_FUN( do_sedit );
DECLARE_DO_FUN( do_sell );
DECLARE_DO_FUN( do_set_boot_time );
DECLARE_DO_FUN( do_setchannel );
DECLARE_DO_FUN( do_setclass );
DECLARE_DO_FUN( do_setcurrency );
DECLARE_DO_FUN( do_sethtown );
DECLARE_DO_FUN( do_setcity );
DECLARE_DO_FUN( do_setquest );
DECLARE_DO_FUN( do_setrace );
DECLARE_DO_FUN( do_setslay );                          /* New setslay command - Samson
                                                        * 8-3-98 */
DECLARE_DO_FUN( do_setweather );
DECLARE_DO_FUN( do_shadowform );
DECLARE_DO_FUN( do_shapeshift );
DECLARE_DO_FUN( do_sharpen );
DECLARE_DO_FUN( do_sheath );
DECLARE_DO_FUN( do_shelter );
DECLARE_DO_FUN( do_shield );
DECLARE_DO_FUN( do_shops );
DECLARE_DO_FUN( do_shopset );
DECLARE_DO_FUN( do_shopstat );
DECLARE_DO_FUN( do_shove );
DECLARE_DO_FUN( do_showchannels );
DECLARE_DO_FUN( do_showclass );
DECLARE_DO_FUN( do_showdeity );
DECLARE_DO_FUN( do_showhtown );
DECLARE_DO_FUN( do_showcity );
DECLARE_DO_FUN( do_showquest );
DECLARE_DO_FUN( do_showrace );
DECLARE_DO_FUN( do_showslay );                         /* New showslay command - Samson
                                                        * 8-3-98 */
DECLARE_DO_FUN( do_showweather );                      /* FB */
DECLARE_DO_FUN( do_shutdown );
DECLARE_DO_FUN( do_shriek );
DECLARE_DO_FUN( do_shrieking_note );
DECLARE_DO_FUN( do_shrink );
DECLARE_DO_FUN( do_train );
DECLARE_DO_FUN( do_truesight );
DECLARE_DO_FUN( do_silence );
DECLARE_DO_FUN( do_sit );
DECLARE_DO_FUN( do_skills );
DECLARE_DO_FUN( do_skin );
DECLARE_DO_FUN( do_skip );
DECLARE_DO_FUN( do_slay );
DECLARE_DO_FUN( do_sleep );
DECLARE_DO_FUN( do_slice );
DECLARE_DO_FUN( do_slist );
DECLARE_DO_FUN( do_slookup );
DECLARE_DO_FUN( do_smell );
DECLARE_DO_FUN( do_smoke );
DECLARE_DO_FUN( do_smuggle );
DECLARE_DO_FUN( do_snatch );
DECLARE_DO_FUN( do_sneak );
DECLARE_DO_FUN( do_snoop );
DECLARE_DO_FUN( do_sober );
DECLARE_DO_FUN( do_solve );
DECLARE_DO_FUN( do_socials );
DECLARE_DO_FUN( do_south );
DECLARE_DO_FUN( do_southeast );
DECLARE_DO_FUN( do_southwest );
DECLARE_DO_FUN( do_speak );
DECLARE_DO_FUN( do_speed );
DECLARE_DO_FUN( do_spells );
DECLARE_DO_FUN( do_spike );
DECLARE_DO_FUN( do_spirits );
DECLARE_DO_FUN( do_split );
DECLARE_DO_FUN( do_sset );
DECLARE_DO_FUN( do_stafflist );
DECLARE_DO_FUN( do_stand );
DECLARE_DO_FUN( do_stat );
DECLARE_DO_FUN( do_statreport );
DECLARE_DO_FUN( do_statshield );
DECLARE_DO_FUN( do_status );                           // Taon
DECLARE_DO_FUN( do_steal );
DECLARE_DO_FUN( do_beep );                             // Taon
DECLARE_DO_FUN( do_sting );
DECLARE_DO_FUN( do_stirring_ballad );
DECLARE_DO_FUN( do_ottos_dance );
DECLARE_DO_FUN( do_unholy_melody );
DECLARE_DO_FUN( do_sound_waves );
DECLARE_DO_FUN( do_stomp );
DECLARE_DO_FUN( do_strip );
DECLARE_DO_FUN( do_stun );
DECLARE_DO_FUN( do_supplicate );
DECLARE_DO_FUN( do_sustenance );
DECLARE_DO_FUN( do_surreal_speed );
DECLARE_DO_FUN( do_sustain_self );                     // Taon
DECLARE_DO_FUN( do_switch );
DECLARE_DO_FUN( do_tackle );
DECLARE_DO_FUN( do_tag );
DECLARE_DO_FUN( do_siphon_strength );
DECLARE_DO_FUN( do_tail_swipe );
DECLARE_DO_FUN( do_tamp );
DECLARE_DO_FUN( do_taunt );
DECLARE_DO_FUN( do_teacher );
DECLARE_DO_FUN( do_torture_mind );
DECLARE_DO_FUN( do_listteachers );
DECLARE_DO_FUN( do_listscholar );
DECLARE_DO_FUN( do_telepathy );
DECLARE_DO_FUN( do_tell );
DECLARE_DO_FUN( do_thaitin );
DECLARE_DO_FUN( do_thicken_skin );
DECLARE_DO_FUN( do_thunderous_hymn );
DECLARE_DO_FUN( do_time );
DECLARE_DO_FUN( do_timezone );
DECLARE_DO_FUN( do_holidays );
DECLARE_DO_FUN( do_saveholiday );
DECLARE_DO_FUN( do_setholiday );
DECLARE_DO_FUN( do_title );
DECLARE_DO_FUN( do_tutorial );
DECLARE_DO_FUN( do_track );
DECLARE_DO_FUN( do_transfer );
DECLARE_DO_FUN( do_trash );
DECLARE_DO_FUN( do_trust );
DECLARE_DO_FUN( do_typo );
DECLARE_DO_FUN( do_unbolt );
DECLARE_DO_FUN( do_unhell );
DECLARE_DO_FUN( do_unholy_sphere );
DECLARE_DO_FUN( do_unlock );
DECLARE_DO_FUN( do_unnuisance );
DECLARE_DO_FUN( do_unsheath );
DECLARE_DO_FUN( do_unshift );
DECLARE_DO_FUN( do_unsilence );
DECLARE_DO_FUN( do_up );
DECLARE_DO_FUN( do_qpset );
DECLARE_DO_FUN( do_glory );                            // Taon
DECLARE_DO_FUN( do_qpstat );
DECLARE_DO_FUN( do_users );
DECLARE_DO_FUN( do_value );
DECLARE_DO_FUN( do_vault );
DECLARE_DO_FUN( do_vampiric_strength );
DECLARE_DO_FUN( do_vassign );
DECLARE_DO_FUN( do_version );
DECLARE_DO_FUN( do_viewskills );
DECLARE_DO_FUN( do_visible );
DECLARE_DO_FUN( do_vnums );
DECLARE_DO_FUN( do_vomit );
DECLARE_DO_FUN( do_vsearch );
DECLARE_DO_FUN( do_wailing );
DECLARE_DO_FUN( do_wake );
DECLARE_DO_FUN( do_ward );
DECLARE_DO_FUN( do_warn );
DECLARE_DO_FUN( do_wear );
DECLARE_DO_FUN( do_weapons );
DECLARE_DO_FUN( do_songs );
DECLARE_DO_FUN( do_weather );
DECLARE_DO_FUN( do_west );
DECLARE_DO_FUN( do_where );
DECLARE_DO_FUN( do_whereis );
DECLARE_DO_FUN( do_whip_of_murazor );
DECLARE_DO_FUN( do_whisper );
DECLARE_DO_FUN( do_who );
DECLARE_DO_FUN( do_whois );
DECLARE_DO_FUN( do_icq_number );                       /* User can enter icq# for finger
                                                        * - Samson 1-4-99 */
DECLARE_DO_FUN( do_email );                            /* User can enter email addy for
                                                        * finger - Samson 4-18-98 */
DECLARE_DO_FUN( do_finger );                           /* Finger command - Samson 4-6-98 */
DECLARE_DO_FUN( do_wizinfo );                          /* Wizinfo command - Samson 6-6-99 
                                                        */
DECLARE_DO_FUN( do_privacy );                          /* Privacy flag toggle - Samson
                                                        * 6-11-99 */

DECLARE_DO_FUN( do_wimpy );
DECLARE_DO_FUN( do_wings );
DECLARE_DO_FUN( do_withdraw );
DECLARE_DO_FUN( do_wizhelp );
DECLARE_DO_FUN( do_wizlist );
DECLARE_DO_FUN( do_wizlock );
DECLARE_DO_FUN( do_wpeace );
DECLARE_DO_FUN( do_zap );
DECLARE_DO_FUN( do_zones );

/* spec funcs */
/*
 * The following special functions are available for mobiles.
 */
DECLARE_SPEC_FUN( spec_breath_any );
DECLARE_SPEC_FUN( spec_breath_acid );
DECLARE_SPEC_FUN( spec_breath_fire );
DECLARE_SPEC_FUN( spec_breath_frost );
DECLARE_SPEC_FUN( spec_breath_gas );
DECLARE_SPEC_FUN( spec_breath_lightning );
DECLARE_SPEC_FUN( spec_cast_adept );
DECLARE_SPEC_FUN( spec_cast_cleric );
DECLARE_SPEC_FUN( spec_cast_mage );
DECLARE_SPEC_FUN( spec_cast_undead );
DECLARE_SPEC_FUN( spec_executioner );
DECLARE_SPEC_FUN( spec_fido );
DECLARE_SPEC_FUN( spec_guard );
DECLARE_SPEC_FUN( spec_janitor );
DECLARE_SPEC_FUN( spec_mayor );
DECLARE_SPEC_FUN( spec_poison );
DECLARE_SPEC_FUN( spec_thief );
DECLARE_SPEC_FUN( spec_greet );
DECLARE_SPEC_FUN( spec_tiamat );
DECLARE_SPEC_FUN( spec_basic_ai );

/* mob prog stuff */
DECLARE_DO_FUN( do_mp_close_passage );
DECLARE_DO_FUN( do_mp_damage );
DECLARE_DO_FUN( do_mp_log );
DECLARE_DO_FUN( do_mp_restore );
DECLARE_DO_FUN( do_mp_open_passage );
DECLARE_DO_FUN( do_mp_practice );
DECLARE_DO_FUN( do_mp_slay );
DECLARE_DO_FUN( do_mpadvance );
DECLARE_DO_FUN( do_mpasound );
DECLARE_DO_FUN( do_mpasupress );
DECLARE_DO_FUN( do_mpgenmob );
DECLARE_DO_FUN( do_mpclanmob );
DECLARE_DO_FUN( do_mptutorial );
DECLARE_DO_FUN( do_mpoutcast );                        // Taon
DECLARE_DO_FUN( do_mpinduct );                         // Taon
DECLARE_DO_FUN( do_mpboat );
DECLARE_DO_FUN( do_mptrade );
DECLARE_DO_FUN( do_mpclear );                          // Taon
DECLARE_DO_FUN( do_mpassist );                         // Taon
DECLARE_DO_FUN( do_mpat );
DECLARE_DO_FUN( do_mpcopy );
DECLARE_DO_FUN( do_mpdream );
DECLARE_DO_FUN( do_mp_deposit );
DECLARE_DO_FUN( do_mp_fill_in );
DECLARE_DO_FUN( do_mp_withdraw );
DECLARE_DO_FUN( do_mpecho );
DECLARE_DO_FUN( do_mpechoaround );
DECLARE_DO_FUN( do_mpechoat );
DECLARE_DO_FUN( do_mpechozone );
DECLARE_DO_FUN( do_mpedit );
DECLARE_DO_FUN( do_opedit );
DECLARE_DO_FUN( do_rpedit );
DECLARE_DO_FUN( do_rprate );
DECLARE_DO_FUN( do_mpforce );
DECLARE_DO_FUN( do_mpinvis );
DECLARE_DO_FUN( do_mpgoto );
DECLARE_DO_FUN( do_mpjunk );
DECLARE_DO_FUN( do_mpkill );
DECLARE_DO_FUN( do_mpmload );
DECLARE_DO_FUN( do_mpmset );
DECLARE_DO_FUN( do_mpcity );
DECLARE_DO_FUN( do_mpnothing );
DECLARE_DO_FUN( do_mpoload );
DECLARE_DO_FUN( do_mposet );
DECLARE_DO_FUN( do_mppardon );
DECLARE_DO_FUN( do_mppeace );
DECLARE_DO_FUN( do_mpeat );
DECLARE_DO_FUN( do_mptoss );
DECLARE_DO_FUN( do_mppersonalize );
DECLARE_DO_FUN( do_mppurge );
DECLARE_DO_FUN( do_mpquest );
DECLARE_DO_FUN( do_mpqkamount );
DECLARE_DO_FUN( do_mpstat );
DECLARE_DO_FUN( do_opstat );
DECLARE_DO_FUN( do_rpstat );
DECLARE_DO_FUN( do_mptransfer );
DECLARE_DO_FUN( do_mpmorph );
DECLARE_DO_FUN( do_mpunmorph );
DECLARE_DO_FUN( do_mpnuisance );
DECLARE_DO_FUN( do_mpunnuisance );
DECLARE_DO_FUN( do_mpbodybag );
DECLARE_DO_FUN( do_mpclearbodies );
DECLARE_DO_FUN( do_mpapply );
DECLARE_DO_FUN( do_mpapplyb );
DECLARE_DO_FUN( do_mppkset );
DECLARE_DO_FUN( do_mpscatter );
DECLARE_DO_FUN( do_mpfavor );
DECLARE_DO_FUN( do_mpdelay );
DECLARE_DO_FUN( do_mpsound );
DECLARE_DO_FUN( do_mpsoundaround );
DECLARE_DO_FUN( do_mpsoundat );
DECLARE_DO_FUN( do_mpmusic );
DECLARE_DO_FUN( do_mpmusicaround );
DECLARE_DO_FUN( do_mpmusicat );

/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN( spell_null );
DECLARE_SPELL_FUN( spell_notfound );
DECLARE_SPELL_FUN( spell_acidic_touch );               // Taon
DECLARE_SPELL_FUN( spell_acid_blast );
DECLARE_SPELL_FUN( spell_animate_corpse );
DECLARE_SPELL_FUN( spell_animate_skeleton );
DECLARE_SPELL_FUN( spell_astral_walk );
DECLARE_SPELL_FUN( spell_greater_smite );
DECLARE_SPELL_FUN( spell_aura_of_life );
DECLARE_SPELL_FUN( spell_flaming_shield );
DECLARE_SPELL_FUN( spell_barbs );
DECLARE_SPELL_FUN( spell_iceshard );
DECLARE_SPELL_FUN( spell_battlefield_view );
DECLARE_SPELL_FUN( spell_bestow_vitae );
DECLARE_SPELL_FUN( spell_blindness );
DECLARE_SPELL_FUN( spell_burning_hands );
DECLARE_SPELL_FUN( spell_call_lightning );
DECLARE_SPELL_FUN( spell_cause_critical );
DECLARE_SPELL_FUN( spell_cause_light );
DECLARE_SPELL_FUN( spell_cause_serious );
DECLARE_SPELL_FUN( spell_chain_lightning );
DECLARE_SPELL_FUN( spell_change_sex );
DECLARE_SPELL_FUN( spell_charm_person );
DECLARE_SPELL_FUN( spell_colour_spray );
DECLARE_SPELL_FUN( spell_control_weather );
DECLARE_SPELL_FUN( spell_create_water );
DECLARE_SPELL_FUN( spell_cure_blindness );
DECLARE_SPELL_FUN( spell_cure_affliction );
DECLARE_SPELL_FUN( spell_cure_poison );
DECLARE_SPELL_FUN( spell_cure_light );
DECLARE_SPELL_FUN( spell_arch_healing );               // Taon
DECLARE_SPELL_FUN( spell_cure_serious );
DECLARE_SPELL_FUN( spell_greater_heal );
DECLARE_SPELL_FUN( spell_cure_critical );
DECLARE_SPELL_FUN( spell_bless_water );
DECLARE_SPELL_FUN( spell_clavus );
DECLARE_SPELL_FUN( spell_curse );
DECLARE_SPELL_FUN( spell_detect_poison );
DECLARE_SPELL_FUN( spell_dispel_evil );
DECLARE_SPELL_FUN( spell_dispel_magic );
DECLARE_SPELL_FUN( spell_divine_intervention );        // Taon
DECLARE_SPELL_FUN( spell_divine_light );               // Taon
DECLARE_SPELL_FUN( spell_draw_mana );
DECLARE_SPELL_FUN( spell_dream );
DECLARE_SPELL_FUN( spell_earthquake );
DECLARE_SPELL_FUN( spell_death_field );
DECLARE_SPELL_FUN( spell_enchant_armor );
DECLARE_SPELL_FUN( spell_enchant_weapon );
DECLARE_SPELL_FUN( spell_energy_drain );
DECLARE_SPELL_FUN( spell_faerie_fire );
DECLARE_SPELL_FUN( spell_faerie_fog );
DECLARE_SPELL_FUN( spell_farsight );
DECLARE_SPELL_FUN( spell_fascinate );
DECLARE_SPELL_FUN( spell_fireball );
DECLARE_SPELL_FUN( spell_firestorm );
DECLARE_SPELL_FUN( spell_flamestrike );
DECLARE_SPELL_FUN( spell_gate );
DECLARE_SPELL_FUN( spell_knock );
DECLARE_SPELL_FUN( spell_harm );
DECLARE_SPELL_FUN( spell_heal );
DECLARE_SPELL_FUN( spell_healing_essence );
DECLARE_SPELL_FUN( spell_identify );
DECLARE_SPELL_FUN( spell_invis );
DECLARE_SPELL_FUN( spell_know_alignment );
DECLARE_SPELL_FUN( spell_lend_health );
DECLARE_SPELL_FUN( spell_locate_object );
DECLARE_SPELL_FUN( spell_magic_missile );
DECLARE_SPELL_FUN( spell_mana_pool );
DECLARE_SPELL_FUN( spell_mist_walk );
DECLARE_SPELL_FUN( spell_nettle_skin );
DECLARE_SPELL_FUN( spell_paralyze );
DECLARE_SPELL_FUN( spell_pass_door );
DECLARE_SPELL_FUN( spell_plant_pass );
DECLARE_SPELL_FUN( spell_poison );
DECLARE_SPELL_FUN( spell_polymorph );
DECLARE_SPELL_FUN( spell_recharge );
DECLARE_SPELL_FUN( spell_refresh );
DECLARE_SPELL_FUN( spell_remove_curse );
DECLARE_SPELL_FUN( spell_remove_invis );
DECLARE_SPELL_FUN( spell_remove_silence );
DECLARE_SPELL_FUN( spell_remove_trap );
DECLARE_SPELL_FUN( spell_shadowform );
DECLARE_SPELL_FUN( spell_shocking_grasp );
DECLARE_SPELL_FUN( spell_silence );
DECLARE_SPELL_FUN( spell_sleep );
DECLARE_SPELL_FUN( spell_smaug );
DECLARE_SPELL_FUN( spell_solar_flight );
DECLARE_SPELL_FUN( spell_summon );
DECLARE_SPELL_FUN( spell_create_food );
DECLARE_SPELL_FUN( spell_summon_light );
DECLARE_SPELL_FUN( spell_teleport );
DECLARE_SPELL_FUN( spell_ventriloquate );
DECLARE_SPELL_FUN( spell_weaken );
DECLARE_SPELL_FUN( spell_wizard_sight );
DECLARE_SPELL_FUN( spell_wizard_eye );
DECLARE_SPELL_FUN( spell_word_of_recall );
DECLARE_SPELL_FUN( spell_acid_breath );
DECLARE_SPELL_FUN( spell_fire_breath );
DECLARE_SPELL_FUN( spell_frost_breath );
DECLARE_SPELL_FUN( spell_gas_breath );
DECLARE_SPELL_FUN( spell_lightning_breath );
DECLARE_SPELL_FUN( spell_spiral_blast );
DECLARE_SPELL_FUN( spell_scorching_surge );
DECLARE_SPELL_FUN( spell_helical_flow );
DECLARE_SPELL_FUN( spell_transport );
DECLARE_SPELL_FUN( spell_portal );
DECLARE_SPELL_FUN( spell_slow );                       // Taon
DECLARE_SPELL_FUN( spell_giant_strength );             // Taon
DECLARE_SPELL_FUN( spell_shadow_bolt );                // Taon
DECLARE_SPELL_FUN( spell_ethereal_fist );
DECLARE_SPELL_FUN( spell_spectral_furor );
DECLARE_SPELL_FUN( spell_hand_of_chaos );
DECLARE_SPELL_FUN( spell_disruption );
DECLARE_SPELL_FUN( spell_sonic_resonance );
DECLARE_SPELL_FUN( spell_mind_wrack );
DECLARE_SPELL_FUN( spell_mind_wrench );
DECLARE_SPELL_FUN( spell_revive );
DECLARE_SPELL_FUN( spell_sulfurous_spray );
DECLARE_SPELL_FUN( spell_caustic_fount );
DECLARE_SPELL_FUN( spell_acetum_primus );
DECLARE_SPELL_FUN( spell_galvanic_whip );
DECLARE_SPELL_FUN( spell_magnetic_thrust );
DECLARE_SPELL_FUN( spell_quantum_spike );
DECLARE_SPELL_FUN( spell_black_hand );
DECLARE_SPELL_FUN( spell_black_fist );
DECLARE_SPELL_FUN( spell_black_lightning );
DECLARE_SPELL_FUN( spell_midas_touch );
DECLARE_SPELL_FUN( spell_bethsaidean_touch );
DECLARE_SPELL_FUN( spell_expurgation );
DECLARE_SPELL_FUN( spell_sacral_divinity );

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if defined(_AIX)
char                   *crypt( const char *key, const char *salt );
#endif

#if defined(apollo)
int                     atoi( const char *string );
void                   *calloc( unsigned nelem, size_t size );
char                   *crypt( const char *key, const char *salt );
#endif

#if defined(hpux)
char                   *crypt( const char *key, const char *salt );
#endif
/*
#if defined(linux)
   char *crypt(const char *key, const char *salt);
#endif
*/
#if defined(MIPS_OS)
char                   *crypt( const char *key, const char *salt );
#endif

#if defined(NeXT)
char                   *crypt( const char *key, const char *salt );
#endif

#if defined(sequent)
char                   *crypt( const char *key, const char *salt );
int                     fclose( FILE * stream );
int                     fprintf( FILE * stream, const char *format, ... );
int                     fread( void *ptr, int size, int n, FILE * stream );
int                     fseek( FILE * stream, long offset, int ptrname );
void                    perror( const char *s );
int                     ungetc( int c, FILE * stream );
#endif

#if defined(sun)
char                   *crypt( const char *key, const char *salt );
int                     fclose( FILE * stream );
int                     fprintf( FILE * stream, const char *format, ... );

#if defined(SYSV)
size_t                  fread( void *ptr, size_t size, size_t n, FILE * stream );
#else
int                     fread( void *ptr, int size, int n, FILE * stream );
#endif
int                     fseek( FILE * stream, long offset, int ptrname );
void                    perror( const char *s );
int                     ungetc( int c, FILE * stream );
#endif

#if defined(ultrix)
char                   *crypt( const char *key, const char *salt );
#endif

/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 * United States to foreign countries.
 *
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if defined(NOCRYPT)
#define crypt(s1, s2) (s1)
#endif

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD  CHAR_DATA
#define MID MOB_INDEX_DATA
#define OD  OBJ_DATA
#define OID OBJ_INDEX_DATA
#define RID ROOM_INDEX_DATA
#define SF  SPEC_FUN
#define BD  BOARD_DATA
#define EDD EXTRA_DESCR_DATA
#define RD  RESET_DATA
#define ED  EXIT_DATA
#define ST  SOCIALTYPE
#define SK  SKILLTYPE
#define DE	DEITY_DATA
#define BT	BANK_DATA

/* channels.c */
void                    announce( const char *argument );

/* calendar.c */
void                    calc_season( void );
char                   *mini_c_time( time_t curtime, int tz );
char                   *c_time( time_t curtime, int tz );
bool                    load_timedata( void );
void                    load_holidays( void );
void                    save_timedata( void );
void                    update_timers( void );
void                    update_calendar( void );

/* act_comm.c */
bool                    circle_follow( CHAR_DATA *ch, CHAR_DATA *victim );
void                    add_follower( CHAR_DATA *ch, CHAR_DATA *master );
void                    stop_follower( CHAR_DATA *ch );
void                    die_follower( CHAR_DATA *ch );
bool                    is_same_group( CHAR_DATA *ach, CHAR_DATA *bch );
void                    send_rip_screen( CHAR_DATA *ch );
void                    send_rip_title( CHAR_DATA *ch );
void                    send_ansi_title( CHAR_DATA *ch );
void                    send_ascii_title( CHAR_DATA *ch );
void                    talk_auction( char *argument );
int                     knows_language( CHAR_DATA *ch, int language, CHAR_DATA *cch );
char                   *translate( int percent, const char *in, const char *name );
int                     tzone_lookup( const char *arg );
char                   *str_time( time_t timet, int tz, const char *format );

/* deity.c */
DE                     *get_deity( char *name );
void                    load_deity( void );
void                    save_deity( DEITY_DATA * deity );
void adjust_favor       args( ( CHAR_DATA *ch, int field, int mod ) );

ROOM_INDEX_DATA        *location_lookup( CHAR_DATA *ch, char *argument );
LOCATION               *new_loc( void );
void                    free_loc( LOCATION * loc );
void                    save_locations( void );
void                    load_locations( void );

extern LOCATION        *location_list;

/* act_info.c */
int                     get_door( char *arg );
char                   *num_punct( int foo );
char                   *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort );
void                    show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort,
                                           bool fShowNothing );
bool                    is_ignoring( CHAR_DATA *ch, CHAR_DATA *ign_ch );
void                    show_race_line( CHAR_DATA *ch, CHAR_DATA *victim );
HELP_DATA              *get_help args( ( CHAR_DATA *ch, char *argument ) );
extern char             message[MSL];

/* act_move.c */
ED                     *find_door( CHAR_DATA *ch, char *arg, bool quiet );
ED                     *get_exit( ROOM_INDEX_DATA *room, short dir );
ED                     *get_exit_to( ROOM_INDEX_DATA *room, short dir, int vnum );
ED                     *get_exit_num( ROOM_INDEX_DATA *room, short count );
ch_ret                  move_char( CHAR_DATA *ch, EXIT_DATA *pexit, int fall );
void                    teleport( CHAR_DATA *ch, int room, EXT_BV * flags );
short                   encumbrance( CHAR_DATA *ch, short move );
bool                    will_fall( CHAR_DATA *ch, int fall );
ch_ret                  pullcheck( CHAR_DATA *ch, int pulse );
const char             *rev_exit( short vdir );

/* Volk added swim stuff here - in act_move, used in update.c */
void                    swim_check( CHAR_DATA *ch, int time );
void                    water_sink( CHAR_DATA *ch, int time );
short                   max_holdbreath( CHAR_DATA *ch );
void                    breath_msg( CHAR_DATA *ch, int percentage );
void                    fbite_msg( CHAR_DATA *ch, int percentage );
void                    humanform_change( CHAR_DATA *ch, bool tohuman );    /* tohuman=TRUE
                                                                             * if going to
                                                                             * human, =FALSE
                                                                             * if going to
                                                                             * dragon */

/* variables.c */
void                    delete_variable( VARIABLE_DATA * vd );
VARIABLE_DATA          *get_tag( CHAR_DATA *ch, char *tag, int vnum );

/* act_obj.c */
obj_ret                 damage_obj( OBJ_DATA *obj );
short                   get_obj_resistance( OBJ_DATA *obj );
void                    obj_fall( OBJ_DATA *obj, bool through );
OD                     *create_money( int amount, int type );
void                    make_scraps( OBJ_DATA *obj );
void                    update_weapon( CHAR_DATA *ch, OBJ_DATA *obj );
void                    identify_object( CHAR_DATA *ch, OBJ_DATA *obj );

/* act_wiz.c */
bool create_new_race    args( ( int race, char *argument ) );
bool create_new_class   args( ( int Class, char *argument ) );
RID                    *find_location args( ( CHAR_DATA *ch, char *arg ) );
void                    echo_to_all( short AT_COLOR, const char *argument, short tar );
void                    echo_to_room( short AT_COLOR, ROOM_INDEX_DATA *room, const char *argument );
void get_reboot_string  args( ( void ) );
struct tm              *update_time args( ( struct tm * old_time ) );
void free_social        args( ( SOCIALTYPE * social ) );
void add_social         args( ( SOCIALTYPE * social ) );
void free_command       args( ( CMDTYPE * command ) );
void unlink_command     args( ( CMDTYPE * command ) );
void add_command        args( ( CMDTYPE * command ) );
void add_bank           args( ( BANK_DATA * bank ) );
void free_bank          args( ( BANK_DATA * bank ) );

/*	bank.c */
char get_bank_password  args( ( BANK_DATA * bank ) );
int get_bank_amount     args( ( BANK_DATA * bank ) );

/* build.c */
int get_cmdflag         args( ( char *flag ) );
char                   *flag_string( int bitvector, const char *const flagarray[] );
char                   *ext_flag_string( EXT_BV * bitvector, const char *const flagarray[] );
int get_mpflag          args( ( char *flag ) );
int get_dir             args( ( char *txt ) );
char                   *strip_cr args( ( char *str ) );
int                     get_cmdtype( char *flag );

/* comm.c */
void save_auth_list     args( ( void ) );
void close_socket       args( ( DESCRIPTOR_DATA *dclose, bool force ) );
void write_to_buffer    args( ( DESCRIPTOR_DATA *d, const char *txt, unsigned int length ) );
void                    write_to_pager( DESCRIPTOR_DATA *d, const char *txt, unsigned int length );
void                    send_to_char( const char *txt, CHAR_DATA *ch );
void send_to_char_color args( ( const char *txt, CHAR_DATA *ch ) );

void                    send_to_desc_color( const char *txt, DESCRIPTOR_DATA *d );
void                    send_to_pager( const char *txt, CHAR_DATA *ch );
void send_to_pager_color args( ( const char *txt, CHAR_DATA *ch ) );
void set_char_color     args( ( short AType, CHAR_DATA *ch ) );
void set_pager_color    args( ( short AType, CHAR_DATA *ch ) );

void                    ch_printf( CHAR_DATA *ch, const char *fmt, ... );
void                    ch_printf_color( CHAR_DATA *ch, const char *fmt, ... );

void                    pager_printf( CHAR_DATA *ch, const char *fmt, ... );
void                    pager_printf_color( CHAR_DATA *ch, const char *fmt, ... );

void                    act( short AType, const char *format, CHAR_DATA *ch, const void *arg1,
                             const void *arg2, int type );
void                    act_printf( short AType, CHAR_DATA *ch, void *arg1, void *arg2, int type,
                                    const char *str, ... );

char                   *myobj args( ( OBJ_DATA *obj ) );
char                   *obj_short args( ( OBJ_DATA *obj ) );

/* reset.c */
RD                     *make_reset args( ( char letter, int extra, int arg1, int arg2, int arg3 ) );
RD                     *add_reset( ROOM_INDEX_DATA *room, char letter, int extra, int arg1,
                                   int arg2, int arg3 );
void reset_area         args( ( AREA_DATA *pArea ) );

/* db.c */
void                    add_letter( char *string, char letter );
const char             *format( const char *fmt, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void                    show_file( CHAR_DATA *ch, const char *filename );
char                   *str_dup args( ( char const *str ) );
void boot_db            args( ( bool fCopyOver ) );
void area_update        args( ( void ) );
void add_char           args( ( CHAR_DATA *ch ) );
CD                     *create_mobile args( ( MOB_INDEX_DATA *pMobIndex ) );
OD                     *create_object args( ( OBJ_INDEX_DATA *pObjIndex, int level ) );
void clear_char         args( ( CHAR_DATA *ch ) );
void free_char          args( ( CHAR_DATA *ch ) );
char                   *get_extra_descr args( ( const char *name, EXTRA_DESCR_DATA *ed ) );
MID                    *get_mob_index args( ( int vnum ) );
OID                    *get_obj_index args( ( int vnum ) );
RID                    *new_get_room_index args( ( int vnum, const char *filename, int line ) );

#define get_room_index( vnum ) new_get_room_index( (vnum), __FILE__, __LINE__ )

char fread_letter       args( ( FILE * fp ) );
int fread_number        args( ( FILE * fp ) );
time_t fread_time       args( ( FILE * fp ) );
EXT_BV fread_bitvector  args( ( FILE * fp ) );
void fwrite_bitvector   args( ( EXT_BV * bits, FILE * fp ) );
char                   *print_bitvector args( ( EXT_BV * bits ) );
char                   *fread_string args( ( FILE * fp ) );
char                   *fread_flagstring args( ( FILE * fp ) );
char                   *fread_null_string( FILE * fp );
char                   *fread_string_nohash args( ( FILE * fp ) );
void fread_to_eol       args( ( FILE * fp ) );
char                   *fread_word args( ( FILE * fp ) );
char                   *fread_line args( ( FILE * fp ) );
int number_fuzzy        args( ( int number ) );
int number_range        args( ( int from, int to ) );
short number_chance     args( ( short low, short high ) );  // Taon
int number_percent      args( ( void ) );
int number_door         args( ( void ) );
int number_bits         args( ( int width ) );
int number_mm           args( ( void ) );
int dice                args( ( int number, int size ) );
int interpolate         args( ( int level, int value_00, int value_32 ) );
void smash_tilde        args( ( char *str ) );
void hide_tilde         args( ( char *str ) );
char                   *show_tilde args( ( char *str ) );
bool                    new_str_cmp( const char *astr, const char *bstr, const char *filename,
                                     int line );

#define str_cmp( point, check ) new_str_cmp( ( point ), ( check ), __FILE__, __LINE__ )
bool str_prefix         args( ( const char *astr, const char *bstr ) );
bool str_infix          args( ( const char *astr, const char *bstr ) );
bool str_suffix         args( ( const char *astr, const char *bstr ) );
char                   *capitalize args( ( const char *str ) );
char                   *lowercase args( ( const char *str ) );
char                   *strlower args( ( const char *str ) );
char                   *strupper args( ( const char *str ) );
char                   *aoran args( ( const char *str ) );
void append_file        args( ( CHAR_DATA *ch, const char *file, const char *str ) );
void append_to_file     args( ( const char *file, const char *str ) );
void                    append_to_file_printf( const char *file, const char *fmt, ... );
void log_string_plus    args( ( const char *str, short log_type, short level ) );
RID                    *make_room( int vnum, AREA_DATA *area );
OID                    *make_object args( ( int vnum, int cvnum, char *name ) );
MID                    *make_mobile args( ( int vnum, int cvnum, char *name ) );
ED                     *make_exit
args( ( ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *to_room, short door ) );
void add_help           args( ( HELP_DATA *pHelp ) );
void fix_area_exits     args( ( AREA_DATA *tarea ) );
void                    load_area_file( AREA_DATA *tarea, const char *filename );
void randomize_exits    args( ( ROOM_INDEX_DATA *room, short maxdir ) );
void make_stafflist     args( ( void ) );
void tail_chain         args( ( void ) );
void delete_room        args( ( ROOM_INDEX_DATA *room ) );
void delete_obj         args( ( OBJ_INDEX_DATA *obj ) );
void delete_mob         args( ( MOB_INDEX_DATA *mob ) );

#if defined(WIN32)
void                    bug( const char *str, ... );
#else
void                    bug( const char *str, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
#endif

/* Functions to add to sorting lists. -- Altrag */
/*void mob_sort  args((MOB_INDEX_DATA *pMob));
void obj_sort  args((OBJ_INDEX_DATA *pObj));
void room_sort  args((ROOM_INDEX_DATA *pRoom));*/
void sort_area          args( ( AREA_DATA *pArea, bool proto ) );
void sort_area_by_name  args( ( AREA_DATA *pArea ) );  /* Fireblade */
void write_projects     args( ( void ) );

#if defined(WIN32)
void                    log_printf( const char *fmt, ... );
#else
void                    log_printf( const char *fmt, ... )
    __attribute__ ( ( format( printf, 1, 2 ) ) );
#endif
size_t                  mudstrlcat( char *dst, const char *src, size_t siz );

// size_t mudstrlcpy( char *dst, const char *src, size_t siz );
#define mudstrlcpy(d, s, z) dbmudstrlcpy((d), (s), (z), __FILE__, __LINE__)
size_t                  dbmudstrlcpy( char *, const char *, size_t, const char *, int );

/* build.c */
void                    RelCreate( relation_type, void *, void * );
void                    RelDestroy( relation_type, void *, void * );
bool can_rmodify        args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room ) );
bool can_omodify        args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool can_mmodify        args( ( CHAR_DATA *ch, CHAR_DATA *mob ) );
bool can_medit          args( ( CHAR_DATA *ch, MOB_INDEX_DATA *mob ) );
void free_reset         args( ( AREA_DATA *are, RESET_DATA *res ) );
void free_area          args( ( AREA_DATA *are ) );
void assign_area        args( ( CHAR_DATA *ch, bool quiet ) );
EDD                    *SetRExtra args( ( ROOM_INDEX_DATA *room, char *keywords ) );
bool DelRExtra          args( ( ROOM_INDEX_DATA *room, char *keywords ) );
EDD                    *SetOExtra args( ( OBJ_DATA *obj, char *keywords ) );
bool DelOExtra          args( ( OBJ_DATA *obj, char *keywords ) );
EDD                    *SetOExtraProto args( ( OBJ_INDEX_DATA *obj, char *keywords ) );
bool DelOExtraProto     args( ( OBJ_INDEX_DATA *obj, char *keywords ) );
void fold_area          args( ( AREA_DATA *tarea, char *filename, bool install ) );
int get_otype           args( ( char *type ) );
int get_atype           args( ( char *type ) );
int get_aflag           args( ( char *flag ) );
int get_a2flag          args( ( char *flag ) );
int get_oflag           args( ( char *flag ) );
int get_wflag           args( ( char *flag ) );
void init_area_weather  args( ( void ) );
void save_weatherdata   args( ( void ) );

/* editor.c */
#define start_editing(ch, data) start_editing_nolimit(ch, data, MSL)
void                    start_editing_nolimit( CHAR_DATA *ch, char *data, short max_size );
void                    stop_editing( CHAR_DATA *ch );
void                    edit_buffer( CHAR_DATA *ch, char *argument );
char                   *copy_buffer( CHAR_DATA *ch );
void                    set_editor_desc( CHAR_DATA *ch, const char *desc );
void                    editor_desc_printf( CHAR_DATA *ch, const char *desc_fmt, ... );
char                   *copy_buffer_nohash( CHAR_DATA *ch );

/* fight.c */
int max_fight           args( ( CHAR_DATA *ch ) );
void violence_update    args( ( void ) );
ch_ret multi_hit        args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
ch_ret projectile_hit  
args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, OBJ_DATA *projectile, short dist ) );
short ris_damage        args( ( CHAR_DATA *ch, short dam, int ris ) );
ch_ret damage           args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt ) );
void update_pos         args( ( CHAR_DATA *victim ) );
void set_fighting       args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void stop_fighting      args( ( CHAR_DATA *ch, bool fBoth ) );
void free_fight         args( ( CHAR_DATA *ch ) );
CD                     *who_fighting args( ( CHAR_DATA *ch ) );
void check_killer       args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void check_attacker     args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void death_cry          args( ( CHAR_DATA *ch ) );
void stop_hunting       args( ( CHAR_DATA *ch ) );
void stop_hating        args( ( CHAR_DATA *ch ) );
void stop_fearing       args( ( CHAR_DATA *ch ) );
void start_hunting      args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void start_hating       args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void start_fearing      args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool is_hunting         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool is_hating          args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool is_fearing         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool is_safe            args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool SHOW ) );
bool legal_loot         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
short VAMP_AC           args( ( CHAR_DATA *ch ) );
short DRAGON_AC         args( ( CHAR_DATA *ch ) );
short MONK_AC           args( ( CHAR_DATA *ch ) );
bool check_illegal_pk   args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void raw_kill           args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool in_arena           args( ( CHAR_DATA *ch ) );
bool can_astral         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

/* misc.c */
void actiondesc         args( ( CHAR_DATA *ch, OBJ_DATA *obj, void *vo ) );
bool fptof              args( ( FILE * stream, const char *data ) );
EXT_BV meb              args( ( int bit ) );
EXT_BV multimeb         args( ( int bit, ... ) );
OD                     *make_trap args( ( int v0, int v1, int v2, int v3 ) );

/* mud_comm.c */
const char             *mprog_type_to_name args( ( int type ) );

/* mud_prog.c */
#ifdef DUNNO_STRSTR
char                   *strstr args( ( const char *s1, const char *s2 ) );
#endif

void mprog_wordlist_check
args( ( char *arg, CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *object, void *vo, int type ) );
void mprog_percent_check
args( ( CHAR_DATA *mob, CHAR_DATA *actor, OBJ_DATA *object, void *vo, int type ) );
void mprog_act_trigger 
args( ( char *buf, CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj, void *vo ) );
void mprog_bribe_trigger args( ( CHAR_DATA *mob, CHAR_DATA *ch, int amount ) );
void mprog_entry_trigger args( ( CHAR_DATA *mob ) );
bool rprog_pre_enter_trigger args( ( CHAR_DATA *mob, ROOM_INDEX_DATA *room ) );
void mprog_give_trigger args( ( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj ) );
void mprog_greet_trigger args( ( CHAR_DATA *mob ) );
void mprog_fight_trigger args( ( CHAR_DATA *mob, CHAR_DATA *ch ) );
void mprog_hitprcnt_trigger args( ( CHAR_DATA *mob, CHAR_DATA *ch ) );
void mprog_death_trigger args( ( CHAR_DATA *killer, CHAR_DATA *mob ) );
void mprog_random_trigger args( ( CHAR_DATA *mob ) );
void mprog_speech_trigger args( ( char *txt, CHAR_DATA *mob ) );
void mprog_script_trigger args( ( CHAR_DATA *mob ) );
void mprog_hour_trigger args( ( CHAR_DATA *mob ) );
void mprog_time_trigger args( ( CHAR_DATA *mob ) );
void rset_supermob      args( ( ROOM_INDEX_DATA *room ) );
void release_supermob   args( (  ) );
void mppause_update     args( (  ) );
char                   *strip_tilde args( ( char *str ) );

void                    progbug( CHAR_DATA *mob, const char *str, ... );

/* player.c */
void set_title          args( ( CHAR_DATA *ch, char *title ) );
int GetMaxBlood         args( ( CHAR_DATA *ch ) );

/* skills.c */
bool can_use_skill      args( ( CHAR_DATA *ch, int percent, int gsn ) );
bool check_skill        args( ( CHAR_DATA *ch, char *command, char *argument ) );
void learn_from_success args( ( CHAR_DATA *ch, int sn ) );
void learn_from_failure args( ( CHAR_DATA *ch, int sn ) );
bool check_phase        args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool check_displacement args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool check_parry        args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool check_dodge        args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool check_tumble       args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool check_grip         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void disarm             args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void trip               args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool mob_fire           args( ( CHAR_DATA *ch, char *name ) );
CD                     *scan_for_victim args( ( CHAR_DATA *ch, EXIT_DATA *pexit, char *name ) );

/* multi.c
 * Volk: Support for multiclass */
int get_maxadept        args( ( CHAR_DATA *ch, int sn, bool availabletochar ) );
int get_maxskill        args( ( CHAR_DATA *ch, int sn, bool availabletochar ) );
bool CAN_LEARN          args( ( CHAR_DATA *ch, int sn, bool canlearnnow ) );
bool can_teach          args( ( CHAR_DATA *ch, CHAR_DATA *teacher, int sn ) );

/* ban.c */
int add_ban             args( ( CHAR_DATA *ch, char *arg1, char *arg2, int time, int type ) );
void show_bans          args( ( CHAR_DATA *ch, int type ) );
void save_banlist       args( ( void ) );
void load_banlist       args( ( void ) );
bool check_total_bans   args( ( DESCRIPTOR_DATA *d ) );
bool check_bans         args( ( CHAR_DATA *ch, int type ) );

/* imm_host.c */
bool check_immortal_domain args( ( CHAR_DATA *ch, char *host ) );
int load_imm_host       args( ( void ) );
int fread_imm_host      args( ( FILE * fp, IMMORTAL_HOST * data ) );
void do_write_imm_host  args( ( void ) );

/* handler.c */
CHAR_DATA              *carried_by( OBJ_DATA *obj );
AREA_DATA              *get_area_obj args( ( OBJ_INDEX_DATA *obj ) );
int get_exp_worth       args( ( CHAR_DATA *ch ) );
int exp_level           args( ( CHAR_DATA *ch, short level ) );
int craft_level         args( ( CHAR_DATA *ch, short level ) );
int exp_class_level     args( ( CHAR_DATA *ch, short level, short class_info ) );
int exp_craft_level     args( ( CHAR_DATA *ch, short level, short class_info ) );
short                   calculate_age( CHAR_DATA *ch );
void set_position       args( ( CHAR_DATA *ch, int position ) );
void sound_to_char     
args( ( const char *fname, int vol, int repeats, int priority, const char *type, const char *url,
        CHAR_DATA *ch ) );
void music_to_char     
args( ( const char *fname, int vol, int repeats, bool continu, const char *type, CHAR_DATA *ch ) );
void reset_sound        args( ( CHAR_DATA *ch ) );
void reset_music        args( ( CHAR_DATA *ch ) );

int strlen_color        args( ( char *argument ) );
short get_trust         args( ( CHAR_DATA *ch ) );
short get_curr_str      args( ( CHAR_DATA *ch ) );
short get_curr_int      args( ( CHAR_DATA *ch ) );
short get_curr_wis      args( ( CHAR_DATA *ch ) );
short get_curr_dex      args( ( CHAR_DATA *ch ) );
short get_curr_con      args( ( CHAR_DATA *ch ) );
short get_curr_cha      args( ( CHAR_DATA *ch ) );
short get_curr_lck      args( ( CHAR_DATA *ch ) );
bool can_take_proto     args( ( CHAR_DATA *ch ) );
int can_carry_n         args( ( CHAR_DATA *ch ) );
int can_carry_w         args( ( CHAR_DATA *ch ) );
bool                    is_name( const char *str, char *namelist );
bool                    is_name_prefix( const char *str, char *namelist );
bool                    nifty_is_name( char *str, char *namelist );
bool                    nifty_is_name_prefix( char *str, char *namelist );
void affect_modify      args( ( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd ) );
char                   *affect_bit_name args( ( EXT_BV * vector ) );
void affect_to_char     args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void affect_remove      args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void affect_strip       args( ( CHAR_DATA *ch, int sn ) );
bool is_affected        args( ( CHAR_DATA *ch, int sn ) );
void affect_join        args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void char_from_room     args( ( CHAR_DATA *ch ) );
void char_to_room       args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
OD                     *obj_to_char args( ( OBJ_DATA *obj, CHAR_DATA *ch ) );
void obj_from_char      args( ( OBJ_DATA *obj ) );
int apply_ac            args( ( OBJ_DATA *obj, int iWear ) );
OD                     *get_eq_char args( ( CHAR_DATA *ch, int iWear ) );
void equip_char         args( ( CHAR_DATA *ch, OBJ_DATA *obj, int iWear ) );
void unequip_char       args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
int count_obj_list      args( ( OBJ_INDEX_DATA *obj, OBJ_DATA *list ) );
void obj_from_room      args( ( OBJ_DATA *obj ) );
OD                     *obj_to_room args( ( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex ) );
OD                     *obj_to_obj args( ( OBJ_DATA *obj, OBJ_DATA *obj_to ) );
void obj_from_obj       args( ( OBJ_DATA *obj ) );
void extract_obj        args( ( OBJ_DATA *obj ) );
void extract_exit       args( ( ROOM_INDEX_DATA *room, EXIT_DATA *pexit ) );
void extract_room       args( ( ROOM_INDEX_DATA *room ) );
void clean_room         args( ( ROOM_INDEX_DATA *room ) );
void clean_obj          args( ( OBJ_INDEX_DATA *obj ) );
void clean_mob          args( ( MOB_INDEX_DATA *mob ) );
void                    clean_resets( ROOM_INDEX_DATA *room );
void extract_char       args( ( CHAR_DATA *ch, bool fPull ) );
CD                     *get_char_room args( ( CHAR_DATA *ch, char *argument ) );
CD                     *get_char_area args( ( CHAR_DATA *ch, char *argument ) );
CD                     *get_char_world args( ( CHAR_DATA *ch, char *argument ) );
CD                     *get_char_area args( ( CHAR_DATA *ch, char *argument ) );
OD                     *get_obj_type args( ( OBJ_INDEX_DATA *pObjIndexData ) );
OD                     *get_obj_list args( ( CHAR_DATA *ch, char *argument, OBJ_DATA *list ) );
OD                     *get_obj_list_rev args( ( CHAR_DATA *ch, char *argument, OBJ_DATA *list ) );
OD                     *get_obj_carry args( ( CHAR_DATA *ch, char *argument ) );
OD                     *get_obj_wear args( ( CHAR_DATA *ch, char *argument ) );
OD                     *get_obj_vnum args( ( CHAR_DATA *ch, int vnum ) );
OD                     *get_obj_here args( ( CHAR_DATA *ch, char *argument ) );
OD                     *get_obj_world args( ( CHAR_DATA *ch, char *argument ) );
int get_obj_number      args( ( OBJ_DATA *obj ) );
int get_obj_weight      args( ( OBJ_DATA *obj, bool real ) );
int                     get_real_obj_weight( OBJ_DATA *obj );
bool room_is_dark       args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool room_is_private    args( ( ROOM_INDEX_DATA *pRoomIndex ) );
CD                     *room_is_dnd args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
bool can_see            args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool can_see_obj        args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool can_drop_obj       args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
const char             *item_type_name args( ( OBJ_DATA *obj ) );
const char             *pull_type_name args( ( int pulltype ) );
ch_ret check_for_trap   args( ( CHAR_DATA *ch, OBJ_DATA *obj, int flag ) );
ch_ret check_room_for_traps args( ( CHAR_DATA *ch, int flag ) );
bool is_trapped         args( ( OBJ_DATA *obj ) );
OD                     *get_trap args( ( OBJ_DATA *obj ) );
ch_ret spring_trap      args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
void name_stamp_stats   args( ( CHAR_DATA *ch ) );
void fix_char           args( ( CHAR_DATA *ch ) );
void showaffect         args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void set_cur_obj        args( ( OBJ_DATA *obj ) );
bool obj_extracted      args( ( OBJ_DATA *obj ) );
void queue_extracted_obj args( ( OBJ_DATA *obj ) );
void clean_obj_queue    args( ( void ) );
void set_cur_char       args( ( CHAR_DATA *ch ) );
bool char_died          args( ( CHAR_DATA *ch ) );
void queue_extracted_char args( ( CHAR_DATA *ch, bool extract ) );
void clean_char_queue   args( ( void ) );
void                    add_timer( CHAR_DATA *ch, short type, int count, DO_FUN *fun, int value );
TIMER                  *get_timerptr args( ( CHAR_DATA *ch, short type ) );
short get_timer         args( ( CHAR_DATA *ch, short type ) );
void extract_timer      args( ( CHAR_DATA *ch, TIMER * timer ) );
void remove_timer       args( ( CHAR_DATA *ch, short type ) );
bool in_soft_range      args( ( CHAR_DATA *ch, AREA_DATA *tarea ) );
bool in_hard_range      args( ( CHAR_DATA *ch, AREA_DATA *tarea ) );
bool chance             args( ( CHAR_DATA *ch, short percent ) );
bool chance_attrib      args( ( CHAR_DATA *ch, short percent, short attrib ) );
OD                     *clone_object args( ( OBJ_DATA *obj ) );
void split_obj          args( ( OBJ_DATA *obj, int num ) );
void separate_obj       args( ( OBJ_DATA *obj ) );
bool empty_obj         
args( ( OBJ_DATA *obj, OBJ_DATA *destobj, ROOM_INDEX_DATA *destroom, bool skelerot ) );
OD                     *find_obj args( ( CHAR_DATA *ch, char *argument, bool carryonly ) );
bool ms_find_obj        args( ( CHAR_DATA *ch ) );
void worsen_mental_state args( ( CHAR_DATA *ch, int mod ) );
void better_mental_state args( ( CHAR_DATA *ch, int mod ) );
void boost_economy      args( ( AREA_DATA *tarea, int gold, int type ) );
void lower_economy      args( ( AREA_DATA *tarea, int gold, int type ) );
void economize_mobgold  args( ( CHAR_DATA *mob ) );
bool economy_has        args( ( AREA_DATA *tarea, int gold, int type ) );
void add_kill           args( ( CHAR_DATA *ch, CHAR_DATA *mob ) );
int times_killed        args( ( CHAR_DATA *ch, CHAR_DATA *mob ) );
void update_aris        args( ( CHAR_DATA *ch ) );
void calc_score         args( ( CHAR_DATA *ch ) );
AREA_DATA              *get_area args( ( char *name ) );    /* FB */
OD                     *get_objtype args( ( CHAR_DATA *ch, short type ) );
int count_users         args( ( OBJ_DATA *obj ) );
int max_weight          args( ( OBJ_DATA *obj ) );

/* interp.c */
bool check_pos          args( ( CHAR_DATA *ch, short position ) );
void                    interpret( CHAR_DATA *ch, char *argument );
bool is_number args( ( const char *arg ) );

// bool is_number          args( ( char *arg ) );
int number_argument     args( ( char *argument, char *arg ) );
const char *one_argument args( ( const char *argument, char *arg_first ) );

char                   *one_http_argument args( ( char *argument, char *arg_first ) );
char                   *one_argument args( ( char *argument, char *arg_first ) );
char                   *one_argument2 args( ( char *argument, char *arg_first ) );
ST                     *find_social args( ( const char *command ) );
CMDTYPE                *find_command args( ( const char *command ) );
void hash_commands      args( (  ) );
void start_timer        args( ( struct timeval * stime ) );
time_t end_timer        args( ( struct timeval * stime ) );
void send_timer         args( ( struct timerset * vtime, CHAR_DATA *ch ) );
void update_userec      args( ( struct timeval * time_used, struct timerset * userec ) );
BT                     *find_bank args( ( char *command ) );

/* magic.c */
bool process_spell_components args( ( CHAR_DATA *ch, int sn ) );
int ch_slookup          args( ( CHAR_DATA *ch, const char *name ) );
int find_spell          args( ( CHAR_DATA *ch, const char *name, bool know ) );
int find_skill          args( ( CHAR_DATA *ch, const char *name, bool know ) );
int find_weapon         args( ( CHAR_DATA *ch, const char *name, bool know ) );
int find_tongue         args( ( CHAR_DATA *ch, const char *name, bool know ) );
int skill_lookup        args( ( const char *name ) );
int herb_lookup         args( ( const char *name ) );
int personal_lookup     args( ( CHAR_DATA *ch, const char *name ) );
int slot_lookup         args( ( int slot ) );
int bsearch_skill       args( ( const char *name, int first, int top ) );
int bsearch_skill_exact args( ( const char *name, int first, int top ) );
int bsearch_skill_prefix args( ( const char *name, int first, int top ) );
bool saves_poison_death args( ( int level, CHAR_DATA *victim ) );
bool saves_wand         args( ( int level, CHAR_DATA *victim ) );
bool saves_para_petri   args( ( int level, CHAR_DATA *victim ) );
bool saves_breath       args( ( int level, CHAR_DATA *victim ) );
bool saves_spell_staff  args( ( int level, CHAR_DATA *victim ) );
ch_ret obj_cast_spell  
args( ( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj ) );
int dice_parse          args( ( CHAR_DATA *ch, int level, char *exp ) );
SK                     *get_skilltype args( ( int sn ) );
short get_chain_type    args( ( ch_ret retcode ) );
ch_ret chain_spells     args( ( int sn, int level, CHAR_DATA *ch, void *vo, short chain ) );

/* save.c */
/* object saving defines for fread/write_obj. -- Altrag */
#define OS_CARRY  0
#define OS_CORPSE  1
#define OS_VAULT   2
void save_char_obj      args( ( CHAR_DATA *ch ) );
bool load_char_obj     
args( ( DESCRIPTOR_DATA *d, char *name, bool preload, bool copyover, bool quiet ) );
void set_alarm          args( ( long seconds ) );
void requip_char        args( ( CHAR_DATA *ch ) );
void fwrite_obj        
args( ( CHAR_DATA *ch, OBJ_DATA *obj, FILE * fp, int iNest, short os_type, bool copyover ) );
OBJ_DATA               *fread_obj args( ( CHAR_DATA *ch, FILE * fp, short os_type ) );
void de_equip_char      args( ( CHAR_DATA *ch ) );
void re_equip_char      args( ( CHAR_DATA *ch ) );
void read_char_mobile   args( ( char *argument ) );
void write_char_mobile  args( ( CHAR_DATA *ch, char *argument ) );
CHAR_DATA              *fread_mobile args( ( FILE * fp ) );
void fwrite_mobile      args( ( FILE * fp, CHAR_DATA *mob ) );

/* special.c */
SF                     *spec_lookup args( ( const char *name ) );
const char             *lookup_spec args( ( SPEC_FUN *special ) );

/* tables.c */
int get_skill           args( ( char *skilltype ) );
const char             *spell_name args( ( SPELL_FUN *spell ) );
const char             *skill_name args( ( DO_FUN *skill ) );
void load_skill_table   args( ( void ) );
void save_skill_table   args( ( void ) );
void sort_skill_table   args( ( void ) );
void remap_slot_numbers args( ( void ) );
void load_socials       args( ( void ) );
void save_socials       args( ( void ) );
void load_bank          args( ( void ) );
void save_bank          args( ( void ) );
void load_commands      args( ( void ) );
void save_commands      args( ( void ) );
SPELL_FUN              *spell_function args( ( char *name ) );
DO_FUN                 *skill_function args( ( char *name ) );
void write_class_file   args( ( int cl ) );
void save_classes       args( ( void ) );
void                    load_classes( void );
void load_herb_table    args( ( void ) );
void save_herb_table    args( ( void ) );
void load_races         args( ( void ) );
void load_tongues       args( ( void ) );
extern int              MAX_PC_CLASS;
extern int              MAX_PC_RACE;

/* update.c */
void advance_level      args( ( CHAR_DATA *ch ) );
void advance_class_level args( ( CHAR_DATA *ch ) );
void gain_exp           args( ( CHAR_DATA *ch, int gain ) );
void gain_craft         args( ( CHAR_DATA *ch, int gain ) );
void gain_condition     args( ( CHAR_DATA *ch, int iCond, int value ) );
void update_handler     args( ( void ) );
void reboot_check       args( ( time_t reset ) );

/* build.c */
int set_hp              args( ( int level ) );
int set_armor_class     args( ( int level ) );
int set_hitroll         args( ( int level ) );
int set_damroll         args( ( int level ) );
short set_num_attacks   args( ( int level ) );
short set_max_chart     args( ( int level ) );
short set_min_chart     args( ( int level ) );
short set_curr_type     args( ( OBJ_DATA *obj, int level ) );
int set_curr_amt        args( ( OBJ_DATA *obj, int level ) );
int set_mob_currency    args( ( int level, int ctype ) );

bool                    IS_GROUPED( CHAR_DATA *ch );

/* monk.c */
void adjust_focus       args( ( CHAR_DATA *ch, short adjustment ) );

#if 0
void reboot_check       args( ( char *arg ) );
#endif

void remove_portal      args( ( OBJ_DATA *portal ) );
void weather_update     args( ( void ) );
void restore_char       args( ( CHAR_DATA *ch ) );

/* hashstr.c */
char                   *str_alloc( const char *str );
char                   *quick_link( const char *str );
int str_free            args( ( const char *str ) );
void show_hash          args( ( int count ) );
char                   *hash_stats args( ( void ) );
char                   *check_hash args( ( const char *str ) );
void hash_dump          args( ( int hash ) );
void show_high_hash     args( ( int top ) );

/* newscore.c */
int get_race            args( ( char *str ) );

/* Dual Class */
bool DUAL_SKILL         args( ( CHAR_DATA *ch, int sn ) );
int dual_adept          args( ( CHAR_DATA *ch, int sn ) );
int hp_max              args( ( CHAR_DATA *ch ) );
int hp_min              args( ( CHAR_DATA *ch ) );
int hitpoints           args( ( CHAR_DATA *ch ) );
bool use_mana           args( ( CHAR_DATA *ch ) );

/* olc stuff (oedit.c redit.c medit.c) */
void medit_parse        args( ( DESCRIPTOR_DATA *d, char *arg ) );
void redit_parse        args( ( DESCRIPTOR_DATA *d, char *arg ) );
void oedit_parse        args( ( DESCRIPTOR_DATA *d, char *arg ) );

bool is_inolc           args( ( DESCRIPTOR_DATA *d ) );
void cleanup_olc        args( ( DESCRIPTOR_DATA *d ) );

/* player.c */
char                   *get_vamp_status args( ( CHAR_DATA *ch ) );

#undef  SK
#undef  CO
#undef  ST
#undef  CD
#undef  MID
#undef  OD
#undef  OID
#undef  RID
#undef  SF
#undef  BD
#undef  CL
#undef  EDD
#undef  RD
#undef  ED

/*
 * defines for use with this get_affect function
 */

#define RIS_000  BV00
#define RIS_R00  BV01
#define RIS_0I0  BV02
#define RIS_RI0  BV03
#define RIS_00S  BV04
#define RIS_R0S  BV05
#define RIS_0IS  BV06
#define RIS_RIS  BV07

#define GA_AFFECTED  BV09
#define GA_RESISTANT  BV10
#define GA_IMMUNE  BV11
#define GA_SUSCEPTIBLE  BV12
#define GA_RIS          BV30

/*
 * mudprograms stuff
 */
extern CHAR_DATA       *supermob;

void                    oprog_speech_trigger( char *txt, CHAR_DATA *ch );
void                    oprog_random_trigger( OBJ_DATA *obj );
void                    oprog_wear_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
bool                    oprog_use_trigger( CHAR_DATA *ch, OBJ_DATA *obj, CHAR_DATA *vict,
                                           OBJ_DATA *targ, void *vo );
void                    oprog_fight_trigger( CHAR_DATA *ch, OBJ_DATA *weapon, CHAR_DATA *vch );
void                    oprog_remove_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_trash_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_damage_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_repair_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_drop_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_zap_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
char                   *oprog_type_to_name( int type );
void                    oprog_greet_trigger( CHAR_DATA *ch );
void                    oprog_get_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_examine_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_pull_trigger( CHAR_DATA *ch, OBJ_DATA *obj );
void                    oprog_push_trigger( CHAR_DATA *ch, OBJ_DATA *obj );

/* mud prog defines */

#define ERROR_PROG        -1
#define IN_FILE_PROG      -2

typedef enum
{
    ACT_PROG, SPEECH_PROG, RAND_PROG, FIGHT_PROG, DEATH_PROG, HITPRCNT_PROG,
    ENTRY_PROG, GREET_PROG, ALL_GREET_PROG, GIVE_PROG, BRIBE_PROG, HOUR_PROG,
    TIME_PROG, WEAR_PROG, REMOVE_PROG, TRASH_PROG, LOOK_PROG, EXA_PROG,
    ZAP_PROG,
    GET_PROG, DROP_PROG, DAMAGE_PROG, REPAIR_PROG, RANDIW_PROG, SPEECHIW_PROG,
    PULL_PROG, PUSH_PROG, SLEEP_PROG, REST_PROG, LEAVE_PROG, SCRIPT_PROG,
    USE_PROG, PRE_ENTER_PROG
} prog_types;

/*
 * For backwards compatability
 */
#define RDEATH_PROG DEATH_PROG
#define ENTER_PROG  ENTRY_PROG
#define RFIGHT_PROG FIGHT_PROG
#define RGREET_PROG GREET_PROG
#define OGREET_PROG GREET_PROG

void                    rprog_leave_trigger( CHAR_DATA *ch );
void                    rprog_enter_trigger( CHAR_DATA *ch );
void                    rprog_sleep_trigger( CHAR_DATA *ch );
void                    rprog_rest_trigger( CHAR_DATA *ch );
void                    rprog_rfight_trigger( CHAR_DATA *ch );
void                    rprog_death_trigger( CHAR_DATA *killer, CHAR_DATA *ch );
void                    rprog_speech_trigger( char *txt, CHAR_DATA *ch );
void                    rprog_random_trigger( CHAR_DATA *ch );
void                    rprog_time_trigger( CHAR_DATA *ch );
void                    rprog_hour_trigger( CHAR_DATA *ch );
char                   *rprog_type_to_name( int type );

#define OPROG_ACT_TRIGGER
#ifdef OPROG_ACT_TRIGGER
void                    oprog_act_trigger( char *buf, OBJ_DATA *mobj, CHAR_DATA *ch, OBJ_DATA *obj,
                                           void *vo );
#endif
#define RPROG_ACT_TRIGGER
#ifdef RPROG_ACT_TRIGGER
void                    rprog_act_trigger( char *buf, ROOM_INDEX_DATA *room, CHAR_DATA *ch,
                                           OBJ_DATA *obj, void *vo );
#endif

#define LEARNED(ch,sn)      (IS_NPC(ch) ? 80 : URANGE(0, ch->pcdata->learned[sn], 101))

/* Structure and macros for using long bit vectors */
#define CHAR_SIZE sizeof(char)

typedef char           *LONG_VECTOR;

#define LV_CREATE(vector, bit_length) \
do \
{ \
  int i; \
  CREATE(vector, char, 1 + bit_length/CHAR_SIZE); \
 \
  for(i = 0; i <= bit_length/CHAR_SIZE; i++) \
  *(vector + i) = 0; \
}while(0)

#define LV_IS_SET(vector, index) \
  (*(vector + index/CHAR_SIZE) & (1 << index%CHAR_SIZE))

#define LV_SET_BIT(vector, index) \
  (*(vector + index/CHAR_SIZE) |= (1 << index%CHAR_SIZE))

#define LV_REMOVE_BIT(vector, index) \
  (*(vector + index/CHAR_SIZE) &= ~(1 << index%CHAR_SIZE))

#define LV_TOGGLE_BIT(vector, index) \
  (*(vector + index/CHAR_SIZE) ^= (1 << index%CHAR_SIZE))

/* mxp stuff - added by Nick Gammon - 18 June 2001 */
/*
  To simply using MXP we'll use special tags where we want to use MXP tags
  and then change them to <, > and & at the last moment.
 
   eg. MXP_BEG "send" MXP_END    becomes: <send>
       MXP_AMP "version;"        becomes: &version;
*/

/* strings */
#define MXP_BEG "\x03"                                 /* becomes < */
#define MXP_END "\x04"                                 /* becomes > */
#define MXP_AMP "\x05"                                 /* becomes & */

/* characters */
#define MXP_BEGc '\x03'                                /* becomes < */
#define MXP_ENDc '\x04'                                /* becomes > */
#define MXP_AMPc '\x05'                                /* becomes & */

// constructs an MXP tag with < and > around it
#define MXPTAG(arg) MXP_BEG arg MXP_END

#define ESC "\x1B"                                     /* esc character */

#define MXPMODE(arg) ESC "[" #arg "z"

#define MXP_open 0                                     /* only MXP commands in the "open" 
                                                        * category are allowed.  */
#define MXP_secure 1                                   /* all tags and commands in MXP
                                                        * are allowed within the line.  */
#define MXP_locked 2                                   /* no MXP or HTML commands are
                                                        * allowed in the line.  The line
                                                        * is not parsed for any tags at
                                                        * all.  */
#define MXP_reset 3                                    /* close all open tags */
#define MXP_secure_once 4                              /* next tag is secure only */
#define MXP_perm_open 5                                /* open mode until mode change */
#define MXP_perm_secure 6                              /* secure mode until mode change */
#define MXP_perm_locked 7                              /* locked mode until mode change */

/* telnet options */
#define TELOPT_BINARY             0                    /* 8-bit data path */
#define TELOPT_ECHO               1                    /* echo */
#define TELOPT_RCP                2                    /* prepare to reconnect */
#define TELOPT_SGA                3                    /* suppress go ahead */
#define TELOPT_NAMS               4                    /* approximate message size */
#define TELOPT_STATUS             5                    /* give status */
#define TELOPT_TM                 6                    /* timing mark */
#define TELOPT_RCTE               7                    /* remote controlled transmission
                                                        * and echo */
#define TELOPT_NAOL               8                    /* negotiate about output line
                                                        * width */
#define TELOPT_NAOP               9                    /* negotiate about output page
                                                        * size */
#define TELOPT_NAOCRD            10                    /* negotiate about CR disposition */
#define TELOPT_NAOHTS            11                    /* negotiate about horizontal
                                                        * tabstops */
#define TELOPT_NAOHTD            12                    /* negotiate about horizontal tab
                                                        * disposition */
#define TELOPT_NAOFFD            13                    /* negotiate about formfeed
                                                        * disposition */
#define TELOPT_NAOVTS            14                    /* negotiate about vertical tab
                                                        * stops */
#define TELOPT_NAOVTD            15                    /* negotiate about vertical tab
                                                        * disposition */
#define TELOPT_NAOLFD            16                    /* negotiate about output LF
                                                        * disposition */
#define TELOPT_XASCII            17                    /* extended ascic character set */
#define TELOPT_LOGOUT            18                    /* force logout */
#define TELOPT_BM                19                    /* byte macro */
#define TELOPT_DET               20                    /* data entry terminal */
#define TELOPT_SUPDUP            21                    /* supdup protocol */
#define TELOPT_SUPDUPOUTPUT      22                    /* supdup output */
#define TELOPT_SNDLOC            23                    /* send location */
#define TELOPT_TTYPE             24                    /* terminal type */
#define TELOPT_EOR               25                    /* end or record */
#define TELOPT_TUID              26                    /* TACACS user identification */
#define TELOPT_OUTMRK            27                    /* output marking */
#define TELOPT_TTYLOC            28                    /* terminal location number */
#define TELOPT_3270REGIME        29                    /* 3270 regime */
#define TELOPT_X3PAD             30                    /* X.3 PAD */
#define TELOPT_NAWS              31                    /* window size */
#define TELOPT_TSPEED            32                    /* terminal speed */
#define TELOPT_LFLOW             33                    /* remote flow control */
#define TELOPT_LINEMODE          34                    /* Linemode option */
#define TELOPT_XDISPLOC          35                    /* X Display Location */
#define TELOPT_OLD_ENVIRON       36                    /* Old - Environment variables */
#define TELOPT_AUTHENTICATION    37                    /* Authenticate */
#define TELOPT_ENCRYPT           38                    /* Encryption option */
#define TELOPT_NEW_ENVIRON       39                    /* New - Environment variables */
#define TELOPT_3270E             40                    /* 3270 Extended (RFC 1647) */
#define TELOPT_XAUTH             41                    /* ??? (Earhart) */
#define TELOPT_CHARSET           42                    /* Character-set (RFC 2066) */
#define TELOPT_RSP               43                    /* Remote Serial Port (Barnes) */
#define TELOPT_COM_PORT          44                    /* Com Port Control (RFC 2217) */
#define TELOPT_SLE               45                    /* Suppress Local Echo (Atmar) -
                                                        * rejected */
#define TELOPT_START_TLS         46                    /* Start TLS
                                                        * Authentication/Encryption */
#define TELOPT_KERMIT            47                    /* Kermit (altman) */
#define TELOPT_SEND_URL          48                    /* Send URL */
#define TELOPT_COMPRESS          85                    /* MCCP (version 1) */
#define TELOPT_MSP               90                    /* MSP */
#define TELOPT_MXP               91                    /* MXP */
#define TELOPT_COMPRESS2 86
#define TELOPT_PRAGMA_LOGON     138                    /* Encrypted Logon option
                                                        * (PragmaSys) */
#define TELOPT_SSPI_LOGON       139                    /* MS SSPI Logon option
                                                        * (PragmaSys) */
#define TELOPT_PRAGMA_HEARTBEAT 140                    /* Server Heartbeat option
                                                        * (PragmaSys) */
#define TELOPT_IBM_SAK          200                    /* IBM Secure Attention Key (not
                                                        * registered) */

#define TELOPT_EXOPL            255                    /* extended-options-list */

#define IAC     255                                    /* interpret as command: */
#define DONT    254                                    /* you are not to use option */
#define DO      253                                    /* please, you use option */
#define WONT    252                                    /* I won't use option */
#define WILL    251                                    /* I will use option */
#define SB      250                                    /* interpret as subnegotiation */
#define GA      249                                    /* you may reverse the line */
#define EL      248                                    /* erase the current line */
#define EC      247                                    /* erase the current character */
#define AYT     246                                    /* are you there */
#define AO      245                                    /* abort output--but let prog
                                                        * finish */
#define IP      244                                    /* interrupt process--permanently */
#define BREAK   243                                    /* break */
#define DM      242                                    /* data mark--for connect.
                                                        * cleaning */
#define NOP     241                                    /* nop */
#define SE      240                                    /* end sub negotiation */
#define EOR     239                                    /* end of record (transparent
                                                        * mode) */
#define ABORT   238                                    /* Abort process */
#define SUSP    237                                    /* Suspend process */
#define xEOF    236                                    /* End of file: EOF is already
                                                        * used... */

#define SYNCH   242                                    /* for telfunc calls */

const char              echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char              echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };

// const char go_ahead_str [] = { IAC, GA, '\0' };
const char              will_mxp_str[] = { IAC, WILL, TELOPT_MXP, '\0' };
const char              start_mxp_str[] = { IAC, SB, TELOPT_MXP, IAC, SE, '\0' };
const char              do_mxp_str[] = { IAC, DO, TELOPT_MXP, '\0' };
const char              dont_mxp_str[] = { IAC, DONT, TELOPT_MXP, '\0' };

#define TELOPT_COMPRESS 85
const char              eor_on_str[] = { IAC, WILL, TELOPT_EOR, '\0' };
const char              compress_on_str[] = { IAC, WILL, TELOPT_COMPRESS, '\0' };
const char              compress2_on_str[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };

#ifdef WIN32
void                    gettimeofday( struct timeval *tv, struct timezone *tz );
void                    kill_timer(  );

/* directory scanning stuff */

struct dirent
{
    char                   *d_name;
};

typedef struct
{
    HANDLE                  hDirectory;
    WIN32_FIND_DATA         Win32FindData;
    struct dirent           dirinfo;
    char                    sDirName[MAX_PATH];
} DIR;

DIR                    *opendir( char *sDirName );
struct dirent          *readdir( DIR * dp );
void                    closedir( DIR * dp );

/* --------------- Stuff for Win32 services ------------------ */
/*

   NJG:

   When "exit" is called to handle an error condition, we really want to
   terminate the game thread, not the whole process.

 */

#define exit(arg) Win32_Exit(arg)
void                    Win32_Exit( int exit_code );

#endif

#define GET_BETTED_ON(ch)    ((ch)->betted_on)
#define GET_BET_AMT(ch) ((ch)->bet_amt)
#define IN_ARENA(ch)            (IS_SET((ch)->in_room->room_flags, ROOM_ARENA))

/**************************************************
* used for xp calculations default is 1 and 1 the xp cals
* use this multiplier at the final so if you want to adjust
* do in small incraments such as 1.50 then test your results
* going above 1 will increase the xp given going below 1 will 
* decrease it
**************************************************/
#define XP_MULT					1
#define HIT_MULT				1
#define FINAL_XP_MULT(a)		(a*XP_MULT)
#define HIT_XP_MULT(a)			(a*HIT_MULT)

bool                    IS_AFFECTED( CHAR_DATA *ch, int arg );
