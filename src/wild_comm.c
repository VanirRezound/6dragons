/*
 * Information: The wilderness control panel for players will go here.
 * This will allow them to set various different map options, which
 * will be worked into the automap.c to modify its display. -Taon
 */

//STATUS: Just breaking ground on this file. -Taon

#include "h/mud.h"

void display_panel( CHAR_DATA *ch )
{
    if ( IS_NPC( ch ) )
        return;

    send_to_char( "&W&UWILDERNESS MAP OPTIONS&d\r\n", ch );
    ch_printf( ch, "&BMAP:          %s\r\n", ch->map_toggle == 0 ? "&RENABLED&d" : "&RDISABLED&d" );
    ch_printf( ch, "&BDESCRIPTION:  %s\r\n",
               ch->map_desc_toggle == 0 ? "&RENABLED&d" : "&RDISABLED&d" );
    ch_printf( ch, "&BROOM NAME:    %s\r\n",
               ch->map_name_toggle == 0 ? "&RENABLED&d" : "&RDISABLED&d" );
    if ( IS_IMMORTAL( ch ) ) {
        ch_printf( ch, "&BMAP TYPE:     %s\r\n",
                   ch->map_type == 0 ? "&RDEFAULT&d" : ch->map_type ==
                   1 ? "&RDIAMOND&d" : ch->map_type == 2 ? "&RSPHERE&d" : "NOTDETECTED" );
        ch_printf( ch, "&BDISPLAY SIZE: %s&d\r\n",
                   ch->map_size == 0 ? "&RSMALL&d" : ch->map_size ==
                   1 ? "&RMEDIUM&d" : ch->map_size == 2 ? "&RLARGE&d" : "&RXXX&d" );
    }
}

//Code status: Functions Completed, though hasn't fully
//been worked into the automap code. -Taon
void do_wilderness( CHAR_DATA *ch, char *argument )
{
    char                    arg[MSL],
                            arg1[MSL],
                            arg2[MSL];

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' || arg2[0] == '\0' ) {
        display_panel( ch );
        send_to_char( "&WSyntax: wild set <option> <value>\r\n", ch );
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char( "&WOptions: type size map name description\r\n", ch );
        }
        else if ( !IS_IMMORTAL( ch ) ) {
            send_to_char( "&WOptions: map name description\r\n", ch );
        }
        return;
    }
    if ( !str_cmp( arg, "set" ) ) {
        if ( !str_cmp( arg1, "type" ) ) {
            if ( !IS_IMMORTAL( ch ) ) {
                send_to_char( "&WSyntax: wild set <option> <value>\r\n", ch );
                send_to_char( "&WOptions: map name description\r\n", ch );
                return;
            }
            if ( !str_cmp( arg2, "default" ) )
                ch->map_type = 0;
            else if ( !str_cmp( arg2, "diamond" ) )
                ch->map_type = 1;
            else if ( !str_cmp( arg2, "sphere" ) )
                ch->map_type = 2;
            else {
                send_to_char( "Types: default, diamond, sphere.\r\n", ch );
                return;
            }
            send_to_char( "Done...\r\n", ch );
            return;
        }
        if ( !str_cmp( arg1, "map" ) ) {
            if ( !str_cmp( arg2, "on" ) )
                ch->map_toggle = 0;
            else if ( !str_cmp( arg2, "off" ) )
                ch->map_toggle = 1;
            else {
                send_to_char( "You can only toggle the map on or off.\r\n", ch );
                return;
            }
            send_to_char( "Done...\r\n", ch );
            return;
        }
        if ( !str_cmp( arg1, "description" ) ) {
            if ( !str_cmp( arg2, "on" ) )
                ch->map_desc_toggle = 0;
            else if ( !str_cmp( arg2, "off" ) )
                ch->map_desc_toggle = 1;
            else {
                send_to_char( "You can only toggle the description on and off.\r\n", ch );
                return;
            }
            send_to_char( "Done...\r\n", ch );
            return;
        }
        if ( !str_cmp( arg1, "name" ) ) {
            if ( !str_cmp( arg2, "on" ) )
                ch->map_name_toggle = 0;
            else if ( !str_cmp( arg2, "off" ) )
                ch->map_name_toggle = 1;
            else {
                send_to_char( "You can only toggle the room name on and off.\r\n", ch );
                return;
            }
            send_to_char( "Done...\r\n", ch );
            return;
        }
        if ( !str_cmp( arg1, "size" ) ) {
            if ( !IS_IMMORTAL( ch ) ) {
                send_to_char( "&WSyntax: wild set <option> <value>\r\n", ch );
                send_to_char( "&WOptions: map name description\r\n", ch );
                return;
            }

            if ( !str_cmp( arg2, "small" ) )
                ch->map_size = 0;
            else if ( !str_cmp( arg2, "medium" ) )
                ch->map_size = 1;
            else if ( !str_cmp( arg2, "large" ) )
                ch->map_size = 2;
            else {
                send_to_char( "You may only select small, medium, or large for size.\r\n", ch );
                return;
            }
            send_to_char( "Done...\r\n", ch );
            return;
        }
        return;
    }
    send_to_char( "Syntax: wilderness <set> <option> <value>\r\n", ch );
    send_to_char( "Options: type size map name description\r\n", ch );
}
