 /**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *                    Object editing module (oedit.c)                     *
 *                                                                        *
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"
#include "h/olc.h"

/* External functions */
extern int              top_affect;
extern int              top_ed;
extern int              top_obj_index;
extern OBJ_INDEX_DATA  *obj_index_hash[MAX_KEY_HASH];
int get_risflag         args( ( char *flag ) );
void medit_disp_aff_flags args( ( DESCRIPTOR_DATA *d ) );
void medit_disp_ris     args( ( DESCRIPTOR_DATA *d ) );
short                   min_archery_chart( int level );
short                   max_archery_chart( int level );
short                   set_min_armor( int level );
short                   set_max_armor( int level );
bool                    is_head_architect( CHAR_DATA *ch );
bool write_to_descriptor args( ( DESCRIPTOR_DATA *d, const char *txt, int length ) );
extern const struct obj_color_type freak_color[];
int get_currency_type   args( ( char *type ) );

/* Internal functions */
DECLARE_DO_FUN( do_oedit_reset );
void oedit_disp_layer_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_container_flags_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_lever_flags_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_extradesc_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_weapon_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val1_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val2_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val3_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val4_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val5_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val6_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_val7_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_type_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_extra_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_wear_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_disp_menu    args( ( DESCRIPTOR_DATA *d ) );

void oedit_parse        args( ( DESCRIPTOR_DATA *d, char *arg ) );
void oedit_disp_spells_menu args( ( DESCRIPTOR_DATA *d ) );
void oedit_liquid_type  args( ( DESCRIPTOR_DATA *d ) );
void oedit_setup_new    args( ( DESCRIPTOR_DATA *d ) );
void oedit_setup_existing args( ( DESCRIPTOR_DATA *d, int real_num ) );
void oedit_save_to_disk args( ( DESCRIPTOR_DATA *d ) ); /* Unused */
void oedit_save_internally args( ( DESCRIPTOR_DATA *d ) );

void oedit_setup        args( ( DESCRIPTOR_DATA *d, int vnum ) );

/*------------------------------------------------------------------------*/

int get_containerflag( char *flag )
{
    int                     x;

    for ( x = 0; x < 32; x++ )

        if ( !str_cmp( flag, container_flags[x] ) )

            return x;

    return -1;
}

void cleanup_olc( DESCRIPTOR_DATA *d )
{
    if ( d->olc ) {
        if ( d->character ) {
            d->character->pcdata->dest_buf = NULL;
            act( AT_ACTION, "$n stops using OLC.", d->character, NULL, NULL, TO_CANSEE );
        }
        d->connected = CON_PLAYING;
        DISPOSE( d->olc );
    }
    do_silentsave( d->character, ( char * ) "" );
    return;
}

/*
 * Starts it all off
 */
void do_ooedit( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA        *d;
    OBJ_DATA               *obj;
    AREA_DATA              *pArea;
    int                     cvnum,
                            vnum;
    OBJ_INDEX_DATA         *pObjIndex;

    if ( IS_NPC( ch ) ) {
        send_to_char( "I don't think so...\r\n", ch );
        return;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        pArea = ch->in_room->area;
        vnum = pArea->low_o_vnum;

        if ( !pArea ) {
            bug( "object: !pArea" );
            return;
        }

        while ( vnum <= pArea->hi_o_vnum && get_obj_index( vnum ) != NULL )
            vnum++;
        if ( vnum > pArea->hi_o_vnum ) {
            send_to_char
                ( "&GYou cannot create any more objects as you have used all that your area is alloted.\r\n",
                  ch );
            return;
        }
        pObjIndex = make_object( vnum, cvnum, argument );
        pObjIndex->area = ch->pcdata->area;

        if ( !pObjIndex ) {
            send_to_char( "Error.\r\n", ch );
            log_string( "do_ocreate: make_object failed." );
            return;
        }
        obj = create_object( pObjIndex, get_trust( ch ) );
        obj_to_char( obj, ch );
        act( AT_IMMORT, "$n makes arcane gestures, and opens $s hands to reveal $p!", ch, obj, NULL,
             TO_ROOM );
        obj->name = STRALLOC( "object" );
    }
    else {
        if ( ( obj = get_obj_world( ch, arg ) ) == NULL ) {
            send_to_char( "Nothing like that in hell, earth, or heaven.\r\n", ch );
            return;
        }
    }

    /*
     * Make sure the object isnt already being edited 
     */
    for ( d = first_descriptor; d; d = d->next )
        if ( d->connected == CON_OEDIT )
            if ( d->olc && OLC_VNUM( d ) == obj->pIndexData->vnum ) {
                ch_printf( ch, "That object is currently being edited by %s.\r\n",
                           d->character->name );
                return;
            }

    if ( !can_omodify( ch, obj ) )
        return;

    d = ch->desc;
    CREATE( d->olc, OLC_DATA, 1 );
    OLC_VNUM( d ) = obj->pIndexData->vnum;
    OLC_CHANGE( d ) = FALSE;
    OLC_VAL( d ) = 0;
    d->character->pcdata->dest_buf = obj;
    d->connected = CON_OEDIT;
    oedit_disp_menu( d );

    act( AT_ACTION, "$n starts using OLC.", ch, NULL, NULL, TO_CANSEE );
    return;
}

void do_ocopy( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    int                     ovnum,
                            cvnum;
    OBJ_INDEX_DATA         *orig;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Usage: ocopy <original> <new>\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    if ( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' ) {
        send_to_char( "Usage: ocopy <original> <new>\r\n", ch );
        return;
    }

    if ( !is_number( arg1 ) || !is_number( argument ) ) {
        send_to_char( "Values must be numeric.\r\n", ch );
        return;
    }

    ovnum = atoi( arg1 );
    cvnum = atoi( argument );

    if ( get_trust( ch ) < LEVEL_AJ_SGT ) {
        AREA_DATA              *pArea;

        if ( !ch->pcdata || !( pArea = ch->pcdata->area ) ) {
            send_to_char( "You must have an assigned area to copy objects.\r\n", ch );
            return;
        }

        if ( cvnum < pArea->low_o_vnum || cvnum > pArea->hi_o_vnum ) {
            send_to_char( "That number is not in your allocated range.\r\n", ch );
            return;
        }
    }

    if ( get_obj_index( cvnum ) ) {
        send_to_char( "Target vnum already exists.\r\n", ch );
        return;
    }

    if ( ( orig = get_obj_index( ovnum ) ) == NULL ) {
        send_to_char( "Source vnum does not exist.\r\n", ch );
        return;
    }
    make_object( cvnum, ovnum, orig->name );
    send_to_char( "Object copied.\r\n", ch );
    return;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/
/* For container flags */
void oedit_disp_container_flags_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    CHAR_DATA              *ch = d->character;
    char                    buf[MAX_STRING_LENGTH];
    int                     i;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( i = 0; i < 5; i++ ) {
        sprintf( buf, "&c%d&w) %s\r\n", i + 1, container_flags[i] );
        send_to_char_color( buf, ch );
    }
    sprintf( buf, "Container flags: &c%s&w\r\n", flag_string( obj->value[1], container_flags ) );
    send_to_char_color( buf, ch );

    send_to_char( "Enter flag, 0 to quit : ", ch );
    return;
}

/*
 * Display lever flags menu
 */
void oedit_disp_lever_flags_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    char                    buf[MAX_STRING_LENGTH];
    int                     counter;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < 29; counter++ ) {
        sprintf( buf, "&c%2d&w) %s\r\n", counter + 1, trig_flags[counter] );
        send_to_char_color( buf, d->character );
    }
    sprintf( buf, "Lever flags: &c%s&w\r\nEnter flag, 0 to quit: ",
             flag_string( obj->value[0], trig_flags ) );
    send_to_char_color( buf, d->character );
    return;
}

/*
 * Fancy layering stuff, trying to lessen confusion :)
 */
void oedit_disp_layer_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_LAYERS;
    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    send_to_char( "Choose which layer, or combination of layers fits best: \r\n\r\n",
                  d->character );
    ch_printf_color( d->character, "[&c%s&w] &g1&w) Nothing Layers\r\n",
                     ( obj->pIndexData->layers == 0 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g2&w) Silk Shirt\r\n",
                     IS_SET( obj->pIndexData->layers, 1 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g3&w) Leather Vest\r\n",
                     IS_SET( obj->pIndexData->layers, 2 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g4&w) Light Chainmail\r\n",
                     IS_SET( obj->pIndexData->layers, 4 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g5&w) Leather Jacket\r\n",
                     IS_SET( obj->pIndexData->layers, 8 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g6&w) Light Cloak\r\n",
                     IS_SET( obj->pIndexData->layers, 16 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g7&w) Loose Cloak\r\n",
                     IS_SET( obj->pIndexData->layers, 32 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g8&w) Cape\r\n",
                     IS_SET( obj->pIndexData->layers, 64 ) ? "X" : " " );
    ch_printf_color( d->character, "[&c%s&w] &g9&w) Magical Effects\r\n",
                     IS_SET( obj->pIndexData->layers, 128 ) ? "X" : " " );
    ch_printf_color( d->character, "\r\nLayer or 0 to exit: " );
}

/* For extra descriptions */
void oedit_disp_extradesc_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    CHAR_DATA              *ch = d->character;
    int                     count = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    if ( obj->pIndexData->first_extradesc ) {
        EXTRA_DESCR_DATA       *ed;

        for ( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next ) {
            ch_printf_color( ch, "&c%2d&w) Keyword: &O%s\r\n", ++count, ed->keyword );
        }
    }
    if ( obj->first_extradesc ) {
        EXTRA_DESCR_DATA       *ed;

        for ( ed = obj->first_extradesc; ed; ed = ed->next ) {
            ch_printf_color( ch, "&c%2d&w) Keyword: &O%s\r\n", ++count, ed->keyword );
        }
    }

    if ( obj->pIndexData->first_extradesc || obj->first_extradesc )
        send_to_char( "\r\n", d->character );

    ch_printf_color( d->character, "&cA&w) Add a new description\r\n" );
    ch_printf_color( d->character, "&cR&w) Remove a description\r\n" );
    ch_printf_color( d->character, "&cQ&w) Quit\r\n" );
    ch_printf_color( d->character, "\r\nEnter choice: " );

    OLC_MODE( d ) = OEDIT_EXTRADESC_MENU;
}

void oedit_disp_extra_choice( DESCRIPTOR_DATA *d )
{
    EXTRA_DESCR_DATA       *ed = ( EXTRA_DESCR_DATA * ) d->character->pcdata->spare_ptr;

    if ( !ed ) {
        oedit_disp_menu( d );
        return;
    }

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    ch_printf_color( d->character, "&c1&w) Keyword: &O%s\r\n",
                     ( ed && ed->keyword ) ? ed->keyword : "Not Set" );
    ch_printf_color( d->character, "&c2&w) Description: \r\n&O%s&w\r\n",
                     ( ed && ed->description ) ? ed->description : "Not Set" );
    ch_printf_color( d->character, "\r\nChange which option? " );

    OLC_MODE( d ) = OEDIT_EXTRADESC_CHOICE;
}

/* Ask for *which* apply to edit and prompt for some other options */
void oedit_disp_prompt_apply_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    CHAR_DATA              *ch = d->character;
    AFFECT_DATA            *paf;
    int                     counter = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next ) {
        ch_printf_color( d->character, " &c%2d&w) ", counter++ );
        showaffect( ch, paf );
    }

    for ( paf = obj->first_affect; paf; paf = paf->next ) {
        ch_printf_color( d->character, " &c%2d&w) ", counter++ );
        showaffect( ch, paf );
    }
    send_to_char_color( " \r\n &cA&w) Add an affect\r\n", ch );
    send_to_char_color( " &cR&w) Remove an affect\r\n", ch );
    send_to_char_color( " &cQ&w) Quit\r\n", ch );

    send_to_char( "\r\nEnter option or affect#: ", ch );
    OLC_MODE( d ) = OEDIT_AFFECT_MENU;
}

/*. Ask for liquid type .*/
void oedit_liquid_type( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    int                     counter,
                            col = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < LIQ_MAX; counter++ ) {
        sprintf( buf, " &w%2d&c) &c%-20.20s ", counter, liq_table[counter].liq_name );
        if ( ++col % 3 == 0 )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, d->character );
    }
    send_to_char_color( "\r\n&wEnter drink type: ", d->character );
    OLC_MODE( d ) = OEDIT_VALUE_3;

    return;
}

/*
 * Display the menu of apply types
 */
void oedit_disp_affect_menu( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    CHAR_DATA              *ch = d->character;
    int                     counter,
                            col = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < MAX_APPLY_TYPE; counter++ ) {
        /*
         * Don't want people choosing these ones 
         */
        if ( counter == 0 || counter == APPLY_EXT_AFFECT )
            continue;

        sprintf( buf, "&c%2d&w) %-20.20s ", counter, a_types[counter] );
        if ( ++col % 3 == 0 )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, ch );
    }
    send_to_char( "\r\nEnter apply type (0 to quit): ", ch );
    OLC_MODE( d ) = OEDIT_AFFECT_LOCATION;

    return;
}

/* spell type */
void oedit_disp_spells_menu( DESCRIPTOR_DATA *d )
{
}

/* object value 0 */
void oedit_disp_val1_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_1;

    switch ( obj->item_type ) {
        case ITEM_LIGHT:
            /*
             * values 0 and 1 are unused.. jump to 2 
             */
            oedit_disp_val3_menu( d );
            break;
        case ITEM_SALVE:
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_POTION:
            send_to_char( "Spell level : ", d->character );
            break;
        case ITEM_KEY:
            send_to_char( "Lock number : ", d->character );
            break;
        case ITEM_MISSILE_WEAPON:
        case ITEM_WEAPON:
            send_to_char( "Condition : ", d->character );
            break;
        case ITEM_ARMOR:
            send_to_char( "Current AC : ", d->character );
            break;
        case ITEM_INSTRUMENT:
            send_to_char( "Type 1) woodwind 2) strings 3) brass 4) drums: ", d->character );
            break;
        case ITEM_QUIVER:
        case ITEM_KEYRING:
        case ITEM_PIPE:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            send_to_char( "Enter Capacity ( 0 to 5000 ) : ", d->character );
            break;
        case ITEM_FOOD:
            send_to_char( "Hours to fill stomach : ", d->character );
            break;
        case ITEM_MONEY:
            send_to_char( "Amount of coins : ", d->character );
            break;
        case ITEM_HERB:
            /*
             * Value 0 unused, skip to 1 
             */
            oedit_disp_val2_menu( d );
            break;
        case ITEM_LEVER:
        case ITEM_SWITCH:
            oedit_disp_lever_flags_menu( d );
            break;

        default:
            oedit_disp_menu( d );
    }
}

/* object value 1 */
void oedit_disp_val2_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_2;

    switch ( obj->item_type ) {
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_POTION:
            send_to_char( "\r\nWhat sn number ( -1 to 528 ) would you like to set it to: ",
                          d->character );
            break;
        case ITEM_SALVE:
        case ITEM_HERB:
            send_to_char( "\r\nCharges: ", d->character );
            break;
        case ITEM_INSTRUMENT:
            send_to_char( "\r\nTune does not need to be set.\r\n", d->character );
            oedit_disp_val3_menu( d );
            break;
        case ITEM_PIPE:
            send_to_char( "\r\nNumber of draws: ", d->character );
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            send_to_char( "\r\nMax number of charges 1 to 20: ", d->character );
            break;
        case ITEM_MISSILE_WEAPON:
            send_to_char
                ( "\r\nIn additon to the projectile damage what minimum damage should the bow do?: ",
                  d->character );
            send_to_char( "\r\n( 0 to 10 is the option available. )\r\n", d->character );
            break;
        case ITEM_WEAPON:
        case ITEM_PROJECTILE:
            send_to_char( "\r\nNumber of damage dice : ", d->character );
            break;
        case ITEM_FOOD:
        case ITEM_TREASURE:
            send_to_char( "\r\nCondition: ", d->character );
            break;
        case ITEM_QUIVER:
        case ITEM_CONTAINER:
            oedit_disp_container_flags_menu( d );
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            send_to_char( "\r\nQuantity : ", d->character );
            break;
        case ITEM_ARMOR:
            send_to_char( "\r\nOriginal AC: ", d->character );
            break;

        default:
            oedit_disp_menu( d );
    }
}

/* object value 2 */
void oedit_disp_val3_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_3;

    switch ( obj->item_type ) {
        case ITEM_LIGHT:
            send_to_char( "Number of hours (0 = burnt, -1 is infinite) : ", d->character );
            break;
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_POTION:
            send_to_char( "\r\nWhat sn number ( -1 to 528 ) would you like to set it to: ",
                          d->character );
            break;
        case ITEM_HERB:
            send_to_char( "Enter the affect 0 - 5: ", d->character );
            send_to_char
                ( "0) Nothing 1) Poison 2) Detect_invis 3) Restore Mana 4) Cure Light 5) Refresh\r\n",
                  d->character );
            break;
        case ITEM_PROJECTILE:
            send_to_char( "\r\nSet dam size dice : ", d->character );
            break;
        case ITEM_SALVE:
            send_to_char( "Maximum number of charges 1 to 20: ", d->character );
            break;
        case ITEM_TREASURE:
            send_to_char( "Dragon Regeneration rate 0 to 3: ", d->character );
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            send_to_char( "Number of charges remaining : ", d->character );
            break;
        case ITEM_MONEY:
            send_to_char( "Coin Type : 0 - 4", d->character );
            break;
        case ITEM_WEAPON:
            send_to_char( "Size of damage dice : ", d->character );
            break;
        case ITEM_MISSILE_WEAPON:
            send_to_char
                ( "In addition to projectile damage what is the maximum damage the bow should do?: ",
                  d->character );
            send_to_char( "\r\n( 1 to 100 is the option available to you. )\r\n", d->character );
            break;
        case ITEM_INSTRUMENT:
            send_to_char( "Enter Condition 1 - 12 : ", d->character );
            break;
        case ITEM_CONTAINER:
        case ITEM_QUIVER:
            send_to_char( "\r\nVnum of key to open container (-1 for no key) : ", d->character );
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            oedit_liquid_type( d );
            break;
        default:
            oedit_disp_menu( d );
    }
}

/* object value 3 */
void oedit_disp_val4_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_4;
    CHAR_DATA              *ch = d->character;
    int                     counter,
                            col = 0;

    switch ( obj->item_type ) {
        case ITEM_CONTAINER:
            send_to_char( "\r\nSet the Container condition 0 to 12:\r\n", d->character );
            break;
        case ITEM_INSTRUMENT:
            send_to_char( "\r\nSet the bonus damage 0 to 10:\r\n", d->character );
            break;
        case ITEM_SALVE:
            send_to_char( "\r\nSet the delay 0 to 20:\r\n", d->character );
            break;
        case ITEM_PIPE:
            send_to_char( "\r\nSet the pipe flags 1 to 8:\r\n", d->character );
            send_to_char
                ( "1) tamped 2) lit 3) hot 4) dirty 5) filthy 6) going out 7) burt 8) full\r\n",
                  d->character );
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_PILL:
            send_to_char( "\r\nWhat sn number ( -1 to 528 ) would you like to set it to: ",
                          d->character );
            break;
        case ITEM_WEAPON:
            {
                for ( counter = 0; counter < DAM_MAX_TYPE; counter++ ) {
                    ch_printf( d->character, "&c%2d&w) %-20.20s ", counter, attack_table[counter] );
                    if ( ++col % 2 == 0 )
                        send_to_char( "\r\n", d->character );
                }
                send_to_char( "\r\nEnter Damage Type: ", d->character );
                break;
            }
        case ITEM_MISSILE_WEAPON:
            send_to_char( "Range able to hit target 1 - 10:", d->character );
            break;
        case ITEM_QUIVER:
            send_to_char( "Condition 0 to 12: ", d->character );
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
        case ITEM_FOOD:
            send_to_char( "Poisoned (0 = not poisoned) : ", d->character );
            break;
        default:
            oedit_disp_menu( d );
    }
}

/* object value 4 */
void oedit_disp_val5_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_5;
    CHAR_DATA              *ch = d->character;
    int                     counter,
                            col = 0;

    switch ( obj->item_type ) {
        case ITEM_SALVE:
            send_to_char( "\r\nWhat sn number ( -1 to 528 ) would you like to set it to: ",
                          d->character );
            break;
        case ITEM_PILL:
            send_to_char( "\r\nFood value: ", d->character );
            break;
        case ITEM_DRINK_CON:
            send_to_char( "\r\nWhat sn number ( -1 to 528 ) would you like to set it to: ",
                          d->character );
            break;
        case ITEM_INSTRUMENT:
            send_to_char
                ( "\r\nSongs played with this instrument will buff +X to each stat ( 0 to 2 ): ",
                  d->character );
            break;
        case ITEM_PROJECTILE:
            send_to_char( "\r\nSet damage type 0 bolt, 1 arrow, 2 dart, 3 stone : ", d->character );
            break;
        case ITEM_WEAPON:
            {
                for ( counter = 0; counter < WEP_MAX; counter++ ) {
                    ch_printf( d->character, "&c%2d&w) %-20.20s ", counter,
                               weapon_skills[counter] );
                    if ( ++col % 2 == 0 )
                        send_to_char( "\r\n", d->character );
                }
                send_to_char( "\r\nEnter Weapon Type: ", d->character );
                break;
            }
        case ITEM_MISSILE_WEAPON:
            send_to_char( "What type of missile weapon ( 6 archery, 7 blowgun ): ", d->character );
            break;
        default:
            oedit_disp_menu( d );
    }
}

/* object value 5 */
void oedit_disp_val6_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_6;
    CHAR_DATA              *ch = d->character;

    send_to_char( "\r\nEnter Reset Level: ", d->character );
}

/* object value 6 */
void oedit_disp_val7_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;

    OLC_MODE( d ) = OEDIT_VALUE_7;
    CHAR_DATA              *ch = d->character;

    switch ( obj->item_type ) {
        case ITEM_WEAPON:
        case ITEM_PROJECTILE:
            {
                if ( !CAN_SHARPEN( obj ) ) {
                    oedit_disp_menu( d );
                }
                send_to_char( "\r\nHow sharp the weapon is 0 - 100 : ", d->character );
                break;
            }
        case ITEM_MISSILE_WEAPON:
            send_to_char( "\r\nSet damage type 0 bolt, 1 arrow, 2 dart, 3 stone : ", d->character );
            break;
        case ITEM_SALVE:
            send_to_char( "\r\nWhat sn number do you want to set it to?", d->character );
            break;
        default:
            oedit_disp_menu( d );
    }
}

/* object type */
void oedit_disp_type_menu( DESCRIPTOR_DATA *d )
{
    char                    buf[MAX_STRING_LENGTH];
    int                     counter,
                            col = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < MAX_ITEM_TYPE; counter++ ) {
        sprintf( buf, "&c%2d&w) %-20.20s ", counter, o_types[counter] );
        if ( ++col % 3 == 0 )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, d->character );
    }
    send_to_char( "\r\nEnter object type: ", d->character );

    return;
}

/* object extra flags */
void oedit_disp_extra_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    char                    buf[MAX_STRING_LENGTH];
    int                     counter,
                            col = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    for ( counter = 0; counter < MAX_ITEM_FLAG; counter++ ) {
        sprintf( buf, "&c%2d&w) %-20.20s ", counter + 1, capitalize( o_flags[counter] ) );
        if ( ++col % 2 == 0 )
            strcat( buf, "\r\n" );
        send_to_char_color( buf, d->character );
    }
    sprintf( buf, "\r\nObject flags: &c%s&w\r\nEnter object extra flag (0 to quit): ",
             ext_flag_string( &obj->extra_flags, o_flags ) );
    send_to_char_color( buf, d->character );

    return;
}

/*
 * Display wear flags menu
 */
void oedit_disp_wear_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    char                    buf[MAX_STRING_LENGTH];
    int                     counter,
                            col = 0;

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );

    if ( obj->item_type == ITEM_MISSILE_WEAPON ) {
        send_to_char( "\r\n&c1.&W take         &c19. &Wmissile&c\r\n", d->character );
    }
    else {
        for ( counter = 0; counter <= ITEM_WEAR_MAX; counter++ ) {
            if ( 1 << counter == ITEM_DUAL_WIELD )
                continue;

            sprintf( buf, "&c%2d&w) %-20.20s ", counter + 1, capitalize( w_flags[counter] ) );
            if ( ++col % 2 == 0 )
                strcat( buf, "\r\n" );
            send_to_char_color( buf, d->character );
        }
    }
    sprintf( buf, "\r\nWear flags: &c%s&w\r\nEnter wear flag, 0 to quit:  ",
             flag_string( obj->wear_flags, w_flags ) );
    send_to_char_color( buf, d->character );
    return;
}

/* display main menu */
void oedit_disp_menu( DESCRIPTOR_DATA *d )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    char                    buf[MAX_STRING_LENGTH];

    ch_printf_color( d->character, "\r\n\r\n&Y6 Dragons Oasis OLC menu&D\r\n" );
    /*
     * . Build first half of menu .
     */
    set_char_color( AT_PLAIN, d->character );
    sprintf( buf,
             "\r\n\r\n-- Item number : [&C%d&w]\r\n"
             "&c1&w) Name     : &C%s\r\n"
             "&c2&w) S-Desc   : &C%s\r\n"
             "&c3&w) L-Desc   :-\r\n&C%s\r\n"
             "&c4&w) A-Desc   :-\r\n&C%s\r\n"
             "&c5&w) Type        : &C%s\r\n"
             "&c6&w) Extra flags : &C%s\r\n",
             obj->pIndexData->vnum,
             obj->name, obj->short_descr, obj->description,
             obj->action_desc ? obj->action_desc : "<not set>\r\n",
             capitalize( item_type_name( obj ) ), ext_flag_string( &obj->extra_flags, o_flags ) );
    send_to_char_color( buf, d->character );

    /*
     * Build second half of the menu 
     */
    sprintf( buf,
             "&c7&w) Wear flags  : &C%s\r\n" "&c8&w) Weight      : &C%d\r\n"
             "&c9&w) Cost        : &C%d\r\n" "&cA&w) Color       : &C%d\r\n"
             "&cB&w) Timer       : &C%d\r\n" "&cC&w) Level       : &C%d\r\n"
             "&cD&w) Layers      : &C%d\r\n"
             "&cE&w) Val0 : [&C%d] &cF&w) Val1 : [&C%d] &cG&w) Val2 : [&C%d] &cH&w) Val3 : [&C%d] \r\n&cI&w) Val4 : [&C%d] &cJ&w) Val5 : [&C%d] &cK&w) Val6 : [&C%d]\r\n"
             "&cL&w) Affect menu\r\n" "&cM&w) Extra descriptions menu\r\n"
             "&cN&w) Currtype    : &c%s\r\n" "&cQ&w) Quit\r\n" "Enter choice : ",
             flag_string( obj->wear_flags, w_flags ), obj->weight, obj->cost, obj->color,
             obj->timer, obj->level, obj->pIndexData->layers, obj->value[0], obj->value[1],
             obj->value[2], obj->value[3], obj->value[4], obj->value[5], obj->value[6],
             curr_types[obj->currtype] );
    send_to_char_color( buf, d->character );
    OLC_MODE( d ) = OEDIT_MAIN_MENU;

    return;
}

/***************************************************************************
 Object affect editing/removing functions
 ***************************************************************************/
void edit_object_affect( DESCRIPTOR_DATA *d, int number )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    int                     count = 0;
    AFFECT_DATA            *paf;

    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next ) {
        if ( count == number ) {
            d->character->pcdata->spare_ptr = paf;
            OLC_VAL( d ) = TRUE;
            oedit_disp_affect_menu( d );
            return;
        }
        count++;
    }

    for ( paf = obj->first_affect; paf; paf = paf->next ) {
        if ( count == number ) {
            d->character->pcdata->spare_ptr = paf;
            OLC_VAL( d ) = TRUE;
            oedit_disp_affect_menu( d );
            return;
        }
        count++;
    }

    send_to_char( "Affect not found.\r\n", d->character );
}

void remove_affect_from_obj( OBJ_DATA *obj, int number )
{
    int                     count = 0;
    AFFECT_DATA            *paf;

    if ( obj->pIndexData->first_affect ) {
        for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next ) {
            if ( count == number ) {
                UNLINK( paf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next,
                        prev );
                DISPOSE( paf );
                --top_affect;
                return;
            }
            count++;
        }
    }

    if ( obj->first_affect ) {
        for ( paf = obj->first_affect; paf; paf = paf->next ) {
            if ( count == number ) {
                UNLINK( paf, obj->first_affect, obj->last_affect, next, prev );
                DISPOSE( paf );
                --top_affect;
                return;
            }
            count++;
        }
    }
}

EXTRA_DESCR_DATA       *oedit_find_extradesc( OBJ_DATA *obj, int number )
{
    int                     count = 0;
    EXTRA_DESCR_DATA       *ed;

    for ( ed = obj->pIndexData->first_extradesc; ed; ed = ed->next ) {
        if ( ++count == number )
            return ed;
    }

    for ( ed = obj->first_extradesc; ed; ed = ed->next ) {
        if ( ++count == number )
            return ed;
    }

    return NULL;
}

/*
 * Bogus command for resetting stuff
 */
void do_oedit_reset( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) ch->pcdata->dest_buf;
    EXTRA_DESCR_DATA       *ed = ( EXTRA_DESCR_DATA * ) ch->pcdata->spare_ptr;
    MPROG_DATA             *mprg = ( MPROG_DATA * ) ch->pcdata->spare_ptr;
    int                     mode = OLC_MODE( ch->desc );

    switch ( ch->substate ) {
        default:
            return;

        case SUB_OBJ_EXTRA:
            if ( !ch->pcdata->dest_buf ) {
                send_to_char( "Fatal error, report to Tagith.\r\n", ch );
                bug( "do_oedit_reset: sub_obj_extra: NULL ch->pcdata->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            /*
             * OLC_DESC(ch->desc) = ch->pcdata->spare_ptr; 
             */
            STRFREE( ed->description );
            ed->description = copy_buffer( ch );
            stop_editing( ch );
            ch->pcdata->dest_buf = obj;
            ch->pcdata->spare_ptr = ed;
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_OEDIT;
            OLC_MODE( ch->desc ) = OEDIT_EXTRADESC_CHOICE;
            oedit_disp_extra_choice( ch->desc );
            return;

        case SUB_OBJ_LONG:
            if ( !ch->pcdata->dest_buf ) {
                send_to_char( "Fatal error, report to Tagith.\r\n", ch );
                bug( "do_oedit_reset: sub_obj_long: NULL ch->pcdata->dest_buf" );
                ch->substate = SUB_NONE;
                return;
            }
            STRFREE( obj->description );
            obj->description = copy_buffer( ch );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                STRFREE( obj->pIndexData->description );
                obj->pIndexData->description = QUICKLINK( obj->description );
            }
            stop_editing( ch );
            ch->pcdata->dest_buf = obj;
            ch->desc->connected = CON_OEDIT;
            ch->substate = SUB_NONE;
            OLC_MODE( ch->desc ) = OEDIT_MAIN_MENU;
            oedit_disp_menu( ch->desc );
            return;
/*
        case SUB_MPROG_EDIT:
            if ( mprg->comlist )
                STRFREE( mprg->comlist );
            mprg->comlist = copy_buffer( ch );
            stop_editing( ch );
            ch->pcdata->dest_buf = obj;
            ch->desc->connected = ch->tempnum;
            ch->substate = SUB_NONE;
            OLC_MODE( ch->desc ) = mode;
            oedit_disp_prog_choice( ch->desc );
            return;
*/
    }
}

const struct obj_color_type freak_color[] = {
    {"light blue"},
    {"orange"},
    {"cyan"},
    {"red"},
    {"blue"},
    {"white"},
    {"blood"},
    {"dblue"},
    {"grey"},
    {"green"},
    {"pink"},
    {"dgreen"},
    {"purple"},
    {"dgrey"},
    {"yellow"}
};

/*
 * This function interprets the arguments that the character passed
 * to it based on which OLC mode you are in at the time
 */
void oedit_parse( DESCRIPTOR_DATA *d, char *arg )
{
    OBJ_DATA               *obj = ( OBJ_DATA * ) d->character->pcdata->dest_buf;
    AFFECT_DATA            *paf = ( AFFECT_DATA * ) d->character->pcdata->spare_ptr;
    AFFECT_DATA            *npaf;
    EXTRA_DESCR_DATA       *ed = ( EXTRA_DESCR_DATA * ) d->character->pcdata->spare_ptr;
    MPROG_DATA             *mprg = ( MPROG_DATA * ) d->character->pcdata->spare_ptr;
    MPROG_DATA             *mprog = obj->pIndexData->mudprogs;
    char                    arg1[MAX_INPUT_LENGTH];
    int                     number = 0,
        max_val,
        min_val,
        value;

    /*
     * bool found; 
     */

    switch ( OLC_MODE( d ) ) {
        case OEDIT_CONFIRM_SAVESTRING:
            switch ( *arg ) {
                case 'y':
                case 'Y':
                    send_to_char( "Saving object to memory.\r\n", d->character );
                    /*
                     * oedit_save_internally(d); 
                     */
                case 'n':
                case 'N':
                    cleanup_olc( d );
                    return;
                default:
                    send_to_char( "Invalid choice!\r\n", d->character );
                    send_to_char( "Do you wish to save this object internally?\r\n", d->character );
                    return;
            }

        case OEDIT_MAIN_MENU:
            /*
             * switch to whichever mode the user selected, display prompt or menu 
             */
            switch ( UPPER( arg[0] ) ) {
                case 'Q':
                case 'q':
                    /*
                     * send_to_char( "Do you wish to save this object internally?: ",
                     * d->character ); OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING; 
                     */
                    cleanup_olc( d );
                    return;
                case '1':
                    send_to_char( "Enter namelist : ", d->character );
                    OLC_MODE( d ) = OEDIT_EDIT_NAMELIST;
                    break;
                case '2':
                    send_to_char( "Enter short desc : ", d->character );
                    OLC_MODE( d ) = OEDIT_SHORTDESC;
                    break;
                case '3':
                    send_to_char( "Enter long desc :-\r\n| ", d->character );
                    OLC_MODE( d ) = OEDIT_LONGDESC;
                    break;
                case '4':
                    /*
                     * lets not 
                     */
                    send_to_char( "Enter action desc :-\r\n", d->character );
                    OLC_MODE( d ) = OEDIT_ACTDESC;
                    break;
                case '5':
                    oedit_disp_type_menu( d );
                    OLC_MODE( d ) = OEDIT_TYPE;
                    break;
                case '6':
                    oedit_disp_extra_menu( d );
                    OLC_MODE( d ) = OEDIT_EXTRAS;
                    break;
                case '7':
                    oedit_disp_wear_menu( d );
                    OLC_MODE( d ) = OEDIT_WEAR;
                    break;
                case '8':
                    send_to_char( "Enter weight : ", d->character );
                    OLC_MODE( d ) = OEDIT_WEIGHT;
                    break;
                case '9':
                    send_to_char( "Enter cost : ", d->character );
                    OLC_MODE( d ) = OEDIT_COST;
                    break;
                case 'A':
                    {
                        int                     counter,
                                                col = 0;

                        send_to_char( "\r\nAvailable object colors\r\n", d->character );
                        for ( counter = 0; counter < MAX_NUMBER; counter++ ) {
                            ch_printf( d->character, "&c%2d&w) %-20.20s ", counter,
                                       freak_color[counter] );
                            if ( ++col % 2 == 0 )
                                send_to_char( "\r\n", d->character );
                        }
                        send_to_char
                            ( "\r\nEnter the number for the corresponding color of the object: ",
                              d->character );
                        OLC_MODE( d ) = OEDIT_COSTPERDAY;
                        break;
                    }
                case 'B':
                    send_to_char( "Enter timer : ", d->character );
                    OLC_MODE( d ) = OEDIT_TIMER;
                    break;
                case 'C':
                    send_to_char( "Enter level : ", d->character );
                    OLC_MODE( d ) = OEDIT_LEVEL;
                    break;
                case 'D':
                    if ( IS_SET( obj->wear_flags, ITEM_WEAR_BODY )
                         || IS_SET( obj->wear_flags, ITEM_WEAR_ABOUT )
                         || IS_SET( obj->wear_flags, ITEM_WEAR_ARMS )
                         || IS_SET( obj->wear_flags, ITEM_WEAR_FEET )
                         || IS_SET( obj->wear_flags, ITEM_WEAR_HANDS )
                         || IS_SET( obj->wear_flags, ITEM_WEAR_LEGS )
                         || IS_SET( obj->wear_flags, ITEM_WEAR_WAIST ) ) {
                        oedit_disp_layer_menu( d );
                        OLC_MODE( d ) = OEDIT_LAYERS;
                    }
                    else
                        send_to_char( "The wear location of this object is not layerable.\r\n",
                                      d->character );
                    break;
                case 'E':
                    oedit_disp_val1_menu( d );
                    break;
                case 'F':
                    oedit_disp_val2_menu( d );
                    break;
                case 'G':
                    oedit_disp_val3_menu( d );
                    break;
                case 'H':
                    oedit_disp_val4_menu( d );
                    break;
                case 'I':
                    oedit_disp_val5_menu( d );
                    break;
                case 'J':
                    oedit_disp_val6_menu( d );
                    break;
                case 'K':
                    oedit_disp_val7_menu( d );
                    break;
                case 'L':
                    oedit_disp_prompt_apply_menu( d );
                    break;
                case 'M':
                    oedit_disp_extradesc_menu( d );
                    break;
                case 'N':
                    send_to_char( "Enter currtype : ", d->character );
                    OLC_MODE( d ) = OEDIT_CURRTYPE;
                    break;
                default:
                    oedit_disp_menu( d );
                    break;
            }
            return;                                    /* end of OEDIT_MAIN_MENU */

        case OEDIT_EDIT_NAMELIST:
            STRFREE( obj->name );
            obj->name = STRALLOC( arg );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                STRFREE( obj->pIndexData->name );
                obj->pIndexData->name = QUICKLINK( obj->name );
            }
            olc_log( d, "Changed name to %s", obj->name );
            break;

        case OEDIT_SHORTDESC:
            STRFREE( obj->short_descr );
            obj->short_descr = STRALLOC( arg );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                STRFREE( obj->pIndexData->short_descr );
                obj->pIndexData->short_descr = QUICKLINK( obj->short_descr );
            }
            olc_log( d, "Changed short to %s", obj->short_descr );
            break;

        case OEDIT_LONGDESC:
            STRFREE( obj->description );
            obj->description = STRALLOC( arg );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                STRFREE( obj->pIndexData->description );
                obj->pIndexData->description = QUICKLINK( obj->description );
            }
            olc_log( d, "Changed longdesc to %s", obj->description );
            break;

        case OEDIT_ACTDESC:
            STRFREE( obj->action_desc );
            obj->action_desc = STRALLOC( arg );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                STRFREE( obj->pIndexData->action_desc );
                obj->pIndexData->action_desc = QUICKLINK( obj->action_desc );
            }
            olc_log( d, "Changed actiondesc to %s", obj->action_desc );
            break;

        case OEDIT_TYPE:
            if ( is_number( arg ) )
                number = atoi( arg );
            else
                number = get_otype( arg );

            if ( ( number < 1 ) || ( number >= MAX_ITEM_TYPE ) ) {
                send_to_char( "Invalid choice, try again : ", d->character );
                return;
            }
            else {
                obj->item_type = ( short ) number;
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->item_type = obj->item_type;
            }
            olc_log( d, "Changed object type to %s", o_types[number] );
            break;

        case OEDIT_EXTRAS:
            while ( arg[0] != '\0' ) {
                arg = one_argument( arg, arg1 );
                if ( is_number( arg1 ) ) {
                    number = atoi( arg1 );

                    if ( number == 0 ) {
                        oedit_disp_menu( d );
                        return;
                    }

                    number -= 1;                       /* Offset for 0 */
                    if ( number < 0 || number > MAX_ITEM_FLAG ) {
                        oedit_disp_extra_menu( d );
                        return;
                    }
                }
                else {
                    number = get_oflag( arg1 );
                    if ( number < 0 || number > MAX_BITS ) {
                        oedit_disp_extra_menu( d );
                        return;
                    }
                }

                if ( number == ITEM_PROTOTYPE && get_trust( d->character ) < LEVEL_AJ_SGT
                     && !is_name( "protoflag", d->character->pcdata->bestowments ) )
                    send_to_char( "You cannot change the prototype flag.\r\n", d->character );
                else {
                    xTOGGLE_BIT( obj->extra_flags, number );
                    olc_log( d, "%s the flag %s",
                             xIS_SET( obj->extra_flags, number ) ? "Added" : "Removed",
                             o_flags[number] );
                }

                /*
                 * If you used a number, you can only do one flag at a time 
                 */
                if ( is_number( arg ) )
                    break;
            }
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                obj->pIndexData->extra_flags = obj->extra_flags;
            oedit_disp_extra_menu( d );
            return;

        case OEDIT_WEAR:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 )
                    break;
                else if ( number < 0 || number > ITEM_WEAR_MAX + 1 ) {
                    send_to_char( "Invalid flag, try again: ", d->character );
                    return;
                }
                else {
                    number -= 1;                       /* Offset to accomodate 0 */
                    TOGGLE_BIT( obj->wear_flags, 1 << number );
                    olc_log( d, "%s the wearloc %s",
                             IS_SET( obj->wear_flags, 1 << number ) ? "Added" : "Removed",
                             w_flags[number] );
                }
            }
            else {
                while ( arg[0] != '\0' ) {
                    arg = one_argument( arg, arg1 );
                    number = get_wflag( arg1 );
                    if ( number != -1 ) {
                        TOGGLE_BIT( obj->wear_flags, 1 << number );
                        olc_log( d, "%s the wearloc %s",
                                 IS_SET( obj->wear_flags, 1 << number ) ? "Added" : "Removed",
                                 w_flags[number] );
                    }
                }
            }
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                obj->pIndexData->wear_flags = obj->wear_flags;
            oedit_disp_wear_menu( d );
            return;

        case OEDIT_WEIGHT:
            number = atoi( arg );
            obj->weight = number;
            olc_log( d, "Changed weight to %d", obj->weight );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                obj->pIndexData->weight = obj->weight;
            break;

        case OEDIT_COST:
            number = atoi( arg );
            obj->cost = number;
            olc_log( d, "Changed cost to %d", obj->cost );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                obj->pIndexData->cost = obj->cost;
            break;

        case OEDIT_CURRTYPE:
            obj->currtype = get_currency_type( arg );
            olc_log( d, "Changed currtype to %s", curr_types[obj->currtype] );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                obj->pIndexData->currtype = obj->currtype;
            break;

        case OEDIT_COSTPERDAY:
            number = atoi( arg );
            obj->color = number;
            olc_log( d, "Changed color to %d", obj->color );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                obj->pIndexData->color = obj->color;
            break;

        case OEDIT_TIMER:
            number = atoi( arg );
            obj->timer = number;
            olc_log( d, "Changed timer to %d", obj->timer );
            break;

        case OEDIT_LEVEL:
            number = atoi( arg );
            obj->level = URANGE( 0, number, MAX_LEVEL );
            olc_log( d, "Changed object level to %d", obj->level );
            if ( obj->item_type == ITEM_ARMOR ) {
                obj->value[0] = set_min_armor( obj->level );
                obj->value[1] = set_max_armor( obj->level );
                obj->pIndexData->value[0] = set_min_armor( obj->level );
                obj->pIndexData->value[1] = set_max_armor( obj->level );
                send_to_char( "Now establishing armor AC values.\r\n", d->character );
            }
            else if ( obj->item_type == ITEM_WEAPON ) {
                obj->value[1] = set_min_chart( obj->level );
                obj->value[2] = set_max_chart( obj->level );
                obj->pIndexData->value[1] = set_min_chart( obj->level );
                obj->pIndexData->value[2] = set_max_chart( obj->level );
                send_to_char( "Now establishing weapon damage values.\r\n", d->character );
            }
            break;

        case OEDIT_LAYERS:
            /*
             * Like they say, easy on the user, hard on the programmer :) 
             */
            /*
             * Or did I just make that up.... 
             */
            number = atoi( arg );
            switch ( number ) {
                case 0:
                    oedit_disp_menu( d );
                    return;
                case 1:
                    obj->pIndexData->layers = 0;
                    break;
                case 2:
                    TOGGLE_BIT( obj->pIndexData->layers, 1 );
                    break;
                case 3:
                    TOGGLE_BIT( obj->pIndexData->layers, 2 );
                    break;
                case 4:
                    TOGGLE_BIT( obj->pIndexData->layers, 4 );
                    break;
                case 5:
                    TOGGLE_BIT( obj->pIndexData->layers, 8 );
                    break;
                case 6:
                    TOGGLE_BIT( obj->pIndexData->layers, 16 );
                    break;
                case 7:
                    TOGGLE_BIT( obj->pIndexData->layers, 32 );
                    break;
                case 8:
                    TOGGLE_BIT( obj->pIndexData->layers, 64 );
                    break;
                case 9:
                    TOGGLE_BIT( obj->pIndexData->layers, 128 );
                    break;
                default:
                    send_to_char( "Invalid selection, try again: ", d->character );
                    return;
            }
            olc_log( d, "Changed layers to %d", obj->pIndexData->layers );
            oedit_disp_layer_menu( d );
            return;

        case OEDIT_VALUE_1:
            {
                if ( obj->item_type == ITEM_ARMOR ) {
                    if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                        write_to_descriptor( d,
                                             "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                             0 );
                        break;
                    }
                }

                number = atoi( arg );
                switch ( obj->item_type ) {
                    case ITEM_SALVE:
                    case ITEM_PILL:
                    case ITEM_SCROLL:
                    case ITEM_WAND:
                    case ITEM_STAFF:
                    case ITEM_POTION:
                        min_val = 0;
                        max_val = 108;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val1_menu( d );
                            return;
                        }
                        break;
                    case ITEM_MISSILE_WEAPON:
                    case ITEM_WEAPON:
                        min_val = 0;
                        max_val = 12;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val1_menu( d );
                            return;
                        }
                        break;
                    case ITEM_QUIVER:
                    case ITEM_KEYRING:
                    case ITEM_PIPE:
                    case ITEM_CONTAINER:
                    case ITEM_DRINK_CON:
                    case ITEM_FOUNTAIN:
                    case ITEM_MONEY:
                        min_val = 0;
                        max_val = 5000;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val1_menu( d );
                            return;
                        }
                        break;
                    case ITEM_INSTRUMENT:
                        min_val = 1;
                        max_val = 4;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val1_menu( d );
                            return;
                        }
                        break;
                    case ITEM_FOOD:
                        min_val = 1;
                        max_val = 100;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val1_menu( d );
                            return;
                        }
                        break;
                    case ITEM_ARMOR:
                        min_val = 0;
                        max_val = 300;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val1_menu( d );
                            return;
                        }
                        break;
                    case ITEM_KEY:
                        break;
                    case ITEM_LEVER:
                    case ITEM_SWITCH:
                        if ( number < 0 || number > 29 )
                            oedit_disp_lever_flags_menu( d );
                        else {
                            if ( number != 0 ) {
                                TOGGLE_BIT( obj->value[0], 1 << ( number - 1 ) );
                                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                                    TOGGLE_BIT( obj->pIndexData->value[0], 1 << ( number - 1 ) );
                                oedit_disp_val1_menu( d );
                            }
                            else
                                oedit_disp_val2_menu( d );
                        }
                        break;

                    default:
                        obj->value[0] = number;
                        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                            obj->pIndexData->value[0] = number;
                        break;
                }
                obj->value[0] = number;
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->value[0] = number;
                olc_log( d, "Changed v0 to %d", obj->value[0] );
                oedit_disp_val2_menu( d );
                return;
            }

        case OEDIT_VALUE_2:
            {
                if ( obj->item_type == ITEM_ARMOR ) {
                    if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                        write_to_descriptor( d,
                                             "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                             0 );
                        break;
                    }
                }
                if ( obj->item_type == ITEM_WEAPON ) {
                    if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                        write_to_descriptor( d,
                                             "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                             0 );
                        break;
                    }
                }

                number = atoi( arg );
                switch ( obj->item_type ) {
                    case ITEM_ARMOR:
                        min_val = 0;
                        max_val = 300;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val2_menu( d );
                            return;
                        }
                        break;
                    case ITEM_SALVE:
                    case ITEM_HERB:
                    case ITEM_WAND:
                    case ITEM_STAFF:
                        min_val = 0;
                        max_val = 20;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val2_menu( d );
                            return;
                        }
                        break;
                    case ITEM_PIPE:
                        min_val = 0;
                        max_val = 8;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val2_menu( d );
                            return;
                        }
                        break;
                    case ITEM_MISSILE_WEAPON:
                        min_val = 0;
                        max_val = 10;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val2_menu( d );
                            return;
                        }
                        break;
                    case ITEM_WEAPON:
                    case ITEM_PROJECTILE:
                    case ITEM_DRINK_CON:
                    case ITEM_FOUNTAIN:
                        min_val = 0;
                        max_val = 100;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val2_menu( d );
                            return;
                        }
                        break;
                    case ITEM_FOOD:
                    case ITEM_TREASURE:
                        min_val = 0;
                        max_val = 12;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val2_menu( d );
                            return;
                        }
                        break;
                    case ITEM_PILL:
                    case ITEM_SCROLL:
                    case ITEM_POTION:
                        if ( !is_number( arg ) )
                            number = skill_lookup( arg );
                        obj->value[1] = number;
                        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                            obj->pIndexData->value[1] = number;
                        oedit_disp_val3_menu( d );
                        break;
                    case ITEM_CONTAINER:
                    case ITEM_QUIVER:
                        number = atoi( arg );
                        if ( number < 0 || number > 5 )
                            oedit_disp_container_flags_menu( d );
                        else {
                            /*
                             * if 0, quit 
                             */
                            if ( atoi( arg ) != 0 ) {
                                switch ( atoi( arg ) ) {
                                    case 1:
                                        arg = STRALLOC( "closeable" );
                                        break;
                                    case 2:
                                        arg = STRALLOC( "pickproof" );
                                        break;
                                    case 3:
                                        arg = STRALLOC( "closed" );
                                        break;
                                    case 4:
                                        arg = STRALLOC( "locked" );
                                        break;
                                    case 5:
                                        arg = STRALLOC( "eatkey" );
                                        break;
                                }
                                number = get_containerflag( arg );
                                TOGGLE_BIT( obj->value[1], 1 << number );
                                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                                    TOGGLE_BIT( obj->pIndexData->value[1], 1 << number );
                                oedit_disp_container_flags_menu( d );
                                return;
                            }
                            else {
                                oedit_disp_val3_menu( d );
                                return;
                            }
                            break;
                        }
                    default:
                        obj->value[1] = number;
                        if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                            obj->pIndexData->value[1] = number;
                        break;
                }
                olc_log( d, "Changed v1 to %d", obj->value[1] );
                break;
            }

        case OEDIT_VALUE_3:
            {
                if ( obj->item_type == ITEM_WEAPON ) {
                    if ( d->character->level < 106 && !is_head_architect( d->character ) ) {
                        write_to_descriptor( d,
                                             "\r\nYou cannot readjust numattacks, contact the head of Area Architect for more information.\r\n",
                                             0 );
                        break;
                    }
                }

                number = atoi( arg );
                /*
                 * Some error checking done here 
                 */
                switch ( obj->item_type ) {
                    case ITEM_SCROLL:
                    case ITEM_POTION:
                    case ITEM_PILL:
                        min_val = -1;
                        max_val = top_sn - 1;
                        if ( !is_number( arg ) )
                            number = skill_lookup( arg );
                        break;
                    case ITEM_HERB:
                        min_val = 0;
                        max_val = 5;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_LIGHT:
                        min_val = -1;
                        max_val = 100;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_INSTRUMENT:
                        min_val = 1;
                        max_val = 12;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_SALVE:
                        min_val = 0;
                        max_val = 20;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_TREASURE:
                        min_val = 0;
                        max_val = 3;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_WAND:
                    case ITEM_STAFF:
                        min_val = 0;
                        max_val = 20;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_QUIVER:
                    case ITEM_CONTAINER:
                        break;
                    case ITEM_MONEY:
                        min_val = 0;
                        max_val = 4;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_WEAPON:
                    case ITEM_PROJECTILE:
                    case ITEM_MISSILE_WEAPON:
                        min_val = 0;
                        max_val = 100;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val3_menu( d );
                            return;
                        }
                        break;
                    case ITEM_DRINK_CON:
                    case ITEM_FOUNTAIN:
                        min_val = 0;
                        max_val = LIQ_MAX;
                        break;
                    default:
                        /*
                         * Would require modifying if you have bvnum 
                         */
                        min_val = -32000;
                        max_val = 32000;
                        break;
                }
                obj->value[2] = URANGE( min_val, number, max_val );
                olc_log( d, "Changed v2 to %d", obj->value[2] );
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->value[2] = obj->value[2];
                oedit_disp_val4_menu( d );
                return;
            }

        case OEDIT_VALUE_4:
            {
                number = atoi( arg );
                switch ( obj->item_type ) {
                    case ITEM_QUIVER:
                    case ITEM_CONTAINER:
                        {
                            min_val = 1;
                            max_val = 12;
                            if ( number < min_val || number > max_val ) {
                                oedit_disp_val4_menu( d );
                                return;
                            }
                            obj->value[3] = URANGE( min_val, number, max_val );
                            olc_log( d, "Changed v3 to %d", obj->value[3] );
                            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                                obj->pIndexData->value[3] = obj->value[3];
                            oedit_disp_val6_menu( d );
                            return;
                            break;
                        }
                    case ITEM_DRINK_CON:
                    case ITEM_FOOD:
                    case ITEM_FOUNTAIN:
                        min_val = 0;
                        max_val = 1;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val4_menu( d );
                            return;
                        }
                        break;
                    case ITEM_MISSILE_WEAPON:
                        min_val = 1;
                        max_val = 10;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val4_menu( d );
                            return;
                        }
                        break;
                    case ITEM_INSTRUMENT:
                        min_val = 1;
                        max_val = 10;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val4_menu( d );
                            return;
                        }
                        break;
                    case ITEM_SALVE:
                        min_val = 1;
                        max_val = 20;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val4_menu( d );
                            return;
                        }
                        break;
                    case ITEM_PIPE:
                        min_val = 1;
                        max_val = 8;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val4_menu( d );
                            return;
                        }
                        break;

                    case ITEM_PILL:
                    case ITEM_SCROLL:
                    case ITEM_POTION:
                    case ITEM_WAND:
                    case ITEM_STAFF:
                        min_val = -1;
                        max_val = top_sn - 1;
                        if ( !is_number( arg ) )
                            number = skill_lookup( arg );
                        break;
                    case ITEM_WEAPON:
                        min_val = 1;
                        max_val = 7;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val4_menu( d );
                            return;
                        }
                        break;
                    default:
                        min_val = -32000;
                        max_val = 32000;
                        break;
                }
                obj->value[3] = URANGE( min_val, number, max_val );
                olc_log( d, "Changed v3 to %d", obj->value[3] );
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->value[3] = obj->value[3];
                oedit_disp_val5_menu( d );
                return;
            }
        case OEDIT_VALUE_5:
            {
                number = atoi( arg );
                switch ( obj->item_type ) {
                    case ITEM_SALVE:
                        if ( !is_number( arg ) )
                            number = skill_lookup( arg );
                        min_val = -1;
                        max_val = top_sn - 1;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    case ITEM_DRINK_CON:
                        if ( !is_number( arg ) )
                            number = skill_lookup( arg );
                        min_val = -1;
                        max_val = top_sn - 1;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    case ITEM_INSTRUMENT:
                        min_val = 0;
                        max_val = 2;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    case ITEM_PILL:
                        min_val = 0;
                        max_val = 32000;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    case ITEM_PROJECTILE:
                        min_val = 0;
                        max_val = 3;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    case ITEM_MISSILE_WEAPON:
                        min_val = 6;
                        max_val = 7;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    case ITEM_WEAPON:
                        min_val = 0;
                        max_val = 15;
                        if ( number < min_val || number > max_val ) {
                            oedit_disp_val5_menu( d );
                            return;
                        }
                        break;
                    default:
                        min_val = -32000;
                        max_val = 32000;
                        break;
                }
                obj->value[4] = URANGE( min_val, number, max_val );
                olc_log( d, "Changed v4 to %d", obj->value[4] );
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->value[4] = obj->value[4];
                oedit_disp_val6_menu( d );
                return;
            }
        case OEDIT_VALUE_6:
            {
                number = atoi( arg );
                min_val = 0;
                max_val = 108;
                if ( number < min_val || number > max_val ) {
                    oedit_disp_val6_menu( d );
                    return;
                }
                obj->value[5] = URANGE( min_val, number, max_val );
                olc_log( d, "Changed v5 to %d", obj->value[5] );
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->value[5] = obj->value[5];
                oedit_disp_val7_menu( d );
                return;
            }

        case OEDIT_VALUE_7:
            {
                number = atoi( arg );
                switch ( obj->item_type ) {
                    case ITEM_WEAPON:
                    case ITEM_PROJECTILE:
                        {
                            min_val = 0;
                            max_val = 100;
                            if ( number < min_val || number > max_val ) {
                                oedit_disp_val7_menu( d );
                                return;
                            }
                            break;
                        }
                    case ITEM_MISSILE_WEAPON:
                        {
                            min_val = 0;
                            max_val = 3;
                            if ( number < min_val || number > max_val ) {
                                oedit_disp_val7_menu( d );
                                return;
                            }
                            break;
                        }
                    case ITEM_SALVE:
                        {
                            if ( !is_number( arg ) )
                                number = skill_lookup( arg );
                            min_val = -1;
                            max_val = top_sn - 1;
                            if ( number < min_val || number > max_val ) {
                                oedit_disp_val7_menu( d );
                                return;
                            }
                            break;
                        }
                }
                obj->value[6] = number;

                if ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_PROJECTILE ) {
                    if ( obj->value[6] > 100 )
                        obj->value[6] = 100;
                    // update_weapon(NULL, obj);
                }

                olc_log( d, "Changed v6 to %d", obj->value[6] );
                if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                    obj->pIndexData->value[6] = number;

                oedit_disp_menu( d );
                return;
            }

        case OEDIT_AFFECT_MENU:
            number = atoi( arg );

            switch ( arg[0] ) {
                default:                              /* if its a number, then its * * * 
                                                        * prolly for editing an affect */
                    if ( is_number( arg ) )
                        edit_object_affect( d, number );
                    else
                        oedit_disp_prompt_apply_menu( d );
                    return;

                case 'r':
                case 'R':
                    /*
                     * Chop off the 'R', if theres a number following use it, otherwise
                     * prompt for input 
                     */
                    arg = one_argument( arg, arg1 );
                    if ( arg && arg[0] != '\0' ) {
                        number = atoi( arg );
                        remove_affect_from_obj( obj, number );
                        oedit_disp_prompt_apply_menu( d );
                    }
                    else {
                        send_to_char( "Remove which affect? ", d->character );
                        OLC_MODE( d ) = OEDIT_AFFECT_REMOVE;
                    }
                    return;

                case 'a':
                case 'A':
                    CREATE( paf, AFFECT_DATA, 1 );

                    d->character->pcdata->spare_ptr = paf;
                    oedit_disp_affect_menu( d );
                    return;

                case 'q':
                case 'Q':
                    d->character->pcdata->spare_ptr = NULL;
                    break;
            }
            break;                                     /* If we reach here, we're done */

        case OEDIT_AFFECT_LOCATION:
            if ( is_number( arg ) ) {
                number = atoi( arg );
                if ( number == 0 ) {
                    /*
                     * Junk the affect 
                     */
                    DISPOSE( paf );
                    d->character->spare_ptr = NULL;
                    oedit_disp_prompt_apply_menu( d );
                    return;
                }
            }
            else
                number = get_atype( arg );

            if ( number < 0 || number >= MAX_APPLY_TYPE || number == APPLY_EXT_AFFECT ) {
                send_to_char( "Invalid location, try again: ", d->character );
                return;
            }

            paf->location = number;
            OLC_MODE( d ) = OEDIT_AFFECT_MODIFIER;
            /*
             * Insert all special affect handling here ie: non numerical stuff 
             */
            /*
             * And add the apropriate case statement below 
             */
            if ( number == APPLY_AFFECT ) {
                d->character->tempnum = 0;
                medit_disp_aff_flags( d );
            }
            else if ( number == APPLY_RESISTANT || number == APPLY_IMMUNE
                      || number == APPLY_SUSCEPTIBLE ) {
                d->character->tempnum = 0;
                medit_disp_ris( d );
            }
            else if ( number == APPLY_WEAPONSPELL || number == APPLY_WEARSPELL
                      || number == APPLY_REMOVESPELL )
                oedit_disp_spells_menu( d );
            else
                send_to_char( "\r\nModifier: ", d->character );
            return;

        case OEDIT_AFFECT_MODIFIER:
            switch ( paf->location ) {
                case APPLY_AFFECT:
                case APPLY_RESISTANT:
                case APPLY_IMMUNE:
                case APPLY_SUSCEPTIBLE:
                    if ( is_number( arg ) ) {
                        number = atoi( arg );
                        if ( number == 0 ) {
                            value = d->character->tempnum;
                            break;
                        }
                        TOGGLE_BIT( d->character->tempnum, 1 << ( number - 1 ) );
                    }
                    else {
                        while ( arg[0] != '\0' ) {
                            arg = one_argument( arg, arg1 );
                            if ( paf->location == APPLY_AFFECT )
                                number = get_aflag( arg1 );
                            else
                                number = get_risflag( arg1 );
                            if ( number < 0 )
                                ch_printf( d->character, "Invalid flag: %s\r\n", arg1 );
                            else
                                TOGGLE_BIT( d->character->tempnum, 1 << number );
                        }
                    }
                    if ( paf->location == APPLY_AFFECT )
                        medit_disp_aff_flags( d );
                    else
                        medit_disp_ris( d );
                    return;

                case APPLY_WEAPONSPELL:
                case APPLY_WEARSPELL:
                case APPLY_REMOVESPELL:
                    if ( is_number( arg ) ) {
                        number = atoi( arg );
                        if ( IS_VALID_SN( number ) )
                            value = number;
                        else {
                            send_to_char( "Invalid sn, try again: ", d->character );
                            return;
                        }
                    }
                    else {
                        value = bsearch_skill_exact( arg, gsn_first_spell, gsn_first_skill - 1 );
                        if ( value < 0 ) {
                            ch_printf( d->character, "Invalid spell %s, try again: ", arg );
                            return;
                        }
                    }
                    break;
                default:
                    value = atoi( arg );
                    break;
            }
            /*
             * Link it in 
             */
            if ( !value || OLC_VAL( d ) == TRUE ) {
                paf->modifier = value;
                olc_log( d, "Modified affect to: %s by %d", a_types[paf->location], value );
                OLC_VAL( d ) = FALSE;
                oedit_disp_prompt_apply_menu( d );
                return;
            }
            CREATE( npaf, AFFECT_DATA, 1 );
            npaf->type = -1;
            npaf->duration = -1;
            npaf->location = URANGE( 0, paf->location, MAX_APPLY_TYPE );
            npaf->modifier = value;
            xCLEAR_BITS( npaf->bitvector );
            npaf->next = NULL;

            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                LINK( npaf, obj->pIndexData->first_affect, obj->pIndexData->last_affect, next,
                      prev );
            else
                LINK( npaf, obj->first_affect, obj->last_affect, next, prev );
            ++top_affect;
            olc_log( d, "Added new affect: %s by %d", a_types[npaf->location], npaf->modifier );

            DISPOSE( paf );
            d->character->spare_ptr = NULL;
            oedit_disp_prompt_apply_menu( d );
            return;

        case OEDIT_AFFECT_RIS:
            /*
             * Unnecessary atm 
             */
            number = atoi( arg );
            if ( number < 0 || number > 32 ) {
                send_to_char( "Unknown flag, try again: ", d->character );
                return;
            }
            return;

        case OEDIT_AFFECT_REMOVE:
            number = atoi( arg );
            remove_affect_from_obj( obj, number );
            olc_log( d, "Removed affect #%d", number );
            oedit_disp_prompt_apply_menu( d );
            return;

        case OEDIT_EXTRADESC_KEY:
            /*
             * if ( SetOExtra( obj, arg ) || SetOExtraProto( obj->pIndexData, arg ) ) {
             * send_to_char( "A extradesc with that keyword already exists.\r\n",
             * d->character ); oedit_disp_extradesc_menu(d); return; } 
             */
            olc_log( d, "Changed exdesc %s to %s", ed->keyword, arg );
            STRFREE( ed->keyword );
            ed->keyword = STRALLOC( arg );
            oedit_disp_extra_choice( d );
            return;

        case OEDIT_EXTRADESC_DESCRIPTION:
            /*
             * Should never reach this 
             */
            break;

        case OEDIT_EXTRADESC_CHOICE:
            number = atoi( arg );
            switch ( number ) {
                case 0:
                    OLC_MODE( d ) = OEDIT_EXTRADESC_MENU;
                    oedit_disp_extradesc_menu( d );
                    return;
                case 1:
                    OLC_MODE( d ) = OEDIT_EXTRADESC_KEY;
                    send_to_char( "Enter keywords, speperated by spaces: ", d->character );
                    return;
                case 2:
                    OLC_MODE( d ) = OEDIT_EXTRADESC_DESCRIPTION;
                    d->character->substate = SUB_OBJ_EXTRA;
                    d->character->last_cmd = do_oedit_reset;

                    send_to_char( "Enter new extra description - :\r\n", d->character );
                    if ( !ed->description )
                        ed->description = STRALLOC( "" );
                    start_editing( d->character, ed->description );
                    return;
            }
            break;

        case OEDIT_EXTRADESC_DELETE:
            ed = oedit_find_extradesc( obj, atoi( arg ) );
            if ( !ed ) {
                send_to_char( "Extra description not found, try again: or type q", d->character );
                return;
            }
            olc_log( d, "Deleted exdesc %s", ed->keyword );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                UNLINK( ed, obj->pIndexData->first_extradesc, obj->pIndexData->last_extradesc, next,
                        prev );
            else
                UNLINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
            STRFREE( ed->keyword );
            STRFREE( ed->description );
            DISPOSE( ed );
            top_ed--;
            oedit_disp_extradesc_menu( d );
            return;

        case OEDIT_EXTRADESC_MENU:
            switch ( UPPER( arg[0] ) ) {
                case 'Q':
                case 'q':
                    break;

                case 'A':
                    CREATE( ed, EXTRA_DESCR_DATA, 1 );

                    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) )
                        LINK( ed, obj->pIndexData->first_extradesc, obj->pIndexData->last_extradesc,
                              next, prev );
                    else
                        LINK( ed, obj->first_extradesc, obj->last_extradesc, next, prev );
                    ed->keyword = STRALLOC( "" );
                    ed->description = STRALLOC( "" );
                    top_ed++;
                    d->character->pcdata->spare_ptr = ed;
                    olc_log( d, "Added new exdesc" );

                    oedit_disp_extra_choice( d );
                    return;

                case 'R':
                    OLC_MODE( d ) = OEDIT_EXTRADESC_DELETE;
                    send_to_char( "Delete which extra description? ", d->character );
                    return;

                default:
                    if ( is_number( arg ) ) {
                        ed = oedit_find_extradesc( obj, atoi( arg ) );
                        if ( !ed ) {
                            send_to_char( "Not found, try again: ", d->character );
                            return;
                        }
                        d->character->spare_ptr = ed;
                        oedit_disp_extra_choice( d );
                    }
                    else
                        oedit_disp_extradesc_menu( d );
                    return;
            }
            break;
/*
        case OEDIT_MPROGS:
            switch ( UPPER( arg[0] ) )
            {
                case 'A':
                    if ( obj->pIndexData->mudprogs != NULL )
                        for ( ; mprog->next; mprog = mprog->next );
                    CREATE( mprg, MPROG_DATA, 1 );
                    if ( obj->pIndexData->mudprogs )
                        mprog->next = mprg;
                    else
                        obj->pIndexData->mudprogs = mprg;
                    mprg->next = NULL;

                    d->character->pcdata->spare_ptr = mprg;
                    oedit_disp_prog_choice( d );
                    return;
                case 'R':
                    send_to_char( "Delete which prog? ", d->character );
                    OLC_MODE( d ) = OEDIT_MPROGS_DELETE;
                    return;
                case 'Q':
                case 'q':
                    break;
                default:
                    if ( is_number( arg ) )
                    {
                        mprg = oedit_find_prog( obj, atoi( arg ) );
                        if ( !mprg )
                        {
                            send_to_char( "Not found, try again: ", d->character );
                            return;
                        }
                        d->character->spare_ptr = mprg;
                        oedit_disp_prog_choice( d );
                    }
                    else
                        oedit_disp_progs( d );
                    return;
            }
            break;

        case OEDIT_MPROGS_CHOICE:
            switch ( UPPER( arg[0] ) )
            {
                case 'A':
                    OLC_MODE( d ) = OEDIT_MPROGS_TYPE;
                    oedit_disp_prog_types( d );
                    return;
                case 'B':
                    OLC_MODE( d ) = OEDIT_MPROGS_ARG;
                    send_to_char( "What are the arguments to the program? ", d->character );
                    return;
                case 'C':
                    if ( mprg->type < 1 )
                    {
                        send_to_char( "The program must have a type before you can edit it.", d->character );
                        return;
                    }
                    d->character->substate = SUB_MPROG_EDIT;
                    d->character->last_cmd = do_oedit_reset;
                    d->character->tempnum = CON_OEDIT;
                    if ( !mprg->comlist )
                        mprg->comlist = STRALLOC( "" );
                    start_editing( d->character, mprg->comlist );
                    return;

                case 'Q':
                case 'q':
                    oedit_disp_progs( d );
                    return;
            }
            break;

        case OEDIT_MPROGS_TYPE:
            d->character->tempnum = mprg->type;
            if ( !is_number( arg ) )
            {
                number = get_mpflag( arg );
                if ( number < 0 )
                {
                    send_to_char( "Invalid program type, try again: ", d->character );
                    return;
                }
            }
            else
            {
                number = atoi( arg );
                if ( number < 0 || number > USE_PROG )
                {
                    send_to_char( "Invalid program type, try again: ", d->character );
                    return;
                }
            }
            mprg->type = number;

            olc_log( d, "Changed prog %s to %s",
                     d->character->tempnum > 0 ? mprog_type_to_name( d->character->tempnum ) : "(none)", mprog_type_to_name( number ) );

            oedit_disp_prog_choice( d );
            return;

        case OEDIT_MPROGS_DELETE:
            mprg = oedit_find_prog( obj, atoi( arg ) );
            if ( !mprg )
            {
                send_to_char( "Not found, try again: ", d->character );
                return;
            }

            STRFREE( mprg->arglist );
            STRFREE( mprg->comlist );
            DISPOSE( mprg );
            mprg = mprg->next;

            oedit_disp_progs( d );
            return;
*/
        default:
            bug( "Oedit_parse: Reached default case!" );
            break;
    }
    /*
     * . If we get here, we have changed something .
     */
    OLC_CHANGE( d ) = TRUE;                            /* . Has changed flag . */
    oedit_disp_menu( d );
}
