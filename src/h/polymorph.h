/***************************************************************************
 * - Chronicles Copyright 2001-2003 by Brad Ensley (Orion Elder)           *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Polymorph module header                                               *
 ***************************************************************************/

typedef struct char_morph CHAR_MORPH;
typedef struct morph_data MORPH_DATA;

/*
 * Polymorph macros.
 */
extern MORPH_DATA      *morph_start;
extern MORPH_DATA      *morph_end;

#define MORPHPERS(ch, looker)  (can_see((looker), (ch)) ?                   \
                                (ch)->morph->morph->short_desc : "someone")

/*
 * Polymorph function definitions.
 */
void fwrite_morph_data  args( ( CHAR_DATA *ch, FILE * fp ) );
void fread_morph_data   args( ( CHAR_DATA *ch, FILE * fp ) );
void clear_char_morph   args( ( CHAR_MORPH * morph ) );
CHAR_MORPH             *make_char_morph args( ( MORPH_DATA * morph ) );
void free_char_morph    args( ( CHAR_MORPH * morph ) );
char                   *race_string args( ( int bitvector ) );
char                   *class_string args( ( int bitvector ) );
void setup_morph_vnum   args( ( void ) );
void unmorph_all        args( ( MORPH_DATA * morph ) );
MORPH_DATA             *get_morph args( ( char *arg ) );
MORPH_DATA             *get_morph_vnum args( ( int arg ) );
int do_morph_char       args( ( CHAR_DATA *ch, MORPH_DATA * morph ) );
MORPH_DATA             *find_morph args( ( CHAR_DATA *ch, char *target, bool is_cast ) );
void do_unmorph_char    args( ( CHAR_DATA *ch ) );
void send_morph_message args( ( CHAR_DATA *ch, MORPH_DATA * morph, bool is_morph ) );
bool can_morph          args( ( CHAR_DATA *ch, MORPH_DATA * morph, bool is_cast ) );
void do_morph           args( ( CHAR_DATA *ch, MORPH_DATA * morph ) );
void do_unmorph         args( ( CHAR_DATA *ch ) );
void save_morphs        args( ( void ) );
void fwrite_morph       args( ( FILE * fp, MORPH_DATA * morph ) );
void load_morphs        args( ( void ) );
MORPH_DATA             *fread_morph args( ( FILE * fp ) );
void free_morph         args( ( MORPH_DATA * morph ) );
void morph_defaults     args( ( MORPH_DATA * morph ) );
void sort_morphs        args( ( void ) );

/*
 * Definitions for use with the polymorph code.
 */
#define ONLY_PKILL      1
#define ONLY_PEACEFULL  2

struct char_morph
{
    MORPH_DATA             *morph;
    EXT_BV                  affected_by;               /* New affected_by added */
    EXT_BV                  no_affected_by;            /* Prevents affects from being
                                                        * added */
    EXT_BV                  affected_by2;              /* New affected_by added */
    EXT_BV                  no_affected_by2;           /* Prevents affects from being
                                                        * added */

    int                     no_immune;                 /* Prevents Immunities */
    int                     no_resistant;              /* Prevents resistances */
    int                     no_suscept;                /* Prevents Susceptibilities */
    int                     immune;                    /* Immunities added */
    int                     resistant;                 /* Resistances added */
    int                     suscept;                   /* Suscepts added */
    int                     timer;                     /* How much time is left */
    short                   ac;
    short                   blood;
    short                   cha;
    short                   con;
    short                   damroll;
    short                   dex;
    short                   dodge;
    short                   hit;
    short                   hitroll;
    short                   inte;
    short                   lck;
    short                   mana;
    short                   move;
    short                   parry;
    short                   saving_breath;
    short                   saving_para_petri;
    short                   saving_poison_death;
    short                   saving_spell_staff;
    short                   saving_wand;
    short                   str;
    short                   tumble;
    short                   wis;
};

struct morph_data
{
    MORPH_DATA             *next;                      /* Next morph file */
    MORPH_DATA             *prev;                      /* Previous morph file */
    char                   *blood;                     /* Blood added vamps only */
    char                   *damroll;
    char                   *description;
    char                   *help;                      /* What player sees for info on
                                                        * morph */
    char                   *hit;                       /* Hitpoints added */
    char                   *hitroll;
    char                   *key_words;                 /* Keywords added to your name */
    char                   *long_desc;                 /* New long_desc for player */
    char                   *mana;                      /* Mana added not for vamps */
    char                   *morph_other;               /* What others see when you morph */
    char                   *morph_self;                /* What you see when you morph */
    char                   *move;                      /* Move added */
    char                   *name;                      /* Name used to polymorph into
                                                        * this */
    char                   *short_desc;                /* New short desc for player */
    char                   *no_skills;                 /* Prevented Skills */
    char                   *skills;
    char                   *unmorph_other;             /* What others see when you
                                                        * unmorph */
    char                   *unmorph_self;              /* What you see when you unmorph */
    EXT_BV                  affected_by;               /* New affected_by added */
    EXT_BV                  affected_by2;              /* New affected_by added */
    int                     Class;                     /* Classes not allowed to use this 
                                                        */
    int                     defpos;                    /* Default position */
    EXT_BV                  no_affected_by;            /* Prevents affects from being
                                                        * added */
    EXT_BV                  no_affected_by2;           /* Prevents affects from being
                                                        * added */
    int                     no_immune;                 /* Prevents Immunities */
    int                     no_resistant;              /* Prevents resistances */
    int                     no_suscept;                /* Prevents Susceptibilities */
    int                     immune;                    /* Immunities added */
    int                     resistant;                 /* Resistances added */
    int                     suscept;                   /* Suscepts added */
    int                     obj[3];                    /* Object needed to morph you */
    int                     race;                      /* Races not allowed to use this */
    int                     timer;                     /* Timer for how long it lasts */
    int                     used;                      /* The amount the morph has been
                                                        * used */
    int                     vnum;                      /* Unique identifier */
    short                   ac;
    short                   bloodused;                 /* Blood morph requires of
                                                        * bloodsuckers */
    short                   cha;                       /* Amount Cha gained/Lost */
    short                   con;                       /* Amount of Con gained/Lost */
    short                   dayfrom;                   /* Starting Day you can morph into 
                                                        * this */
    short                   dayto;                     /* Ending Day you can morph into
                                                        * this */
    short                   dex;                       /* Amount of dex added */
    short                   dodge;                     /* Percent of dodge added IE 1 =
                                                        * 1% */
    short                   hpused;                    /* Amount of hps used to morph */
    short                   inte;                      /* Amount of Int gained/lost */
    short                   lck;                       /* Amount of Lck gained/lost */
    short                   level;                     /* Minimum level to use this morph 
                                                        */
    short                   manaused;                  /* Amount of mana used to morph */
    short                   moveused;                  /* Amount of move used to morph */
    short                   parry;                     /* Percent of parry added IE 1 =
                                                        * 1% */
    short                   pkill;                     /* Pkill Only, Peacefull Only or
                                                        * Both */
    short                   saving_breath;             /* Below are saving adjusted */
    short                   saving_para_petri;
    short                   saving_poison_death;
    short                   saving_spell_staff;
    short                   saving_wand;
    short                   sex;                       /* The sex that can morph into
                                                        * this */
    short                   str;                       /* Amount of str gained lost */
    short                   timefrom;                  /* Hour starting you can morph */
    short                   timeto;                    /* Hour ending that you can morph */
    short                   tumble;                    /* Percent of tumble added IE 1 =
                                                        * 1% */
    short                   wis;                       /* Amount of Wis gained/lost */
    bool                    no_cast;                   /* Can you cast a spell to morph */
    bool                    objuse[3];                 /* Objects needed to morph */
};
