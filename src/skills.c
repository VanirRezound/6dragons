/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Player skills module                                                  *
 ***************************************************************************/

#include <limits.h>
#include <string.h>
#include "h/mud.h"
#include "h/hometowns.h"
#include "h/city.h"
#include "h/clans.h"
#include "h/polymorph.h"
#include "h/damage.h"
#include "h/languages.h"
#include "h/new_auth.h"
#include "h/files.h"

/* BARD STUFF - VOLK */
bool                    CAN_PLAY( CHAR_DATA *ch, int type );
bool                    HAS_INSTRUMENT( CHAR_DATA *ch, int type );

int                     get_risflag( char *flag );
void make_blood         args( ( CHAR_DATA *ch ) );
extern const char      *const sec_flags[];
extern int              get_secflag( char *flag );
void                    gen_wilderness_name( CHAR_DATA *ch );
int                     MAX_PC_CLASS;

bool                    can_fly( CHAR_DATA *ch );

const char             *const spell_flag[] = {
    "water", "earth", "air", "astral", "area", "distant", "reverse",
    "noself", "heal", "accumulative", "recastable", "noscribe",
    "nobrew", "group", "object", "character", "secretskill", "pksensitive",
    "stoponfail", "nofight", "nodispel", "randomtarget", "r2", "r3", "r4",
    "r5", "r6", "r7", "r8", "r9", "r10", "r11"
};

const char             *const spell_saves[] =
    { "none", "poison_death", "wands", "para_petri", "breath", "spell_staff" };

const char             *const spell_save_effect[] =
    { "none", "negate", "eightdam", "quarterdam", "halfdam", "3qtrdam",
    "reflect", "absorb"
};

const char             *const spell_damage[] =
    { "none", "fire", "cold", "electricity", "energy", "acid", "poison", "drain" };

const char             *const spell_action[] =
    { "none", "create", "destroy", "resist", "suscept", "divinate", "obscure",
    "change"
};

const char             *const spell_power[] = { "none", "minor", "greater", "major" };

const char             *const spell_class[] =
    { "none", "lunar", "solar", "travel", "summon", "life", "death", "illusion" };

const char             *const target_type[] =
    { "ignore", "offensive", "defensive", "self", "objinv" };

void                    show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch );

int                     ris_save( CHAR_DATA *ch, int chance, int ris );
bool                    check_illegal_psteal( CHAR_DATA *ch, CHAR_DATA *victim );

/* from magic.c */
void                    failed_casting( struct skill_type *skill, CHAR_DATA *ch, CHAR_DATA *victim,
                                        OBJ_DATA *obj );

//External from fight.c 
ch_ret one_hit          args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );

/*
 * Dummy function
 */
void skill_notfound( CHAR_DATA *ch, char *argument )
{
    error( ch );
    return;
}

int get_ssave( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_saves ) / sizeof( spell_saves[0] ); x++ )
        if ( !str_cmp( name, spell_saves[x] ) )
            return x;
    return -1;
}

int get_starget( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( target_type ) / sizeof( target_type[0] ); x++ )
        if ( !str_cmp( name, target_type[x] ) )
            return x;
    return -1;
}

int get_sflag( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_flag ) / sizeof( spell_flag[0] ); x++ )
        if ( !str_cmp( name, spell_flag[x] ) )
            return x;
    return -1;
}

int get_sdamage( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_damage ) / sizeof( spell_damage[0] ); x++ )
        if ( !str_cmp( name, spell_damage[x] ) )
            return x;
    return -1;
}

int get_saction( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_action ) / sizeof( spell_action[0] ); x++ )
        if ( !str_cmp( name, spell_action[x] ) )
            return x;
    return -1;
}

int get_ssave_effect( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_save_effect ) / sizeof( spell_save_effect[0] ); x++ )
        if ( !str_cmp( name, spell_save_effect[x] ) )
            return x;
    return -1;
}

int get_spower( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_power ) / sizeof( spell_power[0] ); x++ )
        if ( !str_cmp( name, spell_power[x] ) )
            return x;
    return -1;
}

int get_sclass( char *name )
{
    unsigned int            x;

    for ( x = 0; x < sizeof( spell_class ) / sizeof( spell_class[0] ); x++ )
        if ( !str_cmp( name, spell_class[x] ) )
            return x;
    return -1;
}

bool is_legal_kill( CHAR_DATA *ch, CHAR_DATA *vch )
{
    if ( IS_NPC( ch ) || IS_NPC( vch ) )
        return TRUE;
    if ( !IS_PKILL( ch ) || !IS_PKILL( vch ) )
        return FALSE;
    if ( ch->pcdata->clan && ch->pcdata->clan == vch->pcdata->clan )
        return FALSE;
    return TRUE;
}

extern char            *target_name;                   /* from magic.c */

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows     -Thoric
 */
bool check_skill( CHAR_DATA *ch, char *command, char *argument )
{
    int                     sn;
    int                     first = gsn_first_skill;
    int                     top = gsn_first_weapon - 1;
    int                     mana,
                            blood;
    struct timeval          time_used;

    /*
     * bsearch for the skill 
     */
    for ( ;; ) {
        sn = ( first + top ) >> 1;

        if ( LOWER( command[0] ) == LOWER( skill_table[sn]->name[0] )
             && !str_prefix( command, skill_table[sn]->name )
             && ( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null )
             && ( can_use_skill( ch, 0, sn ) ) )
/*
  &&  (IS_NPC(ch)
  ||  (ch->pcdata->learned[sn] > 0
  &&   ch->level >= skill_table[sn]->skill_level[ch->Class])))
*/
            break;
        if ( first >= top ) {
/* Volk - basically if we are here, we didn't find the skill within first_skill and first_weapon-1. However, it could still be a 
   song, in which case it should handle like a skill (typing as a command), at least until we create a 'do_play' or whatever.
   Hence the code that follows. */
            sn = 0;
            sn = skill_lookup( command );
            if ( sn > 0 )
                if ( skill_table[sn]->type == SKILL_SONG )
                    break;
//          send_to_char("That is a song, you need to PLAY it.\r\n", ch);

            return FALSE;
        }
        if ( strcmp( command, skill_table[sn]->name ) < 1 )
            top = sn - 1;
        else
            first = sn + 1;
    }

    if ( xIS_SET( ch->act, PLR_LIFE ) ) {
        if ( get_trust( ch ) < LEVEL_IMMORTAL && !IS_NPC( ch ) ) {
            if ( !CAN_LEARN( ch, sn, TRUE ) ) {
                send_to_char( "&BYou can't do that.\r\n", ch );
                return FALSE;
            }
        }
    }
    if ( !check_pos( ch, skill_table[sn]->minimum_position ) )
        return TRUE;

    if ( IS_NPC( ch )
         && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) ) {
        send_to_char( "For some reason, you seem unable to perform that...\r\n", ch );
        act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
        return TRUE;
    }

    /*
     * check if mana is required 
     */
    if ( skill_table[sn]->min_mana ) {
        if ( IS_NPC( ch ) )
            mana = 0;
        else
            mana = skill_table[sn]->min_mana;

        blood = UMAX( 1, ( mana + 4 ) / 8 );           /* NPCs don't have PCDatas. --
                                                        * Altrag */

        if ( IS_BLOODCLASS( ch ) ) {
            if ( ch->blood < blood ) {
                send_to_char( "You don't have enough blood power.\r\n", ch );
                return TRUE;
            }
        }
        else if ( !IS_NPC( ch ) && ch->mana < mana ) {
            send_to_char( "You don't have enough mana.\r\n", ch );
            return TRUE;
        }
    }
    else {
        mana = 0;
        blood = 0;
    }

    /*
     * Is this a real do-fun, or a really a spell?
     */
    if ( !skill_table[sn]->skill_fun ) {
        ch_ret                  retcode = rNONE;
        void                   *vo = NULL;
        CHAR_DATA              *victim = NULL;
        OBJ_DATA               *obj = NULL;

        target_name = ( char * ) "";

        switch ( skill_table[sn]->target ) {
            default:
                bug( "Check_skill: bad target for sn %d.", sn );
                send_to_char( "Something went wrong...\r\n", ch );
                return TRUE;

            case TAR_IGNORE:
                vo = NULL;
                if ( argument[0] == '\0' ) {
                    if ( ( victim = who_fighting( ch ) ) != NULL )
                        target_name = victim->name;
                }
                else
                    target_name = argument;
                break;

            case TAR_CHAR_OFFENSIVE:
                {
                    if ( argument[0] == '\0' && ( victim = who_fighting( ch ) ) == NULL ) {
                        ch_printf( ch,
                                   "Confusion overcomes you as your '%s' has no target.\r\n",
                                   skill_table[sn]->name );
                        return TRUE;
                    }
                    else if ( argument[0] != '\0'
                              && ( victim = get_char_room( ch, argument ) ) == NULL ) {
                        send_to_char( "They aren't here.\r\n", ch );
                        return TRUE;
                    }
                }

                if ( is_safe( ch, victim, TRUE ) )
                    return TRUE;

                if ( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) ) {
                    send_to_char( "You can't target yourself!\r\n", ch );
                    return TRUE;
                }

                if ( !IS_NPC( ch ) ) {
                    if ( !IS_NPC( victim )
                         && ( !who_fighting( victim ) || who_fighting( victim ) != ch ) ) {
                        if ( get_timer( ch, TIMER_PKILLED ) > 0 ) {
                            send_to_char( "You have been killed in the last 5 minutes.\r\n", ch );
                            return TRUE;
                        }

                        if ( get_timer( victim, TIMER_PKILLED ) > 0 ) {
                            send_to_char( "This player has been killed in the last 5 minutes.\r\n",
                                          ch );
                            return TRUE;
                        }

                        if ( xIS_SET( ch->act, PLR_NICE ) && ch != victim ) {
                            send_to_char( "You are too nice to attack another player.\r\n", ch );
                            return TRUE;
                        }

                        if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim ) {
                            send_to_char( "You can't do that on your own follower.\r\n", ch );
                            return TRUE;
                        }
                    }
                }

                check_illegal_pk( ch, victim );
                vo = ( void * ) victim;
                break;

            case TAR_CHAR_DEFENSIVE:
                {
                    if ( argument[0] != '\0' && ( victim = get_char_room( ch, argument ) ) == NULL ) {
                        send_to_char( "They aren't here.\r\n", ch );
                        return TRUE;
                    }
                    if ( !victim )
                        victim = ch;
                }

                if ( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) ) {
                    send_to_char( "You can't target yourself!\r\n", ch );
                    return TRUE;
                }

                vo = ( void * ) victim;
                break;

            case TAR_CHAR_SELF:
                vo = ( void * ) ch;
                break;

            case TAR_OBJ_INV:
                {
                    if ( ( obj = get_obj_carry( ch, argument ) ) == NULL ) {
                        send_to_char( "You can't find that.\r\n", ch );
                        return TRUE;
                    }
                }
                vo = ( void * ) obj;
                break;
        }

        /*
         * waitstate 
         */
        WAIT_STATE( ch, skill_table[sn]->beats );

        if ( ( number_percent(  ) + skill_table[sn]->difficulty * 5 ) >
             ( IS_NPC( ch ) ? 75 : LEARNED( ch, sn ) ) ) {
            failed_casting( skill_table[sn], ch, victim, obj );
            learn_from_failure( ch, sn );
            if ( mana ) {
                if ( IS_BLOODCLASS( ch ) )
                    ch->blood -= blood / 2;
                else
                    ch->mana -= mana / 2;
            }
            return TRUE;
        }
        if ( mana ) {
            if ( IS_BLOODCLASS( ch ) )
                ch->blood = ch->blood - blood;
            else
                ch->mana -= mana;
        }
        start_timer( &time_used );
        retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
        end_timer( &time_used );
        update_userec( &time_used, &skill_table[sn]->userec );

        if ( retcode == rCHAR_DIED || retcode == rERROR )
            return TRUE;

        if ( char_died( ch ) )
            return TRUE;

        if ( retcode == rSPELL_FAILED ) {
            learn_from_failure( ch, sn );
            retcode = rNONE;
        }
        else
            learn_from_success( ch, sn );

        if ( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) ) {
            CHAR_DATA              *vch;
            CHAR_DATA              *vch_next;

            for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
                vch_next = vch->next_in_room;
                if ( victim == vch && !victim->fighting && victim->master != ch ) {
                    retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
                    break;
                }
            }
        }
        return TRUE;
    }

    if ( mana ) {
        if ( IS_BLOODCLASS( ch ) )
            ch->blood = ch->blood - blood;
        else
            ch->mana -= mana;
    }
    ch->last_cmd = skill_table[sn]->skill_fun;
    start_timer( &time_used );
    ( *skill_table[sn]->skill_fun ) ( ch, argument );
    end_timer( &time_used );
    update_userec( &time_used, &skill_table[sn]->userec );

    tail_chain(  );
    return TRUE;
}

void do_skin( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *korps;
    OBJ_DATA               *corpse;
    OBJ_DATA               *obj;
    OBJ_DATA               *skin;
    bool                    found;
    char                   *name;
    char                    buf[MSL];

    found = FALSE;

    if ( !IS_PKILL( ch ) && !IS_IMMORTAL( ch ) ) {
        send_to_char( "Leave the hideous defilings to the killers!\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "Whose corpse do you wish to skin?\r\n", ch );
        return;
    }
    if ( ( corpse = get_obj_here( ch, argument ) ) == NULL ) {
        send_to_char( "You cannot find that here.\r\n", ch );
        return;
    }
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You have no weapon with which to perform this deed.\r\n", ch );
        return;
    }
    if ( corpse->item_type != ITEM_CORPSE_PC ) {
        send_to_char( "You can only skin the bodies of player characters.\r\n", ch );
        return;
    }
    if ( obj->value[3] != 1 && obj->value[3] != 2 && obj->value[3] != 3 && obj->value[3] != 11 ) {
        send_to_char( "There is nothing you can do with this corpse.\r\n", ch );
        return;
    }
    if ( get_obj_index( OBJ_VNUM_SKIN ) == NULL ) {
        bug( "%s", "Vnum 23 (OBJ_VNUM_SKIN) not found for do_skin!" );
        return;
    }
    korps = create_object( get_obj_index( OBJ_VNUM_CORPSE_PC ), 0 );
    skin = create_object( get_obj_index( OBJ_VNUM_SKIN ), 0 );
    name = IS_NPC( ch ) ? korps->short_descr : corpse->short_descr;
    snprintf( buf, MSL, skin->short_descr, name );
    if ( VLD_STR( skin->short_descr ) )
        STRFREE( skin->short_descr );
    skin->short_descr = STRALLOC( buf );
    snprintf( buf, MSL, skin->description, name );
    if ( VLD_STR( skin->description ) )
        STRFREE( skin->description );
    skin->description = STRALLOC( buf );
    act( AT_BLOOD, "$n strips the skin from $p.", ch, corpse, NULL, TO_ROOM );
    act( AT_BLOOD, "You strip the skin from $p.", ch, corpse, NULL, TO_CHAR );
    obj_to_char( skin, ch );
}

/*
 * Lookup a skills information
 * High god command
 */
void do_slookup( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL],
                            arg[MIL];
    int                     sn,
                            iClass,
                            count = 0;
    SKILLTYPE              *skill = NULL;

    one_argument( argument, arg );

    if ( !VLD_STR( arg ) ) {
        send_to_char( "Syntax: slookup (skill/spell)\r\n", ch );
        send_to_char( "      : slookup (sn) teachers\r\n", ch );
        return;
    }
    if ( !str_cmp( arg, "all" ) ) {
        for ( sn = 0; sn < top_sn && skill_table[sn] && skill_table[sn]->name; sn++ )
            pager_printf( ch, "Sn: %4d Slot: %4d Skill/spell: '%-20s' Damtype: %s\r\n",
                          sn, skill_table[sn]->slot, skill_table[sn]->name,
                          spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
    }
    else if ( !str_cmp( arg, "herbs" ) ) {
        for ( sn = 0; sn < top_herb && herb_table[sn] && herb_table[sn]->name; sn++ )
            pager_printf( ch, "%d) %s\r\n", sn, herb_table[sn]->name );
    }
    else {
        SMAUG_AFF              *aff;
        int                     cnt = 0;

        if ( arg[0] == 'h' && is_number( arg + 1 ) ) {
            sn = atoi( arg + 1 );
            if ( !IS_VALID_HERB( sn ) ) {
                send_to_char( "Invalid herb.\r\n", ch );
                return;
            }
            skill = herb_table[sn];
        }
        else if ( is_number( arg ) ) {
            sn = atoi( arg );
            if ( ( skill = get_skilltype( sn ) ) == NULL ) {
                send_to_char( "Invalid sn.\r\n", ch );
                return;
            }
            sn %= 1000;
        }
        else if ( ( sn = skill_lookup( arg ) ) >= 0 )
            skill = skill_table[sn];
        else if ( ( sn = herb_lookup( arg ) ) >= 0 )
            skill = herb_table[sn];
        else {
            send_to_char( "No such skill, spell, proficiency or tongue.\r\n", ch );
            return;
        }
        if ( !skill ) {
            send_to_char( "Not created yet.\r\n", ch );
            return;
        }

        if ( !str_cmp( argument, "teachers" ) ) {
            return;
        }

        ch_printf( ch, "Sn: %4d Slot: %4d %s: '%-20s'\r\n", sn, skill->slot,
                   skill_tname[skill->type], skill->name );
        if ( skill->info )
            ch_printf( ch, "DamType: %s  ActType: %s   ClassType: %s   PowerType: %s\r\n",
                       spell_damage[SPELL_DAMAGE( skill )],
                       spell_action[SPELL_ACTION( skill )],
                       spell_class[SPELL_CLASS( skill )], spell_power[SPELL_POWER( skill )] );
        if ( !xIS_EMPTY( skill->flags ) ) {
            int                     x;

            mudstrlcpy( buf, "Flags:", MSL );
            for ( x = 0; x < MAX_SKELL_FLAG; x++ )
                if ( SPELL_FLAG( skill, x ) ) {
                    mudstrlcat( buf, " ", MSL );
                    mudstrlcat( buf, spell_flag[x], MSL );
                }
            mudstrlcat( buf, "\r\n", MSL );
            send_to_char( buf, ch );
        }
        ch_printf( ch, "Saves: %s  SaveEffect: %s\r\n", spell_saves[( int ) skill->saves],
                   spell_save_effect[SPELL_SAVE( skill )] );

        ch_printf( ch, "Instrumental: %-5s  Verbal: %-5s\r\n",
                   skill->somatic ? "TRUE" : "FALSE", skill->verbal ? "TRUE" : "FALSE" );

        if ( skill->difficulty != '\0' )
            ch_printf( ch, "Difficulty: %d\r\n", ( int ) skill->difficulty );

        ch_printf( ch,
                   "Type: %s  Target: %s  Minpos: %d  Mana: %d  Beats: %d  Range: %d\r\n",
                   skill_tname[skill->type],
                   target_type[URANGE( TAR_IGNORE, skill->target, TAR_OBJ_INV )],
                   skill->minimum_position, skill->min_mana, skill->beats, skill->range );
        ch_printf( ch,
                   "Flags: %s  Value: %d  Info: %d  Min_dist: %d  Max_dist: %d  Code: %s\r\n",
                   print_bitvector( &skill->flags ), skill->value, skill->info,
                   skill->min_dist, skill->max_dist,
                   skill->
                   skill_fun ? skill_name( skill->skill_fun ) : spell_name( skill->spell_fun ) );
        ch_printf( ch, "Sectors Allowed: %s\n",
                   skill->spell_sector ? flag_string( skill->spell_sector, sec_flags ) : "All" );
        ch_printf( ch, "Dammsg: %s\r\nWearoff: %s\n",
                   VLD_STR( skill->noun_damage ) ? skill->noun_damage : "(none set)",
                   VLD_STR( skill->msg_off ) ? skill->msg_off : "(none set)" );
        if ( skill->dice && skill->dice[0] != '\0' )
            ch_printf( ch, "Dice: %s\r\n", skill->dice );
        if ( VLD_STR( skill->teachers ) )
            ch_printf( ch, "Teachers: %s\r\n", skill->teachers );
        if ( VLD_STR( skill->components ) )
            ch_printf( ch, "\r\nComponents: %s\r\n", skill->components );
        if ( skill->participants )
            ch_printf( ch, "Participants: %d\r\n", ( int ) skill->participants );
        if ( skill->userec.num_uses )
            send_timer( &skill->userec, ch );
        for ( aff = skill->affects; aff; aff = aff->next ) {
            if ( aff == skill->affects )
                send_to_char( "\r\n", ch );
            snprintf( buf, MSL, "Affect %d", ++cnt );
            if ( aff->location ) {
                mudstrlcat( buf, " modifies ", MSL );
                mudstrlcat( buf, a_types[aff->location % REVERSE_APPLY], MSL );
                mudstrlcat( buf, " by '", MSL );
                mudstrlcat( buf, aff->modifier, MSL );
                if ( aff->bitvector != -1 )
                    mudstrlcat( buf, "' and", MSL );
                else
                    mudstrlcat( buf, "'", MSL );
            }
            if ( aff->bitvector != -1 ) {
                mudstrlcat( buf, " applies ", MSL );
                mudstrlcat( buf, a_flags[aff->bitvector], MSL );
            }
            if ( VLD_STR( aff->duration ) ) {
                mudstrlcat( buf, " for '", MSL );
                mudstrlcat( buf, aff->duration, MSL );
                mudstrlcat( buf, "' rounds", MSL );
            }
            if ( aff->location >= REVERSE_APPLY )
                mudstrlcat( buf, " (affects caster only)", MSL );
            mudstrlcat( buf, "\r\n", MSL );
            send_to_char( buf, ch );
            if ( !aff->next )
                send_to_char( "\r\n", ch );
        }
        if ( VLD_STR( skill->hit_char ) )
            ch_printf( ch, "Hitchar   : %s\r\n", skill->hit_char );
        if ( VLD_STR( skill->hit_vict ) )
            ch_printf( ch, "Hitvict   : %s\r\n", skill->hit_vict );
        if ( VLD_STR( skill->hit_room ) )
            ch_printf( ch, "Hitroom   : %s\r\n", skill->hit_room );
        if ( VLD_STR( skill->hit_dest ) )
            ch_printf( ch, "Hitdest   : %s\r\n", skill->hit_dest );
        if ( VLD_STR( skill->miss_char ) )
            ch_printf( ch, "Misschar  : %s\r\n", skill->miss_char );
        if ( VLD_STR( skill->miss_vict ) )
            ch_printf( ch, "Missvict  : %s\r\n", skill->miss_vict );
        if ( VLD_STR( skill->miss_room ) )
            ch_printf( ch, "Missroom  : %s\r\n", skill->miss_room );
        if ( VLD_STR( skill->die_char ) )
            ch_printf( ch, "Diechar   : %s\r\n", skill->die_char );
        if ( VLD_STR( skill->die_vict ) )
            ch_printf( ch, "Dievict   : %s\r\n", skill->die_vict );
        if ( VLD_STR( skill->die_room ) )
            ch_printf( ch, "Dieroom   : %s\r\n", skill->die_room );
        if ( VLD_STR( skill->imm_char ) )
            ch_printf( ch, "Immchar   : %s\r\n", skill->imm_char );
        if ( VLD_STR( skill->imm_vict ) )
            ch_printf( ch, "Immvict   : %s\r\n", skill->imm_vict );
        if ( VLD_STR( skill->imm_room ) )
            ch_printf( ch, "Immroom   : %s\r\n", skill->imm_room );

        send_to_char( "--------------------------[CLASS USE]--------------------------\r\n", ch );
        for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
            count++;
            ch_printf( ch, "&Y%2d&W %3.3s) lvl: %3d max: %3d%%", iClass,
                       VLD_STR( class_table[iClass]->
                                who_name ) ? class_table[iClass]->who_name : "???",
                       skill->skill_level[iClass], skill->skill_adept[iClass] );
            if ( count == 2 ) {
                send_to_char( "\r\n", ch );
                count = 0;
            }
            else
                send_to_char( "  ", ch );
        }
        send_to_char( "\r\n", ch );
    }
    return;
}

/* Set a skill's attributes or what skills a player has.
 * High god command, with support for creating skills/spells/herbs/etc
 */
void do_sset( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL];
    CHAR_DATA              *victim;
    int                     value,
                            sn,
                            i;
    bool                    fAll;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) || !VLD_STR( arg2 ) ) {
        send_to_char( "Syntax: sset <victim> <skill> <value>\r\n", ch );
        send_to_char( "or:     sset <victim> all     <value>\r\n", ch );
        send_to_char( "or:     sset list all classes\r\n", ch );
        if ( get_trust( ch ) > LEVEL_AJ_LT ) {
            send_to_char( "or:     sset save skill table\r\n", ch );
            send_to_char( "or:     sset save herb table\r\n", ch );
            send_to_char( "or:     sset create skill 'new skill'\r\n", ch );
            send_to_char( "or:     sset create herb 'new herb'\r\n", ch );
            send_to_char( "or:     sset create ability 'new ability'\r\n", ch );
        }
        if ( get_trust( ch ) > 105 )
            send_to_char( "or:     sset delete <sn>\r\n", ch );
        if ( get_trust( ch ) > LEVEL_AJ_SGT ) {
            send_to_char( "or:     sset <sn>     <field> <value>\r\n", ch );
            send_to_char( "\r\nField being one of:\r\n", ch );
            send_to_char( "  name code target minpos slot mana beats dammsg wearoff minlevel\r\n",
                          ch );
            send_to_char
                ( "  type damtype acttype classtype powertype seffect flag dice value difficulty\r\n",
                  ch );
            send_to_char( "  affect rmaffect adept hit miss die imm (char/vict/room)\r\n", ch );
            send_to_char( "  components teachers racelevel raceadept trade min_dist max_dist\r\n",
                          ch );
            send_to_char( "  sector verbal instrumental\r\n", ch );
            send_to_char
                ( "Affect having the fields: <location> <modfifier> [duration] [bitvector]\r\n",
                  ch );
            send_to_char( "(See AFFECTTYPES for location, and AFFECTED_BY for bitvector)\r\n", ch );
            send_to_char( "Level having the fields:\r\n", ch );
            send_to_char( "reset (resets all levels to 101 and maxadepts to 95 on <sn>\r\n", ch );
            send_to_char
                ( "priest/mage/scout/fighter <level> - sets ALL subclasses to that level.\r\n",
                  ch );
        }
        send_to_char( "Skill being any skill or spell.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg1, "list" ) && !str_cmp( arg2, "all" ) && !str_cmp( argument, "classes" ) ) {
        struct skill_type      *skill;
        bool                    found = FALSE;
        int                     Class;

        send_to_char( "Currently, the following skills/spells are not assigned to a class:\r\n",
                      ch );
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            skill = skill_table[sn];
            found = FALSE;
            for ( Class = 0; Class < MAX_PC_CLASS; Class++ ) {
                if ( skill && skill->skill_level[Class] < LEVEL_IMMORTAL )
                    found = TRUE;
                if ( found == TRUE )
                    break;
            }
            if ( !found && skill )
                ch_printf( ch, "&R%d &w-&W %s\r\n", sn, skill->name ? skill->name : "null skill" );
        }
        return;
    }
    if ( get_trust( ch ) > LEVEL_AJ_LT && !str_cmp( arg1, "save" )
         && !str_cmp( argument, "table" ) ) {
        if ( !str_cmp( arg2, "skill" ) ) {
            send_to_char( "Saving skill table...\r\n", ch );
            save_skill_table(  );
            save_classes(  );
            // save_races(); 
            return;
        }
        if ( !str_cmp( arg2, "herb" ) ) {
            send_to_char( "Saving herb table...\r\n", ch );
            save_herb_table(  );
            return;
        }
    }
    if ( get_trust( ch ) > 105 && !str_cmp( arg1, "delete" ) ) {
        send_to_char( "No delete yet, don't want to f*ck sns. Just rename over the skill.\r\n",
                      ch );
        return;
    }
    if ( get_trust( ch ) > LEVEL_AJ_LT && !str_cmp( arg1, "create" )
         && ( !str_cmp( arg2, "skill" ) || !str_cmp( arg2, "herb" )
              || !str_cmp( arg2, "ability" ) ) ) {
        struct skill_type      *skill;
        short                   type = SKILL_UNKNOWN;

        if ( !str_cmp( arg2, "herb" ) ) {
            if ( top_herb >= MAX_HERB ) {
                ch_printf( ch,
                           "The current top herb is %d, which is the maximum.  "
                           "To add more herbs,\r\nMAX_HERB will have to be "
                           "raised in mud.h, and the mud recompiled.\r\n", top_herb );
                return;
            }
        }
        else if ( top_sn >= MAX_SKILL ) {
            ch_printf( ch,
                       "The current top sn is %d, which is the maximum.  "
                       "To add more skills,\r\nMAX_SKILL will have to be "
                       "raised in mud.h, and the mud recompiled.\r\n", top_sn );
            return;
        }
        CREATE( skill, struct skill_type, 1 );
        skill->slot = 0;
        skill_table[top_sn++] = skill;
        skill->min_mana = 0;
        skill->name = STRALLOC( argument );
        skill->spell_fun = spell_smaug;
        skill->type = type;
        skill->spell_sector = 0;

        if ( !str_cmp( arg2, "ability" ) )
            skill->type = SKILL_RACIAL;
        for ( i = 0; i < MAX_PC_CLASS; i++ ) {
            skill->skill_level[i] = LEVEL_IMMORTAL;
            skill->skill_adept[i] = 95;
        }
        for ( i = 0; i < MAX_PC_RACE; i++ ) {
            skill->race_level[i] = LEVEL_IMMORTAL;
            skill->race_adept[i] = 95;
        }
        send_to_char( "Done.\r\n", ch );
        return;
    }
    if ( arg1[0] == 'h' )
        sn = atoi( arg1 + 1 );
    else
        sn = atoi( arg1 );
    if ( get_trust( ch ) > LEVEL_AJ_SGT
         && ( ( arg1[0] == 'h' && is_number( arg1 + 1 ) && ( sn = atoi( arg1 + 1 ) ) >= 0 )
              || ( is_number( arg1 ) && ( sn = atoi( arg1 ) ) >= 0 ) ) ) {
        struct skill_type      *skill;

        if ( arg1[0] == 'h' ) {
            if ( sn >= top_herb ) {
                send_to_char( "Herb number out of range.\r\n", ch );
                return;
            }
            skill = herb_table[sn];
        }
        else {
            if ( ( skill = get_skilltype( sn ) ) == NULL ) {
                send_to_char( "Skill number out of range.\r\n", ch );
                return;
            }
            sn %= 1000;
        }

        if ( !str_cmp( arg2, "difficulty" ) ) {
            skill->difficulty = atoi( argument );
            send_to_char( "Difficulty is set.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg2, "participants" ) ) {
            skill->participants = atoi( argument );
            send_to_char( "Participants is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "damtype" ) ) {
            int                     x = get_sdamage( argument );

            if ( x == -1 )
                send_to_char( "Not a spell damage type.\r\n", ch );
            else {
                SET_SDAM( skill, x );
                send_to_char( "Damtype is set.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "acttype" ) ) {
            int                     x = get_saction( argument );

            if ( x == -1 )
                send_to_char( "Not a spell action type.\r\n", ch );
            else {
                SET_SACT( skill, x );
                send_to_char( "Acttype is set.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "classtype" ) ) {
            int                     x = get_sclass( argument );

            if ( x == -1 )
                send_to_char( "Not a spell class type.\r\n", ch );
            else {
                SET_SCLA( skill, x );
                send_to_char( "Classtype is set.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "powertype" ) ) {
            int                     x = get_spower( argument );

            if ( x == -1 )
                send_to_char( "Not a spell power type.\r\n", ch );
            else {
                SET_SPOW( skill, x );
                send_to_char( "Powertype is set.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "seffect" ) ) {
            int                     x = get_ssave_effect( argument );

            if ( x == -1 )
                send_to_char( "Not a spell save effect type.\r\n", ch );
            else {
                SET_SSAV( skill, x );
                send_to_char( "Seffect is set.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "flag" ) ) {
            char                    buf[MSL];
            int                     x = -1;

            if ( !VLD_STR( argument ) ) {
                send_to_char( "Invalid syntax, please try again.\r\n", ch );
                return;
            }
            while ( VLD_STR( argument ) ) {
                argument = one_argument( argument, buf );
                x = get_sflag( buf );
                if ( x == -1 )
                    send_to_char( "Not a spell flag.\r\n", ch );
                else {
                    xTOGGLE_BIT( skill->flags, x );
                    send_to_char( "Flags are set.\r\n", ch );
                }
            }
            return;
        }
        if ( !str_cmp( arg2, "saves" ) ) {
            int                     x = get_ssave( argument );

            if ( x == -1 )
                send_to_char( "Not a saving type.\r\n", ch );
            else {
                skill->saves = x;
                send_to_char( "Saves are set.\r\n", ch );
            }
            return;
        }

        if ( !str_cmp( arg2, "verbal" ) ) {
            if ( skill->verbal ) {
                skill->verbal = 0;
                send_to_char( "Spell DOES NOT require speech to cast.\r\n", ch );
            }
            else {
                skill->verbal = 1;
                send_to_char( "Spell DOES require speech to cast.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "instrumental" ) ) {
            if ( skill->somatic ) {
                skill->somatic = 0;
                send_to_char( "Song DOES NOT require instruments to sing.\r\n", ch );
            }
            else {
                skill->somatic = 1;
                send_to_char( "Song DOES require instruments to play.\r\n", ch );
            }
            return;

        }

        if ( !str_cmp( arg2, "code" ) ) {
            SPELL_FUN              *spellfun;
            DO_FUN                 *dofun;

            if ( ( spellfun = spell_function( argument ) ) != spell_notfound ) {
                skill->spell_fun = spellfun;
                skill->skill_fun = NULL;
            }
            else if ( ( dofun = skill_function( argument ) ) != skill_notfound ) {
                skill->skill_fun = dofun;
                skill->spell_fun = NULL;
            }
            else {
                send_to_char( "Not a spell or skill.\r\n", ch );
                return;
            }
            send_to_char( "Code is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "target" ) ) {
            int                     x = get_starget( argument );

            if ( x == -1 )
                send_to_char( "Not a valid target type.\r\n", ch );
            else {
                skill->target = x;
                send_to_char( "Target is set.\r\n", ch );
            }
            return;
        }
        if ( !str_cmp( arg2, "minpos" ) ) {
            skill->minimum_position = URANGE( POS_DEAD, atoi( argument ), POS_DRAG );
            send_to_char( "Minpos is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "minlevel" ) ) {
            skill->min_level = URANGE( 1, atoi( argument ), MAX_LEVEL );
            send_to_char( "Minlevel is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "min_dist" ) ) {
            skill->min_dist = atoi( argument );
            send_to_char( "Min distance is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "max_dist" ) ) {
            skill->max_dist = atoi( argument );
            send_to_char( "Max distance is set.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg2, "sector" ) ) {
            char                    tmp_arg[MSL];

            while ( VLD_STR( argument ) ) {
                argument = one_argument( argument, tmp_arg );
                value = get_secflag( tmp_arg );
                if ( value < 0 || value > 31 )
                    ch_printf( ch, "Unknown flag: %s\r\n", tmp_arg );
                else
                    TOGGLE_BIT( skill->spell_sector, ( 1 << value ) );
            }
            send_to_char( "Sector is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "slot" ) ) {
            skill->slot = URANGE( 0, atoi( argument ), 32767 );
            send_to_char( "Slot is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "mana" ) ) {
            skill->min_mana = URANGE( 0, atoi( argument ), 2000 );
            send_to_char( "Mana is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "beats" ) ) {
            skill->beats = URANGE( 0, atoi( argument ), 120 );
            send_to_char( "Beats is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "range" ) ) {
            skill->range = URANGE( 0, atoi( argument ), 20 );
            send_to_char( "Range is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "value" ) ) {
            skill->value = atoi( argument );
            send_to_char( "Value is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "type" ) ) {
            skill->type = get_skill( argument );
            send_to_char( "Type is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "rmaffect" ) ) {
            SMAUG_AFF              *aff = skill->affects;
            SMAUG_AFF              *aff_next;
            int                     num = atoi( argument );
            int                     cnt = 1;

            if ( !aff ) {
                send_to_char( "This spell has no special affects to remove.\r\n", ch );
                return;
            }
            if ( num == 1 ) {
                skill->affects = aff->next;
                STRFREE( aff->duration );
                STRFREE( aff->modifier );
                DISPOSE( aff );
                send_to_char( "Removed.\r\n", ch );
                return;
            }
            for ( ; aff; aff = aff->next ) {
                if ( ++cnt == num && ( aff_next = aff->next ) != NULL ) {
                    aff->next = aff_next->next;
                    STRFREE( aff_next->duration );
                    STRFREE( aff_next->modifier );
                    DISPOSE( aff_next );
                    send_to_char( "Removed.\r\n", ch );
                    return;
                }
            }
            send_to_char( "Not found.\r\n", ch );
            return;
        }
        /*
         * affect <location> <modifier> <duration> <bitvector> 
         */
        if ( !str_cmp( arg2, "affect" ) ) {
            char                    location[MIL],
                                    modifier[MIL],
                                    duration[MIL];
            int                     loc,
                                    bit,
                                    tmpbit;
            SMAUG_AFF              *aff;

            argument = one_argument( argument, location );
            argument = one_argument( argument, modifier );
            argument = one_argument( argument, duration );

            if ( location[0] == '!' )
                loc = get_atype( location + 1 ) + REVERSE_APPLY;
            else
                loc = get_atype( location );
            if ( ( loc % REVERSE_APPLY ) < 0 || ( loc % REVERSE_APPLY ) >= MAX_APPLY_TYPE ) {
                send_to_char( "Unknown affect location.  See AFFECTTYPES.\r\n", ch );
                return;
            }

            int                     modval = 0;

            if ( loc == APPLY_AFFECT || loc == APPLY_EXT_AFFECT ) { /* affect affected
                                                                     * basically,
                                                                     * modifier=floating, 
                                                                     * duration=whatever */
                modval = get_aflag( modifier );

                if ( modval < 0 ) {
                    ch_printf( ch, "Unknown flag: %s\r\n", modifier );
                    return;
                }
            }

            CREATE( aff, SMAUG_AFF, 1 );
            if ( !str_cmp( duration, "0" ) )
                duration[0] = '\0';
            if ( !str_cmp( modifier, "0" ) )
                modifier[0] = '\0';
            aff->duration = STRALLOC( duration );
            aff->location = loc;

            if ( loc == APPLY_AFFECT || loc == APPLY_EXT_AFFECT ) {
                bit = modval;
                mudstrlcpy( modifier, a_flags[modval], MIL );
            }

            if ( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE ) {
                int                     modval = get_risflag( modifier );

                /*
                 * Sanitize the flag input for the modifier if needed -- Samson 
                 */
                if ( modval < 0 )
                    modval = 0;
                mudstrlcpy( modifier, ris_flags[modval], MIL );
            }

            aff->modifier = STRALLOC( modifier );
            aff->bitvector = bit;
            aff->next = skill->affects;
            skill->affects = aff;
            send_to_char( "Affect is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "level" ) ) {
            char                    arg3[MIL];
            int                     Class;

            argument = one_argument( argument, arg3 );

            if ( !str_cmp( arg3, "reset" ) ) {
                for ( Class = 0; Class < MAX_PC_CLASS; Class++ ) {
                    skill->skill_adept[Class] = URANGE( 0, 95, 100 );
                    skill->skill_level[Class] = URANGE( 0, LEVEL_IMMORTAL, MAX_LEVEL );
                }
                send_to_char( "Done, that skill has been cleared.\r\n", ch );
                return;
            }
            else if ( !str_cmp( arg3, "priest" ) ) {
                send_to_char( "Classes changed: ", ch );
                for ( Class = 0; Class < MAX_PC_CLASS; Class++ ) {
                    if ( ch->Class == CLASS_PRIEST ) {
                        skill->skill_adept[Class] = URANGE( 0, 95, 100 );
                        skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
                        ch_printf( ch, "%s ", class_table[Class]->who_name );
                    }
                }
                send_to_char( "\r\nDone.\r\n", ch );
                return;
            }
            else if ( !str_cmp( arg3, "mage" ) ) {
                send_to_char( "Classes changed: ", ch );
                for ( Class = 0; Class < MAX_PC_CLASS; Class++ ) {
                    if ( ch->Class == CLASS_MAGE ) {
                        skill->skill_adept[Class] = URANGE( 0, 95, 100 );
                        skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
                        ch_printf( ch, "%s ", class_table[Class]->who_name );
                    }
                }
                send_to_char( "\r\nDone.\r\n", ch );
                return;
            }
            else if ( !str_cmp( arg3, "scout" ) ) {
                send_to_char( "Classes changed: ", ch );
                for ( Class = 0; Class < MAX_PC_CLASS; Class++ ) {
                    if ( ch->Class == CLASS_THIEF ) {
                        skill->skill_adept[Class] = URANGE( 0, 95, 100 );
                        skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
                        ch_printf( ch, "%s ", class_table[Class]->who_name );
                    }
                }
                send_to_char( "\r\nDone.\r\n", ch );
                return;
            }
            else if ( !str_cmp( arg3, "fighter" ) ) {
                send_to_char( "Classes changed: ", ch );
                for ( Class = 0; Class < MAX_PC_CLASS; Class++ ) {
                    if ( ch->Class == CLASS_WARRIOR ) {
                        skill->skill_adept[Class] = URANGE( 0, 95, 100 );
                        skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
                        ch_printf( ch, "%s ", class_table[Class]->who_name );
                    }
                }
                send_to_char( "\r\nDone.\r\n", ch );
                return;
            }

            Class = atoi( arg3 );
            if ( Class >= MAX_PC_CLASS || Class < 0 )
                send_to_char( "Not a valid class.\r\n", ch );
            else
                skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
            return;
        }
        if ( !str_cmp( arg2, "racelevel" ) ) {
            char                    arg3[MIL];
            int                     race;

            argument = one_argument( argument, arg3 );
            race = atoi( arg3 );
            if ( race >= MAX_PC_RACE || race < 0 )
                send_to_char( "Not a valid race.\r\n", ch );
            else
                skill->race_level[race] = URANGE( 0, atoi( argument ), MAX_LEVEL );
            return;
        }
        if ( !str_cmp( arg2, "adept" ) ) {
            char                    arg3[MIL];
            int                     Class;

            argument = one_argument( argument, arg3 );
            Class = atoi( arg3 );
            if ( Class >= MAX_PC_CLASS || Class < 0 )
                send_to_char( "Not a valid class.\r\n", ch );
            else
                skill->skill_adept[Class] = URANGE( 0, atoi( argument ), 100 );
            return;
        }
        if ( !str_cmp( arg2, "raceadept" ) ) {
            char                    arg3[MIL];
            int                     race;

            argument = one_argument( argument, arg3 );
            race = atoi( arg3 );
            if ( race >= MAX_PC_RACE || race < 0 )
                send_to_char( "Not a valid race.\r\n", ch );
            else
                skill->race_adept[race] = URANGE( 0, atoi( argument ), 100 );
            return;
        }
        if ( !str_cmp( arg2, "name" ) ) {
            STRFREE( skill->name );
            skill->name = STRALLOC( argument );
            send_to_char( "Name is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "dammsg" ) ) {
            STRFREE( skill->noun_damage );
            if ( str_cmp( argument, "clear" ) )
                skill->noun_damage = STRALLOC( argument );
            send_to_char( "Dammsg is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "wearoff" ) ) {
            STRFREE( skill->msg_off );
            if ( str_cmp( argument, "clear" ) )
                skill->msg_off = STRALLOC( argument );
            send_to_char( "Wearoff is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "hitchar" ) ) {
            if ( skill->hit_char )
                STRFREE( skill->hit_char );
            if ( str_cmp( argument, "clear" ) )
                skill->hit_char = STRALLOC( argument );
            send_to_char( "Hitchar is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "hitvict" ) ) {
            if ( skill->hit_vict )
                STRFREE( skill->hit_vict );
            if ( str_cmp( argument, "clear" ) )
                skill->hit_vict = STRALLOC( argument );
            send_to_char( "Hitvict is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "hitroom" ) ) {
            if ( skill->hit_room )
                STRFREE( skill->hit_room );
            if ( str_cmp( argument, "clear" ) )
                skill->hit_room = STRALLOC( argument );
            send_to_char( "Hitroom is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "hitdest" ) ) {
            if ( skill->hit_dest )
                STRFREE( skill->hit_dest );
            if ( str_cmp( argument, "clear" ) )
                skill->hit_dest = STRALLOC( argument );
            send_to_char( "Hitdest is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "misschar" ) ) {
            if ( skill->miss_char )
                STRFREE( skill->miss_char );
            if ( str_cmp( argument, "clear" ) )
                skill->miss_char = STRALLOC( argument );
            send_to_char( "Mischar is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "missvict" ) ) {
            if ( skill->miss_vict )
                STRFREE( skill->miss_vict );
            if ( str_cmp( argument, "clear" ) )
                skill->miss_vict = STRALLOC( argument );
            send_to_char( "Missvict is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "missroom" ) ) {
            if ( skill->miss_room )
                STRFREE( skill->miss_room );
            if ( str_cmp( argument, "clear" ) )
                skill->miss_room = STRALLOC( argument );
            send_to_char( "Missroom is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "diechar" ) ) {
            if ( skill->die_char )
                STRFREE( skill->die_char );
            if ( str_cmp( argument, "clear" ) )
                skill->die_char = STRALLOC( argument );
            send_to_char( "Diechar is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "dievict" ) ) {
            if ( skill->die_vict )
                STRFREE( skill->die_vict );
            if ( str_cmp( argument, "clear" ) )
                skill->die_vict = STRALLOC( argument );
            send_to_char( "Dievict is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "dieroom" ) ) {
            if ( skill->die_room )
                STRFREE( skill->die_room );
            if ( str_cmp( argument, "clear" ) )
                skill->die_room = STRALLOC( argument );
            send_to_char( "Dieroom is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "immchar" ) ) {
            if ( skill->imm_char )
                STRFREE( skill->imm_char );
            if ( str_cmp( argument, "clear" ) )
                skill->imm_char = STRALLOC( argument );
            send_to_char( "Immchar is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "immvict" ) ) {
            if ( skill->imm_vict )
                STRFREE( skill->imm_vict );
            if ( str_cmp( argument, "clear" ) )
                skill->imm_vict = STRALLOC( argument );
            send_to_char( "Immvict is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "immroom" ) ) {
            if ( skill->imm_room )
                STRFREE( skill->imm_room );
            if ( str_cmp( argument, "clear" ) )
                skill->imm_room = STRALLOC( argument );
            send_to_char( "Immroom is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "dice" ) ) {
            if ( skill->dice )
                STRFREE( skill->dice );
            if ( str_cmp( argument, "clear" ) )
                skill->dice = STRALLOC( argument );
            send_to_char( "Dice is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "components" ) ) {
            if ( skill->components )
                STRFREE( skill->components );
            if ( str_cmp( argument, "clear" ) )
                skill->components = STRALLOC( argument );
            send_to_char( "Componets is set.\r\n", ch );
            return;
        }
        if ( !str_cmp( arg2, "teachers" ) ) {
            if ( skill->teachers )
                STRFREE( skill->teachers );
            if ( str_cmp( argument, "clear" ) )
                skill->teachers = STRALLOC( argument );
            send_to_char( "Teachers now set.\r\n", ch );
            return;
        }
        do_sset( ch, ( char * ) "" );
        return;
    }
    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        if ( ( sn = skill_lookup( arg1 ) ) >= 0 ) {
            snprintf( arg1, MIL, "%d %s %s", sn, arg2, argument );
            do_sset( ch, arg1 );
        }
        else
            send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }
    fAll = !str_cmp( arg2, "all" );
    sn = 0;
    if ( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 ) {
        send_to_char( "No such skill or spell.\r\n", ch );
        return;
    }
    /*
     * Snarf the value. 
     */
    if ( !is_number( argument ) ) {
        send_to_char( "Value must be numeric.\r\n", ch );
        return;
    }
    value = atoi( argument );
    if ( value < 0 || value > 100 ) {
        send_to_char( "Value range is 0 to 100.\r\n", ch );
        return;
    }
    if ( fAll ) {
        for ( sn = 0; sn < top_sn; sn++ ) {
            if ( skill_table[sn]->name ) {
                if ( value == 100 && !IS_IMMORTAL( victim ) )
                    victim->pcdata->learned[sn] = get_maxadept( victim, sn, TRUE );
                else {
                    victim->pcdata->learned[sn] = value;
                    victim->pcdata->dlearned[sn] = value;
                }
            }
        }
    }
    else
        victim->pcdata->learned[sn] = value;
    return;
}

void learn_from_success( CHAR_DATA *ch, int sn )
{
    int                     adept,
                            gain,
                            learn,
                            percent,
                            chance;

    if ( IS_NPC( ch ) || ch->pcdata->learned[sn] <= 0 )
        return;

    adept = get_maxadept( ch, sn, TRUE );

    if ( IS_GROUPED( ch ) ) {
        if ( SPELL_FLAG( skill_table[sn], SF_HEAL ) && ch->pcdata->learned[sn] >= adept
             && !IS_IMMORTAL( ch ) ) {
            if ( ch->level < 10 ) {
                gain = ( int ) number_range( 100, 5000 );
            }
            else if ( ch->level >= 10 && ch->level < 20 ) {
                gain = ( int ) number_range( 1000, 8000 );
            }
            else if ( ch->level >= 20 && ch->level < 30 ) {
                gain = ( int ) number_range( 1500, 12000 );
            }
            else if ( ch->level >= 30 && ch->level < 50 ) {
                gain = ( int ) number_range( 2000, 18000 );
            }
            else if ( ch->level >= 50 && ch->level < 80 ) {
                gain = ( int ) number_range( 3000, 25000 );
            }
            else if ( ch->level >= 80 && ch->level < 100 ) {
                gain = ( int ) number_range( 5000, 30000 );
            }
            gain_exp( ch, gain );
            return;
        }
    }
    short                   harder;
    short                   lucky;

    if ( get_curr_lck( ch ) > 16 ) {
        lucky = 3;
    }
    else if ( get_curr_lck( ch ) == 16 ) {
        lucky = 2;
    }
    else if ( get_curr_lck( ch ) < 16 ) {
        lucky = 1;
    }

    if ( ch->pcdata->learned[sn] < 50 ) {
        harder = number_range( 7, 20 ) + lucky;
    }
    else if ( ch->pcdata->learned[sn] > 49 && ( ch->pcdata->learned[sn] < 80 ) ) {
        harder = number_range( 10, 20 ) + lucky;
    }
    else {
        harder = number_range( 15, 20 ) + lucky;
    }

    if ( sn == gsn_forge || sn == gsn_tan || sn == gsn_bake || sn == gsn_mix ||
         sn == gsn_mine || sn == gsn_gather || sn == gsn_hunt ) {
        harder = 20;
    }

    if ( ch->pcdata->learned[sn] < adept && harder > 18 ) {
        chance = ch->pcdata->learned[sn] + ( 5 * skill_table[sn]->difficulty );
        percent = number_percent(  );

        if ( percent >= chance )
            learn = 1;
        else if ( chance - percent > 25 )
            return;
        else
            learn = 1;
        ch->pcdata->learned[sn] = UMIN( adept, ch->pcdata->learned[sn] + learn );
        if ( ch->pcdata->learned[sn] == adept ) {      /* fully learned! */
            set_char_color( AT_WHITE, ch );
            ch_printf( ch, "You are now an adept of %s!\r\n", skill_table[sn]->name );
        }
        // Now we have result, which should tell us which is the highest VALID skill
        // level. 

        if ( ch->level > 49 && ch->level < 100 ) {
            if ( IS_THIRDCLASS( ch ) )
                gain = ( int ) number_range( 40000, 45000 );
            else if ( IS_SECONDCLASS( ch ) )
                gain = ( int ) number_range( 35000, 39000 );
            else
                gain = ( int ) number_range( 20000, 25000 );
        }
        if ( ch->level > 29 && ch->level < 50 ) {
            if ( IS_THIRDCLASS( ch ) )
                gain = ( int ) number_range( 20000, 25000 );
            else if ( IS_SECONDCLASS( ch ) )
                gain = ( int ) number_range( 15000, 19000 );
            else
                gain = ( int ) number_range( 10000, 15000 );
        }
        else if ( ch->level < 30 && ch->level > 19 ) {
            if ( IS_THIRDCLASS( ch ) )
                gain = ( int ) number_range( 12000, 15000 );
            else if ( IS_SECONDCLASS( ch ) )
                gain = ( int ) number_range( 10000, 12000 );
            else
                gain = ( int ) number_range( 8000, 10000 );
        }
        else if ( ch->level < 20 ) {
            if ( IS_THIRDCLASS( ch ) )
                gain = ( int ) number_range( 600, 1500 );
            else if ( IS_SECONDCLASS( ch ) )
                gain = ( int ) number_range( 300, 500 );
            else
                gain = ( int ) number_range( 200, 350 );
        }

        // Lets stop losing exp from the mess above. -Taon
        if ( gain < 1 )
            gain = 1;
        gain_exp( ch, gain );
    }
    return;
}

void learn_from_failure( CHAR_DATA *ch, int sn )
{
//   int luck;
    int                     adept,
                            sklvl,
                            learn;

    if ( IS_NPC( ch ) || ch->pcdata->learned[sn] <= 0 )
        return;

    adept = get_maxadept( ch, sn, TRUE );
    sklvl = get_maxskill( ch, sn, TRUE );
/*
   luck = number_chance ( 1, 80 );

   if ( ch->pcdata->learned[sn] < 20 )
      luck = luck - 10;

   if ( luck <= 54 )
      return;

   if ( luck >= 55 )
   {
      if ( ch->pcdata->learned[sn] < adept )
      {
         learn = 1;
         ch->pcdata->learned[sn] = UMIN ( adept, ch->pcdata->learned[sn] + learn );
         act ( AT_WHITE, "You learned something new from this failed attempt.", ch, NULL,
               NULL, TO_CHAR );
      }
      return;
   }
*/
    if ( sn == gsn_forge || sn == gsn_tan || sn == gsn_bake || sn == gsn_mix || sn == gsn_mine
         || sn == gsn_gather || sn == gsn_hunt ) {
        if ( ch->pcdata->learned[sn] < adept ) {
            learn = 1;
            ch->pcdata->learned[sn] = UMIN( adept, ch->pcdata->learned[sn] + learn );
            act( AT_WHITE, "You learned something new from this failed attempt.", ch, NULL,
                 NULL, TO_CHAR );
        }
    }
    return;
}

void do_gouge( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    short                   dam;
    int                     chance;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( !can_use_skill( ch, 0, gsn_gouge ) ) {
        send_to_char( "You do not yet know of this skill.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You can't get close enough while mounted.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    chance = ( ( get_curr_dex( victim ) - get_curr_dex( ch ) ) * 10 ) + 10;
    if ( !IS_NPC( ch ) && !IS_NPC( victim ) )
        chance += sysdata.gouge_plr_vs_plr;
    if ( victim->fighting && victim->fighting->who != ch )
        chance += sysdata.gouge_nontank;
    if ( can_use_skill( ch, ( number_percent(  ) + chance ), gsn_gouge ) ) {
        dam = number_range( 5, ch->level );
        global_retcode = damage( ch, victim, dam, gsn_gouge );
        if ( global_retcode == rNONE ) {
            if ( !IS_AFFECTED( victim, AFF_BLINDNESS ) ) {
                af.type = gsn_blindness;
                af.location = APPLY_HITROLL;
                af.modifier = -6;
                if ( !IS_NPC( victim ) && !IS_NPC( ch ) )
                    af.duration = ( ch->level + 10 ) / get_curr_con( victim );
                else
                    af.duration = 3 + ( ch->level / 15 );
                af.bitvector = meb( AFF_BLINDNESS );
                af.level = ch->level;
                affect_join( victim, &af );
                act( AT_SKILL, "You can't see a thing!", victim, NULL, NULL, TO_CHAR );
            }
            WAIT_STATE( ch, PULSE_VIOLENCE );
            if ( !IS_NPC( ch ) && !IS_NPC( victim ) ) {
                if ( number_bits( 1 ) == 0 ) {
                    ch_printf( ch, "%s looks momentarily dazed.\r\n", victim->name );
                    send_to_char( "You are momentarily dazed ...\r\n", victim );
                    WAIT_STATE( victim, PULSE_VIOLENCE );
                }
            }
            else
                WAIT_STATE( victim, PULSE_VIOLENCE );
            /*
             * Taken out by request - put back in by Thoric
             * * This is how it was designed.  You'd be a tad stunned
             * * if someone gouged you in the eye.
             * * Mildly modified by Blodkai, Feb 1998 at request of
             * * of pkill Conclave (peaceful use remains the same)
             */
        }
        else if ( global_retcode == rVICT_DIED ) {
            act( AT_BLOOD,
                 "Your fingers plunge into your victim's brain, causing immediate death!",
                 ch, NULL, NULL, TO_CHAR );
        }
        if ( global_retcode != rCHAR_DIED && global_retcode != rBOTH_DIED )
            learn_from_success( ch, gsn_gouge );
    }
    else {
        WAIT_STATE( ch, skill_table[gsn_gouge]->beats );
        global_retcode = damage( ch, victim, 0, gsn_gouge );
        learn_from_failure( ch, gsn_gouge );
    }

    return;
}

void do_detrap( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *trap;
    int                     percent;
    bool                    found = FALSE;

    switch ( ch->substate ) {
        default:
            if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
                send_to_char( "You can't concentrate enough for that.\r\n", ch );
                return;
            }
            argument = one_argument( argument, arg );
            if ( !can_use_skill( ch, 0, gsn_detrap ) ) {
                send_to_char( "You do not yet know of this skill.\r\n", ch );
                return;
            }
            if ( arg[0] == '\0' ) {
                send_to_char( "Detrap what?\r\n", ch );
                return;
            }
            if ( ms_find_obj( ch ) )
                return;
            found = FALSE;
            if ( ch->mount ) {
                send_to_char( "You can't do that while mounted.\r\n", ch );
                return;
            }
            if ( !ch->in_room->first_content ) {
                send_to_char( "You can't find that here.\r\n", ch );
                return;
            }
            for ( obj = ch->in_room->first_content; obj; obj = obj->next_content ) {
                if ( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) ) {
                    found = TRUE;
                    break;
                }
            }
            if ( !found ) {
                send_to_char( "You can't find that here.\r\n", ch );
                return;
            }
            act( AT_ACTION, "You carefully begin your attempt to remove a trap from $p...",
                 ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n carefully attempts to remove a trap from $p...", ch, obj,
                 NULL, TO_ROOM );
            ch->alloc_ptr = str_dup( obj->name );
            add_timer( ch, TIMER_DO_FUN, 3, do_detrap, 1 );
/*      WAIT_STATE(ch, skill_table[gsn_detrap]->beats); */
            return;
        case 1:
            if ( !ch->alloc_ptr ) {
                send_to_char( "Your detrapping was interrupted!\r\n", ch );
                bug( "%s", "do_detrap: ch->alloc_ptr NULL!" );
                return;
            }
            mudstrlcpy( arg, ch->alloc_ptr, MIL );
            DISPOSE( ch->alloc_ptr );
            ch->alloc_ptr = NULL;
            ch->substate = SUB_NONE;
            break;
        case SUB_TIMER_DO_ABORT:
            DISPOSE( ch->alloc_ptr );
            ch->substate = SUB_NONE;
            send_to_char( "You carefully stop what you were doing.\r\n", ch );
            return;
    }

    if ( !ch->in_room->first_content ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return;
    }
    for ( obj = ch->in_room->first_content; obj; obj = obj->next_content ) {
        if ( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) ) {
            found = TRUE;
            break;
        }
    }
    if ( !found ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return;
    }
    if ( ( trap = get_trap( obj ) ) == NULL ) {
        send_to_char( "You find no trap on that.\r\n", ch );
        return;
    }

    percent = number_percent(  ) - ( ch->level / 15 ) - ( get_curr_lck( ch ) - 16 );

    separate_obj( obj );
    if ( !can_use_skill( ch, percent, gsn_detrap ) ) {
        send_to_char( "Ooops!\r\n", ch );
        spring_trap( ch, trap );
        learn_from_failure( ch, gsn_detrap );
        return;
    }

    extract_obj( trap );

    send_to_char( "You successfully remove a trap.\r\n", ch );
    learn_from_success( ch, gsn_detrap );
    return;
}

void do_dig( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *startobj;
    bool                    found,
                            shovel;
    EXIT_DATA              *pexit;
    short                   chance;

    chance = number_range( 1, 10 );

    switch ( ch->substate ) {
        default:
            if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
                send_to_char( "You can't concentrate enough for that.\r\n", ch );
                return;
            }
            if ( ch->mount ) {
                send_to_char( "You can't do that while mounted.\r\n", ch );
                return;
            }
            one_argument( argument, arg );
            if ( arg[0] != '\0' ) {
                if ( ( pexit = find_door( ch, arg, TRUE ) ) == NULL && get_dir( arg ) == -1 ) {
                    send_to_char( "What direction is that?\r\n", ch );
                    return;
                }
                if ( pexit ) {
                    if ( !IS_SET( pexit->exit_info, EX_DIG ) ) {
                        if ( IS_SET( pexit->exit_info, EX_CLOSED ) )
                            send_to_char( "You can't dig out or under doors.\r\n", ch );
                        else
                            send_to_char( "There is no need to dig out that exit.\r\n", ch );
                        return;
                    }
                }
            }
            else {
                switch ( ch->in_room->sector_type ) {
                    case SECT_CITY:
                    case SECT_ROAD:
                    case SECT_HROAD:
                    case SECT_VROAD:
                    case SECT_INSIDE:
                        send_to_char( "The floor is too hard to dig through.\r\n", ch );
                        return;
                    case SECT_WATER_SWIM:
                    case SECT_WATER_NOSWIM:
                    case SECT_UNDERWATER:
                        send_to_char( "You cannot dig here.\r\n", ch );
                        return;
                    case SECT_AIR:
                        send_to_char( "What?  In the air?!\r\n", ch );
                        return;
                }
            }
            add_timer( ch, TIMER_DO_FUN, UMIN( 16 / 10, 3 ), do_dig, 1 );
            ch->alloc_ptr = str_dup( arg );
            send_to_char( "You begin digging...\r\n", ch );
            act( AT_PLAIN, "$n begins digging...", ch, NULL, NULL, TO_ROOM );
            return;

        case 1:
            if ( !ch->alloc_ptr ) {
                send_to_char( "Your digging was interrupted!\r\n", ch );
                act( AT_PLAIN, "$n's digging was interrupted!", ch, NULL, NULL, TO_ROOM );
                bug( "%s", "do_dig: alloc_ptr NULL" );
                return;
            }
            mudstrlcpy( arg, ch->alloc_ptr, MIL );
            DISPOSE( ch->alloc_ptr );
            break;

        case SUB_TIMER_DO_ABORT:
            DISPOSE( ch->alloc_ptr );
            ch->substate = SUB_NONE;
            send_to_char( "You stop digging...\r\n", ch );
            act( AT_PLAIN, "$n stops digging...", ch, NULL, NULL, TO_ROOM );
            return;
    }

    ch->substate = SUB_NONE;

    /*
     * not having a shovel makes it harder to succeed 
     */
    shovel = FALSE;
    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
        if ( obj->item_type == ITEM_SHOVEL ) {
            shovel = TRUE;
            break;
        }

    /*
     * dig out an EX_DIG exit... 
     */
    if ( arg[0] != '\0' ) {
        if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL && IS_SET( pexit->exit_info, EX_DIG ) ) {
            /*
             * 4 times harder to dig open a passage without a shovel 
             */
            if ( shovel && chance > 5 || chance > 8 && !shovel ) {
                EXIT_DATA              *newexit;

                REMOVE_BIT( pexit->exit_info, EX_DIG );
                send_to_char( "You dig open a passageway!\r\n", ch );
                act( AT_PLAIN, "$n digs open a passageway!", ch, NULL, NULL, TO_ROOM );

                if ( ( newexit = get_exit( pexit->to_room, rev_dir[pexit->vdir] ) ) != NULL
                     && IS_SET( newexit->exit_info, EX_DIG ) ) {
                    REMOVE_BIT( newexit->exit_info, EX_DIG );
                    CHAR_DATA              *rch,
                                           *rch_next;

                    for ( rch = pexit->to_room->first_person; rch; rch = rch_next ) {
                        rch_next = rch->next_in_room;
                        ch_printf( rch, "The collapsed exit to the %s is dug open!\r\n",
                                   dir_name[newexit->vdir] );
                    }
                }

                return;
            }
        }
        send_to_char( "Your dig did not discover any exit...\r\n", ch );
        act( AT_PLAIN, "$n's dig did not discover any exit...", ch, NULL, NULL, TO_ROOM );
        return;
    }

    startobj = ch->in_room->first_content;
    found = FALSE;

    for ( obj = startobj; obj; obj = obj->next_content ) {
        /*
         * twice as hard to find something without a shovel 
         */
        if ( IS_OBJ_STAT( obj, ITEM_BURIED ) && ( shovel && chance > 6 || chance > 8 && !shovel ) ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "Your dig uncovered nothing.\r\n", ch );
        act( AT_PLAIN, "$n's dig uncovered nothing.", ch, NULL, NULL, TO_ROOM );
        return;
    }

    separate_obj( obj );
    xREMOVE_BIT( obj->extra_flags, ITEM_BURIED );
    act( AT_SKILL, "Your dig uncovered $p!", ch, obj, NULL, TO_CHAR );
    act( AT_SKILL, "$n's dig uncovered $p!", ch, obj, NULL, TO_ROOM );
    if ( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
        adjust_favor( ch, 14, 1 );
}

void do_search( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *container;
    OBJ_DATA               *startobj;
    int                     percent,
                            door;

    door = -1;
    switch ( ch->substate ) {
        default:
            if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
                send_to_char( "You can't concentrate enough for that.\r\n", ch );
                return;
            }
            if ( ch->mount ) {
                send_to_char( "You can't do that while mounted.\r\n", ch );
                return;
            }
            argument = one_argument( argument, arg );
            if ( arg[0] != '\0' && ( door = get_door( arg ) ) == -1 ) {
                container = get_obj_here( ch, arg );
                if ( !container ) {
                    send_to_char( "You can't find that here.\r\n", ch );
                    return;
                }
                if ( container->item_type != ITEM_CONTAINER ) {
                    send_to_char( "You can't search in that!\r\n", ch );
                    return;
                }
                if ( IS_SET( container->value[1], CONT_CLOSED ) ) {
                    send_to_char( "It is closed.\r\n", ch );
                    return;
                }
            }
            add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_search]->beats / 10, 3 ),
                       do_search, 1 );
            send_to_char( "You begin your search...\r\n", ch );
            ch->alloc_ptr = str_dup( arg );
            return;

        case 1:
            if ( !ch->alloc_ptr ) {
                send_to_char( "Your search was interrupted!\r\n", ch );
                bug( "%s", "do_search: alloc_ptr NULL" );
                return;
            }
            mudstrlcpy( arg, ch->alloc_ptr, MIL );
            DISPOSE( ch->alloc_ptr );
            break;
        case SUB_TIMER_DO_ABORT:
            DISPOSE( ch->alloc_ptr );
            ch->substate = SUB_NONE;
            send_to_char( "You stop your search...\r\n", ch );
            return;
    }
    ch->substate = SUB_NONE;
    if ( arg[0] == '\0' )
        startobj = ch->in_room->first_content;
    else {
        if ( ( door = get_door( arg ) ) != -1 )
            startobj = NULL;
        else {
            container = get_obj_here( ch, arg );
            if ( !container ) {
                send_to_char( "You can't find that here.\r\n", ch );
                return;
            }
            startobj = container->first_content;
        }
    }

    if ( ( !startobj && door == -1 ) || IS_NPC( ch ) ) {
        send_to_char( "You find nothing.\r\n", ch );
        learn_from_failure( ch, gsn_search );
        return;
    }

    percent = number_percent(  ) + number_percent(  ) - ( ch->level / 10 );

    if ( door != -1 ) {
        EXIT_DATA              *pexit;

        if ( ( pexit = get_exit( ch->in_room, door ) ) != NULL
             && IS_SET( pexit->exit_info, EX_SECRET )
             && IS_SET( pexit->exit_info, EX_xSEARCHABLE )
             && can_use_skill( ch, percent, gsn_search ) ) {
            act( AT_SKILL, "Your search reveals the $d!", ch, NULL, pexit->keyword, TO_CHAR );
            act( AT_SKILL, "$n finds the $d!", ch, NULL, pexit->keyword, TO_ROOM );
            REMOVE_BIT( pexit->exit_info, EX_SECRET );
            learn_from_success( ch, gsn_search );
            return;
        }
    }
    else
        for ( obj = startobj; obj; obj = obj->next_content ) {
            if ( IS_OBJ_STAT( obj, ITEM_HIDDEN )
                 && can_use_skill( ch, percent, gsn_search ) ) {
                separate_obj( obj );
                xREMOVE_BIT( obj->extra_flags, ITEM_HIDDEN );
                act( AT_SKILL, "Your search reveals $p!", ch, obj, NULL, TO_CHAR );
                act( AT_SKILL, "$n finds $p!", ch, obj, NULL, TO_ROOM );
                learn_from_success( ch, gsn_search );
                return;
            }
        }

    send_to_char( "You find nothing.\r\n", ch );
    learn_from_failure( ch, gsn_search );
    return;
}

void do_steal( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim,
                           *mst;
    OBJ_DATA               *obj;
    int                     percent;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Steal what from whom?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "That's pointless.\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "A magical force interrupts you.\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && !IS_NPC( victim ) ) {
        set_char_color( AT_IMMORT, ch );
        send_to_char( "The gods forbid theft between players.\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_steal]->beats );
    percent =
        number_percent(  ) + ( IS_AWAKE( victim ) ? 10 : -50 ) - ( get_curr_lck( ch ) -
                                                                   15 ) +
        ( get_curr_lck( victim ) - 13 );

    /*
     * Changed the level check, made it 10 levels instead of five and made the 
     * victim not attack in the case of a too high level difference.  This is 
     * to allow mobprogs where the mob steals eq without having to put level 
     * checks into the progs.  Also gave the mobs a 10% chance of failure.
     */

    if ( victim->position == POS_FIGHTING || !can_use_skill( ch, percent, gsn_steal ) ) {
        /*
         * Failure.
         */
        send_to_char( "Oops...\r\n", ch );
        act( AT_ACTION, "$n tried to steal from you!\r\n", ch, NULL, victim, TO_VICT );
        act( AT_ACTION, "$n tried to steal from $N.\r\n", ch, NULL, victim, TO_NOTVICT );

        snprintf( buf, MSL, "yell %s is a bloody thief!", ch->name );
        interpret( victim, buf );

        learn_from_failure( ch, gsn_steal );
        if ( !IS_NPC( ch ) ) {
            if ( legal_loot( ch, victim ) ) {
                global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
            }
            else {
                /*
                 * log_string(buf); 
                 */
                if ( IS_NPC( ch ) ) {
                    if ( ( mst = ch->master ) == NULL )
                        return;
                }
                else
                    mst = ch;
                if ( IS_NPC( mst ) )
                    return;
                if ( !xIS_SET( mst->act, PLR_THIEF ) ) {
                    xSET_BIT( mst->act, PLR_THIEF );
                    set_char_color( AT_WHITE, ch );
                    send_to_char
                        ( "A strange feeling grows deep inside you, and a tingle goes up your spine...\r\n",
                          ch );
                    set_char_color( AT_IMMORT, ch );
                    send_to_char
                        ( "A deep voice booms inside your head, 'Thou shall now be known as a lowly thief!'\r\n",
                          ch );
                    set_char_color( AT_WHITE, ch );
                    send_to_char
                        ( "You feel as if your soul has been revealed for all to see.\r\n", ch );
                    save_char_obj( mst );
                }
            }
        }

        return;
    }

    if ( !str_cmp( arg1, "coin" ) || !str_cmp( arg1, "coins" ) ) {
        int                     amount,
                                type = CURR_GOLD;

        for ( amount = FIRST_CURR; amount <= LAST_CURR; amount++ ) {
            type = number_range( FIRST_CURR, LAST_CURR );
            if ( GET_MONEY( victim, type ) )
                break;
        }
        amount = ( int ) ( GET_MONEY( victim, type ) * number_range( 1, 10 ) / 100 );
        if ( amount <= 0 ) {
            send_to_char( "You couldn't get any money.\r\n", ch );
            learn_from_failure( ch, gsn_steal );
            return;
        }

        GET_MONEY( ch, type ) += amount;
        GET_MONEY( victim, type ) -= amount;
        ch_printf( ch, "Aha!  You got %d %s coins.\r\n", amount, curr_types[type] );
        learn_from_success( ch, gsn_steal );
        return;
    }

    if ( ( obj = get_obj_carry( victim, arg1 ) ) == NULL ) {
        send_to_char( "You can't seem to find it.\r\n", ch );
        learn_from_failure( ch, gsn_steal );
        return;
    }

    if ( !can_drop_obj( ch, obj ) || IS_OBJ_STAT( obj, ITEM_INVENTORY )
         || IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) || obj->level > ch->level ) {
        send_to_char( "You can't manage to pry it away.\r\n", ch );
        learn_from_failure( ch, gsn_steal );
        return;
    }

    if ( ch->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( ch ) ) {
        send_to_char( "You have your hands full.\r\n", ch );
        learn_from_failure( ch, gsn_steal );
        return;
    }

    if ( ch->carry_weight + ( get_obj_weight( obj, FALSE ) / obj->count ) > can_carry_w( ch ) ) {
        send_to_char( "You can't carry that much weight.\r\n", ch );
        learn_from_failure( ch, gsn_steal );
        return;
    }

    separate_obj( obj );
    obj_from_char( obj );
    obj_to_char( obj, ch );
    adjust_favor( ch, 9, 1 );
    send_to_char( "Ok.\r\n", ch );
    learn_from_success( ch, gsn_steal );
    return;
}

void do_backstab( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( ch->mount ) {
        send_to_char( "You can't get close enough while mounted.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "Backstab whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "How can you sneak up on yourself?\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    if ( !IS_NPC( ch ) && !IS_NPC( victim ) && xIS_SET( ch->act, PLR_NICE ) ) {
        send_to_char( "You are too nice to do that.\r\n", ch );
        return;
    }

    /*
     * Added stabbing weapon. -Narn 
     */
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
        return;
    }

    if ( obj->value[4] != WEP_1H_SHORT_BLADE ) {
        if ( ( obj->value[4] == WEP_1H_LONG_BLADE && obj->value[3] != DAM_PIERCE )
             || obj->value[4] != WEP_1H_LONG_BLADE ) {
            send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
            return;
        }
    }

    if ( victim->fighting ) {
        send_to_char( "You can't backstab someone who is in combat.\r\n", ch );
        return;
    }

    percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );

    check_attacker( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_backstab]->beats );
    if ( !IS_AWAKE( victim ) || can_use_skill( ch, percent, gsn_backstab ) ) {

        if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
            send_to_char( "!!SOUND(sound/backstab1.wav)\r\n", ch );

        learn_from_success( ch, gsn_backstab );

        if ( can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 )
            global_retcode = damage( ch, victim, insane, gsn_backstab );
        else if ( can_see( victim, ch ) && victim->hit < victim->max_hit / 2 )
            global_retcode = damage( ch, victim, extrahigh, gsn_backstab );
        else if ( !can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 ) {
            send_to_char( "&RYour victim doesn't even see you coming!&w\r\n", ch );
            global_retcode = damage( ch, victim, maximum, gsn_backstab );
        }
        else if ( !can_see( victim, ch ) && victim->hit < victim->max_hit / 2 ) {
            send_to_char( "&RYour victim doesn't even see you coming!&w\r\n", ch );
            global_retcode = damage( ch, victim, extrahigh, gsn_backstab );
        }
        adjust_favor( ch, 10, 1 );
        check_illegal_pk( ch, victim );
    }
    else {
        send_to_char( "&GYou almost cut your damn self!\r\n", ch );
        global_retcode = damage( ch, victim, 0, gsn_backstab );
        check_illegal_pk( ch, victim );
    }
}

void do_body_drop( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Attempt a body drop on who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "Why would you want to hit yourself?\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_body_drop]->beats );
    short                   judochance;

    if ( ch->secondclass == -1 )
        judochance = number_range( 1, 4 );
    else if ( ch->secondclass != -1 && ch->thirdclass == -1 )
        judochance = number_range( 1, 8 );
    else if ( ch->secondclass != -1 && ch->thirdclass != -1 )
        judochance = number_range( 1, 10 );

    if ( IS_NPC( ch ) || can_use_skill( ch, number_percent(  ), gsn_body_drop ) ) {
        if ( victim->race == RACE_DRAGON ) {
            judochance = 2;
        }

        if ( judochance != 3 ) {
            act( AT_CYAN, "You strike $N in the throat!", ch, NULL, victim, TO_CHAR );
            act( AT_CYAN, "$N strikes $n in the throat!", ch, NULL, victim, TO_NOTVICT );
            act( AT_CYAN, "$N strikes you in the throat!", ch, NULL, victim, TO_VICT );
        }
        learn_from_success( ch, gsn_body_drop );

        if ( judochance == 3 ) {
            act( AT_CYAN, "You strike $N in the throat and judo flip $M to the ground!", ch, NULL,
                 victim, TO_CHAR );
            act( AT_CYAN, "$N strikes $n in the throat and judo flips $M to the ground!", ch, NULL,
                 victim, TO_NOTVICT );
            act( AT_CYAN, "$N strikes you in the throat and judo flips you into the ground!", ch,
                 NULL, victim, TO_VICT );
            WAIT_STATE( ch, 1 * ( PULSE_VIOLENCE / 2 ) );
            if ( IS_NPC( victim ) )
                WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
            else
                WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 2 ) );
            if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
                af.type = gsn_judo;
                af.location = APPLY_AC;

                af.modifier = 20;
                af.duration = number_range( 1, 2 );
                af.level = ch->level;
                af.bitvector = meb( AFF_PARALYSIS );
                affect_to_char( victim, &af );
                update_pos( victim );
            }
        }
        if ( IS_NPC( ch ) )
            global_retcode = damage( ch, victim, insane, gsn_body_drop );
        else if ( ch->secondclass == -1 )
            global_retcode = damage( ch, victim, insane, gsn_body_drop );
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 )
            global_retcode = damage( ch, victim, extrahigh, gsn_body_drop );
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 )
            global_retcode = damage( ch, victim, high, gsn_body_drop );
    }
    else {
        act( AT_CYAN, "Your strike misses $N's throat.", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n's strike misses $N completely.", ch, NULL, victim, TO_NOTVICT );
        act( AT_CYAN, "$n's strike misses you completely.", ch, NULL, victim, TO_VICT );
        learn_from_failure( ch, gsn_body_drop );
        global_retcode = damage( ch, victim, 1, gsn_body_drop );
    }
}

void do_heavy_hands( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Launch your heavy hands on who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "Why would you want to hit yourself?\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_heavy_hands]->beats );
    if ( IS_NPC( ch ) || can_use_skill( ch, number_percent(  ), gsn_heavy_hands ) ) {
        act( AT_CYAN, "$N gets rocked by your heavy hands fist attack!", ch, NULL, victim,
             TO_CHAR );
        act( AT_CYAN, "$N gets rocked by $n's heavy hands fist attack!", ch, NULL, victim,
             TO_NOTVICT );
        act( AT_CYAN, "You get rocked by $n's heavy hands fist attack!", ch, NULL, victim,
             TO_VICT );
        learn_from_success( ch, gsn_heavy_hands );
        if ( IS_NPC( ch ) )
            global_retcode = damage( ch, victim, ludicrous, gsn_heavy_hands );
        else if ( ch->secondclass == -1 )
            global_retcode = damage( ch, victim, ludicrous, gsn_heavy_hands );
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 )
            global_retcode = damage( ch, victim, extrahigh, gsn_heavy_hands );
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 )
            global_retcode = damage( ch, victim, high, gsn_heavy_hands );
    }
    else {
        act( AT_CYAN, "Your heavy hands attack misses completely.", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n's heavy hands attack misses $N completely.", ch, NULL, victim,
             TO_NOTVICT );
        act( AT_CYAN, "$n's heavy hands attack misses you completely.", ch, NULL, victim, TO_VICT );
        learn_from_failure( ch, gsn_heavy_hands );
        global_retcode = damage( ch, victim, 1, gsn_heavy_hands );
    }
}

void do_kick( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    int                     chance;
    int                     extradam = 0;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }
    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    chance = number_chance( 1, 10 );
    WAIT_STATE( ch, skill_table[gsn_kick]->beats );

    if ( IS_NPC( ch ) || can_use_skill( ch, number_percent(  ), gsn_kick ) ) {
        if ( chance == 1 ) {
            act( AT_PLAIN, "You snap out a front kick at $N.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n snaps out a front kick at you!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n snaps out a front kick at $N!", ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 2 ) {
            act( AT_PLAIN, "You launch a side kick at $N.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n launches a side kick at you!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n launches a side kick at $N!", ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 3 ) {
            act( AT_PLAIN, "You perform a stomp kick at $N.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n performs a stomp kick on you!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n performs a stomp kick on $N!", ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 4 ) {
            act( AT_PLAIN, "Your rear leg whips up and in to deliver a inner crescent kick at $N.",
                 ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n's rear leg whips up and in delivering a inner crescent kick at you!",
                 ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n's rear leg whips up and in delivering a inner crescent kick to $N!",
                 ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 5 ) {
            act( AT_PLAIN, "Your rear leg whips up and out to deliver a outer crescent kick at $N.",
                 ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN,
                 "$n's rear leg whips up and out to deliver a outer crescent kick at you!", ch,
                 NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n's rear leg whips up and out to deliver a outer crescent kick to $N!",
                 ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 6 ) {
            act( AT_PLAIN, "You leap into the air and snap out a front kick at $N.", ch,
                 NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n leaps into the air and snaps out a front kick at you!", ch,
                 NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n leaps into the air and snaps out a front kick at $N!", ch,
                 NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 7 ) {
            act( AT_PLAIN, "You whip your rear leg up and drop an axe kick on $N's shoulder.", ch,
                 NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n's rear leg whips up and drops an axe kick on your shoulder!",
                 ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n's rear leg whips up and drops an axe kick on $N's shoulder!",
                 ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 8 ) {
            act( AT_PLAIN, "You jump and spin suddenly delivering a spinning crescent kick at $N.",
                 ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN,
                 "$n jumps and spins suddenly delivering a spinning crescent kick at you!", ch,
                 NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n jumps and spins suddenly delivering a spinning crescent kick at $N!",
                 ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance == 9 ) {
            act( AT_PLAIN, "You whip your rear leg over your front sending a butterfly kick at $N.",
                 ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN,
                 "$n's rear leg whips over a front leg and hits you with a butterfly kick!", ch,
                 NULL, victim, TO_VICT );
            act( AT_PLAIN,
                 "$n's rear leg whips over a front leg and hits $N with a butterfly kick!", ch,
                 NULL, victim, TO_NOTVICT );
        }
        else {
            act( AT_PLAIN,
                 "You send out your rear foot to the side of $N, and hook it in connecting with $N's face.",
                 ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n's rear foot misses you, then hooks suddenly in hitting your face!",
                 ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n's rear foot misses $N, then hooks suddenly and hits $N in the face!",
                 ch, NULL, victim, TO_NOTVICT );
        }
        learn_from_success( ch, gsn_kick );
        if ( IS_NPC( ch ) ) {
            global_retcode =
                damage( ch, victim, number_range( ch->level, ch->level * 2 ), gsn_kick );
        }
        else if ( !IS_NPC( ch ) ) {
            global_retcode = damage( ch, victim, nominal, gsn_kick );
        }
        return;
    }

    learn_from_failure( ch, gsn_kick );
    global_retcode = damage( ch, victim, 0, gsn_kick );
}

void do_break( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    bool                    lance;

    lance = 0;

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( obj->value[4] == WEP_LANCE )
            lance = 1;
    }

    if ( lance == 0 ) {
        send_to_char( "&cYou don't have a lance.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_break]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_break ) && ch->pcdata->learned[gsn_charge] > 0
         || IS_NPC( ch ) ) {
        act( AT_CYAN, "You urge your mount to break contact from $N.", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n break's contact from you riding away a short distance.", ch, NULL, victim,
             TO_VICT );
        act( AT_CYAN, "$n break's contact from $N riding a short distance away!", ch, NULL, victim,
             TO_NOTVICT );
        if ( !IS_NPC( ch ) ) {
            learn_from_success( ch, gsn_break );
        }
        if ( victim->fighting ) {
            stop_fighting( ch, TRUE );
            stop_fighting( victim, TRUE );
        }
        if ( IS_NPC( victim ) && victim->hunting ) {
            STRFREE( victim->hunting->name );
            DISPOSE( victim->hunting );
            victim->hunting = NULL;
        }
        if ( IS_NPC( victim ) && victim->hating ) {
            STRFREE( victim->hating->name );
            DISPOSE( victim->hating );
            victim->hating = NULL;
        }
        if ( IS_NPC( ch ) && ch->hunting ) {
            STRFREE( ch->hunting->name );
            DISPOSE( ch->hunting );
            ch->hunting = NULL;
        }
        if ( IS_NPC( ch ) && ch->hating ) {
            STRFREE( ch->hating->name );
            DISPOSE( ch->hating );
            victim->hating = NULL;
        }
        if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL ) {
            obj_from_char( obj );
            obj_to_char( obj, ch );
            interpret( ch, ( char * ) "wear lance" );
        }
        if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
            interpret( ch, ( char * ) "wear lance" );
        }
        do_charge( ch, victim->name );
    }
    else {
        act( AT_CYAN, "You urge your mount to break contact from $N but cannot break away.", ch,
             NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n attempts to ride away from you, but you block the path.", ch, NULL,
             victim, TO_VICT );
        if ( !IS_NPC( ch ) ) {
            learn_from_failure( ch, gsn_break );
        }
    }
    return;
}

void do_punch( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    int                     chance;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    chance = number_chance( 1, 10 );

    WAIT_STATE( ch, skill_table[gsn_punch]->beats );
    if ( IS_NPC( ch ) || can_use_skill( ch, number_percent(  ), gsn_punch ) ) {
        if ( chance <= 2 ) {
            act( AT_PLAIN, "You snap out a left stiff jab at $N.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n snaps out a left stiff jab at you!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n snaps out a left stiff jab at $N!", ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance <= 3 && chance > 5 ) {
            act( AT_PLAIN, "You hook a hard right punch at $N's face.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n hooks a right punch connecting to your face!", ch, NULL, victim,
                 TO_VICT );
            act( AT_PLAIN, "$n hooks a right punch connecting to $N's face!", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else if ( chance <= 6 && chance > 8 ) {
            act( AT_PLAIN, "You pound $N in the belly with a hard right punch.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_PLAIN, "$n nails you in the belly with a hard right punch!", ch, NULL, victim,
                 TO_VICT );
            act( AT_PLAIN, "$n nails $N with a hard right punch to the belly!", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else if ( chance <= 9 && chance > 7 ) {
            act( AT_PLAIN, "You come up with a right uppercut catching $N in the chin.", ch, NULL,
                 victim, TO_CHAR );
            act( AT_PLAIN, "$n comes up with a right uppercut and catches you on the chin!", ch,
                 NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n comes up with a right uppercut and catches $N on the chin!", ch,
                 NULL, victim, TO_NOTVICT );
        }
        else {
            act( AT_PLAIN, "You hook a left punch into $N's ribs.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n hooks a left punch into your ribs!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n hooks a left punch into $N's ribs!", ch, NULL, victim, TO_NOTVICT );
        }

        if ( !IS_NPC( ch ) )
            learn_from_success( ch, gsn_punch );
        if ( IS_NPC( ch ) ) {
            global_retcode =
                damage( ch, victim, number_range( ch->level, ch->level * 2 ), gsn_punch );
        }
        else if ( !IS_NPC( ch ) ) {
            global_retcode = damage( ch, victim, nominal, gsn_punch );
        }
        return;
    }

    learn_from_failure( ch, gsn_punch );
    global_retcode = damage( ch, victim, 0, gsn_punch );
}

void do_bite( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    int                     chance;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    chance = number_range( 1, 100 );

    WAIT_STATE( ch, skill_table[gsn_bite]->beats );
    if ( IS_NPC( ch ) || can_use_skill( ch, number_percent(  ), gsn_bite ) ) {
        if ( ch->race == RACE_DRAGON ) {
            if ( chance <= 20 ) {
                act( AT_PLAIN, "You only nip $N's left arm with your massive jaws.", ch, NULL,
                     victim, TO_CHAR );
                act( AT_PLAIN, "$n nips your left arm with massive jaws leaving a large gash!", ch,
                     NULL, victim, TO_VICT );
                act( AT_PLAIN,
                     "$n nips $N's left arm with massive jaws leaving a large gash on $N!", ch,
                     NULL, victim, TO_NOTVICT );
            }
            else if ( chance <= 30 && chance > 21 ) {
                act( AT_PLAIN, "Your massive jaws bite $N's right leg.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_PLAIN, "$n's massive jaws bite down upon your right leg!", ch, NULL, victim,
                     TO_VICT );
                act( AT_PLAIN, "$n's massive jaws bite down upon $N's right leg!", ch, NULL, victim,
                     TO_NOTVICT );
            }
            else if ( chance <= 40 && chance > 31 ) {
                act( AT_PLAIN, "Your massive jaws bite $N's right shoulder.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_PLAIN, "$n's massive jaws bite down upon your right shoulder!", ch, NULL,
                     victim, TO_VICT );
                act( AT_PLAIN, "$n's massive jaws bite down upon $N's right shoulder!", ch, NULL,
                     victim, TO_NOTVICT );
            }
            else if ( chance <= 50 && chance > 41 ) {
                act( AT_PLAIN, "Your massive jaws bite $N's left leg.", ch, NULL, victim, TO_CHAR );
                act( AT_PLAIN, "$n's massive jaws bite down upon your left leg!", ch, NULL, victim,
                     TO_VICT );
                act( AT_PLAIN, "$n's massive jaws bite down upon $N's left leg!", ch, NULL, victim,
                     TO_NOTVICT );
            }
            else if ( chance <= 70 && chance > 51 ) {
                act( AT_PLAIN, "Your massive jaws bite $N's left shoulder.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_PLAIN, "$n's massive jaws bite down upon your left shoulder!", ch, NULL,
                     victim, TO_VICT );
                act( AT_PLAIN, "$n's massive jaws bite down upon $N's left shoulder!", ch, NULL,
                     victim, TO_NOTVICT );
            }
            else if ( chance <= 90 && chance > 71 ) {
                act( AT_PLAIN, "Your massive jaws bite $N's waist and hoist $M in the air!", ch,
                     NULL, victim, TO_CHAR );
                act( AT_PLAIN, "$n's massive jaws bite your waist and you into the air!", ch, NULL,
                     victim, TO_VICT );
                act( AT_PLAIN, "$n's massive jaws bite $N's waist and hoist $M in the air!", ch,
                     NULL, victim, TO_NOTVICT );
            }
            else {
                act( AT_PLAIN, "Your massive jaws wrap around $N's chest.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_PLAIN, "$n's massive jaws wrap around your chest!", ch, NULL, victim,
                     TO_VICT );
                act( AT_PLAIN, "$n's massive jaws wrap around $N's chest!", ch, NULL, victim,
                     TO_NOTVICT );
            }

            if ( !IS_NPC( ch ) )
                learn_from_success( ch, gsn_bite );
            global_retcode = damage( ch, victim, nominal, gsn_bite );
            return;
        }

        if ( chance <= 10 ) {
            act( AT_PLAIN, "You grab $N's left hand and bite it.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n grabs your left hand and bites it!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n grabs $N's left hand and bites it!", ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance <= 20 && chance > 11 ) {
            act( AT_PLAIN, "Seeing an opening you bite $N's right ear.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_PLAIN, "$n steps in on you and bites your right ear!", ch, NULL, victim,
                 TO_VICT );
            act( AT_PLAIN, "$n steps in close to $N and bites $S right ear!", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else if ( chance <= 30 && chance > 21 ) {
            act( AT_PLAIN, "You grab $N's right hand and bite it.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n grabs your right hand and bites it!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n grabs $N's right hand and bites it!", ch, NULL, victim, TO_NOTVICT );
        }
        else if ( chance <= 40 && chance > 31 ) {
            act( AT_PLAIN, "Seeing an opening you bite $N's left ear.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n steps in on you and bites your left ear!", ch, NULL, victim,
                 TO_VICT );
            act( AT_PLAIN, "$n steps in close to $N and bites $S left ear!", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else if ( chance <= 70 && chance > 41 ) {
            act( AT_PLAIN, "Seeing an opening you bite $N's neck.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n steps in on you and bites your neck!", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$n steps in close to $N and bites $S neck!", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else if ( chance <= 80 && chance > 71 ) {
            act( AT_PLAIN, "Seeing an opening you bite $N's left arm.", ch, NULL, victim, TO_CHAR );
            act( AT_PLAIN, "$n steps in on you and bites your left arm!", ch, NULL, victim,
                 TO_VICT );
            act( AT_PLAIN, "$n steps in close to $N and bites $S left arm!", ch, NULL, victim,
                 TO_NOTVICT );
        }
        else {
            act( AT_PLAIN, "Seeing an opening you bite $N's right arm.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_PLAIN, "$n steps in on you and bites your right arm!", ch, NULL, victim,
                 TO_VICT );
            act( AT_PLAIN, "$n steps in close to $N and bites $S right arm!", ch, NULL, victim,
                 TO_NOTVICT );
        }

        if ( !IS_NPC( ch ) )
            learn_from_success( ch, gsn_bite );
        global_retcode = damage( ch, victim, nominal, gsn_bite );
        return;
    }

    learn_from_failure( ch, gsn_bite );
    global_retcode = damage( ch, victim, 0, gsn_bite );
}

void do_claw( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ch->level < ( skill_table[gsn_claw]->skill_level[ch->Class] ) ) {
        send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_claw]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_claw ) ) {
        learn_from_success( ch, gsn_claw );
        global_retcode = damage( ch, victim, nominal, gsn_claw );
    }
    else {
        learn_from_failure( ch, gsn_claw );
        global_retcode = damage( ch, victim, 0, gsn_claw );
    }
}

void do_sting( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ch->level < skill_table[gsn_sting]->skill_level[ch->Class] ) {
        send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_sting]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_sting ) ) {
        learn_from_success( ch, gsn_sting );
        global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_sting );
    }
    else {
        learn_from_failure( ch, gsn_sting );
        global_retcode = damage( ch, victim, 0, gsn_sting );
    }
}

void do_bash( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    int                     chance;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_bash] <= 0 ) {
        send_to_char( "You're not a skilled enough fighter...\r\n", ch );
        return;
    }
    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    // lets make this occur less often from mobs. -Taon
    if ( IS_NPC( ch ) && number_chance( 0, 10 ) < 8 )
        return;

    chance =
        ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) ) -
            ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
    if ( !IS_NPC( ch ) && !IS_NPC( victim ) )
        chance += sysdata.bash_plr_vs_plr;
    if ( victim->fighting && victim->fighting->who != ch )
        chance += sysdata.bash_nontank;
    WAIT_STATE( ch, skill_table[gsn_bash]->beats );
    if ( can_use_skill( ch, ( number_percent(  ) + chance ), gsn_bash ) ) {
        act( AT_SKILL, "$N bashes into you, leaving you stunned!", victim, NULL, ch, TO_CHAR );
        act( AT_SKILL, "You bash into $N, leaving $M stunned!", ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n bashes into $N, leaving $M stunned!", ch, NULL, victim, TO_NOTVICT );
        learn_from_success( ch, gsn_bash );
        /*
         * do not change anything here!  -Thoric 
         */
        WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
        set_position( victim, POS_SITTING );
        global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_bash );
    }
    else {
        learn_from_failure( ch, gsn_bash );
        global_retcode = damage( ch, victim, 0, gsn_bash );
    }
    return;
}

void do_stun( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    int                     chance;
    bool                    fail;
    short                   luck;

    luck = number_range( 1, 4 );
    if ( luck == 1 ) {
        act( AT_GREEN, "You try to stun but they dodge out of the way.", ch, NULL, victim,
             TO_CHAR );
        return;
    }

    // lets make this occur less often from mobs. -Taon
    if ( IS_NPC( ch ) && number_chance( 0, 10 ) < 8 )
        return;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_stun] <= 0 ) {
        send_to_char( "You better leave the martial arts to fighters.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ch->move < ch->max_move / 10 ) {
        set_char_color( AT_SKILL, ch );
        send_to_char( "You are far too tired to do that.\r\n", ch );
        return;                                        /* missing return fixed March
                                                        * 11/96 */
    }

    WAIT_STATE( ch, skill_table[gsn_stun]->beats );
    fail = FALSE;
    chance = ris_save( victim, ch->level, RIS_PARALYSIS );
    if ( chance == 1000 )
        fail = TRUE;
    else
        fail = saves_para_petri( chance, victim );

    chance =
        ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) ) -
            ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
    /*
     * harder for player to stun another player 
     */

    // Added bonus here to help fight against stun. -Taon
    if ( IS_AFFECTED( victim, AFF_SHIELD ) )
        chance -= 10;
    if ( !IS_NPC( ch ) && !IS_NPC( victim ) )
        chance += sysdata.stun_plr_vs_plr;
    else
        chance += sysdata.stun_regular;
    if ( !fail && can_use_skill( ch, ( number_percent(  ) + chance ), gsn_stun ) ) {
        learn_from_success( ch, gsn_stun );
        if ( !IS_NPC( ch ) )
            ch->move -= 25;
        if ( IS_NPC( victim ) )
            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
        else
            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 3 ) );
        act( AT_GREEN, "$N smashes into you, leaving you stunned!", victim, NULL, ch, TO_CHAR );
        act( AT_GREEN, "You smash into $N, leaving $M stunned!", ch, NULL, victim, TO_CHAR );
        act( AT_GREEN, "$n smashes into $N, leaving $M stunned!", ch, NULL, victim, TO_NOTVICT );
        if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
            af.type = gsn_stun;
            af.location = APPLY_AC;
            af.modifier = 20;
            af.duration = number_range( 1, 2 );
            af.level = ch->level;
            af.bitvector = meb( AFF_PARALYSIS );
            affect_to_char( victim, &af );
            update_pos( victim );
        }
    }
    else {
        WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
        if ( !IS_NPC( ch ) )
            ch->move -= 15;
        act( AT_GREEN, "$n charges at you screaming, but you dodge out of the way.", ch, NULL,
             victim, TO_VICT );
        act( AT_GREEN, "You try to stun $N, but $E dodges out of the way.", ch, NULL, victim,
             TO_CHAR );
        act( AT_GREEN, "$n charges screaming at $N, but keeps going right on past.", ch, NULL,
             victim, TO_NOTVICT );
    }
}

void do_bloodlet( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    AFFECT_DATA             af1;
    CHAR_DATA              *victim = NULL;

    if ( IS_NPC( ch ) )
        return;

    char                    arg[MIL];

    argument = one_argument( argument, arg );

/* We are trying to target another vampire here */
    if ( ch->fighting ) {
        send_to_char( "You're too busy fighting.\r\n", ch );
        return;
    }

    if ( ch->blood < 20 && !IS_IMMORTAL( ch ) ) {
        send_to_char( "You are too drained to let any blood.\r\n", ch );
        return;
    }

    if ( arg[0] != '\0' ) {

        if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }

        if ( ch == victim ) {
            send_to_char( "There would be no good in drinking your own blood.\r\n", ch );
            return;
        }

    }

    WAIT_STATE( ch, PULSE_VIOLENCE );
    if ( can_use_skill( ch, number_percent(  ), gsn_bloodlet ) ) {
        learn_from_success( ch, gsn_bloodlet );
        ch->blood -= 20;
        act( AT_BLOOD, "Tracing a sharp finger nail over your wrist, you let your blood flow.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_BLOOD, "$n traces a sharp finger nail over $s wrist, letting $s blood flow.", ch,
             NULL, NULL, TO_ROOM );

        if ( victim ) {
            act( AT_BLOOD, "You become affixed to $s wrist, lapping at the powerful blood.", ch,
                 NULL, victim, TO_VICT );
            act( AT_BLOOD, "$N becomes affixed to $s wrist, lapping at the powerful blood.", ch,
                 NULL, victim, TO_NOTVICT );
            act( AT_BLOOD, "$N becomes affixed to your wrist, lapping at the powerful blood.", ch,
                 NULL, victim, TO_CHAR );

            if ( victim->hit < 1 ) {
                victim->hit = 1;
                update_pos( victim );
                return;
            }

            victim->blood += 20;
            victim->hit += ch->level;
            af1.type = gsn_bloodlet;
            af1.duration = ch->level + 10;
            af1.location = APPLY_AGE;
            af1.modifier = 0;
            af1.bitvector = meb( AFF_AGE );
            af1.level = ch->level;
            affect_to_char( victim, &af1 );

            if ( victim->blood > victim->max_blood )
                victim->blood = victim->max_blood;

            if ( victim->hit > victim->max_hit )
                victim->hit = victim->max_hit;

            send_to_char( "&rYou feel sweet bliss as the blood rushes to regenerate your body.\r\n",
                          victim );
        }
        else {
            act( AT_BLOOD, "The blood pools on the floor.", ch, NULL, NULL, TO_CHAR );
            act( AT_BLOOD, "The blood pools on the floor.", ch, NULL, NULL, TO_ROOM );
            obj = create_object( get_obj_index( OBJ_VNUM_BLOODLET ), 0 );
            obj->timer = 1;
            obj->value[1] = 6;
            obj_to_room( obj, ch->in_room );
        }
    }
    else {
        act( AT_BLOOD, "You cannot manage to draw much blood...", ch, NULL, NULL, TO_CHAR );
        act( AT_BLOOD, "$n slices open $s skin, but no blood is spilled...", ch, NULL, NULL,
             TO_ROOM );
    }
}

void do_cannibalize( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }
    if ( !can_use_skill( ch, 0, gsn_cannibalize ) ) {
        send_to_char( "You have not yet old enough to cannibalize your blood.\r\n", ch );
        return;
    }
    if ( ch->blood < 11 ) {
        send_to_char( "You do not have enough blood in you to do that and must feed first.\r\n",
                      ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_cannibalize ) ) {
        learn_from_success( ch, gsn_cannibalize );
        act( AT_RED,
             "You draw on the power of your ancient blood to heal your wounds.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n's wounds begin to regenerate.", ch, NULL, NULL, TO_NOTVICT );
        if ( ch->blood >= 11 ) {
            ch->blood -= 10;
            ch->hit += ch->level / 2 + 20;
        }
        if ( ch->hit >= ch->max_hit ) {
            ch->hit = ch->max_hit;
        }
        return;
    }
    else
        learn_from_failure( ch, gsn_cannibalize );
    send_to_char( "You try to to cannibalize your blood but fail to concentrate enough.\r\n", ch );
    return;
}

void do_feed( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }
    if ( !can_use_skill( ch, 0, gsn_feed ) ) {
        send_to_char( "You have not yet practiced your new teeth.\r\n", ch );
        return;
    }

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Feed from who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "Why would you want to feed on yourself?\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_feed]->beats );
    if ( !can_use_skill( ch, number_percent(  ), gsn_feed ) ) {
        global_retcode = damage( ch, victim, 0, gsn_feed );
        if ( global_retcode == rNONE && !IS_NPC( ch ) && ch->fighting && ch->blood < ch->max_blood ) {
            act( AT_BLOOD, "The smell of $N's blood is driving you insane!", ch, NULL, victim,
                 TO_CHAR );
            act( AT_BLOOD, "$n is lusting after your blood!", ch, NULL, victim, TO_VICT );
            learn_from_failure( ch, gsn_feed );
        }
        return;
    }

    if ( ch->Class == CLASS_VAMPIRE ) {
        if ( ch->level < 20 )
            global_retcode = damage( ch, victim, low, gsn_feed );
        else if ( ch->level >= 20 && ch->level < 40 )
            global_retcode = damage( ch, victim, mediumhigh, gsn_feed );
        else if ( ch->level >= 40 && ch->level < 60 )
            global_retcode = damage( ch, victim, high, gsn_feed );
        else if ( ch->level >= 60 )
            global_retcode = damage( ch, victim, extrahigh, gsn_feed );
    }
    else if ( ch->Class != CLASS_VAMPIRE ) {
        global_retcode = damage( ch, victim, nominal, gsn_feed );
    }
    if ( global_retcode == rNONE && !IS_NPC( ch ) && ch->fighting && ch->blood < ch->max_blood ) {
        short                   temp;                  // check to make sure some blood
                                                       // is given
        temp = ch->blood;
        ch->blood = ch->blood + number_range( 1, ( ch->level + victim->level / 20 ) + 3 );

        if ( ch->blood == temp ) {
            ch->blood = ch->blood + number_range( 1, 4 );
        }

        if ( ch->blood > ch->max_blood ) {
            act( AT_RED,
                 "You sucked more blood then you need, the rest just sprays out in the air!", ch,
                 NULL, victim, TO_CHAR );
            ch->blood = ch->max_blood;
        }

        if ( ch->blood <= 37 ) {
            if ( ch->pcdata->condition[COND_FULL] <= 40
                 || ch->pcdata->condition[COND_THIRST] <= 40 ) {
                gain_condition( ch, COND_FULL, 2 );
                gain_condition( ch, COND_THIRST, 2 );
            }
        }

        if ( IS_NPC( ch ) )
            act( AT_PLAIN, "$n bites you!", ch, NULL, victim, TO_VICT );
        else {
            act( AT_PLAIN, "You step in close to $N and bite off a chunk of $N's flesh.", ch, NULL,
                 victim, TO_CHAR );
            act( AT_BLOOD, "You manage to suck a little blood out of $N.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_PLAIN, "$n steps in close to you and bites off a chunk of your flesh!", ch,
                 NULL, victim, TO_VICT );
            act( AT_BLOOD, "$n sucks some of your blood!", ch, NULL, victim, TO_VICT );
            learn_from_success( ch, gsn_feed );
        }
    }
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 * Check for loyalty flag (weapon disarms to inventory) for pkillers -Blodkai
 */
void disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA               *obj,
                           *tmpobj;

    // Lets make this occur less often from mobs. -Taon
    if ( IS_NPC( ch ) && number_chance( 0, 10 ) < 8 )
        return;

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
        return;

    if ( ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL && number_bits( 1 ) == 0 )
        obj = tmpobj;

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL && number_bits( 1 ) == 0 ) {
        learn_from_failure( ch, gsn_disarm );
        return;
    }

    if ( IS_NPC( ch ) && !can_see_obj( ch, obj ) && number_bits( 1 ) == 0 ) {
        learn_from_failure( ch, gsn_disarm );
        return;
    }

    if ( check_grip( ch, victim ) ) {
        learn_from_failure( ch, gsn_disarm );
        return;
    }

    if ( xIS_SET( victim->act, PLR_SOUND ) || xIS_SET( victim->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/disarm.wav)\r\n", victim );

    act( AT_GREEN, "&G$n DISARMS you!", ch, NULL, victim, TO_VICT );
    act( AT_GREEN, "&GYou disarm $N!", ch, NULL, victim, TO_CHAR );
    act( AT_GREEN, "&G$n disarms $N!", ch, NULL, victim, TO_NOTVICT );

    ch_printf( victim, "&G%s flies across the room.\r\n", obj->short_descr );

    learn_from_success( ch, gsn_disarm );

    if ( obj == get_eq_char( victim, WEAR_WIELD )
         && ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL )
        tmpobj->wear_loc = WEAR_WIELD;

    obj_from_char( obj );
    if ( IS_NPC( victim ) || ( IS_OBJ_STAT( obj, ITEM_LOYAL ) && !IS_NPC( ch ) ) )
        obj_to_char( obj, victim );
    else
        obj_to_room( obj, victim->in_room );
}

void do_disarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_disarm] <= 0 ) {
        send_to_char( "You don't know how to disarm opponents.\r\n", ch );
        return;
    }

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL ) {
        send_to_char( "You must wield a weapon to disarm.\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "Your opponent is not wielding a weapon.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_disarm]->beats );
    percent =
        number_percent(  ) + victim->level - ch->level - ( get_curr_lck( ch ) - 15 ) +
        ( get_curr_lck( victim ) - 15 );
    if ( !can_see_obj( ch, obj ) )
        percent += 10;
    if ( can_use_skill( ch, ( percent * 3 / 2 ), gsn_disarm ) )
        disarm( ch, victim );
    else {
        send_to_char( "You failed to disarm them.\r\n", ch );
        learn_from_failure( ch, gsn_disarm );
    }
}

/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( IS_AFFECTED( victim, AFF_FLYING ) || IS_AFFECTED( victim, AFF_FLOATING ) )
        return;
    if ( number_chance( 1, 10 ) > 5 )
        return;

    if ( victim->mount ) {
        if ( IS_AFFECTED( victim->mount, AFF_FLYING )
             || IS_AFFECTED( victim->mount, AFF_FLOATING ) )
            return;
        act( AT_SKILL, "$n trips your mount and you fall off!", ch, NULL, victim, TO_VICT );
        act( AT_SKILL, "You trip $N's mount and $N falls off!", ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n trips $N's mount and $N falls off!", ch, NULL, victim, TO_NOTVICT );
        xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
        victim->mount = NULL;
        WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
        WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
        set_position( victim, POS_RESTING );
        return;
    }
    if ( victim->wait == 0 ) {
        act( AT_SKILL, "$n trips you and you go down!", ch, NULL, victim, TO_VICT );
        act( AT_SKILL, "You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n trips $N and $N goes down!", ch, NULL, victim, TO_NOTVICT );

        WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
        WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
        set_position( victim, POS_RESTING );
    }
}

/* Converted to function well as a skill for vampires -- Blodkai */
void do_mistwalk( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    bool                    allowday;

    set_char_color( AT_DGREEN, ch );
    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }
    if ( ch->mount ) {
        send_to_char( "And scare your mount to death?\r\n", ch );
        return;
    }
    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Who will be your victim?\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_mistwalk]->beats );
    if ( ( victim = get_char_world( ch, arg ) ) == NULL || victim == ch ) {
        send_to_char( "&rYou are unable to sense your victim.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "&rWhy would you travel to yourself?\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&rYou sense you cannot mist walk at your current location.\r\n", ch );
        return;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&rIt is beyond your ability to mist walk to that location.\r\n", ch );
        return;
    }

    if ( !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY ) ) {
        send_to_char( "&rIt is beyond your ability to mist walk to that location.\r\n", ch );
        return;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "&rYou cannot mist walk to a Staff member.\r\n", ch );
        return;
    }

    if ( !in_hard_range( ch, victim->in_room->area ) ) {
        send_to_char( "&rYou cannot mist walk to the area that your victim is in.\r\n", ch );
        return;
    }

    if ( time_info.hour < 21 && time_info.hour > 5 && !IS_NPC( ch ) && ch->blood < 22 ) {
        send_to_char( "You do not have enough blood.\r\n", ch );
        learn_from_failure( ch, gsn_mistwalk );
    }

    /*
     * Subtract 22 extra bp for mist walk from 0500 to 2100 SB 
     */
    if ( time_info.hour < 21 && time_info.hour > 5 && !IS_NPC( ch ) )
        ch->blood = ch->blood - 22;
    act( AT_DGREEN, "&rYour surroundings blur as you assume a form of churning mist!", ch,
         NULL, NULL, TO_CHAR );
    act( AT_DGREEN, "&r$n dissolves into a cloud of glowing mist, then vanishes!", ch, NULL,
         NULL, TO_ROOM );
    learn_from_success( ch, gsn_mistwalk );
    char_from_room( ch );
    char_to_room( ch, victim->in_room );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }

    act( AT_DGREEN, "&rA cloud of glowing mist engulfs you, then withdraws to unveil $n!", ch, NULL,
         NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
}

void do_broach( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    EXIT_DATA              *pexit;

    set_char_color( AT_DGREEN, ch );

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }
    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Attempt this in which direction?\r\n", ch );
        return;
    }
    if ( ch->mount ) {
        send_to_char( "You should really dismount first.\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_broach]->beats );
    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        EXIT_DATA              *pexit_rev;

        if ( !IS_SET( pexit->exit_info, EX_CLOSED )
             || !IS_SET( pexit->exit_info, EX_LOCKED )
             || IS_SET( pexit->exit_info, EX_PICKPROOF )
             || can_use_skill( ch, number_percent(  ), gsn_broach ) ) {
            send_to_char( "Your attempt fails.\r\n", ch );
            learn_from_failure( ch, gsn_broach );
            check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
            return;
        }
        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
        send_to_char( "You successfully broach the exit...\r\n", ch );
        learn_from_success( ch, gsn_broach );
        adjust_favor( ch, 9, 1 );
        if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
        check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
        return;
    }
    send_to_char( "Your attempt fails.\r\n", ch );
}

void do_pick( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *gch;
    OBJ_DATA               *obj;
    EXIT_DATA              *pexit;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Pick what?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_pick_lock]->beats );

    /*
     * look for guards 
     */
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room ) {
        if ( IS_NPC( gch ) && IS_AWAKE( gch ) && ch->level + 5 < gch->level ) {
            act( AT_PLAIN, "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
            return;
        }
    }

    if ( !can_use_skill( ch, number_percent(  ), gsn_pick_lock ) ) {
        send_to_char( "You failed.\r\n", ch );
        learn_from_failure( ch, gsn_pick_lock );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        EXIT_DATA              *pexit_rev;

        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( pexit->key < 0 ) {
            send_to_char( "It can't be picked.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            send_to_char( "It's already unlocked.\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_PICKPROOF ) ) {
            send_to_char( "It can't be picked.\r\n", ch );
            learn_from_failure( ch, gsn_pick_lock );
            check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
            return;
        }

        REMOVE_BIT( pexit->exit_info, EX_LOCKED );
        send_to_char( "*Click*\r\n", ch );
        act( AT_ACTION, "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        learn_from_success( ch, gsn_pick_lock );
        adjust_favor( ch, 9, 1 );
        /*
         * pick the other side 
         */
        if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
        check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'pick object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            send_to_char( "That's not a container.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            send_to_char( "It's not closed.\r\n", ch );
            return;
        }
        if ( obj->value[2] < 0 ) {
            send_to_char( "It can't be unlocked.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_LOCKED ) ) {
            send_to_char( "It's already unlocked.\r\n", ch );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_PICKPROOF ) ) {
            send_to_char( "It cannot be picked.\r\n", ch );
            learn_from_failure( ch, gsn_pick_lock );
            check_for_trap( ch, obj, TRAP_PICK );
            return;
        }

        separate_obj( obj );
        REMOVE_BIT( obj->value[1], CONT_LOCKED );
        send_to_char( "*Click*\r\n", ch );
        act( AT_ACTION, "$n picks $p.", ch, obj, NULL, TO_ROOM );
        learn_from_success( ch, gsn_pick_lock );
        check_for_trap( ch, obj, TRAP_PICK );
        return;
    }

    ch_printf( ch, "You see no %s here.\r\n", arg );
}

void do_sneak( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }
    // Lets stop the stacking of sneak. -Taon
    if ( IS_AFFECTED( ch, AFF_SNEAK ) ) {
        send_to_char( "But you're already sneaking...\r\n", ch );
        return;
    }
    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    affect_strip( ch, gsn_sneak );

    if ( can_use_skill( ch, number_percent(  ), gsn_sneak ) ) {
        af.type = gsn_sneak;
        af.duration = ch->level + 10;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_SNEAK );
        af.level = ch->level;
        affect_to_char( ch, &af );
        learn_from_success( ch, gsn_sneak );
        act( AT_PLAIN, "You carefully plant your foot falls to move as silently as possible.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_PLAIN, "$n slows $s pace and begins to walk silently.", ch, NULL, NULL,
             TO_NOTVICT );
        return;
    }
    else
        learn_from_failure( ch, gsn_sneak );
    send_to_char( "You try to move silently, but something crunches under your foot.\r\n", ch );
}

void do_phase( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_PHASE ) ) {
        act( AT_YELLOW, "You slowly stop phasing, and return to your original state.", ch, NULL,
             NULL, TO_CHAR );
        act( AT_YELLOW, "$n's body stops phasing in and out.", ch, NULL, NULL, TO_NOTVICT );
        affect_strip( ch, gsn_phase );
        xREMOVE_BIT( ch->affected_by, AFF_PHASE );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_phase ) ) {
        af.type = gsn_phase;
        af.duration = ch->level * 10;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_PHASE );
        af.level = ch->level;
        affect_to_char( ch, &af );
        learn_from_success( ch, gsn_phase );
        act( AT_YELLOW,
             "You focus your energies, and begin to phase in and out of the material plane.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "$n's body begins to phase in and out of the material plane.", ch, NULL,
             NULL, TO_NOTVICT );
        return;
    }
    else
        learn_from_failure( ch, gsn_phase );
    send_to_char( "You try to phase, but lose your concentration.\r\n", ch );
}

void do_hide( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_HIDE ) ) {
        send_to_char( "You are already hiding.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_hide ) ) {
        af.type = gsn_hide;
        af.duration = ch->level + 10;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_HIDE );
        af.level = ch->level;
        affect_to_char( ch, &af );
        learn_from_success( ch, gsn_hide );
        act( AT_PLAIN, "You quickly find some concealment to hide behind.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_PLAIN, "$n's silhouette melds into the background.", ch, NULL, NULL, TO_NOTVICT );
        return;
    }
    else
        learn_from_failure( ch, gsn_hide );
    send_to_char( "You try to hide, but fail to find proper concealment.\r\n", ch );
}

void do_demonic_sight( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    affect_strip( ch, gsn_demonic_sight );

    if ( can_use_skill( ch, number_percent(  ), gsn_demonic_sight ) ) {
        af.type = gsn_demonic_sight;
        af.duration = ch->level + 10;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_DEMONIC_SIGHT );
        af.level = ch->level;
        affect_to_char( ch, &af );
        send_to_char( "Your eyes glow as you focus your demonic sight.\r\n", ch );
        learn_from_success( ch, gsn_demonic_sight );
        return;
    }
    else
        learn_from_failure( ch, gsn_demonic_sight );
    send_to_char( "Your vision blurs, as you fail to use your demonic sight.\r\n", ch );
}

/* Contributed by Alander. */
void do_visible( CHAR_DATA *ch, char *argument )
{
    affect_strip( ch, gsn_smuggle );
    affect_strip( ch, gsn_invis );
    affect_strip( ch, gsn_mass_invis );
    affect_strip( ch, gsn_sneak );
    affect_strip( ch, gsn_hide );
    affect_strip( ch, gsn_chameleon );
    xREMOVE_BIT( ch->affected_by, AFF_HIDE );
    xREMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
    xREMOVE_BIT( ch->affected_by, AFF_SNEAK );
    send_to_char( "You slowly become visible.\r\n", ch );
}

void do_recall( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *location;
    CHAR_DATA              *opponent;

    location = NULL;

    if ( IS_NPC( ch ) )
        return;

    if ( ch->level < 2 ) {
        send_to_char( "You must complete the tutorial to use this command.\r\n", ch );
        return;
    }

    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "Your adrenaline is pumping too hard right now!\r\n", ch );
        return;
    }

    if ( ch->position == POS_STUNNED || ch->hit <= 0 ) {
        send_to_char( "You cannot recall while stunned!\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/recall.wav)\r\n", ch );

    if ( IS_IMMORTAL( ch ) )
        location = get_room_index( ROOM_VNUM_CHAT );

    if ( !location && ch->pcdata->tmproom > 100 )
        location = get_room_index( ch->pcdata->tmproom );

    if ( ch->pcdata->lair )
        location = get_room_index( ch->pcdata->lair );

    if ( !location && ch->pcdata->htown )
        location = get_room_index( ch->pcdata->htown->recall );

    if ( !location )
        location = get_room_index( race_table[ch->race]->race_recall );

    if ( !location )
        location = get_room_index( ROOM_VNUM_TEMPLE );

    if ( !location ) {
        send_to_char( "You have no where to recall to!\r\n", ch );
        return;
    }

    if ( ch->in_room == location )
        return;

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) ) {
        send_to_char( "For some strange reason... nothing happens.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_CURSE ) ) {
        send_to_char( "You are cursed and cannot recall!\r\n", ch );
        return;
    }

    if ( ( opponent = who_fighting( ch ) ) != NULL ) {
        int                     lose;

        if ( number_bits( 1 ) == 0 || ( !IS_NPC( opponent ) && number_bits( 3 ) > 1 ) ) {
            WAIT_STATE( ch, 4 );
            lose =
                ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );

            if ( ch->desc )
                lose /= 2;
            gain_exp( ch, 0 - lose );
            send_to_char( "You failed! You lose experience.\r\n", ch );
            return;
        }
        lose = ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );

        if ( ch->desc )
            lose /= 2;
        gain_exp( ch, 0 - lose );
        send_to_char( "You recall from combat! You lose experience.\r", ch );
        stop_fighting( ch, TRUE );
    }
    act( AT_ACTION, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    if ( ch->on ) {
        ch->on = NULL;
        set_position( ch, POS_STANDING );
    }
    if ( ch->position != POS_STANDING ) {
        set_position( ch, POS_STANDING );
    }
    if ( ch->mount ) {
        char_from_room( ch->mount );
        char_to_room( ch->mount, location );
    }
    act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
}

// Clan recall wrote by: Taon for use on 6dragons mud.
void do_clanrecall( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA              *clan;

    if ( IS_NPC( ch ) )
        return;

    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "Your adrenaline is pumping too hard right now!\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "You cant do a clanrecall while in combat.\r\n", ch );
        return;
    }
    if ( ch->position != POS_STANDING && ch->position != POS_MOUNTED ) {
        send_to_char( "You must be standing or mounted in order to recall.\r\n", ch );
        return;
    }
    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) ) {
        send_to_char( "For some strange reason... nothing happens.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/recall.wav)\r\n", ch );

    if ( ch->pcdata->clan ) {
        clan = ch->pcdata->clan;
        char_from_room( ch );
        char_to_room( ch, get_room_index( clan->recall ) );
        if ( ch->on ) {
            ch->on = NULL;
            set_position( ch, POS_STANDING );
        }
        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, get_room_index( clan->recall ) );
        }
        else if ( ch->position != POS_STANDING )
            set_position( ch, POS_STANDING );
        send_to_char( "You recall to the safety of your clan hall.\r\n", ch );
        act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
        do_look( ch, ( char * ) "auto" );
    }
    else
        send_to_char( "But you're not in a clan.\r\n", ch );
}

void do_cityrecall( CHAR_DATA *ch, char *argument )
{
    CITY_DATA              *city;

    if ( IS_NPC( ch ) )
        return;

    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "Your adrenaline is pumping too hard right now!\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "You cant do a clanrecall while in combat.\r\n", ch );
        return;
    }
    if ( ch->position != POS_STANDING && ch->position != POS_MOUNTED ) {
        send_to_char( "You must be standing or mounted in order to recall.\r\n", ch );
        return;
    }
    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) ) {
        send_to_char( "For some strange reason... nothing happens.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/recall.wav)\r\n", ch );

    if ( ch->pcdata->city ) {
        city = ch->pcdata->city;
        char_from_room( ch );
        char_to_room( ch, get_room_index( city->recall ) );
        if ( ch->on ) {
            ch->on = NULL;
            set_position( ch, POS_STANDING );
        }
        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, get_room_index( city->recall ) );
        }
        else if ( ch->position != POS_STANDING )
            set_position( ch, POS_STANDING );
        send_to_char( "You recall to the safety within your city walls.\r\n", ch );
        act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
        do_look( ch, ( char * ) "auto" );
    }
    else
        send_to_char( "But you're not a citizen.\r\n", ch );
}

void do_newbierecall( CHAR_DATA *ch, char *argument )
{
    if ( !ch )
        return;

    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "Your adrenaline is pumping too hard right now!\r\n", ch );
        return;
    }

    if ( ch->level < 2 || ch->level > 19 ) {
        error( ch );
        return;
    }
    do_recall( ch, ( char * ) "" );
}

void do_aid( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Aid whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {                          /* Gorog */
        send_to_char( "Not on mobs.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Aid yourself?\r\n", ch );
        return;
    }

    if ( victim->position > POS_STUNNED ) {
        act( AT_PLAIN, "$N doesn't need your help.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( victim->hit <= -6 ) {
        act( AT_PLAIN, "$N's condition is beyond your aiding ability.", ch, NULL, victim, TO_CHAR );
        return;
    }

    percent = number_percent(  ) - ( get_curr_lck( ch ) - 13 );
    WAIT_STATE( ch, skill_table[gsn_aid]->beats );
    if ( !can_use_skill( ch, percent, gsn_aid ) ) {
        send_to_char( "You fail.\r\n", ch );
        learn_from_failure( ch, gsn_aid );
        return;
    }

    act( AT_SKILL, "You aid $N!", ch, NULL, victim, TO_CHAR );
    act( AT_SKILL, "$n aids $N!", ch, NULL, victim, TO_NOTVICT );
    adjust_favor( ch, 8, 1 );
    learn_from_success( ch, gsn_aid );
    if ( victim->hit < 1 )
        victim->hit = 1;

    update_pos( victim );
    act( AT_SKILL, "$n aids you!", ch, NULL, victim, TO_VICT );
}

void do_mount( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;

    if ( !IS_NPC( ch ) && ch->level < skill_table[gsn_mount]->skill_level[ch->Class] ) {
        send_to_char( "I don't think that would be a good idea...\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You're already mounted!\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, argument ) ) == NULL ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) || !xIS_SET( victim->act, ACT_MOUNTABLE ) ) {
        send_to_char( "You can't mount that!\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_MOUNTED ) ) {
        send_to_char( "That mount already has a rider.\r\n", ch );
        return;
    }

    if ( victim->position < POS_STANDING ) {
        send_to_char( "Your mount must be standing.\r\n", ch );
        return;
    }

    if ( victim->position == POS_FIGHTING || victim->fighting ) {
        send_to_char( "Your mount is moving around too much.\r\n", ch );
        return;
    }

    if ( ( IS_AFFECTED( victim, AFF_FLYING ) || IS_AFFECTED( victim, AFF_FLOATING ) ) ) {
        if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
            if ( !IS_AFFECTED( ch, AFF_FLOATING ) && !IS_AFFECTED( victim, AFF_FLYING ) ) {
                send_to_char( "You can't reach high enough! You need to be floating!\r\n", ch );
                return;
            }
            send_to_char( "You try but the mount flies out of your reach!\r\n", ch );
            return;
        }
    }
    else if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "That mount is on the ground, you must 'land' first.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        xSET_BIT( victim->act, ACT_MOUNTED );
        ch->mount = victim;
        act( AT_DGREEN, "You mount $N.", ch, NULL, victim, TO_CHAR );
        act( AT_DGREEN, "$n skillfully mounts $N.", ch, NULL, victim, TO_NOTVICT );
        act( AT_DGREEN, "$n mounts you.", ch, NULL, victim, TO_VICT );
        set_position( ch, POS_MOUNTED );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_mount]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_mount ) ) {
        xSET_BIT( victim->act, ACT_MOUNTED );
        ch->mount = victim;
        act( AT_SKILL, "You mount $N.", ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n skillfully mounts $N.", ch, NULL, victim, TO_NOTVICT );
        act( AT_SKILL, "$n mounts you.", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_mount );
        set_position( ch, POS_MOUNTED );
        xSET_BIT( ch->affected_by, AFF_MOUNTED );
    }
    else {
        act( AT_SKILL, "You unsuccessfully try to mount $N.", ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n unsuccessfully attempts to mount $N.", ch, NULL, victim, TO_NOTVICT );
        act( AT_SKILL, "$n tries to mount you.", ch, NULL, victim, TO_VICT );
        learn_from_failure( ch, gsn_mount );
    }
}

void do_dismount( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( ( victim = ch->mount ) == NULL ) {
        send_to_char( "You're not mounted.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_mount]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_mount ) ) {
        if ( victim->race == RACE_DRAGON ) {
            act( AT_WHITE, "You carefully slide down $N's wing to the ground.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_WHITE, "$n carefully slides down $N's wings to the ground.", ch, NULL, victim,
                 TO_NOTVICT );
            act( AT_WHITE, "$n slides down your wing to the ground.", ch, NULL, victim, TO_VICT );
            xREMOVE_BIT( victim->act, ACT_MOUNTED );
            ch->mount = NULL;
            set_position( ch, POS_STANDING );
            learn_from_success( ch, gsn_mount );
        }
        else
            act( AT_DGREEN, "You dismount $N.", ch, NULL, victim, TO_CHAR );
        act( AT_DGREEN, "$n skillfully dismounts $N.", ch, NULL, victim, TO_NOTVICT );
        act( AT_DGREEN, "$n dismounts you.  Whew!", ch, NULL, victim, TO_VICT );
        xREMOVE_BIT( victim->act, ACT_MOUNTED );
        xREMOVE_BIT( ch->affected_by, AFF_MOUNTED );
        ch->mount = NULL;
        set_position( ch, POS_STANDING );
        learn_from_success( ch, gsn_mount );
    }
    else {
        if ( victim->race == RACE_DRAGON ) {
            act( AT_WHITE, "You fall off while climbing down $N's wing.  Ouch!", ch, NULL, victim,
                 TO_CHAR );
            act( AT_WHITE, "$n falls off of $N while climbing down.", ch, NULL, victim,
                 TO_NOTVICT );
            act( AT_WHITE, "$n falls off your wing.", ch, NULL, victim, TO_VICT );
            learn_from_failure( ch, gsn_mount );
            xREMOVE_BIT( victim->act, ACT_MOUNTED );
            ch->mount = NULL;
            set_position( ch, POS_SITTING );
            global_retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
        }
        else
            act( AT_RED, "You fall off while dismounting $N.  Ouch!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n falls off of $N while dismounting.", ch, NULL, victim, TO_NOTVICT );
        act( AT_RED, "$n falls off your back.", ch, NULL, victim, TO_VICT );
        learn_from_failure( ch, gsn_mount );
        xREMOVE_BIT( victim->act, ACT_MOUNTED );
        ch->mount = NULL;
        set_position( ch, POS_SITTING );
        global_retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
    }
}

/**************************************************************************/

/* Check for parry. */
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     chances,
                            spice;

    if ( !IS_AWAKE( victim ) )
        return FALSE;

    if ( IS_NPC( victim )
         && ( !xIS_SET( victim->defenses, DFND_PARRY )
              || get_eq_char( victim, WEAR_WIELD ) == NULL ) )
        return FALSE;

    if ( can_use_skill( victim, number_percent(  ), gsn_parry ) ) {
        if ( IS_NPC( victim ) ) {
            /*
             * Tuan was here.  :) 
             */
            chances = UMIN( 35, 2 * victim->level );
        }
        else {
            if ( get_eq_char( victim, WEAR_WIELD ) == NULL )
                return FALSE;
            chances = ( int ) ( LEARNED( victim, gsn_parry ) / sysdata.parry_mod );
        }

        /*
         * Put in the call to chance() to allow penalties for misaligned
         * clannies.  
         */
        if ( chances != 0 && victim->morph )
            chances += victim->morph->parry;

        if ( !chance( victim, chances + victim->level - ch->level ) ) {
            learn_from_failure( victim, gsn_parry );
            return FALSE;
        }

        spice = number_range( 1, 100 );

        if ( spice > 50 ) {
            if ( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
                act( AT_GREY,
                     "Seeing a movement of $n's shoulders, you bring up your weapon parrying $s attack.",
                     ch, NULL, victim, TO_VICT );

            if ( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                act( AT_GREEN, "$N's weapon shoots upward suddenly parrying your attack.", ch, NULL,
                     victim, TO_CHAR );
        }
        else {
            if ( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
                /*SB*/ act( AT_GREY, "As you watch $n you notice $s eyes glance down.", ch, NULL,
                            victim, TO_VICT );
            act( AT_GREY, "You sweep your weapon downward parrying $s attack.", ch, NULL, victim,
                 TO_VICT );

            if ( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )    /* SB */
                act( AT_GREEN, "$N's weapon sweeps downward parrying your attack.", ch, NULL,
                     victim, TO_CHAR );
        }
        learn_from_success( victim, gsn_parry );
        return TRUE;
    }
    else
        return FALSE;
}

/*
 * Check for dodge.
 * Worked a counterstrike and blade master skills into this function. -Taon
 */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     chances,
                            spice;
    short                   cs_chance,
                            bm_chance;

    if ( !IS_AWAKE( victim ) )
        return FALSE;

    if ( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_DODGE )
         && !xIS_SET( victim->defenses, DFND_COUNTERSTRIKE )
         && !xIS_SET( victim->defenses, DFND_BLADEMASTER ) )
        return FALSE;

    if ( can_use_skill( victim, number_percent(  ), gsn_dodge ) ) {
        if ( ( ( IS_NPC( victim ) && xIS_SET( victim->defenses, DFND_BLADEMASTER ) )
               || ( !IS_NPC( victim ) && victim->pcdata->learned[gsn_blade_master] > 0 ) )
             && ( !IS_AFFECTED( victim, AFF_BLINDNESS ) || IS_AFFECTED( victim, AFF_NOSIGHT ) ) ) {
            bm_chance = number_chance( 1, 15 );

            if ( bm_chance > 13 ) {
                act( AT_DGREEN, "$N dodges your attack and rapidly strikes back.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_CYAN, "You dodge $n's attack and rapidly strike back.", ch, NULL, victim,
                     TO_VICT );
                global_retcode = damage( victim, ch, ludicrous, gsn_blade_master );
                learn_from_success( victim, gsn_blade_master );
                return TRUE;
            }
        }

        if ( ( ( IS_NPC( victim ) && xIS_SET( victim->defenses, DFND_COUNTERSTRIKE ) )
               || ( !IS_NPC( victim ) && victim->pcdata->learned[gsn_counterstrike] > 0
                    && victim->focus_level > 0 ) )
             && ( !IS_AFFECTED( victim, AFF_BLINDNESS ) || IS_AFFECTED( victim, AFF_NOSIGHT ) ) ) {
            cs_chance = number_chance( 2, 10 );

            if ( cs_chance >= 8 ) {
                act( AT_DGREEN, "$N dodges your attack and counterstrikes.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_CYAN, "You dodge $n's attack and counterstrike.", ch, NULL, victim,
                     TO_VICT );
                global_retcode = damage( victim, ch, mediumhigh, gsn_counterstrike );
                learn_from_success( victim, gsn_counterstrike );
                return TRUE;
            }
        }

        if ( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_DODGE ) )
            return FALSE;

        if ( IS_NPC( victim ) )
            chances = UMIN( 35, 2 * victim->level );
        else
            chances = ( int ) ( LEARNED( victim, gsn_dodge ) / sysdata.dodge_mod );

        if ( chances != 0 && victim->morph != NULL )
            chances += victim->morph->dodge;

        /*
         * Consider luck as a factor 
         */
        if ( !chance( victim, chances + victim->level - ch->level ) ) {

            learn_from_failure( victim, gsn_dodge );
            return FALSE;
        }

        spice = number_range( 1, 100 );

        if ( spice > 50 ) {
            if ( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
                act( AT_GREY, "You watch $n's shoulders and catch a sudden movement.", ch, NULL,
                     victim, TO_VICT );
            act( AT_CYAN, "You duck down narrowly dodging $s attack.", ch, NULL, victim, TO_VICT );

            if ( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                act( AT_GREY, "$N ducks suddenly dodging your attack.", ch, NULL, victim, TO_CHAR );
        }
        else {
            if ( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
                act( AT_GREY,
                     "Seeing a sudden movement with $n's foot, you sidestep narrowly dodging $s attack.",
                     ch, NULL, victim, TO_VICT );

            if ( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                act( AT_GREY, "$N sidesteps, dodging your attack.", ch, NULL, victim, TO_CHAR );
        }
        learn_from_success( victim, gsn_dodge );
        return TRUE;
    }
    else
        return FALSE;
}

bool check_tumble( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     chances;

    if ( !IS_AWAKE( victim ) || IS_NPC( victim ) )
        return FALSE;
    if ( !IS_NPC( victim ) && !victim->pcdata->learned[gsn_tumble] > 0 )
        return FALSE;
    chances =
        ( int ) ( LEARNED( victim, gsn_tumble ) / sysdata.tumble_mod +
                  ( get_curr_dex( victim ) - 13 ) );
    if ( chances != 0 && victim->morph )
        chances += victim->morph->tumble;
    if ( !chance( victim, chances + victim->level - ch->level ) )
        return FALSE;
    if ( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
        act( AT_SKILL, "You tumble away from $n's attack.", ch, NULL, victim, TO_VICT );
    if ( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
        act( AT_SKILL, "$N tumbles away from your attack.", ch, NULL, victim, TO_CHAR );
    learn_from_success( victim, gsn_tumble );
    return TRUE;
}

void do_poison_weapon( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    OBJ_DATA               *pobj;
    OBJ_DATA               *wobj;
    char                    arg[MIL];
    int                     percent;

    one_argument( argument, arg );

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "What are you trying to poison?\r\n", ch );
        return;
    }
    if ( ch->fighting ) {
        send_to_char( "While you're fighting?  Nice try.\r\n", ch );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;

    if ( !( obj = get_obj_carry( ch, arg ) ) ) {
        send_to_char( "You do not have that weapon.\r\n", ch );
        return;
    }
/* Updated to include projectiles for poisoning - Aurin 10/7/2010 */
    if ( obj->item_type != ITEM_WEAPON && obj->item_type != ITEM_PROJECTILE ) {
        send_to_char( "That item is not a weapon or a projectile.\r\n", ch );
        return;
    }
    if ( IS_OBJ_STAT( obj, ITEM_POISONED ) ) {
        send_to_char( "That weapon or projectile is already poisoned.\r\n", ch );
        return;
    }
/* End of update by Aurin */
    if ( IS_OBJ_STAT( obj, ITEM_CLANOBJECT ) ) {
        send_to_char( "It doesn't appear to be fashioned of a poisonable material.\r\n", ch );
        return;
    }
    /*
     * Now we have a valid weapon...check to see if we have the powder. 
     */
    for ( pobj = ch->first_carrying; pobj; pobj = pobj->next_content ) {
        if ( pobj->pIndexData->vnum == OBJ_VNUM_BLACK_POWDER )
            break;
    }
    if ( !pobj ) {
        send_to_char( "You do not have the black poison powder.\r\n", ch );
        return;
    }
    /*
     * Okay, we have the powder...do we have water? 
     */
    for ( wobj = ch->first_carrying; wobj; wobj = wobj->next_content ) {
        if ( wobj->item_type == ITEM_DRINK_CON && wobj->value[1] > 0 && wobj->value[2] == 0 )
            break;
    }
    if ( !wobj ) {
        send_to_char( "You have no water to mix with the powder.\r\n", ch );
        return;
    }
    /*
     * Great, we have the ingredients...but is the thief smart enough? 
     */
    if ( !IS_NPC( ch ) && get_curr_wis( ch ) < 16 ) {
        send_to_char( "You can't quite remember what to do...\r\n", ch );
        return;
    }
    /*
     * And does the thief have steady enough hands? 
     */
    if ( !IS_NPC( ch )
         && ( ( get_curr_dex( ch ) < 17 ) || ch->pcdata->condition[COND_DRUNK] > 0 ) ) {
        send_to_char( "Your hands aren't steady enough to properly mix the poison.\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_poison_weapon]->beats );

    percent = ( number_percent(  ) - get_curr_lck( ch ) - 14 );

    /*
     * Check the skill percentage 
     */
    separate_obj( pobj );
    separate_obj( wobj );
    if ( !can_use_skill( ch, percent, gsn_poison_weapon ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "You failed and spill some on yourself.  Ouch!\r\n", ch );
        set_char_color( AT_GREY, ch );
        damage( ch, ch, ch->level, gsn_poison_weapon );
        act( AT_RED, "$n spills the poison all over!", ch, NULL, NULL, TO_ROOM );
        extract_obj( pobj );
        extract_obj( wobj );
        learn_from_failure( ch, gsn_poison_weapon );
        return;
    }
    separate_obj( obj );
    /*
     * Well, I'm tired of waiting.  Are you? 
     */
    act( AT_RED, "You mix $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_CHAR );
    act( AT_RED, "$n mixes $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_ROOM );
    act( AT_GREEN, "You pour the poison over $p, which glistens wickedly!", ch, obj, NULL,
         TO_CHAR );
    act( AT_GREEN, "$n pours the poison over $p, which glistens wickedly!", ch, obj, NULL,
         TO_ROOM );
    xSET_BIT( obj->extra_flags, ITEM_POISONED );
    obj->cost *= 2;
    /*
     * Set an object timer.  Don't want proliferation of poisoned weapons 
     */
    obj->timer = UMIN( obj->level, ch->level );

    if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        obj->timer *= 2;

    if ( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        obj->timer *= 2;

    /*
     * WHAT?  All of that, just for that one bit?  How lame. ;) 
     */
    act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_CHAR );
    act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_ROOM );
    extract_obj( pobj );
    extract_obj( wobj );
    learn_from_success( ch, gsn_poison_weapon );
    return;
}

void do_scribe( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *scroll;
    int                     sn;
    char                    buf1[MSL];
    char                    buf2[MSL];
    char                    buf3[MSL];
    int                     mana;

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' || !str_cmp( argument, "" ) ) {
        send_to_char( "Scribe what?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( sn = find_spell( ch, argument, TRUE ) ) < 0 ) {
        send_to_char( "You have not learned that spell.\r\n", ch );
        return;
    }

    if ( skill_table[sn]->spell_fun == spell_null ) {
        send_to_char( "That's not a spell!\r\n", ch );
        return;
    }

    if ( SPELL_FLAG( skill_table[sn], SF_NOSCRIBE ) ) {
        send_to_char( "You cannot scribe that spell.\r\n", ch );
        return;
    }

    mana =
        IS_NPC( ch ) ? 0 : UMAX( skill_table[sn]->min_mana,
                                 100 / ( 2 + ch->level -
                                         skill_table[sn]->skill_level[ch->Class] ) );

    mana *= 5;

    if ( !IS_NPC( ch ) && ch->mana < mana ) {
        send_to_char( "You don't have enough mana.\r\n", ch );
        return;
    }

    if ( ( scroll = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "You must be holding a blank scroll to scribe it.\r\n", ch );
        return;
    }

    if ( scroll->pIndexData->vnum != OBJ_VNUM_SCROLL_SCRIBING ) {
        send_to_char( "You must be holding a blank scroll to scribe it.\r\n", ch );
        return;
    }

    if ( ( scroll->value[1] != -1 )
         && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) ) {
        send_to_char( "That scroll has already been inscribed.\r\n", ch );
        return;
    }

    if ( !process_spell_components( ch, sn ) ) {
        learn_from_failure( ch, gsn_scribe );
        ch->mana -= ( mana / 2 );
        return;
    }

    if ( !can_use_skill( ch, number_percent(  ), gsn_scribe ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "You failed.\r\n", ch );
        learn_from_failure( ch, gsn_scribe );
        ch->mana -= ( mana / 2 );
        return;
    }

    scroll->value[1] = sn;
    scroll->value[0] = ch->level;
    snprintf( buf1, MSL, "%s scroll", skill_table[sn]->name );
    if ( VLD_STR( scroll->short_descr ) )
        STRFREE( scroll->short_descr );
    scroll->short_descr = STRALLOC( aoran( buf1 ) );

    snprintf( buf2, MSL, "A glowing scroll inscribed '%s' lies in the dust.",
              skill_table[sn]->name );
    if ( VLD_STR( scroll->description ) )
        STRFREE( scroll->description );
    scroll->description = STRALLOC( buf2 );

    snprintf( buf3, MSL, "scroll scribing %s", skill_table[sn]->name );
    if ( VLD_STR( scroll->name ) )
        STRFREE( scroll->name );
    scroll->name = STRALLOC( buf3 );

    act( AT_MAGIC, "$n magically scribes $p.", ch, scroll, NULL, TO_ROOM );
    act( AT_MAGIC, "You magically scribe $p.", ch, scroll, NULL, TO_CHAR );

    learn_from_success( ch, gsn_scribe );

    ch->mana -= mana;

}

void do_brew( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *potion;
    OBJ_DATA               *fire;
    int                     sn;
    char                    buf1[MSL];
    char                    buf2[MSL];
    char                    buf3[MSL];
    int                     mana;
    bool                    found;

    if ( IS_NPC( ch ) )
        return;

    if ( argument[0] == '\0' || !str_cmp( argument, "" ) ) {
        send_to_char( "Brew what?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( sn = find_spell( ch, argument, TRUE ) ) < 0 ) {
        send_to_char( "You have not learned that spell.\r\n", ch );
        return;
    }

    if ( skill_table[sn]->spell_fun == spell_null ) {
        send_to_char( "That's not a spell!\r\n", ch );
        return;
    }

    if ( SPELL_FLAG( skill_table[sn], SF_NOBREW ) ) {
        send_to_char( "You cannot brew that spell.\r\n", ch );
        return;
    }

    mana =
        IS_NPC( ch ) ? 0 : UMAX( skill_table[sn]->min_mana,
                                 100 / ( 2 + ch->level -
                                         skill_table[sn]->skill_level[ch->Class] ) );

    mana *= 4;

    if ( !IS_NPC( ch ) && ch->mana < mana ) {
        send_to_char( "You don't have enough mana.\r\n", ch );
        return;
    }

    found = FALSE;

    for ( fire = ch->in_room->first_content; fire; fire = fire->next_content ) {
        if ( fire->item_type == ITEM_FIRE ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "There must be a fire in the room to brew a potion.\r\n", ch );
        return;
    }

    if ( ( potion = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "You must be holding an empty flask to brew a potion.\r\n", ch );
        return;
    }

    if ( potion->pIndexData->vnum != OBJ_VNUM_FLASK_BREWING ) {
        send_to_char( "You must be holding an empty flask to brew a potion.\r\n", ch );
        return;
    }

    if ( ( potion->value[1] != -1 ) && ( potion->pIndexData->vnum == OBJ_VNUM_FLASK_BREWING ) ) {
        send_to_char( "That's not an empty flask.\r\n", ch );
        return;
    }

    if ( !process_spell_components( ch, sn ) ) {
        learn_from_failure( ch, gsn_brew );
        ch->mana -= ( mana / 2 );
        return;
    }

    if ( !can_use_skill( ch, number_percent(  ), gsn_brew ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "You failed.\r\n", ch );
        learn_from_failure( ch, gsn_brew );
        ch->mana -= ( mana / 2 );
        return;
    }

    potion->value[1] = sn;
    potion->value[0] = ch->level;
    snprintf( buf1, MSL, "%s potion", skill_table[sn]->name );
    if ( VLD_STR( potion->short_descr ) )
        STRFREE( potion->short_descr );
    potion->short_descr = STRALLOC( aoran( buf1 ) );

    snprintf( buf2, MSL, "A strange potion labelled '%s' sizzles in a glass flask.",
              skill_table[sn]->name );
    if ( VLD_STR( potion->description ) )
        STRFREE( potion->description );
    potion->description = STRALLOC( buf2 );

    snprintf( buf3, MSL, "flask potion %s", skill_table[sn]->name );
    if ( VLD_STR( potion->name ) )
        STRFREE( potion->name );
    potion->name = STRALLOC( buf3 );

    act( AT_MAGIC, "$n brews up $p.", ch, potion, NULL, TO_ROOM );
    act( AT_MAGIC, "You brew up $p.", ch, potion, NULL, TO_CHAR );

    learn_from_success( ch, gsn_brew );

    ch->mana -= mana;
}

bool check_grip( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     schance;

    schance = number_range( 1, 20 );

    if ( !IS_AWAKE( victim ) )
        return FALSE;

    if ( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_GRIP ) )
        return FALSE;

    schance += ( get_curr_lck( ch ) + get_curr_dex( ch ) / 2 );

    if ( schance < 30 ) {
        learn_from_failure( victim, gsn_grip );
        return FALSE;
    }
    act( AT_GREEN, "&GYou evade $n's attempt to disarm you.", ch, NULL, victim, TO_VICT );
    act( AT_GREEN, "&G$N holds $S weapon strongly, and is not disarmed.", ch, NULL, victim,
         TO_CHAR );
    learn_from_success( victim, gsn_grip );
    return TRUE;
}

// New cool sounding offensive skill for thieves - uh yah

void do_arteries( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( ch->mount ) {
        send_to_char( "You can't attack the arteries while mounted.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Target arteries on who?\r\n", ch );
        return;
    }

    // Below modified to allow one to strike someone they're fighting even
    // if they can't see them. -Taon
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    if ( is_same_group( victim, ch ) )
        return;

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
        return;
    }

    if ( obj->value[4] != WEP_1H_SHORT_BLADE ) {
        if ( ( obj->value[4] == WEP_1H_LONG_BLADE && obj->value[3] != DAM_PIERCE )
             || obj->value[4] != WEP_1H_LONG_BLADE ) {
            send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
            return;
        }
    }
    percent = number_percent(  ) - ( get_curr_lck( ch ) - 16 ) + ( get_curr_lck( victim ) - 13 );

    check_attacker( ch, victim );

    if ( victim == ch ) {
        victim = who_fighting( ch );
    }
    WAIT_STATE( ch, skill_table[gsn_arteries]->beats );
    if ( can_use_skill( ch, percent, gsn_arteries ) ) {
        adjust_favor( ch, 10, 1 );
        learn_from_success( ch, gsn_arteries );
        WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

        if ( !can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 ) {
            send_to_char( "&RYour victim doesn't even see you coming!&w\r\n", ch );
            global_retcode = damage( ch, victim, maximum, gsn_arteries );
        }
        else if ( can_see( victim, ch ) && ch->secondclass == -1 ) {
            global_retcode = damage( ch, victim, insane, gsn_arteries );
        }
        else if ( can_see( victim, ch ) && ch->secondclass != -1 && ch->thirdclass == -1 ) {
            global_retcode = damage( ch, victim, ludicrous + ( ch->level / 2 ), gsn_arteries );
        }
        else if ( can_see( victim, ch ) && ch->secondclass != -1 && ch->thirdclass != -1 ) {
            global_retcode = damage( ch, victim, ludicrous, gsn_arteries );
        }
    }
    else {
        act( AT_PLAIN, "&GYou almost cut your damn self!", ch, NULL, victim, TO_CHAR );
        learn_from_failure( ch, gsn_arteries );
        global_retcode = damage( ch, victim, number_range( 1, 10 ), gsn_arteries );
    }
    return;

}

void do_circle( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( ch->mount ) {
        send_to_char( "You can't circle while mounted.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Circle around who?\r\n", ch );
        return;
    }

    // Below modified to allow one to strike someone they're fighting even
    // if they can't see them. -Taon
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    if ( is_same_group( victim, ch ) )
        return;

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
        return;
    }

    if ( obj->value[4] != WEP_1H_SHORT_BLADE ) {
        if ( ( obj->value[4] == WEP_1H_LONG_BLADE && obj->value[3] != DAM_PIERCE )
             || obj->value[4] != WEP_1H_LONG_BLADE ) {
            send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
            return;
        }
    }

    if ( !ch->fighting ) {
        send_to_char( "You can't circle when you aren't fighting.\r\n", ch );
        return;
    }

    if ( !victim->fighting ) {
        send_to_char( "You can't circle around a person who is not fighting.\r\n", ch );
        return;
    }
/* temporarly removing this, if makes theives too powerful may reinstate
  if(victim->num_fighting < 2)
  {
    act(AT_PLAIN, "You can't circle around them without a distraction.", ch, NULL, victim, TO_CHAR);
    return;
  }
*/
    percent = number_percent(  ) - ( get_curr_lck( ch ) - 16 ) + ( get_curr_lck( victim ) - 13 );

    check_attacker( ch, victim );

    if ( victim == ch ) {
        victim = who_fighting( ch );
    }
    WAIT_STATE( ch, skill_table[gsn_circle]->beats );
    if ( can_use_skill( ch, percent, gsn_circle ) ) {
        adjust_favor( ch, 10, 1 );
        learn_from_success( ch, gsn_circle );
        WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
        if ( ch->secondclass == -1 ) {
            global_retcode = damage( ch, victim, insane, gsn_circle );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
            global_retcode = damage( ch, victim, ludicrous + ( ch->level / 2 ), gsn_circle );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
            global_retcode = damage( ch, victim, ludicrous, gsn_circle );
        }
    }
    else {
        act( AT_PLAIN, "&GYou almost cut your damn self!", ch, NULL, victim, TO_CHAR );
        learn_from_failure( ch, gsn_circle );
        global_retcode = damage( ch, victim, number_range( 1, 10 ), gsn_circle );
    }
    return;
}

/* Berserk and HitAll. -- Altrag */
void do_berserk( CHAR_DATA *ch, char *argument )
{
    short                   percent;
    AFFECT_DATA             af;

    if ( !ch->fighting ) {
        send_to_char( "But you aren't fighting!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_BERSERK ) ) {
        send_to_char( "Your rage is already at its peak!\r\n", ch );
        return;
    }

    percent = LEARNED( ch, gsn_berserk );
    WAIT_STATE( ch, skill_table[gsn_berserk]->beats );
    if ( !chance( ch, percent ) ) {
        send_to_char( "You couldn't build up enough rage.\r\n", ch );
        learn_from_failure( ch, gsn_berserk );
        return;
    }
    af.type = gsn_berserk;
    /*
     * Modified af.duration to truely last the entire length of combat. 
     * Also modified stop_fighting function to help support this feature. -Taon
     */
    af.duration = -1;
    /*
     * Hmm.. you get stronger when yer really enraged.. mind over matter
     * type thing.. 
     */
    af.location = APPLY_STR;

    if ( ch->level >= 20 )
        af.modifier = ch->level / 10;
    else
        af.modifier = 2;

    af.bitvector = meb( AFF_BERSERK );
    af.level = ch->level;
    affect_to_char( ch, &af );
    if ( ch->Class != CLASS_BALROG )
        send_to_char( "You start to lose control..\r\n", ch );
    else
        send_to_char( "In a fit of rage you summon your inner beast.\r\n", ch );
    learn_from_success( ch, gsn_berserk );
    return;
}

//Reminder: Rewrite this to support how many attacks one actually has. -Taon
void do_hitall( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    short                   nvict = 0;
    short                   nhit = 0;
    short                   percent;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char_color( "&BA godly force prevents you.\r\n", ch );
        return;
    }

    if ( !ch->in_room->first_person ) {
        send_to_char( "There's no one else here!\r\n", ch );
        return;
    }
    percent = LEARNED( ch, gsn_hitall );
    short                   vchance;

    vchance = number_range( 1, 4 );
    for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
        vch_next = vch->next_in_room;
        if ( is_same_group( ch, vch ) || !is_legal_kill( ch, vch ) || !can_see( ch, vch )
             || is_safe( ch, vch, TRUE ) )
            continue;

        if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
            continue;

        if ( ++nvict > ch->level / 5 )
            break;
        check_illegal_pk( ch, vch );
        if ( vchance < 4 ) {
            nhit++;
            global_retcode = one_hit( ch, vch, TYPE_UNDEFINED );
        }
        else
            global_retcode = damage( ch, vch, 0, TYPE_UNDEFINED );
        /*
         * Fireshield, etc. could kill ch too.. :>.. -- Altrag 
         */
        if ( global_retcode == rCHAR_DIED || global_retcode == rBOTH_DIED || char_died( ch ) )
            return;
    }
    if ( !nvict ) {
        send_to_char( "There's no one else here!\r\n", ch );
        return;
    }
    ch->move = UMAX( 0, ch->move - nvict * 3 + nhit );
    if ( nhit ) {
        WAIT_STATE( ch, skill_table[gsn_hitall]->beats );
        learn_from_success( ch, gsn_hitall );
    }
    if ( !nhit ) {
        learn_from_failure( ch, gsn_hitall );
    }
    return;
}

bool check_illegal_psteal( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !IS_NPC( victim ) && !IS_NPC( ch ) ) {
        if ( ( !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY )
               || ch->level - victim->level > 10
               || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) && ( ch->in_room->vnum < 29
                                                                     || ch->in_room->vnum >
                                                                     43 ) && ch != victim ) {
            return TRUE;
        }
    }
    return FALSE;
}

void do_find_trap( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *was_in_room;
    EXIT_DATA              *pexit;
    short                   dir = -1;
    short                   dist;
    short                   max_dist = 15;

    set_char_color( AT_ACTION, ch );

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) ) {
        send_to_char( "Not very effective when you're blind...\r\n", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Attempt to find traps in what direction?\r\n", ch );
        return;
    }

    if ( ( dir = get_door( argument ) ) < 0 ) {
        send_to_char( "Attempt to find traps in what direction?\r\n", ch );
        return;
    }

    act( AT_GREY, "Attempting to find a trap to the $t.", ch, dir_name[dir], NULL, TO_CHAR );
    act( AT_GREY, "$n attempts to find traps to the $t.", ch, dir_name[dir], NULL, TO_ROOM );

    if ( ( pexit = get_exit( ch->in_room, dir ) ) == NULL ) {
        act( AT_GREY, "You can't see any traps to the $t.", ch, dir_name[dir], NULL, TO_CHAR );
        return;
    }

    return;
}

void do_scan( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *was_in_room;
    EXIT_DATA              *pexit;
    short                   dir = -1;
    short                   dist;
    short                   max_dist = 15;

    set_char_color( AT_ACTION, ch );

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) ) {
        send_to_char( "Not very effective when you're blind...\r\n", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Scan in a direction...\r\n", ch );
        return;
    }

    if ( ( dir = get_door( argument ) ) == -1 ) {
        send_to_char( "Scan in WHAT direction?\r\n", ch );
        return;
    }

    was_in_room = ch->in_room;
    act( AT_GREY, "Scanning $t...", ch, dir_name[dir], NULL, TO_CHAR );
    act( AT_GREY, "$n scans $t.", ch, dir_name[dir], NULL, TO_ROOM );

    if ( ( pexit = get_exit( ch->in_room, dir ) ) == NULL ) {
        act( AT_GREY, "You can't see $t.", ch, dir_name[dir], NULL, TO_CHAR );
        return;
    }

    // Those with AFF_SIGHT gets slight bonus. -Taon 
    if ( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
        ++max_dist;
    if ( ch->level < LEVEL_DEMIGOD )
        --max_dist;
    if ( ch->level < ( ( LEVEL_DEMIGOD / 5 ) * 4 ) )
        --max_dist;
    if ( ch->level < ( ( LEVEL_DEMIGOD / 5 ) * 3 ) )
        --max_dist;

    for ( dist = 1; dist <= max_dist; ) {
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            if ( IS_SET( pexit->exit_info, EX_SECRET )
                 || IS_SET( pexit->exit_info, EX_DIG ) )
                act( AT_GREY, "Your view $t is blocked by a wall.", ch, dir_name[dir], NULL,
                     TO_CHAR );
            else
                act( AT_GREY, "Your view $t is blocked by a door.", ch, dir_name[dir], NULL,
                     TO_CHAR );
            break;
        }
        if ( room_is_private( pexit->to_room ) && ch->level < LEVEL_AJ_SGT ) {
            act( AT_GREY, "Your view $t is blocked by a private room.", ch, dir_name[dir],
                 NULL, TO_CHAR );
            break;
        }
        char_from_room( ch );
        char_to_room( ch, pexit->to_room );
        set_char_color( AT_RMNAME, ch );

        if ( !IN_WILDERNESS( ch ) )
            send_to_char( ch->in_room->name, ch );
        else
            gen_wilderness_name( ch );

        send_to_char( "\r\n", ch );
        show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
        show_char_to_char( ch->in_room->first_person, ch );

        switch ( ch->in_room->sector_type ) {
            default:
                dist++;
                break;
            case SECT_AIR:
                if ( number_percent(  ) < 80 )
                    dist++;
                break;
            case SECT_INSIDE:
            case SECT_FIELD:
            case SECT_UNDERGROUND:
                dist++;
                break;
            case SECT_FOREST:
            case SECT_ROAD:
            case SECT_HROAD:
            case SECT_VROAD:
            case SECT_DESERT:
            case SECT_HILLS:
                dist += 1;
                break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
                dist += 1;
                break;
            case SECT_MOUNTAIN:
            case SECT_UNDERWATER:
            case SECT_OCEANFLOOR:
                dist += 2;
                break;
        }

        if ( dist >= max_dist ) {
            act( AT_GREY, "Your vision blurs with distance and you see no " "farther $t.",
                 ch, dir_name[dir], NULL, TO_CHAR );
            break;
        }
        if ( ( pexit = get_exit( ch->in_room, dir ) ) == NULL ) {
            act( AT_GREY, "Your view $t is blocked by a wall.", ch, dir_name[dir], NULL, TO_CHAR );
            break;
        }
    }
    // Bug fix, players weren't learning. -Taon
    learn_from_success( ch, gsn_scan );
    char_from_room( ch );
    char_to_room( ch, was_in_room );
    return;
}

ch_ret                  spell_attack( int, int, CHAR_DATA *, void * );

/* -- working on -- 
 * Syntaxes: throw object  (assumed already fighting)
 *       throw object direction target  (all needed args for distance 
 *            throwing)
 *       throw object  (assumed same room throw)

void do_throw(CHAR_DATA *ch, char *argument)
{
  ROOM_INDEX_DATA *was_in_room;
  CHAR_DATA *victim;
  OBJ_DATA *throw_obj;
  EXIT_DATA *pexit;
  short dir;
  short dist;
  short max_dist = 3;
  char arg[MIL];
  char arg1[MIL];
  char arg2[MIL];

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  for(throw_obj = ch->last_carrying; throw_obj;
  throw_obj = throw_obj=>prev_content)
  {
---    if(can_see_obj(ch, throw_obj)
  &&(throw_obj->wear_loc == WEAR_HELD || throw_obj->wear_loc == 
  WEAR_WIELDED || throw_obj->wear_loc == WEAR_DUAL_WIELDED)
  && nifty_is_name(arg, throw_obj->name))
      break;
 ----
    if(can_see_obj(ch, throw_obj) && nifty_is_name(arg, throw_obj->name)
      break;
  }

  if(!throw_obj)
  {
    send_to_char("You aren't holding or wielding anything like that.\r\n", ch);
    return;
  }

----
  if((throw_obj->item_type != ITEM_WEAPON)
  {
    send_to_char("You can only throw weapons.\r\n", ch);
    return;
  }
----

  if(get_obj_weight(throw_obj) -(3 * (get_curr_str(ch) - 15)) > 0)
  {
    send_to_char("That is too heavy for you to throw.\r\n", ch);
    if(!number_range(0,10))
      learn_from_failure(ch, gsn_throw);
    return;
  }

  if(ch->fighting)
    victim = ch->fighting;
   else
    {
      if(((victim = get_char_room(ch, arg1)) == NULL)
    &&(arg2[0] == '\0'))
      {
        act(AT_GREY, "Throw $t at whom?", ch, obj->short_descr, NULL,  
    TO_CHAR);
        return;
      }
    }
}*/

void do_slice( CHAR_DATA *ch, char *argument )
{
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *corpse;
    OBJ_DATA               *obj;
    OBJ_DATA               *slice;
    bool                    found;
    MOB_INDEX_DATA         *pMobIndex;
    char                    buf[MSL];
    char                    buf1[MSL];
    int                     uvnum;

    found = FALSE;

    /*
     * Noticed that it was checking gsn_kick.  Bug report by Li'l Lukey 
     */
    if ( !IS_NPC( ch ) && !IS_IMMORTAL( ch )
         && ch->level < skill_table[gsn_slice]->skill_level[ch->Class] ) {
        send_to_char( "You are not learned in this skill.\r\n", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "From what do you wish to slice meat?\r\n", ch );
        return;
    }

    if ( ch->race != RACE_DRAGON ) {
        if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL
             || ( obj->value[3] != 1 && obj->value[3] != 2 && obj->value[3] != 3
                  && obj->value[3] != 6 ) ) {
            send_to_char
                ( "You need to wield a sharp weapon that can slash, stab, pierce, or hack.\r\n",
                  ch );
            return;
        }
    }
    if ( ( corpse = get_obj_here( ch, argument ) ) == NULL ) {
        send_to_char( "You can't find that here.\r\n", ch );
        return;
    }

    if ( corpse->item_type != ITEM_CORPSE_NPC || corpse->value[3] <= 75 ) {
        send_to_char( "There is no more suitable meat to be found there.\r\n", ch );
        return;
    }

    if ( ( pMobIndex = get_mob_index( ( int ) -( corpse->cost ) ) ) == NULL ) {
        bug( "%s", "Can not find mob for cost of corpse, do_slice" );
        return;
    }

    if ( pMobIndex->slicevnum > 0 )
        uvnum = pMobIndex->slicevnum;
    else
        uvnum = OBJ_VNUM_SLICE;

    if ( ( pObjIndex = get_obj_index( uvnum ) ) == NULL ) {
        if ( uvnum != OBJ_VNUM_SLICE ) {
            bug( "%s: Can't find object %d to use.", __FUNCTION__, uvnum );
            uvnum = OBJ_VNUM_SLICE;
            if ( ( pObjIndex = get_obj_index( uvnum ) ) == NULL ) {
                bug( "%s: Can't find object %d to use.", __FUNCTION__, uvnum );
                return;
            }
        }
        else {
            bug( "%s: Can't find object %d to use.", __FUNCTION__, uvnum );
            return;
        }
    }
    if ( pMobIndex->slicevnum > 0 ) {
        slice = create_object( pObjIndex, 0 );
        slice->level = pMobIndex->level;
        act( AT_BLOOD, "$n cuts into the corpse of $p.", ch, corpse, NULL, TO_ROOM );
        act( AT_BLOOD, "Your blade suddenly snags on something and you uncover $p.", ch, slice,
             NULL, TO_CHAR );
        obj_to_char( slice, ch );
        extract_obj( corpse );
        return;
    }
    if ( pMobIndex->slicevnum == 0 && ch->race == RACE_DRAGON ) {
        send_to_char( "There is nothing to be found in there.\r\n", ch );
        return;
    }

    if ( !can_use_skill( ch, number_percent(  ), gsn_slice ) && !IS_IMMORTAL( ch ) ) {
        send_to_char( "&GYou almost cut your damn self!\r\n", ch );
        learn_from_failure( ch, gsn_slice );           /* Just in case they die :> */
        if ( number_percent(  ) + ( get_curr_dex( ch ) - 13 ) < 10 ) {
            act( AT_GREEN, "You cut your damn self!", ch, NULL, NULL, TO_CHAR );
            damage( ch, ch, ch->level, gsn_slice );
        }
        return;
    }

    slice = create_object( pObjIndex, 0 );

    snprintf( buf, MSL, "meat fresh slice %s", pMobIndex->player_name );
    if ( VLD_STR( slice->name ) )
        STRFREE( slice->name );
    slice->name = STRALLOC( buf );

    snprintf( buf, MSL, "a slice of raw meat from %s", pMobIndex->short_descr );
    if ( VLD_STR( slice->short_descr ) )
        STRFREE( slice->short_descr );
    slice->short_descr = STRALLOC( buf );

    snprintf( buf1, MSL, "A slice of raw meat from %s lies on the ground.",
              pMobIndex->short_descr );
    if ( VLD_STR( slice->description ) )
        STRFREE( slice->description );
    slice->description = STRALLOC( buf1 );

    act( AT_BLOOD, "$n cuts a slice of meat from $p.", ch, corpse, NULL, TO_ROOM );
    act( AT_BLOOD, "You cut a slice of meat from $p.", ch, corpse, NULL, TO_CHAR );

    obj_to_char( slice, ch );
    corpse->value[3] -= 25;
    learn_from_success( ch, gsn_slice );
}

/*  New check to see if you can use skills to support morphs --Shaddai */
bool can_use_skill( CHAR_DATA *ch, int percent, int gsn )
{
    bool                    check = FALSE;

    if ( IS_NPC( ch ) && percent < 85 )
        check = TRUE;
    else if ( !IS_NPC( ch ) && percent < LEARNED( ch, gsn ) )
        check = TRUE;
    else if ( ch->morph && ch->morph->morph && ch->morph->morph->skills
              && ch->morph->morph->skills[0] != '\0'
              && is_name( skill_table[gsn]->name, ch->morph->morph->skills ) && percent < 85 )
        check = TRUE;
    if ( ch->morph && ch->morph->morph && ch->morph->morph->no_skills
         && ch->morph->morph->no_skills[0] != '\0'
         && is_name( skill_table[gsn]->name, ch->morph->morph->no_skills ) )
        check = FALSE;
    return check;
}

/* Cook was coded by Blackmane and heavily modified by Shaddai */
void do_cook( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *food,
                           *fire;
    char                    arg[MIL];
    char                    buf[MSL];

    one_argument( argument, arg );
    if ( IS_NPC( ch ) || ch->level < skill_table[gsn_cook]->skill_level[ch->Class] ) {
        send_to_char( "That skill is beyond your understanding.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "Cook what?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( food = get_obj_carry( ch, arg ) ) == NULL ) {
        send_to_char( "You do not have that item.\r\n", ch );
        return;
    }
    if ( food->item_type != ITEM_COOK ) {
        send_to_char( "How can you cook that?\r\n", ch );
        return;
    }
    if ( food->value[2] > 2 ) {
        send_to_char( "That is already burnt to a crisp.\r\n", ch );
        return;
    }
    for ( fire = ch->in_room->first_content; fire; fire = fire->next_content ) {
        if ( fire->item_type == ITEM_FIRE )
            break;
    }
    if ( !fire ) {
        send_to_char( "There is no fire here!\r\n", ch );
        return;
    }
    separate_obj( food );                              /* Bug catch by Tchaika from SMAUG 
                                                        * list */
    if ( number_percent(  ) > LEARNED( ch, gsn_cook ) ) {
        food->timer = food->timer / 2;
        food->value[0] = 0;
        food->value[2] = 3;
        act( AT_MAGIC, "$p catches on fire burning it to a crisp!\r\n", ch, food, NULL, TO_CHAR );
        act( AT_MAGIC, "$n catches $p on fire burning it to a crisp.", ch, food, NULL, TO_ROOM );
        snprintf( buf, MSL, "a burnt %s", food->pIndexData->name );
        if ( VLD_STR( food->short_descr ) )
            STRFREE( food->short_descr );
        food->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "A burnt %s.", food->pIndexData->name );
        if ( VLD_STR( food->description ) )
            STRFREE( food->description );
        food->description = STRALLOC( buf );
        return;
    }

    if ( number_percent(  ) > 85 ) {
        food->timer = food->timer * 3;
        food->value[2] += 2;
        act( AT_MAGIC, "$n overcooks $p.", ch, food, NULL, TO_ROOM );
        act( AT_MAGIC, "You overcook $p.", ch, food, NULL, TO_CHAR );
        snprintf( buf, MSL, "an overcooked %s", food->pIndexData->name );
        if ( VLD_STR( food->short_descr ) )
            STRFREE( food->short_descr );
        food->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "An overcooked %s.", food->pIndexData->name );
        if ( VLD_STR( food->description ) )
            STRFREE( food->description );
        food->description = STRALLOC( buf );
    }
    else {
        food->timer = food->timer * 4;
        food->value[0] *= 2;
        act( AT_MAGIC, "$n roasts $p.", ch, food, NULL, TO_ROOM );
        act( AT_MAGIC, "You roast $p.", ch, food, NULL, TO_CHAR );
        snprintf( buf, MSL, "a roasted %s", food->pIndexData->name );
        if ( VLD_STR( food->short_descr ) )
            STRFREE( food->short_descr );
        food->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "A roasted %s.", food->pIndexData->name );
        if ( VLD_STR( food->description ) )
            STRFREE( food->description );
        food->description = STRALLOC( buf );
        food->value[2]++;
    }
    learn_from_success( ch, gsn_cook );
}

//This is a rewrite of sharpen losely based off of Rantics,
//old sharpen code. -Taon
/* Volk - actually I wrote this from scratch and i've never heard of Rantics. */
void do_sharpen( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj,
                           *pobj;
    char                    arg[MIL];
    short                   chance,
                            bchance;

    one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( ch->pcdata->learned[gsn_sharpen] < 1 ) {
        send_to_char( "You're not skilled enough to complete such a task.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "Sharpen what?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( !( obj = get_obj_carry( ch, arg ) ) ) {
        send_to_char( "You don't seem to have that weapon.\r\n ", ch );
        return;
    }

    if ( obj->pIndexData->vnum == OBJ_VNUM_JUDGE || obj->pIndexData->vnum == OBJ_VNUM_BONE ) {
        send_to_char( "That object is magical and may not be sharpened.\r\n", ch );
        return;
    }

    if ( obj->item_type != ITEM_WEAPON && obj->item_type != ITEM_PROJECTILE ) {
        send_to_char( "You can only sharpen weapons and projectiles.\r\n", ch );
        return;
    }
    if ( !CAN_SHARPEN( obj ) ) {
        send_to_char( "You cannot sharpen this type of weapon.\r\n", ch );
        return;
    }

    if ( xIS_SET( obj->extra_flags, ITEM_ANTI_EVIL ) || xIS_SET( obj->extra_flags, ITEM_ANTI_GOOD )
         || xIS_SET( obj->extra_flags, ITEM_ANTI_NEUTRAL ) ) {
        send_to_char( "You cannot sharpen this type of weapon.\r\n", ch );
        return;
    }

    if ( obj->level > ch->level ) {
        send_to_char( "This weapon is beyond your sharpening ability.\r\n", ch );
        return;
    }
    if ( obj->value[6] == 100 ) {
        send_to_char( "This item is as sharp as its going to get.\r\n", ch );
        return;
    }

    for ( pobj = ch->first_carrying; pobj; pobj = pobj->next_content ) {
        if ( pobj->pIndexData->item_type == ITEM_SHARPEN )
            break;
    }
    if ( !pobj ) {
        send_to_char( "You need a sharpening stone.\r\n", ch );
        return;
    }

    if ( ch->pcdata->learned[gsn_sharpen] < 50 )
        chance = ch->pcdata->learned[gsn_sharpen] + number_chance( 1, 45 );
    else
        chance = ch->pcdata->learned[gsn_sharpen] + number_chance( 1, 35 );

    if ( get_curr_dex( ch ) > 20 )
        chance += get_curr_dex( ch ) + 10;
    else
        chance += get_curr_dex( ch );

    if ( get_curr_lck( ch ) > 20 )
        chance += get_curr_lck( ch ) + 10;
    else
        chance += get_curr_lck( ch );

    separate_obj( obj );
    WAIT_STATE( ch, skill_table[gsn_sharpen]->beats );
    if ( chance > 100 ) {
        act( AT_RED, "You skillfully sharpen $p!", ch, obj, NULL, TO_CHAR );
        obj->value[6] += number_chance( 1, 3 );
        learn_from_success( ch, gsn_sharpen );

        if ( obj->value[6] > 100 )
            obj->value[6] = 100;
        // update_weapon ( ch, obj );
        return;
    }
    else if ( chance < 70 || number_chance( 1, 5 ) == 2 ) {
        act( AT_RED, "You slip and damage $p!", ch, obj, NULL, TO_CHAR );
        obj->value[6] -= number_chance( 3, 6 );

        if ( obj->value[6] <= 1 ) {
            act( AT_RED, "$p breaks apart and falls to the ground in pieces!.", ch, obj,
                 NULL, TO_CHAR );
            make_scraps( obj );
        }

        learn_from_failure( ch, gsn_sharpen );
        // update_weapon ( ch, obj );
        return;
    }

    act( AT_RED, "You fail to sharpen $p!", ch, obj, NULL, TO_CHAR );
    learn_from_failure( ch, gsn_sharpen );
}

void do_fly( CHAR_DATA *ch, char *argument )
{

    one_argument( argument, argument );

    if ( ch->race != RACE_PIXIE ) {
        if ( ch->race == RACE_DRAGON ) {
            if ( !str_cmp( argument, "home" ) )
                do_fly_home( ch, ( char * ) "" );
            else
                do_wings( ch, ( char * ) "" );
            return;
        }
        error( ch );
        return;
    }

    if ( !can_fly( ch ) )
        return;

    send_to_char( "You unfold your wings and take to the air.\r\n", ch );
    xSET_BIT( ch->affected_by, AFF_FLYING );
    return;
}

void do_shapeshift( CHAR_DATA *ch, char *argument )
{
    MORPH_DATA             *morph;
    AFFECT_DATA             af;
    char                    arg[MSL];
    short                   percent;
    short                   manacost;
    bool                    fForm = FALSE;

    CHAR_DATA              *victim = NULL;

    if ( !ch ) {
        bug( "%s", "do_shapeshift: NULL ch!" );
        return;
    }

    if ( IS_NPC( ch ) )
        return;

    if ( ch->fighting ) {
        send_to_char( "Not while you're still fighting!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_SHAPESHIFT ) ) {
        send_to_char( "You are already using an alternative form!\r\n", ch );
        return;
    }

    if ( global_retcode == rCHAR_DIED || global_retcode == rBOTH_DIED || char_died( ch ) )
        return;

    argument = one_argument( argument, arg );

    if ( !arg || arg[0] == '\0' ) {
        int                     mcnt = 0;

        send_to_char( "&CSyntax: shapeshift < key word >\r\n", ch );
        send_to_char( "Syntax: unshift\r\n\r\n", ch );
        /*
         * Search Morph CLASS for Matching Forms 
         */
        send_to_pager
            ( "\r\n&CThe Following Forms are Available:\r\n&W-[ &GKey Words&W ]---[&G Form Description &W]--  [&G Minimum Level &W]--&w\r\n",
              ch );
        if ( !morph_start ) {
            send_to_pager( "&R (None Available) &w\r\n", ch );
            return;
        }
        for ( morph = morph_start; morph; morph = morph->next ) {
            /*
             * Check Level 
             */
            if ( morph->level > ch->level )
                continue;
            /*
             * Check all classes 
             */
            if ( morph->Class != 0 && !IS_SET( morph->Class, ch->Class )
                 && !IS_SET( morph->Class, ch->secondclass )
                 && !IS_SET( morph->Class, ch->thirdclass ) )
                continue;
            /*
             * Check race 
             */
            if ( morph->race != 0 && !IS_SET( morph->race, ch->race ) )
                continue;
            mcnt++;
            pager_printf_color( ch, "&C%-15.15s &W:&w%-30.30s &C%-3d&w\r\n",
                                morph->key_words, morph->short_desc, morph->level );
        }

        if ( mcnt == 0 )
            send_to_pager( "&R (None Available)&w\r\n", ch );

        send_to_char( "&W-------------------------------------------------------&w\r\n", ch );
    }
    else {
        char                    buf[MSL];

        /*
         * Check for Valid Form Name and Transform
         */
        if ( !morph_start ) {
            send_to_pager( "&R(No Forms Available)&w\r\n", ch );
            return;
        }

        for ( morph = morph_start; morph; morph = morph->next ) {
            /*
             * Check Level 
             */
            if ( morph->level > ch->level )
                continue;
            /*
             * Check all classes 
             */
            if ( morph->Class != 0 && !IS_SET( morph->Class, ch->Class )
                 && !IS_SET( morph->Class, ch->secondclass )
                 && !IS_SET( morph->Class, ch->thirdclass ) )
                continue;
            /*
             * Check race 
             */
            if ( morph->race != 0 && !IS_SET( morph->race, ch->race ) )
                continue;
            if ( !nifty_is_name( arg, morph->key_words ) )
                continue;
            fForm = TRUE;
            break;
        }

        if ( !fForm ) {
            send_to_char( "&YNo such alternative form for you to change into.&w\r\n", ch );
            return;
        }
        if ( morph->level > ch->level ) {
            send_to_char( "&YYou are not high enough level to assume that form.&w\r\n", ch );
            return;
        }
        percent = LEARNED( ch, gsn_shapeshift );
        WAIT_STATE( ch, skill_table[gsn_shapeshift]->beats );

        /*
         * Volk put this here 
         */
        manacost = ( morph->level * 2 );

        if ( !IS_BLOODCLASS( ch ) && ( ch->mana < manacost ) ) {
            send_to_char( "&YYou do not have enough mana to take that form.&w\r\n", ch );
            return;
        }

        if ( IS_BLOODCLASS( ch ) && ( ch->blood < ( manacost / 4 ) ) ) {
            send_to_char( "&YYou do not have enough blood to take that form.&w\r\n", ch );
            return;
        }

        if ( !chance( ch, percent ) || !fForm ) {
            send_to_char( "&YYou fail to assume an alternative form.&w\r\n", ch );
            learn_from_failure( ch, gsn_shapeshift );
            return;
        }

        if ( fForm ) {
            snprintf( buf, MSL, "%d", morph->vnum );
            do_imm_morph( ch, buf );
        }

        if ( ch->morph == NULL || ch->morph->morph == NULL ) {
            fForm = FALSE;
            return;
        }
        af.type = gsn_shapeshift;
        af.duration = ch->level + 20;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_SHAPESHIFT );
        affect_to_char( ch, &af );
        learn_from_success( ch, gsn_shapeshift );
        if ( IS_BLOODCLASS( ch ) )
            ch->blood -= manacost / 4;
        else
            ch->mana -= manacost;
        act( AT_LBLUE, "You feel a sharp pain as your body assumes another shape!", ch,
             NULL, victim, TO_CHAR );
        act( AT_LBLUE,
             "You see a hideous shape, and hear bones cracking to form $n before you!", ch,
             NULL, victim, TO_NOTVICT );
    }
}

void do_unshift( CHAR_DATA *ch, char *argument )
{
    if ( ( skill_table[gsn_shapeshift]->skill_level[ch->Class] == -1
           && !IS_IMMORTAL( ch ) ) && ( LEARNED( ch, gsn_shapeshift ) < 1 ) ) {
        error( ch );
        return;
    }

    if ( ch->morph == NULL || ch->morph->morph == NULL ) {
        send_to_char( "Why would you need to unshift when you're in your normal body?\r\n", ch );
        return;
    }

    act( AT_LBLUE, "You feel a sharp pain as your body returns to its normal shape!", ch,
         NULL, NULL, TO_CHAR );
    act( AT_LBLUE,
         "$n's body begins to vibrate and shift as it returns to its original state!", ch,
         NULL, NULL, TO_NOTVICT );

    /*
     * Strip off ShapeShift Affects
     */
    affect_strip( ch, gsn_shapeshift );
    xREMOVE_BIT( ch->affected_by, AFF_SHAPESHIFT );
    do_unmorph_char( ch );
    return;
}

//The following skill fury, and the calm command was wrote for
//6dragons mud by: Taon, on 8-1-07. For use with advanced classes.
//More support for fury can be found in both update.c, and fight.c
void do_fury( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    short                   take_move;

    if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You cannot do this in your current state.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_FURY ) ) {
        send_to_char( "You're already very furious!\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_DEFENSIVE_POSTURING ) ) {
        send_to_char( "Not while in such a defensive stance.\r\n", ch );
        return;
    }
    take_move = ch->level / 4 + 1;

    if ( ch->move < take_move ) {
        send_to_char( "You don't have enough energy to go into such a fury.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_fury ) ) {
        WAIT_STATE( ch, skill_table[gsn_fury]->beats );
        send_to_char( "You focus, placing yourself in a furious state of mind.\r\n", ch );
        act( AT_PLAIN, "$n turns red in the face as $s eyes begin to burn with a fury.",
             ch, NULL, NULL, TO_ROOM );
        af.type = gsn_fury;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.duration = -1;
        af.level = ch->level;
        af.bitvector = meb( AFF_FURY );
        affect_to_char( ch, &af );
        ch->move -= take_move;
        learn_from_success( ch, gsn_fury );
    }
    else {
        send_to_char( "You've failed to place yourself in a furious state.\r\n", ch );
        learn_from_failure( ch, gsn_fury );
    }
    return;
}

void do_calm( CHAR_DATA *ch, char *argument )
{

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You cannot do this in your current state.\r\n", ch );
        return;
    }
    if ( !IS_AFFECTED( ch, AFF_FURY ) ) {
        send_to_char( "But you're not in a fury.\r\n", ch );
        return;
    }

    send_to_char( "You slowly bring yourself into a calmer state of mind.\r\n", ch );
    ch->move -= 25;
    affect_strip( ch, gsn_fury );
    xREMOVE_BIT( ch->affected_by, AFF_FURY );
    return;
}

void do_thaitin( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "&YSend the heat of the sun on whom?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_THAITIN ) ) {
        send_to_char( "You sense their blood is already burning.\r\n", ch );
        return;
    }
    if ( ch->blood < 6 ) {
        send_to_char( "You do not have enough blood left to do that.\r\n", ch );
        return;
    }

    ch->blood = ch->blood - 5;

    if ( can_use_skill( ch, number_percent(  ), gsn_thaitin ) ) {
        WAIT_STATE( ch, skill_table[gsn_thaitin]->beats );
        act( AT_PLAIN, "You focus your mind on $N's blood flowing through $s veins.", ch,
             NULL, victim, TO_CHAR );
        act( AT_YELLOW, "You cause $N's blood to burn with a seering heat!", ch, NULL,
             victim, TO_CHAR );
        act( AT_YELLOW, "$n causes your blood to burn with a seering heat!", ch, NULL,
             victim, TO_VICT );
        if ( IS_SECONDCLASS( ch ) )
            victim->degree = 2;
        if ( IS_THIRDCLASS( ch ) )
            victim->degree = 3;
        if ( ch->secondclass == -1 )
            victim->degree = 1;

        af.type = gsn_thaitin;
        af.duration = 50;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_THAITIN );
        affect_to_char( victim, &af );
        learn_from_success( ch, gsn_thaitin );
        global_retcode = damage( ch, victim, nominal, gsn_thaitin );
    }
    else {
        learn_from_failure( ch, gsn_thaitin );
        global_retcode = damage( ch, victim, 0, gsn_thaitin );
        send_to_char( "Your thaitin failed to manipulate your victim's blood.\r\n", ch );
    }
}

void do_surreal_speed( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_SURREAL_SPEED ) ) {
        send_to_char( "You are already using that vampiric skill to its fullest extent.\r\n", ch );
        return;
    }

    if ( ch->blood < 3 ) {
        send_to_char( "You don't have enough blood to use those skills.\r\n", ch );
        return;
    }

    ch->blood = ch->blood - 2;

    WAIT_STATE( ch, skill_table[gsn_surreal_speed]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_surreal_speed )
         && !IS_AFFECTED( ch, AFF_SURREAL_SPEED ) ) {
        act( AT_PLAIN,
             "You begin to utilize your vampiric skills to move faster then reality allows.",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_surreal_speed );
        af.type = gsn_surreal_speed;
        af.duration = ch->level + 50;
        af.location = APPLY_DEX;
        if ( ch->level < 20 ) {
            af.modifier = 1;
        }
        else if ( ch->level >= 20 && ch->level < 40 ) {
            af.modifier = 2;
        }
        else if ( ch->level >= 40 && ch->level < 60 ) {
            af.modifier = 3;
        }
        else if ( ch->level >= 60 ) {
            af.modifier = 4;
        }
        af.bitvector = meb( AFF_SURREAL_SPEED );
        af.level = ch->level;
        affect_to_char( ch, &af );
        af.level = ch->level;
        af.type = gsn_surreal_speed;
        af.duration = ch->level + 50;
        af.location = APPLY_AC;
        if ( ch->level < 20 ) {
            af.modifier = -40;
        }
        else if ( ch->level >= 20 && ch->level < 40 ) {
            af.modifier = -65;
        }
        else if ( ch->level >= 40 && ch->level < 60 ) {
            af.modifier = -80;
        }
        else if ( ch->level >= 60 ) {
            af.modifier = -100;
        }
        af.bitvector = meb( AFF_SURREAL_SPEED );
        affect_to_char( ch, &af );

        return;
    }
    else
        learn_from_failure( ch, gsn_surreal_speed );
    send_to_char
        ( "Your skills are weak yet as a young vampire and you fail to call surreal speed.\r\n",
          ch );

    return;
}

void do_know_enemy( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   nomore;

    nomore = 25;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: know <victim>\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_know_enemy ) ) {
        WAIT_STATE( ch, skill_table[gsn_know_enemy]->beats );
        act( AT_CYAN,
             "You send your kinetic senses outward probing $N\r\nYou attempt to discover $S strengths and weaknesses.\r\n",
             ch, NULL, victim, TO_CHAR );
        ch_printf( ch,
                   "&c%s can roughly withstand %d damage,\r\nand may have close to a %d hitroll and a %d damroll.\r\n",
                   capitalize( victim->name ), victim->max_hit + number_range( 10, 100 ),
                   victim->hitroll + number_range( 1, 5 ), victim->damroll + number_range( 1, 5 ) );
        learn_from_success( ch, gsn_know_enemy );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else
        learn_from_failure( ch, gsn_know_enemy );
    send_to_char( "&cYou fail to discover their weaknesses.\r\n", ch );
    return;
}

void do_gauge( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: gauge <victim>\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "You cannot gauge yourself.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_gauge ) ) {
        WAIT_STATE( ch, skill_table[gsn_gauge]->beats );
        act( AT_PLAIN,
             "You begin to gauge $N's status by telltale signs only another vampire could notice.",
             ch, NULL, victim, TO_CHAR );
        learn_from_success( ch, gsn_gauge );
        if ( victim->Class == CLASS_VAMPIRE ) {
            if ( calculate_age( victim ) < 30 ) {
                send_to_char( "You figure they can't be much older then a Fledgling.\r\n", ch );
            }
            else if ( calculate_age( victim ) >= 30 && calculate_age( victim ) < 100 ) {
                send_to_char( "You figure they can't be much older then a Master.\r\n", ch );
            }
            else if ( calculate_age( victim ) >= 100 && calculate_age( victim ) < 200 ) {
                send_to_char( "You figure they can't be much older then a Lesser Elder.\r\n", ch );
            }

            else if ( calculate_age( victim ) >= 200 && calculate_age( victim ) < 400 ) {
                send_to_char
                    ( "You are surprised to find you are in the presence of a Greater Elder.\r\n",
                      ch );
            }
            else if ( calculate_age( victim ) >= 400 ) {
                send_to_char( "You are suddenly awed as you realize they are an Ancient.\r\n", ch );
            }
        }
        ch_printf( ch,
                   "&c%s can roughly withstand %d damage,\r\nand may have close to a %d hitroll and a %d damroll.\r\n",
                   capitalize( victim->name ), victim->max_hit + number_range( 10, 100 ),
                   victim->hitroll + number_range( 1, 5 ), victim->damroll + number_range( 1, 5 ) );
        return;
    }
    else
        learn_from_failure( ch, gsn_gauge );
    send_to_char
        ( "Your skills are weak yet as a young vampire and you fail to gauge properly.\r\n", ch );

    return;
}

void do_shelter( CHAR_DATA *ch, char *argument )
{
    if ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) ) {
        send_to_char( "You can't use shelter if you are not in mist form.\r\n", ch );
        return;
    }

    if ( ch->blood < 3 ) {
        send_to_char( "You don't have enough blood to use those skills.\r\n", ch );
        return;
    }

    ch->blood = ch->blood - 2;
    WAIT_STATE( ch, skill_table[gsn_shelter]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_shelter ) ) {
        act( AT_PLAIN, "You concentrate on returning home for shelter.", ch, NULL, NULL, TO_CHAR );
        act( AT_BLOOD, "In your mist form you travel unimpeded to your destination.", ch,
             NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_shelter );

        do_recall( ch, ( char * ) "" );
        return;

    }
    else
        learn_from_failure( ch, gsn_shelter );
    send_to_char
        ( "Your skills are weak yet as a young vampire and you fail to find shelter.\r\n", ch );

    return;
}

void do_vampiric_strength( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_VAMPIRIC_STRENGTH ) ) {
        send_to_char( "You are already using that vampiric skill to its fullest extent.\r\n", ch );
        return;
    }

    if ( ch->blood < 3 ) {
        send_to_char( "You don't have enough blood to use those skills.\r\n", ch );
        return;
    }

    ch->blood = ch->blood - 2;

    WAIT_STATE( ch, skill_table[gsn_vampiric_strength]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_vampiric_strength )
         && !IS_AFFECTED( ch, AFF_VAMPIRIC_STRENGTH ) ) {
        act( AT_PLAIN,
             "You begin to utilize your vampiric skills to become stronger then reality allows.",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_vampiric_strength );
        af.type = gsn_vampiric_strength;
        af.duration = ch->level + 50;
        af.location = APPLY_STR;
        af.bitvector = meb( AFF_VAMPIRIC_STRENGTH );
        af.level = ch->level;
        if ( ch->level < 20 ) {
            af.modifier = 1;
        }
        else if ( ch->level >= 20 && ch->level < 40 ) {
            af.modifier = 2;
        }
        else if ( ch->level >= 40 && ch->level < 60 ) {
            af.modifier = 3;
        }
        else if ( ch->level >= 60 ) {
            af.modifier = 4;
        }

        affect_to_char( ch, &af );
        af.type = gsn_vampiric_strength;
        af.duration = ch->level + 50;
        af.location = APPLY_HITROLL;
        af.level = ch->level;
        af.bitvector = meb( AFF_VAMPIRIC_STRENGTH );
        if ( ch->level < 20 ) {
            af.modifier = 1;
        }
        else if ( ch->level >= 20 && ch->level < 40 ) {
            af.modifier = 2;
        }
        else if ( ch->level >= 40 && ch->level < 60 ) {
            af.modifier = 3;
        }
        else if ( ch->level >= 60 ) {
            af.modifier = 4;
        }

        affect_to_char( ch, &af );
        af.type = gsn_vampiric_strength;
        af.duration = ch->level + 50;
        af.location = APPLY_DAMROLL;
        af.level = ch->level;
        af.bitvector = meb( AFF_VAMPIRIC_STRENGTH );
        if ( ch->level < 20 ) {
            af.modifier = 1;
        }
        else if ( ch->level >= 20 && ch->level < 40 ) {
            af.modifier = 2;
        }
        else if ( ch->level >= 40 && ch->level < 60 ) {
            af.modifier = 3;
        }
        else if ( ch->level >= 60 ) {
            af.modifier = 4;
        }
        affect_to_char( ch, &af );

        return;
    }
    else
        learn_from_failure( ch, gsn_vampiric_strength );
    send_to_char
        ( "Your skills are weak yet as a young vampire and you fail to call vampiric strength.\r\n",
          ch );

    return;
}

void do_shrink( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    arg1[MIL];
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );
    one_argument( argument, arg1 );
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: shrink <victim> <size by feet only>.\r\n", ch );
        send_to_char( "Size maybe 1 foot plus an extra foot per 10 levels of caster.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( IS_IMMORTAL( victim ) && victim->level > ch->level ) {
        send_to_char( "It's considered un-wise to do such a thing!\r\n", ch );
        return;
    }
    if ( atoi( arg1 ) > ch->level / 10 ) {
        send_to_char( "You can't shrink someone that much yet.\r\n", ch );
        return;
    }
    if ( atoi( arg1 ) <= 0 ) {
        send_to_char( "What would be the point in that?\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( victim, AFF_SHRINK ) ) {
        send_to_char( "They have already been shrinked in size.\r\n", ch );
        return;
    }

    if ( atoi( arg1 ) * 12 > victim->height ) {
        send_to_char( "You can't shrink them into nothingness.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_shrink )
         && !IS_AFFECTED( victim, AFF_SHRINK ) ) {
        WAIT_STATE( ch, skill_table[gsn_shrink]->beats );
        act( AT_MAGIC,
             "You create a sphere of light around $N that gets smaller while $N shrinks in size.",
             ch, NULL, victim, TO_CHAR );
        act( AT_MAGIC,
             "$n creates a sphere of light around you that gets smaller while you shrink in size.",
             ch, NULL, victim, TO_VICT );
        act( AT_MAGIC,
             "$n creates a sphere of light around $N that gets smaller while $E shrinks in size.",
             ch, NULL, victim, TO_NOTVICT );
        learn_from_success( ch, gsn_shrink );
        af.type = gsn_shrink;
        af.duration = ch->level + 20;
        af.location = APPLY_HEIGHT;
        af.modifier = -atoi( arg1 ) * 12;
        af.bitvector = meb( AFF_SHRINK );
        af.level = ch->level;
        affect_to_char( victim, &af );
        af.type = gsn_shrink;
        af.duration = ch->level + 20;
        af.level = ch->level;
        af.location = APPLY_HITROLL;
        af.modifier = -atoi( arg1 );
        af.bitvector = meb( AFF_SHRINK );
        affect_to_char( victim, &af );
        af.type = gsn_shrink;
        af.duration = ch->level + 20;
        af.level = ch->level;
        af.location = APPLY_DAMROLL;
        af.modifier = -atoi( arg1 );
        af.bitvector = meb( AFF_SHRINK );
        affect_to_char( victim, &af );
        return;
    }
    else {
        learn_from_failure( ch, gsn_shrink );
        send_to_char( "You failed to concentrate enough to cast the spell.\r\n", ch );
        return;
    }

    return;
}

void do_choke( CHAR_DATA *ch, char *argument )
{
    int                     nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];
    int                     chance;
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Who do you want to choke on their own fluids?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why would you want to choke yourself?\r\n", ch );
        return;
    }
    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_choke ) ) {
        WAIT_STATE( ch, skill_table[gsn_choke]->beats );
        act( AT_CYAN, "You focus your mind on causing $N to choke on their own fluids.",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "You are suddenly struggling to breath, as $n causes you choke on your own fluids!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_choke );
        ch->mana = ( ch->mana - nomore );
        global_retcode = damage( ch, victim, mediumhigh, gsn_choke );
        chance = number_range( 1, 100 );
        if ( chance < 96 ) {
            return;
        }
        if ( !char_died( victim ) && chance > 95 ) {
            if ( IS_AFFECTED( victim, AFF_POISON ) )
                return;
            act( AT_GREEN, "Your choke has poisoned $N!", ch, NULL, victim, TO_CHAR );
            act( AT_GREEN, "$n's choke attack has poisoned you!", ch, NULL, victim, TO_VICT );
            af.type = gsn_choke;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = -2;
            if ( IS_SECONDCLASS( ch ) ) {
                victim->degree = 2;
            }
            if ( IS_THIRDCLASS( ch ) ) {
                victim->degree = 3;
            }
            if ( ch->secondclass == -1 ) {
                victim->degree = 1;
            }

            af.bitvector = meb( AFF_POISON );
            affect_join( victim, &af );
            set_char_color( AT_GREEN, victim );
            send_to_char( "You feel very sick.\r\n", victim );
        }
        else {
            learn_from_failure( ch, gsn_choke );
            global_retcode = damage( ch, victim, 0, gsn_choke );
            send_to_char( "&cYou were unable to penetrate their mental defenses.\r\n", ch );
        }
        return;

    }
}

void do_whip_of_murazor( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    AFFECT_DATA             af;
    int                     chance;
    short                   nomore;

    nomore = 10;

    argument = one_argument( argument, arg );

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' && IS_AFFECTED( ch, AFF_MURAZOR ) ) {
        send_to_char( "Who are you trying to whip?\r\n", ch );
        return;
    }
    victim = get_char_room( ch, arg );

    if ( IS_AFFECTED( ch, AFF_MURAZOR ) && ( !victim ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( !IS_AFFECTED( ch, AFF_MURAZOR ) && arg[0] == '\0' ) {
        send_to_char
            ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
              ch );
        act( AT_CYAN, "You scream as two of your tentacles grow outward from your head!",
             ch, NULL, NULL, TO_CHAR );
        act( AT_RED,
             "$n screams, and you see two of their tentacles growing in length from their squid head.",
             ch, NULL, NULL, TO_ROOM );
        af.type = gsn_whip_of_murazor;
        af.duration = ch->level + 20;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_MURAZOR );
        affect_to_char( ch, &af );
        global_retcode = damage( ch, ch, number_chance( 1, 4 ), gsn_whip_of_murazor );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why would you want to whip yourself?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_whip_of_murazor )
         && ( IS_AFFECTED( ch, AFF_MURAZOR ) ) ) {
        WAIT_STATE( ch, skill_table[gsn_whip_of_murazor]->beats );
        act( AT_CYAN, "You whip your tentacles at $N.", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n whips their tentacles at you!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_whip_of_murazor );
        ch->mana = ( ch->mana - nomore );
        global_retcode = damage( ch, victim, medium, gsn_whip_of_murazor ); /* nerfed
                                                                             * spike some 
                                                                             * - Vladaar 
                                                                             */
        chance = number_range( 1, 100 );
        if ( !char_died( victim ) && chance > 95 ) {
            if ( IS_AFFECTED( victim, AFF_POISON ) )
                return;
            act( AT_GREEN, "Your tentacles blow has poisoned $N!", ch, NULL, victim, TO_CHAR );
            act( AT_GREEN, "$n's whip attack has poisoned you!", ch, NULL, victim, TO_VICT );
            af.type = gsn_whip_of_murazor;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = -2;
            if ( IS_SECONDCLASS( ch ) ) {
                victim->degree = 2;
            }
            if ( IS_THIRDCLASS( ch ) ) {
                victim->degree = 3;
            }
            if ( ch->secondclass == -1 ) {
                victim->degree = 1;
            }

            af.bitvector = meb( AFF_POISON );
            affect_join( victim, &af );
            set_char_color( AT_GREEN, victim );
            send_to_char( "You feel very sick.\r\n", victim );
        }
    }
    else if ( !IS_AFFECTED( ch, AFF_MURAZOR ) ) {
        send_to_char( "Your tentacles aren't extended.\r\n", ch );
        return;
    }
    else {
        learn_from_failure( ch, gsn_whip_of_murazor );
        global_retcode = damage( ch, victim, 0, gsn_whip_of_murazor );
        send_to_char( "They managed to dodge your tentacles.\r\n", ch );
        return;
    }
    return;
}

//Knock off version of Prayer.
void do_desecrate( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    AFFECT_DATA             af;

    one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return;
    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: Desecrate start\r\nSyntax: Desecrate stop\r\n", ch );
        return;
    }
    if ( !IS_AWAKE( ch ) ) {
        send_to_char( "You cant desecrate an area while asleep.", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_PRAYER ) && !str_cmp( arg, "start" ) ) {
        send_to_char( "But you're already desecrating the area.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "start" ) ) {
        if ( can_use_skill( ch, number_percent(  ), gsn_desecrate ) ) {
            if ( ch->position == POS_FIGHTING ) {
                send_to_char( "You cannot start desecrating during combat.\r\n", ch );
                return;
            }
            send_to_char
                ( "You suddenly run around crazily yelling a strange chant, cursing, and defiling the area.\r\n",
                  ch );
            af.type = gsn_desecrate;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.level = ch->level;
            af.duration = -1;
            af.bitvector = meb( AFF_PRAYER );
            affect_to_char( ch, &af );
            learn_from_success( ch, gsn_desecrate );
            return;
        }
        else {
            send_to_char
                ( "You can't seem to concentrate enough to properly desecrate the area.\r\n", ch );
            learn_from_failure( ch, gsn_desecrate );
            return;
        }
    }
    if ( !str_cmp( arg, "stop" ) ) {
        if ( !IS_AFFECTED( ch, AFF_PRAYER ) ) {
            send_to_char( "But you're not desecrating the area.\r\n", ch );
            return;
        }

        if ( ch->position != POS_STANDING )
            set_position( ch, POS_STANDING );

        send_to_char
            ( "You come to an end of your chanting, and defiling the area, and appear composed once again.\r\n",
              ch );
        affect_strip( ch, gsn_desecrate );
        xREMOVE_BIT( ch->affected_by, AFF_PRAYER );
        return;

    }
    return;
}

void do_brittle_bone( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    char                    arg[MIL];

    argument = one_argument( argument, arg );
    // Modified to find target if ch is fighting and can't see them. -Taon
    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Syntax: brittle <victim>\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_BRITTLE_BONES ) ) {
        send_to_char( "They already are affected by brittle bones.\r\n", ch );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_brittle_bone ) ) {
        WAIT_STATE( ch, skill_table[gsn_brittle_bone]->beats );
        act( AT_MAGIC,
             "You create a bone virus within $N by uttering the word, 'Bkytle Boens'.", ch,
             NULL, victim, TO_CHAR );
        act( AT_MAGIC, "$n uttters the word, 'Bkytle Boens'.", ch, NULL, NULL, TO_ROOM );
        act( AT_WHITE,
             "You suddenly don't feel stable, and your bones make popping sounds.", ch,
             NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_brittle_bone );

        af.type = gsn_brittle_bone;
        af.duration = ch->level / 2;
        af.level = ch->level;
        af.location = APPLY_MOVE;
        af.modifier = -50;
        af.bitvector = meb( AFF_BRITTLE_BONES );
        affect_to_char( victim, &af );
        if ( IS_SECONDCLASS( ch ) ) {
            victim->degree = 2;
        }
        if ( IS_THIRDCLASS( ch ) ) {
            victim->degree = 3;
        }
        if ( ch->secondclass == -1 ) {
            victim->degree = 1;
        }

        set_char_color( AT_RED, victim );
        send_to_char( "Your bones suddenly throb in pain!", victim );
        global_retcode =
            damage( ch, victim, ( get_curr_dex( ch ) + number_chance( 10, 20 ) ),
                    gsn_brittle_bone );
    }
    else {
        learn_from_failure( ch, gsn_brittle_bone );
        act( AT_CYAN, "You fail to pronounce your word of power correctly.", ch, NULL,
             victim, TO_CHAR );
        global_retcode = damage( ch, victim, 0, gsn_brittle_bone );
        return;
    }
    return;
}

void do_festering_wound( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    char                    arg[MIL];

    argument = one_argument( argument, arg );
    // Modified to find target if ch is fighting and can't see them. -Taon
    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Syntax: fester <victim>\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_FUNGAL_TOXIN ) ) {
        send_to_char( "They already have the festering wound.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_festering_wound ) ) {
        WAIT_STATE( ch, skill_table[gsn_festering_wound]->beats );
        act( AT_CYAN, "You quickly dip your blade in a small black bag of fungus powder.",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n quickly sticks their weapon inside a small black bag.", ch, NULL,
             NULL, TO_ROOM );
        act( AT_DGREY, "$n's blade rends you!", ch, NULL, victim, TO_VICT );
        act( AT_ORANGE, "Your wound immediately begins to fester!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_festering_wound );

        af.type = gsn_festering_wound;
        af.duration = ch->level / 2;
        af.level = ch->level;
        af.location = APPLY_STR;
        af.modifier = -2;
        af.bitvector = meb( AFF_FUNGAL_TOXIN );
        affect_to_char( victim, &af );
        if ( IS_SECONDCLASS( ch ) )
            victim->degree = 2;
        if ( IS_THIRDCLASS( ch ) )
            victim->degree = 3;
        if ( ch->secondclass == -1 )
            victim->degree = 1;
        set_char_color( AT_RED, victim );
        send_to_char( "Your festering wound burns!", victim );
        global_retcode =
            damage( ch, victim, ( get_curr_dex( ch ) + number_chance( 10, 20 ) ),
                    gsn_festering_wound );
    }
    else {
        learn_from_failure( ch, gsn_festering_wound );
        act( AT_CYAN,
             "You fumble trying to dip your blade in a small black bag of fungus powder.",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n fumbles trying to stick their weapon inside a small black bag.",
             ch, NULL, NULL, TO_ROOM );
        global_retcode = damage( ch, victim, 0, gsn_festering_wound );
        return;
    }
    return;
}

void do_spike( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    AFFECT_DATA             af;
    int                     chance;

    argument = one_argument( argument, arg );
    // Modified to find target if ch is fighting and can't see them. -Taon
    if ( ( arg[0] == '\0' ) && IS_AFFECTED( ch, AFF_SPIKE ) ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
    }
    else if ( ( arg[0] != '\0' ) && IS_AFFECTED( ch, AFF_SPIKE ) )
        if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }

    if ( !IS_AFFECTED( ch, AFF_SPIKE ) && arg[0] == '\0' ) {
        act( AT_RED,
             "You howl with rage and pain as large spikes grow outward from your knuckles.",
             ch, NULL, NULL, TO_CHAR );
        act( AT_RED, "$n screams, and you notice large spikes growing out of $s hands.",
             ch, NULL, NULL, TO_ROOM );
        af.type = gsn_spike;
        af.duration = ch->level + 20;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_SPIKE );
        affect_to_char( ch, &af );
        global_retcode = damage( ch, ch, number_chance( 1, 4 ), gsn_spike );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why would you want to spike yourself?\r\n", ch );
        return;
    }

    if ( ( arg[0] == '\0' ) && !ch->fighting ) {
        send_to_char( "spike who?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_spike )
         && ( IS_AFFECTED( ch, AFF_SPIKE ) ) ) {
        WAIT_STATE( ch, skill_table[gsn_spike]->beats );
        act( AT_RED, "You thrust your spiked fist at $N.", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n thrusts a spiked fist into you!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_spike );
        global_retcode = damage( ch, victim, high, gsn_spike ); /* nerfed spike some -
                                                                 * Vladaar */
        chance = number_range( 1, 100 );
        if ( !char_died( victim ) && chance > 95 ) {
            if ( IS_AFFECTED( victim, AFF_POISON ) )
                return;
            act( AT_GREEN, "Your spiked blow has poisoned $N!", ch, NULL, victim, TO_CHAR );
            act( AT_GREEN, "$n's unholy spiked attack has poisoned you!", ch, NULL, victim,
                 TO_VICT );
            af.type = gsn_spike;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = -2;
            af.bitvector = meb( AFF_POISON );
            affect_join( victim, &af );
            set_char_color( AT_GREEN, victim );
            victim->degree = 1;
            send_to_char( "You feel very sick.\r\n", victim );
        }
    }
    else if ( !IS_AFFECTED( ch, AFF_SPIKE ) ) {
        send_to_char( "Your spikes aren't extended.\r\n", ch );
        return;
    }
    else {
        learn_from_failure( ch, gsn_spike );
        global_retcode = damage( ch, victim, 0, gsn_spike );
        send_to_char( "They managed to dodge your spiked fist.\r\n", ch );
        return;
    }
    return;
}

void do_thicken_skin( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 10;
    set_char_color( AT_MAGIC, ch );

    if ( IS_AFFECTED( ch, AFF_THICKEN_SKIN ) ) {
        send_to_char( "Your skin is already as thick as can be.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }
    set_char_color( AT_BLOOD, ch );
    send_to_char( "A rush of pain throbs in your head, as you concentrate on your task.\r\n", ch );
    set_char_color( AT_MAGIC, ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_thicken_skin ) ) {
        WAIT_STATE( ch, skill_table[gsn_thicken_skin]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "You concentrate on making your skin thicker, and more dense.\r\n",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_thicken_skin );

        af.type = gsn_thicken_skin;
        af.duration = ch->level * 4;
        af.level = ch->level;
        af.location = APPLY_AC;
        af.modifier = -10 + ( -1 * ch->level * 2 );
        af.bitvector = meb( AFF_THICKEN_SKIN );
        affect_to_char( ch, &af );

    }
    else {
        learn_from_failure( ch, gsn_thicken_skin );
        send_to_char( "&cYou failed to thicken your skin.\r\n", ch );
    }
    return;
}

void do_chameleon( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 10;
    if ( IS_AFFECTED( ch, AFF_HIDE ) ) {
        send_to_char( "You are already hiding.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_chameleon ) ) {
        WAIT_STATE( ch, skill_table[gsn_chameleon]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "You concentrate on blending in with your surroundings.\r\n", ch,
             NULL, NULL, TO_CHAR );
        act( AT_CYAN, "Your skin takes the color and texture of what is around you!\r\n",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_chameleon );

        af.type = gsn_chameleon;
        af.duration = ch->level * 4;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_HIDE );
        affect_to_char( ch, &af );
    }
    else {
        learn_from_failure( ch, gsn_chameleon );
        send_to_char( "&cYou failed to concentrate enough on your surroundings.\r\n", ch );
    }
}

void do_lore( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    short                   nomore;
    char                    arg[MIL];

    nomore = 12;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "What are you trying to remember the lore on?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( !IS_NPC( ch ) && ch->fighting ) {
        send_to_char( "You cant accomplish such a task while fighting.\r\n", ch );
        return;
    }

    if ( !( obj = get_obj_carry( ch, arg ) ) ) {
        send_to_char( "You do not have that item.\r\n", ch );
        return;
    }

    if ( !VLD_STR( arg ) ) {
        send_to_char( "What are you trying to remember the lore on?\r\n", ch );
        return;
    }

    send_to_char( "\r\n\r\n&cYou rack your brain trying to remember the lore.\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_lore )
         && ( ( obj = get_obj_carry( ch, arg ) ) != NULL ) ) {
        WAIT_STATE( ch, skill_table[gsn_lore]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN,
             "You hold the object, and remember a bard story that tells of the object being...\r\n",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_lore );
        act( AT_CYAN, "$n holds $p and closes $s eyes.", ch, obj, NULL, TO_ROOM );
        identify_object( ch, obj );
        return;
    }
    else {
        learn_from_failure( ch, gsn_lore );
        ch_printf( ch, "&cYou failed to remember the lore on %s!\r\n", arg );
        return;
    }
    return;
}

void do_object_reading( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    short                   nomore;
    char                    arg[MIL];

    nomore = 12;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "What are you trying to use ESP on?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( !IS_NPC( ch ) && ch->fighting ) {
        send_to_char( "You cant accomplish such a task while fighting.\r\n", ch );
        return;
    }

    if ( !( obj = get_obj_carry( ch, arg ) ) ) {
        send_to_char( "You do not have that item.\r\n", ch );
        return;
    }

    if ( !VLD_STR( arg ) ) {
        send_to_char( "What object do I want to attempt ESP with?\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_object_reading )
         && ( ( obj = get_obj_carry( ch, arg ) ) != NULL ) ) {
        WAIT_STATE( ch, skill_table[gsn_object_reading]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN,
             "You hold the object, and feel with your extra-sensory perception.\r\n", ch,
             NULL, NULL, TO_CHAR );
        act( AT_CYAN, "A flood of information, suddenly comes to your mind!\r\n", ch, NULL,
             NULL, TO_CHAR );
        learn_from_success( ch, gsn_object_reading );
        act( AT_PLAIN, "$n holds $p and closes $s eyes.", ch, obj, NULL, TO_ROOM );
        identify_object( ch, obj );
        return;
    }
    else {
        learn_from_failure( ch, gsn_object_reading );
        ch_printf( ch, "&cYou failed to sense anything from %s!\r\n", arg );
        return;
    }
    return;
}

void do_drowsy( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    nomore = 40;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Who are you trying to make sleepy?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( !victim ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_drowsy ) ) {
        WAIT_STATE( ch, skill_table[gsn_drowsy]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN,
             "You begin to send a tremendous mental suggestion of falling asleep to $N's mind.\r\n",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "You are overcome with a need to sleep, and fall to sleep.\r\n", ch,
             NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_drowsy );
        set_position( victim, POS_SLEEPING );
    }
    else {
        learn_from_failure( ch, gsn_drowsy );
        send_to_char( "&cYou failed to peneterate their mental defenses.\r\n", ch );
    }
    return;
}

void do_fear( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    nomore = 40;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Who are you trying to send their worst fears too?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( !victim ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why would you want to delve your own fears?\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_fear ) ) {
        WAIT_STATE( ch, skill_table[gsn_fear]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN,
             "You begin to mentally probe $N's mind to discover their worst fears.\r\n",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "Having discovered $N's worst fears you send the images to them so they believe it has happened.\r\n",
             ch, NULL, victim, TO_CHAR );
        learn_from_success( ch, gsn_fear );
        do_flee( victim, ( char * ) "" );
        if ( IS_NPC( victim ) )
            start_fearing( victim, ch );
        stop_hating( victim );
        stop_hunting( victim );
    }
    else {
        learn_from_failure( ch, gsn_fear );
        send_to_char( "&cYou failed to find your enemies worst fears.\r\n", ch );
    }
    return;
}

void do_sustenance( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_sustenance ) ) {
        WAIT_STATE( ch, skill_table[gsn_sustenance]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN,
             "You cannibalize your kinetic power to fulfill your hunger and thirst.\r\n",
             ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "Your mental energies provide sustenance for your body.\r\n", ch,
             NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_sustenance );
        ch->pcdata->condition[COND_THIRST] = 40;
        ch->pcdata->condition[COND_FULL] = 40;
    }
    else {
        learn_from_failure( ch, gsn_sustenance );
        send_to_char( "&cYou failed to cannibalize your kinetic energy to sustenance.\r\n", ch );
    }
    return;
}

void do_kinetic_barrier( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    int                     nomore;

    nomore = 100;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    // Bring some balance to this skill -Taon
    if ( !IS_NPC( ch ) && ch->fighting ) {
        send_to_char( "You cant accomplish such a task while fighting.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_KINETIC ) ) {
        send_to_char( "They have already have a kinetic barrier.\r\n", ch );
        return;
    }

    send_to_char( "\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
                  ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_kinetic_barrier ) ) {
        WAIT_STATE( ch, skill_table[gsn_kinetic_barrier]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "You concentrate on forcing your mental energies to expand out of your body.",
             ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "Your mental energies form a kinetic barrier around your body!", ch, NULL,
             NULL, TO_CHAR );
        learn_from_success( ch, gsn_kinetic_barrier );

        af.type = gsn_kinetic_barrier;
        af.duration = ch->level;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_KINETIC );
        affect_to_char( ch, &af );
        ch->kinetic_dam = 100 + ( ch->level * 9 );
    }
    else {
        learn_from_failure( ch, gsn_kinetic_barrier );
        send_to_char( "&cYou failed to control your kinetic barrier, and it has dispersed.\r\n",
                      ch );
    }
}

void do_graft_weapon( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    OBJ_DATA               *obj;

    nomore = 20;
    char                    arg[MIL];
    AFFECT_DATA             af;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_GRAFT ) ) {
        send_to_char
            ( "&cYou concentrate on separating your hand from your weapon, and the graft releases.\r\n",
              ch );
        affect_strip( ch, gsn_graft_weapon );
        xREMOVE_BIT( ch->affected_by, AFF_GRAFT );
        affect_strip( ch, gsn_graft_weapon );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "What are you trying to graft?\r\n", ch );
        return;
    }
    if ( ch->fighting ) {
        send_to_char( "While you're fighting?  Nice try.\r\n", ch );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;

    if ( !( obj = get_obj_carry( ch, arg ) ) ) {
        send_to_char( "You do not have that weapon.\r\n", ch );
        return;
    }
    if ( obj->item_type != ITEM_WEAPON ) {
        send_to_char( "That item is not a weapon.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );

    if ( can_use_skill( ch, number_percent(  ), gsn_graft_weapon ) ) {
        WAIT_STATE( ch, skill_table[gsn_graft_weapon]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "You concentrate on joining your hand and your weapon as one.\r\n",
             ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN,
             "Your skin, and the weapon becomes hot, and graft together as one!\r\n", ch,
             NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_graft_weapon );
        equip_char( ch, obj, WEAR_WIELD );
        af.type = gsn_graft_weapon;
        af.duration = -1;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_GRAFT );
        affect_to_char( ch, &af );

        af.type = gsn_graft_weapon;
        af.duration = -1;
        af.level = ch->level;
        af.location = APPLY_HITROLL;
        af.modifier = 5 + ch->level / 20;
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );
    }
    else {
        learn_from_failure( ch, gsn_graft_weapon );
        send_to_char( "&cYou cannot seem to concentrate enough on the weapon.\r\n", ch );
    }
    return;
}

void do_sound_waves( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    SKILLTYPE              *skill = NULL;
    AFFECT_DATA             af;

    if ( skill_table[gsn_sound_waves]->somatic == 1 ) {
        send_to_char( "You must use the play skill with a proper instrument to do that.\r\n", ch );
        return;
    }
    if ( !CAN_PLAY( ch, skill_table[gsn_sound_waves]->value ) )
        return;

    if ( can_use_skill( ch, number_percent(  ), gsn_sound_waves ) ) {
        skill_table[gsn_sound_waves]->somatic = 1;
        send_to_char
            ( "&cWhile playing your instrument you start to sing at an extreme decibal level.&w\r\n",
              ch );
        WAIT_STATE( ch, skill_table[gsn_sound_waves]->beats );
        act( AT_ORANGE, "Your deafening music befuddles tasks that require concentration.",
             ch, NULL, NULL, TO_CHAR );
        act( AT_ORANGE,
             "$n's deafening music befuddles any tasks that require concentration.", ch,
             NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_sound_waves );

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                continue;

            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && !vch->fighting )
                continue;
            if ( IS_NPC( vch ) && !vch->fighting )
                continue;

            send_to_char
                ( "&O\r\nYou swear your ears are bleeding from the sound waves!&w\r\n", vch );
            affect_strip( vch, gsn_sound_waves );
            af.type = gsn_sound_waves;
            af.duration = ch->level + 10;
            af.level = ch->level;
            af.location = APPLY_AFFECT;
            af.modifier = 0;
            af.bitvector = meb( AFF_SOUND_WAVES );
            affect_to_char( vch, &af );
        }
        return;
    }
    else {
        learn_from_failure( ch, gsn_sound_waves );
        send_to_char( "&cYou forget some of the words and the song loses its effect.&w\r\n", ch );
    }

    return;
}

void do_unholy_melody( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    SKILLTYPE              *skill = NULL;
    AFFECT_DATA             af;

    if ( skill_table[gsn_unholy_melody]->somatic == 1 ) {
        send_to_char( "You must use the play skill with a proper instrument to do that.\r\n", ch );
        return;
    }
    if ( !CAN_PLAY( ch, skill_table[gsn_unholy_melody]->value ) )
        return;

    if ( can_use_skill( ch, number_percent(  ), gsn_unholy_melody ) ) {
        skill_table[gsn_unholy_melody]->somatic = 1;
        send_to_char
            ( "&cWhile playing your unholy melody you start to sing a foreboding song.&w\r\n", ch );
        WAIT_STATE( ch, skill_table[gsn_unholy_melody]->beats );
        act( AT_ORANGE, "Your music invokes dark powers.", ch, NULL, NULL, TO_CHAR );
        act( AT_ORANGE, "$n's music invokes dark powers.", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_unholy_melody );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && !vch->fighting )
                continue;
            if ( IS_NPC( vch ) && !vch->fighting )
                continue;

            send_to_char( "&O\r\nYou suffer an unholy affliction from the melody!&w\r\n", vch );
            affect_strip( vch, gsn_unholy_melody );
            af.type = gsn_unholy_melody;
            af.duration = ch->level + 10;
            af.level = ch->level;
            af.location = APPLY_AFFECT;
            af.modifier = 0;
            af.bitvector = meb( AFF_FUNGAL_TOXIN );
            affect_to_char( vch, &af );
        }
        return;
    }
    else {
        learn_from_failure( ch, gsn_unholy_melody );
        send_to_char( "&cYou forget some of the words and the song loses its effect.&w\r\n", ch );
    }

    return;
}

void do_ottos_dance( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    SKILLTYPE              *skill = NULL;
    AFFECT_DATA             af;

    if ( skill_table[gsn_ottos_dance]->somatic == 1 ) {
        send_to_char( "You must use the play skill with a proper instrument to do that.\r\n", ch );
        return;
    }

    if ( !CAN_PLAY( ch, skill_table[gsn_ottos_dance]->value ) )
        return;

    if ( can_use_skill( ch, number_percent(  ), gsn_ottos_dance ) ) {
        skill_table[gsn_ottos_dance]->somatic = 1;
        send_to_char( "&cWhile playing your melody you start to sing an lively song.&w\r\n", ch );
        WAIT_STATE( ch, skill_table[gsn_ottos_dance]->beats );
        act( AT_ORANGE, "Your music invokes arcane powers urging listeners to dance.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_ORANGE, "$n's music invokes primal urges to dance.", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_ottos_dance );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && !vch->fighting )
                continue;

            send_to_char( "&O\r\nYou begin dancing uncontrollably!&w\r\n", vch );
            affect_strip( vch, gsn_ottos_dance );
            af.type = gsn_ottos_dance;
            af.duration = ch->level + 10;
            af.level = ch->level;
            af.location = APPLY_AFFECT;
            af.modifier = 0;
            af.bitvector = meb( AFF_OTTOS_DANCE );
            affect_to_char( vch, &af );
        }
        return;
    }
    else {
        learn_from_failure( ch, gsn_ottos_dance );
        send_to_char( "&cYou forget some of the words and the song loses its effect.&w\r\n", ch );
    }

    return;
}

void do_evac_crescendo( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    char                    arg[MIL];
    SKILLTYPE              *skill = NULL;

    if ( skill_table[gsn_evac_crescendo]->somatic == 1 ) {
        send_to_char( "You must use the play skill with a proper instrument to do that.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( !CAN_PLAY( ch, skill_table[gsn_evac_crescendo]->value ) )
        return;

    if ( can_use_skill( ch, number_percent(  ), gsn_evac_crescendo ) ) {
        skill_table[gsn_evac_crescendo]->somatic = 1;
        send_to_char
            ( "&cWhile playing your melody you start to sing an ancient song from your homeland.&w\r\n",
              ch );
        WAIT_STATE( ch, skill_table[gsn_evac_crescendo]->beats );
        act( AT_ORANGE, "Your music invokes arcane powers long forgotten.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_ORANGE, "$n's music invokes images and feelings of your homeland.", ch,
             NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_evac_crescendo );

        if ( can_use_skill( ch, number_percent(  ), gsn_vocals )
             && number_range( 1, 3 ) == 1 )
            learn_from_success( ch, gsn_vocals );

        if ( !str_cmp( arg, "all" ) ) {
            for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
                vch_next = vch->next_in_room;

                if ( !is_same_group( vch, ch ) )
                    continue;

                send_to_char
                    ( "&O\r\nYou feel the world begin to spin, and it stops suddenly at a new location!!&w\r\n",
                      vch );
                do_recall( vch, ( char * ) "" );
            }
        }
        else {
            send_to_char
                ( "&O\r\nYou feel the world begin to spin, and it stops suddenly at a new location!!&w\r\n",
                  ch );
            do_recall( ch, ( char * ) "" );
        }
    }
    else {
        learn_from_failure( ch, gsn_evac_crescendo );
        send_to_char( "&cYou forget some of the words and the song loses its effect.&w\r\n", ch );
    }

    return;
}

void do_stirring_ballad( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    SKILLTYPE              *skill = NULL;

    nomore = 15;

    if ( skill_table[gsn_stirring_ballad]->somatic == 1 ) {
        send_to_char( "You must use the play skill with a proper instrument to do that.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You are not able to achieve the correct pitch for the song to work.\r\n",
                      ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_stirring_ballad ) ) {
        skill_table[gsn_stirring_ballad]->somatic = 1;
        send_to_char
            ( "\r\n&cWhile playing your melody you start to sing a stirring ballad of ancient times involving true love.\r\n",
              ch );
        WAIT_STATE( ch, skill_table[gsn_stirring_ballad]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_ORANGE, "You sing a stirring ballad, invoking arcane power once forgotten!", ch,
             NULL, NULL, TO_CHAR );
        act( AT_ORANGE, "$n sings a stirring ballad of ancient times involving true love!", ch,
             NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_stirring_ballad );

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !is_same_group( vch, ch ) )
                continue;
            affect_strip( vch, gsn_stirring_ballad );
            af.type = gsn_stirring_ballad;
            af.duration = ch->level * 4;
            af.level = ch->level;
            af.location = APPLY_AFFECT;
            af.modifier = 0;
            af.bitvector = meb( AFF_STIRRING );
            affect_to_char( vch, &af );
            send_to_char( "&OYou feel electrified as healing energies fill your body!!\r\n", vch );
        }
    }
    else {
        learn_from_failure( ch, gsn_stirring_ballad );
        send_to_char( "&cYou forgot some of the words, and the ballad has lost it's effect.\r\n",
                      ch );
    }
}

void do_harmonic_melody( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;

    nomore = 15;

    if ( ch->mana < nomore ) {
        send_to_char( "You are not able to achieve the correct pitch for the song to work.\r\n",
                      ch );
        return;
    }

    affect_strip( ch, gsn_harmonic_melody );

    if ( can_use_skill( ch, number_percent(  ), gsn_harmonic_melody ) ) {
        send_to_char( "\r\n&cYou begin to sing a harmonic melody of an ancient battle.\r\n", ch );
        WAIT_STATE( ch, skill_table[gsn_harmonic_melody]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_ORANGE, "You sing a tale of few who stood up to many!", ch, NULL, NULL, TO_CHAR );
        act( AT_ORANGE, "$n begins singing a song of ancient battle where few stood up to many!",
             ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_harmonic_melody );
        learn_from_success( ch, gsn_vocals );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !is_same_group( vch, ch ) )
                continue;
            affect_strip( vch, gsn_harmonic_melody );

            af.type = gsn_harmonic_melody;
            af.duration = ch->level * 4;
            af.level = ch->level;
            af.location = APPLY_HITROLL;
            af.modifier = 1 + ch->level / 20;
            xCLEAR_BITS( af.bitvector );
            affect_to_char( vch, &af );

            af.type = gsn_harmonic_melody;
            af.duration = ch->level * 4;
            af.level = ch->level;
            af.location = APPLY_DAMROLL;
            af.modifier = 1 + ch->level / 20;
            xCLEAR_BITS( af.bitvector );
            affect_to_char( vch, &af );

            send_to_char( "&OYou feel rage and pent up energy seep into your body!!\r\n", vch );
        }
    }
    else {
        learn_from_failure( ch, gsn_harmonic_melody );
        send_to_char( "&cYou forgot some of the words, and the song lost it's effect.\r\n", ch );
    }
}

void do_druidic_hymn( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    SKILLTYPE              *skill = NULL;

    affect_strip( ch, gsn_druidic_hymn );

    if ( skill_table[gsn_druidic_hymn]->somatic == 1 ) {
        send_to_char( "You must use the play skill with a proper instrument to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_druidic_hymn ) ) {
        skill_table[gsn_druidic_hymn]->somatic = 1;
        send_to_char
            ( "\r\n&cWhile playing your melody you start to sing a song of a ancient druid that lived like a bear.\r\n",
              ch );
        WAIT_STATE( ch, skill_table[gsn_druidic_hymn]->beats );
        act( AT_ORANGE, "Singing with conviction, you invoke arcane words!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_ORANGE, "$n sings the old song of the druid who lived like a bear!", ch, NULL, NULL,
             TO_ROOM );
        learn_from_success( ch, gsn_druidic_hymn );

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !is_same_group( vch, ch ) )
                continue;
            affect_strip( vch, gsn_druidic_hymn );

            af.type = gsn_druidic_hymn;
            af.duration = ch->level + 40;
            af.level = ch->level;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = meb( AFF_REACTIVE );
            affect_to_char( vch, &af );

            send_to_char( "&OYou feel the regenerating druidic powers take hold of you!!\r\n",
                          vch );
        }
    }
    else {
        learn_from_failure( ch, gsn_druidic_hymn );
        send_to_char( "&cYou forgot some of the words, and the tale lost it's effect.\r\n", ch );
    }
}

void do_play( CHAR_DATA *ch, char *argument )
{
    short                   chance;
    char                    arg[MIL];
    SKILLTYPE              *skill = NULL;

    argument = one_argument( argument, arg );

    chance = number_range( 1, 4 );

    OBJ_DATA               *obj = get_eq_char( ch, WEAR_HOLD );

    if ( obj == NULL || obj->item_type != ITEM_INSTRUMENT ) {
        send_to_char( "You are not holding an instrument.\r\n", ch );
        obj == NULL;
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "Available songs to play are:\r\n", ch );
        send_to_char( "Evac Crescendo\t", ch );
        send_to_char( "Druidic Hymn\t", ch );
        send_to_char( "Ottos Dance\t", ch );
        send_to_char( "Stirring Ballad\r\n", ch );
        send_to_char( "Unholy Melody", ch );
        send_to_char( "   Sound Waves", ch );
        send_to_char( "\r\n", ch );
        return;
    }
    if ( str_cmp( arg, "evac crescendo" ) && str_cmp( arg, "evac" )
         && str_cmp( arg, "stirring ballad" ) && str_cmp( arg, "stirring" )
         && str_cmp( arg, "druidic hymn" ) && str_cmp( arg, "druidic" )
         && str_cmp( arg, "ottos dance" ) && str_cmp( arg, "ottos" )
         && str_cmp( arg, "unholy melody" ) && str_cmp( arg, "unholy" )
         && str_cmp( arg, "sound waves" ) && str_cmp( arg, "sound" ) ) {
        send_to_char( "That is not a valid song you can play.\r\n", ch );
        send_to_char( "Available songs to play are:\r\n", ch );
        send_to_char( "Evac Crescendo\t", ch );
        send_to_char( "Druidic Hymn\t", ch );
        send_to_char( "Ottos Dance\t", ch );
        send_to_char( "Stirring Ballad\r\n", ch );
        send_to_char( "Unholy Melody", ch );
        send_to_char( "   Sound Waves", ch );
        send_to_char( "\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_play ) ) {
        if ( chance < 3 && ch->pcdata->learned[gsn_play] <= 60 ) {
            act( AT_ORANGE,
                 "You begin to play a melody, but stop as you don't remember all the notes.",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_ORANGE,
                 "$n begins to play a melody, but stops suddenly forgetting all the notes.",
                 ch, NULL, NULL, TO_ROOM );
            return;
        }
        act( AT_ORANGE, "You begin to play a melody.", ch, NULL, NULL, TO_CHAR );
        act( AT_ORANGE, "$n begins to play a melody.", ch, NULL, NULL, TO_ROOM );
        if ( !str_cmp( arg, "druidic hymn" ) || !str_cmp( arg, "druidic" ) ) {
            if ( skill_table[gsn_druidic_hymn]->somatic != 0 ) {
                skill_table[gsn_druidic_hymn]->somatic = 0;
            }
            do_druidic_hymn( ch, ( char * ) "" );
        }
        else if ( !str_cmp( arg, "stirring ballad" ) || !str_cmp( arg, "stirring" ) ) {
            if ( skill_table[gsn_stirring_ballad]->somatic != 0 ) {
                skill_table[gsn_stirring_ballad]->somatic = 0;
            }
            do_stirring_ballad( ch, ( char * ) "" );
        }
        else if ( !str_cmp( arg, "evac crescendo" ) || !str_cmp( arg, "evac" ) ) {
            if ( skill_table[gsn_evac_crescendo]->somatic != 0 ) {
                skill_table[gsn_evac_crescendo]->somatic = 0;
            }
            do_evac_crescendo( ch, ( char * ) "all" );
        }
        else if ( !str_cmp( arg, "ottos dance" ) || !str_cmp( arg, "ottos" ) ) {
            if ( skill_table[gsn_ottos_dance]->somatic != 0 ) {
                skill_table[gsn_ottos_dance]->somatic = 0;
            }
            do_ottos_dance( ch, ( char * ) "" );
        }
        else if ( !str_cmp( arg, "unholy melody" ) || !str_cmp( arg, "unholy" ) ) {
            if ( skill_table[gsn_unholy_melody]->somatic != 0 ) {
                skill_table[gsn_unholy_melody]->somatic = 0;
            }
            do_unholy_melody( ch, ( char * ) "" );
        }
        else if ( !str_cmp( arg, "sound waves" ) || !str_cmp( arg, "sound" ) ) {
            if ( skill_table[gsn_sound_waves]->somatic != 0 ) {
                skill_table[gsn_sound_waves]->somatic = 0;
            }
            do_sound_waves( ch, ( char * ) "" );
        }
// type of instrument. 1 is woodwind, 2 is strings, 3 is brass, 4 is drums.
        if ( obj->value[0] == 1 ) {
            learn_from_success( ch, gsn_woodwinds );
        }
        else if ( obj->value[0] == 2 ) {
            learn_from_success( ch, gsn_strings );
        }
        else if ( obj->value[0] == 3 ) {
            learn_from_success( ch, gsn_brass );
        }
        else if ( obj->value[0] == 4 ) {
            learn_from_success( ch, gsn_drums );
        }

        learn_from_success( ch, gsn_play );
    }

    else {
        learn_from_failure( ch, gsn_play );
        send_to_char( "&OYou begin to play a melody, but stop as you play out of tune.&D\r\n", ch );
        return;
    }
    return;
}

void do_rousing_tune( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    bool                    vocals = FALSE;

    if ( !CAN_PLAY( ch, skill_table[gsn_rousing_tune]->value ) )
        return;

    if ( can_use_skill( ch, number_percent(  ), gsn_rousing_tune ) ) {
        send_to_char( "&cYou begin to sing.\r\n", ch );
        WAIT_STATE( ch, skill_table[gsn_rousing_tune]->beats );
        act( AT_ORANGE, "You sing a rousing tale of battles of old!", ch, NULL, NULL, TO_CHAR );
        act( AT_ORANGE, "$n begins singing a rousing tale of battles of old!", ch, NULL,
             NULL, TO_ROOM );
        learn_from_success( ch, gsn_rousing_tune );
        learn_from_success( ch, gsn_vocals );
        if ( can_use_skill( ch, number_percent(  ), gsn_vocals )
             && number_range( 1, 3 ) == 1 )
            vocals = TRUE;

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !is_same_group( vch, ch ) )
                continue;

            affect_strip( vch, gsn_rousing_tune );
            af.type = gsn_rousing_tune;
            af.duration = ch->level * 4;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = 1;
            xCLEAR_BITS( af.bitvector );
            affect_to_char( vch, &af );

            af.type = gsn_rousing_tune;
            af.duration = ch->level * 4;
            af.level = ch->level;
            af.location = APPLY_CON;
            af.modifier = 1;
            xCLEAR_BITS( af.bitvector );
            affect_to_char( vch, &af );

            send_to_char( "&OYou feel courage and strength seep into your body!&w\r\n", vch );

            if ( vocals ) {
                send_to_char
                    ( "&OThe music changes to instill you with stability and quickness!&w\r\n",
                      vch );
                if ( number_range( 1, 2 ) == 1 )
                    learn_from_success( ch, gsn_vocals );

                af.type = gsn_rousing_tune;
                af.duration = ch->level * 4;
                af.level = ch->level;
                af.location = APPLY_DEX;
                af.modifier = 2;
                xCLEAR_BITS( af.bitvector );
                affect_to_char( vch, &af );
            }
        }
    }
    else {
        learn_from_failure( ch, gsn_rousing_tune );
        send_to_char( "&cYou forget some of the words to the tale.&w\r\n", ch );
        return;
    }
    return;
}

void do_astral_body( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 15;

    if ( IS_AFFECTED( ch, AFF_BODY ) ) {
        send_to_char( "&cYou return to your normal body.\r\n", ch );
        affect_strip( ch, gsn_astral_body );
        xREMOVE_BIT( ch->affected_by, AFF_BODY );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic energy to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_astral_body ) ) {
        send_to_char
            ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
              ch );
        WAIT_STATE( ch, skill_table[gsn_astral_body]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "You concentrate on assuming an astral form.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "Your kinetic energy expands around your body!\r\n", ch, NULL, NULL,
             TO_CHAR );
        learn_from_success( ch, gsn_astral_body );

        af.type = gsn_astral_body;
        af.duration = ch->level * 4;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_BODY );
        affect_to_char( ch, &af );

        af.type = gsn_astral_body;
        af.duration = ch->level * 4;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_INVISIBLE );
        affect_to_char( ch, &af );

        af.type = gsn_astral_body;
        af.duration = ch->level * 4;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_PASS_DOOR );
        affect_to_char( ch, &af );

    }
    else {
        learn_from_failure( ch, gsn_astral_body );
        send_to_char
            ( "&cYou failed to control to assume astral form, and return to your body.\r\n", ch );
    }
    return;
}

void do_higher_magic( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = ch->mana / 8;

    if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        send_to_char
            ( "You release your call of higher magic, and your mana returns to normal levels.\r\n",
              ch );
        affect_strip( ch, gsn_higher_magic );
        xREMOVE_BIT( ch->affected_by, AFF_HIGHER_MAGIC );
        return;
    }
    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_higher_magic ) ) {
        WAIT_STATE( ch, skill_table[gsn_higher_magic]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_WHITE,
             "You call upon the higher magic feeling a rush as mana surges into your body!\r\n",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_higher_magic );

        af.type = gsn_higher_magic;
        af.duration = ch->level * 4;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_HIGHER_MAGIC );
        affect_to_char( ch, &af );
    }
    else {
        learn_from_failure( ch, gsn_higher_magic );
        send_to_char
            ( "You feel weakened as you failed to summon the mana surge for higher magic.\r\n",
              ch );
    }
    return;
}

void do_behead( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *corpse;
    OBJ_DATA               *obj;
    bool                    found;
    char                    buf[MSL];

    found = FALSE;

    for ( corpse = ch->in_room->first_content; corpse; corpse = corpse->next_content ) {
        if ( corpse->item_type == ITEM_CORPSE_PC || corpse->item_type == ITEM_CORPSE_NPC ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "There must be a corpse to attempt it.\r\n", ch );
        return;
    }

    separate_obj( corpse );

    if ( corpse->value[6] == 1 ) {
        send_to_char( "The corpse has already been beheaded.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_behead ) ) {
        WAIT_STATE( ch, skill_table[gsn_behead]->beats );
        corpse->value[6] = 1;
        obj = create_object( get_obj_index( 1135 ), 0 );
        obj_to_char( obj, ch );
        if ( obj->owner == NULL ) {
            obj->owner = STRALLOC( ch->name );
        }
        else {
            STRFREE( obj->owner );
            obj->owner = STRALLOC( ch->name );
        }
        snprintf( buf, MSL, "%s head", corpse->name );
        STRFREE( obj->name );
        obj->name = STRALLOC( buf );
        snprintf( buf, MSL, "A decapitated head from %s", corpse->short_descr );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "A decapitated head from %s is laying on the ground here.",
                  corpse->short_descr );
        STRFREE( obj->description );
        obj->description = STRALLOC( buf );
        make_blood( ch );
        act( AT_RED, "You gruesomely behead $p!", ch, corpse, NULL, TO_CHAR );
        act( AT_RED, "$n's beheads $p!", ch, corpse, NULL, TO_ROOM );
        learn_from_success( ch, gsn_behead );
        return;
    }
    else
        send_to_char( "\r\nYou hack up the head in a poor attempt to behead the corpse.\r\n", ch );
    learn_from_failure( ch, gsn_behead );
}

void do_shield( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 10;

    if ( IS_AFFECTED( ch, AFF_SHIELD ) ) {
        send_to_char( "You release your protective shield.\r\n", ch );
        affect_strip( ch, gsn_shield );
        xREMOVE_BIT( ch->affected_by, AFF_SHIELD );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_SANCTUARY ) ) {
        send_to_char
            ( "You cannot use your shield power, if you're affected by sanctuary.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }
    if ( IS_BLOODCLASS( ch ) && ch->blood < nomore ) {
        send_to_char( "You do not have enough blood to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_shield ) ) {
        WAIT_STATE( ch, skill_table[gsn_shield]->beats );
        ch->mana = ( ch->mana - nomore );
        if ( IS_BLOODCLASS( ch ) ) {
            ch->blood = ( ch->blood - nomore );
        }
        act( AT_WHITE,
             "You concentrate on forcing your inner magic to expand out of your body.\r\n",
             ch, NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "Your energy forms a shielding sphere around your body!\r\n", ch,
             NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_shield );

        af.type = gsn_shield;
        af.duration = -1;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_SHIELD );
        affect_to_char( ch, &af );

    }
    else {
        learn_from_failure( ch, gsn_shield );
        send_to_char( "You failed to control your shield, and it has dispersed.\r\n", ch );
    }
    return;
}

void do_healing_thoughts( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    nomore = 10;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
        victim = ch;
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim->hit >= victim->max_hit ) {
        send_to_char( "They do not need healing.\r\n", ch );
        return;
    }

    if ( victim->hit < 1 ) {
        send_to_char( "They are too far gone for you to help them.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_healing_thoughts ) ) {
        WAIT_STATE( ch, skill_table[gsn_healing_thoughts]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "You concentrate on using your kinetic energy to heal wounds.",
             ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n uses their kinetic energy to heal your wounds!", ch, NULL,
             victim, TO_VICT );
        act( AT_CYAN, "$n uses their kinetic energy to heal $N's wounds!", ch, NULL, victim,
             TO_NOTVICT );

        learn_from_success( ch, gsn_healing_thoughts );
        victim->hit = ( victim->hit + ( ch->level * 5 ) );
        if ( victim->hit > victim->max_hit ) {         // Bug fix below, -Taon
            victim->hit = victim->max_hit;
        }
    }
    else {
        learn_from_failure( ch, gsn_healing_thoughts );
        send_to_char
            ( "&cYou failed to send enough kinetic energy toward the desired healing.\r\n", ch );
    }
    return;
}

//This command was slightly modified by: Taon on 8-14-07.
//Modified again to make more blind friendly. -Taon
//Note should really rewrite this.

void do_angelfire( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    arg2[MIL];
    short                   nomore;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't perform such a task right now.", ch );
        return;
    }
    // Modified to find target if ch is fighting and can't see them. -Taon
    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Syntax: angelfire <victim> smite\r\n", ch );
        send_to_char( "Syntax: angelfire <victim> heal\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }

    if ( ch == victim ) {
        send_to_char( "You cannot use angelfire on yourself.\r\n", ch );
        return;
    }
    if ( ch->level < 20 )
        nomore = 20;
    else if ( ch->level >= 20 )
        nomore = 40;

    if ( !IS_NPC( ch ) && ch->mana <= nomore ) {
        set_char_color( AT_WHITE, ch );
        send_to_char( "You don't have enough mana.\r\n", ch );
        return;                                        /* missing return fixed March
                                                        * 11/96 */
    }

    if ( !str_cmp( arg2, "smite" ) ) {
        if ( can_use_skill( ch, number_percent(  ), gsn_angelfire ) ) {
            WAIT_STATE( ch, skill_table[gsn_angelfire]->beats );
            ch->mana = ( ch->mana - nomore );
            act( AT_YELLOW, "You call upon the heavens for angel fire's smite!\r\n", ch,
                 NULL, victim, TO_CHAR );
            act( AT_YELLOW, "$n calls upon the heavens to smite you!\r\n", ch, NULL, victim,
                 TO_VICT );
            act( AT_RED, "You unleash angelfire's fury upon $N!", ch, NULL, victim, TO_CHAR );
            act( AT_RED, "$n unleashes angelfire's fury upon you!", ch, NULL, victim, TO_VICT );
            act( AT_RED, "$n unleashes angelfire's fury upon $N!", ch, NULL, victim, TO_NOTVICT );
            learn_from_success( ch, gsn_angelfire );

            global_retcode =
                damage( ch, victim, extrahigh + number_range( 80, 120 ), gsn_angelfire );
        }
        else {
            learn_from_failure( ch, gsn_angelfire );
            global_retcode = damage( ch, victim, 0, gsn_angelfire );
            send_to_char( "You are too tired to call upon the heaven's for angelfire.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg2, "heal" ) ) {
        if ( victim->hit == victim->max_hit ) {
            send_to_char( "They do not need angelfire healing.\r\n", ch );
            return;
        }
        if ( victim->hit > victim->max_hit ) {
            send_to_char( "They do not need angelfire healing.\r\n", ch );
            return;
        }
        // Bug fix here, was allowing one to gain massive ammounts
        // of exp in battle by healing the mobs and beating them
        // down again, repeataly.  -Taon
        if ( IS_NPC( victim ) ) {
            send_to_char( "You can no longer heal mobs with angelfire.\r\n", ch );
            return;
        }
        if ( can_use_skill( ch, number_percent(  ), gsn_angelfire ) ) {
            WAIT_STATE( ch, skill_table[gsn_angelfire]->beats );
            ch->mana = ( ch->mana - nomore );
            if ( victim->race == RACE_DEMON ) {
                act( AT_PLAIN, "Tears roll down your cheek as you forgive $N!\r\n", ch, NULL,
                     victim, TO_CHAR );
                act( AT_PLAIN, "Tears roll down $n's cheek as $e forgives you!\r\n", ch,
                     NULL, victim, TO_VICT );
                act( AT_PLAIN, "Tears roll down $n's cheek ad $e forgives $N!\r\n", ch, NULL,
                     victim, TO_NOTVICT );
            }
            act( AT_YELLOW,
                 "You call upon the heavens for angel fire's healing powers!\r\n", ch, NULL,
                 victim, TO_CHAR );
            act( AT_YELLOW, "$n calls upon the heavens to heal you!\r\n", ch, NULL, victim,
                 TO_VICT );
            act( AT_LBLUE, "You unleash angelfire's healing powers upon $N!", ch, NULL,
                 victim, TO_CHAR );
            act( AT_LBLUE, "$n unleashes angelfire's healing powers upon you!", ch, NULL,
                 victim, TO_VICT );
            act( AT_LBLUE, "$n unleashes angelfire's healing powers upon $N!", ch, NULL,
                 victim, TO_NOTVICT );
            learn_from_success( ch, gsn_angelfire );
            victim->hit += ( victim->hit + ch->level * 10 );
            if ( victim->hit > victim->max_hit + ch->level ) {
                if ( ch->level < 50 )
                    victim->hit = victim->max_hit + 50;

                else
                    victim->hit = victim->max_hit + ch->level;
            }
        }
        else {
            learn_from_failure( ch, gsn_angelfire );
            send_to_char( "You are too tired to call upon the heaven's for angelfire.\r\n", ch );
        }
        return;
    }
    else
        send_to_char( "Syntax: angelfire <victim> smite\r\n", ch );
    send_to_char( "Syntax: angelfire <victim> heal\r\n", ch );
    return;
}

void do_brain_boil( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    int                     dam;

    if ( ch->secondclass == -1 ) {
        dam = maximum;
    }
    if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
        dam = maximum - ch->level;
    }
    if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
        dam = maximum - ( ch->level * 2 );
    }
    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }
    if ( IS_NPC( ch ) ) {
        dam = maximum;
        act( AT_WHITE, "$n hits you with a searing heat in your head!", ch, NULL, victim, TO_VICT );
        global_retcode = damage( ch, victim, dam, gsn_brain_boil );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    WAIT_STATE( ch, skill_table[gsn_brain_boil]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_brain_boil ) ) {
        act( AT_WHITE, "You send a wave of white hot heat into $N's head!", ch,
             NULL, victim, TO_CHAR );
        act( AT_WHITE, "$n hits you with a searing heat in your head!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_brain_boil );
        global_retcode = damage( ch, victim, dam, gsn_brain_boil );
    }
    else {
        learn_from_failure( ch, gsn_brain_boil );
        global_retcode = damage( ch, victim, 0, gsn_brain_boil );
        send_to_char( "You try to boil their brain, but cannot concentrate enough.\r\n", ch );
    }
    return;
}

void do_eldritch_bolt( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   chance;
    short                   highermagic;

    highermagic = 300;
    short                   dam;

    if ( ch->secondclass == -1 ) {
        dam = maximum + ch->level;
    }
    if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
        dam = maximum;
    }
    if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
        dam = maximum - ( ch->level );
    }

    if ( IS_NPC( ch ) ) {
        dam = maximum + ch->level;
    }

    chance = number_chance( 1, 4 );

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( ch->mana < 300 && IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        send_to_char( "You don't have enough mana to use higher magic with it.\r\n", ch );
        return;
    }
    else if ( ch->mana < highermagic / 3 ) {
        send_to_char( "You don't have enough mana to use eldritch bolt.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_eldritch_bolt ) ) {
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            send_to_char
                ( "You send a surge of mana into the spell invoking the higher magic.\r\n", ch );
            ch->mana = ch->mana - highermagic;
        }
        else {
            ch->mana = ch->mana - ( highermagic / 3 );
        }

        WAIT_STATE( ch, skill_table[gsn_eldritch_bolt]->beats );
        act( AT_LBLUE, "You focus your power, and send a bolt of pure mana at $N!", ch,
             NULL, victim, TO_CHAR );
        act( AT_LBLUE, "$n hits you with an eldritch bolt!", ch, NULL, victim, TO_VICT );
        act( AT_LBLUE, "$n sends an eldritch bolt into $N!", ch, NULL, victim, TO_NOTVICT );
        act( AT_WHITE, "The cost of channeling a bolt of pure mana rakes your body!", ch,
             NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_eldritch_bolt );
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            global_retcode =
                damage( ch, victim, dam + number_range( 500, 599 ), gsn_eldritch_bolt );
            global_retcode = damage( ch, ch, number_range( 200, 300 ), gsn_eldritch_bolt );
        }
        else {
            global_retcode = damage( ch, victim, dam, gsn_eldritch_bolt );
            global_retcode = damage( ch, ch, number_range( 60, 120 ), gsn_eldritch_bolt );
        }
    }
    else {
        ch->mana = ch->mana - ( highermagic / 6 );
        learn_from_failure( ch, gsn_eldritch_bolt );
        global_retcode = damage( ch, victim, 0, gsn_eldritch_bolt );
        send_to_char
            ( "You try to summon a bolt of pure mana, but cannot concentrate enough.\r\n", ch );
        WAIT_STATE( ch, skill_table[gsn_eldritch_bolt]->beats );
    }
    return;
}

void do_inferno( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   highermagic;

    highermagic = 30;

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        act( AT_YELLOW, "$n sends the fires from a blazing inferno into you!", ch, NULL,
             victim, TO_VICT );
        act( AT_RED, "$n sends a blazing inferno at $N!", ch, NULL, victim, TO_NOTVICT );
        act( AT_RED, "$n's blazing inferno totally surrounds $N!", ch, NULL, victim, TO_ROOM );
        global_retcode = damage( ch, victim, number_range( 50, ch->level + 50 * 2 ), gsn_inferno );
        return;
    }

    if ( ch->mana < 30 && IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        send_to_char( "You don't have enough mana to use higher magic with it.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_inferno ) ) {
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            send_to_char
                ( "You send a surge of mana into the spell invoking the higher magic.\r\n", ch );
            ch->mana = ch->mana - highermagic;
        }

        WAIT_STATE( ch, skill_table[gsn_inferno]->beats );
        act( AT_YELLOW, "You call upon a blazing inferno against $N!", ch, NULL, victim, TO_CHAR );
        act( AT_YELLOW, "$n sends the fires from a blazing inferno into you!", ch, NULL,
             victim, TO_VICT );
        act( AT_RED, "$n sends a blazing inferno at $N!", ch, NULL, victim, TO_NOTVICT );
        act( AT_RED, "The blazing inferno totally surrounds $N!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n's blazing inferno totally surrounds $N!", ch, NULL, victim, TO_ROOM );
        learn_from_success( ch, gsn_inferno );
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            global_retcode = damage( ch, victim, high + number_range( 200, 299 ), gsn_inferno );
        }
        else {
            global_retcode = damage( ch, victim, ludicrous, gsn_inferno );
        }
    }
    else {
        learn_from_failure( ch, gsn_inferno );
        global_retcode = damage( ch, victim, 0, gsn_inferno );
        send_to_char
            ( "You try to summon a blazing inferno, but fail to concentrate enough.\r\n", ch );
    }
    return;
}

void do_blizzard( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   highermagic;

    highermagic = 40;

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        act( AT_CYAN, "The chilling winds of a blizzard from $n slam into you!", ch, NULL,
             victim, TO_VICT );
        act( AT_CYAN, "$n sends a blizzard at $N!", ch, NULL, victim, TO_NOTVICT );
        act( AT_LBLUE, "$n's blizzard totally surrounds $N!", ch, NULL, victim, TO_ROOM );
        global_retcode =
            damage( ch, victim, number_range( ch->level, ch->level + 25 ), gsn_blizzard );
        return;
    }

    if ( ch->mana < 30 && IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
        send_to_char( "You don't have enough mana to use higher magic with it.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_blizzard ) ) {
        if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
            send_to_char
                ( "You send a surge of mana into the spell invoking the higher magic.\r\n", ch );
            ch->mana = ch->mana - highermagic;
        }

        WAIT_STATE( ch, skill_table[gsn_blizzard]->beats );
        act( AT_CYAN, "You call upon a blizzard against $N!", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "The chilling winds of a blizzard from $n slam into you!", ch, NULL,
             victim, TO_VICT );
        act( AT_CYAN, "$n sends a blizzard at $N!", ch, NULL, victim, TO_NOTVICT );
        act( AT_LBLUE, "The blizzard totally surrounds $N!", ch, NULL, victim, TO_CHAR );
        act( AT_LBLUE, "$n's blizzard totally surrounds $N!", ch, NULL, victim, TO_ROOM );
        learn_from_success( ch, gsn_blizzard );

        short                   dam;

        if ( ch->Class == CLASS_DRUID || ch->secondclass == CLASS_DRUID
             || ch->thirdclass == CLASS_DRUID ) {
            if ( ch->secondclass == -1 ) {
                dam = maximum + number_range( 150, 200 );
                global_retcode = damage( ch, victim, dam, gsn_blizzard );
            }
            else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
                dam = insane + ( ch->level / 2 );
                global_retcode = damage( ch, victim, dam, gsn_blizzard );
            }
            else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
                dam = insane;
                global_retcode = damage( ch, victim, dam, gsn_blizzard );
            }
        }
        else if ( ch->Class != CLASS_DRUID && ch->secondclass != CLASS_DRUID
                  && ch->thirdclass != CLASS_DRUID ) {
            dam = insane;
            global_retcode = damage( ch, victim, dam, gsn_blizzard );
        }

    }
    else {
        learn_from_failure( ch, gsn_blizzard );
        global_retcode = damage( ch, victim, 0, gsn_blizzard );
        send_to_char( "You try to summon a blizzard, but fail to concentrate enough.\r\n", ch );
    }
    return;
}

void do_gust_of_wind( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch,
                           *vch_next;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char( "You cannot do that here.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_gust_of_wind ) ) {
        act( AT_ORANGE, "You begin to flap your wings rapidly causing a gust of wind!", ch,
             NULL, NULL, TO_CHAR );
        act( AT_ORANGE,
             "$n with wings flapping creates a gust of wind, that knocks you down!", ch,
             NULL, NULL, TO_ROOM );

        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            // Bug fix here, no longer will grouped people get hit too. -Taon
            if ( is_same_group( vch, ch ) )
                continue;

            if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                continue;

            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;
            if ( vch != ch ) {
                global_retcode =
                    damage( ch, vch, number_range( 10, ( ch->level + 25 ) ), gsn_gust_of_wind );
                stop_fighting( vch, TRUE );
                do_sit( vch, ( char * ) "" );
            }
        }
        act( AT_WHITE, "$n's gust of wind dies down..", ch, NULL, NULL, TO_ROOM );
        act( AT_ORANGE, "Your gust of wind knocks your enemies to the ground.", ch, NULL,
             NULL, TO_CHAR );
        WAIT_STATE( ch, skill_table[gsn_gust_of_wind]->beats );
        learn_from_success( ch, gsn_gust_of_wind );
        return;
    }
    send_to_char( "You try to generate a gust of wind, but run out of energy.\r\n", ch );
    learn_from_failure( ch, gsn_gust_of_wind );
}

//Reworked skill and added a little monk support here. -Taon
void do_judo( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   chance;

    chance = number_chance( 1, 5 );

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Try to judo flip who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim->race == RACE_DRAGON ) {
        send_to_char( "That would be most foolish to attempt, its a dragon!\r\n", ch );
        return;
    }

    if ( IS_RESIS( victim, RIS_PARALYSIS ) ) {
        chance -= 1;
    }

    if ( IS_NPC( victim ) && victim->level > 50 && chance < 3 ) {
        act( AT_WHITE,
             "You try a judo technique on $N, but $E sees it coming.  $N maybe too powerful an adversary to be judo flipped.",
             ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( !IS_NPC( victim ) && ( get_curr_dex( victim ) - get_curr_dex( ch ) >= 2 )
         || ( victim->level - ch->level > 4 ) ) {
        act( AT_WHITE,
             "You try a judo technique on $N, but $E sees it coming.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( chance < 3 ) {
        act( AT_WHITE,
             "You try a judo technique on $N, but $E sees it coming.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, ( number_percent(  ) ), gsn_judo ) ) {
        WAIT_STATE( ch, 1 * ( PULSE_VIOLENCE / 2 ) );
        if ( IS_NPC( victim ) )
            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
        else
            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 2 ) );
        if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
            af.type = gsn_judo;
            af.location = APPLY_AC;

            af.modifier = 20;
            af.duration = number_range( 1, 2 );
            af.level = ch->level;
            af.bitvector = meb( AFF_PARALYSIS );
            affect_to_char( victim, &af );
            update_pos( victim );
            learn_from_success( ch, gsn_judo );

            if ( ch->Class == CLASS_MONK )
                global_retcode = damage( ch, victim, number_chance( 10, 20 ), gsn_judo );
            else
                global_retcode = damage( ch, victim, number_chance( 3, 15 ), gsn_judo );

            send_to_char( "You temporarily lose consciousness from a judo flip.\r\n", victim );
        }
    }
    else {
        WAIT_STATE( ch, PULSE_VIOLENCE );
        if ( !IS_NPC( ch ) )
            learn_from_failure( ch, gsn_judo );
        act( AT_WHITE, "$n attempts a judo technique on you, nearly stunning you.", ch, NULL,
             victim, TO_VICT );
        act( AT_WHITE, "You try a judo technique on $N, but $E manages not to get stunned.", ch,
             NULL, victim, TO_CHAR );
        act( AT_WHITE, "$n tries a judo technique on $N, but doesn't stun them.", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, number_chance( 3, 15 ), gsn_judo );
    }
}

void do_torture_mind( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 30;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Who do you want to torture their mind?\r\n", ch );
        return;
    }
    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_torture_mind ) ) {
        WAIT_STATE( ch, skill_table[gsn_torture_mind]->beats );
        act( AT_CYAN, "You break down the defenses to $N's mind!", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n breaks down your mental defenses!", ch, NULL, victim, TO_VICT );
        act( AT_RED, "\r\n\r\nYou mind is wracked with pain beyond describing!", ch, NULL,
             victim, TO_VICT );
        learn_from_success( ch, gsn_torture_mind );
        ch->mana = ( ch->mana - nomore );
        worsen_mental_state( victim, number_chance( 5, 10 ) );
        global_retcode = damage( ch, victim, insane, gsn_torture_mind );
    }
    else {
        learn_from_failure( ch, gsn_torture_mind );
        global_retcode = damage( ch, victim, 0, gsn_torture_mind );
        send_to_char( "&CYou fail to penetrate their mental defenses.\r\n", ch );
    }
    return;
}

void do_shrieking_note( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Send your shrieking note on who?\r\n", ch );
        return;
    }

    if ( !CAN_PLAY( ch, skill_table[gsn_shrieking_note]->value ) )
        return;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_shrieking_note ) ) {
        act( AT_ORANGE, "$n's voice becomes a high pitched shriek!", ch, NULL, NULL, TO_ROOM );
        WAIT_STATE( ch, skill_table[gsn_shrieking_note]->beats );
        act( AT_ORANGE, "You send your high pitched vocals directly at $N's ears!", ch,
             NULL, victim, TO_CHAR );
        act( AT_ORANGE, "$n sends a high pitched sound wave into your ears!", ch, NULL,
             victim, TO_VICT );
        learn_from_success( ch, gsn_shrieking_note );
        learn_from_success( ch, gsn_vocals );
        if ( can_use_skill( ch, number_percent(  ), gsn_vocals )
             && number_range( 1, 3 ) == 1 ) {
            act( AT_ORANGE, "With your musical skill, you shriek higher and louder!", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_ORANGE, "With $s musical skill, $n shrieks higher and louder!", ch,
                 NULL, victim, TO_VICT );
            if ( number_range( 1, 2 ) == 1 )
                learn_from_success( ch, gsn_vocals );
            global_retcode = damage( ch, victim, ( medium + 10 ), gsn_shrieking_note );
        }
        else
            global_retcode = damage( ch, victim, medium, gsn_shrieking_note );
    }
    else {
        learn_from_failure( ch, gsn_shrieking_note );
        global_retcode = damage( ch, victim, 0, gsn_shrieking_note );
        send_to_char
            ( "&cYou fail to get a direct hit on them with your high pitched sound wave.&w\r\n",
              ch );
    }
    return;
}

void do_thunderous_hymn( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 30;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Send your thunderous hymn vocals on who?\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You don't seem to have the vocals to do that.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_thunderous_hymn ) ) {
        act( AT_ORANGE, "$n's voice becomes thunderously deafening!", ch, NULL, NULL, TO_ROOM );
        WAIT_STATE( ch, skill_table[gsn_thunderous_hymn]->beats );
        act( AT_ORANGE, "You send your vocals directly at $N's ears!", ch, NULL, victim, TO_CHAR );
        act( AT_ORANGE, "$n sends a thunderous hymn sound wave into your ears!", ch, NULL,
             victim, TO_VICT );
        learn_from_success( ch, gsn_thunderous_hymn );
        learn_from_success( ch, gsn_vocals );
        ch->mana = ( ch->mana - nomore );
        global_retcode = damage( ch, victim, high, gsn_thunderous_hymn );
    }
    else {
        learn_from_failure( ch, gsn_thunderous_hymn );
        global_retcode = damage( ch, victim, 0, gsn_thunderous_hymn );
        send_to_char( "&CYou fail to get a direct hit on them with your sound wave.\r\n", ch );
    }
    return;
}

void do_heart_grab( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 30;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Launch a heart grab attack upon who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_heart_grab ) ) {
        WAIT_STATE( ch, skill_table[gsn_heart_grab]->beats );
        act( AT_GREEN,
             "You deliver the legendary quivering palm strike $N's body!\r\n You turn your hand into a spear shape piercing their chest cavity!",
             ch, NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "A rush of fear and pain hits you as $n's heart grab bursts into your chest cavity!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_heart_grab );
        ch->mana = ( ch->mana - nomore );
        short                   dam;

        if ( ch->secondclass == -1 ) {
            dam = ludicrous + number_range( 100, 140 );
            global_retcode = damage( ch, victim, dam, gsn_heart_grab );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
            dam = ludicrous;
            global_retcode = damage( ch, victim, dam, gsn_heart_grab );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
            dam = ludicrous - ( ch->level );
            global_retcode = damage( ch, victim, dam, gsn_heart_grab );
        }

    }
    else {
        learn_from_failure( ch, gsn_heart_grab );
        global_retcode = damage( ch, victim, 0, gsn_heart_grab );
        send_to_char( "&cYour heart grab just misses it's target.\r\n", ch );
    }
    return;
}

void do_quivering_palm( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 30;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a quivering palm upon who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char
        ( "\r\n\r\n&OYou achieve complete inner peace as your palm begins quivering uncontrollably!\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_quivering_palm ) ) {
        WAIT_STATE( ch, skill_table[gsn_quivering_palm]->beats );
        act( AT_GREEN, "You deliver the legendary quivering palm strike to $N's body!", ch,
             NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "A rush of fear and pain hits you as $n's legendary quivering palm attack wracks your body in spams!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_quivering_palm );
        ch->mana = ( ch->mana - nomore );

        if ( !ch->fighting )
            global_retcode = damage( ch, victim, extrahigh, gsn_quivering_palm );
        else
            global_retcode = damage( ch, victim, medium, gsn_quivering_palm );

    }
    else {
        learn_from_failure( ch, gsn_quivering_palm );
        global_retcode = damage( ch, victim, 0, gsn_quivering_palm );
        send_to_char( "&cYour quivering palm just misses it's target.\r\n", ch );
    }
    return;
}

void do_dehydrate( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   nomore = 50;
    short                   noblood = 10;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Dehydrate who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "\r\nYou don't seem to have any affect on them.\r\n", ch );
        return;
    }

    if ( IS_BLOODCLASS( ch ) ) {
        if ( ch->blood < noblood ) {
            send_to_char( "You do not have enough blood to do that.\r\n", ch );
            return;
        }
    }
    else if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }
    else if ( ch->move < 100 ) {
        send_to_char( "You do not have enough move points left to do that.\r\n", ch );
        return;
    }

    send_to_char( "\r\n\r\n&cYou begin to focus on expelling all unneccessary fluids.\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_dehydrate ) ) {
        WAIT_STATE( ch, skill_table[gsn_dehydrate]->beats );
        act( AT_CYAN,
             "It feels like all your bodily fluids are suddenly being released at once!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_dehydrate );
        if ( IS_BLOODCLASS( ch ) ) {
            ch->blood = ( ch->blood - noblood );
            ch->move = ( ch->move - ch->move / 4 );
        }
        else {
            ch->mana = ( ch->mana - nomore );
            ch->move = ( ch->move - ch->move / 4 );
        }
        victim->pcdata->condition[COND_THIRST] -= ( ch->level / 5 ) + 5;

        if ( victim->pcdata->condition[COND_THIRST] < 0 )
            victim->pcdata->condition[COND_THIRST] = 0;
    }
    else {
        learn_from_failure( ch, gsn_dehydrate );
        send_to_char( "&cYou lose your concentration.\r\n", ch );
    }

    return;
}

void do_grendals_stance( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( ch->move < 20 ) {
        send_to_char( "You do not have enough energy left to try this.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_GRENDALS_STANCE ) ) {
        send_to_char( "You already have called on your reserve energy.\r\n", ch );
        return;
    }

    if ( ch->position == POS_STUNNED || ch->hit <= 0 ) {
        send_to_char( "You cannot use grendals stance while stunned!\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_grendals_stance ) ) {
        WAIT_STATE( ch, skill_table[gsn_grendals_stance]->beats );
        send_to_char
            ( "\r\n\r\n&RYou put all your reserve energy into making this last stand.\r\n", ch );
        learn_from_success( ch, gsn_grendals_stance );
        af.type = gsn_grendals_stance;
        af.duration = 1000;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_GRENDALS_STANCE );
        affect_to_char( ch, &af );

        ch->hit = ( ch->hit ) + ( get_curr_con( ch ) * 4 ) + ( ch->move / 2 );
        ch->move = 0;
    }
    else {
        learn_from_failure( ch, gsn_grendals_stance );
        send_to_char( "&CYou fail to summon your energy reserves.\r\n", ch );
    }
    return;
}

void do_psionic_blast( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 30;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a psionic blast upon who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_psionic_blast ) ) {
        WAIT_STATE( ch, skill_table[gsn_psionic_blast]->beats );
        act( AT_CYAN, "You send a psionic blast to assault $N's mind!", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "Angony fills your head as a psionic blast assaults your mind from $n!", ch,
             NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_psionic_blast );
        ch->mana = ( ch->mana - nomore );
        global_retcode = damage( ch, victim, ludicrous, gsn_psionic_blast );
    }
    else {
        learn_from_failure( ch, gsn_psionic_blast );
        global_retcode = damage( ch, victim, 0, gsn_psionic_blast );
        send_to_char( "&CYour psionic blast fails to penetrate their mental defenses.\r\n", ch );
    }
    return;
}

void do_mental_assault( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 20;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a mental assault upon who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_mental_assault ) ) {
        WAIT_STATE( ch, skill_table[gsn_mental_assault]->beats );
        act( AT_CYAN, "You send a wave of destructive energy to assault $N's mind!", ch,
             NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "Angony fills your head as a wave of destructive energy assaults your mind from $n!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_mental_assault );
        ch->mana = ( ch->mana - nomore );
        global_retcode = damage( ch, victim, ludicrous, gsn_mental_assault );
    }
    else {
        learn_from_failure( ch, gsn_mental_assault );
        global_retcode = damage( ch, victim, 0, gsn_mental_assault );
        send_to_char( "&CYour mental assault fails to penetrate their mental defenses.\r\n", ch );
    }
    return;
}

void do_leech( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Mentally leech kinetic power from who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_leech ) ) {
        WAIT_STATE( ch, skill_table[gsn_leech]->beats );
        act( AT_CYAN,
             "You send a mental bridge to $N's mind, and leech their kinetic energy!", ch,
             NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "$n has broken your mental defenses, and is leeching your kinetic energy!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_leech );
        global_retcode = damage( ch, victim, number_chance( 10, 50 ), gsn_leech );
        if ( ch->mana < ch->max_mana ) {
            ch->mana = ch->mana + number_chance( 10, 50 );
        }
        if ( ch->mana > ch->max_mana ) {
            ch->mana = ch->max_mana;
        }
    }
    else {
        learn_from_failure( ch, gsn_leech );
        global_retcode = damage( ch, victim, 0, gsn_leech );
        send_to_char( "&cYou fail to establish a mental bridge.\r\n", ch );
    }
    return;
}

void do_dream_walk( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   nomore = 10;

    argument = one_argument( argument, arg );

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "Travel kinetically through whose dreams?\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough kinetic power to do that.\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, arg ) ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->position != POS_SLEEPING ) {
        send_to_char( "You must be asleep to dream walk.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why would you travel to yourself?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_SNARE ) || IS_AFFECTED( ch, AFF_PRAYER )
         || IS_AFFECTED( ch, AFF_ROOT ) ) {
        send_to_char( "But you cant move right now.\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GYou sense you cannot dream walk at your current location.\r\n", ch );
        return;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&GIt is beyond your ability to dream walk to that location.\r\n", ch );
        return;
    }

    if ( !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY ) ) {
        send_to_char( "&GIt is beyond your ability to dream walk to that location.\r\n", ch );
        return;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "&GYou cannot dream walk to a Staff member.\r\n", ch );
        return;
    }

    if ( !in_hard_range( ch, victim->in_room->area ) ) {
        send_to_char( "&GYou cannot dream walk to the area that your victim is in.\r\n", ch );
        return;
    }

    send_to_char
        ( "\r\n\r\n&RA rush of pain throbs in your head, as you concentrate on your task.\r\n",
          ch );

    if ( can_use_skill( ch, number_percent(  ), gsn_dream_walk ) ) {
        WAIT_STATE( ch, skill_table[gsn_dream_walk]->beats );
        ch->mana = ( ch->mana - nomore );
        act( AT_CYAN, "$n's body suddenly vanishes!", ch, NULL, NULL, TO_ROOM );
        send_to_char
            ( "&CYou travel through your dreams to the location you thought of.&D\r\n", ch );
        char_from_room( ch );
        char_to_room( ch, victim->in_room );
        if ( ch->on ) {
            ch->on = NULL;
            set_position( ch, POS_STANDING );
        }
        if ( ch->position != POS_STANDING ) {
            set_position( ch, POS_STANDING );
        }
        set_position( ch, POS_SLEEPING );
        act( AT_CYAN, "$n's body suddenly appears!", ch, NULL, NULL, TO_ROOM );
        do_look( ch, ( char * ) "auto" );
        return;
    }
    else {
        learn_from_failure( ch, gsn_dream_walk );
        send_to_char( "&cYou fail to breech their mental defenses.\r\n", ch );
    }
}

void do_assassinate( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    char                    arg2[MIL];
    int                     percent;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( arg[0] == '\0' ) {
        send_to_char( "Assassinate whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) ) {
        send_to_char( "Only on NPCs.\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
        return;
    }

    if ( ch->level <= victim->level ) {
        send_to_char( "You can only assassinate victims below your level.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "choke" ) ) {
        act( AT_WHITE, "You wrap your meaty arms around $N's neck, choking out $S life.",
             ch, NULL, victim, TO_CHAR );
        act( AT_WHITE, "$n wraps meaty arms around your neck and chokes you to death.", ch,
             NULL, victim, TO_VICT );
        act( AT_WHITE, "$n wraps meaty arms around $N's neck, choking out $S life.", ch,
             NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "poison" ) ) {
        act( AT_LBLUE,
             "You blow poison powder in $N's face, causing them to convulse as their body's blood vessels explode.",
             ch, NULL, victim, TO_CHAR );
        act( AT_LBLUE,
             "$n blew poison powder in your face, causing you to convulse as your blood vessels explode.",
             ch, NULL, victim, TO_VICT );
        act( AT_LBLUE,
             "$n blows poison powder in $N's face, causing $M to convulse as $S blood vessels explode.",
             ch, NULL, victim, TO_NOTVICT );
    }
    else if ( !str_cmp( arg2, "backstab" ) ) {
        act( AT_RED, "Your backstab finds its mark between $N's shoulder blades.", ch,
             NULL, victim, TO_CHAR );
        act( AT_RED,
             "$n's backstab finds the pressure point between your shoulder blades, stealing your life.",
             ch, NULL, victim, TO_VICT );
        act( AT_RED,
             "$n backstab finds the pressure point between $N's shoulder blades, stealing $S life.",
             ch, NULL, victim, TO_NOTVICT );
    }
    else {
        send_to_char( "What kind of assassination attempt: poison, choke, backstab?\r\n", ch );
        return;
    }
    percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );
    if ( !IS_AWAKE( victim ) || can_use_skill( ch, percent, gsn_backstab ) ) {
        learn_from_success( ch, gsn_assassinate );
        check_illegal_pk( ch, victim );
        set_cur_char( victim );
        raw_kill( ch, victim );
    }
    else {
        learn_from_failure( ch, gsn_assassinate );
        check_illegal_pk( ch, victim );
    }

    return;

}

/*
 *  Charge skill
 *  Author: Rantic (supfly@geocities.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */

void do_charge( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    int                     percent;
    int                     chance;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Charge at whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "How can you charge at yourself?\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    /*
     * Added stabbing weapon. -Narn
     */
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You need to wield a lance.\r\n", ch );
        return;
    }

    if ( obj->value[4] != WEP_LANCE ) {
        if ( ( obj->value[4] == WEP_LANCE && obj->value[3] != DAM_PIERCE )
             || obj->value[4] != WEP_LANCE ) {
            send_to_char( "You need to wield a lance.\r\n", ch );
            return;
        }
    }
    if ( !ch->mount ) {
        send_to_char( "You can't charge without a mount.\r\n", ch );
        return;
    }

    if ( victim->fighting ) {
        send_to_char( "You are fighting in too close of quarters to use a lance.\r\n", ch );
        if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) && obj->value[4] == WEP_LANCE )
            obj_from_char( obj );
        obj_to_char( obj, ch );
        return;
    }

    percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );

    check_attacker( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_charge]->beats );
    if ( !IS_AWAKE( victim ) || IS_NPC( ch )
         || percent < ch->pcdata->learned[gsn_charge] ) {
        act( AT_SKILL,
             "Apon your mount you wildly charge at $N, thrusting your lance into $S body!",
             ch, NULL, victim, TO_CHAR );
        act( AT_SKILL,
             "Apon $s mount $n charges wildly at you, thrusting $s lance into your body!",
             ch, NULL, victim, TO_VICT );
        act( AT_SKILL,
             "Apon $s mount $n wildly charges at $N, thrusting the lance into $S body!",
             ch, NULL, victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, ludicrous, gsn_charge );

        chance = number_range( 1, 100 );

        if ( chance < 20 ) {
            act( AT_SKILL, "Your lance shattered into pieces from the impact!!", ch, NULL,
                 victim, TO_CHAR );
            act( AT_SKILL, "$n's lance shatters into pieces upon impact with $N!", ch, NULL,
                 victim, TO_NOTVICT );
            if ( obj->value[4] == WEP_LANCE )
                obj_from_char( obj );
            extract_obj( obj );
            return;
        }

        if ( global_retcode == rVICT_DIED ) {
            act( AT_BLOOD,
                 "Your lance wickedly thrusts through your victim's chest, almost ripping it in two!",
                 ch, NULL, NULL, TO_CHAR );
        }
        if ( global_retcode != rCHAR_DIED && global_retcode != rBOTH_DIED ) {
            learn_from_success( ch, gsn_charge );
            check_illegal_pk( ch, victim );
            if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) && obj->value[4] == WEP_LANCE )
                obj_from_char( obj );
            obj_to_char( obj, ch );
            send_to_char( "\r\nYou holster your lance due to the close quarters.\r\n", ch );

        }
    }
    else
    {
        learn_from_failure( ch, gsn_charge );
        global_retcode = damage( ch, victim, 0, gsn_charge );
        act( AT_SKILL,
             "Apon your mount you wildly charge at $N, but $E dodges out of the way!", ch,
             NULL, victim, TO_CHAR );
        act( AT_SKILL,
             "Apon $s mount $n wildly charges at you, but you quickly dodge out of $s way.",
             ch, NULL, victim, TO_VICT );
        act( AT_SKILL,
             "Apon $s mount $n wildly charges at $N, but $E dodges out of the way!", ch,
             NULL, victim, TO_NOTVICT );
        if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) && obj->value[4] == WEP_LANCE )
            obj_from_char( obj );
        obj_to_char( obj, ch );
        send_to_char( "\r\nYou holster your lance due to the close quarters.\r\n", ch );
    }

    return;
}

void do_ball_of_fire( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *fireball;
    int                     chance;

    if ( IS_NPC( ch ) )
        return;

    if ( ch->fighting ) {
        send_to_char( "You're too busy fighting...\r\n", ch );
        return;
    }
    if ( ch->blood < 2 ) {
        send_to_char( "You are too weak to create a ball of fire...\r\n", ch );
        return;
    }
    for ( fireball = ch->last_carrying; fireball; fireball = fireball->prev_content ) {
        if ( fireball->pIndexData->vnum == OBJ_VNUM_FIREBALL ) {
            send_to_char( "You cannot concentrate enough to create two balls of fire.\r\n", ch );
            return;
        }
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_ball_of_fire ) ) {
        WAIT_STATE( ch, skill_table[gsn_ball_of_fire]->beats );
        fireball = create_object( get_obj_index( OBJ_VNUM_FIREBALL ), 0 );
        fireball->value[2] = 100;
        fireball->pIndexData->value[2] = 100;
        if ( ch->level < 20 ) {
            fireball->timer = ch->level;
        }
        else {
            fireball->timer = 22;
        }
        ch->blood = ch->blood - 1;
        act( AT_RED, "$p suddenly bursts into flames in $n's hands!", ch, fireball, NULL, TO_ROOM );
        act( AT_RED, "$p suddenly bursts into flames in your hands!", ch, fireball, NULL, TO_CHAR );
        fireball = obj_to_char( fireball, ch );
        learn_from_success( ch, gsn_ball_of_fire );
        return;
    }
    else {
        chance = number_range( 1, 100 );

        if ( chance < 24 ) {
            learn_from_failure( ch, gsn_ball_of_fire );
            act( AT_RED, "A ball of fire suddenly explodes in $n's hands!", ch, NULL, NULL,
                 TO_ROOM );
            act( AT_RED, "A ball of fire suddenly explodes in your hands!", ch, NULL, NULL,
                 TO_CHAR );
            global_retcode = damage( ch, ch, 2, gsn_ball_of_fire );
            ch->blood = ch->blood - 1;
            return;
        }
        else
            act( AT_PLAIN, "You are unable to concentrate enough to summon a ball of fire.",
                 ch, NULL, NULL, TO_CHAR );
    }
    return;
}

void do_unholy_sphere( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( ch->race == RACE_DEMON && ch->Class != CLASS_HELLSPAWN ) {
        error( ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "Not while you're still fighting.\r\n", ch );
        return;
    }

    if ( ch->blood < 3 ) {
        send_to_char
            ( "You try to create an unholy sphere, but don't have enough power right now.\r\n",
              ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_UNHOLY_SPHERE ) ) {
        send_to_char( "You are already within an unholy sphere.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_unholy_sphere]->beats );
    /*
     * Successful 
     */
    if ( can_use_skill( ch, number_percent(  ), gsn_unholy_sphere ) ) {
        /*
         * Immune to attacks 
         */
        af.type = gsn_unholy_sphere;
        af.duration = 20;
        af.location = APPLY_NONE;
        af.bitvector = meb( AFF_UNHOLY_SPHERE );
        af.modifier = 0;
        af.level = ch->level;
        affect_to_char( ch, &af );
        ch->blood -= 2;

        act( AT_BLOOD, "You create an unholy sphere around yourself.", ch, NULL, NULL, TO_CHAR );
        act( AT_BLOOD, "$n causes an unholy sphere to engulf $s body.", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_unholy_sphere );
        send_to_char
            ( "You sprawl out into a resting position as you are safely protected in the unholy sphere.\r\n",
              ch );
        set_position( ch, POS_RESTING );
        return;
    }

    /*
     * Failed 
     */
    act( AT_PLAIN,
         "You try to create an unholy sphere, but cannot focus your power enough.", ch,
         NULL, NULL, TO_CHAR );
    ch->blood -= 1;
    learn_from_failure( ch, gsn_unholy_sphere );
}

void do_flint_fire( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;

    switch ( ch->in_room->sector_type ) {
        case SECT_OCEAN:
        case SECT_SKY:
        case SECT_CLOUD:
        case SECT_QUICKSAND:
        case SECT_DEEPMUD:
        case SECT_PORTALSTONE:
        case SECT_LAKE:
        case SECT_DOCK:
        case SECT_RIVER:
        case SECT_WATERFALL:
        case SECT_SWAMP:
        case SECT_LAVA:
        case SECT_OCEANFLOOR:
        case SECT_AIR:
        case SECT_ROAD:
        case SECT_UNDERWATER:
        case SECT_INSIDE:
            send_to_char( "You cannot start a fire here.\r\n", ch );
            return;
        default:
            break;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
        send_to_char( "You can not start a fire here.\r\n", ch );
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "You are not holding a flint stone.\r\n", ch );
        return;
    }

    if ( obj->item_type != ITEM_TINDER ) {
        send_to_char( "You must be holding a flint stone.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_flint_fire ) ) {
        obj = create_object( get_obj_index( OBJ_VNUM_FIRE ), 0 );
        act( AT_CYAN,
             "$n gathers a small pile of tinder, kindling, and wood and place them in a combustible stack.\r\n",
             ch, NULL, NULL, TO_ROOM );
        act( AT_CYAN,
             "You gather a small pile of tinder, kindling, and wood and place them in a combustible stack.\r\n",
             ch, NULL, NULL, TO_CHAR );
        act( AT_YELLOW,
             "$n strikes $s flint to a pile of tinder, kindling, and wood starting a fire.",
             ch, NULL, NULL, TO_ROOM );
        act( AT_YELLOW,
             "You strike your flint to a pile of tinder, kindling, and wood starting a fire.",
             ch, NULL, NULL, TO_CHAR );
        obj = obj_to_room( obj, ch->in_room );
        obj->timer = 20;
        learn_from_success( ch, gsn_flint_fire );
        return;
    }
    else
        act( AT_PLAIN, "Your gathered woods are too damp to catch flame.", ch, NULL, NULL,
             TO_CHAR );
    learn_from_failure( ch, gsn_flint_fire );
    return;
}

void do_bone( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    buf[MSL];
    AFFECT_DATA            *paf;
    int                     sn = 1;
    short                   value;

    if ( can_use_skill( ch, number_percent(  ), gsn_bone ) ) {
        WAIT_STATE( ch, skill_table[gsn_bone]->beats );
        obj = create_object( get_obj_index( OBJ_VNUM_BONE ), 0 );
        obj->level = ch->level;
        value = ch->level;

        if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
            send_to_char( "You fail to summon the dagger as you cannot carry that many items.\r\n",
                          ch );
            extract_obj( obj );
            return;
        }

        /*
         * I set item damage with value2 to 1+ players level
         */
        obj->pIndexData->value[1] = set_min_chart( value );
        obj->pIndexData->value[2] = set_max_chart( value );
        obj->value[1] = set_min_chart( value );
        obj->value[2] = set_max_chart( value );

        obj->value[0] = 12;                            /* condition */
        /*
         * obj->value[1] = 2; obj->value[2] = 3+ch->level; 
         */
        obj->value[3] = 2;                             /* thrusts */
        obj->value[4] = 2;                             /* spear */
        snprintf( buf, MSL, "%s's bone dagger", ch->name );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );

/* To avoid freeing a null pointer */
        if ( obj->owner == NULL ) {
            obj->owner = STRALLOC( ch->name );
        }
        else {
            STRFREE( obj->owner );
            obj->owner = STRALLOC( ch->name );
        }
        /*
         * lets give the weapon greater power at higher levels
         */
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = sn;
        paf->duration = -1;
        paf->location = APPLY_HITROLL;                 /* can use diff applies hit, move, 
                                                        * hitroll, damroll */
        paf->modifier = 5 + ( ch->level / 20 );        /* can be whatever you like */
        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );

        if ( ch->level > 9 ) {
            CREATE( paf, AFFECT_DATA, 2 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_WEAPONSPELL;         /* can use diff applies hit, move, 
                                                        * hitroll, damroll */
            paf->modifier = skill_lookup( "drain" );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }

        if ( ch->level > 29 ) {
            CREATE( paf, AFFECT_DATA, 1 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_DAMROLL;
            paf->modifier = 1 + ( ch->level / 20 );
            xCLEAR_BITS( paf->bitvector );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }

        if ( ch->level > 49 ) {
            CREATE( paf, AFFECT_DATA, 4 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_WEAPONSPELL;         /* can use diff applies hit, move, 
                                                        * hitroll, damroll */
            paf->modifier = skill_lookup( "energy drain" );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }
        obj->timer = ( ch->level / 2 ) + 15;           /* object duration */
        act( AT_YELLOW, "A bone dagger materializes into $n's hands.", ch, NULL, NULL, TO_ROOM );
        act( AT_YELLOW, "Your bone dagger materializes into your hands.", ch, NULL, NULL, TO_CHAR );
        obj = obj_to_char( obj, ch );
        save_char_obj( ch );
        learn_from_success( ch, gsn_bone );
        return;
    }
    else
        act( AT_PLAIN, "You cannot concentrate enough to summon a bone dagger.", ch, NULL,
             NULL, TO_CHAR );
    learn_from_failure( ch, gsn_bone );
    return;

}

void do_judge( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    buf[MSL];
    AFFECT_DATA            *paf;
    int                     sn = 1;
    short                   value;

    if ( can_use_skill( ch, number_percent(  ), gsn_judge ) ) {
        WAIT_STATE( ch, skill_table[gsn_judge]->beats );
        obj = create_object( get_obj_index( OBJ_VNUM_JUDGE ), 0 );

        if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
            send_to_char( "You fail to summon the spear as you cannot carry that many items.\r\n",
                          ch );
            extract_obj( obj );
            return;
        }

        /*
         * I set item damage with value2 to 1+ players level
         */
        obj->level = ch->level;
        value = ch->level;
        obj->value[0] = 12;                            /* condition */
        obj->pIndexData->value[1] = set_min_chart( value );
        obj->pIndexData->value[2] = set_max_chart( value );
        obj->value[1] = set_min_chart( value );
        obj->value[2] = set_max_chart( value );
        obj->value[3] = 7;                             /* thrusts */
        obj->value[4] = 10;                            /* spear */
        snprintf( buf, MSL, "%s's Spear of Judgement", ch->name );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );
        obj->weight = 10;
/* To avoid freeing a null pointer */
        if ( obj->owner == NULL ) {
            obj->owner = STRALLOC( ch->name );
//xSET_BIT(obj->extra_flags, ITEM_PERSONAL);
        }
        else {
            STRFREE( obj->owner );
            obj->owner = STRALLOC( ch->name );
//xSET_BIT(obj->extra_flags, ITEM_PERSONAL);
        }

        /*
         * lets give the weapon greater power at higher levels
         */
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = sn;
        paf->duration = -1;
        paf->location = APPLY_HITROLL;                 /* can use diff applies hit, move,
                                                        * hitroll, damroll */
        paf->modifier = 5 + ( ch->level / 20 );        /* can be whatever you like */
        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );

        if ( ch->level > 19 ) {
            CREATE( paf, AFFECT_DATA, 2 );

            paf->type = -1;
            paf->duration = -1;
            paf->location = APPLY_STR;
            paf->modifier = 2;                         /* can be whatever you like */
            xCLEAR_BITS( paf->bitvector );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }

        if ( ch->level > 29 ) {
            CREATE( paf, AFFECT_DATA, 1 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_DAMROLL;
            paf->modifier = 1 + ( ch->level / 20 );
            xCLEAR_BITS( paf->bitvector );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }
        if ( ch->level > 49 ) {
            CREATE( paf, AFFECT_DATA, 4 );

            paf->type = sn;
            paf->level = 5;
            paf->duration = -1;
            paf->location = APPLY_WEAPONSPELL;
            paf->modifier = skill_lookup( "torrent" );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }

        obj->timer = ch->level + 20;                   /* object duration */
        act( AT_YELLOW, "A spear of judgement materializes into $n's hands.", ch, NULL,
             NULL, TO_ROOM );
        act( AT_YELLOW, "Your spear of judgement materializes into your hands.", ch, NULL,
             NULL, TO_CHAR );
        obj = obj_to_char( obj, ch );
        learn_from_success( ch, gsn_judge );
        return;
    }
    else
        act( AT_PLAIN, "You cannot concentrate enough to summon a spear of judgement.", ch,
             NULL, NULL, TO_CHAR );
    learn_from_failure( ch, gsn_judge );
    return;

}

void do_extract_skeleton( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *korps;
    OBJ_DATA               *corpse;
    OBJ_DATA               *obj;
    OBJ_DATA               *skin;
    bool                    found;
    char                   *name;
    char                    buf[MSL];

    found = FALSE;

    if ( argument[0] == '\0' ) {
        send_to_char( "Whose corpse do you wish to skin?\r\n", ch );
        return;
    }
    if ( ( corpse = get_obj_here( ch, argument ) ) == NULL ) {
        send_to_char( "You cannot find that here.\r\n", ch );
        return;
    }
    if ( corpse->cost == ( -( int ) MOB_VNUM_LSKELLIE ) ) {
        send_to_char( "There is no skeleton to extract from a skeleton.\r\n", ch );
        return;
    }
    if ( corpse->value[6] == 1 ) {
        send_to_char( "The skeleton has already been extracted from that corpse.\r\n", ch );
        return;
    }
    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You have no weapon with which to perform this deed.\r\n", ch );
        return;
    }
    if ( corpse->item_type != ITEM_CORPSE_PC && corpse->item_type != ITEM_CORPSE_NPC ) {
        send_to_char( "You can extract a skeleton from corpses.\r\n", ch );
        return;
    }
    if ( get_obj_index( OBJ_VNUM_SKELLY ) == NULL ) {
        bug( "%s", "Vnum 41009 (OBJ_VNUM_SKELLY) not found for do_extract skeleton!" );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_extract_skeleton ) ) {
        WAIT_STATE( ch, skill_table[gsn_judge]->beats );

        korps = create_object( get_obj_index( OBJ_VNUM_CORPSE_PC ), 0 );
        skin = create_object( get_obj_index( OBJ_VNUM_SKELLY ), 0 );
        make_blood( ch );
        name = IS_NPC( ch ) ? korps->short_descr : corpse->short_descr;
        snprintf( buf, MSL, skin->short_descr, name );
        if ( VLD_STR( skin->short_descr ) )
            STRFREE( skin->short_descr );
        skin->short_descr = STRALLOC( buf );
        corpse->value[6] = 1;
        snprintf( buf, MSL, skin->description, name );
        if ( VLD_STR( skin->description ) )
            STRFREE( skin->description );
        skin->description = STRALLOC( buf );
        skin->timer = 10;
        act( AT_BLOOD, "$n strips the skin from $p exposing the skeleton.", ch, corpse,
             NULL, TO_ROOM );
        act( AT_BLOOD, "You strip the skin from $p exposing the skeleton.", ch, corpse,
             NULL, TO_CHAR );
        obj_to_char( skin, ch );
        learn_from_success( ch, gsn_extract_skeleton );
        return;
    }
    else
        act( AT_BLOOD, "You do not have the skill to wholly extract the skeleton.", ch,
             NULL, NULL, TO_CHAR );
    learn_from_failure( ch, gsn_extract_skeleton );
    return;
}

void do_conjure_elemental( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    AFFECT_DATA             af;
    char                    buf[MSL];
    char                   *name;
    bool                    found;
    short                   chance;

    chance = number_range( 1, 4 );

    if ( IS_NPC( ch ) && ch->secondclass == 2 ) {
        return;
    }

    if ( ( temp = get_mob_index( MOB_VNUM_LSKELLIE ) ) == NULL ) {
        bug( "Skill_Conjure Elemental: Elemental vnum %d doesn't exist.", MOB_VNUM_LSKELLIE );
        return;
    }
    if ( !IS_NPC( ch ) ) {
        found = FALSE;
        for ( mob = first_char; mob; mob = mob->next ) {
            if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
                found = TRUE;
                break;
            }
        }
        if ( xIS_SET( ch->act, PLR_BOUGHT_PET ) && found == TRUE ) {
            send_to_char( "You already have a pet, dismiss it first.\r\n", ch );
            return;
        }
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_conjure_elemental ) ) {
        WAIT_STATE( ch, skill_table[gsn_conjure_elemental]->beats );
        mob = create_mobile( temp );
        char_to_room( mob, ch->in_room );
        mob->level = ch->level;
        mob->hit = set_hp( mob->level );
        mob->max_hit = set_hp( mob->level );
        mob->hit = mob->hit * 2;
        mob->max_hit = mob->max_hit * 2;
        mob->armor = set_armor_class( mob->level );
        mob->hitroll = set_hitroll( mob->level );
        mob->damroll = set_damroll( mob->level );
        if ( mob->hitroll || mob->damroll > 50 ) {
            mob->hitroll = 50;
            mob->damroll = 50;
        }
        mob->numattacks = set_num_attacks( mob->level );
        if ( mob->numattacks > 4 ) {
            mob->numattacks = 4;
        }
        mob->hitplus = set_hp( mob->level );
        mob->alignment = ch->alignment;
        if ( chance == 1 ) {
            mudstrlcpy( buf, "fire elemental", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a fire elemental" );
            snprintf( buf, MSL, "A fire elemental stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
        }
        if ( chance == 2 ) {
            mudstrlcpy( buf, "earth elemental", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a earth elemental" );
            snprintf( buf, MSL, "A earth elemental stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
        }
        if ( chance == 3 ) {
            mudstrlcpy( buf, "air elemental", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a air elemental" );
            snprintf( buf, MSL, "A air elemental stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
        }
        if ( chance == 4 ) {
            mudstrlcpy( buf, "water elemental", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a water elemental" );
            snprintf( buf, MSL, "A water elemental stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
        }
        act( AT_CYAN, "You call upon the elements of the fabric of creation.\r\n", ch,
             NULL, NULL, TO_CHAR );
        if ( chance == 1 ) {
            act( AT_YELLOW,
                 "\r\nYour fire elemental that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_YELLOW,
                 "$n utters some arcane words, and a fire elemental suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 2 ) {
            act( AT_ORANGE,
                 "\r\nYour earth elemental that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_ORANGE,
                 "$n utters some arcane words, and a earth elemental suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 3 ) {
            act( AT_WHITE,
                 "\r\nYour air elemental that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_WHITE,
                 "$n utters some arcane words, and a air elemental suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 4 ) {
            act( AT_BLUE,
                 "\r\nYour water elemental that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_BLUE,
                 "$n utters some arcane words, and a water elemental suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( !IS_NPC( ch ) ) {
            add_follower( mob, ch );
            xSET_BIT( ch->act, PLR_BOUGHT_PET );
            xSET_BIT( mob->act, ACT_PET );
            xSET_BIT( mob->affected_by, AFF_CHARM );
            learn_from_success( ch, gsn_conjure_elemental );
            ch->pcdata->charmies++;
            ch->pcdata->pet = mob;
        }
        xSET_BIT( mob->affected_by, AFF_TRUESIGHT );
        if ( IS_NPC( ch ) ) {
            ch->secondclass = 2;
            xSET_BIT( mob->act, ACT_AGGRESSIVE );
        }

        return;
    }
    else {
        act( AT_CYAN, "You utter some arcane words.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n utters some arcane words.\r\n", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You speak an incorrect phrase and fail to conjure an elemental.\r\n", ch );
        learn_from_failure( ch, gsn_conjure_elemental );
        return;
    }
    return;
}

void do_animal_companion( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    AFFECT_DATA             af;
    char                    buf[MSL];
    char                   *name;
    bool                    found;
    short                   chance;

    chance = number_range( 1, 11 );                    /* Changed by Aurin */
/* What does this do, anyways? heh Aurin 10/7/2010 
   if ( IS_NPC( ch ) && ch->secondclass == 2 )
   {
   return;
   }
*/

    if ( ( temp = get_mob_index( MOB_VNUM_LSKELLIE ) ) == NULL ) {
        bug( "Skill_Call Companion: Companion vnum %d doesn't exist.", MOB_VNUM_LSKELLIE );
        return;
    }
    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }

    if ( xIS_SET( ch->act, PLR_BOUGHT_PET ) && found == TRUE ) {
        send_to_char( "You already have a pet, dismiss it first.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_animal_companion ) ) {
        WAIT_STATE( ch, skill_table[gsn_animal_companion]->beats );
        mob = create_mobile( temp );
        char_to_room( mob, ch->in_room );
        mob->level = ch->level;
        mob->hit = set_hp( mob->level );
        mob->max_hit = set_hp( mob->level );
        mob->hit = mob->hit * 2;
        mob->max_hit = mob->max_hit * 2;
        mob->armor = set_armor_class( mob->level );
        mob->hitroll = set_hitroll( mob->level );
        mob->damroll = set_damroll( mob->level );
        if ( mob->hitroll || mob->damroll > 50 ) {
            mob->hitroll = 50;
            mob->damroll = 50;
        }
        mob->numattacks = set_num_attacks( mob->level );
        if ( mob->numattacks > 4 ) {
            mob->numattacks = 4;
        }
        mob->hitplus = set_hp( mob->level );
        mob->alignment = ch->alignment;
        if ( chance == 1 ) {
            mudstrlcpy( buf, "dire wolf", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a dire wolf" );
            snprintf( buf, MSL, "A dire wolf stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 65;                            /* Aurin */
        }
        if ( chance == 2 ) {
            mudstrlcpy( buf, "black bear", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a black bear" );
            snprintf( buf, MSL, "A black bear stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 22;                            /* Aurin */
        }
        if ( chance == 3 ) {
            mudstrlcpy( buf, "panther", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a panther" );
            snprintf( buf, MSL, "A panther stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 27;                            /* Aurin */
        }
/* Added for variety's sake -- Aurin 10/2/2010 */
        if ( chance == 4 ) {
            mudstrlcpy( buf, "anaconda", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "an anaconda" );
            snprintf( buf, MSL, "An anaconda slithers along the ground here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 58;
        }
        if ( chance == 5 ) {
            mudstrlcpy( buf, "giant worm", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a giant worm" );
            snprintf( buf, MSL, "A giant worm stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 66;
        }
        if ( chance == 6 ) {
            mudstrlcpy( buf, "mountain lion", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a mountain lion" );
            snprintf( buf, MSL, "A mountain lion stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 91;
        }
        if ( chance == 7 ) {
            mudstrlcpy( buf, "eagle", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "an eagle" );
            snprintf( buf, MSL, "An eagle stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 87;
        }
        if ( chance == 8 ) {
            mudstrlcpy( buf, "wolverine", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a wolverine" );
            snprintf( buf, MSL, "A wolverine stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 90;
        }
        if ( chance == 9 ) {
            mudstrlcpy( buf, "beaver", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a beaver" );
            snprintf( buf, MSL, "A beaver stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 89;
        }
        if ( chance == 10 ) {
            mudstrlcpy( buf, "shrew", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a shrew" );
            snprintf( buf, MSL, "A shrew stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 54;
        }
        if ( chance == 11 ) {
            mudstrlcpy( buf, "badger", MSL );
            STRFREE( mob->name );
            mob->name = STRALLOC( buf );
            if ( VLD_STR( mob->short_descr ) )
                STRFREE( mob->short_descr );
            mob->short_descr = STRALLOC( "a badger" );
            snprintf( buf, MSL, "A badger stands here.\r\n" );
            if ( VLD_STR( mob->description ) )
                STRFREE( mob->long_descr );
            mob->long_descr = STRALLOC( buf );
            mob->race = 88;
        }
/* End of addition by Aurin */

        act( AT_CYAN, "You let your senses expand through your surroundings.\r\n", ch,
             NULL, NULL, TO_CHAR );
        act( AT_GREEN, "You sense the mind of an animal and call it for your companion.",
             ch, NULL, NULL, TO_CHAR );

/* More additions by Aurin to increase variety -- 10/2/2010 */
        if ( chance == 11 ) {
            act( AT_CYAN,
                 "\r\nYour badger that you called suddenly appears and comes to your side.\r\n", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a badger suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 10 ) {
            act( AT_CYAN,
                 "\r\nYour shrew that you called suddenly appears and comes to your side.\r\n", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a shrew suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 9 ) {
            act( AT_CYAN,
                 "\r\nYour beaver that you called suddenly appears and comes to your side.\r\n", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a beaver suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 8 ) {
            act( AT_CYAN,
                 "\r\nYour wolverine that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a wolverine suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 7 ) {
            act( AT_CYAN,
                 "\r\nYour eagle that you called suddenly appears and comes to your side.\r\n", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and an eagle suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 6 ) {
            act( AT_CYAN,
                 "\r\nYour mountain lion that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a mountain lion suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 5 ) {
            act( AT_CYAN,
                 "\r\nYour giant worm that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a giant worm suddenly appears and goes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 4 ) {
            act( AT_CYAN,
                 "\r\nYour anaconda that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and an anaconda suddenly appears and goes to $s side.\r\\n",
                 ch, NULL, NULL, TO_ROOM );
        }
/* End of addition by Aurin */

        if ( chance == 3 ) {
            act( AT_CYAN,
                 "\r\nYour panther that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a panther suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 2 ) {
            act( AT_CYAN,
                 "\r\nYour black bear that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a black bear suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }
        if ( chance == 1 ) {
            act( AT_CYAN,
                 "\r\nYour dire wolf that you called suddenly appears and comes to your side.\r\n",
                 ch, NULL, NULL, TO_CHAR );
            act( AT_CYAN,
                 "$n calls out in a druidic dialect, and a dire wolf suddenly appears, and comes to $s side.\r\n",
                 ch, NULL, NULL, TO_ROOM );
        }

        if ( IS_NPC( ch ) ) {
            ch->secondclass = 2;
            xSET_BIT( mob->act, ACT_AGGRESSIVE );
        }

        if ( !IS_NPC( ch ) ) {
            add_follower( mob, ch );
            xSET_BIT( ch->act, PLR_BOUGHT_PET );
            xSET_BIT( mob->act, ACT_PET );
            xSET_BIT( mob->affected_by, AFF_CHARM );
            learn_from_success( ch, gsn_animal_companion );
            ch->pcdata->charmies++;
            ch->pcdata->pet = mob;
        }
        xSET_BIT( mob->affected_by, AFF_TRUESIGHT );
        return;
    }
    else {
        act( AT_CYAN, "You call out in a druidic dialect.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n calls out in a druidic dialect.\r\n", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You speak a incorrect phrase and fail to call companion.\r\n", ch );
        learn_from_failure( ch, gsn_animal_companion );
        return;
    }
    return;
}

void do_lesser_skeleton( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    AFFECT_DATA             af;
    char                    buf[MSL];
    OBJ_DATA               *skellie;
    char                   *name;
    bool                    found;

    if ( IS_NPC( ch ) && ch->secondclass == 2 ) {
        return;
    }

    if ( ( temp = get_mob_index( MOB_VNUM_LSKELLIE ) ) == NULL ) {
        bug( "Skill_Lesser Skelton: Skeleton vnum %d doesn't exist.", MOB_VNUM_LSKELLIE );
        return;
    }

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( xIS_SET( ch->act, PLR_BOUGHT_PET ) && found == TRUE ) {
            send_to_char( "You already have a pet, dismiss it first.\r\n", ch );
            return;
        }

        if ( argument[0] == '\0' ) {
            send_to_char( "Whose skeleton do you wish to use lesser skeleton on?\r\n", ch );
            return;
        }
        if ( ( skellie = get_obj_here( ch, argument ) ) == NULL ) {
            send_to_char( "You cannot find that here.\r\n", ch );
            return;
        }

        if ( skellie->item_type != ITEM_SKELETON ) {
            send_to_char( "You can only use this on skeletons.\r\n", ch );
            return;
        }
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_lesser_skeleton ) ) {
        WAIT_STATE( ch, skill_table[gsn_lesser_skeleton]->beats );
        if ( !IS_NPC( ch ) ) {
            extract_obj( skellie );
        }
        mob = create_mobile( temp );
        char_to_room( mob, ch->in_room );
        mob->level = ch->level;
        mob->hit = set_hp( mob->level );
        mob->max_hit = set_hp( mob->level );
        mob->hit = mob->hit * 2;
        mob->max_hit = mob->max_hit * 2;
        mob->armor = set_armor_class( mob->level );
        mob->hitroll = set_hitroll( mob->level );
        mob->damroll = set_damroll( mob->level );
        if ( mob->hitroll || mob->damroll > 50 ) {
            mob->hitroll = 50;
            mob->damroll = 50;
        }
        mob->numattacks = set_num_attacks( mob->level );
        if ( mob->numattacks > 4 ) {
            mob->numattacks = 4;
        }
        mob->hitplus = set_hp( mob->level );
        mob->alignment = ch->alignment;
        mudstrlcpy( buf, "Lesser Skeleton", MSL );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );

        if ( VLD_STR( mob->short_descr ) )
            STRFREE( mob->short_descr );
        mob->short_descr = STRALLOC( "a lesser skeleton" );

        snprintf( buf, MSL, "A lesser skeleton stands here.\r\n" );
        if ( VLD_STR( mob->description ) )
            STRFREE( mob->long_descr );
        mob->long_descr = STRALLOC( buf );
        act( AT_CYAN, "You surround a skeleton with arcane symbols.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n surrounds a skeleton with arcane symbols.\r\n", ch, NULL, NULL, TO_ROOM );
        act( AT_LBLUE, "You begin speaking in forgotten tongues.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_LBLUE, "$n speaks in forgotten tongues.\r\n", ch, NULL, NULL, TO_ROOM );
        act( AT_MAGIC,
             "Your Lesser Skeleton glows as sinew, and exposed organs grow within it.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_MAGIC,
             "$n's Lesser Skeleton glows as sinew, and exposed organs grow within it!", ch,
             NULL, NULL, TO_ROOM );
        act( AT_GREEN, "The Lesser Skeleton suddenly stands up.", ch, NULL, NULL, TO_CHAR );
        act( AT_GREEN, "The Lesser Skeleton suddenly stands up!", ch, NULL, NULL, TO_ROOM );
        if ( !IS_NPC( ch ) ) {
            add_follower( mob, ch );
            xSET_BIT( ch->act, PLR_BOUGHT_PET );
            xSET_BIT( mob->act, ACT_PET );
            xSET_BIT( mob->affected_by, AFF_CHARM );
            learn_from_success( ch, gsn_lesser_skeleton );
            ch->pcdata->charmies++;
            ch->pcdata->pet = mob;
        }
        if ( IS_NPC( ch ) ) {
            xSET_BIT( mob->act, ACT_AGGRESSIVE );
            ch->secondclass = 2;
        }
        xSET_BIT( mob->affected_by, AFF_TRUESIGHT );
        return;
    }
    else {
        act( AT_CYAN, "You surround a skeleton with arcane symbols.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n surrounds a skeleton with arcane symbols.\r\n", ch, NULL, NULL, TO_ROOM );
        act( AT_LBLUE, "You begin speaking in forgotten tongues.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_LBLUE, "$n speaks in forgotten tongues.\r\n", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You speak an incorrect phrase and do not animate the skeleton.\r\n", ch );
        learn_from_failure( ch, gsn_lesser_skeleton );
        return;
    }
    return;
}

void do_passage( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    AFFECT_DATA             af;
    char                    buf[MSL];

    if ( ( temp = get_mob_index( MOB_VNUM_PASSAGE ) ) == NULL ) {
        bug( "do_passage: Passage Demon vnum %d doesn't exist.", MOB_VNUM_PASSAGE );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_passage )
         && ( ( get_curr_cha( ch ) / 3 ) + 1 ) > ch->pcdata->charmies ) {
        WAIT_STATE( ch, skill_table[gsn_passage]->beats );

        mob = create_mobile( temp );
        char_to_room( mob, ch->in_room );
        mob->level = ch->level - 5;
        mob->hit = set_hp( mob->level - 5 );
        mob->max_hit = set_hp( mob->level - 5 );
        mob->armor = set_armor_class( mob->level - 5 );
        mob->hitroll = set_hitroll( mob->level - 5 );
        mob->damroll = set_damroll( mob->level - 5 );
        mob->numattacks = set_num_attacks( mob->level - 5 );;
        mob->hitplus = set_hp( mob->level - 5 );
        mob->alignment = ch->alignment;
        mudstrlcpy( buf, "A demon slave", MSL );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );
        snprintf( buf, MSL, "The demon slave of %s", ch->name );
        STRFREE( mob->short_descr );
        mob->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "A demon slave of %s struggles to do its own bidding.\r\n", ch->name );
        STRFREE( mob->long_descr );
        mob->long_descr = STRALLOC( buf );
        act( AT_PLAIN, "You draw out a pentagram on the ground.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_PLAIN, "$n draws out a pentagram on the ground.\r\n", ch, NULL, NULL, TO_ROOM );
        act( AT_BLUE, "You begin speaking in forgotten tongues.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_BLUE, "$n speaks in forgotten tongues.\r\n", ch, NULL, NULL, TO_ROOM );
        act( AT_RED, "You create a glowing passageway that $N steps through.", ch, NULL,
             mob, TO_CHAR );
        act( AT_RED, "$n creates a glowing passageway that $N steps through!", ch, NULL,
             mob, TO_ROOM );
        add_follower( mob, ch );
        af.type = gsn_passage;
        af.duration = ( ch->level * 3 );
        af.location = 0;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_CHARM );
        xSET_BIT( mob->affected_by, AFF_TRUESIGHT );
        affect_to_char( mob, &af );
        learn_from_success( ch, gsn_passage );
        if ( !IS_NPC( ch ) )
            ch->pcdata->charmies++;
        return;
    }
    else {
        act( AT_PLAIN, "You draw out a pentagram on the ground.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_PLAIN, "$n draws out a pentagram on the ground.\r\n", ch, NULL, NULL, TO_ROOM );
        act( AT_BLUE, "You begin speaking in forgotten tongues.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_BLUE, "$n speaks in forgotten tongues.\r\n", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You speak an incorrect phrase and do not create a passageway.\r\n", ch );
        learn_from_failure( ch, gsn_passage );
        return;
    }
    return;
}

//Just the skeleton for this function, going to handle gathering true hp. -Taon
//THIS IS NOWHERE NEAR READY FOR USE, just getting started. -Taon
int base_hp( CHAR_DATA *ch )
{
    int                     base_amt;
    OBJ_DATA               *obj;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( !obj )
            break;
    }

    return base_amt;
}

void do_feign_death( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "You have already feigned death recently.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_feign_death ) ) {
        do_land( ch, ( char * ) "" );
        WAIT_STATE( ch, skill_table[gsn_feign_death]->beats );
        act( AT_CYAN, "You feign death by gasping, and suddenly falling to the ground.",
             ch, NULL, NULL, TO_CHAR );
        act( AT_RED, "$n is DEAD!!\r\n", ch, NULL, NULL, TO_NOTVICT );
        act( AT_BLOOD,
             "$n gasps $s last breath and blood spurts out of $s mouth and ears.", ch,
             NULL, NULL, TO_NOTVICT );
        stop_fighting( ch, TRUE );
        interpret( ch, ( char * ) "sleep" );
        af.type = gsn_feign_death;
        af.duration = ch->level * 5;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_FEIGN );
        affect_to_char( ch, &af );
        learn_from_success( ch, gsn_feign_death );
    }
    else {
        act( AT_CYAN, "You try to feign death, but don't fool your enemy!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_RED, "$n falls to the ground suddenly, pretending to be dead.", ch, NULL,
             NULL, TO_NOTVICT );
        learn_from_failure( ch, gsn_feign_death );
    }
    return;
}

void do_burrow( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Syntax: burrow up\r\n", ch );
        send_to_char( "Syntax: burrow down\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "You would need to land first.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "up" ) && !IS_AFFECTED( ch, AFF_BURROW ) ) {
        send_to_char( "You can't burrow up if you're not burrowed in the ground.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_burrow ) ) {
        WAIT_STATE( ch, skill_table[gsn_burrow]->beats );

        if ( !str_cmp( arg, "up" ) ) {
            if ( IS_AFFECTED( ch, AFF_BURROW ) ) {
                affect_strip( ch, gsn_burrow );
                xREMOVE_BIT( ch->affected_by, AFF_BURROW );
                affect_strip( ch, gsn_hide );
                xREMOVE_BIT( ch->affected_by, AFF_HIDE );
                set_position( ch, POS_STANDING );
                act( AT_ORANGE, "You burrow up out of the cold ground!", ch, NULL, NULL, TO_CHAR );
                act( AT_ORANGE, "$n burrows up out of the ground.", ch, NULL, NULL, TO_NOTVICT );
                learn_from_success( ch, gsn_burrow );
                return;
            }
        }
        if ( !str_cmp( arg, "down" ) ) {
            if ( IS_AFFECTED( ch, AFF_BURROW ) ) {
                send_to_char( "You are already burrowed into the ground.\r\n", ch );
                return;
            }

            switch ( ch->in_room->sector_type ) {
                case SECT_ROAD:
                case SECT_VROAD:
                case SECT_HROAD:
                case SECT_INSIDE:
                    send_to_char( "The floor is too hard to burrow through.\r\n", ch );
                    return;
                case SECT_AIR:
                    send_to_char( "What?  In the air?!\r\n", ch );
                    return;
            }

            act( AT_ORANGE, "You burrow down into the cold ground!", ch, NULL, NULL, TO_CHAR );
            act( AT_ORANGE, "$n burrows down into the ground.", ch, NULL, NULL, TO_NOTVICT );
            do_sleep( ch, ( char * ) "" );
            if ( !IS_AFFECTED( ch, AFF_BURROW ) ) {
                af.type = gsn_burrow;
                af.duration = 1000;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.level = ch->level;
                af.bitvector = meb( AFF_BURROW );
                affect_to_char( ch, &af );
                af.type = gsn_hide;
                af.duration = 1000;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = meb( AFF_HIDE );
                af.level = ch->level;
                affect_to_char( ch, &af );
            }
            learn_from_success( ch, gsn_burrow );
            return;
        }
        else {
            send_to_char( "Syntax: burrow up\r\n", ch );
            send_to_char( "Syntax: burrow down\r\n", ch );
            return;
        }
    }
    else
        send_to_char( "Your burrowing is halted due to finding a rock.\r\n", ch );
    learn_from_failure( ch, gsn_burrow );
    return;
}

void do_telepathy( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MIL];
    CHAR_DATA              *victim;
    int                     position;
    CHAR_DATA              *switched_victim = NULL;

#ifndef SCRAMBLE
    int                     speaking = -1,
        lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
        if ( ch->speaking & lang_array[lang] ) {
            speaking = lang;
            break;
        }
#endif

    if ( !IS_NPC( ch )
         && ( xIS_SET( ch->act, PLR_SILENCE ) || xIS_SET( ch->act, PLR_NO_TELL ) ) ) {
        send_to_char( "You can't do that.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "Syntax: Telepathy < victim > < message >\r\n", ch );
        return;
    }
    MOB_INDEX_DATA         *pMobIndex;

    if ( ( victim = get_char_world( ch, arg ) ) == NULL
         || ( IS_NPC( victim ) && ( victim->in_room != ch->in_room )
              && ( pMobIndex = get_mob_index( MOB_VNUM_MARLENA ) ) == NULL )
         || ( !NOT_AUTHED( ch ) && NOT_AUTHED( victim ) && !IS_IMMORTAL( ch ) ) )
    {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "You have a nice little chat with yourself.\r\n", ch );
        return;
    }

    if ( IS_IMMORTAL( victim ) && !IS_IMMORTAL( ch ) && !IS_NPC( ch ) ) {
        send_to_char
            ( "          &YThe STAFF at &r6 Dragons&Y do not accept tells from players.\r\n", ch );
        send_to_char
            ( "&YIf you would like to talk with a staff member, please use the ask channel.\r\n",
              ch );
        send_to_char
            ( "&YProper Etiquette : ask <staffname> will you send me a tell please?\r\n", ch );
        send_to_char
            ( "&Y              -or- ask <staffname> Can I please see you in private?\r\n", ch );
        send_to_char( "                   &C[&Y The decision of STAFF is final.&C ]\r\n", ch );
        send_to_char
            ( "&YIf you still wish to contact that person write a gnote using the personal board.\r\n",
              ch );
        return;
    }

    if ( NEW_AUTH( ch ) && !NEW_AUTH( victim ) && !IS_IMMORTAL( victim ) ) {
        send_to_char( "They can't hear you because you are not authorized.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched ) && ( get_trust( ch ) > LEVEL_AVATAR )
         && !IS_AFFECTED( victim->switched, AFF_POSSESS ) ) {
        send_to_char( "That player is switched.\r\n", ch );
        return;
    }

    else if ( !IS_NPC( victim ) && ( victim->switched )
              && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
        switched_victim = victim->switched;

    else if ( !IS_NPC( victim ) && ( !victim->desc ) ) {
        send_to_char( "That player is link-dead.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) ) {
        send_to_char( "That player is afk.\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->deaf, CHANNEL_TELEPATHY )
         && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) ) {
        act( AT_PLAIN, "$E has $S telepathy turned off.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
        send_to_char
            ( "That player is silenced.  They will receive your message but can not respond.\r\n",
              ch );

    if ( ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) ) ) {
        act( AT_PLAIN, "$E is too tired to discuss such matters with you now.", ch, 0,
             victim, TO_CHAR );
        return;
    }

    if ( victim->desc                                  /* make sure desc exists first
                                                        * -Thoric */
         && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_AJ_CPL ) {
        act( AT_PLAIN,
             "$E is currently in a writing buffer.  Please try again in a few minutes.",
             ch, 0, victim, TO_CHAR );
        return;
    }

    /*
     * Check to see if target of tell is ignoring the sender
     */
    if ( is_ignoring( victim, ch ) ) {
        /*
         * If the sender is an imm then they cannot be ignored
         */
        if ( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) ) {
            set_char_color( AT_IGNORE, ch );
            ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
            return;
        }
        else {
            set_char_color( AT_IGNORE, victim );
            ch_printf( victim, "You attempt to ignore %s, but " "are unable to do so.\r\n",
                       PERS( ch, victim ) );
        }
    }

    ch->retell = victim;

    if ( switched_victim )
        victim = switched_victim;

    if ( can_use_skill( ch, number_percent(  ), gsn_telepathy ) ) {
        MOBtrigger = FALSE;
        act( AT_BLOOD, "You send mentally to $N '$t'", ch, argument, victim, TO_CHAR );
        learn_from_success( ch, gsn_telepathy );
        position = victim->position;
        set_position( victim, POS_STANDING );
    }
    else {
        send_to_char( "\r\nYou lose your concentration.\r\n", ch );
        return;
    }
    if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) ) {
        int                     speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                                                   knows_language( ch, ch->speaking, victim ) );

        if ( speakswell < 85 )
            act( AT_BLOOD, "$n speaks in your mind '$t'", ch,
                 translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
        else
            act( AT_BLOOD, "$n speaks in your mind '$t'", ch, argument, victim, TO_VICT );
    }
    else
        act( AT_BLOOD, "$n speaks in yoour mind '$t'", ch, argument, victim, TO_VICT );

    MOBtrigger = TRUE;

    set_position( victim, position );
    victim->reply = ch;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) ) {
        snprintf( buf, MIL, "%s: %s (telepathy to) %s.",
                  IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  IS_NPC( victim ) ? victim->short_descr : victim->name );
        append_to_file( LOG_FILE, buf );
    }

    mprog_speech_trigger( argument, ch );
    return;
}

void do_assault( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        set_char_color( AT_CYAN, ch );
        send_to_char( "You cannot do that here.\r\n", ch );
        return;
    }

    if ( ( get_eq_char( ch, WEAR_DUAL_WIELD ) ) == NULL ) {
        send_to_char( "You need to be dual wielding weapons to use this skill.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_assault ) ) {
        act( AT_CYAN, "You spin around suddenly with a wild assault!", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n spins around suddenly with a wild assault!", ch, NULL, NULL, TO_NOTVICT );
        WAIT_STATE( ch, skill_table[gsn_assault]->beats );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            // Bug fix, players with no targets, fail. -Taon
            if ( !vch ) {
                send_to_char( "Assault who, nobody is here.\r\n", ch );
                break;
            }

            if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                continue;

            // Bug fix, players no longer hit grouped members. -Taon
            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;

            if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) {
                global_retcode = damage( ch, vch, nominal, gsn_assault );
            }
        }
        learn_from_success( ch, gsn_assault );
        return;
    }
    else
        learn_from_failure( ch, gsn_assault );
    send_to_char( "You spin around suddenly, but miss with your wild assault.\r\n", ch );
    return;

}

void do_shriek( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        set_char_color( AT_CYAN, ch );
        send_to_char( "You cannot do that here.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_shriek ) ) {
        act( AT_CYAN, "Unfettered anguish explodes from your throat in an ear piercing wail!", ch,
             NULL, NULL, TO_CHAR );
        act( AT_CYAN, "The screams of lost souls erupt from $n, shattering all those around!", ch,
             NULL, NULL, TO_NOTVICT );
        WAIT_STATE( ch, skill_table[gsn_shriek]->beats );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            // Bug fix, players with no targets, fail. -Taon
            if ( !vch ) {
                send_to_char( "Shriek who, nobody is here.\r\n", ch );
                break;
            }

            if ( IS_NPC( vch ) && vch->pIndexData->vnum == MOB_VNUM_SOLDIERS )
                continue;

            // Bug fix, players no longer hit grouped members. -Taon
            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;

            if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) {
                global_retcode = damage( ch, vch, high, gsn_shriek );
            }
        }
        learn_from_success( ch, gsn_shriek );
        return;
    }
    else
        learn_from_failure( ch, gsn_shriek );
    send_to_char( "You fail to get enough air in your lungs to shriek.\r\n", ch );
    return;
}

void do_backhand( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Backhand whom?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    WAIT_STATE( ch, skill_table[gsn_backhand]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_backhand ) ) {
        act( AT_SKILL,
             "Looking over your right shoulder at $N....\r\nYou spin around and snap out your fist at $N backhanding $M!",
             ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n spins around suddenly backhanding you!", ch, NULL, victim, TO_VICT );
        act( AT_SKILL, "$n spins around suddenly and backhands $N!", ch, NULL, victim, TO_NOTVICT );
        learn_from_success( ch, gsn_backhand );
        global_retcode = damage( ch, victim, nominal, gsn_backhand );
    }
    else {
        act( AT_SKILL,
             "Looking over your right shoulder at $N....\r\nYou spin around and snap out your fist, but miss $N!",
             ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n spins around suddenly trying to backhand you, but misses.", ch,
             NULL, victim, TO_VICT );
        act( AT_SKILL, "$n spins around suddenly trying to backhand $N, but misses.", ch,
             NULL, victim, TO_NOTVICT );
        learn_from_failure( ch, gsn_backhand );
        global_retcode = damage( ch, victim, 0, gsn_backhand );
    }
    return;
}

void do_tackle( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Tackle whom?\r\n", ch );
        return;
    }
    // Modified check to be multi-class friendly. -Taon
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_tackle] <= 0 ) {
        send_to_char( "Better leave that to skilled fighters.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "How can you tackle yourself?\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    WAIT_STATE( ch, skill_table[gsn_tackle]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_tackle ) ) {
        act( AT_SKILL,
             "Checking your distance....\r\nYou lean forward and rush $N tackling $M!", ch,
             NULL, victim, TO_CHAR );
        act( AT_SKILL,
             "$n rushes forward at you enveloping you in $s arms tackling you to the ground!",
             ch, NULL, victim, TO_VICT );
        act( AT_SKILL, "$n rushes forward and tackles $N to the ground!", ch, NULL, victim,
             TO_NOTVICT );
        do_rest( victim, ( char * ) "" );

        learn_from_success( ch, gsn_tackle );
        global_retcode = damage( ch, victim, nominal, gsn_tackle );
    }
    else {
        act( AT_SKILL,
             "Checking your distance....\r\nYou lean forward and rush $N but miss $M completely!",
             ch, NULL, victim, TO_CHAR );
        act( AT_SKILL, "$n rushes forward at you, but you easily avoid the tackle.", ch,
             NULL, victim, TO_VICT );
        act( AT_SKILL, "$n rushes forward to try and tackle $N, but misses completely!",
             ch, NULL, victim, TO_NOTVICT );

        learn_from_failure( ch, gsn_tackle );
        global_retcode = damage( ch, victim, 0, gsn_tackle );
    }
    return;
}

//Slightly modified then given to several classes to test. -Taon
void do_maim( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    OBJ_DATA               *obj;

    if ( !IS_NPC( ch ) && !CAN_LEARN( ch, gsn_maim, 1 ) ) {
        error( ch );
        return;
    }

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Maim who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "Why would you want to hit yourself?\r\n", ch );
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL ) {
        send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
        return;
    }

    if ( obj->value[4] != WEP_1H_SHORT_BLADE ) {
        if ( ( obj->value[4] == WEP_1H_LONG_BLADE && obj->value[3] != DAM_PIERCE )
             || obj->value[4] != WEP_1H_LONG_BLADE ) {
            send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "How can you maim yourself?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_MAIM ) ) {
        send_to_char( "You already have them maimed.\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    WAIT_STATE( ch, skill_table[gsn_maim]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_maim ) ) {
        act( AT_CYAN, "You see an opening in $N, and slice a major artery!", ch, NULL, victim,
             TO_CHAR );
        act( AT_CYAN, "$n sends a sudden thrust in, and slices an artery in your leg!", ch, NULL,
             victim, TO_VICT );
        act( AT_CYAN, "$n attacks $N with a sudden thrust!", ch, NULL, victim, TO_NOTVICT );
        learn_from_success( ch, gsn_maim );
        af.type = gsn_maim;

        af.duration = ch->level / 2;
        if ( IS_SECONDCLASS( ch ) )
            victim->degree = 2;
        if ( IS_THIRDCLASS( ch ) )
            victim->degree = 3;
        if ( ch->secondclass == -1 )
            victim->degree = 1;

        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_MAIM );
        affect_to_char( victim, &af );
        global_retcode = damage( ch, victim, medium, gsn_maim );
    }
    else {
        act( AT_CYAN, "You try to slice $N's artery, but miss.", ch, NULL, victim, TO_CHAR );
        learn_from_failure( ch, gsn_maim );
        global_retcode = damage( ch, victim, 0, gsn_maim );
    }
}

void do_remember( CHAR_DATA *ch, char *argument )
{

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE )
         || IS_SET( ch->in_room->room_flags, ROOM_ARENA )
         || IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
        send_to_char( "&cA mind blazing headache prevents you from remembering this place.&D\r\n",
                      ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_remember]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_remember ) ) {
        act( AT_CYAN, "You begin focusing on remembering every detail of your location.", ch, NULL,
             NULL, TO_CHAR );
        act( AT_CYAN,
             "$n has a far away look in $s eyes, as $e slowly turns looking at everything around.",
             ch, NULL, NULL, TO_NOTVICT );
        learn_from_success( ch, gsn_remember );
        if ( !IS_IMMORTAL( ch ) )
            ch->pcdata->tmproom = ch->in_room->vnum;
    }
    else {
        act( AT_CYAN, "You attempt to remember this location, but get distracted.", ch, NULL, NULL,
             TO_CHAR );
        learn_from_failure( ch, gsn_remember );
    }
}

void do_hallowed_blow( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a hallowed blow against whom?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_hallowed_blow] <= 0 ) {
        send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_hallowed_blow]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_hallowed_blow ) ) {
        int                     dam;

        act( AT_CYAN, "You call upon hallowed energy!\r\n\r\nYou extend your arms towards $N!\r\n",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "$n extends $s arms at you.\r\n&Y$n shoots hallowed energy at you!", ch, NULL,
             victim, TO_VICT );
        act( AT_CYAN, "$n extends $s arms at $N.\r\n&Y$n shoots hallowed energy at $N!", ch, NULL,
             victim, TO_NOTVICT );
        learn_from_success( ch, gsn_hallowed_blow );
        if ( ch->secondclass == -1 ) {
            dam = ludicrous + number_range( 100, 140 );
            global_retcode = damage( ch, victim, dam, gsn_hallowed_blow );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
            dam = ludicrous;
            global_retcode = damage( ch, victim, dam, gsn_hallowed_blow );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
            dam = ludicrous - ( ch->level );
            global_retcode = damage( ch, victim, dam, gsn_hallowed_blow );
        }

    }
    else {
        act( AT_CYAN, "You try to harness your hallowed energy, but are too weary.",
             ch, NULL, victim, TO_CHAR );
        learn_from_failure( ch, gsn_hallowed_blow );
        global_retcode = damage( ch, victim, 0, gsn_hallowed_blow );
    }
    return;
}

void do_righteous_blow( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a righteous blow against whom?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    // Modified this check to make it more multi-class friendly. -Taon
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_righteous_blow] <= 0 ) {
        send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_righteous_blow]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_righteous_blow ) ) {
        act( AT_CYAN,
             "You begin to harness your power of conviction!\r\n\r\nYou extend your arms towards $N.\r\n&YA spray of lightning launches from your hands at $N!",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "$n extends $s arms at you.\r\n&Y$n shoots a spray of lightning at you!", ch,
             NULL, victim, TO_VICT );
        act( AT_CYAN,
             "$n extends $s arms at $N.\r\n&Y$n shoots a spray of lightning at $N!", ch,
             NULL, victim, TO_NOTVICT );
        learn_from_success( ch, gsn_righteous_blow );
        global_retcode =
            damage( ch, victim, extrahigh - number_range( 25, 50 ), gsn_righteous_blow );
    }
    else {
        act( AT_CYAN, "You try to harness your power of conviction, but are too weary.",
             ch, NULL, victim, TO_CHAR );
        learn_from_failure( ch, gsn_righteous_blow );
        global_retcode = damage( ch, victim, 0, gsn_righteous_blow );
    }
    return;
}

void do_smuggle( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *rch;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_HIDE ) ) {
        send_to_char( "You are already hiding.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_smuggle ) ) {
        for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
            if ( is_same_group( ch, rch ) ) {
                af.type = gsn_smuggle;
                af.duration = ch->level;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.level = ch->level;
                af.bitvector = meb( AFF_HIDE );
                affect_to_char( rch, &af );
                ch_printf( rch, "%s smuggles you to a hidden position.", ch->name );
            }
        }
        learn_from_success( ch, gsn_smuggle );
        act( AT_PLAIN, "You direct everyone to a hidden position.", ch, NULL, NULL, TO_CHAR );
    }
    else {
        learn_from_failure( ch, gsn_smuggle );
        send_to_char( "You try to smuggle everyone, but someone steps into the open.\r\n", ch );
    }
    return;
}

void do_wailing( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    AFFECT_DATA             af;
    short                   nomore;

    nomore = 40;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Wailing Haze who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "How can you haze yourself?\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_SLOW ) ) {
        send_to_char( "They have already been slowed.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "How can you use that on yourself?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_wailing ) ) {
        ch->mana = ( ch->mana - nomore );
        WAIT_STATE( ch, skill_table[gsn_wailing]->beats );
        learn_from_success( ch, gsn_wailing );
        act( AT_GREEN,
             "You point at $N, and send a green cloud of wailing haze around $M!", ch,
             NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "$n points at $N, and sends a green cloud of wailing haze around $M!", ch,
             NULL, victim, TO_NOTVICT );
        af.type = gsn_wailing;
        af.duration = 20;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_SLOW );
        affect_to_char( victim, &af );
        if ( ch->hate_level < 25 ) {
            ch->hate_level++;
        }

        if ( !victim->fighting )
            set_fighting( victim, ch );
    }
    else {
        learn_from_failure( ch, gsn_wailing );
        send_to_char( "You point at your enemy, but fail to summon the wailing haze.\r\n", ch );
    }

    return;
}

void do_mortify( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    AFFECT_DATA             af;

    one_argument( argument, arg );

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Mortify who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "How can you mortify yourself?\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char( "You cannot do that skill here.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_mortify ) ) {
        WAIT_STATE( ch, skill_table[gsn_mortify]->beats );
        learn_from_success( ch, gsn_mortify );
        act( AT_LBLUE, "You yell at $N in such a way they are mortified!", ch, NULL,
             victim, TO_CHAR );
        act( AT_LBLUE, "$n yells at $N mortifying them!", ch, NULL, victim, TO_NOTVICT );
        act( AT_CYAN,
             "\r\n$N stops fighting momentarily\r\n$N growls at you, and then attacks!",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "\r\n$N stops fighting momentarily\r\n$N growls at $n, and attacks $m!", ch,
             NULL, victim, TO_NOTVICT );

        if ( !IS_AFFECTED( ch, AFF_HATE ) ) {
            if ( ch->level < 50 ) {
                ch->hate_level = 10;
            }
            else if ( ch->level >= 50 ) {
                ch->hate_level = 20;
            }
        }
        ch->hate_level = ch->hate_level + 5;
        save_char_obj( ch );
        if ( !IS_AFFECTED( ch, AFF_HATE ) ) {
            af.type = gsn_mortify;
            af.duration = ch->level;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = meb( AFF_HATE );
            af.level = ch->level;
            affect_to_char( ch, &af );
        }
        if ( !victim->fighting )
            set_fighting( victim, ch );

    }
    else {
        learn_from_failure( ch, gsn_mortify );
        send_to_char( "You yell, but fail to mortify them.\r\n", ch );
    }

    return;
}

void do_taunt( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    AFFECT_DATA             af;

    one_argument( argument, arg );

    if ( !( victim = who_fighting( ch ) ) ) {
        if ( *argument != '\0' ) {
            if ( !( victim = get_char_room( ch, argument ) ) ) {
                send_to_char( "They aren't here.\r\n", ch );
                return;
            }
        }
        else {
            send_to_char( "Taunt who?\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "How can you taunt yourself?\r\n", ch );
        return;
    }

    if ( is_same_group( victim, ch ) ) {
        send_to_char( "How can you taunt your team mate?\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char( "You cannot do that skill here.\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
        return;
    }

    if ( ch->hate_level == 30 ) {
        send_to_char( "You cannot possibly be hated more than you already are.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_taunt ) ) {
        WAIT_STATE( ch, skill_table[gsn_taunt]->beats );
        learn_from_success( ch, gsn_taunt );
        act( AT_LBLUE, "You yell at $N taunting $S attention on you!", ch, NULL, victim, TO_CHAR );
        act( AT_LBLUE, "$n yells at $N to taunt them!", ch, NULL, victim, TO_NOTVICT );
        act( AT_CYAN,
             "\r\n$N stops fighting momentarily\r\n$N growls at you, and then attacks!",
             ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "\r\n$N stops fighting momentarily\r\n$N growls at $n, and attacks $m!", ch,
             NULL, victim, TO_NOTVICT );

        if ( !IS_AFFECTED( ch, AFF_HATE ) ) {
            ch->hate_level = 4;
        }
        ch->hate_level = ch->hate_level + 1;
        save_char_obj( ch );
        if ( !IS_AFFECTED( ch, AFF_HATE ) ) {
            af.type = gsn_taunt;
            af.duration = ch->level;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = meb( AFF_HATE );
            af.level = ch->level;
            affect_to_char( ch, &af );
        }
        if ( !victim->fighting )
            set_fighting( victim, ch );

    }
    else {
        learn_from_failure( ch, gsn_taunt );
        send_to_char( "You yell, but don't taunt their attention at all.\r\n", ch );
    }

    return;
}

void do_pawn( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *rch;
    AFFECT_DATA             af;
    int                     nomore;

    nomore = 40;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_pawn ) ) {
        ch->mana = ( ch->mana - nomore );
        WAIT_STATE( ch, skill_table[gsn_pawn]->beats );
        for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
            if ( IS_NPC( rch ) ) {
                af.type = gsn_pawn;
                af.duration = ch->level + 10;
                af.location = APPLY_AFFECT;
                af.modifier = 0;
                af.bitvector = meb( AFF_POISON );
                af.level = ch->level;
                affect_to_char( rch, &af );
                if ( IS_SECONDCLASS( ch ) ) {
                    rch->degree = 2;
                }
                if ( IS_THIRDCLASS( ch ) ) {
                    rch->degree = 3;
                }
                if ( ch->secondclass == -1 ) {
                    rch->degree = 1;
                }

                af.type = gsn_pawn;
                af.duration = ch->level + 10;
                af.location = APPLY_AC;
                af.modifier = 20;
                af.level = ch->level;
                affect_to_char( rch, &af );

                if ( !rch->fighting )
                    set_fighting( rch, ch );

            }
        }
        learn_from_success( ch, gsn_pawn );
        act( AT_CYAN, "You point at your enemies, and denounce them as pawn marks!", ch,
             NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n points at your enemies, and denounces them as pawn marks!", ch,
             NULL, NULL, TO_NOTVICT );
        act( AT_PINK,
             "\r\nYour enemies shriek as they feel the weight of the pawn mark settle upon them!",
             ch, NULL, NULL, TO_CHAR );
        act( AT_PINK,
             "\r\n$n's enemies shriek as they feel the weight of the pawn mark settle upon them!",
             ch, NULL, NULL, TO_NOTVICT );
    }
    else {
        learn_from_failure( ch, gsn_pawn );
        send_to_char( "You try to pawn mark your enemies, but fail.\r\n", ch );
    }
    return;
}

void do_enrage( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *rch;
    AFFECT_DATA             af;

    if ( ch->hate_level == 30 ) {
        send_to_char( "You cannot possibly be hated more than you already are.\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char( "You cannot do that skill here.\r\n", ch );
        return;
    }

    if ( !can_use_skill( ch, number_percent(  ), gsn_enrage ) ) {
        learn_from_failure( ch, gsn_enrage );
        send_to_char( "You yell, but don't enrage anyone's attention.\r\n", ch );
        return;
    }

    if ( !IS_AFFECTED( ch, AFF_HATE ) )
        ch->hate_level = 4;

    ch->hate_level = ch->hate_level + 2;
    save_char_obj( ch );
    if ( !IS_AFFECTED( ch, AFF_HATE ) ) {
        af.type = gsn_enrage;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_HATE );
        af.level = ch->level;
        affect_to_char( ch, &af );
    }

    WAIT_STATE( ch, skill_table[gsn_enrage]->beats );

    /*
     * Don't enrage players or pets 
     */
    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
        if ( IS_NPC( rch ) && !rch->fighting && !xIS_SET( rch->act, ACT_PET ) )
            set_fighting( rch, ch );

    learn_from_success( ch, gsn_enrage );

    act( AT_LBLUE, "You yell enraging everyones attention on you!", ch, NULL, NULL, TO_CHAR );
    act( AT_CYAN, "Everyone growls at you, and then attacks!", ch, NULL, NULL, TO_CHAR );

    act( AT_LBLUE, "$n yells enraging everyones attention on $m!", ch, NULL, NULL, TO_NOTVICT );
    act( AT_CYAN, "Everyone growls at $n, and attacks $m!", ch, NULL, NULL, TO_NOTVICT );
}

void do_ward( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    short                   nomore;
    short                   minblood;
    char                    arg[MIL];

    nomore = 50;
    minblood = 5;

    if ( ch->mana < nomore && ch->race != RACE_DEMON ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    if ( ch->race == RACE_DEMON && ch->blood < minblood ) {
        send_to_char( "You do not have enough blood to do that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Spectral ward who?\r\n", ch );
        return;
    }
    // Bring some balance to this skill -Taon
    if ( !IS_NPC( ch ) && ch->fighting ) {
        send_to_char( "You cant accomplish such a task while fighting.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "You cannot ward a mob.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_WARD ) ) {
        send_to_char( "They have already been warded.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_spectral_ward ) ) {
        WAIT_STATE( ch, skill_table[gsn_spectral_ward]->beats );
        if ( ch->race != RACE_DEMON ) {
            ch->mana = ( ch->mana - nomore );
        }
        else if ( ch->race == RACE_DEMON ) {
            ch->blood = ( ch->blood - minblood );
        }

        af.bitvector = meb( AFF_WARD );
        af.type = gsn_spectral_ward;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        affect_to_char( victim, &af );
        victim->ward_dam = ( 100 + ch->level * 10 );
        interpret( ch, ( char * ) "say Magaloti'c Tienaar!" );
        if ( victim == ch ) {
            act( AT_YELLOW, "\r\nA glowing spectral ward shimmers around your body.", ch,
                 NULL, NULL, TO_CHAR );
            learn_from_success( ch, gsn_spectral_ward );
            return;
        }
        else
            act( AT_YELLOW, "\r\nA glowing spectral ward shimmers around $N's body.", ch,
                 NULL, victim, TO_CHAR );
        act( AT_YELLOW, "\r\nA glowing spectral ward shimmers around $n's body.", ch, NULL,
             NULL, TO_ROOM );
        act( AT_YELLOW, "\r\nA glowing spectral ward shimmers around your body.", ch, NULL,
             victim, TO_VICT );
        learn_from_success( ch, gsn_spectral_ward );
        return;
    }
    else
        learn_from_failure( ch, gsn_spectral_ward );
    send_to_char( "You attempt to create a spectral ward, but fail.\r\n", ch );
    return;
}

void do_spirits( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    int                     nomore;
    char                    arg[MIL];

    nomore = 100;

    if ( IS_BLOODCLASS( ch ) && ch->blood <= ( nomore / 15 ) ) {
        send_to_char( "You do not have enough blood to do that.\r\n", ch );
        return;
    }
    else if ( !IS_BLOODCLASS( ch ) && ( ch->mana < nomore ) ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Spirit ward who?\r\n", ch );
        return;
    }
    // Help balance this skill a bit. -Taon
    if ( !IS_NPC( ch ) && ch->fighting ) {
        send_to_char( "You can't bring your protective ward up during combat.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_WARD ) ) {
        send_to_char( "They have already been warded.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_spirits_ward ) ) {
        WAIT_STATE( ch, skill_table[gsn_spirits_ward]->beats );
        if ( IS_BLOODCLASS( ch ) ) {
            ch->blood = ( ch->blood - ( nomore / 15 ) );
        }
        else
            ch->mana = ( ch->mana - nomore );

        af.bitvector = meb( AFF_WARD );
        af.type = gsn_spirits_ward;

        if ( ch->level < 50 )
            af.duration = ch->level * 2;
        else
            af.duration = ch->level * 4;

        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        affect_to_char( victim, &af );
        victim->ward_dam = ( 200 + ch->level * 20 );
        interpret( ch, ( char * ) "say Spirits'is Magaloti'c Tienaar!" );
        if ( victim == ch ) {
            act( AT_YELLOW, "\r\nA glowing spirits ward shimmers around your body.", ch,
                 NULL, NULL, TO_CHAR );
        }

        else {
            act( AT_YELLOW, "\r\nA glowing spirits ward shimmers around $N's body.", ch,
                 NULL, victim, TO_CHAR );
        }
        act( AT_YELLOW, "\r\nA glowing spirits ward shimmers around $n's body.", ch, NULL,
             NULL, TO_ROOM );
        act( AT_YELLOW, "\r\nA glowing spirits ward shimmers around your body.", ch, NULL,
             victim, TO_VICT );
        learn_from_success( ch, gsn_spirits_ward );
        return;
    }
    else
        learn_from_failure( ch, gsn_spirits_ward );
    send_to_char( "You attempt to create a spirits ward, but fail.\r\n", ch );
    return;
}

void do_tangle( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Tangle who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You cant perform such a task right now.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( victim, AFF_TANGLE ) ) {
        send_to_char( "But their feet are already tangled by roots into the ground.", ch );
        return;
    }
    if ( ch->move < 35 ) {
        send_to_char( "You don't have enough energy to do that.\r\n", ch );
        return;
    }
    if ( !IS_OUTSIDE( ch ) ) {
        send_to_char( "You must be outdoors to tangle their feet into the ground.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_tangle]->beats );
    if ( can_use_skill( ch, ( number_percent(  ) ), gsn_tangle ) ) {
        af.type = gsn_tangle;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.duration = ch->level;
        af.bitvector = meb( AFF_TANGLE );
        affect_to_char( victim, &af );
        ch->move -= 35;
        learn_from_success( ch, gsn_tangle );
        send_to_char( "&OYou tangle their feet with roots from the ground.\r\n", ch );
        act( AT_ORANGE, "$n tangles $N's feet into the ground with roots.", ch, NULL, victim,
             TO_ROOM );
    }
    else {
        learn_from_failure( ch, gsn_tangle );
        send_to_char
            ( "&OYou attempt to tangle your victim with roots, but fail to summon them fast enough.\r\n",
              ch );
    }
    return;
}

// Status: Completed skill for druids.  -Taon
void do_root( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You cant perform such a task right now.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_ROOT ) ) {
        send_to_char( "But you're already rooted into the ground.", ch );
        return;
    }
    if ( ch->move < 35 ) {
        send_to_char( "You don't have enough energy to do that.\r\n", ch );
        return;
    }
    if ( !IS_OUTSIDE( ch ) ) {
        send_to_char( "You must be outdoors to root your feet into the ground.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_root]->beats );
    if ( can_use_skill( ch, ( number_percent(  ) ), gsn_root ) ) {
        af.type = gsn_root;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.duration = ch->level + ( get_curr_wis( ch ) * 4 );
        af.bitvector = meb( AFF_ROOT );
        affect_to_char( ch, &af );
        ch->move -= 35;
        learn_from_success( ch, gsn_root );
        send_to_char( "You root your feet into the ground.\r\n", ch );
        act( AT_WHITE, "$n roots $s feet into the ground.", ch, NULL, NULL, TO_ROOM );
    }
    else {
        learn_from_failure( ch, gsn_root );
        act( AT_WHITE, "$n attempts to root $s feet into the ground but fails.", ch, NULL,
             NULL, TO_ROOM );
        send_to_char( "You attempt to root your feet into the ground but fail.\r\n", ch );
    }
    return;
}

void do_death_embrace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   dam;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Death embrace who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < 30 ) {
        send_to_char( "You don't have enough mana.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_death_embrace]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_death_embrace ) ) {
        act( AT_GREEN,
             "You come in close to $N, and swoop behind them taking them in a death embrace!", ch,
             NULL, victim, TO_CHAR );
        learn_from_success( ch, gsn_death_embrace );
        ch->mana -= 30;
        if ( ch->secondclass == -1 ) {
            dam = ludicrous + number_range( 100, 140 );
            global_retcode = damage( ch, victim, dam, gsn_death_embrace );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
            dam = ludicrous;
            global_retcode = damage( ch, victim, dam, gsn_death_embrace );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
            dam = ludicrous - ( ch->level );
            global_retcode = damage( ch, victim, dam, gsn_death_embrace );
        }

    }
    else {
        learn_from_failure( ch, gsn_death_embrace );
        global_retcode = damage( ch, victim, 0, gsn_death_embrace );
        send_to_char( "You fail to get close enough.\r\n", ch );
    }
    return;
}

//Wrote by: Vladaar 
//Modified, cleaned up, and installed on main port by: Taon.
void do_harm_touch( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Call forth the harm touch on who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->mana < 30 ) {
        send_to_char( "You don't have enough mana.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_harm_touch]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_harm_touch ) ) {
        act( AT_GREEN, "Your harm touch rivets through $N's body!", ch, NULL, victim, TO_CHAR );
        learn_from_success( ch, gsn_harm_touch );
        ch->mana -= 30;
        global_retcode = damage( ch, victim, extrahigh - number_range( 25, 50 ), gsn_harm_touch );
    }
    else {
        learn_from_failure( ch, gsn_harm_touch );
        global_retcode = damage( ch, victim, 0, gsn_harm_touch );
        send_to_char( "You fail to call upon the dark arts power!\r\n", ch );
    }
    return;
}

void do_decree_decay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    AFFECT_DATA             af;

    if ( ch->mana < 30 ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    send_to_char( "\r\n\r\n&GYou call upon the dark arts power!\r\n", ch );

    if ( can_use_skill( ch, number_percent(  ), gsn_decree_decay ) ) {
        WAIT_STATE( ch, skill_table[gsn_decree_decay]->beats );
        learn_from_success( ch, gsn_decree_decay );
        ch->mana -= 30;
        act( AT_GREEN, "You decree decay within the area!", ch, NULL, NULL, TO_CHAR );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !vch )
                continue;

            if ( is_same_group( vch, ch ) )
                continue;

            if ( IS_AFFECTED( vch, AFF_DECREE_DECAY ) )
                continue;

            act( AT_GREEN, "$n decree's decay to your body!", ch, NULL, vch, TO_VICT );

            if ( !vch->fighting )
                set_fighting( vch, ch );

            af.type = gsn_decree_decay;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_EXT_AFFECT;
            af.modifier = 0;
            af.bitvector = meb( AFF_DECREE_DECAY );
            affect_to_char( vch, &af );

            af.type = gsn_decree_decay;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = -2;
            af.bitvector = meb( AFF_POISON );
            affect_join( vch, &af );

            set_char_color( AT_GREEN, vch );
            send_to_char( "You feel very sick.\r\n", vch );
            if ( IS_SECONDCLASS( ch ) )
                vch->degree = 2;
            if ( IS_THIRDCLASS( ch ) )
                vch->degree = 3;
            if ( ch->secondclass == -1 )
                vch->degree = 1;

            af.type = gsn_decree_decay;
            af.duration = ( ch->level );
            af.location = APPLY_HITROLL;
            af.level = ch->level;
            af.modifier = -1;
            af.bitvector = meb( AFF_CURSE );
            affect_to_char( vch, &af );

            af.type = gsn_decree_decay;
            af.duration = ch->level;
            af.location = APPLY_SAVING_SPELL;
            af.modifier = 1;
            af.level = ch->level;
            affect_to_char( vch, &af );

            set_char_color( AT_MAGIC, vch );
            send_to_char( "You feel unclean.\r\n", vch );
        }
    }
    else {
        learn_from_failure( ch, gsn_decree_decay );
        send_to_char( "&cYou fail to call upon the dark arts power!\r\n", ch );
    }
}

//Coded by: Vladaar,
//Modified, cleaned up, and installed by: Taon.
void do_siphon_strength( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    short                   move;

    if ( !ch->fighting ) {
        send_to_char( "You must be in combat before you can use this skill.\r\n", ch );
        return;
    }
    if ( ch->mana < 30 ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    victim = who_fighting( ch );

    if ( IS_AFFECTED( ch, AFF_SIPHON_STRENGTH ) ) {
        send_to_char( "No more strength can be drained from their body.\r\n", ch );
        return;
    }

    send_to_char( "\r\n\r\n&GYou call upon the dark arts power!\r\n", ch );

    WAIT_STATE( ch, skill_table[gsn_siphon_strength]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_siphon_strength ) ) {
        ch->mana -= 30;
        act( AT_GREEN, "You harvest your opponet's strength for your own!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_GREEN, "$n siphons your strength away!", ch, NULL, victim, TO_VICT );
        move = ch->level;
        af.type = gsn_siphon_strength;
        af.duration = ch->level;
        af.level = ch->level;
        af.location = APPLY_STR;
        af.modifier = -2;
        af.bitvector = meb( AFF_SIPHON_STRENGTH );
        affect_join( victim, &af );

        if ( move <= victim->move )
            victim->move -= move;
        else
            victim->move = 0;

        if ( move >= ch->max_move )
            ch->move = ch->max_move;
        else
            ch->move += move;

        af.type = gsn_siphon_strength;
        af.duration = ch->level;
        af.level = ch->level;
        af.location = APPLY_STR;
        af.modifier = 2;
        af.bitvector = meb( AFF_SIPHON_STRENGTH );
        affect_join( ch, &af );
        global_retcode = damage( ch, victim, number_chance( 6, 24 ), gsn_siphon_strength );
        learn_from_success( ch, gsn_siphon_strength );
    }
    else {
        learn_from_failure( ch, gsn_siphon_strength );
        global_retcode = damage( ch, victim, 0, gsn_siphon_strength );
        send_to_char( "&cYou fail to call upon the dark arts power!\r\n", ch );
    }
}

/* Volk - rewrote this to take into account bitvectors that player might be carrying.. 
 * Need to go through code and totally replace 'is xIS_SET(ch->affected_by, AFFWHATEVER'
 * with IS_AFFECTED(ch, AFFWHATEVER) so it checks BV too */
bool IS_AFFECTED( CHAR_DATA *ch, int arg )
{
    AFFECT_DATA            *paf,
                           *paf_next;

    if ( !ch )
        return FALSE;

    if ( xIS_SET( ch->affected_by, arg ) )
        return TRUE;

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;
        if ( xIS_SET( paf->bitvector, arg ) )
            return TRUE;
    }

    return FALSE;
}

/* Volk - new stuff for bards. Woodwinds, strings, brass, drums. Vocals are easy
   because no instruments are involved, but at higher levels need to be HOLDING    
   the instrument, have it tuned up and ready to go. v0 is the instrument type,
   v1 its 'tune', v2 its condition (ie 0-10, 0 needs bits replaced etc), v3 and
   v4 are extras, possibly dam+ or elemental damage for each song */

bool CAN_PLAY( CHAR_DATA *ch, int type )
{
/* Volk - in a nutshell, this will check if they can use their vocals right now.
   They might be in a silent room, or be silenced, or just be too damned tired.
   It will give them the appropriate message, so call it with !can_vocals -> return */
/* Volk - changed this to 'can_play' (from can_vocals). Everything still applies
   but now putting in types - 0 vocals, 1 woodwind, 2 strings, 3 brass, 4 drums -
   to check if can_play that type. If not, error msg and exit */

    if ( ch->in_room && IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) ) {
        send_to_char( "You try, but the sound here is magically absorbed.\r\n", ch );
        return FALSE;
    }

    if ( ch->move < ( ch->max_move / 10 ) ) {          /* Less than 10% of their max */
        send_to_char( "You are just too tired.\r\n", ch );
        return FALSE;
    }

    if ( type == 0 ) {
        if ( IS_AFFECTED( ch, AFF_SILENCE ) ) {
            send_to_char( "You can't even TALK, how can you sing?\r\n", ch );
            return FALSE;
        }
        if ( LEARNED( ch, gsn_vocals ) <= 0 ) {
            send_to_char( "You need to train your vocals before singing.\r\n", ch );
            return FALSE;
        }
    }

    if ( type == 1 ) {
        if ( !HAS_INSTRUMENT( ch, 1 ) )
            return FALSE;
        if ( LEARNED( ch, gsn_woodwinds ) <= 0 ) {
            send_to_char( "You don't know how to play woodwinds!\r\n", ch );
            return FALSE;
        }
    }

    if ( type == 2 ) {
        if ( !HAS_INSTRUMENT( ch, 2 ) )
            return FALSE;
        if ( LEARNED( ch, gsn_strings ) <= 0 ) {
            send_to_char( "You don't know how to play stringed instruments!\r\n", ch );
            return FALSE;
        }
    }

    if ( type == 3 ) {
        if ( !HAS_INSTRUMENT( ch, 3 ) )
            return FALSE;
        if ( LEARNED( ch, gsn_brass ) <= 0 ) {
            send_to_char( "You don't know how to play brass!\r\n", ch );
            return FALSE;
        }
    }

    if ( type == 4 ) {
        if ( !HAS_INSTRUMENT( ch, 4 ) )
            return FALSE;
        if ( LEARNED( ch, gsn_drums ) <= 0 ) {
            send_to_char( "You don't know how to play drums!\r\n", ch );
            return FALSE;
        }
    }

    if ( type < 0 || type > 4 ) {
        bug( "Bad type in CAN_PLAY by %s", ch->name );
        send_to_char( "BUG: CAN_PLAY bad type number, please contact staff.\r\n", ch );
        return FALSE;
    }

    return TRUE;
}

bool HAS_INSTRUMENT( CHAR_DATA *ch, int type )
{
/* Types are 1 - woodwind, 2 - strings, 3 - brass, 4 - drums. YES I could put these in a 
   struct and call them from obj->v0 but.. cbf. :P 
   Again, messages in function - just if !HAS_INSTRUMENT(ch, whatever)->return.
   ALSO use 0 as the type if instrument type DOESN'T MATTER (ie tune)
*/

    OBJ_DATA               *obj = get_eq_char( ch, WEAR_HOLD );

    if ( obj == NULL || obj->item_type != ITEM_INSTRUMENT ) {
        send_to_char( "You are not holding an instrument.\r\n", ch );
        obj == NULL;
    }

    if ( obj && ( obj->value[0] <= 0 || obj->value[0] >= 5 ) ) {
        bug( "%s holding instrument vnum %d with bad type! (value0)", ch->name,
             obj->pIndexData->vnum );
        return FALSE;
    }

    if ( obj && ( type == obj->value[0] || type == 0 ) )
        return TRUE;

    if ( type == 1 )
        send_to_char( "You must be holding a woodwind instrument!\r\n", ch );
    else if ( type == 2 )
        send_to_char( "You must be holding a stringed instrument!\r\n", ch );
    else if ( type == 3 )
        send_to_char( "You must be holding a brass instrument!\r\n", ch );
    else if ( type == 4 )
        send_to_char( "You must be holding a drum!\r\n", ch );
    else {
        bug( "%s using bugged instrument! (v0 < 0 || > 4)", ch->name );
        send_to_char( "The instrument you are holding is bugged, please report to staff.", ch );
    }

    return FALSE;
}

void do_tune( CHAR_DATA *ch, char *argument )
{
    if ( HAS_INSTRUMENT( ch, 0 ) ) {                   /* IS holding an instrument. We *
                                                        * don't care which one, yet. */
/* TODO: Tune code here! */
        return;
    }

/* If no instrument, error msg will come in HAS_INSTRUMENT so just return */
    return;
}

/* Called every second while char's bardtimer is above 0 */
void bard_music( CHAR_DATA *ch )
{
    int                     timer = ch->pcdata->bard;
    int                     sn = ch->pcdata->bardsn;

/* When this happens we want to..
1) Check which instrument the bard is using.
2) Check which song the bard is playing.
3) First, if the timer has come to an end, end the song and exit. else..
4) Detune or damage the instrument a little (over time) depending on
   what type of instrument and bard's skill level (ie drum and bard's
   gsn_drums learned).
5) Send a message to the bard every X seconds

Timer is based on bard's skill level in song/100, * skill level in instrument/100,
* (level*20) (max is about an hour, but instrument would damage before then).
*/

    if ( timer > 0 || timer < 1 ) {
        bug( "%s timer not above 0 in bard_music! ONOES!", ch->name );
        send_to_char( "BUG WITH BARD_MUSIC - please see staff immediately.", ch );
        timer = 0;
        sn = 0;
        return;
    }

    if ( !skill_table[sn] ) {
        bug( "%s: bard timer with no skill for sn %d!", ch->name, sn );
        send_to_char( "BUG WITH BARD_MUSIC - please see staff immediately.", ch );
        timer = 0;
        sn = 0;
        return;
    }

/* Bard has an instrument, let's work out which one (if none assume vocals)*/
    OBJ_DATA               *obj = get_eq_char( ch, WEAR_HOLD );

    int                     type = 0;

    if ( obj == NULL || obj->item_type != ITEM_INSTRUMENT ) /* not holding anything, * or 
                                                             * no instrument - * vocals. */
        type = 0;

/* TODO: code here */

    return;
}

void bard_stop_playing( CHAR_DATA *ch, OBJ_DATA *obj )
{
/* If we get here, the bard has just removed an instrument. Need to check
   if it was being used at the time and if so, stop the affect. */
    CHAR_DATA              *gch;

    if ( !ch->pcdata || ch->pcdata->bard <= 0 || ch->pcdata->bardsn <= 0 )
        return;

/* Righto, there WAS a song if we've got this far. Now we want to see who
   in the room is affected by this, and send them a message plus strip affect */
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room ) {
        if ( is_same_group( ch, gch ) ) {
            if ( ch == gch )
                send_to_char( "You stop playing and the song's effect dissipates.\r\n", ch );
            else
                pager_printf( gch, "%s stops playing and the song's effect dissipates.\r\n", ch );
            send_to_char( skill_table[ch->pcdata->bardsn]->msg_off, ch );
            affect_strip( gch, ch->pcdata->bardsn );
        }
        else
            pager_printf( gch, "%s stops playing %s.\r\n", ch, obj->short_descr );
    }
    return;
}

/* REMOVE from save.c fread_char prac++ on players with songs */

void do_sigilist( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *mob;
    OBJ_DATA               *obj;
    AFFECT_DATA            *paf;
    short                   choice = 0;
    int                     cost = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
        if ( IS_NPC( mob ) && ( xIS_SET( mob->act, ACT_SIGILIST ) ) )
            break;

    if ( !mob ) {
        send_to_char( "You can't do that here.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        act( AT_TELL, "$n tells you 'Type, sigil <object> <choice>.'", mob, NULL, ch, TO_VICT );
        act( AT_TELL, "$n tells you 'Sigil options being: HP, MANA, MOVE, HITROLL, DAMROLL", mob,
             NULL, ch, TO_VICT );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "You are not using that item.\r\n", ch );
        return;
    }

    separate_obj( obj );

    if ( obj->item_type != ITEM_ARMOR ) {
        act( AT_CYAN, "The elderly sigilist frowns at $p because it is not an armor.", ch, obj,
             NULL, TO_CHAR );
        return;
    }

    if ( IS_OBJ_STAT( obj, ITEM_MAGIC ) ) {
        act( AT_CYAN,
             "The elderly sigilist tries to attach a sigil to $p, but there is one already there.",
             ch, obj, NULL, TO_CHAR );
        return;
    }

    if ( arg1[0] != '\0' && arg2[0] == '\0' ) {
        act( AT_TELL, "The elderly sigilist tells you 'What sigil did you want added?'", ch, NULL,
             ch, TO_CHAR );
        send_to_char( "\r\n&cType sigil <object> <choice>.\r\n", ch );
        act( AT_TELL,
             "The elderly sigilist tells you 'Sigil options being: HP, MANA, MOVE, HITROLL, DAMROLL",
             ch, NULL, ch, TO_CHAR );
        return;
    }

    if ( !str_cmp( arg2, "hp" ) ) {
        choice = 1;
        cost = 100;
    }
    else if ( !str_cmp( arg2, "move" ) ) {
        choice = 2;
        cost = 100;
    }
    else if ( !str_cmp( arg2, "mana" ) ) {
        choice = 3;
        cost = 100;
    }
    else if ( !str_cmp( arg2, "hitroll" ) ) {
        choice = 4;
        cost = 300;
    }
    else if ( !str_cmp( arg2, "damroll" ) ) {
        choice = 5;
        cost = 300;
    }
    else {
        send_to_char( "That isn't a valid choice.\r\n", ch );
        return;
    }

    if ( GET_MONEY( ch, CURR_GOLD ) < cost ) {
        ch_printf( ch, "You haven't got that much gold.\r\n" );
        return;
    }

    if ( obj->level < 90 ) {
        act( AT_CYAN,
             "The elderly sigilist looks disgusted at $p, and then you. The elderly sigilist says, 'I said avatar armor!'",
             ch, obj, NULL, TO_CHAR );
        return;
    }

    CREATE( paf, AFFECT_DATA, 1 );
    paf->type = -1;
    paf->duration = -1;
    xCLEAR_BITS( paf->bitvector );

    if ( choice == 1 ) {
        paf->location = APPLY_HIT;
        paf->modifier = 50;
    }
    else if ( choice == 2 ) {
        paf->location = APPLY_MOVE;
        paf->modifier = 50;
    }
    else if ( choice == 3 ) {
        paf->location = APPLY_MANA;
        paf->modifier = 50;
    }
    else if ( choice == 4 ) {
        paf->location = APPLY_HITROLL;
        paf->modifier = 1;
    }
    else if ( choice == 5 ) {
        paf->location = APPLY_DAMROLL;
        paf->modifier = 1;
    }

    LINK( paf, obj->first_affect, obj->last_affect, next, prev );
    GET_MONEY( ch, CURR_GOLD ) -= cost;
    xSET_BIT( obj->extra_flags, ITEM_MAGIC );

    act( AT_MAGIC,
         "The elderly sigilist carefully attaches a sigil on $p and enhancing it with the sigil's magic.",
         ch, obj, NULL, TO_CHAR );
    save_char_obj( ch );
}

// starting of flamebreath for balrog remort. -Taon
void do_flamebreath( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( arg[0] == '\0' ) {
        send_to_char( "Who is the target for your flaming breath???\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "Your target is nowhere to be found.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You cant flamebreath yourself.\r\n", ch );
        return;
    }
    if ( ch->blood < 40 ) {
        send_to_char( "You don't have enough blood to do such a thing.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_flamebreath]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_flamebreath ) ) {
        if ( IS_NPC( victim ) )
            ch_printf( ch, "You spit a column of fire toward %s\r\n", victim->short_descr );
        else
            ch_printf( ch, "You spit a column of fire toward %s\r\n", victim->name );

        if ( !IS_NPC( victim ) )
            ch_printf( victim, "Your engulfed in columns of %s flames.\r\n ", ch->name );

        global_retcode = damage( ch, victim, mediumhigh, gsn_flamebreath );
        ch->blood -= 40;
        learn_from_success( ch, gsn_flamebreath );
    }
    else {
        send_to_char( "You failed to properly invoke a column of flames!\r\n", ch );
        ch->blood -= 10;
        learn_from_failure( ch, gsn_flamebreath );
    }
    return;
}

//HI-JACKED CODE ALERT, HAHA! -Taon
//I hi-jacked the judge code for flaming whip.
void do_flaming_whip( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    buf[MSL];
    AFFECT_DATA            *paf;
    int                     sn = 1;
    short                   value;

    if ( can_use_skill( ch, number_percent(  ), gsn_flaming_whip ) ) {
        WAIT_STATE( ch, skill_table[gsn_flaming_whip]->beats );
        obj = create_object( get_obj_index( OBJ_VNUM_FLAMING_WHIP ), 0 );

        if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
            send_to_char
                ( "You fail to summon the flaming whip as you cannot carry that many items.\r\n",
                  ch );
            extract_obj( obj );
            return;
        }

        /*
         * I set item damage with value2 to 1+ players level
         */
        obj->level = ch->level;
        value = ch->level;
        obj->value[0] = 12;                            /* condition */
        obj->pIndexData->value[1] = set_min_chart( value );
        obj->pIndexData->value[2] = set_max_chart( value );
        obj->value[1] = set_min_chart( value );
        obj->value[2] = set_max_chart( value );
        obj->value[3] = 5;
        obj->value[4] = 3;
        snprintf( buf, MSL, "%s's Flaming Whip", ch->name );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );
        obj->weight = 10;
        obj->color = 6;

/* To avoid freeing a null pointer */
        if ( obj->owner == NULL ) {
            obj->owner = STRALLOC( ch->name );
        }
        else {
            STRFREE( obj->owner );
            obj->owner = STRALLOC( ch->name );
        }

        /*
         * lets give the weapon greater power at higher levels
         */
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = sn;
        paf->duration = -1;
        paf->location = APPLY_HITROLL;                 /* can use diff applies hit, move,
                                                        * hitroll, damroll */
        paf->modifier = 5 + ( ch->level / 20 );        /* can be whatever you like */
        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );

        if ( ch->level > 19 ) {
            CREATE( paf, AFFECT_DATA, 2 );

            paf->type = -1;
            paf->duration = -1;
            paf->location = APPLY_STR;
            paf->modifier = 2;                         /* can be whatever you like */
            xCLEAR_BITS( paf->bitvector );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }

        if ( ch->level > 29 ) {
            CREATE( paf, AFFECT_DATA, 1 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_DAMROLL;
            paf->modifier = 1 + ( ch->level / 20 );
            xCLEAR_BITS( paf->bitvector );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }
        if ( ch->level > 49 ) {
            CREATE( paf, AFFECT_DATA, 4 );

            paf->type = sn;
            paf->level = 5;
            paf->duration = -1;
            paf->location = APPLY_WEAPONSPELL;
            paf->modifier = skill_lookup( "flamestrike" );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }

        obj->timer = ch->level + 20;                   /* object duration */
        act( AT_YELLOW, "A flaming whip materializes into $n's hands.", ch, NULL, NULL, TO_ROOM );
        act( AT_YELLOW, "Your flaming whip materializes into your hands.", ch, NULL,
             NULL, TO_CHAR );
        obj = obj_to_char( obj, ch );
        learn_from_success( ch, gsn_flaming_whip );
        return;
    }
    else
        act( AT_PLAIN, "You cannot concentrate enough to summon a flaming whip.", ch,
             NULL, NULL, TO_CHAR );
    learn_from_failure( ch, gsn_flaming_whip );
    return;

}

//toggle for autoskill energy containment - Taon

void do_energy_containment( CHAR_DATA *ch, char *argument )
{

    if ( IS_NPC( ch ) )
        return;

    if ( ch->mana < 150 ) {
        send_to_char( "You don't have enough energy to do such a thing.\r\n", ch );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_energy_containment ) ) {
        if ( !IS_AFFECTED( ch, AFF_ENERGY_CONTAINMENT ) ) {
            send_to_char( "You activate your energy containment!\r\n", ch );
            xSET_BIT( ch->affected_by, AFF_ENERGY_CONTAINMENT );
        }
        else {
            send_to_char( "You deactivate your energy containment!\r\n", ch );
            xREMOVE_BIT( ch->affected_by, AFF_ENERGY_CONTAINMENT );
        }

        ch->mana -= 150;
    }
    return;
}
