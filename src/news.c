/***************************************************************************
 *                                                                         *
 *   .oOOOo.    o                                        o                 *
 *  .O     o   O                             o          O                  *
 *  o          o                                        o                  *
 *  o          O                                        O                  *
 *  o          OoOo.  `OoOo.  .oOo.  'OoOo.  O   .oOo   o   .oOo.  .oOo    *
 *  O          o   o   o      O   o   o   O  o   O      O   OooO'  `Ooo.   *
 *  `o     .o  o   O   O      o   O   O   o  O   o      o   O          O   *
 *   `OoooO'   O   o   o      `OoO'   o   O  o'  `OoO'  Oo  `OoO'  `OoO'   *
 *                                                                         *
 ***************************************************************************
 * - Chronicles Copyright 2001-2003 by Brad Ensley (Orion Elder)           *
 ***************************************************************************
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - News module                                                           *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/news.h"

/*
 * Global Variables
 */
NEWS_DATA              *first_news;
NEWS_DATA              *last_news;
bool                    USE_HTML_NEWS;

/*
 * Local Functions
 */
void load_news          args( ( void ) );
void add_news           args( ( char *argument ) );
void write_news         args( ( void ) );
void generate_html_news args( ( void ) );
void show_news          args( ( CHAR_DATA *ch, int show_type, int count ) );
char                   *align_news args( ( char *argument ) );
char                   *news_argument args( ( char *argument, char *arg_first ) );
NEWS_DATA              *fread_news args( ( FILE * fpin ) );
void clear_news         args( ( bool sMatch, int nCount ) );
void format_posttime    args( ( NEWS_DATA * news ) );
char                   *html_color args( ( char *color, char *text ) );
char                   *html_format args( ( char *argument ) );

void do_news( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_NEWS_LENGTH],
                            arg2[MAX_NEWS_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( argument && argument[0] == '\0' ) {
        send_to_char( "\r\nSyntax: news [<option>]\r\n", ch );
        send_to_char( "Option being: version, all, last [<#>], first [<#>], [#]\r\n", ch );
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char( "\r\nSyntax: news [<field>]\r\n", ch );
            send_to_char( "Field being: add, load, edit, remove\r\n", ch );
            send_to_char( "  <date>, list\r\n", ch );
            return;
        }
        return;
    }

    if ( argument && argument[0] != '\0' ) {
        argument = one_argument( argument, arg1 );
        if ( !str_cmp( arg1, "version" ) ) {
            ch_printf_color( ch, "&cT&Che &cE&Clder &cC&Chronicles &cV&Cersion&c: &W%s&c.\r\n",
                             NEWS_VERSION );
            return;
        }
        else if ( is_number( arg1 ) ) {
            show_news( ch, TYPE_SHOW_ONE, atoi( arg1 ) );
            return;
        }
        else if ( !str_cmp( arg1, "all" ) ) {
            show_news( ch, TYPE_ALL, -1 );
            send_to_char( "\r\nFor more details see also 'help news'\r\n", ch );
            return;
        }
        else if ( !str_cmp( arg1, "first" ) ) {
            int                     show_count = -1;

            argument = one_argument( argument, arg2 );
            if ( !arg2 || arg2[0] == '\0' ) {
                show_count = -1;
            }
            else {
                if ( is_number( arg2 ) )
                    show_count = atoi( arg2 );
                else
                    show_count = -1;
            }
            show_news( ch, TYPE_LIST_FIRST, show_count );
        }
        else if ( !str_cmp( arg1, "last" ) ) {
            int                     show_count = -1;

            argument = one_argument( argument, arg2 );
            if ( !arg2 || arg2[0] == '\0' ) {
                show_count = 5;
            }
            else {
                if ( is_number( arg2 ) )
                    show_count = atoi( arg2 );
                else
                    show_count = -1;
            }

            show_news( ch, TYPE_LIST_LAST, show_count );
            send_to_char( "\r\nFor more details see also 'help news'\r\n", ch );
            return;
        }
        else if ( !IS_IMMORTAL( ch ) ) {
            show_news( ch, TYPE_NORMAL, -1 );
            return;
        }
        else if ( !str_cmp( arg1, "html" ) ) {
            if ( !argument || argument[0] == '\0' ) {
                ch_printf_color( ch, "&C&GHTML News Creation is &C&W%s&C&G.\r\n",
                                 USE_HTML_NEWS == TRUE ? "ON" : "OFF" );
                return;
            }
            else if ( !str_cmp( argument, "toggle" ) || !str_cmp( argument, "on" )
                      || !str_cmp( argument, "off" ) ) {
                if ( !str_cmp( argument, "toggle" ) )
                    USE_HTML_NEWS = !USE_HTML_NEWS;
                else if ( !str_cmp( argument, "on" ) )
                    USE_HTML_NEWS = TRUE;
                else
                    USE_HTML_NEWS = FALSE;

                do_news( ch, ( char * ) "html" );
                write_news(  );
                if ( USE_HTML_NEWS == TRUE )
                    generate_html_news(  );
                return;
            }
            else {
                do_news( ch, ( char * ) "html" );
                do_help( ch, ( char * ) "news" );
                return;
            }
        }
        else if ( !str_cmp( arg1, "add" ) && ( argument && argument[0] != '\0' ) ) {
            add_news( argument );
            send_to_char_color( "&C&GNews added.\r\n", ch );
        }
        else if ( !str_cmp( arg1, "load" ) ) {
            clear_news( FALSE, 0 );
            load_news(  );
            if ( USE_HTML_NEWS == TRUE )
                generate_html_news(  );
            send_to_char_color( "&C&GNews loaded.\r\n", ch );
        }
        else if ( !str_cmp( arg1, "list" ) ) {
            show_news( ch, TYPE_IMM_LIST, -1 );
        }
        else if ( !str_cmp( arg1, "remove" ) && ( argument && argument[0] != '\0' ) ) {
            bool                    clearAll = FALSE;

            if ( !str_cmp( argument, "all" ) ) {
                clearAll = TRUE;
            }
            if ( !clearAll && !is_number( argument ) ) {
                send_to_char_color( "Argument must be a number.\r\n", ch );
                return;
            }
            if ( clearAll != TRUE )
                clear_news( TRUE, atoi( argument ) );
            else
                clear_news( FALSE, 0 );
            write_news(  );
            if ( USE_HTML_NEWS == TRUE )
                generate_html_news(  );
            send_to_char_color( "&C&GNews removed.\r\n", ch );
        }
        else {
            do_help( ch, ( char * ) "news" );
            return;
        }
    }
    else {
        show_news( ch, TYPE_NORMAL, -1 );
    }
}

void load_news(  )
{
    FILE                   *fpin = FileOpen( NEWS_FILE, "r" );

    if ( fpin == NULL ) {
        bug( "Cannot open news.dat for reading." );
        perror( NEWS_FILE );
        return;
    }

    for ( ;; ) {
        char                    letter;
        char                   *word;
        NEWS_DATA              *news;

        letter = fread_letter( fpin );

        if ( letter == '*' ) {
            fread_to_eol( fpin );
            continue;
        }

        if ( letter != '#' ) {
            bug( "Load_news: # not found." );
            break;
        }

        word = ( feof( fpin ) ? ( char * ) "End" : fread_word( fpin ) );

        if ( !str_cmp( word, "News" ) ) {
            news = fread_news( fpin );

            if ( news != NULL )
                LINK( news, first_news, last_news, next, prev );
        }
        else if ( !str_cmp( word, "HTML" ) ) {
            USE_HTML_NEWS = fread_number( fpin ) == 1 ? TRUE : FALSE;
        }
        else if ( !str_cmp( word, "End" ) ) {
            FileClose( fpin );
            break;
        }
        else {
            bug( "Load_news: bad section encountered." );
            break;
        }
    }
}

void add_news( char *argument )
{
    NEWS_DATA              *news = NULL;

    CREATE( news, NEWS_DATA, 1 );
    news->time_stamp = time( NULL );
    news->data = align_news( argument );
    format_posttime( news );
    LINK( news, first_news, last_news, next, prev );

    write_news(  );
    if ( USE_HTML_NEWS == TRUE )
        generate_html_news(  );
}

void write_news(  )
{
    FILE                   *fpout;
    NEWS_DATA              *news;

    if ( ( fpout = FileOpen( NEWS_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open news.dat for writing." );
        perror( NEWS_FILE );
        return;
    }

    fptof( fpout, "#HTML\n" );
    fprintf( fpout, "%d\n\n", USE_HTML_NEWS == TRUE ? 1 : 0 );
    for ( news = first_news; news != NULL; news = news->next ) {
        fptof( fpout, "#News\n" );
        fprintf( fpout, "TimeStamp %ld\n", news->time_stamp > 0 ? news->time_stamp : -1 );
        fprintf( fpout, "NewsData  %s~\n", strip_cr( news->data ) );
        fptof( fpout, "End\n\n" );
    }
    fptof( fpout, "#End\n" );

    FileClose( fpout );
}

void generate_html_news(  )
{
    FILE                   *fpout;
    NEWS_DATA              *news;

    if ( ( fpout = FileOpen( HTML_NEWS_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open %s for writing.", HTML_NEWS_FILE );
        perror( HTML_NEWS_FILE );
        return;
    }
    fptof( fpout, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n" );
    fptof( fpout, "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" );
    fptof( fpout, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n" );
    fptof( fpout, "  <head>\n" );
    fprintf( fpout, "    <title>%s Game News</title>\n", sysdata.mud_name );
    fptof( fpout,
           "<meta name=\"description\" content=\"What is 6 Dragons? 6 Dragons is a unique multiplayer user dimension (MUD) which has been in existence for over 2 years. This text based environment lets you assume the role of a character you create and interact with other players and the inhabitants of a medieval fantasy world. 6 Dragons Mud is based in a medieval fantasy world where simple magic is common and other higher magic must be sought out from teachers. There are heaps of exotic realms to explore, with many quests to be completed. 6 Dragons is similar to a role-playing game where you have absolute control over your characters actions, and you decide what course the character will take in the game.\"/>\n" );
    fptof( fpout,
           "<meta name=\"keywords\" content=\"dragon, harry potter, medieval, wow, eq, mmorpg, free, game, mud, smaug\" />\n" );
    fptof( fpout,
           "<link rel=\"stylesheet\" type=\"text/css\" href=\"http://6dragons.org/style.css\" media=\"screen\"/>\n" );
    fptof( fpout, "<style type=\"text/css\">\n" );
    fptof( fpout, "/*<![CDATA[*/\n" );
    fptof( fpout, "div.c2 {text-align: center}\n" );
    fptof( fpout, "div.c1 {margin-left: 0%;text-align: left}\n" );
    fptof( fpout, "/*]]>*/\n" );
    fptof( fpout, "</style>\n" );
    fptof( fpout, "</head>\n" );
    fptof( fpout, "<body>\n" );
    fptof( fpout, "<div class=\"c2\" id=\"wrapper\">\n" );
    fptof( fpout, "<div id=\"container\">\n" );
    fptof( fpout, "<div class=\"header\"></div>\n" );
    fptof( fpout,
           "<div class=\"navigation\"><table cellspacing=\"0\" cellpadding=\"0%\"><tr><th><a href=\"http://6dragons.org/index.html\">Home</a></th> <th><a href=\"telnet://6dragons.org:4000\">Play</a></th><th> <a href=\"http://6dragons.org/bin/vote.html\">Vote</a> </th><th><a href=\"http://6dragons.org/bin/staff.html\">Staff</a></th><th> <a href=\"http://6dragons.org/bin/races.html\">Races</a> </th><th><a href=\"http://6dragons.org/bin/classes.html\">Classes</a> </th><th><a href=\"http://6dragons.org/bin/slists.html\">Slists</a></th><th> <a href=\"http://6dragons.org/bin/guide.html\">Guide</a> </th><th><a href=\"http://6dragons.org/news.html\">News</a></th><th> <a href=\"http://6dragons.org/bin/story.html\">Story</a></th><th><th><a href=\"http://6dragons.org/bin/wild.html\">Wilderness</a></th></tr></table>\n" );
    fptof( fpout, "</div>\n" );
    fptof( fpout, "\n<div class=\"c1\" id=\"wrapper\">\n" );
    fptof( fpout, "\n<p>\n\n\n\n" );

    fptof( fpout, "        <table width=\"90%\" border=\"0\" align=\"CENTER\">\n" );

    for ( news = first_news; news != NULL; news = news->next ) {
        /*
         * Day, Month, and Year in their raw form.
         */
        char                    day[MAX_NEWS_LENGTH];
        char                    month[MAX_NEWS_LENGTH];
        char                    year[MAX_NEWS_LENGTH];

        /*
         * Day, Month and Year after formatted for HTML.
         */
        char                    mon_buf[MAX_NEWS_LENGTH];
        char                    day_buf[MAX_NEWS_LENGTH];
        char                    year_buf[MAX_NEWS_LENGTH];

        /*
         * Date separator after formatted for HTML.
         */
        char                    sep_buf[MAX_NEWS_LENGTH];

        /*
         * Date and News after completely formatted and ready for HTML export.
         */
        char                    date_buf[MAX_NEWS_LENGTH];
        char                    news_data_buf[MAX_NEWS_LENGTH];

        sprintf( day, "%2.2d", news->day );
        sprintf( month, "%2.2d", news->month );
        sprintf( year, "%4.4d", news->year );
        sprintf( mon_buf, "%s", month );
        sprintf( day_buf, "%s", day );
        sprintf( year_buf, "%s", year );
        sprintf( sep_buf, "%s", "/" );

        sprintf( date_buf, "%s%s%s%s%s", mon_buf, sep_buf, day_buf, sep_buf, year_buf );
        sprintf( news_data_buf, "%s", news->data );
        fptof( fpout, "          <tr>\n" );

        fptof( fpout, "            <td width=\"20%\" valign=\"TOP\">\n" );
        fprintf( fpout, "%s\n", date_buf );
        fptof( fpout, "            </td>\n" );

        fptof( fpout, "            <td width=\"80%\" valign=\"TOP\">\n" );
        fprintf( fpout, "%s\n", news_data_buf );

        fptof( fpout, "            </td>\n" );

        fptof( fpout, "          </tr>\n\n" );
    }

    fptof( fpout, "        </table>\n" );

    fptof( fpout, "<br>\n<br>\n" );
    fptof( fpout, "<br>\n<br>\n" );
    fptof( fpout, "</p>\n\n" );
    fptof( fpout, "          <small>\n" );
    fprintf( fpout,
             "Chronicles News System<a href=\"mailto:orion_elder@charter.net?subject=The Elder Chronicles News Snippet, %s\">Orion Elder</a>.<br>",
             sysdata.mud_name );
    fptof( fpout,
           "Download at<a href=\"http://www.geocities.com/knytehawk/smaug/index.html\" target=\"SSWRA\">The SSWRA</a>.<br>" );
    fptof( fpout, "          </small>\n" );

    fptof( fpout, "\n\n\n\n" );
    fptof( fpout, "</div>\n" );
    fptof( fpout, "</div>\n" );
    fptof( fpout, "</div>\n" );
    fptof( fpout, "</div>\n" );
    fptof( fpout, "  </body>\n" );
    fptof( fpout, "</html>\n" );

    FileClose( fpout );
}

void show_news( CHAR_DATA *ch, int show_type, int count )
{
    NEWS_DATA              *news = NULL,
        *curr_news = NULL;
    int                     nCount = 1;
    char                    buf[MAX_NEWS_LENGTH * 2];
    char                    bar[MAX_NEWS_LENGTH * 2];

    if ( IS_BLIND( ch ) ) {
        send_to_char( "6D News\r\n\r\n", ch );
    }
    else {
        sprintf( bar,
                 "%s--------------------------------=[ %s6D News %s]=---------------------------------\r\n",
                 AT_6D, AT_TRANS, AT_6D );
        send_to_pager_color( bar, ch );
    }
    switch ( show_type ) {
        case TYPE_SHOW_ONE:
            for ( news = first_news; news; news = news->next ) {
                if ( nCount++ != count )
                    continue;
                sprintf( buf, "%s%2.2d%s/%s%2.2d%s/%s%4.4d  %s%s\r\n", AT_DATE, news->month,
                         AT_SEPARATOR, AT_DATE, news->day, AT_SEPARATOR, AT_DATE, news->year,
                         AT_NEWS, news->data );
                send_to_pager_color( buf, ch );
                break;
            }
            break;

        case TYPE_IMM_LIST:
            for ( news = first_news, nCount = 1; news != NULL; news = news->next, nCount++ ) {
                sprintf( buf, "%s%10d%s]  %s%s\r\n", AT_DATE, nCount, AT_SEPARATOR, AT_NEWS,
                         news->data );
                send_to_pager_color( buf, ch );
            }
            break;

        case TYPE_NORMAL:
        case TYPE_ALL:
        case TYPE_LIST_FIRST:
            for ( news = first_news, nCount = 1; news != NULL; news = news->next, nCount++ ) {
                bool                    fShow = FALSE;
                bool                    fBreakOut = FALSE;

                if ( show_type == TYPE_ALL )
                    fShow = TRUE;
                else if ( show_type == TYPE_NORMAL
                          && ( ( news->time_stamp > ch->pcdata->last_read_news ) || !news->next ) )
                    fShow = TRUE;
                else if ( show_type == TYPE_LIST_FIRST ) {
                    if ( count > 0 ) {
                        if ( nCount <= count )
                            fShow = TRUE;
                    }
                    else {
                        news = first_news;
                        fShow = TRUE;
                        fBreakOut = TRUE;
                    }
                }
                if ( news && fShow == TRUE ) {
                    sprintf( buf, "%s%2.2d%s/%s%2.2d%s/%s%4.4d  %s%s\r\n", AT_DATE, news->month,
                             AT_SEPARATOR, AT_DATE, news->day, AT_SEPARATOR, AT_DATE, news->year,
                             AT_NEWS, news->data );
                    send_to_pager_color( buf, ch );
                    if ( news->time_stamp > ch->pcdata->last_read_news ) {
                        ch->pcdata->last_read_news = news->time_stamp;
                    }
                    if ( fBreakOut == TRUE )
                        break;
                }
            }
            break;

        case TYPE_LIST_LAST:
            if ( count > 0 ) {
                for ( news = last_news, nCount = 1; news != NULL && nCount <= count;
                      news = news->prev, nCount++ ) {
                    curr_news = news;
                }
            }
            else
                curr_news = last_news;

            for ( news = curr_news; news != NULL; news = news->next ) {
                if ( news ) {
                    sprintf( buf, "%s%2.2d%s/%s%2.2d%s/%s%4.4d  %s%s\r\n", AT_DATE, news->month,
                             AT_SEPARATOR, AT_DATE, news->day, AT_SEPARATOR, AT_DATE, news->year,
                             AT_NEWS, news->data );
                    send_to_pager_color( buf, ch );
                    if ( news->time_stamp > ch->pcdata->last_read_news ) {
                        ch->pcdata->last_read_news = news->time_stamp;
                    }
                }
            }
            break;

        default:
            break;
    }
}

char                   *align_news( char *argument )
{
    char                    buf[MAX_NEWS_LENGTH];
    char                    arg[MAX_NEWS_LENGTH];
    char                   *return_buf;
    int                     num = 0;
    int                     count = 0;
    int                     date_len = 10;
    int                     spacer = 2;
    int                     total = ( date_len + spacer );

    strcpy( buf, "" );

    if ( argument == NULL || argument[0] == '\0' )
        return ( char * ) "";

    for ( ;; ) {
        int                     i = 0;
        int                     length = 0;
        int                     longlen = 0;

        argument = news_argument( argument, arg );

        // We use the length without the color spaces for wrapping
        length = strlen_color( arg );

        if ( ( total + length ) >= 79 ) {
            int                     index;

            buf[num] = '\n';
            num++;
            buf[num] = '\r';
            num++;
            for ( index = 0; index < ( date_len + spacer ); index++ ) {
                buf[num] = ' ';
                num++;
            }
            total = ( date_len + spacer );
        }

        // We use the length with the color spaces for substitution
        longlen = strlen( arg );

        for ( i = 0; i < longlen; i++ ) {
            if ( arg[count] == '&' || arg[count] == '^' ) {
                if ( arg[count + 1] == '\0' ) {
                    arg[count] = '\0';
                }
                else if ( ( arg[count] == '&' && arg[count + 1] == '&' )
                          || ( arg[count] == '^' && arg[count + 1] == '^' ) ) {
                    buf[num] = arg[count];
                    num++;
                    count++;
                    i++;
                    buf[num] = arg[count];
                    num++;
                    count++;
                    total++;
                }
                else {
                    count += 2;
                    i++;
                }
            }
            else {
                buf[num] = arg[count];
                total++;
                num++;
                count++;
            }
        }

        if ( argument != NULL && argument[0] != '\0' ) {
            buf[num] = ' ';
            num++;
            total++;
            count = 0;
        }
        else {
            buf[num] = '\0';
            break;
        }
    }

    return_buf = STRALLOC( buf );

    return return_buf;
}

char                   *news_argument( char *argument, char *arg_first )
{
    char                    cEnd;
    short                   count;

    count = 0;

    if ( !argument || argument[0] == '\0' ) {
        arg_first[0] = '\0';
        return argument;
    }

    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';

    while ( *argument != '\0' || ++count >= 255 ) {
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

NEWS_DATA              *fread_news( FILE * fpin )
{
    const char             *word;
    bool                    fMatch;
    NEWS_DATA              *news = NULL;

    CREATE( news, NEWS_DATA, 1 );

    for ( ;; ) {
        word = feof( fpin ) ? "End" : fread_word( fpin );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fpin );
                break;

            case 'D':
                if ( !str_cmp( word, "Day" ) ) {
                    news->day = fread_number( fpin );
                    fMatch = TRUE;
                    break;
                }

            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return news;

            case 'M':
                if ( !str_cmp( word, "Month" ) ) {
                    news->month = fread_number( fpin );
                    fMatch = TRUE;
                    break;
                }

            case 'N':
                if ( !str_cmp( word, "NewsData" ) ) {
                    news->data = fread_string( fpin );
                    fMatch = TRUE;
                    break;
                }

            case 'T':
                if ( !str_cmp( word, "TimeStamp" ) ) {
                    news->time_stamp = fread_number( fpin );
                    if ( news->time_stamp > 0 ) {
                        format_posttime( news );
                    }
                    fMatch = TRUE;
                    break;
                }

            case 'Y':
                if ( !str_cmp( word, "Year" ) ) {
                    news->year = fread_number( fpin );
                    fMatch = TRUE;
                    break;
                }
        }

        if ( !fMatch ) {
            bug( "Load_news: no match: %s", word );
            bug( word, 0 );
        }
    }
    return NULL;
}

void clear_news( bool sMatch, int nCount )
{
    int                     nCurrent = 1;
    NEWS_DATA              *news;

    if ( sMatch == FALSE ) {
        while ( ( news = first_news ) != NULL ) {
            STRFREE( news->data );
            UNLINK( news, first_news, last_news, next, prev );
            DISPOSE( news );
        }
    }
    else {
        for ( news = first_news; news != NULL; news = news->next ) {
            if ( nCount == nCurrent ) {
                STRFREE( news->data );
                UNLINK( news, first_news, last_news, next, prev );
                DISPOSE( news );
                break;
            }
            else
                nCurrent++;
        }
    }
}

void format_posttime( NEWS_DATA * news )
{
    if ( news == NULL )
        return;

    if ( news->time_stamp > 0 ) {
        int                     day,
                                month,
                                year;
        struct tm              *time = localtime( &news->time_stamp );

        day = time->tm_mday;
        month = ( time->tm_mon + 1 );
        year = ( time->tm_year + 1900 );

        news->day = day;
        news->month = month;
        news->year = year;
    }
    else {
        int                     day,
                                month,
                                year;
        time_t                  t = time( NULL );
        struct tm              *time = localtime( &t );

        day = time->tm_mday;
        month = ( time->tm_mon + 1 );
        year = time->tm_year;

        news->day = day;
        news->month = month;
        news->year = year;
        news->time_stamp = t;
    }

    return;
}

char                   *html_color( const char *color, const char *text )
{
    static char             buf[MAX_HTML_LENGTH];

    if ( !color || color[0] == '\0' ) {
        html_color( "#2e1806", text );
    }
    else if ( !text || text[0] == '\0' ) {
        html_color( color, " " );
    }
    else {
        sprintf( buf, "%s%s%c", color, text, '\0' );
    }

    return buf;
}

/*
char *html_format( char *argument )
{
    int index = 0, bufcount = 0;
    char bad_chars[] = { '<', '>' };
    char rep_chars[][MAX_NEWS_LENGTH] = { "&#60;", "&#62;" };
    static char buf[MAX_HTML_LENGTH];

    if ( !argument || argument[0] == '\0' )
    {
	return "";
    }

    buf[0] = '\0';

    while( *argument != '\0' )
    {
	bool cFound = FALSE;

	for( index = 0; index < strlen( bad_chars ); index++ )
	{
	    if ( *argument == bad_chars[index] )
	    {
		cFound = TRUE;
		break;
	    }
	}

	if ( cFound )
	{
	    int temp;
	    char new_char[MAX_HTML_LENGTH];

	    sprintf( new_char, "%s", rep_chars[index] );

	    for( temp = 0; temp < strlen( new_char ); temp++, bufcount++ )
	    {
		buf[bufcount] = new_char[temp];
	    }
	}
	else
	{
	    buf[bufcount] = *argument;
	    bufcount++;
	}

	argument++;
    }

    buf[bufcount] = '\0';
    return buf;
}
*/

char                   *html_format( char *argument )
{
    return argument;
}
