/* Figured it would be nice to put all dragon skills in one file, easier for me to balance the power anyways.
   - Vladaar
:::::::-.  :::::::..    :::.      .,-:::::/      ...   :::.    :::. .::::::. 
 ;;,   `';,;;;;``;;;;   ;;`;;   ,;;-'````'    .;;;;;;;.`;;;;,  `;;;;;;`    ` 
 `[[     [[ [[[,/[[['  ,[[ '[[, [[[   [[[[[[/,[[     \[[,[[[[[. '[['[==/[[[[,
  $$,    $$ $$$$$$c   c$$$cc$$$c"$$c.    "$$ $$$,     $$$$$$ "Y$c$$  '''    $
  888_,o8P' 888b "88bo,888   888,`Y8bo,,,o88o"888,_ _,88P888    Y88 88b    dP
  MMMMP"`   MMMM   "W" YMM   ""`   `'YMUP"YMM  "YMMMMMP" MMM     YM  "YMmMY" 
*/

#include <ctype.h>
#include <string.h>
#include "h/mud.h"
#include "h/languages.h"
#include "h/new_auth.h"
#include "h/files.h"
#include "h/damage.h"

//For some of the newer dragon spells. -Taon
extern void             failed_casting( SKILLTYPE * skill, CHAR_DATA *ch, CHAR_DATA *victim,
                                        OBJ_DATA *obj );

bool can_fly( CHAR_DATA *ch )
{
    set_char_color( AT_GREY, ch );

    switch ( ch->position ) {
        default:
            break;

        case POS_SLEEPING:
        case POS_RESTING:
        case POS_SITTING:
        case POS_MEDITATING:
            send_to_char( "You must at least be standing to fly!\r\n", ch );
            return FALSE;
    }

    if ( ch->position < POS_FIGHTING ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return FALSE;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return FALSE;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You are already flying - type 'land' and try again.\r\n", ch );
        return 0;
    }

    if ( ch->mount ) {
        send_to_char( "You can't do that while mounted.\r\n", ch );
        return 0;
    }

    return TRUE;
}

void do_wings( CHAR_DATA *ch, char *argument )
{
    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/wings.wav)\r\n", ch );
    set_char_color( AT_GREY, ch );

    if ( ch->race != RACE_DRAGON && ch->race != RACE_CELESTIAL && ch->race != RACE_DEMON
         && ch->race != RACE_VAMPIRE ) {
        if ( ch->race == RACE_PIXIE ) {
            do_fly( ch, ( char * ) "" );
            return;
        }
        error( ch );
        return;
    }

    if ( !can_fly( ch ) )
        return;

    if ( ch->move < 25 ) {
        send_to_char( "You're too tired to do such a thing.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_wings]->beats );
    if ( ch->race == RACE_VAMPIRE ) {
        ch->blood -= 1;
    }
    else
        ch->move -= 25;
    if ( !IS_NPC( ch ) ) {
        if ( ch->race == RACE_VAMPIRE && ch->pcdata->hp_balance != 5 ) {
            act( AT_CYAN,
                 "You suddenly have appendages growing on your back and sprout forth wings!", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_CYAN, "$n suddenly has appendages growing on $s back and sprouts forth wings!",
                 ch, NULL, NULL, TO_ROOM );
            ch->pcdata->hp_balance = 5;
            global_retcode = damage( ch, ch, 1, gsn_wings );
        }
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_wings ) ) {
        AFFECT_DATA             af;

        af.type = gsn_wings;
        af.duration = ch->level * 2 + 40;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_FLYING );
        affect_to_char( ch, &af );
        act( AT_GREY, "You carefully unfold your wings and take flight into the air.", ch, NULL,
             NULL, TO_CHAR );
        act( AT_GREY, "$n unfolds $s wings and takes flight into the air.", ch, NULL, NULL,
             TO_ROOM );
        learn_from_success( ch, gsn_wings );
    }
    else {
        act( AT_GREY,
             "You carefully unfold your wings, but stumble as you try to fly into the air.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_GREY, "$n unfolds $s wings, but stumbles trying to fly into the air.", ch, NULL,
             NULL, TO_ROOM );
        learn_from_failure( ch, gsn_wings );
    }
    return;
}

//Skill is complete.  8-5-08  -Taon

void do_submerged( CHAR_DATA *ch, char *argument )
{

    int                     move_loss;
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
        send_to_char( "Your lungs can already breath with ease under the water.\r\n", ch );
        return;
    }

    move_loss = 50 - get_curr_dex( ch );

    if ( ch->move < move_loss ) {
        send_to_char( "You don't have enough energy.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_submerged ) ) {
        send_to_char( "Your lungs form into gills allowing you to breathe underwater.\r\n", ch );
        af.type = gsn_submerged;
        af.duration = ch->level * 10;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_AQUA_BREATH );
        affect_to_char( ch, &af );
        ch->move -= move_loss;
        learn_from_success( ch, gsn_submerged );
    }
    else
        send_to_char( "You've failed to urge your lungs to breathe underwater.\r\n", ch );
    learn_from_failure( ch, gsn_submerged );
    return;
}

//Skill for black dragons, wrote as an autoskill by Taon, rewrote as
//a skill by Taon.

void do_dominate( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   lvl_diff,
                            chance;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Who do you wish to dominate?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "You must be fighting in order to dominate someone.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }

    if ( ch->move < 25 ) {
        send_to_char( "You dont have enought move to accomplish this task.\r\n", ch );
        return;
    }

    chance = number_range( 1, 100 );
    lvl_diff = ch->level - victim->level;

    if ( lvl_diff < 0 )
        chance -= 20;
    else
        chance += lvl_diff;

    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 5;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) || IS_IMMORTAL( victim ) )
        chance = 0;

    if ( can_use_skill( ch, number_percent(  ), gsn_ballistic ) && chance > 75 ) {
        WAIT_STATE( ch, skill_table[gsn_ballistic]->beats );

        if ( !IS_NPC( victim ) ) {
            ch_printf( ch, "You gaze deeply into the eyes of %s, dominating their soul.\r\n",
                       victim->name );
            ch_printf( victim, "%s gazes deeply into your eyes, dominating your soul.\r\n",
                       ch->name );
        }
        else
            ch_printf( ch, "You gaze deeply into the eyes of %s, dominating their soul.",
                       capitalize( victim->short_descr ) );

        ch->move -= 25;
        do_flee( victim, ( char * ) "" );
        learn_from_success( ch, gsn_dominate );
    }
    else
        send_to_char( "You were unable to dominate your target.\r\n", ch );

    if ( ch->pcdata->learned[gsn_dominate] > 0 )
        learn_from_failure( ch, gsn_dominate );

    return;
}

//Tangle will be a skill meant for silver dragons to tangle their foes
//with their tails. Preventing them from escape, during the duration of
//the entanglement.   -Taon

void do_entangle( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    short                   chance,
                            lvl_diff;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "You must be fighting in order to use your entangle ability.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }
    if ( ch == victim ) {
        send_to_char( "Why in the world would you want to do that?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_TANGLED ) ) {
        send_to_char( "They're already entangled.\r\n", ch );
        return;
    }
    if ( ch->move < 25 ) {
        send_to_char( "You dont have enough move to accomplish this task.\r\n", ch );
        return;
    }

    chance = number_range( 1, 100 );
    lvl_diff = ch->level - victim->level;

    if ( lvl_diff < 0 )
        chance -= 20;
    else
        chance += lvl_diff;

    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 5;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) || IS_IMMORTAL( victim ) )
        chance = 0;

    if ( can_use_skill( ch, number_percent(  ), gsn_entangle ) && chance > 75 ) {
        WAIT_STATE( ch, skill_table[gsn_entangle]->beats );

        if ( !IS_NPC( victim ) ) {
            ch_printf( ch, "You entangle %s's legs with your tail.\r\n", victim->name );
            ch_printf( victim, "%s entangles your legs with their tail.\r\n", ch->name );
        }
        else
            ch_printf( ch, "You entangle %s's legs with your tail.\r\n",
                       capitalize( victim->short_descr ) );

        af.type = gsn_entangle;
        af.duration = ch->level / 2;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_TANGLED );
        affect_to_char( victim, &af );
        ch->move -= 25;
        learn_from_success( ch, gsn_entangle );
    }
    else
        send_to_char( "You were unable to entangle your target.\r\n", ch );

    if ( ch->pcdata->learned[gsn_entangle] > 0 )
        learn_from_failure( ch, gsn_entangle );

    return;
}

//This skill isnt complete, just getting a good foothold on it. -Taon
void do_ballistic( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    char                    arg[MIL];
    short                   miss,
                            xFactor;
    bool                    exist = FALSE;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Who do you wish to strike with your ballistic attack?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "You must be fighting in order to use your ballistic abilities.\r\n",
                          ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }
    if ( ch->mana < 40 ) {
        send_to_char( "You dont have enough mana to do such a task.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "Why in the world would you want to do that?\r\n", ch );
        return;
    }
/* Why a silver dragon that breaths steel, titanium would need an object?  - Vladaar
 for(obj = ch->first_carrying; obj; obj = obj->next_content)
 {
   if(!obj)
    break;

   if(obj->item_type == ITEM_STONE)
   {
    exist = TRUE;
    break;
   }  
 }

 if(!exist)
 {
  send_to_char("You're not carrying any stones.\r\n", ch);
  return;
 }
*/

    if ( can_use_skill( ch, number_percent(  ), gsn_ballistic ) ) {
        WAIT_STATE( ch, skill_table[gsn_ballistic]->beats );

        miss = ( get_curr_dex( victim ) * 2 ) + number_chance( 25, 70 );
        xFactor = ch->pcdata->learned[gsn_ballistic];

        if ( miss > 100 ) {
            act( AT_MAGIC,
                 "You magically hurl a stone at $N, but they quickly dodge out of the way.", ch,
                 NULL, victim, TO_CHAR );
            act( AT_MAGIC, "You quickly sidestep a stone that $n magically hurled at you.", ch,
                 NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n magically hurls a stone at $N, but misses.", ch, NULL, victim,
                 TO_NOTVICT );
            global_retcode = damage( ch, victim, 0, gsn_ballistic );
        }
        else if ( xFactor <= 40 ) {
            act( AT_MAGIC, "You magically hurl a stone at $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n magically hurls a stone at you.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n magically hurls a stone at $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, mediumhigh, gsn_ballistic );
        }
        else if ( xFactor <= 65 ) {
            act( AT_MAGIC, "You magically thrust a stone toward $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n magically thrusts a stone at you.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n magically thrusts a stone at $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, high, gsn_ballistic );
        }
        else if ( xFactor <= 85 ) {
            act( AT_MAGIC, "You summon the energy to magically project a stone at $N.", ch, NULL,
                 victim, TO_CHAR );
            act( AT_MAGIC, "$n thrusts a stone directly at you.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n magically thrusts a stone at $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, extrahigh, gsn_ballistic );
        }
        else {
            act( AT_MAGIC, "You accurately project a stone toward $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n accurately thrusts a stone at you.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n accurately thrusts a stone at $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, ludicrous, gsn_ballistic );
        }

/*
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
*/
        learn_from_success( ch, gsn_ballistic );
        ch->mana -= 40;
    }
    else {
        send_to_char( "You fail to summon a ballistic attack.\r\n", ch );
        global_retcode = damage( ch, victim, 0, gsn_ballistic );
        learn_from_failure( ch, gsn_ballistic );
        ch->mana -= 20;
        return;
    }
    return;
}

void do_gut( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    int                     chance;
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Who are you trying to gut?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
// Fixed so can't target self -- Aurin 12/3/2010
    if ( victim == ch ) {
        send_to_char( "Why are you trying to gut yourself?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_gut ) ) {
        WAIT_STATE( ch, skill_table[gsn_gut]->beats );
        act( AT_RED, "You thrust your taloned claws at $N.", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n thrusts taloned claws into you!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_gut );
        global_retcode = damage( ch, victim, low, gsn_gut );    /* nerfed gut some -
                                                                 * Vladaar */
        chance = number_range( 1, 100 );
        if ( chance < 95 )
            return;

        if ( !char_died( victim ) ) {
            if ( IS_AFFECTED( victim, AFF_POISON ) )
                return;
            act( AT_GREEN, "Your talons have poisoned $N!", ch, NULL, victim, TO_CHAR );
            act( AT_GREEN, "$n's talon attack has poisoned you!", ch, NULL, victim, TO_VICT );
            af.type = gsn_gut;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = -2;
            af.bitvector = meb( AFF_POISON );
            affect_join( victim, &af );
            set_char_color( AT_GREEN, victim );
            send_to_char( "You feel very sick.\r\n", victim );
        }
        else {
            learn_from_failure( ch, gsn_gut );
            WAIT_STATE( ch, 8 );
            global_retcode = damage( ch, victim, 0, gsn_gut );
            send_to_char( "They managed to dodge your talon claws.\r\n", ch );
            return;
        }
        return;

    }
    else {
        act( AT_RED, "You failed to thrust your taloned claws at $N.", ch, NULL, victim, TO_CHAR );
    }
}

void do_tears( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 20;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        victim = ch;
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "Not while fighting.\r\n", ch );
        return;
    }

    if ( victim == who_fighting( ch ) ) {
        send_to_char( "But you're fighting them....\r\n", ch );
        return;
    }
    if ( ch->mana <= nomore ) {
        send_to_char( "You do not have enough mana to summon your tears.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_tears ) ) {
        affect_strip( victim, gsn_tears );
        WAIT_STATE( ch, skill_table[gsn_tears]->beats );
        ch->mana = ( ch->mana - nomore );
        learn_from_success( ch, gsn_tears );
        act( AT_YELLOW, "You focus your mana into righteous blessing.\r\n", ch, NULL, victim,
             TO_CHAR );
        if ( victim == ch ) {
            act( AT_CYAN, "Your tears fill you with a righteous blessing!", ch, NULL, victim,
                 TO_CHAR );
            act( AT_CYAN, "$n's tears well up and $s assumes a righteous appearance!", ch, NULL,
                 victim, TO_NOTVICT );
        }
        else {
            act( AT_CYAN, "Your tears fall upon $N filling them with a righteous blessing!", ch,
                 NULL, victim, TO_CHAR );
            act( AT_CYAN, "$n's tears fall upon you filling you with a righteous blessing!", ch,
                 NULL, victim, TO_VICT );
            act( AT_CYAN, "$n's tears fall upon $N and $N assumes a righteous appearance!", ch,
                 NULL, victim, TO_NOTVICT );
        }
        af.type = gsn_tears;
        af.duration = ch->level * 2;
        af.location = APPLY_RESISTANT;
        af.modifier = 16;
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( victim, &af );

        af.type = gsn_tears;
        af.duration = ch->level * 2;
        af.location = APPLY_RESISTANT;
        af.modifier = 32;
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( victim, &af );

        af.type = gsn_tears;
        af.duration = ch->level * 2;
        af.location = APPLY_RESISTANT;
        af.modifier = 64;
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( victim, &af );

        return;
    }
    else
        act( AT_CYAN, "You try to shed some tears, but get distracted.", ch, NULL, victim,
             TO_CHAR );
    learn_from_failure( ch, gsn_tears );
    return;
}

void do_defend( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 20;

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_SANCTUARY ) ) {
        send_to_char( "You already have a defensive shield in place.\r\n", ch );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_defend ) ) {
        if ( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL, TO_ROOM );
        spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
        WAIT_STATE( ch, skill_table[gsn_defend]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else
        send_to_char( "You attempt to put a defending shield, but get distracted.\r\n", ch );
    return;
}

void do_cone( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;
    short                   chance;

    nomore = 20;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Send a cone attack on who?\r\n", ch );
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

    if ( IS_NPC( ch ) ) {
        act( AT_DGREY, "You launch a cone of shadow energy from your hand!", ch, NULL, victim,
             TO_CHAR );
        act( AT_DGREY, "$n launches a cone of shadow energy that engulfs you!", ch, NULL, victim,
             TO_VICT );
        act( AT_DGREY, "$n launches a cone of shadow energy, engulfing $N in its fury!", ch, NULL,
             victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    chance = ( 1, 10 );

    if ( chance < 3 ) {
        send_to_char( "You attempt to lanch a cone attack, but get distracted.\r\n", ch );
        return;
    }

    short                   twoheaded;

    if ( ch->Class == CLASS_TWOHEADED ) {
        twoheaded = number_range( 1, 5 );
    }
    if ( ch->pcdata->tmpclass == 8 || twoheaded == 1 ) {
        act( AT_DGREY, "You launch a cone of shadow energy from your hand!", ch, NULL, victim,
             TO_CHAR );
        act( AT_DGREY, "$n launches a cone of shadow energy that engulfs you!", ch, NULL, victim,
             TO_VICT );
        act( AT_DGREY, "$n launches a cone of shadow energy, engulfing $N in its fury!", ch, NULL,
             victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 9 || twoheaded == 2 ) {
        act( AT_YELLOW, "You launch a cone of holy energy from your hand!", ch, NULL, victim,
             TO_CHAR );
        act( AT_YELLOW, "$n launches a cone of holy energy that engulfs you!", ch, NULL, victim,
             TO_VICT );
        act( AT_YELLOW, "$n launches a cone of holy energy, engulfing $N inits fury!", ch, NULL,
             victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 10 || twoheaded == 3 ) {
        act( AT_GREY, "You launch a cone of iron shards from your hand!", ch, NULL, victim,
             TO_CHAR );
        act( AT_GREY, "$n launches a cone of iron shards that tear into you!", ch, NULL, victim,
             TO_VICT );
        act( AT_GREY, "$n launches a cone of iron shards that tear into $N!", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 11 || twoheaded == 4 ) {
        act( AT_RED, "You launch a cone of choking smoke from your hand!", ch, NULL, victim,
             TO_CHAR );
        act( AT_RED, "$n launches a cone of smoke that chokes you!", ch, NULL, victim, TO_VICT );
        act( AT_RED, "$n launches a cone of smoke, choking $N!", ch, NULL, victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 12 || twoheaded == 5 ) {
        act( AT_LBLUE, "You launch a cone of water from your hand!", ch, NULL, victim, TO_CHAR );
        act( AT_LBLUE, "$n launches a cone of water that engulfs you in its fury!", ch, NULL,
             victim, TO_VICT );
        act( AT_LBLUE, "$n launches a cone of water that engulfs $N in its fury!", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else
        log_string( "Bug: Dragon Lord player with incorrect tmpclass #" );
    return;
}

/* Strip all their gear for changing form */
void remove_all_equipment( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    int                     x,
                            y;

    if ( !ch )
        return;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
        if ( obj->wear_loc > -1 )
            unequip_char( ch, obj );
}

void humanform_change( CHAR_DATA *ch, bool tohuman )
{                                                      /* tohuman=TRUE if going to human,
                                                        * * =FALSE if going to dragon */
    short                   backup[MAX_SKILL];
    bool                    dshowbackup[MAX_SKILL];
    int                     sn = 0;
    short                   ability = 0;
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) ) {
        send_to_char( "You can't do humanform_change if you are NPC!", ch );
        return;
    }

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "You can't do humanform_change if you are sleeping!", ch );
        return;
    }

    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
        dshowbackup[sn] = FALSE;
        backup[sn] = 0;
    }

    if ( tohuman ) {
        send_to_char( "\r\nYou remove all your gear as your size is about to vastly change.\r\n",
                      ch );
        remove_all_equipment( ch );

        ch->pcdata->tmprace = ch->race;
        ch->pcdata->tmpclass = ch->Class;
        ch->race = 0;
        ch->Class = CLASS_DRAGONLORD;

        /*
         * Ok this is a simple way of handling it, we toss through the sns and set
         * everything up as we go 
         */
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            /*
             * Toss into backup 
             */
            if ( ch->pcdata->learned[sn] > 0 ) {
                backup[sn] = ch->pcdata->learned[sn];
                dshowbackup[sn] = ch->pcdata->dshowlearned[sn];
            }

            /*
             * Toss on human skills 
             */
            if ( ch->pcdata->dlearned[sn] > 0 ) {
                ch->pcdata->learned[sn] = ch->pcdata->dlearned[sn];
                ch->pcdata->dlearned[sn] = 0;
                ch->pcdata->dshowlearned[sn] = ch->pcdata->dshowdlearned[sn];
            }

            /*
             * Toss backup into dlearned 
             */
            if ( backup[sn] > 0 ) {
                ch->pcdata->dlearned[sn] = backup[sn];
                backup[sn] = 0;
                ch->pcdata->dshowdlearned[sn] = dshowbackup[sn];
            }
        }

        // Lastly reinstate human form % ..
        ability = skill_lookup( "human form" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "common" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "draconic" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];

        while ( ch->first_affect )
            affect_remove( ch, ch->first_affect );

        ch->pcdata->tmpmax_hit = ch->max_hit;
        ch->pcdata->tmpheight = ch->height;
        ch->pcdata->tmpweight = ch->weight;

        /*
         * Lets give them the same percent of their higher hp 
         */
        {
            double                  hitprcnt = 0.0;

            if ( ch->hit > 0 && ch->max_hit > 0 )
                hitprcnt = ( ch->hit / ch->max_hit );
            ch->max_hit -= ch->max_hit / 6;
            if ( hitprcnt > 0.0 )
                ch->hit = ( int ) ( ch->max_hit * hitprcnt );
        }

        ch->height = 72;
        ch->weight = 200;

        if ( !IS_NPC( ch ) )
            update_aris( ch );

        save_char_obj( ch );
        WAIT_STATE( ch, skill_table[gsn_human_form]->beats );
        act( AT_MAGIC, "You call upon ancient magic and assume human form.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_MAGIC, "$n suddenly shimmers and assumes human form.", ch, NULL, NULL, TO_ROOM );
        act( AT_CYAN,
             "You fight down a sense of disorientation as your once massive body is human size!",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_human_form );
        af.type = gsn_human_form;
        af.duration = ch->level * 2;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_DRAGONLORD );
        affect_to_char( ch, &af );
        send_to_char( "\r\nNow in your new form, you once again don your gear.\r\n", ch );
        interpret( ch, ( char * ) "wear all" );
        save_char_obj( ch );
    }
    else {                                             /* going to dragon */

        send_to_char( "\r\nYou remove all your gear as your size is about to vastly change.\r\n",
                      ch );
        remove_all_equipment( ch );

        send_to_char( "You release the ancient magic, and return to dragon form.\r\n", ch );

        act( AT_MAGIC, "$n suddenly shimmers and assumes the form of a mighty dragon!", ch, NULL,
             NULL, TO_ROOM );
        act( AT_CYAN, "You feel pure elation as your human body is restored to dragon size!", ch,
             NULL, NULL, TO_CHAR );

        while ( ch->first_affect )
            affect_remove( ch, ch->first_affect );

        /*
         * Lets give them the same percent of their higher hp 
         */
        {
            double                  hitprcnt = 0.0;

            if ( ch->hit > 0 && ch->max_hit > 0 )
                hitprcnt = ( ch->hit / ch->max_hit );
            if ( ch->pcdata->tmpmax_hit )
                ch->max_hit = ch->pcdata->tmpmax_hit;
            else {
                ch->max_hit *= 2;                      /* Since all we did was divide it
                                                        * might as well multiply it now
                                                        * but give bug message too. */
                bug( "%s: %s had 0 tmpmax_hit and had to double their current hp.", __FUNCTION__,
                     ch->name );
            }
            if ( hitprcnt > 0.0 )
                ch->hit = ( int ) ( ch->max_hit * hitprcnt );
        }
        ch->height = ch->pcdata->tmpheight;
        ch->weight = ch->pcdata->tmpweight;
        ch->race = ch->pcdata->tmprace;
        ch->Class = ch->pcdata->tmpclass;
        ch->pcdata->tmpmax_hit = 0;
        ch->pcdata->tmpheight = 0;
        ch->pcdata->tmpweight = 0;
        ch->pcdata->tmprace = 0;
        ch->pcdata->tmpclass = 0;

        /*
         * Ok this is a simple way of handling it, we toss through the sns and set
         * everything up as we go 
         */
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            /*
             * Toss into backup 
             */
            if ( ch->pcdata->learned[sn] > 0 ) {
                backup[sn] = ch->pcdata->learned[sn];
                dshowbackup[sn] = ch->pcdata->dshowlearned[sn];
            }

            /*
             * Toss on human skills 
             */
            if ( ch->pcdata->dlearned[sn] > 0 ) {
                ch->pcdata->learned[sn] = ch->pcdata->dlearned[sn];
                ch->pcdata->dlearned[sn] = 0;
                ch->pcdata->dshowlearned[sn] = ch->pcdata->dshowdlearned[sn];
                ch->pcdata->dshowdlearned[sn] = FALSE;
            }

            /*
             * Toss backup into dlearned 
             */
            if ( backup[sn] > 0 ) {
                ch->pcdata->dlearned[sn] = backup[sn];
                backup[sn] = 0;
                ch->pcdata->dshowdlearned[sn] = dshowbackup[sn];
            }
        }

        /*
         * Lastly reinstate human form % .. 
         */
        ability = skill_lookup( "human form" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "common" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "draconic" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];

        if ( !IS_NPC( ch ) )
            update_aris( ch );
        save_char_obj( ch );
    }
}

void do_human_form( CHAR_DATA *ch, char *argument )
{
    short                   nomore = 100;
    AFFECT_DATA             af;

    // Temp fix for players switching from dragons to human form to remove curse and
    // poison. -Taon
    if ( IS_AFFECTED( ch, AFF_CURSE ) || IS_AFFECTED( ch, AFF_POISON ) ) {
        send_to_char( "You can't change form when cursed or affected by poison.", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        affect_strip( ch, gsn_human_form );
        xREMOVE_BIT( ch->affected_by, AFF_DRAGONLORD );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_human_form ) ) {
        ch->mana = ( ch->mana - nomore );
        humanform_change( ch, TRUE );
        return;
    }
    else {
        learn_from_failure( ch, gsn_human_form );
        send_to_char( "&cYou fail to summon the ancient magic.\r\n", ch );
        return;
    }
}

void do_lick( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   nomore;

    nomore = ch->level + 10;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        victim = ch;
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "Not while fighting.\r\n", ch );
        return;
    }

    if ( victim == who_fighting( ch ) ) {
        send_to_char( "But you're fighting them....\r\n", ch );
        return;
    }
    if ( ch->mana <= nomore ) {
        send_to_char( "You do not have enough mana to perform a healing dragon lick.\r\n", ch );
        return;
    }

    if ( victim->hit >= victim->max_hit ) {
        send_to_char( "This one is already at max health.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_lick ) ) {
        WAIT_STATE( ch, skill_table[gsn_lick]->beats );
        ch->mana = ( ch->mana - nomore );
        learn_from_success( ch, gsn_lick );
        act( AT_YELLOW, "You focus your mana into healing energies.\r\n", ch, NULL, victim,
             TO_CHAR );
        if ( victim == ch ) {
            act( AT_CYAN, "You open your mighty jaws, and lick your wounds.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_CYAN, "$n opens $s mighty jaws and licks $s wounds!", ch, NULL, victim,
                 TO_NOTVICT );
            act( AT_MAGIC, "Your wounds begin to heal up and stop bleeding!", ch, NULL, victim,
                 TO_CHAR );
        }
        else {
            act( AT_CYAN, "You open your mighty jaws, and lick $N's wounds.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_CYAN, "$n opens $s mighty jaws, and licks your wounds!", ch, NULL, victim,
                 TO_VICT );
            act( AT_CYAN, "$n opens $s mighty jaws and licks $N's wounds!", ch, NULL, victim,
                 TO_NOTVICT );
            act( AT_MAGIC, "Your wounds begin to heal up and stop bleeding!", ch, NULL, victim,
                 TO_VICT );
        }
        victim->hit = victim->hit + ( ch->level / 2 ) + 50;
        if ( victim->hit > victim->max_hit ) {         // Bug fix below, -Taon
            victim->hit = victim->max_hit;
        }
        return;
    }
    else
        act( AT_CYAN, "You try to focus your mana into healing energies, but get distracted.", ch,
             NULL, victim, TO_CHAR );
    learn_from_failure( ch, gsn_lick );
    return;
}

const int               MidDBLevel = 30;
const int               HighDBLevel = 60;

int GetBreathDam( int level, int power )
{
// Rewrote the entire damage scale here. -Taon
// Rewrote it again based on breath levels - Torin - 9/12/2007 2:28PM  dam1 is level 1, dam2 is level 30, dam3 is level 60
// power = URANGE(1, power, 3);
// Updated default and Case 2 so damage wasn't the same for <50 level - 
// Aurin 11/21/2010
    switch ( power ) {
        default:
            if ( level < 8 )
                return number_range( 30, 35 );
            if ( level < 15 )
                return number_range( 40, 45 ) + level * 2;
            if ( level < 25 )
                return number_range( 55, 65 ) + level * 3;
            if ( level < 35 )
                return ( int ) ( number_range( 95, 105 ) + level * 3.5 );
            if ( level < 50 )
                return ( int ) ( number_range( 150, 160 ) + level * 3.8 );
            if ( level < 65 )
                return ( int ) ( number_range( 250, 260 ) + level * 4.0 );
            if ( level < 80 )
                return ( int ) ( number_range( 255, 265 ) + level * 5.0 );
            return ( int ) ( number_range( 255, 265 ) + level * 6 );
        case 2:
            if ( level < 8 )
                return number_range( 45, 55 );
            if ( level < 15 )
                return number_range( 65, 75 ) + level * 2;
            if ( level < 25 )
                return number_range( 85, 95 ) + level * 3;
            if ( level < 35 )
                return ( int ) ( number_range( 175, 185 ) + level * 3.5 );
            if ( level < 50 )
                return ( int ) ( number_range( 185, 195 ) + level * 3.8 );
            if ( level < 65 )
                return ( int ) ( number_range( 275, 285 ) + level * 4.0 );
            if ( level < 80 )
                return ( int ) ( number_range( 295, 310 ) + level * 5.0 );
            return ( int ) ( number_range( 295, 310 ) + level * 6 );
        case 3:
            if ( level < 8 )
                return number_range( 50, 60 );
            if ( level < 15 )
                return ( int ) ( number_range( 100, 110 ) + level * 2.5 );
            if ( level < 25 )
                return ( int ) ( number_range( 190, 200 ) + level * 3.5 );
            if ( level < 35 )
                return ( int ) ( number_range( 250, 260 ) + level * 4 );
            if ( level < 50 )
                return ( int ) ( number_range( 275, 395 ) + level * 4.5 );
            if ( level < 65 )
                return ( int ) ( number_range( 585, 605 ) + level * 5.5 );
            if ( level < 80 )
                return ( int ) ( number_range( 695, 710 ) + level * 6.5 );
            return ( int ) ( number_range( 695, 710 ) + level * 8 );
    }
}

void SendDBHelp( CHAR_DATA *ch )
{
    char                    buf[MIL];

    send_to_char( "What do you want to breathe?\r\n", ch );
    send_to_char( "Syntax: breathe [victim] [type]\r\n", ch );
    send_to_char( "Types are as follows:\r\n", ch );
    switch ( ch->Class ) {
        default:
            strcpy( buf, "none\r\n" );
            break;
        case CLASS_BLACK:
            sprintf( buf, "shadow, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "darkness, ",
                     ( ch->level < HighDBLevel ) ? "" : "death, " );
            break;
        case CLASS_RED:
            sprintf( buf, "smoke, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "fire, ",
                     ( ch->level < HighDBLevel ) ? "" : "hellfire, " );
            break;
        case CLASS_SILVER:
            sprintf( buf, "iron, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "steel, ",
                     ( ch->level < HighDBLevel ) ? "" : "titanium, " );
            break;
        case CLASS_BLUE:
            sprintf( buf, "water, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "ice, ",
                     ( ch->level < HighDBLevel ) ? "" : "glacial, " );
            break;
        case CLASS_GOLD:
            sprintf( buf, "holy, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "divine, ",
                     ( ch->level < HighDBLevel ) ? "" : "heavenly, " );
            break;
        case CLASS_TWOHEADED:
            sprintf( buf, "shadow, smoke, iron, water, holy, %s%saoe\r\n",
                     ( ch->level < MidDBLevel ) ? "" : "darkness, fire, steel, ice, divine,\r\n",
                     ( ch->level <
                       HighDBLevel ) ? "" : "heavenly, death, titanium, hellfire, glacial, " );
            break;
    }
    send_to_char( buf, ch );
    return;
}

void do_breathe( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch,
                           *vch_next;
    AFFECT_DATA             af;
    char                    arg[MIL];
    char                    arg2[MIL];
    bool                    ch_died = FALSE;

    int                     nomore = UMIN( ch->level + 10, 60 );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ( ch->move <= 25 || ch->move < nomore ) ) {
        set_char_color( AT_WHITE, ch );
        send_to_char( "You are far too tired to do that.\r\n", ch );
        return;                                        /* missing return fixed March
                                                        * 11/96 */
    }
    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( !str_cmp( arg, "aoe" ) && !*arg2 ) || !str_cmp( arg2, "aoe" ) ) {
        if ( ch->move < nomore * 2 ) {
            set_char_color( AT_WHITE, ch );
            send_to_char( "You are far too tired to do that.\r\n", ch );
            return;
        }

        short                   twoheaded;

        twoheaded = 0;
        if ( ch->Class == CLASS_TWOHEADED ) {
            twoheaded = number_range( 1, 5 );
        }

        if ( ch->Class == CLASS_GOLD || twoheaded == 1 ) {
            act( AT_YELLOW, "You breathe out a cloud of holy energy!", ch, NULL, NULL, TO_CHAR );
            act( AT_YELLOW,
                 "$n breathes out a cloud of holy energy, engulfing everything in its fury!", ch,
                 NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_RED || twoheaded == 2 ) {
            act( AT_RED, "You breathe out a choking cloud of smoke!", ch, NULL, NULL, TO_CHAR );
            act( AT_RED,
                 "$n breathes out a choking cloud of smoke, engulfing everything in its fury!", ch,
                 NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_BLACK || twoheaded == 3 ) {
            act( AT_DGREY, "You breathe out a cloud of shadow energy!", ch, NULL, NULL, TO_CHAR );
            act( AT_DGREY,
                 "$n breathes out a cloud of shadow energy, engulfing everything in its fury!", ch,
                 NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_SILVER || twoheaded == 4 ) {
            act( AT_GREY, "You breathe out a huge mass of iron shards!", ch, NULL, NULL, TO_CHAR );
            act( AT_GREY,
                 "$n breathes out a huge mass of iron shards, engulfing everything in its fury!",
                 ch, NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_BLUE || twoheaded == 5 ) {
            act( AT_LBLUE, "You breathe out a heavy stream of water!", ch, NULL, NULL, TO_CHAR );
            act( AT_LBLUE,
                 "$n breathes out a heavy stream of water, engulfing everything in its fury!", ch,
                 NULL, NULL, TO_ROOM );
        }
        WAIT_STATE( ch, skill_table[gsn_breath]->beats );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !IS_NPC( vch ) && check_phase( ch, vch ) )
                return;
            if ( !IS_NPC( vch ) && check_tumble( ch, vch ) )
                return;

            if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
                continue;

            // Bug fix here, was striking grouped members. -Taon
            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;
            if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) {
                if ( char_died( ch ) ) {
                    ch_died = TRUE;
                    break;
                }
                global_retcode = damage( ch, vch, GetBreathDam( ch->level, 1 ), gsn_breath );
            }
        }
        ch->move = ( ch->move - nomore * 2 );
        learn_from_success( ch, gsn_breath );
        return;
    }
    if ( arg[0] == '\0' && !ch->fighting ) {
        SendDBHelp( ch );
        return;
    }
    CHAR_DATA              *victim = get_char_room( ch, arg );

    if ( !victim ) {
        if ( *arg2 || ( victim = who_fighting( ch ) ) == NULL ) {
            send_to_char( "They aren't here.\r\n", ch );
            return;
        }
        strcpy( arg2, arg );
    }

    short                   coin;

    coin = number_chance( 1, 4 );

    if ( !IS_NPC( victim ) ) {
        OBJ_DATA               *obj;

        if ( ( obj = get_eq_char( victim, WEAR_SHIELD ) ) != NULL ) {
            act( AT_DGREEN, "You block $n's dragon breath with your shield.", ch, NULL, victim,
                 TO_VICT );
            act( AT_DGREEN, "$N blocks your dragon breath with $S shield.", ch, NULL, victim,
                 TO_CHAR );
            return;
        }
        else if ( victim->pcdata->learned[skill_lookup( "tumble" )] > 0 && coin > 2 ) {
            act( AT_SKILL, "You tumble away from $n's dragon breath.", ch, NULL, victim, TO_VICT );
            act( AT_SKILL, "$N tumbles away from your dragon breath.", ch, NULL, victim, TO_CHAR );
            return;
        }
        else if ( victim->pcdata->learned[skill_lookup( "phase" )] > 0 && coin > 2 ) {
            act( AT_DGREEN, "Your body phases, absorbing $n's dragon breath.", ch, NULL, victim,
                 TO_VICT );
            act( AT_DGREEN, "$N's body phases, absorbing your dragon breath.", ch, NULL, victim,
                 TO_CHAR );
            return;
        }
    }

    int                     chance = number_range( 1, 100 );

// Updated to allow damage from second head only if target specified; 
// reduced second head damage so there is a noticeable difference at 
// higher levels...this was originally intended as a small damage 
// increase - Aurin 11/21/2010

    if ( ch->Class == CLASS_TWOHEADED && chance ) {
        act( AT_SKILL, "Your other dragon head also breathes at the same time!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_SKILL, "$n's other dragon head also breathes at the same time!", ch, NULL, NULL,
             TO_ROOM );
        if ( ch->fighting )
            victim = who_fighting( ch );
        global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
    }

// BLACK Dragons - Torin
// Following ifcheck is a bugfix - Aurin 11/22/2010
    if ( char_died( victim ) ) {
        ch_died = TRUE;
    }
    else {
        if ( ch->Class == CLASS_BLACK || ch->Class == CLASS_TWOHEADED ) {
            if ( !str_prefix( arg2, "death" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    learn_from_success( ch, gsn_breath );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_DGREY, "You breathe out a cloud of pure, malevolent EVIL!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_DGREY,
                         "$n breathes out a cloud of pure, malevolent EVIL that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_DGREY,
                         "$n breathes out a cloud of pure, malevolent EVIL, engulfing $N in its fury!",
                         ch, NULL, victim, TO_NOTVICT );
                    if ( chance < 11 ) {
// stop players being changed to human priests - Vladaar
                        if ( IS_AFFECTED( victim, AFF_DRAGONLORD ) ) {
                            interpret( victim, ( char * ) "human" );
                        }

                        while ( victim->first_affect )
                            affect_remove( victim, victim->first_affect );
                        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
                        }
                        else if ( IS_AFFECTED( victim, AFF_SHIELD ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SHIELD );
                        }
                        if ( IS_AFFECTED( victim, AFF_POISON ) )
                            return;
                        act( AT_DGREY,
                             "Your malevolent EVIL cloud has greatly dispelled and poisoned $N!",
                             ch, NULL, victim, TO_CHAR );
                        act( AT_DGREY,
                             "$n's malevolent EVIL cloud has greatly dispelled and poisoned you!",
                             ch, NULL, victim, TO_VICT );
                        af.type = gsn_poison;
                        af.duration = ch->level;
                        af.level = ch->level;
                        af.location = APPLY_STR;
                        af.modifier = -2;
                        victim->degree = 1;
                        af.bitvector = meb( AFF_POISON );
                        affect_join( victim, &af );

                    }

                    // Made death breath hit a little harder because of the
                    // chance being taking. -Taon
                    global_retcode =
                        damage( ch, victim, GetBreathDam( ch->level, 3 ) + ( ch->level * 2 ),
                                gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "darkness" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_DGREY, "You breathe out a cloud of pure darkness!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_DGREY, "$n breathes out a cloud of darkness that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_DGREY, "$n breathes out a cloud of darkness, engulfing $N in its fury!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_DGREY, "Your cloud of darkness has greatly dispelled $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_DGREY, "$n's cloud of darkness has greatly dispelled you!", ch,
                             NULL, victim, TO_VICT );
                        while ( victim->first_affect )
                            affect_remove( victim, victim->first_affect );
                        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
                        }
                        else if ( IS_AFFECTED( victim, AFF_SHIELD ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SHIELD );
                        }

                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "shadow" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_DGREY, "You breathe out a cloud of shadow energy!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_DGREY, "$n breathes out a cloud of shadow energy that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_DGREY,
                         "$n breathes out a cloud of shadow energy, engulfing $N in its fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        if ( IS_AFFECTED( victim, AFF_POISON ) )
                            return;
                        act( AT_DGREY, "Your cloud of darkness has poisoned $N!", ch, NULL, victim,
                             TO_CHAR );
                        act( AT_DGREY, "$n's cloud of darkness has poisoned you!", ch, NULL, victim,
                             TO_VICT );
                        af.type = gsn_poison;
                        af.duration = ch->level;
                        af.level = ch->level;
                        af.location = APPLY_STR;
                        af.modifier = -2;
                        victim->degree = 1;
                        af.bitvector = meb( AFF_POISON );
                        affect_join( victim, &af );
                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_RED || ch->Class == CLASS_TWOHEADED ) {
            // RED dragons - Torin
            if ( !str_prefix( arg2, "hellfire" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );

                    act( AT_RED, "You breathe out the pure, white-hot flames of HELL!!!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_RED,
                         "$n breathes out the pure, white-hot flames of HELL that engulf you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_RED,
                         "$n breathes out the pure, white-hote flames of HELL, engulfing $N in its fury!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_RED, "Your hellfire breath catches $N on fire!", ch, NULL, victim,
                             TO_CHAR );
                        act( AT_RED, "$n's hellfire breath catches you on fire!", ch, NULL, victim,
                             TO_VICT );
                        af.type = gsn_thaitin;
                        af.duration = 50;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_THAITIN );
                        affect_to_char( victim, &af );
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

                    }
                    else
                        global_retcode =
                            damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "fire" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_RED, "You breathe out a massive cloud of fire!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_RED, "$n breathes out massive cloud of fire that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_RED,
                         "$n breathes out a massive cloud of fire, engulfing $N in its fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_RED, "Your massive cloud of fire catches $N on fire!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_RED, "$n's massive cloud of fire catches you on fire!", ch, NULL,
                             victim, TO_VICT );
                        af.type = gsn_thaitin;
                        af.duration = 50;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_THAITIN );
                        affect_to_char( victim, &af );
                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "smoke" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_RED, "You breathe out a choking cloud of smoke!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_RED, "$n breathes out a cloud of smoke that chokes you!", ch, NULL,
                         victim, TO_VICT );
                    act( AT_RED, "$n breathes out a cloud of smoke, choking $N!", ch, NULL, victim,
                         TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( !IS_AFFECTED( victim, AFF_BLINDNESS ) && chance < 11 ) {
                        act( AT_RED, "Your cloud of smoke blinds $N!", ch, NULL, victim, TO_CHAR );
                        act( AT_RED, "$n's cloud of smoke blinds you!", ch, NULL, victim, TO_VICT );

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
                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_SILVER || ch->Class == CLASS_TWOHEADED ) {
            // SILVER dragons - Torin
            if ( !str_prefix( arg2, "titanium" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_GREY, "You breathe out a huge, solid mass of titanium!!!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_GREY,
                         "$n breathes out a huge, solid mass of titanium that slams into you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_GREY,
                         "$n breathes out a huge, solid mass of titanium that slams into $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_GREY, "Your titanium breath knocks down and weakens $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_GREY, "$n's titanium breath weakens and knocks you down!!!", ch,
                             NULL, victim, TO_VICT );
                        af.type = gsn_breath;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.duration = 1;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_PARALYSIS );
                        affect_to_char( victim, &af );
                        if ( IS_NPC( victim ) )
                            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
                        else
                            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 2 ) );
                        update_pos( victim );
                        af.type = gsn_breath;
                        af.duration = ch->level + 10;
                        af.location = APPLY_STR;
                        af.modifier = -4;
                        af.level = ch->level;
                        xCLEAR_BITS( af.bitvector );
                        affect_to_char( victim, &af );

                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "steel" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_GREY, "You breathe out a stream of molten steel!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_GREY, "$n breathes out a stream of molten steel that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_GREY,
                         "$n breathes out a stream of molten steel, engulfing $N in its fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_GREY, "Your steel breath knocks $N down!!!", ch, NULL, victim, TO_CHAR );   // Aurin
                        act( AT_GREY, "$n's steel breath knocks you down!!!", ch, NULL, victim, TO_VICT );  // Aurin
                        af.type = gsn_breath;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.duration = 1;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_PARALYSIS );
                        affect_to_char( victim, &af );
                        if ( IS_NPC( victim ) )
                            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
                        else
                            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 2 ) );
                        update_pos( victim );
                    }

                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "iron" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_GREY, "You breathe out a huge mass of iron shards!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_GREY, "$n breathes out a huge mass of iron shards that tear into you!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_GREY, "$n breathes out a huge mass of iron shards that tear into $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_GREY, "Your iron shards weaken $N!", ch, NULL, victim, TO_CHAR );
                        act( AT_GREY, "$n's iron shards weaken you!", ch, NULL, victim, TO_VICT );
                        af.type = gsn_breath;
                        af.duration = ch->level + 10;
                        af.location = APPLY_STR;
                        af.modifier = -4;
                        af.level = ch->level;
                        xCLEAR_BITS( af.bitvector );
                        affect_to_char( victim, &af );
                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_BLUE || ch->Class == CLASS_TWOHEADED ) {
            // BLUE Dragons
            if ( !str_prefix( arg2, "glacial" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_LBLUE, "You breathe out a huge, solid mass of ice!!!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_LBLUE, "$n breathes out a huge, solid mass of ice that slams into you!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_LBLUE, "$n breathes out a huge, solid mass of ice that slams into $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                    if ( chance < 11 && !char_died( victim ) ) {
                        act( AT_LBLUE,
                             "Your glacial breath gives frostbite and starts to drown $N!", ch,
                             NULL, victim, TO_CHAR );
                        act( AT_LBLUE,
                             "$n's glacial breath gives you frostbite and starts to drown you!", ch,
                             NULL, victim, TO_VICT );
                        if ( IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 300, 400 ), gsn_breath );
                        }
                        else if ( !IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 300, 400 ), gsn_breath );
                            victim->pcdata->frostbite = -5;
                            victim->pcdata->holdbreath = -5;
                        }

                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "ice" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_LBLUE, "You breathe out a hail of ice shards!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_LBLUE, "$n breathes out a hail of ice shards that engulf you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_LBLUE,
                         "$n breathes out a hail of ice shards, engulfing $N in its fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 && !char_died( victim ) ) {
                        act( AT_LBLUE, "Your breathe of ice gives frost bite to $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_LBLUE, "$n's breathe of ice gives frost bite to you!", ch, NULL,
                             victim, TO_VICT );
                        if ( IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 100, 300 ), gsn_breath );
                        }
                        else if ( !IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 100, 300 ), gsn_breath );
                            victim->pcdata->frostbite = -5;
                        }
                    }

                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "water" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_LBLUE, "You breathe out a heavy stream of water!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_LBLUE,
                         "$n breathes out a heavy stream of water that engulfs you in its fury!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_LBLUE,
                         "$n breathes out a heavy stream of water that engulfs $N in its fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                    if ( chance < 11 && !char_died( victim ) ) {
                        act( AT_LBLUE, "Your breathe of water begins to drown $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_LBLUE, "$n's breathe of heavy water begins to drown you!", ch, NULL,
                             victim, TO_VICT );
                        if ( IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 50, 200 ), gsn_breath );
                        }
                        else if ( !IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 50, 200 ), gsn_breath );
                            victim->pcdata->holdbreath = -5;
                        }
                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_GOLD || ch->Class == CLASS_TWOHEADED ) {
            // GOLD dragons - Torin
            if ( !str_prefix( arg2, "heavenly" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_YELLOW, "You breathe out a maelstrom of heavenly power!!!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_YELLOW,
                         "$n breathes out a maelstrom of heavenly power that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "$n breathes out a maelstrom of heavely power that engulfs $N in its glorious power!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_YELLOW,
                             "Your maelstrom of heavenly power cures your afflictions, and absorbs some of $N's health!",
                             ch, NULL, victim, TO_CHAR );
                        if ( ch->hit < ch->max_hit - 500 ) {
                            ch->hit += 500;
                        }
                        affect_strip( ch, gsn_maim );
                        affect_strip( ch, gsn_brittle_bone );
                        affect_strip( ch, gsn_festering_wound );
                        affect_strip( ch, gsn_poison );
                        affect_strip( ch, gsn_thaitin );
                        xREMOVE_BIT( ch->affected_by, AFF_THAITIN );
                        xREMOVE_BIT( ch->affected_by, AFF_BRITTLE_BONES );
                        xREMOVE_BIT( ch->affected_by, AFF_FUNGAL_TOXIN );
                        xREMOVE_BIT( ch->affected_by, AFF_MAIM );
                    }
                    return;
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "divine" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_YELLOW, "You breathe out a stream of divine energy!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_YELLOW, "$n breathes out a stream of divine energy that engulfs you!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "$n breathes out a stream of divine energy, engulfing $N in its fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_YELLOW,
                             "Your stream of divine energy absorbs $N's health and returns some to you!",
                             ch, NULL, victim, TO_CHAR );
                        act( AT_YELLOW,
                             "$n's stream of divine energy absorbs your health and heals them!", ch,
                             NULL, victim, TO_VICT );
                        if ( ch->hit < ch->max_hit - 200 ) {
                            ch->hit += 200;
                        }
                    }

                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "holy" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_YELLOW, "You breathe out a cloud of holy energy!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_YELLOW, "$n breathes out a cloud of holy energy that engulfs you!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "$n breathes out a cloud of holy energy, engulfing $N inits fury!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_YELLOW, "Your cloud of holy energy cures your afflictions!", ch,
                             NULL, victim, TO_CHAR );
                        affect_strip( ch, gsn_maim );
                        affect_strip( ch, gsn_brittle_bone );
                        affect_strip( ch, gsn_festering_wound );
                        affect_strip( ch, gsn_poison );
                        affect_strip( ch, gsn_thaitin );
                        xREMOVE_BIT( ch->affected_by, AFF_THAITIN );
                        xREMOVE_BIT( ch->affected_by, AFF_BRITTLE_BONES );
                        xREMOVE_BIT( ch->affected_by, AFF_FUNGAL_TOXIN );
                        xREMOVE_BIT( ch->affected_by, AFF_MAIM );

                    }

                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "You couldn't summon enough power to use your mighty breath.\r\n",
                                  ch );
                }
                return;
            }
        }
        SendDBHelp( ch );
        return;
    }
}                                                      // Closes out char_died ifcheck -
                                                       // Aurin

void do_stomp( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "You must be fighting in order to stomp.\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why in the world would you want to do that?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_stomp ) ) {
        WAIT_STATE( ch, skill_table[gsn_stomp]->beats );
        act( AT_RED, "You drive your foot down upon $N!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "A shadow appears overhead as $n's foot comes crashing down at you!", ch, NULL,
             victim, TO_VICT );
        act( AT_RED, "$n drives a massive foot down upon $N smashing them into the ground!", ch,
             NULL, victim, TO_NOTVICT );
        act( AT_MAGIC, "The earth trembles beneath your feet!", ch, NULL, NULL, TO_CHAR );
        act( AT_MAGIC, "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_stomp );
        global_retcode = damage( ch, victim, medium, gsn_stomp );
    }
    else {
        learn_from_failure( ch, gsn_stomp );
        global_retcode = damage( ch, victim, 0, gsn_stomp );
        send_to_char( "You try to stomp, but they dodge your attempt.\r\n", ch );
    }
    return;
}

//Not quite completed but installed for testing and general use. 8-5-08 -Taon
void do_deathroll( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   dam;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "Do the deathroll on who?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "You can't do a deathroll on yourself...\r\n", ch );
        return;
    }
    if ( !IS_AFFECTED( ch, AFF_FLYING ) || ch->in_room->sector_type == SECT_UNDERWATER ) {
        send_to_char( "You must either be flying, or underwater to properly preform this task.\r\n",
                      ch );
        return;
    }
    if ( victim->position == POS_SITTING || victim->position == POS_RESTING
         || victim->position == POS_SLEEPING ) {
        send_to_char( "They're already to close to the ground.", ch );
        return;
    }
    if ( ch->move < 30 ) {
        send_to_char( "You're too weak to do such a task.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_deathroll ) ) {
        WAIT_STATE( ch, skill_table[gsn_deathroll]->beats );
        act( AT_RED,
             "You dive toward $N snatching them up with your powerful jaws, putting them into a deathroll!",
             ch, NULL, victim, TO_CHAR );
        act( AT_RED,
             "A wavery shadow appears overhead in the water as $n comes crashing down at you, snatching you with their jaws, sending you into a deathroll!",
             ch, NULL, victim, TO_VICT );
        act( AT_RED,
             "$n dives down upon $N snatching them with their jaws, sending them into a death roll!",
             ch, NULL, victim, TO_NOTVICT );

        if ( ch->in_room->sector_type == SECT_UNDERWATER
             || ch->in_room->sector_type == SECT_OCEAN
             || ch->in_room->sector_type == SECT_WATERFALL
             || ch->in_room->sector_type == SECT_RIVER
             || ch->in_room->sector_type == SECT_LAKE
             || ch->in_room->sector_type == SECT_DOCK || ch->in_room->sector_type == SECT_WATER_SWIM
             || ch->in_room->sector_type == SECT_WATER_NOSWIM
             || ch->in_room->sector_type == SECT_SWAMP ) {
            act( AT_RED, "You drag $N deeper into the water!", ch, NULL, victim, TO_CHAR );
            act( AT_RED, "$n drags you deeper into the water, before releasing.", ch, NULL, victim,
                 TO_VICT );
            send_to_char( "&cYou panic and swallow a lot of water causing you to choke.\r\n&D",
                          victim );
            dam = number_chance( 1, 5 );
            victim->hit -= dam;
            victim->move -= dam;
        }
        else {
            act( AT_RED, "You slam $N into the ground!", ch, NULL, victim, TO_CHAR );
            act( AT_RED, "$n throws you from their jaws to the ground!", ch, NULL, victim,
                 TO_VICT );
            send_to_char( "&OYou slam into the ground, leaving you gasping for breath.\r\n&D",
                          victim );
            victim->position = POS_SITTING;

            if ( IS_AFFECTED( victim, AFF_FLYING ) ) {
                xREMOVE_BIT( victim->affected_by, AFF_FLYING );
                affect_strip( victim, gsn_wings );
            }
            dam = number_chance( 5, 10 );
            victim->hit -= dam;
            victim->move -= dam;
            update_pos( victim );
        }

        ch->move -= 30;
        learn_from_success( ch, gsn_deathroll );
        global_retcode = damage( ch, victim, extrahigh, gsn_deathroll );
    }
    else {
        ch->move -= 15;
        learn_from_failure( ch, gsn_deathroll );
        global_retcode = damage( ch, victim, 0, gsn_deathroll );
        send_to_char( "Your dive was off allowing your foe to dodge your attempt.\r\n", ch );
    }
    return;
}

//Skill hellfire for red dragons, nearly complete. -Taon
void do_hellfire( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim = NULL;
    char                    arg[MIL];
    short                   dam,
                            chance;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Do hellfire inferno on who?\r\n", ch );
        return;
    }

    if ( VLD_STR( arg ) )
        victim = get_char_room( ch, arg );
    else if ( ch->fighting )
        victim = who_fighting( ch );

    if ( !victim ) {
        send_to_char( "You must specify a target or be fighting to use this skill.", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "You can't do a hellfire inferno on yourself...\r\n", ch );
        return;
    }

    if ( ch->move < 50 || ch->mana < 50 ) {
        send_to_char( "You're too weak to do such a task.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_hellfire ) ) {
        WAIT_STATE( ch, skill_table[gsn_hellfire]->beats );
        act( AT_RED, "You open you massive jaws and release an intense inferno towards $N!", ch,
             NULL, victim, TO_CHAR );
        act( AT_RED, "$n opens their massive jaws and releases an intense inferno toward you.", ch,
             NULL, victim, TO_VICT );
        act( AT_RED, "$n opens their massive jaws releasing an intense inferno toward $N.", ch,
             NULL, victim, TO_NOTVICT );

        // Give Balrog remort a little advantage here. -Taon
        if ( ch->Class != CLASS_BALROG )
            chance = number_chance( 1, 10 );
        else
            chance = number_chance( 1, 15 );

        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && chance > 8 ) {
            act( AT_RED, "You burst through $N's shield of sanctuary, causing it to falter!", ch,
                 NULL, victim, TO_CHAR );
            act( AT_RED, "$n burst through your sanctuary, causing it to falter.", ch, NULL, victim,
                 TO_VICT );
            xREMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
            affect_strip( victim, gsn_sanctuary );
        }

        ch->move -= 50;
        if ( ch->Class != CLASS_BALROG )
            ch->mana -= 50;
        else
            ch->blood -= 50;
        learn_from_success( ch, gsn_hellfire );

        // Balrogs get this spell at a higher level, giving them
        // a stage up damage bonus. - Taon
        if ( ch->Class != CLASS_BALROG )
            global_retcode = damage( ch, victim, high, gsn_hellfire );
        else
            global_retcode = damage( ch, victim, extrahigh, gsn_hellfire );
    }
    else {
        ch->move -= 25;

        if ( ch->Class != CLASS_BALROG )
            ch->mana -= 25;
        else
            ch->blood -= 25;
        learn_from_failure( ch, gsn_hellfire );
        global_retcode = damage( ch, victim, 0, gsn_hellfire );
        send_to_char( "You was unable to properly invoke an inferno.\r\n", ch );
    }
}

void do_devour( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "You are concentrating too much for that!\r\n", ch );
        return;
    }
    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "You can only dream of devouring corpses while sleeping\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "Devour which corpse?\r\n", ch );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;

    obj = get_obj_list_rev( ch, arg, ch->in_room->last_content );
    if ( !obj ) {
        send_to_char( "You can't find it.\r\n", ch );
        return;
    }

    if ( obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_COOK ) {
        send_to_char( "You cannot devour that.\r\n", ch );
        return;
    }

    separate_obj( obj );
    extract_obj( obj );

    if ( can_use_skill( ch, number_percent(  ), gsn_devour ) ) {
        act( AT_RED, "You greedily devour $p in one bite.", ch, obj, NULL, TO_CHAR );
        act( AT_RED, "$n swoops down and devours $p completely in one bite!", ch, obj, NULL,
             TO_ROOM );

        if ( obj->item_type == ITEM_COOK ) {
            ch->pcdata->condition[COND_THIRST] += 1;
            ch->pcdata->condition[COND_FULL] += 1;
        }
        else {
            if ( ch->pcdata->condition[COND_THIRST] < 20 ) {
                ch->pcdata->condition[COND_THIRST] = 20;
                ch->pcdata->condition[COND_FULL] = 20;
            }
            if ( ch->pcdata->condition[COND_THIRST] > 19 ) {
                ch->pcdata->condition[COND_THIRST] += 2;
                ch->pcdata->condition[COND_FULL] += 2;
            }
            if ( ch->pcdata->condition[COND_THIRST] > 40 ) {
                ch->pcdata->condition[COND_THIRST] = 30;
                ch->pcdata->condition[COND_FULL] = 30;
            }
        }
        learn_from_success( ch, gsn_devour );
    }
    else {
        send_to_char( "You try to devour, but the corpse was too rancid for you.\r\n", ch );
        learn_from_failure( ch, gsn_devour );
    }
    return;
}

void do_tail_swipe( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   chance;

    chance = number_range( 1, 10 );

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    /*
     * Lets slow down how often an NPC does this attack lol its murder on players, I think
     * half the time is good 
     */
    if ( IS_NPC( ch ) && chance <= 8 )
        return;

    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "Do a tail swipe on who?\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_tail_swipe]->beats );

    if ( can_use_skill( ch, ( number_percent(  ) ), gsn_tail_swipe ) ) {
        if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
            WAIT_STATE( victim, PULSE_VIOLENCE );
            act( AT_WHITE, "$n swipes you with $s tail.", ch, NULL, victim, TO_VICT );
            act( AT_WHITE, "You swipe $N with your tail.", ch, NULL, victim, TO_CHAR );
            act( AT_WHITE, "$n swipes $N with $s tail.", ch, NULL, victim, TO_NOTVICT );
            af.type = gsn_tail_swipe;
            af.location = APPLY_AC;
            af.modifier = 20;
            af.duration = 1;
            af.level = ch->level;
            af.bitvector = meb( AFF_PARALYSIS );
            affect_to_char( victim, &af );
            update_pos( victim );
            learn_from_success( ch, gsn_tail_swipe );
            global_retcode = damage( ch, victim, number_chance( 5, 20 ), gsn_tail_swipe );
        }
    }
    else {
        WAIT_STATE( ch, PULSE_VIOLENCE );
        if ( !IS_NPC( ch ) )
            learn_from_failure( ch, gsn_tail_swipe );
        act( AT_WHITE, "$n swipes their tail at you, but you dodge it.", ch, NULL, victim,
             TO_VICT );
        act( AT_WHITE, "You try to tail swipe $N, but $E dodges out of the way.", ch, NULL, victim,
             TO_CHAR );
        act( AT_WHITE, "$n swipes their tail at $N, but misses them narrowly.", ch, NULL, victim,
             TO_NOTVICT );
    }
}

void do_rage( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    if ( is_affected( ch, gsn_rage ) ) {
        send_to_char( "You are already in a state of rage.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_rage ) ) {
        WAIT_STATE( ch, skill_table[gsn_rage]->beats );
        af.type = gsn_rage;
        af.duration = ch->level + 10;
        af.location = APPLY_HITROLL;
        af.modifier = 2 + ( ch->level / 17 );
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( ch, &af );

        af.type = gsn_rage;
        af.duration = ch->level + 10;
        af.location = APPLY_DAMROLL;
        af.level = ch->level;
        af.modifier = 2 + ( ch->level / 17 );
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

        af.type = gsn_rage;
        af.duration = ch->level + 10;
        af.location = APPLY_STR;
        af.level = ch->level;
        af.modifier = 2 + ( ch->level / 25 );
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

        act( AT_RED, "You harness the power of dragon rage and feel its strength rush into you!",
             ch, NULL, NULL, TO_CHAR );
        act( AT_RED, "$n begins shaking with an uncontrollable rage!", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_rage );
        return;
    }
    else
        learn_from_failure( ch, gsn_rage );
    send_to_char( "You try to bring forth the rage, but cannot summon it.\r\n", ch );
    return;
}

void do_truesight( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't concentrate enough for that.\r\n", ch );
        return;
    }

    affect_strip( ch, gsn_truesight );

    WAIT_STATE( ch, skill_table[gsn_truesight]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_truesight ) ) {
        af.type = gsn_truesight;
        af.duration = ch->level * 3 + 25;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_TRUESIGHT );
        affect_to_char( ch, &af );
        send_to_char( "Your eyes glow as you focus your enhanced sight.\r\n", ch );
        learn_from_success( ch, gsn_truesight );
        return;
    }
    else
        learn_from_failure( ch, gsn_truesight );
    send_to_char( "Your vision blurs, as you fail to use your enhanced sight.\r\n", ch );
    return;
}

void do_fly_home( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *sroom;
    int                     rvnum = 42081;
    CHAR_DATA              *opponent;

    if ( ch->move < 50 ) {
        send_to_char( "You need to rest before you can fly home.\r\n", ch );
        return;
    }

    if ( !( sroom = get_room_index( rvnum ) ) ) {
        send_to_char( "Can't find the room to send you to if you fly home.\r\n", ch );
        bug( "%s: failed to find room vnum %d.", __FUNCTION__, rvnum );
        return;
    }

    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
        do_wings( ch, ( char * ) "" );
        send_to_char( "\r\n", ch );
    }

    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You failed to get off the ground, first you must be flying.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
        send_to_char( "!!SOUND(sound/wings.wav)\r\n", ch );

    if ( ( opponent = who_fighting( ch ) ) != NULL ) {
        int                     lose;

        if ( number_bits( 1 ) == 0 || ( !IS_NPC( opponent ) && number_bits( 3 ) > 1 ) ) {
            WAIT_STATE( ch, 4 );
            lose =
                ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );

            if ( ch->desc )
                lose /= 2;
            gain_exp( ch, 0 - lose );
            send_to_char( "You failed to fly away quick enough! You lose experience.\r\n", ch );
            return;
        }
        lose = ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );

        if ( ch->desc )
            lose /= 2;
        gain_exp( ch, 0 - lose );
        send_to_char( "You flew away from combat! You lose experience.\r\n", ch );
        stop_fighting( ch, TRUE );
    }

    if ( ch->in_room->sector_type == SECT_INSIDE || ch->in_room->sector_type == SECT_UNDERGROUND
         || IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
        if ( ch->mana < 50 ) {
            send_to_char( "You need to gain more mana before you can breech the void.\r\n", ch );
            return;
        }
        send_to_char( "You breach the void between time and space, and reappear in the skies.\r\n",
                      ch );
    }

    WAIT_STATE( ch, skill_table[gsn_fly_home]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_fly_home ) ) {
        act( AT_WHITE, "You fly higher into the sky, and head for home.", ch, NULL, NULL, TO_CHAR );
        if ( ch->in_room->sector_type == SECT_INSIDE || ch->in_room->sector_type == SECT_UNDERGROUND
             || IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
            act( AT_WHITE, "$n breechs the void between time and space, and disappears.\r\n", ch,
                 NULL, NULL, TO_ROOM );
            ch->mana = ch->mana - 50;
        }
        else {
            act( AT_WHITE, "$n soars up into the sky, and flies off into the horizon.\r\n", ch,
                 NULL, NULL, TO_ROOM );
        }
        char_from_room( ch );
        char_to_room( ch, sroom );
        act( AT_RMNAME, "Within the Sky", ch, NULL, NULL, TO_CHAR );
        act( AT_RMDESC,
             "&CThe clear open sky shows the vastness of the ground below,\r\nand sky around.  Landmarks that cannot be seen on the ground\r\nare easily viewed from the sky.",
             ch, NULL, NULL, TO_CHAR );
        act( AT_EXITS, "Exits: north east south west", ch, NULL, NULL, TO_CHAR );
        send_to_char( "\r\n&cYou glide down to the ground.\r\r\n\n", ch );
        /*
         * Moved the below to just after the land command, so that it shows the correct
         * room landing in, instead of a random wilderness map - Aurin 9/19/2010
         * do_look(ch, (char *)""); 
         */
        if ( ch->pcdata->lair ) {
            send_to_char
                ( "\r\nYou breach the void between time and space, and reappear in the skies.\r\n\r\n",
                  ch );
            sroom = get_room_index( ch->pcdata->lair );
            char_from_room( ch );
            char_to_room( ch, sroom );
            save_char_obj( ch );
        }
        do_land( ch, ( char * ) "" );
        do_look( ch, ( char * ) "" );                  /* Aurin 9/19/2010 */
        ch->move = ch->move - 50;
        learn_from_success( ch, gsn_fly_home );
        return;
    }
    else {
        send_to_char( "You fail to breech the void between time and space.\r\n", ch );
        learn_from_failure( ch, gsn_fly_home );
        return;
    }
}

void do_pluck( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "You can't pluck someone from the ground, you should be flying first.\r\n",
                      ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "You must be fighting in order to pluck.\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "Why in the world would you want to do that?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_pluck ) ) {
        WAIT_STATE( ch, skill_table[gsn_pluck]->beats );
        act( AT_RED, "You fly downward and pluck up $N in your claws!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n dived from the sky and plucked you up in mighty claws!", ch, NULL, victim,
             TO_VICT );
        act( AT_RED, "$n dived from the sky and plucked up $N in mighty claws!", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, mediumhigh, gsn_pluck );
        learn_from_success( ch, gsn_pluck );
        if ( victim->position != POS_DEAD ) {
            act( AT_WHITE, "You soar high into air with $N!", ch, NULL, victim, TO_CHAR );
            act( AT_WHITE, "$n soars high into the air taking you way off the ground!", ch, NULL,
                 victim, TO_VICT );
            act( AT_WHITE, "$n soars high into the air with $N!", ch, NULL, victim, TO_NOTVICT );
            act( AT_RED, "You release your grip on $N and let them fall to the ground!", ch, NULL,
                 victim, TO_CHAR );
            act( AT_RED, "$n releasing grip causes you to fall to the ground!", ch, NULL, victim,
                 TO_VICT );
            act( AT_RED, "$n drops $N and they fall from the sky to the ground!", ch, NULL, victim,
                 TO_NOTVICT );
            global_retcode = damage( ch, victim, mediumhigh, gsn_drop );
            return;
        }
    }
    else {
        learn_from_failure( ch, gsn_pluck );
        global_retcode = damage( ch, victim, 0, gsn_pluck );
        act( AT_RED, "You try to dive down, and pluck up $N, but miss your target!", ch, NULL,
             victim, TO_CHAR );
    }
    return;
}

void do_snatch( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim = NULL;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return;
    }

    if ( VLD_STR( arg ) )
        victim = get_char_room( ch, arg );
    else if ( ch->fighting )
        victim = who_fighting( ch );

    if ( !victim ) {
        send_to_char( "You must specify who you want to snatch or be fighting someone.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Why in the world would you want to do that?\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_snatch ) ) {
        WAIT_STATE( ch, skill_table[gsn_snatch]->beats );
        act( AT_RED, "You reach out and snatch $N with your jaws!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n has snatched you up, mauling you in powerful jaws!", ch, NULL, victim,
             TO_VICT );
        act( AT_RED, "$n reaches out and snatches up $N in powerful jaws!", ch, NULL, victim,
             TO_NOTVICT );
        learn_from_success( ch, gsn_snatch );
        global_retcode = damage( ch, victim, low, gsn_snatch );
    }
    else {
        learn_from_failure( ch, gsn_snatch );
        global_retcode = damage( ch, victim, 0, gsn_snatch );
        send_to_char( "You try to snatch, but they dodge your attempt.\r\n", ch );
    }
    return;
}

void do_impale( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    // OBJ_DATA *obj;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Impale whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim->fighting ) {
        send_to_char( "You can't impale someone who is in combat.\r\n", ch );
        return;
    }

    percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );

    check_attacker( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_impale]->beats );
    if ( !IS_AWAKE( victim ) || can_use_skill( ch, percent, gsn_impale ) ) {

        if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
            send_to_char( "!!SOUND(sound/backstab1.wav)\r\n", ch );

        learn_from_success( ch, gsn_impale );
/* Volk: Insane damage is good. Want to give added damage though
 * for things like if the mob can't see you, if you're sneaking etc */
        if ( can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 )
            global_retcode = damage( ch, victim, insane, gsn_impale );

        else if ( can_see( victim, ch ) && victim->hit < victim->max_hit / 2 ) {
            global_retcode = damage( ch, victim, high, gsn_impale );
        }
        else if ( !can_see( victim, ch ) && victim->hit < victim->max_hit / 2 ) {
            send_to_char( "&RYour victim doesn't even see you coming!&w\r\n", ch );
            global_retcode = damage( ch, victim, high, gsn_impale );
        }
        else if ( !can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 ) {
            send_to_char( "&RYour victim doesn't even see you coming!&w\r\n", ch );
            global_retcode = damage( ch, victim, maximum, gsn_impale );
        }

        check_illegal_pk( ch, victim );
    }
    else {
        learn_from_failure( ch, gsn_impale );
        send_to_char( "&cYour impale just misses your intended victim.\r\n", ch );
        check_illegal_pk( ch, victim );
    }
    return;
}

 // Nearly complete, just a few adjustments here and there and some
 // more in depth support throughout the code and its finished. Completed
 // enough I've already got it in play. -Taon
ch_ret spell_flaming_shield( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return rNONE;
    }
    if ( IS_AFFECTED( ch, AFF_FLAMING_SHIELD ) ) {
        send_to_char( "You're already surrounded by a shield of flames.\r\n", ch );
        return rNONE;
    }

    send_to_char( "&rYou are surrounded by a shield of flames.&d\r\n", ch );
    act( AT_MAGIC, "$n is suddenly surrounded by a shield of flames.", ch, NULL, NULL, TO_ROOM );
    af.type = gsn_flaming_shield;
    af.duration = ch->level * 10;
    af.level = ch->level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = meb( AFF_FLAMING_SHIELD );
    affect_to_char( ch, &af );

    return rNONE;
}

//Simple spell made with low level in mind, for black dragons. -Taon
ch_ret spell_acidic_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    int                     dam;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return rNONE;
    }

    if ( !IS_NPC( victim ) )
        ch_printf( ch, "Acidic materials collect at the tips of your fingers as you touch %s.\r\n",
                   victim->name );
    else
        ch_printf( ch, "Acidic materials collect at the tips of your fingers as you touch %s.\r\n",
                   capitalize( victim->short_descr ) );

    if ( ch->level < 50 )
        dam = low;
    else
        dam = medium;

    if ( !IS_AFFECTED( victim, AFF_ACIDIC_TOUCH ) ) {
        send_to_char( "You cause your target to shiver in pain.\r\n", ch );
        send_to_char( "You causes you to shiver in pain.\r\n", victim );
        af.type = gsn_acidic_touch;
        af.duration = ch->level * 10;
        af.level = ch->level;
        af.location = APPLY_STR;

        if ( ch->level < 25 )
            af.modifier = -1;
        else if ( ch->level < 50 )
            af.modifier = -2;
        else if ( ch->level < 75 )
            af.modifier = -3;
        else
            af.modifier = -4;

        af.bitvector = meb( AFF_ACIDIC_TOUCH );
        affect_to_char( victim, &af );
    }

    return damage( ch, victim, dam, sn );
}

//Spell for Golden dragons. -Taon
ch_ret spell_divine_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_BLINDNESS ) ) {
        send_to_char( "They're already blinded..\r\n", ch );
        return rNONE;
    }
    if ( ch == victim ) {
        send_to_char( "You can't cast this on yourself.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = 2;
    else
        af.duration = 4;

    af.bitvector = meb( AFF_BLINDNESS );
    affect_to_char( victim, &af );

    if ( !IS_NPC( victim ) )
        ch_printf( ch, "You summon a divine light which blinds %s.\r\n", victim->name );
    else
        ch_printf( ch, "You summon a divine light which blinds %s.\r\n", victim->short_descr );

    ch_printf( victim, "%s summons a divine light which blinds you.\r\n", ch->name );

    return rNONE;
}

//Spell for lower level golden dragons. -Taon
ch_ret spell_lend_health( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( ch->fighting ) {
        send_to_char( "Not while fighting.\r\n", ch );
        return rNONE;
    }
    if ( victim->hit >= victim->max_hit ) {
        send_to_char( "They dont need the healing.\r\n", ch );
        return rNONE;
    }
    if ( ch->hit <= ch->level ) {
        send_to_char( "You dont have enough life to lend.\r\n", ch );
        return rNONE;
    }

    if ( !IS_NPC( victim ) )
        ch_printf( ch, "You lend health to %s.\r\n", victim->name );
    else
        ch_printf( ch, "You lend health to %s.\r\n", victim->short_descr );

    ch_printf( victim, "%s lends health to you.\r\n", ch->name );

    victim->hit += ch->level;
    ch->hit -= ch->level;

    if ( victim->hit > victim->max_hit )
        victim->hit = victim->max_hit;

    return rNONE;
}

//Completed on 8-11-08 by: Taon.
ch_ret spell_shadow_bolt( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    short                   align_mod;
    int                     dam;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Your new form prevents the use of this skill.\r\n", ch );
        return rNONE;
    }

    if ( ch->move < 25 ) {
        send_to_char( "You dont have enough move points to accomplish such a task.\r\n", ch );
        return rNONE;
    }

    if ( ch->alignment <= -300 )
        dam = maximum;
    else if ( ch->alignment > -300 )
        dam = extrahigh;

    align_mod = number_chance( 1, 20 );
    victim->alignment -= align_mod;
    ch->move -= 25;

    if ( !IS_NPC( victim ) )
        ch_printf( ch, "&zYou summon a bolt formed of dark energies to attack %s.\r\n",
                   victim->name );
    else
        ch_printf( ch, "&zYou summon a bolt formed of dark energies to attack %s.\r\n",
                   capitalize( victim->short_descr ) );

    return damage( ch, victim, dam, sn );
}

//Spell completed on 8-9-08 by: Taon.
ch_ret spell_aura_of_life( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_AURA_LIFE ) ) {
        send_to_char( "You already have an aura of life surrounding your being.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = ch->level * 2;
    else
        af.duration = ch->level * 4;

    af.bitvector = meb( AFF_AURA_LIFE );
    affect_to_char( ch, &af );

    act( AT_MAGIC, "$n is suddenly surrounded by a glowing aura.", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "An aura of healing energy collects around you.", ch, NULL, NULL, TO_CHAR );

    return rNONE;

}

//Divine intervention for high level Gold dragons. -Taon
ch_ret spell_divine_intervention( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( ch->fighting ) {
        send_to_char( "Not while fighting.\r\n", ch );
        return rNONE;
    }
    if ( IS_AFFECTED( ch, AFF_DIVINE_INTERVENTION ) ) {
        send_to_char( "You can't cast this again, yet..\r\n", ch );
        return rNONE;
    }
    if ( ch->alignment < 300 ) {
        send_to_char( "Your alignment isn't good enough to invoke this spell.\r\n", ch );
        return rNONE;
    }
    if ( ch->hit >= ch->max_hit ) {
        send_to_char( "You dont need the healing.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;
    af.duration = 1000;
    af.bitvector = meb( AFF_DIVINE_INTERVENTION );
    affect_to_char( ch, &af );

    act( AT_MAGIC, "$n glows brightly, as their wounds rapidly heal.", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "You glow brightly as your wounds rapidly heal..", ch, NULL, NULL, TO_CHAR );
    ch->hit = ch->max_hit;
    return rNONE;

}

ch_ret spell_healing_essence( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_REACTIVE ) ) {
        send_to_char( "You already have the healing essence cast.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = ch->level + 30;
    else
        af.duration = ch->level + 50;

    af.bitvector = meb( AFF_REACTIVE );
    affect_to_char( ch, &af );

    act( AT_MAGIC, "$n roars, 'GarGacha!", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "You roar, 'GarGacha!", ch, NULL, NULL, TO_CHAR );

    return rNONE;
}

//Spell firestorm was derived from earthquake, by Taon.
ch_ret spell_firestorm( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    act( AT_MAGIC, "You release a burst of flames into the air, damaging everyone around you!", ch,
         NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n releases a burst of intense flames.", ch, NULL, NULL, TO_ROOM );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;

        if ( !vch )
            break;
        if ( vch == ch )
            continue;
        if ( !vch->in_room )
            continue;
        if ( is_same_group( vch, ch ) )
            continue;

        if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
            continue;

        if ( vch->in_room == ch->in_room ) {
            ch_printf( vch, "&R%s engulfs you in flames.&d\r\n", ch->name );
            global_retcode = damage( ch, vch, extrahigh, sn );
        }
    }
    return rNONE;
}

//Spell death field for black dragons. -Taon
//Not quite finished, but installed for testing.
ch_ret spell_death_field( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );
    ROOM_INDEX_DATA        *location;
    short                   chance;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }

    act( AT_MAGIC, "You begin to utter the words of the unspoken!", ch, NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n begins to chant the words of the unspoken.", ch, NULL, NULL, TO_ROOM );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;
        chance = number_chance( 1, 100 );

        if ( !vch || !vch->next )
            break;
        if ( vch == ch )
            continue;
        if ( !vch->in_room )
            continue;
        if ( is_same_group( vch, ch ) )
            continue;

        if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
            continue;

        if ( vch->in_room == ch->in_room ) {
            if ( ch->level - vch->level >= 10 && ch->alignment <= -500 ) {
                if ( chance < 5 ) {
                    location = get_room_index( ROOM_VNUM_TEMPLE );
                    ch_printf( vch, "You immediatly succumb to &R%s attack.&d\r\n", ch->name );
                    ch_printf( ch, "%s immediatly succumbs to your attacks.&d\r\n", vch->name );
                    vch->hit = 1;
                    stop_fighting( vch, TRUE );

                    if ( !IS_NPC( vch ) ) {
                        char_from_room( vch );
                        char_to_room( vch, location );
                    }
                }
                else if ( chance < 10 ) {
                    if ( !IS_NPC( ch ) && ch->pcdata->lair )
                        location = get_room_index( ch->pcdata->lair );
                    if ( !location )
                        location = get_room_index( ROOM_VNUM_TEMPLE );

                    ch_printf( ch,
                               "You fail to properly summon your dark powers, leaving you drained.&R%s.&d\r\n",
                               ch->name );
                    ch->hit = 1;
                    ch->move = 1;
                    ch->mana = 1;
                    ch->alignment += 50;
                    stop_fighting( ch, TRUE );

                    if ( !IS_NPC( ch ) ) {
                        char_from_room( ch );
                        char_to_room( ch, location );
                    }
                }
                else {
                    ch_printf( vch,
                               "&z%s invades your mental barriers, causing you massive pain.&d\r\n",
                               ch->name );
                    ch_printf( ch, "%s glows brightly.\r\n", vch->name );
                    global_retcode = damage( ch, vch, extrahigh, sn );
                }
            }
            else {
                ch_printf( vch,
                           "&z%s makes a feeble attempt to bypass your mental barriers, causing some pain.&d\r\n",
                           ch->name );
                ch_printf( ch, "%s glows dimly.\r\n", vch->name );
                global_retcode = damage( ch, vch, high, sn );
            }
        }
    }
    return rNONE;
}

//Spell Clavus was derived from earthquake by Taon.
ch_ret spell_clavus( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    if ( ch->move < 50 ) {
        send_to_char( "You dont have the required movement to do such a task.\r\n", ch );
        return rSPELL_FAILED;
    }

    act( AT_MAGIC, "You scream in fury as you release dozens of metallic spikes into the air.", ch,
         NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n screams in fury as they release dozens of metallic spikes into the air.", ch,
         NULL, NULL, TO_ROOM );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;

        if ( !vch )
            break;
        if ( vch == ch )
            continue;
        if ( !vch->in_room )
            continue;
        if ( IS_AFFECTED( vch, AFF_FLYING ) )
            continue;
        if ( is_same_group( vch, ch ) )
            continue;

        if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
            continue;

        if ( IS_IMMORTAL( vch ) && vch->in_room ) {
            ch_printf( vch, "You stand with resolve as you cannot be struck by %s's spikes.\r\n",
                       ch->name );
            continue;
        }

        if ( vch->in_room == ch->in_room ) {
            ch_printf( vch, "&zYou're struck by one of %s's metallic spikes.&d\r\n", ch->name );
            global_retcode = damage( ch, vch, extrahigh, sn );
        }

    }

    ch->move -= 50;
    return rNONE;
}
