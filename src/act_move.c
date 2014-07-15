 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Player movement module                                                *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include "h/mud.h"
#include "h/hint.h"
#include "h/ftag.h"
#include "h/hometowns.h"
#include "h/polymorph.h"

/* Volk - from do_look in act_info.c */
void                    check_random_mobs( CHAR_DATA *ch );
void                    check_water_mobs( CHAR_DATA *ch );
void                    check_sky_mobs( CHAR_DATA *ch );
void                    do_build_walk( CHAR_DATA *ch, char *argument );

/* Vladaar - changed this 08/04/2008, please dont change anymore.  This is due to findings
that although movement is cool to be realistic, but as a player it really sucks.  They spend
all their time having to rest, because their movement points were getting used way too fast.  
Realism vs. fun....  Fun wins every time or no players will we have.  
*/

/* Give them more moves, make moves regen faster, let refresh heal more moves. We need more 
 * leeway otherwise move mod spells like fly and float lose functionality..  - Volk
 */

const short             movement_loss[SECT_MAX] = {
/* 47 all up! SECT_INSIDE, SECT_ROAD, SECT_FIELD, SECT_FOREST, SECT_HILLS, SECT_MOUNTAIN, */
    1, 1, 1, 1, 2, 3,
/*  SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_UNDERWATER, SECT_AIR, SECT_DESERT, */
    2, 2, 2, 1, 1,
/*  SECT_AREA_ENT, SECT_OCEANFLOOR, SECT_UNDERGROUND, SECT_LAVA, SECT_SWAMP,  */
    1, 2, 1, 4, 2,
/*  SECT_CITY, SECT_VROAD, SECT_HROAD, SECT_OCEAN, SECT_JUNGLE, SECT_GRASSLAND, */
    1, 1, 1, 2, 2, 1,
/*  SECT_CROSSROAD, SECT_THICKFOREST, SECT_HIGHMOUNTAIN, SECT_ARCTIC,  */
    1, 2, 3, 2,
/*  SECT_WATERFALL, SECT_RIVER, SECT_DOCK, SECT_LAKE, SECT_CAMPSITE, SECT_PORTALSTONE, */
    2, 2, 1, 2, 1, 1,
/*  SECT_DEEPMUD, SECT_QUICKSAND, SECT_PASTURELAND, SECT_VALLEY, SECT_MOUNTAINPASS,  */
    4, 5, 1, 1, 2,
/*  SECT_BEACH, SECT_FOG, SECT_NOCHANGE, SECT_SKY, SECT_CLOUD,  */
    1, 2, 1, 1, 1,
/*  SECT_DCLOUD, SECT_ORE, SECT_MAX */
    1, 1
};

const char             *const dir_name[] = {
    "north", "east", "south", "west",
    "up", "down", "northeast", "northwest",
    "southeast", "southwest", "somewhere", "explore"
};

const int               trap_door[] = {
    TRAP_N, TRAP_E, TRAP_S, TRAP_W, TRAP_U, TRAP_D,
    TRAP_NE, TRAP_NW, TRAP_SE, TRAP_SW
};

const short             rev_dir[] = {
    2, 3, 0, 1,
    5, 4, 9, 8,
    7, 6, 10, 11
};

/* Build walk to new rooms - Vladaar - http://6dragons.org */
void do_build_walk( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA        *location,
                           *ch_location;
    AREA_DATA              *pArea;
    int                     vnum,
                            edir;
    char                    tmpcmd[MAX_INPUT_LENGTH];
    EXIT_DATA              *xit;

    if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
        set_char_color( AT_PLAIN, ch );

        ch_location = ch->in_room;

        argument = one_argument( argument, arg );

        edir = get_dir( arg );

        xit = get_exit( ch_location, edir );

        if ( !xit ) {
            pArea = ch->in_room->area;
            vnum = pArea->low_r_vnum;

            if ( !pArea ) {
                bug( "buildwalking: !pArea" );
                return;
            }

            while ( vnum <= pArea->hi_r_vnum && get_room_index( vnum ) != NULL )
                vnum++;
            if ( vnum > pArea->hi_r_vnum ) {
                send_to_char
                    ( "&GYou cannot buildwalk anymore as there are no empty higher number rooms to be found.\r\n",
                      ch );
                return;
            }
            ch_printf( ch, "&GBuildwalking from room %d to %d to the %s.\r\r\n\n",
                       ch->in_room->vnum, vnum, arg );

            location = make_room( vnum, pArea );
            if ( !location ) {
                bug( "buildwalking: make_room failed" );
                return;
            }
            location->area = pArea;
            sprintf( tmpcmd, "bexit %s %d", arg, vnum );
            do_redit( ch, tmpcmd );
        }
        else {
            vnum = xit->vnum;
            location = get_room_index( vnum );
            ch_printf( ch,
                       "&GCannot buildwalk back into a room that you already created an exit.\r\r\n\n" );
        }

        location->sector_type = ch_location->sector_type;
        location->room_flags = ch_location->room_flags;
        sprintf( buf, "%d", vnum );
        do_goto( ch, buf );

        return;
    }
    move_char( ch, get_exit( ch->in_room, edir ), 0 );
}

/*
 * Local functions.
 */
OBJ_DATA               *has_key args( ( CHAR_DATA *ch, int key ) );
void                    to_channel( const char *argument, const char *xchannel, int level );
void                    trap_sprung(  );

const char             *const sect_names[SECT_MAX][2] = {
    {"In a room", "inside"}, {"In a city", "cities"},
    {"In a field", "fields"}, {"In a forest", "forests"},
    {"hill", "hills"}, {"On a mountain", "mountains"},
    {"In the water", "waters"}, {"In rough water", "waters"},
    {"Underwater", "underwaters"}, {"In the air", "air"},
    {"In a desert", "deserts"}, {"Somewhere", "unknown"},
    {"ocean floor", "ocean floor"}, {"underground", "underground"}
};

const int               sent_total[SECT_MAX] = {
    3, 5, 4, 4, 1, 1, 1, 1, 1, 2, 2, 25, 1, 1
};

const char             *const room_sents[SECT_MAX][25] = {
    {
     "rough hewn walls of granite with the occasional spider crawling around",
     "signs of a recent battle from the bloodstains on the floor",
     "a damp musty odour not unlike rotting vegetation"},
    {
     "the occasional stray digging through some garbage",
     "merchants trying to lure customers to their tents",
     "some street people putting on an interesting display of talent",
     "an argument between a customer and a merchant about the price of an item",
     "several shady figures talking down a dark alleyway"},
    {
     "sparce patches of brush and shrubs",
     "a small cluster of trees far off in the distance",
     "grassy fields as far as the eye can see",
     "a wide variety of weeds and wildflowers"},
    {
     "tall, dark evergreens prevent you from seeing very far",
     "many huge oak trees that look several hundred years old",
     "a solitary lonely weeping willow",
     "a patch of bright white birch trees slender and tall"},
    {
     "rolling hills lightly speckled with violet wildflowers"},
    {
     "the rocky mountain pass offers many hiding places"},
    {
     "the water is smooth as glass"},
    {
     "rough waves splash about angrily"},
    {
     "a small school of fish"},
    {
     "the land far below",
     "a misty haze of clouds"},
    {
     "sand as far as the eye can see",
     "an oasis far in the distance"},
    {
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual",
     },
    {"rocks and coral which litter the ocean floor."},
    {"a lengthy tunnel of rock."}
};

char                   *grab_word( char *argument, char *arg_first )
{
    char                    cEnd;
    short                   count;

    count = 0;

    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' || ++count >= 255 ) {
        if ( *argument == cEnd ) {
            argument++;
            break;
        }
        *arg_first++ = *argument++;
    }
    *arg_first = '\0';

    while ( isspace( *argument ) )
        argument++;

    return argument;
}

char                   *wordwrap( char *txt, short wrap )
{
    static char             buf[MSL];
    char                   *bufp;

    buf[0] = '\0';
    bufp = buf;
    if ( txt != NULL ) {
        char                    line[MSL];
        char                    temp[MSL];
        char                   *ptr,
                               *p;
        int                     ln,
                                x;

        ++bufp;
        line[0] = '\0';
        ptr = txt;
        while ( *ptr ) {
            ptr = grab_word( ptr, temp );
            ln = strlen( line );
            x = strlen( temp );
            if ( ( ln + x + 1 ) < wrap ) {
                if ( ln > 0 && line[ln - 1] == '.' )
                    mudstrlcat( line, "  ", MSL );
                else
                    mudstrlcat( line, " ", MSL );
                mudstrlcat( line, temp, MSL );
                p = strchr( line, '\n' );
                if ( !p )
                    p = strchr( line, '\r' );
                if ( p ) {
                    mudstrlcat( buf, line, MSL );
                    line[0] = '\0';
                }
            }
            else {
                mudstrlcat( line, "\r\n", MSL );
                mudstrlcat( buf, line, MSL );
                mudstrlcpy( line, temp, MSL );
            }
        }
        if ( line[0] != '\0' )
            mudstrlcat( buf, line, MSL );
    }
    return bufp;
}

const char             *rev_exit( short vdir )
{
    switch ( vdir ) {
        default:
            return "somewhere";
        case 0:
            return "the south";
        case 1:
            return "the west";
        case 2:
            return "the north";
        case 3:
            return "the east";
        case 4:
            return "below";
        case 5:
            return "above";
        case 6:
            return "the southwest";
        case 7:
            return "the southeast";
        case 8:
            return "the northwest";
        case 9:
            return "the northeast";
        case 11:
            return "the entrance";
    }

    return "<??\?>";
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work.  -Thoric
 */
EXIT_DATA              *get_exit( ROOM_INDEX_DATA *room, short dir )
{
    EXIT_DATA              *xit;

    if ( !room ) {
        bug( "%s", "Get_exit: NULL room" );
        return NULL;
    }

    for ( xit = room->first_exit; xit; xit = xit->next )
        if ( xit->vdir == dir )
            return xit;
    return NULL;
}

/*
 * Function to get an exit, leading the the specified room
 */
EXIT_DATA              *get_exit_to( ROOM_INDEX_DATA *room, short dir, int vnum )
{
    EXIT_DATA              *xit;

    if ( !room ) {
        bug( "%s", "Get_exit: NULL room" );
        return NULL;
    }

    for ( xit = room->first_exit; xit; xit = xit->next )
        if ( xit->vdir == dir && xit->vnum == vnum )
            return xit;
    return NULL;
}

/*
 * Function to get the nth exit of a room   -Thoric
 */
EXIT_DATA              *get_exit_num( ROOM_INDEX_DATA *room, short count )
{
    EXIT_DATA              *xit;
    int                     cnt;

    if ( !room ) {
        bug( "%s", "Get_exit: NULL room" );
        return NULL;
    }

    for ( cnt = 0, xit = room->first_exit; xit; xit = xit->next )
        if ( ++cnt == count )
            return xit;
    return NULL;
}

/*
 * Modify movement due to encumbrance     -Thoric
 */
short encumbrance( CHAR_DATA *ch, short move )
{
    int                     cur,
                            max;

    max = can_carry_w( ch );
    cur = ch->carry_weight;
    if ( cur >= max )
        return move * 4;
    else if ( cur >= max * 0.95 )
        return ( short ) ( move * 3.5 );
    else if ( cur >= max * 0.90 )
        return move * 3;
    else if ( cur >= max * 0.85 )
        return ( short ) ( move * 2.5 );
    else if ( cur >= max * 0.80 )
        return move * 2;
    else if ( cur >= max * 0.75 )
        return ( short ) ( move * 1.5 );
    else
        return move;
}

/*
 * Check to see if a character can fall down, checks for looping   -Thoric
 */
bool will_fall( CHAR_DATA *ch, int fall )
{
    if ( !ch ) {
        bug( "%s", "will_fall: NULL *ch!!" );
        return FALSE;
    }

    if ( !ch->in_room ) {
        bug( "will_fall: Character in NULL room: %s", ch->name ? ch->name : "Unknown?!?" );
        return FALSE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NOFLOOR ) && CAN_GO( ch, DIR_DOWN )
         && ( !IS_AFFECTED( ch, AFF_FLYING )
              || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) ) ) {
        if ( fall > 80 ) {
            bug( "Falling (in a loop?) more than 80 rooms: vnum %d", ch->in_room->vnum );
            char_from_room( ch );
            char_to_room( ch, get_room_index( ch->pcdata->htown->recall ) );
            fall = 0;
            return TRUE;
        }
        set_char_color( AT_FALLING, ch );
        send_to_char( "You're falling down...\r\n", ch );
        move_char( ch, get_exit( ch->in_room, DIR_DOWN ), ++fall );
        return TRUE;
    }
    return FALSE;
}

void remove_bexit_flag( EXIT_DATA *pexit, int flag )
{
    EXIT_DATA              *pexit_rev;

    REMOVE_BIT( pexit->exit_info, flag );
    if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
        REMOVE_BIT( pexit_rev->exit_info, flag );
}

void set_bexit_flag( EXIT_DATA *pexit, int flag )
{
    EXIT_DATA              *pexit_rev;

    SET_BIT( pexit->exit_info, flag );
    if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
        SET_BIT( pexit_rev->exit_info, flag );
}

//Slightly modified to prevent those that are praying, ensared, or 
//rooted from moving. -Taon
ch_ret move_char( CHAR_DATA *ch, EXIT_DATA *pexit, int fall )
{
    ROOM_INDEX_DATA        *in_room;
    ROOM_INDEX_DATA        *to_room;
    ROOM_INDEX_DATA        *from_room;
    OBJ_DATA               *boat;
    char                    buf[MSL];
    const char             *txt,
                           *dtxt;
    ch_ret                  retcode;
    int                     move = 0;
    short                   door,
                            chance;
    bool                    drunk = FALSE;
    bool                    nuisance = FALSE;
    bool                    brief = FALSE;
    bool                    autofly = FALSE;
    bool                    autoopen = FALSE;

    if ( IS_AFFECTED( ch, AFF_OTTOS_DANCE ) ) {
        short                   chance;

        chance = number_range( 1, 4 );
        if ( chance < 4 ) {
            act( AT_ACTION, "You attempt to move, but are too caught up in the dance.", ch, NULL,
                 NULL, TO_CHAR );
            act( AT_ACTION, "$n nearly falls attempting to move away while dancing.", ch, NULL,
                 NULL, TO_ROOM );
            return FALSE;
        }
        if ( chance == 4 )
            act( AT_ACTION, "You somehow manage to dance in the direction you want to go.", ch,
                 NULL, NULL, TO_CHAR );
        act( AT_ACTION, "$n dances away from the music.", ch, NULL, NULL, TO_ROOM );
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
            send_to_char( "You cannot move from your burrowed position.\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
            xREMOVE_BIT( ch->affected_by, AFF_BURROW );
        }

        if ( IS_AFFECTED( ch, AFF_SNARE ) ) {
            send_to_char( "You're stuck in a snare, you cannot move!\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_ROOT ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "You can't move while rooted.\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_TANGLE ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "You can't move while tangled.\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_PRAYER ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "You can't move while in prayer.\r\n", ch );
            return FALSE;
        }

        if ( !IS_NPC( ch ) && IS_SET( ch->pcdata->tag_flags, TAG_FROZEN )
             && IS_SET( ch->pcdata->tag_flags, TAG_PLAYING ) ) {
            send_to_char( "You've been freezetagged, you can't move!\r\n", ch );
            return FALSE;
        }

        chance = number_range( 1, 100 );
        if ( chance < 10 && ch->level < 6 ) {
            hint_update(  );
        }

        if ( ch->position == POS_MEDITATING ) {
            send_to_char( "You are concentrating too much to do that.\r\n", ch );
            return FALSE;
        }

        if ( ch->in_room->sector_type == SECT_OCEAN && ch->move == 0
             && IS_AFFECTED( ch, AFF_FLYING ) ) {
            xREMOVE_BIT( ch->affected_by, AFF_FLYING );
        }

        if ( IS_IMMORTAL( ch ) ) {
            ch->pcdata->tmproom = ch->in_room->vnum;
        }

        if ( IS_DRUNK( ch, 2 ) && ch->position != POS_SHOVE && ch->position != POS_DRAG )
            drunk = TRUE;

        if ( ch->pcdata && xIS_SET( ch->act, PLR_TUTORIAL ) ) {
            ch_printf( ch, "&cPlease wait a moment for further instructions.\r\n" );
            if ( !IS_IMMORTAL( ch ) &&
                 ( ch->in_room && !str_cmp( ch->in_room->area->filename, "tutorial.are" ) )
                 || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "etutorial.are" ) )
                 || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "dtutorial.are" ) ) ) {
                ch_printf( ch,
                           "&cYou are temporarily held in place while the\r\ntutorial ensures the quest mob has completed\r\ngiving all neccessary information. You can\r\ndecrease the wait with &WTEXTSPEED&c command.\r\n" );
            }
            return FALSE;
        }

        /*
         * Nuisance flag, makes them walk in random directions 50% of the time. -Shaddai
         */
        if ( ch->pcdata->nuisance && ch->pcdata->nuisance->flags > 8 &&
             ch->position != POS_SHOVE && ch->position != POS_DRAG
             && number_percent(  ) > ( ch->pcdata->nuisance->flags * ch->pcdata->nuisance->power ) )
            nuisance = TRUE;
    }
    if ( IS_AFFECTED( ch, AFF_PARALYSIS ) ) {
        send_to_char( "You can't seem to move your legs!\r\n", ch );
        return FALSE;
    }

    in_room = ch->in_room;

    if ( !fall && ( nuisance || drunk ) ) {
        door = number_door(  );
        pexit = get_exit( ch->in_room, door );
    }

    retcode = rNONE;
    txt = NULL;

    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_MOUNTED ) )
        return retcode;

    from_room = in_room;

    if ( !pexit || ( to_room = pexit->to_room ) == NULL ) {
        if ( drunk && ch->position != POS_MOUNTED && ch->in_room->sector_type != SECT_WATER_SWIM
             && ch->in_room->sector_type != SECT_WATER_NOSWIM
             && ch->in_room->sector_type != SECT_UNDERWATER
             && ch->in_room->sector_type != SECT_OCEANFLOOR ) {
            switch ( number_bits( 4 ) ) {
                default:
                    act( AT_ACTION, "You drunkenly stumble into some obstacle.", ch, NULL, NULL,
                         TO_CHAR );
                    act( AT_ACTION, "$n drunkenly stumbles into a nearby obstacle.", ch, NULL, NULL,
                         TO_ROOM );
                    break;
                case 3:
                    act( AT_ACTION,
                         "In your drunken stupor you trip over your own feet and tumble to the ground.",
                         ch, NULL, NULL, TO_CHAR );
                    act( AT_ACTION, "$n stumbles drunkenly, trips and tumbles to the ground.", ch,
                         NULL, NULL, TO_ROOM );
                    set_position( ch, POS_RESTING );
                    break;
                case 4:
                    act( AT_SOCIAL, "You utter a string of slurred obscenities.", ch, NULL, NULL,
                         TO_CHAR );
                    act( AT_ACTION,
                         "Something blurry and immovable has intercepted you as you stagger along.",
                         ch, NULL, NULL, TO_CHAR );
                    act( AT_HURT,
                         "Oh geez... THAT really hurt.  Everything slowly goes dark and numb...",
                         ch, NULL, NULL, TO_CHAR );
                    act( AT_ACTION, "$n drunkenly staggers into something.", ch, NULL, NULL,
                         TO_ROOM );
                    act( AT_SOCIAL, "$n utters a string of slurred obscenities: @*&^%@*&!", ch,
                         NULL, NULL, TO_ROOM );
                    act( AT_ACTION, "$n topples to the ground with a thud.", ch, NULL, NULL,
                         TO_ROOM );
                    set_position( ch, POS_INCAP );
                    break;
            }
        }
        else if ( nuisance )
            act( AT_ACTION, "You stare around trying to remember where you where going.", ch, NULL,
                 NULL, TO_CHAR );
        else if ( drunk )
            act( AT_ACTION,
                 "You stare around trying to make sense of things through your drunken stupor.", ch,
                 NULL, NULL, TO_CHAR );
        else
            send_to_char( "Alas, you cannot go that way.\r\n", ch );
        return rNONE;
    }
    door = pexit->vdir;

    if ( ch->morph != NULL && !str_cmp( ch->morph->morph->name, "fish" )
         && ( to_room->sector_type != SECT_UNDERWATER && to_room->sector_type != SECT_OCEAN
              && to_room->sector_type != SECT_LAKE && to_room->sector_type != SECT_WATER_SWIM
              && to_room->sector_type != SECT_OCEANFLOOR && to_room->sector_type != SECT_RIVER
              && to_room->sector_type != SECT_WATER_NOSWIM ) ) {
        act( AT_ACTION, "You flop about on the land as there is no water!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_ACTION, "$n flops about on the land as there is no water!", ch, NULL, NULL,
             TO_ROOM );
        return rNONE;
    }

    /*
     * Exit is only a "window", there is no way to travel in that direction
     * unless it's a door with a window in it -Thoric
     */
    if ( IS_SET( pexit->exit_info, EX_WINDOW ) && !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
        send_to_char( "Alas, you cannot go that way.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( pexit->exit_info, EX_DIG ) ) {
        send_to_char( "Alas, you cannot go that way.\r\n", ch );
        return rNONE;
    }

    if ( IS_SET( pexit->exit_info, EX_PORTAL ) && IS_NPC( ch ) ) {
        act( AT_PLAIN, "Mobs can't use portals.", ch, NULL, NULL, TO_CHAR );
        return rNONE;
    }

    // Put out flamnig shields if they enter the water. -Taon
    if ( ( to_room->sector_type == SECT_UNDERWATER || to_room->sector_type == SECT_OCEAN
           || to_room->sector_type == SECT_LAKE || to_room->sector_type == SECT_WATER_SWIM
           || to_room->sector_type == SECT_RIVER || to_room->sector_type == SECT_WATER_NOSWIM )
         && IS_AFFECTED( ch, AFF_FLAMING_SHIELD ) ) {
        send_to_char( "&BYour flaming shield smolders out as it touches the water.&d\r\n", ch );
        affect_strip( ch, gsn_flaming_shield );
        xREMOVE_BIT( ch->affected_by, AFF_FLAMING_SHIELD );
    }

    // stop water wilderness mobs from following out of water?
    if ( IS_NPC( ch ) ) {
        if ( xIS_SET( ch->act, ACT_WATER )
             && ( to_room->sector_type != SECT_OCEANFLOOR || to_room->sector_type != SECT_OCEAN
                  || to_room->sector_type != SECT_UNDERWATER
                  || to_room->sector_type != SECT_WATER_SWIM
                  || to_room->sector_type != SECT_WATER_NOSWIM
                  || to_room->sector_type != SECT_RIVER ) ) {
            return FALSE;
        }
    }

    if ( rprog_pre_enter_trigger( ch, to_room ) == TRUE )
        return rNONE;

    if ( IS_NPC( ch )
         && ( IS_SET( pexit->exit_info, EX_NOMOB )
              || IS_SET( to_room->room_flags, ROOM_NO_MOB ) ) ) {
        act( AT_PLAIN, "Mobs can't enter there.", ch, NULL, NULL, TO_CHAR );
        return rNONE;
    }

    // Status: Just getting a good foot-hold.. -Taon
    if ( IN_WILDERNESS( ch ) ) {
        switch ( to_room->sector_type ) {              // Allow fall throughs in this
                // switch. -Taon
            case SECT_OCEAN:
                if ( IS_AFFECTED( ch, AFF_FLYING ) )   // && ch->race == RACE_DRAGON )
                {
                    send_to_char( "You soar over the mighty ocean below.\r\n", ch );
                }
                /*
                 * else if( IS_AFFECTED( ch, AFF_FLYING ) && ch->race != RACE_DRAGON ) {
                 * send_to_char( "Expending enormous energy, you soar over the mighty ocean
                 * below.\r\n", ch ); ch->move = ch->move/4; } 
                 */
                else if ( IS_IMMORTAL( ch ) )
                    send_to_char( "You gracefully walk upon water.\r\n", ch );
                else {
                    if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
                        send_to_char
                            ( "You cannot float on water, you will have to land and swim.\r\n",
                              ch );
                        return rNONE;
                    }
                }
                break;
            case SECT_ARCTIC:
                break;
            case SECT_HILLS:
                break;
            case SECT_GRASSLAND:
            case SECT_FIELD:
                break;
            case SECT_LAKE:
                break;
            case SECT_CAMPSITE:
                break;
            case SECT_CROSSROAD:
            case SECT_VROAD:
            case SECT_HROAD:
                send_to_char( "You journey upon a roadway.\r\n", ch );
                break;
            case SECT_SWAMP:
                break;
            case SECT_FOREST:
                break;
            case SECT_MOUNTAIN:
                break;
            case SECT_HIGHMOUNTAIN:
                if ( !IS_IMMORTAL( ch ) ) {
                    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
                        send_to_char( "You must be flying to go there.\r\n.\r\n", ch );
                        return FALSE;
                    }
                }
                break;
            case SECT_RIVER:
                break;
            case SECT_LAVA:
                if ( ch->race != RACE_DEMON && !IS_IMMORTAL( ch ) ) {
                    send_to_char( "Your flesh burns as its struck by boiling lava.\r\n", ch );
                    if ( ch->hit > 50 )
                        ch->hit -= 50;
                    else
                        ch->hit = 1;
                }
                break;
            case SECT_DOCK:
                break;
            case SECT_DESERT:
                break;
            case SECT_JUNGLE:
            case SECT_THICKFOREST:
                break;
            case SECT_WATERFALL:
                break;
            default:
                break;
        }
    }

    if ( IS_SET( pexit->exit_info, EX_CLOSED )
         && ( ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) && !xIS_SET( ch->act, PLR_SHADOWFORM ) )
              || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) ) {
        if ( !IS_SET( pexit->exit_info, EX_SECRET ) && !IS_SET( pexit->exit_info, EX_DIG ) ) {
            if ( drunk ) {
                act( AT_PLAIN, "$n runs into the $d in $s drunken state.", ch, NULL, pexit->keyword,
                     TO_ROOM );
                act( AT_PLAIN, "You run into the $d in your drunken state.", ch, NULL,
                     pexit->keyword, TO_CHAR );
            }
            else {
                if ( IS_AFFECTED( ch, AFF_FLYING )
                     && ch->in_room->sector_type != SECT_INSIDE
                     && pexit->to_room->sector_type != SECT_INSIDE
                     && !IS_SET( ch->in_room->room_flags, ROOM_INDOORS )
                     && !IS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) )
                    autofly = TRUE;
                else if ( xIS_SET( ch->act, PLR_AUTODOOR )
                          && !IS_SET( pexit->exit_info, EX_LOCKED ) )
                    autoopen = TRUE;
                else
                    act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
            }
        }
        else {
            if ( drunk )
                send_to_char( "You stagger around in your drunken state.\r\n", ch );
            else
                send_to_char( "Alas, you cannot go that way.\r\n", ch );
        }

        if ( !autofly && !autoopen )                   /* Let autofly and autoopen go on */
            return rNONE;
    }
/*
   if( !fall && IS_AFFECTED( ch, AFF_CHARM ) && ch->master
   && in_room == ch->master->in_room )
   {
      send_to_char( "What... and leave your beloved master?\r\n", ch );
      return rNONE;
   }
*/
    if ( room_is_private( to_room ) ) {
        send_to_char( "That room is private right now.\r\n", ch );
        return rNONE;
    }

    if ( room_is_dnd( ch, to_room ) ) {
        send_to_char( "That room is \"do not disturb\" right now.\r\n", ch );
        return rNONE;
    }

    if ( IS_NPC( ch )
         && ( IS_SET( pexit->exit_info, EX_NOMOB )
              || IS_SET( to_room->room_flags, ROOM_NO_MOB ) ) ) {
        return rNONE;
    }

    if ( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area ) {
        if ( ch->level < to_room->area->low_hard_range ) {
            if ( ch->race == RACE_DRAGON && to_room->area->low_hard_range < 100 ) {
                set_char_color( AT_TELL, ch );
                send_to_char
                    ( "A voice in your mind says, 'You might not be ready for this path, but because you are a Dragon anything may be possible!\r\n",
                      ch );
            }
            else if ( ch->race != RACE_DRAGON
                      || ( to_room->area->low_hard_range == 100 && ch->level < 100 ) ) {
                set_char_color( AT_TELL, ch );
                switch ( to_room->area->low_hard_range - ch->level ) {
                    case 1:
                        send_to_char
                            ( "A voice in your mind says, 'You are nearly ready to go that way...'\r\n",
                              ch );
                        break;
                    case 2:
                        send_to_char
                            ( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'\r\n",
                              ch );
                        break;
                    case 3:
                        send_to_char
                            ( "A voice in your mind says, 'You are not ready to go down that path... yet.'.\r\n",
                              ch );
                        break;
                    default:
                        send_to_char
                            ( "A voice in your mind says, 'You are not ready to go down that path.'.\r\n",
                              ch );
                }
                return rNONE;
            }
        }
        else if ( ch->level > to_room->area->hi_hard_range ) {
            set_char_color( AT_TELL, ch );
            send_to_char
                ( "A voice in your mind says, 'There is nothing more for you down that path.'\r\n",
                  ch );
            return rNONE;
        }
    }

    if ( !fall && !IS_NPC( ch ) ) {
        /*
         * Prevent deadlies from entering an antipkill-flagged area from a non-flagged
         * area, but allow them to move around if already inside one. - Blodkai
         */
        if ( IS_SET( to_room->area->flags, AFLAG_ANTIPKILL )
             && !IS_SET( ch->in_room->area->flags, AFLAG_ANTIPKILL ) && ( IS_PKILL( ch )
                                                                          && !IS_IMMORTAL( ch ) ) )
        {
            set_char_color( AT_MAGIC, ch );
            send_to_char
                ( "\r\nA godly force forbids deadly characters from entering that area...\r\n",
                  ch );
            return rNONE;
        }

        if ( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR
             || IS_SET( pexit->exit_info, EX_FLY ) ) {
            if ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) {
                send_to_char( "Your mount can't fly.\r\n", ch );
                return rNONE;
            }
            if ( !ch->mount && !IS_AFFECTED( ch, AFF_FLYING ) ) {
                send_to_char( "You'd need to fly to go there.\r\n", ch );
                return rNONE;
            }
        }

        if ( in_room->sector_type == SECT_WATER_NOSWIM
             || to_room->sector_type == SECT_WATER_NOSWIM ) {
            if ( ( ch->mount && !IS_FLOATING( ch->mount ) ) || !IS_FLOATING( ch ) ) {
                /*
                 * Look for a boat.
                 * We can use the boat obj for a more detailed description.
                 */
                if ( ( boat = get_objtype( ch, ITEM_BOAT ) ) != NULL ) {
                    if ( drunk )
                        txt = "paddles unevenly";
                    else
                        txt = "paddles";
                }
                else {
                    if ( ch->mount )
                        send_to_char( "Your mount would drown!\r\n", ch );
                    else
                        send_to_char( "You'd need a boat to go there.\r\n", ch );
                    return rNONE;
                }
            }
        }
        /*
         * Added this for swimming underwater 
         */
        if ( in_room->sector_type == SECT_UNDERWATER || in_room->sector_type == SECT_OCEANFLOOR ) {
            if ( ch->pcdata->learned[gsn_swim] > 50 ) {
                txt = "swims confidently";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats / 2 ) );
            }
            else if ( ch->pcdata->learned[gsn_swim] > 0 ) {
                txt = "swims shakily";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats ) );
            }
            else {
                txt = "swims";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats * 2 ) );
            }
        }

        if ( ( in_room->sector_type == SECT_WATER_SWIM
               || in_room->sector_type == SECT_WATER_NOSWIM || in_room->sector_type == SECT_OCEAN )
             && ( to_room->sector_type == SECT_UNDERWATER
                  || to_room->sector_type == SECT_OCEANFLOOR ) ) {
            if ( ch->pcdata->learned[gsn_swim] > 50 ) {
                txt = "dives confidently";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats / 2 ) );
            }
            else if ( ch->pcdata->learned[gsn_swim] > 0 ) {
                txt = "dives shakily";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats ) );
            }
            else {
                txt = "slips underwater";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats * 2 ) );
            }
        }

        if ( IS_SET( pexit->exit_info, EX_CLIMB ) ) {
            bool                    found;

            found = FALSE;
            if ( ch->mount && IS_AFFECTED( ch->mount, AFF_FLYING ) )
                found = TRUE;
            else if ( IS_AFFECTED( ch, AFF_FLYING ) )
                found = TRUE;

            if ( !found && !ch->mount ) {
                if ( ( !IS_NPC( ch ) && number_percent(  ) > LEARNED( ch, gsn_climb ) ) || drunk
                     || ch->mental_state < -90 ) {
                    send_to_char( "You start to climb... but lose your grip and fall!\r\n", ch );
                    learn_from_failure( ch, gsn_climb );
                    if ( pexit->vdir == DIR_DOWN ) {
                        retcode = move_char( ch, pexit, 1 );
                        return retcode;
                    }
                    set_char_color( AT_RED, ch );
                    send_to_char( "OUCH! You hit the ground!\r\n", ch );
                    WAIT_STATE( ch, 10 );
                    retcode = damage( ch, ch, ( pexit->vdir == DIR_UP ? 10 : 5 ), TYPE_UNDEFINED );
                    return retcode;
                }
                found = TRUE;
                learn_from_success( ch, gsn_climb );
                WAIT_STATE( ch, skill_table[gsn_climb]->beats );
                txt = "climbs";
            }

            if ( !found ) {
                send_to_char( "You can't climb.\r\n", ch );
                return rNONE;
            }
        }

        if ( ch->mount ) {
            bool                    retVal = FALSE;

            switch ( ch->mount->position ) {
                case POS_DEAD:
                    send_to_char( "Your mount is dead!\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_MORTAL:
                case POS_INCAP:
                    send_to_char( "Your mount is hurt far too badly to move.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_STUNNED:
                    send_to_char( "Your mount is too stunned to do that.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_SLEEPING:
                    send_to_char( "Your mount is sleeping.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_RESTING:
                    send_to_char( "Your mount is resting.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_SITTING:
                    send_to_char( "Your mount is sitting down.\r\n", ch );
                    retVal = TRUE;
                    break;
                default:
                    break;
            }

            if ( retVal == TRUE )
                return rNONE;

            if ( !IS_FLOATING( ch->mount ) )
                move += movement_loss[UMIN( SECT_MAX - 1, in_room->sector_type )];
            else
                move += 1;

            if ( ch->race == RACE_DRAGON && ch->Class == CLASS_BLUE
                 && ( ch->in_room->sector_type == SECT_OCEAN
                      || ch->in_room->sector_type == SECT_UNDERWATER
                      || ch->in_room->sector_type == SECT_RIVER
                      || ch->in_room->sector_type == SECT_OCEANFLOOR
                      || ch->in_room->sector_type == SECT_WATER_SWIM
                      || ch->in_room->sector_type == SECT_LAKE ) ) {
                move += 1;
            }

            if ( ch->mount->move < move ) {
                send_to_char( "Your mount is too exhausted.\r\n", ch );
                return rNONE;
            }
        }
        else {
            if ( !IS_FLOATING( ch ) )
                move +=
                    encumbrance( ch, movement_loss[UMIN( SECT_MAX - 1, in_room->sector_type )] );
            else
                move += 1;

            if ( ch->move < move ) {
                send_to_char( "You are too exhausted.\r\n", ch );
                return rNONE;
            }
        }

        WAIT_STATE( ch, 1 );
        if ( ch->mount )
            ch->mount->move -= move;
        else
            ch->move -= move;
    }

    /*
     * Check if player can fit in the room
     */
    if ( to_room->tunnel > 0 ) {
        CHAR_DATA              *ctmp;
        int                     count = ch->mount ? 1 : 0;

        for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room ) {
            if ( ++count >= to_room->tunnel ) {
                if ( ch->mount && count == to_room->tunnel )
                    send_to_char( "There is no room for both you and your mount there.\r\n", ch );
                else
                    send_to_char( "There is no room for you there.\r\n", ch );
                return rNONE;
            }
        }
    }

    if ( to_room->height > 0 ) {
        short                   gap;

        gap = ch->height - to_room->height;
        if ( ch->height > to_room->height ) {
            ch_printf( ch,
                       "You are too tall to fit in there by %d inches.\r\nYou may want to try the crawl command.\r\n",
                       gap );
            return rNONE;
        }
    }

    if ( ch->position == POS_CRAWL ) {
        txt = "crawls";
    }

    if ( door == DIR_EXPLORE ) {
        txt = "leaves to";
    }

    /*
     * Check for traps on exit - later
     */
    if ( IS_NPC( ch ) || !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
        if ( fall )
            txt = "falls";
        else if ( !txt ) {
            if ( ch->mount ) {
                if ( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
                    txt = "floats";
                else if ( IS_AFFECTED( ch->mount, AFF_FLYING ) )
                    txt = "flies";
                else
                    txt = "rides";
            }
            else {
                if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
                    if ( drunk )
                        txt = "floats unsteadily";
                    else
                        txt = "floats";
                }
                else if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
                    if ( drunk )
                        txt = "flies shakily";
                    else
                        txt = "flies";
                }
                else if ( ch->position == POS_SHOVE )
                    txt = "is shoved";
                else if ( ch->position == POS_DRAG )
                    txt = "is dragged";
                else if ( ch->position == POS_CROUCH )
                    txt = "in a crouched position, walks";
                else if ( ch->position == POS_CRAWL )
                    txt = "crawls away";
                else {
                    if ( drunk )
                        txt = "stumbles drunkenly";
                    else
                        txt = "leaves";
                }
            }
        }

        if ( VLD_STR( pexit->keyword ) ) {
            if ( autoopen ) {
                act( AT_PLAIN, "\r\nYou open the $d.\r\n", ch, NULL, pexit->keyword, TO_CHAR );
                act( AT_PLAIN, "\r\n$n opens the $d.\r\n", ch, NULL, pexit->keyword, TO_ROOM );
            }
            if ( autofly ) {
                if ( ch->race == RACE_DRAGON ) {
                    act( AT_PLAIN,
                         "\r\nUsing your mighty wings, you take flight up and over the closed $d.\r\n",
                         ch, NULL, pexit->keyword, TO_CHAR );
                    act( AT_PLAIN,
                         "\r\n$n flaps $s mighty wings and takes flight up and over the closed $d.\r\n",
                         ch, NULL, pexit->keyword, TO_ROOM );
                }
                else {
                    act( AT_PLAIN, "\r\nYou fly up and over the closed $d.\r\n", ch, NULL,
                         pexit->keyword, TO_CHAR );
                    act( AT_PLAIN, "\r\n$n takes flight up and over the closed $d.\r\n", ch, NULL,
                         pexit->keyword, TO_ROOM );
                }
            }
        }

        /*
         * Print some messages about leaving the room. -Orion
         */
        if ( !IS_AFFECTED( ch, AFF_SNEAK ) ) {
            if ( ch->mount ) {
                act_printf( AT_ACTION, ch, NULL, ch->mount, TO_NOTVICT, "$n %s %s upon $N.", txt,
                            dir_name[door] );
            }
            else {
                act_printf( AT_ACTION, ch, NULL, ( void * ) dir_name[door], TO_ROOM, "$n %s $T.",
                            txt );
            }
        }
        else {
            CHAR_DATA              *temp_char,
                                   *temp_next;
            char                    sneak_buf1[MSL],
                                    sneak_buf2[MSL];
            bool                    hasMount = ch->mount ? TRUE : FALSE;

            if ( hasMount ) {
                snprintf( sneak_buf1, MSL, "$n attempts to sneak away upon %s.%c",
                          IS_NPC( ch->mount ) ? ch->mount->short_descr : ch->mount->name, '\0' );
                snprintf( sneak_buf2, MSL, "$n %s %s upon %s.%c", txt, dir_name[door],
                          IS_NPC( ch->mount ) ? ch->short_descr : ch->name, '\0' );
            }
            else {
                snprintf( sneak_buf1, MSL, "$n attempts to sneak away.%c", '\0' );
                snprintf( sneak_buf2, MSL, "$n %s %s.%c", txt, dir_name[door], '\0' );
            }

            for ( temp_char = ch->in_room->first_person; temp_char; temp_char = temp_next ) {
                temp_next = temp_char->next_in_room;

                if ( IS_AFFECTED( temp_char, AFF_DETECT_SNEAK ) ) {
                    act( AT_ACTION, sneak_buf1, ch, NULL, temp_char, TO_VICT );
                    act( AT_ACTION, sneak_buf2, ch, NULL, temp_char, TO_VICT );
                }
            }
        }
    }

    rprog_leave_trigger( ch );

    if ( char_died( ch ) )
        return global_retcode;

    char_from_room( ch );
    char_to_room( ch, to_room );
    if ( ch->mount ) {
        rprog_leave_trigger( ch->mount );

        /*
         * Mount bug fix test. -Orion
         */
        if ( char_died( ch->mount ) )
            return global_retcode;

        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, to_room );
        }

    }

    if ( IS_NPC( ch ) || !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
        if ( fall )
            txt = "falls";
        else if ( ch->mount ) {
            if ( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
                txt = "floats in";
            else if ( IS_AFFECTED( ch->mount, AFF_FLYING ) )
                txt = "flies in";
            else
                txt = "rides in";
        }
        else {

            if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
                if ( drunk )
                    txt = "floats in unsteadily";
                else
                    txt = "floats in";
            }
            else if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
                if ( drunk )
                    txt = "flies in shakily";
                else
                    txt = "flies in";
            }
            else if ( ch->position == POS_SHOVE )
                txt = "is shoved in";
            else if ( ch->position == POS_DRAG )
                txt = "is dragged in";
            else if ( ch->position == POS_CROUCH )
                txt = "in a crouched position, walks in";
            else if ( ch->position == POS_CRAWL )
                txt = "crawls in";
            else {
                if ( drunk )
                    txt = "stumbles drunkenly in";
                else
                    txt = "arrives";
            }
        }

        dtxt = rev_exit( door );

        /*
         * Print some entering messages. -Orion
         */
        if ( !IS_AFFECTED( ch, AFF_SNEAK ) ) {
            if ( ch->mount ) {
                act_printf( AT_ACTION, ch, NULL, ch->mount, TO_ROOM, "$n %s from %s upon $N.", txt,
                            dtxt );
            }
            else {
                act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n %s from %s.", txt, dtxt );
            }
        }
        else {
            CHAR_DATA              *temp_char,
                                   *temp_next;
            char                    sneak_buf1[MSL],
                                    sneak_buf2[MSL];
            bool                    hasMount = ch->mount ? TRUE : FALSE;

            if ( hasMount ) {
                snprintf( sneak_buf1, MSL, "$n attempts to sneak closer upon %s.%c",
                          IS_NPC( ch->mount ) ? ch->mount->short_descr : ch->mount->name, '\0' );
                snprintf( sneak_buf2, MSL, "$n %s from %s upon %s.%c", txt, dtxt,
                          IS_NPC( ch->mount ) ? ch->short_descr : ch->name, '\0' );
            }
            else {
                mudstrlcpy( sneak_buf1, "$n attempts to sneak closer.", MSL );
                snprintf( sneak_buf2, MSL, "$n %s from %s.", txt, dtxt );
            }

            for ( temp_char = ch->in_room->first_person; temp_char; temp_char = temp_next ) {
                temp_next = temp_char->next_in_room;

                if ( IS_AFFECTED( temp_char, AFF_DETECT_SNEAK )
                     || ( IS_AFFECTED( temp_char, AFF_DEMONIC_SIGHT )
                          && ( IS_AFFECTED( temp_char, AFF_NOSIGHT )
                               || !IS_AFFECTED( temp_char, AFF_BLINDNESS ) ) ) ) {
                    act( AT_ACTION, sneak_buf1, ch, NULL, temp_char, TO_VICT );
                    act( AT_ACTION, sneak_buf2, ch, NULL, temp_char, TO_VICT );
                }
            }
        }
    }

    if ( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area ) {
        set_char_color( AT_MAGIC, ch );
        if ( ch->level < to_room->area->low_soft_range ) {
            send_to_char( "You feel uncomfortable being in this strange land...\r\n", ch );
        }
        else if ( ch->level > to_room->area->hi_soft_range ) {
            send_to_char( "You feel there is not much to gain visiting this place...\r\n", ch );
        }
    }

    /*
     * Make sure everyone sees the room description of death traps. -Orion
     */
    if ( IS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !IS_IMMORTAL( ch ) ) {
        if ( xIS_SET( ch->act, PLR_BRIEF ) )
            brief = TRUE;
        xREMOVE_BIT( ch->act, PLR_BRIEF );
    }

// Volk - ONCE per group!!
/* Righto - we'll check if ch is in a group first. If not, hit them with encounter. If so
   and they are the leader, hit them. If so and their leader IS NOT in the room, hit them */

    CHAR_DATA              *gch;
    bool                    random = FALSE;

    if ( !IS_GROUPED( ch ) )
        random = TRUE;
    else if ( ch->leader == ch )
        random = TRUE;
    else                                               /* ch is in a group - where is * * 
                                                        * leader */
        for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
            if ( ch->leader == gch )
                break;

    if ( !random && ch->leader != gch )
        random = TRUE;

    do_look( ch, ( char * ) "auto" );

    if ( random ) {
        if ( IN_WILDERNESS( ch ) || IN_RIFT( ch ) ) {
            if ( ch->in_room->sector_type == SECT_OCEAN )
                check_water_mobs( ch );
            else
                check_random_mobs( ch );
        }

    }

    if ( brief )
        xSET_BIT( ch->act, PLR_BRIEF );

    /*
     * Put good-old EQ-munching death traps back in!  -Thoric
     */
    if ( IS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !IS_IMMORTAL( ch ) ) {
        act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
        set_char_color( AT_DEAD, ch );
        send_to_char( "Oopsie... you're dead!\r\n", ch );
        snprintf( buf, MSL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
        log_string( buf );
        to_channel( buf, "log", LEVEL_IMMORTAL );
        extract_char( ch, FALSE );

        return rCHAR_DIED;
    }

    /*
     * BIG ugly looping problem here when the character is mptransed back to the starting 
     * room. To avoid this, check how many chars are in the room at the start and stop
     * processing followers after doing the right number of them. -Narn
     */
    if ( !fall ) {
        CHAR_DATA              *fch;
        CHAR_DATA              *nextinroom;
        int                     chars = 0,
            count = 0;

        for ( fch = from_room->first_person; fch; fch = fch->next_in_room )
            chars++;

        for ( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom ) {
            nextinroom = fch->next_in_room;
            count++;
            if ( fch != ch && fch->master == ch && !xIS_SET( fch->affected_by, AFF_GRAZE ) ) {
                if ( fch->position == POS_STANDING ) {
                    if ( !xIS_SET( fch->act, PLR_BOAT ) )
                        act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                    move_char( fch, pexit, 0 );
                }
                else if ( fch->position == POS_MOUNTED ) {
                    act( AT_ACTION, "You steer your mount to follow $N.", fch, NULL, ch, TO_CHAR );
                    move_char( fch, pexit, 0 );
                }
                else
                    send_to_char( "You need to be standing to follow!", fch );
            }
        }
    }

    if ( ch->in_room->first_content )
        retcode = check_room_for_traps( ch, TRAP_ENTER_ROOM );

    if ( retcode != rNONE )
        return retcode;

    if ( char_died( ch ) )
        return retcode;

    /*
     * Really should check mounts in this chunk of code. -Orion
     */
    mprog_entry_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    rprog_enter_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    mprog_greet_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    oprog_greet_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    if ( !will_fall( ch, fall ) && fall > 0 ) {
        if ( !IS_AFFECTED( ch, AFF_FLOATING )
             || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) ) {
            set_char_color( AT_RED, ch );
            send_to_char( "OUCH! You hit the ground!\r\n", ch );
            WAIT_STATE( ch, 10 );
            retcode = damage( ch, ch, 20 * fall, TYPE_UNDEFINED );
        }
        else {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "You lightly float down to the ground.\r\n", ch );
        }
    }
    return retcode;
}

void do_crouch( CHAR_DATA *ch, char *argument )
{

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You cannot crouch while you are flying.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "You cannot crouch while you are floating.\r\n", ch );
        return;
    }

    if ( ch->race == RACE_DRAGON ) {
        send_to_char( "You cannot crouch.\r\n", ch );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "Maybe you should finish this fight first?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "You are still on your mount.\r\n", ch );
        return;
    }

    if ( ch->position == POS_STANDING ) {
        send_to_char( "You crouch down low to make yourself smaller.\r\n", ch );
        act( AT_ACTION, "$n crouches down low to make $mself smaller.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_CROUCH );
//    ch->height = ch->height / 2;
//    ch->hitroll = ch->hitroll / 2;
//    (int) ch->damroll =( double ) ch->damroll / 2;
        return;
    }
    else {
        send_to_char( "You must be standing to crouch.\r\n", ch );
        return;
    }

    return;
}

void do_crawl( CHAR_DATA *ch, char *argument )
{

    if ( ch->position == POS_CRAWL ) {
        send_to_char( "You are already crawling!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You cannot crawl while you are flying.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "You cannot crawl while you are floating.\r\n", ch );
        return;
    }

    if ( ch->race == RACE_DRAGON ) {
        send_to_char( "Dragons cannot crawl.\r\n", ch );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "Maybe you should finish this fight first?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "You are still on your mount.\r\n", ch );
        return;
    }

    if ( ch->position == POS_STANDING ) {
        send_to_char( "You get down on your hands and knees to make yourself smaller.\r\n", ch );
        act( AT_ACTION, "$n gets down on $s hands and knees.", ch, NULL, NULL, TO_ROOM );
        set_position( ch, POS_CRAWL );
        return;
    }

    send_to_char( "You must be standing to crawl.\r\n", ch );
    return;
}

void do_north( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );

        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }

    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "north" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0 );
            return;
        }
    }
    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0 );
        return;
    }
}

void do_explore( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );

        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }

    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "explore" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_EXPLORE ), 0 );
            return;
        }
    }
}

void do_east( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }

    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "east" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0 );
        return;
    }

}

void do_south( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }

        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "south" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0 );
        return;
    }

}

void do_west( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }

        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "west" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0 );
        return;
    }

}

void do_up( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }

    if ( IN_WILDERNESS( ch ) && IS_AFFECTED( ch, AFF_FLYING ) ) {
        set_char_color( AT_RMNAME, ch );
        send_to_char( "\r\nWithin the Vast Sky\r\n", ch );
        set_char_color( AT_LBLUE, ch );
        send_to_char( "The clear open sky shows the vastness of the ground below,\r\n", ch );
        send_to_char( "and sky around.  Landmarks that cannot be seen on the ground\r\n", ch );
        send_to_char( "are easily viewed from the sky.\r\n", ch );
        set_char_color( AT_EXITS, ch );
        send_to_char( "Exits: up down.\r\n\r\n", ch );
        WAIT_STATE( ch, 5 );
        send_to_char( "\r\n&cYou soar up into the sky.\r\n\r\n", ch );
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "up" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_UP ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_UP ), 0 );
        return;
    }
}

void do_down( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "down" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0 );
        return;
    }
}

void do_northeast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "northeast" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0 );
        return;
    }
}

void do_northwest( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "northwest" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0 );
        return;
    }
}

void do_southeast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "southeast" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0 );
        return;
    }
}

void do_southwest( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "elude" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "There is no one following you.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "You somehow manage to elude your follower!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n suddenly eludes you when you weren't expecting it!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N tries to manuever suddenly to stop you from following!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "southwest" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0 );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0 );
        return;
    }
}

EXIT_DATA              *find_door( CHAR_DATA *ch, char *arg, bool quiet )
{
    EXIT_DATA              *pexit;
    int                     door;

    if ( arg == NULL || !str_cmp( arg, "" ) )
        return NULL;

    pexit = NULL;
    if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
        door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
        door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
        door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
        door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
        door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
        door = 5;
    else if ( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
        door = 6;
    else if ( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
        door = 7;
    else if ( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
        door = 8;
    else if ( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
        door = 9;
    else {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next ) {
            if ( ( quiet || IS_SET( pexit->exit_info, EX_ISDOOR ) ) && pexit->keyword
                 && nifty_is_name_prefix( arg, pexit->keyword ) )
                return pexit;
        }
        if ( !quiet )
            act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
        return NULL;
    }

    if ( ( pexit = get_exit( ch->in_room, door ) ) == NULL ) {
        if ( !quiet )
            act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
        return NULL;
    }

    if ( quiet )
        return pexit;

    if ( IS_SET( pexit->exit_info, EX_SECRET ) ) {
        act( AT_PLAIN, "You see no $T here.", ch, NULL, arg, TO_CHAR );
        return NULL;
    }

    if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
        send_to_char( "You can't do that.\r\n", ch );
        return NULL;
    }

    return pexit;
}

void toggle_bexit_flag( EXIT_DATA *pexit, int flag )
{
    EXIT_DATA              *pexit_rev;

    TOGGLE_BIT( pexit->exit_info, flag );
    if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
        TOGGLE_BIT( pexit_rev->exit_info, flag );
}

void do_open( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    EXIT_DATA              *pexit;
    int                     door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Open what?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'open door' 
         */
        EXIT_DATA              *pexit_rev;

        if ( ch->level < ( MAX_LEVEL - 4 ) ) {
            if ( IS_SET( pexit->exit_info, EX_SECRET ) && pexit->keyword
                 && !nifty_is_name( arg, pexit->keyword ) ) {
                ch_printf( ch, "You see no %s here.\r\n", arg );
                return;
            }
            if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
                send_to_char( "You can't do that.\r\n", ch );
                return;
            }
            if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
                send_to_char( "It's already open.\r\n", ch );
                return;
            }
            if ( IS_SET( pexit->exit_info, EX_LOCKED ) && IS_SET( pexit->exit_info, EX_BOLTED ) ) {
                send_to_char( "The bolts locked.\r\n", ch );
                return;
            }
            if ( IS_SET( pexit->exit_info, EX_BOLTED ) ) {
                send_to_char( "It's bolted.\r\n", ch );
                return;
            }
            if ( IS_SET( pexit->exit_info, EX_LOCKED ) ) {
                send_to_char( "It's locked.\r\n", ch );
                return;
            }
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name_prefix( arg, pexit->keyword ) ) ) {
            act( AT_ACTION, "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
            act( AT_ACTION, "You open the $d.", ch, NULL, pexit->keyword, TO_CHAR );
            if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
                CHAR_DATA              *rch;

                for ( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
                    act( AT_ACTION, "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
            remove_bexit_flag( pexit, EX_CLOSED );
            if ( ( door = pexit->vdir ) >= 0 && door < 10 )
                check_room_for_traps( ch, trap_door[door] );
            return;
        }
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'open object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            ch_printf( ch, "%s is not a container.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            ch_printf( ch, "%s is already open.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSEABLE ) ) {
            ch_printf( ch, "%s cannot be opened or closed.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_LOCKED ) ) {
            ch_printf( ch, "%s is locked.\r\n", capitalize( obj->short_descr ) );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_CLOSED );
        act( AT_ACTION, "You open $p.", ch, obj, NULL, TO_CHAR );
        act( AT_ACTION, "$n opens $p.", ch, obj, NULL, TO_ROOM );
        check_for_trap( ch, obj, TRAP_OPEN );
        return;
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
    return;
}

void do_close( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    EXIT_DATA              *pexit;
    int                     door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Close what?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'close door' 
         */
        EXIT_DATA              *pexit_rev;

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "It's already closed.\r\n", ch );
            return;
        }

        act( AT_ACTION, "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        act( AT_ACTION, "You close the $d.", ch, NULL, pexit->keyword, TO_CHAR );

        /*
         * close the other side 
         */
        if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
            CHAR_DATA              *rch;

            SET_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
                act( AT_ACTION, "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
        }
        set_bexit_flag( pexit, EX_CLOSED );
        if ( ( door = pexit->vdir ) >= 0 && door < 10 )
            check_room_for_traps( ch, trap_door[door] );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'close object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            ch_printf( ch, "%s is not a container.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_CLOSED ) ) {
            ch_printf( ch, "%s is already closed.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSEABLE ) ) {
            ch_printf( ch, "%s cannot be opened or closed.\r\n", capitalize( obj->short_descr ) );
            return;
        }

        SET_BIT( obj->value[1], CONT_CLOSED );
        act( AT_ACTION, "You close $p.", ch, obj, NULL, TO_CHAR );
        act( AT_ACTION, "$n closes $p.", ch, obj, NULL, TO_ROOM );
        check_for_trap( ch, obj, TRAP_CLOSE );
        return;
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
    return;
}

/*
 * Keyring support added by Thoric
 * Idea suggested by Onyx <MtRicmer@worldnet.att.net> of Eldarion
 *
 * New: returns pointer to key/NULL instead of TRUE/FALSE
 *
 * If you want a feature like having immortals always have a key... you'll
 * need to code in a generic key, and make sure extract_obj doesn't extract it
 */
OBJ_DATA               *has_key( CHAR_DATA *ch, int key )
{
    OBJ_DATA               *obj,
                           *obj2;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( obj->pIndexData->vnum == key
             || ( obj->item_type == ITEM_KEY && obj->value[0] == key ) )
            return obj;
        else if ( obj->item_type == ITEM_KEYRING )
            for ( obj2 = obj->first_content; obj2; obj2 = obj2->next_content )
                if ( obj2->pIndexData->vnum == key || obj2->value[0] == key )
                    return obj2;
    }

    return NULL;
}

void do_lock( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj,
                           *key;
    EXIT_DATA              *pexit;
    int                     count;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Lock what?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'lock door' 
         */

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( pexit->key < 0 ) {
            send_to_char( "It can't be locked.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, pexit->key ) ) == NULL ) {
            send_to_char( "You lack the key.\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            send_to_char( "It's already locked.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Click*\r\n", ch );
            count = key->count;
            key->count = 1;
            act( AT_ACTION, "$n locks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
            key->count = count;
            set_bexit_flag( pexit, EX_LOCKED );
            return;
        }
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'lock object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            send_to_char( "That's not a container.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( obj->value[2] < 0 ) {
            send_to_char( "It can't be locked.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, obj->value[2] ) ) == NULL ) {
            send_to_char( "You lack the key.\r\n", ch );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_LOCKED ) ) {
            send_to_char( "It's already locked.\r\n", ch );
            return;
        }

        SET_BIT( obj->value[1], CONT_LOCKED );
        send_to_char( "*Click*\r\n", ch );
        count = key->count;
        key->count = 1;
        act( AT_ACTION, "$n locks $p with $P.", ch, obj, key, TO_ROOM );
        key->count = count;
        return;
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
    return;
}

void do_unlock( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj,
                           *key;
    EXIT_DATA              *pexit;
    int                     count;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Unlock what?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'unlock door' 
         */

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( pexit->key < 0 ) {
            send_to_char( "It can't be unlocked.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, pexit->key ) ) == NULL ) {
            send_to_char( "You lack the key.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            send_to_char( "It's already unlocked.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Click*\r\n", ch );
            count = key->count;
            key->count = 1;
            act( AT_ACTION, "$n unlocks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
            key->count = count;
            if ( IS_SET( pexit->exit_info, EX_EATKEY ) ) {
                separate_obj( key );
                extract_obj( key );
            }
            remove_bexit_flag( pexit, EX_LOCKED );
            return;
        }
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'unlock object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            send_to_char( "That's not a container.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( obj->value[2] < 0 ) {
            send_to_char( "It can't be unlocked.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, obj->value[2] ) ) == NULL ) {
            send_to_char( "You lack the key.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_LOCKED ) ) {
            send_to_char( "It's already unlocked.\r\n", ch );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_LOCKED );
        send_to_char( "*Click*\r\n", ch );
        count = key->count;
        key->count = 1;
        act( AT_ACTION, "$n unlocks $p with $P.", ch, obj, key, TO_ROOM );
        key->count = count;
        if ( IS_SET( obj->value[1], CONT_EATKEY ) ) {
            separate_obj( key );
            extract_obj( key );
        }
        return;
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
    return;
}

void do_bashdoor( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    char                    arg[MIL];

    /*
     * Adjusted check to make it more multi-class friendly.  -Taon 
     */
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_bashdoor] <= 0 ) {
        send_to_char( "You're not enough of a warrior to bash doors!\r\n", ch );
        return;
    }
    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Bash what?\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "You can't break off your fight.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, FALSE ) ) != NULL ) {
        ROOM_INDEX_DATA        *to_room;
        EXIT_DATA              *pexit_rev;
        int                     chance;
        const char             *keyword;

        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "&GCalm down.  It is already open.\r\n", ch );
            return;
        }

        WAIT_STATE( ch, skill_table[gsn_bashdoor]->beats );

        if ( IS_SET( pexit->exit_info, EX_SECRET ) )
            keyword = "wall";
        else
            keyword = pexit->keyword;
        if ( !IS_NPC( ch ) )
            chance = LEARNED( ch, gsn_bashdoor ) / 2;
        else
            chance = 90;
        if ( IS_SET( pexit->exit_info, EX_LOCKED ) )
            chance /= 3;

        if ( IS_SET( pexit->exit_info, EX_BASHPROOF ) ) {
            act( AT_GREEN,
                 "WHAAM!  You slam into $d and realize it would take more then you can give to budge it!",
                 ch, NULL, keyword, TO_CHAR );
            act( AT_GREEN, "$n slams into $d!", ch, NULL, keyword, TO_ROOM );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_BASHPROOF ) && ch->move >= 15
             && number_percent(  ) < ( chance + 4 * ( get_curr_str( ch ) - 16 ) ) ) {
            REMOVE_BIT( pexit->exit_info, EX_CLOSED );
            if ( IS_SET( pexit->exit_info, EX_LOCKED ) )
                REMOVE_BIT( pexit->exit_info, EX_LOCKED );
            SET_BIT( pexit->exit_info, EX_BASHED );

            act( AT_GREEN, "Crash!  You bashed open the $d!", ch, NULL, keyword, TO_CHAR );
            act( AT_GREEN, "$n bashes open the $d!", ch, NULL, keyword, TO_ROOM );
            learn_from_success( ch, gsn_bashdoor );

            if ( ( to_room = pexit->to_room ) != NULL && ( pexit_rev = pexit->rexit ) != NULL
                 && pexit_rev->to_room == ch->in_room ) {
                CHAR_DATA              *rch;

                REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
                if ( IS_SET( pexit_rev->exit_info, EX_LOCKED ) )
                    REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
                SET_BIT( pexit_rev->exit_info, EX_BASHED );

                for ( rch = to_room->first_person; rch; rch = rch->next_in_room ) {
                    act( AT_SKILL, "The $d crashes open!", rch, NULL, pexit_rev->keyword, TO_CHAR );
                }
            }
            damage( ch, ch, ( ch->max_hit / 20 ), gsn_bashdoor );

        }
        else {
            act( AT_GREEN, "WHAAAAM!!!  You bash against the $d, but it doesn't budge.", ch, NULL,
                 keyword, TO_CHAR );
            act( AT_GREEN, "WHAAAAM!!!  $n bashes against the $d, but it holds strong.", ch, NULL,
                 keyword, TO_ROOM );
            damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
        }
    }
    else {
        act( AT_GREEN, "WHAAAAM!!!  You bash against the wall, but it doesn't budge.", ch, NULL,
             NULL, TO_CHAR );
        act( AT_GREEN, "WHAAAAM!!!  $n bashes against the wall, but it holds strong.", ch, NULL,
             NULL, TO_ROOM );
        damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
    }
    return;
}

void do_stand( CHAR_DATA *ch, char *argument )
{

    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;
    ROOM_INDEX_DATA        *in_room;
    ch_ret                  retcode;

    in_room = ch->in_room;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "You stop feigning death, and stand up.\r\n", ch );
        act( AT_ACTION, "$n stands up from $s feigned death.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_STANDING );
        affect_strip( ch, gsn_feign_death );
        xREMOVE_BIT( ch->affected_by, AFF_FEIGN );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "Maybe you should finish this fight first?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "You are already sitting - on your mount.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_UNHOLY_SPHERE ) ) {
        act( AT_BLOOD, "No longer resting, you dismiss your unholy sphere.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_BLOOD, "$n dismisses $s unholy sphere.", ch, NULL, NULL, TO_ROOM );
        affect_strip( ch, gsn_unholy_sphere );
        xREMOVE_BIT( ch->affected_by, AFF_UNHOLY_SPHERE );
        return;
    }

    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "You don't see that here.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "It has to be furniture silly.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], STAND_ON ) && !IS_SET( obj->value[2], STAND_IN )
             && !IS_SET( obj->value[2], STAND_AT ) ) {
            send_to_char( "You can't stand on that.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "There's no room to stand on $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "You stop meditating and look about.\r\n", ch );
            act( AT_ACTION, "$n stops meditating and regains $s awareness.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_STANDING );
            break;

        case POS_CRAWL:
            if ( in_room->height > 0 ) {
                if ( ch->height > in_room->height ) {
                    send_to_char( "You hit your head on the ceiling from trying to stand!\r\n",
                                  ch );
                    send_to_char( "&ROuch, that really hurt!\r\n", ch );
                    retcode = damage( ch, ch, 2, TYPE_UNDEFINED );
                    return;
                }
            }
            send_to_char( "You get off your hands and knees standing fully up.\r\n", ch );
            act( AT_ACTION, "$n gets off $s hands and knees standing fully up.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_STANDING );
            break;

        case POS_SLEEPING:
            if ( IS_AFFECTED( ch, AFF_SLEEP ) ) {
                send_to_char( "You can't wake up!\r\n", ch );
                return;
            }

            if ( obj == NULL ) {
                send_to_char( "You wake and stand up.\r\n", ch );
                act( AT_ACTION, "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if ( IS_SET( obj->value[2], STAND_AT ) ) {
                act( AT_ACTION, "You wake and stand at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                act( AT_ACTION, "You wake and stand on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You wake and stand in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_STANDING );
            do_look( ch, ( char * ) "auto" );
            break;
        case POS_RESTING:
        case POS_SITTING:
            if ( obj == NULL ) {
                send_to_char( "You stand up.\r\n", ch );
                act( AT_ACTION, "$n stands up.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if ( IS_SET( obj->value[2], STAND_AT ) ) {
                act( AT_ACTION, "You stand at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n stands at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                act( AT_ACTION, "You stand on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You stand in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_STANDING );
            break;
        case POS_STANDING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], STAND_AT ) ) {
                    act( AT_ACTION, "You stand at $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n stands at $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                    act( AT_ACTION, "You stand on $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "You stand in $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n stands on $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "You hop off of $p and stand on the ground.", ch, ch->on, NULL,
                     TO_CHAR );
                act( AT_ACTION, "$n hops off of $p and stands on the ground.", ch, ch->on, NULL,
                     TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "You are already standing.\r\n", ch );
            break;

    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "The shear weight of $n was too much for $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Your attempt to sit on $p caused it to break.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Your rest is disrupted by you falling to the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Your sleep is disrupted by your hard landing on the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Your lounging is disrupted by $p breaking.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "You take a very bad fall after $p breaks.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }

    return;
}

void do_trance( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You cannot trance while you are flying.\r\n", ch );
        return;
    }

    if ( ch->mana >= ch->max_mana ) {
        send_to_char( "You don't need to trance.\r\n", ch );
        return;
    }

    if ( ch->position != POS_MEDITATING ) {
        send_to_char( "You can't trance if you're not in a meditative position.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_trance ) ) {
        if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 ) {
            set_char_color( AT_RED, ch );
            send_to_char
                ( "Your adrenaline is pumping too hard for the deep trance!\r\n&cYou are able to calm yourself enough for the lesser trance.\r\n",
                  ch );
        }

        if ( ch->mana <= ch->max_mana / 2 && get_timer( ch, TIMER_RECENTFIGHT ) <= 0 ) {
            WAIT_STATE( ch, ( 16 ) );
            act( AT_CYAN, "You enter a deep trance state within your meditation.", ch, NULL, NULL,
                 TO_CHAR );
            act( AT_CYAN, "$n seems at utter peace with $mself.", ch, NULL, NULL, TO_ROOM );
            ch->mana = ch->mana + ( ch->max_mana / 6 );
            if ( ch->mana > ch->max_mana ) {
                ch->mana = ch->max_mana / 2 + number_range( 1, 20 );
            }
        }
        else {
            if ( get_timer( ch, TIMER_RECENTFIGHT ) <= 0 ) {
                send_to_char
                    ( "&cYou have too much mana already collected to enter the deep trance, but enter a lesser trance-like state.\r\n",
                      ch );
            }
            if ( ch->mana <= ch->max_mana - 100 ) {
                WAIT_STATE( ch, ( 32 ) );
                ch->mana += 50;
            }
            return;
        }
        learn_from_success( ch, gsn_trance );
        return;
    }
    else {
        send_to_char( "&cYou were not able to concentrate enough.\r\n", ch );
        learn_from_failure( ch, gsn_trance );
    }
    return;

}

void do_meditate( CHAR_DATA *ch, char *argument )
{
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You cannot meditate while you are flying.\r\n", ch );
        return;
    }

    if ( ch->mana >= ch->max_mana ) {
        send_to_char( "You don't need to meditate.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_meditate ) ) {
        switch ( ch->position ) {
            case POS_SLEEPING:
                if ( IS_AFFECTED( ch, AFF_SLEEP ) ) {
                    send_to_char( "You can't seem to wake up!\r\n", ch );
                    return;
                }

                send_to_char( "You wake and begin meditating.\r\n", ch );
                act( AT_ACTION,
                     "$n wakes up rolls $s eyes back, and begins chanting in meditation.", ch, NULL,
                     NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                break;

            case POS_RESTING:
                send_to_char( "You stop resting and begin meditating.\r\n", ch );
                act( AT_ACTION,
                     "$n stops resting rolls $s eyes back, and begins chanting in meditation.", ch,
                     NULL, NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                break;

            case POS_STANDING:
                send_to_char( "You begin meditating.\r\n", ch );
                act( AT_ACTION, "$n rolls $s eyes back, and begins chanting in meditation.", ch,
                     NULL, NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                break;

            case POS_SITTING:
                send_to_char( "You stand up and begin meditating.\r\n", ch );
                act( AT_ACTION,
                     "$n stands up rolls $s eyes back, and begins chanting in meditation.", ch,
                     NULL, NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                learn_from_success( ch, gsn_meditate );
                return;

            case POS_MEDITATING:
                send_to_char( "You are already meditating.\r\n", ch );
                return;

            case POS_FIGHTING:
            case POS_EVASIVE:
            case POS_DEFENSIVE:
            case POS_AGGRESSIVE:
            case POS_BERSERK:
                send_to_char( "You are busy fighting!\r\n", ch );
                return;

            case POS_MOUNTED:
                send_to_char( "You cannot meditate while riding.\r\n", ch );
                return;
        }
    }
    else {
        if ( ch->position == POS_MEDITATING )
            send_to_char( "You are already meditating!\r\n", ch );
        else {
            send_to_char( "You were not able to concentrate enough.\r\n", ch );
            learn_from_failure( ch, gsn_meditate );
        }
    }
    return;

}

void do_sit( CHAR_DATA *ch, char *argument )
{

    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "You stop feigning death, and move to a sitting position.\r\n", ch );
        act( AT_ACTION, "$n sits up from $s feigned death.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_SITTING );
        affect_strip( ch, gsn_feign_death );
        xREMOVE_BIT( ch->affected_by, AFF_FEIGN );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "Maybe you should finish this fight first?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "You are already sitting - on your mount.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You are flying, try landing first.\r\n", ch );
        return;
    }

    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "You don't see that here.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "It has to be furniture silly.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], SIT_ON ) && !IS_SET( obj->value[2], SIT_IN )
             && !IS_SET( obj->value[2], SIT_AT ) ) {
            send_to_char( "You can't sit on that.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "There's no room to sit on $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "You stop meditating and sit down.\r\n", ch );
            act( AT_ACTION, "$n stops meditating and sits down.", ch, NULL, NULL, TO_ROOM );
            set_position( ch, POS_SITTING );
            break;
        case POS_SLEEPING:
            if ( obj == NULL ) {
                send_to_char( "You wake and sit up.\r\n", ch );
                act( AT_ACTION, "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_AT ) ) {
                act( AT_ACTION, "You wake up and sit at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) ) {
                act( AT_ACTION, "You wake and sit on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You wake and sit in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM );
            }

            set_position( ch, POS_SITTING );
            break;
        case POS_RESTING:
            if ( obj == NULL )
                send_to_char( "You stop resting.\r\n", ch );
            else if ( IS_SET( obj->value[2], SIT_AT ) ) {
                act( AT_ACTION, "You sit at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) ) {
                act( AT_ACTION, "You sit on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SITTING );
            break;

        case POS_CRAWL:
            send_to_char( "You get off your hands and knees and sit.\r\n", ch );
            act( AT_ACTION, "$n gets off $s hands and knees and sit.", ch, NULL, NULL, TO_ROOM );
            set_position( ch, POS_SITTING );
            break;

        case POS_SITTING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], SIT_AT ) ) {
                    act( AT_ACTION, "You sit at $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n sits at $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                    act( AT_ACTION, "You sit on $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "You sit in $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "You hop off of $p and sit on the ground.", ch, ch->on, NULL,
                     TO_CHAR );
                act( AT_ACTION, "$n hops off of $p and sits on the ground.", ch, ch->on, NULL,
                     TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "You are already sitting.\r\n", ch );
            break;
        case POS_STANDING:
            if ( obj == NULL ) {
                send_to_char( "You sit down.\r\n", ch );
                act( AT_ACTION, "$n sits down on the ground.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_AT ) ) {
                act( AT_ACTION, "You sit down at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits down at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) ) {
                act( AT_ACTION, "You sit on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You sit down in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits down in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SITTING );
            break;
    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "The shear weight of $n was too much for $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Your attempt to sit on $p caused it to break.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Your rest is disrupted by you falling to the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Your sleep is disrupted by your hard landing on the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Your lounging is disrupted by $p breaking.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "You take a very bad fall after $p breaks.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }

    return;
}

void do_rest( CHAR_DATA *ch, char *argument )
{

    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "You cannot move from your burrowed position.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "You stop feigning death, and sprawl out resting.\r\n", ch );
        act( AT_ACTION, "$n sprawls out resting from $s feigned death.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_RESTING );
        affect_strip( ch, gsn_feign_death );
        xREMOVE_BIT( ch->affected_by, AFF_FEIGN );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "Maybe you should finish this fight first?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "You would need to land first.\r\n", ch );
        return;
    }

    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "You are already sitting - on your mount.\r\n", ch );
        return;
    }
    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "You don't see that here.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "It has to be furniture silly.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], REST_ON ) && !IS_SET( obj->value[2], REST_IN )
             && !IS_SET( obj->value[2], REST_AT ) ) {
            send_to_char( "You can't rest on that.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "There's no room to rest on $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "You stop meditating and sprawl out haphazardly.\r\n", ch );
            act( AT_ACTION, "$n stops meditating and sprawls out haphazardly.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_RESTING );
            break;
        case POS_SLEEPING:
            if ( obj == NULL ) {
                send_to_char( "You wake up and start resting.\r\n", ch );
                act( AT_ACTION, "$n wakes up and starts resting.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) ) {
                act( AT_ACTION, "You wake up and rest at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) ) {
                act( AT_ACTION, "You wake up and rest on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You wake up and rest in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_RESTING );
            break;
        case POS_RESTING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], REST_AT ) ) {
                    act( AT_ACTION, "You rest at $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n rests at $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], REST_ON ) ) {
                    act( AT_ACTION, "You rest on $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n rests on $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "You rest in $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n rests on $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "You hop off of $p and start resting on the ground.", ch, ch->on,
                     NULL, TO_CHAR );
                act( AT_ACTION, "$n hops off of $p and starts to rest on the ground.", ch, ch->on,
                     NULL, TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "You are already resting.\r\n", ch );
            break;
        case POS_STANDING:
            if ( obj == NULL ) {
                send_to_char( "You lay down, and rest peacefully.\r\n", ch );
                act( AT_ACTION, "$n lays down and rests.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) ) {
                act( AT_ACTION, "You sit down at $p and rest.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) ) {
                act( AT_ACTION, "You sit on $p and rest.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sits on $p and rests.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You rest in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n rests in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_RESTING );
            break;

        case POS_CRAWL:
            send_to_char( "You roll onto your back and rest.\r\n", ch );
            act( AT_ACTION, "$n rolls onto their back and rests.", ch, NULL, NULL, TO_ROOM );
            set_position( ch, POS_RESTING );
            break;

        case POS_SITTING:
            if ( obj == NULL ) {
                send_to_char( "You lay down and rest peacefully.\r\n", ch );
                act( AT_ACTION, "$n lays down and rests.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) ) {
                act( AT_ACTION, "You rest at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n rests at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) ) {
                act( AT_ACTION, "You rest on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n rests on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You rest in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n rests in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_RESTING );
            break;
    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "The shear weight of $n was too much for $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Your attempt to sit on $p caused it to break.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Your rest is disrupted by you falling to the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Your sleep is disrupted by your hard landing on the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Your lounging is disrupted by $p breaking.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "You take a very bad fall after $p breaks.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }
    rprog_rest_trigger( ch );
    return;
}

void do_sleep( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "Maybe you should finish this fight first?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "You would need to land first.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_PRAYER ) ) {
        send_to_char( "Can't go to sleep in the middle of a prayer!\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "If you wish to go to sleep, get off of your mount first.\r\n", ch );
        return;
    }
    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "You don't see that here.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "It has to be furniture silly.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], SLEEP_ON ) && !IS_SET( obj->value[2], SLEEP_IN )
             && !IS_SET( obj->value[2], SLEEP_AT ) ) {
            send_to_char( "You can't sleep on that.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "There's no room to sleep on $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "You stop meditating and close your eyes falling asleep.\r\n", ch );
            act( AT_ACTION, "$n stops meditating and slumps over, dead asleep.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_SLEEPING );
            break;

        case POS_SLEEPING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                    act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                    act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "You sleep in $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "You are already using $p for furniture.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "You hop off of $p and try to sleep on the ground.", ch, ch->on,
                     NULL, TO_CHAR );
                act( AT_ACTION, "$n hops off of $p and falls quickly asleep on the ground.", ch,
                     ch->on, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "You are already sleeping.\r\n", ch );
            break;
        case POS_RESTING:
            if ( obj == NULL ) {
                send_to_char( "You lean your head back more and go to sleep.\r\n", ch );
                act( AT_ACTION, "$n lies back and falls asleep on the ground.", ch, NULL, NULL,
                     TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
            }

            else {
                act( AT_ACTION, "You sleep in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SLEEPING );
            break;
        case POS_SITTING:
            if ( obj == NULL ) {
                send_to_char( "You lay down and go to sleep.\r\n", ch );
                act( AT_ACTION, "$n lies back and falls asleep on the ground.", ch, NULL, NULL,
                     TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
            }

            else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You sleep in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SLEEPING );

            break;
        case POS_STANDING:
            if ( IS_AFFECTED( ch, AFF_FEIGN ) && ( obj == NULL ) ) {
                act( AT_ACTION, "$n hits the ground ... DEAD.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( obj == NULL && !IS_AFFECTED( ch, AFF_FEIGN ) ) {
                send_to_char( "You drop down and fall asleep on the ground.\r\n", ch );
                act( AT_ACTION, "$n drops down and falls asleep on the ground.", ch, NULL, NULL,
                     TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                act( AT_ACTION, "You sleep at $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps at $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                act( AT_ACTION, "You sleep on $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps on $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "You sleep down in $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n sleeps down in $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SLEEPING );
            break;
    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "The shear weight of $n was too much for $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Your attempt to sit on $p caused it to break.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Your rest is disrupted by you falling to the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Your sleep is disrupted by your hard landing on the ground after $p broke.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Your lounging is disrupted by $p breaking.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "You take a very bad fall after $p breaks.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }
    rprog_sleep_trigger( ch );
    return;
}

void do_wake( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        do_stand( ch, argument );
        return;
    }

    if ( !IS_AWAKE( ch ) ) {
        send_to_char( "You are asleep yourself!\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_AWAKE( victim ) ) {
        act( AT_PLAIN, "$N is already awake.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_SLEEP ) || victim->position < POS_SLEEPING ) {
        act( AT_PLAIN, "You can't seem to wake $M!", ch, NULL, victim, TO_CHAR );
        return;
    }

    act( AT_ACTION, "You wake $M.", ch, NULL, victim, TO_CHAR );
    set_position( victim, POS_STANDING );
    act( AT_ACTION, "$n wakes you.", ch, NULL, victim, TO_VICT );
    return;
}

/*
 * teleport a character to another room
 */
void teleportch( CHAR_DATA *ch, ROOM_INDEX_DATA *room, bool show )
{
    char                    buf[MSL];

    if ( room_is_private( room ) )
        return;
    act( AT_ACTION, "$n disappears suddenly!", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, room );
    act( AT_ACTION, "$n arrives suddenly!", ch, NULL, NULL, TO_ROOM );
    if ( show )
        do_look( ch, ( char * ) "auto" );
    if ( IS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !IS_IMMORTAL( ch ) ) {
        act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
        set_char_color( AT_DEAD, ch );
        send_to_char( "Oopsie... you're dead!\r\n", ch );
        snprintf( buf, MSL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
        log_string( buf );
        to_channel( buf, "log", LEVEL_IMMORTAL );
        extract_char( ch, FALSE );
    }
}

void teleport( CHAR_DATA *ch, int room, EXT_BV * flags )
{
    CHAR_DATA              *nch,
                           *nch_next;
    ROOM_INDEX_DATA        *start = ch->in_room,
        *dest;
    bool                    show;

    dest = get_room_index( room );
    if ( !dest ) {
        bug( "teleport: bad room vnum %d", room );
        return;
    }

    if ( xIS_SET( *flags, TELE_SHOWDESC ) )
        show = TRUE;
    else
        show = FALSE;
    if ( !xIS_SET( *flags, TELE_TRANSALL ) ) {
        teleportch( ch, dest, show );
        return;
    }

    /*
     * teleport everybody in the room 
     */
    for ( nch = start->first_person; nch; nch = nch_next ) {
        nch_next = nch->next_in_room;
        teleportch( nch, dest, show );
    }

    /*
     * teleport the objects on the ground too 
     */
    if ( xIS_SET( *flags, TELE_TRANSALLPLUS ) ) {
        OBJ_DATA               *obj,
                               *obj_next;

        for ( obj = start->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            obj_from_room( obj );
            obj_to_room( obj, dest );
        }
    }
}

/*
 * "Climb" in a certain direction.     -Thoric
 */
void do_climb( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    bool                    found;

    found = FALSE;
    if ( argument[0] == '\0' ) {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if ( IS_SET( pexit->exit_info, EX_xCLIMB ) ) {
                move_char( ch, pexit, 0 );
                return;
            }
        send_to_char( "You cannot climb here.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
         && IS_SET( pexit->exit_info, EX_xCLIMB ) ) {
        move_char( ch, pexit, 0 );
        return;
    }
    send_to_char( "You cannot climb there.\r\n", ch );
    return;
}

/*
 * "enter" something (moves through an exit)   -Thoric
 */
void do_enter( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    bool                    found;
    OBJ_DATA               *obj;

    found = FALSE;
    CHAR_DATA              *victim = NULL;

    if ( argument[0] == '\0' ) {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if ( IS_SET( pexit->exit_info, EX_xENTER ) ) {
                move_char( ch, pexit, 0 );
                return;
            }
        if ( ch->in_room->sector_type != SECT_INSIDE && IS_OUTSIDE( ch ) )
            for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
                if ( pexit->to_room
                     && ( pexit->to_room->sector_type == SECT_INSIDE
                          || IS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) ) ) {
                    move_char( ch, pexit, 0 );
                    return;
                }
        send_to_char( "You cannot find an entrance here.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
         && IS_SET( pexit->exit_info, EX_xENTER ) ) {
        move_char( ch, pexit, 0 );
        return;
    }
    send_to_char( "You cannot enter that.\r\n", ch );
    return;
}

/*
 * Leave through an exit.     -Thoric
 */
void do_leave( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    bool                    found;

    found = FALSE;

    if ( argument[0] == '\0' ) {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if ( IS_SET( pexit->exit_info, EX_xLEAVE ) ) {
                move_char( ch, pexit, 0 );
                return;
            }
        if ( ch->in_room->sector_type == SECT_INSIDE || !IS_OUTSIDE( ch ) )
            for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
                if ( pexit->to_room && pexit->to_room->sector_type != SECT_INSIDE
                     && !IS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) ) {
                    move_char( ch, pexit, 0 );
                    return;
                }
        send_to_char( "You cannot find an exit here.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
         && IS_SET( pexit->exit_info, EX_xLEAVE ) ) {
        move_char( ch, pexit, 0 );
        return;
    }
    send_to_char( "You cannot leave that way.\r\n", ch );
    return;
}

/*
 * Check to see if an exit in the room is pulling (or pushing) players around.
 * Some types may cause damage.     -Thoric
 *
 * People kept requesting currents (like SillyMUD has), so I went all out
 * and added the ability for an exit to have a "pull" or a "push" force
 * and to handle different types much beyond a simple water current.
 *
 * This check is called by violence_update().  I'm not sure if this is the
 * best way to do it, or if it should be handled by a special queue.
 *
 * Future additions to this code may include equipment being blown away in
 * the wind (mostly headwear), and people being hit by flying objects
 *
 * TODO:
 *  handle more pulltypes
 *  give "entrance" messages for players and objects
 *  proper handling of player resistance to push/pulling
 */
ch_ret pullcheck( CHAR_DATA *ch, int pulse )
{
    ROOM_INDEX_DATA        *room;
    EXIT_DATA              *xtmp,
                           *xit = NULL;
    OBJ_DATA               *obj,
                           *obj_next;
    bool                    move = FALSE,
        moveobj = TRUE,
        showroom = TRUE;
    int                     pullfact,
                            pull;
    int                     resistance;
    const char             *tochar = NULL,
        *toroom = NULL,
        *objmsg = NULL;
    const char             *destrm = NULL,
        *destob = NULL,
        *dtxt = "somewhere";

    if ( ( room = ch->in_room ) == NULL ) {
        bug( "pullcheck: %s not in a room?!?", ch->name );
        return rNONE;
    }

    /*
     * Find the exit with the strongest force (if any) 
     */
    for ( xtmp = room->first_exit; xtmp; xtmp = xtmp->next )
        if ( xtmp->pull && xtmp->to_room && ( !xit || abs( xtmp->pull ) > abs( xit->pull ) ) )
            xit = xtmp;

    if ( !xit )
        return rNONE;

    pull = xit->pull;

    /*
     * strength also determines frequency 
     */
    pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );

    /*
     * strongest pull not ready yet... check for one that is 
     */
    if ( ( pulse % pullfact ) != 0 ) {
        for ( xit = room->first_exit; xit; xit = xit->next )
            if ( xit->pull && xit->to_room ) {
                pull = xit->pull;
                pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );
                if ( ( pulse % pullfact ) != 0 )
                    break;
            }

        if ( !xit )
            return rNONE;
    }

    /*
     * negative pull = push... get the reverse exit if any 
     */
    if ( pull < 0 )
        if ( ( xit = get_exit( room, rev_dir[xit->vdir] ) ) == NULL )
            return rNONE;

    dtxt = rev_exit( xit->vdir );

    /*
     * First determine if the player should be moved or not
     * Check various flags, spells, the players position and strength vs.
     * the pull, etc... any kind of checks you like.
     */
    switch ( xit->pulltype ) {
        case PULL_CURRENT:
        case PULL_WHIRLPOOL:
            switch ( room->sector_type ) {
                    /*
                     * allow whirlpool to be in any sector type 
                     */
                default:
                    if ( xit->pulltype == PULL_CURRENT )
                        break;
                case SECT_OCEAN:
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                    if ( ( ch->mount && !IS_FLOATING( ch->mount ) )
                         || ( !ch->mount && !IS_FLOATING( ch ) ) )
                        move = TRUE;
                    break;

                case SECT_UNDERWATER:
                case SECT_OCEANFLOOR:
                    move = TRUE;
                    break;
            }
            break;
        case PULL_GEYSER:
        case PULL_WAVE:
            move = TRUE;
            break;

        case PULL_WIND:
        case PULL_STORM:
            /*
             * if not flying... check weight, position & strength 
             */
            move = TRUE;
            break;

        case PULL_COLDWIND:
            /*
             * if not flying... check weight, position & strength 
             */
            /*
             * also check for damage due to bitter cold 
             */
            move = TRUE;
            break;

        case PULL_HOTAIR:
            /*
             * if not flying... check weight, position & strength 
             */
            /*
             * also check for damage due to heat 
             */
            move = TRUE;
            break;

            /*
             * light breeze -- very limited moving power 
             */
        case PULL_BREEZE:
            move = FALSE;
            break;

            /*
             * exits with these pulltypes should also be blocked from movement
             * ie: a secret locked pickproof door with the name "_sinkhole_", etc
             */
        case PULL_EARTHQUAKE:
        case PULL_SINKHOLE:
        case PULL_QUICKSAND:
        case PULL_LANDSLIDE:
        case PULL_SLIP:
        case PULL_LAVA:
            if ( ( ch->mount && !IS_FLOATING( ch->mount ) )
                 || ( !ch->mount && !IS_FLOATING( ch ) ) )
                move = TRUE;
            break;

            /*
             * as if player moved in that direction him/herself 
             */
        case PULL_UNDEFINED:
            return move_char( ch, xit, 0 );

            /*
             * all other cases ALWAYS move 
             */
        default:
            move = TRUE;
            break;
    }

    /*
     * assign some nice text messages 
     */
    switch ( xit->pulltype ) {
        case PULL_MYSTERIOUS:
            /*
             * no messages to anyone 
             */
            showroom = FALSE;
            break;
        case PULL_WHIRLPOOL:
        case PULL_VACUUM:
            tochar = "You are sucked $T!";
            toroom = "$n is sucked $T!";
            destrm = "$n is sucked in from $T!";
            objmsg = "$p is sucked $T.";
            destob = "$p is sucked in from $T!";
            break;
        case PULL_CURRENT:
        case PULL_LAVA:
            tochar = "You drift $T.";
            toroom = "$n drifts $T.";
            destrm = "$n drifts in from $T.";
            objmsg = "$p drifts $T.";
            destob = "$p drifts in from $T.";
            break;
        case PULL_BREEZE:
            tochar = "You drift $T.";
            toroom = "$n drifts $T.";
            destrm = "$n drifts in from $T.";
            objmsg = "$p drifts $T in the breeze.";
            destob = "$p drifts in from $T.";
            break;
        case PULL_GEYSER:
        case PULL_WAVE:
            tochar = "You are pushed $T!";
            toroom = "$n is pushed $T!";
            destrm = "$n is pushed in from $T!";
            destob = "$p floats in from $T.";
            break;
        case PULL_EARTHQUAKE:
            tochar = "The earth opens up and you fall $T!";
            toroom = "The earth opens up and $n falls $T!";
            destrm = "$n falls from $T!";
            objmsg = "$p falls $T.";
            destob = "$p falls from $T.";
            break;
        case PULL_SINKHOLE:
            tochar = "The ground suddenly gives way and you fall $T!";
            toroom = "The ground suddenly gives way beneath $n!";
            destrm = "$n falls from $T!";
            objmsg = "$p falls $T.";
            destob = "$p falls from $T.";
            break;
        case PULL_QUICKSAND:
            tochar = "You begin to sink $T into the quicksand!";
            toroom = "$n begins to sink $T into the quicksand!";
            destrm = "$n sinks in from $T.";
            objmsg = "$p begins to sink $T into the quicksand.";
            destob = "$p sinks in from $T.";
            break;
        case PULL_LANDSLIDE:
            tochar = "The ground starts to slide $T, taking you with it!";
            toroom = "The ground starts to slide $T, taking $n with it!";
            destrm = "$n slides in from $T.";
            objmsg = "$p slides $T.";
            destob = "$p slides in from $T.";
            break;
        case PULL_SLIP:
            tochar = "You lose your footing!";
            toroom = "$n loses $s footing!";
            destrm = "$n slides in from $T.";
            objmsg = "$p slides $T.";
            destob = "$p slides in from $T.";
            break;
        case PULL_VORTEX:
            tochar = "You are sucked into a swirling vortex of colors!";
            toroom = "$n is sucked into a swirling vortex of colors!";
            toroom = "$n appears from a swirling vortex of colors!";
            objmsg = "$p is sucked into a swirling vortex of colors!";
            objmsg = "$p appears from a swirling vortex of colors!";
            break;
        case PULL_HOTAIR:
            tochar = "A blast of hot air blows you $T!";
            toroom = "$n is blown $T by a blast of hot air!";
            destrm = "$n is blown in from $T by a blast of hot air!";
            objmsg = "$p is blown $T.";
            destob = "$p is blown in from $T.";
            break;
        case PULL_COLDWIND:
            tochar = "A bitter cold wind forces you $T!";
            toroom = "$n is forced $T by a bitter cold wind!";
            destrm = "$n is forced in from $T by a bitter cold wind!";
            objmsg = "$p is blown $T.";
            destob = "$p is blown in from $T.";
            break;
        case PULL_WIND:
            tochar = "A strong wind pushes you $T!";
            toroom = "$n is blown $T by a strong wind!";
            destrm = "$n is blown in from $T by a strong wind!";
            objmsg = "$p is blown $T.";
            destob = "$p is blown in from $T.";
            break;
        case PULL_STORM:
            tochar = "The raging storm drives you $T!";
            toroom = "$n is driven $T by the raging storm!";
            destrm = "$n is driven in from $T by a raging storm!";
            objmsg = "$p is blown $T.";
            destob = "$p is blown in from $T.";
            break;
        default:
            if ( pull > 0 ) {
                tochar = "You are pulled $T!";
                toroom = "$n is pulled $T.";
                destrm = "$n is pulled in from $T.";
                objmsg = "$p is pulled $T.";
                objmsg = "$p is pulled in from $T.";
            }
            else {
                tochar = "You are pushed $T!";
                toroom = "$n is pushed $T.";
                destrm = "$n is pushed in from $T.";
                objmsg = "$p is pushed $T.";
                objmsg = "$p is pushed in from $T.";
            }
            break;
    }

    /*
     * Do the moving 
     */
    if ( move ) {
        /*
         * display an appropriate exit message 
         */
        if ( tochar ) {
            act( AT_PLAIN, tochar, ch, NULL, dir_name[xit->vdir], TO_CHAR );
            send_to_char( "\r\n", ch );
        }
        if ( toroom )
            act( AT_PLAIN, toroom, ch, NULL, dir_name[xit->vdir], TO_ROOM );

        /*
         * move the char 
         */
        if ( xit->pulltype == PULL_SLIP )
            return move_char( ch, xit, 1 );
        char_from_room( ch );
        char_to_room( ch, xit->to_room );
        /*
         * move the mount too 
         */
        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, xit->to_room );
            if ( showroom )
                do_look( ch->mount, ( char * ) "auto" );
        }

        /*
         * If they are mounted mount should show up when it auto looks 
         */
        if ( showroom )
            do_look( ch, ( char * ) "auto" );
        /*
         * display an appropriate entrance message 
         */
        /*
         * Only need to send to room really, since char already knows where they were pushed etc... 
         */
        if ( destrm )
            act( AT_PLAIN, destrm, ch, NULL, dtxt, TO_ROOM );
    }

    /*
     * move objects in the room 
     */
    if ( moveobj ) {
        for ( obj = room->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;

            if ( IS_OBJ_STAT( obj, ITEM_BURIED ) || !CAN_WEAR( obj, ITEM_TAKE ) )
                continue;

            resistance = get_obj_weight( obj, FALSE );
            if ( IS_OBJ_STAT( obj, ITEM_METAL ) )
                resistance = ( resistance * 6 ) / 5;
            switch ( obj->item_type ) {
                case ITEM_SCROLL:
                case ITEM_NOTE:
                case ITEM_TRASH:
                    resistance >>= 2;
                    break;
                case ITEM_SCRAPS:
                case ITEM_CONTAINER:
                    resistance >>= 1;
                    break;
                case ITEM_PEN:
                case ITEM_WAND:
                    resistance = ( resistance * 5 ) / 6;
                    break;

                case ITEM_CORPSE_PC:
                case ITEM_CORPSE_NPC:
                case ITEM_FOUNTAIN:
                    resistance <<= 2;
                    break;
            }

            /*
             * is the pull greater than the resistance of the object? 
             */
            if ( ( abs( pull ) * 10 ) > resistance ) {
                if ( objmsg && room->first_person ) {
                    act( AT_PLAIN, objmsg, room->first_person, obj, dir_name[xit->vdir], TO_CHAR );
                    act( AT_PLAIN, objmsg, room->first_person, obj, dir_name[xit->vdir], TO_ROOM );
                }
                if ( destob && xit->to_room->first_person ) {
                    act( AT_PLAIN, destob, xit->to_room->first_person, obj, dtxt, TO_CHAR );
                    act( AT_PLAIN, destob, xit->to_room->first_person, obj, dtxt, TO_ROOM );
                }
                obj_from_room( obj );
                obj_to_room( obj, xit->to_room );
            }
        }
    }

    return rNONE;
}

/*
 * This function bolts a door. Written by Blackmane
 */

void do_bolt( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    EXIT_DATA              *pexit;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Bolt what?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_ISBOLT ) ) {
            send_to_char( "You don't see a bolt.\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_BOLTED ) ) {
            send_to_char( "It's already bolted.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Clunk*\r\n", ch );
            act( AT_ACTION, "$n bolts the $d.", ch, NULL, pexit->keyword, TO_ROOM );
            set_bexit_flag( pexit, EX_BOLTED );
            return;
        }
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
    return;
}

/*
 * This function unbolts a door.  Written by Blackmane
 */

void do_unbolt( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    EXIT_DATA              *pexit;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Unbolt what?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_ISBOLT ) ) {
            send_to_char( "You don't see a bolt.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_BOLTED ) ) {
            send_to_char( "It's already unbolted.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Clunk*\r\n", ch );
            act( AT_ACTION, "$n unbolts the $d.", ch, NULL, pexit->keyword, TO_ROOM );
            remove_bexit_flag( pexit, EX_BOLTED );
            return;
        }
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
    return;
}

short max_holdbreath( CHAR_DATA *ch )
{
    int                     breath;

    switch ( ch->race ) {
        default:
            breath = 60;
            break;

        case RACE_DRAGON:
            breath = 100;
            break;

        case RACE_GNOME:
        case RACE_ELF:
        case RACE_DROW:
        case RACE_HALFLING:
            breath = 40;
            break;

        case RACE_DWARF:
        case RACE_TROLL:
        case RACE_ORC:
        case RACE_OGRE:
            breath = 80;
            break;

        case RACE_PIXIE:
            breath = 30;
            break;
    }

    breath += get_curr_con( ch );
    breath += ch->level;

    if ( breath )
        return breath;
    else
        return 0;
}

bool can_swim( CHAR_DATA *ch )
{
    if ( IS_NPC( ch ) )
        return TRUE;

    if ( ch->move > 0 && ch->hit > 0 ) {
        if ( ch->pcdata->learned[gsn_swim] > number_percent(  ) )
            return TRUE;
    }
    else
        return FALSE;

    if ( !ch->pcdata->learned[gsn_swim] )
        return FALSE;

    if ( ch->weight < ( ch->level * ch->pcdata->learned[gsn_swim] ) )
        return TRUE;

    return FALSE;
}

void fbite_msg( CHAR_DATA *ch, int percentage )
{
    if ( percentage == 0 )
        percentage = 100 * ch->pcdata->frostbite / max_holdbreath( ch );

    if ( percentage > 90 )
        send_to_char( "&WFrost bite:(          &W)&D\r\n", ch );
    else if ( percentage > 80 )
        send_to_char( "&WFrost bite: (&R+         &W)&D\r\n", ch );
    else if ( percentage > 70 )
        send_to_char( "&WFrost bite: (&R++        &W)&D\r\n", ch );
    else if ( percentage > 60 )
        send_to_char( "&WFrost bite: (&R+++       &W)&D\r\n", ch );
    else if ( percentage > 50 )
        send_to_char( "&WFrost bite: (&Y++++      &W)&D\r\n", ch );
    else if ( percentage > 40 )
        send_to_char( "&WFrost bite: (&Y+++++     &W)&D\r\n", ch );
    else if ( percentage > 30 )
        send_to_char( "&WFrost bite: (&Y++++++    &W)&D\r\n", ch );
    else if ( percentage > 20 )
        send_to_char( "&WFrost bite: (&G+++++++   &W)&D\r\n", ch );
    else if ( percentage > 10 )
        send_to_char( "&WFrost bite: (&G++++++++  &W)&D\r\n", ch );
    else if ( percentage > 0 )
        send_to_char( "&WFrost bite: (&G+++++++++ &W)&D\r\n", ch );
    else if ( percentage == 0 )
        send_to_char( "&WFrost bite: (&G++++++++++&W)&D\r\n", ch );

    return;
}

void breath_msg( CHAR_DATA *ch, int percentage )
{
    if ( percentage == 0 )
        percentage = 100 * ch->pcdata->holdbreath / max_holdbreath( ch );

    if ( percentage > 90 )
        send_to_char( "&WBreath left:(          &W)&D\r\n", ch );
    else if ( percentage > 80 )
        send_to_char( "&WBreath left: (&R+         &W)&D\r\n", ch );
    else if ( percentage > 70 )
        send_to_char( "&WBreath left: (&R++        &W)&D\r\n", ch );
    else if ( percentage > 60 )
        send_to_char( "&WBreath left: (&R+++       &W)&D\r\n", ch );
    else if ( percentage > 50 )
        send_to_char( "&WBreath left: (&Y++++      &W)&D\r\n", ch );
    else if ( percentage > 40 )
        send_to_char( "&WBreath left: (&Y+++++     &W)&D\r\n", ch );
    else if ( percentage > 30 )
        send_to_char( "&WBreath left: (&Y++++++    &W)&D\r\n", ch );
    else if ( percentage > 20 )
        send_to_char( "&WBreath left: (&G+++++++   &W)&D\r\n", ch );
    else if ( percentage > 10 )
        send_to_char( "&WBreath left: (&G++++++++  &W)&D\r\n", ch );
    else if ( percentage > 0 )
        send_to_char( "&WBreath left: (&G+++++++++ &W)&D\r\n", ch );
    else if ( percentage == 0 )
        send_to_char( "&WBreath left: (&G++++++++++&W)&D\r\n", ch );

    return;
}

void water_sink( CHAR_DATA *ch, int time )
{
    EXIT_DATA              *pexit;
    OBJ_DATA               *obj;
    ROOM_INDEX_DATA        *room = ch->in_room;

/*  Bug fix - if not already underwater, fly/float will stop sinking.  */
    if ( room->sector_type != SECT_UNDERWATER
         && ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) )
        return;

    for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next ) {
        if ( pexit->to_room
             && ( pexit->to_room->sector_type == SECT_UNDERWATER
                  || pexit->to_room->sector_type == SECT_OCEANFLOOR ) && pexit->vdir == DIR_DOWN ) {
            if ( !can_swim( ch ) && ( number_bits( time ) == 0 ) ) {    /* Chance to sink 
                                                                         */
                act( AT_CYAN, "You sink down into the briny depths!", ch, NULL, NULL, TO_CHAR );
                act( AT_CYAN, "$n can't keep up $s paddle, and slowly sinks into the depths.", ch,
                     NULL, NULL, TO_ROOM );

                if ( pexit->to_room->first_person )
                    act( AT_CYAN, "$n can't keep swimming above, and slowly sinks down before you.",
                         pexit->to_room->first_person, NULL, NULL, TO_ROOM );
                char_from_room( ch );
                char_to_room( ch, pexit->to_room );
                interpret( ch, ( char * ) "look" );

                if ( ch->mount ) {
                    char_from_room( ch->mount );
                    char_to_room( ch->mount, pexit->to_room );
                    interpret( ch->mount, ( char * ) "look" );
                }
            }

/*  Let's deal with sinking objects too  */

            for ( obj = room->first_content; obj; obj = obj->next_content ) {
                if ( number_bits( 3 ) == 0 ) {
                    if ( room->first_person ) {
                        act( AT_CYAN, "$p slowly sinks down into the depths.", room->first_person,
                             obj, NULL, TO_CHAR );
                        act( AT_CYAN, "$p slowly sinks down into the depths.", room->first_person,
                             obj, NULL, TO_ROOM );
                    }
                    if ( pexit->to_room->first_person ) {
                        act( AT_CYAN, "$p sinks down from above.", pexit->to_room->first_person,
                             obj, NULL, TO_CHAR );
                        act( AT_CYAN, "$p sinks down from above.", pexit->to_room->first_person,
                             obj, NULL, TO_ROOM );
                    }
                    obj_from_room( obj );
                    obj_to_room( obj, pexit->to_room );
                }
            }
        }
    }
    return;
}

void swim_check( CHAR_DATA *ch, int time )
{
    int                     maxbreath = max_holdbreath( ch );
    int                     percentage = 100 * ch->pcdata->holdbreath / maxbreath;
    OBJ_DATA               *obj;
    int                     dam;
    short                   chance;

/*  Mobs can't drown - yet! Nor immortals. */
    if ( IS_NPC( ch ) )
        return;

    chance = number_range( 1, 100 );

/*  Players are swimming or above the surface of the water. They may also have a boat.  */

    if ( ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_OCEAN
         || ch->in_room->sector_type == SECT_LAKE || ch->in_room->sector_type == SECT_RIVER ) {
        if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
            if ( chance == 3 ) {
                send_to_char( "&CYou float above the surface of the water.&D\r\n", ch );
            }
            return;
        }

        for ( obj = ch->first_carrying; obj; obj = obj->next_content )
            if ( obj->item_type == ITEM_BOAT )
                break;

        if ( obj ) {
            if ( number_bits( 4 ) == 0 )
                send_to_char( "&CYou paddle on the water in your boat.&D\r\n", ch );
            return;
        }

        /*
         * Check swim or sink. No damage until underwater. Aqua breath will reduce damage 
         * to 0.  
         */

        int                     mov;
        int                     swim = ch->pcdata->learned[gsn_swim] - number_range( 1,
                                                                                     ( ch->pcdata->
                                                                                       learned
                                                                                       [gsn_swim] /
                                                                                       2 ) );

        if ( ch->move > 0 ) {                          /* With moves, player can swim or
                                                        * * struggle to swim */
            mov = number_range( ch->max_move - 5, ch->max_move - 15 );
            mov = UMAX( 1, mov );

            if ( ch->position == POS_SLEEPING && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
                send_to_char( "&RToo exhausted to swim, you choke on a mouthful of water!&D\r\n",
                              ch );
                dam = number_range( ch->max_hit / 20, ch->max_hit / 15 );
                dam = UMAX( 1, dam );
                damage( ch, ch, dam, TYPE_UNDEFINED );
                ch->move += 5;
                return;
            }
            if ( swim > 90 ) {
                mov /= 10;
                if ( number_bits( 2 ) == 0 )
                    switch ( number_range( 1, 3 ) ) {
                        default:
                            break;

                        case 1:
                            send_to_char
                                ( "&CYou roll over onto your back and switch to backstroke.&D\r\n",
                                  ch );
                            learn_from_success( ch, gsn_swim );
                            break;

                        case 2:
                            send_to_char
                                ( "&CWith both arms in front of you, you confidently launch into a butterfly stroke.&D\r\n",
                                  ch );
                            learn_from_success( ch, gsn_swim );
                            break;

                        case 3:
                            send_to_char
                                ( "&CKicking gracefully, you breaststroke through the water.&D\r\n",
                                  ch );
                            learn_from_success( ch, gsn_swim );
                            break;
                    }
            }
            else if ( swim > 70 ) {
                mov /= 8;
                if ( number_bits( 3 ) == 0 ) {
                    send_to_char
                        ( "&CYou paddle through the water confidently, with even strokes.&D\r\n",
                          ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 50 ) {
                mov /= 6;
                if ( number_bits( 3 ) == 0 ) {
                    send_to_char( "&CYou swim fairly well through the water.&D\r\n", ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 30 ) {
                mov /= 4;
                if ( number_bits( 3 ) == 0 ) {
                    send_to_char( "&CYou manage to swim quite decently.&D\r\n", ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 10 ) {
                mov /= 3;
                if ( number_bits( 2 ) == 0 ) {
                    send_to_char
                        ( "&CYou are a little clumsy, but manage to paddle evenly enough in the water.&D\r\n",
                          ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 0 ) {
                mov /= 2;
                if ( number_bits( 2 ) == 0 ) {
                    send_to_char
                        ( "&CYou struggle but manage to keep your head above the surface, barely.&D\r\n",
                          ch );
                    learn_from_failure( ch, gsn_swim );
                }
            }
            else {                                     /* Eep - drowning! */

                if ( number_bits( 2 ) == 0 ) {
                    send_to_char( "&CYou splash furiously but produce little more than foam!&D\r\n",
                                  ch );
                    learn_from_failure( ch, gsn_swim );
                }
            }
            mov = number_range( 5, 15 );

            if ( ch->race == RACE_DRAGON && ch->Class == CLASS_BLUE ) {
                mov /= 2;
            }

            if ( ch->morph != NULL && !str_cmp( ch->morph->morph->name, "fish" ) ) {
                mov /= 2;
                mov /= 2;
            }

            if ( ch->move - mov < 0 )
                ch->move = 0;
            else
                ch->move -= mov;
        }
        else {                                         /* No moves, start damaging hp * * 
                                                        * (only a little - more chance to * * *
                                                        * sink) */
            int                     inroom = ch->in_room->vnum;

            water_sink( ch, 0 );

            if ( IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
                if ( number_bits( 3 ) == 0 )
                    send_to_char
                        ( "&CYou panic as you sink underwater, but manage to breathe under it easily enough.&D\r\n",
                          ch );
                return;
            }

            if ( ch->in_room->vnum == inroom ) {       /* Character didn't sink */
                if ( number_bits( 1 ) == 0 )
                    send_to_char
                        ( "&RToo exhausted to swim, you choke on a mouthful of water!&D\r\n", ch );
                dam = number_range( ch->max_hit / 20, ch->max_hit / 15 );
                dam = UMAX( 1, dam );
                damage( ch, ch, dam, TYPE_UNDEFINED );
                ch->move += 5;
            }
            else
                send_to_char
                    ( "&CToo exhausted to swim, you hold your breath and &Rsink&C underwater!&D\r\n",
                      ch );

            return;
        }
    }
    else {                                             /* Players are underwater (and * * 
                                                        * drowning). Swim has no affect. */

        if ( IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
            if ( number_bits( 5 ) == 0 )
                send_to_char( "&CYou breathe easily underwater.&D\r\n", ch );
            return;
        }

        if ( ch->pcdata->holdbreath >= maxbreath ) {
            ch->pcdata->holdbreath = maxbreath;
            if ( number_bits( 1 ) == 0 )
                water_sink( ch, 1 );
            send_to_char( "&CYou choke on a mouthful of water! &RYOU ARE DROWNING!&D\r\n", ch );
            dam = number_range( ch->max_hit / 20, ch->max_hit / 10 );
            damage( ch, ch, dam, TYPE_UNDEFINED );
        }

        if ( ( percentage > 50 && number_bits( 1 ) == 0 ) || number_bits( 2 ) == 0 ) {
            breath_msg( ch, percentage );

            if ( number_bits( 2 ) == 0 ) {
                if ( percentage > 90 )
                    send_to_char( "&RYou need to surface for air!!&D\r\n", ch );
                else if ( percentage > 80 )
                    send_to_char( "&CYou need to surface for air!&D\r\n", ch );
                else if ( percentage > 70 )
                    send_to_char( "&CYou need to surface for air.&D\r\n", ch );
                else if ( percentage > 20 )
                    send_to_char
                        ( "&CYou are starting to tire, and will need to surface soon.&D\r\n", ch );
                else if ( percentage == 0 )
                    send_to_char( "&CYou have plenty of air left.\r\n", ch );
            }
        }
    }
    return;
}

void hint_update(  )
{
    DESCRIPTOR_DATA        *d;

    if ( time_info.hour % 1 == 0 ) {
        for ( d = first_descriptor; d; d = d->next ) {
            if ( d->connected == CON_PLAYING && IS_AWAKE( d->character ) && d->character->pcdata ) {
                if ( IS_SET( d->character->pcdata->flags, PCFLAG_HINTS ) && number_bits( 1 ) == 0 ) {
                    if ( d->character->level > 5 )
                        REMOVE_BIT( d->character->pcdata->flags, PCFLAG_HINTS );
                    else
                        ch_printf_color( d->character, "\r\n&Chint: &c%s\r\n",
                                         get_hint( d->character->level ) );
                }
            }
        }
    }
    return;
}
