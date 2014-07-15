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

/* New editor code
 * Author: Cronel (cronel_kal@hotmail.com)
 * of FrozenMUD (empire.digiunix.net 4000)
 * Permission to use and distribute this code is granted provided
 * this header is retained and unaltered, and the distribution
 * package contains all the original files unmodified.
 * If you modify this code and use/distribute modified versions
 * you must give credit to the original author(s). */

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "h/mud.h"

/* Data types and other definitions */
typedef struct editor_line EDITOR_LINE;

#define CHAR_BLOCK (80)
#define BLOCK_ROUNDUP(size) (((size)+CHAR_BLOCK-1) / CHAR_BLOCK * CHAR_BLOCK)
#define NOLIMIT (-1)
#define RESIZE_IF_NEEDED(buf, buf_size, buf_used, added_use) \
  if((buf_used) + (added_use) >= (buf_size)) \
  { \
    short added_size; \
    added_size = BLOCK_ROUNDUP(added_use); \
    if(added_size == 0) \
      added_size = CHAR_BLOCK; \
    RECREATE((buf), char, buf_size + added_size); \
    (buf_size) += added_size; \
  }
struct editor_line
{
    char                   *line;                      /* line text */
    short                   line_size;                 /* size allocated in "line" */
    short                   line_used;                 /* bytes used of "line" */
    EDITOR_LINE            *next;
};

struct editor_data
{
    EDITOR_LINE            *first_line;                /* list of lines */
    short                   line_count;                /* number of lines allocated */
    EDITOR_LINE            *on_line;                   /* pointer to the line being
                                                        * edited */
    int                     text_size;                 /* total size of text (not
                                                        * counting newlines). */
    int                     max_size;                  /* max size in chars of string
                                                        * being edited (counting
                                                        * newlines) */
    char                   *desc;                      /* buffer description */
};

/* "max_size" is the maximum size of the final text converted to string 
   "text_size" is equal to the strlen of all lines added up; the actual
   total length when converted to string is equal to this number plus
   line_count * 2, because of the trailing "\r\n" that has to be added
   to each line (of course, plus 1 because of the final \0).
   Thus, if(total_size + line_count * 2 +1) > max_size, the buffer cant
   hold more data. 
   Hence, this define: */
#define TOTAL_BUFFER_SIZE(edd) (edd->text_size + edd->line_count * 2 + 1)

/* Function declarations */
/* funcs to manipulate editor datas */
EDITOR_LINE            *make_new_line( char *str );
void                    discard_editdata( EDITOR_DATA * edd );
EDITOR_DATA            *clone_editdata( EDITOR_DATA * edd );
EDITOR_DATA            *str_to_editdata( char *str, short max_size );
char                   *editdata_to_str( EDITOR_DATA * edd );
void                    discard_line_list( EDITOR_LINE * eline_list );
EDITOR_LINE            *detach_line_range( EDITOR_DATA * edd, short from, short to );
void                    attach_line_range( EDITOR_DATA * edd, short position,
                                           EDITOR_LINE * line_list );

/* simple functions to set a description for what's currently being edited */

/* the main editor functions visible to the rest of the code */
/* main editing function */

/* misc functions */
char                   *finer_one_argument( char *argument, char *arg_first );
char                   *text_replace( char *src, char *word_src, char *word_dst, short *pnew_size,
                                      short *prepl_count );
EDITOR_LINE            *format_text( EDITOR_LINE * line_list, short max_width );

/* editor sub functions */
void                    editor_print_info( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_help( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_clear_buf( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_search_and_replace( CHAR_DATA *ch, EDITOR_DATA * edd,
                                                   char *argument );
void                    editor_insert_line( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_delete_line( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_goto_line( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_list( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_abort( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_escaped_cmd( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_save( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );
void                    editor_format( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument );

/* Edit_data manipulation functions */
EDITOR_LINE            *make_new_line( char *str )
{
    EDITOR_LINE            *new_line;
    short                   size;

    size = strlen( str );
    size = BLOCK_ROUNDUP( size );
    if ( size == 0 )
        size = CHAR_BLOCK;
    CREATE( new_line, EDITOR_LINE, 1 );
    CREATE( new_line->line, char, size );

    new_line->line_size = size;
    new_line->line_used = strlen( str );
    strcpy( new_line->line, str );
    return new_line;
}

void discard_editdata( EDITOR_DATA * edd )
{
    discard_line_list( edd->first_line );
    if ( edd->desc )
        STRFREE( edd->desc );
    DISPOSE( edd );
    return;
}

void discard_line_list( EDITOR_LINE * eline_list )
{
    EDITOR_LINE            *eline,
                           *elnext;

    eline = eline_list;
    while ( eline ) {
        elnext = eline->next;
        DISPOSE( eline->line );
        DISPOSE( eline );
        eline = elnext;
    }
    return;
}

/* Returns a copy of the editor */
EDITOR_DATA            *clone_editdata( EDITOR_DATA * edd )
{
    EDITOR_DATA            *new_edd;
    EDITOR_LINE            *new_line,
                           *eline,
                            root_line;

    CREATE( new_edd, EDITOR_DATA, 1 );
    new_line = &root_line;
    for ( eline = edd->first_line; eline; eline = eline->next ) {
        new_line->next = make_new_line( eline->line );
        if ( edd->on_line == eline )
            new_edd->on_line = new_line->next;
        new_line = new_line->next;
    }
    new_edd->max_size = edd->max_size;
    new_edd->text_size = edd->text_size;
    new_edd->line_count = edd->line_count;
    new_edd->first_line = root_line.next;
    if ( VLD_STR( edd->desc ) )
        new_edd->desc = STRALLOC( edd->desc );
    else
        new_edd->desc = NULL;
    return new_edd;
}

/* Converts a string to editdata. It will stop converting if it reaches "max_size" and will return the partial conversion at that point. */
EDITOR_DATA            *str_to_editdata( char *str, short max_size )
{
    char                   *p;
    EDITOR_DATA            *edd;
    EDITOR_LINE            *eline;
    short                   i;
    short                   tsize,
                            line_count;

    CREATE( edd, EDITOR_DATA, 1 );
    eline = make_new_line( ( char * ) "" );
    edd->first_line = eline;
    i = 0;
    tsize = 0;
    line_count = 1;
    p = str;
    if ( !p )
        p = ( char * ) "";
    while ( *p ) {
        if ( max_size != NOLIMIT && tsize + line_count * 2 + 1 >= max_size )
            break;
        if ( *p == '\r' );
        else if ( *p == '\n' ) {
            eline->line[i] = '\0';
            eline->next = make_new_line( ( char * ) "" );
            eline = eline->next;
            line_count++;
            i = 0;
        }
        else {
            eline->line[i] = *p;
            eline->line_used++;
            tsize++;
            i++;
            RESIZE_IF_NEEDED( eline->line, eline->line_size, eline->line_used, 1 );
        }
        p++;
    }
    if ( eline->line[0] != '\0' ) {
        eline->line[i] = '\0';
        eline->next = make_new_line( ( char * ) "" );
        line_count++;
        eline = eline->next;
    }
    edd->line_count = line_count;
    edd->on_line = eline;
    edd->max_size = max_size;
    edd->text_size = tsize;
    return edd;
}

/* Removes the tildes from a line, except if it's the last character. */
void smush_tilde( char *str )
{
    int                     len;
    char                    last;
    char                   *strptr;

    strptr = str;
    len = strlen( str );
    if ( len )
        last = strptr[len - 1];
    else
        last = '\0';
    for ( ; *str != '\0'; str++ ) {
        if ( *str == '~' )
            *str = '-';
    }
    if ( len )
        strptr[len - 1] = last;
    return;
}

char                   *editdata_to_str( EDITOR_DATA * edd )
{
    EDITOR_LINE            *eline;
    char                   *buf,
                           *src,
                           *tmp;
    short                   size,
                            used,
                            i;

    CREATE( buf, char, MSL );

    size = MSL;
    used = 0;
    buf[0] = '\0';
    eline = edd->first_line;
    i = 0;
    while ( eline ) {
        /*
         * ignore the last empty line 
         */
        if ( eline->next == NULL && eline->line[0] == '\0' )
            break;
        src = eline->line;
        while ( *src ) {
            buf[i++] = *src++;
            used++;
            if ( used >= size - 3 ) {
                RECREATE( buf, char, size + MSL );

                size += MSL;
            }
        }
        buf[i++] = '\n';
        buf[i++] = '\r';
        used += 2;
        eline = eline->next;
    }
    buf[i++] = '\0';
    used++;
    if ( VLD_STR( buf ) ) {
        tmp = STRALLOC( buf );
        smush_tilde( tmp );
    }
    else
        tmp = NULL;
    DISPOSE( buf );
    return tmp;
}

/* Returns the line number currently being edited 
 * or -1 if there are no lines in the editor or the
 * insertion point is not positioned anywhere */
short get_online_index( EDITOR_DATA * edd )
{
    short                   i;
    EDITOR_LINE            *eline;

    eline = edd->first_line;
    i = 0;
    while ( eline ) {
        i++;
        if ( eline == edd->on_line )
            break;
        eline = eline->next;
    }
    if ( eline == NULL )
        return -1;
    else
        return i;
}

/* Sets the insertion point of the editor to the given
 * index. If the index is out of range it will set it
 * to the first or the last line. */
void set_online_index( EDITOR_DATA * edd, short index )
{
    short                   i;
    EDITOR_LINE            *eline,
                           *prev;

    if ( edd->first_line == NULL )
        return;
    eline = edd->first_line;
    i = 1;
    prev = NULL;
    while ( i < index && eline ) {
        i++;
        prev = eline;
        eline = eline->next;
    }
    edd->on_line = eline ? eline : prev;
    return;
}

/* Detaches a range of lines from the editor. If the line range is invalid, it 
 * does nothing and returns NULL. If it's valid, it detaches it and returns it 
 * as a list of lines. If the editor insertion point is somewhere within the 
 * range it will leave it pointing immediately after or before the line range.
 * If the range comprises all the lines in the editor, it will leave the 
 * editor empty. */
EDITOR_LINE            *detach_line_range( EDITOR_DATA * edd, short linefrom, short lineto )
{
    EDITOR_LINE            *eline_from_prev,
                           *eline_to,
                           *eline,
                            root;
    short                   i;
    bool                    move_on_line;

    if ( linefrom < 1 || lineto > edd->line_count ) {
        bug( "detach_line_range: %d-%d Out of range (%d-%d).", linefrom, lineto, 1,
             edd->line_count );
        return NULL;
    }
    /*
     * Find the start and end lines 
     */
    root.next = edd->first_line;
    eline = &root;
    i = 1;
    while ( i < linefrom ) {
        i++;
        eline = eline->next;
    }
    eline_from_prev = eline;
    /*
     * We actualy need the line previous to the first one, se we can delink it. Single linked lists... 
     */
    while ( i <= lineto ) {
        i++;
        eline = eline->next;
    }
    eline_to = eline;
    /*
     * Substract the sizes of all lines from the editor 
     */
    /*
     * and check to see if the insertion point is within the range 
     */
    move_on_line = FALSE;
    eline = eline_from_prev->next;
    while ( eline != eline_to->next ) {
        if ( eline == edd->on_line )
            move_on_line = TRUE;
        edd->text_size -= eline->line_used;
        edd->line_count--;
        eline = eline->next;
    }
    /*
     * Leave the insertion point somewhere valid 
     */
    if ( move_on_line ) {
        if ( eline_to->next )
            /*
             * there's something below the range? leave it there 
             */
            edd->on_line = eline_to->next;
        else if ( eline_from_prev != &root )
            /*
             * nothing bellow the range, but something above it? there.. 
             */
            edd->on_line = eline_from_prev;
        else
            /*
             * the entire buffer is being detached. leave it on NULL 
             */
            edd->on_line = NULL;
    }
    /*
     * Delink the lines from the editor 
     */
    eline = eline_from_prev->next;
    eline_from_prev->next = eline_to->next;
    eline_to->next = NULL;
    edd->first_line = root.next;
    return eline;
}

/* Inserts a given list of lines at the specified position within the
 * editor data. If the position is invalid, it will try to insert at the
 * beginning or end. The insertion point is not touched.  */
void attach_line_range( EDITOR_DATA * edd, short position, EDITOR_LINE * line_list )
{
    EDITOR_LINE             root,
                           *eline,
                           *eline_last;
    short                   i;

    if ( position > edd->line_count && edd->line_count > 0 )
        position = edd->line_count;
    else if ( position < 1 || position > edd->line_count )
        position = 1;
    /*
     * Add to text size of edd 
     */
    eline = line_list;
    eline_last = NULL;
    while ( eline ) {
        edd->text_size += eline->line_used;
        edd->line_count++;
        /*
         * remember the last one on the list 
         */
        if ( eline->next == NULL )
            eline_last = eline;
        eline = eline->next;
    }
    /*
     * Find the position to insert 
     */
    root.next = edd->first_line;
    eline = &root;
    i = 1;
    while ( i < position ) {
        i++;
        eline = eline->next;
    }
    /*
     * link it at the end 
     */
    eline_last->next = eline->next;
    eline->next = line_list;
    edd->first_line = root.next;
}

/* Main editor functions */
void set_editor_desc( CHAR_DATA *ch, const char *new_desc )
{
    if ( !ch || !ch->editor )
        return;
    if ( VLD_STR( ch->editor->desc ) )
        STRFREE( ch->editor->desc );
    if ( VLD_STR( new_desc ) )
        ch->editor->desc = STRALLOC( new_desc );
    else
        ch->editor->desc = NULL;
    return;
}

/* Just to save code... */
void editor_desc_printf( CHAR_DATA *ch, const char *desc_fmt, ... )
{
    char                    buf[MSL * 2];              /* umpf.. */
    va_list                 args;

    va_start( args, desc_fmt );
    vsnprintf( buf, MSL * 2, desc_fmt, args );
    va_end( args );
    set_editor_desc( ch, buf );
    return;
}

void start_editing_nolimit( CHAR_DATA *ch, char *old_text, short max_total )
{
    if ( !ch->desc ) {
        bug( "%s", "Fatal: start_editing: no desc" );
        return;
    }
    if ( ch->substate == SUB_RESTRICTED )
        bug( "%s", "NOT GOOD: start_editing: ch->substate == SUB_RESTRICTED" );
    set_char_color( AT_CYAN, ch );
    send_to_char
        ( "&cBegin entering your text now\r\nWhile writing you can use the below commands.\r\n/? = help /s = save and exit /c = clear /l = list /i = insert line /g = goto line number\r\n",
          ch );
    send_to_char( "-----------------------------------------------------------------------\r\n",
                  ch );
    if ( ch->editor )
        stop_editing( ch );
    ch->editor = str_to_editdata( old_text, max_total );
    ch->editor->desc = STRALLOC( "Unknown buffer" );
    ch->desc->connected = CON_EDITING;
    send_to_char( "> ", ch );
    return;
}

char                   *copy_buffer( CHAR_DATA *ch )
{
    char                   *buf;

    if ( !ch ) {
        bug( "%s", "copy_buffer: null ch" );
        return NULL;
    }
    if ( !ch->editor ) {
        bug( "%s", "copy_buffer: null editor" );
        return NULL;
    }
    buf = editdata_to_str( ch->editor );
    return buf;
}

char                   *editdata_to_str_nohash( EDITOR_DATA * edd )
{
    EDITOR_LINE            *eline;
    char                   *buf,
                           *src,
                           *tmp;
    short                   size,
                            used,
                            i;

    CREATE( buf, char, MSL );

    size = MSL;
    used = 0;
    buf[0] = '\0';
    eline = edd->first_line;
    i = 0;
    while ( eline ) {
        /*
         * ignore the last empty line 
         */
        if ( eline->next == NULL && eline->line[0] == '\0' )
            break;
        src = eline->line;
        while ( *src ) {
            buf[i++] = *src++;
            used++;
            if ( used >= size - 3 ) {
                RECREATE( buf, char, size + MSL );

                size += MSL;
            }
        }
        buf[i++] = '\n';
        buf[i++] = '\r';
        used += 2;
        eline = eline->next;
    }
    buf[i++] = '\0';
    used++;
    if ( VLD_STR( buf ) ) {
        tmp = STRALLOC( buf );
        smush_tilde( tmp );
    }
    else
        tmp = NULL;
    STRFREE( buf );
    return tmp;
}

char                   *copy_buffer_nohash( CHAR_DATA *ch )
{
    char                   *buf;

    if ( !ch ) {
        bug( "%s", "copy_buffer: null ch" );
        return NULL;
    }
    if ( !ch->editor ) {
        bug( "%s", "copy_buffer: null editor" );
        return NULL;
    }
    buf = editdata_to_str_nohash( ch->editor );
    return buf;
}

void stop_editing( CHAR_DATA *ch )
{
    set_char_color( AT_PLAIN, ch );
    discard_editdata( ch->editor );
    ch->editor = NULL;
    send_to_char( "Done.\r\n", ch );
    ch->dest_buf = NULL;
    ch->spare_ptr = NULL;
    ch->substate = SUB_NONE;
    if ( !ch ) {
        bug( "%s", "Fatal: stop_editing: no character" );
        return;
    }
    if ( !ch->desc ) {
        bug( "%s", "Fatal: stop_editing: no desc" );
        return;
    }
    ch->desc->connected = CON_PLAYING;
    return;
}

void edit_buffer( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA        *d;
    EDITOR_DATA            *edd;
    EDITOR_LINE            *newline;
    char                    cmd[MIL];
    short                   linelen;
    bool                    cont_line;
    char                   *p;

    d = ch->desc;
    if ( d == NULL ) {
        send_to_char( "You have no descriptor.\r\n", ch );
        return;
    }
    if ( d->connected != CON_EDITING ) {
        send_to_char( "You can't do that!\r\n", ch );
        bug( "%s", "Edit_buffer: d->connected != CON_EDITING" );
        return;
    }
    if ( ch->substate <= SUB_PAUSE ) {
        send_to_char( "You can't do that!\r\n", ch );
        bug( "Edit_buffer: illegal ch->substate (%d)", ch->substate );
        d->connected = CON_PLAYING;
        return;
    }
    if ( !ch->editor ) {
        send_to_char( "You can't do that!\r\n", ch );
        bug( "%s", "Edit_buffer: null editor" );
        d->connected = CON_PLAYING;
        return;
    }
    edd = ch->editor;
    if ( ( argument[0] == '/' && argument[1] != '/' ) || argument[0] == '\\' ) {
        argument = one_argument( argument, cmd );
        if ( !str_cmp( cmd + 1, "?" ) )
            editor_help( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "c" ) )
            editor_clear_buf( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "r" ) )
            editor_search_and_replace( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "i" ) )
            editor_insert_line( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "d" ) )
            editor_delete_line( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "g" ) )
            editor_goto_line( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "l" ) )
            editor_list( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "a" ) )
            editor_abort( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "s" ) )
            editor_save( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "!" ) )
            editor_escaped_cmd( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "p" ) )
            editor_print_info( ch, edd, argument );
        else if ( !str_cmp( cmd + 1, "f" ) )
            editor_format( ch, edd, argument );
        else
            send_to_char( "Uh? Type '/?' to see the list of valid editor commands.\r\n", ch );
        if ( cmd[1] != 'a' && cmd[1] != 's' )
            send_to_char( "> ", ch );
        return;
    }
    /*
     * Kludgy fix. Read_from_buffer in comm.c adds a space on
     * empty lines. Don't let this fill up usable buffer space.. 
     */
    if ( argument[0] == ' ' && argument[1] == '\0' )
        argument[0] = '\0';
    linelen = strlen( argument );
    /*
     * "Line continuation" feature to go around the "Line too long" 
     * truncation forced by the input functions in comm.c (could be 
     * done with /g too). 
     */
    p = argument + linelen - 1;
    while ( p > argument && isspace( *p ) )
        p--;
    if ( p > argument && *p == '\\' ) {
        cont_line = TRUE;
        *p = '\0';
    }
    else
        cont_line = FALSE;
    if ( edd->max_size != NOLIMIT && TOTAL_BUFFER_SIZE( edd ) + linelen + 2 >= edd->max_size ) {
        send_to_char( "Buffer full.\r\n", ch );
        editor_save( ch, edd, ( char * ) "" );
    }
    else {
        /*
         * add it to the current line 
         */
        RESIZE_IF_NEEDED( edd->on_line->line, edd->on_line->line_size, edd->on_line->line_used,
                          linelen + 1 );
        strcat( edd->on_line->line, argument );
        edd->on_line->line_used += linelen;
        edd->text_size += linelen;
        /*
         * create a line and advance to it 
         */
        if ( !cont_line ) {
            newline = make_new_line( ( char * ) "" );
            newline->next = edd->on_line->next;
            edd->on_line->next = newline;
            edd->on_line = newline;
            edd->line_count++;
        }
        else
            send_to_char( "(Continued)\r\n", ch );
        send_to_char( "> ", ch );
    }
}

void editor_print_info( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    ch_printf( ch, "Currently editing: %s\r\n", edd->desc ? edd->desc : "(Null description)" );
    ch_printf( ch, "Total lines: %4d   On line:  %4d\r\n", edd->line_count,
               get_online_index( edd ) );
    ch_printf( ch, "Buffer size: %4d   Max size: ", TOTAL_BUFFER_SIZE( edd ) );
    if ( edd->max_size == NOLIMIT )
        send_to_char( "Infinite\r\n", ch );
    else
        ch_printf( ch, "%d\r\n", edd->max_size );
    return;
}

void editor_help( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    short                   i;
    const char             *arg[] =
        { "", "l", "c", "d", "g", "i", "r", "a", "p", "!", "s", "f", NULL };
    const char             *editor_help[] = {
        /*
         * general help 
         */
        "Editing commands\r\n"
            "---------------------------------\r\n"
            "/l [range]      list buffer\r\n"
            "/c              clear buffer\r\n"
            "/d <line>       delete line\r\n"
            "/g <line>       goto line\r\n"
            "/i <line>       insert line\r\n"
            "/r <old> <new>  global replace\r\n"
            "/a              abort editing\r\n"
            "/p              print information\r\n"
            "/! <command>    execute command (do not use another editing command)\r\n"
            "/s              save buffer\r\n" "/f [range]      format paragraph\r\n"
            "Type /? <command>  to get more information on each command.\r\n\r\n",
        "/l [range]: Lists the buffer. Shows what you've written. Optionaly\r\n"
            "   takes a range of lines as argument.\r\n",
        "/c: Clears the buffer, leaving only one empty line.\r\n",
        "/d <line>: Deletes a line. If you delete the line currently being\r\n"
            "   edited, the insertion point is moved down if possible, if not, up.\r\n",
        "/g <line>: Moves the insertion point to a given line.\r\n",
        "/i <line>: Inserts an empty line before the given line.\r\n",
        "/r <old text> <new text>: Global search and replace text. The arguments\r\n"
            "  are case-sensitive. To replace a multi-word text, surround it with\r\n"
            "  single quotes. When inside quotes, you must escape the single quote\r\n"
            "  character, double quote character, and the bar: (') becomes (\\'),\r\n"
            "  (\") becomes (\\\") and (\\) becomes (\\\\)\r\n",
        "/a: Aborts edition, terminating the edition session and throwing\r\n"
            "   away what you've edited.\r\n",
        "/p: Prints information about the current editing session.\r\n",
        "/!: Escaped command. Executes the given command as if you were\r\n"
            "   outside the editor. This is only allowed to imms, since it can\r\n"
            "   potentialy crash the mud.\r\n",
        "/s: Saves the current buffer, terminating the edition session.\r\n",
        "/f [range] [width]; Format paragraph. Takes the text in the \r\n"
            "   given lines and formats it to form a uniform looking \"paragraph\",\r\n"
            "   by concatenating the lines and breaking them, doing word wrapping.\r\n"
            "   It gets rid of excess spaces, respecting only the spaces at the\r\n"
            "   beginning of the first line. First optional argument is the line\r\n"
            "   range to wich the formatting is to be applied, wich can be two\r\n"
            "   line numbers or the word \"all\". If no range is given, the\r\n"
            "   formatting is applied to the whole buffer. Second optional argument\r\n"
            "   is the maximum width of lines; this value defaults to 75 characters.\r\n",
    };
    for ( i = 0; arg[i] != NULL; i++ ) {
        if ( !str_cmp( argument, arg[i] ) )
            break;
    }
    if ( arg[i] == NULL )
        send_to_char( "No editor help about that.\r\n", ch );
    else
        send_to_char( editor_help[i], ch );
    return;
}

void editor_clear_buf( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    char                   *desc;
    short                   max_size;

    max_size = edd->max_size;
    if ( VLD_STR( edd->desc ) )
        desc = STRALLOC( edd->desc );
    else
        desc = NULL;
    discard_editdata( edd );
    ch->editor = str_to_editdata( ( char * ) "", max_size );
    ch->editor->desc = desc;
    send_to_char( "Buffer cleared.\r\n", ch );
    return;
}

void editor_search_and_replace( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    char                    word_src[MIL];
    char                    word_dst[MIL];
    EDITOR_DATA            *cloned_edd;
    EDITOR_LINE            *eline;
    char                   *new_text;
    short                   new_size,
                            repl_count,
                            line_repl;

    argument = finer_one_argument( argument, word_src );
    argument = finer_one_argument( argument, word_dst );
    if ( word_src[0] == '\0' || word_dst[0] == '\0' ) {
        send_to_char( "Need word to replace, and replacement.\r\n", ch );
        return;
    }
    if ( strcmp( word_src, word_dst ) == 0 ) {
        send_to_char( "Done.\r\n", ch );
        return;
    }
    /*
     * Warning: the replacement of the word can result in the buffer growing
     * * larger than its maximum allowed size. To control this, the buffer is
     * * cloned, the replacement is applied to the clone, and if the size results
     * * legal after the operation, the original buffer is discarded and the 
     * * clone is assigned as the current editing buffer. If the clone's size
     * * results too large after the replacement, the clone is simply discarded
     * * and a warning is given to the user 
     * * It's a lot of overhead but it'll have to do... 
     */
    cloned_edd = clone_editdata( edd );
    eline = cloned_edd->first_line;
    repl_count = 0;
    while ( eline ) {
        new_text = text_replace( eline->line, word_src, word_dst, &new_size, &line_repl );
        DISPOSE( eline->line );
        eline->line = new_text;
        cloned_edd->text_size -= eline->line_used;
        eline->line_used = strlen( eline->line );
        cloned_edd->text_size += eline->line_used;
        eline->line_size = new_size;
        repl_count += line_repl;
        eline = eline->next;
    }
    if ( cloned_edd->max_size != NOLIMIT
         && TOTAL_BUFFER_SIZE( cloned_edd ) >= cloned_edd->max_size ) {
        send_to_char( "As a result of this operation, the buffer would grow\r\n"
                      "larger than its maximum allowed size. Operation has been\r\ncancelled.\r\n",
                      ch );
        discard_editdata( cloned_edd );
    }
    else {
        ch_printf( ch, "Replacing all occurrences of '%s' with '%s'...\r\n", word_src, word_dst );
        discard_editdata( edd );
        ch->editor = cloned_edd;
        ch_printf( ch, "Found and replaced %d occurrence(s).\r\n", repl_count );
    }
    return;
}

void editor_insert_line( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    short                   lineindex;
    EDITOR_LINE            *newline;

    if ( argument[0] == '\0' || !is_number( argument ) ) {
        send_to_char( "Must supply the line number.\r\n", ch );
        return;
    }
    lineindex = atoi( argument );
    if ( lineindex < 1 || lineindex > edd->line_count ) {
        ch_printf( ch, "Line number is out of range (1-%d).\r\n", edd->line_count );
        return;
    }
    newline = make_new_line( ( char * ) "" );
    attach_line_range( edd, lineindex, newline );
    ch_printf( ch, "Inserted line at %d.\r\n", lineindex );
    return;
}

void editor_delete_line( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    short                   lineindex;
    EDITOR_LINE            *del_line;

    if ( argument[0] == '\0' || !is_number( argument ) ) {
        send_to_char( "Must supply the line number.\r\n", ch );
        return;
    }
    lineindex = atoi( argument );
    if ( lineindex < 1 || lineindex > edd->line_count ) {
        ch_printf( ch, "Line number is out of range (1-%d).\r\n", edd->line_count );
        return;
    }
    if ( edd->line_count == 1 ) {
        if ( edd->first_line->line[0] != '\0' ) {
            edd->first_line->line[0] = '\0';
            edd->first_line->line_used = 0;
            edd->text_size = 0;
            send_to_char( "Deleted line 1.\r\n", ch );
        }
        else
            send_to_char( "The buffer is empty.\r\n", ch );
        return;
    }
    del_line = detach_line_range( edd, lineindex, lineindex );  /* TEST ME */
    discard_line_list( del_line );
    ch_printf( ch, "Deleted line %d.\r\n", lineindex );
    return;
}

void editor_goto_line( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    short                   lineindex;

    if ( !VLD_STR( argument ) || !is_number( argument ) ) {
        send_to_char( "Must supply the line number.\r\n", ch );
        return;
    }
    lineindex = atoi( argument );
    if ( lineindex < 1 || lineindex > edd->line_count ) {
        ch_printf( ch, "Line number is out of range (1-%d).\r\n", edd->line_count );
        return;
    }
    set_online_index( edd, lineindex );
    ch_printf( ch, "On line %d.\r\n", lineindex );
    return;
}

void editor_list( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    EDITOR_LINE            *eline;
    short                   line_num;
    short                   from,
                            to;
    char                    arg1[MIL];

    argument = one_argument( argument, arg1 );
    if ( arg1[0] != '\0' && is_number( arg1 ) )
        from = atoi( arg1 );
    else
        from = 1;
    argument = one_argument( argument, arg1 );
    if ( arg1[0] != '\0' && is_number( arg1 ) )
        to = atoi( arg1 );
    else
        to = edd->line_count;
    send_to_pager( "------------------\r\n", ch );
    line_num = 1;
    eline = edd->first_line;
    while ( eline ) {
        if ( line_num >= from && line_num <= to )
            pager_printf( ch, "%2d>%c%s\r\n", line_num, eline == edd->on_line ? '*' : ' ',
                          eline->line );
        eline = eline->next;
        line_num++;
    }
    send_to_pager( "------------------\r\n", ch );
    return;
}

void editor_abort( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    /*
     * We want it to free the data and these are handled in the command to free them if
     * null so clear it and finish it 
     */
    if ( ch->substate == SUB_BUG_DESC || ch->substate == SUB_IDEA_DESC
         || ch->substate == SUB_TYPO_DESC ) {
        editor_clear_buf( ch, edd, argument );
        ch->desc->connected = CON_PLAYING;
        if ( ch->last_cmd )
            ( *ch->last_cmd ) ( ch, ( char * ) "" );
        return;
    }

    send_to_char( "\r\nAborting... ", ch );
    stop_editing( ch );

    if ( ch->tempnum )
        ch->substate = ch->tempnum;
    if ( ch->pcdata->in_progress )
        ch->pcdata->in_progress = NULL;
}

void editor_escaped_cmd( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    if ( get_trust( ch ) > LEVEL_IMMORTAL ) {
        DO_FUN                 *last_cmd;
        int                     substate = ch->substate;

        last_cmd = ch->last_cmd;
        ch->substate = SUB_RESTRICTED;
        interpret( ch, argument );
        ch->substate = substate;
        ch->last_cmd = last_cmd;
        set_char_color( AT_GREEN, ch );
        send_to_char( "\r\n", ch );
    }
    else
        send_to_char( "You can't use '/!'.\r\n", ch );
    return;
}

void editor_save( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    DESCRIPTOR_DATA        *d;

    d = ch->desc;
    d->connected = CON_PLAYING;

    if ( !ch->last_cmd )
        return;
    ( *ch->last_cmd ) ( ch, ( char * ) "" );
    return;
}

void editor_format( CHAR_DATA *ch, EDITOR_DATA * edd, char *argument )
{
    EDITOR_LINE            *eline_list,
                           *formatted_lines,
                           *eline;
    short                   lineto,
                            linefrom,
                            width,
                            line_count;
    char                    arg1[MIL];
    bool                    add_final_line,
                            complete_format;

    width = 75;
    linefrom = 1;
    lineto = edd->line_count;
    /*
     * parse out the arguments.. boring 
     */
    if ( argument[0] != '\0' ) {
        argument = one_argument( argument, arg1 );
        if ( str_cmp( arg1, "all" ) ) {
            if ( arg1[0] == '\0' ) {
                send_to_char( "You must give both starting and ending line.\r\n", ch );
                return;
            }
            if ( !is_number( arg1 ) ) {
                send_to_char( "Argument must be numeric.\r\n", ch );
                return;
            }
            if ( atoi( arg1 ) < 1 || atoi( arg1 ) > edd->line_count ) {
                ch_printf( ch, "Valid line number for first argument is from 1 to %d\r\n",
                           edd->line_count );
                return;
            }
            linefrom = atoi( arg1 );
            argument = one_argument( argument, arg1 );
            if ( arg1[0] == '\0' ) {
                send_to_char( "You must give both starting and ending line.\r\n", ch );
                return;
            }
            if ( !is_number( arg1 ) ) {
                send_to_char( "Argument must be numeric.\r\n", ch );
                return;
            }
            if ( atoi( arg1 ) < 1 || atoi( arg1 ) > edd->line_count || atoi( arg1 ) < linefrom ) {
                ch_printf( ch, "Valid line number for second argument is from %d to %d\r\n",
                           linefrom, edd->line_count );
                return;
            }
            lineto = atoi( arg1 );
        }
        if ( argument[0] == '\0' )
            width = 75;
        else if ( !is_number( argument ) ) {
            send_to_char( "Argument must be numeric.\r\n", ch );
            return;
        }
        else {
            width = atoi( argument );
            if ( width < 15 || width > 240 ) {
                send_to_char( "Valid widths are from 15 to 240 characters.\r\n", ch );
                return;
            }
        }
    }
    complete_format = FALSE;
    add_final_line = FALSE;
    if ( lineto == edd->line_count ) {
        add_final_line = TRUE;
        if ( linefrom == 1 )
            complete_format = TRUE;
    }
    eline_list = detach_line_range( edd, linefrom, lineto );
    if ( !eline_list ) {
        bug( "%s", "editor_format: Null eline_list" );
        return;
    }
    ch_printf( ch, "Formatting lines %d-%d (%d lines) at %d columns...\r\n", linefrom, lineto,
               lineto - linefrom + 1, width );
    formatted_lines = format_text( eline_list, width );
    /*
     * count the number of formatted lines 
     */
    for ( line_count = 0, eline = formatted_lines; eline; line_count++, eline = eline->next );
    attach_line_range( edd, linefrom, formatted_lines );
    if ( add_final_line ) {
        eline = edd->first_line;
        while ( eline->next )
            eline = eline->next;
        eline->next = make_new_line( ( char * ) "" );
        edd->line_count++;
    }
    /*
     * Check the size! Check the size! 
     */
    if ( edd->max_size != NOLIMIT && TOTAL_BUFFER_SIZE( edd ) >= edd->max_size ) {
        send_to_char( "As a result of this operation, the buffer would grow\r\n"
                      "larger than its maximum allowed size. Operation has been\r\ncancelled.\r\n",
                      ch );
        formatted_lines = detach_line_range( edd, linefrom, linefrom + line_count - 1 );
        attach_line_range( edd, linefrom, eline_list );
    }
    else {
        discard_line_list( eline_list );
        ch_printf( ch, "Formatted to %d lines.\r\n", 40 );
    }
    if ( complete_format )
        set_online_index( edd, edd->line_count );
    return;
}

/* Misc functions */
/* Formats the given list of lines neatly and returns the formatted
 * result in a new list. "Formatting" consists in wrapping lines to
 * have a max of "max_width" at most, doing word wrapping, and getting
 * rid of excess whitespace, except for the whitespace at the start
 * of the first line. */
EDITOR_LINE            *format_text( EDITOR_LINE * line_list, short max_width )
{
    char                   *dest_buf;
    short                   dest_used,
                            dest_size;
    short                   cur_width,
                            i,
                            j;
    short                   start_word,
                            end_word;
    EDITOR_LINE            *eline,
                           *last_line;
    EDITOR_DATA            *edd;

    if ( !line_list ) {
        bug( "%s", "format_text: Null EDITOR_LINE" );
    }
    /*
     * set up a destination buffer 
     */
    CREATE( dest_buf, char, CHAR_BLOCK );

    dest_used = 1;
    dest_size = CHAR_BLOCK;
    cur_width = 0;
    j = 0;
    /*
     * set up source coordinates 
     */
    eline = line_list;
    i = 0;
    /*
     * copy first line whitespace
     * * loop while space and while size is less than CHAR_BLOCK
     * * copy to destination 
     */
    while ( isspace( eline->line[i] ) && dest_used < CHAR_BLOCK )
        dest_buf[j++] = eline->line[i++];
    dest_buf[j] = '\0';
    cur_width = j;
    dest_used = j + 1;
    /*
     * loop through lines 
     */
    while ( eline ) {
        /*
         * loop within line 
         */
        while ( eline->line[i] != '\0' ) {
            /*
             * loop while space (ignore) 
             */

/* Check for TWO spaces after a full stop. */
            while ( isspace( eline->line[i] ) && eline->line[i] != '\0' ) {
/*
        if(eline->line[i-1] == "." && !isspace(eline->line[i-2]) && eline->line[i+1] != '\0' && !isspace(eline->line[i+1]) )  // Volk - something. moo
        {

        }
*/
                i++;
            }

            if ( eline->line[i] == '\0' )
                break;
            start_word = i;
            /*
             * loop while non-space 
             */
            while ( !isspace( eline->line[i] ) && eline->line[i] != '\0' )
                i++;
            end_word = i;
            /*
             * if current width plus the word is bigger than max width 
             */
            if ( cur_width + ( end_word - start_word ) >= max_width ) {
                /*
                 * resize if needed 
                 */
                RESIZE_IF_NEEDED( dest_buf, dest_size, dest_used, 2 );
                /*
                 * break a newline 
                 */
                dest_buf[dest_used - 1] = '\0';
                /*
                 * The -1 is to overwrite the last space 
                 */
                strcat( dest_buf, "\r\n" );
                dest_used += 2;
                cur_width = 0;
            }
            /*
             * resize if needed 
             */
            RESIZE_IF_NEEDED( dest_buf, dest_size, dest_used, ( end_word - start_word ) + 1 );
            /*
             * add the word, plus a space 
             */
            strncat( dest_buf, eline->line + start_word, ( end_word - start_word ) );
            strcat( dest_buf, " " );
            dest_used += ( end_word - start_word ) + 1;
            cur_width += ( end_word - start_word ) + 1;
        }
        /*
         * advance the line coordinates 
         */
        eline = eline->next;
        i = 0;
    }
    /*
     * convert the dest buffer to a list of lines 
     */
    edd = str_to_editdata( dest_buf, NOLIMIT );
    eline = edd->first_line;
    edd->first_line = NULL;
    discard_editdata( edd );
    DISPOSE( dest_buf );
    /*
     * get rid of the last empty line .. 
     */
    if ( eline->next ) {
        last_line = eline;
        while ( last_line->next->next )
            last_line = last_line->next;
        discard_line_list( last_line->next );
        last_line->next = NULL;
    }
    return eline;
}

/* Replaces a word word_src in src for word_dst. Returns a pointer to a newly 
 * allocated buffer containing the line with the replacements. Stores in 
 * pnew_size the size of the allocated buffer, wich may be different from the
 * length of the string and is a multiple of CHAR_BLOCK. Stores in prepl_count
 * the number of replacements it made */
char                   *text_replace( char *src, char *word_src, char *word_dst, short *pnew_size,
                                      short *prepl_count )
{
    char                   *dst_buf;
    char                   *next_found,
                           *last_found;
    short                   dst_used,
                            dst_size,
                            len;
    short                   repl_count;

    /*
     * prepare the destination buffer 
     */
    CREATE( dst_buf, char, CHAR_BLOCK );

    dst_size = CHAR_BLOCK;
    dst_used = 0;
    dst_buf[0] = '\0';
    last_found = src;
    repl_count = 0;
    for ( ;; ) {
        /*
         * look for next instance of word 
         */
        next_found = strstr( last_found, word_src );
        if ( next_found == NULL ) {
            /*
             * if theres no more instances of word,
             * copy the rest of the src 
             */
            len = strlen( last_found );
            RESIZE_IF_NEEDED( dst_buf, dst_size, dst_used, len + 1 );
            strcat( dst_buf, last_found );
            dst_used += len;
            break;
        }
        /*
         * copy the buffer up to this instance of the word
         * and then copy the replacement word 
         */
        len = next_found - last_found + strlen( word_dst );
        RESIZE_IF_NEEDED( dst_buf, dst_size, dst_used, len + 1 );
        strncat( dst_buf, last_found, next_found - last_found );
        strcat( dst_buf, word_dst );
        dst_used += len;
        last_found = next_found + strlen( word_src );
        repl_count++;
    }
    *pnew_size = dst_size;
    *prepl_count = repl_count;
    return dst_buf;
}

/* Pick off one argument from a string and return the rest.
   Understands quotes.
   A pickier version than regular one_argument, it will not
   convert to lowercase, and it can handle the (') character
   when it's escaped inside '. */
char                   *finer_one_argument( char *argument, char *arg_first )
{
    char                    cEnd;
    short                   count;
    bool                    escaped;

    count = 0;

    while ( isspace( *argument ) )
        argument++;
    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;
    escaped = FALSE;
    while ( *argument != '\0' || ++count >= MIL ) {
        if ( cEnd != ' ' && escaped ) {
            if ( *argument == '\\' )
                *arg_first = '\\';
            else if ( *argument == '\'' )
                *arg_first = '\'';
            else if ( *argument == '"' )
                *arg_first = '"';
            else
                *arg_first = *argument;
            arg_first++;
            argument++;
            escaped = FALSE;
            continue;
        }
        if ( cEnd != ' ' && *argument == '\\' && !escaped ) {
            escaped = TRUE;
            argument++;
            continue;
        }
        if ( *argument == cEnd ) {
            argument++;
            break;
        }
        *arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';
    while ( isspace( *argument ) )
        argument++;
    return argument;
}

#undef CHAR_BLOCK
#undef BLOCK_ROUNDUP
#undef NOLIMIT
#undef RESIZE_IF_NEEDED
#undef TOTAL_BUFFER_SIZE

void nstralloc( char **pointer )
{
    if ( !*pointer )
        *pointer = STRALLOC( "" );
    return;
}
