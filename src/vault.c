 /*****************************************************************************
 * DikuMUD (C) 1990, 1991 by:                                                *
 *   Sebastian Hammer, Michael Seifert, Hans Henrik Staefeldt, Tom Madsen,   *
 *   and Katja Nyboe.                                                        *
 *---------------------------------------------------------------------------*
 * MERC 2.1 (C) 1992, 1993 by:                                               *
 *   Michael Chastain, Michael Quan, and Mitchell Tse.                       *
 *---------------------------------------------------------------------------*
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998 by: Derek Snider.                    *
 *   Team: Thoric, Altrag, Blodkai, Narn, Haus, Scryn, Rennard, Swordbearer, *
 *         gorog, Grishnakh, Nivek, Tricops, and Fireblade.                  *
 *---------------------------------------------------------------------------*
 * SMAUG 1.7 FUSS by: Samson and others of the SMAUG community.              *
 *                    Their contributions are greatly appreciated.           *
 *---------------------------------------------------------------------------*
 * LoP (C) 2006, 2007, 2008 by: the LoP team.                                *
 *****************************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "h/mud.h"
#include "h/files.h"

bool                    check_parse_name( char *name, bool newchar );
CHAR_DATA              *find_banker( CHAR_DATA *ch );

void fwrite_vault( CHAR_DATA *ch, OBJ_DATA *vault, char *uname )
{
    FILE                   *fp = NULL;
    char                    vaultfile[MIL],
                           *name;

    if ( !ch || IS_NPC( ch ) )
        return;

    if ( !vault ) {
        bug( "%s: NULL object. for character %s", __FUNCTION__, ch->name );
        send_to_char( "There was a problem trying to write your vault file!\r\n", ch );
        return;
    }

    name = capitalize( uname );
    snprintf( vaultfile, sizeof( vaultfile ), "%s%s", VAULT_DIR, name );
    if ( !( fp = fopen( vaultfile, "w" ) ) ) {
        bug( "%s: Couldn't open %s for write", __FUNCTION__, vaultfile );
        send_to_char( "There was some problem in writing your vault file!\r\n", ch );
        return;
    }
    fwrite_obj( ch, vault, fp, 0, OS_VAULT, TRUE );
    fprintf( fp, "%s", "#END\n" );
    fclose( fp );
}

int count_vault_items( OBJ_DATA *obj )
{
    OBJ_DATA               *vault;
    int                     count = 0;

    if ( !obj || !obj->first_content )
        return 0;
    for ( vault = obj->first_content; vault; vault = vault->next_content ) {
        count += vault->count;
        if ( vault->first_content )
            count += count_vault_items( vault );
    }
    return count;
}

void do_vault( CHAR_DATA *ch, char *argument )
{
    FILE                   *fp = NULL;
    char                    buf[MIL],
                            arg[MIL],
                           *name = NULL,
        *action = NULL;
    OBJ_DATA               *vault,
                           *vault_next;
    OBJ_INDEX_DATA         *ivault;
    CHAR_DATA              *banker;
    int                     oldcount = 0,
        newcount = 0;
    short                   cost = 1;

    if ( !ch || IS_NPC( ch ) )
        return;

    name = capitalize( ch->name );

    argument = one_argument( argument, arg );
    if ( arg != NULL && arg[0] != '\0' && IS_IMMORTAL( ch ) ) { /* Immortals can load
                                                                 * others * lockers */
        if ( str_cmp( arg, "put" ) && str_cmp( arg, "get" ) && str_cmp( arg, "list" )
             && check_parse_name( capitalize( arg ), FALSE ) ) {
            name = capitalize( arg );
            argument = one_argument( argument, arg );
        }
    }

    if ( arg == NULL || arg[0] == '\0'
         || ( str_cmp( arg, "put" ) && str_cmp( arg, "get" ) && str_cmp( arg, "list" ) ) ) {
        send_to_char
            ( "Syntax: vault put/get all\r\nSyntax: vault put/get <object name>\r\nSyntax: vault list\r\n",
              ch );
        return;
    }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_BANK ) ) {
        send_to_char( "You need to be in a bank to use your vault!\r\n", ch );
        return;
    }

    snprintf( buf, sizeof( buf ), "%s%s", VAULT_DIR, name );
    if ( ( fp = fopen( buf, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '#' ) {
                word = fread_word( fp );
                if ( !strcmp( word, "END" ) )
                    break;
                else if ( !str_cmp( word, "VAULT" ) )
                    fread_obj( ch, fp, OS_VAULT );
                else if ( !strcmp( word, "OBJECT" ) )
                    fread_obj( ch, fp, OS_CARRY );
            }
        }
        fclose( fp );
    }
    else {
        if ( !( ivault = get_obj_index( OBJ_VNUM_VAULT ) ) ) {
            send_to_char( "There is no vault to use currently.\r\n", ch );
            bug( "%s: Can't find the index vault! Vnum %d.", __FUNCTION__, OBJ_VNUM_VAULT );
            return;
        }
        if ( !( vault = create_object( ivault, 0 ) ) ) {
            send_to_char( "There is no vault to use currently.\r\n", ch );
            bug( "%s: Failed to create a vault! Vnum %d.", __FUNCTION__, OBJ_VNUM_VAULT );
            return;
        }
        snprintf( buf, sizeof( buf ), vault->name, name );
        STRSET( vault->name, buf );
        snprintf( buf, sizeof( buf ), vault->short_descr, name );
        STRSET( vault->short_descr, buf );
        snprintf( buf, sizeof( buf ), vault->description, name );
        STRSET( vault->description, buf );
        obj_to_room( vault, ch->in_room );
    }

    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "You're not in a bank!\r\n", ch );
        return;
    }

    for ( vault = ch->in_room->first_content; vault; vault = vault->next_content )
        if ( vault->pIndexData->vnum == OBJ_VNUM_VAULT && !str_cmp( vault->name, name ) )
            break;
    if ( !vault ) {
        send_to_char( "There was a problem finding your vault.\r\n", ch );
        return;
    }

    oldcount = count_vault_items( vault );

    if ( GET_MONEY( ch, CURR_SILVER ) < 1 && str_cmp( arg, "list" ) ) {
        snprintf( buf, sizeof( buf ), "say %s, you can't afford the 1 silver to use your vault.",
                  ch->name );
        interpret( banker, buf );
        return;
    }

    if ( !str_cmp( arg, "put" ) ) {
        snprintf( buf, sizeof( buf ), "put %s %s", argument, name );
        interpret( ch, buf );
        action = ( char * ) "deposit";
    }
    else if ( !str_cmp( arg, "get" ) ) {
        snprintf( buf, sizeof( buf ), "get %s %s", argument, name );
        interpret( ch, buf );
        action = ( char * ) "withdraw";
    }
    else if ( !str_cmp( arg, "list" ) ) {
        snprintf( buf, sizeof( buf ), "look in %s", name );
        interpret( ch, buf );
    }

    for ( vault = ch->in_room->first_content; vault; vault = vault_next ) {
        vault_next = vault->next_content;

        if ( vault->pIndexData->vnum != OBJ_VNUM_VAULT )
            continue;
        if ( !IS_IMMORTAL( ch ) && !str_cmp( vault->name, name ) ) {
            newcount = count_vault_items( vault );
            if ( newcount != oldcount && action ) {
                snprintf( buf, sizeof( buf ), "say %s, it costed you 1 silver to %s.", ch->name,
                          action );
                interpret( banker, buf );
                GET_MONEY( ch, CURR_SILVER ) -= cost;
            }
        }

        if ( vault->first_content )
            fwrite_vault( ch, vault, vault->name );
        else {
            snprintf( buf, sizeof( buf ), "%s%s", VAULT_DIR, capitalize( vault->name ) );
            remove_file( buf );
        }
        extract_obj( vault );
    }
}

void rename_vault( CHAR_DATA *ch, char *newname )
{
    OBJ_DATA               *vault;
    FILE                   *fp;
    char                    name[MSL],
                            uname[MSL],
                            buf[MSL];

    snprintf( name, sizeof( name ), "%s", capitalize( ch->name ) );
    snprintf( uname, sizeof( uname ), "%s", capitalize( newname ) );

    snprintf( buf, sizeof( buf ), "%s%s", VAULT_DIR, name );
    if ( ( fp = fopen( buf, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '#' ) {
                word = fread_word( fp );
                if ( !strcmp( word, "END" ) )
                    break;
                if ( !strcmp( word, "OBJECT" ) )
                    fread_obj( ch, fp, OS_VAULT );
            }
        }
        fclose( fp );
    }

    for ( vault = ch->in_room->first_content; vault; vault = vault->next_content )
        if ( !str_cmp( vault->name, ch->name ) )
            break;

    if ( vault ) {
        if ( vault->first_content ) {
            snprintf( buf, sizeof( buf ), vault->pIndexData->name, uname );
            STRSET( vault->name, buf );
            snprintf( buf, sizeof( buf ), vault->pIndexData->short_descr, uname );
            STRSET( vault->short_descr, buf );
            snprintf( buf, sizeof( buf ), vault->pIndexData->description, uname );
            STRSET( vault->description, buf );
            fwrite_vault( ch, vault, uname );
        }
        else {
            snprintf( buf, sizeof( buf ), "%s%s", VAULT_DIR, name );
            remove_file( buf );
        }
        extract_obj( vault );
    }
}
