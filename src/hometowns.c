 /*
  * Yet another attempt to write a decent hometown code - Vladaar 
  */

#include "h/mud.h"
#include "h/files.h"
#include "h/hometowns.h"
#include "h/key.h"

HTOWN_DATA             *first_htown;
HTOWN_DATA             *last_htown;

/* local routines */
void                    fread_htown( HTOWN_DATA * htown, FILE * fp );
extern bool             load_htown_file( const char *htownfile );
void                    write_htown_list( void );

HTOWN_DATA             *get_htown( const char *name )
{
    HTOWN_DATA             *htown;

    for ( htown = first_htown; htown; htown = htown->next )
        if ( !str_cmp( name, htown->name ) )
            return htown;
    return NULL;
}

void write_htown_list( void )
{
    HTOWN_DATA             *thtown;
    FILE                   *fpout;
    char                    filename[256];

    snprintf( filename, 256, "%s%s", HTOWN_DIR, HTOWN_LIST );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "FATAL: cannot open %s for writing!\r\n", filename );
        perror( filename );
        return;
    }
    for ( thtown = first_htown; thtown; thtown = thtown->next )
        fprintf( fpout, "%s\n", thtown->filename );
    fprintf( fpout, "$\n" );
    FileClose( fpout );
    return;
}

/* Save a htown's data to its data file */
void save_htown( HTOWN_DATA * htown )
{
    FILE                   *fp;
    char                    filename[256];

    if ( !htown ) {
        bug( "%s", "save_htown: null htown pointer!" );
        return;
    }
    if ( !VLD_STR( htown->filename ) ) {
        bug( "save_htown: %s has no filename", htown->name );
        return;
    }
    snprintf( filename, 256, "%s%s", HTOWN_DIR, htown->filename );
    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "save_htown: can't open %s for writing.", filename );
        perror( filename );
    }
    else {
        fprintf( fp, "#HTOWN\n" );
        if ( VLD_STR( htown->name ) )
            fprintf( fp, "Name        %s~\n", htown->name );
        if ( VLD_STR( htown->filename ) )
            fprintf( fp, "Filename    %s~\n", htown->filename );
        if ( VLD_STR( htown->description ) )
            fprintf( fp, "Description %s~\n", htown->description );
        if ( VLD_STR( htown->ruler ) )
            fprintf( fp, "Ruler       %s~\n", htown->ruler );
        if ( VLD_STR( htown->general ) )
            fprintf( fp, "General     %s~\n", htown->general );
        if ( VLD_STR( htown->nation ) )
            fprintf( fp, "Nation      %s~\n", htown->nation );
        fprintf( fp, "Members     %d\n", htown->members );
        fprintf( fp, "Temple      %d\n", htown->temple );
        fprintf( fp, "Startroom   %d\n", htown->startroom );
        fprintf( fp, "Recall      %d\n", htown->recall );
        if ( VLD_STR( htown->race ) )
            fprintf( fp, "Race        %s~\n", htown->race );
        fprintf( fp, "End\n\n" );
        fprintf( fp, "#END\n" );
    }
    FileClose( fp );
    return;
}

/* Read in actual htown data. */
void fread_htown( HTOWN_DATA * htown, FILE * fp )
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
            case 'D':
                KEY( "Description", htown->description, fread_string( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return;
                break;
            case 'F':
                KEY( "Filename", htown->filename, fread_string( fp ) );
                break;
            case 'G':
                KEY( "General", htown->general, fread_string( fp ) );
                break;
            case 'M':
                KEY( "Members", htown->members, fread_number( fp ) );
                break;
            case 'N':
                KEY( "Name", htown->name, fread_string( fp ) );
                KEY( "Nation", htown->nation, fread_string( fp ) );
                break;
            case 'R':
                KEY( "Race", htown->race, fread_string( fp ) );
                KEY( "Recall", htown->recall, fread_number( fp ) );
                KEY( "Ruler", htown->ruler, fread_string( fp ) );
                break;
            case 'S':
                KEY( "Startroom", htown->startroom, fread_number( fp ) );
            case 'T':
                KEY( "Temple", htown->temple, fread_number( fp ) );
        }
        if ( !fMatch ) {
            bug( "Fread_htown: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

/* Load a htown file */
bool load_htown_file( const char *htownfile )
{
    char                    filename[256];
    HTOWN_DATA             *htown;
    FILE                   *fp;
    bool                    found;

    CREATE( htown, HTOWN_DATA, 1 );
    found = FALSE;
    snprintf( filename, 256, "%s%s", HTOWN_DIR, htownfile );
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
                bug( "%s", "Load_htown_file: # not found." );
                break;
            }
            word = fread_word( fp );
            if ( !str_cmp( word, "HTOWN" ) ) {
                fread_htown( htown, fp );
                break;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_htown_file: bad section." );
                break;
            }
        }
        FileClose( fp );
    }
    if ( found )
        LINK( htown, first_htown, last_htown, next, prev );
    else
        DISPOSE( htown );
    return found;
}

/* Load in all the htown files. */
void load_htowns(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    htownlist[256];

    first_htown = NULL;
    last_htown = NULL;
    log_string( "Loading htowns..." );
    snprintf( htownlist, 256, "%s%s", HTOWN_DIR, HTOWN_LIST );
    if ( ( fpList = FileOpen( htownlist, "r" ) ) == NULL ) {
        bug( "load_htowns: can't open %s for reading.", htownlist );
        perror( htownlist );
        exit( 1 );
    }
    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        log_string( filename );
        if ( filename[0] == '$' )
            break;
        if ( !load_htown_file( filename ) )
            bug( "Cannot load htown file: %s", filename );
    }
    FileClose( fpList );
    log_string( " Done htowns " );
    return;
}

void do_htown_outcast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    HTOWN_DATA             *htown;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    htown = ch->pcdata->htown;
    if ( ( htown->ruler == NULL || str_cmp( ch->name, htown->ruler ) )
         && ( htown->general == NULL || str_cmp( ch->name, htown->general ) ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_char( "Outcast whom from your htown?\r\n", ch );
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
    if ( victim == ch ) {
        send_to_char( "Kick yourself out of your own htown?\r\n", ch );
        return;
    }
    if ( victim->pcdata->htown != ch->pcdata->htown ) {
        send_to_char( "This player does not belong to your htown!\r\n", ch );
        return;
    }
    --htown->members;
    victim->pcdata->htown = NULL;
    if ( VLD_STR( victim->pcdata->htown_name ) )
        STRFREE( victim->pcdata->htown_name );
    act( AT_MAGIC, "You outcast $N from $t", ch, htown->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n outcasts $N from $t", ch, htown->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n outcasts you from $t", ch, htown->name, victim, TO_VICT );
    save_char_obj( victim );
    save_htown( htown );
    return;
}

void do_sethtown( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    HTOWN_DATA             *htown;

    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Usage: sethtown <htown> <field> <player>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " ruler general members recall race desc\r\n", ch );
        send_to_char( " startroom temple name filename nation\r\n", ch );
        return;
    }
    htown = get_htown( arg1 );
    if ( !htown ) {
        send_to_char( "No such htown.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "ruler" ) ) {
        if ( VLD_STR( htown->ruler ) )
            STRFREE( htown->ruler );
        if ( VLD_STR( argument ) )
            htown->ruler = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "general" ) ) {
        if ( VLD_STR( htown->general ) )
            STRFREE( htown->general );
        if ( VLD_STR( argument ) )
            htown->general = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "nation" ) ) {
        if ( VLD_STR( htown->nation ) )
            STRFREE( htown->nation );
        if ( VLD_STR( argument ) )
            htown->nation = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "members" ) )
        htown->members = atoi( argument );
    else if ( !str_cmp( arg2, "recall" ) )
        htown->recall = atoi( argument );
    else if ( !str_cmp( arg2, "startroom" ) )
        htown->startroom = atoi( argument );
    else if ( !str_cmp( arg2, "temple" ) )
        htown->temple = atoi( argument );
    else if ( get_trust( ch ) < LEVEL_IMMORTAL ) {
        do_sethtown( ch, ( char * ) "" );
        return;
    }
    else if ( !str_cmp( arg2, "name" ) ) {
        if ( VLD_STR( htown->name ) )
            STRFREE( htown->name );
        if ( VLD_STR( argument ) )
            htown->name = STRALLOC( argument );
    }
    else if ( !str_cmp( arg2, "filename" ) ) {
        if ( VLD_STR( htown->filename ) )
            STRFREE( htown->filename );
        if ( VLD_STR( argument ) )
            htown->filename = STRALLOC( argument );
        write_htown_list(  );
    }
    else if ( !str_cmp( arg2, "desc" ) ) {
        if ( VLD_STR( htown->description ) )
            STRFREE( htown->description );
        if ( VLD_STR( argument ) )
            htown->description = STRALLOC( argument );
    }
    else if ( get_trust( ch ) < LEVEL_AJ_CPL ) {
        do_sethtown( ch, ( char * ) "" );
        return;
    }
    else if ( !str_cmp( arg2, "race" ) ) {
        if ( VLD_STR( htown->race ) )
            STRFREE( htown->race );
        if ( VLD_STR( argument ) )
            htown->race = STRALLOC( argument );
    }
    else {
        do_sethtown( ch, ( char * ) "" );
        return;
    }
    send_to_char( "Done.\r\n", ch );
    save_htown( htown );
    return;
}

void do_showhtown( CHAR_DATA *ch, char *argument )
{
    HTOWN_DATA             *htown;

    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Usage: showhtown <htown>\r\n", ch );
        return;
    }
    htown = get_htown( argument );
    if ( !htown ) {
        send_to_char( "No such htown.\r\n", ch );
        return;
    }
    ch_printf( ch, "\r\n&wHometown: &W%s\r\n&wFilename:  &W%s\r\n", htown->name, htown->filename );
    ch_printf( ch, "&wNation:       &W%s\r\n", htown->nation );
    ch_printf( ch, "&wRuler:        &W%s\r\n", htown->ruler );
    ch_printf( ch, "&wGeneral:      &W%s\r\n", htown->general );
    ch_printf( ch, "&wMembers:      &W%-d\r\n", htown->members );
    ch_printf( ch, "&wRecall:       &W%-5d\r\n&wRace:         &W%s\r\n", htown->recall,
               capitalize( htown->race ) );
    ch_printf( ch, "&wStartroom:    &W%-5d\r\n&wTemple:       &W%-5d\r\n", htown->startroom,
               htown->temple );
    ch_printf( ch, "&wDescription:\r\n&W%s\r\n", htown->description );
    return;
}

void do_makehtown( CHAR_DATA *ch, char *argument )
{
    char                    filename[256];
    HTOWN_DATA             *htown;
    bool                    found;

    set_char_color( AT_IMMORT, ch );
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Usage: makehtown <htown name>\r\n", ch );
        return;
    }
    found = FALSE;
    snprintf( filename, 256, "%s%s", HTOWN_DIR, strlower( argument ) );
    CREATE( htown, HTOWN_DATA, 1 );
    LINK( htown, first_htown, last_htown, next, prev );
    htown->name = STRALLOC( argument );
    send_to_char( "htown now made.\r\n", ch );
}

void do_htowns( CHAR_DATA *ch, char *argument )
{
    HTOWN_DATA             *htown;

    set_char_color( AT_CYAN, ch );
    if ( !first_htown ) {
        send_to_char( "There are no htowns currently formed.\r\n", ch );
        return;
    }
    if ( !VLD_STR( argument ) ) {
        send_to_char( "\r\n&WCurrent Home Towns Available in the &R6 Dragons &Wrealms\r\n", ch );
        send_to_char
            ( "&c----------------------------------------------------------------------------\r\n",
              ch );
        send_to_char
            ( "\r\n&cHome Town                Nation               Ruler             General\r\n",
              ch );
        for ( htown = first_htown; htown; htown = htown->next )
            ch_printf( ch, "&C%-24s %-20s %-17s %-12s\r\n", htown->name, htown->nation,
                       htown->ruler, htown->general );
        send_to_char
            ( "\r\n&cUse '&Whtowns &c<&Wname of htown&c>' for more detailed information.\r\n", ch );
        return;
    }
    htown = get_htown( argument );
    if ( !htown ) {
        send_to_char( "&cNo such htown exists...\r\n", ch );
        return;
    }
    ch_printf( ch, "&c\r\n%s\r\n", htown->name );
    ch_printf( ch,
               "&cRuler:     &w%s\r\n&cGeneral:   &w%s\r\n&cRace:       &w%s\r\n&cNation   &w%s\r\n&cMembers:   &w%d\r\n",
               htown->ruler, htown->general, capitalize( htown->race ), htown->nation,
               htown->members );
    ch_printf( ch, "&cDescription:\r\n&w%s\r\n", htown->description );
    return;
}
