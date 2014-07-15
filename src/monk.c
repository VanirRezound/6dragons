/****************************************************************************
*Status: Incomplete.  Est: 95%                                              *
*                                                                           *
*Author: Dustan Gunn "Taon"                                                 *
*Author's Email: Itztaon@aol.com                                            *
*Code Information: The home of most of the monk class skills.               *
*Other: This was wrote for use on 6dragons mud. This isn't the final        *
*header, simply temporary until I complete it.                              *
*                                                                           * 
*****************************************************************************
*                        MONK CLASS MODULE                                  *
****************************************************************************/

#include "h/mud.h"

//This tenuous function handles all focus_level adjustments. -Taon
void adjust_focus( CHAR_DATA *ch, short adjustment )
{
    if ( IS_NPC( ch ) )
        return;

    if ( ch->pcdata->learned[gsn_focus] > 0 ) {
        ch->focus_level += adjustment;

        // check min/max dont surpass either. -Taon
        if ( ch->focus_level > 50 )
            ch->focus_level = 50;
        if ( ch->focus_level < 0 )
            ch->focus_level = 0;
    }
    return;
}

//Status: Completed skill.. -Taon
void do_defensive_posturing( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    if ( IS_AFFECTED( ch, AFF_BERSERK ) || IS_AFFECTED( ch, AFF_FURY ) ) {
        send_to_char( "You're too furious to bring yourself into a defensive posture.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_DEFENSIVE_POSTURING ) ) {
        send_to_char( "You're already in a defensive posture.\r\n", ch );
        return;
    }
    if ( ch->move < 60 - get_curr_dex( ch ) ) {
        send_to_char( "You don't have enough move.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_defensive_posturing]->beats * 2 );
    if ( can_use_skill( ch, number_percent(  ), gsn_defensive_posturing ) ) {
        ch->move -= 60 - get_curr_dex( ch );

        af.type = gsn_defensive_posturing;
        af.location = APPLY_AC;

        if ( ch->pcdata->learned[gsn_defensive_posturing] < 25 )
            af.modifier = -10;
        else if ( ch->pcdata->learned[gsn_defensive_posturing] < 50 )
            af.modifier = -25;
        else if ( ch->pcdata->learned[gsn_defensive_posturing] < 75 )
            af.modifier = -30;
        else if ( ch->pcdata->learned[gsn_defensive_posturing] < 85 )
            af.modifier = -35;
        else
            af.modifier = -40;

        af.bitvector = meb( AFF_DEFENSIVE_POSTURING );
        af.level = ch->level;

        if ( get_curr_dex( ch ) < 15 )
            af.duration = ch->level * 3;
        else if ( get_curr_dex( ch ) < 20 )
            af.duration = ch->level * 4;
        else if ( get_curr_dex( ch ) < 25 )
            af.duration = ch->level * 5;
        else
            af.duration = ch->level * 6;

        affect_to_char( ch, &af );
        send_to_char( "You take up a strong defensive posture.\r\n", ch );

        if ( number_chance( 1, 5 ) == 2 )
            learn_from_success( ch, gsn_defensive_posturing );
    }
    else {
        send_to_char( "You fail to take up a decent defensive posture.\r\n", ch );
        learn_from_failure( ch, gsn_defensive_posturing );
    }
    return;
}

//Was having issues with the smaug_spell version of it, so I just
//wrote the skill. Simple enough. -Taon
//Status: Completed skill
void do_keen_eye( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    if ( ch->pcdata->learned[gsn_keen_eye] <= 0 ) {
        send_to_char( "You wouldn't know where to start!\r\n", ch );
        return;
    }
    if ( ch->position != POS_STANDING ) {
        send_to_char( "You must be standing in order to use this.\r\n", ch );
        return;
    }
    if ( ch->move < 25 ) {
        send_to_char( "You dont have enough energy.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_KEEN_EYE ) ) {
        send_to_char( "Your eyes already see unseen things.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_keen_eye]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_keen_eye ) ) {
        af.type = gsn_keen_eye;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.duration = ch->level * 5;
        af.bitvector = meb( AFF_DETECT_HIDDEN );
        af.level = ch->level;
        affect_to_char( ch, &af );

        af.type = gsn_keen_eye;
        af.location = APPLY_HITROLL;
        af.modifier = get_curr_str( ch ) / 5;
        af.duration = ch->level * 5;
        af.bitvector = meb( AFF_KEEN_EYE );
        af.level = ch->level;
        affect_to_char( ch, &af );
        send_to_char( "&cYour keen eyes take in things others would not notice.\r\n", ch );
        ch->move -= 25;
        learn_from_success( ch, gsn_keen_eye );
        return;
    }
    else
        learn_from_failure( ch, gsn_keen_eye );
    send_to_char( "&cYour vision blurs, as you attempt to use your keen eyes.\r\n", ch );
    return;
}

//Status: Completed skill.
void do_boost( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    short                   take_move;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You cant move well enough to perform such a task.", ch );
        return;
    }
    if ( ch->pcdata->learned[gsn_boost] <= 0 ) {
        send_to_char( "You wouldn't know where to begin!\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BOOST ) ) {
        send_to_char( "You're already as fast as lightning.\r\n", ch );
        return;
    }

    if ( ch->pcdata->learned[gsn_boost] < 50 )
        take_move = 50;
    else if ( ch->pcdata->learned[gsn_boost] < 80 )
        take_move = 35;
    else
        take_move = 20;

    if ( ch->move < take_move ) {
        send_to_char( "You don't have enough energy to boost yourself.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_boost]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_boost ) ) {
        act( AT_YELLOW, "Your body jolts with great haste.", ch, NULL, NULL, TO_CHAR );
        af.type = gsn_boost;

        if ( ch->level < 20 )
            af.duration = ch->level + ch->pcdata->learned[gsn_boost] + 50;
        else if ( ch->level < 40 )
            af.duration = ( ch->level * 2 ) + ch->pcdata->learned[gsn_boost] + 75;
        else if ( ch->level < 60 )
            af.duration = ( ch->level * 3 ) + ch->pcdata->learned[gsn_boost] + 100;
        else if ( ch->level < 90 )
            af.duration = ( ch->level * 4 ) + ( ch->pcdata->learned[gsn_boost] * 3 );
        else
            af.duration = ( ch->level * 5 ) + ( ch->pcdata->learned[gsn_boost] * 4 );

        af.location = APPLY_DEX;

        if ( get_curr_dex( ch ) < 15 )
            af.modifier = 1;
        else if ( get_curr_dex( ch ) < 20 )
            af.modifier = 2;
        else if ( get_curr_dex( ch ) < 25 )
            af.modifier = 3;
        else
            af.modifier = 4;

        af.level = ch->level;
        af.bitvector = meb( AFF_BOOST );
        affect_to_char( ch, &af );
        ch->move -= take_move;
        learn_from_success( ch, gsn_boost );
        return;
    }
    else
        act( AT_CYAN, "You fail to jolt your body with great haste.", ch, NULL, NULL, TO_CHAR );
    learn_from_failure( ch, gsn_boost );
    return;
}

//Status: complete. -Taon
//Recently converted so that Dragons can use it as Iron Scales as-well. -Taon
void do_iron_skin( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    short                   chance;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "But you're charmed...\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_IRON_SKIN ) ) {
        send_to_char( "You're already as tough as iron.\r\n", ch );
        return;
    }
    if ( ch->position == POS_FIGHTING ) {
        send_to_char( "You can't perform such a task while in combat.", ch );
        return;
    }
    if ( ch->move < 40 - get_curr_dex( ch ) ) {
        send_to_char( "You're too tired to do such a thing.\r\n", ch );
        return;
    }

    chance =
        ( get_curr_dex( ch ) / 2 ) + ch->pcdata->learned[gsn_iron_skin] + number_range( 1, 30 );

    if ( ch->pcdata->learned[gsn_iron_skin] > 0 ) {
        ch->move -= 40 - get_curr_dex( ch );

        if ( chance >= number_range( 50, 100 ) ) {
            if ( ch->race != RACE_DRAGON )
                send_to_char( "Your skin turns as hard as iron.\r\n", ch );
            else
                send_to_char( "Your scales turns as hard as iron.\r\n", ch );

            af.type = gsn_iron_skin;

            if ( ch->level < 30 )
                af.duration = ( ch->level * 3 ) + ( get_curr_wis( ch ) * 5 );
            else
                af.duration = ( ch->level * 4 ) + ( get_curr_wis( ch ) * 6 );

            af.level = ch->level;
            af.location = APPLY_AC;

            if ( ch->level < 25 )
                af.modifier = -25;
            else if ( ch->level < 50 )
                af.modifier = -35;
            else if ( ch->level < 75 )
                af.modifier = -45;
            else
                af.modifier = -55;

            af.bitvector = meb( AFF_IRON_SKIN );
            affect_to_char( ch, &af );

            af.type = gsn_iron_skin;

            if ( ch->level < 30 )
                af.duration = ( ch->level * 3 ) + ( get_curr_wis( ch ) * 5 );
            else
                af.duration = ( ch->level * 4 ) + ( get_curr_wis( ch ) * 6 );

            af.level = ch->level;
            af.location = APPLY_DEX;

            if ( ch->pcdata->learned[gsn_iron_skin] < 30 )
                af.modifier = -4;
            else if ( ch->pcdata->learned[gsn_iron_skin] < 50 )
                af.modifier = -3;
            else if ( ch->pcdata->learned[gsn_iron_skin] < 85 )
                af.modifier = -2;
            else
                af.modifier = -1;

            af.bitvector = meb( AFF_IRON_SKIN );
            affect_to_char( ch, &af );

            af.type = gsn_iron_skin;

            af.location = APPLY_DAMROLL;

            if ( ch->level < 50 )
                af.modifier = 3;
            else
                af.modifier = 5;

            if ( ch->level < 30 )
                af.duration = ( ch->level * 3 ) + ( get_curr_wis( ch ) * 5 );
            else
                af.duration = ( ch->level * 4 ) + ( get_curr_wis( ch ) * 6 );

            af.level = ch->level;
            af.bitvector = meb( AFF_IRON_SKIN );
            affect_to_char( ch, &af );

            learn_from_success( ch, gsn_iron_skin );

            return;
        }
        else {
            send_to_char( "You've failed to turn yourself as hard as iron.\r\n", ch );

            learn_from_failure( ch, gsn_iron_skin );

            return;
        }
    }
    else {
        send_to_char( "You wouldn't know where to start.\r\n", ch );
        return;
    }
    return;
}

//Status: Just started.... -Taon
//Notes: Will be used to help monks fend off hunger. -Taon
void do_sustain_self( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return;
    if ( ch->focus_level < 1 ) {
        send_to_char( "You're not focused enough for such a task.", ch );
        return;
    }
    if ( ch->move < 50 - get_curr_dex( ch ) ) {
        send_to_char( "You dont have enough energy to accomplish such a feat.", ch );
        return;
    }

    if ( ch->pcdata->learned[gsn_sustain_self] > 0 ) {
        ch->move -= 50 - get_curr_dex( ch );

        if ( !IS_AFFECTED( ch, AFF_SUSTAIN_SELF ) ) {
            send_to_char( "You urge your body to fight off hunger.\r\n", ch );
            learn_from_success( ch, gsn_sustain_self );
            adjust_focus( ch, -1 );
            af.type = gsn_sustain_self;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.duration = -1;
            af.bitvector = meb( AFF_SUSTAIN_SELF );
            af.level = ch->level;
            affect_to_char( ch, &af );
            return;
        }
        else {
            send_to_char( "Your body will no longer fight off hunger.\r\n", ch );
            xREMOVE_BIT( ch->affected_by, AFF_SUSTAIN_SELF );
            return;
        }
    }
    else {
        send_to_char( "You wouldn't have a clue on how to do such a thing.\r\n", ch );
        return;
    }
    return;
}

// Status: Skill completed. -Taon
void do_sidestep( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vict;
    AFFECT_DATA             af;
    short                   advantage;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You can't do that right now.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) ) {
        send_to_char( "You cant see them to properly sidestep!\r\n", ch );
        return;
    }

    if ( !ch->fighting || !( vict = who_fighting( ch ) ) ) {
        send_to_char( "You must be fighting..\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( vict, AFF_SIDESTEP ) ) {
        send_to_char( "They're already off balance.\r\n", ch );
        return;
    }

    advantage =
        ( ch->level - vict->level ) + ( get_curr_dex( ch ) - get_curr_dex( vict ) ) +
        number_chance( 1, 10 );

    if ( advantage < 0 )
        advantage = 0;

    if ( ch->Class == CLASS_MONK )
        advantage = 2;

    if ( ch->focus_level >= 30 )
        advantage += 5;

    WAIT_STATE( ch, skill_table[gsn_sidestep]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_sidestep ) ) {
        af.type = gsn_sidestep;
        af.location = APPLY_AC;

        if ( advantage > 0 )
            af.modifier = advantage;
        else
            af.modifier = 1;

        af.duration = ( ch->level / 10 ) + 1;
        af.level = ch->level;
        af.bitvector = meb( AFF_SIDESTEP );
        affect_to_char( vict, &af );
        ch_printf( ch, "You sidestep %s throwing them off balance.\r\n", PERS( vict, ch ) );
        ch_printf( vict, "%s's sidesteps you, throwing you off balance.\r\n", PERS( ch, vict ) );
        learn_from_success( ch, gsn_sidestep );
    }
    else {
        ch_printf( vict, "%s fails to sidestep you!\r\n", PERS( ch, vict ) );
        ch_printf( ch, "You fail to sidestep %s!\r\n", PERS( vict, ch ) );
        learn_from_failure( ch, gsn_sidestep );
    }
}

//New daze skill for monks complete. -Taon
//Msg: Also used by shadowknights.

void do_daze( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    short                   chance;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    if ( ch->pcdata->learned[gsn_daze] <= 0 ) {
        send_to_char( "You dont know this skill.\r\n", ch );
        return;
    }
    if ( !ch->fighting ) {
        send_to_char( "But you're not fighting anyone!\r\n", ch );
        return;
    }
    if ( ch->move < 35 - get_curr_dex( ch ) ) {
        send_to_char( "You don't have enough energy.\r\n", ch );
        return;
    }

    victim = who_fighting( ch );

    if ( !IS_NPC( ch ) && IS_AFFECTED( victim, AFF_SLOW ) ) {
        send_to_char( "They're already slow enough.\r\n", ch );
        return;
    }

    chance =
        ( ch->pcdata->learned[gsn_daze] / 2 ) + number_chance( 20,
                                                               55 ) + get_curr_dex( ch ) -
        get_curr_dex( victim ) + ( get_curr_str( ch ) / 5 );

    if ( ch->level < victim->level - 20 )
        chance += ch->level - victim->level;
    else
        chance += ch->level - victim->level + 10;

    if ( chance > 100 ) {
        WAIT_STATE( ch, 2 * ( PULSE_VIOLENCE / 2 ) );
        WAIT_STATE( victim, 3 * ( PULSE_VIOLENCE / 2 ) );
        af.type = gsn_daze;
        af.location = APPLY_STR;

        if ( get_curr_str( ch ) < 12 )
            af.modifier = -1;
        else if ( get_curr_str( ch ) < 16 )
            af.modifier = -2;
        else if ( get_curr_str( ch ) < 20 )
            af.modifier = -3;
        else
            af.modifier = -4;

        af.level = ch->level;
        af.duration = ch->level / 2;
        af.bitvector = meb( AFF_SLOW );
        affect_to_char( victim, &af );
        learn_from_success( ch, gsn_daze );
        ch->move -= 35 - get_curr_dex( ch );
        ch_printf( ch, "&cYou strike %s in the back on the neck, leaving them dazed.\r\n",
                   victim->name );
        ch_printf( victim, "&c%s strikes at your neck, leaving you dazed.\r\n", ch->name );
    }
    else {
        WAIT_STATE( ch, 3 * ( PULSE_VIOLENCE / 2 ) );
        send_to_char( "You fail to properly daze your target!\r\n", ch );
        learn_from_failure( ch, gsn_daze );
    }
    return;
}

//Status: Shieldblock Code installed... -Taon

//This is complete though I intend on giving a good
//chance of causing damage to the shield when this is
//invoked. 

//Status: Installed. -Taon

bool check_phase( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     dice;

    if ( IS_NPC( victim ) )
        return FALSE;
    if ( !IS_AWAKE( victim ) )
        return FALSE;
    if ( victim->move < 2 || victim->mana < 2 )
        return FALSE;
    if ( number_chance( 1, 3 ) == 1 )
        return FALSE;

    if ( !IS_AFFECTED( victim, AFF_PHASE ) ) {
        return FALSE;
    }

    if ( can_use_skill( victim, number_percent(  ), gsn_phase ) ) {
        dice = ( int ) ( LEARNED( victim, gsn_phase ) / sysdata.phase_mod );

        if ( victim->focus_level > 30 )
            dice += victim->focus_level / 3;

        if ( get_curr_dex( victim ) >= get_curr_dex( ch ) + 5 )
            dice += get_curr_dex( victim ) - get_curr_dex( ch ) + 5;
        else
            dice += get_curr_dex( victim ) - get_curr_dex( ch );

        if ( IS_AFFECTED( victim, AFF_DEFENSIVE_POSTURING ) )
            dice += LEARNED( victim, ( gsn_defensive_posturing / 10 ) );

        if ( ( IS_AFFECTED( victim, AFF_BLINDNESS ) && !IS_AFFECTED( victim, AFF_NOSIGHT ) )
             || victim->position == POS_SITTING || victim->position == POS_RESTING )
            dice /= 2;

        if ( !chance( victim, dice + victim->level - ch->level ) ) {
            learn_from_failure( victim, gsn_phase );
            return FALSE;
        }

        act( AT_DGREEN, "Your body phases, absorbing $n's attack.", ch, NULL, victim, TO_VICT );
        act( AT_CYAN, "$N's body phases, absorbing your attack.", ch, NULL, victim, TO_CHAR );
        victim->move -= 2;
        victim->mana -= 2;
        learn_from_success( victim, gsn_phase );

        if ( number_chance( 1, 5 ) > 4 )
            if ( ch->Class == CLASS_MONK )
                adjust_focus( victim, -1 );

        return TRUE;
    }
    else
        return FALSE;
}

//Status: Completed skill, also used by priests/angels. -Taon
void do_layhands( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    int                     heal_rate,
                            heal_xp;

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "You must provide a target.\r\n", ch );
        return;
    }
    if ( ch->mana < 150 && ch->Class != CLASS_MONK ) {
        send_to_char( "You don't have enough mana to accomplish this task.\r\n", ch );
        return;
    }
    else if ( ch->move < 40 ) {
        send_to_char( "You dont have enough move to accomplish such a task.\r\n", ch );
        return;
    }
    if ( ch->Class == CLASS_MONK && ch->focus_level < 5 ) {
        send_to_char( "You don't have enough focus to lay hands.", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_CHARM ) || ch->fighting ) {
        send_to_char( "You can't concentrate on such a task right now.\r\n", ch );
        return;
    }
    if ( ch->fighting ) {
        send_to_char( "Not while fighting.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They're not anywhere to be seen around here.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "You can't lay hands on yourself.\r\n", ch );
        return;
    }
    if ( victim == who_fighting( ch ) ) {
        send_to_char( "But you're fighting them....\r\n", ch );
        return;
    }
    if ( victim->hit >= victim->max_hit ) {
        send_to_char( "This one is already at max health.\r\n", ch );
        return;
    }

    if ( ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK || ch->thirdclass == CLASS_MONK ) {
        if ( ch->focus_level <= 10 )
            heal_rate = ( ch->level * 5 ) + ( get_curr_int( ch ) * 3 );
        else if ( ch->focus_level <= 20 )
            heal_rate = ( ch->level * 6 ) + ( get_curr_int( ch ) * 3 );
        else if ( ch->focus_level <= 30 )
            heal_rate = ( ch->level * 7 ) + ( get_curr_int( ch ) * 4 );
        else if ( ch->focus_level <= 40 )
            heal_rate = ( ch->level * 8 ) + ( get_curr_int( ch ) * 4 );
        else
            heal_rate = ( ch->level * 9 ) + ( get_curr_int( ch ) * 5 );
    }
    else {
        if ( ch->pcdata->learned[gsn_layhands] < 30 )
            heal_rate = ( ch->level * 3 ) + ( get_curr_int( ch ) * 3 );
        else if ( ch->pcdata->learned[gsn_layhands] < 50 )
            heal_rate = ( ch->level * 4 ) + ( get_curr_int( ch ) * 3 );
        else if ( ch->pcdata->learned[gsn_layhands] < 70 )
            heal_rate = ( ch->level * 5 ) + ( get_curr_int( ch ) * 3 );
        else if ( ch->pcdata->learned[gsn_layhands] < 90 )
            heal_rate = ( ch->level * 5 ) + ( get_curr_int( ch ) * 5 );
        else
            heal_rate = ( ch->level * 6 ) + ( get_curr_int( ch ) * 6 );
    }

    if ( ch->Class == CLASS_MONK || ch->Class == CLASS_ANGEL || ch->Class == CLASS_PRIEST )
        heal_xp = ( heal_rate / 2 ) + ch->level;
    gain_exp( ch, heal_xp );

    if ( heal_rate > victim->max_hit )
        heal_rate = victim->max_hit;

    WAIT_STATE( ch, skill_table[gsn_layhands]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_layhands ) ) {
        if ( ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK
             || ch->thirdclass == CLASS_MONK ) {
            ch->move -= 40;
            adjust_focus( ch, -5 );
        }
        else
            ch->mana -= 150;

        if ( !IS_NPC( ch ) )
            ch_printf( ch, "You lay hands upon %s\r\n", victim->name );
        else
            ch_printf( ch, "You lay hands upon %s\r\n", victim->short_descr );

        ch_printf( victim, "%s lays hands upon you, feeling you with life.\r\n", ch->name );
        act( AT_MAGIC, "$n lays hands upon $s.", ch, NULL, victim, TO_ROOM );

        victim->hit += heal_rate;
        learn_from_success( ch, gsn_layhands );
    }
    else {
        send_to_char( "You've failed to properly mend the wounds of your target.\r\n", ch );
        learn_from_failure( ch, gsn_layhands );
    }
    return;
}

//Status: Completed skill. -Taon
void do_untangle( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( arg[0] == '\0' ) {
        send_to_char( "Untangle who???\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "Your target is nowhere to be found.\r\n", ch );
        return;
    }

    if ( !IS_AFFECTED( victim, AFF_SNARE ) && !IS_AFFECTED( victim, AFF_TANGLE ) ) {
        send_to_char( "But they're not ensnared or tangled!\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "You can't untangle yourself.\r\n", ch );
        return;
    }
    if ( ch->move < 20 ) {
        send_to_char( "You don't have enough energy to do such a thing.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_untangle]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_untangle ) ) {
        if ( IS_NPC( victim ) )
            ch_printf( ch, "You untangle %s\r\n", victim->short_descr );
        else
            ch_printf( ch, "You untangle %s\r\n", victim->name );

        ch_printf( victim, "You're untangled by %s\r\n ", ch->name );
        affect_strip( victim, gsn_ensnare );
        xREMOVE_BIT( victim->affected_by, AFF_SNARE );
        affect_strip( victim, gsn_tangle );
        xREMOVE_BIT( victim->affected_by, AFF_TANGLE );
        ch->move -= 20;
        learn_from_success( ch, gsn_untangle );
    }
    else {
        send_to_char( "You've failed to untangle your target!\r\n", ch );
        learn_from_failure( ch, gsn_untangle );
    }
    return;
}

/* Idea behind this is a monk or assassin can ensnare their target,
   leaving them unable to leave that room, until snare wears off. -Taon
   STATUS: 90% complete, will complete tomorrow night.
*/

void do_ensnare( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    char                    arg[MIL];
    int                     chance;

    argument = one_argument( argument, arg );

    if ( IS_NPC( ch ) )
        return;

    if ( ch->pcdata->learned[gsn_ensnare] < 0 ) {
        send_to_char( "You wouldn't know where to begin!\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "You must provide a target to ensnare.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "Your target is nowhere to be found!\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "Ensnare yourself, eh?", ch );
        return;
    }
    if ( ch->move < 25 ) {
        send_to_char( "You dont have enough movement!\r\n", ch );
        return;
    }
    if ( victim->mount ) {
        send_to_char( "You can't snare them while they're mounted.\r\n", ch );
        return;
    }

    chance =
        number_chance( 1,
                       25 ) + ( ch->pcdata->learned[gsn_ensnare] / 2 ) + ( ch->level -
                                                                           victim->level ) +
        ( ( get_curr_dex( ch ) - get_curr_dex( victim ) ) * 2 );

    if ( victim->position == POS_SITTING || victim->position == POS_RESTING || !IS_AWAKE( ch ) )
        chance += 20;
    if ( IS_AFFECTED( victim, AFF_SIDESTEP ) )
        chance += 10;
    if ( IS_AFFECTED( ch, AFF_SIDESTEP ) )
        chance -= 10;
    if ( IS_IMMORTAL( ch ) )
        chance = 101;

    if ( chance > 100 || number_chance( 1, 10 ) == 2 ) {

        af.type = gsn_ensnare;
        af.location = APPLY_NONE;
        af.modifier = 0;

        if ( ch->level < 20 )
            af.duration = 30 - get_curr_str( victim );
        else if ( ch->level < 40 )
            af.duration = 40 - get_curr_str( victim );
        else if ( ch->level < 60 )
            af.duration = 50 - get_curr_str( victim );
        else if ( ch->level < 80 )
            af.duration = 60 - get_curr_str( victim );
        else
            af.duration = 70 - get_curr_str( victim );

        af.bitvector = meb( AFF_SNARE );
        af.level = ch->level;
        affect_to_char( victim, &af );

        if ( IS_NPC( victim ) )
            ch_printf( ch, "You've ensnared %s.\r\n", victim->short_descr );
        else
            ch_printf( ch, "You've ensnared %s.\r\n", victim->name );

        ch->move -= 25;
        ch_printf( victim, "You've been ensnared by %s.\r\n", ch->name );
        learn_from_success( ch, gsn_ensnare );
    }
    else {
        send_to_char( "You've failed to snare your target.\r\n", ch );
        learn_from_failure( ch, gsn_ensnare );
    }
    return;
}

/* Status: Completed skill. -Taon */
void do_recoil( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    short                   chance;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "But you're charmed.\r\n", ch );
        return;
    }
    if ( ch->pcdata->learned[gsn_recoil] <= 0 ) {
        send_to_char( "You wouldn't know where to start.\r\n", ch );
        return;
    }
    if ( ch->move < 35 - get_curr_dex( ch ) ) {
        send_to_char( "You don't have enough energy to do such a task.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_RECOIL ) ) {
        send_to_char( "You're already recoiled.\r\n", ch );
        return;
    }

    chance = get_curr_dex( ch ) + get_curr_str( ch ) + ( ch->pcdata->learned[gsn_recoil] / 2 );

    if ( IS_IMMORTAL( ch ) )
        chance = 101;

    if ( chance > number_chance( 60, 100 ) ) {
        WAIT_STATE( ch, skill_table[gsn_recoil]->beats );
        af.type = gsn_recoil;
        af.location = APPLY_DAMROLL;
        af.duration = ch->level * ( get_curr_wis( ch ) / 4 );
        af.modifier = get_curr_str( ch ) / 10;
        af.level = ch->level;
        af.bitvector = meb( AFF_RECOIL );
        affect_to_char( ch, &af );

        af.type = gsn_recoil;
        af.location = APPLY_STR;
        af.duration = ch->level * ( get_curr_wis( ch ) / 4 );

        if ( ch->level < 50 )
            af.modifier = 2;
        else if ( ch->level < 75 )
            af.modifier = 3;
        else if ( ch->level < 90 )
            af.modifier = 4;
        else
            af.modifier = 5;

        af.level = ch->level;
        af.bitvector = meb( AFF_RECOIL );
        affect_to_char( ch, &af );
        send_to_char( "You recoil your posture for maximum strength.\r\n", ch );
        ch->move -= 35 - get_curr_dex( ch );
        learn_from_success( ch, gsn_recoil );
        return;
    }
    else {
        WAIT_STATE( ch, skill_table[gsn_recoil]->beats );
        send_to_char( "You failed to properly recoil your body.\r\n", ch );
        learn_from_failure( ch, gsn_recoil );
        return;
    }
}

//Autoskill for monks. -Taon
//Note: Also given to several other classes.
bool check_displacement( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     chance;

    if ( IS_NPC( victim ) )
        return FALSE;

    if ( !IS_AWAKE( victim ) )
        return FALSE;

    if ( number_chance( 1, 7 ) < 4 )
        return FALSE;

    if ( victim->pcdata->learned[gsn_displacement] <= 0 )
        return FALSE;

    chance =
        ( victim->pcdata->learned[gsn_displacement] / sysdata.displacement_mod ) +
        get_curr_dex( victim ) + ( victim->level - ch->level ) + ( get_curr_lck( victim ) / 3 );

    if ( IS_AFFECTED( victim, AFF_SLOW ) )
        chance -= 10;

    if ( chance > number_chance( 20, 60 ) ) {
        act( AT_DGREEN, "$n hits your displaced image.", ch, NULL, victim, TO_VICT );
        act( AT_CYAN, "You hit $N's displaced image.", ch, NULL, victim, TO_CHAR );

        if ( number_chance( 0, 5 ) > 4 )
            learn_from_success( victim, gsn_displacement );

        return TRUE;
    }

    learn_from_failure( victim, gsn_displacement );
    return FALSE;
}

//Status: Nearly completed ritual skill for monks/priests/angels. -Taon
void do_ritual( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    short                   chance;

    if ( IS_NPC( ch ) )
        return;
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "You cant perform such a task right now.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_RITUAL ) ) {
        send_to_char( "You have to wait a little longer to do it again.\r\n", ch );
        return;
    }
    if ( ch->move < 40 ) {
        send_to_char( "You don't have enough energy to do that.\r\n", ch );
        return;
    }
    if ( !IS_OUTSIDE( ch ) ) {
        send_to_char( "You must be outdoors to perform such a ritual.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_ritual]->beats );
    if ( can_use_skill( ch, ( number_percent(  ) ), gsn_ritual ) ) {
        chance = number_chance( 1, 5 );

        if ( chance == 1 ) {
            af.type = gsn_ritual;
            af.location = APPLY_HIT;
            af.modifier = ch->level / 2;
            af.level = ch->level;
            af.duration = ch->level + ( get_curr_int( ch ) * 5 );
            af.bitvector = meb( AFF_RITUAL );
            affect_to_char( ch, &af );
        }
        else if ( chance == 2 ) {
            af.type = gsn_ritual;
            af.location = APPLY_HIT;
            af.modifier = ch->level / 2;
            af.level = ch->level;
            af.duration = ch->level + ( get_curr_int( ch ) * 5 );
            af.bitvector = meb( AFF_RITUAL );
            affect_to_char( ch, &af );

            af.type = gsn_ritual;
            af.location = APPLY_MANA;
            af.modifier = ch->level / 2;
            af.level = ch->level;
            af.duration = ch->level + ( get_curr_int( ch ) * 5 );
            af.bitvector = meb( AFF_RITUAL );
            affect_to_char( ch, &af );
        }
        else if ( chance == 3 ) {
            af.type = gsn_ritual;
            af.location = APPLY_MANA;
            af.modifier = ch->level / 2;
            af.level = ch->level;
            af.duration = ch->level + ( get_curr_int( ch ) * 5 );
            af.bitvector = meb( AFF_RITUAL );
            affect_to_char( ch, &af );
        }
        else if ( chance == 4 ) {
            af.type = gsn_ritual;
            af.location = APPLY_HITROLL;

            if ( get_curr_str( ch ) > 3 )
                af.modifier = get_curr_str( ch ) / 4;
            else
                af.modifier = 1;

            af.level = ch->level;
            af.duration = ch->level + ( get_curr_int( ch ) * 5 );
            af.bitvector = meb( AFF_RITUAL );
            affect_to_char( ch, &af );
        }
        else {
            af.type = gsn_ritual;
            af.location = APPLY_DAMROLL;

            if ( get_curr_str( ch ) > 3 )
                af.modifier = get_curr_str( ch ) / 4;
            else
                af.modifier = 1;

            af.level = ch->level;
            af.duration = ch->level + ( get_curr_int( ch ) * 5 );
            af.bitvector = meb( AFF_RITUAL );
            affect_to_char( ch, &af );
        }

        ch->move -= 30;
        learn_from_success( ch, gsn_ritual );
        send_to_char( "You glow as you turn your face towards the sky.\r\n", ch );
        act( AT_WHITE, "$n glows as they turn their face towards the sky..", ch, NULL, NULL,
             TO_ROOM );
    }
    else {
        send_to_char( "You've Failed.\r\n", ch );
        learn_from_failure( ch, gsn_ritual );
    }

    return;
}
