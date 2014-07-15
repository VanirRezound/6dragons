 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Wizard/God/Immortal command module                                    *
 ***************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#define F_OK 0
#endif
#include <stdlib.h>

#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/shops.h"
#include "h/auction.h"
#include "h/languages.h"
#include "h/polymorph.h"
#include "h/events.h"
#include "h/city.h"

#define RESTORE_INTERVAL 7200

extern bool             doubleexp;
extern bool             happyhouron;
extern time_t           starttimedoubleexp;
extern time_t           starthappyhour;

void                    remove_from_rollcalls( char *name );
void                    remove_from_rosters( char *name );
void                    write_quest_list(  );

bool                    exists_player( char *name );

const char             *const save_flag[] = {
    "death", "kill", "passwd", "drop", "put", "give", "auto", "zap",
    "auction", "get", "receive", "idle", "backup", "quitbackup", "fill",
    "empty", "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24",
    "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

void                    string_stripblink( char *argument, char *string );
void                    rename_vault( CHAR_DATA *ch, char *newname );

/* from reset.c */
int generate_itemlevel  args( ( AREA_DATA *pArea, OBJ_INDEX_DATA *pObjIndex ) );
void                    build_wizinfo( bool bootup );

/* from comm.c */
bool check_parse_name   args( ( char *name, bool newchar ) );

#define IS_WAITING_FOR_AUTH(ch) (!IS_NPC(ch) && (ch)->desc)
void remove_from_auth   args( ( char *name ) );

/* from boards.c */
void                    note_attach( CHAR_DATA *ch );

/* from build.c */
int                     get_risflag( char *flag );
int                     get_defenseflag( char *flag );
int                     get_attackflag( char *flag );
int                     get_pc_race( char *type );
int get_langflag        args( ( char *flag ) );
int get_langnum         args( ( char *flag ) );

/* from tables.c */
void                    write_race_file( int ra );

void                    open_mud_log( void );

/* Local functions. */
void                    save_watchlist( void );
void                    close_area( AREA_DATA *pArea );
int                     get_color( char *argument );   /* function proto */
void                    sort_reserved( RESERVE_DATA * pRes );

/* Global variables. */

char                    reboot_time[50];
time_t                  new_boot_time_t;
extern struct tm        new_boot_struct;
extern bool             mud_down;
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA  *obj_index_hash[MAX_KEY_HASH];
extern MOB_INDEX_DATA  *mob_index_hash[MAX_KEY_HASH];

void close_all_areas( void )
{
    AREA_DATA              *area;

    while ( area = last_area )
        close_area( area );
}

int get_saveflag( char *name )
{
    size_t                  x;

    for ( x = 0; x < sizeof( save_flag ) / sizeof( save_flag[0] ); x++ )
        if ( !str_cmp( name, save_flag[x] ) )
            return x;
    return -1;
}

/*
 * Toggle "Do Not Disturb" flag. Used to prevent lower level imms from
 * using commands like "trans" and "goto" on higher level imms.
 */
void do_dnd( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];

    if ( !IS_NPC( ch ) && ch->pcdata )
        if ( IS_SET( ch->pcdata->flags, PCFLAG_DND ) ) {
            REMOVE_BIT( ch->pcdata->flags, PCFLAG_DND );
            send_to_char( "Your 'do not disturb' flag is now off.\r\n", ch );
            snprintf( buf, MSL, "%-20s has gone dnd.", ch->name );
            if ( !str_cmp( ch->name, "Vladaar" ) ) {
                interpret( ch, ( char * ) "title &W Captain, in &C[&WDumbledore's Army&C]&W" );
            }
            else {
                interpret( ch, ( char * ) "title &C No longer DND, ready to chat away." );
            }
            interpret( ch, ( char * ) "listen all" );
        }
        else {
            SET_BIT( ch->pcdata->flags, PCFLAG_DND );
            send_to_char( "Your 'do not disturb' flag is now on.\r\n", ch );
            interpret( ch, ( char * ) "title &W [&YWorking&W]&C &WBEEP&C only if needed." );
            interpret( ch,
                       ( char * )
                       "sooc Turning all channels off to get work done.  Please do not disturb with beep command unless emergency." );
            interpret( ch,
                       ( char * )
                       "ooc Turning all channels off to get work done.  Please do not disturb with beep command unless emergency." );
            interpret( ch, ( char * ) "listen none" );
            snprintf( buf, MSL, "%-20s has turned off dnd.", ch->name );
        }
    else
        send_to_char( "huh?\r\n", ch );
}

void do_wizhelp( CHAR_DATA *ch, char *argument )
{
    CMDTYPE                *cmd;
    int                     value = 0,
        col,
        hash,
        curr_lvl,
        count;

    // Quick crash fix until I have time to fix the real issue here. -Taon
    if ( ch->desc->original ) {
        send_to_char( "This command is temporarly disabled while switched. 'Until Repaired.'\r\n",
                      ch );
        return;
    }

    /*
     * Do they just want to see a certian levels? 
     */
    if ( VLD_STR( argument ) && is_number( argument ) )
        value = atoi( argument );
    if ( value < LEVEL_IMMORTAL || value > MAX_LEVEL )
        value = 0;
    for ( curr_lvl = LEVEL_IMMORTAL; curr_lvl <= MAX_LEVEL; curr_lvl++ ) {
        col = 0;
        count = 0;
        for ( hash = 0; hash < 126; hash++ ) {
            for ( cmd = command_hash[hash]; cmd; cmd = cmd->next ) {
                if ( value != 0 && value != curr_lvl )
                    continue;
                if ( ( cmd->level == curr_lvl ) && cmd->level <= MAX_LEVEL ) {
                    if ( ch->level < ( MAX_LEVEL - 2 ) && cmd->cshow == FALSE )
                        continue;
                    if ( VLD_STR( ch->pcdata->bestowments )
                         && is_name( cmd->name, ch->pcdata->bestowments ) ) {
                        if ( cmd->level <= get_trust( ch ) )
                            continue;
                    }
                    else if ( cmd->level > get_trust( ch ) )
                        continue;
                    count++;
                    if ( count == 1 )
                        pager_printf( ch, "\r\n[LEVEL %-2d]\r\n", curr_lvl );
                    col++;
                    pager_printf( ch, "%s&W%-13.13s%s",
                                  cmd->do_fun == skill_notfound ? "&R[&D" : " ", cmd->name,
                                  cmd->do_fun == skill_notfound ? "&R]&D" : " " );
                    if ( col == 5 ) {
                        send_to_pager( "\r\n", ch );
                        col = 0;
                    }
                }
            }
        }
        if ( col != 0 )
            send_to_pager( "\r\n", ch );
    }
    return;
}

void do_restrict( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    char                    buf[MSL];
    short                   level,
                            hash;
    CMDTYPE                *cmd;
    bool                    found;

    found = FALSE;
    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Restrict which command?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg2 );
    if ( arg2[0] == '\0' )
        level = get_trust( ch );
    else
        level = atoi( arg2 );

    level = UMAX( UMIN( get_trust( ch ), level ), 0 );

    hash = arg[0] % 126;
    for ( cmd = command_hash[hash]; cmd; cmd = cmd->next ) {
        if ( !str_prefix( arg, cmd->name ) && cmd->level <= get_trust( ch ) ) {
            found = TRUE;
            break;
        }
    }

    if ( found ) {
        if ( !str_prefix( arg2, "show" ) ) {
            snprintf( buf, MSL, "%s show", cmd->name );
            do_cedit( ch, buf );
            return;
        }
        cmd->level = level;
        ch_printf( ch, "You restrict %s to level %d\r\n", cmd->name, level );
        snprintf( buf, MSL, "%s restricting %s to level %d", ch->name, cmd->name, level );
        log_string( buf );
        append_to_file( VLOG_FILE, buf );
    }
    else
        send_to_char( "You may not restrict that command.\r\n", ch );

    return;
}

/* 
 * Check if the name prefix uniquely identifies a char descriptor
 */
CHAR_DATA              *get_waiting_desc( CHAR_DATA *ch, char *name )
{
    DESCRIPTOR_DATA        *d;
    CHAR_DATA              *ret_char = NULL;
    static unsigned int     number_of_hits;

    number_of_hits = 0;
    for ( d = first_descriptor; d; d = d->next ) {
        if ( d->character && ( !str_prefix( name, d->character->name ) )
             && IS_WAITING_FOR_AUTH( d->character ) ) {
            if ( ++number_of_hits > 1 ) {
                ch_printf( ch, "%s does not uniquely identify a char.\r\n", name );
                return NULL;
            }
            ret_char = d->character;                   /* return current char on exit */
        }
    }
    if ( number_of_hits == 1 )
        return ret_char;
    else {
        send_to_char( "No one like that waiting for authorization.\r\n", ch );
        return NULL;
    }
}

bool parse_for_n( char *argument )
{
    char                    test[3];
    u_int                   counter = 0;
    bool                    found = FALSE;

    while ( counter < strlen( argument ) - 1 && !found ) {
        test[0] = argument[counter];
        test[1] = argument[counter + 1];
        test[2] = '\0';
        if ( !str_cmp( test, "$n" ) )
            found = TRUE;
        counter++;
    }
    return found;
}

void do_bamfin( CHAR_DATA *ch, char *argument )
{
    if ( !IS_NPC( ch ) ) {
        if ( argument[0] != '\0' )
            if ( !parse_for_n( argument ) ) {
                send_to_char( "Line has to include $n\r\n", ch );
                return;
            }
        smash_tilde( argument );
        if ( ch->pcdata->bamfin )
            STRFREE( ch->pcdata->bamfin );
        ch->pcdata->bamfin = STRALLOC( argument );
        send_to_char( "&YYour Staff bamfin is now set.\r\n", ch );
    }
    return;
}

void do_bamfout( CHAR_DATA *ch, char *argument )
{
    if ( !IS_NPC( ch ) ) {
        if ( argument[0] != '\0' )
            if ( !parse_for_n( argument ) ) {
                send_to_char( "Line have to include $n\r\n", ch );
                return;
            }
        smash_tilde( argument );
        if ( ch->pcdata->bamfout )
            STRFREE( ch->pcdata->bamfout );
        ch->pcdata->bamfout = STRALLOC( argument );
        send_to_char( "&YYour Staff bamfout is now set.\r\n", ch );
    }
    return;
}

void do_rank( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) )
        return;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Usage:  rank <string>.\r\n", ch );
        send_to_char( "   or:  rank none.\r\n", ch );
        return;
    }

    smash_tilde( argument );
    STRFREE( ch->pcdata->rank );

    if ( !str_cmp( argument, "none" ) )
        ch->pcdata->rank = STRALLOC( "" );
    else if ( color_strlen( argument ) > 18 ) {
        send_to_char( "Rank too long! The maximum length is 18 chars.\r\n", ch );
        return;
    }
    else {
        char                    rankstring[MAX_STRING_LENGTH];

        string_stripblink( argument, rankstring );

        {
            int                     rcount = strlen( rankstring );

            while ( rcount > 0 ) {
                if ( rankstring[rcount] == '&' || rankstring[rcount] == '^'
                     || rankstring[rcount] == '\0' )
                    rankstring[rcount] == '\0';
                else
                    break;
                rcount--;
            }
        }

        ch->pcdata->rank = STRALLOC( rankstring );
    }

    send_to_char( "Rank is now set.\r\n", ch );
}

void do_retire( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Retire whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to retire you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    if ( victim->level < LEVEL_IMMORTAL && IS_SET( victim->pcdata->flags, PCFLAG_TEMP ) ) {
        send_to_char( "The minimum for retirement is to be off temp status.\r\n", ch );
        return;
    }
    if ( IS_RETIRED( victim ) ) {
        REMOVE_BIT( victim->pcdata->flags, PCFLAG_RETIRED );
        ch_printf( ch, "%s returns from retirement.\r\n", victim->name );
        ch_printf( victim, "%s brings you back from retirement.\r\n", ch->name );
    }
    else {
        SET_BIT( victim->pcdata->flags, PCFLAG_RETIRED );
        ch_printf( ch, "%s is now a retired immortal.\r\n", victim->name );
        ch_printf( victim, "Courtesy of %s, you are now a retired immortal.\r\n", ch->name );
    }
    return;
}

void do_delay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    buf[MSL];
    int                     delay;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( !*arg ) {
        send_to_char( "Syntax:  delay <victim> <# of rounds>\r\n", ch );
        return;
    }
    if ( !( victim = get_char_world( ch, arg ) ) ) {
        send_to_char( "No such character online.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Lag a MOB?.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to delay you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !*arg ) {
        send_to_char( "For how long do you wish to delay them?\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "none" ) ) {
        send_to_char( "All character delay removed.\r\n", ch );
        victim->wait = 0;
        return;
    }
    delay = atoi( arg );
    if ( delay < 1 ) {
        send_to_char( "Pointless.  Try a positive number.\r\n", ch );
        return;
    }
    if ( delay > 999 ) {
        send_to_char( "You cruel bastard.  Just kill them.\r\n", ch );
        return;
    }
    WAIT_STATE( victim, delay * PULSE_VIOLENCE );
    ch_printf( ch, "You've delayed %s for %d rounds.\r\n", victim->name, delay );
    snprintf( buf, MSL, "%-20s has delayed %s for %d rounds.", ch->name, victim->name, delay );
    append_to_file( VLOG_FILE, buf );
    return;
}

void do_deny( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MSL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Deny whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to deny you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    xSET_BIT( victim->act, PLR_DENY );
    set_char_color( AT_IMMORT, victim );
    send_to_char( "You are denied access!\r\n", victim );
    ch_printf( ch, "You have denied access to %s.\r\n", victim->name );
    snprintf( buf, MSL, "%-20s has denied access to %s.", ch->name, victim->name );
    append_to_file( VLOG_FILE, buf );
    if ( victim->fighting )
        stop_fighting( victim, TRUE );                 /* Blodkai, 97 */
    do_quit( victim, ( char * ) "" );
    return;
}

void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    DESCRIPTOR_DATA        *d;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Disconnect whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( victim->desc == NULL ) {
        act( AT_PLAIN, "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( victim->level >= get_trust( ch ) ) {
        send_to_char( "&RNever use this command against a higher Staff member.\r\n", ch );
        send_to_char( "&RYou've been warned....\r\n", ch );
        act( AT_RED, "$n tried to D/C you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    for ( d = first_descriptor; d; d = d->next ) {
        if ( d == victim->desc ) {
            close_socket( d, FALSE );
            send_to_char( "Ok.\r\n", ch );
            return;
        }
    }
    bug( "%s", "Do_disconnect: ** desc not found **." );
    send_to_char( "Descriptor not found!\r\n", ch );
    return;
}

/*
 * Force a level one player to quit.             Gorog
 */
void do_fquit( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MIL];
    char                    buf[MSL];

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Force whom to quit?\r\n", ch );
        return;
    }
    if ( !( victim = get_char_world( ch, arg1 ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( victim->level != 1 ) {
        send_to_char( "They are not level one!\r\n", ch );
        return;
    }
    set_char_color( AT_IMMORT, victim );
    send_to_char( "A MUD administrator forces you to quit...\r\n", victim );
    if ( victim->fighting )
        stop_fighting( victim, TRUE );
    do_quit( victim, ( char * ) "" );
    ch_printf( ch, "You forced %s to quit.\r\n", victim->name );
    snprintf( buf, MSL, "%-20s has forced %s to fquit.", ch->name, victim->name );
    append_to_file( VLOG_FILE, buf );
    return;
}

void do_forceclose( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MSL];
    DESCRIPTOR_DATA        *d;
    int                     desc;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Usage: forceclose <descriptor#>\r\n", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    desc = atoi( arg );
    for ( d = first_descriptor; d; d = d->next ) {
        if ( ( int ) d->descriptor == desc ) {
            if ( get_trust( victim ) >= get_trust( ch ) ) {
                send_to_char( "&RNever try to use a command against a higher Staff member...\r\n",
                              ch );
                send_to_char( "&RYou have been warned....\r\n", ch );
                act( AT_RED, "$n tried to FileClose you against your wishes.", ch, NULL, victim,
                     TO_VICT );
                return;
            }

            close_socket( d, FALSE );
            send_to_char( "Ok.\r\n", ch );
            snprintf( buf, MSL, "%-20s has FileClosed %s.", ch->name, victim->name );
            append_to_file( VLOG_FILE, buf );
            return;
        }
    }
    send_to_char( "Not found!\r\n", ch );
    return;
}

void do_pardon( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: pardon <character> <killer|thief|attacker>.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "attacker" ) ) {
        if ( xIS_SET( victim->act, PLR_ATTACKER ) ) {
            xREMOVE_BIT( victim->act, PLR_ATTACKER );
            ch_printf( ch, "Attacker flag removed from %s.\r\n", victim->name );
            set_char_color( AT_IMMORT, victim );
            send_to_char( "You are no longer an ATTACKER.\r\n", victim );
        }
        return;
    }
    if ( !str_cmp( arg2, "killer" ) ) {
        if ( xIS_SET( victim->act, PLR_KILLER ) ) {
            xREMOVE_BIT( victim->act, PLR_KILLER );
            ch_printf( ch, "Killer flag removed from %s.\r\n", victim->name );
            set_char_color( AT_IMMORT, victim );
            send_to_char( "You are no longer a KILLER.\r\n", victim );
        }
        return;
    }
    if ( !str_cmp( arg2, "thief" ) ) {
        if ( xIS_SET( victim->act, PLR_THIEF ) ) {
            xREMOVE_BIT( victim->act, PLR_THIEF );
            ch_printf( ch, "Thief flag removed from %s.\r\n", victim->name );
            set_char_color( AT_IMMORT, victim );
            send_to_char( "You are no longer a THIEF.\r\n", victim );
        }
        return;
    }
    send_to_char( "Syntax: pardon <character> <killer|thief>.\r\n", ch );
    return;
}

void echo_to_all( short AT_COLOR, const char *argument, short tar )
{
    DESCRIPTOR_DATA        *d;

    if ( !argument || argument[0] == '\0' )
        return;

    for ( d = first_descriptor; d; d = d->next ) {
        /*
         * Added showing echoes to players who are editing, so they won't
         * miss out on important info like upcoming reboots. --Narn 
         */
        if ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) {
            /*
             * This one is kinda useless except for switched.. 
             */
            if ( tar == ECHOTAR_PC && IS_NPC( d->character ) )
                continue;
            else if ( tar == ECHOTAR_IMM && !IS_IMMORTAL( d->character ) )
                continue;
            set_char_color( AT_COLOR, d->character );
            send_to_char( argument, d->character );
            send_to_char( "\r\n", d->character );
        }
    }
    return;
}

void do_echo( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    short                   color;
    int                     target;
    char                   *parg;

    set_char_color( AT_IMMORT, ch );

    if ( xIS_SET( ch->act, PLR_NO_EMOTE ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "Echo what?\r\n", ch );
        return;
    }

    if ( ( color = get_color( argument ) ) )
        argument = one_argument( argument, arg );
    parg = argument;
    argument = one_argument( argument, arg );
    if ( !str_cmp( arg, "PC" ) || !str_cmp( arg, "player" ) )
        target = ECHOTAR_PC;
    else if ( !str_cmp( arg, "imm" ) )
        target = ECHOTAR_IMM;
    else {
        target = ECHOTAR_ALL;
        argument = parg;
    }
    if ( !color && ( color = get_color( argument ) ) )
        argument = one_argument( argument, arg );
    if ( !color )
        color = AT_IMMORT;
    one_argument( argument, arg );
    echo_to_all( color, argument, target );
    append_to_file( VLOG_FILE, argument );
}

void echo_to_room( short AT_COLOR, ROOM_INDEX_DATA *room, char *argument )
{
    CHAR_DATA              *vic;

    for ( vic = room->first_person; vic; vic = vic->next_in_room ) {
        set_char_color( AT_COLOR, vic );
        send_to_char( argument, vic );
        send_to_char( "\r\n", vic );
    }
}

void do_recho( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    short                   color;

    set_char_color( AT_IMMORT, ch );

    if ( xIS_SET( ch->act, PLR_NO_EMOTE ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "Recho what?\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( ( color = get_color( argument ) ) ) {
        argument = one_argument( argument, arg );
        echo_to_room( color, ch->in_room, argument );
    }
    else
        echo_to_room( AT_IMMORT, ch->in_room, argument );
}

ROOM_INDEX_DATA        *find_location( CHAR_DATA *ch, char *arg )
{
    ROOM_INDEX_DATA        *pRoom;
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;

    if ( is_number( arg ) )
        return get_room_index( atoi( arg ) );
    if ( ( pRoom = location_lookup( ch, arg ) ) != NULL )
        return pRoom;
    if ( !str_cmp( arg, "pk" ) )                       /* "Goto pk", "at pk", etc */
        return get_room_index( last_pkroom );

    if ( ( victim = get_char_world( ch, arg ) ) != NULL )
        return victim->in_room;

    if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
        return obj->in_room;

    return NULL;
}

void do_transfer( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    ROOM_INDEX_DATA        *location;
    DESCRIPTOR_DATA        *d;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Transfer whom (and where)?\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "all" ) && get_trust( ch ) >= LEVEL_AJ_SGT ) {
        for ( d = first_descriptor; d; d = d->next ) {
            if ( d->connected == CON_PLAYING && d->character != ch && d->character->in_room
                 && d->newstate != 2 && can_see( ch, d->character ) ) {
                char                    buf[MSL];

                snprintf( buf, MSL, "%s %s", d->character->name, arg2 );
                do_transfer( ch, buf );
                append_to_file( VLOG_FILE, buf );
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' ) {
        location = ch->in_room;
    }
    else {
        if ( ( location = find_location( ch, arg2 ) ) == NULL ) {
            send_to_char( "No such location.\r\n", ch );
            return;
        }
        if ( room_is_private( location ) && get_trust( ch ) < sysdata.level_override_private ) {
            send_to_char( "That room is private right now.\r\n", ch );
            return;
        }
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( !victim->in_room ) {
        send_to_char( "They have no physical location!\r\n", ch );
        return;
    }
    /*
     * modification to prevent a low level imm from transferring a 
     */
    /*
     * higher level imm with the DND flag on.  - Gorog             
     */
    if ( !IS_NPC( victim ) && get_trust( ch ) < get_trust( victim )
         && victim->desc && ( victim->desc->connected == CON_PLAYING
                              || victim->desc->connected == CON_EDITING )
         && IS_SET( victim->pcdata->flags, PCFLAG_DND ) ) {
        pager_printf( ch, "Sorry. %s does not wish to be disturbed.\r\n", victim->name );
        pager_printf( victim, "Your DND flag just foiled %s's transfer command.\r\n", ch->name );
        return;
    }
    /*
     * end of modification 
     */

    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to transfer you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    if ( victim->fighting )
        stop_fighting( victim, TRUE );
    act( AT_MAGIC, "$n disappears in a cloud of swirling colors.", victim, NULL, NULL, TO_ROOM );
    victim->retran = victim->in_room->vnum;
    char_from_room( victim );
    char_to_room( victim, location );

    if ( victim->position == POS_CRAWL ) {
        victim->hitroll *= 4;
        victim->damroll *= 4;
        set_position( victim, POS_STANDING );
    }
    if ( victim->on ) {
        victim->on = NULL;
        set_position( victim, POS_STANDING );
    }
    if ( victim->position != POS_STANDING ) {
        set_position( victim, POS_STANDING );
    }

    act( AT_MAGIC, "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
    if ( ch != victim )
        act( AT_IMMORT, "$n has transferred you.", ch, NULL, victim, TO_VICT );
    do_look( victim, ( char * ) "auto" );
    if ( !IS_IMMORTAL( victim ) && !IS_NPC( victim ) && !in_hard_range( victim, location->area ) )
        act( AT_DANGER, "Warning:  this player's level is not within the area's level range.", ch,
             NULL, NULL, TO_CHAR );
}

void do_retran( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    char                    buf[MSL];

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Retransfer whom?\r\n", ch );
        return;
    }
    if ( !( victim = get_char_world( ch, arg ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    snprintf( buf, MSL, "'%s' %d", victim->name, victim->retran );
    do_transfer( ch, buf );
    return;
}

void do_regoto( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];

    snprintf( buf, MSL, "%d", ch->regoto );
    do_goto( ch, buf );
    return;
}

/*  Added do_at and do_atobj to reduce lag associated with at
 *  --Shaddai
 */
void do_at( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    ROOM_INDEX_DATA        *location = NULL;
    ROOM_INDEX_DATA        *original;
    CHAR_DATA              *wch = NULL,
        *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "At where what?\r\n", ch );
        return;
    }
    if ( is_number( arg ) )
        location = get_room_index( atoi( arg ) );
    else if ( !str_cmp( arg, "pk" ) )
        location = get_room_index( last_pkroom );
    else if ( ( wch = get_char_world( ch, arg ) ) == NULL || wch->in_room == NULL ) {
        send_to_char( "No such mobile or player in existance.\r\n", ch );
        return;
    }
    if ( !location && wch )
        location = wch->in_room;

    if ( !location ) {
        send_to_char( "No such location exists.\r\n", ch );
        return;
    }

    /*
     * The following mod is used to prevent players from using the 
     */
    /*
     * at command on a higher level immortal who has a DND flag    
     */
    if ( wch && !IS_NPC( wch ) && IS_SET( wch->pcdata->flags, PCFLAG_DND )
         && get_trust( ch ) < get_trust( wch ) ) {
        pager_printf( ch, "Sorry. %s does not wish to be disturbed.\r\n", wch->name );
        pager_printf( wch, "Your DND flag just foiled %s's at command.\r\n", ch->name );
        return;
    }
    /*
     * End of modification  -- Gorog 
     */

    if ( room_is_private( location ) ) {
        if ( get_trust( ch ) < LEVEL_AJ_LT ) {
            send_to_char( "That room is private right now.\r\n", ch );
            return;
        }
        else
            send_to_char( "Overriding private flag!\r\n", ch );
    }

    if ( ( victim = room_is_dnd( ch, location ) ) ) {
        pager_printf( ch, "That room is \"do not disturb\" right now.\r\n" );
        pager_printf( victim, "Your DND flag just foiled %s's atmob command\r\n", ch->name );
        return;
    }

    set_char_color( AT_PLAIN, ch );
    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    if ( !char_died( ch ) ) {
        char_from_room( ch );
        char_to_room( ch, original );
    }
    return;
}

void do_atobj( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    ROOM_INDEX_DATA        *location;
    ROOM_INDEX_DATA        *original;
    OBJ_DATA               *obj;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "At where what?\r\n", ch );
        return;
    }

    if ( ( obj = get_obj_world( ch, arg ) ) == NULL || !obj->in_room ) {
        send_to_char( "No such object in existance.\r\n", ch );
        return;
    }
    location = obj->in_room;
    if ( room_is_private( location ) ) {
        if ( get_trust( ch ) < LEVEL_AJ_LT ) {
            send_to_char( "That room is private right now.\r\n", ch );
            return;
        }
        else
            send_to_char( "Overriding private flag!\r\n", ch );
    }

    if ( ( victim = room_is_dnd( ch, location ) ) ) {
        pager_printf( ch, "That room is \"do not disturb\" right now.\r\n" );
        pager_printf( victim, "Your DND flag just foiled %s's atobj command\r\n", ch->name );
        return;
    }

    set_char_color( AT_PLAIN, ch );
    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    if ( !char_died( ch ) ) {
        char_from_room( ch );
        char_to_room( ch, original );
    }
    return;
}

void do_rat( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    ROOM_INDEX_DATA        *location;
    ROOM_INDEX_DATA        *original;
    int                     Start,
                            End,
                            vnum;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "Syntax: rat <start> <end> <command>\r\n", ch );
        return;
    }

    Start = atoi( arg1 );
    End = atoi( arg2 );
    if ( Start < 1 || End < Start || Start > End || Start == End || End > MAX_VNUM ) {
        send_to_char( "Invalid range.\r\n", ch );
        return;
    }
    if ( !str_cmp( argument, "quit" ) ) {
        send_to_char( "I don't think so!\r\n", ch );
        return;
    }

    original = ch->in_room;
    for ( vnum = Start; vnum <= End; vnum++ ) {
        if ( ( location = get_room_index( vnum ) ) == NULL )
            continue;
        char_from_room( ch );
        char_to_room( ch, location );
        interpret( ch, argument );
    }

    char_from_room( ch );
    char_to_room( ch, original );
    send_to_char( "Done.\r\n", ch );
    return;
}

void do_rstat( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL],
                            arg[MIL];
    const char             *sect;
    ROOM_INDEX_DATA        *location;
    OBJ_DATA               *obj;
    CHAR_DATA              *rch;
    EXIT_DATA              *pexit;
    AFFECT_DATA            *paf;
    int                     cnt;
    const static char      *dir_text[] =
        { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?", "ex" };

    one_argument( argument, arg );
    if ( !str_cmp( arg, "ex" ) || !str_cmp( arg, "exits" ) ) {
        location = ch->in_room;

        ch_printf( ch, "&cExits for room '&W%s&c'  Vnum &W%d\r\n", location->name, location->vnum );
        for ( cnt = 0, pexit = location->first_exit; pexit; pexit = pexit->next )
            ch_printf( ch,
                       "&W%2d)  &w%s to %-5d  &cKey: &w%d  &cFlags: &w%s\r\n  &cKeywords: '&w%s&c'\r\n     Exdesc: &w%s     &cBack link: &w%d  &cVnum: &w%d  &cPulltype: &w%s  &cPull: &w%d\r\n",
                       ++cnt, dir_text[pexit->vdir],
                       pexit->to_room ? pexit->to_room->vnum : 0, pexit->key,
                       flag_string( pexit->exit_info, ex_flags ), pexit->keyword,
                       VLD_STR( pexit->description ) ? pexit->description : "(none).\r\n",
                       pexit->rexit ? pexit->rexit->vnum : 0, pexit->rvnum,
                       pull_type_name( pexit->pulltype ), pexit->pull );
        return;
    }
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
    if ( !location ) {
        send_to_char( "No such location.\r\n", ch );
        return;
    }
    if ( ch->in_room != location && room_is_private( location ) ) {
        if ( get_trust( ch ) < LEVEL_AJ_LT ) {
            send_to_char( "That room is private right now.\r\n", ch );
            return;
        }
        else
            send_to_char( "Overriding private flag!\r\n", ch );
    }
    ch_printf( ch, "&cName: &w%s\r\n&cArea: &w%s  &cFilename: &w%s\r\n", location->name,
               location->area ? location->area->name : "None????",
               location->area ? location->area->filename : "None????" );
    switch ( ch->in_room->sector_type ) {
        default:
            sect = "??\?!";
            break;
        case SECT_INSIDE:
            sect = "Inside";
            break;
        case SECT_JUNGLE:
            sect = "Jungle";
            break;
        case SECT_ROAD:
            sect = "Road";
            break;
        case SECT_VROAD:
            sect = "VerticalRoad";
            break;
        case SECT_HROAD:
            sect = "HorizontalRoad";
            break;
        case SECT_CAMPSITE:
            sect = "Campsite";
            break;
        case SECT_DOCK:
            sect = "Dock";
            break;
        case SECT_LAKE:
            sect = "Lake";
            break;
        case SECT_WATERFALL:
            sect = "Waterfall";
            break;
        case SECT_RIVER:
            sect = "River";
            break;
        case SECT_GRASSLAND:
            sect = "Grassland";
            break;
        case SECT_CROSSROAD:
            sect = "Crossroad";
            break;
        case SECT_ARCTIC:
            sect = "Arctic";
            break;
        case SECT_THICKFOREST:
            sect = "Thickforest";
            break;
        case SECT_HIGHMOUNTAIN:
            sect = "Highmountain";
            break;
        case SECT_CITY:
            sect = "City";
            break;
        case SECT_FIELD:
            sect = "Field";
            break;
        case SECT_FOREST:
            sect = "Forest";
            break;
        case SECT_HILLS:
            sect = "Hills";
            break;
        case SECT_MOUNTAIN:
            sect = "Mountains";
            break;
        case SECT_WATER_SWIM:
            sect = "Swim";
            break;
        case SECT_WATER_NOSWIM:
            sect = "Noswim";
            break;
        case SECT_UNDERWATER:
            sect = "Underwater";
            break;
        case SECT_AIR:
            sect = "Air";
            break;
        case SECT_DESERT:
            sect = "Desert";
            break;
        case SECT_OCEANFLOOR:
            sect = "Oceanfloor";
            break;
        case SECT_OCEAN:
            sect = "Ocean";
            break;
        case SECT_UNDERGROUND:
            sect = "Underground";
            break;
        case SECT_LAVA:
            sect = "Lava";
            break;
        case SECT_SWAMP:
            sect = "Swamp";
            break;
    }
    ch_printf( ch, "&cVnum: &w%d   &cSector: &w%d (%s)   &cLight: &w%d", location->vnum,
               location->sector_type, sect, location->light );
    if ( location->tunnel > 0 )
        ch_printf( ch, "   &cTunnel: &W%d  ", location->tunnel );
    if ( location->height > 0 )
        ch_printf( ch, "   &cHeight: &W%d", location->height );
    send_to_char( "\r\n", ch );
    if ( location->tele_delay > 0 || location->tele_vnum > 0 )
        ch_printf( ch, "&cTeleDelay: &R%d   &cTeleVnum: &R%d\r\n", location->tele_delay,
                   location->tele_vnum );
    ch_printf( ch, "&cRoom flags: &w%s\r\n", flag_string( location->room_flags, r_flags ) );
    ch_printf( ch, "&cDescription:\r\n&w%s", location->description );
    if ( location->first_extradesc ) {
        EXTRA_DESCR_DATA       *ed;

        send_to_char( "&cExtra description keywords: &w'", ch );
        for ( ed = location->first_extradesc; ed; ed = ed->next ) {
            send_to_char( ed->keyword, ch );
            if ( ed->next )
                send_to_char( " ", ch );
        }
        send_to_char( "'\r\n", ch );
    }
    for ( paf = location->first_affect; paf; paf = paf->next )
        ch_printf( ch, "&cAffect: &w%s &cby &w%d.\r\n", flag_string( paf->location, a_types ),
                   paf->modifier );
    send_to_char( "&cCharacters: &w", ch );
    for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
        if ( can_see( ch, rch ) ) {
            send_to_char( " ", ch );
            one_argument( rch->name, buf );
            send_to_char( buf, ch );
        }
    }
    send_to_char( "\r\n&cObjects:    &w", ch );
    for ( obj = location->first_content; obj; obj = obj->next_content ) {
        send_to_char( " ", ch );
        one_argument( obj->name, buf );
        send_to_char( buf, ch );
    }
    send_to_char( "\r\n", ch );
    if ( location->first_exit )
        send_to_char( "&c------------------- &wEXITS &c-------------------\r\n", ch );
    for ( cnt = 0, pexit = location->first_exit; pexit; pexit = pexit->next )
        ch_printf( ch, "%2d) %-2s to %-5d.  Key: %d  Keywords: %s  Flags: %s.\r\n", ++cnt,
                   dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0, pexit->key,
                   VLD_STR( pexit->keyword ) ? pexit->keyword : "(none)",
                   flag_string( pexit->exit_info, ex_flags ) );
    return;
}

/* Face-lift by Demora */
void do_ostat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    AFFECT_DATA            *paf;
    OBJ_DATA               *obj;

    set_char_color( AT_CYAN, ch );
    one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_char( "Ostat what?\r\n", ch );
        return;
    }
    if ( arg[0] != '\'' && arg[0] != '"' && strlen( argument ) > strlen( arg ) )
        mudstrlcpy( arg, argument, MIL );
    if ( ( obj = get_obj_world( ch, arg ) ) == NULL ) {
        send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
        return;
    }
    ch_printf( ch, "&cName: &C%s\r\n", VLD_STR( obj->name ) ? obj->name : "" );
    ch_printf( ch, "&cVnum: &w%d  ", obj->pIndexData->vnum );
    ch_printf( ch, "&cArea: &w%s &c-&W %s\r\n",
               obj->pIndexData->area ? obj->pIndexData->area->name : "(none)",
               obj->pIndexData->area ? obj->pIndexData->area->filename : "(none)" );
    ch_printf( ch, "&cType: &w%s  ", item_type_name( obj ) );
    ch_printf( ch, "&cCount:  &w%d  ", obj->pIndexData->count );
    ch_printf( ch, "&cGcount: &w%d\r\n", obj->count );
/*
  ch_printf(ch, "&cSliced Vnum: &w%d  ", obj->serial);
  ch_printf(ch, "&cSliced Index Vnum: &w%d  ", obj->pIndexData->serial);
 */
    ch_printf( ch, "&cTopSerial#: &w%d\r\n", cur_obj_serial );
    ch_printf( ch, "&cShort description: &C%s\r\n",
               VLD_STR( obj->short_descr ) ? obj->short_descr : "" );
    ch_printf( ch, "&cLong description : &C%s\r\n",
               VLD_STR( obj->description ) ? obj->description : "" );
    if ( VLD_STR( obj->action_desc ) )
        ch_printf( ch, "&cAction description: &w%s\r\n", obj->action_desc );
    ch_printf( ch, "&cWear flags : &w%s\r\n", flag_string( obj->wear_flags, w_flags ) );
    ch_printf( ch, "&cExtra flags: &w%s\r\n", ext_flag_string( &obj->extra_flags, o_flags ) );
    ch_printf( ch, "&cNumber: &w%d/%d   ", 1, get_obj_number( obj ) );
    ch_printf( ch, "&cWeight: &w%d/%d   ", obj->weight, get_obj_weight( obj, TRUE ) );
    ch_printf( ch, "&cLayers: &w%d   ", obj->pIndexData->layers );
    ch_printf( ch, "&cWear_loc: &w%d\r\n", obj->wear_loc );

    if ( obj->currtype && obj->currtype < MAX_CURR_TYPE )
        ch_printf( ch, "&cCost: &w%d&c/%s    &d", obj->cost, curr_types[obj->currtype] );
    else
        send_to_char( "&RERROR:&WCan't establish this items currtype.&d    ", ch );

    ch_printf( ch, "&cSize:&w %d, %s (proto %d)\r\n", obj->size, obj_sizes[obj->size],
               obj->pIndexData->size );

    ch_printf( ch, "&cColor: &w%-2d   &cOwner: &w%-12s &cGlory: &w%-3d&c/&w%-3d\r\n", obj->color,
               obj->owner ? obj->owner : "(nobody)", obj->glory, ( obj->level * 2 ) - 1 );
    ch_printf( ch, "&cRent : &w%-4d ", obj->pIndexData->rent );
    send_to_char( "&cTimer: ", ch );
    if ( obj->timer > 0 )
        ch_printf( ch, "&R%-6d ", obj->timer );
    else
        ch_printf( ch, "&w%-6d ", obj->timer );
    ch_printf( ch, "      &cLevel: &P%d\r\n", obj->level );
    ch_printf( ch, "&cIn room: &w%d  ", obj->in_room == NULL ? 0 : obj->in_room->vnum );
    ch_printf( ch, "&cIn object: &w%s  ",
               obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr );
    ch_printf( ch, "&cCarried by: &C%s\r\n",
               obj->carried_by == NULL ? "(none)" : obj->carried_by->name );
    ch_printf( ch, "&cIndex Values : &w%d %d %d %d %d %d %d.\r\n", obj->pIndexData->value[0],
               obj->pIndexData->value[1], obj->pIndexData->value[2], obj->pIndexData->value[3],
               obj->pIndexData->value[4], obj->pIndexData->value[5], obj->pIndexData->value[6] );
    ch_printf( ch, "&cObject Values: &w%d %d %d %d %d %d %d.\r\n", obj->value[0], obj->value[1],
               obj->value[2], obj->value[3], obj->value[4], obj->value[5], obj->value[6] );
    if ( obj->pIndexData->first_extradesc ) {
        EXTRA_DESCR_DATA       *ed;

        send_to_char( "Primary description keywords:   '", ch );
        for ( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next ) {
            send_to_char( ed->keyword, ch );
            if ( ed->next )
                send_to_char( " ", ch );
        }
        send_to_char( "'.\r\n", ch );
    }
    if ( obj->first_extradesc ) {
        EXTRA_DESCR_DATA       *ed;

        send_to_char( "Secondary description keywords: '", ch );
        for ( ed = obj->first_extradesc; ed; ed = ed->next ) {
            send_to_char( ed->keyword, ch );
            if ( ed->next )
                send_to_char( " ", ch );
        }
        send_to_char( "'.\r\n", ch );
    }
    if ( obj->pIndexData->rating > 0 )
        ch_printf( ch, "&RObj Rating: &w%d\r\n", obj->pIndexData->rating );
    if ( obj->pIndexData->item_type == ITEM_CONTAINER ) {
        ch_printf( ch, "&cIndex flags: &w%s&c\r\n",
                   flag_string( obj->pIndexData->value[1], container_flags ) );
        ch_printf( ch, "&cCurrent flags: &w%s&c\r\n",
                   flag_string( obj->value[1], container_flags ) );
    }
    for ( paf = obj->first_affect; paf; paf = paf->next )
        ch_printf( ch, "&cAffects &w%s &cby &w%d. (extra)\r\n",
                   a_types[paf->location % REVERSE_APPLY], paf->modifier );
    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
        ch_printf( ch, "&cAffects &w%s &cby &w%d.\r\n", a_types[paf->location % REVERSE_APPLY],
                   paf->modifier );
    return;
}

void do_vclear( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( argument[0] == '\0' ) {
        send_to_char( "Vclear whom?\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, argument ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( get_trust( ch ) < get_trust( victim ) ) {
        send_to_char( "Their godly glow prevents you from getting a good look.\r\n", ch );
        return;
    }

    if ( !victim->variables ) {
        send_to_char( "They have no variables currently assigned to them.\r\n", ch );
        return;
    }

    if ( victim->variables ) {
        VARIABLE_DATA          *vd,
                               *vd_next = NULL,
            *vd_prev = NULL;

        for ( vd = victim->variables; vd; vd = vd_next ) {
            vd_next = vd->next;

            if ( vd->timer > 0 ) {
                if ( vd == victim->variables )
                    victim->variables = vd_next;
                else if ( vd_prev )
                    vd_prev->next = vd_next;
                delete_variable( vd );
            }
            else
                vd_prev = vd;
        }
    }
}

void do_vstat( CHAR_DATA *ch, char *argument )
{
    VARIABLE_DATA          *vd;
    CHAR_DATA              *victim;

    if ( argument[0] == '\0' ) {
        send_to_char( "Vstat whom?\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, argument ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( get_trust( ch ) < get_trust( victim ) ) {
        send_to_char( "Their godly glow prevents you from getting a good look.\r\n", ch );
        return;
    }

    if ( !victim->variables ) {
        send_to_char( "They have no variables currently assigned to them.\r\n", ch );
        return;
    }

    pager_printf( ch, "\r\n&cName: &C%-20s &cRoom : &w%-10d", victim->name,
                  victim->in_room == NULL ? 0 : victim->in_room->vnum );
    pager_printf( ch, "\r\n&cVariables:\r\n" );

    /*
     * Variables:
     * Vnum:           Tag:                 Type:     Timer:
     * Flags:
     * Data:
     */
    for ( vd = victim->variables; vd; vd = vd->next ) {
        pager_printf( ch, "  &cVnum: &W%-10d &cTag: &W%-15s &cTimer: &W%d\r\n", vd->vnum, vd->tag,
                      vd->timer );
        pager_printf( ch, "  &cType: " );
        if ( vd->data ) {
            switch ( vd->type ) {
                case vtSTR:
                    if ( vd->data )
                        pager_printf( ch, "&CString     &cData: &W%s", ( char * ) vd->data );
                    break;

                case vtINT:
                    if ( vd->data )
                        pager_printf( ch, "&CInteger    &cData: &W%ld", ( long ) vd->data );
                    break;

                case vtXBIT:
                    if ( vd->data ) {
                        char                    buf[MAX_STRING_LENGTH];
                        int                     started = 0;
                        int                     x;

                        buf[0] = '\0';
                        for ( x = MAX_BITS; x > 0; --x ) {
                            if ( !started && xIS_SET( *( EXT_BV * ) vd->data, x ) )
                                started = x;
                        }

                        for ( x = 1; x <= started; x++ )
                            strcat( buf, xIS_SET( *( EXT_BV * ) vd->data, x ) ? "1 " : "0 " );

                        if ( buf[0] != '\0' )
                            buf[strlen( buf ) - 1] = '\0';
                        pager_printf( ch, "&CXBIT       &cData: &w[&W%s&w]", buf );
                    }
                    break;
            }
        }
        else
            send_to_pager( "&CNo Data", ch );

        send_to_pager( "\r\r\n\n", ch );
    }
    return;
}

void do_mstat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            hpbuf[MSL],
                            mnbuf[MSL],
                            mvbuf[MSL],
                            bdbuf[MSL];
    AFFECT_DATA            *paf;
    CHAR_DATA              *victim;
    SKILLTYPE              *skill;
    int                     x;

    one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_pager( "Mstat whom?\r\n", ch );
        return;
    }
    if ( arg[0] != '\'' && arg[0] != '"' && strlen( argument ) > strlen( arg ) )
        mudstrlcpy( arg, argument, MIL );
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_pager( "They aren't here.\r\n", ch );
        return;
    }
    if ( get_trust( ch ) < get_trust( victim ) && !IS_NPC( victim ) ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Their godly glow prevents you from getting a good look.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) && get_trust( ch ) < LEVEL_AJ_LT
         && xIS_SET( victim->act, ACT_STATSHIELD ) ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Their godly glow prevents you from getting a good look.\r\n", ch );
        return;
    }
    pager_printf( ch, "\r\n&c%s: &C%-20s", IS_NPC( victim ) ? "Mobile name" : "Name",
                  victim->name );
    if ( !IS_NPC( victim ) )
        pager_printf( ch, "&cStatus : &w%-10s",
                      CAN_PKILL( victim ) ? "Deadly" : IS_PKILL( victim ) ? "Pre-Deadly" :
                      "Non-Deadly" );
    if ( IS_NPC( victim ) )
        pager_printf( ch, "&cArea: &w%-50s\r\n",
                      victim->pIndexData->area ? victim->pIndexData->area->name : "(none)" );
    if ( !IS_NPC( victim ) && victim->pcdata->clan )
        pager_printf( ch, "   &cClan: &w%s", victim->pcdata->clan->name );
    send_to_pager( "\r\n", ch );
    if ( get_trust( ch ) >= LEVEL_AJ_CPL && !IS_NPC( victim ) && victim->desc )
        pager_printf( ch,
                      "&cHost: &w%s   Descriptor: %d  &cTrust: &w%d  &cAuthBy: &w%s\r\n",
                      victim->desc->host, victim->desc->descriptor, victim->trust,
                      VLD_STR( victim->pcdata->authed_by ) ? victim->pcdata->
                      authed_by : "(unknown)" );
    if ( !IS_NPC( victim ) && victim->pcdata->release_date != 0 )
        pager_printf( ch, "&cHelled until %24.24s by %s.\r\n",
                      ctime( &victim->pcdata->release_date ),
                      VLD_STR( victim->pcdata->helled_by ) ? victim->pcdata->
                      helled_by : "(unknown)" );
    pager_printf( ch,
                  "&cVnum: &w%-5d    &cSex: &w%-6s    &cRoom: &w%-5d    &cCount: &w%d   &cKilled: &w%d\r\n",
                  IS_NPC( victim ) ? victim->pIndexData->vnum : 0,
                  victim->sex == SEX_MALE ? "male" : victim->sex ==
                  SEX_FEMALE ? "female" : "neutral",
                  victim->in_room == NULL ? 0 : victim->in_room->vnum,
                  IS_NPC( victim ) ? victim->pIndexData->count : 1,
                  IS_NPC( victim ) ? victim->pIndexData->killed : victim->pcdata->mdeaths +
                  victim->pcdata->pdeaths );
    pager_printf( ch,
                  "&cStr: &C%2d&c)(Int: &C%2d&c)(Wis: &C%2d&c)(Dex: &C%2d&c)(Con: &C%2d&c)(Cha: &C%2d&c)(Lck: &C%2d&c\r\n",
                  get_curr_str( victim ), get_curr_int( victim ), get_curr_wis( victim ),
                  get_curr_dex( victim ), get_curr_con( victim ), get_curr_cha( victim ),
                  get_curr_lck( victim ) );
    pager_printf( ch, "&cLevel   : &P%-2d              ", victim->level );
    pager_printf( ch, "&cClass  : &w%-2.2d/%-10s   &cRace      : &w%-2.2d/%-10s\r\n",
                  victim->Class, IS_NPC( victim ) ? victim->Class < MAX_NPC_CLASS
                  && victim->Class >=
                  0 ? npc_class[victim->Class] : "unknown" : victim->Class < MAX_PC_CLASS
                  && class_table[victim->Class]->who_name
                  && class_table[victim->Class]->who_name[0] !=
                  '\0' ? class_table[victim->Class]->who_name : "unknown", victim->race,
                  IS_NPC( victim ) ? victim->race < MAX_NPC_RACE
                  && victim->race >=
                  0 ? npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE
                  && race_table[victim->race]->race_name
                  && race_table[victim->race]->race_name[0] !=
                  '\0' ? race_table[victim->race]->race_name : "unknown", npc_race[victim->race],
                  victim->position );

    if ( victim->secondclass >= 0 && victim->secondclass < MAX_PC_CLASS )
        pager_printf( ch, "&cSecClass: &w%-2.2d/%-10s &cLevel: &P%-2d\r\n", victim->secondclass,
                      class_table[victim->secondclass]->who_name, victim->secondlevel );
    if ( victim->thirdclass >= 0 && victim->thirdclass < MAX_PC_CLASS )
        pager_printf( ch, "&cThiClass: &w%-2.2d/%-10s &cLevel: &P%-2d\r\n", victim->thirdclass,
                      class_table[victim->thirdclass]->who_name, victim->thirdlevel );

    snprintf( hpbuf, MSL, "%d/%d", victim->hit, victim->max_hit );
    if ( victim->pcdata && victim->pcdata->tmpmax_hit )
        snprintf( hpbuf + strlen( hpbuf ), MSL - strlen( hpbuf ), "[%d]",
                  victim->pcdata->tmpmax_hit );

    snprintf( mnbuf, MSL, "%d/%d", victim->mana, victim->max_mana );
    snprintf( mvbuf, MSL, "%d/%d", victim->move, victim->max_move );
    if ( IS_NPC( victim ) )
        ch_printf( ch, "&cClan &w%s\t&c Influence &w%d\r\n", victim->clan, victim->influence );
    if ( !IS_NPC( victim ) ) {
        send_to_char( "&cMoney - Type  (Amount on Hand)\r\n", ch );
        for ( x = 1; x < MAX_CURR_TYPE; x++ )
            ch_printf( ch, "&c%s:&w %d  ", cap_curr_types[x], GET_MONEY( victim, x ) );
    }
    else {
        for ( x = 1; x < MAX_CURR_TYPE; x++ )
            ch_printf( ch, "&c%s:&w %d  ", cap_curr_types[x], GET_MONEY( victim, x ) );
    }
    if ( IS_BLOODCLASS( victim ) && !IS_NPC( victim ) ) {
        snprintf( bdbuf, MSL, "%d/%d", victim->blood, victim->max_blood );
        pager_printf( ch,
                      "\r\n&cHps     : &w%-12s    &cBlood  : &w%-12s    &cMove      : &w%-12s\r\n",
                      hpbuf, bdbuf, mvbuf );
    }
    else
        pager_printf( ch,
                      "\r\n&cHps     : &w%-12s    &cMana   : &w%-12s    &cMove      : &w%-12s\r\n",
                      hpbuf, mnbuf, mvbuf );
    pager_printf( ch,
                  "&cHitroll : &C%-5d           &cAlign  : &w%-5d           &cArmorClass: &w%d\r\n",
                  GET_HITROLL( victim ), victim->alignment, GET_AC( victim ) );
    pager_printf( ch,
                  "&cDamroll : &C%-5d           &cWimpy  : &w%-5d           &cPosition  : &w%d\r\n",
                  GET_DAMROLL( victim ), victim->wimpy, victim->position );
    pager_printf( ch, "&cFighting: &w%-13s   &cMaster : &w%-13s   &cLeader    : &w%s\r\n",
                  victim->fighting ? victim->fighting->who->name : "(none)",
                  victim->master ? victim->master->name : "(none)",
                  victim->leader ? victim->leader->name : "(none)" );
    if ( IS_NPC( victim ) )
        pager_printf( ch,
                      "&cHating  : &w%-13s   &cHunting: &w%-13s   &cFearing   : &w%s\r\n",
                      victim->hating ? victim->hating->name : "(none)",
                      victim->hunting ? victim->hunting->name : "(none)",
                      victim->fearing ? victim->fearing->name : "(none)" );
    else
        pager_printf_color( ch, "&cDeity   : &w%-13s&w   &cFavor  : &w%-5d\r\n",
                            victim->pcdata->deity ? victim->pcdata->deity->name : "(none)",
                            victim->pcdata->favor );
    if ( IS_NPC( victim ) )
        pager_printf( ch,
                      "&cMob hitdie : &C%dd%d+%d    &cMob damdie : &C%dd%d+%3d    &cIndex damdie : &C%dd%d+%3d\r\n&cNumAttacks : &C%3d   &cPosition:&C %2d&c DefPosition: &C%2d&w\r\n",
                      victim->pIndexData->hitnodice, victim->pIndexData->hitsizedice,
                      victim->pIndexData->hitplus, victim->barenumdie,
                      victim->baresizedie, victim->damplus, victim->pIndexData->damnodice,
                      victim->pIndexData->damsizedice, victim->pIndexData->damplus,
                      victim->numattacks, victim->position, victim->defposition );
    pager_printf( ch, "&cMentalState: &w%-3d   &cEmotionalState: &w%-3d   ", victim->mental_state,
                  victim->emotional_state );
    if ( !IS_NPC( victim ) )
        pager_printf( ch, "&cThirst: &w%d   &cFull: &w%d   &cDrunk: &w%d\r\n",
                      victim->pcdata->condition[COND_THIRST], victim->pcdata->condition[COND_FULL],
                      victim->pcdata->condition[COND_DRUNK] );
    else
        send_to_pager( "\r\n", ch );
    pager_printf( ch,
                  "&cSave versus: &w%d %d %d %d %d       &cItems: &w(%d/%d)  &cWeight &w(%d/%d)\r\n",
                  victim->saving_poison_death, victim->saving_wand,
                  victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff,
                  victim->carry_number, can_carry_n( victim ), victim->carry_weight,
                  can_carry_w( victim ) );
    pager_printf( ch, "&cYear: &w%-5d  &cSecs: &w%d  &cTimer: &w%d\r\n", calculate_age( victim ),
                  ( int ) victim->played, victim->timer );

    if ( get_timer( victim, TIMER_PKILLED ) )
        pager_printf( ch, "&cTimerPkilled:  &R%d\r\n", get_timer( victim, TIMER_PKILLED ) );
    if ( get_timer( victim, TIMER_RECENTFIGHT ) )
        pager_printf( ch, "&cTimerRecentfight:  &R%d\r\n", get_timer( victim, TIMER_RECENTFIGHT ) );
    if ( get_timer( victim, TIMER_ASUPRESSED ) )
        pager_printf( ch, "&cTimerAsupressed:  &R%d\r\n", get_timer( victim, TIMER_ASUPRESSED ) );

    if ( !IS_NPC( victim ) ) {
        pager_printf( ch, "&cHometown:  &w%s\r\n", victim->pcdata->htown_name );
    }

    if ( !IS_NPC( victim ) && IS_CITY( victim ) ) {
        pager_printf( ch, "&cCity:      &w%s\r\n", victim->pcdata->city_name );
    }

    if ( IS_NPC( victim ) ) {
        pager_printf( ch, "&cAct Flags  : &w%s\r\n", ext_flag_string( &victim->act, act_flags ) );
    }
    else {
        pager_printf( ch, "&cPlayerFlags: &w%s\r\n", ext_flag_string( &victim->act, plr_flags ) );
        pager_printf( ch, "&cPcflags    : &w%s\r\n",
                      flag_string( victim->pcdata->flags, pc_flags ) );
        if ( victim->pcdata->nuisance ) {
            pager_printf( ch,
                          "&RNuisance   &cStage: (&R%d&c/%d)  Power:  &w%d  &cTime:  &w%s.\r\n",
                          victim->pcdata->nuisance->flags, MAX_NUISANCE_STAGE,
                          victim->pcdata->nuisance->power,
                          ctime( &victim->pcdata->nuisance->time ) );
        }
    }
    if ( victim->morph ) {
        if ( victim->morph->morph )
            pager_printf( ch, "&cMorphed as : (&C%d&c) &C%s    &cTimer: &C%d\r\n",
                          victim->morph->morph->vnum, victim->morph->morph->short_desc,
                          victim->morph->timer );
        else
            pager_printf( ch, "&cMorphed as: Morph was deleted.\r\n" );
    }
    pager_printf( ch, "&cAffected by: &C%s\r\n",
                  !xIS_EMPTY( victim->affected_by ) ? ext_flag_string( &victim->affected_by,
                                                                       a_flags ) : "Nothing" );
/* pager_printf(ch, "&cAffected by2: &C%s\r\n", !xIS_EMPTY(victim->affected_by2)? 
ext_flag_string(&victim->affected_by2, a2_flags) : "Nothing"); */
    pager_printf( ch, "&cSpeaks: &w%d   &cSpeaking: &w%d   &cExperience: &w%d", victim->speaks,
                  victim->speaking, victim->exp );
    if ( !IS_NPC( victim ) && victim->wait )
        pager_printf( ch, "   &cWaitState: &R%d\r\n", victim->wait / 12 );
    else
        send_to_pager( "\r\n", ch );
    send_to_pager( "&cLanguages  : &w", ch );
    for ( x = 0; lang_array[x] != LANG_UNKNOWN; x++ ) {
        if ( knows_language( victim, lang_array[x], victim )
             || ( IS_NPC( victim ) && victim->speaks == 0 ) ) {
            if ( IS_SET( lang_array[x], victim->speaking )
                 || ( IS_NPC( victim ) && !victim->speaking ) )
                set_pager_color( AT_RED, ch );
            send_to_pager( lang_names[x], ch );
            send_to_pager( " ", ch );
            set_pager_color( AT_PLAIN, ch );
        }
        else if ( IS_SET( lang_array[x], victim->speaking )
                  || ( IS_NPC( victim ) && !victim->speaking ) ) {
            set_pager_color( AT_PINK, ch );
            send_to_pager( lang_names[x], ch );
            send_to_pager( " ", ch );
            set_pager_color( AT_PLAIN, ch );
        }
    }
    send_to_pager( "\r\n", ch );
    if ( victim->pcdata && VLD_STR( victim->pcdata->bestowments ) )
        pager_printf( ch, "&cBestowments: &w%s\r\n", victim->pcdata->bestowments );
    if ( IS_NPC( victim ) )
        pager_printf( ch, "&cShortdesc  : &w%s\r\n&cLongdesc   : &w%s",
                      VLD_STR( victim->short_descr ) ? victim->short_descr : "(none set)",
                      VLD_STR( victim->long_descr ) ? victim->long_descr : "(none set)\r\n" );
    else {
        if ( VLD_STR( victim->short_descr ) )
            pager_printf( ch, "&cShortdesc  : &w%s\r\n", victim->short_descr );
        if ( VLD_STR( victim->long_descr ) )
            pager_printf( ch, "&cLongdesc   : &w%s\r\n", victim->long_descr );
    }
    if ( IS_NPC( victim ) && victim->color )
        ch_printf( ch, "&cColor: &w%-2d\r\n", victim->color );
    if ( IS_NPC( victim ) && victim->spec_fun )
        pager_printf( ch, "&cMobile has spec fun: &w%s\r\n", lookup_spec( victim->spec_fun ) );
    if ( IS_NPC( victim ) )
        pager_printf( ch, "&cBody Parts : &w%s\r\n",
                      ext_flag_string( &victim->xflags, part_flags ) );
    if ( victim->resistant > 0 )
        pager_printf( ch, "&cResistant  : &w%s\r\n", flag_string( victim->resistant, ris_flags ) );
    if ( victim->immune > 0 )
        pager_printf( ch, "&cImmune     : &w%s\r\n", flag_string( victim->immune, ris_flags ) );
    if ( victim->susceptible > 0 )
        pager_printf( ch, "&cSusceptible: &w%s\r\n",
                      flag_string( victim->susceptible, ris_flags ) );
    if ( IS_NPC( victim ) ) {
        pager_printf( ch, "&cAttacks    : &w%s\r\n",
                      ext_flag_string( &victim->attacks, attack_flags ) );
        pager_printf( ch, "&cDefenses   : &w%s\r\n",
                      ext_flag_string( &victim->defenses, defense_flags ) );
    }
/*
    if( !IS_NPC( victim ) &&( victim->focus_level || victim->faith ) )
        pager_printf( ch, "&cFocus(&w%d&c) Faith(&W%d&c)&w%s&d\r\n", victim->focus_level, victim->faith );
*/
    for ( paf = victim->first_affect; paf; paf = paf->next ) {
        if ( ( ( skill = get_skilltype( paf->type ) ) != NULL ) ) {
            ch_printf( ch, "&c%s: &w'%s' mods %s", skill_tname[skill->type], skill->name,
                       a_types[paf->location % REVERSE_APPLY] );
            if ( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE
                 || paf->location == APPLY_SUSCEPTIBLE )
                ch_printf( ch, " to %s", flag_string( paf->modifier, ris_flags ) );
            else if ( paf->location == APPLY_AFFECT && paf->modifier == 0 ) {
                if ( !str_cmp( "Unknown", ext_flag_string( &paf->bitvector, a_flags ) ) )
                    ch_printf( ch, " by %s", print_bitvector( &paf->bitvector ) );
                else
                    ch_printf( ch, " by %s", ext_flag_string( &paf->bitvector, a_flags ) );
            }
            else
                ch_printf( ch, " by %d", paf->modifier );
            ch_printf( ch, " at lvl %d for %d rnds with bits %s.\r\n", paf->level, paf->duration,
                       !xIS_EMPTY( paf->bitvector ) ? ext_flag_string( &paf->bitvector,
                                                                       a_flags ) : "(none)" );
        }
    }
}

void do_mfind( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    MOB_INDEX_DATA         *pMobIndex;
    int                     hash;
    int                     nMatch;
    bool                    fAll;

    set_pager_color( AT_PLAIN, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Mfind whom?\r\n", ch );
        return;
    }

    fAll = !str_cmp( arg, "all" );
    nMatch = 0;

    /*
     * This goes through all the hash entry points (1024), and is therefore
     * much faster, though you won't get your vnums in order... oh well. :)
     *
     * Tests show that Furey's method will usually loop 32,000 times, calling
     * get_mob_index()... which loops itself, an average of 1-2 times...
     * So theoretically, the above routine may loop well over 40,000 times,
     * and my routine bellow will loop for as many index_mobiles are on
     * your mud... likely under 3000 times.
     * -Thoric
     */
    for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
        for ( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
            if ( fAll || nifty_is_name( arg, pMobIndex->player_name ) ) {
                nMatch++;
                pager_printf( ch, "[%5d] %s\r\n", pMobIndex->vnum,
                              capitalize( pMobIndex->short_descr ) );
            }

    if ( nMatch )
        pager_printf( ch, "Number of matches: %d\n", nMatch );
    else
        send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
    return;
}

void do_ofind( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_INDEX_DATA         *pObjIndex;
    int                     hash;
    int                     nMatch;
    bool                    fAll;

    set_pager_color( AT_PLAIN, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Ofind what?\r\n", ch );
        return;
    }

    fAll = !str_cmp( arg, "all" );
    nMatch = 0;

    /*
     * This goes through all the hash entry points (1024), and is therefore
     * much faster, though you won't get your vnums in order... oh well. :)
     *
     * Tests show that Furey's method will usually loop 32,000 times, calling
     * get_obj_index()... which loops itself, an average of 2-3 times...
     * So theoretically, the above routine may loop well over 50,000 times,
     * and my routine bellow will loop for as many index_objects are on
     * your mud... likely under 3000 times.
     * -Thoric
     */
    for ( hash = 0; hash < MAX_KEY_HASH; hash++ )
        for ( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
            if ( fAll || nifty_is_name( arg, pObjIndex->name ) ) {
                nMatch++;
                pager_printf( ch, "[%5d] %s\r\n", pObjIndex->vnum,
                              capitalize( pObjIndex->short_descr ) );
            }

    if ( nMatch )
        pager_printf( ch, "Number of matches: %d\n", nMatch );
    else
        send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
    return;
}

void do_mwhere( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    bool                    found;

    set_pager_color( AT_PLAIN, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Mwhere whom?\r\n", ch );
        return;
    }

    found = FALSE;
    for ( victim = first_char; victim; victim = victim->next ) {
        if ( IS_NPC( victim ) && victim->in_room && nifty_is_name( arg, victim->name ) ) {
            found = TRUE;
            pager_printf( ch, "[%5d] %-28s [%5d] %s\r\n", victim->pIndexData->vnum,
                          victim->short_descr, victim->in_room->vnum, victim->in_room->name );
        }
    }

    if ( !found )
        act( AT_PLAIN, "You didn't find any $T.", ch, NULL, arg, TO_CHAR );
    return;
}

void do_gwhere( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    DESCRIPTOR_DATA        *d;
    bool                    found = FALSE,
        pmobs = FALSE;
    int                     low = 1,
        high = MAX_LEVEL,
        count = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] != '\0' ) {
        if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
            send_to_pager
                ( "\r\n&wSyntax:  gwhere | gwhere <low> <high> | gwhere <low> <high> mobs\r\n",
                  ch );
            return;
        }
        low = atoi( arg1 );
        high = atoi( arg2 );
    }
    if ( low < 1 || high < low || low > high || high > MAX_LEVEL ) {
        send_to_pager( "&wInvalid level range.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg3 );
    if ( !str_cmp( arg3, "mobs" ) )
        pmobs = TRUE;

    pager_printf( ch, "\r\n&cGlobal %s locations:&w\r\n", pmobs ? "mob" : "player" );
    if ( !pmobs ) {
        for ( d = first_descriptor; d; d = d->next )
            if ( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
                 && ( victim = d->character ) != NULL && !IS_NPC( victim ) && victim->in_room
                 && can_see( ch, victim ) && victim->level >= low && victim->level <= high ) {
                found = TRUE;
                pager_printf( ch, "&c(&C%2d&c) &w%-12.12s   [%-5d - %-19.19s]   &c%-25.25s\r\n",
                              victim->level, victim->name, victim->in_room->vnum,
                              victim->in_room->area->name, victim->in_room->name );
                count++;
            }
    }
    else {
        for ( victim = first_char; victim; victim = victim->next )
            if ( IS_NPC( victim ) && victim->in_room && can_see( ch, victim )
                 && victim->level >= low && victim->level <= high ) {
                found = TRUE;
                pager_printf( ch, "&c(&C%2d&c) &w%-12.12s   [%-5d - %-19.19s]   &c%-25.25s\r\n",
                              victim->level, victim->name, victim->in_room->vnum,
                              victim->in_room->area->name, victim->in_room->name );
                count++;
            }
    }
    pager_printf( ch, "&c%d %s found.\r\n", count, pmobs ? "mobs" : "characters" );
    return;
}

void do_gfighting( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    DESCRIPTOR_DATA        *d;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    bool                    found = FALSE,
        pmobs = FALSE,
        phating = FALSE,
        phunting = FALSE;
    int                     low = 1,
        high = MAX_LEVEL,
        count = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] != '\0' ) {
        if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
            send_to_pager
                ( "\r\n&wSyntax:  gfighting | gfighting <low> <high> | gfighting <low> <high> mobs\r\n",
                  ch );
            return;
        }
        low = atoi( arg1 );
        high = atoi( arg2 );
    }
    if ( low < 1 || high < low || low > high || high > MAX_LEVEL ) {
        send_to_pager( "&wInvalid level range.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg3 );
    if ( !str_cmp( arg3, "mobs" ) )
        pmobs = TRUE;
    else if ( !str_cmp( arg3, "hating" ) )
        phating = TRUE;
    else if ( !str_cmp( arg3, "hunting" ) )
        phunting = TRUE;

    pager_printf( ch, "\r\n&cGlobal %s conflict:\r\n", pmobs ? "mob" : "character" );
    if ( !pmobs && !phating && !phunting ) {
        for ( d = first_descriptor; d; d = d->next )
            if ( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
                 && ( victim = d->character ) != NULL && !IS_NPC( victim ) && victim->in_room
                 && can_see( ch, victim ) && victim->fighting && victim->level >= low
                 && victim->level <= high ) {
                found = TRUE;
                pager_printf( ch,
                              "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                              victim->name, victim->level, victim->fighting->who->level,
                              IS_NPC( victim->fighting->who ) ? victim->fighting->
                              who->short_descr : victim->fighting->who->name,
                              IS_NPC( victim->fighting->who ) ? victim->fighting->who->pIndexData->
                              vnum : 0, victim->in_room->area->name,
                              victim->in_room == NULL ? 0 : victim->in_room->vnum );
                count++;
            }
    }
    else if ( !phating && !phunting ) {
        for ( victim = first_char; victim; victim = victim->next )
            if ( IS_NPC( victim ) && victim->in_room && can_see( ch, victim ) && victim->fighting
                 && victim->level >= low && victim->level <= high ) {
                found = TRUE;
                pager_printf( ch,
                              "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                              victim->name, victim->level, victim->fighting->who->level,
                              IS_NPC( victim->fighting->who ) ? victim->fighting->
                              who->short_descr : victim->fighting->who->name,
                              IS_NPC( victim->fighting->who ) ? victim->fighting->who->pIndexData->
                              vnum : 0, victim->in_room->area->name,
                              victim->in_room == NULL ? 0 : victim->in_room->vnum );
                count++;
            }
    }
    else if ( !phunting && phating ) {
        for ( victim = first_char; victim; victim = victim->next )
            if ( IS_NPC( victim ) && victim->in_room && can_see( ch, victim ) && victim->hating
                 && victim->level >= low && victim->level <= high ) {
                found = TRUE;
                pager_printf( ch,
                              "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                              victim->name, victim->level, victim->hating->who->level,
                              IS_NPC( victim->hating->who ) ? victim->hating->
                              who->short_descr : victim->hating->who->name,
                              IS_NPC( victim->hating->who ) ? victim->hating->who->pIndexData->
                              vnum : 0, victim->in_room->area->name,
                              victim->in_room == NULL ? 0 : victim->in_room->vnum );
                count++;
            }
    }
    else if ( phunting ) {
        for ( victim = first_char; victim; victim = victim->next )
            if ( IS_NPC( victim ) && victim->in_room && can_see( ch, victim ) && victim->hunting
                 && victim->level >= low && victim->level <= high ) {
                found = TRUE;
                pager_printf( ch,
                              "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                              victim->name, victim->level, victim->hunting->who->level,
                              IS_NPC( victim->hunting->who ) ? victim->hunting->
                              who->short_descr : victim->hunting->who->name,
                              IS_NPC( victim->hunting->who ) ? victim->hunting->who->pIndexData->
                              vnum : 0, victim->in_room->area->name,
                              victim->in_room == NULL ? 0 : victim->in_room->vnum );
                count++;
            }
    }
    pager_printf( ch, "&c%d %s conflicts located.\r\n", count, pmobs ? "mob" : "character" );
    return;
}

/* Added 'show' argument for lowbie imms without ostat -- Blodkai */
/* Made show the default action :) Shaddai */
/* Trimmed size, added vict info, put lipstick on the pig -- Blod */
void do_bodybag( CHAR_DATA *ch, char *argument )
{
    char                    buf2[MSL];
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *owner;
    OBJ_DATA               *obj;
    bool                    found = FALSE,
        bag = FALSE;

    argument = one_argument( argument, arg1 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\r\n", ch );
        return;
    }

    snprintf( buf2, MSL, "the corpse of %s", arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg2[0] != '\0'
         && ( str_cmp( arg2, "yes" ) && str_cmp( arg2, "bag" ) && str_cmp( arg2, "now" ) ) ) {
        send_to_char( "\r\n&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\r\n",
                      ch );
        return;
    }
    if ( !str_cmp( arg2, "yes" ) || !str_cmp( arg2, "bag" ) || !str_cmp( arg2, "now" ) )
        bag = TRUE;

    pager_printf( ch, "\r\n&P%s remains of %s ... ", bag ? "Retrieving" : "Searching for",
                  capitalize( arg1 ) );
    for ( obj = first_object; obj; obj = obj->next ) {
        if ( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == 11 ) ) {
            send_to_pager( "\r\n", ch );
            found = TRUE;
            pager_printf( ch,
                          "&P%s:  %s%-12.12s %s  &PIn:  &w%-22.22s  &P[&w%5d&P]   &PTimer:  %s%2d",
                          bag ? "Bagging" : "Corpse", bag ? "&Y" : "&w",
                          capitalize( arg1 ), IS_OBJ_STAT( obj,
                                                           ITEM_CLANCORPSE ) ? "&RPK" :
                          "&R  ", obj->in_room->area->name, obj->in_room->vnum,
                          obj->timer < 1 ? "&w" : obj->timer < 5 ? "&R" : obj->timer <
                          10 ? "&Y" : "&w", obj->timer );
            if ( bag ) {
                obj_from_room( obj );
                obj = obj_to_char( obj, ch );
                obj->timer = -1;
                save_char_obj( ch );
            }
        }
    }
    if ( !found ) {
        send_to_pager( "&Pno corpse was found.\r\n", ch );
        return;
    }
    send_to_pager( "\r\n", ch );
    for ( owner = first_char; owner; owner = owner->next ) {
        if ( IS_NPC( owner ) )
            continue;
        if ( can_see( ch, owner ) && !str_cmp( arg1, owner->name ) )
            break;
    }
    if ( owner == NULL ) {
        pager_printf( ch, "&P%s is not currently online.\r\n", capitalize( arg1 ) );
        return;
    }
    return;
}

/*
 * Truncate a char string if it's length exceeds a given value. Only used in do_owhere, but 
 * I see no point in repetitiously typing out the code. -Orion
 * Renamed to strunc because it gives warnings when named trunc. - Remcon
 */
void strunc( char *s, u_int len )
{
    if ( strlen( s ) > len )
        s[len] = '\0';
}

/*
 * OWhere by Gorog. -Orion
 */
//Bugfix below was crashing. -Taon
void do_owhere( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL],
                            field[MIL];
    char                    arg[MIL];
    OBJ_DATA               *obj,
                           *outer_obj;
    bool                    found = FALSE;
    int                     icnt = 0,
        vnum = 0;
    char                    heading[] =
        "     Vnum Short Desc         Vnum Room/Char          Vnum  Container\r\n";

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        pager_printf( ch, "Owhere what?\r\n" );
        return;
    }
    if ( is_number( arg ) )
        vnum = atoi( arg );

    for ( obj = first_object; obj; obj = obj->next ) {
        if ( vnum ) {
            if ( vnum != obj->pIndexData->vnum )
                continue;
        }
        else if ( !nifty_is_name( arg, obj->name ) )
            continue;

        if ( !found )
            send_to_pager( heading, ch );              /* print report heading */
        found = TRUE;

        outer_obj = obj;
        while ( outer_obj->in_obj )
            outer_obj = outer_obj->in_obj;

        snprintf( field, MIL, "%-18s", obj_short( obj ) );
        strunc( field, 18 );
        snprintf( buf, MSL, "%3d %5d %-18s ", ++icnt, obj->pIndexData->vnum, field );
        if ( outer_obj->carried_by ) {
            snprintf( field, MIL, "%-18s", PERS( outer_obj->carried_by, ch ) );
            strunc( field, 18 );
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%5d %-18s ",
                      ( IS_NPC( outer_obj->carried_by ) ? outer_obj->carried_by->pIndexData->
                        vnum : 0 ), field );
            if ( outer_obj != obj ) {
                snprintf( field, MIL, "%-18s", obj->in_obj->name );
                strunc( field, 18 );
                snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%5d %-18s ",
                          obj->in_obj->pIndexData->vnum, field );
            }
            mudstrlcat( buf, "\r\n", MSL );
            send_to_pager( buf, ch );
        }
        else if ( outer_obj->in_room ) {
            snprintf( field, MIL, "%-18s", outer_obj->in_room->name );
            strunc( field, 18 );
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%5d %-18s ",
                      outer_obj->in_room->vnum, field );
            if ( outer_obj != obj ) {
                snprintf( field, MIL, "%-18s", obj->in_obj->name );
                strunc( field, 18 );
                snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%5d %-18s ",
                          obj->in_obj->pIndexData->vnum, field );
            }
            mudstrlcat( buf, "\r\n", MSL );
            send_to_pager( buf, ch );
        }
    }
    if ( !found )
        pager_printf( ch, "None found.\r\n" );
}

void do_reboot( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    CHAR_DATA              *vch;

    set_char_color( AT_IMMORT, ch );
    if ( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" )
         && str_cmp( argument, "and sort skill table" ) ) {
        send_to_char( "Syntax:  'reboot mud now' or 'reboot nosave'\r\n", ch );
        return;
    }
    if ( auction->item )
        do_auction( ch, ( char * ) "stop" );
    snprintf( buf, MSL, "Reboot by %s.", ch->name );
    do_echo( ch, buf );
    if ( !str_cmp( argument, "and sort skill table" ) ) {
        sort_skill_table(  );
        save_skill_table(  );
    }
    /*
     * Save all characters before booting. 
     */
    if ( str_cmp( argument, "nosave" ) )
        for ( vch = first_char; vch; vch = vch->next )
            if ( !IS_NPC( vch ) )
                save_char_obj( vch );
    mud_down = TRUE;
    return;
}

void do_shutdown( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];

/*    extern bool mud_down;  */
    CHAR_DATA              *vch;

    set_char_color( AT_IMMORT, ch );

    if ( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) ) {
        send_to_char( "Syntax:  'shutdown mud now' or 'shutdown nosave'\r\n", ch );
        return;
    }

    if ( auction->item )
        do_auction( ch, ( char * ) "stop" );
    snprintf( buf, MSL, "Shutdown by %s.", ch->name );
    append_file( ch, SHUTDOWN_FILE, buf );
    mudstrlcat( buf, "\r\n", MSL );
    do_echo( ch, buf );

    /*
     * Save all characters before booting. 
     */
    if ( str_cmp( argument, "nosave" ) )
        for ( vch = first_char; vch; vch = vch->next )
            if ( !IS_NPC( vch ) )
                save_char_obj( vch );
    mud_down = TRUE;
    return;
}

void do_snoop( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    DESCRIPTOR_DATA        *d;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Snoop whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( !victim->desc ) {
        send_to_char( "No descriptor to snoop.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "Cancelling all snoops.\r\n", ch );
        for ( d = first_descriptor; d; d = d->next )
            if ( d->snoop_by == ch->desc )
                d->snoop_by = NULL;
        return;
    }
    if ( victim->desc->snoop_by ) {
        send_to_char( "Busy already.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to snoop you against your wishes.", ch, NULL, victim, TO_VICT );
        return;

    }
    if ( ch->desc ) {
        for ( d = ch->desc->snoop_by; d; d = d->snoop_by )
            if ( d->character == victim || d->original == victim ) {
                send_to_char( "No snoop loops.\r\n", ch );
                return;
            }
    }

/*  Snoop notification for higher imms, if desired, uncomment this */
#ifdef TOOSNOOPY
    if ( get_trust( victim ) > LEVEL_AJ_CPL && get_trust( ch ) < LEVEL_AJ_GENERAL )
        write_to_descriptor( victim->desc->descriptor,
                             "\r\nYou feel like someone is watching your every move...\r\n", 0 );
#endif

    victim->desc->snoop_by = ch->desc;
    send_to_char( "&cYou utter the arcane words, 'Walschyou'.\r\n", ch );
    act( AT_YELLOW,
         "A common house hold fly appears in your hand, then is transported to follow your victim.",
         ch, NULL, NULL, TO_CHAR );
    send_to_char( "&cYou smile, as you now see everything your victim does and says.\r\n", ch );
    return;
}

void do_statshield( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( IS_NPC( ch ) || get_trust( ch ) < LEVEL_AJ_LT ) {
        error( ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "Statshield which mobile?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "No such mobile.\r\n", ch );
        return;
    }
    if ( !IS_NPC( victim ) ) {
        send_to_char( "You can only statshield mobiles.\r\n", ch );
        return;
    }
    if ( xIS_SET( victim->act, ACT_STATSHIELD ) ) {
        xREMOVE_BIT( victim->act, ACT_STATSHIELD );
        ch_printf( ch, "You have lifted the statshield on %s.\r\n", victim->short_descr );
    }
    else {
        xSET_BIT( victim->act, ACT_STATSHIELD );
        ch_printf( ch, "You have applied a statshield to %s.\r\n", victim->short_descr );
    }
    return;
}

void do_switch( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Switch into whom?\r\n", ch );
        return;
    }
    if ( !ch->desc )
        return;
    if ( ch->desc->original ) {
        send_to_char( "You are already switched.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_STATSHIELD )
         && get_trust( ch ) < LEVEL_AJ_LT ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Their godly glow prevents you from getting close enough.\r\n", ch );
        return;
    }
    if ( victim->desc ) {
        send_to_char( "Character in use.\r\n", ch );
        return;
    }
    if ( !IS_NPC( victim ) && ch->level < LEVEL_AJ_SGT
         && ( !victim->redirect || victim->redirect != ch ) ) {
        send_to_char( "You cannot switch into a player!\r\n", ch );
        return;
    }
    if ( victim->switched ) {
        send_to_char( "You can't switch into a player that is switched!\r\n", ch );
        return;
    }
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_FREEZE ) ) {
        send_to_char( "You shouldn't switch into a player that is frozen!\r\n", ch );
        return;
    }
    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;
    ch->switched = victim;
    send_to_char( "Ok.\r\n", victim );
    return;
}

void do_return( CHAR_DATA *ch, char *argument )
{

    if ( !IS_NPC( ch ) && get_trust( ch ) < LEVEL_IMMORTAL ) {
        error( ch );
        return;
    }
    set_char_color( AT_IMMORT, ch );

    if ( !ch->desc )
        return;
    if ( !ch->desc->original ) {
        send_to_char( "You aren't switched.\r\n", ch );
        return;
    }

    send_to_char( "You return to your original body.\r\n", ch );

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_POSSESS ) ) {
        affect_strip( ch, gsn_possess );
        xREMOVE_BIT( ch->affected_by, AFF_POSSESS );
    }

    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc->character->switched = NULL;
    ch->desc = NULL;
    return;
}

void do_minvoke( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    MOB_INDEX_DATA         *pMobIndex;
    CHAR_DATA              *victim;
    int                     vnum;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax:  minvoke <vnum>\r\n", ch );
        return;
    }
    if ( !is_number( arg ) ) {
        char                    arg2[MIL];
        int                     hash,
                                cnt;
        int                     count = number_argument( arg, arg2 );

        vnum = -1;
        for ( hash = cnt = 0; hash < MAX_KEY_HASH; hash++ )
            for ( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
                if ( nifty_is_name( arg2, pMobIndex->player_name ) && ++cnt == count ) {
                    vnum = pMobIndex->vnum;
                    break;
                }
        if ( vnum == -1 ) {
            send_to_char( "No such mobile exists.\r\n", ch );
            return;
        }
    }
    else
        vnum = atoi( arg );

    if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        AREA_DATA              *pArea;

        if ( IS_NPC( ch ) ) {
            error( ch );
            return;
        }
        if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
            send_to_char( "You must have an assigned area to invoke this mobile.\r\n", ch );
            return;
        }
        if ( vnum < pArea->low_m_vnum || vnum > pArea->hi_m_vnum ) {
            send_to_char( "That number is not in your allocated range.\r\n", ch );
            return;
        }
    }
    if ( !( pMobIndex = get_mob_index( vnum ) ) ) {
        send_to_char( "No mobile has that vnum.\r\n", ch );
        return;
    }

    if ( first_char->prev )
        send_to_char( "First_char already has a prev.\r\n", ch );
    victim = create_mobile( pMobIndex );
    if ( first_char->prev )
        send_to_char( "First_char has a prev after create_mobile.\r\n", ch );
    if ( !victim ) {
        send_to_char( "Failed to minvoke the mobile.\r\n", ch );
        return;
    }

    char_to_room( victim, ch->in_room );
    if ( first_char->prev )
        send_to_char( "First_char has a prev after char_to_room.\r\n", ch );
    act( AT_IMMORT, "$n invokes $N!", ch, NULL, victim, TO_ROOM );
    ch_printf( ch, "&YYou invoke %s (&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\r\n", pMobIndex->short_descr,
               pMobIndex->vnum, pMobIndex->player_name, victim->level );
}

void do_oinvoke( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    buf[MSL];
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *obj;
    int                     vnum;
    int                     level;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: oinvoke <vnum> <level>.\r\n", ch );
        return;
    }
    if ( arg2[0] == '\0' ) {
        level = get_trust( ch );
    }
    else {
        if ( !is_number( arg2 ) ) {
            send_to_char( "Syntax:  oinvoke <vnum> <level>\r\n", ch );
            return;
        }
        level = atoi( arg2 );
        if ( level < 0 || level > get_trust( ch ) ) {
            send_to_char( "Limited to your trust level.\r\n", ch );
            return;
        }
    }
    if ( !is_number( arg1 ) ) {
        char                    arg[MIL];
        int                     hash,
                                cnt;
        int                     count = number_argument( arg1, arg );

        vnum = -1;
        for ( hash = cnt = 0; hash < MAX_KEY_HASH; hash++ )
            for ( pObjIndex = obj_index_hash[hash]; pObjIndex; pObjIndex = pObjIndex->next )
                if ( nifty_is_name( arg, pObjIndex->name ) && ++cnt == count ) {
                    vnum = pObjIndex->vnum;
                    break;
                }
        if ( vnum == -1 ) {
            send_to_char( "No such object exists.\r\n", ch );
            return;
        }
    }
    else
        vnum = atoi( arg1 );

    if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        AREA_DATA              *pArea;

        if ( IS_NPC( ch ) ) {
            error( ch );
            return;
        }
        if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
            send_to_char( "You must have an assigned area to invoke this object.\r\n", ch );
            return;
        }
        if ( vnum < pArea->low_o_vnum || vnum > pArea->hi_o_vnum ) {
            send_to_char( "That number is not in your allocated range.\r\n", ch );
            return;
        }
    }
    if ( ( pObjIndex = get_obj_index( vnum ) ) == NULL ) {
        send_to_char( "No object has that vnum.\r\n", ch );
        return;
    }

    if ( level == 0 ) {
        AREA_DATA              *temp_area;

        if ( ( temp_area = get_area_obj( pObjIndex ) ) == NULL )
            level = ch->level;
        else {
            level = generate_itemlevel( temp_area, pObjIndex );
            level = URANGE( 0, level, LEVEL_DEMIGOD );
        }
    }

    obj = create_object( pObjIndex, level );
    if ( CAN_WEAR( obj, ITEM_TAKE ) ) {
        obj = obj_to_char( obj, ch );
    }
    else {
        obj = obj_to_room( obj, ch->in_room );
        act( AT_IMMORT, "$n fashions $p from ether!", ch, obj, NULL, TO_ROOM );
    }
    /*
     * I invoked what? --Blodkai 
     */
    ch_printf( ch, "&YYou invoke %s (&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\r\n", pObjIndex->short_descr,
               pObjIndex->vnum, pObjIndex->name, obj->level );
    snprintf( buf, MSL, "%-20s has invoked %s.", ch->name, pObjIndex->short_descr );
    append_to_file( VLOG_FILE, buf );
    return;
}

void do_purge( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        /*
         * 'purge' 
         */
        CHAR_DATA              *vnext;
        OBJ_DATA               *obj_next;

        for ( victim = ch->in_room->first_person; victim; victim = vnext ) {
            vnext = victim->next_in_room;
            if ( IS_NPC( victim ) && victim != ch )
                extract_char( victim, TRUE );
        }

        for ( obj = ch->in_room->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            extract_obj( obj );
        }

        act( AT_IMMORT, "$n purges the room!", ch, NULL, NULL, TO_ROOM );
        act( AT_IMMORT, "You have purged the room!", ch, NULL, NULL, TO_CHAR );
        return;
    }
    victim = NULL;
    obj = NULL;

    /*
     * fixed to get things in room first -- i.e., purge portal (obj),
     * * no more purging mobs with that keyword in another room first
     * * -- Tri 
     */
    if ( ( victim = get_char_room( ch, arg ) ) == NULL
         && ( obj = get_obj_here( ch, arg ) ) == NULL ) {
        if ( ( victim = get_char_world( ch, arg ) ) == NULL && ( obj = get_obj_world( ch, arg ) ) == NULL ) {   /* no 
                                                                                                                 * get_obj_room 
                                                                                                                 */
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
    }

/* Single object purge in room for high level purge - Scryn 8/12*/
    if ( obj ) {
        separate_obj( obj );
        act( AT_IMMORT, "$n purges $p.", ch, obj, NULL, TO_ROOM );
        act( AT_IMMORT, "You make $p disappear in a puff of smoke!", ch, obj, NULL, TO_CHAR );
        extract_obj( obj );
        return;
    }

    if ( !IS_NPC( victim ) ) {
        send_to_char( "Not on PC's.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You cannot purge yourself!\r\n", ch );
        return;
    }

    act( AT_IMMORT, "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    act( AT_IMMORT, "You make $N disappear in a puff of smoke!", ch, NULL, victim, TO_CHAR );
    extract_char( victim, TRUE );
    return;
}

void do_low_purge( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Purge what?\r\n", ch );
        return;
    }

    victim = NULL;
    obj = NULL;
    if ( ( victim = get_char_room( ch, arg ) ) == NULL
         && ( obj = get_obj_here( ch, arg ) ) == NULL ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return;
    }

    if ( obj ) {
        separate_obj( obj );
        act( AT_IMMORT, "$n purges $p!", ch, obj, NULL, TO_ROOM );
        act( AT_IMMORT, "You make $p disappear in a puff of smoke!", ch, obj, NULL, TO_CHAR );
        extract_obj( obj );
        return;
    }

    if ( !IS_NPC( victim ) ) {
        send_to_char( "Not on PC's.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You cannot purge yourself!\r\n", ch );
        return;
    }

    act( AT_IMMORT, "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
    act( AT_IMMORT, "You make $N disappear in a puff of smoke!", ch, NULL, victim, TO_CHAR );
    extract_char( victim, TRUE );
    return;
}

void do_balzhur( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MSL];
    char                    buf2[MSL];
    char                   *name;
    CHAR_DATA              *victim;
    AREA_DATA              *pArea;
    int                     sn;

    set_char_color( AT_BLOOD, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Who is deserving of such a fate?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't currently playing.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "This will do little good on mobiles.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to Balzhur you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    if ( victim->level >= LEVEL_IMMORTAL ) {
        interpret( victim, ( char * ) "holylight" );
        interpret( victim, ( char * ) "shadowform" );
        xSET_BIT( victim->act, PLR_STAFF );
        REMOVE_BIT( victim->pcdata->flags, PCFLAG_TEMP );
        GET_MONEY( victim, CURR_GOLD ) = 0;
        GET_MONEY( victim, CURR_SILVER ) = 0;
        GET_MONEY( victim, CURR_BRONZE ) = 0;
        GET_MONEY( victim, CURR_COPPER ) = 0;
        STRFREE( victim->pcdata->council_name );
        victim->pcdata->council = NULL;
        interpret( victim, ( char * ) "speak common" );
        interpret( victim, ( char * ) "title a regular player." );
        victim->perm_str = 15;
        victim->perm_int = 13;
        victim->perm_wis = 12;
        victim->perm_dex = 15;
        victim->perm_con = 12;
        victim->perm_cha = 15;
        victim->perm_lck = 14;

        interpret( victim, ( char * ) "save" );
    }

    set_char_color( AT_WHITE, ch );
    send_to_char( "You summon the demon Balzhur to wreak your wrath!\r\n", ch );
    send_to_char( "Balzhur sneers at you evilly, then vanishes in a puff of smoke.\r\n", ch );
    set_char_color( AT_IMMORT, victim );
    send_to_char( "You hear an ungodly sound in the distance that makes your blood run cold!\r\n",
                  victim );
    snprintf( buf, MSL, "Balzhur screams, 'You are MINE %s!!!'", victim->name );
    echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
    victim->level = 2;
    victim->trust = 0;
    victim->exp = 2000;
    victim->max_hit = 10;
    victim->max_mana = 100;
    victim->max_move = 100;
    victim->pcdata->apply_blood = 0;
    for ( sn = 0; sn < top_sn; sn++ )
        victim->pcdata->learned[sn] = 0;
    victim->practice = 0;
    victim->hit = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    name = capitalize( victim->name );
    snprintf( buf, MSL, "%s%s", STAFF_DIR, name );
    set_char_color( AT_RED, ch );
    if ( !remove( buf ) )
        send_to_char( "Player's immortal data destroyed.\r\n", ch );
    else if ( errno != ENOENT ) {
        ch_printf( ch, "Unknown error #%d - %s (immortal data).  Report to Thoric\r\n", errno,
                   strerror( errno ) );
        snprintf( buf2, MSL, "%s balzhuring %s", ch->name, buf );
        perror( buf2 );
    }
    snprintf( buf2, MSL, "%s.are", name );
    for ( pArea = first_build; pArea; pArea = pArea->next )
        if ( !str_cmp( pArea->filename, buf2 ) ) {
            snprintf( buf, MSL, "%s%s", BUILD_DIR, buf2 );
            if ( IS_SET( pArea->status, AREA_LOADED ) ) {
                fold_area( pArea, buf, FALSE );
                close_area( pArea );
            }
            snprintf( buf2, MSL, "%s.bak", buf );
            set_char_color( AT_RED, ch );              /* Log message changes colors */
            remove( buf2 );
            if ( !rename( buf, buf2 ) )
                send_to_char( "Player's area data destroyed.  Area saved as backup.\r\n", ch );
            else if ( errno != ENOENT ) {
                ch_printf( ch, "Unknown error #%d - %s (area data).  Report to  Thoric.\r\n", errno,
                           strerror( errno ) );
                snprintf( buf2, MSL, "%s destroying %s", ch->name, buf );
                perror( buf2 );
            }
            break;
        }

    advance_level( victim );
    do_help( victim, ( char * ) "M_BALZHUR_" );
    set_char_color( AT_WHITE, victim );
    send_to_char( "You awake after a long period of time...\r\n", victim );
    while ( victim->first_carrying )
        extract_obj( victim->first_carrying );
    return;
}

void do_advance( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    buf[MIL];
    CHAR_DATA              *victim;
    int                     level;
    int                     iLevel;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) ) {
        send_to_char( "Syntax:  advance <character> <level>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "That character is not on the game.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You cannot advance a mobile.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) || ch == victim ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to demote you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }
    if ( ( level = atoi( arg2 ) ) < 1 || level > MAX_LEVEL ) {
        send_to_char( "Level range is 1 to 108.\r\n", ch );
        return;
    }
    if ( level > get_trust( ch ) ) {
        send_to_char( "Level limited to your trust level.\r\n", ch );
        return;
    }
    /*
     * Lower level:
     * *   Reset to level 1.
     * *   Then raise again.
     * *   Currently, an imp can lower another imp.
     * *   -- Swiftest
     * *   Can't lower imms >= your trust (other than self) per Narn's change.
     * *   Few minor text changes as well.  -- Blod
     */
    if ( level <= victim->level ) {
        int                     sn;

        set_char_color( AT_IMMORT, victim );

        if ( victim->level > LEVEL_DEMIGOD || IS_IMMORTAL( victim ) ) {
            if ( victim->pcdata->bestowments )
                STRFREE( victim->pcdata->bestowments );
            xREMOVE_BIT( victim->act, PLR_SHADOWFORM );
            xREMOVE_BIT( victim->act, PLR_HOLYLIGHT );
            if ( !IS_RETIRED( victim ) ) {
                snprintf( buf, MIL, "%s%s", STAFF_DIR, capitalize( ch->name ) );
                remove( buf );
            }
        }

        if ( level < victim->level ) {
            ch_printf( ch, "Demoting %s from level %d to level %d!\r\n", victim->name,
                       victim->level, level );
            send_to_char( "Cursed and forsaken!  The gods have lowered your level...\r\n", victim );
        }
        else {
            ch_printf( ch, "%s is already level %d.  Re-advancing...\r\n", victim->name, level );
            send_to_char( "Deja vu!  Your mind reels as you re-live your past levels!\r\n",
                          victim );
        }
        victim->level = 1;
        victim->exp = 0;
        victim->max_hit = 20;
        victim->max_mana = 100;
        victim->max_move = 100;
        victim->pcdata->apply_blood = 0;
        for ( sn = 0; sn < top_sn; sn++ )
            victim->pcdata->learned[sn] = 0;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        advance_level( victim );
        if ( victim->pcdata->rank )
            STRFREE( victim->pcdata->rank );
        victim->pcdata->wizinvis = victim->trust;
        if ( xIS_SET( victim->act, PLR_WIZINVIS ) && ( victim->level <= LEVEL_DEMIGOD ) ) {
            xREMOVE_BIT( victim->act, PLR_WIZINVIS );
            victim->pcdata->wizinvis = victim->trust;
        }
    }
    else {
        ch_printf( ch, "Raising %s from level %d to level %d!\r\n", victim->name, victim->level,
                   level );
        if ( IS_DEMIGOD( victim ) ) {
            set_char_color( AT_IMMORT, victim );
            act( AT_IMMORT,
                 "$n makes some arcane gestures with $s hands, then points $s finger at you!", ch,
                 NULL, victim, TO_VICT );
            act( AT_IMMORT,
                 "$n makes some arcane gestures with $s hands, then points $s finger at $N!", ch,
                 NULL, victim, TO_NOTVICT );
            set_char_color( AT_WHITE, victim );
            send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
            set_char_color( AT_LBLUE, victim );
        }
        switch ( level ) {
            default:
                send_to_char( "The gods feel fit to raise your level!\r\n", victim );
                break;
            case LEVEL_IMMORTAL:
                do_help( victim, ( char * ) "M_GODLVL1_" );
                set_char_color( AT_WHITE, victim );
                send_to_char( "You awake... all your possessions are gone.\r\n", victim );
                while ( victim->first_carrying )
                    extract_obj( victim->first_carrying );
                break;
            case LEVEL_AJ_CPL:
                do_help( victim, ( char * ) "M_GODLVL2_" );
                break;
            case LEVEL_AJ_SGT:
                do_help( victim, ( char * ) "M_GODLVL3_" );
                break;
            case LEVEL_AJ_COLONEL:
                do_help( victim, ( char * ) "M_GODLVL4_" );
                break;
            case LEVEL_AJ_GENERAL:
                do_help( victim, ( char * ) "M_GODLVL5_" );
                break;
        }
    }
    for ( iLevel = victim->level; iLevel < level; iLevel++ ) {
        if ( level < LEVEL_IMMORTAL )
            send_to_char( "You raise a level!!\r\n", victim );
        victim->level += 1;
        advance_level( victim );
    }
    victim->exp = 0;
    victim->trust = 0;
    return;
}

void do_elevate( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: elevate <char>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( victim->level == LEVEL_IMMORTAL ) {
        send_to_char( "Elevating a player...\r\n", ch );
        set_char_color( AT_IMMORT, victim );
        act( AT_IMMORT, "$n begins to chant softly... then makes some arcane gestures...", ch, NULL,
             NULL, TO_ROOM );
        set_char_color( AT_WHITE, victim );
        send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
        set_char_color( AT_LBLUE, victim );
        do_help( victim, ( char * ) "M_GODLVL2_" );
        victim->level = LEVEL_AJ_CPL;
        set_char_color( AT_WHITE, victim );
        advance_level( victim );
        victim->exp = 0;
        victim->trust = 0;
        return;
    }
    if ( victim->level == LEVEL_AJ_CPL ) {
        send_to_char( "Elevating a player...\r\n", ch );
        set_char_color( AT_IMMORT, victim );
        act( AT_IMMORT, "$n begins to chant softly... then makes some arcane gestures...", ch, NULL,
             NULL, TO_ROOM );
        set_char_color( AT_WHITE, victim );
        send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
        set_char_color( AT_LBLUE, victim );
        do_help( victim, ( char * ) "M_GODLVL3_" );
        victim->level = LEVEL_AJ_SGT;
        set_char_color( AT_WHITE, victim );
        advance_level( victim );
        victim->exp = 0;
        victim->trust = 0;
        return;
    }
    else
        send_to_char( "You cannot elevate this character.\r\n", ch );
    return;
}

void do_immortalize( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax:  immortalize <char>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( victim->level != LEVEL_DEMIGOD ) {
        send_to_char( "This player is not yet worthy of immortality.\r\n", ch );
        return;
    }

    send_to_char( "Immortalizing a player...\r\n", ch );
    set_char_color( AT_IMMORT, victim );
    act( AT_IMMORT, "$n begins to chant softly... then raises $s arms to the sky...", ch, NULL,
         NULL, TO_ROOM );
    set_char_color( AT_WHITE, victim );
    send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
    set_char_color( AT_LBLUE, victim );
    do_help( victim, ( char * ) "M_GODLVL1_" );
    set_char_color( AT_WHITE, victim );
    send_to_char( "You awake... all your possessions are gone.\r\n", victim );
    while ( victim->first_carrying )
        extract_obj( victim->first_carrying );
    victim->level = LEVEL_IMMORTAL;
    advance_level( victim );

    /*
     * Remove clan and update accordingly 
     */
    if ( victim->pcdata->clan ) {
        --victim->pcdata->clan->members;
        victim->pcdata->clan = NULL;
        STRFREE( victim->pcdata->clan_name );
    }
    victim->exp = 0;
    victim->trust = 0;
    save_char_obj( victim );
    return;
}

void do_trust( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    int                     level;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) ) {
        send_to_char( "Syntax:  trust <char> <level>.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }
    if ( ( level = atoi( arg2 ) ) < 0 || level > MAX_LEVEL ) {
        ch_printf( ch, "Level must be 0 (reset) or 1 to %d.\r\n", MAX_LEVEL );
        return;
    }
    if ( level > get_trust( ch ) ) {
        send_to_char( "Limited to your own trust.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to change your trust level against your wishes.", ch, NULL, victim,
             TO_VICT );
        return;
    }

    victim->trust = level;
    send_to_char( "Ok.\r\n", ch );
    return;
}

/* Summer 1997 --Blod */
void do_scatter( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    ROOM_INDEX_DATA        *pRoomIndex;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Scatter whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "It's called teleport.  Try it.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to scatter you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    for ( ;; ) {
        pRoomIndex = get_room_index( number_range( 0, MAX_VNUM ) );
        if ( pRoomIndex )
            if ( !IS_SET( pRoomIndex->room_flags, ROOM_PRIVATE )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_SOLITARY )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_NO_ASTRAL )
                 && !IS_SET( pRoomIndex->room_flags, ROOM_PROTOTYPE ) )
                break;
    }
    if ( victim->fighting )
        stop_fighting( victim, TRUE );
    act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the winds.", ch, NULL, victim,
         TO_NOTVICT );
    act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim,
         TO_VICT );
    act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim,
         TO_CHAR );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    set_position( victim, POS_RESTING );
    act( AT_MAGIC, "$n staggers forth from a sudden gust of wind, and collapses.", victim, NULL,
         NULL, TO_ROOM );
    do_look( victim, ( char * ) "auto" );
    return;
}

/*
void do_strew(CHAR_DATA *ch, char *argument)
{
    char arg1 [MIL];
    char arg2 [MIL];
    CHAR_DATA *victim;
    OBJ_DATA *obj_next;
    OBJ_DATA *obj_lose;
    ROOM_INDEX_DATA *pRoomIndex;

    set_char_color(AT_IMMORT, ch);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    if(arg1[0] == '\0' || arg2[0] == '\0') {
      send_to_char("Strew who, what?\r\n", ch);
      return;
    }
    if( (victim = get_char_room(ch, arg1)) == NULL) {
      send_to_char("It would work better if they were here.\r\n", ch);
      return;
    }
    if(victim == ch) {
      send_to_char("Try taking it out on someone else first.\r\n", ch);
      return;
    }
 if( get_trust( victim ) >= get_trust(ch))
{
send_to_char("&RNever try to use a command against a higher Staff member...\r\n", ch);
send_to_char("&RYou have been warned....\r\n", ch);
act(AT_RED, "$n tried to strew you against your wishes.", ch, NULL, victim, TO_VICT);
        return;
}

    if(!str_cmp(arg2, "coins")) {
      if(victim->gold < 1) {
        send_to_char("Drat, this one's got no gold to start with.\r\n", ch);
        return;
      }
      victim->gold = 0;
      act(AT_MAGIC, "$n gestures and an unearthly gale sends $N's coins flying!", ch, NULL, victim, TO_NOTVICT);
      act(AT_MAGIC, "You gesture and an unearthly gale sends $N's coins flying!", ch, NULL, victim, TO_CHAR);
      act(AT_MAGIC, "As $n gestures, an unearthly gale sends your currency flying!", ch, NULL, victim, TO_VICT);
      return;
    }
    for(; ;) {
      pRoomIndex = get_room_index(number_range(0, MAX_VNUM));
        if(pRoomIndex)
          if(!IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
          &&   !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
          &&   !IS_SET(pRoomIndex->room_flags, ROOM_NO_ASTRAL)
          &&   !IS_SET(pRoomIndex->room_flags, ROOM_PROTOTYPE))
        break;
    }
    if(!str_cmp(arg2, "inventory")) {
      act(AT_MAGIC, "$n speaks a single word, sending $N's possessions flying!", ch, NULL, victim, TO_NOTVICT);
      act(AT_MAGIC, "You speak a single word, sending $N's possessions flying!", ch, NULL, victim, TO_CHAR);
      act(AT_MAGIC, "$n speaks a single word, sending your possessions flying!", ch, NULL, victim, TO_VICT);
      for(obj_lose=victim->first_carrying; obj_lose; obj_lose=obj_next) {
        obj_next = obj_lose->next_content;
        obj_from_char(obj_lose);
        obj_to_room(obj_lose, pRoomIndex);
        pager_printf(ch, "\t&w%s sent to %d\r\n", capitalize(obj_lose->short_descr), pRoomIndex->vnum);
      }
      return;
    }  
    send_to_char("Strew their coins or inventory?\r\n", ch);
    return;
}
*/

void do_strip( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj_next;
    OBJ_DATA               *obj_lose;
    int                     count = 0;

    set_char_color( AT_OBJECT, ch );
    if ( !argument ) {
        send_to_char( "Strip who?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
        send_to_char( "They're not here.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "Kinky.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to strip you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    act( AT_OBJECT, "Searching $N ...", ch, NULL, victim, TO_CHAR );
    for ( obj_lose = victim->first_carrying; obj_lose; obj_lose = obj_next ) {
        obj_next = obj_lose->next_content;
        obj_from_char( obj_lose );
        obj_to_char( obj_lose, ch );
        pager_printf( ch, "  &G... %s (&g%s) &Gtaken.\r\n", capitalize( obj_lose->short_descr ),
                      obj_lose->name );
        count++;
    }
    if ( !count )
        pager_printf( ch, "&GNothing found to take.\r\n", ch );
    return;
}

void do_restore( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    bool                    boost = FALSE;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Restore whom?\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg2 );
    if ( !str_cmp( arg2, "boost" ) && get_trust( ch ) >= LEVEL_AJ_SGT ) {
        send_to_char( "Boosting!\r\n", ch );
        boost = TRUE;
    }
    if ( !str_cmp( arg, "all" ) ) {
        CHAR_DATA              *vch;
        CHAR_DATA              *vch_next;

        if ( !ch->pcdata )
            return;

        if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
            if ( IS_NPC( ch ) ) {
                send_to_char( "You can't do that.\r\n", ch );
                return;
            }
            else {
                /*
                 * Check if the player did a restore all within the last 18 hours. 
                 */
                if ( current_time - last_restore_all_time < RESTORE_INTERVAL ) {
                    send_to_char( "Sorry, you can't do a restore all yet.\r\n", ch );
                    do_restoretime( ch, ( char * ) "" );
                    return;
                }
            }
        }
        last_restore_all_time = current_time;
        ch->pcdata->restore_time = current_time;
        save_char_obj( ch );
        send_to_char( "Beginning 'restore all' ...\r\n", ch );
        for ( vch = first_char; vch; vch = vch_next ) {
            vch_next = vch->next;

            if ( !IS_NPC( vch ) && !in_arena( vch ) ) {
                if ( boost ) {
                    vch->hit = vch->max_hit * 2;
                    vch->mana = vch->max_mana * 2;
                    vch->move = vch->max_move * 2;
                    vch->blood = vch->max_blood * 2;
                }
                else {
                    vch->hit = vch->max_hit;
                    vch->mana = vch->max_mana;
                    vch->move = vch->max_move;
                    vch->blood = vch->max_blood;
                }
                update_pos( vch );
                if ( boost )
                    act( AT_IMMORT, "$n has restored you, and boosted your stats above maximum!",
                         ch, NULL, vch, TO_VICT );
                else
                    act( AT_IMMORT, "$n has restored you.", ch, NULL, vch, TO_VICT );
            }
        }
        send_to_char( "Restored.\r\n", ch );
    }
    else {

        CHAR_DATA              *victim;

        if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }

        if ( get_trust( ch ) < LEVEL_AJ_CPL && victim != ch
             && !( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) ) {
            send_to_char( "You can't do that.\r\n", ch );
            return;
        }

        if ( boost ) {
            victim->hit = victim->max_hit * 2;
            victim->mana = victim->max_mana * 2;
            victim->blood = victim->max_blood * 2;
            victim->move = victim->max_move * 2;
        }
        else {
            victim->hit = victim->max_hit;
            victim->mana = victim->max_mana;
            victim->blood = victim->max_blood;
            victim->move = victim->max_move;
        }
        update_pos( victim );
        if ( boost )
            act( AT_IMMORT, "$n has restored you, and boosted your stats above maximum!", ch, NULL,
                 victim, TO_VICT );
        else
            act( AT_IMMORT, "$n has restored you.", ch, NULL, victim, TO_VICT );
        send_to_char( "Restored.\r\n", ch );
        return;
    }
}

void do_restoretime( CHAR_DATA *ch, char *argument )
{
    long int                time_passed;
    int                     hour,
                            minute;

    set_char_color( AT_IMMORT, ch );

    if ( !last_restore_all_time )
        ch_printf( ch, "There has been no restore all since reboot.\r\n" );
    else {
        time_passed = current_time - last_restore_all_time;
        hour = ( int ) ( time_passed / 3600 );
        minute = ( int ) ( ( time_passed - ( hour * 3600 ) ) / 60 );
        ch_printf( ch, "The  last restore all was %d hours and %d minutes ago.\r\n", hour, minute );
    }

    if ( !ch->pcdata )
        return;

    if ( !ch->pcdata->restore_time ) {
        send_to_char( "You have never done a restore all.\r\n", ch );
        return;
    }

    time_passed = current_time - ch->pcdata->restore_time;
    hour = ( int ) ( time_passed / 3600 );
    minute = ( int ) ( ( time_passed - ( hour * 3600 ) ) / 60 );
    ch_printf( ch, "Your last restore all was %d hours and %d minutes ago.\r\n", hour, minute );
    return;
}

void do_freeze( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_LBLUE, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Freeze whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to freeze you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }
    if ( xIS_SET( victim->act, PLR_FREEZE ) ) {
        xREMOVE_BIT( victim->act, PLR_FREEZE );
        send_to_char( "Your frozen form suddenly thaws.\r\n", victim );
        ch_printf( ch, "%s is now unfrozen.\r\n", victim->name );
    }
    else {
        xSET_BIT( victim->act, PLR_FREEZE );
        send_to_char( "A godly force turns your body to ice!\r\n", victim );
        ch_printf( ch, "You have frozen %s.\r\n", victim->name );
    }
    save_char_obj( victim );
    return;
}

void do_log( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Log whom?\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "all" ) ) {
        if ( ch->level < 107 ) {
            send_to_char
                ( "You cannot log all.  Logging all uses alot of memory.\r\nThus can now only be done by Colonel's and above.\r\n",
                  ch );
            return;
        }

        if ( fLogAll ) {
            fLogAll = FALSE;
            send_to_char( "Log ALL off.\r\n", ch );
        }
        else {
            fLogAll = TRUE;
            send_to_char( "Log ALL on.\r\n", ch );
        }
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to log you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    /*
     * No level check, gods can log anyone.
     */

    if ( xIS_SET( victim->act, PLR_LOG ) ) {
        xREMOVE_BIT( victim->act, PLR_LOG );
        ch_printf( ch, "LOG removed from %s.\r\n", victim->name );
    }
    else {
        xSET_BIT( victim->act, PLR_LOG );
        ch_printf( ch, "LOG applied to %s.\r\n", victim->name );
    }
    return;
}

void do_litterbug( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Set litterbug flag on whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to litterbug you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    set_char_color( AT_IMMORT, victim );
    if ( xIS_SET( victim->act, PLR_LITTERBUG ) ) {
        xREMOVE_BIT( victim->act, PLR_LITTERBUG );
        send_to_char( "You can drop items again.\r\n", victim );
        ch_printf( ch, "LITTERBUG removed from %s.\r\n", victim->name );
    }
    else {
        xSET_BIT( victim->act, PLR_LITTERBUG );
        send_to_char( "A strange force prevents you from dropping any more items!\r\n", victim );
        ch_printf( ch, "LITTERBUG set on %s.\r\n", victim->name );
    }
    return;
}

void do_noemote( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Noemote whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to noemote you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    set_char_color( AT_IMMORT, victim );
    if ( xIS_SET( victim->act, PLR_NO_EMOTE ) ) {
        xREMOVE_BIT( victim->act, PLR_NO_EMOTE );
        send_to_char( "You can emote again.\r\n", victim );
        ch_printf( ch, "NOEMOTE removed from %s.\r\n", victim->name );
    }
    else {
        xSET_BIT( victim->act, PLR_NO_EMOTE );
        send_to_char( "You can't emote!\r\n", victim );
        ch_printf( ch, "NOEMOTE applied to %s.\r\n", victim->name );
    }
    return;
}

void do_notell( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Notell whom?", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to notell you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    set_char_color( AT_IMMORT, victim );
    if ( xIS_SET( victim->act, PLR_NO_TELL ) ) {
        xREMOVE_BIT( victim->act, PLR_NO_TELL );
        send_to_char( "You can use tells again.\r\n", victim );
        ch_printf( ch, "NOTELL removed from %s.\r\n", victim->name );
    }
    else {
        xSET_BIT( victim->act, PLR_NO_TELL );
        send_to_char( "You can't use tells!\r\n", victim );
        ch_printf( ch, "NOTELL applied to %s.\r\n", victim->name );
    }
    return;
}

void do_notitle( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Notitle whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to notitle you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    set_char_color( AT_IMMORT, victim );
    if ( IS_SET( victim->pcdata->flags, PCFLAG_NOTITLE ) ) {
        REMOVE_BIT( victim->pcdata->flags, PCFLAG_NOTITLE );
        send_to_char( "You can set your own title again.\r\n", victim );
        ch_printf( ch, "NOTITLE removed from %s.\r\n", victim->name );
    }
    else {
        SET_BIT( victim->pcdata->flags, PCFLAG_NOTITLE );
        snprintf( buf, MSL, "the %s",
                  title_table[victim->Class][victim->level][victim->sex == SEX_FEMALE ? 1 : 0] );
        set_title( victim, buf );
        send_to_char( "You can't set your own title!\r\n", victim );
        ch_printf( ch, "NOTITLE set on %s.\r\n", victim->name );
    }
    return;
}

void do_silence( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Silence whom?", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to silence you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    set_char_color( AT_IMMORT, victim );
    if ( xIS_SET( victim->act, PLR_SILENCE ) ) {
        send_to_char( "Player already silenced, use unsilence to remove.\r\n", ch );
    }
    else {
        xSET_BIT( victim->act, PLR_SILENCE );
        send_to_char( "You can't use channels!\r\n", victim );
        ch_printf( ch, "You SILENCE %s.\r\n", victim->name );
    }
    return;
}

/* Much better than toggling this with do_silence, yech --Blodkai */
void do_unsilence( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Unsilence whom?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( get_trust( victim ) > get_trust( ch ) ) {
        send_to_char( "You failed.\r\n", ch );
        return;
    }
    set_char_color( AT_IMMORT, victim );
    if ( xIS_SET( victim->act, PLR_SILENCE ) ) {
        xREMOVE_BIT( victim->act, PLR_SILENCE );
        send_to_char( "You can use channels again.\r\n", victim );
        ch_printf( ch, "SILENCE removed from %s.\r\n", victim->name );
    }
    else {
        send_to_char( "That player is not silenced.\r\n", ch );
    }
    return;
}

void do_peace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *rch;

    act( AT_IMMORT, "$n booms, 'PEACE!'", ch, NULL, NULL, TO_ROOM );
    act( AT_IMMORT, "You boom, 'PEACE!'", ch, NULL, NULL, TO_CHAR );
    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
        if ( rch->fighting ) {
            stop_fighting( rch, TRUE );
            do_sit( rch, ( char * ) "" );
        }

        /*
         * Added by Narn, Nov 28/95 
         */
        stop_hating( rch );
        stop_hunting( rch );
        stop_fearing( rch );
    }

    send_to_char( "&YOk.\r\n", ch );
    return;
}

WATCH_DATA             *first_watch;
WATCH_DATA             *last_watch;

void free_watchlist( void )
{
    WATCH_DATA             *pw,
                           *pw_next;

    for ( pw = first_watch; pw; pw = pw_next ) {
        pw_next = pw->next;
        UNLINK( pw, first_watch, last_watch, next, prev );
        DISPOSE( pw->imm_name );
        DISPOSE( pw->player_site );
        DISPOSE( pw->target_name );
        DISPOSE( pw );
    }
    return;
}

void save_watchlist( void )
{
    WATCH_DATA             *pwatch;
    FILE                   *fp;

    if ( !( fp = FileOpen( SYSTEM_DIR WATCH_LIST, "w" ) ) ) {
        bug( "Save_watchlist: Cannot open %s", WATCH_LIST );
        perror( WATCH_LIST );
        return;
    }

    for ( pwatch = first_watch; pwatch; pwatch = pwatch->next )
        fprintf( fp, "%d %s~%s~%s~\n", pwatch->imm_level, pwatch->imm_name,
                 pwatch->target_name ? pwatch->target_name : " ",
                 pwatch->player_site ? pwatch->player_site : " " );
    fprintf( fp, "-1\n" );
    FileClose( fp );
    return;
}

void do_wizlock( CHAR_DATA *ch, char *argument )
{
    sysdata.wiz_lock = !sysdata.wiz_lock;

    set_char_color( AT_DANGER, ch );

    if ( sysdata.wiz_lock )
        send_to_char( "Game wizlocked.\r\n", ch );
    else
        send_to_char( "Game un-wizlocked.\r\n", ch );
    return;
}

void do_noresolve( CHAR_DATA *ch, char *argument )
{
    sysdata.NO_NAME_RESOLVING = !sysdata.NO_NAME_RESOLVING;

    if ( sysdata.NO_NAME_RESOLVING )
        send_to_char( "&YName resolving disabled.\r\n", ch );
    else
        send_to_char( "&YName resolving enabled.\r\n", ch );
    return;
}

/* Output of command reformmated by Samson 2-8-98, and again on 4-7-98 */
void do_users( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA        *d;
    int                     count,
                            puppets = 0;
    char                    arg[MIL];
    const char             *st;
    CHAR_DATA              *person;

    set_pager_color( AT_PLAIN, ch );
    one_argument( argument, arg );
    count = 0;
    send_to_pager( "Desc|     Constate      |Idle|    Player    | HostIP                   \r\n",
                   ch );
    send_to_pager( "----+-------------------+----+--------------+--------------------------\r\n",
                   ch );

    for ( person = first_char; person; person = person->next ) {
        if ( IS_NPC( person ) || IS_IMMORTAL( person ) )
            continue;

        if ( IS_PUPPET( person ) ) {
            st = "Playing";

            char                    ip[MSL];

            snprintf( ip, MSL, "%d.%d.%d.%d", number_range( 2, 254 ), number_range( 2, 254 ),
                      number_range( 2, 254 ), number_range( 2, 254 ) );
            puppets++;
            pager_printf( ch, " %3d| %-17s |%4d| %-12s | %s&D\r\n", number_range( 1, 15 ), st,
                          number_range( 0, 100 ), person->name, ip );
        }

    }

    for ( d = first_descriptor; d; d = d->next ) {
        switch ( d->connected ) {
            case CON_PLAYING:
                st = "Playing";
                break;
            case CON_GET_NAME:
                st = "Get name";
                break;
            case CON_ACCOUNT_MENU:
                st = "Account Menu";
                break;
            case CON_GET_NEW_RACE:
                st = "Get New Race";
                break;
            case CON_GET_NEW_CLASS:
                st = "Get New Class";
                break;
            case CON_GET_FIRST_CHOICE:
                st = "Get First Choice";
                break;
            case CON_GET_SECOND_CHOICE:
                st = "Get Second Choice";
                break;
            case CON_GET_SECOND_CLASS:
                st = "Get Second Class";
                break;
            case CON_GET_THIRD_CLASS:
                st = "Get Third Class";
                break;
            case CON_GET_ANSI:
                st = "ANSI";
                break;
            case CON_NOTE_TO:
                st = "Note To";
                break;
            case CON_NOTE_SUBJECT:
                st = "Note Subject";
                break;
            case CON_NOTE_EXPIRE:
                st = "Note Expire";
                break;
            case CON_MENU:
                st = "Menu";
                break;
            case CON_ENTER_MENU:
                st = "Enter Menu";
                break;
            case CON_ENTER_GAME:
                st = "Enter Game";
                break;
            case CON_REDIT:
                st = "REdit";
                break;
            case CON_OEDIT:
                st = "OEdit";
                break;
            case CON_MEDIT:
                st = "MEdit";
                break;
            case CON_GET_OLD_PASSWORD:
                st = "Get password";
                break;
            case CON_CONFIRM_NEW_NAME:
                st = "Confirm name";
                break;
            case CON_GET_NEW_PASSWORD:
                st = "New password";
                break;
            case CON_CONFIRM_NEW_PASSWORD:
                st = "Confirm password";
                break;
            case CON_GET_NEW_SEX:
                st = "Get sex";
                break;
            case CON_EDITING:
                st = "In line editor";
                break;
            case CON_PRESS_ENTER:
                st = "Press enter";
                break;
            case CON_ROLL_STATS:
                st = "Rolling stats";
                break;
            case CON_COPYOVER_RECOVER:
                st = "copyover recover";
                break;
            case CON_DELETE:
                st = "Confirm delete";
                break;
            case CON_PLOADED:
                st = "Ploaded";
                break;
            default:
                st = "Invalid!!!!";
                break;
        }
        if ( arg[0] == '\0' ) {
            if ( get_trust( ch ) >= LEVEL_AJ_CAPTAIN
                 || ( d->character && can_see( ch, d->character )
                      && !is_ignoring( d->character, ch ) ) ) {
                count++;
                pager_printf( ch, " %3d| %-17s |%4d| %-12s | %s\r\n", d->descriptor, st,
                              d->idle / 4,
                              d->original ? d->original->name : d->character ? d->character->
                              name : "(None!)", d->host );
            }
        }
        else {
            if ( ( get_trust( ch ) >= LEVEL_AJ_GENERAL
                   || ( d->character && can_see( ch, d->character ) ) )
                 && ( !str_prefix( arg, d->host )
                      || ( d->character && !str_prefix( arg, d->character->name ) ) ) ) {
                count++;
                pager_printf( ch, " %3d| %2d|%4d| %-12s | %s\r\n", d->descriptor, d->connected,
                              d->idle / 4,
                              d->original ? d->original->name : d->character ? d->character->
                              name : "(None!)", d->host );
            }
        }
    }
    if ( puppets > 0 )
        pager_printf( ch, "%d user%s and %d puppet%s.\r\n", count, count == 1 ? "" : "s", puppets,
                      puppets == 1 ? "" : "s" );
    else
        pager_printf( ch, "%d user%s.\r\n", count, count == 1 ? "" : "s" );
    return;
}

/* Thanks to Grodyn for pointing out bugs in this function. */
void do_force( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    bool                    mobsonly;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "Force whom to do what?\r\n", ch );
        return;
    }

    mobsonly = get_trust( ch ) < sysdata.level_forcepc;

    if ( !str_cmp( arg, "all" ) ) {
        CHAR_DATA              *vch;
        CHAR_DATA              *vch_next;

        if ( mobsonly ) {
            send_to_char( "Force whom to do what?\r\n", ch );
            return;
        }

        for ( vch = first_char; vch; vch = vch_next ) {
            vch_next = vch->next;

            if ( !IS_NPC( vch ) && get_trust( vch ) < get_trust( ch ) ) {
                act( AT_IMMORT, "$n forces you to '$t'.", ch, argument, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else {
        CHAR_DATA              *victim;

        if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }

        if ( victim == ch ) {
            send_to_char( "Aye aye, right away!\r\n", ch );
            return;
        }

        if ( get_trust( victim ) >= get_trust( ch ) ) {
            send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
            send_to_char( "&RYou have been warned....\r\n", ch );
            act( AT_RED, "$n tried to force you against your wishes.", ch, NULL, victim, TO_VICT );
            return;
        }

        if ( get_trust( ch ) < LEVEL_AJ_CPL && IS_NPC( victim ) && !str_prefix( "mp", argument ) ) {
            send_to_char( "You can't force a mob to do that!\r\n", ch );
            return;
        }
        act( AT_IMMORT, "$n forces you to '$t'.", ch, argument, victim, TO_VICT );
        interpret( victim, argument );
    }

    send_to_char( "Ok.\r\n", ch );
    return;
}

void do_invis( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    short                   level;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg && arg[0] != '\0' ) {
        if ( !is_number( arg ) ) {
            send_to_char( "Usage: invis | invis <level>\r\n", ch );
            return;
        }
        level = atoi( arg );
        if ( level < 2 || level > get_trust( ch ) ) {
            send_to_char( "Invalid level.\r\n", ch );
            return;
        }

        if ( !IS_NPC( ch ) ) {
            ch->pcdata->wizinvis = level;
            ch_printf( ch, "Wizinvis level set to %d.\r\n", level );
        }

        if ( IS_NPC( ch ) ) {
            ch->mobinvis = level;
            ch_printf( ch, "Mobinvis level set to %d.\r\n", level );
        }
        return;
    }

    if ( !IS_NPC( ch ) ) {
        if ( ch->pcdata->wizinvis < 2 )
            ch->pcdata->wizinvis = ch->level;
    }
    if ( IS_NPC( ch ) ) {
        if ( ch->mobinvis < 2 )
            ch->mobinvis = ch->level;
    }
    if ( xIS_SET( ch->act, PLR_WIZINVIS ) ) {
        xREMOVE_BIT( ch->act, PLR_WIZINVIS );
        act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly fade back into existence.\r\n", ch );
    }
    else {
        act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly vanish into thin air.\r\n", ch );
        xSET_BIT( ch->act, PLR_WIZINVIS );
    }
    return;
}

void do_shadowform( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) )
        return;

    if ( xIS_SET( ch->act, PLR_SHADOWFORM ) ) {
        xREMOVE_BIT( ch->act, PLR_SHADOWFORM );
        send_to_char( "Your body returns to normal as Shadowform mode is off.\r\n", ch );
    }
    else {
        xSET_BIT( ch->act, PLR_SHADOWFORM );
        send_to_char( "Your body turns translucent as Shadowform mode is on.\r\n", ch );
    }
    return;
}

void do_holylight( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) )
        return;

    if ( xIS_SET( ch->act, PLR_HOLYLIGHT ) ) {
        xREMOVE_BIT( ch->act, PLR_HOLYLIGHT );
        send_to_char( "Your eyes return to normal as Holy light mode is off.\r\n", ch );
    }
    else {
        xSET_BIT( ch->act, PLR_HOLYLIGHT );
        send_to_char( "Your eyes burn brightly as Holy light mode is on.\r\n", ch );
    }
    return;
}

bool vnums_are_free( const char *check, int low_range, int high_range )
{
    AREA_DATA              *cArea;

    /*
     * Default it to TRUE 
     */
    bool                    free_vnums = TRUE;
    bool                    check_rooms = FALSE;
    bool                    check_mobs = FALSE;
    bool                    check_objs = FALSE;
    bool                    check_all = FALSE;

    /*
     * Have to see what we should be checking 
     */
    if ( !str_cmp( check, "rooms" ) )
        check_rooms = TRUE;
    else if ( !str_cmp( check, "mobs" ) )
        check_mobs = TRUE;
    else if ( !str_cmp( check, "objs" ) )
        check_objs = TRUE;
    else
        check_all = TRUE;

    /*
     * If not free set it to FALSE 
     */
    /*
     * Check Installed areas 
     */
    for ( cArea = first_area; cArea; cArea = cArea->next ) {
        if ( IS_SET( cArea->status, AREA_DELETED ) )
            continue;
        if ( ( check_rooms || check_all )
             && ( ( low_range >= cArea->low_r_vnum && low_range <= cArea->hi_r_vnum )
                  || ( high_range >= cArea->low_r_vnum && high_range <= cArea->hi_r_vnum ) ) )
            free_vnums = FALSE;
        if ( ( check_mobs || check_all )
             && ( ( low_range >= cArea->low_m_vnum && low_range <= cArea->hi_m_vnum )
                  || ( high_range >= cArea->low_m_vnum && high_range <= cArea->hi_m_vnum ) ) )
            free_vnums = FALSE;
        if ( ( check_objs || check_all )
             && ( ( low_range >= cArea->low_o_vnum && low_range <= cArea->hi_o_vnum )
                  || ( high_range >= cArea->low_o_vnum && high_range <= cArea->hi_o_vnum ) ) )
            free_vnums = FALSE;
    }
    /*
     * Check Building areas 
     */
    for ( cArea = first_build; cArea; cArea = cArea->next ) {
        if ( IS_SET( cArea->status, AREA_DELETED ) )
            continue;
        if ( ( check_rooms || check_all )
             && ( ( low_range >= cArea->low_r_vnum && low_range <= cArea->hi_r_vnum )
                  || ( high_range >= cArea->low_r_vnum && high_range <= cArea->hi_r_vnum ) ) )
            free_vnums = FALSE;
        if ( ( check_mobs || check_all )
             && ( ( low_range >= cArea->low_m_vnum && low_range <= cArea->hi_m_vnum )
                  || ( high_range >= cArea->low_m_vnum && high_range <= cArea->hi_m_vnum ) ) )
            free_vnums = FALSE;
        if ( ( check_objs || check_all )
             && ( ( low_range >= cArea->low_o_vnum && low_range <= cArea->hi_o_vnum )
                  || ( high_range >= cArea->low_o_vnum && high_range <= cArea->hi_o_vnum ) ) )
            free_vnums = FALSE;
    }
    return free_vnums;
}

void do_rassign( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    int                     r_lo,
                            r_hi;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    r_lo = atoi( arg2 );
    r_hi = atoi( arg3 );
    if ( !VLD_STR( arg1 ) || r_lo <= 0 || r_hi <= 0 ) {
        send_to_char( "Syntax: rassign <who> <low> <high>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They don't seem to be around.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "They wouldn't know what to do with a room range.\r\n", ch );
        return;
    }
    if ( r_lo > r_hi ) {
        send_to_char( "Unacceptable room range.\r\n", ch );
        return;
    }
    if ( r_lo == 0 )
        r_hi = 0;
    if ( !vnums_are_free( "rooms", r_lo, r_hi ) ) {
        send_to_char( "Those vnums are already being used in another area.\r\n", ch );
        return;
    }
    victim->pcdata->r_range_lo = r_lo;
    victim->pcdata->r_range_hi = r_hi;
    assign_area( victim, FALSE );
    if ( !victim->pcdata->area ) {
        ch_printf( ch, "Failed to assign rooms %d - %d.\r\n", r_lo, r_hi );
        ch_printf( ch, "Players room range is at %d - %d.\r\n", victim->pcdata->r_range_lo,
                   victim->pcdata->r_range_hi );
        return;
    }
    send_to_char( "Done.\r\n", ch );
    set_char_color( AT_IMMORT, victim );
    ch_printf( victim, "%s has assigned you the room vnum range %d - %d.\r\n", ch->name, r_lo,
               r_hi );
    if ( r_lo == 0 ) {                                 /* Scryn 8/12/95 */
        REMOVE_BIT( victim->pcdata->area->status, AREA_LOADED );
        SET_BIT( victim->pcdata->area->status, AREA_DELETED );
    }
    else {
        SET_BIT( victim->pcdata->area->status, AREA_LOADED );
        REMOVE_BIT( victim->pcdata->area->status, AREA_DELETED );
    }
    return;
}

void do_oassign( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    int                     o_lo,
                            o_hi;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    o_lo = atoi( arg2 );
    o_hi = atoi( arg3 );

    if ( arg1[0] == '\0' || o_lo < 0 || o_hi < 0 ) {
        send_to_char( "Syntax: oassign <who> <low> <high>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They don't seem to be around.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "They wouldn't know what to do with an object range.\r\n", ch );
        return;
    }
    if ( o_lo > o_hi ) {
        send_to_char( "Unacceptable object range.\r\n", ch );
        return;
    }
    if ( !vnums_are_free( "objs", o_lo, o_hi ) ) {
        send_to_char( "Those vnums are already being used in another area.\r\n", ch );
        return;
    }
    victim->pcdata->o_range_lo = o_lo;
    victim->pcdata->o_range_hi = o_hi;
    assign_area( victim, FALSE );
    send_to_char( "Done.\r\n", ch );
    set_char_color( AT_IMMORT, victim );
    ch_printf( victim, "%s has assigned you the object vnum range %d - %d.\r\n", ch->name, o_lo,
               o_hi );
    return;
}

void do_massign( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    int                     m_lo,
                            m_hi;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    m_lo = atoi( arg2 );
    m_hi = atoi( arg3 );

    if ( arg1[0] == '\0' || m_lo < 0 || m_hi < 0 ) {
        send_to_char( "Syntax: massign <who> <low> <high>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They don't seem to be around.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) || get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "They wouldn't know what to do with a monster range.\r\n", ch );
        return;
    }
    if ( m_lo > m_hi ) {
        send_to_char( "Unacceptable monster range.\r\n", ch );
        return;
    }
    if ( !vnums_are_free( "mobs", m_lo, m_hi ) ) {
        send_to_char( "Those vnums are already being used in another area.\r\n", ch );
        return;
    }
    victim->pcdata->m_range_lo = m_lo;
    victim->pcdata->m_range_hi = m_hi;
    assign_area( victim, FALSE );
    send_to_char( "Done.\r\n", ch );
    set_char_color( AT_IMMORT, victim );
    ch_printf( victim, "%s has assigned you the monster vnum range %d - %d.\r\n", ch->name, m_lo,
               m_hi );
    return;
}

void do_cmdtable( CHAR_DATA *ch, char *argument )
{
    int                     hash,
                            cnt;
    CMDTYPE                *cmd;
    char                    arg[MIL];

    one_argument( argument, arg );

    if ( strcmp( arg, "lag" ) ) {                      /* display normal command table */
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Commands and Number of Uses This Run\r\n", ch );
        set_pager_color( AT_PLAIN, ch );
        for ( cnt = hash = 0; hash < 126; hash++ )
            for ( cmd = command_hash[hash]; cmd; cmd = cmd->next ) {
                if ( ( ++cnt ) % 4 )
                    pager_printf( ch, "%-6.6s %4d\t", cmd->name, cmd->userec.num_uses );
                else
                    pager_printf( ch, "%-6.6s %4d\r\n", cmd->name, cmd->userec.num_uses );
            }
        send_to_char( "\r\n", ch );
    }
    else {                                             /* display commands causing lag */

        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Commands that have caused lag this run\r\n", ch );
        set_pager_color( AT_PLAIN, ch );
        for ( cnt = hash = 0; hash < 126; hash++ )
            for ( cmd = command_hash[hash]; cmd; cmd = cmd->next ) {
                if ( !cmd->lag_count )
                    continue;
                else if ( ( ++cnt ) % 4 )
                    pager_printf( ch, "%-6.6s %4d\t", cmd->name, cmd->lag_count );
                else
                    pager_printf( ch, "%-6.6s %4d\r\n", cmd->name, cmd->lag_count );
            }
        send_to_char( "\r\n", ch );
    }

    return;
}

void do_mortalize( CHAR_DATA *ch, char *argument )
{
    char                    fname[1024];
    char                    name[256];
    struct stat             fst;
    bool                    loaded;
    DESCRIPTOR_DATA        *d;
    int                     old_room_vnum;
    char                    buf[MSL];
    char                    buf2[MSL];
    CHAR_DATA              *victim;
    AREA_DATA              *pArea;
    int                     sn;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, name );
    if ( name[0] == '\0' ) {
        send_to_char( "Usage: mortalize <playername>\r\n", ch );
        return;
    }

    name[0] = UPPER( name[0] );
    snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );
    if ( stat( fname, &fst ) != -1 ) {
        CREATE( d, DESCRIPTOR_DATA, 1 );

        d->next = NULL;
        d->prev = NULL;
        d->connected = CON_GET_NAME;
        d->outsize = 2000;
        CREATE( d->outbuf, char, d->outsize );

        loaded = load_char_obj( d, name, FALSE, FALSE, TRUE );
        add_char( d->character );
        old_room_vnum = d->character->in_room->vnum;
        char_to_room( d->character, ch->in_room );
        if ( get_trust( d->character ) >= get_trust( ch ) ) {
            do_tell( d->character, ( char * ) "Do *NOT* disturb me again!" );
            send_to_char( "I think you'd better leave that player alone!\r\n", ch );
            d->character->desc = NULL;
            do_quit( d->character, ( char * ) "" );
            return;
        }
        d->character->desc = NULL;
        victim = d->character;
        d->character = NULL;
        DISPOSE( d->outbuf );
        DISPOSE( d );
        victim->level = LEVEL_DEMIGOD;
        victim->exp = 0;
        victim->max_hit = 800;
        victim->max_mana = 800;
        victim->max_move = 800;
        victim->pcdata->apply_blood = 0;
        for ( sn = 0; sn < top_sn; sn++ )
            victim->pcdata->learned[sn] = 0;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        advance_level( victim );
        STRFREE( victim->pcdata->rank );
        if ( xIS_SET( victim->act, PLR_WIZINVIS ) )
            victim->pcdata->wizinvis = victim->trust;
        if ( xIS_SET( victim->act, PLR_WIZINVIS ) && ( victim->level <= LEVEL_DEMIGOD ) ) {
            xREMOVE_BIT( victim->act, PLR_WIZINVIS );
            victim->pcdata->wizinvis = victim->trust;
        }
        snprintf( buf, MSL, "%s%s", STAFF_DIR, capitalize( victim->name ) );

        if ( !remove( buf ) )
            send_to_char( "Player's immortal data destroyed.\r\n", ch );
        else if ( errno != ENOENT ) {
            ch_printf( ch, "Unknown error #%d - %s (immortal data).  Report to Thoric\r\n", errno,
                       strerror( errno ) );
            snprintf( buf2, MSL, "%s mortalizing %s", ch->name, buf );
            perror( buf2 );
        }
        snprintf( buf2, MSL, "%s.are", capitalize( argument ) );
        for ( pArea = first_build; pArea; pArea = pArea->next )
            if ( !strcmp( pArea->filename, buf2 ) ) {
                snprintf( buf, MSL, "%s%s", BUILD_DIR, buf2 );
                if ( IS_SET( pArea->status, AREA_LOADED ) ) {
                    fold_area( pArea, buf, FALSE );
                    close_area( pArea );
                }
                snprintf( buf2, MSL, "%s.bak", buf );
                set_char_color( AT_RED, ch );
                remove( buf2 );
                if ( !rename( buf, buf2 ) )
                    send_to_char( "Player's area data destroyed.  Area saved as backup.\r\n", ch );
                else if ( errno != ENOENT ) {
                    ch_printf( ch, "Unknown error #%d - %s (area data).  Report to Thoric.\r\n",
                               errno, strerror( errno ) );
                    snprintf( buf2, MSL, "%s mortalizing %s", ch->name, buf );
                    perror( buf2 );
                }
            }
        while ( victim->first_carrying )
            extract_obj( victim->first_carrying );
        do_quit( victim, ( char * ) "" );
        return;
    }
    send_to_char( "No such player.\r\n", ch );
    return;
}

/* Pfreload: allows you to "reload" a pfile so you can edit it in shell without making a player log out
   The idea was taken from another snippet, but I pretty much had to write this one myself
   This is writen for FotE, but should work in any SMAUG based mud (i think) -->Keberus
 */
void do_pfreload( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Reload who?\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, arg ) ) ) {
        send_to_char( "They are not here.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) > get_trust( ch ) ) {
        send_to_char( "You can't reload someone who's a higher level!\r\n", ch );
        return;
    }

    if ( exists_player( victim->name ) ) {
        DESCRIPTOR_DATA        *d;
        char                    name[MAX_STRING_LENGTH];
        char                    buf[MAX_STRING_LENGTH];
        ROOM_INDEX_DATA        *in_room;
        bool                    Load;

        d = NULL;
        d = victim->desc;

        sprintf( name, "%s", victim->name );
        in_room = victim->in_room;
        /*
         * clear descriptor pointer to get rid of bug message in log
         */
        victim->desc = NULL;
        extract_char( victim, TRUE );
        d->character = NULL;

        Load = load_char_obj( d, name, FALSE, FALSE, TRUE );    /* Volk - true is quiet * 
                                                                 * mode */
        victim = d->character;
        victim->desc = d;
        victim->timer = 0;
        /*
         * Insert in the char_list
         */
        LINK( d->character, first_char, last_char, next, prev );
        char_to_room( victim, in_room );

        if ( Load ) {
            send_to_char( "Your pfile has been reloaded.\r\n", victim );
            send_to_char( "Their pfile has been reloaded.\r\n", ch );
            sprintf( buf, "%s has been reloaded.", victim->name );
            log_string( buf );
        }
        else {
            send_to_char( "Hrmm bug, it didnt work.\r\n", ch );
            return;
        }
    }

    else
        send_to_char( "They have to have a pfile before you can load it.\r\n", ch );

    return;
}

/*
 * Load up a player file
 */
void do_loadup( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *temp;
    char                    fname[1024];
    char                    name[256];
    struct stat             fst;
    bool                    loaded;
    DESCRIPTOR_DATA        *d;
    int                     old_room_vnum = 0;
    char                    buf[MSL];

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, name );
    if ( name[0] == '\0' ) {
        send_to_char( "Usage: loadup <playername>\r\n", ch );
        return;
    }
    for ( temp = first_char; temp; temp = temp->next ) {
        if ( IS_NPC( temp ) )
            continue;
        if ( can_see( ch, temp ) && !str_cmp( name, temp->name ) )
            break;
    }
    if ( temp != NULL ) {
        ch_printf( ch, "%s - Already playing.\r\n", name );
        return;
    }
    name[0] = UPPER( name[0] );
    snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );
    if ( stat( fname, &fst ) == -1 || !check_parse_name( capitalize( name ), FALSE ) ) {
        ch_printf( ch, "%s - No such player.\r\n", name );
        return;
    }

    if ( stat( fname, &fst ) != -1 ) {
        CREATE( d, DESCRIPTOR_DATA, 1 );

        d->next = NULL;
        d->prev = NULL;
        d->connected = CON_PLOADED;
        d->outsize = 2000;
        CREATE( d->outbuf, char, d->outsize );

        /*
         * Bug here - send_to_char_color? 
         */
        loaded = load_char_obj( d, name, FALSE, FALSE, FALSE );

        add_char( d->character );
        if ( d && d->character && d->character->in_room )
            old_room_vnum = d->character->in_room->vnum;
        char_to_room( d->character, ch->in_room );
        if ( get_trust( d->character ) >= get_trust( ch ) ) {
            do_tell( d->character, ( char * ) "Do *NOT* disturb me again!" );
            send_to_char( "I think you'd better leave that player alone!\r\n", ch );
            d->character->desc = NULL;
            do_quit( d->character, ( char * ) "" );
            return;
        }
        if ( IS_PUPPET( d->character ) ) {
            send_to_char( "This player is still a puppet! Removing puppet flag.\r\n", ch );
            REMOVE_BIT( d->character->pcdata->flags, PCFLAG_PUPPET );
            // d->character->desc->puppet = 0;
        }
        d->character->desc = NULL;
        d->character->retran = old_room_vnum;
        d->character = NULL;
        DISPOSE( d->outbuf );
        DISPOSE( d );
        snprintf( buf, MAX_STRING_LENGTH, "%s has entered the game.\r\n", capitalize( name ) );
        act( AT_IMMORT, buf, ch, NULL, NULL, TO_ROOM );
        ch_printf( ch, "Player %s loaded from room %d%s.\r\n", capitalize( name ), old_room_vnum,
                   !old_room_vnum ? "&RUnknown&D" : "" );
        return;
    }

    /*
     * else no player file 
     */
    ch_printf( ch, "%s - No such player.\r\n", name );
}

void do_fixchar( CHAR_DATA *ch, char *argument )
{
    char                    name[MSL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, name );
    if ( name[0] == '\0' ) {
        send_to_char( "Usage: fixchar <playername>\r\n", ch );
        return;
    }

    victim = get_char_world( ch, name );
    if ( !victim ) {
        send_to_char( "They're not here.\r\n", ch );
        return;
    }
    if ( victim->position == 100 ) {
        victim->position = 115;
    }
    if ( victim->carry_weight < 0 ) {
        interpret( victim, ( char * ) "remove all" );
        interpret( victim, ( char * ) "drop all" );
        victim->carry_weight = 0;
    }
    fix_char( victim );

    send_to_char( "Done.\r\n", ch );
}

void do_newbieset( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *obj;
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: newbieset <char>.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( ( victim->level < 1 ) || ( victim->level > 5 ) ) {
        send_to_char( "Level of victim must be between 1 and 5.\r\n", ch );
        return;
    }

    obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_VEST ), 1 );
    obj_to_char( obj, victim );
    obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_SHIELD ), 1 );
    obj_to_char( obj, victim );
    obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_BANNER ), 1 );
    obj_to_char( obj, victim );

    obj = create_object( get_obj_index( OBJ_VNUM_SCHOOL_DAGGER ), 1 );
    obj_to_char( obj, victim );

    /*
     * Added by Brittany, on Nov. 24, 1996. The object is the adventurer's 
     * guide to the realms of despair, part of academy.are. 
     */
    {
        OBJ_INDEX_DATA         *obj_ind = get_obj_index( 10333 );

        if ( obj_ind != NULL ) {
            obj = create_object( obj_ind, 1 );
            obj_to_char( obj, victim );
        }
    }

/* Added the burlap sack to the newbieset.  The sack is part of sgate.are
   called Spectral Gate.  Brittany */

    {

        OBJ_INDEX_DATA         *obj_ind = get_obj_index( 123 );

        if ( obj_ind != NULL ) {
            obj = create_object( obj_ind, 1 );
            obj_to_char( obj, victim );
        }
    }

    act( AT_IMMORT, "$n has equipped you with a newbieset.", ch, NULL, victim, TO_VICT );
    ch_printf( ch, "You have re-equipped %s.\r\n", victim->name );
    return;
}

/*
 * Extract area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "joe.are susan.are"
 * - Gorog
 */
void extract_area_names( char *inp, char *out )
{
    char                    buf[MIL],
                           *pbuf = buf;
    int                     len;

    *out = '\0';
    while ( inp && *inp ) {
        inp = one_argument( inp, buf );
        if ( ( len = strlen( buf ) ) >= 5 && !strcmp( ".are", pbuf + len - 4 ) ) {
            if ( *out )
                mudstrlcat( out, " ", MSL );
            mudstrlcat( out, buf, MSL );
        }
    }
}

/*
 * Remove area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "aset sedit cset"
 * - Gorog
 */
void remove_area_names( char *inp, char *out )
{
    char                    buf[MIL],
                           *pbuf = buf;
    int                     len;

    *out = '\0';
    while ( inp && *inp ) {
        inp = one_argument( inp, buf );
        if ( ( len = strlen( buf ) ) < 5 || strcmp( ".are", pbuf + len - 4 ) ) {
            if ( *out )
                mudstrlcat( out, " ", MSL );
            mudstrlcat( out, buf, MSL );
        }
    }
}

/* Allows members of the Area Council to add Area names to the bestow field.
 * Area names mus end with ".are" so that no commands can be bestowed.
 */
void do_bestowarea( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MSL];
    CHAR_DATA              *victim;
    int                     arg_len;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_char( "Syntax:\r\n"
                      "bestowarea <victim> <filename>.are\r\n"
                      "bestowarea <victim> none             removes bestowed areas\r\n"
                      "bestowarea <victim> list             lists bestowed areas\r\n"
                      "bestowarea <victim>                  lists bestowed areas\r\n", ch );
        return;
    }
    if ( !( victim = get_char_world( ch, arg ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't give special abilities to a mob!\r\n", ch );
        return;
    }
    if ( get_trust( victim ) < LEVEL_IMMORTAL ) {
        send_to_char( "They aren't an immortal.\r\n", ch );
        return;
    }
    if ( !*argument || !str_cmp( argument, "list" ) ) {
        extract_area_names( victim->pcdata->bestowments, buf );
        ch_printf( ch, "Bestowed areas: %s\r\n", buf );
        return;
    }
    if ( !str_cmp( argument, "none" ) ) {
        if ( VLD_STR( victim->pcdata->bestowments ) )
            remove_area_names( victim->pcdata->bestowments, buf );
        if ( VLD_STR( victim->pcdata->bestowments ) )
            STRFREE( victim->pcdata->bestowments );
        if ( VLD_STR( buf ) ) {
            smash_tilde( buf );
            victim->pcdata->bestowments = STRALLOC( buf );
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }
    arg_len = strlen( argument );
    if ( arg_len < 5 || argument[arg_len - 4] != '.' || argument[arg_len - 3] != 'a'
         || argument[arg_len - 2] != 'r' || argument[arg_len - 1] != 'e' ) {
        send_to_char( "You can only bestow an area name\r\n", ch );
        send_to_char( "E.G. bestow joe sam.are\r\n", ch );
        return;
    }
    if ( VLD_STR( victim->pcdata->bestowments ) ) {
        snprintf( buf, MSL, "%s %s", victim->pcdata->bestowments, argument );
        STRFREE( victim->pcdata->bestowments );
    }
    else
        snprintf( buf, MSL, "%s", argument );
    if ( VLD_STR( buf ) ) {
        smash_tilde( buf );
        victim->pcdata->bestowments = STRALLOC( buf );
    }
    set_char_color( AT_IMMORT, victim );
    ch_printf( victim, "%s has bestowed on you the area: %s\r\n", ch->name, argument );
    send_to_char( "Done.\r\n", ch );
    return;
}

void do_bestow( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            buf[MSL],
                            arg_buf[MSL];
    CHAR_DATA              *victim;
    CMDTYPE                *cmd;
    bool                    fComm = FALSE;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_char( "Bestow whom with what?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't give special abilities to a mob!\r\n", ch );
        return;
    }
    if ( get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to bestow you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }
    if ( !VLD_STR( argument ) || !str_cmp( argument, "show list" ) ) {
        ch_printf( ch, "Current bestowed commands on %s: %s.\r\n", victim->name,
                   victim->pcdata->bestowments );
        return;
    }
    if ( !str_cmp( argument, "none" ) ) {
        if ( VLD_STR( victim->pcdata->bestowments ) )
            STRFREE( victim->pcdata->bestowments );
        ch_printf( ch, "Bestowments removed from %s.\r\n", victim->name );
        ch_printf( victim, "%s has removed your bestowed commands.\r\n", ch->name );
        return;
    }
    arg_buf[0] = '\0';
    argument = one_argument( argument, arg );
    while ( VLD_STR( arg ) ) {
        char                   *cmd_buf,
                                cmd_tmp[MIL];
        bool                    cFound = FALSE;

        if ( !( cmd = find_command( arg ) ) ) {
            ch_printf( ch, "No such command as %s!\r\n", arg );
            argument = one_argument( argument, arg );
            continue;
        }
        else if ( cmd->level > get_trust( ch ) ) {
            ch_printf( ch, "You can't bestow the %s command!\r\n", arg );
            argument = one_argument( argument, arg );
            continue;
        }
        cmd_buf = victim->pcdata->bestowments;
        cmd_buf = one_argument( cmd_buf, cmd_tmp );
        while ( VLD_STR( cmd_tmp ) ) {
            if ( !str_cmp( cmd_tmp, arg ) ) {
                cFound = TRUE;
                break;
            }
            cmd_buf = one_argument( cmd_buf, cmd_tmp );
        }
        if ( cFound == TRUE ) {
            argument = one_argument( argument, arg );
            continue;
        }
        snprintf( arg, MIL, "%s ", arg );
        mudstrlcat( arg_buf, arg, MSL );
        argument = one_argument( argument, arg );
        fComm = TRUE;
    }
    if ( !fComm ) {
        send_to_char
            ( "Good job, knucklehead... you just bestowed them with that master command called 'NOTHING!'\r\n",
              ch );
        return;
    }
    if ( arg_buf[strlen( arg_buf ) - 1] == ' ' )
        arg_buf[strlen( arg_buf ) - 1] = '\0';
    if ( VLD_STR( victim->pcdata->bestowments ) ) {
        snprintf( buf, MSL, "%s %s", victim->pcdata->bestowments, arg_buf );
        STRFREE( victim->pcdata->bestowments );
    }
    else
        snprintf( buf, MSL, "%s", arg_buf );
    if ( VLD_STR( buf ) ) {
        smash_tilde( buf );
        victim->pcdata->bestowments = STRALLOC( buf );
    }
    set_char_color( AT_IMMORT, victim );
    ch_printf( victim, "%s has bestowed on you the command(s): %s\r\n", ch->name, arg_buf );
    send_to_char( "Done.\r\n", ch );
    return;
}

struct tm              *update_time( struct tm *old_time )
{
    time_t                  time;

    time = mktime( old_time );
    return localtime( &time );
}

void do_set_boot_time( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg1[MIL];
    bool                    check;

    check = FALSE;
    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: setboot time {hour minute <day> <month> <year>}\r\n", ch );
        send_to_char( "        setboot manual {0/1}\r\n", ch );
        send_to_char( "        setboot default\r\n", ch );
        ch_printf( ch, "Boot time is currently set to %s, manual bit is set to %d\r\n", reboot_time,
                   set_boot_time->manual );
        return;
    }

    if ( !str_cmp( arg, "time" ) ) {
        struct tm              *now_time;

        argument = one_argument( argument, arg );
        argument = one_argument( argument, arg1 );
        if ( !*arg || !*arg1 || !is_number( arg ) || !is_number( arg1 ) ) {
            send_to_char( "You must input a value for hour and minute.\r\n", ch );
            return;
        }

        now_time = localtime( &current_time );
        if ( ( now_time->tm_hour = atoi( arg ) ) < 0 || now_time->tm_hour > 23 ) {
            send_to_char( "Valid range for hour is 0 to 23.\r\n", ch );
            return;
        }
        if ( ( now_time->tm_min = atoi( arg1 ) ) < 0 || now_time->tm_min > 59 ) {
            send_to_char( "Valid range for minute is 0 to 59.\r\n", ch );
            return;
        }

        argument = one_argument( argument, arg );
        if ( *arg != '\0' && is_number( arg ) ) {
            if ( ( now_time->tm_mday = atoi( arg ) ) < 1 || now_time->tm_mday > 31 ) {
                send_to_char( "Valid range for day is 1 to 31.\r\n", ch );
                return;
            }
            argument = one_argument( argument, arg );
            if ( *arg != '\0' && is_number( arg ) ) {
                if ( ( now_time->tm_mon = atoi( arg ) ) < 1 || now_time->tm_mon > 12 ) {
                    send_to_char( "Valid range for month is 1 to 12.\r\n", ch );
                    return;
                }
                now_time->tm_mon--;
                argument = one_argument( argument, arg );
                if ( ( now_time->tm_year = atoi( arg ) - 1900 ) < 0 || now_time->tm_year > 199 ) {
                    send_to_char( "Valid range for year is 1900 to 2099.\r\n", ch );
                    return;
                }
            }
        }

        now_time->tm_sec = 0;
        if ( mktime( now_time ) < current_time ) {
            send_to_char( "You can't set a time previous to today!\r\n", ch );
            return;
        }
        if ( set_boot_time->manual == 0 )
            set_boot_time->manual = 1;
        new_boot_time = update_time( now_time );
        new_boot_struct = *new_boot_time;
        new_boot_time = &new_boot_struct;
        reboot_check( mktime( new_boot_time ) );
        get_reboot_string(  );

        ch_printf( ch, "Boot time set to %s\r\n", reboot_time );
        check = TRUE;
    }
    else if ( !str_cmp( arg, "manual" ) ) {
        argument = one_argument( argument, arg1 );
        if ( arg1[0] == '\0' ) {
            send_to_char( "Please enter a value for manual boot on/off\r\n", ch );
            return;
        }
        if ( !is_number( arg1 ) ) {
            send_to_char( "Value for manual must be 0 (off) or 1 (on)\r\n", ch );
            return;
        }
        if ( atoi( arg1 ) < 0 || atoi( arg1 ) > 1 ) {
            send_to_char( "Value for manual must be 0 (off) or 1 (on)\r\n", ch );
            return;
        }

        set_boot_time->manual = atoi( arg1 );
        ch_printf( ch, "Manual bit set to %s\r\n", arg1 );
        check = TRUE;
        get_reboot_string(  );
        return;
    }

    else if ( !str_cmp( arg, "default" ) ) {
        set_boot_time->manual = 0;
        /*
         * Reinitialize new_boot_time 
         */
        new_boot_time = localtime( &current_time );
        new_boot_time->tm_mday += 1;
        if ( new_boot_time->tm_hour > 12 )
            new_boot_time->tm_mday += 1;
        new_boot_time->tm_hour = 6;
        new_boot_time->tm_min = 0;
        new_boot_time->tm_sec = 0;
        new_boot_time = update_time( new_boot_time );

        sysdata.DENY_NEW_PLAYERS = FALSE;

        send_to_char( "Reboot time set back to normal.\r\n", ch );
        check = TRUE;
    }

    if ( !check ) {
        send_to_char( "Invalid argument for setboot.\r\n", ch );
        return;
    }
    else {
        get_reboot_string(  );
        new_boot_time_t = mktime( new_boot_time );
    }
}

/*
 * Online high level immortal command for displaying what the encryption
 * of a name/password would be, taking in 2 arguments - the name and the
 * password - can still only change the password if you have access to 
 * pfiles and the correct password
 *
 * Rewritten to provide better information on the use of the command. -Orion
 */
void do_form_password( CHAR_DATA *ch, char *argument )
{
    char                    pass[MSL],
                            name[MSL];

    argument = one_argument( argument, pass );
    argument = one_argument( argument, name );

    if ( !pass || pass[0] == '\0' ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "A password value is required for proper use.\r\n\r\n", ch );
        do_help( ch, ( char * ) "formpass" );
        return;
    }
    if ( !name || name[0] == '\0' ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "A character name value is required for proper use.\r\n\r\n", ch );
        do_help( ch, ( char * ) "formpass" );
        return;
    }

    set_char_color( AT_IMMORT, ch );
    ch_printf( ch, "Those two arguments encrypted result in:  %s\r\n", crypt( pass, name ) );
    return;
}

/*
 * Purge a player file.  No more player.  -- Altrag
 */
void do_destro( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_RED, ch );
    send_to_char( "If you want to destroy a character, spell it out!\r\n", ch );
    return;
}

/* This could have other applications too.. move if needed. -- Altrag */
void close_area( AREA_DATA *pArea )
{
    CHAR_DATA              *ech,
                           *ech_next;
    OBJ_DATA               *eobj,
                           *eobj_next;
    ROOM_INDEX_DATA        *rid,
                           *rid_next;
    OBJ_INDEX_DATA         *oid,
                           *oid_next;
    MOB_INDEX_DATA         *mid,
                           *mid_next;
    int                     icnt;

    for ( ech = first_char; ech; ech = ech_next ) {
        ech_next = ech->next;

        if ( ech->fighting )
            stop_fighting( ech, TRUE );
        if ( IS_NPC( ech ) ) {
            /*
             * if mob is in area, or part of area. 
             */
            if ( URANGE( pArea->low_m_vnum, ech->pIndexData->vnum, pArea->hi_m_vnum ) ==
                 ech->pIndexData->vnum || ( ech->in_room && ech->in_room->area == pArea ) )
                extract_char( ech, TRUE );
            continue;
        }
        if ( ech->in_room && ech->in_room->area == pArea )
            do_recall( ech, ( char * ) "" );
    }
    for ( eobj = first_object; eobj; eobj = eobj_next ) {
        eobj_next = eobj->next;
        /*
         * if obj is in area, or part of area. 
         */
        if ( URANGE( pArea->low_o_vnum, eobj->pIndexData->vnum, pArea->hi_o_vnum ) ==
             eobj->pIndexData->vnum || ( eobj->in_room && eobj->in_room->area == pArea ) )
            extract_obj( eobj );
    }
    for ( icnt = 0; icnt < MAX_KEY_HASH; icnt++ ) {
        for ( rid = room_index_hash[icnt]; rid; rid = rid_next ) {
            rid_next = rid->next;

            if ( rid->area != pArea )
                continue;
            delete_room( rid );
        }
        for ( mid = mob_index_hash[icnt]; mid; mid = mid_next ) {
            mid_next = mid->next;

            if ( mid->vnum < pArea->low_m_vnum || mid->vnum > pArea->hi_m_vnum )
                continue;
            delete_mob( mid );
        }
        for ( oid = obj_index_hash[icnt]; oid; oid = oid_next ) {
            oid_next = oid->next;

            if ( oid->vnum < pArea->low_o_vnum || oid->vnum > pArea->hi_o_vnum )
                continue;
            delete_obj( oid );
        }
    }

    if ( IS_SET( pArea->flags, AFLAG_PROTOTYPE ) ) {
        UNLINK( pArea, first_build, last_build, next, prev );
        UNLINK( pArea, first_bsort, last_bsort, next_sort, prev_sort );
    }
    else {
        UNLINK( pArea, first_area, last_area, next, prev );
        UNLINK( pArea, first_asort, last_asort, next_sort, prev_sort );
        UNLINK( pArea, first_area_name, last_area_name, next_sort_name, prev_sort_name );
    }

    UNLINK( pArea, first_full_area, last_full_area, next_area, prev_area );

    STRFREE( pArea->name );
    STRFREE( pArea->filename );
    if ( VLD_STR( pArea->resetmsg ) )
        STRFREE( pArea->resetmsg );
    if ( VLD_STR( pArea->clanname ) )
        STRFREE( pArea->clanname );
    if ( VLD_STR( pArea->author ) )
        STRFREE( pArea->author );
    if ( VLD_STR( pArea->derivatives ) )
        STRFREE( pArea->derivatives );
    if ( VLD_STR( pArea->htown ) )
        STRFREE( pArea->htown );
    if ( VLD_STR( pArea->desc ) )
        STRFREE( pArea->desc );
    if ( pArea->influencer )
        pArea->influencer = NULL;

    DISPOSE( pArea );
}

void do_destroy( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    buf[MAX_STRING_LENGTH];
    char                    buf2[MAX_STRING_LENGTH];
    char                    buf3[MAX_STRING_LENGTH];
    char                    buf4[MAX_STRING_LENGTH];
    char                    arg[MAX_INPUT_LENGTH];
    char                   *name;
    struct stat             fst;

   /******************************************************************/

    set_char_color( AT_RED, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Destroy what player file?\r\n", ch );
        return;
    }

    /*
     * Set the file points.
     */
    name = capitalize( arg );
    snprintf( buf, MAX_STRING_LENGTH, "%s%c/%s", PLAYER_DIR, tolower( arg[0] ), name );
    snprintf( buf3, MAX_STRING_LENGTH, "%s%c/%s.lua", PLAYER_DIR, tolower( arg[0] ), name );
   /******************************************************************/

    /*
     * This check makes sure the name is valid and that the file is there, else there
     * is no need to go on. -Orion
     */
    if ( !check_parse_name( name, TRUE ) || lstat( buf, &fst ) == -1 ) {
        ch_printf( ch, "No player exists by the name %s.\r\n", name );
        return;
    }

    for ( victim = first_char; victim; victim = victim->next )
        if ( !IS_NPC( victim ) && !str_cmp( victim->name, arg ) )
            break;

    if ( !victim ) {
        DESCRIPTOR_DATA        *d;

        /*
         * Make sure they aren't halfway logged in. 
         */
        for ( d = first_descriptor; d; d = d->next )
            if ( ( victim = d->character ) && !IS_NPC( victim ) && !str_cmp( victim->name, arg ) )
                break;
        if ( d )
            close_socket( d, TRUE );
    }
    else {
        int                     x,
                                y;

        quitting_char = victim;
        save_char_obj( victim );
        saving_char = NULL;
        extract_char( victim, TRUE );
        for ( x = 0; x < MAX_WEAR; x++ )
            for ( y = 0; y < MAX_LAYERS; y++ )
                save_equipment[x][y] = NULL;
    }

    if ( !remove( buf ) ) {
        AREA_DATA              *pArea;

        remove_from_rosters( name );
        remove_from_rollcalls( name );

        snprintf( buf, sizeof( buf ), "%s%s", VAULT_DIR, name );
        if ( !remove( buf ) )
            send_to_char( "Player's vault data destroyed.\r\n", ch );
        else if ( errno != ENOENT ) {
            ch_printf( ch, "Unknown error #%d - %s (vault data).\r\n", errno, strerror( errno ) );
            snprintf( buf2, sizeof( buf2 ), "%s destroying %s", ch->name, buf );
            perror( buf2 );
        }
        set_char_color( AT_RED, ch );
        ch_printf( ch, "Player %s destroyed.\r\n", name );
        snprintf( buf, MAX_STRING_LENGTH, "%s%s", STAFF_DIR, name );
        if ( !remove( buf ) )
            send_to_char( "Player's immortal data destroyed.\r\n", ch );
        else if ( errno != ENOENT ) {
            ch_printf( ch, "Unknown error #%d - %s (immortal data). Report to www.6dragons.org\r\n",
                       errno, strerror( errno ) );
            snprintf( buf2, MAX_STRING_LENGTH, "%s destroying %s", ch->name, buf );
            perror( buf2 );
        }

        snprintf( buf2, MAX_STRING_LENGTH, "%s.are", name );
        for ( pArea = first_build; pArea; pArea = pArea->next ) {
            if ( pArea->filename && !str_cmp( pArea->filename, buf2 ) ) {
                snprintf( buf, MAX_STRING_LENGTH, "%s%s", BUILD_DIR, buf2 );
                if ( IS_SET( pArea->status, AREA_LOADED ) )
                    fold_area( pArea, buf, FALSE );
                close_area( pArea );
                snprintf( buf2, MAX_STRING_LENGTH, "%s.bak", buf );
                set_char_color( AT_RED, ch );          /* Log message changes colors */
                remove( buf2 );
                if ( !rename( buf, buf2 ) )
                    send_to_char( "Player's area data destroyed.  Area saved as backup.\r\n", ch );
                else if ( errno != ENOENT ) {
                    ch_printf( ch, "Unknown error #%d - %s (area data). Report to someone.\r\n",
                               errno, strerror( errno ) );
                    snprintf( buf2, MAX_STRING_LENGTH, "%s destroying %s", ch->name, buf );
                    perror( buf2 );
                }
                break;
            }
        }
    }
    else if ( errno == ENOENT ) {
        set_char_color( AT_PLAIN, ch );
        send_to_char( "Player does not exist.\r\n", ch );
    }
    else {
        set_char_color( AT_WHITE, ch );
        ch_printf( ch, "Unknown error #%d - %s. Report to www.6dragons.org\r\n", errno,
                   strerror( errno ) );
        snprintf( buf, MAX_STRING_LENGTH, "%s destroying %s", ch->name, arg );
        perror( buf );
    }

    snprintf( buf, MSL,
              "The Demon Lord Balzhur, has been sent to feed!!  Balzhur screams, 'You are MINE %s' and gobbles them up completely!!!",
              victim->name );
    echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );

    if ( !remove( buf3 ) ) {
        set_char_color( AT_RED, ch );
        ch_printf( ch, "LUA: Player %s Lua file destroyed.\r\n", name );
    }
}

extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH]; /* db.c */

/* Super-AT command:
FOR ALL <action>
FOR MORTALS <action>
FOR GODS <action>
FOR MOBS <action>
FOR EVERYWHERE <action>

Executes action several times, either on ALL players (not including yourself),
MORTALS (including trusted characters), GODS (characters with level higher than
L_HERO), MOBS (Not recommended) or every room (not recommended either!)

If you insert a # in the action, it will be replaced by the name of the target.

If # is a part of the action, the action will be executed for every target
in game. If there is no #, the action will be executed for every room containg
at least one target, but only once per room. # cannot be used with FOR EVERY-
WHERE. # can be anywhere in the action.

Example: 

FOR ALL SMILE -> you will only smile once in a room with 2 players.
FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.

Destroying the characters this command acts upon MAY cause it to fail. Try to
avoid something like FOR MOBS PURGE (although it actually works at my MUD).

FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work
though :)

The command works by transporting the character to each of the rooms with 
target in them. Private rooms are not violated.

*/

/* Expand the name of a character into a string that identifies THAT
   character within a room. E.g. the second 'guard' -> 2. guard
*/
const char             *name_expand( CHAR_DATA *ch )
{
    int                     count = 1;
    CHAR_DATA              *rch;
    char                    name[MIL];                 /* HOPEFULLY no mob has a name
                                                        * longer than THAT */
    static char             outbuf[MIL];

    if ( !IS_NPC( ch ) )
        return ch->name;

    one_argument( ch->name, name );                    /* copy the first word into name */

    if ( !name[0] ) {                                  /* weird mob .. no keywords */
        mudstrlcpy( outbuf, "", MIL );                 /* Do not return NULL, just an *
                                                        * empty buffer */
        return outbuf;
    }

    /*
     * ->people changed to ->first_person -- TRI 
     */
    for ( rch = ch->in_room->first_person; rch && ( rch != ch ); rch = rch->next_in_room )
        if ( is_name( name, rch->name ) )
            count++;

    snprintf( outbuf, MIL, "%d.%s", count, name );
    return outbuf;
}

void do_for( CHAR_DATA *ch, char *argument )
{
    char                    range[MIL];
    char                    buf[MSL];
    bool                    fGods = FALSE,
        fMortals = FALSE,
        fMobs = FALSE,
        fEverywhere = FALSE,
        found;
    ROOM_INDEX_DATA        *room,
                           *old_room;
    CHAR_DATA              *p,
                           *p_prev;                    /* p_next to p_prev -- TRI */
    int                     i;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, range );
    if ( !range[0] || !argument[0] ) {                 /* invalid usage? */
        do_help( ch, ( char * ) "for" );
        return;
    }

    if ( !str_prefix( "quit", argument ) ) {
        send_to_char( "Are you trying to crash the MUD or something?\r\n", ch );
        return;
    }

    if ( !str_cmp( range, "all" ) ) {
        fMortals = TRUE;
        fGods = TRUE;
    }
    else if ( !str_cmp( range, "gods" ) )
        fGods = TRUE;
    else if ( !str_cmp( range, "mortals" ) )
        fMortals = TRUE;
    else if ( !str_cmp( range, "mobs" ) )
        fMobs = TRUE;
    else if ( !str_cmp( range, "everywhere" ) )
        fEverywhere = TRUE;
    else
        do_help( ch, ( char * ) "for" );               /* show syntax */

    /*
     * do not allow # to make it easier 
     */
    if ( fEverywhere && strchr( argument, '#' ) ) {
        send_to_char( "Cannot use FOR EVERYWHERE with the # thingie.\r\n", ch );
        return;
    }

    set_char_color( AT_PLAIN, ch );
    if ( strchr( argument, '#' ) ) {                   /* replace # ? */
        /*
         * char_list - last_char, p_next - gch_prev -- TRI 
         */
        for ( p = last_char; p; p = p_prev ) {
            p_prev = p->prev;                          /* TRI */
            /*
             * p_next = p->next; 
             *//*
             * In case someone DOES try to AT MOBS SLAY # 
             */
            found = FALSE;

            if ( !( p->in_room ) || room_is_private( p->in_room ) || ( p == ch ) )
                continue;

            if ( IS_NPC( p ) && fMobs )
                found = TRUE;
            else if ( !IS_NPC( p ) && p->level >= LEVEL_IMMORTAL && fGods )
                found = TRUE;
            else if ( !IS_NPC( p ) && p->level < LEVEL_IMMORTAL && fMortals )
                found = TRUE;

            /*
             * It looks ugly to me.. but it works :) 
             */
            if ( found ) {                             /* p is 'appropriate' */
                char                   *pSource = argument; /* head of buffer to be
                                                             * parsed */
                char                   *pDest = buf;   /* parse into this */

                while ( *pSource ) {
                    if ( *pSource == '#' ) {           /* Replace # with name of target */
                        const char             *namebuf = name_expand( p );

                        if ( namebuf )                 /* in case there is no mob name ?? 
                                                        */
                            while ( *namebuf )         /* copy name over */
                                *( pDest++ ) = *( namebuf++ );

                        pSource++;
                    }
                    else
                        *( pDest++ ) = *( pSource++ );
                }                                      /* while */
                *pDest = '\0';                         /* Terminate */

                /*
                 * Execute 
                 */
                old_room = ch->in_room;
                char_from_room( ch );
                char_to_room( ch, p->in_room );
                interpret( ch, buf );
                char_from_room( ch );
                char_to_room( ch, old_room );

            }                                          /* if found */
        }                                              /* for every char */
    }
    else {                                             /* just for every room with the *
                                                        * * * * appropriate people in it */

        for ( i = 0; i < MAX_KEY_HASH; i++ )           /* run through all the buckets */
            for ( room = room_index_hash[i]; room; room = room->next ) {
                found = FALSE;

                /*
                 * Anyone in here at all? 
                 */
                if ( fEverywhere )                     /* Everywhere executes always */
                    found = TRUE;
                else if ( !room->first_person )        /* Skip it if room is empty */
                    continue;
                /*
                 * ->people changed to first_person -- TRI 
                 */

                /*
                 * Check if there is anyone here of the requried type 
                 */
                /*
                 * Stop as soon as a match is found or there are no more ppl in room 
                 */
                /*
                 * ->people to ->first_person -- TRI 
                 */
                for ( p = room->first_person; p && !found; p = p->next_in_room ) {

                    if ( p == ch )                     /* do not execute on oneself */
                        continue;

                    if ( IS_NPC( p ) && fMobs )
                        found = TRUE;
                    else if ( !IS_NPC( p ) && ( p->level >= LEVEL_IMMORTAL ) && fGods )
                        found = TRUE;
                    else if ( !IS_NPC( p ) && ( p->level <= LEVEL_IMMORTAL ) && fMortals )
                        found = TRUE;
                }                                      /* for everyone inside the room */

                if ( found && !room_is_private( room ) ) {  /* Any of the required type
                                                             * here * AND room * not *
                                                             * private? */
                    /*
                     * This may be ineffective. Consider moving character out of old_room
                     * once at beginning of command then moving back at the end.
                     * This however, is more safe?
                     */

                    old_room = ch->in_room;
                    char_from_room( ch );
                    char_to_room( ch, room );
                    interpret( ch, argument );
                    char_from_room( ch );
                    char_to_room( ch, old_room );
                }                                      /* if found */
            }                                          /* for every room in a bucket */
    }                                                  /* if strchr */
}                                                      /* do_for */

void save_sysdata       args( ( SYSTEM_DATA sys ) );

void update_calendar( void )
{
    sysdata.daysperyear = sysdata.dayspermonth * sysdata.monthsperyear;
    sysdata.hoursunrise = sysdata.hoursperday / 4;
    sysdata.hourdaybegin = sysdata.hoursunrise + 1;
    sysdata.hournoon = sysdata.hoursperday / 2;
    sysdata.hoursunset = ( ( sysdata.hoursperday / 4 ) * 3 );
    sysdata.hournightbegin = sysdata.hoursunset + 1;
    sysdata.hourmidnight = sysdata.hoursperday;
    calc_season(  );
    return;
}

void update_timers( void )
{
    sysdata.pulsetick = sysdata.secpertick * sysdata.pulsepersec;
    sysdata.pulseviolence = 3 * sysdata.pulsepersec;
    sysdata.pulsemobile = 4 * sysdata.pulsepersec;
    sysdata.pulsecalendar = 4 * sysdata.pulsetick;
    sysdata.pulseweather = 2 * sysdata.pulsetick;
    sysdata.pulseaffect = sysdata.pulsetick / 5;
    return;
}

void do_cset( CHAR_DATA *ch, char *argument )
{
    char                    arg[MSL];
    int                     level,
                            value;

    set_pager_color( AT_PLAIN, ch );

    if ( argument[0] == '\0' ) {
        pager_printf( ch, "\r\n&WMud_name: %s", sysdata.mud_name );
        pager_printf( ch,
                      "\r\n&WMail:\r\n  &wRead all mail: &W%d  &wRead mail for free: &W%d  &wWrite mail for free: &W%d\r\n",
                      sysdata.read_all_mail, sysdata.read_mail_free, sysdata.write_mail_free );
        pager_printf( ch, "&WHTTP&w: %s\r\n\r\n", sysdata.http );
        pager_printf( ch, "&WDouble Experience&w (On/OFF) &W%s\r\n",
                      sysdata.daydexp ? "On" : "Off" );
        pager_printf( ch, "\r\n&WAdvanced_Player &w(ON/OFF) &W%s\r\n",
                      sysdata.Advanced_Player ? "On" : "Off" );
        pager_printf( ch, "\r\n&WAUTH: &w(ON/OFF) &W%s\r\n", sysdata.WAIT_FOR_AUTH ? "On" : "Off" );

        pager_printf( ch, "\r\n&wQuotes: &W%d\r\n", sysdata.numquotes );

        pager_printf( ch, "\r\n&wPfiles On/Off status: &W%s  &wDays before  newbie_purge: &W%d\r\n",
                      sysdata.CLEANPFILES ? "On" : "Off", sysdata.newbie_purge );
        pager_printf( ch, "&wDays before  regular_purge: &W%d\r\n", sysdata.regular_purge );
        pager_printf( ch, "  &wTake all mail: &W%d\r\n", sysdata.take_others_mail );
        pager_printf( ch,
                      "&WChannels:\r\n  &wMuse: &W%d   &wThink: &W%d   &wLog: &W%d   &wBuild: &W%d\r\n",
                      sysdata.muse_level, sysdata.think_level, sysdata.log_level,
                      sysdata.build_level );
        pager_printf( ch, "&WBuilding:\r\n  &wproto_modify: &W%d  &wPlayer msetting: &W%d\r\n",
                      sysdata.level_modify_proto, sysdata.level_mset_player );
        pager_printf( ch, "&WClans:\r\n  &wOverseer: &W%s   &wAdvisor: &W%s\r\n",
                      sysdata.clan_overseer, sysdata.clan_advisor );
        pager_printf( ch, "&WBan Data:\r\n  &wBan Site Level: &W%d   &wBan Class Level: &W%d   ",
                      sysdata.ban_site_level, sysdata.ban_class_level );
        pager_printf( ch, "&wBan Race Level: &W%d\r\n", sysdata.ban_race_level );
        pager_printf( ch,
                      "&WDefenses:\r\n  &wDodge_mod: &W%d  &wParry_mod: &W%d  &wTumble_mod: &W%d &wPhase_mod:&W %d&D\r\n",
                      sysdata.dodge_mod, sysdata.parry_mod, sysdata.tumble_mod, sysdata.phase_mod );
        pager_printf( ch, "&w  Displacement_mod: &W%d&D\r\n", sysdata.displacement_mod );
        pager_printf( ch, "&WOther:\r\n  &wforcepc:             &W%-7d", sysdata.level_forcepc );
        pager_printf( ch, "&woverride_private:         &W%-3d\r\n",
                      sysdata.level_override_private );
        pager_printf( ch, "  &wPenalty to bash plr vs. plr:  &W%-7d", sysdata.bash_plr_vs_plr );
        pager_printf( ch, "&wPenalty to non-tank bash:      &W%-3d\r\n", sysdata.bash_nontank );
        pager_printf( ch, "  &wPenalty to gouge plr vs. plr: &W%-7d", sysdata.gouge_plr_vs_plr );
        pager_printf( ch, "&wPenalty to non-tank gouge:     &W%-3d\r\n", sysdata.gouge_nontank );
        pager_printf( ch, "  &wPenalty regular stun chance:  &W%-7d", sysdata.stun_regular );
        pager_printf( ch, "&wPenalty to stun plr vs. plr:   &W%-3d\r\n", sysdata.stun_plr_vs_plr );
        pager_printf( ch, "  &wPercent damage plr vs. plr:   &W%-7d", sysdata.dam_plr_vs_plr );
        pager_printf( ch, "&wPercent damage plr vs. mob:    &W%-3d \r\n", sysdata.dam_plr_vs_mob );
        pager_printf( ch, "  &wPercent damage mob vs. plr:   &W%-7d", sysdata.dam_mob_vs_plr );
        pager_printf( ch, "&wPercent damage mob vs. mob:    &W%-3d\r\n", sysdata.dam_mob_vs_mob );
        pager_printf( ch, "  &wgetnotake : &W%-7d", sysdata.level_getobjnotake );
        pager_printf( ch, "&wAutosave frequency (minutes):  &W%d\r\n", sysdata.save_frequency );
        pager_printf( ch, "  &wMax level difference bestow:  &W%-7d", sysdata.bestow_dif );
        pager_printf( ch, "&wChecking Imm_host is:          &W%s\r\n",
                      ( sysdata.check_imm_host ) ? "ON" : "off" );
        pager_printf( ch, "  &wMorph Optimization is:        &W%-7s",
                      ( sysdata.morph_opt ) ? "ON" : "off" );
        pager_printf( ch, "&wSaving Pets is:                &W%s\r\n",
                      ( sysdata.save_pets ) ? "ON" : "off" );
        pager_printf( ch, "  &wPkill looting is:             &W%-7s",
                      ( sysdata.pk_loot ) ? "ON" : "off" );
        pager_printf( ch, "&wRequired who argument:         &W%-3s\r\n",
                      ( sysdata.reqwho_arg ) ? "ON" : "off" );
        pager_printf( ch, "  &wSave flags: &W%s\r\n",
                      ext_flag_string( &sysdata.save_flags, save_flag ) );
        pager_printf( ch, "&wGMB Bonus per player Online:    &W%d\r\n", sysdata.gmb );
        pager_printf( ch, "&WCalendar:\r\n" );
        pager_printf( ch,
                      "&wPulse for weather: &W%d  &wSeconds per tick: &W%d   &wPulse per second: &W%d &wPulse Affect: &W%d\r\n",
                      sysdata.pulseweather, sysdata.secpertick, sysdata.pulsepersec,
                      sysdata.pulseaffect );
        pager_printf( ch,
                      "  &wHours per day: &W%d &wDays per week: &W%d &wDays per month: &W%d &wMonths per year: &W%d &wDays per year: &W%d\r\n",
                      sysdata.hoursperday, sysdata.daysperweek, sysdata.dayspermonth,
                      sysdata.monthsperyear, sysdata.daysperyear );
        pager_printf( ch,
                      "  &wPULSE_TICK: &W%d &wPULSE_VIOLENCE: &W%d &wPULSE_MOBILE: &W%d &wPULSE_CALENDAR: &W%d\r\n",
                      sysdata.pulsetick, sysdata.pulseviolence, sysdata.pulsemobile,
                      sysdata.pulsecalendar );
        pager_printf( ch, "  &wIdents retries:               &W%-7d", sysdata.ident_retries );
        pager_printf( ch, "&wWizlock is:                    &W%-7s\r\n",
                      ( sysdata.wiz_lock ) ? "ON" : "off" );
        pager_printf( ch, "  &wAutoboot Time:                &W%2.2d:%-4.2d", sysdata.autoboot_hour,
                      sysdata.autoboot_minute );
        pager_printf( ch, "&wAutoboot Period:               &W%-7d\r\n", sysdata.autoboot_period );
        pager_printf( ch, "  &wLeave 'clan_timer': &W%-2d days\r\n", sysdata.clan_timer );
        pager_printf( ch, "  &wNolog Clan Leadership : &W%-2d days\r\n", sysdata.clanlog );
        pager_printf( ch,
                      "  &wclass1maxadept: &W%-3d &wclass2maxadept: &W%-3d &wclass3maxadept: &W%-3d\r\n",
                      sysdata.class1maxadept, sysdata.class2maxadept, sysdata.class3maxadept );
        pager_printf( ch, "  &wPotions: &W%5d&w oob&W %5d&w pve&W %5d&w pvp\r\n",
                      sysdata.potionsoob, sysdata.potionspve, sysdata.potionspvp );
        return;
    }

    argument = one_argument( argument, arg );
    smash_tilde( argument );

    level = ( short ) atoi( argument );

    if ( !str_cmp( arg, "help" ) ) {
        do_help( ch, ( char * ) "controls" );
        return;
    }

    if ( !str_cmp( arg, "pfiles" ) ) {

        sysdata.CLEANPFILES = !sysdata.CLEANPFILES;

        if ( sysdata.CLEANPFILES )
            send_to_char( "Pfile autocleanup enabled.\r\n", ch );
        else
            send_to_char( "Pfile autocleanup disabled.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "auth" ) ) {

        sysdata.WAIT_FOR_AUTH = !sysdata.WAIT_FOR_AUTH;

        if ( sysdata.WAIT_FOR_AUTH )
            send_to_char( "Name authorization system enabled.\r\n", ch );
        else
            send_to_char( "Name authorization system disabled.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "dexp" ) ) {

        sysdata.daydexp = !sysdata.daydexp;

        if ( sysdata.daydexp )
            echo_to_all( AT_RED, "[&WEvent&R] &CIt is time for Double Experience! Get killing!\r\n",
                         ECHOTAR_ALL );
        else
            echo_to_all( AT_RED, "[&WEvent&R] &CDouble Experience is over.\r\n", ECHOTAR_ALL );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "Advanced_Player" ) ) {

        sysdata.Advanced_Player = !sysdata.Advanced_Player;

        if ( sysdata.Advanced_Player )
            send_to_char( "Advanced Player system enabled.\r\n", ch );
        else
            send_to_char( "Advanced Player system disabled.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "save" ) ) {
        save_sysdata( sysdata );
        send_to_char( "Cset functions saved.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "http" ) ) {
        STRFREE( sysdata.http );
        if ( !argument || argument[0] == '\0' ) {
            sysdata.http = STRALLOC( "Not Set" );
            send_to_char( "HTTP address cleared.\r\n", ch );
        }
        else {
            sysdata.http = STRALLOC( argument );
            ch_printf( ch, "HTTP address set to %s\r\n", argument );
        }
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "mudname" ) ) {
        if ( sysdata.mud_name )
            STRFREE( sysdata.mud_name );
        sysdata.mud_name = STRALLOC( argument );
        send_to_char( "Name set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "saveflag" ) ) {
        int                     x = get_saveflag( argument );

        if ( x < 0 || x > MAX_SAVE )
            send_to_char( "Not a save flag.\r\n", ch );
        else {
            xTOGGLE_BIT( sysdata.save_flags, x );
            send_to_char( "Ok.\r\n", ch );
        }
        return;
    }

    if ( !str_prefix( arg, "clan_overseer" ) ) {
        STRFREE( sysdata.clan_overseer );
        sysdata.clan_overseer = STRALLOC( argument );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_prefix( arg, "clan_advisor" ) ) {
        STRFREE( sysdata.clan_advisor );
        sysdata.clan_advisor = STRALLOC( argument );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    value = ( short ) atoi( argument );

    if ( !str_cmp( arg, "quotes" ) ) {
        sysdata.numquotes = value;
        ch_printf( ch, "Quotes set to %d.\r\n", sysdata.numquotes );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "max-holidays" ) ) {
        sysdata.maxholiday = value;
        ch_printf( ch, "Max Holiday set to %d.\r\n", value );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "hours-per-day" ) ) {
        sysdata.hoursperday = value;
        ch_printf( ch, "Hours per day set to %d.\r\n", value );
        update_calendar(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "days-per-week" ) ) {
        sysdata.daysperweek = value;
        ch_printf( ch, "Days per week set to %d.\r\n", value );
        update_calendar(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "days-per-month" ) ) {
        sysdata.dayspermonth = value;
        ch_printf( ch, "Days per month set to %d.\r\n", value );
        update_calendar(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "months-per-year" ) ) {
        sysdata.monthsperyear = value;
        ch_printf( ch, "Months per year set to %d.\r\n", value );
        update_calendar(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "seconds-per-tick" ) ) {
        sysdata.secpertick = value;
        ch_printf( ch, "Seconds per tick set to %d.\r\n", value );
        update_timers(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "pulse-per-second" ) ) {
        sysdata.pulsepersec = value;
        ch_printf( ch, "Pulse per second set to %d.\r\n", value );
        update_timers(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "pulseweather" ) ) {
        sysdata.pulseweather = value;
        ch_printf( ch, "Pulse Weather set to %d.\r\n", value );
        update_timers(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_cmp( arg, "pulseaffect" ) ) {
        sysdata.pulseaffect = value;
        ch_printf( ch, "Pulse Affect set to %d.\r\n", value );
        update_timers(  );
        save_sysdata( sysdata );
        return;
    }

    if ( !str_prefix( arg, "autoboot_time" ) ) {
        int                     minute,
                                hour;
        char                    arg1[MSL];
        char                    arg2[MSL];

        argument = one_argument( argument, arg1 );
        argument = one_argument( argument, arg2 );

        hour = ( short ) atoi( arg1 );
        minute = ( short ) atoi( arg2 );

        if ( minute > 60 )
            hour += ( minute / 60 );

        if ( ( minute % 60 ) > 0 )
            minute %= 60;

        if ( ( hour % 24 ) > 0 )
            hour %= 24;

        sysdata.autoboot_hour = hour;
        sysdata.autoboot_minute = minute;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    level = ( short ) atoi( argument );

    if ( !str_prefix( arg, "savefrequency" ) ) {
        sysdata.save_frequency = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "class1maxadept" ) ) {
        if ( level < 0 || level > 100 ) {
            send_to_char( "Please set a level between 0 and 100% adept.\r\n", ch );
            return;
        }
        sysdata.class1maxadept = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "class2maxadept" ) ) {
        if ( level < 0 || level > 100 ) {
            send_to_char( "Please set a level between 0 and 100% adept.\r\n", ch );
            return;
        }
        sysdata.class2maxadept = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "class3maxadept" ) ) {
        if ( level < 0 || level > 100 ) {
            send_to_char( "Please set a level between 0 and 100% adept.\r\n", ch );
            return;
        }
        sysdata.class3maxadept = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "potionsoob" ) ) {
        if ( level < 0 || level > 60 ) {
            send_to_char( "Potion lag is between 0 and 60 seconds. \r\n", ch );
            return;
        }
        sysdata.potionsoob = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "potionspvp" ) ) {
        if ( level < 0 || level > 60 ) {
            send_to_char( "Potion lag is between 0 and 60 seconds. \r\n", ch );
            return;
        }
        sysdata.potionspvp = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "potionspve" ) ) {
        if ( level < 0 || level > 60 ) {
            send_to_char( "Potion lag is between 0 and 60 seconds. \r\n", ch );
            return;
        }
        sysdata.potionspve = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "clan_timer" ) ) {
        if ( level < 1 || level > 90 ) {
            send_to_char( "Minimum 1 day, maximum 90 days before joining another clan.\r\n", ch );
            return;
        }
        sysdata.clan_timer = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "clanlog" ) ) {
        if ( level < 1 || level > 14 ) {
            send_to_char( "Minimum 1 day, maximum 14 days before losing leadership title.\r\n",
                          ch );
            return;
        }
        sysdata.clanlog = level;
        send_to_char( "Clan Log in days set for removal of clan leadership.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "newbie_purge" ) ) {
        if ( level < 1 ) {
            send_to_char( "You must specify a period of at least 1 day.\r\n", ch );
            return;
        }

        sysdata.newbie_purge = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "regular_purge" ) ) {
        if ( level < 1 ) {
            send_to_char( "You must specify a period of at least 1 day.\r\n", ch );
            return;
        }

        sysdata.regular_purge = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_prefix( arg, "autoboot_period" ) ) {
        if ( level > 365 )
            level = 365;

        sysdata.autoboot_period = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_prefix( arg, "checkimmhost" ) ) {
        if ( level != 0 && level != 1 ) {
            send_to_char( "Use 1 to turn it on, 0 to turn in off.\r\n", ch );
            return;
        }
        sysdata.check_imm_host = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "bash_pvp" ) ) {
        sysdata.bash_plr_vs_plr = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "bash_nontank" ) ) {
        sysdata.bash_nontank = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "gouge_pvp" ) ) {
        sysdata.gouge_plr_vs_plr = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "gouge_nontank" ) ) {
        sysdata.gouge_nontank = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "displacement_mod" ) ) {
        sysdata.displacement_mod = level > 0 ? level : 1;
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "phase_mod" ) ) {
        sysdata.phase_mod = level > 0 ? level : 1;
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "dodge_mod" ) ) {
        sysdata.dodge_mod = level > 0 ? level : 1;
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "parry_mod" ) ) {
        sysdata.parry_mod = level > 0 ? level : 1;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "tumble_mod" ) ) {
        sysdata.tumble_mod = level > 0 ? level : 1;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "stun" ) ) {
        sysdata.stun_regular = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "stun_pvp" ) ) {
        sysdata.stun_plr_vs_plr = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "dam_pvp" ) ) {
        sysdata.dam_plr_vs_plr = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "gmb" ) ) {
        sysdata.gmb = value;
        send_to_char( "GMB set.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "get_notake" ) ) {
        sysdata.level_getobjnotake = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "dam_pvm" ) ) {
        sysdata.dam_plr_vs_mob = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "dam_mvp" ) ) {
        sysdata.dam_mob_vs_plr = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "dam_mvm" ) ) {
        sysdata.dam_mob_vs_mob = level;
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "ident_retries" ) || !str_cmp( arg, "ident" ) ) {
        sysdata.ident_retries = level;
        if ( level > 20 )
            send_to_char( "Caution:  This setting may cause the game to lag.\r\n", ch );
        else if ( level <= 0 )
            send_to_char( "Ident lookups turned off.\r\n", ch );
        else
            send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( level < 0 || level > MAX_LEVEL ) {
        send_to_char( "Invalid value for new control.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "read_all" ) )
        sysdata.read_all_mail = level;
    else if ( !str_cmp( arg, "read_free" ) )
        sysdata.read_mail_free = level;
    else if ( !str_cmp( arg, "write_free" ) )
        sysdata.write_mail_free = level;
    else if ( !str_cmp( arg, "take_all" ) )
        sysdata.take_others_mail = level;
    else if ( !str_cmp( arg, "muse" ) )
        sysdata.muse_level = level;
    else if ( !str_cmp( arg, "think" ) )
        sysdata.think_level = level;
    else if ( !str_cmp( arg, "log" ) )
        sysdata.log_level = level;
    else if ( !str_cmp( arg, "build" ) )
        sysdata.build_level = level;
    else if ( !str_cmp( arg, "proto_modify" ) )
        sysdata.level_modify_proto = level;
    else if ( !str_cmp( arg, "get_notake" ) )
        sysdata.level_getobjnotake = level;
    else if ( !str_cmp( arg, "override_private" ) )
        sysdata.level_override_private = level;
    else if ( !str_cmp( arg, "bestow_dif" ) )
        sysdata.bestow_dif = level > 0 ? level : 1;
    else if ( !str_cmp( arg, "forcepc" ) )
        sysdata.level_forcepc = level;
    else if ( !str_cmp( arg, "ban_site_level" ) )
        sysdata.ban_site_level = level;
    else if ( !str_cmp( arg, "ban_race_level" ) )
        sysdata.ban_race_level = level;
    else if ( !str_cmp( arg, "ban_class_level" ) )
        sysdata.ban_class_level = level;
    else if ( !str_cmp( arg, "petsave" ) ) {
        if ( level )
            sysdata.save_pets = TRUE;
        else
            sysdata.save_pets = FALSE;
    }
    else if ( !str_cmp( arg, "pk_loot" ) ) {
        if ( level ) {
            send_to_char( "Pkill looting is enabled.\r\n", ch );
            sysdata.pk_loot = TRUE;
        }
        else {
            send_to_char( "Pkill looting is disabled.  (use cset pkloot 1 to enable)\r\n", ch );
            sysdata.pk_loot = FALSE;
        }
    }
    else if ( !str_cmp( arg, "reqwhoarg" ) ) {
        if ( sysdata.reqwho_arg == FALSE ) {
            send_to_char( "Required who argument enabled.\r\n", ch );
            sysdata.reqwho_arg = TRUE;
        }
        else {
            send_to_char( "Required who argument disabled.\r\n", ch );
            sysdata.reqwho_arg = FALSE;
        }
    }
    else if ( !str_cmp( arg, "wizlock" ) ) {
        if ( sysdata.wiz_lock == FALSE ) {
            send_to_char( "Wizlock enabled.\r\n", ch );
            sysdata.wiz_lock = TRUE;
        }
        else {
            send_to_char( "Wizlock disabled.\r\n", ch );
            sysdata.wiz_lock = FALSE;
        }
    }
    else if ( !str_cmp( arg, "morph_opt" ) ) {
        if ( level )
            sysdata.morph_opt = TRUE;
        else
            sysdata.morph_opt = FALSE;
    }
    else if ( !str_cmp( arg, "mset_player" ) )
        sysdata.level_mset_player = level;
    else {
        send_to_char( "Invalid argument.\r\n", ch );
        return;
    }
    send_to_char( "Ok.\r\n", ch );
    return;
}

void get_reboot_string( void )
{
    snprintf( reboot_time, 50, "%s", asctime( new_boot_time ) );
}

void do_hell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   time;
    bool                    h_d = FALSE;
    struct tm              *tms;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( !*arg ) {
        send_to_char( "Hell who, and for how long?\r\n", ch );
        return;
    }
    if ( !( victim = get_char_world( ch, arg ) ) || IS_NPC( victim ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "There is no point in helling an immortal.\r\n", ch );
        return;
    }
    if ( victim->pcdata->release_date != 0 ) {
        ch_printf( ch, "They are already in hell until %24.24s, by %s.\r\n",
                   ctime( &victim->pcdata->release_date ), victim->pcdata->helled_by );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !*arg || !is_number( arg ) ) {
        send_to_char( "Hell them for how long?\r\n", ch );
        return;
    }

    time = atoi( arg );
    if ( time <= 0 ) {
        send_to_char( "You cannot hell for zero or negative time.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !*arg || !str_cmp( arg, "hours" ) )
        h_d = TRUE;
    else if ( str_cmp( arg, "days" ) ) {
        send_to_char( "Is that value in hours or days?\r\n", ch );
        return;
    }
    else if ( time > 1 ) {
        send_to_char( "You may not hell a person for more than 1 day at a time.\r\n", ch );
        return;
    }
    tms = localtime( &current_time );

    if ( h_d )
        tms->tm_hour += time;
    else
        tms->tm_mday += time;
    victim->pcdata->release_date = mktime( tms );
    victim->pcdata->helled_by = STRALLOC( ch->name );
    ch_printf( ch, "%s will be released from hell at %24.24s.\r\n", victim->name,
               ctime( &victim->pcdata->release_date ) );
    act( AT_MAGIC, "$n disappears in a cloud of hellish light.", victim, NULL, ch, TO_NOTVICT );
    char_from_room( victim );
    char_to_room( victim, get_room_index( ROOM_VNUM_HELL ) );
    act( AT_MAGIC, "$n appears in a could of hellish light.", victim, NULL, ch, TO_NOTVICT );
    do_look( victim, ( char * ) "auto" );
    ch_printf( victim,
               "&CThe Staff are not pleased with your actions.\r\r\n\n"
               "&YYou shall remain in hell for %d %s%s.\r\n", time, ( h_d ? "hour" : "day" ),
               ( time == 1 ? "" : "s" ) );
    save_char_obj( victim );                           /* used to save ch, fixed by *
                                                        * Thoric 09/17/96 */
    return;
}

void do_unhell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    ROOM_INDEX_DATA        *location;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg );
    if ( !*arg ) {
        send_to_char( "Unhell whom..?\r\n", ch );
        return;
    }
    location = ch->in_room;

    if ( ( victim = get_char_world( ch, arg ) ) == NULL || IS_NPC( victim ) ) {
        send_to_char( "There is no one by that name in HELL!\r\n", ch );
        return;
    }

/*
  if(victim->in_room->vnum != ROOM_VNUM_HELL)
  {
    send_to_char("No one like that is in hell.\r\n", ch);
    return;
  }
*/
    if ( victim->pcdata->clan )
        location = get_room_index( victim->pcdata->clan->recall );
    else
        location = get_room_index( ROOM_VNUM_TEMPLE );
    if ( !location )
        location = ch->in_room;
    MOBtrigger = FALSE;
    act( AT_MAGIC, "$n disappears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
    char_from_room( victim );
    char_to_room( victim, location );
    send_to_char( "The gods have smiled on you and released you from hell early!\r\n", victim );
    do_look( victim, ( char * ) "auto" );
    if ( victim != ch )
        send_to_char( "They have been released.\r\n", ch );
    if ( victim->pcdata->helled_by ) {
        if ( str_cmp( ch->name, victim->pcdata->helled_by ) )
            ch_printf( ch,
                       "(You should probably write a note to %s, explaining the early release.)\r\n",
                       victim->pcdata->helled_by );
        STRFREE( victim->pcdata->helled_by );
        victim->pcdata->helled_by = NULL;
    }

    MOBtrigger = FALSE;
    act( AT_MAGIC, "$n appears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
    victim->pcdata->release_date = 0;
    save_char_obj( victim );
    return;
}

/* Vnum search command by Swordbearer */
void do_vsearch( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    bool                    found = FALSE;
    OBJ_DATA               *obj;
    OBJ_DATA               *in_obj;
    int                     obj_counter = 1;
    int                     argi;

    set_pager_color( AT_PLAIN, ch );

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax:  vsearch <vnum>.\r\n", ch );
        return;
    }

    argi = atoi( arg );
    if ( argi < 1 || argi > MAX_VNUM ) {
        send_to_char( "Vnum out of range.\r\n", ch );
        return;
    }
    for ( obj = first_object; obj != NULL; obj = obj->next ) {
        if ( !can_see_obj( ch, obj ) || !( argi == obj->pIndexData->vnum ) )
            continue;

        found = TRUE;
        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj );

        if ( in_obj->carried_by != NULL )
            pager_printf( ch, "[%2d] Level %d %s carried by %s.\r\n", obj_counter, obj->level,
                          obj_short( obj ), PERS( in_obj->carried_by, ch ) );
        else
            pager_printf( ch, "[%2d] [%-5d] %s in %s.\r\n", obj_counter,
                          ( ( in_obj->in_room ) ? in_obj->in_room->vnum : 0 ), obj_short( obj ),
                          ( in_obj->in_room == NULL ) ? "somewhere" : in_obj->in_room->name );

        obj_counter++;
    }

    if ( !found )
        send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
    return;
}

/* 
 * Simple function to let any imm make any player instantly sober.
 * Saw no need for level restrictions on this.
 * Written by Narn, Apr/96 
 */
void do_sober( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MIL];

    set_char_color( AT_IMMORT, ch );

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on mobs.\r\n", ch );
        return;
    }

    if ( victim->pcdata )
        victim->pcdata->condition[COND_DRUNK] = 0;
    send_to_char( "Ok.\r\n", ch );
    set_char_color( AT_IMMORT, victim );
    send_to_char( "You feel sober again.\r\n", victim );
    return;
}

/*
 * Free a social structure     -Thoric
 */
void free_social( SOCIALTYPE * social )
{
    if ( social->name )
        STRFREE( social->name );
    if ( social->char_no_arg )
        STRFREE( social->char_no_arg );
    if ( social->others_no_arg )
        STRFREE( social->others_no_arg );
    if ( social->char_found )
        STRFREE( social->char_found );
    if ( social->others_found )
        STRFREE( social->others_found );
    if ( social->vict_found )
        STRFREE( social->vict_found );
    if ( social->char_auto )
        STRFREE( social->char_auto );
    if ( social->others_auto )
        STRFREE( social->others_auto );
    DISPOSE( social );
}

/*
 * Remove a social from it's hash index     -Thoric
 */
void unlink_social( SOCIALTYPE * social )
{
    SOCIALTYPE             *tmp,
                           *tmp_next;
    int                     hash;

    if ( !social ) {
        bug( "%s", "Unlink_social: NULL social" );
        return;
    }

    if ( social->name[0] < 'a' || social->name[0] > 'z' )
        hash = 0;
    else
        hash = ( social->name[0] - 'a' ) + 1;

    if ( social == ( tmp = social_index[hash] ) ) {
        social_index[hash] = tmp->next;
        return;
    }
    for ( ; tmp; tmp = tmp_next ) {
        tmp_next = tmp->next;
        if ( social == tmp_next ) {
            tmp->next = tmp_next->next;
            return;
        }
    }
}

/*
 * Add a social to the social index table   -Thoric
 * Hashed and insert sorted
 */
void add_social( SOCIALTYPE * social )
{
    int                     hash,
                            x;
    SOCIALTYPE             *tmp,
                           *prev;

    if ( !social ) {
        bug( "%s", "Add_social: NULL social" );
        return;
    }

    if ( !social->name ) {
        bug( "%s", "Add_social: NULL social->name" );
        return;
    }

    if ( !social->char_no_arg ) {
        bug( "%s", "Add_social: NULL social->char_no_arg" );
        return;
    }

    /*
     * make sure the name is all lowercase 
     */
    for ( x = 0; social->name[x] != '\0'; x++ )
        social->name[x] = LOWER( social->name[x] );

    if ( social->name[0] < 'a' || social->name[0] > 'z' )
        hash = 0;
    else
        hash = ( social->name[0] - 'a' ) + 1;

    if ( ( prev = tmp = social_index[hash] ) == NULL ) {
        social->next = social_index[hash];
        social_index[hash] = social;
        return;
    }

    for ( ; tmp; tmp = tmp->next ) {
        if ( ( x = strcmp( social->name, tmp->name ) ) == 0 ) {
            bug( "Add_social: trying to add duplicate name to bucket %d", hash );
            free_social( social );
            return;
        }
        else if ( x < 0 ) {
            if ( tmp == social_index[hash] ) {
                social->next = social_index[hash];
                social_index[hash] = social;
                return;
            }
            prev->next = social;
            social->next = tmp;
            return;
        }
        prev = tmp;
    }

    /*
     * add to end 
     */
    prev->next = social;
    social->next = NULL;
    return;
}

/*
 * Social editor/displayer/save/delete     -Thoric
 */
void do_sedit( CHAR_DATA *ch, char *argument )
{
    SOCIALTYPE             *social;
    char                    arg1[MIL];
    char                    arg2[MIL];

    set_char_color( AT_SOCIAL, ch );

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: sedit <social> [field]\r\n", ch );
        send_to_char( "Syntax: sedit <social> create\r\n", ch );
        if ( get_trust( ch ) > LEVEL_AJ_CPL )
            send_to_char( "Syntax: sedit <social> delete\r\n", ch );
        if ( get_trust( ch ) >= LEVEL_AJ_CPL )
            send_to_char( "Syntax: sedit <save>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( "  cnoarg onoarg cfound ofound vfound cauto oauto\r\n", ch );
        return;
    }

    if ( get_trust( ch ) >= LEVEL_AJ_CPL && !str_cmp( arg1, "save" ) ) {
        save_socials(  );
        send_to_char( "Saved.\r\n", ch );
        return;
    }

    social = find_social( arg1 );
    if ( !str_cmp( arg2, "create" ) ) {
        if ( social ) {
            send_to_char( "That social already exists!\r\n", ch );
            return;
        }
        CREATE( social, SOCIALTYPE, 1 );
        social->name = STRALLOC( arg1 );
        snprintf( arg2, MIL, "You %s.", arg1 );
        social->char_no_arg = STRALLOC( arg2 );
        add_social( social );
        send_to_char( "Social added.\r\n", ch );
        return;
    }

    if ( !social ) {
        send_to_char( "Social not found.\r\n", ch );
        return;
    }

    if ( arg2[0] == '\0' || !str_cmp( arg2, "show" ) ) {
        ch_printf( ch, "Social: %s\r\n\r\nCNoArg: %s\r\n", social->name, social->char_no_arg );
        ch_printf( ch, "ONoArg: %s\r\nCFound: %s\r\nOFound: %s\r\n",
                   social->others_no_arg ? social->others_no_arg : "(not set)",
                   social->char_found ? social->char_found : "(not set)",
                   social->others_found ? social->others_found : "(not set)" );
        ch_printf( ch, "VFound: %s\r\nCAuto : %s\r\nOAuto : %s\r\n",
                   social->vict_found ? social->vict_found : "(not set)",
                   social->char_auto ? social->char_auto : "(not set)",
                   social->others_auto ? social->others_auto : "(not set)" );
        return;
    }
    if ( get_trust( ch ) >= LEVEL_AJ_CPL && !str_cmp( arg2, "delete" ) ) {
        unlink_social( social );
        free_social( social );
        send_to_char( "Deleted.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "cnoarg" ) ) {
        if ( argument[0] == '\0' || !str_cmp( argument, "clear" ) ) {
            send_to_char( "You cannot clear this field.  It must have a message.\r\n", ch );
            return;
        }
        if ( social->char_no_arg )
            STRFREE( social->char_no_arg );
        social->char_no_arg = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "onoarg" ) ) {
        if ( social->others_no_arg )
            STRFREE( social->others_no_arg );
        if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
            social->others_no_arg = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "cfound" ) ) {
        if ( social->char_found )
            STRFREE( social->char_found );
        if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
            social->char_found = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "ofound" ) ) {
        if ( social->others_found )
            STRFREE( social->others_found );
        if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
            social->others_found = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "vfound" ) ) {
        if ( social->vict_found )
            STRFREE( social->vict_found );
        if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
            social->vict_found = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "cauto" ) ) {
        if ( social->char_auto )
            STRFREE( social->char_auto );
        if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
            social->char_auto = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "oauto" ) ) {
        if ( social->others_auto )
            STRFREE( social->others_auto );
        if ( argument[0] != '\0' && str_cmp( argument, "clear" ) )
            social->others_auto = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( get_trust( ch ) > LEVEL_AJ_SGT && !str_cmp( arg2, "name" ) ) {
        bool                    relocate;

        one_argument( argument, arg1 );
        if ( arg1[0] == '\0' ) {
            send_to_char( "Cannot clear name field!\r\n", ch );
            return;
        }
        if ( arg1[0] != social->name[0] ) {
            unlink_social( social );
            relocate = TRUE;
        }
        else
            relocate = FALSE;
        if ( social->name )
            STRFREE( social->name );
        social->name = STRALLOC( arg1 );
        if ( relocate )
            add_social( social );
        send_to_char( "Done.\r\n", ch );
        return;
    }

    /*
     * display usage message 
     */
    do_sedit( ch, ( char * ) "" );
}

/*
 * Free a command structure     -Thoric
 */
void free_command( CMDTYPE * command )
{
    if ( command->name )
        STRFREE( command->name );
    DISPOSE( command );
}

/*
 * Remove a command from it's hash index   -Thoric
 */
void unlink_command( CMDTYPE * command )
{
    CMDTYPE                *tmp,
                           *tmp_next;
    int                     hash;

    if ( !command ) {
        bug( "%s", "Unlink_command NULL command" );
        return;
    }

    hash = command->name[0] % 126;

    if ( command == ( tmp = command_hash[hash] ) ) {
        command_hash[hash] = tmp->next;
        return;
    }
    for ( ; tmp; tmp = tmp_next ) {
        tmp_next = tmp->next;
        if ( command == tmp_next ) {
            tmp->next = tmp_next->next;
            return;
        }
    }
}

/* Add a command to the command hash table -Thoric */
void add_command( CMDTYPE * command )
{
    int                     hash,
                            x;
    CMDTYPE                *tmp,
                           *prev;

    if ( !command ) {
        bug( "%s", "Add_command: NULL command" );
        return;
    }
    if ( !command->name ) {
        bug( "%s", "Add_command: NULL command->name" );
        return;
    }
    if ( !command->do_fun ) {
        bug( "%s", "Add_command: NULL command->do_fun" );
        return;
    }
    /*
     * make sure the name is all lowercase 
     */
    for ( x = 0; command->name[x] != '\0'; x++ )
        command->name[x] = LOWER( command->name[x] );
    hash = command->name[0] % 126;
    if ( ( prev = tmp = command_hash[hash] ) == NULL ) {
        command->next = command_hash[hash];
        command_hash[hash] = command;
        return;
    }
    /*
     * add to the END of the list 
     */
    for ( ; tmp; tmp = tmp->next )
        if ( !tmp->next ) {
            tmp->next = command;
            command->next = NULL;
        }
    return;
}

/* Command editor/displayer/save/delete     -Thoric
 * Added support for interpret flags                            -Shaddai
 */
void do_cedit( CHAR_DATA *ch, char *argument )
{
    CMDTYPE                *command;
    char                    arg1[MIL];
    char                    arg2[MIL];

    set_char_color( AT_IMMORT, ch );
    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax: cedit save cmdtable\r\n", ch );
        if ( get_trust( ch ) > LEVEL_AJ_SGT ) {
            send_to_char( "Syntax: cedit <command> create [code]\r\n", ch );
            send_to_char( "Syntax: cedit <command> delete\r\n", ch );
            send_to_char( "Syntax: cedit <command> show\r\n", ch );
            send_to_char( "Syntax: cedit <command> raise\r\n", ch );
            send_to_char( "Syntax: cedit <command> lower\r\n", ch );
            send_to_char( "Syntax: cedit <command> list\r\n", ch );
            send_to_char( "Syntax: cedit <command> [field]\r\n", ch );
            send_to_char( "\r\nField being one of:\r\n", ch );
            send_to_char( "  level position log code flags cshow\r\n", ch );
        }
        return;
    }
    if ( get_trust( ch ) > LEVEL_AJ_SGT && !str_cmp( arg1, "save" )
         && !str_cmp( arg2, "cmdtable" ) ) {
        save_commands(  );
        send_to_char( "Saved.\r\n", ch );
        return;
    }
    command = find_command( arg1 );
    if ( get_trust( ch ) > LEVEL_AJ_SGT && !str_cmp( arg2, "create" ) ) {
        if ( command && VLD_STR( command->name ) && !str_cmp( command->name, arg1 ) ) {
            ch_printf( ch, "%s command already exists!\r\n", command->name );
            return;
        }
        CREATE( command, CMDTYPE, 1 );
        command->lag_count = 0;                        /* FB */
        command->name = STRALLOC( arg1 );
        command->level = get_trust( ch );
        if ( *argument )
            one_argument( argument, arg2 );
        else
            snprintf( arg2, MIL, "do_%s", arg1 );
        command->do_fun = skill_function( arg2 );
        add_command( command );
        send_to_char( "Command added.\r\n", ch );
        if ( command->do_fun == skill_notfound )
            ch_printf( ch, "Code %s not found.  Set to no code.\r\n", arg2 );
        return;
    }
    if ( !command ) {
        send_to_char( "Command not found.\r\n", ch );
        return;
    }
    else if ( command->level > get_trust( ch ) && str_cmp( ch->name, "Taon" ) ) {
        send_to_char( "You cannot touch this command.\r\n", ch );
        return;
    }
    if ( !VLD_STR( arg2 ) || !str_cmp( arg2, "show" ) ) {
        ch_printf( ch, "Command:  %s\r\nLevel:    %d"
                   "\r\nPosition: %d\r\nLog:      %d"
                   "\r\nCode:     %s\r\nShown:    %s\r\nFlags:  %s\r\n",
                   command->name, command->level, command->position, command->log,
                   skill_name( command->do_fun ), ( command->cshow == 1 ? "Yes" : "No" ),
                   flag_string( command->flags, cmd_flags ) );
        if ( command->userec.num_uses )
            send_timer( &command->userec, ch );
        return;
    }
    if ( get_trust( ch ) <= LEVEL_AJ_SGT ) {
        do_cedit( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "raise" ) ) {
        CMDTYPE                *tmp,
                               *tmp_next;
        int                     hash = command->name[0] % 126;

        if ( ( tmp = command_hash[hash] ) == command ) {
            send_to_char( "That command is already at the top.\r\n", ch );
            return;
        }
        if ( tmp->next == command ) {
            command_hash[hash] = command;
            tmp_next = tmp->next;
            tmp->next = command->next;
            command->next = tmp;
            ch_printf( ch, "Moved %s above %s.\r\n", command->name, command->next->name );
            return;
        }
        for ( ; tmp; tmp = tmp->next ) {
            tmp_next = tmp->next;
            if ( tmp_next->next == command ) {
                tmp->next = command;
                tmp_next->next = command->next;
                command->next = tmp_next;
                ch_printf( ch, "Moved %s above %s.\r\n", command->name, command->next->name );
                return;
            }
        }
        send_to_char( "ERROR -- Not Found!\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "lower" ) ) {
        CMDTYPE                *tmp,
                               *tmp_next;
        int                     hash = command->name[0] % 126;

        if ( command->next == NULL ) {
            send_to_char( "That command is already at the bottom.\r\n", ch );
            return;
        }
        tmp = command_hash[hash];
        if ( tmp == command ) {
            tmp_next = tmp->next;
            command_hash[hash] = command->next;
            command->next = tmp_next->next;
            tmp_next->next = command;

            ch_printf( ch, "Moved %s below %s.\r\n", command->name, tmp_next->name );
            return;
        }
        for ( ; tmp; tmp = tmp->next ) {
            if ( tmp->next == command ) {
                tmp_next = command->next;
                tmp->next = tmp_next;
                command->next = tmp_next->next;
                tmp_next->next = command;

                ch_printf( ch, "Moved %s below %s.\r\n", command->name, tmp_next->name );
                return;
            }
        }
        send_to_char( "ERROR -- Not Found!\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "list" ) ) {
        CMDTYPE                *tmp;
        int                     hash = command->name[0] % 126;

        pager_printf( ch, "Priority placement for [%s]:\r\n", command->name );
        for ( tmp = command_hash[hash]; tmp; tmp = tmp->next ) {
            if ( tmp == command )
                set_pager_color( AT_GREEN, ch );
            else
                set_pager_color( AT_PLAIN, ch );
            pager_printf( ch, "  %s\r\n", tmp->name );
        }
        return;
    }
    if ( !str_cmp( arg2, "delete" ) ) {
        unlink_command( command );
        free_command( command );
        send_to_char( "Deleted.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "code" ) ) {
        DO_FUN                 *fun = skill_function( argument );

        if ( fun == skill_notfound ) {
            send_to_char( "Code not found.\r\n", ch );
            return;
        }
        command->do_fun = fun;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "level" ) ) {
        int                     level = atoi( argument );

        if ( ( level < 0 || level > get_trust( ch ) ) ) {
            send_to_char( "Level out of range.\r\n", ch );
            return;
        }
        command->level = level;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "cshow" ) ) {
        int                     cshow = atoi( argument );

        if ( ( cshow < 0 || cshow > 1 ) ) {
            send_to_char( "cshow should be 1(shown) or 0(not shown).\r\n", ch );
            return;
        }
        command->cshow = cshow;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "log" ) ) {
        int                     log = atoi( argument );

        if ( log < 0 || log > LOG_COMM ) {
            send_to_char( "Log out of range.\r\n", ch );
            return;
        }
        command->log = log;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "position" ) ) {
        int                     position = atoi( argument );

        if ( position < 0 || position > POS_DRAG ) {
            send_to_char( "Position out of range.\r\n", ch );
            return;
        }
        command->position = position;
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "flags" ) ) {
        int                     flag;

        if ( is_number( argument ) )
            flag = atoi( argument );
        else
            flag = get_cmdflag( argument );
        if ( flag < 0 || flag >= 32 ) {
            if ( is_number( argument ) )
                ch_printf( ch, "Invalid flag: range is from 0 to 31.\n" );
            else
                ch_printf( ch, "Unknown flag %s.\n", argument );
            return;
        }

        TOGGLE_BIT( command->flags, 1 << flag );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "name" ) ) {
        bool                    relocate;

        one_argument( argument, arg1 );
        if ( arg1[0] == '\0' ) {
            send_to_char( "Cannot clear name field!\r\n", ch );
            return;
        }
        if ( arg1[0] != command->name[0] ) {
            unlink_command( command );
            relocate = TRUE;
        }
        else
            relocate = FALSE;
        if ( command->name )
            STRFREE( command->name );
        command->name = STRALLOC( arg1 );
        if ( relocate )
            add_command( command );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    do_cedit( ch, ( char * ) "" );
}

/* quest point set - TRI
 * syntax is: qpset char give/take amount
 */

void do_qpset( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    CHAR_DATA              *victim;
    int                     amount;
    bool                    give = TRUE;

    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Cannot qpset as an NPC.\r\n", ch );
        return;
    }
    if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    amount = atoi( arg3 );
    if ( arg[0] == '\0' || arg2[0] == '\0' || amount <= 0 ) {
        send_to_char( "Syntax: qpset <character> <give/take> <amount>\r\n", ch );
        send_to_char( "Amount must be a positive number greater than 0, and less then 51.\r\n",
                      ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "There is no such player currently playing.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Glory cannot be given to or taken from a mob.\r\n", ch );
        return;
    }

    set_char_color( AT_IMMORT, victim );
    if ( nifty_is_name_prefix( arg2, ( char * ) "give" ) ) {
        give = TRUE;
    }
    else if ( nifty_is_name_prefix( arg2, ( char * ) "take" ) )
        give = FALSE;
    else {
        do_qpset( ch, ( char * ) "" );
        return;
    }

    if ( give ) {
        if ( amount > 50 ) {
            send_to_char( "You cannot issue more then 50 qp per event.\r\n", ch );
            return;
        }
        victim->quest_curr += amount;
        victim->quest_accum += amount;
        ch_printf( victim, "Your glory has been increased by %d.\r\n", amount );
        ch_printf( ch, "You have increased the glory of %s by %d.\r\n", victim->name, amount );
    }
    else {
        if ( victim->quest_curr - amount < 0 ) {
            ch_printf( ch, "%s does not have %d glory to take.\r\n", victim->name, amount );
            return;
        }
        else {
            victim->quest_curr -= amount;
            ch_printf( victim, "Your glory has been decreased by %d.\r\n", amount );
            ch_printf( ch, "You have decreased the glory of %s by %d.\r\n", victim->name, amount );
        }
    }
    return;
}

/* Easy way to check a player's glory -- Blodkai, June 97 */
void do_qpstat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    set_char_color( AT_IMMORT, ch );

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax:  qpstat <character>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "No one by that name currently in the Realms.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Mobs don't have glory.\r\n", ch );
        return;
    }
    ch_printf( ch, "%s has %d glory, out of a lifetime total of %d.\r\n", victim->name,
               victim->quest_curr, victim->quest_accum );
    return;
}

/* Simple, small way to make keeping track of small mods easier - Blod */
/*
void do_fixed(CHAR_DATA * ch, char *argument)
{
  char buf[MSL];
  struct tm *t = localtime(&current_time);

  set_char_color(AT_OBJECT, ch);
  if(argument[0] == '\0')
  {
    send_to_char("\r\nUsage:  'fixed list' or 'fixed <message>'", ch);
    if(get_trust(ch) >= LEVEL_AJ_SGT)
      send_to_char(" or 'fixed clear now'\r\n", ch);
    else
      send_to_char("\r\n", ch);
    return;
  }
  if(!str_cmp(argument, "clear now") && get_trust(ch) >= LEVEL_AJ_SGT)
  {
    FILE *fp = FileOpen(FIXED_FILE, "w");
    if(fp)
      FileClose(fp);
    send_to_char("Fixed file cleared.\r\n", ch);
    return;
  }
  if(!str_cmp(argument, "list"))
  {
    send_to_char("\r\n&g[&GDate  &g|  &GVnum&g]\r\n", ch);
    show_file(ch, FIXED_FILE);
  }
  else
  {
    snprintf(buf, MSL, "&g|&G%-2.2d/%-2.2d &g| &G%5d&g|  %s:  &G%s", t->tm_mon + 1, t->tm_mday, ch->in_room ? ch->in_room->vnum : 0, IS_NPC(ch) ? ch->short_descr : ch->name, argument);
    append_to_file(FIXED_FILE, buf);
    send_to_char("Thanks, your modification has been logged.\r\n", ch);
  }
  return;
}
*/

/* Title    : Fshow Gold v1.0
 * Author   : Chris Coulter (aka Gabriel Androctus)
 * MUD Page : http://www.perils.org/
 * Res. Page: http://www.perils.org/goodies/
 * Platform : SMAUG v1.4a
 * Rel. Date: 12.29.00
 * Descrip. : Enhances the handling of player/mob logs.
 */
void do_fshow( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    set_char_color( AT_IMMORT, ch );

    one_argument( argument, arg );
    if ( ( arg[0] == '\0' ) && ch->level > 107 ) {
        send_to_char( "Syntax: fshow < event | plevel | auth | last | sys_bugs | csave | log >\r\n",
                      ch );
        return;
    }
    else if ( ( arg[0] == '\0' ) && ch->level <= 107 ) {
        send_to_char( "Syntax: fshow < event | plevel | auth | last | sys_bugs >\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "event" ) ) {
        if ( doubleexp )
            ch_printf( ch, "Mud Wide Double Exp started running at %24.24s.\r\n",
                       ctime( &starttimedoubleexp ) );
        if ( happyhouron )
            ch_printf( ch, "Mud Wide Happy Hour started running at %24.24s.\r\n",
                       ctime( &starthappyhour ) );
        return;
    }

    if ( !str_cmp( arg, "plevel" ) ) {
        set_char_color( AT_NOTE, ch );
        show_file( ch, PLEVEL_FILE );
        return;
    }
    if ( !str_cmp( arg, "last" ) ) {
        set_char_color( AT_NOTE, ch );
        show_file( ch, LAST_FILE );
        return;
    }
    if ( ch->level > 107 ) {
        if ( !str_cmp( arg, "log" ) ) {
            show_file( ch, VLOG_FILE );
            return;
        }
        else if ( !str_cmp( arg, "csave" ) ) {
            show_file( ch, CSAVE_FILE );
            return;
        }
    }
    if ( !str_cmp( arg, "auth" ) ) {
        show_file( ch, AUTH_FILE );
        return;
    }

    if ( !str_cmp( arg, "sys_bugs" ) ) {
        show_file( ch, BUG_FILE );
        return;
    }

    send_to_char( "No such file.\r\n", ch );
    return;
}

void do_fclear( CHAR_DATA *ch, char *argument )
{
    const char             *filename;
    FILE                   *fp;
    char                    arg1[MSL];
    char                    arg2[MIL];

    one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    set_char_color( AT_RED, ch );
    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Syntax: fclear < plevel | auth | last | sys_bugs >\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "plevel" ) )
        filename = PLEVEL_FILE;
    else if ( !str_cmp( arg1, "log" ) )
        filename = VLOG_FILE;
    else if ( !str_cmp( arg1, "csave" ) )
        filename = CSAVE_FILE;
    else if ( !str_cmp( arg1, "last" ) )
        filename = LAST_FILE;
    else if ( !str_cmp( arg1, "auth" ) )
        filename = AUTH_FILE;
    else if ( !str_cmp( arg1, "sys_bugs" ) )
        filename = BUG_FILE;
    else {
        do_fclear( ch, ( char * ) "" );
        return;
    }
    fp = FileOpen( filename, "w" );
    FileClose( fp );
    ch_printf( ch, "%s file cleared.\r\n", arg1 );
    return;
}

RESERVE_DATA           *first_reserved;
RESERVE_DATA           *last_reserved;
void save_reserved( void )
{
    RESERVE_DATA           *res;
    FILE                   *fp;

    if ( !( fp = FileOpen( SYSTEM_DIR RESERVED_LIST, "w" ) ) ) {
        bug( "Save_reserved: cannot open %s", RESERVED_LIST );
        perror( RESERVED_LIST );
        return;
    }
    for ( res = first_reserved; res; res = res->next )
        fprintf( fp, "%s~\n", res->name );
    fprintf( fp, "$~\n" );
    FileClose( fp );
    return;
}

void do_khistory( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *tmob;
    char                    arg1[MIL],
                            arg2[MIL];
    CHAR_DATA              *vch;
    int                     track;

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( IS_IMMORTAL( ch ) ) {
        argument = one_argument( argument, arg1 );
        argument = one_argument( argument, arg2 );

        if ( arg1[0] == '\0' ) {
            ch_printf( ch, "Syntax: khistory (player), or khistory (player) clear.\r\n" );
            return;
        }
        vch = get_char_world( ch, arg1 );

        if ( !vch || IS_NPC( vch ) ) {
            ch_printf( ch, "They are not here.\r\n" );
            return;
        }

        if ( arg2[0] != '\0' ) {
            if ( !str_cmp( arg2, "clear" ) ) {
                for ( track = 0; track < MAX_KILLTRACK; track++ ) {
                    if ( vch->pcdata->killed[track].vnum ) {
                        vch->pcdata->killed[track].vnum = 0;
                    }
                }
                send_to_char( "You've deleted their kill buffer.\r\n", ch );
                return;
            }
            else {
                send_to_char( "Please use khistory (player) clear.\r\n", ch );
                return;
            }
        }
    }
    else
        vch = ch;

    set_char_color( AT_BLOOD, ch );
    ch_printf( ch, "Kill history for %s:\r\n", vch->name );

    for ( ( track = 0 ); track < MAX_KILLTRACK && vch->pcdata->killed[track].vnum; track++ ) {

        tmob = get_mob_index( vch->pcdata->killed[track].vnum );

        if ( !tmob ) {
            bug( "killhistory: unknown mob vnum" );
            continue;
        }

        set_char_color( AT_RED, ch );
        ch_printf( ch, "   %-30s", capitalize( tmob->short_descr ) );
        if ( IS_IMMORTAL( ch ) ) {
            set_char_color( AT_BLOOD, ch );
            ch_printf( ch, "(" );
            set_char_color( AT_RED, ch );
            ch_printf( ch, "%-5d", tmob->vnum );
            set_char_color( AT_BLOOD, ch );
            ch_printf( ch, ")\r\n" );
        }
        else
            ch_printf( ch, "\r\n" );
    }

    return;
}

/*
 * Command to check for multiple ip addresses in the mud.
 * Added this new struct to do matching
 * If ya think of a better way do it, easiest way I could think of at
 * 2 in the morning :) --Shaddai
 */

typedef struct ipcompare_data IPCOMPARE_DATA;
struct ipcompare_data
{
    struct ipcompare_data  *prev;
    struct ipcompare_data  *next;
    char                   *host;
    char                   *name;
    char                   *user;
    int                     connected;
    int                     count;
    int                     descriptor;
    int                     idle;
    int                     port;
    bool                    printed;
};

void do_ipcompare( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    DESCRIPTOR_DATA        *d;
    char                    arg[MIL];
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                   *addie = NULL;
    bool                    prefix = FALSE,
        suffix = FALSE,
        inarea = FALSE,
        inroom = FALSE,
        inworld = FALSE;
    int                     count = 0,
        times = -1;
    bool                    fMatch;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    set_pager_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "ipcompare pkill\r\n", ch );
        send_to_char( "ipcompare total\r\n", ch );
        send_to_char( "ipcompare <person> [room|area|world] [#]\r\n", ch );
        send_to_char( "ipcompare <site>   [room|area|world] [#]\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "total" ) ) {
        IPCOMPARE_DATA         *first_ip = NULL,
            *last_ip = NULL,
            *hmm,
            *hmm_next;

        for ( d = first_descriptor; d; d = d->next ) {
            fMatch = FALSE;
            for ( hmm = first_ip; hmm; hmm = hmm->next )
                if ( !str_cmp( hmm->host, d->host ) )
                    fMatch = TRUE;
            if ( !fMatch ) {
                IPCOMPARE_DATA         *temp;

                CREATE( temp, IPCOMPARE_DATA, 1 );
                temp->host = STRALLOC( d->host );
                LINK( temp, first_ip, last_ip, next, prev );
                count++;
            }
        }
        for ( hmm = first_ip; hmm; hmm = hmm_next ) {
            hmm_next = hmm->next;
            UNLINK( hmm, first_ip, last_ip, next, prev );
            if ( hmm->host )
                STRFREE( hmm->host );
            DISPOSE( hmm );
        }
        ch_printf( ch, "There were %d unique ip addresses found.\r\n", count );
        return;
    }
    else if ( !str_cmp( arg, "pkill" ) ) {
        IPCOMPARE_DATA         *first_ip = NULL,
            *last_ip = NULL,
            *hmm,
            *hmm_next;

        send_to_pager( "\r\nDesc|Con|Idle| Port | Player      ", ch );
        if ( get_trust( ch ) >= LEVEL_IMMORTAL )
            send_to_pager( "@HostIP           ", ch );
        if ( get_trust( ch ) >= LEVEL_AJ_CPL )
            send_to_pager( "| Username\r\n", ch );
        send_to_pager( "----+---+----+------+-------------", ch );
        if ( get_trust( ch ) >= LEVEL_IMMORTAL )
            send_to_pager( "------------------", ch );
        if ( get_trust( ch ) >= LEVEL_AJ_CPL )
            send_to_pager( "+---------\r\n", ch );

        for ( d = first_descriptor; d; d = d->next ) {
            IPCOMPARE_DATA         *temp;

            if ( ( d->connected != CON_PLAYING && d->connected != CON_EDITING )
                 || d->character == NULL || !CAN_PKILL( d->character )
                 || !can_see( ch, d->character ) )
                continue;
            CREATE( temp, IPCOMPARE_DATA, 1 );
            temp->host = STRALLOC( d->host );
            temp->descriptor = d->descriptor;
            temp->connected = d->connected;
            temp->idle = d->idle;
            temp->port = d->port;
            temp->name =
                ( d->original ? STRALLOC( d->original->name ) : d->
                  character ? STRALLOC( d->character->name ) : STRALLOC( "(none)" ) );
            temp->count = 0;
            temp->printed = FALSE;
            LINK( temp, first_ip, last_ip, next, prev );
        }

        for ( d = first_descriptor; d; d = d->next ) {
            fMatch = FALSE;
            if ( ( d->connected != CON_PLAYING && d->connected != CON_EDITING )
                 || d->character == NULL || !can_see( ch, d->character ) )
                continue;
            for ( hmm = first_ip; hmm; hmm = hmm->next ) {
                if ( !str_cmp( hmm->host, d->host )
                     && str_cmp( hmm->name,
                                 ( d->original ? d->original->name : d->character ? d->character->
                                   name : "(none)" ) ) ) {
                    fMatch = TRUE;
                    break;
                }
            }
            if ( fMatch && hmm ) {
                hmm->count++;
                if ( !hmm->printed && hmm->count > 0 ) {
                    pager_printf( ch, " %3d| %2d|%4d|%6d| %-12s", hmm->descriptor, hmm->connected,
                                  hmm->idle / 4, hmm->port, hmm->name );
                    if ( get_trust( ch ) >= LEVEL_IMMORTAL )
                        pager_printf( ch, "@%-16s \r\n", hmm->host );
                    hmm->printed = TRUE;
                }
                pager_printf( ch, " %3d| %2d|%4d|%6d| %-12s", d->descriptor, d->connected,
                              d->idle / 4, d->port,
                              d->original ? d->original->name : d->character ? d->character->
                              name : "(none)" );
                if ( get_trust( ch ) >= LEVEL_IMMORTAL )
                    pager_printf( ch, "@%-16s \r\n", d->host );
            }
        }
        for ( hmm = first_ip; hmm; hmm = hmm_next ) {
            hmm_next = hmm->next;
            UNLINK( hmm, first_ip, last_ip, next, prev );
            if ( hmm->name )
                STRFREE( hmm->name );
            if ( hmm->host )
                STRFREE( hmm->host );
            DISPOSE( hmm );
        }
        return;
    }
    if ( arg1[0] != '\0' ) {
        if ( is_number( arg1 ) )
            times = atoi( arg1 );
        else {
            if ( !str_cmp( arg1, "room" ) )
                inroom = TRUE;
            else if ( !str_cmp( arg1, "area" ) )
                inarea = TRUE;
            else
                inworld = TRUE;
        }
        if ( arg2[0] != '\0' ) {
            if ( is_number( arg2 ) )
                times = atoi( arg2 );
            else {
                send_to_char( "Please see help ipcompare for more info.\r\n", ch );
                return;
            }
        }
    }
    if ( ( victim = get_char_world( ch, arg ) ) != NULL && victim->desc ) {
        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }
        addie = victim->desc->host;
    }
    else {
        addie = arg;
        if ( arg[0] == '*' ) {
            prefix = TRUE;
            addie++;
        }
        if ( addie[strlen( addie ) - 1] == '*' ) {
            suffix = TRUE;
            addie[strlen( addie ) - 1] = '\0';
        }
    }
    send_to_pager( "\r\nDesc|Con|Idle| Port | Player      ", ch );
    if ( get_trust( ch ) >= LEVEL_IMMORTAL )
        send_to_pager( "@HostIP           ", ch );
    if ( get_trust( ch ) >= LEVEL_AJ_CPL )
        send_to_pager( "| Username\r\n", ch );
    send_to_pager( "----+---+----+------+-------------", ch );
    if ( get_trust( ch ) >= LEVEL_IMMORTAL )
        send_to_pager( "------------------", ch );
    if ( get_trust( ch ) >= LEVEL_AJ_CPL )
        send_to_pager( "+---------\r\n", ch );
    for ( d = first_descriptor; d; d = d->next ) {
        if ( !d->character || ( d->connected != CON_PLAYING && d->connected != CON_EDITING )
             || !can_see( ch, d->character ) )
            continue;
        if ( inroom && ch->in_room != d->character->in_room )
            continue;
        if ( inarea && ch->in_room->area != d->character->in_room->area )
            continue;
        if ( times > 0 && count == ( times - 1 ) )
            break;
        if ( prefix && suffix && strstr( addie, d->host ) )
            fMatch = TRUE;
        else if ( prefix && !str_suffix( addie, d->host ) )
            fMatch = TRUE;
        else if ( suffix && !str_prefix( addie, d->host ) )
            fMatch = TRUE;
        else if ( !str_cmp( d->host, addie ) )
            fMatch = TRUE;
        else
            fMatch = FALSE;
        if ( fMatch ) {
            count++;
            pager_printf( ch, " %3d| %2d|%4d|%6d| %-12s", d->descriptor, d->connected, d->idle / 4,
                          d->port,
                          d->original ? d->original->name : d->character ? d->character->
                          name : "(none)" );
            if ( get_trust( ch ) >= LEVEL_IMMORTAL )
                pager_printf( ch, "@%-16s \r\n", d->host );
        }
    }
    pager_printf( ch, "%d user%s.\r\n", count, count == 1 ? "" : "s" );
    return;
}

/*
 * New nuisance flag to annoy people that deserve it :) --Shaddai
 */
void do_nuisance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    arg1[MIL];
    char                    arg2[MIL];
    struct tm              *now_time;
    int                     time = 0,
        max_time = 0,
        power = 1;
    bool                    minute = FALSE,
        day = FALSE,
        hour = FALSE;

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: nuisance <victim> [Options]\r\n", ch );
        send_to_char( "Options:\r\n", ch );
        send_to_char( "  power <level 1-10>\r\n", ch );
        send_to_char( "  time  <days>\r\n", ch );
        send_to_char( "  maxtime <#> <minutes/hours/days>\r\n", ch );
        send_to_char( "Defaults: Time -- forever, power -- 1, maxtime 8 days.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "There is no one on with that name.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't set a nuisance flag on a mob.\r\n", ch );
        return;
    }

    if ( get_trust( ch ) <= get_trust( victim ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to nuisance you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }

    if ( victim->pcdata->nuisance ) {
        send_to_char( "That flag has already been set.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    while ( argument[0] != '\0' ) {
        if ( !str_cmp( arg1, "power" ) ) {
            argument = one_argument( argument, arg1 );
            if ( arg1[0] == '\0' || !is_number( arg1 ) ) {
                send_to_char( "Power option syntax: power <number>\r\n", ch );
                return;
            }
            if ( ( power = atoi( arg1 ) ) < 1 || power > 10 ) {
                send_to_char( "Power must be 1 - 10.\r\n", ch );
                return;
            }
        }
        else if ( !str_cmp( arg1, "time" ) ) {
            argument = one_argument( argument, arg1 );
            if ( arg1[0] == '\0' || !is_number( arg1 ) ) {
                send_to_char( "Time option syntax: time <number> (In days)\r\n", ch );
                return;
            }
            if ( ( time = atoi( arg1 ) ) < 1 ) {
                send_to_char( "Time must be a positive number.\r\n", ch );
                return;
            }
        }
        else if ( !str_cmp( arg1, "maxtime" ) ) {
            argument = one_argument( argument, arg1 );
            argument = one_argument( argument, arg2 );
            if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg1 ) ) {
                send_to_char( "Maxtime option syntax: maxtime <number> <minute|day|hour>\r\n", ch );
                return;
            }
            if ( ( max_time = atoi( arg1 ) ) < 1 ) {
                send_to_char( "Maxtime must be a positive number.\r\n", ch );
                return;
            }
            if ( !str_cmp( arg2, "minutes" ) )
                minute = TRUE;
            else if ( !str_cmp( arg2, "hours" ) )
                hour = TRUE;
            else if ( !str_cmp( arg2, "days" ) )
                day = TRUE;
        }
        else {
            ch_printf( ch, "Unknown option %s.\r\n", arg1 );
            return;
        }
        argument = one_argument( argument, arg1 );
    }

    if ( minute && ( max_time < 1 || max_time > 59 ) ) {
        send_to_char( "Minutes must be 1 to 59.\r\n", ch );
        return;
    }
    else if ( hour && ( max_time < 1 || max_time > 23 ) ) {
        send_to_char( "Hours must be 1 - 23.\r\n", ch );
        return;
    }
    else if ( day && ( max_time < 1 || max_time > 999 ) ) {
        send_to_char( "Days must be 1 - 999.\r\n", ch );
        return;
    }
    else if ( !max_time ) {
        day = TRUE;
        max_time = 7;
    }
    CREATE( victim->pcdata->nuisance, NUISANCE_DATA, 1 );
    victim->pcdata->nuisance->time = current_time;
    victim->pcdata->nuisance->flags = 1;
    victim->pcdata->nuisance->power = power;
    now_time = localtime( &current_time );

    if ( minute )
        now_time->tm_min += max_time;
    else if ( hour )
        now_time->tm_hour += max_time;
    else
        now_time->tm_mday += max_time;

    victim->pcdata->nuisance->max_time = mktime( now_time );
    if ( time ) {
        add_timer( victim, TIMER_NUISANCE, ( 28800 * time ), NULL, 0 );
        ch_printf( ch, "Nuisance flag set for %d days.\r\n", time );
    }
    else
        send_to_char( "Nuisance flag set forever\r\n", ch );
    return;
}

void do_unnuisance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    TIMER                  *timer,
                           *timer_next;
    char                    arg[MIL];

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }
    one_argument( argument, arg );

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "There is no one on with that name.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't remove a nuisance flag from a mob.\r\n", ch );
        return;
    }
    if ( get_trust( ch ) <= get_trust( victim ) ) {
        send_to_char( "You can't do that.\r\n", ch );
        return;
    }
    if ( !victim->pcdata->nuisance ) {
        send_to_char( "They do not have that flag set.\r\n", ch );
        return;
    }
    for ( timer = victim->first_timer; timer; timer = timer_next ) {
        timer_next = timer->next;
        if ( timer->type == TIMER_NUISANCE )
            extract_timer( victim, timer );
    }
    DISPOSE( victim->pcdata->nuisance );
    send_to_char( "Nuisance flag removed.\r\n", ch );
    return;
}

void do_pcrename( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    newname[MSL];
    char                    oldname[MSL];
    char                    buf[MSL];

    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );
    smash_tilde( arg2 );

    if ( IS_NPC( ch ) )
        return;

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: rename <victim> <new name>\r\n", ch );
        return;
    }

    if ( !check_parse_name( arg2, 1 ) ) {
        send_to_char( "Illegal name.\r\n", ch );
        return;
    }
    /*
     * Just a security precaution so you don't rename someone you don't mean 
     * * too --Shaddai
     */
    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "That person is not in the room.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't rename NPC's.\r\n", ch );
        return;
    }

    if ( get_trust( ch ) < get_trust( victim ) ) {
        send_to_char( "&RNever try to use a command against a higher Staff member...\r\n", ch );
        send_to_char( "&RYou have been warned....\r\n", ch );
        act( AT_RED, "$n tried to pcrename you against your wishes.", ch, NULL, victim, TO_VICT );
        return;
    }
    snprintf( newname, MSL, "%s%c/%s", PLAYER_DIR, tolower( arg2[0] ), capitalize( arg2 ) );
    snprintf( oldname, MSL, "%s%c/%s", PLAYER_DIR, tolower( victim->pcdata->filename[0] ),
              capitalize( victim->pcdata->filename ) );
    if ( access( newname, F_OK ) == 0 ) {
        send_to_char( "That name already exists.\r\n", ch );
        return;
    }

    /*
     * Have to remove the old god entry in the directories 
     */
    if ( IS_IMMORTAL( victim ) ) {
        char                    godname[MSL];

        snprintf( godname, MSL, "%s%s", STAFF_DIR, capitalize( victim->pcdata->filename ) );
        remove( godname );
    }

    /*
     * Take care of renaming the vault while you can 
     */
    char                    oldvault[MSL];
    char                    newvault[MSL];

    rename_vault( ch, newname );

    snprintf( oldvault, sizeof( oldvault ), "%s%s", VAULT_DIR, oldname );
    snprintf( newvault, sizeof( newvault ), "%s%s", VAULT_DIR, newname );
    if ( !rename( oldvault, newvault ) )
        log_printf( "%s: Vault data for %s renamed to %s.\r\n", __FUNCTION__, oldname, newname );

    /*
     * Remember to change the names of the areas 
     */
    if ( victim->pcdata->area ) {
        char                    filename[MSL];
        char                    newfilename[MSL];

        snprintf( filename, MSL, "%s%s.are", BUILD_DIR, victim->name );
        snprintf( newfilename, MSL, "%s%s.are", BUILD_DIR, capitalize( arg2 ) );
        remove( newfilename );
        rename( filename, newfilename );
        snprintf( filename, MSL, "%s%s.are.bak", BUILD_DIR, victim->name );
        snprintf( newfilename, MSL, "%s%s.are.bak", BUILD_DIR, capitalize( arg2 ) );
        remove( newfilename );
        rename( filename, newfilename );
    }

    STRFREE( victim->name );
    victim->name = STRALLOC( capitalize( arg2 ) );
    STRFREE( victim->pcdata->filename );
    victim->pcdata->filename = STRALLOC( capitalize( arg2 ) );
    if ( remove( oldname ) ) {
        snprintf( buf, MSL, "Error: Couldn't delete file %s in do_rename.", oldname );
        send_to_char( "Couldn't delete the old file!\r\n", ch );
        log_string( oldname );
    }
    /*
     * Time to save to force the affects to take place 
     */
    save_char_obj( victim );

    /*
     * Now lets update the wizlist 
     */
    if ( IS_IMMORTAL( victim ) )
        send_to_char( "Character was renamed.\r\n", ch );
}

/*
 * Counts the number of times a target string occurs in a source string. Case insensitive, 
 * by Gorog. It's only used in the three functions below, but as I mentioned with the trunc 
 * function, I see no point in repetitiously typing out that code. -Orion
 */
int str_count( char *psource, char *ptarget )
{
    char                   *ptemp = psource;
    int                     count = 0;

    while ( ( ptemp = strstr( ptemp, ptarget ) ) ) {
        ptemp++;
        count++;
    }
    return count;
}

/* Use cedit to add in as imm command.
 * Syntax is: Oinvade <# of objects> <object vnum> [level of object]
 * 
 * If no level is specified level will be 0. I always hated invoking
 * objects at level 50+ automatically as an imm.
 */
void do_oinvade( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    int                     count,
                            created,
                            level;
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    count = atoi( arg1 );
    level = atoi( arg3 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Invade <# objs> <obj vnum> <level>\r\n", ch );
        return;
    }
    if ( count > 300 ) {
        send_to_char( "Whoa...Less than 300 please.\r\n", ch );
        return;
    }
    if ( ( pObjIndex = get_obj_index( atoi( arg2 ) ) ) == NULL ) {
        send_to_char( "No object has that vnum.\r\n", ch );
        return;
    }

    for ( created = 0; created < count; created++ ) {

        obj = create_object( pObjIndex, level );
        obj = obj_to_char( obj, ch );
    }
    ch_printf( ch, "&YAt your bidding %d level %d %s appear. (&W#%d &Y- &W%s&Y)\r\n", created,
               level, pObjIndex->short_descr, pObjIndex->vnum, pObjIndex->name );

    return;
}

//oscatter allow an object to be scattered throughout an area. -Taon
void do_oscatter( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    OBJ_INDEX_DATA         *pObjIndex;
    AREA_DATA              *tarea;
    ROOM_INDEX_DATA        *location;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    arg4[MSL];
    int                     count,
                            created;
    bool                    found = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );
    count = atoi( arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_to_char( "oscatter <area> <# of objects> <object> <bury/hide> (optional!)\r\n", ch );
        return;
    }

    for ( tarea = first_area; tarea; tarea = tarea->next )
        if ( !str_cmp( tarea->filename, arg1 ) ) {
            found = TRUE;
            break;
        }
    if ( !found ) {
        send_to_char( "Area not found.\r\n", ch );
        return;
    }
    if ( count > 200 ) {
        send_to_char( "Less than 200 at a time please.\r\n", ch );
        return;
    }
    if ( arg4[0] != '\0' && str_cmp( arg4, "bury" ) && str_cmp( arg4, "hide" ) ) {
        send_to_char( "Please use 'bury' or 'hide' arguments, or leave blank.\r\n", ch );
        return;
    }
    if ( ( pObjIndex = get_obj_index( atoi( arg3 ) ) ) == NULL ) {
        send_to_char( "No object has that vnum.\r\n", ch );
        return;
    }

    bool                    change = FALSE;

    for ( created = 0; created < count; created++ ) {
        if ( ( location =
               get_room_index( number_range( tarea->low_r_vnum, tarea->hi_r_vnum ) ) ) == NULL ) {
            --created;
            continue;
        }
        obj = create_object( pObjIndex, 1 );
        if ( !str_cmp( arg4, "bury" ) ) {
            switch ( location->sector_type ) {
                case SECT_ROAD:
                case SECT_VROAD:
                case SECT_HROAD:
                case SECT_INSIDE:
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                case SECT_AIR:
                    change = TRUE;
            }
            if ( change ) {
                extract_obj( obj );
                --created;
                change = FALSE;
                continue;
            }

//Volk: Let's check you CAN bury in this room
            xSET_BIT( obj->extra_flags, ITEM_BURIED );
        }

        if ( !str_cmp( arg4, "hide" ) )
            xSET_BIT( obj->extra_flags, ITEM_HIDDEN );

        obj_to_room( obj, location );
    }

    send_to_char( "The object scattering was successful!\r\n", ch );

    return;
}

/* Use cedit to add in as imm command.
 * Syntax is: Invade <area filename> <# of invaders> <vnum of mobs> 
 * example: Invade newacad.are 300 10399 would send 300 mistress tsythia's rampaging 
 * through the academy. This function doesnt make the mobiles aggressive but can be
 * modified to do so easily if you wish this, or you can just edit the mob before
 * hand.
 */
void do_invade( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    CHAR_DATA              *victim;
    AREA_DATA              *tarea;
    ROOM_INDEX_DATA        *location;
    int                     count,
                            created;
    bool                    found = FALSE;
    MOB_INDEX_DATA         *pMobIndex;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    count = atoi( arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Invade <area> <# of invaders> <mob vnum>\r\n", ch );
        return;
    }
    for ( tarea = first_area; tarea; tarea = tarea->next )
        if ( !str_cmp( tarea->filename, arg1 ) ) {
            found = TRUE;
            break;
        }
    if ( !found ) {
        send_to_char( "Area not found.\r\n", ch );
        return;
    }
    if ( count > 300 ) {
        send_to_char( "Whoa...Less than 300 please.\r\n", ch );
        return;
    }
    if ( ( pMobIndex = get_mob_index( atoi( arg3 ) ) ) == NULL ) {
        send_to_char( "No mobile has that vnum.\r\n", ch );
        return;
    }

    for ( created = 0; created < count; created++ ) {
        if ( ( location =
               get_room_index( number_range( tarea->low_r_vnum, tarea->hi_r_vnum ) ) ) == NULL ) {
            --created;
            continue;
        }
        if ( IS_SET( location->room_flags, ROOM_SAFE ) ) {
            --created;
            continue;
        }
        victim = create_mobile( pMobIndex );
        char_to_room( victim, location );
    }
    send_to_char( "The invasion was successful!\r\n", ch );

    return;
}

void                    remove_all_equipment( CHAR_DATA *ch );

void do_life( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MIL];

    if ( IS_NPC( ch ) )
        return;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg );

    if ( ch->pcdata->tmplevel < LEVEL_IMMORTAL && !IS_IMMORTAL( ch ) ) {
        error( ch );
        return;
    }
    if ( ( arg[0] == '\0' ) && ( IS_IMMORTAL( ch ) ) && ( !ch->pcdata->tmplevel > LEVEL_IMMORTAL ) ) {
        send_to_char( "Syntax: Life <level>\r\n", ch );
        return;
    }
    if ( IS_IMMORTAL( ch ) ) {
        if ( is_number( arg ) ) {
            short                   level;
            short                   num;

            num = 1;
            level = atoi( arg );
            if ( level > ch->level || level < 2 ) {
                send_to_char( "Invalid Level.\r\n", ch );
                return;
            }
            remove_all_equipment( ch );
            if ( ch->pcdata->tmplevel == 0 )
                ch->pcdata->tmplevel = ch->level;
            if ( ch->pcdata->tmptrust == 0 )
                ch->pcdata->tmptrust = ch->trust;
            ch->level = level;
            ch->trust = 0;
            if ( xIS_SET( ch->act, PLR_WIZINVIS ) )
                xREMOVE_BIT( ch->act, PLR_WIZINVIS );
            ch->pcdata->tmpmax_hit = ch->max_hit;
            ch->pcdata->tmpmax_move = ch->max_move;
            if ( IS_BLOODCLASS( ch ) ) {
                ch->pcdata->tmpmax_mana = ch->max_blood;
            }
            else {
                ch->pcdata->tmpmax_mana = ch->max_mana;
            }
            save_char_obj( ch );
            ch->max_hit = 10;
            ch->hit = 10;
            if ( IS_BLOODCLASS( ch ) ) {
                ch->max_blood = 10;
                ch->blood = 10;
            }
            else {
                ch->max_mana = 10;
                ch->mana = 10;
            }
            ch->max_move = 10;
            ch->move = 10;
            if ( !xIS_SET( ch->act, PLR_LIFE ) )
                xSET_BIT( ch->act, PLR_LIFE );
            if ( IS_SECONDCLASS( ch ) && !IS_THIRDCLASS( ch ) ) {
                level *= 2;
            }
            if ( IS_THIRDCLASS( ch ) ) {
                level *= 3;
            }
            while ( num <= level ) {
                advance_level( ch );
                num++;
            }
            restore_char( ch );
            snprintf( buf, MIL, "%s has taken on a player life form within the realms.", ch->name );
            echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
            act( AT_PLAIN, "$n coalesces into a player life form.", ch, NULL, NULL, TO_ROOM );
            act( AT_PLAIN, "You coalesce into a player life form.", ch, NULL, NULL, TO_CHAR );
            return;
        }
        else if ( ch->pcdata->tmplevel > LEVEL_DEMIGOD ) {
            ch->level = ch->pcdata->tmplevel;
            ch->trust = ch->pcdata->tmptrust;
            ch->max_hit = ch->pcdata->tmpmax_hit;
            ch->max_move = ch->pcdata->tmpmax_move;
            if ( IS_BLOODCLASS( ch ) ) {
                ch->max_blood = ch->pcdata->tmpmax_mana;
            }
            else {
                ch->max_mana = ch->pcdata->tmpmax_mana;
            }
            ch->pcdata->tmpmax_hit = 0;
            ch->pcdata->tmpmax_move = 0;
            ch->pcdata->tmpmax_mana = 0;
            ch->pcdata->tmplevel = 0;
            ch->pcdata->tmptrust = 0;
            if ( !xIS_SET( ch->act, PLR_LIFE ) )
                xREMOVE_BIT( ch->act, PLR_LIFE );
            save_char_obj( ch );
            act( AT_PLAIN, "$n returns to a Staff state.", ch, NULL, NULL, TO_ROOM );
            act( AT_PLAIN, "You return to your Staff state.", ch, NULL, NULL, TO_CHAR );
            snprintf( buf, MIL, "%s has returned to a Staff state within the realms.", ch->name );
            echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
            if ( ch->max_hit > 100000 || ch->max_hit < 0 ) {
                ch->max_hit = 30000;
            }
            restore_char( ch );
            return;
        }
    }
    else if ( ch->pcdata->tmplevel > LEVEL_DEMIGOD ) {
        ch->level = ch->pcdata->tmplevel;
        ch->trust = ch->pcdata->tmptrust;
        ch->max_hit = ch->pcdata->tmpmax_hit;
        ch->max_move = ch->pcdata->tmpmax_move;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->max_blood = ch->pcdata->tmpmax_mana;
        }
        else {
            ch->max_mana = ch->pcdata->tmpmax_mana;
        }
        ch->pcdata->tmpmax_hit = 0;
        ch->pcdata->tmpmax_move = 0;
        ch->pcdata->tmpmax_mana = 0;
        ch->pcdata->tmplevel = 0;
        ch->pcdata->tmptrust = 0;
        if ( !xIS_SET( ch->act, PLR_LIFE ) )
            xREMOVE_BIT( ch->act, PLR_LIFE );
        save_char_obj( ch );
        act( AT_PLAIN, "$n returns to a Staff state.", ch, NULL, NULL, TO_ROOM );
        act( AT_PLAIN, "You return to your Staff state.", ch, NULL, NULL, TO_CHAR );
        snprintf( buf, MIL, "%s has returned to a Staff state within the realms.", ch->name );
        echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
        if ( ch->max_hit > 100000 || ch->max_hit < 0 ) {
            ch->max_hit = 30000;
        }
        restore_char( ch );
        return;
    }
    else
        error( ch );

    return;
}

/* New command to view a player's skills - Samson 4-13-98 */
void do_viewskills( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    int                     sn;
    int                     col;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "&zSyntax: viewskills <player>.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "No such person in the game.\r\n", ch );
        return;
    }

    col = 0;

    if ( !IS_NPC( victim ) ) {
        set_char_color( AT_MAGIC, ch );
        pager_printf( ch, "&CYou are viewing the skills of %s\r\n", victim->name );
        for ( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ ) {
            if ( skill_table[sn]->name == NULL )
                break;

            if ( victim->pcdata->learned[sn] == 0 )
                continue;
            pager_printf( ch, "&c%20s %3d%% ", skill_table[sn]->name, victim->pcdata->learned[sn] );
            if ( ++col % 3 == 0 )
                send_to_pager( "\r\n", ch );
        }
        if ( IS_DRAGON( victim ) ) {
            send_to_char( "\r\n&CHas the following DRAGONFORM (hidden) skills:\r\n", ch );
            for ( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ ) {
                if ( skill_table[sn]->name == NULL )
                    break;

                if ( victim->pcdata->dlearned[sn] == 0 )
                    continue;
                pager_printf( ch, "&c%20s %3d%% ", skill_table[sn]->name,
                              victim->pcdata->dlearned[sn] );
                if ( ++col % 3 == 0 )
                    send_to_pager( "\r\n", ch );
            }

        }

    }
    if ( !victim || !IS_NPC( victim ) )
        pager_printf( ch, "\r\n\r\n&C%s is a %s race with the class of %s", victim->name,
                      race_table[victim->race]->race_name, class_table[victim->Class]->who_name );
    send_to_pager( ".\r\n\r\n", ch );
    if ( !IS_NPC( victim ) )
        pager_printf( ch, "\r\n&C%s has %d remaining practices left.\r\n", victim->name,
                      victim->practice );
    return;
}

/*  Dump command...This command creates a text file with the stats of every  *
 *  mob, or object in the mud, depending on the argument given.              *
 *  Obviously, this will tend to create HUGE files, so it is recommended     *
 *  that it be only given to VERY high level imms, and preferably those      *
 *  with shell access, so that they may clean it out, when they are done     *
 *  with it.
 */

void do_wpeace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;

    if ( !ch->pcdata )
        return;

    set_char_color( AT_IMMORT, ch );

    send_to_char( "Declaring World Peace ...\r\n", ch );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;
        if ( ch->desc == NULL || ch->desc->connected != CON_PLAYING )
            continue;
        {
            if ( vch->fighting ) {
                stop_fighting( vch, TRUE );
                set_position( vch, POS_STUNNED );
                WAIT_STATE( vch, 5 * PULSE_VIOLENCE + 2 );
            }
            /*
             * Added by Narn, Nov 28/95 
             */
            stop_hating( vch );
            stop_hunting( vch );
            stop_fearing( vch );
            act( AT_IMMORT, "$n has declared world peace.", ch, NULL, vch, TO_VICT );
        }
    }
    send_to_char( "The world is now at peace.\r\n", ch );
    return;
}

void do_saveall( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *pfs;
    AREA_DATA              *barea;
    char                    filename[256];
    char                    buf[MSL * 2];

    send_to_char( "&YSave all in process this may take a few moments\r\n", ch );
    send_to_char( "&Ydepending on the amount of players that are on.\r\n", ch );

    /*
     * Save The Help Files.because I hate losing helpfiles 
     */
    do_hset( ch, ( char * ) "save" );

    /*
     * Save all the areas in the Building Directory that are loaded 
     */
    send_to_char( "&OSaving Builders Areas....\r\n", ch );
    for ( barea = first_build; barea; barea = barea->next ) {
        if ( !IS_SET( barea->status, AREA_LOADED ) )
            continue;
        snprintf( filename, 256, "%s%s", BUILD_DIR, barea->filename );
        fold_area( barea, filename, FALSE );
    }

    /*
     * Save all characters now 
     */
    send_to_char( "&OSaving all online players.\r\n", ch );
    for ( pfs = first_char; pfs; pfs = pfs->next ) {

        if ( !IS_NPC( pfs ) ) {
            save_char_obj( pfs );
            pager_printf( ch, "&PNow Saving %-s...\r\n", pfs->name );
        }
    }

    snprintf( buf, MSL * 2,
              "&RYou have been saved by your friendly &r6 Dragons&R Staff member: %s ...&D",
              ch->name );
    do_echo( ch, buf );

    /*
     * Time to save the Easier Stuff 
     */

    /*
     * Lets Save Classes 
     */
    save_classes(  );
    send_to_char( "&OClasses Saved.\r\n", ch );

    /*
     * Save the commands table just incase 
     */
    save_commands(  );
    send_to_char( "&OCommands Tables Saved.\r\n", ch );

    write_quest_list(  );
    send_to_char( "&OQuests saved.\r\n", ch );

    /*
     * Save the Socials table 
     */
    save_socials(  );
    send_to_char( "&OSocials Table Saved.\r\n", ch );

    /*
     * save the skills table 
     */
    save_skill_table(  );
    send_to_char( "&OSkill Table Saved.\r\n", ch );

    /*
     * Save the herb table 
     */
    save_herb_table(  );
    send_to_char( "&OHerb Table Saved.\r\n", ch );

    send_to_char( "\r\n", ch );
    send_to_char( "&RAll Game Data is Now Saved!!.&D\r\n", ch );
    send_to_char( "&RYou May Now Reboot the Mud!!..&D\r\n", ch );
}

/* do_promote added by Vladaar - AJ 1/31/03 */
void do_promote( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    CHAR_DATA              *victim;
    char                    buf[MIL];
    int                     value;
    OBJ_DATA               *obj;
    int                     iLevel;

    set_char_color( AT_YELLOW, ch );

    if ( IS_NPC( ch ) || !IS_IMMORTAL( ch ) ) {
        send_to_char( "You can't promote.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "You cannot promote yourself\r\n", ch );
        return;
    }

    if ( victim->level >= ch->level ) {
        send_to_char( "You cannot promote someone of equal or higher level then yourself.\r\n",
                      ch );
        return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_to_char( "Syntax: Promote <player> level <value>\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "level" ) ) {

        if ( !is_number( arg3 ) ) {
            send_to_char( "Value must be numeric.\r\n", ch );
            return;
        }
        value = atoi( arg3 );

        if ( value < LEVEL_IMMORTAL || value > MAX_LEVEL ) {
            ch_printf( ch, "Value must be between %d and %d.\r\n", LEVEL_IMMORTAL, MAX_LEVEL );
            return;
        }
        victim->level = value;
        set_char_color( AT_WHITE, victim );
        send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
        set_char_color( AT_LBLUE, victim );
        if ( victim->level < 100 ) {
            snprintf( buf, MIL,
                      "\r\n\r\nPlease congratulate our newest &rSix Dragons &WStaff member %s!",
                      victim->name );
            echo_to_all( AT_WHITE, buf, ECHOTAR_ALL );
        }
        victim->perm_str = 25;
        victim->perm_int = 25;
        victim->perm_wis = 25;
        victim->perm_dex = 25;
        victim->perm_con = 25;
        victim->perm_cha = 25;
        victim->perm_lck = 25;

        for ( iLevel = 0; iLevel < top_sn; iLevel++ )
            victim->pcdata->learned[iLevel] = 100;
        send_to_char( "You know all available spells/skills/tongues/weapons now.\r\n", victim );
        victim->max_hit = 30000;
        victim->hit = ch->max_hit;
        victim->max_move = 30000;
        victim->move = ch->max_move;
        victim->max_mana = 30000;
        victim->mana = ch->max_mana;
        obj = create_object( get_obj_index( OBJ_VNUM_BOOK ), 0 );
        obj->level = victim->level;
        obj = obj_to_char( obj, victim );

        interpret( victim, ( char * ) "listen all" );
        interpret( victim, ( char * ) "holylight" );
        interpret( victim, ( char * ) "shadowform" );
        xSET_BIT( victim->act, PLR_STAFF );
        send_to_char( "You have been set to config +staff for area info.\r\n", victim );
        restore_char( victim );
        interpret( victim, ( char * ) "speak all" );
        interpret( victim, ( char * ) "title the New &r6 Dragons&D Staff Member." );
        interpret( victim, ( char * ) "save" );
#ifdef I3
        victim->pcdata->i3chardata->i3perm = I3PERM_IMM;
        interpret( victim, ( char * ) "i3listen dchat" );
#endif
        make_stafflist(  );
        interpret( victim, ( char * ) "sooc Greetings everybody!" );
        send_to_char( "\r\n&PSooc&C is the main channel STAFF speak on.\r\n\r\n", victim );
        send_to_char( "\r\n&YPlease set your email address with the email command.\r\n\r\n",
                      victim );
        send_to_char
            ( "\r\n&CYou now have access to ichat channel, and can talk to admin on other muds with it.\r\n",
              victim );
        send_to_char
            ( "\r\n&RYou now have a &Y6 Dragons - How to be a efficient STAFF member book&R in your hands.\r\n",
              victim );
        return;
    }
    return;
}

extern bool             doubleexp;

void do_reward( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MSL];
    char                    arg2[MSL];

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( !*arg ) {
        send_to_char( "Reward: <victim>\r\n", ch );
        if ( ch->level > 107 ) {
            send_to_char( "Reward: <victim> bonus\r\n", ch );
        }
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        bug( "%s", "Reward: Target not in the room" );
        return;
    }

    if ( IS_NPC( victim ) )
        return;

    if ( !str_cmp( arg2, "bonus" ) && ch->level > 107 ) {
        xSET_BIT( victim->act, PLR_R54 );
        send_to_char( "\r\n&CYou reward them for their hard work.\r\n", ch );
        send_to_char
            ( "\r\n&YYou have been rewarded for your hard work on 6 Dragons with a extra experience hour.. Thank You!\r\n",
              victim );
        return;
    }
    victim->pcdata->getsdoubleexp = TRUE;
    victim->pcdata->last_dexpupdate = current_time;
    send_to_char( "\r\n&CYou reward them for their hard work.\r\n", ch );
    send_to_char
        ( "\r\n&YYou have been rewarded for your hard work on 6 Dragons with a double experience hour.. Thank You!\r\n",
          victim );
}

void do_heavens_blessing( CHAR_DATA *ch, char *argument )
{
    int                     sn = gsn_heavens_blessing;
    long int                time_passed;
    int                     hour,
                            minute;
    AFFECT_DATA             af;
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    char                    arg[MIL];

    argument = one_argument( argument, arg );
    if ( !ch || IS_NPC( ch ) )
        return;
    if ( !VLD_STR( arg ) ) {
        set_char_color( AT_IMMORT, ch );
        send_to_char( "Syntax: Heavens < all, last >\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "last" ) ) {
        set_char_color( AT_IMMORT, ch );
        if ( !last_heaven_all_time )
            ch_printf( ch, "There has been no heavens blessing since reboot.\r\n" );
        else {
            time_passed = current_time - last_heaven_all_time;
            hour = ( int ) ( time_passed / 3600 );
            minute = ( int ) ( ( time_passed - ( hour * 3600 ) ) / 60 );
            ch_printf( ch, "The last heavens blessing was %d hours and %d minutes ago.\r\n", hour,
                       minute );
        }
        if ( !ch->pcdata )
            return;
        if ( !ch->pcdata->heaven_time ) {
            send_to_char( "You have never done a heavens blessing.\r\n", ch );
            return;
        }
        time_passed = current_time - ch->pcdata->heaven_time;
        hour = ( int ) ( time_passed / 3600 );
        minute = ( int ) ( ( time_passed - ( hour * 3600 ) ) / 60 );
        ch_printf( ch, "Your last heavens blessing was %d hours and %d minutes ago.\r\n", hour,
                   minute );
        return;
    }
    if ( !str_cmp( arg, "all" ) ) {
        set_char_color( AT_IMMORT, ch );
        if ( !ch->pcdata )
            return;
        /*
         * Check if the player did a restore all within the last 18 hours. 
         */
        if ( ch->level < ( MAX_LEVEL - 1 )
             && ( ( current_time - last_heaven_all_time ) < RESTORE_INTERVAL
                  || ( current_time - ch->pcdata->heaven_time ) < RESTORE_INTERVAL ) ) {
            send_to_char( "Sorry, you can't do a heavens blessing this soon again.\r\n", ch );
            return;
        }
        if ( doubleexp && ch->level < ( MAX_LEVEL - 1 ) ) {
            send_to_char
                ( "Sorry, you can't do a heavens blessing while double exp hour is going on.\r\n",
                  ch );
            return;
        }
        act( AT_YELLOW, "You have sent Heavens Blessing throughout the realms!", ch, NULL, NULL,
             TO_CHAR );
        last_heaven_all_time = current_time;
        ch->pcdata->heaven_time = current_time;
        save_char_obj( ch );
        for ( vch = first_char; vch; vch = vch_next ) {

            vch_next = vch->next;

            if ( IS_IMMORTAL( vch ) )                  // Stop immortals from getting the 
                                                       // 
                // 
                // 
                continue;
            if ( !IS_NPC( vch ) ) {
                if ( !IS_AFFECTED( vch, AFF_HEAVENS_BLESS ) ) {
                    if ( vch->race == RACE_OGRE || vch->race == RACE_ORC || vch->race == RACE_TROLL
                         || vch->race == RACE_DROW || vch->race == RACE_GOBLIN ) {
                        act( AT_RED,
                             "You feel a itch on your soul as an Unholy Blessing fills your being!",
                             vch, NULL, NULL, TO_CHAR );
                    }
                    if ( vch->race == RACE_HUMAN || vch->race == RACE_ELF
                         || vch->race == RACE_DWARF || vch->race == RACE_HALFLING
                         || vch->race == RACE_PIXIE || vch->race == RACE_GNOME
                         || vch->race == RACE_DRAGON || vch->race == RACE_SHADE ) {
                        act( AT_YELLOW, "You feel warm as Heavens Blessing fills your being!", vch,
                             NULL, NULL, TO_CHAR );
                    }
                    restore_char( vch );
                    af.type = sn;
                    af.level = ch->level;
                    af.duration = ch->level * 10;
                    af.location = APPLY_AFFECT;
                    af.modifier = 0;
                    af.bitvector = meb( AFF_SANCTUARY );
                    affect_to_char( vch, &af );
                    af.level = ch->level;
                    af.type = sn;
                    af.duration = ch->level * 10;
                    af.location = APPLY_DAMROLL;
                    af.modifier = 20;
                    af.bitvector = meb( AFF_HEAVENS_BLESS );
                    affect_to_char( vch, &af );
                    af.level = ch->level;
                    af.type = sn;
                    af.duration = ch->level * 10;
                    af.location = APPLY_HITROLL;
                    af.modifier = 20;
                    af.bitvector = meb( AFF_HEAVENS_BLESS );
                    affect_to_char( vch, &af );
                    af.level = ch->level;
                    af.type = sn;
                    af.duration = ch->level * 10;
                    af.location = APPLY_AC;
                    af.modifier = -100;
                    af.bitvector = meb( AFF_HEAVENS_BLESS );
                    affect_to_char( vch, &af );
                }
            }
        }
    }
    return;
}

//Status: Nearly complete.. -Taon
//Information: A mild from of punishment. A PR tool really.

void do_zot( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MSL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "You must provide a target.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "But they're not here.\r\n", ch );
        return;
    }
    if ( ch->level <= victim->level ) {
        send_to_char( "You cannot zot them, they're above your level.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You can't use this on mobs.\r\n", ch );
        return;
    }

    log_printf( "%s zots %s.", ch->name, victim->name );

    if ( ch->fighting )
        WAIT_STATE( ch, 8 * PULSE_VIOLENCE + 1 );
    else
        WAIT_STATE( ch, 60 );

    ch_printf( ch, "You invoke a surge of lightning to zot %s.\r\n", victim->name );
    ch_printf( victim, "%s invokes a surge of lightning to zot the life out of you.\r\n",
               ch->name );
    send_to_char( "You must have done something wrong!!!\r\n", victim );
    victim->hit = 1;
    victim->mana = 1;
    victim->move = 1;
    return;
}

void do_lair( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    int                     lair;
    char                    arg[MSL];
    char                    arg2[MIL];

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( !VLD_STR( arg ) ) {
        send_to_char( "Syntax: Lair <victim> <vnum>\r\n", ch );
        return;
    }
    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) )
        return;
    if ( !str_cmp( arg2, "clear" ) ) {
        victim->pcdata->lair = 0;
        save_char_obj( victim );
        send_to_char( "Lair vnum # cleared.\r\n", ch );
        return;
    }
    if ( !is_number( arg2 ) ) {
        send_to_char( "You must enter numeric data.\r\n", ch );
        return;
    }
    lair = atoi( arg2 );
    if ( lair < 40168 ) {
        send_to_char( "Valid range is greater than 40168.\r\n", ch );
        return;
    }
    victim->pcdata->lair = lair;
    save_char_obj( victim );
    send_to_char( "Lair vnum # set.\r\n", ch );
    return;
}

void                    mobile_update( void );
void                    time_update( void );           /* FB */
void                    char_update( void );
void                    obj_update( void );
void                    aggr_update( void );
void                    auction_update( void );
void                    violence_update( void );
void                    UpdateWeather( void );

void do_ftick( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_STRING_LENGTH];

    one_argument( argument, arg );
    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "Syntax: ftick weather|char|obj|area|mob|violence|aggr|auction|all\r\n", ch );
        return;
    }
    if ( !str_prefix( arg, "area" ) )
        area_update(  );
    else if ( !str_prefix( arg, "mob" ) )
        mobile_update(  );
    else if ( !str_prefix( arg, "char" ) )
        char_update(  );
    else if ( !str_prefix( arg, "obj" ) )
        obj_update(  );
    else if ( !str_prefix( arg, "violence" ) )
        violence_update(  );
    else if ( !str_prefix( arg, "aggr" ) )
        aggr_update(  );
    else if ( !str_prefix( arg, "auction" ) )
        auction_update(  );
    else if ( !str_prefix( arg, "weather" ) )
        UpdateWeather(  );
    else if ( !str_prefix( arg, "all" ) ) {
        char_update(  );
        obj_update(  );
        area_update(  );
        mobile_update(  );
        violence_update(  );
        aggr_update(  );
        auction_update(  );
        UpdateWeather(  );
    }
    else {
        do_ftick( ch, ( char * ) "" );
        return;
    }
    send_to_char( "Tick!\r\n", ch );
}

void do_apurge( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MSL];
    DESCRIPTOR_DATA        *d;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
        victim = ch;
    else
        victim = get_char_world( ch, arg );

    if ( !str_cmp( arg, "all" ) ) {
        for ( d = first_descriptor; d != NULL; d = d->next ) {
            victim = d->character;

            if ( victim == NULL || IS_NPC( victim ) )
                continue;

            while ( victim->first_affect )
                affect_remove( victim, victim->first_affect );
            affect_strip( victim, gsn_maim );
            affect_strip( victim, gsn_brittle_bone );
            affect_strip( victim, gsn_festering_wound );
            affect_strip( victim, gsn_poison );
            affect_strip( victim, gsn_thaitin );
            affect_strip( victim, gsn_rotten_gut );    // Aurin
            xREMOVE_BIT( victim->affected_by, AFF_BRITTLE_BONES );
            xREMOVE_BIT( victim->affected_by, AFF_FUNGAL_TOXIN );
            xREMOVE_BIT( victim->affected_by, AFF_MAIM );
            xREMOVE_BIT( victim->affected_by, AFF_THAITIN );
            xREMOVE_BIT( victim->affected_by, AFF_ROTTEN_GUT ); // Aurin

        }

        send_to_char( "You have removed everyones spell affects.\r\n", ch );
        return;
    }

    while ( victim->first_affect )
        affect_remove( victim, victim->first_affect );

    affect_strip( victim, gsn_maim );
    affect_strip( victim, gsn_brittle_bone );
    affect_strip( victim, gsn_festering_wound );
    affect_strip( victim, gsn_poison );
    affect_strip( victim, gsn_thaitin );
    affect_strip( victim, gsn_rotten_gut );            // Aurin
    xREMOVE_BIT( victim->affected_by, AFF_BRITTLE_BONES );
    xREMOVE_BIT( victim->affected_by, AFF_FUNGAL_TOXIN );
    xREMOVE_BIT( victim->affected_by, AFF_MAIM );
    xREMOVE_BIT( victim->affected_by, AFF_THAITIN );
    xREMOVE_BIT( victim->affected_by, AFF_ROTTEN_GUT ); // Aurin

    if ( victim != ch ) {
        act( AT_RED, "All affects stripped from $N.", ch, 0, victim, TO_CHAR );
        send_to_char( "All of your spell affects have been removed!\r\n", victim );
    }
    else
        send_to_char( "All affects stripped from yourself.\r\n", ch );

    return;
}

char                   *itoa( int foo )
{
    static char             bar[256];

    snprintf( bar, 256, "%d", foo );
    return ( bar );
}

/* Walk-To-Link Snippet
 * Toggle the "building" flag.
 */
void do_buildwalk( CHAR_DATA *ch, char *argument )
{
    // Ignore people who can't use it.
    if ( !IS_IMMORTAL( ch ) || ch->pcdata == NULL ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    // Toggle the building flag.

    // Remove the flag if you have it.
    if ( IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
        REMOVE_BIT( ch->pcdata->flags, PCFLAG_BUILDWALK );

        send_to_char( "&GYou are no longer buildwalking.\r\n", ch );
        act( AT_GREEN, "&G$n is no longer buildwalking.", ch, NULL, NULL, TO_CANSEE );
    }
    // Set the flag otherwise.
    else {
        SET_BIT( ch->pcdata->flags, PCFLAG_BUILDWALK );

        send_to_char( "&GYou are now buildwalking.\r\n", ch );
        act( AT_GREY, "&G$n is now buildwalking.", ch, NULL, NULL, TO_CANSEE );
        return;
    }
}

/* rgrid command by Dracones */

#define MAX_RGRID_ROOMS 6000

void do_rgrid( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    arg3[MAX_INPUT_LENGTH];

    EXIT_DATA              *xit;
    char                    buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA        *location,
                           *ch_location,
                           *tmp;
    AREA_DATA              *pArea;
    int                     vnum,
                            maxnum,
                            x,
                            y,
                            z;
    int                     room_count;
    int                     room_hold[MAX_RGRID_ROOMS];

    set_char_color( AT_PLAIN, ch );

    ch_location = ch->in_room;
    pArea = ch->pcdata->area;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "Create a block of rooms.\r\n", ch );
        send_to_char( "Usage: rgrid <x> length <y> width <z> depth\r\n", ch );
        return;
    }
    x = atoi( arg );
    y = atoi( arg2 );
    z = atoi( arg3 );

    if ( x < 1 || y < 1 ) {

        send_to_char( "You must specify an x and y of at least 1.\r\n", ch );
        return;
    }
    if ( z < 1 )
        z = 1;

    maxnum = x * y * z;

    sprintf( buf, "Attempting to create a block of %d rooms, %d x %d x %d.\r\n", maxnum, x, y, z );
    send_to_char( buf, ch );

    if ( maxnum > MAX_RGRID_ROOMS ) {

        sprintf( buf, "The maximum number of rooms this mud can create at once is %d.\r\n",
                 MAX_RGRID_ROOMS );
        send_to_char( buf, ch );
        send_to_char( "Please try to create a smaller block of rooms.\r\n", ch );
        return;
    }

    room_count = 0;
    send_to_char( "Checking for available rooms...\r\n", ch );

    if ( pArea->low_r_vnum + maxnum > pArea->hi_r_vnum ) {

        send_to_char( "You don't even have that many rooms assigned to you.\r\n", ch );
        return;
    }

    for ( vnum = pArea->low_r_vnum; vnum <= pArea->hi_r_vnum; vnum++ ) {

        if ( get_room_index( vnum ) == NULL )
            room_count++;

        if ( room_count >= maxnum )
            break;
    }

    if ( room_count < maxnum ) {
        send_to_char( "There aren't enough free rooms in your assigned range!\r\n", ch );
        return;
    }

    send_to_char( "Enough free rooms were found, creating the rooms...\r\n", ch );

    room_count = 0;
    vnum = pArea->low_r_vnum;

    while ( room_count < maxnum ) {

        if ( get_room_index( vnum ) == NULL ) {

            room_hold[room_count++] = vnum;
            location = make_room( vnum, ch->pcdata->area );
            if ( !location ) {
                bug( "rgrid: make_room failed" );
                return;
            }

            location->area = ch->pcdata->area;
            /*
             * location->name = ch_location->name; location->description = ch_location->description; 
             */
            location->sector_type = ch_location->sector_type;
            location->room_flags = ch_location->room_flags;

            /*
             * Below here you may add anything else you wish to be
             * copied into the rgrided rooms.
             *
             * NiteDesc is specific my mud, do not add if not needed -- Drac
             */

        }
        vnum++;
    }

    send_to_char( "Rooms created, linking the exits...\r\n", ch );

    for ( room_count = 1; room_count <= maxnum; room_count++ ) {

        vnum = room_hold[room_count - 1];

        // Check to see if we can make N exits
        if ( room_count % x ) {

            location = get_room_index( vnum );
            tmp = get_room_index( room_hold[room_count] );

            xit = make_exit( location, tmp, 0 );
            xit->keyword = STRALLOC( "" );
            xit->description = STRALLOC( "" );
            xit->key = -1;
            xit->exit_info = 0;

        }

        // Check to see if we can make S exits
        if ( ( room_count - 1 ) % x ) {

            location = get_room_index( vnum );
            tmp = get_room_index( room_hold[room_count - 2] );

            xit = make_exit( location, tmp, 2 );
            xit->keyword = STRALLOC( "" );
            xit->description = STRALLOC( "" );
            xit->key = -1;
            xit->exit_info = 0;

        }

        // Check to see if we can make E exits
        if ( ( room_count - 1 ) % ( x * y ) < x * y - x ) {

            location = get_room_index( vnum );
            tmp = get_room_index( room_hold[room_count + x - 1] );

            xit = make_exit( location, tmp, 1 );
            xit->keyword = STRALLOC( "" );
            xit->description = STRALLOC( "" );
            xit->key = -1;
            xit->exit_info = 0;

        }

        // Check to see if we can make W exits
        if ( ( room_count - 1 ) % ( x * y ) >= x ) {

            location = get_room_index( vnum );
            tmp = get_room_index( room_hold[room_count - x - 1] );

            xit = make_exit( location, tmp, 3 );
            xit->keyword = STRALLOC( "" );
            xit->description = STRALLOC( "" );
            xit->key = -1;
            xit->exit_info = 0;

        }

        // Check to see if we can make D exits
        if ( room_count > x * y ) {

            location = get_room_index( vnum );
            tmp = get_room_index( room_hold[room_count - x * y - 1] );

            xit = make_exit( location, tmp, 5 );
            xit->keyword = STRALLOC( "" );
            xit->description = STRALLOC( "" );
            xit->key = -1;
            xit->exit_info = 0;

        }

        // Check to see if we can make U exits
        if ( room_count <= maxnum - ( x * y ) ) {

            location = get_room_index( vnum );
            tmp = get_room_index( room_hold[room_count + x * y - 1] );

            xit = make_exit( location, tmp, 4 );
            xit->keyword = STRALLOC( "" );
            xit->description = STRALLOC( "" );
            xit->key = -1;
            xit->exit_info = 0;

        }

    }

    return;
}

 /*
  * "Claim" an object.  Will allow an immortal to "grab" an object no matter
  * where it is hiding.  ie: from a player's inventory, from deep inside
  * a container, from a mobile, from anywhere.                 -Thoric
  */
void do_oclaim( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];

    char                    arg3[MAX_INPUT_LENGTH];
    char                   *who = NULL;
    CHAR_DATA              *vch = NULL;
    OBJ_DATA               *obj;
    bool                    silently = FALSE,
        found = FALSE;
    int                     number,
                            count,
                            vnum;

    number = number_argument( argument, arg );
    argument = arg;
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "&cSyntax: oclaim <&Cobject&c> [&Cfrom who&c] [&C+silent&c]\r\n", ch );
        return;
    }
    if ( arg3[0] == '\0' ) {
        if ( arg2[0] != '\0' ) {
            if ( !str_cmp( arg2, "+silent" ) )
                silently = TRUE;
            else
                who = arg2;
        }
    }
    else {
        who = arg2;
        if ( !str_cmp( arg3, "+silent" ) )
            silently = TRUE;
    }

    if ( who ) {
        if ( ( vch = get_char_world( ch, who ) ) == NULL ) {
            send_to_pager( "They aren't here.\r\n", ch );
            return;
        }
        if ( get_trust( ch ) < get_trust( vch ) && !IS_NPC( vch ) ) {
            act( AT_TELL, "$n tells you, 'Keep your hands to yourself!'", vch, NULL, ch, TO_VICT );
            return;
        }
    }

    if ( is_number( arg1 ) )
        vnum = atoi( arg1 );
    else
        vnum = -1;
    count = 0;
    for ( obj = first_object; obj; obj = obj->next ) {
        if ( can_see_obj( ch, obj )
             && ( obj->pIndexData->vnum == vnum || nifty_is_name( arg1, obj->name ) ) && ( !vch
                                                                                           || vch ==
                                                                                           carried_by
                                                                                           ( obj ) ) )
            if ( ( count += obj->count ) = number ) {
                found = TRUE;
                break;
            }
    }
    if ( !found && vnum != -1 ) {
        send_to_char( "You can't find that.\r\n", ch );
        return;
    }

    count = 0;
    for ( obj = first_object; obj; obj = obj->next ) {
        if ( can_see_obj( ch, obj )
             && ( obj->pIndexData->vnum == vnum || nifty_is_name_prefix( arg1, obj->name ) )
             && ( !vch || vch == carried_by( obj ) ) )
            if ( ( count += obj->count ) = number ) {
                found = TRUE;
                break;
            }
    }

    if ( !found ) {
        send_to_char( "You can't find that.\r\n", ch );
        return;
    }

    if ( !vch && ( vch = carried_by( obj ) ) != NULL ) {
        if ( get_trust( ch ) < get_trust( vch ) && !IS_NPC( vch ) ) {
            act( AT_TELL, "$n tells you, 'Keep your hands off $p!  It's mine.'", vch, obj, ch,
                 TO_VICT );
            act( AT_IMMORT, "$n tried to lay claim to $p from your possession!", vch, obj, ch,
                 TO_CHAR );
            return;
        }
    }

    separate_obj( obj );
    if ( obj->item_type == ITEM_PORTAL )
        remove_portal( obj );

    if ( obj->carried_by )
        obj_from_char( obj );
    else if ( obj->in_room )
        obj_from_room( obj );
    else if ( obj->in_obj )
        obj_from_obj( obj );

    obj_to_char( obj, ch );
    if ( vch ) {
        if ( !silently ) {
            act( AT_IMMORT, "$n claims $p from you!", ch, obj, vch, TO_VICT );
            act( AT_IMMORT, "$n claims $p from $N!", ch, obj, vch, TO_NOTVICT );
            act( AT_IMMORT, "You claim $p from $N!", ch, obj, vch, TO_CHAR );
        }
        else
            act( AT_IMMORT, "You silently claim $p from $N.", ch, obj, vch, TO_CHAR );
    }
    else {
        if ( !silently ) {
            /*
             * notify people in the room... (not done yet)
             */
            act( AT_IMMORT, "You claim $p!", ch, obj, NULL, TO_CHAR );
        }
        else
            act( AT_IMMORT, "You silently claim $p.", ch, obj, NULL, TO_CHAR );
    }
}

void do_prefresh( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Reload who?\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, arg ) ) ) {
        send_to_char( "They are not here.\r\n", ch );
        return;
    }

    if ( get_trust( victim ) > get_trust( ch ) ) {
        send_to_char( "You can't reload someone who's a higher level!\r\n", ch );
        return;
    }

    if ( exists_player( victim->name ) ) {
        DESCRIPTOR_DATA        *d;
        char                    name[MAX_STRING_LENGTH];
        char                    buf[MAX_STRING_LENGTH];
        ROOM_INDEX_DATA        *in_room;
        bool                    Load;

        d = NULL;
        d = victim->desc;

        sprintf( name, "%s", victim->name );
        in_room = victim->in_room;
        /*
         * clear descriptor pointer to get rid of bug message in log
         */
        victim->desc = NULL;
        extract_char( victim, TRUE );
        d->character = NULL;
        Load = load_char_obj( d, name, FALSE, FALSE, TRUE );
        victim = d->character;
        victim->desc = d;
        victim->timer = 0;
        /*
         * Insert in the char_list
         */
        LINK( d->character, first_char, last_char, next, prev );
        char_to_room( victim, in_room );

        if ( Load ) {
            send_to_char( "Your pfile has been reloaded.\r\n", victim );
            send_to_char( "Their pfile has been reloaded.\r\n", ch );
            sprintf( buf, "%s has been reloaded.", victim->name );
            log_string( buf );
        }
        else {
            send_to_char( "Hrmm bug, it didnt work.\r\n", ch );
            return;
        }
    }

    else
        send_to_char( "They have to have a pfile before you can load it.\r\n", ch );

    return;
}
