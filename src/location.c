/***************************************************************************
 * This code may be used freely within any non-commercial MUD, all I ask   *
 * is that these comments remain in tact and that you give me any feedback *
 * or bug reports you come up with.                                        *
 *                                  -- Midboss (eclipsing.souls@gmail.com) *
 ***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "h/mud.h"
#include "h/files.h"

LOCATION               *first_location;
LOCATION               *last_location;

/*
 * Syntax: location
 * Syntax: location add <name> <vnum>
 * Syntax: location delete <name>
 * Syntax: location save
 */
void do_locations( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            arg2[MIL],
                            arg3[MIL];
    LOCATION               *pLoc;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    // Are we looking for a list?
    if ( arg[0] == '\0' || !str_prefix( arg, "list" ) ) {
        bool                    found = FALSE;
        char                    buf[MSL];

        // Initialize the buffer...
        buf[0] = '\0';

        // Header.
        sprintf( buf + strlen( buf ), "You may goto the following pre-defined points:\r\n" );

        // Let's build a list.
        for ( pLoc = first_location; pLoc; pLoc = pLoc->next ) {
            // Set it to true.
            if ( !found )
                found = TRUE;

            // Add it to the buffer.
            sprintf( buf + strlen( buf ), "%-20s [room %d]\r\n", pLoc->name, pLoc->vnum );
        }
        send_to_char( "&cSyntax: location&D\r\n", ch );
        send_to_char( "&cSyntax: location add <&Cname&c> <&Cvnum&c>&D\r\n", ch );
        send_to_char( "&cSyntax: location delete <&Cname&c>&D\r\n", ch );
        send_to_char( "&cSyntax: location save&D\r\r\n\n", ch );
        // None found?
        if ( !found )
            send_to_char( "You may not use any pre-defined goto points.\r\n", ch );
        // Send the buffer.
        else
            send_to_char( buf, ch );
        return;
    }
    else if ( !str_prefix( arg, "save" ) )
        save_locations(  );
    // Creating one?
    else if ( !str_prefix( arg, "add" ) || !str_prefix( arg, "create" ) ) {
        ROOM_INDEX_DATA        *pRoom;
        int                     vnum;

        // Not just anyone can do this.
        if ( get_trust( ch ) < LEVEL_AJ_GENERAL ) {
            send_to_char( "You're not authorized to do that.\r\n", ch );
            return;
        }

        // We have a name, right?
        if ( arg2[0] == '\0' ) {
            send_to_char( "You need to provide a name.\r\n", ch );
            return;
        }

        // Idiot-proofing.
        if ( strlen( arg2 ) > 20 || strlen( arg2 ) < 3 ) {
            send_to_char( "That name is either too long or too short.\r\n", ch );
            return;
        }

        // Make sure we have a vnum.
        if ( arg3[0] == '\0' || !is_number( arg3 ) ) {
            send_to_char( "You need to provide a room vnum.\r\n", ch );
            return;
        }

        // Find out if the room exists.
        vnum = atoi( arg3 );

        if ( ( pRoom = get_room_index( vnum ) ) == NULL ) {
            send_to_char( "That room doesn't exist.\r\n", ch );
            return;
        }

        // Create the location and stick it in the list.
        pLoc = new_loc(  );
        STRFREE( pLoc->name );
        pLoc->name = str_dup( arg2 );
        pLoc->vnum = vnum;
        LINK( pLoc, first_location, last_location, next, prev );
        return;
    }
    else if ( !str_prefix( arg, "delete" ) ) {
        LOCATION               *list,
                               *next_list;

        // Can we do it?
        if ( get_trust( ch ) < LEVEL_AJ_GENERAL ) {
            send_to_char( "You're not authorized to do that.\r\n", ch );
            return;
        }

        // No arg?
        if ( arg2[0] == '\0' ) {
            send_to_char( "Which goto point do you want to delete?\r\n", ch );
            return;
        }

        // Make sure it's NULL, then try to find one.
        pLoc = NULL;

        for ( list = first_location; list; list = list->next ) {
            if ( !str_prefix( arg2, list->name ) ) {
                pLoc = list;
                break;
            }
        }

        // No location?
        if ( pLoc == NULL ) {
            send_to_char( "No such goto point.\r\n", ch );
            return;
        }

        UNLINK( pLoc, first_location, last_location, next, prev );
        free_loc( pLoc );
        send_to_char( "Goto point deleted.\r\n", ch );
        return;
    }
    // Get a list.
    else
        do_locations( ch, ( char * ) "" );
}

/* Look up a 'goto point' room. */
ROOM_INDEX_DATA        *location_lookup( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *pRoom;
    LOCATION               *list;

    for ( list = first_location; list != NULL; list = list->next ) {
        if ( !str_prefix( argument, list->name )
             && ( pRoom = get_room_index( list->vnum ) ) != NULL )
            return pRoom;
    }
    return NULL;
}

// Save all locations.
void save_locations( void )
{
    LOCATION               *pLoc;
    FILE                   *fp;

    if ( ( fp = fopen( "system/locations.txt", "w" ) ) == NULL ) {
        bug( "%s", "Location list not found." );
        return;
    }

    for ( pLoc = first_location; pLoc != NULL; pLoc = pLoc->next )
        fprintf( fp, "Loc %s~ %d\n", pLoc->name, pLoc->vnum );

    fprintf( fp, "End\n" );
    fclose( fp );
    fp = NULL;
}

//Load all locations.
void load_locations( void )
{
    LOCATION               *pLoc;
    FILE                   *fp;
    const char             *word;
    bool                    fMatch;

    // Get the file.
    if ( ( fp = fopen( "system/locations.txt", "r" ) ) == NULL ) {
        bug( "%s", "Location list not found." );
        return;
    }

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
                // Comment.
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

                // Load a location...
            case 'L':
                if ( !str_cmp( word, "Loc" ) ) {
                    pLoc = new_loc(  );
                    STRFREE( pLoc->name );
                    pLoc->name = fread_string( fp );
                    pLoc->vnum = fread_number( fp );
                    LINK( pLoc, first_location, last_location, next, prev );
                    fMatch = TRUE;
                    break;
                }
                break;

                // End of list?
            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    fclose( fp );
                    fp = NULL;
                    return;
                }
                break;
        }
    }
    fclose( fp );
    fp = NULL;
}

LOCATION               *new_loc( void )
{
    LOCATION               *loc;

    CREATE( loc, LOCATION, 1 );
    loc->name = NULL;
    return loc;
}

void free_loc( LOCATION * loc )
{
    if ( !loc )
        return;

    STRFREE( loc->name );
    DISPOSE( loc );
}

void free_all_locations( void )
{
    LOCATION               *loc,
                           *loc_next;

    for ( loc = first_location; loc; loc = loc_next ) {
        loc_next = loc->next;
        UNLINK( loc, first_location, last_location, next, prev );
        free_loc( loc );
    }
}
