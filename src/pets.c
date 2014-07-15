#include "h/mud.h"

// Revamped pet code is going here.

void do_beckon( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    bool                    found;

    set_pager_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) )
        return;

    found = FALSE;

    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            act( AT_CYAN, "$n beckons for $N to come to $s side.", ch, NULL, mob, TO_ROOM );
            act( AT_CYAN, "You beckon for $N to come to your side.", ch, NULL, mob, TO_CHAR );
            ch_printf( ch, "\r\n%s suddenly comes to you.\r\n", capitalize( mob->short_descr ) );
            if ( xIS_SET( mob->affected_by, AFF_GRAZE ) )
                xREMOVE_BIT( mob->affected_by, AFF_GRAZE );
            char_from_room( mob );
            char_to_room( mob, ch->in_room );
        }
    }

    if ( !found )
        send_to_char( "You do not have a pet to beckon.\r\n", ch );
}

void do_graze( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    bool                    found;

    set_pager_color( AT_PLAIN, ch );
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            if ( xIS_SET( mob->affected_by, AFF_GRAZE ) ) {
                xREMOVE_BIT( mob->affected_by, AFF_GRAZE );
                act( AT_CYAN, "$n motions for $N to stop grazing and attend.", ch, NULL, mob,
                     TO_ROOM );
                act( AT_CYAN, "You motion for $N to stop grazing and attend you.", ch, NULL, mob,
                     TO_CHAR );
                return;
            }
            found = TRUE;
            if ( ch->position == POS_MOUNTED )
                do_dismount( ch, ( char * ) "" );
            act( AT_CYAN, "$n lets $N go free to graze for food.", ch, NULL, mob, TO_ROOM );
            act( AT_CYAN, "You let $N go free to graze for food.", ch, NULL, mob, TO_CHAR );
            xSET_BIT( mob->affected_by, AFF_GRAZE );
        }
    }
    if ( !found )
        send_to_char( "You do not have a pet to graze.\r\n", ch );
}

void do_distract( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    bool                    found;
    CHAR_DATA              *mob;

    found = FALSE;

    if ( !ch->fighting ) {
        send_to_char( "Distract what, you're not even fighting.\r\n", ch );
        return;
    }

    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "You do not have a pet.\r\n", ch );
        return;
    }

    if ( xIS_SET( mob->act, ACT_MOUNTABLE ) ) {
        send_to_char
            ( "The pet cannot be a mountable pet, only summoned creatures can use this command.\r\n",
              ch );
        return;
    }

    if ( !mob->fighting || !( victim = who_fighting( mob ) ) ) {
        send_to_char( "He is not even fighting yet.\r\n", ch );
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char( "You cannot do that skill here.\r\n", ch );
        return;
    }

    stop_fighting( victim, TRUE );
    set_fighting( victim, mob );
    start_hating( victim, mob );
    mob->hate_level = ch->hate_level + 2;
    WAIT_STATE( ch, 12 );
    act( AT_LBLUE, "You signal at $N to start distracting your opponent!", ch, NULL, mob, TO_CHAR );
    act( AT_LBLUE, "$n signals at $N to start distracting $s opponent.", ch, NULL, mob,
         TO_NOTVICT );
    act( AT_CYAN, "\r\n$N stops fighting momentarily\r\n$N growls at $n, and attacks $m!", mob,
         NULL, victim, TO_NOTVICT );
}
