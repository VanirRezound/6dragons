/****************************************************************************
 * AFKMud Copyright 1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,   *
 * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                             Alias module                                 *
 ****************************************************************************/

/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

typedef struct alias_type ALIAS_DATA;

struct alias_type
{
    ALIAS_DATA             *next;
    ALIAS_DATA             *prev;
    char                   *name;
    char                   *cmd;
};

void                    free_aliases( CHAR_DATA *ch );
bool                    check_alias( CHAR_DATA *ch, char *command, char *argument );
ALIAS_DATA             *find_alias( CHAR_DATA *ch, char *argument );
