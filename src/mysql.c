/*  Volk's attempt at making contact with the MYSQL server, so we can
 *  eventually integrate the back end of the website PHP code with the
 *  mud. Seems like a lot of code, and will have to make tables etc,
 *  but i'll give it a go.  THYQUEST.COM 4000
 */
/* Wow! So this is old. Nearly 2008 now.. But i'm going to finally do it.
 * First step is the planning. I'm going to put accounts into the nanny function. These are going to be based
 * on an account id/name which will first check if said account name exists on the forums. If so, it will then
 * prompt for the forums password.
 * If a player tries to create an account with a name that is already taken on the forums, but NOT in the game
 * , it will say 'this account has been registered on our webpage. do you wish to choose another name or 
 * type the password to enter this account?'.
 * I also need a way to create a forums account for every new account that comes in. Just check the website code,
 * easy.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <string>
#include "h/mud.h"
#include "h/mysql.h"
#if !defined(WIN32)
#include "/usr/include/mysql/mysql.h"
#endif
#include "h/md5.h"

void                    to_channel( const char *argument, const char *xchannel, int level );
bool                    db_connect;

MYSQL_RES              *result;
MYSQL                  *db;
char                    sql[1000];

void connect_mysql(  )
{
    if ( ( db = mysql_init( NULL ) ) == NULL ) {
        bug( "%s", "connect_mysql: init error" );
        return;
    }
    if ( !mysql_real_connect
         ( db, DB_HOST, DB_USER, DB_PWD, DB_DB, DB_PORT, DB_SOCKET, DB_CLIENT_FLAG ) ) {
        bug( "connect_mysql: error: %u %s", mysql_errno( db ), mysql_error( db ) );
        return;
    }
    db_connect = TRUE;
//  db_last_action = 0;   
    return;
}

void disconnect_mysql(  )
{
    mysql_close( db );
    db_connect = FALSE;
    db = NULL;
    return;
}

void reconnect_mysql(  )
{
    disconnect_mysql(  );
    connect_mysql(  );
}

// Route ALL mysql queries through this wrapper to ensure they are escaped
// properly, to thwart various SQL injection attacks.
int mysql_safe_query( char *fmt, ... )
{
    va_list                 argp;
    int                     i = 0;
    double                  j = 0;
    char                   *s = 0,
        *out = 0,
        *p = 0;
    char                    safe[MAX_STRING_LENGTH];
    char                    query[MAX_STRING_LENGTH];
    int                     result;

    *query = '\0';
    *safe = '\0';

    va_start( argp, fmt );

    for ( p = fmt, out = query; *p != '\0'; p++ ) {
        if ( *p != '%' ) {
            *out++ = *p;
            continue;
        }

        switch ( *++p ) {
            case 'c':
                i = va_arg( argp, int );

                out += sprintf( out, "%c", i );;
                break;
            case 's':
                s = va_arg( argp, char * );

                if ( !s ) {
                    out += sprintf( out, " " );
                    break;
                }
                mysql_real_escape_string( db, safe, s, strlen( s ) );
                out += sprintf( out, "%s", safe );
                break;
            case 'd':
                i = va_arg( argp, int );

                out += sprintf( out, "%d", i );
                break;
            case 'f':
                j = va_arg( argp, double );

                out += sprintf( out, "%f", j );
                break;
            case '%':
                out += sprintf( out, "%%" );
                break;
        }
    }

    *out = '\0';

    va_end( argp );

    result = mysql_real_query( db, query, strlen( query ) );

    return ( result );
}

// Convert unix time to mysql-style datetime string
// mysql_time must be at least 20 char lenth string
void time2mysql( time_t unixtime, char *mysql_time )
{
    struct tm               time_tm;

    localtime_r( &unixtime, &time_tm );
    strftime( mysql_time, 20, "%Y-%m-%d %H:%M:%S", &time_tm );
}

// Convert mysql-style datetime string into unix time
time_t mysql2time( const char *mysql_time )
{
    struct tm               time_tm;

    strptime( mysql_time, "%Y-%m-%d %H:%M:%S", &time_tm );
    return mktime( &time_tm );
}

/* checks their password hasn't been changed.. */
int mysql_checkpass( int userid, char *password )
{
    char                    query[MAX_STRING_LENGTH];
    MYSQL_RES              *res;
    MYSQL_ROW               row;

    if ( !db_connect )
        connect_mysql(  );

    if ( db == NULL )
        return -1;

//  mudstrlcpy(query, "select mudpass from forums_users where user_id = '", 1000);
//  mudstrlcat(query, userid, 1000);
//  mudstrlcat(query, "'", 1000);
    snprintf( query, 1000, "select mudpass from forums_users where user_id = %d", userid );

//  sql = mysql_exec(db, query);
    if ( mysql_query( db, query ) )
        return -2;
/* Failed to query */

    res = mysql_use_result( db );

    if ( res )
        row = mysql_fetch_row( res );
    else
        return -3;

/* We have a res here, we'll need to free it at the end. */
    int                     buf;

    if ( !row )
        buf = -4;
    else if ( !str_cmp( row[0], password ) )
        buf = -5;
    else
        buf = -6;

    mysql_free_result( res );
    return buf;
}

/* Checks for valid username/password. Returns -1 if no username found, -2 if password wrong, -3 for error
   else returns their forums ID */
int mysql_checkaccount( char *username, char *password )
{
    char                    query[MAX_STRING_LENGTH];
    MYSQL_RES              *res;
    MYSQL_ROW               row;

    if ( !db_connect )
        connect_mysql(  );

    if ( db == NULL )
        return -3;

    mudstrlcpy( query, "select user_id, mudpass from forums_users where username_clean = '", 1000 );
    mudstrlcat( query, username, 1000 );
    mudstrlcat( query, "'", 1000 );

//  db_last_action = 0;

//  sql = mysql_exec(db, query);
    if ( mysql_query( db, query ) )
        return -3;
/* Failed to query */

    res = mysql_use_result( db );

    if ( res )
        row = mysql_fetch_row( res );
    else
        return -1;

/* We have a res here, we'll need to free it at the end. */
    int                     buf;

    if ( !row )
        buf = -1;

// Volk - so far we HAVE a username, we have a row with the password row[0], let's check our password against it */
    else if ( !row[1] || row[1] == '\0' )
        buf = -4;
    else if ( !str_cmp( password, row[1] ) )
        buf = atoi( row[0] );
    else
        buf = -2;

    mysql_free_result( res );
    return buf;
}

int mysql_forums( CHAR_DATA *ch, char *argument )
{
    char                    query[MAX_STRING_LENGTH];
    char                    buf[MAX_STRING_LENGTH * 100];
    char                    buf1[MAX_STRING_LENGTH * 100];
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    bool                    free_result = FALSE;
    MYSQL_RES              *res;
    MYSQL_ROW               row;

    buf[0] = '\0';

    if ( !db_connect )
        connect_mysql(  );

    if ( db == NULL )
        return -1;

    if ( !ch || !ch->pcdata )
        return 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ch->pcdata->sqlnumber == 0 && str_cmp( arg1, "register" ) ) {
        send_to_char
            ( "Before accessing forums you must register with your forums username and password.\r\n",
              ch );
        send_to_char( "Please use 'forums register' command for details.\r\n", ch );
        return 0;
    }

/* Let's check if their password has changed now.. */
    if ( ch->pcdata->sqlnumber > 0 && str_cmp( arg1, "register" ) ) {
        int                     chk = mysql_checkpass( ch->pcdata->sqlnumber, ch->pcdata->sqlpass );

        if ( chk == -1 )
            send_to_char( "No database access!\r\n", ch );
        if ( chk == -2 )
            send_to_char( "Query failed.\r\n", ch );
        if ( chk == -3 )
            send_to_char( "No result - query passed but no data.\r\n", ch );
        if ( chk == -4 )
            send_to_char
                ( "Could not find that user in the database, please register with a new name/password.\r\n",
                  ch );
        if ( chk == -6 )
            send_to_char( "Wrong password. Please register with a new name/password.\r\n", ch );
        if ( chk < -5 || chk > -5 )
            return 0;
    }

    strcat( buf, "&Y6Dragons Forum&c\r\n" );

    if ( !str_cmp( arg1, "register" ) ) {
        if ( !arg2 || !argument || !VLD_STR( arg2 ) || !VLD_STR( argument ) ) {
            send_to_char( "Syntax: forums register (username) (password)\r\n", ch );
            send_to_char( "  .. with your registered profile username and password.\r\n\r\n", ch );
            send_to_char
                ( "If you do not have a registered profile please go to 6dragons.org/forums and click the 'Register' button.\r\n",
                  ch );
            return 0;
        }

        /*
         * If we're here they HAVE entered a username and password, time to compare 
         */
        int                     check = mysql_checkaccount( arg2, argument );

        if ( check == -1 )                             /* checkaccount(username,
                                                        * password) */
            send_to_char( "Could not find that username in the database.\r\n", ch );
        else if ( check == -2 )
            send_to_char( "Wrong password.\r\n", ch );
        else if ( check == -3 )
            send_to_char( "Unknown error!\r\n", ch );
        else if ( check == -4 ) {
            send_to_char
                ( "Before we can verify your username/password, you MUST log out/in from the website forums.\r\n",
                  ch );
            send_to_char( "Please go to 6dragons.org/forums and click the 'In-Out' button.\r\n",
                          ch );
        }
        else {
            send_to_char( "Success! You can now access forums from within the mud.\r\n", ch );
            ch->pcdata->sqlpass = STRALLOC( argument ); /* Store for later comparisons */
            ch->pcdata->sqlnumber = check;
        }
        return 0;
    }

    if ( !strcmp( arg1, "" ) ) {
        bool                    first = FALSE;

        if ( IS_IMMORTAL( ch ) ) {
            sprintf( query,
                     "SELECT f.forum_id, f.forum_name, f.forum_topics, f.forum_posts, f.forum_last_post_subject, "
                     "       f.forum_last_poster_name, f.forum_last_post_time, f.parent_id "
                     "FROM forums_forums f " "ORDER BY f.left_id" );
        }
        else {
            sprintf( query,
                     "SELECT f.forum_id, f.forum_name, f.forum_topics, f.forum_posts, f.forum_last_post_subject, "
                     "       f.forum_last_poster_name, f.forum_last_post_time, f.parent_id "
                     "FROM forums_forums f " "WHERE f.forum_id != 3 AND f.parent_id != 3 "
                     "ORDER BY f.left_id" );
        }

        // 0 - f.forum_id
        // 1 - f.forum_name
        // 2 - f.forum_topics
        // 3 - f.forum_posts
        // 4 - f.forum_last_post_subject
        // 5 - f.forum_last_poster_name
        // 6 - f.forum_last_post_time
        // 7 - f.parent_id

        if ( mysql_query( db, query ) )
            return -2;

        res = mysql_use_result( db );
        free_result = TRUE;

        while ( ( row = mysql_fetch_row( res ) ) != NULL ) {
            if ( !strcmp( row[7], "0" ) ) {
                if ( first == TRUE )
                    strcat( buf, "\r\n" );
                first = TRUE;

                sprintf( buf1, "&cCategory: %s\r\n", row[1] );
                strcat( buf, buf1 );
                sprintf( buf1,
                         "FNum Forum Name                          Topic Posts Poster     Last Post Time\r\n" );
                strcat( buf, buf1 );
                sprintf( buf1,
                         "==== =================================== ===== ===== ========== ===============&C\r\n" );
                strcat( buf, buf1 );
            }
            else {
                sprintf( buf1, "%4s %-35s [%3s] [%3s] %-11s %-15s\r\n", row[0], row[1], row[2],
                         row[3], row[5], mini_c_time( atol( row[6] ), -1 ) );
                strcat( buf, buf1 );
            }
        }
    }
    else if ( is_number( arg1 ) ) {
        int                     forumid;

        forumid = atoi( arg1 );

        if ( IS_IMMORTAL( ch ) ) {
            sprintf( query,
                     "SELECT t.topic_id, t.topic_title, t.topic_first_poster_name, t.topic_time, "
                     "       t.topic_replies, t.topic_views, t.topic_last_poster_name, t.topic_last_post_time "
                     "FROM forums_topics t " "WHERE t.forum_id = %d AND t.topic_type IN (0, 1) "
                     "ORDER BY t.topic_type DESC, t.topic_last_post_time DESC", forumid );
        }
        else {
            sprintf( query,
                     "SELECT t.topic_id, t.topic_title, t.topic_first_poster_name, t.topic_time, "
                     "       t.topic_replies, t.topic_views, t.topic_last_poster_name, t.topic_last_post_time "
                     "FROM forums_topics t LEFT JOIN forums_forums f ON t.forum_id = f.forum_id "
                     "WHERE t.forum_id = %d AND t.topic_type IN (0, 1) "
                     "AND f.forum_id != 3 AND f.parent_id != 3 "
                     "ORDER BY t.topic_type DESC, t.topic_last_post_time DESC", forumid );
        }
        // 0 - t.topic_id
        // 1 - t.topic_title
        // 2 - t.topic_first_poster_name
        // 3 - t.topic_time
        // 4 - t.topic_replies
        // 5 - t.topic_views
        // 6 - t.topic_last_poster_name
        // 7 - t.topic_last_post_time

        if ( mysql_query( db, query ) )
            return -2;

        res = mysql_use_result( db );
        free_result = TRUE;

        sprintf( buf1,
                 "&cTNum Topic                                    Poster     Topic Time      Reply Views LastPoster Last Post Time\r\n" );
        strcat( buf, buf1 );
        sprintf( buf1,
                 "==== ======================================== ========== =============== ===== ===== ========== ===============&C\r\n" );
        strcat( buf, buf1 );

        while ( ( row = mysql_fetch_row( res ) ) != NULL ) {
            sprintf( buf1, "%4s %-40s %-10s %-15s [%3s] [%3s] %-11s %-15s\r\n", row[0], row[1],
                     row[2], mini_c_time( atol( row[3] ), -1 ), row[4], row[5], row[6],
                     mini_c_time( atol( row[7] ), -1 ) );
            strcat( buf, buf1 );
        }
    }
    else if ( is_number( arg2 ) ) {
        if ( !str_cmp( arg1, "topic" ) ) {
            int                     topicid;

            topicid = atoi( arg2 );

            if ( IS_IMMORTAL( ch ) ) {
                sprintf( query,
                         "SELECT p.post_id, p.post_subject, p.post_time, u.username "
                         "FROM forums_posts p LEFT JOIN forums_users u ON p.poster_id = u.user_id "
                         "WHERE p.topic_id = %d " "ORDER BY p.post_time DESC", topicid );
            }
            else {
                sprintf( query,
                         "SELECT p.post_id, p.post_subject, p.post_time, u.username "
                         "FROM forums_posts p LEFT JOIN forums_users u ON p.poster_id = u.user_id LEFT JOIN forums_forums f ON p.forum_id = f.forum_id "
                         "WHERE p.topic_id = %d " "AND f.forum_id != 3 AND f.parent_id != 3 "
                         "ORDER BY p.post_time DESC", topicid );
            }
            // 0 - p.post_id
            // 1 - p.post_subject
            // 2 - p.post_time
            // 3 - u.username

            if ( mysql_query( db, query ) )
                return -2;

            res = mysql_use_result( db );
            free_result = TRUE;

            sprintf( buf1,
                     "&cPNum Post                                          Poster     Last Post Time\r\n" );
            strcat( buf, buf1 );
            sprintf( buf1,
                     "==== ============================================= ========== ==============&C\r\n" );
            strcat( buf, buf1 );

            while ( ( row = mysql_fetch_row( res ) ) != NULL ) {
                sprintf( buf1, "%4s %-45s %-11s %-15s\r\n", row[0], row[1], row[3],
                         mini_c_time( atol( row[2] ), -1 ) );
                strcat( buf, buf1 );
            }
        }
        else if ( !str_cmp( arg1, "write" ) ) {        /* meat function, create a post */
            send_to_char( "Under Construction\r\n", ch );
            return 0;
        }
/*
send_to_char("      : forum new <fnum> <topic name>' will create a new empty topic in forum <fnum>.\r\n", ch);
send_to_char("      : forum write <tnum> <post name>' will put you in a buffer to write a post in topic <tnum>.\r\n", ch);
*/
        else if ( !str_cmp( arg1, "new" ) ) {          /* create a new empty topic */
            int                     forumid = atoi( arg2 );

            /*
             * 'argument' is going to be the topic name 
             */

            /*
             * STEPS - Volk
             */
/*1. Work out if forumid is a forum they have PERMISSION to post in */
/*2. If no, exit. If yes, get the NEXT VALID UNUSED TOPIC NUMBER. */
/*3. Add into database with found topic number (topic_id) and argument as the topic name (topic_title). */
/* topic_first_poster_name will be their registered forums name and password */
/* topic_views = 0, topic_replies = 0 */

            // 0 - t.topic_id
            // 1 - t.topic_title
            // 2 - t.topic_first_poster_name
            // 3 - t.topic_time
            // 4 - t.topic_replies
            // 5 - t.topic_views
            // 6 - t.topic_last_poster_name
            // 7 - t.topic_last_post_time

        }

        else if ( !str_cmp( arg1, "post" ) ) {
            int                     postid;

            postid = atoi( arg2 );

            if ( IS_IMMORTAL( ch ) ) {
                sprintf( query,
                         "SELECT p.post_id, p.post_subject, p.post_text, p.post_time, u.username "
                         "FROM forums_posts p LEFT JOIN forums_users u ON p.poster_id = u.user_id "
                         "WHERE p.post_id = %d", postid );
            }
            else {
                sprintf( query,
                         "SELECT p.post_id, p.post_subject, p.post_text, p.post_time, u.username "
                         "FROM forums_posts p LEFT JOIN forums_users u ON p.poster_id = u.user_id LEFT JOIN forums_forums f ON p.forum_id = f.forum_id "
                         "WHERE p.post_id = %d " "AND f.forum_id != 3 AND f.parent_id != 3",
                         postid );
            }
            // 0 - p.post_id
            // 1 - p.post_subject
            // 2 - p.post_text
            // 3 - p.post_time
            // 4 - u.username

            if ( mysql_query( db, query ) )
                return -2;

            res = mysql_use_result( db );
            free_result = TRUE;

            while ( ( row = mysql_fetch_row( res ) ) != NULL ) {
                sprintf( buf1, "ID: %s\r\n", row[0] );
                strcat( buf, buf1 );
                sprintf( buf1, "From: %s\r\n", row[4] );
                strcat( buf, buf1 );
                sprintf( buf1, "Subject: %s\r\n", row[1] );
                strcat( buf, buf1 );
                sprintf( buf1, "Time: %s\r\n", mini_c_time( atol( row[3] ), -1 ) );
                strcat( buf, buf1 );
                sprintf( buf1, "Text:\r\n%s\r\n", row[2] );
                strcat( buf, buf1 );
            }
        }
        else {
            strcat( buf, "Invalid command!\r\n" );
        }
    }
    else {
        strcat( buf, "Invalid command!\r\n" );
    }

    send_to_char( buf, ch );

    send_to_char( "\r\nSyntax: 'forums <fnum>' will list all the topics for that forum.\r\n", ch );
    send_to_char( "        'forums topic <tnum>' will list all the posts for that topic.\r\n", ch );
    send_to_char( "        'forums post <pnum>' will display the post.\r\n\r\n", ch );
    send_to_char
        ( "        'forums new <fnum> <topic name>' will create a new empty topic in forum <fnum>.\r\n",
          ch );
    send_to_char
        ( "        'forums write <tnum> <post name>' will put you in a buffer to write a post in topic <tnum>.\r\n\r\n",
          ch );
    send_to_char
        ( "        'forums register <username> <password>' to change your forums profile.\r\n",
          ch );
    /*
     * Release memory used to store results and close connection 
     */
    if ( free_result == TRUE )
        mysql_free_result( res );

    return 0;
}

void do_forums( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    int                     errorcode;

    errorcode = mysql_forums( ch, argument );

    switch ( errorcode ) {
        case -1:
            send_to_char( "Failed to connect to the database.\r\n", ch );
            return;
        case -2:
            send_to_char( "Failed to query.\r\n", ch );
            return;
        case 0:
            return;
        default:
            send_to_char( "Unknown error.\r\n", ch );
            return;
    }
}

void do_sqlhelp( CHAR_DATA *ch, char *argument )
{
    MYSQL_RES              *result = NULL;
    MYSQL_ROW               row = NULL;
    char                    query[MAX_STRING_LENGTH * 100];
    char                    buf[MAX_STRING_LENGTH * 100];
    char                    arg1[MAX_INPUT_LENGTH];

    std::string s_query;
    std::string s_helplevel;
    std::string s_helpkeyword;
    std::string s_helptext;
    std::string s_helpmodifiedtime;

    if ( !db_connect )
        connect_mysql(  );

    if ( db == NULL )
        return;

    set_pager_color( AT_NOTE, ch );

    if ( argument[0] == '\0' ) {
        send_to_char( "Syntax: sqlhelp dumpsql - to save all helps to sql\r\n"
                      "        sqlhelp <keyword> - to query for keyword\r\n", ch );
        return;
    }
    else if ( !str_cmp( argument, "dumpsql" ) ) {
        char                   *modified_time;
        char                    buf2[MAX_INPUT_LENGTH];
        HELP_DATA              *pHelp;

        send_to_char( "Truncating help sql table.\r\n", ch );

        s_query = "TRUNCATE TABLE mud_helpfiles;";
        sprintf( query, "TRUNCATE TABLE mud_helpfiles;" );

        if ( mysql_query( db, s_query.c_str(  ) ) ) {
            ch_printf( ch,
                       "Encountered error when querying.\r\n" "mySQL error: %u\r\n" "Error: %s\r\n",
                       mysql_errno( db ), mysql_error( db ) );
        }

        send_to_char( "Dumping all helps to help sql table.\r\n", ch );

        for ( pHelp = first_help; pHelp; pHelp = pHelp->next ) {
//            time2mysql( pHelp->modified_time, modified_time );
//            s_helpkeyword = pHelp->keyword;
//            sprintf( buf2, "%d", pHelp->level );
//            s_helplevel = buf2;
//            s_helptext = pHelp->text;
//            s_helpmodifiedtime = modified_time;

            // std::replace(s_helpkeyword.begin(), s_helpkeyword.end(), '\'', '\"');
/*
            std::string single_quote = "\"";
            std::string two_single_quotes = "\"\"";
            int                     i;

            i = 0;
            while ( std::string::npos != ( i = s_helpkeyword.find_first_of( single_quote, i ) ) )
            {
                s_helpkeyword.replace( i, i + 1, two_single_quotes );
                i += 2;
            }

            i = 0;
            while ( std::string::npos != ( i = s_helptext.find_first_of( single_quote, i ) ) )
            {
                s_helptext.replace( i, i + 1, two_single_quotes );
                i += 2;
            }
*/
/*
            s_query = "INSERT INTO mud_helpfiles "
                "    (Level, Keyword, Text, ModifiedTime) "
                "VALUES(" + s_helplevel + ", '" + s_helpkeyword + "', '" + s_helptext + "', '" + s_helpmodifiedtime + "');";
            if ( mysql_query( db, s_query.c_str( ) ) )
            {
                ch_printf( ch, "Encountered error when querying.\r\n" "mySQL error: %u\r\n" "Error: %s\r\n", mysql_errno( db ), mysql_error( db ) );
            }
*/
        }

        send_to_char( "Done.\r\n", ch );
        return;
    }

    sprintf( query,
             "SELECT level, keyword, text FROM mud_helpfiles WHERE level <= %d AND keyword LIKE '%%%%%s%%%%';",
             ch->level, argument );
    ch_printf( ch, "%s\r\n", query );
    if ( ( mysql_safe_query( query ) ) != 0 ) {
        ch_printf( ch, "Encountered error when querying.\r\n" "mySQL error: %u\r\n" "Error: %s\r\n",
                   mysql_errno( db ), mysql_error( db ) );
        return;
    }

    // Store the helpfile name
    *buf = 0;
    if ( ( result = mysql_store_result( db ) ) != 0 ) {
        if ( ( row = mysql_fetch_row( result ) ) ) {
            sprintf( buf, "%s", row[2] );
        }
        mysql_free_result( result );
        send_to_char( buf, ch );
    }
    else {
        ch_printf( ch, "Encountered error when querying.\r\n" "mySQL error: %u\r\n" "Error: %s\r\n",
                   mysql_errno( db ), mysql_error( db ) );
        return;
    }
}
