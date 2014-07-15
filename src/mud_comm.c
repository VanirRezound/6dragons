/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - MOR programs module based on mob program code by N'Atas-h             *
 ***************************************************************************/

#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/hometowns.h"
#include "h/files.h"
#include "h/languages.h"
#include "h/polymorph.h"
#include "h/clans.h"                                   // For clans..
ch_ret                  simple_damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt );

#define NEW_AUTH(ch)         (!IS_NPC(ch) && (ch)->level == 1)
bool                    remove_chquest( CHAR_DATA *ch, QUEST_DATA * quest );
int get_npc_race        args( ( char *type ) );
int get_actflag         args( ( char *flag ) );
int get_risflag         args( ( char *flag ) );
int get_partflag        args( ( char *flag ) );
int get_attackflag      args( ( char *flag ) );
int get_defenseflag     args( ( char *flag ) );
int get_langflag        args( ( char *flag ) );
int get_langnum         args( ( char *flag ) );
int get_trigflag        args( ( char *flag ) );
extern int              top_affect;
void                    to_channel( const char *argument, const char *xchannel, int level );
CHAP_DATA              *get_chap_from_quest( int x, QUEST_DATA * quest );
void                    update_chquest( CHAR_DATA *ch, QUEST_DATA * quest, int nchapter );
bool                    DONT_UPPER;
void                    check_clan_leaders( CLAN_DATA * clan, char *player );

extern OBJ_DATA        *dragonegg;
extern int              arena_population;
void                    end_game( bool silent );

const char             *mprog_type_to_name( int type )
{
    switch ( type ) {
        case IN_FILE_PROG:
            return "in_file_prog";
        case ACT_PROG:
            return "act_prog";
        case SPEECH_PROG:
            return "speech_prog";
        case RAND_PROG:
            return "rand_prog";
        case FIGHT_PROG:
            return "fight_prog";
        case HITPRCNT_PROG:
            return "hitprcnt_prog";
        case DEATH_PROG:
            return "death_prog";
        case ENTRY_PROG:
            return "entry_prog";
        case GREET_PROG:
            return "greet_prog";
        case ALL_GREET_PROG:
            return "all_greet_prog";
        case GIVE_PROG:
            return "give_prog";
        case BRIBE_PROG:
            return "bribe_prog";
        case HOUR_PROG:
            return "hour_prog";
        case TIME_PROG:
            return "time_prog";
        case WEAR_PROG:
            return "wear_prog";
        case REMOVE_PROG:
            return "remove_prog";
        case TRASH_PROG:
            return "trash_prog";
        case LOOK_PROG:
            return "look_prog";
        case EXA_PROG:
            return "exa_prog";
        case ZAP_PROG:
            return "zap_prog";
        case GET_PROG:
            return "get_prog";
        case DROP_PROG:
            return "drop_prog";
        case REPAIR_PROG:
            return "repair_prog";
        case DAMAGE_PROG:
            return "damage_prog";
        case PULL_PROG:
            return "pull_prog";
        case PUSH_PROG:
            return "push_prog";
        case SCRIPT_PROG:
            return "script_prog";
        case SLEEP_PROG:
            return "sleep_prog";
        case REST_PROG:
            return "rest_prog";
        case LEAVE_PROG:
            return "leave_prog";
        case USE_PROG:
            return "use_prog";
        case PRE_ENTER_PROG:
            return "pre_enter_prog";
        default:
            return "ERROR_PROG";
    }
}

/* A trivial rehack of do_mstat.  This doesnt show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MUDprograms which are set.
 */

void do_mpstat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    MPROG_DATA             *mprg;
    CHAR_DATA              *victim;
    int                     cnt = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "MProg stat whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) ) {
        send_to_char( "Only Mobiles can have MobPrograms!\r\n", ch );
        return;
    }
    if ( get_trust( ch ) < LEVEL_AJ_SGT && xIS_SET( victim->act, ACT_STATSHIELD ) ) {
        set_pager_color( AT_IMMORT, ch );
        send_to_pager( "Their godly glow prevents you from getting a good look.\r\n", ch );
        return;
    }
    if ( xIS_EMPTY( victim->pIndexData->progtypes ) ) {
        send_to_char( "That Mobile has no Programs set.\r\n", ch );
        return;
    }

    ch_printf( ch, "Name: %s.  Vnum: %d.\r\n", victim->name, victim->pIndexData->vnum );

    ch_printf( ch, "Short description: %s.\r\nLong  description: %s", victim->short_descr,
               victim->long_descr[0] != '\0' ? victim->long_descr : "(none).\r\n" );

    ch_printf( ch, "Hp: %d/%d.  Mana: %d/%d.  Move: %d/%d. \r\n", victim->hit, victim->max_hit,
               victim->mana, victim->max_mana, victim->move, victim->max_move );

    ch_printf( ch, "Lv: %d.  Class: %d.  Align: %d.  AC: %d.  Exp: %d.\r\n", victim->level,
               victim->Class, victim->alignment, GET_AC( victim ), victim->exp );

    for ( mprg = victim->pIndexData->mudprogs; mprg; mprg = mprg->next )
        ch_printf( ch, "%d> %s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprg->type ),
                   mprg->arglist, mprg->comlist );
    return;
}

/* Opstat - Scryn 8/12*/
void do_opstat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    MPROG_DATA             *mprg;
    OBJ_DATA               *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "OProg stat what?\r\n", ch );
        return;
    }

    if ( ( obj = get_obj_world( ch, arg ) ) == NULL ) {
        send_to_char( "You cannot find that.\r\n", ch );
        return;
    }

    if ( xIS_EMPTY( obj->pIndexData->progtypes ) ) {
        send_to_char( "That object has no programs set.\r\n", ch );
        return;
    }

    ch_printf( ch, "Name: %s.  Vnum: %d.\r\n", obj->name, obj->pIndexData->vnum );

    ch_printf( ch, "Short description: %s.\r\n", obj->short_descr );

    for ( mprg = obj->pIndexData->mudprogs; mprg; mprg = mprg->next )
        ch_printf( ch, ">%s %s\r\n%s\r\n", mprog_type_to_name( mprg->type ), mprg->arglist,
                   mprg->comlist );

    return;

}

/* Rpstat - Scryn 8/12 */
void do_rpstat( CHAR_DATA *ch, char *argument )
{
    MPROG_DATA             *mprg;

    if ( xIS_EMPTY( ch->in_room->progtypes ) ) {
        send_to_char( "This room has no programs set.\r\n", ch );
        return;
    }

    ch_printf( ch, "Name: %s.  Vnum: %d.\r\n", ch->in_room->name, ch->in_room->vnum );

    for ( mprg = ch->in_room->mudprogs; mprg; mprg = mprg->next )
        ch_printf( ch, ">%s %s\r\n%s\r\n", mprog_type_to_name( mprg->type ), mprg->arglist,
                   mprg->comlist );
    return;
}

void do_mpclanmob( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *imob = NULL;
    CHAR_DATA              *mob = NULL;
    int                     vnum = -1;
    CHAR_DATA              *victim;
    char                    arg[MSL];
    CLAN_DATA              *clan;

    argument = one_argument( argument, arg );

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !*arg ) {
        progbug( ch, "Mpgenmob: missing victim" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        progbug( ch, "Mpclanmob: Target not in the room" );
        return;
    }

    if ( IS_NPC( victim ) || IS_IMMORTAL( victim ) )
        return;

    clan = victim->pcdata->clan;

    if ( !str_cmp( victim->pcdata->clan_name, "alliance" ) )
        vnum = number_range( 41021, 41025 );
    else
        vnum = number_range( 41026, 41029 );

    if ( vnum == -1 )
        return;
    imob = get_mob_index( vnum );

    if ( !imob ) {
        snprintf( log_buf, MAX_STRING_LENGTH, "mpclanmob: Missing mob for vnum %d", vnum );
        log_string( log_buf );
        return;
    }

    mob = create_mobile( imob );
    mob->timer = 2;
    mob->level = victim->level + 5;
    mob->hit = set_hp( mob->level );
    mob->max_hit = set_hp( mob->level );
    mob->armor = set_armor_class( mob->level );
    mob->hitroll = set_hitroll( mob->level );
    mob->damroll = set_damroll( mob->level );
    mob->numattacks = set_num_attacks( mob->level );
    mob->hitplus = set_hp( mob->level );
    if ( victim->level < 4 )
        mob->hit = 25;
    start_hating( mob, victim );
    start_hunting( mob, victim );
    char_to_room( mob, victim->in_room );
    set_fighting( mob, victim );
    global_retcode = multi_hit( mob, victim, TYPE_UNDEFINED );
    do_bash( mob, ( char * ) "" );
    return;
}

void do_mpgenmob( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *imob = NULL;
    CHAR_DATA              *mob = NULL;
    int                     vnum = -1;
    CHAR_DATA              *victim;
    char                    arg[MSL];

    argument = one_argument( argument, arg );

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !*arg ) {
        progbug( ch, "Mpgenmob: missing victim" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        progbug( ch, "Mpgenmob: Target not in the room" );
        return;
    }

    if ( IS_NPC( victim ) || IS_IMMORTAL( victim ) )
        return;

    vnum = number_range( 41000, 41020 );

    if ( vnum == -1 )
        return;
    imob = get_mob_index( vnum );

    if ( !imob ) {
        snprintf( log_buf, MAX_STRING_LENGTH, "mpgenmob: Missing mob for vnum %d", vnum );
        log_string( log_buf );
        return;
    }
    mob = create_mobile( imob );
    mob->timer = 2;
    mob->level = victim->level;
    mob->hit = set_hp( mob->level );
    mob->max_hit = set_hp( mob->level );
    mob->armor = set_armor_class( mob->level );
    mob->hitroll = set_hitroll( mob->level );
    mob->damroll = set_damroll( mob->level );
    mob->numattacks = set_num_attacks( mob->level );
    mob->hitplus = set_hp( mob->level );
    if ( victim->level < 4 )
        mob->hit = 25;
    start_hating( mob, victim );
    start_hunting( mob, victim );
    char_to_room( mob, victim->in_room );
    set_fighting( mob, victim );
    global_retcode = multi_hit( mob, victim, TYPE_UNDEFINED );
    do_bash( mob, ( char * ) "" );
    return;
}

void do_mptutorial( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MSL];

    argument = one_argument( argument, arg );

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !*arg ) {
        // progbug( ch, "Mptutorial: missing victim" );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        // progbug( ch, "Mptutorial: Target not in the room" );
        return;
    }

    if ( !IS_NPC( victim ) ) {
        if ( xIS_SET( victim->act, PLR_TUTORIAL ) ) {
            xREMOVE_BIT( victim->act, PLR_TUTORIAL );
            // Volk - cancel mptut after 30 seconds!
            victim->pcdata->tutorialtimer = 0;
        }
        else if ( victim->level > 99 ) {
            xSET_BIT( victim->act, PLR_TUTORIAL );
            victim->pcdata->tutorialtimer = 5;
            return;
        }
        else if ( ( victim->pcdata->textspeed > 8 ) && ( !xIS_SET( victim->act, PLR_TUTORIAL ) ) ) {
            xSET_BIT( victim->act, PLR_TUTORIAL );
            victim->pcdata->tutorialtimer = 12;
            return;
        }
        else if ( victim->pcdata->textspeed > 6 && victim->pcdata->textspeed < 9
                  && ( !xIS_SET( victim->act, PLR_TUTORIAL ) ) ) {
            xSET_BIT( victim->act, PLR_TUTORIAL );
            victim->pcdata->tutorialtimer = 20;
            return;
        }
        else if ( !xIS_SET( victim->act, PLR_TUTORIAL ) ) {
            xSET_BIT( victim->act, PLR_TUTORIAL );
            victim->pcdata->tutorialtimer = 26;
        }
    }
}

void do_mppersonalize( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    CHAR_DATA              *victim;
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg1[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        error( ch );
        return;
    }

    if ( !*arg ) {
        progbug( ch, "MpPersonalize: no args" );
        return;
    }

    if ( !*arg1 ) {
        progbug( ch, "MpPersonalize: missing arg" );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL ) {
        progbug( ch, "MpPersonalize: no object" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        progbug( ch, "MpPersonalize: Target not in world" );
        return;
    }
    else if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        progbug( ch, "MpPersonalize: Target not in the room" );
        return;
    }

    if ( obj->owner )
        STRFREE( obj->owner );

    obj->owner = STRALLOC( victim->name );
    return;
}

/* Woowoo - Blodkai, November 1997 */
void do_mpasupress( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    int                     rnds;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Mpasupress who?\r\n", ch );
        progbug( ch, "Mpasupress:  invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    if ( arg2[0] == '\0' ) {
        send_to_char( "Supress their attacks for how many rounds?\r\n", ch );
        progbug( ch, "Mpasupress:  invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "No such victim in the room.\r\n", ch );
        progbug( ch, "Mpasupress:  victim not present [%s]", ch->name );
        return;
    }
    rnds = atoi( arg2 );
    if ( rnds < 0 || rnds > 32767 ) {
        send_to_char( "Invalid number of rounds to supress attacks.\r\n", ch );
        progbug( ch, "Mpsupress:  invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    add_timer( victim, TIMER_ASUPRESSED, rnds, NULL, 0 );
    return;
}

/* lets the mobile kill any player or mobile without murder*/
void do_mpkill( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }
    if ( !ch ) {
        bug( "%s", "Nonexistent ch in do_mpkill!" );
        return;
    }
    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        progbug( ch, "MpKill - no argument [%s]", ch->name );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpkill [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( victim == ch ) {
        progbug( ch, "MpKill - Bad victim to attack [%s]", ch->name );
        return;
    }
    if ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
         || ch->position == POS_BERSERK )
        return;

    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   items using all.xxxxx or just plain all of them */

void do_mpjunk( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpjunk - No argument [%s]", ch->name );
        return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) ) {
        if ( ( obj = get_obj_wear( ch, arg ) ) != NULL ) {
            unequip_char( ch, obj );
            extract_obj( obj );
            return;
        }
        if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
            return;
        extract_obj( obj );
    }
    else
        for ( obj = ch->first_carrying; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( arg[3] == '\0' || is_name( &arg[4], obj->name ) ) {
                if ( obj->wear_loc != WEAR_NONE )
                    unequip_char( ch, obj );
                extract_obj( obj );
            }
        }

    return;

}

/*
 * This function examines a text string to see if the first "word" is a
 * color indicator (e.g. _red, _whi_, _blu).  -  Gorog
 */
int get_color( char *argument )
{                                                      /* get color code from command *
                                                        * string */
    char                    color[MIL];
    const char             *cptr;
    static char const      *color_list =
        "_bla_red_dgr_bro_dbl_pur_cya_cha_dch_ora_gre_yel_blu_pin_lbl_whi";
    static char const      *blink_list =
        "*bla*red*dgr*bro*dbl*pur*cya*cha*dch*ora*gre*yel*blu*pin*lbl*whi";

    one_argument( argument, color );
    if ( color[0] != '_' && color[0] != '*' )
        return 0;
    if ( ( cptr = strstr( color_list, color ) ) )
        return ( cptr - color_list ) / 4;
    if ( ( cptr = strstr( blink_list, color ) ) )
        return ( cptr - blink_list ) / 4 + AT_BLINK;
    return 0;
}

/* Prints the argument to all the rooms around the mobile */
void do_mpasound( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    ROOM_INDEX_DATA        *was_in_room;
    EXIT_DATA              *pexit;
    short                   color;
    EXT_BV                  actflags;

    if ( !ch ) {
        bug( "%s", "Nonexistent ch in do_mpasound!" );
        return;
    }
    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        progbug( ch, "Mpasound - No argument [%s]", ch->name );
        return;
    }
    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    if ( ( color = get_color( argument ) ) )
        argument = one_argument( argument, arg1 );
    was_in_room = ch->in_room;
    for ( pexit = was_in_room->first_exit; pexit; pexit = pexit->next ) {
        if ( pexit->to_room && pexit->to_room != was_in_room ) {
            ch->in_room = pexit->to_room;
            MOBtrigger = FALSE;
            if ( color )
                act( color, argument, ch, NULL, NULL, TO_ROOM );
            else
                act( AT_SAY, argument, ch, NULL, NULL, TO_ROOM );
        }
    }
    ch->act = actflags;
    ch->in_room = was_in_room;
    return;
}

/* prints the message to all in the room other than the mob and victim */
void do_mpechoaround( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;
    short                   color;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }
    argument = one_argument( argument, arg );
    if ( !VLD_STR( arg ) ) {
        progbug( ch, "Mpechoaround - No argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpechoaround [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    /*
     * DONT_UPPER prevents argument[0] from being captilized. --Shaddai 
     */
    DONT_UPPER = TRUE;
    if ( ( color = get_color( argument ) ) ) {
        argument = one_argument( argument, arg );
        act( color, argument, ch, NULL, victim, TO_NOTVICT );
    }
    else
        act( AT_ACTION, argument, ch, NULL, victim, TO_NOTVICT );

    DONT_UPPER = FALSE;                                /* Always set it back to false */
    ch->act = actflags;
}

//Status: Completed mpcommand. -Taon
// Volk: Updated to work with a timer.
void do_mpoutcast( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    CLAN_DATA              *clan;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        progbug( ch, "MpOutcast - Invalid argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "MpOutcast [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    clan = victim->pcdata->clan;

    if ( !clan ) {
        progbug( ch, "MpOutcast - Victim isnt in a clan. [%s]", victim->name );
        return;
    }

/* Volk: Need to put support in now that the victim has left the clan
 * to make sure they can't turn around and join another clan (ie for
 * looting). Set their pcdata->clan_timer */

    victim->pcdata->clan_timer = current_time;

    if ( clan->members > 0 )
        --clan->members;
    remove_roster( victim->pcdata->clan, victim->name );
    --victim->pcdata->clan->members;
    victim->pcdata->clan = NULL;
    STRFREE( victim->pcdata->clan_name );
    victim->pcdata->clan_name = STRALLOC( "" );
    send_to_char( "You've been stamped an outcast by your clan.\r\n", victim );
    QUEST_DATA             *quest;
    short                   x;

    quest = get_quest_from_name( clan->name );
    remove_chquest( victim, quest );
    save_char_obj( victim );
    save_clan( clan );
    check_clan_leaders( clan, victim->name );
}

extern bool             first_tutorial_room;
extern bool             second_tutorial_room;
extern bool             third_tutorial_room;
extern bool             first_etutorial_room;
extern bool             second_etutorial_room;
extern bool             third_etutorial_room;
extern bool             first_dtutorial_room;
extern bool             second_dtutorial_room;
extern bool             third_dtutorial_room;

//Clear the tutorial rooms properly, making way for the next
//newbie to enter the game. -Taon

void do_mpclear( CHAR_DATA *ch, char *argument )
{

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        error( ch );
        return;
    }

    if ( ch->in_room->vnum == ROOM_AUTH_START )
        first_tutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START2 )
        second_tutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START3 )
        third_tutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START4 )
        first_etutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START5 )
        second_etutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START6 )
        third_etutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START7 )
        first_dtutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START8 )
        second_dtutorial_room = FALSE;
    else if ( ch->in_room->vnum == ROOM_AUTH_START9 )
        third_dtutorial_room = FALSE;
    else
        progbug( ch, "Mpclear - Bad mob, [%s]! Bad arg, [%s]! Invalid room [%d]", ch->name,
                 argument, ch->in_room->vnum );

    return;
}

//This mpcommand allows a mob to assist the player "arg\victim" in combat
//against the target.
//Author: Taon "Dustan Gunn"
void do_mpassist( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *target;
    char                    arg[MIL];

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "MpAssist - Invalid argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        progbug( ch, "MpAssist [%d]- victim [%s] does not exist",
                 ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( victim->fighting ) {
        target = who_fighting( victim );

        if ( !IS_NPC( target ) )
            ch_printf( victim, "%s takes up a fighting position and attacks %s.\r\n",
                       ch->short_descr, target->name );
        else
            ch_printf( victim, "%s takes up a fighting position and attacks %s.\r\n",
                       ch->short_descr, target->short_descr );

        if ( !IS_NPC( victim ) )
            ch_printf( target,
                       "%s takes up a fighting position and defends %s by attacking you .\r\n",
                       ch->short_descr, victim->name );
        else
            ch_printf( target,
                       "%s takes up a fighting position and defends %s by attacking you .\r\n",
                       ch->short_descr, victim->short_descr );

        set_fighting( ch, target );
    }
}

void do_mptrade( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    arg1[MIL];

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mptrade - Invalid argument [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mptrade [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    if ( victim->race == RACE_DRAGON || victim->Class == CLASS_DRAGONLORD ) {
        act( AT_CYAN, "The city manager is too terrified of $n to grant that.", victim, NULL, ch,
             TO_ROOM );
        send_to_char( "&cThe city manager is too terrified of you to grant that.\r\n", victim );
        return;
    }

    if ( !str_cmp( arg1, "trade" ) ) {
        QUEST_DATA             *quest;

        quest = get_quest_from_name( arg1 );
        remove_chquest( victim, quest );
        victim->pcdata->tradeclass = 0;
        victim->pcdata->tradelevel = 0;
        if ( victim->pcdata->learned[gsn_mine] > 0 )
            victim->pcdata->learned[gsn_mine] = 0;
        if ( victim->pcdata->learned[gsn_gather] > 0 )
            victim->pcdata->learned[gsn_gather] = 0;
        if ( victim->pcdata->learned[gsn_forge] > 0 )
            victim->pcdata->learned[gsn_forge] = 0;
        if ( victim->pcdata->learned[gsn_bake] > 0 )
            victim->pcdata->learned[gsn_bake] = 0;
        if ( victim->pcdata->learned[gsn_mix] > 0 )
            victim->pcdata->learned[gsn_mix] = 0;
        if ( victim->pcdata->learned[gsn_hunt] > 0 )
            victim->pcdata->learned[gsn_hunt] = 0;
        if ( victim->pcdata->learned[gsn_tan] > 0 )
            victim->pcdata->learned[gsn_tan] = 0;

        act( AT_CYAN, "The city manager rips up $n's apprenticeship card.", victim, NULL, ch,
             TO_ROOM );
        send_to_char( "&cThe city manager rips up your apprenticeship card.\r\n", victim );
    }
    else if ( !str_cmp( arg1, "blacksmith" ) ) {
        if ( victim->pcdata->tradeclass > 0 )
            return;
        act( AT_CYAN, "The city manager signs off on $n's apprenticeship card.", victim, NULL, ch,
             TO_ROOM );
        act( AT_CYAN, "The city manager signs off on your apprenticeship card.", ch, NULL, NULL,
             TO_CHAR );
        victim->pcdata->tradeclass = 20;
        victim->pcdata->tradelevel = 1;
        victim->pcdata->learned[gsn_mine] = 10;
        victim->pcdata->learned[gsn_forge] = 10;
        save_char_obj( victim );
    }
    else if ( !str_cmp( arg1, "baker" ) ) {
        if ( victim->pcdata->tradeclass > 0 )
            return;
        act( AT_CYAN, "The city manager signs off on $n's apprenticeship card.", victim, NULL, ch,
             TO_ROOM );
        act( AT_CYAN, "The city manager signs off on your apprenticeship card.", ch, NULL, NULL,
             TO_CHAR );
        victim->pcdata->tradeclass = 21;
        victim->pcdata->tradelevel = 1;
        victim->pcdata->learned[gsn_gather] = 10;
        victim->pcdata->learned[gsn_bake] = 10;
        victim->pcdata->learned[gsn_mix] = 10;
        save_char_obj( victim );
    }
    else if ( !str_cmp( arg1, "tanner" ) ) {
        if ( victim->pcdata->tradeclass > 0 )
            return;
        act( AT_CYAN, "The city manager signs off on $n's apprenticeship card.", victim, NULL, ch,
             TO_ROOM );
        act( AT_CYAN, "The city manager signs off on your apprenticeship card.", ch, NULL, NULL,
             TO_CHAR );
        victim->pcdata->tradeclass = 22;
        victim->pcdata->tradelevel = 1;
        victim->pcdata->learned[gsn_hunt] = 10;
        victim->pcdata->learned[gsn_tan] = 10;
        save_char_obj( victim );
    }
}

void do_mpboat( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpboat - Invalid argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpboat [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( xIS_SET( victim->act, PLR_BOAT ) ) {
        xREMOVE_BIT( victim->act, PLR_BOAT );
        return;
    }
    else
        xSET_BIT( victim->act, PLR_BOAT );
    return;
}

//mpinduct, so certain mobs can induct players in clans.
//Status: Completed mpcommand. -Taon
void do_mpinduct( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    CLAN_DATA              *clan;
    char                    arg[MIL];

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "MpInduct - Invalid argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpinduct [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( victim->pcdata->clan != NULL ) {
        progbug( ch, "MpInduct - Target already in a clan [%s]", victim->name );
        return;
    }
    if ( IS_NPC( victim ) ) {
        progbug( ch, "MpInduct - Attempted to target a mob [%s]", victim->short_descr );
        return;
    }
    if ( victim->race == RACE_DRAGON || victim->Class == CLASS_DRAGONLORD ) {
        send_to_char
            ( "You cannot join that quest for a clan, your primal instincts demand you stay aloof.\r\n",
              victim );
        return;
    }

    int                     timer = current_time - victim->pcdata->clan_timer;

    if ( timer > 0 )                                   // Volk: This means they left a
        // clan recently!
    {
        /*
         * Volk: Means they haven't waited long enough! 
         */
        if ( timer < ( sysdata.clan_timer * 86400 ) ) {
            timer = ( sysdata.clan_timer * 86400 ) - timer;
            int                     days = ( timer / 86400 ) >= 1 ? ( timer / 86400 ) : 0;

            timer -= ( days * 86400 );
            int                     hours = ( timer / 3600 ) >= 1 ? ( timer / 3600 ) : 0;

            timer -= ( hours * 3600 );
            int                     mins = ( timer / 60 ) >= 1 ? ( timer / 60 ) : 0;

            timer -= ( mins * 60 );

            if ( days > 0 )
                ch_printf( victim,
                           "Outcast, you have %d days, %d hours and %d minutes until you can join another clan!\r\n",
                           days, hours, mins );
            else if ( hours > 0 )
                ch_printf( victim,
                           "Outcast, you have %d hours and %d minutes until you can join another clan!\r\n",
                           hours, mins );
            else if ( mins > 0 )
                ch_printf( victim,
                           "Outcast, you have %d mins and %d seconds until you can join another clan!\r\n",
                           mins, timer );
            else if ( timer > 0 )
                ch_printf( victim,
                           "Outcast, you have %d seconds until you can join another clan!\r\n",
                           timer );
            else
                progbug( ch, "MpInduct - Target [%s] has a bugged pcdata->clan_timer!",
                         victim->name );

//         return;
        }
        // Volk: May as well continue here, they've waited long enough.
    }

    if ( xIS_SET( ch->act, ACT_HCLAN_LEADER ) )
        clan = get_clan( "Throng" );                   // Yay vladaar fixed crash here,
    // it was stilled named horde
    else if ( xIS_SET( ch->act, ACT_ACLAN_LEADER ) )
        clan = get_clan( "Alliance" );
    else if ( xIS_SET( ch->act, ACT_NCLAN_LEADER ) )
        clan = get_clan( "Halcyon" );
    else {
        progbug( ch, "MpInduct - Mob doesn't have proper act flag set.  [%s]", ch->short_descr );
        return;
    }

    if ( timer < ( sysdata.clan_timer * 86400 ) ) {
        timer = ( sysdata.clan_timer * 86400 ) - timer;
        int                     days = ( timer / 86400 ) >= 1 ? ( timer / 86400 ) : 0;

        timer -= ( days * 86400 );
        int                     hours = ( timer / 3600 ) >= 1 ? ( timer / 3600 ) : 0;

        timer -= ( hours * 3600 );
        int                     mins = ( timer / 60 ) >= 1 ? ( timer / 60 ) : 0;

        timer -= ( mins * 60 );

        QUEST_DATA             *quest;
        CHQUEST_DATA           *chquest;

        if ( !str_cmp( clan->name, "throng" ) ) {
            quest = get_quest_from_name( "throng" );
        }
        if ( !str_cmp( clan->name, "alliance" ) ) {
            quest = get_quest_from_name( "alliance" );
        }
        if ( !str_cmp( clan->name, "halcyon" ) ) {
            quest = get_quest_from_name( "halcyon" );
        }

        if ( !str_cmp( clan->name, quest->name )
             && ( timer > 0 || days > 0 || mins > 0 || hours > 0 ) ) {
            for ( chquest = victim->pcdata->first_quest; chquest; chquest = chquest->next ) {
                if ( chquest->questnum != quest->number )
                    continue;
                UNLINK( chquest, victim->pcdata->first_quest, victim->pcdata->last_quest, next,
                        prev );
                DISPOSE( chquest );
                ch_printf( victim, "quest %s removed from %s due to having to wait to reapply.\r\n",
                           quest->name, victim->name );
                return;
            }
            return;
        }
    }
    victim->pcdata->clan = clan;
    victim->pcdata->clan_timer = 0;

    STRFREE( victim->pcdata->clan_name );
    victim->pcdata->clan_name = QUICKLINK( clan->name );
    act( AT_WHITE, "You induct $N into $t.", ch, clan->name, victim, TO_CHAR );
    act( AT_WHITE, "$n inducts $N into $t.", ch, clan->name, victim, TO_NOTVICT );
    act( AT_WHITE, "$n inducts you into $t.", ch, clan->name, victim, TO_VICT );
    if ( !str_cmp( victim->pcdata->clan_name, "alliance" ) ) {
        interpret( victim, ( char * ) "listen alliance" );
    }
    if ( !str_cmp( victim->pcdata->clan_name, "throng" ) ) {
        interpret( victim, ( char * ) "listen throng" );
    }
    if ( !str_cmp( victim->pcdata->clan_name, "halcyon" ) ) {
        interpret( victim, ( char * ) "listen halcyon" );
    }
    add_roster( victim->pcdata->clan, victim->name,
                capitalize( race_table[victim->race]->race_name ), victim->level,
                victim->pcdata->pkills, victim->pcdata->mkills, victim->pcdata->mdeaths,
                victim->pcdata->tradeclass, victim->pcdata->tradelevel );
    ++victim->pcdata->clan->members;
    save_char_obj( victim );
    save_clan( clan );
}

/* prints message only to victim */
void do_mpechoat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;
    short                   color;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpechoat - No argument [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpechoat [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    DONT_UPPER = TRUE;
    if ( argument[0] == '\0' )
        act( AT_ACTION, " ", ch, NULL, victim, TO_VICT );
    else if ( ( color = get_color( argument ) ) ) {
        argument = one_argument( argument, arg );
        act( color, argument, ch, NULL, victim, TO_VICT );
    }
    else
        act( AT_ACTION, argument, ch, NULL, victim, TO_VICT );

    DONT_UPPER = FALSE;

    ch->act = actflags;
}

/* prints message to room at large. */

void do_mpecho( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    short                   color;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    DONT_UPPER = TRUE;
    if ( argument[0] == '\0' )
        act( AT_ACTION, " ", ch, NULL, NULL, TO_ROOM );
    else if ( ( color = get_color( argument ) ) ) {
        argument = one_argument( argument, arg1 );
        act( color, argument, ch, NULL, NULL, TO_ROOM );
    }
    else
        act( AT_ACTION, argument, ch, NULL, NULL, TO_ROOM );
    DONT_UPPER = FALSE;
    ch->act = actflags;
}

/* sound support -haus */

void do_mpsoundaround( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpsoundaround - No argument [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpsoundaround [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    act_printf( AT_ACTION, ch, NULL, victim, TO_NOTVICT, "!!SOUND(%s)\n", argument );

    ch->act = actflags;
}

/* prints message only to victim */

void do_mpsoundat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        progbug( ch, "Mpsoundat - No argument [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpsoundat [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    act_printf( AT_ACTION, ch, NULL, victim, TO_VICT, "!!SOUND(%s)\n", argument );
    ch->act = actflags;
}

/* prints message to room at large. */

void do_mpsound( CHAR_DATA *ch, char *argument )
{
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        progbug( ch, "Mpsound - called w/o argument [%s]", ch->name );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "!!SOUND(%s)\n", argument );
    ch->act = actflags;
}

/* end sound stuff ----------------------------------------*/

/* Music stuff, same as above, at zMUD coders' request -- Blodkai */
void do_mpmusicaround( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpmusicaround - No argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpmusicaround [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    act_printf( AT_ACTION, ch, NULL, victim, TO_NOTVICT, "!!MUSIC(%s)\n", argument );
    ch->act = actflags;
    return;
}

void do_mpmusic( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpmusic - No argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpmusic [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    act_printf( AT_ACTION, ch, NULL, victim, TO_ROOM, "!!MUSIC(%s)\n", argument );
    ch->act = actflags;
    return;
}

void do_mpmusicat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpmusicat - No argument [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpmusicat [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    act_printf( AT_ACTION, ch, NULL, victim, TO_VICT, "!!MUSIC(%s)\n", argument );
    ch->act = actflags;
    return;
}

/* lets the mobile load an item or mobile.  All items
are loaded into inventory.  you can specify a level with
the load object portion as well. */
void do_mpmload( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    MOB_INDEX_DATA         *pMobIndex;
    CHAR_DATA              *victim;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number( arg ) ) {
        progbug( ch, "Mpmload - Bad vnum as arg [%s]", ch->name );
        return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL ) {
        progbug( ch, "Mpmload - Bad mob vnum [%s]", ch->name );
        return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    return;
}

void do_mpoload( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *obj;
    int                     level;
    int                     timer = 0;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number( arg1 ) ) {
        progbug( ch, "Mpoload - Bad syntax [%s]", ch->name );
        return;
    }

    if ( arg2[0] == '\0' )
        level = get_trust( ch );
    else {
        /*
         * New feature from Alander.
         */
        if ( !is_number( arg2 ) ) {
            progbug( ch, "Mpoload - Bad level syntax [%s]", ch->name );
            return;
        }
        level = atoi( arg2 );
        if ( level < 0 || level > get_trust( ch ) ) {
            progbug( ch, "Mpoload - Bad level [%s]", ch->name );
            return;
        }

        /*
         * New feature from Thoric.
         */
        timer = atoi( argument );
        if ( timer < 0 ) {
            progbug( ch, "Mpoload - Bad timer [%s]", ch->name );
            return;
        }
    }

    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL ) {
        progbug( ch, "Mpoload - Bad vnum arg [%s]", ch->name );
        return;
    }

    obj = create_object( pObjIndex, level );
    obj->timer = timer;
    if ( CAN_WEAR( obj, ITEM_TAKE ) )
        obj_to_char( obj, ch );
    else
        obj_to_room( obj, ch->in_room );

    return;
}

/* Just a hack of do_pardon from act_wiz.c -- Blodkai, 6/15/97 */
void do_mppardon( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        progbug( ch, "Mppardon:  missing argument [%s]", ch->name );
        send_to_char( "Mppardon who for what?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        progbug( ch, "Mppardon: offender not present [%s]", ch->name );
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        progbug( ch, "Mppardon:  trying to pardon NPC [%s]", ch->name );
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "attacker" ) ) {
        if ( xIS_SET( victim->act, PLR_ATTACKER ) ) {
            xREMOVE_BIT( victim->act, PLR_ATTACKER );
            send_to_char( "Attacker flag removed.\r\n", ch );
            send_to_char( "Your crime of attack has been pardoned.\r\n", victim );
        }
        return;
    }
    if ( !str_cmp( arg2, "killer" ) ) {
        if ( xIS_SET( victim->act, PLR_KILLER ) ) {
            xREMOVE_BIT( victim->act, PLR_KILLER );
            send_to_char( "Killer flag removed.\r\n", ch );
            send_to_char( "Your crime of murder has been pardoned.\r\n", victim );
        }
        return;
    }
    if ( !str_cmp( arg2, "litterbug" ) ) {
        if ( xIS_SET( victim->act, PLR_LITTERBUG ) ) {
            xREMOVE_BIT( victim->act, PLR_LITTERBUG );
            send_to_char( "Litterbug flag removed.\r\n", ch );
            send_to_char( "Your crime of littering has been pardoned./n/r", victim );
        }
        return;
    }
    if ( !str_cmp( arg2, "thief" ) ) {
        if ( xIS_SET( victim->act, PLR_THIEF ) ) {
            xREMOVE_BIT( victim->act, PLR_THIEF );
            send_to_char( "Thief flag removed.\r\n", ch );
            send_to_char( "Your crime of theft has been pardoned.\r\n", victim );
        }
        return;
    }
    send_to_char( "Pardon who for what?\r\n", ch );
    progbug( ch, "Mppardon: Invalid argument [%s]", ch->name );
    return;
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MUDprogram
   otherwise ugly stuff will happen */
void do_mppurge( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];                 /* Aurin 8/30/2010 */
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' ) {
        /*
         * 'purge'
         */
        CHAR_DATA              *vnext;

        for ( victim = ch->in_room->first_person; victim; victim = vnext ) {
            vnext = victim->next_in_room;
            if ( IS_NPC( victim ) && victim != ch )
                extract_char( victim, TRUE );
        }
        while ( ch->in_room->first_content )
            extract_obj( ch->in_room->first_content );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
            extract_obj( obj );
        }
        else
            progbug( ch, "Mppurge - Bad argument [%s]", ch->name );
        return;
    }

/* Added to allow mppurge to remove single item silently from player
 * inventory - guts provided by Syrin, installed by Aurin 8-30-2010
 */
    if ( !IS_NPC( victim ) ) {
        if ( arg2[0] == '\0' ) {
            progbug( ch, "Mppurge - Trying to purge a PC [%s]", ch->name );
            return;
        }
        else {
            if ( ( obj = get_obj_wear( victim, arg ) ) != NULL ) {
                unequip_char( victim, obj );
                extract_obj( obj );
                return;
            }
            if ( ( obj = get_obj_carry( victim, arg2 ) ) == NULL )
                progbug( ch, "Mppurge - Item not found in inventory [%s]", ch->name );
            else
                extract_obj( obj );
            return;
        }
    }
    /*
     * End of addition - Aurin 
     */

    /*
     * End of addition by Syrin 
     */

    if ( victim == ch ) {
        progbug( ch, "Mppurge - Trying to purge oneself [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) && victim->pIndexData->vnum == 3 ) {
        progbug( ch, "Mppurge: trying to purge supermob [%s]", ch->name );
        return;
    }

    extract_char( victim, TRUE );
    return;
}

/* Allow mobiles to go wizinvis with programs -- SB */

void do_mpinvis( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    short                   level;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( arg && arg[0] != '\0' ) {
        if ( !is_number( arg ) ) {
            progbug( ch, "Mpinvis - Non numeric argument [%s]", ch->name );
            return;
        }
        level = atoi( arg );
        if ( level < 2 || level > LEVEL_IMMORTAL ) {
            progbug( ch, "MPinvis - Invalid level [%s]", ch->name );
            return;
        }

        ch->mobinvis = level;
        ch_printf( ch, "Mobinvis level set to %d.\r\n", level );
        return;
    }

    if ( ch->mobinvis < 2 )
        ch->mobinvis = ch->level;

    if ( xIS_SET( ch->act, ACT_MOBINVIS ) ) {
        xREMOVE_BIT( ch->act, ACT_MOBINVIS );
        act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly fade back into existence.\r\n", ch );
    }
    else {
        xSET_BIT( ch->act, ACT_MOBINVIS );
        act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly vanish into thin air.\r\n", ch );
    }
    return;
}

/* lets the mobile goto any location it wishes that is not private */
/* Mounted chars follow their mobiles now - Blod, 11/97 */
void do_mpgoto( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    ROOM_INDEX_DATA        *location;
    CHAR_DATA              *fch;
    CHAR_DATA              *fch_next;
    ROOM_INDEX_DATA        *in_room;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpgoto - No argument [%s]", ch->name );
        return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL ) {
        progbug( ch, "Mpgoto - No such location [%s]", ch->name );
        return;
    }

    in_room = ch->in_room;
    if ( ch->fighting )
        stop_fighting( ch, TRUE );
    char_from_room( ch );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }

    char_to_room( ch, location );
    for ( fch = in_room->first_person; fch; fch = fch_next ) {
        fch_next = fch->next_in_room;
        if ( fch->mount && fch->mount == ch ) {
            char_from_room( fch );
            char_to_room( fch, location );
        }
    }
    return;
}

/* lets the mobile do a command at another location. Very useful */

void do_mpat( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    ROOM_INDEX_DATA        *location;
    ROOM_INDEX_DATA        *original;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        progbug( ch, "Mpat - Bad argument [%s]", ch->name );
        return;
    }

    if ( ( location = find_location( ch, arg ) ) == NULL ) {
        progbug( ch, "Mpat - No such location [%s]", ch->name );
        return;
    }

    original = ch->in_room;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    if ( !char_died( ch ) ) {
        char_from_room( ch );
        char_to_room( ch, original );
    }

    return;
}

/* allow a mobile to advance a player's level... very dangerous */
void do_mpadvance( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    int                     level;
    int                     iLevel;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpadvance - Bad syntax [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        progbug( ch, "Mpadvance - Victim not there [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) ) {
        progbug( ch, "Mpadvance - Victim is NPC [%s]", ch->name );
        return;
    }

    if ( IS_AVATAR( victim ) || IS_DUALAVATAR( victim ) || IS_TRIAVATAR( victim ) )
        return;

    level = victim->level + 1;

    if ( victim->level > ch->level ) {
        act( AT_TELL, "$n tells you, 'Sorry... you must seek someone more powerful than I.'", ch,
             NULL, victim, TO_VICT );
        return;
    }

    if ( IS_AVATAR( victim ) || IS_DUALAVATAR( victim ) || IS_TRIAVATAR( victim ) ) {
        set_char_color( AT_IMMORT, victim );
        act( AT_IMMORT,
             "$n makes some arcane gestures with $s hands, then points $s fingers at you!", ch,
             NULL, victim, TO_VICT );
        act( AT_IMMORT,
             "$n makes some arcane gestures with $s hands, then points $s fingers at $N!", ch, NULL,
             victim, TO_NOTVICT );
        set_char_color( AT_WHITE, victim );
        send_to_char( "You suddenly feel very strange...\r\n\r\n", victim );
        set_char_color( AT_LBLUE, victim );
    }

    switch ( level ) {
        default:
            send_to_char( "You feel more powerful!\r\n", victim );
            break;
        case LEVEL_IMMORTAL:
            do_help( victim, ( char * ) "M_GODLVL1_" );
            set_char_color( AT_WHITE, victim );
            send_to_char( "You awake... all your possessions are gone.\r\n", victim );
            while ( victim->first_carrying )
                extract_obj( victim->first_carrying );
            break;
        case LEVEL_AJ_CPL:
            do_help( victim, ( char * ) "M_GODLVL2_" );
            break;
        case LEVEL_AJ_SGT:
            do_help( victim, ( char * ) "M_GODLVL3_" );
            break;
        case LEVEL_AJ_COLONEL:
            do_help( victim, ( char * ) "M_GODLVL4_" );
            break;
        case LEVEL_AJ_GENERAL:
            do_help( victim, ( char * ) "M_GODLVL5_" );
            break;
    }

    for ( iLevel = victim->level; iLevel < level; iLevel++ ) {
        if ( level < LEVEL_IMMORTAL )
            send_to_char( "You raise a level!!  ", victim );
        victim->level += 1;

        advance_level( victim );
    }
    victim->exp = 1000 * UMAX( 1, victim->level );
    victim->trust = 0;
    return;
}

/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location 
   the area argument transfers everyone in the current area to the
   specified location */

void do_mptransfer( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    buf[MSL];
    ROOM_INDEX_DATA        *location;
    CHAR_DATA              *victim;
    CHAR_DATA              *nextinroom;
    CHAR_DATA              *immortal;
    DESCRIPTOR_DATA        *d;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        progbug( ch, "Mptransfer - Bad syntax [%s]", ch->name );
        return;
    }

    /*
     * Put in the variable nextinroom to make this work right. -Narn 
     */
    if ( !str_cmp( arg1, "all" ) ) {
        for ( victim = ch->in_room->first_person; victim; victim = nextinroom ) {
            nextinroom = victim->next_in_room;
            if ( victim != ch && !NEW_AUTH( victim ) && can_see( ch, victim ) ) {
                snprintf( buf, MSL, "%s %s", victim->name, arg2 );
                do_mptransfer( ch, buf );
            }
        }
        return;
    }
    /*
     * This will only transfer PC's in the area not Mobs --Shaddai 
     */
    if ( !str_cmp( arg1, "area" ) ) {
        for ( d = first_descriptor; d; d = d->next ) {
            if ( !d->character || ( d->connected != CON_PLAYING && d->connected != CON_EDITING ) || !can_see( ch, d->character ) || ch->in_room->area != d->character->in_room->area || d->character->level == 1 )  /* new 
                                                                                                                                                                                                                     * auth 
                                                                                                                                                                                                                     */
                continue;
            snprintf( buf, MSL, "%s %s", d->character->name, arg2 );
            do_mptransfer( ch, buf );
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' ) {
        location = ch->in_room;
    }
    else {
        if ( ( location = find_location( ch, arg2 ) ) == NULL ) {
            progbug( ch, "Mptransfer - No such location [%s]", ch->name );
            return;
        }

        if ( room_is_private( location ) ) {
            progbug( ch, "Mptransfer - Private room [%s]", ch->name );
            return;
        }
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        progbug( ch, "Mptransfer - No such person [%s]", ch->name );
        return;
    }

    if ( !victim->in_room ) {
        progbug( ch, "Mptransfer - Victim in Limbo [%s]", ch->name );
        return;
    }

    if ( NEW_AUTH( victim ) && location->area != victim->in_room->area ) {
        progbug( ch, "Mptransfer - unauthed char (%s)", victim->name );
        return;
    }

/* If victim not in area's level range, do not transfer */
    if ( !in_hard_range( victim, location->area )
         && !IS_SET( location->room_flags, ROOM_PROTOTYPE ) )
        return;

    if ( victim->fighting )
        stop_fighting( victim, TRUE );

/* hey... if an immortal's following someone, they should go with a mortal
 * when they're mptrans'd, don't you think?
 *  -- TRI
 */

    for ( immortal = victim->in_room->first_person; immortal; immortal = nextinroom ) {
        nextinroom = immortal->next_in_room;
        if ( IS_NPC( immortal ) || get_trust( immortal ) < LEVEL_IMMORTAL
             || immortal->master != victim )
            continue;
        if ( immortal->fighting )
            stop_fighting( immortal, TRUE );
        char_from_room( immortal );
        char_to_room( immortal, location );
    }

    char_from_room( victim );
    if ( victim->on ) {
        victim->on = NULL;
        set_position( victim, POS_STANDING );
    }
    if ( victim->position != POS_STANDING ) {
        set_position( victim, POS_STANDING );
    }

    char_to_room( victim, location );

    return;

}

void                    update_chquest_kamount( CHAR_DATA *ch, QUEST_DATA * quest, int nchapter );

/* This is to decrease the amount they need to kill to finish the quest */
void do_mpqkamount( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    QUEST_DATA             *quest;
    char                    arg[MIL];                  /* Victim's name */
    char                    arg2[MIL];                 /* Quest name */
    int                     chapter;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || arg2[0] == '\0' ) {
        progbug( ch, "Mpqueststart - Bad syntax [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_world( ch, arg ) ) ) {
        progbug( ch, "Mpquest - No such victim [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) )
        return;

    quest = get_quest_from_name( arg2 );

    if ( !quest ) {
        progbug( ch, "Mpquest - No quest named [%s]! [%d][%s]", arg2, ch->pIndexData->vnum,
                 ch->name );
        return;
    }

    if ( !str_cmp( argument, "remove" ) ) {
        CHQUEST_DATA           *chquest;

        for ( chquest = victim->pcdata->first_quest; chquest; chquest = chquest->next ) {
            if ( chquest->questnum != quest->number )
                continue;
            UNLINK( chquest, victim->pcdata->first_quest, victim->pcdata->last_quest, next, prev );
            DISPOSE( chquest );
            ch_printf( ch, "You remove quest %s from %s.\r\n", quest->name, victim->name );
            return;
        }
        send_to_char( "That player isn't currently on that quest.\r\n", ch );
        return;
    }
    if ( !is_number( argument ) ) {
        progbug( ch, "Mpquest - No chapter [%s]! [%d][%s]", argument, ch->pIndexData->vnum,
                 ch->name );
        return;
    }

    chapter = atoi( argument );

    if ( chapter == ( quest->chapters + 1 ) ) {
        update_chquest( victim, quest, chapter );
        return;
    }

    if ( victim->level < quest->level ) {
        ch_printf( victim, "&RYou must be level %d to start this quest.\r\n&D", quest->level );
        return;
    }

    CHAP_DATA              *chap = get_chap_from_quest( chapter, quest );

    if ( !chap ) {
        bug( "%s: no chapter %d for quest %d.", __FUNCTION__, chapter, quest->number );
        ch_printf( victim, "&RCan't do chapter %d for this quest because it doesn't exist.\r\n&D",
                   chapter );
        return;
    }

    if ( chap && chap->level > victim->level ) {
        ch_printf( victim, "&RYou must be level %d to start this chapter.\r\n&D", chap->level );
        return;
    }

    update_chquest_kamount( victim, quest, chapter );
}

void do_mpquest( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim,
                           *vch;
    QUEST_DATA             *quest;
    CHAP_DATA              *chap = NULL;
    char                    arg[MIL];                  /* Victim's name */
    char                    arg2[MIL];                 /* Quest name */
    int                     chapter;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || arg2[0] == '\0' ) {
        progbug( ch, "Mpquest - Bad syntax [%s]", ch->name );
        return;
    }

    if ( !( victim = get_char_world( ch, arg ) ) ) {
        progbug( ch, "Mpquest - No such victim [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) )
        return;

    quest = get_quest_from_name( arg2 );

    if ( !quest ) {
        progbug( ch, "Mpquest - No quest named [%s]! [%d][%s]", arg2, ch->pIndexData->vnum,
                 ch->name );
        return;
    }

    if ( !str_cmp( argument, "remove" ) ) {
        CHQUEST_DATA           *chquest;

        for ( chquest = victim->pcdata->first_quest; chquest; chquest = chquest->next ) {
            if ( chquest->questnum != quest->number )
                continue;
            UNLINK( chquest, victim->pcdata->first_quest, victim->pcdata->last_quest, next, prev );
            DISPOSE( chquest );
            ch_printf( ch, "You remove quest %s from %s.\r\n", quest->name, victim->name );
            return;
        }
        send_to_char( "That player isn't currently on that quest.\r\n", ch );
        return;
    }

    if ( !is_number( argument ) ) {
        progbug( ch, "Mpquest - No chapter [%s]! [%d][%s]", argument, ch->pIndexData->vnum,
                 ch->name );
        return;
    }

    chapter = atoi( argument );

    if ( chapter != ( quest->chapters + 1 ) ) {
        chap = get_chap_from_quest( chapter, quest );

        if ( !chap ) {
            bug( "%s: no chapter %d for quest %d.", __FUNCTION__, chapter, quest->number );
            ch_printf( victim,
                       "&RCan't do chapter %d for this quest because it doesn't exist.\r\n&D",
                       chapter );
            return;
        }
    }

    if ( chapter == ( quest->chapters + 1 ) ) {
        update_chquest( victim, quest, chapter );
        return;
    }

    if ( victim->level < quest->level ) {
        ch_printf( victim, "&RYou must be level %d to start this quest.\r\n&D", quest->level );
        return;
    }

    if ( chap && chap->level > victim->level ) {
        ch_printf( victim, "&RYou must be level %d to start this chapter.\r\n&D", chap->level );
        return;
    }

    update_chquest( victim, quest, chapter );
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */

void do_mpforce( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        progbug( ch, "Mpforce - Bad syntax [%s]", ch->name );
        return;
    }

    if ( !str_cmp( arg, "all" ) ) {
        CHAR_DATA              *vch,
                               *vch_next;

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;
            if ( get_trust( vch ) < get_trust( ch ) && can_see( ch, vch ) )
                interpret( vch, argument );
        }
    }
    else {
        CHAR_DATA              *victim;

        if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
            progbug( ch, "Mpforce - No such victim [%s]", ch->name );
            return;
        }

        if ( victim == ch ) {
            progbug( ch, "Mpforce - Forcing oneself [%s]", ch->name );
            return;
        }

        if ( !IS_NPC( victim ) && ( !victim->desc ) && IS_IMMORTAL( victim ) ) {
            progbug( ch, "Mpforce - Attempting to force link dead immortal [%s]", ch->name );
            return;
        }

        interpret( victim, argument );
    }

    return;
}

/*
 * mpnuisance mpunnuisance just incase we need them later --Shaddai
 */

void do_mpnuisance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MSL];
    struct tm              *now_time;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' ) {
        progbug( ch, "Mpnuisance - called w/o enough argument(s) [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mpnuisance [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }
    if ( IS_NPC( victim ) ) {
        progbug( ch, "Mpnuisance: victim is a mob [%s]", ch->name );
        return;
    }
    if ( IS_IMMORTAL( victim ) ) {
        progbug( ch, "Mpnuisance: not allowed on immortals [%s]", ch->name );
        return;
    }
    if ( victim->pcdata->nuisance ) {
        progbug( ch, "Mpnuisance: victim is already nuisanced [%s]", ch->name );
        return;
    }
    CREATE( victim->pcdata->nuisance, NUISANCE_DATA, 1 );
    victim->pcdata->nuisance->time = current_time;
    victim->pcdata->nuisance->flags = 1;
    victim->pcdata->nuisance->power = 2;
    now_time = localtime( &current_time );
    now_time->tm_mday += 1;
    victim->pcdata->nuisance->max_time = mktime( now_time );
    add_timer( victim, TIMER_NUISANCE, ( 28800 * 2 ), NULL, 0 );
    return;
}

void do_mpunnuisance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    TIMER                  *timer,
                           *timer_next;
    char                    arg1[MSL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' ) {
        progbug( ch, "Mpunnuisance - called w/o enough argument(s) [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mpunnuisance [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }

    if ( IS_NPC( victim ) ) {
        progbug( ch, "Mpunnuisance: victim was a mob [%s]", ch->name );
        return;
    }

    if ( IS_IMMORTAL( victim ) ) {
        progbug( ch, "Mpunnuisance: victim was an immortal [%s]", ch->name );
        return;
    }

    if ( !ch->pcdata->nuisance ) {
        progbug( ch, "Mpunnuisance: victim is not nuisanced [%s]", ch->name );
        return;
    }
    for ( timer = victim->first_timer; timer; timer = timer_next ) {
        timer_next = timer->next;
        if ( timer->type == TIMER_NUISANCE )
            extract_timer( victim, timer );
    }
    DISPOSE( victim->pcdata->nuisance );
    return;
}

void do_mpclearbodies( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    OBJ_DATA               *contents;
    char                    arg[MSL];
    char                    buf2[MSL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    for ( obj = ch->in_room->first_content; obj; obj = obj->next_content ) {
        if ( ( obj->item_type == ITEM_CORPSE_PC ) && obj->first_content == NULL )
            break;
    }

    if ( !obj ) {
        return;
    }

    extract_obj( obj );
    return;
}

void do_mpbodybag( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    char                    arg[MSL];
    char                    buf2[MSL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpbodybag - called w/o enough argument(s) [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpbodybag [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( IS_NPC( victim ) ) {
        progbug( ch, "Mpbodybag: bodybagging a npc corpse [%s]", ch->name );
        return;
    }
    snprintf( buf2, MSL, "the corpse of %s", arg );
    for ( obj = first_object; obj; obj = obj->next ) {
        if ( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == 11 ) ) {
            obj_from_room( obj );
            obj = obj_to_char( obj, ch );
            obj->timer = -1;
        }
    }
    /*
     * Maybe should just make the command logged... Shrug I am not sure
     * * --Shaddai
     */
//  progbug("Mpbodybag: Grabbed %s", buf2);
    return;
}

/*
 * mpmorph and mpunmorph for morphing people with mobs. --Shaddai
 */
void do_mpmorph( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    MORPH_DATA             *morph;
    char                    arg1[MIL];
    char                    arg2[MIL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        progbug( ch, "Mpmorph - called w/o enough argument(s) [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mpmorph [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }

    if ( !is_number( arg2 ) )
        morph = get_morph( arg2 );
    else
        morph = get_morph_vnum( atoi( arg2 ) );
    if ( !morph ) {
        progbug( ch, "Mpmorph - unknown morph [%s]", ch->name );
        return;
    }
    if ( victim->morph ) {
        progbug( ch, "Mpmorph - victim already morphed [%s]", ch->name );
        return;
    }
    do_morph_char( victim, morph );
    return;
}

void do_mpunmorph( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MSL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpmorph - called w/o an argument [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpunmorph [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( !victim->morph ) {
        progbug( ch, "Mpunmorph: victim not morphed [%s]", ch->name );
        return;
    }
    do_unmorph_char( victim );
    return;
}

void do_mpechozone( CHAR_DATA *ch, char *argument )
{                                                      /* Blod, late 97 */
    char                    arg1[MIL];
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    short                   color;
    EXT_BV                  actflags;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    actflags = ch->act;
    xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    if ( ( color = get_color( argument ) ) )
        argument = one_argument( argument, arg1 );
    DONT_UPPER = TRUE;
    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;
        if ( vch->in_room->area == ch->in_room->area && !IS_NPC( vch ) && IS_AWAKE( vch ) ) {
            if ( argument[0] == '\0' )
                act( AT_ACTION, " ", vch, NULL, NULL, TO_CHAR );
            else if ( color )
                act( color, argument, vch, NULL, NULL, TO_CHAR );
            else
                act( AT_ACTION, argument, vch, NULL, NULL, TO_CHAR );
        }
    }
    DONT_UPPER = FALSE;
    ch->act = actflags;
}

/*
 *  Haus' toys follow:
 */

/*
 * syntax:  mppractice victim spell_name max%
 *
 */
void do_mp_practice( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    buf[MIL];
    CHAR_DATA              *victim;
    int                     sn,
                            max,
                            tmp,
                            adept;
    char                   *skill_name;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_to_char( "Mppractice: bad syntax", ch );
        progbug( ch, "Mppractice - Bad syntax [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Mppractice: Student not in room? Invis?", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mppractice [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }

    if ( ( sn = skill_lookup( arg2 ) ) < 0 ) {
        send_to_char( "Mppractice: Invalid spell/skill name", ch );
        progbug( ch, "Mppractice: Invalid spell/skill name [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Mppractice: Can't train a mob", ch );
        progbug( ch, "Mppractice: Can't train a mob [%s]", ch->name );
        return;
    }

    skill_name = skill_table[sn]->name;

    max = atoi( arg3 );
    if ( ( max < 0 ) || ( max > 100 ) ) {
        snprintf( buf, MIL, "mp_practice: Invalid maxpercent: %d", max );
        send_to_char( buf, ch );
        progbug( ch, "%s [%s]", buf, ch->name );
        return;
    }

    // Added support for multi-classes -Taon 
    if ( victim->level < skill_table[sn]->skill_level[victim->Class]
         || victim->level < skill_table[sn]->skill_level[victim->secondclass]
         || victim->level < skill_table[sn]->skill_level[victim->thirdclass] ) {
        act_printf( AT_TELL, ch, NULL, victim, TO_VICT,
                    "$n attempts to tutor you in %s, but it's beyond your comprehension.",
                    skill_name );
        return;
    }

    /*
     * adept is how high the player can learn it 
     */
    /*
     * adept = class_table[ch->Class]->skill_adept; 
     * VOLK: Modified for more than one class (ie multiclass)
     */
    adept = get_maxadept( victim, sn, TRUE );

    if ( ( victim->pcdata->learned[sn] >= adept ) || ( victim->pcdata->learned[sn] >= max ) ) {
        act_printf( AT_TELL, ch, NULL, victim, TO_VICT,
                    "$n shows some knowledge of %s, but yours is clearly superior.", skill_name );
        return;
    }

    /*
     * past here, victim learns something 
     */
    tmp = UMIN( victim->pcdata->learned[sn] + int_app[get_curr_int( victim )].learn, max );
    act( AT_ACTION, "$N demonstrates $t to you.  You feel more learned in this subject.", victim,
         skill_table[sn]->name, ch, TO_CHAR );

    victim->pcdata->learned[sn] = max;

    if ( victim->pcdata->learned[sn] >= adept ) {
        victim->pcdata->learned[sn] = adept;
        act( AT_TELL, "$n tells you, 'You have learned all I know on this subject...'", ch, NULL,
             victim, TO_VICT );
    }
    return;

}

void do_mpscatter( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MSL];
    char                    arg2[MSL];
    CHAR_DATA              *victim;
    ROOM_INDEX_DATA        *pRoomIndex;
    int                     low_vnum,
                            high_vnum,
                            rvnum;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Mpscatter whom?\r\n", ch );
        progbug( ch, "Mpscatter: invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mpscatter [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }
    if ( IS_IMMORTAL( victim ) && get_trust( victim ) >= get_trust( ch ) ) {
        send_to_char( "You haven't the power to succeed against this victim.\r\n", ch );
        progbug( ch, "Mpscatter: victim level too high [%s]", ch->name );
        return;
    }
    if ( arg2[0] == '\0' ) {
        send_to_char( "You must specify a low vnum.\r\n", ch );
        progbug( ch, "Mpscatter:  missing low vnum [%s]", ch->name );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "You must specify a high vnum.\r\n", ch );
        progbug( ch, "Mpscatter:  missing high vnum [%s]", ch->name );
        return;
    }
    low_vnum = atoi( arg2 );
    high_vnum = atoi( argument );
    if ( low_vnum < 1 || high_vnum < low_vnum || low_vnum > high_vnum || low_vnum == high_vnum
         || high_vnum > MAX_VNUM ) {
        send_to_char( "Invalid range.\r\n", ch );
        progbug( ch, "Mpscatter:  invalid range [%s]", ch->name );
        return;
    }
    while ( 1 ) {
        rvnum = number_range( low_vnum, high_vnum );
        pRoomIndex = get_room_index( rvnum );
        if ( pRoomIndex )
            break;
    }
    if ( victim->fighting )
        stop_fighting( victim, TRUE );
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    victim->position = POS_RESTING;
    do_look( victim, ( char * ) "auto" );
    return;
}

/*
 * syntax: mpslay (character)
 */
void do_mp_slay( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    CHAR_DATA              *victim;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "mpslay whom?\r\n", ch );
        progbug( ch, "Mpslay: invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mpslay [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You try to slay yourself.  You fail.\r\n", ch );
        progbug( ch, "Mpslay: trying to slay self [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) && victim->pIndexData->vnum == 3 ) {
        send_to_char( "You cannot slay supermob!\r\n", ch );
        progbug( ch, "Mpslay: trying to slay supermob [%s]", ch->name );
        return;
    }

    if ( victim->level < LEVEL_IMMORTAL ) {
        act( AT_IMMORT, "You slay $M in cold blood!", ch, NULL, victim, TO_CHAR );
        act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT );
        act( AT_IMMORT, "$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT );
        set_cur_char( victim );
        raw_kill( ch, victim );
        stop_fighting( ch, FALSE );
        stop_hating( ch );
        stop_fearing( ch );
        stop_hunting( ch );
    }
    else {
        act( AT_IMMORT, "You attempt to slay $M and fail!", ch, NULL, victim, TO_CHAR );
        act( AT_IMMORT, "$n attempts to slay you.  What a kneebiter!", ch, NULL, victim, TO_VICT );
        act( AT_IMMORT, "$n attempts to slay $N.  Needless to say $e fails.", ch, NULL, victim,
             TO_NOTVICT );
    }
    return;
}

/*
 * syntax: mpdamage (character) (#hps)
 */
void do_mp_damage( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    buf[MSL];
    CHAR_DATA              *victim;
    CHAR_DATA              *nextinroom;
    int                     dam;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "mpdamage whom?\r\n", ch );
        progbug( ch, "Mpdamage: invalid argument1 [%s]", ch->name );
        return;
    }
    /*
     * Am I asking for trouble here or what? But I need it. -- Blodkai 
     */
    if ( !str_cmp( arg1, "all" ) ) {
        for ( victim = ch->in_room->first_person; victim; victim = nextinroom ) {
            nextinroom = victim->next_in_room;
            if ( victim != ch && can_see( ch, victim ) ) {  /* Could go either way */
                snprintf( buf, MSL, "'%s' %s", victim->name, arg2 );
                do_mp_damage( ch, buf );
            }
        }
        return;
    }
    if ( arg2[0] == '\0' ) {
        send_to_char( "mpdamage inflict how many hps?\r\n", ch );
        progbug( ch, "Mpdamage: invalid argument2 [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        progbug( ch, "Mpdamage: victim not in room", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You can't mpdamage yourself.\r\n", ch );
        progbug( ch, "Mpdamage: trying to damage self [%s]", ch->name );
        if ( ch->description )
            progbug( ch, "MpDamage extra info: description of ch: %s", ch->description );
        return;
    }
    dam = atoi( arg2 );
    if ( ( dam < 0 ) || ( dam > 32767 ) ) {
        send_to_char( "Mpdamage how much?\r\n", ch );
        progbug( ch, "Mpdamage: invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    /*
     * this is kinda begging for trouble        
     */
    /*
     * Note from Thoric to whoever put this in...
     * Wouldn't it be better to call damage(ch, ch, dam, dt)?
     * I hate redundant code
     */
    if ( simple_damage( ch, victim, dam, TYPE_UNDEFINED ) == rVICT_DIED ) {
        stop_fighting( ch, FALSE );
        stop_hating( ch );
        stop_fearing( ch );
        stop_hunting( ch );
    }
    return;
}

void do_mp_log( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    struct tm              *t = localtime( &current_time );

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        progbug( ch, "Mp_log:  non-existent entry [%s]", ch->name );
        return;
    }
    snprintf( buf, MSL, "&p%-2.2d/%-2.2d | %-2.2d:%-2.2d  &P%s:  &p%s", t->tm_mon + 1, t->tm_mday,
              t->tm_hour, t->tm_min, ch->short_descr, argument );
    append_to_file( MOBLOG_FILE, buf );
    return;
}

/*
 * syntax: mprestore (character) (#hps)                Gorog
 */
void do_mp_restore( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    int                     hp;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "mprestore whom?\r\n", ch );
        progbug( ch, "Mprestore: invalid argument1 [%s]", ch->name );
        return;
    }

    if ( arg2[0] == '\0' ) {
        send_to_char( "mprestore how many hps?\r\n", ch );
        progbug( ch, "Mprestore: invalid argument2 [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg1 ) ) )
            progbug( ch, "Mprestore [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg1 );
        return;
    }

    hp = atoi( arg2 );

    if ( ( hp < 0 ) || ( hp > 32767 ) ) {
        send_to_char( "Mprestore how much?\r\n", ch );
        progbug( ch, "Mprestore: invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    hp += victim->hit;
    victim->hit = ( hp > 30000 || hp < 0 || hp > victim->max_hit ) ? victim->max_hit : hp;
}

void do_mpfavor( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim;
    int                     favor;
    char                   *tmp;
    bool                    plus = FALSE,
        minus = FALSE;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "mpfavor whom?\r\n", ch );
        progbug( ch, "Mpfavor: invalid argument1", ch );
        return;
    }

    if ( arg2[0] == '\0' ) {
        send_to_char( "mpfavor how much favor?\r\n", ch );
        progbug( ch, "Mpfavor: invalid argument2", ch );
        return;
    }

    tmp = arg2;
    if ( tmp[0] == '+' ) {
        plus = TRUE;
        tmp++;
        if ( tmp[0] == '\0' ) {
            send_to_char( "mpfavor how much favor?\r\n", ch );
            progbug( ch, "Mpfavor: invalid argument2", ch );
            return;
        }
    }
    else if ( tmp[0] == '-' ) {
        minus = TRUE;
        tmp++;
        if ( tmp[0] == '\0' ) {
            send_to_char( "mpfavor how much favor?\r\n", ch );
            progbug( ch, "Mpfavor: invalid argument2", ch );
            return;
        }
    }
    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "Victim must be in room.\r\n", ch );
        progbug( ch, "Mpfavor: victim not in room", ch );
        return;
    }

    favor = atoi( tmp );
    if ( plus )
        victim->pcdata->favor = URANGE( -2500, victim->pcdata->favor + favor, 2500 );
    else if ( minus )
        victim->pcdata->favor = URANGE( -2500, victim->pcdata->favor - favor, 2500 );
    else
        victim->pcdata->favor = URANGE( -2500, favor, 2500 );
}

/*
 * Syntax mp_open_passage x y z
 *
 * opens a 1-way passage from room x to room y in direction z
 *
 *  won't mess with existing exits
 */
void do_mp_open_passage( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    ROOM_INDEX_DATA        *targetRoom,
                           *fromRoom;
    int                     targetRoomVnum,
                            fromRoomVnum,
                            exit_num;
    EXIT_DATA              *pexit;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    if ( !is_number( arg1 ) ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    fromRoomVnum = atoi( arg1 );
    if ( ( fromRoom = get_room_index( fromRoomVnum ) ) == NULL ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    if ( !is_number( arg2 ) ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    targetRoomVnum = atoi( arg2 );
    if ( ( targetRoom = get_room_index( targetRoomVnum ) ) == NULL ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    if ( !is_number( arg3 ) ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    exit_num = atoi( arg3 );
    if ( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) ) {
        progbug( ch, "MpOpenPassage - Bad syntax [%s][%d]", ch->name,
                 ch->pIndexData ? ch->pIndexData->vnum : 0 );
        return;
    }

    if ( ( pexit = get_exit( fromRoom, exit_num ) ) != NULL ) {
        /*
         * Now it ignores if the passage already exists, and does nothing.
         * if(!IS_SET(pexit->exit_info, EX_PASSAGE)) return; progbug("MpOpenPassage -
         * Exit exists [%s][%d]", ch->name, ch->pIndexData ? ch->pIndexData->vnum : 0); 
         */
        return;
    }

    pexit = make_exit( fromRoom, targetRoom, exit_num );
    pexit->key = -1;
    pexit->exit_info = EX_PASSAGE;
    return;
}

/*
 * Syntax mp_fillin x
 * Simply closes the door
 */
void do_mp_fill_in( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    EXIT_DATA              *pexit;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    one_argument( argument, arg );

    if ( ( pexit = find_door( ch, arg, TRUE ) ) == NULL ) {
        progbug( ch, "MpFillIn - Exit does not exist [%s]", ch->name );
        return;
    }
    SET_BIT( pexit->exit_info, EX_CLOSED );
    return;
}

/*
 * Syntax mp_close_passage x y 
 *
 * closes a passage in room x leading in direction y
 *
 * the exit must have EX_PASSAGE set
 */
void do_mp_close_passage( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    ROOM_INDEX_DATA        *fromRoom;
    int                     fromRoomVnum,
                            exit_num;
    EXIT_DATA              *pexit;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg2[0] == '\0' ) {
        progbug( ch, "MpClosePassage - Bad syntax [%s]", ch->name );
        return;
    }

    if ( !is_number( arg1 ) ) {
        progbug( ch, "MpClosePassage - Bad syntax [%s]", ch->name );
        return;
    }

    fromRoomVnum = atoi( arg1 );
    if ( ( fromRoom = get_room_index( fromRoomVnum ) ) == NULL ) {
        progbug( ch, "MpClosePassage - Bad syntax [%s]", ch->name );
        return;
    }

    if ( !is_number( arg2 ) ) {
        progbug( ch, "MpClosePassage - Bad syntax [%s]", ch->name );
        return;
    }

    exit_num = atoi( arg2 );
    if ( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) ) {
        progbug( ch, "MpClosePassage - Bad syntax [%s]", ch->name );
        return;
    }

    if ( ( pexit = get_exit( fromRoom, exit_num ) ) == NULL ) {
        return;                                        /* already closed, ignore...  so
                                                        * rand_progs */
        /*
         * can close without spam 
         */
    }

    if ( !IS_SET( pexit->exit_info, EX_PASSAGE ) ) {
        progbug( ch, "MpClosePassage - Exit not a passage [%s]", ch->name );
        return;
    }

    extract_exit( fromRoom, pexit );

    /*
     * act(AT_PLAIN, "A passage closes!", ch, NULL, NULL, TO_CHAR); 
     */
    /*
     * act(AT_PLAIN, "A passage closes!", ch, NULL, NULL, TO_ROOM); 
     */

    return;
}

/*
 * Does nothing.  Used for scripts.
 */
void do_mpnothing( CHAR_DATA *ch, char *argument )
{
    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    return;
}

/*
 *   Sends a message to sleeping character.  Should be fun
 *    with room sleep_progs
 *
 */
void do_mpdream( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MSL];
    CHAR_DATA              *vict;

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg1 );

    if ( ( vict = get_char_world( ch, arg1 ) ) == NULL ) {
        progbug( ch, "Mpdream: No such character [%s]", ch->name );
        return;
    }

    if ( vict->position <= POS_SLEEPING ) {
        send_to_char( argument, vict );
        send_to_char( "\r\n", vict );
    }
    return;
}

void do_mpapply( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    buf[MSL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        progbug( ch, "Mpapply - bad syntax [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
        progbug( ch, "Mpapply - no such player in room. [%s]", ch->name );
        return;
    }

    if ( !victim->desc ) {
        send_to_char( "Not on linkdeads.\r\n", ch );
        return;
    }

    if ( !NEW_AUTH( victim ) )
        return;

    if ( victim->pcdata->auth_state >= 1 )
        return;

    snprintf( buf, MSL, "%s@%s new %s %s %s applying...",
              victim->name, victim->desc->host, race_table[victim->race]->race_name,
              class_table[victim->Class]->who_name,
              IS_PKILL( victim ) ? "(Deadly)" : "(Peaceful)" );
    to_channel( buf, "Auth", LEVEL_IMMORTAL );
    victim->pcdata->auth_state = 1;
    return;
}

/*
 * Deposit some gold into the current area's economy  -Thoric
 */
void do_mp_deposit( CHAR_DATA *ch, char *argument )
{
    char                    arg[MSL];
    int                     money;

    if ( !IS_NPC( ch ) || ( ch->desc ) ) {
        error( ch );
        return;
    }

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpdeposit - bad syntax [%s]", ch->name );
        return;
    }
    money = atoi( arg );
    if ( money <= GET_MONEY( ch, DEFAULT_CURR ) && ch->in_room ) {
        GET_MONEY( ch, DEFAULT_CURR ) -= money;
        boost_economy( ch->in_room->area, money, DEFAULT_CURR );
    }
}

/*
 * Withdraw some gold from the current area's economy  -Thoric
 */
void do_mp_withdraw( CHAR_DATA *ch, char *argument )
{
    char                    arg[MSL];
    int                     money;

    if ( !IS_NPC( ch ) || ( ch->desc ) ) {
        error( ch );
        return;
    }

    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        progbug( ch, "Mpwithdraw - bad syntax [%s]", ch->name );
        return;
    }
    money = atoi( arg );
    if ( GET_MONEY( ch, DEFAULT_CURR ) < 1000000000 && money < 1000000000 && ch->in_room
         && economy_has( ch->in_room->area, money, DEFAULT_CURR ) ) {
        GET_MONEY( ch, DEFAULT_CURR ) += money;
        lower_economy( ch->in_room->area, money, DEFAULT_CURR );
    }
}

void do_mpdelay( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    int                     delay;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !*arg ) {
        send_to_char( "Delay for how many rounds?n\r", ch );
        progbug( ch, "Mpdelay: no duration specified [%s]", ch->name );
        return;
    }
    if ( !( victim = get_char_room( ch, arg ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mpdelay [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "Not against immortals.\r\n", ch );
        progbug( ch, "Mpdelay: target is immortal [%s]", ch->name );
        return;
    }
    argument = one_argument( argument, arg );
    if ( !*arg || !is_number( arg ) ) {
        send_to_char( "Delay them for how many rounds?\r\n", ch );
        progbug( ch, "Mpdelay: invalid (nonexistant?) argument [%s]", ch->name );
        return;
    }
    delay = atoi( arg );
    if ( delay < 1 || delay > 30 ) {
        send_to_char( "Argument out of range.\r\n", ch );
        progbug( ch, "Mpdelay:  argument out of range (1 to 30) [%s]", ch->name );
        return;
    }
    WAIT_STATE( victim, delay * PULSE_VIOLENCE );
    send_to_char( "Mpdelay applied.\r\n", ch );
    return;
}

void                    arena_chan( char *argument );

void do_mptoss( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    argument = one_argument( argument, arg );
    OBJ_DATA               *egg = NULL;
    CHAR_DATA              *victim;
    int                     rand,
                            tries = 0;

    if ( !*arg ) {
        progbug( ch, "Mptoss: missing victim" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        progbug( ch, "Mptoss: Target not in the room" );
        return;
    }

    /*
     * Check to see that they have the egg first 
     */
    for ( egg = victim->first_carrying; egg; egg = egg->next_content ) {
        if ( egg->pIndexData->vnum == 41007 )
            return;
    }

    rand = number_range( 4401, 4420 );
    /*
     * Lets make sure we have a valid room and bug if we dont 
     */
    while ( !get_room_index( rand ) ) {
        bug( "%s: room vnum %d is NULL.", __FUNCTION__, rand );
        rand = number_range( 4401, 4421 );
        if ( ++tries >= 10 )                           /* Only try this 10 times */
            return;
    }
    char_from_room( victim );
    char_to_room( victim, get_room_index( rand ) );
    act( AT_CYAN, "$n gets tossed by the green dragon and lands on the ground!\r\n", victim, NULL,
         NULL, TO_ROOM );
    act( AT_TELL, "You get tossed by the green dragon and land on the ground!\r\n", victim, NULL,
         NULL, TO_CHAR );
    do_look( victim, ( char * ) "auto" );
}

void                    find_arena_winner( CHAR_DATA *ch );
short                   get_arena_count(  );

void do_mpeat( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA        *d;
    char                    arg[MIL];
    char                    buf[MSL];
    CHAR_DATA              *victim,
                           *target_victim = NULL,
        *utarget = NULL;
    int                     vnum;
    ROOM_INDEX_DATA        *location;

    argument = one_argument( argument, arg );
    bool                    found = FALSE;

    if ( !*arg ) {
        progbug( ch, "Mpeat: missing victim" );
        return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        progbug( ch, "Mpeat: Target not in the room" );
        return;
    }

    if ( !dragonegg ) {
        progbug( ch, "Mpeat: There is no dragonegg to check." );
        return;
    }

    if ( !dragonegg->carried_by ) {
        progbug( ch, "Mpeat: Dragonegg isn't being carried by anyone." );
        return;
    }

    if ( victim != dragonegg->carried_by )
        return;

    if ( get_arena_count(  ) == 1 ) {
        find_arena_winner( NULL );
        return;
    }

    act( AT_RED, "$n spots $N holding her dragon egg, and with her mighty jaws, devours $N!", ch,
         NULL, victim, TO_ROOM );
    snprintf( buf, MIL, "%s has been eaten attempting to tease the dragon!", victim->name );
    arena_chan( buf );

    if ( xIS_SET( victim->act, PLR_TEASE ) )
        xREMOVE_BIT( victim->act, PLR_TEASE );

    obj_from_char( dragonegg );                        /* Take the egg from the one
                                                        * carrying it */
    char_from_room( victim );
    char_to_room( victim, get_room_index( victim->pcdata->htown->recall ) );
    --arena_population;
    act( AT_TELL, "$n falls out of the sky!\r\n", victim, NULL, NULL, TO_ROOM );
    act( AT_TELL, "You fall suddenly out of the sky!\r\n", victim, NULL, NULL, TO_CHAR );
    do_look( victim, ( char * ) "auto" );

    // Give an egg back to a player if in Arena room vnum range
    for ( vnum = 4400; vnum <= 4421; vnum++ ) {
        if ( !( location = get_room_index( vnum ) ) )
            continue;
        for ( target_victim = location->first_person; target_victim;
              target_victim = target_victim->next_in_room ) {
            if ( !target_victim || IS_NPC( target_victim ) )
                continue;
            if ( !utarget || number_range( 1, 5 ) > 3 )
                utarget = target_victim;
        }
    }

    if ( utarget ) {
        send_to_char( "You now have the egg.\r\n", utarget );
//      log_printf( "%s: egg was given to %s.", __FUNCTION__, utarget->name );
        obj_to_char( dragonegg, utarget );
    }
    else {
        extract_obj( dragonegg );
        dragonegg = NULL;
        end_game( FALSE );
    }
}

void do_mppeace( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *rch;
    CHAR_DATA              *victim;

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !*arg ) {
        send_to_char( "Who do you want to mppeace?\r\n", ch );
        progbug( ch, "Mppeace: invalid (nonexistent?) argument [%s]", ch->name );
        return;
    }
    if ( !str_cmp( arg, "all" ) ) {
        for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
            if ( rch->fighting ) {
                stop_fighting( rch, TRUE );
                do_sit( rch, ( char * ) "" );
            }
            stop_hating( rch );
            stop_hunting( rch );
            stop_fearing( rch );
        }
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They must be in the room.n\r", ch );
        if ( !( victim = get_char_world( ch, arg ) ) )
            progbug( ch, "Mppeace [%d]- victim [%s] does not exist",
                     ch->pIndexData ? ch->pIndexData->vnum : 0, arg );
        return;
    }
    if ( victim->fighting )
        stop_fighting( victim, TRUE );
    stop_hating( ch );
    stop_hunting( ch );
    stop_fearing( ch );
    stop_hating( victim );
    stop_hunting( victim );
    stop_fearing( victim );
    return;
}

void do_mppkset( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MSL];

    if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ) ) {
        error( ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( argument[0] == '\0' || arg[0] == '\0' ) {
        progbug( ch, "Mppkset - bad syntax [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        progbug( ch, "Mppkset - no such player in room. [%s]", ch->name );
        return;
    }

    if ( !str_cmp( argument, "yes" ) || !str_cmp( argument, "y" ) ) {
        if ( !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
            SET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
    }
    else if ( !str_cmp( argument, "no" ) || !str_cmp( argument, "n" ) ) {
        if ( IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
            REMOVE_BIT( victim->pcdata->flags, PCFLAG_DEADLY );
    }
    else {
        progbug( ch, "Mppkset - bad syntax [%s]", ch->name );
        return;
    }
    return;
}

/*
 * Inflict damage from a mudprogram
 *
 *  note: should be careful about using victim afterwards
 */
ch_ret simple_damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    short                   dameq;
    bool                    npcvict;
    OBJ_DATA               *damobj;
    ch_ret                  retcode;
    char                    buf[MSL];

    retcode = rNONE;

    if ( !ch ) {
        bug( "%s", "Damage: null ch!" );
        return rERROR;
    }
    if ( !victim ) {
        progbug( ch, "Damage: null victim! [%s]", ch->name );
        return rVICT_DIED;
    }

    if ( victim->position == POS_DEAD ) {
        return rVICT_DIED;
    }

    npcvict = IS_NPC( victim );

    if ( dam ) {
        if ( IS_FIRE( dt ) )
            dam = ris_damage( victim, dam, RIS_FIRE );
        else if ( IS_COLD( dt ) )
            dam = ris_damage( victim, dam, RIS_COLD );
        else if ( IS_ACID( dt ) )
            dam = ris_damage( victim, dam, RIS_ACID );
        else if ( IS_ELECTRICITY( dt ) )
            dam = ris_damage( victim, dam, RIS_ELECTRICITY );
        else if ( IS_ENERGY( dt ) )
            dam = ris_damage( victim, dam, RIS_ENERGY );
        else if ( dt == gsn_poison )
            dam = ris_damage( victim, dam, RIS_POISON );
        else if ( dt == ( TYPE_HIT + 7 ) || dt == ( TYPE_HIT + 8 ) )
            dam = ris_damage( victim, dam, RIS_BLUNT );
        else if ( dt == ( TYPE_HIT + 2 ) || dt == ( TYPE_HIT + 11 ) )
            dam = ris_damage( victim, dam, RIS_PIERCE );
        else if ( dt == ( TYPE_HIT + 1 ) || dt == ( TYPE_HIT + 3 ) )
            dam = ris_damage( victim, dam, RIS_SLASH );
        if ( dam < 0 )
            dam = 0;
    }

    if ( victim != ch ) {
        /*
         * Damage modifiers.
         */
        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) )
            dam /= 2;

        if ( IS_AFFECTED( victim, AFF_PROTECT ) && IS_EVIL( ch ) )
            dam -= ( int ) ( dam / 4 );

        if ( dam < 0 )
            dam = 0;

        /*
         * dam_message(ch, victim, dam, dt); 
         */
    }

    /*
     * Check for EQ damage.... ;)
     */

    if ( dam > 10 ) {
        /*
         * get a random body eq part 
         */
        dameq = number_range( WEAR_LIGHT, WEAR_EYES );
        damobj = get_eq_char( victim, dameq );
        if ( damobj ) {
            if ( dam > get_obj_resistance( damobj ) ) {
                set_cur_obj( damobj );
                damage_obj( damobj );
            }
        }
    }

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= dam;
    if ( !IS_NPC( victim ) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
        victim->hit = 1;

    if ( !npcvict && get_trust( victim ) >= LEVEL_IMMORTAL && get_trust( ch ) >= LEVEL_IMMORTAL
         && victim->hit < 1 )
        victim->hit = 1;
    update_pos( victim );

    switch ( victim->position ) {
        case POS_MORTAL:
            act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL,
                 NULL, TO_ROOM );
            act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.", victim,
                 NULL, NULL, TO_CHAR );
            break;

        case POS_INCAP:
            act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.", victim, NULL,
                 NULL, TO_ROOM );
            act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.", victim,
                 NULL, NULL, TO_CHAR );
            break;

        case POS_STUNNED:
            if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
                act( AT_ACTION, "$n is stunned, but will probably recover.", victim, NULL, NULL,
                     TO_ROOM );
                act( AT_HURT, "You are stunned, but will probably recover.", victim, NULL, NULL,
                     TO_CHAR );
            }
            break;

        case POS_DEAD:
            act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
            act( AT_DEAD, "You have been KILLED!!\r\n", victim, 0, 0, TO_CHAR );
            break;

        default:
            if ( dam > victim->max_hit / 4 )
                act( AT_RED, "That really did HURT!", victim, 0, 0, TO_CHAR );
            if ( victim->hit < victim->max_hit / 4 )
                act( AT_RED, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0,
                     TO_CHAR );
            break;
    }

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD ) {
        if ( !npcvict ) {
/*
         snprintf( buf, MSL, "%s (%d) killed by %s at %d", victim->name, victim->level,
                   ( IS_NPC( ch ) ? ch->short_descr : ch->name ),
                    victim->in_room->vnum );
         log_string( buf );
         to_channel( buf, "Monitor", LEVEL_IMMORTAL );
         if( victim != ch )
            snprintf( buf, MSL, "%s has been killed by %s!", victim->name,
                      ( IS_NPC( ch ) ? ch->short_descr : ch->name ) );
         else
            snprintf( buf, MSL, "%s has been killed!", victim->name );
         announce( buf );
*/
            /*
             * Dying penalty:
             * 1/2 way back to previous level.
             */
            if ( !in_arena( victim ) ) {
                if ( victim->exp > 0 )
                    gain_exp( victim, 0 - ( victim->exp / 2 ) );
            }

        }
        set_cur_char( victim );
        raw_kill( ch, victim );
        victim = NULL;

        return rVICT_DIED;
    }

    if ( victim == ch )
        return rNONE;

    /*
     * Take care of link dead people.
     */
    if ( !npcvict && !victim->desc ) {
        if ( number_range( 0, victim->wait ) == 0 ) {
            do_recall( victim, ( char * ) "" );
            return rNONE;
        }
    }

    /*
     * Wimp out?
     */
    if ( npcvict && dam > 0 ) {
        if ( ( xIS_SET( victim->act, ACT_WIMPY ) && number_bits( 1 ) == 0
               && victim->hit < victim->max_hit / 2 ) || ( IS_AFFECTED( victim, AFF_CHARM )
                                                           && victim->master
                                                           && victim->master->in_room !=
                                                           victim->in_room ) ) {
            start_fearing( victim, ch );
            stop_hunting( victim );
            do_flee( victim, ( char * ) "" );
        }
    }

    if ( !npcvict && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait == 0 )
        do_flee( victim, ( char * ) "" );
    else if ( !npcvict && xIS_SET( victim->act, PLR_FLEE ) )
        do_flee( victim, ( char * ) "" );

    tail_chain(  );
    return rNONE;
}

void do_mphtown( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    HTOWN_DATA             *htown;
    CHAR_DATA              *victim;

    argument = one_argument( argument, arg1 );

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( victim->pcdata->htown ) {
        if ( victim->pcdata->htown->members > 1 )
            --victim->pcdata->htown->members;
        save_htown( victim->pcdata->htown );
    }

    if ( VLD_STR( victim->pcdata->htown_name ) )
        STRFREE( victim->pcdata->htown_name );

    victim->pcdata->htown = NULL;

    if ( !( htown = get_htown( argument ) ) ) {
        send_to_char( "No such hometown.\r\n", victim );
        return;
    }

    victim->pcdata->htown_name = STRALLOC( htown->name );
    victim->pcdata->htown = htown;
    ++victim->pcdata->htown->members;
    save_htown( victim->pcdata->htown );
}

void do_mpmset( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    buf[MSL];
    char                    outbuf[MSL];
    CHAR_DATA              *victim;
    int                     value,
                            v2;
    int                     minattr,
                            maxattr;

    /*
     * A desc means switched.. too many loopholes if we allow that.. 
     */
    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        error( ch );
        return;
    }

    smash_tilde( argument );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    mudstrlcpy( arg3, argument, MIL );

    if ( !*arg1 ) {
        progbug( ch, "MpMset: no args [%s]", ch->name );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        progbug( ch, "MpMset: no victim [%s]", ch->name );
        return;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "You can't do that!\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_PROTOTYPE ) ) {
        progbug( ch, "MpMset: victim is proto [%s]", ch->name );
        return;
    }

    if ( IS_NPC( victim ) ) {
        minattr = 1;
        maxattr = 25;
    }
    else {
        minattr = 3;
        maxattr = 18;
    }

    value = is_number( arg3 ) ? atoi( arg3 ) : -1;
    if ( atoi( arg3 ) < -1 && value == -1 )
        value = atoi( arg3 );

    if ( !str_cmp( arg2, "str" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid str [%s]", ch->name );
            return;
        }
        victim->perm_str = value;
        return;
    }

    if ( !str_cmp( arg2, "int" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid int [%s]", ch->name );
            return;
        }
        victim->perm_int = value;
        return;
    }

    if ( !str_cmp( arg2, "gold" ) ) {
        if ( value < 0 || value > 10000 ) {
            progbug( ch, "MpMset: Invalid gold amount setting [%s]", ch->name );
            return;
        }
        victim->money[CURR_GOLD] = value;
        return;
    }

    if ( !str_cmp( arg2, "silver" ) ) {
        if ( value < 0 || value > 10000 ) {
            progbug( ch, "MpMset: Invalid silver amount setting [%s]", ch->name );
            return;
        }
        victim->money[CURR_SILVER] = value;
        return;
    }

    if ( !str_cmp( arg2, "bronze" ) ) {
        if ( value < 0 || value > 10000 ) {
            progbug( ch, "MpMset: Invalid bronze amount setting [%s]", ch->name );
            return;
        }
        victim->money[CURR_BRONZE] = value;
        return;
    }
    if ( !str_cmp( arg2, "copper" ) ) {
        if ( value < 0 || value > 10000 ) {
            progbug( ch, "MpMset: Invalid copper amount setting [%s]", ch->name );
            return;
        }
        victim->money[CURR_COPPER] = value;
        return;
    }

    if ( !str_cmp( arg2, "wis" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid wis [%s]", ch->name );
            return;
        }
        victim->perm_wis = value;
        return;
    }

    if ( !str_cmp( arg2, "dex" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid dex [%s]", ch->name );
            return;
        }
        victim->perm_dex = value;
        return;
    }

    if ( !str_cmp( arg2, "con" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid con [%s]", ch->name );
            return;
        }
        victim->perm_con = value;
        return;
    }

    if ( !str_cmp( arg2, "cha" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid cha [%s]", ch->name );
            return;
        }
        victim->perm_cha = value;
        return;
    }

    if ( !str_cmp( arg2, "lck" ) ) {
        if ( value < minattr || value > maxattr ) {
            progbug( ch, "MpMset: Invalid lck [%s]", ch->name );
            return;
        }
        victim->perm_lck = value;
        return;
    }

    if ( !str_cmp( arg2, "sav1" ) ) {
        if ( value < -30 || value > 30 ) {
            progbug( ch, "MpMset: Invalid sav1 [%s]", ch->name );
            return;
        }
        victim->saving_poison_death = value;
        return;
    }

    if ( !str_cmp( arg2, "sav2" ) ) {
        if ( value < -30 || value > 30 ) {
            progbug( ch, "MpMset: Invalid sav2 [%s]", ch->name );
            return;
        }
        victim->saving_wand = value;
        return;
    }

    if ( !str_cmp( arg2, "sav3" ) ) {
        if ( value < -30 || value > 30 ) {
            progbug( ch, "MpMset: Invalid sav3 [%s]", ch->name );
            return;
        }
        victim->saving_para_petri = value;
        return;
    }

    if ( !str_cmp( arg2, "sav4" ) ) {
        if ( value < -30 || value > 30 ) {
            progbug( ch, "MpMset: Invalid sav4 [%s]", ch->name );
            return;
        }
        victim->saving_breath = value;
        return;
    }

    if ( !str_cmp( arg2, "sav5" ) ) {
        if ( value < -30 || value > 30 ) {
            progbug( ch, "MpMset: Invalid sav5 [%s]", ch->name );
            return;
        }
        victim->saving_spell_staff = value;
        return;
    }

    if ( !str_cmp( arg2, "sex" ) ) {
        if ( value < 0 || value > 2 ) {
            progbug( ch, "MpMset: Invalid sex [%s]", ch->name );
            return;
        }
        victim->sex = value;
        return;
    }

    if ( !str_cmp( arg2, "class" ) ) {
        if ( IS_NPC( victim ) ) {                      /* Broken by Haus... fixed by *
                                                        * Thoric */
            if ( value >= MAX_NPC_CLASS || value < 0 ) {
                progbug( ch, "MpMset: Invalid npc class [%s]", ch->name );
                return;
            }
            victim->Class = value;
            return;
        }
        else {
            if ( value >= MAX_PC_CLASS || value < 0 ) {
                progbug( ch, "MpMset: Invalid pc class [%s]", ch->name );
                return;
            }
            log_printf( "%s setting pc class for %s to %d", ch->name, victim->name, value );
            victim->Class = value;
            return;
        }
    }

    if ( !str_cmp( arg2, "race" ) ) {
        value = get_npc_race( arg3 );
        if ( value < 0 )
            value = atoi( arg3 );
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc race [%s]", ch->name );
            return;
        }
        if ( value < 0 || value >= MAX_NPC_RACE ) {
            progbug( ch, "MpMset: Invalid npc race [%s]", ch->name );
            return;
        }
        victim->race = value;
        return;
    }

    if ( !str_cmp( arg2, "htown" ) ) {
        HTOWN_DATA             *htown;

        if ( IS_NPC( victim ) ) {
            send_to_char( "Not on NPC's.\r\n", ch );
            return;
        }

        if ( !VLD_STR( arg3 ) ) {                      // free up htown if already have
                                                       // one
            if ( victim->pcdata->htown ) {
                if ( victim->pcdata->htown->members > 1 )
                    --victim->pcdata->htown->members;
                save_htown( victim->pcdata->htown );
            }
            if ( VLD_STR( victim->pcdata->htown_name ) )
                STRFREE( victim->pcdata->htown_name );
            victim->pcdata->htown = NULL;
            send_to_char( "Removed from htown.\r\n", ch );
            return;
        }
        htown = get_htown( arg3 );
        if ( !htown ) {
            send_to_char( "No such hometown.\r\n", ch );
            return;
        }
        ++victim->pcdata->htown->members;
        save_htown( victim->pcdata->htown );
        if ( VLD_STR( victim->pcdata->htown_name ) )
            STRFREE( victim->pcdata->htown_name );
        victim->pcdata->htown_name = QUICKLINK( htown->name );
        victim->pcdata->htown = htown;
        return;

    }

    if ( !str_cmp( arg2, "armor" ) ) {
        if ( value < -300 || value > 300 ) {
            send_to_char( "AC range is -300 to 300.\r\n", ch );
            return;
        }
        victim->armor = value;
        return;
    }

    if ( !str_cmp( arg2, "level" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc level [%s]", ch->name );
            return;
        }

        if ( value < 0 || value > MAX_LEVEL ) {
            progbug( ch, "MpMset: Invalid npc level [%s]", ch->name );
            return;
        }
        victim->level = value;
        return;
    }

    if ( !str_cmp( arg2, "numattacks" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc numattacks [%s]", ch->name );
            return;
        }

        if ( value < 0 || value > 20 ) {
            progbug( ch, "MpMset: Invalid npc numattacks [%s]", ch->name );
            return;
        }
        victim->numattacks = value;
        return;
    }

    if ( !str_cmp( arg2, "hitroll" ) ) {
        victim->hitroll = URANGE( 0, value, 85 );
        return;
    }

    if ( !str_cmp( arg2, "damroll" ) ) {
        victim->damroll = URANGE( 0, value, 65 );
        return;
    }

    if ( !str_cmp( arg2, "hp" ) ) {
        if ( value < 1 || value > 32767 ) {
            progbug( ch, "MpMset: Invalid hp [%s]", ch->name );
            return;
        }
        victim->max_hit = value;
        return;
    }

    if ( !str_cmp( arg2, "mana" ) ) {
        if ( value < 0 || value > 32767 ) {
            progbug( ch, "MpMset: Invalid mana [%s]", ch->name );
            return;
        }
        victim->max_mana = value;
        return;
    }

    if ( !str_cmp( arg2, "move" ) ) {
        if ( value < 0 || value > 32767 ) {
            progbug( ch, "MpMset: Invalid move [%s]", ch->name );
            return;
        }
        victim->max_move = value;
        return;
    }

    if ( !str_cmp( arg2, "practice" ) ) {
        if ( value < 0 || value > 100 ) {
            progbug( ch, "MpMset: Invalid practice [%s]", ch->name );
            return;
        }
        victim->practice = value;
        return;
    }

    if ( !str_cmp( arg2, "align" ) ) {
        if ( value < -1000 || value > 1000 ) {
            progbug( ch, "MpMset: Invalid align [%s]", ch->name );
            return;
        }
        victim->alignment = value;
        return;
    }
    if ( !str_cmp( arg2, "mentalstate" ) ) {
        if ( value < -100 || value > 100 ) {
            progbug( ch, "MpMset: Invalid mentalstate [%s]", ch->name );
            return;
        }
        victim->mental_state = value;
        return;
    }

    if ( !str_cmp( arg2, "emotion" ) ) {
        if ( value < -100 || value > 100 ) {
            progbug( ch, "MpMset: Invalid emotion [%s]", ch->name );
            return;
        }
        victim->emotional_state = value;
        return;
    }

    if ( !str_cmp( arg2, "thirst" ) ) {
        if ( IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set npc thirst [%s]", ch->name );
            return;
        }

        if ( value < 0 || value > 100 ) {
            progbug( ch, "MpMset: Invalid pc thirst [%s]", ch->name );
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        return;
    }

    if ( !str_cmp( arg2, "drunk" ) ) {
        if ( IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set npc drunk [%s]", ch->name );
            return;
        }

        if ( value < 0 || value > 100 ) {
            progbug( ch, "MpMset: Invalid pc drunk [%s]", ch->name );
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if ( !str_cmp( arg2, "full" ) ) {
        if ( IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set npc full [%s]", ch->name );
            return;
        }

        if ( value < 0 || value > 100 ) {
            progbug( ch, "MpMset: Invalid pc full [%s]", ch->name );
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        return;
    }

    if ( !str_cmp( arg2, "blood" ) ) {
        if ( IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set npc blood [%s]", ch->name );
            return;
        }

        if ( value < 0 || value > 300000 ) {
            progbug( ch, "MpMset: Invalid pc blood [%s]", ch->name );
            return;
        }
        victim->max_blood = value;
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc name [%s]", ch->name );
            return;
        }

        STRFREE( victim->name );
        victim->name = STRALLOC( arg3 );
        return;
    }

    if ( !str_cmp( arg2, "short" ) ) {
        STRFREE( victim->short_descr );
        victim->short_descr = STRALLOC( arg3 );
        return;
    }

    if ( !str_cmp( arg2, "long" ) ) {
        STRFREE( victim->long_descr );
        mudstrlcpy( buf, arg3, MSL );
        mudstrlcat( buf, "\r\n", MSL );
        victim->long_descr = STRALLOC( buf );
        return;
    }

    if ( !str_cmp( arg2, "title" ) ) {
        if ( IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set npc title [%s]", ch->name );
            return;
        }

        set_title( victim, arg3 );
        return;
    }

    if ( !str_cmp( arg2, "spec" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc spec [%s]", ch->name );
            return;
        }

        if ( !str_cmp( arg3, "none" ) ) {
            victim->spec_fun = NULL;
            return;
        }

        if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 ) {
            progbug( ch, "MpMset: Invalid spec [%s]", ch->name );
            return;
        }
        return;
    }

    if ( !str_cmp( arg2, "flags" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc flags [%s]", ch->name );
            return;
        }

        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no flags [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_actflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                progbug( ch, "MpMset: Invalid flag [%s]", ch->name );
            else {
                if ( value == ACT_PROTOTYPE )
                    progbug( ch, "MpMset: can't set prototype flag [%s]", ch->name );
                else if ( value == ACT_IS_NPC )
                    progbug( ch, "MpMset: can't remove npc flag [%s]", ch->name );
/*
       else if(value == ACT_POLYMORPHED)
  progbug("MpMset: can't change polymorphed flag [%s]", ch->name);
*/
                else
                    xTOGGLE_BIT( victim->act, value );
            }
        }
        return;
    }

    if ( !str_cmp( arg2, "affected" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't modify pc affected [%s]", ch->name );
            return;
        }

        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no affected [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_aflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                progbug( ch, "MpMset: Invalid affected [%s]", ch->name );
            else
                xTOGGLE_BIT( victim->affected_by, value );
        }
        return;
    }

    /*
     * save some more finger-leather for setting RIS stuff
     * Why there's can_modify checks here AND in the called function, Ill
     * never know, so I removed them.. -- Alty
     */
    if ( !str_cmp( arg2, "r" ) ) {
        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "i" ) ) {
        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "s" ) ) {
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "ri" ) ) {
        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "rs" ) ) {
        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "is" ) ) {
        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "ris" ) ) {
        snprintf( outbuf, MSL, "%s resistant %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        snprintf( outbuf, MSL, "%s immune %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        snprintf( outbuf, MSL, "%s susceptible %s", arg1, arg3 );
        do_mpmset( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "resistant" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc resistant [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no resistant [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                progbug( ch, "MpMset: Invalid resistant [%s]", ch->name );
            else
                TOGGLE_BIT( victim->resistant, 1 << value );
        }
        return;
    }

    if ( !str_cmp( arg2, "immune" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc immune [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no immune [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                progbug( ch, "MpMset: Invalid immune [%s]", ch->name );
            else
                TOGGLE_BIT( victim->immune, 1 << value );
        }
        return;
    }

    if ( !str_cmp( arg2, "susceptible" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc susceptible [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no susceptible [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_risflag( arg3 );
            if ( value < 0 || value > 31 )
                progbug( ch, "MpMset: Invalid susceptible [%s]", ch->name );
            else
                TOGGLE_BIT( victim->susceptible, 1 << value );
        }
        return;
    }

    if ( !str_cmp( arg2, "part" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc part [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no part [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_partflag( arg3 );
            if ( value < 0 || value > 31 )
                progbug( ch, "MpMset: Invalid part [%s]", ch->name );
            else
                xTOGGLE_BIT( victim->xflags, 1 << value );
        }
        return;
    }

    if ( !str_cmp( arg2, "attack" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc attack [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no attack [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_attackflag( arg3 );
            if ( value < 0 )
                progbug( ch, "MpMset: Invalid attack [%s]", ch->name );
            else
                xTOGGLE_BIT( victim->attacks, value );
        }
        return;
    }

    if ( !str_cmp( arg2, "defense" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc defense [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no defense [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_defenseflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                progbug( ch, "MpMset: Invalid defense [%s]", ch->name );
            else
                xTOGGLE_BIT( victim->defenses, value );
        }
        return;
    }

    if ( !str_cmp( arg2, "pos" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc pos [%s]", ch->name );
            return;
        }
        if ( value < 0 || value > POS_STANDING ) {
            progbug( ch, "MpMset: Invalid pos [%s]", ch->name );
            return;
        }
        victim->position = value;
        return;
    }

    if ( !str_cmp( arg2, "defpos" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc defpos [%s]", ch->name );
            return;
        }
        if ( value < 0 || value > POS_STANDING ) {
            progbug( ch, "MpMset: Invalid defpos [%s]", ch->name );
            return;
        }
        victim->defposition = value;
        return;
    }

    if ( !str_cmp( arg2, "speaks" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no speaks [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_langflag( arg3 );
            v2 = get_langnum( arg3 );
            if ( value == LANG_UNKNOWN )
                progbug( ch, "MpMset: Invalid speaks [%s]", ch->name );
            else if ( !IS_NPC( victim ) ) {
                if ( !( value &= VALID_LANGS ) ) {
                    progbug( ch, "MpMset: Invalid player language [%s]", ch->name );
                    continue;
                }
                if ( v2 == -1 )
                    ch_printf( ch, "Unknown language: %s\r\n", arg3 );
                else
                    TOGGLE_BIT( victim->speaks, 1 << v2 );
            }
            else {
                if ( v2 == -1 )
                    ch_printf( ch, "Unknown language: %s\r\n", arg3 );
                else
                    TOGGLE_BIT( victim->speaks, 1 << v2 );
            }
        }
        if ( !IS_NPC( victim ) ) {
            REMOVE_BIT( victim->speaks, race_table[victim->race]->language );
            if ( !knows_language( victim, victim->speaking, victim ) )
                victim->speaking = race_table[victim->race]->language;
        }
        return;
    }

    if ( !str_cmp( arg2, "speaking" ) ) {
        if ( !IS_NPC( victim ) ) {
            progbug( ch, "MpMset: can't set pc speaking [%s]", ch->name );
            return;
        }
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpMset: no speaking [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_langflag( arg3 );
            if ( value == LANG_UNKNOWN )
                progbug( ch, "MpMset: Invalid speaking [%s]", ch->name );
            else {
                v2 = get_langnum( arg3 );
                if ( v2 == -1 )
                    ch_printf( ch, "Unknown language: %s\r\n", arg3 );
                else
                    TOGGLE_BIT( victim->speaks, 1 << v2 );
            }
        }
        return;
    }

    progbug( ch, "MpMset: Invalid field [%s]", ch->name );
    return;
}

void do_mposet( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    buf[MSL];
    OBJ_DATA               *obj;
    char                    outbuf[MSL];
    int                     value,
                            tmp;

    /*
     * A desc means switched.. too many loopholes if we allow that.. 
     */
    if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) || ch->desc ) {
        error( ch );
        return;
    }

    smash_tilde( argument );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    mudstrlcpy( arg3, argument, MIL );

    if ( !*arg1 ) {
        progbug( ch, "MpOset: no args [%s]", ch->name );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg1 ) ) == NULL ) {
        progbug( ch, "MpOset: no object [%s]", ch->name );
        return;
    }

    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
        progbug( ch, "MpOset: can't set prototype items [%s]", ch->name );
        return;
    }
    separate_obj( obj );
    value = atoi( arg3 );

    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) ) {
        obj->value[0] = value;
        return;
    }

    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) ) {
        obj->value[1] = value;
        return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) ) {
        obj->value[2] = value;
        return;
    }

    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) ) {
        obj->value[3] = value;
        return;
    }

    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) ) {
        obj->value[4] = value;
        return;
    }

    if ( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) ) {
        obj->value[5] = value;
        return;
    }

    if ( !str_cmp( arg2, "value6" ) || !str_cmp( arg2, "v6" ) ) {
        obj->value[6] = value;
        return;
    }

    if ( !str_cmp( arg2, "color" ) ) {
        obj->color = value;
        return;
    }

    if ( !str_cmp( arg2, "type" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpOset: no type [%s]", ch->name );
            return;
        }
        value = get_otype( argument );
        if ( value < 1 ) {
            progbug( ch, "MpOset: Invalid type [%s]", ch->name );
            return;
        }
        obj->item_type = ( short ) value;
        return;
    }

    if ( !str_cmp( arg2, "flags" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpOset: no flags [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_oflag( arg3 );
            if ( value < 0 || value > MAX_BITS )
                progbug( ch, "MpOset: Invalid flag [%s]", ch->name );
            else {
                if ( value == ITEM_PROTOTYPE )
                    progbug( ch, "MpOset: can't set prototype flag [%s]", ch->name );
                else
                    xTOGGLE_BIT( obj->extra_flags, value );
            }
        }
        return;
    }

    if ( !str_cmp( arg2, "wear" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpOset: no wear [%s]", ch->name );
            return;
        }
        while ( argument[0] != '\0' ) {
            argument = one_argument( argument, arg3 );
            value = get_wflag( arg3 );
            if ( value < 0 || value > 31 )
                progbug( ch, "MpOset: Invalid wear [%s]", ch->name );
            else
                TOGGLE_BIT( obj->wear_flags, 1 << value );
        }

        return;
    }

    if ( !str_cmp( arg2, "level" ) ) {
        obj->level = value;
        return;
    }

    if ( !str_cmp( arg2, "weight" ) ) {
        obj->weight = value;
        return;
    }

    if ( !str_cmp( arg2, "cost" ) ) {
        obj->cost = value;
        return;
    }

    if ( !str_cmp( arg2, "timer" ) ) {
        obj->timer = value;
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        STRFREE( obj->name );
        obj->name = STRALLOC( arg3 );
        return;
    }

    if ( !str_cmp( arg2, "short" ) ) {
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( arg3 );
        /*
         * Feature added by Narn, Apr/96 
         * * If the item is not proto, add the word 'rename' to the keywords
         * * if it is not already there.
         */
        if ( str_infix( "mprename", obj->name ) ) {
            snprintf( buf, MSL, "%s %s", obj->name, "mprename" );
            STRFREE( obj->name );
            obj->name = STRALLOC( buf );
        }
        return;
    }

    if ( !str_cmp( arg2, "long" ) ) {
        STRFREE( obj->description );
        mudstrlcpy( buf, arg3, MSL );
        obj->description = STRALLOC( buf );
        return;
    }

    if ( !str_cmp( arg2, "actiondesc" ) ) {
        if ( strstr( arg3, "%n" ) || strstr( arg3, "%d" ) || strstr( arg3, "%l" ) ) {
            progbug( ch, "MpOset: Illegal actiondesc [%s]", ch->name );
            return;
        }
        STRFREE( obj->action_desc );
        obj->action_desc = STRALLOC( arg3 );
        return;
    }

    if ( !str_cmp( arg2, "affect" ) ) {
        AFFECT_DATA            *paf;
        short                   loc;
        int                     bitv;

        argument = one_argument( argument, arg2 );
        if ( arg2[0] == '\0' || !argument || argument[0] == 0 ) {
            progbug( ch, "MpOset: Bad affect syntax [%s]", ch->name );
            send_to_char( "Usage: oset <object> affect <field> <value>\r\n", ch );
            return;
        }
        loc = get_atype( arg2 );
        if ( loc < 1 ) {
            progbug( ch, "MpOset: Invalid affect field [%s]", ch->name );
            return;
        }
        if ( loc >= APPLY_AFFECT && loc < APPLY_WEAPONSPELL ) {
            bitv = 0;
            while ( argument[0] != '\0' ) {
                argument = one_argument( argument, arg3 );
                if ( loc == APPLY_AFFECT )
                    value = get_aflag( arg3 );
                else
                    value = get_risflag( arg3 );
                if ( value < 0 || value > 31 )
                    progbug( ch, "MpOset: bad affect flag [%s]", ch->name );
                else
                    SET_BIT( bitv, 1 << value );
            }
            if ( !bitv )
                return;
            value = bitv;
        }
        else {
            argument = one_argument( argument, arg3 );
            value = atoi( arg3 );
        }
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = loc;
        paf->modifier = value;
        xCLEAR_BITS( paf->bitvector );
        paf->next = NULL;
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        ++top_affect;
        return;
    }

    if ( !str_cmp( arg2, "rmaffect" ) ) {
        AFFECT_DATA            *paf;
        short                   loc,
                                count;

        if ( !argument || argument[0] == '\0' ) {
            progbug( ch, "MpOset: no rmaffect [%s]", ch->name );
            return;
        }
        loc = atoi( argument );
        if ( loc < 1 ) {
            progbug( ch, "MpOset: Invalid rmaffect [%s]", ch->name );
            return;
        }

        count = 0;

        for ( paf = obj->first_affect; paf; paf = paf->next ) {
            if ( ++count == loc ) {
                UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
                DISPOSE( paf );
                send_to_char( "Removed.\r\n", ch );
                --top_affect;
                return;
            }
        }
        progbug( ch, "MpOset: rmaffect not found [%s]", ch->name );
        return;
    }

    /*
     * save some finger-leather
     */
    if ( !str_cmp( arg2, "ris" ) ) {
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "r" ) ) {
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "i" ) ) {
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }
    if ( !str_cmp( arg2, "s" ) ) {
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "ri" ) ) {
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "rs" ) ) {
        snprintf( outbuf, MSL, "%s affect resistant %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }

    if ( !str_cmp( arg2, "is" ) ) {
        snprintf( outbuf, MSL, "%s affect immune %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        snprintf( outbuf, MSL, "%s affect susceptible %s", arg1, arg3 );
        do_mposet( ch, outbuf );
        return;
    }

    /*
     * Make it easier to set special object values by name than number
     *       -Thoric
     */
    tmp = -1;
    switch ( obj->item_type ) {
        case ITEM_WEAPON:
            if ( !str_cmp( arg2, "weapontype" ) ) {
                unsigned int            x;

                value = -1;
                for ( x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); x++ )
                    if ( !str_cmp( arg3, attack_table[x] ) )
                        value = x;
                if ( value < 0 ) {
                    progbug( ch, "MpOset: Invalid weapon type [%s]", ch->name );
                    return;
                }
                tmp = 3;
                break;
            }
            if ( !str_cmp( arg2, "condition" ) )
                tmp = 0;
            break;
        case ITEM_ARMOR:
            if ( !str_cmp( arg2, "condition" ) )
                tmp = 3;
            if ( !str_cmp( arg2, "ac" ) )
                tmp = 1;
            break;
        case ITEM_SALVE:
            if ( !str_cmp( arg2, "slevel" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "maxdoses" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "doses" ) )
                tmp = 2;
            if ( !str_cmp( arg2, "delay" ) )
                tmp = 3;
            if ( !str_cmp( arg2, "spell1" ) )
                tmp = 4;
            if ( !str_cmp( arg2, "spell2" ) )
                tmp = 6;
            if ( tmp >= 4 && tmp <= 6 )
                value = skill_lookup( arg3 );
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            if ( !str_cmp( arg2, "slevel" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "spell1" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "spell2" ) )
                tmp = 2;
            if ( !str_cmp( arg2, "spell3" ) )
                tmp = 3;
            if ( tmp >= 1 && tmp <= 3 )
                value = skill_lookup( arg3 );
            break;
        case ITEM_STAFF:
        case ITEM_WAND:
            if ( !str_cmp( arg2, "slevel" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "spell" ) ) {
                tmp = 3;
                value = skill_lookup( arg3 );
            }
            if ( !str_cmp( arg2, "maxcharges" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "charges" ) )
                tmp = 2;
            break;
        case ITEM_CONTAINER:
            if ( !str_cmp( arg2, "capacity" ) )
                tmp = 0;
            if ( !str_cmp( arg2, "cflags" ) )
                tmp = 1;
            if ( !str_cmp( arg2, "key" ) )
                tmp = 2;
            break;
        case ITEM_SWITCH:
        case ITEM_LEVER:
        case ITEM_PULLCHAIN:
        case ITEM_BUTTON:
            if ( !str_cmp( arg2, "tflags" ) ) {
                tmp = 0;
                value = get_trigflag( arg3 );
            }
            break;
    }
    if ( tmp >= 0 && tmp <= 3 ) {
        obj->value[tmp] = value;
        return;
    }

    progbug( ch, "MpOset: Invalid field [%s]", ch->name );
    return;
}
