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

/* Structure */
typedef struct lcnv_data LCNV_DATA;
typedef struct lang_data LANG_DATA;

/* Tongues / Languages structures */

struct lcnv_data
{
    LCNV_DATA              *next;
    LCNV_DATA              *prev;
    char                   *old;
    int                     olen;
    char                   *lnew;
    int                     nlen;
};

struct lang_data
{
    LANG_DATA              *next;
    LANG_DATA              *prev;
    char                   *name;
    LCNV_DATA              *first_precnv;
    LCNV_DATA              *last_precnv;
    char                   *alphabet;
    LCNV_DATA              *first_cnv;
    LCNV_DATA              *last_cnv;
};

/* Languages -- Altrag */
#define LANG_COMMON      BV00                          /* Human base language */
#define LANG_ELVEN       BV01                          /* Elven base language */
#define LANG_DWARVEN     BV02                          /* Dwarven base language */
#define LANG_PIXISH      BV03                          /* Pixie/Fairy base language */
#define LANG_OGRIAN      BV04                          /* Ogre base language */
#define LANG_ORCISH      BV05                          /* Orc base language */
#define LANG_TROLLESE    BV06                          /* Troll base language */
#define LANG_GOBLIC      BV07
#define LANG_HOBBIT      BV08
#define LANG_GNOMISH     BV09
#define LANG_INFERNAL    BV10
#define LANG_CELESTIAL   BV11
#define LANG_DRACONIC    BV12
#define LANG_UNCOMMON    BV13
#define LANG_DEMONIC     BV14
#define LANG_CENTAURIAN  BV15
#define LANG_SILENT      BV16
#define LANG_UNKNOWN        0                          /* Anything that doesnt fit a
                                                        * category */

#define VALID_LANGS  (LANG_COMMON  | LANG_ELVEN  | LANG_DWARVEN  \
   | LANG_PIXISH  | LANG_OGRIAN  | LANG_ORCISH  \
   | LANG_TROLLESE | LANG_GOBLIC  | LANG_HOBBIT  \
   | LANG_GNOMISH  | LANG_INFERNAL  \
   | LANG_CELESTIAL | LANG_DRACONIC  | LANG_UNCOMMON  \
   | LANG_DEMONIC | LANG_CENTAURIAN  | LANG_SILENT )

/* Language gsns. -- Altrag */
extern short            gsn_common;
extern short            gsn_elven;
extern short            gsn_dwarven;
extern short            gsn_pixish;
extern short            gsn_ogrian;
extern short            gsn_orcish;
extern short            gsn_trollese;
extern short            gsn_goblic;
extern short            gsn_hobbit;
extern short            gsn_draconic;
extern short            gsn_demonic;
extern short            gsn_centaurian;
extern short            gsn_celestial;
extern short            gsn_silent;

extern int const        lang_array[];
extern const char      *const lang_names[];

extern LANG_DATA       *first_lang;
extern LANG_DATA       *last_lang;
