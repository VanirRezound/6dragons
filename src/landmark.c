#include <limits.h>
#include <string.h>
#include "h/files.h"
#include "h/key.h"
#include "h/landmark.h"
#include "h/mud.h"

LANDMARK_DATA          *first_landmark;
LANDMARK_DATA          *last_landmark;

void add_landmark( LANDMARK_DATA * landmark )
{
    ROOM_INDEX_DATA        *room;

    if ( !landmark )
        return;
    LINK( landmark, first_landmark, last_landmark, next, prev );

    /*
     * Lets set what area it belongs to based on the room 
     */
    if ( ( room = get_room_index( landmark->vnum ) ) )
        landmark->area = room->area;
}

void unlink_landmark( LANDMARK_DATA * landmark )
{
    if ( !landmark )
        return;
    UNLINK( landmark, first_landmark, last_landmark, next, prev );
}

void free_landmark( LANDMARK_DATA * landmark )
{
    if ( !landmark )
        return;
    STRFREE( landmark->name );
    landmark->area = NULL;
    DISPOSE( landmark );
}

void free_all_landmarks( void )
{
    LANDMARK_DATA          *landmark,
                           *landmark_next;

    for ( landmark = first_landmark; landmark; landmark = landmark_next ) {
        landmark_next = landmark->next;
        unlink_landmark( landmark );
        free_landmark( landmark );
    }
}

void fread_landmark( FILE * fp )
{
    LANDMARK_DATA          *landmark;
    const char             *word;
    bool                    fMatch;

    CREATE( landmark, LANDMARK_DATA, 1 );

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !landmark->name ) {
                        bug( "%s: Name not found", __FUNCTION__ );
                        free_landmark( landmark );
                        return;
                    }
                    add_landmark( landmark );
                    return;
                }
                break;

            case 'N':
                KEY( "Name", landmark->name, fread_string( fp ) );
                break;

            case 'V':
                KEY( "Vnum", landmark->vnum, fread_number( fp ) );
                break;
        }
        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }

    bug( "%s: End not found", __FUNCTION__ );
    free_landmark( landmark );
}

void load_landmarks( void )
{
    FILE                   *fp;

    if ( !( fp = FileOpen( LANDMARK_FILE, "r" ) ) ) {
        bug( "%s: Can't open %s for read.", __FUNCTION__, LANDMARK_FILE );
        perror( LANDMARK_FILE );
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
            bug( "%s: # not found.", __FUNCTION__ );
            break;
        }

        word = fread_word( fp );
        if ( !str_cmp( word, "LANDMARK" ) ) {
            fread_landmark( fp );
            continue;
        }
        else if ( !str_cmp( word, "END" ) )
            break;
        else {
            bug( "%s: bad section %s.", __FUNCTION__, word );
            continue;
        }
    }
    FileClose( fp );
}

void save_landmarks( void )
{
    FILE                   *fpout;
    LANDMARK_DATA          *landmark;

    if ( !( fpout = FileOpen( LANDMARK_FILE, "w" ) ) ) {
        bug( "%s: can't open %s for writting.", __FUNCTION__, LANDMARK_FILE );
        perror( LANDMARK_FILE );
        return;
    }

    for ( landmark = first_landmark; landmark; landmark = landmark->next ) {
        if ( !VLD_STR( landmark->name ) ) {
            bug( "%s: NULL name for room vnum %d...not saving.", __FUNCTION__, landmark->vnum );
            continue;
        }

        fprintf( fpout, "#LANDMARK\n" );
        fprintf( fpout, "Name   %s~\n", landmark->name );
        fprintf( fpout, "Vnum   %d\n", landmark->vnum );
        fprintf( fpout, "End\n\n" );
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

/* Lookup landmark by name */
LANDMARK_DATA          *find_landmark( char *lname )
{
    LANDMARK_DATA          *landmark;

    if ( !VLD_STR( lname ) )
        return NULL;

    for ( landmark = first_landmark; landmark; landmark = landmark->next ) {
        if ( VLD_STR( landmark->name ) && !str_cmp( landmark->name, lname ) )
            return landmark;
    }
    return NULL;
}

ROOM_INDEX_DATA        *get_landmark_room( char *lname )
{
    LANDMARK_DATA          *landmark;

    if ( !( landmark = find_landmark( lname ) ) )
        return NULL;
    return ( get_room_index( landmark->vnum ) );
}

void show_landmark_list( CHAR_DATA *ch )
{
    LANDMARK_DATA          *landmark;

    if ( !first_landmark ) {
        send_to_char( "No landmarks currently.\r\n", ch );
        return;
    }
    int                     count,
                            scount;

    count = 0;
    scount = 0;
    send_to_char( "You can see the following landmarks:\r\n", ch );
    for ( landmark = first_landmark; landmark; landmark = landmark->next ) {
        /*
         * Show full list to Staff only landmarks for that area to players 
         */
        if ( !IS_IMMORTAL( ch ) && ch->in_room && ch->in_room->area != landmark->area )
            continue;
        if ( !IS_IMMORTAL( ch ) ) {
            ch_printf( ch, "%-20s   ", landmark->name );
            count++;
            if ( count == 3 ) {
                send_to_char( "\r\n", ch );
                count = 0;
            }
        }
        if ( IS_IMMORTAL( ch ) ) {
            ch_printf( ch, "%-20s ", landmark->name );
            ch_printf( ch, "[%-10s ]   ",
                       ( landmark->area
                         && VLD_STR( landmark->area->filename ) ) ? landmark->area->
                       filename : "Unknown" );
            scount++;
            if ( scount == 2 ) {
                send_to_char( "\r\n", ch );
                scount = 0;
            }
        }
    }
}

/* Allow a staff member to add a landmark */
void do_addlandmark( CHAR_DATA *ch, char *argument )
{
    LANDMARK_DATA          *landmark;
    char                    arg[MSL];
    int                     vnum = 0;

    argument = one_argument( argument, arg );
    if ( !VLD_STR( arg ) || !is_number( argument ) ) {
        send_to_char( "Syntax: addlandmark <name> <vnum>\r\n", ch );
        return;
    }
    vnum = atoi( argument );
    if ( !get_room_index( vnum ) ) {
        send_to_char( "Not a valid room vnum.\r\n", ch );
        return;
    }
    if ( find_landmark( arg ) ) {
        send_to_char( "There is already a landmark using that name.\r\n", ch );
        return;
    }
    CREATE( landmark, LANDMARK_DATA, 1 );
    landmark->name = STRALLOC( arg );
    landmark->vnum = vnum;
    add_landmark( landmark );
    save_landmarks(  );
    ch_printf( ch, "Landmark (%s) created using vnum %d.\r\n", landmark->name, landmark->vnum );
}

void do_removelandmark( CHAR_DATA *ch, char *argument )
{
    LANDMARK_DATA          *landmark;

    if ( !VLD_STR( argument ) ) {
        send_to_char( "Syntax: removelandmark <landmark>\r\n", ch );
        return;
    }

    if ( !( landmark = find_landmark( argument ) ) ) {
        send_to_char( "There is no landmark by that name.\r\n", ch );
        return;
    }

    unlink_landmark( landmark );
    free_landmark( landmark );
    save_landmarks(  );
    send_to_char( "That landmark has been removed.\r\n", ch );
}

/* Allow a Staff member to set a landmark */
void do_setlandmark( CHAR_DATA *ch, char *argument )
{
    LANDMARK_DATA          *landmark;
    char                    arg[MSL],
                            arg2[MSL];

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( !VLD_STR( arg ) || !VLD_STR( arg2 ) || !VLD_STR( argument ) ) {
        send_to_char( "Syntax: setlandmark <landmark> <name/vnum> <value>.\r\n", ch );
        return;
    }

    if ( !( landmark = find_landmark( arg ) ) ) {
        send_to_char( "There is no such landmark to edit.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        ch_printf( ch, "Changed %s to", landmark->name );
        STRFREE( landmark->name );
        landmark->name = STRALLOC( argument );
        ch_printf( ch, " %s", landmark->name );
        save_landmarks(  );
        return;
    }
    else if ( !str_cmp( arg2, "vnum" ) ) {
        int                     vnum = atoi( argument );

        if ( !get_room_index( vnum ) ) {
            ch_printf( ch, "%s isn't a valid room vnum.\r\n", argument );
            return;
        }
        landmark->vnum = vnum;
        ch_printf( ch, "%s's vnum is now %d.\r\n", landmark->name, landmark->vnum );
        save_landmarks(  );
        return;
    }

    do_setlandmark( ch, ( char * ) "" );
}
