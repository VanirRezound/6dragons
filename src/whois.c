/* Volk: Thought i'd replace whois with this FINGER v3 by Samson because it's cleaner 
and i'm lazy. :) */
/****************************************************************************
 *  *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *   *                  / \    |      | /  |\   /| |     | |  \                 *
 *    *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *     *                /-----\  |      | \  |  v  | |     | |  /                 *
 *      *               /       \ |      |  \ |     | +-----+ +-/                  *
 *       ****************************************************************************
 *        * AFKMud Copyright 1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,   *
 *         * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 *          *                                                                          *
 *           * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 *            * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 *             * Grishnakh, Fireblade, and Nivek.                                         *
 *              *                                                                          *
 *               * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                *                                                                          *
 *                 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 *                  * Michael Seifert, and Sebastian Hammer.                                   *
 *                   ****************************************************************************
 *                    *                        Finger and Wizinfo Module                         *
 *                     ****************************************************************************/

/******************************************************
 *         Additions and changes by Edge of Acedia
 *                       Rewritten do_finger to better
 *                                    handle info of offline players.
 *                                               E-mail: nevesfirestar2002@yahoo.com
 *                                                ******************************************************/

#include <ctype.h>
#include <string.h>
#if !defined(WIN32)
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/whois.h"

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				      field = value;			\
				      fMatch = TRUE;			\
				      break;				\
				}

/* Begin wizinfo stuff - Samson 6-6-99 */
bool check_parse_name   args( ( char *name, bool newchar ) );

WIZINFO_DATA           *first_wizinfo;
WIZINFO_DATA           *last_wizinfo;

/* Construct wizinfo list from god dir info - Samson 6-6-99 */
void add_to_wizinfo( char *name, WIZINFO_DATA * wiz )
{
    WIZINFO_DATA           *wiz_prev;

    wiz->name = STRALLOC( name );
    if ( !wiz->email )
        wiz->email = STRALLOC( "Not Set" );

    for ( wiz_prev = first_wizinfo; wiz_prev; wiz_prev = wiz_prev->next )
        if ( strcasecmp( wiz_prev->name, name ) >= 0 )
            break;

    if ( !wiz_prev )
        LINK( wiz, first_wizinfo, last_wizinfo, next, prev );
    else
        INSERT( wiz, wiz_prev, first_wizinfo, next, prev );
}

void clear_wizinfo( bool bootup )
{
    WIZINFO_DATA           *wiz,
                           *next;

    if ( !bootup ) {
        for ( wiz = first_wizinfo; wiz; wiz = next ) {
            next = wiz->next;
            UNLINK( wiz, first_wizinfo, last_wizinfo, next, prev );
            STRFREE( wiz->name );
            STRFREE( wiz->email );
            DISPOSE( wiz );
        }
    }

    first_wizinfo = NULL;
    last_wizinfo = NULL;
}

void fread_info( WIZINFO_DATA * wiz, FILE * fp )
{
    const char             *word;
    bool                    fMatch;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'E':
                KEY( "Email", wiz->email, fread_string( fp ) );
                if ( !str_cmp( word, "End" ) )
                    return;
                break;

            case 'I':
                KEY( "ICQ", wiz->icq, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", wiz->level, fread_number( fp ) );
                break;

        }

        if ( !fMatch )
            fread_to_eol( fp );
    }
}

void build_wizinfo( bool bootup )
{
    DIR                    *dp;
    struct dirent          *dentry;
    FILE                   *fp;
    WIZINFO_DATA           *wiz;
    char                    buf[256];

    clear_wizinfo( bootup );                           /* Clear out the table before
                                                        * rebuilding a new one */

    dp = opendir( STAFF_DIR );

    dentry = readdir( dp );

    while ( dentry ) {
        /*
         *        * Added by Tarl 3 Dec 02 because we are now using CVS 
         *               */
        if ( !str_cmp( dentry->d_name, "CVS" ) ) {
            dentry = readdir( dp );
            continue;
        }
        if ( dentry->d_name[0] != '.' ) {
            snprintf( buf, 256, "%s%s", STAFF_DIR, dentry->d_name );
            fp = FileOpen( buf, "r" );
            if ( fp ) {
                CREATE( wiz, WIZINFO_DATA, 1 );
                fread_info( wiz, fp );
                add_to_wizinfo( dentry->d_name, wiz );
                FileClose( fp );
                fp = NULL;
            }
        }
        dentry = readdir( dp );
    }
    closedir( dp );
}

/* 
 *  * Wizinfo information.
 *   * Added by Samson on 6-6-99
 *    */
void do_wizinfo( CHAR_DATA *ch, char *argument )
{
    WIZINFO_DATA           *wiz;
    char                    buf[MAX_STRING_LENGTH];

    send_to_pager( "Contact Information for the STAFF:\r\r\n\n", ch );
    send_to_pager( "Name         Email Address                     ICQ#\r\n", ch );
    send_to_pager( "------------+---------------------------------+----------\r\n", ch );

    for ( wiz = first_wizinfo; wiz; wiz = wiz->next ) {
        snprintf( buf, MAX_STRING_LENGTH, "%-12s %-33s %10d", wiz->name, wiz->email, wiz->icq );
        strncat( buf, "\r\n", MAX_STRING_LENGTH );
        send_to_pager( buf, ch );
    }
}

void do_whois( CHAR_DATA *ch, char *argument )
{
    do_finger( ch, argument );
}

/* End wizinfo stuff - Samson 6-6-99 */

/* Finger snippet courtesy of unknown author. Installed by Samson 4-6-98 */
/* File read/write code redone using standard Smaug I/O routines - Samson 9-12-98 */
/* Data gathering now done via the pfiles, eliminated separate finger files - Samson 12-21-98 */
/* Improvements for offline players by Edge of Acedia 8-26-03 */
/* Further refined by Samson on 8-26-03 */
void do_finger( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim = NULL;

    // CMDTYPE *command;
    ROOM_INDEX_DATA        *temproom,
                           *original = NULL;
    int                     level = LEVEL_IMMORTAL;
    char                    buf[MAX_STRING_LENGTH],
                            fingload[256];
    struct stat             fst;
    time_t                  laston;
    bool                    loaded = FALSE,
        skip = FALSE;
    char                    buf2[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobs can't use the whois command.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Whois whom?\r\n", ch );
        return;
    }

    snprintf( buf, MAX_STRING_LENGTH, "0.%s", argument );

    /*
     *     * If player is online, check for fingerability (yeah, I coined that one)  -Edge 
     *         */
    if ( ( victim = get_char_world( ch, buf ) ) != NULL ) {
        if ( IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) && !IS_IMMORTAL( ch ) ) {
            ch_printf( ch, "%s has privacy enabled.\r\n", victim->name );
            return;
        }

    }

    /*
     * Check for offline players - Edge 
     */
    else {
        DESCRIPTOR_DATA        *d;

        snprintf( fingload, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ),
                  capitalize( argument ) );

        /*
         * Bug fix here provided by Senir to stop /dev/null crash 
         */
        if ( stat( fingload, &fst ) == -1 || !check_parse_name( capitalize( argument ), FALSE ) ) {
            ch_printf( ch, "&YNo such player named '%s'.\r\n", argument );
            return;
        }

        laston = fst.st_mtime;
        temproom = get_room_index( ROOM_VNUM_LIMBO );
        if ( !temproom ) {
            bug( "%s", "do_whois: Limbo room is not available!" );
            send_to_char( "Fatal error, report to STAFF.\r\n", ch );
            return;
        }

        CREATE( d, DESCRIPTOR_DATA, 1 );
        d->next = NULL;
        d->prev = NULL;
        d->connected = CON_GET_NAME;
        d->outsize = 2000;
        CREATE( d->outbuf, char, d->outsize );

        argument[0] = UPPER( argument[0] );

        loaded = load_char_obj( d, argument, FALSE, FALSE, TRUE );  /* Volk - TRUE is
                                                                     * quiet mode!! hahah 
                                                                     */
        LINK( d->character, first_char, last_char, next, prev );
        original = d->character->in_room;
        char_to_room( d->character, temproom );
        victim = d->character;                         /* Hopefully this will work, if
                                                        * not, we're SOL */
        d->character->desc = NULL;
        d->character = NULL;
        DISPOSE( d->outbuf );
        DISPOSE( d );

        if ( IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) && !IS_IMMORTAL( ch ) ) {
            ch_printf( ch, "%s has privacy enabled.\r\n", victim->name );
            skip = TRUE;
        }

        loaded = TRUE;
    }

    if ( !skip ) {
        send_to_char( "&w          Whois Info\r\n", ch );
        send_to_char( "          -----------\r\n", ch );
        ch_printf( ch, "&wName      : &G%-12s &wMUD Age: &G%d\r\n", victim->name,
                   calculate_age( victim ) );
        ch_printf( ch, "&wLevel     : &G%-12d &w    Sex: &G%s\r\n", victim->level,
                   victim->sex == SEX_MALE ? "Male" : victim->sex ==
                   SEX_FEMALE ? "Female" : "Other" );
        ch_printf( ch, "&wRace      : &G%-12s &w  Class: &G",
                   capitalize( race_table[victim->race]->race_name ) );
        if ( IS_THIRDCLASS( victim ) )
            ch_printf( ch, "%-3.3s/%-3.3s/%-3.3s\r\n",
                       capitalize( class_table[victim->Class]->who_name ),
                       class_table[victim->secondclass]->who_name,
                       class_table[victim->thirdclass]->who_name );
        else if ( IS_SECONDCLASS( victim ) )
            ch_printf( ch, "%-5.5s/%-5.5s\r\n", capitalize( class_table[victim->Class]->who_name ),
                       class_table[victim->secondclass]->who_name );
        else
            ch_printf( ch, "%-15s\r\n", capitalize( class_table[victim->Class]->who_name ) );
        int                     feet = victim->height / 12;
        int                     inches = victim->height % 12;

        if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
            ch_printf( ch, "&wSetting   : &G6D EXTREME!&w    since:&G %d &wlevel\r\n",
                       victim->pcdata->extlevel );
        }
        else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
            send_to_char( "&wSetting   : &GNormal\r\n", ch );
        }
        if ( victim->pcdata->rprate == 1 ) {
            send_to_char( "&wRP Rating : &GNovice\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 2 ) {
            send_to_char( "&wRP Rating : &GAmatuer\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 3 ) {
            send_to_char( "&wRP Rating : &GApprentice\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 4 ) {
            send_to_char( "&wRP Rating : &GAverage\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 5 ) {
            send_to_char( "&wRP Rating : &GRole Player\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 6 ) {
            send_to_char( "&wRP Rating : &GAbove Average\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 7 ) {
            send_to_char( "&wRP Rating : &GGood\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 8 ) {
            send_to_char( "&wRP Rating : &GGreat\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 9 ) {
            send_to_char( "&wRP Rating : &GExpert\r\n", ch );
        }
        else if ( victim->pcdata->rprate == 10 ) {
            send_to_char( "&wRP Rating : &GMaster\r\n", ch );
        }

        ch_printf( ch, "&wHeight    : &G%2d'%2d\"        &wWeight: &G%d lbs\r\n", feet, inches,
                   victim->weight );
        if ( victim->pcdata->tradeclass == 20 )
            ch_printf( ch, "&wTrade     : &GBlacksmith    &wCrafting Level:&G %d\r\n",
                       victim->pcdata->tradelevel );
        if ( victim->pcdata->tradeclass == 21 )
            ch_printf( ch, "&wTrade     : &GBaker         &wCrafting Level:&G %d\r\n",
                       victim->pcdata->tradelevel );
        if ( victim->pcdata->tradeclass == 22 )
            ch_printf( ch, "&wTrade     : &GTanner         &wCrafting Level:&G %d\r\n",
                       victim->pcdata->tradelevel );

        ch_printf( ch, "&wTitle     :&G%s\r\n", victim->pcdata->title );
        ch_printf( ch, "&wHomepage  : &G%s\r\n",
                   victim->pcdata->homepage != NULL ? victim->pcdata->homepage : "Not specified" );
        if ( ch->level == LEVEL_AJ_GENERAL ) {
            ch_printf( ch, "&wEmail     : &G%s\r\n",
                       victim->pcdata->email != NULL ? victim->pcdata->email : "Not specified" );
            ch_printf( ch, "&wICQ#      : &G%d\r\n", victim->pcdata->icq );
        }
        if ( !loaded )
            send_to_char( "&wLast on   : &GOnline now!\r\n", ch );
        else {
            ch_printf( ch, "&wLast on   : &G%s", ctime( &laston ) );
        }
        int                     score = 0;

        if ( victim->level > 1 ) {
            score += victim->level / 2;
        }
        score += GET_HITROLL( victim );
        score += GET_DAMROLL( victim );

        if ( GET_AC( victim ) > 0 )
            score -= 5;
        else if ( GET_AC( victim ) <= 0 && GET_AC( victim ) > -99 )
            score += 1;
        else if ( GET_AC( victim ) <= -99 && GET_AC( victim ) > -199 )
            score += 2;
        else if ( GET_AC( victim ) <= -199 && GET_AC( victim ) > -299 )
            score += 3;
        else if ( GET_AC( victim ) <= -299 && GET_AC( victim ) > -399 )
            score += 4;
        else if ( GET_AC( victim ) <= -399 && GET_AC( victim ) > -499 )
            score += 5;
        else if ( GET_AC( victim ) <= -499 && GET_AC( victim ) > -599 )
            score += 6;
        else if ( GET_AC( victim ) <= -599 && GET_AC( victim ) > -699 )
            score += 7;
        else if ( GET_AC( victim ) <= -699 && GET_AC( victim ) > -799 )
            score += 8;
        else if ( GET_AC( victim ) <= -799 && GET_AC( victim ) > -899 )
            score += 9;
        else if ( GET_AC( victim ) <= -899 )
            score += 10;
        if ( get_curr_str( victim ) >= 16 && get_curr_str( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_str( victim ) >= 18 ) {
            score += 2;
        }
        else if ( get_curr_str( victim ) < 16 ) {
            score -= 1;
        }
        if ( get_curr_int( victim ) >= 16 && get_curr_int( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_int( victim ) >= 18 ) {
            score += 2;
        }
        else if ( get_curr_int( victim ) < 12 ) {
            score -= 1;
        }
        if ( get_curr_wis( victim ) >= 16 && get_curr_wis( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_wis( victim ) >= 18 ) {
            score += 2;
        }
        else if ( get_curr_wis( victim ) < 12 ) {
            score -= 1;
        }
        if ( get_curr_lck( victim ) >= 16 && get_curr_lck( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_lck( victim ) >= 18 ) {
            score += 2;
        }
        else if ( get_curr_lck( victim ) < 12 ) {
            score -= 1;
        }
        if ( get_curr_cha( victim ) >= 16 && get_curr_cha( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_cha( victim ) >= 18 ) {
            score += 2;
        }
        if ( get_curr_con( victim ) >= 16 && get_curr_con( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_con( victim ) >= 18 ) {
            score += 2;
        }
        else if ( get_curr_con( victim ) < 16 ) {
            score -= 1;
        }
        if ( get_curr_dex( victim ) >= 16 && get_curr_dex( victim ) < 18 ) {
            score += 1;
        }
        else if ( get_curr_dex( victim ) >= 18 ) {
            score += 2;
        }
        else if ( get_curr_dex( victim ) < 16 ) {
            score -= 1;
        }
        if ( CAN_PKILL( victim ) ) {
            score += victim->pcdata->pkills;
            score -= victim->pcdata->pdeaths;
        }
        if ( victim->pcdata->mkills >= 1000 ) {
            score += victim->pcdata->mkills / 1000;
        }
        if ( victim->pcdata->mdeaths >= 10 ) {
            score -= victim->pcdata->mdeaths / 10;
        }

        if ( victim->max_hit > 5000 && victim->max_hit <= 8000 )
            score += 3;
        else if ( victim->max_hit > 8000 && victim->max_hit <= 9000 )
            score += 5;
        else if ( victim->max_hit > 9000 && victim->max_hit <= 10000 )
            score += 8;
        else if ( victim->max_hit > 10000 )
            score += 10;
        if ( victim->max_mana > 2000 && victim->max_mana <= 3000 )
            score += 3;
        else if ( victim->max_mana > 3000 && victim->max_mana <= 4000 )
            score += 5;
        else if ( victim->max_mana > 4000 && victim->max_mana <= 5000 )
            score += 8;
        else if ( victim->max_mana > 5000 )
            score += 10;

/* Volk - fun stuff, display percentage of areas discovered */
        float                   areaperc = 0;
        AREA_DATA              *pArea;
        FOUND_AREA             *found;
        int                     areacount = 0;
        int                     foundcount = 0;

        for ( pArea = first_asort; pArea; pArea = pArea->next_sort ) {
            if ( !IS_SET( pArea->flags, AFLAG_NODISCOVERY )
                 || !IS_SET( pArea->flags, AFLAG_UNOTSEE ) ) {
                areacount++;
            }
            for ( found = victim->pcdata->first_area; found; found = found->next ) {
                if ( !strcmp( found->area_name, pArea->name ) )
                    foundcount++;

            }
        }
        areaperc = ( foundcount * 100 ) / areacount;
        if ( areaperc >= 50 && areaperc < 60 ) {
            score += 4;
        }
        else if ( areaperc >= 60 && areaperc < 70 ) {
            score += 5;
        }
        else if ( areaperc >= 70 && areaperc < 80 ) {
            score += 6;
        }
        else if ( areaperc >= 80 && areaperc < 90 ) {
            score += 7;
        }
        else if ( areaperc >= 90 ) {
            score += 8;
        }

        ch_printf( ch, "&wTop Player Rating: &G%d\r\n", score );
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char( "&wSTAFF Information\r\n", ch );
            send_to_char( "--------------------\r\n", ch );
            ch_printf( ch, "&wIP Info       : &G%s\r\n",
                       !victim->desc ? "Unknown" : victim->desc->host );
            ch_printf( ch, "&wTime played   : &G%ld hours\r\n",
                       ( long int ) GET_TIME_PLAYED( victim ) );
            ch_printf( ch, "&wAuthorized by : &G%s\r\n",
                       victim->pcdata->authed_by ? victim->pcdata->authed_by : ( sysdata.
                                                                                 WAIT_FOR_AUTH ?
                                                                                 "Not Authed" :
                                                                                 "The Code" ) );
            ch_printf( ch, "&wPrivacy Status: &G%s\r\n",
                       IS_SET( victim->pcdata->flags, PCFLAG_PRIVACY ) ? "Enabled" : "Disabled" );
            snprintf( buf2, MAX_STRING_LENGTH, "list %s", buf );
            do_comment( ch, buf2 );
        }
        ch_printf( ch, "&wBio:\r\n&G%s\r\n",
                   victim->pcdata->bio ? victim->pcdata->bio : "Not created" );
    }

    if ( loaded ) {
        int                     x,
                                y;

        char_from_room( victim );
        char_to_room( victim, original );

        quitting_char = victim;
//      save_char_obj( victim );
/* Volk - personally don't see the need to save as no victim stats have changed except laston */

        if ( sysdata.save_pets && victim->pcdata->pet )
            extract_char( victim->pcdata->pet, TRUE );

        saving_char = NULL;

        /*
         *        * After extract_char the ch is no longer valid!
         *               */
        extract_char( victim, TRUE );
        for ( x = 0; x < MAX_WEAR; x++ )
            for ( y = 0; y < MAX_LAYERS; y++ )
                save_equipment[x][y] = NULL;
    }
}

/* Added a clone of homepage to let players input their email addy - Samson 4-18-98 */
void do_email( CHAR_DATA *ch, char *argument )
{
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' ) {
        if ( !ch->pcdata->email )
            ch->pcdata->email = STRALLOC( "" );
        ch_printf( ch, "Your email address is: %s\r\n", show_tilde( ch->pcdata->email ) );
        return;
    }

    if ( !str_cmp( argument, "clear" ) ) {
        if ( ch->pcdata->email )
            STRFREE( ch->pcdata->email );
        ch->pcdata->email = STRALLOC( "" );

        if ( IS_IMMORTAL( ch ) );
        {
            save_char_obj( ch );
            build_wizinfo( FALSE );
        }

        send_to_char( "Email address cleared.\r\n", ch );
        return;
    }

    strncpy( buf, argument, MAX_STRING_LENGTH );

    if ( strlen( buf ) > 70 )
        buf[70] = '\0';

    hide_tilde( buf );
    if ( ch->pcdata->email )
        STRFREE( ch->pcdata->email );
    ch->pcdata->email = STRALLOC( buf );
    if ( IS_IMMORTAL( ch ) );
    {
        save_char_obj( ch );
        build_wizinfo( FALSE );
    }
    send_to_char( "Email address set.\r\n", ch );
}

void do_icq_number( CHAR_DATA *ch, char *argument )
{
    int                     icq;

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' ) {
        if ( !ch->pcdata->icq )
            ch->pcdata->icq = 0;
        ch_printf( ch, "Your ICQ# is: %d\r\n", ch->pcdata->icq );
        return;
    }

    if ( !str_cmp( argument, "clear" ) ) {
        ch->pcdata->icq = 0;

        if ( IS_IMMORTAL( ch ) );
        {
            save_char_obj( ch );
            build_wizinfo( FALSE );
        }

        send_to_char( "ICQ# cleared.\r\n", ch );
        return;
    }

    if ( !is_number( argument ) ) {
        send_to_char( "You must enter numeric data.\r\n", ch );
        return;
    }

    icq = atoi( argument );

    if ( icq < 1 ) {
        send_to_char( "Valid range is greater than 0.\r\n", ch );
        return;
    }

    ch->pcdata->icq = icq;

    if ( IS_IMMORTAL( ch ) );
    {
        save_char_obj( ch );
        build_wizinfo( FALSE );
    }

    send_to_char( "ICQ# set.\r\n", ch );
}

void do_homepage( CHAR_DATA *ch, char *argument )
{
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( !argument || argument[0] == '\0' ) {
        if ( !ch->pcdata->homepage )
            ch->pcdata->homepage = STRALLOC( "" );
        ch_printf( ch, "Your homepage is: %s\r\n", show_tilde( ch->pcdata->homepage ) );
        return;
    }

    if ( !str_cmp( argument, "clear" ) ) {
        if ( ch->pcdata->homepage )
            STRFREE( ch->pcdata->homepage );
        ch->pcdata->homepage = STRALLOC( "" );
        send_to_char( "Homepage cleared.\r\n", ch );
        return;
    }

    if ( strstr( argument, "://" ) )
        strncpy( buf, argument, MAX_STRING_LENGTH );
    else
        snprintf( buf, MAX_STRING_LENGTH, "http://%s", argument );
    if ( strlen( buf ) > 70 )
        buf[70] = '\0';

    hide_tilde( buf );
    if ( ch->pcdata->homepage )
        STRFREE( ch->pcdata->homepage );
    ch->pcdata->homepage = STRALLOC( buf );
    send_to_char( "Homepage set.\r\n", ch );
}

void do_privacy( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobs can't use the privacy toggle.\r\n", ch );
        return;
    }

    TOGGLE_BIT( ch->pcdata->flags, PCFLAG_PRIVACY );

    if ( IS_SET( ch->pcdata->flags, PCFLAG_PRIVACY ) ) {
        send_to_char( "Privacy flag enabled.\r\n", ch );
        return;
    }
    else {
        send_to_char( "Privacy flag disabled.\r\n", ch );
        return;
    }
}
