 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Player settings/statistics module                                     *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/hometowns.h"
#include "h/clans.h"
#include "h/languages.h"
#include "h/files.h"
#include "h/polymorph.h"

/* Locals */
char                   *tiny_affect_loc_name( int location );
void                    colorize_equipment( OBJ_DATA *obj, CHAR_DATA *ch );
char                   *default_prompt( CHAR_DATA *ch );
char                   *default_fprompt( CHAR_DATA *ch );
void                    string_stripblink( char *argument, char *string );

/* local function for save_scores */
void save_scores        args( (  ) );
void                    combine_affects( OBJ_DATA *obj );
extern bool             doubleexp;

void do_solve( CHAR_DATA *ch, char *argument )
{
    char                    arg[MSL];

    if ( !ch || !ch->pcdata )
        return;
    if ( !VLD_STR( argument ) ) {
        send_to_char( "Syntax: solve <code>\r\n", ch );
        return;
    }
    if ( !ch->pcdata->authexp ) {
        send_to_char( "You don't have any reason to enter a solve code.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg );
    if ( !str_cmp( ch->pcdata->authexp, arg ) ) {
        STRFREE( ch->pcdata->authexp );
        send_to_char
            ( "You have entered the correct solve code and your triple exp timer has been updated.\r\n",
              ch );

        /*
         * Increase the timer 
         */
        if ( ch->pcdata->double_exp_timer < 100 )
            ch->pcdata->double_exp_timer = ( ch->pcdata->double_exp_timer + 10 );
        else if ( ch->pcdata->double_exp_timer < 250 )
            ch->pcdata->double_exp_timer = ( ch->pcdata->double_exp_timer + 8 );
        else if ( ch->pcdata->double_exp_timer < 500 )
            ch->pcdata->double_exp_timer = ( ch->pcdata->double_exp_timer + 5 );
        else if ( ch->pcdata->double_exp_timer < 1000 )
            ch->pcdata->double_exp_timer = ( ch->pcdata->double_exp_timer + 2 );
        else
            ch->pcdata->double_exp_timer = ( ch->pcdata->double_exp_timer + 1 );

        ch_printf( ch, "You now have %d percent on your triple exp timer.\r\n",
                   ch->pcdata->double_exp_timer );

        /*
         * Set the time of last update 
         */
        ch->pcdata->last_dexpupdate = current_time;
    }
    else
        send_to_char( "You entered an incorrect solve code.\r\n", ch );
}

void do_money( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *bank;
    int                     i,
                            uamount;
    bool                    found = FALSE;

    set_char_color( AT_GOLD, ch );
    for ( i = 1; i < MAX_CURR_TYPE; i++ ) {
        if ( ( uamount = GET_MONEY( ch, i ) ) ) {
            ch_printf( ch, "You have %d %s piece%s on you.\r\n", uamount, curr_types[i],
                       uamount != 1 ? "s" : "" );
            found = TRUE;
        }
    }
    if ( !found )
        send_to_char( "You have no money on you!\r\n", ch );

    if ( ch->pcdata && ch->pcdata->bank && VLD_STR( ch->pcdata->bank->name )
         && ( bank = find_bank( ch->pcdata->bank->name ) ) ) {
        ch_printf( ch, "\r\n&CFor the %s bank account the following funds are available:\r\n",
                   bank->name );
        ch_printf( ch, "&YThere is %d gold.\r\n", bank->gold );
        ch_printf( ch, "&YThere is %d silver.\r\n", bank->silver );
        ch_printf( ch, "&YThere is %d bronze.\r\n", bank->bronze );
        ch_printf( ch, "&YThere is %d copper.\r\n", bank->copper );
    }
}

void do_hate( CHAR_DATA *ch, char *argument )
{
    send_to_char
        ( "&C===========================================================================\r\n", ch );
    send_to_char
        ( "&C=====                   Current Mob Hate Level                        =====\r\n", ch );
    send_to_char
        ( "&C=====                                                                 =====\r\n", ch );
    set_pager_color( AT_LBLUE, ch );
    if ( ch->hate_level == 10 ) {
        pager_printf( ch,
                      "=====         Currently you rank: &R%d&C in Hate from Mobs                =====\r\n",
                      ch->hate_level );
    }
    else {
        pager_printf( ch,
                      "=====         Currently you rank: &R%d&C in Hate from Mobs                 =====\r\n",
                      ch->hate_level );
    }
    send_to_char
        ( "&C=====                                                                 =====\r\n", ch );
    send_to_char
        ( "&C===========================================================================\r\n", ch );
    return;
}

/* Allow us to use a bar to display the percent on almost anything */
char                   *percent_bar_display( CHAR_DATA *ch, int percent )
{
    static char             pbuf[MSL];

    if ( !ch )
        return ( char * ) "";
    if ( percent < 0 )
        percent = 0;
    /*
     * Don't change these to mudstrlcpy/mudstrlcat/snprintf - Valgrind doesnt like it if
     * you do 
     */
    /*
     * If not in this range just show the percent 
     */
    if ( percent > 10 || percent <= 100 ) {
        sprintf( pbuf, "&Y[&R|%s&Y%s%s&B%s%s&G%s%s%s%s&Y]", percent > 20 ? "|" : " ",
                 percent > 30 ? "|" : " ", percent > 40 ? "|" : " ",
                 percent > 50 ? "|" : " ", percent > 60 ? "|" : " ", percent > 70 ? "|" : " ",
                 percent > 80 ? "|" : " ", percent > 90 ? "|" : " ", percent >= 100 ? "|" : " " );
    }
    else                                               /* Under 10% just show the percent 
                                                        * in a number */
        sprintf( pbuf, "&W[%s%d%%&W]",
                 percent >= 60 ? "&G" : percent >= 40 ? "&B" : percent >= 20 ? "&Y" : "&R",
                 percent );
    return pbuf;
}

// attempt at making score more friendly for the blind - Vladaar
void do_bscore( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];

    if ( IS_NPC( ch ) )
        return;

    if ( ch->desc && ch->desc->original ) {
        interpret( ch, ( char * ) "return" );
        return;
    }

    set_pager_color( AT_LBLUE, ch );
    pager_printf( ch, "\r\n&RScore Sheet for %s%s&C.\r\n", ch->name, ch->pcdata->title );
    if ( ch->secondclass == -1 ) {
        pager_printf( ch, "You're a %s race with a level %d %s class.\r\n",
                      !VLD_STR( race_table[ch->race]->race_name ) ? "Unknown" : race_table[ch->
                                                                                           race]->
                      race_name, ch->firstlevel > 0 ? ch->firstlevel : ch->level,
                      capitalize( class_table[ch->Class]->who_name ) );
    }
    else if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
        pager_printf( ch, "You're a %s race with a level %d %s/%s class.\r\n",
                      !VLD_STR( race_table[ch->race]->race_name ) ? "Unknown" : race_table[ch->
                                                                                           race]->
                      race_name, ch->level, capitalize( class_table[ch->Class]->who_name ),
                      class_table[ch->secondclass]->who_name );
    }
    else if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
        pager_printf( ch, "You're a %s race with a level %d %s/%s/%s class\r\n",
                      !VLD_STR( race_table[ch->race]->race_name ) ? "Unknown" : race_table[ch->
                                                                                           race]->
                      race_name, ch->level, capitalize( class_table[ch->Class]->who_name ),
                      class_table[ch->secondclass]->who_name,
                      class_table[ch->thirdclass]->who_name );
    }
    pager_printf( ch, "You are %d years old and %s is your hometown.\r\n", calculate_age( ch ),
                  !ch->pcdata->htown ? "None" : VLD_STR( ch->pcdata->htown->name ) ? ch->pcdata->
                  htown->name : "Unknown" );
    pager_printf( ch,
                  "Your current stats are the following:\r\nStr %d Int %d Wis %d Con %d Dex %d Cha %d Lck %d\r\n",
                  get_curr_str( ch ), get_curr_int( ch ), get_curr_wis( ch ), get_curr_con( ch ),
                  get_curr_dex( ch ), get_curr_cha( ch ), get_curr_lck( ch ) );
    pager_printf( ch, "You have a HitRoll of %d a Damroll of %d and %d Armor Class.\r\n",
                  GET_HITROLL( ch ), GET_DAMROLL( ch ), GET_AC( ch ) );
    if ( IS_BLOODCLASS( ch ) ) {
        ch_printf( ch, "You have %d/%d hp %d/%d blood %d/%d move.\r\n", ch->hit, ch->max_hit,
                   ch->blood, ch->max_blood, ch->move, ch->max_move );
    }
    else {
        ch_printf( ch, "You have %d/%d hp %d/%d mana %d/%d move.\r\n", ch->hit, ch->max_hit,
                   ch->mana, ch->max_mana, ch->move, ch->max_move );
    }

    ch_printf( ch, "Your current alignment is %+d.\r\n", ch->alignment );
    ch_printf( ch, "&CYou have %d Glory Points.\r\n", ch->quest_curr );
    if ( ch->pcdata->deity ) {
        if ( ch->pcdata->favor > 2250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "loved" );
        else if ( ch->pcdata->favor > 2000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "cherished" );
        else if ( ch->pcdata->favor > 1750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "honored" );
        else if ( ch->pcdata->favor > 1500 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "praised" );
        else if ( ch->pcdata->favor > 1250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "favored" );
        else if ( ch->pcdata->favor > 1000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "respected" );
        else if ( ch->pcdata->favor > 750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "liked" );
        else if ( ch->pcdata->favor > 250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "tolerated" );
        else if ( ch->pcdata->favor > -250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "ignored" );
        else if ( ch->pcdata->favor > -750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "shunned" );
        else if ( ch->pcdata->favor > -1000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "disliked" );
        else if ( ch->pcdata->favor > -1250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "dishonored" );
        else if ( ch->pcdata->favor > -1500 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "disowned" );
        else if ( ch->pcdata->favor > -1750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "abandoned" );
        else if ( ch->pcdata->favor > -2000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "despised" );
        else if ( ch->pcdata->favor > -2250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "hated" );
        else
            snprintf( buf, MAX_STRING_LENGTH, "%s", "damned" );
        pager_printf( ch, "You are devoted to %-20s deity and your favor level is %s.\r\n",
                      ch->pcdata->deity->name, buf );
    }
}

void do_score( CHAR_DATA *ch, char *argument )
{
    int                     iLang,
                            x,
                            count = 0;
    const char             *divider =
        "================================================================================\r\n";
    char                    buf[MSL],
                            hr_buf[MSL],
                            dr_buf[MSL],
                            print_buf[MSL];
    int                     feet,
                            inches;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_BLIND( ch ) ) {
        do_bscore( ch, ( char * ) "" );
        return;
    }

    /*
     * Deal with switched imms 
     */
    if ( ch->desc && ch->desc->original ) {
        interpret( ch, ( char * ) "return" );
        return;
    }
    buf[0] = hr_buf[0] = dr_buf[0] = print_buf[0] = '\0';
    set_pager_color( AT_LBLUE, ch );
    pager_printf( ch, "\r\n&RScore for %s%s&C.\r\n", ch->name, ch->pcdata->title );
    if ( get_trust( ch ) != ch->level )
        pager_printf( ch, "You are trusted at level %d.\r\n", get_trust( ch ) );
    /*
     * Print the score divider. -Orion 
     */
    send_to_pager( divider, ch );

    pager_printf( ch, "Hometown: %s\r\n",
                  !ch->pcdata->htown ? "None" : VLD_STR( ch->pcdata->htown->name ) ? ch->pcdata->
                  htown->name : "Unknown" );
    pager_printf( ch, "Level : %-3d         Race : %-10.10s          Played: %d hours\r\n",
                  ch->level,
                  !VLD_STR( race_table[ch->race]->race_name ) ? "Unknown" : race_table[ch->race]->
                  race_name, GET_TIME_PLAYED( ch ) );

    pager_printf( ch, "Class : (%3d)%-12s\r\n", ch->firstlevel > 0 ? ch->firstlevel : ch->level,
                  capitalize( class_table[ch->Class]->who_name ) );
    if ( IS_SECONDCLASS( ch ) )
        pager_printf( ch, "Class : (%3d)%-12s\r\n", ch->secondlevel,
                      capitalize( class_table[ch->secondclass]->who_name ) );
    if ( IS_THIRDCLASS( ch ) )
        pager_printf( ch, "Class : (%3d)%-12s\r\n", ch->thirdlevel,
                      capitalize( class_table[ch->thirdclass]->who_name ) );

    pager_printf( ch, "Years : %d                                     ", calculate_age( ch ) );

    pager_printf( ch, "Log In: %24.24s\r\n", ctime( &( ch->logon ) ) );
    snprintf( hr_buf, MSL, "HitRoll: &R%-4d&C", GET_HITROLL( ch ) );
    snprintf( dr_buf, MSL, "DamRoll: &R%-4d&C", GET_DAMROLL( ch ) );

    feet = ch->height / 12;
    inches = ch->height % 12;
    int                     bonus;

    bonus = ch->level;
    snprintf( buf, MSL, "%d'%d\"", feet, inches );
    pager_printf( ch, "Height: &R%-11s&C Weight: &R%-12d&C       ", buf, ch->weight );
    if ( bonus < 2 ) {
        bonus = 1;
    }
    if ( ch->race == RACE_DRAGON )
        pager_printf( ch, "Dragon Size Bonus: &R%-3d&C", ( bonus / 2 ) + 10 );
    send_to_pager( "\r\n", ch );
    pager_printf( ch, "STR   : &Y%2.2d&C(&W%2.2d&C)      %13s              Saved : %s",
                  get_curr_str( ch ), ch->perm_str, hr_buf,
                  ch->save_time ? ctime( &( ch->save_time ) ) : "N/A\r\n" );
    pager_printf( ch, "INT   : &Y%2.2d&C(&W%2.2d&C)      %13s              Time  : %s",
                  get_curr_int( ch ), ch->perm_int, dr_buf, ctime( &current_time ) );
    if ( GET_AC( ch ) >= 200 )
        mudstrlcpy( buf, "&Rnaked in the wind", MSL );
    else if ( GET_AC( ch ) >= 150 )
        mudstrlcpy( buf, "&rthe rags of a beggar", MSL );
    else if ( GET_AC( ch ) >= 120 )
        mudstrlcpy( buf, "&rimproper for adventure", MSL );
    else if ( GET_AC( ch ) >= 85 )
        mudstrlcpy( buf, "&Yshabby and threadbare", MSL );
    else if ( GET_AC( ch ) >= 60 )
        mudstrlcpy( buf, "&Yof poor quality", MSL );
    else if ( GET_AC( ch ) >= 40 )
        mudstrlcpy( buf, "&pscant protection", MSL );
    else if ( GET_AC( ch ) >= 20 )
        mudstrlcpy( buf, "&pthat of a knave", MSL );
    else if ( GET_AC( ch ) >= 0 )
        mudstrlcpy( buf, "&Pmoderately crafted", MSL );
    else if ( GET_AC( ch ) >= -140 )
        mudstrlcpy( buf, "&Pwell crafted", MSL );
    else if ( GET_AC( ch ) >= -180 )
        mudstrlcpy( buf, "&Othe envy of squires", MSL );
    else if ( GET_AC( ch ) >= -200 )
        mudstrlcpy( buf, "&Oexcellently crafted", MSL );
    else if ( GET_AC( ch ) >= -300 )
        mudstrlcpy( buf, "&cthe envy of knights", MSL );
    else if ( GET_AC( ch ) >= -450 )
        mudstrlcpy( buf, "&cthe envy of barons", MSL );
    else if ( GET_AC( ch ) >= -600 )
        mudstrlcpy( buf, "&wthe envy of dukes", MSL );
    else if ( GET_AC( ch ) >= -850 )
        mudstrlcpy( buf, "&wthe envy of kings", MSL );
    else if ( GET_AC( ch ) >= -1000 )
        mudstrlcpy( buf, "&wthe envy of emperors", MSL );
    else
        mudstrlcpy( buf, "&Wthat of an avatar", MSL );
    pager_printf( ch, "WIS   : &Y%2.2d&C(&W%2.2d&C)      ", get_curr_wis( ch ), ch->perm_wis );
    if ( ch->race == RACE_DRAGON )
        snprintf( print_buf, MSL, "Dragon Hide Armor: &R%4.4d, %s&C", GET_AC( ch ), buf );
    else
        snprintf( print_buf, MSL, "Armor: &R%4.4d, %s&C", GET_AC( ch ), buf );
    pager_printf( ch, "%s\r\n", print_buf );
    if ( ch->alignment > 900 )
        mudstrlcpy( buf, "devout", MSL );
    else if ( ch->alignment > 700 )
        mudstrlcpy( buf, "noble", MSL );
    else if ( ch->alignment > 350 )
        mudstrlcpy( buf, "honorable", MSL );
    else if ( ch->alignment > 100 )
        mudstrlcpy( buf, "worthy", MSL );
    else if ( ch->alignment > -100 )
        mudstrlcpy( buf, "neutral", MSL );
    else if ( ch->alignment > -350 )
        mudstrlcpy( buf, "base", MSL );
    else if ( ch->alignment > -700 )
        mudstrlcpy( buf, "evil", MSL );
    else if ( ch->alignment > -900 )
        mudstrlcpy( buf, "ignoble", MSL );
    else
        mudstrlcpy( buf, "fiendish", MSL );
    if ( ch->level < 10 )
        snprintf( print_buf, MSL, "%-17.17s", buf );
    else
        snprintf( print_buf, MSL, "%+4d, %-11.11s", ch->alignment, buf );
    pager_printf( ch,
                  "DEX   : &Y%2.2d&C(&W%2.2d&C)      Align: %-17.17s   Items : %5.5d (Max %5.5d)\r\n",
                  get_curr_dex( ch ), ch->perm_dex, print_buf, ch->carry_number,
                  can_carry_n( ch ) );
    switch ( ch->position ) {
        case POS_DEAD:
            mudstrlcpy( buf, "decomposing", MSL );
            break;
        case POS_MORTAL:
            mudstrlcpy( buf, "mortally wounded", MSL );
            break;
        case POS_INCAP:
            mudstrlcpy( buf, "incapacitated", MSL );
            break;
        case POS_STUNNED:
            mudstrlcpy( buf, "stunned", MSL );
            break;
        case POS_SLEEPING:
            mudstrlcpy( buf, "sleeping", MSL );
            break;
        case POS_MEDITATING:
            mudstrlcpy( buf, "meditating", MSL );
            break;
        case POS_RESTING:
            mudstrlcpy( buf, "resting", MSL );
            break;
        case POS_STANDING:
            mudstrlcpy( buf, "standing", MSL );
            break;
        case POS_FIGHTING:
            mudstrlcpy( buf, "fighting", MSL );
            break;
        case POS_EVASIVE:
            mudstrlcpy( buf, "(evasive)", MSL );
            break;
        case POS_DEFENSIVE:
            mudstrlcpy( buf, "(defensive)", MSL );
            break;
        case POS_AGGRESSIVE:
            mudstrlcpy( buf, "(aggressive)", MSL );
            break;
        case POS_BERSERK:
            mudstrlcpy( buf, "(berserk)", MSL );
            break;
        case POS_MOUNTED:
            mudstrlcpy( buf, "mounted", MSL );
            break;
        case POS_SITTING:
            mudstrlcpy( buf, "sitting", MSL );
            break;
    }
    pager_printf( ch,
                  "CON   : &Y%2.2d&C(&W%2.2d&C)      Pos'n: %-17.17s   Equipment Weight: %5.5d\r\n",
                  get_curr_con( ch ), ch->perm_con, buf, ch->carry_weight );
    pager_printf( ch,
                  "CHA   : &Y%2.2d&C(&W%2.2d&C)      Wimpy: %-5d&C               (Max Can Carry: %7.7d)\r\n",
                  get_curr_cha( ch ), ch->perm_cha, ch->wimpy, can_carry_w( ch ) );
    pager_printf( ch, "LCK   : &Y%2.2d&C(&W%2.2d&C) \r\n", get_curr_lck( ch ), ch->perm_lck );
    pager_printf( ch,
                  "PRACT : %4.4d        &YHitpoints: %-6d of %6d&C   Pager  : [%c]%2d    AutoExit [%c]",
                  ch->practice, ch->hit, ch->max_hit, IS_SET( ch->pcdata->flags,
                                                              PCFLAG_PAGERON ) ? '*' : ' ',
                  ch->pcdata->pagerlen, xIS_SET( ch->act, PLR_AUTOEXIT ) ? '*' : ' ' );
    if ( IS_MINDFLAYER( ch ) )
        snprintf( print_buf, MSL, "&cKinetic : %-6d of %6d", ch->mana, ch->max_mana );
    else if ( IS_BLOODCLASS( ch ) )
        snprintf( print_buf, MSL, "&RBlood    : %-6d of %6d", ch->blood, ch->max_blood );
    else
        snprintf( print_buf, MSL, "&BMana     : %-6d of %6d", ch->mana, ch->max_mana );
    pager_printf( ch, "%s", "\r\n" );

    snprintf( buf, MSL, "%d", ch->exp );
    pager_printf( ch, "&CEXP   : %-9.9s   %-29.29s   &CMKills : %-5.5d    AutoLoot [%c]\r\n", buf,
                  print_buf, ch->pcdata->mkills, xIS_SET( ch->act, PLR_AUTOLOOT ) ? '*' : ' ' );
    pager_printf( ch,
                  "                    &GMove     : %-6d of %6d&C   Mdeaths: %-5.5d    AutoTrash[%c]\r\n",
                  ch->move, ch->max_move, ch->pcdata->mdeaths, xIS_SET( ch->act,
                                                                        PLR_AUTOTRASH ) ? 'X' :
                  ' ' );
    for ( x = 1; x < MAX_CURR_TYPE; x++ )
        pager_printf( ch, "&C%s:&Y %d  ", cap_curr_types[x], GET_MONEY( ch, x ) );
    send_to_pager( "\r\n&C", ch );

    send_to_pager( "Languages: ", ch );
    for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ ) {
        if ( knows_language( ch, lang_array[iLang], ch ) || ( IS_NPC( ch ) && ch->speaks == 0 ) ) {
            if ( lang_array[iLang] & ch->speaking || ( IS_NPC( ch ) && !ch->speaking ) )
                pager_printf( ch, "%s", "&C" );
            else
                pager_printf( ch, "%s", "&R" );
            pager_printf( ch, "%-10.10s&D ", lang_names[iLang] );
            if ( ++count % 6 == 0 )
                send_to_pager( "\r\n           ", ch );
        }
    }
    if ( count > 0 )
        send_to_pager( "\r\n", ch );
    if ( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
        pager_printf( ch, "You are bestowed with the command(s): %s.\r\n",
                      ch->pcdata->bestowments );

/* Volk - fun stuff, display percentage of areas discovered */
    float                   areaperc = 0;
    AREA_DATA              *pArea;
    FOUND_AREA             *found;
    int                     areacount = 0;
    int                     foundcount = 0;

    for ( pArea = first_asort; pArea; pArea = pArea->next_sort ) {
        if ( !IS_SET( pArea->flags, AFLAG_NODISCOVERY ) || !IS_SET( pArea->flags, AFLAG_UNOTSEE ) ) {
            areacount++;
        }
        for ( found = ch->pcdata->first_area; found; found = found->next ) {
            if ( !strcmp( found->area_name, pArea->name ) )
                foundcount++;

        }
    }

    areaperc = ( foundcount * 100 ) / areacount;

    ch_printf( ch, "&CGlory Points:&W %d&C      Areas discovered:&W %.2g%&D\r\n", ch->quest_curr,
               areaperc );

    if ( ch->morph && ch->morph->morph ) {
        send_to_pager( divider, ch );
        if ( IS_IMMORTAL( ch ) )
            pager_printf( ch, "You are Shapeshifted as %s.\r\n", ch->morph->morph->short_desc );
        else
            pager_printf( ch, "You are Shapeshifted into %s.\r\n", ch->morph->morph->short_desc );
        send_to_pager( divider, ch );
    }

    if ( IS_THIRDCLASS( ch ) )
        ch_printf( ch, "Expratio Ratio: Class 1: %d Class 2: %d Class 3: %d \r\n",
                   ch->firstexpratio, ch->secondexpratio, ch->thirdexpratio );
    if ( IS_SECONDCLASS( ch ) && !IS_THIRDCLASS( ch ) )
        ch_printf( ch, "Expratio Ratio: Class 1: %d Class 2: %d \r\n", ch->firstexpratio,
                   ch->secondexpratio );

    if ( CAN_PKILL( ch ) ) {
        send_to_pager( divider, ch );
        pager_printf( ch,
                      "PKILL DATA:  Pkills (%3.3d)     Illegal Pkills (%3.3d)     Pdeaths (%3.3d)\r\n",
                      ch->pcdata->pkills, ch->pcdata->illegal_pk, ch->pcdata->pdeaths );
    }
    if ( ch->pcdata->deity ) {
        send_to_pager
            ( "----------------------------------------------------------------------------\r\n",
              ch );
        if ( ch->pcdata->favor > 2250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "loved" );
        else if ( ch->pcdata->favor > 2000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "cherished" );
        else if ( ch->pcdata->favor > 1750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "honored" );
        else if ( ch->pcdata->favor > 1500 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "praised" );
        else if ( ch->pcdata->favor > 1250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "favored" );
        else if ( ch->pcdata->favor > 1000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "respected" );
        else if ( ch->pcdata->favor > 750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "liked" );
        else if ( ch->pcdata->favor > 250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "tolerated" );
        else if ( ch->pcdata->favor > -250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "ignored" );
        else if ( ch->pcdata->favor > -750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "shunned" );
        else if ( ch->pcdata->favor > -1000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "disliked" );
        else if ( ch->pcdata->favor > -1250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "dishonored" );
        else if ( ch->pcdata->favor > -1500 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "disowned" );
        else if ( ch->pcdata->favor > -1750 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "abandoned" );
        else if ( ch->pcdata->favor > -2000 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "despised" );
        else if ( ch->pcdata->favor > -2250 )
            snprintf( buf, MAX_STRING_LENGTH, "%s", "hated" );
        else
            snprintf( buf, MAX_STRING_LENGTH, "%s", "damned" );
        pager_printf( ch, "Deity:  %-20s  Favor: %s\r\n", ch->pcdata->deity->name, buf );
    }

    if ( !IS_IMMORTAL( ch ) && ch->pcdata ) {
        if ( ch->pcdata->double_exp_timer > 0 ) {
            send_to_char( "\r\n&RSix Dragon's Player Triple Experience Status Bar\r\n", ch );
            if ( ch->pcdata->double_exp_timer < 10 )
                ch_printf( ch, "&C[&R|             &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 20 && ch->pcdata->double_exp_timer >= 10 )
                ch_printf( ch, "&C[&R||            &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 30 && ch->pcdata->double_exp_timer >= 20 )
                ch_printf( ch, "&C[&R|||           &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 40 && ch->pcdata->double_exp_timer >= 30 )
                ch_printf( ch, "&C[&R|||&O|        &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 50 && ch->pcdata->double_exp_timer >= 40 )
                ch_printf( ch, "&C[&R|||&O||       &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 60 && ch->pcdata->double_exp_timer >= 50 )
                ch_printf( ch, "&C[&R|||&O|||      &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 70 && ch->pcdata->double_exp_timer >= 60 )
                ch_printf( ch, "&C[&R|||&O|||&Y|    &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 80 && ch->pcdata->double_exp_timer >= 70 )
                ch_printf( ch, "&C[&R|||&O|||&Y||   &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 90 && ch->pcdata->double_exp_timer >= 80 )
                ch_printf( ch, "&C[&R|||&O|||&Y|||  &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 100 && ch->pcdata->double_exp_timer >= 90 )
                ch_printf( ch, "&C[&R|||&O|||&Y|||| &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer >= 100 )
                ch_printf( ch, "&C[&R|||&O|||&Y||||&G|&C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            send_to_char( "\r\n&CNote: See also help solve, and help triple exp.&D\r\n", ch );
        }
    }

    if ( IS_IMMORTAL( ch ) ) {
        send_to_pager( divider, ch );
        pager_printf( ch, "&rSix Dragons&C STAFF DATA: Wizinvis [%c]  Wizlevel (%d)\r\n",
                      xIS_SET( ch->act, PLR_WIZINVIS ) ? '*' : ' ', ch->pcdata->wizinvis );
        pager_printf( ch, "&CBamfin       : %s\r\n",
                      VLD_STR( ch->pcdata->bamfin ) ? ch->
                      pcdata->bamfin :
                      "&C[OoC]: $n please &Rvote&C for us at &Yhttp://6dragons.org/bin/vote.html &Cthis message suddenly &REXPLODES!&c and $n is standing here!" );
        pager_printf( ch, "&CBamfout      : %s&C\r\n",
                      VLD_STR( ch->pcdata->bamfout ) ? ch->
                      pcdata->bamfout :
                      "&C[OoC]: $n please &Rvote&C for us at &Yhttp://6dragons.org/bin/vote.html &Cthis message suddenly &REXPLODES!&c and $n is GONE!" );
        /*
         * Area Loaded info - Scryn 8/11
         */
        if ( ch->pcdata->area ) {
            pager_printf( ch,
                          "Vnums        : Room (%-5.5d - %-5.5d)  Object (%-5.5d - %-5.5d)  Mob (%-5.5d - %-5.5d)\r\n",
                          ch->pcdata->area->low_r_vnum, ch->pcdata->area->hi_r_vnum,
                          ch->pcdata->area->low_o_vnum, ch->pcdata->area->hi_o_vnum,
                          ch->pcdata->area->low_m_vnum, ch->pcdata->area->hi_m_vnum );
            pager_printf( ch, "Area Loaded [%c]\r\n",
                          ( IS_SET( ch->pcdata->area->status, AREA_LOADED ) ) ? '*' : ' ' );
        }
    }
    return;
}

//This will display only current and needed for each class level
//to those whom are blind. -Taon
//Note: I slapped this together quickly, I plan on making it more
//compact when I have some more time. -Taon
void blind_level( CHAR_DATA *ch )
{
    ch_printf( ch, "\r\n&cClass: &C%s&c\r\n", class_table[ch->Class]->who_name );
    if ( !IS_SECONDCLASS( ch ) && !IS_THIRDCLASS( ch ) ) {
        ch_printf( ch, "Level: %d\r\n", ch->level );
        ch_printf( ch, "You currently have %d experience points.\r\n", ch->exp );
        ch_printf( ch, "You need %d more experience to achieve next level.\r\n\r\n",
                   ( exp_level( ch, ch->level + 1 ) - ch->exp ) );
    }
    else {
        ch_printf( ch, "Class level: %d\r\n", ch->firstlevel );
        ch_printf( ch, "You currently have %d experience points.\r\n", ch->firstexp );
        ch_printf( ch, "You need %d to achieve next level.\r\n\r\n",
                   ( exp_class_level( ch, ch->firstlevel + 1, ch->Class ) - ch->firstexp ) );
    }

    if ( IS_SECONDCLASS( ch ) ) {
        ch_printf( ch, "\r\nSecond Class: &C%s&c\r\n", class_table[ch->secondclass]->who_name );
        ch_printf( ch, "Class level: %d\r\n", ch->secondlevel );
        ch_printf( ch, "You currently have %d experience points.\r\n", ch->secondexp );
        ch_printf( ch, "You need %d to achieve next level.\r\n\r\n",
                   ( exp_class_level( ch, ch->secondlevel + 1, ch->secondclass ) -
                     ch->secondexp ) );
    }

    if ( IS_THIRDCLASS( ch ) ) {
        ch_printf( ch, "\r\n&cThird Class: &C%s&c\r\n", class_table[ch->thirdclass]->who_name );
        ch_printf( ch, "Class level: %d\r\n", ch->thirdlevel );
        ch_printf( ch, "You currently have %d experience points.\r\n", ch->thirdexp );
        ch_printf( ch, "You need %d to achieve next level.\r\n\r\n",
                   ( exp_class_level( ch, ch->thirdlevel + 1, ch->thirdclass ) - ch->thirdexp ) );
    }

    if ( ch->pcdata->tradeclass == 20 ) {
        ch_printf( ch, "\r\n&cClass:&C Blacksmith &c \r\n" );
        ch_printf( ch, "Trade level: %d\r\n", ch->pcdata->tradelevel );
        ch_printf( ch, "You currently have %d crafting points.\r\n", ch->pcdata->craftpoints );
        ch_printf( ch, "You need %d to achieve next trade level.\r\n\r\n",
                   exp_craft_level( ch, ch->pcdata->tradelevel + 1,
                                    ch->pcdata->tradeclass ) - ch->pcdata->craftpoints );
    }
    if ( ch->pcdata->tradeclass == 21 ) {
        ch_printf( ch, "\r\n&cClass:&C Baker &c \r\n" );
        ch_printf( ch, "Trade level: %d\r\n", ch->pcdata->tradelevel );
        ch_printf( ch, "You currently have %d crafting points.\r\n", ch->pcdata->craftpoints );
        ch_printf( ch, "You need %d to achieve next trade level.\r\n\r\n",
                   exp_craft_level( ch, ch->pcdata->tradelevel + 1,
                                    ch->pcdata->tradeclass ) - ch->pcdata->craftpoints );
    }
    if ( ch->pcdata->tradeclass == 22 ) {
        ch_printf( ch, "\r\n&cClass:&C Tanner &c \r\n" );
        ch_printf( ch, "Trade level: %d\r\n", ch->pcdata->tradelevel );
        ch_printf( ch, "You currently have %d crafting points.\r\n", ch->pcdata->craftpoints );
        ch_printf( ch, "You need %d to achieve next trade level.\r\n\r\n",
                   exp_craft_level( ch, ch->pcdata->tradelevel + 1,
                                    ch->pcdata->tradeclass ) - ch->pcdata->craftpoints );
    }
    if ( ch->pcdata->double_exp_timer > 0 ) {
        send_to_char( "\r\n&RSix Dragon's Player Triple Experience Status Bar\r\n", ch );
        ch_printf( ch, "&C[&W%d&C]", ch->pcdata->double_exp_timer );
        send_to_char( "\r\n&CNote: See also help solve, and help triple exp.&D\r\n", ch );
    }

    if ( IS_GROUPED( ch ) && !happyhouron )
        send_to_char( "&WYou will receive Double Experience for being grouped.\r\n", ch );
    else if ( IS_GROUPED( ch ) && happyhouron )
        send_to_char
            ( "&WYou will receive Triple Experience for being grouped during HAPPY HOUR!\r\n", ch );
    else if ( ( doubleexp || sysdata.daydexp ) && ( !ch->pcdata || !ch->pcdata->getsdoubleexp ) )
        send_to_char( "&WYou will receive Double Experience.&D\r\n", ch );
    else if ( ch->pcdata && ch->pcdata->getsdoubleexp )
        send_to_char( "&WYou will receive Triple Experience.&D\r\n", ch );
}

/*
 * -Thoric
 * Display your current exp, level, and surrounding level exp requirements
 */
void do_level( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    buf2[MSL];
    int                     x,
                            lowlvl,
                            hilvl;

    set_char_color( AT_SCORE, ch );

    if ( xIS_SET( ch->act, PLR_BLIND ) ) {
        blind_level( ch );
        return;
    }

    // Heavily modified, for multi-classing support. -Taon
    // Pulled first FOR loop out, ifchecks use less memory for what's needed - good idea
    // though

    ch_printf( ch, "\r\n&cClass:&C %s &c \r\n", class_table[ch->Class]->who_name );

    if ( !IS_SECONDCLASS( ch ) && !IS_THIRDCLASS( ch ) ) {
        snprintf( buf, MSL, " exp  (Current: %12s)", num_punct( ch->exp ) );
        snprintf( buf2, MSL, " exp  (Needed:  %12s)",
                  num_punct( exp_level( ch, ch->level + 1 ) - ch->exp ) );

        lowlvl = UMAX( 1, ch->level );
        hilvl = URANGE( ch->level, ch->level + 1, MAX_LEVEL );

        for ( x = lowlvl; x <= hilvl; x++ )
            ch_printf( ch, "(%2d) %12s%s\r\n", x, num_punct( exp_level( ch, x ) ),
                       ( x == ch->level ) ? buf : ( x == ch->level + 1 ) ? buf2 : " exp" );
    }
    else {
        snprintf( buf, MSL, " exp  (Current: %12s)", num_punct( ch->firstexp ) );
        snprintf( buf2, MSL, " exp  (Needed:  %12s)",
                  num_punct( exp_class_level( ch, ch->firstlevel + 1, ch->Class ) -
                             ch->firstexp ) );

        lowlvl = UMAX( 1, ch->firstlevel );
        hilvl = URANGE( ch->firstlevel, ch->firstlevel + 1, MAX_LEVEL );

        for ( x = lowlvl; x <= hilvl; x++ )
            ch_printf( ch, "(%2d) %12s%s\r\n", x, num_punct( exp_class_level( ch, x, ch->Class ) ),
                       ( x == ch->firstlevel ) ? buf : ( x ==
                                                         ch->firstlevel + 1 ) ? buf2 : " exp" );
    }

    if ( IS_SECONDCLASS( ch ) ) {
        ch_printf( ch, "\r\n&cClass:&C %s &c \r\n", class_table[ch->secondclass]->who_name );
        snprintf( buf, MSL, " exp  (Current: %12s)", num_punct( ch->secondexp ) );
        snprintf( buf2, MSL, " exp  (Needed:  %12s)",
                  num_punct( exp_class_level( ch, ch->secondlevel + 1, ch->secondclass ) -
                             ch->secondexp ) );

        lowlvl = UMAX( 1, ch->secondlevel );
        hilvl = URANGE( ch->secondlevel, ch->secondlevel + 1, MAX_LEVEL );

        for ( x = lowlvl; x <= hilvl; x++ )
            ch_printf( ch, "(%2d) %12s%s\r\n", x,
                       num_punct( exp_class_level( ch, x, ch->secondclass ) ),
                       ( x == ch->secondlevel ) ? buf : ( x ==
                                                          ch->secondlevel + 1 ) ? buf2 : " exp" );
    }

    if ( IS_THIRDCLASS( ch ) ) {
        ch_printf( ch, "\r\n&cClass:&C %s &c \r\n", class_table[ch->thirdclass]->who_name );
        snprintf( buf, MSL, " exp  (Current: %12s)", num_punct( ch->thirdexp ) );
        snprintf( buf2, MSL, " exp  (Needed:  %12s)",
                  num_punct( exp_class_level( ch, ch->thirdlevel + 1, ch->thirdclass ) -
                             ch->thirdexp ) );

        lowlvl = UMAX( 1, ch->thirdlevel );
        hilvl = URANGE( ch->thirdlevel, ch->thirdlevel + 1, MAX_LEVEL );

        for ( x = lowlvl; x <= hilvl; x++ )
            ch_printf( ch, "(%2d) %12s%s\r\n", x,
                       num_punct( exp_class_level( ch, x, ch->thirdclass ) ),
                       ( x == ch->thirdlevel ) ? buf : ( x ==
                                                         ch->thirdlevel + 1 ) ? buf2 : " exp" );
    }

    if ( ch->pcdata->tradeclass == 20 || ch->pcdata->tradeclass == 21
         || ch->pcdata->tradeclass == 22 ) {
        lowlvl = UMAX( 1, ch->pcdata->tradelevel );
        hilvl = URANGE( ch->pcdata->tradelevel, ch->pcdata->tradelevel + 1, 20 );

        if ( ch->pcdata->tradeclass == 20 )
            ch_printf( ch, "\r\n&cClass:&C Blacksmith &c \r\n" );
        else if ( ch->pcdata->tradeclass == 21 )
            ch_printf( ch, "\r\n&cClass:&C Baker &c \r\n" );
        else if ( ch->pcdata->tradeclass == 22 )
            ch_printf( ch, "\r\n&cClass:&C Tanner &c \r\n" );

        snprintf( buf, MSL, " craftpoints  (Current: %12s)", num_punct( ch->pcdata->craftpoints ) );
        snprintf( buf2, MSL, " craftpoints  (Needed:  %12s)",
                  num_punct( exp_craft_level
                             ( ch, ch->pcdata->tradelevel + 1,
                               ch->pcdata->tradeclass ) - ch->pcdata->craftpoints ) );

        for ( x = lowlvl; x <= hilvl; x++ )
            ch_printf( ch, "(%2d) %12s%s\r\n", x,
                       num_punct( exp_craft_level( ch, x, ch->pcdata->tradeclass ) ),
                       ( x == ch->pcdata->tradelevel ) ? buf : ( x ==
                                                                 ch->pcdata->tradelevel +
                                                                 1 ) ? buf2 : " craftpoints" );
    }

    // Moved this outside of the loop. -Taon 
    if ( ch->pcdata ) {
        if ( ch->pcdata->double_exp_timer > 0 ) {
            send_to_char( "\r\n&RSix Dragon's Player Triple Experience Status Bar\r\n", ch );
            if ( ch->pcdata->double_exp_timer < 10 )
                ch_printf( ch, "&C[&R|             &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 20 && ch->pcdata->double_exp_timer >= 10 )
                ch_printf( ch, "&C[&R||            &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 30 && ch->pcdata->double_exp_timer >= 20 )
                ch_printf( ch, "&C[&R|||           &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 40 && ch->pcdata->double_exp_timer >= 30 )
                ch_printf( ch, "&C[&R|||&O|        &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 50 && ch->pcdata->double_exp_timer >= 40 )
                ch_printf( ch, "&C[&R|||&O||       &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 60 && ch->pcdata->double_exp_timer >= 50 )
                ch_printf( ch, "&C[&R|||&O|||      &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 70 && ch->pcdata->double_exp_timer >= 60 )
                ch_printf( ch, "&C[&R|||&O|||&Y|    &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 80 && ch->pcdata->double_exp_timer >= 70 )
                ch_printf( ch, "&C[&R|||&O|||&Y||   &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 90 && ch->pcdata->double_exp_timer >= 80 )
                ch_printf( ch, "&C[&R|||&O|||&Y|||  &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer < 100 && ch->pcdata->double_exp_timer >= 90 )
                ch_printf( ch, "&C[&R|||&O|||&Y|||| &C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );
            if ( ch->pcdata->double_exp_timer >= 100 )
                ch_printf( ch, "&C[&R|||&O|||&Y||||&G|&C] &W%d Percent&D",
                           ch->pcdata->double_exp_timer );

            send_to_char( "\r\n&CNote: See also help solve, and help triple exp.&D\r\n", ch );
        }
        if ( ch->pcdata && ch->pcdata->getsdoubleexp == TRUE ) {
            int                     dexptime =
                3600 - ( current_time - ch->pcdata->last_dexpupdate );

            char                    minsecs[MSL];

            int                     mins = dexptime / 60;
            int                     secs = dexptime - ( mins * 60 );

            if ( mins > 0 )
                snprintf( minsecs, MSL, "%dm %ds", mins, secs );
            else
                snprintf( minsecs, MSL, "%ds", dexptime );

            ch_printf( ch, "&YYou currently have %s of triple exp left!&D\r\n", minsecs );
        }
    }

    if ( IS_GROUPED( ch ) && !happyhouron )
        send_to_char( "&WYou will receive Double Experience for being grouped.\r\n", ch );
    else if ( IS_GROUPED( ch ) && happyhouron )
        send_to_char
            ( "&WYou will receive Triple Experience for being grouped during HAPPY HOUR!\r\n", ch );
    else if ( ( doubleexp || sysdata.daydexp ) && ( !ch->pcdata || !ch->pcdata->getsdoubleexp ) )
        send_to_char( "&WYou will receive Double Experience.&D\r\n", ch );
    else if ( ch->pcdata && ch->pcdata->getsdoubleexp )
        send_to_char( "&WYou will receive Triple Experience.&D\r\n", ch );
}

/* Volk - better way to do this - one function to handle all */
void display_aff( CHAR_DATA *ch, AFFECT_DATA *paf, SKILLTYPE * skill )
{                                                      /* type ie * SKILL_SONG etc */
    ch_printf( ch, "&B%-5s:&w ", skill_tname[skill->type] );

    if ( paf->location == APPLY_AFFECT )
        paf->location == APPLY_EXT_AFFECT;

    if ( paf->location == APPLY_NONE )
        ch_printf( ch, "%-58s &Bat lvl&R %-3d&B for&R %-4d&B seconds.&D\r\n", skill->name,
                   paf->level, paf->duration );
    else {
        ch_printf( ch, "%-14s &W- &PModifies&w %-14s&B", skill->name,
                   a_types[paf->location % REVERSE_APPLY] );

        if ( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE
             || paf->location == APPLY_SUSCEPTIBLE )
            ch_printf( ch, " to &Y%-14s", flag_string( paf->modifier, ris_flags ) );
        else if ( ( paf->location == APPLY_EXT_AFFECT ) && paf->modifier == 0 )
//        ch_printf(ch, " by &Y%-14s", affect_bit_name( &paf->bitvector ));
            ch_printf( ch, " by &Y%-14s", ext_flag_string( &paf->bitvector, a_flags ) );

        else
            ch_printf( ch, " by &Y%-14d", paf->modifier );

        ch_printf( ch, " &Bat lvl&R %-3d&B for&R %-4d&B seconds.&D\r\n", paf->level,
                   paf->duration );
    }
}

/* New affects command for AFKMud by Zarius */
/* Basic Idea from Altanos Codebase */
void do_affected( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    AFFECT_DATA            *paf,
                           *paf_next;
    SKILLTYPE              *skill;

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );
    send_to_char( "&z-=&w[ &WAffects Summary &w]&z=-&D\r\n", ch );
    send_to_char( "&R* &zImbued with: ", ch );
    ch_printf( ch, "&C%s\r\n",
               !xIS_EMPTY( ch->affected_by ) ? ext_flag_string( &ch->affected_by,
                                                                a_flags ) : "&cNothing" );

    if ( ch->immune > 0 ) {
        send_to_char( "&R*&z Immune to:&D ", ch );
        ch_printf( ch, "&C%s\r\n", flag_string( ch->immune, ris_flags ) );
    }
    else
        send_to_char( "&R*&z Immune to:&c Nothing&D\r\n", ch );
    if ( ch->resistant > 0 ) {
        send_to_char( "&R*&z Resistant to:&D ", ch );
        ch_printf( ch, "&C%s\r\n", flag_string( ch->resistant, ris_flags ) );
    }
    else
        send_to_char( "&R*&z Resistant to:&c Nothing&D\r\n", ch );
    if ( ch->susceptible > 0 ) {
        send_to_char( "&R*&z Susceptible to:&D ", ch );
        ch_printf( ch, "&C%s\r\n", flag_string( ch->susceptible, ris_flags ) );
    }
    else
        send_to_char( "&R*&z Susceptible to:&c Nothing&D\r\n", ch );

    int                     songx = 0,
        spellx = 0,
        skillx = 0;

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;

        if ( ( skill = get_skilltype( paf->type ) ) != NULL ) {
            if ( !str_cmp( skill_tname[skill->type], "spell" ) ) {
                if ( !spellx )
                    send_to_char( "\r\n&z-=&w[ &WSpell Affects &w]&z=-&D\r\n", ch );
                spellx++;
                ch_printf( ch, "&W%2d&z> ", spellx );
                display_aff( ch, paf, skill );
            }
        }
    }

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;

        if ( ( skill = get_skilltype( paf->type ) ) != NULL ) {

            if ( !str_cmp( skill_tname[skill->type], "skill" ) ) {
                if ( !skillx )
                    send_to_char( "\r\n&z-=&w[ &WSkill Affects &w]&z=-&D\r\n", ch );
                skillx++;
                ch_printf( ch, "&W%2d&z> ", skillx );
                display_aff( ch, paf, skill );
            }
        }
    }

    for ( paf = ch->first_affect; paf; paf = paf_next ) {
        paf_next = paf->next;

        if ( ( skill = get_skilltype( paf->type ) ) != NULL ) {

            if ( !str_cmp( skill_tname[skill->type], "song" ) ) {
                if ( !songx )
                    send_to_char( "\r\n&z-=&w[ &WSong Affects &w]&z=-&D\r\n", ch );
                songx++;
                ch_printf( ch, "&W%2d&z> ", songx );
                display_aff( ch, paf, skill );
            }
        }
    }

    if ( !songx && !spellx && !skillx )
        send_to_char( "\r\n&R* &wNo spell, skill or song affects you.\r\n", ch );

    send_to_char( "\r\n&z-=&w[ &WSaves vs Spells Resistances &w]&z=-&D\r\n", ch );
    ch_printf( ch, "&zSpell_Staff   &W- &G%-4d\r\n", ch->saving_spell_staff );
    ch_printf( ch, "&zPara_Petri    &W- &G%-4d\r\n", ch->saving_para_petri );
    ch_printf( ch, "&zWand          &W- &G%-4d\r\n", ch->saving_wand );
    ch_printf( ch, "&zPoison_Death  &W- &G%-4d\r\n", ch->saving_poison_death );
    ch_printf( ch, "&zBreath        &W- &G%-4d\r\n&D", ch->saving_breath );
    return;
}

void do_inventory( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_RED, ch );
    send_to_char( "You are carrying:\r\n", ch );
    show_list_to_char( ch->first_carrying, ch, TRUE, TRUE );
    return;
}

void                    remove_all_equipment( CHAR_DATA *ch );

void do_remort( CHAR_DATA *ch, char *argument )
{
    int                     sn;
    CHQUEST_DATA           *chquest;
    int                     iLang;
    char                    buf[MSL];

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( argument && argument[0] == '\0' ) {
        send_to_char
            ( "\r\nAre you sure you want to remort?  You will have to\r\nstart over with new spells/skills at level 1 and get new equipment.\r\nType remort yes if your sure.\r\n",
              ch );
        return;
    }

    if ( !str_cmp( argument, "yes" )
         && ( IS_AVATAR( ch ) || IS_DUALAVATAR( ch ) || IS_TRIAVATAR( ch ) ) ) {

// set all spells/skills to zero
        for ( sn = 0; sn < top_sn; sn++ ) {
            if ( skill_table[sn]->name
                 && ( sn != gsn_mine && sn != gsn_forge && sn != gsn_tan && sn != gsn_bake
                      && sn != gsn_gather && sn != gsn_mix && sn != gsn_hunt ) ) {
                ch->pcdata->learned[sn] = 0;
                ch->pcdata->dlearned[sn] = 0;
            }
        }
        // HOPEFULLY prevent being purged from pfile code at low levels

        xSET_BIT( ch->act, PLR_EXEMPT );
        if ( !IS_BLOODCLASS( ch ) ) {
            if ( ch->race == RACE_DRAGON ) {
                ch->Class = 21;
                ch->height = 140;
                ch->weight = 325;
            }
            else {
                ch->Class = 20;
                ch->secondclass = -1;
                ch->thirdclass = -1;
            }
        }
        else {
            ch->Class = 22;
            ch->race = 16;
        }
// remove all effects
        if ( ch->first_affect ) {
            while ( ch->first_affect )
                affect_remove( ch, ch->first_affect );
        }

        if ( ch->pcdata->first_quest ) {
            CHQUEST_DATA           *chquest_next;

            for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest_next ) {
                chquest_next = chquest->next;
                UNLINK( chquest, ch->pcdata->first_quest, ch->pcdata->last_quest, next, prev );
                DISPOSE( chquest );
            }
        }

        remove_all_equipment( ch );
        if ( ( iLang = skill_lookup( "common" ) ) == 0 )
            ch->pcdata->learned[iLang] = 100;
        ch->max_hit = 200;
        ch->hit = 200;
        ch->max_mana = 100;
        ch->mana = 100;
        ch->max_move = 100;
        ch->move = 100;
        ch->level = 2;
        ch->firstlevel = -1;
        ch->secondlevel = -1;
        ch->thirdlevel = -1;
        ch->practice = 20;
        ch->pcdata->preglory_hit = 0;
        ch->pcdata->preglory_mana = 0;
        ch->pcdata->preglory_move = 0;
        if ( ch->pcdata->tmproom > 100 ) {
            ch->pcdata->tmproom = 0;
        }
        snprintf( buf, MSL, "&RAnnounce: &Y%s a new %s is reborn among us!", ch->name,
                  capitalize( class_table[ch->Class]->who_name ) );
        echo_to_all( AT_LBLUE, buf, ECHOTAR_ALL );
    }
    return;
}

void do_skills( CHAR_DATA *ch, char *argument )
{
    int                     sn,
                            tshow = SKILL_UNKNOWN;
    int                     col = 0;
    short                   lasttype = SKILL_UNKNOWN,
        cnt = 1;

    set_pager_color( AT_CYAN, ch );

    set_pager_color( AT_CYAN, ch );
    pager_printf_color( ch, " &R                                     Skills&D \r\n" );
    set_pager_color( AT_CYAN, ch );

    for ( sn = 0; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            break;

        if ( skill_table[sn]->type != SKILL_SKILL )
            continue;

        if ( sn == gsn_mine || sn == gsn_forge || sn == gsn_tan || sn == gsn_bake
             || sn == gsn_gather || sn == gsn_mix || sn == gsn_hunt )
            continue;

        if ( !CAN_LEARN( ch, sn, TRUE ) )
            continue;

        if ( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;

        if ( skill_table[sn]->type == SKILL_SKILL ) {
            int                     count,
                                    type_length;

            count++;
            set_pager_color( AT_CYAN, ch );
            pager_printf( ch, "&c%20.20s&D  %3d&D", skill_table[sn]->name,
                          ch->pcdata->learned[sn] );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
    }
}

void do_spells( CHAR_DATA *ch, char *argument )
{
    int                     sn,
                            tshow = SKILL_UNKNOWN;
    int                     col = 0;
    short                   lasttype = SKILL_UNKNOWN,
        cnt = 1;

    set_pager_color( AT_CYAN, ch );

    set_pager_color( AT_CYAN, ch );
    pager_printf_color( ch, " &R                                     Spells&D \r\n" );
    set_pager_color( AT_CYAN, ch );

    for ( sn = 0; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            break;

        if ( skill_table[sn]->type != SKILL_SPELL )
            continue;

        if ( sn == gsn_mine || sn == gsn_forge || sn == gsn_tan || sn == gsn_bake
             || sn == gsn_gather || sn == gsn_mix || sn == gsn_hunt )
            continue;

        if ( !CAN_LEARN( ch, sn, TRUE ) )
            continue;

        if ( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;
        if ( skill_table[sn]->type == SKILL_SPELL ) {
            int                     count,
                                    type_length;

            count++;
            set_pager_color( AT_CYAN, ch );
            pager_printf( ch, "&c%20.20s&D  %3d&D", skill_table[sn]->name,
                          ch->pcdata->learned[sn] );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
    }
}

void do_weapons( CHAR_DATA *ch, char *argument )
{
    int                     sn,
                            tshow = SKILL_UNKNOWN;
    int                     col = 0;
    short                   lasttype = SKILL_UNKNOWN,
        cnt = 1;

    set_pager_color( AT_CYAN, ch );

    set_pager_color( AT_CYAN, ch );
    pager_printf_color( ch, " &R                                     Weapons&D \r\n" );
    set_pager_color( AT_CYAN, ch );

    for ( sn = 0; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            break;

        if ( skill_table[sn]->type != SKILL_WEAPON )
            continue;

        if ( sn == gsn_mine || sn == gsn_forge || sn == gsn_tan || sn == gsn_bake
             || sn == gsn_gather || sn == gsn_mix || sn == gsn_hunt )
            continue;
        if ( !CAN_LEARN( ch, sn, TRUE ) )
            continue;

        if ( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;
        if ( skill_table[sn]->type == SKILL_WEAPON ) {
            int                     count,
                                    type_length;

            count++;
            set_pager_color( AT_CYAN, ch );
            pager_printf( ch, "&c%20.20s&D  %3d&D", skill_table[sn]->name,
                          ch->pcdata->learned[sn] );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
    }
}

/* Need to calc averate rprate score and determine
if a player has voted already once that day on someone 
if a player has already voted tell send them a message
that they can only rprate the same person once per day. - Vladaar */

void do_rprate( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MSL];
    char                    arg2[MIL];
    short                   value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "You cannot rprate mobs.\r\n", ch );
        return;
    }

    if ( !VLD_STR( arg1 ) ) {
        send_to_char( "Syntax: rprate <character> number\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Shame on you, you cannot rprate yourself.\r\n", ch );
        return;
    }

    if ( arg2[0] == '\0' || !is_number( arg2 ) ) {
        send_to_char( "Syntax: rprate <character> number\r\n", ch );
        return;
    }

    value = atoi( arg2 );

    if ( value < 1 || value > 10 ) {
        send_to_char( "Rprate range is 1 to 10.\r\n", ch );
        return;
    }

    // Keep people from going up or down too fast from one person
    if ( victim->pcdata->rprate > 7 && value < 4 ) {
        value += 3;
    }
    if ( victim->pcdata->rprate > 0 && victim->pcdata->rprate < 4 && value > 7 ) {
        value -= 3;
    }

    if ( !victim->pcdata->lastrated || ( ( current_time - 1800 ) > victim->pcdata->lastrated ) ) {
        if ( victim->pcdata->rprate && victim->pcdata->rprate > 0 )
            victim->pcdata->rprate = ( ( value + victim->pcdata->rprate ) / 2 );
        else
            victim->pcdata->rprate = value;
        victim->pcdata->lastrated = current_time;
        ch_printf( ch, "You've rated %s for the hour!\n\r", victim->name );
    }
    else
        send_to_char
            ( "Sorry, that person has been rated in the last hour. Please try again later.\n\r",
              ch );
    return;
}

void do_songs( CHAR_DATA *ch, char *argument )
{
    int                     sn,
                            tshow = SKILL_UNKNOWN;
    int                     col = 0;
    short                   lasttype = SKILL_UNKNOWN,
        cnt = 1;

    set_pager_color( AT_CYAN, ch );

    set_pager_color( AT_CYAN, ch );
    pager_printf_color( ch, " &R                                       Songs&D \r\n" );
    set_pager_color( AT_CYAN, ch );

    for ( sn = 0; sn < top_sn; sn++ ) {
        if ( !skill_table[sn]->name )
            break;

        if ( skill_table[sn]->type != SKILL_SONG )
            continue;

        if ( sn == gsn_mine || sn == gsn_forge || sn == gsn_tan || sn == gsn_bake
             || sn == gsn_gather || sn == gsn_mix || sn == gsn_hunt )
            continue;
        if ( !CAN_LEARN( ch, sn, TRUE ) )
            continue;

        if ( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;
        if ( skill_table[sn]->type == SKILL_SONG ) {
            int                     count,
                                    type_length;

            count++;
            set_pager_color( AT_CYAN, ch );
            pager_printf( ch, "&c%20.20s&D  %3d&D", skill_table[sn]->name,
                          ch->pcdata->learned[sn] );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
    }
}

void do_equipment( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    int                     iWear;
    bool                    found;
    bool                    empty_slot;
    int                     start,
                            cond;

    set_char_color( AT_RED, ch );
    send_to_char( "You are using:       Condition    Equipment\r\n", ch );
    found = FALSE;
    set_char_color( AT_OBJECT, ch );
    for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
        empty_slot = TRUE;

        for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
            if ( obj->wear_loc == iWear ) {
                empty_slot = FALSE;
                if ( ( !IS_NPC( ch ) ) && ( ch->race > 0 ) && ( ch->race < MAX_PC_RACE ) )
                    send_to_char( race_table[ch->race]->where_name[iWear], ch );
                else
                    send_to_char( where_name[iWear], ch );
                if ( can_see_obj( ch, obj ) ) {
                    colorize_equipment( obj, ch );
                    switch ( obj->item_type ) {
                        case ITEM_ARMOR:
                            if ( obj->value[1] == 0 )
                                obj->value[1] = 1;     /* Crash fix - Vladaar */
                            cond = ( int ) ( ( 10 * obj->value[0] / obj->value[1] ) );
                            break;
                        case ITEM_WEAPON:
                            cond = ( int ) ( ( 10 * obj->value[0] / 12 ) );
                            break;
                        default:
                            cond = -1;
                            break;
                    }
                    send_to_char( "&C<&R", ch );
                    if ( cond >= 0 ) {
                        if ( !IS_BLIND( ch ) ) {
                            for ( start = 1; start <= 10; start++ ) {
                                if ( start <= cond )
                                    send_to_char( "+", ch );
                                else
                                    send_to_char( " ", ch );
                            }
                        }
                        else {
                            if ( cond >= 10 )
                                send_to_char( "perfect", ch );
                            else if ( cond >= 6 )
                                send_to_char( "  good ", ch );
                            else if ( cond >= 3 )
                                send_to_char( "  poor ", ch );
                            else
                                send_to_char( "  bad  ", ch );
                        }
                    }
                    send_to_char( "&C>&D  ", ch );
                    send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
                    send_to_char( "&D\r\n&C", ch );
                }
                else
                    send_to_char( "something.\r\n", ch );
                found = TRUE;
            }
        }
        if ( empty_slot ) {
            if ( !IS_NPC( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_SHOWSLOTS ) ) {
                if ( ( ch->race >= 0 ) && ( ch->race < MAX_PC_RACE ) )
                    send_to_char( race_table[ch->race]->where_name[iWear], ch );
                else
                    send_to_char( where_name[iWear], ch );
                send_to_char( "Nothing.&C\r\n", ch );
            }
        }
    }
    if ( IS_NPC( ch ) )
        send_to_char( "Nothing.\r\n", ch );
    else if ( !found && !IS_SET( ch->pcdata->flags, PCFLAG_SHOWSLOTS ) )
        send_to_char( "Nothing.\r\n", ch );
}

void set_title( CHAR_DATA *ch, char *title )
{
    char                    buf[MSL];

    if ( IS_NPC( ch ) ) {
        bug( "%s", "Set_title: NPC." );
        return;
    }
    if ( isalpha( title[0] ) || isdigit( title[0] ) ) {
        buf[0] = ' ';
        mudstrlcpy( buf + 1, title, MSL - 1 );
    }
    else
        mudstrlcpy( buf, title, MSL );
    if ( VLD_STR( ch->pcdata->title ) )
        STRFREE( ch->pcdata->title );
    ch->pcdata->title = STRALLOC( buf );
    return;
}

void do_title( CHAR_DATA *ch, char *argument )
{
    char                    titlestring[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;
    set_char_color( AT_SCORE, ch );
    if ( IS_SET( ch->pcdata->flags, PCFLAG_NOTITLE ) ) {
        set_char_color( AT_IMMORT, ch );
        send_to_char( "The Six Dragons STAFF prohibit you from changing your title.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "Change your title to what?\r\n", ch );
        return;
    }

    if ( strlen( argument ) > 50 )
        argument[50] = '\0';
    smash_tilde( argument );
    string_stripblink( argument, titlestring );
    set_title( ch, titlestring );
    send_to_char( "Your title is set.\r\n", ch );
    return;
}

/* Set your personal description -Thoric */
void do_description( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    if ( !ch->desc ) {
        bug( "%s", "do_description: no descriptor" );
        return;
    }

    switch ( ch->substate ) {
        default:
            bug( "%s", "do_description: illegal substate" );
            return;

        case SUB_RESTRICTED:
            send_to_char( "You cannot use this command from within another command.\r\n", ch );
            return;

        case SUB_NONE:
            ch->substate = SUB_PERSONAL_DESC;
            ch->dest_buf = ch;
            start_editing( ch, ch->description );
            return;

        case SUB_PERSONAL_DESC:
            if ( VLD_STR( ch->description ) )
                STRFREE( ch->description );
            ch->description = copy_buffer( ch );
            stop_editing( ch );
            return;
    }
}

/* Ripped off do_description for whois bio's -- Scryn*/
void do_bio( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;
    if ( !ch->desc ) {
        bug( "%s", "do_bio: no descriptor" );
        return;
    }

    switch ( ch->substate ) {
        default:
            bug( "%s", "do_bio: illegal substate" );
            return;

        case SUB_RESTRICTED:
            send_to_char( "You cannot use this command from within another command.\r\n", ch );
            return;

        case SUB_NONE:
            ch->substate = SUB_PERSONAL_BIO;
            ch->dest_buf = ch;
            start_editing( ch, ch->pcdata->bio );
            return;

        case SUB_PERSONAL_BIO:
            if ( VLD_STR( ch->pcdata->bio ) )
                STRFREE( ch->pcdata->bio );
            ch->pcdata->bio = copy_buffer( ch );
            stop_editing( ch );
            return;
    }
}

/*
 * New stat and statreport command coded by Morphina
 * Bug fixes by Shaddai
 */

void do_statreport( CHAR_DATA *ch, char *argument )
{
    char                    buf[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( IS_BLOODCLASS( ch ) ) {
        ch_printf( ch, "You report: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n", ch->hit, ch->max_hit,
                   ch->blood, ch->max_blood, ch->move, ch->max_move, ch->exp );
        snprintf( buf, sizeof( buf ), "$n reports: %d/%d hp %d/%d blood %d/%d mv %d xp.", ch->hit,
                  ch->max_hit, ch->blood, ch->max_blood, ch->move, ch->max_move, ch->exp );
        act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
    }
    else {
        ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %d xp.\r\n", ch->hit, ch->max_hit,
                   ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
        snprintf( buf, sizeof( buf ), "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp.", ch->hit,
                  ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
        act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
    }

    ch_printf( ch,
               "Your base stats:    %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.\r\n",
               ch->perm_str, ch->perm_wis, ch->perm_int, ch->perm_dex, ch->perm_con, ch->perm_cha,
               ch->perm_lck );
    snprintf( buf, sizeof( buf ),
              "$n's base stats:    %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.",
              ch->perm_str, ch->perm_wis, ch->perm_int, ch->perm_dex, ch->perm_con, ch->perm_cha,
              ch->perm_lck );
    act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );

    ch_printf( ch,
               "Your current stats: %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.\r\n",
               get_curr_str( ch ), get_curr_wis( ch ), get_curr_int( ch ), get_curr_dex( ch ),
               get_curr_con( ch ), get_curr_cha( ch ), get_curr_lck( ch ) );
    snprintf( buf, sizeof( buf ),
              "$n's current stats: %-2d str %-2d wis %-2d int %-2d dex %-2d con %-2d cha %-2d lck.",
              get_curr_str( ch ), get_curr_wis( ch ), get_curr_int( ch ), get_curr_dex( ch ),
              get_curr_con( ch ), get_curr_cha( ch ), get_curr_lck( ch ) );
    act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
    return;
}

void do_stat( CHAR_DATA *ch, char *argument )
{
    short                   type = 0;

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( argument && argument[0] != '\0' ) {
        if ( !str_cmp( argument, "hp" ) )
            type = 1;
        else if ( !str_cmp( argument, "mana" ) || !str_cmp( argument, "blood" ) )
            type = 2;
        else if ( !str_cmp( argument, "move" ) )
            type = 3;
    }

    if ( type == 1 ) {
        ch_printf( ch, "You have %d out of %d hit points.\r\n", ch->hit, ch->max_hit );
        return;
    }

    if ( type == 2 ) {
        if ( IS_BLOODCLASS( ch ) )
            ch_printf( ch, "You have %d out of %d blood points.\r\n", ch->blood, ch->max_blood );
        else
            ch_printf( ch, "You have %d out of %d mana points.\r\n", ch->mana, ch->max_mana );
        return;
    }

    if ( type == 3 ) {
        ch_printf( ch, "You have %d out of %d movement points.\r\n", ch->move, ch->max_move );
        return;
    }

    if ( IS_BLOODCLASS( ch ) )
        ch_printf( ch,
                   "You have %d out of %d hit points, %d out of %d blood points, %d out of %d movement points and %d experience points.\r\n",
                   ch->hit, ch->max_hit, ch->blood, ch->max_blood, ch->move, ch->max_move,
                   ch->exp );
    else
        ch_printf( ch,
                   "You have %d out of %d hit points, %d out of %d mana points, %d out of %d movement points and %d experience points.\r\n",
                   ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );

    ch_printf( ch,
               "Your base stats are %d strength, %d wisdom, %d intelligence, %d dexterity, %d constitution, %d charisma and %d luck.\r\n",
               ch->perm_str, ch->perm_wis, ch->perm_int, ch->perm_dex, ch->perm_con, ch->perm_cha,
               ch->perm_lck );

    ch_printf( ch,
               "Your current stats are %d strength, %d wisdom, %d intelligence, %d dexterity, %d constitution, %d charisma and %d luck.\r\n",
               get_curr_str( ch ), get_curr_wis( ch ), get_curr_int( ch ), get_curr_dex( ch ),
               get_curr_con( ch ), get_curr_cha( ch ), get_curr_lck( ch ) );
}

void do_report( CHAR_DATA *ch, char *argument )
{
    char                    buf[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) && ch->fighting )
        return;

    if ( IS_AFFECTED( ch, AFF_POSSESS ) ) {
        send_to_char( "You can't do that in your current state of mind!\r\n", ch );
        return;
    }

    // Building support for players that use 'screen readers'. -Taon
    if ( xIS_SET( ch->act, PLR_BLIND ) ) {

        if ( IS_BLOODCLASS( ch ) ) {
            ch_printf( ch,
                       "You have %d out of %d hit points, %d out of %d blood points, %d out of %d move points and %d experience points.\r\n",
                       ch->hit, ch->max_hit, ch->blood, ch->max_blood, ch->move, ch->max_move,
                       ch->exp );
            snprintf( buf, MAX_INPUT_LENGTH, "$n reports: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n",
                      ch->hit, ch->max_hit, ch->blood, ch->max_blood, ch->move, ch->max_move,
                      ch->exp );
        }
        else {
            ch_printf( ch,
                       "You have %d out of %d hit points, %d out of %d mana points, %d out of %d move points and %d experience points.\r\n",
                       ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move,
                       ch->exp );
            snprintf( buf, MAX_INPUT_LENGTH, "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp.",
                      ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move,
                      ch->exp );

        }
        act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
        return;
    }

    if ( IS_BLOODCLASS( ch ) ) {
        ch_printf( ch, "You report: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n", ch->hit, ch->max_hit,
                   ch->blood, ch->max_blood, ch->move, ch->max_move, ch->exp );
        snprintf( buf, MAX_INPUT_LENGTH, "$n reports: %d/%d hp %d/%d blood %d/%d mv %d xp.\r\n",
                  ch->hit, ch->max_hit, ch->blood, ch->max_blood, ch->move, ch->max_move, ch->exp );
    }
    else {
        ch_printf( ch, "You report: %d/%d hp %d/%d mana %d/%d mv %d xp.\r\n", ch->hit, ch->max_hit,
                   ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
        snprintf( buf, MAX_INPUT_LENGTH, "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp.", ch->hit,
                  ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp );
    }

    act( AT_REPORT, buf, ch, NULL, NULL, TO_ROOM );
}

/* Show stats of ch */

void do_fprompt( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    set_char_color( AT_GREY, ch );

    if ( IS_NPC( ch ) )
        return;

    smash_tilde( argument );
    one_argument( argument, arg );
    if ( !VLD_STR( arg ) || !str_cmp( arg, "display" ) ) {
        send_to_char( "Your current fighting prompt string:\r\n", ch );
        set_char_color( AT_WHITE, ch );
        ch_printf( ch, "%s\r\n",
                   !VLD_STR( ch->pcdata->fprompt ) ? default_fprompt( ch ) : ch->pcdata->fprompt );
        set_char_color( AT_GREY, ch );
        send_to_char( "Type 'help prompt' for information on changing your prompt.\r\n", ch );
        return;
    }
    send_to_char( "Replacing old fighting prompt of:\r\n", ch );
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "%s\r\n",
               !VLD_STR( ch->pcdata->fprompt ) ? default_fprompt( ch ) : ch->pcdata->fprompt );
    if ( VLD_STR( ch->pcdata->fprompt ) )
        STRFREE( ch->pcdata->fprompt );
    if ( strlen( argument ) > 128 )
        argument[128] = '\0';
    if ( str_cmp( arg, "default" ) )
        ch->pcdata->fprompt = STRALLOC( argument );
    return;
}

void do_prompt( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    set_char_color( AT_GREY, ch );

    if ( IS_NPC( ch ) )
        return;

    smash_tilde( argument );
    one_argument( argument, arg );
    if ( !VLD_STR( arg ) || !str_cmp( arg, "display" ) ) {
        send_to_char( "Your current prompt string:\r\n", ch );
        set_char_color( AT_WHITE, ch );
        ch_printf( ch, "%s&D\r\n",
                   !VLD_STR( ch->pcdata->prompt ) ? default_prompt( ch ) : ch->pcdata->prompt );
        set_char_color( AT_GREY, ch );
        send_to_char( "Type 'help prompt' for information on changing your prompt.\r\n", ch );
        return;
    }
    send_to_char( "Replacing old prompt of:\r\n", ch );
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "%s&D\r\n",
               !VLD_STR( ch->pcdata->prompt ) ? default_prompt( ch ) : ch->pcdata->prompt );

    if ( VLD_STR( ch->pcdata->prompt ) )
        STRFREE( ch->pcdata->prompt );
    if ( strlen( argument ) > 128 )
        argument[128] = '\0';
    if ( str_cmp( arg, "default" ) ) {
        if ( !str_cmp( arg, "standard" ) ) {
            char                    buf[MSL];

            ch->pcdata->prompt = STRALLOC( default_prompt( ch ) );

        }
        else if ( !str_cmp( arg, "current" ) ) {
            char                    buf[MSL];

            ch->pcdata->prompt = STRALLOC( default_prompt( ch ) );

        }
        else
            ch->pcdata->prompt = STRALLOC( argument );
    }
    send_to_char( "With prompt of:\r\n", ch );
    set_char_color( AT_WHITE, ch );
    ch_printf( ch, "%s&D\r\n",
               !VLD_STR( ch->pcdata->prompt ) ? default_prompt( ch ) : ch->pcdata->prompt );
}

/* Get the maximum blood of a player. If mobile, return 0. -Orion */
int GetMaxBlood( CHAR_DATA *ch )
{
    int                     blood = 0;

    if ( IS_IMMORTAL( ch ) ) {
        blood = 300000;
        return blood;
    }
    if ( !IS_NPC( ch ) && IS_BLOODCLASS( ch ) ) {
        if ( ( long int ) GET_TIME_PLAYED( ch ) < 30 )
            blood = ( ch->level + 10 ) + ch->pcdata->apply_blood;
        if ( ( long int ) GET_TIME_PLAYED( ch ) >= 30 && ( long int ) GET_TIME_PLAYED( ch ) < 100 )
            blood = ( ch->level + 15 ) + ch->pcdata->apply_blood;
        if ( ( long int ) GET_TIME_PLAYED( ch ) && ( long int ) GET_TIME_PLAYED( ch ) < 200 )
            blood = ( ch->level + 20 ) + ch->pcdata->apply_blood;
        if ( ( long int ) GET_TIME_PLAYED( ch ) >= 200 )
            blood = ( ch->level + 25 ) + ch->pcdata->apply_blood;
    }
    return blood;
}

void do_extreme( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;
    argument = one_argument( argument, arg );

    if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
        send_to_char( "&GYou are already playing 6D EXTREME!\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        set_char_color( AT_GREEN, ch );
        send_to_char( "To go EXTREME, you MUST type EXTREME YES to do so.\r\n", ch );
        send_to_char( "Remember, this decision is IRREVERSEABLE, so consider it carefully!\r\n",
                      ch );
        return;
    }

    if ( str_cmp( arg, "yes" ) ) {
        set_char_color( AT_GREEN, ch );
        send_to_char( "To go EXTREME, you MUST type EXTREME YES to do so.\r\n", ch );
        send_to_char( "Remember, this decision is IRREVERSEABLE, so consider it carefully!\r\n",
                      ch );
        return;
    }

    if ( !xIS_SET( ch->act, PLR_EXTREME ) )
        xSET_BIT( ch->act, PLR_EXTREME );
    set_char_color( AT_GREEN, ch );
    send_to_char( "You have joined 6D in EXTREME difficulty mode!\r\n", ch );
    log_printf( "%s has gone 6D EXTREME!", ch->name );
    ch->pcdata->extlevel = ch->level;
    return;
}

/* New command for players to become pkillers - Samson 4-12-98 */
void do_deadly( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;
    argument = one_argument( argument, arg );
    if ( IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) {
        send_to_char( "You are already a deadly character!\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        set_char_color( AT_RED, ch );
        send_to_char( "To become a pkiller, you MUST type DEADLY YES to do so.\r\n", ch );
        send_to_char( "Remember, this decision is IRREVERSEABLE, so consider it carefully!\r\n",
                      ch );
        return;
    }
    if ( str_cmp( arg, "yes" ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "To become a pkiller, you MUST type DEADLY YES to do so.\r\n", ch );
        send_to_char( "Remember, this decision is IRREVERSEABLE, so consider it carefully!\r\n",
                      ch );
        return;
    }
    if ( !str_cmp( ch->pcdata->clan_name, "halcyon" ) ) {
        send_to_char
            ( "You cannot be a pkiller in the Halcyon Clan, first you must leave the clan.\r\n",
              ch );
        return;
    }
    if ( !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
        SET_BIT( ch->pcdata->flags, PCFLAG_DEADLY );
    set_char_color( AT_YELLOW, ch );
    send_to_char( "You have joined the ranks of the deadly. The gods cease to protect you!\r\n\r\n",
                  ch );
    if ( ch->secondclass == -1 ) {
        if ( ch->level < 10 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 5 );
        }
        else if ( ch->level >= 10 && ch->level < 30 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 4 );
        }
        else if ( ch->level >= 30 && ch->level < 60 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 3 );
        }
        else if ( ch->level >= 60 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 2 );
        }
        ch->hit = ch->max_hit;
        send_to_char( "&GGravoc's ancient blood fills your veins!\r\n", ch );
    }
    if ( ch->secondclass != -1 && ch->thirdclass == -1 ) {
        if ( ch->level < 10 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 5 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
        }
        else if ( ch->level >= 10 && ch->level < 30 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 4 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
        }
        else if ( ch->level >= 30 && ch->level < 60 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 3 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
        }
        else if ( ch->level >= 60 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 2 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
        }
        ch->hit = ch->max_hit;
        send_to_char( "&GGravoc's ancient blood fills your veins!\r\n", ch );
    }
    if ( ch->secondclass != -1 && ch->thirdclass != -1 ) {
        if ( ch->level < 10 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 5 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
            ch->max_hit = ch->max_hit + ( ch->thirdlevel * 5 );
        }
        else if ( ch->level >= 10 && ch->level < 30 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 4 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
            ch->max_hit = ch->max_hit + ( ch->thirdlevel * 5 );
        }
        else if ( ch->level >= 30 && ch->level < 60 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 3 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
            ch->max_hit = ch->max_hit + ( ch->thirdlevel * 5 );
        }
        else if ( ch->level >= 60 ) {
            ch->max_hit = ch->max_hit + ( ch->level * 2 );
            ch->max_hit = ch->max_hit + ( ch->secondlevel * 5 );
            ch->max_hit = ch->max_hit + ( ch->thirdlevel * 5 );
        }
        ch->hit = ch->max_hit;
        send_to_char( "&GGravoc's ancient blood fills your veins!\r\n", ch );
    }
    log_printf( "%s has become a pkiller!", ch->name );
    return;
}

void do_expire( CHAR_DATA *ch, char *argument )
{
    if ( !ch ) {
        bug( "%s", "do_expire: NULL ch!" );
        return;
    }

    if ( IS_NPC( ch ) )
        return;

    if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
        send_to_char( "No Way! You are a EXTREME player! Why do you need to expire, wimp!\r\n",
                      ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "No Way! .. You're still fighting...\r\n", ch );
        return;
    }
    if ( global_retcode == rCHAR_DIED || global_retcode == rBOTH_DIED || char_died( ch ) )
        return;
    if ( ch->position <= POS_STUNNED && ch->hit <= 0 ) {
        send_to_char( "&RYou have chosen to EXPIRE.\r\n", ch );
        raw_kill( ch, ch );
    }
    else
        send_to_char( "You are unable to EXPIRE at this time.\r\n", ch );
    return;
}

void do_roll( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    int                     chance;
    int                     winamount;
    const char             *rolled1,
                           *rolled2;

    for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room ) {
        if ( IS_NPC( mob ) && xIS_SET( mob->act, ACT_GRALLOC ) )
            break;
    }
    if ( !mob ) {
        send_to_char( "You have to find a Gralloc gambler to do that.\r\n", ch );
        return;
    }
    if ( GET_MONEY( ch, CURR_BRONZE ) < 10 ) {
        send_to_char( "\r\n&cYou do not have enough bronze to play GRALLOC.\r\n", ch );
        return;
    }

    send_to_char( "\r\n&cYou place 10 bronze in the pot and throw the dice.\r\n", ch );
    GET_MONEY( ch, CURR_BRONZE ) -= 10;

    chance = number_range( 1, 7 );
    winamount = 0;
    if ( chance == 1 ) {
        rolled1 = "a GRALLOC!";
        rolled2 = "both knights.";
        winamount = 50;
    }
    else if ( chance == 2 ) {
        rolled1 = "goblin death";
        rolled2 = "a goblin and a skull";
        winamount = -10;
    }
    else if ( chance == 3 ) {
        rolled1 = "witches curse";
        rolled2 = "a witch and a moon";
        winamount = -10;
    }
    else if ( chance == 4 ) {
        rolled1 = "druid's pass, stalemate";
        rolled2 = "a druid and a goblin";
    }
    else if ( chance == 5 ) {
        rolled1 = "a kings treasure";
        rolled2 = "a knight and a king";
        winamount = 10;
    }
    else if ( chance == 6 ) {
        rolled1 = "a hangman's noose";
        rolled2 = "a skull and a skull";
        winamount - 10;
    }
    else {
        rolled1 = "a celestial blessing";
        rolled2 = "a knight and a angel";
        winamount = 10;
    }
    act_printf( AT_LBLUE, ch, NULL, mob, TO_CHAR, "$N says 'You rolled %s!'", rolled1 );
    act_printf( AT_ACTION, ch, NULL, mob, TO_CHAR, "The dice came up as %s.", rolled2 );
    if ( winamount > 0 )                               /* Only change it when they
                                                        * actually gain, they already put 
                                                        * * * 10 * in the pot to play */
        GET_MONEY( ch, CURR_BRONZE ) += winamount;
    if ( winamount < 0 ) {
        winamount = ( 0 - winamount );
        act_printf( AT_ACTION, ch, NULL, mob, TO_CHAR,
                    "$N reaches into the pot and takes %d bronze coins.", winamount );
    }
    else if ( winamount > 0 )
        act_printf( AT_ACTION, ch, NULL, mob, TO_CHAR,
                    "$N reaches into the pot and hands you %d bronze coins.", winamount );
}

const char             *gl_names[] = {
    "mana", "hp", "strength", "intelligence", "wisdom", "dexterity", "constitution",
    "charisma",
    "hitroll", "damroll", "armor", "rename", "rekey",
    "detect_invis", "detect_hidden", "detect_magic", "sneak", "hide",
    "invisible", "infrared", "flying", "floating", "aqua_breath", "scrying", "pass_door",
    "glow", "hum", "loyal", "bless", "metal"
};

const int               gl_costs[] = {
/*  "mana", "hp", "str", "int", "wis", "dex", "con", "cha",  */
    2, 2, 20, 20, 20, 20, 20, 20,
/*  "hitroll", "damroll", "armor", "rename", "rekey",  */
    35, 35, 5, 20, 20,
/*  "det_invis", "det_hidden", "det_magic", "sneak", "hide",  */
    35, 35, 5, 40, 40,
/*  "invis", "infra", "fly", "float", "aqua", "scry", "passdoor",  */
    75, 25, 75, 35, 45, 55, 75,
/*  "glow", "hum", "loyal", "bless", "metal"  */
    15, 10, 50, 40, 25
};

extern int              top_affect;

/* Volk - two functions. One is the meat function (do_glory) the other is basically
 * a handler for any glory affects going on the object. */
void glory_add( CHAR_DATA *ch, OBJ_DATA *obj, char *argument )
{
    if ( !argument || argument[0] == '\0' ) {
        // Volk - give them a list of commands and glory cost here - help glorycosts
        send_to_char
            ( "&cValid stats   - &Pmana hp strength intelligence wisdom dexterity constitution charisma\r\n",
              ch );
        send_to_char( "                &Phitroll damroll armor rename rekey\r\n", ch );
        send_to_char
            ( "&cValid affs    - &Pdetect_invis detect_hidden detect_magic sneak hide invisible infrared\r\n",
              ch );
        send_to_char( "                &Pflying floating aqua_breath scrying pass_door\r\n", ch );
        send_to_char( "&cValid toggles - &Pglow hum loyal bless metal\r\n\r\n", ch );
        send_to_char
            ( "&RPlease type 'glory costs' for prices or see HELP GLORYAFFECTS for more info.\r\n",
              ch );
        return;
    }

    char                    arg[MSL];
    int                     mod = 1,
        cost = 0;
    int                     maxglory = ( obj->level * 2 ) - 1;

    argument = one_argument( argument, arg );

    if ( argument[0] != '\0' && str_cmp( arg, "rekey" ) && str_cmp( arg, "rename" ) ) {
        mod = atoi( argument );
        if ( mod <= 0 )
            mod = 1;
    }

    // Volk - here, so far we have an 'arg' to check fields and a 'mod' if the player
    // wants 
    // more than 1 stat at a time.
    // We also have ch to check glory and obj to check max glory. :)
    int                     x;

    for ( x = 0; x < ( sizeof( gl_names ) / sizeof( gl_names[0] ) ); x++ ) {
        if ( !str_cmp( arg, gl_names[x] ) ) {          /* found it */
            cost = gl_costs[x];
            break;
        }
    }

    if ( cost <= 0 ) {
        send_to_char
            ( "That is not a valid glory affect. Please type 'glory add (object)' without an argument to see a list of affects.\r\n",
              ch );
        return;
    }

    cost *= mod;

    if ( !str_cmp( arg, "armor" ) )                    // armor needs to be NEGATIVE mod
        mod -= mod * 2;

    if ( cost > ( maxglory - obj->glory ) ) {
        ch_printf( ch,
                   "This equals %d glory points. There is currently %d points left on the object.\r\n",
                   cost, ( maxglory - obj->glory ) );
        send_to_char( "There is not enough room! See 'glory stat (object)' for help.\r\n", ch );
        return;
    }

    if ( cost > ch->quest_curr ) {
        ch_printf( ch, "This equals %d glory points. You currently have %d glory points.\r\n", cost,
                   ch->quest_curr );
        send_to_char( "You need more glory to add that affect!\r\n", ch );
        return;
    }

    // Where are we? Oh, right. We have the cost. We know the player has enough glory,
    // and
    // the item has room. Now let's set
    // the owner, add the affect, update ch glory and obj glory and bugger off.
    ch->quest_curr -= cost;
    if ( !obj->owner )
        obj->owner = STRALLOC( ch->name );
    obj->glory += cost;

    if ( !str_cmp( arg, "hp" ) )                       // silly yes, but players know hp, 
                                                       // 
        // 
    {
        AFFECT_DATA            *paf;

        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = APPLY_HIT;
        paf->modifier = mod;
        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        ++top_affect;
        combine_affects( obj );
        send_to_char( "Done.\r\n", ch );
        save_char_obj( ch );
        return;
    }

/* DIRTY hack workarounds for infravision and aqua breath 
  if(!str_cmp(arg, "infravision"))
  {
    AFFECT_DATA *paf;

    CREATE(paf, AFFECT_DATA, 1);
    paf->type = -1;
    paf->duration = -1;
    paf->location = APPLY_AFFECT;
    SET_BIT(paf->modifier, 1 << get_aflag((char *)"infravision") );
    xCLEAR_BITS(paf->bitvector);
    LINK(paf, obj->first_affect, obj->last_affect, next, prev);
    ++top_affect;
    send_to_char("Done.\r\n", ch);
    save_char_obj(ch);
    return;
  
  }
*/
    if ( !str_cmp( arg, "aqua_breath" ) ) {
        AFFECT_DATA            *paf;

        CREATE( paf, AFFECT_DATA, 1 );

        paf->type = -1;
        paf->duration = -1;
        paf->location = APPLY_AFFECT;
        SET_BIT( paf->modifier, 1 << get_aflag( ( char * ) "aqua_breath" ) );
        xCLEAR_BITS( paf->bitvector );
        LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        ++top_affect;
        send_to_char( "Done.\r\n", ch );
        save_char_obj( ch );
        return;

    }

// Sigh, been trying to get around this but.. let's set the affect now? We still have x..

//deal with rename/rekey FIRST then lump the rest

    set_char_color( AT_CYAN, ch );

    if ( !str_cmp( arg, "rename" ) ) {
        if ( argument && argument[0] != '\0' ) {
            ch_printf( ch, "&cChanging item's description from &C%s&c to&C %s&c..\r\n",
                       obj->short_descr, argument );
            STRFREE( obj->short_descr );
            obj->short_descr = STRALLOC( argument );
            send_to_char
                ( "&RDone!&c Please note, the KEYWORDS used to drop/equip/etc remain the same.\r\n",
                  ch );
            save_char_obj( ch );
        }
        else
            send_to_char
                ( "You must specify the new keywords, ie 'rename whatever' or 'rename big pink sword'.\r\n",
                  ch );
        return;
    }

    if ( !str_cmp( arg, "rekey" ) ) {
        if ( argument && argument[0] != '\0' ) {
            ch_printf( ch, "&cChanging item's keywords from &C%s&c to &C%s&c..\r\n", obj->name,
                       argument );
            STRFREE( obj->name );
            obj->name = STRALLOC( argument );
            send_to_char
                ( "&RDone!&c Please note, the DESCRIPTION when looking at the item remains the same.\r\n",
                  ch );
            save_char_obj( ch );
        }
        else
            send_to_char
                ( "You must specify the new keywords, ie 'rekey whatever' or 'rekey big pink sword'.\r\n",
                  ch );
        return;
    }

    /*
     * Now everything else.. 
     */
    AFFECT_DATA            *paf;
    short                   loc = 0;
    int                     value = mod;
    int                     bitv = 0;

    loc = get_atype( arg );

    if ( loc < 1 || loc >= APPLY_AFFECT ) {            /* If didn't find str, int, con
                                                        * etc.. must be -1, * should be
                                                        * APPLY_AFFECT */
        loc = APPLY_AFFECT;
        value = get_aflag( arg );                      /* Now looking for invisible,
                                                        * flying etc.. */

        if ( value < 0 || value >= MAX_AFFECTED_BY ) { /* Didn't find it either! Must be
                                                        * object * flag. */
            value = get_oflag( arg );
            if ( value < 0 || value > MAX_BITS ) {     /* I'm stumped, we should NEVER
                                                        * get here! bug out * heh */
                send_to_char( "No such glory affect.\r\n", ch );
                bug( "Player %s tried to buy unknown glory affect %s!\r\n", ch->name, arg );
                ch->quest_curr += cost;
                if ( obj->owner )
                    STRFREE( obj->owner );
                obj->glory -= cost;
                return;
            }
            xTOGGLE_BIT( obj->extra_flags, value );
            send_to_char( "Done.\r\n", ch );
            save_char_obj( ch );
            return;
        }
        else
            SET_BIT( bitv, 1 << value );
    }

    if ( bitv > 0 )
        value = bitv;

    CREATE( paf, AFFECT_DATA, 1 );

    paf->type = -1;
    paf->duration = -1;
    paf->location = loc;
    paf->modifier = value;
    xCLEAR_BITS( paf->bitvector );
    paf->next = NULL;
    LINK( paf, obj->first_affect, obj->last_affect, next, prev );
    ++top_affect;
    combine_affects( obj );
    send_to_char( "Done.\r\n", ch );
    save_char_obj( ch );
    return;
}

void do_glory( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MSL],
                            arg2[MSL];
    OBJ_DATA               *obj;
    int                     max;

    argument = one_argument( argument, arg1 );
    set_char_color( AT_CYAN, ch );

    if ( !str_cmp( arg1, "hp" ) ) {
        if ( ch->pcdata->preglory_hit >= 2000 ) {
            send_to_char( "You already are at your maximum hp you may gain.\r\n", ch );
            return;
        }

        if ( ch->max_hit >= 10000 && ch->race != RACE_DRAGON ) {
            send_to_char( "You are already at maximum hp that a player may posess.\r\n", ch );
            return;
        }

        if ( ch->max_hit >= 15000 && ch->race == RACE_DRAGON ) {
            send_to_char( "You are already at maximum hp that a dragon may posess.\r\n", ch );
            return;
        }

        if ( 25 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 25;
        ch->max_hit += 25;
        ch->pcdata->preglory_hit += 25;
        ch->hit = ch->max_hit;
        send_to_char( "Your maximum hp has just increased by 25!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "move" ) ) {
        if ( ch->pcdata->preglory_move >= 2000 ) {
            send_to_char( "You already are at your maximum move you may gain.\r\n", ch );
            return;
        }

        if ( 25 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 25;
        ch->max_move += 25;
        ch->pcdata->preglory_move += 25;
        ch->move = ch->max_move;
        send_to_char( "Your maximum move has just increased by 25!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "mana" ) ) {
        if ( ch->pcdata->preglory_mana >= 2000 ) {
            send_to_char( "You already are at your maximum mana you may gain.\r\n", ch );
            return;
        }

        if ( ch->max_mana >= 10000 ) {
            send_to_char( "You are already at maximum mana that a player may posess.\r\n", ch );
            return;
        }
        if ( 25 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 25;
        ch->max_mana += 25;
        ch->pcdata->preglory_mana += 25;
        ch->mana = ch->max_mana;
        send_to_char( "Your maximum mana has just increased by 25!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "str" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_STR )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_STR )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_STR )
                max = 16;
        }
        if ( ch->perm_str >= max ) {
            send_to_char
                ( "You are already at maximum strength that a player may gain through glory.\r\n",
                  ch );
            return;
        }

        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_str += 1;
        send_to_char( "Your strength has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }
    if ( !str_cmp( arg1, "lck" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_LCK )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_LCK )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_LCK )
                max = 16;
        }
        if ( ch->perm_lck >= max ) {
            send_to_char
                ( "You are already at maximum luck that a player may gain through glory.\r\n", ch );
            return;
        }
        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_lck += 1;
        send_to_char( "Your luck has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "int" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_INT )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_INT )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_INT )
                max = 16;
        }
        if ( ch->perm_int >= max ) {
            send_to_char
                ( "You are already at maximum intelligence that a player may gain through glory.\r\n",
                  ch );
            return;
        }
        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_int += 1;
        send_to_char( "Your intelligence has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "wis" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_WIS )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_WIS )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_WIS )
                max = 16;
        }
        if ( ch->perm_wis >= max ) {
            send_to_char
                ( "You are already at maximum wisdom that a player may gain through glory.\r\n",
                  ch );
            return;
        }
        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_wis += 1;
        send_to_char( "Your wisdom has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "con" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_CON )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_CON )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_CON )
                max = 16;
        }
        if ( ch->perm_con >= max ) {
            send_to_char
                ( "You are already at maximum constitution that a player may gain through glory.\r\n",
                  ch );
            return;
        }
        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_con += 1;
        send_to_char( "Your constitution has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }
    if ( !str_cmp( arg1, "dex" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_DEX )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_DEX )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_DEX )
                max = 16;
        }
        if ( ch->perm_dex >= max ) {
            send_to_char
                ( "You are already at maximum dexterity that a player may gain through glory.\r\n",
                  ch );
            return;
        }
        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_dex += 1;
        send_to_char( "Your dexterity has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg1, "cha" ) ) {
        max = 20;                                      /* Default max for stats */
        if ( class_table[ch->Class] ) {
            if ( class_table[ch->Class]->attr_prime == APPLY_CHA )
                max = 25;
            else if ( class_table[ch->Class]->attr_second == APPLY_CHA )
                max = 22;
            else if ( class_table[ch->Class]->attr_deficient == APPLY_CHA )
                max = 16;
        }
        if ( ch->perm_cha >= max ) {
            send_to_char
                ( "You are already at maximum charisma that a player may gain through glory.\r\n",
                  ch );
            return;
        }
        if ( 30 > ch->quest_curr ) {
            send_to_char( "You don't have enough glory points.\r\n", ch );
            return;
        }
        ch->quest_curr -= 30;

        ch->perm_cha += 1;
        send_to_char( "Your charisma has just increased by one!\r\n", ch );
        save_char_obj( ch );
        return;
    }

    if ( argument && argument[0] != '\0' ) {
        argument = one_argument( argument, arg2 );
        if ( ( obj = get_obj_carry( ch, arg2 ) ) == NULL ) {
            send_to_char( "The object must be in your inventory.\r\n", ch );
            return;
        }

        int                     maxglory = ( obj->level * 2 ) - 1;

        if ( !str_cmp( arg1, "add" ) ) {
            if ( obj->owner && str_cmp( ch->name, obj->owner ) )
                send_to_char( "Only the owner of an object can delete the glory affects on it.\r\n",
                              ch );
            else
                glory_add( ch, obj, argument );
            return;
        }

        if ( !str_cmp( arg1, "delete" ) ) {
            if ( !obj->owner || str_cmp( ch->name, obj->owner ) )
                send_to_char( "Only the owner of an object can delete the glory affects on it.\r\n",
                              ch );
            else if ( obj->glory <= 0 )
                send_to_char( "That object has NO glory affects on it - nothing to delete.\r\n",
                              ch );
            else if ( !strcmp( argument, "YES" ) ) {
                int                     vnum = obj->pIndexData->vnum;
                int                     lvl = obj->level;

                ch_printf( ch,
                           "&RAll glory affects stripped from &C%s&R. &cThis is &C%d &cpoints worth!\r\n",
                           obj->short_descr, obj->glory );
                separate_obj( obj );
                extract_obj( obj );
                obj = create_object( get_obj_index( vnum ), lvl );
                obj->glory = 0;
                obj_to_char( obj, ch );
            }
            else {
                ch_printf( ch, "Are you SURE you want to remove all glory affects from %s?\r\n",
                           obj->short_descr );
                ch_printf( ch,
                           "This is NON REVERSIBLE, no exceptions. If you wish to continue, please write 'glory delete (obj) YES'.\r\n" );
            }
            return;
        }

        if ( !str_cmp( arg1, "stat" ) ) {
            ch_printf( ch, "\r\n&cOBJ:&C %-20s  &cLEVEL:&C %d\r\n", obj->short_descr, obj->level );
            ch_printf( ch, "&cGlory:&C %-3d&c/&C%-3d\r\n", obj->glory, maxglory );
            if ( VLD_STR( obj->owner ) )
                ch_printf( ch, "&cOwner:&C %s\r\n", obj->owner );
            ch_printf( ch, "\r\n&C%s &chas &C%d&c glory points available to fill.\r\n",
                       obj->short_descr, maxglory - obj->glory );
            return;
        }
    }

    if ( !str_cmp( arg1, "costs" ) ) {
        send_to_char( "\r\n\t\t\t   &CGLORY COSTS TO ITEM AFFECTS\r\r\n\n", ch );
        for ( int x = 0; x < ( sizeof( gl_names ) / sizeof( gl_names[0] ) ); x++ ) {
            ch_printf( ch, "&c%-15s &P%5d     ", gl_names[x], gl_costs[x] );    /* found
                                                                                 * it */
            if ( ( x + 1 ) % 3 == 0 )
                send_to_char( "\r\n", ch );
        }
        send_to_char( "\r\n                  &CGLORY COSTS TO PLAYER PERMANENT AFFECTS\r\n", ch );
        send_to_char
            ( "\r\n        &cmana          &P25  &chp          &P25    &cmove          &P25\r\n        &cstr           &P30  &cint         &P30    &cwis           &P30\r\n        &cdex           &P30  &ccon         &P30    &ccha           &P30\r\n        &clck           &P30\r\n",
              ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Note player affects increase hp/mana/move by 25, abilities only by 1.\r\n",
                      ch );
        send_to_char
            ( "You will only be able to increase hp/mana/move by a maximum of 2000 over your current max.\r\r\n\n",
              ch );
        send_to_char
            ( "\r\n&cSee also &WHELP GLORY, CONTRIBUTE, GLORYAFFECTS, VOTE, REMINDER, DONATE&D\r\n",
              ch );
        return;
    }

    send_to_char( "&cSyntax: glory <&Cfield&c> <&Cobject&c> <&Cbuff&c> <&Cvalue&c>", ch );
    send_to_char( "\r\n&cField being one of: &Padd delete stat costs\r\n\r\n", ch );
    send_to_char( "&CImbuing objects with Glory\r\n", ch );
    send_to_char( "&cie glory add sword bless\r\n", ch );
    send_to_char( "&cie glory add sword mana 5\r\n", ch );
    send_to_char( "&cie glory delete sword - (remove all glory affects)\r\n", ch );
    send_to_char( "&cie glory stat sword - (view owner and total glory)&w\r\n", ch );
    send_to_char( "&cie glory costs - (view all glory affects & prices)&w\r\n", ch );
    send_to_char( "&CImbuing yourself directly with Glory\r\n", ch );
    send_to_char( "&cie glory <&Caffect&c>\r\n", ch );
    send_to_char
        ( "\r\n&cSee also &WHELP GLORY, CONTRIBUTE, GLORYAFFECTS, VOTE, REMINDER, DONATE&D\r\n",
          ch );
    return;
}

void do_train( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    SKILLTYPE              *skill = NULL;
    char                    arg[MIL],
                            buf[MIL];
    int                     sn;

    if ( sn == gsn_mine || sn == gsn_forge || sn == gsn_bake || sn == gsn_gather ) {
        send_to_char( "\r\nYou cannot train another in a tradeskill.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {          /* We're checking the 'what' here */
        victim = ( CHAR_DATA * ) ch->dest_buf;

        if ( !victim ) {
            send_to_char( "You have no offers.\r\n", ch );
            if ( ch->alloc_ptr )
                DISPOSE( ch->alloc_ptr );              // Just in case.
            return;
        }
        ch_printf( ch, "%s offered to %s\r\n", PERS( victim, ch ), ch->alloc_ptr );
        return;
    }

    argument = one_argument( argument, arg );

    if ( !str_cmp( arg, "yes" ) ) {
        void                   *safety;
        char                    arg2[MSL];

        victim = ( CHAR_DATA * ) ch->dest_buf;
        ch->dest_buf = NULL;

        if ( !victim ) {
            send_to_char( "Noone's asked you anything.\r\n", ch );
            if ( ch->alloc_ptr )
                DISPOSE( ch->alloc_ptr );
            return;
        }

        if ( ch->alloc_ptr )
            sprintf( arg2, "%s", ch->alloc_ptr );
        else {
            send_to_char( "Not sure what they were trying to train you in.\r\n", ch );
            return;
        }

        if ( ch->alloc_ptr )
            DISPOSE( ch->alloc_ptr );

        sn = skill_lookup( arg2 );
        if ( IS_VALID_SN( sn ) )
            skill = skill_table[sn];
        if ( !skill ) {
            send_to_char( "Not sure what they were trying to train you in.\r\n", ch );
            return;
        }

        if ( char_died( victim ) || victim->in_room != ch->in_room ) {
            ch_printf( ch, "%s is gone.\r\n", PERS( victim, ch ) );
            return;
        }

        act( AT_SAY, "$n says yes to you.\r\n", ch, NULL, victim, TO_VICT );
        act( AT_SAY, "You say yes to $N", ch, NULL, victim, TO_CHAR );

        ch->pcdata->learned[sn] = 5;

        switch ( skill->type ) {
            default:
            case SKILL_SKILL:
                act( AT_GREEN, "\r\nYou perform the skill on them until they appear to grasp it.",
                     victim, NULL, NULL, TO_CHAR );
                act( AT_GREEN,
                     "\r\n$n performs the skill on you until you show that you can do it.\r\n",
                     victim, NULL, ch, TO_VICT );
                act( AT_RED, "\r\nOuch your body so sore from learning you pass out!\r\n", victim,
                     NULL, ch, TO_VICT );
                ch->hit = 0;
                set_position( ch, POS_STUNNED );

                send_to_char( "\r\nYou fall to the ground exhausted from the effort.\r\n", victim );
                victim->move = 0;
                break;

            case SKILL_WEAPON:
                act( AT_GREEN, "\r\nYou perform the skill on them until they appear to grasp it.",
                     victim, NULL, NULL, TO_CHAR );
                act( AT_GREEN,
                     "\r\n$n performs the skill on you until you show that you can do it.\r\n",
                     victim, NULL, ch, TO_VICT );
                act( AT_RED, "\r\nOuch your body so sore from learning you pass out!\r\n", victim,
                     NULL, ch, TO_VICT );
                ch->hit = 0;
                set_position( ch, POS_STUNNED );

                send_to_char( "\r\nYou fall to the ground exhausted from the effort.\r\n", victim );
                victim->move = 0;
                break;

            case SKILL_SPELL:
                act( AT_GREEN,
                     "\r\nYou teach the required componets, and gestures of the magic spell.",
                     victim, NULL, NULL, TO_CHAR );
                act( AT_GREEN,
                     "\r\n$n teaches you the required componets, and gestures of the magic spell.\r\n",
                     victim, NULL, ch, TO_VICT );
                act( AT_RED, "\r\nOuch your head throbs so hard from learning you pass out!\r\n",
                     victim, NULL, ch, TO_VICT );
                ch->hit = 0;
                set_position( ch, POS_STUNNED );

                send_to_char( "\r\nYour mana is exhausted from the toll of teaching the magic.\r\n",
                              victim );
                victim->mana = 0;
                break;

            case SKILL_TONGUE:
                act( AT_GREEN, "\r\nYou teach the basic words of the language.", victim, NULL, NULL,
                     TO_CHAR );
                act( AT_GREEN, "\r\n$n teaches you the basic words of the language.\r\n", victim,
                     NULL, ch, TO_VICT );
                act( AT_RED, "\r\nOuch your head throbs so hard from learning you pass out!\r\n",
                     victim, NULL, ch, TO_VICT );
                ch->hit = 0;
                set_position( ch, POS_STUNNED );

                send_to_char
                    ( "\r\nYour mana is exhausted from the toll of teaching the language.\r\n",
                      victim );
                victim->mana = 0;
                break;

            case SKILL_SONG:
                act( AT_GREEN, "\r\nYou teach the basic words of the song.", victim, NULL, NULL,
                     TO_CHAR );
                act( AT_GREEN, "\r\n$n teaches you the basic words of the song.\r\n", victim, NULL,
                     ch, TO_VICT );
                act( AT_RED, "\r\nOuch your head throbs so hard from learning you pass out!\r\n",
                     victim, NULL, ch, TO_VICT );
                ch->hit = 0;
                set_position( ch, POS_STUNNED );

                send_to_char( "\r\nYour mana is exhausted from the toll of teaching the song.\r\n",
                              victim );
                victim->mana = 0;
                break;
        }
        return;
    }

    if ( !str_cmp( arg, "no" ) ) {
        victim = ( CHAR_DATA * ) ch->dest_buf;

        ch->dest_buf = NULL;
        if ( ch->alloc_ptr )
            DISPOSE( ch->alloc_ptr );

        if ( !victim ) {
            send_to_char( "Noone's asked you anything.\r\n", ch );
            return;
        }
        if ( char_died( victim ) || victim->in_room != ch->in_room ) {
            ch_printf( ch, "%s is gone.\r\n", PERS( victim, ch ) );
            return;
        }
        act( AT_SAY, "$n says no to you.\r\n", ch, NULL, victim, TO_VICT );
        act( AT_SAY, "You say no to $N", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        send_to_char( "There is no one here by that name.\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) ) {
        send_to_char( "You cannot train them.\r\n", ch );
        return;
    }
    if ( victim->dest_buf ) {
        send_to_char( "Someone is already attempting to train them something.\r\n", ch );
        return;
    }
    if ( ( sn = skill_lookup( argument ) ) == -1 ) {
        send_to_char( "There is no such skill.\r\n", ch );
        return;
    }
    if ( !CAN_LEARN( victim, sn, TRUE ) ) {
        send_to_char( "They cannot learn that.\r\n", ch );
        return;
    }
    if ( !LEARNED( ch, sn ) ) {
        send_to_char( "You cannot train others what you do not know.\r\n", ch );
        return;
    }
    if ( victim->pcdata->learned[sn] > 0 ) {
        send_to_char( "They already know that.\r\n", ch );
        return;
    }
    act( AT_SAY, "$n asks you permission to train you $t.", ch, argument, victim, TO_VICT );
    act( AT_SAY, "You ask $N permission to train $M $t.", ch, argument, victim, TO_CHAR );
    victim->alloc_ptr = str_dup( argument );
    victim->dest_buf = ch;
}

/* 
Sammy, new code for you - 'addaffects (obj) (rating) [number]' 
where rating is a number from 1 to 15 with 10 being average powered item, 
and number (OPTIONAL) is the number of affects. No number = randomised.
*/
void do_addaffects( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MSL],
                            arg2[MSL];
    short                   rating = 0;
    OBJ_DATA               *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( !IS_IMMORTAL( ch ) || IS_NPC( ch ) ) {
        send_to_char( "Only STAFF can use this command.\r\n", ch );
        return;
    }

    if ( !arg1 || arg1[0] == '\0' || !arg2 || arg2[0] == '\0' || !is_number( arg2 )
         || ( rating = atoi( arg2 ) ) < 1 || rating > 15 ) {
        send_to_char
            ( "Syntax: 'addaffects (obj) (rating) [number]'\r\n  where (rating) is a number from 1 to 15 (avg 10) and optional (number)\r\n  the number of affects (no number = random)\r\n",
              ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "That object does not exist in your inventory.\r\n", ch );
        return;
    }

    if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
        send_to_char( "Item MUST be set with a prototype flag.\r\n", ch );
        return;
    }

/* For now, we'll work this out the same as glory.. */
    int                     glory = ( obj->level * 2 ) - 1;
    short                   numaffects;

    glory *= rating;
    glory /= 10;
    OBJ_INDEX_DATA         *pObjIndex = obj->pIndexData;

    pObjIndex->rating = glory;

    ch_printf( ch,
               "&cObject has been set at &C%d&c. Taking into account the object's level, this equals &C%d&c points (obj rating).\r\n",
               rating, glory );

    if ( argument && argument[0] != '\0' && is_number( argument ) )
        numaffects = atoi( argument );
    else
        numaffects = number_range( 1, number_range( 1, 5 ) );
    // second number range to 
    // stop a lot of affects
    // on items..

    if ( numaffects <= 0 || numaffects > 5 ) {
        ch_printf( ch,
                   "&cInvalid number of affects - &C%d&c. Please choose a number between 1 and 5.\r\n",
                   numaffects );
        return;
    }

/* Last thing to do - wipe all affects on object. */
    AFFECT_DATA            *paf;

    for ( paf = pObjIndex->first_affect; paf; paf = paf->next ) {
        UNLINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
        DISPOSE( paf );
        --top_affect;
    }

/* What is the best way to randomise glory picks but also get the maximum into the item? I'd say 
   pick biggest first, then fill up on hp/mana/whatever at the end.. */
    int                     picked = 0;
    int                     x = 0,
        y = 0,
        z = 0;

    for ( x = numaffects; x < 1; x-- )                 // add each affect
    {
        if ( x == 1 ) {                                /* Panic mode - no more affects,
                                                        * so let's FILL the item from
                                                        * here. */
            if ( glory > 75 ) {                        /* too much for flag, let's
                                                        * hp/mana it, anything to
                                                        * glory_costs[10] */
                if ( glory % 35 == 0 )                 /* hit/dam */
                    y = number_range( 8, 9 );
                else if ( glory % 20 == 0 )            /* stats */
                    y = number_range( 2, 7 );
                else {
                    y = number_range( 0, 2 );          /* 2 being armor, cheat */
                    if ( y == 2 )
                        y = 10;
                }

            }
            /*
             * Last affect, glory <= 75 
             */

        }

/* Any number of affects, pick one at random */

    }

    return;
}

typedef struct score_data SCORE_DATA;
struct score_data
{
    SCORE_DATA             *next;
    SCORE_DATA             *prev;
    char                   *name;
    int                     score;
};

SCORE_DATA             *first_score;
SCORE_DATA             *last_score;

#define MAX_SCORES 20                                  /* variable for easy adjustment of 
                                                        * how many to display */

/* local functions */
void replace_score      args( ( CHAR_DATA *ch ) );
bool valid_player       args( ( char *name ) );
void fix_score_length   args( (  ) );
int count_scores        args( (  ) );

/*
 * This is the function that actually tallies a persons score and sets it up
 * to add it to the list, be sure to add it to do_save so that saving will adjust
 * the players scores
 */

void calc_score( CHAR_DATA *ch )
{
    int                     score = 0;

    if ( IS_NPC( ch ) )                                /* Mobiles dont have pcdata */
        return;

    if ( IS_IMMORTAL( ch ) || ch->pcdata->tmplevel > 100 )  /* Dont want to clutter your
                                                             * table * with imms. */
        return;

    /*
     * level formula 
     */
    if ( ch->level >= 2 ) {
        score += ch->level / 2;
    }
    score += GET_HITROLL( ch );
    score += GET_DAMROLL( ch );

    if ( GET_AC( ch ) > 0 )
        score -= 5;
    else if ( GET_AC( ch ) <= 0 && GET_AC( ch ) > -99 )
        score += 1;
    else if ( GET_AC( ch ) <= -99 && GET_AC( ch ) > -199 )
        score += 2;
    else if ( GET_AC( ch ) <= -199 && GET_AC( ch ) > -299 )
        score += 3;
    else if ( GET_AC( ch ) <= -299 && GET_AC( ch ) > -399 )
        score += 4;
    else if ( GET_AC( ch ) <= -399 && GET_AC( ch ) > -499 )
        score += 5;
    else if ( GET_AC( ch ) <= -499 && GET_AC( ch ) > -599 )
        score += 6;
    else if ( GET_AC( ch ) <= -599 && GET_AC( ch ) > -699 )
        score += 7;
    else if ( GET_AC( ch ) <= -699 && GET_AC( ch ) > -799 )
        score += 8;
    else if ( GET_AC( ch ) <= -799 && GET_AC( ch ) > -899 )
        score += 9;
    else if ( GET_AC( ch ) <= -899 )
        score += 10;
    if ( get_curr_str( ch ) >= 16 && get_curr_str( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_str( ch ) >= 18 ) {
        score += 2;
    }
    else if ( get_curr_str( ch ) < 16 ) {
        score -= 1;
    }
    if ( get_curr_int( ch ) >= 16 && get_curr_int( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_int( ch ) >= 18 ) {
        score += 2;
    }
    else if ( get_curr_int( ch ) < 12 ) {
        score -= 1;
    }
    if ( get_curr_wis( ch ) >= 16 && get_curr_wis( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_wis( ch ) >= 18 ) {
        score += 2;
    }
    else if ( get_curr_wis( ch ) < 12 ) {
        score -= 1;
    }
    if ( get_curr_lck( ch ) >= 16 && get_curr_lck( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_lck( ch ) >= 18 ) {
        score += 2;
    }
    else if ( get_curr_lck( ch ) < 12 ) {
        score -= 1;
    }
    if ( get_curr_cha( ch ) >= 16 && get_curr_cha( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_cha( ch ) >= 18 ) {
        score += 2;
    }
    if ( get_curr_con( ch ) >= 16 && get_curr_con( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_con( ch ) >= 18 ) {
        score += 2;
    }
    else if ( get_curr_con( ch ) < 16 ) {
        score -= 1;
    }
    if ( get_curr_dex( ch ) >= 16 && get_curr_dex( ch ) < 18 ) {
        score += 1;
    }
    else if ( get_curr_dex( ch ) >= 18 ) {
        score += 2;
    }
    else if ( get_curr_dex( ch ) < 16 ) {
        score -= 1;
    }
    if ( CAN_PKILL( ch ) ) {
        score += ch->pcdata->pkills;
        score -= ch->pcdata->pdeaths;
    }
    if ( ch->pcdata->mkills >= 1000 ) {
        score += ch->pcdata->mkills / 1000;
    }
    if ( ch->pcdata->mdeaths >= 10 ) {
        score -= ch->pcdata->mdeaths / 10;
    }

    if ( ch->max_hit > 5000 && ch->max_hit <= 8000 )
        score += 3;
    else if ( ch->max_hit > 8000 && ch->max_hit <= 9000 )
        score += 5;
    else if ( ch->max_hit > 9000 && ch->max_hit <= 10000 )
        score += 8;
    else if ( ch->max_hit > 10000 )
        score += 10;

    if ( ch->max_mana > 2000 && ch->max_mana <= 3000 )
        score += 3;
    else if ( ch->max_mana > 3000 && ch->max_mana <= 4000 )
        score += 5;
    else if ( ch->max_mana > 4000 && ch->max_mana <= 5000 )
        score += 8;
    else if ( ch->max_mana > 5000 )
        score += 10;

/* Volk - fun stuff, display percentage of areas discovered */
    float                   areaperc = 0;
    AREA_DATA              *pArea;
    FOUND_AREA             *found;
    int                     areacount = 0;
    int                     foundcount = 0;

    for ( pArea = first_asort; pArea; pArea = pArea->next_sort ) {
        if ( !IS_SET( pArea->flags, AFLAG_NODISCOVERY ) || !IS_SET( pArea->flags, AFLAG_UNOTSEE ) ) {
            areacount++;
        }
        for ( found = ch->pcdata->first_area; found; found = found->next ) {
            if ( !strcmp( found->area_name, pArea->name ) )
                foundcount++;

        }
    }

    areaperc = ( foundcount * 100 ) / areacount;
    if ( areaperc >= 50 && areaperc < 60 ) {
        score += 4;
    }
    else if ( areaperc >= 60 && areaperc < 70 ) {
        score += 5;
    }
    else if ( areaperc >= 70 && areaperc < 80 ) {
        score += 6;
    }
    else if ( areaperc >= 80 && areaperc < 90 ) {
        score += 7;
    }
    else if ( areaperc >= 90 ) {
        score += 8;
    }

    /*
     * Now that we've calculated the score its time to set it 
     */
    ch->pcdata->score = score;
    replace_score( ch );                               /* use the data we just collected */
    return;
}

/* The actual insertion of a new score */

void add_score( CHAR_DATA *ch )
{
    int                     value = ch->pcdata->score;
    SCORE_DATA             *score,
                           *newscore;
    int                     i;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_IMMORTAL( ch ) )
        return;

    for ( i = 1, score = first_score; i <= MAX_SCORES; score = score->next, i++ ) {
        if ( !score ) {                                /* there are empty slots at end of 
                                                        * list, add there */
            CREATE( newscore, SCORE_DATA, 1 );
            newscore->name = STRALLOC( ch->name );
            newscore->score = value;
            LINK( newscore, first_score, last_score, next, prev );
            break;
        }
        /*
         * This section inserts the higher value into the higher slot 
         */
        else if ( value > score->score ) {
            CREATE( newscore, SCORE_DATA, 1 );
            newscore->name = STRALLOC( ch->name );
            newscore->score = value;
            INSERT( newscore, score, first_score, next, prev );
            break;
        }
    }
    fix_score_length(  );
    save_scores(  );
}

void free_all_scores( void )
{
    SCORE_DATA             *score,
                           *score_next;

    for ( score = first_score; score; score = score_next ) {
        score_next = score->next;
        UNLINK( score, first_score, last_score, next, prev );
        STRFREE( score->name );
        DISPOSE( score );
    }
}

/* This is used to determine ensure an existing score is removed before a new one
is added, so that you dont have repeats and so if a persons score drops, its actually
changed instead of staying forever at its highest point */

void replace_score( CHAR_DATA *ch )
{
    SCORE_DATA             *score;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_IMMORTAL( ch ) )
        return;

    for ( score = first_score; score; score = score->next )
        if ( !str_cmp( ch->name, score->name ) )
            break;

    if ( !score || ( str_cmp( score->name, ch->name ) ) ) {
        add_score( ch );
        return;
    }

    UNLINK( score, first_score, last_score, next, prev );
    STRFREE( score->name );
    DISPOSE( score );

    add_score( ch );
}

/* This will make sure it stays at only the max you want to display 
by checking to see how many total there are and cutting off the last one until
there are only the amount you want, default is 10 */

void fix_score_length(  )
{
    SCORE_DATA             *score;
    short                   x;

    x = count_scores(  );

    while ( x > MAX_SCORES ) {
        score = last_score;
        UNLINK( score, first_score, last_score, next, prev );
        STRFREE( score->name );
        DISPOSE( score );
        x = count_scores(  );
    }
}

/* very simple function to count how many scores are in memory */

int count_scores(  )
{
    SCORE_DATA             *score;
    short                   x = 0;

    for ( score = first_score; score; score = score->next )
        ++x;
    return x;
}

/* the actual command to see whos where */

void do_ratings( CHAR_DATA *ch, char *argument )
{
    SCORE_DATA             *score;
    short                   counter = 1;

    ch_printf( ch, "     &cThe Top &C20&c Players with 6 Dragons!\r\r\n\n" );
    ch_printf( ch, "\t      Name        Score\r\n" );
    if ( !IS_BLIND( ch ) ) {
        ch_printf( ch, "\t-------------------------------&C\r\n" );
    }
    for ( score = first_score; score; score = score->next ) {
        if ( !IS_BLIND( ch ) ) {
            ch_printf( ch, "\t&c[%2d]&C %-12s %-32d\r\n", counter, score->name, score->score );
            ++counter;
        }
        else if ( IS_BLIND( ch ) ) {
            ch_printf( ch, "\t&c%2d&C %-12s %-32d\r\n", counter, score->name, score->score );
            ++counter;
        }
    }
    if ( !IS_BLIND( ch ) ) {
        ch_printf( ch, "\t&c-------------------------------\r\r\n\n" );
    }
    ch_printf( ch, "\t&cSee &WHELP RATINGS&c for more information.\r\n" );
}

void load_scores(  )
{
    FILE                   *fp;
    SCORE_DATA             *score;

    if ( ( fp = FileOpen( SCORES_FILE, "r" ) ) == NULL ) {
        bug( "Cannot open scores.dat for reading" );
        return;
    }

    for ( ;; ) {
        char                   *word;

        word = ( feof( fp ) ? ( char * ) "End" : fread_word( fp ) );

        if ( !str_cmp( word, "Score" ) ) {
            CREATE( score, SCORE_DATA, 1 );
            score->score = fread_number( fp );
            score->name = fread_string( fp );
            LINK( score, first_score, last_score, next, prev );
            continue;
        }

        if ( !str_cmp( word, "End" ) ) {
            FileClose( fp );
            fp = NULL;
            return;
        }
    }

    if ( fp ) {
        FileClose( fp );
        fp = NULL;
    }
}

bool valid_player( char *name )
{
    char                    strsave[MAX_INPUT_LENGTH];
    FILE                   *fp = NULL;

    sprintf( strsave, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );
    if ( ( fp = FileOpen( strsave, "r" ) ) != NULL ) {
        FileClose( fp );
        fp = NULL;
    }
    else                                               /* pfile gone */
        return FALSE;
/*
   sprintf( strsave, "%s%s", STAFF_DIR, capitalize( name) );
   if ( ( fp = FileOpen( strsave, "r" ) ) != NULL ) 
   {
      FileClose( fp );
      fp = NULL;
      return FALSE;   
   }
   else
      ;
 */
    return TRUE;
}

void save_scores(  )
{
    SCORE_DATA             *score;
    FILE                   *fp;

    if ( ( fp = FileOpen( SCORES_FILE, "w" ) ) == NULL ) {
        bug( "Cannot open scores.dat for writing" );
        perror( SCORES_FILE );
        return;
    }

    for ( score = first_score; score; score = score->next ) {
        if ( !valid_player( score->name ) )
            continue;
        else
            fprintf( fp, "Score %d %s~\n", score->score, score->name );
    }
    fprintf( fp, "End\n\n" );
    FileClose( fp );
}
