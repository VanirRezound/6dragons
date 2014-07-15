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

extern const struct smith_ores_type smith_ores_table[];
extern const struct smith_items_type tier_one[];
extern const struct baker_table fruit_types[];
extern const struct baker_table food_one[];
extern const struct baker_table drink_one[];
extern const struct hunter_table hunted_one[];
extern const struct tanner_table tanned_one[];

typedef struct ReactivefailQQ REACTIVE_FAIL_DATA;

extern REACTIVE_FAIL_DATA *first_qq;
extern REACTIVE_FAIL_DATA *last_qq;

#define CRAFTQS_FILE "craftqs.dat"

struct ReactivefailQQ                                  /* hahah i'm hilarious - Volk */
{
    REACTIVE_FAIL_DATA     *next;
    REACTIVE_FAIL_DATA     *prev;
    const char             *question;                  /* The question itself! */
    const char             *answer1;                   /* The answer! */
    const char             *answer2;                   /* Might have other answers.. */
    const char             *answer3;                   /* as above */
    int                     trade;                     /* What trade it applies to */
    int                     number;                    /* Makes it easier to find */
};

struct smith_items_type
{
    const char             *name;
    int                     level;
    int                     cost;
    int                     weight;
    int                     wear_flags;
    int                     item_type;
    int                     base_v0;
    int                     base_v1;
    int                     base_v2;
    int                     base_v3;
    int                     base_v4;
    int                     base_v5;
    int                     base_v6;
};

struct baker_table
{
    const char             *name;
    int                     level;
    int                     cost;
    int                     weight;
    int                     wear_flags;
    int                     item_type;
    int                     base_v0;
    int                     base_v1;
    int                     base_v2;
    int                     base_v3;
    int                     base_v4;
    int                     base_v5;
    int                     base_v6;
};

struct tanner_table
{
    const char             *name;
    const char             *short_descr;
    int                     level;
    int                     cost;
    int                     weight;
    int                     wear_flags;
    int                     item_type;
    int                     base_v0;
    int                     base_v1;
    int                     base_v2;
    int                     base_v3;
    int                     base_v4;
    int                     base_v5;
    int                     base_v6;
};

struct hunter_table
{
    const char             *name;
    const char             *short_descr;
    const char             *long_descr;
    int                     tlevel;
    short                   race;
    short                   level;
};

struct smith_ores_type
{
    const char             *name;
    int                     cost_stone;
    int                     armor_mod;
    int                     workability;
    const char             *color;
};

const struct smith_ores_type smith_ores_table[] = {
    {"bronze ore", 10, 10, 25, "reddish-gold"},        /* 1 */
    {"iron ore", 25, 15, 22, "bright-gray"},           /* 2 */
    {"steel ore", 50, 25, 20, "silvery-gray"},         /* 3 */
    {"silver ore", 120, 8, 18, "bright"},              /* 4 */
    {"gold ore", 225, 5, 16, "gleaming"},              /* 5 */
    {"mithril ore", 300, 30, 25, "bright-light-green"}, /* 6 */
    {"adamantite ore", 400, 35, 27, "bright-white"},   /* 7 */
    {NULL, 0}
};

const struct baker_table fruit_types[] = {
    {"strawberries", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 3, 0, 0},    /* 1 */
    {"oranges", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0}, /* 2 */
    {"blueberries", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 4, 0, 0}, /* 3 */
    {"raspberries", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 6, 0, 0}, /* 4 */
    {"watermelons", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 9, 0, 0}, /* 5 */
    {"peaches", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0}, /* 6 */
    {"pears", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 9, 0, 0},   /* 7 */
    {"tomatoes", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0},    /* 8 */

    {"corn", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 14, 0, 0},   /* 1 */
    {"lettuce", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 11, 0, 0},    /* 2 */
    {"green beans", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 9, 0, 0}, /* 3 */
    {"peas", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 9, 0, 0},    /* 4 */
    {"carrots", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0}, /* 5 */
    {"onions", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 5, 0, 0},  /* 6 */
    {"radishes", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 3, 0, 0},    /* 7 */
    {"pickles", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 11, 0, 0},    /* 8 */

    {"cinnamon", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0},    /* 1 */
    {"sugar cane", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 5, 0, 0},  /* 2 */
    {"garlic", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 5, 0, 0},  /* 3 */
    {"wheat", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0},   /* 4 */
    {"fig", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 11, 0, 0},    /* 5 */
    {"ginger", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0},  /* 6 */
    {"milkweed", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 1, 0, 0},    /* 7 */
    {"pepper", 0, 1, 2, ITEM_TAKE, ITEM_RAW, 0, 0, 0, 0, 13, 0, 0}, /* 8 */
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const struct baker_table food_one[] = {
    {"snack", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0}, /* 1 */
    {"dessert", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},   /* 2 */
    {"pie", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},   /* 3 */
    {"cake", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 4 */
    {"salad", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0}, /* 5 */
    {"entree", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},    /* 6 */
    {"loaf", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 7 */
    {"muffin", 0, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},    /* 8 */

    {"soup", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 1 */
    {"stuffing", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 2 */
    {"cream", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0}, /* 3 */
    {"roast", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0}, /* 4 */
    {"stew", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 5 */
    {"tart", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 6 */
    {"pastry", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},    /* 7 */
    {"pudding", 5, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},   /* 8 */

    {"crisp", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},    /* 1 */
    {"patties", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 2 */
    {"skewers", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 3 */
    {"crumble", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 4 */
    {"spread", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},   /* 5 */
    {"strudel", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},  /* 6 */
    {"casserole", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0},    /* 7 */
    {"sandwich", 15, 1, 2, ITEM_TAKE, ITEM_FOOD, 0, 0, 0, 0, 0, 0}, /* 8 */
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const struct baker_table drink_one[] = {
    {"wooden-decanter", 0, 1, 1, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},  /* 1 */
    {"rabbitskin-flask", 0, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0}, /* 2 */
    {"wooden-goblet", 0, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 3 */
    {"wooden-tankard", 0, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 4 */
    {"wooden-chalice", 0, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 5 */
    {"wooden-vessell", 0, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 6 */
    {"wooden-mug", 0, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 7 */
    {"wooden-pitcher", 0, 1, 3, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 8 */

    {"bronze-decanter", 5, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},  /* 1 */
    {"stagskin-flask", 5, 1, 1, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 2 */
    {"bronze-goblet", 5, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 3 */
    {"bronze-tankard", 5, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 4 */
    {"bronze-chalice", 5, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 5 */
    {"bronze-vessell", 5, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 6 */
    {"bronze-mug", 5, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 7 */
    {"bronze-pitcher", 5, 1, 5, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 8 */

    {"iron-decanter", 15, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},   /* 1 */
    {"deerskin-flask", 15, 1, 1, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},  /* 2 */
    {"iron-goblet", 15, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0}, /* 3 */
    {"iron-tankard", 15, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 4 */
    {"iron-chalice", 15, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 5 */
    {"iron-vessell", 15, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 6 */
    {"iron-mug", 15, 1, 2, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 7 */
    {"iron-pitcher", 15, 1, 5, ITEM_TAKE, ITEM_DRINK_CON, 0, 0, 0, 0, 0, 0},    /* 8 */
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const struct smith_items_type tier_one[] = {
    {"knife", 0, 1, 2, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 2, 2, 0, 0},
    {"cock-spur", 0, 1, 2, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 2, 2, 0, 0},
    {"stiletto", 0, 1, 2, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 2, 2, 0, 0},
    {"dagger", 0, 1, 2, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 2, 2, 0, 0},
    {"keris", 0, 1, 2, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 2, 2, 0, 0},
    {"dirk", 0, 1, 2, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 2, 2, 0, 0},
    {"hand-axe", 0, 1, 5, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 1, 0, 3, 9, 0, 0},
    {"scale-tunic", 0, 1, 20, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"hauberk", 0, 1, 20, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"haubergeon", 0, 1, 20, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"chain-hauberk", 0, 1, 20, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"scale-hauberk", 0, 1, 20, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"scarab", 0, 1, 20, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"torque", 0, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"mask", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_FACE, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"girdle", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_WAIST, ITEM_ARMOR, 0, 1, 0, 0, 0, 0, 0},
    {"poinard", 5, 1, 4, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 2, 2, 0, 0},
    {"long-knife", 5, 1, 3, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 2, 2, 0, 0},
    {"shortsword", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},

    {"langsax", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"leaf-sword", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"gladius", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"cutlass", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"sabre", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"rapier", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"cinqueda", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"spatha", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"falchion", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"scimitar", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"nimcha", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"hanger", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"kastane", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"yatagan", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"longsword", 5, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 2, 0, 1, 1, 0, 0},
    {"chain-mail", 5, 1, 10, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 2, 0, 0, 0, 0, 0},
    {"scale-mail", 5, 1, 10, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 2, 0, 0, 0, 0, 0},
    {"brooch", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 2, 0, 0, 0, 0, 0},
    {"pendant", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 2, 0, 0, 0, 0, 0},
    {"wrist-band", 5, 1, 3, ITEM_TAKE | ITEM_WEAR_WRIST, ITEM_ARMOR, 0, 2, 0, 0, 0, 0, 0},

    {"backsword", 10, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 1, 0, 0},
    {"ringsword", 10, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 1, 0, 0},
    {"broadsword", 10, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 1, 0, 0},
    {"flamberge", 10, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 1, 0, 0},
    {"bastard-sword", 10, 1, 8, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 1, 0, 0},
    {"greatsword", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 0, 0, 0},
    {"war-sword", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 0, 0, 0},
    {"claymore", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 1, 0, 0, 0},
    {"hatchet", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 3, 9, 0, 0},
    {"recade", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 3, 9, 0, 0},
    {"francisca", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 3, 9, 0, 0},
    {"battle-axe", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 3, 8, 0, 0},
    {"broad-axe", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 3, 8, 0, 0},
    {"war-axe", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 3, 8, 0, 0},
    {"horsemans-mace", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 4, 5, 0, 0},
    {"war-hammer", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 4, 5, 0, 0},
    {"morning-star", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 4, 5, 0, 0},
    {"battle-hammer", 10, 1, 12, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 4, 4, 0, 0},
    {"footmans-mace", 10, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 4, 5, 0, 0},

    {"angon", 15, 1, 6, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 4, 5, 0, 0},
    {"assegai", 15, 1, 10, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 6, 10, 0, 0},
    {"spear", 15, 1, 10, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 6, 10, 0, 0},
    {"pilum", 15, 1, 10, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 6, 10, 0, 0},
    {"javelin", 15, 1, 10, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 6, 10, 0, 0},
    {"lance", 15, 1, 15, ITEM_TAKE | ITEM_WIELD, ITEM_WEAPON, 0, 3, 0, 6, 12, 0, 0},
    {"plate-mail", 15, 1, 25, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"field-plate", 15, 1, 30, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"full-plate", 15, 1, 35, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"finger-ring", 15, 1, 1, ITEM_TAKE | ITEM_WEAR_FINGER, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"finger-band", 15, 1, 1, ITEM_TAKE | ITEM_WEAR_FINGER, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"ring-hoop", 15, 1, 1, ITEM_TAKE | ITEM_WEAR_FINGER, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"ring-chain", 15, 1, 1, ITEM_TAKE | ITEM_WEAR_FINGER, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"ring-band", 15, 1, 1, ITEM_TAKE | ITEM_WEAR_FINGER, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"necklace", 15, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"medallion", 15, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"fibula", 15, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"neck-chain", 15, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"neck-pin", 15, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"amulet", 15, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},

    {"choker", 20, 1, 3, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"cufflink", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_WRIST, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"wrist-pin", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_WRIST, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"armlet", 20, 1, 8, ITEM_TAKE | ITEM_WEAR_ARMS, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"armband", 20, 1, 8, ITEM_TAKE | ITEM_WEAR_ARMS, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"earring", 20, 1, 1, ITEM_TAKE | ITEM_WEAR_EARS, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"ear-hoop", 20, 1, 1, ITEM_TAKE | ITEM_WEAR_EARS, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"ear-chain", 20, 1, 1, ITEM_TAKE | ITEM_WEAR_EARS, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"ear-pendant", 20, 1, 1, ITEM_TAKE | ITEM_WEAR_EARS, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"buckle", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_WAIST, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"tiara", 20, 1, 3, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"comb", 20, 1, 3, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"hairpin", 20, 1, 3, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"diadem", 20, 1, 3, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"spectacles", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_EYES, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"monacle", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_EYES, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"eyestrainers", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_EYES, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"eyeferns", 20, 1, 2, ITEM_TAKE | ITEM_WEAR_EYES, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const struct tanner_table tanned_one[] = {
    {"boots", "a pair of boots", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_FEET, ITEM_ARMOR, 0, 3, 0, 0, 0, 0,
     0},
    {"sabatons", "a pair of sabatons", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_FEET, ITEM_ARMOR, 0, 3, 0, 0,
     0, 0, 0},
    {"gloves", "a pair of gloves", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_HANDS, ITEM_ARMOR, 0, 3, 0, 0, 0,
     0, 0},
    {"sandals", "a pair of sandles", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_FEET, ITEM_ARMOR, 0, 3, 0, 0, 0,
     0, 0},
    {"gorget", "a gorget", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_NECK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"bracers", "a pair of bracers", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_WRIST, ITEM_ARMOR, 0, 3, 0, 0,
     0, 0, 0},
    {"gauntlets", "a pair of gauntlets", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_HANDS, ITEM_ARMOR, 0, 3, 0,
     0, 0, 0, 0},
    {"vambraces", "a pair of vambraces", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_ARMS, ITEM_ARMOR, 0, 3, 0,
     0, 0, 0, 0},
    {"sheath", "a sheath", 0, 1, 2, ITEM_TAKE | ITEM_WEAR_SHEATH, ITEM_SHEATH, 0, 3, 0, 0, 0, 0, 0},
    {"greaves", "a pair of greaves", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_LEGS, ITEM_ARMOR, 0, 3, 0, 0, 0,
     0, 0},
    {"war skirt", "a war skirt", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_WAIST, ITEM_ARMOR, 0, 3, 0, 0, 0, 0,
     0},
    {"belt", "a belt", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_WAIST, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"pauldrons", "a pair of pauldrons", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_ARMS, ITEM_ARMOR, 0, 3, 0,
     0, 0, 0, 0},
    {"spaulder", "a pair of spaulders", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_ARMS, ITEM_ARMOR, 0, 3, 0, 0,
     0, 0, 0},
    {"armet", "a armet", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"coif", "a coif", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"helm", "a helm", 5, 1, 2, ITEM_TAKE | ITEM_WEAR_HEAD, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"robe", "a robe", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_ABOUT, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"cloak", "a cloak", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_ABOUT, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"cape", "a cape", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_ABOUT, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"tunic", "a tunic", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"vest", "a vest", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"brigandine", "a brigandine", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_BODY, ITEM_ARMOR, 0, 3, 0, 0, 0,
     0, 0},
    {"cuirass", "a cuirass", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_BACK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0, 0},
    {"back hide", "a back hide", 15, 1, 2, ITEM_TAKE | ITEM_WEAR_BACK, ITEM_ARMOR, 0, 3, 0, 0, 0, 0,
     0},
    {NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const struct hunter_table hunted_one[] = {
    {"rabbit", "a small rabbit", "A small rabbit is here.", 0, 83, 1},  /* 1 */
    {"pheasant", "a small pheasant", "A small pheasant is here.", 0, 83, 1},    /* 2 */
    {"dove", "a small dove", "A small dove is here.", 0, 83, 1},    /* 3 */
    {"goose", "a small goose", "A small goose is here.", 0, 83, 1}, /* 4 */
    {"duck", "a small duck", "A small duck is here.", 0, 83, 1},    /* 5 */
    {"squirrel", "a small squirrel", "A small squirrel is here.", 0, 83, 1},    /* 6 */
    {"badger", "a small badger", "A small badger is here.", 0, 83, 1},  /* 7 */
    {"raccoon", "a small raccoon", "A small raccoon is here.", 0, 83, 1},   /* 8 */
    {"deer", "a white tailed deer", "A white tailed deer is here.", 5, 83, 1},  /* 1 */
    {"turkey", "a turkey", "A turkey is here.", 5, 83, 1},  /* 2 */
    {"wild boar", "a wild boar", "A wild boar is here.", 5, 83, 1}, /* 3 */
    {"antelope", "a antelope", "A antelope is here.", 5, 83, 1},    /* 4 */
    {"coyote", "a coyote", "A coyote is here.", 5, 83, 1},  /* 5 */
    {"bobcat", "a bobcat", "A bobcat is here.", 5, 83, 1},  /* 6 */
    {"wolf", "a wolf", "A wolf is here.", 5, 83, 1},   /* 7 */
    {"leopard", "a leopard", "A leopard is here.", 5, 83, 1},   /* 8 */
    {"bear", "a bear", "A bear is here.", 15, 83, 1},  /* 1 */
    {"moose", "a moose", "A moose is here.", 15, 83, 1},    /* 2 */
    {"lion", "a lion", "A lion is here.", 15, 83, 1},  /* 3 */
    {"unicorn", "a unicorn", "A unicorn is here.", 15, 83, 1},  /* 4 */
    {"griffon", "a griffon", "A griffon is here.", 15, 83, 1},  /* 5 */
    {"bugbear", "a bugbear", "A bugbear is here.", 15, 83, 1},  /* 6 */
    {"giant toad", "a giant toad", "A giant toad is here.", 15, 83, 1}, /* 7 */
    {"hydra", "a hydra", "A hydra is here.", 15, 83, 1},    /* 8 */
    {NULL, NULL, NULL, 0, 0, 0}
};
