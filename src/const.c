/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - MUD constants module                                                  *
 ***************************************************************************/
#include "h/mud.h"
#include "h/files.h"

/* undef these at EOF */
#define AM 95
#define AC 95
#define AT 85
#define AW 85
#define AV 95
#define AD 95
#define AR 90
#define AA 95

const char             *obj_sizes[] = {
    "magical", "tiny", "small", "average", "large",
    "enormous", "colossal", "gargantuan"
};

#if 0
const struct at_color_type at_color_table[AT_MAXCOLOR] = { {"plain", AT_GREY},
{"action", AT_GREY},
{"say", AT_LBLUE},
{"gossip", AT_LBLUE},
{"yell", AT_WHITE},
{"tell", AT_WHITE},
{"whisper", AT_WHITE},
{"hit", AT_WHITE},
{"hitme", AT_YELLOW},
{"immortal", AT_YELLOW},
{"hurt", AT_RED},
{"falling", AT_WHITE + AT_BLINK},
{"danger", AT_RED + AT_BLINK},
{"magic", AT_BLUE},
{"consider", AT_GREY},
{"report", AT_GREY},
{"poison", AT_GREEN},
{"social", AT_CYAN},
{"dying", AT_YELLOW},
{"dead", AT_RED},
{"skill", AT_GREEN},
{"carnage", AT_BLOOD},
{"damage", AT_WHITE},
{"flee", AT_YELLOW},
{"roomname", AT_WHITE},
{"roomdesc", AT_YELLOW},
{"object", AT_GREEN},
{"person", AT_PINK},
{"list", AT_BLUE},
{"bye", AT_GREEN},
{"gold", AT_YELLOW},
{"gtell", AT_BLUE},
{"note", AT_GREEN},
{"hungry", AT_ORANGE},
{"thirsty", AT_BLUE},
{"fire", AT_RED},
{"sober", AT_WHITE},
{"wearoff", AT_YELLOW},
{"exits", AT_WHITE},
{"score", AT_LBLUE},
{"reset", AT_DGREEN},
{"log", AT_PURPLE},
{"diemsg", AT_WHITE},
{"wartalk", AT_RED},
{"racetalk", AT_DGREEN},
{"ignore", AT_GREEN},
{"divider", AT_PLAIN},
{"morph", AT_GREY},
};
#endif

/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
    {-5, -4, 0, 0},                                    /* 0 */
    {-5, -4, 3, 1},                                    /* 1 */
    {-3, -2, 3, 2},
    {-3, -1, 10, 3},                                   /* 3 */
    {-2, -1, 25, 4},
    {-2, -1, 55, 5},                                   /* 5 */
    {-1, 0, 80, 6},
    {-1, 0, 90, 7},
    {0, 0, 100, 8},
    {0, 0, 100, 9},
    {0, 0, 115, 10},                                   /* 10 */
    {0, 0, 115, 11},
    {0, 0, 140, 12},
    {0, 0, 140, 13},                                   /* 13 */
    {0, 1, 170, 14},
    {1, 1, 170, 15},                                   /* 15 */
    {1, 2, 195, 16},
    {2, 3, 220, 22},
    {2, 4, 250, 25},                                   /* 18 */
    {3, 5, 400, 30},
    {3, 6, 500, 35},                                   /* 20 */
    {4, 7, 600, 40},
    {5, 7, 700, 45},
    {6, 8, 800, 50},
    {8, 10, 900, 55},
    {10, 12, 999, 60}                                  /* 25 */
};

const struct int_app_type int_app[26] = {
    {3},                                               /* 0 */
    {5},                                               /* 1 */
    {7},
    {8},                                               /* 3 */
    {9},
    {10},                                              /* 5 */
    {11},
    {12},
    {13},
    {15},
    {17},                                              /* 10 */
    {19},
    {22},
    {25},
    {28},
    {31},                                              /* 15 */
    {34},
    {37},
    {40},                                              /* 18 */
    {44},
    {49},                                              /* 20 */
    {55},
    {60},
    {70},
    {85},
    {99}                                               /* 25 */
};

const struct wis_app_type wis_app[26] = {
    {1},                                               /* 0 */
    {1},                                               /* 1 */
    {1},
    {1},                                               /* 3 */
    {1},
    {1},                                               /* 5 */
    {2},
    {2},
    {2},
    {3},
    {3},                                               /* 10 */
    {3},
    {3},
    {3},
    {3},
    {4},                                               /* 15 */
    {4},
    {5},
    {6},                                               /* 18 */
    {7},
    {8},                                               /* 20 */
    {8},
    {8},
    {8},
    {8},
    {8}                                                /* 25 */
};

const struct dex_app_type dex_app[26] = {
    {45},                                              /* 0 */
    {40},                                              /* 1 */
    {35},
    {30},
    {25},
    {20},                                              /* 5 */
    {10},
    {9},
    {8},
    {7},
    {6},                                               /* 10 */
    {5},
    {4},
    {2},
    {0},
    {-1},                                              /* 15 */
    {-1},
    {-2},
    {-3},
    {-4},
    {-5},                                              /* 20 */
    {-6},
    {-7},
    {-8},
    {-9},
    {-10}                                              /* 25 */
};

const struct con_app_type con_app[26] = {
    {-4, 20},                                          /* 0 */
    {-3, 25},                                          /* 1 */
    {-2, 30},
    {-2, 35},                                          /* 3 */
    {-1, 40},
    {-1, 45},                                          /* 5 */
    {-1, 50},
    {0, 55},
    {0, 60},
    {0, 65},
    {0, 70},                                           /* 10 */
    {0, 75},
    {0, 80},
    {0, 85},
    {0, 88},
    {1, 90},                                           /* 15 */
    {2, 95},
    {2, 97},
    {3, 99},                                           /* 18 */
    {3, 99},
    {4, 99},                                           /* 20 */
    {4, 99},
    {5, 99},
    {6, 99},
    {7, 99},
    {8, 99}                                            /* 25 */
};

const struct cha_app_type cha_app[26] = {
    {-60},                                             /* 0 */
    {-50},                                             /* 1 */
    {-50},
    {-40},
    {-30},
    {-20},                                             /* 5 */
    {-10},
    {-5},
    {-1},
    {0},
    {0},                                               /* 10 */
    {0},
    {0},
    {0},
    {1},
    {5},                                               /* 15 */
    {10},
    {20},
    {30},
    {40},
    {50},                                              /* 20 */
    {60},
    {70},
    {80},
    {90},
    {99}                                               /* 25 */
};

/* Have to fix this up - not exactly sure how it works (Scryn) */
const struct lck_app_type lck_app[26] = {
    {60},                                              /* 0 */
    {50},                                              /* 1 */
    {50},
    {40},
    {30},
    {20},                                              /* 5 */
    {10},
    {0},
    {0},
    {0},
    {0},                                               /* 10 */
    {0},
    {0},
    {0},
    {0},
    {-10},                                             /* 15 */
    {-15},
    {-20},
    {-30},
    {-40},
    {-50},                                             /* 20 */
    {-60},
    {-75},
    {-90},
    {-105},
    {-120}                                             /* 25 */
};

/*
 * Liquid properties.
 * Used in #OBJECT section of area file.
 */
const struct liq_type   liq_table[LIQ_MAX] = {
    {"water", "clear", {0, 1, 10}},                    /* 0 */
    {"beer", "amber", {3, 2, 5}},
    {"wine", "rose", {5, 2, 5}},
    {"ale", "brown", {2, 2, 5}},
    {"dark ale", "dark", {1, 2, 5}},

    {"whisky", "golden", {6, 1, 4}},                   /* 5 */
    {"lemonade", "pink", {0, 1, 8}},
    {"firebreather", "boiling", {10, 0, 0}},
    {"local specialty", "everclear", {3, 3, 3}},
    {"slime mold juice", "green", {0, 4, -8}},

    {"milk", "white", {0, 3, 6}},                      /* 10 */
    {"tea", "tan", {0, 1, 6}},
    {"coffee", "black", {0, 1, 6}},
    {"blood", "red", {0, 2, -1}},
    {"salt water", "clear", {0, 1, -2}},

    {"cola", "cherry", {0, 1, 5}},                     /* 15 */
    {"mead", "honey color", {4, 2, 5}},                /* 16 */
    {"grog", "thick brown", {3, 2, 5}},                /* 17 */
    {"black potion", "rainbow", {0, 0, 0}},
    {"vomit", "sickly green", {0, -1, -1}},

    {"holy water", "clear", {0, 1, 10}}                /* 20 */
};

/* Working in monks here. -Taon */

const char             *m_attack_table[8] = {
    "hit", "uppercut", "slap", "left hook",
    "right hook", "jab", "left knee",
    "right knee"
};

const char             *m_attack_table_plural[8] = {
    "hits", "uppercuts", "slaps", "left hooks",
    "right hooks", "jabs", "left knees",
    "right knees"
};

/* removed "pea" and added chop, spear, smash - Grimm */
/* Removed duplication in damage types - Samson 1-9-00 */
const char             *attack_table[DAM_MAX_TYPE] = {
    "hit", "slash", "stab", "hack", "crush", "lash", "pierce",
    "thrust"
};

const char             *attack_table_plural[DAM_MAX_TYPE] = {
    "hits", "slashes", "stabs", "hacks", "crushes", "lashes", "pierces",
    "thrusts"
};

const char             *weapon_skills[WEP_MAX] = {
    "2h Long Swords", "1h Long Blades", "1h Short Blades", "Whips",
    "2h Bludgeons", "1h Bludgeons", "Archery", "Blowguns", "2h Axes",
    "1h Axes", "Spear", "Staff", "Lance", "Flail", "Talon", "Polearm"
};

const char             *projectiles[PROJ_MAX] = {
    "Bolt", "Arrow", "Dart", "Stone"
};

const char             *s_blade_messages[24] = {
    "miss", "barely scratch", "scratch", "nick", "cut", "slash", "tear",
    "rip", "gash", "lacerate", "hack", "maul", "rend", "decimate",
    "_mangle_", "_devastate_", "_cleave_", "_butcher_", "DISEMBOWEL",
    "DISFIGURE", "GUT", "EVISCERATE", "* SLAUGHTER *",
    "&R*** ANNIHILATE ***&D"
};

const char             *p_blade_messages[24] = {
    "misses", "barely scratches", "scratches", "nicks", "slashes", "hits",
    "tears", "rips", "gashes", "lacerates", "hacks", "mauls", "rends",
    "decimates", "_mangles_", "_devastates_", "_cleaves_", "_butchers_",
    "DISEMBOWELS", "DISFIGURES", "GUTS", "EVISCERATES", "* SLAUGHTERS *",
    "&R*** ANNIHILATES ***&D"
};

const char             *s_blunt_messages[24] = {
    "miss", "barely scuff", "scuff", "pelt", "bruise", "strike", "thrash",
    "batter", "flog", "pummel", "smash", "maul", "bludgeon", "decimate",
    "_shatter_", "_devastate_", "_maim_", "_cripple_", "MUTILATE", "DISFIGURE",
    "MASSACRE", "PULVERIZE", "* OBLITERATE *", "&R*** ANNIHILATE ***&D"
};

const char             *p_blunt_messages[24] = {
    "misses", "barely scuffs", "scuffs", "pelts", "bruises", "strikes",
    "thrashes", "batters", "flogs", "pummels", "smashes", "mauls",
    "bludgeons", "decimates", "_shatters_", "_devastates_", "_maims_",
    "_cripples_", "MUTILATES", "DISFIGURES", "MASSACRES", "PULVERIZES",
    "* OBLITERATES *", "&R*** ANNIHILATES ***&D"
};

const char             *s_generic_messages[24] = {
    "miss", "brush", "scratch", "graze", "nick", "jolt", "wound",
    "injure", "hit", "jar", "thrash", "maul", "decimate", "_traumatize_",
    "_devastate_", "_maim_", "_demolish_", "MUTILATE", "MASSACRE",
    "PULVERIZE", "DESTROY", "* OBLITERATE *", "&R*** ANNIHILATE ***&D",
    "**** SMITE ****"
};

const char             *p_generic_messages[24] = {
    "misses", "brushes", "scratches", "grazes", "nicks", "jolts", "wounds",
    "injures", "hits", "jars", "thrashes", "mauls", "decimates", "_traumatizes_",
    "_devastates_", "_maims_", "_demolishes_", "MUTILATES", "MASSACRES",
    "PULVERIZES", "DESTROYS", "* OBLITERATES *", "&R*** ANNIHILATES ***&D",
    "**** SMITES ****"
};

const char            **const s_message_table[DAM_MAX_TYPE] = {
    s_generic_messages,                                /* hit */
    s_blade_messages,                                  /* slash */
    s_blade_messages,                                  /* stab */
    s_blade_messages,                                  /* hack */
    s_blunt_messages,                                  /* crush */
    s_blunt_messages,                                  /* lash */
    s_blade_messages,                                  /* pierce */
    s_blade_messages,                                  /* thrust */
};

const char            **const p_message_table[DAM_MAX_TYPE] = {
    p_generic_messages,                                /* hit */
    p_blade_messages,                                  /* slash */
    p_blade_messages,                                  /* stab */
    p_blade_messages,                                  /* hack */
    p_blunt_messages,                                  /* crush */
    p_blunt_messages,                                  /* lash */
    p_blade_messages,                                  /* pierce */
    p_blade_messages,                                  /* thrust */
};

/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n)  n
#define LI LEVEL_IMMORTAL

#undef AM
#undef AC
#undef AT
#undef AW
#undef AV
#undef AD
#undef AR
#undef AA

#undef LI
