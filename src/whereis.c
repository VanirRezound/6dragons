/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 *   LoP (C) 2006, 2007, 2008, 2009 by: the LoP team.                      *
 ***************************************************************************
 * - Tracking/hunting module                                               *
 ***************************************************************************/

#include "h/mud.h"
#include "h/track.h"
#include "h/hometowns.h"

/* This is a great idea heh, or not but an attempt to replace whereis lua
with Remcon's new track code.  Could let us do away with lua if it works out.  */

int                     find_first_step( ROOM_INDEX_DATA *src, ROOM_INDEX_DATA *target,
                                         int maxdist );

void do_whereis( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vict;
    char                    arg[MIL],
                            fpath[MSL * 2];
    int                     dir,
                            maxdist,
                            lastdir = 0,
        lastnumber = 0;
    bool                    firststep = true;
    HTOWN_DATA             *htown;
    ROOM_INDEX_DATA        *location = NULL;

    if ( IS_NPC( ch ) ) {
        return;
    }

    htown = get_htown( ch->pcdata->htown_name );
    if ( !htown ) {
        send_to_char( "You have no hometown assigned.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( ch->in_room && str_cmp( ch->in_room->area->filename, "paleon.are" )
         && str_cmp( ch->in_room->area->filename, "dakar.are" )
         && str_cmp( ch->in_room->area->filename, "forbidden.are" )
         && str_cmp( ch->in_room->area->filename, "tutorial.are" )
         && str_cmp( ch->in_room->area->filename, "etutorial.are" )
         && str_cmp( ch->in_room->area->filename, "dtutorial.are" ) ) {
        send_to_char( "You must be in your home town to use this command.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' && !str_cmp( htown->name, "Paleon City" ) ) {
        if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "paleon.are" )
             || !str_cmp( ch->in_room->area->filename, "tutorial.are" ) ) {
            send_to_char( "&c    Destination : Mob name\r\n", ch );
            send_to_char( "-----------------------------\r\n", ch );
            send_to_char( "        Armorer : &CVaekon&c\r\n", ch );
            send_to_char( "           Bank : &CJoram&c\r\n", ch );
            send_to_char( "East gate guard : &CMarak&c\r\n", ch );
            send_to_char( "         Healer : &CJulias&c\r\n", ch );
            send_to_char( "      Librarian : &CFysklor&c\r\n", ch );
            send_to_char( "        Magical : &CAmelia&c\r\n", ch );
            send_to_char( "West gate guard : &CDunar&c\r\n", ch );
            send_to_char( "        Petshop : &CAnimal&c\r\n", ch );
            send_to_char( "        Repairs : &CTier&c\r\n", ch );
            send_to_char( "        Resizer : &CGerald&c\r\n", ch );
            send_to_char( "         Square : &CCentral&c\r\n", ch );
            send_to_char( "         Stable : &CJonah&c\r\n", ch );
            send_to_char( "       Supplies : &CMaekans&c\r\n", ch );
            send_to_char( "         Tavern : &CMalachi&c\r\n", ch );
            send_to_char( "        Teacher : &CSymon&c\r\n", ch );
            send_to_char( "     Undertaker : &CVicar&c\r\n", ch );
            send_to_char( "        Weapons : &CLorax&c\r\n\r\n", ch );

            send_to_char
                ( "Type 'whereis [&Cmob&c]' to get a suggested route (eg. 'whereis Vaekon').\r\n",
                  ch );
            return;
        }
    }
    else if ( arg[0] == '\0' && !str_cmp( htown->name, "Dakar City" ) ) {
        if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "dakar.are" )
             || !str_cmp( ch->in_room->area->filename, "etutorial.are" ) ) {
            send_to_char( "&c    Destination : Mob name\r\n", ch );
            send_to_char( "-----------------------------\r\n", ch );
            send_to_char( "        Armorer : &CRautal&c\r\n", ch );
            send_to_char( "           Bank : &CGrindatck&c\r\n", ch );
            send_to_char( "         Healer : &CYochlol&c\r\n", ch );
            send_to_char( "        Magical : &CDralak&c\r\n", ch );
            send_to_char( "     North gate : &CDravea&c\r\n", ch );
            send_to_char( "        Petshop : &CGyakas&c\r\n", ch );
            send_to_char( "       Pawn shop: &CPawn&c\r\n", ch );
            send_to_char( "        Repairs : &CTakarz&c\r\n", ch );
            send_to_char( "        Resizer : &CPaetael&c\r\n", ch );
            send_to_char( "         Square : &CCentral&c\r\n", ch );
            send_to_char( "       Supplies : &CSupply&c\r\n", ch );
            send_to_char( "         Tavern : &CKorlok&c\r\n", ch );
            send_to_char( "        Teacher : &CJardal&c\r\n", ch );
            send_to_char( "     Undertaker : &CHakast&c\r\n", ch );
            send_to_char( "        Weapons : &CDaktar&c\r\n", ch );
            send_to_char( "    Black Tower : &CDurgas&c\r\n\r\n", ch );

            send_to_char
                ( "Type 'whereis [&Cmob&c]' to get a suggested route (eg. 'whereis Rautal').\r\n",
                  ch );
            return;
        }
    }
    else if ( arg[0] == '\0' && !str_cmp( htown->name, "Forbidden City" ) ) {
        if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "forbidden.are" )
             || !str_cmp( ch->in_room->area->filename, "dtutorial.are" ) ) {
            send_to_char( "    &cDestination : Mob name\r\n", ch );
            send_to_char( "-----------------------------\r\n", ch );
            send_to_char( "        Armorer : &CUmbar&c\r\n", ch );
            send_to_char( "           Bank : &CHaevar&c\r\n", ch );
            send_to_char( "         Healer : &CRaesaav&c\r\n", ch );
            send_to_char( "        Magical : &CShozat&c\r\n", ch );
            send_to_char( "        Repairs : &CUalnar&c\r\n", ch );
            send_to_char( "        Resizer : &CCerax&c\r\n", ch );
            send_to_char( "    High Perche : &CCentral&c\r\n", ch );
            send_to_char( "       Supplies : &CDretaz&c\r\n", ch );
            send_to_char( "         Tavern : &CTaenoc&c\r\n", ch );
            send_to_char( "        Teacher : &CUlave&c\r\n", ch );
            send_to_char( "     Undertaker : &CEustar&c\r\n", ch );
            send_to_char( "        Weapons : &CChorak&c\r\n", ch );
            send_to_char( "  City Entrance : &CGsaar&c\r\n\r\n", ch );

            send_to_char
                ( "&cType 'whereis [&Cmob&c]' to get a suggested route (eg. 'whereis Ulave').\r\n",
                  ch );
            return;
        }
    }

    if ( !str_cmp( ch->in_room->area->filename, "forbidden.are" )
         && ( !str_cmp( htown->name, "Paleon City" ) || !str_cmp( htown->name, "Dakar City" ) ) ) {
        send_to_char( "This is not your homeland you don't know your way around.\r\n", ch );
        return;
    }
    else if ( !str_cmp( ch->in_room->area->filename, "paleon.are" )
              && ( !str_cmp( htown->name, "Forbidden City" )
                   || !str_cmp( htown->name, "Dakar City" ) ) ) {
        send_to_char( "This is not your homeland you don't know your way around.\r\n", ch );
        return;
    }
    else if ( !str_cmp( ch->in_room->area->filename, "dakar.are" )
              && ( !str_cmp( htown->name, "Paleon City" )
                   || !str_cmp( htown->name, "Forbidden City" ) ) ) {
        send_to_char( "This is not your homeland you don't know your way around.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "vaskar" ) ) {
        send_to_char( "&cHis whereabouts are unknown, you must search for him.\r\n", ch );
        return;
    }

    if ( str_cmp( arg, "central" ) ) {
        if ( !( vict = get_char_area( ch, arg ) ) ) {
            send_to_char
                ( "&cType 'whereis [&Cmob&c]' to get a suggested route (eg. 'whereis Vaekon'). \r\n",
                  ch );
            return;
        }
    }
    maxdist = 100 + ch->level * 30;

    /*
     * Ok I want it to give a nice quick overview to how to get there 
     */
    {
        ROOM_INDEX_DATA        *fromroom = ch->in_room;
        int                     steps = 0;

        fpath[0] = '\0';

        if ( !str_cmp( arg, "central" ) && !str_cmp( ch->in_room->area->filename, "dakar.are" ) ) {
            location = get_room_index( 11210 );
        }
        else if ( !str_cmp( arg, "central" )
                  && !str_cmp( ch->in_room->area->filename, "paleon.are" ) ) {
            location = get_room_index( 16007 );
        }
        else if ( !str_cmp( arg, "central" )
                  && !str_cmp( ch->in_room->area->filename, "forbidden.are" ) ) {
            location = get_room_index( 35515 );
        }
        else if ( !str_cmp( arg, "central" )
                  && !str_cmp( ch->in_room->area->filename, "tutorial.are" ) ) {
            location = get_room_index( 5100 );
        }
        else if ( !str_cmp( arg, "central" )
                  && !str_cmp( ch->in_room->area->filename, "etutorial.are" ) ) {
            location = get_room_index( 33210 );
        }
        else if ( !str_cmp( arg, "central" )
                  && !str_cmp( ch->in_room->area->filename, "dtutorial.are" ) ) {
            location = get_room_index( 19015 );
        }

        for ( steps = 0; steps < 1000; steps++ ) {
            if ( !str_cmp( arg, "central" ) ) {
                dir = find_first_step( fromroom, location, maxdist );
            }
            else {
                dir = find_first_step( fromroom, vict->in_room, maxdist );
            }
            if ( steps == 0 ) {
                if ( dir == BFS_ERROR ) {
                    send_to_char( "Hmm... At some point something went wrong.\r\n", ch );
                    return;
                }
                else if ( dir == BFS_ALREADY_THERE ) {
                    send_to_char( "You're already in the same room!\r\n", ch );
                    return;
                }
                else if ( dir == BFS_NO_PATH ) {
                    send_to_char
                        ( "Type 'whereis [&Cmob&c]' to get a suggested route (eg. 'whereis Vaekon').\r\n",
                          ch );
                    return;
                }

                lastdir = dir;
            }
            else {
                if ( dir == BFS_ERROR ) {
                    send_to_char
                        ( "Hmm... At some point something went wrong.\r\nsomething seems to be wrong.\r\n",
                          ch );
                    return;
                }
            }

            /*
             * Ok now for the fun stuff we want to get the direction name and number of times
             * and then put them all together 
             */
            if ( lastdir != dir ) {
                char                    snum[MIL];

                snprintf( snum, sizeof( snum ), "%s%d %s", !firststep ? ", " : "", lastnumber,
                          dir_name[lastdir] );
                mudstrlcat( fpath, snum, sizeof( fpath ) );

                lastnumber = 1;
                firststep = false;
                lastdir = dir;
            }
            else
                lastnumber++;

            if ( dir == BFS_ALREADY_THERE || dir == BFS_NO_PATH )
                break;

            fromroom = get_exit( fromroom, dir )->to_room;  /* Change fromroom
                                                             * information */
        }
    }
    ch_printf( ch, "To get to %s walk: %s.\r\n", arg, fpath );
}
