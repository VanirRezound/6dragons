#include "h/mud.h"

/* Let noobs go to places for free under level 20, after that take people to general locations at cost */
void do_activate( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *stone,
                           *rune = NULL;
    bool                    found = FALSE;
    char                    arg[MIL];
    ROOM_INDEX_DATA        *room;
    int                     rvnum,
                            ivnum,
                            unum;

    argument = one_argument( argument, arg );

    if ( ch->level < 2 ) {
        send_to_char( "You must complete the tutorial to use this command.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        return;
    }

    for ( stone = ch->in_room->first_content; stone; stone = stone->next_content ) {
        if ( stone->item_type == ITEM_STONE ) {
            found = TRUE;
            break;
        }
    }

    ivnum = ch->in_room->vnum;
    if ( ivnum == 18636 || ivnum == 1501 ) {
        if ( ivnum == 18636 )
            rvnum = 1501;                              // Shattered Refuge
        else if ( ivnum == 1501 )
            rvnum = 18636;                             // Back to Time Machine

        if ( !( room = get_room_index( rvnum ) ) ) {
            send_to_char( "Couldn't send you there. Please inform someone.\r\n", ch );
            return;
        }

        send_to_char( "The time machine begins to hum.\r\n", ch );
        char_from_room( ch );
        char_to_room( ch, room );
        act( AT_YELLOW, "$n appears in a spray of shooting light.", ch, NULL, ch, TO_ROOM );
        do_look( ch, ( char * ) "auto" );
        return;
    }

    if ( !found && ch->in_room->sector_type != SECT_PORTALSTONE ) {
        send_to_char( "There must be a portalstone here in order to use activate command.\r\n",
                      ch );
        return;
    }

    /*
     * Make level 20 and higher require a rune to use 
     */
    if ( ch->level >= 20 ) {
        if ( ms_find_obj( ch ) )
            return;

        if ( !( rune = get_eq_char( ch, WEAR_HOLD ) ) || rune->item_type != ITEM_RUNE ) {
            send_to_char( "You must be holding an rune to activate the portal stone.\r\n", ch );
            return;
        }
    }

    if ( !xIS_SET( ch->act, PLR_ACTIVATE ) ) {
        if ( !arg || arg[0] == '\0' ) {
            act( AT_ORANGE,
                 "The portal stone, begins to hum and a series of locations appear in your mind.",
                 ch, NULL, ch, TO_CHAR );
            act( AT_CYAN, "$n touches the portal stone, and it begins to hum.", ch, NULL, ch,
                 TO_ROOM );
            send_to_char
                ( "&cThe names of the places, as well as how dangerous the area is comes to your mind in a rush.\r\n",
                  ch );
            xSET_BIT( ch->act, PLR_ACTIVATE );
        }
        else {
            send_to_char
                ( "You cannot activate the portalstone with an area already focused in your mind.\r\n",
                  ch );
            send_to_char( "Syntax: Activate - This will turn on the portalstone.\r\n", ch );
            send_to_char
                ( "Syntax: Activate < Location Name > - Will transport you after the stone has been turned on.\r\n",
                  ch );
            return;
        }
    }

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "\r\n\r\n&GLocation                   Difficulty Level\r\n", ch );
        send_to_char( "&G1. Merchant's Discovery        00 - 10\r\n", ch );
        send_to_char( "&G2. Norrinton's South Harbor    10 - 20\r\n", ch );
        send_to_char( "&G3. The Abyss                   05 - 10\r\n", ch );
        send_to_char( "&G4. Gnome Tower                 05 - 20\r\n", ch );
        send_to_char( "&G5. Kirwood Swamp               00 - 05\r\n", ch );
        send_to_char( "&G6. Abandoned Cabin             05 - 20\r\n", ch );
        send_to_char( "&G7. Garden                      15 - 20\r\n", ch );
        send_to_char( "&G8. Tufkul'ar                   15 - 30\r\n", ch );
        send_to_char( "&G9. Durdun                      10 - 20\r\n", ch );
        send_to_char( "&G10.Spiral Village              10 - 20\r\n", ch );
        send_to_char( "&G11.Ice Cavern                   5 - 20\r\n", ch );
        if ( ch->race == RACE_DRAGON ) {
            send_to_char( "&G12.Under the Volcano           10 - 20 [&RDragon Only Area&G]\r\n",
                          ch );
            send_to_char( "&cProper Syntax: Activate < Location Number ( 1 - 12 )>\r\n", ch );
        }
        else {
            send_to_char( "&G12.Under Sewer                 03 - 10 [&RGroup Area&G]\r\n", ch );
            send_to_char( "&G13.Bowels of the Citadel       10 - 25 [&RGroup Area&G]\r\n", ch );
            send_to_char( "&cProper Syntax: Activate < Location Number ( 1 - 13 )>\r\n", ch );
        }
        return;
    }

    if ( !is_number( arg ) ) {
        send_to_char( "Syntax: Activate <#>\r\n", ch );
        return;
    }
    unum = atoi( arg );

    if ( unum == 1 )
        rvnum = 2758;
    else if ( unum == 2 )
        rvnum = 10001;
    else if ( unum == 3 )
        rvnum = 7501;
    else if ( unum == 4 )
        rvnum = 18631;
    else if ( unum == 5 )
        rvnum = 27001;
    else if ( unum == 6 )
        rvnum = 29000;
    else if ( unum == 7 )
        rvnum = 4008;
    else if ( unum == 8 )
        rvnum = 28251;
    else if ( unum == 9 )
        rvnum = 18300;
    else if ( unum == 10 )
        rvnum = 13027;
    else if ( unum == 11 )
        rvnum = 6279;
    else if ( unum == 12 ) {
        if ( ch->race == RACE_DRAGON )
            rvnum = 8310;
        else
            rvnum = 4901;
    }
    else if ( unum == 13 )
        rvnum = 5300;

    if ( !( room = get_room_index( rvnum ) ) ) {
        send_to_char( "Couldn't send you there. Please inform someone.\r\n", ch );
        return;
    }

    if ( ch->level >= 20 && rune )
        extract_obj( rune );

    act( AT_ORANGE, "A strange light envelopes you!", ch, NULL, ch, TO_CHAR );
    act( AT_ORANGE, "$n is enveloped in a strange light!", ch, NULL, ch, TO_ROOM );
    char_from_room( ch );

    char_to_room( ch, room );
    act( AT_ORANGE, "The light suddenly fades!", ch, NULL, ch, TO_CHAR );
    act( AT_ORANGE, "$n appears within a strange light!", ch, NULL, ch, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    xREMOVE_BIT( ch->act, PLR_ACTIVATE );
}
