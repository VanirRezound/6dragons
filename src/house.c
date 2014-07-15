/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                     Housing Module Source File                           *
 ****************************************************************************
 * Author : Senir                                                           *
 * E-Mail : oldgaijin@yahoo.com                                             *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include "h/mud.h"
#include "h/clans.h"
#include "h/files.h"
#include "h/house.h"

void                    wipe_resets( ROOM_INDEX_DATA *room );

HOME_DATA              *first_home;
HOME_DATA              *last_home;
HOMEBUY_DATA           *first_homebuy;
HOMEBUY_DATA           *last_homebuy;
LMSG_DATA              *first_lmsg;
LMSG_DATA              *last_lmsg;
ACCESSORIES_DATA       *first_accessory;
ACCESSORIES_DATA       *last_accessory;

void do_house( CHAR_DATA *ch, char *argument )
{

    char                    arg[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim = NULL;
    ROOM_INDEX_DATA        *location = NULL;
    HOME_DATA              *homedata = NULL;
    short                   i = 0;

    switch ( ch->substate ) {

        default:
            break;

        case SUB_ROOM_DESC:

            location = ( ROOM_INDEX_DATA * ) ch->dest_buf;
            if ( !location ) {
                bug( "house: sub_room_desc: NULL ch->dest_buf" );
                location = ch->in_room;
            }

            STRFREE( location->description );
            location->description = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;
            save_residence( location );
            send_to_char_color( "&C&wHouse room description set.\r\n", ch );
            return;

    }

    if ( IS_NPC( ch ) ) {
        send_to_char_color( "&C&wMob's don't have houses.\r\n", ch );
        return;
    }

    for ( homedata = first_home; homedata; homedata = homedata->next ) {
        if ( !str_cmp( homedata->name, ch->name ) )
            break;
    }

    if ( ( !homedata || homedata->apartment ) && !IS_IMMORTAL( ch ) ) {
        send_to_char_color( "&C&wYou don\'t have a house.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char_color( "&C&BSyntax:\r\n", ch );
        send_to_char_color( "&C&G house <argument>\r\n", ch );
        send_to_char_color( "&C&BWhere the argument is one of:\r\n", ch );
        send_to_char_color( "&C&G name <Desired Title of Room>\r\n desc\r\n", ch );
        send_to_char_color( "&C&G addroom <direction>\r\n", ch );

        if ( IS_IMMORTAL( ch ) ) {
            send_to_char_color( "&C&BImmortal Arguments:\r\n", ch );
            send_to_char_color( "&C&G set <character name> <vnum> (apartment)\r\n", ch );
            send_to_char_color
                ( "&C&G set <character name> addroom <current house vnum> <direction>\r\n", ch );
            send_to_char_color( "&C&G remove <character name>\r\n", ch );
            send_to_char_color( "&C&G givekey <character name>\r\n", ch );
        }

        return;
    }

    argument = one_argument( argument, arg );

    if ( homedata ) {

        for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
            if ( homedata->vnum[i] == ch->in_room->vnum ) {
                location = ch->in_room;
                break;
            }
        }

        if ( !location ) {
            send_to_char_color( "&C&wNot in a house room. Editing your first house room.\r\n", ch );

            if ( ( location = get_room_index( homedata->vnum[0] ) ) == NULL ) {
                send_to_char_color
                    ( "&C&wYour first house room doesn't exist. Please report to immortal.\r\n",
                      ch );
                return;
            }

        }
    }

    if ( !str_cmp( arg, "name" ) ) {

        if ( !argument[0] || argument[0] == '\0' ) {
            send_to_char_color( "&C&wHouse name <Desired Title of Room>\r\n", ch );
            return;
        }

        if ( !homedata || homedata->apartment ) {
            send_to_char_color( "&C&wYou don\'t have a house.\r\n", ch );
            return;
        }

        STRFREE( location->name );
        location->name = STRALLOC( argument );
        save_residence( location );
        send_to_char_color( "&C&wRoom title set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "desc" ) ) {
        if ( !homedata || homedata->apartment ) {
            send_to_char_color( "&C&wYou don\'t have a house.\r\n", ch );
            return;
        }

        ch->tempnum = SUB_NONE;
        ch->substate = SUB_ROOM_DESC;
        ch->dest_buf = location;
        start_editing( ch, location->description );
        /*
         * Uncomment line below if you use Cronel's Editor 
         */
        // editor_desc_printf( ch, "Description of \"%s.\"", location->name );
        return;
    }

    if ( !str_cmp( arg, "addroom" ) ) {

        if ( !argument || argument == '\0' ) {
            send_to_char_color( "&C&wHouse addroom <direction>\r\n", ch );
            return;
        }

        if ( !homedata || homedata->apartment ) {
            send_to_char_color( "&C&wYou don\'t have a house.\r\n", ch );
            return;
        }

        if ( ch->gold < ADDITIONAL_ROOM_COST ) {
            send_to_char_color( "&C&wYou don't have enough to buy an additional room.\r\n", ch );
            return;
        }

        for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
            if ( homedata->vnum[i] == 0 )
                break;
        }

        if ( i == MAX_HOUSE_ROOMS ) {
            send_to_char_color( "&C&wYou have the max allowable rooms in your house.\r\n", ch );
            return;
        }

        if ( ( get_exit( location, get_dir( argument ) ) ) != NULL ) {
            send_to_char_color( "&C&wThere's already a room in that direction.\r\n", ch );
            return;
        }

        if ( !add_room( homedata, location, argument ) ) {
            send_to_char_color( "&C&wRoom couldn't be created. Please report to immortal.\r\n",
                                ch );
            return;
        }

        ch->gold -= ADDITIONAL_ROOM_COST;

        send_to_char_color( "&C&wRoom added.\r\n", ch );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        do_house( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg, "set" ) ) {
        char                    arg2[MAX_INPUT_LENGTH];
        char                    arg3[MAX_INPUT_LENGTH];
        bool                    apartment = FALSE;

        argument = one_argument( argument, arg2 );
        argument = one_argument( argument, arg3 );

        location = NULL;

        if ( !arg2 || arg2[0] == '\0' || !arg3 || arg3[0] == '\0'
             || ( !is_number( arg3 ) && str_cmp( arg3, "addroom" ) ) ) {
            send_to_char_color( "&C&wHouse set <character name> <vnum> (apartment)\r\n", ch );
            send_to_char_color
                ( "&C&wHouse set <character name> addroom <current house vnum> <direction>\r\n",
                  ch );
            return;
        }

        if ( ( victim = get_char_world( ch, arg2 ) ) == NULL ) {
            send_to_char_color( "&C&wThat player is not online.\r\n", ch );
            return;
        }

        if ( IS_NPC( victim ) ) {
            send_to_char_color( "&C&wNPC's can't get residences.\r\n", ch );
            return;
        }

        for ( homedata = first_home; homedata; homedata = homedata->next ) {
            if ( !str_cmp( homedata->name, victim->name ) )
                break;
        }

        if ( homedata && str_cmp( arg3, "addroom" ) ) {
            send_to_char_color( "&C&wThey already have a residence.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg3, "addroom" ) ) {
            bool                    found = FALSE;

            argument = one_argument( argument, arg3 );

            if ( !argument || argument[0] == '\0' || !arg3 || arg3[0] == '\0'
                 || !is_number( arg3 ) ) {
                send_to_char_color
                    ( "&C&wHouse set <character name> addroom <current house vnum> <direction>\r\n",
                      ch );
                return;
            }

            if ( homedata->apartment ) {
                send_to_char_color( "&C&wApartments may only have one room.\r\n", ch );
                return;
            }

            for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
                if ( homedata->vnum[i] == atoi( arg3 ) )
                    found = TRUE;
                if ( homedata->vnum[i] == 0 )
                    break;
            }

            if ( i == MAX_HOUSE_ROOMS ) {
                send_to_char_color( "&C&wThey have the max allowable rooms.\r\n", ch );
                return;
            }

            if ( !found ) {
                send_to_char_color( "&C&wThat vnum is not one of their house vnums.\r\n", ch );
                return;
            }

            if ( ( location = get_room_index( atoi( arg3 ) ) ) == NULL ) {
                send_to_char_color( "&C&wThat location doesn't exist.\r\n", ch );
                return;
            }

            if ( ( get_exit( location, get_dir( argument ) ) ) != NULL ) {
                send_to_char_color
                    ( "&C&wThere's already a room in that direction in that vnum.\r\n", ch );
                return;
            }

            if ( !add_room( homedata, location, argument ) ) {
                send_to_char_color( "&C&wError: Room couldn't be created.\r\n", ch );
                send_to_char_color
                    ( "&C&wPossibly room creation failure or all vnums of housing area full.\r\n",
                      ch );
                return;
            }

            send_to_char_color( "&C&wAdditional room added to their house.\r\n", ch );
            return;
        }

        if ( ( location = get_room_index( atoi( arg3 ) ) ) == NULL ) {
            send_to_char_color( "&C&wThat location doesn't exist.\r\n", ch );
            return;
        }

        if ( argument && argument[0] != '\0' && !str_cmp( argument, "apartment" ) )
            apartment = TRUE;

        if ( !set_house( victim, atoi( arg3 ), apartment ) ) {
            send_to_char_color( "&C&wThe residence couldn't be created.\r\n", ch );
            return;
        }

        send_to_char_color( "&C&wResidence created successfully.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "remove" ) ) {

        if ( !argument || argument[0] == '\0' ) {
            send_to_char_color( "&C&wHouse remove <character name>\r\n", ch );
            return;
        }

        if ( ( victim = get_char_world( ch, argument ) ) == NULL ) {
            send_to_char_color( "&C&wThat player is not online.\r\n", ch );
            return;
        }

        if ( IS_NPC( victim ) ) {
            send_to_char_color( "&C&wNPC's don't have residences to begin with.\r\n", ch );
            return;
        }

        for ( homedata = first_home; homedata; homedata = homedata->next ) {
            if ( !str_cmp( homedata->name, victim->name ) )
                break;
        }

        if ( !homedata ) {
            send_to_char_color( "&C&wThey don't have a house.\r\n", ch );
            return;
        }

        if ( !remove_house( victim ) ) {
            send_to_char_color( "&C&wThe residence couldn't be removed.\r\n", ch );
            return;
        }

        send_to_char_color( "&C&wResidence removed.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "givekey" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            send_to_char_color( "&C&wHouse givekey <character name>\r\n", ch );
            return;
        }

        if ( ( victim = get_char_world( ch, argument ) ) == NULL ) {
            send_to_char_color( "&C&wThat player is not online.\r\n", ch );
            return;
        }

        if ( IS_NPC( victim ) ) {
            send_to_char_color( "&C&wNPC's don't have residences to begin with.\r\n", ch );
            return;
        }

        for ( homedata = first_home; homedata; homedata = homedata->next ) {
            if ( !str_cmp( homedata->name, victim->name ) )
                break;
        }

        if ( !homedata ) {
            send_to_char_color( "&C&wThey don't have a house.\r\n", ch );
            return;
        }

        if ( homedata->apartment ) {
            send_to_char_color( "&C&wApartments don't have keys.\r\n", ch );
            return;
        }

        if ( !give_key( ch, homedata->vnum[0] ) ) {
            send_to_char_color( "&C&wKey wasn't able to be given.\r\n", ch );
            return;
        }

        send_to_char_color( "&C&wKey given successfully.\r\n", ch );
        return;
    }

    do_house( ch, ( char * ) "" );
    return;
}

void do_gohome( CHAR_DATA *ch, char *argument )
{
    HOME_DATA              *homedata = NULL;
    char                    buf[MAX_STRING_LENGTH];

    if ( !ch->desc )
        return;

    for ( homedata = first_home; homedata; homedata = homedata->next ) {
        if ( !str_cmp( homedata->name, ch->name ) )
            break;
    }

    if ( !homedata ) {
        send_to_char( "You don\'t have a home.\r\n", ch );
        return;
    }
#ifdef EXTENDED_ROOMS
    if ( xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) )
#else
    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) )
#endif
    {
        send_to_char_color( "&C&wYou can\'t gohome from here.\r\n", ch );
        return;
    }

    switch ( ch->position ) {
        default:
            break;
        case POS_MORTAL:
        case POS_INCAP:
        case POS_DEAD:
        case POS_STUNNED:
            send_to_char_color( "&C&wYou can\'t do that, you\'re hurt too badly.\r\n", ch );
            return;
        case POS_SITTING:
        case POS_RESTING:
        case POS_SLEEPING:
            do_wake( ch, ( char * ) "auto" );
            return;
        case POS_FIGHTING:
        case POS_DEFENSIVE:
        case POS_AGGRESSIVE:
        case POS_BERSERK:
        case POS_EVASIVE:
            send_to_char_color( "&C&wNo way!  You are still fighting!\r\n", ch );
            return;
    }

    sprintf( buf, "%s disappears to their private sanctuary.", ch->name );
    act( AT_GREY, buf, ch, NULL, ch, TO_ROOM );

    char_from_room( ch );
    char_to_room( ch, get_room_index( homedata->vnum[0] ) );

    sprintf( buf,
             "As a vortex of luminescent light forms, %s slides gracefully through into their private sanctuary.",
             ch->name );
    act( AT_GREY, buf, ch, NULL, ch, TO_ROOM );

    do_look( ch, ( char * ) "" );

    return;
}

void do_residence( CHAR_DATA *ch, char *argument )
{
    HOME_DATA              *tmphome = NULL;
    ROOM_INDEX_DATA        *location = NULL;
    char                    area[MAX_STRING_LENGTH];
    short                   i,
                            y,
                            z = 0;

    if ( !first_home ) {
        send_to_char_color( "&C&wThere are no residences currently.\r\n", ch );
        return;
    }

    y = 1;

    send_to_char_color( "&C&W             Current Residences\r\n", ch );

    if ( !IS_IMMORTAL( ch ) ) {
        send_to_char_color( "&C&R### &W| &RPlayer Name &W| &RHousing Area &W| &R# of Rooms &W|\r\n",
                            ch );
        send_to_char_color( "&C&W-----------------------------------------------\r\n", ch );
    }
    else {
        send_to_char_color
            ( "&C&R### &W| &RPlayer Name &W| &RHousing Area &W| &R# of Rooms &W| &RStart Vnum &W|\r\n",
              ch );
        send_to_char_color( "&C&W------------------------------------------------------------\r\n",
                            ch );
    }

    for ( tmphome = first_home; tmphome; tmphome = tmphome->next ) {
        if ( ( location = get_room_index( tmphome->vnum[0] ) ) == NULL )
            strcpy( area, "None" );
        else
            strcpy( area, location->area->name );

        area[12] = '\0';

        for ( i = 0; i < MAX_HOUSE_ROOMS; i++ )
            if ( tmphome->vnum[i] > 0 )
                z++;

        if ( !IS_IMMORTAL( ch ) )
            ch_printf_color( ch, "&C&c%3d &z| &c%11s &z| &c%12s &z| &c%10d &z|\r\n", y,
                             tmphome->name, area, z );
        else
            ch_printf_color( ch, "&C&c%3d &z| &c%11s &z| &c%12s &z| &c%10d &z| &c%10d &z|\r\n", y,
                             tmphome->name, area, z, tmphome->vnum[0] );

        y++;
        z = 0;
    }

    if ( !IS_IMMORTAL( ch ) )
        send_to_pager_color( "&C&W-----------------------------------------------\r\n", ch );
    else
        send_to_pager_color( "&C&W------------------------------------------------------------\r\n",
                             ch );

    return;
}

void do_accessories( CHAR_DATA *ch, char *argument )
{
    ACCESSORIES_DATA       *tmpacc = NULL;
    HOME_DATA              *homedata = NULL;
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    short                   i = 0;

    if ( IS_NPC( ch ) ) {
        send_to_char_color( "&C&wMobs don't have homes to begin with.\r\n", ch );
        return;
    }

    for ( homedata = first_home; homedata; homedata = homedata->next ) {
        if ( !str_cmp( homedata->name, ch->name ) )
            break;
    }

    if ( !homedata && !IS_IMMORTAL( ch ) ) {
        send_to_char_color( "&C&wYou need a house to get accessories for it.\r\n", ch );
        return;
    }

    if ( homedata && homedata->apartment && !IS_IMMORTAL( ch ) ) {
        send_to_char_color( "&C&wApartments can't be furnished.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( !arg || arg[0] == '\0' || ( is_number( arg ) && arg2[0] == '\0' ) ) {
        send_to_char_color( "&C&gSyntax:\r\n", ch );
        send_to_char_color( "&C&w accessories <argument>\r\n", ch );
        send_to_char_color( "&C&gWhere the argument is one of:\r\n", ch );
        send_to_char_color( "&C&w list\r\n", ch );
        send_to_char_color( "&C&w <# of accessory> show\r\n", ch );
        send_to_char_color( "&C&w <# of accessory> buy\r\n", ch );

        if ( IS_IMMORTAL( ch ) ) {
            send_to_char_color( "&C&gImmortal Arguments:\r\n", ch );
#ifndef HOUSE_MOBS
            send_to_char_color( "&C&w add <accessory vnum>\r\n", ch );
#else
            send_to_char_color( "&C&w add <accessory vnum> (mob)\r\n", ch );
#endif
            send_to_char_color( "&C&w <# of accessory> remove\r\n", ch );
            send_to_char_color( "&C&w <# of accessory> setprice <amount>\r\n", ch );
        }

        return;
    }

    if ( !is_number( arg ) ) {
        if ( !str_cmp( arg, "list" ) ) {
            OBJ_INDEX_DATA         *obj = NULL;
            MOB_INDEX_DATA         *mob = NULL;
            char                   *name;
            char                    tmparg[MAX_INPUT_LENGTH];

            if ( !first_accessory ) {
                send_to_char_color( "&C&wThere are no accessories currently.\r\n", ch );
                return;
            }

            i = 1;

            send_to_char_color( "&C&c            House Accessories\r\n", ch );
            send_to_char_color( "&C&g## &w|       &gAccessory Name       &w|    &gPrice    &w|\r\n",
                                ch );
            send_to_char_color( "&C&w-----------------------------------------------\r\n", ch );

            for ( tmpacc = first_accessory; tmpacc; tmpacc = tmpacc->next ) {
                if ( !tmpacc->mob ) {
                    if ( ( obj = get_obj_index( tmpacc->vnum ) ) == NULL ) {
                        sprintf( tmparg,
                                 "Accessories list: object vnum %d doesn't exist for accessory %d.",
                                 tmpacc->vnum, i );
                        bug( tmparg, 0 );
                        ch_printf_color( ch,
                                         "&C&g%2d &W| &RAccessory object could not be found.\r\n",
                                         i );
                        i++;
                        continue;
                    }

                    name = STRALLOC( obj->short_descr );
                }
                else {
                    if ( ( mob = get_mob_index( tmpacc->vnum ) ) == NULL ) {
                        sprintf( tmparg,
                                 "Accessories list: mob vnum %d doesn't exist for accessory %d.",
                                 tmpacc->vnum, i );
                        bug( tmparg, 0 );
                        ch_printf_color( ch, "&C&g%2d &W| &RAccessory mob could not be found.\r\n",
                                         i );
                        i++;
                        continue;
                    }

                    name = STRALLOC( mob->short_descr );
                }

                one_argument( name, tmparg );
                if ( !str_cmp( tmparg, "a" ) || !str_cmp( tmparg, "an" ) )
                    name = one_argument( name, tmparg );
                name[24] = '\0';

                ch_printf_color( ch, "&C&g%2d &W| &g%26s &W| &g%11d &W|\r\n", i, name,
                                 tmpacc->price );
                i++;
            }

            send_to_char_color( "&C&w-----------------------------------------------\r\n", ch );

            return;
        }

        if ( !IS_IMMORTAL( ch ) ) {
            do_accessories( ch, ( char * ) "" );
            return;
        }

        if ( !str_cmp( arg, "add" ) ) {
            OBJ_INDEX_DATA         *obj = NULL;
            MOB_INDEX_DATA         *mob = NULL;
            ACCESSORIES_DATA       *newacc = NULL;

            if ( !arg2 || arg2[0] == '\0' || !is_number( arg2 ) ) {
#ifndef HOUSE_MOBS
                send_to_char_color( "&C&wAccessories add <accessory vnum>\r\n", ch );
#else
                send_to_char_color( "&C&wAccessories add <accessory vnum> (mob)\r\n", ch );
#endif
                return;
            }

#ifdef HOUSE_MOBS
            if ( argument && argument[0] != '\0' && !str_cmp( argument, "mob" ) ) {
                if ( ( mob = get_mob_index( atoi( arg2 ) ) ) == NULL ) {
                    send_to_char_color( "&C&wThat mob doesn't exist.\r\n", ch );
                    return;
                }
            }
#endif

            if ( !mob && ( obj = get_obj_index( atoi( arg2 ) ) ) == NULL ) {
                send_to_char_color( "&C&wThat object doesn't exist.\r\n", ch );
                return;
            }

            for ( tmpacc = first_accessory; tmpacc; tmpacc = tmpacc->next ) {
                if ( tmpacc->vnum == atoi( arg2 ) ) {
                    if ( mob && newacc->mob ) {
                        send_to_char_color( "&C&wThat mob is already on accessories.\r\n", ch );
                        return;
                    }
                    else if ( !mob && !newacc->mob ) {
                        send_to_char_color( "&C&wThat object is already on accessories.\r\n", ch );
                        return;
                    }

                    continue;
                }
            }

            CREATE( newacc, ACCESSORIES_DATA, 1 );
            LINK( newacc, first_accessory, last_accessory, next, prev );

            newacc->vnum = atoi( arg2 );

            if ( !mob ) {
                newacc->price = obj->cost;
                newacc->mob = FALSE;
            }
            else {
#ifdef HOUSE_MOBS
                newacc->price = DEFAULT_MOB_PRICE;
                newacc->mob = TRUE;
#endif
            }

            save_accessories(  );

            send_to_char_color( "&C&wAccessory added.\r\n", ch );

            return;
        }

        do_accessories( ch, ( char * ) "" );
        return;
    }

    for ( tmpacc = first_accessory; tmpacc; tmpacc = tmpacc->next )
        if ( ++i == atoi( arg ) )
            break;

    if ( !tmpacc ) {
        send_to_char_color( "&C&wThat isn't a valid accessory.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "show" ) ) {
        OBJ_INDEX_DATA         *obj = NULL;
        MOB_INDEX_DATA         *mob = NULL;

        if ( !tmpacc->mob ) {
            if ( ( obj = get_obj_index( tmpacc->vnum ) ) == NULL ) {
                send_to_char_color( "&C&wError: Accessory doesn't exist.\r\n", ch );
                return;
            }

            send_to_char_color( "&C&cObject Accessory Information\r\n", ch );
            ch_printf_color( ch, "&C&gName: &w%s\r\n", obj->short_descr );
            ch_printf_color( ch, "&C&gType: &w%s\r\n", o_types[obj->item_type] );
            ch_printf_color( ch, "&C&gFlags: &w%s\r\n",
                             ext_flag_string( &obj->extra_flags, o_flags ) );
            ch_printf_color( ch, "&C&gWeight: &w%d\r\n", obj->weight );

            return;
        }
        else {
            if ( ( mob = get_mob_index( tmpacc->vnum ) ) == NULL ) {
                send_to_char_color( "&C&wError: Accessory doesn't exist.\r\n", ch );
                return;
            }

            send_to_char_color( "&C&cMob Accessory Information\r\n", ch );
            ch_printf_color( ch, "&C&gName: &w%s\r\n", mob->short_descr );
            ch_printf_color( ch, "&C&gRace: &w%s\r\n",
                             ( mob->race < MAX_NPC_RACE
                               && mob->race >= 0 ? npc_race[mob->race] : "unknown" ) );
            ch_printf_color( ch, "&C&gClass: &w%s\r\n",
                             ( mob->Class < MAX_NPC_CLASS
                               && mob->Class >= 0 ? npc_class[mob->Class] : "unknown" ) );
            ch_printf_color( ch, "&C&gAverage Hp: &w%d\r\n",
                             ( !mob->
                               hitnodice ? ( mob->level * 8 +
                                             number_range( mob->level * mob->level / 4,
                                                           mob->level *
                                                           mob->level ) ) : ( mob->hitnodice *
                                                                              number_range( 1,
                                                                                            mob->
                                                                                            hitsizedice )
                                                                              + mob->hitplus ) ) );
            ch_printf_color( ch, "&C&gAverage AC: &w%d\r\n",
                             ( mob->ac ? mob->ac : interpolate( mob->level, 100, -100 ) ) );
            ch_printf_color( ch, "&C&gNumber of Attacks(Per Round): &w%d\r\n", mob->numattacks );

            return;
        }
    }

    if ( !str_cmp( arg2, "buy" ) ) {
        ROOM_INDEX_DATA        *location = NULL;
        OBJ_INDEX_DATA         *objindex = NULL;
        OBJ_DATA               *obj = NULL;
        MOB_INDEX_DATA         *mobindex = NULL;

#ifdef HOUSE_MOBS
        CHAR_DATA              *mob = NULL;
#endif

        if ( ch->gold < tmpacc->price ) {
            send_to_char_color( "&C&wYou don't have enough to buy that accessory.\r\n", ch );
            return;
        }

        if ( !homedata || homedata->apartment ) {
            send_to_char_color( "&C&wYou need a house first.\r\n", ch );
            return;
        }

        for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
            if ( ch->in_room->vnum == homedata->vnum[i] ) {
                location = ch->in_room;
                send_to_char_color( "&C&wPlacing accessory in the room you are currently in.\r\n",
                                    ch );
                break;
            }
        }

        if ( !location ) {
            send_to_char_color
                ( "&C&wYou aren't currently in your house. Placing accessory in your first house room.\r\n",
                  ch );

            if ( ( location = get_room_index( homedata->vnum[0] ) ) == NULL ) {
                send_to_char_color
                    ( "&C&wError: House start room could not be found. Aborting accessory buying.\r\n",
                      ch );
                return;
            }
        }

        if ( ( tmpacc->mob && ( mobindex = get_mob_index( tmpacc->vnum ) ) == NULL )
             || ( !tmpacc->mob && ( objindex = get_obj_index( tmpacc->vnum ) ) == NULL ) ) {
            send_to_char_color( "&C&wError: Accessory doesn't exist. Aborting.\r\n", ch );
            return;
        }

#ifdef HOUSE_MOBS
        if ( mobindex ) {

            mob = create_mobile( mobindex );
            char_to_room( mob, location );
            add_reset( location, 'M', 1, mobindex->vnum, mobindex->count, location->vnum );
        }
#endif

        if ( objindex ) {
            obj = create_object( objindex, objindex->level );
            obj_to_room( obj, location );
        }

        ch->gold -= ADDITIONAL_ROOM_COST;
        fwrite_house( homedata );
        send_to_char_color( "&C&wAccessory purchased.\r\n", ch );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        do_accessories( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "remove" ) ) {
        UNLINK( tmpacc, first_accessory, last_accessory, next, prev );

        tmpacc->vnum = 0;
        tmpacc->price = 0;
        tmpacc->mob = FALSE;

        DISPOSE( tmpacc );
        save_accessories(  );

        send_to_char_color( "&C&wAccessory removed.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "setprice" ) ) {
        if ( !argument || argument[0] == '\0' || !is_number( argument ) ) {
            send_to_char_color( "&C&wAccessories <# of accessory> setprice <amount>\r\n", ch );
            return;
        }

        tmpacc->price = atoi( argument );
        save_accessories(  );

        send_to_char_color( "&C&wAccessory price adjusted.\r\n", ch );
        return;
    }

    do_accessories( ch, ( char * ) "" );
    return;
}

void do_homebuy( CHAR_DATA *ch, char *argument )
{
    HOMEBUY_DATA           *tmpres = NULL;
    HOME_DATA              *tmphome = NULL;
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];
    short                   i = 0;

    if ( IS_NPC( ch ) ) {
        send_to_char_color( "&C&wMobs can't buy residences.\r\n", ch );
        return;
    }

    for ( tmphome = first_home; tmphome; tmphome = tmphome->next ) {
        if ( !str_cmp( tmphome->name, ch->name ) && !IS_IMMORTAL( ch ) ) {
            send_to_char_color( "&C&wYou already have a residence.\r\n", ch );
            return;
        }
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( !arg || arg[0] == '\0' || ( is_number( arg ) && arg2[0] == '\0' ) ) {
        send_to_char_color( "&C&CSyntax:\r\n", ch );
        send_to_char_color( "&C&c homebuy <argument>\r\n", ch );
        send_to_char_color( "&C&CWhere the argument is one of:\r\n", ch );
        send_to_char_color( "&C&c list\r\n", ch );
        send_to_char_color( "&C&c <# of auction item> show\r\n", ch );
        send_to_char_color( "&C&c <# of auction item> bid <amount>\r\n", ch );

        if ( IS_IMMORTAL( ch ) ) {
            send_to_char_color( "&C&CImmortal Arguments:\r\n", ch );
            send_to_char_color( "&C&c add <residence vnum> (apartment)\r\n", ch );
            send_to_char_color( "&C&c <# of auction item> remove\r\n", ch );
            send_to_char_color( "&C&c <# of auction item> setbid <amount>\r\n", ch );
            send_to_char_color( "&C&c <# of auction item> bidincrement <percent>\r\n", ch );
            send_to_char_color( "&C&c <# of auction item> timeremainder <days> (<hours>)\r\n", ch );
            send_to_char_color( "&C&c <# of auction item> clearbidder\r\n", ch );

        }

        return;
    }

    if ( !is_number( arg ) ) {
        if ( !str_cmp( arg, "list" ) ) {
            ROOM_INDEX_DATA        *location = NULL;
            short                   apt,
                                    hou = 0;
            int                     bidinc = 0;
            int                     days = 0;
            char                    name[256];
            char                    area[256];

            if ( !first_homebuy ) {
                send_to_char_color( "&C&wThere are no residences currently on auction.\r\n", ch );
                return;
            }

            send_to_char_color( "&C&W                 Residence Auction\r\n", ch );
            send_to_char_color
                ( "&C&c## &C| &cResidence Name &C| &cHousing Area &C| &cCurr. Bid &C| &cBid Inc. &C| &cRemaining Time &C|\r\n",
                  ch );
            send_to_char_color
                ( "&C&C----------------------------------------------------------------------------\r\n",
                  ch );

            apt = 1;
            hou = 1;
            i = 1;

            for ( tmpres = first_homebuy; tmpres; tmpres = tmpres->next ) {
                if ( ( location = get_room_index( tmpres->vnum ) ) == NULL ) {
                    sprintf( buf, "Homebuy List: Item #%d's location doesn't exist.", i );
                    bug( buf, 0 );
                    ch_printf_color( ch, "&C&c%2d &C| &cResidence could not be found.\r\n", i );
                    i++;
                    continue;
                }

                strcpy( area, location->area->name );
                area[12] = '\0';

                if ( tmpres->apartment ) {
                    sprintf( name, "Apartment %d", apt );
                    apt++;
                }
                else {
                    sprintf( name, "House %d", hou );
                    hou++;
                }

                bidinc = ( tmpres->incpercent * tmpres->bid ) / 100;
                days = tmpres->endtime / 48;

                ch_printf_color( ch,
                                 "&C&c%2d &C| &c%14s &C| &c%12s &C| &c%9d &C| &c%8d &C| &c%2d &CDays &c%2d &CHrs &C|\r\n",
                                 i, name, area, tmpres->bid, bidinc, days,
                                 ( ( tmpres->endtime - ( days * 48 ) ) / 2 ) );

                i++;
            }

            send_to_char_color
                ( "&C&C----------------------------------------------------------------------------\r\n",
                  ch );

            return;
        }

        if ( !IS_IMMORTAL( ch ) ) {
            do_homebuy( ch, ( char * ) "" );
            return;
        }

        if ( !str_cmp( arg, "add" ) ) {
            bool                    apartment = FALSE;

            if ( !arg2 || arg2[0] == '\0' || !is_number( arg2 ) ) {
                send_to_char_color( "&C&wHomebuy add <residence vnum> (apartment)\r\n", ch );
                return;
            }

            if ( argument[0] != '\0' && !str_cmp( argument, "apartment" ) )
                apartment = TRUE;

            if ( ( get_room_index( atoi( arg2 ) ) ) == NULL ) {
                send_to_char_color( "&C&wThat location doesn't exist.\r\n", ch );
                return;
            }

            for ( tmpres = first_homebuy; tmpres; tmpres = tmpres->next ) {
                if ( tmpres->vnum == atoi( arg2 ) ) {
                    send_to_char_color( "&C&wThat vnum is already on auction.\r\n", ch );
                    return;
                }
            }

            if ( !add_homebuy( ch, atoi( arg2 ), apartment, 0 ) ) {
                send_to_char_color( "&C&wResidence couldn't be added to the auction.\r\n", ch );
                return;
            }

            send_to_char_color( "&C&wResidence added to the auction successfully.\r\n", ch );
            return;
        }

        do_homebuy( ch, ( char * ) "" );
        return;
    }

    for ( tmpres = first_homebuy; tmpres; tmpres = tmpres->next )
        if ( ++i == atoi( arg ) )
            break;

    if ( !tmpres ) {
        send_to_char_color( "&C&wThat isn't a residence on auction.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "show" ) ) {
        ROOM_INDEX_DATA        *location = NULL;
        int                     bidinc = 0;
        int                     days = 0;

        if ( ( location = get_room_index( tmpres->vnum ) ) == NULL ) {
            send_to_char_color( "&C&wResidence doesn't exist.\r\n", ch );
            sprintf( buf, "Homebuy show: Residence %d has a nonexistant location", i );
            bug( buf, 0 );
            return;
        }

        bidinc = ( tmpres->incpercent * tmpres->bid ) / 100;
        days = tmpres->endtime / 48;
        send_to_char_color( "&C&WHome Auction Residence Information\r\n", ch );
        ch_printf_color( ch, "&C&CName: &c%s\r\n", location->name );
        ch_printf_color( ch, "&C&CType: &c%s\r\n", ( tmpres->apartment ? "Apartment" : "House" ) );
        ch_printf_color( ch, "&C&CHousing Area: &c%s\r\n", location->area->name );
        ch_printf_color( ch, "&C&CCurrent Bid: &c%d\r\n", tmpres->bid );
        ch_printf_color( ch, "&C&CBid Increment: &c%d\r\n", bidinc );
        ch_printf_color( ch, "&C&CTime Left: &c%d &CDays &c%d &CHours\r\n", days,
                         ( ( tmpres->endtime - ( days * 48 ) ) / 2 ) );
        ch_printf_color( ch, "&C&CSeller: &c%s\r\n", tmpres->seller );
        ch_printf_color( ch, "&C&CBidder: &c%s\r\n", tmpres->bidder );

        if ( IS_IMMORTAL( ch ) )
            ch_printf_color( ch, "&C&CVnum: &c%d\r\n", tmpres->vnum );

        return;
    }

    if ( !str_cmp( arg2, "bid" ) ) {
        HOMEBUY_DATA           *checkhome = NULL;
        int                     bidinc = ( tmpres->incpercent * tmpres->bid ) / 100;

        if ( !argument || argument[0] == '\0' || !is_number( argument ) ) {
            send_to_char_color( "&C&wHomebuy <# of auction item> bid <amount>\r\n", ch );
            return;
        }

        for ( tmphome = first_home; tmphome; tmphome = tmphome->next ) {
            if ( !str_cmp( tmphome->name, ch->name ) ) {
                send_to_char_color( "&C&wYou already have a residence.\r\n", ch );
                return;
            }
        }

        for ( checkhome = first_homebuy; checkhome; checkhome = checkhome->next ) {
            if ( !str_cmp( checkhome->bidder, ch->name ) ) {
                send_to_char_color( "&C&wYou are already bidding on a residence.\r\n", ch );
                return;
            }
        }

        if ( !str_cmp( tmpres->seller, ch->name ) ) {
            send_to_char_color( "&C&wThe seller can't bid on their own item.\r\n", ch );
            return;
        }

        if ( atoi( argument ) < ( tmpres->bid + bidinc ) ) {
            send_to_char_color
                ( "&C&wYou must bid at least the current bid plus the bid increment.\r\n", ch );
            return;
        }

        if ( ch->gold < atoi( argument ) ) {
            send_to_char_color( "&C&wYou don't have that much.\r\n", ch );
            return;
        }

        STRFREE( tmpres->bidder );
        tmpres->bidder = STRALLOC( ch->name );

        tmpres->bid = atoi( argument );

        save_homebuy(  );

        send_to_char_color( "&C&wYou have successfully bid on that residence.\r\n", ch );

        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        do_homebuy( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "setbid" ) ) {
        if ( !argument || argument[0] == '\0' || !is_number( argument ) ) {
            send_to_char_color( "&C&wHomebuy <# of auction item> setprice <amount>\r\n", ch );
            return;
        }

        tmpres->bid = atoi( argument );
        save_homebuy(  );

        send_to_char_color( "&C&wBid for residence set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "clearbidder" ) ) {
        if ( !str_cmp( tmpres->bidder, "None" ) ) {
            send_to_char_color( "&C&wNo bidder to clear.\r\n", ch );
            return;
        }

        STRFREE( tmpres->bidder );
        tmpres->bidder = STRALLOC( "None" );
        save_homebuy(  );

        send_to_char_color( "&C&wBidder for residence cleared.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "bidincrement" ) ) {
        if ( !argument || argument[0] == '\0' || !is_number( argument ) ) {
            send_to_char_color( "&C&wHomebuy <# of auction item> bidincrement <percent>\r\n", ch );
            return;
        }

        tmpres->incpercent = atoi( argument );
        save_homebuy(  );

        send_to_char_color( "&C&wBid increment percentage set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "timeremainder" ) ) {
        char                    arg3[MAX_INPUT_LENGTH];
        int                     hours = 0;

        argument = one_argument( argument, arg3 );

        if ( !arg3 || arg3[0] == '\0' || !is_number( arg3 ) ) {
            send_to_char_color
                ( "&C&wHomebuy <# of auction item> timeremainder <days> (<hours>)\r\n", ch );
            return;
        }

        if ( atoi( arg3 ) >= 100 || atoi( arg3 ) <= -1 ) {
            send_to_char_color( "&C&wDays can be from 0 to 99.\r\n", ch );
            return;
        }

        if ( argument[0] != '\0' && is_number( argument ) ) {
            if ( atoi( argument ) <= -1 || atoi( argument ) >= 24 ) {
                send_to_char_color( "&C&wHours can be from 0 to 23.\r\n", ch );
                return;
            }

            hours = atoi( argument );
        }

        tmpres->endtime = ( atoi( arg3 ) * 48 ) + ( hours * 2 );

        save_homebuy(  );

        send_to_char_color( "&C&wTime remainder set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "remove" ) ) {
        if ( !remove_homebuy( tmpres ) ) {
            send_to_char_color( "&C&wResidence could not be removed from auction successfully.\r\n",
                                ch );
            return;
        }

        send_to_char_color( "&C&wResidence removed from auction successfully.\r\n", ch );
        return;
    }

    do_homebuy( ch, ( char * ) "" );

    return;
}

void do_sellhouse( CHAR_DATA *ch, char *argument )
{
    HOME_DATA              *tmphome = NULL;

    if ( IS_NPC( ch ) ) {
        send_to_char_color( "&C&wMobs don't have residences to begin with.\r\n", ch );
        return;
    }

    for ( tmphome = first_home; tmphome; tmphome = tmphome->next )
        if ( !str_cmp( tmphome->name, ch->name ) )
            break;

    if ( !tmphome ) {
        send_to_char_color( "&C&wYou need a residence if your going to sell it.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char_color( "&C&wOptions for Sellhouse Command:\r\n", ch );
        send_to_char_color( "&C&R  Sellhouse yes\r\n", ch );
        send_to_char_color( "&C&w Places your residence on auction with default starting bid.\r\n",
                            ch );
        send_to_char_color( "&C&R  Sellhouse <amount>\r\n", ch );
        send_to_char_color
            ( "&C&w Places your residence on auction with the given amount as starting bid.\r\n",
              ch );
        return;
    }

    if ( is_number( argument ) ) {
        if ( ( tmphome->apartment && atoi( argument ) < MIN_APARTMENT_BID )
             || ( !tmphome->apartment && atoi( argument ) < MIN_HOUSE_BID ) ) {
            send_to_char_color
                ( "&C&wAmount has to be equal to or greater than the minimum bid for your residence type.\r\n",
                  ch );
            return;
        }

        if ( !add_homebuy( ch, tmphome->vnum[0], tmphome->apartment, atoi( argument ) ) ) {
            send_to_char_color
                ( "&C&wResidence couldn't be added to the housing auction successfully. Sell has been aborted.\r\n",
                  ch );
            return;
        }

        send_to_char_color
            ( "&C&wYou have successfully placed your residence on auction for the \r\ngiven starting bid.\r\n"
              "Profit from the sell should occur when your house has been bought on auction.\r\n"
              "Loss of your residence should also occur at that time.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "yes" ) ) {
        if ( !add_homebuy( ch, tmphome->vnum[0], tmphome->apartment, 0 ) ) {
            send_to_char_color
                ( "&C&wResidence couldn't be added to the housing auction successfully. Sell has been aborted.\r\n",
                  ch );
            return;
        }

        send_to_char_color
            ( "&C&wYou have successfully placed your residence on auction for the \r\ndefault minimum bid for your residence type.\r\n"
              "Profit from the sell should occur when your house has been bought on auction.\r\n"
              "Loss of your residence should also occur at that time.\r\n", ch );
        return;
    }

    do_sellhouse( ch, ( char * ) "" );
}

void save_residence( ROOM_INDEX_DATA *location )
{
    AREA_DATA              *tarea = NULL;
    char                    filename[256];

    if ( !location ) {
        bug( "Save_residence: NULL Location" );
        return;
    }

    for ( tarea = first_area; tarea; tarea = tarea->next ) {
        if ( tarea == location->area ) {
            fold_area( location->area, location->area->filename, FALSE );
            return;
        }
    }

    for ( tarea = first_build; tarea; tarea = tarea->next ) {
        if ( tarea == location->area ) {
            sprintf( filename, "%s%s", BUILD_DIR, location->area->filename );
            fold_area( location->area, filename, FALSE );
            return;
        }
    }

    bug( "Save_residence: Location doesn't have an area." );
    return;
}

bool set_house( CHAR_DATA *ch, int vnum, bool apartment )
{
    HOME_DATA              *tmphome,
                           *shome = NULL;
    char                    buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA        *location = NULL;
    EXIT_DATA              *pexit = NULL;
    OBJ_INDEX_DATA         *obj,
                           *key = NULL;

    if ( !ch || !ch->name || vnum <= 0 )
        return FALSE;

    CREATE( tmphome, HOME_DATA, 1 );

    tmphome->name = STRALLOC( ch->name );
    tmphome->apartment = apartment;
    tmphome->vnum[0] = vnum;

    if ( first_home ) {
        bool                    found = FALSE;

        for ( shome = first_home; shome; shome = shome->next ) {
            if ( strcmp( tmphome->name, shome->name ) < 0 ) {
                INSERT( tmphome, shome, first_home, next, prev );
                found = TRUE;
                break;
            }
        }

        if ( !found )
            LINK( tmphome, first_home, last_home, next, prev );
    }
    else {
        LINK( tmphome, first_home, last_home, next, prev );
    }

    update_house_list(  );
    fwrite_house( tmphome );

    if ( ( location = get_room_index( vnum ) ) == NULL )
        return FALSE;

    if ( apartment )
        sprintf( buf, "%s's Apartment", tmphome->name );
    else
        sprintf( buf, "%s's House", tmphome->name );

    STRFREE( location->name );
    location->name = STRALLOC( buf );
    STRFREE( location->description );
    if ( apartment )
        location->description =
            STRALLOC
            ( "A room surrounds you, filled with the dirt and filth of previous tenants.\r\nA spray painted frame of the last tenant can be seen on the floor in a \r\nfar corner.\r\n" );
    else
        location->description =
            STRALLOC( "This is your desc. You can edit this with HOUSE DESC.\r\n" );

    location->sector_type = 0;

#ifndef EXTENDED_ROOMS
    SET_BIT( location->room_flags, ROOM_NO_SUMMON );
    SET_BIT( location->room_flags, ROOM_NO_ASTRAL );
    SET_BIT( location->room_flags, ROOM_INDOORS );
    SET_BIT( location->room_flags, ROOM_SAFE );
#else
    xSET_BIT( location->room_flags, ROOM_NO_SUMMON );
    xSET_BIT( location->room_flags, ROOM_NO_ASTRAL );
    xSET_BIT( location->room_flags, ROOM_INDOORS );
    xSET_BIT( location->room_flags, ROOM_SAFE );
#endif

    for ( pexit = location->first_exit; pexit; pexit = pexit->next ) {
        SET_BIT( pexit->exit_info, EX_ISDOOR );
        SET_BIT( pexit->exit_info, EX_CLOSED );
        SET_BIT( pexit->exit_info, EX_LOCKED );
        SET_BIT( pexit->exit_info, EX_NOPASSDOOR );
        SET_BIT( pexit->exit_info, EX_PICKPROOF );
        SET_BIT( pexit->exit_info, EX_BASHPROOF );
        SET_BIT( pexit->exit_info, EX_NOMOB );

        if ( !apartment )
            pexit->key = location->vnum;

        if ( pexit->rexit ) {
            SET_BIT( pexit->rexit->exit_info, EX_ISDOOR );
            SET_BIT( pexit->rexit->exit_info, EX_CLOSED );
            SET_BIT( pexit->rexit->exit_info, EX_LOCKED );
            SET_BIT( pexit->rexit->exit_info, EX_NOPASSDOOR );
            SET_BIT( pexit->rexit->exit_info, EX_PICKPROOF );
            SET_BIT( pexit->rexit->exit_info, EX_BASHPROOF );
            SET_BIT( pexit->rexit->exit_info, EX_NOMOB );

            if ( !apartment )
                pexit->rexit->key = location->vnum;
        }

    }

    if ( apartment )
        return TRUE;

    if ( ( obj = get_obj_index( location->vnum ) ) != NULL ) {
        delete_obj( obj );
        save_residence( location );
    }

    sprintf( buf, "%s's House Key", tmphome->name );
    key = make_object( location->vnum, 0, buf );
    key->value[0] = location->vnum;
    key->item_type = ITEM_TREASURE;
    key->level = 1;
    STRFREE( key->short_descr );
    key->short_descr = STRALLOC( buf );
    SET_BIT( key->wear_flags, ITEM_TAKE );
    SET_BIT( key->wear_flags, ITEM_HOLD );

    xREMOVE_BIT( key->extra_flags, ITEM_PROTOTYPE );

    save_residence( location );

    return TRUE;
}

bool remove_house( CHAR_DATA *ch )
{
    HOME_DATA              *homedata = NULL;
    ROOM_INDEX_DATA        *location,
                           *addloc = NULL;
    OBJ_INDEX_DATA         *key = NULL;
    OBJ_DATA               *obj = NULL;
    CHAR_DATA              *mob,
                           *mob_next = NULL;
    EXIT_DATA              *pexit = NULL;
    char                    filename[256];
    short                   i = 0;
    RESET_DATA             *mreset,
                           *mreset_next = NULL;

    for ( homedata = first_home; homedata; homedata = homedata->next ) {
        if ( !str_cmp( homedata->name, ch->name ) )
            break;
    }

    if ( !homedata )
        return FALSE;

    if ( ( location = get_room_index( homedata->vnum[0] ) ) == NULL )
        return FALSE;

    while ( ( obj = location->first_content ) != NULL )
        extract_obj( obj );

    for ( mob = location->first_person; mob; mob = mob_next ) {
        mob_next = mob->next;

        if ( IS_NPC( mob ) )
            extract_char( mob, TRUE );

    }

    sprintf( filename, "%s%s", HOUSE_DIR, capitalize( homedata->name ) );

    remove( filename );

    STRFREE( homedata->name );

    if ( homedata->vnum[1] > 0 ) {
        for ( i = 1; i < MAX_HOUSE_ROOMS; i++ ) {
            if ( homedata->vnum[i] <= 0 )
                continue;

            if ( ( addloc = get_room_index( homedata->vnum[i] ) ) == NULL ) {
                homedata->vnum[i] = 0;
                continue;
            }

            for ( pexit = addloc->first_exit; pexit; pexit = pexit->next ) {
                if ( pexit->rexit )
                    extract_exit( pexit->to_room, pexit->rexit );

            }

            wipe_resets( addloc );

            homedata->vnum[i] = 0;

            delete_room( addloc );

#ifdef ADDED_ROOM_HOUSING_AREA

            if ( ADDED_ROOM_HOUSING_AREA ) {
                char                    filename[256];
                AREA_DATA              *pArea,
                                       *tarea = NULL;

                pArea = get_area( ADDED_ROOM_HOUSING_AREA );

                for ( tarea = first_area; tarea; tarea = tarea->next ) {
                    if ( tarea == pArea )
                        fold_area( pArea, pArea->filename, FALSE );
                }

                for ( tarea = first_build; tarea; tarea = tarea->next ) {
                    if ( tarea == pArea ) {
                        sprintf( filename, "%s%s", BUILD_DIR, pArea->filename );
                        fold_area( pArea, filename, FALSE );
                    }
                }
            }
#endif

        }

    }

    if ( !homedata->apartment && ( key = get_obj_index( homedata->vnum[0] ) ) != NULL ) {
        delete_obj( key );
    }

    STRFREE( location->name );
    location->name = STRALLOC( "Vacant Residence" );
    STRFREE( location->description );
    location->description = STRALLOC( "" );
    location->sector_type = 0;

#ifndef EXTENDED_ROOMS
    REMOVE_BIT( location->room_flags, ROOM_NO_SUMMON );
    REMOVE_BIT( location->room_flags, ROOM_NO_ASTRAL );
    REMOVE_BIT( location->room_flags, ROOM_INDOORS );
    REMOVE_BIT( location->room_flags, ROOM_SAFE );
#else
    xREMOVE_BIT( location->room_flags, ROOM_NO_SUMMON );
    xREMOVE_BIT( location->room_flags, ROOM_NO_ASTRAL );
    xREMOVE_BIT( location->room_flags, ROOM_INDOORS );
    xREMOVE_BIT( location->room_flags, ROOM_SAFE );
#endif

    for ( pexit = location->first_exit; pexit; pexit = pexit->next ) {
        pexit->key = -1;
        REMOVE_BIT( pexit->exit_info, EX_ISDOOR );
        REMOVE_BIT( pexit->exit_info, EX_CLOSED );
        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
        REMOVE_BIT( pexit->exit_info, EX_NOPASSDOOR );
        REMOVE_BIT( pexit->exit_info, EX_PICKPROOF );
        REMOVE_BIT( pexit->exit_info, EX_BASHPROOF );
        REMOVE_BIT( pexit->exit_info, EX_NOMOB );

        if ( pexit->rexit ) {
            pexit->rexit->key = -1;
            REMOVE_BIT( pexit->rexit->exit_info, EX_ISDOOR );
            REMOVE_BIT( pexit->rexit->exit_info, EX_CLOSED );
            REMOVE_BIT( pexit->rexit->exit_info, EX_LOCKED );
            REMOVE_BIT( pexit->rexit->exit_info, EX_NOPASSDOOR );
            REMOVE_BIT( pexit->rexit->exit_info, EX_PICKPROOF );
            REMOVE_BIT( pexit->rexit->exit_info, EX_BASHPROOF );
            REMOVE_BIT( pexit->rexit->exit_info, EX_NOMOB );
        }
    }

    wipe_resets( location );

    save_residence( location );
    UNLINK( homedata, first_home, last_home, next, prev );
    DISPOSE( homedata );
    update_house_list(  );

    return TRUE;
}

bool add_room( HOME_DATA * homedata, ROOM_INDEX_DATA *location, char *argument )
{
    ROOM_INDEX_DATA        *addloc = NULL;
    AREA_DATA              *pArea = NULL;
    EXIT_DATA              *pexit,
                           *rexit = NULL;
    char                    buf[MAX_STRING_LENGTH];
    short                   i = 0;

    if ( !location || !homedata || !argument || argument[0] == '\0' )
        return FALSE;

#ifdef ADDED_ROOM_HOUSING_AREA
    pArea = get_area( ADDED_ROOM_HOUSING_AREA );
#else
    pArea = location->area;
#endif

    if ( !pArea )
        return FALSE;

    for ( i = pArea->low_r_vnum; i < pArea->hi_r_vnum; i++ ) {
        if ( !( get_room_index( i ) ) ) {
            if ( !( addloc = make_room( i, pArea ) ) )
                return FALSE;
            break;
        }
    }

    if ( !addloc )
        return FALSE;

    if ( ( pexit = make_exit( location, addloc, get_dir( argument ) ) ) == NULL
         || ( rexit = make_exit( addloc, location, rev_dir[get_dir( argument )] ) ) == NULL )
        return FALSE;

    pexit->keyword = STRALLOC( "" );
    pexit->description = STRALLOC( "" );
    pexit->key = -1;
    pexit->exit_info = 0;
    rexit->keyword = STRALLOC( "" );
    rexit->description = STRALLOC( "" );
    rexit->key = -1;
    rexit->exit_info = 0;
    pexit->rexit = rexit;
    rexit->rexit = pexit;

    for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
        if ( homedata->vnum[i] == 0 ) {
            homedata->vnum[i] = addloc->vnum;
            break;
        }
    }

    fwrite_house( homedata );

    sprintf( buf, "%s\'s Additional House Room %d", capitalize( homedata->name ), i );
    STRFREE( addloc->name );
    addloc->name = STRALLOC( buf );
    STRFREE( addloc->description );
    addloc->description = STRALLOC( "This is your desc. You can edit this with HOUSE DESC.\r\n" );
    addloc->sector_type = 0;
#ifndef EXTENDED_ROOMS
    SET_BIT( addloc->room_flags, ROOM_NO_SUMMON );
    SET_BIT( addloc->room_flags, ROOM_NO_ASTRAL );
    SET_BIT( addloc->room_flags, ROOM_INDOORS );
    SET_BIT( addloc->room_flags, ROOM_SAFE );
    REMOVE_BIT( addloc->room_flags, ROOM_PROTOTYPE );
#else
    xSET_BIT( addloc->room_flags, ROOM_NO_SUMMON );
    xSET_BIT( addloc->room_flags, ROOM_NO_ASTRAL );
    xSET_BIT( addloc->room_flags, ROOM_INDOORS );
    xSET_BIT( addloc->room_flags, ROOM_SAFE );
    xREMOVE_BIT( addloc->room_flags, ROOM_PROTOTYPE );
#endif

    addloc->area = pArea;
    save_residence( addloc );
    if ( addloc->area != location->area )
        save_residence( location );

    return TRUE;
}

bool give_key( CHAR_DATA *ch, int vnum )
{
    OBJ_INDEX_DATA         *keyindex = NULL;
    OBJ_DATA               *key = NULL;

    if ( !ch || vnum <= 0 )
        return FALSE;

    if ( ( keyindex = get_obj_index( vnum ) ) == NULL )
        return FALSE;

    if ( ( key = create_object( keyindex, 1 ) ) == NULL )
        return FALSE;

    obj_to_char( key, ch );

    send_to_char( "A house key appears in your inventory.\r\n", ch );

    return TRUE;
}

void fwrite_house( HOME_DATA * homedata )
{
    FILE                   *fpout;
    short                   i,
                            j = 0;
    OBJ_DATA               *obj = NULL;
    char                    buf[MAX_STRING_LENGTH];
    char                    strsave[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA        *location = NULL;

    if ( !homedata || !homedata->name || homedata->name[0] == '\0' ) {
        bug( "Fwrite_house: NULL homedata" );
        return;
    }

    sprintf( strsave, "%s%s", HOUSE_DIR, capitalize( homedata->name ) );

    if ( ( fpout = FileOpen( strsave, "w" ) ) == NULL ) {
        sprintf( buf, "Fwrite_house: Cannot open %s for writing.", strsave );
        bug( buf, 0 );
        return;
    }

    fprintf( fpout, "#HOME\n" );
    fprintf( fpout, "Name        %s~\n", homedata->name );

    for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
        if ( homedata->vnum[i] > 0 )
            fprintf( fpout, "Vnum    %d\n", homedata->vnum[i] );
    }

    fprintf( fpout, "Apartment   %d\n", homedata->apartment );
    fprintf( fpout, "End\n\n" );

    for ( i = 0; i < MAX_HOUSE_ROOMS; i++ ) {
        if ( homedata->vnum[i] > 0 && ( location = get_room_index( homedata->vnum[i] ) ) != NULL ) {
            obj = location->last_content;
            j = supermob->level;
            supermob->level = MAX_LEVEL;

            if ( obj )
                fwrite_obj( supermob, obj, fpout, 0, OS_CARRY, TRUE );
            supermob->level = j;
        }
    }

    fprintf( fpout, "#END\n" );
    FileClose( fpout );
    fpout = NULL;
}

void load_homedata(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    list[256];
    char                    buf[MAX_STRING_LENGTH];

    first_home = NULL;
    last_home = NULL;

    sprintf( list, "%s%s", HOUSE_DIR, HOUSE_LIST );

    if ( ( fpList = FileOpen( list, "r" ) ) == NULL ) {
        log_string( "Cannot open house.lst for reading. Home loading aborted." );
        return;
    }

    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );

        if ( filename[0] == '$' )
            break;

        if ( !load_house_file( filename ) ) {
            sprintf( buf, "Cannot load house file: %s", filename );
            bug( buf, 0 );
        }

    }

    FileClose( fpList );
    fpList = NULL;
    return;

}

#define MAX_NEST	100
static OBJ_DATA        *rgObjNest[MAX_NEST];

bool load_house_file( const char *name )
{
    FILE                   *fp;
    char                    filename[256];
    char                    buf[MAX_STRING_LENGTH];
    int                     iNest,
                            vnum = 0;
    OBJ_DATA               *obj = NULL;

    sprintf( filename, "%s%s", HOUSE_DIR, name );

    if ( ( fp = FileOpen( filename, "r" ) ) == NULL )
        return FALSE;

    for ( iNest = 0; iNest < MAX_NEST; iNest++ )
        rgObjNest[iNest] = NULL;

    for ( ;; ) {
        char                    letter;
        const char             *word;

        letter = fread_letter( fp );

        if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }

        if ( letter != '#' ) {

            sprintf( buf, "Load_house_file: # not found in %s.", filename );
            bug( buf, 0 );
            break;
        }

        word = fread_word( fp );

        if ( !str_cmp( word, "HOME" ) ) {
            vnum = fread_house( fp );
            if ( vnum <= 2 )
                vnum = 2;
            rset_supermob( get_room_index( vnum ) );
            continue;
        }
        else if ( !str_cmp( word, "OBJECT" ) ) {
            fread_obj( supermob, fp, OS_CARRY );
            continue;
        }
        else if ( !str_cmp( word, "END" ) ) {
            break;
        }
        else {
            sprintf( buf, "Load_house_file: bad section: %s in file %s.", word, filename );
            bug( buf, 0 );
            break;
        }
    }

    FileClose( fp );
    fp = NULL;

    while ( ( obj = supermob->first_carrying ) != NULL ) {
        obj_from_char( obj );
        obj_to_room( obj, get_room_index( vnum ) );
    }

    release_supermob(  );

    return TRUE;
}

void update_house_list(  )
{
    HOME_DATA              *tmphome;
    FILE                   *fpout;
    char                    filename[256];

    sprintf( filename, "%s%s", HOUSE_DIR, HOUSE_LIST );

    if ( ( fpout = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "FATAL: cannot open house.lst for writing!\r\n" );
        return;
    }

    for ( tmphome = first_home; tmphome; tmphome = tmphome->next )
        fprintf( fpout, "%s\n", capitalize( tmphome->name ) );

    fprintf( fpout, "$\n" );
    FileClose( fpout );
    fpout = NULL;
}

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

int fread_house( FILE * fp )
{
    short                   i = 0;
    HOME_DATA              *homedata = NULL;
    char                    buf[MAX_STRING_LENGTH];

    CREATE( homedata, HOME_DATA, 1 );

    for ( ;; ) {
        const char             *word;
        bool                    fMatch;

        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {

            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !homedata->name ) {
                        bug( "Fread_house: NULL Name" );
                        DISPOSE( homedata );
                        return -1;
                    }

                    LINK( homedata, first_home, last_home, next, prev );

                    return ( homedata->vnum[0] <= 0 ? 2 : homedata->vnum[0] );
                }

                break;

            case 'A':
                KEY( "Apartment", homedata->apartment, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", homedata->name, fread_string( fp ) );
                break;

            case 'V':
                KEY( "Vnum", homedata->vnum[i++], fread_number( fp ) );
                break;
        }

        if ( !fMatch ) {
            sprintf( buf, "Fread_house: no match: %s", word );
            bug( buf, 0 );
        }
    }
}

#undef KEY

void save_house_by_vnum( int vnum )
{
    HOME_DATA              *tmphome = NULL;
    short                   i = 0;

    for ( tmphome = first_home; tmphome; tmphome = tmphome->next )
        for ( i = 0; i < MAX_HOUSE_ROOMS; i++ )
            if ( tmphome->vnum[i] == vnum )
                fwrite_house( tmphome );

    return;
}

void save_accessories(  )
{
    FILE                   *fpout;
    ACCESSORIES_DATA       *tmpacc = NULL;
    OBJ_INDEX_DATA         *obj = NULL;
    MOB_INDEX_DATA         *mob = NULL;

    if ( ( fpout = FileOpen( ACCESSORIES_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open homeaccessories.dat for writing" );
        perror( ACCESSORIES_FILE );
        return;
    }

    for ( tmpacc = first_accessory; tmpacc; tmpacc = tmpacc->next ) {
        if ( !tmpacc->mob ) {
            if ( ( obj = get_obj_index( tmpacc->vnum ) ) == NULL )
                continue;
        }
        else {
            if ( ( mob = get_mob_index( tmpacc->vnum ) ) == NULL )
                continue;
        }

        fprintf( fpout, "#ACCESSORIES\n" );
        fprintf( fpout, "Vnum        %d\n", tmpacc->vnum );
        fprintf( fpout, "Price       %d\n", tmpacc->price );
        fprintf( fpout, "Mob         %d\n", tmpacc->mob );
        fprintf( fpout, "End\n\n" );
    }

    fprintf( fpout, "#END\n" );
    FileClose( fpout );
    fpout = NULL;
}

void load_accessories(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( ACCESSORIES_FILE, "r" ) ) == NULL ) {
        bug( "Cannot open homeaccessories.dat. Aborting loadup of accessories data." );
        return;
    }

    for ( ;; ) {
        char                    letter;
        char                   *word;

        letter = fread_letter( fp );

        if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }

        if ( letter != '#' ) {
            bug( "Load_accessories: # not found." );
            break;
        }

        word = fread_word( fp );

        if ( !str_cmp( word, "ACCESSORIES" ) ) {
            fread_accessories( fp );
            continue;
        }
        else if ( !str_cmp( word, "END" ) )
            break;
        else {
            bug( "Load_accessories: bad section." );
            continue;
        }
    }

    FileClose( fp );
    fp = NULL;
}

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_accessories( FILE * fp )
{
    ACCESSORIES_DATA       *newacc = NULL;
    char                    buf[MAX_STRING_LENGTH];

    CREATE( newacc, ACCESSORIES_DATA, 1 );

    for ( ;; ) {
        const char             *word;
        bool                    fMatch;

        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {

            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( ( newacc->mob && ( get_mob_index( newacc->vnum ) ) == NULL )
                         || ( !newacc->mob && ( get_obj_index( newacc->vnum ) ) == NULL ) ) {
                        bug( "Fread_accessories: Accessory doesn't exist" );
                        DISPOSE( newacc );
                        return;
                    }

                    LINK( newacc, first_accessory, last_accessory, next, prev );

                    return;
                }

                break;

            case 'M':
                KEY( "Mob", newacc->mob, fread_number( fp ) );
                break;

            case 'P':
                KEY( "Price", newacc->price, fread_number( fp ) );
                break;

            case 'V':
                KEY( "Vnum", newacc->vnum, fread_number( fp ) );
                break;
        }

        if ( !fMatch ) {
            sprintf( buf, "Fread_accessories: no match: %s", word );
            bug( buf, 0 );
        }
    }
}

#undef KEY

bool add_homebuy( CHAR_DATA *seller, int vnum, bool apartment, int price )
{
    HOMEBUY_DATA           *tmphome = NULL;

    if ( vnum <= 0 || !seller || IS_NPC( seller ) )
        return FALSE;

    for ( tmphome = first_homebuy; tmphome; tmphome = tmphome->next ) {
        if ( tmphome->vnum == vnum )
            break;
    }

    if ( tmphome )
        return FALSE;

    if ( ( get_room_index( vnum ) ) == NULL )
        return FALSE;

    CREATE( tmphome, HOMEBUY_DATA, 1 );
    LINK( tmphome, first_homebuy, last_homebuy, next, prev );

    tmphome->vnum = vnum;
    tmphome->apartment = apartment;
    tmphome->endtime = ( 7 * 48 );
    tmphome->incpercent = DEFAULT_BID_INCREMENT_PERCENTAGE;
    tmphome->bidder = STRALLOC( "None" );
    tmphome->seller = STRALLOC( seller->name );

    if ( price <= 0 ) {
        if ( apartment )
            tmphome->bid = MIN_APARTMENT_BID;
        else
            tmphome->bid = MIN_HOUSE_BID;
    }
    else {
        tmphome->bid = price;
    }

    save_homebuy(  );

    return TRUE;
}

bool remove_homebuy( HOMEBUY_DATA * tmphome )
{
    if ( !tmphome )
        return FALSE;

    STRFREE( tmphome->bidder );
    STRFREE( tmphome->seller );
    tmphome->incpercent = 0;
    tmphome->apartment = FALSE;
    tmphome->bid = 0;
    tmphome->vnum = 0;
    tmphome->endtime = 0;

    UNLINK( tmphome, first_homebuy, last_homebuy, next, prev );

    DISPOSE( tmphome );

    save_homebuy(  );

    return TRUE;
}

void save_homebuy(  )
{
    FILE                   *fpout;
    HOMEBUY_DATA           *tmphome = NULL;
    LMSG_DATA              *tmpmsg = NULL;

    if ( ( fpout = FileOpen( HOMEBUY_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open homebuy.dat for writing" );
        perror( HOMEBUY_FILE );
        return;
    }

    for ( tmphome = first_homebuy; tmphome; tmphome = tmphome->next ) {
        if ( ( get_room_index( tmphome->vnum ) ) == NULL )
            continue;

        fprintf( fpout, "#HOMEBUY\n" );
        fprintf( fpout, "Apartment   %d\n", tmphome->apartment );
        fprintf( fpout, "Bid         %d\n", tmphome->bid );
        fprintf( fpout, "Bidder      %s~\n", tmphome->bidder );
        fprintf( fpout, "BidIncPerc  %d\n", tmphome->incpercent );
        fprintf( fpout, "Endtime     %d\n", tmphome->endtime );
        fprintf( fpout, "Seller      %s~\n", tmphome->seller );
        fprintf( fpout, "Vnum        %d\n", tmphome->vnum );
        fprintf( fpout, "End\n\n" );
    }

    for ( tmpmsg = first_lmsg; tmpmsg; tmpmsg = tmpmsg->next ) {
        fprintf( fpout, "#LOGINMSG\n" );
        fprintf( fpout, "Name        %s~\n", tmpmsg->name );
        fprintf( fpout, "Type        %d\n", tmpmsg->type );
        fprintf( fpout, "End\n\n" );
    }

    fprintf( fpout, "#END\n" );
    FileClose( fpout );
    fpout = NULL;
}

void load_homebuy(  )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( HOMEBUY_FILE, "r" ) ) == NULL ) {
        bug( "Cannot open homebuy.dat. Aborting loadup of home auction data." );
        return;
    }

    for ( ;; ) {
        char                    letter;
        char                   *word;

        letter = fread_letter( fp );

        if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }

        if ( letter != '#' ) {
            bug( "Load_accessories: # not found." );
            break;
        }

        word = fread_word( fp );

        if ( !str_cmp( word, "HOMEBUY" ) ) {
            fread_homebuy( fp );
            continue;
        }
        else if ( !str_cmp( word, "LOGINMSG" ) ) {
            fread_loginmsg( fp );
            continue;
        }
        else if ( !str_cmp( word, "END" ) )
            break;
        else {
            bug( "Load_homebuy: bad section." );
            continue;
        }
    }

    FileClose( fp );
    fp = NULL;
}

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_homebuy( FILE * fp )
{
    HOMEBUY_DATA           *newhome = NULL;
    char                    buf[MAX_STRING_LENGTH];

    CREATE( newhome, HOMEBUY_DATA, 1 );

    for ( ;; ) {
        const char             *word;
        bool                    fMatch;

        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {

            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                KEY( "Apartment", newhome->apartment, fread_number( fp ) );
                break;

            case 'B':
                KEY( "Bid", newhome->bid, fread_number( fp ) );
                KEY( "Bidder", newhome->bidder, fread_string( fp ) );
                KEY( "BidIncPerc", newhome->incpercent, fread_number( fp ) );
                break;

            case 'E':

                KEY( "Endtime", newhome->endtime, fread_number( fp ) );

                if ( !str_cmp( word, "End" ) ) {
                    if ( ( get_room_index( newhome->vnum ) ) == NULL ) {
                        bug( "Fread_homebuy: Residence doesn't exist" );
                        DISPOSE( newhome );
                        return;
                    }

                    LINK( newhome, first_homebuy, last_homebuy, next, prev );

                    return;
                }

                break;

            case 'S':
                KEY( "Seller", newhome->seller, fread_string( fp ) );
                break;

            case 'V':
                KEY( "Vnum", newhome->vnum, fread_number( fp ) );
                break;

        }

        if ( !fMatch ) {
            sprintf( buf, "Fread_homebuy: no match: %s", word );
            bug( buf, 0 );
        }
    }
}

#undef KEY

void homebuy_update(  )
{
    HOMEBUY_DATA           *tmphome,
                           *tmphomenext = NULL;
    CHAR_DATA              *bidder,
                           *seller = NULL;
    char                    buf[MAX_STRING_LENGTH];

    for ( tmphome = first_homebuy; tmphome; tmphome = tmphomenext ) {
        tmphomenext = tmphome->next;

        if ( --tmphome->endtime > 0 ) {
            save_homebuy(  );
            continue;
        }

        bidder = NULL;
        seller = NULL;

        if ( tmphome->bidder[0] != '\0' && str_cmp( tmphome->bidder, "None" )
             && ( bidder = load_player( tmphome->bidder ) ) != NULL ) {

            if ( bidder->gold < tmphome->bid ) {
                if ( bidder->gold >= ( ( tmphome->bid * PENALTY_PERCENTAGE ) / 100 ) )
                    bidder->gold -= ( ( tmphome->bid * PENALTY_PERCENTAGE ) / 100 );
                else
                    bidder->gold = 0;

                tmphome->endtime = ( 7 * 48 );

                STRFREE( tmphome->bidder );
                tmphome->bidder = STRALLOC( "None" );

                if ( bidder->desc )
                    send_to_char_color
                        ( "&C&wYou did not have enough money for the residence you bid on.\r\n"
                          "It has been readded to the auction and you've been penalized.\r\n",
                          bidder );
                else
                    add_loginmsg( bidder->name, 0 );

                save_homebuy(  );
                logoff( bidder );

                continue;

            }

        }

        if ( tmphome->seller[0] == '\0' || ( seller = load_player( tmphome->seller ) ) == NULL ) {
            sprintf( buf, "Homebuy_update: Seller of residence with vnum %d could not be found.",
                     tmphome->vnum );
            bug( buf, 0 );

            if ( !remove_homebuy( tmphome ) ) {
                sprintf( buf,
                         "Homebuy_update: Residence with vnum %d could not be removed from housing auction",
                         tmphome->vnum );
                bug( buf, 0 );
            }

            if ( bidder->desc )
                send_to_char_color
                    ( "&C&wThere was an error in looking up the seller for the residence\r\n"
                      "you had been on. Residence removed and no interaction has taken place.\r\n",
                      bidder );
            else
                add_loginmsg( bidder->name, 1 );

            if ( bidder )
                logoff( bidder );

            continue;
        }

        if ( !bidder ) {

            if ( seller->gold >= ( ( tmphome->bid * PENALTY_PERCENTAGE ) / 100 ) )
                seller->gold -= ( ( tmphome->bid * PENALTY_PERCENTAGE ) / 100 );
            else
                seller->gold = 0;

            if ( !remove_homebuy( tmphome ) ) {

                sprintf( buf,
                         "Homebuy_update: Residence with vnum %d could not be removed from housing auction",
                         tmphome->vnum );
                bug( buf, 0 );

            }

            if ( seller->desc )
                send_to_char_color
                    ( "&C&wThere was no bidder on your residence. Your residence has been removed\r\n"
                      "from auction and you have been penalized.\r\n", seller );
            else
                add_loginmsg( seller->name, 2 );

            logoff( seller );

            continue;

        }

        if ( !remove_house( seller ) && !IS_IMMORTAL( seller ) ) {

            sprintf( buf,
                     "Homebuy_update: Residence could not be removed from seller successfully for vnum %d",
                     tmphome->vnum );
            bug( buf, 0 );
            logoff( bidder );
            logoff( seller );
            continue;

        }

        if ( !set_house( bidder, tmphome->vnum, tmphome->apartment ) ) {

            sprintf( buf, "Homebuy_update: Residence could not be setup for vnum %d",
                     tmphome->vnum );
            bug( buf, 0 );
            logoff( bidder );
            logoff( seller );
            continue;

        }

        if ( !give_key( bidder, tmphome->vnum ) ) {

            sprintf( buf, "Homebuy_update: Key for residence with vnum %d could not be given.",
                     tmphome->vnum );
            bug( buf, 0 );

        }

        bidder->gold -= tmphome->bid;
        seller->gold += tmphome->bid;

        if ( !remove_homebuy( tmphome ) ) {

            sprintf( buf,
                     "Homebuy_update: Residence with vnum %d could not be removed from housing auction",
                     tmphome->vnum );
            bug( buf, 0 );

        }

        if ( bidder->desc )
            send_to_char_color( "&C&wYou have successfully received your new residence.\r\n",
                                bidder );
        else
            add_loginmsg( bidder->name, 3 );

        if ( seller->desc )
            send_to_char_color( "&C&wYou have successfully sold your residence.\r\n", seller );
        else
            add_loginmsg( bidder->name, 4 );

        logoff( bidder );
        logoff( seller );
    }

    return;
}

CHAR_DATA              *load_player( char *name )
{
    CHAR_DATA              *onlinechar = NULL;
    DESCRIPTOR_DATA        *d = NULL;
    struct stat             fst;
    char                    buf[MAX_STRING_LENGTH];
    int                     oldvnum = 0;

    sprintf( buf, "%s%c/%s", PLAYER_DIR, LOWER( name[0] ), capitalize( name ) );

    for ( onlinechar = first_char; onlinechar; onlinechar = onlinechar->next ) {

        if ( IS_NPC( onlinechar ) )
            continue;

        if ( !str_cmp( onlinechar->name, name ) )
            return onlinechar;

    }

    if ( !check_parse_name( name, FALSE ) )
        return NULL;

    if ( stat( buf, &fst ) == -1 )
        return NULL;

    CREATE( d, DESCRIPTOR_DATA, 1 );
    d->next = NULL;
    d->prev = NULL;
    d->connected = CON_GET_NAME;
    d->outsize = 2000;
    CREATE( d->outbuf, char, d->outsize );

    load_char_obj( d, name, FALSE, FALSE, FALSE );
    add_char( d->character );

    oldvnum =
        ( ( get_room_index( d->character->in_room->vnum ) ) !=
          NULL ) ? d->character->in_room->vnum : ROOM_VNUM_LIMBO;

    char_to_room( d->character, get_room_index( oldvnum ) );

    d->character->retran = oldvnum;

    d->character->desc = NULL;

    d->character = NULL;
    DISPOSE( d->outbuf );
    DISPOSE( d );

    for ( onlinechar = first_char; onlinechar; onlinechar = onlinechar->next ) {

        if ( IS_NPC( onlinechar ) )
            continue;

        if ( !str_cmp( onlinechar->name, name ) )
            return onlinechar;

    }

    return NULL;
}

void logoff( CHAR_DATA *ch )
{
    int                     x,
                            y;

    x = y = 0;

    if ( !ch || IS_NPC( ch ) || ch->desc != NULL || ch->switched != NULL )
        return;

    if ( ch->position == POS_MOUNTED )
        do_dismount( ch, ( char * ) "" );

    set_char_color( AT_GREY, ch );

    if ( !ch->in_room || ( get_room_index( ch->in_room->vnum ) ) == NULL )
        ch->in_room = get_room_index( ROOM_VNUM_LIMBO );

    quitting_char = ch;
    save_char_obj( ch );

    if ( sysdata.save_pets && ch->pcdata && ch->pcdata->pet )
        extract_char( ch->pcdata->pet, TRUE );

    if ( ch->pcdata && ch->pcdata->clan )
        save_clan( ch->pcdata->clan );

    saving_char = NULL;

    extract_char( ch, TRUE );

    for ( x = 0; x < MAX_WEAR; x++ )
        for ( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;

    return;
}

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}

void fread_loginmsg( FILE * fp )
{
    LMSG_DATA              *newmsg = NULL;
    char                    buf[MAX_STRING_LENGTH];

    CREATE( newmsg, LMSG_DATA, 1 );

    for ( ;; ) {
        const char             *word;
        bool                    fMatch;

        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {

            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'E':

                if ( !str_cmp( word, "End" ) ) {
                    if ( !newmsg->name || newmsg->name[0] == '\0' ) {
                        bug( "Fread_loginmsg: Login message with null name." );
                        DISPOSE( newmsg );
                        return;
                    }

                    LINK( newmsg, first_lmsg, last_lmsg, next, prev );

                    return;
                }

                break;

            case 'N':
                KEY( "Name", newmsg->name, fread_string( fp ) );
                break;

            case 'T':
                KEY( "Type", newmsg->type, fread_number( fp ) );
                break;

        }

        if ( !fMatch ) {
            sprintf( buf, "Fread_loginmsg: no match: %s", word );
            bug( buf, 0 );
        }
    }
}

#undef KEY

void add_loginmsg( char *name, short type )
{
    LMSG_DATA              *newmsg = NULL;

    if ( type < 0 || !name || name[0] == '\0' )
        return;

    CREATE( newmsg, LMSG_DATA, 1 );
    LINK( newmsg, first_lmsg, last_lmsg, next, prev );

    newmsg->type = type;
    newmsg->name = STRALLOC( name );

    save_homebuy(  );

    return;
}

void check_loginmsg( CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH];
    LMSG_DATA              *tmpmsg = NULL;

    if ( !ch || IS_NPC( ch ) )
        return;

    for ( tmpmsg = first_lmsg; tmpmsg; tmpmsg = tmpmsg->next )
        if ( !str_cmp( tmpmsg->name, ch->name ) )
            break;

    if ( !tmpmsg )
        return;

    switch ( tmpmsg->type ) {
        case 0:
            sprintf( buf,
                     "&C&wYou did not have enough money for the residence you bid on.\r\n"
                     "It has been readded to the auction and you've been penalized." );
            break;
        case 1:
            sprintf( buf,
                     "&C&wThere was an error in looking up the seller for the residence\r\n"
                     "you had been on. Residence removed and no interaction has taken place.\r\n" );
            break;
        case 2:
            sprintf( buf,
                     "&C&wThere was no bidder on your residence. Your residence has been\r\n"
                     "removed from auction and you have been penalized.\r\n" );
            break;
        case 3:
            sprintf( buf, "&C&wYou have successfully received your new residence.\r\n" );
            break;
        case 4:
            sprintf( buf, "&C&wYou have successfully sold your residence.\r\n" );
            break;
        default:
            sprintf( buf, "Error: Unknown Homebuy login msg: %d for %s.", tmpmsg->type, ch->name );
            log_string( buf );
            sprintf( buf, "&C&RYou have an invalid homebuy login msg.\r\n" );
            break;
    }

    send_to_char_color( buf, ch );

    STRFREE( tmpmsg->name );
    tmpmsg->type = 0;

    UNLINK( tmpmsg, first_lmsg, last_lmsg, next, prev );
    DISPOSE( tmpmsg );

    save_homebuy(  );

    return;
}
