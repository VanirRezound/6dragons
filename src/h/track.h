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

/* track.c */
void found_prey         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void hunt_victim        args( ( CHAR_DATA *ch ) );

#define BFS_ERROR     -1
#define BFS_ALREADY_THERE  -2
#define BFS_NO_PATH     -3
#define BFS_MARK    536870912
