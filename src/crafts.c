/***************************************************************************
                      _____  __      __  __      __
                     /  _  \/  \    /  \/  \    /  \
                    /  /_\  \   \/\/   /\   \/\/   /
                   /    |    \        /  \        /
                   \____|__  /\__/\  /    \__/\  /
                           \/      \/          \/

    As the Wheel Weaves based on ROM 2.4. Original code by Dalsor.
    See changes.log for a list of changes from the original ROM code.
    Credits for code created by other authors have been left
 	intact at the head of each function.

    Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,
    Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.

    Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael
    Chastain, Michael Quan, and Mitchell Tse.

    In order to use any part of this Merc Diku Mud, you must comply with
    both the original Diku license in 'license.doc' as well the Merc
    license in 'license.txt'.  In particular, you may not remove either of
    these copyright notices.

    Much time and thought has gone into this software and you are
    benefitting.  We hope that you share your changes too.  What goes
    around, comes around.

	ROM 2.4 is copyright 1993-1998 Russ Taylor
	ROM has been brought to you by the ROM consortium
	    Russ Taylor (rtaylor@hypercube.org)
	    Gabrielle Taylor (gtaylor@hypercube.org)
	    Brian Moore (zump@rom.org)
	By using this code, you have agreed to follow the terms of the
	ROM license, in the file Rom24/doc/rom.license
 ***************************************************************************/

#include <string.h>
#include "h/mud.h"
#include "h/crafts.h"
#include "h/files.h"
#include "h/key.h"
#include "h/clans.h"

#define SMITH_PRODUCT    41002

#define MAX_SMITH_ITEMS  96
#define MAX_FOOD 24
#define MAX_DRINK 24
#define MAX_FRUIT 24
#define MAX_TANNED 24
#define MAX_HUNTED 24
short                   set_min_armor( int level );
short                   set_max_armor( int level );

#define MAX_ORE_ITEMS   8
extern bool             cexp;
int clan_lookup         args( ( const char *name ) );
REACTIVE_FAIL_DATA     *first_qq;
REACTIVE_FAIL_DATA     *last_qq;

int                     usetier;

int drink_lookup( const char *name )
{
    int                     item;

    for ( item = 0; item < MAX_DRINK; item++ ) {
        if ( !str_cmp( name, drink_one[item].name ) ) {
            usetier = 1;
            return item;
        }
    }
    usetier = -1;
    return -1;
}

int tier_lookup( const char *name )
{
    int                     item;

    for ( item = 0; item < MAX_SMITH_ITEMS; item++ ) {
        if ( !str_cmp( name, tier_one[item].name ) ) {
            usetier = 1;
            return item;
        }
    }
    usetier = -1;
    return -1;
}

short set_tier_min( int level, int tier )
{
    if ( tier == 1 ) {
        if ( level <= 4 )
            return 3;
        if ( level <= 9 )
            return 5;
        if ( level <= 19 )
            return 7;
        if ( level <= 25 )
            return 12;
        if ( level <= 34 )
            return 17;
        if ( level <= 44 )
            return 22;
        if ( level <= 54 )
            return 27;
        if ( level <= 64 )
            return 32;
        if ( level <= 74 )
            return 37;
        if ( level <= 84 )
            return 42;
        if ( level <= 99 )
            return 47;
        return 52;
    }
    if ( tier == 2 ) {
        if ( level <= 4 )
            return 4;
        if ( level <= 9 )
            return 6;
        if ( level <= 19 )
            return 8;
        if ( level <= 25 )
            return 13;
        if ( level <= 34 )
            return 18;
        if ( level <= 44 )
            return 23;
        if ( level <= 54 )
            return 28;
        if ( level <= 64 )
            return 33;
        if ( level <= 74 )
            return 38;
        if ( level <= 84 )
            return 43;
        if ( level <= 99 )
            return 48;
        return 53;
    }
    /*
     * If you get here its tier 3 
     */
    if ( level <= 4 )
        return 6;
    if ( level <= 9 )
        return 8;
    if ( level <= 19 )
        return 10;
    if ( level <= 25 )
        return 15;
    if ( level <= 34 )
        return 20;
    if ( level <= 44 )
        return 25;
    if ( level <= 54 )
        return 30;
    if ( level <= 64 )
        return 35;
    if ( level <= 74 )
        return 40;
    if ( level <= 84 )
        return 45;
    if ( level <= 99 )
        return 50;
    return 55;
}

short set_tier_max( int level, int tier )
{
    if ( tier == 1 ) {
        if ( level <= 4 )
            return 7;
        if ( level <= 9 )
            return 12;
        if ( level <= 19 )
            return 17;
        if ( level <= 24 )
            return 22;
        if ( level <= 34 )
            return 32;
        if ( level <= 39 )
            return 37;
        if ( level <= 44 )
            return 42;
        if ( level <= 49 )
            return 47;
        if ( level <= 54 )
            return 52;
        if ( level <= 60 )
            return 62;
        if ( level <= 70 )
            return 72;
        if ( level <= 80 )
            return 82;
        if ( level <= 90 )
            return 92;
        if ( level <= 99 )
            return 102;
        return 100;
    }
    if ( tier == 2 ) {
        if ( level <= 4 )
            return 8;
        if ( level <= 9 )
            return 13;
        if ( level <= 19 )
            return 18;
        if ( level <= 24 )
            return 23;
        if ( level <= 34 )
            return 33;
        if ( level <= 39 )
            return 38;
        if ( level <= 44 )
            return 43;
        if ( level <= 49 )
            return 48;
        if ( level <= 54 )
            return 53;
        if ( level <= 60 )
            return 63;
        if ( level <= 70 )
            return 73;
        if ( level <= 80 )
            return 83;
        if ( level <= 90 )
            return 93;
        if ( level <= 99 )
            return 103;
        return 103;
    }
    /*
     * If here its tier 3 
     */
    if ( level <= 4 )
        return 10;
    if ( level <= 9 )
        return 15;
    if ( level <= 19 )
        return 20;
    if ( level <= 24 )
        return 25;
    if ( level <= 34 )
        return 35;
    if ( level <= 39 )
        return 40;
    if ( level <= 44 )
        return 45;
    if ( level <= 49 )
        return 50;
    if ( level <= 54 )
        return 55;
    if ( level <= 60 )
        return 65;
    if ( level <= 70 )
        return 75;
    if ( level <= 80 )
        return 85;
    if ( level <= 90 )
        return 95;
    if ( level <= 99 )
        return 105;
    return 105;
}

void learn_from_craft( CHAR_DATA *ch, int sn )
{
    int                     adept,
                            gain,
                            learn,
                            percent,
                            chance;
    char                    buf[MSL];

    if ( IS_NPC( ch ) || ch->pcdata->learned[sn] <= 0 )
        return;

    adept = 95;

    if ( adept < 1 )
        return;

    if ( ch->pcdata->tradelevel >= 20 )
        return;

    if ( sn == gsn_forge || sn == gsn_tan || sn == gsn_bake || sn == gsn_mix ) {
        if ( ch->pcdata->tradelevel <= 19 )
            gain = number_chance( 100, 200 );
        if ( ch->pcdata->tradelevel <= 15 )
            gain = number_chance( 75, 200 );
        if ( ch->pcdata->tradelevel <= 10 )
            gain = number_chance( 50, 80 );
        if ( ch->pcdata->tradelevel <= 5 )
            gain = number_chance( 25, 50 );
        if ( ch->pcdata->tradelevel <= 2 )
            gain = number_chance( 3, 10 );
    }

    if ( sn == gsn_mine || sn == gsn_hunt || sn == gsn_gather ) {
        if ( ch->pcdata->tradelevel <= 19 )
            gain = number_chance( 15, 25 );
        if ( ch->pcdata->tradelevel <= 15 )
            gain = number_chance( 5, 15 );
        if ( ch->pcdata->tradelevel <= 10 )
            gain = number_chance( 5, 10 );
        if ( ch->pcdata->tradelevel <= 5 )
            gain = number_chance( 1, 5 );
        if ( ch->pcdata->tradelevel <= 2 )
            gain = 1;
    }

    if ( cexp ) {
        gain *= 2.0;
    }

    ch->pcdata->craftpoints += gain;
    ch_printf( ch, "&WYou receive %d crafting point%s.\r\n", gain, gain == 1 ? "" : "s" );

    if ( ch->pcdata->craftpoints >= craft_level( ch, ch->pcdata->tradelevel + 1 ) ) {
        set_char_color( AT_WHITE, ch );

        ch_printf( ch, "You have now obtained crafting level %d!&D\r\n", ++ch->pcdata->tradelevel );
        ch->pcdata->craftpoints =
            ( ch->pcdata->craftpoints - craft_level( ch, ( ch->pcdata->tradelevel ) ) );
    }

    if ( ch->pcdata->tradelevel == 20 ) {
        snprintf( buf, MSL,
                  "The realms rejoice as %s has just achieved crafting level %d and is a Master of the Craft!&D",
                  ch->name, ch->pcdata->tradelevel );
        announce( buf );
    }
}

void do_mine( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    arg1[MIL],
                            name[MSL],
                            shortdesc[MSL],
                            longdesc[MSL];
    int                     color,
                            value,
                            cost,
                            knows,
                            level;
    short                   chance;

    argument = one_argument( argument, arg1 );

    if ( IS_NPC( ch ) )
        return;

    if ( !VLD_STR( arg1 ) || !str_cmp( arg1, "list" ) ) {
        send_to_char( "&CThe Listing of Ore for Mining&c\r\n", ch );

        send_to_char( "&cSyntax: Mine <&Core type&c>\r\n", ch );

        send_to_char( "Ores: bronze", ch );
        if ( ch->pcdata->tradelevel >= 3 )
            send_to_char( " silver", ch );
        else
            send_to_char( " silver(3)", ch );
        if ( ch->pcdata->tradelevel >= 5 )
            send_to_char( " gold", ch );
        else
            send_to_char( " gold(5)", ch );
        if ( ch->pcdata->tradelevel >= 8 )
            send_to_char( " iron", ch );
        else
            send_to_char( " iron(8)", ch );
        if ( ch->pcdata->tradelevel >= 10 )
            send_to_char( " steel", ch );
        else
            send_to_char( " steel(10)", ch );
        if ( ch->pcdata->tradelevel >= 15 )
            send_to_char( " mithril", ch );
        else
            send_to_char( " mithril(15)", ch );
        if ( ch->pcdata->tradelevel >= 20 )
            send_to_char( " adamantite", ch );
        else
            send_to_char( " adamantite(20)", ch );

        send_to_char( "\r\n", ch );
        return;
    }

    if ( ch->in_room->sector_type != SECT_ORE ) {
        send_to_char( "You cannot mine here.\r\n", ch );
        return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "You are not holding a pick tool.\r\n", ch );
        return;
    }

    if ( obj->item_type != ITEM_TOOL ) {
        send_to_char( "You must be holding a pick tool.\r\n", ch );
        return;
    }

    if ( ch->move < 5 ) {
        send_to_char( "You don't have enough energy to go keep mining.\r\n", ch );
        return;
    }

    chance = number_chance( 1, 10 );

    if ( !str_cmp( arg1, "gold" ) || !str_cmp( arg1, "silver" ) || !str_cmp( arg1, "bronze" ) || !str_cmp( arg1, "iron" ) || !str_cmp( arg1, "steel" ) );   /* Does 
                                                                                                                                                             * nothing 
                                                                                                                                                             * and 
                                                                                                                                                             * keeps 
                                                                                                                                                             * going 
                                                                                                                                                             */
    else if ( !str_cmp( arg1, "mithril" ) )
        chance = number_chance( 1, 5 );                /* More chance of failing on this */
    else if ( !str_cmp( arg1, "adamantite" ) )
        chance = number_chance( 1, 3 );                /* More chance of failing on this */
    else {                                             /* If none of those then give a
                                                        * message */

        send_to_char( "Syntax: Mine <material>\r\n", ch );
        send_to_char( "Material being: bronze, silver, gold, iron, steel, mithril, adamantite\r\n",
                      ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_mine]->beats );

    if ( !can_use_skill( ch, number_percent(  ), gsn_mine ) ) {
        learn_from_failure( ch, gsn_mine );
        send_to_char( "You cannot find anything of worth from your mining.\r\n", ch );
        return;
    }

    if ( ( ch->carry_number + 1 ) > can_carry_n( ch ) ) {
        send_to_char( "You can't mine anymore because you can't carry more ores.\r\n", ch );
        return;
    }

    ch->move -= 1;

    if ( chance < 3 ) {
        if ( chance <= 1 ) {
            act( AT_ORANGE, "$n's mining only digs up a pile of dirt.\r\n", ch, NULL, NULL,
                 TO_ROOM );
            send_to_char( "&OYour mining only digs up a pile of dirt.\r\n", ch );
        }
        else if ( chance >= 2 ) {
            act( AT_CYAN,
                 "$n begins to labor in earnest chipping away at the rock with $s pick axe.\r\n",
                 ch, NULL, NULL, TO_ROOM );
            send_to_char( "&cYou begin to labor in earnest chipping away at the rock.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg1, "bronze" ) ) {
        snprintf( name, sizeof( name ), "%s", "bronze ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "a bronze ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "A bronze ore has been left here." );
        color = 1;
        value = CURR_COPPER;
        cost = 1;
        knows = 15;
        level = 0;
    }
    else if ( !str_cmp( arg1, "silver" ) ) {
        snprintf( name, sizeof( name ), "%s", "silver ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "a silver ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "A silver ore has been left here." );
        color = 5;
        value = CURR_COPPER;
        cost = 1;
        knows = 40;
        level = 3;
    }
    else if ( !str_cmp( arg1, "iron" ) ) {
        snprintf( name, sizeof( name ), "%s", "iron ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "a iron ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "A iron ore has been left here." );
        color = 13;
        value = CURR_COPPER;
        cost = 1;
        knows = 70;
        level = 8;
    }
    else if ( !str_cmp( arg1, "steel" ) ) {
        snprintf( name, sizeof( name ), "%s", "steel ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "a steel ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "A steel ore has been left here." );
        color = 13;
        value = CURR_COPPER;
        cost = 1;
        knows = 90;
        level = 10;
    }
    else if ( !str_cmp( arg1, "mithril" ) ) {
        snprintf( name, sizeof( name ), "%s", "mithril ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "a mithril ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "A mithril ore has been left here." );
        color = 9;
        value = CURR_COPPER;
        cost = 1;
        knows = 92;
        level = 15;
    }
    else if ( !str_cmp( arg1, "adamantite" ) ) {
        snprintf( name, sizeof( name ), "%s", "adamantite ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "an adamantite ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "An adamantite ore has been left here." );
        color = 5;
        value = CURR_COPPER;
        cost = 1;
        knows = 93;
        level = 20;
    }
    else if ( !str_cmp( arg1, "gold" ) ) {
        snprintf( name, sizeof( name ), "%s", "gold ore" );
        snprintf( shortdesc, sizeof( shortdesc ), "%s", "a gold ore" );
        snprintf( longdesc, sizeof( longdesc ), "%s", "A gold ore has been left here." );
        color = 14;
        value = CURR_COPPER;
        cost = 1;
        knows = 80;
        level = 5;
    }

    obj = create_object( get_obj_index( OBJ_VNUM_ORE ), 0 );
    if ( obj->name )
        STRFREE( obj->name );
    if ( obj->short_descr )
        STRFREE( obj->short_descr );
    if ( obj->description )
        STRFREE( obj->description );
    obj->name = STRALLOC( name );
    obj->short_descr = STRALLOC( shortdesc );
    obj->description = STRALLOC( longdesc );
    obj->color = color;
    GET_VALUE( obj, type ) = value;
    obj->cost = cost;

    if ( ( ch->carry_weight + obj->weight ) > can_carry_w( ch ) ) {
        send_to_char( "It's too heavy for you to lift with everything else you're holding.\r\n",
                      ch );
        learn_from_failure( ch, gsn_mine );
        extract_obj( obj );
        return;
    }

    if ( ch->pcdata->learned[gsn_mine] < knows || ch->pcdata->tradelevel < level ) {
        send_to_char( "You begin to mine for ore.\r\n", ch );
        pager_printf( ch, "&GYour mining pays off as you successfully unearthed %s!\r\n",
                      shortdesc );
        act( AT_GREEN, "$n's mining suddenly unearths $p.", ch, obj, NULL, TO_ROOM );
        pager_printf( ch,
                      "&GYour lack of mining skills has ruined the find with a split in the %s!\r\n",
                      name );
        act( AT_GREEN, "$n's looks away in disgust as $e realizes the ore is ruined.", ch, NULL,
             NULL, TO_ROOM );
        learn_from_failure( ch, gsn_mine );
        extract_obj( obj );
        return;
    }

    obj_to_char( obj, ch );
    send_to_char( "You begin to mine for ore.\r\n", ch );
    pager_printf( ch, "&GYour mining pays off as you successfully unearthed %s!\r\n",
                  obj->short_descr );
    act( AT_GREEN, "$n's mining suddenly unearths $p.", ch, obj, NULL, TO_ROOM );
    learn_from_craft( ch, gsn_mine );
}

void send_forge_syntax( CHAR_DATA *ch )
{
    if ( !ch )
        return;

    send_to_char( "&cSyntax: forge <&Cmaterial&c> into <&Citem&c> <&Clevel&c>\r\n", ch );
    send_to_char( "Syntax: forge list\r\n", ch );
    send_to_char( "Syntax: forge fire\r\n", ch );
    send_to_char( "Materials being: bronze, iron, steel, silver, gold, mithril, adamantite\r\n",
                  ch );
    send_to_char
        ( "Note: There are &C4 steps&c, keep doing the forge command until the last step.\r\n",
          ch );
}

int get_armor_ac_mod( int level, int tlevel )
{
    if ( tlevel >= 20 ) {
        if ( level > 19 )
            return 3;
        if ( level > 4 )
            return 4;
        return 5;
    }
    if ( tlevel >= 15 ) {
        if ( level > 19 )
            return 1;
        if ( level > 4 )
            return 2;
        return 3;
    }
    if ( tlevel > 5 ) {
        if ( level > 19 )
            return -5;
        if ( level > 4 )
            return -3;
        return -1;
    }
    if ( level > 19 )
        return -10;
    if ( level > 4 )
        return -4;
    return -1;
}

void do_forge( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *ore,
                           *item;
    AFFECT_DATA            *paf;
    struct skill_type      *skill = NULL;
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            arg4[MIL];
    char                    name_buf[MSL],
                            short_buf[MSL],
                            long_buf[MSL],
                            extra_buf[MSL];
    const char             *adj;
    int                     i = 0,
        x = 0,
        difficulty = 0,
        output = 0,
        sn = 1;
    short                   mnum = 0;
    bool                    itm = FALSE,
        hasore = FALSE,
        found = FALSE;

    name_buf[0] = '\0';
    short_buf[0] = '\0';
    long_buf[0] = '\0';
    extra_buf[0] = '\0';

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );

    if ( IS_NPC( ch ) || ch->desc == NULL )
        return;

    if ( !arg1 || arg1[0] == '\0' ) {
        send_forge_syntax( ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
        int                     y,
                                count = 0;
        char                    wearloc[MSL];

        send_to_char( "\t\t\t&CThe Forge Listing\r\n\r\n", ch );
        for ( y = 0; y < MAX_SMITH_ITEMS; y++ ) {
            count++;

            /*
             * We don't want the spaces so done seperate and then spaces handled where sent 
             */
            if ( !IS_BLIND( ch ) ) {
                snprintf( wearloc, sizeof( wearloc ), "&c(&w%s&c)%s(%d)",
                          IS_SET( tier_one[y].wear_flags, ITEM_WEAR_FINGER ) ? "finger"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_NECK ) ? "neck"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_BODY ) ? "body"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_HEAD ) ? "head"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_LEGS ) ? "legs"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_FEET ) ? "feet"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_HANDS ) ? "hands"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_ARMS ) ? "arms"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_SHIELD ) ? "shield"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_ABOUT ) ? "about"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_WAIST ) ? "waist"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_WRIST ) ? "wrist"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WIELD ) ? "wield"
                          : IS_SET( tier_one[y].wear_flags, ITEM_HOLD ) ? "hold"
                          : IS_SET( tier_one[y].wear_flags, ITEM_DUAL_WIELD ) ? "dual"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_EARS ) ? "ears"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_EYES ) ? "eyes"
                          : IS_SET( tier_one[y].wear_flags, ITEM_MISSILE_WIELD ) ? "missile"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_BACK ) ? "back"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_FACE ) ? "face"
                          : IS_SET( tier_one[y].wear_flags, ITEM_WEAR_ANKLE ) ? "ankle"
                          : IS_SET( tier_one[y].wear_flags,
                                    ITEM_WEAR_SHEATH ) ? "sheath" : "unknown",
                          tier_one[y].name ? tier_one[y].name : "null name", tier_one[y].level );
                ch_printf( ch, "&c%-32s ", wearloc );
            }
            else {
                ch_printf( ch, "%2d &c%-28s", tier_one[y].level,
                           tier_one[y].name ? tier_one[y].name : "null name" );
            }

            if ( count == 3 ) {
                send_to_char( "\r\n", ch );
                count = 0;
            }
        }
        if ( count != 0 )
            send_to_char( "\r\n", ch );
        send_forge_syntax( ch );
        return;
    }

    if ( !str_cmp( arg1, "fire" ) ) {
        OBJ_DATA               *stove;
        bool                    found;

        found = FALSE;

        for ( stove = ch->in_room->first_content; stove; stove = stove->next_content ) {
            if ( stove->item_type == ITEM_FORGE ) {
                found = TRUE;
                break;
            }
        }

        if ( !found ) {
            send_to_char( "There must be a forge to fire it.\r\n", ch );
            return;
        }

        if ( stove->value[0] == 1 ) {
            send_to_char( "There is no need to fire the forge, it is already.\r\n", ch );
            return;
        }

        OBJ_DATA               *coal;

        for ( coal = ch->first_carrying; coal; coal = coal->next_content ) {
            if ( coal->item_type == ITEM_COAL )
                break;
        }
        if ( !coal ) {
            send_to_char( "You do not have any coal to fire the forge.\r\n", ch );
            return;
        }
        separate_obj( coal );
        obj_from_char( coal );

        act( AT_CYAN, "$n fires up the forge lighting the coal within it.", ch, NULL, NULL,
             TO_ROOM );
        act( AT_CYAN, "You fire the forge lighting the coal within it.", ch, NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "A flame flickers within the forge.", ch, NULL, NULL, TO_ROOM );
        act( AT_YELLOW, "A flame flickers within the forge.", ch, NULL, NULL, TO_CHAR );
        extract_obj( coal );
        stove->value[0] = 1;
        return;
    }

    if ( arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_forge_syntax( ch );
        return;
    }

    if ( str_cmp( arg2, "into" ) ) {
        send_forge_syntax( ch );
        return;
    }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_TRADESKILLS ) ) {
        send_to_char( "You must be in a tradeskills building to do this.\r\n", ch );
        return;
    }

    {
        OBJ_DATA               *stove;
        bool                    found;

        found = FALSE;

        for ( stove = ch->in_room->first_content; stove; stove = stove->next_content ) {
            if ( stove->item_type == ITEM_FORGE ) {
                found = TRUE;
                break;
            }
        }

        if ( !found ) {
            send_to_char( "There must be a forge in the room in order to do that.\r\n", ch );
            return;
        }

        if ( !( ore = get_obj_carry( ch, arg1 ) ) ) {
            send_to_char( "You aren't carrying that.\r\n\r\n", ch );
            send_forge_syntax( ch );
            return;
        }

/*  Volk - quick patch, if we've got the ore, let's check now if there's an upgrade
    Problem with this is it will no longer use multiple ores to construct weapons etc.
    OBJ_DATA *obj;
    for(obj = ch->last_carrying; obj; obj = obj->prev_content)
    {
      if (!str_cmp(obj->name, ore->name) && obj->value[0] > ore->value[0])
        ore = obj;
    }
*/
        {
            separate_obj( ore );

            if ( ore->item_type != ITEM_RAW ) {
                send_to_char( "This is not a proper ore.\r\n", ch );
                return;
            }

            for ( x = 0; x < MAX_ORE_ITEMS - 1; x++ ) {
                if ( !str_cmp( ore->name, smith_ores_table[x].name ) ) {
                    hasore = TRUE;
                    break;
                }
            }

            if ( ore->value[0] == 4 )
                hasore = TRUE;

            if ( !hasore ) {
                send_to_char( "This is not a proper ore.\r\n", ch );
                return;
            }

            // This is the nitty gritty of checking tier output
            i = tier_lookup( arg3 );
            if ( i < 0 || ch->pcdata->tradelevel < tier_one[i].level ) {
                send_to_char( "That isn't a something you can create.\r\n", ch );
                return;
            }

            if ( str_cmp( arg1, "bronze" ) && str_cmp( arg1, "silver" ) && str_cmp( arg1, "gold" )
                 && str_cmp( arg1, "iron" ) && str_cmp( arg1, "steel" )
                 && str_cmp( arg1, "mithril" ) && str_cmp( arg1, "adamantite" ) ) {
                send_to_char
                    ( "You can only use bronze, silver, gold, iron, steel, mithril or adamantite.\r\n",
                      ch );
                return;
            }

            SKILLTYPE              *skill = get_skilltype( gsn_forge );

            WAIT_STATE( ch, 15 );

            if ( ore->value[0] == 0 ) {
                if ( stove->value[0] != 1 ) {
                    send_to_char( "You have to fire the forge first.\r\n", ch );
                    send_to_char( "Syntax: forge fire\r\n", ch );
                    return;
                }

                if ( ch->pcdata->learned[gsn_forge] <= number_range( 1, 5 )
                     || number_percent(  ) <= 5 ) {
                    act( AT_CYAN,
                         "$n prepares the ore by placing it in the furnace, but the material is left too long and melts.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You prepare the ore by placing it in the furnace, but the material is left too long and melts.",
                         ch, NULL, NULL, TO_CHAR );
                    obj_from_char( ore );
                    extract_obj( ore );
                    learn_from_failure( ch, gsn_forge );
                    return;
                }

                act( AT_CYAN,
                     "$n prepares the ore by placing it in the forge and letting it get white-hot before pulling it out.",
                     ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN,
                     "You prepare the ore by placing it in the forge and letting it get white-hot before pulling it out.",
                     ch, NULL, NULL, TO_CHAR );
                WAIT_STATE( ch, 15 );

                ore->value[0] = 1;
                if ( ore->short_descr )
                    STRFREE( ore->short_descr );
                ore->short_descr = STRALLOC( "&Wwhite-hot ore" );
                learn_from_success( ch, gsn_forge );
                return;
            }

            if ( ore->value[0] == 1 ) {
                if ( ch->pcdata->learned[gsn_forge] <= number_range( 1, 15 )
                     || number_percent(  ) <= 10 ) {
                    act( AT_CYAN,
                         "$n places the heated ore on the anvil and begins to shape it with the hammer, but gets distracted and distorts the material beyond use.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You place the heated ore on the anvil and begin to shape it with your hammer, but get distracted and distort the material beyond use.",
                         ch, NULL, NULL, TO_CHAR );
                    obj_from_char( ore );
                    extract_obj( ore );
                    learn_from_failure( ch, gsn_forge );
                    return;
                }

                act( AT_CYAN,
                     "$n places the heated ore on the anvil and begins to shape it with the hammer.",
                     ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN,
                     "You place the heated ore on the anvil and begin to shape it with your hammer.",
                     ch, NULL, NULL, TO_CHAR );
                WAIT_STATE( ch, 15 );

                ore->value[0] = 2;
                if ( ore->short_descr )
                    STRFREE( ore->short_descr );
                ore->short_descr = STRALLOC( "&Yformed hot ore" );
                learn_from_success( ch, gsn_forge );
                return;
            }

            /*
             * They should only get these and spend alot of time on the hammery part the rest
             * should be fairly straight foward 
             */
            if ( ore->value[0] == 2 ) {
                if ( !can_use_skill( ch, number_percent(  ), gsn_forge )
                     || number_percent(  ) <= 25 ) {
                    act( AT_CYAN, "$n hammers away at the material.", ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN, "You hammer away at the material.", ch, NULL, NULL, TO_CHAR );
                    learn_from_failure( ch, gsn_forge );
                    return;
                }

                if ( number_percent(  ) <= 20 ) {
                    act( AT_CYAN,
                         "As $n starts to hammer the material, $s realizes it has cooled off and needs to be reheated.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "As you start to hammer the material, you realize it has cooled off and needs to be reheated.",
                         ch, NULL, NULL, TO_CHAR );
                    learn_from_failure( ch, gsn_forge );
                    ore->value[0] = 0;
                    if ( ore->short_descr )
                        STRFREE( ore->short_descr );
                    ore->short_descr = STRALLOC( "&cformed cooled ore" );
                    return;
                }

                act( AT_CYAN, "$n places the cooling material in a barrel of water to cool it off.",
                     ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN, "You place the cooling material in a barrel of water to cool it off.",
                     ch, NULL, NULL, TO_CHAR );
                WAIT_STATE( ch, 15 );

                ore->value[0] = 3;
                if ( ore->short_descr )
                    STRFREE( ore->short_descr );
                ore->short_descr = STRALLOC( "&wthe roughly finished product" );
                learn_from_success( ch, gsn_forge );
                return;
            }

            if ( !can_use_skill( ch, number_percent(  ), gsn_forge ) || number_percent(  ) <= 10 ) {
                act( AT_CYAN,
                     "You spin the grinding wheel to smooth the rough edges, but get distracted and distort the material beyond use.",
                     ch, NULL, NULL, TO_CHAR );
                act( AT_CYAN,
                     "$n spins the grinding wheel to smooth the rough edges, but gets distracted and distorts the material beyond use.",
                     ch, NULL, NULL, TO_ROOM );
                obj_from_char( ore );
                extract_obj( ore );
                learn_from_failure( ch, gsn_forge );
                return;
            }

            act( AT_CYAN, "$n spins the grinding wheel to smooth the rough edges of $s work.", ch,
                 NULL, NULL, TO_ROOM );
            act( AT_CYAN, "You spin the grinding wheel to smooth the rough edges of your work.", ch,
                 NULL, NULL, TO_CHAR );
            WAIT_STATE( ch, 15 );

            obj_from_char( ore );
            extract_obj( ore );

            learn_from_craft( ch, gsn_forge );

            item = create_object( get_obj_index( SMITH_PRODUCT ), 1 );
            if ( !str_cmp( arg1, "bronze" ) )
                item->color = 1;
            else if ( !str_cmp( arg1, "silver" ) )
                item->color = 5;
            else if ( !str_cmp( arg1, "gold" ) )
                item->color = 14;
            else if ( !str_cmp( arg1, "iron" ) )
                item->color = 13;
            else if ( !str_cmp( arg1, "steel" ) )
                item->color = 13;
            else if ( !str_cmp( arg1, "mithril" ) )
                item->color = 9;
            else if ( !str_cmp( arg1, "adamantite" ) )
                item->color = 5;

            if ( arg4[0] == '\0' )
                item->level = ch->level;
            else
                item->level = atoi( arg4 );

            {
                int                     tier,
                                        item_type,
                                        wearflags,
                                        weight,
                                        cost;
                const char             *name;

                if ( tier_one[i].level >= 15 )
                    tier = 3;
                else if ( tier_one[i].level > 5 && tier_one[i].level < 15 )
                    tier = 2;
                else
                    tier = 1;

                name = tier_one[i].name;
                item_type = tier_one[i].item_type;
                wearflags = tier_one[i].wear_flags;
                weight = tier_one[i].weight;
                cost = tier_one[i].weight;
                mnum = 1;

                if ( ch->pcdata->tradelevel <= 1 )
                    adj = "poorly";
                else if ( ch->pcdata->tradelevel <= 5 )
                    adj = "simply";
                else if ( ch->pcdata->tradelevel <= 8 )
                    adj = "properly";
                else if ( ch->pcdata->tradelevel <= 10 )
                    adj = "well";
                else if ( ch->pcdata->tradelevel <= 15 )
                    adj = "finely";
                else if ( ch->pcdata->tradelevel <= 19 )
                    adj = "masterfully";
                else
                    adj = "legendary";

                sprintf( name_buf, "%s", name );
                sprintf( short_buf, "a %s, %s forged from %s %s", name, adj,
                         smith_ores_table[x].color, smith_ores_table[x].name );
                sprintf( long_buf, "Here lies a %s, %s forged from %s %s.", name, adj,
                         smith_ores_table[x].color, smith_ores_table[x].name );
                item->item_type = item_type;
                item->wear_flags += wearflags;
                item->weight = weight;
                item->cost = cost;
                if ( item->item_type == ITEM_ARMOR ) {
                    send_to_char( "Its a newly forged piece of armor!\r\n", ch );
                    item->value[0] = set_min_armor( item->level );
                    item->value[1] = set_max_armor( item->level );
                }
                if ( item->item_type == ITEM_WEAPON ) {
                    send_to_char( "Its a newly forged weapon!\r\n", ch );
                    item->value[0] = 12;
                    item->value[1] = set_tier_min( item->level, tier );
                    item->value[2] = set_tier_max( item->level, tier );
                    item->value[3] = tier_one[i].base_v3;
                    item->value[4] = tier_one[i].base_v4;
                }
            }

            sprintf( extra_buf,
                     "\r\n&CThis crafted item bears the seal of %s, the %s blacksmith.\r\n",
                     ch->name,
                     ch->pcdata->tradelevel <= 5 ? "apprentice" : ch->pcdata->tradelevel <=
                     10 ? "journeyman" : ch->pcdata->tradelevel <=
                     19 ? "expert" : ch->pcdata->tradelevel == 20 ? "master" : "reknowned" );

            if ( item->name )
                STRFREE( item->name );
            if ( item->short_descr )
                STRFREE( item->short_descr );
            if ( item->description )
                STRFREE( item->description );

            item->name = STRALLOC( name_buf );
            item->short_descr = STRALLOC( short_buf );
            item->description = STRALLOC( long_buf );

            EXTRA_DESCR_DATA       *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );

            LINK( ed, item->first_extradesc, item->last_extradesc, next, prev );
            ed->keyword = STRALLOC( item->name );
            ed->description = STRALLOC( extra_buf );

            if ( item->item_type == ITEM_WEAPON ) {
                if ( item->value[4] == 0 || item->value[4] == 4 || item->value[4] == 8 ) {
                    short                   bonus;

                    bonus = item->level / 5;
                    if ( bonus < 1 ) {
                        bonus = 1;
                    }
                    item->value[1] =
                        set_min_chart( item->level ) + bonus * 3 + set_min_chart( item->level / 2 );
                    item->value[2] =
                        set_max_chart( item->level ) + bonus + ( set_max_chart( item->level ) / 2 );
                    item->pIndexData->value[1] =
                        set_min_chart( item->level ) + bonus * 3 + set_min_chart( item->level / 2 );
                    item->pIndexData->value[2] =
                        set_max_chart( item->level ) + bonus + ( set_max_chart( item->level ) / 2 );
                    item->weight = 15;
                }

                if ( ch->pcdata->tradelevel <= 5 ) {
                    item->value[0] = 6;
                    item->value[1] -= 1;
                    item->value[2] -= 1;
                    GET_VALUE( item, type ) = CURR_BRONZE;
                    item->cost = 25;
                }

                if ( ch->pcdata->tradelevel > 5 ) {
                    int                     modifier;

                    if ( ch->pcdata->tradelevel >= 15 ) {
                        if ( item->level >= 90 )
                            modifier = 15;
                        else if ( item->level >= 70 )
                            modifier = 8;
                        else if ( item->level >= 50 )
                            modifier = 5;
                        else if ( item->level >= 30 )
                            modifier = 3;
                        else
                            modifier = 2;
                    }
                    else if ( ch->pcdata->tradelevel >= 20 ) {
                        if ( item->level >= 90 )
                            modifier = 12;
                        else if ( item->level >= 70 )
                            modifier = 10;
                        else if ( item->level >= 50 )
                            modifier = 8;
                        else if ( item->level >= 30 )
                            modifier = 6;
                        else
                            modifier = 4;
                    }
                    else
                        modifier = 1;

                    item->value[0] = 8;

                    /*
                     * Chance to make it better or worser 
                     */
                    if ( number_percent(  ) > 50 )
                        modifier += number_range( 1, 5 );
                    else if ( number_percent(  ) < 50 )
                        modifier -= number_range( 1, 5 );
                    if ( modifier == 0 )
                        modifier = 1;
                    CREATE( paf, AFFECT_DATA, 1 );

                    paf->type = sn;
                    paf->duration = -1;
                    paf->location = APPLY_HITROLL;
                    paf->modifier = modifier;
                    xCLEAR_BITS( paf->bitvector );
                    LINK( paf, item->first_affect, item->last_affect, next, prev );
                    GET_VALUE( item, type ) = CURR_SILVER;
                    item->cost = 10;
                }
                if ( ch->pcdata->tradelevel >= 15 ) {
                    int                     modifier;

                    if ( ch->pcdata->tradelevel >= 20 ) {
                        if ( item->level >= 90 )
                            modifier = 12;
                        else if ( item->level >= 70 )
                            modifier = 10;
                        else if ( item->level >= 50 )
                            modifier = 8;
                        else if ( item->level >= 30 )
                            modifier = 6;
                        else
                            modifier = 4;
                    }
                    else {
                        if ( item->level >= 90 )
                            modifier = 10;
                        else if ( item->level >= 70 )
                            modifier = 8;
                        else if ( item->level >= 50 )
                            modifier = 6;
                        else if ( item->level >= 30 )
                            modifier = 4;
                        else
                            modifier = 3;
                    }
                    item->value[0] = 10;
                    item->value[1] += 1;
                    item->value[2] += 1;

                    /*
                     * Chance to make it better or worser 
                     */
                    if ( number_percent(  ) > 50 )
                        modifier += number_range( 1, 5 );
                    else if ( number_percent(  ) < 50 )
                        modifier -= number_range( 1, 5 );
                    if ( modifier == 0 )
                        modifier = 1;
                    CREATE( paf, AFFECT_DATA, 1 );

                    paf->type = sn;
                    paf->duration = -1;
                    paf->location = APPLY_DAMROLL;
                    paf->modifier = modifier;
                    xCLEAR_BITS( paf->bitvector );
                    LINK( paf, item->first_affect, item->last_affect, next, prev );

                    GET_VALUE( item, type ) = CURR_SILVER;
                    item->cost = 25;
                }
                if ( ch->pcdata->tradelevel >= 20 ) {
                    GET_VALUE( item, type ) = CURR_GOLD;
                    item->cost = 25;
                }
            }
            else if ( item->item_type == ITEM_ARMOR ) {
                CREATE( paf, AFFECT_DATA, 1 );

                paf->type = sn;
                paf->duration = -1;
                paf->location = APPLY_HIT;

                if ( ch->pcdata->tradelevel >= 20 ) {
                    if ( item->level >= 90 )
                        paf->modifier = 20;
                    else if ( item->level >= 70 )
                        paf->modifier = 10;
                    else if ( item->level >= 50 )
                        paf->modifier = 8;
                    else if ( item->level >= 30 )
                        paf->modifier = 5;
                    else
                        paf->modifier = 3;
                    GET_VALUE( item, type ) = CURR_GOLD;
                    item->cost = 25;
                }
                else if ( ch->pcdata->tradelevel >= 15 ) {
                    if ( item->level >= 90 )
                        paf->modifier = 15;
                    else if ( item->level >= 70 )
                        paf->modifier = 8;
                    else if ( item->level >= 50 )
                        paf->modifier = 5;
                    else if ( item->level >= 30 )
                        paf->modifier = 3;
                    else
                        paf->modifier = 2;
                    GET_VALUE( item, type ) = CURR_SILVER;
                    item->cost = 25;
                }
                else if ( ch->pcdata->tradelevel > 5 ) {
                    paf->modifier = 2;
                    GET_VALUE( item, type ) = CURR_SILVER;
                    item->cost = 10;
                }
                else {
                    GET_VALUE( item, type ) = CURR_BRONZE;
                    item->cost = 25;
                    paf->modifier = 1;
                }

                /*
                 * Chance to make it better or worser 
                 */
                if ( number_percent(  ) > 50 )
                    paf->modifier += number_range( 1, 5 );
                else if ( number_percent(  ) < 50 )
                    paf->modifier -= number_range( 1, 5 );
                if ( paf->modifier == 0 )
                    paf->modifier = 1;

                xCLEAR_BITS( paf->bitvector );
                LINK( paf, item->first_affect, item->last_affect, next, prev );

                if ( number_percent(  ) > 90 ) {
                    CREATE( paf, AFFECT_DATA, 1 );

                    paf->type = sn;
                    paf->duration = -1;
                    paf->location = APPLY_AC;
                    paf->modifier = number_range( -10, 10 );
                    xCLEAR_BITS( paf->bitvector );
                    LINK( paf, item->first_affect, item->last_affect, next, prev );
                }
                item->value[1] += get_armor_ac_mod( item->level, ch->pcdata->tradelevel );
                item->value[0] = item->value[1];
            }

            {
                int                     ichange = 0;

                if ( !str_cmp( arg1, "silver" ) )
                    ichange = 3;
                else if ( !str_cmp( arg1, "gold" ) )
                    ichange = 5;
                else if ( !str_cmp( arg1, "iron" ) )
                    ichange = 8;
                else if ( !str_cmp( arg1, "steel" ) )
                    ichange = 10;
                else if ( !str_cmp( arg1, "mithril" ) )
                    ichange = 12;
                else if ( !str_cmp( arg1, "adamantite" ) )
                    ichange = 15;

                if ( item->item_type == ITEM_WEAPON ) {
                    item->value[0] += ichange;
                    item->value[2] += ichange;
                }
                if ( item->item_type == ITEM_ARMOR ) {
                    item->value[0] += ichange;
                    item->value[1] += ichange;
                }
            }

            if ( ch->carry_number + get_obj_number( item ) > can_carry_n( ch ) ) {
                send_to_char
                    ( "You can't carry that many items, and drop the ore into the furnace.\r\n",
                      ch );
                extract_obj( item );
                return;
            }

            if ( ( ch->carry_weight + get_obj_weight( item, FALSE ) ) > can_carry_w( ch ) ) {
                send_to_char
                    ( "You can't carry that much weight, and drop the ore into the furnace.\r\n",
                      ch );
                extract_obj( item );
                return;
            }

            obj_to_char( item, ch );

            {
                short                   extinguish = number_chance( 1, 8 );

                if ( extinguish == 8 ) {
                    send_to_char
                        ( "\r\n&wThe forge burns the last of the coal and the flame is extinguished.\r\n",
                          ch );
                    stove->value[0] = 0;
                }
            }

            if ( IS_CLANNED( ch ) ) {
                CLAN_DATA              *clan;

                clan = ch->pcdata->clan;
                ch->pcdata->clanpoints += 1;
                clan->totalpoints += 1;
                ch_printf( ch,
                           "\r\n&G%s clan has gained a status point from your craftsmanship, now totaling %d clan status points!\r\n",
                           clan->name, clan->totalpoints );
                save_char_obj( ch );
                save_clan( clan );
            }
            return;
        }
        tail_chain(  );
    }
}

void send_tan_syntax( CHAR_DATA *ch )
{
    if ( !ch )
        return;

    send_to_char( "&cSyntax: tan <&Chide&c> into <&Citem&c> <&Clevel&c>\r\n", ch );
    send_to_char( "Syntax: tan list\r\n", ch );
    send_to_char
        ( "Note: There are &C4 steps&c, keep doing the tan command until the last step.\r\n", ch );
}

void do_tan( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *skin;
    OBJ_DATA               *item;
    char                    arg1[MIL],
                            buf[MSL],
                            arg2[MIL],
                            arg3[MIL],
                            arg4[MIL];
    char                    name_buf[MSL],
                            short_buf[MSL],
                            long_buf[MSL],
                            extra_buf[MSL];
    const char             *adj;
    int                     i = 0,
        x = 0,
        sn = 1,
        usetan,
        usehunt;
    bool                    itm = FALSE,
        hasskin = FALSE,
        found = FALSE;
    short                   chance;
    AFFECT_DATA            *paf;
    struct skill_type      *skill = NULL;

    chance = number_range( 1, 10 );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );

    name_buf[0] = '\0';
    short_buf[0] = '\0';
    long_buf[0] = '\0';
    extra_buf[0] = '\0';

    if ( IS_NPC( ch ) )
        return;

    if ( arg1[0] == '\0' ) {
        send_tan_syntax( ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
        int                     y,
                                count = 0;
        char                    wearloc[MSL];

        send_to_char( "\t\t\t&CThe Tannery Listing\r\n\r\n", ch );
        for ( y = 0; y < MAX_TANNED; y++ ) {
            if ( ch->pcdata->tradelevel < tanned_one[y].level )
                continue;

            count++;

            /*
             * We don't want the spaces so done seperate and then spaces handled where sent 
             */
            snprintf( wearloc, sizeof( wearloc ), "&c(&w%s&c)%s",
                      IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_FINGER ) ? "finger"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_NECK ) ? "neck"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_BODY ) ? "body"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_HEAD ) ? "head"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_LEGS ) ? "legs"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_FEET ) ? "feet"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_HANDS ) ? "hands"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_ARMS ) ? "arms"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_SHIELD ) ? "shield"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_ABOUT ) ? "about"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_WAIST ) ? "waist"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_WRIST ) ? "wrist"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WIELD ) ? "wield"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_HOLD ) ? "hold"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_DUAL_WIELD ) ? "dual"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_EARS ) ? "ears"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_EYES ) ? "eyes"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_MISSILE_WIELD ) ? "missile"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_BACK ) ? "back"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_FACE ) ? "face"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_ANKLE ) ? "ankle"
                      : IS_SET( tanned_one[y].wear_flags, ITEM_WEAR_SHEATH ) ? "sheath" : "unknown",
                      tanned_one[y].name ? tanned_one[y].name : "null name" );

            ch_printf( ch, "&c%-28s ", wearloc );

            if ( count == 3 ) {
                send_to_char( "\r\n", ch );
                count = 0;
            }
        }
        if ( count != 0 )
            send_to_char( "\r\n", ch );
        send_tan_syntax( ch );
        return;
    }

    if ( arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_tan_syntax( ch );
        return;

    }
    if ( str_cmp( arg2, "into" ) ) {
        send_tan_syntax( ch );
        return;
    }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_TRADESKILLS ) ) {
        send_to_char( "You must be in a tradeskills building to do this.\r\n", ch );
        return;
    }

    {
        if ( ( skin = get_obj_carry( ch, arg1 ) ) != NULL ) {
            separate_obj( skin );
            if ( skin->item_type != ITEM_RAW ) {
                send_to_char( "This is not a proper animal hide.\r\n", ch );
                return;
            }

            for ( x = 0; x < MAX_HUNTED; x++ ) {
                if ( !str_cmp( arg1, hunted_one[x].name ) ) {
                    usehunt = x;
                    hasskin = TRUE;
                    break;
                }
            }

            if ( !hasskin ) {
                send_to_char( "This is not a proper animal hide.\r\n", ch );
                return;
            }

            for ( i = 0; i < MAX_TANNED; i++ ) {
                if ( !str_cmp( arg3, tanned_one[i].name ) ) {
                    usetan = i;
                    itm = TRUE;
                    break;
                }
            }

            if ( !itm || ch->pcdata->tradelevel < tanned_one[i].level ) {
                send_to_char( "This is not a valid item type.\r\n", ch );
                return;
            }

            SKILLTYPE              *skill = get_skilltype( gsn_tan );

            if ( ch->pcdata->tradelevel <= 1 )
                adj = "poorly";
            else if ( ch->pcdata->tradelevel <= 5 )
                adj = "simply";
            else if ( ch->pcdata->tradelevel <= 8 )
                adj = "properly";
            else if ( ch->pcdata->tradelevel <= 10 )
                adj = "well";
            else if ( ch->pcdata->tradelevel <= 15 )
                adj = "finely";
            else if ( ch->pcdata->tradelevel <= 19 )
                adj = "masterfully";
            else
                adj = "legendary";

            WAIT_STATE( ch, 15 );

            if ( skin->value[0] == 0 ) {
                if ( ch->pcdata->learned[gsn_tan] <= number_range( 1, 5 )
                     || number_percent(  ) <= 25 ) {
                    act( AT_CYAN,
                         "$n prepares the hide by placing it in a lime water mixture filled basin, frowns as $e realizes the hide has already started rotting.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You prepare the hide by placing it in a lime water mixture filled basin to wash off any blood, but you realize the hide has already started rotting.",
                         ch, NULL, NULL, TO_CHAR );
                    obj_from_char( skin );
                    extract_obj( skin );
                    learn_from_failure( ch, gsn_tan );
                    return;
                }

                act( AT_CYAN,
                     "$n prepares the hide by placing it in the lime water mixture filled basin.",
                     ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN,
                     "You prepare the hide by placing it in the lime water mixture filled basin, and begin scrubbing off any blood.",
                     ch, NULL, NULL, TO_CHAR );

                skin->value[0] = 1;
                for ( x = 0; x < MAX_HUNTED; x++ ) {
                    if ( !str_cmp( arg1, hunted_one[x].name ) ) {
                        snprintf( buf, MSL, "hide %s", hunted_one[x].name );
                        if ( VLD_STR( skin->name ) )
                            STRFREE( skin->name );
                        skin->name = STRALLOC( buf );
                        break;
                    }
                }
                if ( VLD_STR( skin->short_descr ) )
                    STRFREE( skin->short_descr );
                skin->short_descr = STRALLOC( "a prepared hide" );
                skin->color = 1;
                if ( VLD_STR( skin->description ) )
                    STRFREE( skin->description );
                skin->description = STRALLOC( "A prepared hide has been left here." );
                learn_from_success( ch, gsn_tan );
                return;
            }

            if ( skin->value[0] == 1 ) {
                if ( !can_use_skill( ch, number_percent(  ), gsn_tan ) || number_percent(  ) <= 25 ) {
                    act( AT_CYAN, "$n scrubs away at the hide.", ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN, "You scrub away at the hide.", ch, NULL, NULL, TO_CHAR );
                    learn_from_failure( ch, gsn_tan );
                    return;
                }

                act( AT_CYAN, "$n places nutgals tannic acid on the hide to preserve it.", ch, NULL,
                     NULL, TO_ROOM );
                act( AT_CYAN, "You place nutgals tannic acid on the hide to preserve it.", ch, NULL,
                     NULL, TO_CHAR );
                skin->value[0] = 2;
                for ( x = 0; x < MAX_HUNTED; x++ ) {
                    if ( !str_cmp( arg1, hunted_one[x].name ) ) {
                        snprintf( buf, MSL, "hide %s", hunted_one[x].name );
                        if ( VLD_STR( skin->name ) )
                            STRFREE( skin->name );
                        skin->name = STRALLOC( buf );
                        break;
                    }
                }
                if ( VLD_STR( skin->short_descr ) )
                    STRFREE( skin->short_descr );
                skin->short_descr = STRALLOC( "a preserved hide" );
                skin->color = 1;
                if ( VLD_STR( skin->description ) )
                    STRFREE( skin->description );
                skin->description = STRALLOC( "A preserved hide has been left here." );
                learn_from_success( ch, gsn_tan );
                return;
            }

            if ( skin->value[0] == 2 ) {
                act( AT_CYAN, "$n treats the hide with linseed oil, and cuts it into shapes.", ch,
                     NULL, NULL, TO_ROOM );
                act( AT_CYAN,
                     "You treat the hide with linseed oil, and cut it into the desired shape.", ch,
                     NULL, NULL, TO_CHAR );
                skin->value[0] = 3;
                for ( x = 0; x < MAX_HUNTED; x++ ) {
                    if ( !str_cmp( arg1, hunted_one[x].name ) ) {
                        snprintf( buf, MSL, "hide %s", hunted_one[x].name );
                        if ( VLD_STR( skin->name ) )
                            STRFREE( skin->name );
                        skin->name = STRALLOC( buf );
                        break;
                    }
                }
                if ( VLD_STR( skin->short_descr ) )
                    STRFREE( skin->short_descr );
                skin->short_descr = STRALLOC( "a pattern of hide shapes" );
                skin->color = 1;
                if ( VLD_STR( skin->description ) )
                    STRFREE( skin->description );
                skin->description = STRALLOC( "A pattern of hide shapes has been left here." );
                learn_from_success( ch, gsn_tan );
                return;
            }

            act( AT_CYAN, "$n sews the hide pattern shapes into a tanned final product.", ch, NULL,
                 NULL, TO_ROOM );
            act( AT_CYAN, "You sew the hide pattern shapes into a tanned final product.", ch, NULL,
                 NULL, TO_CHAR );

            obj_from_char( skin );
            extract_obj( skin );

            learn_from_craft( ch, gsn_tan );
            item = create_object( get_obj_index( SMITH_PRODUCT ), 1 );
            if ( arg4[0] == '\0' ) {
                item->level = ch->level;
            }
            else
                item->level = atoi( arg4 );

            i = usetan;
            x = usehunt;

            sprintf( name_buf, "%s", tanned_one[i].name );
            sprintf( short_buf, "%s, %s tanned from %s", tanned_one[i].short_descr, adj,
                     hunted_one[x].name );
            sprintf( long_buf, "Here lies %s, %s tanned from %s.", tanned_one[i].short_descr, adj,
                     hunted_one[x].name );
            item->item_type = tanned_one[i].item_type;
            item->wear_flags += tanned_one[i].wear_flags;
            item->weight = ( tanned_one[i].weight );
            item->cost = tanned_one[i].weight;
            item->value[0] = set_min_armor( item->level );
            item->value[1] = set_max_armor( item->level );
            item->pIndexData->value[0] = set_min_armor( item->level );
            item->pIndexData->value[1] = set_max_armor( item->level );

            if ( ch->pcdata->tradelevel < 5 ) {
                item->value[0] = item->value[0] - 1;
                item->value[1] = item->value[1] - 1;
                GET_VALUE( item, type ) = CURR_BRONZE;
                item->cost = 25;
            }

            if ( ch->pcdata->tradelevel < 15 && ch->pcdata->tradelevel >= 5 ) {
                CREATE( paf, AFFECT_DATA, 1 );

                paf->type = sn;
                paf->duration = -1;
                paf->location = APPLY_HIT;
                paf->modifier = 1;
                xCLEAR_BITS( paf->bitvector );
                LINK( paf, item->first_affect, item->last_affect, next, prev );
                GET_VALUE( item, type ) = CURR_SILVER;
                item->cost = 10;
            }

            if ( ch->pcdata->tradelevel < 20 && ch->pcdata->tradelevel >= 15 ) {
                CREATE( paf, AFFECT_DATA, 1 );

                paf->type = sn;
                paf->duration = -1;
                paf->location = APPLY_HIT;
                if ( item->level >= 90 )
                    paf->modifier = 15;
                if ( item->level >= 70 && item->level < 90 )
                    paf->modifier = 8;
                if ( item->level >= 50 && item->level < 70 )
                    paf->modifier = 5;
                if ( item->level >= 30 && item->level < 50 )
                    paf->modifier = 3;
                if ( item->level < 30 )
                    paf->modifier = 2;
                xCLEAR_BITS( paf->bitvector );
                LINK( paf, item->first_affect, item->last_affect, next, prev );
                GET_VALUE( item, type ) = CURR_SILVER;
                item->cost = 20;
            }

            if ( ch->pcdata->tradelevel >= 20 ) {
                CREATE( paf, AFFECT_DATA, 1 );

                paf->type = sn;
                paf->duration = -1;
                paf->location = APPLY_HIT;
                if ( item->level >= 90 )
                    paf->modifier = 20;
                if ( item->level >= 70 && item->level < 90 )
                    paf->modifier = 10;
                if ( item->level >= 50 && item->level < 70 )
                    paf->modifier = 8;
                if ( item->level >= 30 && item->level < 50 )
                    paf->modifier = 5;
                if ( item->level < 30 )
                    paf->modifier = 3;
                xCLEAR_BITS( paf->bitvector );
                LINK( paf, item->first_affect, item->last_affect, next, prev );
                GET_VALUE( item, type ) = CURR_GOLD;
                item->cost = 10;
            }

            sprintf( extra_buf,
                     "\r\n&CThis tanned item bears the seal of %s, the %s tanner.\r\n",
                     ch->name,
                     ch->pcdata->tradelevel <=
                     5 ? "apprentice" : ch->pcdata->tradelevel <=
                     10 ? "journeyman" : ch->pcdata->tradelevel <=
                     19 ? "expert" : ch->pcdata->tradelevel == 20 ? "master" : "reknowned" );
            if ( VLD_STR( item->name ) )
                STRFREE( item->name );
            item->name = STRALLOC( name_buf );
            if ( VLD_STR( item->short_descr ) )
                STRFREE( item->short_descr );
            item->short_descr = STRALLOC( short_buf );
            if ( VLD_STR( item->description ) )
                STRFREE( item->description );
            item->description = STRALLOC( long_buf );

            EXTRA_DESCR_DATA       *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );

            LINK( ed, item->first_extradesc, item->last_extradesc, next, prev );
            ed->keyword = STRALLOC( item->name );
            ed->description = STRALLOC( extra_buf );
            item->color = 1;
            obj_to_char( item, ch );
            CLAN_DATA              *clan;

            if ( IS_CLANNED( ch ) ) {
                clan = ch->pcdata->clan;
                ch->pcdata->clanpoints += 1;
                clan->totalpoints += 1;
                ch_printf( ch,
                           "\r\n&G%s clan has gained a status point from your craftsmanship, now totaling %d clan status points!\r\n",
                           clan->name, clan->totalpoints );
                save_char_obj( ch );
                save_clan( clan );
            }

            return;
        }
        else {
            send_tan_syntax( ch );
            return;
        }
        tail_chain(  );
    }
}

void do_hunt( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *arrow,
                           *bow,
                           *slice,
                           *skin;
    CHAR_DATA              *victim = NULL,
        *mob;
    MOB_INDEX_DATA         *prey;
    char                    buf[MSL],
                            arg1[MIL],
                            name_buf[MSL],
                            short_buf[MSL],
                            long_buf[MSL];
    int                     i = 0;
    short                   chance;
    bool                    iprey = FALSE,
        found = FALSE;

    chance = number_range( 1, 10 );

    argument = one_argument( argument, arg1 );

    if ( IS_NPC( ch ) )
        return;

    switch ( ch->in_room->sector_type ) {
        case SECT_OCEAN:
        case SECT_ARCTIC:
        case SECT_BEACH:
        case SECT_FOG:
        case SECT_SKY:
        case SECT_CLOUD:
        case SECT_SNOW:
        case SECT_ORE:
        case SECT_QUICKSAND:
        case SECT_DEEPMUD:
        case SECT_PORTALSTONE:
        case SECT_LAKE:
        case SECT_CAMPSITE:
        case SECT_DOCK:
        case SECT_RIVER:
        case SECT_WATERFALL:
        case SECT_CROSSROAD:
        case SECT_VROAD:
        case SECT_HROAD:
        case SECT_ROAD:
        case SECT_CITY:
        case SECT_LAVA:
        case SECT_OCEANFLOOR:
        case SECT_AREA_ENT:
        case SECT_AIR:
        case SECT_UNDERWATER:
        case SECT_INSIDE:
            send_to_char( "You cannot hunt here.\r\n", ch );
            return;

        default:
            break;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
        send_to_char( "You can't do that in here.\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->area->flags, AFLAG_INDOORS ) ) {
        send_to_char( "You can't do that in here.\r\n", ch );
        return;
    }

    if ( ch->move < 5 ) {
        send_to_char( "You don't have enough energy to go keep hunting.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "&cSyntax: Hunt <&Cprey&c>\r\n", ch );
        send_to_char( "Syntax: Hunt list\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
        int                     count = 0;

        send_to_char( "\t\t\t&CThe Listing of Huntable Prey\r\n\r\n", ch );
        for ( int y = 0; y < MAX_HUNTED; y++ ) {
            if ( ch->pcdata->tradelevel < hunted_one[y].tlevel )
                continue;
            count++;
            ch_printf( ch, "&c%-15s      ", hunted_one[y].name ? hunted_one[y].name : "null name" );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
        if ( count != 0 )
            send_to_char( "\r\n", ch );
        send_to_char( "\r\n&cSyntax: Hunt <&Cprey&c>\r\n", ch );
        send_to_char( "Syntax: Hunt list\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_hunt]->beats );

    if ( !can_use_skill( ch, number_percent(  ), gsn_hunt ) ) {
        learn_from_failure( ch, gsn_hunt );
        send_to_char( "You stalk the area for your prey, but find nothing.\r\n", ch );
        return;
    }

    if ( !( bow = get_eq_char( ch, WEAR_MISSILE_WIELD ) ) ) {
        send_to_char( "But you are not wielding a missile weapon!!\r\n", ch );
        return;
    }

    if ( ( arrow = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "You are not holding a projectile!\r\n", ch );
        return;
    }

    if ( arrow->item_type != ITEM_PROJECTILE ) {
        send_to_char( "You are not holding a projectile!\r\n", ch );
        return;
    }

    if ( ( ch->carry_number + 2 ) > can_carry_n( ch ) ) {
        send_to_char( "You can't hunt anymore because you can't carry that much.\r\n", ch );
        return;
    }

    ch->move -= 1;

    for ( i = 0; i < MAX_HUNTED; i++ ) {
        if ( !str_cmp( arg1, hunted_one[i].name ) ) {
            iprey = TRUE;
            break;
        }
    }

    if ( !iprey || ch->pcdata->tradelevel < hunted_one[i].tlevel ) {
        send_to_char( "This is not something you can hunt.\r\n", ch );
        return;
    }

    if ( !( prey = get_mob_index( MOB_VNUM_PREY ) ) ) {
        bug( "hunt: prey vnum %d doesn't exist.", MOB_VNUM_PREY );
        return;
    }

    if ( ch->pcdata->learned[gsn_hunt] <= 50 && chance <= 3 ) {
        send_to_char( "You begin to stalk the area for your prey.\r\n", ch );
        send_to_char( "A small game trail has tracks and droppings of your prey you track.\r\n",
                      ch );
        pager_printf( ch, "\r\n&GYour stalking pays off as you successfully found a %s!\r\n",
                      arg1 );
        act( AT_GREEN, "$n stalks up upon an animal.\r\n", ch, NULL, NULL, TO_ROOM );
        pager_printf( ch,
                      "\r\n&GYour lack of hunting skills shows as the %s smells you and runs away!\r\n",
                      arg1 );
        act( AT_GREEN, "$n's looks away in disgust as $e realizes the animal ran off.\r\n", ch,
             NULL, NULL, TO_ROOM );
        learn_from_failure( ch, gsn_hunt );
        return;
    }

    mob = create_mobile( prey );

    sprintf( name_buf, "%s", hunted_one[i].name );
    sprintf( short_buf, "%s", hunted_one[i].short_descr );
    sprintf( long_buf, "%s", hunted_one[i].long_descr );
    mob->race = hunted_one[i].race;
    mob->level = hunted_one[i].level;

    send_to_char( "You begin to stalk the area for your prey.\r\n", ch );
    send_to_char( "A small game trail has tracks and droppings of your prey you track.\r\n", ch );
    pager_printf( ch, "\r\n&GYour stalking pays off as you successfully found a %s!\r\n", arg1 );
    act( AT_GREEN, "$n stalks up upon an animal.\r\n", ch, NULL, NULL, TO_ROOM );

    if ( mob->name )
        STRFREE( mob->name );
    mob->name = STRALLOC( name_buf );
    if ( mob->short_descr )
        STRFREE( mob->short_descr );
    mob->short_descr = STRALLOC( short_buf );
    if ( mob->long_descr )
        STRFREE( mob->long_descr );
    mob->long_descr = STRALLOC( long_buf );
    pager_printf( ch,
                  "\r\n&RAn adrenalin rush fills you from spotting a %s as you fumble for your arrow!\r\n",
                  arg1 );
    pager_printf( ch, "\r\n&cYou notch your arrow and take careful aim.\r\n" );
    pager_printf( ch, "\r\n&GYou let loose your arrow striking %s in the heart!\r\n", arg1 );
    act( AT_GREEN, "$n suddenly shoots $s bow at something.\r\n", ch, NULL, NULL, TO_ROOM );
    char_to_room( mob, ch->in_room );

    if ( !( victim = mob ) ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    set_cur_char( victim );
    raw_kill( ch, victim );
    extract_obj( arrow );

    pager_printf( ch,
                  "\r\n&cFollowing the trail of blood to your prey, you begin to gut the animal\r\nso the meat will not rot.  You cut out the meat, and separate the animal's hide.\r\n" );

    slice = create_object( get_obj_index( OBJ_VNUM_SLICE ), 0 );
    if ( VLD_STR( slice->name ) )
        STRFREE( slice->name );
    slice->name = STRALLOC( "meat fresh slice" );
    snprintf( buf, MSL, "a slice of raw meat from %s", mob->short_descr );
    if ( VLD_STR( slice->short_descr ) )
        STRFREE( slice->short_descr );
    slice->short_descr = STRALLOC( buf );
    snprintf( buf, MSL, "A slice of raw meat from %s lies on the ground.", mob->short_descr );
    if ( VLD_STR( slice->description ) )
        STRFREE( slice->description );
    slice->description = STRALLOC( buf );
    obj_to_char( slice, ch );

    skin = create_object( get_obj_index( OBJ_VNUM_SKIN ), 0 );
    snprintf( buf, MSL, "hide %s", mob->name );
    if ( VLD_STR( skin->name ) )
        STRFREE( skin->name );
    skin->name = STRALLOC( buf );
    snprintf( buf, MSL, "a freshly cut hide from %s", mob->short_descr );
    if ( VLD_STR( skin->short_descr ) )
        STRFREE( skin->short_descr );
    skin->short_descr = STRALLOC( buf );
    snprintf( buf, MSL, "A freshly cut hide from %s lies on the ground.", mob->short_descr );
    if ( VLD_STR( skin->description ) )
        STRFREE( skin->description );
    skin->description = STRALLOC( buf );
    skin->item_type = ITEM_RAW;
    obj_to_char( skin, ch );
    if ( xIS_SET( ch->act, PLR_AUTOTRASH ) )
        do_trash( ch, ( char * ) "corpse" );
    learn_from_craft( ch, gsn_hunt );
}

void do_gather( CHAR_DATA *ch, char *argument )
{
    char                    name_buf[MSL];
    char                    short_buf[MSL];
    char                    long_buf[MSL];
    OBJ_DATA               *obj;
    char                    arg1[MIL];
    OBJ_DATA               *item;
    int                     i = 0;
    bool                    itm = FALSE,
        found = FALSE;
    short                   chance;

    chance = number_range( 1, 10 );

    argument = one_argument( argument, arg1 );

    if ( IS_NPC( ch ) )
        return;

    switch ( ch->in_room->sector_type ) {
        case SECT_OCEAN:
        case SECT_ARCTIC:
        case SECT_BEACH:
        case SECT_FOG:
        case SECT_SKY:
        case SECT_CLOUD:
        case SECT_SNOW:
        case SECT_ORE:
        case SECT_QUICKSAND:
        case SECT_DEEPMUD:
        case SECT_PORTALSTONE:
        case SECT_LAKE:
        case SECT_CAMPSITE:
        case SECT_DOCK:
        case SECT_RIVER:
        case SECT_WATERFALL:
        case SECT_CROSSROAD:
        case SECT_VROAD:
        case SECT_HROAD:
        case SECT_ROAD:
        case SECT_CITY:
        case SECT_SWAMP:
        case SECT_LAVA:
        case SECT_OCEANFLOOR:
        case SECT_AREA_ENT:
        case SECT_AIR:
        case SECT_UNDERWATER:
        case SECT_INSIDE:
            send_to_char( "You cannot gather here.\r\n", ch );
            return;

        default:
            break;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
        send_to_char( "You can't do that in here.\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->area->flags, AFLAG_INDOORS ) ) {
        send_to_char( "You can't do that in here.\r\n", ch );
        return;
    }

    if ( ch->move < 5 ) {
        send_to_char( "You don't have enough energy to go keep gathering.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "&cSyntax: Gather <&Cingredient&c>\r\n", ch );
        send_to_char( "Syntax: Gather list\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
        send_to_char( "\t\t\t&CThe Listing of Gatherable Items\r\n\r\n", ch );
        for ( int y = 0; y < MAX_FRUIT; y++ ) {
            ch_printf( ch, "&c%-15s      ",
                       fruit_types[y].name ? fruit_types[y].name : "null name" );
            if ( ( y + 1 ) % 4 == 0 )
                send_to_char( "\r\n", ch );
        }
        send_to_char( "\r\n&cSyntax: Gather <&Cingredient&c>\r\n", ch );
        send_to_char( "Syntax: Gather list\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_gather]->beats );

    if ( !can_use_skill( ch, number_percent(  ), gsn_gather ) ) {
        learn_from_failure( ch, gsn_gather );
        send_to_char( "You look around for things to gather, but find nothing.\r\n", ch );
        return;
    }

    if ( ( ch->carry_number + 1 ) > can_carry_n( ch ) ) {
        send_to_char( "You can't gather anymore because you can't carry anything else.\r\n", ch );
        return;
    }

    ch->move = ch->move - 1;

    if ( ch->pcdata->learned[gsn_gather] <= 50 && chance <= 3 ) {
        send_to_char( "You begin to search the area for ingredients to gather.\r\n", ch );
        pager_printf( ch, "\r\n&GYour gathering pays off as you successfully gathered %s!\r\n",
                      arg1 );
        act( AT_GREEN, "$n reaches down and gathers some ingredient.\r\n", ch, NULL, NULL,
             TO_ROOM );
        pager_printf( ch, "\r\n&GYour lack of gathering skills shows as the %s is rotten!\r\n",
                      arg1 );
        act( AT_GREEN, "$n's looks away in disgust as $e realizes the ingredient is rotten.\r\n",
             ch, NULL, NULL, TO_ROOM );
        learn_from_failure( ch, gsn_gather );
        return;
    }

    for ( i = 0; i < MAX_FRUIT; i++ ) {
        if ( !str_cmp( arg1, fruit_types[i].name ) ) {
            itm = TRUE;
            break;
        }
    }

    // May need to comment out this check to get it to work.
    if ( itm == FALSE ) {
        send_to_char( "This is not something you can gather.\r\n", ch );
        return;
    }

    item = create_object( get_obj_index( OBJ_VNUM_GATHER ), 1 );

    sprintf( name_buf, "%s", fruit_types[i].name );
    sprintf( short_buf, "a handful of %s", fruit_types[i].name );
    sprintf( long_buf, "A handful of %s has been left here.", fruit_types[i].name );
    item->item_type = fruit_types[i].item_type;
    item->wear_flags += fruit_types[i].wear_flags;
    item->weight = ( fruit_types[i].weight );
    item->cost = fruit_types[i].weight;
    item->color = fruit_types[i].base_v4;

    if ( ( ch->carry_weight + item->weight ) > can_carry_w( ch ) ) {
        send_to_char( "It's too heavy for you to lift with everything else you're holding.\r\n",
                      ch );
        extract_obj( item );
        return;
    }

    if ( item->name )
        STRFREE( item->name );
    if ( item->short_descr )
        STRFREE( item->short_descr );
    if ( item->description )
        STRFREE( item->description );

    item->name = STRALLOC( name_buf );
    item->short_descr = STRALLOC( short_buf );
    item->description = STRALLOC( long_buf );

    send_to_char( "You begin to search the area for ingredients to gather.\r\n", ch );
    pager_printf( ch, "\r\n&GYour gathering pays off as you successfully gathered %s!\r\n",
                  item->short_descr );
    act( AT_GREEN, "$n reaches down and gathers $p.\r\n", ch, item, NULL, TO_ROOM );
    learn_from_craft( ch, gsn_gather );
    WAIT_STATE( ch, skill_table[gsn_gather]->beats );
    obj_to_char( item, ch );
}

void send_bake_syntax( CHAR_DATA *ch )
{
    if ( !ch )
        return;
    send_to_char( "\r\n&cSyntax: bake <&Cingredient&c> into <&Cfood&c> <&Clevel&c>\r\n", ch );
    send_to_char( "&cSyntax: bake list\r\n", ch );
    send_to_char( "&cSyntax: bake fire\r\n", ch );
    send_to_char
        ( "&cNote: There are &C4 steps&c, keep doing the bake command until the last step.\r\n",
          ch );
}

void do_bake( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *bakedfood;
    OBJ_DATA               *ingredient;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    arg4[MIL];
    char                    name_buf[MSL];
    char                    short_buf[MSL];
    char                    long_buf[MSL];
    char                    extra_buf[MSL];
    const char             *adj;
    bool                    itm = FALSE,
        hasingred = FALSE;
    int                     i = 0,
        x = 0;
    struct skill_type      *skill = NULL;

    name_buf[0] = '\0';
    short_buf[0] = '\0';
    long_buf[0] = '\0';
    extra_buf[0] = '\0';

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );

    if ( IS_NPC( ch ) || ch->desc == NULL )
        return;

    if ( arg1[0] == '\0' ) {
        send_bake_syntax( ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
        int                     count = 0;

        send_to_char( "\t\t\t&CThe Baked Food Listing\r\n\r\n", ch );
        for ( int y = 0; y < MAX_FOOD; y++ ) {
            if ( ch->pcdata->tradelevel < food_one[y].level )
                continue;
            count++;
            ch_printf( ch, "&c%-15s      ", food_one[y].name ? food_one[y].name : "null name" );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
        if ( count != 0 )
            send_to_char( "\r\n", ch );
        send_bake_syntax( ch );
        return;
    }

    if ( !str_cmp( arg1, "fire" ) ) {
        OBJ_DATA               *stove;
        bool                    found;

        found = FALSE;

        for ( stove = ch->in_room->first_content; stove; stove = stove->next_content ) {
            if ( stove->item_type == ITEM_STOVE ) {
                found = TRUE;
                break;
            }
        }

        if ( !found ) {
            send_to_char( "There must be an oven to fire it.\r\n", ch );
            return;
        }

        if ( stove->value[0] == 1 ) {
            send_to_char( "There is no need to fire the oven, it is already.\r\n", ch );
            return;
        }

        OBJ_DATA               *coal;

        for ( coal = ch->first_carrying; coal; coal = coal->next_content ) {
            if ( coal->item_type == ITEM_COAL )
                break;
        }

        if ( !coal ) {
            send_to_char( "You do not have any coal to fire the oven.\r\n", ch );
            return;
        }

        separate_obj( coal );
        obj_from_char( coal );

        act( AT_CYAN, "$n fires up the oven lighting the coal within it.", ch, NULL, NULL,
             TO_ROOM );
        act( AT_CYAN, "You fire the oven lighting the coal within it.", ch, NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "\r\nA flame flickers within the oven.", ch, NULL, NULL, TO_ROOM );
        act( AT_YELLOW, "\r\nA flame flickers within the oven.", ch, NULL, NULL, TO_CHAR );
        extract_obj( coal );
        stove->value[0] = 1;
        return;
    }

    if ( arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_bake_syntax( ch );
        return;

    }
    if ( str_cmp( arg2, "into" ) ) {
        send_bake_syntax( ch );
        return;
    }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_TRADESKILLS ) ) {
        send_to_char( "You must be in a tradeskills building to do this.\r\n", ch );
        return;
    }

    {
        OBJ_DATA               *stove;
        bool                    found;

        found = FALSE;

        for ( stove = ch->in_room->first_content; stove; stove = stove->next_content ) {
            if ( stove->item_type == ITEM_STOVE ) {
                found = TRUE;
                break;
            }
        }

        if ( !found ) {
            send_to_char( "There must be a oven in the room in order to do that.\r\n", ch );
            return;
        }

        if ( stove->value[0] != 1 ) {
            send_to_char( "&cYou have to fire the oven first.\r\n", ch );
            send_to_char( "&cSyntax: bake fire\r\n", ch );
            return;
        }

        if ( ( ingredient = get_obj_carry( ch, arg1 ) ) != NULL ) {
            separate_obj( ingredient );

            if ( ingredient->item_type != ITEM_RAW ) {
                send_to_char( "This is not a proper ingredient.\r\n", ch );
                return;
            }
            for ( x = 0; x < MAX_FRUIT; x++ ) {
                if ( !str_cmp( ingredient->name, fruit_types[x].name ) ) {
                    hasingred = TRUE;
                    break;
                }
            }

            if ( !hasingred ) {
                send_to_char( "This is not a proper ingredient.\r\n", ch );
                return;
            }

            // This is the nitty gritty of checking tier output
            for ( i = 0; i < MAX_FOOD; i++ ) {
                if ( ch->pcdata->tradelevel < food_one[i].level )
                    continue;
                if ( !str_cmp( arg3, food_one[i].name ) ) {
                    itm = TRUE;
                    break;
                }
            }

            if ( itm == FALSE ) {
                send_to_char( "This is not a valid food type.\r\n", ch );
                return;
            }

            SKILLTYPE              *skill = get_skilltype( gsn_bake );

            if ( ch->pcdata->tradelevel <= 1 )
                adj = "poorly";
            else if ( ch->pcdata->tradelevel <= 5 )
                adj = "simply";
            else if ( ch->pcdata->tradelevel <= 8 )
                adj = "properly";
            else if ( ch->pcdata->tradelevel <= 10 )
                adj = "well";
            else if ( ch->pcdata->tradelevel <= 15 )
                adj = "finely";
            else if ( ch->pcdata->tradelevel <= 19 )
                adj = "masterfully";
            else
                adj = "legendary";

            WAIT_STATE( ch, 15 );

            if ( ingredient->value[0] == 0 ) {
                if ( ch->pcdata->learned[gsn_bake] <= number_range( 1, 5 )
                     || number_percent(  ) <= 25 ) {
                    act( AT_CYAN,
                         "$n prepares the ingredient by placing it in the oven, but the ingredient is left too long and burns.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You prepare the ingredient by placing it in the oven, but the ingredient is left too long and burns.",
                         ch, NULL, NULL, TO_CHAR );
                    extract_obj( ingredient );
                    learn_from_failure( ch, gsn_bake );
                    return;
                }

                act( AT_CYAN, "$n prepares the ingredient by placing it in the oven.", ch, NULL,
                     NULL, TO_ROOM );
                act( AT_CYAN, "You prepare the ingredient by placing it in the oven.", ch, NULL,
                     NULL, TO_CHAR );
                ingredient->value[0] += 1;
                ingredient->short_descr = STRALLOC( "a cooked dish" );
                learn_from_success( ch, gsn_bake );
                return;
            }

            if ( ingredient->value[0] == 1 ) {
                if ( ch->pcdata->learned[gsn_bake] <= number_range( 1, 10 )
                     || number_percent(  ) <= 25 ) {
                    act( AT_CYAN,
                         "$n places the cooked dish on the table and begins to add garnishments but gets distracted and ruins the dish by adding too much.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You place the cooked dish on the table and begin to add garnishments, but get distracted and ruin the dish by adding too much.",
                         ch, NULL, NULL, TO_CHAR );
                    extract_obj( ingredient );
                    learn_from_failure( ch, gsn_bake );
                    return;
                }

                act( AT_CYAN,
                     "$n places the cooked dish on the table and begins to add garnishments.", ch,
                     NULL, NULL, TO_ROOM );
                act( AT_CYAN,
                     "You place the cooked dish on the table and begin to add garnishments.", ch,
                     NULL, NULL, TO_CHAR );
                ingredient->value[0] += 1;
                ingredient->short_descr = STRALLOC( "a prepped dish" );
                learn_from_success( ch, gsn_bake );
                return;
            }

            if ( ingredient->value[0] == 2 ) {
                if ( !can_use_skill( ch, number_percent(  ), gsn_bake )
                     || number_percent(  ) <= 25 ) {
                    act( AT_CYAN,
                         "$n tastes the dish to see if it is right, then keeps adding to it.", ch,
                         NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You taste the dish to see if it is right, then continue adding to it.",
                         ch, NULL, NULL, TO_CHAR );
                    learn_from_failure( ch, gsn_bake );
                    return;
                }

                if ( number_percent(  ) <= 25 ) {
                    act( AT_CYAN,
                         "$n begins to stir the prepped dish, but gets distracted and ruins the dish by over stirring it.",
                         ch, NULL, NULL, TO_ROOM );
                    act( AT_CYAN,
                         "You begin to stir the prepped dish, but get distracted and ruin the dish by over stirring it.",
                         ch, NULL, NULL, TO_CHAR );
                    extract_obj( ingredient );
                    learn_from_failure( ch, gsn_bake );
                    return;
                }
                act( AT_CYAN, "$n begins to stir the prepped dish.", ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN, "You begin to stir the prepped dish.", ch, NULL, NULL, TO_CHAR );
                ingredient->value[0] += 1;
                ingredient->short_descr = STRALLOC( "a prepared dish" );
                learn_from_success( ch, gsn_bake );
                return;
            }

            if ( number_percent(  ) <= 25 ) {
                act( AT_CYAN,
                     "$n takes out some spices, but gets distracted and ruins the dish by adding too much.",
                     ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN,
                     "You take out some spices, but get distracted and ruin the dish by adding too much.",
                     ch, NULL, NULL, TO_CHAR );
                extract_obj( ingredient );
                learn_from_failure( ch, gsn_bake );
                return;
            }

            act( AT_CYAN, "$n takes out some spices, and adds the finishing touches.", ch, NULL,
                 NULL, TO_ROOM );
            act( AT_CYAN, "You take out some spices, and add the finishing touches.", ch, NULL,
                 NULL, TO_CHAR );

            learn_from_craft( ch, gsn_bake );
            bakedfood = create_object( get_obj_index( OBJ_VNUM_CRAFTFOOD ), 1 );
            if ( arg4[0] == '\0' )
                bakedfood->level = ch->level;
            else
                bakedfood->level = atoi( arg4 );

            found = FALSE;
            for ( i = 0; i < MAX_FOOD; i++ ) {
                if ( ch->pcdata->tradelevel < food_one[i].level )
                    continue;
                if ( !str_cmp( arg3, food_one[i].name ) ) {
                    found = TRUE;
                    break;
                }
            }

            if ( !found ) {
                send_to_char( "Can't create it for some odd reason, inform a STAFF member.\r\n",
                              ch );
                return;
            }

            sprintf( name_buf, "%s", food_one[i].name );
            sprintf( short_buf, "a %s, %s cooked from %s", food_one[i].name, adj,
                     STRALLOC( arg1 ) );
            sprintf( long_buf, "Here lies a %s, %s cooked from %s.", food_one[i].name, adj,
                     STRALLOC( arg1 ) );
            bakedfood->item_type = food_one[i].item_type;
            bakedfood->wear_flags += food_one[i].wear_flags;
            bakedfood->weight = ( food_one[i].weight );
            bakedfood->cost = food_one[i].weight;
            bakedfood->value[6] = 1;
            bakedfood->value[0] = 5;
            bakedfood->pIndexData->value[6] = 1;
            if ( ch->pcdata->tradelevel >= 20 ) {
                GET_VALUE( bakedfood, type ) = CURR_GOLD;
                bakedfood->cost = 30;
            }
            else if ( ch->pcdata->tradelevel >= 15 ) {
                GET_VALUE( bakedfood, type ) = CURR_GOLD;
                bakedfood->cost = 25;
            }
            else if ( ch->pcdata->tradelevel >= 10 ) {
                GET_VALUE( bakedfood, type ) = CURR_GOLD;
                bakedfood->cost = 15;
            }
            else if ( ch->pcdata->tradelevel >= 5 ) {
                GET_VALUE( bakedfood, type ) = CURR_SILVER;
                bakedfood->cost = 50;
            }
            else {
                GET_VALUE( bakedfood, type ) = CURR_BRONZE;
                bakedfood->cost = 25;
            }

            sprintf( extra_buf,
                     "\r\n&CThis crafted dish bears the seal of %s, the %s baker.\r\n",
                     ch->name,
                     ch->pcdata->tradelevel <=
                     5 ? "apprentice" : ch->pcdata->tradelevel <=
                     10 ? "journeyman" : ch->pcdata->tradelevel <=
                     19 ? "expert" : ch->pcdata->tradelevel == 20 ? "master" : "reknowned" );
            if ( bakedfood->name )
                STRFREE( bakedfood->name );
            if ( bakedfood->short_descr )
                STRFREE( bakedfood->short_descr );
            if ( bakedfood->description )
                STRFREE( bakedfood->description );

            bakedfood->name = STRALLOC( name_buf );
            bakedfood->short_descr = STRALLOC( short_buf );
            bakedfood->description = STRALLOC( long_buf );

            EXTRA_DESCR_DATA       *ed;

            CREATE( ed, EXTRA_DESCR_DATA, 1 );

            LINK( ed, bakedfood->first_extradesc, bakedfood->last_extradesc, next, prev );
            ed->keyword = STRALLOC( bakedfood->name );
            ed->description = STRALLOC( extra_buf );

            if ( ch->pcdata->tradelevel <= 1 )
                bakedfood->value[1] = 1;
            else if ( ch->pcdata->tradelevel <= 5 )
                bakedfood->value[1] = 3;
            else if ( ch->pcdata->tradelevel <= 8 )
                bakedfood->value[1] = 5;
            else if ( ch->pcdata->tradelevel <= 10 )
                bakedfood->value[1] = 8;
            else if ( ch->pcdata->tradelevel <= 15 )
                bakedfood->value[1] = 10;
            else if ( ch->pcdata->tradelevel <= 19 )
                bakedfood->value[1] = 12;
            else
                bakedfood->value[1] = 15;

            bakedfood->color = ingredient->color;
            extract_obj( ingredient );

            if ( ch->carry_number + get_obj_number( bakedfood ) > can_carry_n( ch ) ) {
                send_to_char
                    ( "You can't carry that many items, and drop the food on the floor.\r\n", ch );
                extract_obj( bakedfood );
                return;
            }

            if ( ch->carry_weight + ( get_obj_weight( bakedfood, FALSE ) * 1 ) + ( 1 > 1 ? 2 : 0 ) >
                 can_carry_w( ch ) ) {
                send_to_char
                    ( "You can't carry that much weight, and drop the food on the floor.\r\n", ch );
                extract_obj( bakedfood );
                return;
            }

            obj_to_char( bakedfood, ch );
            short                   extinguish;

            extinguish = number_chance( 1, 8 );
            if ( extinguish == 8 ) {
                send_to_char
                    ( "\r\n&wThe oven burns the last of the coal and the flame is extinguished.\r\n",
                      ch );
                stove->value[0] = 0;
            }
            CLAN_DATA              *clan;

            if ( IS_CLANNED( ch ) ) {
                clan = ch->pcdata->clan;
                ch->pcdata->clanpoints += 1;
                clan->totalpoints += 1;
                ch_printf( ch,
                           "\r\n&G%s clan has gained a status point from your craftsmanship, now totaling %d clan status points!\r\n",
                           clan->name, clan->totalpoints );
                save_char_obj( ch );
                save_clan( clan );
            }

            return;
        }
        else {
            send_bake_syntax( ch );
            return;
        }
        tail_chain(  );
    }
}

void send_mix_syntax( CHAR_DATA *ch )
{
    if ( !ch )
        return;
    send_to_char( "&cSyntax: mix <&Cingredient&c> into <&Cdrink&c> <&Clevel&c>\r\n", ch );
    send_to_char( "Syntax: mix list\r\n", ch );
    send_to_char
        ( "Note: There are &C4 steps&c, keep doing the mix command until the last step.\r\n", ch );
}

void do_mix( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *mixeddrink;
    OBJ_DATA               *ingredient;
    char                    arg1[MIL];
    char                    arg2[MIL];
    char                    arg3[MIL];
    char                    arg4[MIL];
    char                    name_buf[MSL];
    char                    short_buf[MSL];
    char                    long_buf[MSL];
    char                    extra_buf[MSL];
    const char             *adj;
    bool                    itm = FALSE,
        hasingred = FALSE;
    int                     i = 0,
        x = 0;

    name_buf[0] = '\0';
    short_buf[0] = '\0';
    long_buf[0] = '\0';
    extra_buf[0] = '\0';
    bool                    found;

    found = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );

    if ( IS_NPC( ch ) || ch->desc == NULL )
        return;

    if ( arg1[0] == '\0' ) {
        send_mix_syntax( ch );
        return;
    }

    if ( !str_cmp( arg1, "list" ) ) {
        int                     count = 0;

        send_to_char( "\t\t\t&CThe Mixed Drink Listing\r\n\r\n", ch );
        for ( int y = 0; y < MAX_DRINK; y++ ) {
            if ( ch->pcdata->tradelevel < drink_one[y].level )
                continue;
            count++;
            ch_printf( ch, "&c%-15s      ", drink_one[y].name ? drink_one[y].name : "null name" );
            if ( count == 3 ) {
                count = 0;
                send_to_char( "\r\n", ch );
            }
        }
        if ( count != 0 )
            send_to_char( "\r\n", ch );
        send_mix_syntax( ch );
        return;
    }

    if ( arg2[0] == '\0' || arg3[0] == '\0' || str_cmp( arg2, "into" ) ) {
        send_mix_syntax( ch );
        return;
    }

    if ( !IS_SET( ch->in_room->room_flags, ROOM_TRADESKILLS ) ) {
        send_to_char( "You must be in a tradeskills building to do this.\r\n", ch );
        return;
    }

    if ( !( ingredient = get_obj_carry( ch, arg1 ) ) ) {
        send_mix_syntax( ch );
        return;
    }

    separate_obj( ingredient );

    if ( ingredient->item_type != ITEM_RAW ) {
        send_to_char( "This is not a proper ingredient.\r\n", ch );
        return;
    }

    for ( x = 0; x < MAX_FRUIT; x++ ) {
        if ( !str_cmp( ingredient->name, fruit_types[x].name ) ) {
            hasingred = TRUE;
            break;
        }
    }
    if ( !hasingred ) {
        send_to_char( "This is not a proper ingredient.\r\n", ch );
        return;
    }

    i = drink_lookup( arg3 );
    if ( i < 0 || ch->pcdata->tradelevel < drink_one[i].level ) {
        send_to_char( "That isn't a valid drink to mix.\r\n", ch );
        return;
    }

    if ( ch->pcdata->tradelevel <= 1 )
        adj = "poorly";
    else if ( ch->pcdata->tradelevel <= 5 )
        adj = "simply";
    else if ( ch->pcdata->tradelevel <= 8 )
        adj = "properly";
    else if ( ch->pcdata->tradelevel <= 10 )
        adj = "well";
    else if ( ch->pcdata->tradelevel <= 15 )
        adj = "finely";
    else if ( ch->pcdata->tradelevel <= 19 )
        adj = "masterfully";
    else
        adj = "legendary";

    WAIT_STATE( ch, 15 );

    if ( ingredient->value[0] == 0 ) {
        if ( ch->pcdata->learned[gsn_mix] <= 5 || number_percent(  ) < 10 ) {
            act( AT_CYAN,
                 "$n prepares the ingredient by placing it in the mixing bowl, and grinding it up.\r\nWhile grinding the ingredient $e notices that maggots are in it, and it is now ruined.",
                 ch, NULL, NULL, TO_ROOM );
            act( AT_CYAN,
                 "You prepare the ingredient by placing it in the mixing bowl, and grinding it up.\r\nWhile grinding the ingredient you notice that maggots are in it, and it is now ruined.",
                 ch, NULL, NULL, TO_CHAR );
            obj_from_char( ingredient );
            extract_obj( ingredient );
            learn_from_failure( ch, gsn_mix );
            return;
        }
        act( AT_CYAN,
             "$n prepares the ingredient by placing it in the mixing bowl, and grinding it up.", ch,
             NULL, NULL, TO_ROOM );
        act( AT_CYAN,
             "You prepare the ingredient by placing it in the mixing bowl, and grinding it up.", ch,
             NULL, NULL, TO_CHAR );
        ingredient->value[0] += 1;
        ingredient->short_descr = STRALLOC( "a powdery mix" );
        learn_from_success( ch, gsn_mix );
        return;
    }

    if ( ingredient->value[0] == 1 ) {
        if ( ch->pcdata->learned[gsn_mix] <= 10 || number_percent(  ) < 20 ) {
            act( AT_CYAN,
                 "$n places the mixing bowl on the table and begins adding honey to sweeten it.\r\n$n realizes the drink is ruined because $e added too much.",
                 ch, NULL, NULL, TO_ROOM );
            act( AT_CYAN,
                 "You place the mixing bowl on the table and begin adding honey to sweeten it.\r\nYou realize the drink is ruined because you added too much.",
                 ch, NULL, NULL, TO_CHAR );
            obj_from_char( ingredient );
            extract_obj( ingredient );
            learn_from_failure( ch, gsn_mix );
            return;
        }

        act( AT_CYAN,
             "$n places the mixing bowl on the table and begins adding honey to sweeten it.", ch,
             NULL, NULL, TO_ROOM );
        act( AT_CYAN,
             "You place the mixing bowl on the table and begin adding honey to sweeten it.", ch,
             NULL, NULL, TO_CHAR );
        ingredient->value[0] += 1;
        ingredient->short_descr = STRALLOC( "a sweetened mix" );
        learn_from_success( ch, gsn_mix );
        return;
    }

    if ( ingredient->value[0] == 2 ) {
        if ( ch->pcdata->learned[gsn_mix] <= 15 || number_percent(  ) < 25 ) {
            act( AT_CYAN,
                 "$n begins to add fluids to the sweetened mix.\r\n$n realizes the drink is ruined because $e has added too much.",
                 ch, NULL, NULL, TO_ROOM );
            act( AT_CYAN,
                 "You begin to add fluids to the sweetened mix.\r\nYou realize the drink is ruined because you have added too much.",
                 ch, NULL, NULL, TO_CHAR );
            obj_from_char( ingredient );
            extract_obj( ingredient );
            learn_from_failure( ch, gsn_mix );
            return;
        }

        act( AT_CYAN, "$n begins to add fluids to the sweetened mix.", ch, NULL, NULL, TO_ROOM );
        act( AT_CYAN, "You begin to add fluids to the sweetened mix.", ch, NULL, NULL, TO_CHAR );
        ingredient->value[0] += 1;
        ingredient->short_descr = STRALLOC( "a watery mix" );
        learn_from_success( ch, gsn_mix );
        return;
    }

    if ( ingredient->value[0] == 3 ) {
        if ( ch->pcdata->learned[gsn_mix] <= 15 || number_percent(  ) < 25 ) {
            act( AT_CYAN,
                 "$n takes out some garnishments, and realizes that while adding the finishing touches $e has ruined it.\r\n",
                 ch, NULL, NULL, TO_ROOM );
            act( AT_CYAN,
                 "You take out some garnishments, and realize that while adding the finishing touches you have ruined it.",
                 ch, NULL, NULL, TO_CHAR );
            obj_from_char( ingredient );
            extract_obj( ingredient );
            learn_from_failure( ch, gsn_mix );
            return;
        }
        act( AT_CYAN, "$n takes out some garnishments, and adds the finishing touch.", ch, NULL,
             NULL, TO_ROOM );
        act( AT_CYAN, "You take out some garnishments, and add the finishing touch.", ch, NULL,
             NULL, TO_CHAR );
        ingredient->value[0] += 1;
    }

    learn_from_craft( ch, gsn_mix );
    mixeddrink = create_object( get_obj_index( OBJ_VNUM_CRAFTFOOD ), 1 );
    if ( arg4[0] == '\0' )
        mixeddrink->level = ch->level;
    else
        mixeddrink->level = atoi( arg4 );

    sprintf( name_buf, "%s", drink_one[i].name );
    sprintf( short_buf, "a %s, %s mixed from %s", drink_one[i].name, adj, STRALLOC( arg1 ) );
    sprintf( long_buf, "Here lies a %s, %s mixed from %s.", drink_one[i].name, adj,
             STRALLOC( arg1 ) );

    mixeddrink->item_type = drink_one[i].item_type;
    mixeddrink->wear_flags += drink_one[i].wear_flags;
    mixeddrink->weight = ( drink_one[i].weight );
    mixeddrink->cost = drink_one[i].weight;
    mixeddrink->value[0] = 1;
    mixeddrink->value[1] = 1;
    mixeddrink->value[2] = 16;
    mixeddrink->value[6] = 1;

    if ( ch->pcdata->tradelevel < 5 ) {
        GET_VALUE( mixeddrink, type ) = CURR_BRONZE;
        mixeddrink->cost = 25;
    }
    else if ( ch->pcdata->tradelevel >= 5 && ch->pcdata->tradelevel < 10 ) {
        GET_VALUE( mixeddrink, type ) = CURR_SILVER;
        mixeddrink->cost = 50;
    }
    else if ( ch->pcdata->tradelevel >= 10 && ch->pcdata->tradelevel < 15 ) {
        GET_VALUE( mixeddrink, type ) = CURR_GOLD;
        mixeddrink->cost = 15;
    }
    else if ( ch->pcdata->tradelevel >= 15 ) {
        GET_VALUE( mixeddrink, type ) = CURR_GOLD;
        mixeddrink->cost = 25;
    }

    sprintf( extra_buf, "\r\n&CThis crafted drink bears the seal of %s, the %s baker.\r\n",
             ch->name,
             ch->pcdata->tradelevel <= 5 ? "apprentice" : ch->pcdata->tradelevel <=
             10 ? "journeyman" : ch->pcdata->tradelevel <= 19 ? "expert" : ch->pcdata->tradelevel ==
             20 ? "master" : "reknowned" );

    if ( mixeddrink->name )
        STRFREE( mixeddrink->name );
    if ( mixeddrink->short_descr )
        STRFREE( mixeddrink->short_descr );
    if ( mixeddrink->description )
        STRFREE( mixeddrink->description );

    mixeddrink->name = STRALLOC( name_buf );
    mixeddrink->short_descr = STRALLOC( short_buf );
    mixeddrink->description = STRALLOC( long_buf );

    EXTRA_DESCR_DATA       *ed;

    CREATE( ed, EXTRA_DESCR_DATA, 1 );

    LINK( ed, mixeddrink->first_extradesc, mixeddrink->last_extradesc, next, prev );
    ed->keyword = STRALLOC( mixeddrink->name );
    ed->description = STRALLOC( extra_buf );

    obj_from_char( ingredient );
    extract_obj( ingredient );

    if ( ch->carry_number + get_obj_number( mixeddrink ) > can_carry_n( ch ) ) {
        send_to_char( "You can't carry that many items, and drop the food on the floor.\r\n", ch );
        extract_obj( mixeddrink );
        return;
    }

    if ( ch->carry_weight + ( get_obj_weight( mixeddrink, FALSE ) * 1 ) + ( 1 > 1 ? 2 : 0 ) >
         can_carry_w( ch ) ) {
        send_to_char( "You can't carry that much weight, and drop the food on the floor.\r\n", ch );
        extract_obj( mixeddrink );
        return;
    }

    CLAN_DATA              *clan;

    if ( IS_CLANNED( ch ) ) {
        clan = ch->pcdata->clan;
        ch->pcdata->clanpoints += 1;
        clan->totalpoints += 1;
        ch_printf( ch,
                   "\r\n&G%s clan has gained a status point from your craftsmanship, now totaling %d clan status points!\r\n",
                   clan->name, clan->totalpoints );
        save_clan( clan );
    }

    obj_to_char( mixeddrink, ch );
    save_char_obj( ch );
}
