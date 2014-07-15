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
 *                          Dynamic Channel System                          *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef REGEX
#include <regex.h>
#endif

#ifdef FREEBSD
#include <unistd.h>
#include <regex.h>
#endif
#include "h/key.h"
#include "h/mud.h"
#include "h/files.h"
#include "h/channels.h"
#include "h/languages.h"
#include "h/clans.h"
#include "h/city.h"

#ifdef REGEX
extern int re_exec      _RE_ARGS( ( const char * ) );
#endif

MUD_CHANNEL            *first_channel;
MUD_CHANNEL            *last_channel;

const char             *const chan_types[] = {
    "Global", "Zone", "Alliance", "Council", "PK", "Log", "Room", "Secret", "Throng",
    "Halcyon", "Paleon", "Dakar", "Forbidden"
};

int get_chantypes( char *name )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( chan_types ) / sizeof( chan_types[0] ) ); x++ )
        if ( !str_cmp( chan_types[x], name ) )
            return x;
    return -1;
}

/* Provided by Remcon to stop crashes with channel history */
char                   *add_percent( char *str )
{
    static char             newstr[MSL];
    int                     i,
                            j;

    for ( i = j = 0; str[i] != '\0'; i++ ) {
        if ( str[i] == '%' )
            newstr[j++] = '%';
        newstr[j++] = str[i];
    }
    newstr[j] = '\0';
    return newstr;
}

void read_channel( MUD_CHANNEL * channel, FILE * fp )
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
            case 'C':
                KEY( "ChanName", channel->name, fread_string( fp ) );
                KEY( "ChanLevel", channel->level, fread_number( fp ) );
                KEY( "ChanType", channel->type, fread_number( fp ) );
                KEY( "ChanHistory", channel->keephistory, fread_number( fp ) );
                break;
            case 'D':
                KEY( "DoScramble", channel->doscramble, fread_number( fp ) );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return;
                break;
        }
        if ( !fMatch ) {
            bug( "read_channel: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

void load_mudchannels( void )
{
    FILE                   *fp;
    MUD_CHANNEL            *channel;

    first_channel = NULL;
    last_channel = NULL;

    log_string( "Loading mud channels..." );

    if ( ( fp = FileOpen( CHANNEL_FILE, "r" ) ) == NULL ) {
        log_string( "No channel file found." );
        return;
    }

    for ( ;; ) {
        char                    letter;
        char                   *word;

        letter = fread_letter( fp );
        if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }

        if ( letter != '#' ) {
            bug( "%s", "load_channels: # not found." );
            break;
        }

        word = fread_word( fp );
        if ( !str_cmp( word, "CHANNEL" ) ) {
            CREATE( channel, MUD_CHANNEL, 1 );
            read_channel( channel, fp );

            LINK( channel, first_channel, last_channel, next, prev );
            continue;
        }
        else if ( !str_cmp( word, "END" ) )
            break;
        else {
            bug( "load_channels: bad section: %s.", word );
            continue;
        }
    }
    FileClose( fp );
    return;
}

void save_mudchannels( void )
{
    FILE                   *fp;
    MUD_CHANNEL            *channel;

    if ( ( fp = FileOpen( CHANNEL_FILE, "w" ) ) == NULL ) {
        log_string( "Couldn't write to channel file." );
        return;
    }

    for ( channel = first_channel; channel; channel = channel->next ) {
        if ( channel->name ) {
            fprintf( fp, "#CHANNEL\n" );
            fprintf( fp, "ChanName    %s~\n", channel->name );
            fprintf( fp, "ChanLevel   %d\n", channel->level );
            fprintf( fp, "ChanType    %d\n", channel->type );
            fprintf( fp, "ChanHistory %d\n", channel->keephistory );
            fprintf( fp, "DoScramble  %d\n", channel->doscramble );
            fprintf( fp, "End\n\n" );
        }
    }
    fprintf( fp, "#END\n" );
    FileClose( fp );
}

MUD_CHANNEL            *find_channel( const char *name )
{
    MUD_CHANNEL            *channel = NULL;

    for ( channel = first_channel; channel; channel = channel->next ) {

        if ( !str_cmp( channel->name, name ) )
            return channel;
    }
    return NULL;
}

void do_makechannel( CHAR_DATA *ch, char *argument )
{
    MUD_CHANNEL            *channel;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "&GSyntax: makechannel <name>\r\n", ch );
        return;
    }
    if ( ( channel = find_channel( argument ) ) ) {
        send_to_char( "&RA channel with that name already exists.\r\n", ch );
        return;
    }
    CREATE( channel, MUD_CHANNEL, 1 );
    channel->name = STRALLOC( argument );
    channel->level = LEVEL_IMMORTAL;
    channel->type = CHAN_GLOBAL;
    channel->keephistory = FALSE;
    channel->doscramble = FALSE;
    LINK( channel, first_channel, last_channel, next, prev );
    ch_printf( ch, "&YNew channel &G%s &Ycreated.\r\n", argument );
    save_mudchannels(  );
    return;
}

void do_setchannel( CHAR_DATA *ch, char *argument )
{
    MUD_CHANNEL            *channel;
    char                    arg[MIL],
                            arg2[MIL],
                            arg3[MIL];

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "&GSyntax: setchannel <channel> <field> <value>\r\n\r\n", ch );
        send_to_char( "&YField may be one of the following:\r\n", ch );
        send_to_char( "name level type history invite\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg );
    if ( !( channel = find_channel( arg ) ) ) {
        send_to_char( "&RNo channel by that name exists.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg2 );
    if ( !arg || arg2[0] == '\0' ) {
        do_setchannel( ch, ( char * ) "" );
        return;
    }
    if ( !str_cmp( arg2, "name" ) ) {
        ch_printf( ch, "&YChannel &G%s &Yrenamed to &G%s\r\n", channel->name, argument );
        STRFREE( channel->name );
        channel->name = STRALLOC( argument );
        save_mudchannels(  );
        return;
    }
    if ( !str_cmp( arg2, "level" ) ) {
        int                     level;

        if ( !is_number( argument ) ) {
            send_to_char( "&RLevel must be numerical.\r\n", ch );
            return;
        }
        level = atoi( argument );
        if ( level < 1 || level > MAX_LEVEL ) {
            ch_printf( ch, "&RInvalid level. Acceptable range is 1 to %d.\r\n", MAX_LEVEL );
            return;
        }
        channel->level = level;
        ch_printf( ch, "&YChannel &G%s &Ylevel changed to &G%d\r\n", channel->name, level );
        save_mudchannels(  );
        return;
    }

    // working in the 6d channel for selected private users. -Taon
    argument = one_argument( argument, arg3 );

    if ( !str_cmp( arg2, "invite" ) ) {
        CHAR_DATA              *victim;

        if ( !is_number( arg3 ) && ( victim = get_char_world( ch, arg3 ) ) ) {
            if ( str_cmp( ch->name, "Vladaar" ) ) {
                send_to_char( "Only the game admin can invite others to this channel.\r\n", ch );
                return;
            }
        }

        if ( arg3[0] == '\0' ) {
            send_to_char( "You must select a target.\r\n", ch );
            return;
        }
        if ( ( victim = get_char_world( ch, arg3 ) ) == NULL ) {
            send_to_char( "That player doesn't exist.\r\n", ch );
            return;
        }

        if ( victim->chan_invite != 1 )
            victim->chan_invite = 1;
        else                                           // toggle off. -Taon
        {
            victim->chan_invite = 0;
            ch_printf( victim, "&wYou've been removed from &Y%s&w channel.&d\r\n", channel->name );
            ch_printf( ch, "&wYou removed %s from the &Y%s&w channel.&d\r\n", victim->name,
                       channel->name );
            return;
        }
        ch_printf( victim,
                   "&wYou've been invited to join the &Y%s&w channel.  Usually, only the Game Admin, and Heads of Councils may use this channel.&d\r\n",
                   channel->name );
        ch_printf( ch, "&wYou invited %s to join the &Y%s&w channel.&d\r\n", victim->name,
                   channel->name );
        interpret( victim, ( char * ) "listen 6d" );
        return;
    }
    if ( !str_cmp( arg2, "type" ) ) {
        int                     type = get_chantypes( arg3 );

        if ( type == -1 ) {
            send_to_char( "&RInvalid channel type.\r\n", ch );
            return;
        }
        channel->type = type;
        send_to_char( "New type set.\r\n", ch );
        save_mudchannels(  );
        return;
    }
    if ( !str_cmp( arg2, "history" ) ) {
        channel->keephistory = !channel->keephistory;

        if ( channel->keephistory )
            ch_printf( ch, "&YChannel &G%s &Ywill now keep a history.\r\n", channel->name );
        else
            ch_printf( ch, "&YChannel &G%s &Ywill no longer keep a history.\r\n", channel->name );
        save_mudchannels(  );
        return;
    }
    if ( !str_cmp( arg2, "scramble" ) ) {
        channel->doscramble = !channel->doscramble;

        if ( channel->doscramble )
            ch_printf( ch, "&YChannel &G%s &Ywill now scramble the text by languages.\r\n",
                       channel->name );
        else
            ch_printf( ch, "&YChannel &G%s &Ywill no longer scramble the text.\r\n",
                       channel->name );
        save_mudchannels(  );
        return;
    }
    do_setchannel( ch, ( char * ) "" );
}

void do_destroychannel( CHAR_DATA *ch, char *argument )
{
    MUD_CHANNEL            *channel;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "&GSyntax: destroychannel <name>\r\n", ch );
        return;
    }
    if ( !( channel = find_channel( argument ) ) ) {
        send_to_char( "&RNo channel with that name exists.\r\n", ch );
        return;
    }
    STRFREE( channel->name );
    UNLINK( channel, first_channel, last_channel, next, prev );
    DISPOSE( channel );
    ch_printf( ch, "&YChannel &G%s &Ydestroyed.\r\n", argument );
    save_mudchannels(  );
    return;
}

void do_showchannels( CHAR_DATA *ch, char *argument )
{
    MUD_CHANNEL            *channel;

    send_to_char( "&WName               &YLevel &BType       &GHistory?   &GScramble?\r\n", ch );
    send_to_char( "&W--------------------------------------------------------\r\n", ch );
    for ( channel = first_channel; channel; channel = channel->next )
        ch_printf( ch, "&W%-18s &Y%-4d  &B%-10s &G%8s   &G%9s\r\n", capitalize( channel->name ),
                   channel->level, chan_types[channel->type], channel->keephistory ? "Yes" : "No",
                   channel->doscramble ? "Yes" : "No" );
}

/* Stuff borrowed from I3/MUD-Net code to handle channel listening */

/*  changetarg: extract a single argument (with given max length) from
 *  argument to arg; if arg==NULL, just skip an arg, don't copy it out
 */
const char             *getarg( const char *argument, char *arg, int length )
{
    int                     len = 0;

    while ( *argument && isspace( *argument ) )
        argument++;
    if ( arg )
        while ( *argument && !isspace( *argument ) && len < length - 1 )
            *arg++ = *argument++, len++;
    else
        while ( *argument && !isspace( *argument ) )
            argument++;
    while ( *argument && !isspace( *argument ) )
        argument++;
    while ( *argument && isspace( *argument ) )
        argument++;
    if ( arg )
        *arg = 0;
    return argument;
}

/* Check for a name in a list */
int hasname( const char *list, const char *name )
{
    const char             *p;
    char                    arg[MIL];

    if ( !list )
        return ( 0 );
    p = getarg( list, arg, MIL );
    while ( arg[0] ) {
        if ( !strcasecmp( name, arg ) )
            return 1;
        p = getarg( p, arg, MIL );
    }
    return 0;
}

/* Add a name to a list */
void addname( char **list, const char *name )
{
    char                    buf[MSL];

    if ( hasname( *list, name ) )
        return;
    if ( *list && *list[0] != '\0' )
        snprintf( buf, MSL, "%s %s", *list, name );
    else
        mudstrlcpy( buf, name, MSL );
    if ( *list )
        STRFREE( *list );
    *list = STRALLOC( buf );
}

/* Remove a name from a list */
void removename( char **list, const char *name )
{
    char                    buf[MSL];
    char                    arg[MIL];
    const char             *p;

    buf[0] = 0;
    p = getarg( *list, arg, MIL );
    while ( arg[0] ) {
        if ( strcasecmp( arg, name ) ) {
            if ( buf[0] )
                mudstrlcat( buf, " ", MSL );
            mudstrlcat( buf, arg, MSL );
        }
        p = getarg( p, arg, MIL );
    }
    STRFREE( *list );
    *list = STRALLOC( buf );
}

void do_listen( CHAR_DATA *ch, char *argument )
{
    MUD_CHANNEL            *channel;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobs can't use this command.\r\n", ch );
        return;
    }
    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "&GSyntax: listen \r\n", ch );
        send_to_char( "&GSyntax: listen all\r\n", ch );
        send_to_char( "&GSyntax: listen none\r\n", ch );
        send_to_char( "&GFor a list of channels, type &Wchannels\r\n", ch );
        send_to_char( "&YYou are listening to the following local mud channels:\r\n\r\n", ch );
        ch_printf( ch, "&W%s\r\n", ch->pcdata->chan_listen );
        return;
    }
    if ( !str_cmp( argument, "all" ) ) {
        for ( channel = first_channel; channel; channel = channel->next ) {
            if ( ( ( ch->level >= channel->level ) || ( ch->trust >= channel->level ) )
                 && !hasname( ch->pcdata->chan_listen, channel->name ) )
                addname( &ch->pcdata->chan_listen, channel->name );
        }
        send_to_char( "&YYou are now listening to all available channels.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "none" ) ) {
        for ( channel = first_channel; channel; channel = channel->next ) {
            if ( ( ( ch->level >= channel->level ) || ( ch->trust >= channel->level ) )
                 && hasname( ch->pcdata->chan_listen, channel->name ) )
                removename( &ch->pcdata->chan_listen, channel->name );
        }
        channel = find_channel( "say" );
        addname( &ch->pcdata->chan_listen, channel->name );
        /*
         * Volk - still should listen to the SAY channel 
         */
        send_to_char( "&YYou no longer listen to any available channel.\r\n", ch );
        return;
    }
    if ( hasname( ch->pcdata->chan_listen, argument ) ) {
        if ( !str_cmp( argument, "say" ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "Mortals can not turn off the SAY channel.\r\n", ch );
            return;
        }
        removename( &ch->pcdata->chan_listen, argument );
        ch_printf( ch, "&YYou no longer listen to &W%s\r\n", argument );
    }
    else {
        if ( !( channel = find_channel( argument ) ) ) {
            send_to_char( "No such channel.\r\n", ch );
            return;
        }
        if ( ( channel->level > ch->level ) && ( channel->level > ch->trust ) ) {
            send_to_char( "That channel is above your level.\r\n", ch );
            return;
        }
        addname( &ch->pcdata->chan_listen, argument );
        ch_printf( ch, "&YYou now listen to &W%s\r\n", channel->name );
    }
    return;
}

/* Revised channel display by Zarius */
void do_channels( CHAR_DATA *ch, char *argument )
{
    MUD_CHANNEL            *channel;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobs can't use this command.\r\n", ch );
        return;
    }
    send_to_char( "&YThe following channels are available:\r\n", ch );
    send_to_char( "To toggle a channel, use the &Wlisten &Ycommand.\r\n\r\n", ch );
    send_to_char( "&WChannel        On/Off&D\r\n", ch );
    send_to_char( "&B-----------------------&D\r\n", ch );
    for ( channel = first_channel; channel; channel = channel->next ) {
        if ( ( ch->level >= channel->level ) || ( ch->trust >= channel->level ) ) {
            ch_printf( ch, "&w%-17s%s&D\r\n", capitalize( channel->name ),
                       ( hasname( ch->pcdata->chan_listen, channel->name ) ) ? "&GOn" : "&ROff" );
        }
    }
    send_to_char( "\r\n", ch );
}

void invert( char *arg1, char *arg2 )
{
    int                     i = 0;
    int                     len = strlen( arg1 ) - 1;

    while ( i <= len ) {
        *( arg2 + i ) = *( arg1 + ( len - i ) );
        i++;
    }
    *( arg2 + i ) = '\0';
}

/* Duplicate of to_channel from act_comm.c modified for dynamic channels */
void send_tochannel( CHAR_DATA *ch, MUD_CHANNEL * channel, char *argument )
{
    char                    buf[MSL],
                            buf2[MSL],
                            col[MIL],
                            word[MIL];
    char                    logbuf[MSL];
    char                   *arg,
                           *socbuf_char = NULL,
        *socbuf_vict = NULL,
        *socbuf_other = NULL;
    CHAR_DATA              *victim = NULL;
    CHAR_DATA              *vch;
    SOCIALTYPE             *social = NULL;
    int                     position,
                            x;
    short                   color;
    struct tm              *local;
    time_t                  t;
    bool                    emote = FALSE;

    if ( !ch )
        return;
#ifndef SCRAMBLE
    int                     speaking = -1,
        lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
        if ( ch->speaking & lang_array[lang] ) {
            speaking = lang;
            break;
        }
#endif

    if ( ch->chan_invite != 1 && channel->type == CHAN_SECRET ) {
        send_to_char( "You're not invited to talk on this channel.\r\n", ch );
        return;
    }

    if ( channel->type == CHAN_ROOM && ch->position == POS_SLEEPING && !IS_IMMORTAL( ch ) ) {
        send_to_char( "Your sleeping, and cannot do that right now.\r\n", ch );
        return;
    }

    if ( !str_cmp( channel->name, "shout" ) && ch->position == POS_SLEEPING && !IS_IMMORTAL( ch ) ) {
        send_to_char( "Your sleeping, and cannot do that right now.\r\n", ch );
        return;
    }

    if ( !IS_CLANNED( ch ) && !IS_IMMORTAL( ch ) ) {
        if ( !str_cmp( channel->name, "alliance" ) || !str_cmp( channel->name, "throng" )
             || !str_cmp( channel->name, "halcyon" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
    }

    if ( !IS_CITY( ch ) && !IS_IMMORTAL( ch ) ) {
        if ( !str_cmp( channel->name, "halcyon" ) || !str_cmp( channel->name, "paleon" )
             || !str_cmp( channel->name, "forbidden" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
    }

    if ( IS_CLANNED( ch ) && !IS_IMMORTAL( ch ) ) {
        if ( str_cmp( ch->pcdata->clan_name, "throng" ) && !str_cmp( channel->name, "throng" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
        if ( str_cmp( ch->pcdata->clan_name, "halcyon" ) && !str_cmp( channel->name, "halcyon" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
        if ( str_cmp( ch->pcdata->clan_name, "alliance" ) && !str_cmp( channel->name, "alliance" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
    }

    if ( IS_CITY( ch ) && !IS_IMMORTAL( ch ) ) {
        if ( str_cmp( ch->pcdata->city_name, "paleon city" )
             && !str_cmp( channel->name, "paleon" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
        if ( str_cmp( ch->pcdata->city_name, "dakar city" ) && !str_cmp( channel->name, "dakar" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
        if ( str_cmp( ch->pcdata->city_name, "forbidden city" )
             && !str_cmp( channel->name, "forbidden" ) ) {
            send_to_char( "Huh?\r\n", ch );
            return;
        }
    }

    if ( ch->in_room && IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) && !IS_IMMORTAL( ch ) ) {
        if ( !IS_NPC( ch ) )
            send_to_char( "The room absorbs your words!\r\n", ch );
        return;
    }

    if ( ch->position == POS_MEDITATING && strcmp( channel->name, "ooc" ) ) {
        send_to_char( "You are concentrating too much for that!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        if ( ch->master )
            send_to_char( "I don't think so...\r\n", ch->master );
        return;
    }

    if ( !str_cmp( argument, "8ball" ) && !strcmp( channel->name, "ooc" ) ) {
        CHAR_DATA              *mob;
        short                   chance;

        chance = number_range( 1, 6 );

        if ( ( mob = get_char_world( ch, ( char * ) "magic 8 ball" ) ) == NULL )
            return;
        interpret( ch, ( char * ) "ooc Oh magic 8 ball we seek an answer." );
        if ( chance == 1 )
            snprintf( buf, MSL, "ooc %s It is doubtful.", ch->name );
        else if ( chance == 2 )
            snprintf( buf, MSL, "ooc %s Concentrate and ask again.", ch->name );
        else if ( chance == 3 )
            snprintf( buf, MSL, "ooc %s Outlook not good.", ch->name );
        else if ( chance == 4 )
            snprintf( buf, MSL, "ooc %s Reply hazy, try again.", ch->name );
        else if ( chance == 5 )
            snprintf( buf, MSL, "ooc %s Outlook good.", ch->name );
        else
            snprintf( buf, MSL, "ooc %s Signs point to yes.", ch->name );
        interpret( mob, buf );
        return;
    }

    if ( !VLD_STR( argument ) ) {
        const char             *name;

        if ( ch->desc->original ) {
            send_to_char( "You cannot be switched to check the history.\r\n", ch );
            return;
        }

        if ( !channel->keephistory ) {
            ch_printf( ch, "%s what?\r\n", capitalize( channel->name ) );
            return;
        }
        ch_printf( ch, "&cThe last 20 %s messages:\r\n", channel->name );
        if ( !str_cmp( channel->name, "say" ) ) {
            for ( x = 0; x < 20; x++ ) {
                if ( ch->pcdata->say_history[x] == NULL )
                    break;
                ch_printf( ch, "%s &D\r\n", ch->pcdata->say_history[x] );
            }
            return;
        }
        else {
            for ( x = 0; x < 20; x++ ) {
                if ( channel->history[x][0] != NULL ) {
                    switch ( channel->hlevel[x] ) {
                        case 0:
                            name = channel->history[x][0];
                            break;
                        case 1:
                            if ( IS_AFFECTED( ch, AFF_DETECT_INVIS ) || IS_IMMORTAL( ch ) )
                                name = channel->history[x][0];
                            else
                                name = "Someone";
                            break;
                        case 2:
                            if ( ch->level >= channel->hinvis[x]
                                 || ch->trust >= channel->hinvis[x] )
                                name = channel->history[x][0];
                            else
                                name = "Someone";
                            break;
                        default:
                            name = "Someone";
                    }
                    ch_printf( ch, channel->history[x][1], name );
                }
                else
                    break;
            }
            return;
        }
    }
    if ( xIS_SET( ch->act, PLR_SILENCE ) ) {
        ch_printf( ch, "You can't %s.\r\n", channel->name );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_BURROW ) && strcmp( channel->name, "ooc" ) ) {
        send_to_char( "You cannot speak while burrowed, but you hear things telepathically.\r\n",
                      ch );
        return;
    }

    /*
     * OK, this is hackish - until I can figure out a better method 
     */
    color = -1;
    if ( !str_cmp( channel->name, "chat" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "ooc" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "alliance" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "throng" ) )
        color = AT_RED;
    if ( !str_cmp( channel->name, "halcyon" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "dakar" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "paleon" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "forbidden" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "osay" ) )
        color = AT_CYAN;
    if ( !str_cmp( channel->name, "say" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "icc" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "avtalk" ) )
        color = AT_YELLOW;
    if ( !str_cmp( channel->name, "sooc" ) )
        color = AT_PINK;
    if ( !str_cmp( channel->name, "6d" ) )
        color = AT_YELLOW;
    if ( !str_cmp( channel->name, "yell" ) )
        color = AT_ORANGE;
    if ( !str_cmp( channel->name, "shout" ) )
        color = AT_DGREEN;
    if ( !str_cmp( channel->name, "muse" ) )
        color = AT_MUSE;
    if ( !str_cmp( channel->name, "think" ) )
        color = AT_THINK;
    if ( !str_cmp( channel->name, "auction" ) )
        color = AT_AUCTION;
    if ( !str_cmp( channel->name, "wartalk" ) )
        color = AT_WARTALK;
    if ( !str_cmp( channel->name, "hint" ) )
        color = AT_LBLUE;
    if ( !str_cmp( channel->name, "quest" ) )
        color = AT_CYAN;

    if ( color == -1 )
        color = AT_GOSSIP;

    snprintf( col, MIL, "%s", color_str( color, ch ) );

    arg = argument;
    arg = one_argument( arg, word );

    if ( word[0] == '@' && ( social = find_social( word + 1 ) ) != NULL ) {
        if ( arg && *arg ) {
            char                    name[MIL];

            one_argument( arg, name );

            if ( ( victim = get_char_world( ch, name ) ) )
                arg = one_argument( arg, name );

            if ( !victim ) {
                socbuf_char = social->char_no_arg;
                socbuf_vict = social->others_no_arg;
                socbuf_other = social->others_no_arg;
                if ( !socbuf_char && !socbuf_other )
                    social = NULL;
            }
            else if ( victim == ch ) {
                socbuf_char = social->char_auto;
                socbuf_vict = social->others_auto;
                socbuf_other = social->others_auto;
                if ( !socbuf_char && !socbuf_other )
                    social = NULL;
            }
            else if ( victim != ch ) {
                socbuf_char = social->char_found;
                socbuf_vict = social->vict_found;
                socbuf_other = social->others_found;
                if ( !socbuf_char && !socbuf_other && !socbuf_vict )
                    social = NULL;
            }
            else
                social = NULL;
        }
        else {
            socbuf_char = social->char_no_arg;
            socbuf_vict = social->others_no_arg;
            socbuf_other = social->others_no_arg;
            if ( !socbuf_char && !socbuf_other )
                social = NULL;
        }
    }

    if ( word[0] == ',' )
        emote = TRUE;

    if ( ( !str_cmp( channel->name, "say" ) ) || ( !str_cmp( channel->name, "osay" ) ) ) {
        ch_printf( ch, "%sYou %s '%s'\r\n", col, channel->name, argument );
    }
    else {
        if ( social ) {
            act_printf( AT_PLAIN, ch, argument, victim, TO_CHAR, " &W[%s%s&W] %s%s", col,
                        capitalize( channel->name ), col, socbuf_char );
        }
        else if ( emote ) {
            argument = argument + 1;
            ch_printf( ch, " &W[%s%s&W] %s%s %s\r\n", col, capitalize( channel->name ), col,
                       ch->name, argument );
        }
        else if ( !str_cmp( channel->name, "hint" ) ) {
            ch_printf( ch, "%s %s&c '%s'\r\n", col, channel->name, argument );
        }
        else
            ch_printf( ch, "%s [%s] '%s'\r\n", col, capitalize( channel->name ), argument );
    }

    if ( !str_cmp( channel->name, "sooc" ) ) {
        snprintf( buf2, MSL, "&P%s: %s (%s)&D", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( CSAVE_FILE, buf2 );
    }
    if ( !str_cmp( channel->name, "muse" ) ) {
        snprintf( buf2, MSL, "&g%s: %s (%s)&D", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( CSAVE_FILE, buf2 );
    }
    if ( !str_cmp( channel->name, "ooc" ) ) {
        snprintf( buf2, MSL, "&C%s: %s (%s)&D", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( CSAVE_FILE, buf2 );
    }
    if ( !str_cmp( channel->name, "chat" ) ) {
        snprintf( buf2, MSL, "&C%s: %s (%s)&D", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( CSAVE_FILE, buf2 );
    }
    if ( !str_cmp( channel->name, "think" ) ) {
        snprintf( buf2, MSL, "&R%s: %s (%s)&D", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( CSAVE_FILE, buf2 );
    }
    if ( !str_cmp( channel->name, "6d" ) ) {
        snprintf( buf2, MSL, "&Y%s: %s (%s)&D", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( CSAVE_FILE, buf2 );
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) ) {
        snprintf( buf2, MSL, "%s: %s (%s)", IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  channel->name );
        append_to_file( LOG_FILE, buf2 );
    }

    /*
     * Channel history. Records the last 20 messages to channels which keep historys 
     */
    if ( channel->keephistory ) {
        for ( x = 0; x < 20; x++ ) {
            int                     type;

            type = 0;
            if ( IS_AFFECTED( ch, AFF_INVISIBLE ) )
                type = 1;
            if ( ch->pcdata && xIS_SET( ch->act, PLR_WIZINVIS ) )
                type = 2;
            if ( !ch->pcdata && xIS_SET( ch->act, ACT_MOBINVIS ) )
                type = 3;

            if ( channel->history[x][0] == NULL ) {
                if ( VLD_STR( channel->history[x][0] ) )
                    STRFREE( channel->history[x][0] );
                if ( VLD_STR( channel->history[x][1] ) )
                    STRFREE( channel->history[x][1] );
                if ( IS_NPC( ch ) )
                    channel->history[x][0] = STRALLOC( ch->short_descr );
                else
                    channel->history[x][0] = STRALLOC( ch->name );

                t = time( NULL );
                local = localtime( &t );
                argument = add_percent( argument );
                snprintf( logbuf, MSL, "   &R[%-2.2d:%-2.2d] &G%%s%s %s\r\n", local->tm_hour,
                          local->tm_min, emote ? "" : ":", argument );
                channel->history[x][1] = STRALLOC( logbuf );
                channel->hlevel[x] = type;
                channel->hinvis[x] = 0;
                if ( type == 3 )
                    channel->hinvis[x] = ch->mobinvis;
                else if ( type == 2 )
                    channel->hinvis[x] = ch->pcdata->wizinvis;
                break;
            }

            if ( x == 19 ) {
                int                     y;

                for ( y = 1; y < 20; y++ ) {
                    int                     z = y - 1;

                    if ( channel->history[z][0] != NULL ) {
                        STRFREE( channel->history[z][0] );
                        STRFREE( channel->history[z][1] );
                        channel->history[z][0] = STRALLOC( channel->history[y][0] );
                        channel->history[z][1] = STRALLOC( channel->history[y][1] );
                        channel->hlevel[z] = channel->hlevel[y];
                        channel->hinvis[z] = channel->hinvis[y];
                    }
                }
                if ( VLD_STR( channel->history[x][0] ) )
                    STRFREE( channel->history[x][0] );
                if ( VLD_STR( channel->history[x][1] ) )
                    STRFREE( channel->history[x][1] );
                if ( IS_NPC( ch ) )
                    channel->history[x][0] = STRALLOC( ch->short_descr );
                else
                    channel->history[x][0] = STRALLOC( ch->name );

                t = time( NULL );
                local = localtime( &t );
                argument = add_percent( argument );
                snprintf( logbuf, MSL, "   &R[%-2.2d:%-2.2d] &G%%s%s %s\r\n", local->tm_hour,
                          local->tm_min, emote ? "" : ":", argument );
                channel->history[x][1] = STRALLOC( logbuf );
                channel->hlevel[x] = type;
                channel->hinvis[x] = 0;
                if ( type == 3 )
                    channel->hinvis[x] = ch->mobinvis;
                else if ( type == 2 )
                    channel->hinvis[x] = ch->pcdata->wizinvis;
            }
        }
    }

    for ( vch = first_char; vch; vch = vch->next ) {
        if ( vch == ch )
            continue;

        /*
         * So puppets can redirect says and that 
         */
        if ( IS_PUPPET( vch ) ) {
            char                   *sbuf = argument;
            char                    lbuf[MIL + 4];

            position = vch->position;
            set_position( vch, POS_STANDING );

            if ( vch->level < channel->level && vch->trust < channel->level )
                continue;

            /*
             * Make it skip the ooc and chat for puppets 
             */
            if ( !str_cmp( channel->name, "ooc" ) || !str_cmp( channel->name, "chat" ) )
                continue;

            if ( IS_SET( vch->in_room->room_flags, ROOM_SILENCE ) )
                continue;

            if ( channel->type == CHAN_ZONE && vch->in_room->area != ch->in_room->area )
                continue;

            if ( channel->type == CHAN_ROOM && vch->in_room != ch->in_room )
                continue;

            if ( channel->type == CHAN_PK && !IS_PKILL( vch ) && !IS_IMMORTAL( vch ) )
                continue;

            if ( ( vch->position == POS_SLEEPING ) && channel->type == CHAN_ROOM
                 && !IS_IMMORTAL( vch ) )
                continue;

            if ( channel->type == CHAN_SECRET ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( vch->chan_invite != 1 )
                    continue;
            }

            if ( channel->type == CHAN_ALLIANCE ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->clan_name || str_cmp( vch->pcdata->clan_name, "alliance" ) )
                    continue;
            }

            if ( channel->type == CHAN_THRONG ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->clan_name || str_cmp( vch->pcdata->clan_name, "throng" ) )
                    continue;
            }

            if ( channel->type == CHAN_HALCYON ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->clan_name || str_cmp( vch->pcdata->clan_name, "halcyon" ) )
                    continue;
            }

            if ( channel->type == CHAN_PALEON ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->city_name || str_cmp( vch->pcdata->city_name, "paleon city" ) )
                    continue;
            }

            if ( channel->type == CHAN_DAKAR ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->city_name || str_cmp( vch->pcdata->city_name, "dakar city" ) )
                    continue;
            }

            if ( channel->type == CHAN_FORBIDDEN ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->city_name
                     || str_cmp( vch->pcdata->city_name, "forbidden city" ) )
                    continue;
            }

            snprintf( col, MIL, "%s", color_str( color, vch ) );
            if ( ( ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_MOBINVIS ) )
                   || ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_WIZINVIS ) ) ) && can_see( vch, ch )
                 && IS_IMMORTAL( vch ) )
                snprintf( lbuf, sizeof( lbuf ), "%s(%d) ", col,
                          ( !IS_NPC( ch ) ) ? ch->pcdata->wizinvis : ch->mobinvis );
            else
                lbuf[0] = '\0';

            if ( ( !str_cmp( channel->name, "say" ) ) || ( !str_cmp( channel->name, "osay" ) ) ) {
                if ( !social && !emote ) {
                    snprintf( buf, MSL, "$n %ss '$t'", channel->name );
                    act( AT_SAY, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
                }
            }
            else if ( !social && !emote ) {
                if ( !str_cmp( channel->name, "hint" ) ) {
                    snprintf( buf, MSL, "%s %s $t%s", col, channel->name, col );
                    act( AT_PLAIN, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
                }
                else
                    snprintf( buf, MSL, "%s [%s] $n '$t%s'", col, capitalize( channel->name ),
                              col );
                act( AT_PLAIN, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
            }
            if ( emote ) {
                snprintf( buf, MSL, " &W[%s%s&W] %s$n $t", col, capitalize( channel->name ), col );
                act( AT_PLAIN, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
            }
            if ( social ) {
                if ( vch == victim )
                    act_printf( AT_PLAIN, ch, NULL, vch, TO_VICT, " &W[%s%s&W] %s%s", col,
                                capitalize( channel->name ), col, socbuf_vict );
                else
                    act_printf( AT_PLAIN, ch, vch, victim, TO_THIRD, " &W[%s%s&W] %s%s", col,
                                capitalize( channel->name ), col, socbuf_other );
            }
            set_position( vch, position );
        }

        if ( !vch->desc )
            continue;

        if ( vch->desc->connected == CON_PLAYING
             && ( IS_NPC( vch ) || hasname( vch->pcdata->chan_listen, channel->name ) ) ) {
            char                   *sbuf = argument;
            char                    lbuf[MIL + 4];     /* invis level string + buf */

            if ( ( vch->level < channel->level ) && ( vch->trust < channel->level ) )
                continue;

            if ( IS_SET( vch->in_room->room_flags, ROOM_SILENCE ) )
                continue;

            if ( channel->type == CHAN_ZONE && vch->in_room->area != ch->in_room->area )
                continue;

            if ( channel->type == CHAN_ROOM && vch->in_room != ch->in_room )
                continue;

            if ( channel->type == CHAN_PK && !IS_PKILL( vch ) && !IS_IMMORTAL( vch ) )
                continue;

            if ( ( vch->position == POS_SLEEPING ) && channel->type == CHAN_ROOM
                 && !IS_IMMORTAL( vch ) ) {
                continue;
            }
            if ( channel->type == CHAN_SECRET ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( vch->chan_invite != 1 )
                    continue;
            }

            if ( channel->type == CHAN_ALLIANCE ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( str_cmp( vch->pcdata->clan_name, "alliance" ) )
                    continue;
            }

            if ( channel->type == CHAN_HALCYON ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( str_cmp( vch->pcdata->clan_name, "halcyon" ) )
                    continue;
            }

            if ( channel->type == CHAN_THRONG ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( str_cmp( vch->pcdata->clan_name, "throng" ) )
                    continue;
            }

            if ( channel->type == CHAN_COUNCIL ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( vch->pcdata->council != ch->pcdata->council )
                    continue;
            }

            if ( channel->type == CHAN_PALEON ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->city_name || str_cmp( vch->pcdata->city_name, "paleon city" ) )
                    continue;
            }

            if ( channel->type == CHAN_DAKAR ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->city_name || str_cmp( vch->pcdata->city_name, "dakar city" ) )
                    continue;
            }

            if ( channel->type == CHAN_FORBIDDEN ) {
                if ( IS_NPC( vch ) )
                    continue;
                if ( !vch->pcdata->city_name
                     || str_cmp( vch->pcdata->city_name, "forbidden city" ) )
                    continue;
            }

            position = vch->position;
            set_position( vch, POS_STANDING );

            snprintf( col, MIL, "%s", color_str( color, vch ) );
            if ( ( ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_MOBINVIS ) )
                   || ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_WIZINVIS ) ) ) && can_see( vch, ch )
                 && IS_IMMORTAL( vch ) )
                snprintf( lbuf, sizeof( lbuf ), "%s(%d) ", col,
                          ( !IS_NPC( ch ) ) ? ch->pcdata->wizinvis : ch->mobinvis );
            else
                lbuf[0] = '\0';

            /*
             * Should we scramble the channel? 
             */
            if ( channel->doscramble ) {
#ifndef SCRAMBLE
                if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) ) {
                    int                     speakswell =
                        UMIN( knows_language( vch, ch->speaking, ch ),
                              knows_language( ch, ch->speaking, vch ) );

                    if ( speakswell < 85 )
                        sbuf = translate( speakswell, argument, lang_names[speaking] );
                }
#else
                if ( !knows_language( vch, ch->speaking, ch )
                     && ( !IS_NPC( ch ) || ch->speaking != 0 ) )
                    sbuf = scramble( argument, ch->speaking );
#endif
            }

            /*
             * Check to see if target is ignoring the sender 
             */
            if ( is_ignoring( vch, ch ) ) {
                /*
                 * If the sender is an imm then they cannot be ignored 
                 */
                if ( !IS_IMMORTAL( ch ) || vch->level > ch->level || vch->trust > ch->level ) {
                    /*
                     * Off to oblivion! 
                     */
                    continue;
                }
                else
                    set_char_color( AT_IGNORE, vch );
            }

            MOBtrigger = FALSE;

            /*
             * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002
             * Volk looking into this crashing the mud over and over.. 
             */

            if ( ( !str_cmp( channel->name, "say" ) )  /* || (!str_cmp(channel->name, *
                                                        * "osay")) */  ) {
                if ( !social && !emote ) {
                    snprintf( buf, MSL, "$n %ss '$t'", channel->name );
                    act( AT_SAY, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
                }
                else {
                    act_printf( AT_PLAIN, ch, vch, victim, TO_THIRD, "%s%ss, %s%s", col,
                                socbuf_other, col, channel->name );
                }
                if ( !IS_NPC( vch ) ) {
                    snprintf( logbuf, MSL,
                              "&R[%-2.2d/%-2.2d %-2.2d:%-2.2d] &B%s said '&W %s &D&B'&D",
                              local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min,
                              PERS( ch, vch ), sbuf );
                    for ( x = 0; x < 20; x++ ) {
                        if ( vch->pcdata->say_history[x] == NULL ) {
                            vch->pcdata->say_history[x] = STRALLOC( logbuf );
                            break;
                        }
                        if ( x == 19 ) {
                            int                     i;

                            for ( i = 1; i < 20; i++ ) {
                                STRFREE( vch->pcdata->say_history[i - 1] );
                                vch->pcdata->say_history[i - 1] =
                                    STRALLOC( vch->pcdata->say_history[i] );
                            }
                            STRFREE( vch->pcdata->say_history[x] );
                            vch->pcdata->say_history[x] = STRALLOC( logbuf );
                        }
                    }
                }
            }
            else if ( !social && !emote ) {
                snprintf( buf, MSL, "%s [%s] $n '$t%s'", col, capitalize( channel->name ), col );
                act( AT_PLAIN, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
            }
            if ( emote ) {
                snprintf( buf, MSL, " &W[%s%s&W] %s$n $t", col, capitalize( channel->name ), col );
                act( AT_PLAIN, strcat( lbuf, buf ), ch, sbuf, vch, TO_VICT );
            }
            if ( social ) {
                if ( vch == victim )
                    act_printf( AT_PLAIN, ch, NULL, vch, TO_VICT, " &W[%s%s&W] %s%s", col,
                                capitalize( channel->name ), col, socbuf_vict );
                else
                    act_printf( AT_PLAIN, ch, vch, victim, TO_THIRD, " &W[%s%s&W] %s%s", col,
                                capitalize( channel->name ), col, socbuf_other );
            }
            set_position( vch, position );
            /*
             * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
             */
        }
    }

    /*
     * Do programs 
     */
    if ( !str_cmp( channel->name, "osay" ) || !str_cmp( channel->name, "say" ) ) {
        mprog_speech_trigger( argument, ch );
        oprog_speech_trigger( argument, ch );
        rprog_speech_trigger( argument, ch );
    }
}

void to_channel( const char *argument, const char *xchannel, int level )
{
    MUD_CHANNEL            *channel;
    char                    buf[MSL];
    DESCRIPTOR_DATA        *d;

    if ( !first_descriptor || argument[0] == '\0' )
        return;

    if ( !( channel = find_channel( xchannel ) ) )
        return;

    if ( channel->type != CHAN_LOG )
        return;

    snprintf( buf, MSL, "%s: %s\r\n", capitalize( channel->name ), argument );
    for ( d = first_descriptor; d; d = d->next ) {
        CHAR_DATA              *vch;

        vch = d->original ? d->original : d->character;

        if ( !vch )
            continue;

        if ( d->original )
            continue;

        /*
         * This could be coming in higher than the normal level, so check first 
         */
        if ( ( vch->level < level ) && ( vch->trust < level ) )
            continue;

        if ( d->connected == CON_PLAYING && hasname( vch->pcdata->chan_listen, channel->name ) ) {
            set_char_color( AT_LOG, vch );
            send_to_char_color( buf, vch );
        }
    }
    return;
}

bool local_channel_hook( CHAR_DATA *ch, char *command, char *argument )
{
    MUD_CHANNEL            *channel;

    if ( !( channel = find_channel( command ) ) )
        return FALSE;

    if ( ( ch->level < channel->level ) && ( ch->trust < channel->level ) )
        return FALSE;

    /*
     * Logs are meant to be seen, not talked on 
     */
    if ( channel->type == CHAN_LOG )
        return FALSE;

    if ( !IS_NPC( ch ) && !hasname( ch->pcdata->chan_listen, command ) ) {
        ch_printf( ch, "&RYou are not listening to the &G%s &Rchannel.\r\n", channel->name );
        return TRUE;
    }

    send_tochannel( ch, channel, argument );
    return TRUE;
}

/*  Volk 10-8-06  -  Pretty colours, used for global announcements like births and deaths, clan stuff.  */
void announce( const char *argument )
{
    char                    buf[MSL];

    snprintf( buf, MSL, "\r\n&W[&RAnnouncement&W]&C %s&D", argument );
    echo_to_all( AT_RED, buf, ECHOTAR_ALL );
    return;
}
