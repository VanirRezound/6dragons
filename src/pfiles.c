 /****************************************************************************
 * AFKMud Copyright 1997-2002 Alsherok. Contributors: Samson, Dwip, Whir,   *
 * Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.                        *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                          Pfile Pruning Module                            *
 ****************************************************************************/

#include <ctype.h>
#include <string.h>
#if !defined(WIN32)
#include <dirent.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/pfiles.h"
#include "h/key.h"

/* Globals */
time_t                  pfile_time;
HOUR_MIN_SEC            set_pfile_time_struct;
HOUR_MIN_SEC           *set_pfile_time;
struct tm              *new_pfile_time;
struct tm               new_pfile_struct;
time_t                  new_pfile_time_t;
short                   num_pfiles;                    /* Count up number of pfiles */
bool                    exists_file( char *name );
void                    remove_from_rollcalls( char *name );
void                    remove_from_rosters( char *name );

void remove_oldest_pfile_backup( void )
{
    char                    buf[MSL],
                            oldestname[MSL];
    DIR                    *dp;
    struct dirent          *de;
    struct stat             fst;
    time_t                  oldesttime = current_time;

    if ( !( dp = opendir( BACKUP_DIR ) ) ) {
        bug( "%s: can't open %s", __FUNCTION__, BACKUP_DIR );
        perror( BACKUP_DIR );
        return;
    }

    oldestname[0] = '\0';
    /*
     * Ok have the directory open so lets check the files and the time and remove the
     * oldest one 
     */
    while ( ( de = readdir( dp ) ) ) {
        if ( de->d_name[0] == '.' )
            continue;

        snprintf( buf, sizeof( buf ), "%s%s", BACKUP_DIR, de->d_name );
        if ( stat( buf, &fst ) == -1 )
            continue;

        if ( oldesttime > fst.st_mtime ) {             /* The oldest has the lowest time */
            oldesttime = fst.st_mtime;
            snprintf( oldestname, sizeof( oldestname ), "%s%s", BACKUP_DIR, de->d_name );
        }
    }
    closedir( dp );
    dp = NULL;

    if ( VLD_STR( oldestname ) && !remove( oldestname ) )
        log_string( "%s: %s has been deleted to keep the pfile backups down.", __FUNCTION__,
                    oldestname );
}

void init_pfile_scan_time( void )
{
    /*
     * Init pfile scan time. 
     */
    set_pfile_time = &set_pfile_time_struct;
    new_pfile_time = update_time( localtime( &current_time ) );
    /*
     * Copies *new_pfile_time to new_pfile_struct, and then points
     * new_pfile_time to new_pfile_struct again. -- Alty 
     */
    new_pfile_struct = *new_pfile_time;
    new_pfile_time = &new_pfile_struct;
    new_pfile_time->tm_mday += 1;
    if ( new_pfile_time->tm_hour > 12 )
        new_pfile_time->tm_mday += 1;
    new_pfile_time->tm_sec = 0;
    new_pfile_time->tm_min = 0;
    new_pfile_time->tm_hour = 3;
    /*
     * Update new_pfile_time (due to day increment) 
     */
    new_pfile_time = update_time( new_pfile_time );
    new_pfile_struct = *new_pfile_time;
    new_pfile_time = &new_pfile_struct;
    /*
     * Bug fix submitted by Gabe Yoder 
     */
    new_pfile_time_t = mktime( new_pfile_time );
    /*
     * check_pfiles(mktime(new_pfile_time)); 
     */
    if ( !load_timedata(  ) )
        log_string( "Pfile scan time reset to default time of 3am." );
}

time_t                  now_time;
short                   deleted = 0;
short                   days = 0;

void fread_pfile( FILE * fp, time_t tdiff, char *fname, bool count )
{
    const char             *word;
    char                   *name = NULL;
    char                   *clan = NULL;
    short                   level = 0;
    short                   file_ver = 0;
    EXT_BV                  pact;
    bool                    fMatch;
    struct dirent          *de,
                           *ep;

    xCLEAR_BITS( pact );
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                KEY( "Act", pact, fread_bitvector( fp ) );
                break;
            case 'B':
                if ( !strcmp( word, "Bio" ) ) {
                    fread_flagstring( fp );
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'C':
                KEY( "Clan", clan, fread_string( fp ) );
                break;
            case 'E':
                if ( !strcmp( word, "End" ) )
                    goto timecheck;
                break;
            case 'L':
                KEY( "Level", level, fread_number( fp ) );
                break;
            case 'N':
                KEY( "Name", name, fread_string( fp ) );
                break;
            case 'V':
                KEY( "Version", file_ver, fread_number( fp ) );
                break;
        }
        if ( !fMatch )
            fread_to_eol( fp );
    }

  timecheck:
    if ( count == FALSE && !xIS_SET( pact, PLR_EXEMPT ) ) {
        days = sysdata.regular_purge;
        if ( level < 2 )
            days = 1;
        else if ( level < 50 )
            days = sysdata.newbie_purge;

        if ( level < LEVEL_IMMORTAL && tdiff > days ) {
            if ( unlink( fname ) == -1 )
                perror( "Unlink" );
            else {
                snprintf( fname, sizeof( fname ), "%s%s", VAULT_DIR, name );
                if ( !remove( fname ) )
                    log_string( "Player's vault data destroyed.\r\n" );

                snprintf( fname, sizeof( fname ), "%s%c/%s.lua", PLAYER_DIR, tolower( name[0] ),
                          name );
                if ( !remove( fname ) )
                    log_string( "Player's lua data destroyed.\r\n" );

                log_printf( "Player %s was deleted. Exceeded time limit of %d[%ld] days.", name,
                            days, tdiff );
#ifdef AUTO_AUTH
                remove_from_auth( name );
#endif
                remove_from_rosters( name );
                remove_from_rollcalls( name );
                deleted++;
            }
        }
    }
    if ( clan != NULL ) {
        CLAN_DATA              *c = get_clan( clan );

        if ( !c )
            return;
        c->members++;
        save_clan( c );
    }
    if ( VLD_STR( name ) )
        STRFREE( name );
    if ( VLD_STR( clan ) )
        STRFREE( clan );
}

void read_pfile( char *dirname, char *filename, bool count )
{
    FILE                   *fp;
    char                    fname[MSL];
    struct stat             fst;
    time_t                  tdiff;

    now_time = time( 0 );

    snprintf( fname, MSL, "%s/%s", dirname, filename );
    if ( stat( fname, &fst ) != -1 ) {
        tdiff = ( now_time - fst.st_mtime ) / 86400;

        if ( ( fp = FileOpen( fname, "r" ) ) != NULL ) {
            for ( ;; ) {
                char                    letter;
                const char             *word;

                letter = fread_letter( fp );
                if ( ( letter != '#' ) && ( !feof( fp ) ) )
                    continue;
                word = feof( fp ) ? "End" : fread_word( fp );
                if ( !str_cmp( word, "End" ) )
                    break;
                if ( !str_cmp( word, "PLAYER" ) )
                    fread_pfile( fp, tdiff, fname, count );
                else if ( !str_cmp( word, "END" ) )    /* Done */
                    break;
            }
            FileClose( fp );
        }
    }
}

/* Since there are alot of unneeded lua files tossed this in to get rid of unneeded ones */
void check_lua( char *dirname, char *filename )
{
    char                    fname[MSL];
    struct stat             fst;
    time_t                  tdiff;
    int                     x;

    /*
     * Check for the pfile 
     */
    snprintf( fname, sizeof( fname ), "%s/%s", dirname, filename );

    /*
     * Lets cut off the .lua 
     */
    x = strlen( fname );
    x -= 3;
    snprintf( fname, x, "%s/%s", dirname, filename );

    /*
     * Pfile exist 
     */
    if ( stat( fname, &fst ) != -1 )
        return;

    /*
     * Doesn't exist 
     */
    snprintf( fname, sizeof( fname ), "%s/%s", dirname, filename );
    if ( !remove( fname ) )
        log_printf( "%s lua data destroyed.", fname );
}

void pfile_scan( bool count )
{
    DIR                    *dp;
    struct dirent          *dentry;
    CLAN_DATA              *clan;
    char                    directory_name[100];
    short                   alpha_loop;
    short                   cou = 0;

    deleted = 0;

    now_time = time( 0 );

#if !defined(WIN32)
//  nice(20);
#endif

    /*
     * Reset all clans to 0 members prior to scan - Samson 7-26-00 
     */
    if ( !count )
        for ( clan = first_clan; clan; clan = clan->next )
            clan->members = 0;

    for ( alpha_loop = 0; alpha_loop <= 25; alpha_loop++ ) {
        snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
        /*
         * log_string(directory_name); 
         */
        dp = opendir( directory_name );
        dentry = readdir( dp );
        while ( dentry ) {
            /*
             * Added by Tarl 3 Dec 02 because we are now using CVS 
             */
            if ( !str_cmp( dentry->d_name, "CVS" ) ) {
                dentry = readdir( dp );
                continue;
            }
            if ( !str_suffix( ".lua", dentry->d_name ) ) {
                check_lua( directory_name, dentry->d_name );
                dentry = readdir( dp );
                continue;
            }
            if ( dentry->d_name[0] != '.' ) {
                if ( !count )
                    read_pfile( directory_name, dentry->d_name, count );
                cou++;
            }
            dentry = readdir( dp );
        }
        closedir( dp );
    }
    if ( !count )
        log_string( "Pfile cleanup completed." );
    else
        log_string( "Pfile count completed." );
    log_printf( "Total pfiles scanned: %d", cou );
    if ( !count ) {
        log_printf( "Total pfiles deleted: %d", deleted );
        log_printf( "Total pfiles remaining: %d", cou - deleted );
        num_pfiles = cou - deleted;
    }
    else
        num_pfiles = cou;
}

void save_pfiles( void )
{
    FILE                   *error_log;
    char                    buf[MIL],
                            buf2[MIL];
    int                     logindex,
                            count = 0,
        nlogindex;

    for ( logindex = 1; logindex <= 11; logindex++ ) {
        snprintf( buf, MIL, "%s%dpfiles.tgz", BACKUP_DIR, logindex );
        if ( exists_file( buf ) )
            count++;
    }

    if ( count >= 10 )
        remove_oldest_pfile_backup(  );

    for ( logindex = 1;; logindex++ ) {
        snprintf( buf, MIL, "%s%dpfiles.tgz", BACKUP_DIR, logindex );
        if ( exists_file( buf ) )
            continue;
        break;
    }

    /*
     * Ok we have an empty log to use so use it 
     */
    snprintf( buf, sizeof( buf ), "tar -czf %s%dpfiles.tgz %s", BACKUP_DIR, logindex, PLAYER_DIR );
//  system(buf);
}

void do_pfiles( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobs cannot use this command!\r\n", ch );
        return;
    }
    if ( !VLD_STR( argument ) ) {
        save_pfiles(  );

        log_printf( "Manual pfile cleanup started by %s.", ch->name );
        pfile_scan( FALSE );
#ifdef SAMSONRENT
        rent_update(  );
#endif
        return;
    }

    if ( !str_cmp( argument, "settime" ) ) {
        new_pfile_time_t = current_time + 86400;
        save_timedata(  );
        send_to_char( "New cleanup time set for 24 hrs from now.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "count" ) ) {
        log_printf( "Pfile count started by %s.", ch->name );
        pfile_scan( TRUE );
        return;
    }
    send_to_char( "Invalid argument.\r\n", ch );
}

void check_pfiles( time_t reset )
{
    /*
     * This only counts them up on reboot if the cleanup isn't needed - Samson 1-2-00 
     */
    if ( reset == 255 && new_pfile_time_t > current_time ) {
        reset = 0;                                     /* Call me paranoid, but it might
                                                        * be meaningful later on */
        log_string( "Counting pfiles....." );
        pfile_scan( TRUE );
        return;
    }
    if ( new_pfile_time_t <= current_time ) {
        if ( sysdata.CLEANPFILES == TRUE ) {
            new_pfile_time_t = current_time + 86400;
            save_timedata(  );
            log_string( "Automated pfile cleanup beginning...." );
            pfile_scan( FALSE );
#ifdef SAMSONRENT
            if ( reset == 0 )
                rent_update(  );
#endif
        }
        else {
            new_pfile_time_t = current_time + 86400;
            save_timedata(  );
            log_string( "Counting pfiles....." );
            pfile_scan( TRUE );
#ifdef SAMSONRENT
            if ( reset == 0 )
                rent_update(  );
#endif
        }
    }
}
