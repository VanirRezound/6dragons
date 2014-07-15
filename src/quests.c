/******************************************************
              Thy Quest mud -  By Volk, 2006
                  http://www.thyquest.com
 ******************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/key.h"
#include "h/clans.h"

QUEST_DATA             *first_quest;
QUEST_DATA             *last_quest;

void                    add_quest( QUEST_DATA * quest );
char                   *show_timeleft( int time );
void                    add_chapter( QUEST_DATA * quest, CHAP_DATA * chap );

void unlink_chapter( QUEST_DATA * quest, CHAP_DATA * chap )
{
    UNLINK( chap, quest->first_chapter, quest->last_chapter, next, prev );
}

void unlink_quest( QUEST_DATA * quest )
{
    UNLINK( quest, first_quest, last_quest, next, prev );
}

void free_chapter( QUEST_DATA * quest, CHAP_DATA * chapter )
{
    if ( !chapter || !quest )
        return;
    unlink_chapter( quest, chapter );
    STRFREE( chapter->desc );
    STRFREE( chapter->bio );
    DISPOSE( chapter );
}

void free_quest( QUEST_DATA * quest )
{
    CHAP_DATA              *chapter,
                           *chapter_next;

    if ( !quest )
        return;
    unlink_quest( quest );
    STRFREE( quest->name );
    STRFREE( quest->desc );
    for ( chapter = quest->first_chapter; chapter; chapter = chapter_next ) {
        chapter_next = chapter->next;
        free_chapter( quest, chapter );
    }
    DISPOSE( quest );
}

void free_all_quest( void )
{
    while ( last_quest )
        free_quest( last_quest );
}

QUEST_DATA             *get_quest_from_number( int x )
{
    QUEST_DATA             *quest;

    for ( quest = first_quest; quest; quest = quest->next ) {
        if ( quest->number == x )
            return quest;
    }

    return NULL;
}

CHAP_DATA              *get_chap_from_quest( int x, QUEST_DATA * quest )
{
    CHAP_DATA              *chap;

    for ( chap = quest->first_chapter; chap; chap = chap->next ) {
        if ( chap->number == x )
            return chap;
    }

    return NULL;
}

QUEST_DATA             *get_quest_from_name( const char *name )
{
    QUEST_DATA             *quest;

    for ( quest = first_quest; quest; quest = quest->next ) {
        if ( !str_cmp( quest->name, name ) )
            return quest;
    }

    return NULL;
}

int get_number_from_quest( QUEST_DATA * quest )
{
    if ( quest )
        return quest->number;

    return -1;
}

void add_chapter( QUEST_DATA * quest, CHAP_DATA * chap )
{
    CHAP_DATA              *tmp;

    if ( !chap ) {
        bug( "%s", "Add_chapter: NULL chap" );
        return;
    }
    if ( !quest ) {
        bug( "%s", "ADD_chapter: NULL quest" );
        return;
    }

    for ( tmp = quest->first_chapter; tmp; tmp = tmp->next ) {
        if ( chap->number < tmp->number ) {
            INSERT( chap, tmp, quest->first_chapter, next, prev );
            return;
        }
    }
    LINK( chap, quest->first_chapter, quest->last_chapter, next, prev );
}

void add_new_quest( CHAR_DATA *ch, QUEST_DATA * quest )
{
    send_to_char( "New quest added.\r\n", ch );
    add_quest( quest );
}

void add_quest( QUEST_DATA * quest )
{
    QUEST_DATA             *tmp;
    int                     qcount = 0;

    if ( !quest ) {
        bug( "%s", "Add_quest: NULL quest" );
        return;
    }

    if ( !quest->name ) {
        bug( "%s", "Add_quest: NULL quest->name" );
        return;
    }

    if ( quest->number != -1 && get_quest_from_number( quest->number ) != NULL ) {
        bug( "%s: Already a quest numbered %d!", __FUNCTION__, quest->number );
        return;
    }

    for ( tmp = first_quest; tmp; tmp = tmp->next ) {
        /*
         * Get the highest number used so far and use the one after that 
         */
        if ( qcount < tmp->number )
            qcount = tmp->number;

        if ( quest->number != -1 && quest->number < tmp->number ) {
            INSERT( quest, tmp, first_quest, next, prev );
            return;
        }
    }

    if ( quest->number == -1 )
        quest->number = ( qcount + 1 );

    LINK( quest, first_quest, last_quest, next, prev );
}

void fwrite_chap( CHAP_DATA * chap, FILE * fp )
{
    if ( !chap )
        return;

    fprintf( fp, "%s", "#NCHAPTER\n" );
    fprintf( fp, "Number      %d\n", chap->number );
    if ( chap->desc )
        fprintf( fp, "Desc        %s~\n", strip_cr( chap->desc ) );
    if ( chap->bio )
        fprintf( fp, "Bio         %s~\n", chap->bio );
    fprintf( fp, "Timelimit   %d\n", chap->timelimit );
    fprintf( fp, "Level       %d\n", chap->level );
    fprintf( fp, "KAmount     %d\n", chap->kamount );
    fprintf( fp, "#CHAPEND\n" );
}

/* Write the quest. */
void fwrite_quest( QUEST_DATA * quest, FILE * fp )
{
    CHAP_DATA              *chap;

    if ( !quest )
        return;

    fprintf( fp, "%s", "#NQUEST\n" );
    fprintf( fp, "Number      %d\n", quest->number );
    fprintf( fp, "SType        %d\n", quest->stype );
    fprintf( fp, "SVnum       %d\n", quest->svnum );
    fprintf( fp, "Name        %s~\n", quest->name );
    fprintf( fp, "Desc        %s~\n", strip_cr( quest->desc ) );
    fprintf( fp, "Timelimit   %d\n", quest->timelimit );
    fprintf( fp, "Level       %d\n", quest->level );
    fprintf( fp, "Chapters    %d\n", quest->chapters );
    if ( quest->skipchapters )
        fprintf( fp, "%s", "SkipChapters\n" );
    for ( chap = quest->first_chapter; chap; chap = chap->next )
        fwrite_chap( chap, fp );

    fprintf( fp, "End\n\n" );
}

void write_quest_list( void )
{
    FILE                   *fpout;
    QUEST_DATA             *quest;
    char                    filename[256];
    int                     x;

    snprintf( filename, 256, "%s", QUESTS_FILE );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "Cannot open %s for writing!\r\n", QUESTS_FILE );
        return;
    }

    for ( quest = first_quest; quest; quest = quest->next )
        fwrite_quest( quest, fpout );

    fprintf( fpout, "#END\n" );
    FileClose( fpout );
}

void fread_chap( bool cnew, QUEST_DATA * quest, FILE * fp )
{
    const char             *word;
    bool                    fMatch;
    CHAP_DATA              *chap;

    CREATE( chap, CHAP_DATA, 1 );

    if ( !cnew )
        chap->number = fread_number( fp );

    for ( ;; ) {
        word = feof( fp ) ? "#CHAPEND" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case '#':
                if ( !str_cmp( word, "#CHAPEND" ) ) {
                    add_chapter( quest, chap );
                    return;
                }
                break;

            case 'C':
                if ( !str_cmp( word, "Bio" ) ) {
                    chap->bio = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'D':
                if ( !str_cmp( word, "Desc" ) ) {
                    chap->desc = fread_string( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'K':
                KEY( "KAmount", chap->kamount, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", chap->level, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Number", chap->number, fread_number( fp ) );
                break;

            case 'T':
                KEY( "Timelimit", chap->timelimit, fread_number( fp ) );
                break;
        }

        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }
}

/* Read in a quest. */
void fread_quest( bool cnew, FILE * fp )
{
    const char             *word;
    bool                    fMatch;
    QUEST_DATA             *quest;

    CREATE( quest, QUEST_DATA, 1 );

    quest->skipchapters = FALSE;
    if ( !cnew )
        quest->number = fread_number( fp );
    else
        quest->number = -1;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case '#':
                if ( !strcmp( word, "#CHAPTER" ) ) {
                    fread_chap( FALSE, quest, fp );
                    fMatch = TRUE;
                    break;
                }
                if ( !strcmp( word, "#NCHAPTER" ) ) {
                    fread_chap( TRUE, quest, fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY( "Chapters", quest->chapters, fread_number( fp ) );
                break;

            case 'D':
                if ( !str_cmp( word, "Desc" ) ) {
                    KEY( "Desc", quest->desc, fread_string( fp ) );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    add_quest( quest );
                    return;
                }
                break;

            case 'L':
                KEY( "Level", quest->level, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", quest->name, fread_string( fp ) );
                KEY( "Number", quest->number, fread_number( fp ) );
                break;

            case 'S':
                KEY( "SType", quest->stype, fread_number( fp ) );
                KEY( "SVnum", quest->svnum, fread_number( fp ) );
                if ( !str_cmp( word, "SkipChapters" ) ) {
                    quest->skipchapters = TRUE;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'T':
                KEY( "Timelimit", quest->timelimit, fread_number( fp ) );
                break;
        }
        if ( !fMatch )
            bug( "Fread_quest: no match: %s", word );
    }
}

void load_quest_list( void )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( QUESTS_FILE, "r" ) ) != NULL ) {

        for ( ;; ) {
            char                    letter;
            char                   *word;
            int                     x;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_quest_list: # not found." );
                break;
            }

            word = fread_word( fp );

            if ( !str_cmp( word, "QUEST" ) ) {
                fread_quest( FALSE, fp );
                continue;
            }
            else if ( !str_cmp( word, "NQUEST" ) ) {
                fread_quest( TRUE, fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_quest_list: bad section." );
                continue;
            }
        }
        FileClose( fp );
    }
    else {
        perror( QUESTS_FILE );
        bug( "%s", "Cannot open quests.dat" );
        exit( 0 );
    }
}

void do_setquest( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            arg4[MIL];
    QUEST_DATA             *quest;
    int                     x;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        error( ch );
        return;
    }

    if ( !ch->desc ) {
        bug( "%s", "do_setquest (desc): no descriptor" );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;

        case SUB_QUEST_DESC:
            if ( !ch->dest_buf || !( quest = ( QUEST_DATA * ) ch->dest_buf ) ) {
                bug( "%s: sub_quest_desc: NULL ch->dest_buf", __FUNCTION__ );
                ch->substate = SUB_NONE;
                return;
            }
            ch->dest_buf = NULL;
            if ( VLD_STR( quest->desc ) )
                STRFREE( quest->desc );
            quest->desc = copy_buffer( ch );
            stop_editing( ch );
            write_quest_list(  );
            ch->substate = SUB_NONE;
            return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "&cUsage: setquest save all\r\n", ch );
        send_to_char( "       setquest <&Cqname&c> create\r\n", ch );
        send_to_char( "       setquest <&Cqname&c> delete\r\n", ch );
        send_to_char( "       setquest <&Cqname&c> chapter <#> delete\r\n", ch );
        send_to_char( "       setquest <&Cqname&c> desc\r\n", ch );
        send_to_char( "       setquest <&Cqname&c> <&Cfield&c>\r\n", ch );
        send_to_char( "       setquest <&Cquest&c> remove <&Cplayer&c>\r\n", ch );
        send_to_char( "       showquest <&Cqname&c>\r\n", ch );
        send_to_char( "  Field being one of:\r\n", ch );
        send_to_char( "level svnum stype chapters chapter timelimit skipchapters\r\n", ch );
        send_to_char( "\r\nchapter <n> <field2>\r\n", ch );
        send_to_char( "  Field2 being one of:\r\n", ch );
        send_to_char( "create delete svnum stype\r\n", ch );
        send_to_char( "name timelimit level kamount\r\n", ch );
        send_to_char( "Note: 3600 = 1 hour timelimit\r\n", ch );
        return;
    }

    if ( ( !str_cmp( arg1, "tutorial" ) && ch->level < 108 )
         || ( !str_cmp( arg1, "etutorial" ) && ch->level < 108 ) || ( !str_cmp( arg1, "dtutorial" )
                                                                      && ch->level < 108 ) ) {
        send_to_char( "You need Vladaar's permission to change anything with tutorials.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "save" ) ) {
        if ( !str_cmp( arg2, "all" ) ) {
            write_quest_list(  );
            send_to_char( "All quests saved.\r\n", ch );
            return;
        }
    }

    quest = get_quest_from_name( arg1 );

    if ( !str_cmp( arg2, "create" ) ) {
        if ( quest && VLD_STR( quest->name ) && !str_cmp( quest->name, arg1 ) ) {
            ch_printf( ch, "(%s): quest already exists!\r\n", quest->name );
            return;
        }

        CREATE( quest, QUEST_DATA, 1 );
        quest->name = STRALLOC( arg1 );
        quest->number = -1;
        add_new_quest( ch, quest );
        write_quest_list(  );
        return;
    }

    if ( !quest ) {
        send_to_char( "No quest by that name.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "desc" ) ) {
        if ( !ch->desc ) {
            bug( "%s", "do_setquest (desc): no descriptor" );
            return;
        }

        ch->substate = SUB_QUEST_DESC;
        ch->dest_buf = quest;
        start_editing( ch, quest->desc );
        return;
    }

    if ( !str_cmp( arg2, "delete" ) ) {
        free_quest( quest );
        send_to_char( "Deleted.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "skipchapters" ) ) {
        quest->skipchapters = !quest->skipchapters;
        ch_printf( ch, "That quest will %s allow chapters to be skipped.\r\n",
                   quest->skipchapters ? "now" : "no longer" );
        return;
    }

    if ( !str_cmp( arg2, "chapters" ) ) {
        x = atoi( argument );
        if ( x < 0 || x > 30 ) {
            send_to_char( "Chapters must be between 0 and 30.\r\n", ch );
            return;
        }
        quest->chapters = x;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "svnum" ) ) {
        quest->svnum = atoi( argument );
        ch_printf( ch, "That quest's svnum (starting vnum) is set to %d.\r\n", quest->svnum );
        return;
    }

    if ( !str_cmp( arg2, "stype" ) ) {
        x = atoi( argument );
        if ( x < 0 || x > 2 ) {
            send_to_char
                ( "Can only set stype (starting type) to 0 for Mobiles, 1 for Objects, 2 for Rooms.\r\n",
                  ch );
            return;
        }
        quest->stype = x;
        ch_printf( ch, "That quest's stype (starting type) is set to %d[%s].\r\n", quest->stype,
                   ( quest->stype == 0 ) ? "Mobile" : ( quest->stype ==
                                                        1 ) ? "Object" : ( quest->stype ==
                                                                           2 ) ? "Room" :
                   "Unknown" );
        return;
    }

    if ( !str_cmp( arg2, "chapter" ) ) {
        CHAP_DATA              *chap = NULL;

        argument = one_argument( argument, arg3 );
        argument = one_argument( argument, arg4 );

        int                     chapno = atoi( arg3 );

        if ( chapno < 1 || chapno > MAX_CHAPTERS ) {
            ch_printf( ch, "Chapter range is 1 to %d.\r\n", MAX_CHAPTERS );
            return;
        }

        if ( !str_cmp( arg4, "create" ) ) {
            if ( get_chap_from_quest( chapno, quest ) ) {
                send_to_char( "That chapter already exists!\r\n", ch );
                return;
            }

            if ( !get_chap_from_quest( ( chapno - 1 ), quest ) && chapno > 1 ) {
                ch_printf( ch, "How can you create chapter %d before chapter %d even exists?\r\n",
                           chapno, chapno - 1 );
                return;
            }
            if ( chapno > quest->chapters ) {
                ch_printf( ch, "How can you create chapter %d when there are only %d chapters?\r\n",
                           chapno, quest->chapters );
                send_to_char( "Set more - 'setquest (questname) chapters (number)'.\r\n", ch );
                return;
            }
            CREATE( chap, CHAP_DATA, 1 );
            chap->number = chapno;
            add_chapter( quest, chap );
            send_to_char( "New chapter added.\r\n", ch );
            write_quest_list(  );
            return;
        }

        chap = get_chap_from_quest( chapno, quest );

        if ( !chap ) {
            send_to_char( "No such chapter.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg4, "delete" ) ) {
            free_chapter( quest, chap );
            send_to_char( "Chapter Deleted.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg4, "name" ) ) {
            if ( VLD_STR( chap->desc ) )
                STRFREE( chap->desc );
            if ( VLD_STR( argument ) ) {
                if ( strlen( argument ) > 50 )
                    argument[50] = '\0';
                chap->desc = STRALLOC( argument );
            }
            write_quest_list(  );
            send_to_char( "Done.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg4, "timelimit" ) ) {
            x = atoi( argument );

            if ( x < 0 || x > 3600 ) {
                send_to_char( "Time limit is between 0 and 3600 (one hour).\r\n", ch );
                return;
            }

            chap->timelimit = x;
            send_to_char( "Done.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg4, "kamount" ) ) {
            int                     kamount = atoi( argument );

            if ( kamount < 0 ) {
                send_to_char( "You have to set kamount to 0 or higher.\r\n", ch );
                return;
            }
            chap->kamount = kamount;
            send_to_char( "Done.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg4, "level" ) ) {
            int                     level = atoi( argument );

            if ( level < 0 || level > MAX_LEVEL ) {
                ch_printf( ch, "Level range is between 0 and %d.\r\n", MAX_LEVEL );
                return;
            }

            chap->level = level;
            send_to_char( "Done.\r\n", ch );
            return;
        }
        do_setquest( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "level" ) ) {
        x = atoi( argument );
        if ( x < 1 || x > MAX_LEVEL ) {
            send_to_char( "Quest level must be between 1 and max.\r\n", ch );
            return;
        }
        quest->level = x;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "timelimit" ) ) {
        x = atoi( argument );
        if ( x < 0 || x > 3600 ) {
            send_to_char
                ( "Quest time limit must be between 0 (no timer) and 3600 seconds (1 hour).\r\n",
                  ch );
            return;
        }
        quest->timelimit = x;
        send_to_char( "Done.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "remove" ) ) {
        CHAR_DATA              *victim;
        CHQUEST_DATA           *chquest;

        if ( ( victim = get_char_world( ch, argument ) ) != NULL && !IS_NPC( victim ) ) {
            x = get_number_from_quest( quest );
            for ( chquest = victim->pcdata->first_quest; chquest; chquest = chquest->next ) {
                if ( chquest->questnum != quest->number )
                    continue;
                UNLINK( chquest, victim->pcdata->first_quest, victim->pcdata->last_quest, next,
                        prev );
                DISPOSE( chquest );
                ch_printf( ch, "You remove quest %s from %s.\r\n", quest->name, victim->name );
                ch_printf( victim, "Quest %s has been removed from your journal.\r\n",
                           quest->name );
                return;
            }
            send_to_char( "That player isn't currently on that quest.\r\n", ch );
            return;
        }
        else
            send_to_char( "That player isn't currently online.\r\n", ch );
        return;
    }

    do_setquest( ch, ( char * ) "" );
    return;
}

void do_showquest( CHAR_DATA *ch, char *argument )
{
    char                    arg[MSL],
                            arg2[MSL];
    CHAP_DATA              *chap;
    int                     count = 0;
    QUEST_DATA             *quest;
    int                     x,
                            y;

    if ( !ch || IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( !arg || arg[0] == '\0' ) {
        ch_printf( ch, "&Y%4s %3s %20s %4s %3s %5s %6s %s&D\r\n", "Num", "Lvl", "Name", "Desc",
                   "Cha", "Lmt", "Type", "Vnum" );

        for ( quest = first_quest; quest; quest = quest->next ) {
            count++;
            ch_printf( ch, "&G%4d &R%3d &W%20s %s &R%3d %5d &W%6s &w%d%s&D\r\n", quest->number,
                       quest->level, quest->name, quest->desc ? "   &GY" : "   &RN",
                       quest->chapters, quest->timelimit,
                       ( quest->stype == 0 ) ? "Mobile" : ( quest->stype ==
                                                            1 ) ? "Object" : ( quest->stype ==
                                                                               2 ) ? "Room" :
                       "Unknown", quest->svnum, quest->skipchapters ? " &GCan Skip" : "" );
        }
        ch_printf( ch, "\r\n&G%d &wquests.&D\r\n", count );
        return;
    }

    quest = get_quest_from_name( arg );

    if ( !quest ) {
        send_to_char( "No such quest. Use 'showquest [(questname)] [(chapter #)]'.\r\n", ch );
        return;
    }

    ch_printf( ch, "\r\n&zName:&w %s\r\n", quest->name );
    ch_printf( ch, "&zMin Level:&w %-3d  &zChapters:&w %-3d  &zTime Limit:&w %-5d\r\n",
               quest->level, quest->chapters, quest->timelimit );
    ch_printf( ch, "&zSkipChapters: &w%s\r\n", quest->skipchapters ? "Yes" : "No" );

    if ( !arg2 || arg2[0] == '\0' ) {
        for ( chap = quest->first_chapter; chap; chap = chap->next )
            ch_printf( ch,
                       "&c  - Chapter &C%d&c - Minlev: &C%d &cTLimit: &C%d &cKAmount: &C%d &cName:&C %s\r\n",
                       chap->number, chap->level, chap->timelimit, chap->kamount, chap->desc );
        return;
    }

    if ( !is_number( arg2 ) ) {
        send_to_char( "Use 'showquest [(questname)] [(chapter #)]'.\r\n", ch );
        return;
    }

    chap = get_chap_from_quest( atoi( arg2 ), quest );
    if ( !chap ) {
        send_to_char( "No such chapter to display.\r\n", ch );
        return;
    }

    ch_printf( ch, "&c  - Chapter &C%d&c - Name:&C %s\r\n\r\n&cDESC - &C%s", chap->number,
               chap->desc, chap->bio ? chap->bio : "(not yet entered)" );
}

void do_journal( CHAR_DATA *ch, char *argument )
{
    QUEST_DATA             *quest;
    CHQUEST_DATA           *chquest;
    CHAP_DATA              *chap;
    char                    arg1[MIL],
                            arg2[MIL];
    char                   *chapdesc;
    int                     x = 0,
        total = 0,
        cando = 0,
        done = 0,
        num = 0,
        progress = 0,
        avail = 0;
    bool                    found = FALSE,
        completed = FALSE;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( VLD_STR( arg1 ) && !str_cmp( arg1, "completed" ) ) {
        completed = TRUE;
        argument = one_argument( argument, arg1 );
    }

    argument = one_argument( argument, arg2 );

    if ( !arg1 || arg1[0] == '\0' ) {
        char                    questtime[MSL] = "";
        char                    chaptime[MSL] = "";
        char                    minsecs[MSL] = "";
        int                     time = 0,
            mins = 0,
            secs = 0;

        ch_printf( ch, "\r\n&Y6 Dragons &CQuest Journal for %s&D\r\n\r\n", ch->name );
        send_to_char( "&cLvl           Quest Name Chapter Quest Progress&D\r\n", ch );

        for ( quest = first_quest; quest; quest = quest->next ) {
            num = quest->number;
            total++;
            if ( ch->level < quest->level )
                continue;
            cando++;

            for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
                if ( chquest->questnum == quest->number ) {
                    progress = chquest->progress;
                    break;
                }
            }
            if ( !progress || !chquest )
                continue;

            chap = get_chap_from_quest( progress, quest );

            if ( !completed ) {
                if ( progress > quest->chapters ) {
                    done++;
                    continue;
                }

                if ( !chap )
                    continue;

                if ( chap && chap->desc )
                    chapdesc = chap->desc;
                else
                    chapdesc = ( char * ) "NULL: No desc for this chapter.";
            }
            else {                                     /* Only show completed quest */

                if ( progress > quest->chapters ) {
                    chapdesc = ( char * ) "Quest Finished!";
                    done++;
                }
                else
                    continue;
            }

            avail++;

            if ( ch->level == quest->level )
                send_to_char( "&Y", ch );
            else if ( quest->level >= ( ch->level - 3 ) )
                send_to_char( "&G", ch );
            else
                send_to_char( "&z", ch );
            ch_printf( ch, "%3d %20s %7d %s", quest->level, capitalize( quest->name ), progress,
                       chapdesc );
            if ( chquest->kamount > 0 )
                ch_printf( ch, "&R(%d)&D", chquest->kamount );
            send_to_char( "\r\n", ch );
            if ( chquest->chaplimit > 0 )
                ch_printf( ch, "You have %s remaining to finish the above chapter.\r\n",
                           show_timeleft( chquest->chaplimit ) );
            if ( chquest->questlimit > 0 )
                ch_printf( ch, "You have %s remaining to finish the above quest.\r\n",
                           show_timeleft( chquest->questlimit ) );
        }

        if ( avail == 0 && done == 0 )
            send_to_char( "&BYou haven't yet found any quests! Go explore!&D", ch );

        send_to_char( "\r\n\r\n", ch );

        if ( IS_IMMORTAL( ch ) )
            ch_printf( ch, "&cQuests Made:  &C%d\r\n", total );
        ch_printf( ch, "&cQuests Found: &C%d    &cQuests Finished: &C%d&D\r\n", avail, done );
        send_to_char( "\r\n&GMore quest info: &WJournal <quest name>&D\r\n", ch );
        send_to_char( "\r\n&GTo see completed quest: &WJournal completed&D\r\n", ch );
        return;
    }

    quest = get_quest_from_name( arg1 );
    if ( !quest ) {
        send_to_char( "&GProper Syntax : journal\r\n                journal <quest name>&D\r\n",
                      ch );
        return;
    }

    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questnum != quest->number )
            continue;
        send_to_char( "&GReading the quest description...\r\n", ch );
        pager_printf( ch, "&W%s&D\r\n", quest->desc ? quest->desc : "(Not created!)" );
        found = TRUE;
        break;
    }

    if ( !found )
        send_to_char( "That is not a quest in your journal!\r\n", ch );
}

bool remove_chquest( CHAR_DATA *ch, QUEST_DATA * quest )
{
    CHQUEST_DATA           *chquest;

    if ( !ch || !quest || !ch->pcdata )
        return FALSE;

    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questnum == quest->number ) {
            UNLINK( chquest, ch->pcdata->first_quest, ch->pcdata->last_quest, next, prev );
            DISPOSE( chquest );
            return TRUE;
        }
    }

    return FALSE;
}

void link_chquest( CHAR_DATA *ch, CHQUEST_DATA * chquest )
{
    CHQUEST_DATA           *tmp;

    if ( !chquest || !ch || !ch->pcdata )
        return;

    for ( tmp = ch->pcdata->first_quest; tmp; tmp = tmp->next ) {
        if ( chquest->questnum < tmp->questnum ) {
            INSERT( chquest, tmp, ch->pcdata->first_quest, next, prev );
            return;
        }
    }
    LINK( chquest, ch->pcdata->first_quest, ch->pcdata->last_quest, next, prev );
}

char                   *show_timeleft( int time )
{
    static char             buf[MSL];
    int                     mins,
                            seconds;

    mins = ( time / 60 );
    seconds = ( time - ( mins * 60 ) );
    snprintf( buf, sizeof( MSL ), "%s", "" );
    if ( mins > 0 ) {
        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%d minute%s ", mins,
                  mins == 1 ? "" : "s" );
        if ( seconds > 0 )
            snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%s", "and " );
    }
    if ( seconds > 0 )
        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ), "%d second%s", seconds,
                  seconds == 1 ? "" : "s" );
    return buf;
}

/* Used to update where people are in the quest */
void update_chquest( CHAR_DATA *ch, QUEST_DATA * quest, int nchapter )
{
    CHQUEST_DATA           *chquest;
    CHAP_DATA              *chapter;
    int                     mins,
                            seconds;
    bool                    skipped = FALSE;

    if ( !quest || !ch || !ch->pcdata )
        return;

    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questnum == quest->number )
            break;
    }

    /*
     * If no quest need to start it for them 
     */
    if ( !chquest ) {
        if ( nchapter != 1 ) {
            send_to_char( "&GYou have to do the quest in order. Check your &WJOURNAL&D.\r\n", ch );
            return;
        }
        chapter = get_chap_from_quest( 1, quest );
        if ( !chapter ) {
            send_to_char
                ( "For now this is all you can do on this quest. Try back another time.\r\n", ch );
            return;
        }
        CREATE( chquest, CHQUEST_DATA, 1 );
        chquest->questnum = quest->number;
        chquest->questlimit = quest->timelimit;
        chquest->progress = 1;
        chquest->times = 1;
        chquest->kamount = 0;
        chquest->chaplimit = chapter->timelimit;
        link_chquest( ch, chquest );
        send_to_char( "\r\n&GYou have started a new quest!&D\r\n", ch );

        if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
            send_to_char( "!!SOUND(sound/quest.wav)\r\n", ch );

        send_to_char( "&GType &WJOURNAL &Gto see the quests you have to date.&D\r\n", ch );
        if ( chquest->chaplimit > 0 )
            ch_printf( ch, "&RYou have %s left to finish this chapter.&D\r\n",
                       show_timeleft( chquest->chaplimit ) );
        if ( chquest->questlimit > 0 )
            ch_printf( ch, "&RYou have %s left to finish this quest.&D\r\n",
                       show_timeleft( chquest->questlimit ) );
        return;
    }

    if ( !quest->skipchapters ) {
        if ( nchapter != ( chquest->progress + 1 ) ) {
            send_to_char( "&GYou have to do the quest in order. Check your &WJOURNAL&D.\r\n", ch );
            return;
        }

        /*
         * Ok so we have the chquest so lets increase it 
         */
        ++chquest->progress;
    }
    else {
        chquest->progress = nchapter;                  /* If we skip need to set it to
                                                        * the new chapter */
        skipped = TRUE;
    }

    /*
     * Ok redoing it? 
     */
    if ( chquest->progress == 1 ) {
        int                     times = chquest->times;

        chapter = get_chap_from_quest( 1, quest );
        if ( !chapter ) {
            send_to_char
                ( "For now this is all you can do on this quest. Try back another time.\r\n", ch );
            return;
        }

        chquest->questlimit = quest->timelimit;
        chquest->chaplimit = chapter->timelimit;
        chquest->kamount = 0;
        send_to_char( "\r\n&GYou have started a new quest!&D\r\n", ch );
        send_to_char( "&GType &WJOURNAL &Gto see the quests you have to date.&D\r\n", ch );
        if ( chquest->chaplimit > 0 )
            ch_printf( ch, "&RYou have %s left to finish this chapter.&D\r\n",
                       show_timeleft( chquest->chaplimit ) );
        if ( chquest->questlimit > 0 )
            ch_printf( ch, "&RYou have %s left to finish this quest.&D\r\n",
                       show_timeleft( chquest->questlimit ) );
        ch_printf( ch, "&BYou have already attempted this quest %d time%s&D\r\n", times,
                   times >= 5 ? "s!" : times == 1 ? "s." : "." );
        chquest->times++;
        return;
    }

    /*
     * Was this the end of the quest? 
     */
    if ( chquest->progress == ( quest->chapters + 1 ) ) {
        send_to_char( "&GYou have finished your quest!&D\r\n", ch );
        gain_exp( ch, 0 + ch->level * 100 + quest->level * 2000 );
        if ( ch->pcdata->clan ) {
            CLAN_DATA              *clan;

            clan = ch->pcdata->clan;
            ch->pcdata->clanpoints += 1;
            clan->totalpoints += 1;
            ch_printf( ch,
                       "&G%s clan has gained 1 point from your quest, now totaling %d clan status points!\r\n",
                       clan->name, clan->totalpoints );
            save_clan( clan );
        }

        if ( chquest->questlimit )
            ch_printf( ch, "&RYou had %s left when you completed the quest.&D\r\n",
                       show_timeleft( chquest->questlimit ) );
        chquest->chaplimit = 0;
        chquest->questlimit = 0;
        chquest->kamount = 0;
        return;
    }

    chapter = get_chap_from_quest( chquest->progress, quest );
    if ( !chapter ) {
        send_to_char( "For now this is all you can do on this quest. Try back another time.\r\n",
                      ch );
        return;
    }

    chquest->chaplimit = chapter->timelimit;
    chquest->kamount = 0;

    /*
     * If they aren't skipping show this message 
     */
//   if( !quest->skipchapters && skipped )
    send_to_char( "&GYou continue the quest, type &WJOURNAL &Gto see the update.&D\r\n", ch );

    if ( chquest->chaplimit > 0 )
        ch_printf( ch, "&RYou have %s left to finish this chapter.&D\r\n",
                   show_timeleft( chquest->chaplimit ) );
    if ( chquest->questlimit > 0 )
        ch_printf( ch, "&RYou have %s left to finish this quest.&D\r\n",
                   show_timeleft( chquest->questlimit ) );
}

int get_chapter( CHAR_DATA *ch, QUEST_DATA * quest )
{
    CHQUEST_DATA           *chquest;

    if ( !ch || !ch->pcdata || !quest )
        return 0;
    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questnum == quest->number )
            return chquest->progress;
    }

    return 0;
}

int get_chkamount( CHAR_DATA *ch, QUEST_DATA * quest )
{
    CHQUEST_DATA           *chquest;

    if ( !ch || !ch->pcdata || !quest )
        return -1;
    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questnum == quest->number )
            return chquest->kamount;
    }

    return -1;
}

/* The kamount is set to 0 and worked up */
void update_chquest_kamount( CHAR_DATA *ch, QUEST_DATA * quest, int nchapter )
{
    CHQUEST_DATA           *chquest;
    CHAP_DATA              *chapter;
    int                     mins,
                            seconds;

    if ( !quest || !ch || !ch->pcdata )
        return;

    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questnum == quest->number )
            break;
    }

    if ( !chquest ) {
        send_to_char( "You aren't currently on that quest.\r\n", ch );
        return;
    }

    /*
     * Only update if we are on the right chapter 
     */
    if ( nchapter != chquest->progress ) {
        send_to_char( "&GYou have to do the quest in order. Check your &WJOURNAL&D.\r\n", ch );
        return;
    }

    ++chquest->kamount;
}
