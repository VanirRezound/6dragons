/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Ban module header                                                     *
 ***************************************************************************/

typedef struct ban_data BAN_DATA;

/*
 * Ban Types --- Shaddai
 */
typedef enum
{
    BAN_WARN = -1, BAN_SITE = 1, BAN_CLASS, BAN_RACE
} ban_types;

/*
 * Site ban structure.
 */
struct ban_data
{
    BAN_DATA               *next;
    BAN_DATA               *prev;
    char                   *name;                      /* Name of site/class/race banned */
    char                   *user;                      /* Name of user from site */
    char                   *note;                      /* Why it was banned */
    char                   *ban_by;                    /* Who banned this site */
    char                   *ban_time;                  /* Time it was banned */
    int                     flag;                      /* Class or Race number */
    int                     unban_date;                /* When ban expires */
    short                   duration;                  /* How long it is banned for */
    short                   level;                     /* Level that is banned */
    bool                    warn;                      /* Echo on warn channel */
    bool                    prefix;                    /* Use of *site */
    bool                    suffix;                    /* Use of site* */
};

extern BAN_DATA        *first_ban;
extern BAN_DATA        *last_ban;
extern BAN_DATA        *first_ban_class;
extern BAN_DATA        *last_ban_class;
extern BAN_DATA        *first_ban_race;
extern BAN_DATA        *last_ban_race;
