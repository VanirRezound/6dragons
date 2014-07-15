#include "h/mud.h"
/* companion code - attempt to allow players to
group with one another no matter what the level range,
also allow pkillers to pkill in any level range */

/* known bugs that would need fixing.  Like life command
occassionally hp, mana, move will go to all zeros instead
of their former, could be with crashes or copyovers, not
sure yet. */
void                    remove_all_equipment( CHAR_DATA *ch );

void do_companion( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MIL];
    CHAR_DATA              *buddy;

    if ( IS_NPC( ch ) )
        return;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg );

    if ( IS_IMMORTAL( ch ) )
        return;

    if ( ( arg[0] == '\0' ) ) {
        send_to_char( "&cSyntax: Companion <&Cbuddy&c>\r\n", ch );
        send_to_char( "        Companion revert\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_LIFE ) && str_cmp( arg, "revert" ) ) {
        send_to_char( "You are already in companion mode, use companion revert.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "revert" ) ) {
        if ( !xIS_SET( ch->act, PLR_LIFE ) || ch->position == POS_FIGHTING ) {
            send_to_char( "You can't revert, if you did not companion.\r\n", ch );
            return;
        }
        remove_all_equipment( ch );
        ch->level = ch->pcdata->tmplevel;
        ch->trust = ch->pcdata->tmptrust;
        ch->max_hit = ch->pcdata->tmpmax_hit;
        ch->max_move = ch->pcdata->tmpmax_move;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->max_blood = ch->pcdata->tmpmax_mana;
        }
        else {
            ch->max_mana = ch->pcdata->tmpmax_mana;
        }
        ch->pcdata->tmpmax_hit = 0;
        ch->pcdata->tmpmax_move = 0;
        ch->pcdata->tmpmax_mana = 0;
        ch->pcdata->tmplevel = 0;
        ch->pcdata->tmptrust = 0;
        if ( xIS_SET( ch->act, PLR_LIFE ) )
            xREMOVE_BIT( ch->act, PLR_LIFE );
        save_char_obj( ch );
        act( AT_MAGIC, "$n reverts back to former status.", ch, NULL, NULL, TO_ROOM );
        act( AT_MAGIC, "You revert back to your former status.", ch, NULL, NULL, TO_CHAR );
        snprintf( buf, MIL,
                  "%s has reverted back to their former status with the companion command.",
                  ch->name );
        echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );
        if ( ch->max_hit > 100000 || ch->max_hit < 0 ) {
            ch->max_hit = 30000;
        }
        restore_char( ch );
// Adding fix for reverted characters remaining in group out of their 
// level range - Aurin 12/9/2010
        if ( ch->leader != NULL ) {
            do_follow( ch, ( char * ) "self" );
        }
        else {
            do_group( ch, ( char * ) "disband" );
        }
        return;
    }

    if ( ( buddy = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "No such person in the game.\r\n", ch );
        return;
    }

    if ( buddy ) {
        short                   level;
        short                   num;

        num = 1;
        level = buddy->level;
        if ( level > ch->level || level < 2 ) {
            send_to_char( "Invalid Level.\r\n", ch );
            return;
        }

        if ( ch->pcdata->tmplevel == 0 )
            ch->pcdata->tmplevel = ch->level;
        if ( ch->pcdata->tmptrust == 0 )
            ch->pcdata->tmptrust = ch->trust;
        remove_all_equipment( ch );
        ch->level = level;
        ch->trust = 0;
        ch->pcdata->tmpmax_hit = ch->max_hit;
        ch->pcdata->tmpmax_move = ch->max_move;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->pcdata->tmpmax_mana = ch->max_blood;
        }
        else {
            ch->pcdata->tmpmax_mana = ch->max_mana;
        }
        save_char_obj( ch );
        ch->max_hit = 10;
        ch->hit = 10;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->max_blood = 10;
            ch->blood = 10;
        }
        else {
            ch->max_mana = 10;
            ch->mana = 10;
        }
        ch->max_move = 10;
        ch->move = 10;

        if ( IS_SECONDCLASS( ch ) && !IS_THIRDCLASS( ch ) ) {
            level *= 2;
        }
        if ( IS_THIRDCLASS( ch ) ) {
            level *= 3;
        }
        if ( !xIS_SET( ch->act, PLR_LIFE ) )
            xSET_BIT( ch->act, PLR_LIFE );

        while ( num <= level ) {
            advance_level( ch );
            num++;
        }
        restore_char( ch );
        snprintf( buf, MIL, "%s has used the companion command to regress to a lower status.",
                  ch->name );
        echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );
        act( AT_MAGIC, "$n has transended back to their former status.", ch, NULL, NULL, TO_ROOM );
        act( AT_MAGIC, "You transend back to your former status.", ch, NULL, NULL, TO_CHAR );
        return;
    }
    return;
}
