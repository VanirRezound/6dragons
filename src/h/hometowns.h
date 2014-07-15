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

/* My attempt to reduce the size of my executable by putting this stuff in its own h file */
/* Vladaar - 2-10-02 CTA */

/* Structure is a good thing */
typedef struct htown_data HTOWN_DATA;

struct htown_data
{
    HTOWN_DATA             *next;                      /* next hometown in list */
    HTOWN_DATA             *prev;                      /* previous hometown in list */
    char                   *filename;                  /* Hometown filename */
    char                   *name;                      /* Hometown name */
    char                   *description;               /* A brief description of the
                                                        * hometown */
    char                   *ruler;                     /* Hometown Ruler */
    char                   *general;                   /* Hometown general */
    char                   *race;                      /* hometown race */
    int                     recall;                    /* hometown recall */
    int                     startroom;
    int                     temple;
    char                   *nation;
    short                   members;                   /* Number of hometown members */
};

/* Externals Rule */

extern HTOWN_DATA      *first_htown;
extern HTOWN_DATA      *last_htown;

#define HT      HTOWN_DATA

/* hometowns.c */

HT                     *get_htown args( ( const char *name ) );
void load_htowns        args( ( void ) );
void save_htown         args( ( HTOWN_DATA * htown ) );
