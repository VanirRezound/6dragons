/********************************************************************************
 **    ( .-""-.   )+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+-+ +-+-+-+-+(   .-""-. )   **
 **     / _  _ \ ( | | | | | | |D|A|R|K| |R|E|A|L|M|S| | | | | | ) / _  _ \    **
 **     |(_\/_)|  )+-+-+-+-+-+ +-+-+-+-+-+-+-+-+-+-+-+ +-+-+-+-+(  |/_)(_\|    **
 **     (_ /\ _)           MULTIVERSE Distribution v1.0            (_ /\ _)    **
 **      |v==v| (       Multiverse source created by Exodus       ) |mmmm|     **
 **      '-..-'           with Aeone, Kain, Kyros, and Kast         '-..-'     **
 **----------------------------------------------------------------------------**
 ** SMAUG 1.4 written by : Thoric (Derek Snider) with Altrag, Blodkai,         **
 **                        Haus, Narn, Scryn, Swordbearer, Tricops, Gorog,     **
 **                        Rennard, Grishnakh, Fireblade and Nivek.            **
 **----------------------------------------------------------------------------**
 ** Original MERC 2.1 by : Hatchet, Furey, and Kahn.                           **
 ** Original Diku MUD by : Hans Staerfeldt, Katja Nyboe, Tom Madsen,           **
 **                        Michael Seifert & Sebastian Hammer.                 **
 **----------------------------------------------------------------------------**
 *		           Bug/Idea/Typo Tracking Module		        *
 ********************************************************************************/

#include <time.h>
#include "h/files.h"
#include "h/mud.h"
#include "h/key.h"

BUG_DATA               *firstBug;
BUG_DATA               *lastBug;
IDEA_DATA              *firstIdea;
IDEA_DATA              *lastIdea;
TYPO_DATA              *firstTypo;
TYPO_DATA              *lastTypo;

char                   *timeStamp args( ( void ) );
void checkBuidty        args( ( CHAR_DATA *ch ) );
void nstralloc          args( ( char **pointer ) );

struct bug_data
{
    BUG_DATA               *next;
    BUG_DATA               *prev;
    char                   *bugDesc;
    char                   *foundBy;
    char                   *foundWhen;
    char                   *fixedBy;
    char                   *fixedWhen;
    char                   *fixDesc;
    short                   type;
    short                   bonus;
    bool                    reward;
    int                     room;
};

struct idea_data
{
    IDEA_DATA              *next;
    IDEA_DATA              *prev;
    char                   *ideaDesc;
    char                   *madeBy;
    char                   *madeWhen;
    char                   *usedBy;
    char                   *usedWhen;
    char                   *useDesc;
    short                   type;
    short                   bonus;
    bool                    reward;
    int                     room;
};

struct typo_data
{
    TYPO_DATA              *next;
    TYPO_DATA              *prev;
    char                   *typoDesc;
    char                   *foundBy;
    char                   *foundWhen;
    char                   *fixedBy;
    char                   *fixedWhen;
    short                   type;
    short                   bonus;
    bool                    reward;
    int                     room;
};

void freeOneBug( BUG_DATA * Bug )
{
    UNLINK( Bug, firstBug, lastBug, next, prev );
    STRFREE( Bug->bugDesc );
    STRFREE( Bug->foundBy );
    STRFREE( Bug->foundWhen );
    STRFREE( Bug->fixedBy );
    STRFREE( Bug->fixedWhen );
    STRFREE( Bug->fixDesc );
    DISPOSE( Bug );
}

void freeBugs( void )
{
    BUG_DATA               *bug,
                           *bug_next;

    for ( bug = firstBug; bug; bug = bug_next ) {
        bug_next = bug->next;
        freeOneBug( bug );
    }
}

// Quick timestamp function. Feel free to change the format if you wish --Exo
char                   *timeStamp( void )
{
    static char             buf[MSL];
    struct tm              *t = localtime( &current_time );
    bool                    ispm = FALSE;
    short                   htime;

    htime = t->tm_hour;
    if ( htime >= 12 ) {
        htime -= 12;
        ispm = TRUE;
    }
    snprintf( buf, MSL, "%02d-%02d-%04d %02d:%02d:%02d%s", ( t->tm_mon + 1 ), t->tm_mday,
              ( t->tm_year + 1900 ), htime, t->tm_min, t->tm_sec, ispm ? "PM" : "AM" );
    return buf;
}

void saveBugs( void )
{
    FILE                   *fp = NULL;
    BUG_DATA               *Bug = NULL;
    char                    fname[MFL];

    snprintf( fname, MFL, "%s%s", SYSTEM_DIR, PBUG_FILE );
    if ( !( fp = FileOpen( fname, "w" ) ) ) {
        bug( "%s: Couldn't open %s for writing!", __FUNCTION__, fname );
        return;
    }

    for ( Bug = firstBug; Bug; Bug = Bug->next ) {
        fprintf( fp, "#BUG\n" );
        fprintf( fp, "FoundBy       %s~\n", Bug->foundBy );
        fprintf( fp, "FoundWhen     %s~\n", Bug->foundWhen );
        fprintf( fp, "BugDesc       %s~\n", strip_cr( Bug->bugDesc ) );
        fprintf( fp, "Type          %d\n", Bug->type );
        if ( Bug->fixedBy )
            fprintf( fp, "FixedBy       %s~\n", Bug->fixedBy );
        if ( Bug->fixedWhen )
            fprintf( fp, "FixedWhen     %s~\n", Bug->fixedWhen );
        if ( Bug->fixDesc )
            fprintf( fp, "FixDesc       %s~\n", Bug->fixDesc );
        fprintf( fp, "Reward        %d\n", Bug->reward );
        fprintf( fp, "RewardBonus   %d\n", Bug->bonus );
        fprintf( fp, "Room          %d\n", Bug->room );
        fprintf( fp, "End\n\n" );
    }
    fprintf( fp, "#END\n" );
    FileClose( fp );
}

void fReadBug( BUG_DATA * Bug, FILE * fp )
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

            case 'B':
                KEY( "BugDesc", Bug->bugDesc, fread_string( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    nstralloc( &Bug->bugDesc );
                    nstralloc( &Bug->foundBy );
                    nstralloc( &Bug->foundWhen );
                    nstralloc( &Bug->fixedBy );
                    nstralloc( &Bug->fixedWhen );
                    nstralloc( &Bug->fixDesc );
                    return;
                }
                break;

            case 'F':
                KEY( "FixDesc", Bug->fixDesc, fread_string( fp ) );
                KEY( "FixedBy", Bug->fixedBy, fread_string( fp ) );
                KEY( "FixedWhen", Bug->fixedWhen, fread_string( fp ) );
                KEY( "FoundBy", Bug->foundBy, fread_string( fp ) );
                KEY( "FoundWhen", Bug->foundWhen, fread_string( fp ) );
                break;

            case 'R':
                KEY( "Reward", Bug->reward, fread_number( fp ) );
                KEY( "RewardBonus", Bug->bonus, fread_number( fp ) );
                KEY( "Room", Bug->room, fread_number( fp ) );
                break;

            case 'T':
                KEY( "Type", Bug->type, fread_number( fp ) );
                break;
        }

        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }
}

void loadBugs( void )
{
    char                    fname[MFL];
    BUG_DATA               *Bug;
    FILE                   *fp;

    firstBug = NULL;
    lastBug = NULL;
    snprintf( fname, MFL, "%s%s", SYSTEM_DIR, PBUG_FILE );
    if ( ( fp = FileOpen( fname, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            const char             *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s: # not found: (%c)", __FUNCTION__, letter );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "BUG" ) ) {
                CREATE( Bug, BUG_DATA, 1 );
                fReadBug( Bug, fp );
                LINK( Bug, firstBug, lastBug, next, prev );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s: bad section: %s", __FUNCTION__, word );
                continue;
            }
        }
        FileClose( fp );
    }
    return;
}

const char             *bugSeverity( short type )
{
    switch ( type ) {
        case 1:
            return "Lesser";
            break;
        case 2:
            return "Minor";
            break;
        case 3:
            return "Major";
            break;
        case 4:
            return "Severe";
            break;
        case 5:
            return "Critical";
            break;
        default:
            return "Unknown??";
            break;
    }
}

short severityArg( char *arg )
{

    if ( !str_cmp( arg, "lesser" ) )
        return 1;
    else if ( !str_cmp( arg, "nothing" ) )
        return 0;
    else if ( !str_cmp( arg, "minor" ) )
        return 2;
    else if ( !str_cmp( arg, "major" ) )
        return 3;
    else if ( !str_cmp( arg, "severe" ) )
        return 4;
    else if ( !str_cmp( arg, "critical" ) )
        return 5;
    else
        return -1;
}

void bugReward( CHAR_DATA *ch, BUG_DATA * error )
{
    int                     gold = 0;
    int                     glory = 0;
    char                    buf1[MSL],
                            buf2[MSL];

    if ( error->reward || IS_NPC( ch ) )
        return;

    if ( !str_cmp( error->foundBy, ch->name ) ) {
        switch ( error->type ) {
            case 0:
                gold = 0;
                glory = 0;
                break;

            case 1:
                gold = 10;
                glory = 1;
                break;

            case 2:
                gold = 25;
                glory = 2;
                break;

            case 3:
                gold = 50;
                glory = 3;
                break;

            case 4:
                gold = 100;
                glory = 5;
                break;

            case 5:
                gold = 200;
                glory = 10;
                break;

            default:
                break;
        }

        if ( error->bonus == 1 ) {
            gold = gold + ( gold / 2 );
            glory = glory + ( glory / 2 );
        }
        else if ( error->bonus == 2 ) {
            gold = gold * 2;
            glory = glory * 2;
        }
        else if ( error->bonus == 3 ) {
            gold = ( gold * 2 ) + ( gold / 2 );
            glory = ( glory * 2 ) + ( glory / 2 );
        }

        snprintf( buf1, MSL, "%s", num_punct( gold ) );
        snprintf( buf2, MSL, "%s", num_punct( glory ) );
        if ( gold == 0 && glory == 0 ) {
            ch_printf( ch,
                       "&CSomeone says, 'The STAFF did not find it fit to reward you for your reported bug!'\r\n" );
            ch_printf( ch,
                       "&CSomeone says, 'The bug was probably submitted prior to your discovery.  Keep looking though!'\r\n" );
        }
        else
            ch_printf( ch,
                       "&CSomeone says, 'The STAFF have rewarded you %s gold and %s glory for finding a %s bug!'\r\n",
                       buf1, buf2, bugSeverity( error->type ) );
        error->reward = TRUE;
        ch->pcdata->num_bugs++;
        ch->pcdata->bugReward1 += gold;
        ch->pcdata->bugReward2 += glory;
        GET_MONEY( ch, CURR_GOLD ) += gold;
        ch->quest_curr += glory;
/* Volk - bug fix */
        send_to_pager
            ( "&CYour bug was:\r\n&B----------------------------------------------------------------------&W\r\n",
              ch );
        send_to_pager( error->bugDesc, ch );
        send_to_pager( "\r\n", ch );
        send_to_pager
            ( "&CSTAFF note:\r\n&B----------------------------------------------------------------------&W\r\n",
              ch );
        send_to_pager( error->fixDesc, ch );
        send_to_pager( "\r\n", ch );
        save_char_obj( ch );
        freeOneBug( error );
        saveBugs(  );
        return;
    }
    return;
}

void do_bug( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    BUG_DATA               *Bug,
                           *nBug;
    short                   cnt = 0;
    DESCRIPTOR_DATA        *d;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;

        case SUB_RESTRICTED:
            send_to_char( "You cannot do this while in another command.\r\n", ch );
            return;

        case SUB_BUG_DESC:
            nBug = ( BUG_DATA * ) ch->dest_buf;
            if ( nBug->bugDesc )
                STRFREE( nBug->bugDesc );
            nBug->bugDesc = copy_buffer( ch );
            stop_editing( ch );

            if ( !VLD_STR( nBug->bugDesc ) ) {
                send_to_char
                    ( "&CSomeone says, 'Nothing was in the bug report so it has been discarded.'\r\n",
                      ch );
                freeOneBug( nBug );
            }
            else {
                send_to_char( "&CSomeone says, 'Thanks, your bug report has been recorded.'\r\n",
                              ch );
                send_to_char( "&CSomeone says, 'You will be notified when the bug is fixed.'\r\n",
                              ch );
                log_printf( "%s has reported a bug.", ch->name );
                saveBugs(  );
            }
            ch->substate = ch->tempnum;
            return;

        case SUB_BUG_FIXDESC:
            Bug = ( BUG_DATA * ) ch->dest_buf;
            if ( Bug->fixDesc )
                STRFREE( Bug->fixDesc );
            Bug->fixDesc = copy_buffer( ch );
            stop_editing( ch );
            saveBugs(  );
            ch->substate = ch->tempnum;
            send_to_char
                ( "Ok, the bug has been fixed and the player who reported it will be notified.\r\n",
                  ch );
            if ( Bug->foundBy ) {
                for ( d = first_descriptor; d; d = d->next ) {
                    if ( !d->character )
                        continue;
                    if ( !str_cmp( Bug->foundBy, d->character->name ) && !NULLSTR( Bug->fixedBy ) ) {
                        checkBuidty( d->character );
                        break;
                    }
                }
            }
            return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( NULLSTR( arg1 ) ) {
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char
                ( "&WThe following are the known bugs within the &CSIX DRAGONS&W realms&D\r\n",
                  ch );
            send_to_char( "&CNum   Room    Date       Time        Name\r\n", ch );
            send_to_char( "&B--------------------------------------------------\r\n", ch );
            for ( Bug = firstBug; Bug; Bug = Bug->next ) {
                cnt++;
                ch_printf( ch, "%s%3d>  %-6d  %-21s  %-10s\r\n",
                           NULLSTR( Bug->fixedBy ) ? "&R" : "&G", cnt, Bug->room, Bug->foundWhen,
                           Bug->foundBy );
            }

            if ( cnt <= 0 ) {
                send_to_char( "No bugs have been reported.\r\n", ch );
                return;
            }
            send_to_char( "&B--------------------------------------------------\r\n", ch );

            send_to_char( "\r\n&WSyntax: bug <field> <number> <severity> <bonus>\r\n", ch );
            send_to_char( "\r\n&cField being   : edit, delete, show, fixed, report\r\n", ch );
            send_to_char( "&cNumber being  : (what bug number)\r\n", ch );
            send_to_char( "&cSeverity being: nothing, lesser, minor, major, severe, critical\r\n",
                          ch );
            send_to_char( "&cBonus being   : none, bonus, bonusx2, both\r\n", ch );
        }
        else {
            send_to_char( "&CBugs\r\nLv Degree    Gold   Glory\r\n", ch );
            send_to_char( "&B--------------------------\r\n", ch );
            send_to_char( "&W0  nothing    0        0\r\n", ch );
            send_to_char( "&W1  lesser    10        1\r\n", ch );
            send_to_char( "&W2  minor     25        2\r\n", ch );
            send_to_char( "&W3  major     50        3\r\n", ch );
            send_to_char( "&W4  severe    100       5\r\n", ch );
            send_to_char( "&W5  critical  200      10\r\n", ch );
            send_to_char( "&B--------------------------\r\n", ch );
            send_to_char
                ( "&CThe STAFF of 6 Dragons will reward you based on the above criteria for\r\nbugs reported.  The reward is automatically generated by the code once the bug is fixed.\r\n",
                  ch );
            send_to_char( "\r\nIf you want to report a bug, type &WBUG REPORT&D\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg1, "report" ) ) {
        CREATE( nBug, BUG_DATA, 1 );
        LINK( nBug, firstBug, lastBug, next, prev );
        nBug->foundBy = STRALLOC( ch->name );
        nBug->foundWhen = STRALLOC( timeStamp(  ) );
        nBug->type = -1;
        nBug->reward = FALSE;
        nBug->room = ch->in_room ? ch->in_room->vnum : -1;
        nBug->fixedBy = NULL;
        nBug->fixedWhen = NULL;
        nBug->fixDesc = NULL;
        nBug->bugDesc = NULL;

        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_BUG_DESC;
        ch->dest_buf = nBug;
        start_editing( ch, nBug->bugDesc );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "show" ) ) {
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "Show which bug?\r\nSyntax: bug show <number>\r\n", ch );
            return;
        }

        cnt = 0;
        for ( Bug = firstBug; Bug; Bug = Bug->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Bug ) {
            send_to_char( "No such bug.\r\n", ch );
            return;
        }
        send_to_char( "&CNum   Room    Date       Time        Name\r\n", ch );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        ch_printf( ch, "%s%3d&C>  &W%-6d  %-21s  %-10s\r\n", NULLSTR( Bug->fixedBy ) ? "&R" : "&W",
                   cnt, Bug->room, Bug->foundWhen, Bug->foundBy );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        ch_printf( ch, "&CDescription\r\n&W%s\r\n", Bug->bugDesc );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );

        if ( !NULLSTR( Bug->fixedBy ) ) {
            ch_printf( ch, "&CFixed by: &W%s\r\n", Bug->fixedBy );
            ch_printf( ch, "&CWhen    : &W%s\r\n", Bug->fixedWhen );
            ch_printf( ch, "&CSeverity: &W%s\r\n", bugSeverity( Bug->type ) );
            ch_printf( ch, "&CRewarded: &W%s\r\n", Bug->reward ? "Yes" : "No" );

            if ( Bug->bonus > 0 )
                ch_printf( ch, "&CBonus   : &W%s\r\n",
                           Bug->bonus == 1 ? "+50%" : Bug->bonus == 1 ? "x2" : Bug->bonus >
                           1 ? "+50% x2" : "None" );
            send_to_char
                ( "&B----------------------------------------------------------------------\r\n",
                  ch );
            ch_printf( ch, "&CNotes\r\n&W%s\r\n", Bug->fixDesc );
            send_to_char
                ( "&B----------------------------------------------------------------------\r\n",
                  ch );
        }
        return;
    }
    else if ( !str_cmp( arg1, "edit" ) ) {
        cnt = 0;
        for ( Bug = firstBug; Bug; Bug = Bug->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Bug ) {
            send_to_char( "No such bug.\r\n", ch );
            return;
        }

        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_BUG_DESC;
        ch->dest_buf = Bug;
        start_editing( ch, Bug->bugDesc );
        return;
    }
    else if ( !str_cmp( arg1, "fixed" ) ) {
        char                    arg3[MIL];
        char                    arg4[MIL];

        argument = one_argument( argument, arg3 );
        argument = one_argument( argument, arg4 );
        if ( NULLSTR( arg2 ) ) {
            send_to_char
                ( "&WWhich bug is fixed?\r\nSyntax: bug fixed <number> <severity> <bonus>\r\n",
                  ch );
            send_to_char( "Severity can be:\r\n  &wnothing lesser minor major severe critical\r\n",
                          ch );
            send_to_char( "&WBonus can be either: &wnone bonus bonusx2 &Wor &wboth\r\n", ch );
            send_to_char
                ( "\r\n&RDo not set a bug as fixed until you are positive the issue is resolved.\r\n",
                  ch );
            return;
        }

        cnt = 0;
        for ( Bug = firstBug; Bug; Bug = Bug->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Bug ) {
            send_to_char( "No such bug.\r\n", ch );
            return;
        }

        if ( NULLSTR( arg3 ) || severityArg( arg3 ) <= -1 ) {
            send_to_char
                ( "How severe is the bug you are fixing?\r\nValid types are:  nothing lesser minor major severe critical\r\n",
                  ch );
            return;
        }

        if ( NULLSTR( arg4 ) ) {
            send_to_char
                ( "Are you giving a bonus for this bug?\r\nValid types are: bonus bonusx2 both\r\nIf not, use none.\r\n",
                  ch );
            return;
        }

        if ( !str_cmp( arg4, "bonus" ) )
            Bug->bonus = 1;
        else if ( !str_cmp( arg4, "bonusx2" ) )
            Bug->bonus = 2;
        else if ( !str_cmp( arg4, "both" ) )
            Bug->bonus = 3;
        else if ( !str_cmp( arg4, "none" ) )
            Bug->bonus = -1;
        else {
            send_to_char
                ( "If you're giving a bonus to the bug reporter, use bonus or bonusx2. If not, use none.\r\n",
                  ch );
            return;
        }

        if ( !NULLSTR( Bug->fixedBy ) ) {
            send_to_char( "That bug has already been fixed.\r\n", ch );
            return;
        }

        Bug->fixedBy = STRALLOC( ch->name );
        Bug->fixedWhen = STRALLOC( timeStamp(  ) );
        Bug->type = severityArg( arg3 );
        Bug->fixDesc = NULL;
        Bug->reward = FALSE;
        send_to_char( "Please enter a description of how the issue was resolved.\r\n", ch );
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;

        ch->substate = SUB_BUG_FIXDESC;
        ch->dest_buf = Bug;
        start_editing( ch, Bug->fixDesc );
        return;
    }
    else if ( !str_cmp( arg1, "delete" ) ) {
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "Delete which bug?\r\nSyntax: bug delete <number>\r\n", ch );
            return;
        }
        cnt = 0;

        for ( Bug = firstBug; Bug; Bug = Bug->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Bug ) {
            send_to_char( "No such bug.\r\n", ch );
            return;
        }
        ch_printf( ch, "Bug #%d deleted.\r\n", cnt );
        freeOneBug( Bug );
        saveBugs(  );
        return;
    }
    else {
        send_to_char( "\r\n&WSyntax: bug\r\nSyntax: bug report\r\n"
                      "Syntax: bug show <number>\r\nSyntax: bug fixed <number> <severity> <bonus>\r\n",
                      ch );
        return;
    }
}

void freeOneIdea( IDEA_DATA * idea )
{
    UNLINK( idea, firstIdea, lastIdea, next, prev );
    STRFREE( idea->ideaDesc );
    STRFREE( idea->madeBy );
    STRFREE( idea->madeWhen );
    STRFREE( idea->usedBy );
    STRFREE( idea->usedWhen );
    STRFREE( idea->useDesc );
    DISPOSE( idea );
}

void freeIdeas( void )
{
    IDEA_DATA              *idea,
                           *idea_next;

    for ( idea = firstIdea; idea; idea = idea_next ) {
        idea_next = idea->next;
        freeOneIdea( idea );
    }
}

void saveIdeas( void )
{
    FILE                   *fp = NULL;
    IDEA_DATA              *idea = NULL;
    char                    fname[MFL];

    snprintf( fname, MFL, "%s%s", SYSTEM_DIR, IDEA_FILE );
    if ( !( fp = FileOpen( fname, "w" ) ) ) {
        bug( "%s: Couldn't open %s for writing", __FUNCTION__, fname );
        return;
    }

    for ( idea = firstIdea; idea; idea = idea->next ) {
        fprintf( fp, "#IDEA\n" );
        fprintf( fp, "MadeBy        %s~\n", idea->madeBy );
        fprintf( fp, "MadeWhen      %s~\n", idea->madeWhen );
        fprintf( fp, "IdeaDesc      %s~\n", strip_cr( idea->ideaDesc ) );
        fprintf( fp, "Type          %d\n", idea->type );
        if ( idea->usedBy )
            fprintf( fp, "UsedBy        %s~\n", idea->usedBy );
        if ( idea->usedWhen )
            fprintf( fp, "UsedWhen      %s~\n", idea->usedWhen );
        if ( idea->useDesc )
            fprintf( fp, "UseDesc       %s~\n", idea->useDesc );
        fprintf( fp, "Reward        %d\n", idea->reward );
        fprintf( fp, "RewardBonus   %d\n", idea->bonus );
        fprintf( fp, "Room          %d\n", idea->room );
        fprintf( fp, "End\n\n" );
    }
    fprintf( fp, "#END\n" );
    FileClose( fp );
}

void fReadIdea( IDEA_DATA * idea, FILE * fp )
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

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    nstralloc( &idea->ideaDesc );
                    nstralloc( &idea->madeBy );
                    nstralloc( &idea->madeWhen );
                    nstralloc( &idea->usedBy );
                    nstralloc( &idea->usedWhen );
                    nstralloc( &idea->useDesc );
                    return;
                }
                break;

            case 'I':
                KEY( "IdeaDesc", idea->ideaDesc, fread_string( fp ) );
                break;

            case 'M':
                KEY( "MadeBy", idea->madeBy, fread_string( fp ) );
                KEY( "MadeWhen", idea->madeWhen, fread_string( fp ) );
                break;

            case 'R':
                KEY( "Reward", idea->reward, fread_number( fp ) );
                KEY( "RewardBonus", idea->bonus, fread_number( fp ) );
                KEY( "Room", idea->room, fread_number( fp ) );
                break;

            case 'T':
                KEY( "Type", idea->type, fread_number( fp ) );
                break;

            case 'U':
                KEY( "UsedBy", idea->usedBy, fread_string( fp ) );
                KEY( "UsedWhen", idea->usedWhen, fread_string( fp ) );
                KEY( "UseDesc", idea->useDesc, fread_string( fp ) );
        }

        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }
}

void loadIdeas( void )
{
    char                    fname[MFL];
    IDEA_DATA              *idea;
    FILE                   *fp;

    firstIdea = NULL;
    lastIdea = NULL;
    snprintf( fname, MFL, "%s%s", SYSTEM_DIR, IDEA_FILE );
    if ( ( fp = FileOpen( fname, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            const char             *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s: # not found: (%c)", __FUNCTION__, letter );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "IDEA" ) ) {
                CREATE( idea, IDEA_DATA, 1 );
                fReadIdea( idea, fp );
                LINK( idea, firstIdea, lastIdea, next, prev );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s: bad section: %s", __FUNCTION__, word );
                continue;
            }
        }
        FileClose( fp );
    }
    return;
}

const char             *ideaLevel( short type )
{
    switch ( type ) {
        case 0:
            return "nothing";
            break;
        case 1:
            return "good";
            break;
        case 2:
            return "great";
            break;
        case 3:
            return "phenomenal";
            break;
        default:
            return "unknown??";
            break;
    }
}

short ideaLevelArg( char *arg )
{
    if ( !str_cmp( arg, "good" ) )
        return 1;
    else if ( !str_cmp( arg, "nothing" ) )
        return 0;
    else if ( !str_cmp( arg, "great" ) )
        return 2;
    else if ( !str_cmp( arg, "phenomenal" ) )
        return 3;
    else
        return -1;
}

void ideaReward( CHAR_DATA *ch, IDEA_DATA * idea )
{
    int                     gold = 0;
    int                     glory = 0;
    char                    buf1[MSL],
                            buf2[MSL];

    if ( idea->reward )
        return;

    if ( !str_cmp( idea->madeBy, ch->name ) ) {
        switch ( idea->type ) {
            case 0:
                gold = 0;
                glory = 0;
                break;

            case 1:
                gold = 3;
                glory = 1;
                break;

            case 2:
                gold = 5;
                glory = 3;
                break;

            case 3:
                gold = 10;
                glory = 5;
                break;

            default:
                break;
        }

        if ( idea->bonus == 1 ) {
            gold = gold + ( gold / 2 );
            glory = glory + ( glory / 2 );
        }
        else if ( idea->bonus == 2 ) {
            gold = gold * 2;
            glory = glory * 2;
        }
        else if ( idea->bonus == 3 ) {
            gold = ( gold * 2 ) + ( gold / 2 );
            glory = ( glory * 2 ) + ( glory / 2 );
        }

        snprintf( buf1, MSL, "%s", num_punct( gold ) );
        snprintf( buf2, MSL, "%s", num_punct( glory ) );
        if ( gold == 0 && glory == 0 ) {
            ch_printf( ch,
                       "&CSomeone says, 'The STAFF did not find it fit to reward you for your submitted idea!'\r\n" );
            ch_printf( ch,
                       "&CSomeone says, 'The idea was probably submitted prior to your discovery.  Keep looking though!'\r\n" );
        }
        else
            ch_printf( ch,
                       "&CSomeone says, 'The STAFF have rewarded you %s gold and %s glory for your %s idea!'\r\n",
                       buf1, buf2, ideaLevel( idea->type ) );
        idea->reward = TRUE;
        ch->pcdata->num_ideas++;
        ch->pcdata->ideaReward1 += gold;
        ch->pcdata->ideaReward2 += glory;
        GET_MONEY( ch, CURR_GOLD ) += gold;
        ch->quest_curr += glory;

        send_to_pager
            ( "&CYour idea was:\r\n&B----------------------------------------------------------------------&W\r\n",
              ch );
        send_to_pager( idea->ideaDesc, ch );
        send_to_pager( "\r\n", ch );
        send_to_pager
            ( "&CSTAFF note:\r\n&B----------------------------------------------------------------------&W\r\n",
              ch );
        send_to_pager( idea->useDesc, ch );
        send_to_pager( "\r\n", ch );

        save_char_obj( ch );
        freeOneIdea( idea );
        saveIdeas(  );
        return;
    }
    return;
}

void do_idea( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    IDEA_DATA              *Idea = NULL,
        *nIdea;
    short                   cnt = 0;
    DESCRIPTOR_DATA        *d;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;

        case SUB_RESTRICTED:
            send_to_char( "You cannot do this while in another command.\r\n", ch );
            return;

        case SUB_IDEA_DESC:
            nIdea = ( IDEA_DATA * ) ch->dest_buf;
            STRFREE( nIdea->ideaDesc );
            nIdea->ideaDesc = copy_buffer( ch );
            stop_editing( ch );

            if ( !VLD_STR( nIdea->ideaDesc ) ) {
                send_to_char
                    ( "&CSomeone says, 'Nothing was in the idea submission so it has been discarded.'\r\n",
                      ch );
                freeOneIdea( nIdea );
            }
            else {
                saveIdeas(  );
                send_to_char
                    ( "&CSomeone says, 'Thanks, your idea submission has been recorded.'\r\n", ch );
                send_to_char
                    ( "&CSomeone says, 'You will be notified if the idea is implemented.'\r\n",
                      ch );
                log_printf( "%s has submitted an idea.", ch->name );
            }

            ch->substate = ch->tempnum;
            return;

        case SUB_IDEA_USEDESC:
            Idea = ( IDEA_DATA * ) ch->dest_buf;
            if ( Idea->useDesc )
                STRFREE( Idea->useDesc );
            Idea->useDesc = copy_buffer( ch );
            stop_editing( ch );
            saveIdeas(  );
            ch->substate = ch->tempnum;
            send_to_char
                ( "Ok, the idea has been used and the player who submitted it will be notified.\r\n",
                  ch );
            if ( Idea->madeBy ) {
                for ( d = first_descriptor; d; d = d->next ) {
                    if ( !d->character )
                        continue;
                    if ( !str_cmp( Idea->madeBy, d->character->name ) && !NULLSTR( Idea->usedBy ) ) {
                        checkBuidty( d->character );
                        break;
                    }
                }
            }
            return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( NULLSTR( arg1 ) ) {
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char
                ( "&WThe following are the known ideas within the &CSIX DRAGONS&W realms&D\r\n",
                  ch );
            send_to_char( "&CNum   Room    Date       Time        Name\r\n", ch );
            send_to_char( "&B--------------------------------------------------\r\n", ch );
            for ( Idea = firstIdea; Idea; Idea = Idea->next ) {
                cnt++;
                ch_printf( ch, "%s%3d>  %-6d  %-21s  %-10s\r\n",
                           NULLSTR( Idea->usedBy ) ? "&R" : "&G", cnt, Idea->room, Idea->madeWhen,
                           Idea->madeBy );
            }

            if ( cnt <= 0 ) {
                send_to_char( "No ideas have been submitted.\r\n", ch );
                return;
            }
            send_to_char( "&B--------------------------------------------------\r\n", ch );

            send_to_char( "\r\n&WSyntax: idea <field> <number> <level> <bonus>\r\n", ch );
            send_to_char( "\r\n&cField being   : edit, delete, show, use, submit\r\n", ch );
            send_to_char( "&cNumber being  : (what idea number)\r\n", ch );
            send_to_char( "&cLevel being   : nothing, good, great, phenomenal\r\n", ch );
            send_to_char( "&cBonus being   : none, bonus, bonusx2, both\r\n", ch );
        }
        else {
            send_to_char( "&CIdeas\r\nLv Degree     Gold   Glory\r\n", ch );
            send_to_char( "&B--------------------------\r\n", ch );
            send_to_char( "&W0  Nothing    0         0\r\n", ch );
            send_to_char( "&W1  Good       3         1\r\n", ch );
            send_to_char( "&W2  Great      5         3\r\n", ch );
            send_to_char( "&W3  Phenomenal 10        5\r\n", ch );
            send_to_char( "&B--------------------------\r\n", ch );
            send_to_char
                ( "&CThe STAFF of 6 Dragons will reward you based on the above criteria for\r\nideas submitted.  The reward is automatically generated by the code once the idea is utilized.\r\n",
                  ch );
            send_to_char( "\r\nIf you want to submit a idea, type &WIDEA SUBMIT&D\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg1, "submit" ) ) {
        CREATE( nIdea, IDEA_DATA, 1 );
        LINK( nIdea, firstIdea, lastIdea, next, prev );
        nIdea->madeBy = STRALLOC( ch->name );
        nIdea->madeWhen = STRALLOC( timeStamp(  ) );
        nIdea->type = -1;
        nIdea->reward = FALSE;
        nIdea->room = ch->in_room ? ch->in_room->vnum : -1;
        nIdea->usedBy = NULL;
        nIdea->usedWhen = NULL;
        nIdea->useDesc = NULL;
        nIdea->ideaDesc = NULL;

        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_IDEA_DESC;
        ch->dest_buf = nIdea;
        start_editing( ch, nIdea->ideaDesc );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "edit" ) ) {
        cnt = 0;
        for ( nIdea = firstIdea; nIdea; nIdea = nIdea->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !nIdea ) {
            send_to_char( "No such idea.\r\n", ch );
            return;
        }

        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_IDEA_DESC;
        ch->dest_buf = nIdea;
        start_editing( ch, nIdea->ideaDesc );
        return;
    }
    else if ( !str_cmp( arg1, "show" ) ) {
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "Show which idea?\r\nSyntax: idea show <number>\r\n", ch );
            return;
        }

        cnt = 0;
        for ( Idea = firstIdea; Idea; Idea = Idea->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Idea ) {
            send_to_char( "No such idea.\r\n", ch );
            return;
        }
        send_to_char( "&CNum   Room    Date       Time        Name\r\n", ch );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        ch_printf( ch, "%s%3d&C>  &W%-6d  %-21s  %-10s\r\n", NULLSTR( Idea->usedBy ) ? "&R" : "&W",
                   cnt, Idea->room, Idea->madeWhen, Idea->madeBy );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        ch_printf( ch, "&CDescription\r\n&W%s\r\n", Idea->ideaDesc );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        if ( !NULLSTR( Idea->usedBy ) ) {
            ch_printf( ch, "&CUsed by : &W%s\r\n", Idea->usedBy );
            ch_printf( ch, "&CWhen    : &W%s\r\n", Idea->usedWhen );
            ch_printf( ch, "&CLevel   : &W%s\r\n", ideaLevel( Idea->type ) );
            ch_printf( ch, "&CRewarded: &W%s\r\n", Idea->reward ? "Yes" : "No" );
            if ( Idea->bonus > 0 )
                ch_printf( ch, "&CBonus   : &W%s\r\n",
                           Idea->bonus == 1 ? "+50%" : Idea->bonus == 2 ? "x2" : Idea->bonus >
                           2 ? "+50% x2" : "None" );
            send_to_char
                ( "&B----------------------------------------------------------------------\r\n",
                  ch );
            ch_printf( ch, "&CNotes\r\n&W%s\r\n", Idea->useDesc );
            send_to_char
                ( "&B----------------------------------------------------------------------\r\n",
                  ch );
        }
        return;
    }
    else if ( !str_cmp( arg1, "use" ) ) {
        char                    arg3[MIL];
        char                    arg4[MIL];

        argument = one_argument( argument, arg3 );
        argument = one_argument( argument, arg4 );
        if ( NULLSTR( arg2 ) ) {
            send_to_char
                ( "&WWhich idea has been used?\r\nSyntax: idea used <number> <level> <bonus>\r\n",
                  ch );
            send_to_char( "Level can be:\r\n  nothing good great phenomenal\r\n", ch );
            send_to_char( "Bonus can be either: &wnone bonus bonusx2 &Wor &wboth\r\n", ch );
            send_to_char
                ( "\r\n&RDo not set a idea as used until you are positive it has been fully implemented.\r\n",
                  ch );
            return;
        }

        cnt = 0;
        for ( Idea = firstIdea; Idea; Idea = Idea->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Idea ) {
            send_to_char( "No such idea.\r\n", ch );
            return;
        }

        if ( NULLSTR( arg3 ) || ideaLevelArg( arg3 ) <= -1 ) {
            send_to_char
                ( "How good is the idea you are using?\r\nValid types are: nothing good great phenomenal\r\n",
                  ch );
            return;
        }

        if ( NULLSTR( arg4 ) ) {
            send_to_char
                ( "Are you giving a bonus for this idea?\r\nValid types are: none bonus bonusx2 both\r\n",
                  ch );
            return;
        }

        if ( !str_cmp( arg4, "bonus" ) )
            Idea->bonus = 1;
        else if ( !str_cmp( arg4, "bonusx2" ) )
            Idea->bonus = 2;
        else if ( !str_cmp( arg4, "both" ) )
            Idea->bonus = 3;
        else if ( !str_cmp( arg4, "none" ) )
            Idea->bonus = -1;
        else {
            send_to_char
                ( "If you're giving a bonus to the idea submitter, use bonus or bonusx2. If not, use none.\r\n",
                  ch );
            return;
        }

        if ( !NULLSTR( Idea->usedBy ) ) {
            send_to_char( "That idea has already been used.\r\n", ch );
            return;
        }

        Idea->usedBy = STRALLOC( ch->name );
        Idea->usedWhen = STRALLOC( timeStamp(  ) );
        Idea->type = ideaLevelArg( arg3 );
        Idea->reward = FALSE;
        if ( VLD_STR( argument ) ) {
            Idea->useDesc = STRALLOC( capitalize( argument ) );
            send_to_char
                ( "Ok, the idea has been used and the player who submitted it will be notified.\r\n",
                  ch );
        }
        else {
            send_to_char( "Please enter a description of how the idea was implemented.\r\n", ch );
            if ( ch->substate == SUB_REPEATCMD )
                ch->tempnum = SUB_REPEATCMD;
            else
                ch->tempnum = SUB_NONE;

            ch->substate = SUB_IDEA_USEDESC;
            ch->dest_buf = Idea;
            start_editing( ch, Idea->useDesc );
        }
        return;
    }
    else if ( !str_cmp( arg1, "delete" ) ) {
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "Delete which idea?\r\nSyntax: idea delete <number>\r\n", ch );
            return;
        }

        cnt = 0;
        for ( Idea = firstIdea; Idea; Idea = Idea->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !Idea ) {
            send_to_char( "No such idea.\r\n", ch );
            return;
        }
        ch_printf( ch, "Idea #%d deleted.\r\n", cnt );
        freeOneIdea( Idea );
        saveIdeas(  );
        return;
    }
    else {
        send_to_char( "\r\n&WSyntax: idea\r\nSyntax: idea submit\r\n"
                      "Syntax: idea show <number>\r\nSyntax: idea use <number> <level> <bonus>\r\n",
                      ch );
        return;
    }
    return;
}

void freeOneTypo( TYPO_DATA * typo )
{
    UNLINK( typo, firstTypo, lastTypo, next, prev );
    STRFREE( typo->typoDesc );
    STRFREE( typo->foundBy );
    STRFREE( typo->foundWhen );
    STRFREE( typo->fixedBy );
    STRFREE( typo->fixedWhen );
    DISPOSE( typo );
}

void freeTypos( void )
{
    TYPO_DATA              *typo,
                           *typo_next;

    for ( typo = firstTypo; typo; typo = typo_next ) {
        typo_next = typo->next;
        freeOneTypo( typo );
    }
}

void saveTypos( void )
{
    FILE                   *fp = NULL;
    TYPO_DATA              *typo = NULL;
    char                    fname[MFL];

    snprintf( fname, MFL, "%s%s", SYSTEM_DIR, TYPO_FILE );
    if ( !( fp = FileOpen( fname, "w" ) ) ) {
        bug( "%s: Couldn't open %s for writing", __FUNCTION__, fname );
        return;
    }

    for ( typo = firstTypo; typo; typo = typo->next ) {
        fprintf( fp, "#TYPO\n" );
        fprintf( fp, "FoundBy       %s~\n", typo->foundBy );
        fprintf( fp, "FoundWhen     %s~\n", typo->foundWhen );
        fprintf( fp, "TypoDesc      %s~\n", strip_cr( typo->typoDesc ) );
        fprintf( fp, "Type          %d\n", typo->type );
        if ( typo->fixedBy )
            fprintf( fp, "FixedBy       %s~\n", typo->fixedBy );
        if ( typo->fixedWhen )
            fprintf( fp, "FixedWhen     %s~\n", typo->fixedWhen );
        fprintf( fp, "Reward        %d\n", typo->reward );
        fprintf( fp, "RewardBonus   %d\n", typo->bonus );
        fprintf( fp, "Room          %d\n", typo->room );
        fprintf( fp, "End\n\n" );
    }
    fprintf( fp, "#END\n" );
    FileClose( fp );
    return;
}

void fReadTypo( TYPO_DATA * typo, FILE * fp )
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

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    nstralloc( &typo->typoDesc );
                    nstralloc( &typo->foundBy );
                    nstralloc( &typo->foundWhen );
                    nstralloc( &typo->fixedBy );
                    nstralloc( &typo->fixedWhen );
                    return;
                }
                break;

            case 'F':
                KEY( "FixedBy", typo->fixedBy, fread_string( fp ) );
                KEY( "FixedWhen", typo->fixedWhen, fread_string( fp ) );
                KEY( "FoundBy", typo->foundBy, fread_string( fp ) );
                KEY( "FoundWhen", typo->foundWhen, fread_string( fp ) );
                break;

            case 'R':
                KEY( "Reward", typo->reward, fread_number( fp ) );
                KEY( "RewardBonus", typo->bonus, fread_number( fp ) );
                KEY( "Room", typo->room, fread_number( fp ) );
                break;

            case 'T':
                KEY( "Type", typo->type, fread_number( fp ) );
                KEY( "TypoDesc", typo->typoDesc, fread_string( fp ) );
                break;
        }

        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }
}

void loadTypos( void )
{
    char                    fname[MFL];
    TYPO_DATA              *typo;
    FILE                   *fp;

    firstTypo = NULL;
    lastTypo = NULL;
    snprintf( fname, MFL, "%s%s", SYSTEM_DIR, TYPO_FILE );
    if ( ( fp = FileOpen( fname, "r" ) ) != NULL ) {
        for ( ;; ) {
            char                    letter;
            const char             *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s: # not found.", __FUNCTION__ );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "TYPO" ) ) {
                CREATE( typo, TYPO_DATA, 1 );
                fReadTypo( typo, fp );
                LINK( typo, firstTypo, lastTypo, next, prev );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s: bad section: %s", __FUNCTION__, word );
                continue;
            }
        }
        FileClose( fp );
    }
    return;
}

const char             *typoLevel( short type )
{
    switch ( type ) {
        case 0:
            return "nothing";
            break;
        case 1:
            return "misspelled";
            break;
        case 2:
            return "misused";
            break;
        case 3:
            return "offensive";
            break;
        default:
            return "Unknown??";
            break;
    }
}

short typoLevelArg( char *arg )
{
    if ( !str_cmp( arg, "misspelled" ) )
        return 1;
    else if ( !str_cmp( arg, "nothing" ) )
        return 0;
    else if ( !str_cmp( arg, "misused" ) )
        return 2;
    else if ( !str_cmp( arg, "offensive" ) )
        return 3;
    else
        return -1;
}

void typoReward( CHAR_DATA *ch, TYPO_DATA * typo )
{
    int                     gold = 0;
    int                     glory = 0;
    char                    buf1[MSL],
                            buf2[MSL];

    if ( typo->reward )
        return;

    if ( !str_cmp( typo->foundBy, ch->name ) ) {
        switch ( typo->type ) {
            case 0:
                gold = 0;
                glory = 0;
                break;
            case 1:
                gold = 1;
                glory = 1;
                break;
            case 2:
                gold = 3;
                glory = 3;
                break;
            case 3:
                gold = 5;
                glory = 5;
                break;
            default:
                break;
        }

        if ( typo->bonus == 1 ) {
            gold = gold + ( gold / 2 );
            glory = glory + ( glory / 2 );
        }
        else if ( typo->bonus == 2 ) {
            gold = gold * 2;
            glory = glory * 2;
        }
        else if ( typo->bonus == 3 ) {
            gold = ( gold * 2 ) + ( gold / 2 );
            glory = ( glory * 2 ) + ( glory / 2 );
        }
        snprintf( buf1, MSL, "%s", num_punct( gold ) );
        snprintf( buf2, MSL, "%s", num_punct( glory ) );
        if ( gold == 0 && glory == 0 ) {
            ch_printf( ch,
                       "&CSomeone says, 'The STAFF did not find it fit to reward you for your reported typo!'\r\n" );
            ch_printf( ch,
                       "&CSomeone says, 'The typo was probably submitted prior to your discovery.  Keep looking though!'\r\n" );
        }
        else
            ch_printf( ch,
                       "&CSomeone says, 'The STAFF have rewarded you %s gold and %s glory for finding %s typo!'\r\n",
                       buf1, buf2, aoran( typoLevel( typo->type ) ) );

        typo->reward = TRUE;
        ch->pcdata->num_typos++;
        ch->pcdata->typoReward1 += gold;
        ch->pcdata->typoReward2 += glory;
        GET_MONEY( ch, CURR_GOLD ) += gold;
        ch->quest_curr += glory;

        send_to_pager
            ( "&CYour typo was:\r\n&B----------------------------------------------------------------------&W\r\n",
              ch );
        send_to_pager( typo->typoDesc, ch );
        send_to_pager( "&C\r\nThanks!\r\n", ch );

        save_char_obj( ch );
        freeOneTypo( typo );
        saveTypos(  );
        return;
    }
    return;
}

void do_typo( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    TYPO_DATA              *typo = NULL,
        *nTypo;
    short                   cnt = 0;
    DESCRIPTOR_DATA        *d;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;

        case SUB_RESTRICTED:
            send_to_char( "You cannot do this while in another command.\r\n", ch );
            return;

        case SUB_TYPO_DESC:
            nTypo = ( TYPO_DATA * ) ch->dest_buf;
            if ( nTypo->typoDesc )
                STRFREE( nTypo->typoDesc );
            nTypo->typoDesc = copy_buffer( ch );
            stop_editing( ch );
            if ( !VLD_STR( nTypo->typoDesc ) ) {
                send_to_char
                    ( "&CSomeone says, 'Nothing was in the typo report so it has been discarded.'\r\n",
                      ch );
                freeOneTypo( nTypo );
            }
            else {
                saveTypos(  );
                ch->substate = ch->tempnum;
                send_to_char( "&CSomeone says, 'Thanks, your typo report has been recorded.'\r\n",
                              ch );
                send_to_char( "&CSomeone says, 'You will be notified when the typo is fixed.'\r\n",
                              ch );
                log_printf( "%s has reported a typo.", ch->name );
            }

            ch->substate = ch->tempnum;
            return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( NULLSTR( arg1 ) ) {
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char
                ( "&WThe following are the known typos within the &CSIX DRAGONS&W realms&D\r\n",
                  ch );
            send_to_char( "&CNum   Room    Date       Time        Name\r\n", ch );
            send_to_char( "&B--------------------------------------------------\r\n", ch );
            for ( typo = firstTypo; typo; typo = typo->next ) {
                cnt++;
                ch_printf( ch, "%s%3d>  %-6d  %-21s  %-10s\r\n",
                           NULLSTR( typo->fixedBy ) ? "&R" : "&G", cnt, typo->room, typo->foundWhen,
                           typo->foundBy );
            }

            if ( cnt <= 0 ) {
                send_to_char( "No typos have been reported.\r\n", ch );
                return;
            }
            send_to_char( "&B--------------------------------------------------\r\n", ch );
            send_to_char( "\r\n&WSyntax: typo <field> <number> <level> <bonus>\r\n", ch );
            send_to_char( "\r\n&cField being   : edit, delete, report, show, fixed\r\n", ch );
            send_to_char( "&cNumber being  : (what typo number)\r\n", ch );
            send_to_char( "&cLevel being: nothing, misspelled, misused, offensive\r\n", ch );
            send_to_char( "&cBonus being   : none, bonus, bonusx2, both\r\n", ch );
        }
        else {
            send_to_char( "&CTypos\r\nLv Degree     Gold   Glory\r\n", ch );
            send_to_char( "&B--------------------------\r\n", ch );
            send_to_char( "&W0  nothing    0         0\r\n", ch );
            send_to_char( "&W1  misspelled 1         1\r\n", ch );
            send_to_char( "&W2  misused    3         3\r\n", ch );
            send_to_char( "&W3  offensive  5         5\r\n", ch );
            send_to_char( "&B--------------------------\r\n", ch );
            send_to_char
                ( "&CTypos are words in the game that are either misspelled, misused, or offensive.\r\n",
                  ch );
            send_to_char
                ( "&CThe STAFF of 6 Dragons will reward you based on the above criteria for\r\ntypos reported. The reward is automatically generated by the code once the typo is fixed.\r\n",
                  ch );
            send_to_char( "\r\nIf you want to report a typo, type &WTYPO REPORT&D\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg1, "report" ) ) {
        CREATE( nTypo, TYPO_DATA, 1 );
        LINK( nTypo, firstTypo, lastTypo, next, prev );
        nTypo->foundBy = STRALLOC( ch->name );
        nTypo->foundWhen = STRALLOC( timeStamp(  ) );
        nTypo->type = -1;
        nTypo->reward = FALSE;
        nTypo->room = ch->in_room ? ch->in_room->vnum : -1;
        nTypo->fixedBy = NULL;
        nTypo->fixedWhen = NULL;
        nTypo->typoDesc = NULL;
        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;

        ch->substate = SUB_TYPO_DESC;
        ch->dest_buf = nTypo;
        start_editing( ch, nTypo->typoDesc );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "edit" ) ) {
        cnt = 0;
        for ( nTypo = firstTypo; nTypo; nTypo = nTypo->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !nTypo ) {
            send_to_char( "No such typo.\r\n", ch );
            return;
        }

        if ( ch->substate == SUB_REPEATCMD )
            ch->tempnum = SUB_REPEATCMD;
        else
            ch->tempnum = SUB_NONE;
        ch->substate = SUB_TYPO_DESC;
        ch->dest_buf = nTypo;
        start_editing( ch, nTypo->typoDesc );
        return;
    }
    else if ( !str_cmp( arg1, "show" ) ) {
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "Show which typo?\r\nSyntax: typo show <number>\r\n", ch );
            return;
        }

        cnt = 0;
        for ( typo = firstTypo; typo; typo = typo->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !typo ) {
            send_to_char( "No such typo.\r\n", ch );
            return;
        }

        send_to_char( "&CNum   Room    Date       Time        Name\r\n", ch );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        ch_printf( ch, "%s%3d&C>  &W%-6d  %-21s  %-10s\r\n", NULLSTR( typo->fixedBy ) ? "&R" : "&W",
                   cnt, typo->room, typo->foundWhen, typo->foundBy );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        ch_printf( ch, "&CDescription\r\n&W%s\r\n", typo->typoDesc );
        send_to_char
            ( "&B----------------------------------------------------------------------\r\n", ch );
        if ( !NULLSTR( typo->fixedBy ) ) {
            ch_printf( ch, "&CUsed by : &W%s\r\n", typo->fixedBy );
            ch_printf( ch, "&CWhen    : &W%s\r\n", typo->fixedWhen );
            ch_printf( ch, "&CLevel   : &W%s\r\n", typoLevel( typo->type ) );
            ch_printf( ch, "&CRewarded: &W%s\r\n", typo->reward ? "Yes" : "No" );
            if ( typo->bonus > 0 )
                ch_printf( ch, "&CBonus   : &W%s\r\n",
                           typo->bonus == 1 ? "+50%" : typo->bonus == 2 ? "x2" : typo->bonus >
                           2 ? "+50% x2" : "None" );
        }
        return;
    }
    else if ( !str_cmp( arg1, "fixed" ) ) {
        char                    arg3[MIL];
        char                    arg4[MIL];

        argument = one_argument( argument, arg3 );
        argument = one_argument( argument, arg4 );
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "&WWhich typo is fixed?\r\nSyntax: typo use <number> <level> <bonus>\r\n",
                          ch );
            send_to_char( "Level can be:\r\n  &wnothing misspelled misused offensive\r\n", ch );
            send_to_char( "&WBonus can be either: &wnone bonus bonusx2 &Wor &wboth\r\n", ch );
            send_to_char
                ( "\r\n&RDo not set a typo as fixed until you are positive it has been corrected.\r\n",
                  ch );
            return;
        }

        cnt = 0;
        for ( typo = firstTypo; typo; typo = typo->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !typo ) {
            send_to_char( "No such typo.\r\n", ch );
            return;
        }

        if ( NULLSTR( arg3 ) || typoLevelArg( arg3 ) == -1 ) {
            send_to_char
                ( "&WHow wrong was the typo you corrected?\r\nValid types are: &wnothing misspelled misused offensive\r\n",
                  ch );
            return;
        }

        if ( NULLSTR( arg4 ) ) {
            send_to_char
                ( "&WAre you giving a bonus for this typo?\r\nValid types are: &wnone bonus bonusx2 both\r\n",
                  ch );
            return;
        }

        if ( !str_cmp( arg4, "bonus" ) )
            typo->bonus = 1;
        else if ( !str_cmp( arg4, "bonusx2" ) )
            typo->bonus = 2;
        else if ( !str_cmp( arg4, "both" ) )
            typo->bonus = 3;
        else if ( !str_cmp( arg4, "none" ) )
            typo->bonus = -1;
        else {
            send_to_char
                ( "If you're giving a bonus to the typo reporter, use bonus, bonusx2 or both. If not, use none.\r\n",
                  ch );
            return;
        }

        if ( !NULLSTR( typo->fixedBy ) ) {
            send_to_char( "That typo has already been used.\r\n", ch );
            return;
        }

        typo->fixedBy = STRALLOC( ch->name );
        typo->fixedWhen = STRALLOC( timeStamp(  ) );
        typo->type = typoLevelArg( arg3 );
        typo->reward = FALSE;
        send_to_char
            ( "Ok, the typo has been corrected and the player who reported it will be notified.\r\n",
              ch );
        saveTypos(  );
        if ( typo->foundBy ) {
            for ( d = first_descriptor; d; d = d->next ) {
                if ( !d->character )
                    continue;
                if ( !str_cmp( typo->foundBy, d->character->name ) && !NULLSTR( typo->fixedBy ) ) {
                    checkBuidty( d->character );
                    break;
                }
            }
        }
        return;
    }
    else if ( !str_cmp( arg1, "delete" ) ) {
        if ( NULLSTR( arg2 ) ) {
            send_to_char( "Delete which typo?\r\nSyntax: typo delete <number>\r\n", ch );
            return;
        }

        cnt = 0;
        for ( typo = firstTypo; typo; typo = typo->next ) {
            cnt++;
            if ( cnt == atoi( arg2 ) )
                break;
        }

        if ( !typo ) {
            send_to_char( "No such typo.\r\n", ch );
            return;
        }

        ch_printf( ch, "typo #%d deleted.\r\n", cnt );
        freeOneTypo( typo );
        saveTypos(  );
        return;
    }
    else {
        send_to_char( "\r\n&WSyntax: typo\r\nSyntax: typo report <typo>\r\n"
                      "Syntax: typo show <number>\r\nSyntax: typo fixed <number> <level> <bonus>\r\n",
                      ch );
        return;
    }
    return;
}

void checkBuidty( CHAR_DATA *ch )
{
    BUG_DATA               *Bug,
                           *bug_next;
    IDEA_DATA              *idea,
                           *idea_next;
    TYPO_DATA              *typo,
                           *typo_next;
    int                     bcnt = 0,
        icnt = 0,
        tcnt = 0;

    if ( !ch ) {
        bug( "%s: NULL ch!", __FUNCTION__ );
        return;
    }

    for ( Bug = firstBug; Bug; Bug = bug_next ) {
        bug_next = Bug->next;

        if ( NULLSTR( Bug->fixedBy ) ) {
            bcnt++;
            continue;
        }

        if ( Bug->reward )
            continue;

        if ( !str_cmp( Bug->foundBy, ch->name ) )
            bugReward( ch, Bug );
    }

    for ( idea = firstIdea; idea; idea = idea_next ) {
        idea_next = idea->next;

        if ( NULLSTR( idea->usedBy ) ) {
            icnt++;
            continue;
        }

        if ( idea->reward )
            continue;

        if ( !str_cmp( idea->madeBy, ch->name ) )
            ideaReward( ch, idea );
    }

    for ( typo = firstTypo; typo; typo = typo_next ) {
        typo_next = typo->next;

        if ( NULLSTR( typo->fixedBy ) ) {
            tcnt++;
            continue;
        }

        if ( typo->reward )
            continue;

        if ( !str_cmp( typo->foundBy, ch->name ) )
            typoReward( ch, typo );
    }

    if ( IS_IMMORTAL( ch ) ) {
        if ( bcnt > 0 )
            ch_printf( ch,
                       "&R+++ &WThere are }R%d &d&Wgame issues that need resolving. Type &GBUG &Wto view. &R+++&D\r\n",
                       bcnt );

        if ( icnt > 0 )
            ch_printf( ch,
                       "&R+++ &WThere are }R%d &d&Wideas that need reviewing. Type &GIDEA &Wto view. &R+++&D\r\n",
                       icnt );

        if ( tcnt > 0 )
            ch_printf( ch,
                       "&R+++ &WThere are }R%d &d&Wtypos that need to be fixed. Type &GTYPO &Wto view. &R+++&D\r\n",
                       tcnt );
    }
}

void pruneBuidty( void )
{
    BUG_DATA               *Bug = NULL;
    IDEA_DATA              *Idea = NULL;
    TYPO_DATA              *Typo = NULL;

    for ( Bug = firstBug; Bug; Bug = Bug->next ) {
        if ( Bug && Bug->reward == TRUE )
            freeOneBug( Bug );
    }
    saveBugs(  );

    for ( Idea = firstIdea; Idea; Idea = Idea->next ) {
        if ( Idea && Idea->reward == TRUE )
            freeOneIdea( Idea );
    }
    saveIdeas(  );

    for ( Typo = firstTypo; Typo; Typo = Typo->next ) {
        if ( Typo && Typo->reward == TRUE )
            freeOneTypo( Typo );
    }
    saveTypos(  );
    return;
}
