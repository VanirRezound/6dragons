/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - AFKMud     Copyright 1997, 1998, 1999, 2000, 2001, 2002 by Alsherok   *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Copyover module                                                       *
 ***************************************************************************/

#include <ctype.h>
#include <sys/stat.h>
#include "h/key.h"
#if defined(WIN32)
#include <process.h>
#else
#include <dirent.h>
#endif
#include <string.h>
#include "h/mud.h"
#include "h/copyover.h"
#include "h/auction.h"
#include "h/files.h"
#include "h/city.h"
#include "h/hometowns.h"

void                    get_curr_players( void );

#define MAX_NEST  100
static OBJ_DATA        *rgObjNest[MAX_NEST];
ROOM_INDEX_DATA        *room_index_hash[MAX_KEY_HASH];
extern int              control;
void                    end_tag( void );
void                    open_mud_log( void );
bool write_to_descriptor args( ( DESCRIPTOR_DATA *d, const char *txt, int length ) );
bool write_to_descriptor_old args( ( int desc, const char *txt, int length ) );

/* Used by copyover code */
void save_areas( void )
{
    AREA_DATA              *tarea;
    char                    filename[256];

    for ( tarea = first_build; tarea; tarea = tarea->next ) {
        if ( !IS_SET( tarea->status, AREA_LOADED ) )
            continue;
        snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
        fold_area( tarea, filename, FALSE );
    }
}

/*  Warm reboot stuff, gotta make sure to thank Erwin for this :) */
void do_copyover( CHAR_DATA *ch, char *argument )
{
    FILE                   *fp;
    DESCRIPTOR_DATA        *d,
                           *de_next;
    CHAR_DATA              *victim = NULL;
    char                    buf[100],
                            buf2[100],
                            buf3[100],
                            buf4[100];
    int                     fighting_count = 0;
    int                     editing_count = 0;
    int                     deleting_count = 0;
    int                     newbie_count = 0;
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;

    if ( ch && IS_NPC( ch ) )
        return;

    do_fclear( ch, ( char * ) "sys_bugs" );

    end_tag(  );

    for ( d = first_descriptor; d; d = d->next ) {
        /*
         * Volk - this needs to go here 
         */
        if ( d->character && IS_IMMORTAL( d->character ) && d->character->pcdata )
            d->character->pcdata->tmproom = d->character->in_room->vnum;

        if ( d->connected == CON_EDITING || d->connected == CON_NOTE_TO
             || d->connected == CON_NOTE_SUBJECT || d->connected == CON_NOTE_EXPIRE
             || d->connected == CON_EDITING )
            editing_count++;

        if ( d->connected == CON_DELETE )
            deleting_count++;

        if ( d->character && d->character->level < 2 )
            newbie_count++;

        if ( ( d->connected == CON_PLAYING ) && ( victim = d->character ) != NULL
             && !IS_NPC( victim ) && victim->fighting )
            fighting_count++;
    }

    if ( ch ) {
        if ( fighting_count || editing_count || deleting_count ) {
            ch_printf( ch, "%s", "Can't hotboot at this time.\r\n" );
            if ( fighting_count )
                ch_printf( ch, "There are %d combats in progress.\r\n", fighting_count );
            if ( editing_count )
                ch_printf( ch, "There are %d players editing things.\r\n", editing_count );
            if ( deleting_count )
                ch_printf( ch, "There are %d players deleting.\r\n", deleting_count );
            if ( auction->item != NULL )
                send_to_char( "There is an auction currently in progress.\r\n", ch );
            return;
        }
    }

    open_mud_log(  );
    log_string( "Copyover: Spawning new log file" );

    log_string( "Saving player files and connection states...." );

    if ( ch && ch->desc )
        write_to_descriptor( ch->desc, "\033[0m", 0 );
    snprintf( buf, 100,
              "\r\nThe flow of time is halted momentarily as the world is reshaped!\r\n" );

    fp = FileOpen( COPYOVER_FILE, "w" );
    if ( !fp ) {
        send_to_char( "Copyover file not writeable, aborted.\r\n", ch );
        log_printf( "Could not write to copyover file: %s. Copyover aborted.", COPYOVER_FILE );
        perror( "do_copyover: FileOpen" );
        return;
    }

    /*
     * For each playing descriptor, save its state 
     */
    for ( d = first_descriptor; d; d = de_next ) {
        CHAR_DATA              *och = CH( d );

        de_next = d->next;                             /* We delete from the list , so
                                                        * need to save this */
//    if(!d->character || d->connected < CON_PLAYING) /* drop those logging on */
        if ( !d->character || d->connected < CON_PLAYING ) {
            write_to_descriptor( d, "\r\nSorry, we are rebooting. Come back in a few minutes.\r\n",
                                 0 );
            close_socket( d, FALSE );                  /* throw'em out */
        }
        else {
            fprintf( fp, "%d %d %d %d %d %d %s %s\n", d->descriptor, 0, och->in_room->vnum,
                     och->was_in_room ? och->was_in_room->vnum : och->in_room->vnum, d->port,
                     d->idle, och->name, d->host );
            /*
             * One of two places this gets changed 
             */
            och->pcdata->copyover = TRUE;
            save_char_obj( och );
            write_to_descriptor( d, buf, 0 );
        }
    }

    /*
     * let's also save puppets now! - Volk 
     */
    CHAR_DATA              *puppet,
                           *next_pup;

    for ( puppet = first_char; puppet; puppet = next_pup ) {
        next_pup = puppet->next;

        if ( !IS_PUPPET( puppet ) )
            continue;

        fprintf( fp, "%d %d %d %d %d %d %s %s\n ", 0, 1, puppet->in_room->vnum,
                 puppet->was_in_room ? puppet->was_in_room->vnum : puppet->in_room->vnum, 0,
                 0, puppet->name, puppet->redirect->name );
        puppet->pcdata->copyover = TRUE;
        save_char_obj( puppet );

    }

    fprintf( fp, "0 0 0 0 0 %d maxp maxp\n", sysdata.maxplayers );
    fprintf( fp, "-1\n" );
    FileClose( fp );

    /*
     * Consider changing all loaded prototype areas here, if you use OLC 
     */
    log_string( "Saving modified area files..." );
    save_areas(  );

    log_string( "Executing copyover...." );

    /*
     * added this in case there's a need to debug the contents of the various files 
     */
    if ( argument && !str_cmp( argument, "debug" ) ) {
        log_string( "Copyover debug - Aborting before execl" );
        return;
    }

#ifdef I3
    if ( i3_is_connected(  ) ) {
        i3_save_chanlist(  );
        i3_save_mudlist(  );
        i3_save_history(  );
    }
#endif

    /*
     * Close reserve and other always-open files and release other resources 
     */
    /*
     * exec - descriptors are inherited 
     */
    snprintf( buf, 100, "%d", port );
    snprintf( buf2, 100, "%d", control );

#ifdef IMC
    if ( this_imcmud )
        snprintf( buf3, 100, "%d", this_imcmud->desc );
    else
        strncpy( buf3, "-1", 100 );
#ifdef I3
    snprintf( buf4, 100, "%d", I3_socket );
#else
    strncpy( buf4, "-1", 100 );
#endif
#else
#ifdef I3
    snprintf( buf3, 100, "%d", I3_socket );
#else
    strncpy( buf3, "-1", 100 );
#endif
    strncpy( buf4, "-1", 100 );
#endif

    /*
     * Uncomment this bfd_close line if you've installed the BFD snippet, you'll need it. 
     */
    // bfd_close(abfd);
    execl( EXE_FILE, "6dragons", buf, "copyover", buf2, buf3, buf4, ( char * ) NULL );
    /*
     * Failed - sucessful exec will not return 
     */
    perror( "do_copyover: execl" );
    log_string( "Copyover execution failed!!" );
    send_to_char( "Copyover FAILED!\r\n", ch );
}

/* Recover from a copyover - load players */
void copyover_recover(  )
{
    CHAR_DATA              *puppetmaster;
    DESCRIPTOR_DATA        *d = NULL;
    FILE                   *fp;
    char                    name[100],
                            host[MSL];
    int                     desc,
                            dcompress,
                            room,
                            lastroom,
                            dport,
                            fscanned,
                            idle,
                            maxp = 0;
    bool                    fOld;
    ROOM_INDEX_DATA        *location;

    fp = FileOpen( COPYOVER_FILE, "r" );
    if ( !fp ) {                                       /* there are some descriptors open 
                                                        * which will hang forever then ? */
        perror( "copyover_recover: FileOpen" );
        log_string( "Copyover file not found. Exitting.\r\n" );
        exit( 1 );
    }
    unlink( COPYOVER_FILE );                           /* In case something crashes -
                                                        * doesn't prevent reading */
    for ( ;; ) {
        d = NULL;
        fscanned =
            fscanf( fp, "%d %d %d %d %d %d %s %s\n", &desc, &dcompress, &room, &lastroom, &dport,
                    &idle, name, host );
        if ( desc == -1 || feof( fp ) )
            break;
        if ( !str_cmp( name, "maxp" ) || !str_cmp( host, "maxp" ) ) {
            maxp = idle;
            continue;
        }

        if ( dcompress == 1 ) {                        /* puppet! all chars loaded at
                                                        * this stage */

            if ( !( puppetmaster = get_char_world( NULL, host ) ) ) {
                bug( "%s: puppet couldn't be loaded because %s isn't online", __FUNCTION__, host );
                continue;
            }
            CREATE( d, DESCRIPTOR_DATA, 1 );

            if ( !load_char_obj( d, name, FALSE, TRUE, FALSE ) ) {
                ch_printf( get_char_world( NULL, host ), "PUPPET: %s failed to load.\r\n", name );
                continue;
            }
            CHAR_DATA              *puppet = CH( d );

            add_char( puppet );
            char_to_room( puppet, get_room_index( room ) );
//      act(AT_MAGIC, "$n appears in a puff of ethereal smoke!", puppet, NULL, NULL, TO_ROOM);   TOO SPAMMY!!

            if ( ++num_descriptors > sysdata.maxplayers )
                sysdata.maxplayers = num_descriptors;

            puppet->desc = NULL;
            puppet->redirect = puppetmaster;
            SET_BIT( puppet->pcdata->flags, PCFLAG_PUPPET );
            continue;
        }

        /*
         * Write something, and check if it goes error-free 
         */
        if ( !write_to_descriptor_old( desc, "\r\nThe ether swirls in chaos.\r\n", 0 ) ) {
            closesocket( desc );                       /* nope */
            continue;
        }

        CREATE( d, DESCRIPTOR_DATA, 1 );

        d->next = NULL;
        d->descriptor = desc;
        d->connected = CON_GET_NAME;
        d->outsize = 2000;
        d->idle = 0;
        d->lines = 0;
        d->scrlen = 24;
        d->newstate = 0;
        d->prevcolor = 0x08;
        CREATE( d->outbuf, char, d->outsize );

        d->host = STRALLOC( host );
        d->port = dport;
        d->idle = idle;
        LINK( d, first_descriptor, last_descriptor, next, prev );
        d->connected = CON_COPYOVER_RECOVER;           /* negative so close_socket will
                                                        * cut them off */
        /*
         * Now, find the pfile 
         */
        fOld = load_char_obj( d, name, FALSE, TRUE, FALSE );
        if ( !fOld ) {                                 /* Player file not found?! */
            write_to_descriptor( d,
                                 "\r\nSomehow, your character was lost during copyover. Contact the immortals ASAP.\r\n",
                                 0 );
            close_socket( d, FALSE );
        }
        else {                                         /* ok! */

            write_to_descriptor( d, "\r\nTime resumes its normal flow.\r\n", 0 );

            if ( IS_IMMORTAL( d->character ) ) {
                add_char( d->character );
                if ( d->character->pcdata ) {
                    if ( d->character->pcdata->tmproom )
                        location = get_room_index( d->character->pcdata->tmproom );
                }
                if ( !location )
                    location = get_room_index( 1200 );
//    char_from_room( d->character );
                char_to_room( d->character, location );

            }
            else {
                d->character->in_room = get_room_index( room );
                d->character->was_in_room = get_room_index( lastroom );

                if ( !d->character->in_room && !d->character->was_in_room ) {
                    d->character->in_room = get_room_index( d->character->pcdata->htown->recall );
                    d->character->was_in_room =
                        get_room_index( d->character->pcdata->htown->recall );
                }

                add_char( d->character );
                if ( d->character->in_room )
                    char_to_room( d->character, d->character->in_room );
                else
                    char_to_room( d->character, d->character->was_in_room );
            }

            if ( d->character->position == POS_MOUNTED )
                d->character->position = POS_STANDING;
            act( AT_MAGIC, "A puff of ethereal smoke dissipates around you!", d->character, NULL,
                 NULL, TO_CHAR );
            act( AT_MAGIC, "$n appears in a puff of ethereal smoke!", d->character, NULL, NULL,
                 TO_ROOM );
            d->connected = CON_PLAYING;

            if ( ++num_descriptors > sysdata.maxplayers )
                sysdata.maxplayers = num_descriptors;
        }
    }
    FileClose( fp );
    if ( maxp > sysdata.maxplayers )
        sysdata.maxplayers = maxp;
    log_string( "Copyover recovery complete." );

    get_curr_players(  );
#ifdef I3
    /*
     * Initialize and connect to I3
     */
    i3_startup( FALSE, port, 0 );

#endif

// check_pets( d->character );
    return;
}
