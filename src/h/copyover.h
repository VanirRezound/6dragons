/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - AFKMud     Copyright 1997, 1998, 1999, 2000, 2001, 2002 by Alsherok   *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Copyover module header                                                *
 ***************************************************************************/

#ifndef CH
#define CH(d)   ((d)->original ? (d)->original : (d)->character)
#endif

#ifndef MSL
#define MSL MSL
#endif

#define COPYOVER_FILE SYSTEM_DIR "copyover.dat"        /* for copyovers */
#define EXE_FILE "./6dragons"
#define COPYOVER_DIR "copyover/"                       /* For storing objects across
                                                        * copyovers */
#define MOB_FILE  "mobs.dat"                           /* For storing mobs across
                                                        * copyovers */

/* warmboot code */
void                    copyover_recover( void );
void                    load_world( CHAR_DATA *ch );
