/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Tracking/hunting module                                               *
 ***************************************************************************/

#include "h/mud.h"
#include "h/track.h"

#define TRACK_THROUGH_DOORS

extern short            top_room;

/* You can define or not define TRACK_THOUGH_DOORS, above, depending on
 * whether or not you want track to find paths which lead through closed
 * or hidden doors.
 */

CHAR_DATA              *get_char_area( CHAR_DATA *ch, char *argument );

ROOM_INDEX_DATA        *get_landmark_room( char *lname );
void                    show_landmark_list( CHAR_DATA *ch );

typedef struct bfs_queue_struct BFS_DATA;
struct bfs_queue_struct
{
    ROOM_INDEX_DATA        *room;
    char                    dir;
    BFS_DATA               *next;
};

static BFS_DATA        *queue_head = NULL,
    *queue_tail = NULL,
    *room_queue = NULL;

/* Utility macros */
#define MARK(room)  (SET_BIT((room)->room_flags, BFS_MARK))
#define UNMARK(room)  (REMOVE_BIT((room)->room_flags, BFS_MARK))
#define IS_MARKED(room)  (IS_SET((room)->room_flags, BFS_MARK))

bool valid_edge( EXIT_DATA *pexit )
{
    if ( pexit->to_room
#ifndef TRACK_THROUGH_DOORS
         && !IS_SET( pexit->exit_info, EX_CLOSED )
#endif
         && !IS_MARKED( pexit->to_room ) )
        return TRUE;
    else
        return FALSE;
}

void bfs_enqueue( ROOM_INDEX_DATA *room, char dir )
{
    BFS_DATA               *curr;

    curr = ( BFS_DATA * ) malloc( sizeof( BFS_DATA ) );
    curr->room = room;
    curr->dir = dir;
    curr->next = NULL;

    if ( queue_tail ) {
        queue_tail->next = curr;
        queue_tail = curr;
    }
    else
        queue_head = queue_tail = curr;
}

void bfs_dequeue( void )
{
    BFS_DATA               *curr;

    curr = queue_head;

    if ( !( queue_head = queue_head->next ) )
        queue_tail = NULL;
    free( curr );
}

void bfs_clear_queue( void )
{
    while ( queue_head )
        bfs_dequeue(  );
}

void room_enqueue( ROOM_INDEX_DATA *room )
{
    BFS_DATA               *curr;

    curr = ( BFS_DATA * ) malloc( sizeof( BFS_DATA ) );
    curr->room = room;
    curr->next = room_queue;

    room_queue = curr;
}

void clean_room_queue( void )
{
    BFS_DATA               *curr,
                           *curr_next;

    for ( curr = room_queue; curr; curr = curr_next ) {
        UNMARK( curr->room );
        curr_next = curr->next;
        free( curr );
    }
    room_queue = NULL;
}

int find_first_step( ROOM_INDEX_DATA *src, ROOM_INDEX_DATA *target, int maxdist )
{
    int                     curr_dir,
                            count;
    EXIT_DATA              *pexit;

    if ( !src || !target ) {
        bug( "%s", "Illegal value passed to find_first_step (track.c)" );
        return BFS_ERROR;
    }

    if ( src == target )
        return BFS_ALREADY_THERE;

    if ( src->area != target->area )
        return BFS_NO_PATH;

    room_enqueue( src );
    MARK( src );

    /*
     * first, enqueue the first steps, saving which direction we're going. 
     */
    for ( pexit = src->first_exit; pexit; pexit = pexit->next ) {
        if ( valid_edge( pexit ) ) {
            curr_dir = pexit->vdir;
            MARK( pexit->to_room );
            room_enqueue( pexit->to_room );
            bfs_enqueue( pexit->to_room, curr_dir );
        }
    }

    count = 0;
    while ( queue_head ) {
        if ( ++count > maxdist ) {
            bfs_clear_queue(  );
            clean_room_queue(  );
            return BFS_NO_PATH;
        }
        if ( queue_head->room == target ) {
            curr_dir = queue_head->dir;
            bfs_clear_queue(  );
            clean_room_queue(  );
            return curr_dir;
        }
        else {
            for ( pexit = queue_head->room->first_exit; pexit; pexit = pexit->next ) {
                if ( valid_edge( pexit ) ) {
                    curr_dir = pexit->vdir;
                    MARK( pexit->to_room );
                    room_enqueue( pexit->to_room );
                    bfs_enqueue( pexit->to_room, queue_head->dir );
                }
            }
            bfs_dequeue(  );
        }
    }
    clean_room_queue(  );

    return BFS_NO_PATH;
}

void do_track( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vict;
    char                    arg[MIL],
                            fpath[MSL * 2];
    int                     dir,
                            maxdist,
                            lastdir = 0,
        lastnumber = 0;
    bool                    firststep = true;

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_track] <= 0 ) {
        send_to_char( "You do not know of this skill yet.\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg == NULL || arg[0] == '\0' ) {
        send_to_char( "Whom are you trying to track?\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_track]->beats );

    if ( !( vict = get_char_area( ch, arg ) ) ) {
        send_to_char( "You can't find a trail of anyone like that.\r\n", ch );
        return;
    }
    maxdist = 100 + ch->level * 30;

    if ( !IS_NPC( ch ) )
        maxdist = ( maxdist * LEARNED( ch, gsn_track ) ) / 100;

    /*
     * Ok I want it to give a nice quick overview to how to get there 
     */
    {
        ROOM_INDEX_DATA        *fromroom = ch->in_room;
        int                     steps = 0,
            maxsteps;

        fpath[0] = '\0';

        maxsteps = URANGE( 1, ( 1 + ( ch->level / 10 ) ), 10 );

        for ( steps = 0; steps <= maxsteps; steps++ ) {
            dir = find_first_step( fromroom, vict->in_room, maxdist );
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
                    send_to_char( "You can't sense a trail from here.\r\n", ch );
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

    ch_printf( ch, "&cYou notice signs of someone passing that lead %s from here...\r\n", fpath );
    learn_from_success( ch, gsn_track );
}

void found_prey( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char                    buf[MSL];
    char                    victname[MSL];

    if ( victim == NULL ) {
        bug( "%s", "Found_prey: null victim" );
        return;
    }

    if ( victim->in_room == NULL ) {
        bug( "%s", "Found_prey: null victim->in_room" );
        return;
    }

    snprintf( victname, MSL, "%s", IS_NPC( victim ) ? victim->short_descr : victim->name );
    if ( !can_see( ch, victim ) ) {
        if ( number_percent(  ) < 90 )
            return;
        switch ( number_bits( 2 ) ) {
            case 0:
                snprintf( buf, MSL, "say Don't make me find you, %s!", victname );
                interpret( ch, buf );
                break;
            case 1:
                act( AT_ACTION, "$n sniffs around the room for $N.", ch, NULL, victim, TO_NOTVICT );
                act( AT_ACTION, "You sniff around the room for $N.", ch, NULL, victim, TO_CHAR );
                act( AT_ACTION, "$n sniffs around the room for you.", ch, NULL, victim, TO_VICT );
                interpret( ch, ( char * ) "say I can smell your blood!" );
                break;
            case 2:
                snprintf( buf, MSL, "yell I'm going to tear %s apart!", victname );
                interpret( ch, buf );
                break;
            case 3:
                interpret( ch, ( char * ) "say Just wait until I find you..." );
                break;
        }
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        if ( number_percent(  ) < 90 )
            return;
        switch ( number_bits( 2 ) ) {
            case 0:
                interpret( ch, ( char * ) "say C'mon out, you coward!" );
                snprintf( buf, MSL, "yell %s is a bloody coward!", victname );
                interpret( ch, buf );
                break;
            case 1:
                snprintf( buf, MSL, "say Let's take this outside, %s", victname );
                interpret( ch, buf );
                break;
            case 2:
                snprintf( buf, MSL, "yell %s is a yellow-bellied wimp!", victname );
                interpret( ch, buf );
                break;
            case 3:
                act( AT_ACTION, "$n takes a few swipes at $N.", ch, NULL, victim, TO_NOTVICT );
                act( AT_ACTION, "You try to take a few swipes $N.", ch, NULL, victim, TO_CHAR );
                act( AT_ACTION, "$n takes a few swipes at you.", ch, NULL, victim, TO_VICT );
                break;
        }
        return;
    }

    switch ( number_bits( 2 ) ) {
        case 0:
            snprintf( buf, MSL, "yell Your blood is mine, %s!", victname );
            interpret( ch, buf );
            break;
        case 1:
            snprintf( buf, MSL, "say Alas, we meet again, %s!", victname );
            interpret( ch, buf );
            break;
        case 2:
            snprintf( buf, MSL, "say What do you want on your tombstone, %s?", victname );
            interpret( ch, buf );
            break;
        case 3:
            act( AT_ACTION, "$n lunges at $N from out of nowhere!", ch, NULL, victim, TO_NOTVICT );
            act( AT_ACTION, "You lunge at $N catching $M off guard!", ch, NULL, victim, TO_CHAR );
            act( AT_ACTION, "$n lunges at you from out of nowhere!", ch, NULL, victim, TO_VICT );
    }
    stop_hunting( ch );
    set_fighting( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

void hunt_victim( CHAR_DATA *ch )
{
    bool                    found;
    CHAR_DATA              *tmp;
    EXIT_DATA              *pexit;
    short                   ret;

    if ( !ch || !ch->hunting || ch->position < 5 )
        return;

    /*
     * make sure the char still exists 
     */
    for ( found = FALSE, tmp = first_char; tmp && !found; tmp = tmp->next )
        if ( ch->hunting->who == tmp )
            found = TRUE;

    if ( !found ) {
        do_tell( ch, ( char * ) "Damn!  My prey is gone!!" );
        stop_hunting( ch );
        return;
    }

    if ( ch->in_room == ch->hunting->who->in_room ) {
        if ( ch->fighting )
            return;
        found_prey( ch, ch->hunting->who );
        return;
    }

    ret = find_first_step( ch->in_room, ch->hunting->who->in_room, 500 + ch->level * 25 );
    if ( ret < 0 ) {
        do_tell( ch, ( char * ) "Damn!  Lost my prey!" );
        stop_hunting( ch );
        return;
    }
    else {
        if ( ( pexit = get_exit( ch->in_room, ret ) ) == NULL ) {
            bug( "%s", "Hunt_victim: lost exit?" );
            return;
        }
        move_char( ch, pexit, FALSE );

        /*
         * Crash bug fix by Shaddai 
         */
        if ( char_died( ch ) )
            return;

        if ( !ch->hunting ) {
            if ( !ch->in_room ) {
                bug( "Hunt_victim: no ch->in_room!  Mob #%d, name: %s.  Placing mob in limbo.",
                     ch->pIndexData->vnum, ch->name );
                char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
                return;
            }
            do_tell( ch, ( char * ) "Damn!  Lost my prey!" );
            return;
        }
        if ( ch->in_room == ch->hunting->who->in_room )
            found_prey( ch, ch->hunting->who );
        else {
            CHAR_DATA              *vch;

            /*
             * perform a ranged attack if possible 
             * Changed who to name as scan_for_victim expects the name and
             * Not the char struct. --Shaddai
             */
            if ( ( vch = scan_for_victim( ch, pexit, ch->hunting->name ) ) != NULL ) {
                if ( !mob_fire( ch, ch->hunting->who->name ) ) {
                    /*
                     * ranged spell attacks go here 
                     */
                }
            }
        }
        return;
    }
}

void do_landmark( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *room;
    int                     dir;

    if ( IS_NPC( ch ) )
        return;

    /*
     * Give a list of valid landmarks 
     */
    if ( !VLD_STR( argument ) ) {
        show_landmark_list( ch );
        send_to_char( "\r\nType landmark <name>\r\n", ch );
        send_to_char( "     landmark clear\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "clear" ) ) {
        send_to_char( "You are no longer looking at that landmark.\r\n", ch );
        if ( ch->landmark )
            STRFREE( ch->landmark );
        return;
    }

    // Make sure players can always find their hometowns
    if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "ywild.are" ) ) {
        if ( !str_cmp( argument, "paleon city" ) ) {
            send_to_char
                ( "You are too far north to see the landmark..\r\nYou remember heading south should get you back to paleon city.\r\n",
                  ch );
            return;
        }
        else if ( !str_cmp( argument, "forbidden city" ) ) {
            send_to_char
                ( "You are too far north to see the landmark..\r\nYou remember heading south should get you back to paleon city.\r\n",
                  ch );
            return;
        }
        else if ( !str_cmp( argument, "kirwood swamp" ) ) {
            send_to_char
                ( "You are too far north to see the landmark..\r\nYou remember heading south should get you back to paleon city.\r\n",
                  ch );
            return;
        }
    }

    // Make sure players can always find their hometowns
    if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "xwild.are" ) ) {
        if ( !str_cmp( argument, "paleon city" ) ) {
            send_to_char
                ( "You are too far north to see the landmark..\r\nYou remember heading north should get you back to paleon city.\r\n",
                  ch );
            return;
        }
        else if ( !str_cmp( argument, "forbidden city" ) ) {
            send_to_char
                ( "You are too far north to see the landmark..\r\nYou remember heading north should get you back to paleon city.\r\n",
                  ch );
            return;
        }
        else if ( !str_cmp( argument, "kirwood swamp" ) ) {
            send_to_char
                ( "You are too far north to see the landmark..\r\nYou remember heading north should get you back to paleon city.\r\n",
                  ch );
            return;
        }
    }

    if ( !( room = get_landmark_room( argument ) ) ) {
        send_to_char( "You can't find any such landmark.\n\r", ch );
        return;
    }

    ch->landmark = STRALLOC( argument );               /* Setup their landmark so it can
                                                        * be auto done */

    /*
     * The # here is how far it checks 
     */
    dir = find_first_step( ch->in_room, room, 11500 );

    switch ( dir ) {
        case BFS_ERROR:
            send_to_char( "&cHmm... something seems to be wrong.&D\n\r", ch );
            break;

        case BFS_ALREADY_THERE:
            send_to_char( "\r\n&cYou're at your landmark! and can now type &WEXPLORE&c to enter.&D",
                          ch );
            if ( ch->landmark )
                STRFREE( ch->landmark );
            break;

        case BFS_NO_PATH:
            send_to_char( "&cThat landmark is too far to see from here.&D\n\r", ch );
            break;

        default:
            ch_printf( ch, "\r\n&cHead &C%s&c to get to %s.\n\r", dir_name[dir], argument );
            break;
    }
}
