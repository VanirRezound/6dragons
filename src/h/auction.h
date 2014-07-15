/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 * - Win32 port by Nick Gammon                                             *
 ***************************************************************************
 * - Main MUD header                                                       *
 ***************************************************************************/
/* structure stuff */
typedef struct auction_data AUCTION_DATA;              /* auction data */

/* how many items to track.... prevent repeat auctions */
#define AUCTION_MEM 3

struct auction_data
{
    OBJ_DATA               *item;                      /* a pointer to the item */
    CHAR_DATA              *seller;                    /* a pointer to the seller - which 
                                                        * may NOT quit */
    CHAR_DATA              *buyer;                     /* a pointer to the buyer - which
                                                        * may NOT quit */
    int                     bet;                       /* last bet - or 0 if noone has
                                                        * bet anything */
    short                   currtype;
    short                   going;                     /* 1,2, sold */
    short                   pulse;                     /* how many pulses (.25 sec) until 
                                                        * another call-out ? */
    int                     starting;
    OBJ_INDEX_DATA         *history[AUCTION_MEM];      /* store auction history */
    short                   hist_timer;                /* clear out history buffer if
                                                        * auction is idle */
};

/* Good ole externals */
extern AUCTION_DATA    *auction;
void                    auction_update( void );
