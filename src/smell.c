/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - smelling/hunting module                                               *
 ***************************************************************************/

#include "h/mud.h"
#include "h/smell.h"

#define BFS_ERROR     -1
#define BFS_ALREADY_THERE  -2
#define BFS_NO_PATH     -3
#define BFS_MARK    536870912

#define SMELL_THROUGH_DOORS

extern short            top_room;

/* You can define or not define smell_THOUGH_DOORS, above, depending on
 * whether or not you want smell to find paths which lead through closed
 * or hidden doors.
 */

bool                    valid_edge( EXIT_DATA *pexit );
void                    clean_room_queue( void );
void                    room_enqueue( ROOM_INDEX_DATA *room );
void                    bfs_enqueue( ROOM_INDEX_DATA *room, char dir );
void                    bfs_dequeue( void );
void                    bfs_clear_queue( void );
int                     find_first_step( ROOM_INDEX_DATA *src, ROOM_INDEX_DATA *target,
                                         int maxdist );

typedef struct bfs_queue_struct BFS_DATA;
struct bfs_queue_struct
{
    ROOM_INDEX_DATA        *room;
    char                    dir;
    BFS_DATA               *next;
};

/* Utility macros */
#define MARK(room)  (SET_BIT((room)->room_flags, BFS_MARK))
#define UNMARK(room)  (REMOVE_BIT((room)->room_flags, BFS_MARK))
#define IS_MARKED(room)  (IS_SET((room)->room_flags, BFS_MARK))

void do_smell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vict;
    char                    arg[MIL],
                            fpath[MSL * 2];
    int                     dir,
                            maxdist,
                            lastdir = 0,
        lastnumber = 0;
    bool                    firststep = true;

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_smell] <= 0 ) {
        send_to_char( "You do not know of this skill yet.\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg == NULL || arg[0] == '\0' ) {
        send_to_char( "Whom are you trying to catch the scent of?\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_smell]->beats );

    if ( !( vict = get_char_area( ch, arg ) ) ) {
        send_to_char( "You can't catch the scent of anyone like that.\r\n", ch );
        return;
    }
    maxdist = 100 + ch->level * 30;
    if ( !IS_NPC( ch ) )
        maxdist = ( maxdist * LEARNED( ch, gsn_smell ) ) / 100;

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
                    send_to_char( "You can't catch the scent from here.\r\n", ch );
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
             * Ok now for the fun stuff we want to get 
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

    ch_printf( ch, "&cThe scent leads you %s from here...\r\n", fpath );
    learn_from_success( ch, gsn_smell );
}
