 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Main structure manipulation module                                    *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "h/mud.h"
#include "h/hometowns.h"
#include "h/clans.h"
#include "h/polymorph.h"
#include <stdarg.h>
#include "h/ftag.h"
#include "h/files.h"

#define NEW_AUTH(ch)         (!IS_NPC(ch) && (ch)->level == 1)

extern OBJ_DATA        *dragonegg;

extern int              top_exit;
extern int              top_ed;
extern int              top_affect;
extern int              cur_qobjs;
extern int              cur_qchars;
extern CHAR_DATA       *gch_prev;
extern OBJ_DATA        *gobj_prev;
extern REL_DATA        *first_relation;
extern REL_DATA        *last_relation;
CHAR_DATA              *cur_char;
ROOM_INDEX_DATA        *cur_room;
bool                    cur_char_died;
ch_ret                  global_retcode;

void                    delete_reset( RESET_DATA *pReset );
int                     cur_obj;
int                     cur_obj_serial;
bool                    cur_obj_extracted;
obj_ret                 global_objcode;
OBJ_DATA               *group_object( OBJ_DATA *obj1, OBJ_DATA *obj2 );
bool                    in_magic_container( OBJ_DATA *obj );
bool                    is_magic_container( OBJ_DATA *obj );

void stralloc_printf( char **pointer, const char *fmt, ... )
{
    char                    buf[MSL * 2];
    va_list                 args;

    va_start( args, fmt );
    vsnprintf( buf, MSL * 2, fmt, args );
    va_end( args );
    if ( *pointer )
        STRFREE( *pointer );
    *pointer = STRALLOC( buf );
    return;
}

void strdup_printf( char **pointer, char *fmt, ... )
{
    char                    buf[MSL * 2];
    va_list                 args;

    va_start( args, fmt );
    vsnprintf( buf, MSL * 2, fmt, args );
    va_end( args );
    if ( *pointer )
        DISPOSE( *pointer );
    *pointer = str_dup( buf );
}

/*
 * Return how much exp a char has

int get_exp( CHAR_DATA * ch )
{
   return ch->exp;
}

*
 * Calculate roughly how much experience a character is worth
 */
int get_exp_worth( CHAR_DATA *ch )
{
    int                     wexp;

    wexp = ch->level * ch->level * ch->level * 5;
    wexp += ch->max_hit;
    wexp -= ( ch->armor - 50 ) * 2;
    wexp += ( ch->barenumdie * ch->baresizedie + GET_DAMROLL( ch ) ) * 50;
    wexp += GET_HITROLL( ch ) * ch->level * 10;
    wexp = URANGE( MIN_EXP_WORTH, wexp, MAX_EXP_WORTH );
    return wexp;
}

short get_exp_base( CHAR_DATA *ch, short class_info )
{
    int                     base;

    if ( IS_NPC( ch ) )
        return 1000;

    base = class_table[class_info]->exp_base;

    return base;
}

short get_craft_base( CHAR_DATA *ch, short class_info )
{
    int                     base;

    if ( IS_NPC( ch ) )
        return 1000;

    base = class_table[class_info]->craft_base;

    return base;
}

/*
 * Updated exp_level function, to solve the pointless use of big numbers, and to
 * correct the old bad maths. -Orion
 */

int exp_level( CHAR_DATA *ch, short level )
{
    int                     exp_level;

    level = UMAX( 0, level );
    exp_level = ( get_exp_base( ch, ch->Class ) * ( ( 3 * level * level ) - ( 9 * level ) + 7 ) );

    return exp_level;
}

int craft_level( CHAR_DATA *ch, short level )
{
    int                     craft_level;

    level = UMAX( 0, level );
    craft_level = ( get_craft_base( ch, 20 ) * ( ( 3 * level * level ) - ( 9 * level ) + 7 ) );

    return craft_level;
}

int exp_craft_level( CHAR_DATA *ch, short level, short class_info )
{
    int                     craft_level;

    level = UMAX( 0, level );
    craft_level = ( get_craft_base( ch, 20 ) * ( ( 3 * level * level ) - ( 9 * level ) + 7 ) );

    return craft_level;
}

/*  This is almost a mock of the function exp_level but handles nicer
 *  for multi-classes, I could have just wrote exp_level to handle all 
 *  this but its only really needed in a few places, like do_level.  -Taon 
 */
// Great call - I was just about to do that. :P
int exp_class_level( CHAR_DATA *ch, short level, short class_info )
{
    int                     exp_level;

    level = UMAX( 0, level );
    exp_level = ( get_exp_base( ch, class_info ) * ( ( 3 * level * level ) - ( 9 * level ) + 7 ) );

    return exp_level;
}

/* Retrieve a character's trusted level for permission checking. */
short get_trust( CHAR_DATA *ch )
{
    if ( ch && ch->desc && ch->desc->original )
        ch = ch->desc->original;

    if ( ch->trust != 0 )
        return ch->trust;

    if ( IS_NPC( ch ) ) {
        if ( ch->level >= LEVEL_AJ_MAJOR )
            return LEVEL_AJ_MAJOR;
    }
    else {
        if ( ch->level >= LEVEL_IMMORTAL && IS_RETIRED( ch ) )
            return LEVEL_IMMORTAL;
    }

    return ch->level;
}

/* One hopes this will do as planned and determine how old a PC is based on the birthdate
   we record at creation. - Samson 10-25-99 */
short calculate_age( CHAR_DATA *ch )
{
    short                   age,
                            num_days,
                            ch_days;

    if ( IS_NPC( ch ) )
        return -1;

    num_days = ( time_info.month + 1 ) * sysdata.dayspermonth;
    num_days += time_info.day;

    ch_days = ( ch->pcdata->month + 1 ) * sysdata.dayspermonth;
    ch_days += ch->pcdata->day;

    age = time_info.year - ch->pcdata->year;

    if ( ch->level > 5 && ch->level < 10 )
        age += 1;
    else if ( ch->level > 9 && ch->level < 20 )
        age += 2;
    else if ( ch->level > 19 && ch->level < 30 )
        age += 3;
    else if ( ch->level > 29 && ch->level < 40 )
        age += 4;
    else if ( ch->level > 39 && ch->level < 50 )
        age += 5;
    else if ( ch->level > 49 && ch->level < 60 )
        age += 7;
    else if ( ch->level > 59 && ch->level < 70 )
        age += 8;
    else if ( ch->level > 69 && ch->level < 80 )
        age += 9;
    else if ( ch->level > 79 && ch->level < 90 )
        age += 10;
    else if ( ch->level > 89 && ch->level < 100 )
        age += 11;

    if ( ch_days - num_days > 0 )
        age -= 1;

    if ( ch->race == RACE_VAMPIRE )
        age += 100;

    if ( age < 1 ) {
        age = 100;
    }

    return age;
}

/* Retrieve character's current strength. */
short get_curr_str( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_str + ch->mod_str, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_STR || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_STR )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_STR )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_str + ch->mod_str, max );
}

/* Retrieve character's current intelligence. */
short get_curr_int( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_int + ch->mod_int, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_INT || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_INT )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_INT )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_int + ch->mod_int, max );
}

/* Retrieve character's current wisdom. */
short get_curr_wis( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_wis + ch->mod_wis, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_WIS || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_WIS )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_WIS )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_wis + ch->mod_wis, max );
}

/* Retrieve character's current dexterity. */
short get_curr_dex( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_dex + ch->mod_dex, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_DEX || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_DEX )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_DEX )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_dex + ch->mod_dex, max );
}

/* Retrieve character's current constitution. */
short get_curr_con( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_con + ch->mod_con, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_CON || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_CON )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_CON )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_con + ch->mod_con, max );
}

/* Retrieve character's current charisma. */
short get_curr_cha( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_cha + ch->mod_cha, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_CHA || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_CHA )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_CHA )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_cha + ch->mod_cha, max );
}

/* Retrieve character's current luck. */
short get_curr_lck( CHAR_DATA *ch )
{
    short                   max;

    if ( !class_table[ch->Class] )
        return URANGE( 3, ch->perm_lck + ch->mod_lck, 20 );
    if ( IS_NPC( ch ) || class_table[ch->Class]->attr_prime == APPLY_LCK || IS_IMMORTAL( ch ) )
        max = 25;
    else if ( class_table[ch->Class]->attr_second == APPLY_LCK )
        max = 22;
    else if ( class_table[ch->Class]->attr_deficient == APPLY_LCK )
        max = 16;
    else
        max = 20;
    return URANGE( 3, ch->perm_lck + ch->mod_lck, max );
}

/* Retrieve a character's carry capacity.
 * Vastly reduced (finally) due to containers  -Thoric
 */
int can_carry_n( CHAR_DATA *ch )
{
    int                     penalty = 0;

    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return get_trust( ch ) * 100;
    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_PET ) )
        return 1;
    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_IMMORTAL ) )
        return ch->level * 100;
    if ( IS_NPC( ch ) && ch->pIndexData && ch->pIndexData->pShop )
        return 20;
    if ( get_eq_char( ch, WEAR_WIELD ) )
        ++penalty;
    if ( get_eq_char( ch, WEAR_DUAL_WIELD ) )
        ++penalty;
    if ( get_eq_char( ch, WEAR_MISSILE_WIELD ) )
        ++penalty;
    if ( get_eq_char( ch, WEAR_HOLD ) )
        ++penalty;
    if ( get_eq_char( ch, WEAR_SHIELD ) )
        ++penalty;
    return URANGE( 10, ( ch->level + 15 ) / 5 + get_curr_dex( ch ) - 13 - penalty, 20 );
}

/* Retrieve a character's carry capacity. */
int can_carry_w( CHAR_DATA *ch )
{
    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return 100000;

    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_PET ) )
        return 100;

    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_IMMORTAL ) )
        return 100000;

    if ( IS_NPC( ch ) && ch->pIndexData && ch->pIndexData->pShop )
        return 5000;

    if ( ch->race == RACE_DRAGON ) {
        return str_app[get_curr_str( ch )].carry * 2;
    }
    return str_app[get_curr_str( ch )].carry;
}

/*
 * See if a player/mob can take a piece of prototype eq  -Thoric
 */
bool can_take_proto( CHAR_DATA *ch )
{
    if ( IS_IMMORTAL( ch ) )
        return TRUE;
    else if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_PROTOTYPE ) )
        return TRUE;
    else
        return FALSE;
}

/*
 * See if a string is one of the names of an object.
 */
bool is_name( const char *str, char *namelist )
{
    char                    name[MIL];

    if ( !VLD_STR( str ) && !VLD_STR( namelist ) )
        return TRUE;
    if ( !VLD_STR( str ) || !VLD_STR( namelist ) )
        return FALSE;
    while ( VLD_STR( namelist ) ) {
        namelist = one_argument( namelist, name );
        if ( !VLD_STR( name ) )
            return FALSE;
        if ( !str_cmp( str, name ) )
            return TRUE;
    }
    return FALSE;
}

bool is_name_prefix( const char *str, char *namelist )
{
    char                    name[MIL];

    if ( !VLD_STR( str ) && !VLD_STR( namelist ) )
        return TRUE;
    if ( !VLD_STR( str ) || !VLD_STR( namelist ) )
        return FALSE;
    while ( VLD_STR( namelist ) ) {
        namelist = one_argument( namelist, name );
        if ( !VLD_STR( name ) )
            return FALSE;
        if ( !str_prefix( str, name ) )
            return TRUE;
    }
    return FALSE;
}

/*
 * See if a string is one of the names of an object.  -Thoric
 * Treats a dash as a word delimiter as well as a space
 */
bool is_name2( const char *str, char *namelist )
{
    char                    name[MIL];

    if ( !VLD_STR( str ) && !VLD_STR( namelist ) )
        return TRUE;
    if ( !VLD_STR( str ) || !VLD_STR( namelist ) )
        return FALSE;
    while ( VLD_STR( namelist ) ) {
        namelist = one_argument2( namelist, name );
        if ( !VLD_STR( name ) )
            return FALSE;
        if ( !str_cmp( str, name ) )
            return TRUE;
    }
    return FALSE;
}

bool is_name2_prefix( const char *str, char *namelist )
{
    char                    name[MIL];

    if ( !VLD_STR( str ) && !VLD_STR( namelist ) )
        return TRUE;
    if ( !VLD_STR( str ) || !VLD_STR( namelist ) )
        return FALSE;
    while ( VLD_STR( namelist ) ) {
        namelist = one_argument2( namelist, name );
        if ( !VLD_STR( name ) )
            return FALSE;
        if ( !str_prefix( str, name ) )
            return TRUE;
    }
    return FALSE;
}

/*        -Thoric
 * Checks if str is a name in namelist supporting multiple keywords
 */
bool nifty_is_name( char *str, char *namelist )
{
    char                    name[MIL];

    if ( !VLD_STR( str ) && !VLD_STR( namelist ) )
        return FALSE;
    if ( !VLD_STR( str ) || !VLD_STR( namelist ) )
        return TRUE;
    while ( VLD_STR( str ) ) {
        str = one_argument2( str, name );
        if ( !VLD_STR( name ) )
            return TRUE;
        if ( !is_name2( name, namelist ) )
            return FALSE;
    }
    return TRUE;
}

bool nifty_is_name_prefix( char *str, char *namelist )
{
    char                    name[MIL];

    if ( !VLD_STR( str ) && !VLD_STR( namelist ) )
        return FALSE;
    if ( !VLD_STR( str ) || !VLD_STR( namelist ) )
        return TRUE;
    while ( VLD_STR( str ) ) {
        str = one_argument2( str, name );
        if ( !VLD_STR( name ) )
            return TRUE;
        if ( !is_name2_prefix( name, namelist ) )
            return FALSE;
    }
    return TRUE;
}

void room_affect( ROOM_INDEX_DATA *pRoomIndex, AFFECT_DATA *paf, bool fAdd )
{
    if ( fAdd ) {
        switch ( paf->location ) {
            case APPLY_ROOMFLAG:
            case APPLY_SECTORTYPE:
                break;
            case APPLY_ROOMLIGHT:
                pRoomIndex->light += paf->modifier;
                break;
            case APPLY_TELEVNUM:
            case APPLY_TELEDELAY:
                break;
        }
    }
    else {
        switch ( paf->location ) {
            case APPLY_ROOMFLAG:
            case APPLY_SECTORTYPE:
                break;
            case APPLY_ROOMLIGHT:
                pRoomIndex->light -= paf->modifier;
                break;
            case APPLY_TELEVNUM:
            case APPLY_TELEDELAY:
                break;
        }
    }
}

/*
 * Modify a skill (hopefully) properly   -Thoric
 *
 * On "adding" a skill modifying affect, the value set is unimportant
 * upon removing the affect, the skill it enforced to a proper range.
 */
void modify_skill( CHAR_DATA *ch, int sn, int mod, bool fAdd )
{
    if ( !IS_NPC( ch ) ) {
        if ( fAdd )
            ch->pcdata->learned[sn] += mod;
        else
            ch->pcdata->learned[sn] =
                URANGE( 0, ch->pcdata->learned[sn] + mod, get_maxadept( ch, sn, TRUE ) );
    }
    return;
}

/* This is used to unstun someone when they shouldn't be stunned any longer instead of waiting on char_update */
void unstun( CHAR_DATA *ch )
{
    if ( !ch || ch->hit <= 0 || ch->position != POS_STUNNED || IS_AFFECTED( ch, AFF_PARALYSIS ) )
        return;

    /*
     * Else paralysis has worn off etc... so go ahead and update their position 
     */
    update_pos( ch );
}

/* Apply or remove an affect to a character. */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd )
{
    OBJ_DATA               *wield;
    int                     mod;
    struct skill_type      *skill;
    ch_ret                  retcode;

    mod = paf->modifier;

    if ( fAdd ) {
        xSET_BITS( ch->affected_by, paf->bitvector );
        if ( ( paf->location % REVERSE_APPLY ) == APPLY_RECURRINGSPELL ) {
            mod = abs( mod );
            if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
                 && skill->type == SKILL_SPELL )
                xSET_BIT( ch->affected_by, AFF_RECURRINGSPELL );
            else
                bug( "affect_modify(%s) APPLY_RECURRINGSPELL with bad sn %d", ch->name, mod );
            return;
        }
    }
    else {
        xREMOVE_BITS( ch->affected_by, paf->bitvector );

        /*
         * might be an idea to have a duration removespell which returns
         * the spell after the duration... but would have to store
         * the removed spell's information somewhere...  -Thoric
         * (Though we could keep the affect, but disable it for a duration)
         */

        if ( paf->location % REVERSE_APPLY == APPLY_RECURRINGSPELL ) {
            mod = abs( mod );
            if ( !IS_VALID_SN( mod ) || ( skill = skill_table[mod] ) == NULL
                 || skill->type != SKILL_SPELL )
                bug( "affect_modify(%s) APPLY_RECURRINGSPELL with bad sn %d", ch->name, mod );
            xREMOVE_BIT( ch->affected_by, AFF_RECURRINGSPELL );
            return;
        }

        switch ( paf->location % REVERSE_APPLY ) {
            case APPLY_AFFECT:
                REMOVE_BIT( ch->affected_by.bits[0], mod );
                unstun( ch );
                return;

            case APPLY_EXT_AFFECT:
                xREMOVE_BIT( ch->affected_by, mod );
                unstun( ch );
                return;

            case APPLY_RESISTANT:
                REMOVE_BIT( ch->resistant, mod );
                unstun( ch );
                return;

            case APPLY_IMMUNE:
                REMOVE_BIT( ch->immune, mod );
                unstun( ch );
                return;

            case APPLY_SUSCEPTIBLE:
                REMOVE_BIT( ch->susceptible, mod );
                unstun( ch );
                return;

            case APPLY_REMOVE:
                SET_BIT( ch->affected_by.bits[0], mod );
                unstun( ch );
                return;

            default:
                break;
        }
        mod = 0 - mod;
    }

    switch ( paf->location % REVERSE_APPLY ) {
        default:
            bug( "Affect_modify: unknown location %d.", paf->location );
            return;

        case APPLY_NONE:
            break;

        case APPLY_STR:
            ch->mod_str += mod;
            break;

        case APPLY_DEX:
            ch->mod_dex += mod;
            break;

        case APPLY_INT:
            ch->mod_int += mod;
            break;

        case APPLY_WIS:
            ch->mod_wis += mod;
            break;

        case APPLY_CON:
            ch->mod_con += mod;
            break;

        case APPLY_CHA:
            ch->mod_cha += mod;
            break;

        case APPLY_LCK:
            ch->mod_lck += mod;
            break;

        case APPLY_SEX:
            ch->sex = ( ch->sex + mod ) % 3;
            if ( ch->sex < 0 )
                ch->sex += 2;
            ch->sex = URANGE( 0, ch->sex, 2 );
            break;

        case APPLY_CLASS:
            break;
        case APPLY_RACE:
            break;

        case APPLY_AGE:
            break;
            /*
             * Regular apply types
             */
        case APPLY_HEIGHT:
            ch->height += mod;
            break;
        case APPLY_WEIGHT:
            ch->weight += mod;
            break;
        case APPLY_MANA:
            ch->max_mana += mod;
            break;
        case APPLY_HIT:
            ch->max_hit += mod;
            break;
        case APPLY_MOVE:
            ch->max_move += mod;
            break;
        case APPLY_AC:
            ch->armor += mod;
            break;
        case APPLY_HITROLL:
            ch->hitroll += mod;
            break;
        case APPLY_DAMROLL:
            ch->damroll += mod;
            break;
        case APPLY_SAVING_POISON:
            ch->saving_poison_death += mod;
            break;
        case APPLY_SAVING_ROD:
            ch->saving_wand += mod;
            break;
        case APPLY_SAVING_PARA:
            ch->saving_para_petri += mod;
            break;
        case APPLY_SAVING_BREATH:
            ch->saving_breath += mod;
            break;
        case APPLY_SAVING_SPELL:
            ch->saving_spell_staff += mod;
            break;

            /*
             * Bitvector modifying apply types
             */
        case APPLY_AFFECT:
            SET_BIT( ch->affected_by.bits[0], mod );
            break;
        case APPLY_EXT_AFFECT:
            xSET_BIT( ch->affected_by, mod );
            break;
        case APPLY_RESISTANT:
            SET_BIT( ch->resistant, mod );
            break;
        case APPLY_IMMUNE:
            SET_BIT( ch->immune, mod );
            break;
        case APPLY_SUSCEPTIBLE:
            SET_BIT( ch->susceptible, mod );
            break;
        case APPLY_WEAPONSPELL:                       /* see fight.c */
            break;
        case APPLY_REMOVE:
            REMOVE_BIT( ch->affected_by.bits[0], mod );
            break;

            /*
             * Player condition modifiers
             */
        case APPLY_FULL:
            if ( !IS_NPC( ch ) )
                ch->pcdata->condition[COND_FULL] =
                    URANGE( 0, ch->pcdata->condition[COND_FULL] + mod, 48 );
            break;

        case APPLY_THIRST:
            if ( !IS_NPC( ch ) )
                ch->pcdata->condition[COND_THIRST] =
                    URANGE( 0, ch->pcdata->condition[COND_THIRST] + mod, 48 );
            break;

        case APPLY_DRUNK:
            if ( !IS_NPC( ch ) )
                ch->pcdata->condition[COND_DRUNK] =
                    URANGE( 0, ch->pcdata->condition[COND_DRUNK] + mod, 48 );
            break;

        case APPLY_BLOOD:
            if ( !IS_NPC( ch ) )
                ch->pcdata->apply_blood += mod;
            break;

        case APPLY_MENTALSTATE:
            ch->mental_state = URANGE( -100, ch->mental_state + mod, 100 );
            break;
        case APPLY_EMOTION:
            ch->emotional_state = URANGE( -100, ch->emotional_state + mod, 100 );
            break;

            /*
             * Specialty modfiers
             */
        case APPLY_CONTAGIOUS:
            break;
        case APPLY_ODOR:
            break;
        case APPLY_STRIPSN:
            if ( IS_VALID_SN( mod ) )
                affect_strip( ch, mod );
            else
                bug( "affect_modify: APPLY_STRIPSN invalid sn %d", mod );
            break;

            /*
             * spell cast upon wear/removal of an object -Thoric 
             */
        case APPLY_WEARSPELL:
        case APPLY_REMOVESPELL:
            if ( IS_SET( ch->in_room->room_flags, ROOM_NO_MAGIC ) || IS_SET( ch->immune, RIS_MAGIC ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL && !fAdd ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL && !fAdd ) || saving_char == ch   /* so 
                                                                                                                                                                                                                                                                         * save/quit 
                                                                                                                                                                                                                                                                         * doesn't 
                                                                                                                                                                                                                                                                         * trigger 
                                                                                                                                                                                                                                                                         */
                 || loading_char == ch )               /* so loading doesn't trigger */
                return;

            mod = abs( mod );
            if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
                 && skill->type == SKILL_SPELL ) {
                if ( skill->target == TAR_IGNORE || skill->target == TAR_OBJ_INV ) {
                    bug( "APPLY_WEARSPELL trying to apply bad target spell.  SN is %d.", mod );
                    return;
                }
                if ( ( retcode = ( *skill->spell_fun ) ( mod, ch->level, ch, ch ) ) == rCHAR_DIED
                     || char_died( ch ) )
                    return;
            }
            break;

            /*
             * Skill apply types
             */
        case APPLY_PERCENTAGE:
            break;

        case APPLY_R1:
        case APPLY_R2:
        case APPLY_R3:
        case APPLY_R4:
        case APPLY_R5:
        case APPLY_R6:
        case APPLY_R7:
        case APPLY_R8:
        case APPLY_R9:
        case APPLY_R10:
        case APPLY_R11:
        case APPLY_R12:
        case APPLY_R13:
        case APPLY_R14:
        case APPLY_R15:
        case APPLY_R16:
        case APPLY_R17:
        case APPLY_R18:
        case APPLY_R19:
        case APPLY_R20:
        case APPLY_R21:
        case APPLY_R22:
        case APPLY_R23:
        case APPLY_R24:
        case APPLY_R25:
        case APPLY_R26:
        case APPLY_R27:
        case APPLY_R28:
        case APPLY_R29:
        case APPLY_R30:
        case APPLY_R31:
            break;

            /*
             * Room apply types
             */
        case APPLY_ROOMFLAG:
        case APPLY_SECTORTYPE:
        case APPLY_ROOMLIGHT:
        case APPLY_TELEVNUM:
            break;

            /*
             * Object apply types
             */
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if ( !IS_NPC( ch ) && saving_char != ch && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL
         && get_obj_weight( wield, FALSE ) > str_app[get_curr_str( ch )].wield ) {
        static int              depth;

        if ( depth == 0 ) {
            depth++;
            act( AT_ACTION, "You are too weak to wield $p any longer.", ch, wield, NULL, TO_CHAR );
            act( AT_ACTION, "$n stops wielding $p.", ch, wield, NULL, TO_ROOM );
            unequip_char( ch, wield );
            depth--;
        }
    }

    unstun( ch );
}

/*
 * Give an affect to a char.
 * Volk - why not give blind_sight to blindsight then?
 */

void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA            *paf_new;

    if ( !ch ) {
        bug( "Affect_to_char(NULL, %d)", paf ? paf->type : 0 );
        return;
    }

    if ( !paf ) {
        bug( "Affect_to_char(%s, NULL)", ch->name );
        return;
    }

    CREATE( paf_new, AFFECT_DATA, 1 );
    LINK( paf_new, ch->first_affect, ch->last_affect, next, prev );
    paf_new->type = paf->type;
    paf_new->level = paf->level;
    paf_new->duration = paf->duration;
    paf_new->location = paf->location;
    paf_new->modifier = paf->modifier;
    paf_new->bitvector = paf->bitvector;
    affect_modify( ch, paf_new, TRUE );
}

/* Remove an affect from a char. */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    if ( !ch->first_affect ) {
        bug( "Affect_remove(%s, %d): no affect.", ch->name, paf ? paf->type : 0 );
        return;
    }

    affect_modify( ch, paf, FALSE );

    UNLINK( paf, ch->first_affect, ch->last_affect, next, prev );
    DISPOSE( paf );
    unstun( ch );
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA            *paf;
    AFFECT_DATA            *paf_next;

    if ( sn == gsn_human_form )
        humanform_change( ch, FALSE );

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;
        if ( paf->type == sn )
            affect_remove( ch, paf );
    }
    unstun( ch );
}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA            *paf;

    for ( paf = ch->first_affect; paf; paf = paf->next )
        if ( paf->type == sn )
            return TRUE;

    return FALSE;
}

/*
 * Add or enhance an affect.
 * Limitations put in place by Thoric, they may be high... but at least
 * they're there :)
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA            *paf_old;

    for ( paf_old = ch->first_affect; paf_old; paf_old = paf_old->next )
        if ( paf_old->type == paf->type ) {
            paf->duration = UMIN( 1000000, paf->duration + paf_old->duration );
            if ( paf->modifier )
                paf->modifier = UMIN( 5000, paf->modifier + paf_old->modifier );
            else
                paf->modifier = paf_old->modifier;
            affect_remove( ch, paf_old );
            break;
        }

    affect_to_char( ch, paf );
    return;
}

/*
 * Apply only affected and RIS on a char
 */
void aris_affect( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    xSET_BITS( ch->affected_by, paf->bitvector );
    switch ( paf->location % REVERSE_APPLY ) {
        case APPLY_AFFECT:
            SET_BIT( ch->affected_by.bits[0], paf->modifier );
            break;
        case APPLY_RESISTANT:
            SET_BIT( ch->resistant, paf->modifier );
            break;
        case APPLY_IMMUNE:
            SET_BIT( ch->immune, paf->modifier );
            break;
        case APPLY_SUSCEPTIBLE:
            SET_BIT( ch->susceptible, paf->modifier );
            break;
    }
}

/*
 * Update affecteds and RIS for a character in case things get messed.
 * This should only really be used as a quick fix until the cause
 * of the problem can be hunted down. - FB
 * Last modified: June 30, 1997
 *
 * Quick fix?  Looks like a good solution for a lot of problems.
 */

void silent_land( CHAR_DATA *ch )
{
    AFFECT_DATA            *paf,
                           *paf_next;

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;
        if ( xIS_SET( paf->bitvector, AFF_FLYING ) || xIS_SET( paf->bitvector, AFF_FLOATING ) )
            affect_remove( ch, paf );
    }

    xREMOVE_BIT( ch->affected_by, AFF_FLYING );
    xREMOVE_BIT( ch->affected_by, AFF_FLOATING );
}

/* Temp mod to bypass immortals so they can keep their mset affects,
 * just a band-aid until we get more time to look at it -- Blodkai */
void update_aris( CHAR_DATA *ch )
{
    AFFECT_DATA            *paf;
    OBJ_DATA               *obj;
    int                     hiding;
    bool                    flying = FALSE;

    if ( IS_NPC( ch ) || IS_IMMORTAL( ch ) )
        return;

    /*
     * So chars using hide skill will continue to hide 
     */
    hiding = IS_AFFECTED( ch, AFF_HIDE );
    flying = IS_AFFECTED( ch, AFF_FLYING );

    xCLEAR_BITS( ch->affected_by );
    ch->resistant = 0;
    ch->immune = 0;
    ch->susceptible = 0;
    xCLEAR_BITS( ch->no_affected_by );
    ch->no_resistant = 0;
    ch->no_immune = 0;
    ch->no_susceptible = 0;

    /*
     * Add in effects from race 
     */
    xSET_BITS( ch->affected_by, race_table[ch->race]->affected );
    SET_BIT( ch->resistant, race_table[ch->race]->resist );
    SET_BIT( ch->susceptible, race_table[ch->race]->suscept );

    /*
     * Add in effects from class 
     */
    if ( class_table[ch->Class] ) {
        xSET_BITS( ch->affected_by, class_table[ch->Class]->affected );
        SET_BIT( ch->resistant, class_table[ch->Class]->resist );
        SET_BIT( ch->susceptible, class_table[ch->Class]->suscept );
    }
    // Added in Secondclass and Thirdclass support -Taon
    if ( IS_SECONDCLASS( ch ) && class_table[ch->secondclass] ) {
        xSET_BITS( ch->affected_by, class_table[ch->secondclass]->affected );
        SET_BIT( ch->resistant, class_table[ch->secondclass]->resist );
        SET_BIT( ch->susceptible, class_table[ch->secondclass]->suscept );
    }
    if ( IS_THIRDCLASS( ch ) && class_table[ch->thirdclass] ) {
        xSET_BITS( ch->affected_by, class_table[ch->thirdclass]->affected );
        SET_BIT( ch->resistant, class_table[ch->thirdclass]->resist );
        SET_BIT( ch->susceptible, class_table[ch->thirdclass]->suscept );
    }

    /*
     * Add in effects from deities 
     */
    if ( ch->pcdata->deity ) {
        if ( ch->pcdata->favor > ch->pcdata->deity->affectednum )
            xSET_BITS( ch->affected_by, ch->pcdata->deity->affected );
        if ( ch->pcdata->favor > ch->pcdata->deity->elementnum )
            SET_BIT( ch->resistant, ch->pcdata->deity->element );
        if ( ch->pcdata->favor < ch->pcdata->deity->susceptnum )
            SET_BIT( ch->susceptible, ch->pcdata->deity->suscept );
    }

    /*
     * Add in effect from spells 
     */
    for ( paf = ch->first_affect; paf; paf = paf->next )
        aris_affect( ch, paf );

    /*
     * Add in effects from equipment 
     */
    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( obj->wear_loc != WEAR_NONE ) {
            for ( paf = obj->first_affect; paf; paf = paf->next )
                aris_affect( ch, paf );

            for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
                aris_affect( ch, paf );
        }
    }

    /*
     * Add in effects from the room 
     */
    if ( ch->in_room )                                 /* non-existant char booboo-fix
                                                        * --TRI */
        for ( paf = ch->in_room->first_affect; paf; paf = paf->next )
            aris_affect( ch, paf );

    /*
     * WINGS wearoff message 
     */

    /*
     * Add in effects for polymorph 
     */
    if ( ch->morph ) {
        xSET_BITS( ch->affected_by, ch->morph->affected_by );
        SET_BIT( ch->immune, ch->morph->immune );
        SET_BIT( ch->resistant, ch->morph->resistant );
        SET_BIT( ch->susceptible, ch->morph->suscept );
        /*
         * Right now only morphs have no_ things --Shaddai 
         */
        xSET_BITS( ch->no_affected_by, ch->morph->no_affected_by );
        SET_BIT( ch->no_immune, ch->morph->no_immune );
        SET_BIT( ch->no_resistant, ch->morph->no_resistant );
        SET_BIT( ch->no_susceptible, ch->morph->no_suscept );
    }

    /*
     * If they were hiding before, make them hiding again 
     */
    if ( hiding )
        xSET_BIT( ch->affected_by, AFF_HIDE );

    if ( flying )
        xSET_BIT( ch->affected_by, AFF_FLYING );

    if ( IS_AFFECTED( ch, AFF_BURROW ) || ch->position == POS_MEDITATING )
        silent_land( ch );
}

/* Move a char out of a room. */
void char_from_room( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    AFFECT_DATA            *paf;
    ROOM_INDEX_DATA        *room;

    if ( !ch->in_room ) {
        bug( "%s", "Char_from_room: NULL." );
        return;
    }

    if ( !IS_NPC( ch ) )
        --ch->in_room->area->nplayer;

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT
         && obj->value[2] != 0 && ch->in_room->light > 0 )
        --ch->in_room->light;

    /*
     * Character's affect on the room 
     */
    for ( paf = ch->first_affect; paf; paf = paf->next )
        room_affect( ch->in_room, paf, FALSE );

    room = ch->in_room;

    UNLINK( ch, ch->in_room->first_person, ch->in_room->last_person, next_in_room, prev_in_room );
    ch->was_in_room = ch->in_room;
    ch->in_room = NULL;
    ch->next_in_room = NULL;
    ch->prev_in_room = NULL;

    /*
     * Room's affect on the character 
     */
    if ( !char_died( ch ) ) {
        for ( paf = room->first_affect; paf; paf = paf->next )
            affect_modify( ch, paf, FALSE );

        if ( char_died( ch ) )                         /* could die from removespell, etc 
                                                        */
            return;
    }

    if ( !IS_NPC( ch ) && get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
        remove_timer( ch, TIMER_SHOVEDRAG );
}

 /*
  * check to see if the area a character has moved into
  * is new to them and grant exp if so
  */
void discover_area( CHAR_DATA *ch, AREA_DATA *area )
{
    FOUND_AREA             *found,
                           *fnext;
    int                     exp;

    if ( area == NULL || ch == NULL || ch->pcdata == NULL )
        return;

    for ( found = ch->pcdata->first_area; found; found = fnext ) {
        fnext = found->next;

        if ( !found->area_name )
            continue;

        if ( strcmp( found->area_name, area->name ) == 0 )
            return;
    }

    CREATE( found, FOUND_AREA, 1 );
    found->area_name = STRALLOC( area->name );
    if ( found->area_name )
        LINK( found, ch->pcdata->first_area, ch->pcdata->last_area, next, prev );
    else {
        DISPOSE( found );
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        exp = ( 1000 * ( area->low_soft_range + area->hi_soft_range ) );
        if ( exp > 30000 ) {
            exp = 30000;
        }
        if ( ch->level < 5 ) {
            exp = number_range( 2000, 5000 );
        }
        else if ( ch->level < 10 ) {
            exp = number_range( 5000, 10000 );
        }
    }
    ch_printf( ch, "&OYou have discovered a new area called %s!&d\r\n", area->name );
    gain_exp( ch, exp );
}

/* Move a char into a room. */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    OBJ_DATA               *obj;
    AFFECT_DATA            *paf;

    if ( !ch ) {
        bug( "%s: NULL ch!", __FUNCTION__ );
        return;
    }

    if ( !pRoomIndex || !get_room_index( pRoomIndex->vnum ) ) {
        bug( "%s: %s -> NULL room!  Putting char in limbo (%d)", __FUNCTION__, ch->name,
             ROOM_VNUM_LIMBO );
        /*
         * This used to just return, but there was a problem with crashing
         * and I saw no reason not to just put the char in limbo.  -Narn
         */
        pRoomIndex = get_room_index( ROOM_VNUM_LIMBO );
    }

    ch->in_room = pRoomIndex;
    if ( ch->home_vnum < 1 )
        ch->home_vnum = ch->in_room->vnum;

    if ( !IS_NPC( ch ) && ( !IS_SET( pRoomIndex->area->flags, AFLAG_NODISCOVERY ) ) )
        discover_area( ch, pRoomIndex->area );

    /*
     * VOLK: is THIS causig our problems? 
     */
    LINK( ch, pRoomIndex->first_person, pRoomIndex->last_person, next_in_room, prev_in_room );

    if ( !IS_NPC( ch ) )
        if ( ++pRoomIndex->area->nplayer > pRoomIndex->area->max_players )
            pRoomIndex->area->max_players = pRoomIndex->area->nplayer;

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT
         && obj->value[2] != 0 )
        ++pRoomIndex->light;

    /*
     * Room's effect on the character 
     */
    if ( !char_died( ch ) ) {
        for ( paf = pRoomIndex->first_affect; paf; paf = paf->next )
            affect_modify( ch, paf, TRUE );

        if ( char_died( ch ) )                         /* could die from a wearspell, etc 
                                                        */
            return;
    }

    /*
     * Character's effect on the room 
     */
    for ( paf = ch->first_affect; paf; paf = paf->next )
        room_affect( pRoomIndex, paf, TRUE );

    if ( !IS_NPC( ch ) && IS_SET( pRoomIndex->room_flags, ROOM_SAFE )
         && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
        add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );/*-30 Seconds-*/

    /*
     * Delayed Teleport rooms     -Thoric
     * Should be the last thing checked in this function
     */
    if ( IS_SET( pRoomIndex->room_flags, ROOM_TELEPORT ) && pRoomIndex->tele_delay > 0 ) {
        TELEPORT_DATA          *tele;

        for ( tele = first_teleport; tele; tele = tele->next )
            if ( tele->room == pRoomIndex )
                return;

        CREATE( tele, TELEPORT_DATA, 1 );
        LINK( tele, first_teleport, last_teleport, next, prev );
        tele->room = pRoomIndex;
        tele->timer = pRoomIndex->tele_delay;
    }

    if ( !ch->was_in_room )
        ch->was_in_room = ch->in_room;
}

void free_teleports( void )
{
    TELEPORT_DATA          *tele,
                           *tele_next;

    for ( tele = first_teleport; tele; tele = tele_next ) {
        tele_next = tele->next;

        UNLINK( tele, first_teleport, last_teleport, next, prev );
        DISPOSE( tele );
    }
}

/* Give an obj to a char. */
OBJ_DATA               *obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    OBJ_DATA               *otmp,
                           *obj_next;
    OBJ_DATA               *oret = obj;
    bool                    skipgroup,
                            grouped;
    int                     oweight = get_obj_weight( obj, FALSE );
    int                     onum = get_obj_number( obj );
    int                     wear_loc = obj->wear_loc;
    EXT_BV                  extra_flags = obj->extra_flags;
    int                     weight;

    skipgroup = FALSE;
    grouped = FALSE;

    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
        if ( !IS_IMMORTAL( ch ) && ( IS_NPC( ch ) && !xIS_SET( ch->act, ACT_PROTOTYPE ) ) )
            return obj_to_room( obj, ch->in_room );
    }

    /*
     * Reset information should be set after the object is given to who it needs to be
     * given to, so here lets free it up so it can reset later 
     */
    if ( obj->reset && !IS_NPC( ch ) ) {
        obj->reset->obj = NULL;
        obj->reset = NULL;
    }

    if ( loading_char == ch ) {
        int                     x,
                                y;

        for ( x = 0; x < MAX_WEAR; x++ )
            for ( y = 0; y < MAX_LAYERS; y++ )
                if ( save_equipment[x][y] == obj ) {
                    skipgroup = TRUE;
                    break;
                }
    }
    if ( IS_NPC( ch ) && ch->pIndexData->pShop )
        skipgroup = TRUE;
    if ( IS_OBJ_STAT( obj, ITEM_NOGROUP ) )
        skipgroup = TRUE;
    if ( obj->reset )
        skipgroup = TRUE;

    if ( !skipgroup ) {
        for ( otmp = ch->first_carrying; otmp; otmp = obj_next ) {
            obj_next = otmp->next_content;
            if ( ( oret = group_object( otmp, obj ) ) == otmp ) {
                grouped = TRUE;
                break;
            }
        }
    }
    if ( !grouped ) {
        if ( !IS_NPC( ch ) || !ch->pIndexData->pShop ) {
            LINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );
            obj->carried_by = ch;
            obj->in_room = NULL;
            obj->in_obj = NULL;
        }
        else {
            /*
             * If ch is a shopkeeper, add the obj using an insert sort 
             */
            for ( otmp = ch->first_carrying; otmp; otmp = otmp->next_content ) {
                if ( obj->level > otmp->level ) {
                    INSERT( obj, otmp, ch->first_carrying, next_content, prev_content );
                    break;
                }
                else if ( obj->level == otmp->level
                          && strcmp( obj->short_descr, otmp->short_descr ) < 0 ) {
                    INSERT( obj, otmp, ch->first_carrying, next_content, prev_content );
                    break;
                }
            }
            if ( !otmp ) {
                LINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );
            }
            obj->carried_by = ch;
            obj->in_room = NULL;
            obj->in_obj = NULL;
        }
    }
    if ( wear_loc == WEAR_NONE ) {
        ch->carry_number += onum;
        ch->carry_weight += is_magic_container( obj ) ? obj->weight : oweight;
    }
    else if ( !xIS_SET( extra_flags, ITEM_MAGIC ) )
        ch->carry_weight += oweight;

    return ( oret ? oret : obj );
}

/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA *obj )
{
    CHAR_DATA              *ch;

    if ( ( ch = obj->carried_by ) == NULL ) {
        bug( "%s", "Obj_from_char: null ch." );
        return;
    }

    if ( obj->wear_loc != WEAR_NONE )
        unequip_char( ch, obj );

    /*
     * obj may drop during unequip... 
     */
    if ( !obj->carried_by )
        return;

    if ( ch )
        UNLINK( obj, ch->first_carrying, ch->last_carrying, next_content, prev_content );

    if ( IS_OBJ_STAT( obj, ITEM_COVERING ) && obj->first_content )
        empty_obj( obj, NULL, NULL, FALSE );

    if ( ch ) {
        ch->carry_number -= get_obj_number( obj );
        ch->carry_weight -= get_obj_weight( obj, FALSE );
    }

    obj->in_room = NULL;
    obj->carried_by = NULL;
    return;
}

/*
 * Find the ac value of an obj, including position effect.
 */

int apply_ac( OBJ_DATA *obj, int iWear )
{
    if ( obj->item_type != ITEM_ARMOR )
        return 0;

    switch ( iWear ) {
        case WEAR_BODY:
            return obj->value[0];
        case WEAR_HEAD:
            return obj->value[0];
        case WEAR_LEGS:
            return obj->value[0];
        case WEAR_FEET:
            return obj->value[0];
        case WEAR_HANDS:
            return obj->value[0];
        case WEAR_ARMS:
            return obj->value[0];
        case WEAR_SHIELD:
            return obj->value[0];
        case WEAR_FINGER_L:
            return obj->value[0];
        case WEAR_FINGER_R:
            return obj->value[0];
        case WEAR_NECK_1:
            return obj->value[0];
        case WEAR_NECK_2:
            return obj->value[0];
        case WEAR_ABOUT:
            return obj->value[0];
        case WEAR_WAIST:
            return obj->value[0];
        case WEAR_WRIST_L:
            return obj->value[0];
        case WEAR_WRIST_R:
            return obj->value[0];
        case WEAR_HOLD:
            return obj->value[0];
        case WEAR_EYES:
            return obj->value[0];
        case WEAR_FACE:
            return obj->value[0];
        case WEAR_BACK:
            return obj->value[0];
        case WEAR_ANKLE_L:
            return obj->value[0];
        case WEAR_ANKLE_R:
            return obj->value[0];
    }

    return 0;
}

/*
 * Find a piece of eq on a character.
 * Will pick the top layer if clothing is layered.  -Thoric
 */
OBJ_DATA               *get_eq_char( CHAR_DATA *ch, int iWear )
{
    OBJ_DATA               *obj,
                           *maxobj = NULL;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( obj->wear_loc == iWear ) {
            if ( !obj->pIndexData->layers ) {
                return obj;
            }
            else if ( !maxobj || obj->pIndexData->layers > maxobj->pIndexData->layers ) {
                maxobj = obj;
            }
        }
    }

    return maxobj;
}

/* Equip a char with an obj. */
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
    AFFECT_DATA            *paf;
    OBJ_DATA               *otmp;

    if ( ( otmp = get_eq_char( ch, iWear ) ) != NULL
         && ( !otmp->pIndexData->layers || !obj->pIndexData->layers ) ) {
        bug( "Equip_char: already equipped (%d).", iWear );
        return;
    }

    if ( obj->carried_by != ch ) {
        bug( "%s", "equip_char: obj not being carried by ch!" );
        return;
    }

    separate_obj( obj );                               /* just in case */
    if ( ( IS_OBJ_STAT( obj, ITEM_ANTI_EVIL ) && IS_EVIL( ch ) )
         || ( IS_OBJ_STAT( obj, ITEM_ANTI_GOOD ) && IS_GOOD( ch ) )
         || ( IS_OBJ_STAT( obj, ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL( ch ) ) ) {
        /*
         * Thanks to Morgenes for the bug fix here!
         */
        if ( loading_char != ch ) {
            act( AT_MAGIC, "You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR );
            act( AT_MAGIC, "$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM );
        }
        if ( obj->carried_by )
            obj_from_char( obj );
        obj_to_room( obj, ch->in_room );
        oprog_zap_trigger( ch, obj );
        if ( xIS_SET( sysdata.save_flags, SV_ZAPDROP ) && !char_died( ch ) )
            save_char_obj( ch );
        return;
    }

    ch->armor -= apply_ac( obj, iWear );
    obj->wear_loc = iWear;
    ch->carry_number -= get_obj_number( obj );

    if ( iWear != WEAR_SHEATH ) {
        if ( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
            ch->carry_weight -= obj->weight;
        for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
            affect_modify( ch, paf, TRUE );
        for ( paf = obj->first_affect; paf; paf = paf->next )
            affect_modify( ch, paf, TRUE );

        if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room )
            ++ch->in_room->light;
    }
}

/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
    AFFECT_DATA            *paf;

    if ( obj->wear_loc == WEAR_NONE ) {
        bug( "%s", "Unequip_char: already unequipped." );
        return;
    }

    ch->carry_number += get_obj_number( obj );
    if ( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        ch->carry_weight += obj->weight;

    ch->armor += apply_ac( obj, obj->wear_loc );

    if ( obj->wear_loc != WEAR_SHEATH ) {
        for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
            affect_modify( ch, paf, FALSE );
        if ( obj->carried_by )
            for ( paf = obj->first_affect; paf; paf = paf->next )
                affect_modify( ch, paf, FALSE );
    }

    obj->wear_loc = -1;

    update_aris( ch );

    if ( !obj->carried_by )
        return;

    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room
         && ch->in_room->light > 0 )
        --ch->in_room->light;

    return;
}

/*
 * Move an obj out of a room.
 */
void write_corpses      args( ( CHAR_DATA *ch, char *name, OBJ_DATA *objrem ) );

int                     falling;

void obj_from_room( OBJ_DATA *obj )
{
    ROOM_INDEX_DATA        *in_room;
    AFFECT_DATA            *paf;

    if ( ( in_room = obj->in_room ) == NULL ) {
        bug( "obj_from_room: Null in_room  for obj vnum %d.", obj ? obj->pIndexData->vnum : 0 );
        return;
    }

    for ( paf = obj->first_affect; paf; paf = paf->next )
        room_affect( in_room, paf, FALSE );

    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
        room_affect( in_room, paf, FALSE );

    UNLINK( obj, in_room->first_content, in_room->last_content, next_content, prev_content );

    /*
     * uncover contents 
     */
    if ( IS_OBJ_STAT( obj, ITEM_COVERING ) && obj->first_content )
        empty_obj( obj, NULL, obj->in_room, FALSE );

    if ( obj->item_type == ITEM_FIRE )
        obj->in_room->light -= obj->count;

    obj->carried_by = NULL;
    obj->in_obj = NULL;
    obj->in_room = NULL;
    if ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
        write_corpses( NULL, obj->short_descr + 14, obj );
    return;
}

/*
 * Move an obj into a room.
 */
OBJ_DATA               *obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    OBJ_DATA               *otmp,
                           *oret;
    short                   count = obj->count;
    short                   item_type = obj->item_type;
    AFFECT_DATA            *paf;
    bool                    skipgroup = FALSE;

    if ( !pRoomIndex ) {
        bug( "obj_to_room: Null pRoomIndex for obj vnum %d.", obj ? obj->pIndexData->vnum : 0 );
        if ( obj )
            extract_obj( obj );
        return NULL;
    }

    for ( paf = obj->first_affect; paf; paf = paf->next )
        room_affect( pRoomIndex, paf, TRUE );

    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
        room_affect( pRoomIndex, paf, TRUE );

    if ( IS_OBJ_STAT( obj, ITEM_NOGROUP ) )
        skipgroup = TRUE;

    if ( !skipgroup ) {
        for ( otmp = pRoomIndex->first_content; otmp; otmp = otmp->next_content )
            if ( ( oret = group_object( otmp, obj ) ) == otmp ) {
                if ( item_type == ITEM_FIRE )
                    pRoomIndex->light += count;
                return oret;
            }
    }

    /*
     * Reset information should be set after the object is put in room, so here lets free
     * it up so it can reset later 
     */
    if ( obj->reset )
        obj->reset->obj = NULL;
    obj->reset = NULL;
    LINK( obj, pRoomIndex->first_content, pRoomIndex->last_content, next_content, prev_content );
    obj->in_room = pRoomIndex;
    obj->carried_by = NULL;
    obj->in_obj = NULL;
    obj->room_vnum = pRoomIndex->vnum;                 /* copyover tracker */
    if ( item_type == ITEM_FIRE )
        pRoomIndex->light += count;
    falling++;
    obj_fall( obj, FALSE );
    falling--;
    if ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
        write_corpses( NULL, obj->short_descr + 14, NULL );
    return obj;
}

/*
 * Move an object into an object.
 */
OBJ_DATA               *obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
    OBJ_DATA               *otmp,
                           *oret;
    CHAR_DATA              *who;
    bool                    skipgroup = FALSE;

    if ( obj == obj_to ) {
        bug( "Obj_to_obj: trying to put object inside itself: vnum %d", obj->pIndexData->vnum );
        return obj;
    }

    /*
     * Reset information should be set after the object is put where it needs to be, so
     * here lets free it up so it can reset later 
     */
    if ( obj->reset && !obj_to->reset ) {
        obj->reset->obj = NULL;
        obj->reset = NULL;
    }

    if ( ( who = carried_by( obj_to ) ) != NULL && !in_magic_container( obj_to ) )
        who->carry_weight += get_obj_weight( obj, FALSE );

    if ( IS_OBJ_STAT( obj, ITEM_NOGROUP ) )
        skipgroup = TRUE;

    if ( !skipgroup )
        for ( otmp = obj_to->first_content; otmp; otmp = otmp->next_content )
            if ( ( oret = group_object( otmp, obj ) ) == otmp )
                return oret;

    LINK( obj, obj_to->first_content, obj_to->last_content, next_content, prev_content );

    obj->in_obj = obj_to;
    obj->in_room = NULL;
    obj->carried_by = NULL;

    return obj;
}

/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA *obj )
{
    OBJ_DATA               *obj_from;

    if ( ( obj_from = obj->in_obj ) == NULL ) {
        bug( "%s", "Obj_from_obj: null obj_from." );
        return;
    }

    UNLINK( obj, obj_from->first_content, obj_from->last_content, next_content, prev_content );

    /*
     * uncover contents 
     */
    if ( IS_OBJ_STAT( obj, ITEM_COVERING ) && obj->first_content )
        empty_obj( obj, obj->in_obj, NULL, FALSE );

    obj->in_obj = NULL;
    obj->in_room = NULL;
    obj->carried_by = NULL;

    for ( ; obj_from; obj_from = obj_from->in_obj ) {
        if ( obj_from->carried_by && !in_magic_container( obj_from ) )
            obj_from->carried_by->carry_weight -= get_obj_weight( obj, FALSE );
    }

    return;
}

/* Will count users on furniture -- Xerves */
int count_users( OBJ_DATA *obj )
{
    CHAR_DATA              *fch;
    int                     count = 0;

    if ( obj->in_room == NULL )
        return 0;

    for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room )
        if ( fch->on == obj )
            count++;

    return count;
}

int max_weight( OBJ_DATA *obj )
{
    CHAR_DATA              *fch;
    int                     weight = 0;

    if ( obj->in_room == NULL )
        return 200000;

    for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room )
        if ( fch->on == obj )
            weight = weight + fch->weight;

    return weight;
}

extern OBJ_DATA        *dragonegg;

/* Extract an obj from the world. */
void extract_obj( OBJ_DATA *obj )
{
    OBJ_DATA               *obj_content;
    REL_DATA               *RQueue,
                           *rq_next;

    if ( obj_extracted( obj ) ) {
        bug( "extract_obj: obj %d already extracted!", obj->pIndexData->vnum );
        return;
    }

    if ( obj->reset )
        obj->reset->obj = NULL;
    obj->reset = NULL;

    if ( dragonegg && obj == dragonegg )
        dragonegg = NULL;

    if ( obj->item_type == ITEM_PORTAL )
        remove_portal( obj );
    if ( obj->carried_by )
        obj_from_char( obj );
    else if ( obj->in_room )
        obj_from_room( obj );
    else if ( obj->in_obj )
        obj_from_obj( obj );
    while ( ( obj_content = obj->last_content ) != NULL )
        extract_obj( obj_content );

    /*
     * remove affects 
     */
    {
        AFFECT_DATA            *paf;
        AFFECT_DATA            *paf_next;

        for ( paf = obj->first_affect; paf; paf = paf_next ) {
            paf_next = paf->next;
            DISPOSE( paf );
        }
        obj->first_affect = obj->last_affect = NULL;
    }
    /*
     * remove extra descriptions 
     */
    {
        EXTRA_DESCR_DATA       *ed;
        EXTRA_DESCR_DATA       *ed_next;

        for ( ed = obj->first_extradesc; ed; ed = ed_next ) {
            ed_next = ed->next;
            if ( VLD_STR( ed->description ) )
                STRFREE( ed->description );
            if ( VLD_STR( ed->keyword ) )
                STRFREE( ed->keyword );
            DISPOSE( ed );
        }
        obj->first_extradesc = obj->last_extradesc = NULL;
    }
    if ( obj == gobj_prev )
        gobj_prev = obj->prev;
    for ( RQueue = first_relation; RQueue; RQueue = rq_next ) {
        rq_next = RQueue->next;
        if ( RQueue->Type == relOSET_ON ) {
            if ( obj == RQueue->Subject )
                ( ( CHAR_DATA * ) RQueue->Actor )->dest_buf = NULL;
            else
                continue;
            UNLINK( RQueue, first_relation, last_relation, next, prev );
            DISPOSE( RQueue );
        }
    }
    UNLINK( obj, first_object, last_object, next, prev );
    /*
     * shove onto extraction queue 
     */
    queue_extracted_obj( obj );
    obj->pIndexData->count -= obj->count;
    numobjsloaded -= obj->count;
    --physicalobjects;
    if ( obj->serial == cur_obj ) {
        cur_obj_extracted = TRUE;
        if ( global_objcode == rNONE )
            global_objcode = rOBJ_EXTRACTED;
    }
}

/* Extract a char from the world. */
void extract_char( CHAR_DATA *ch, bool fPull )
{
    CHAR_DATA              *wch;
    OBJ_DATA               *obj;
    char                    buf[MSL];
    ROOM_INDEX_DATA        *location;
    REL_DATA               *RQueue,
                           *rq_next;

    if ( !ch ) {
        bug( "%s: NULL ch.", __FUNCTION__ );
        return;
    }
    if ( !ch->in_room ) {
        bug( "%s: %s in NULL room.", __FUNCTION__, ch->name ? ch->name : "???" );
        return;
    }
    if ( ch == supermob ) {
        bug( "%s: ch == supermob!", __FUNCTION__ );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_ARENA ) ) {
        ch->hit = ch->max_hit;
        ch->mana = ch->max_mana;
        ch->move = ch->max_move;
    }

    if ( char_died( ch ) ) {
        bug( "%s: %s already died!", __FUNCTION__, ch->name );
        return;
    }

    if ( ch == cur_char )
        cur_char_died = TRUE;
    /*
     * shove onto extraction queue 
     */
    queue_extracted_char( ch, fPull );
    for ( RQueue = first_relation; RQueue; RQueue = rq_next ) {
        rq_next = RQueue->next;
        if ( fPull && RQueue->Type == relMSET_ON ) {
            if ( ch == RQueue->Subject )
                ( ( CHAR_DATA * ) RQueue->Actor )->dest_buf = NULL;
            else if ( ch != RQueue->Actor )
                continue;
            UNLINK( RQueue, first_relation, last_relation, next, prev );
            DISPOSE( RQueue );
        }
    }
    if ( gch_prev == ch )
        gch_prev = ch->prev;
    if ( fPull )
        die_follower( ch );
    stop_fighting( ch, TRUE );

    if ( ch->mount ) {
        xREMOVE_BIT( ch->mount->act, ACT_MOUNTED );
        ch->mount = NULL;
        set_position( ch, POS_STANDING );
    }

    if ( IS_NPC( ch ) ) {
        xREMOVE_BIT( ch->act, ACT_MOUNTED );
    }

    /*
     * check if this NPC was a mount or a pet 
     */
    if ( IS_NPC( ch ) ) {
        if ( ch->reset )
            ch->reset->ch = NULL;
        ch->reset = NULL;
        for ( wch = first_char; wch; wch = wch->next ) {
            if ( wch->mount == ch ) {
                wch->mount = NULL;
                set_position( wch, POS_STANDING );
                if ( wch->in_room == ch->in_room ) {
                    act( AT_SOCIAL, "Your faithful mount, $N collapses beneath you...", wch, NULL,
                         ch, TO_CHAR );
                    act( AT_SOCIAL, "Sadly you dismount $M for the last time.", wch, NULL, ch,
                         TO_CHAR );
                    act( AT_PLAIN, "$n sadly dismounts $N for the last time.", wch, NULL, ch,
                         TO_ROOM );
                }
            }
            if ( wch->pcdata && wch->pcdata->pet == ch ) {
                xREMOVE_BIT( wch->act, PLR_BOUGHT_PET );
                wch->pcdata->pet = NULL;
                if ( wch->in_room == ch->in_room )
                    act( AT_SOCIAL, "You mourn for the loss of $N.", wch, NULL, ch, TO_CHAR );
            }
        }
    }

    xREMOVE_BIT( ch->act, ACT_MOUNTED );
    while ( ( obj = ch->last_carrying ) != NULL )
        extract_obj( obj );
    char_from_room( ch );
    if ( !fPull ) {
        location = NULL;

        if ( !IS_NPC( ch ) && ch->pcdata->clan )
            location = get_room_index( ch->pcdata->clan->recall );
        if ( !IS_NPC( ch ) && ch->pcdata->htown )
            location = get_room_index( ch->pcdata->htown->temple );
        if ( !location )
            location = get_room_index( ROOM_VNUM_ALTAR );
        if ( !location )
            location = get_room_index( 1 );
        char_to_room( ch, location );
        /*
         * Make things a little fancier     -Thoric 
         */
        if ( ( wch = get_char_room( ch, ( char * ) "healer" ) ) != NULL ) {
            act( AT_MAGIC, "$n mutters a few incantations, waves $s hands and points $s finger.",
                 wch, NULL, NULL, TO_ROOM );
            act( AT_MAGIC, "$n appears from some strange swirling mists!", ch, NULL, NULL,
                 TO_ROOM );
            snprintf( buf, MSL, "say Welcome back to the land of the living, %s\r\n",
                      capitalize( ch->name ) );
            snprintf( buf, MSL,
                      "say %s, if you need it there is an undertaker that can get your corpse.\r\n",
                      capitalize( ch->name ) );
            interpret( wch, buf );
        }
        else
            act( AT_MAGIC, "$n appears from some strange swirling mists!", ch, NULL, NULL,
                 TO_ROOM );
        set_position( ch, POS_RESTING );
        return;
    }
    if ( IS_NPC( ch ) ) {
        --ch->pIndexData->count;
        --nummobsloaded;
    }
    if ( ch->desc && ch->desc->original )
        do_return( ch, ( char * ) "" );
    for ( wch = first_char; wch; wch = wch->next ) {
        if ( wch->reply == ch )
            wch->reply = NULL;
        if ( wch->retell == ch )
            wch->retell = NULL;
    }
    UNLINK( ch, first_char, last_char, next, prev );
    if ( ch->desc ) {
        if ( ch->desc->character != ch )
            bug( "%s Extract_char: char's descriptor points to another char", __FUNCTION__ );
        else {
            ch->desc->character = NULL;
            close_socket( ch->desc, FALSE );
            ch->desc = NULL;
        }
    }
    return;
}

/* Find a char in the room. */
CHAR_DATA              *get_char_room( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *rch;
    int                     number,
                            count,
                            vnum;

    number = number_argument( argument, arg );
    if ( !str_cmp( arg, "self" ) )
        return ch;
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );
    else
        vnum = -1;
    count = 0;
    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
        if ( can_see( ch, rch )
             && ( nifty_is_name( arg, rch->name )
                  || ( IS_NPC( rch ) && vnum == rch->pIndexData->vnum ) ) ) {
            if ( number == 0 && !IS_NPC( rch ) )
                return rch;
            else if ( ++count == number )
                return rch;
        }
    if ( vnum != -1 )
        return NULL;

    /*
     * If we didn't find an exact match, run through the list of characters
     * * again looking for prefix matching, ie gu == guard.
     * * Added by Narn, Sept/96
     */
    count = 0;
    for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
        if ( !can_see( ch, rch ) || !nifty_is_name_prefix( arg, rch->name ) )
            continue;
        if ( number == 0 && !IS_NPC( rch ) )
            return rch;
        else if ( ++count == number )
            return rch;
    }
    return NULL;
}

CHAR_DATA              *get_mob( int vnum )
{
    CHAR_DATA              *wch;

    if ( get_mob_index( vnum ) == NULL )
        return NULL;

    /*
     * check the world for an exact match 
     */
    for ( wch = first_char; wch; wch = wch->next )
        if ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum )
            return wch;

    return NULL;
}

/* Only actually use ones in the same area */
CHAR_DATA              *get_char_area( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *wch;
    int                     number,
                            count,
                            vnum;

    number = number_argument( argument, arg );
    count = 0;
    if ( !str_cmp( arg, "self" ) )
        return ch;

    if ( !ch || ch == NULL ) {
        for ( wch = first_char; wch; wch = wch->next )
            if ( wch->in_room->area == ch->in_room->area && nifty_is_name( arg, wch->name ) )
                return wch;
    }

    /*
     * Allow reference by vnum for saints+ -Thoric 
     */
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );
    else
        vnum = -1;

    /*
     * check the room for an exact match 
     */
    for ( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
        if ( wch->in_room->area == ch->in_room->area
             && ( ( nifty_is_name( arg, wch->name ) )
                  || ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum ) ) ) {
            if ( number == 0 && !IS_NPC( wch ) )
                return wch;
            else if ( ++count == number )
                return wch;
        }
    count = 0;
    /*
     * check the world for an exact match 
     */
    for ( wch = first_char; wch; wch = wch->next )
        if ( wch->in_room->area == ch->in_room->area
             && ( nifty_is_name( arg, wch->name )
                  || ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum ) ) ) {
            if ( number == 0 && !IS_NPC( wch ) )
                return wch;
            else if ( ++count == number )
                return wch;
        }
    /*
     * bail out if looking for a vnum match 
     */
    if ( vnum != -1 )
        return NULL;
    /*
     * If we didn't find an exact match, check the room for
     * * for a prefix match, ie gu == guard.
     * * Added by Narn, Sept/96
     */
    count = 0;
    for ( wch = ch->in_room->first_person; wch; wch = wch->next_in_room ) {
        if ( !can_see( ch, wch ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
        if ( number == 0 && !IS_NPC( wch ) )
            return wch;
        else if ( ++count == number )
            return wch;
    }

    /*
     * If we didn't find a prefix match in the room, run through the full list
     * of characters looking for prefix matching, ie gu == guard.
     * Added by Narn, Sept/96
     */
    count = 0;
    for ( wch = first_char; wch; wch = wch->next ) {
        if ( wch->in_room->area != ch->in_room->area )
            continue;
        if ( !can_see( ch, wch ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
        if ( number == 0 && !IS_NPC( wch ) )
            return wch;
        else if ( ++count == number )
            return wch;
    }

    return NULL;
}

/* Find a char in the world. */
CHAR_DATA              *get_char_world( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *wch;
    int                     number,
                            count,
                            vnum;

    number = number_argument( argument, arg );
    count = 0;
    if ( !str_cmp( arg, "self" ) )
        return ch;

    if ( !ch || ch == NULL ) {
        for ( wch = first_char; wch; wch = wch->next )
            if ( nifty_is_name( arg, wch->name ) )
                return wch;
    }

    /*
     * Allow reference by vnum for saints+ -Thoric 
     */
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );
    else
        vnum = -1;

    /*
     * check the room for an exact match 
     */
    for ( wch = ch->in_room->first_person; wch; wch = wch->next_in_room )
        if ( can_see( ch, wch )
             && ( ( nifty_is_name( arg, wch->name ) )
                  || ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum ) ) ) {
            if ( number == 0 && !IS_NPC( wch ) )
                return wch;
            else if ( ++count == number )
                return wch;
        }
    count = 0;
    /*
     * check the world for an exact match 
     */
    for ( wch = first_char; wch; wch = wch->next )
        if ( can_see( ch, wch )
             && ( nifty_is_name( arg, wch->name )
                  || ( IS_NPC( wch ) && vnum == wch->pIndexData->vnum ) ) ) {
            if ( number == 0 && !IS_NPC( wch ) )
                return wch;
            else if ( ++count == number )
                return wch;
        }
    /*
     * bail out if looking for a vnum match 
     */
    if ( vnum != -1 )
        return NULL;
    /*
     * If we didn't find an exact match, check the room for
     * * for a prefix match, ie gu == guard.
     * * Added by Narn, Sept/96
     */
    count = 0;
    for ( wch = ch->in_room->first_person; wch; wch = wch->next_in_room ) {
        if ( !can_see( ch, wch ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
        if ( number == 0 && !IS_NPC( wch ) )
            return wch;
        else if ( ++count == number )
            return wch;
    }

    /*
     * If we didn't find a prefix match in the room, run through the full list
     * of characters looking for prefix matching, ie gu == guard.
     * Added by Narn, Sept/96
     */
    count = 0;
    for ( wch = first_char; wch; wch = wch->next ) {
        if ( !can_see( ch, wch ) || !nifty_is_name_prefix( arg, wch->name ) )
            continue;
        if ( number == 0 && !IS_NPC( wch ) )
            return wch;
        else if ( ++count == number )
            return wch;
    }

    return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA               *get_obj_list( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    int                     number;
    int                     count;

    number = number_argument( argument, arg );
    count = 0;
    for ( obj = list; obj; obj = obj->next_content )
        if ( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
            if ( ( count += obj->count ) >= number )
                return obj;

    /*
     * If we didn't find an exact match, run through the list of objects
     * again looking for prefix matching, ie swo == sword.
     * Added by Narn, Sept/96
     */
    count = 0;
    for ( obj = list; obj; obj = obj->next_content )
        if ( can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
            if ( ( count += obj->count ) >= number )
                return obj;

    return NULL;
}

/*
 * Find an obj in a list...going the other way   -Thoric
 */
OBJ_DATA               *get_obj_list_rev( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    int                     number;
    int                     count;

    number = number_argument( argument, arg );
    count = 0;
    for ( obj = list; obj; obj = obj->prev_content )
        if ( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
            if ( ( count += obj->count ) >= number )
                return obj;

    /*
     * If we didn't find an exact match, run through the list of objects
     * again looking for prefix matching, ie swo == sword.
     * Added by Narn, Sept/96
     */
    count = 0;
    for ( obj = list; obj; obj = obj->prev_content )
        if ( can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
            if ( ( count += obj->count ) >= number )
                return obj;

    return NULL;
}

/*
 * Find an obj in player's inventory or wearing via a vnum -Shaddai
 */

OBJ_DATA               *get_obj_vnum( CHAR_DATA *ch, int vnum )
{
    OBJ_DATA               *obj;

    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        if ( can_see_obj( ch, obj ) && obj->pIndexData->vnum == vnum )
            return obj;
    return NULL;
}

/*
 * Find an obj in player's inventory.
 */
OBJ_DATA               *get_obj_carry( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    int                     number,
                            count,
                            vnum;

    number = number_argument( argument, arg );
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );
    else
        vnum = -1;

    count = 0;
    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
             && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
            if ( ( count += obj->count ) >= number )
                return obj;

    if ( vnum != -1 )
        return NULL;

    /*
     * If we didn't find an exact match, run through the list of objects
     * again looking for prefix matching, ie swo == sword.
     * Added by Narn, Sept/96
     */
    count = 0;
    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
             && nifty_is_name_prefix( arg, obj->name ) )
            if ( ( count += obj->count ) >= number )
                return obj;

    return NULL;
}

/*
 * Find an obj in player's equipment.
 */
OBJ_DATA               *get_obj_wear( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    int                     number,
                            count,
                            vnum;

    number = number_argument( argument, arg );

    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );
    else
        vnum = -1;

    count = 0;
    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        if ( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj )
             && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
            if ( ++count == number )
                return obj;

    if ( vnum != -1 )
        return NULL;

    /*
     * If we didn't find an exact match, run through the list of objects
     * again looking for prefix matching, ie swo == sword.
     * Added by Narn, Sept/96
     */
    count = 0;
    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
        if ( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj )
             && nifty_is_name_prefix( arg, obj->name ) )
            if ( ++count == number )
                return obj;

    return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
/*
OBJ_DATA *get_obj_here ( CHAR_DATA *ch, char *argument )
{
   OBJ_DATA *obj;

   obj = get_obj_list_rev ( ch, argument, ch->in_room->last_content );
   if ( obj )
      return obj;

   if ( ( obj = get_obj_carry ( ch, argument ) ) != NULL )
      return obj;

   if ( ( obj = get_obj_wear ( ch, argument ) ) != NULL )
      return obj;

   return NULL;
}
*/

/*
 * This had to be redone because if you had 2 of something in room and 3 in inventory
 * only the 2 in room and the 3rd in inventory was useable
 */
typedef struct obj_here OBJ_HERE;
struct obj_here
{
    OBJ_HERE               *next,
                           *prev;
    OBJ_DATA               *obj;                       /* Just a pointer to the object
                                                        * that is here */
};
OBJ_HERE               *first_ohere;
OBJ_HERE               *last_ohere;

void create_obj_here_list( OBJ_DATA *list )
{
    OBJ_DATA               *obj;
    OBJ_HERE               *ohere;

    for ( obj = list; obj; obj = obj->prev_content ) {
        CREATE( ohere, OBJ_HERE, 1 );
        ohere->obj = obj;
        LINK( ohere, first_ohere, last_ohere, next, prev );
    }
}

void free_obj_here_list( void )
{
    OBJ_HERE               *ohere,
                           *ohere_next;

    for ( ohere = first_ohere; ohere; ohere = ohere_next ) {
        ohere_next = ohere->next;
        ohere->obj = NULL;
        UNLINK( ohere, first_ohere, last_ohere, next, prev );
    }
}

OBJ_DATA               *get_obj_here( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    OBJ_HERE               *ohere;
    char                    arg[MIL];
    int                     number,
                            count = 0,
        vnum = -1;

    number = number_argument( argument, arg );
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );

    create_obj_here_list( ch->in_room->last_content );
    create_obj_here_list( ch->last_carrying );

    count = 0;
    /*
     * Check for an exact match 
     */
    for ( ohere = first_ohere; ohere; ohere = ohere->next ) {
        if ( !( obj = ohere->obj ) )
            continue;
        if ( !can_see_obj( ch, obj ) )
            continue;
        if ( !nifty_is_name( arg, obj->name ) && obj->pIndexData->vnum != vnum )
            continue;
        if ( ( count += obj->count ) < number )
            continue;
        free_obj_here_list(  );
        return obj;
    }

    /*
     * If we were looking for a vnum and didn't find it return 
     */
    if ( vnum != -1 ) {
        free_obj_here_list(  );
        return NULL;
    }

    count = 0;
    /*
     * Check for a prefix match 
     */
    for ( ohere = first_ohere; ohere; ohere = ohere->next ) {
        if ( !( obj = ohere->obj ) )
            continue;
        if ( !can_see_obj( ch, obj ) )
            continue;
        if ( !nifty_is_name_prefix( arg, obj->name ) )
            continue;
        if ( ( count += obj->count ) < number )
            continue;
        free_obj_here_list(  );
        return obj;
    }

    free_obj_here_list(  );
    return NULL;
}

/* Find an obj in the world. */
OBJ_DATA               *get_obj_world( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    int                     number,
                            count,
                            vnum;

    if ( ( obj = get_obj_here( ch, argument ) ) != NULL )
        return obj;
    number = number_argument( argument, arg );
    /*
     * Allow reference by vnum for saints+ -Thoric 
     */
    if ( get_trust( ch ) >= LEVEL_IMMORTAL && is_number( arg ) )
        vnum = atoi( arg );
    else
        vnum = -1;
    count = 0;
    for ( obj = first_object; obj; obj = obj->next )
        if ( can_see_obj( ch, obj )
             && ( nifty_is_name( arg, obj->name ) || vnum == obj->pIndexData->vnum ) )
            if ( ( count += obj->count ) >= number )
                return obj;
    /*
     * bail out if looking for a vnum 
     */
    if ( vnum != -1 )
        return NULL;
    /*
     * If we didn't find an exact match, run through the list of objects
     * * again looking for prefix matching, ie swo == sword.
     * * Added by Narn, Sept/96
     */
    count = 0;
    for ( obj = first_object; obj; obj = obj->next )
        if ( can_see_obj( ch, obj ) && nifty_is_name_prefix( arg, obj->name ) )
            if ( ( count += obj->count ) >= number )
                return obj;
    return NULL;
}

/* How mental state could affect finding an object -Thoric
 * Used by get/drop/put/quaff/recite/etc
 * Increasingly freaky based on mental state and drunkeness
 */
bool ms_find_obj( CHAR_DATA *ch )
{
    int                     ms = ch->mental_state;
    int                     drunk = IS_NPC( ch ) ? 0 : ch->pcdata->condition[COND_DRUNK];
    const char             *t;

    /*
     * we're going to be nice and let nothing weird happen unless
     * you're a tad messed up
     */
    drunk = UMAX( 1, drunk );
    if ( abs( ms ) + ( drunk / 3 ) < 30 )
        return FALSE;
    if ( ( number_percent(  ) + ( ms < 0 ? 15 : 5 ) ) > abs( ms ) / 2 + drunk / 4 )
        return FALSE;
    if ( ms > 15 )                                     /* range 1 to 20 -- feel free to
                                                        * add more */
        switch ( number_range( UMAX( 1, ( ms / 5 - 15 ) ), ( ms + 4 ) / 5 ) ) {
            default:
            case 1:
                t = "As you reach for it, you forgot what it was...\r\n";
                break;
            case 2:
                t = "As you reach for it, something inside stops you...\r\n";
                break;
            case 3:
                t = "As you reach for it, it seems to move out of the way...\r\n";
                break;
            case 4:
                t = "You grab frantically for it, but can't seem to get a hold of it...\r\n";
                break;
            case 5:
                t = "It disappears as soon as you touch it!\r\n";
                break;
            case 6:
                t = "You would if it would stay still!\r\n";
                break;
            case 7:
                t = "Whoa!  It's covered in blood!  Ack!  Ick!\r\n";
                break;
            case 8:
                t = "Wow... trails!\r\n";
                break;
            case 9:
                t = "You reach for it, then notice the back of your hand is growing something!\r\n";
                break;
            case 10:
                t = "As you grasp it, it shatters into tiny shards which bite into your flesh!\r\n";
                break;
            case 11:
                t = "What about that huge dragon flying over your head?!?!?\r\n";
                break;
            case 12:
                t = "You stratch yourself instead...\r\n";
                break;
            case 13:
                t = "You hold the universe in the palm of your hand!\r\n";
                break;
            case 14:
                t = "You're too scared.\r\n";
                break;
            case 15:
                t = "Your mother smacks your hand... 'NO!'\r\n";
                break;
            case 16:
                t = "Your hand grasps the worst pile of revoltingness that you could ever imagine!\r\n";
                break;
            case 17:
                t = "You stop reaching for it as it screams out at you in pain!\r\n";
                break;
            case 18:
                t = "What about the millions of burrow-maggots feasting on your arm?!?!\r\n";
                break;
            case 19:
                t = "That doesn't matter anymore... you've found the true answer to everything!\r\n";
                break;
            case 20:
                t = "A THY QUEST General has no need for that.\r\n";
                break;
        }
    else {
        int                     sub = URANGE( 1, abs( ms ) / 2 + drunk, 60 );

        switch ( number_range( 1, sub / 10 ) ) {
            default:
            case 1:
                t = "In just a second...\r\n";
                break;
            case 2:
                t = "You can't find that...\r\n";
                break;
            case 3:
                t = "It's just beyond your grasp...\r\n";
                break;
            case 4:
                t = "...but it's under a pile of other stuff...\r\n";
                break;
            case 5:
                t = "You go to reach for it, but pick your nose instead.\r\n";
                break;
            case 6:
                t = "Which one?!?  I see two... no three...\r\n";
                break;
        }
    }
    send_to_char( t, ch );
    return TRUE;
}

/*
 * Generic get obj function that supports optional containers.  -Thoric
 * currently only used for "eat" and "quaff".
 */
OBJ_DATA               *find_obj( CHAR_DATA *ch, char *argument, bool carryonly )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *obj = NULL;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( !str_cmp( arg2, "from" ) && argument[0] != '\0' )
        argument = one_argument( argument, arg2 );

    if ( arg2[0] == '\0' ) {
        if ( carryonly && ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
            send_to_char( "You do not have that item.\r\n", ch );
            return NULL;
        }
        else if ( !carryonly && ( obj = get_obj_here( ch, arg1 ) ) == NULL ) {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg1, TO_CHAR );
            return NULL;
        }
        return obj;
    }
    else {
        OBJ_DATA               *container = NULL;

        if ( carryonly && ( container = get_obj_carry( ch, arg2 ) ) == NULL
             && ( container = get_obj_wear( ch, arg2 ) ) == NULL ) {
            send_to_char( "You do not have that item.\r\n", ch );
            return NULL;
        }
        if ( !carryonly && ( container = get_obj_here( ch, arg2 ) ) == NULL ) {
            act( AT_PLAIN, "I see no $T here.", ch, NULL, arg2, TO_CHAR );
            return NULL;
        }

        if ( !IS_OBJ_STAT( container, ITEM_COVERING )
             && IS_SET( container->value[1], CONT_CLOSED ) ) {
            act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
            return NULL;
        }

        obj = get_obj_list( ch, arg1, container->first_content );
        if ( !obj )
            act( AT_PLAIN,
                 IS_OBJ_STAT( container,
                              ITEM_COVERING ) ? "I see nothing like that beneath $p." :
                 "I see nothing like that in $p.", ch, container, NULL, TO_CHAR );

        return obj;
    }
//  return NULL;
}

int get_obj_number( OBJ_DATA *obj )
{
    if ( !obj ) {
        bug( "%s", "get_obj_number: NULL obj" );
        return FALSE;
    }
    return obj->count;
}

/* Return TRUE if an object is, or nested inside a magic container */
bool in_magic_container( OBJ_DATA *obj )
{
    if ( !obj ) {
        bug( "%s", "in_magic_container: NULL obj" );
        return FALSE;
    }
    if ( obj->item_type == ITEM_CONTAINER && IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        return TRUE;
    if ( obj->in_obj )
        return in_magic_container( obj->in_obj );
    return FALSE;
}

/*
 * Return TRUE if an object is a magic container. -Orion
 */
bool is_magic_container( OBJ_DATA *obj )
{
    if ( !obj ) {
        bug( "%s", "is_magic_container: NULL obj" );
        return FALSE;
    }
    if ( obj->item_type == ITEM_CONTAINER && IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        return TRUE;
    return FALSE;
}

/*
 * Return real weight of an object, including weight of contents.
 */
int get_obj_weight( OBJ_DATA *obj, bool real )
{
    int                     weight;

    if ( !obj ) {
        bug( "%s", "Obj could cause a crash weight set to 1 get_obj_weight: NULL obj" );
        return 1;
    }

    weight = obj->count * obj->weight;

    if ( real || obj->item_type != ITEM_CONTAINER || !IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        for ( obj = obj->first_content; obj; obj = obj->next_content )
            weight += get_obj_weight( obj, real );

    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if ( !pRoomIndex ) {
        bug( "%s", "room_is_dark: NULL pRoomIndex" );
        return TRUE;
    }

    if ( pRoomIndex->light > 0 )
        return FALSE;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_LIGHTED )
         || IS_SET( pRoomIndex->area->flags, AFLAG_LIGHTED ) )
        return FALSE;

    /*
     * Volk - we can test for aflag darkness now because we HAVE NO LIGHT! 
     */
    if ( IS_SET( pRoomIndex->room_flags, ROOM_DARK )
         || IS_SET( pRoomIndex->area->flags, AFLAG_DARKNESS ) )
        return TRUE;

    if ( pRoomIndex->sector_type == SECT_INSIDE || pRoomIndex->sector_type == SECT_ROAD )
        return FALSE;

    if ( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
        return TRUE;

    return FALSE;
}

/*
 * If room is "do not disturb" return the pointer to the imm with dnd flag
 * NULL if room is not "do not disturb".
 */
CHAR_DATA              *room_is_dnd( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA              *rch;

    if ( !pRoomIndex ) {
        bug( "%s", "room_is_dnd: NULL pRoomIndex" );
        return NULL;
    }

    if ( !IS_SET( pRoomIndex->room_flags, ROOM_DND ) )
        return NULL;

    for ( rch = pRoomIndex->first_person; rch; rch = rch->next_in_room ) {
        if ( !IS_NPC( rch ) && rch->pcdata && IS_IMMORTAL( rch )
             && IS_SET( rch->pcdata->flags, PCFLAG_DND ) && get_trust( ch ) < get_trust( rch )
             && can_see( ch, rch ) )
            return rch;
    }
    return NULL;
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA              *rch;
    int                     count;

    if ( !pRoomIndex ) {
        bug( "%s", "room_is_private: NULL pRoomIndex" );
        return FALSE;
    }

    count = 0;
    for ( rch = pRoomIndex->first_person; rch; rch = rch->next_in_room )
        count++;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_PRIVATE ) && count >= 2 )
        return TRUE;

    if ( IS_SET( pRoomIndex->room_flags, ROOM_SOLITARY ) && count >= 1 )
        return TRUE;

    return FALSE;
}

bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )
{
    /*
     * If ch is victim should always be true 
     */
    if ( ch == victim )
        return TRUE;

    if ( !victim )
        return FALSE;

    if ( victim == supermob && ( ch->level < 108 || IS_NPC( ch ) ) )
        return FALSE;

    if ( ch == supermob )
        return TRUE;

    if ( !ch ) {
        if ( IS_AFFECTED( victim, AFF_INVISIBLE ) || IS_AFFECTED( victim, AFF_HIDE )
             || IS_AFFECTED( victim, AFF_BURROW ) || xIS_SET( victim->act, PLR_WIZINVIS ) )
            return FALSE;
        else
            return TRUE;
    }

    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_WIZINVIS )
         && get_trust( ch ) < victim->pcdata->wizinvis )
        return FALSE;

    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_MOBINVIS ) && ch->level < 101 )
        return FALSE;

    if ( IS_NPC( ch ) && IS_CLANNED( victim ) && !victim->fighting
         && str_cmp( victim->pcdata->clan_name, "halcyon" ) && !IS_AFFECTED( victim, AFF_INVISIBLE )
         && !IS_AFFECTED( victim, AFF_HIDE ) && !IS_AFFECTED( victim, AFF_SNEAK ) ) {
        AREA_DATA              *tarea = victim->in_room->area;
        CLAN_DATA              *clan = victim->pcdata->clan;
        short                   chance = number_range( 1, 70 );

        if ( ch->in_room == victim->in_room && tarea->influencer && clan
             && clan != tarea->influencer && !IS_SET( tarea->flags, AFLAG_NOINFLUENCE ) ) {
            if ( chance == 70 ) {
                if ( ch->influence == 20
                     && strcasecmp( lookup_spec( ch->spec_fun ), "spec_cast_adept" )
                     && ch->race < 18 ) {
                    char                    buf[MSL];

                    chance = number_chance( 1, 15 );

                    ch->influence = 19;

                    if ( chance == 1 )
                        interpret( ch, ( char * ) "say Your kind is not welcomed here." );
                    else if ( chance == 3 )
                        interpret( ch, ( char * ) "growl" );
                    else if ( chance == 5 )
                        interpret( ch, ( char * ) "emote looks nervous, and moves away from you." );
                    else if ( chance == 6 )
                        interpret( ch,
                                   ( char * )
                                   "emote looks at you and whispers something to another." );
                    else if ( chance == 8 )
                        interpret( ch, ( char * ) "say A sorry lot your clan is." );
                    else if ( chance == 9 )
                        interpret( ch, ( char * ) "emote moves back into the shadows." );
                    else if ( chance == 15 && !xIS_SET( ch->act, ACT_PRACTICE )
                              && !xIS_SET( ch->act, ACT_NEWBPRACTICE )
                              && str_cmp( lookup_spec( ch->spec_fun ), "spec_cast_adept" )
                              && !xIS_SET( ch->act, ACT_PACIFIST ) ) {
                        interpret( ch,
                                   ( char * )
                                   "say Your clan has been terrorizing us for too long!." );
                        if ( victim && victim->level - ch->level <= 5
                             || ch->level - victim->level <= 5 && ch->level >= 20 ) {
                            do_mpkill( ch, victim->name );
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    if ( !IS_NPC( ch ) && !IS_NPC( victim ) ) {
        if ( ( IS_SET( ch->pcdata->tag_flags, TAG_PLAYING )
               || IS_SET( ch->pcdata->tag_flags, TAG_WAITING ) )
             && ( IS_SET( victim->pcdata->tag_flags, TAG_PLAYING )
                  || IS_SET( victim->pcdata->tag_flags, TAG_WAITING ) ) )
            return TRUE;
    }

    if ( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && !IS_NPC( victim ) && IS_PKILL( victim )
         && victim->timer > 1 && !victim->desc )
        return FALSE;

    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_HOLYLIGHT ) )
        return TRUE;

    if ( IS_AFFECTED( victim, AFF_BURROW ) ) {
        if ( !IS_NPC( victim ) && IS_NPC( ch ) && ch->level < victim->level )
            return FALSE;
        if ( number_range( 1, 100 ) < LEARNED( ch, gsn_earthspeak ) )
            return TRUE;
        return FALSE;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_TRUESIGHT ) )
        return TRUE;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) )
        return FALSE;

    if ( IS_AFFECTED( ch, AFF_TRUESIGHT ) || IS_AFFECTED( ch, AFF_DEMONIC_SIGHT ) ) {
        if ( victim->level > ch->level && IS_AFFECTED( victim, AFF_INVISIBLE )
             && !IS_NPC( victim ) )
            return FALSE;

        return TRUE;
    }

    if ( room_is_dark( ch->in_room ) && !IS_AFFECTED( ch, AFF_INFRARED ) )
        return FALSE;

    if ( IS_AFFECTED( victim, AFF_HIDE ) && !IS_AFFECTED( ch, AFF_DETECT_HIDDEN )
         && !victim->fighting )
        return FALSE;

    if ( IS_AFFECTED( victim, AFF_INVISIBLE )
         && ( !IS_AFFECTED( ch, AFF_DETECT_INVIS ) || victim->level > ch->level ) )
        return FALSE;

    return TRUE;
}

/* True if char can see obj. */
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_HOLYLIGHT ) )
        return TRUE;

    if ( IS_NPC( ch ) && ch->pIndexData->vnum == 3 )
        return TRUE;

/* Volk - Quick amendment here - if item is buried and has earthspeak, can sometimes see it */
    if ( IS_OBJ_STAT( obj, ITEM_BURIED ) ) {
        if ( number_range( 1, 100 ) > LEARNED( ch, gsn_earthspeak ) )
            return FALSE;
        else
            return TRUE;
    }

/* Truesight mobs can see EVERYTHING (except burrow hahah) - Volk */
    if ( IS_AFFECTED( ch, AFF_TRUESIGHT ) && IS_NPC( ch ) )
        return TRUE;

/* Players cannot see invis items above their level */
    if ( IS_OBJ_STAT( obj, ITEM_INVIS ) ) {
        if ( obj->level > ch->level )
            return FALSE;
        if ( !IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
            return FALSE;
    }

/* While not blind, truesight/demonic on players sees all (except invis)*/
    if ( ( IS_AFFECTED( ch, AFF_TRUESIGHT ) || IS_AFFECTED( ch, AFF_DEMONIC_SIGHT ) )
         && ( !IS_AFFECTED( ch, AFF_BLINDNESS ) || IS_AFFECTED( ch, AFF_NOSIGHT ) ) )
        return TRUE;

    if ( IS_OBJ_STAT( obj, ITEM_HIDDEN ) && !IS_AFFECTED( ch, AFF_DETECT_HIDDEN ) )
        return FALSE;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) )
        return FALSE;

    /*
     * can see lights in the dark 
     */
    if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
        return TRUE;

    if ( room_is_dark( ch->in_room ) ) {
        /*
         * can see glowing items in the dark... invisible or not 
         */
        if ( IS_OBJ_STAT( obj, ITEM_GLOW ) )
            return TRUE;
        if ( !IS_AFFECTED( ch, AFF_INFRARED ) )
            return FALSE;
    }

    return TRUE;
}

/* True if char can drop obj. */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_OBJ_STAT( obj, ITEM_NODROP ) )
        return TRUE;

    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return TRUE;

    if ( IS_NPC( ch ) && ch->pIndexData->vnum == 3 )
        return TRUE;

    return FALSE;
}

/* Return ascii name of an item type. */
const char             *item_type_name( OBJ_DATA *obj )
{
    if ( obj->item_type < 1 || obj->item_type > MAX_ITEM_TYPE ) {
        bug( "Item_type_name: unknown type %d.", obj->item_type );
        return "(unknown)";
    }
    return o_types[obj->item_type];
}

/* Return ascii name of pulltype exit setting. */
const char             *pull_type_name( int pulltype )
{
    if ( pulltype >= PT_FIRE )
        return ex_pfire[pulltype - PT_FIRE];
    if ( pulltype >= PT_AIR )
        return ex_pair[pulltype - PT_AIR];
    if ( pulltype >= PT_EARTH )
        return ex_pearth[pulltype - PT_EARTH];
    if ( pulltype >= PT_WATER )
        return ex_pwater[pulltype - PT_WATER];
    if ( pulltype < 0 )
        return "ERROR";
    return ex_pmisc[pulltype];
}

/*
 * Set off a trap (obj) upon character (ch)   -Thoric
 */
ch_ret spring_trap( CHAR_DATA *ch, OBJ_DATA *obj )
{
    int                     dam;
    int                     typ;
    int                     lev;
    const char             *txt;
    ch_ret                  retcode;

    typ = obj->value[1];
    lev = obj->value[2];

    retcode = rNONE;

    switch ( typ ) {
        default:
            txt = "hit by a trap";
            break;
        case TRAP_TYPE_POISON_GAS:
            txt = "surrounded by a green cloud of gas";
            break;
        case TRAP_TYPE_POISON_DART:
            txt = "hit by a dart";
            break;
        case TRAP_TYPE_POISON_NEEDLE:
            txt = "pricked by a needle";
            break;
        case TRAP_TYPE_POISON_DAGGER:
            txt = "stabbed by a dagger";
            break;
        case TRAP_TYPE_POISON_ARROW:
            txt = "struck with an arrow";
            break;
        case TRAP_TYPE_BLINDNESS_GAS:
            txt = "surrounded by a red cloud of gas";
            break;
        case TRAP_TYPE_SLEEPING_GAS:
            txt = "surrounded by a yellow cloud of gas";
            break;
        case TRAP_TYPE_FLAME:
            txt = "struck by a burst of flame";
            break;
        case TRAP_TYPE_EXPLOSION:
            txt = "hit by an explosion";
            break;
        case TRAP_TYPE_ACID_SPRAY:
            txt = "covered by a spray of acid";
            break;
        case TRAP_TYPE_ELECTRIC_SHOCK:
            txt = "suddenly shocked";
            break;
        case TRAP_TYPE_BLADE:
            txt = "sliced by a razor sharp blade";
            break;
        case TRAP_TYPE_SEX_CHANGE:
            txt = "surrounded by a mysterious aura";
            break;
    }

    dam = number_range( obj->value[2], obj->value[2] * 2 );
    act_printf( AT_HITME, ch, NULL, NULL, TO_CHAR, "You are %s!", txt );
    act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n is %s.", txt );
    --obj->value[0];
    if ( obj->value[0] <= 0 )
        extract_obj( obj );
    switch ( typ ) {
        default:
        case TRAP_TYPE_POISON_DART:
        case TRAP_TYPE_POISON_NEEDLE:
        case TRAP_TYPE_POISON_DAGGER:
        case TRAP_TYPE_POISON_ARROW:
            /*
             * hmm... why not use spell_poison() here? 
             */
            retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
            if ( retcode == rNONE )
                retcode = damage( ch, ch, dam, TYPE_UNDEFINED );
            break;
        case TRAP_TYPE_POISON_GAS:
            retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
            break;
        case TRAP_TYPE_BLINDNESS_GAS:
            retcode = obj_cast_spell( gsn_blindness, lev, ch, ch, NULL );
            break;
        case TRAP_TYPE_SLEEPING_GAS:
            retcode = obj_cast_spell( skill_lookup( "sleep" ), lev, ch, ch, NULL );
            break;
        case TRAP_TYPE_ACID_SPRAY:
            retcode = obj_cast_spell( skill_lookup( "acid blast" ), lev, ch, ch, NULL );
            break;
        case TRAP_TYPE_SEX_CHANGE:
            retcode = obj_cast_spell( skill_lookup( "change sex" ), lev, ch, ch, NULL );
            break;
        case TRAP_TYPE_FLAME:
        case TRAP_TYPE_EXPLOSION:
            retcode = obj_cast_spell( gsn_fireball, lev, ch, ch, NULL );
            break;
        case TRAP_TYPE_ELECTRIC_SHOCK:
        case TRAP_TYPE_BLADE:
            retcode = damage( ch, ch, dam, TYPE_UNDEFINED );
    }
    return retcode;
}

/*
 * Check an object for a trap     -Thoric
 */
ch_ret check_for_trap( CHAR_DATA *ch, OBJ_DATA *obj, int flag )
{
    OBJ_DATA               *check;
    ch_ret                  retcode;

    if ( !obj->first_content )
        return rNONE;

    retcode = rNONE;

    for ( check = obj->first_content; check; check = check->next_content )
        if ( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) ) {
            retcode = spring_trap( ch, check );
            if ( retcode != rNONE )
                return retcode;
        }
    return retcode;
}

/*
 * Check the room for a trap     -Thoric
 */
ch_ret check_room_for_traps( CHAR_DATA *ch, int flag )
{
    OBJ_DATA               *check;
    ch_ret                  retcode;

    retcode = rNONE;

    if ( !ch )
        return rERROR;
    if ( !ch->in_room || !ch->in_room->first_content )
        return rNONE;

    for ( check = ch->in_room->first_content; check; check = check->next_content ) {
        if ( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) ) {
            retcode = spring_trap( ch, check );
            if ( retcode != rNONE )
                return retcode;
        }
    }
    return retcode;
}

/*
 * return TRUE if an object contains a trap   -Thoric
 */
bool is_trapped( OBJ_DATA *obj )
{
    OBJ_DATA               *check;

    if ( !obj->first_content )
        return FALSE;

    for ( check = obj->first_content; check; check = check->next_content )
        if ( check->item_type == ITEM_TRAP )
            return TRUE;

    return FALSE;
}

/*
 * If an object contains a trap, return the pointer to the trap  -Thoric
 */
OBJ_DATA               *get_trap( OBJ_DATA *obj )
{
    OBJ_DATA               *check;

    if ( !obj->first_content )
        return NULL;

    for ( check = obj->first_content; check; check = check->next_content )
        if ( check->item_type == ITEM_TRAP )
            return check;

    return NULL;
}

/*
 * Return a pointer to the first object of a certain type found that
 * a player is carrying/wearing
 */
OBJ_DATA               *get_objtype( CHAR_DATA *ch, short type )
{
    OBJ_DATA               *obj;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
        if ( obj->item_type == type )
            return obj;

    return NULL;
}

/* Remove an exit from a room -Thoric */
void extract_exit( ROOM_INDEX_DATA *room, EXIT_DATA *pexit )
{
    UNLINK( pexit, room->first_exit, room->last_exit, next, prev );
    if ( pexit->rexit )
        pexit->rexit->rexit = NULL;
    if ( VLD_STR( pexit->keyword ) )
        STRFREE( pexit->keyword );
    if ( VLD_STR( pexit->description ) )
        STRFREE( pexit->description );
    DISPOSE( pexit );
}

/* clean out a room (leave list pointers intact) -Thoric */
void clean_room( ROOM_INDEX_DATA *room )
{
    EXTRA_DESCR_DATA       *ed,
                           *ed_next;
    EXIT_DATA              *pexit,
                           *pexit_next;
    MPROG_DATA             *mprog,
                           *mprog_next;

    if ( VLD_STR( room->description ) )
        STRFREE( room->description );
    if ( VLD_STR( room->name ) )
        STRFREE( room->name );
    for ( mprog = room->mudprogs; mprog; mprog = mprog_next ) {
        mprog_next = mprog->next;
        if ( VLD_STR( mprog->arglist ) )
            STRFREE( mprog->arglist );
        if ( VLD_STR( mprog->comlist ) )
            STRFREE( mprog->comlist );
        DISPOSE( mprog );
    }
    for ( ed = room->first_extradesc; ed; ed = ed_next ) {
        ed_next = ed->next;
        if ( VLD_STR( ed->description ) )
            STRFREE( ed->description );
        if ( VLD_STR( ed->keyword ) )
            STRFREE( ed->keyword );
        DISPOSE( ed );
        top_ed--;
    }
    room->first_extradesc = NULL;
    room->last_extradesc = NULL;
    for ( pexit = room->first_exit; pexit; pexit = pexit_next ) {
        pexit_next = pexit->next;
        if ( VLD_STR( pexit->keyword ) )
            STRFREE( pexit->keyword );
        if ( VLD_STR( pexit->description ) )
            STRFREE( pexit->description );
        DISPOSE( pexit );
        top_exit--;
    }
    room->first_exit = NULL;
    room->last_exit = NULL;
    room->room_flags = 0;
    room->sector_type = 0;
    room->light = 0;
}

/* clean out an object (index) (leave list pointers intact)  -Thoric */
void clean_obj( OBJ_INDEX_DATA *obj )
{
    AFFECT_DATA            *paf,
                           *paf_next;
    EXTRA_DESCR_DATA       *ed,
                           *ed_next;
    MPROG_DATA             *mprog,
                           *mprog_next;

    if ( VLD_STR( obj->name ) )
        STRFREE( obj->name );
    if ( VLD_STR( obj->short_descr ) )
        STRFREE( obj->short_descr );
    if ( VLD_STR( obj->description ) )
        STRFREE( obj->description );
    if ( VLD_STR( obj->action_desc ) )
        STRFREE( obj->action_desc );
    xCLEAR_BITS( obj->extra_flags );
    obj->currtype = DEFAULT_CURR;
    for ( paf = obj->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;
        DISPOSE( paf );
        top_affect--;
    }
    obj->first_affect = NULL;
    obj->last_affect = NULL;
    for ( ed = obj->first_extradesc; ed; ed = ed_next ) {
        ed_next = ed->next;
        if ( VLD_STR( ed->description ) )
            STRFREE( ed->description );
        if ( VLD_STR( ed->keyword ) )
            STRFREE( ed->keyword );
        DISPOSE( ed );
        top_ed--;
    }
    obj->first_extradesc = NULL;
    obj->last_extradesc = NULL;
    for ( mprog = obj->mudprogs; mprog; mprog = mprog_next ) {
        mprog_next = mprog->next;
        if ( VLD_STR( mprog->arglist ) )
            STRFREE( mprog->arglist );
        if ( VLD_STR( mprog->comlist ) )
            STRFREE( mprog->comlist );
        DISPOSE( mprog );
    }
}

/* clean out a mobile (index) (leave list pointers intact)  -Thoric */
void clean_mob( MOB_INDEX_DATA *mob )
{
    MPROG_DATA             *mprog,
                           *mprog_next;

    if ( VLD_STR( mob->player_name ) )
        STRFREE( mob->player_name );
    if ( VLD_STR( mob->short_descr ) )
        STRFREE( mob->short_descr );
    if ( VLD_STR( mob->long_descr ) )
        STRFREE( mob->long_descr );
    if ( VLD_STR( mob->description ) )
        STRFREE( mob->description );
    if ( AREA_VERSION_WRITE > 5 ) {
        if ( VLD_STR( mob->clanname ) )
            STRFREE( mob->clanname );
        mob->influence = 0;
    }

    mob->spec_fun = NULL;
    mob->pShop = NULL;
    mob->rShop = NULL;
    xCLEAR_BITS( mob->progtypes );
    for ( mprog = mob->mudprogs; mprog; mprog = mprog_next ) {
        mprog_next = mprog->next;
        if ( VLD_STR( mprog->arglist ) )
            STRFREE( mprog->arglist );
        if ( VLD_STR( mprog->comlist ) )
            STRFREE( mprog->comlist );
        DISPOSE( mprog );
    }
    xCLEAR_BITS( mob->act );
    xCLEAR_BITS( mob->affected_by );
    /*
     * xCLEAR_BITS(mob->affected_by2); 
     */
    xCLEAR_BITS( mob->attacks );
    xCLEAR_BITS( mob->defenses );
}

extern int              top_reset;

/* Remove all resets from an area -Thoric */
void clean_resets( ROOM_INDEX_DATA *room )
{
    RESET_DATA             *pReset,
                           *pReset_next;

    for ( pReset = room->first_reset; pReset; pReset = pReset_next ) {
        pReset_next = pReset->next;
        delete_reset( pReset );
        --top_reset;
    }
    room->first_reset = NULL;
    room->last_reset = NULL;
}

/* Rewritten by Whir. Thanks to Vor/Casteele for help 2-1-98 */
/* Racial bonus calculations moved to this function and removed
   from comm.c - Samson 2-2-98 */
/* Updated to AD&D standards by Samson 9-5-98 */
/* Changed to use internal random number generator instead of
   OS dependant random() function - Samson 9-5-98 */

void name_stamp_stats( CHAR_DATA *ch )
{
    ch->statpoints = 6;
    ch->perm_str = race_table[ch->race]->base_str;
    ch->perm_int = race_table[ch->race]->base_int;
    ch->perm_wis = race_table[ch->race]->base_wis;
    ch->perm_dex = race_table[ch->race]->base_dex;
    ch->perm_con = race_table[ch->race]->base_con;
    ch->perm_cha = race_table[ch->race]->base_cha;
    ch->perm_lck = race_table[ch->race]->base_lck;
}

/* "Fix" a character's stats -Thoric */
void fix_char( CHAR_DATA *ch )
{
    AFFECT_DATA            *aff;
    OBJ_DATA               *obj;

    de_equip_char( ch );

    for ( aff = ch->first_affect; aff; aff = aff->next )
        affect_modify( ch, aff, FALSE );

    xCLEAR_BITS( ch->affected_by );
    xSET_BITS( ch->affected_by, race_table[ch->race]->affected );

    ch->mental_state = -10;
    ch->hit = UMAX( 1, ch->hit );
    ch->mana = UMAX( 1, ch->mana );
    ch->move = UMAX( 1, ch->move );
    ch->armor = 100;
    ch->mod_str = 0;
    ch->mod_dex = 0;
    ch->mod_wis = 0;
    ch->mod_int = 0;
    ch->mod_con = 0;
    ch->mod_cha = 0;
    ch->mod_lck = 0;
    ch->damroll = 0;
    ch->hitroll = 0;
    ch->alignment = URANGE( -1000, ch->alignment, 1000 );
    ch->saving_breath = 0;
    ch->saving_wand = 0;
    ch->saving_para_petri = 0;
    ch->saving_spell_staff = 0;
    ch->saving_poison_death = 0;

    ch->carry_weight = 0;
    ch->carry_number = 0;

    for ( aff = ch->first_affect; aff; aff = aff->next )
        affect_modify( ch, aff, TRUE );

    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( obj->wear_loc == WEAR_NONE )
            ch->carry_number += get_obj_number( obj );
        if ( !xIS_SET( obj->extra_flags, ITEM_MAGIC ) )
            ch->carry_weight += get_obj_weight( obj, FALSE );
    }

    re_equip_char( ch );
}

/* Show an affect verbosely to a character   -Thoric */
void showaffect( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    char                    buf[MSL];
    int                     x;

    if ( !paf ) {
        bug( "%s", "showaffect: NULL paf" );
        return;
    }
    if ( paf->location != APPLY_NONE && paf->modifier != 0 ) {
        switch ( paf->location ) {
            default:
                snprintf( buf, MSL, "Affects %s by %d.\r\n", a_types[paf->location % REVERSE_APPLY],
                          paf->modifier );
                break;

            case APPLY_AFFECT:
                snprintf( buf, MSL, "Affects %s by", a_types[paf->location % REVERSE_APPLY] );
                for ( x = 0; x < 32; x++ )
                    if ( IS_SET( paf->modifier, 1 << x ) ) {
                        mudstrlcat( buf, " ", MSL );
                        mudstrlcat( buf, a_flags[x], MSL );
                    }
                mudstrlcat( buf, "\r\n", MSL );
                break;

            case APPLY_WEAPONSPELL:
            case APPLY_WEARSPELL:
            case APPLY_REMOVESPELL:
                snprintf( buf, MSL, "Casts spell '%s'\r\n",
                          IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->
                          name : "unknown" );
                break;

            case APPLY_RESISTANT:
            case APPLY_IMMUNE:
            case APPLY_SUSCEPTIBLE:
                snprintf( buf, MSL, "Affects %s by", a_types[paf->location % REVERSE_APPLY] );
                for ( x = 0; x < 32; x++ )
                    if ( IS_SET( paf->modifier, 1 << x ) ) {
                        mudstrlcat( buf, " ", MSL );
                        mudstrlcat( buf, ris_flags[x], MSL );
                    }
                mudstrlcat( buf, "\r\n", MSL );
                break;
        }
        send_to_char( buf, ch );
    }
}

/* Set the current global object to obj -Thoric */
void set_cur_obj( OBJ_DATA *obj )
{
    cur_obj = obj->serial;
    cur_obj_extracted = FALSE;
    global_objcode = rNONE;
}

/* Check the recently extracted object queue for obj  -Thoric */
bool obj_extracted( OBJ_DATA *obj )
{
    OBJ_DATA               *cod;

    if ( obj->serial == cur_obj && cur_obj_extracted )
        return TRUE;
    for ( cod = extracted_obj_queue; cod; cod = cod->next )
        if ( obj == cod )
            return TRUE;
    return FALSE;
}

/* Stick obj onto extraction queue */
void queue_extracted_obj( OBJ_DATA *obj )
{
    ++cur_qobjs;
    obj->next = extracted_obj_queue;
    extracted_obj_queue = obj;
}

/* Clean out the extracted object queue */
void clean_obj_queue(  )
{
    OBJ_DATA               *obj;

    while ( extracted_obj_queue ) {
        obj = extracted_obj_queue;
        extracted_obj_queue = extracted_obj_queue->next;
        if ( !obj ) {
            --cur_qobjs;
            continue;
        }
        if ( VLD_STR( obj->name ) )
            STRFREE( obj->name );
        if ( VLD_STR( obj->owner ) )
            STRFREE( obj->owner );
        if ( VLD_STR( obj->description ) )
            STRFREE( obj->description );
        if ( VLD_STR( obj->short_descr ) )
            STRFREE( obj->short_descr );
        if ( VLD_STR( obj->action_desc ) )
            STRFREE( obj->action_desc );
        DISPOSE( obj );
        --cur_qobjs;
    }
}

/* Set the current global character to ch   -Thoric */
void set_cur_char( CHAR_DATA *ch )
{
    cur_char = ch;
    cur_char_died = FALSE;
    cur_room = ch->in_room;
    global_retcode = rNONE;
}

/* Check to see if ch died recently     -Thoric */
bool char_died( CHAR_DATA *ch )
{
    EXTRACT_CHAR_DATA      *ccd;

    if ( ch == cur_char && cur_char_died )
        return TRUE;
    for ( ccd = extracted_char_queue; ccd; ccd = ccd->next )
        if ( ccd->ch == ch )
            return TRUE;
    return FALSE;
}

/* Add ch to the queue of recently extracted characters  -Thoric */
void queue_extracted_char( CHAR_DATA *ch, bool extract )
{
    EXTRACT_CHAR_DATA      *ccd;

    if ( !ch ) {
        bug( "%s", "queue_extracted char: ch = NULL" );
        return;
    }
    CREATE( ccd, EXTRACT_CHAR_DATA, 1 );
    ccd->ch = ch;
    ccd->room = ch->in_room;
    ccd->extract = extract;
    if ( ch == cur_char )
        ccd->retcode = global_retcode;
    else
        ccd->retcode = rCHAR_DIED;
    ccd->next = extracted_char_queue;
    extracted_char_queue = ccd;
    cur_qchars++;
}

/* clean out the extracted character queue */
void clean_char_queue(  )
{
    EXTRACT_CHAR_DATA      *ccd;

    for ( ccd = extracted_char_queue; ccd; ccd = extracted_char_queue ) {
        extracted_char_queue = ccd->next;
        if ( ccd->extract )
            free_char( ccd->ch );
        DISPOSE( ccd );
        --cur_qchars;
    }
}

/* Add a timer to ch      -Thoric
 * Support for "call back" time delayed commands
 */
void add_timer( CHAR_DATA *ch, short type, int count, DO_FUN *fun, int value )
{
    TIMER                  *timer;

    for ( timer = ch->first_timer; timer; timer = timer->next )
        if ( timer->type == type ) {
            timer->count = count;
            timer->do_fun = fun;
            timer->value = value;
            break;
        }
    if ( !timer ) {
        CREATE( timer, TIMER, 1 );
        timer->count = count;
        timer->type = type;
        timer->do_fun = fun;
        timer->value = value;
        LINK( timer, ch->first_timer, ch->last_timer, next, prev );
    }
}

TIMER                  *get_timerptr( CHAR_DATA *ch, short type )
{
    TIMER                  *timer;

    for ( timer = ch->first_timer; timer; timer = timer->next )
        if ( timer->type == type )
            return timer;
    return NULL;
}

short get_timer( CHAR_DATA *ch, short type )
{
    TIMER                  *timer;

    if ( ( timer = get_timerptr( ch, type ) ) != NULL )
        return timer->count;
    else
        return 0;
}

void extract_timer( CHAR_DATA *ch, TIMER * timer )
{
    if ( !timer ) {
        bug( "%s", "extract_timer: NULL timer" );
        return;
    }
    UNLINK( timer, ch->first_timer, ch->last_timer, next, prev );
    DISPOSE( timer );
    return;
}

void remove_timer( CHAR_DATA *ch, short type )
{
    TIMER                  *timer;

    for ( timer = ch->first_timer; timer; timer = timer->next )
        if ( timer->type == type )
            break;
    if ( timer )
        extract_timer( ch, timer );
}

bool in_soft_range( CHAR_DATA *ch, AREA_DATA *tarea )
{
    if ( IS_IMMORTAL( ch ) )
        return TRUE;
    else if ( IS_NPC( ch ) )
        return TRUE;
    else if ( ch->level >= tarea->low_soft_range || ch->level <= tarea->hi_soft_range )
        return TRUE;
    else
        return FALSE;
}

bool can_astral( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( victim == ch || !victim->in_room || IS_SET( victim->in_room->room_flags, ROOM_PRIVATE )
         || IS_SET( victim->in_room->room_flags, ROOM_SOLITARY )
         || IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
         || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL )
         || IS_SET( victim->in_room->room_flags, ROOM_DEATH )
         || IS_SET( victim->in_room->room_flags, ROOM_PROTOTYPE ) || ( IS_NPC( victim )
                                                                       && xIS_SET( victim->act,
                                                                                   ACT_PROTOTYPE ) ) )
        return FALSE;
    else
        return TRUE;
}

bool in_hard_range( CHAR_DATA *ch, AREA_DATA *tarea )
{
    if ( IS_IMMORTAL( ch ) )
        return TRUE;
    else if ( IS_NPC( ch ) )
        return TRUE;
    else if ( ch->level >= tarea->low_hard_range && ch->level <= tarea->hi_hard_range )
        return TRUE;
    else
        return FALSE;
}

/* Scryn, standard luck check 2/2/96 */
bool chance( CHAR_DATA *ch, short percent )
{
    short                   ms;

    if ( !ch ) {
        bug( "%s", "Chance: null ch!" );
        return FALSE;
    }
    /*
     * Mental state bonus/penalty:  Your mental state is a ranged value with
     * * zero (0) being at a perfect mental state (bonus of 10).
     * * negative values would reflect how sedated one is, and
     * * positive values would reflect how stimulated one is.
     * * In most circumstances you'd do best at a perfectly balanced state.
     */

    ms = 10 - abs( ch->mental_state );
    if ( ( number_percent(  ) - get_curr_lck( ch ) + 13 - ms ) <= percent )
        return TRUE;
    else
        return FALSE;
}

bool chance_attrib( CHAR_DATA *ch, short percent, short attrib )
{
    if ( !ch ) {
        bug( "%s", "Chance: null ch!" );
        return FALSE;
    }
    if ( number_percent(  ) - get_curr_lck( ch ) + 13 - attrib + 13 <= percent )
        return TRUE;
    else
        return FALSE;
}

void clone_affects( OBJ_DATA *obj, OBJ_DATA *cobj )
{
    AFFECT_DATA            *paf,
                           *cpaf;

    for ( cpaf = cobj->first_affect; cpaf; cpaf = cpaf->next ) {
        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = cpaf->type;
        paf->duration = cpaf->duration;
        paf->location = cpaf->location;
        paf->modifier = cpaf->modifier;
        paf->bitvector = cpaf->bitvector;
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        top_affect++;
    }
}

void clone_extradescs( OBJ_DATA *obj, OBJ_DATA *cobj )
{
    EXTRA_DESCR_DATA       *ed,
                           *ced;

    for ( ced = cobj->first_extradesc; ced; ced = ced->next ) {
        CREATE( ed, EXTRA_DESCR_DATA, 1 );

        ed->keyword = QUICKLINK( ced->keyword );
        ed->description = QUICKLINK( ced->description );
        LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
        top_ed++;
    }
}

/* Make a simple clone of an object (no extras...yet)  -Thoric */
OBJ_DATA               *clone_object( OBJ_DATA *obj )
{
    OBJ_DATA               *clone;

    CREATE( clone, OBJ_DATA, 1 );

    clone->pIndexData = obj->pIndexData;
    clone->name = QUICKLINK( obj->name );
    clone->short_descr = QUICKLINK( obj->short_descr );
    clone->description = QUICKLINK( obj->description );
    clone->action_desc = QUICKLINK( obj->action_desc );
    clone->item_type = obj->item_type;
    clone->extra_flags = obj->extra_flags;
    clone->wear_flags = obj->wear_flags;
    clone->wear_loc = obj->wear_loc;
    clone->weight = obj->weight;
    clone->cost = obj->cost;
    clone->currtype = obj->currtype;
    clone->color = obj->color;
    clone->level = obj->level;
    clone->timer = obj->timer;
    clone->value[0] = obj->value[0];
    clone->value[1] = obj->value[1];
    clone->value[2] = obj->value[2];
    clone->value[3] = obj->value[3];
    clone->value[4] = obj->value[4];
    clone->value[5] = obj->value[5];
    clone->value[6] = obj->value[6];
    clone->count = 1;
    ++obj->pIndexData->count;
    ++numobjsloaded;
    ++physicalobjects;
    cur_obj_serial = UMAX( ( cur_obj_serial + 1 ) & ( BV30 - 1 ), 1 );
    clone->serial = clone->pIndexData->serial = cur_obj_serial;
    clone_extradescs( clone, obj );
    clone_affects( clone, obj );
    LINK( clone, first_object, last_object, next, prev );
    return clone;
}

/* Compare the extra descriptions of two objects and see if they match */
bool has_same_extradescs( OBJ_DATA *obj, OBJ_DATA *nobj )
{
    EXTRA_DESCR_DATA       *ed,
                           *ned;
    char                   *argument,
                           *nargument;
    char                    arg[MSL],
                            narg[MSL];

    if ( !obj->first_extradesc && !nobj->first_extradesc )
        return TRUE;
    if ( !obj->first_extradesc && nobj->first_extradesc )
        return FALSE;
    if ( obj->first_extradesc && !nobj->first_extradesc )
        return FALSE;

    /*
     * Check each extra description 
     */
    for ( ed = obj->first_extradesc, ned = nobj->first_extradesc; ed && ned;
          ed = ed->next, ned = ned->next ) {
        /*
         * If this is it for one and not the other return FALSE 
         */
        if ( ed->next && !ned->next )
            return FALSE;
        if ( !ed->next && ned->next )
            return FALSE;

        /*
         * Check to make sure keywords match and are in the same places etc... 
         */
        argument = ed->keyword;
        nargument = ned->keyword;
        for ( ;; ) {
            argument = one_argument( argument, arg );
            nargument = one_argument( nargument, narg );
            if ( arg[0] == '\0' && narg[0] == '\0' )
                break;
            if ( arg[0] == '\0' && narg[0] != '\0' )
                return FALSE;
            if ( arg[0] != '\0' && narg[0] == '\0' )
                return FALSE;
            if ( str_cmp( arg, narg ) )
                return FALSE;
        }

        /*
         * Check to make sure the descriptions line up exactly 
         */
        argument = ed->description;
        nargument = ned->description;
        for ( ;; ) {
            argument = one_argument( argument, arg );
            nargument = one_argument( nargument, narg );
            if ( arg[0] == '\0' && narg[0] == '\0' )
                break;
            if ( arg[0] == '\0' && narg[0] != '\0' )
                return FALSE;
            if ( arg[0] != '\0' && narg[0] == '\0' )
                return FALSE;
            if ( str_cmp( arg, narg ) )
                return FALSE;
        }
    }

    /*
     * Well looks like it all matches up so allow them to be groupped 
     */
    return TRUE;
}

bool has_same_affects( OBJ_DATA *obj, OBJ_DATA *cobj )
{
    AFFECT_DATA            *paf,
                           *cpaf;

    paf = obj->first_affect;
    cpaf = cobj->first_affect;

    if ( !paf && !cpaf )
        return TRUE;
    if ( paf && !cpaf )
        return FALSE;
    if ( !paf && cpaf )
        return FALSE;
    for ( ; paf && cpaf; paf = paf->next, cpaf = cpaf->next ) {
        if ( !paf->next && cpaf->next )
            return FALSE;
        if ( paf->next && !cpaf->next )
            return FALSE;
        if ( paf->type != cpaf->type )
            return FALSE;
        if ( paf->duration != cpaf->duration )
            return FALSE;
        if ( paf->location != cpaf->location )
            return FALSE;
        if ( paf->modifier != cpaf->modifier )
            return FALSE;
        if ( !xSAME_BITS( paf->bitvector, cpaf->bitvector ) )
            return FALSE;
    }

    return TRUE;
}

/*
 * If possible group obj2 into obj1     -Thoric
 * This code, along with clone_object, obj->count, and special support
 * for it implemented throughout handler.c and save.c should show improved
 * performance on MUDs with players that hoard tons of potions and scrolls
 * as this will allow them to be grouped together both in memory, and in
 * the player files.
 */
OBJ_DATA               *group_object( OBJ_DATA *obj1, OBJ_DATA *obj2 )
{
    if ( !obj1 || !obj2 )
        return NULL;
    if ( obj1 == obj2 )
        return obj1;

    if ( obj1->pIndexData == obj2->pIndexData && ( ( VLD_STR( obj1->name ) && VLD_STR( obj2->name ) && !str_cmp( obj1->name, obj2->name ) ) || ( !obj1->name && !obj2->name ) ) && ( ( VLD_STR( obj1->short_descr ) && VLD_STR( obj2->short_descr ) && !str_cmp( obj1->short_descr, obj2->short_descr ) ) || ( !obj1->short_descr && !obj2->short_descr ) ) && ( ( VLD_STR( obj1->description ) && VLD_STR( obj2->description ) && !str_cmp( obj1->description, obj2->description ) ) || ( !obj1->description && !obj2->description ) ) && obj1->item_type == obj2->item_type && xSAME_BITS( obj1->extra_flags, obj2->extra_flags ) && obj1->wear_flags == obj2->wear_flags && obj1->wear_loc == obj2->wear_loc && obj1->weight == obj2->weight && obj1->cost == obj2->cost && !IS_OBJ_STAT( obj1, ITEM_NOGROUP ) && !IS_OBJ_STAT( obj2, ITEM_NOGROUP ) && obj1->level == obj2->level && obj1->timer == obj2->timer && obj1->value[0] == obj2->value[0] && obj1->value[1] == obj2->value[1] && obj1->value[2] == obj2->value[2] && obj1->value[3] == obj2->value[3] && obj1->value[4] == obj2->value[4] && obj1->value[5] == obj2->value[5] && obj1->value[6] == obj2->value[6] && !obj1->first_content && !obj2->first_content && obj1->count + obj2->count > 0    /* prevent 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     * count 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     * overflow 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     */
         && obj1->currtype == obj2->currtype && obj1->color == obj2->color
         && has_same_affects( obj1, obj2 ) && has_same_extradescs( obj1, obj2 ) ) {
        obj1->count += obj2->count;
        obj1->pIndexData->count += obj2->count;
        numobjsloaded += obj2->count;
        extract_obj( obj2 );
        return obj1;
    }
    return obj2;
}

/* Split off a grouped object -Thoric
 * decreased obj's count to num, and creates a new object containing the rest
 */
void split_obj( OBJ_DATA *obj, int num )
{
    int                     count = obj->count;
    OBJ_DATA               *rest;

    if ( count <= num || num == 0 )
        return;
    rest = clone_object( obj );
    --obj->pIndexData->count;                          /* since clone_object() ups this
                                                        * value */
    --numobjsloaded;
    rest->count = obj->count - num;
    obj->count = num;
    if ( obj->carried_by ) {
        LINK( rest, obj->carried_by->first_carrying, obj->carried_by->last_carrying, next_content,
              prev_content );
        rest->carried_by = obj->carried_by;
        rest->in_room = NULL;
        rest->in_obj = NULL;
    }
    else if ( obj->in_room ) {
        LINK( rest, obj->in_room->first_content, obj->in_room->last_content, next_content,
              prev_content );
        rest->carried_by = NULL;
        rest->in_room = obj->in_room;
        rest->in_obj = NULL;
    }
    else if ( obj->in_obj ) {
        LINK( rest, obj->in_obj->first_content, obj->in_obj->last_content, next_content,
              prev_content );
        rest->in_obj = obj->in_obj;
        rest->in_room = NULL;
        rest->carried_by = NULL;
    }
}

/* Done to regroup objects after obj_update is done */
void regroup_obj( OBJ_DATA *obj )
{
    OBJ_DATA               *container;
    CHAR_DATA              *ch;
    ROOM_INDEX_DATA        *room;

    if ( obj->reset )                                  /* Don't do this with objects that 
                                                        * are resets, causes alot of
                                                        * issues */
        return;

    if ( obj->carried_by ) {
        /*
         * Dont do anything if its being worn 
         */
        if ( obj->wear_loc != WEAR_NONE )
            return;
        ch = obj->carried_by;
        obj_from_char( obj );
        obj_to_char( obj, ch );
    }
    else if ( obj->in_room ) {
        room = obj->in_room;
        obj_from_room( obj );
        obj_to_room( obj, room );
    }
    else if ( obj->in_obj ) {
        container = obj->in_obj;
        obj_from_obj( obj );
        obj_to_obj( obj, container );
    }
}

void separate_obj( OBJ_DATA *obj )
{
    split_obj( obj, 1 );
}

/*
 * Empty an obj's contents... optionally into another obj, or a room
 */
bool empty_obj( OBJ_DATA *obj, OBJ_DATA *destobj, ROOM_INDEX_DATA *destroom, bool skelerot )
{
    OBJ_DATA               *otmp,
                           *otmp_next;
    CHAR_DATA              *ch = obj->carried_by;
    bool                    movedsome = FALSE;

    if ( !obj ) {
        bug( "%s", "empty_obj: NULL obj" );
        return FALSE;
    }
    if ( destobj || ( !destroom && !ch && ( destobj = obj->in_obj ) != NULL ) ) {
        for ( otmp = obj->first_content; otmp; otmp = otmp_next ) {
            if ( skelerot ) {
                otmp->timer = 5;
                xSET_BIT( otmp->extra_flags, ITEM_SKELEROT );
            }

            otmp_next = otmp->next_content;
            /*
             * only keys on a keyring 
             */
            if ( destobj->item_type == ITEM_KEYRING && otmp->item_type != ITEM_KEY )
                continue;
            if ( destobj->item_type == ITEM_QUIVER && otmp->item_type != ITEM_PROJECTILE )
                continue;
            if ( ( destobj->item_type == ITEM_CONTAINER || destobj->item_type == ITEM_KEYRING
                   || destobj->item_type == ITEM_QUIVER )
                 && get_obj_weight( otmp, FALSE ) + get_obj_weight( destobj,
                                                                    FALSE ) > destobj->value[0] )
                continue;
            obj_from_obj( otmp );
            obj_to_obj( otmp, destobj );
            movedsome = TRUE;
        }
        return movedsome;
    }
    if ( destroom || ( !ch && ( destroom = obj->in_room ) != NULL ) ) {
        for ( otmp = obj->first_content; otmp; otmp = otmp_next ) {
            if ( skelerot ) {
                otmp->timer = 5;
                xSET_BIT( otmp->extra_flags, ITEM_SKELEROT );
            }
            otmp_next = otmp->next_content;
            if ( ch && HAS_PROG( otmp->pIndexData, DROP_PROG ) && otmp->count > 1 ) {
                separate_obj( otmp );
                obj_from_obj( otmp );
                if ( !otmp_next )
                    otmp_next = obj->first_content;
            }
            else
                obj_from_obj( otmp );
            otmp = obj_to_room( otmp, destroom );
            if ( ch ) {
                oprog_drop_trigger( ch, otmp );        /* mudprogs */
                if ( char_died( ch ) )
                    ch = NULL;
            }
            movedsome = TRUE;
        }
        return movedsome;
    }
    if ( ch ) {
        for ( otmp = obj->first_content; otmp; otmp = otmp_next ) {
            if ( skelerot && otmp->timer == 0 ) {
                otmp->timer = 6;
                xSET_BIT( otmp->extra_flags, ITEM_SKELEROT );
            }

            otmp_next = otmp->next_content;
            obj_from_obj( otmp );
            obj_to_char( otmp, ch );
            movedsome = TRUE;
        }
        return movedsome;
    }
    bug( "empty_obj: could not determine a destination for vnum %d", obj->pIndexData->vnum );
    return FALSE;
}

/*
 * Improve mental state      -Thoric
 */
void better_mental_state( CHAR_DATA *ch, int mod )
{
    int                     c = URANGE( 0, abs( mod ), 20 );
    int                     con = get_curr_con( ch );

    c += number_percent(  ) < con ? 1 : 0;

    if ( ch->mental_state < 0 )
        ch->mental_state = URANGE( -100, ch->mental_state + c, 0 );
    else if ( ch->mental_state > 0 )
        ch->mental_state = URANGE( 0, ch->mental_state - c, 100 );
}

/*
 * Deteriorate mental state     -Thoric
 */
void worsen_mental_state( CHAR_DATA *ch, int mod )
{
    int                     c = URANGE( 0, abs( mod ), 20 );
    int                     con = get_curr_con( ch );

    c -= number_percent(  ) < con ? 1 : 0;
    if ( c < 1 )
        return;

    /*
     * Nuisance flag makes state worsen quicker. --Shaddai 
     */
    if ( !IS_NPC( ch ) && ch->pcdata->nuisance && ch->pcdata->nuisance->flags > 2 )
        c = ( int ) ( c +
                      .4 * ( ( ch->pcdata->nuisance->flags - 2 ) * ch->pcdata->nuisance->power ) );

    if ( ch->mental_state < 0 )
        ch->mental_state = URANGE( -100, ch->mental_state - c, 100 );
    else if ( ch->mental_state > 0 )
        ch->mental_state = URANGE( -100, ch->mental_state + c, 100 );
    else
        ch->mental_state -= c;
}

/*
 * Add gold to an area's economy     -Thoric
 */
void boost_economy( AREA_DATA *tarea, int gold, int type )
{
    while ( gold >= 1000000000 ) {
        ++tarea->high_economy[type];
        gold -= 1000000000;
    }
    tarea->low_economy[type] += gold;
    while ( tarea->low_economy[type] >= 1000000000 ) {
        ++tarea->high_economy[type];
        tarea->low_economy[type] -= 1000000000;
    }
}

/*
 * Take gold from an area's economy                             -Thoric
 */
void lower_economy( AREA_DATA *tarea, int gold, int type )
{
    while ( gold >= 1000000000 ) {
        --tarea->high_economy[type];
        gold -= 1000000000;
    }
    tarea->low_economy[type] -= gold;
    while ( tarea->low_economy[type] < 0 ) {
        --tarea->high_economy[type];
        tarea->low_economy[type] += 1000000000;
    }
}

/*
 * Check to see if economy has at least this much gold             -Thoric
 */
bool economy_has( AREA_DATA *tarea, int gold, int type )
{
    int                     hasgold =
        ( ( tarea->high_economy[type] > 0 ) ? 1 : 0 ) * 1000000000 + tarea->low_economy[type];

    if ( hasgold >= gold )
        return TRUE;
    return FALSE;
}

void economize_mobgold( CHAR_DATA *mob )
{
    AREA_DATA              *tarea;
    int                     gold,
                            type;

    for ( type = 3; type < MAX_CURR_TYPE; type++ ) {
        /*
         * make sure it isn't way too much 
         */

        if ( !mob->in_room )
            return;
        tarea = mob->in_room->area;

        gold =
            ( ( tarea->high_economy[type] > 0 ) ? 1 : 0 ) * 1000000000 + tarea->low_economy[type];

        GET_MONEY( mob, type ) = URANGE( 1, GET_MONEY( mob->pIndexData, type ), 100000 );
        if ( GET_MONEY( mob, type ) )
            lower_economy( tarea, GET_MONEY( mob, type ), type );

    }
}

/*
 * Add another notch on that there belt... ;)
 * Keep track of the last so many kills by vnum   -Thoric
 */

void add_kill( CHAR_DATA *ch, CHAR_DATA *mob )
{
    int                     vnum;

    if ( IS_NPC( ch ) ) {
        bug( "%s", "add_kill: trying to add kill to npc" );
        return;
    }
    if ( !IS_NPC( mob ) ) {
        bug( "%s", "add_kill: trying to add kill non-npc" );
        return;
    }

    vnum = mob->pIndexData->vnum;

    memmove( ( char * ) ch->pcdata->killed + sizeof( KILLED_DATA ), ch->pcdata->killed,
             ( MAX_KILLTRACK - 1 ) * sizeof( KILLED_DATA ) );

    ch->pcdata->killed[0].vnum = vnum;

}

/*
 * Return how many times this player has killed this mob  -Thoric
 * Only keeps track of so many (MAX_KILLTRACK), and keeps track by vnum
 */

int times_killed( CHAR_DATA *ch, CHAR_DATA *mob )
{
    int                     x,
                            vnum;
    short                   track = 0;

    if ( IS_NPC( ch ) ) {
        bug( "%s", "times_killed: ch is not a player" );
        return 0;
    }
    if ( !IS_NPC( mob ) ) {
        bug( "%s", "add_kill: mob is not a mobile" );
        return 0;
    }

    vnum = mob->pIndexData->vnum;
//  track = URANGE(2, ((ch->level + 3) * MAX_KILLTRACK) / LEVEL_AVATAR, MAX_KILLTRACK);

    for ( x = 0; x < MAX_KILLTRACK; x++ )
        if ( ch->pcdata->killed[x].vnum == vnum )
            track += 1;
        else if ( ch->pcdata->killed[x].vnum == 0 )
            break;
    return track;
}

/*
 * Return how many times this player has killed this mob	-Thoric
 * Only keeps track of so many (MAX_KILLTRACK), and keeps track by vnum
 */
/*
int times_killed( CHAR_DATA * ch, CHAR_DATA * mob )
{
   int vnum, x, count = 0;

   if( IS_NPC( ch ) )
   {
      bug( "%s", "times_killed: ch is not a player" );
      return 0;
   }
   if( !IS_NPC( mob ) )
   {
      bug( "%s", "add_kill: mob is not a mobile" );
      return 0;
   }

   vnum = mob->pIndexData->vnum;
   short track = MAX_KILLTRACK;
   for( x = 0; x < track; x++ )
      if( ch->pcdata->killed[x].vnum == vnum )
         count += 1;
      else if( ch->pcdata->killed[x].vnum == 0 )
         break;
   return count;
}
*/

/*
 * returns area with name matching input string
 * Last Modified : July 21, 1997
 * Fireblade
 */
AREA_DATA              *get_area( char *name )
{
    AREA_DATA              *pArea;

    if ( !name ) {
        bug( "%s", "get_area: NULL input string." );
        return NULL;
    }

    for ( pArea = first_area; pArea; pArea = pArea->next ) {
        if ( nifty_is_name( name, pArea->name ) )
            break;
    }

    if ( !pArea ) {
        for ( pArea = first_build; pArea; pArea = pArea->next ) {
            if ( nifty_is_name( name, pArea->name ) )
                break;
        }
    }

    return pArea;
}

AREA_DATA              *get_area_obj( OBJ_INDEX_DATA *pObjIndex )
{
    AREA_DATA              *pArea;

    if ( !pObjIndex ) {
        bug( "%s", "get_area_obj: pObjIndex is NULL." );
        return NULL;
    }
    for ( pArea = first_area; pArea; pArea = pArea->next ) {
        if ( pObjIndex->vnum >= pArea->low_o_vnum && pObjIndex->vnum <= pArea->hi_o_vnum )
            break;
    }
    return pArea;
}

int strlen_color( char *argument )
{
    char                   *str;
    unsigned int            i;
    int                     length;

    length = 0;
    str = argument;

    if ( argument == NULL )
        return length;

    for ( i = 0; i < strlen( argument ); i++ ) {
        if ( ( str[i] != '&' ) && ( str[i] != '^' ) ) {
            length++;
            continue;
        }

        if ( ( str[i] == '&' ) || ( str[i] == '^' ) ) {
            if ( ( str[i] == '&' ) && ( str[i + 1] == '&' ) )
                length += 2;
            else if ( ( str[i] == '^' ) && ( str[i + 1] == '^' ) )
                length += 2;
            else
                length--;
        }
    }

    return length;
}

/* Bugged? "*/
const char             *PERS( CHAR_DATA *ch, CHAR_DATA *looker )
{
/* Deal with morphed players first.. */
    if ( ch->morph != NULL && ch->morph->morph != NULL ) {
        if ( IS_IMMORTAL( looker ) )
            return ch->name;

        if ( looker->level > ch->level )
            return ch->name;

        if ( can_see( looker, ch ) )                   // Can see them?
            return ch->morph->morph->short_desc;
    }
    else {
        if ( can_see( looker, ch ) ) {
            if ( IS_NPC( ch ) )
                return ( ch->short_descr ) ? ch->short_descr : "NULL short_descr";
            else
                return ( ch->name ) ? ch->name : ( char * ) "NULL ch->name";
        }                                              // Can't see them!
    }

/* If we are here, looker can't see ch */
    if ( !IS_IMMORTAL( looker ) )
        return "someone";

/* If we're here, it's an imm.. show them! */
    if ( IS_NPC( ch ) )
        return ch->short_descr;

    return ch->name;
}

char                   *affect_bit_name( EXT_BV * vector )
{
    static char             buf[512];

    buf[0] = '\0';
    if ( xIS_SET( *vector, AFF_BLINDNESS ) )
        mudstrlcat( buf, " blindness", 512 );
    if ( xIS_SET( *vector, AFF_INVISIBLE ) )
        mudstrlcat( buf, " invisible", 512 );
    if ( xIS_SET( *vector, AFF_DETECT_EVIL ) )
        mudstrlcat( buf, " detect_evil", 512 );
    if ( xIS_SET( *vector, AFF_DETECT_INVIS ) )
        mudstrlcat( buf, " detect_invis", 512 );
    if ( xIS_SET( *vector, AFF_DETECT_MAGIC ) )
        mudstrlcat( buf, " detect_magic", 512 );
    if ( xIS_SET( *vector, AFF_DETECT_HIDDEN ) )
        mudstrlcat( buf, " detect_hidden", 512 );
    if ( xIS_SET( *vector, AFF_FASCINATE ) )
        mudstrlcat( buf, " fascinate", 512 );
    if ( xIS_SET( *vector, AFF_SANCTUARY ) )
        mudstrlcat( buf, " sanctuary", 512 );
    if ( xIS_SET( *vector, AFF_FAERIE_FIRE ) )
        mudstrlcat( buf, " faerie_fire", 512 );
    if ( xIS_SET( *vector, AFF_INFRARED ) )
        mudstrlcat( buf, " infrared", 512 );
    if ( xIS_SET( *vector, AFF_CURSE ) )
        mudstrlcat( buf, " curse", 512 );
    if ( xIS_SET( *vector, AFF_SHIELD ) )
        mudstrlcat( buf, " shield", 512 );
    if ( xIS_SET( *vector, AFF_POISON ) )
        mudstrlcat( buf, " poison", 512 );
    if ( xIS_SET( *vector, AFF_PROTECT ) )
        mudstrlcat( buf, " protect", 512 );
    if ( xIS_SET( *vector, AFF_PARALYSIS ) )
        mudstrlcat( buf, " paralysis", 512 );
    if ( xIS_SET( *vector, AFF_SNEAK ) )
        mudstrlcat( buf, " sneak", 512 );
    if ( xIS_SET( *vector, AFF_HIDE ) )
        mudstrlcat( buf, " hide", 512 );
    if ( xIS_SET( *vector, AFF_SLEEP ) )
        mudstrlcat( buf, " sleep", 512 );
    if ( xIS_SET( *vector, AFF_CHARM ) )
        mudstrlcat( buf, " charm", 512 );
    if ( xIS_SET( *vector, AFF_FLYING ) )
        mudstrlcat( buf, " flying", 512 );
    if ( xIS_SET( *vector, AFF_PASS_DOOR ) )
        mudstrlcat( buf, " pass_door", 512 );
    if ( xIS_SET( *vector, AFF_FLOATING ) )
        mudstrlcat( buf, " floating", 512 );
    if ( xIS_SET( *vector, AFF_TRUESIGHT ) )
        mudstrlcat( buf, " truesight", 512 );
    if ( xIS_SET( *vector, AFF_DETECTTRAPS ) )
        mudstrlcat( buf, " detect_traps", 512 );
    if ( xIS_SET( *vector, AFF_SCRYING ) )
        mudstrlcat( buf, " scrying", 512 );
    if ( xIS_SET( *vector, AFF_FIRESHIELD ) )
        mudstrlcat( buf, " fireshield", 512 );
    if ( xIS_SET( *vector, AFF_SHOCKSHIELD ) )
        mudstrlcat( buf, " shockshield", 512 );
    if ( xIS_SET( *vector, AFF_DETECT_SNEAK ) )
        mudstrlcat( buf, " detect sneak", 512 );
    if ( xIS_SET( *vector, AFF_ICESHIELD ) )
        mudstrlcat( buf, " iceshield", 512 );
    if ( xIS_SET( *vector, AFF_POSSESS ) )
        mudstrlcat( buf, " possess", 512 );
    if ( xIS_SET( *vector, AFF_BERSERK ) )
        mudstrlcat( buf, " berserk", 512 );
    if ( xIS_SET( *vector, AFF_AQUA_BREATH ) )
        mudstrlcat( buf, " aqua_breath", 512 );

    if ( xIS_SET( *vector, AFF_RECURRINGSPELL ) )
        mudstrlcat( buf, " recurringspell", 512 );
    if ( xIS_SET( *vector, AFF_CONTAGIOUS ) )
        mudstrlcat( buf, " contagious", 512 );
    if ( xIS_SET( *vector, AFF_ACIDMIST ) )
        mudstrlcat( buf, " acidmist", 512 );
    if ( xIS_SET( *vector, AFF_VENOMSHIELD ) )
        mudstrlcat( buf, " venomshield", 512 );
    if ( xIS_SET( *vector, AFF_SHAPESHIFT ) )
        mudstrlcat( buf, " shapeshift", 512 );
    if ( xIS_SET( *vector, AFF_DEMONIC_SIGHT ) )
        mudstrlcat( buf, " demonic_sight", 512 );
    if ( xIS_SET( *vector, AFF_NOSIGHT ) )
        mudstrlcat( buf, " nosight", 512 );
    if ( xIS_SET( *vector, AFF_SPIKE ) )
        mudstrlcat( buf, " spike", 512 );
    if ( xIS_SET( *vector, AFF_SHOCKSHIELD ) )
        mudstrlcat( buf, " shockshield", 512 );

/* VOLK TODO: Finish if needed */
    return ( buf[0] != '\0' ) ? buf + 1 : ( char * ) "none";
}

void set_position( CHAR_DATA *ch, int position )
{
    if ( ch->position == POS_CRAWL ) {
        if ( !IS_NPC( ch ) )
            ch->height = ch->pcdata->tmpcrawl;
        else
            ch->height *= 2;

        if ( position == POS_FIGHTING || position == POS_BERSERK || position == POS_EVASIVE
             || position == POS_DEFENSIVE || position == POS_AGGRESSIVE ) {
            send_to_char( "You stand up and begin to fight.", ch );
        }
    }
    if ( position == POS_CRAWL ) {
        if ( !IS_NPC( ch ) )
            ch->pcdata->tmpcrawl = ch->height;

        ch->height /= 2;
    }
    ch->position = position;
}

void remove_file( const char *filename )
{
    unlink( filename );
}

/*
 * Who's carrying an item -- recursive for nested objects       -Thoric
 */
CHAR_DATA              *carried_by( OBJ_DATA *obj )
{
    if ( obj->in_obj )
        return carried_by( obj->in_obj );

    return obj->carried_by;
}

/* After a new affect is added or the object is fully loaded combine the affects */
void combine_affects( OBJ_DATA *obj )
{
    AFFECT_DATA            *paf,
                           *tmpaf,
                           *tmpaf_next;

    if ( !obj )
        return;

    /*
     * Start by going through the list once 
     */
    for ( paf = obj->first_affect; paf; paf = paf->next ) {
        /*
         * Now check all the affects past this one to see if some can be combined with this
         * one 
         */
        for ( tmpaf = paf->next; tmpaf; tmpaf = tmpaf_next ) {
            tmpaf_next = tmpaf->next;
            if ( tmpaf->type == paf->type && tmpaf->location == paf->location
                 && xSAME_BITS( tmpaf->bitvector, paf->bitvector ) ) {
                if ( tmpaf->location == APPLY_WEAPONSPELL || tmpaf->location == APPLY_WEARSPELL ) {
                    if ( paf->modifier != tmpaf->modifier )
                        continue;
                    UNLINK( tmpaf, obj->first_affect, obj->last_affect, next, prev );
                    DISPOSE( tmpaf );
                    --top_affect;
                }
                paf->modifier += tmpaf->modifier;
                UNLINK( tmpaf, obj->first_affect, obj->last_affect, next, prev );
                DISPOSE( tmpaf );
                --top_affect;
            }
        }
    }
}

/* Mud Sound Protocol Functions
 * Original author unknown.
 * Smaug port by Chris Coulter (aka Gabriel Androctus)
 */

/* Trigger sound to character
 *
 * "fname" is the name of the sound file to be played
 * "vol" is the volume level to play the sound at    
 * "repeats" is the number of times to play the sound
 * "priority" is the priority of the sound
 * "type" is the sound class 
 * "URL" is the optional download URL for the sound file
 * "ch" is the character to play the sound for
 *
 * More detailed information at http://www.zuggsoft.com/
*/
void sound_to_char( const char *fname, int vol, int repeats, int priority, const char *type,
                    const char *url, CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH];

    if ( !xIS_SET( ch->act, PLR_SOUND ) )
        return;

    if ( vol == MSP_DEFAULT )
        vol = 100;
    if ( repeats == MSP_DEFAULT )
        repeats = 1;
    if ( priority == MSP_DEFAULT )
        priority = 50;

    if ( url[0] != '\0' )
        ch_printf( ch, "!!SOUND(%s/%s V=%d L=%d P=%d T=%s U=%s)\r\n", type, fname, vol, repeats,
                   priority, type, url );
    else
        ch_printf( ch, "!!SOUND(%s/%s V=%d L=%d P=%d T=%s)\r\n", type, fname, vol, repeats,
                   priority, type );
}

/* Trigger music file ...
 *
 * "fname" is the name of the music file to be played
 * "vol" is the volume level to play the music at    
 * "repeats" is the number of times to play the music file
 * "continu" specifies whether the file should be restarted if requested again
 * "type" is the sound class 
 * "URL" is the optional download URL for the sound file
 * "ch" is the character to play the music for 
 *
 * more detailed information at: http://www.zuggsoft.com/
*/
void music_to_char( const char *fname, int vol, int repeats, int continu, const char *type,
                    const char *url, CHAR_DATA *ch )
{
    char                    buf[MAX_STRING_LENGTH];

    if ( !xIS_SET( ch->act, PLR_MUSIC ) )
        return;

    if ( vol == MSP_DEFAULT )
        vol = 100;
    if ( repeats == MSP_DEFAULT )
        repeats = 1;
    if ( continu == MSP_DEFAULT )
        continu = 1;

    if ( url[0] != '\0' )
        ch_printf( ch, "!!MUSIC(%s/%s V=%d L=%d C=%d T=%s U=%s)\r\n", type, fname, vol, repeats,
                   continu, type, url );
    else
        ch_printf( ch, "!!MUSIC(%s/%s V=%d L=%d C=%d T=%s)\r\n", type, fname, vol, repeats, continu,
                   type );

    return;
}

/* stop playing sound */
void reset_sound( CHAR_DATA *ch )
{
    if ( xIS_SET( ch->act, PLR_SOUND ) )
        send_to_char( "!!SOUND(Off)\r\n", ch );
}

/* stop playing music */
void reset_music( CHAR_DATA *ch )
{
    if ( xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!MUSIC(Off)\r\n", ch );
}

bool                    check_parse_name( char *name, bool newchar );
bool valid_pfile( const char *filename )
{
    char                    buf[MSL];
    struct stat             fst;

    if ( !filename || filename[0] == '\0' )
        return false;

    snprintf( buf, sizeof( buf ), "%s%c/%s", PLAYER_DIR, tolower( filename[0] ),
              capitalize( filename ) );
    if ( stat( buf, &fst ) == -1 || !check_parse_name( capitalize( filename ), FALSE ) )
        return FALSE;
    return TRUE;
}

/* Return real weight of an object, including weight of contents. */
int get_real_obj_weight( OBJ_DATA *obj )
{
    int                     weight;

    weight = obj->count * obj->weight;

    for ( obj = obj->first_content; obj; obj = obj->next_content )
        weight += get_real_obj_weight( obj );

    return weight;
}
