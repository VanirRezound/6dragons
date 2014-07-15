/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997-2002  Jesse DeFer
          http://www.dotd.com  dotd@dotd.com
 ******************************************************/
struct currency_data
{
    CURRENCY_DATA          *next_currency;
    CURRENCY_DATA          *prev_currency;
    short                   type;
    int                     tsiints;
    int                     charge;
};

struct currency_index_data
{
    CURR_INDEX_DATA        *next_currindex;
    CURR_INDEX_DATA        *prev_currindex;
    CURRENCY_DATA          *first_currency;
    CURRENCY_DATA          *last_currency;
    short                   primary;
    int                     vnum;
    int                     charge;
    char                   *name;
};

int get_currency_type   args( ( char *type ) );
float get_worth         args( ( CURRENCY_DATA * c1, CURRENCY_DATA * c2 ) );
short get_primary_curr  args( ( ROOM_INDEX_DATA *room ) );
int obj_cost            args( ( ROOM_INDEX_DATA *room, OBJ_DATA *obj, int currtype ) );

char                   *get_primary_curr_str( ROOM_INDEX_DATA *room );
int                     convert_curr( ROOM_INDEX_DATA *room, int amount, int fromtype, int totype );
int                     obj_primary_curr_value( ROOM_INDEX_DATA *room, OBJ_DATA *obj );
void                    assign_currindex( ROOM_INDEX_DATA *room );

int                     money_weight( int amount, int type );
int                     max_carry_money( CHAR_DATA *ch, int type );
