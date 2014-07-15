/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 ***************************************************************************
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 * - Win32 port by Nick Gammon                                             *
 ***************************************************************************
 * - Main MUD header                                                       *
 ***************************************************************************/

/* Structure stuff */

typedef struct shop_data SHOP_DATA;
typedef struct repairshop_data REPAIR_DATA;

/*
 * Shop types.
 */
#define MAX_TRADE   5

struct shop_data
{
    SHOP_DATA              *next;                      /* Next shop in list */
    SHOP_DATA              *prev;                      /* Previous shop in list */
    int                     keeper;                    /* Vnum of shop keeper mob */
    short                   buy_type[MAX_TRADE];       /* Item types shop will buy */
    short                   profit_buy;                /* Cost multiplier for buying */
    short                   profit_sell;               /* Cost multiplier for selling */
    short                   open_hour;                 /* First opening hour */
    short                   close_hour;                /* First closing hour */
};

#define MAX_FIX  3
#define SHOP_FIX  1
#define SHOP_RECHARGE  2

struct repairshop_data
{
    REPAIR_DATA            *next;                      /* Next shop in list */
    REPAIR_DATA            *prev;                      /* Previous shop in list */
    int                     keeper;                    /* Vnum of shop keeper mob */
    short                   fix_type[MAX_FIX];         /* Item types shop will fix */
    short                   profit_fix;                /* Cost multiplier for fixing */
    short                   shop_type;                 /* Repair shop type */
    short                   open_hour;                 /* First opening hour */
    short                   close_hour;                /* First closing hour */
};

extern SHOP_DATA       *first_shop;
extern SHOP_DATA       *last_shop;
extern REPAIR_DATA     *first_repair;
extern REPAIR_DATA     *last_repair;
