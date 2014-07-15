  /*
   * CITY POLITICS CODE START - Vladaar 
   */

#include "h/mud.h"
#include "h/files.h"
#include "h/city.h"
#include "h/key.h"
#include <time.h>
#include <string.h>

CITY_DATA              *first_city;
CITY_DATA              *last_city;

bool                    valid_pfile( const char *filename );
void                    fix_city_order( CITY_DATA * city );
void                    fread_city( CITY_DATA * city, FILE * fp );
extern bool             load_city_file( const char *cityfile );
void                    write_city_list( void );
void                    end_siege( CITY_DATA * city, short tend );
extern int              get_npc_class( char *Class );

void insert_rollcall( CITY_DATA * city, ROLLCALL_DATA * rollcall )
{
    ROLLCALL_DATA          *uroll;
    int                     daydiff,
                            odaydiff;

    for ( uroll = city->first_citizen; uroll; uroll = uroll->next ) {
        odaydiff = ( current_time - uroll->lastupdated ) / 86400;
        daydiff = ( current_time - rollcall->lastupdated ) / 86400;

        if ( odaydiff < 3 && uroll->level > rollcall->level )
            continue;
        /*
         * So if level is the same or they havent logged in for 3 days just insert here
         */
        INSERT( rollcall, uroll, city->first_citizen, next, prev );
        return;
    }
    LINK( rollcall, city->first_citizen, city->last_citizen, next, prev );
}

CITY_DATA              *get_city( const char *name )
{
    CITY_DATA              *city;

    if ( !VLD_STR( name ) )
        return NULL;

    for ( city = first_city; city; city = city->next ) {
        if ( !str_cmp( name, city->name ) )
            return city;
    }
    return NULL;
}

void proc_chan( CHAR_DATA *ch, char *argument )
{
    char                    bufx[MIL];
    DESCRIPTOR_DATA        *d;
    CITY_DATA              *city;

    city = get_city( ch->pcdata->city_name );
    if ( !city )
        return;

    snprintf( bufx, MIL,
              "&GA town crier hurriedly reads off a new proclamation shouting Hear ye, Hear ye: %s&D\r\n",
              argument );
    for ( d = first_descriptor; d; d = d->next ) {
        if ( d->connected != CON_PLAYING || !d->character || !d->character->pcdata )
            continue;
        if ( !IS_IMMORTAL( d->character ) ) {
            if ( !VLD_STR( d->character->pcdata->city_name ) )
                continue;
            if ( str_cmp( d->character->pcdata->city_name, city->name ) )
                continue;
        }
        send_to_char( bufx, d->character );
    }
}

void write_city_list( void )
{
    CITY_DATA              *tcity;
    FILE                   *fpout;
    char                    filename[256];

    snprintf( filename, 256, "%s%s", CITY_DIR, CITY_LIST );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "FATAL: cannot open %s for writing!\r\n", filename );
        perror( filename );
        return;
    }
    for ( tcity = first_city; tcity; tcity = tcity->next )
        fprintf( fpout, "%s\n", tcity->filename );
    fprintf( fpout, "$\n" );
    FileClose( fpout );
    return;
}

void fwrite_citizenlist( FILE * fp, CITY_DATA * city )
{
    ROLLCALL_DATA          *rollcall;

    for ( rollcall = city->first_citizen; rollcall; rollcall = rollcall->next ) {
        if ( !valid_pfile( rollcall->name ) )          /* Just don't save if not a valid
                                                        * pfile */
            continue;
        fprintf( fp, "%s", "#ROLLCALL\n" );
        fprintf( fp, "Name      %s~\n", rollcall->name );
        fprintf( fp, "Joined    %ld\n", ( time_t ) rollcall->joined );
        fprintf( fp, "Updated    %ld\n", ( time_t ) rollcall->lastupdated );
        fprintf( fp, "Class     %s~\n", npc_class[rollcall->Class] );
        fprintf( fp, "Level     %d\n", rollcall->level );
        fprintf( fp, "Kills     %d\n", rollcall->kills );
        fprintf( fp, "Deaths    %d\n", rollcall->deaths );
        fprintf( fp, "%s", "End\n\n" );
    }
}

/* Save a city's data to its data file */
void save_city( CITY_DATA * city )
{
    FILE                   *fp;
    char                    filename[256];

    if ( !city ) {
        bug( "%s: null city pointer!", __FUNCTION__ );
        return;
    }
    if ( !VLD_STR( city->filename ) ) {
        bug( "%s: %s has no filename", __FUNCTION__, city->name );
        return;
    }

    snprintf( filename, 256, "%s%s", CITY_DIR, city->filename );
    if ( !( fp = FileOpen( filename, "w" ) ) ) {
        bug( "%s: can't open %s for writing.", __FUNCTION__, filename );
        perror( filename );
        return;
    }

    fprintf( fp, "#CITY\n" );
    if ( VLD_STR( city->name ) )
        fprintf( fp, "Name          %s~\n", city->name );
    if ( VLD_STR( city->filename ) )
        fprintf( fp, "Filename      %s~\n", city->filename );
    if ( VLD_STR( city->description ) )
        fprintf( fp, "Description   %s~\n", strip_cr( city->description ) );
    if ( VLD_STR( city->bank ) )
        fprintf( fp, "Bank          %s~\n", city->bank );
    if ( VLD_STR( city->duke ) )
        fprintf( fp, "Duke          %s~\n", city->duke );
    if ( VLD_STR( city->baron ) )
        fprintf( fp, "Baron         %s~\n", city->baron );
    if ( VLD_STR( city->captain ) )
        fprintf( fp, "Captain       %s~\n", city->captain );
    if ( VLD_STR( city->sheriff ) )
        fprintf( fp, "Sheriff       %s~\n", city->sheriff );
    if ( VLD_STR( city->knight ) )
        fprintf( fp, "Knight        %s~\n", city->knight );
    if ( VLD_STR( city->allegiance ) )
        fprintf( fp, "Allegiance    %s~\n", city->allegiance );
    fprintf( fp, "Defense          %d\n", city->defense );
    fprintf( fp, "Offense          %d\n", city->offense );
    fprintf( fp, "OffenseCatapult  %d\n", city->ocatapult );
    fprintf( fp, "DefenseCatapult  %d\n", city->dcatapult );
    fprintf( fp, "Ballista         %d\n", city->ballista );
    fprintf( fp, "Soldiers         %d\n", city->soldiers );
    fprintf( fp, "Guards           %d\n", city->guards );
    fprintf( fp, "DefenseArchers   %d\n", city->darchers );
    fprintf( fp, "OffenseArchers   %d\n", city->oarchers );
    fprintf( fp, "DefenseWarriors  %d\n", city->dwarriors );
    fprintf( fp, "OffenseWarriors  %d\n", city->owarriors );
    fprintf( fp, "Arrows           %d\n", city->arrows );
    fprintf( fp, "Platform         %d\n", city->platform );
    fprintf( fp, "Ram              %d\n", city->ram );
    fprintf( fp, "Pitch            %d\n", city->pitch );
    fprintf( fp, "Wins             %d\n", city->wins );
    fprintf( fp, "Loses            %d\n", city->loses );
    fprintf( fp, "Recall           %d\n", city->recall );
    fprintf( fp, "End\n\n" );
    fwrite_citizenlist( fp, city );
    fprintf( fp, "#END\n" );
    FileClose( fp );
}

/* Read in actual city data. */
void fread_city( CITY_DATA * city, FILE * fp )
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

            case 'A':
                KEY( "Allegiance", city->allegiance, fread_string( fp ) );
                KEY( "Arrows", city->arrows, fread_number( fp ) );
                break;
            case 'B':
                KEY( "Ballista", city->ballista, fread_number( fp ) );
                KEY( "Bank", city->bank, fread_string( fp ) );
                KEY( "Baron", city->baron, fread_string( fp ) );
                break;
            case 'C':
                KEY( "Captain", city->captain, fread_string( fp ) );
                break;
            case 'D':
                KEY( "DefenseCatapult", city->dcatapult, fread_number( fp ) );
                KEY( "Defense", city->defense, fread_number( fp ) );
                KEY( "Description", city->description, fread_string( fp ) );
                KEY( "Duke", city->duke, fread_string( fp ) );
                KEY( "DefenseArchers", city->darchers, fread_number( fp ) );
                KEY( "DefenseWarriors", city->dwarriors, fread_number( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return;
                break;
            case 'F':
                KEY( "Filename", city->filename, fread_string( fp ) );
                break;
            case 'G':
                KEY( "Guards", city->guards, fread_number( fp ) );
                break;
            case 'K':
                KEY( "Knight", city->knight, fread_string( fp ) );
                break;
            case 'L':
                KEY( "Loses", city->loses, fread_number( fp ) );
                break;
            case 'N':
                KEY( "Name", city->name, fread_string( fp ) );
                break;
            case 'O':
                KEY( "OffenseCatapult", city->ocatapult, fread_number( fp ) );
                KEY( "Offense", city->offense, fread_number( fp ) );
                KEY( "OffenseArchers", city->oarchers, fread_number( fp ) );
                KEY( "OffenseWarriors", city->owarriors, fread_number( fp ) );
                break;
            case 'P':
                KEY( "Pitch", city->pitch, fread_number( fp ) );
                KEY( "Platform", city->platform, fread_number( fp ) );
                break;
            case 'R':
                KEY( "Ram", city->ram, fread_number( fp ) );
                KEY( "Recall", city->recall, fread_number( fp ) );
                break;
            case 'S':
                KEY( "Sheriff", city->sheriff, fread_string( fp ) );
                KEY( "Soldiers", city->soldiers, fread_number( fp ) );
                break;
            case 'W':
                KEY( "Wins", city->wins, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_city: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

void                    fread_citizenlist( CITY_DATA * city, FILE * fp );

/* Load a city file */
bool load_city_file( const char *cityfile )
{
    char                    filename[256];
    CITY_DATA              *city;
    FILE                   *fp;
    bool                    found;

    CREATE( city, CITY_DATA, 1 );
    found = FALSE;
    city->beingsieged = FALSE;
    city->siegestarted = 0;
    city->siege2started = 0;
    snprintf( filename, 256, "%s%s", CITY_DIR, cityfile );
    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {
        found = TRUE;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }
            if ( letter != '#' ) {
                bug( "Load_city_file: # not found." );
                break;
            }
            word = fread_word( fp );
            if ( !str_cmp( word, "CITY" ) )
                fread_city( city, fp );
            else if ( !str_cmp( word, "ROLLCALL" ) )
                fread_citizenlist( city, fp );
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s: bad section: %s.", __FUNCTION__, word );
                break;
            }
        }
        FileClose( fp );
    }
    if ( found )
        LINK( city, first_city, last_city, next, prev );
    else
        DISPOSE( city );
    return found;
}

/* Load in all the city files. */
void load_cities(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    citylist[256];

    first_city = NULL;
    last_city = NULL;
    log_string( "Loading cities..." );
    snprintf( citylist, 256, "%s%s", CITY_DIR, CITY_LIST );
    if ( ( fpList = FileOpen( citylist, "r" ) ) == NULL ) {
        bug( "load_cities: can't open %s for reading.", citylist );
        perror( citylist );
        exit( 1 );
    }
    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        log_string( filename );

        if ( filename[0] == '$' )
            break;
        if ( !load_city_file( filename ) )
            bug( "Cannot load city file: %s", filename );
    }
    FileClose( fpList );
    log_string( " Done cities " );
    return;
}

void do_revoke( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    CITY_DATA              *city;

    if ( IS_NPC( ch ) || !ch->pcdata->city ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    city = ch->pcdata->city;

    if ( !str_cmp( ch->name, city->duke ) || !str_cmp( ch->name, city->baron ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Revoke whom from your city?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( victim->pcdata->city != ch->pcdata->city ) {
        send_to_char( "This player does not belong to your city!\r\n", ch );
        return;
    }
    victim->pcdata->city = NULL;
    if ( VLD_STR( victim->pcdata->city_name ) )
        STRFREE( victim->pcdata->city_name );
    act( AT_MAGIC, "You revoke $N from $t", ch, city->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n revokes $N from $t", ch, city->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n revokes you from $t", ch, city->name, victim, TO_VICT );
    remove_rollcall( city, victim->name );
    save_char_obj( victim );
    save_city( city );
    return;
}

void do_setcity( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CITY_DATA              *city;
    char                    buf[MSL];

    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: setcity <city> <field> <player>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " duke baron captain sheriff knight\r\n", ch );
        send_to_char( " desc endsiege reset recall\r\n", ch );
        send_to_char( " name filename allegiance\r\n", ch );
        return;
    }

    city = get_city( arg1 );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "duke" ) ) {
        if ( VLD_STR( city->duke ) )
            STRFREE( city->duke );
        if ( VLD_STR( argument ) )
            city->duke = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "baron" ) ) {
        if ( VLD_STR( city->baron ) )
            STRFREE( city->baron );
        if ( VLD_STR( argument ) )
            city->baron = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "endsiege" ) ) {
        sprintf( buf, "&CThe siege against %s has been ended early by %s!", city->name, ch->name );
        announce( buf );
        city->siegestarted = 0;
        end_siege( city, 1 );
        return;
    }
    else if ( !str_cmp( arg2, "bank" ) ) {
        if ( VLD_STR( city->bank ) )
            STRFREE( city->bank );
        if ( VLD_STR( argument ) )
            city->bank = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "captain" ) ) {
        if ( VLD_STR( city->captain ) )
            STRFREE( city->captain );
        if ( VLD_STR( argument ) )
            city->captain = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "sheriff" ) ) {
        if ( VLD_STR( city->sheriff ) )
            STRFREE( city->sheriff );
        if ( VLD_STR( argument ) )
            city->sheriff = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "knight" ) ) {
        if ( VLD_STR( city->knight ) )
            STRFREE( city->knight );
        if ( VLD_STR( argument ) )
            city->knight = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "allegiance" ) ) {
        if ( VLD_STR( city->allegiance ) )
            STRFREE( city->allegiance );
        if ( VLD_STR( argument ) )
            city->allegiance = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "recall" ) ) {
        city->recall = atoi( argument );
        send_to_char( "City recall set.\r\n", ch );
    }

    else if ( !str_cmp( arg2, "reset" ) ) {
        city->ballista = 0;
        city->ram = 0;
        city->platform = 0;
        city->ocatapult = 0;
        city->soldiers = 0;
        city->oarchers = 0;
        city->owarriors = 0;
        city->pitch = 0;
        city->arrows = 0;
        city->dcatapult = 0;
        city->guards = 0;
        city->darchers = 0;
        city->dwarriors = 0;
        city->defense = 0;
        city->offense = 0;
        save_city( city );
    }
    else if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        do_setcity( ch, ( char * ) "" );
        return;
    }
    else if ( !str_cmp( arg2, "name" ) ) {
        if ( VLD_STR( city->name ) )
            STRFREE( city->name );
        if ( VLD_STR( argument ) )
            city->name = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "filename" ) ) {
        if ( VLD_STR( city->filename ) )
            STRFREE( city->filename );
        if ( VLD_STR( argument ) )
            city->filename = STRALLOC( argument );
        write_city_list(  );
    }
    else if ( !str_cmp( arg2, "desc" ) ) {
        if ( VLD_STR( city->description ) )
            STRFREE( city->description );
        if ( VLD_STR( argument ) )
            city->description = STRALLOC( argument );
    }
    else if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        do_setcity( ch, ( char * ) "" );
        return;
    }
    else {
        do_setcity( ch, ( char * ) "" );
        return;
    }
    send_to_char( "Done.\r\n", ch );
    save_city( city );
    return;
}

void do_showcity( CHAR_DATA *ch, char *argument )
{
    CITY_DATA              *city;

    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Usage: showcity <city>\r\n", ch );
        return;
    }

    city = get_city( argument );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }
    ch_printf( ch, "\r\n&wCity:     &W%s\r\n", city->name ? city->name : "Not Set" );
    ch_printf( ch, "&wFilename:     &W%s\r\n", city->filename ? city->filename : "Not Set" );
    ch_printf( ch, "&wAllegiance:   &W%s\r\n", city->allegiance ? city->allegiance : "Not Set" );
    ch_printf( ch, "&wDuke:         &W%s\r\n", city->duke ? city->duke : "Not Set" );
    ch_printf( ch, "&wBaron:        &W%s\r\n", city->baron ? city->baron : "Not Set" );
    ch_printf( ch, "&wCaptain:      &W%s\r\n", city->captain ? city->captain : "Not Set" );
    ch_printf( ch, "&wSheriff:      &W%s\r\n", city->sheriff ? city->sheriff : "Not Set" );
    ch_printf( ch, "&wKnight:       &W%s\r\n", city->knight ? city->knight : "Not Set" );
    ch_printf( ch, "&wDefense:      &W%d\r\n", city->defense );
    ch_printf( ch, "&wOffense:      &W%d\r\n", city->offense );
    ch_printf( ch, "&wWins:         &W%d\r\n", city->wins );
    ch_printf( ch, "&wLoses:        &W%d\r\n", city->loses );
    ch_printf( ch, "&wRecall:       &W%d\r\n", city->recall );
    ch_printf( ch, "&wDescription:\r\n&W%s\r\n",
               city->description ? city->description : "Not Set" );
}

void do_makecity( CHAR_DATA *ch, char *argument )
{
    char                    filename[256];
    CITY_DATA              *city;
    bool                    found;

    set_char_color( AT_IMMORT, ch );
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Usage: makecity <city name>\r\n", ch );
        return;
    }
    found = FALSE;
    snprintf( filename, 256, "%s%s", CITY_DIR, strlower( argument ) );
    CREATE( city, CITY_DATA, 1 );
    LINK( city, first_city, last_city, next, prev );
    city->name = STRALLOC( argument );
    send_to_char( "city now made.\r\n", ch );
}

void do_city( CHAR_DATA *ch, char *argument )
{
    CITY_DATA              *city;

    set_char_color( AT_CYAN, ch );
    if ( !first_city ) {
        send_to_char( "There are no cities currently formed.\r\n", ch );
        return;
    }
    if ( !VLD_STR( argument ) ) {
        send_to_char( "\r\n&WCurrent Cities in the 6 Dragons realms\r\n", ch );
        send_to_char
            ( "&c----------------------------------------------------------------------------\r\n",
              ch );
        send_to_char( "\r\n&cCity                     Allegiance           Duke\r\n", ch );
        for ( city = first_city; city; city = city->next )
            ch_printf( ch, "&C%-24s %-20s %-20s\r\n", city->name ? city->name : "Unknown City",
                       city->allegiance ? city->allegiance : "No Allegiance",
                       city->duke ? city->duke : "No Duke" );
        send_to_char( "\r\n&cUse '&Wcity &c<&Wname of city&c>' for more detailed information.\r\n",
                      ch );
        return;
    }
    city = get_city( argument );
    if ( !city ) {
        send_to_char( "&cNo such city exists...\r\n", ch );
        return;
    }
    ch_printf( ch, "&c\r\n%s\r\n", city->name );
    ch_printf( ch, "&cDuke:       &w%s\r\n", city->duke ? city->duke : "No One" );
    ch_printf( ch, "&cBaron:      &w%s\r\n", city->baron ? city->baron : "No One" );
    ch_printf( ch, "&cCaptain:    &w%s\r\n", city->captain ? city->captain : "No One" );
    ch_printf( ch, "&cSheriff:    &w%s\r\n", city->sheriff ? city->sheriff : "No One" );
    ch_printf( ch, "&cKnight:     &w%s\r\n", city->knight ? city->knight : "No One" );
    ch_printf( ch, "&cAllegiance  &w%s\r\n", city->allegiance ? city->allegiance : "No One" );
    ch_printf( ch, "&cDefense:    &w%d\r\n", city->defense );
    ch_printf( ch, "&cOffense:    &w%d\r\n", city->offense );
    ch_printf( ch, "&cWins:       &w%d    &cLoses:      &w%d\r\n", city->wins, city->loses );
    ch_printf( ch, "&cDescription:\r\n&w%s\r\n",
               city->description ? city->description : "No Description" );
}

void do_mpcity( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    arg1[MIL];

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpcity - Invalid argument [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpcity [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    CITY_DATA              *city;

    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", victim );
        return;
    }

    city = get_city( arg1 );
    if ( !city ) {
        send_to_char( "No such city.\r\n", victim );
        return;
    }
/*
         if( victim->pcdata->city )
         {
            save_city( victim->pcdata->city );
         if( VLD_STR( victim->pcdata->city_name ) )
            STRFREE( victim->pcdata->city_name );
         victim->pcdata->city = NULL;
   act( AT_WHITE, "$n revokes $N's citizenship.", ch, NULL, victim, TO_NOTVICT );
   act( AT_WHITE, "$n revokes your citizenship.", ch, NULL, victim, TO_VICT );
         return;
         }
*/
    if ( VLD_STR( victim->pcdata->city_name ) )
        STRFREE( victim->pcdata->city_name );
    victim->pcdata->city_name = QUICKLINK( city->name );
    victim->pcdata->city = city;
    add_rollcall( city, victim->name, victim->Class, victim->level, victim->pcdata->mkills,
                  victim->pcdata->mdeaths );
    save_char_obj( victim );
    act( AT_WHITE, "$n awards $N citizenship in $t.", ch, city->name, victim, TO_NOTVICT );
    act( AT_WHITE, "$n inducts you citizenship in $t.", ch, city->name, victim, TO_VICT );

    return;
}

void do_fortify( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    CITY_DATA              *city;
    BANK_DATA              *bank;

    if ( IS_NPC( ch ) || !ch->pcdata->city ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    city = get_city( ch->pcdata->city_name );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }

    bank = find_bank( city->bank );

    if ( !bank ) {
        send_to_char( "There is no account for that city!\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: fortify <field>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " pitch arrows catapults guards warriors archers\r\n", ch );
        send_to_char( " costs - lists costs to fortify\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "costs" ) ) {
        ch_printf( ch, "Pitch costs 50 gold. You have %d of them.\r\n", city->pitch );
        ch_printf( ch, "Arrows costs 10 gold. You have %d of them.\r\n", city->arrows );
        ch_printf( ch, "Catapults costs 50 gold. You have %d of them.\r\n", city->dcatapult );
        ch_printf( ch, "Guards costs 10 gold. You have %d of them.\r\n", city->guards );
        ch_printf( ch, "Archers costs 10 gold. You have %d of them.\r\n", city->darchers );
        ch_printf( ch, "Warriors costs 10 gold. You have %d of them.\r\n", city->dwarriors );
        ch_printf( ch, "You have %d gold in %s city bank.\r\n", bank->gold, city->name );
        return;
    }

    if ( city->beingsieged == TRUE ) {
        send_to_char( "You cannot fortify during a siege\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "pitch" ) ) {
        if ( city->pitch >= 10 ) {
            send_to_char( "You cannot have more then 10 cauldrons of pitch.\r\n", ch );
            return;
        }
        if ( bank->gold < 50 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 50;
        send_to_char( "You fortify your city with a cauldron of burning pitch.\r\n", ch );
        city->pitch += 1;
        city->defense += 10;
        save_bank(  );
        if ( city->defense > 500 ) {
            city->defense = 500;
        }
        save_city( city );
        return;
    }

    else if ( !str_cmp( arg1, "arrows" ) ) {
        if ( city->arrows >= 50 ) {
            send_to_char( "You cannot have more then 50 dozen arrows.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You fortify your city with a dozen arrows.\r\n", ch );
        city->arrows += 1;
        city->defense += 1;
        if ( city->defense > 500 ) {
            city->defense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "catapults" ) ) {
        if ( city->dcatapult >= 10 ) {
            send_to_char( "You cannot have more then 10 catapults.\r\n", ch );
            return;
        }
        if ( bank->gold < 50 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 50;
        send_to_char( "You fortify your city with a new catapult.\r\n", ch );
        city->dcatapult += 1;
        city->defense += 5;
        if ( city->defense > 500 ) {
            city->defense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "guards" ) ) {
        if ( city->guards >= 50 ) {
            send_to_char( "You cannot have more then 50 guards.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You fortify your city with a new guard.\r\n", ch );
        city->guards += 1;
        city->defense += 2;
        save_bank(  );
        if ( city->defense > 500 ) {
            city->defense = 500;
        }
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "archers" ) ) {
        if ( city->darchers >= 50 ) {
            send_to_char( "You cannot have more then 50 archers.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You fortify your city with a new archer.\r\n", ch );
        city->darchers += 1;
        city->defense += 2;
        save_bank(  );
        if ( city->defense > 500 ) {
            city->defense = 500;
        }
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "warriors" ) ) {
        if ( city->dwarriors >= 50 ) {
            send_to_char( "You cannot have more then 50 warriors.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You fortify your city with a new warrior.\r\n", ch );
        city->dwarriors += 1;
        city->defense += 2;
        save_bank(  );
        if ( city->defense > 500 ) {
            city->defense = 500;
        }
        save_city( city );
        return;
    }

// Build up the city defenses against a siege
    return;
}

void do_siege( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    CITY_DATA              *city;
    BANK_DATA              *bank;

    if ( IS_NPC( ch ) || !ch->pcdata->city ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg1 );
    city = get_city( ch->pcdata->city_name );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }

    bank = find_bank( city->bank );

    if ( !bank ) {
        send_to_char( "There is no account for that city!\r\n", ch );
        return;
    }

    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: siege <field>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " catapults ballista ram platform soldiers archers warriors\r\n", ch );
        send_to_char( " costs - displays the costs for siege items\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "costs" ) ) {
        ch_printf( ch, "Ballista costs 50 gold. You have %d of them.\r\n", city->ballista );
        ch_printf( ch, "Rams cost 40 gold. You have %d of them.\r\n", city->ram );
        ch_printf( ch, "Platforms costs 50 gold. You have %d of them.\r\n", city->platform );
        ch_printf( ch, "Catapults costs 50 gold. You have %d of them.\r\n", city->ocatapult );
        ch_printf( ch, "Soldiers costs 10 gold. You have %d of them.\r\n", city->soldiers );
        ch_printf( ch, "Archers costs 10 gold. You have %d of them.\r\n", city->oarchers );
        ch_printf( ch, "Warriors costs 10 gold. You have %d of them.\r\n", city->owarriors );
        ch_printf( ch, "You have %d gold in %s city bank.\r\n", bank->gold, city->name );
        return;
    }

    if ( city->beingsieged == TRUE ) {
        send_to_char( "You cannot add to your siege during a siege\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "ballista" ) ) {
        if ( city->ballista >= 5 ) {
            send_to_char( "You cannot have more then 5 ballistas.\r\n", ch );
            return;
        }
        if ( bank->gold < 50 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 50;
        send_to_char( "You add to your city arsenal a new ballista.\r\n", ch );
        city->ballista += 1;
        city->offense += 10;
        if ( city->offense > 500 ) {
            city->offense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "ram" ) ) {
        if ( city->ram >= 1 ) {
            send_to_char( "You cannot have more then 1 ram.\r\n", ch );
            return;
        }
        if ( bank->gold < 40 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 40;
        send_to_char( "You add to your city arsenal a new ram.\r\n", ch );
        city->ram += 1;
        city->offense += 25;
        save_bank(  );
        if ( city->offense > 500 ) {
            city->offense = 500;
        }

        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "catapults" ) ) {
        if ( city->ocatapult >= 5 ) {
            send_to_char( "You cannot have more then 5 catapults.\r\n", ch );
            return;
        }
        if ( bank->gold < 50 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 50;
        send_to_char( "You add to your city arsenal a new catapult.\r\n", ch );
        city->ocatapult += 1;
        city->offense += 10;
        if ( city->offense > 500 ) {
            city->offense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "soldiers" ) ) {
        if ( city->soldiers >= 50 ) {
            send_to_char( "You cannot have more then 50 soldiers.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You add to your city arsenal a new soldier.\r\n", ch );
        city->soldiers += 1;
        city->offense += 2;
        if ( city->offense > 500 ) {
            city->offense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "archers" ) ) {
        if ( city->oarchers >= 50 ) {
            send_to_char( "You cannot have more then 50 archerss.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You add to your city arsenal a new archer.\r\n", ch );
        city->oarchers += 1;
        city->offense += 2;
        if ( city->offense > 500 ) {
            city->offense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }
    else if ( !str_cmp( arg1, "warriors" ) ) {
        if ( city->owarriors >= 50 ) {
            send_to_char( "You cannot have more then 50 warriors.\r\n", ch );
            return;
        }
        if ( bank->gold < 10 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 10;
        send_to_char( "You add to your city arsenal a new warrior.\r\n", ch );
        city->owarriors += 1;
        city->offense += 2;
        if ( city->offense > 500 ) {
            city->offense = 500;
        }
        save_bank(  );
        save_city( city );
        return;
    }

    else if ( !str_cmp( arg1, "platform" ) ) {
        if ( city->platform >= 5 ) {
            send_to_char( "You cannot have more then 5 wooden platforms.\r\n", ch );
            return;
        }
        if ( bank->gold < 50 ) {
            send_to_char( "Your city bank account cannot afford that.\r\n", ch );
            return;
        }
        bank->gold -= 50;
        send_to_char( "You add to your city arsenal a new wooden platform.\r\n", ch );
        city->platform += 1;
        city->offense += 15;
        save_bank(  );
        if ( city->offense > 500 ) {
            city->offense = 500;
        }
        save_city( city );
        return;
    }
// Build up offense in order to lay siege
    return;
}

// In addition to do_contribute, all citizens should have to contribute once a day via tax
void do_tax( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MIL];
    char                    arg2[MIL];
    CITY_DATA              *city;
    BANK_DATA              *bank;
    int                     amount;
    int                     currtime = time( 0 );

    if ( IS_NPC( ch ) || !ch->pcdata->city ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    city = get_city( ch->pcdata->city_name );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }

    bank = find_bank( city->bank );

    if ( ( city->duke && !str_cmp( ch->name, city->duke ) )
         || ( city->baron && !str_cmp( ch->name, city->baron ) ) || ( city->captain
                                                                      && !str_cmp( ch->name,
                                                                                   city->captain ) )
         || ( city->sheriff && !str_cmp( ch->name, city->sheriff ) ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !bank ) {
        send_to_char( "There is no account for that city!\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, arg1 ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "They are not a citizen.\r\n", ch );
        return;
    }

    if ( !victim->pcdata->city ) {
        send_to_char( "They are not a citizen.\r\n", ch );
        return;
    }

    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: tax <player>\r\n", ch );
        return;
    }

    if ( str_cmp( ch->pcdata->city_name, victim->pcdata->city_name ) ) {
        send_to_char( "They are not a citizen of your city.\r\n", ch );
        return;
    }

    amount = 1;

    if ( ( currtime - victim->pcdata->warn ) < 3600 ) {
        send_to_char
            ( "Please wait at least one mud hour before attempting to collect taxes again.\r\n",
              ch );
        return;
    }

    if ( amount > GET_MONEY( victim, CURR_GOLD ) ) {
        victim->pcdata->warn = currtime;
        send_to_char( "They don't have enough gold to pay their taxes.\r\n", ch );
        send_to_char
            ( "You don't have enough gold to pay your taxes.\r\nYou have one mud hour to come up with 1 gold piece for your taxes.",
              victim );
        return;
    }

    bank->gold += amount;
    GET_MONEY( victim, CURR_GOLD ) -= amount;
    ch_printf( ch, "\r\n&cYou collect %d gold from %s to add to the city bank.\r\n", amount,
               victim->name );
    ch_printf( victim, "\r\n&cYou pay %d gold for your taxes to the city bank.\r\n", amount );
    save_char_obj( victim );
    save_bank(  );

// tax should be 1 gold daily, on a 3 strike system.  
// If you don't have 1 gold after 3 strikes, lose citizenship.
}

void do_contribute( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    CITY_DATA              *city;
    BANK_DATA              *bank;
    int                     amount;

    if ( IS_NPC( ch ) || !ch->pcdata->city ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    city = get_city( ch->pcdata->city_name );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }

    bank = find_bank( city->bank );

    if ( !bank ) {
        send_to_char( "There is no account for that city!\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "Usage: contribute <amount>\r\n", ch );
        return;
    }

    amount = atoi( arg1 );

    if ( amount > GET_MONEY( ch, CURR_GOLD ) ) {
        send_to_char( "You don't have enough gold to contribute that much.\r\n", ch );
        return;
    }

    bank->gold += amount;
    GET_MONEY( ch, CURR_GOLD ) -= amount;
    ch_printf( ch, "\r\n&cYou contribute %d gold to the city bank.\r\n", amount );
    save_char_obj( ch );
    save_bank(  );
}

void do_setenemy( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CITY_DATA              *city;

    if ( IS_NPC( ch ) || !ch->pcdata->city ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: setenemy <field> <player>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " race class player city clan\r\n", ch );
        return;
    }
    city = get_city( ch->pcdata->city_name );
    if ( !city ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
    }

    return;
}

/* Count up the defenders */
int count_defenders( CITY_DATA * city )
{
    ROOM_INDEX_DATA        *location;
    OBJ_DATA               *obj;
    CHAR_DATA              *rch;
    int                     defenders = 0;

    if ( !str_cmp( city->name, "Paleon City" ) ) {
        location = get_room_index( PALEON_DEFEND );
        if ( !location )
            bug( "%s: couldn't find room %d.", __FUNCTION__, PALEON_DEFEND );
        else {
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }
            location = get_room_index( PALEON_DEFEND2 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }
            location = get_room_index( PALEON_DEFEND3 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }

        }
    }
    else if ( !str_cmp( city->name, "Dakar City" ) ) {
        defenders = 0;
        location = get_room_index( DAKAR_DEFEND );
        if ( !location )
            bug( "%s: couldn't find room %d.", __FUNCTION__, DAKAR_DEFEND );
        else {
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }
            location = get_room_index( DAKAR_DEFEND2 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }
            location = get_room_index( DAKAR_DEFEND3 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }

        }
    }
    else if ( !str_cmp( city->name, "Forbidden City" ) ) {
        defenders = 0;
        location = get_room_index( FORBIDDEN_DEFEND );
        if ( !location )
            bug( "%s: couldn't find room %d.", __FUNCTION__, FORBIDDEN_DEFEND );
        else {
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }
            location = get_room_index( FORBIDDEN_DEFEND2 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }
            location = get_room_index( FORBIDDEN_DEFEND3 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    defenders++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    defenders++;
            }

        }
    }

    /*
     * If we got here they have no items or guards so return true 
     */
    return defenders;
}

/* Count up the attackers */
int count_attackers( CITY_DATA * city )
{
    ROOM_INDEX_DATA        *location;
    OBJ_DATA               *obj;
    CHAR_DATA              *rch;
    int                     attackers = 0;

    if ( !str_cmp( city->name, "Paleon City" ) ) {
        location = get_room_index( PALEON_SIEGE );
        if ( !location )
            bug( "%s: couldn't find room %d.", __FUNCTION__, PALEON_SIEGE );
        else {
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }
            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;
            }
            location = get_room_index( PALEON_SIEGE2 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }
            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;

            }
            location = get_room_index( PALEON_SIEGE3 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }
            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;
            }

        }
    }
    else if ( !str_cmp( city->name, "Dakar City" ) ) {
        attackers = 0;
        location = get_room_index( DAKAR_SIEGE );
        if ( !location )
            bug( "%s: couldn't find room %d.", __FUNCTION__, DAKAR_SIEGE );
        else {
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;
            }
            location = get_room_index( DAKAR_SIEGE2 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;
            }
            location = get_room_index( DAKAR_SIEGE3 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;

            }

        }
    }
    else if ( !str_cmp( city->name, "Forbidden City" ) ) {
        attackers = 0;
        location = get_room_index( FORBIDDEN_SIEGE );
        if ( !location )
            bug( "%s: couldn't find room %d.", __FUNCTION__, FORBIDDEN_SIEGE );
        else {
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;

            }
            location = get_room_index( FORBIDDEN_SIEGE2 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;

            }
            location = get_room_index( FORBIDDEN_SIEGE3 );
            for ( obj = location->first_content; obj; obj = obj->next_content ) {
                if ( obj->item_type == ITEM_SABOTAGE )
                    attackers++;
            }

            for ( rch = location->first_person; rch; rch = rch->next_in_room ) {
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                    attackers++;
                if ( IS_NPC( rch ) && rch->pIndexData->vnum == MOB_VNUM_ARCHERS )
                    attackers++;

            }

        }
    }

    /*
     * If we got here they have no items or guards so return true 
     */
    return attackers;
}

/* Put what should happen when the siege ends in here */
void end_siege( CITY_DATA * city, short tend )
{
    CITY_DATA              *acity = NULL;              /* Set this to who is attacking
                                                        * and ending the siege now */
    char                    buf[MSL];
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    ROOM_INDEX_DATA        *location = NULL,
        *location2 = NULL,
        *location3 = NULL;
    CHAR_DATA              *rch,
                           *rch_next;
    int                     attackers = 0,
        defenders = 0;

    if ( !city )
        return;

    /*
     * Ok first lets see which one ends now 
     */
    if ( tend == 1 ) {
        acity = get_city( city->attacker );
        STRFREE( city->attacker );
        city->siegestarted = 0;
    }
    if ( tend == 2 ) {
        acity = get_city( city->attacker2 );
        STRFREE( city->attacker2 );
        city->siege2started = 0;
    }

    if ( !acity )                                      /* Make sure acity is valid incase 
                                                        * we got some bad data somewhere
                                                        * or * something odd happened lol 
                                                        */
        return;

    attackers = count_attackers( city );
    defenders = count_defenders( city );

    /*
     * I think it's best to use the attackers and defenders instead of defense and offense
     * from the citys 
     */
    if ( ( attackers + acity->offense ) > ( defenders + city->defense ) ) {
        sprintf( buf, "&C%s has defeated %s and stormed their city walls!", acity->name,
                 city->name );
        announce( buf );
        acity->wins += 1;
        city->loses += 1;
        city->beingsieged = 0;
        acity->beingsieged = 0;
        // declare attackers winners
        CHAR_DATA              *wch;

        for ( wch = first_char; wch; wch = wch->next ) {
            if ( IS_NPC( wch ) || !IS_CITY( wch ) )
                continue;
            if ( wch->pcdata->city_name && wch->pcdata->city_name[0] != '\0'
                 && !str_cmp( wch->pcdata->city_name, acity->name ) ) {
                send_to_char( "&cYou gain &W10&c Glory for winning the siege!\r\n", wch );
                wch->quest_curr += 10;
            }
        }

    }
    else {
        sprintf( buf, "&C%s has defeated %s and protected their city walls!", city->name,
                 acity->name );
        announce( buf );
        city->wins += 1;
        acity->loses += 1;
        city->beingsieged = 0;
        acity->beingsieged = 0;

        CHAR_DATA              *tch;

        for ( tch = first_char; tch; tch = tch->next ) {
            if ( IS_NPC( tch ) || !IS_CITY( tch ) )
                continue;
            if ( tch->pcdata->city_name && tch->pcdata->city_name[0] != '\0'
                 && !str_cmp( tch->pcdata->city_name, city->name ) ) {
                send_to_char( "&cYou gain &W10&c Glory for winning the siege!\r\n", tch );
                tch->quest_curr += 10;
            }
        }

        // declare defenders winners
    }

    if ( !str_cmp( city->name, "Paleon City" ) ) {
        location = get_room_index( PALEON_SIEGE );
        location2 = get_room_index( PALEON_SIEGE2 );
        location3 = get_room_index( PALEON_SIEGE3 );
    }
    else if ( !str_cmp( city->name, "Dakar City" ) ) {
        location = get_room_index( DAKAR_SIEGE );
        location2 = get_room_index( DAKAR_SIEGE2 );
        location3 = get_room_index( DAKAR_SIEGE3 );
    }
    else if ( !str_cmp( city->name, "Forbidden City" ) ) {
        location = get_room_index( FORBIDDEN_SIEGE );
        location2 = get_room_index( FORBIDDEN_SIEGE2 );
        location3 = get_room_index( FORBIDDEN_SIEGE3 );
    }
    /*
     * Ok have a valid location so extract sabotage items 
     */
    if ( location ) {
        REMOVE_BIT( location->room_flags, ROOM_ARENA );
        REMOVE_BIT( location2->room_flags, ROOM_ARENA );
        REMOVE_BIT( location3->room_flags, ROOM_ARENA );
        for ( obj = location->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( obj->item_type == ITEM_SABOTAGE ) {
                obj_from_room( obj );
                extract_obj( obj );
            }
        }

        for ( obj = location2->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( obj->item_type == ITEM_SABOTAGE ) {
                obj_from_room( obj );
                extract_obj( obj );
            }
        }

        for ( obj = location3->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( obj->item_type == ITEM_SABOTAGE ) {
                obj_from_room( obj );
                extract_obj( obj );
            }
        }

        for ( rch = location->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_SOLDIERS )
                continue;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_ARCHERS )
                continue;
            extract_char( rch, TRUE );
        }
        for ( rch = location2->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_SOLDIERS )
                continue;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_ARCHERS )
                continue;
            extract_char( rch, TRUE );
        }
        for ( rch = location3->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_SOLDIERS )
                continue;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_ARCHERS )
                continue;
            extract_char( rch, TRUE );
        }
    }

    if ( !str_cmp( city->name, "Paleon City" ) ) {
        location = get_room_index( PALEON_DEFEND );
        location2 = get_room_index( PALEON_DEFEND2 );
        location3 = get_room_index( PALEON_DEFEND3 );
    }
    else if ( !str_cmp( city->name, "Dakar City" ) ) {
        location = get_room_index( DAKAR_DEFEND );
        location2 = get_room_index( DAKAR_DEFEND2 );
        location3 = get_room_index( DAKAR_DEFEND3 );
    }
    else if ( !str_cmp( city->name, "Forbidden City" ) ) {
        location = get_room_index( FORBIDDEN_DEFEND );
        location2 = get_room_index( FORBIDDEN_DEFEND2 );
        location3 = get_room_index( FORBIDDEN_DEFEND3 );
    }
    if ( location ) {
        REMOVE_BIT( location->room_flags, ROOM_ARENA );
        REMOVE_BIT( location2->room_flags, ROOM_ARENA );
        REMOVE_BIT( location3->room_flags, ROOM_ARENA );
        for ( obj = location->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( obj->item_type == ITEM_SABOTAGE ) {
                obj_from_room( obj );
                extract_obj( obj );
            }
        }

        for ( obj = location2->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( obj->item_type == ITEM_SABOTAGE ) {
                obj_from_room( obj );
                extract_obj( obj );
            }
        }

        for ( obj = location3->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( obj->item_type == ITEM_SABOTAGE ) {
                obj_from_room( obj );
                extract_obj( obj );
            }
        }

        for ( rch = location->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_SOLDIERS )
                continue;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_ARCHERS )
                continue;
            extract_char( rch, TRUE );
        }
        for ( rch = location2->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_SOLDIERS )
                continue;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_ARCHERS )
                continue;
            extract_char( rch, TRUE );
        }
        for ( rch = location3->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_SOLDIERS )
                continue;
            if ( !IS_NPC( rch ) || rch->pIndexData->vnum != MOB_VNUM_ARCHERS )
                continue;
            extract_char( rch, TRUE );
        }

    }

    /*
     * Ok city is the defending city, acity is the attacking city 
     */
}

bool should_end_siege( CITY_DATA * city )
{
    /*
     * If no attackers or defenders end it 
     */
    if ( !count_defenders( city ) )
        return TRUE;
    else if ( !count_attackers( city ) )
        return TRUE;
    return FALSE;
}

time_t                  last_siege_check = 0;

void handle_sieges( void )
{
    CITY_DATA              *city;
    char                    buf[MSL];

    if ( !last_siege_check )
        last_siege_check = ( current_time - 1 );

    if ( last_siege_check >= current_time )
        return;

    for ( city = first_city; city; city = city->next ) {
        if ( !city->beingsieged )
            continue;

        last_siege_check = current_time;

        if ( should_end_siege( city ) ) {              /* Ok so the defense/attackers are 
                                                        * gone so end the sieges */
            if ( city->siegestarted != 0 )
                end_siege( city, 1 );
            if ( city->siege2started != 0 )
                end_siege( city, 2 );
            continue;
        }

        if ( ( ( city->siegestarted + 1800 ) - current_time ) == 1200 )
            announce( "&CThere are 20 minutes of time left in the siege!" );
        else if ( ( ( city->siegestarted + 1800 ) - current_time ) == 600 )
            announce( "&CThere are 10 minutes of time left in the siege!" );
        else if ( ( ( city->siegestarted + 1800 ) - current_time ) == 180 )
            announce( "&CThere are 3 minutes of time left in the siege!" );

        if ( city->siegestarted != 0 && current_time > ( city->siegestarted + 1800 ) )
            end_siege( city, 1 );
        if ( city->siege2started != 0 && current_time > ( city->siege2started + 1800 ) )
            end_siege( city, 2 );
    }

}

bool can_siege( CITY_DATA * city, char *name )
{
    if ( city->duke && !str_cmp( name, city->duke ) )
        return TRUE;
    if ( city->baron && !str_cmp( name, city->baron ) )
        return TRUE;
    if ( city->captain && !str_cmp( name, city->captain ) )
        return TRUE;
    if ( city->sheriff && !str_cmp( name, city->sheriff ) )
        return TRUE;
    if ( city->knight && !str_cmp( name, city->knight ) )
        return TRUE;
    return FALSE;
}

void do_laysiege( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    int                     count,
                            created,
                            vnum;
    CITY_DATA              *ocity;
    CITY_DATA              *dcity;
    CHAR_DATA              *mob;
    OBJ_DATA               *obj;
    OBJ_INDEX_DATA         *oindex;
    char                    buf[MSL];
    ROOM_INDEX_DATA        *location = NULL,
        *dlocation = NULL,
        *location2 = NULL,
        *location3 = NULL,
        *dlocation2 = NULL,
        *dlocation3 = NULL;
    short                   dcnum;                     /* What city is defending? */
    short                   acnum;                     /* What city is attacking? */

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh ?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: laysiege <city>\r\n", ch );
        return;
    }

    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "Your adrenaline is pumping too hard right now!\r\n", ch );
        return;
    }

    ocity = get_city( ch->pcdata->city_name );
    dcity = get_city( arg1 );

    if ( !ocity ) {
        send_to_char( "You aren't even a citizen of a city.\r\n", ch );
        return;
    }

    if ( !dcity ) {
        send_to_char( "No such city.\r\n", ch );
        return;
    }

    if ( !can_siege( ocity, ch->name ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( ocity == dcity ) {
        send_to_char( "Why would you need to laysiege against your own town?\r\n", ch );
        return;
    }

    if ( ch->in_room->vnum != PALEON_SIEGE && ch->in_room->vnum != FORBIDDEN_SIEGE
         && ch->in_room->vnum != DAKAR_SIEGE ) {
        send_to_char( "You are not at a siege location.\r\n", ch );
        return;
    }

    /*
     * Remember we can have 2 sieges on a city 
     */
    if ( dcity->siegestarted && dcity->siege2started ) {
        send_to_char( "There is already a siege underway.\r\n", ch );
        return;
    }

    CHAR_DATA              *wch;
    bool                    SIEGE = FALSE;

    // I need a loop check here to see if any defenders online
    for ( wch = first_char; wch; wch = wch->next ) {
        if ( IS_NPC( wch ) || !IS_CITY( wch ) )
            continue;
        if ( wch->pcdata->city_name && wch->pcdata->city_name[0] != '\0'
             && !str_cmp( wch->pcdata->city_name, dcity->name ) ) {
            SIEGE = TRUE;
            break;
        }
    }

    if ( ( VLD_STR( dcity->attacker ) && !str_cmp( dcity->attacker, ocity->name ) )
         || ( VLD_STR( dcity->attacker2 ) && !str_cmp( dcity->attacker2, ocity->name ) ) ) {
        send_to_char( "You are already attacking that city.\r\n", ch );
        return;
    }

    if ( SIEGE == FALSE ) {
        send_to_char( "There are no defenders online to laysiege against.\r\n", ch );
        return;
    }
    /*
     * No point in putting it more then once if the city attacks more then once 
     */
    if ( !VLD_STR( dcity->attacker ) ) {
        if ( !VLD_STR( dcity->attacker2 ) || str_cmp( dcity->attacker2, ocity->name ) ) {
            dcity->siegestarted = current_time;
            dcity->attacker = STRALLOC( ocity->name );
        }
    }
    else if ( !VLD_STR( dcity->attacker2 ) ) {
        if ( !VLD_STR( dcity->attacker ) || str_cmp( dcity->attacker, ocity->name ) ) {
            dcity->siege2started = current_time;
            dcity->attacker = STRALLOC( ocity->name );
        }
    }

    dcity->beingsieged = TRUE;
    sprintf( buf, "&C%s has started a siege against %s!", ocity->name, dcity->name );
    announce( buf );

    if ( !str_cmp( dcity->name, "Paleon City" ) ) {
        dlocation = get_room_index( PALEON_DEFEND );
        dlocation2 = get_room_index( PALEON_DEFEND2 );
        dlocation3 = get_room_index( PALEON_DEFEND3 );
        location = get_room_index( PALEON_SIEGE );
        location2 = get_room_index( PALEON_SIEGE2 );
        location3 = get_room_index( PALEON_SIEGE3 );
        dcnum = 0;
    }
    else if ( !str_cmp( dcity->name, "Dakar City" ) ) {
        dlocation = get_room_index( DAKAR_DEFEND );
        dlocation2 = get_room_index( DAKAR_DEFEND2 );
        dlocation3 = get_room_index( DAKAR_DEFEND3 );
        location = get_room_index( DAKAR_SIEGE );
        location2 = get_room_index( DAKAR_SIEGE2 );
        location3 = get_room_index( DAKAR_SIEGE3 );
        dcnum = 1;
    }
    else if ( !str_cmp( dcity->name, "Forbidden City" ) ) {
        dlocation = get_room_index( FORBIDDEN_DEFEND );
        dlocation2 = get_room_index( FORBIDDEN_DEFEND2 );
        dlocation3 = get_room_index( FORBIDDEN_DEFEND3 );
        location = get_room_index( FORBIDDEN_SIEGE );
        location2 = get_room_index( FORBIDDEN_SIEGE2 );
        location3 = get_room_index( FORBIDDEN_SIEGE3 );
        dcnum = 2;
    }

    if ( !str_cmp( ocity->name, "Paleon City" ) )
        acnum = 0;
    else if ( !str_cmp( ocity->name, "Dakar City" ) )
        acnum = 1;
    else if ( !str_cmp( ocity->name, "Forbidden City" ) )
        acnum = 2;

    if ( !dlocation )
        bug( "%s: defend location is NULL.", __FUNCTION__ );
    else {
        SET_BIT( dlocation->room_flags, ROOM_ARENA );
        SET_BIT( dlocation2->room_flags, ROOM_ARENA );
        SET_BIT( dlocation3->room_flags, ROOM_ARENA );

        count = dcity->dcatapult;
        if ( count > 0 ) {
            if ( dcnum == 0 )
                vnum = OBJ_VNUM_PCATAPULT;
            else if ( dcnum == 1 )
                vnum = OBJ_VNUM_DCATAPULT;
            else if ( dcnum == 2 )
                vnum = OBJ_VNUM_FCATAPULT;

            if ( !( oindex = get_obj_index( vnum ) ) || !( obj = create_object( oindex, 1 ) ) )
                bug( "%s: Couldn't create catapults %d.", __FUNCTION__, vnum );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, dlocation );
            }
        }

        count = dcity->guards;
        if ( count > 0 ) {
            for ( created = 0; created < count; created++ ) {
                mob = create_mobile( get_mob_index( MOB_VNUM_SOLDIERS ) );
                mob->color = 1;
                char_to_room( mob, dlocation2 );
            }
        }

        count = dcity->darchers;
        if ( count > 0 ) {
            for ( created = 0; created < count; created++ ) {
                mob = create_mobile( get_mob_index( MOB_VNUM_ARCHERS ) );
                mob->color = 1;
                char_to_room( mob, dlocation );
            }
        }

        count = dcity->dwarriors;
        if ( count > 0 ) {
            for ( created = 0; created < count; created++ ) {
                mob = create_mobile( get_mob_index( MOB_VNUM_SOLDIERS ) );
                mob->color = 1;
                char_to_room( mob, dlocation3 );
            }
        }

        count = dcity->pitch;
        if ( count > 0 ) {
            if ( dcnum == 0 )
                vnum = OBJ_VNUM_PPITCH;
            else if ( dcnum == 1 )
                vnum = OBJ_VNUM_DPITCH;
            else if ( dcnum == 2 )
                vnum = OBJ_VNUM_FPITCH;

            if ( !( oindex = get_obj_index( vnum ) ) || !( obj = create_object( oindex, 1 ) ) )
                bug( "%s: Couldn't create pitches %d.", __FUNCTION__, vnum );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, dlocation2 );
            }
        }

        count = dcity->arrows;
        if ( count > 0 ) {
            if ( dcnum == 0 )
                vnum = OBJ_VNUM_PARROWS;
            else if ( dcnum == 1 )
                vnum = OBJ_VNUM_DARROWS;
            else if ( dcnum == 2 )
                vnum = OBJ_VNUM_FARROWS;

            if ( !( oindex = get_obj_index( vnum ) ) || !( obj = create_object( oindex, 1 ) ) )
                bug( "%s: Couldn't create arrows %d.", __FUNCTION__, vnum );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, dlocation3 );
            }
        }
    }

    if ( !location )
        bug( "%s: siege location is NULL.", __FUNCTION__ );
    else {
        SET_BIT( location->room_flags, ROOM_ARENA );
        SET_BIT( location2->room_flags, ROOM_ARENA );
        SET_BIT( location3->room_flags, ROOM_ARENA );
        count = ocity->ocatapult;
        if ( count > 0 ) {
            if ( acnum == 0 )
                obj = create_object( get_obj_index( OBJ_VNUM_PCATAPULT ), 1 );
            else if ( acnum == 1 )
                obj = create_object( get_obj_index( OBJ_VNUM_DCATAPULT ), 1 );
            else if ( acnum == 2 )
                obj = create_object( get_obj_index( OBJ_VNUM_FCATAPULT ), 1 );
            if ( !obj )
                bug( "%s: Couldn't create catapults.", __FUNCTION__ );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, location );
            }
        }

        count = ocity->ballista;
        if ( count > 0 ) {
            if ( acnum == 0 )
                obj = create_object( get_obj_index( OBJ_VNUM_PBALLISTA ), 1 );
            else if ( acnum == 1 )
                obj = create_object( get_obj_index( OBJ_VNUM_DBALLISTA ), 1 );
            else if ( acnum == 2 )
                obj = create_object( get_obj_index( OBJ_VNUM_FBALLISTA ), 1 );
            if ( !obj )
                bug( "%s: Couldn't create ballistas.", __FUNCTION__ );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, location2 );
            }
        }

        count = ocity->ram;
        if ( count > 0 ) {
            if ( acnum == 0 )
                obj = create_object( get_obj_index( OBJ_VNUM_PRAM ), 1 );
            else if ( acnum == 1 )
                obj = create_object( get_obj_index( OBJ_VNUM_DRAM ), 1 );
            else if ( acnum == 2 )
                obj = create_object( get_obj_index( OBJ_VNUM_FRAM ), 1 );
            if ( !obj )
                bug( "%s: Couldn't create rams.", __FUNCTION__ );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, location2 );
            }
        }

        count = ocity->soldiers;
        if ( count > 0 ) {
            for ( created = 0; created < count; created++ ) {
                mob = create_mobile( get_mob_index( MOB_VNUM_SOLDIERS ) );
                mob->color = 1;
                char_to_room( mob, location3 );
            }
        }

        count = ocity->oarchers;
        if ( count > 0 ) {
            for ( created = 0; created < count; created++ ) {
                mob = create_mobile( get_mob_index( MOB_VNUM_ARCHERS ) );
                mob->color = 1;
                char_to_room( mob, location2 );
            }
        }
        count = ocity->owarriors;
        if ( count > 0 ) {
            for ( created = 0; created < count; created++ ) {
                mob = create_mobile( get_mob_index( MOB_VNUM_SOLDIERS ) );
                mob->color = 1;
                char_to_room( mob, location );
            }
        }

        count = ocity->platform;
        if ( count > 0 ) {
            if ( acnum == 0 )
                obj = create_object( get_obj_index( OBJ_VNUM_PPLATFORM ), 1 );
            else if ( acnum == 1 )
                obj = create_object( get_obj_index( OBJ_VNUM_DPLATFORM ), 1 );
            else if ( acnum == 2 )
                obj = create_object( get_obj_index( OBJ_VNUM_FPLATFORM ), 1 );
            if ( !obj )
                bug( "%s: Couldn't create platforms.", __FUNCTION__ );
            else {
                obj->color = 1;
                obj->count = count;
                obj_to_room( obj, location3 );
            }
        }
    }
}

void do_proclaim( CHAR_DATA *ch, char *argument )
{
    CITY_DATA              *city;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh ?\r\n", ch );
        return;
    }

    if ( !VLD_STR( argument ) ) {
        send_to_char( "Usage: proclaim <message>\r\n", ch );
        return;
    }

    city = get_city( ch->pcdata->city_name );

    if ( !city ) {
        send_to_char( "You aren't even a citizen of a city.\r\n", ch );
        return;
    }

    proc_chan( ch, argument );
}

void do_sabotage( CHAR_DATA *ch, char *argument )
{
    CITY_DATA              *city = NULL;
    OBJ_DATA               *obj;
    bool                    found = FALSE;
    short                   chance = number_range( 1, 10 );
    int                     ovnum;

    for ( obj = ch->in_room->first_content; obj; obj = obj->next_content ) {
        if ( obj->item_type == ITEM_SABOTAGE ) {
            found = TRUE;
            break;
        }
    }

    CHAR_DATA              *soldier = NULL;
    bool                    mfound = FALSE;

    for ( soldier = ch->in_room->first_person; soldier; soldier = soldier->next_in_room ) {
        if ( IS_NPC( soldier ) && soldier->pIndexData->vnum == MOB_VNUM_SOLDIERS ) {
            mfound = TRUE;
            break;
        }
    }
    if ( !found || !obj ) {
        send_to_char( "There must be a siege object present to sabotage.\r\n", ch );
        return;
    }

    separate_obj( obj );
    ovnum = obj->pIndexData->vnum;

    if ( ovnum == OBJ_VNUM_PCATAPULT || ovnum == OBJ_VNUM_PBALLISTA || ovnum == OBJ_VNUM_PRAM
         || ovnum == OBJ_VNUM_PPITCH || ovnum == OBJ_VNUM_PPLATFORM || ovnum == OBJ_VNUM_PARROWS )
        city = get_city( "Paleon City" );
    else if ( ovnum == OBJ_VNUM_DCATAPULT || ovnum == OBJ_VNUM_DBALLISTA || ovnum == OBJ_VNUM_DRAM
              || ovnum == OBJ_VNUM_DPITCH || ovnum == OBJ_VNUM_DPLATFORM
              || ovnum == OBJ_VNUM_DARROWS )
        city = get_city( "Dakar City" );
    else if ( ovnum == OBJ_VNUM_FCATAPULT || ovnum == OBJ_VNUM_FBALLISTA || ovnum == OBJ_VNUM_FRAM
              || ovnum == OBJ_VNUM_FPITCH || ovnum == OBJ_VNUM_FPLATFORM
              || ovnum == OBJ_VNUM_FARROWS )
        city = get_city( "Forbidden City" );

    if ( !city ) {
        bug( "%s: city is NULL for object vnum %d.", __FUNCTION__, ovnum );
        return;
    }

    WAIT_STATE( ch, 20 );

    if ( chance < 5 ) {
        ch_printf( ch, "&cYou attempt to sabotage %s, but fail to complete your task.\r\n",
                   obj->short_descr );
        if ( mfound ) {
            interpret( soldier, ( char * ) "mpecho" );
            interpret( soldier,
                       ( char * )
                       "mpecho &cA shout is taken up as a soldier observes the sabotage attempt!&D" );
            interpret( soldier, ( char * ) "mpecho" );
            soldier->level = ch->level - 4;
            soldier->hit = set_hp( soldier->level - 2 );
            soldier->max_hit = set_hp( soldier->level - 2 );
            soldier->armor = set_armor_class( soldier->level - 2 );
            soldier->hitroll = set_hitroll( soldier->level - 2 );
            soldier->damroll = set_damroll( soldier->level - 2 );
            soldier->numattacks = set_num_attacks( soldier->level - 2 );
            soldier->hitplus = set_hp( soldier->level - 2 );
            set_fighting( soldier, ch );
        }
        return;
    }
    ch_printf( ch, "&cYou attempt to sabotage %s.\r\nYou successfully destroyed %s.\r\n",
               obj->short_descr, obj->short_descr );
    if ( mfound ) {
        interpret( soldier, ( char * ) "mpecho" );
        interpret( soldier,
                   ( char * )
                   "mpecho &cA shout is taken up as a soldier observes the sabotage act!&D" );
        interpret( soldier, ( char * ) "mpecho" );
        soldier->level = ch->level - 4;
        soldier->hit = set_hp( soldier->level - 2 );
        soldier->max_hit = set_hp( soldier->level - 2 );
        soldier->armor = set_armor_class( soldier->level - 2 );
        soldier->hitroll = set_hitroll( soldier->level - 2 );
        soldier->damroll = set_damroll( soldier->level - 2 );
        soldier->numattacks = set_num_attacks( soldier->level - 2 );
        soldier->hitplus = set_hp( soldier->level - 2 );
        set_fighting( soldier, ch );
    }
    if ( ovnum == OBJ_VNUM_PCATAPULT || ovnum == OBJ_VNUM_DCATAPULT || ovnum == OBJ_VNUM_FCATAPULT ) {
        if ( city->ocatapult > 0 && city->offense > 9 ) {
            city->ocatapult -= 1;
            city->offense -= 10;
        }
    }
    else if ( ovnum == OBJ_VNUM_PBALLISTA || ovnum == OBJ_VNUM_DBALLISTA
              || ovnum == OBJ_VNUM_FBALLISTA ) {
        if ( city->ballista > 0 && city->offense > 9 ) {
            city->ballista -= 1;
            city->offense -= 10;
        }
    }
    else if ( ovnum == OBJ_VNUM_PRAM || ovnum == OBJ_VNUM_DRAM || ovnum == OBJ_VNUM_FRAM ) {
        if ( city->ram > 0 && city->offense > 24 ) {
            city->ram -= 1;
            city->offense -= 25;
        }
    }
    else if ( ovnum == OBJ_VNUM_PPITCH || ovnum == OBJ_VNUM_DPITCH || ovnum == OBJ_VNUM_FPITCH ) {
        if ( city->pitch > 0 && city->defense > 9 ) {
            city->pitch -= 1;
            city->defense -= 10;
        }
    }
    else if ( ovnum == OBJ_VNUM_PARROWS || ovnum == OBJ_VNUM_DARROWS || ovnum == OBJ_VNUM_FARROWS ) {
        if ( city->arrows > 0 && city->defense > 9 ) {
            city->arrows -= 1;
            city->defense -= 1;
        }
    }
    short                   rand;

    rand = number_range( 1, 10 );
    if ( rand > 6 ) {
        send_to_char( "&WYou gain one glory for sabotaging the enemy!\r\n", ch );
        ch->quest_curr += 1;
    }
    else {
        gain_exp( ch, 0 + ch->level * 1500 );
    }
    extract_obj( obj );
    save_city( city );
}

 /**************************************************************************
 * Mindcloud Copyright 2001-2003 by Jeff Boschee (Zarius),                 *
 * Additional credits are in the help file CODECREDITS                     *
 * All Rights Reserved.                                                    *
 ***************************************************************************/

/* globals */
char                   *vote_topic;
int                     vote[3];
int                     player_vote = 0;

void                    write_votes( char *topic, float yes, float no, int total );
void                    read_votes( void );

typedef struct vote_info VOTE;

struct vote_info
{
    VOTE                   *next;
    char                   *topic;
    float                   yes;
    float                   no;
    int                     total;
};

VOTE                   *first_vote;

void do_vote( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA        *d;
    VOTE                   *s_vote;
    char                    buf[MAX_STRING_LENGTH];
    int                     num = 0;
    int                     i = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char( "&cSyntax: vote < &Cfield1&c > <&C field2 &c>\r\n", ch );
            send_to_char( "&cfield1 being: show, yes, no, topic, player, cancel\r\n", ch );
            send_to_char( "&cfield2 being: seconds, on, off\r\n", ch );
        }

        if ( vote[0] ) {
            ch_printf( ch,
                       "The topic of the vote is [%s]\n\r"
                       "There are roughly %d seconds left to vote.\n\r", vote_topic, vote[0] );
            return;
        }
        send_to_char( "There is no vote going on at this time.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "show" ) ) {
        int                     count = 0;

        buf[0] = '\0';

        if ( arg2[0] == '\0' )
            num = 0;
        else
            num = atoi( arg2 );

        for ( s_vote = first_vote; s_vote; s_vote = s_vote->next ) {
            count++;
            if ( num != 0 && num < count )
                continue;

            ch_printf( ch,
                       "&D\n\r" "&w Vote Topic:&w %s\n\r" "&w Yes -&R %.2f%%\n\r"
                       "&w No -&R %.2f%%\n\r" "&w Total Votes -&R %d\n\r" "&D\n\r", s_vote->topic,
                       s_vote->yes, s_vote->no, ( int ) s_vote->total );
        }
        if ( !count )
            send_to_char( "There are no votes to display.\r\n", ch );

        return;
    }

    if ( !str_cmp( arg1, "topic" ) && ( IS_IMMORTAL( ch ) || player_vote ) ) {
        if ( vote[0] ) {
            send_to_char( "There is all ready a vote going on.\n\r", ch );
            return;
        }

        if ( !is_number( arg2 ) ) {
            send_to_char
                ( "Second argument must be the amount of time in seconds, increments of 5.\n\r",
                  ch );
            return;
        }

        if ( argument[0] == '\0' || strlen( argument ) < 5 ) {
            send_to_char( "You must supply a valid topic.\n\r", ch );
            return;
        }

        if ( atoi( arg2 ) % 5 != 0 ) {
            send_to_char( "Timer must be in increments of 5.\n\r", ch );
            return;
        }

        if ( atoi( arg2 ) > 300 ) {
            send_to_char( "Dont you think 5 minutes is long enough time to vote?\n\r", ch );
            return;
        }

        sprintf( buf, "&D\n\r"
                 "&R          %s has started a vote\n\r"
                 "&D\n\r" "&Y Topic:\n\r" "&w %s\n\r" "&D\n\r"
                 "&w     You have &R%d &wseconds to vote.\n\r" "&D\n\r"
                 "&w &PSyntax&D: &Cvote yes &D- &Cvote no\n\r", ch->name, argument, atoi( arg2 ) );
        do_echo( ch, buf );
        vote[0] = atoi( arg2 );
        vote[1] = 0;
        vote[2] = 0;
        vote[3] = 0;
        vote_topic = str_dup( argument );

        for ( d = first_descriptor; d; d = d->next ) {
            if ( d->connected != CON_PLAYING || !d->character )
                continue;
            d->character->pcdata->voted = 0;
        }
        return;
    }

    if ( IS_IMMORTAL( ch ) ) {
        if ( !str_cmp( arg1, "cancel" ) ) {
            vote[0] = 0;
            vote[1] = 0;
            vote[2] = 0;
            STRFREE( vote_topic );
            sprintf( buf, "Info-> The vote has been canceled by %s.\n\r", ch->name );
            do_echo( ch, buf );
            return;
        }

        if ( !str_cmp( arg1, "player" ) ) {
            if ( !str_cmp( arg2, "on" ) ) {
                player_vote = 1;
                send_to_char( "Players may start a vote topic now.\n\r", ch );
                return;
            }
            if ( !str_cmp( arg2, "off" ) ) {
                player_vote = 0;
                send_to_char( "Players may not start a vote topic now.\n\r", ch );
                return;
            }
            send_to_char( "Syntax: vote player <on/off>\n\r", ch );
            return;
        }
    }

    if ( !vote[0] ) {
        send_to_char( "There is no vote going on.\n\r", ch );
        return;
    }

    if ( ch->pcdata->voted ) {
        send_to_char( "You can only vote once.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "yes" ) ) {
        vote[1]++;
        ch->pcdata->voted = 1;
        send_to_char( "You vote yes.\n\r", ch );
        return;
    }

    if ( !str_cmp( arg1, "no" ) ) {
        vote[2]++;
        ch->pcdata->voted = 1;
        send_to_char( "You vote no.\n\r", ch );
        return;
    }
    send_to_char( "Vote yes or no.\n\r", ch );
}

void vote_update( void )
{
    char                    buf[MAX_STRING_LENGTH];
    float                   total,
                            x1,
                            x2;

    if ( !vote[0] )
        return;

    vote[0] -= 5;
    if ( vote[0] <= 0 ) {
        total = vote[1] + vote[2];
        x1 = vote[1] == 0 ? 0 : ( float ) vote[1] / total * 100;
        x2 = vote[2] == 0 ? 0 : ( float ) vote[2] / total * 100;

        sprintf( buf, "&wVote has ended for topic &D[&W%s&D]\n\r", vote_topic );
        announce( buf );
        sprintf( buf,
                 "&wVote outcome is &GYes&D[&Y%.2f%%&D] &RNo&D[&Y%.2f%%&D] &BTotal Votes&D[&Y%d&D]\n\r",
                 x1, x2, ( int ) total );
        announce( buf );
        write_votes( vote_topic, x1, x2, ( int ) total );
        vote[0] = 0;
    }
}

void write_votes( char *topic, float yes, float no, int total )
{
    VOTE                   *svote;
    FILE                   *fpout;
    char                    filename[256];

    snprintf( filename, 256, "%s", VOTE_FILE );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "FATAL: cannot open %s for writing!\r\n", filename );
        perror( filename );
        return;
    }

    CREATE( svote, VOTE, 1 );
    svote->topic = topic;
    svote->yes = yes;
    svote->no = no;
    svote->total = total;
    svote->next = first_vote;
    first_vote = svote;

    for ( svote = first_vote; svote; svote = svote->next )
        fprintf( fpout, "vote\n%s~ %d %d %d\n", svote->topic, ( int ) svote->yes, ( int ) svote->no,
                 ( int ) svote->total );

    fprintf( fpout, "$\n" );
    FileClose( fpout );
}

void read_votes( void )
{
    FILE                   *fp;
    VOTE                   *svote;
    char                   *word;

    if ( ( fp = FileOpen( VOTE_FILE, "r" ) ) == NULL ) {
        bug( "read_votes: unable to open file ro reading!" );
        return;
    }

    while ( 1 ) {
        word = fread_word( fp );

        if ( !str_cmp( word, "end" ) )
            break;

        if ( !str_cmp( word, "vote" ) ) {
            CREATE( svote, VOTE, 1 );
            svote->topic = fread_string( fp );
            svote->yes = fread_number( fp );
            svote->no = fread_number( fp );
            svote->total = fread_number( fp );
            svote->next = first_vote;
            first_vote = svote;
            continue;
        }

        bug( "read_votes: wrong word!" );
        break;
    }
}

// City Rosters?

void add_rollcall( CITY_DATA * city, char *name, int Class, int level, int kills, int deaths )
{
    ROLLCALL_DATA          *rollcall;

    CREATE( rollcall, ROLLCALL_DATA, 1 );
    rollcall->name = STRALLOC( name );
    rollcall->Class = Class;
    rollcall->level = level;
    rollcall->kills = kills;
    rollcall->deaths = deaths;
    rollcall->joined = current_time;
    rollcall->lastupdated = current_time;
    insert_rollcall( city, rollcall );
}

void remove_rollcall( CITY_DATA * city, char *name )
{
    ROLLCALL_DATA          *rollcall,
                           *roll_next;

    if ( !city || !name || name[0] == '\0' )
        return;

    if ( VLD_STR( city->duke ) && !str_cmp( city->duke, name ) )
        STRFREE( city->duke );
    if ( VLD_STR( city->baron ) && !str_cmp( city->baron, name ) )
        STRFREE( city->baron );
    if ( VLD_STR( city->captain ) && !str_cmp( city->captain, name ) )
        STRFREE( city->captain );
    if ( VLD_STR( city->sheriff ) && !str_cmp( city->sheriff, name ) )
        STRFREE( city->sheriff );
    if ( VLD_STR( city->knight ) && !str_cmp( city->knight, name ) )
        STRFREE( city->knight );

    for ( rollcall = city->first_citizen; rollcall; rollcall = roll_next ) {
        roll_next = rollcall->next;

        if ( !str_cmp( name, rollcall->name ) ) {
            STRFREE( rollcall->name );
            UNLINK( rollcall, city->first_citizen, city->last_citizen, next, prev );
            DISPOSE( rollcall );
        }
    }

    fix_city_order( city );
}

/* Remove this one from any and all roll calls */
void remove_from_rollcalls( char *name )
{
    CITY_DATA              *city;

    if ( !VLD_STR( name ) )
        return;

    for ( city = first_city; city; city = city->next ) {
        remove_rollcall( city, name );
        save_city( city );
    }
}

void update_rollcall( CHAR_DATA *ch )
{
    CITY_DATA              *city;
    ROLLCALL_DATA          *rollcall;

    if ( !ch || !ch->pcdata || !( city = ch->pcdata->city ) )
        return;

    for ( rollcall = ch->pcdata->city->first_citizen; rollcall; rollcall = rollcall->next ) {
        if ( !str_cmp( ch->name, rollcall->name ) ) {
            rollcall->level = ch->level;
            rollcall->kills = ch->pcdata->mkills;
            rollcall->deaths = ch->pcdata->mdeaths;
            rollcall->lastupdated = current_time;
            UNLINK( rollcall, city->first_citizen, city->last_citizen, next, prev );
            insert_rollcall( city, rollcall );
            save_city( ch->pcdata->city );
            return;
        }
    }
    /*
     * If we make it here, assume they haven't been added previously 
     */
    add_rollcall( ch->pcdata->city, ch->name, ch->Class, ch->level, ch->pcdata->mkills,
                  ch->pcdata->mdeaths );
    save_city( ch->pcdata->city );
    return;
}

/* For use during city removal and memory cleanup */
void remove_all_rollcalls( CITY_DATA * city )
{
    ROLLCALL_DATA          *rollcall,
                           *rollcall_next;

    for ( rollcall = city->first_citizen; rollcall; rollcall = rollcall_next ) {
        rollcall_next = rollcall->next;

        STRFREE( rollcall->name );
        UNLINK( rollcall, city->first_citizen, city->last_citizen, next, prev );
        DISPOSE( rollcall );
    }
}

void do_rollcall( CHAR_DATA *ch, char *argument )
{
    CITY_DATA              *city;
    ROLLCALL_DATA          *rollcall;
    char                    arg[MAX_INPUT_LENGTH],
                            arg2[MAX_INPUT_LENGTH];
    int                     total = 0;

    if ( IS_NPC( ch ) ) {
        send_to_char( "NPCs can't use this command.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Usage: rollcall <cityname>\r\n", ch );
        if ( IS_IMMORTAL( ch ) || IS_DUKE( ch ) || IS_BARON( ch ) )
            send_to_char( "Usage: rollcall <cityname> remove <name>\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !( city = get_city( arg ) ) ) {
        ch_printf( ch, "No such city known as %s\r\n", arg );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        ch_printf( ch, "Citizenship rollcall for the %s\r\n\r\n", city->name );
        ch_printf( ch, "%-15.15s  %-15.15s %-6.6s %-6.6s %-6.6s %s\r\n", "Name", "Class", "Level",
                   "Kills", "Deaths", "Joined on" );
        send_to_char
            ( "-------------------------------------------------------------------------------------\r\n",
              ch );
        for ( rollcall = city->first_citizen; rollcall; rollcall = rollcall->next ) {
            if ( rollcall->level > 100 )
                continue;

            ch_printf( ch, "%-15.15s  %-15.15s %-6d %-6d %-6d %s", rollcall->name,
                       capitalize( npc_class[rollcall->Class] ), rollcall->level, rollcall->kills,
                       rollcall->deaths, ctime( &rollcall->joined ) );
            total++;
        }
        ch_printf( ch, "\r\nThere are %d citizen%s in %s\r\n", total, total == 1 ? "" : "s",
                   city->name );
        return;
    }

    if ( IS_IMMORTAL( ch ) || ( city->duke && !str_cmp( ch->name, city->duke ) )
         || ( city->baron && !str_cmp( ch->name, city->baron ) ) ) {
        argument = one_argument( argument, arg2 );
        if ( !str_cmp( arg2, "remove" ) ) {
            if ( !argument || argument[0] == '\0' ) {
                send_to_char( "Remove who from the rollcall?\r\n", ch );
                return;
            }
            remove_rollcall( city, argument );
            save_city( city );
            ch_printf( ch, "%s has been removed from the rollcall for %s\r\n", argument,
                       city->name );
            return;
        }
    }
    do_rollcall( ch, ( char * ) "" );
}

void fread_citizenlist( CITY_DATA * city, FILE * fp )
{
    ROLLCALL_DATA          *rollcall;
    const char             *word;
    bool                    fMatch;

    CREATE( rollcall, ROLLCALL_DATA, 1 );
    rollcall->lastupdated = current_time;
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'C':
                if ( !str_cmp( word, "Class" ) ) {
                    char                   *temp = fread_string( fp );
                    int                     Class = get_npc_class( temp );

                    if ( Class < 0 || Class >= MAX_NPC_CLASS ) {
                        bug( "%s: Invalid class in city rollcall", __FUNCTION__ );
                        Class = get_npc_class( ( char * ) "warrior" );
                    }
                    STRFREE( temp );
                    rollcall->Class = Class;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'D':
                KEY( "Deaths", rollcall->deaths, fread_number( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    LINK( rollcall, city->first_citizen, city->last_citizen, next, prev );
                    return;
                }
                break;

            case 'J':
                KEY( "Joined", rollcall->joined, fread_number( fp ) );
                break;

            case 'K':
                KEY( "Kills", rollcall->kills, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", rollcall->level, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", rollcall->name, fread_string( fp ) );
                break;

            case 'U':
                KEY( "Updated", rollcall->lastupdated, fread_number( fp ) );
                break;

        }
        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }
}

/* This is used to update the city positions if one becomes empty */
void fix_city_order( CITY_DATA * city )
{
    char                    buf[MSL];

    if ( !city )
        return;

    if ( !VLD_STR( city->duke ) ) {
        if ( city->duke )
            STRFREE( city->duke );
        if ( VLD_STR( city->baron ) ) {
            city->duke = STRALLOC( city->baron );
            city->baron = NULL;
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Duke for %s!",
                      city->duke, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
        else if ( VLD_STR( city->captain ) ) {
            city->duke = STRALLOC( city->captain );
            city->captain = NULL;
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Duke for %s!",
                      city->duke, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
        else if ( VLD_STR( city->sheriff ) ) {
            city->duke = STRALLOC( city->sheriff );
            city->sheriff = NULL;
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Duke for %s!",
                      city->duke, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
        else if ( VLD_STR( city->knight ) ) {
            city->duke = STRALLOC( city->knight );
            STRFREE( city->knight );
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Duke for %s!",
                      city->duke, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
    }

    if ( !VLD_STR( city->baron ) ) {
        if ( city->baron )
            STRFREE( city->baron );
        if ( VLD_STR( city->captain ) ) {
            city->baron = STRALLOC( city->captain );
            city->captain = NULL;
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Baron for %s!",
                      city->baron, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
        else if ( VLD_STR( city->sheriff ) ) {
            city->baron = STRALLOC( city->sheriff );
            city->sheriff = NULL;
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Baron for %s!",
                      city->baron, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
        else if ( VLD_STR( city->knight ) ) {
            city->baron = STRALLOC( city->knight );
            STRFREE( city->knight );
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Baron for %s!",
                      city->baron, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
    }

    if ( !VLD_STR( city->captain ) ) {
        if ( city->captain )
            STRFREE( city->captain );
        if ( VLD_STR( city->sheriff ) ) {
            city->captain = STRALLOC( city->sheriff );
            city->sheriff = NULL;
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Captain for %s!",
                      city->captain, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
        else if ( VLD_STR( city->knight ) ) {
            city->captain = STRALLOC( city->knight );
            STRFREE( city->knight );
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Captain for %s!",
                      city->captain, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
    }

    if ( !VLD_STR( city->sheriff ) ) {
        if ( city->sheriff )
            STRFREE( city->sheriff );
        if ( VLD_STR( city->knight ) ) {
            city->sheriff = STRALLOC( city->knight );
            STRFREE( city->knight );
            snprintf( buf, MIL,
                      "&W[&RAnnouncement&W]&C %s has been awarded the position of Sheriff for %s!",
                      city->sheriff, city->name );
            echo_to_all( AT_RED, buf, ECHOTAR_ALL );
        }
    }
}
