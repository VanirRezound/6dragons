/* LoP code team: Remcon, Paradigm
 * Special thinks go out to all these people for verious snippets used
 *   in this codebase.
 * Samson, Cronel, Kratas, John 'Noplex' Bellone, Desden el Chaman Tibetano,
 *   Orion, Scion, Xerves, Whir, Belgarath, Zarius, Gangien, Desden, Cid,
 *   John Strange, Cyrus, Robcon, Xorith, and Jay Roman
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,
 *   Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,
 *   Tricops and Fireblade
 * Merc 2.1 Diku Mud improvments copyright(C)1992, 1993 by Michael
 *   Chastain, Michael Quan, and Mitchell Tse.
 * Original Diku Mud copyright(C)1990, 1991 by Sebastian Hammer,
 *   Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe. */

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include "h/mud.h"
#include "h/files.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/*
 Note Board system, (c) 1995-96 Erwin S. Andreasen, erwin@pip.dknet.dk
 =====================================================================
 Basically, the notes are split up into several boards. The boards do not
 exist physically, they can be read anywhere and in any position.
 Each of the note boards has its own file. Each of the boards can have its own
 "rights": who can read/write.
 Each character has an extra field added, namele the timestamp of the last note
 read by him/her on a certain board.
 The note entering system is changed too, making it more interactive. When
 entering a note, a character is put AFK and into a special CON_ state.
 Everything typed goes into the note.
 For the immortals it is possible to purge notes based on age. An Archive
 options is available which moves the notes older than X days into a special
 board. The file of this board should then be moved into some other directory
 during e.g. the startup script and perhaps renamed depending on date.
 Note that write_level MUST be >= read_level or else there will be strange
 output in certain functions.
 Board DEFAULT_BOARD must be at least readable by *everyone*.
*/
void                    stralloc_printf( char **pointer, const char *fmt, ... );

void                    unlink_note( GLOBAL_BOARD_DATA * board, NOTE_DATA *note );
void                    do_gtext( CHAR_DATA *ch, char *argument );

#define VOTE_NONE 0
#define VOTE_OPEN 1
#define VOTE_CLOSED 2

#define NOTE_DIR "notes/"
#define DEF_NORMAL 0
#define DEF_INCLUDE 1
#define DEF_EXCLUDE 2
#define MAX_LINE_LENGTH 80
#define MAX_NOTE_TEXT (4*MSL - 1000)
bool                    is_note_to( CHAR_DATA *ch, NOTE_DATA *note );

#define L_SUP (MAX_LEVEL - 1)                          /* if not already defined */

GLOBAL_BOARD_DATA       boards[MAX_BOARD] = {
    {"General", "All Purpose", 0, 2, "all", DEF_INCLUDE, 60, NULL},
    {"Personal", "Personal Messages", 0, 1, "all", DEF_EXCLUDE, 60, NULL},
    {"Stories", "Role Play", 0, 2, "all", DEF_INCLUDE, 60, NULL},
    {"Staff", "Staff Messages", LEVEL_IMMORTAL, LEVEL_IMMORTAL, "staff", DEF_NORMAL, 60, NULL}
};

long                    last_note_stamp = 0;           /* To generate unique timestamps
                                                        * on notes */

#define BOARD_NOACCESS -1
#define BOARD_NOTFOUND -1
bool                    next_board( CHAR_DATA *ch );

/* recycle a note */
void free_global_note( NOTE_DATA *note )
{
    if ( note->sender )
        STRFREE( note->sender );
    if ( note->to_list )
        STRFREE( note->to_list );
    if ( note->subject )
        STRFREE( note->subject );
    if ( note->date )
        STRFREE( note->date );
    if ( note->text )
        STRFREE( note->text );
    if ( note->yesvotes )
        STRFREE( note->yesvotes );
    if ( note->novotes )
        STRFREE( note->novotes );
    if ( note->abstentions )
        STRFREE( note->abstentions );
    DISPOSE( note );
}

void free_all_global_boards( void )
{
    int                     i;
    GLOBAL_BOARD_DATA      *board;
    NOTE_DATA              *note,
                           *next_note;

    for ( i = 0; i < MAX_BOARD; i++ ) {
        board = &boards[i];
        for ( note = board->note_first; note; note = next_note ) {
            next_note = note->next;
            free_global_note( note );
        }
    }
}

/* allocate memory for a new note or recycle */
/* This should be redone, btw. It should work as it is, but smaug really
 * doesn't really bother much with recyling... why start now?
 * Also means redoing function above... Not difficult to redo, but your
 * choice out there. */
NOTE_DATA              *new_note(  )
{
    NOTE_DATA              *note;

    CREATE( note, NOTE_DATA, 1 );

    note->next = NULL;
    note->sender = NULL;
    note->expire = 0;
    note->to_list = NULL;
    note->subject = NULL;
    note->date = NULL;
    note->date_stamp = 0;
    note->text = NULL;
    note->voting = 0;
    return note;
}

/* append this note to the given file */
void append_note( FILE * fp, NOTE_DATA *note )
{
    fprintf( fp, "Sender      %s~\n", note->sender ? note->sender : "" );
    fprintf( fp, "Date        %s~\n", note->date ? note->date : "" );
    fprintf( fp, "Stamp       %ld\n", note->date_stamp );
    fprintf( fp, "Expire      %ld\n", note->expire );
    fprintf( fp, "To          %s~\n", note->to_list ? note->to_list : "" );
    fprintf( fp, "Subject     %s~\n", note->subject ? note->subject : "" );
    fprintf( fp, "Text\n%s~\n", note->text ? strip_cr( note->text ) : "" );
    fprintf( fp, "Voting      %d\n", note->voting );
    fprintf( fp, "Yesvotes    %s~\n", note->yesvotes ? note->yesvotes : "" );
    fprintf( fp, "Novotes     %s~\n", note->novotes ? note->novotes : "" );
    fprintf( fp, "Abstentions %s~\n\n", note->abstentions ? note->abstentions : "" );
}

/* Save a note in a given board */
void finish_note( GLOBAL_BOARD_DATA * board, NOTE_DATA *note )
{
    FILE                   *fp;
    NOTE_DATA              *p;
    char                    filename[200];

    /*
     * The following is done in order to generate unique date_stamps 
     */
    if ( last_note_stamp >= current_time )
        note->date_stamp = ++last_note_stamp;
    else {
        note->date_stamp = current_time;
        last_note_stamp = current_time;
    }
    if ( board->note_first ) {                         /* are there any notes in there
                                                        * now? */
        for ( p = board->note_first; p->next; p = p->next );    /* empty */
        p->next = note;
    }
    else                                               /* nope. empty list. */
        board->note_first = note;
    /*
     * append note to note file 
     */
    snprintf( filename, 200, "%s%s", NOTE_DIR, board->short_name );
    fp = FileOpen( filename, "a" );
    if ( !fp ) {
        bug( "%s", "Could not open one of the note files in append mode" );
        return;
    }
    append_note( fp, note );
    FileClose( fp );
}

/* Find the number of a board */
int board_number( const GLOBAL_BOARD_DATA * board )
{
    int                     i;

    for ( i = 0; i < MAX_BOARD; i++ )
        if ( board == &boards[i] )
            return i;
    return -1;
}

/* Find a board number based on  a string */
int board_lookup( const char *name )
{
    int                     i;

    for ( i = 0; i < MAX_BOARD; i++ )
        if ( !str_cmp( boards[i].short_name, name ) )
            return i;
    return -1;
}

/* Remove list from the list. Do not free note */
void unlink_note( GLOBAL_BOARD_DATA * board, NOTE_DATA *note )
{
    NOTE_DATA              *p;

    if ( board->note_first == note )
        board->note_first = note->next;
    else {
        for ( p = board->note_first; p && p->next != note; p = p->next );
        if ( !p )
            bug( "%s", "unlink_note: could not find note." );
        else
            p->next = note->next;
    }
}

/* Find the nth note on a board. Return NULL if ch has no access to that note */
NOTE_DATA              *find_note( CHAR_DATA *ch, GLOBAL_BOARD_DATA * board, int num )
{
    int                     count = 0;
    NOTE_DATA              *p;

    for ( p = board->note_first; p; p = p->next )
        if ( ++count == num )
            break;
    if ( ( count == num ) && is_note_to( ch, p ) )
        return p;
    else
        return NULL;
}

/* save a single board */
void save_board( GLOBAL_BOARD_DATA * board )
{
    FILE                   *fp;
    char                    filename[200];
    NOTE_DATA              *note;

    snprintf( filename, 200, "%s%s", NOTE_DIR, board->short_name );
    fp = FileOpen( filename, "w" );
    if ( !fp ) {
        bug( "Error writing to: %s", filename );
        return;
    }
    else {
        for ( note = board->note_first; note; note = note->next )
            append_note( fp, note );
        FileClose( fp );
    }
}

/* Show one note to a character */
void show_note_to_char( CHAR_DATA *ch, NOTE_DATA *note, int num )
{
    pager_printf( ch, "&c[&C%4d&c] %s: %s\r\n", num, note->sender, note->subject );
    pager_printf( ch, "&cDate:  &C%s\r\n", note->date );
    pager_printf( ch, "&cTo:    &C%s\r\n", note->to_list );
    pager_printf( ch, "&cVoting: &C%s\r\n", note->voting == VOTE_OPEN ? "open" : "closed" );
    if ( VLD_STR( note->yesvotes ) )
        pager_printf( ch, "&cYes Votes:   &C%s\r\n", note->yesvotes );
    if ( VLD_STR( note->novotes ) )
        pager_printf( ch, "&cNo Votes:    &C%s\r\n", note->novotes );
    if ( VLD_STR( note->abstentions ) )
        pager_printf( ch, "&cAbstentions: &C%s\r\n", note->abstentions );
    if ( !IS_BLIND( ch ) ) {
        send_to_pager
            ( "&c===========================================================================&w\r\n",
              ch );
    }
    pager_printf( ch, "&c%s\r\n", note->text );
}

/* Load a single board */
void load_board( GLOBAL_BOARD_DATA * board )
{
    FILE                   *fp;
    NOTE_DATA              *last2_note;
    char                    filename[200];
    bool                    resave = FALSE;

    snprintf( filename, 200, "%s%s", NOTE_DIR, board->short_name );
    fp = FileOpen( filename, "r" );
    /*
     * Silently return 
     */
    if ( !fp )
        return;
    /*
     * Start note fetching. copy of db.c:load_notes() 
     */
    last2_note = NULL;
    for ( ;; ) {
        NOTE_DATA              *pnote;
        char                    letter;

        do {
            letter = getc( fp );
            if ( feof( fp ) ) {
                FileClose( fp );
                fp = NULL;
                if ( resave )
                    save_board( board );
                return;
            }
        }
        while ( isspace( letter ) );
        ungetc( letter, fp );
        CREATE( pnote, NOTE_DATA, sizeof( *pnote ) );

        if ( str_cmp( fread_word( fp ), "sender" ) )
            break;
        pnote->sender = fread_string( fp );
        if ( str_cmp( fread_word( fp ), "date" ) )
            break;
        pnote->date = fread_string( fp );
        if ( str_cmp( fread_word( fp ), "stamp" ) )
            break;
        pnote->date_stamp = fread_number( fp );
        if ( str_cmp( fread_word( fp ), "expire" ) )
            break;
        pnote->expire = fread_number( fp );
        if ( str_cmp( fread_word( fp ), "to" ) )
            break;
        pnote->to_list = fread_string( fp );
        if ( str_cmp( fread_word( fp ), "subject" ) )
            break;
        pnote->subject = fread_string( fp );
        if ( str_cmp( fread_word( fp ), "text" ) )
            break;
        pnote->text = fread_string( fp );
        if ( !str_cmp( fread_word( fp ), "voting" ) ) {
            pnote->voting = fread_number( fp );
            if ( str_cmp( fread_word( fp ), "yesvotes" ) )
                break;
            pnote->yesvotes = fread_string( fp );
            if ( str_cmp( fread_word( fp ), "novotes" ) )
                break;
            pnote->novotes = fread_string( fp );
            if ( str_cmp( fread_word( fp ), "abstentions" ) )
                break;
            pnote->abstentions = fread_string( fp );
        }
        pnote->next = NULL;                            /* jic */

        /*
         * Should this note be archived right now ? 
         */
        if ( pnote->expire < current_time ) {
            free_global_note( pnote );
            resave = TRUE;
            continue;
        }
        if ( board->note_first == NULL )
            board->note_first = pnote;
        else
            last2_note->next = pnote;
        last2_note = pnote;
    }
    bug( "%s", "Load_notes: bad key word." );
}

/* Initialize structures. Load all boards. */
void load_global_boards( void )
{
    int                     i;

    for ( i = 0; i < MAX_BOARD; i++ )
        load_board( &boards[i] );
}

/* Return the number of unread notes 'ch' has in 'board' */
/* Returns BOARD_NOACCESS if ch has no access to board */
int unread_notes( CHAR_DATA *ch, GLOBAL_BOARD_DATA * board, bool fall )
{
    NOTE_DATA              *note;
    int                     count = 0;

    if ( board->read_level > get_trust( ch ) )
        return BOARD_NOACCESS;

    if ( !fall ) {
        for ( note = board->note_first; note; note = note->next )
            if ( is_note_to( ch, note )
                 && ( note->date_stamp > ch->pcdata->last2_note[board_number( board )] ) )
                count++;
    }
    else
        for ( note = board->note_first; note; note = note->next )
            if ( is_note_to( ch, note ) )
                count++;

    return count;
}

/* COMMANDS */
/* Start writing a note */
void do_nwrite( CHAR_DATA *ch, char *argument )
{
    char                   *strtime;

    if ( !ch || IS_NPC( ch ) )
        return;
    if ( get_trust( ch ) < ch->pcdata->board->write_level ) {
        send_to_char( "You cannot post notes on this board.\r\n", ch );
        return;
    }
    /*
     * continue previous note, if any text was written
     */
    if ( ch->pcdata->in_progress && ( !ch->pcdata->in_progress->text ) ) {
        send_to_char
            ( "Note in progress cancelled because you did not manage to write any text \r\nbefore losing link.\r\n\r\n",
              ch );
        free_global_note( ch->pcdata->in_progress );
        ch->pcdata->in_progress = NULL;
    }
    if ( !ch->pcdata->in_progress ) {
        ch->pcdata->in_progress = new_note(  );
        ch->pcdata->in_progress->sender = STRALLOC( ch->name );
        /*
         * convert to ascii. ctime returns a string which last character is \n, so remove that 
         */
        strtime = ctime( &current_time );
        strtime[strlen( strtime ) - 1] = '\0';
        ch->pcdata->in_progress->date = STRALLOC( strtime );
    }
    act( AT_GREEN, "$n starts writing a note.&w", ch, NULL, NULL, TO_ROOM );
    /*
     * Begin writing the note ! 
     */
    ch_printf( ch, "&cYou are now %s a new note on the &C%s&c board.\r\n",
               ch->pcdata->in_progress->text ? "continuing" : "posting",
               ch->pcdata->board->short_name );
    // ch_printf(ch, "&cFrom&C: %s\r\n", ch->name);
    if ( !ch->pcdata->in_progress->text ) {            /* Are we continuing an old note
                                                        * or not? */
/*
    switch (ch->pcdata->board->force_type)
    {
      case DEF_NORMAL:
        ch_printf(ch, "If you press Return, default recipient \"&W%s&w\" will be chosen.\r\n", ch->pcdata->board->names);
        break;
      case DEF_INCLUDE:
        ch_printf(ch, "The recipient list MUST include \"&W%s&w\". If not, it will be added automatically.\r\n", ch->pcdata->board->names);
        break;
      case DEF_EXCLUDE:
        ch_printf(ch, "The recipient of this note must NOT include: \"&W%s&w\".", ch->pcdata->board->names);
        break;
    }
*/
        if ( ch->pcdata->board == &boards[PERSONAL_BOARD] ) {
            send_to_char( "\r\n&cType who you want the note to go to&C :", ch );
            ch->desc->connected = CON_NOTE_TO;
        }
        else {
            mudstrlcat( argument, " ", MSL );
            mudstrlcat( argument, ch->pcdata->board->names, MSL );
            ch->pcdata->in_progress->to_list = STRALLOC( argument );
            send_to_char( "&c\r\nType the subject of this note&C: ", ch );
            ch->desc->connected = CON_NOTE_SUBJECT;
        }
        /*
         * nanny takes over from here 
         */
    }
    else {                                             /* we are continuing, print out *
                                                        * * * * all the fields and the
                                                        * note * * so * * * far */
        ch_printf( ch,
                   "&cType who you want to write the note to.  If you are not on the personal board, then type all. &C:      %s\r\n&cWhen do you want the note to expire&C: %s\r\n&cWhat is the note subject&C: %s\r\n",
                   ch->pcdata->in_progress->to_list, ctime( &ch->pcdata->in_progress->expire ),
                   ch->pcdata->in_progress->subject );
        send_to_char( ch->pcdata->in_progress->text, ch );
        do_gtext( ch, ( char * ) "write" );
    }
    return;
}

/* Read next note in current group. If no more notes, go to next board */
void do_nread( CHAR_DATA *ch, char *argument )
{
    NOTE_DATA              *p;
    int                     count = 0,
        number;

    if ( !ch || IS_NPC( ch ) )
        return;

    if ( !str_cmp( argument, "again" ) ) {             /* read last note again */
    }
    else if ( is_number( argument ) ) {
        number = atoi( argument );

        for ( p = ch->pcdata->board->note_first; p; p = p->next ) {
            if ( ++count == number )
                break;
        }
        if ( !p || !is_note_to( ch, p ) )
            send_to_char( "No such note.\r\n", ch );
        else {
            show_note_to_char( ch, p, count );
            if ( p->date_stamp > ch->pcdata->last2_note[board_number( ch->pcdata->board )] )
                ch->pcdata->last2_note[board_number( ch->pcdata->board )] = p->date_stamp;
        }
    }
    else {                                             /* just next one */

        count = 1;

        for ( p = ch->pcdata->board->note_first; p; p = p->next, count++ ) {
            if ( ( p->date_stamp > ch->pcdata->last2_note[board_number( ch->pcdata->board )] )
                 && is_note_to( ch, p ) ) {
                show_note_to_char( ch, p, count );
                ch->pcdata->last2_note[board_number( ch->pcdata->board )] = p->date_stamp;
                return;
            }
        }
        send_to_char( "No new notes in this board.\r\n", ch );
        if ( next_board( ch ) )
            ch_printf( ch, "Changed to next board, %s.\r\n", ch->pcdata->board->short_name );
        else
            send_to_char( "There are no more boards.\r\n", ch );
    }
}

void do_nvote( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MSL],
                            buf[MSL];
    NOTE_DATA              *p;
    int                     count = 0,
        number;

    argument = one_argument( argument, arg1 );
    if ( !is_number( arg1 ) ) {
        send_to_char( "Syntax: note vote <note #> <open/close/yes/no/abstain>\r\n", ch );
        return;
    }
    else {
        number = atoi( arg1 );

        for ( p = ch->pcdata->board->note_first; p; p = p->next )
            if ( ++count == number )
                break;
        if ( !p || !is_note_to( ch, p ) ) {
            send_to_char( "No such note to vote on.\r\n", ch );
            return;
        }
        else {
            char                   *strtime;

            if ( !str_cmp( argument, "open" ) ) {
                if ( str_cmp( ch->name, p->sender ) ) {
                    send_to_char( "You are not the author of this note.\r\n", ch );
                    return;
                }
                p->voting = VOTE_OPEN;
                strtime = ctime( &current_time );
                strtime[strlen( strtime ) - 1] = '\0';
                p->date = STRALLOC( strtime );
                p->date_stamp = current_time;
                act( AT_ACTION, "$n opens voting on a note.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "Voting opened.\r\n", ch );
                save_board( ch->pcdata->board );       /* save the board */
                return;
            }
            else if ( !str_cmp( argument, "close" ) ) {
                if ( str_cmp( ch->name, p->sender ) ) {
                    send_to_char( "You are not the author of this note.\r\n", ch );
                    return;
                }
                p->voting = VOTE_CLOSED;
                strtime = ctime( &current_time );
                strtime[strlen( strtime ) - 1] = '\0';
                p->date = STRALLOC( strtime );
                p->date_stamp = current_time;
                act( AT_ACTION, "$n closes voting on a note.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "Voting closed.\r\n", ch );
                save_board( ch->pcdata->board );       /* save the board */
                return;
            }
            /*
             * Make sure the note is open for voting before going on. 
             */
            if ( p->voting != VOTE_OPEN ) {
                send_to_char( "Voting is not open on this note.\r\n", ch );
                return;
            }
            /*
             * Can only vote once on a note. 
             */
            snprintf( buf, MSL, "%s %s %s", p->yesvotes ? p->yesvotes : "",
                      p->novotes ? p->novotes : "", p->abstentions ? p->abstentions : "" );
            if ( is_name( ch->name, buf ) ) {
                send_to_char( "You have already voted on this note.\r\n", ch );
                return;
            }
            if ( !str_cmp( argument, "yes" ) ) {
                if ( VLD_STR( p->yesvotes ) )
                    stralloc_printf( &( p->yesvotes ), "%s %s", p->yesvotes, ch->name );
                else
                    stralloc_printf( &( p->yesvotes ), "%s", ch->name );
                strtime = ctime( &current_time );
                strtime[strlen( strtime ) - 1] = '\0';
                p->date = STRALLOC( strtime );
                p->date_stamp = current_time;
                act( AT_ACTION, "$n votes on a note.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "Ok.\r\n", ch );
                save_board( ch->pcdata->board );       /* save the board */
                return;
            }
            else if ( !str_cmp( argument, "no" ) ) {
                if ( VLD_STR( p->novotes ) )
                    stralloc_printf( &( p->novotes ), "%s %s", p->novotes, ch->name );
                else
                    stralloc_printf( &( p->novotes ), "%s", ch->name );
                strtime = ctime( &current_time );
                strtime[strlen( strtime ) - 1] = '\0';
                p->date = STRALLOC( strtime );
                p->date_stamp = current_time;
                act( AT_ACTION, "$n votes on a note.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "Ok.\r\n", ch );
                save_board( ch->pcdata->board );       /* save the board */
                return;
            }
            else if ( !str_cmp( argument, "abstain" ) ) {
                if ( VLD_STR( p->abstentions ) )
                    stralloc_printf( &( p->abstentions ), "%s %s", p->abstentions, ch->name );
                else
                    stralloc_printf( &( p->abstentions ), "%s", ch->name );
                strtime = ctime( &current_time );
                strtime[strlen( strtime ) - 1] = '\0';
                p->date = STRALLOC( strtime );
                p->date_stamp = current_time;
                act( AT_ACTION, "$n votes on a note.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "Ok.\r\n", ch );
                save_board( ch->pcdata->board );       /* save the board */
                return;
            }
            /*
             * if they get here show them something lol 
             */
            send_to_char( "Syntax: note vote <note #> <open/close/yes/no/abstain>\r\n", ch );
            return;
        }
        /*
         * if they get here show them something lol 
         */
        send_to_char( "Syntax: note vote <note #> <open/close/yes/no/abstain>\r\n", ch );
        return;
    }
    /*
     * if they get here show them something lol 
     */
    send_to_char( "Syntax: note vote <note #> <open/close/yes/no/abstain>\r\n", ch );
    return;
}

void do_nedit( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MSL],
                           *strtime;
    NOTE_DATA              *p;
    int                     count = 0,
        number;

    argument = one_argument( argument, arg1 );
    switch ( ch->substate ) {
        default:
            break;
        case SUB_RESTRICTED:
            send_to_char( "You cannot do this while in another command.\r\n", ch );
            return;
        case SUB_GNOTE:
            if ( ch->substate == SUB_REPEATCMD )
                ch->tempnum = SUB_REPEATCMD;
            else
                ch->tempnum = SUB_NONE;
            ch->pcdata->in_progress = ( NOTE_DATA * ) ch->dest_buf;
            STRFREE( ch->pcdata->in_progress->text );
            ch->pcdata->in_progress->text = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;
            finish_note( ch->pcdata->board, ch->pcdata->in_progress );
            ch->pcdata->in_progress = NULL;
            save_board( ch->pcdata->board );           /* save the board */
            return;
    }
    if ( !is_number( arg1 ) ) {
        send_to_char( "Syntax: note edit <note #>\r\n", ch );
        return;
    }
    else {
        number = atoi( arg1 );

        if ( ch->substate == SUB_RESTRICTED ) {
            send_to_char( "You cannot write a note from within another command.\r\n", ch );
            return;
        }
        if ( ch->pcdata->board == NULL )
            ch->pcdata->board = &boards[DEFAULT_BOARD];
        for ( p = ch->pcdata->board->note_first; p; p = p->next )
            if ( ++count == number )
                break;
        if ( !p ) {
            send_to_char( "No such note to edit.\r\n", ch );
            return;
        }
        else {
            if ( str_cmp( ch->name, p->sender ) && ch->level < MAX_LEVEL ) {
                send_to_char( "You are not the author of this note.\r\n", ch );
                return;
            }
            ch->substate = SUB_GNOTE;
            ch->pcdata->in_progress = new_note(  );
            ch->dest_buf = ch->pcdata->in_progress;
            /*
             * copy the old note to the new note 
             */
            if ( VLD_STR( p->sender ) )
                ch->pcdata->in_progress->sender = STRALLOC( p->sender );
            ch->pcdata->in_progress->expire = p->expire;
            if ( VLD_STR( p->to_list ) )
                ch->pcdata->in_progress->to_list = STRALLOC( p->to_list );
            if ( VLD_STR( p->subject ) )
                ch->pcdata->in_progress->subject = STRALLOC( p->subject );
            /*
             * change date and date stamp since its been edited 
             */
            strtime = ctime( &current_time );
            strtime[strlen( strtime ) - 1] = '\0';
            ch->pcdata->in_progress->date = STRALLOC( strtime );
            ch->pcdata->in_progress->date_stamp = current_time;;
            ch->pcdata->in_progress->voting = p->voting;
            if ( VLD_STR( p->yesvotes ) )
                ch->pcdata->in_progress->yesvotes = STRALLOC( p->yesvotes );
            if ( VLD_STR( p->novotes ) )
                ch->pcdata->in_progress->novotes = STRALLOC( p->novotes );
            if ( VLD_STR( p->abstentions ) )
                ch->pcdata->in_progress->abstentions = STRALLOC( p->abstentions );
            if ( VLD_STR( p->text ) )
                ch->pcdata->in_progress->text = STRALLOC( p->text );
            /*
             * Delete the old note 
             */
            unlink_note( ch->pcdata->board, p );
            free_global_note( p );
            start_editing( ch, ch->pcdata->in_progress->text );
            set_editor_desc( ch, "A global note." );
            return;
        }
        send_to_char( "Syntax: note edit <note #>\r\n", ch );
        return;
    }
    send_to_char( "Syntax: note edit <note #>\r\n", ch );
    return;
}

/* Remove a note */
void do_nremove( CHAR_DATA *ch, char *argument )
{
    NOTE_DATA              *p;

    if ( !is_number( argument ) ) {
        send_to_char( "Remove which note?\r\n", ch );
        return;
    }
    p = find_note( ch, ch->pcdata->board, atoi( argument ) );
    if ( !p ) {
        send_to_char( "No such note.\r\n", ch );
        return;
    }
    if ( str_cmp( ch->name, p->sender ) && ( get_trust( ch ) < MAX_LEVEL ) ) {
        send_to_char( "You are not authorized to remove this note.\r\n", ch );
        return;
    }
    unlink_note( ch->pcdata->board, p );
    free_global_note( p );
    send_to_char( "Note removed!\r\n", ch );
    save_board( ch->pcdata->board );                   /* save the board */
}

/* List all notes or if argument given, list N of the last notes */
/* Shows REAL note numbers! */
void do_nlist( CHAR_DATA *ch, char *argument )
{
    int                     count = 0,
        show = 0,
        num = 0,
        has_shown = 0;
    time_t                  last2_note;
    NOTE_DATA              *p;

    if ( is_number( argument ) ) {                     /* first, count the number of
                                                        * notes */
        show = atoi( argument );
        for ( p = ch->pcdata->board->note_first; p; p = p->next )
            if ( is_note_to( ch, p ) )
                count++;
    }
    ch_printf( ch, "          &cNotes on %s:&w\r\n" "          &cNum> Author        Subject\r\n",
               ch->pcdata->board->short_name );
    last2_note = ch->pcdata->last2_note[board_number( ch->pcdata->board )];
    for ( p = ch->pcdata->board->note_first; p; p = p->next ) {
        num++;
        if ( is_note_to( ch, p ) ) {
            has_shown++;                               /* note that we want to see X
                                                        * VISIBLE note, not just last X */
            if ( !show || ( ( count - show ) < has_shown ) )
                ch_printf( ch, "          &C%3d&c %c %-13s %s \r\n", num,
                           last2_note < p->date_stamp ? '*' : ' ', p->sender, p->subject );
        }
    }
}

/* catch up with some notes */
void do_ncatchup( CHAR_DATA *ch, char *argument )
{
    GLOBAL_BOARD_DATA      *board = NULL;
    NOTE_DATA              *p;
    int                     count = 0,
        tcount = 0,
        i;

    for ( i = 0; i < MAX_BOARD; i++ ) {
        board = &boards[i];
        if ( !board )
            continue;
        count = unread_notes( ch, &boards[i], FALSE );
        if ( count <= 0 )
            continue;
        tcount += count;
        /*
         * Find last note 
         */
        for ( p = board->note_first; p && p->next; p = p->next );
        /*
         * For ends on the line above 
         */
        if ( !p )
            continue;
        else
            ch->pcdata->last2_note[board_number( &boards[i] )] = p->date_stamp;
    }
    if ( tcount == 0 )
        send_to_char( "No notes on any board to skip.\r\n", ch );
    else
        ch_printf( ch, "You have skipped reading %d messages.\r\n", tcount );
    return;
}

/* Dispatch function for backwards compatibility */
void do_global_note( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    if ( !ch || IS_NPC( ch ) )
        return;
    switch ( ch->substate ) {
        default:
            break;
        case SUB_RESTRICTED:
            send_to_char( "You cannot do this while in another command.\r\n", ch );
            return;
        case SUB_GNOTE:
            if ( ch->substate == SUB_REPEATCMD )
                ch->tempnum = SUB_REPEATCMD;
            else
                ch->tempnum = SUB_NONE;
            ch->pcdata->in_progress = ( NOTE_DATA * ) ch->dest_buf;
            if ( VLD_STR( ch->pcdata->in_progress->text ) )
                STRFREE( ch->pcdata->in_progress->text );
            ch->pcdata->in_progress->text = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;
            finish_note( ch->pcdata->board, ch->pcdata->in_progress );
            ch->pcdata->in_progress = NULL;
            save_board( ch->pcdata->board );           /* save the board */
            return;
    }
    argument = one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        send_to_char( "Syntax: note list\r\n"
                      "Syntax: note read <note #>\r\n"
                      "Syntax: note write\r\n"
                      "Syntax: note remove <note #>\r\n"
                      "Syntax: note catchup\r\n"
                      "Syntax: note edit <note #>\r\n"
                      "Syntax: note vote <note #> open\r\n"
                      "Syntax: note vote <note #> close\r\n" "Syntax: note vote <note #> yes\r\n"
                      "Syntax: note vote <note #> no\r\n" "Syntax: note vote <note #> abstain\r\n",
                      ch );
        return;
    }
    if ( ch->pcdata->board == NULL )
        ch->pcdata->board = &boards[DEFAULT_BOARD];
    if ( !str_cmp( arg, "read" ) )                     /* 'note' or 'note read X' */
        do_nread( ch, argument );
    else if ( !str_cmp( arg, "list" ) )
        do_nlist( ch, argument );
    else if ( !str_cmp( arg, "write" ) )
        do_nwrite( ch, argument );
    else if ( !str_cmp( arg, "remove" ) )
        do_nremove( ch, argument );
    else if ( !str_cmp( arg, "catchup" ) )
        do_ncatchup( ch, argument );
    else if ( !str_cmp( arg, "vote" ) )
        do_nvote( ch, argument );
    else if ( !str_cmp( arg, "edit" ) )
        do_nedit( ch, argument );
    else
        do_help( ch, ( char * ) "note" );
    return;
}

/* Show all accessible boards with their numbers of unread messages OR
 change board. New board name can be given as a number or as a name (e.g.
 board personal or board 4 */
void do_global_boards( CHAR_DATA *ch, char *argument )
{
    int                     i,
                            count,
                            number;

    if ( !ch || IS_NPC( ch ) )
        return;
    if ( !VLD_STR( argument ) ) {                      /* show boards */
        int                     unread,
                                all;
        char                   *permissions;

        count = 1;
        send_to_char( "\r\n          &cNum        Name Unread/All  Description&W\r\n", ch );
        if ( !IS_BLIND( ch ) ) {
            send_to_char( "          &C=== ============ ========== ====================&W\r\n",
                          ch );
        }
        for ( i = 0; i < MAX_BOARD; i++ ) {
            unread = unread_notes( ch, &boards[i], FALSE ); /* how many unread notes? */
            all = unread_notes( ch, &boards[i], TRUE ); /* how many unread notes? */
            if ( unread != BOARD_NOACCESS ) {
                ch_printf( ch, "          &C%2d &c%12s [&R%s%4d&c/&G%-4d&c] %s\r\n", count,
                           boards[i].short_name, unread ? "&R" : "&G", unread, all,
                           boards[i].long_name );
                count++;
            }                                          /* if has access */
        }                                              /* for each board */
        if ( ch->pcdata->board == NULL )
            ch->pcdata->board = &boards[DEFAULT_BOARD];
        ch_printf( ch, "\r\n          &cCurrently viewing &C%s&c board.\r\n",
                   ch->pcdata->board->short_name );

        /*
         * Inform of rights 
         */
        send_to_char( "          &cSyntax: &Cunread\r\n", ch );
        send_to_char( "          &cSee help &Cboard&D\r\n", ch );
        return;
    }                                                  /* if empty argument */
    if ( ch->pcdata->in_progress ) {
        send_to_char( "Please finish your interrupted note first.\r\n", ch );
        return;
    }
    /*
     * Change board based on its number 
     */
    if ( is_number( argument ) ) {
        count = 0;
        number = atoi( argument );
        for ( i = 0; i < MAX_BOARD; i++ )
            if ( unread_notes( ch, &boards[i], FALSE ) != BOARD_NOACCESS )
                if ( ++count == number )
                    break;
        if ( count == number ) {                       /* found the board.. change to it */
            ch->pcdata->board = &boards[i];
            ch_printf( ch, "Current board changed to &W%s&w. %s.\r\n", boards[i].short_name,
                       ( get_trust( ch ) <
                         boards[i].
                         write_level ) ? "You can only read here" :
                       "You can both read and write here" );
        }
        else                                           /* so such board */
            send_to_char( "No such board.\r\n", ch );
        return;
    }
    /*
     * Non-number given, find board with that name 
     */
    for ( i = 0; i < MAX_BOARD; i++ )
        if ( !str_cmp( boards[i].short_name, argument ) )
            break;
    /*
     * Does ch have access to this board? 
     */
    if ( i == MAX_BOARD || unread_notes( ch, &boards[i], FALSE ) == BOARD_NOACCESS ) {
        send_to_char( "No such board.\r\n", ch );
        return;
    }
    ch->pcdata->board = &boards[i];
    ch_printf( ch, "Current board changed to &W%s&w. %s.\r\n", boards[i].short_name,
               ( get_trust( ch ) <
                 boards[i].
                 write_level ) ? "You can only read here" : "You can both read and write here" );
    return;
}

/* tries to change to the next accessible board */
bool next_board( CHAR_DATA *ch )
{
    int                     i = board_number( ch->pcdata->board ) + 1;

    if ( !ch )
        return FALSE;
    while ( ( i < MAX_BOARD ) && ( unread_notes( ch, &boards[i], FALSE ) == BOARD_NOACCESS ) )
        i++;
    if ( i == MAX_BOARD ) {
        ch->pcdata->board = &boards[0];
        return TRUE;
    }
    else {
        ch->pcdata->board = &boards[i];
        return TRUE;
    }
    return FALSE;
}

void do_gtext( CHAR_DATA *ch, char *argument )
{
    switch ( ch->substate ) {
        default:
            break;
        case SUB_RESTRICTED:
            send_to_char( "You cannot do this while in another command.\r\n", ch );
            return;
        case SUB_GNOTE:
            if ( ch->substate == SUB_REPEATCMD )
                ch->tempnum = SUB_REPEATCMD;
            else
                ch->tempnum = SUB_NONE;
            ch->pcdata->in_progress = ( NOTE_DATA * ) ch->dest_buf;
            STRFREE( ch->pcdata->in_progress->text );
            ch->pcdata->in_progress->text = copy_buffer( ch );
            stop_editing( ch );
            ch->substate = ch->tempnum;
            finish_note( ch->pcdata->board, ch->pcdata->in_progress );
            ch->pcdata->in_progress = NULL;
            save_board( ch->pcdata->board );           /* save the board */
            return;
    }

    if ( !str_cmp( argument, "write" ) ) {
        if ( ch->substate == SUB_RESTRICTED ) {
            send_to_char( "You cannot write a note from within another command.\r\n", ch );
            return;
        }
        if ( ch->pcdata->board == NULL )
            ch->pcdata->board = &boards[DEFAULT_BOARD];
        if ( get_trust( ch ) < ch->pcdata->board->write_level ) {
            send_to_char( "You cannot post notes on this board.\r\n", ch );
            return;
        }
        ch->substate = SUB_GNOTE;
        ch->dest_buf = ch->pcdata->in_progress;
        start_editing( ch, ch->pcdata->in_progress->text );
        set_editor_desc( ch, "A global note." );
        return;
    }
    return;
}

void handle_con_note_to( DESCRIPTOR_DATA *d, char *argument )
{
    CHAR_DATA              *ch = d->character;

    if ( !ch->pcdata->in_progress ) {
        d->connected = CON_PLAYING;
        bug( "%s", "nanny: In CON_NOTE_TO, but no note in progress" );
        return;
    }
    smash_tilde( argument );                           /* change ~ to - as we save this
                                                        * field as a string later */
    if ( strlen( argument ) > 60 ) {
        send_to_char( "No, no. This is just the To List. You're not writing the note yet.\r\n",
                      ch );
        return;
    }
    switch ( ch->pcdata->board->force_type ) {
        case DEF_NORMAL:                              /* default field */
            if ( !VLD_STR( argument ) ) {              /* empty string? */
                ch->pcdata->in_progress->to_list = STRALLOC( ch->pcdata->board->names );
                ch_printf( ch, "Assumed default recipient: &W%s&w\r\n", ch->pcdata->board->names );
            }
            else
                ch->pcdata->in_progress->to_list = STRALLOC( argument );
            break;
        case DEF_INCLUDE:                             /* forced default toall */
            if ( !is_name( ch->pcdata->board->names, argument ) ) {
                mudstrlcat( argument, " ", MSL );
                mudstrlcat( argument, ch->pcdata->board->names, MSL );
                ch->pcdata->in_progress->to_list = STRALLOC( argument );
                send_to_char( "&cDefault recipient assigned.\r\n", ch );
            }
            else
                ch->pcdata->in_progress->to_list = STRALLOC( argument );
            break;
        case DEF_EXCLUDE:                             /* forced exclude */
            if ( !VLD_STR( argument ) ) {
                send_to_char( "You must specify a recipient.\r\n&YTo&w:      ", ch );
                return;
            }
            if ( is_name( ch->pcdata->board->names, argument ) ) {
                ch_printf( ch,
                           "You are not allowed to send notes to %s on this board. Try again.\r\n"
                           "&YTo&w:      ", ch->pcdata->board->names );
                return;                                /* return from nanny, not changing 
                                                        * to the next state! */
            }
            else
                ch->pcdata->in_progress->to_list = STRALLOC( argument );
            break;
    }
    send_to_char( "&c\r\nType the subject of this note&C: ", ch );
    d->connected = CON_NOTE_SUBJECT;
    return;
}

void handle_con_note_subject( DESCRIPTOR_DATA *d, char *argument )
{
    CHAR_DATA              *ch = d->character;

    if ( !ch->pcdata->in_progress ) {
        d->connected = CON_PLAYING;
        bug( "%s", "nanny: In CON_NOTE_SUBJECT, but no note in progress" );
        return;
    }
    smash_tilde( argument );                           /* change ~ to - as we save this
                                                        * field as a string later */
    /*
     * Do not allow empty subjects 
     */
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Please find a meaningful subject!\r\n", ch );
        send_to_char( "&cThe subject of this note is&C: ", ch );
    }
    else if ( strlen( argument ) > 60 ) {
        send_to_char( "No, no. This is just the Subject. You're note writing the note yet.\r\n",
                      ch );
    }
    else {                                             /* advance to next stage */

        ch->pcdata->in_progress->subject = STRALLOC( argument );
        if ( IS_IMMORTAL( ch ) ) {                     /* immortals get to choose number
                                                        * of expire days */
            ch_printf( ch,
                       "\r\n&cHow many days do you want this note to expire in?\r\n"
                       "Press Enter for default value for this board, &C%d&c days.\r\n"
                       "&YExpire&w:  ", ch->pcdata->board->purge_days );
            d->connected = CON_NOTE_EXPIRE;
        }
        else {
            ch->pcdata->in_progress->expire =
                current_time + ch->pcdata->board->purge_days * 24L * 3600L;
            // ch_printf(ch, "&cThis note will expire &C%s&c\r",
            // ctime(&ch->pcdata->in_progress->expire));
            do_gtext( ch, ( char * ) "write" );
        }
    }
    return;
}

void handle_con_note_expire( DESCRIPTOR_DATA *d, char *argument )
{
    CHAR_DATA              *ch = d->character;
    time_t                  expire;
    int                     days;

    if ( !ch->pcdata->in_progress ) {
        d->connected = CON_PLAYING;
        bug( "%s", "nanny: In CON_NOTE_EXPIRE, but no note in progress" );
        return;
    }
    /*
     * Numeric argument. no tilde smashing 
     */
    if ( !VLD_STR( argument ) )                        /* assume default expire */
        days = ch->pcdata->board->purge_days;
    else if ( is_number( argument ) )
        days = atoi( argument );
    else
        days = ch->pcdata->board->purge_days;
    if ( days <= 0 ) {
        send_to_char( "Write the number of days!\r\n", ch );
        send_to_char( "&YExpire&w:  ", ch );
        return;
    }
    expire = current_time + ( days * 24L * 3600L );    /* 24 hours, 3600 seconds */
    ch->pcdata->in_progress->expire = expire;
    /*
     * note that ctime returns XXX\n so we only need to add an \r 
     */
    do_gtext( ch, ( char * ) "write" );
    return;
}

#undef VOTE_NONE
#undef VOTE_OPEN
#undef VOTE_CLOSED
#undef NOTE_DIR
#undef DEF_NORMAL
#undef DEF_INCLUDE
#undef DEF_EXCLUDE
#undef MAX_LINE_LENGTH
#undef MAX_NOTE_TEXT
#undef L_SUP
#undef BOARD_NOACCESS
#undef BOARD_NOTFOUND
