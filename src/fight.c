 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Battle and death module                                               *
 ***************************************************************************/

#include <ctype.h>                                     /* fixes tolower implicit
                                                        * declarations */
#include <string.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/polymorph.h"
#include "h/damage.h"
#include "h/arena.h"
#include "h/city.h"
#include "h/hometowns.h"

extern char             lastplayercmd[MIL];
extern CHAR_DATA       *gch_prev;
void                    add_pkill( CHAR_DATA *ch, CHAR_DATA *victim );
PKILLED_DATA           *has_pkilled( CHAR_DATA *ch, char *name );

#define MIN_DISTANCE 1

//Following will be declared through header once testing is finished. -Taon
extern bool             arena_underway;
extern void             find_arena_winner( CHAR_DATA *ch );

void                    lodge_projectile( CHAR_DATA *ch, CHAR_DATA *victim );

void                    adjust_faith( CHAR_DATA *ch, short adjustment );

OBJ_DATA               *used_weapon;                   /* Used to figure out which weapon 
                                                        * later */

#define NEW_AUTH(ch)         (!IS_NPC(ch) && (ch)->level == 1)

bool                    countpkill( CHAR_DATA *ch, CHAR_DATA *victim );

/*
 * Local functions.
 */
void                    set_distance( CHAR_DATA *ch, int distance );
void group_gain         args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int xp_compute          args( ( CHAR_DATA *gch, CHAR_DATA *victim ) );
int                     align_compute( CHAR_DATA *gch, CHAR_DATA *victim );
ch_ret one_hit          args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
int obj_hitroll         args( ( OBJ_DATA *obj ) );
void show_condition     args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void make_corpse        args( ( CHAR_DATA *ch, CHAR_DATA *killer ) );
void make_blood         args( ( CHAR_DATA *ch ) );
void                    to_channel( const char *argument, const char *xchannel, int level );
void                    increase_clan_pdeaths( CLAN_DATA * clan, int level );
void                    increase_clan_pkills( CLAN_DATA * clan, int level );

/*
 * Check to see if player's attacks are (still?) suppressed
 * #ifdef TRI
 */
bool is_attack_supressed( CHAR_DATA *ch )
{
    TIMER                  *timer;

    if ( IS_NPC( ch ) )
        return FALSE;

    timer = get_timerptr( ch, TIMER_ASUPRESSED );

    if ( !timer )
        return FALSE;

    /*
     * perma-supression -- bard? (can be reset at end of fight, or spell, etc) 
     */
    if ( timer->value == -1 )
        return TRUE;

    /*
     * this is for timed supressions 
     */
    if ( timer->count >= 1 )
        return TRUE;

    return FALSE;
}

/*
 * Check to see if weapon is poisoned.
 */
bool is_wielding_poisoned( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;

    if ( !used_weapon )
        return FALSE;

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL && used_weapon == obj
         && IS_OBJ_STAT( obj, ITEM_POISONED ) )
        return TRUE;
    if ( ( obj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL && used_weapon == obj
         && IS_OBJ_STAT( obj, ITEM_POISONED ) )
        return TRUE;

    return FALSE;
}

/*
 * hunting, hating and fearing code     -Thoric
 */
bool is_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->hunting || ch->hunting->who != victim )
        return FALSE;

    return TRUE;
}

bool is_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->hating || ch->hating->who != victim )
        return FALSE;

    return TRUE;
}

bool is_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->fearing || ch->fearing->who != victim )
        return FALSE;

    return TRUE;
}

void stop_hunting( CHAR_DATA *ch )
{
    if ( ch->hunting ) {
        STRFREE( ch->hunting->name );
        DISPOSE( ch->hunting );
        ch->hunting = NULL;
    }
    return;
}

void stop_hating( CHAR_DATA *ch )
{
    if ( ch->hating ) {
        STRFREE( ch->hating->name );
        DISPOSE( ch->hating );
        ch->hating = NULL;
    }
    return;
}

void stop_fearing( CHAR_DATA *ch )
{
    if ( ch->fearing ) {
        STRFREE( ch->fearing->name );
        DISPOSE( ch->fearing );
        ch->fearing = NULL;
    }
    return;
}

void stop_summoning( CHAR_DATA *ch )
{
    CHAR_DATA              *vch,
                           *vch_next;

    if ( !ch || !ch->in_room )
        return;
    for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
        vch_next = vch->next_in_room;

        if ( vch->hunting && vch->hunting->who == ch )
            stop_hunting( vch );
        if ( vch->hating && vch->hating->who == ch )
            stop_hating( vch );
    }
}

void start_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->hunting )
        stop_hunting( ch );

    CREATE( ch->hunting, HHF_DATA, 1 );
    ch->hunting->name = QUICKLINK( victim->name );
    ch->hunting->who = victim;
    return;
}

void start_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->hating )
        stop_hating( ch );

    CREATE( ch->hating, HHF_DATA, 1 );
    ch->hating->name = QUICKLINK( victim->name );
    ch->hating->who = victim;
    return;
}

void start_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->fearing )
        stop_fearing( ch );

    CREATE( ch->fearing, HHF_DATA, 1 );
    ch->fearing->name = QUICKLINK( victim->name );
    ch->fearing->who = victim;
    return;
}

short DRAGON_AC( CHAR_DATA *ch )
{
    int                     hide;

    if ( ch->race == RACE_DRAGON ) {
        hide = ( int ) ( ch->armor + ( ( ch->level + 20 ) * -2.0 ) );
        return hide;
    }
    else
        return 0;
}

/*
 * Get the current armor class for a vampire based on time of day
 * Though wasnt my Idea, I found it left undone and coded it. -Taon
 */
short VAMP_AC( CHAR_DATA *ch )
{
    short                   armor_value = 0;

    if ( IS_OUTSIDE( ch ) && ( ch->Class == CLASS_VAMPIRE ) ) {
        switch ( time_info.sunlight ) {
            case SUN_LIGHT:
                armor_value = 50;
                break;
            case SUN_RISE:
            case SUN_SET:
                armor_value = 25;
                break;
        }
    }

    if ( armor_value == 0 )                            // Must be dark or inside, small
        // bonus!
        armor_value = -get_curr_dex( ch ) - 10;

    return armor_value;
}

//Currently in use. -Taon

short MONK_AC( CHAR_DATA *ch )
{
    int                     flesh_armor;

    if ( ch->Class == CLASS_MONK )
        flesh_armor = -get_curr_dex( ch );
    else
        flesh_armor = 0;

    return flesh_armor;
}

int max_fight( CHAR_DATA *ch )
{
    return 8;
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 * Many hours spent fixing bugs in here by Thoric, as noted by residual
 * debugging checks.  If you never get any of these error messages again
 * in your logs... then you can comment out some of the checks without
 * worry.
 *
 * Note:  This function also handles some non-violence updates.
 */
void violence_update( void )
{
    CHAR_DATA              *ch;
    CHAR_DATA              *lst_ch;
    CHAR_DATA              *victim;
    CHAR_DATA              *rch,
                           *rch_next;
    AFFECT_DATA            *paf,
                           *paf_next;
    TIMER                  *timer,
                           *timer_next;
    ch_ret                  retcode;
    int                     attacktype,
                            cnt;
    SKILLTYPE              *skill;
    static int              pulse = 0;

    lst_ch = NULL;
    pulse = ( pulse + 1 ) % 100;

    for ( ch = last_char; ch; lst_ch = ch, ch = gch_prev ) {
        set_cur_char( ch );

        if ( ch == first_char && ch->prev ) {
            bug( "%s", "ERROR: first_char->prev != NULL, fixing..." );
            ch->prev = NULL;
        }

        gch_prev = ch->prev;

        if ( gch_prev && gch_prev->next != ch ) {
            bug( "FATAL: violence_update: %s->prev->next doesn't point to ch. Short-cutting here",
                 ch->name );
            ch->prev = NULL;
            gch_prev = NULL;
        }

        /*
         * See if we got a pointer to someone who recently died...
         * if so, either the pointer is bad... or it's a player who
         * "died", and is back at the healer...
         * Since he/she's in the char_list, it's likely to be the later...
         * and should not already be in another fight already
         */
        if ( char_died( ch ) )
            continue;

        /*
         * See if we got a pointer to some bad looking data...
         */
        if ( !ch->in_room || !ch->name ) {
            log_string( "violence_update: bad ch record!  (Shortcutting.)" );
            log_printf( "ch: %ld  ch->in_room: %ld  ch->prev: %ld  ch->next: %ld", ( long ) ch,
                        ( long ) ch->in_room, ( long ) ch->prev, ( long ) ch->next );
            log_string( lastplayercmd );
            if ( lst_ch )
                log_printf( "lst_ch: %ld  lst_ch->prev: %ld  lst_ch->next: %ld", ( long ) lst_ch,
                            ( long ) lst_ch->prev, ( long ) lst_ch->next );
            else
                log_string( "lst_ch: NULL" );
            gch_prev = NULL;
            continue;
        }

        /*
         * Experience gained during battle deceases as battle drags on
         */
        if ( ch->fighting )
            if ( ( ++ch->fighting->duration % 24 ) == 0 )
                ch->fighting->xp = ( ( ch->fighting->xp * 10 ) / 6 );

        if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) && who_fighting( ch->master ) != NULL
             && !xIS_SET( ch->act, ACT_MOUNTABLE ) ) {
            if ( !ch->fighting ) {
                victim = who_fighting( ch->master );
                if ( victim != NULL ) {
                    set_fighting( ch, victim );
                }
            }
        }

        for ( timer = ch->first_timer; timer; timer = timer_next ) {
            timer_next = timer->next;
            if ( --timer->count <= 0 ) {
                if ( timer->type == TIMER_ASUPRESSED ) {
                    if ( timer->value == -1 ) {
                        timer->count = 1000;
                        continue;
                    }
                }
                if ( timer->type == TIMER_NUISANCE ) {
                    DISPOSE( ch->pcdata->nuisance );
                }

                if ( timer->type == TIMER_DO_FUN ) {
                    int                     tempsub;

                    tempsub = ch->substate;
                    ch->substate = timer->value;
                    ( timer->do_fun ) ( ch, ( char * ) "" );
                    if ( char_died( ch ) )
                        break;
                    ch->substate = tempsub;
                }
                extract_timer( ch, timer );
            }
        }

        if ( char_died( ch ) )
            continue;

        /*
         * We need spells that have shorter durations than an hour.
         * So a melee round sounds good to me... -Thoric
         */
        for ( paf = ch->first_affect; paf; paf = paf_next ) {
            paf_next = paf->next;
            if ( paf->duration > 0 )
                paf->duration--;
            else if ( paf->duration < 0 );
            else {
                if ( !paf_next || paf_next->type != paf->type || paf_next->duration > 0 ) {
                    skill = get_skilltype( paf->type );
                    if ( paf->type > 0 && skill && skill->msg_off ) {
                        set_char_color( AT_GREY, ch );
                        send_to_char( skill->msg_off, ch );
                        send_to_char( "\r\n", ch );
                    }
                }
                if ( paf->type == gsn_possess ) {
                    ch->desc->character = ch->desc->original;
                    ch->desc->original = NULL;
                    ch->desc->character->desc = ch->desc;
                    ch->desc->character->switched = NULL;
                    ch->desc = NULL;
                }
                affect_remove( ch, paf );

                if ( paf->type == gsn_taunt && !ch->fighting ) {
                    xREMOVE_BIT( ch->affected_by, AFF_HATE );
                    ch->hate_level = 0;
                }

                if ( paf->type == gsn_enrage && !ch->fighting ) {
                    xREMOVE_BIT( ch->affected_by, AFF_HATE );
                    ch->hate_level = 0;
                }

                if ( paf->type == gsn_human_form ) {
                    humanform_change( ch, FALSE );
                }
            }
        }

        if ( char_died( ch ) )
            continue;

        /*
         * check for exits moving players around 
         */
        if ( ( retcode = pullcheck( ch, pulse ) ) == rCHAR_DIED || char_died( ch ) )
            continue;

        /*
         * Let the battle begin! 
         */
        if ( ( victim = who_fighting( ch ) ) == NULL || IS_AFFECTED( ch, AFF_PARALYSIS ) )
            continue;

        retcode = rNONE;

        if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
            log_printf( "violence_update: %s is attempting to fight %s in a SAFE room.", ch->name,
                        victim->name );
            stop_fighting( ch, TRUE );
        }
        else if ( IS_AWAKE( ch ) && ch->in_room == victim->in_room ) {
            retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
            if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_AUTOGLANCE ) && ch != NULL && victim != NULL
                 && who_fighting( ch ) == victim && !char_died( victim ) && !char_died( ch ) ) {
                show_condition( ch, victim );
            }
        }
        else
            stop_fighting( ch, FALSE );

        if ( char_died( ch ) )
            continue;

        if ( retcode == rCHAR_DIED || ( victim = who_fighting( ch ) ) == NULL )
            continue;

        /*
         *  Mob triggers
         *  -- Added some victim death checks, because it IS possible.. -- Alty
         */
        rprog_rfight_trigger( ch );
        if ( char_died( ch ) || char_died( victim ) )
            continue;
        mprog_hitprcnt_trigger( ch, victim );
        if ( char_died( ch ) || char_died( victim ) )
            continue;
        mprog_fight_trigger( ch, victim );
        if ( char_died( ch ) || char_died( victim ) )
            continue;

        if ( get_eq_char( ch, WEAR_WIELD ) ) {
            oprog_fight_trigger( ch, get_eq_char( ch, WEAR_WIELD ), victim );
            if ( char_died( ch ) )
                continue;
        }
        if ( get_eq_char( ch, WEAR_DUAL_WIELD ) ) {
            oprog_fight_trigger( ch, get_eq_char( ch, WEAR_DUAL_WIELD ), victim );
            if ( char_died( ch ) )
                continue;
        }

        /*
         * NPC special attack flags     -Thoric
         */
        if ( IS_NPC( ch ) ) {
            if ( !xIS_EMPTY( ch->attacks ) ) {
                attacktype = -1;
                if ( 30 + ( ch->level / 4 ) >= number_percent(  ) ) {
                    cnt = 0;
                    for ( ;; ) {
                        if ( cnt++ > 10 ) {
                            attacktype = -1;
                            break;
                        }
                        attacktype = number_range( 7, MAX_ATTACK_TYPE - 1 );
                        if ( xIS_SET( ch->attacks, attacktype ) )
                            break;
                    }
                    switch ( attacktype ) {
                        case ATCK_BASH:
                            do_bash( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_STUN:
                            do_stun( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_EBOLT:
                            do_eldritch_bolt( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_GOUGE:
                            do_gouge( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_FEED:
                            do_feed( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_DRAIN:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_energy_drain( skill_lookup( "energy drain" ), ch->level, ch,
                                                    victim );
                            break;
                        case ATCK_FIREBREATH:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_fire_breath( skill_lookup( "fire breath" ), ch->level, ch,
                                                   victim );
                            break;
                        case ATCK_GSMITE:
                            retcode =
                                spell_greater_smite( skill_lookup( "greater smite" ), ch->level, ch,
                                                     victim );
                            break;
                        case ATCK_FROSTBREATH:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_frost_breath( skill_lookup( "frost breath" ), ch->level, ch,
                                                    victim );
                            break;
                        case ATCK_ACIDBREATH:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_acid_breath( skill_lookup( "acid breath" ), ch->level, ch,
                                                   victim );
                            break;
                        case ATCK_BLIGHTNING:
                            retcode =
                                spell_black_lightning( skill_lookup( "black lightning" ), ch->level,
                                                       ch, victim );
                            break;
                        case ATCK_LTOUCH:
                            retcode =
                                spell_smaug( skill_lookup( "lich touch" ), ch->level, ch, victim );
                            break;
                        case ATCK_INFERNO:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            do_inferno( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_BRAINBOIL:
                            do_brain_boil( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_BLIZZARD:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            do_blizzard( ch, ( char * ) "" );
                            retcode = global_retcode;
                            break;
                        case ATCK_LIGHTNBREATH:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_lightning_breath( skill_lookup( "lightning breath" ),
                                                        ch->level, ch, victim );
                            break;
                        case ATCK_GASBREATH:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_gas_breath( skill_lookup( "gas breath" ), ch->level, ch,
                                                  victim );
                            break;
                        case ATCK_SPIRALBLAST:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_spiral_blast( skill_lookup( "spiral blast" ), ch->level, ch,
                                                    victim );
                            break;
                        case ATCK_POISON:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode = spell_poison( gsn_poison, ch->level, ch, victim );
                            break;
                        case ATCK_NASTYPOISON:
                            /*
                             * retcode = spell_nasty_poison(skill_lookup("nasty poison"), ch->level, ch, victim);
                             */
                            break;
                        case ATCK_GAZE:
                            /*
                             * retcode = spell_gaze(skill_lookup("gaze"), ch->level, ch, victim);
                             */
                            break;
                        case ATCK_BLINDNESS:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode = spell_blindness( gsn_blindness, ch->level, ch, victim );
                            break;
                        case ATCK_CAUSESERIOUS:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_cause_serious( skill_lookup( "cause serious" ), ch->level, ch,
                                                     victim );
                            break;
                        case ATCK_EARTHQUAKE:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_earthquake( skill_lookup( "earthquake" ), ch->level, ch,
                                                  victim );
                            break;
                        case ATCK_CAUSECRITICAL:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_cause_critical( skill_lookup( "cause critical" ), ch->level,
                                                      ch, victim );
                            break;
                        case ATCK_CURSE:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode = spell_curse( skill_lookup( "curse" ), ch->level, ch, victim );
                            break;
                        case ATCK_FLAMESTRIKE:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_flamestrike( skill_lookup( "flamestrike" ), ch->level, ch,
                                                   victim );
                            break;
                        case ATCK_HARM:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode = spell_harm( skill_lookup( "harm" ), ch->level, ch, victim );
                            break;
                        case ATCK_FIREBALL:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_fireball( skill_lookup( "fireball" ), ch->level, ch, victim );
                            break;
                        case ATCK_COLORSPRAY:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_colour_spray( skill_lookup( "colour spray" ), ch->level, ch,
                                                    victim );
                            break;
                        case ATCK_WEAKEN:
                            act( AT_MAGIC, "$n utters a few incantations.", ch, NULL, NULL,
                                 TO_ROOM );
                            retcode =
                                spell_weaken( skill_lookup( "weaken" ), ch->level, ch, victim );
                            break;
                    }
                    if ( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
                        continue;
                }
            }
            /*
             * NPC special defense flags     -Thoric
             */
            if ( !xIS_EMPTY( ch->defenses ) ) {
                attacktype = -1;
                if ( 50 + ( ch->level / 4 ) > number_percent(  ) ) {
                    cnt = 0;
                    for ( ;; ) {
                        if ( cnt++ > 10 ) {
                            attacktype = -1;
                            break;
                        }
                        attacktype = number_range( 2, ( MAX_DEFENSE_TYPE - 1 ) );
                        if ( xIS_SET( ch->defenses, attacktype ) )
                            break;
                    }

                    switch ( attacktype ) {
                        case DFND_MINORHEALING:
                            /*
                             * A few quick checks in the cure ones so that a) less spam and
                             * b) we don't have mobs looking stupider than normal by healing
                             * themselves when they aren't even being hit (although that
                             * doesn't happen TOO often 
                             */
                            if ( ch->hit < ch->max_hit ) {
                                act( AT_MAGIC,
                                     "$n mutters a few incantations...and looks a little better.",
                                     ch, NULL, NULL, TO_ROOM );
                                retcode =
                                    spell_cure_serious( skill_lookup( "cure serious" ), ch->level,
                                                        ch, ch );
                            }
                            break;
                        case DFND_ARCHHEALING:
                            if ( ch->hit < ch->max_hit ) {
                                act( AT_MAGIC, "$n utters the words, 'arch healing'.", ch, NULL,
                                     NULL, TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "arch healing" ), ch->level, ch,
                                                 ch );
                            }
                            break;
                        case DFND_GHEAL:
                            retcode =
                                spell_greater_heal( skill_lookup( "greater heal" ), ch->level, ch,
                                                    ch );
                            break;

                        case DFND_ACOMPANION:
                            do_animal_companion( ch, ( char * ) "" );
                            break;

                        case DFND_LSKELLIE:
                            do_lesser_skeleton( ch, ( char * ) "" );
                            break;

                        case DFND_CELEMENTAL:
                            do_conjure_elemental( ch, ( char * ) "" );
                            break;

                        case DFND_SWARD:
                            do_spirits( ch, ch->name );
                            break;

                        case DFND_DBLESS:
                            retcode =
                                spell_smaug( skill_lookup( "dark blessings" ), ch->level, ch, ch );
                            break;

                        case DFND_SYLVANWIND:
                            if ( ch->hit < ch->max_hit ) {
                                act( AT_GREEN, "$n calls upon the sylvan winds healing breeze.", ch,
                                     NULL, NULL, TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "sylvan wind" ), ch->level, ch, ch );
                            }
                            break;
                        case DFND_HEAL:
                            if ( ch->hit < ch->max_hit ) {
                                act( AT_MAGIC,
                                     "$n mutters a few incantations...and looks much healthier.",
                                     ch, NULL, NULL, TO_ROOM );
                                retcode = spell_smaug( skill_lookup( "heal" ), ch->level, ch, ch );
                            }
                            break;
                        case DFND_DISPELMAGIC:
                            if ( victim->first_affect ) {
                                act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_dispel_magic( skill_lookup( "dispel magic" ), ch->level,
                                                        ch, victim );
                            }
                            break;
                        case DFND_DISPELEVIL:
                            act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                            retcode =
                                spell_dispel_evil( skill_lookup( "dispel evil" ), ch->level, ch,
                                                   victim );
                            break;
                        case DFND_TELEPORT:
                            retcode =
                                spell_teleport( skill_lookup( "teleport" ), ch->level, ch, ch );
                            break;
                        case DFND_BESTOWVITAE:
                            retcode =
                                spell_bestow_vitae( skill_lookup( "bestow vitae" ), ch->level, ch,
                                                    ch );
                            break;

                        case DFND_SHOCKSHIELD:
                            if ( !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) ) {
                                act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "shockshield" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
                        case DFND_VENOMSHIELD:
                            if ( !IS_AFFECTED( ch, AFF_VENOMSHIELD ) ) {
                                act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "venomshield" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
                        case DFND_ACIDMIST:
                            if ( !IS_AFFECTED( ch, AFF_ACIDMIST ) ) {
                                act( AT_MAGIC, "$n utters a few incantations ...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "acidmist" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
                        case DFND_FIRESHIELD:
                            if ( !IS_AFFECTED( ch, AFF_FIRESHIELD ) ) {
                                act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "fireshield" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
                        case DFND_ICESHIELD:
                            if ( !IS_AFFECTED( ch, AFF_ICESHIELD ) ) {
                                act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "iceshield" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
                        case DFND_TRUESIGHT:
                            if ( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
                                retcode = spell_smaug( skill_lookup( "true" ), ch->level, ch, ch );
                            else
                                retcode = rNONE;
                            break;

                        case DFND_SHIELD:
                            if ( !IS_AFFECTED( ch, AFF_SHIELD ) ) {
                                act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL,
                                     TO_ROOM );
                                do_shield( ch, ( char * ) "" );
                            }
                            else
                                retcode = rNONE;
                            break;

                        case DFND_SANCTUARY:
                            if ( !IS_AFFECTED( ch, AFF_SANCTUARY ) ) {
                                act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL,
                                     TO_ROOM );
                                retcode =
                                    spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
                            }
                            else
                                retcode = rNONE;
                            break;
                    }
                    if ( attacktype != -1 && ( retcode == rCHAR_DIED || char_died( ch ) ) )
                        continue;
                }
            }
        }

        /*
         * Fun for the whole family!
         */
        for ( rch = ch->in_room->first_person; rch; rch = rch_next ) {
            rch_next = rch->next_in_room;

            /*
             *   Group Fighting Styles Support:
             *   If ch is tanking
             *   If rch is using a more aggressive style than ch
             *   Then rch is the new tank   -h
             */
            /*
             * &&(is_same_group(ch, rch)) 
             */

            if ( ( !IS_NPC( ch ) && !IS_NPC( rch ) ) && ( rch != ch ) && ( rch->fighting )
                 && ( who_fighting( rch->fighting->who ) == ch )
                 && ( !xIS_SET( rch->fighting->who->act, ACT_AUTONOMOUS ) )
                 && ( rch->style < ch->style ) ) {
                rch->fighting->who->fighting->who = rch;

            }

            if ( IS_AWAKE( rch ) && !rch->fighting ) {
                /*
                 * PC's auto-assist others in their group.
                 */
                if ( !IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) ) {
                    if ( ( ( !IS_NPC( rch ) && rch->desc ) || IS_AFFECTED( rch, AFF_CHARM ) )
                         && is_same_group( ch, rch ) && !is_safe( rch, victim, TRUE ) )
                        multi_hit( rch, victim, TYPE_UNDEFINED );
                    continue;
                }

                /*
                 * NPC's assist NPC's of same type or 12.5% chance regardless.
                 * Volk changed this to only assisting same type.
                 */
                if ( IS_NPC( rch ) && !IS_AFFECTED( rch, AFF_CHARM )
                     && !xIS_SET( rch->act, ACT_NOASSIST ) ) {
                    if ( char_died( ch ) )
                        break;
                    if ( rch->pIndexData == ch->pIndexData ) {
                        CHAR_DATA              *vch;
                        CHAR_DATA              *target;
                        int                     number;

                        target = NULL;
                        number = 0;
                        for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room ) {
                            if ( can_see( rch, vch ) && is_same_group( vch, victim )
                                 && number_range( 0, number ) == 0 ) {
                                if ( vch->mount && vch->mount == rch )
                                    target = NULL;
                                else {
                                    target = vch;
                                    number++;
                                }
                            }
                        }

                        if ( target )
                            multi_hit( rch, target, TYPE_UNDEFINED );
                    }
                }
            }
        }
    }
}

/*
 * Do one group of attacks.
 */
ch_ret multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    int                     chance;
    int                     dual_bonus;
    ch_ret                  retcode;

    /*
     * add timer to pkillers
     * Volk: And support for arena! 
     */
    if ( !IS_NPC( ch ) && !IS_NPC( victim ) && !in_arena( ch ) && !in_arena( victim ) ) {
        if ( xIS_SET( ch->act, PLR_NICE ) )
            return rNONE;
        add_timer( ch, TIMER_RECENTFIGHT, 11, NULL, 0 );
        add_timer( victim, TIMER_RECENTFIGHT, 11, NULL, 0 );
    }

    ch->success_attack = 0;

    if ( is_attack_supressed( ch ) )
        return rNONE;

    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_NOATTACK ) )
        return rNONE;
    /*
     * -- Altrag 
     */
    chance = IS_NPC( ch ) ? 100 : ( LEARNED( ch, gsn_berserk ) * 5 / 2 );
    if ( IS_AFFECTED( ch, AFF_BERSERK ) && number_percent(  ) < chance ) {
        ch->success_attack = number_range( 1, 2 );
        retcode = one_hit( ch, victim, dt );

        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }

    if ( get_eq_char( ch, WEAR_DUAL_WIELD ) ) {
        if ( IS_NPC( ch ) ) {
            chance = number_range( 65, 100 );
        }
        else {
            chance = number_range( 1, 100 );
        }
        if ( get_curr_dex( ch ) > 15 ) {
            chance += 20;
        }
        else if ( get_curr_dex( ch ) > 17 ) {
            chance += 35;
        }
        else if ( get_curr_dex( ch ) > 19 ) {
            chance += 55;
        }

        learn_from_success( ch, gsn_dual_wield );
        ch->success_attack += 1;
        if ( !IS_NPC( ch ) ) {
            if ( ch->pcdata->learned[gsn_second_attack] > 0 && chance > 15 ) {
                ch->success_attack = 2;
            }
            if ( ch->pcdata->learned[gsn_third_attack] > 0 && chance > 25 ) {
                ch->success_attack = 3;
            }
            if ( ch->pcdata->learned[gsn_fourth_attack] > 0 && chance > 25 ) {
                ch->success_attack = 4;
            }
            if ( ch->pcdata->learned[gsn_fifth_attack] > 0 && chance > 25 ) {
                ch->success_attack = 5;
            }
            if ( ch->pcdata->learned[gsn_sixth_attack] > 0 && chance > 35 ) {
                ch->success_attack = 6;
            }
            if ( ch->pcdata->learned[gsn_seventh_attack] > 0 && chance > 35 ) {
                ch->success_attack = 7;
            }
            if ( ch->pcdata->learned[gsn_eighth_attack] > 0 && chance > 45 ) {
                ch->success_attack = 8;
            }
        }

        if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
            if ( ch->success_attack > 2 ) {
                ch->success_attack = ch->success_attack - number_range( 1, 2 );
            }
        }

        if ( IS_NPC( ch ) ) {
            ch->success_attack = ch->numattacks;
            if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
                if ( ch->success_attack > 2 ) {
                    ch->success_attack = ch->success_attack - number_range( 1, 3 );
                }
            }

        }
        if ( ch->success_attack < 2 ) {
            ch->success_attack = 2;
        }
        ch->success_attack = ( ch->success_attack / 2 );
        retcode = one_hit( ch, victim, dt );

        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }
    else
        learn_from_failure( ch, gsn_dual_wield );

/* Start of compressed attacks */

    /*
     * NPC predetermined number of attacks -Thoric 
     */
    if ( IS_NPC( ch ) && ch->numattacks > 0 ) {
        for ( chance = 0; chance < ch->numattacks; chance++ ) {
            ch->success_attack += 1;
        }
        short                   bslow;

        bslow = number_range( 1, 4 );
        if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
            if ( ch->success_attack > 2 && bslow < 3 ) {
                ch->success_attack = ch->success_attack / 2;
            }
            else if ( ch->success_attack > 2 && bslow > 2 ) {
                ch->success_attack = ch->success_attack - number_range( 1, 2 );
            }
        }

        retcode = one_hit( ch, victim, dt );
        return retcode;
    }
    if ( IS_AFFECTED( ch, AFF_BOOST ) || IS_AFFECTED( ch, AFF_SURREAL_SPEED )
         || IS_AFFECTED( ch, AFF_FURY ) )
        chance += 10;
    if ( ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) )
         || IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 10;
    if ( IS_AFFECTED( ch, AFF_BOOST ) || IS_AFFECTED( ch, AFF_SURREAL_SPEED )
         || IS_AFFECTED( ch, AFF_FURY ) )
        chance += 10;
    if ( ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) )
         || IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 10;

    /*
     * Volk added 'autofeed' for vampires 
     */
    if ( !IS_NPC( ch ) && IS_VAMPIRE( ch ) && number_percent(  ) > 80 ) {

        chance = IS_NPC( ch ) ? ch->level : LEARNED( ch, gsn_bite );
        if ( number_percent(  ) < chance ) {
            if ( number_percent(  ) > 80 && LEARNED( ch, gsn_feed ) > 0 )
                do_feed( ch, ( char * ) "" );
            else
                do_bite( ch, ( char * ) "" );
        }
    }

    if ( !IS_NPC( ch ) && IS_DRAGON( ch ) && number_percent(  ) >= 95 ) {
        if ( LEARNED( ch, gsn_bite ) > 0 )
            do_bite( ch, ( char * ) "" );
    }

    /*
     * If fighting blind, should up blindsight gsn - Volk 
     */
    if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_NOSIGHT ) && IS_AFFECTED( ch, AFF_BLINDNESS ) )
        learn_from_success( ch, gsn_nosight );

    /*
     * Check for berserk attack 
     */
    chance = IS_NPC( ch ) ? 100 : ( LEARNED( ch, gsn_berserk ) * 5 / 2 );
    if ( IS_AFFECTED( ch, AFF_BERSERK ) && number_percent(  ) < chance ) {
        ch->success_attack += 1;
    }

    /*
     * Roll for first attack 
     */
    chance = number_range( 1, 20 );
    chance += get_curr_str( ch ) / 5;
    chance += get_curr_dex( ch ) / 5;
    if ( chance < 1 ) {
        chance = 1;
    }
    if ( chance > 15 )
        ch->success_attack = 1;

    /*
     * Roll for second attack 
     */
    chance = IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_second_attack ) );

    if ( get_eq_char( ch, WEAR_DUAL_WIELD ) && !IS_NPC( ch ) ) {
        if ( get_curr_dex( ch ) > 15 ) {
            chance += 20;
        }
        else if ( get_curr_dex( ch ) > 17 ) {
            chance += 35;
        }
        else if ( get_curr_dex( ch ) > 19 ) {
            chance += 55;
        }
    }

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_second_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_second_attack );

    /*
     * Roll for third attack 
     */
    chance = IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_third_attack ) );

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_third_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_third_attack );

    /*
     * Roll for forth attack 
     */
    chance = IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_fourth_attack ) );

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_fourth_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_fourth_attack );

    /*
     * Roll for fifth attack 
     */
    chance = IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_fifth_attack ) );

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_fifth_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_fifth_attack );

    /*
     * Roll for sixth attack 
     */
    chance =
        IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_sixth_attack ) +
                                             ( get_curr_dex( ch ) * 2 ) );

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_sixth_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_sixth_attack );

    /*
     * Roll for seventh attack 
     */
    chance =
        IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_seventh_attack ) +
                                             ( get_curr_dex( ch ) * 2 ) );

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_seventh_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_seventh_attack );

    /*
     * Roll for eighth attack 
     */
    chance =
        IS_NPC( ch ) ? ch->level : ( int ) ( LEARNED( ch, gsn_eighth_attack ) +
                                             ( get_curr_dex( ch ) * 2 ) );

    if ( number_percent(  ) < chance ) {
        learn_from_success( ch, gsn_eighth_attack );
        ch->success_attack += 1;
    }
    else
        learn_from_failure( ch, gsn_eighth_attack );

    if ( get_eq_char( ch, WEAR_DUAL_WIELD ) ) {
        if ( ch->success_attack < 2 ) {
            ch->success_attack = 2;
        }
        ch->success_attack = ( ch->success_attack / 2 );
    }

    if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
        if ( ch->success_attack > 2 ) {
            ch->success_attack = ch->success_attack - number_range( 1, 2 );
        }
    }

    // two-handers
    OBJ_DATA               *obj;

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL ) {
        if ( obj->value[4] == WEP_2H_LONG_BLADE || obj->value[4] == WEP_2H_AXE
             || obj->value[4] == WEP_2H_BLUDGEON ) {
            ch->success_attack /= 2;
            if ( ch->success_attack < 2 ) {
                ch->success_attack = 1;
            }
        }
    }

/* End of compressed attacks */

    if ( ( retcode = one_hit( ch, victim, dt ) ) != rNONE )
        return retcode;

    retcode = rNONE;

    if ( retcode == rNONE ) {
        int                     move = 0;

        if ( !IS_AFFECTED( ch, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLOATING ) )
            move =
                encumbrance( ch,
                             movement_loss[URANGE( 0, ch->in_room->sector_type, SECT_MAX - 1 )] );
        else
            move = encumbrance( ch, 1 );
        if ( ch->move )
            ch->move = UMAX( 0, ch->move - move );
    }
    return retcode;

}

/*
 * Weapon types, haus
 */

int weapon_prof_bonus_check( CHAR_DATA *ch, OBJ_DATA *wield, int *gsn_ptr )
{
    int                     bonus = 0;

    *gsn_ptr = gsn_pugilism;                           /* Change back to -1 if this fails 
                                                        * horribly */

    if ( !IS_NPC( ch ) && wield ) {
        switch ( wield->value[4] ) {
                /*
                 * Restructured weapon system - Samson 11-20-99 
                 */
            default:
                *gsn_ptr = -1;
                break;
            case WEP_2H_LONG_BLADE:
                *gsn_ptr = gsn_2h_long_blades;
                break;
            case WEP_1H_LONG_BLADE:
                *gsn_ptr = gsn_1h_long_blades;
                break;
            case WEP_1H_SHORT_BLADE:
                *gsn_ptr = gsn_1h_short_blades;
                break;
            case WEP_WHIP:
                *gsn_ptr = gsn_whips;
                break;
            case WEP_2H_BLUDGEON:
                *gsn_ptr = gsn_2h_bludgeons;
                break;
            case WEP_LANCE:
                *gsn_ptr = gsn_lances;
                break;
            case WEP_FLAIL:
                *gsn_ptr = gsn_flails;
                break;
            case WEP_TALON:
                *gsn_ptr = gsn_talons;
                break;
            case WEP_1H_BLUDGEON:
                *gsn_ptr = gsn_1h_bludgeons;
                break;
            case WEP_ARCHERY:
                *gsn_ptr = gsn_archery;
                break;
            case WEP_BLOWGUN:
                *gsn_ptr = gsn_blowguns;
                break;
            case WEP_2H_AXE:
                *gsn_ptr = gsn_2h_axes;
                break;
            case WEP_1H_AXE:
                *gsn_ptr = gsn_1h_axes;
                break;
            case WEP_SPEAR:
                *gsn_ptr = gsn_spears;
                break;
            case WEP_STAFF:
                *gsn_ptr = gsn_staves;
                break;
            case WEP_POLEARM:
                *gsn_ptr = gsn_polearms;
                break;
        }
        if ( *gsn_ptr != -1 )
            bonus = ( int ) ( ( LEARNED( ch, *gsn_ptr ) - 50 ) / 10 );
    }

    return bonus;
}

/*
 * Calculate the tohit bonus on the object and return RIS values.
 * -- Altrag
 */
int obj_hitroll( OBJ_DATA *obj )
{
    int                     tohit = 0;
    AFFECT_DATA            *paf;

    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
        if ( paf->location == APPLY_HITROLL )
            tohit += paf->modifier;
    for ( paf = obj->first_affect; paf; paf = paf->next )
        if ( paf->location == APPLY_HITROLL )
            tohit += paf->modifier;
    return tohit;
}

/*
 * Offensive shield level modifier
 */
short off_shld_lvl( CHAR_DATA *ch, CHAR_DATA *victim )
{
    short                   lvl;

    lvl = ch->level / 2;
    if ( number_percent(  ) + ( victim->level - lvl ) <= 100 )
        return lvl;
    else
        return 0;
}

/*
 * Hit one guy once.
 */
ch_ret one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA               *wield;
    int                     victim_ac;
    int                     thac0,
                            thac0_00,
                            thac0_32;
    int                     plusris;
    int                     dam;
    int                     diceroll;
    int                     attacktype,
                            cnt;
    int                     prof_bonus;
    int                     prof_gsn = -1;
    ch_ret                  retcode = rNONE;
    static bool             dual_flip = FALSE;
    int                     bonus;
    int                     feet;
    int                     chance,
                            fw_chance;

    /*
     * int w_index; 
     */

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return rVICT_DIED;

    if ( IS_AFFECTED( victim, AFF_UNHOLY_SPHERE ) ) {
        send_to_char( "You cannot get near their unholy sphere.\r\n", ch );
        return rNONE;
    }

//  chance = number_chance( 1, 10 );

    if ( number_chance( 1, 10 ) == 10 ) {
        if ( IS_AFFECTED( ch, AFF_SLOW ) ) {
            act( AT_GREEN, "You try to attack, but move in a slow motion state.", ch, NULL, NULL,
                 TO_CHAR );
            act( AT_GREEN, "$n tries to attack, but moves in a slow motion state.", ch, NULL, NULL,
                 TO_ROOM );
            return rNONE;
        }
    }

    // Flaming shield support. -Taon
    if ( IS_AFFECTED( victim, AFF_FLAMING_SHIELD ) && number_chance( 1, 15 ) >= 9 ) {

        ch_printf( victim, "&rYour flaming shield strikes %s.&D\r\n", ch->name );

        if ( IS_NPC( victim ) )
            ch_printf( ch, "&rYou're struck with %s's flaming shield..&D\r\n",
                       victim->short_descr );
        else
            ch_printf( ch, "&rYou're struck with %s's flaming shield..&D\r\n", victim->name );

        global_retcode = damage( victim, ch, number_chance( 1, 20 ), gsn_flaming_shield );
    }

    used_weapon = NULL;
    /*
     * Figure out the weapon doing the damage   -Thoric
     * Dual wield support -- switch weapons each attack
     */
    if ( ( wield = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL ) {
        if ( dual_flip == FALSE ) {
            dual_flip = TRUE;
            wield = get_eq_char( ch, WEAR_WIELD );
        }
        else
            dual_flip = FALSE;
    }
    else
        wield = get_eq_char( ch, WEAR_WIELD );

    used_weapon = wield;
/*
  if(wield)
    prof_bonus = weapon_prof_bonus_check(ch, wield, &prof_gsn);
  else
    prof_bonus = 0;
*/

    prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );

    if ( ch->fighting                                  /* make sure fight is already
                                                        * started */
         && dt == TYPE_UNDEFINED && IS_NPC( ch ) && !xIS_EMPTY( ch->attacks ) ) {
        cnt = 0;
        for ( ;; ) {
            attacktype = number_range( 0, ( MAX_ATTACK_TYPE - 1 ) );
            if ( xIS_SET( ch->attacks, attacktype ) )
                break;
            if ( cnt++ > 16 ) {
                attacktype = -1;
                break;
            }
        }
        if ( attacktype == ATCK_BACKSTAB )
            attacktype = -1;
        if ( wield && number_percent(  ) > 25 )
            attacktype = -1;
        if ( !wield && number_percent(  ) > 50 )
            attacktype = -1;

        switch ( attacktype ) {
            default:
                break;

            case ATCK_BITE:
                do_bite( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_CLAWS:
                do_claw( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_TAIL_SWIPE:
                do_tail_swipe( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_STING:
                do_sting( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_PUNCH:
                do_punch( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_KICK:
                do_kick( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_HHANDS:
                do_heavy_hands( ch, ( char * ) "" );
                retcode = global_retcode;
                break;

            case ATCK_TRIP:
                attacktype = 0;
                break;
        }
        if ( attacktype >= 0 )
            return retcode;
    }

    if ( dt == TYPE_UNDEFINED ) {
        dt = TYPE_HIT;
        if ( wield && wield->item_type == ITEM_WEAPON )
            dt += wield->value[3];
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
    thac0 = interpolate( ch->level, thac0_00, thac0_32 ) - GET_HITROLL( ch );
    victim_ac = UMAX( -19, ( int ) ( GET_AC( victim ) / 10 ) );

    /*
     * if you can't see what's coming... 
     */
    if ( wield && !can_see_obj( victim, wield ) )
        victim_ac += 1;
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;

    // Following checks added by -Taon.
    if ( victim->faith >= 10 && !IS_NPC( victim ) ) {
        victim_ac -= victim->pcdata->learned[gsn_faith] / 8;

        if ( number_chance( 0, 10 ) == 1 )
            adjust_faith( victim, -1 );
    }

    if ( victim->race == RACE_DRAGON )
        victim_ac -= 10;

    /*
     * "learning" between combatants.  Takes the intelligence difference,
     * and multiplies by the times killed to make up a learning bonus
     * given to whoever is more intelligent  -Thoric
     * (basically the more intelligent one "learns" the other's fighting style)
     */
    if ( ch->fighting && ch->fighting->who == victim ) {
        short                   times = ch->fighting->timeskilled;

        if ( times ) {
            short                   intdiff = get_curr_int( ch ) - get_curr_int( victim );

            if ( intdiff != 0 )
                victim_ac += ( intdiff * times ) / 10;
        }
    }

    /*
     * Weapon proficiency bonus 
     */
    victim_ac += prof_bonus;
    /*
     * The moment of excitement! 
     */
    diceroll = number_chance( 1, 20 );
    if ( diceroll == 0 || ( diceroll != 19 && diceroll < thac0 - victim_ac ) ) {
        int                     bonus = 10;

        if ( prof_gsn != -1 )
            learn_from_failure( ch, prof_gsn );

        /*
         * Physical hit with no damage (hit natural AC) -Shade 
         */
        if ( !IS_NPC( victim ) && race_table[victim->race] )
            bonus = race_table[victim->race]->ac_plus;
        if ( diceroll < thac0 - victim_ac
             && diceroll >=
             thac0 - ( 10 + ( dex_app[get_curr_dex( ch )].defensive / 10 ) + ( bonus / 10 )
                       && diceroll != 19 && diceroll != 0 ) ) {
            short                   drama;             // Changed from int to short

            // -Taon.
            drama = number_range( 1, 100 );

            if ( victim->Class == CLASS_MAGE ) {
                if ( drama > 50 ) {
                    act( AT_LBLUE, "Your attack bounces off $N's shimmering wall of energy.", ch,
                         NULL, victim, TO_CHAR );
                    act( AT_LBLUE, "Your wall of energy shimmers as it deflects $n's attack.", ch,
                         NULL, victim, TO_VICT );
                    act( AT_LBLUE, "$n's attack bounces off $N's shimmering wall of energy.", ch,
                         NULL, victim, TO_NOTVICT );
                }
                else {
                    act( AT_ORANGE,
                         "Your attack penetrates $N's shimmering wall of energy, but glances off $m.",
                         ch, NULL, victim, TO_CHAR );
                    act( AT_ORANGE,
                         "Your wall of energy fails to deflects $n's attack, but it glances off you.",
                         ch, NULL, victim, TO_VICT );
                    act( AT_ORANGE,
                         "$n's attack penetrates $N's wall of energy, but it glances off $m.", ch,
                         NULL, victim, TO_NOTVICT );
                }
            }
            if ( victim->race == RACE_DRAGON ) {
                if ( drama > 50 ) {
                    act( AT_YELLOW,
                         "Sparks fly as your attack is deflected by $N's thick dragon scales.", ch,
                         NULL, victim, TO_CHAR );
                    act( AT_YELLOW, "Sparks fly as your dragon hide deflects $n's attack.", ch,
                         NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "Sparks fly as $n's attack is deflected by $N's thick dragon scales.", ch,
                         NULL, victim, TO_NOTVICT );
                }
                else {
                    act( AT_ORANGE, "Your attack barely penetrates $N's smooth dragon scales.", ch,
                         NULL, victim, TO_CHAR );
                    act( AT_ORANGE, "$n's attack barely penetrates your smooth dragon scales.", ch,
                         NULL, victim, TO_VICT );
                    act( AT_ORANGE, "$n's attack barely penetrates $N's smooth dragon scales.", ch,
                         NULL, victim, TO_NOTVICT );
                }
            }
            if ( victim->Class != CLASS_MAGE && victim->race != RACE_DRAGON ) {
                if ( drama > 50 ) {
                    act( AT_YELLOW, "Sparks fly as your attack is deflected by $N's armor.", ch,
                         NULL, victim, TO_CHAR );
                    act( AT_YELLOW, "Sparks fly as your armor deflects $n's attack.", ch, NULL,
                         victim, TO_VICT );
                    act( AT_YELLOW, "Sparks fly as $n's attack is deflected by $N's armor.", ch,
                         NULL, victim, TO_NOTVICT );
                }
                else {
                    act( AT_ORANGE,
                         "Your attack gets through $N's armor, but is only superficial in damage.",
                         ch, NULL, victim, TO_CHAR );
                    act( AT_ORANGE,
                         "$n's attack penetrates your armor, but only causes superficial damage.",
                         ch, NULL, victim, TO_VICT );
                    act( AT_ORANGE,
                         "$n's attack penetrates $N's armor, but only causes supericial damage.",
                         ch, NULL, victim, TO_NOTVICT );
                }
            }

            if ( !victim->fighting && victim->in_room == ch->in_room )
                set_fighting( victim, ch );
            tail_chain(  );
            return rNONE;
        }
        else {
            /*
             * Miss. 
             */
            damage( ch, victim, 0, dt );
            tail_chain(  );
            return rNONE;
        }
    }
    /*
     * Hit.
     * Calc damage.
     */

    dam = number_range( ch->barenumdie, ch->baresizedie * ch->barenumdie ) + ch->damplus;

    if ( IS_NPC( ch ) && ( xIS_SET( victim->act, PLR_EXTREME ) ) ) {
        dam *= 2;
    }

    if ( wield ) {                                     /* bare hand dice formula fixed by 
                                                        * Thoric */
        dam += number_range( wield->value[1], wield->value[2] );

        if ( IS_NPC( ch ) && ( xIS_SET( victim->act, PLR_EXTREME ) ) ) {
            dam *= 2;
        }

        if ( CAN_SHARPEN( wield ) && wield->value[6] > 10 ) {
            if ( wield->value[2] > 150 )
                wield->value[2] = set_max_chart( wield->level );
            wield->pIndexData->value[2] = set_max_chart( wield->level );
            if ( wield->value[1] > 80 )
                wield->pIndexData->value[1] = set_min_chart( wield->level );
            wield->value[1] = set_min_chart( wield->level );
            if ( wield->value[4] == 0 || wield->value[4] == 4 || wield->value[4] == 8 ) {
                short                   bonus;

                bonus = wield->level / 5;
                if ( bonus < 1 ) {
                    bonus = 1;
                }
                wield->value[1] =
                    set_min_chart( wield->level ) + bonus * 3 + set_min_chart( wield->level / 2 );
                wield->value[2] =
                    set_max_chart( wield->level ) + bonus + ( set_max_chart( wield->level ) / 2 );
                wield->pIndexData->value[1] =
                    set_min_chart( wield->level ) + bonus * 3 + set_min_chart( wield->level / 2 );
                wield->pIndexData->value[2] =
                    set_max_chart( wield->level ) + bonus + ( set_max_chart( wield->level ) / 2 );
            }
            wield->value[6] = 0;
            wield->pIndexData->value[6] = 0;

/*
      if(number_chance(1, 20) == 1) 
      {
       
        ch_printf(ch, "&wAs you attack, a small nick forms in %s, it will need to be sharpened.&D\r\n", wield->short_descr);
        wield->value[6]--;
        update_weapon(ch, wield);
      }

      else if(number_range(1, 100) == 1) 
      {
        ch_printf(ch, "&wAs you attack, a crack forms in %s it will need to be sharpened!&D\r\n", wield->short_descr);
        wield->value[6] -= 1;
        update_weapon(ch, wield);
      }
      if(wield->value[6] < 25)
        wield->value[6] = 25;
*/
        }
    }

/* 
 * Critical Hit Snippet
 *   By Josh Jenks [ Haelyn ]
 */

    chance = number_range( 1, 100 );

    if ( chance > 99 ) {
        act( AT_FIRE, "You attempt a critical strike on $N!", ch, NULL, victim, TO_CHAR );
        act( AT_FIRE, "$n attempts a critical strike on you!", ch, NULL, victim, TO_VICT );
        act( AT_FIRE, "$n attempts a critical strike on $N!", ch, NULL, victim, TO_NOTVICT );
        dam += ( int ) ( get_curr_str( ch ) * 2 );
    }

    if ( ch->race == RACE_CENTAUR && chance < 26 ) {
        act( AT_ORANGE, "Your hooves trample $N!", ch, NULL, victim, TO_CHAR );
        act( AT_ORANGE, "$n's hooves trample you!", ch, NULL, victim, TO_VICT );
        act( AT_ORANGE, "$n's hooves trample $N!", ch, NULL, victim, TO_NOTVICT );
        dam += ( int ) ( get_curr_str( ch ) * 2 );
    }

    /*
     * Bonuses.
     */
    // two-handers
    OBJ_DATA               *obj;

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL ) {
        if ( obj->value[4] == WEP_2H_LONG_BLADE || obj->value[4] == WEP_2H_AXE
             || obj->value[4] == WEP_2H_BLUDGEON ) {
            dam *= 4;
        }
    }

    /*
     * Blind people shouldnt hit as hard, or as often. -Taon 
     */
    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) && !IS_AFFECTED( ch, AFF_NOSIGHT ) )
        dam = ( int ) ( dam - ch->level / 2 );

    // Those whom are boosted are penalized a little damage. -Taon
    if ( IS_AFFECTED( ch, AFF_BOOST ) )
        dam = ( int ) ( dam - number_chance( 1, 10 ) );

    // focus support -Taon
    if ( ch->focus_level > 0 ) {
        // Lets try to keep focus from going away too fast. -Taon
        if ( number_chance( 1, 10 ) > 9 )
            adjust_focus( ch, -2 );
// Damage bonus changed to number_range to attempt to balance monk hitting power out.
        if ( ch->focus_level <= 20 )
            dam += number_range( 1, 2 );
        else if ( ch->focus_level < 30 )
            dam += number_range( 1, 4 );
        else if ( ch->focus_level < 40 )
            dam += number_range( 2, 4 );
        else
            dam += 3;
    }

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_martial_arts] > 0
         && get_eq_char( ch, WEAR_WIELD ) == NULL && get_eq_char( ch, WEAR_DUAL_WIELD ) == NULL ) {
        short                   martial = 0;

        if ( ch->level <= 4 )
            martial += number_chance( 1, 5 );
        else if ( ch->level <= 9 )
            martial += number_chance( 3, 8 );
        else if ( ch->level <= 19 )
            martial += number_chance( 3, 8 );
        else if ( ch->level <= 25 )
            martial += number_chance( 4, 8 );
        else if ( ch->level <= 34 )
            martial += number_chance( 4, 8 );
        else if ( ch->level <= 44 )
            martial += number_chance( 5, 8 );
        else if ( ch->level <= 54 )
            martial += number_chance( 6, 8 );
        else if ( ch->level <= 64 )
            martial += number_chance( 7, 9 );
        else if ( ch->level <= 74 )
            martial += number_chance( 8, 9 );
        else if ( ch->level <= 84 )
            martial += number_chance( 9, 10 );
        else if ( ch->level <= 99 )
            martial += number_chance( 9, 10 );
        else
            martial += 10;

        if ( ch->pcdata->learned[gsn_martial_arts] <= 10 )
            dam += martial / 2;
        else if ( ch->pcdata->learned[gsn_martial_arts] <= 20 )
            dam += martial / 2;
        else if ( ch->pcdata->learned[gsn_martial_arts] <= 30 )
            dam += martial / 2;
        else if ( ch->pcdata->learned[gsn_martial_arts] <= 40 )
            dam += martial + 2;
        else if ( ch->pcdata->learned[gsn_martial_arts] <= 50 )
            dam += martial + 4;
        else if ( ch->pcdata->learned[gsn_martial_arts] <= 70 )
            dam += martial + 5;
        else if ( ch->pcdata->learned[gsn_martial_arts] <= 90 )
            dam += martial + 6;
        else
            dam += martial + 7;

        if ( number_chance( 0, 5 ) == 1 )
            learn_from_success( ch, gsn_martial_arts );
    }

    if ( ch->race == RACE_DRAGON && !IS_NPC( ch ) ) {
        feet = ch->level;
        if ( feet < 2 ) {
            feet = 1;
        }
        dam += ( GET_DAMROLL( ch ) + feet );
    }
    dam += GET_DAMROLL( ch );

    if ( ch->position == POS_CRAWL ) {
        dam /= 2;
    }

    if ( prof_bonus )
        dam += prof_bonus / 4;

    if ( victim->position == POS_BERSERK )
        dam = ( int ) ( dam * 1.8 );
    if ( ch->position == POS_BERSERK )
        dam = ( int ) ( dam * 2.0 );

    // Some fury, sidestep support. -Taon
    if ( IS_AFFECTED( victim, AFF_FURY ) )
        dam = ( int ) ( dam + number_range( 1, 4 ) );
    if ( IS_AFFECTED( ch, AFF_FURY ) )
        dam = ( int ) ( dam + number_range( 1, 4 ) );
    if ( IS_AFFECTED( victim, AFF_SIDESTEP ) )
        dam = ( int ) ( dam + number_range( 1, 4 ) );

    // Following supports autoskill combat mind. -Taon
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_combat_mind] > 0 && number_chance( 0, 10 ) > 8 ) {

        if ( ch->pcdata->learned[gsn_combat_mind] < 30 )
            dam += ( int ) ( dam + number_range( 1, 2 ) );
        else if ( ch->pcdata->learned[gsn_combat_mind] < 60 )
            dam += ( int ) ( dam + number_range( 1, 3 ) );
        else if ( ch->pcdata->learned[gsn_combat_mind] < 85 )
            dam += ( int ) ( dam + number_range( 1, 4 ) );
        else
            dam += ( int ) ( dam + number_range( 2, 4 ) );

        learn_from_success( ch, gsn_combat_mind );
    }

    // Following supports autoskill find weakness. -Taon
    fw_chance = number_chance( 1, 10 );

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_find_weakness] > 0 && fw_chance > 9 ) {
        send_to_char( "&WYou find a weakness in your foe's armor.&D\r\n", ch );
        dam += ( int ) ( get_curr_str( ch ) * 2 );
        ch->move -= 2;
        learn_from_success( ch, gsn_find_weakness );
    }

    // Masters eye autoskill, near mock of enhanced damage but stronger -Taon
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_masters_eye] > 0 ) {
        dam += ( int ) ( dam + number_range( 1, 4 ) );
        learn_from_success( ch, gsn_masters_eye );
    }

    if ( IS_NPC( ch ) && ( xIS_SET( victim->act, PLR_EXTREME ) ) ) {
        dam *= 2;
    }

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_enhanced_damage] > 0 ) {
        dam += ( int ) ( dam * LEARNED( ch, gsn_enhanced_damage ) / 90 );
        ch->move -= 1;
        learn_from_success( ch, gsn_enhanced_damage );
    }

//dam bonus for those with battle trance skill, also
//also made more use of the AFF_FURY bit here. -Taon

    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_battle_trance] > 0 ) {
        if ( ch->Class == CLASS_BALROG )
            dam += number_range( 1, 25 );
        else
            dam += number_range( 1, 20 );
        ch->move -= 3;
        learn_from_success( ch, gsn_battle_trance );
    }

    // Added bonus damage for targets whom are sitting or resting. -Taon
    if ( victim->position == POS_SITTING || victim->position == POS_RESTING )
        dam = ( int ) ( dam * 1.1 );
    if ( !IS_AWAKE( victim ) )
        dam *= 2;
    if ( dt == gsn_backstab )
        dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 10 );
    if ( dt == gsn_circle )
        dam *= ( 2 + URANGE( 2, ch->level - ( victim->level / 4 ), 30 ) / 16 );

    if ( dam <= 0 )
        dam = 1;

    plusris = 0;

    if ( wield ) {
        if ( IS_OBJ_STAT( wield, ITEM_MAGIC ) )
            dam = ris_damage( victim, dam, RIS_MAGIC );
        else
            dam = ris_damage( victim, dam, RIS_NONMAGIC );

        /*
         * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll  -Thoric
         */
        plusris = obj_hitroll( wield );
    }
    else
        dam = ris_damage( victim, dam, RIS_NONMAGIC );

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
            mod -= 3;

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

    if ( prof_gsn != -1 ) {
        if ( dam > 0 )
            learn_from_success( ch, prof_gsn );
        else
            learn_from_failure( ch, prof_gsn );
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
            if ( found )
                return rNONE;
        }
        dam = 0;
    }

    if ( IS_NPC( victim ) && xIS_SET( victim->act, ACT_IMMORTAL ) ) {
        victim->hit += dam;
        if ( number_bits( 4 ) == 0 ) {
            act( AT_RED, "$N seems unaffected by your attacks.", ch, NULL, victim, TO_CHAR );
            act( AT_RED, "$N seems unaffected by $n's attacks.", ch, NULL, victim, TO_ROOM );
        }
    }

    if ( IS_AFFECTED( victim, AFF_INVISIBLE ) ) {
        affect_strip( victim, gsn_invis );
        xREMOVE_BIT( victim->affected_by, AFF_INVISIBLE );
    }

    if ( IS_AFFECTED( victim, AFF_KINETIC ) ) {
        if ( victim->kinetic_dam > dam ) {
            act( AT_YELLOW, "Your attack strikes $N's kinetic barrier.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_YELLOW, "$n's attack strikes $N's kinetic barrier.", ch, NULL, victim,
                 TO_ROOM );
            victim->kinetic_dam -= dam;
            dam = 0;
        }
        else {
            act( AT_YELLOW, "Your attack has shattered $N's kinetic barrier!", ch, NULL, victim,
                 TO_CHAR );
            act( AT_YELLOW, "$n's attack has shattered $N's kinetic barrier!", ch, NULL, victim,
                 TO_ROOM );
            affect_strip( victim, gsn_kinetic_barrier );
            xREMOVE_BIT( victim->affected_by, AFF_KINETIC );
        }
    }

    if ( IS_AFFECTED( victim, AFF_WARD ) ) {
        if ( victim->ward_dam > dam ) {
            act( AT_YELLOW, "Your attack strikes $N's protective ward.", ch, NULL, victim,
                 TO_CHAR );
            act( AT_YELLOW, "$n's attack strikes $N's protective ward.", ch, NULL, victim,
                 TO_ROOM );
            victim->ward_dam -= dam;
            dam = 0;
        }
        else {
            act( AT_YELLOW, "Your attack has shattered $N's protective ward!", ch, NULL, victim,
                 TO_CHAR );
            act( AT_YELLOW, "$n's attack has shattered $N's protective ward!", ch, NULL, victim,
                 TO_ROOM );
            affect_strip( victim, gsn_spirits_ward );
            affect_strip( victim, gsn_spectral_ward );
            xREMOVE_BIT( victim->affected_by, AFF_WARD );
        }
    }

    if ( ( retcode = damage( ch, victim, dam, dt ) ) != rNONE )
        return retcode;
    if ( char_died( ch ) )
        return rCHAR_DIED;
    if ( char_died( victim ) )
        return rVICT_DIED;

    retcode = rNONE;
    if ( dam == 0 )
        return retcode;

    /*
     * Weapon spell support     -Thoric
     * Each successful hit casts a spell
     */
    if ( wield && !IS_SET( victim->immune, RIS_MAGIC )
         && !IS_SET( victim->in_room->room_flags, ROOM_NO_MAGIC ) ) {
        AFFECT_DATA            *aff;

        for ( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
            if ( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier )
                 && skill_table[aff->modifier]->spell_fun )
                retcode =
                    ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier,
                                                                 ( wield->level + 3 ) / 3, ch,
                                                                 victim );
        if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
            return retcode;
        for ( aff = wield->first_affect; aff; aff = aff->next )
            if ( aff->location == APPLY_WEAPONSPELL && IS_VALID_SN( aff->modifier )
                 && skill_table[aff->modifier]->spell_fun )
                retcode =
                    ( *skill_table[aff->modifier]->spell_fun ) ( aff->modifier,
                                                                 ( wield->level + 3 ) / 3, ch,
                                                                 victim );
        if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
            return retcode;
    }

    /*
     * magic shields that retaliate     -Thoric
     */
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD ) && !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
        retcode = spell_smaug( skill_lookup( "flare" ), off_shld_lvl( victim, ch ), victim, ch );
    if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
        return retcode;

    if ( IS_AFFECTED( victim, AFF_NETTLE ) && !IS_AFFECTED( ch, AFF_NETTLE ) )
        retcode = spell_barbs( skill_lookup( "barbs" ), off_shld_lvl( victim, ch ), victim, ch );
    if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
        return retcode;

    if ( IS_AFFECTED( victim, AFF_ICESHIELD ) && !IS_AFFECTED( ch, AFF_ICESHIELD ) )
        retcode =
            spell_iceshard( skill_lookup( "iceshard" ), off_shld_lvl( victim, ch ), victim, ch );
    if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
        return retcode;

    if ( IS_AFFECTED( victim, AFF_SHOCKSHIELD ) && !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
        retcode = spell_smaug( skill_lookup( "torrent" ), off_shld_lvl( victim, ch ), victim, ch );
    if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
        return retcode;

    if ( IS_AFFECTED( victim, AFF_ACIDMIST ) && !IS_AFFECTED( ch, AFF_ACIDMIST ) )
        retcode = spell_smaug( skill_lookup( "acidshot" ), off_shld_lvl( victim, ch ), victim, ch );
    if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
        return retcode;

    if ( IS_AFFECTED( victim, AFF_VENOMSHIELD ) && !IS_AFFECTED( ch, AFF_VENOMSHIELD ) )
        retcode =
            spell_smaug( skill_lookup( "venomshot" ), off_shld_lvl( victim, ch ), victim, ch );
    if ( retcode != rNONE || char_died( ch ) || char_died( victim ) )
        return retcode;

    tail_chain(  );
    return retcode;
}

/*
 * Calculate damage based on resistances, immunities and suceptibilities
 *     -Thoric
 */
short ris_damage( CHAR_DATA *ch, short dam, int ris )
{
    short                   modifier;

    if ( IS_SET( ch->immune, ris ) && !IS_SET( ch->no_immune, ris ) )
        return -1;

    /*
     * Change it based on resistance 
     */
    if ( IS_SET( ch->resistant, ris ) && !IS_SET( ch->no_resistant, ris ) ) {
        modifier = 0;
        if ( dam > 0 )
            modifier = ( dam / 10 );
        dam -= modifier;
    }

    /*
     * Increase if susceptible 
     */
    if ( IS_SET( ch->susceptible, ris ) && !IS_SET( ch->no_susceptible, ris ) ) {
        modifier = 0;
        if ( dam > 0 )
            modifier = ( dam / 10 );
        dam += modifier;
    }

    if ( IS_AFFECTED( ch, AFF_UNSEARING_SKIN ) && ris == RIS_FIRE );
    {
        modifier = 0;
        if ( dam > 0 )
            modifier = ( dam / 5 );
        dam -= modifier;
    }

    if ( dam <= 0 )
        return -1;
    return dam;
}

/* Inflict damage from a hit.   This is one damn big function. */
void                    new_dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, unsigned int dt,
                                         OBJ_DATA *obj );

ch_ret damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    OBJ_DATA               *damobj;
    OBJ_DATA               *pObj;
    CHAR_DATA              *gch;
    char                    buf[MSL],
                            buf1[MSL];
    short                   dameq,
                            maxdam,
                            dampmod;
    short                   anopc = 0,
        bnopc = 0;
    bool                    npcvict,
                            loot;
    ch_ret                  retcode = rNONE;

    if ( !ch ) {
        bug( "%s", "Damage: null ch!" );
        return rERROR;
    }
    if ( !victim ) {
        bug( "%s", "Damage: null victim!" );
        return rVICT_DIED;
    }

    if ( victim->position == POS_DEAD )
        return rVICT_DIED;

    if ( IS_AFFECTED( victim, AFF_FEIGN ) )
        return rVICT_DIED;

    if ( IS_AFFECTED( ch, AFF_FASCINATE ) )
        return rNONE;

    npcvict = IS_NPC( victim );

    /*
     * Check damage types for RIS     -Thoric
     */
    if ( dam && dt != TYPE_UNDEFINED ) {
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
        else if ( IS_DRAIN( dt ) )
            dam = ris_damage( victim, dam, RIS_DRAIN );
        else if ( dt == gsn_poison || IS_POISON( dt ) )
            dam = ris_damage( victim, dam, RIS_POISON );
        else if ( dt == ( TYPE_HIT + DAM_CRUSH ) ) {
            /*
             * Added checks for the 3 new dam types, and removed DAM_PEA - Grimm 
             * Removed excess duplication, added hack and lash RIS types - Samson 1-9-00 
             */
            dam = ris_damage( victim, dam, RIS_BLUNT );
        }
        else if ( dt == ( TYPE_HIT + DAM_STAB ) || dt == ( TYPE_HIT + DAM_PIERCE )
                  || dt == ( TYPE_HIT + DAM_THRUST ) )
            dam = ris_damage( victim, dam, RIS_PIERCE );
        else if ( dt == ( TYPE_HIT + DAM_SLASH ) )
            dam = ris_damage( victim, dam, RIS_SLASH );
        else if ( dt == ( TYPE_HIT + DAM_HACK ) )
            dam = ris_damage( victim, dam, RIS_HACK );
        else if ( dt == ( TYPE_HIT + DAM_LASH ) )
            dam = ris_damage( victim, dam, RIS_LASH );

        if ( dam == -1 ) {
            if ( dt >= 0 && dt < top_sn ) {
                bool                    found = FALSE;
                SKILLTYPE              *skill = skill_table[dt];

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
                if ( found )
                    return rNONE;
            }
            dam = 0;
        }
    }

    /*
     * Precautionary step mainly to prevent people in Hell from finding
     * a way out. --Shaddai
     */
    if ( IS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
        dam = 0;

    if ( dam && npcvict && ch != victim ) {
        if ( !xIS_SET( victim->act, ACT_SENTINEL ) ) {
            if ( victim->hunting ) {
                if ( victim->hunting->who != ch ) {
                    STRFREE( victim->hunting->name );
                    victim->hunting->name = QUICKLINK( ch->name );
                    victim->hunting->who = ch;
                }
            }
            else if ( !xIS_SET( victim->act, ACT_PACIFIST ) )   /* Gorog */
                start_hunting( victim, ch );
        }

        if ( victim->hating ) {
            if ( victim->hating->who != ch ) {
                STRFREE( victim->hating->name );
                victim->hating->name = QUICKLINK( ch->name );
                victim->hating->who = ch;
            }
        }
        else if ( !xIS_SET( victim->act, ACT_PACIFIST ) )   /* Gorog */
            start_hating( victim, ch );
    }

    /*
     * Stop up any residual loopholes.
     */
    if ( dt == gsn_backstab )
        maxdam = ch->level * 100;
    else
        maxdam = ch->level * 100;

    if ( dam > maxdam && IS_NPC( victim ) ) {
//    bug("Damage: %d more than %d points!", dam, maxdam);
//    bug("** %s (lvl %d) -> %s **", ch->name, ch->level, victim->name);
        dam = maxdam;
    }

    if ( victim != ch ) {
        /*
         * Lets change the hate_level if need be 
         * Not sure if we should change it up and down but shrug 
         * Feel free to modify it how you want, this is just mainly testing for now 
         * Depending on how much damage is normaly done on how many you might need to
         * give Different spots for I just did up to 50 for now 
         */
        if ( !IS_AFFECTED( ch, AFF_HATE ) ) {
            if ( dam > 0 ) {
                if ( dam <= 10 )
                    ch->hate_level = 0;
                else if ( dam <= 20 )
                    ch->hate_level = 1;
                else if ( dam <= 30 )
                    ch->hate_level = 2;
                else if ( dam <= 40 )
                    ch->hate_level = 3;
                else if ( dam <= 50 )
                    ch->hate_level = 4;
                else if ( dam <= 60 )
                    ch->hate_level = 5;
                else if ( dam <= 70 )
                    ch->hate_level = 6;
                else if ( dam <= 80 )
                    ch->hate_level = 7;
                else if ( dam <= 90 )
                    ch->hate_level = 8;
                else if ( dam <= 100 )
                    ch->hate_level = 9;
                else if ( dam <= 150 )
                    ch->hate_level = 10;
                else if ( dam <= 200 )
                    ch->hate_level = 11;
                else if ( dam <= 250 )
                    ch->hate_level = 12;
                else if ( dam <= 300 )
                    ch->hate_level = 13;
                else if ( dam <= 350 )
                    ch->hate_level = 14;
                else if ( dam <= 400 )
                    ch->hate_level = 15;
                else if ( dam <= 450 )
                    ch->hate_level = 16;
                else if ( dam <= 500 )
                    ch->hate_level = 17;
                else if ( dam <= 550 )
                    ch->hate_level = 18;
                else if ( dam <= 600 )
                    ch->hate_level = 19;
                else if ( dam <= 650 )
                    ch->hate_level = 20;
                else if ( dam <= 700 )
                    ch->hate_level = 21;
                else if ( dam <= 800 )
                    ch->hate_level = 22;
                else if ( dam <= 850 )
                    ch->hate_level = 23;
                else if ( dam <= 875 )
                    ch->hate_level = 24;
                else if ( dam <= 900 )
                    ch->hate_level = 25;
                else if ( dam <= 950 )
                    ch->hate_level = 26;
                else if ( dam <= 975 )
                    ch->hate_level = 27;
                else if ( dam <= 900 )
                    ch->hate_level = 28;
                else if ( dam <= 950 )
                    ch->hate_level = 29;
                else if ( dam <= 1000 )
                    ch->hate_level = 30;
                else if ( dam <= 1100 )
                    ch->hate_level = 31;
                else if ( dam <= 1200 )
                    ch->hate_level = 32;
                else if ( dam <= 1300 )
                    ch->hate_level = 33;
                else if ( dam <= 1400 )
                    ch->hate_level = 34;
                else if ( dam <= 1500 )
                    ch->hate_level = 35;
                else if ( dam <= 1600 )
                    ch->hate_level = 36;
                else if ( dam <= 1800 )
                    ch->hate_level = 37;
                else if ( dam <= 1900 )
                    ch->hate_level = 38;
                else if ( dam <= 2000 )
                    ch->hate_level = 39;
                else if ( dam <= 2100 )
                    ch->hate_level = 40;
                else if ( dam <= 2200 )
                    ch->hate_level = 41;
                else if ( dam <= 2300 )
                    ch->hate_level = 42;
                else                                   /* Shrug might as well have a
                                                        * default for anything else */
                    ch->hate_level = 43;
            }
        }

        /*
         * Certain attacks are forbidden.
         * Most other attacks are returned.
         */
        if ( is_safe( ch, victim, TRUE ) )
            return rNONE;

/* So you're safe. Or are you? Still need to check peacefuls/deadlies, and config +nice - volk */
/* On second thought, HTF to check if they HAVEN'T used murder command? need a bool-in-function.
   Or even better, let's move this mess to the ch_ret spell_smaug func where this happened in the
   first place! */

        check_attacker( ch, victim );

        if ( victim->position > POS_STUNNED ) {
            if ( !victim->fighting && victim->in_room == ch->in_room )
                set_fighting( victim, ch );

            /*
             * vwas: victim->position = POS_FIGHTING; 
             */
            if ( IS_NPC( victim ) && victim->fighting )
                set_position( victim, POS_FIGHTING );
            else if ( victim->fighting ) {
                switch ( victim->style ) {
                    case ( STYLE_EVASIVE ):
                        set_position( victim, POS_EVASIVE );
                        break;
                    case ( STYLE_DEFENSIVE ):
                        set_position( victim, POS_DEFENSIVE );
                        break;
                    case ( STYLE_AGGRESSIVE ):
                        set_position( victim, POS_AGGRESSIVE );
                        break;
                    case ( STYLE_BERSERK ):
                        set_position( victim, POS_BERSERK );
                        break;
                    default:
                        set_position( victim, POS_FIGHTING );
                }
            }
        }

        if ( victim->position > POS_STUNNED ) {
            if ( !ch->fighting && victim->in_room == ch->in_room )
                set_fighting( ch, victim );

            /*
             * If victim is charmed, ch might attack victim's master.
             */
            if ( IS_NPC( ch ) && npcvict && IS_AFFECTED( victim, AFF_CHARM ) && victim->master
                 && victim->master->in_room == ch->in_room && number_bits( 3 ) == 0 ) {
                stop_fighting( ch, FALSE );
                retcode = multi_hit( ch, victim->master, TYPE_UNDEFINED );
                return retcode;
            }
        }

        /*
         * More charm stuff.
         */
        if ( victim->master == ch )
            stop_follower( victim );

        /*
         * Pkill stuff.  If a deadly attacks another deadly or is attacked by
         * one, then ungroup any nondealies.  Disabled untill I can figure out
         * the right way to do it.
         */

        /*
         * count the # of non-pkill pc in a(not including == ch) 
         */
        for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
            if ( is_same_group( ch, gch ) && !IS_NPC( gch ) && !IS_PKILL( gch ) && ( ch != gch ) )
                anopc++;

        /*
         * count the # of non-ekill pc in b(not including == victim) 
         */
        for ( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
            if ( is_same_group( victim, gch ) && !IS_NPC( gch ) && !IS_PKILL( gch )
                 && ( victim != gch ) )
                bnopc++;

        /*
         * only consider disbanding if both groups have 1(+) non-pk pc,
         * or when one participant is pc, and the other group has 1(+)
         * pk pc's (in the case that participant is only pk pc in group) 
         */
        if ( ( bnopc > 0 && anopc > 0 ) || ( bnopc > 0 && !IS_NPC( ch ) )
             || ( anopc > 0 && !IS_NPC( victim ) ) ) {
            /*
             * Disband from same group first 
             */
            if ( is_same_group( ch, victim ) ) {
                /*
                 * Messages to char and master handled in stop_follower 
                 */
                act( AT_ACTION, "$n disbands from $N's group.",
                     ( ch->leader == victim ) ? victim : ch, NULL,
                     ( ch->leader == victim ) ? victim->master : ch->master, TO_NOTVICT );
                if ( ch->leader == victim )
                    stop_follower( victim );
                else
                    stop_follower( ch );
            }
            /*
             * if leader isnt pkill, leave the group and disband ch 
             */
            if ( ch->leader != NULL && !IS_NPC( ch->leader ) && !IS_PKILL( ch->leader ) ) {
                act( AT_ACTION, "$n disbands from $N's group.", ch, NULL, ch->master, TO_NOTVICT );
                stop_follower( ch );
            }
            else {
                for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
                    if ( is_same_group( gch, ch ) && !IS_NPC( gch ) && !IS_PKILL( gch )
                         && gch != ch ) {
                        act( AT_ACTION, "$n disbands from $N's group.", ch, NULL, gch->master,
                             TO_NOTVICT );
                        stop_follower( gch );
                    }
            }
            /*
             * if leader isnt pkill, leave the group and disband victim 
             */
            if ( victim->leader != NULL && !IS_NPC( victim->leader )
                 && !IS_PKILL( victim->leader ) ) {
                act( AT_ACTION, "$n disbands from $N's group.", victim, NULL, victim->master,
                     TO_NOTVICT );
                stop_follower( victim );
            }
            else {
                for ( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
                    if ( is_same_group( gch, victim ) && !IS_NPC( gch ) && !IS_PKILL( gch )
                         && gch != victim ) {
                        act( AT_ACTION, "$n disbands from $N's group.", gch, NULL, gch->master,
                             TO_NOTVICT );
                        stop_follower( gch );
                    }
            }
        }

        /*
         * Inviso attacks ... not.
         */
        if ( IS_AFFECTED( ch, AFF_INVISIBLE ) ) {
            affect_strip( ch, gsn_invis );
            affect_strip( ch, gsn_mass_invis );
            xREMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
            act( AT_MAGIC, "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
        }

        /*
         * Take away Hide 
         */
        if ( IS_AFFECTED( ch, AFF_HIDE ) )
            xREMOVE_BIT( ch->affected_by, AFF_HIDE );

        /*
         * Damage modifiers.
         */

        if ( IS_NPC( ch ) && ( xIS_SET( victim->act, PLR_EXTREME ) ) ) {
            dam *= 2;
        }

        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && IS_AFFECTED( victim, AFF_SHIELD )
             && number_bits( 5 ) == 0 )
            send_to_char
                ( "&RSanctuary does not stack with shield - the damage lessened is the same!\r\n",
                  victim );
        if ( !IS_NPC( victim ) ) {
            if ( IS_AFFECTED( victim, AFF_SANCTUARY )
                 && victim->pcdata->learned[gsn_granite_skin] == 0 )
                dam /= 2;
            if ( IS_AFFECTED( victim, AFF_PROTECT )
                 && victim->pcdata->learned[gsn_granite_skin] == 0 )
                dam -= ( int ) ( dam / 10 );
            if ( IS_AFFECTED( victim, AFF_SHIELD ) && !IS_AFFECTED( victim, AFF_SANCTUARY )
                 && victim->pcdata->learned[gsn_granite_skin] == 0 )
                dam = ( int ) ( dam / 2 );
        }
        else {
            if ( IS_AFFECTED( victim, AFF_SANCTUARY ) )
                dam /= 2;
            if ( IS_AFFECTED( victim, AFF_PROTECT ) )
                dam -= ( int ) ( dam / 10 );
            if ( IS_AFFECTED( victim, AFF_SHIELD ) && !IS_AFFECTED( victim, AFF_SANCTUARY ) )
                dam = ( int ) ( dam / 2 );
        }

        if ( victim->race == RACE_DRAGON )
            dam /= 2;                                  // stop dragons from getting ass
                                                       // handed to them from non-dragons

        if ( IS_AFFECTED( victim, AFF_DEFENSIVE_POSTURING ) && !IS_AFFECTED( victim, AFF_SANCTUARY )
             && !IS_AFFECTED( victim, AFF_SHIELD ) )
            dam -= ( int ) ( dam / 20 );

        if ( dam > 0 && IS_AFFECTED( victim, AFF_IRON_SKIN ) ) {
            short                   iron;

            iron = number_range( 1, 10 );

            if ( victim->secondclass == -1 && iron > 3 )
                dam = ( int ) ( dam / 10 );
            if ( victim->secondclass != -1 && victim->thirdclass == -1 && iron > 5 )
                dam = ( int ) ( dam / 5 );
            if ( victim->secondclass != -1 && victim->thirdclass != -1 && iron > 9 )
                dam = ( int ) ( dam / 2 );
        }
        bool                    shield = FALSE;
        short                   deflect;
        int                     iWear;

        deflect = number_range( 1, 4 );
        // Lets reduce damage if wearing wear_shield 
        for ( iWear = 0; iWear < MAX_WEAR; iWear++ ) {
            for ( pObj = victim->first_carrying; pObj; pObj = pObj->next_content ) {
                if ( pObj->wear_loc == iWear ) {
                    if ( get_eq_char( victim, WEAR_SHIELD ) ) {
                        shield = TRUE;
                    }
                }
            }
        }

        OBJ_DATA               *obj;

        if ( ( obj = get_eq_char( victim, WEAR_SHIELD ) ) != NULL && deflect >= 2 ) {
            if ( obj->value[0] != 1 ) {
                if ( obj->level <= 20 ) {
                    obj->value[0] -= 1;
                }
                else if ( obj->level <= 40 && obj->level > 20 ) {
                    obj->value[0] -= 2;
                }
                else if ( obj->level <= 60 && obj->level > 40 ) {
                    obj->value[0] -= 3;
                }
                else if ( obj->level > 60 ) {
                    obj->value[0] -= 4;
                }
            }
            if ( obj->value[0] < 1 ) {
                obj->value[0] = 2;
            }
            if ( obj->value[0] < obj->value[1] / 2 && obj->value[0] != 1 ) {
                obj->value[0] = 1;
                act( AT_ACTION, "Your shield crumples around your arm and is now useless.", victim,
                     NULL, NULL, TO_CHAR );
                act( AT_ACTION, "$n's shield crumples around $s arm and is now useless.", victim,
                     NULL, NULL, TO_ROOM );
            }
        }

        /*
         * Need a check in here to check shield object condition, and whether it would scrap 
         * from the amount of damage it is taking. -- Vladaar 
         */

        if ( shield == TRUE && deflect >= 2 && obj->value[0] != 1 ) {
            act( AT_ACTION, "Your attack strikes $N's shield.", ch, NULL, victim, TO_CHAR );
            act( AT_ACTION, "$n's attack strikes $N's shield.", ch, NULL, victim, TO_ROOM );
            dam = ( int ) ( dam / 2 );
        }

        if ( dam > 0 && !IS_NPC( victim ) && ( victim->pcdata->learned[gsn_mitigate] > 0 ) ) {
            short                   mitty;

            mitty = number_range( 1, 10 );

            if ( victim->secondclass == -1 && mitty > 5 ) {
                dam = ( int ) ( dam / 10 );
                learn_from_success( victim, gsn_mitigate );
            }
            if ( victim->secondclass != -1 && victim->thirdclass == -1 && mitty > 7 ) {
                dam = ( int ) ( dam / 5 );
                learn_from_success( victim, gsn_mitigate );
            }
            if ( victim->secondclass != -1 && victim->thirdclass != -1 && mitty > 9 ) {
                dam = ( int ) ( dam / 2 );
                learn_from_success( victim, gsn_mitigate );
            }
        }

        if ( dam > 0 && !IS_NPC( victim ) && ( victim->pcdata->learned[gsn_marble_skin] > 0 ) ) {
            short                   skin;

            skin = number_range( 1, 10 );
            skin += 1 + ( victim->level / 10 );
            if ( skin > 5 ) {
                dam = dam - ( dam / 10 );
                if ( skin >= 8 ) {
                    learn_from_success( victim, gsn_marble_skin );
                }
            }
        }

        if ( dam > 0 && !IS_NPC( victim ) && ( victim->pcdata->learned[gsn_granite_skin] > 0 ) ) {
            short                   gskin;

            gskin = number_range( 1, 10 );
            gskin += 1 + ( victim->level / 10 );
            if ( gskin > 5 ) {
                dam = ( int ) ( dam / 2 );
                if ( gskin >= 8 ) {
                    learn_from_success( victim, gsn_granite_skin );
                }
            }
        }

        if ( dam > 0 && !IS_NPC( victim ) && ( victim->pcdata->learned[gsn_glacial_armor] > 0 ) ) {
            short                   glacial;

            glacial = number_range( 1, 10 );

            if ( victim->secondclass == -1 && glacial > 3 ) {
                learn_from_success( victim, gsn_glacial_armor );
                dam = ( int ) ( dam - dam / 10 );
            }

        }
        if ( dam < 0 )
            dam = 0;

        SKILLTYPE              *skill;
        int                     mod;

        mod = abs( dt );
        if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
             && skill->type == SKILL_SPELL && IS_SINGLECLASS( ch ) && NOT_FWARRIOR( ch )
             && ch->Class != CLASS_ANGEL && ch->Class != CLASS_HELLSPAWN && ch->race != RACE_DRAGON
             && ch->Class != CLASS_DRAGONLORD && ch->Class != CLASS_BARD ) {
            dam *= 2;
        }

        if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
             && skill->type == SKILL_SPELL && NOT_FWARRIOR( ch ) && NOT_SWARRIOR( ch )
             && NOT_TWARRIOR( ch ) && ch->secondclass != -1 ) {
            dam = ( int ) ( dam * 1.5 );
        }

        if ( mod == gsn_kick || mod == gsn_heavy_hands || mod == gsn_punch || mod == gsn_bite
             || mod == ATCK_FIREBALL || mod == ATCK_CLAWS || mod == ATCK_TAIL_SWIPE
             || mod == ATCK_STING || mod == ATCK_TRIP || mod == ATCK_BASH || mod == ATCK_EBOLT
             || mod == ATCK_STUN || mod == ATCK_GOUGE || mod == ATCK_BACKSTAB || mod == ATCK_FEED
             || mod == ATCK_DRAIN || mod == ATCK_FIREBREATH || mod == ATCK_GSMITE
             || mod == ATCK_FROSTBREATH || mod == ATCK_ACIDBREATH || mod == ATCK_LIGHTNBREATH
             || mod == ATCK_GASBREATH || mod == ATCK_POISON || mod == ATCK_NASTYPOISON
             || mod == ATCK_GAZE || mod == ATCK_BLINDNESS || mod == ATCK_CAUSESERIOUS
             || mod == ATCK_EARTHQUAKE || mod == ATCK_CAUSECRITICAL || mod == ATCK_CURSE
             || mod == ATCK_FLAMESTRIKE || mod == ATCK_HARM || mod == ATCK_FIREBALL
             || mod == ATCK_COLORSPRAY || mod == ATCK_WEAKEN || mod == ATCK_SPIRALBLAST
             || mod == ATCK_INFERNO || mod == ATCK_BLIZZARD || mod == ATCK_BLIGHTNING
             || mod == ATCK_BRAINBOIL || mod == ATCK_LTOUCH ) {
            ch->success_attack = 1;
        }

        if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
             && skill->type == SKILL_SPELL && ch->success_attack != 1 ) {
            ch->success_attack = 1;
        }
        else if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
                  && skill->type == SKILL_SPELL && victim->success_attack != 1 ) {
            victim->success_attack = 1;
        }
        else if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
                  && skill->type == SKILL_SKILL && ch->success_attack != 1 ) {
            ch->success_attack = 1;
        }
        else if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
                  && skill->type == SKILL_SKILL && dt > TYPE_HIT && victim->success_attack != 1 ) {
            victim->success_attack = 1;
        }

        if ( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL
             && skill->type == SKILL_SKILL && IS_NPC( victim ) ) {
            dam = ( int ) ( dam * 1.8 );
        }

        ch->damage_amount = 0;
        dam = dam * ch->success_attack;                // Fury
        ch->damage_amount = dam;

        /*
         * Check for disarm, trip, parry, dodge, tumble and phase, 
         shieldblock, and displacement.
         */
        if ( dt >= TYPE_HIT && ch->in_room == victim->in_room ) {
            if ( IS_NPC( ch ) && xIS_SET( ch->defenses, DFND_DISARM ) && ch->level > 9
                 && number_percent(  ) < ch->level / 3 )
                disarm( ch, victim );
            // lets make this occur less often from mobs. -Taon
            if ( IS_NPC( ch ) && number_chance( 0, 10 ) > 8 ) {
                if ( IS_NPC( ch ) && xIS_SET( ch->attacks, ATCK_TRIP ) && ch->level > 5
                     && number_percent(  ) < ch->level / 2 )
                    trip( ch, victim );
            }
            if ( check_parry( ch, victim ) ) {
                if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
                    send_to_char( "!!SOUND(sound/parry.wav)\r\n", ch );
                return rNONE;
            }

            if ( check_dodge( ch, victim ) ) {
                if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
                    send_to_char( "!!SOUND(sound/miss.wav)\r\n", ch );
                return rNONE;
            }

            if ( check_tumble( ch, victim ) ) {
                if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
                    send_to_char( "!!SOUND(sound/miss.wav)\r\n", ch );
                return rNONE;
            }

            if ( check_phase( ch, victim ) ) {
                if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
                    send_to_char( "!!SOUND(sound/miss.wav)\r\n", ch );
                return rNONE;
            }

            if ( check_displacement( ch, victim ) ) {
                if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
                    send_to_char( "!!SOUND(sound/miss.wav)\r\n", ch );
                return rNONE;
            }

        }

        /*
         * Check control panel settings and modify damage
         */
        if ( IS_NPC( ch ) ) {
            if ( npcvict )
                dampmod = sysdata.dam_mob_vs_mob;
            else
                dampmod = sysdata.dam_mob_vs_plr;
        }
        else {
            if ( npcvict )
                dampmod = sysdata.dam_plr_vs_mob;
            else
                dampmod = sysdata.dam_plr_vs_plr;
        }
        if ( dampmod > 0 )
            dam = ( dam * dampmod ) / 100;
    }

    /*
     * Code to handle equipment getting damaged, and also support  -Thoric
     * bonuses/penalties for having or not having equipment where hit
     */

    if ( dam > 50 && dt != TYPE_UNDEFINED && dt != gsn_poison ) {
        /*
         * get a random body eq part 
         */
        // Lets not let eq damage in the arena. -Taon
        if ( !in_arena( victim ) ) {
            dameq = number_range( WEAR_LIGHT, WEAR_SHEATH );
            damobj = get_eq_char( victim, dameq );
            if ( damobj ) {
                if ( dam > get_obj_resistance( damobj ) && number_bits( 1 ) == 0
                     && number_chance( 1, 10 ) >= 8 ) {
                    set_cur_obj( damobj );
                    damage_obj( damobj );
                }
                dam -= 5;                              /* add a bonus for having
                                                        * something to block the blow */
            }
            else
                dam += 5;                              /* add penalty for bare skin! */
        }
    }

    if ( dam == 0 )
        ch->success_attack = 0;

    if ( ch != victim )
        dam_message( ch, victim, dam, dt );

    /*
     * Hurt the victim, Inform the victim of his new state.
     */
    victim->hit -= dam;

    if ( dam > 0 )
        lodge_projectile( ch, victim );

    short                   chance;

    chance = number_range( 1, 4 );
    if ( victim->secondclass == -1 ) {
        chance += 1;
    }

    if ( IS_AFFECTED( victim, AFF_REACTIVE ) && dam > 0 && victim->hit > 0
         && number_percent(  ) > 40 && chance > 1 ) {
        act( AT_MAGIC, "$n reactively heals some damage.", victim, NULL, NULL, TO_ROOM );
        act( AT_MAGIC, "An energy fills you and you reactively heal some of the damage.", victim,
             NULL, NULL, TO_CHAR );
        if ( victim->level < 20 )
            victim->hit += number_range( 3, 18 );
        if ( victim->level > 19 && victim->level < 40 )
            victim->hit += number_range( 6, 36 );
        if ( victim->level > 39 && victim->level < 60 )
            victim->hit += number_range( 9, 54 );
        if ( victim->level > 59 )
            victim->hit += number_range( 18, 72 );
        if ( !IS_NPC( victim ) && victim->secondclass == -1 && victim->Class == CLASS_PRIEST
             || victim->Class == CLASS_DRUID )
            victim->hit += number_range( 9, 54 );
        if ( !IS_NPC( victim ) && victim->secondclass != -1 && victim->thirdclass == -1
             && victim->Class == CLASS_PRIEST || victim->Class == CLASS_DRUID )
            victim->hit += number_range( 6, 24 );
        if ( !IS_NPC( victim ) && victim->Class == CLASS_ANGEL )
            victim->hit += number_range( 3, 12 );
        if ( victim->hit > victim->max_hit )
            victim->hit = victim->max_hit;
    }

    /*
     * Get experience based on % of damage done       -Thoric
     */
    if ( dam && ch != victim && !IS_NPC( ch ) && ch->fighting && ch->fighting->xp
         && ( !IS_NPC( victim ) || !xIS_SET( victim->act, ACT_IMMORTAL ) ) ) {
        int                     xp_gain = 0;

        if ( !IS_AFFECTED( ch, AFF_FEIGN ) ) {
            if ( ch->fighting->who == victim )
                xp_gain =
                    ( ( ch->fighting->xp * dam ) / ( victim->max_hit ) ) * ( victim->level /
                                                                             ch->level );
            else
                xp_gain = ( int ) ( xp_compute( ch, victim ) * 0.85 * dam ) / victim->max_hit;
            // Stop players from losing exp during combat. -Taon
            if ( xp_gain <= 0 )
                xp_gain = 0;

            gain_exp( ch, xp_gain );
        }
    }

    /*
     * If immortal or new player don't let them die 
     */
    if ( !IS_NPC( victim ) && ( victim->level >= LEVEL_IMMORTAL || NEW_AUTH( victim ) )
         && victim->hit < 1 )
        victim->hit = 1;

    if ( dam > 0 && dt > TYPE_HIT && !IS_AFFECTED( victim, AFF_POISON )
         && is_wielding_poisoned( ch ) && !IS_SET( victim->immune, RIS_POISON )
         && !saves_poison_death( ch->level, victim ) ) {
        AFFECT_DATA             af;

        af.type = gsn_poison;
        af.duration = 20;
        af.location = APPLY_STR;
        af.modifier = -2;
        af.level = ch->level;
        af.bitvector = meb( AFF_POISON );
        affect_join( victim, &af );
        if ( IS_SECONDCLASS( ch ) )
            victim->degree = 2;
        if ( IS_THIRDCLASS( ch ) )
            victim->degree = 3;
        if ( ch->secondclass == -1 )
            victim->degree = 1;

        victim->mental_state =
            URANGE( 20, victim->mental_state + ( IS_PKILL( victim ) ? 1 : 2 ), 100 );
    }

    /*
     * Vampire self preservation     -Thoric
     */
    if ( IS_BLOODCLASS( victim ) ) {
        if ( dam >= ( victim->max_hit / 10 ) )
            victim->blood = ( victim->blood - 1 - ( victim->level / 20 ) );

        if ( victim->hit <= ( victim->max_hit / 8 ) && victim->blood > 5 ) {
            if ( victim->blood >= victim->blood / 2 ) {
                victim->hit += URANGE( 12, ( victim->max_hit / 30 ), 45 );
                victim->blood = ( victim->blood - URANGE( 6, victim->level / 10, 12 ) );
            }
            else {
                victim->blood = ( victim->blood - URANGE( 3, victim->level / 10, 8 ) );
                victim->hit += URANGE( 4, ( victim->max_hit / 30 ), 15 );
            }
            set_char_color( AT_BLOOD, victim );
            send_to_char( "You howl with rage as the beast within stirs!\r\n", victim );
        }
    }
    if ( !npcvict && get_trust( victim ) >= LEVEL_IMMORTAL && get_trust( ch ) >= LEVEL_IMMORTAL
         && victim->hit < 1 )
        victim->hit = 1;
    if ( !IS_NPC( victim ) && NEW_AUTH( victim ) && victim->hit < 1 )
        victim->hit = 1;

    update_pos( victim );

    switch ( victim->position ) {
        case POS_MORTAL:
            act( AT_RED, "$n is mortally wounded, and will die soon, if not aided.", victim, NULL,
                 NULL, TO_ROOM );
            act( AT_RED, "You are mortally wounded, and will die soon, if not aided.", victim, NULL,
                 NULL, TO_CHAR );
            break;

        case POS_INCAP:
            act( AT_RED, "$n is incapacitated and will slowly die, if not aided.", victim, NULL,
                 NULL, TO_ROOM );
            act( AT_RED, "You are incapacitated and will slowly die, if not aided.", victim, NULL,
                 NULL, TO_CHAR );
            break;

        case POS_STUNNED:
            if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
                act( AT_ACTION, "$n is stunned, but will probably recover.", victim, NULL, NULL,
                     TO_ROOM );
                act( AT_RED, "You are stunned, but will probably recover.", victim, NULL, NULL,
                     TO_CHAR );
            }
            break;

        case POS_DEAD:
            if ( dt >= 0 && dt < top_sn ) {
                SKILLTYPE              *skill = skill_table[dt];

                if ( skill->die_char && skill->die_char[0] != '\0' )
                    act( AT_RED, skill->die_char, ch, NULL, victim, TO_CHAR );
                if ( skill->die_vict && skill->die_vict[0] != '\0' )
                    act( AT_RED, skill->die_vict, ch, NULL, victim, TO_VICT );
                if ( skill->die_room && skill->die_room[0] != '\0' )
                    act( AT_RED, skill->die_room, ch, NULL, victim, TO_NOTVICT );
            }
            act( AT_RED, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
            act( AT_RED, "You have been KILLED!!\r\n", victim, 0, 0, TO_CHAR );
            break;

        default:
            /*
             * Victim mentalstate affected, not attacker -- oops ;)
             * Thanks to gfinello@mail.karmanet.it for finding this bug
             */
            if ( dam > victim->max_hit / 4 ) {
                act( AT_RED, "That really did HURT!", victim, 0, 0, TO_CHAR );
                if ( number_bits( 3 ) == 0 )
                    worsen_mental_state( victim, 1 );
            }
            if ( victim->hit < victim->max_hit / 6 )
            {
                act( AT_RED, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0,
                     TO_CHAR );
                if ( number_bits( 2 ) == 0 )
                    worsen_mental_state( victim, 1 );
            }
            break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE( victim ) && !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
        stop_summoning( victim );

        if ( !npcvict && IS_NPC( ch ) )
            stop_fighting( victim, TRUE );
        else
            stop_fighting( victim, FALSE );
    }

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD ) {
        /*
         * Let's put arena code at the top. We DON'T want to hit group_gain. 
         */
        if ( !IS_NPC( ch ) && !IS_NPC( victim ) && in_arena( victim ) ) {
            if ( victim != ch )
                snprintf( buf, MSL, "%s has been slain by %s!", victim->name, ch->name );
            else
                snprintf( buf, MSL, "%s has been slain!", victim->name );

            if ( victim->position == POS_DEAD ) {
                set_position( victim, POS_STANDING );
            }

            CLAN_DATA              *clan;

            clan = ch->pcdata->clan;

            if ( victim != ch && IS_CLANNED( ch ) && IS_CLANNED( victim )
                 && victim->pcdata->clan == clan ) {
                if ( !clan->warmaster || !str_cmp( clan->warmaster, victim->name ) ) {
                    if ( !clan->chieftain || str_cmp( clan->chieftain, ch->name ) ) {
                        if ( clan->warmaster )
                            STRFREE( clan->warmaster );
                        clan->warmaster = STRALLOC( ch->name );
                        save_clan( clan );
                    }
                }
            }

            // Clan members gain points for the clan with kills.
            if ( !IS_NPC( ch ) && victim != ch && !IS_NPC( victim ) ) {
                if ( ch->pcdata->clan ) {
                    clan = ch->pcdata->clan;
                    ch->pcdata->clanpoints += 3;
                    clan->totalpoints += 3;
                    ch_printf( ch,
                               "&G%s clan has gained 3 points from your kill, now totaling %d clan status points!\r\n",
                               clan->name, clan->totalpoints );
                    if ( countpkill( ch, victim ) )
                        increase_clan_pkills( ch->pcdata->clan, ch->level );
                    save_clan( clan );
                }
                if ( victim->pcdata->clan && countpkill( ch, victim ) ) {
                    increase_clan_pdeaths( victim->pcdata->clan, victim->level );
                    save_clan( victim->pcdata->clan );
                }
                ch->pcdata->pkills++;
                victim->pcdata->pdeaths++;
                add_pkill( ch, victim );
                save_char_obj( ch );
            }

            arena_population--;
            victim->arena_loss++;
            arena_chan( buf );

            /*
             * Volk: Let's send them on their way, restored 
             */
            restore_char( victim );
            char_from_room( victim );
            char_to_room( victim, get_room_index( victim->pcdata->htown->recall ) );
            affect_strip( victim, gsn_maim );
            affect_strip( victim, gsn_brittle_bone );
            affect_strip( victim, gsn_festering_wound );
            affect_strip( victim, gsn_poison );
            xREMOVE_BIT( victim->affected_by, AFF_BRITTLE_BONES );
            xREMOVE_BIT( victim->affected_by, AFF_FUNGAL_TOXIN );
            xREMOVE_BIT( victim->affected_by, AFF_MAIM );

            /*
             * So players don't drown if returning underwater from the water arena, heh 
             */
            if ( victim->pcdata && !IS_NPC( victim ) )
                victim->pcdata->holdbreath = 0;

            if ( arena_population == 1 && arena_underway && victim != ch ) {
                find_arena_winner( ch );
                char_from_room( ch );
                char_to_room( ch, get_room_index( ch->pcdata->htown->recall ) );

                if ( ch->pcdata )
                    ch->pcdata->holdbreath = 0;
            }

            return rNONE;
        }

        group_gain( ch, victim );

        /*
         * Always loose 8% exp for each class on death 
         */
        if ( !npcvict ) {
            if ( IS_THIRDCLASS( victim ) ) {
                if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->firstexp -=
                        ( int ) ( exp_class_level( victim, victim->firstlevel + 1, victim->Class ) *
                                  .8 );
                }
                else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->firstexp -=
                        ( int ) ( exp_class_level( victim, victim->firstlevel + 1, victim->Class ) *
                                  .08 );
                }

                if ( victim->firstexp < 0 )
                    victim->firstexp = 0;
                if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->secondexp -=
                        ( int ) ( exp_class_level
                                  ( victim, victim->secondlevel + 1, victim->secondclass ) * .8 );
                }
                else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->secondexp -=
                        ( int ) ( exp_class_level
                                  ( victim, victim->secondlevel + 1, victim->secondclass ) * .08 );
                }

                if ( victim->secondexp < 0 )
                    victim->secondexp = 0;

                if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->thirdexp -=
                        ( int ) ( exp_class_level
                                  ( victim, victim->thirdlevel + 1, victim->thirdclass ) * .8 );
                }
                else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->thirdexp -=
                        ( int ) ( exp_class_level
                                  ( victim, victim->thirdlevel + 1, victim->thirdclass ) * .08 );
                }

                if ( victim->thirdexp < 0 )
                    victim->thirdexp = 0;
            }
            else if ( IS_SECONDCLASS( victim ) ) {
                if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->firstexp -=
                        ( int ) ( exp_class_level( victim, victim->firstlevel + 1, victim->Class ) *
                                  .8 );
                }
                else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->firstexp -=
                        ( int ) ( exp_class_level( victim, victim->firstlevel + 1, victim->Class ) *
                                  .08 );
                }

                if ( victim->firstexp < 0 )
                    victim->firstexp = 0;
                if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->secondexp -=
                        ( int ) ( exp_class_level
                                  ( victim, victim->secondlevel + 1, victim->secondclass ) * .8 );
                }
                else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->secondexp -=
                        ( int ) ( exp_class_level
                                  ( victim, victim->secondlevel + 1, victim->secondclass ) * .08 );
                }

                if ( victim->secondexp < 0 )
                    victim->secondexp = 0;
            }
            else {
                if ( xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->exp -= ( int ) ( exp_level( victim, victim->level + 1 ) * .8 );;
                }
                else if ( !xIS_SET( victim->act, PLR_EXTREME ) ) {
                    victim->exp -= ( int ) ( exp_level( victim, victim->level + 1 ) * .08 );
                }
                if ( victim->exp < 0 )
                    victim->exp = 0;
            }
        }
        else if ( !IS_NPC( ch ) && IS_NPC( victim ) ) { /* keep track of mob vnum killed */
            if ( !happyhouron || !IS_GROUPED( ch ) ) {
                add_kill( ch, victim );
                /*
                 * Add to kill tracker for grouped chars, as well. -Halcyon 
                 */
                for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
                    if ( is_same_group( gch, ch ) && !IS_NPC( gch ) && gch != ch )
                        add_kill( gch, victim );
            }
        }
        check_killer( ch, victim );

        if ( ch->in_room == victim->in_room )
            loot = legal_loot( ch, victim );
        else
            loot = FALSE;

        set_cur_char( victim );
        raw_kill( ch, victim );

        if ( victim == ch )
            return rNONE;

        if ( !IS_NPC( ch ) && loot ) {
            /*
             * Automoney by Scryn 8/12 . Editted by Volk to pay clan tithes!
             */
            if ( xIS_SET( ch->act, PLR_AUTOMONEY ) ) {
                int                     init,
                                        diff;
                OBJ_DATA               *corpse;
                bool                    members = FALSE;

                if ( ( corpse = get_obj_here( ch, ( char * ) "corpse" ) ) ) {
                    OBJ_DATA               *mobj;
                    char                    mbuf[80];

                    for ( mobj = corpse->first_content; mobj; mobj = mobj->next_content ) {
                        if ( mobj->item_type != ITEM_MONEY )
                            continue;
                        init = GET_MONEY( ch, mobj->value[2] );
                        snprintf( mbuf, 80, "%s corpse", curr_types[mobj->value[2]] );
                        do_get( ch, mbuf );            /* Get the gold/silver from the
                                                        * corpse */
                        diff = GET_MONEY( ch, mobj->value[2] ) - init;
                        if ( diff > 1 ) {
                            for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room ) {
                                if ( is_same_group( gch, ch ) && gch != ch && !IS_NPC( gch ) ) {
                                    members = TRUE;
                                    snprintf( buf1, MSL, "%d %s", diff,
                                              curr_types[mobj->value[2]] );
                                    do_split( ch, buf1 );
                                }
                            }
                            /*
                             * if (!members) clan_tithe(ch, diff, mobj->value[2]); 
                             */
                        }
                    }
                }
            }
            if ( xIS_SET( ch->act, PLR_AUTOLOOT ) && victim != ch )
                do_get( ch, ( char * ) "all corpse" );
            else
                do_look( ch, ( char * ) "in corpse" );

            if ( xIS_SET( ch->act, PLR_AUTOTRASH ) )
                do_trash( ch, ( char * ) "corpse" );
        }

        if ( xIS_SET( sysdata.save_flags, SV_KILL ) )
            save_char_obj( ch );
        return rVICT_DIED;
    }

    /*
     * Take care of link dead people.
     */
    if ( !npcvict && !victim->desc && !IS_SET( victim->pcdata->flags, PCFLAG_NORECALL ) ) {
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

    /*
     * Now I fiqure this is the best place to switch victims :) 
     * Should we do same group only or just anyone fighting it? 
     */
    if ( ch->fighting && victim != ch )
        for ( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
            if ( is_same_group( victim, gch ) && ( victim != gch )
                 && gch->hate_level > victim->hate_level ) {
                stop_fighting( ch, FALSE );            /* Stop the current fighting for
                                                        * ch only */
                set_fighting( ch, gch );               /* Start them on the next one? */
                snprintf( buf, MSL, "\r\n&O Yell [%s] %s you will die for that!", ch->name,
                          gch->name );
                do_recho( ch, buf );
            }

    tail_chain(  );
    return retcode;
}

/*
 * Changed is_safe to have the show_messg boolian.  This is so if you don't
 * want to show why you can't kill someone you can turn it off.  This is
 * useful for things like area attacks.  --Shaddai
 */
/* Volk - needs restrictions for peacefuls/nice flag/etc */
bool is_safe( CHAR_DATA *ch, CHAR_DATA *victim, bool show_messg )
{
    if ( char_died( victim ) || char_died( ch ) )
        return TRUE;

    /*
     * Thx Josh! 
     */
    if ( who_fighting( ch ) == ch )
        return FALSE;

    if ( !victim ) {                                   /* Gonna find this is_safe crash
                                                        * bug -Blod */
        bug( "Is_safe: %s opponent does not exist!", ch->name );
        return TRUE;
    }
    if ( !victim->in_room ) {
        bug( "Is_safe: %s has no physical location!", victim->name );
        return TRUE;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_SAFE ) ) {
        if ( show_messg ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "A magical force prevents you from attacking.\r\n", ch );
        }
        return TRUE;
    }

    if ( ( IS_PACIFIST( ch ) ) && !IN_ARENA( ch ) ) {  /* Fireblade */
        if ( show_messg ) {
            set_char_color( AT_MAGIC, ch );
            ch_printf( ch, "You are a pacifist and will not fight.\r\n" );
        }
        return TRUE;
    }

    if ( IS_PACIFIST( victim ) ) {                     /* Gorog */
        if ( show_messg ) {
            set_char_color( AT_MAGIC, ch );
            ch_printf( ch, "%s is a pacifist and will not fight.\r\n",
                       capitalize( victim->short_descr ) );
        }
        return TRUE;
    }

    if ( in_arena( ch ) )
        return FALSE;

    if ( !IS_NPC( ch ) && ch->level >= LEVEL_IMMORTAL )
        return FALSE;

    if ( !IS_NPC( ch ) && !IS_NPC( victim ) && ch != victim
         && ( IS_SET( victim->in_room->area->flags, AFLAG_ANTIPKILL )
              || IS_SET( victim->in_room->area->flags, AFLAG_NOPKILL ) ) ) {
        if ( show_messg ) {
            set_char_color( AT_IMMORT, ch );
            send_to_char( "The gods have forbidden player killing in this area.\r\n", ch );
        }
        return TRUE;
    }

    if ( IS_NPC( ch ) || IS_NPC( victim ) )
        return FALSE;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) && ( ch->level - victim->level > 10 )
         && !in_arena( ch ) )
        return FALSE;

    if ( ( ch->level < 5 ) && !in_arena( ch ) ) {
        if ( show_messg ) {
            set_char_color( AT_WHITE, ch );
            send_to_char( "You are not yet ready, needing age or experience, if not both. \r\n",
                          ch );
        }
        return TRUE;
    }

    if ( ( victim->level < 5 ) && !in_arena( victim ) ) {
        if ( show_messg ) {
            set_char_color( AT_WHITE, ch );
            send_to_char( "They are yet too young to die.\r\n", ch );
        }
        return TRUE;
    }

    if ( ch->level <= 10 && !in_arena( ch ) ) {        /* 10 and lower */
        if ( ( ch->level - victim->level >= 3 ) || ( victim->level - ch->level >= 3 ) ) {
            if ( show_messg ) {
                set_char_color( AT_IMMORT, ch );
                send_to_char
                    ( "The STAFF of 6 Dragons do not allow murder when there is such a difference in level.\r\n",
                      ch );
            }
            return TRUE;
        }
    }

    if ( ch->level > 29 && !in_arena( ch ) ) {         /* 30 and upwards */
        if ( ( ch->level - victim->level > 10 ) || ( victim->level - ch->level > 10 ) ) {
            if ( show_messg ) {
                set_char_color( AT_IMMORT, ch );
                send_to_char
                    ( "The STAFF of 6 Dragons do not allow murder when there is such a difference in level.\r\n",
                      ch );
            }
            return TRUE;
        }
    }
    else if ( ch->level < 30 && !in_arena( ch ) ) {    /* less than 30 */
        if ( ( ch->level - victim->level > 5 ) || ( victim->level - ch->level > 5 ) ) {
            if ( show_messg ) {
                set_char_color( AT_IMMORT, ch );
                send_to_char
                    ( "The STAFF of 6 Dragons do not allow murder when there is such a difference in level.\r\n",
                      ch );
            }
            return TRUE;
        }
    }

    if ( ( get_timer( victim, TIMER_PKILLED ) > 0 ) && !in_arena( victim ) ) {
        if ( show_messg ) {
            set_char_color( AT_GREEN, ch );
            send_to_char( "That character has died within the last 1 minute.\r\n", ch );
        }
        return TRUE;
    }

    /*
     * You have killed them 
     */
    if ( !in_arena( ch ) && has_pkilled( ch, victim->name ) ) {
        if ( show_messg ) {
            set_char_color( AT_RED, ch );
            send_to_char( "You have pkilled that character within the past 30 minutes.\r\n", ch );
        }
        return TRUE;
    }

    /*
     * Should always check both sides 
     */
    if ( !in_arena( victim ) && has_pkilled( victim, ch->name ) ) {
        if ( show_messg ) {
            set_char_color( AT_RED, ch );
            send_to_char( "They have pkilled you within the past 30 minutes.\r\n", victim );
        }
        return TRUE;
    }

    if ( ( get_timer( ch, TIMER_PKILLED ) > 0 ) && !in_arena( ch ) ) {
        if ( show_messg ) {
            set_char_color( AT_GREEN, ch );
            send_to_char( "You have been killed within the last 1 minute.\r\n", ch );
        }
        return TRUE;
    }

    return FALSE;
}

/*
 * just verify that a corpse looting is legal
 */
bool legal_loot( CHAR_DATA *ch, CHAR_DATA *victim )
{
    /*
     * anyone can loot mobs 
     */
    if ( IS_NPC( victim ) )
        return TRUE;
    /*
     * non-charmed mobs can loot anything 
     */
    if ( IS_NPC( ch ) && !ch->master )
        return TRUE;
    /*
     * members of different clans can loot too! -Thoric 
     */
    if ( !IS_NPC( ch ) && !IS_NPC( victim ) && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
         && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
        return TRUE;
    return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int                     level_ratio;

    /*
     * NPC's are fair game. 
     */
    if ( IS_NPC( victim ) )
        return;

    level_ratio = URANGE( 1, ch->level / victim->level, 50 );
    if ( !IS_NPC( ch ) ) {

        if ( ch->pcdata->deity ) {
            if ( victim->race == ch->pcdata->deity->npcrace )
                adjust_favor( ch, 3, level_ratio );
            else if ( victim->race == ch->pcdata->deity->npcfoe )
                adjust_favor( ch, 17, level_ratio );
            else
                adjust_favor( ch, 2, level_ratio );
        }

        ch->pcdata->mkills++;
        if ( ch->pcdata->clan )
            ch->pcdata->clan->mkills++;
        // update_member(ch);
        ch->in_room->area->mkills++;
    }

    /*
     * If you kill yourself nothing happens.
     */

    if ( ch == victim || ch->level >= LEVEL_IMMORTAL )
        return;

    /*
     * Any character in the arena is ok to kill.
     * Added pdeath and pkills here
     */
    if ( in_arena( ch ) || in_arena( victim ) )
        return;

    if ( xIS_SET( victim->act, PLR_KILLER ) || xIS_SET( victim->act, PLR_THIEF ) )
        return;

    /*
     * clan checks     -Thoric 
     */
    if ( !IS_NPC( ch ) && !IS_NPC( victim ) && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
         && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) {
        /*
         * not of same clan? Go ahead and kill!!! 
         */
        if ( !ch->pcdata->clan || !victim->pcdata->clan
             || ch->pcdata->clan != victim->pcdata->clan ) {
            // update_member(ch);
            ch->hit = ch->max_hit;
            ch->mana = ch->max_mana;
            ch->move = ch->max_move;
            if ( IS_BLOODCLASS( ch ) ) {
                ch->blood = ch->max_blood;
            }
            update_pos( victim );
            if ( victim != ch ) {
                act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into $n.", ch,
                     victim->name, NULL, TO_ROOM );
                act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into you.", ch,
                     victim->name, NULL, TO_CHAR );
            }

            if ( ch->pcdata->deity ) {
                if ( victim->race == ch->pcdata->deity->npcrace )
                    adjust_favor( victim, 11, level_ratio );
                else if ( victim->race == ch->pcdata->deity->npcfoe )
                    adjust_favor( ch, 2, level_ratio );
                else
                    adjust_favor( ch, 2, level_ratio );
            }

            if ( victim->pcdata->deity ) {
                if ( ch->race == victim->pcdata->deity->npcrace )
                    adjust_favor( victim, 12, level_ratio );
                else if ( ch->race == victim->pcdata->deity->npcfoe )
                    adjust_favor( victim, 15, level_ratio );
                else
                    adjust_favor( victim, 11, level_ratio );
            }

            // update_member(victim);
            add_timer( victim, TIMER_PKILLED, 23, NULL, 0 );
            WAIT_STATE( victim, 3 * PULSE_VIOLENCE );
            /*
             * xSET_BIT(victim->act, PLR_PK); 
             */
            return;
        }
    }

    /*
     * Charm-o-rama.
     */
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        if ( !ch->master ) {
            bug( "Check_killer: %s bad AFF_CHARM", IS_NPC( ch ) ? ch->short_descr : ch->name );
            affect_strip( ch, gsn_charm_person );
            xREMOVE_BIT( ch->affected_by, AFF_CHARM );
            return;
        }

        /*
         * stop_follower(ch); 
         */
        if ( ch->master )
            check_killer( ch->master, victim );
        return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC( ch ) ) {
        if ( !IS_NPC( victim ) ) {
            int                     level_ratio;

            victim->pcdata->mdeaths++;
            if ( victim->pcdata->clan )
                victim->pcdata->clan->mdeaths++;
            // update_member(ch);
            if ( victim->pcdata->deity ) {
                if ( ch->race == victim->pcdata->deity->npcrace )
                    adjust_favor( victim, 12, level_ratio );
                else if ( ch->race == victim->pcdata->deity->npcfoe )
                    adjust_favor( victim, 15, level_ratio );
                else
                    adjust_favor( victim, 11, level_ratio );
            }

        }
        return;
    }

    if ( !IS_NPC( ch ) ) {
        if ( ch->pcdata->clan )
            ch->pcdata->clan->illegal_pk++;
        ch->pcdata->illegal_pk++;
        ch->in_room->area->illegal_pk++;
    }

    if ( !IS_NPC( victim ) ) {
        victim->in_room->area->pdeaths++;
    }

    set_char_color( AT_WHITE, ch );
    send_to_char( "A strange feeling grows deep inside you, and a tingle goes up your spine...\r\n",
                  ch );
    set_char_color( AT_IMMORT, ch );
    send_to_char
        ( "A deep voice booms inside your head, 'Thou shall now be known as a deadly murderer!!!'\r\n",
          ch );
    set_char_color( AT_WHITE, ch );
    send_to_char( "You feel as if your soul has been revealed for all to see.\r\n", ch );
    save_char_obj( ch );
    return;
}

/*
 * See if an attack justifies a ATTACKER flag.
 */
void check_attacker( CHAR_DATA *ch, CHAR_DATA *victim )
{

/* 
 * Made some changes to this function Apr 6/96 to reduce the prolifiration
 * of attacker flags in the realms. -Narn
 */
    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC( victim ) || xIS_SET( victim->act, PLR_KILLER )
         || xIS_SET( victim->act, PLR_THIEF ) )
        return;

    if ( IN_ARENA( ch ) || IN_ARENA( victim ) || in_arena( ch ) || in_arena( victim ) )
        return;

/* Pkiller versus pkiller will no longer ever make an attacker flag
    { if(!(ch->pcdata->clan && victim->pcdata->clan
      && ch->pcdata->clan == victim->pcdata->clan))  return; }
*/

    /*
     * Charm-o-rama.
     */
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        if ( !ch->master ) {
            // bug( "Check_attacker: %s bad AFF_CHARM", IS_NPC( ch ) ? ch->short_descr :
            // ch->name );
            affect_strip( ch, gsn_charm_person );
            xREMOVE_BIT( ch->affected_by, AFF_CHARM );
            return;
        }

        /*
         * Won't have charmed mobs fighting give the master an attacker 
         * flag.  The killer flag stays in, and I'll put something in 
         * do_murder. -Narn 
         */
        /*
         * xSET_BIT(ch->master->act, PLR_ATTACKER);
         */
        /*
         * stop_follower(ch); 
         */
        return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC( ch ) || ch == victim || ch->level >= LEVEL_IMMORTAL
         || xIS_SET( ch->act, PLR_ATTACKER ) || xIS_SET( ch->act, PLR_KILLER ) || IN_ARENA( ch ) )
        return;

    // xSET_BIT( ch->act, PLR_ATTACKER );
    save_char_obj( ch );
    return;
}

/* Set position of a victim. */
void update_pos( CHAR_DATA *victim )
{
    if ( !victim ) {
        bug( "%s", "update_pos: null victim" );
        return;
    }

    if ( victim->hit > 0 ) {
        if ( victim->position == POS_STUNNED )
            set_position( victim, POS_STANDING );
        if ( IS_AFFECTED( victim, AFF_PARALYSIS ) )
            set_position( victim, POS_STUNNED );
        return;
    }

    if ( IS_NPC( victim ) || victim->hit <= -11 ) {
        if ( victim->mount ) {
            act( AT_ACTION, "$n falls from $N.", victim, NULL, victim->mount, TO_ROOM );
            xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
            victim->mount = NULL;
            xREMOVE_BIT( victim->affected_by, AFF_MOUNTED );
        }
        set_position( victim, POS_DEAD );
        return;
    }

    if ( victim->hit <= -6 )
        set_position( victim, POS_MORTAL );
    else if ( victim->hit <= -3 )
        set_position( victim, POS_INCAP );
    else
        set_position( victim, POS_STUNNED );

    if ( victim->position > POS_STUNNED && IS_AFFECTED( victim, AFF_PARALYSIS ) )
        set_position( victim, POS_STUNNED );

    if ( victim->mount ) {
        act( AT_ACTION, "$n falls unconscious from $N.", victim, NULL, victim->mount, TO_ROOM );
        xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
        victim->mount = NULL;
    }
}

void set_distance( CHAR_DATA *ch, int distance )
{
    if ( ch->fighting == NULL )
        return;

    ch->fighting->distance = distance;
    if ( ch->fighting->who && ch->fighting->who->fighting
         && ch->fighting->who->fighting->who == ch ) {
        ch->fighting->who->fighting->distance = distance;
    }
}

void do_rush( CHAR_DATA *ch, char *argument )
{
    int                     distance;

    if ( ch->fighting == NULL || ch->fighting->who == NULL ) {
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }
    if ( ch->fighting->distance == MIN_DISTANCE )
        send_to_char( "You can't get any closer!\r\n", ch );
    else {
        distance = ch->fighting->distance - number_chance( 1, 4 );
//    ch->fighting->who->fighting->distance = ch->fighting->distance;
        if ( distance < MIN_DISTANCE )
            distance = MIN_DISTANCE;
        set_distance( ch, distance );

        act( AT_GREEN, "Yelling a battle cry you rush in towards $N.", ch, NULL, ch->fighting->who,
             TO_CHAR );
        act( AT_GREEN, "$n yelling a battle cry rushes in towards you.", ch, NULL,
             ch->fighting->who, TO_VICT );
        act( AT_GREEN, "$n yelling a battle cry rushes in towards $N.", ch, NULL, ch->fighting->who,
             TO_NOTVICT );
        WAIT_STATE( ch, 12 );
    }
}

void do_retreat( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( ( victim = who_fighting( ch ) ) != NULL ) {
        set_distance( ch, ch->fighting->distance + number_range( 1, 4 ) );
//  ch->fighting->who->fighting->distance = ch->fighting->distance;
        act( AT_GREEN, "You move back away from $N.", ch, NULL, ch->fighting->who, TO_CHAR );
        act( AT_GREEN, "$n moves back away from you.", ch, NULL, ch->fighting->who, TO_VICT );
        act( AT_GREEN, "$n moves back away from $N.", ch, NULL, ch->fighting->who, TO_NOTVICT );
        WAIT_STATE( ch, 12 );
    }
    else
        send_to_char( "You aren't fighting anyone.\r\n", ch );
    return;

}

/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    FIGHT_DATA             *fight;

    if ( ch->fighting ) {
        bug( "Set_fighting: %s -> %s (already fighting %s)", ch->name, victim->name,
             ch->fighting->who->name );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_SLEEP ) )
        affect_strip( ch, gsn_sleep );

    /*
     * Limit attackers -Thoric 
     */
    if ( victim->num_fighting > max_fight( victim ) ) {
        send_to_char( "There are too many people fighting for you to join in.\r\n", ch );
        return;
    }

    CREATE( fight, FIGHT_DATA, 1 );
    fight->who = victim;

    if ( in_arena( ch ) )
        fight->xp = 0;
    else
        fight->xp = ( int ) xp_compute( ch, victim ) * 2;
    fight->align = align_compute( ch, victim );
    if ( !IS_NPC( ch ) && IS_NPC( victim ) )
        fight->timeskilled = times_killed( ch, victim );
    ch->num_fighting = 1;
    ch->fighting = fight;
    set_distance( ch, STARTING_DISTANCE );
    /*
     * ch->position = POS_FIGHTING; 
     */
    if ( IS_NPC( ch ) )
        set_position( ch, POS_FIGHTING );
    else
        switch ( ch->style ) {
            case ( STYLE_EVASIVE ):
                set_position( ch, POS_EVASIVE );
                break;
            case ( STYLE_DEFENSIVE ):
                set_position( ch, POS_DEFENSIVE );
                break;
            case ( STYLE_AGGRESSIVE ):
                set_position( ch, POS_AGGRESSIVE );
                break;
            case ( STYLE_BERSERK ):
                set_position( ch, POS_BERSERK );
                break;
            default:
                set_position( ch, POS_FIGHTING );
        }
    victim->num_fighting++;
    if ( victim->switched && IS_AFFECTED( victim->switched, AFF_POSSESS ) ) {
        send_to_char( "You are disturbed!\r\n", victim->switched );
        do_return( victim->switched, ( char * ) "" );
    }
    return;
}

CHAR_DATA              *who_fighting( CHAR_DATA *ch )
{
    if ( !ch ) {
        bug( "%s who_fighting: null ch", __FUNCTION__ );
        return NULL;
    }
    if ( !ch->fighting )
        return NULL;
    return ch->fighting->who;
}

void free_fight( CHAR_DATA *ch )
{
    if ( !ch ) {
        bug( "%s", "Free_fight: null ch!" );
        return;
    }
    if ( ch->fighting ) {
        if ( !char_died( ch->fighting->who ) )
            --ch->fighting->who->num_fighting;
        DISPOSE( ch->fighting );
    }
    ch->fighting = NULL;
    if ( ch->mount )
        set_position( ch, POS_MOUNTED );
    else
        set_position( ch, POS_STANDING );

    // Tangle wears off after combat. -Taon
    if ( IS_AFFECTED( ch, AFF_TANGLED ) ) {
        affect_strip( ch, gsn_entangle );
        set_char_color( AT_WEAROFF, ch );
        send_to_char( skill_table[gsn_entangle]->msg_off, ch );
        send_to_char( "\r\n", ch );
    }

    /*
     * Berserk wears off after combat. -- Altrag 
     */
    if ( IS_AFFECTED( ch, AFF_BERSERK ) ) {
        affect_strip( ch, gsn_berserk );
        set_char_color( AT_WEAROFF, ch );
        send_to_char( skill_table[gsn_berserk]->msg_off, ch );
        send_to_char( "\r\n", ch );
    }
    return;
}

/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA              *fch;

    free_fight( ch );
    update_pos( ch );

    // Strip Berserk, fighting has stopped. -Taon
    if ( IS_AFFECTED( ch, AFF_BERSERK ) ) {
        affect_strip( ch, gsn_berserk );
        xREMOVE_BIT( ch->affected_by, AFF_BERSERK );
    }
    if ( !fBoth )                                      /* major short cut here by Thoric */
        return;

    for ( fch = first_char; fch; fch = fch->next ) {
        if ( who_fighting( fch ) == ch ) {
            free_fight( fch );
            update_pos( fch );
        }
    }
    return;
}

/* Vnums for the various bodyparts */
int                     part_vnums[] = { 12,           /* Head */
    14,                                                /* arms */
    15,                                                /* legs */
    13,                                                /* heart */
    44,                                                /* brains */
    16,                                                /* guts */
    45,                                                /* hands */
    46,                                                /* feet */
    47,                                                /* fingers */
    48,                                                /* ear */
    49,                                                /* eye */
    50,                                                /* long_tongue */
    51,                                                /* eyestalks */
    52,                                                /* tentacles */
    53,                                                /* fins */
    54,                                                /* wings */
    55,                                                /* tail */
    56,                                                /* scales */
    59,                                                /* claws */
    87,                                                /* fangs */
    58,                                                /* horns */
    57,                                                /* tusks */
    55,                                                /* tailattack */
    85,                                                /* sharpscales */
    84,                                                /* beak */
    86,                                                /* haunches */
    83,                                                /* hooves */
    82,                                                /* paws */
    81,                                                /* forelegs */
    80,                                                /* feathers */
    0,                                                 /* r1 */
    0                                                  /* r2 */
};

/* Messages for flinging off the various bodyparts */
const char             *part_messages[] = {
    "$n's severed head plops from its neck.",
    "$n's arm is sliced from $s dead body.",
    "$n's leg is sliced from $s dead body.",
    "$n's heart is torn from $s chest.",
    "$n's brains spill grotesquely from $s head.",
    "$n's guts spill grotesquely from $s torso.",
    "$n's hand is sliced from $s dead body.",
    "$n's foot is sliced from $s dead body.",
    "A finger is sliced from $n's dead body.",
    "$n's ear is sliced from $s dead body.",
    "$n's eye is gouged from its socket.",
    "$n's tongue is torn from $s mouth.",
    "An eyestalk is sliced from $n's dead body.",
    "A tentacle is severed from $n's dead body.",
    "A fin is sliced from $n's dead body.",
    "A wing is severed from $n's dead body.",
    "$n's tail is sliced from $s dead body.",
    "A scale falls from the body of $n.",
    "A claw is torn from $n's dead body.",
    "$n's fangs are torn from $s mouth.",
    "A horn is wrenched from the body of $n.",
    "$n's tusk is torn from $s dead body.",
    "$n's tail is sliced from $s dead body.",
    "A ridged scale falls from the body of $n.",
    "$n's beak is sliced from $s dead body.",
    "$n's haunches are sliced from $s dead body.",
    "A hoof is sliced from $n's dead body.",
    "A paw is sliced from $n's dead body.",
    "$n's foreleg is sliced from $s dead body.",
    "Some feathers fall from $n's dead body.",
    "r1 message.",
    "r2 message."
};

/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)  
 * Support for additional bodyparts by Fireblade
 */
void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA        *was_in_room;
    const char             *msg;
    EXIT_DATA              *pexit;
    int                     vnum,
                            index,
                            i;

    if ( !ch ) {
        bug( "%s", "DEATH_CRY: null ch!" );
        return;
    }

    vnum = 0;
    msg = NULL;

    switch ( number_chance( 0, 5 ) ) {
        default:
            msg = "You hear $n's death cry.";
            break;
        case 0:
            msg = "$n screams furiously as $e falls to the ground in a heap!";
            break;
        case 1:
            msg = "$n hits the ground ... DEAD.";
            break;
        case 2:
            msg = "$n catches $s guts in $s hands as they pour through $s fatal" " wound!";
            break;
        case 3:
            msg = "$n splatters blood on your armor.";
            break;
        case 4:
            msg = "$n gasps $s last breath and blood spurts out of $s " "mouth and ears.";
            break;

        case 5:
            /*
             * Why waste time if it isn't needed? -Orion
             */
            if ( xIS_EMPTY( ch->xflags ) ) {
                msg = "You hear $n's death cry.";
                break;
            }

            index = number_range( 0, MAX_PARTS - 1 );

            for ( i = 0; i < MAX_PARTS; i++ ) {
                if ( HAS_BODYPART( ch, index ) ) {
                    msg = part_messages[index];
                    vnum = part_vnums[index];
                    break;
                }
                else {
                    index = number_range( 0, MAX_PARTS - 1 );
                }
            }

            if ( !msg )
                msg = "You hear $n's death cry.";
            break;
    }

    act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum ) {
        char                    buf[MSL];
        OBJ_DATA               *obj;
        char                   *name;

        if ( !get_obj_index( vnum ) ) {
            bug( "%s", "death_cry: invalid vnum" );
            return;
        }

        name = IS_NPC( ch ) ? ch->short_descr : ch->name;
        obj = create_object( get_obj_index( vnum ), 0 );
        obj->timer = number_chance( 4, 7 );
        if ( IS_AFFECTED( ch, AFF_POISON ) )
            obj->value[3] = 10;

        snprintf( buf, MSL, obj->short_descr, name );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );

        snprintf( buf, MSL, obj->description, name );
        STRFREE( obj->description );
        obj->description = STRALLOC( buf );

        obj = obj_to_room( obj, ch->in_room );
    }

    if ( IS_NPC( ch ) )
        msg = "You hear something's death cry.";
    else
        msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( pexit = was_in_room->first_exit; pexit; pexit = pexit->next ) {
        if ( pexit->to_room && pexit->to_room != was_in_room ) {
            ch->in_room = pexit->to_room;
            act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );
        }
    }
    ch->in_room = was_in_room;

    return;
}

void raw_kill( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char                    buf[MSL];
    char                    arg[MSL];
    char                    buf2[MSL];

    if ( !victim ) {
        bug( "%s", "raw_kill: null victim!" );
        return;
    }

    /*
     * backup in case hp goes below 1 
     */
    if ( !IS_NPC( victim ) && victim->level < 2 ) {
        bug( "%s", "raw_kill: killing someone still in tutorial." );
        return;
    }

    /*
     * Take care of morphed characters
     */
    if ( victim->morph ) {
        do_unmorph_char( victim );
        save_char_obj( victim );
    }

    if ( IS_AFFECTED( victim, AFF_DRAGONLORD ) || victim->Class == CLASS_DRAGONLORD ) {
        affect_strip( victim, gsn_human_form );
        xREMOVE_BIT( victim->affected_by, AFF_DRAGONLORD );
        victim->hit = number_range( 10, 50 );
        save_char_obj( victim );
    }

    if ( IS_AFFECTED( victim, AFF_SHRINK ) ) {
        affect_strip( victim, gsn_shrink );
        xREMOVE_BIT( victim->affected_by, AFF_SHRINK );
    }

    if ( xIS_SET( victim->affected_by, AFF_MOUNTED ) ) {
        do_dismount( victim, ( char * ) "" );
        save_char_obj( victim );
    }

    if ( IS_NPC( victim ) ) {
        if ( victim->pIndexData->vnum == MOB_VNUM_SOLDIERS
             || victim->pIndexData->vnum == MOB_VNUM_ARCHERS ) {
            short                   rand;

            rand = number_range( 1, 10 );
            if ( rand > 6 ) {
                send_to_char
                    ( "&WYou gain one glory for killing the enemy that is trying to raid your homeland!\r\n",
                      ch );
                ch->quest_curr += 1;
            }
            else {
                gain_exp( ch, number_range( 3000, 8000 ) );
            }

            if ( ch->pcdata->city ) {
                CITY_DATA              *city = NULL;

                // Check which city to remove a point from
                if ( ch->in_room->vnum == PALEON_DEFEND || ch->in_room->vnum == PALEON_DEFEND2
                     || ch->in_room->vnum == PALEON_DEFEND3 ) {
                    city = get_city( "Paleon City" );
                    if ( city->guards > 0 && city->defense > 0 ) {
                        city->guards -= 1;
                        city->defense -= 1;
                    }
                    save_city( city );
                }
                else if ( ch->in_room->vnum == FORBIDDEN_DEFEND
                          || ch->in_room->vnum == FORBIDDEN_DEFEND2
                          || ch->in_room->vnum == FORBIDDEN_DEFEND3 ) {
                    city = get_city( "Forbidden City" );
                    if ( city->guards > 0 && city->defense > 0 ) {
                        city->guards -= 1;
                        city->defense -= 1;
                    }
                    save_city( city );
                }
                else if ( ch->in_room->vnum == DAKAR_DEFEND || ch->in_room->vnum == DAKAR_DEFEND2
                          || ch->in_room->vnum == DAKAR_DEFEND3 ) {
                    city = get_city( "Dakar City" );
                    if ( city->guards > 0 && city->defense > 0 ) {
                        city->guards -= 1;
                        city->defense -= 1;
                    }
                    save_city( city );
                }
                else if ( ch->in_room->vnum == DAKAR_SIEGE || ch->in_room->vnum == DAKAR_SIEGE2
                          || ch->in_room->vnum == DAKAR_SIEGE3 ) {
                    city = get_city( ch->pcdata->city_name );
                    if ( city->soldiers > 0 && city->offense > 0 ) {
                        city->soldiers -= 1;
                        city->offense -= 1;
                    }
                    save_city( city );
                }
                else if ( ch->in_room->vnum == FORBIDDEN_SIEGE
                          || ch->in_room->vnum == FORBIDDEN_SIEGE2
                          || ch->in_room->vnum == FORBIDDEN_SIEGE3 ) {
                    city = get_city( ch->pcdata->city_name );
                    if ( city->soldiers > 0 && city->offense > 0 ) {
                        city->soldiers -= 1;
                        city->offense -= 1;
                    }
                    save_city( city );
                }
                else if ( ch->in_room->vnum == PALEON_SIEGE || ch->in_room->vnum == PALEON_SIEGE2
                          || ch->in_room->vnum == PALEON_SIEGE3 ) {
                    city = get_city( ch->pcdata->city_name );
                    if ( city->soldiers > 0 && city->offense > 0 ) {
                        city->soldiers -= 1;
                        city->offense -= 1;
                    }
                    save_city( city );
                }
            }
        }
    }
    if ( !IS_NPC( victim ) && ( victim->pcdata->tmpcrawl > victim->height ) ) {
        victim->height = victim->pcdata->tmpcrawl;
        save_char_obj( victim );
    }
    // idea of Fya that we need to draw players more into non-wilderness, by giving cash
    // -
    // Vladaar

    if ( IS_NPC( victim ) && ( !IN_WILDERNESS( ch ) && !IN_RIFT( ch ) ) ) {
        if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
            GET_MONEY( victim, CURR_GOLD ) += number_range( 1, 20 );
        }
        if ( victim->level > 2 && victim->level < 20 && !xIS_SET( ch->act, PLR_EXTREME ) )
            GET_MONEY( victim, CURR_BRONZE ) += number_range( 1, 20 );
        else if ( victim->level > 19 && victim->level < 60 && !xIS_SET( ch->act, PLR_EXTREME ) )
            GET_MONEY( victim, CURR_SILVER ) += number_range( 1, 20 );
        else if ( victim->level > 59 && victim->level < 100 && !xIS_SET( ch->act, PLR_EXTREME ) )
            GET_MONEY( victim, CURR_GOLD ) += number_range( 1, 20 );
    }

    if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) ) {
        short                   sound;

        sound = number_range( 1, 10 );

        if ( sound < 6 )
            send_to_char( "!!SOUND(sound/deathcry1.wav)\r\n", ch );
        else if ( sound > 5 )
            send_to_char( "!!SOUND(sound/deathcry2.wav)\r\n", ch );
    }

    if ( !IS_NPC( victim ) && ( victim->height < victim->pcdata->tmpheight ) ) {
        short                   ability;

        victim->max_hit = victim->pcdata->tmpmax_hit;
        victim->height = victim->pcdata->tmpheight;
        victim->weight = victim->pcdata->tmpweight;
        victim->race = victim->pcdata->tmprace;
        victim->Class = victim->pcdata->tmpclass;
        ability = skill_lookup( "cone" );
        victim->pcdata->learned[ability] = 0;
        ability = skill_lookup( "fly" );
        victim->pcdata->learned[ability] = 0;
        ability = skill_lookup( "defend" );
        victim->pcdata->learned[ability] = 0;
        save_char_obj( victim );
    }

    /*
     * Deal with switched imms
     */
    if ( victim->desc && victim->desc->original ) {
        CHAR_DATA              *temp = victim;

        interpret( victim, ( char * ) "return" );
        save_char_obj( victim );
    }

    stop_summoning( victim );

    stop_fighting( victim, TRUE );

    // Clan members gain points for the clan with kills.
    if ( !IS_NPC( ch ) ) {
        CLAN_DATA              *clan;

        if ( IS_NPC( victim ) ) {
            if ( ch->pcdata->clan && victim->level >= ch->level ) {
                ch->pcdata->clanpoints += 1;
                ch->pcdata->clan->totalpoints += 1;
                clan = ch->pcdata->clan;
                ch_printf( ch,
                           "&G%s clan has gained a point from your kill, now totaling %d clan status points!\r\n",
                           clan->name, clan->totalpoints );
                save_clan( ch->pcdata->clan );
                save_char_obj( ch );
            }
            ch->pcdata->mkills++;
            if ( ch->pcdata->clan )
                ch->pcdata->clan->mkills++;
            ch->in_room->area->mkills++;
        }
        if ( !IS_NPC( victim ) && victim != ch ) {
            ch->pcdata->clanpoints += 3;

            if ( ch->pcdata->clan ) {
                ch->pcdata->clan->totalpoints += 3;
                clan = ch->pcdata->clan;
                ch_printf( ch,
                           "&G%s clan has gained 3 points from your kill, now totaling %d clan status points!\r\n",
                           clan->name, clan->totalpoints );
                if ( countpkill( ch, victim ) )
                    increase_clan_pkills( ch->pcdata->clan, ch->level );
                save_clan( clan );
            }
            if ( victim->pcdata->clan && countpkill( ch, victim ) ) {
                increase_clan_pdeaths( victim->pcdata->clan, victim->level );
                save_clan( victim->pcdata->clan );
            }
            ch->pcdata->pkills++;
            victim->pcdata->pdeaths++;
            add_pkill( ch, victim );
            save_char_obj( ch );
        }
    }

/*  Not sure where else to put the announcement - Volk  */

    if ( !IS_NPC( victim ) ) {
        if ( victim != ch )
            snprintf( buf, MSL, "%s has been killed by %s!", victim->name,
                      ( IS_NPC( ch ) ? ch->short_descr : ch->name ) );
        else
            snprintf( buf, MSL, "%s has been killed!", victim->name );
        announce( buf );

        if ( victim != ch )
            snprintf( buf, MSL, "%s (%d) killed by %s at %d", victim->name, victim->level,
                      ( IS_NPC( ch ) ? ch->short_descr : ch->name ), victim->in_room->vnum );
        else
            snprintf( buf, MSL, "%s (%d) bled to death at %d", victim->name, victim->level,
                      victim->in_room->vnum );
        log_string( buf );
        to_channel( buf, "Monitor", LEVEL_IMMORTAL );

    }

    mprog_death_trigger( ch, victim );
    if ( char_died( victim ) )
        return;

    rprog_death_trigger( ch, victim );
    if ( char_died( victim ) )
        return;

    if ( IS_NPC( victim ) )
        make_corpse( victim, ch );

    if ( victim->in_room->sector_type == SECT_OCEANFLOOR
         || victim->in_room->sector_type == SECT_UNDERWATER
         || victim->in_room->sector_type == SECT_WATER_SWIM
         || victim->in_room->sector_type == SECT_WATER_NOSWIM )
        act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, NULL, NULL,
             TO_ROOM );
    else if ( victim->in_room->sector_type == SECT_AIR )
        act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, NULL, NULL, TO_ROOM );
    else
        make_blood( victim );

    if ( IS_NPC( victim ) ) {
        victim->pIndexData->killed++;
        extract_char( victim, TRUE );
        victim = NULL;
        return;
    }

    set_char_color( AT_DIEMSG, victim );

    if ( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
        do_help( victim, ( char * ) "new_death" );
    else
        do_help( victim, ( char * ) "_DIEMSG_" );
    make_corpse( victim, ch );

    strcpy( arg, victim->name );

    extract_char( victim, FALSE );

    if ( !victim ) {
        bug( "%s", "oops! raw_kill: extract_char destroyed pc char" );
        return;
    }
    while ( victim->first_affect )
        affect_remove( victim, victim->first_affect );
    victim->affected_by = race_table[victim->race]->affected;
    victim->resistant = 0;
    victim->susceptible = 0;
    victim->immune = 0;
    victim->carry_weight = 0;
    victim->armor = 100;
    victim->armor += race_table[victim->race]->ac_plus;
    victim->attacks = race_table[victim->race]->attacks;
    victim->defenses = race_table[victim->race]->defenses;
    victim->mod_str = 0;
    victim->mod_dex = 0;
    victim->mod_wis = 0;
    victim->mod_int = 0;
    victim->mod_con = 0;
    victim->mod_cha = 0;
    victim->mod_lck = 0;
    victim->damroll = 0;
    victim->hitroll = 0;
    victim->mental_state = -10;
    victim->alignment = URANGE( -1000, victim->alignment, 1000 );

    victim->saving_poison_death = race_table[victim->race]->saving_poison_death;
    victim->saving_wand = race_table[victim->race]->saving_wand;
    victim->saving_para_petri = race_table[victim->race]->saving_para_petri;
    victim->saving_breath = race_table[victim->race]->saving_breath;
    victim->saving_spell_staff = race_table[victim->race]->saving_spell_staff;
    victim->position = POS_RESTING;
    victim->hit = UMAX( 1, victim->hit );
    /*
     * Shut down some of those naked spammer killers - Blodkai 
     */
    if ( !IS_AVATAR( ch ) && !IS_DUALAVATAR( ch ) && !IS_TRIAVATAR( ch ) )
        victim->mana = UMAX( 1, victim->mana );
    else
        victim->mana = 1;
    victim->move = UMAX( 1, victim->move );

    /*
     * Pardon crimes...  -Thoric 
     */
    if ( xIS_SET( victim->act, PLR_KILLER ) ) {
        xREMOVE_BIT( victim->act, PLR_KILLER );
        send_to_char( "The gods have pardoned you for your murderous acts.\r\n", victim );
    }
    if ( xIS_SET( victim->act, PLR_THIEF ) ) {
        xREMOVE_BIT( victim->act, PLR_THIEF );
        send_to_char( "The gods have pardoned you for your thievery.\r\n", victim );
    }
    victim->pcdata->condition[COND_FULL] = 12;
    victim->pcdata->condition[COND_THIRST] = 12;
    if ( IS_BLOODCLASS( victim ) )
        victim->blood = ( victim->level / 2 );
    if ( xIS_SET( sysdata.save_flags, SV_DEATH ) )
        save_char_obj( victim );
}

void align_zap( CHAR_DATA *ch )
{
    OBJ_DATA               *obj,
                           *obj_next;

    for ( obj = ch->first_carrying; obj; obj = obj_next ) {
        obj_next = obj->next_content;
        if ( obj->wear_loc == WEAR_NONE )
            continue;

        if ( ( IS_OBJ_STAT( obj, ITEM_ANTI_EVIL ) && IS_EVIL( ch ) )
             || ( IS_OBJ_STAT( obj, ITEM_ANTI_GOOD ) && IS_GOOD( ch ) )
             || ( IS_OBJ_STAT( obj, ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL( ch ) ) ) {
            act( AT_MAGIC, "You are zapped by $p.", ch, obj, NULL, TO_CHAR );
            act( AT_MAGIC, "$n is zapped by $p.", ch, obj, NULL, TO_ROOM );
            obj_from_char( obj );
            obj = obj_to_room( obj, ch->in_room );
            oprog_zap_trigger( ch, obj );              /* mudprogs */
            if ( char_died( ch ) )
                return;
        }
    }
}

/* The new and improved mob xp_flag formula. Data used is according to information from Dwip.
 * Sue him if this isn't working right :)
 * This is gonna be one big, ugly set of ifchecks.
 * Samson 5-18-01, Modified by Volk 04-Sep-06
 * This will now take an input experience and mob, and output modified experience for that mob.
 * Renamed to 'ExpRatio' and highly modified.
 */
int ExpRatio( CHAR_DATA *mob, int xp )
{
    int                     flags = 0;

    if ( mob->armor < 0 )
        flags++;

    if ( mob->numattacks > 4 )
        flags += 2;

    if ( IS_ACT_FLAG( mob, ACT_AGGRESSIVE ) || IS_ACT_FLAG( mob, ACT_WILD_AGGR ) )
        flags++;

    if ( IS_ACT_FLAG( mob, ACT_WIMPY ) )
        flags -= 1;

    if ( IS_AFFECTED( mob, AFF_DETECT_INVIS ) || IS_AFFECTED( mob, AFF_DETECT_HIDDEN )
         || IS_AFFECTED( mob, AFF_TRUESIGHT ) )
        flags++;

    if ( IS_AFFECTED( mob, AFF_SANCTUARY ) )
        flags++;

    if ( IS_AFFECTED( mob, AFF_SNEAK ) || IS_AFFECTED( mob, AFF_HIDE )
         || IS_AFFECTED( mob, AFF_INVISIBLE ) )
        flags++;

    if ( IS_AFFECTED( mob, AFF_FIRESHIELD ) )
        flags++;

    if ( IS_AFFECTED( mob, AFF_SHOCKSHIELD ) )
        flags++;

    if ( IS_AFFECTED( mob, AFF_ICESHIELD ) )
        flags++;

    if ( IS_AFFECTED( mob, AFF_VENOMSHIELD ) )
        flags += 2;

    if ( IS_AFFECTED( mob, AFF_ACIDMIST ) )
        flags += 2;
/*
   if( IS_AFFECTED( mob, AFF_BLADEBARRIER ) )
      flags += 2;
*/
    if ( IS_RESIS( mob, RIS_FIRE ) || IS_RESIS( mob, RIS_COLD ) || IS_RESIS( mob, RIS_ELECTRICITY )
         || IS_RESIS( mob, RIS_ENERGY ) || IS_RESIS( mob, RIS_ACID ) )
        flags++;

    if ( IS_RESIS( mob, RIS_BLUNT ) || IS_RESIS( mob, RIS_SLASH ) || IS_RESIS( mob, RIS_PIERCE )
         || IS_RESIS( mob, RIS_HACK ) || IS_RESIS( mob, RIS_LASH ) )
        flags++;

    if ( IS_RESIS( mob, RIS_SLEEP ) || IS_RESIS( mob, RIS_CHARM ) || IS_RESIS( mob, RIS_HOLD )
         || IS_RESIS( mob, RIS_POISON ) || IS_RESIS( mob, RIS_PARALYSIS ) )
        flags++;

    if ( IS_RESIS( mob, RIS_PLUS1 ) || IS_RESIS( mob, RIS_PLUS2 ) || IS_RESIS( mob, RIS_PLUS3 )
         || IS_RESIS( mob, RIS_PLUS4 ) || IS_RESIS( mob, RIS_PLUS5 ) || IS_RESIS( mob, RIS_PLUS6 ) )
        flags++;

    if ( IS_RESIS( mob, RIS_MAGIC ) )
        flags += 2;

    if ( IS_RESIS( mob, RIS_NONMAGIC ) )
        flags++;

    if ( IS_IMMUNE( mob, RIS_FIRE ) || IS_IMMUNE( mob, RIS_COLD )
         || IS_IMMUNE( mob, RIS_ELECTRICITY ) || IS_IMMUNE( mob, RIS_ENERGY )
         || IS_IMMUNE( mob, RIS_ACID ) )
        flags += 2;

    if ( IS_IMMUNE( mob, RIS_BLUNT ) || IS_IMMUNE( mob, RIS_SLASH ) || IS_IMMUNE( mob, RIS_PIERCE )
         || IS_IMMUNE( mob, RIS_HACK ) || IS_IMMUNE( mob, RIS_LASH ) )
        flags += 2;

    if ( IS_IMMUNE( mob, RIS_SLEEP ) || IS_IMMUNE( mob, RIS_CHARM ) || IS_IMMUNE( mob, RIS_HOLD )
         || IS_IMMUNE( mob, RIS_POISON ) || IS_IMMUNE( mob, RIS_PARALYSIS ) )
        flags += 2;

    if ( IS_IMMUNE( mob, RIS_PLUS1 ) || IS_IMMUNE( mob, RIS_PLUS2 ) || IS_IMMUNE( mob, RIS_PLUS3 )
         || IS_IMMUNE( mob, RIS_PLUS4 ) || IS_IMMUNE( mob, RIS_PLUS5 )
         || IS_IMMUNE( mob, RIS_PLUS6 ) )
        flags += 2;

    if ( IS_IMMUNE( mob, RIS_MAGIC ) )
        flags += 2;

    if ( IS_IMMUNE( mob, RIS_NONMAGIC ) )
        flags++;

    if ( IS_SUSCEP( mob, RIS_FIRE ) || IS_SUSCEP( mob, RIS_COLD )
         || IS_SUSCEP( mob, RIS_ELECTRICITY ) || IS_SUSCEP( mob, RIS_ENERGY )
         || IS_SUSCEP( mob, RIS_ACID ) )
        flags -= 2;

    if ( IS_SUSCEP( mob, RIS_BLUNT ) || IS_SUSCEP( mob, RIS_SLASH ) || IS_SUSCEP( mob, RIS_PIERCE )
         || IS_SUSCEP( mob, RIS_HACK ) || IS_SUSCEP( mob, RIS_LASH ) )
        flags -= 3;

    if ( IS_SUSCEP( mob, RIS_SLEEP ) || IS_SUSCEP( mob, RIS_CHARM ) || IS_SUSCEP( mob, RIS_HOLD )
         || IS_SUSCEP( mob, RIS_POISON ) || IS_SUSCEP( mob, RIS_PARALYSIS ) )
        flags -= 3;

    if ( IS_SUSCEP( mob, RIS_PLUS1 ) || IS_SUSCEP( mob, RIS_PLUS2 ) || IS_SUSCEP( mob, RIS_PLUS3 )
         || IS_SUSCEP( mob, RIS_PLUS4 ) || IS_SUSCEP( mob, RIS_PLUS5 )
         || IS_SUSCEP( mob, RIS_PLUS6 ) )
        flags -= 2;

    if ( IS_SUSCEP( mob, RIS_MAGIC ) )
        flags -= 3;

    if ( IS_SUSCEP( mob, RIS_NONMAGIC ) )
        flags -= 2;
/*
   if( IS_ABSORB( mob, RIS_FIRE ) || IS_ABSORB( mob, RIS_COLD ) || IS_ABSORB( mob, RIS_ELECTRICITY )
       || IS_ABSORB( mob, RIS_ENERGY ) || IS_ABSORB( mob, RIS_ACID ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_BLUNT ) || IS_ABSORB( mob, RIS_SLASH ) || IS_ABSORB( mob, RIS_PIERCE )
       || IS_ABSORB( mob, RIS_HACK ) || IS_ABSORB( mob, RIS_LASH ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_SLEEP ) || IS_ABSORB( mob, RIS_CHARM ) || IS_ABSORB( mob, RIS_HOLD )
    || IS_ABSORB( mob, RIS_POISON ) || IS_ABSORB( mob, RIS_PARALYSIS ) )
	flags += 3;

   if( IS_ABSORB( mob, RIS_PLUS1 ) || IS_ABSORB( mob, RIS_PLUS2 ) || IS_ABSORB( mob, RIS_PLUS3 )
       || IS_ABSORB( mob, RIS_PLUS4 ) || IS_ABSORB( mob, RIS_PLUS5 ) || IS_ABSORB( mob, RIS_PLUS6 ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_MAGIC ) )
      flags += 3;

   if( IS_ABSORB( mob, RIS_NONMAGIC ) )
      flags += 2;
*/
    if ( IS_ATTACK( mob, ATCK_BASH ) )
        flags++;

    if ( IS_ATTACK( mob, ATCK_STUN ) )
        flags++;

    if ( IS_ATTACK( mob, ATCK_EBOLT ) )
        flags += 3;

    if ( IS_ATTACK( mob, ATCK_BLIGHTNING ) || IS_ATTACK( mob, ATCK_LTOUCH ) )
        flags += 3;

    if ( IS_ATTACK( mob, ATCK_BACKSTAB ) )
        flags += 2;

    if ( IS_ATTACK( mob, ATCK_FIREBREATH ) || IS_ATTACK( mob, ATCK_FROSTBREATH )
         || IS_ATTACK( mob, ATCK_ACIDBREATH ) || IS_ATTACK( mob, ATCK_LIGHTNBREATH )
         || IS_ATTACK( mob, ATCK_GASBREATH ) || IS_ATTACK( mob, ATCK_GSMITE ) )
        flags += 2;

    if ( IS_ATTACK( mob, ATCK_EARTHQUAKE ) || IS_ATTACK( mob, ATCK_FIREBALL )
         || IS_ATTACK( mob, ATCK_COLORSPRAY ) )
        flags += 2;

    if ( IS_ATTACK( mob, ATCK_SPIRALBLAST ) )
        flags += 2;

    if ( IS_ATTACK( mob, ATCK_BITE ) || IS_ATTACK( mob, ATCK_CLAWS )
         || IS_ATTACK( mob, ATCK_TAIL_SWIPE ) || IS_ATTACK( mob, ATCK_STING )
         || IS_ATTACK( mob, ATCK_PUNCH ) || IS_ATTACK( mob, ATCK_KICK )
         || IS_ATTACK( mob, ATCK_TRIP ) || IS_ATTACK( mob, ATCK_GOUGE )
         || IS_ATTACK( mob, ATCK_DRAIN ) || IS_ATTACK( mob, ATCK_POISON )
         || IS_ATTACK( mob, ATCK_NASTYPOISON ) || IS_ATTACK( mob, ATCK_GAZE )
         || IS_ATTACK( mob, ATCK_BLINDNESS ) || IS_ATTACK( mob, ATCK_CAUSESERIOUS )
         || IS_ATTACK( mob, ATCK_CAUSECRITICAL ) || IS_ATTACK( mob, ATCK_CURSE )
         || IS_ATTACK( mob, ATCK_FLAMESTRIKE ) || IS_ATTACK( mob, ATCK_HARM )
         || IS_ATTACK( mob, ATCK_WEAKEN ) || IS_ATTACK( mob, ATCK_HHANDS ) )
        flags++;

    if ( IS_DEFENSE( mob, DFND_PARRY ) )
        flags++;

    if ( IS_DEFENSE( mob, DFND_DODGE ) )
        flags++;

    if ( IS_DEFENSE( mob, DFND_DISPELEVIL ) )
        flags++;

    if ( IS_DEFENSE( mob, DFND_DISPELMAGIC ) )
        flags++;

    if ( IS_DEFENSE( mob, DFND_DISARM ) )
        flags++;

    if ( IS_DEFENSE( mob, DFND_SANCTUARY ) )
        flags += 2;

    if ( IS_DEFENSE( mob, DFND_HEAL ) || IS_DEFENSE( mob, DFND_SHIELD )
         || IS_DEFENSE( mob, DFND_BLESS ) || IS_DEFENSE( mob, DFND_STONESKIN )
         || IS_DEFENSE( mob, DFND_SWARD )
         || IS_DEFENSE( mob, DFND_TELEPORT ) || IS_DEFENSE( mob, DFND_GRIP )
         || IS_DEFENSE( mob, DFND_CELEMENTAL )
         || IS_DEFENSE( mob, DFND_TRUESIGHT ) || IS_DEFENSE( mob, DFND_DBLESS )
         || IS_DEFENSE( mob, DFND_GHEAL ) || IS_DEFENSE( mob, DFND_ACOMPANION )
         || IS_DEFENSE( mob, DFND_LSKELLIE ) )
        flags++;

    /*
     * Gotta do this because otherwise mobs with negative flags will take xp AWAY from the player. 
     */
    if ( flags < 0 )
        flags = 0;

    /*
     * And cap all mobs to no more than 12 flags to keep xp from going haywire 
     */
    if ( flags > 12 )
        flags = 12;

    /*
     * At 12 flags, we would double the experience (100%). At 0 flags, do nothing. 
     */

    int                     modxp = xp * flags / 5;

    return modxp;
}

void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA              *gch,
                           *gch_next,
                           *lch;
    int                     xp = 0,
        max_level = 0,
        members = 0;

    /*
     * Monsters don't get kill xp's or alignment changes ( exception: charmies )
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( victim == ch )
        return;

    /*
     * We hope this works of course 
     */
    if ( IS_NPC( ch ) ) {
        if ( ch->leader ) {
            if ( !IS_NPC( ch->leader ) && ch->leader == ch->master && IS_AFFECTED( ch, AFF_CHARM )
                 && ch->in_room == ch->leader->in_room )
                ch = ch->master;
        }
    }

    /*
     * See above. If this is STILL an NPC after that, then yes, we need to bail out now. 
     */
    if ( IS_NPC( ch ) )
        return;

    members = 0;
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room ) {
        if ( !is_same_group( gch, ch ) )
            continue;
        /*
         * Count members only if they're PCs so charmies don't dillute the kill 
         */
        if ( !IS_NPC( gch ) ) {
            members++;
            max_level = UMAX( max_level, gch->level );
        }
    }
    if ( members == 0 ) {
        bug( "%s", "Group_gain: members." );
        members = 1;
    }

    lch = ch->leader ? ch->leader : ch;
    for ( gch = ch->in_room->first_person; gch; gch = gch_next ) {
        gch_next = gch->next_in_room;

        if ( !is_same_group( gch, ch ) )
            continue;
        xp = ( int ) ( xp_compute( gch, victim ) * 0.1765 ) / members;
        if ( !gch->fighting )
            xp /= 2;
        gch->alignment = align_compute( gch, victim );
        /*
         * class_monitor( gch ); 
         */
        if ( !IS_NPC( victim ) || !xIS_SET( victim->act, ACT_IMMORTAL ) )
            gain_exp( gch, xp );                       /* group gain */
        align_zap( gch );
    }
    return;
}

int align_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
    int                     align,
                            newalign,
                            divalign;

    align = ( gch->alignment - victim->alignment );

    /*
     * slowed movement in good & evil ranges by a factor of 5, h 
     * Added divalign to keep neutral chars shifting faster -- Blodkai 
     * This is obviously gonna take a lot more thought 
     */

    if ( gch->alignment > -350 && gch->alignment < 350 )
        divalign = 4;
    else
        divalign = 20;

    if ( align > 500 )
        newalign = UMIN( gch->alignment + ( align - 500 ) / divalign, 1000 );
    else if ( align < -500 )
        newalign = UMAX( gch->alignment + ( align + 500 ) / divalign, -1000 );
    else
        newalign = gch->alignment - ( int ) ( gch->alignment / divalign );

    return newalign;
}

/*
 * Calculate how much XP gch should gain for killing victim
 * Lots of redesigning for new exp system by Thoric
 *
*/
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
    int                     xp;
    int                     gchlev = gch->level;

//   xp = ( get_exp_worth( victim ) * URANGE( 0, ( victim->level - gchlev ) + 15, 20 ) ) / 20;
/*  This URANGE - return b if between a or c, otherwise return one of those two. */
    xp = ( get_exp_worth( victim ) * URANGE( 0, ( victim->level - gchlev ) + 21, 10 ) ) / 10;

    if ( gch->race == RACE_DRAGON && ( victim->level - gchlev < 20 ) && xp < 1000 ) {
        xp = number_range( 8000, 15000 );
    }

/*  In affect, this should allow players to fight within 20 levels of their own (+21). It
 *  caps at 10, so you don't gain too much for killing at your own level (or over), but is only
 *  divide by 10 too (so potential to gain more experience than normal for lower/higher mobs) */

    /*
     * get 1/4 exp for players               -Thoric 
     */
    if ( !IS_NPC( victim ) )
        xp /= 4;

    /*
     * reduce exp for killing the same mob repeatedly    -Thoric 
     */
    if ( !IS_NPC( gch ) && IS_NPC( victim ) ) {
        int                     times = times_killed( gch, victim );

        if ( times >= 10 )
            xp = 0;
        else if ( times ) {
            xp = ( xp * ( 20 - times ) ) / 20;
            if ( times > 8 )
                xp /= 3;
            else if ( times > 5 )
                xp >>= 1;
        }
    }

/*  Volk - After all that, throw experience through ExpRatio (to give more
 * XP for stronger mobs) then cap it off to ensure players don't gain too much */
    xp = ExpRatio( victim, xp );
    xp /= 3;
    return URANGE( 0, xp, exp_level( gch, gchlev + 1 ) - exp_level( gch, gchlev ) );
}

/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
void new_dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, unsigned int dt, OBJ_DATA *obj )
{
    char                    buf1[256],
                            buf2[256],
                            buf3[256];
    const char             *vs;
    const char             *vp;
    const char             *attack;
    char                    punct;
    short                   dampc;
    struct skill_type      *skill = NULL;
    bool                    gcflag = FALSE;
    bool                    gvflag = FALSE;
    int                     d_index,
                            w_index;
    ROOM_INDEX_DATA        *was_in_room;
    short                   chance;

    if ( !dam )
        dampc = 0;
    else
        dampc =
            ( ( dam * 1000 ) / victim->max_hit ) + ( 50 -
                                                     ( ( victim->hit * 50 ) / victim->max_hit ) );

    if ( ch->in_room != victim->in_room ) {
        was_in_room = ch->in_room;
        char_from_room( ch );
        char_to_room( ch, victim->in_room );
    }
    else
        was_in_room = NULL;

    if ( dt > 0 && dt < ( unsigned int ) top_sn )
        w_index = 0;
    else if ( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
        w_index = ( dt - TYPE_HIT );
    else {
//      bug ( "Dam_message: bad dt %d from %s in %d.", dt, ch->name, ch->in_room->vnum );
        dt = TYPE_HIT;
        w_index = 0;
    }

    /*
     * get the damage index 
     */
    if ( dam == 0 )
        d_index = 0;
    else if ( dampc < 0 )
        d_index = 1;
    else if ( dampc <= 100 )
        d_index = 1 + dampc / 10;
    else if ( dampc <= 200 )
        d_index = 11 + ( dampc - 100 ) / 20;
    else if ( dampc <= 900 )
        d_index = 16 + ( dampc - 200 ) / 100;
    else
        d_index = 23;

    /*
     * Lookup the damage message 
     */
    vs = s_message_table[w_index][d_index];
    vp = p_message_table[w_index][d_index];

/* Volk: Are damage messages ALWAYS in plural?? Why no VS messages?
 * Maybe a dirty hack, but have something to check the last letter
 * of the name or attack of the player? If it's an s, use vs the
 * singular form (ie Your swords MASSACRE person!), or else use vp
 * the plural form (ie Your pitchfork MASSACRES person!). */

    punct = ( dampc <= 30 ) ? '.' : '!';

    if ( dam == 0 && ( !IS_NPC( ch ) && ( IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) ) )
        gcflag = TRUE;

    if ( dam == 0 && ( !IS_NPC( victim ) && ( IS_SET( victim->pcdata->flags, PCFLAG_GAG ) ) ) )
        gvflag = TRUE;

    if ( dt >= 0 && dt < ( int unsigned ) top_sn )
        skill = skill_table[dt];

    if ( IS_AFFECTED( victim, AFF_PRAYER ) ) {
        send_to_char( "You come to an end in your prayer and slowly open your eyes.\r\n", victim );
        affect_strip( victim, gsn_prayer );
        xREMOVE_BIT( victim->affected_by, AFF_PRAYER );
    }

    chance = number_range( 1, 100 );

    if ( ch->success_attack >= 1 ) {
        if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
            send_to_char( "!!SOUND(sound/playerhit.wav)\r\n", ch );
    }
    else if ( ch->success_attack < 1 ) {
        if ( xIS_SET( ch->act, PLR_SOUND ) || xIS_SET( ch->act, PLR_MUSIC ) )
            send_to_char( "!!SOUND(sound/miss.wav)\r\n", ch );
    }

    if ( dt == TYPE_HIT && ( !IS_NPC( ch ) && ( ch->pcdata->learned[gsn_martial_arts] > 0 ) ) ) {
        if ( chance <= 10 ) {
            snprintf( buf1, 256, "$n's elbow %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your elbow %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's elbow %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else if ( chance <= 30 ) {
            snprintf( buf1, 256, "$n's palmheel attack %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your palmheel attack %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's palmheel attack %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else if ( chance <= 40 ) {
            snprintf( buf1, 256, "$n's sidekick %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your sidekick %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's sidekick %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else if ( chance <= 50 ) {
            snprintf( buf1, 256, "$n's knifehand strike %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your knifehand strike %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's knifehand strike %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else if ( chance <= 60 ) {
            snprintf( buf1, 256, "$n's axekick %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your axekick %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's axekick %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else if ( chance <= 70 ) {
            snprintf( buf1, 256, "$n's hammer fist %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your hammer fist %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's hammer fist %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        // Following three added in by Taon.
        else if ( chance <= 80 ) {
            snprintf( buf1, 256, "$n's right hook %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your right hook %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's right hook %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );

        }
        else if ( chance <= 90 ) {
            snprintf( buf1, 256, "$n's left hook %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your left hook %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's left hook %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else {
            snprintf( buf1, 256, "$n's jab %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your jab %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's jab %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
    }
    else if ( dt == TYPE_HIT && ( ( ch )->race == RACE_DRAGON ) ) {
        if ( chance <= 33 ) {
            snprintf( buf1, 256, "$n's talons %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your talons %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's talons %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else if ( chance >= 34 && chance < 75 ) {
            snprintf( buf1, 256, "$n's massive maw %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your massive maw %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's massive maw %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
        else {
            snprintf( buf1, 256, "$n's tail %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf2, 256, "Your tail %s $N %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
            snprintf( buf3, 256, "$n's tail %s you %d %s for %d total damage%c", vp,
                      ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                      ch->damage_amount, punct );
        }
    }
    else if ( dt == TYPE_HIT ) {
        snprintf( buf1, 256, "$n %s $N %d %s for %d total damage%c", vp, ch->success_attack,
                  ( ch->success_attack == 1 ) ? "time" : "times", ch->damage_amount, punct );
        snprintf( buf2, 256, "You %s $N %d %s for %d total damage%c", vs, ch->success_attack,
                  ( ch->success_attack == 1 ) ? "time" : "times", ch->damage_amount, punct );
        snprintf( buf3, 256, "$n %s you %d %s for %d total damage%c", vp, ch->success_attack,
                  ( ch->success_attack == 1 ) ? "time" : "times", ch->damage_amount, punct );
    }
    else if ( dt > TYPE_HIT && is_wielding_poisoned( ch ) ) {
        if ( dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
            attack = attack_table[dt - TYPE_HIT];
        else {
            bug( "Dam_message: bad dt %d from %s in %d.", dt, ch->name, ch->in_room->vnum );
            dt = TYPE_HIT;
            attack = attack_table[0];
        }
        set_char_color( AT_GREEN, ch );
        snprintf( buf1, 256, "$n's poisoned %s $N %d %s for %d total damage%c", vp,
                  ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                  ch->damage_amount, punct );
        snprintf( buf2, 256, "Your poisoned %s $N %d %s for %d total damage%c", vp,
                  ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                  ch->damage_amount, punct );
        snprintf( buf3, 256, "$n's poisoned %s you %d %s for %d total damage%c", vp,
                  ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                  ch->damage_amount, punct );
    }
    else {
        if ( skill ) {
            attack = skill->noun_damage;
            if ( dam == 0 ) {
                bool                    found = FALSE;

                if ( skill->miss_char && skill->miss_char[0] != '\0' ) {
                    act( AT_CYAN, skill->miss_char, ch, NULL, victim, TO_CHAR );
                    found = TRUE;
                }
                if ( skill->miss_vict && skill->miss_vict[0] != '\0' ) {
                    act( AT_GREEN, skill->miss_vict, ch, NULL, victim, TO_VICT );
                    found = TRUE;
                }
                if ( skill->miss_room && skill->miss_room[0] != '\0' ) {
                    if ( strcmp( skill->miss_room, "supress" ) )
                        act( AT_WHITE, skill->miss_room, ch, NULL, victim, TO_NOTVICT );
                    found = TRUE;
                }
                if ( found ) {                         /* miss message already sent */
                    if ( was_in_room ) {
                        char_from_room( ch );
                        char_to_room( ch, was_in_room );
                    }
                    return;
                }
            }
            else {
                if ( skill->hit_char && skill->hit_char[0] != '\0' )
                    act( AT_CYAN, skill->hit_char, ch, NULL, victim, TO_CHAR );
                if ( skill->hit_vict && skill->hit_vict[0] != '\0' )
                    act( AT_DGREY, skill->hit_vict, ch, NULL, victim, TO_VICT );
                if ( skill->hit_room && skill->hit_room[0] != '\0' )
                    act( AT_GREEN, skill->hit_room, ch, NULL, victim, TO_NOTVICT );
            }
        }
        else if ( dt >= TYPE_HIT
                  && dt < TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) ) {
            if ( obj )
                attack = obj->short_descr;
            else
                attack = attack_table[dt - TYPE_HIT];
        }
        else {
            bug( "Dam_message: bad dt %d from %s in %d.", dt, ch->name, ch->in_room->vnum );
            dt = TYPE_HIT;
            attack = attack_table[0];

        }
        snprintf( buf1, 256, "$n's %s %s $N %d %s for %d total damage%c", attack, vp,
                  ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                  ch->damage_amount, punct );
        snprintf( buf2, 256, "Your %s %s $N %d %s for %d total damage%c", attack, vp,
                  ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                  ch->damage_amount, punct );
        snprintf( buf3, 256, "$n's %s %s you %d %s for %d total damage%c", attack, vp,
                  ch->success_attack, ( ch->success_attack == 1 ) ? "time" : "times",
                  ch->damage_amount, punct );
    }

    act( AT_GREEN, buf1, ch, NULL, victim, TO_NOTVICT );
    if ( !gcflag )
        act( AT_CYAN, buf2, ch, NULL, victim, TO_CHAR );
    if ( !gvflag )
        act( AT_DGREY, buf3, ch, NULL, victim, TO_VICT );

    if ( was_in_room ) {
        char_from_room( ch );
        char_to_room( ch, was_in_room );
    }
}

#ifndef dam_message
void dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    new_dam_message( ch, victim, dam, dt );
}
#endif

void do_kill( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

/*    CHAR_DATA *obj;  */

    one_argument( argument, arg );

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "You are concentrating too much for that!\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_PRAYER ) ) {
        send_to_char( "Not while in prayer.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "Kill whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_UNHOLY_SPHERE ) ) {
        send_to_char( "You cannot get near their unholy sphere.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_FASCINATE ) ) {
        xREMOVE_BIT( victim->affected_by, AFF_FASCINATE );
    }

    if ( IS_NPC( victim ) && victim->morph ) {
        send_to_char
            ( "This creature appears strange to you.  Look upon it more closely before attempting to kill it.",
              ch );
        return;
    }

    if ( !IS_NPC( victim ) ) {
/* Volk: Support for arena. */
        if ( in_arena( ch ) && in_arena( victim ) ) {
            do_murder( ch, victim->name );
            return;
        }

        if ( !xIS_SET( victim->act, PLR_KILLER ) && !xIS_SET( victim->act, PLR_THIEF ) ) {
            send_to_char( "You must MURDER a player.\r\n", ch );
            return;
        }
    }

    if ( victim == ch ) {
        send_to_char( "You hit yourself.  Ouch!\r\n", ch );
        multi_hit( ch, ch, TYPE_UNDEFINED );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim ) {
        act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
         || ch->position == POS_BERSERK ) {
        send_to_char( "You do the best you can!\r\n", ch );
        return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    check_attacker( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

void do_murder( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg[MIL];
    CHAR_DATA              *victim;

    one_argument( argument, arg );

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "You are concentrating too much for that!\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_PRAYER ) ) {
        send_to_char( "Not while you're praying.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "Murder whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) && xIS_SET( ch->act, PLR_PKSAFE ) ) {
        send_to_char( "You cannot do that during this event!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_FASCINATE ) ) {
        xREMOVE_BIT( victim->affected_by, AFF_FASCINATE );
    }

    if ( IS_AFFECTED( victim, AFF_UNHOLY_SPHERE ) ) {
        send_to_char( "You cannot get near their unholy sphere.\r\n", ch );
        return;
    }

    if ( is_safe( ch, victim, TRUE ) )
        return;

    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        if ( ch->master == victim ) {
            act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
            return;
        }
        else {
/*
         if ( ch->master )
            xSET_BIT ( ch->master->act, PLR_ATTACKER );
*/
        }
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
         || ch->position == POS_BERSERK ) {
        send_to_char( "You do the best you can!\r\n", ch );
        return;
    }
/* Volk: Going to put this in here. I don't feel it's fair if a player
 * gets shunted in the arena for accidentally being flagged PLR_NICE :P */
    if ( in_arena( ch ) && in_arena( victim ) ) {
        multi_hit( ch, victim, TYPE_UNDEFINED );
        return;
    }

    if ( !IS_NPC( victim ) && xIS_SET( ch->act, PLR_NICE ) ) {
        send_to_char( "You feel too nice to do that!\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) ) {
        char                    logbuf[MSL];

        snprintf( logbuf, MSL, "%s: murder %s.", ch->name, victim->name );
        log_string_plus( logbuf, LOG_NORMAL, ch->level );
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    snprintf( buf, MSL, "Wartalk: %s 'Help! I am being attacked by %s!", victim->name,
              IS_NPC( ch ) ? ch->short_descr : ch->name );
    echo_to_all( AT_RED, buf, ECHOTAR_ALL );

    if ( !in_arena( ch ) || !in_arena( victim ) )
        check_illegal_pk( ch, victim );

    check_attacker( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}

/*
 * Check to see if the player is in an "Arena".
 */
bool in_arena( CHAR_DATA *ch )
{
    if ( IS_SET( ch->in_room->room_flags, ROOM_ARENA ) )
        return TRUE;
    if ( IS_SET( ch->in_room->area->flags, AFLAG_FREEKILL ) )
        return TRUE;
    if ( ch->in_room->vnum >= arena_low_vnum && ch->in_room->vnum <= arena_high_vnum )
        return TRUE;
    if ( !str_cmp( ch->in_room->area->filename, "arena.are" ) )
        return TRUE;

    return FALSE;
}

bool check_illegal_pk( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char                    buf[MSL];
    char                    buf2[MSL];
    char                    logbuf[MSL];

    if ( !IS_NPC( victim ) && !IS_NPC( ch ) ) {
        if ( ( !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY )
               || ch->level - victim->level > 10 || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) )
             && !in_arena( ch ) && ch != victim && !( IS_IMMORTAL( ch )
                                                      && IS_IMMORTAL( victim ) ) ) {
            if ( IS_NPC( ch ) )
                snprintf( buf, MSL, " (%s)", ch->name );
            if ( IS_NPC( victim ) )
                snprintf( buf2, MSL, " (%s)", victim->name );
            snprintf( logbuf, MSL,
                      "&p%s on %s%s in &W***&rILLEGAL PKILL&W*** &pattempt at %d",
                      ( lastplayercmd ), ( IS_NPC( victim ) ? victim->short_descr : victim->name ),
                      ( IS_NPC( victim ) ? buf2 : "" ), victim->in_room->vnum );
            last_pkroom = victim->in_room->vnum;
            log_string( logbuf );
            to_channel( logbuf, "Monitor", LEVEL_IMMORTAL );

/*
      snprintf(buf, MSL, "Wartalk: %s 'Help! I am being attacked by %s!", victim->name, IS_NPC(ch) ? ch->short_descr : ch->name);

 // Volk - do we need this more than the first check?? (pfft we'll use the one in do_murder)
      if(who_fighting(victim) != ch)
        echo_to_all(AT_RED, buf, ECHOTAR_ALL);
*/
            check_attacker( ch, victim );
            return TRUE;
        }
    }
    return FALSE;
}

//Adjusted flee to take much less exp. -Taon

void do_flee( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *was_in;
    ROOM_INDEX_DATA        *now_in;
    int                     attempt,
                            los;
    short                   door;
    EXIT_DATA              *pexit;

    if ( !who_fighting( ch ) ) {
        if ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
             || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
             || ch->position == POS_BERSERK ) {
            if ( ch->mount )
                set_position( ch, POS_MOUNTED );
            else
                set_position( ch, POS_STANDING );
        }
        send_to_char( "You aren't fighting anyone.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BERSERK ) ) {
        send_to_char( "Flee while berserking?  You aren't thinking very clearly...\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_SNARE ) ) {
        send_to_char( "You cannot flee while ensnared!\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_TANGLED ) ) {
        send_to_char( "You cannot flee while tangled.\r\n", ch );
        return;
    }
    if ( ch->move <= 0 ) {
        send_to_char( "You're too exhausted to flee from combat!\r\n", ch );
        return;
    }
    /*
     * No fleeing while more aggressive than standard or hurt. - Haus 
     */
    if ( !IS_NPC( ch ) && ch->position < POS_FIGHTING ) {
        send_to_char( "You can't flee in an aggressive stance...\r\n", ch );
        return;
    }
    if ( IS_NPC( ch ) && ch->position <= POS_SLEEPING )
        return;
    was_in = ch->in_room;
    for ( attempt = 0; attempt < 8; attempt++ ) {
        door = number_door(  );
        if ( ( pexit = get_exit( was_in, door ) ) == NULL
             || !pexit->to_room
             || IS_SET( pexit->exit_info, EX_NOFLEE ) || ( IS_SET( pexit->exit_info, EX_CLOSED )
                                                           && !IS_AFFECTED( ch, AFF_PASS_DOOR ) )
             || ( IS_NPC( ch ) && IS_SET( pexit->to_room->room_flags, ROOM_NO_MOB ) ) )
            continue;
        affect_strip( ch, gsn_sneak );
        xREMOVE_BIT( ch->affected_by, AFF_SNEAK );
        if ( ch->mount && ch->mount->fighting )
            stop_fighting( ch->mount, TRUE );
        move_char( ch, pexit, 0 );
        if ( ( now_in = ch->in_room ) == was_in )
            continue;
        ch->in_room = was_in;
        act( AT_FLEE, "$n flees head over heels!", ch, NULL, NULL, TO_ROOM );
        ch->in_room = now_in;
        act( AT_FLEE, "$n glances around for signs of pursuit.", ch, NULL, NULL, TO_ROOM );
        if ( !IS_NPC( ch ) ) {
            act( AT_FLEE, "You flee head over heels from combat!", ch, NULL, NULL, TO_CHAR );
            if ( ch->pcdata->deity ) {

                if ( ch && ch->race == ch->pcdata->deity->npcrace )
                    adjust_favor( ch, 1, 0 );
                else if ( ch && ch->race == ch->pcdata->deity->npcfoe )
                    adjust_favor( ch, 16, -2 );
                else
                    adjust_favor( ch, 0, -1 );
            }
            los = ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) / 8 );

            if ( ch->level < 20 )
                los = 3500 + number_range( 1, 100 );

            // Capping loss -Taon
            if ( los > 12400 )
                los = 12400 + number_range( 1, 100 );

            if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
                los *= 3;
            }

            if ( !IS_AVATAR( ch ) && !IS_DUALAVATAR( ch ) && !IS_TRIAVATAR( ch ) )
                gain_exp( ch, 0 - los );
        }
        stop_fighting( ch, TRUE );
        return;
    }

    los = ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) / 7 );

    if ( ch->level < 20 )
        los = 3500 + number_range( 1, 100 );

    // Capping loss -Taon
    if ( los > 12400 )
        los = 12400 + number_range( 1, 100 );

    if ( xIS_SET( ch->act, PLR_EXTREME ) ) {
        los *= 3;
    }

    act( AT_FLEE, "You attempt to flee from combat but can't escape!", ch, NULL, NULL, TO_CHAR );
    if ( !IS_AVATAR( ch ) && !IS_DUALAVATAR( ch ) && !IS_TRIAVATAR( ch )
         && number_bits( 1 ) == 0 && ch->fighting && ch->fighting->who
         && ( !IS_NPC( ch->fighting->who ) || !xIS_SET( ch->fighting->who->act, ACT_IMMORTAL ) ) )
        gain_exp( ch, 0 - los );
    return;
}

/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch, CHAR_DATA *killer )
{
    char                    buf[MSL];
    OBJ_DATA               *corpse;
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    char                   *name;
    int                     x = 0;

    if ( IS_NPC( ch ) ) {
        if ( ch->pIndexData->vnum == MOB_VNUM_ANIMATED_CORPSE
             || ch->pIndexData->vnum == MOB_VNUM_ANIMATED_SKEL ) {
            act( AT_MAGIC, "$n disintegrates to the dust from whence it came.", ch, NULL,
                 ch->short_descr, TO_ROOM );
            return;
        }
        name = ch->short_descr;
        corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_NPC ), 0 );
        corpse->timer = 6;
        corpse->value[3] = 100;
        /*
         * if(IS_NPC(ch) && ch->slicevnum > 0 ) { corpse->serial = ch->slicevnum; }
         * if(IS_NPC(ch) && ch->pIndexData->slicevnum > 0 ) { corpse->pIndexData->serial
         * = ch->slicevnum; } 
         */
        for ( x = 1; x < MAX_CURR_TYPE; x++ )
            if ( GET_MONEY( ch, x ) ) {
                ch->in_room->area->looted[x] += GET_MONEY( ch, x );

                obj = create_money( GET_MONEY( ch, x ), x );
                snprintf( buf, 127, "make_corpse %d %s", ch->pIndexData->vnum, ch->name );

                obj_to_obj( obj, corpse );
                GET_MONEY( ch, x ) = 0;
            }                                          /* Cannot use these! They are
                                                        * used. corpse->value[0] =
                                                        * (int)ch->pIndexData->vnum;
                                                        * corpse->value[1] = * * * *
         * (int)ch->max_hit; *//*
         * Using corpse cost to cheat, since corpses not sellable 
         */
        corpse->cost = ( -( int ) ch->pIndexData->vnum );
        corpse->currtype = DEFAULT_CURR;
        corpse->weight = 30;
        if ( corpse->weight < 5 )
            corpse->weight = number_range( 20, 30 );
    }
    else {
        name = ch->name;
        corpse = create_object( get_obj_index( OBJ_VNUM_CORPSE_PC ), 0 );
        corpse->value[5] = 0;
        if ( in_arena( ch ) )
            corpse->timer = 0;
        else
            corpse->timer = 40;
        corpse->value[2] = ( int ) ( corpse->timer / 8 );
        corpse->value[4] = ch->level;
        if ( CAN_PKILL( ch ) && sysdata.pk_loot )
            xSET_BIT( corpse->extra_flags, ITEM_CLANCORPSE );

        /*
         * Pkill corpses get save timers, in ticks (approx 70 seconds)
         * This should be anough for the killer to type 'get all corpse'. 
         */
        if ( !IS_NPC( ch ) && !IS_NPC( killer ) )
            corpse->value[3] = 1;
        else
            corpse->value[3] = 0;
    }
    if ( CAN_PKILL( ch ) && CAN_PKILL( killer ) && ch != killer ) {
        snprintf( buf, MSL, "%s", killer->name );
        if ( corpse->action_desc )
            STRFREE( corpse->action_desc );
        corpse->action_desc = STRALLOC( buf );
    }
    /*
     * Added corpse name - make locate easier , other skills 
     */
    snprintf( buf, MSL, "corpse %s", name );
    STRFREE( corpse->name );
    corpse->name = STRALLOC( buf );
    snprintf( buf, MSL, corpse->short_descr, name );
    STRFREE( corpse->short_descr );
    corpse->short_descr = STRALLOC( buf );
    snprintf( buf, MSL, corpse->description, name );
    STRFREE( corpse->description );
    corpse->description = STRALLOC( buf );
    for ( obj = ch->first_carrying; obj; obj = obj_next ) {
        obj_next = obj->next_content;
        obj_from_char( obj );
        if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) )
            extract_obj( obj );
        else
            obj_to_obj( obj, corpse );
    }
    obj_to_room( corpse, ch->in_room );

    return;
}

void make_blood( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;

    obj = create_object( get_obj_index( OBJ_VNUM_BLOOD ), 0 );
    obj->timer = number_chance( 2, 4 );
    obj->value[1] = number_range( 3, UMIN( 5, ch->level ) );
    obj_to_room( obj, ch->in_room );
}

PKILLED_DATA           *new_pkill( void )
{
    PKILLED_DATA           *pkill;

    CREATE( pkill, PKILLED_DATA, 1 );
    pkill->name = NULL;
    pkill->timekilled = current_time;
    return pkill;
}

void free_pkill( PKILLED_DATA * pkill )
{
    if ( !pkill )
        return;
    STRFREE( pkill->name );
    DISPOSE( pkill );
}

void free_all_pkills( CHAR_DATA *ch )
{
    PKILLED_DATA           *pkill,
                           *pkill_next;

    if ( !ch || !ch->pcdata )
        return;
    for ( pkill = ch->pcdata->first_pkill; pkill; pkill = pkill_next ) {
        pkill_next = pkill->next;
        UNLINK( pkill, ch->pcdata->first_pkill, ch->pcdata->last_pkill, next, prev );
        free_pkill( pkill );
    }
}

PKILLED_DATA           *has_pkilled( CHAR_DATA *ch, char *name )
{
    PKILLED_DATA           *pkill,
                           *pkill_next;

    if ( !ch || !ch->pcdata )
        return NULL;

    for ( pkill = ch->pcdata->first_pkill; pkill; pkill = pkill_next ) {
        pkill_next = pkill->next;

        /*
         * First lets check and see if 30 mins is up if so free it 
         */
        if ( ( current_time - pkill->timekilled ) >= 1800 ) {
            UNLINK( pkill, ch->pcdata->first_pkill, ch->pcdata->last_pkill, next, prev );
            free_pkill( pkill );
            continue;
        }

        /*
         * Ok lets see if this is the one we need to check 
         */
        if ( !str_cmp( pkill->name, name ) )
            return pkill;
    }
    return NULL;
}

void add_pkill( CHAR_DATA *ch, CHAR_DATA *victim )
{
    PKILLED_DATA           *pkill;

    if ( !ch || !victim || IS_NPC( ch ) || IS_NPC( victim ) )
        return;
    /*
     * Previous kill lets update time (Note it should have been removed in check but just
     * incase 
     */
    if ( ( pkill = has_pkilled( ch, victim->name ) ) ) {
        pkill->timekilled = current_time;
        return;
    }
    pkill = new_pkill(  );
    if ( !pkill )
        return;
    pkill->name = STRALLOC( victim->name );
    pkill->timekilled = current_time;
    LINK( pkill, ch->pcdata->first_pkill, ch->pcdata->last_pkill, next, prev );
}

bool countpkill( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->level >= 30 )
        if ( ( ch->level - victim->level > 10 ) || ( victim->level - ch->level > 10 ) )
            return FALSE;
    if ( ch->level > 10 && ch->level < 30 )
        if ( ( ch->level - victim->level > 5 ) || ( victim->level - ch->level > 5 ) )
            return FALSE;
    if ( ch->level <= 10 )
        if ( ( ch->level - victim->level > 3 ) || ( victim->level - ch->level > 3 ) )
            return FALSE;
    if ( victim->level >= 30 )
        if ( ( ch->level - victim->level > 10 ) || ( victim->level - ch->level > 10 ) )
            return FALSE;
    if ( victim->level > 10 && victim->level < 30 )
        if ( ( ch->level - victim->level > 5 ) || ( victim->level - ch->level > 5 ) )
            return FALSE;
    if ( victim->level <= 10 )
        if ( ( ch->level - victim->level > 3 ) || ( victim->level - ch->level > 3 ) )
            return FALSE;
    return TRUE;
}
