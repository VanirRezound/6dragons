 /****************************************************************************
 * AFKMud (c)1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,          *
 * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                      New Name Authorization module                       *
 ****************************************************************************/

/*
 *  New name authorization system
 *  Author: Rantic (supfly@geocities.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */

#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/hometowns.h"
#include "h/new_auth.h"
#include "h/key.h"

/* from comm.c, for do_name */
bool check_parse_name   args( ( char *name, bool newchar ) );

/* from act_wiz.c, for do_authorize */
CHAR_DATA              *get_waiting_desc args( ( CHAR_DATA *ch, char *name ) );
void                    to_channel( const char *argument, const char *xchannel, int level );

AUTH_LIST              *first_auth_name;
AUTH_LIST              *last_auth_name;

void do_name_generator( CHAR_DATA *ch, char *argument )
{
    int                     start_counter = 0;
    int                     middle_counter = 0;
    int                     end_counter = 0;
    char                    start_string[100][10];
    char                    middle_string[100][10];
    char                    end_string[100][10];
    char                    tempstring[151];
    struct timeval          starttime;
    time_t                  t;
    char                    name[300];
    FILE                   *infile;
    char                   *fgetsed;

    infile = FileOpen( NAMEGEN_FILE, "r" );
    if ( infile == NULL ) {
        send_to_char( "NameGen File Not Found.\r\n", ch );
        return;
    }

    while ( str_cmp( tempstring, "[start]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
    }

    while ( str_cmp( tempstring, "[middle]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
        if ( tempstring[0] != '/' ) {
            strcpy( start_string[start_counter++], tempstring );
        }
    }
    while ( str_cmp( tempstring, "[end]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
        if ( tempstring[0] != '/' ) {
            strcpy( middle_string[middle_counter++], tempstring );
        }
    }
    while ( str_cmp( tempstring, "[finish]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
        if ( tempstring[0] != '/' ) {
            strcpy( end_string[end_counter++], tempstring );
        }
    }
    FileClose( infile );
    gettimeofday( &starttime, NULL );
    srand( ( unsigned ) time( &t ) + starttime.tv_usec );
    start_counter--;
    middle_counter--;
    end_counter--;

    strcpy( name, start_string[rand(  ) % start_counter] ); /* get a start */
    mudstrlcat( name, middle_string[rand(  ) % middle_counter], 300 );  /* get a middle */
    mudstrlcat( name, end_string[rand(  ) % end_counter], 300 );    /* get an ending */
    mudstrlcat( name, "\r\n", 300 );
    send_to_char( name, ch );                          /* print the name */
    return;
}

void name_generator( char *argument )
{
    int                     start_counter = 0;
    int                     middle_counter = 0;
    int                     end_counter = 0;
    char                    start_string[100][10];
    char                    middle_string[100][10];
    char                    end_string[100][10];
    char                    tempstring[151];
    struct timeval          starttime;
    time_t                  t;
    char                    name[300];
    FILE                   *infile;
    char                   *fgetsed;

    infile = FileOpen( NAMEGEN_FILE, "r" );
    if ( infile == NULL ) {
        log_string( "Can't find NAMEGEN file." );
        return;
    }

    while ( str_cmp( tempstring, "[start]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
    }
    while ( str_cmp( tempstring, "[middle]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
        if ( tempstring[0] != '/' ) {
            strcpy( start_string[start_counter++], tempstring );
        }
    }
    while ( str_cmp( tempstring, "[end]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
        if ( tempstring[0] != '/' ) {
            strcpy( middle_string[middle_counter++], tempstring );
        }
    }
    while ( str_cmp( tempstring, "[finish]" ) != 0 ) {
        fgetsed = fgets( tempstring, 150, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
        if ( tempstring[0] != '/' ) {
            strcpy( end_string[end_counter++], tempstring );
        }
    }
    FileClose( infile );
    gettimeofday( &starttime, NULL );
    srand( ( unsigned ) time( &t ) + starttime.tv_usec );
    start_counter--;
    middle_counter--;
    end_counter--;

    strcpy( name, start_string[rand(  ) % start_counter] ); /* get a start */
    mudstrlcat( name, middle_string[rand(  ) % middle_counter], 300 );  /* get a middle */
    mudstrlcat( name, end_string[rand(  ) % end_counter], 300 );    /* get an ending */
    mudstrlcat( argument, name, 300 );
    return;
}

/* Added by Tarl 5 Dec 02 to allow picking names from a file. Used for the namegen
   code in reset.c */
void pick_name( char *argument, char *filename )
{
    struct timeval          starttime;
    time_t                  t;
    char                    name[100];
    char                    tempstring[100];
    int                     counter = 0;
    FILE                   *infile;
    char                    names[200][20];
    char                   *fgetsed;

    infile = FileOpen( filename, "r" );
    if ( infile == NULL ) {
        log_printf( "Can't find %s", filename );
        return;
    }

    while ( str_cmp( tempstring, "[start]" ) != 0 ) {
        fgetsed = fgets( tempstring, 100, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';   /* remove linefeed */
    }
    while ( str_cmp( tempstring, "[finish]" ) != 0 ) {
        fgetsed = fgets( tempstring, 100, infile );
        tempstring[strlen( tempstring ) - 1] = '\0';
        if ( tempstring[0] != '/' ) {
            strcpy( names[counter++], tempstring );
        }
    }
    FileClose( infile );
    gettimeofday( &starttime, NULL );
    srand( ( unsigned ) time( &t ) + starttime.tv_usec );
    counter--;
    strcpy( name, names[rand(  ) % counter] );
    mudstrlcat( argument, name, MSL );
    return;
}

bool exists_player( char *name )
{
    struct stat             fst;
    char                    buf[MSL];

    /*
     * Stands to reason that if there ain't a name to look at, they damn well don't exist! 
     */
    if ( !name || !str_cmp( name, "" ) )
        return FALSE;

    snprintf( buf, MSL, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );

    if ( stat( buf, &fst ) != -1 )
        return TRUE;
    else
        return FALSE;
}

void write_auth_file( FILE * fpout, AUTH_LIST * list )
{
    fprintf( fpout, "Name  %s~\n", list->name );
    fprintf( fpout, "State  %d\n", list->state );
    if ( list->authed_by )
        fprintf( fpout, "AuthedBy       %s~\n", list->authed_by );
    if ( list->change_by )
        fprintf( fpout, "Change  %s~\n", list->change_by );
    if ( list->denied_by )
        fprintf( fpout, "Denied  %s~\n", list->denied_by );
    fprintf( fpout, "End\n\n" );
}

AUTH_LIST              *fread_auth( FILE * fp )
{
    AUTH_LIST              *new_auth;
    bool                    fMatch;
    const char             *word;

    CREATE( new_auth, AUTH_LIST, 1 );
    new_auth->authed_by = NULL;
    new_auth->change_by = NULL;
    new_auth->denied_by = NULL;
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                KEY( "AuthedBy", new_auth->authed_by, fread_string( fp ) );
                break;
            case 'C':
                KEY( "Change", new_auth->change_by, fread_string( fp ) );
                break;
            case 'D':
                KEY( "Denied", new_auth->denied_by, fread_string( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    fMatch = TRUE;
                    LINK( new_auth, first_auth_name, last_auth_name, next, prev );
                    return NULL;
                }
                break;
            case 'N':
                KEY( "Name", new_auth->name, fread_string( fp ) );
                break;
            case 'S':
                if ( !str_cmp( word, "State" ) ) {
                    new_auth->state = fread_number( fp );
                    if ( new_auth->state == AUTH_ONLINE || new_auth->state == AUTH_LINK_DEAD )
                        new_auth->state = AUTH_OFFLINE;
                    fMatch = TRUE;
                    break;
                }
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_auth: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

void save_auth_list( void )
{
    FILE                   *fpout;
    AUTH_LIST              *list;

    if ( ( fpout = FileOpen( AUTH_FILE, "w" ) ) == NULL ) {
        bug( "%s", "Cannot open auth.dat for writing." );
        perror( AUTH_FILE );
        return;
    }
    for ( list = first_auth_name; list; list = list->next ) {
        fprintf( fpout, "#AUTH\n" );
        write_auth_file( fpout, list );
    }
    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

void clear_auth_list( void )
{
    AUTH_LIST              *auth,
                           *nauth;

    for ( auth = first_auth_name; auth; auth = nauth ) {
        nauth = auth->next;
        if ( !exists_player( auth->name ) ) {
            UNLINK( auth, first_auth_name, last_auth_name, next, prev );
            if ( auth->authed_by )
                STRFREE( auth->authed_by );
            if ( auth->change_by )
                STRFREE( auth->change_by );
            if ( auth->denied_by )
                STRFREE( auth->denied_by );
            STRFREE( auth->name );
            DISPOSE( auth );
        }
    }
    save_auth_list(  );
}

void load_auth_list( void )
{
    FILE                   *fp;
    int                     x;

    first_auth_name = last_auth_name = NULL;
    if ( ( fp = FileOpen( AUTH_FILE, "r" ) ) != NULL ) {
        x = 0;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }
            if ( letter != '#' ) {
                bug( "%s", "Load_auth_list: # not found." );
                break;
            }
            word = fread_word( fp );
            if ( !str_cmp( word, "AUTH" ) ) {
                fread_auth( fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "load_auth_list: bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        bug( "Cannot open %s", AUTH_FILE );
        exit( 0 );
    }
    clear_auth_list(  );
}

int get_auth_state( CHAR_DATA *ch )
{
    AUTH_LIST              *namestate;
    int                     state;

    state = AUTH_AUTHED;

    for ( namestate = first_auth_name; namestate; namestate = namestate->next ) {
        if ( !str_cmp( namestate->name, ch->name ) ) {
            state = namestate->state;
            break;
        }
    }
    return state;
}

AUTH_LIST              *get_auth_name( char *name )
{
    AUTH_LIST              *mname;

    if ( last_auth_name && last_auth_name->next != NULL )
        bug( "Last_auth_name->next != NULL: %s", last_auth_name->next->name );

    for ( mname = first_auth_name; mname; mname = mname->next ) {
        if ( !str_cmp( mname->name, name ) )           /* If the name is already in the
                                                        * list, break */
            break;
    }
    return mname;
}

void add_to_auth( CHAR_DATA *ch )
{
    AUTH_LIST              *new_name;

    new_name = get_auth_name( ch->name );
    if ( new_name != NULL )
        return;
    else {
        CREATE( new_name, AUTH_LIST, 1 );
        new_name->name = STRALLOC( ch->name );
        new_name->state = AUTH_ONLINE;                 /* Just entered the game */
        LINK( new_name, first_auth_name, last_auth_name, next, prev );
        save_auth_list(  );
    }
}

void remove_from_auth( char *name )
{
    AUTH_LIST              *old_name;

    old_name = get_auth_name( name );
    if ( old_name == NULL )                            /* Its not old */
        return;
    else {
        UNLINK( old_name, first_auth_name, last_auth_name, next, prev );
        if ( old_name->authed_by )
            STRFREE( old_name->authed_by );
        if ( old_name->change_by )
            STRFREE( old_name->change_by );
        if ( old_name->denied_by )
            STRFREE( old_name->denied_by );
        STRFREE( old_name->name );
        DISPOSE( old_name );
        save_auth_list(  );
    }
}

void check_auth_state( CHAR_DATA *ch )
{
    AUTH_LIST              *old_auth;
    CMDTYPE                *command;
    int                     level = LEVEL_IMMORTAL;
    char                    buf[MSL];
    char                   *name;
    int                     x,
                            y;

    command = find_command( "authorize" );
    if ( !command )
        level = LEVEL_IMMORTAL;
    else
        level = command->level;

    old_auth = get_auth_name( ch->name );
    if ( old_auth == NULL )
        return;

    if ( old_auth->state == AUTH_OFFLINE               /* checking as they enter the game 
                                                        */
         || old_auth->state == AUTH_LINK_DEAD ) {
        old_auth->state = AUTH_ONLINE;
        save_auth_list(  );
    }
    else if ( old_auth->state == AUTH_CHANGE_NAME ) {
        ch_printf( ch,
                   "&R\r\nThe MUD Administrators have found the name %s\r\n"
                   "to be unacceptable. You must choose a new one.\r\n"
                   "The name you choose must be medieval and original.\r\n"
                   "No titles, descriptive words, or names close to any existing\r\n"
                   "Staff member's name. See 'help name'.\r\n", ch->name );
    }
    else if ( old_auth->state == AUTH_DENIED ) {
        set_char_color( AT_RED, ch );
        send_to_char( "You have been denied access to the game.\r\n", ch );
        remove_from_auth( ch->name );

        quitting_char = ch;
        save_char_obj( ch );
        saving_char = NULL;
        extract_char( ch, TRUE );
        for ( x = 0; x < MAX_WEAR; x++ )
            for ( y = 0; y < MAX_LAYERS; y++ )
                save_equipment[x][y] = NULL;

        name = capitalize( ch->name );
        snprintf( buf, MSL, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), name );
        if ( !remove( buf ) ) {
            snprintf( buf, MSL, "Pre-Auth %s denied. Player file destroyed.\r\n", name );
            to_channel( buf, "Auth", level );
        }
    }
    else if ( old_auth->state == AUTH_AUTHED ) {
        if ( ch->pcdata->authed_by )
            STRFREE( ch->pcdata->authed_by );
        if ( old_auth->authed_by ) {
            ch->pcdata->authed_by = QUICKLINK( old_auth->authed_by );
            STRFREE( old_auth->authed_by );
        }
        else
            ch->pcdata->authed_by = STRALLOC( "The Code" );

        ch_printf( ch,
                   "\r\n&GThe MUD Administrators have accepted the name %s.\r\n"
                   "You are now free to roam the %s.\r\n", ch->name, sysdata.mud_name );
        REMOVE_BIT( ch->pcdata->flags, PCFLAG_UNAUTHED );
        remove_from_auth( ch->name );
        return;
    }
    return;
}

/* new auth */
void do_authorize( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    buf[MSL];
    CHAR_DATA              *victim = NULL;
    AUTH_LIST              *auth;
    CMDTYPE                *command;
    int                     level = LEVEL_IMMORTAL;
    bool                    offline,
                            authed,
                            changename,
                            denied,
                            pending;
    char                   *name;
    int                     x,
                            y;

    offline = authed = changename = denied = pending = FALSE;
    auth = NULL;

    /*
     * Checks level of authorize command, for log messages. - Samson 10-18-98 
     */
    command = find_command( "authorize" );
    if ( !command )
        level = LEVEL_IMMORTAL;
    else
        level = command->level;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !arg1 || arg1[0] == '\0' ) {
        send_to_char( "To approve a waiting character: auth <name>\r\n", ch );
        send_to_char( "To deny a waiting character:    auth <name> no\r\n", ch );
        send_to_char( "To ask a waiting character to change names: auth <name> name\r\n", ch );
        send_to_char( "To have the code verify the list: auth fixlist\r\n", ch );

        set_char_color( AT_DIVIDER, ch );
        send_to_char( "\r\n--- Characters awaiting approval ---\r\n", ch );

        for ( auth = first_auth_name; auth; auth = auth->next ) {
            if ( auth->state == AUTH_CHANGE_NAME )
                changename = TRUE;
            else if ( auth->state == AUTH_AUTHED )
                authed = TRUE;
            else if ( auth->state == AUTH_DENIED )
                denied = TRUE;

            if ( auth->name != NULL && auth->state < AUTH_CHANGE_NAME )
                pending = TRUE;
        }
        if ( pending ) {
            for ( auth = first_auth_name; auth; auth = auth->next ) {
                if ( auth->state < AUTH_CHANGE_NAME ) {
                    switch ( auth->state ) {
                        default:
                            mudstrlcpy( buf, "Unknown?", MSL );
                            break;
                        case AUTH_LINK_DEAD:
                            mudstrlcpy( buf, "Link Dead", MSL );
                            break;
                        case AUTH_ONLINE:
                            mudstrlcpy( buf, "Online", MSL );
                            break;
                        case AUTH_OFFLINE:
                            mudstrlcpy( buf, "Offline", MSL );
                            break;
                    }

                    ch_printf( ch, "%20s %-10s\r\n", auth->name, buf );
                }
            }
        }
        else
            send_to_char( "None\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "fixlist" ) ) {
        send_to_pager( "Checking authorization list...\r\n", ch );
        clear_auth_list(  );
        send_to_pager( "Done.\r\n", ch );
        return;
    }

    auth = get_auth_name( arg1 );
    if ( auth != NULL ) {
        if ( auth->state == AUTH_OFFLINE || auth->state == AUTH_LINK_DEAD ) {
            offline = TRUE;
            if ( arg2[0] == '\0' || !str_cmp( arg2, "accept" ) || !str_cmp( arg2, "yes" ) ) {
                auth->state = AUTH_AUTHED;
                auth->authed_by = QUICKLINK( ch->name );
                save_auth_list(  );
                snprintf( buf, MSL, "%s: authorized", auth->name );
                to_channel( buf, "Auth", level );
                ch_printf( ch, "You have authorized %s.\r\n", auth->name );
                return;
            }
            else if ( !str_cmp( arg2, "no" ) || !str_cmp( arg2, "deny" ) ) {
                auth->state = AUTH_DENIED;
                auth->denied_by = QUICKLINK( ch->name );
                save_auth_list(  );
                snprintf( buf, MSL, "%s: denied authorization", auth->name );
                to_channel( buf, "Auth", level );
                ch_printf( ch, "You have denied %s.\r\n", auth->name );
                /*
                 * Addition so that denied names get added to reserved list - Samson 10-18-98 
                 */
                snprintf( buf, MSL, "%s add", auth->name );
                do_reserve( ch, buf );                 /* Samson 7-27-98 */
                return;
            }
            else if ( !str_cmp( arg2, "name" ) || !str_cmp( arg2, "n" ) ) {
                auth->state = AUTH_CHANGE_NAME;
                auth->change_by = QUICKLINK( ch->name );
                save_auth_list(  );
                snprintf( buf, MSL, "%s: name denied", auth->name );
                to_channel( buf, "Auth", level );
                ch_printf( ch, "You requested %s change names.\r\n", auth->name );
                /*
                 * Addition so that requested name changes get added to reserved list - Samson 10-18-98 
                 */
                snprintf( buf, MSL, "%s add", auth->name );
                do_reserve( ch, buf );
                return;
            }
            else {
                send_to_char( "Invalid argument.\r\n", ch );
                return;
            }
        }
        else {
            victim = get_waiting_desc( ch, arg1 );
            if ( victim == NULL )
                return;

            set_char_color( AT_IMMORT, victim );
            if ( arg2[0] == '\0' || !str_cmp( arg2, "accept" ) || !str_cmp( arg2, "yes" ) ) {
                if ( victim->pcdata->authed_by )
                    STRFREE( victim->pcdata->authed_by );
                victim->pcdata->authed_by = QUICKLINK( ch->name );
                snprintf( buf, MSL, "%s: authorized", victim->name );
                to_channel( buf, "Auth", level );
                ch_printf( ch, "You have authorized %s.\r\n", victim->name );
                ch_printf( victim,
                           "\r\n&GThe MUD Administrators have accepted the name %s.\r\n"
                           "You are now free to roam the %s.\r\n", victim->name, sysdata.mud_name );
                REMOVE_BIT( victim->pcdata->flags, PCFLAG_UNAUTHED );
                remove_from_auth( victim->name );
                return;
            }
            else if ( !str_cmp( arg2, "no" ) || !str_cmp( arg2, "deny" ) ) {
                send_to_char( "&RYou have been denied access.\r\n", victim );
                snprintf( buf, MSL, "%s: denied authorization", victim->name );
                to_channel( buf, "Auth", level );
                ch_printf( ch, "You have denied %s.\r\n", victim->name );
                remove_from_auth( victim->name );
                /*
                 * Addition to add denied names to reserved list - Samson 10-18-98 
                 */
                snprintf( buf, MSL, "%s add", victim->name );
                do_reserve( ch, buf );
#ifdef SAMSONRENT
                if ( ch->level == 1 )
                    do_quit( ch, ( char * ) "yes" );
#else
                if ( ch->level == 1 )
                    do_quit( ch, ( char * ) "" );
#endif
                else {
                    quitting_char = victim;
                    save_char_obj( victim );
                    saving_char = NULL;
                    extract_char( victim, TRUE );
                    for ( x = 0; x < MAX_WEAR; x++ )
                        for ( y = 0; y < MAX_LAYERS; y++ )
                            save_equipment[x][y] = NULL;

                    name = capitalize( victim->name );
                    snprintf( buf, MSL, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), name );
                    if ( !remove( buf ) ) {
                        snprintf( buf, MSL, "Pre-Auth %s denied. Player file destroyed.\r\n",
                                  name );
                        to_channel( buf, "Auth", level );
                    }
                }
            }
            else if ( !str_cmp( arg2, "name" ) || !str_cmp( arg2, "n" ) ) {
                auth->state = AUTH_CHANGE_NAME;
                auth->change_by = QUICKLINK( ch->name );
                save_auth_list(  );
                snprintf( buf, MSL, "%s: name denied", victim->name );
                to_channel( buf, "Auth", level );
                ch_printf( victim,
                           "&R\r\nThe MUD Administrators have found the name %s to be unacceptable.\r\n"
                           "You may choose a new name when you reach the end of this area.\r\n"
                           "The name you choose must be medieval and original.\r\n"
                           "No titles, descriptive words, or names close to any existing\r\n"
                           "Immortal's name. See 'help name'.\r\n", victim->name );
                ch_printf( ch, "You requested %s change names.\r\n", victim->name );
                /*
                 * Addition to put denied name on reserved list - Samson 10-18-98 
                 */
                snprintf( buf, MSL, "%s add", victim->name );
                do_reserve( ch, buf );
                return;
            }
            else {
                send_to_char( "Invalid argument.\r\n", ch );
                return;
            }
        }
    }
    else {
        send_to_char( "No such player pending authorization.\r\n", ch );
        return;
    }
}

/* new auth */
void do_name( CHAR_DATA *ch, char *argument )
{
    char                    fname[1024];
    struct stat             fst;
    CHAR_DATA              *tmp;
    AUTH_LIST              *auth_name;

    auth_name = NULL;
    auth_name = get_auth_name( ch->name );
    if ( auth_name == NULL ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument[0] = UPPER( argument[0] );

    if ( !check_parse_name( argument, TRUE ) ) {
        send_to_char( "Illegal name, try another.\r\n", ch );
        return;
    }

    if ( !str_cmp( ch->name, argument ) ) {
        send_to_char( "That's already your name!\r\n", ch );
        return;
    }

    for ( tmp = first_char; tmp; tmp = tmp->next ) {
        if ( !str_cmp( argument, tmp->name ) )
            break;
    }

    if ( tmp ) {
        send_to_char( "That name is already taken.  Please choose another.\r\n", ch );
        return;
    }

    snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
    if ( stat( fname, &fst ) != -1 ) {
        send_to_char( "That name is already taken.  Please choose another.\r\n", ch );
        return;
    }
    snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( ch->name[0] ), capitalize( ch->name ) );
    unlink( fname );                                   /* cronel, for auth */

    STRFREE( ch->name );
    ch->name = STRALLOC( argument );
    STRFREE( ch->pcdata->filename );
    ch->pcdata->filename = STRALLOC( argument );
    send_to_char( "Your name has been changed and is being submitted for approval.\r\n", ch );
    log_printf( "%s name has been changed to %s.", auth_name->name, ch->name );
    auth_name->name = STRALLOC( argument );
    auth_name->state = AUTH_ONLINE;
    if ( auth_name->change_by )
        STRFREE( auth_name->change_by );
    save_auth_list(  );
    return;
}

/* changed for new auth */
void do_mpapplyb( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    buf[MSL];
    CMDTYPE                *command;
    int                     level = LEVEL_IMMORTAL;

    /*
     * Checks to see level of authorize command.
     * * Makes no sense to see the auth channel if you can't auth. - Samson 12-28-98 
     */
    command = find_command( "authorize" );
    if ( !command )
        level = LEVEL_IMMORTAL;
    else
        level = command->level;
    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        progbug( ch, "Mpapplyb - bad syntax [%s]", ch->name );
        return;
    }
    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
        progbug( ch, "Mpapplyb - no such player in room. [%s]", ch->name );
        return;
    }
    if ( !victim->desc ) {
        send_to_char( "Not on linkdeads.\r\n", ch );
        return;
    }

    if ( victim->secondclass == -1 && victim->level > 1 ) {
        return;
    }

    if ( victim->secondclass != -1 && victim->level > 1 && victim->secondlevel > 1 ) {
        return;
    }

    if ( victim->fighting )
        stop_fighting( victim, TRUE );
    char_from_room( victim );
    if ( !IS_IMMORTAL( victim ) ) {
        if ( victim->pcdata && victim->pcdata->htown )
            char_to_room( victim, get_room_index( victim->pcdata->htown->startroom ) );
        else
            char_to_room( victim, get_room_index( ROOM_VNUM_TEMPLE ) );
        victim->level = victim->level + 1;
        advance_level( victim );

        if ( victim->secondclass != -1 ) {
            victim->secondlevel = victim->secondlevel + 1;
            advance_class_level( victim );
        }

        victim->hit = victim->max_hit;
        victim->move = victim->max_move;
        victim->mana = victim->max_mana;

        interpret( victim, ( char * ) "listen all" );
        snprintf( buf, MSL, "Please welcome new player %s to the realms of Six Dragons!",
                  victim->name );
        announce( buf );
        act( AT_WHITE, "$n enters this world from within a column of blinding light!", victim, NULL,
             NULL, TO_ROOM );
        do_look( victim, ( char * ) "auto" );
    }
    else {
        char_to_room( victim, get_room_index( victim->pcdata->htown->startroom ) );
        do_look( victim, ( char * ) "auto" );
    }
    if ( NOT_AUTHED( victim ) ) {
        snprintf( buf, MSL, "%s@%s New player entering the game.\r\n", victim->name,
                  victim->desc->host );
        to_channel( buf, "Auth", level );
    }
    if ( ch->pcdata )
        ch->pcdata->textspeed = 5;
    return;
}

/* changed for new auth */
void auth_update( void )
{
    AUTH_LIST              *auth;
    char                    buf[MIL],
                            showbuf[MSL];
    CMDTYPE                *command;
    DESCRIPTOR_DATA        *d;
    int                     level = LEVEL_IMMORTAL;
    bool                    found_imm = FALSE;         /* Is at least 1 immortal on? */
    bool                    found_hit = FALSE;         /* was at least one found? */

    command = find_command( "authorize" );
    if ( !command )
        level = LEVEL_IMMORTAL;
    else
        level = command->level;

    strcpy( showbuf, "--- Characters awaiting approval ---\r\n" );
    for ( auth = first_auth_name; auth; auth = auth->next ) {
        if ( auth != NULL && auth->state < AUTH_CHANGE_NAME ) {
            snprintf( buf, MIL, "Name: %s      Status: %s\r\n", auth->name,
                      ( auth->state == AUTH_ONLINE ) ? "Online" : "Offline" );
            mudstrlcat( showbuf, buf, MSL );
            found_hit = TRUE;
        }
    }
    if ( found_hit ) {
        for ( d = first_descriptor; d; d = d->next )
            if ( d->connected == CON_PLAYING && d->character && IS_IMMORTAL( d->character )
                 && d->character->level >= level )
                found_imm = TRUE;

        if ( found_imm )
            to_channel( showbuf, "Auth", level );
    }
}

void save_reserved      args( ( void ) );
void sort_reserved      args( ( RESERVE_DATA * pRes ) );

/* Modified to require an "add" or "remove" argument in addition to name - Samson 10-18-98 */
void do_reserve( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    RESERVE_DATA           *res;

    set_char_color( AT_PLAIN, ch );

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( !*arg ) {
        int                     wid = 0;

        send_to_char
            ( "To add a name: reserve <name> add\r\nTo remove a name: reserve <name> remove\r\n",
              ch );
        send_to_char( "\r\n-- Reserved Names --\r\n", ch );
        for ( res = first_reserved; res; res = res->next ) {
            ch_printf( ch, "%c%-17s ", ( *res->name == '*' ? '*' : ' ' ),
                       ( *res->name == '*' ? res->name + 1 : res->name ) );
            if ( ++wid % 4 == 0 )
                send_to_char( "\r\n", ch );
        }
        if ( wid % 4 != 0 )
            send_to_char( "\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "remove" ) ) {
        for ( res = first_reserved; res; res = res->next )
            if ( !str_cmp( arg, res->name ) ) {
                UNLINK( res, first_reserved, last_reserved, next, prev );
                STRFREE( res->name );
                DISPOSE( res );
                save_reserved(  );
                send_to_char( "Name no longer reserved.\r\n", ch );
                return;
            }
        ch_printf( ch, "The name %s isn't on the reserved list.\r\n", arg );
        return;

    }

    if ( !str_cmp( arg2, "add" ) ) {
        for ( res = first_reserved; res; res = res->next )
            if ( !str_cmp( arg, res->name ) ) {
                ch_printf( ch, "The name %s has already been reserved.\r\n", arg );
                return;
            }

        CREATE( res, RESERVE_DATA, 1 );
        res->name = STRALLOC( arg );
        sort_reserved( res );
        save_reserved(  );
        send_to_char( "Name reserved.\r\n", ch );
        return;
    }
    send_to_char( "Invalid argument.\r\n", ch );
}
