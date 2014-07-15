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
 *                             Archery Module                               *
 ****************************************************************************/

/*
Bowfire Code v1.0 (c)1997-99 Feudal Realms 

This is my bow and arrow code that I wrote based off of a thrown weapon code
that I had from long ago (if you wrote it let me know so I can give you
credit for that part, I do not have it anymore), it's a little more complex 
than I had originally wanted, but well, it works.  There are a couple things 
that are involved which if you don't want to use, remove them, that simple.  
One of them are the use of the "lodged" wearbits.  The code is designed to 
lodge an arrow in a victim, not just do damage to them once, and there are three
places it can lodge, etc.  Included are all of the pieces of code for quivers,
arrows, drawing arrows, dislodging, etc.  Use whatever of this code that you
want, if you have a credits page, add me on there, and please drop me an email
at mustang@roscoe.mudservices.com so I know its out there somewhere being used.

Any bugs that people find, if you email me, I will fix, unless it's something
from a modification that you made, and if that's the case, I will probably
help you figure out what's up with it if I can.  My code is not stock, and I
tried to add in everything that people might need to add in this feature.

Thanks,
Tch

===============================================================================
Features in v1.0

- Bowfire from adjacent rooms at targets
- Arrows lodge in various body parts (leg, arm, and chest)
- Quiver and arrow new item types
- Shoulder wearbit used for quivers (if you want it)
- Dislodging arrows does damage
- OLC support for arrows and quivers

===============================================================================

Add in a bow weapon type with the other ones on the list.(if you don't know 
how to do this, grep/search for sword and add in bow in the respective places)
  
=============================================================================== 

Bowfire code ported for Smaug 1.4a by Samson.
Combined with portions of the Smaug 1.4a archery code.
Additional portions by Samson.
*/

#include "h/mud.h"

bool                    was_fired_projectile;
int                     projectile_pchance;
OBJ_DATA               *used_projectile;

int                     weapon_prof_bonus_check( CHAR_DATA *ch, OBJ_DATA *wield, int *gsn_ptr );
int                     obj_hitroll( OBJ_DATA *obj );
ch_ret                  spell_attack( int, int, CHAR_DATA *, void * );
void                    wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, short wear_bit );

void lodge_projectile( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !was_fired_projectile || !used_projectile )
        return;

    if ( used_projectile->in_obj )
        obj_from_obj( used_projectile );
    if ( used_projectile->carried_by )
        obj_from_char( used_projectile );

    was_fired_projectile = FALSE;
    if ( char_died( victim ) ) {
        extract_obj( used_projectile );
        used_projectile = NULL;
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_restring ) ) {
        WAIT_STATE( ch, skill_table[gsn_restring]->beats );
        learn_from_success( ch, gsn_restring );
        do_draw( ch, ( char * ) "" );
    }

    obj_to_char( used_projectile, victim );

    switch ( projectile_pchance ) {
        case 1:
        case 2:
        case 3:                                       /* Hit in the arm */
            if ( get_eq_char( victim, WEAR_LODGE_ARM ) ) {
                if ( used_projectile->in_obj )
                    obj_from_obj( used_projectile );
                if ( used_projectile->carried_by )
                    obj_from_char( used_projectile );
                extract_obj( used_projectile );
                used_projectile = NULL;
                break;
            }
            xSET_BIT( used_projectile->extra_flags, ITEM_LODGED );
            SET_BIT( used_projectile->wear_flags, ITEM_LODGE_ARM );
            wear_obj( victim, used_projectile, TRUE, get_wflag( ( char * ) "lodge_arm" ) );
            act( AT_ACTION, "The projectile got lodged in your arm.", ch, NULL, victim, TO_VICT );
            act( AT_ACTION, "The projectile got lodged in $s arm.", ch, NULL, victim, TO_CHAR );
            break;

        case 4:
        case 5:
        case 6:                                       /* Hit in the leg */
            if ( get_eq_char( victim, WEAR_LODGE_LEG ) ) {
                if ( used_projectile->in_obj )
                    obj_from_obj( used_projectile );
                if ( used_projectile->carried_by )
                    obj_from_char( used_projectile );
                extract_obj( used_projectile );
                used_projectile = NULL;
                break;
            }
            xSET_BIT( used_projectile->extra_flags, ITEM_LODGED );
            SET_BIT( used_projectile->wear_flags, ITEM_LODGE_LEG );
            wear_obj( victim, used_projectile, TRUE, get_wflag( ( char * ) "lodge_leg" ) );
            act( AT_ACTION, "The projectile got lodged in your leg.", ch, NULL, victim, TO_VICT );
            act( AT_ACTION, "The projectile got lodged in $s leg.", ch, NULL, victim, TO_CHAR );
            break;

        case 7:
        case 8:
        case 9:
        case 10:                                      /* Hit in the chest */
            if ( get_eq_char( victim, WEAR_LODGE_RIB ) ) {
                if ( used_projectile->in_obj )
                    obj_from_obj( used_projectile );
                if ( used_projectile->carried_by )
                    obj_from_char( used_projectile );
                extract_obj( used_projectile );
                used_projectile = NULL;
                break;
            }
            xSET_BIT( used_projectile->extra_flags, ITEM_LODGED );
            SET_BIT( used_projectile->wear_flags, ITEM_LODGE_RIB );
            wear_obj( victim, used_projectile, TRUE, get_wflag( ( char * ) "lodge_rib" ) );
            act( AT_ACTION, "The projectile got lodged in your rib.", ch, NULL, victim, TO_VICT );
            act( AT_ACTION, "The projectile got lodged in $s rib.", ch, NULL, victim, TO_CHAR );
            break;
    }
}

OBJ_DATA               *find_quiver( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;

    for ( obj = ch->last_carrying; obj; obj = obj->prev_content ) {
        if ( can_see_obj( ch, obj ) ) {
            if ( obj->item_type == ITEM_QUIVER && !IS_SET( obj->value[1], CONT_CLOSED ) )
                return obj;
        }
    }
    return NULL;
}

OBJ_DATA               *find_projectile( CHAR_DATA *ch, OBJ_DATA *quiver )
{
    OBJ_DATA               *obj;

    for ( obj = quiver->last_content; obj; obj = obj->prev_content ) {
        if ( can_see_obj( ch, obj ) ) {
            if ( obj->item_type == ITEM_PROJECTILE )
                return obj;
        }
    }
    return NULL;
}

/* Bowfire code -- used to draw an arrow from a quiver */
void do_draw( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *bow;
    OBJ_DATA               *arrow;
    OBJ_DATA               *quiver;
    int                     hand_count = 0;

    if ( ( bow = get_eq_char( ch, WEAR_MISSILE_WIELD ) ) == NULL ) {
        send_to_char( "You are not wielding a missile weapon!\r\n", ch );
        return;
    }

    if ( ( quiver = find_quiver( ch ) ) == NULL ) {
        send_to_char( "You aren't wearing a quiver where you can get to it!\r\n", ch );
        return;
    }

    if ( get_eq_char( ch, WEAR_LIGHT ) != NULL )
        hand_count++;
    if ( get_eq_char( ch, WEAR_SHIELD ) != NULL )
        hand_count++;
    if ( get_eq_char( ch, WEAR_HOLD ) != NULL )
        hand_count++;
    if ( get_eq_char( ch, WEAR_WIELD ) != NULL )
        hand_count++;
    if ( hand_count > 1 ) {
        send_to_char( "You need a free hand to draw with.\r\n", ch );
        return;
    }

    if ( get_eq_char( ch, WEAR_HOLD ) != NULL ) {
        send_to_char( "Your hand is not empty!\r\n", ch );
        return;
    }

    if ( ( arrow = find_projectile( ch, quiver ) ) == NULL ) {
        send_to_char( "Your quiver is empty!!\r\n", ch );
        return;
    }

    /*
     * Forgot about this little important bit 
     */
    separate_obj( arrow );

    /*
     * OOPS - had these backwards, this is now correct :) 
     */
    if ( bow->value[6] != arrow->value[4] ) {
        send_to_char( "You drew the wrong projectile type for this weapon!\r\n", ch );
        obj_from_obj( arrow );
        obj_to_char( arrow, ch );
        return;
    }

    WAIT_STATE( ch, PULSE_VIOLENCE );
    act_printf( AT_ACTION, ch, quiver, NULL, TO_ROOM, "$n draws %s from $p.", arrow->short_descr );
    act_printf( AT_ACTION, ch, quiver, NULL, TO_CHAR, "You draw %s from $p.", arrow->short_descr );

    obj_from_obj( arrow );
    arrow = obj_to_char( arrow, ch );
    separate_obj( arrow );
    equip_char( ch, arrow, WEAR_HOLD );
}

/* Bowfire code -- Used to dislodge an arrow already lodged */
void do_dislodge( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *arrow = NULL;
    int                     dam = 0;

    if ( argument[0] == '\0' ) {                       /* empty */
        send_to_char( "Dislodge what?\r\n", ch );
        return;
    }

    if ( get_eq_char( ch, WEAR_LODGE_RIB ) != NULL ) {
        arrow = get_eq_char( ch, WEAR_LODGE_RIB );
        act( AT_CARNAGE, "With a wrenching pull, you dislodge $p from your chest.", ch, arrow, NULL,
             TO_CHAR );
        act( AT_CARNAGE, "$n winces in pain as $e dislodges $p from $s chest.", ch, arrow, NULL,
             TO_ROOM );
        unequip_char( ch, arrow );
        REMOVE_BIT( arrow->wear_flags, ITEM_LODGE_RIB );
        xREMOVE_BIT( arrow->extra_flags, ITEM_LODGED );
        dam = number_range( ( 3 * arrow->value[1] ), ( 3 * arrow->value[2] ) );
        damage( ch, ch, dam, TYPE_UNDEFINED );
        return;
    }
    else if ( get_eq_char( ch, WEAR_LODGE_ARM ) != NULL ) {
        arrow = get_eq_char( ch, WEAR_LODGE_ARM );
        act( AT_CARNAGE, "With a tug you dislodge $p from your arm.", ch, arrow, NULL, TO_CHAR );
        act( AT_CARNAGE, "$n winces in pain as $e dislodges $p from $s arm.", ch, arrow, NULL,
             TO_ROOM );
        unequip_char( ch, arrow );
        REMOVE_BIT( arrow->wear_flags, ITEM_LODGE_ARM );
        xREMOVE_BIT( arrow->extra_flags, ITEM_LODGED );
        dam = number_range( ( 3 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
        damage( ch, ch, dam, TYPE_UNDEFINED );
        return;
    }
    else if ( get_eq_char( ch, WEAR_LODGE_LEG ) != NULL ) {
        arrow = get_eq_char( ch, WEAR_LODGE_LEG );
        act( AT_CARNAGE, "With a tug you dislodge $p from your leg.", ch, arrow, NULL, TO_CHAR );
        act( AT_CARNAGE, "$n winces in pain as $e dislodges $p from $s leg.", ch, arrow, NULL,
             TO_ROOM );
        unequip_char( ch, arrow );
        REMOVE_BIT( arrow->wear_flags, ITEM_LODGE_LEG );
        xREMOVE_BIT( arrow->extra_flags, ITEM_LODGED );
        dam = number_range( ( 2 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
        damage( ch, ch, dam, TYPE_UNDEFINED );
        return;
    }
    else {
        send_to_char( "You have nothing lodged in your body.\r\n", ch );
        return;
    }
}

/*
 * Hit one guy with a projectile.
 * Handles use of missile weapons (wield = missile weapon)
 * or thrown items/weapons
 */
ch_ret projectile_hit( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield, OBJ_DATA *projectile,
                       short dist )
{
    int                     victim_ac;
    int                     thac0;
    int                     thac0_00;
    int                     thac0_32;
    int                     plusris;
    int                     dam = 0;
    int                     diceroll;
    int                     prof_bonus;
    int                     prof_gsn = -1;
    int                     proj_bonus;
    int                     dt;
    ch_ret                  retcode;

    if ( !projectile )
        return rNONE;

    if ( projectile->item_type == ITEM_PROJECTILE || projectile->item_type == ITEM_WEAPON ) {
        dt = TYPE_HIT + projectile->value[3];
        if ( wield )
            proj_bonus = number_range( wield->value[1], wield->value[2] );
        else
            proj_bonus = 0;
    }
    else {
        dt = TYPE_UNDEFINED;
        proj_bonus = 0;
    }

    /*
     * Can't beat a dead char!
     */
    if ( victim->position == POS_DEAD || char_died( victim ) ) {
        if ( projectile->in_obj )
            obj_from_obj( projectile );
        if ( projectile->carried_by )
            obj_from_char( projectile );
        extract_obj( projectile );
        return rVICT_DIED;
    }

    if ( wield )
        prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
    else
        prof_bonus = 0;

    if ( dt == TYPE_UNDEFINED ) {
        dt = TYPE_HIT;
        if ( wield && wield->item_type == ITEM_MISSILE_WEAPON )
            dt += wield->value[1];
    }

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC( ch ) ) {
        thac0_00 = ch->mobthac0;
        thac0_32 = 0;
    }
    else {
        thac0_00 = class_table[ch->Class]->thac0_00;
        thac0_32 = class_table[ch->Class]->thac0_32;
    }
    thac0 = interpolate( ch->level, thac0_00, thac0_32 ) - GET_HITROLL( ch ) + ( dist * 2 );

    victim_ac = UMAX( -19, ( int ) ( GET_AC( victim ) / 10 ) );

    /*
     * if you can't see what's coming... 
     */
    if ( !can_see_obj( victim, projectile ) )
        victim_ac += 1;
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;

    /*
     * Weapon proficiency bonus 
     */
    victim_ac += prof_bonus;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 );

    if ( diceroll == 0 || ( diceroll != 19 && diceroll < thac0 - victim_ac ) ) {
        /*
         * Miss. 
         */
        if ( prof_gsn != -1 )
            learn_from_failure( ch, prof_gsn );

        if ( projectile->in_obj )
            obj_from_obj( projectile );
        if ( projectile->carried_by )
            obj_from_char( projectile );

        /*
         * Do something with the projectile 
         */
        if ( number_percent(  ) < 50 )
            extract_obj( projectile );
        else
            obj_to_room( projectile, victim->in_room );

        damage( ch, victim, 0, dt );
        tail_chain(  );
        return rNONE;
    }

    /*
     * Hit.
     * Calc damage.
     */

    projectile_pchance = number_range( 1, 10 );

    switch ( projectile_pchance ) {
        case 1:
        case 2:
        case 3:                                       /* Hit in the arm */
            dam = number_range( projectile->value[1], projectile->value[2] ) + proj_bonus;
            break;
        case 4:
        case 5:
        case 6:                                       /* Hit in the leg */
            dam =
                number_range( ( 2 * projectile->value[1] ),
                              ( 2 * projectile->value[2] ) ) + proj_bonus;
            break;
        case 7:
        case 8:
        case 9:
        case 10:                                      /* Hit in the chest */
            dam =
                number_range( ( 3 * projectile->value[1] ),
                              ( 3 * projectile->value[2] ) ) + proj_bonus;
            break;
    }

    /*
     * Bonuses.
     */
    dam += GET_DAMROLL( ch );

    if ( prof_bonus )
        dam += prof_bonus / 4;

    /*
     * Calculate Damage Modifiers from Victim's Fighting Style
     */

    if ( victim->position == POS_BERSERK )
        dam = ( int ) ( 1.2 * dam );
    else if ( victim->position == POS_AGGRESSIVE )
        dam = ( int ) ( 1.1 * dam );
    else if ( victim->position == POS_DEFENSIVE )
        dam = ( int ) ( .85 * dam );
    else if ( victim->position == POS_EVASIVE )
        dam = ( int ) ( .8 * dam );

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_enhanced_damage] > 0
         && number_percent(  ) < ch->pcdata->learned[gsn_enhanced_damage] ) {
        dam += ( int ) ( dam * LEARNED( ch, gsn_enhanced_damage ) / 120 );
    }
    else
        learn_from_failure( ch, gsn_enhanced_damage );

    if ( !IS_AWAKE( victim ) )
        dam = ( int ) ( 2 * dam );

    if ( dam <= 0 )
        dam = 1;

    plusris = 0;

    if ( IS_OBJ_STAT( projectile, ITEM_MAGIC ) )
        dam = ris_damage( victim, dam, RIS_MAGIC );
    else
        dam = ris_damage( victim, dam, RIS_NONMAGIC );

    /*
     * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll  -Thoric
     */
    if ( wield )
        plusris = obj_hitroll( wield );

    /*
     * check for RIS_PLUSx      -Thoric 
     */
    if ( dam ) {
        int                     x,
                                res,
                                imm,
                                sus,
                                mod;

        if ( plusris )
            plusris = RIS_PLUS1 << UMIN( plusris, 7 );

        /*
         * initialize values to handle a zero plusris 
         */
        imm = res = -1;
        sus = 1;

        /*
         * find high ris 
         *//*
         * FIND ME 
         */
        for ( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 ) {
            if ( IS_SET( victim->immune, x ) )
                imm = x;
            if ( IS_SET( victim->resistant, x ) )
                res = x;
            if ( IS_SET( victim->susceptible, x ) )
                sus = x;
        }
        mod = 10;
        if ( imm >= plusris )
            mod -= 10;
        if ( res >= plusris )
            mod -= 2;
        if ( sus <= plusris )
            mod += 2;

        /*
         * check if immune 
         */
        if ( mod <= 0 )
            dam = -1;
        if ( mod != 10 )
            dam = ( dam * mod ) / 10;
    }

    /*
     * immune to damage 
     */
    if ( dam == -1 ) {
        if ( dt >= 0 && dt < top_sn ) {
            SKILLTYPE              *skill = skill_table[dt];
            bool                    found = FALSE;

            if ( skill->imm_char && skill->imm_char[0] != '\0' ) {
                act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
                found = TRUE;
            }
            if ( skill->imm_vict && skill->imm_vict[0] != '\0' ) {
                act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
                found = TRUE;
            }
            if ( skill->imm_room && skill->imm_room[0] != '\0' ) {
                act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
                found = TRUE;
            }
            if ( found ) {
                if ( projectile->in_obj )
                    obj_from_obj( projectile );
                if ( projectile->carried_by )
                    obj_from_char( projectile );

                if ( number_percent(  ) < 50 )
                    extract_obj( projectile );
                else
                    obj_to_room( projectile, victim->in_room );
                return rNONE;
            }
        }
        dam = 0;
    }

    was_fired_projectile = TRUE;
    used_projectile = projectile;
    retcode = damage( ch, victim, dam, dt );

    if ( retcode != rNONE ) {
        if ( used_projectile && !xIS_SET( used_projectile->extra_flags, ITEM_LODGED ) ) {
            if ( used_projectile->in_obj )
                obj_from_obj( used_projectile );
            if ( used_projectile->carried_by )
                obj_from_char( used_projectile );
            extract_obj( used_projectile );
            used_projectile = NULL;
        }
        return retcode;
    }

    if ( char_died( ch ) ) {
        if ( used_projectile && !xIS_SET( used_projectile->extra_flags, ITEM_LODGED ) ) {
            if ( used_projectile->in_obj )
                obj_from_obj( used_projectile );
            if ( used_projectile->carried_by )
                obj_from_char( used_projectile );
            extract_obj( used_projectile );
            used_projectile = NULL;
        }
        return rCHAR_DIED;
    }
    if ( char_died( victim ) ) {
        if ( used_projectile ) {
            if ( used_projectile->in_obj )
                obj_from_obj( used_projectile );
            if ( used_projectile->carried_by )
                obj_from_char( used_projectile );
            extract_obj( used_projectile );
            used_projectile = NULL;
        }
        return rVICT_DIED;
    }

    retcode = rNONE;
    if ( dam == 0 ) {
        if ( used_projectile ) {
            if ( used_projectile->in_obj )
                obj_from_obj( used_projectile );
            if ( used_projectile->carried_by )
                obj_from_char( used_projectile );
            if ( number_percent(  ) < 50 )
                extract_obj( used_projectile );
            else
                obj_to_room( used_projectile, victim->in_room );
            used_projectile = NULL;
        }
        return retcode;
    }

    /*
     * weapon spells -Thoric 
     */
    if ( wield && !IS_SET( victim->immune, RIS_MAGIC )
         && !IS_SET( victim->in_room->room_flags, ROOM_NO_MAGIC ) ) {
        AFFECT_DATA            *aff;

        for ( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
            if ( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier )
                 && skill_table[aff->modifier]->spell_fun )
                retcode =
                    ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, 7, ch, victim );
        if ( retcode != rNONE || char_died( ch ) || char_died( victim ) ) {
            if ( used_projectile ) {
                if ( used_projectile->in_obj )
                    obj_from_obj( used_projectile );
                if ( used_projectile->carried_by )
                    obj_from_char( used_projectile );
                extract_obj( used_projectile );
                used_projectile = NULL;
            }
            return retcode;
        }
        for ( aff = wield->first_affect; aff; aff = aff->next )
            if ( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier )
                 && skill_table[aff->modifier]->spell_fun )
                retcode =
                    ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier, 7, ch, victim );
        if ( retcode != rNONE || char_died( ch ) || char_died( victim ) ) {
            if ( used_projectile ) {
                if ( used_projectile->in_obj )
                    obj_from_obj( used_projectile );
                if ( used_projectile->carried_by )
                    obj_from_char( used_projectile );
                extract_obj( used_projectile );
                used_projectile = NULL;
            }
            return retcode;
        }
    }

    if ( used_projectile && !xIS_SET( used_projectile->extra_flags, ITEM_LODGED ) ) {
        if ( used_projectile->in_obj )
            obj_from_obj( used_projectile );
        if ( used_projectile->carried_by )
            obj_from_char( used_projectile );
        extract_obj( used_projectile );
        used_projectile = NULL;
    }

    tail_chain(  );
    return retcode;
}

/*
 * Perform the actual attack on a victim   -Thoric
 */
ch_ret ranged_got_target( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *weapon, OBJ_DATA *projectile,
                          short dist, short dt, char *stxt, short color )
{
    /*
     * added wtype for check to determine skill used for ranged attacks - Grimm 
     */
    short                   wtype = 0;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        /*
         * safe room, bubye projectile 
         */
        if ( projectile ) {
            ch_printf( ch, "Your %s is blasted from existance by a godly presense.",
                       myobj( projectile ) );
            act( color, "A godly presence smites $p!", ch, projectile, NULL, TO_ROOM );
            if ( projectile->in_obj )
                obj_from_obj( projectile );
            if ( projectile->carried_by )
                obj_from_char( projectile );
            extract_obj( projectile );
        }
        else {
            ch_printf( ch, "Your %s is blasted from existance by a godly presense.", stxt );
            act( color, "A godly presence smites $t!", ch, aoran( stxt ), NULL, TO_ROOM );
        }
        return rNONE;
    }

    /*
     * check dam type of projectile to determine value of wtype 
     * * wtype points to same "short" as the skill assigned to that
     * * range by the code and as such the proper skill will be used. 
     * * Grimm 
     */
    switch ( projectile->value[4] ) {
        default:
        case 1:
            wtype = gsn_archery;
            break;
        case 2:
            wtype = gsn_blowguns;
            break;
    }

    // make sure archery code only does one attack per shot?
    if ( ch->success_attack > 1 )
        ch->success_attack = 1;

    short                   hit;

    hit = number_range( 1, 4 );
    if ( ch->success_attack < 1 && hit > 1 )
        ch->success_attack = 1;

    if ( ch->success_attack == 1 ) {
        start_hating( victim, ch );
        start_hunting( victim, ch );
    }

    if ( number_percent(  ) > 25
         || ( projectile && weapon && can_use_skill( ch, number_percent(  ), wtype ) ) ) {
        if ( IS_NPC( victim ) )                        /* This way the poor sap can hunt
                                                        * its attacker */
            xREMOVE_BIT( victim->act, ACT_SENTINEL );
        if ( projectile )
            global_retcode = projectile_hit( ch, victim, weapon, projectile, dist );
        else
            global_retcode = spell_attack( dt, ch->level, ch, victim );
    }
    else {
        switch ( projectile->value[4] ) {
            default:
            case 1:
                learn_from_failure( ch, gsn_archery );
                break;
            case 2:
                learn_from_failure( ch, gsn_blowguns );
                break;
        }
        global_retcode = damage( ch, victim, 0, dt );

        if ( projectile ) {
            if ( projectile->in_obj )
                obj_from_obj( projectile );
            if ( projectile->carried_by )
                obj_from_char( projectile );
            /*
             * 50% chance of getting lost 
             */
            if ( number_percent(  ) < 50 )
                extract_obj( projectile );
            else {
                obj_to_room( projectile, victim->in_room );
            }
        }
    }
    return global_retcode;
}

/*
 * Basically the same guts as do_scan() from above (please keep them in
 * sync) used to find the victim we're firing at.  -Thoric
 */
CHAR_DATA              *scan_for_victim( CHAR_DATA *ch, EXIT_DATA *pexit, char *name )
{
    CHAR_DATA              *victim;
    ROOM_INDEX_DATA        *was_in_room;
    short                   dist,
                            dir;
    short                   max_dist = 8;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) || !pexit )
        return NULL;

    was_in_room = ch->in_room;

    if ( ch->level < 50 )
        --max_dist;
    if ( ch->level < 40 )
        --max_dist;
    if ( ch->level < 30 )
        --max_dist;

    for ( dist = 1; dist <= max_dist; ) {
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) )
            break;

        if ( room_is_private( pexit->to_room ) && ch->level < sysdata.level_override_private )
            break;

        char_from_room( ch );
        char_to_room( ch, pexit->to_room );

        if ( ( victim = get_char_room( ch, name ) ) != NULL ) {
            char_from_room( ch );
            char_to_room( ch, was_in_room );
            return victim;
        }

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
            case SECT_DESERT:
            case SECT_HILLS:
                dist += 2;
                break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
                dist += 3;
                break;
            case SECT_MOUNTAIN:
            case SECT_UNDERWATER:
            case SECT_OCEANFLOOR:
                dist += 4;
                break;
        }

        if ( dist >= max_dist )
            break;

        dir = pexit->vdir;
        if ( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
            break;
    }

    char_from_room( ch );
    char_to_room( ch, was_in_room );

    return NULL;
}

/*
 * Generic use ranged attack function   -Thoric & Tricops
 */
ch_ret ranged_attack( CHAR_DATA *ch, char *argument, OBJ_DATA *weapon, OBJ_DATA *projectile,
                      short dt, short range )
{
    CHAR_DATA              *victim,
                           *vch;
    EXIT_DATA              *pexit;
    ROOM_INDEX_DATA        *was_in_room;
    char                    arg[MIL];
    char                    arg1[MIL];
    char                    temp[MIL];
    char                    buf[MSL];
    SKILLTYPE              *skill = NULL;
    short                   dir = -1,
        dist = 0,
        color = AT_GREY;
    const char             *dtxt = "somewhere";
    const char             *stxt = "burst of energy";
    int                     count;

    if ( argument && argument[0] != '\0' && argument[0] == '\'' ) {
        one_argument( argument, temp );
        argument = temp;
    }

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );

    if ( arg[0] == '\0' ) {
        send_to_char( "Where?  At who?\r\n", ch );
        return rNONE;
    }

    victim = NULL;

    /*
     * get an exit or a victim 
     */
    if ( ( pexit = find_door( ch, arg, TRUE ) ) == NULL ) {
        if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
            send_to_char( "Aim in what direction?\r\n", ch );
            return rNONE;
        }
        else {
            if ( who_fighting( ch ) == victim ) {
                send_to_char( "They are too close to release that type of attack!\r\n", ch );
                return rNONE;
            }
        }
    }
    else
        dir = pexit->vdir;

    /*
     * check for ranged attacks from private rooms, etc 
     */
    if ( !victim ) {
        if ( IS_SET( ch->in_room->room_flags, ROOM_PRIVATE )
             || IS_SET( ch->in_room->room_flags, ROOM_SOLITARY ) ) {
            send_to_char( "You cannot perform a ranged attack from a private room.\r\n", ch );
            return rNONE;
        }
        if ( ch->in_room->tunnel > 0 ) {
            count = 0;
            for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
                ++count;
            if ( count >= ch->in_room->tunnel ) {
                send_to_char( "This room is too cramped to perform such an attack.\r\n", ch );
                return rNONE;
            }
        }
    }

    if ( IS_VALID_SN( dt ) )
        skill = skill_table[dt];

    if ( pexit && !pexit->to_room ) {
        send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
        return rNONE;
    }

    /*
     * Check for obstruction 
     */
    if ( pexit && IS_SET( pexit->exit_info, EX_CLOSED ) ) {
        if ( IS_SET( pexit->exit_info, EX_SECRET ) || IS_SET( pexit->exit_info, EX_DIG ) )
            send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
        else
            send_to_char( "Are you expecting to fire through a door!?\r\n", ch );
        return rNONE;
    }

#ifdef EXTENDEDWALLCODE
    /*
     * Keeps em from firing through a wall but can still fire through an 
     * arrow slit or window, Marcus 
     */
    if ( pexit ) {
        if ( ( IS_SET( pexit->exit_flags, EX_FORTIFIED )
               || IS_SET( pexit->exit_flags, EX_HEAVY )
               || IS_SET( pexit->exit_flags, EX_MEDIUM )
               || IS_SET( pexit->exit_flags, EX_LIGHT )
               || IS_SET( pexit->exit_flags, EX_CRUMBLING ) )
             && !IS_SET( pexit->exit_flags, EX_WINDOW )
             && !IS_SET( pexit->exit_flags, EX_ASLIT ) ) {
            send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
            return rNONE;
        }
    }
#endif

    vch = NULL;
    if ( pexit && arg1[0] != '\0' ) {
        if ( ( vch = scan_for_victim( ch, pexit, arg1 ) ) == NULL ) {
            send_to_char( "You cannot see your target.\r\n", ch );
            return rNONE;
        }

        /*
         * don't allow attacks on mobs that are in a no-missile room --Shaddai 
         */

        if ( IS_SET( vch->in_room->room_flags, ROOM_NOMISSILE ) ) {
            send_to_char( "You can't get a clean shot off.\r\n", ch );
            return rNONE;
        }

        /*
         * can't properly target someone heavily in battle 
         */
        if ( vch->num_fighting > max_fight( vch ) ) {
            send_to_char( "There is too much activity there for you to get a clear shot.\r\n", ch );
            return rNONE;
        }
    }
    if ( vch ) {
        if ( !IS_NPC( vch ) && !IS_NPC( ch ) && xIS_SET( ch->act, PLR_NICE ) ) {
            send_to_char( "Your too nice to do that!\r\n", ch );
            return rNONE;
        }
        if ( vch && is_safe( ch, vch, TRUE ) )
            return rNONE;
    }
    was_in_room = ch->in_room;

    if ( projectile ) {
        separate_obj( projectile );
        if ( pexit ) {
            if ( weapon ) {
                act( AT_GREEN, "&GYou fire $p $T.", ch, projectile, dir_name[dir], TO_CHAR );
                act( AT_GREEN, "&G$n fires $p $T.", ch, projectile, dir_name[dir], TO_ROOM );
            }
            else {
                act( AT_GREEN, "&GYou throw $p $T.", ch, projectile, dir_name[dir], TO_CHAR );
                act( AT_GREEN, "&G$n throw $p $T.", ch, projectile, dir_name[dir], TO_ROOM );
            }
        }
        else {
            if ( weapon ) {
                act( AT_GREEN, "&GYou fire $p at $N.", ch, projectile, victim, TO_CHAR );
                act( AT_GREEN, "&G$n fires $p at $N.", ch, projectile, victim, TO_NOTVICT );
                act( AT_GREEN, "&G$n fires $p at you!", ch, projectile, victim, TO_VICT );
            }
            else {
                act( AT_GREEN, "&GYou throw $p at $N.", ch, projectile, victim, TO_CHAR );
                act( AT_GREEN, "&G$n throws $p at $N.", ch, projectile, victim, TO_NOTVICT );
                act( AT_GREEN, "&G$n throws $p at you!", ch, projectile, victim, TO_VICT );
            }
        }
    }
    else if ( skill ) {
        if ( skill->noun_damage && skill->noun_damage[0] != '\0' )
            stxt = skill->noun_damage;
        else
            stxt = skill->name;
        /*
         * a plain "spell" flying around seems boring 
         */
        if ( !str_cmp( stxt, "spell" ) )
            stxt = "magical burst of energy";
        if ( skill->type == SKILL_SPELL ) {
            color = AT_MAGIC;
            if ( pexit ) {
                act( AT_MAGIC, "You release $t $T.", ch, aoran( stxt ), dir_name[dir], TO_CHAR );
                act( AT_MAGIC, "$n releases $s $t $T.", ch, stxt, dir_name[dir], TO_ROOM );
            }
            else {
                act( AT_MAGIC, "You release $t at $N.", ch, aoran( stxt ), victim, TO_CHAR );
                act( AT_MAGIC, "$n releases $s $t at $N.", ch, stxt, victim, TO_NOTVICT );
                act( AT_MAGIC, "$n releases $s $t at you!", ch, stxt, victim, TO_VICT );
            }
        }
    }
    else {
        bug( "Ranged_attack: no projectile, no skill dt %d", dt );
        return rNONE;
    }

    /*
     * victim in same room 
     */
    if ( victim ) {
        check_illegal_pk( ch, victim );
        check_attacker( ch, victim );
        return ranged_got_target( ch, victim, weapon, projectile, 0, dt, ( char * ) stxt, color );
    }

    /*
     * assign scanned victim 
     */
    victim = vch;

    /*
     * reverse direction text from move_char 
     */
    dtxt = rev_exit( pexit->vdir );

    while ( dist <= range ) {
        char_from_room( ch );
        char_to_room( ch, pexit->to_room );

        if ( IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            /*
             * whadoyahknow, the door's closed 
             */
            if ( projectile ) {
                act_printf( color, ch, NULL, NULL, TO_CHAR,
                            "You see your %s pierce a door in the distance to the %s.",
                            myobj( projectile ), dir_name[dir] );
                act_printf( color, ch, projectile, NULL, TO_ROOM,
                            "$p flies in from %s and implants itself solidly in the %sern door.",
                            dtxt, dir_name[dir] );
                if ( projectile->in_obj )
                    obj_from_obj( projectile );
                if ( projectile->carried_by )
                    obj_from_char( projectile );
                extract_obj( projectile );
                if ( can_use_skill( ch, number_percent(  ), gsn_restring ) ) {
                    WAIT_STATE( ch, skill_table[gsn_restring]->beats );
                    do_draw( ch, ( char * ) "" );
                }
            }
            else {
                act_printf( color, ch, NULL, NULL, TO_CHAR,
                            "You see your %s hit a door in the distance to the %s.", stxt,
                            dir_name[dir] );
                snprintf( buf, MSL,
                          "%s flies in from %s and implants itself solidly in the %sern door.",
                          aoran( stxt ), dtxt, dir_name[dir] );
                buf[0] = UPPER( buf[0] );
                act( color, buf, ch, NULL, NULL, TO_ROOM );
                if ( can_use_skill( ch, number_percent(  ), gsn_restring ) ) {
                    WAIT_STATE( ch, skill_table[gsn_restring]->beats );
                    do_draw( ch, ( char * ) "" );
                }
            }
            break;
        }

        /*
         * no victim? pick a random one 
         */
        if ( !victim ) {
            for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room ) {
                if ( ( ( IS_NPC( ch ) && !IS_NPC( vch ) ) || ( !IS_NPC( ch ) && IS_NPC( vch ) ) )
                     && number_bits( 1 ) == 0 ) {
                    victim = vch;
                    break;
                }
            }
            if ( victim && is_safe( ch, victim, FALSE ) ) {
                char_from_room( ch );
                char_to_room( ch, was_in_room );
                return rNONE;
            }
        }

        /*
         * In the same room as our victim? 
         */
        if ( victim && ch->in_room == victim->in_room ) {
            if ( projectile )
                act( color, "$p flies in from $T.", ch, projectile, dtxt, TO_ROOM );
            else
                act( color, "$t flies in from $T.", ch, aoran( stxt ), dtxt, TO_ROOM );

            /*
             * get back before the action starts 
             */
            char_from_room( ch );
            char_to_room( ch, was_in_room );

            check_illegal_pk( ch, victim );
            check_attacker( ch, victim );
            return ranged_got_target( ch, victim, weapon, projectile, dist, dt, ( char * ) stxt,
                                      color );
        }

        if ( dist == range ) {
            if ( projectile ) {
                act( color, "Your $t falls harmlessly to the ground to the $T.", ch,
                     myobj( projectile ), ( char * ) dir_name[dir], TO_CHAR );
                act( color, "$p flies in from $T and falls harmlessly to the ground here.", ch,
                     projectile, ( char * ) dtxt, TO_ROOM );
                if ( projectile->in_obj )
                    obj_from_obj( projectile );
                if ( projectile->carried_by )
                    obj_from_char( projectile );
                obj_to_room( projectile, ch->in_room );
            }
            else {
                act( color, "Your $t fizzles out harmlessly to the $T.", ch, ( char * ) stxt,
                     ( char * ) dir_name[dir], TO_CHAR );
                act( color, "$t flies in from $T and fizzles out harmlessly.", ch,
                     aoran( ( char * ) stxt ), ( char * ) dtxt, TO_ROOM );
            }
            break;
        }

        if ( ( pexit = get_exit( ch->in_room, dir ) ) == NULL ) {
            if ( projectile ) {
                act( color, "Your $t hits a wall and bounces harmlessly to the ground to the $T.",
                     ch, myobj( projectile ), ( char * ) dir_name[dir], TO_CHAR );
                act( color, "$p strikes the $Tern wall and falls harmlessly to the ground.", ch,
                     projectile, ( char * ) dir_name[dir], TO_ROOM );
                if ( projectile->in_obj )
                    obj_from_obj( projectile );
                if ( projectile->carried_by )
                    obj_from_char( projectile );
                obj_to_room( projectile, ch->in_room );
                if ( can_use_skill( ch, number_percent(  ), gsn_restring ) ) {
                    WAIT_STATE( ch, skill_table[gsn_restring]->beats );
                    do_draw( ch, ( char * ) "" );
                }
            }
            else {
                act( color, "Your $t harmlessly hits a wall to the $T.", ch, ( char * ) stxt,
                     ( char * ) dir_name[dir], TO_CHAR );
                act( color, "$t strikes the $Tern wall and falls harmlessly to the ground.", ch,
                     aoran( ( char * ) stxt ), ( char * ) dir_name[dir], TO_ROOM );
                if ( can_use_skill( ch, number_percent(  ), gsn_restring ) ) {
                    WAIT_STATE( ch, skill_table[gsn_restring]->beats );
                    do_draw( ch, ( char * ) "" );
                }
            }
            break;
        }
        if ( projectile )
            act( color, "$p flies in from $T.", ch, projectile, dtxt, TO_ROOM );
        else
            act( color, "$t flies in from $T.", ch, aoran( stxt ), dtxt, TO_ROOM );
        dist++;
    }

    char_from_room( ch );
    char_to_room( ch, was_in_room );

    return rNONE;
}

/* Bowfire code -- actual firing function */
void do_fire( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim = NULL;
    OBJ_DATA               *arrow;
    OBJ_DATA               *bow;
    short                   max_dist;

    if ( ( bow = get_eq_char( ch, WEAR_MISSILE_WIELD ) ) == NULL ) {
        send_to_char( "But you are not wielding a missile weapon!!\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL ) {
        send_to_char( "Fire at whom or what?\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "none" ) || !str_cmp( arg, "self" ) || victim == ch ) {
        send_to_char( "How exactly did you plan on firing at yourself?\r\n", ch );
        return;
    }

    if ( ( arrow = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "You are not holding a projectile!\r\n", ch );
        return;
    }

    if ( arrow->item_type != ITEM_PROJECTILE ) {
        send_to_char( "You are not holding a projectile!\r\n", ch );
        return;
    }

    /*
     * modify maximum distance based on bow-type and ch's class/str/etc 
     */
    max_dist = URANGE( 1, bow->value[3], 10 );

    if ( bow->value[6] != arrow->value[4] ) {
        const char             *msg = "You have nothing to fire...\r\n";

        switch ( bow->value[6] ) {
            case PROJ_BOLT:
                msg = "You have no bolts...\r\n";
                break;
            case PROJ_ARROW:
                msg = "You have no arrows...\r\n";
                break;
            case PROJ_DART:
                msg = "You have no darts...\r\n";
                break;
            case PROJ_STONE:
                msg = "You have no slingstones...\r\n";
                break;
        }
        send_to_char( msg, ch );
        return;
    }

    /*
     * Add wait state to fire for pkill, etc... 
     */
    WAIT_STATE( ch, 6 );

    short                   chance;

    chance = number_range( 1, 10 );

    if ( chance > 8 ) {
        switch ( bow->value[6] ) {
            case PROJ_BOLT:
                learn_from_success( ch, gsn_archery );
                break;
            case PROJ_ARROW:
                learn_from_success( ch, gsn_archery );
                break;
            case PROJ_DART:
                learn_from_success( ch, gsn_blowguns );
                break;
            case PROJ_STONE:
                learn_from_success( ch, gsn_archery );
                break;
        }
    }

    /*
     * handle the ranged attack 
     */
    ranged_attack( ch, argument, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );
}

/*
 * Attempt to fire at a victim.
 * Returns FALSE if no attempt was made
 */
bool mob_fire( CHAR_DATA *ch, char *name )
{
    OBJ_DATA               *arrow;
    OBJ_DATA               *bow;
    short                   max_dist;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
        return FALSE;

    if ( ( bow = get_eq_char( ch, WEAR_MISSILE_WIELD ) ) == NULL )
        return FALSE;

    if ( ( arrow = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
        return FALSE;

    if ( arrow->item_type != ITEM_PROJECTILE )
        return FALSE;

    if ( bow->value[3] != arrow->value[6] )
        return FALSE;

    /*
     * modify maximum distance based on bow-type and ch's class/str/etc 
     */
    max_dist = URANGE( 1, bow->value[3], 10 );
    ranged_attack( ch, name, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );

    return TRUE;
}
