
 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Update module                                                         *
 ***************************************************************************/

#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/auction.h"
#include "h/new_auth.h"
#include "h/arena.h"
#include "h/events.h"

void                    handle_sieges( void );

bool                    doubleexp = FALSE;
bool                    cexp = FALSE;
time_t                  starttimedoubleexp;
time_t                  starttimecexp;
void                    stralloc_printf( char **pointer, const char *fmt, ... );
extern bool             happyhouron;
time_t                  starthappyhour;
time_t                  startrunevent;
void                    tag_update( void );
void save_sysdata       args( ( SYSTEM_DATA sys ) );
extern void             vote_update( void );

WOLLA( dailyevents );
void                    create_wood( CHAR_DATA *ch, short resource, int size, bool magictree,
                                     ROOM_INDEX_DATA *room );
void                    adjust_faith( CHAR_DATA *ch, short adjustment );
void make_corpse        args( ( CHAR_DATA *ch, CHAR_DATA *killer ) );
void check_pfiles       args( ( time_t reset ) );
bool                    is_mppaused( CHAR_DATA *mob );

/* Volk - bards! */
void                    bard_music( CHAR_DATA *ch );

/* Local functions. */
int                     hit_gain( CHAR_DATA *ch );
int                     mana_gain( CHAR_DATA *ch );
int                     move_gain( CHAR_DATA *ch );

void                    mobile_update( void );
void                    time_update( void );           /* FB */
void                    char_update( void );
void                    char_affects( void );
void                    obj_update( void );
void                    aggr_update( void );
void                    room_act_update( void );
void                    obj_act_update( void );
void                    char_check( void );
void                    drunk_randoms( CHAR_DATA *ch );
void                    hallucinations( CHAR_DATA *ch );
void                    subtract_times( struct timeval *etime, struct timeval *stime );
void make_blood         args( ( CHAR_DATA *ch ) );
void                    write_corpses( CHAR_DATA *ch, char *name, OBJ_DATA *objrem );

void                    update_quest( CHAR_DATA *ch );

// void class_monitor( CHAR_DATA * gch );

void hunt_victim        args( ( CHAR_DATA *ch ) );
void found_prey         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

void                    regroup_obj( OBJ_DATA *obj );

/*
 * Global Variables
 */
CHAR_DATA              *gch_prev;
OBJ_DATA               *gobj_prev;

CHAR_DATA              *timechar;

const char             *corpse_descs[] = {
    "The corpse of %s is in the last stages of decay.",
    "The corpse of %s is crawling with vermin.",
    "The corpse of %s fills the air with a foul stench.",
    "The corpse of %s is buzzing with flies.",
    "The corpse of %s lies here."
};

extern int              top_exit;

//New arena code support. -Taon
extern bool             arena_underway;
extern bool             arena_prep;
extern bool             challenge;

extern void parse_entry args( ( short time ) );
extern void             silent_end(  );
extern bool             mud_down;

/* Updated to give a chance for solving a problem also */
void char_dexpupdate( CHAR_DATA *ch )
{
    char                    buf[MSL];                  /* Acutal data */
    char                    display_buf[MSL];          /* Display data */

    const char             *authorization_list[76] = {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
        "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
        "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+",
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
        "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="
    };
    int                     count,
                            random,
                            extra_random,
                            extra;

    /*
     * Sanity check 
     */
    if ( !ch || !ch->pcdata )
        return;

    /*
     * Staff finds it annoying and not really needed 
     */
    if ( ch->level >= LEVEL_IMMORTAL )
        return;

    if ( IS_AVATAR( ch ) )
        return;

    if ( ch->secondclass != -1 && ch->thirdclass == -1 && ch->secondlevel == 100
         && ch->firstlevel == 100 )
        return;

    if ( ch->secondclass != -1 && ch->thirdclass != -1 && ch->secondlevel == 100
         && ch->firstlevel == 100 && ch->thirdlevel == 100 )
        return;

    if ( !ch->pcdata->last_dexpupdate )
        ch->pcdata->last_dexpupdate = current_time;

    if ( ( current_time - 1800 ) < ch->pcdata->last_dexpupdate )
        return;

    /*
     * Time has expired for their hour of double exp update this 
     */
    if ( ch->pcdata->getsdoubleexp ) {
        if ( ( current_time - 3600 ) > ch->pcdata->last_dexpupdate ) {
            /*
             * Update time so another hour before they get to gain again 
             */
            ch->pcdata->last_dexpupdate = current_time;
            ch->pcdata->getsdoubleexp = FALSE;
            send_to_char( "Your hour of triple experience has ended.\r\n", ch );
        }
        return;                                        /* Don't let players solve while
                                                        * dexp on */
    }

    snprintf( buf, MSL, "%s", "" );
    snprintf( display_buf, MSL, "%s", "" );

    /*
     * 20% chance of doing a math problem 
     */
    if ( number_percent(  ) > 80 ) {
        bool                    first = TRUE;
        int                     sans = 0;              /* Keep track of the answer */

        for ( extra = 0; extra < 3; extra++ ) {
            random = number_range( 1, 99 );

            if ( first ) {
                snprintf( buf, MSL, "%d", random );
                mudstrlcat( display_buf, buf, MSL );
                sans += random;
                first = FALSE;
                continue;
            }
            /*
             * add 
             */
            if ( number_percent(  ) > 50 ) {
                snprintf( buf, MSL, " plus %d", random );
                mudstrlcat( display_buf, buf, MSL );
                sans += random;
            }
            else {                                     /* subtract */

                snprintf( buf, MSL, " minus %d", random );
                mudstrlcat( display_buf, buf, MSL );
                sans -= random;
            }
        }
        snprintf( buf, MSL, "%d", sans );
        ch->pcdata->last_dexpupdate = current_time;
        if ( VLD_STR( ch->pcdata->authexp ) )
            STRFREE( ch->pcdata->authexp );
        ch->pcdata->authexp = STRALLOC( buf );
        if ( xIS_SET( ch->act, PLR_SOUND ) )
            send_to_char( "!!SOUND(sound/solve.wav)\r\n", ch );
        ch_printf( ch,
                   "&YTo receive an increase in your triple exp timer you must solve this problem: %s\r\n",
                   display_buf );
        send_to_char( "To solve it type 'solve <answer>' Answers are case sensitive.\r\n", ch );
        send_to_char
            ( "Once your status bar is full you can activate triple exp by typing config texp.\r\n",
              ch );
        return;
    }

    /*
     * Get what to be used as the extra now 
     */
    extra = number_chance( 0, 75 );
    /*
     * If not in range set default to X 
     */
    if ( extra > 75 || extra < 0 )
        extra = 61;

    for ( count = 0; count < 6; count++ ) {
        random = number_chance( 0, 75 );
        extra_random = number_chance( 0, 2 );

        /*
         * Lets make sure random is in the right range 
         * And make sure its not using an extra 
         */
        if ( random > 75 || random < 0 )
            random = ( 22 + count );
        /*
         * Make sure random and extra arent the same 
         */
        if ( random == extra ) {
            count--;                                   /* Decrease count so it goes till
                                                        * it has the right amount */
            continue;                                  /* Continue since dont want it
                                                        * using an extra for a random */
        }
        if ( extra_random == 1 ) {
            mudstrlcat( display_buf, authorization_list[extra], MSL );
            if ( extra == 32 || extra == 31 )
                mudstrlcat( display_buf, authorization_list[extra], MSL );
        }
        /*
         * Get what it should use for that number 
         */
        mudstrlcat( buf, authorization_list[random], MSL );
        /*
         * Need to put a double in sometimes to display right 
         */
        if ( random == 32 || random == 31 )
            mudstrlcat( display_buf, authorization_list[random], MSL );
        mudstrlcat( display_buf, authorization_list[random], MSL );
    }
    ch->pcdata->last_dexpupdate = current_time;
    if ( VLD_STR( ch->pcdata->authexp ) )
        STRFREE( ch->pcdata->authexp );
    ch->pcdata->authexp = STRALLOC( buf );
    if ( xIS_SET( ch->act, PLR_SOUND ) )
        send_to_char( "!!SOUND(sound/solve.wav)\r\n", ch );
    ch_printf( ch,
               "&YTo receive an increase in your triple exp timer type in \"solve %s\", leave out all %s's\r\nOnce your status bar is full when you type level...\r\nYou can activate triple exp by typing config texp.\r\n",
               display_buf, authorization_list[extra] );
}

/*
 * Advancement stuff.
 */

// BALANCE HP FUNCTION DESIGNED TO BRING
// GAME BALANCE FOR ANY CLASS/RACE COMBOS.
int hitpoints( CHAR_DATA *ch )
{
    int                     gain;

    if ( ch->race == RACE_DRAGON || ch->Class == CLASS_DRAGONLORD ) {
        gain = number_range( 90, 100 );
        if ( ch->Class == CLASS_BLACK ) {
            gain += number_range( 8, 15 );             // est. 9000 hp
        }
        else if ( ch->Class != CLASS_BLACK ) {
            gain += number_range( 5, 12 );             // est 8800 hp
        }
    }
    if ( ch->race != RACE_DRAGON ) {
// SINGLE CLASS WARRIORS gain most non-dragon hp
        if ( IS_SINGLECLASS( ch ) && ch->Class == CLASS_WARRIOR ) {
            gain = number_range( 95, 102 );            // est 8000 hp
        }
// SINGLE CLASS NON-MELEE TYPE CLASSES
        else if ( ch->secondclass == -1 && ch->Class != CLASS_WARRIOR
                  && ch->Class != CLASS_SHADOWKNIGHT && ch->Class != CLASS_CRUSADER
                  && ch->Class != CLASS_HELLSPAWN ) {
            gain = number_range( 50, 60 );             // est 6000 hp
        }
// SINGLE CLASS NON-WARRIOR BUT MELEE
        else if ( ch->secondclass == -1
                  && ( ch->Class == CLASS_SHADOWKNIGHT || ch->Class == CLASS_CRUSADER
                       || ch->Class == CLASS_HELLSPAWN && !IS_SECONDCLASS( ch ) ) ) {
            gain = number_range( 60, 70 );             // est 7000 hp
        }
// Now dual class times 2 hp per level
// DUAL CLASS WITH ONE CLASS BEING WARRIOR
        else if ( ch->secondclass != -1 && ch->thirdclass == -1
                  && ( ch->Class == CLASS_WARRIOR || ch->secondclass == CLASS_WARRIOR ) ) {
            gain = number_range( 30, 35 );             // 7000 hp
        }
// DUAL CLASS WITH ONE CLASS BEING MELEE
        else if ( ch->secondclass != -1 && ch->thirdclass == -1
                  && ( ch->Class != CLASS_WARRIOR && ch->secondclass != CLASS_WARRIOR
                       && ( ch->Class == CLASS_SHADOWKNIGHT || ch->Class == CLASS_CRUSADER
                            || ch->Class == CLASS_HELLSPAWN || ch->secondclass == CLASS_SHADOWKNIGHT
                            || ch->secondclass == CLASS_CRUSADER
                            || ch->secondclass == CLASS_HELLSPAWN ) ) ) {
            gain = number_range( 25, 30 );             // 6000 hp
        }
// DUAL CLASS WITH NO CLASSES AS MELEE
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 && ch->Class != CLASS_WARRIOR
                  && ch->secondclass != CLASS_WARRIOR && ch->Class != CLASS_SHADOWKNIGHT
                  && ch->Class != CLASS_CRUSADER && ch->Class != CLASS_HELLSPAWN
                  && ch->secondclass != CLASS_SHADOWKNIGHT && ch->secondclass != CLASS_CRUSADER
                  && ch->secondclass != CLASS_HELLSPAWN ) {
            gain = number_range( 20, 25 );             // 5000 hp
        }
// TRIPLE CLASS WITH WARRIOR
        else if ( ch->secondclass != -1 && ch->thirdclass != -1
                  && ( ch->Class == CLASS_WARRIOR || ch->secondclass == CLASS_WARRIOR
                       || ch->thirdclass == CLASS_WARRIOR ) ) {
            gain = number_range( 15, 20 );             // 6000 hp
        }
// TRIPLE CLASS WITH ONE CLASS BEING MELEE
        else if ( ch->secondclass != -1 && ch->thirdclass != -1
                  && ( ch->Class != CLASS_WARRIOR && ch->secondclass != CLASS_WARRIOR
                       && ( ch->Class == CLASS_SHADOWKNIGHT || ch->Class == CLASS_CRUSADER
                            || ch->Class == CLASS_HELLSPAWN || ch->secondclass == CLASS_SHADOWKNIGHT
                            || ch->secondclass == CLASS_CRUSADER
                            || ch->secondclass == CLASS_HELLSPAWN
                            || ch->thirdclass == CLASS_SHADOWKNIGHT
                            || ch->thirdclass == CLASS_CRUSADER
                            || ch->thirdclass == CLASS_HELLSPAWN ) ) ) {
            gain = number_range( 14, 18 );             // 5400 hp
        }
// TRIPLE CLASS WITH NO CLASSES AS MELEE
        else if ( ch->secondclass != -1 && ch->thirdclass != -1 && ch->Class != CLASS_WARRIOR
                  && ch->secondclass != CLASS_WARRIOR && ch->Class != CLASS_SHADOWKNIGHT
                  && ch->Class != CLASS_CRUSADER && ch->Class != CLASS_HELLSPAWN
                  && ch->secondclass != CLASS_SHADOWKNIGHT && ch->secondclass != CLASS_CRUSADER
                  && ch->secondclass != CLASS_HELLSPAWN && ch->thirdclass != CLASS_SHADOWKNIGHT
                  && ch->thirdclass != CLASS_CRUSADER && ch->thirdclass != CLASS_HELLSPAWN ) {
            gain = number_range( 13, 17 );             // 5100 hp
        }
// MONKS NEED HP TOO
        else if ( ch->secondclass == -1 && ch->Class == CLASS_MONK ) {
            gain = number_range( 62, 74 );
        }
        else if ( ch->secondclass != -1 && ch->thirdclass == -1 && ch->Class != CLASS_WARRIOR
                  && ch->secondclass != CLASS_WARRIOR && ch->Class == CLASS_MONK
                  || ch->secondclass == CLASS_MONK ) {
            gain = number_range( 28, 33 );
        }

        if ( IS_PKILL( ch ) ) {
            gain += 5;
        }
    }
    return gain;
}

/* Used in advance_level */
int mana_max( CHAR_DATA *ch )
{
    return class_table[ch->Class]->mana_max;
}

int mana_min( CHAR_DATA *ch )
{
    return class_table[ch->Class]->mana_min;
}

//A little error handling never hurt anyone. -Taon
// Hear Hear! - Volk
bool legal_expratio( CHAR_DATA *ch )
{
    if ( IS_IMMORTAL( ch ) || IS_NPC( ch ) )
        return TRUE;

    if ( !IS_THIRDCLASS( ch ) && !IS_SECONDCLASS( ch ) )
        return TRUE;

    if ( IS_THIRDCLASS( ch )
         && ( ( ch->firstexpratio + ch->secondexpratio + ch->thirdexpratio ) == 100 ) )
        return TRUE;

    if ( IS_SECONDCLASS( ch ) && ( ( ch->firstexpratio + ch->secondexpratio ) == 100 ) )
        return TRUE;

    send_to_char
        ( "\r\n&RAn illegal expratio setting has been detected! Please use the &PEXPRATIO&R command to\r\n"
          "distribute experience between your classes. Until this is fixed you will not be\r\n"
          "able to gain experience! \r\n", ch );
    return FALSE;
}

void advance_level( CHAR_DATA *ch )
{
    char                    buf2[MSL];
    int                     add_blood,
                            add_move,
                            add_prac,
                            hgain,
                            wgain,
                            add_mana;
    int                     hp;
    short                   Classes = 1;

    hgain = number_chance( 6, 12 );
    wgain = number_chance( 60, 105 );

    if ( ch->race == RACE_DRAGON && !IS_IMMORTAL( ch ) && !xIS_SET( ch->act, PLR_LIFE ) ) {
        ch->height += hgain;
        ch->weight += wgain;
    }

    if ( ch->pcdata && ch->pcdata->tmprace == RACE_DRAGON && !IS_IMMORTAL( ch )
         && !xIS_SET( ch->act, PLR_LIFE ) ) {
        if ( ch->pcdata->tmpheight )
            ch->pcdata->tmpheight += hgain;
        if ( ch->pcdata->tmpweight )
            ch->pcdata->tmpweight += wgain;
    }

    if ( !IS_THIRDCLASS( ch ) && IS_SECONDCLASS( ch ) )
        Classes = 2;
    else if ( IS_SECONDCLASS( ch ) && IS_THIRDCLASS( ch ) )
        Classes = 3;

    hp = con_app[get_curr_con( ch )].hitp + hitpoints( ch );

    /*
     * bonus for deadlies 
     */
    if ( IS_PKILL( ch ) && !xIS_SET( ch->act, PLR_LIFE ) ) {
        add_prac++;
        send_to_char( "&RGravoc's Pandect steels your sinews.\r\n", ch );
    }

    if ( ch->Class == CLASS_PRIEST )
        add_mana = ( get_curr_wis( ch ) / 2 + number_range( mana_min( ch ), mana_max( ch ) ) );
    else if ( ch->Class == CLASS_MAGE )
        add_mana = ( get_curr_int( ch ) / 2 + number_range( mana_min( ch ), mana_max( ch ) ) );
    else
        add_mana = ( get_curr_int( ch ) / 4 + number_range( mana_min( ch ), mana_max( ch ) ) );

    // added by Vladaar to give people a reason to be a singleclassed caster
    if ( ( ch->Class == CLASS_MAGE || ch->Class == CLASS_PRIEST || ch->Class == CLASS_DRUID
           || ch->Class == CLASS_PSIONIC ) && ch->secondclass == -1 ) {
        add_mana = ( add_mana * 2 );
    }

    if ( add_mana > 0 )
        add_mana /= Classes;

    if ( add_mana < 1 )
        add_mana = 1;

    if ( IS_BLOODCLASS( ch ) )
        add_blood = number_chance( 1, 4 );
    else
        add_blood = number_chance( 1, 2 );

    add_move = number_range( 5, ( get_curr_con( ch ) + get_curr_dex( ch ) ) / 4 );
    if ( add_move > 0 )
        add_move /= Classes;

    if ( add_move < 1 )
        add_move = 1;

    /*
     * Players with low WIS aren't gaining.. Guess multiclass. 
     */
    add_prac = wis_app[get_curr_wis( ch )].practice;

    if ( add_prac < 1 )
        add_prac = number_range( 1, 2 );

    hp = UMAX( 1, hp );

    add_mana = UMAX( 0, add_mana );
    add_move = UMAX( 20, add_move );

    if ( ch->race == RACE_DRAGON || ( ch->pcdata && ch->pcdata->tmprace == RACE_DRAGON ) )
        add_move *= 2;

    ch->max_blood += add_blood;
    ch->max_hit += hp;
    ch->max_mana += add_mana;
    ch->max_move += add_move;
    if ( ch->pcdata && ch->pcdata->tmprace == RACE_DRAGON && ch->pcdata->tmpmax_hit )
        ch->pcdata->tmpmax_hit = ( int ) ( hp * 100 ) + ( ch->pcdata->tmpmax_hit * 100 );
    if ( !xIS_SET( ch->act, PLR_LIFE ) ) {
        ch->practice += add_prac;
    }
    if ( ch->level < LEVEL_IMMORTAL && !xIS_SET( ch->act, PLR_LIFE ) ) {
        set_char_color( AT_WHITE, ch );
        ch_printf( ch, "Your gain is: %d/%d hp, ", ( int ) hp, ch->max_hit );
        if ( IS_BLOODCLASS( ch ) )
            ch_printf( ch, "%d/%d bp, ", add_blood, ch->max_blood );
        else
            ch_printf( ch, "%d/%d mana, ", add_mana, ch->max_mana );
        ch_printf( ch, "%d/%d mv, %d practices", add_move, ch->max_move, add_prac );
        if ( ch->race == RACE_DRAGON )
            ch_printf( ch, ", %d height & %d weight", hgain, wgain );
        send_to_char( ".\r\n", ch );
        if ( !IS_NPC( ch ) ) {
            snprintf( buf2, MSL, "&G%-13s  ->&w%-2d  &G-&w  %-5d&G   Rvnum: %-5d   %s %s",
                      ch->name, ch->level, calculate_age( ch ),
                      ch->in_room == NULL ? 0 : ch->in_room->vnum,
                      capitalize( race_table[ch->race]->race_name ),
                      class_table[ch->Class]->who_name );
            snprintf( buf2, MSL,
                      "Gains: %d hp, %d bp, %d mana, %d move, %d prac, %d height, %d weight", hp,
                      add_blood, add_mana, add_move, add_prac, hgain, wgain );
            snprintf( buf2, MSL, "Gains: %d hp, %d mana, %d move, %d prac, %d height, %d weight",
                      hp, add_mana, add_move, add_prac, hgain, wgain );
        }
    }
    // update_member( ch );
}

void gain_craft( CHAR_DATA *ch, int gain )
{
    char                    buf[MSL];

    if ( IS_NPC( ch ) )
        return;

    if ( gain < 1 ) {
        gain = number_chance( 1, 4 );
    }
    ch->exp += ( int ) gain;
    ch_printf( ch, "&WYou receive %d crafting points.%s\r\n", gain );

    if ( ch->exp >= exp_level( ch, ch->level + 1 ) ) {
        set_char_color( AT_WHITE, ch );

        ch_printf( ch, "You have now obtained experience level %d!&D\r\n", ++ch->level );
        ch->exp = ( ch->exp - exp_level( ch, ( ch->level ) ) );
    }
    return;
}

int                     num_players_online;

void get_curr_players( void )
{
    CHAR_DATA              *ch,
                           *ch_next;
    int                     count = 0;

    for ( ch = first_char; ch; ch = ch_next ) {
        ch_next = ch->next;
        if ( IS_NPC( ch ) || IS_IMMORTAL( ch ) )       /* Skip npcs and immortals */
            continue;
        if ( ch->level != 0 && ch->level < 5 )
            continue;
        if ( ch->firstlevel != 0 && ch->firstlevel < 5 )
            continue;
        count++;
    }
    num_players_online = count;
}

void gain_exp( CHAR_DATA *ch, int gain )
{
    char                    buf[MSL];
    double                  modgain;
    int                     oldexp,
                            expratio = 0,
        pcount;

    if ( IS_NPC( ch ) )
        return;

    modgain = gain;

    if ( IS_IMMORTAL( ch ) || IS_AVATAR( ch )
         || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "tutorial.are" ) )
         || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "etutorial.are" ) )
         || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "dtutorial.are" ) ) )
        return;

    if ( ch->secondclass != -1 && ch->thirdclass == -1 && ch->secondlevel == 100
         && ch->firstlevel == 100 )
        return;

    if ( ch->secondclass != -1 && ch->thirdclass != -1 && ch->secondlevel == 100
         && ch->firstlevel == 100 && ch->thirdlevel == 100 )
        return;

    expratio = ch->firstexpratio;
    expratio += ch->secondexpratio;
    expratio += ch->thirdexpratio;
    if ( expratio > 100 ) {
        send_to_char
            ( "Your expratios are more then 100 and you will not gain any exp until you fix them.\r\n",
              ch );
        return;
    }

    if ( ch->firstlevel >= 100 && ch->firstexpratio > 0 ) {
        send_to_char
            ( "Your first class is maxed out and you need to adjust your expratios to gain more exp.\r\n",
              ch );
        return;
    }
    if ( ch->secondlevel >= 100 && ch->secondexpratio > 0 ) {
        send_to_char
            ( "Your second class is maxed out and you need to adjust your expratios to gain more exp.\r\n",
              ch );
        return;
    }
    if ( ch->thirdlevel >= 100 && ch->thirdexpratio > 0 ) {
        send_to_char
            ( "Your third class is maxed out and you need to adjust your expratios to gain more exp.\r\n",
              ch );
        return;
    }

    if ( !IN_WILDERNESS( ch ) ) {
        modgain *= 1.6;
    }

    /*
     * Multi Class bonus 
     */
    if ( modgain > 0 ) {
        if ( ch->secondclass >= 0 ) {
            if ( ch->thirdclass >= 0 )
                modgain *= 1.5;
            else
                modgain *= 1.2;
        }
    }

    /*
     * per-race experience multipliers 
     */
    if ( ch->Class != CLASS_HERO ) {
        modgain *= ( race_table[ch->race]->exp_multiplier / 100.0 );
    }

    // HIGHER LEVEL DRAGONS NEED HELP
    if ( ch->race == RACE_DRAGON && ch->Class != CLASS_HERO ) {
        if ( ch->level > 89 )
            modgain *= 1.5;
        else if ( ch->level > 79 )
            modgain *= 1.4;
        else if ( ch->level > 69 )
            modgain *= 1.3;
        else if ( ch->level > 50 )
            modgain *= 1.2;
    }

    /*
     * Give them double exp 
     */
    if ( modgain > 0 && ch->pcdata ) {
        /*
         * Took and commented out all these spammy messages it is quite annoying, they can
         * look at level and who to see if getting extra exp 
         */
        if ( IS_GROUPED( ch ) && !happyhouron ) {
//         send_to_char( "&WYou receive Triple Experience for being grouped.\r\n", ch );
            modgain *= 3.0;
        }
        else if ( IS_GROUPED( ch ) && happyhouron ) {
//         send_to_char( "&WYou receive Triple Experience for being grouped during HAPPY HOUR!\r\n", ch );
            modgain *= 3.0;
        }
        else if ( ( doubleexp || sysdata.daydexp )
                  && ( !ch->pcdata || !ch->pcdata->getsdoubleexp ) ) {
//         send_to_char( "&WYou receive Double Experience.&D\r\n", ch );
            modgain *= 2.0;
        }
        else if ( ch->pcdata && ch->pcdata->getsdoubleexp ) {
//         send_to_char( "&WYou receive Triple Experience.&D\r\n", ch );
            modgain *= 3.0;
        }

        /*
         * New change to give more experience based on players online 
         */
        if ( !num_players_online )
            get_curr_players(  );
        pcount = num_players_online;
        if ( pcount > 40 )
            pcount = 40;

        if ( pcount )
            modgain *= ( ( 100 + ( pcount * sysdata.gmb ) ) / 100 );
    }

    if ( xIS_SET( ch->act, PLR_R54 ) )
        modgain *= 4.0;

    /*
     * To help out newbies by giving more XP 
     */
    if ( ch->level > 10 ) {
        if ( ch->level < 20 )
            modgain *= 1.3;
        else if ( ch->level < 30 )
            modgain *= 1.5;
        else if ( ch->level < 40 )
            modgain *= 1.5;
        else if ( ch->level < 50 )
            modgain *= 1.5;
        else if ( ch->level < 60 )
            modgain *= 1.4;
        else if ( ch->level < 70 )
            modgain *= 1.4;
        else if ( ch->level < 100 )
            modgain *= 1.25;
    }

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) )
        modgain /= 4;

    if ( ch->Class == CLASS_HERO ) {
        modgain /= 2;
        modgain /= 2;
        if ( modgain < 100 )
            modgain = number_range( 100, 300 );
    }

    // Capped exp gains based on level. -Taon
    if ( !xIS_SET( ch->act, PLR_R54 ) ) {
        if ( modgain > 12000 && ch->level < 5 ) {
            modgain = 12000 - number_range( 1, 999 );
        }
        else if ( modgain > 20000 && ch->level < 10 && ch->level > 4 ) {
            modgain = 20000 - number_range( 1, 999 );
        }
        else if ( modgain > 25000 && ch->level < 15 && ch->level > 9 ) {
            modgain = 25000 - number_range( 1, 999 );
        }
        else if ( modgain > 32000 && ch->level < 20 && ch->level > 14 ) {
            modgain = 32000 - number_range( 1, 999 );
        }
        else if ( modgain > 44000 && ch->level < 30 && ch->level > 19 ) {
            modgain = 44000 - number_range( 1, 999 );
        }
        else if ( modgain > 52000 && ch->level < 40 && ch->level > 29 ) {
            modgain = 52000 - number_range( 1, 1999 );
        }
        else if ( modgain > 65000 && ch->level < 60 && ch->level > 39 ) {
            modgain = 65000 - number_range( 1, 2999 );
        }
        else if ( modgain > 85000 && ch->level < 80 && ch->level > 59 ) {
            modgain = 85000 - number_range( 1, 5999 );
        }
        else if ( modgain > 100000 && ch->level <= 100 && ch->level > 79 ) {
            modgain = 100000 - number_range( 1, 5999 );
        }
    }

    // fix so r54 never advances 30 levels a time with some crazy exp jump
    if ( xIS_SET( ch->act, PLR_R54 ) ) {
        if ( modgain > 30000 && ch->level < 5 ) {
            modgain = 30000 - number_range( 1, 999 );
        }
        else if ( modgain > 40000 && ch->level < 10 && ch->level >= 5 ) {
            modgain = 40000 - number_range( 1, 999 );
        }
        else if ( modgain > 60000 && ch->level < 20 && ch->level >= 10 ) {
            modgain = 60000 - number_range( 1, 999 );
        }
        else if ( modgain > 150000 && ch->level <= 100 && ch->level >= 20 ) {
            modgain = 150000 - number_range( 9999, 20999 );
        }
    }
    oldexp = ch->exp;

    if ( !legal_expratio( ch ) )
        return;

    if ( IS_THIRDCLASS( ch ) || IS_SECONDCLASS( ch ) ) {
        int                     level = 0;             /* 1 = first, 2 = second, 3 =
                                                        * third */

        /*
         * No experience gained if level is higher than others MC_LD = Multiclass Level
         * Difference 
         */

        if ( ( ch->firstlevel - ch->secondlevel ) >= MC_LD && ch->firstexpratio > 0 )
            level = 1;
        if ( ( ch->secondlevel - ch->firstlevel ) >= MC_LD && ch->secondexpratio > 0 )
            level = 2;

        if ( IS_THIRDCLASS( ch ) ) {
            if ( ( ch->firstlevel - ch->thirdlevel ) >= MC_LD && ch->firstexpratio > 0 )
                level = 1;
            if ( ( ch->secondlevel - ch->thirdlevel ) >= MC_LD && ch->secondexpratio > 0 )
                level = 2;
            if ( ( ( ch->thirdlevel - ch->firstlevel ) >= MC_LD
                   || ( ch->thirdlevel - ch->secondlevel ) >= MC_LD ) && ch->thirdexpratio > 0 )
                level = 3;
        }

        if ( level > 0 )
            ch_printf( ch, "&RYour class &P%s&R is not gaining any experience!\r\n"
                       "You will not gain experience while one of your levels is %d or more higher than another.\r\n"
                       "Please adjust &PEXPRATIO&R to continue gaining experience in other classes.\r\n",
                       level == 1 ? class_table[ch->Class]->who_name : level ==
                       2 ? class_table[ch->secondclass]->who_name : class_table[ch->thirdclass]->
                       who_name, MC_LD );

        if ( level != 1 )
            ch->firstexp += ( ( int ) modgain * ch->firstexpratio / 100 );
        if ( level != 2 )
            ch->secondexp += ( ( int ) modgain * ch->secondexpratio / 100 );
        if ( level != 3 )
            ch->thirdexp += ( ( int ) modgain * ch->thirdexpratio / 100 );

        /*
         * Christ, negative exp isn't fun - forgot to UMAX it 
         */
        ch->firstexp = UMAX( 0, ch->firstexp );
        ch->secondexp = UMAX( 0, ch->secondexp );
        ch->thirdexp = UMAX( 0, ch->thirdexp );

        if ( IS_THIRDCLASS( ch ) || IS_SECONDCLASS( ch ) ) {
            if ( modgain < 10 )
                modgain = number_range( 20, 60 );
            if ( modgain > 0 && level == 0 && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                ch_printf( ch, "&WYou receive %d experience points.\r\n", ( int ) modgain );
            else if ( modgain < 0 && level == 0 && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                ch_printf( ch, "&WYou lose %d experience points.\r\n", ( int ) modgain );
        }
        if ( !xIS_SET( ch->act, PLR_LIFE ) ) {
            advance_class_level( ch );                 /* Gain levels where appropriate */
        }
    }
    else {
        ch->exp = UMAX( 0, ch->exp + ( int ) modgain );
        if ( !IS_SECONDCLASS( ch ) ) {
            if ( ch->exp - oldexp < 10 )
                ch->exp += number_range( 20, 60 );
            if ( ( ch->exp - oldexp ) > 0 && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                ch_printf( ch, "&WYou receive %d experience points.\r\n", ( ch->exp - oldexp ) );
            else if ( ( oldexp - ch->exp ) > 0 && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
                ch_printf( ch, "&WYou lose %d experience points.\r\n", ( oldexp - ch->exp ) );
        }
        /*
         * ch->exp will always be ZERO for multiclass players so may as well keep this
         * open!
         */
        /*
         * Volk - what about messages though? 
         */
        /*
         * Taon - Bugfix here, had to stop players from getting spemmed everytime a
         * demigod gained exp. Made new PCFLAG to stop it from occuring. 
         */

        if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) && ch->exp >= exp_level( ch, ch->level + 1 ) ) {
            send_to_char( "You cannot gain a level while in human form.\r\n", ch );
            return;
        }

        if ( ( IS_AVATAR( ch ) || IS_DUALAVATAR( ch ) || IS_TRIAVATAR( ch ) || IS_DEMIGOD( ch ) )
             && !IS_IMMORTAL( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_DEMIGOD )
             && !xIS_SET( ch->act, PLR_LIFE ) ) {
            DESCRIPTOR_DATA        *d;
            char                    message[MSL];

            SET_BIT( ch->pcdata->flags, PCFLAG_DEMIGOD );
            snprintf( message, MSL, "%s",
                      IS_DEMIGOD( ch ) ? "has risen in power and become a Avatar!" :
                      "achieved the next level of Avatarhood!" );
            snprintf( buf, MSL, "%s has just %s!", ch->name, message );
            for ( d = first_descriptor; d; d = d->next ) {
                if ( d->connected == CON_PLAYING && d->character != ch ) {
                    set_char_color( AT_IMMORT, d->character );
                    send_to_char( buf, d->character );
                    send_to_char( "\r\n", d->character );
                }
            }
            set_char_color( AT_WHITE, ch );
        }

        while ( !IS_DEMIGOD( ch ) && ch->exp >= exp_level( ch, ch->level + 1 )
                && !xIS_SET( ch->act, PLR_LIFE ) ) {
            ch_printf( ch, "&WYou have now obtained experience level %d!&D\r\n", ++ch->level );
            if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
                ch_printf( ch, "&GPlaying 6D EXTREME you gain 5 glory!&D\r\n" );
                ch->quest_curr += 5;
            }
            ch->exp = ( ch->exp - exp_level( ch, ( ch->level ) ) );
            advance_level( ch );
            restore_char( ch );
            if ( xIS_SET( ch->act, PLR_MUSIC ) || xIS_SET( ch->act, PLR_SOUND ) )
                send_to_char( "!!SOUND(sound/level.wav)\r\n", ch );
            set_char_color( AT_WHITE, ch );
            send_to_char_color
                ( "&YYou have gained insight in the realms, and have been restored!\r\n", ch );
            snprintf( buf, MSL, "The realms rejoice as %s has just achieved level %d!&D", ch->name,
                      ch->level );
            announce( buf );
            snprintf( buf, MSL, "%24.24s: %s obtained level %d!%s%s&D", ctime( &current_time ),
                      ch->name, ch->level, ( doubleexp ? " (Double)" : "" ),
                      ( happyhouron ? " (HappyHour)" : "" ) );
            append_to_file( PLEVEL_FILE, buf );
        }
    }
}

/* Regeneration stuff. */
int hit_gain( CHAR_DATA *ch )
{
    int                     gain;
    short                   bio_chance,
                            rh_chance;
    OBJ_DATA               *obj;

    if ( IS_NPC( ch ) )
        gain = ( int ) ( double ) ( ( ch->level * 2 ) * 1.8 );
    else {
        gain = UMIN( 10, ch->level );

        switch ( ch->position ) {
            case POS_DEAD:
                return 0;
            case POS_MORTAL:
                return -1;
            case POS_INCAP:
                return -1;
            case POS_STUNNED:
                return 1;
            case POS_SLEEPING:
                gain += ( int ) ( double ) ( ch->max_hit * .1 );
                gain += ( int ) ( double ) ( get_curr_con( ch ) * 3.5 );
                break;
            case POS_RESTING:
                gain += ( int ) ( double ) ( ch->max_hit * .08 );
                gain += ( int ) ( double ) ( get_curr_con( ch ) * 2.5 );
                break;
            case POS_SITTING:
                gain += ( int ) ( double ) ( ch->max_hit * .06 );
                gain += ( int ) ( double ) ( get_curr_con( ch ) );
                break;
            case POS_MEDITATING:
                gain += ( int ) ( double ) ( ch->max_hit * .04 );
                gain += UMIN( 5, 10 );
                break;
        }

        if ( IS_AFFECTED( ch, AFF_CRAFTED_FOOD1 ) )
            gain *= 3;
        if ( IS_AFFECTED( ch, AFF_CRAFTED_FOOD2 ) )
            gain *= 6;
        if ( IS_AFFECTED( ch, AFF_CRAFTED_FOOD3 ) )
            gain *= 10;

        // Not sure how this will work out just want to make it beneficial to be baker,
        // may 
        // adjust later.
        if ( IS_AFFECTED( ch, AFF_CRAFTED_FOOD1 )
             && ( !IS_NPC( ch ) && ch->position == POS_FIGHTING ) )
            gain += ( int ) ( double ) ( get_curr_con( ch ) / 3 );
        if ( IS_AFFECTED( ch, AFF_CRAFTED_FOOD2 )
             && ( !IS_NPC( ch ) && ch->position == POS_FIGHTING ) )
            gain += ( int ) ( double ) ( get_curr_con( ch ) / 2 );
        if ( IS_AFFECTED( ch, AFF_CRAFTED_FOOD3 )
             && ( !IS_NPC( ch ) && ch->position == POS_FIGHTING ) )
            gain += ( int ) ( double ) ( get_curr_con( ch ) );

        if ( IS_VAMPIRE( ch ) ) {
            if ( ch->blood <= 1 )
                gain /= 2;
            else if ( ch->blood >= ( 8 + ch->level ) )
                gain *= 2;
            if ( IS_OUTSIDE( ch ) ) {
                switch ( time_info.sunlight ) {
                    case SUN_RISE:
                    case SUN_SET:
                        gain /= 2;
                        break;
                    case SUN_LIGHT:
                        gain /= 4;
                        break;
                }
            }
        }
        if ( !in_arena( ch ) ) {
            if ( !IS_NPC( ch ) && ch->race == RACE_DRAGON && ch->position != POS_FIGHTING ) {
                for ( obj = ch->last_carrying; obj; obj = obj->prev_content ) {
                    if ( obj->item_type == ITEM_TREASURE && ch->level >= obj->level ) { /* Volk 
                                                                                         * - 
                                                                                         * bug
                                                                                         * *
                                                                                         * fix */
                        if ( obj->value[2] == 0 )
                            gain = ( int ) ( double ) ( gain * 1.5 );
                        if ( obj->value[2] == 1 )
                            gain = ( int ) ( double ) ( gain * 2.2 );
                        if ( obj->value[2] == 2 )
                            gain = ( int ) ( double ) ( gain * 2.5 );
                        if ( obj->value[2] == 3 )
                            gain = ( int ) ( gain * 3 );

                    }
                }
            }

            if ( !IS_NPC( ch ) && ch->race == RACE_DRAGON && ch->position == POS_FIGHTING ) {
                for ( obj = ch->last_carrying; obj; obj = obj->prev_content ) {
                    if ( obj->item_type == ITEM_TREASURE && ch->level >= obj->level ) { /* Volk 
                                                                                         * - 
                                                                                         * bug
                                                                                         * *
                                                                                         * fix */
                        if ( obj->value[2] == 0 )
                            gain = ( int ) ( double ) ( gain * 1.1 );
                        if ( obj->value[2] == 1 )
                            gain = ( int ) ( double ) ( gain * 1.2 );
                        if ( obj->value[2] == 2 )
                            gain = ( int ) ( double ) ( gain * 1.3 );
                        if ( obj->value[2] == 3 )
                            gain = ( int ) ( gain * 1.4 );

                    }
                }
            }
        }
        // Support for root, autoskill for druids. -Taon

        if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_root] > 0 && ch->move > 10
             && IS_AFFECTED( ch, AFF_ROOT ) ) {
            send_to_char( "Your rooted feet draws healing from surrounding nature.\r\n", ch );
            ch->move -= 10;
            gain *= ( int ) ( double ) ( 2.9 );
        }

        // For biofeedback autoskill. -Taon

        bio_chance = number_chance( 1, 10 );

        if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_biofeedback] > 0 && bio_chance >= 9
             && ch->move > 49 ) {
            send_to_char( "&RYour biofeedback rapidly heals your wounds.&D\r\n", ch );

            if ( ch->Class == CLASS_MONK ) {
                adjust_focus( ch, -3 );
                if ( ch->focus_level < 35 )
                    gain *= 12;
                else
                    gain *= 18;
            }
            else
                gain *= 10;

            ch->move -= 50 - get_curr_dex( ch );
            learn_from_success( ch, gsn_biofeedback );
        }

        // For rapid healing autoskill. -Taon

        rh_chance = number_chance( 1, 5 );

        if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_rapid_healing] > 0 && rh_chance > 4 ) {
            if ( ch->pcdata->learned[gsn_rapid_healing] <= 30 ) {
                send_to_char( "&WYou feel as if you are healing at a higher rate.&D\r\n", ch );
                gain *= ( int ) ( double ) ( 2.4 );
                ch->move -= 10;
            }
            else if ( ch->pcdata->learned[gsn_rapid_healing] <= 50 ) {
                send_to_char( "&WYou feel as if you're rapidly healing.&D\r\n", ch );
                gain *= ( int ) ( double ) ( 2.5 );
                ch->move -= 15;
            }
            else if ( ch->pcdata->learned[gsn_rapid_healing] <= 70 ) {
                send_to_char( "&WYou feel as if you're rapidly healing.&D\r\n", ch );
                gain *= ( int ) ( double ) ( 3.1 );
                ch->move -= 20;
            }
            else if ( ch->pcdata->learned[gsn_rapid_healing] < 90 ) {
                send_to_char( "&WYour body feels as if it's healing at an intense rate.&D\r\n",
                              ch );
                gain *= ( int ) ( double ) ( 3.5 );
                ch->move -= 25;
            }
            else if ( ch->pcdata->learned[gsn_rapid_healing] >= 90 ) {
                send_to_char( "&WYour body feels as if it's healing at an intense rate.&D\r\n",
                              ch );
                gain *= ( int ) ( double ) ( 3.8 );
                ch->move -= 30;
            }
            learn_from_success( ch, gsn_rapid_healing );
        }

        // A little faith support. -Taon
        if ( ch->pcdata->learned[gsn_faith] > 0 ) {
            if ( number_chance( 1, 6 ) == 5 ) {
                adjust_faith( ch, -2 );
                learn_from_success( ch, gsn_faith );
            }

            if ( IS_AFFECTED( ch, AFF_PRAYER ) )
                gain += ch->pcdata->learned[gsn_faith];
            else
                gain += ch->pcdata->learned[gsn_faith] / 2;
        }

        // A little aura of life support. -Taon
        if ( IS_AFFECTED( ch, AFF_AURA_LIFE ) && number_chance( 1, 10 ) >= 9 ) {
            send_to_char( "You gather healing energies from your aura of life.\r\n", ch );
            gain += number_chance( 5, 20 );
        }

        // More focus support. -Taon
        if ( ch->pcdata->learned[gsn_focus] > 0 ) {
            if ( number_chance( 1, 10 ) == 10 )
                adjust_focus( ch, -1 );

            if ( ch->focus_level < 20 )
                gain += ch->pcdata->learned[gsn_focus] / 3;
            else if ( ch->focus_level < 30 )
                gain += ch->pcdata->learned[gsn_focus] / 2;
            else if ( ch->focus_level < 40 )
                gain += ( ch->pcdata->learned[gsn_focus] / 2 ) + 10;
            else
                gain += ch->pcdata->learned[gsn_focus];
        }

        if ( !IS_NPC( ch ) && ( ch->race == RACE_TROLL || IS_AFFECTED( ch, AFF_STIRRING ) ) )
            gain *= 3;
        if ( ch->on != NULL && ch->on->item_type == ITEM_FURNITURE && ( ch->on->value[3] > 0 ) )
            gain *= ch->on->value[3] / 100;
        if ( ch->pcdata->condition[COND_FULL] == 0 )
            gain /= ( int ) ( double ) ( 1.5 );
        if ( ch->pcdata->condition[COND_THIRST] == 0 )
            gain /= ( int ) ( double ) ( 1.5 );
    }
    // A little wilderness support. -Taon
    if ( ch->in_room->sector_type == SECT_CAMPSITE && IN_WILDERNESS( ch ) )
        gain *= 2;
    if ( IS_AFFECTED( ch, AFF_POISON ) )
        gain /= 4;
    return UMIN( gain, ch->max_hit - ch->hit );
}

int mana_gain( CHAR_DATA *ch )
{
    int                     gain;
    OBJ_DATA               *obj;

    if ( IS_NPC( ch ) )
        gain = ch->level;
    else {
        gain = UMIN( 5, ch->level );

/* Volk: Can't gain while dying! :P */
        if ( ch->position < POS_SLEEPING )
            return 0;

        switch ( ch->position ) {
            case POS_SLEEPING:
                gain += ( int ) ( double ) ( get_curr_int( ch ) * 3.50 );
                break;
            case POS_RESTING:
                gain += ( int ) ( double ) ( get_curr_int( ch ) * 2.25 );
                break;
            case POS_MEDITATING:
                gain +=
                    ( int ) ( double ) ( ( get_curr_int( ch ) * 5 ) + ch->level +
                                         LEARNED( ch, gsn_meditate ) );
                break;
        }

        if ( ch->pcdata->condition[COND_FULL] == 0 )
            gain /= 2;

        if ( ch->pcdata->condition[COND_THIRST] == 0 )
            gain /= 2;

    }

    if ( ch->on != NULL && ch->on->item_type == ITEM_FURNITURE && ( ch->on->value[4] > 0 ) )
        gain = gain * ch->on->value[4] / 100;

    if ( IS_AFFECTED( ch, AFF_POISON ) )
        gain /= 4;

    if ( IS_AFFECTED( ch, AFF_MANA_POOL ) )
        gain *= ( int ) ( double ) ( 1.5 );

    if ( IS_AFFECTED( ch, AFF_CRAFTED_DRINK1 ) )
        gain *= 3;
    if ( IS_AFFECTED( ch, AFF_CRAFTED_DRINK2 ) )
        gain *= 6;
    if ( IS_AFFECTED( ch, AFF_CRAFTED_DRINK3 ) )
        gain *= 10;

    // Not sure how this will work out just want to make it beneficial to be baker, may
    // adjust later.
    if ( IS_AFFECTED( ch, AFF_CRAFTED_DRINK1 )
         && ( !IS_NPC( ch ) && ch->position == POS_FIGHTING ) )
        gain += ( int ) ( double ) ( get_curr_int( ch ) / 3 );
    if ( IS_AFFECTED( ch, AFF_CRAFTED_DRINK2 )
         && ( !IS_NPC( ch ) && ch->position == POS_FIGHTING ) )
        gain += ( int ) ( double ) ( get_curr_int( ch ) / 2 );
    if ( IS_AFFECTED( ch, AFF_CRAFTED_DRINK3 )
         && ( !IS_NPC( ch ) && ch->position == POS_FIGHTING ) )
        gain += ( int ) ( double ) ( get_curr_int( ch ) );

    if ( !IS_NPC( ch ) && ch->race == RACE_DRAGON && ch->position != POS_FIGHTING ) {
        for ( obj = ch->last_carrying; obj; obj = obj->prev_content ) {
            if ( obj->item_type == ITEM_TREASURE && ch->level >= obj->level ) { /* Volk - 
                                                                                 * bug
                                                                                 * fix */
                if ( obj->value[2] == 0 )
                    gain = ( int ) ( double ) ( gain * 1.5 );
                if ( obj->value[2] == 1 )
                    gain = ( int ) ( double ) ( gain * 2.2 );
                if ( obj->value[2] == 2 )
                    gain = ( int ) ( double ) ( gain * 2.5 );
                if ( obj->value[2] == 3 )
                    gain = ( int ) ( gain * 3 );

            }
        }
    }

/* added by Vladaar to give people a reason to be a singleclassed caster
 Volk: Indent your code, make it easier for people to fix later. :P  Let's use the ifchecks defined in mud.h.. 
 Also going to make this ANY single class, because you've missed necro and others.. */
    if ( !xIS_SET( race_table[ch->race]->flags, RACE_ADVANCED ) && !IS_SECONDCLASS( ch ) )
        gain *= ( int ) ( double ) ( 1.5 );

/* and then ON TOP of that, let's have casters gain a bit more */
    if ( IS_CASTER( ch ) )
        gain *= ( int ) ( double ) ( 1.5 );
    if ( IS_2CASTER( ch ) )
        gain *= ( int ) ( double ) ( 1.2 );

    if ( ch->level > 40 )
        gain *= 2;

    return UMIN( gain, ch->max_mana - ch->mana );
}

int move_gain( CHAR_DATA *ch )
{
    int                     gain;
    OBJ_DATA               *obj;

    if ( IS_NPC( ch ) ) {
        gain = ch->level;
    }
    else {
        gain = UMAX( 15, 2 * ch->level + 20 );

        switch ( ch->position ) {
            case POS_DEAD:
                return 0;
            case POS_MORTAL:
                return -1;
            case POS_INCAP:
                return -1;
            case POS_STUNNED:
                return 1;
            case POS_SLEEPING:
                gain += ( int ) ( double ) ( get_curr_dex( ch ) * 4.5 );
                break;
            case POS_RESTING:
                gain += ( int ) ( double ) ( get_curr_dex( ch ) * 2.5 );
                break;
        }
        // For autoskill endurance. -Taon
        if ( ch->pcdata->learned[gsn_endurance] > 0 ) {
            gain += ( ch->pcdata->learned[gsn_endurance] / 2 ) + ( get_curr_dex( ch ) / 3 );
            if ( number_chance( 1, 4 ) > 3 )
                learn_from_success( ch, gsn_endurance );
        }
        if ( IS_VAMPIRE( ch ) ) {
            if ( ch->blood <= 1 )
                gain /= 2;
            else if ( ch->blood >= ( 8 + ch->level ) )
                gain *= 2;
            if ( IS_OUTSIDE( ch ) ) {
                switch ( time_info.sunlight ) {
                    case SUN_RISE:
                    case SUN_SET:
                        gain /= 2;
                        break;
                    case SUN_LIGHT:
                        gain /= 4;
                        break;
                }
            }
        }
        if ( ch->pcdata->condition[COND_FULL] == 0 )
            gain /= 2;

        if ( ch->pcdata->condition[COND_THIRST] == 0 )
            gain /= 2;
    }

    if ( ch->on != NULL && ch->on->item_type == ITEM_FURNITURE && ( ch->on->value[3] > 0 ) )
        gain = gain * ch->on->value[3] / 100;

    if ( !IS_NPC( ch ) && ch->race == RACE_DRAGON && ch->position != POS_FIGHTING ) {
        for ( obj = ch->last_carrying; obj; obj = obj->prev_content ) {
            if ( obj->item_type == ITEM_TREASURE && ch->level >= obj->level ) { /* Volk - 
                                                                                 * bug
                                                                                 * fix */
                if ( obj->value[2] == 0 )
                    gain = ( int ) ( double ) ( gain * 1.5 );
                if ( obj->value[2] == 1 )
                    gain = ( int ) ( double ) ( gain * 2.2 );
                if ( obj->value[2] == 2 )
                    gain = ( int ) ( double ) ( gain * 2.5 );
                if ( obj->value[2] == 3 )
                    gain = ( int ) ( gain * 3 );

            }
        }
    }

    // A little wilderness support. -Taon
    if ( ch->in_room->sector_type == SECT_CAMPSITE && IN_WILDERNESS( ch ) )
        gain *= 2;
    if ( IS_AFFECTED( ch, AFF_FURY ) )
        gain /= 2;
    if ( IS_AFFECTED( ch, AFF_POISON ) )
        gain /= 4;

    return UMIN( gain, ch->max_move - ch->move );
}

/* Volk changed this a bit to low thirst */
void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
    int                     condition;
    ch_ret                  retcode = rNONE;

    if ( value == 0 || IS_NPC( ch ) || ch->level >= LEVEL_IMMORTAL || NEW_AUTH( ch ) )
        return;

    if ( IS_BLOODCLASS( ch ) )
        return;

    if ( IS_AVATAR( ch ) || IS_DUALAVATAR( ch ) || IS_TRIAVATAR( ch ) )
        return;

    condition = ch->pcdata->condition[iCond];
    ch->pcdata->condition[iCond] = URANGE( 0, condition + value, 48 );

    if ( ch->pcdata->condition[iCond] == 0 ) {
        switch ( iCond ) {
            case COND_FULL:
                set_char_color( AT_HUNGRY, ch );
                send_to_char( "You are STARVING!\r\n", ch );
                act( AT_HUNGRY, "$n is starved half to death!", ch, NULL, NULL, TO_ROOM );
                if ( !IS_PKILL( ch ) || number_bits( 1 ) == 0 )
                    worsen_mental_state( ch, 1 );
                if ( ch->position == POS_SLEEPING ) {
                    send_to_char( "\r\nYou awaken out of your sleep from lack of food!\r\n", ch );
                }
                retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
                break;

            case COND_THIRST:
                set_char_color( AT_THIRSTY, ch );
                send_to_char( "You are DYING of THIRST!\r\n", ch );
                act( AT_THIRSTY, "$n is dying of thirst!", ch, NULL, NULL, TO_ROOM );
                worsen_mental_state( ch, IS_PKILL( ch ) ? 1 : 2 );
                if ( ch->position == POS_SLEEPING ) {
                    send_to_char( "\r\nYou awaken out of your sleep from lack of drink!\r\n", ch );
                }
                retcode = damage( ch, ch, 2, TYPE_UNDEFINED );
                break;

            case COND_DRUNK:
                if ( condition != 0 ) {
                    set_char_color( AT_SOBER, ch );
                    send_to_char( "You're sober.\r\n", ch );
                }
                retcode = rNONE;
                break;
            default:
                bug( "Gain_condition: invalid condition type %d", iCond );
                retcode = rNONE;
                break;
        }
    }

    if ( retcode != rNONE )
        return;

    if ( ch->pcdata->condition[iCond] == 1 ) {
        switch ( iCond ) {
            case COND_FULL:
                set_char_color( AT_HUNGRY, ch );
                send_to_char( "You are really hungry.\r\n", ch );
                act( AT_HUNGRY, "You can hear $n's stomach growling.", ch, NULL, NULL, TO_ROOM );
                if ( number_bits( 1 ) == 0 )
                    worsen_mental_state( ch, 1 );
                break;

            case COND_THIRST:
                set_char_color( AT_THIRSTY, ch );
                send_to_char( "You are really thirsty.\r\n", ch );
                worsen_mental_state( ch, 1 );
                act( AT_THIRSTY, "$n looks a little parched.", ch, NULL, NULL, TO_ROOM );
                break;

            case COND_DRUNK:
                if ( condition != 0 ) {
                    set_char_color( AT_SOBER, ch );
                    send_to_char( "You are feeling a little less light headed.\r\n", ch );
                }
                break;
        }
    }

    if ( ch->pcdata->condition[iCond] == 2 ) {
        switch ( iCond ) {
            case COND_FULL:
                set_char_color( AT_HUNGRY, ch );
                send_to_char( "You are hungry.\r\n", ch );
                break;

            case COND_THIRST:
                set_char_color( AT_THIRSTY, ch );
                send_to_char( "You are thirsty.\r\n", ch );
                break;

        }
    }

    if ( ch->pcdata->condition[iCond] == 3 ) {
        switch ( iCond ) {
            case COND_FULL:
                set_char_color( AT_HUNGRY, ch );
                send_to_char( "You are a mite peckish.\r\n", ch );
                break;

            case COND_THIRST:
                set_char_color( AT_THIRSTY, ch );
                send_to_char( "You could use a sip of something refreshing.\r\n", ch );
                break;

        }
    }

    return;
}

void decide_distance( CHAR_DATA *ch )
{
    CHAR_DATA              *victim;

    short                   chance;

    chance = number_range( 1, 4 );

    if ( chance = 2 ) {
        if ( ( victim = who_fighting( ch ) ) != NULL ) {
            if ( victim->Class == CLASS_MAGE || victim->secondclass == CLASS_MAGE
                 || victim->thirdclass == CLASS_MAGE ) {
// do_rush(ch, "");
                return;
            }
            else if ( victim->Class == CLASS_PRIEST || victim->secondclass == CLASS_PRIEST
                      || victim->thirdclass == CLASS_PRIEST ) {
// do_rush(ch, "");
                return;
            }
            else {
// do_retreat(ch, "");
                return;
            }
        }
        return;

    }

}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Mud cpu time.
 */
void mobile_update( void )
{
    char                    buf[MSL];
    CHAR_DATA              *ch;
    EXIT_DATA              *pexit;
    int                     door;
    ch_ret                  retcode;
    short                   spam_guard;                /* Taon */

    retcode = rNONE;

    /*
     * Examine all mobs. 
     */
    for ( ch = last_char; ch; ch = gch_prev ) {
        set_cur_char( ch );

        if ( ch == first_char && ch->prev ) {
            bug( "%s", "mobile_update: first_char->prev != NULL... fixed" );
            ch->prev = NULL;
        }

        if ( ch->sex > 2 )
            ch->sex = 1;

        gch_prev = ch->prev;

        if ( gch_prev && gch_prev->next != ch ) {
            bug( "FATAL: Mobile_update: %s->prev->next doesn't point to ch. Short-cutting here",
                 ch->name );
            gch_prev = NULL;
            ch->prev = NULL;
        }

        if ( !IS_NPC( ch ) ) {
            drunk_randoms( ch );
            hallucinations( ch );
            continue;
        }

        if ( IS_NPC( ch ) ) {
            if ( ch->influence == 19 )
                ch->influence = 20;
        }

        if ( !ch->in_room || IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_PARALYSIS ) )
            continue;

        if ( ( ch->pIndexData->vnum == MOB_VNUM_ANIMATED_CORPSE
               || ch->pIndexData->vnum == MOB_VNUM_ANIMATED_SKEL
               || ch->pIndexData->vnum == MOB_VNUM_PASSAGE ) && !IS_AFFECTED( ch, AFF_CHARM ) ) {
            if ( ch->in_room->first_person ) {
                if ( ch->pIndexData->vnum == MOB_VNUM_PASSAGE )
                    act( AT_MAGIC, "$n vanishes as $s master loses control.", ch, NULL, NULL,
                         TO_ROOM );
                else
                    act( AT_MAGIC, "$n returns to the dust from whence $e came.", ch, NULL, NULL,
                         TO_ROOM );
            }
            if ( IS_NPC( ch ) )                        /* Guard against purging switched? 
                                                        */
                extract_char( ch, TRUE );
            continue;
        }

        if ( IS_NPC( ch ) && ch->fighting != NULL ) {
            decide_distance( ch );
        }

        if ( ch->pIndexData->vnum == MOB_VNUM_WIZARD_EYE && !IS_AFFECTED( ch, AFF_WIZARD_EYE ) ) {
            if ( ch->in_room->first_person )
                act( AT_MAGIC, "$n blinks out of existance as your power source wanes.", ch, NULL,
                     NULL, TO_ROOM );

            if ( IS_NPC( ch ) )                        /* Guard against purging switched? 
                                                        */
                extract_char( ch, TRUE );
            continue;
        }

        if ( !xIS_SET( ch->act, ACT_RUNNING ) && !xIS_SET( ch->act, ACT_SENTINEL ) && !ch->fighting
             && ch->hunting ) {
            if ( ch->hit <= 100 )                      // Lets stop almost dead mobs from 
                                                       // 
                // 
                // 
                // continuing. -Taon 
                continue;

            WAIT_STATE( ch, 6 * PULSE_VIOLENCE );
            hunt_victim( ch );
            continue;
        }

// Wilderness check to see if mobs are still running around
        if ( ch->timer > 0 ) {
            --ch->timer;

            if ( ch->timer == 0 && !ch->fighting ) {
                act( AT_CYAN, "$n suddenly runs off into the distance.", ch, NULL, NULL, TO_ROOM );
                if ( IS_NPC( ch ) )                    /* Guard against purging switched? 
                                                        */
                    char_from_room( ch );
                char_to_room( ch, get_room_index( 4 ) );
            }
            continue;
        }

        /*
         * Examine call for special procedure 
         */
        if ( !xIS_SET( ch->act, ACT_RUNNING ) && ch->spec_fun && !is_mppaused( ch ) ) {
            if ( ( *ch->spec_fun ) ( ch ) )
                continue;
            if ( char_died( ch ) )
                continue;
        }

        /*
         * Check for mudprogram script on mob 
         */
        if ( HAS_PROG( ch->pIndexData, SCRIPT_PROG ) ) {
            mprog_script_trigger( ch );
            continue;
        }

        if ( ch != cur_char ) {
            bug( "%s", "Mobile_update: ch != cur_char after spec_fun" );
            continue;
        }

        /*
         * That's all for sleeping / busy monster 
         */
        if ( ch->position != POS_STANDING )
            continue;

        if ( xIS_SET( ch->act, ACT_MOUNTED ) ) {
            if ( xIS_SET( ch->act, ACT_AGGRESSIVE ) || xIS_SET( ch->act, ACT_WILD_AGGR ) )
                do_emote( ch, ( char * ) "snarls and growls." );
            continue;
        }

        // Created spam_guard variable to make this less spammy. -Taon
        spam_guard = number_chance( 1, 20 );

        if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE )
             && ( spam_guard >= 19
                  && ( xIS_SET( ch->act, ACT_AGGRESSIVE ) || xIS_SET( ch->act, ACT_WILD_AGGR ) ) ) )
            do_emote( ch, ( char * ) "glares around and snarls." );
        /*
         * MOBprogram random trigger 
         */
        if ( ch->in_room->area->nplayer > 0 ) {
            mprog_random_trigger( ch );
            if ( char_died( ch ) )
                continue;
            if ( ch->position < POS_STANDING )
                continue;
        }

        /*
         * MOBprogram hour trigger: do something for an hour 
         */
        mprog_hour_trigger( ch );

        if ( char_died( ch ) )
            continue;

        rprog_hour_trigger( ch );
        if ( char_died( ch ) )
            continue;

        if ( ch->position < POS_STANDING )
            continue;

        /*
         * Scavenge 
         */
        if ( xIS_SET( ch->act, ACT_SCAVENGER ) && ch->in_room->first_content
             && number_bits( 2 ) == 0 ) {
            OBJ_DATA               *obj;
            OBJ_DATA               *obj_best;
            int                     max;

            max = 1;
            obj_best = NULL;
            for ( obj = ch->in_room->first_content; obj; obj = obj->next_content ) {
                if ( CAN_WEAR( obj, ITEM_TAKE ) && obj->cost > max
                     && !IS_OBJ_STAT( obj, ITEM_BURIED ) ) {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if ( obj_best ) {
                obj_from_room( obj_best );
                obj_to_char( obj_best, ch );
                act( AT_ACTION, "$n gets $p.", ch, obj_best, NULL, TO_ROOM );
            }
        }

        /*
         * Wander 
         */
        if ( !xIS_SET( ch->act, ACT_RUNNING )
             && !xIS_SET( ch->act, ACT_SENTINEL )
             && !xIS_SET( ch->act, ACT_PROTOTYPE )
             && ( door = number_bits( 5 ) ) <= 9
             && ( pexit = get_exit( ch->in_room, door ) ) != NULL
             && pexit->to_room && !IS_SET( pexit->exit_info, EX_CLOSED )
             && !IS_SET( pexit->to_room->room_flags, ROOM_NO_MOB )
             && !IS_SET( pexit->to_room->room_flags, ROOM_DEATH )
             && ( !xIS_SET( ch->act, ACT_STAY_AREA )
                  || pexit->to_room->area == ch->in_room->area ) ) {
            retcode = move_char( ch, pexit, 0 );
            /*
             * If ch changes position due
             * to it's or someother mob's
             * movement via MOBProgs,
             * continue - Kahn 
             */
            if ( char_died( ch ) )
                continue;
            if ( retcode != rNONE || xIS_SET( ch->act, ACT_SENTINEL )
                 || ch->position < POS_STANDING )
                continue;
        }

        /*
         * Flee 
         */
        if ( ch->hit < ch->max_hit / 2
             && ( door = number_bits( 4 ) ) <= 9
             && ( pexit = get_exit( ch->in_room, door ) ) != NULL && pexit->to_room
             && !IS_SET( pexit->exit_info, EX_CLOSED )
             && !IS_SET( pexit->to_room->room_flags, ROOM_NO_MOB ) ) {
            CHAR_DATA              *rch;
            bool                    found;

            found = FALSE;
            for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
                if ( is_fearing( ch, rch ) ) {
                    switch ( number_bits( 2 ) ) {
                        case 0:
                            snprintf( buf, MSL, "yell Get away from me, %s!", rch->name );
                            break;
                        case 1:
                            snprintf( buf, MSL, "yell Leave me be, %s!", rch->name );
                            break;
                        case 2:
                            snprintf( buf, MSL, "yell %s is trying to kill me!  Help!", rch->name );
                            break;
                        case 3:
                            snprintf( buf, MSL, "yell Someone save me from %s!", rch->name );
                            break;
                    }
                    interpret( ch, buf );
                    found = TRUE;
                    break;
                }
            }
            if ( found )
                retcode = move_char( ch, pexit, 0 );
        }
    }
    return;
}

/* Anything that should be updating based on time should go here - like hunger/thirst for one */
void char_calendar_update( void )
{
    CHAR_DATA              *ch;

    for ( ch = last_char; ch; ch = gch_prev ) {
        if ( ch == first_char && ch->prev ) {
            bug( "%s", "char_calendar_update: first_char->prev != NULL... fixed" );
            ch->prev = NULL;
        }
        gch_prev = ch->prev;
        if ( gch_prev && gch_prev->next != ch ) {
            bug( "%s", "char_calendar_update: ch->prev->next != ch" );
            return;
        }

        if ( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) ) {
            gain_condition( ch, COND_DRUNK, -1 );

            /*
             * Newbies won't starve now - Samson 10-2-98 
             */
            if ( ch->in_room && ch->level > 3 )
                gain_condition( ch, COND_FULL, -1 + race_table[ch->race]->hunger_mod );

            /*
             * Newbies won't dehydrate now - Samson 10-2-98 
             */
            if ( ch->in_room && ch->level > 3 ) {
                int                     sector;

                sector = ch->in_room->sector_type;

                switch ( sector ) {
                    default:
                        gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
                        break;
                    case SECT_DESERT:
                        gain_condition( ch, COND_THIRST, -3 + race_table[ch->race]->thirst_mod );
                        break;
                    case SECT_UNDERWATER:
                    case SECT_OCEANFLOOR:
                        if ( number_bits( 1 ) == 0 )
                            gain_condition( ch, COND_THIRST,
                                            -1 + race_table[ch->race]->thirst_mod );
                        break;
                }
            }
        }
    }
}

/*
 * Update all chars, including mobs.
 * This function is performance sensitive.
 */
void char_update( void )
{
    CHAR_DATA              *ch;
    CHAR_DATA              *ch_save;
    short                   save_count = 0;

    ch_save = NULL;

    for ( ch = last_char; ch; ch = gch_prev ) {
        if ( ch == first_char && ch->prev ) {
            bug( "%s", "char_update: first_char->prev != NULL... fixed" );
            ch->prev = NULL;
        }
        gch_prev = ch->prev;
        set_cur_char( ch );
        if ( gch_prev && gch_prev->next != ch ) {
            bug( "%s", "char_update: ch->prev->next != ch" );
            return;
        }

        if ( IS_IMMORTAL( ch ) ) {
            ch->mental_state = -10;
        }

        if ( IS_AFFECTED( ch, AFF_OTTOS_DANCE ) ) {
            act( AT_PLAIN, "You keep dancing to the irresistable tune of ottos dance!", ch, NULL,
                 NULL, TO_CHAR );
            act( AT_PLAIN, "$n dances around to the irresistable tune of ottos dance!", ch, NULL,
                 NULL, TO_ROOM );
            if ( ch->move > 5 )
                ch->move -= 3;
        }

        if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
            xREMOVE_BIT( ch->affected_by, AFF_BURROW );
        }

        if ( IS_AFFECTED( ch, AFF_SOUND_WAVES ) ) {
            act( AT_PLAIN, "You can hardly concentrate with the deafening sound waves!", ch, NULL,
                 NULL, TO_CHAR );
        }

        /*
         * Do updates for double_exp 
         */
        if ( ch && ch->pcdata )
            char_dexpupdate( ch );

        // Prayer, faith, and focus support follows. -Taon
        // right now faith and focus both will increase until better solution
        // is found. Was made when monks were single class only.
        if ( IS_AFFECTED( ch, AFF_PRAYER ) ) {
            adjust_focus( ch, 5 );
            adjust_faith( ch, 10 );

            ch->move -= 3;

            if ( IS_AWAKE( ch ) && ch->Class == CLASS_HELLSPAWN || ch->Class == CLASS_SHADOWKNIGHT
                 || ch->Class == CLASS_VAMPIRE ) {
                switch ( number_range( 1, 20 ) ) {
                    default:
                        break;
                    case 1:
                    case 2:
                        act( AT_PLAIN, "You get the sudden urge to kneel down while chanting.", ch,
                             NULL, NULL, TO_CHAR );
                        act( AT_PLAIN, "$n kneels down and continues $s chanting.", ch, NULL, NULL,
                             TO_ROOM );
                        set_position( ch, POS_SITTING );
                        break;
                    case 3:
                    case 4:
                        act( AT_PLAIN, "While deeply chanting you utter foul desecrations.", ch,
                             NULL, NULL, TO_CHAR );
                        act( AT_PLAIN, "While deeply chanting $n utters foul desecrations.", ch,
                             NULL, NULL, TO_ROOM );
                        break;
                    case 5:
                    case 6:
                        act( AT_PLAIN,
                             "Your head weaves back and forth while you desecrate the area.", ch,
                             NULL, NULL, TO_CHAR );
                        act( AT_PLAIN,
                             "$n's head weaves back and forth while $e desecrates the area.", ch,
                             NULL, NULL, TO_ROOM );
                        break;
                    case 7:
                    case 8:
                        act( AT_PLAIN, "You begin to chant in an unknown tongue.", ch, NULL, NULL,
                             TO_CHAR );
                        act( AT_PLAIN, "$n begins to speak in an unknown tongue.", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                    case 9:
                        act( AT_PLAIN, "As you defile the area your soul feels invigorated.", ch,
                             NULL, NULL, TO_CHAR );
                        act( AT_PLAIN, "As $n defiles the area $s soul feels invigorated.", ch,
                             NULL, NULL, TO_ROOM );
                        break;
                    case 10:
                        act( AT_PLAIN, "You chant '&CMshlam, Malak, Mshlam&D'.", ch, NULL, NULL,
                             TO_CHAR );
                        act( AT_PLAIN, "$n chants '&CMshlam, Malak, Mshlam&D'.", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                }
            }
            else if ( IS_AWAKE( ch ) ) {
                // Allow fall throughs.  
                switch ( number_range( 1, 20 ) ) {
                    default:
                        break;
                    case 1:
                    case 2:
                        act( AT_PLAIN, "You get the sudden urge to kneel down while in prayer.", ch,
                             NULL, NULL, TO_CHAR );
                        act( AT_PLAIN, "$n kneels down and continues $s prayers.", ch, NULL, NULL,
                             TO_ROOM );
                        set_position( ch, POS_SITTING );
                        break;
                    case 3:
                    case 4:
                        act( AT_PLAIN, "While deep in prayer you utter the gospels of the creator.",
                             ch, NULL, NULL, TO_CHAR );
                        act( AT_PLAIN, "While deep in prayer $n utters the gospels of the creator.",
                             ch, NULL, NULL, TO_ROOM );
                        break;
                    case 5:
                    case 6:
                        act( AT_PLAIN, "Your head weaves back and forth while in prayer.", ch, NULL,
                             NULL, TO_CHAR );
                        act( AT_PLAIN, "$n's head weaves back and forth while $e hums a prayer.",
                             ch, NULL, NULL, TO_ROOM );
                        break;
                    case 7:
                    case 8:
                        act( AT_PLAIN, "You begin to chant in an unknown tongue.", ch, NULL, NULL,
                             TO_CHAR );
                        act( AT_PLAIN, "$n begins to speak in an unknown tongue.", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                    case 9:
                        act( AT_PLAIN, "Your soul feels invigorated as you praise the Creator.", ch,
                             NULL, NULL, TO_CHAR );
                        act( AT_PLAIN, "$n's soul feels invigorated as $e praises the Creator.", ch,
                             NULL, NULL, TO_ROOM );
                        break;
                    case 10:
                        act( AT_PLAIN, "You chant '&CMshlam, Malak, Mshlam&D'.", ch, NULL, NULL,
                             TO_CHAR );
                        act( AT_PLAIN, "$n chants '&CMshlam, Malak, Mshlam&D'.", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                }
            }
            learn_from_success( ch, gsn_focus );
        }

        /*
         * Do a room_prog rand check right off the bat
         * * if ch disappears (rprog might wax npc's), continue
         */

        if ( !IS_NPC( ch ) && ch->position == POS_MEDITATING && ch->hit > 0 ) {
            if ( ch->mana >= ch->max_mana ) {
                send_to_char( "Having regained your mana, you stop meditating and stand up.\r\n",
                              ch );
                set_position( ch, POS_STANDING );
            }
            else if ( number_bits( 5 ) == 0 )
                learn_from_success( ch, gsn_meditate );
        }

        if ( !IS_NPC( ch ) )
            rprog_random_trigger( ch );
        if ( char_died( ch ) )
            continue;
        if ( IS_NPC( ch ) )
            mprog_time_trigger( ch );
        if ( char_died( ch ) )
            continue;
        rprog_time_trigger( ch );
        if ( char_died( ch ) )
            continue;
        /*
         * See if player should be auto-saved. 
         */
        if ( !IS_NPC( ch ) && ( !ch->desc || ch->desc->connected == CON_PLAYING ) && ch->level >= 2
             && current_time - ch->save_time > ( sysdata.save_frequency * 60 ) )
            ch_save = ch;
        else
            ch_save = NULL;

        /*
         * hate_level check 
         */
        if ( ( !IS_NPC( ch ) && ch->position != POS_FIGHTING ) ) {
            ch->hate_level = 0;
        }

        if ( IS_AFFECTED( ch, AFF_GRENDALS_STANCE ) && ch->position != POS_FIGHTING ) {
            if ( ch->hit > ch->max_hit ) {
                ch->hit = ch->max_hit;
            }
            if ( ch->move < ch->max_move ) {
                ch->move = ch->move + 30;
            }
            send_to_char( "You feel exhausted as your grendals stance energy reserves fades.\r\n",
                          ch );
            affect_strip( ch, gsn_grendals_stance );
            xREMOVE_BIT( ch->affected_by, AFF_GRENDALS_STANCE );
        }

        if ( ch->position >= POS_STUNNED ) {
            if ( ch->hit < ch->max_hit )
                ch->hit += hit_gain( ch );
            if ( ch->mana < ch->max_mana )
                ch->mana += mana_gain( ch );
            if ( ch->move < ch->max_move )
                ch->move += move_gain( ch );
        }

        /*
         * Expire variables 
         */
        if ( ch->variables ) {
            VARIABLE_DATA          *vd,
                                   *vd_next = NULL,
                *vd_prev = NULL;

            for ( vd = ch->variables; vd; vd = vd_next ) {
                vd_next = vd->next;

                if ( vd->timer > 0 && --vd->timer == 0 ) {
                    if ( vd == ch->variables )
                        ch->variables = vd_next;
                    else if ( vd_prev )
                        vd_prev->next = vd_next;
                    delete_variable( vd );
                }
                else
                    vd_prev = vd;
            }
        }

        // The following is support for Aura of Life. -Taon
        if ( IS_AFFECTED( ch, AFF_AURA_LIFE ) && number_range( 1, 6 ) >= 5 ) {
            CHAR_DATA              *wch,
                                   *wch_next;

            for ( wch = ch->in_room->first_person; wch; wch = wch_next ) {
                wch_next = wch->next_in_room;

                if ( !wch )
                    break;
                if ( wch == ch )
                    continue;
                if ( who_fighting( ch ) == wch )
                    continue;
                if ( wch->hit >= wch->max_hit )
                    continue;
                if ( wch->alignment < 0 )
                    continue;
                if ( IS_NPC( wch ) )
                    continue;
                ch_printf( ch,
                           "You glow brightly as %s draws healing energies off your aura of life.\r\n",
                           wch->name );
                ch_printf( wch,
                           "%s glows brightly as you draw healing energies off their aura of life.\r\n",
                           ch->name );

                if ( ch->hit < ch->max_hit - 100 )
                    ch->hit += 100;
                else
                    ch->hit = ch->max_hit;
            }
        }

        if ( IS_AFFECTED( ch, AFF_FURY ) ) {
            if ( ch->move < 300 ) {
                send_to_char( "&WYou are too winded to maintain a furious state.\r\n", ch );
                affect_strip( ch, gsn_fury );
                xREMOVE_BIT( ch->affected_by, AFF_FURY );
            }
            else
                send_to_char( "&WYou are getting winded from your furious pace.\r\n", ch );
            ch->move -= 200;
        }

        update_pos( ch );

        if ( ( ch->morph ) && !IS_AFFECTED( ch, AFF_SHAPESHIFT ) )
            do_unshift( ch, ( char * ) "" );
        /*
         * To make people with a nuisance's flags life difficult 
         * * --Shaddai
         */
        if ( !IS_NPC( ch ) && ch->pcdata->nuisance ) {
            long int                temp;

            if ( ch->pcdata->nuisance->flags < MAX_NUISANCE_STAGE ) {
                temp = ch->pcdata->nuisance->max_time - ch->pcdata->nuisance->time;
                temp *= ch->pcdata->nuisance->flags;
                temp /= MAX_NUISANCE_STAGE;
                temp += ch->pcdata->nuisance->time;
                if ( temp < current_time )
                    ch->pcdata->nuisance->flags++;
            }
        }
        if ( !IS_NPC( ch ) && ch->level < LEVEL_IMMORTAL ) {
            OBJ_DATA               *obj;

            if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT
                 && obj->value[2] > 0 ) {
                if ( --obj->value[2] == 0 && ch->in_room ) {
                    ch->in_room->light -= obj->count;
                    if ( ch->in_room->light < 0 )
                        ch->in_room->light = 0;
                    act( AT_ACTION, "$p goes out.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "$p goes out.", ch, obj, NULL, TO_CHAR );
                    if ( obj->serial == cur_obj )
                        global_objcode = rOBJ_EXPIRED;
                    extract_obj( obj );
                }
            }
            if ( ++ch->timer >= 12 ) {
                if ( !IS_IDLE( ch ) && !ch->fighting ) {
                    if ( ch->fighting )
                        stop_fighting( ch, TRUE );
                    act( AT_ACTION, "$n disappears into the void.", ch, NULL, NULL, TO_ROOM );
                    send_to_char( "You disappear into the void.\r\n", ch );
                    if ( xIS_SET( sysdata.save_flags, SV_IDLE ) )
                        save_char_obj( ch );
                    SET_BIT( ch->pcdata->flags, PCFLAG_IDLE );
                    char_from_room( ch );
                    char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
                }
            }
            if ( ch->pcdata->condition[COND_DRUNK] > 8 )
                worsen_mental_state( ch, ch->pcdata->condition[COND_DRUNK] / 8 );
            if ( ch->pcdata->condition[COND_FULL] > 1 ) {
                switch ( ch->position ) {
                    case POS_SLEEPING:
                        better_mental_state( ch, 5 );
                        break;
                    case POS_RESTING:
                        better_mental_state( ch, 4 );
                        break;
                    case POS_SITTING:
                    case POS_MOUNTED:
                        better_mental_state( ch, 3 );
                        break;
                    case POS_STANDING:
                        better_mental_state( ch, 2 );
                        break;
                    case POS_FIGHTING:
                    case POS_EVASIVE:
                    case POS_DEFENSIVE:
                    case POS_AGGRESSIVE:
                    case POS_BERSERK:
                        if ( number_bits( 2 ) == 0 )
                            better_mental_state( ch, 1 );
                        break;
                }
            }
            if ( ch->pcdata->condition[COND_THIRST] > 1 ) {
                switch ( ch->position ) {
                    case POS_SLEEPING:
                        better_mental_state( ch, 6 );
                        break;
                    case POS_RESTING:
                        better_mental_state( ch, 5 );
                        break;
                    case POS_SITTING:
                    case POS_MOUNTED:
                        better_mental_state( ch, 4 );
                        break;
                    case POS_STANDING:
                        better_mental_state( ch, 2 );
                        break;
                    case POS_FIGHTING:
                    case POS_EVASIVE:
                    case POS_DEFENSIVE:
                    case POS_AGGRESSIVE:
                    case POS_BERSERK:
                        if ( number_bits( 2 ) == 0 )
                            better_mental_state( ch, 1 );
                        break;
                }
            }
            gain_condition( ch, COND_DRUNK, -1 );
            if ( !IS_VAMPIRE( ch ) && number_bits( 1 ) == 0 )
                gain_condition( ch, COND_FULL, 0 - race_table[ch->race]->hunger_mod );
            if ( IS_VAMPIRE( ch ) && ch->level >= 10 ) {
                if ( time_info.hour < 21 && time_info.hour > 5 )
                    ch->blood = ch->blood - 1;
            }
            if ( IS_VAMPIRE( ch ) ) {
                if ( ch->blood < 0 || ch->hit < -9 ) {
                    set_char_color( AT_BLOOD, ch );
                    send_to_char( "You have died from lack of blood!!\r\n", ch );
                    raw_kill( ch, ch );
                }
                else if ( ch->blood < 3 ) {
                    set_char_color( AT_BLOOD, ch );
                    send_to_char( "You are starved to feast on blood!\r\n", ch );
                    act( AT_BLOOD, "$n is suffering from lack of blood!", ch, NULL, NULL, TO_ROOM );
                    worsen_mental_state( ch, 2 );
                    if ( ch->level > 2 ) {
                        if ( ch->hit < 20 ) {
                            ch->hit = ch->hit - 1;
                        }
                        else {
                            ch->hit -= ch->max_hit / 20;
                        }
                    }
                }
            }
/* Volk: Was going to remove !IS_VAMPIRE as the Vampire race already has
 * 0 thirstmod and 0 hungermod, but am thinking eventually (hopefully!)
 * Vampire could be a flag instead of a race. So i'll leave this here. */

            if ( !IS_VAMPIRE( ch ) && number_bits( 1 ) == 0 && CAN_PKILL( ch )
                 && ( ch->pcdata->condition[COND_THIRST] - 9 > 10 ) )
                gain_condition( ch, COND_THIRST, ( 0 - race_table[ch->race]->thirst_mod ) / 2 );

            if ( !IS_NPC( ch ) && ch->pcdata->nuisance ) {
                int                     value;

                value = ( ( 0 - ch->pcdata->nuisance->flags ) * ch->pcdata->nuisance->power );
                gain_condition( ch, COND_THIRST, value );
                gain_condition( ch, COND_FULL, value );
            }

        }

        if ( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) && ch->pcdata->release_date > 0
             && ch->pcdata->release_date <= current_time ) {
            ROOM_INDEX_DATA        *location;

            if ( ch->pcdata->clan )
                location = get_room_index( ch->pcdata->clan->recall );
            else
                location = get_room_index( ROOM_VNUM_TEMPLE );
            if ( !location )
                location = ch->in_room;
            MOBtrigger = FALSE;
            char_from_room( ch );
            char_to_room( ch, location );
            send_to_char
                ( "The 6 Dragons STAFF have released you from hell as your sentence is up!\r\n",
                  ch );
            do_look( ch, ( char * ) "auto" );
            if ( VLD_STR( ch->pcdata->helled_by ) )
                STRFREE( ch->pcdata->helled_by );
            ch->pcdata->helled_by = NULL;
            ch->pcdata->release_date = 0;
            save_char_obj( ch );
        }
        if ( !char_died( ch ) ) {
            OBJ_DATA               *arrow = NULL;
            int                     dam = 0;

            if ( ( arrow = get_eq_char( ch, WEAR_LODGE_RIB ) ) != NULL ) {
                dam = number_range( ( 2 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
                act( AT_CARNAGE, "$n suffers damage from $p stuck in $s rib.", ch, arrow, NULL,
                     TO_ROOM );
                act( AT_CARNAGE, "You suffer damage from $p stuck in your rib.", ch, arrow, NULL,
                     TO_CHAR );
                damage( ch, ch, dam, TYPE_UNDEFINED );
            }
            if ( ( arrow = get_eq_char( ch, WEAR_LODGE_LEG ) ) != NULL ) {
                dam = number_range( arrow->value[1], arrow->value[2] );
                act( AT_CARNAGE, "$n suffers damage from $p stuck in $s leg.", ch, arrow, NULL,
                     TO_ROOM );
                act( AT_CARNAGE, "You suffer damage from $p stuck in your leg.", ch, arrow, NULL,
                     TO_CHAR );
                damage( ch, ch, dam, TYPE_UNDEFINED );
            }
            if ( ( arrow = get_eq_char( ch, WEAR_LODGE_ARM ) ) != NULL ) {
                dam = number_range( arrow->value[1], arrow->value[2] );
                act( AT_CARNAGE, "$n suffers damage from $p stuck in $s arm.", ch, arrow, NULL,
                     TO_ROOM );
                act( AT_CARNAGE, "You suffer damage from $p stuck in your arm.", ch, arrow, NULL,
                     TO_CHAR );
                damage( ch, ch, dam, TYPE_UNDEFINED );
            }
            if ( char_died( ch ) )
                continue;
        }
        if ( !char_died( ch ) ) {
            if ( ch->position == POS_INCAP )
                damage( ch, ch, 1, TYPE_UNDEFINED );
            else if ( ch->position == POS_MORTAL )
                damage( ch, ch, 4, TYPE_UNDEFINED );
            else if ( ch->position == POS_INCAP )
                damage( ch, ch, 2, TYPE_UNDEFINED );
            else if ( ch->position == POS_MORTAL )
                damage( ch, ch, 4, TYPE_UNDEFINED );
            if ( char_died( ch ) )
                continue;

            if ( IS_AFFECTED( ch, AFF_SHIELD ) ) {
                if ( IS_BLOODCLASS( ch ) ) {
                    if ( ch->blood < 25 ) {
                        act( AT_YELLOW,
                             "Your shield falls as you lack the blood power to maintain it anymore.",
                             ch, NULL, NULL, TO_CHAR );
                        affect_strip( ch, gsn_shield );
                        xREMOVE_BIT( ch->affected_by, AFF_SHIELD );
                    }
                    else
                        act( AT_YELLOW,
                             "You will more blood power into your shield to maintain its strength.",
                             ch, NULL, NULL, TO_CHAR );
                    ch->blood = ( ch->blood - 5 );
                }
                else {
                    if ( ch->mana < 25 ) {
                        act( AT_YELLOW,
                             "Your shield falls as you lack the energy to maintain it anymore.", ch,
                             NULL, NULL, TO_CHAR );
                        affect_strip( ch, gsn_shield );
                        xREMOVE_BIT( ch->affected_by, AFF_SHIELD );
                    }
                    else
                        act( AT_YELLOW,
                             "You will more energy into your shield to maintain its strength.", ch,
                             NULL, NULL, TO_CHAR );
                    ch->mana = ( ch->mana - 10 );
                }
            }

            if ( IS_AFFECTED( ch, AFF_HIGHER_MAGIC ) ) {
                if ( ch->mana < 100 ) {
                    act( AT_CYAN,
                         "Your mana surge is depleted and you no longer call upon the higher magic.",
                         ch, NULL, NULL, TO_CHAR );
                    affect_strip( ch, gsn_higher_magic );
                    xREMOVE_BIT( ch->affected_by, AFF_HIGHER_MAGIC );
                }
                else
                    act( AT_CYAN,
                         "You call upon more mana from your mana surge for the higher magic.", ch,
                         NULL, NULL, TO_CHAR );

                // Minor bug fix. -Taon
                if ( ch->mana - 50 >= 0 )
                    ch->mana -= 50;
                else
                    ch->mana = 0;

            }

            if ( IS_AFFECTED( ch, AFF_BODY ) ) {
                if ( ch->mana < 25 ) {
                    act( AT_CYAN,
                         "You return to your normal body as you lack the energy to maintain it anymore.",
                         ch, NULL, NULL, TO_CHAR );
                    affect_strip( ch, gsn_astral_body );
                    xREMOVE_BIT( ch->affected_by, AFF_BODY );
                }
                else
                    act( AT_CYAN, "You will more energy into your astral form to maintain it.", ch,
                         NULL, NULL, TO_CHAR );
                ch->mana = ( ch->mana - 10 );
            }

            /*
             * Recurring spell affect 
             */
            if ( IS_AFFECTED( ch, AFF_RECURRINGSPELL ) ) {
                AFFECT_DATA            *paf,
                                       *paf_next;
                SKILLTYPE              *skill;
                bool                    found = FALSE,
                    died = FALSE;

                for ( paf = ch->first_affect; paf; paf = paf_next ) {
                    paf_next = paf->next;
                    if ( paf->location == APPLY_RECURRINGSPELL ) {
                        found = TRUE;
                        if ( IS_VALID_SN( paf->modifier )
                             && ( skill = skill_table[paf->modifier] ) != NULL
                             && skill->type == SKILL_SPELL ) {
                            if ( ( *skill->spell_fun ) ( paf->modifier, ch->level, ch,
                                                         ch ) == rCHAR_DIED || char_died( ch ) ) {
                                died = TRUE;
                                break;
                            }
                        }
                    }
                }
                if ( died )
                    continue;
                if ( !found )
                    xREMOVE_BIT( ch->affected_by, AFF_RECURRINGSPELL );
            }
            if ( ch->mental_state >= 30 )
                switch ( ( ch->mental_state + 5 ) / 10 ) {
                    case 3:
                        send_to_char( "You feel feverish.\r\n", ch );
                        act( AT_ACTION, "$n looks kind of out of it.", ch, NULL, NULL, TO_ROOM );
                        break;
                    case 4:
                        send_to_char( "You do not feel well at all.\r\n", ch );
                        act( AT_ACTION, "$n doesn't look too good.", ch, NULL, NULL, TO_ROOM );
                        break;
                    case 5:
                        send_to_char( "You need help!\r\n", ch );
                        act( AT_ACTION, "$n looks like $e could use your help.", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                    case 6:
                        send_to_char( "Seekest thou a cleric.\r\n", ch );
                        act( AT_ACTION, "Someone should fetch a healer for $n.", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                    case 7:
                        send_to_char( "You feel reality slipping away...\r\n", ch );
                        act( AT_ACTION, "$n doesn't appear to be aware of what's going on.", ch,
                             NULL, NULL, TO_ROOM );
                        break;
                    case 8:
                        send_to_char( "You begin to understand... everything.\r\n", ch );
                        act( AT_ACTION, "$n starts ranting like a madman!", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                    case 9:
                        send_to_char( "You are ONE with the universe.\r\n", ch );
                        act( AT_ACTION,
                             "$n is ranting on about 'the answer', 'ONE' and other mumbo-jumbo...",
                             ch, NULL, NULL, TO_ROOM );
                        break;
                    case 10:
                        send_to_char( "You feel the end is near.\r\n", ch );
                        act( AT_ACTION, "$n is muttering and ranting in tongues...", ch, NULL, NULL,
                             TO_ROOM );
                        break;
                }
            if ( ch->mental_state <= -30 )
                switch ( ( abs( ch->mental_state ) + 5 ) / 10 ) {
                    case 10:
                        if ( ch->position > POS_SLEEPING ) {
                            if ( ( ch->position == POS_STANDING || ch->position < POS_FIGHTING )
                                 && number_percent(  ) + 10 < abs( ch->mental_state ) )
                                do_sleep( ch, ( char * ) "" );
                            else
                                send_to_char( "You're barely conscious.\r\n", ch );
                        }
                        break;
                    case 9:
                        if ( ch->position > POS_SLEEPING ) {
                            if ( ( ch->position == POS_STANDING || ch->position < POS_FIGHTING )
                                 && ( number_percent(  ) + 20 ) < abs( ch->mental_state ) )
                                do_sleep( ch, ( char * ) "" );
                            else
                                send_to_char( "You can barely keep your eyes open.\r\n", ch );
                        }
                        break;
                    case 8:
                        if ( ch->position > POS_SLEEPING ) {
                            if ( ch->position < POS_SITTING
                                 && ( number_percent(  ) + 30 ) < abs( ch->mental_state ) )
                                do_sleep( ch, ( char * ) "" );
                            else
                                send_to_char( "You're extremely drowsy.\r\n", ch );
                        }
                        break;
                    case 7:
                        if ( ch->position > POS_RESTING )
                            send_to_char( "You feel very unmotivated.\r\n", ch );
                        break;
                    case 6:
                        if ( ch->position > POS_RESTING )
                            send_to_char( "You feel sedated.\r\n", ch );
                        break;
                    case 5:
                        if ( ch->position > POS_RESTING )
                            send_to_char( "You feel sleepy.\r\n", ch );
                        break;
                    case 4:
                        if ( ch->position > POS_RESTING )
                            send_to_char( "You feel tired.\r\n", ch );
                        break;
                    case 3:
                        if ( ch->position > POS_RESTING )
                            send_to_char( "You could use a rest.\r\n", ch );
                        break;
                }
            if ( ch->timer > 24 )
                do_quit( ch, ( char * ) "" );
            else if ( ch == ch_save && xIS_SET( sysdata.save_flags, SV_AUTO ) && ++save_count < 10 )    /* save 
                                                                                                         * max 
                                                                                                         * of 
                                                                                                         * 10 
                                                                                                         * per 
                                                                                                         * tick 
                                                                                                         */
                save_char_obj( ch );
        }
    }
    return;
}

/* Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{
    OBJ_DATA               *obj;
    short                   AT_TEMP;

    for ( obj = last_object; obj; obj = gobj_prev ) {
        CHAR_DATA              *rch;
        const char             *message;

        separate_obj( obj );
        if ( obj == first_object && obj->prev ) {
            bug( "%s", "obj_update: first_object->prev != NULL... fixed" );
            obj->prev = NULL;
        }
        gobj_prev = obj->prev;
        if ( gobj_prev && gobj_prev->next != obj ) {
            bug( "%s", "obj_update: obj->prev->next != obj" );
            return;
        }

        if ( obj->cost == 0 ) {

        }

        if ( obj_extracted( obj ) )
            continue;
        if ( obj->carried_by )
            oprog_random_trigger( obj );
        else if ( obj->in_room && obj->in_room->area->nplayer > 0 )
            oprog_random_trigger( obj );
        if ( obj_extracted( obj ) )
            continue;
        if ( obj->item_type == ITEM_PIPE ) {
            if ( IS_SET( obj->value[3], PIPE_LIT ) ) {
                if ( --obj->value[1] <= 0 ) {
                    obj->value[1] = 0;
                    REMOVE_BIT( obj->value[3], PIPE_LIT );
                }
                else if ( IS_SET( obj->value[3], PIPE_HOT ) )
                    REMOVE_BIT( obj->value[3], PIPE_HOT );
                else {
                    if ( IS_SET( obj->value[3], PIPE_GOINGOUT ) ) {
                        REMOVE_BIT( obj->value[3], PIPE_LIT );
                        REMOVE_BIT( obj->value[3], PIPE_GOINGOUT );
                    }
                    else
                        SET_BIT( obj->value[3], PIPE_GOINGOUT );
                }
                if ( !IS_SET( obj->value[3], PIPE_LIT ) )
                    SET_BIT( obj->value[3], PIPE_FULLOFASH );
            }
            else
                REMOVE_BIT( obj->value[3], PIPE_HOT );
        }

        if ( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC ) {
            short                   timerfrac = UMAX( 1, obj->timer - 1 );

            if ( obj->item_type == ITEM_CORPSE_PC )
                timerfrac = ( int ) ( obj->timer / 8 + 1 );
            if ( obj->timer > 0 && obj->value[2] > timerfrac ) {
                char                    buf[MSL];
                char                    name[MSL];
                char                   *bufptr;

                bufptr = one_argument( obj->short_descr, name );
                bufptr = one_argument( bufptr, name );
                bufptr = one_argument( bufptr, name );
                separate_obj( obj );
                obj->value[2] = timerfrac;
                snprintf( buf, MSL, corpse_descs[UMIN( timerfrac - 1, 4 )], bufptr );
                if ( VLD_STR( obj->description ) )
                    STRFREE( obj->description );
                obj->description = STRALLOC( buf );
            }
        }

/*
    if( obj->item_type == ITEM_CORPSE_PC && obj->value[5] == 0 )
    {
      char name[MSL];
      char *bufptr;
      int frac = URANGE(1, obj->timer/8, 5);

      bufptr = one_argument( obj->short_descr, name );
      bufptr = one_argument( bufptr, name );
      bufptr = one_argument( bufptr, name );

      separate_obj( obj );
      obj->value[2] = frac;
      stralloc_printf( &obj->description, corpse_descs[frac], bufptr );
      write_corpses( NULL, bufptr, NULL );
    }

    if( obj->item_type == ITEM_CORPSE_PC && obj->value[5] == 1 )
      write_corpses( NULL, obj->short_descr + 14, NULL );

    if( obj->item_type == ITEM_CORPSE_NPC && obj->timer <= 5 )
    {
      char name[MSL];
      char *bufptr;
      bufptr = one_argument( obj->short_descr, name );
      bufptr = one_argument( bufptr, name );
      bufptr = one_argument( bufptr, name );

      separate_obj( obj );
      stralloc_printf( &obj->description, corpse_descs[obj->timer], bufptr );
    }
*/
        /*
         * don't let inventory decay 
         */
        if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) ) {
            continue;
        }

        /*
         * groundrot items only decay on the ground 
         */
        if ( IS_OBJ_STAT( obj, ITEM_GROUNDROT ) && !obj->in_room ) {
            continue;
        }
        if ( ( obj->timer <= 0 || --obj->timer > 0 ) ) {
            continue;
        }
        /*
         * if we get this far, object's timer has expired. 
         */
        AT_TEMP = AT_PLAIN;
        switch ( obj->item_type ) {
            default:
                if ( obj->pIndexData->vnum == OBJ_VNUM_VOMIT )
                    message = "$p dries up, and suddenly the room smells a little cleaner.";
                else
                    message = "$p mysteriously vanishes.";
                AT_TEMP = AT_PLAIN;
                break;
/*
      case ITEM_RESOURCE:
	message = "The magical glow surrounding $p fades away.";
	AT_TEMP = AT_MAGIC;
	break;
*/
            case ITEM_CONTAINER:
                message = "$p falls apart, tattered from age.";
                AT_TEMP = AT_OBJECT;
                break;
            case ITEM_PORTAL:
                message = "&cSuddenly $p unravels and winks from existence.";
                remove_portal( obj );
                obj->item_type = ITEM_TRASH;           /* so extract_obj */
                AT_TEMP = AT_MAGIC;                    /* doesn't remove_portal */
                break;
            case ITEM_FOUNTAIN:
                message = "$p dries up.";
                AT_TEMP = AT_BLUE;
                break;
            case ITEM_CORPSE_NPC:
            case ITEM_CORPSE_PC:
                message = "$p rots entirely away, leaving only a skeleton behind.";
                AT_TEMP = AT_MAGIC;
                break;
            case ITEM_SKELETON:
                message = "$p finally crumbles into dust.";
                AT_TEMP = AT_OBJECT;
                break;
            case ITEM_COOK:
            case ITEM_FOOD:
                message = "$p is devoured by a swarm of maggots.";
                AT_TEMP = AT_HUNGRY;
                break;
            case ITEM_BLOOD:
                message = "$p slowly seeps into the ground.";
                AT_TEMP = AT_BLOOD;
                break;
            case ITEM_BLOODSTAIN:
                message = "$p dries up into flakes and blows away.";
                AT_TEMP = AT_BLOOD;
                break;
            case ITEM_SCRAPS:
                message = "$p crumble and decay into nothing.";
                AT_TEMP = AT_OBJECT;
                break;
            case ITEM_FIRE:
                /*
                 * This is removed because it is done in obj_from_room
                 * * Thanks to gfinello@mail.karmanet.it for pointing this out.
                 * * --Shaddai
                 * if(obj->in_room)
                 * {
                 * --obj->in_room->light;
                 * if(obj->in_room->light < 0)
                 * obj->in_room->light = 0;
                 * }
                 */
                message = "$p burns out.";
                AT_TEMP = AT_FIRE;
        }
        if ( obj->carried_by && !xIS_SET( obj->extra_flags, ITEM_SKELEROT ) ) {
            act( AT_TEMP, message, obj->carried_by, obj, NULL, TO_CHAR );
        }
        else if ( obj->in_room && ( rch = obj->in_room->first_person ) != NULL
                  && !IS_OBJ_STAT( obj, ITEM_BURIED )
                  && !xIS_SET( obj->extra_flags, ITEM_SKELEROT ) ) {
            act( AT_TEMP, message, rch, obj, NULL, TO_ROOM );
            act( AT_TEMP, message, rch, obj, NULL, TO_CHAR );
        }

        if ( ( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
             && obj->timer < 1 ) {
            if ( obj->carried_by )
                empty_obj( obj, NULL, obj->carried_by->in_room, TRUE );
            else if ( obj->in_room )
                empty_obj( obj, NULL, obj->in_room, TRUE );

            char                    name[MSL],
                                    name2[MSL];
            char                   *bufptr;
            OBJ_DATA               *newskel;

            newskel = create_object( get_obj_index( OBJ_VNUM_SKELETON ), obj->level );
            newskel->timer = 5;

            bufptr = one_argument( obj->short_descr, name );
            bufptr = one_argument( bufptr, name );
            bufptr = one_argument( bufptr, name );
            mudstrlcpy( name2, bufptr, MSL );          /* Dunno why, but it's corrupting
                                                        * if I don't do this - Samson */

            stralloc_printf( &newskel->name, "skeleton %s", name2 );
            stralloc_printf( &newskel->short_descr, "the skeleton of %s", name2 );
            stralloc_printf( &newskel->description, "The skeleton of %s lies here.", name2 );

            if ( obj->carried_by )
                obj_to_char( newskel, obj->carried_by );
            else if ( obj->in_room )
                obj_to_room( newskel, obj->in_room );
        }
/*
    if(obj->item_type == ITEM_RESOURCE && xIS_SET(obj->extra_flags, ITEM_MAGIC) && obj->timer < 1)
    {
      if(obj->value[0] == 1) 
      {
        if(obj->carried_by)
          create_wood(obj->carried_by, obj->value[1], obj->value[2], FALSE, NULL);
        else if(obj->in_room)
          create_wood(NULL, obj->value[1], obj->value[2], FALSE, obj->in_room);
      }
    }
*/
        if ( obj->serial == cur_obj )
            global_objcode = rOBJ_EXPIRED;
        if ( obj->timer < 1 )
            extract_obj( obj );
    }
    return;
}

/*
 * Function to check important stuff happening to a player
 * This function should take about 5% of mud cpu time
 */
void char_check( void )
{
    CHAR_DATA              *ch,
                           *ch_next;
    EXIT_DATA              *pexit;
    static int              cnt = 0;
    int                     door,
                            retcode;

    /*
     * This little counter can be used to handle periodic events 
     */
    cnt = ( cnt + 1 ) % SECONDS_PER_TICK;

    for ( ch = first_char; ch; ch = ch_next ) {
        set_cur_char( ch );
        ch_next = ch->next;
        will_fall( ch, 0 );

        if ( char_died( ch ) )
            continue;

        if ( !IS_NPC( ch ) && ch->in_room->sector_type == SECT_ARCTIC && !IS_IMMORTAL( ch ) ) {
            fbite_msg( ch, 0 );
        }

        if ( IS_NPC( ch ) ) {
            if ( ( cnt & 1 ) )
                continue;

            /*
             * running mobs  -Thoric 
             */
            if ( xIS_SET( ch->act, ACT_RUNNING ) ) {
                if ( !xIS_SET( ch->act, ACT_SENTINEL ) && !ch->fighting && ch->hunting ) {

                    if ( ch->hit <= 100 )              // Taon
                        continue;

                    WAIT_STATE( ch, 8 * PULSE_VIOLENCE + 2 );
                    hunt_victim( ch );
                    continue;
                }

                if ( ch->spec_fun ) {
                    if ( ( *ch->spec_fun ) ( ch ) )
                        continue;
                    if ( char_died( ch ) )
                        continue;
                }

                if ( !xIS_SET( ch->act, ACT_SENTINEL )
                     && !xIS_SET( ch->act, ACT_PROTOTYPE )
                     && ( door = number_bits( 4 ) ) <= 9
                     && ( pexit = get_exit( ch->in_room, door ) ) != NULL
                     && pexit->to_room && !IS_SET( pexit->exit_info, EX_CLOSED )
                     && !IS_SET( pexit->to_room->room_flags, ROOM_NO_MOB )
                     && !IS_SET( pexit->to_room->room_flags, ROOM_DEATH )
                     && ( !xIS_SET( ch->act, ACT_STAY_AREA )
                          || pexit->to_room->area == ch->in_room->area ) ) {
                    retcode = move_char( ch, pexit, 0 );
                    if ( char_died( ch ) )
                        continue;
                    if ( retcode != rNONE || xIS_SET( ch->act, ACT_SENTINEL )
                         || ch->position < POS_STANDING )
                        continue;
                }
            }
            continue;
        }
        else {

            if ( ch->mount && ch->in_room != ch->mount->in_room ) {
                xREMOVE_BIT( ch->mount->act, ACT_MOUNTED );
                ch->mount = NULL;
                set_position( ch, POS_STANDING );
                send_to_char( "No longer upon your mount, you fall to the ground...\r\nOUCH!\r\n",
                              ch );
            }

            /*
             * Volk - reduce 30 second mptut timer to 0 
             */
            if ( ch->pcdata && ch->pcdata->tutorialtimer > 0 ) {
                ch->pcdata->tutorialtimer--;
                if ( ch->pcdata->tutorialtimer < 1 ) {
                    ch->pcdata->tutorialtimer = 0;
                    xREMOVE_BIT( ch->act, PLR_TUTORIAL );
                    // Volk - Insert a message here if you like, ie 'Your 30 second timer 
                    // 
                    // 
                    // has expired, you can now move again'.
                }
            }

            update_quest( ch );

/* Volk - BARDS NOW PLAY MUSIC! WOO! */
            if ( ch->pcdata && ch->pcdata->bard > 0 )
                bard_music( ch );
/* This function checks the bard timer, used while a song is currently playing.
   When the bard timer reaches 0 then the song stops. While music is playing the
   instrument can be damaged or go out of tune. */

/*  Put a call to the swim function. All swim related stuff now found in act_move.c  */
            if ( ch->in_room && IS_WATER_SECT( ch->in_room->sector_type ) ) {
                water_sink( ch, 5 );
                swim_check( ch, cnt );
            }

            if ( !IS_NPC( ch ) && ch->in_room && ch->in_room->sector_type
                 && ch->in_room->sector_type == SECT_ARCTIC ) {

                if ( ch->pcdata->frostbite >= max_holdbreath( ch ) && !IS_IMMORTAL( ch ) ) {
                    int                     dam;
                    short                   chance;

                    chance = number_range( 1, 4 );
                    ch->pcdata->frostbite = max_holdbreath( ch );
                    if ( chance > 2 ) {
                        if ( ch->resistant == 2 ) {
                            send_to_char
                                ( "&CIt is freezing, but your magical warmth wards off most of the damage!&D\r\n",
                                  ch );
                            dam = number_range( 1, 4 );
                            damage( ch, ch, dam, TYPE_UNDEFINED );
                        }
                        else if ( ch->resistant != 2 ) {
                            send_to_char( "&CYou feel numb from the cold! &RYOU ARE DYING!&D\r\n",
                                          ch );
                            dam = number_range( 5, 30 );
                            damage( ch, ch, dam, TYPE_UNDEFINED );
                        }
                    }
                }
                ch->pcdata->frostbite++;
            }

            if ( !IS_NPC( ch ) && ch->in_room && ch->in_room->sector_type
                 && ( ch->in_room->sector_type != SECT_ARCTIC && ch->pcdata
                      && ch->pcdata->frostbite > 0 ) && !IS_IMMORTAL( ch ) ) {
                ch->pcdata->frostbite -= max_holdbreath( ch ) / 20;
                if ( ch->pcdata->frostbite < 0 )
                    ch->pcdata->frostbite = 0;
                if ( number_bits( 2 ) == 0 ) {
                    send_to_char( "&CYou manage to thaw out somewhat.&D\r\n", ch );
                    fbite_msg( ch, 0 );
                }
            }

/* Volk - I know, dirty, dirty code. But it gets the job done for now. */
            if ( ( !IS_NPC( ch ) || ch->race != RACE_VAMPIRE ) && ch->in_room
                 && ch->in_room->sector_type && ( ch->in_room->sector_type == SECT_UNDERWATER
                                                  || ch->in_room->sector_type == SECT_OCEANFLOOR )
                 && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
                ch->pcdata->holdbreath++;

            if ( ( !IS_NPC( ch ) || ch->race != RACE_VAMPIRE ) && ch->in_room
                 && ch->in_room->sector_type
                 &&
                 ( ( ch->in_room->sector_type != SECT_UNDERWATER
                     && ch->in_room->sector_type != SECT_OCEANFLOOR )
                   || IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) && ch->pcdata
                 && ch->pcdata->holdbreath > 0 ) {
                ch->pcdata->holdbreath -= max_holdbreath( ch ) / 20;
                if ( ch->pcdata->holdbreath < 0 )
                    ch->pcdata->holdbreath = 0;
                if ( number_bits( 2 ) == 0 ) {
                    send_to_char( "&CYou manage to catch some of your breath back.&D\r\n", ch );
                    breath_msg( ch, 0 );
                }
            }

            if ( char_died( ch ) )
                continue;

            /*
             * beat up on link dead players 
             */
            if ( !ch->desc ) {
                CHAR_DATA              *wch,
                                       *wch_next;

                for ( wch = ch->in_room->first_person; wch; wch = wch_next ) {
                    wch_next = wch->next_in_room;

                    if ( !IS_NPC( wch ) || wch->fighting || IS_AFFECTED( wch, AFF_CHARM )
                         || !IS_AWAKE( wch ) || ( xIS_SET( wch->act, ACT_WIMPY ) && IS_AWAKE( ch ) )
                         || !can_see( wch, ch ) )
                        continue;

                    if ( is_hating( wch, ch ) ) {
                        found_prey( wch, ch );
                        continue;
                    }

                    if ( ( !xIS_SET( wch->act, ACT_AGGRESSIVE )
                           && !xIS_SET( wch->act, ACT_WILD_AGGR ) )
                         || xIS_SET( wch->act, ACT_MOUNTED )
                         || IS_SET( wch->in_room->room_flags, ROOM_SAFE ) )
                        continue;
                    global_retcode = multi_hit( wch, ch, TYPE_UNDEFINED );
                }
            }
        }
    }
}

/*
 * Aggress.
 *
 * for each descriptor
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function should take 5% to 10% of ALL mud cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 */
void aggr_update( void )
{
    DESCRIPTOR_DATA        *d,
                           *dnext;
    CHAR_DATA              *wch,
                           *ch,
                           *ch_next,
                           *vch,
                           *vch_next,
                           *victim;
    struct act_prog_data   *apdtmp;

#ifdef UNDEFD
    /*
     *  GRUNT!  To do
     *
     */
    if ( IS_NPC( wch ) && wch->mpactnum > 0 && wch->in_room->area->nplayer > 0 ) {
        MPROG_ACT_LIST         *tmp_act,
                               *tmp2_act;

        for ( tmp_act = wch->mpact; tmp_act; tmp_act = tmp_act->next ) {
            oprog_wordlist_check( tmp_act->buf, wch, tmp_act->ch, tmp_act->obj, tmp_act->vo,
                                  ACT_PROG );
            DISPOSE( tmp_act->buf );
        }
        for ( tmp_act = wch->mpact; tmp_act; tmp_act = tmp2_act ) {
            tmp2_act = tmp_act->next;
            DISPOSE( tmp_act );
        }
        wch->mpactnum = 0;
        wch->mpact = NULL;
    }
#endif
    /*
     * check mobprog act queue 
     */
    while ( ( apdtmp = mob_act_list ) != NULL ) {
        wch = ( CHAR_DATA * ) mob_act_list->vo;
        if ( !char_died( wch ) && wch->mpactnum > 0 ) {
            MPROG_ACT_LIST         *tmp_act;

            while ( ( tmp_act = wch->mpact ) != NULL ) {
                if ( tmp_act->obj && obj_extracted( tmp_act->obj ) )
                    tmp_act->obj = NULL;
                if ( tmp_act->ch && !char_died( tmp_act->ch ) )
                    mprog_wordlist_check( tmp_act->buf, wch, tmp_act->ch, tmp_act->obj, tmp_act->vo,
                                          ACT_PROG );
                wch->mpact = tmp_act->next;
                DISPOSE( tmp_act->buf );
                DISPOSE( tmp_act );
            }
            wch->mpactnum = 0;
            wch->mpact = NULL;
        }
        mob_act_list = apdtmp->next;
        DISPOSE( apdtmp );
    }
    /*
     * Just check descriptors here for victims to aggressive mobs
     * We can check for linkdead victims in char_check  -Thoric
     */
    for ( d = first_descriptor; d; d = dnext ) {
/* Volk - this line is crashing the hell out of us when I run MySQL stuff. How come? 
 * Maybe because it can't FIND d->next? Or because MySQL is stalling the descriptor? */
        dnext = d->next;
        if ( d->connected != CON_PLAYING || ( wch = d->character ) == NULL )
            continue;
        if ( char_died( wch ) || IS_NPC( wch ) || wch->level >= LEVEL_IMMORTAL || !wch->in_room )
            continue;
        for ( ch = wch->in_room->first_person; ch; ch = ch_next ) {
            int                     count;

            ch_next = ch->next_in_room;
            if ( !IS_NPC( ch ) || ch->fighting || IS_AFFECTED( ch, AFF_CHARM ) || !IS_AWAKE( ch )
                 || ( xIS_SET( ch->act, ACT_WIMPY ) && IS_AWAKE( wch ) ) || !can_see( ch, wch ) )
                continue;

            if ( is_hating( ch, wch ) ) {
                found_prey( ch, wch );
                continue;
            }
            if ( ( !xIS_SET( ch->act, ACT_AGGRESSIVE ) && !xIS_SET( ch->act, ACT_WILD_AGGR ) )
                 || xIS_SET( ch->act, ACT_MOUNTED )
                 || IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
                continue;
            /*
             * Ok we have a 'wch' player character and a 'ch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             *
             * Depending on flags set, the mob may attack another mob
             */
            count = 0;
            victim = NULL;
            /*
             * See if we should continue 
             */
            if ( !wch || !wch->in_room || !wch->in_room->first_person )
                continue;
            for ( vch = wch->in_room->first_person; vch; vch = vch_next ) {
                vch_next = vch->next_in_room;
/* Volk - added the middle bit re levels so wild_aggr mobs only attack if chars are higher or lower within 10 levels */
                if ( ( !IS_NPC( vch ) || ( xIS_SET( ch->act, ACT_WILD_AGGR ) && ( vch->level > ch->level || ch->level <= vch->level + 10 ) ) || xIS_SET( vch->act, ACT_ANNOYING ) ) /* && 
                                                                                                                                                                                     * (vch->level 
                                                                                                                                                                                     * > 
                                                                                                                                                                                     * ch->level 
                                                                                                                                                                                     * || 
                                                                                                                                                                                     * ch->level 
                                                                                                                                                                                     * <= 
                                                                                                                                                                                     * vch->level 
                                                                                                                                                                                     * + 
                                                                                                                                                                                     * 10) 
                                                                                                                                                                                     */
                     &&vch->level < LEVEL_IMMORTAL && ( !xIS_SET( ch->act, ACT_WIMPY )
                                                        || !IS_AWAKE( vch ) )
                     && can_see( ch, vch ) ) {
                    if ( number_range( 0, count ) == 0 )
                        victim = vch;
                    count++;
                }
            }
            if ( !victim && !xIS_SET( ch->act, ACT_WILD_AGGR ) ) {
                bug( "Aggr_update: null victim. [%d]", count );
                continue;
            }
            /*
             * backstabbing mobs (Thoric) 
             */
            if ( IS_NPC( ch ) && xIS_SET( ch->attacks, ATCK_BACKSTAB ) ) {
                OBJ_DATA               *obj;

                if ( !ch->mount && ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL
                     && ( obj->value[4] == WEP_1H_SHORT_BLADE ) && !victim->fighting
                     && victim->hit >= victim->max_hit ) {
                    check_attacker( ch, victim );
                    WAIT_STATE( ch, skill_table[gsn_backstab]->beats );
                    if ( !IS_AWAKE( victim ) || number_percent(  ) + 5 < ch->level ) {
                        global_retcode = multi_hit( ch, victim, gsn_backstab );
                        continue;
                    }
                    else {
                        global_retcode = damage( ch, victim, 0, gsn_backstab );
                        continue;
                    }
                }
            }
            global_retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
        }
    }
    return;
}

/* From interp.c */
bool check_social       args( ( CHAR_DATA *ch, char *command, char *argument ) );

/*
 * drunk randoms  - Tricops
 * (Made part of mobile_update  -Thoric)
 */
void drunk_randoms( CHAR_DATA *ch )
{
    CHAR_DATA              *rvch = NULL;
    CHAR_DATA              *vch;
    short                   drunk;
    short                   position;

    if ( IS_NPC( ch ) || ch->pcdata->condition[COND_DRUNK] <= 0 )
        return;

    if ( number_percent(  ) < 30 )
        return;

    drunk = ch->pcdata->condition[COND_DRUNK];
    position = ch->position;
    set_position( ch, POS_STANDING );

    if ( number_percent(  ) < ( 2 * drunk / 20 ) )
        check_social( ch, ( char * ) "burp", ( char * ) "" );
    else if ( number_percent(  ) < ( 2 * drunk / 20 ) )
        check_social( ch, ( char * ) "hiccup", ( char * ) "" );
    else if ( number_percent(  ) < ( 2 * drunk / 20 ) )
        check_social( ch, ( char * ) "drool", ( char * ) "" );
    else if ( number_percent(  ) < ( 2 * drunk / 20 ) )
        check_social( ch, ( char * ) "fart", ( char * ) "" );
    else if ( drunk > ( 10 + ( get_curr_con( ch ) / 5 ) )
              && number_percent(  ) < ( 2 * drunk / 18 ) ) {
        for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
            if ( number_percent(  ) < 10 )
                rvch = vch;
        check_social( ch, ( char * ) "puke", ( char * ) ( rvch ? rvch->name : ( char * ) "" ) );
    }

    set_position( ch, position );
    return;
}

/*
 * Random hallucinations for those suffering from an overly high mentalstate
 * (Hats off to Albert Hoffman's "problem child")  -Thoric
 */
void hallucinations( CHAR_DATA *ch )
{
    if ( ch->mental_state >= 30
         && number_bits( 5 - ( ch->mental_state >= 50 ) - ( ch->mental_state >= 75 ) ) == 0 ) {
        const char             *t;

        switch ( number_range( 1, UMIN( 21, ( ch->mental_state + 5 ) / 5 ) ) ) {
            default:
            case 1:
                t = "You feel very restless... you can't sit still.\r\n";
                break;
            case 2:
                t = "You're tingling all over.\r\n";
                break;
            case 3:
                t = "Your skin is crawling.\r\n";
                break;
            case 4:
                t = "You suddenly feel that something is terribly wrong.\r\n";
                break;
            case 5:
                t = "Those damn little fairies keep laughing at you!\r\n";
                break;
            case 6:
                t = "You can hear your mother crying...\r\n";
                break;
            case 7:
                t = "Have you been here before, or not?  You're not sure...\r\n";
                break;
            case 8:
                t = "Painful childhood memories flash through your mind.\r\n";
                break;
            case 9:
                t = "You hear someone call your name in the distance...\r\n";
                break;
            case 10:
                t = "Your head is pulsating... you can't think straight.\r\n";
                break;
            case 11:
                t = "The ground... seems to be squirming...\r\n";
                break;
            case 12:
                t = "You're not quite sure what is real anymore.\r\n";
                break;
            case 13:
                t = "It's all a dream... or is it?\r\n";
                break;
            case 14:
                t = "You hear your grandchildren praying for you to watch over them.\r\n";
                break;
            case 15:
                t = "They're coming to get you... coming to take you away...\r\n";
                break;
            case 16:
                t = "You begin to feel all powerful!\r\n";
                break;
            case 17:
                t = "You're light as air... the heavens are yours for the taking.\r\n";
                break;
            case 18:
                t = "Your whole life flashes by... and your future...\r\n";
                break;
            case 19:
                t = "You are everywhere and everything... you know all and are all!\r\n";
                break;
            case 20:
                t = "You feel immortal!\r\n";
                break;
            case 21:
                t = "Ahh... the power of a Supreme Entity... what to do...\r\n";
                break;
        }
        send_to_char( t, ch );
    }
    return;
}

void tele_update( void )
{
    TELEPORT_DATA          *tele,
                           *tele_next;

    if ( !first_teleport )
        return;

    for ( tele = first_teleport; tele; tele = tele_next ) {
        tele_next = tele->next;
        if ( --tele->timer <= 0 ) {
            if ( tele->room->first_person ) {
                EXT_BV                  temp_bits;

                xCLEAR_BITS( temp_bits );

                xSET_BIT( temp_bits, TELE_TRANSALL );
                if ( IS_SET( tele->room->room_flags, ROOM_TELESHOWDESC ) ) {
                    xSET_BIT( temp_bits, TELE_SHOWDESC );
                }
                teleport( tele->room->first_person, tele->room->tele_vnum, &temp_bits );
            }
            UNLINK( tele, first_teleport, last_teleport, next, prev );
            DISPOSE( tele );
        }
    }
}

/* Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
void update_handler( void )
{
    static int              pulse_weather;
    static int              pulse_area;
    static int              pulse_affect;
    static int              pulse_mobile;
    static int              pulse_violence;
    static int              pulse_point;
    static int              pulse_second;
    static int              pulse_time;
    struct timeval          stime;
    struct timeval          etime;
    static int              pulse_start_arena = PULSE_ARENA;
    static int              pulse_arena = PULSE_ARENA;
    static int              pulse_vote;

    if ( timechar ) {
        set_char_color( AT_PLAIN, timechar );
        send_to_char( "Starting update timer.\r\n", timechar );
        gettimeofday( &stime, NULL );
    }
    if ( --pulse_area <= 0 ) {
        pulse_area = number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 );
        area_update(  );
    }
    if ( --pulse_mobile <= 0 ) {
        pulse_mobile = PULSE_MOBILE;
        mobile_update(  );
    }

    if ( --pulse_vote <= 0 ) {
        pulse_vote = PULSE_PER_SECOND * 5;
        vote_update(  );
    }

    if ( --pulse_violence <= 0 ) {
        pulse_violence = PULSE_VIOLENCE;
        violence_update(  );
    }

    if ( arena_prep || challenge ) {
        if ( --pulse_start_arena <= 0 ) {
            pulse_start_arena = PULSE_ARENA;

            // Never let a challenge run down unless both combatants are there. -Taon
            if ( challenge && arena_population < 2 );
            else if ( prep_length > 0 ) {

                prep_length--;
                parse_entry( prep_length );

            }
            else if ( arena_population > 1 && !arena_underway )
                run_game(  );
            else if ( arena_population <= 1 && !arena_underway )
                silent_end(  );
        }
    }

    if ( arena_population <= 0 && arena_underway )     /* Incase there is one underway
                                                        * and no population just end it */
        silent_end(  );

    if ( arena_population && arena_underway ) {
        if ( --pulse_arena <= 0 ) {
            pulse_arena = PULSE_ARENA;
            do_game(  );
        }
    }

    if ( --pulse_time <= 0 ) {
        pulse_time = sysdata.pulsecalendar;
        char_calendar_update(  );
        dailyevents(  );
    }

    if ( --pulse_point <= 0 ) {
        pulse_point = number_range( ( int ) ( PULSE_TICK * 0.75 ), ( int ) ( PULSE_TICK * 1.25 ) );

        time_update(  );                               /* If looking for slower passing * 
                                                        * time, move this to just above *
                                                        * char_calendar_update( ); */
        char_update(  );
        obj_update(  );
        tag_update(  );
    }

    if ( --pulse_weather <= 0 ) {
        pulse_weather = sysdata.pulseweather;
        UpdateWeather(  );                             /* New Weather Updater -Kayle */
    }

    if ( --pulse_affect <= 0 ) {
        pulse_affect = sysdata.pulseaffect;
        char_affects(  );
    }

    if ( --pulse_second <= 0 ) {
        pulse_second = PULSE_PER_SECOND;
        char_check(  );
        reboot_check( 0 );
        check_pfiles( 0 );
    }
    if ( --auction->pulse <= 0 ) {
        auction->pulse = PULSE_AUCTION;
        auction_update(  );
    }

    if ( !starttimedoubleexp )
        starttimedoubleexp = current_time;             /* Make sure its set */
    /*
     * done to end the double exp if already running 
     */
    if ( doubleexp ) {
        if ( ( current_time - 3600 ) >= starttimedoubleexp )
            do_dexp( NULL, ( char * ) "" );
    }

    if ( !starttimecexp )
        starttimecexp = current_time;                  /* Make sure its set */
    /*
     * done to end the double craft exp if already running 
     */
    if ( cexp ) {
        if ( ( current_time - 3600 ) >= starttimecexp )
            do_cexp( NULL, ( char * ) "" );
    }

    if ( !starthappyhour )
        starthappyhour = current_time;
    if ( happyhouron ) {
        if ( ( current_time - 3600 ) >= starthappyhour )
            do_happyhour( NULL, ( char * ) "" );
    }

    /*
     * Volk - Coded for Vladaar, he wants to give a message every hour if 2 or more players 
     * on.. 
     */
    if ( !startrunevent )
        startrunevent = current_time;
    if ( ( current_time - 3600 ) >= startrunevent ) {
        /*
         * Check there are more than two players on 
         */
        int                     countx = 0;
        CHAR_DATA              *person,
                               *greet;

        for ( person = first_char; person; person = person->next ) {
            if ( IS_NPC( person ) || IS_IMMORTAL( person ) || !person->desc )
                continue;

            /*
             * AND GREETS ALTS 
             */
            if ( ( greet = get_char_world( person, ( char * ) "Greetmir" ) ) != NULL )
                if ( !str_cmp( person->desc->host, greet->desc->host ) )
                    continue;

            countx++;
        }
        if ( countx >= 2 )
            echo_to_all( number_range( 0, 10 ),
                         "Any Online STAFF REMINDER: Please entertain the players - run an event! See &WHELP TEMPLATE&D.",
                         ECHOTAR_IMM );
        startrunevent = current_time;
    }

    mppause_update(  );                                /* Check for pauseing mud progs
                                                        * -rkb */
    tele_update(  );
    aggr_update(  );
    obj_act_update(  );
    room_act_update(  );
    clean_obj_queue(  );                               /* dispose of extracted objects */
    clean_char_queue(  );                              /* dispose of dead mobs/quitting
                                                        * chars */

    handle_sieges(  );

    if ( timechar ) {
        gettimeofday( &etime, NULL );
        set_char_color( AT_PLAIN, timechar );
        send_to_char( "Update timing complete.\r\n", timechar );
        subtract_times( &etime, &stime );
        ch_printf( timechar, "Timing took %d.%06d seconds.\r\n", etime.tv_sec, etime.tv_usec );
        timechar = NULL;
    }
    tail_chain(  );
}

void do_dexp( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];

    if ( ch && doubleexp && ( !argument || str_cmp( argument, "off" ) ) ) {
        send_to_char( "Double exp is currently going.\r\n", ch );
        send_to_char( "If you wish to turn it off use \"dexp off\".\r\n", ch );
        return;
    }

    doubleexp = !doubleexp;
    starttimedoubleexp = current_time;
    save_sysdata( sysdata );                           /* Save the sysdata so it keeps it 
                                                        * updated on if doubleexp is
                                                        * going or not */

    if ( doubleexp )
        announce( "The 6 Dragons Double Experience Hour has begun!" );
    else
        announce( "The 6 Dragons Double Experience Hour is now over!" );

    snprintf( buf, sizeof( buf ), "%24.24s: %s %s Double Experience Hour", ctime( &current_time ),
              ( ch
                && VLD_STR( ch->name ) ) ? ch->name : "System", doubleexp ? "started" : "ended" );
    append_to_file( PLEVEL_FILE, buf );
}

void do_cexp( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];

    if ( ch && cexp && ( !argument || str_cmp( argument, "off" ) ) ) {
        send_to_char( "Crafting Double exp is currently going.\r\n", ch );
        send_to_char( "If you wish to turn it off use \"cexp off\".\r\n", ch );
        return;
    }

    cexp = !cexp;
    starttimecexp = current_time;
    save_sysdata( sysdata );

    if ( cexp )
        announce( "&GThe 6 Dragons Crafting Double Experience Hour has begun!" );
    else
        announce( "&GThe 6 Dragons Crafting Double Experience Hour is now over!" );

    snprintf( buf, sizeof( buf ), "%24.24s: %s %s Crafting Double Experience Hour",
              ctime( &current_time ), ( ch
                                        && VLD_STR( ch->name ) ) ? ch->name : "System",
              cexp ? "started" : "ended" );
    append_to_file( PLEVEL_FILE, buf );
}

void do_happyhour( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    CHAR_DATA              *person;
    int                     track;

    happyhouron = !happyhouron;
    starthappyhour = current_time;
    save_sysdata( sysdata );

    if ( happyhouron ) {
        announce( "The 6 Dragons Happy Hour is now in session!" );
        for ( person = first_char; person; person = person->next )
            for ( track = 0; track < MAX_KILLTRACK; track++ ) {
                if ( track == 0 )
                    send_to_char( "No kill history during Happy Hour! GO GO GO!\r\n", person );
                if ( person && !IS_NPC( person ) && person->pcdata
                     && person->pcdata->killed[track].vnum )
                    person->pcdata->killed[track].vnum = 0;
            }
    }
    else
        announce( "Boo! The 6 Dragons Happy Hour has been cancelled." );

    snprintf( buf, sizeof( buf ), "%24.24s: %s %s Happy Hour", ctime( &current_time ),
              ( ch
                && VLD_STR( ch->name ) ) ? ch->name : "System", happyhouron ? "started" : "ended" );
    append_to_file( PLEVEL_FILE, buf );
}

void remove_portal( OBJ_DATA *portal )
{
    ROOM_INDEX_DATA        *fromRoom,
                           *toRoom;
    EXIT_DATA              *pexit;
    bool                    found;

    if ( !portal ) {
        bug( "%s", "remove_portal: portal is NULL" );
        return;
    }

    fromRoom = portal->in_room;
    found = FALSE;
    if ( !fromRoom ) {
        bug( "%s", "remove_portal: portal->in_room is NULL" );
        return;
    }

    for ( pexit = fromRoom->first_exit; pexit; pexit = pexit->next )
        if ( IS_SET( pexit->exit_info, EX_PORTAL ) ) {
            found = TRUE;
            break;
        }

    if ( !found ) {
        bug( "remove_portal: portal not found in room %d!", fromRoom->vnum );
        return;
    }

    if ( pexit->vdir != DIR_PORTAL )
        bug( "remove_portal: exit in dir %d != DIR_PORTAL", pexit->vdir );

    if ( ( toRoom = pexit->to_room ) == NULL )
        bug( "%s", "remove_portal: toRoom is NULL" );

    extract_exit( fromRoom, pexit );

    return;
}

void reboot_check( time_t reset )
{
    const static char      *tmsg[] = { "You feel the ground shake as the end comes near!",
        "Lightning crackles in the sky above!",
        "Crashes of thunder sound across the land!",
        "The sky has suddenly turned midnight black.",
        "You notice the life forms around you slowly dwindling away.",
        "The seas across the realm have turned frigid.",
        "The aura of magic that surrounds the realms seems slightly unstable.",
        "You sense a change in the magical forces surrounding you."
    };
    static const int        times[] = { 60, 120, 180, 240, 300, 600, 900, 1800 };
    static const int        timesize =
        UMIN( sizeof( times ) / sizeof( *times ), sizeof( tmsg ) / sizeof( *tmsg ) );
    char                    buf[MSL];
    static int              trun;
    static bool             init = FALSE;

    if ( !init || reset >= current_time ) {
        for ( trun = timesize - 1; trun >= 0; trun-- )
            if ( reset >= current_time + times[trun] )
                break;
        init = TRUE;
        return;
    }

    if ( new_boot_time_t - boot_time < 60 * 60 * 18 && !set_boot_time->manual )
        return;

    if ( new_boot_time_t <= current_time ) {
        CHAR_DATA              *vch;

        /*
         * extern bool mud_down; 
         */

        if ( auction->item ) {
            snprintf( buf, MSL, "Sale of %s has been stopped by mud.", auction->item->short_descr );
            talk_auction( buf );
            obj_to_char( auction->item, auction->seller );
            auction->item = NULL;
            if ( auction->buyer && auction->buyer != auction->seller ) {
                GET_MONEY( auction->buyer, auction->currtype ) += auction->bet;
                send_to_char( "Your money has been returned.\r\n", auction->buyer );
            }
        }
        echo_to_all( AT_YELLOW,
                     "You are forced from these realms by a strong "
                     "magical presence\r\nas life here is reconstructed.", ECHOTAR_ALL );
        log_string( "Automatic Reboot" );
        for ( vch = first_char; vch; vch = vch->next )
            if ( !IS_NPC( vch ) )
                save_char_obj( vch );
        mud_down = TRUE;
        return;
    }

    if ( trun != -1 && new_boot_time_t - current_time <= times[trun] ) {
        echo_to_all( AT_YELLOW, tmsg[trun], ECHOTAR_ALL );
        if ( trun <= 5 )
            sysdata.DENY_NEW_PLAYERS = TRUE;
        --trun;
        return;
    }
    return;
}

/* the auction update*/

void auction_update( void )
{
    int                     tax,
                            pay;
    char                    buf[MSL];

    if ( !auction->item ) {
        if ( AUCTION_MEM > 0 && auction->history[0] && ++auction->hist_timer == 6 * AUCTION_MEM ) {
            int                     i;

            for ( i = AUCTION_MEM - 1; i >= 0; i-- ) {
                if ( auction->history[i] ) {
                    auction->history[i] = NULL;
                    auction->hist_timer = 0;
                    break;
                }
            }
        }
        return;
    }

    switch ( ++auction->going ) {                      /* increase the going state */
        case 1:                                       /* going once */
        case 2:                                       /* going twice */
            if ( auction->bet > auction->starting )
                snprintf( buf, MSL, "%s: going %s for %s %s.", auction->item->short_descr,
                          ( ( auction->going == 1 ) ? "once" : "twice" ), num_punct( auction->bet ),
                          curr_types[auction->currtype] );
            else
                snprintf( buf, MSL, "%s: going %s (bid not received yet).",
                          auction->item->short_descr,
                          ( ( auction->going == 1 ) ? "once" : "twice" ) );
            talk_auction( buf );
            break;

        case 3:                                       /* SOLD! */
            if ( !auction->buyer && auction->bet ) {
                bug( "Auction code reached SOLD, with NULL buyer, but %d %s bid", auction->bet,
                     curr_types[auction->currtype] );
                auction->bet = 0;
            }
            if ( auction->bet > 0 && auction->buyer != auction->seller ) {
                snprintf( buf, MSL, "%s sold to %s for %s.", auction->item->short_descr,
                          IS_NPC( auction->buyer ) ? auction->buyer->short_descr : auction->buyer->
                          name, num_punct( auction->bet ) );
                talk_auction( buf );
                act( AT_ACTION, "The auctioneer materializes before you, and hands you $p.",
                     auction->buyer, auction->item, NULL, TO_CHAR );
                act( AT_ACTION, "The auctioneer materializes before $n, and hands $m $p.",
                     auction->buyer, auction->item, NULL, TO_ROOM );
                if ( ( auction->buyer->carry_weight + get_obj_weight( auction->item, FALSE ) ) >
                     can_carry_w( auction->buyer ) ) {
                    act( AT_PLAIN, "$p is too heavy for you to carry with your current inventory.",
                         auction->buyer, auction->item, NULL, TO_CHAR );
                    act( AT_PLAIN, "$n is carrying too much to also carry $p, and $e drops it.",
                         auction->buyer, auction->item, NULL, TO_ROOM );
                    obj_to_room( auction->item, auction->buyer->in_room );
                }
                else
                    obj_to_char( auction->item, auction->buyer );
                pay = ( int ) ( double ) ( auction->bet * 0.9 );
                tax = auction->bet - pay;
                boost_economy( auction->seller->in_room->area, tax, auction->currtype );
                GET_MONEY( auction->seller, auction->currtype ) += pay; /* give him the
                                                                         * money, tax 10
                                                                         * % */
                ch_printf( auction->seller,
                           "The auctioneer pays you %d %s, charging an auction fee of %d %s.\r\n",
                           pay, curr_types[auction->currtype], tax, curr_types[auction->currtype] );
                auction->item = NULL;                  /* reset item */
                if ( xIS_SET( sysdata.save_flags, SV_AUCTION ) ) {
                    save_char_obj( auction->buyer );
                    save_char_obj( auction->seller );
                }
            }
            else {                                     /* not sold */

                snprintf( buf, MSL, "No bids received for %s - removed from auction.\r\n",
                          auction->item->short_descr );
                talk_auction( buf );
                act( AT_ACTION, "The auctioneer appears before you to return $p to you.",
                     auction->seller, auction->item, NULL, TO_CHAR );
                act( AT_ACTION, "The auctioneer appears before $n to return $p to $m.",
                     auction->seller, auction->item, NULL, TO_ROOM );
                if ( ( auction->seller->carry_weight + get_obj_weight( auction->item, FALSE ) ) >
                     can_carry_w( auction->seller ) ) {
                    act( AT_PLAIN,
                         "You drop $p as it is just too much to carry"
                         " with everything else you're carrying.", auction->seller, auction->item,
                         NULL, TO_CHAR );
                    act( AT_PLAIN,
                         "$n drops $p as it is too much extra weight"
                         " for $m with everything else.", auction->seller, auction->item, NULL,
                         TO_ROOM );
                    obj_to_room( auction->item, auction->seller->in_room );
                }
                else
                    obj_to_char( auction->item, auction->seller );
                tax = 10;
                if ( GET_MONEY( auction->seller, CURR_COPPER ) - tax < 0 ) {
                    tax = GET_MONEY( auction->seller, CURR_COPPER );
                    GET_MONEY( auction->seller, CURR_COPPER ) = 0;
                }
                else
                    GET_MONEY( auction->seller, CURR_COPPER ) -= tax;
                boost_economy( auction->seller->in_room->area, tax, CURR_COPPER );
                ch_printf( auction->seller,
                           "The auctioneer charges you an auction fee of %d %s.\r\n", tax,
                           curr_types[CURR_COPPER] );
                if ( xIS_SET( sysdata.save_flags, SV_AUCTION ) )
                    save_char_obj( auction->seller );
            }                                          /* else */
            auction->item = NULL;                      /* clear auction */
    }                                                  /* switch */
}                                                      /* func */

void subtract_times( struct timeval *etime, struct timeval *stime )
{
    etime->tv_sec -= stime->tv_sec;
    etime->tv_usec -= stime->tv_usec;
    while ( etime->tv_usec < 0 ) {
        etime->tv_usec += 1000000;
        etime->tv_sec--;
    }
    return;
}

void unburrowall( void )
{
    CHAR_DATA              *ch,
                           *ch_next;

    for ( ch = first_char; ch; ch = ch_next ) {
        ch_next = ch->next;

        if ( IS_AFFECTED( ch, AFF_BURROW ) ) {
            affect_strip( ch, gsn_burrow );
            xREMOVE_BIT( ch->affected_by, AFF_BURROW );
            set_position( ch, POS_STANDING );
            send_to_char( "You sense the sun setting in the distance, and hunger for blood!\r\n",
                          ch );
            act( AT_ORANGE, "You burrow up out of the cold ground!", ch, NULL, NULL, TO_CHAR );
            act( AT_ORANGE, "$n burrows up out of the ground.", ch, NULL, NULL, TO_NOTVICT );
            learn_from_success( ch, gsn_burrow );
        }
    }
}

void damagevampires( void )
{
    CHAR_DATA              *ch,
                           *ch_next;

    for ( ch = first_char; ch; ch = ch_next ) {
        ch_next = ch->next;

        if ( !IS_VAMPIRE( ch ) || !IS_OUTSIDE( ch ) || IS_IMMORTAL( ch ) || IS_NPC( ch )
             || NOT_AUTHED( ch ) || IS_AFFECTED( ch, AFF_BURROW ) )
            continue;
        switch ( time_info.sunlight ) {
            case SUN_RISE:
                act( AT_YELLOW, "You cringe in pain as the sun rises.", ch, NULL, NULL, TO_CHAR );
                act( AT_YELLOW, "$n cringes in pain as the sun rises.", ch, NULL, NULL, TO_ROOM );
                ch->hit -= ch->hit / 20;
                ch->blood = ch->blood - 1;
                break;

            case SUN_LIGHT:
                if ( ch->blood < 10 && ch->level > 2 ) {
                    act( AT_RED, "You did not have enough blood to withstand the sun's fury!", ch,
                         NULL, NULL, TO_CHAR );
                    act( AT_RED, "You begin to burn up in a explosion of fire!", ch, NULL, NULL,
                         TO_CHAR );
                    act( AT_RED, "$n begins to burn up in a explosion of fire from the sun's fury!",
                         ch, NULL, NULL, TO_ROOM );
                    ch->hit -= ch->hit / 10;
                }
                break;

            case SUN_SET:
                act( AT_YELLOW, "You cringe in pain as the sun sets.", ch, NULL, NULL, TO_CHAR );
                act( AT_YELLOW, "$n cringes in pain as the sun sets.", ch, NULL, NULL, TO_ROOM );
                ch->hit -= ch->hit / 20;
                ch->blood = ch->blood - 1;
                break;
        }
    }
}

/*
 * update the time
 */
void time_update( void )
{
    DESCRIPTOR_DATA        *d;
    int                     n;
    const char             *echo;                      /* echo string */
    int                     echo_color;                /* color for the echo */
    char                    buf[MSL];

    n = number_bits( 2 );
    echo = NULL;
    echo_color = AT_GREY;

    ++time_info.hour;

    if ( time_info.hour == sysdata.hourdaybegin || time_info.hour == sysdata.hoursunrise
         || time_info.hour == sysdata.hournoon || time_info.hour == sysdata.hoursunset
         || time_info.hour == sysdata.hournightbegin ) {
        for ( d = first_descriptor; d; d = d->next ) {
            if ( d->connected == CON_PLAYING && IS_OUTSIDE( d->character )
                 && IS_AWAKE( d->character ) ) {
                struct WeatherCell     *cell = getWeatherCell( d->character->in_room->area );

                switch ( time_info.hour ) {
                    case 6:
                        {
                            const char             *echo_strings[4] = {
                                "The day has begun.\r\n",
                                "The day has begun.\r\n",
                                "The sky slowly begins to glow.\r\n",
                                "The sun slowly embarks upon a new day.\r\n"
                            };
                            time_info.sunlight = SUN_RISE;
                            echo = echo_strings[n];
                            echo_color = AT_YELLOW;
                            break;
                        }

                    case 7:
                        {
                            const char             *echo_strings[4] = {
                                "The sun rises in the east.\r\n",
                                "The sun rises in the east.\r\n",
                                "The hazy sun rises over the horizon.\r\n",
                                "Day breaks as the sun lifts into the sky.\r\n"
                            };
                            time_info.sunlight = SUN_LIGHT;
                            echo = echo_strings[n];
                            echo_color = AT_ORANGE;
                            break;
                        }

                    case 12:
                        {
                            if ( getCloudCover( cell ) > 21 ) {
                                echo = "It's noon.\r\n";
                            }
                            else {
                                const char             *echo_strings[2] = {
                                    "The intensity of the sun heralds the noon hour.\r\n",
                                    "The sun's bright rays beat down upon your shoulders.\r\n"
                                };

                                echo = echo_strings[n % 2];
                            }
                            time_info.sunlight = SUN_LIGHT;
                            echo_color = AT_WHITE;
                            break;
                        }

                    case 18:
                        {
                            const char             *echo_strings[4] = {
                                "The sun slowly disappears in the west.\r\n",
                                "The reddish sun sets past the horizon.\r\n",
                                "The sky turns a reddish orange as the sun ends its journey.\r\n",
                                "The sun's radiance dims as it sinks in the sky.\r\n"
                            };
                            time_info.sunlight = SUN_SET;
                            echo = echo_strings[n];
                            echo_color = AT_RED;
                            break;
                        }

                    case 19:
                        {
                            if ( getCloudCover( cell ) > 21 ) {
                                const char             *echo_strings[2] = {
                                    "The night begins.\r\n",
                                    "Twilight descends around you.\r\n"
                                };

                                echo = echo_strings[n % 2];
                            }
                            else {
                                const char             *echo_strings[2] = {
                                    "The moon's gentle glow diffuses through the night sky.\r\n",
                                    "The night sky gleams with glittering starlight.\r\n"
                                };

                                echo = echo_strings[n % 2];
                            }
                            time_info.sunlight = SUN_DARK;
                            echo_color = AT_DBLUE;
                            break;
                        }
                }

                if ( !echo )
                    continue;
                set_char_color( echo_color, d->character );
                send_to_char( echo, d->character );
            }
        }
    }

    if ( time_info.hour >= 6 && time_info.hour < 18 )
        damagevampires(  );
    else if ( time_info.hour == 18 )
        unburrowall(  );

    if ( time_info.hour == sysdata.hourmidnight ) {
        time_info.hour = 0;
        time_info.day++;
        RandomizeCells(  );
    }

    if ( time_info.day >= sysdata.dayspermonth ) {
        time_info.day = 0;
        time_info.month++;
    }

    if ( time_info.month >= sysdata.monthsperyear ) {
        time_info.month = 0;
        time_info.year++;
    }
    calc_season(  );                                   /* Samson 5-6-99 */
    /*
     * Save game world time - Samson 1-21-99 
     */
    save_timedata(  );
}

void update_quest( CHAR_DATA *ch )
{
    CHQUEST_DATA           *chquest;
    int                     timer = 0,
        x = 0;

    if ( !ch || IS_NPC( ch ) || !ch->pcdata )
        return;

    for ( chquest = ch->pcdata->first_quest; chquest; chquest = chquest->next ) {
        if ( chquest->questlimit > 0 ) {
            chquest->questlimit--;
            timer = chquest->questlimit;

            if ( timer == 0 ) {
                send_to_char( "&RYou have run out of time for your quest!\r\n", ch );
                send_to_char( "You must start the quest again.&D\r\n", ch );
                chquest->progress = 0;
                chquest->chaplimit = 0;
                chquest->kamount = 0;
                chquest->times++;
                continue;
            }

            if ( timer == 60 || timer == 120 || timer == 180 || timer == 300 )
                ch_printf( ch, "&YYou have %d minute%s left to finish your quest.\r\n", timer / 60,
                           ( timer / 60 ) > 1 ? "s" : "" );
            else if ( timer == 30 || timer == 15 || timer == 10 || timer == 5 || timer == 2
                      || timer == 1 )
                ch_printf( ch, "&RYou only have %d second%s left to finish your quest!\r\n", timer,
                           timer > 1 ? "s" : "" );
        }
        if ( chquest->chaplimit > 0 ) {
            chquest->chaplimit--;
            timer = chquest->chaplimit;

            if ( timer == 0 ) {
                send_to_char( "&RYou have run out of time to finish the chapter of your quest!\r\n",
                              ch );
                send_to_char( "You must start the chapter again.&D\r\n", ch );
                chquest->progress--;
                continue;
            }

            if ( timer == 60 || timer == 120 || timer == 180 || timer == 300 )
                ch_printf( ch,
                           "&YYou have %d minute%s left to finish the chapter of your quest.\r\n",
                           timer / 60, ( timer / 60 ) > 1 ? "s" : "" );
            else if ( timer == 30 || timer == 15 || timer == 10 || timer == 5 || timer == 2
                      || timer == 1 )
                ch_printf( ch,
                           "&RYou only have %d second%s left to finish the chapter of your quest!\r\n",
                           timer, timer > 1 ? "s" : "" );
        }
    }
}

bool IS_GROUPED( CHAR_DATA *ch )
{
    CHAR_DATA              *gch;

    if ( !ch || !ch->in_room )
        return FALSE;

    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
        if ( is_same_group( gch, ch ) && !IS_NPC( gch ) && gch != ch )
            return TRUE;

    return FALSE;
}

void restore_char( CHAR_DATA *ch )
{
    if ( ch->hit < ch->max_hit )
        ch->hit = ch->max_hit;
    if ( ch->mana < ch->max_mana )
        ch->mana = ch->max_mana;
    if ( ch->move < ch->max_move )
        ch->move = ch->max_move;
    if ( ch->race == RACE_DEMON || ch->race == RACE_VAMPIRE ) {
        if ( ch->blood < ch->max_blood )
            ch->blood = ch->max_blood;
    }
    if ( ch->pcdata ) {
        ch->pcdata->condition[COND_BLOODTHIRST] = ( 10 + ch->level );
        ch->pcdata->condition[COND_FULL] = 40;         /* add this if you wish to restore 
                                                        * hunger */
        ch->pcdata->condition[COND_THIRST] = 40;       /* add this if you wish to restore 
                                                        * thirst */
    }
    update_pos( ch );
    send_to_char_color( "&YThe 6D Staff are pleased with your progress, and have restored you!\r\n",
                        ch );
}

int get_totaldamage( CHAR_DATA *ch )
{
    int                     totaldamage;

    totaldamage = 1;

    if ( ch->hit <= 0 )
        return totaldamage;

    /*
     * Have to limit it to ch->level * 99 to keep it under max damage or you get bug
     * messages 
     */
    totaldamage += number_range( ch->hit / 20, ch->hit / 8 );

    if ( totaldamage > 12 )
        totaldamage /= ch->degree;

    if ( totaldamage < 8 )
        totaldamage = number_range( 5, 20 );

    if ( totaldamage > 100 )
        totaldamage -= get_curr_con( ch );

    if ( totaldamage > 30000 )
        totaldamage = 30000;

    return totaldamage;
}

void char_affects( void )
{
    CHAR_DATA              *ch;
    int                     totaldamage;
    short                   chance;
    bool                    stripped;

    for ( ch = last_char; ch; ch = gch_prev ) {
        stripped = FALSE;

        if ( ch == first_char && ch->prev ) {
            bug( "char_affects: first_char->prev != NULL... fixed" );
            ch->prev = NULL;
        }

        gch_prev = ch->prev;
        set_cur_char( ch );
        if ( gch_prev && gch_prev->next != ch ) {
            bug( "char_affects: ch->prev->next != ch" );
            return;
        }

        if ( ch->degree < 1 )
            ch->degree = 3;

        chance = number_range( 1, 10 );
        if ( IS_AFFECTED( ch, AFF_POISON ) && !char_died( ch ) ) {
            if ( IS_NPC( ch ) && chance >= 8 ) {
                affect_strip( ch, gsn_poison );
                xREMOVE_BIT( ch->affected_by, AFF_POISON );
                stripped = TRUE;
            }
            else {
                act( AT_POISON, "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM );
                act( AT_POISON, "You shiver and suffer.", ch, NULL, NULL, TO_CHAR );
                ch->mental_state = URANGE( 1, ch->mental_state + ( IS_NPC( ch ) ? 1 : 2 ), 50 );
                if ( ch->position == POS_SLEEPING )
                    send_to_char( "\r\nYou awaken out of your sleep from the deadly poison!\r\n",
                                  ch );

                totaldamage = get_totaldamage( ch );
                damage( ch, ch, totaldamage, gsn_poison );
            }
        }

        chance = number_range( 1, 10 );
        if ( IS_AFFECTED( ch, AFF_FUNGAL_TOXIN ) && !char_died( ch ) ) {
            if ( IS_NPC( ch ) && chance >= 8 ) {
                affect_strip( ch, gsn_festering_wound );
                xREMOVE_BIT( ch->affected_by, AFF_FUNGAL_TOXIN );
                stripped = TRUE;
            }
            else {
                act( AT_CYAN, "$n appears to be terribly ill and is sweating profusely.", ch, NULL,
                     NULL, TO_ROOM );
                act( AT_RED, "The fungal toxin continues to fester!", ch, NULL, NULL, TO_CHAR );
                if ( ch->position == POS_SLEEPING )
                    send_to_char( "\r\nYou awaken out of your sleep from deadly fungal toxin!\r\n",
                                  ch );

                totaldamage = get_totaldamage( ch );
                damage( ch, ch, totaldamage, gsn_festering_wound );
            }
        }

        chance = number_range( 1, 10 );
        if ( IS_AFFECTED( ch, AFF_MAIM ) && !char_died( ch ) ) {
            if ( IS_NPC( ch ) && chance >= 8 ) {
                affect_strip( ch, gsn_maim );
                xREMOVE_BIT( ch->affected_by, AFF_MAIM );
                stripped = TRUE;
            }
            else {
                act( AT_RED, "$n is slightly limping as $s leg sprays blood into the air.", ch,
                     NULL, NULL, TO_ROOM );
                act( AT_RED, "You are limping tenderly now as your leg sprays blood into the air!",
                     ch, NULL, NULL, TO_CHAR );
                if ( ch->position == POS_SLEEPING )
                    send_to_char
                        ( "\r\nYou awaken out of your sleep from the pain of your wound!\r\n", ch );

                totaldamage = get_totaldamage( ch );
                damage( ch, ch, totaldamage, gsn_maim );
                make_blood( ch );
            }
        }

        chance = number_range( 1, 10 );
        if ( IS_AFFECTED( ch, AFF_THAITIN ) && !char_died( ch ) ) {
            if ( IS_NPC( ch ) && chance >= 8 ) {
                affect_strip( ch, gsn_thaitin );
                xREMOVE_BIT( ch->affected_by, AFF_THAITIN );
                stripped = TRUE;
            }
            else {
                act( AT_YELLOW, "You cringe in pain as your blood burns!", ch, NULL, NULL,
                     TO_CHAR );
                act( AT_YELLOW, "$n cringes in pain as $s blood burns!", ch, NULL, NULL, TO_ROOM );
                if ( ch->position == POS_SLEEPING )
                    send_to_char( "\r\nYou awaken out of your sleep from the burning!\r\n", ch );

                totaldamage = get_totaldamage( ch );
                damage( ch, ch, totaldamage, gsn_thaitin );
            }
        }

        chance = number_range( 1, 10 );
        if ( IS_AFFECTED( ch, AFF_BRITTLE_BONES ) && !char_died( ch ) ) {
            if ( IS_NPC( ch ) && chance >= 8 ) {
                affect_strip( ch, gsn_brittle_bone );
                xREMOVE_BIT( ch->affected_by, AFF_BRITTLE_BONES );
                stripped = TRUE;
            }
            else {
                act( AT_CYAN, "$n appears to be moving in pain.", ch, NULL, NULL, TO_ROOM );
                act( AT_WHITE, "A bone suddenly cracks within your body!", ch, NULL, NULL,
                     TO_CHAR );
                act( AT_RED, "That really hurt!", ch, NULL, NULL, TO_CHAR );
                if ( ch->position == POS_SLEEPING )
                    send_to_char
                        ( "\r\nYou awaken out of your sleep due to the pain in your bones!\r\n",
                          ch );

                totaldamage = get_totaldamage( ch );
                damage( ch, ch, totaldamage, gsn_brittle_bone );
            }
        }

        if ( IS_NPC( ch ) && stripped )
            act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL,
                 NULL, TO_ROOM );
    }
}
