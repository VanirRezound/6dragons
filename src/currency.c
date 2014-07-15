/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997-2002  Jesse DeFer
          http://www.dotd.com  dotd@dotd.com
 ******************************************************/

/*static char rcsid[] = "$Id: currency.c,v 1.18 2002/10/12 20:06:09 dotd Exp $";*/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/currency.h"
#include "h/files.h"
#include "h/shops.h"
#include "h/key.h"

#define CURRENCY_FILE SYSTEM_DIR "currency.dat"

const char             *const curr_types[] = {
    "none", "gold", "silver", "bronze", "copper"
};

const char             *const cap_curr_types[] = {
    "None", "Gold", "Silver", "Bronze", "Copper"
};

CURR_INDEX_DATA        *first_curr_index;
CURR_INDEX_DATA        *last_curr_index;
CURR_INDEX_DATA        *mud_curr_index;

int get_currency_type( char *type )
{
    int                     x;

    if ( !str_cmp( type, "coin" ) || !str_cmp( type, "coins" ) )
        return DEFAULT_CURR;
    for ( x = 0; x < MAX_CURR_TYPE; x++ )
        if ( !str_cmp( type, curr_types[x] ) )
            return x;
    return CURR_NONE;
}

float get_worth( CURRENCY_DATA * c1, CURRENCY_DATA * c2 )
{
    if ( !c1 || !c2 )
        return 0.0;
    return ( ( float ) c1->tsiints / ( float ) c2->tsiints );
}

short get_primary_curr( ROOM_INDEX_DATA *room )
{
    if ( !room )
        return DEFAULT_CURR;
    if ( room->currindex )
        return room->currindex->primary;
    if ( room->area->currindex )
        return room->area->currindex->primary;
    return mud_curr_index->primary;
}

char                   *get_primary_curr_str( ROOM_INDEX_DATA *room )
{
    return ( char * ) cap_curr_types[get_primary_curr( room )];
}

void free_currency( CURR_INDEX_DATA * currindex, CURRENCY_DATA * curr )
{
    UNLINK( curr, currindex->first_currency, currindex->last_currency, next_currency,
            prev_currency );
    DISPOSE( curr );
}

void free_currindex( CURR_INDEX_DATA * currindex )
{
    CURRENCY_DATA          *curr;

    if ( !currindex )
        return;
    UNLINK( currindex, first_curr_index, last_curr_index, next_currindex, prev_currindex );
    while ( ( curr = currindex->first_currency ) )
        free_currency( currindex, curr );
    if ( currindex->name )
        STRFREE( currindex->name );
    DISPOSE( currindex );
}

void free_currencies( void )
{
    while ( first_curr_index )
        free_currindex( first_curr_index );
}

CURR_INDEX_DATA        *make_currindex( void )
{
    CURR_INDEX_DATA        *currindex;

    CREATE( currindex, CURR_INDEX_DATA, 1 );
    currindex->vnum = 0;
    currindex->primary = CURR_NONE;
    currindex->name = STRALLOC( "Unnamed" );
    currindex->first_currency = NULL;
    currindex->last_currency = NULL;
    LINK( currindex, first_curr_index, last_curr_index, next_currindex, prev_currindex );
    return currindex;
}

CURRENCY_DATA          *make_currency( CURR_INDEX_DATA * currindex )
{
    CURRENCY_DATA          *curr;

    CREATE( curr, CURRENCY_DATA, 1 );
    curr->type = CURR_NONE;
    curr->tsiints = 0;
    LINK( curr, currindex->first_currency, currindex->last_currency, next_currency, prev_currency );
    return curr;
}

CURR_INDEX_DATA        *find_currindex_vnum( int vnum )
{
    CURR_INDEX_DATA        *currindex;

    if ( !first_curr_index )
        return NULL;
    for ( currindex = first_curr_index; currindex; currindex = currindex->next_currindex )
        if ( currindex->vnum == vnum )
            return currindex;
    return NULL;
}

CURRENCY_DATA          *find_currency( CURR_INDEX_DATA * currindex, int type )
{
    CURRENCY_DATA          *curr;

    if ( !currindex || !currindex->first_currency )
        return NULL;
    for ( curr = currindex->first_currency; curr; curr = curr->next_currency )
        if ( curr->type == type )
            return curr;
    return NULL;
}

void init_mud_curr_index( void )
{
    CURRENCY_DATA          *c;
    int                     x;

    mud_curr_index = make_currindex(  );
    mud_curr_index->primary = DEFAULT_CURR;
    STRFREE( mud_curr_index->name );
    mud_curr_index->name = STRALLOC( "MUD-Wide Currency" );
    for ( x = 0; x < MAX_CURR_TYPE; x++ ) {
        c = make_currency( mud_curr_index );
        c->type = x;
        switch ( x ) {
            case CURR_GOLD:
                c->tsiints = 500;
                break;
            case CURR_SILVER:
                c->tsiints = 200;
                break;
            case CURR_BRONZE:
                c->tsiints = 100;
                break;
            case CURR_COPPER:
                c->tsiints = 10000;
                break;
            default:
                c->tsiints = 150;
                break;
        }
    }
}

int convert_curr( ROOM_INDEX_DATA *room, int amount, int fromtype, int totype )
{
    CURR_INDEX_DATA        *rc,
                           *ac;
    CURRENCY_DATA          *from,
                           *to;

    if ( !room ) {
        bug( "Non existant room passed to convert_curr." );
        return -1;
    }
    rc = room->currindex;
    ac = room->area->currindex;
    if ( !( from = find_currency( rc, fromtype ) ) )
        if ( !( from = find_currency( ac, fromtype ) ) )
            from = find_currency( mud_curr_index, fromtype );
    if ( !( to = find_currency( rc, totype ) ) )
        if ( !( to = find_currency( ac, totype ) ) )
            to = find_currency( mud_curr_index, totype );
    if ( !from || !to ) {
        bug( "Currency conversion returns -1 (%d to %d) for room %d.", fromtype, totype,
             room->vnum );
        return -1;
    }
    if ( from == to )
        return amount;
    return ( int ) ( ( float ) amount * get_worth( from, to ) );
}

int obj_primary_curr_value( ROOM_INDEX_DATA *room, OBJ_DATA *obj )
{
    if ( !obj ) {
        bug( "No obj given to obj_primary_curr_value." );
        return -1;
    }
    return convert_curr( room, obj->cost, obj->currtype, get_primary_curr( room ) );
}

void assign_currindex( ROOM_INDEX_DATA *room )
{
    if ( !room ) {
        bug( "CURRENCY: No room." );
        return;
    }
    if ( room->currvnum ) {
        room->currindex = find_currindex_vnum( room->currvnum );
        return;
    }
    if ( room->area->currvnum ) {
        room->currindex = find_currindex_vnum( room->area->currvnum );
        return;
    }
    room->currindex = mud_curr_index;
}

static void save_currency( void )
{
    CURR_INDEX_DATA        *currindex;
    CURRENCY_DATA          *curr;
    FILE                   *fp;

    if ( ( fp = FileOpen( CURRENCY_FILE, "w" ) ) == NULL ) {
        bug( "Unable to write currency file: %s", CURRENCY_FILE );
        return;
    }
    for ( currindex = first_curr_index; currindex; currindex = currindex->next_currindex ) {
        if ( currindex->vnum == 0 )
            continue;
        fprintf( fp, "#CURRINDEX\n" );
        fprintf( fp, "Vnum           %d\n", currindex->vnum );
        fprintf( fp, "Primary        %d\n", currindex->primary );
        fprintf( fp, "Name           %s~\n", currindex->name );
        fprintf( fp, "Charge         %d\n", currindex->charge );
        for ( curr = currindex->first_currency; curr; curr = curr->next_currency ) {
            fprintf( fp, "Currency       %d %d %d\n", curr->tsiints, curr->type, curr->charge );
        }
        fprintf( fp, "End\n\n" );
    }
    fprintf( fp, "#END\n" );
    FileClose( fp );
}

static void fread_currindex( CURR_INDEX_DATA * currindex, FILE * fp )
{
    const char             *word = NULL;
    bool                    fMatch = FALSE;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return;
                break;
            case 'C':
                KEY( "Charge", currindex->charge, fread_number( fp ) );
                if ( !str_cmp( word, "Currency" ) ) {
                    CURRENCY_DATA          *curr;

                    curr = make_currency( currindex );
                    curr->tsiints = fread_number( fp );
                    curr->type = fread_number( fp );
                    curr->charge = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'N':
                KEY( "Name", currindex->name, fread_string( fp ) );
                break;
            case 'P':
                KEY( "Primary", currindex->primary, fread_number( fp ) );
                break;
            case 'V':
                KEY( "Vnum", currindex->vnum, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_currency: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

void load_currency( void )
{
    FILE                   *fp;
    CURR_INDEX_DATA        *currindex;
    bool                    found = FALSE;

    init_mud_curr_index(  );
    if ( ( fp = FileOpen( CURRENCY_FILE, "r" ) ) != NULL ) {
        found = TRUE;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }
            if ( letter != '#' ) {
                bug( "load_currency: # not found." );
                break;
            }
            word = fread_word( fp );
            if ( !str_cmp( word, "CURRINDEX" ) ) {
                currindex = make_currindex(  );
                STRFREE( currindex->name );
                fread_currindex( currindex, fp );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "load_currency: bad section." );
                break;
            }
        }
        FileClose( fp );
    }
}

void do_listcurrency( CHAR_DATA *ch, char *argument )
{
    CURR_INDEX_DATA        *currindex;
    CURRENCY_DATA          *curr;

    if ( !first_curr_index ) {
        send_to_char( "There are no currencies.\r\n", ch );
        return;
    }
    for ( currindex = first_curr_index; currindex; currindex = currindex->next_currindex ) {
        ch_printf( ch, "%-5d: %-41s Charge: %d%%\r\n", currindex->vnum, currindex->name,
                   currindex->charge );
        if ( !currindex->first_currency )
            send_to_char( " This curremcy index has no currency.\r\n", ch );
        else
            for ( curr = currindex->first_currency; curr; curr = curr->next_currency )
                ch_printf( ch, " %cCurrency: %-20s Tsiints: %-5d  Charge: %d%%\r\n",
                           curr->type == currindex->primary ? '*' : ' ', curr_types[curr->type],
                           curr->tsiints, curr->charge );
    }
}

void do_setcurrency( CHAR_DATA *ch, char *argument )
{
    CURR_INDEX_DATA        *currindex;
    CURRENCY_DATA          *curr;
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            arg4[MIL];

    if ( !str_cmp( argument, "save" ) ) {
        save_currency(  );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: setcurrency save\r\n", ch );
        send_to_char( "Syntax: setcurrency <currindex vnum> <field> [arguments]\r\n\r\n", ch );
        send_to_char( "Field being one of:\r\n  create vnum name primary charge save delete\r\n",
                      ch );
        send_to_char( "Field may also be a currency type:\r\n", ch );
        send_to_char( "Syntax: setcurrency <currindex vnum> <type> <field> [arguments]\r\n", ch );
        send_to_char( "Field being one of:\r\n  tsiints charge delete\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "create" ) ) {
        int                     x = atoi( arg1 );

        if ( find_currindex_vnum( x ) ) {
            send_to_char( "A currency index with that vnum already exists.\r\n", ch );
            return;
        }
        currindex = make_currindex(  );
        currindex->vnum = x;
        if ( argument && *argument != '\0' ) {
            if ( currindex->name )
                STRFREE( currindex->name );
            currindex->name = STRALLOC( argument );
        }
        save_currency(  );
        assign_currindex( get_room_index( currindex->vnum ) );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !( currindex = find_currindex_vnum( atoi( arg1 ) ) ) ) {
        send_to_char( "Unable to find that currindex.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "delete" ) ) {
        free_currindex( currindex );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        do_setcurrency( ch, ( char * ) "" );
        return;
    }
    if ( !str_cmp( arg2, "name" ) ) {
        if ( currindex->name )
            STRFREE( currindex->name );
        currindex->name = STRALLOC( argument );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg3 );
    if ( !str_cmp( arg2, "primary" ) ) {
        currindex->primary = get_currency_type( arg3 );
        for ( curr = currindex->first_currency; curr; curr = curr->next_currency )
            if ( curr->type == currindex->primary )
                break;
        if ( !curr || curr->type != currindex->primary ) {
            currindex->primary = CURR_NONE;
            send_to_char( "Please use makecurrency before you set that as a primary.\r\n", ch );
            return;
        }
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "vnum" ) ) {
        if ( !is_number( arg3 ) ) {
            send_to_char( "Vnum must be a number.\r\n", ch );
            return;
        }
        if ( find_currindex_vnum( atoi( arg3 ) ) ) {
            send_to_char( "There is another currindex with that vnum.\r\n", ch );
            return;
        }
        currindex->vnum = atoi( arg3 );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "charge" ) ) {
        currindex->charge = URANGE( 0, atoi( arg3 ), 100 );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !( curr = find_currency( currindex, get_currency_type( arg2 ) ) ) ) {
        int                     type,
                                x;

        if ( ( type = get_currency_type( arg2 ) ) == CURR_NONE ) {
            send_to_char( "That is an invalid currency type.\r\n", ch );
            return;
        }
        send_to_char( "Creating currency.\r\n", ch );
        x = atoi( arg1 );
        curr = make_currency( currindex );
        curr->tsiints = x;
        curr->type = type;
        save_currency(  );
        return;
    }
    if ( !str_cmp( arg3, "delete" ) ) {
        if ( currindex->primary == curr->type ) {
            send_to_char( "Primary currency deleted.\r\n", ch );
            currindex->primary = CURR_NONE;
        }
        free_currency( currindex, curr );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        do_setcurrency( ch, ( char * ) "" );
        return;
    }
    argument = one_argument( argument, arg4 );
    if ( !is_number( arg4 ) ) {
        send_to_char( "The fourth argument must be a number.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg3, "tsiints" ) ) {
        curr->tsiints = atoi( arg4 );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg3, "charge" ) ) {
        curr->charge = UMIN( atoi( arg4 ), 100 );
        send_to_char( "Ok.\r\n", ch );
        return;
    }
    do_setcurrency( ch, ( char * ) "" );
}

void do_exchange( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *keeper;
    SHOP_DATA              *pShop;
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL];
    int                     value,
                            newvalue = 0,
        change = 0;
    int                     exchangetype = 0,
        newtype = 0;

    pShop = NULL;
    for ( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
        if ( IS_NPC( keeper ) && ( pShop = keeper->pIndexData->pShop ) != NULL )
            break;
    if ( ( !pShop ) && !IS_SET( ch->in_room->room_flags, ROOM_BANK ) ) {
        send_to_char( "You're not in a bank.\r\n", ch );
        return;
    }
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    if ( !VLD_STR( arg1 ) || !VLD_STR( arg2 ) || !VLD_STR( arg3 ) ) {
        send_to_char( "&CSyntax: exchange <amount> <cointype> <cointype>\r\n", ch );
        send_to_char( "&cExchange Rate\r\n", ch );
        send_to_char( "&COne Gold Coin   = 10 Silver Coins\r\n", ch );
        send_to_char( "&COne Silver Coin = 10 Bronze Coins\r\n", ch );
        send_to_char( "&COne Bronze Coin = 10 Copper Coins\r\n", ch );
        return;
    }
    value = atoi( arg1 );
    if ( value <= 0 ) {
        send_to_char( "Must use a number higher then 0.\r\n", ch );
        return;
    }

    if ( str_cmp( arg2, "gold" ) && str_cmp( arg2, "silver" ) && str_cmp( arg2, "bronze" )
         && str_cmp( arg2, "copper" ) ) {
        do_exchange( ch, ( char * ) "" );
        return;
    }
    if ( str_cmp( arg3, "gold" ) && str_cmp( arg3, "silver" ) && str_cmp( arg3, "bronze" )
         && str_cmp( arg3, "copper" ) ) {
        do_exchange( ch, ( char * ) "" );
        return;
    }

    /*
     * Go ahead and check values no point in going furthur if they dont have enough 
     */
    if ( !str_cmp( arg2, "gold" ) && GET_MONEY( ch, CURR_GOLD ) < value ) {
        send_to_char( "You don't have that much gold to exchange.\r\n", ch );
        return;
    }
    else if ( !str_cmp( arg2, "silver" ) && GET_MONEY( ch, CURR_SILVER ) < value ) {
        send_to_char( "You don't have that much silver to exchange.\r\n", ch );
        return;
    }
    else if ( !str_cmp( arg2, "bronze" ) && GET_MONEY( ch, CURR_BRONZE ) < value ) {
        send_to_char( "You don't have that much bronze to exchange.\r\n", ch );
        return;
    }
    else if ( !str_cmp( arg2, "copper" ) && GET_MONEY( ch, CURR_COPPER ) < value ) {
        send_to_char( "You don't have that much copper to exchange.\r\n", ch );
        return;
    }

    /*
     * Lets get types etc... 
     */
    if ( !str_cmp( arg2, "gold" ) )
        exchangetype = CURR_GOLD;
    else if ( !str_cmp( arg2, "silver" ) )
        exchangetype = CURR_SILVER;
    else if ( !str_cmp( arg2, "bronze" ) )
        exchangetype = CURR_BRONZE;
    else if ( !str_cmp( arg2, "copper" ) )
        exchangetype = CURR_COPPER;

    if ( !str_cmp( arg3, "gold" ) )
        newtype = CURR_GOLD;
    else if ( !str_cmp( arg3, "silver" ) )
        newtype = CURR_SILVER;
    else if ( !str_cmp( arg3, "bronze" ) )
        newtype = CURR_BRONZE;
    else if ( !str_cmp( arg3, "copper" ) )
        newtype = CURR_COPPER;

    /*
     * This would be pointless 
     */
    if ( exchangetype == newtype ) {
        send_to_char( "No point in exchanging for the same kind of currency.\r\n", ch );
        return;
    }

    /*
     * Get how much newvalue would be 
     */
    if ( !str_cmp( arg2, "gold" ) ) {
        if ( !str_cmp( arg3, "silver" ) ) {
            newvalue = ( value * 10 );
            change = ( value - ( newvalue / 10 ) );
        }
        else if ( !str_cmp( arg3, "bronze" ) ) {
            newvalue = ( value * 100 );
            change = ( value - ( newvalue / 100 ) );
        }
        else if ( !str_cmp( arg3, "copper" ) ) {
            newvalue = ( value * 1000 );
            change = ( value - ( newvalue / 1000 ) );
        }
    }
    else if ( !str_cmp( arg2, "silver" ) ) {
        if ( !str_cmp( arg3, "gold" ) ) {
            newvalue = ( value / 10 );
            change = ( value - ( newvalue * 10 ) );
        }
        else if ( !str_cmp( arg3, "bronze" ) ) {
            newvalue = ( value * 10 );
            change = ( value - ( newvalue / 10 ) );
        }
        else if ( !str_cmp( arg3, "copper" ) ) {
            newvalue = ( value * 100 );
            change = ( value - ( newvalue / 100 ) );
        }
    }
    else if ( !str_cmp( arg2, "bronze" ) ) {
        if ( !str_cmp( arg3, "gold" ) ) {
            newvalue = ( value / 100 );
            change = ( value - ( newvalue * 100 ) );
        }
        else if ( !str_cmp( arg3, "silver" ) ) {
            newvalue = ( value / 10 );
            change = ( value - ( newvalue * 10 ) );
        }
        else if ( !str_cmp( arg3, "copper" ) ) {
            newvalue = ( value * 10 );
            change = ( value - ( newvalue / 10 ) );
        }
    }
    else if ( !str_cmp( arg2, "copper" ) ) {
        if ( !str_cmp( arg3, "gold" ) ) {
            newvalue = ( value / 1000 );
            change = ( value - ( newvalue * 1000 ) );
        }
        else if ( !str_cmp( arg3, "silver" ) ) {
            newvalue = ( value / 100 );
            change = ( value - ( newvalue * 100 ) );
        }
        else if ( !str_cmp( arg3, "bronze" ) ) {
            newvalue = ( value / 10 );
            change = ( value - ( newvalue * 10 ) );
        }
    }

    /*
     * Make sure they would be gaining something 
     */
    if ( newvalue <= 0 ) {
        send_to_char( "Thats not enough to exchange.\r\n", ch );
        return;
    }

    /*
     * Take exchange currency 
     */
    GET_MONEY( ch, exchangetype ) -= value;
    /*
     * Return any change 
     */
    if ( change > 0 )
        GET_MONEY( ch, exchangetype ) += change;
    /*
     * Give new currency 
     */
    GET_MONEY( ch, newtype ) += newvalue;

    ch_printf( ch, "You exchange %d %s coins for %d %s coins.\r\n", value, arg2, newvalue, arg3 );
    if ( change > 0 )
        ch_printf( ch, "Your change of %d %s coins has been returned to you.\r\n", change, arg2 );
    return;
}

/* returns the cost of an object in currtype currency */
int obj_cost( ROOM_INDEX_DATA *room, OBJ_DATA *obj, int currtype )
{
    return convert_curr( room, obj->cost, obj->currtype, currtype );
}

/* coins per pound */
const int               money_weights[MAX_CURR_TYPE] = {
    1, 1000, 1500, 2000, 750
};

int money_weight( int amount, int type )
{
    return ( amount / money_weights[type] );
}

int max_carry_money( CHAR_DATA *ch, int type )
{
    int                     amount = can_carry_w( ch ) - ch->carry_weight;

    if ( amount > 1000000 )
        return ( 1000000 * money_weights[type] );
    return ( amount * money_weights[type] );
}
