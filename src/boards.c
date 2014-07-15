/* LoP code team: Remcon, Paradigm                                          *
 * Special thinks go out to all these people for verious snippets used      *
 *   in this codebase.                                                      *
 * Samson, Cronel, Kratas, John 'Noplex' Bellone, Desden el Chaman Tibetano,*
 *   Orion, Scion, Xerves, Whir, Belgarath, Zarius, Gangien, Desden, Cid,   *
 *   John Strange, Cyrus, Robcon, Xorith, and Jay Roman                     *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,                    *
 *   Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,                  *
 *   Tricops and Fireblade                                                  *
 * Merc 2.1 Diku Mud improvments copyright(C)1992, 1993 by Michael          *
 *   Chastain, Michael Quan, and Mitchell Tse.                              *
 * Original Diku Mud copyright(C)1990, 1991 by Sebastian Hammer,            *
 *   Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
*/
#include "h/mud.h"
#include "h/files.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "h/key.h"

#define OBJ_VNUM_NOTE 36

void                    free_note( NOTE_DATA *pnote );
void                    free_board( BOARD_DATA * board );
void                    stralloc_printf( char **pointer, const char *fmt, ... );

/* Defines for voting on notes. -- Narn */
#define VOTE_NONE 0
#define VOTE_OPEN 1
#define VOTE_CLOSED 2

BOARD_DATA             *first_board;
BOARD_DATA             *last_board;

bool                    is_note_to( CHAR_DATA *ch, NOTE_DATA *pnote );
void                    note_attach( CHAR_DATA *ch );
void                    note_remove( CHAR_DATA *ch, BOARD_DATA * board, NOTE_DATA *pnote );
void                    do_note( CHAR_DATA *ch, char *arg_passed, bool IS_MAIL );

/* Returns TRUE if note is addressed to ch */
bool is_note_to( CHAR_DATA *ch, NOTE_DATA *note )
{
    if ( !note )
        return FALSE;
    if ( !str_cmp( ch->name, note->sender ) )
        return TRUE;
    if ( is_name( "all", note->to_list ) )
        return TRUE;
    if ( IS_IMMORTAL( ch ) && ( is_name( "imm", note->to_list )
                                || is_name( "imms", note->to_list )
                                || is_name( "immortal", note->to_list )
                                || is_name( "god", note->to_list )
                                || is_name( "gods", note->to_list )
                                || is_name( "staff", note->to_list )
                                || is_name( "admin", note->to_list )
                                || is_name( "staffs", note->to_list )
                                || is_name( "admins", note->to_list )
                                || is_name( "immortals", note->to_list ) ) )
        return TRUE;
    if ( ( get_trust( ch ) == MAX_LEVEL )
         && ( is_name( "imp", note->to_list ) || is_name( "imps", note->to_list )
              || is_name( "implementor", note->to_list )
              || is_name( "implementors", note->to_list ) ) )
        return TRUE;
    if ( is_name( ch->name, note->to_list ) )
        return TRUE;
    /*
     * Allow a note to e.g. 40 to send to characters level 40 and above 
     */
    if ( is_number( note->to_list ) && get_trust( ch ) >= atoi( note->to_list ) )
        return TRUE;
    /*
     * Allow MAX_LEVEL - 1 and higher to see all notes to make sure
     * * nothing is being done that shouldn't be 
     */
    if ( get_trust( ch ) >= ( MAX_LEVEL - 1 ) )
        return TRUE;
    return FALSE;
}

void free_board( BOARD_DATA * board )
{
    NOTE_DATA              *note,
                           *next_note;

    STRFREE( board->note_file );
    STRFREE( board->read_group );
    STRFREE( board->post_group );
    STRFREE( board->extra_readers );
    STRFREE( board->extra_removers );
    for ( note = board->first_note; note; note = next_note ) {
        next_note = note->next;
        free_note( note );
    }
    UNLINK( board, first_board, last_board, next, prev );
    DISPOSE( board );
    return;
}

bool can_remove( CHAR_DATA *ch, BOARD_DATA * board )
{
    /*
     * If your trust is high enough, you can remove it. 
     */
    if ( get_trust( ch ) >= board->min_remove_level )
        return TRUE;
    if ( VLD_STR( board->extra_removers ) )
        if ( is_name( ch->name, board->extra_removers ) )
            return TRUE;
    return FALSE;
}

bool can_read( CHAR_DATA *ch, BOARD_DATA * board )
{
    /*
     * If your trust is high enough, you can read it. 
     */
    if ( get_trust( ch ) >= board->min_read_level )
        return TRUE;
    /*
     * Your trust wasn't high enough, so check if a read_group or extra 
     * * readers have been set up. 
     */
/*
  if(VLD_STR(board->read_group))
  {
    if(ch->pcdata->clan && !str_cmp(ch->pcdata->clan->name, board->read_group)) 
      return TRUE; 
  }
*/
    if ( VLD_STR( board->extra_readers ) )
        if ( is_name( ch->name, board->extra_readers ) )
            return TRUE;
    return FALSE;
}

bool can_post( CHAR_DATA *ch, BOARD_DATA * board )
{
    /*
     * If your trust is high enough, you can post. 
     */
    if ( get_trust( ch ) >= board->min_post_level )
        return TRUE;
    /*
     * Your trust wasn't high enough, so check if a post_group has been set up. 
     */
/*
  if(VLD_STR(board->post_group))
  {
    if(ch->pcdata->clan && !str_cmp(ch->pcdata->clan->name, board->post_group)) 
      return TRUE; 
  }
*/
    return FALSE;
}

/* board commands. */
void write_boards_txt( void )
{
    BOARD_DATA             *tboard;
    FILE                   *fpout;
    char                    filename[256];

    snprintf( filename, 256, "%s", BOARD_FILE );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "%s", "FATAL: cannot open board.dat for writing!\r\n" );
        return;
    }
    for ( tboard = first_board; tboard; tboard = tboard->next ) {
        fprintf( fpout, "Filename          %s~\n", tboard->note_file );
        fprintf( fpout, "Vnum              %d\n", tboard->board_obj );
        fprintf( fpout, "Min_read_level    %d\n", tboard->min_read_level );
        fprintf( fpout, "Min_post_level    %d\n", tboard->min_post_level );
        fprintf( fpout, "Min_remove_level  %d\n", tboard->min_remove_level );
        fprintf( fpout, "Max_posts         %d\n", tboard->max_posts );
        fprintf( fpout, "Type              %d\n", tboard->type );
        fprintf( fpout, "Read_group        %s~\n", tboard->read_group );
        fprintf( fpout, "Post_group        %s~\n", tboard->post_group );
        fprintf( fpout, "Extra_readers     %s~\n", tboard->extra_readers );
        fprintf( fpout, "Extra_removers    %s~\n", tboard->extra_removers );
        if ( tboard->ocopymessg )
            fprintf( fpout, "OCopymessg        %s~\n", tboard->ocopymessg );
        if ( tboard->olistmessg )
            fprintf( fpout, "OListmessg        %s~\n", tboard->olistmessg );
        if ( tboard->opostmessg )
            fprintf( fpout, "OPostmessg        %s~\n", tboard->opostmessg );
        if ( tboard->oreadmessg )
            fprintf( fpout, "OReadmessg        %s~\n", tboard->oreadmessg );
        if ( tboard->oremovemessg )
            fprintf( fpout, "ORemovemessg      %s~\n", tboard->oremovemessg );
        if ( tboard->otakemessg )
            fprintf( fpout, "OTakemessg        %s~\n", tboard->otakemessg );
        if ( tboard->postmessg )
            fprintf( fpout, "Postmessg         %s~\n", tboard->postmessg );
        fprintf( fpout, "End\n\n" );
    }
    FileClose( fpout );
    return;
}

BOARD_DATA             *get_board( OBJ_DATA *obj )
{
    BOARD_DATA             *board;

    for ( board = first_board; board; board = board->next )
        if ( board->board_obj == obj->pIndexData->vnum )
            return board;
    return NULL;
}

BOARD_DATA             *find_board( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    BOARD_DATA             *board;

    for ( obj = ch->in_room->first_content; obj; obj = obj->next_content )
        if ( ( board = get_board( obj ) ) != NULL )
            return board;
    return NULL;
}

void note_attach( CHAR_DATA *ch )
{
    NOTE_DATA              *pnote;

    if ( ch->pnote )
        return;
    CREATE( pnote, NOTE_DATA, 1 );
    pnote->next = NULL;
    pnote->prev = NULL;
    pnote->sender = QUICKLINK( ch->name );
    ch->pnote = pnote;
    return;
}

void write_board( BOARD_DATA * board )
{
    FILE                   *fp;
    char                    filename[256];
    NOTE_DATA              *pnote;

    /*
     * Rewrite entire list. 
     */
    snprintf( filename, 256, "%s%s", BOARD_DIR, board->note_file );
    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        perror( filename );
    }
    else {
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( pnote->sender )
                fprintf( fp, "Sender      %s~\n", pnote->sender );
            if ( pnote->date )
                fprintf( fp, "Date        %s~\n", pnote->date );
            if ( pnote->to_list )
                fprintf( fp, "To          %s~\n", pnote->to_list );
            if ( pnote->subject )
                fprintf( fp, "Subject     %s~\n", pnote->subject );
            fprintf( fp, "Voting      %d\n", pnote->voting );
            if ( pnote->yesvotes )
                fprintf( fp, "Yesvotes    %s~\n", pnote->yesvotes );
            if ( pnote->novotes )
                fprintf( fp, "Novotes     %s~\n", pnote->novotes );
            if ( pnote->abstentions )
                fprintf( fp, "Abstentions %s~\n", pnote->abstentions );
            if ( pnote->text )
                fprintf( fp, "Text\n%s~\n", strip_cr( pnote->text ) );
            fprintf( fp, "Ctime %ld\n", pnote->current );
            fprintf( fp, "End\n\n" );
        }
        FileClose( fp );
    }
    return;
}

void free_note( NOTE_DATA *pnote )
{
    STRFREE( pnote->text );
    STRFREE( pnote->subject );
    STRFREE( pnote->to_list );
    STRFREE( pnote->date );
    STRFREE( pnote->sender );
    if ( pnote->yesvotes )
        STRFREE( pnote->yesvotes );
    if ( pnote->novotes )
        STRFREE( pnote->novotes );
    if ( pnote->abstentions )
        STRFREE( pnote->abstentions );
    DISPOSE( pnote );
    return;
}

void note_remove( CHAR_DATA *ch, BOARD_DATA * board, NOTE_DATA *pnote )
{
    if ( !board ) {
        bug( "%s", "note remove: null board" );
        return;
    }
    if ( !pnote ) {
        bug( "%s", "note remove: null pnote" );
        return;
    }
    /*
     * Remove note from linked list. 
     */
    UNLINK( pnote, board->first_note, board->last_note, next, prev );
    --board->num_posts;
    free_note( pnote );
    write_board( board );
    return;
}

OBJ_DATA               *find_quill( CHAR_DATA *ch )
{
    OBJ_DATA               *quill;

    for ( quill = ch->last_carrying; quill; quill = quill->prev_content )
        if ( quill->item_type == ITEM_PEN && can_see_obj( ch, quill ) )
            return quill;
    return NULL;
}

void do_noteroom( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    char                    arg[MSL],
                            arg_passed[MSL];

    if ( !ch || IS_NPC( ch ) )
        return;
    mudstrlcpy( arg_passed, argument, MSL );
    switch ( ch->substate ) {
        case SUB_WRITING_NOTE:
            do_note( ch, arg_passed, FALSE );
            break;
        default:
            argument = one_argument( argument, arg );
            smash_tilde( argument );
            if ( !str_cmp( arg, "write" ) || !str_cmp( arg, "to" ) || !str_cmp( arg, "subject" )
                 || !str_cmp( arg, "show" ) ) {
                do_note( ch, arg_passed, FALSE );
                return;
            }
            board = find_board( ch );
            if ( !board ) {
                send_to_char( "There is no bulletin board here to look at.\r\n", ch );
                return;
            }
            if ( board->type != BOARD_NOTE ) {
                send_to_char( "You can only use note commands on a note board.\r\n", ch );
                return;
            }
            else {
                do_note( ch, arg_passed, FALSE );
                return;
            }
            break;
    }
    return;
}

void do_mailroom( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    char                    arg[MSL],
                            arg_passed[MSL];

    if ( !ch || IS_NPC( ch ) )
        return;
    mudstrlcpy( arg_passed, argument, MSL );
    switch ( ch->substate ) {
        case SUB_WRITING_NOTE:
            do_note( ch, arg_passed, TRUE );
            break;
        default:
            argument = one_argument( argument, arg );
            smash_tilde( argument );
            if ( !str_cmp( arg, "write" ) || !str_cmp( arg, "to" ) || !str_cmp( arg, "subject" )
                 || !str_cmp( arg, "show" ) ) {
                do_note( ch, arg_passed, TRUE );
                return;
            }
            board = find_board( ch );
            if ( !board ) {
                send_to_char( "There is no mail facility here.\r\n", ch );
                return;
            }
            if ( board->type != BOARD_MAIL ) {
                send_to_char( "You can only use mail commands in a post office.\r\n", ch );
                return;
            }
            else {
                do_note( ch, arg_passed, TRUE );
                return;
            }
            break;
    }
    return;
}

void do_note( CHAR_DATA *ch, char *arg_passed, bool IS_MAIL )
{
    char                    buf[MSL],
                            arg[MIL];
    NOTE_DATA              *pnote;
    BOARD_DATA             *board;
    int                     vnum,
                            anum,
                            first_list;
    OBJ_DATA               *quill = NULL,
        *paper = NULL,
        *tmpobj = NULL;
    EXTRA_DESCR_DATA       *ed = NULL;
    bool                    mfound = FALSE;

    if ( !ch || !ch->desc || IS_NPC( ch ) )
        return;
    switch ( ch->substate ) {
        default:
            break;
        case SUB_WRITING_NOTE:
            if ( ( paper = get_eq_char( ch, WEAR_HOLD ) ) == NULL
                 || paper->item_type != ITEM_PAPER ) {
                bug( "%s", "do_note: player not holding paper" );
                stop_editing( ch );
                return;
            }
            ed = ( EXTRA_DESCR_DATA * ) ( ch->dest_buf );
            if ( VLD_STR( ed->description ) )
                STRFREE( ed->description );
            ed->description = copy_buffer( ch );
            stop_editing( ch );
            return;
    }
    set_char_color( AT_NOTE, ch );
    arg_passed = one_argument( arg_passed, arg );
    smash_tilde( arg_passed );
    /*
     * Reusing alot of code but this will have to do till I can think
     * * of a better way to do it. --Shaddai 
     */
    if ( !str_cmp( arg, "date" ) ) {
        board = find_board( ch );
        if ( !board ) {
            send_to_char( "There is no board here to look at.\r\n", ch );
            return;
        }
        if ( !can_read( ch, board ) ) {
            send_to_char( "You cannot make any sense of the cryptic scrawl on this board...\r\n",
                          ch );
            return;
        }
        first_list = atoi( arg_passed );
        if ( first_list ) {
            if ( IS_MAIL ) {
                send_to_char( "You cannot use a list number (at this time) with mail.\r\n", ch );
                return;
            }
            if ( first_list < 1 ) {
                send_to_char( "You can't read a note before 1!\r\n", ch );
                return;
            }
        }
        if ( !IS_MAIL ) {
            set_pager_color( AT_NOTE, ch );
            vnum = 0;
            for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
                vnum++;
                if ( ( first_list && vnum >= first_list ) || !first_list )
                    pager_printf( ch, "%2d%c %-12s%c %-24s : %-35s\r\n", vnum,
                                  is_note_to( ch, pnote ) ? ')' : '}', pnote->sender,
                                  ( pnote->voting != VOTE_NONE ) ? ( pnote->voting ==
                                                                     VOTE_OPEN ? 'V' : 'C' ) : ':',
                                  pnote->date, pnote->subject );
            }
            if ( board->olistmessg )
                act( AT_ACTION, board->olistmessg, ch, NULL, NULL, TO_CANSEE );
            else
                act( AT_ACTION, "$n glances over the notes.", ch, NULL, NULL, TO_CANSEE );
            return;
        }
        else {
            vnum = 0;
            if ( IS_MAIL ) {                           /* SB Mail check for Brit */
                for ( pnote = board->first_note; pnote; pnote = pnote->next )
                    if ( is_note_to( ch, pnote ) )
                        mfound = TRUE;
                if ( !mfound && get_trust( ch ) < sysdata.read_all_mail ) {
                    send_to_char( "You have no mail.\r\n", ch );
                    return;
                }
            }
            for ( pnote = board->first_note; pnote; pnote = pnote->next )
                if ( is_note_to( ch, pnote ) || get_trust( ch ) >= sysdata.read_all_mail )
                    ch_printf( ch, "%2d%c %-12s: %-13s\r\n", ++vnum,
                               is_note_to( ch, pnote ) ? '-' : '}', pnote->sender, pnote->subject );
            return;
        }
    }
    if ( !str_cmp( arg, "list" ) ) {
        board = find_board( ch );
        if ( !board ) {
            send_to_char( "There is no board here to look at.\r\n", ch );
            return;
        }
        if ( !can_read( ch, board ) ) {
            send_to_char( "You cannot make any sense of the cryptic scrawl on this board...\r\n",
                          ch );
            return;
        }
        first_list = atoi( arg_passed );
        if ( first_list ) {
            if ( IS_MAIL ) {
                send_to_char( "You cannot use a list number (at this time) with mail.\r\n", ch );
                return;
            }
            if ( first_list < 1 ) {
                send_to_char( "You can't read a note before 1!\r\n", ch );
                return;
            }
        }
        if ( !IS_MAIL ) {
            set_pager_color( AT_NOTE, ch );
            vnum = 0;
            for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
                vnum++;
                if ( ( first_list && vnum >= first_list ) || !first_list )
                    pager_printf( ch, "%2d%c %-12s%c %-12.12s : %s\r\n", vnum,
                                  is_note_to( ch, pnote ) ? ')' : '}', pnote->sender,
                                  ( pnote->voting != VOTE_NONE ) ? ( pnote->voting ==
                                                                     VOTE_OPEN ? 'V' : 'C' ) : ':',
                                  pnote->to_list, pnote->subject );
            }
            if ( board->olistmessg )
                act( AT_ACTION, board->olistmessg, ch, NULL, NULL, TO_CANSEE );
            else
                act( AT_ACTION, "$n glances over the notes.", ch, NULL, NULL, TO_CANSEE );
            return;
        }
        else {
            vnum = 0;
            if ( IS_MAIL ) {                           /* SB Mail check for Brit */
                for ( pnote = board->first_note; pnote; pnote = pnote->next )
                    if ( is_note_to( ch, pnote ) )
                        mfound = TRUE;
                if ( !mfound && get_trust( ch ) < sysdata.read_all_mail ) {
                    send_to_char( "You have no mail.\r\n", ch );
                    return;
                }
            }
            for ( pnote = board->first_note; pnote; pnote = pnote->next )
                if ( is_note_to( ch, pnote ) || get_trust( ch ) >= sysdata.read_all_mail )
                    ch_printf( ch, "%2d%c %s: %s\r\n", ++vnum, is_note_to( ch, pnote ) ? '-' : '}',
                               pnote->sender, pnote->subject );
            return;
        }
    }
    if ( !str_cmp( arg, "read" ) ) {
        bool                    fAll;

        board = find_board( ch );
        if ( !board ) {
            send_to_char( "There is no board here to look at.\r\n", ch );
            return;
        }
        if ( !can_read( ch, board ) ) {
            send_to_char( "You cannot make any sense of the cryptic scrawl on this board...\r\n",
                          ch );
            return;
        }
        if ( !str_cmp( arg_passed, "all" ) ) {
            fAll = TRUE;
            anum = 0;
        }
        else if ( is_number( arg_passed ) ) {
            fAll = FALSE;
            anum = atoi( arg_passed );
        }
        else {
            send_to_char( "Note read which number?\r\n", ch );
            return;
        }
        set_pager_color( AT_NOTE, ch );
        if ( !IS_MAIL ) {
            vnum = 0;
            for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
                vnum++;
                if ( vnum == anum || fAll ) {
                    pager_printf( ch, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n%s", vnum, pnote->sender,
                                  pnote->subject, pnote->date, pnote->to_list, pnote->text );
                    if ( ( VLD_STR( pnote->yesvotes ) ) || ( VLD_STR( pnote->novotes ) )
                         || ( VLD_STR( pnote->abstentions ) ) ) {
                        send_to_pager
                            ( "------------------------------------------------------------\r\n",
                              ch );
                        pager_printf( ch, "Votes:\r\nYes:     %s\r\nNo:      %s\r\nAbstain: %s\r\n",
                                      pnote->yesvotes ? pnote->yesvotes : "",
                                      pnote->novotes ? pnote->novotes : "",
                                      pnote->abstentions ? pnote->abstentions : "" );
                    }
                    if ( board->oreadmessg )
                        act( AT_ACTION, board->oreadmessg, ch, NULL, NULL, TO_CANSEE );
                    else
                        act( AT_ACTION, "$n reads a note.", ch, NULL, NULL, TO_CANSEE );
                    return;
                }
            }
            send_to_char( "No such note.\r\n", ch );
            return;
        }
        else {
            vnum = 0;
            for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
                if ( is_note_to( ch, pnote ) || get_trust( ch ) >= sysdata.read_all_mail ) {
                    vnum++;
                    if ( vnum == anum || fAll ) {
                        pager_printf( ch, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n%s", vnum, pnote->sender,
                                      pnote->subject, pnote->date, pnote->to_list, pnote->text );
                        return;
                    }
                }
            }
            send_to_char( "No such message.\r\n", ch );
            return;
        }
    }
    /*
     * Voting added by Narn, June '96 
     */
    if ( !str_cmp( arg, "vote" ) ) {
        char                    arg2[MIL];

        arg_passed = one_argument( arg_passed, arg2 );

        board = find_board( ch );
        if ( !board ) {
            send_to_char( "There is no bulletin board here.\r\n", ch );
            return;
        }
        if ( !can_read( ch, board ) ) {
            send_to_char( "You cannot vote on this board.\r\n", ch );
            return;
        }
        if ( is_number( arg2 ) )
            anum = atoi( arg2 );
        else {
            send_to_char( "Note vote which number?\r\n", ch );
            return;
        }
        vnum = 1;
        for ( pnote = board->first_note; pnote && vnum < anum; pnote = pnote->next )
            vnum++;
        if ( !pnote ) {
            send_to_char( "No such note.\r\n", ch );
            return;
        }
        /*
         * Options: open close yes no abstain 
         */
        /*
         * If you're the author of the note and can read the board you can open 
         * * and close voting, if you can read it and voting is open you can vote. 
         */
        if ( !str_cmp( arg_passed, "open" ) ) {
            if ( str_cmp( ch->name, pnote->sender ) ) {
                send_to_char( "You are not the author of this note.\r\n", ch );
                return;
            }
            pnote->voting = VOTE_OPEN;
            act( AT_ACTION, "$n opens voting on a note.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "Voting opened.\r\n", ch );
            write_board( board );
            return;
        }
        if ( !str_cmp( arg_passed, "close" ) ) {
            if ( str_cmp( ch->name, pnote->sender ) ) {
                send_to_char( "You are not the author of this note.\r\n", ch );
                return;
            }
            pnote->voting = VOTE_CLOSED;
            act( AT_ACTION, "$n closes voting on a note.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "Voting closed.\r\n", ch );
            write_board( board );
            return;
        }
        /*
         * Make sure the note is open for voting before going on. 
         */
        if ( pnote->voting != VOTE_OPEN ) {
            send_to_char( "Voting is not open on this note.\r\n", ch );
            return;
        }
        /*
         * Can only vote once on a note. 
         */
        snprintf( buf, MSL, "%s %s %s", pnote->yesvotes, pnote->novotes, pnote->abstentions );
        if ( is_name( ch->name, buf ) ) {
            send_to_char( "You have already voted on this note.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg_passed, "yes" ) ) {
            if ( VLD_STR( pnote->yesvotes ) )
                stralloc_printf( &( pnote->yesvotes ), "%s %s", pnote->yesvotes, ch->name );
            else
                stralloc_printf( &( pnote->yesvotes ), "%s", ch->name );
            act( AT_ACTION, "$n votes on a note.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "Ok.\r\n", ch );
            write_board( board );
            return;
        }
        if ( !str_cmp( arg_passed, "no" ) ) {
            if ( VLD_STR( pnote->novotes ) )
                stralloc_printf( &( pnote->novotes ), "%s %s", pnote->novotes, ch->name );
            else
                stralloc_printf( &( pnote->novotes ), "%s", ch->name );
            act( AT_ACTION, "$n votes on a note.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "Ok.\r\n", ch );
            write_board( board );
            return;
        }
        if ( !str_cmp( arg_passed, "abstain" ) ) {
            if ( VLD_STR( pnote->abstentions ) )
                stralloc_printf( &( pnote->abstentions ), "%s %s", pnote->abstentions, ch->name );
            else
                stralloc_printf( &( pnote->abstentions ), "%s", ch->name );
            act( AT_ACTION, "$n votes on a note.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "Ok.\r\n", ch );
            write_board( board );
            return;
        }
        do_note( ch, ( char * ) "", FALSE );
    }
    if ( !str_cmp( arg, "write" ) ) {
        if ( ch->substate == SUB_RESTRICTED ) {
            send_to_char( "You cannot write a note from within another command.\r\n", ch );
            return;
        }
        if ( get_trust( ch ) < sysdata.write_mail_free ) {
            quill = find_quill( ch );
            if ( !quill ) {
                send_to_char( "You need a quill to write a note.\r\n", ch );
                return;
            }
            if ( quill->value[0] < 1 ) {
                send_to_char( "Your quill is dry.\r\n", ch );
                return;
            }
        }
        if ( ( paper = get_eq_char( ch, WEAR_HOLD ) ) == NULL || paper->item_type != ITEM_PAPER ) {
            if ( get_trust( ch ) < sysdata.write_mail_free ) {
                send_to_char
                    ( "You need to be holding a fresh piece of parchment to write a note.\r\n",
                      ch );
                return;
            }
            if ( ( paper = create_object( get_obj_index( OBJ_VNUM_NOTE ), 0 ) ) == NULL ) {
                bug( "do_note: Couldn't create_object [%d]", OBJ_VNUM_NOTE );
                return;
            }
            if ( ( tmpobj = get_eq_char( ch, WEAR_HOLD ) ) != NULL )
                unequip_char( ch, tmpobj );
            paper = obj_to_char( paper, ch );
            equip_char( ch, paper, WEAR_HOLD );
            act( AT_MAGIC, "A piece of parchment magically appears in $n's hands!", ch, NULL, NULL,
                 TO_ROOM );
            act( AT_MAGIC, "A piece of parchment appears in your hands.", ch, NULL, NULL, TO_CHAR );
        }
        if ( paper->value[0] < 2 ) {
            paper->value[0] = 1;
            ed = SetOExtra( paper, ( char * ) "_text_" );
            ch->substate = SUB_WRITING_NOTE;
            ch->dest_buf = ed;
            if ( get_trust( ch ) < sysdata.write_mail_free )
                --quill->value[0];
            start_editing( ch, ed->description );
            {
                char                   *subject,
                                       *to_list;

                if ( ( subject = get_extra_descr( "_subject_", paper->first_extradesc ) ) == NULL )
                    subject = ( char * ) "(no subject)";
                if ( ( to_list = get_extra_descr( "_to_", paper->first_extradesc ) ) == NULL )
                    to_list = ( char * ) "(nobody)";
                editor_desc_printf( ch, "Note entitled '%s', addressed to %s.", subject, to_list );
            }
            return;
        }
        else {
            send_to_char( "You cannot modify this note.\r\n", ch );
            return;
        }
    }
    if ( !str_cmp( arg, "subject" ) ) {
        if ( get_trust( ch ) < sysdata.write_mail_free ) {
            quill = find_quill( ch );
            if ( !quill ) {
                send_to_char( "You need a quill to write a note.\r\n", ch );
                return;
            }
            if ( quill->value[0] < 1 ) {
                send_to_char( "Your quill is dry.\r\n", ch );
                return;
            }
        }
        if ( !VLD_STR( arg_passed ) ) {
            send_to_char( "What do you wish the subject to be?\r\n", ch );
            return;
        }
        if ( ( paper = get_eq_char( ch, WEAR_HOLD ) ) == NULL || paper->item_type != ITEM_PAPER ) {
            if ( get_trust( ch ) < sysdata.write_mail_free ) {
                send_to_char
                    ( "You need to be holding a fresh piece of parchment to write a note.\r\n",
                      ch );
                return;
            }
            if ( ( paper = create_object( get_obj_index( OBJ_VNUM_NOTE ), 0 ) ) == NULL ) {
                bug( "do_note: Couldn't create_object [%d]", OBJ_VNUM_NOTE );
                return;
            }
            if ( ( tmpobj = get_eq_char( ch, WEAR_HOLD ) ) != NULL )
                unequip_char( ch, tmpobj );
            paper = obj_to_char( paper, ch );
            equip_char( ch, paper, WEAR_HOLD );
            act( AT_MAGIC, "A piece of parchment magically appears in $n's hands!", ch, NULL, NULL,
                 TO_ROOM );
            act( AT_MAGIC, "A piece of parchment appears in your hands.", ch, NULL, NULL, TO_CHAR );
        }
        if ( paper->value[1] > 1 ) {
            send_to_char( "You cannot modify this note.\r\n", ch );
            return;
        }
        else {
            paper->value[1] = 1;
            ed = SetOExtra( paper, ( char * ) "_subject_" );
            STRFREE( ed->description );
            ed->description = STRALLOC( arg_passed );
            send_to_char( "Ok.\r\n", ch );
            return;
        }
    }
    if ( !str_cmp( arg, "to" ) ) {
        struct stat             fst;
        char                    fname[1024];

        if ( get_trust( ch ) < sysdata.write_mail_free ) {
            quill = find_quill( ch );
            if ( !quill ) {
                send_to_char( "You need a quill to write a note.\r\n", ch );
                return;
            }
            if ( quill->value[0] < 1 ) {
                send_to_char( "Your quill is dry.\r\n", ch );
                return;
            }
        }
        if ( !VLD_STR( arg_passed ) ) {
            send_to_char( "Please specify an addressee.\r\n", ch );
            return;
        }
        if ( ( paper = get_eq_char( ch, WEAR_HOLD ) ) == NULL || paper->item_type != ITEM_PAPER ) {
            if ( get_trust( ch ) < sysdata.write_mail_free ) {
                send_to_char
                    ( "You need to be holding a fresh piece of parchment to write a note.\r\n",
                      ch );
                return;
            }
            if ( ( paper = create_object( get_obj_index( OBJ_VNUM_NOTE ), 0 ) ) == NULL ) {
                bug( "do_note: Couldn't create_object [%d]", OBJ_VNUM_NOTE );
                return;
            }
            if ( ( tmpobj = get_eq_char( ch, WEAR_HOLD ) ) != NULL )
                unequip_char( ch, tmpobj );
            paper = obj_to_char( paper, ch );
            equip_char( ch, paper, WEAR_HOLD );
            act( AT_MAGIC, "A piece of parchment magically appears in $n's hands!", ch, NULL, NULL,
                 TO_ROOM );
            act( AT_MAGIC, "A piece of parchment appears in your hands.", ch, NULL, NULL, TO_CHAR );
        }
        if ( paper->value[2] > 1 ) {
            send_to_char( "You cannot modify this note.\r\n", ch );
            return;
        }
        arg_passed[0] = UPPER( arg_passed[0] );
        snprintf( fname, 1024, "%s%c/%s", PLAYER_DIR, tolower( arg_passed[0] ),
                  capitalize( arg_passed ) );
        if ( !IS_MAIL || stat( fname, &fst ) != -1 || !str_cmp( arg_passed, "all" ) ) {
            paper->value[2] = 1;
            ed = SetOExtra( paper, ( char * ) "_to_" );
            STRFREE( ed->description );
            ed->description = STRALLOC( arg_passed );
            send_to_char( "Ok.\r\n", ch );
            return;
        }
        else {
            send_to_char( "No player exists by that name.\r\n", ch );
            return;
        }
    }
    if ( !str_cmp( arg, "show" ) ) {
        char                   *subject,
                               *to_list,
                               *text;

        if ( ( paper = get_eq_char( ch, WEAR_HOLD ) ) == NULL || paper->item_type != ITEM_PAPER ) {
            send_to_char( "You are not holding a note.\r\n", ch );
            return;
        }
        subject = get_extra_descr( "_subject_", paper->first_extradesc );
        if ( subject == NULL || !str_cmp( "", subject ) )
            subject = ( char * ) "(no subject)";
        to_list = get_extra_descr( "_to_", paper->first_extradesc );
        if ( to_list == NULL || !str_cmp( "", to_list ) )
            to_list = ( char * ) "(nobody)";
        text = get_extra_descr( "_text_", paper->first_extradesc );
        if ( text == NULL || !str_cmp( "", text ) )
            text = ( char * ) "The note is blank.\r\n";
        pager_printf( ch, "%s: %s\r\nTo: %s\r\n%s", ch->name, subject, to_list, text );
        return;
    }
    if ( !str_cmp( arg, "post" ) ) {
        char                   *strtime,
                               *to,
                               *subj,
                               *text /* , *np = NULL */ ;

        if ( ( paper = get_eq_char( ch, WEAR_HOLD ) ) == NULL || paper->item_type != ITEM_PAPER ) {
            send_to_char( "You are not holding a note.\r\n", ch );
            return;
        }
        if ( paper->value[0] == 0 ) {
            send_to_char( "There is nothing written on this note.\r\n", ch );
            return;
        }
        if ( paper->value[1] == 0 ) {
            send_to_char( "This note has no subject.\r\n", ch );
            return;
        }
        if ( paper->value[2] == 0 ) {
            send_to_char( "This note is addressed to no one!\r\n", ch );
            return;
        }
        strtime = ctime( &current_time );
        strtime[strlen( strtime ) - 1] = '\0';
        to = get_extra_descr( "_to_", paper->first_extradesc );
        subj = get_extra_descr( "_subject_", paper->first_extradesc );
        text = get_extra_descr( "_text_", paper->first_extradesc );
        board = find_board( ch );
        if ( !board ) {
            send_to_char( "There is no bulletin board here to post your note on.\r\n", ch );
            return;
        }
        if ( !can_post( ch, board ) ) {
            send_to_char( "A magical force prevents you from posting your note here...\r\n", ch );
            return;
        }
        if ( board->num_posts >= board->max_posts ) {
            send_to_char( "There is no room on this board to post your note.\r\n", ch );
            return;
        }
        if ( board->opostmessg )
            act( AT_ACTION, board->opostmessg, ch, NULL, NULL, TO_ROOM );
        else
            act( AT_ACTION, "$n posts a note.", ch, NULL, NULL, TO_ROOM );
        CREATE( pnote, NOTE_DATA, 1 );

        pnote->date = STRALLOC( strtime );
        pnote->current = current_time;
        pnote->to_list = to ? STRALLOC( to ) : STRALLOC( "all" );
        if ( VLD_STR( text ) )
            pnote->text = STRALLOC( text );
        if ( VLD_STR( subj ) )
            pnote->subject = STRALLOC( subj );
        pnote->sender = QUICKLINK( ch->name );
        pnote->voting = 0;
        LINK( pnote, board->first_note, board->last_note, next, prev );
        board->num_posts++;
        write_board( board );
        if ( board->postmessg )
            act( AT_ACTION, board->postmessg, ch, NULL, NULL, TO_CHAR );
        else
            send_to_char( "You post your note on the board.\r\n", ch );
        extract_obj( paper );
        return;
    }
    if ( !str_cmp( arg, "remove" ) || !str_cmp( arg, "take" ) || !str_cmp( arg, "copy" ) ) {
        char                    take;

        board = find_board( ch );
        if ( !board ) {
            send_to_char( "There is no board here to take a note from!\r\n", ch );
            return;
        }
        if ( !str_cmp( arg, "take" ) )
            take = 1;
        else if ( !str_cmp( arg, "copy" ) ) {
            if ( !IS_IMMORTAL( ch ) ) {
                send_to_char( "Huh?  Type 'help note' for usage.\r\n", ch );
                return;
            }
            take = 2;
        }
        else
            take = 0;
        if ( !is_number( arg_passed ) ) {
            send_to_char( "Note remove which number?\r\n", ch );
            return;
        }
        if ( !can_read( ch, board ) ) {
            send_to_char
                ( "You can't make any sense of what's posted here, let alone remove anything!\r\n",
                  ch );
            return;
        }
        anum = atoi( arg_passed );
        vnum = 0;
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( IS_MAIL
                 && ( ( is_note_to( ch, pnote ) ) || get_trust( ch ) >= sysdata.take_others_mail ) )
                vnum++;
            else if ( !IS_MAIL )
                vnum++;
            if ( ( is_note_to( ch, pnote ) || can_remove( ch, board ) ) && ( vnum == anum ) ) {
                if ( ( is_name( "all", pnote->to_list ) )
                     && ( get_trust( ch ) < sysdata.take_others_mail ) && ( take == 1 ) ) {
                    send_to_char( "Notes addressed to 'all' can not be taken.\r\n", ch );
                    return;
                }
                if ( take != 0 ) {
                    if ( ( paper = create_object( get_obj_index( OBJ_VNUM_NOTE ), 0 ) ) == NULL ) {
                        bug( "do_note: Couldn't create_object [%d]", OBJ_VNUM_NOTE );
                        return;
                    }
                    ed = SetOExtra( paper, ( char * ) "_sender_" );
                    STRFREE( ed->description );
                    ed->description = QUICKLINK( pnote->sender );
                    ed = SetOExtra( paper, ( char * ) "_text_" );
                    STRFREE( ed->description );
                    ed->description = QUICKLINK( pnote->text );
                    ed = SetOExtra( paper, ( char * ) "_to_" );
                    STRFREE( ed->description );
                    ed->description = QUICKLINK( pnote->to_list );
                    ed = SetOExtra( paper, ( char * ) "_subject_" );
                    STRFREE( ed->description );
                    ed->description = QUICKLINK( pnote->subject );
                    ed = SetOExtra( paper, ( char * ) "_date_" );
                    STRFREE( ed->description );
                    ed->description = QUICKLINK( pnote->date );
                    ed = SetOExtra( paper, ( char * ) "note" );
                    stralloc_printf( &( ed->description ),
                                     "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n%s\r\n",
                                     pnote->sender, pnote->to_list, pnote->subject, pnote->text );
                    paper->value[0] = 2;
                    paper->value[1] = 2;
                    paper->value[2] = 2;
                    stralloc_printf( &( paper->short_descr ), "a note from %s to %s", pnote->sender,
                                     pnote->to_list );
                    stralloc_printf( &( paper->description ),
                                     "A note from %s to %s lies on the ground.", pnote->sender,
                                     pnote->to_list );
                    stralloc_printf( &( paper->name ), "note parchment paper %s", pnote->to_list );
                }
                if ( take != 2 )
                    note_remove( ch, board, pnote );
                send_to_char( "Ok.\r\n", ch );
                if ( take == 1 ) {
                    if ( board->otakemessg )
                        act( AT_ACTION, board->otakemessg, ch, NULL, NULL, TO_ROOM );
                    else
                        act( AT_ACTION, "$n takes a note.", ch, NULL, NULL, TO_ROOM );
                    obj_to_char( paper, ch );
                }
                else if ( take == 2 ) {
                    if ( board->ocopymessg )
                        act( AT_ACTION, board->ocopymessg, ch, NULL, NULL, TO_ROOM );
                    else
                        act( AT_ACTION, "$n copies a note.", ch, NULL, NULL, TO_ROOM );
                    obj_to_char( paper, ch );
                }
                else {
                    if ( board->oremovemessg )
                        act( AT_ACTION, board->oremovemessg, ch, NULL, NULL, TO_ROOM );
                    else
                        act( AT_ACTION, "$n removes a note.", ch, NULL, NULL, TO_ROOM );
                }
                return;
            }
        }
        send_to_char( "No such note.\r\n", ch );
        return;
    }
    send_to_char( "Huh?  Type 'help note' for usage.\r\n", ch );
    return;
}

void do_checkboards( CHAR_DATA *ch, char *arg_passed )
{
    NOTE_DATA              *pnote;
    BOARD_DATA             *board;
    OBJ_INDEX_DATA         *obj;
    OBJ_DATA               *obj2;
    bool                    found;
    time_t                  oldtime,
                            newtime;

    newtime = 0;
    if ( IS_NPC( ch ) )
        return;
    oldtime = ch->pcdata->boards;
    found = FALSE;
    for ( board = first_board; board; board = board->next ) {
        if ( board->type == 1 )
            continue;
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            newtime = 0;
            newtime = pnote->current;
        }
        if ( newtime > oldtime && can_read( ch, board ) ) {
            obj = get_obj_index( board->board_obj );
            if ( !obj ) {
                bug( "checkboards: NULL board object for board %d", board->board_obj );
                continue;
            }
            if ( !IS_IMMORTAL( ch ) )
                ch_printf( ch, "%s has new posts on it to read.\r\n", obj->short_descr );
            else
                ch_printf( ch, "%s, vnum %d, has new posts on it to read.\r\n", obj->short_descr,
                           obj->vnum );
            for ( obj2 = first_object; obj2; obj2 = obj2->next ) {
                if ( !obj )
                    continue;
                if ( obj2->pIndexData->vnum == board->board_obj ) {
                    ch_printf( ch, "  and can be found in room %d\r\n", obj2->in_room->vnum );
                    break;
                }
            }
            found = TRUE;
        }
    }
    if ( !found )
        send_to_char( "No new posts found.\r\n", ch );
    ch->pcdata->boards = current_time;
    return;
}

BOARD_DATA             *read_board( char *boardfile, FILE * fp )
{
    BOARD_DATA             *board;
    char                   *word;
    bool                    fMatch;
    char                    letter;

    do {
        letter = getc( fp );
        if ( feof( fp ) ) {
            FileClose( fp );
            return NULL;
        }
    }
    while ( isspace( letter ) );
    ungetc( letter, fp );
    CREATE( board, BOARD_DATA, 1 );
    /*
     * Setup pointers --Shaddai 
     */
    board->otakemessg = NULL;
    board->opostmessg = NULL;
    board->oremovemessg = NULL;
    board->olistmessg = NULL;
    board->ocopymessg = NULL;
    board->oreadmessg = NULL;
    board->postmessg = NULL;
    for ( ;; ) {
        word = ( char * ) ( feof( fp ) ? "End" : fread_word( fp ) );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'E':
                KEY( "Extra_readers", board->extra_readers, fread_string( fp ) );
                KEY( "Extra_removers", board->extra_removers, fread_string( fp ) );
                if ( !str_cmp( word, "End" ) ) {
                    board->num_posts = 0;
                    board->first_note = NULL;
                    board->last_note = NULL;
                    board->next = NULL;
                    board->prev = NULL;
                    if ( board->board_obj != 0 && get_obj_index( board->board_obj ) == NULL )
                        bug( "Fread_board: %s board has a bad obj vnum", board->note_file );
                    return board;
                }
                break;
            case 'F':
                KEY( "Filename", board->note_file, fread_string( fp ) );
                break;
            case 'M':
                KEY( "Min_read_level", board->min_read_level, fread_number( fp ) );
                KEY( "Min_post_level", board->min_post_level, fread_number( fp ) );
                KEY( "Min_remove_level", board->min_remove_level, fread_number( fp ) );
                KEY( "Max_posts", board->max_posts, fread_number( fp ) );
                break;
            case 'O':
                KEY( "OTakemessg", board->otakemessg, fread_string( fp ) );
                KEY( "OCopymessg", board->ocopymessg, fread_string( fp ) );
                KEY( "OReadmessg", board->oreadmessg, fread_string( fp ) );
                KEY( "ORemovemessg", board->oremovemessg, fread_string( fp ) );
                KEY( "OListmessg", board->olistmessg, fread_string( fp ) );
                KEY( "OPostmessg", board->opostmessg, fread_string( fp ) );
                break;
            case 'P':
                KEY( "Post_group", board->post_group, fread_string( fp ) );
                KEY( "Postmessg", board->postmessg, fread_string( fp ) );
                break;
            case 'R':
                KEY( "Read_group", board->read_group, fread_string( fp ) );
                break;
            case 'T':
                KEY( "Type", board->type, fread_number( fp ) );
                break;
            case 'V':
                KEY( "Vnum", board->board_obj, fread_number( fp ) );
                break;
        }
        if ( !fMatch )
            bug( "read_board: no match: %s", word );
    }
    return board;
}

NOTE_DATA              *read_note( char *notefile, FILE * fp )
{
    NOTE_DATA              *pnote;
    char                   *word;
    bool                    fMatch;
    char                    letter;

    do {
        letter = getc( fp );
        if ( feof( fp ) ) {
            FileClose( fp );
            return NULL;
        }
    }
    while ( isspace( letter ) );
    ungetc( letter, fp );
    CREATE( pnote, NOTE_DATA, 1 );

    for ( ;; ) {
        word = ( char * ) ( feof( fp ) ? "End" : fread_word( fp ) );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                KEY( "Abstentions", pnote->abstentions, fread_string( fp ) );
                break;
            case 'C':
                KEY( "Ctime", pnote->current, fread_number( fp ) );
                break;
            case 'D':
                KEY( "Date", pnote->date, fread_string( fp ) );
                break;
            case 'E':
                if ( !strcmp( word, "End" ) ) {
                    fMatch = TRUE;
                    pnote->next = NULL;
                    pnote->prev = NULL;
                    return pnote;
                }
                break;
            case 'N':
                KEY( "Novotes", pnote->novotes, fread_string( fp ) );
                break;
            case 'S':
                KEY( "Sender", pnote->sender, fread_string( fp ) );
                KEY( "Subject", pnote->subject, fread_string( fp ) );
                break;
            case 'T':
                KEY( "To", pnote->to_list, fread_string( fp ) );
                KEY( "Text", pnote->text, fread_string( fp ) );
                break;
            case 'V':
                KEY( "Voting", pnote->voting, fread_number( fp ) );
                break;
            case 'Y':
                KEY( "Yesvotes", pnote->yesvotes, fread_string( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_char: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

/* Load boards file. */
void load_boards( void )
{
    FILE                   *board_fp;
    FILE                   *note_fp;
    BOARD_DATA             *board;
    NOTE_DATA              *pnote;
    char                    boardfile[256];
    char                    notefile[256];

    first_board = NULL;
    last_board = NULL;
    snprintf( boardfile, 256, "%s", BOARD_FILE );
    if ( ( board_fp = FileOpen( boardfile, "r" ) ) == NULL )
        return;
    while ( ( board = read_board( boardfile, board_fp ) ) != NULL ) {
        LINK( board, first_board, last_board, next, prev );
        snprintf( notefile, 256, "%s%s", BOARD_DIR, board->note_file );
        if ( ( note_fp = FileOpen( notefile, "r" ) ) != NULL ) {
            while ( ( pnote = read_note( notefile, note_fp ) ) != NULL ) {
                LINK( pnote, board->first_note, board->last_note, next, prev );
                board->num_posts++;
            }
        }
    }
    return;
}

void do_makeboard( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;

    if ( !ch || IS_NPC( ch ) )
        return;
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Syntax: makeboard <filename>\r\n", ch );
        return;
    }
    smash_tilde( argument );
    CREATE( board, BOARD_DATA, 1 );
    LINK( board, first_board, last_board, next, prev );
    board->note_file = STRALLOC( strlower( argument ) );
    return;
}

void do_bset( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    bool                    found;
    char                    arg1[MIL],
                            arg2[MIL];
    int                     value;

    if ( !ch || IS_NPC( ch ) )
        return;
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    set_char_color( AT_NOTE, ch );
    if ( !VLD_STR( arg1 ) || !VLD_STR( arg2 ) ) {
        send_to_char( "Syntax: bset <board filename> <field> value\r\n"
                      "\r\nField being one of:\r\n"
                      "  ovnum read post remove maxpost filename type\r\n"
                      "  read_group post_group extra_readers extra_removers\r\n"
                      "The following will affect how an action is sent:\r\n"
                      "  oremove otake olist oread ocopy opost postmessg\r\n", ch );
        return;
    }
    value = atoi( argument );
    found = FALSE;
    for ( board = first_board; board; board = board->next )
        if ( !str_cmp( arg1, board->note_file ) ) {
            found = TRUE;
            break;
        }
    if ( !found ) {
        send_to_char( "Board not found.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "ovnum" ) ) {
        if ( !get_obj_index( value ) ) {
            send_to_char( "No such object.\r\n", ch );
            return;
        }
        board->board_obj = value;
        write_boards_txt(  );
        send_to_char( "Done. (board's object vnum set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "read" ) ) {
        if ( value < 0 || value > MAX_LEVEL ) {
            send_to_char( "Value outside valid character level range.\r\n", ch );
            return;
        }
        board->min_read_level = value;
        write_boards_txt(  );
        send_to_char( "Done. (minimum reading level set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "read_group" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No reading group specified.\r\n", ch );
            return;
        }
        STRFREE( board->read_group );
        if ( str_cmp( argument, "none" ) )
            board->read_group = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.  (reading group set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "post_group" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No posting group specified.\r\n", ch );
            return;
        }
        STRFREE( board->post_group );
        if ( str_cmp( argument, "none" ) )
            board->post_group = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.  (posting group set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "postmessg" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message specified.\r\n", ch );
            return;
        }
        if ( board->postmessg )
            STRFREE( board->postmessg );
        if ( str_cmp( argument, "none" ) )
            board->postmessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "opost" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message specified.\r\n", ch );
            return;
        }
        if ( board->opostmessg )
            STRFREE( board->opostmessg );
        if ( str_cmp( argument, "none" ) )
            board->opostmessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "oremove" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message specified.\r\n", ch );
            return;
        }
        if ( board->oremovemessg )
            STRFREE( board->oremovemessg );
        if ( str_cmp( argument, "none" ) )
            board->oremovemessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "otake" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message specified.\r\n", ch );
            return;
        }
        if ( board->otakemessg )
            STRFREE( board->otakemessg );
        if ( str_cmp( argument, "none" ) )
            board->otakemessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "ocopy" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message specified.\r\n", ch );
            return;
        }
        if ( board->ocopymessg )
            STRFREE( board->ocopymessg );
        if ( str_cmp( argument, "none" ) )
            board->ocopymessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "oread" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message sepcified.\r\n", ch );
            return;
        }
        if ( board->oreadmessg )
            STRFREE( board->oreadmessg );
        if ( str_cmp( argument, "none" ) )
            board->oreadmessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "olist" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No message specified.\r\n", ch );
            return;
        }
        if ( board->olistmessg )
            STRFREE( board->olistmessg );
        if ( str_cmp( argument, "none" ) )
            board->olistmessg = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "extra_removers" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No names specified.\r\n", ch );
            return;
        }
        if ( !str_cmp( argument, "none" ) )
            STRFREE( board->extra_removers );
        else {
            if ( VLD_STR( board->extra_removers ) )
                stralloc_printf( &( board->extra_removers ), "%s %s", board->extra_removers,
                                 argument );
            else
                stralloc_printf( &( board->extra_removers ), "%s", argument );
        }
        write_boards_txt(  );
        send_to_char( "Done. (extra removers set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "extra_readers" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No names specified.\r\n", ch );
            return;
        }
        if ( !str_cmp( argument, "none" ) )
            STRFREE( board->extra_readers );
        else {
            if ( VLD_STR( board->extra_readers ) )
                stralloc_printf( &( board->extra_readers ), "%s %s", board->extra_readers,
                                 argument );
            else
                stralloc_printf( &( board->extra_readers ), "%s", argument );
        }
        write_boards_txt(  );
        send_to_char( "Done.  (extra readers set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "filename" ) ) {
        if ( !VLD_STR( argument ) ) {
            send_to_char( "No filename specified.\r\n", ch );
            return;
        }
        STRFREE( board->note_file );
        board->note_file = STRALLOC( argument );
        write_boards_txt(  );
        send_to_char( "Done.  (board's filename set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "post" ) ) {
        if ( value < 0 || value > MAX_LEVEL ) {
            send_to_char( "Value outside valid character level range.\r\n", ch );
            return;
        }
        board->min_post_level = value;
        write_boards_txt(  );
        send_to_char( "Done.  (minimum posting level set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "remove" ) ) {
        if ( value < 0 || value > MAX_LEVEL ) {
            send_to_char( "Value outside valid character level range.\r\n", ch );
            return;
        }
        board->min_remove_level = value;
        write_boards_txt(  );
        send_to_char( "Done.  (minimum remove level set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "maxpost" ) ) {
        if ( value < 1 || value > 999 ) {
            send_to_char( "Value out of range.\r\n", ch );
            return;
        }
        board->max_posts = value;
        write_boards_txt(  );
        send_to_char( "Done.  (maximum number of posts set)\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "type" ) ) {
        if ( value < 0 || value > 1 ) {
            send_to_char( "Value out of range.\r\n", ch );
            return;
        }
        board->type = value;
        write_boards_txt(  );
        send_to_char( "Done.  (board's type set)\r\n", ch );
        return;
    }
    do_bset( ch, ( char * ) "" );
    return;
}

void do_bstat( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    bool                    found;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;
    argument = one_argument( argument, arg );
    found = FALSE;
    for ( board = first_board; board; board = board->next )
        if ( !str_cmp( arg, board->note_file ) ) {
            found = TRUE;
            break;
        }
    if ( !found ) {
        if ( VLD_STR( argument ) ) {
            send_to_char( "&GBoard not found. Syntax: bstat <board filename>\r\n", ch );
            return;
        }
        else {
            board = find_board( ch );
            if ( !board ) {
                send_to_char( "&GNo board present. Syntax: bstat <board filename>\r\n", ch );
                return;
            }
        }
    }
    ch_printf( ch,
               "\r\n&GFilename: &W%-15.15s &GOVnum: &W%-5d  &GRead: &W%-2d  &GPost: &W%-2d  &GRemove: &W%-2d\r\n&GMaxpost:  &W%-3d              &GType: &W%d\r\n&GPosts:    %d\r\n",
               board->note_file, board->board_obj, board->min_read_level, board->min_post_level,
               board->min_remove_level, board->max_posts, board->type, board->num_posts );
    ch_printf( ch,
               "&GRead_group:     &W%s\r\n&GPost_group:     &W%s\r\n&GExtra_readers:  &W%s\r\n&GExtra_removers: &W%s\r\n",
               board->read_group, board->post_group, board->extra_readers, board->extra_removers );
    ch_printf( ch, "&GPost Message:    %s\r\n",
               board->postmessg ? board->postmessg : "Default Message" );
    ch_printf( ch, "&GOPost Message:   %s\r\n",
               board->opostmessg ? board->opostmessg : "Default Message" );
    ch_printf( ch, "&GORead Message:   %s\r\n",
               board->oreadmessg ? board->oreadmessg : "Default Message" );
    ch_printf( ch, "&GORemove Message: %s\r\n",
               board->oremovemessg ? board->oremovemessg : "Default Message" );
    ch_printf( ch, "&GOTake Message:   %s\r\n",
               board->otakemessg ? board->otakemessg : "Default Message" );
    ch_printf( ch, "&GOList Message:   %s\r\n",
               board->olistmessg ? board->olistmessg : "Default Message" );
    ch_printf( ch, "&GOCopy Message:   %s\r\n",
               board->ocopymessg ? board->ocopymessg : "Default Message" );
    return;
}

void do_boards( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;

    if ( IS_NPC( ch ) )
        return;
    if ( !first_board ) {
        send_to_char( "&GThere are no boards yet.\r\n", ch );
        return;
    }
    for ( board = first_board; board; board = board->next )
        pager_printf( ch,
                      "&G%-15.15s #: %7d Read: %3d Post: %3d Rmv: %3d Max: %3d Posts: &g%3d &GType: %d\r\n",
                      board->note_file, board->board_obj, board->min_read_level,
                      board->min_post_level, board->min_remove_level, board->max_posts,
                      board->num_posts, board->type );
}

void mail_count( CHAR_DATA *ch )
{
    BOARD_DATA             *board;
    NOTE_DATA              *note;
    int                     cnt_to = 0,
        cnt_from = 0;

    for ( board = first_board; board; board = board->next )
        if ( board->type == BOARD_MAIL && can_read( ch, board ) )
            for ( note = board->first_note; note; note = note->next ) {
                if ( is_name( ch->name, note->to_list ) )
                    ++cnt_to;
                else if ( !str_cmp( ch->name, note->sender ) )
                    ++cnt_from;
            }
    if ( cnt_to )
        ch_printf( ch, "You have %d mail message%swaiting for you.\r\n", cnt_to,
                   ( cnt_to > 1 ) ? "s " : " " );

    if ( cnt_from )
        ch_printf( ch, "You have %d mail message%swritten by you.\r\n", cnt_from,
                   ( cnt_from > 1 ) ? "s " : " " );
    return;
}

#undef VOTE_NONE
#undef VOTE_OPEN
#undef VOTE_CLOSED
#undef OBJ_VNUM_NOTE
