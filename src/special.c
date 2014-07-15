/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Special procedure module                                              *
 ***************************************************************************/

#include "h/mud.h"
#include <string.h>

/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN               *spec_lookup( const char *name )
{
    if ( !str_cmp( name, "spec_breath_any" ) )
        return spec_breath_any;
    if ( !str_cmp( name, "spec_breath_acid" ) )
        return spec_breath_acid;
    if ( !str_cmp( name, "spec_breath_fire" ) )
        return spec_breath_fire;
    if ( !str_cmp( name, "spec_breath_frost" ) )
        return spec_breath_frost;
    if ( !str_cmp( name, "spec_breath_gas" ) )
        return spec_breath_gas;
    if ( !str_cmp( name, "spec_breath_lightning" ) )
        return spec_breath_lightning;
    if ( !str_cmp( name, "spec_cast_adept" ) )
        return spec_cast_adept;
    if ( !str_cmp( name, "spec_cast_cleric" ) )
        return spec_cast_cleric;
    if ( !str_cmp( name, "spec_cast_mage" ) )
        return spec_cast_mage;
    if ( !str_cmp( name, "spec_cast_undead" ) )
        return spec_cast_undead;
    if ( !str_cmp( name, "spec_executioner" ) )
        return spec_executioner;
    if ( !str_cmp( name, "spec_fido" ) )
        return spec_fido;
    if ( !str_cmp( name, "spec_guard" ) )
        return spec_guard;
    if ( !str_cmp( name, "spec_janitor" ) )
        return spec_janitor;
    if ( !str_cmp( name, "spec_mayor" ) )
        return spec_mayor;
    if ( !str_cmp( name, "spec_poison" ) )
        return spec_poison;
    if ( !str_cmp( name, "spec_thief" ) )
        return spec_thief;
    if ( !str_cmp( name, "spec_greet" ) )
        return spec_greet;
    if ( !str_cmp( name, "spec_tiamat" ) )
        return spec_tiamat;
    if ( !str_cmp( name, "spec_basic_ai" ) )
        return spec_basic_ai;

    return 0;
}

/*
 * Given a pointer, return the appropriate spec fun text.
 */
const char             *lookup_spec( SPEC_FUN *special )
{
    if ( special == spec_breath_any )
        return "spec_breath_any";

    if ( special == spec_breath_acid )
        return "spec_breath_acid";
    if ( special == spec_breath_fire )
        return "spec_breath_fire";
    if ( special == spec_breath_frost )
        return "spec_breath_frost";
    if ( special == spec_breath_gas )
        return "spec_breath_gas";
    if ( special == spec_breath_lightning )
        return "spec_breath_lightning";
    if ( special == spec_cast_adept )
        return "spec_cast_adept";
    if ( special == spec_cast_cleric )
        return "spec_cast_cleric";
    if ( special == spec_cast_mage )
        return "spec_cast_mage";
    if ( special == spec_cast_undead )
        return "spec_cast_undead";
    if ( special == spec_executioner )
        return "spec_executioner";
    if ( special == spec_fido )
        return "spec_fido";
    if ( special == spec_guard )
        return "spec_guard";
    if ( special == spec_janitor )
        return "spec_janitor";
    if ( special == spec_mayor )
        return "spec_mayor";
    if ( special == spec_poison )
        return "spec_poison";
    if ( special == spec_thief )
        return "spec_thief";
    if ( special == spec_greet )
        return "spec_greet";
    if ( special == spec_tiamat )
        return "spec_tiamat";
    if ( special == spec_basic_ai )
        return "spec_basic_ai";
    return "";
}

/* if a spell casting mob is hating someone... try and summon them */
void summon_if_hating( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    char                    buf[MSL];
    char                    name[MIL];
    bool                    found = FALSE;

    if ( ch->position <= POS_SLEEPING )
        return;

    if ( ch->fighting || ch->fearing || !ch->hating
         || IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
        return;

    /*
     * if player is close enough to hunt... don't summon 
     */
    if ( ch->hunting )
        return;

    one_argument( ch->hating->name, name );

    /*
     * make sure the char exists - works even if player quits 
     */
    for ( victim = first_char; victim; victim = victim->next ) {
        if ( !str_cmp( ch->hating->name, victim->name ) ) {
            found = TRUE;
            break;
        }
    }

    if ( !found )
        return;
    if ( ch->in_room == victim->in_room )
        return;
    if ( !IS_NPC( victim ) )
        snprintf( buf, MSL, "summon 0.%s", name );
    else
        snprintf( buf, MSL, "summon %s", name );
    do_cast( ch, buf );
    return;
}

/*
 * Core procedure for dragons.
 */
bool dragon( CHAR_DATA *ch, char *spell_name )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    int                     sn;

    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;
        if ( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( !victim )
        return FALSE;

    if ( ( sn = skill_lookup( spell_name ) ) < 0 )
        return FALSE;
    ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
    return TRUE;
}

//Just getting started. -Taon
bool spec_basic_ai( CHAR_DATA *ch )
{
    CHAR_DATA              *victim,
                           *v_next;
    short                   rand = number_chance( 1, 5 );

    if ( ch->fighting || ch->position != POS_STANDING )
        return FALSE;

    // Check each person in the room.
    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;
        break;                                         // Just here to prevent madness.
        // -Taon
    }

    if ( !victim )
        return FALSE;

    log_string( "Warning: spec spec_basic_ai isn't complete and found on a mob." );
    return TRUE;
}

bool spec_tiamat( CHAR_DATA *ch )
{
    short                   chance;

    chance = number_range( 1, 5 );

    if ( ch->fighting ) {
        interpret( ch, ( char * ) "mppause 10" );
        act( AT_ACTION, "$n's five dragon heads roar deafeningly in concert!", ch, NULL, NULL,
             TO_ROOM );
        interpret( ch, ( char * ) "mppause 10" );
        interpret( ch, ( char * ) "stomp $n" );
        interpret( ch, ( char * ) "mppause 20" );
        if ( chance <= 2 ) {
            act( AT_ACTION, "$n's black dragon head maw opens wide!", ch, NULL, NULL, TO_ROOM );
            interpret( ch, ( char * ) "c 'acid breath'" );
            interpret( ch, ( char * ) "mppause 20" );
            interpret( ch, ( char * ) "mpat 4 look" );
            interpret( ch, ( char * ) "mppause 20" );
            interpret( ch, ( char * ) "stomp $n" );
            act( AT_ACTION, "$n's red dragon head maw opens wide!", ch, NULL, NULL, TO_ROOM );
            interpret( ch, ( char * ) "c 'fire breath'" );
        }
        interpret( ch, ( char * ) "mppause 20" );
        interpret( ch, ( char * ) "mpat 4 look" );
        interpret( ch, ( char * ) "mppause 20" );
        interpret( ch, ( char * ) "stomp $n" );
        if ( chance <= 4 ) {
            act( AT_ACTION, "$n's blue dragon head maw opens wide!", ch, NULL, NULL, TO_ROOM );
            interpret( ch, ( char * ) "c 'frost breath'" );
            interpret( ch, ( char * ) "mppause 20" );
            interpret( ch, ( char * ) "mpat 4 look" );
            interpret( ch, ( char * ) "mppause 20" );
            act( AT_ACTION, "$n's green dragon head maw opens wide!", ch, NULL, NULL, TO_ROOM );
            interpret( ch, ( char * ) "c 'gas breath'" );
            interpret( ch, ( char * ) "c 'bestow vitae'" );
        }
        interpret( ch, ( char * ) "mppause 20" );
        interpret( ch, ( char * ) "mpat 4 look" );
        interpret( ch, ( char * ) "mppause 20" );
        if ( chance == 5 ) {
            act( AT_ACTION, "$n's white dragon head maw opens wide!", ch, NULL, NULL, TO_ROOM );
            interpret( ch, ( char * ) "c 'lightning breath'" );
            interpret( ch, ( char * ) "gut $n" );
        }
        interpret( ch, ( char * ) "mppause 20" );
        interpret( ch, ( char * ) "mpat 4 look" );
        interpret( ch, ( char * ) "mppause 20" );
        interpret( ch, ( char * ) "mpat 4 look" );
        interpret( ch, ( char * ) "mppause 20" );
    }
    return TRUE;
}

/*
 * Special procedures for mobiles.
 */
bool spec_breath_any( CHAR_DATA *ch )
{
    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    switch ( number_bits( 3 ) ) {
        case 0:
            return spec_breath_fire( ch );
        case 1:
        case 2:
            return spec_breath_lightning( ch );
        case 3:
            return spec_breath_gas( ch );
        case 4:
            return spec_breath_acid( ch );
        case 5:
        case 6:
        case 7:
            return spec_breath_frost( ch );
    }

    return FALSE;
}

bool spec_breath_acid( CHAR_DATA *ch )
{
    return dragon( ch, ( char * ) "acid breath" );
}

bool spec_breath_fire( CHAR_DATA *ch )
{
    return dragon( ch, ( char * ) "fire breath" );
}

bool spec_breath_frost( CHAR_DATA *ch )
{
    return dragon( ch, ( char * ) "frost breath" );
}

bool spec_breath_gas( CHAR_DATA *ch )
{
    int                     sn;

    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    if ( ( sn = skill_lookup( "gas breath" ) ) < 0 )
        return FALSE;
    ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, NULL );
    return TRUE;
}

bool spec_breath_lightning( CHAR_DATA *ch )
{
    return dragon( ch, ( char * ) "lightning breath" );
}

bool spec_cast_adept( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    char                    affect[MIL],
                            affect2[MIL];
    AFFECT_DATA            *paf;
    SKILLTYPE              *skill;
    ch_ret                  retcode;
    short                   chance;

    chance = number_range( 1, 4 );
    if ( !IS_AWAKE( ch ) || ch->fighting )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;

        if ( victim != ch && can_see( ch, victim ) && number_bits( 1 ) == 0 )
            break;
    }

    if ( !victim )
        return FALSE;

    snprintf( affect, MIL, " " );
    for ( paf = victim->first_affect; paf; paf = paf->next )
        if ( ( skill = get_skilltype( paf->type ) ) != NULL ) {
            snprintf( affect2, MIL, "%s ", skill->name );
            mudstrlcat( affect, affect2, MIL );
        }

    switch ( number_bits( 3 ) ) {
        case 0:
            if ( !is_name( "armor", affect ) )
                spell_smaug( skill_lookup( "armor" ), ch->level, ch, victim );
            return TRUE;
        case 1:
            if ( !is_name( "bless", affect ) )
                spell_smaug( skill_lookup( "bless" ), ch->level, ch, victim );
            return TRUE;
        case 2:
            if ( IS_AFFECTED( victim, AFF_BLINDNESS ) )
                spell_cure_blindness( skill_lookup( "cure blindness" ), ch->level, ch, victim );
            return TRUE;
        case 3:
            if ( victim->hit < victim->max_hit ) {
                if ( victim->level < 5 )
                    spell_cure_serious( skill_lookup( "cure serious" ), ch->level, ch, victim );
                else if ( victim->level > 4 && victim->level <= 9 )
                    spell_cure_critical( skill_lookup( "cure critical" ), ch->level, ch, victim );
                else if ( victim->level > 9 && victim->level <= 19 )
                    spell_heal( skill_lookup( "heal" ), ch->level, ch, victim );
                else if ( victim->level > 19 && victim->level <= 40 && chance != 4 )
                    spell_greater_heal( skill_lookup( "greater heal" ), ch->level, ch, victim );
                else if ( victim->level > 40 && chance != 4 )
                    spell_smaug( skill_lookup( "restore life" ), ch->level, ch, victim );
                else if ( victim->level > 19 && chance == 4 ) {
                    if ( victim->hit < victim->max_hit / 2 ) {
                        send_to_char
                            ( "&WA white hot light fills you as your body receives life energies!\r\n",
                              victim );
                        victim->hit = victim->max_hit / 2;
                    }
                }
                return TRUE;
            }
        case 4:
            if ( IS_AFFECTED( victim, AFF_POISON ) )
                spell_cure_poison( skill_lookup( "cure affliction" ), ch->level, ch, victim );
            return TRUE;
        case 5:
            if ( victim->move < victim->max_move )
                spell_refresh( skill_lookup( "refresh" ), ch->level, ch, victim );
            return TRUE;
        case 6:
            if ( IS_AFFECTED( victim, AFF_CURSE ) )
                spell_remove_curse( skill_lookup( "remove curse" ), ch->level, ch, victim );
            return TRUE;
        case 7:
            if ( victim->hit < victim->max_hit ) {
                if ( victim->level < 5 )
                    spell_cure_serious( skill_lookup( "cure serious" ), ch->level, ch, victim );
                else if ( victim->level > 4 && victim->level <= 9 )
                    spell_cure_critical( skill_lookup( "cure critical" ), ch->level, ch, victim );
                else if ( victim->level > 9 && victim->level <= 19 )
                    spell_heal( skill_lookup( "heal" ), ch->level, ch, victim );
                else if ( victim->level > 19 && victim->level <= 40 )
                    spell_greater_heal( skill_lookup( "greater heal" ), ch->level, ch, victim );
                else if ( victim->level > 40 )
                    spell_smaug( skill_lookup( "restore life" ), ch->level, ch, victim );

            }
            return TRUE;
    }
    return FALSE;
}

bool spec_cast_cleric( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    const char             *spell;
    int                     sn;

    summon_if_hating( ch );

    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;
        if ( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( !victim || victim == ch )
        return FALSE;

    for ( ;; ) {
        int                     min_level;

        switch ( number_bits( 4 ) ) {
            case 0:
                min_level = 0;
                spell = "cause light";
                break;
            case 1:
                min_level = 3;
                spell = "cause serious";
                break;
            case 2:
                min_level = 6;
                spell = "earthquake";
                break;
            case 3:
                min_level = 7;
                spell = "blindness";
                break;
            case 4:
                min_level = 9;
                spell = "cause critical";
                break;
            case 5:
                min_level = 10;
                spell = "dispel evil";
                break;
            case 6:
                min_level = 12;
                spell = "curse";
                break;
            case 7:
                min_level = 13;
                spell = "flamestrike";
                break;
            case 8:
            case 9:
            case 10:
                min_level = 15;
                spell = "harm";
                break;
            default:
                min_level = 16;
                spell = "dispel magic";
                break;
        }

        if ( ch->level >= min_level )
            break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
    return TRUE;
}

bool spec_cast_mage( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    const char             *spell;
    int                     sn;

    summon_if_hating( ch );

    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;
        if ( who_fighting( victim ) && number_bits( 2 ) == 0 )
            break;
    }

    if ( !victim || victim == ch )
        return FALSE;

    for ( ;; ) {
        int                     min_level;

        switch ( number_bits( 4 ) ) {
            case 0:
                min_level = 0;
                spell = "magic missile";
                break;
            case 1:
                min_level = 3;
                spell = "chill touch";
                break;
            case 2:
                min_level = 7;
                spell = "weaken";
                break;
            case 3:
                min_level = 8;
                spell = "galvanic whip";
                break;
            case 4:
                min_level = 11;
                spell = "colour spray";
                break;
            case 5:
                min_level = 12;
                spell = "weaken";
                break;
            case 6:
                min_level = 13;
                spell = "energy drain";
                break;
            case 7:
                min_level = 14;
                spell = "spectral furor";
                break;
            case 8:
                min_level = 10;
                spell = "fireball";
                break;
            case 9:
                min_level = 15;
                spell = "fireball";
                break;
            default:
                min_level = 20;
                spell = "acid blast";
                break;
        }

        if ( ch->level >= min_level )
            break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
    return TRUE;
}

bool spec_cast_undead( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    const char             *spell;
    int                     sn;

    summon_if_hating( ch );

    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;
        if ( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( !victim || victim == ch )
        return FALSE;

    for ( ;; ) {
        int                     min_level;

        switch ( number_bits( 4 ) ) {
            case 0:
                min_level = 0;
                spell = "chill touch";
                break;
            case 1:
                min_level = 11;
                spell = "weaken";
                break;
            case 2:
                min_level = 12;
                spell = "curse";
                break;
            case 3:
                min_level = 13;
                spell = "blindness";
                break;
            case 4:
                min_level = 14;
                spell = "poison";
                break;
            case 5:
                min_level = 15;
                spell = "energy drain";
                break;
            case 6:
                min_level = 18;
                spell = "harm";
                break;
            default:
                min_level = 40;
                spell = "gate";
                break;
        }

        if ( ch->level >= min_level )
            break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
    return TRUE;
}

bool spec_executioner( CHAR_DATA *ch )
{
    char                    buf[MSL];
    MOB_INDEX_DATA         *cityguard;
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    const char             *crime;

    if ( !IS_AWAKE( ch ) || ch->fighting || ch->position != POS_STANDING )
        return FALSE;

    crime = "";
    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;

        if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_KILLER ) ) {
            crime = "KILLER";
            break;
        }

        if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_THIEF ) ) {
            crime = "THIEF";
            break;
        }
    }

    if ( !victim )
        return FALSE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        snprintf( buf, MSL, "yell %s is a %s!  As well as a COWARD!", victim->name, crime );
        interpret( ch, buf );
        return TRUE;
    }
    snprintf( buf, MSL, "yell %s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!", victim->name, crime );
    interpret( ch, buf );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    if ( char_died( ch ) )
        return TRUE;

    /*
     * Added log in case of missing cityguard -- Tri 
     */
    cityguard = get_mob_index( MOB_VNUM_CITYGUARD );

    if ( !cityguard ) {
        bug( "Missing Cityguard - Vnum:[%d]", MOB_VNUM_CITYGUARD );
        return TRUE;
    }

    char_to_room( create_mobile( cityguard ), ch->in_room );
    char_to_room( create_mobile( cityguard ), ch->in_room );
    return TRUE;
}

bool spec_fido( CHAR_DATA *ch )
{
    OBJ_DATA               *corpse;
    OBJ_DATA               *c_next;
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;

    if ( !IS_AWAKE( ch ) )
        return FALSE;

    for ( corpse = ch->in_room->first_content; corpse; corpse = c_next ) {
        c_next = corpse->next_content;
        if ( corpse->item_type != ITEM_CORPSE_NPC )
            continue;

        act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
        for ( obj = corpse->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            obj_from_obj( obj );
            obj_to_room( obj, ch->in_room );
        }
        extract_obj( corpse );
        return TRUE;
    }

    return FALSE;
}

bool spec_guard( CHAR_DATA *ch )
{
    char                    buf[MSL];
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    CHAR_DATA              *ech;
    const char             *crime;

    if ( !IS_AWAKE( ch ) || ch->fighting || ch->position == POS_SITTING )
        return FALSE;

    ech = NULL;
    crime = "";

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;

        if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_KILLER ) ) {
            crime = "KILLER";
            break;
        }

        if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_THIEF ) ) {
            crime = "THIEF";
            break;
        }

        if ( victim->fighting && who_fighting( victim ) != ch && victim->alignment < 300
             && IS_NPC( victim ) )
            ech = victim;

    }

    if ( victim && IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        snprintf( buf, MSL, "yell %s is a %s!  As well as a COWARD!", victim->name, crime );
        interpret( ch, buf );
        return TRUE;
    }

    if ( victim ) {
        snprintf( buf, MSL, "yell %s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
                  victim->name, crime );
        interpret( ch, buf );
        multi_hit( ch, victim, TYPE_UNDEFINED );
        return TRUE;
    }

    if ( ech ) {
        act( AT_YELL, "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!", ch, NULL, NULL, TO_ROOM );
        multi_hit( ch, ech, TYPE_UNDEFINED );
        return TRUE;
    }

    return FALSE;
}

bool spec_janitor( CHAR_DATA *ch )
{
    OBJ_DATA               *trash;
    OBJ_DATA               *trash_next;

    if ( !IS_AWAKE( ch ) || ch->fighting )
        return FALSE;

    for ( trash = ch->in_room->first_content; trash; trash = trash_next ) {
        trash_next = trash->next_content;
        if ( !IS_SET( trash->wear_flags, ITEM_TAKE ) || IS_OBJ_STAT( trash, ITEM_BURIED ) )
            continue;
        if ( trash->item_type == ITEM_DRINK_CON || trash->item_type == ITEM_TRASH
             || trash->cost < 500 || ( trash->pIndexData->vnum == OBJ_VNUM_SHOPPING_BAG
                                       && !trash->first_content ) ) {
            act( AT_ACTION, "$n quickly picks up some trash.", ch, NULL, NULL, TO_ROOM );
            obj_from_room( trash );
            obj_to_char( trash, ch );
            return TRUE;
        }
    }

    return FALSE;
}

bool spec_mayor( CHAR_DATA *ch )
{
    static const char       open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static const char       close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char      *path;
    static int              pos;
    static bool             move;

    if ( !move ) {
        if ( time_info.hour == 6 ) {
            path = open_path;
            move = TRUE;
            pos = 0;
        }

        if ( time_info.hour == 20 ) {
            path = close_path;
            move = TRUE;
            pos = 0;
        }
    }

    if ( ch->fighting )
        return spec_cast_cleric( ch );
    if ( !move || ch->position < POS_SLEEPING )
        return FALSE;

    switch ( path[pos] ) {
        case '0':
        case '1':
        case '2':
        case '3':
            move_char( ch, get_exit( ch->in_room, path[pos] - '0' ), 0 );
            break;

        case 'W':
            ch->position = POS_STANDING;
            act( AT_ACTION, "$n awakens and groans loudly.", ch, NULL, NULL, TO_ROOM );
            break;

        case 'S':
            ch->position = POS_SLEEPING;
            act( AT_ACTION, "$n lies down and falls asleep.", ch, NULL, NULL, TO_ROOM );
            break;

        case 'a':
            act( AT_SAY, "$n says 'Hello Honey!'", ch, NULL, NULL, TO_ROOM );
            break;

        case 'b':
            act( AT_SAY, "$n says 'What a view!  I must do something about that dump!'", ch, NULL,
                 NULL, TO_ROOM );
            break;

        case 'c':
            act( AT_SAY, "$n says 'Vandals!  Youngsters have no respect for anything!'", ch, NULL,
                 NULL, TO_ROOM );
            break;

        case 'd':
            act( AT_SAY, "$n says 'Good day, citizens!'", ch, NULL, NULL, TO_ROOM );
            break;

        case 'e':
            act( AT_SAY, "$n says 'I hereby declare the town of Darkhaven open!'", ch, NULL, NULL,
                 TO_ROOM );
            break;

        case 'E':
            act( AT_SAY, "$n says 'I hereby declare the town of Darkhaven closed!'", ch, NULL, NULL,
                 TO_ROOM );
            break;

        case 'O':
            do_unlock( ch, ( char * ) "gate" );
            do_open( ch, ( char * ) "gate" );
            break;

        case 'C':
            do_close( ch, ( char * ) "gate" );
            do_lock( ch, ( char * ) "gate" );
            break;

        case '.':
            move = FALSE;
            break;
    }

    pos++;
    return FALSE;
}

bool spec_poison( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;

    if ( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE
         && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE
         && ch->position != POS_BERSERK )
        return FALSE;

    if ( ( victim = who_fighting( ch ) ) == NULL || number_percent(  ) > 2 * ch->level )
        return FALSE;

    act( AT_HIT, "You bite $N!", ch, NULL, victim, TO_CHAR );
    act( AT_ACTION, "$n bites $N!", ch, NULL, victim, TO_NOTVICT );
    act( AT_POISON, "$n bites you!", ch, NULL, victim, TO_VICT );
    spell_poison( gsn_poison, ch->level, ch, victim );
    return TRUE;
}

bool spec_thief( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;

    if ( ch->position != POS_STANDING || ( ch->fighting && who_fighting( ch ) != victim ) )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;

        if ( IS_NPC( victim ) || victim->level >= LEVEL_IMMORTAL || number_bits( 2 ) != 0 || !can_see( ch, victim ) )   /* Thx 
                                                                                                                         * Glop 
                                                                                                                         */
            continue;

        if ( IS_AWAKE( victim ) && number_range( 0, ch->level ) == 0 ) {
            act( AT_ACTION, "You discover $n's hands in your sack of money!", ch, NULL, victim,
                 TO_VICT );
            act( AT_ACTION, "$N discovers $n's hands in $S sack of money!", ch, NULL, victim,
                 TO_NOTVICT );
            return TRUE;
        }
        else {
            int                     money,
                                    maxmoney,
                                    type = CURR_GOLD;

            for ( money = 0; money < 10; money++ ) {
                type = number_range( FIRST_CURR, LAST_CURR );
                if ( GET_MONEY( victim, type ) )
                    break;
            }
            if ( !GET_MONEY( victim, type ) )
                return FALSE;

            maxmoney = ch->level * 1000;
            money =
                GET_MONEY( victim, type ) * number_range( 1, URANGE( 2, ch->level / 4, 10 ) ) / 100;
            GET_MONEY( ch, type ) += 9 * money / 10;
            GET_MONEY( victim, type ) -= money;
            if ( GET_MONEY( ch, type ) > maxmoney ) {
                boost_economy( ch->in_room->area, GET_MONEY( ch, type ) - maxmoney / 2, type );
                GET_MONEY( ch, type ) = maxmoney / 2;
            }
            return TRUE;
        }
    }

    return FALSE;
}

//Slightly modified by Taon.
//Reminder: This really just needs to be rewrote. -Taon
bool spec_greet( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *v_next;
    int                     rnd_say;

    for ( victim = ch->in_room->first_person; victim; victim = v_next ) {
        v_next = victim->next_in_room;
        if ( IS_NPC( victim ) )
            break;
    }

    if ( victim == NULL || victim == ch )
        return FALSE;

    rnd_say = number_range( 1, 10 );

    if ( rnd_say <= 4 )
        act( AT_PLAIN, "$n nods $m head at $N.", ch, NULL, NULL, TO_ROOM );
    else if ( rnd_say <= 6 )
        act( AT_PLAIN, "$n boldy accepts the presence of $N.", ch, NULL, NULL, TO_ROOM );
    else if ( rnd_say <= 8 )
        act( AT_PLAIN, "$n greets $N.", ch, NULL, NULL, TO_ROOM );
    else if ( rnd_say <= 10 )
        act( AT_PLAIN, "$n smiles warmly at $N.", ch, NULL, NULL, TO_ROOM );
    return TRUE;
}
