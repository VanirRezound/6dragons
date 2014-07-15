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
/* Cause we gotta have structure */

typedef struct clan_data CLAN_DATA;
typedef struct council_data COUNCIL_DATA;
void save_clan_storeroom args( ( CHAR_DATA *ch ) );

#define MAX_CLAN		   50

typedef enum
{
    CLAN_PLAIN, CLAN_VAMPIRE, CLAN_WARRIOR, CLAN_DRUID, CLAN_MAGE, CLAN_CELTIC,
    CLAN_THIEF, CLAN_CLERIC, CLAN_UNDEAD, CLAN_CHAOTIC, CLAN_NEUTRAL,
    CLAN_LAWFUL,
    CLAN_NOKILL
} clan_types;

typedef enum
{
    GROUP_CLAN, GROUP_COUNCIL
} group_types;

typedef struct pkillarea_data PKILLAREA_DATA;

struct pkillarea_data
{
    PKILLAREA_DATA         *next;
    PKILLAREA_DATA         *prev;
    char                   *name;
};

typedef struct roster_data ROSTER_DATA;
void                    remove_roster( CLAN_DATA * clan, char *name );
void                    add_roster( CLAN_DATA * clan, char *name, char *race, int level, int pkills,
                                    int kills, int deaths, int tradeclass, int tradelevel );
void                    update_roster( CHAR_DATA *ch );
struct roster_data
{
    ROSTER_DATA            *next;
    ROSTER_DATA            *prev;
    char                   *name;
    time_t                  joined;
    time_t                  lastupdated;
    char                   *race;
    int                     level;
    int                     pkills;
    int                     kills;
    int                     deaths;
    int                     tradeclass;
    int                     tradelevel;
};

struct clan_data
{
    CLAN_DATA              *next;                      /* next clan in list */
    CLAN_DATA              *prev;                      /* previous clan in list */
    ROSTER_DATA            *first_member;
    ROSTER_DATA            *last_member;
    char                   *filename;                  /* Clan filename */
    char                   *name;                      /* Clan name */
    char                   *motto;                     /* Clan motto */
    char                   *description;               /* A brief description of the clan 
                                                        */
    char                   *chieftain;
    char                   *warmaster;
    char                   *badge;                     /* Clan badge on who/where/to_room 
                                                        */
    char                   *intro;                     /* Clan introduction (seen when
                                                        * logging on) */
    int                     pkills[10];                /* Number of pkills on behalf of
                                                        * clan */
    int                     pdeaths[10];               /* Number of pkills against clan */
    int                     mkills;                    /* Number of mkills on behalf of
                                                        * clan */
    int                     mdeaths;                   /* Number of clan deaths due to
                                                        * mobs */
    int                     illegal_pk;                /* Number of illegal pk's by clan */
    int                     score;                     /* Overall score */
    short                   clan_type;                 /* See clan type defines */
    short                   members;                   /* Number of clan members */
    short                   mem_limit;                 /* Number of clan members allowed */
    short                   alignment;                 /* Clan's general alignment */
    short                   arena_victory;             /* Total arena victories by all
                                                        * members. */
    int                     board;                     /* Vnum of clan board */
    int                     clanobj1;                  /* Vnum of first clan obj */
    int                     clanobj2;                  /* Vnum of second clan obj */
    int                     clanobj3;                  /* Vnum of third clan obj */
    int                     clanobj4;                  /* Vnum of fourth clan obj */
    int                     clanobj5;                  /* Vnum of fifth clan obj */
    int                     recall;                    /* Vnum of clan's recall room */
    int                     guard1;                    /* Vnum of clan guard type 1 */
    int                     guard2;                    /* Vnum of clan guard type 2 */
    int                     status;
    int                     totalpoints;               // total # of clan points clan
                                                       // members
    // contribute
    PKILLAREA_DATA         *first_pkillarea;
    PKILLAREA_DATA         *last_pkillarea;
    int                     chieflog;
    int                     warlog;
};

void                    update_member( CHAR_DATA *ch );

struct council_data
{
    COUNCIL_DATA           *next;                      /* next council in list */
    COUNCIL_DATA           *prev;                      /* previous council in list */
    char                   *filename;                  /* Council filename */
    char                   *name;                      /* Council name */
    char                   *description;               /* A brief description of the
                                                        * council */
    char                   *head;                      /* Council head */
    char                   *head2;                     /* Council co-head */
    char                   *powers;                    /* Council powers */
    short                   members;                   /* Number of council members */
    int                     board;                     /* Vnum of council board */
    int                     meeting;                   /* Vnum of council's meeting room */
};

bool                    is_valid_filename( CHAR_DATA *ch, const char *direct,
                                           const char *filename );

#define IS_CLANNED(ch) ( !IS_NPC( (ch) ) && (ch)->pcdata->clan )
#define IS_CHIEFTAIN(ch) ( IS_CLANNED( (ch) ) && VLD_STR( (ch)->pcdata->clan->chieftain ) && !str_cmp( (ch)->pcdata->clan->chieftain, (ch)->name ) )
#define IS_WARMASTER(ch) ( IS_CLANNED( (ch) ) && VLD_STR( (ch)->pcdata->clan->warmaster ) && !str_cmp( (ch)->pcdata->clan->warmaster, (ch)->name ) )
#define IS_INFLUENCED(ch, area) ( IS_CLANNED( (ch) ) && (area)->influencer && (ch)->pcdata->clan == (area)->influencer )

extern CLAN_DATA       *first_clan;
extern CLAN_DATA       *last_clan;
extern COUNCIL_DATA    *first_council;
extern COUNCIL_DATA    *last_council;

DECLARE_DO_FUN( do_victories );

#define CL	CLAN_DATA
#define	CO	COUNCIL_DATA

/* clans.c */
CL                     *get_clan( const char *name );
void                    load_clans( void );
void                    save_clan( CLAN_DATA * clan );
CO                     *get_council( const char *name );
void                    load_councils( void );
void                    save_council( COUNCIL_DATA * council );
