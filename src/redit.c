/**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *                      Room editing module (medit.c)                     *
 *                                                                        *
\**************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "h/mud.h"
#include "h/olc.h"

/*------------------------------------------------------------------------*/
/* function prototypes */

int get_rflag           args( ( char *flag ) );

DECLARE_DO_FUN( do_redit_reset );
void redit_disp_extradesc_menu args( ( DESCRIPTOR_DATA *d ) );
void redit_disp_exit_menu args( ( DESCRIPTOR_DATA *d ) );
void redit_disp_exit_flag_menu args( ( DESCRIPTOR_DATA *d ) );
void redit_disp_flag_menu args( ( DESCRIPTOR_DATA *d ) );
void redit_disp_sector_menu args( ( DESCRIPTOR_DATA *d ) );
void redit_disp_menu    args( ( DESCRIPTOR_DATA *d ) );
void redit_parse        args( ( DESCRIPTOR_DATA *d, char *arg ) );
void redit_setup_new    args( ( DESCRIPTOR_DATA *d ) );
void free_room          args( ( ROOM_INDEX_DATA *room ) );

/*-----------------------------------------------------------------------*/
/* Global variable declarations/externals */
/* EXIT_DATA *get_exit_number( ROOM_INDEX_DATA *room, int xit ); */
void oedit_disp_extra_choice args( ( DESCRIPTOR_DATA *d ) );
extern const char      *const ex_flags[];
extern int              top_ed;

/*-----------------------------------------------------------------------*/

void do_oredit( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA        *d;
    ROOM_INDEX_DATA        *room;

    if ( IS_NPC( ch ) || !ch->desc ) {
        send_to_char( "I don't think so...\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( argument[0] == '\0' )
        room = ch->in_room;
    else {
        if ( is_number( arg ) ) {
            argument = one_argument( argument, arg );
            room = get_room_index( atoi( arg ) );
        }
        else {
            send_to_char( "Vnum must be specified in numbers!\r\n", ch );
            return;
        }
    }

    if ( !room ) {
        send_to_char( "That room does not exist!\r\n", ch );
        return;
    }

    /*
     * Make sure the room isnt already being edited 
     */
    for ( d = first_descriptor; d; d = d->next )
        if ( d->connected == CON_REDIT )
            if ( d->olc && OLC_VNUM( d ) == room->vnum ) {
                ch_printf( ch, "That room is currently being edited by %s.\r\n",
                           d->character->name );
                return;
            }

    if ( !can_rmodify( ch, room ) )
        return;

    d = ch->desc;
    CREATE( d->olc, OLC_DATA, 1 );
    OLC_VNUM( d ) = room->vnum;
    OLC_CHANGE( d ) = FALSE;
    d->character->pcdata->dest_buf = room;
    d->connected = CON_REDIT;
    redit_disp_menu( d );

    act( AT_ACTION, "$n starts using OLC.", ch, NULL, NULL, TO_ROOM );
    return;
}

void do_rcopy( CHAR_DATA *ch, char *argument )
{
    return;
}

bool is_inolc( DESCRIPTOR_DATA *d )
{
    /*
     * safeties, not that its necessary really... 
     */
    if ( !d || !d->character )
        return FALSE;

    if ( IS_NPC( d->character ) )
        return FALSE;

    /*
     * objs 
     */
    if ( d->connected == CON_OEDIT )
        return TRUE;

    /*
     * mobs 
     */
    if ( d->connected == CON_MEDIT )
        return TRUE;

    /*
     * rooms 
     */
    if ( d->connected == CON_REDIT )
        return TRUE;

    return FALSE;
}

/*
 * Log all changes to catch those sneaky bastards =)
 */
void olc_log( DESCRIPTOR_DATA *d, const char *format, ... )
{
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    CHAR_DATA              *victim = ( CHAR_DATA * ) d->character->pcdata->dest_buf;
    char                    logline[MAX_STRING_LENGTH];
    va_list                 args;

    if ( !d ) {
        bug( "olc_log: called with null descriptor" );
        return;
    }

    va_start( args, format );
    vsprintf( logline, format, args );
    va_end( args );

    sprintf( log_buf, "Log %s:", d->character->name );
    if ( d->connected == CON_REDIT )
        sprintf( log_buf, "%s ROOM(%d): ", log_buf, room->vnum );
    else if ( d->connected == CON_OEDIT )
        sprintf( log_buf, "%s OBJ(%d): ", log_buf, obj->pIndexData->vnum );
    else if ( d->connected == CON_MEDIT ) {
        if ( IS_NPC( victim ) )
            sprintf( log_buf, "%s MOB(%d): ", log_buf, victim->pIndexData->vnum );
        else
            sprintf( log_buf, "%s PLR(%s): ", log_buf, victim->name );
    }
    else {
        bug( "olc_log: called with a bad connected state" );
        return;
    }
    sprintf( log_buf, "%s%s", log_buf, logline );
    log_string_plus( log_buf, LOG_BUILD, get_trust( d->character ) );
}

/**************************************************************************
  Menu functions 
 **************************************************************************/

/*
 * Nice fancy redone Extra Description stuff :)
 */
void redit_disp_extradesc_prompt_menu( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    EXTRA_DESCR_DATA       *ed;
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    int                     counter = 0;

    for ( ed = room->first_extradesc; ed; ed = ed->next ) {
        sprintf( buf, "&c%2d&w) %-40.40s\r\n", counter++, ed->keyword );
        send_to_char_color( buf, d->character );
    }
    send_to_char( "\r\nWhich extra description do you want to edit? ", d->character );
}

void redit_disp_extradesc_menu( DESCRIPTOR_DATA *d )
{
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    int                     count = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    if ( room->first_extradesc ) {
        EXTRA_DESCR_DATA       *ed;

        for ( ed = room->first_extradesc; ed; ed = ed->next ) {
            ch_printf_color( d->character, "&c%2d&w) Keyword: &O%s\r\n", ++count, ed->keyword );
        }
        send_to_char( "\r\n", d->character );
    }

    ch_printf_color( d->character, "&cA&w) Add a new description\r\n" );
    ch_printf_color( d->character, "&cR&w) Remove a description\r\n" );
    ch_printf_color( d->character, "&cQ&w) Quit\r\n" );
    ch_printf_color( d->character, "\r\nEnter choice: " );

    OLC_MODE( d ) = REDIT_EXTRADESC_MENU;
}

/* For exits */
void redit_disp_exit_menu( DESCRIPTOR_DATA *d )
{
    /*
     * char buf[MAX_STRING_LENGTH]; 
     */
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    EXIT_DATA              *pexit;
    int                     cnt;

    OLC_MODE( d ) = REDIT_EXIT_MENU;
    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( cnt = 0, pexit = room->first_exit; pexit; pexit = pexit->next ) {
        ch_printf( d->character, "&c%2d&w) %-10.10s to %-5d. Key: %d Keywords: %s Flags: %s\r\n",
                   ++cnt, dir_name[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                   pexit->key, ( pexit->keyword
                                 && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)",
                   flag_string( pexit->exit_info, ex_flags ) );
    }
    if ( room->first_exit )
        send_to_char( "\r\n", d->character );
    send_to_char_color( "&cA&w) Add a new exit\r\n", d->character );
    send_to_char_color( "&cR&w) Remove an exit\r\n", d->character );
    send_to_char_color( "&cQ&w) Quit\r\n", d->character );

    send_to_char( "\r\nEnter choice: ", d->character );
}

void redit_disp_exit_edit( DESCRIPTOR_DATA *d )
{
    char                    flags[MAX_STRING_LENGTH];
    EXIT_DATA              *pexit = ( EXIT_DATA * ) d->character->pcdata->spare_ptr;
    int                     i;

    flags[0] = '\0';
    for ( i = 0; i <= MAX_EXFLAG; i++ )
        if ( pexit->exit_info && IS_SET( pexit->exit_info, 1 << i ) ) {
            strcat( flags, ex_flags[i] );
            strcat( flags, " " );
        }

    OLC_MODE( d ) = REDIT_EXIT_EDIT;
    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf_color( d->character, "&c1&w) Direction  : &c%s\r\n", dir_name[pexit->vdir] );
    ch_printf_color( d->character, "&c2&w) To Vnum    : &c%d\r\n",
                     pexit->to_room ? pexit->to_room->vnum : -1 );
    ch_printf_color( d->character, "&c3&w) Key        : &c%d\r\n", pexit->key );
    ch_printf_color( d->character, "&c4&w) Keyword    : &c%s\r\n",
                     ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)" );
    ch_printf_color( d->character, "&c5&w) Flags      : &c%s\r\n",
                     flags[0] != '\0' ? flags : "(none)" );
    ch_printf_color( d->character, "&c6&w) Description: &c%s\r\n",
                     ( pexit->description
                       && pexit->description[0] != '\0' ) ? pexit->description : "(none)" );
    ch_printf_color( d->character, "&cQ&w) Quit\r\n" );
    ch_printf_color( d->character, "\r\nEnter choice: " );

    return;
}

void redit_disp_exit_dirs( DESCRIPTOR_DATA *d )
{
    int                     i;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i <= DIR_SOMEWHERE; i++ ) {
        ch_printf_color( d->character, "&c%2d&w) %s\r\n", i, dir_name[i] );
    }
    send_to_char( "\r\nChoose a direction: ", d->character );

    return;
}

/* For exit flags */
void redit_disp_exit_flag_menu( DESCRIPTOR_DATA *d )
{
    EXIT_DATA              *pexit = ( EXIT_DATA * ) d->character->pcdata->spare_ptr;
    char                    buf[MAX_STRING_LENGTH];
    char                    buf1[MAX_STRING_LENGTH];
    int                     i;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i <= MAX_EXFLAG; i++ ) {
        if ( ( 1 << i == EX_RES1 ) || ( 1 << i == EX_RES2 ) || ( 1 << i == EX_PORTAL ) )
            continue;
        ch_printf_color( d->character, "&c%2d&w) %-20.20s\r\n", i + 1, ex_flags[i] );
    }
    buf1[0] = '\0';
    for ( i = 0; i <= MAX_EXFLAG; i++ )
        if ( IS_SET( pexit->exit_info, 1 << i ) ) {
            strcat( buf1, ex_flags[i] );
            strcat( buf1, " " );
        }

    sprintf( buf, "\r\nExit flags: &c%s&w\r\n" "Enter room flags, 0 to quit: ", buf1 );
    send_to_char_color( buf, d->character );
    OLC_MODE( d ) = REDIT_EXIT_FLAGS;
}

/* For room flags */
void redit_disp_flag_menu( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    int                     counter,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < 31; counter++ ) {
        if ( counter == 28 || counter == 29 || counter == 31 )
            continue;

        sprintf( buf, "&c%2d&w) %-20.20s ", counter + 1, r_flags[counter] );

        if ( !( ++columns % 2 ) )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, d->character );
    }
    ch_printf_color( d->character, "\r\nRoom flags: &c%s&w\r\nEnter room flags, 0 to quit : ",
                     flag_string( room->room_flags, r_flags ) );
    OLC_MODE( d ) = REDIT_FLAGS;
}

/* for sector type */
void redit_disp_sector_menu( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    int                     counter,
                            columns = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < SECT_MAX; counter++ ) {
        if ( counter == SECT_MAX )
            continue;

        sprintf( buf, "&c%2d&w) %-20.20s ", counter, sec_flags[counter] );

        if ( !( ++columns % 2 ) )
            strcat( buf, "\r\n" );

        send_to_char_color( buf, d->character );
    }
    send_to_char( "\r\nEnter sector type : ", d->character );
    OLC_MODE( d ) = REDIT_SECTOR;
}

/* the main menu */
void redit_disp_menu( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    const char             *sect;

    switch ( room->sector_type ) {
        default:
            sect = "Unknown";
            break;
        case SECT_INSIDE:
            sect = "Inside";
            break;
        case SECT_CITY:
            sect = "City";
            break;
        case SECT_FIELD:
            sect = "Field";
            break;
        case SECT_FOREST:
            sect = "Forest";
            break;
        case SECT_HILLS:
            sect = "Hills";
            break;
        case SECT_MOUNTAIN:
            sect = "Mountains";
            break;
        case SECT_WATER_SWIM:
            sect = "Swim";
            break;
        case SECT_WATER_NOSWIM:
            sect = "Noswim";
            break;
        case SECT_UNDERWATER:
            sect = "Underwater";
            break;
        case SECT_AIR:
            sect = "Air";
            break;
        case SECT_DESERT:
            sect = "Desert";
            break;
        case SECT_OCEANFLOOR:
            sect = "Oceanfloor";
            break;
        case SECT_UNDERGROUND:
            sect = "Underground";
            break;
        case SECT_LAVA:
            sect = "Lava";
            break;
        case SECT_SWAMP:
            sect = "Swamp";
            break;
        case SECT_VROAD:
            sect = "Vertical Road";
            break;
        case SECT_HROAD:
            sect = "Horizontal Road";
            break;
        case SECT_OCEAN:
            sect = "Ocean";
            break;
        case SECT_JUNGLE:
            sect = "Jungle";
            break;
        case SECT_GRASSLAND:
            sect = "Grassland";
            break;
        case SECT_CROSSROAD:
            sect = "Crossroad";
            break;
        case SECT_THICKFOREST:
            sect = "Thick Forest";
            break;
        case SECT_HIGHMOUNTAIN:
            sect = "High Mountain";
            break;
        case SECT_ARCTIC:
            sect = "Arctic";
            break;
        case SECT_WATERFALL:
            sect = "Waterfall";
            break;
        case SECT_RIVER:
            sect = "River";
            break;
        case SECT_DOCK:
            sect = "Dock";
            break;
        case SECT_LAKE:
            sect = "Lake";
            break;
        case SECT_CAMPSITE:
            sect = "Campsite";
            break;
        case SECT_PORTALSTONE:
            sect = "Portal stone";
            break;
        case SECT_DEEPMUD:
            sect = "Deep Mud";
            break;
        case SECT_QUICKSAND:
            sect = "Quick Sand";
            break;
        case SECT_PASTURELAND:
            sect = "Pasture Land";
            break;
        case SECT_VALLEY:
            sect = "Valley";
            break;
        case SECT_MOUNTAINPASS:
            sect = "Mountain Pass";
            break;
        case SECT_BEACH:
            sect = "Beach";
            break;
        case SECT_FOG:
            sect = "Fog";
            break;
        case SECT_SKY:
            sect = "Sky";
            break;
        case SECT_CLOUD:
            sect = "Cloud";
            break;
        case SECT_SNOW:
            sect = "Snow";
            break;
        case SECT_ORE:
            sect = "Ore";
            break;
    }
    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    sprintf( buf,
             "\r\n\r\n&w-- Room number : [&C%d&w]      Room area: [&C%-30.30s&w]\r\n"
             "&c1&w) Name        : &C%s\r\n"
             "&c2&w) Description :\r\n&C%s\r\n"
             "&c3&w) Room flags  : &C%s\r\n"
             "&c4&w) Sector type : &C%s\r\n"
             "&c5&w) Tunnel      : &C%d\r\n"
             "&c6&w) TeleDelay   : &C%d\r\n"
             "&c7&w) TeleVnum    : &C%d\r\n"
             "&cA&w) Exit menu\r\n"
             "&cB&w) Extra descriptions menu\r\n"
             "&cC&w) Height      : &C%d\r\n"
             "&cQ&w) Quit\r\n"
             "Enter choice : ",
             OLC_NUM( d ),
             room->area ? room->area->name : "None????", room->name, room->description,
             flag_string( room->room_flags, r_flags ), sect, room->tunnel, room->tele_delay,
             room->tele_vnum, room->height );
    set_char_color( AT_PLAIN, d->character );
    send_to_char_color( buf, d->character );

    OLC_MODE( d ) = REDIT_MAIN_MENU;
}

EXTRA_DESCR_DATA       *redit_find_extradesc( ROOM_INDEX_DATA *room, int number )
{
    int                     count = 0;
    EXTRA_DESCR_DATA       *ed;

    for ( ed = room->first_extradesc; ed; ed = ed->next ) {
        if ( ++count == number )
            return ed;
    }

    return NULL;
}

void do_redit_reset( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) ch->pcdata->dest_buf;
    EXTRA_DESCR_DATA       *ed = ( EXTRA_DESCR_DATA * ) ch->pcdata->spare_ptr;

    switch ( ch->substate ) {
        case SUB_ROOM_DESC:
            if ( !ch->pcdata->dest_buf ) {
                /*
                 * If theres no dest_buf, theres no object, so stick em back as playing 
                 */
                send_to_char( "Fatal error, report to Tagith.\r\n", ch );
                bug( "do_redit_reset: sub_obj_extra: NULL ch->pcdata->dest_buf" );
                ch->substate = SUB_NONE;
                ch->desc->connected = CON_PLAYING;
                return;
            }
            STRFREE( room->description );
            room->description = copy_buffer( ch );
            stop_editing( ch );
            ch->pcdata->dest_buf = room;
            ch->desc->connected = CON_REDIT;
            ch->substate = SUB_NONE;

            olc_log( ch->desc, "Edited room description" );
            redit_disp_menu( ch->desc );
            return;

        case SUB_ROOM_EXTRA:
            STRFREE( ed->description );
            ed->description = copy_buffer( ch );
            stop_editing( ch );
            ch->pcdata->dest_buf = room;
            ch->pcdata->spare_ptr = ed;
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_REDIT;
            oedit_disp_extra_choice( ch->desc );
            OLC_MODE( ch->desc ) = REDIT_EXTRADESC_CHOICE;
            olc_log( ch->desc, "Edit description for exdesc %s", ed->keyword );

            return;
    }
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse( DESCRIPTOR_DATA *d, char *arg )
{
    ROOM_INDEX_DATA        *room = ( ROOM_INDEX_DATA * ) d->character->pcdata->dest_buf;
    ROOM_INDEX_DATA        *tmp;
    EXIT_DATA              *pexit = ( EXIT_DATA * ) d->character->pcdata->spare_ptr;
    EXTRA_DESCR_DATA       *ed = ( EXTRA_DESCR_DATA * ) d->character->pcdata->spare_ptr;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];
    int                     number = 0;

    switch ( OLC_MODE( d ) ) {
        case REDIT_CONFIRM_SAVESTRING:
            switch ( *arg ) {
                case 'y':
                case 'Y':
                    /*
                     * redit_save_internally(d); 
                     */
                    sprintf( log_buf, "OLC: %s edits room %d", d->character->name, OLC_NUM( d ) );
                    log_string_plus( log_buf, LOG_BUILD, d->character->level );
                    cleanup_olc( d );
                    send_to_char( "Room saved to memory.\r\n", d->character );
                    break;
                case 'n':
                case 'N':
                    cleanup_olc( d );
                    break;
                default:
                    send_to_char( "Invalid choice!\r\n", d->character );
                    send_to_char( "Do you wish to save this room internally? : ", d->character );
                    break;
            }
            return;

        case REDIT_MAIN_MENU:
            switch ( *arg ) {
                case 'q':
                case 'Q':
                    /*
                     * if (OLC_CHANGE(d)) { *. Something has been modified .*
                     * send_to_char( "Do you wish to save this room internally? : ",
                     * d->character ); OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING; } else 
                     */
                    cleanup_olc( d );
                    return;
                case '1':
                    send_to_char( "Enter room name:-\r\n| ", d->character );
                    OLC_MODE( d ) = REDIT_NAME;
                    break;
                case '2':
                    OLC_MODE( d ) = REDIT_DESC;
                    d->character->substate = SUB_ROOM_DESC;
                    d->character->last_cmd = do_redit_reset;

                    send_to_char( "Enter room description:-\r\n", d->character );
                    if ( !room->description )
                        room->description = STRALLOC( "" );
                    start_editing( d->character, room->description );
                    break;
                case '3':
                    redit_disp_flag_menu( d );
                    break;
                case '4':
                    redit_disp_sector_menu( d );
                    break;
                case '5':
                    send_to_char( "How many people can fit in the room? ", d->character );
                    OLC_MODE( d ) = REDIT_TUNNEL;
                    break;
                case '6':
                    send_to_char( "How long before people are teleported out? ", d->character );
                    OLC_MODE( d ) = REDIT_TELEDELAY;
                    break;
                case '7':
                    send_to_char( "Where are they teleported to? ", d->character );
                    OLC_MODE( d ) = REDIT_TELEVNUM;
                    break;
                case 'a':
                case 'A':
                    redit_disp_exit_menu( d );
                    break;
                case 'b':
                case 'B':
                    redit_disp_extradesc_menu( d );
                    break;
                case 'c':
                case 'C':
                    send_to_char( "\r\nWhat height is the room in inches?\r\n", d->character );
                    send_to_char( "Zero = no limit and 10000 inches = max height.\r\n",
                                  d->character );
                    OLC_MODE( d ) = REDIT_HEIGHT;
                    break;

                default:
                    send_to_char( "Invalid choice!", d->character );
                    redit_disp_menu( d );
                    break;
            }
            return;

        case REDIT_NAME:
            STRFREE( room->name );
            room->name = STRALLOC( arg );
            olc_log( d, "Changed name to %s", room->name );
            break;

        case REDIT_DESC:
            /*
             * we will NEVER get here 
             */
            bug( "Reached REDIT_DESC case in redit_parse" );
            break;

        case REDIT_FLAGS:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;
                else if ( number < 0 || number > 32 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                else {
                    number -= 1;                       /* Offset for 0 */
                    TOGGLE_BIT( room->room_flags, 1 << number );
                    olc_log( d, "%s the room flag %s",
                             IS_SET( room->room_flags, 1 << number ) ? "Added" : "Removed",
                             r_flags[number] );
                }
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_rflag( arg1 );
                    if ( number > 0 ) {
                        TOGGLE_BIT( room->room_flags, 1 << number );
                        olc_log( d, "%s the room flag %s",
                                 IS_SET( room->room_flags, 1 << number ) ? "Added" : "Removed",
                                 r_flags[number] );
                    }
                }
            }
            redit_disp_flag_menu( d );
            return;

        case REDIT_SECTOR:
            number = atoi( arg );
            if ( number < 0 || number >= SECT_MAX ) {
                send_to_char( "Invalid choice!", d->character );
                redit_disp_sector_menu( d );
                return;
            }
            else
                room->sector_type = number;
            olc_log( d, "Changed sector to %s", sec_flags[number] );
            break;

        case REDIT_TUNNEL:
            number = atoi( arg );
            room->tunnel = URANGE( 0, number, 1000 );
            olc_log( d, "Changed tunnel amount to %d", room->tunnel );
            break;

        case REDIT_TELEDELAY:
            number = atoi( arg );
            room->tele_delay = number;
            olc_log( d, "Changed teleportation delay to %d", room->tele_delay );
            break;

        case REDIT_TELEVNUM:
            number = atoi( arg );
            room->tele_vnum = URANGE( 1, number, MAX_VNUM );
            olc_log( d, "Changed teleportation vnum to %d", room->tele_vnum );
            break;

        case REDIT_HEIGHT:
            number = atoi( arg );
            room->height = URANGE( 0, number, 10000 );
            olc_log( d, "Changed height to %d", room->height );
            break;

        case REDIT_EXIT_MENU:
            switch ( UPPER( arg[0] ) ) {
                default:
                    if ( is_number( arg ) ) {
                        number = atoi( arg );
                        pexit = get_exit_num( room, number );
                        d->character->pcdata->spare_ptr = pexit;
                        redit_disp_exit_edit( d );
                        return;
                    }
                    redit_disp_exit_menu( d );
                    return;
                case 'A':
                    OLC_MODE( d ) = REDIT_EXIT_ADD;
                    redit_disp_exit_dirs( d );
                    return;
                case 'R':
                    OLC_MODE( d ) = REDIT_EXIT_DELETE;
                    send_to_char( "Delete which exit? ", d->character );
                    return;
                case 'Q':
                    d->character->pcdata->spare_ptr = NULL;
                    break;
            }
            break;

        case REDIT_EXIT_EDIT:
            switch ( UPPER( arg[0] ) ) {
                case 'Q':
                    d->character->pcdata->spare_ptr = NULL;
                    redit_disp_exit_menu( d );
                    return;
                case '1':
                    /*
                     * OLC_MODE(d) = REDIT_EXIT_DIR; redit_disp_exit_dirs(d); 
                     */
                    send_to_char( "This option can only be changed by remaking the exit.\r\n",
                                  d->character );
                    break;
                case '2':
                    OLC_MODE( d ) = REDIT_EXIT_VNUM;
                    send_to_char( "Which room does this exit go to? ", d->character );
                    return;
                case '3':
                    OLC_MODE( d ) = REDIT_EXIT_KEY;
                    send_to_char( "What is the vnum of the key to this exit? ", d->character );
                    return;
                case '4':
                    OLC_MODE( d ) = REDIT_EXIT_KEYWORD;
                    send_to_char( "What is the keyword to this exit? ", d->character );
                    return;
                case '5':
                    OLC_MODE( d ) = REDIT_EXIT_FLAGS;
                    redit_disp_exit_flag_menu( d );
                    return;
                case '6':
                    OLC_MODE( d ) = REDIT_EXIT_DESC;
                    send_to_char( "Description:\r\n] ", d->character );
                    return;
            }
            redit_disp_exit_edit( d );
            return;

        case REDIT_EXIT_DESC:
            if ( !arg || arg[0] == '\0' )
                pexit->description = STRALLOC( "" );
            else {
                sprintf( buf, "%s\r\n", arg );
                pexit->description = STRALLOC( buf );
            }
            olc_log( d, "Changed %s description to %s", dir_name[pexit->vdir], arg ? arg : "none" );
            redit_disp_exit_edit( d );
            return;

        case REDIT_EXIT_ADD:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number < DIR_NORTH || number > DIR_SOMEWHERE ) {
                    send_to_char( "Invalid direction, try again: ", d->character );
                    return;
                }
                d->character->tempnum = number;
            }
            else {
                number = get_dir( arg );
                pexit = get_exit( room, number );
                if ( pexit ) {
                    send_to_char( "An exit in that direction already exists.\r\n", d->character );
                    redit_disp_exit_menu( d );
                    return;
                }
                d->character->tempnum = number;
            }
            OLC_MODE( d ) = REDIT_EXIT_ADD_VNUM;
            send_to_char( "Which room does this exit go to? ", d->character );
            return;

        case REDIT_EXIT_ADD_VNUM:
            number = atoi( arg );
            if ( ( tmp = get_room_index( number ) ) == NULL ) {
                send_to_char( "Non-existant room.\r\n", d->character );
                OLC_MODE( d ) = REDIT_EXIT_MENU;
                redit_disp_exit_menu( d );
                return;
            }
            pexit = make_exit( room, tmp, d->character->tempnum );
            pexit->keyword = STRALLOC( "" );
            pexit->description = STRALLOC( "" );
            pexit->key = -1;
            pexit->exit_info = 0;
            act( AT_IMMORT, "$n reveals a hidden passage!", d->character, NULL, NULL, TO_ROOM );
            d->character->pcdata->spare_ptr = pexit;

            olc_log( d, "Added %s exit to %d", dir_name[pexit->vdir], pexit->vnum );

            OLC_MODE( d ) = REDIT_EXIT_EDIT;
            redit_disp_exit_edit( d );
            return;

        case REDIT_EXIT_DELETE:
            if ( !is_number( arg ) ) {
                send_to_char( "Exit must be specified in a number.\r\n", d->character );
                redit_disp_exit_menu( d );
            }
            number = atoi( arg );
            pexit = get_exit_num( room, number );
            if ( !pexit ) {
                send_to_char( "That exit does not exist.\r\n", d->character );
                redit_disp_exit_menu( d );
            }
            olc_log( d, "Removed %s exit", dir_name[pexit->vdir] );
            extract_exit( room, pexit );
            redit_disp_exit_menu( d );
            return;

        case REDIT_EXIT_VNUM:
            number = atoi( arg );
            if ( number < 0 || number > MAX_VNUM ) {
                send_to_char( "Invalid room number, try again : ", d->character );
                return;
            }
            if ( get_room_index( number ) == NULL ) {
                send_to_char( "That room does not exist, try again: ", d->character );
                return;
            }
            pexit->vnum = number;
            olc_log( d, "%s exit vnum changed to %d", dir_name[pexit->vdir], pexit->vnum );
            redit_disp_exit_menu( d );
            return;

        case REDIT_EXIT_KEYWORD:
            STRFREE( pexit->keyword );
            pexit->keyword = STRALLOC( arg );
            olc_log( d, "Changed %s keyword to %s", dir_name[pexit->vdir], pexit->keyword );
            redit_disp_exit_edit( d );
            return;

        case REDIT_EXIT_KEY:
            number = atoi( arg );
            if ( number < 0 || number > MAX_VNUM )
                send_to_char( "Invalid vnum, try again: ", d->character );
            else {
                pexit->key = number;
                redit_disp_exit_edit( d );
            }
            olc_log( d, "%s key vnum is now %d", dir_name[pexit->vdir], pexit->key );
            return;

        case REDIT_EXIT_FLAGS:
            number = atoi( arg );
            if ( number == 0 ) {
                redit_disp_exit_edit( d );
                return;
            }

            if ( ( number < 0 ) || ( number > MAX_EXFLAG + 1 ) || ( 1 << ( number - 1 ) == EX_RES1 )
                 || ( 1 << ( number - 1 ) == EX_RES2 ) || ( 1 << ( number - 1 ) == EX_PORTAL ) ) {
                send_to_char( "That's not a valid choice!\r\n", d->character );
                redit_disp_exit_flag_menu( d );
            }
            number -= 1;
            TOGGLE_BIT( pexit->exit_info, 1 << number );
            olc_log( d, "%s %s to %s exit",
                     IS_SET( pexit->exit_info, 1 << number ) ? "Added" : "Removed",
                     ex_flags[number], dir_name[pexit->vdir] );
            redit_disp_exit_flag_menu( d );
            return;

        case REDIT_EXTRADESC_DELETE:
            ed = redit_find_extradesc( room, atoi( arg ) );
            if ( !ed ) {
                send_to_char( "Not found, try again: ", d->character );
                return;
            }
            olc_log( d, "Deleted exdesc %s", ed->keyword );
            UNLINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
            STRFREE( ed->keyword );
            STRFREE( ed->description );
            DISPOSE( ed );
            top_ed--;
            redit_disp_extradesc_menu( d );
            return;

        case REDIT_EXTRADESC_CHOICE:
            switch ( UPPER( arg[0] ) ) {
                case 'Q':
                    if ( !ed->keyword || !ed->description ) {
                        send_to_char( "No keyword and/or description, junking...", d->character );
                        UNLINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
                        STRFREE( ed->keyword );
                        STRFREE( ed->keyword );
                        DISPOSE( ed );
                        top_ed--;
                    }
                    d->character->pcdata->spare_ptr = NULL;
                    redit_disp_extradesc_menu( d );
                    return;
                case '1':
                    OLC_MODE( d ) = REDIT_EXTRADESC_KEY;
                    send_to_char( "Keywords, seperated by spaces: ", d->character );
                    return;
                case '2':
                    OLC_MODE( d ) = REDIT_EXTRADESC_DESCRIPTION;
                    d->character->substate = SUB_ROOM_EXTRA;
                    d->character->last_cmd = do_redit_reset;

                    send_to_char( "Enter new extradesc description: \r\n", d->character );
                    start_editing( d->character, ed->description );
                    return;
            }
            break;

        case REDIT_EXTRADESC_KEY:
            /*
             * if ( SetRExtra( room, arg ) ) { send_to_char( "A extradesc with that
             * keyword already exists.\r\n", d->character );
             * redit_disp_extradesc_menu(d); return; } 
             */
            olc_log( d, "Changed exkey %s to %s", ed->keyword, arg );
            STRFREE( ed->keyword );
            ed->keyword = STRALLOC( arg );
            oedit_disp_extra_choice( d );
            OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
            return;

        case REDIT_EXTRADESC_MENU:
            switch ( UPPER( arg[0] ) ) {
                case 'Q':
                    break;
                case 'A':
                    CREATE( ed, EXTRA_DESCR_DATA, 1 );

                    LINK( ed, room->first_extradesc, room->last_extradesc, next, prev );
                    ed->keyword = STRALLOC( "" );
                    ed->description = STRALLOC( "" );
                    top_ed++;
                    d->character->pcdata->spare_ptr = ed;
                    olc_log( d, "Added new exdesc" );

                    oedit_disp_extra_choice( d );
                    OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
                    return;
                case 'R':
                    OLC_MODE( d ) = REDIT_EXTRADESC_DELETE;
                    send_to_char( "Delete which extra description? ", d->character );
                    return;
                default:
                    if ( is_number( arg ) ) {
                        ed = redit_find_extradesc( room, atoi( arg ) );
                        if ( !ed ) {
                            send_to_char( "Not found, try again: ", d->character );
                            return;
                        }
                        d->character->pcdata->spare_ptr = ed;
                        oedit_disp_extra_choice( d );
                        OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
                    }
                    else
                        redit_disp_extradesc_menu( d );
                    return;
            }
            break;

        default:
            /*
             * we should never get here 
             */
            bug( "Reached default case in parse_redit" );
            break;
    }
    /*
     * Log the changes, so we can keep track of those sneaky bastards 
     */
    /*
     * Don't log on the flags cause it does that above 
     */
    /*
     * if ( OLC_MODE(d) != REDIT_FLAGS ) olc_log( d, arg ); 
     */

    /*
     * . If we get this far, something has be changed .
     */
    OLC_CHANGE( d ) = TRUE;
    redit_disp_menu( d );
}
