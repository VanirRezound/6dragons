/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Database management module                                            *
 ***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifndef WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include "h/mud.h"
#include "h/ban.h"
#include "h/copyover.h"
#include "h/files.h"
#include "h/shops.h"
#include "h/auction.h"
#include "h/languages.h"
#include "h/currency.h"
#include "h/polymorph.h"
#include "h/key.h"
#include "h/news.h"
#include "h/mssp.h"
#include "h/hint.h"
#include "h/ftag.h"
#include "h/arena.h"

void                    get_curr_players( void );

void                    save_clan_storeroom( CHAR_DATA *ch );
void                    load_quest_list(  );
extern bool             challenge;
void                    free_all_pkills( CHAR_DATA *ch );
extern int              _filbuf( FILE * );
static OBJ_DATA        *rgObjNest[MAX_NEST];

extern CHAR_DATA       *greendragon;
extern bool             doubleexp;
extern time_t           starttimedoubleexp;
extern time_t           starthappyhour;
void                    load_boards( void );
void                    load_global_boards( void );
void load_news          args( ( void ) );

int                     area_version;
void                    load_landmarks( void );
void                    load_htowns(  );
void                    load_cities(  );
void                    load_councils(  );
void                    load_clans(  );
void                    load_orders(  );
void                    init_supermob(  );
void                    load_slays(  );
void                    load_mudchannels(  );
void                    load_storages( void );
void                    load_storage( char *filename );
void loadBugs           args( ( void ) );
void loadIdeas          args( ( void ) );
void loadTypos          args( ( void ) );

/* Globals. */
void                    to_channel( const char *argument, const char *xchannel, int level );
STAFFENT               *first_staff;
STAFFENT               *last_staff;

short                   min_archery_chart( int level );
short                   max_archery_chart( int level );
short                   set_min_armor( int level );
short                   set_max_armor( int level );

time_t                  last_restore_all_time = 0;
time_t                  last_heaven_all_time = 0;

HELP_DATA              *first_help;
HELP_DATA              *last_help;

SHOP_DATA              *first_shop;
SHOP_DATA              *last_shop;

REPAIR_DATA            *first_repair;
REPAIR_DATA            *last_repair;

TELEPORT_DATA          *first_teleport;
TELEPORT_DATA          *last_teleport;

OBJ_DATA               *extracted_obj_queue;
EXTRACT_CHAR_DATA      *extracted_char_queue;

CHAR_DATA              *first_char;
CHAR_DATA              *last_char;
int                     FilesOpen;
FILE_DATA              *first_filedata;
FILE_DATA              *last_filedata;
char                   *help_greeting;
char                    log_buf[2 * MSL];

OBJ_DATA               *first_object;
OBJ_DATA               *last_object;
TIME_INFO_DATA          time_info;

int                     cur_qobjs;
int                     cur_qchars;
int                     nummobsloaded;
int                     numobjsloaded;
int                     physicalobjects;
int                     last_pkroom;
time_t                  mud_start_time;

AUCTION_DATA           *auction;                       /* auctions */

short                   gsn_body_drop;
short                   gsn_cannibalize;
short                   gsn_marble_skin;
short                   gsn_granite_skin;
short                   gsn_break;
short                   gsn_brain_boil;
short                   gsn_death_embrace;
short                   gsn_heart_grab;
short                   gsn_hallowed_blow;
short                   gsn_mitigate;
short                   gsn_glacial_armor;
short                   gsn_behead;
short                   gsn_remember;
short                   gsn_dehydrate;
short                   gsn_dye;
short                   gsn_flint_fire;
short                   gsn_brittle_bone;
short                   gsn_festering_wound;
short                   gsn_druidic_hymn;
short                   gsn_shriek;
short                   gsn_shrieking_note;
short                   gsn_harmonic_melody;
short                   gsn_human_form;
short                   gsn_evac_crescendo;
short                   gsn_thunderous_hymn;
short                   gsn_stirring_ballad;
short                   gsn_ottos_dance;
short                   gsn_unholy_melody;
short                   gsn_sound_waves;
short                   gsn_rousing_tune;
short                   gsn_silence;
short                   gsn_telepathy;
short                   gsn_heavens_blessing;
short                   gsn_shapeshift;
short                   gsn_shield;
short                   gsn_higher_magic;
short                   gsn_sanctuary;
short                   gsn_wings;
short                   gsn_demonic_sight;
short                   gsn_martial_arts;              // Taon
short                   gsn_mine;
short                   gsn_biofeedback;               // Taon
short                   gsn_iron_skin;                 // Taon
short                   gsn_impale;
short                   gsn_sustain_self;              // Taon
short                   gsn_root;                      // Taon
short                   gsn_untangle;                  // Taon
short                   gsn_daze;                      // Taon
short                   gsn_sidestep;                  // Taon
short                   gsn_dominate;                  // Taon
short                   gsn_fury;                      // Taon
short                   gsn_ensnare;                   // Taon
short                   gsn_layhands;                  // Taon
short                   gsn_endurance;                 // Taon
short                   gsn_boost;                     // Taon
short                   gsn_prayer;                    // Taon
short                   gsn_ritual;                    // Taon
short                   gsn_acidic_touch;              // Taon
short                   gsn_desecrate;                 // Muyahahahah!
short                   gsn_focus;                     // Taon
short                   gsn_defensive_posturing;       // Taon
short                   gsn_recoil;                    // Taon
short                   gsn_keen_eye;                  // Taon
short                   gsn_faith;
short                   gsn_know_enemy;
short                   gsn_flamebreath;               // Taon
short                   gsn_energy_containment;        // Taon
short                   gsn_rotten_gut;                // Aurin

short                   gsn_cone;
short                   gsn_defend;
short                   gsn_assassinate;
short                   gsn_eldritch_bolt;
short                   gsn_breath;
short                   gsn_lick;
short                   gsn_tears;
short                   gsn_taunt;
short                   gsn_mortify;
short                   gsn_assault;
short                   gsn_enrage;
short                   gsn_pawn;
short                   gsn_wailing;

/* weaponry */
/* changed to reflect new weapon types - Grimm */
short                   gsn_pugilism;
short                   gsn_2h_long_blades;
short                   gsn_1h_long_blades;
short                   gsn_1h_short_blades;
short                   gsn_whips;
short                   gsn_2h_bludgeons;
short                   gsn_1h_bludgeons;
short                   gsn_blowguns;
short                   gsn_polearms;
short                   gsn_2h_axes;
short                   gsn_1h_axes;
short                   gsn_spears;
short                   gsn_staves;
short                   gsn_archery;
short                   gsn_restring;
short                   gsn_lances;
short                   gsn_flails;
short                   gsn_talons;

/* thief */
short                   gsn_detrap;
short                   gsn_backstab;
short                   gsn_arteries;
short                   gsn_circle;
short                   gsn_phase;                     // Taon
short                   gsn_displacement;              // Taon
short                   gsn_dodge;
short                   gsn_hide;
short                   gsn_smuggle;
short                   gsn_peek;
short                   gsn_pick_lock;
short                   gsn_sneak;
short                   gsn_steal;
short                   gsn_gouge;
short                   gsn_poison_weapon;
short                   gsn_ball_of_fire;
short                   gsn_blizzard;
short                   gsn_inferno;

/* brawler */
short                   gsn_feign_death;

/* thief & warrior */
short                   gsn_disarm;
short                   gsn_enhanced_damage;
short                   gsn_grendals_stance;
short                   gsn_masters_eye;               // Taon
short                   gsn_kick;
short                   gsn_quivering_palm;
short                   gsn_tackle;
short                   gsn_backhand;
short                   gsn_righteous_blow;
short                   gsn_maim;
short                   gsn_parry;
short                   gsn_second_attack;
short                   gsn_third_attack;
short                   gsn_fourth_attack;
short                   gsn_fifth_attack;
short                   gsn_sixth_attack;
short                   gsn_seventh_attack;
short                   gsn_eighth_attack;
short                   gsn_dual_wield;
short                   gsn_punch;
short                   gsn_bash;
short                   gsn_stun;
short                   gsn_bashdoor;
short                   gsn_grip;
short                   gsn_berserk;
short                   gsn_hitall;
short                   gsn_tumble;
short                   gsn_rapid_healing;             // Taon
short                   gsn_decree_decay;              // Vladaar
short                   gsn_combat_mind;               // Taon
short                   gsn_find_weakness;             // Taon
short                   gsn_battle_trance;             // Taon
short                   gsn_counterstrike;             // Taon
short                   gsn_blade_master;              // Taon
short                   gsn_heavy_hands;

/* vampire */
short                   gsn_feed;
short                   gsn_bloodlet;
short                   gsn_broach;
short                   gsn_mistwalk;

/* crafts */
short                   gsn_tan;
short                   gsn_forge;
short                   gsn_hunt;
short                   gsn_gather;
short                   gsn_bake;
short                   gsn_mix;
short                   gsn_crafted_food;
short                   gsn_crafted_drink;

/* other */
short                   gsn_aid;
short                   gsn_track;
short                   gsn_smell;
short                   gsn_search;
short                   gsn_earthspeak;
short                   gsn_mount;
short                   gsn_snatch;
short                   gsn_shelter;
short                   gsn_gauge;
short                   gsn_burrow;
short                   gsn_vampiric_strength;
short                   gsn_surreal_speed;
short                   gsn_shrink;
short                   gsn_passage;
short                   gsn_conjure_elemental;
short                   gsn_animal_companion;
short                   gsn_lesser_skeleton;
short                   gsn_extract_skeleton;
short                   gsn_judge;
short                   gsn_judo;
short                   gsn_paralyze;
short                   gsn_bone;
short                   gsn_angelfire;
short                   gsn_unholy_sphere;
short                   gsn_charge;
short                   gsn_meditate;
short                   gsn_trance;
short                   gsn_pluck;
short                   gsn_drop;
short                   gsn_rage;
short                   gsn_fly_home;
short                   gsn_draw_mana;                 // Taon
short                   gsn_siphon_strength;           // Vladaar
short                   gsn_harm_touch;                // Vladaar
short                   gsn_devour;
short                   gsn_spike;
short                   gsn_gut;
short                   gsn_hellfire;                  // Taon
short                   gsn_deathroll;                 // Taon
short                   gsn_ballistic;                 // Taon
short                   gsn_flaming_shield;            // Taon
short                   gsn_tangle;
short                   gsn_entangle;                  // Taon
short                   gsn_submerged;                 // Taon
short                   gsn_flaming_whip;              // Taon
short                   gsn_thaitin;
short                   gsn_truesight;
short                   gsn_nosight;
short                   gsn_tail_swipe;
short                   gsn_gust_of_wind;
short                   gsn_stomp;
short                   gsn_bite;
short                   gsn_claw;
short                   gsn_sting;
short                   gsn_scribe;
short                   gsn_brew;
short                   gsn_climb;
short                   gsn_cook;
short                   gsn_scan;
short                   gsn_slice;

/* BARD - Volk */
short                   gsn_vocals;
short                   gsn_play;
short                   gsn_woodwinds;
short                   gsn_strings;
short                   gsn_brass;
short                   gsn_drums;

/* Brain guy skills */
short                   gsn_choke;
short                   gsn_thicken_skin;
short                   gsn_mental_assault;
short                   gsn_kinetic_barrier;
short                   gsn_leech;
short                   gsn_dream_walk;
short                   gsn_psionic_blast;
short                   gsn_graft_weapon;
short                   gsn_chameleon;
short                   gsn_heal;
short                   gsn_healing_thoughts;
short                   gsn_object_reading;
short                   gsn_lore;
short                   gsn_whip_of_murazor;
short                   gsn_astral_body;
short                   gsn_sustenance;
short                   gsn_fear;
short                   gsn_drowsy;
short                   gsn_torture_mind;

/* spells */
short                   gsn_aqua_breath;
short                   gsn_blindness;
short                   gsn_charm_person;
short                   gsn_curse;
short                   gsn_invis;
short                   gsn_spectral_ward;
short                   gsn_spirits_ward;
short                   gsn_mass_invis;
short                   gsn_poison;
short                   gsn_sleep;
short                   gsn_possess;
short                   gsn_fireball;

/* languages */
short                   gsn_common;
short                   gsn_elven;
short                   gsn_dwarven;
short                   gsn_pixish;
short                   gsn_ogrian;
short                   gsn_orcish;
short                   gsn_trollese;
short                   gsn_goblic;
short                   gsn_hobbit;
short                   gsn_draconic;
short                   gsn_demonic;
short                   gsn_centaurian;
short                   gsn_silent;

/* for searching */
short                   gsn_first_spell;
short                   gsn_first_skill;
short                   gsn_first_weapon;
short                   gsn_first_tongue;
short                   gsn_first_trade;
short                   gsn_first_song;
short                   gsn_top_sn;

/* Volk's new skills */
short                   gsn_swim;

/* sharpen skill --Cronel */
short                   gsn_sharpen;

/* PRIESTS */
short                   gsn_anoint;

/* Locals. */
MOB_INDEX_DATA         *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA         *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

AREA_DATA              *first_full_area;
AREA_DATA              *last_full_area;
AREA_DATA              *first_area;
AREA_DATA              *last_area;
AREA_DATA              *first_area_name;               /* Used for alphanum. sort */
AREA_DATA              *last_area_name;
AREA_DATA              *first_build;
AREA_DATA              *last_build;
AREA_DATA              *first_asort;
AREA_DATA              *last_asort;
AREA_DATA              *first_bsort;
AREA_DATA              *last_bsort;

SYSTEM_DATA             sysdata;

int                     top_affect;
int                     top_area;
int                     top_ed;
int                     top_exit;
int                     top_help;
int                     top_mob_index;
int                     top_obj_index;
int                     top_reset;
int                     top_room;
int                     top_shop;
int                     top_repair;

/* Semi-locals. */
bool                    fBootDb;
FILE                   *fpArea;
char                    strArea[MIL];

/* Local booting procedures. */
void init_mm            args( ( void ) );

void boot_log           args( ( const char *str, ... ) );
void load_area          args( ( FILE * fp ) );
void load_author        args( ( AREA_DATA *tarea, FILE * fp ) );
void load_derivatives   args( ( AREA_DATA *tarea, FILE * fp ) );
void load_homeland      args( ( AREA_DATA *tarea, FILE * fp ) );
void load_desc          args( ( AREA_DATA *tarea, FILE * fp ) );
void load_resetmsg      args( ( AREA_DATA *tarea, FILE * fp ) );    /* Rennard */
void load_clanname      args( ( AREA_DATA *tarea, FILE * fp ) );
void load_flags         args( ( AREA_DATA *tarea, FILE * fp ) );
void load_helps         args( ( AREA_DATA *tarea, FILE * fp ) );
void load_mobiles       args( ( AREA_DATA *tarea, FILE * fp ) );
void load_objects       args( ( AREA_DATA *tarea, FILE * fp ) );
void load_resets        args( ( AREA_DATA *tarea, FILE * fp ) );
void load_rooms         args( ( AREA_DATA *tarea, FILE * fp ) );
void load_shops         args( ( AREA_DATA *tarea, FILE * fp ) );
void load_repairs       args( ( AREA_DATA *tarea, FILE * fp ) );
void load_specials      args( ( AREA_DATA *tarea, FILE * fp ) );
void load_ranges        args( ( AREA_DATA *tarea, FILE * fp ) );
void load_climate       args( ( AREA_DATA *tarea, FILE * fp ) );    /* FB */
void load_influence     args( ( AREA_DATA *tarea, FILE * fp ) );
void load_neighbor      args( ( AREA_DATA *tarea, FILE * fp ) );
void load_buildlist     args( ( void ) );
bool load_systemdata    args( ( SYSTEM_DATA * sys ) );
void                    save_sysdata( SYSTEM_DATA sys );
void load_version       args( ( AREA_DATA *tarea, FILE * fp ) );
void                    load_watchlist( void );
void load_reserved      args( ( void ) );

void initialize_economy args( ( void ) );
static void load_higheconomy args( ( AREA_DATA *tarea, FILE * fp ) );
static void load_loweconomy args( ( AREA_DATA *tarea, FILE * fp ) );
void load_currency      args( ( void ) );
static void load_economy args( ( AREA_DATA *tarea, FILE * fp ) );
static void load_areacurr args( ( AREA_DATA *tarea, FILE * fp ) );

void fix_exits          args( ( void ) );
void sort_reserved      args( ( RESERVE_DATA * pRes ) );
void load_auth_list     args( ( void ) );              /* New Auth Code */
void                    build_wizinfo( bool bootup );
void                    load_hint( void );

/* External booting function */
void                    load_corpses( void );
void load_scores        args( ( void ) );
void                    renumber_put_resets( ROOM_INDEX_DATA *room );
void                    wipe_resets( ROOM_INDEX_DATA *room );

/* MUDprogram locals */
int                     mprog_name_to_type( char *name );
MPROG_DATA             *mprog_file_read( char *f, MPROG_DATA * mprg, MOB_INDEX_DATA *pMobIndex );
MPROG_DATA             *oprog_file_read( char *f, MPROG_DATA * mprg, OBJ_INDEX_DATA *pObjIndex );
MPROG_DATA             *rprog_file_read( char *f, MPROG_DATA * mprg, ROOM_INDEX_DATA *pRoomIndex );
void                    mprog_read_programs( FILE * fp, MOB_INDEX_DATA *pMobIndex );
void                    oprog_read_programs( FILE * fp, OBJ_INDEX_DATA *pObjIndex );
void                    rprog_read_programs( FILE * fp, ROOM_INDEX_DATA *pRoomIndex );

void shutdown_mud( const char *reason )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( SHUTDOWN_FILE, "a" ) ) != NULL ) {
        fprintf( fp, "%s\n", reason );
        FileClose( fp );
    }
}

/* Big mama top level function. */
void boot_db( bool fCopyOver )
{
    short                   wear,
                            x;

    fpArea = NULL;
    unlink( BOOTLOG_FILE );
    boot_log( "---------------------[ Boot Log ]--------------------" );

    log_string( "Loading commands" );
    load_commands(  );
    mud_start_time = current_time;
    load_mudchannels(  );

    log_string( "Loading sysdata configuration..." );
    /*
     * default values 
     */
    sysdata.read_all_mail = LEVEL_AJ_LT;
    sysdata.read_mail_free = LEVEL_IMMORTAL;
    sysdata.write_mail_free = LEVEL_IMMORTAL;
    sysdata.take_others_mail = LEVEL_AJ_LT;
    sysdata.muse_level = LEVEL_AJ_LT;
    sysdata.think_level = LEVEL_AJ_MAJOR;
    sysdata.build_level = LEVEL_AJ_CPL;
    sysdata.log_level = LEVEL_LOG;
    sysdata.level_modify_proto = LEVEL_AJ_SGT;
    sysdata.level_override_private = LEVEL_AJ_LT;
    sysdata.level_mset_player = LEVEL_AJ_SGT;
    sysdata.stun_plr_vs_plr = 65;
    sysdata.stun_regular = 15;
    sysdata.gouge_nontank = 0;
    sysdata.gouge_plr_vs_plr = 0;
    sysdata.bash_nontank = 0;
    sysdata.bash_plr_vs_plr = 0;
    sysdata.dodge_mod = 2;
    sysdata.parry_mod = 2;
    sysdata.tumble_mod = 4;
    sysdata.phase_mod = 4;
    sysdata.displacement_mod = 3;
    sysdata.dam_plr_vs_plr = 100;
    sysdata.dam_plr_vs_mob = 100;
    sysdata.dam_mob_vs_plr = 100;
    sysdata.dam_mob_vs_mob = 100;
    sysdata.level_getobjnotake = LEVEL_AJ_SGT;
    sysdata.save_frequency = 20;                       /* minutes */
    sysdata.bestow_dif = 5;
    sysdata.check_imm_host = 1;
    sysdata.morph_opt = 1;
    sysdata.save_pets = 0;
    sysdata.pk_loot = 1;
    sysdata.reqwho_arg = 1;
    sysdata.wiz_lock = 0;
    sysdata.secpertick = 70;
    sysdata.pulsepersec = 4;
    sysdata.hoursperday = 24;
    sysdata.daysperweek = 7;
    sysdata.dayspermonth = 31;
    sysdata.monthsperyear = 17;
    sysdata.autoboot_hour = 6;
    sysdata.autoboot_minute = 0;
    sysdata.autoboot_period = 7;
    xSET_BIT( sysdata.save_flags, SV_DEATH );
    xSET_BIT( sysdata.save_flags, SV_PASSCHG );
    xSET_BIT( sysdata.save_flags, SV_AUTO );
    xSET_BIT( sysdata.save_flags, SV_PUT );
    xSET_BIT( sysdata.save_flags, SV_DROP );
    xSET_BIT( sysdata.save_flags, SV_GIVE );
    xSET_BIT( sysdata.save_flags, SV_AUCTION );
    xSET_BIT( sysdata.save_flags, SV_ZAPDROP );
    xSET_BIT( sysdata.save_flags, SV_IDLE );

    if ( !load_systemdata( &sysdata ) ) {
        log_string( "Not found.  Creating new configuration." );
        sysdata.alltimemax = 0;
        sysdata.mud_name = STRALLOC( "(Name not set)" );
        update_timers(  );
        update_calendar(  );
        save_sysdata( sysdata );
    }

    log_string( "Loading socials" );
    load_socials(  );

    log_string( "Loading skill table" );
    load_skill_table(  );
    sort_skill_table(  );
    remap_slot_numbers(  );                            /* must be after the sort */

    gsn_first_spell = 0;
    gsn_first_skill = 0;
    gsn_first_weapon = 0;
    gsn_first_tongue = 0;
    gsn_first_trade = 0;
    gsn_first_song = 0;
    gsn_top_sn = top_sn;

    for ( x = 0; x < top_sn; x++ ) {
        if ( !gsn_first_spell && skill_table[x]->type == SKILL_SPELL )
            gsn_first_spell = x;
        else if ( !gsn_first_skill && skill_table[x]->type == SKILL_SKILL )
            gsn_first_skill = x;
        else if ( !gsn_first_weapon && skill_table[x]->type == SKILL_WEAPON )
            gsn_first_weapon = x;
        else if ( !gsn_first_tongue && skill_table[x]->type == SKILL_TONGUE )
            gsn_first_tongue = x;
        else if ( !gsn_first_song && skill_table[x]->type == SKILL_SONG )
            gsn_first_song = x;
    }

    log_string( "Loading classes" );
    load_classes(  );

    log_string( "Loading races" );
    load_races(  );

    log_string( "Loading herb table" );
    load_herb_table(  );

    log_string( "Loading tongues" );
    load_tongues(  );

    log_string( "Making 6 Dragons stafflist" );
    make_stafflist(  );

    log_string( "Loading MSSP Data..." );
    load_mssp_data(  );

    log_string( "Building wizinfo" );
    build_wizinfo( TRUE );

    loadBugs(  );
    loadIdeas(  );
    loadTypos(  );

    fBootDb = TRUE;

    nummobsloaded = 0;
    numobjsloaded = 0;
    physicalobjects = 0;
    sysdata.maxplayers = 0;
    first_object = NULL;
    last_object = NULL;
    first_char = NULL;
    last_char = NULL;
    first_full_area = NULL;
    last_full_area = NULL;
    first_area = NULL;
    first_area_name = NULL;                            /* Used for alphanum. sort */
    last_area_name = NULL;
    last_area = NULL;
    first_build = NULL;
    last_area = NULL;
    first_shop = NULL;
    last_shop = NULL;
    first_repair = NULL;
    last_repair = NULL;
    first_teleport = NULL;
    last_teleport = NULL;
    first_asort = NULL;
    last_asort = NULL;
    extracted_obj_queue = NULL;
    extracted_char_queue = NULL;
    first_filedata = NULL;
    last_filedata = NULL;
    FilesOpen = 0;
    cur_qobjs = 0;
    cur_qchars = 0;
    cur_char = NULL;
    cur_obj = 0;
    cur_obj_serial = 0;
    cur_char_died = FALSE;
    cur_obj_extracted = FALSE;
    cur_room = NULL;
    quitting_char = NULL;
    loading_char = NULL;
    saving_char = NULL;
    last_pkroom = 1;
    immortal_host_start = NULL;
    immortal_host_end = NULL;

    first_ban_class = NULL;
    last_ban_class = NULL;
    first_ban_race = NULL;
    last_ban_race = NULL;
    first_ban = NULL;
    last_ban = NULL;
    first_news = NULL;
    last_news = NULL;

    CREATE( auction, AUCTION_DATA, 1 );
    auction->item = NULL;
    auction->hist_timer = 0;
    for ( x = 0; x < AUCTION_MEM; x++ )
        auction->history[x] = NULL;

    for ( wear = 0; wear < MAX_WEAR; wear++ )
        for ( x = 0; x < MAX_LAYERS; x++ )
            save_equipment[wear][x] = NULL;

    /*
     * Init random number generator.
     */
    log_string( "Initializing random number generator" );
    init_mm(  );

/*
    * Set time and weather.
    */
    {
        long                    lhour,
                                lday,
                                lmonth;

        log_string( "Setting time and weather." );

        if ( !load_timedata(  ) ) {                    /* Loads time from stored file if
                                                        * TRUE - Samson 1-21-99 */
            boot_log( "Resetting mud time based on current system time." );
            lhour = ( current_time - 650336715 ) / ( sysdata.pulsetick / sysdata.pulsepersec );
            time_info.hour = lhour % sysdata.hoursperday;
            lday = lhour / sysdata.hoursperday;
            time_info.day = lday % sysdata.dayspermonth;
            lmonth = lday / sysdata.dayspermonth;
            time_info.month = lmonth % sysdata.monthsperyear;
            time_info.year = lmonth / sysdata.monthsperyear;
        }

        if ( time_info.hour < sysdata.hoursunrise )
            time_info.sunlight = SUN_DARK;
        else if ( time_info.hour < sysdata.hourdaybegin )
            time_info.sunlight = SUN_RISE;
        else if ( time_info.hour < sysdata.hoursunset )
            time_info.sunlight = SUN_LIGHT;
        else if ( time_info.hour < sysdata.hournightbegin )
            time_info.sunlight = SUN_SET;
        else
            time_info.sunlight = SUN_DARK;
    }

    if ( !load_weathermap(  ) ) {
        InitializeWeatherMap(  );
    }

    log_string( "Loading holiday chart..." );          /* Samson 5-13-99 */
    load_holidays(  );
    end_tag(  );
    /*
     * Assign gsn's for skills which need them.
     */
    {
        log_string( "Assigning gsn's" );

        ASSIGN_GSN( gsn_body_drop, "body drop" );
        ASSIGN_GSN( gsn_cannibalize, "cannibalize" );
        ASSIGN_GSN( gsn_marble_skin, "marble skin" );
        ASSIGN_GSN( gsn_granite_skin, "granite skin" );
        ASSIGN_GSN( gsn_break, "break" );
        ASSIGN_GSN( gsn_brain_boil, "brain boil" );
        ASSIGN_GSN( gsn_death_embrace, "death embrace" );
        ASSIGN_GSN( gsn_heart_grab, "heart grab" );
        ASSIGN_GSN( gsn_hallowed_blow, "hallowed blow" );
        ASSIGN_GSN( gsn_glacial_armor, "glacial armor" );
        ASSIGN_GSN( gsn_mitigate, "mitigate" );
        ASSIGN_GSN( gsn_behead, "behead" );
        ASSIGN_GSN( gsn_remember, "remember" );
        ASSIGN_GSN( gsn_flint_fire, "flint fire" );
        ASSIGN_GSN( gsn_brittle_bone, "brittle bone" );
        ASSIGN_GSN( gsn_festering_wound, "festering wound" );
        ASSIGN_GSN( gsn_dehydrate, "dehydrate" );
        ASSIGN_GSN( gsn_dye, "dye" );
        ASSIGN_GSN( gsn_druidic_hymn, "druidic hymn" );
        ASSIGN_GSN( gsn_shriek, "shriek" );
        ASSIGN_GSN( gsn_shrieking_note, "shrieking note" );
        ASSIGN_GSN( gsn_harmonic_melody, "harmonic melody" );
        ASSIGN_GSN( gsn_human_form, "human form" );
        ASSIGN_GSN( gsn_evac_crescendo, "evac crescendo" );
        ASSIGN_GSN( gsn_thunderous_hymn, "thunderous hymn" );
        ASSIGN_GSN( gsn_stirring_ballad, "stirring ballad" );
        ASSIGN_GSN( gsn_ottos_dance, "ottos dance" );
        ASSIGN_GSN( gsn_unholy_melody, "unholy melody" );
        ASSIGN_GSN( gsn_sound_waves, "sound waves" );
        ASSIGN_GSN( gsn_rousing_tune, "rousing tune" );
        ASSIGN_GSN( gsn_silence, "silence" );
        ASSIGN_GSN( gsn_telepathy, "telepathy" );
        ASSIGN_GSN( gsn_heavens_blessing, "heavens blessing" );
        ASSIGN_GSN( gsn_shapeshift, "shapeshift" );
        ASSIGN_GSN( gsn_shield, "shield" );
        ASSIGN_GSN( gsn_higher_magic, "higher magic" );
        ASSIGN_GSN( gsn_sanctuary, "sanctuary" );
        ASSIGN_GSN( gsn_wings, "wings" );
        ASSIGN_GSN( gsn_demonic_sight, "demonic sight" );
        ASSIGN_GSN( gsn_fury, "fury" );
        ASSIGN_GSN( gsn_ensnare, "ensnare" );
        ASSIGN_GSN( gsn_layhands, "layhands" );
        ASSIGN_GSN( gsn_endurance, "endurance" );
        ASSIGN_GSN( gsn_sidestep, "sidestep" );
        ASSIGN_GSN( gsn_daze, "daze" );
        ASSIGN_GSN( gsn_recoil, "recoil" );
        ASSIGN_GSN( gsn_keen_eye, "keen eye" );
        ASSIGN_GSN( gsn_defensive_posturing, "defensive posturing" );
        ASSIGN_GSN( gsn_boost, "boost" );
        ASSIGN_GSN( gsn_prayer, "prayer" );
        ASSIGN_GSN( gsn_ritual, "ritual" );
        ASSIGN_GSN( gsn_desecrate, "desecrate" );
        ASSIGN_GSN( gsn_faith, "faith" );
        ASSIGN_GSN( gsn_martial_arts, "martial arts" );
        ASSIGN_GSN( gsn_biofeedback, "biofeedback" );
        ASSIGN_GSN( gsn_untangle, "untangle" );
        ASSIGN_GSN( gsn_iron_skin, "iron skin" );
        ASSIGN_GSN( gsn_impale, "impale" );
        ASSIGN_GSN( gsn_sustain_self, "sustain self" );
        ASSIGN_GSN( gsn_root, "root" );
        ASSIGN_GSN( gsn_acidic_touch, "acidic touch" );
        ASSIGN_GSN( gsn_entangle, "entangle" );
        ASSIGN_GSN( gsn_tangle, "tangle" );
        ASSIGN_GSN( gsn_cone, "cone" );
        ASSIGN_GSN( gsn_defend, "defend" );
        ASSIGN_GSN( gsn_assassinate, "assassinate" );
        ASSIGN_GSN( gsn_breath, "breath" );
        ASSIGN_GSN( gsn_lick, "lick" );
        ASSIGN_GSN( gsn_tears, "tears" );
        ASSIGN_GSN( gsn_eldritch_bolt, "eldritch bolt" );
        ASSIGN_GSN( gsn_taunt, "taunt" );
        ASSIGN_GSN( gsn_mortify, "mortify" );
        ASSIGN_GSN( gsn_assault, "assault" );
        ASSIGN_GSN( gsn_enrage, "enrage" );
        ASSIGN_GSN( gsn_pawn, "pawn" );
        ASSIGN_GSN( gsn_wailing, "wailing" );
        ASSIGN_GSN( gsn_flamebreath, "flamebreath" );
        ASSIGN_GSN( gsn_energy_containment, "energy containment" );
        ASSIGN_GSN( gsn_rotten_gut, "rotten gut" );    // Aurin
        /*
         * new gsn assigns for the new weapon skills - Grimm 
         */
        ASSIGN_GSN( gsn_pugilism, "pugilism" );
        ASSIGN_GSN( gsn_2h_long_blades, "2h long blades" );
        ASSIGN_GSN( gsn_1h_long_blades, "1h long blades" );
        ASSIGN_GSN( gsn_1h_short_blades, "1h short blades" );
        ASSIGN_GSN( gsn_whips, "whips" );
        ASSIGN_GSN( gsn_2h_bludgeons, "2h bludgeons" );
        ASSIGN_GSN( gsn_1h_bludgeons, "1h bludgeons" );
        ASSIGN_GSN( gsn_blowguns, "blowguns" );
        ASSIGN_GSN( gsn_2h_axes, "2h axes" );
        ASSIGN_GSN( gsn_1h_axes, "1h axes" );
        ASSIGN_GSN( gsn_spears, "spears" );
        ASSIGN_GSN( gsn_staves, "staves" );
        ASSIGN_GSN( gsn_archery, "archery" );
        ASSIGN_GSN( gsn_restring, "restring" );
        ASSIGN_GSN( gsn_lances, "lances" );
        ASSIGN_GSN( gsn_flails, "flails" );
        ASSIGN_GSN( gsn_talons, "talons" );
        ASSIGN_GSN( gsn_polearms, "polearms" );

        ASSIGN_GSN( gsn_detrap, "detrap" );
        ASSIGN_GSN( gsn_arteries, "arteries" );
        ASSIGN_GSN( gsn_backstab, "backstab" );
        ASSIGN_GSN( gsn_circle, "circle" );
        ASSIGN_GSN( gsn_tumble, "tumble" );
        ASSIGN_GSN( gsn_dodge, "dodge" );
        ASSIGN_GSN( gsn_displacement, "displacement" );
        ASSIGN_GSN( gsn_phase, "phase" );
        ASSIGN_GSN( gsn_hide, "hide" );
        ASSIGN_GSN( gsn_smuggle, "smuggle" );
        ASSIGN_GSN( gsn_peek, "peek" );
        ASSIGN_GSN( gsn_pick_lock, "pick lock" );
        ASSIGN_GSN( gsn_sneak, "sneak" );
        ASSIGN_GSN( gsn_steal, "steal" );
        ASSIGN_GSN( gsn_gouge, "gouge" );
        ASSIGN_GSN( gsn_poison_weapon, "poison weapon" );
        ASSIGN_GSN( gsn_ball_of_fire, "ball of fire" );
        ASSIGN_GSN( gsn_feign_death, "feign death" );
        ASSIGN_GSN( gsn_disarm, "disarm" );
        ASSIGN_GSN( gsn_enhanced_damage, "enhanced damage" );
        ASSIGN_GSN( gsn_grendals_stance, "grendals stance" );
        ASSIGN_GSN( gsn_focus, "focus" );
        ASSIGN_GSN( gsn_masters_eye, "masters eye" );
        ASSIGN_GSN( gsn_kick, "kick" );
        ASSIGN_GSN( gsn_know_enemy, "know enemy" );
        ASSIGN_GSN( gsn_quivering_palm, "quivering palm" );
        ASSIGN_GSN( gsn_inferno, "inferno" );
        ASSIGN_GSN( gsn_blizzard, "blizzard" );
        ASSIGN_GSN( gsn_tackle, "tackle" );
        ASSIGN_GSN( gsn_backhand, "backhand" );
        ASSIGN_GSN( gsn_righteous_blow, "righteous blow" );
        ASSIGN_GSN( gsn_maim, "maim" );
        ASSIGN_GSN( gsn_parry, "parry" );
        ASSIGN_GSN( gsn_second_attack, "second attack" );
        ASSIGN_GSN( gsn_third_attack, "third attack" );
        ASSIGN_GSN( gsn_fourth_attack, "fourth attack" );
        ASSIGN_GSN( gsn_fifth_attack, "fifth attack" );
        ASSIGN_GSN( gsn_sixth_attack, "sixth attack" );
        ASSIGN_GSN( gsn_seventh_attack, "seventh attack" );
        ASSIGN_GSN( gsn_eighth_attack, "eighth attack" );
        ASSIGN_GSN( gsn_dual_wield, "dual wield" );
        ASSIGN_GSN( gsn_punch, "punch" );
        ASSIGN_GSN( gsn_bash, "bash" );
        ASSIGN_GSN( gsn_stun, "stun" );
        ASSIGN_GSN( gsn_bashdoor, "doorbash" );
        ASSIGN_GSN( gsn_grip, "grip" );
        ASSIGN_GSN( gsn_berserk, "berserk" );
        ASSIGN_GSN( gsn_hitall, "hitall" );
        ASSIGN_GSN( gsn_feed, "feed" );
        ASSIGN_GSN( gsn_bloodlet, "bloodlet" );
        ASSIGN_GSN( gsn_broach, "broach" );
        ASSIGN_GSN( gsn_mine, "mine" );
        ASSIGN_GSN( gsn_mistwalk, "mistwalk" );
        ASSIGN_GSN( gsn_forge, "forge" );
        ASSIGN_GSN( gsn_tan, "tan" );
        ASSIGN_GSN( gsn_hunt, "hunt" );
        ASSIGN_GSN( gsn_mix, "mix" );
        ASSIGN_GSN( gsn_crafted_food, "crafted_food" );
        ASSIGN_GSN( gsn_crafted_drink, "crafted_drink" );
        ASSIGN_GSN( gsn_gather, "gather" );
        ASSIGN_GSN( gsn_bake, "bake" );
        ASSIGN_GSN( gsn_aid, "aid" );
        ASSIGN_GSN( gsn_track, "track" );
        ASSIGN_GSN( gsn_smell, "smell" );
        ASSIGN_GSN( gsn_search, "search" );
        ASSIGN_GSN( gsn_earthspeak, "earthspeak" );
        ASSIGN_GSN( gsn_mount, "mount" );
        ASSIGN_GSN( gsn_snatch, "snatch" );
        ASSIGN_GSN( gsn_shelter, "shelter" );
        ASSIGN_GSN( gsn_gauge, "gauge" );
        ASSIGN_GSN( gsn_burrow, "burrow" );
        ASSIGN_GSN( gsn_vampiric_strength, "vampiric strength" );
        ASSIGN_GSN( gsn_surreal_speed, "surreal speed" );
        ASSIGN_GSN( gsn_shrink, "shrink" );
        ASSIGN_GSN( gsn_passage, "passage" );
        ASSIGN_GSN( gsn_conjure_elemental, "conjure elemental" );
        ASSIGN_GSN( gsn_animal_companion, "animal companion" );
        ASSIGN_GSN( gsn_lesser_skeleton, "lesser skeleton" );
        ASSIGN_GSN( gsn_extract_skeleton, "extract skeleton" );
        ASSIGN_GSN( gsn_judo, "judo" );
        ASSIGN_GSN( gsn_paralyze, "paralyze" );
        ASSIGN_GSN( gsn_judge, "judge" );
        ASSIGN_GSN( gsn_bone, "bone" );
        ASSIGN_GSN( gsn_angelfire, "angelfire" );
        ASSIGN_GSN( gsn_unholy_sphere, "unholy sphere" );
        ASSIGN_GSN( gsn_charge, "charge" );
        ASSIGN_GSN( gsn_meditate, "meditate" );
        ASSIGN_GSN( gsn_trance, "trance" );
        ASSIGN_GSN( gsn_pluck, "pluck" );
        ASSIGN_GSN( gsn_drop, "drop" );
        ASSIGN_GSN( gsn_rage, "rage" );
        ASSIGN_GSN( gsn_fly_home, "fly home" );
        ASSIGN_GSN( gsn_draw_mana, "draw mana" );      // Taon
        ASSIGN_GSN( gsn_harm_touch, "harm touch" );    // Vladaar
        ASSIGN_GSN( gsn_siphon_strength, "siphon strength" );   // Vladaar
        ASSIGN_GSN( gsn_devour, "devour" );
        ASSIGN_GSN( gsn_spike, "spike" );
        ASSIGN_GSN( gsn_gut, "gut" );
        ASSIGN_GSN( gsn_submerged, "submerged" );
        ASSIGN_GSN( gsn_deathroll, "deathroll" );
        ASSIGN_GSN( gsn_ballistic, "ballistic" );
        ASSIGN_GSN( gsn_hellfire, "hellfire" );
        ASSIGN_GSN( gsn_flaming_shield, "flaming shield" );
        ASSIGN_GSN( gsn_flaming_whip, "flaming whip" );

/* Bard stuff! */
        ASSIGN_GSN( gsn_play, "play" );
        ASSIGN_GSN( gsn_vocals, "vocals" );
        ASSIGN_GSN( gsn_woodwinds, "woodwinds" );
        ASSIGN_GSN( gsn_strings, "strings" );
        ASSIGN_GSN( gsn_brass, "brass" );
        ASSIGN_GSN( gsn_drums, "drums" );

/* Brain guy skills */
        ASSIGN_GSN( gsn_choke, "choke" );
        ASSIGN_GSN( gsn_thicken_skin, "thicken skin" );
        ASSIGN_GSN( gsn_mental_assault, "mental assault" );
        ASSIGN_GSN( gsn_kinetic_barrier, "kinetic barrier" );
        ASSIGN_GSN( gsn_leech, "leech" );
        ASSIGN_GSN( gsn_dream_walk, "dream walk" );
        ASSIGN_GSN( gsn_psionic_blast, "psionic blast" );
        ASSIGN_GSN( gsn_graft_weapon, "graft weapon" );
        ASSIGN_GSN( gsn_chameleon, "chameleon" );
        ASSIGN_GSN( gsn_heal, "heal" );
        ASSIGN_GSN( gsn_healing_thoughts, "healing thoughts" );
        ASSIGN_GSN( gsn_object_reading, "object reading" );
        ASSIGN_GSN( gsn_lore, "lore" );
        ASSIGN_GSN( gsn_whip_of_murazor, "whip of murazor" );
        ASSIGN_GSN( gsn_astral_body, "astral body" );
        ASSIGN_GSN( gsn_sustenance, "sustenance" );
        ASSIGN_GSN( gsn_fear, "fear" );
        ASSIGN_GSN( gsn_drowsy, "drowsy" );
        ASSIGN_GSN( gsn_torture_mind, "torture mind" );

        ASSIGN_GSN( gsn_thaitin, "thaitin" );
        ASSIGN_GSN( gsn_truesight, "truesight" );
        ASSIGN_GSN( gsn_nosight, "nosight" );
        ASSIGN_GSN( gsn_tail_swipe, "tail swipe" );
        ASSIGN_GSN( gsn_gust_of_wind, "gust of wind" );
        ASSIGN_GSN( gsn_stomp, "stomp" );
        ASSIGN_GSN( gsn_bite, "bite" );
        ASSIGN_GSN( gsn_claw, "claw" );
        ASSIGN_GSN( gsn_sting, "sting" );
        ASSIGN_GSN( gsn_scribe, "scribe" );
        ASSIGN_GSN( gsn_brew, "brew" );
        ASSIGN_GSN( gsn_climb, "climb" );
        ASSIGN_GSN( gsn_cook, "cook" );
        ASSIGN_GSN( gsn_scan, "scan" );
        ASSIGN_GSN( gsn_slice, "slice" );
        ASSIGN_GSN( gsn_fireball, "fireball" );
        ASSIGN_GSN( gsn_aqua_breath, "aqua breath" );
        ASSIGN_GSN( gsn_swim, "swim" );
        ASSIGN_GSN( gsn_blindness, "blindness" );
        ASSIGN_GSN( gsn_charm_person, "charm person" );
        ASSIGN_GSN( gsn_curse, "curse" );
        ASSIGN_GSN( gsn_invis, "invis" );
        ASSIGN_GSN( gsn_spectral_ward, "spectral ward" );
        ASSIGN_GSN( gsn_spirits_ward, "spirits ward" );
        ASSIGN_GSN( gsn_mass_invis, "mass invis" );
        ASSIGN_GSN( gsn_poison, "poison" );
        ASSIGN_GSN( gsn_sleep, "sleep" );
        ASSIGN_GSN( gsn_possess, "possess" );
        ASSIGN_GSN( gsn_common, "common" );
        ASSIGN_GSN( gsn_elven, "elven" );
        ASSIGN_GSN( gsn_dwarven, "dwarven" );
        ASSIGN_GSN( gsn_pixish, "pixish" );
        ASSIGN_GSN( gsn_ogrian, "ogrian" );
        ASSIGN_GSN( gsn_orcish, "orcish" );
        ASSIGN_GSN( gsn_trollese, "trollese" );
        ASSIGN_GSN( gsn_goblic, "goblic" );
        ASSIGN_GSN( gsn_hobbit, "hobbit" );
        ASSIGN_GSN( gsn_draconic, "draconic" );
        ASSIGN_GSN( gsn_demonic, "demonic" );
        ASSIGN_GSN( gsn_centaurian, "centaurian" );
        ASSIGN_GSN( gsn_silent, "silent" );

        ASSIGN_GSN( gsn_counterstrike, "counterstrike" );   // Taon
        ASSIGN_GSN( gsn_blade_master, "blade master" ); // Taon
        ASSIGN_GSN( gsn_heavy_hands, "heavy hands" );
        ASSIGN_GSN( gsn_find_weakness, "find weakness" );   // Taon 
        ASSIGN_GSN( gsn_rapid_healing, "rapid healing" );   // Taon
        ASSIGN_GSN( gsn_decree_decay, "decree decay" ); // Vladaar
        ASSIGN_GSN( gsn_combat_mind, "combat mind" );  // Taon
        ASSIGN_GSN( gsn_battle_trance, "battle trance" );   // Taon
        ASSIGN_GSN( gsn_sharpen, "sharpen" );          /* sharpen skill --Cronel */

/*  PRIESTS */
        ASSIGN_GSN( gsn_anoint, "anoint" );
    }

    /*
     * Read in all the area files.
     */
    {
        FILE                   *fpList;

        log_string( "Reading in area files..." );
        if ( ( fpList = FileOpen( AREA_LIST, "r" ) ) == NULL ) {
            perror( AREA_LIST );
            shutdown_mud( "Unable to open area list" );
            exit( 1 );
        }

        for ( ;; ) {
            mudstrlcpy( strArea, fread_word( fpList ), MIL );
            if ( strArea[0] == '$' )
                break;
            load_area_file( last_area, strArea );
        }
        FileClose( fpList );
    }

    /*
     * Moved to before resetting areas since autoloaded 
     */
    log_string( "Loading buildlist" );
    load_buildlist(  );

    /*
     *   initialize supermob.
     *    must be done before reset_area!
     *
     */
    init_supermob(  );

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the notes file.
     */
    {
        log_string( "Fixing exits" );
        fix_exits(  );
        fBootDb = FALSE;
        log_string( "Initializing economy" );
        initialize_economy(  );

        log_string( "Resetting areas" );
        area_update(  );
        log_string( "Loading storerooms" );
        load_storages(  );
        log_string( "Loading clans" );
        load_clans(  );
        log_string( "Loading quests" );
        load_quest_list(  );
        log_string( "Loading currency..." );
        load_currency(  );
        log_string( "Loading banks" );
        load_bank(  );
        log_string( "Loading landmarks" );
        load_landmarks(  );
        log_string( "Loading councils" );
        load_councils(  );
        log_string( "Loading deities" );
        load_deity(  );
        log_string( "Loading hometowns" );
        load_htowns(  );
        log_string( "Loading cities" );
        load_cities(  );
        log_string( "Loading watches" );
        load_watchlist(  );
        log_string( "Loading bans" );
        load_banlist(  );
        log_string( "Loading reserved names" );
        load_reserved(  );
        log_string( "Loading auth namelist" );
        load_auth_list(  );
        save_auth_list(  );
        log_string( "Loading corpses" );
        load_corpses(  );
        log_string( "Loading News" );
        load_news(  );
        load_scores(  );
        log_string( "Loading Hints..." );
        load_hint(  );
        log_string( "Loading slay table" );            /* Online slay table - Samson
                                                        * 8-3-98 */
        load_slays(  );
        log_string( "Loading Immortal Hosts" );
        load_imm_host(  );
        /*
         * Morphs MUST be loaded after class and race tables are set up --Shaddai 
         */
        log_string( "Loading Morphs" );
        load_morphs(  );

        load_locations(  );
        log_string( "Loading Global Boards" );
        load_global_boards(  );
        MOBtrigger = TRUE;
    }
    return;
}

/* Load an 'area' header line. */
void load_area( FILE * fp )
{
    AREA_DATA              *pArea;

    CREATE( pArea, AREA_DATA, 1 );

    pArea->first_room = pArea->last_room = NULL;
    pArea->name = fread_string( fp );
    pArea->author = STRALLOC( "unknown" );
    pArea->filename = STRALLOC( strArea );
    pArea->age = 15;
    pArea->influencer = NULL;
    pArea->reset_frequency = 15;
    pArea->hi_soft_range = MAX_LEVEL;
    pArea->hi_hard_range = MAX_LEVEL;
    pArea->weatherx = 0;
    pArea->weathery = 0;
    pArea->clanname = STRALLOC( "none" );
    pArea->influence = 0;
    LINK( pArea, first_area, last_area, next, prev );
    LINK( pArea, first_full_area, last_full_area, next_area, prev_area );
    top_area++;
}

/* Load the version number of the area file if none exists, then it
 * is set to version 0 when #AREA is read in which is why we check for
 * the #AREA here.  --Shaddai
 */
void load_version( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_version: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    area_version = fread_number( fp );
    return;
}

/* Load an author section. Scryn 2/1/96 */
void load_author( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_author: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    if ( tarea->author )
        STRFREE( tarea->author );
    tarea->author = fread_string( fp );
    return;
}

/* Load a derivative author section. Orion
 * Derived from load_author by Scryn.
 */
void load_derivatives( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_derivatives: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    if ( tarea->derivatives )
        STRFREE( tarea->derivatives );
    tarea->derivatives = fread_string( fp );
    return;
}

void load_homeland( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_homeland: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    if ( tarea->htown )
        STRFREE( tarea->htown );
    tarea->htown = fread_string( fp );
    return;
}

void load_desc( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_desc: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    if ( tarea->desc )
        STRFREE( tarea->desc );
    tarea->desc = fread_string( fp );
}

/* Load an economy section. Thoric */
static void load_economy( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_economy: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    tarea->high_economy[DEFAULT_CURR] = fread_number( fp );
    tarea->low_economy[DEFAULT_CURR] = fread_number( fp );
    return;
}

void load_areacurr( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_areacurr: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    tarea->currvnum = fread_number( fp );
    return;
}

void load_higheconomy( AREA_DATA *tarea, FILE * fp )
{
    int                     x1,
                            x2;

    if ( !tarea ) {
        bug( "%s Load_higheconomy: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    x1 = x2 = 0;
    while ( ( x1 = fread_number( fp ) ) >= 0 && x2 < MAX_CURR_TYPE )
        tarea->high_economy[x2++] = x1;
    fread_to_eol( fp );
    return;
}

void load_loweconomy( AREA_DATA *tarea, FILE * fp )
{
    int                     x1,
                            x2;

    if ( !tarea ) {
        bug( "%s Load_loweconomy: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }

    x1 = x2 = 0;
    while ( ( x1 = fread_number( fp ) ) >= 0 && x2 < MAX_CURR_TYPE )
        tarea->low_economy[x2++] = x1;
    fread_to_eol( fp );

    return;
}

/* Reset Message Load, Rennard */
void load_resetmsg( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_resetmsg: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }

    if ( tarea->resetmsg )
        STRFREE( tarea->resetmsg );
    tarea->resetmsg = fread_string( fp );
    return;
}

void load_clanname( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_clanname: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }

    if ( tarea->clanname )
        STRFREE( tarea->clanname );
    tarea->clanname = fread_string( fp );
}

void load_influence( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_influence: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }

    tarea->influence = fread_number( fp );
    return;
}

/*
 * Load an weather cell. -Kayle
 */
void load_weathercell( AREA_DATA *tarea, FILE * fp )
{
    if ( !tarea ) {
        bug( "%s Load_economy: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }

    tarea->weatherx = fread_number( fp );
    tarea->weathery = fread_number( fp );
    return;
}

/*
 * Load area flags. Narn, Mar/96 
 */
void load_flags( AREA_DATA *tarea, FILE * fp )
{
    char                   *ln;
    int                     x1,
                            x2;

    if ( !tarea ) {
        bug( "%s Load_flags: no #AREA seen yet.", __FUNCTION__ );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    ln = fread_line( fp );
    x1 = x2 = 0;
    sscanf( ln, "%d %d", &x1, &x2 );
    tarea->flags = x1;
    tarea->reset_frequency = x2;
    if ( x2 )
        tarea->age = x2;
    return;
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword.   -Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( HELP_DATA *pHelp )
{
    HELP_DATA              *tHelp;
    int                     match;

    /*
     * Dont add if invalid keyword 
     */
    if ( !VLD_STR( pHelp->keyword ) )
        return;
    for ( tHelp = first_help; tHelp; tHelp = tHelp->next ) {
        if ( pHelp->level == tHelp->level && strcmp( pHelp->keyword, tHelp->keyword ) == 0 ) {
            bug( "%s add_help: duplicate: %s.  Deleting.", __FUNCTION__, pHelp->keyword );
            if ( VLD_STR( pHelp->text ) )
                STRFREE( pHelp->text );
            if ( VLD_STR( pHelp->text ) )
                STRFREE( pHelp->keyword );
            DISPOSE( pHelp );
            return;
        }
        else if ( ( match =
                    strcmp( pHelp->keyword[0] ==
                            '\'' ? pHelp->keyword + 1 : pHelp->keyword,
                            tHelp->keyword[0] == '\'' ? tHelp->keyword + 1 : tHelp->keyword ) ) < 0
                  || ( match == 0 && pHelp->level > tHelp->level ) ) {
            if ( !tHelp->prev )
                first_help = pHelp;
            else
                tHelp->prev->next = pHelp;
            pHelp->prev = tHelp->prev;
            pHelp->next = tHelp;
            tHelp->prev = pHelp;
            break;
        }
    }
    if ( !tHelp )
        LINK( pHelp, first_help, last_help, next, prev );
    top_help++;
}

/* Load a help section. */
void load_helps( AREA_DATA *tarea, FILE * fp )
{
    HELP_DATA              *pHelp;
    struct timeval          now_time;

    for ( ;; ) {
        CREATE( pHelp, HELP_DATA, 1 );

        pHelp->level = fread_number( fp );
        pHelp->modified_time = fread_number( fp );
        pHelp->keyword = fread_string( fp );
        if ( VLD_STR( pHelp->keyword ) && pHelp->keyword[0] == '$' ) {
            STRFREE( pHelp->keyword );
            DISPOSE( pHelp );
            break;
        }
        if ( !VLD_STR( pHelp->keyword ) ) {
            DISPOSE( pHelp );
            /*
             * Fread the next string and then continue 
             */
            fread_flagstring( fp );
            continue;
        }
        pHelp->text = fread_string( fp );
        if ( !str_cmp( pHelp->keyword, "genesis_greeting" ) )
            help_greeting = pHelp->text;
        /*
         * If no time set set it to current time 
         */
        if ( !pHelp->modified_time ) {
            gettimeofday( &now_time, NULL );
            pHelp->modified_time = ( time_t ) now_time.tv_sec;
        }
        add_help( pHelp );
    }
    return;
}

/* Add a character to the list of all characters  -Thoric */
void add_char( CHAR_DATA *ch )
{
    LINK( ch, first_char, last_char, next, prev );
}

/* Load a mob section. */
void load_mobiles( AREA_DATA *tarea, FILE * fp )
{
    MOB_INDEX_DATA         *pMobIndex;
    char                   *ln;
    int                     x1,
                            x2,
                            x3,
                            x4,
                            x5,
                            x6,
                            x7,
                            x8;

    if ( !tarea ) {
        bug( "Load_mobiles: no #AREA seen yet." );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    for ( ;; ) {
        char                    buf[MSL];
        int                     vnum;
        char                    letter;
        int                     iHash;
        bool                    oldmob;
        bool                    tmpBootDb;

        letter = fread_letter( fp );
        if ( letter != '#' ) {
            bug( "Load_mobiles: # not found." );
            if ( fBootDb ) {
                shutdown_mud( "# not found" );
                exit( 1 );
            }
            else
                return;
        }
        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;
        tmpBootDb = fBootDb;
        fBootDb = FALSE;
        if ( get_mob_index( vnum ) ) {
            if ( tmpBootDb ) {
                bug( "Load_mobiles: vnum %d duplicated.", vnum );
                shutdown_mud( "duplicate vnum" );
                exit( 1 );
            }
            else {
                pMobIndex = get_mob_index( vnum );
                snprintf( buf, MSL, "Cleaning mobile: %d", vnum );
                log_string_plus( buf, LOG_BUILD, sysdata.log_level );
                clean_mob( pMobIndex );
                oldmob = TRUE;
            }
        }
        else {
            oldmob = FALSE;
            CREATE( pMobIndex, MOB_INDEX_DATA, 1 );
        }
        fBootDb = tmpBootDb;
        pMobIndex->vnum = vnum;
        if ( fBootDb ) {
            if ( !tarea->low_m_vnum )
                tarea->low_m_vnum = vnum;
            if ( vnum > tarea->hi_m_vnum )
                tarea->hi_m_vnum = vnum;
        }
        pMobIndex->area = tarea;
        pMobIndex->player_name = fread_string( fp );
        pMobIndex->short_descr = fread_string( fp );
        pMobIndex->long_descr = fread_string( fp );
        pMobIndex->description = fread_string( fp );
        if ( area_version > 4 )
            pMobIndex->color = fread_number( fp );
        if ( area_version > 5 ) {
            pMobIndex->clanname = fread_string( fp );
            if ( VLD_STR( pMobIndex->clanname ) )
                pMobIndex->clanname[0] = UPPER( pMobIndex->clanname[0] );
            pMobIndex->influence = fread_number( fp );
        }
        if ( area_version > 6 )
            pMobIndex->slicevnum = fread_number( fp );

        if ( VLD_STR( pMobIndex->long_descr ) )
            pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
        if ( VLD_STR( pMobIndex->description ) )
            pMobIndex->description[0] = UPPER( pMobIndex->description[0] );
        pMobIndex->act = fread_bitvector( fp );
        xSET_BIT( pMobIndex->act, ACT_IS_NPC );
        pMobIndex->affected_by = fread_bitvector( fp );
        /*
         * pMobIndex->affected_by2 = fread_bitvector(fp); 
         */
        pMobIndex->pShop = NULL;
        pMobIndex->rShop = NULL;
        pMobIndex->alignment = fread_number( fp );
        letter = fread_letter( fp );
        pMobIndex->level = fread_number( fp );
        pMobIndex->mobthac0 = fread_number( fp );
        pMobIndex->ac = fread_number( fp );
        pMobIndex->hitnodice = fread_number( fp );
        /*
         * 'd'  
         */ fread_letter( fp );
        pMobIndex->hitsizedice = fread_number( fp );
        /*
         * '+'  
         */ fread_letter( fp );
        pMobIndex->hitplus = fread_number( fp );
        pMobIndex->damnodice = fread_number( fp );
        /*
         * 'd'  
         */ fread_letter( fp );
        pMobIndex->damsizedice = fread_number( fp );
        /*
         * '+'  
         */ fread_letter( fp );
        pMobIndex->damplus = fread_number( fp );
        if ( area_version == 1 )
            pMobIndex->gold = fread_number( fp );
        else {
            int                     val0;

            for ( val0 = 0; val0 < MAX_CURR_TYPE; val0++ )
                GET_MONEY( pMobIndex, val0 ) = fread_number( fp );

            /*
             * Lets only actually use copper for now 
             */
            for ( val0 = 0; val0 < MAX_CURR_TYPE; val0++ )
                if ( val0 != CURR_COPPER )
                    GET_MONEY( pMobIndex, val0 ) = 0;
        }
        pMobIndex->exp = fread_number( fp );
        pMobIndex->position = fread_number( fp );
        if ( pMobIndex->position < 100 ) {
            switch ( pMobIndex->position ) {
                default:
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    break;
                case 5:
                    pMobIndex->position = 6;
                    break;
                case 6:
                    pMobIndex->position = 8;
                    break;
                case 7:
                    pMobIndex->position = 9;
                    break;
                case 8:
                    pMobIndex->position = 12;
                    break;
                case 9:
                    pMobIndex->position = 13;
                    break;
                case 10:
                    pMobIndex->position = 14;
                    break;
                case 11:
                    pMobIndex->position = 15;
                    break;
            }
        }
        else
            pMobIndex->position -= 100;
        pMobIndex->defposition = fread_number( fp );
        if ( pMobIndex->defposition < 100 ) {
            switch ( pMobIndex->defposition ) {
                default:
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    break;
                case 5:
                    pMobIndex->defposition = 6;
                    break;
                case 6:
                    pMobIndex->defposition = 8;
                    break;
                case 7:
                    pMobIndex->defposition = 9;
                    break;
                case 8:
                    pMobIndex->defposition = 12;
                    break;
                case 9:
                    pMobIndex->defposition = 13;
                    break;
                case 10:
                    pMobIndex->defposition = 14;
                    break;
                case 11:
                    pMobIndex->defposition = 15;
                    break;
            }
        }
        else
            pMobIndex->defposition -= 100;
        /*
         * Back to meaningful values. 
         */
        pMobIndex->sex = fread_number( fp );
        if ( letter != 'S' && letter != 'C' ) {
            bug( "Load_mobiles: vnum %d: letter '%c' not S or C.", vnum, letter );
            shutdown_mud( "bad mob data" );
            exit( 1 );
        }
        if ( letter == 'C' ) {                         /* Realms complex mob -Thoric */
            pMobIndex->perm_str = fread_number( fp );
            pMobIndex->perm_int = fread_number( fp );
            pMobIndex->perm_wis = fread_number( fp );
            pMobIndex->perm_dex = fread_number( fp );
            pMobIndex->perm_con = fread_number( fp );
            pMobIndex->perm_cha = fread_number( fp );
            pMobIndex->perm_lck = fread_number( fp );
            pMobIndex->saving_poison_death = fread_number( fp );
            pMobIndex->saving_wand = fread_number( fp );
            pMobIndex->saving_para_petri = fread_number( fp );
            pMobIndex->saving_breath = fread_number( fp );
            pMobIndex->saving_spell_staff = fread_number( fp );
            ln = fread_line( fp );
            x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
            sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
            pMobIndex->race = x1;
            pMobIndex->Class = x2;
            pMobIndex->height = x3;
            pMobIndex->weight = x4;
            pMobIndex->speaks = x5;
            pMobIndex->speaking = x6;
            pMobIndex->numattacks = x7;
            if ( !pMobIndex->speaks )
                pMobIndex->speaks = LANG_COMMON;
            if ( !pMobIndex->speaking )
                pMobIndex->speaking = LANG_COMMON;
            pMobIndex->hitroll = fread_number( fp );
            pMobIndex->damroll = fread_number( fp );
            pMobIndex->xflags = fread_bitvector( fp );
            pMobIndex->resistant = fread_number( fp );
            pMobIndex->immune = fread_number( fp );
            pMobIndex->susceptible = fread_number( fp );
            pMobIndex->attacks = fread_bitvector( fp );
            pMobIndex->defenses = fread_bitvector( fp );
        }
        else {
            pMobIndex->perm_str = 13;
            pMobIndex->perm_dex = 13;
            pMobIndex->perm_int = 13;
            pMobIndex->perm_wis = 13;
            pMobIndex->perm_cha = 13;
            pMobIndex->perm_con = 13;
            pMobIndex->perm_lck = 13;
            pMobIndex->Class = 4;
            xCLEAR_BITS( pMobIndex->xflags );
            xCLEAR_BITS( pMobIndex->attacks );
            xCLEAR_BITS( pMobIndex->defenses );
        }
        letter = fread_letter( fp );
        if ( letter == '>' ) {
            ungetc( letter, fp );
            mprog_read_programs( fp, pMobIndex );
        }
        else
            ungetc( letter, fp );
        if ( !oldmob ) {
            iHash = vnum % MAX_KEY_HASH;
            pMobIndex->next = mob_index_hash[iHash];
            mob_index_hash[iHash] = pMobIndex;
            top_mob_index++;
        }
    }
    return;
}

/* Load an obj section. */
void load_objects( AREA_DATA *tarea, FILE * fp )
{
    OBJ_INDEX_DATA         *pObjIndex;
    char                    letter;
    char                   *ln;
    int                     x1,
                            x2,
                            x3,
                            x4,
                            x5,
                            x6,
                            x7;

    if ( !tarea ) {
        bug( "Load_objects: no #AREA seen yet." );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }
    for ( ;; ) {
        char                    buf[MSL];
        int                     vnum;
        int                     iHash;
        bool                    tmpBootDb;
        bool                    oldobj;

        letter = fread_letter( fp );
        if ( letter != '#' ) {
            bug( "Load_objects: # not found." );
            if ( fBootDb ) {
                shutdown_mud( "# not found" );
                exit( 1 );
            }
            else
                return;
        }
        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;
        tmpBootDb = fBootDb;
        fBootDb = FALSE;
        if ( get_obj_index( vnum ) ) {
            if ( tmpBootDb ) {
                bug( "Load_objects: vnum %d duplicated.", vnum );
                shutdown_mud( "duplicate vnum" );
                exit( 1 );
            }
            else {
                pObjIndex = get_obj_index( vnum );
                snprintf( buf, MSL, "Cleaning object: %d", vnum );
                log_string_plus( buf, LOG_BUILD, sysdata.log_level );
                clean_obj( pObjIndex );
                oldobj = TRUE;
            }
        }
        else {
            oldobj = FALSE;
            CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );
        }
        fBootDb = tmpBootDb;
        pObjIndex->vnum = vnum;
        pObjIndex->area = tarea;
        if ( fBootDb ) {
            if ( !tarea->low_o_vnum )
                tarea->low_o_vnum = vnum;
            if ( vnum > tarea->hi_o_vnum )
                tarea->hi_o_vnum = vnum;
        }
        pObjIndex->name = fread_string( fp );
        pObjIndex->short_descr = fread_string( fp );
        pObjIndex->description = fread_string( fp );
        pObjIndex->action_desc = fread_string( fp );
        if ( VLD_STR( pObjIndex->description ) )
            pObjIndex->description[0] = UPPER( pObjIndex->description[0] );
        pObjIndex->item_type = fread_number( fp );
        pObjIndex->extra_flags = fread_bitvector( fp );
        ln = fread_line( fp );
        x1 = x2 = 0;
        sscanf( ln, "%d %d", &x1, &x2 );
        pObjIndex->wear_flags = x1;
        pObjIndex->layers = x2;
        ln = fread_line( fp );
        x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
        sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
        pObjIndex->value[0] = x1;
        pObjIndex->value[1] = x2;
        pObjIndex->value[2] = x3;
        pObjIndex->value[3] = x4;
        pObjIndex->value[4] = x5;
        pObjIndex->value[5] = x6;
        pObjIndex->value[6] = x7;
        pObjIndex->weight = fread_number( fp );
        pObjIndex->weight = UMAX( 1, pObjIndex->weight );
        pObjIndex->cost = fread_number( fp );
        pObjIndex->rent = fread_number( fp );          /* unused */
        if ( area_version > 1 ) {
            pObjIndex->currtype = fread_number( fp );
            pObjIndex->currtype = URANGE( FIRST_CURR, pObjIndex->currtype, LAST_CURR );
        }
        if ( area_version > 3 )
            pObjIndex->color = fread_number( fp );
        if ( area_version >= 1 ) {
            switch ( pObjIndex->item_type ) {
                case ITEM_PILL:
                case ITEM_POTION:
                case ITEM_SCROLL:
                    pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
                    pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
                    pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
                    break;
                case ITEM_STAFF:
                case ITEM_WAND:
                    pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
                    break;
                case ITEM_SALVE:
                    pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
                    pObjIndex->value[6] = skill_lookup( fread_word( fp ) );
                    break;
            }
        }
        for ( ;; ) {
            letter = fread_letter( fp );

            if ( letter == 'A' ) {
                AFFECT_DATA            *paf;

                CREATE( paf, AFFECT_DATA, 1 );

                paf->type = -1;
                paf->duration = -1;
                paf->location = fread_number( fp );
                if ( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
                     || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
                     || paf->location == APPLY_RECURRINGSPELL )
                    paf->modifier = slot_lookup( fread_number( fp ) );
                else
                    paf->modifier = fread_number( fp );
                xCLEAR_BITS( paf->bitvector );
                LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
                top_affect++;
            }
            else if ( letter == 'E' ) {
                EXTRA_DESCR_DATA       *ed;

                CREATE( ed, EXTRA_DESCR_DATA, 1 );

                ed->keyword = fread_string( fp );
                ed->description = fread_string( fp );
                LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
                top_ed++;
            }
            else if ( letter == '>' ) {
                ungetc( letter, fp );
                oprog_read_programs( fp, pObjIndex );
            }
            else if ( letter == 'S' ) {
//        pObjIndex->size = fread_number(fp);
                ln = fread_line( fp );
                x1 = x2 = 0;
                sscanf( ln, "%d %d", &x1, &x2 );
                pObjIndex->size = x1;
                if ( x2 )
                    pObjIndex->rating = x2;
//        fread_to_eol(fp);
            }
            else {
                ungetc( letter, fp );
                break;
            }
        }
        /*
         * Translate spell "slot numbers" to internal "skill numbers." 
         */
        if ( area_version == 0 )
            switch ( pObjIndex->item_type ) {
                case ITEM_PILL:
                case ITEM_POTION:
                case ITEM_SCROLL:
                    pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
                    pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
                    pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
                    break;
                case ITEM_STAFF:
                case ITEM_WAND:
                    pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
                    break;
                case ITEM_SALVE:
                    pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
                    pObjIndex->value[6] = slot_lookup( pObjIndex->value[6] );
                    break;
            }
        if ( !oldobj ) {
            iHash = vnum % MAX_KEY_HASH;
            pObjIndex->next = obj_index_hash[iHash];
            obj_index_hash[iHash] = pObjIndex;
            top_obj_index++;
        }
    }
    return;
}

/* Load a reset section. */
void load_resets( AREA_DATA *tarea, FILE * fp )
{
    ROOM_INDEX_DATA        *pRoomIndex = NULL;
    ROOM_INDEX_DATA        *roomlist;
    bool                    not01 = FALSE;
    int                     count = 0;

    if ( !tarea ) {
        bug( "Load_resets: no #AREA seen yet." );
        if ( fBootDb ) {
            shutdown_mud( "No #AREA" );
            exit( 1 );
        }
        else
            return;
    }

    if ( !tarea->first_room ) {
        bug( "load_resets: No #ROOMS section found. Cannot load resets." );
        if ( fBootDb ) {
            shutdown_mud( "No #ROOMS" );
            exit( 1 );
        }
        else
            return;
    }

    for ( ;; ) {
        EXIT_DATA              *pexit;
        char                    letter;
        int                     extra,
                                arg1,
                                arg2,
                                arg3;

        if ( ( letter = fread_letter( fp ) ) == 'S' )
            break;

        if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }

        extra = fread_number( fp );
        if ( letter == 'M' || letter == 'O' )
            extra = 0;
        arg1 = fread_number( fp );
        arg2 = fread_number( fp );
        arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
        fread_to_eol( fp );
        ++count;

        /*
         * Validate parameters.
         * We're calling the index functions for the side effect.
         */
        switch ( letter ) {
            default:
                bug( "%s: bad command '%c'.", __FUNCTION__, letter );
                if ( fBootDb )
                    boot_log( "load_resets: %s (%d) bad command '%c'.", __FUNCTION__,
                              tarea->filename, count, letter );
                return;

            case 'M':
                if ( get_mob_index( arg1 ) == NULL && fBootDb )
                    boot_log( "load_resets: %s (%d) 'M': mobile %d doesn't exist.", tarea->filename,
                              count, arg1 );

                if ( ( pRoomIndex = get_room_index( arg3 ) ) == NULL && fBootDb )
                    boot_log( "load_resets: %s (%d) 'M': room %d doesn't exist.", tarea->filename,
                              count, arg3 );
                else
                    add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                break;

            case 'O':
                if ( get_obj_index( arg1 ) == NULL && fBootDb )
                    boot_log( "load_resets: %s (%d) '%c': object %d doesn't exist.",
                              tarea->filename, count, letter, arg1 );

                if ( ( pRoomIndex = get_room_index( arg3 ) ) == NULL && fBootDb )
                    boot_log( "load_resets: %s (%d) '%c': room %d doesn't exist.", tarea->filename,
                              count, letter, arg3 );
                else {
                    if ( !pRoomIndex )
                        bug( "load_resets: Unable to add room reset - room not found." );
                    else
                        add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                }
                break;

            case 'P':
                if ( get_obj_index( arg1 ) == NULL && fBootDb )
                    boot_log( "load_resets: %s (%d) '%c': object %d doesn't exist.",
                              tarea->filename, count, letter, arg1 );
                if ( arg3 > 0 ) {
                    if ( get_obj_index( arg3 ) == NULL && fBootDb )
                        boot_log( "load_resets: %s (%d) 'P': destination object %d doesn't exist.",
                                  tarea->filename, count, arg3 );
                    if ( extra > 1 )
                        not01 = TRUE;
                }
                if ( !pRoomIndex )
                    bug( "load_resets: Unable to add room reset - room not found." );
                else {
                    if ( arg3 == 0 )
                        arg3 = OBJ_VNUM_MONEY_ONE;     // This may look stupid, but for
                    // some reason it works.
                    add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                }
                break;

            case 'G':
            case 'E':
                if ( get_obj_index( arg1 ) == NULL && fBootDb )
                    boot_log( "load_resets: %s (%d) '%c': object %d doesn't exist.",
                              tarea->filename, count, letter, arg1 );
                if ( !pRoomIndex )
                    bug( "load_resets: Unable to add room reset - room not found." );
                else
                    add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                break;
            case 'T':
                if ( IS_SET( extra, TRAP_OBJ ) )
                    bug( "load_resets: Unable to add legacy object trap reset. Must be converted manually." );
                else {
                    if ( !( pRoomIndex = get_room_index( arg3 ) ) )
                        bug( "load_resets: Unable to add trap reset - room not found." );
                    else
                        add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                }
                break;

            case 'H':
                bug( "load_resets: Unable to convert legacy hide reset. Must be converted manually." );
                break;

            case 'D':
                if ( !( pRoomIndex = get_room_index( arg1 ) ) ) {
                    bug( "load_resets: 'D': room %d doesn't exist.", arg1 );
                    bug( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
                    if ( fBootDb )
                        boot_log( "load_resets: %s (%d) 'D': room %d doesn't exist.",
                                  tarea->filename, count, arg1 );
                    break;
                }

                if ( arg2 < 0 || arg2 > MAX_DIR + 1 || !( pexit = get_exit( pRoomIndex, arg2 ) )
                     || !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
                    bug( "load_resets: 'D': exit %d not door.", arg2 );
                    bug( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
                    if ( fBootDb )
                        boot_log( "load_resets: %s (%d) 'D': exit %d not door.", tarea->filename,
                                  count, arg2 );
                }

                if ( arg3 < 0 || arg3 > 2 ) {
                    bug( "load_resets: 'D': bad 'locks': %d.", arg3 );
                    if ( fBootDb )
                        boot_log( "load_resets: %s (%d) 'D': bad 'locks': %d.", tarea->filename,
                                  count, arg3 );
                }
                add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                break;

            case 'R':
                if ( !( pRoomIndex = get_room_index( arg1 ) ) && fBootDb )
                    boot_log( "load_resets: %s (%d) 'R': room %d doesn't exist.", tarea->filename,
                              count, arg1 );
                else
                    add_reset( pRoomIndex, letter, extra, arg1, arg2, arg3 );
                if ( arg2 < 0 || arg2 > 11 ) {
                    bug( "load_resets: 'R': bad exit %d.", arg2 );
                    if ( fBootDb )
                        boot_log( "load_resets: %s (%d) 'R': bad exit %d.", tarea->filename, count,
                                  arg2 );
                    break;
                }
                break;
        }
    }
    if ( !not01 ) {
        for ( roomlist = tarea->first_room; roomlist; roomlist = roomlist->next_aroom )
            renumber_put_resets( roomlist );
    }
    return;
}

void load_room_reset( ROOM_INDEX_DATA *room, FILE * fp )
{
    EXIT_DATA              *pexit;
    char                    letter;
    int                     extra,
                            arg1,
                            arg2,
                            arg3;
    bool                    not01 = FALSE;
    int                     count = 0;

    letter = fread_letter( fp );
    extra = fread_number( fp );
    if ( letter == 'M' || letter == 'O' )
        extra = 0;
    arg1 = fread_number( fp );
    arg2 = fread_number( fp );
    arg3 = ( letter == 'G' || letter == 'R' ) ? 0 : fread_number( fp );
    fread_to_eol( fp );
    ++count;

    /*
     * Validate parameters.
     * We're calling the index functions for the side effect.
     */
    switch ( letter ) {
        default:
            bug( "%s: Room: %d: bad command '%c'.", __FUNCTION__, room->vnum, letter );
            if ( fBootDb )
                boot_log( "%s: Room: %d: %s (%d) bad command '%c'.", __FUNCTION__, room->vnum,
                          room->area->filename, count, letter );
            return;

        case 'M':
            if ( get_mob_index( arg1 ) == NULL && fBootDb )
                boot_log( "%s: Room: %d: %s (%d) 'M': mobile %d doesn't exist.", __FUNCTION__,
                          room->vnum, room->area->filename, count, arg1 );
            break;

        case 'O':
            if ( get_obj_index( arg1 ) == NULL && fBootDb )
                boot_log( "%s: Room: %d: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__,
                          room->vnum, room->area->filename, count, letter, arg1 );
            break;

        case 'P':
            if ( get_obj_index( arg1 ) == NULL && fBootDb )
                boot_log( "%s: Room: %d: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__,
                          room->vnum, room->area->filename, count, letter, arg1 );
            if ( arg3 <= 0 )
                arg3 = OBJ_VNUM_MONEY_ONE;             // This may look stupid, but for
            // some reason it works.
            if ( get_obj_index( arg3 ) == NULL && fBootDb )
                boot_log( "%s: Room: %d: %s (%d) 'P': destination object %d doesn't exist.",
                          __FUNCTION__, room->vnum, room->area->filename, count, arg3 );
            if ( extra > 1 )
                not01 = TRUE;
            break;

        case 'G':
        case 'E':
            if ( get_obj_index( arg1 ) == NULL && fBootDb )
                boot_log( "%s: Room: %d: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__,
                          room->vnum, room->area->filename, count, letter, arg1 );
            break;

        case 'T':
        case 'H':
            break;

        case 'D':
            if ( arg2 < 0 || arg2 > MAX_DIR + 1 || !( pexit = get_exit( room, arg2 ) )  /* ||
                                                                                         * *
                                                                                         * !IS_SET(
                                                                                         * *
                                                                                         * pexit->exit_info, 
                                                                                         * * * *
                                                                                         * EX_ISDOOR
                                                                                         * * ) */  ) {
                bug( "%s: Room: %d: 'D': exit %d not door.", __FUNCTION__, room->vnum, arg2 );
                bug( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
                if ( fBootDb )
                    boot_log( "%s: Room: %d: %s (%d) 'D': exit %d not door.", __FUNCTION__,
                              room->vnum, room->area->filename, count, arg2 );
            }

            if ( arg3 < 0 || arg3 > 3 ) {              /* Volk - changed from 2 to 3 as
                                                        * we now have 3 resets (3 * being 
                                                        * a * * * DIG tunnel) */
                bug( "%s: Room: %d: 'D': bad 'locks': %d.", __FUNCTION__, room->vnum, arg3 );
                if ( fBootDb )
                    boot_log( "%s: Room: %d: %s (%d) 'D': bad 'locks': %d.", __FUNCTION__,
                              room->vnum, room->area->filename, count, arg3 );
            }
            break;

        case 'R':
            if ( arg2 < 0 || arg2 > 11 ) {
                bug( "%s: Room: %d: 'R': bad exit %d.", __FUNCTION__, room->vnum, arg2 );
                if ( fBootDb )
                    boot_log( "%s: Room: %d: %s (%d) 'R': bad exit %d.", __FUNCTION__, room->vnum,
                              room->area->filename, count, arg2 );
                break;
            }
            break;
    }
    add_reset( room, letter, extra, arg1, arg2, arg3 );

    if ( !not01 )
        renumber_put_resets( room );
}

/* Load a room section. */
void load_rooms( AREA_DATA *tarea, FILE * fp )
{
    ROOM_INDEX_DATA        *pRoomIndex;
    char                    buf[MSL];
    char                   *ln;

    if ( !tarea ) {
        bug( "Load_rooms: no #AREA seen yet." );
        shutdown_mud( "No #AREA" );
        exit( 1 );
    }
    tarea->first_room = tarea->last_room = NULL;
    for ( ;; ) {
        int                     vnum;
        char                    letter;
        int                     door;
        int                     iHash;
        bool                    tmpBootDb;
        bool                    oldroom;
        int                     x1,
                                x2,
                                x3,
                                x4,
                                x5,
                                x6,
                                x7;

        letter = fread_letter( fp );
        if ( letter != '#' ) {
            bug( "Load_rooms: # not found." );
            if ( fBootDb ) {
                shutdown_mud( "# not found" );
                exit( 1 );
            }
            else
                return;
        }

        vnum = fread_number( fp );
        if ( vnum == 0 )
            break;

        tmpBootDb = fBootDb;
        fBootDb = FALSE;
        if ( get_room_index( vnum ) != NULL ) {
            if ( tmpBootDb ) {
                bug( "Load_rooms: vnum %d duplicated.", vnum );
                shutdown_mud( "duplicate vnum" );
                exit( 1 );
            }
            else {
                pRoomIndex = get_room_index( vnum );
                snprintf( buf, MSL, "Cleaning room: %d", vnum );
                log_string_plus( buf, LOG_BUILD, sysdata.log_level );
                clean_room( pRoomIndex );
                oldroom = TRUE;
            }
        }
        else {
            oldroom = FALSE;
            CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );

            pRoomIndex->first_person = NULL;
            pRoomIndex->last_person = NULL;
            pRoomIndex->first_content = NULL;
            pRoomIndex->last_content = NULL;
        }

        fBootDb = tmpBootDb;
        pRoomIndex->area = tarea;
        pRoomIndex->vnum = vnum;
        pRoomIndex->first_extradesc = NULL;
        pRoomIndex->last_extradesc = NULL;

        if ( fBootDb ) {
            if ( !tarea->low_r_vnum )
                tarea->low_r_vnum = vnum;
            if ( vnum > tarea->hi_r_vnum )
                tarea->hi_r_vnum = vnum;
        }
        pRoomIndex->name = fread_string( fp );
        pRoomIndex->description = fread_string( fp );

        /*
         * Area number     fread_number(fp); 
         */
        ln = fread_line( fp );
        x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
        sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );

        bool                    resource = FALSE;

        if ( x1 == 1 )
            resource = TRUE;

        pRoomIndex->room_flags = x2;
        pRoomIndex->sector_type = x3;
        pRoomIndex->tele_delay = x4;
        pRoomIndex->tele_vnum = x5;
        pRoomIndex->tunnel = x6;
        pRoomIndex->height = x7;
        assign_currindex( pRoomIndex );

        if ( pRoomIndex->sector_type < 0 || pRoomIndex->sector_type >= SECT_MAX ) {
            bug( "Fread_rooms: vnum %d has bad sector_type %d.", vnum, pRoomIndex->sector_type );
            pRoomIndex->sector_type = 1;
        }
        pRoomIndex->light = 0;
        pRoomIndex->first_exit = NULL;
        pRoomIndex->last_exit = NULL;

        /*
         * Gah, need to go through each area and force save it! 
         */
        if ( resource ) {
            letter = fread_letter( fp );

            if ( letter == '0' )                       /* old resource code - kill it */
                fread_to_eol( fp );
            else
                ungetc( letter, fp );
        }
/*
      x1 = x2 = 0;
      sscanf(ln, "%d %d", &x1, &x2);

      pRoomIndex->rarity = x1;
      pRoomIndex->maxresources = x2;
      pRoomIndex->curresources = x2;
*/

        for ( ;; ) {
            letter = fread_letter( fp );

            if ( letter == 'S' )
                break;

            if ( letter == 'D' ) {
                EXIT_DATA              *pexit;
                int                     locks;

                door = fread_number( fp );
                if ( door < 0 || door > 11 ) {
                    bug( "Fread_rooms: vnum %d has bad door number %d.", vnum, door );
                    if ( fBootDb )
                        exit( 1 );
                }
                else {
                    pexit = make_exit( pRoomIndex, NULL, door );
                    pexit->description = fread_string( fp );
                    pexit->keyword = fread_string( fp );
                    pexit->exit_info = 0;
                    ln = fread_line( fp );
                    x1 = x2 = x3 = x4 = x5 = x6 = 0;
                    sscanf( ln, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );

                    locks = x1;
                    pexit->key = x2;
                    pexit->vnum = x3;
                    pexit->vdir = door;
                    pexit->pulltype = x4;
                    pexit->pull = x5;

                    switch ( locks ) {
                        case 1:
                            pexit->exit_info = EX_ISDOOR;
                            break;
                        case 2:
                            pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
                            break;
                        default:
                            pexit->exit_info = locks;
                    }
                }
            }
            else if ( letter == 'E' ) {
                EXTRA_DESCR_DATA       *ed;

                CREATE( ed, EXTRA_DESCR_DATA, 1 );

                ed->keyword = fread_string( fp );
                ed->description = fread_string( fp );
                LINK( ed, pRoomIndex->first_extradesc, pRoomIndex->last_extradesc, next, prev );
                top_ed++;
            }
            else if ( letter == 'R' )
                load_room_reset( pRoomIndex, fp );

            else if ( letter == '>' ) {
                ungetc( letter, fp );
                rprog_read_programs( fp, pRoomIndex );
            }
            else {
                bug( "Load_rooms: vnum %d has flag '%c' not 'DES'.", vnum, letter );
                shutdown_mud( "Room flag not DES" );
                exit( 1 );
            }

        }

        if ( !oldroom ) {
            iHash = vnum % MAX_KEY_HASH;
            pRoomIndex->next = room_index_hash[iHash];
            room_index_hash[iHash] = pRoomIndex;
            LINK( pRoomIndex, tarea->first_room, tarea->last_room, next_aroom, prev_aroom );
            top_room++;
        }
    }
}

/*
 * Load a shop section.
 */
void load_shops( AREA_DATA *tarea, FILE * fp )
{
    SHOP_DATA              *pShop;

    for ( ;; ) {
        MOB_INDEX_DATA         *pMobIndex;
        int                     iTrade;

        CREATE( pShop, SHOP_DATA, 1 );

        pShop->keeper = fread_number( fp );
        if ( pShop->keeper == 0 ) {
            DISPOSE( pShop );
            break;
        }
        for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
            pShop->buy_type[iTrade] = fread_number( fp );
        pShop->profit_buy = fread_number( fp );
        pShop->profit_sell = fread_number( fp );
        pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
        pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
        pShop->open_hour = fread_number( fp );
        pShop->close_hour = fread_number( fp );
        fread_to_eol( fp );
        pMobIndex = get_mob_index( pShop->keeper );
        pMobIndex->pShop = pShop;

        if ( !first_shop )
            first_shop = pShop;
        else
            last_shop->next = pShop;
        pShop->next = NULL;
        pShop->prev = last_shop;
        last_shop = pShop;
        top_shop++;
    }
    return;
}

/*
 * Load a repair shop section.     -Thoric
 */
void load_repairs( AREA_DATA *tarea, FILE * fp )
{
    REPAIR_DATA            *rShop;

    for ( ;; ) {
        MOB_INDEX_DATA         *pMobIndex;
        int                     iFix;

        CREATE( rShop, REPAIR_DATA, 1 );
        rShop->keeper = fread_number( fp );
        if ( rShop->keeper == 0 ) {
            DISPOSE( rShop );
            break;
        }
        for ( iFix = 0; iFix < MAX_FIX; iFix++ )
            rShop->fix_type[iFix] = fread_number( fp );
        rShop->profit_fix = fread_number( fp );
        rShop->shop_type = fread_number( fp );
        rShop->open_hour = fread_number( fp );
        rShop->close_hour = fread_number( fp );
        fread_to_eol( fp );
        pMobIndex = get_mob_index( rShop->keeper );
        pMobIndex->rShop = rShop;

        if ( !first_repair )
            first_repair = rShop;
        else
            last_repair->next = rShop;
        rShop->next = NULL;
        rShop->prev = last_repair;
        last_repair = rShop;
        top_repair++;
    }
    return;
}

/*
 * Load spec proc declarations.
 */
void load_specials( AREA_DATA *tarea, FILE * fp )
{
    for ( ;; ) {
        MOB_INDEX_DATA         *pMobIndex;
        char                    letter;

        switch ( letter = fread_letter( fp ) ) {
            default:
                bug( "Load_specials: letter '%c' not *MS.", letter );
                exit( 1 );

            case 'S':
                return;

            case '*':
                break;

            case 'M':
                pMobIndex = get_mob_index( fread_number( fp ) );
                pMobIndex->spec_fun = spec_lookup( fread_word( fp ) );
                if ( pMobIndex->spec_fun == 0 ) {
                    bug( "Load_specials: 'M': vnum %d.", pMobIndex->vnum );
                    exit( 1 );
                }
                break;
        }

        fread_to_eol( fp );
    }
}

/*
 * Load soft / hard area ranges.
 */
void load_ranges( AREA_DATA *tarea, FILE * fp )
{
    int                     x1,
                            x2,
                            x3,
                            x4;
    char                   *ln;

    if ( !tarea ) {
        bug( "Load_ranges: no #AREA seen yet." );
        shutdown_mud( "No #AREA" );
        exit( 1 );
    }

    for ( ;; ) {
        ln = fread_line( fp );

        if ( ln[0] == '$' )
            break;

        x1 = x2 = x3 = x4 = 0;
        sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

        tarea->low_soft_range = x1;
        tarea->hi_soft_range = x2;
        tarea->low_hard_range = x3;
        tarea->hi_hard_range = x4;
    }
    return;

}

/*
 * Load climate information for the area
 * Last modified: July 13, 1997
 * Fireblade
 */
/*
 * With the new Weather System, these are unneeded as the weather is it's own
 * entity seperated from everything else. - Kayle 10-17-07
 */
void load_climate( AREA_DATA *tarea, FILE * fp )
{
    fread_number( fp );
    fread_number( fp );
    fread_number( fp );
}

/*
 * Load data for a neghboring weather system
 * Last modified: July 13, 1997
 * Fireblade
 */
/*
 * With the new Weather System, these are unneeded as the weather is it's own
 * entity seperated from everything else. - Kayle 10-17-07
 */
void load_neighbor( AREA_DATA *tarea, FILE * fp )
{
    fread_flagstring( fp );
}

/*
 * Go through all areas, and set up initial economy based on mob
 * levels and gold
 */
void initialize_economy( void )
{
    AREA_DATA              *tarea;
    MOB_INDEX_DATA         *mob;
    int                     idx,
                            money,
                            rng,
                            type = DEFAULT_CURR;

    for ( tarea = first_area; tarea; tarea = tarea->next ) {
        /*
         * skip area if they already got some money 
         */
        if ( tarea->currindex )
            type = tarea->currindex->primary;
        if ( tarea->high_economy[type] > 0 || tarea->low_economy[type] > 10000 )
            continue;
        rng = tarea->hi_soft_range - tarea->low_soft_range;
        if ( rng )
            rng /= 2;
        else
            rng = 25;
        money = rng * rng * 50000;
        boost_economy( tarea, money, type );
        for ( idx = tarea->low_m_vnum; idx < tarea->hi_m_vnum; idx++ )
            if ( ( mob = get_mob_index( idx ) ) != NULL )
                boost_economy( tarea, GET_MONEY( mob, type ) * 10, type );
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
    ROOM_INDEX_DATA        *pRoomIndex;
    EXIT_DATA              *pexit,
                           *pexit_next,
                           *rev_exit;
    int                     iHash;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
        for ( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next ) {
            bool                    fexit;

            fexit = FALSE;
            for ( pexit = pRoomIndex->first_exit; pexit; pexit = pexit_next ) {
                pexit_next = pexit->next;
                pexit->rvnum = pRoomIndex->vnum;
                if ( pexit->vnum <= 0
                     || ( pexit->to_room = get_room_index( pexit->vnum ) ) == NULL ) {
                    if ( fBootDb )
                        boot_log( "Fix_exits: room %d, exit %s leads to bad vnum (%d)",
                                  pRoomIndex->vnum, dir_name[pexit->vdir], pexit->vnum );

                    bug( "Deleting %s exit in room %d", dir_name[pexit->vdir], pRoomIndex->vnum );
                    extract_exit( pRoomIndex, pexit );
                }
                else
                    fexit = TRUE;
            }
            if ( !fexit )
                SET_BIT( pRoomIndex->room_flags, ROOM_NO_MOB );
        }
    }

    /*
     * Set all the rexit pointers   -Thoric 
     */
    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
        for ( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next ) {
            for ( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next ) {
                if ( pexit->to_room && !pexit->rexit ) {
                    rev_exit =
                        get_exit_to( pexit->to_room, rev_dir[pexit->vdir], pRoomIndex->vnum );
                    if ( rev_exit ) {
                        pexit->rexit = rev_exit;
                        rev_exit->rexit = pexit;
                    }
                }
            }
        }
    }

    return;
}

/*
 * Get diku-compatable exit by number     -Thoric
 */
EXIT_DATA              *get_exit_number( ROOM_INDEX_DATA *room, int xit )
{
    EXIT_DATA              *pexit;
    int                     count;

    count = 0;
    for ( pexit = room->first_exit; pexit; pexit = pexit->next )
        if ( ++count == xit )
            return pexit;
    return NULL;
}

/*
 * (prelude...) This is going to be fun... NOT!
 * (conclusion) QSort is f*cked!
 */
int exit_comp( EXIT_DATA **xit1, EXIT_DATA **xit2 )
{
    int                     d1,
                            d2;

    d1 = ( *xit1 )->vdir;
    d2 = ( *xit2 )->vdir;

    if ( d1 < d2 )
        return -1;
    if ( d1 > d2 )
        return 1;
    return 0;
}

void sort_exits( ROOM_INDEX_DATA *room )
{
    EXIT_DATA              *pexit;                     /* *texit *//* Unused */
    EXIT_DATA              *exits[MAX_REXITS];
    int                     x,
                            nexits;

    nexits = 0;
    for ( pexit = room->first_exit; pexit; pexit = pexit->next ) {
        exits[nexits++] = pexit;
        if ( nexits > MAX_REXITS ) {
            bug( "sort_exits: more than %d exits in room... fatal", nexits );
            return;
        }
    }
    qsort( &exits[0], nexits, sizeof( EXIT_DATA * ),
           ( int ( * )( const void *, const void * ) ) exit_comp );
    for ( x = 0; x < nexits; x++ ) {
        if ( x > 0 )
            exits[x]->prev = exits[x - 1];
        else {
            exits[x]->prev = NULL;
            room->first_exit = exits[x];
        }
        if ( x >= ( nexits - 1 ) ) {
            exits[x]->next = NULL;
            room->last_exit = exits[x];
        }
        else
            exits[x]->next = exits[x + 1];
    }
}

void randomize_exits( ROOM_INDEX_DATA *room, short maxdir )
{
    EXIT_DATA              *pexit;
    int                     nexits,                    /* maxd, */
                            d0,
                            d1,
                            count,
                            door;                      /* Maxd unused */
    int                     vdirs[MAX_REXITS];

    nexits = 0;
    for ( pexit = room->first_exit; pexit; pexit = pexit->next )
        vdirs[nexits++] = pexit->vdir;

    for ( d0 = 0; d0 < nexits; d0++ ) {
        if ( vdirs[d0] > maxdir )
            continue;
        count = 0;
        while ( vdirs[( d1 = number_range( d0, nexits - 1 ) )] > maxdir || ++count > 5 );
        if ( vdirs[d1] > maxdir )
            continue;
        door = vdirs[d0];
        vdirs[d0] = vdirs[d1];
        vdirs[d1] = door;
    }
    count = 0;
    for ( pexit = room->first_exit; pexit; pexit = pexit->next )
        pexit->vdir = vdirs[count++];

    sort_exits( room );
}

/* Repopulate areas periodically. */
void area_update( void )
{
    AREA_DATA              *pArea;

    for ( pArea = first_area; pArea; pArea = pArea->next ) {
        CHAR_DATA              *pch;
        int                     reset_age = pArea->reset_frequency ? pArea->reset_frequency : 15;

        if ( ( reset_age == -1 && pArea->age == -1 ) || ++pArea->age < ( reset_age - 1 ) )
            continue;

        /*
         * Check for PC's.
         */
        if ( pArea->nplayer > 0 && pArea->age == ( reset_age - 1 ) ) {
            char                    buf[MSL];

            /*
             * Rennard 
             */
            if ( pArea->resetmsg )
                snprintf( buf, MSL, "%s\r\n", pArea->resetmsg );
            else
                mudstrlcpy( buf, "You hear some squeaking sounds...\r\n", MSL );
            for ( pch = first_char; pch; pch = pch->next ) {
                if ( !IS_NPC( pch ) && IS_AWAKE( pch ) && pch->in_room
                     && pch->in_room->area == pArea ) {
                    set_char_color( AT_RESET, pch );
                    send_to_char( buf, pch );
                }
            }
        }

        /*
         * Check age and reset.
         * Note: Mud Academy resets every 3 minutes (not 15).
         */
        if ( pArea->nplayer == 0 || pArea->age >= reset_age ) {
            ROOM_INDEX_DATA        *pRoomIndex;

/*      fprintf(stderr, "Resetting: %s\n", pArea->filename); */
            reset_area( pArea );
            if ( reset_age == -1 )
                pArea->age = -1;
            else
                pArea->age = number_range( 0, reset_age / 5 );
            pRoomIndex = get_room_index( ROOM_VNUM_SCHOOL );
            if ( pRoomIndex != NULL && pArea == pRoomIndex->area && pArea->reset_frequency == 0 )
                pArea->age = 15 - 3;
        }
    }
    return;
}

/* Create an instance of a mobile. */
CHAR_DATA              *create_mobile( MOB_INDEX_DATA *pMobIndex )
{
    CHAR_DATA              *mob;
    short                   chance;

    if ( !pMobIndex ) {
        bug( "Create_mobile: NULL pMobIndex." );
        exit( 1 );
    }

    CREATE( mob, CHAR_DATA, 1 );

    clear_char( mob );
    mob->pIndexData = pMobIndex;
    mob->reset = NULL;
    mob->name = QUICKLINK( pMobIndex->player_name );
    mob->short_descr = QUICKLINK( pMobIndex->short_descr );
    mob->long_descr = QUICKLINK( pMobIndex->long_descr );
    mob->description = QUICKLINK( pMobIndex->description );
    mob->spec_fun = pMobIndex->spec_fun;
    mob->mpscriptpos = 0;
    mob->hate_level = 0;                               /* Default for hate_level */
    if ( area_version > 5 ) {
        mob->clanname = STRALLOC( "Clanname" );
        mob->influence = 0;
    }
    mob->slicevnum = pMobIndex->slicevnum;
    mob->level = number_fuzzy( pMobIndex->level );
    mob->act = pMobIndex->act;
    mob->home_vnum = -1;
    mob->reset = NULL;
    if ( xIS_SET( mob->act, ACT_MOBINVIS ) )
        mob->mobinvis = mob->level;
    mob->affected_by = pMobIndex->affected_by;
    mob->alignment = pMobIndex->alignment;
    mob->sex = pMobIndex->sex;
    mob->color = pMobIndex->color;
    // I re-wrote this, no reason for it to check the same value twice. -Taon
    if ( !xIS_SET( mob->act, ACT_OVERRIDE ) ) {
        if ( !pMobIndex->hitnodice ) {
            mob->hit = set_hp( mob->level );
            mob->max_hit = set_hp( mob->level );
        }
        else {
            mob->hit = set_hp( mob->level );
            mob->max_hit = set_hp( mob->level );
        }
        mob->armor = set_armor_class( mob->level );
        mob->hitroll = set_hitroll( mob->level );
        mob->damroll = set_damroll( mob->level );
        mob->numattacks = set_num_attacks( mob->level );;
        mob->hitplus = set_hp( mob->level );
    }
    else {
        mob->armor = pMobIndex->ac;
        mob->hitroll = pMobIndex->hitroll;
        mob->damroll = pMobIndex->damroll;
        mob->numattacks = pMobIndex->numattacks;
        mob->hitplus = pMobIndex->hitplus;

        if ( !pMobIndex->hitnodice ) {
            mob->hit = mob->hitplus;
            mob->max_hit = mob->hitplus;
        }
        else {
            mob->hit = pMobIndex->hitplus;
            mob->max_hit = pMobIndex->hitplus;
        }
    }

    /*
     * lets put things back the way they used to be! -Thoric
     */
/*
   if ( !IN_WILDERNESS( mob ) &&  !IN_RIFT( mob ))
   {
   if ( mob->level > 2 && mob->level < 20 )
   {
   GET_MONEY ( mob, CURR_BRONZE ) += number_range( 1, 20 );
   }
   else if ( mob->level > 19 && mob->level < 60 )
   {
   GET_MONEY ( mob, CURR_SILVER ) += number_range( 1, 20 );
   }
   else if (  mob->level > 59 && mob->level < 100 )
   {
   GET_MONEY ( mob, CURR_GOLD ) += number_range( 1, 20 );
   }
   }
   else if ( IN_WILDERNESS( mob ) &&  IN_RIFT( mob ))
   {
   GET_MONEY ( mob, CURR_COPPER ) += number_range( 1, 5 );
   }
*/
    mob->exp = pMobIndex->exp;

// Volk - easier than this..
    if ( pMobIndex->defposition == POS_DEFENSIVE || pMobIndex->position == POS_DEFENSIVE ) {
        pMobIndex->defposition = POS_STANDING;
        pMobIndex->position = POS_STANDING;
    }

    mob->position = pMobIndex->position;
    mob->defposition = pMobIndex->defposition;

    mob->barenumdie = pMobIndex->damnodice;
    mob->baresizedie = pMobIndex->damsizedice;
    mob->mobthac0 = pMobIndex->mobthac0;
    mob->damplus = pMobIndex->damplus;

    mob->perm_str = pMobIndex->perm_str;
    mob->perm_dex = pMobIndex->perm_dex;
    mob->perm_wis = pMobIndex->perm_wis;
    mob->perm_int = pMobIndex->perm_int;
    mob->perm_con = pMobIndex->perm_con;
    mob->perm_cha = pMobIndex->perm_cha;
    mob->perm_lck = pMobIndex->perm_lck;

    mob->race = pMobIndex->race;
    mob->Class = pMobIndex->Class;
    mob->xflags = pMobIndex->xflags;
    mob->saving_poison_death = pMobIndex->saving_poison_death;
    mob->saving_wand = pMobIndex->saving_wand;
    mob->saving_para_petri = pMobIndex->saving_para_petri;
    mob->saving_breath = pMobIndex->saving_breath;
    mob->saving_spell_staff = pMobIndex->saving_spell_staff;
    mob->height = pMobIndex->height;
    mob->weight = pMobIndex->weight;
    mob->resistant = pMobIndex->resistant;
    mob->immune = pMobIndex->immune;
    mob->susceptible = pMobIndex->susceptible;
    mob->attacks = pMobIndex->attacks;
    mob->defenses = pMobIndex->defenses;
    mob->speaks = pMobIndex->speaks;
    mob->speaking = pMobIndex->speaking;

    /*
     * Perhaps add this to the index later --Shaddai 
     */
    xCLEAR_BITS( mob->no_affected_by );
    mob->no_resistant = 0;
    mob->no_immune = 0;
    mob->no_susceptible = 0;

    /*
     * Insert in list. 
     */
    add_char( mob );
    pMobIndex->count++;
    nummobsloaded++;
    return mob;
}

/* Create an instance of an object. */
OBJ_DATA               *create_object( OBJ_INDEX_DATA *pObjIndex, int level )
{
    OBJ_DATA               *obj;

    if ( !pObjIndex ) {
        bug( "Create_object: NULL pObjIndex." );
        exit( 1 );
    }
    CREATE( obj, OBJ_DATA, 1 );

    obj->pIndexData = pObjIndex;
    obj->reset = NULL;
    obj->in_room = NULL;
    obj->level = level;
    obj->wear_loc = -1;
    obj->count = 1;
    cur_obj_serial = UMAX( ( cur_obj_serial + 1 ) & ( BV30 - 1 ), 1 );
    obj->serial = obj->pIndexData->serial = cur_obj_serial;
    obj->name = QUICKLINK( pObjIndex->name );
    obj->short_descr = QUICKLINK( pObjIndex->short_descr );
    obj->description = QUICKLINK( pObjIndex->description );
    obj->action_desc = QUICKLINK( pObjIndex->action_desc );
    obj->item_type = pObjIndex->item_type;
    obj->extra_flags = pObjIndex->extra_flags;
    obj->wear_flags = pObjIndex->wear_flags;
    obj->value[0] = pObjIndex->value[0];
    obj->value[1] = pObjIndex->value[1];
    obj->value[2] = pObjIndex->value[2];
    obj->value[3] = pObjIndex->value[3];
    obj->value[4] = pObjIndex->value[4];
    obj->value[5] = pObjIndex->value[5];
    obj->value[6] = pObjIndex->value[6];
    obj->weight = pObjIndex->weight;
    // Below is old code. -Taon

    obj->size = pObjIndex->size;
    obj->cost = pObjIndex->cost;
    obj->currtype = URANGE( FIRST_CURR, pObjIndex->currtype, LAST_CURR );

    // New object currency type amt handler. -Taon
    // Don't uncomment this it isn't done, it will crash us.
    // obj->currtype = set_curr_type(obj, obj->level);
    // obj->cost = set_curr_amt(obj, obj->level);

    if ( obj->value[5] > 0 ) {
        obj->level = obj->value[5];
    }

    obj->color = pObjIndex->color;
    /*
     * Mess with object properties. 
     */
    switch ( obj->item_type ) {
        default:
            bug( "Read_object: vnum %d bad type.", pObjIndex->vnum );
            bug( "------------------------>  %d ", obj->item_type );
            break;

        case ITEM_LIGHT:
        case ITEM_TREASURE:
        case ITEM_FURNITURE:
        case ITEM_TRASH:
        case ITEM_SHARPEN:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_KEY:
        case ITEM_KEYRING:
        case ITEM_PIECE:
        case ITEM_ODOR:
        case ITEM_RESOURCE:
        case ITEM_TOOL:
        case ITEM_SKELETON:
        case ITEM_DYE:
        case ITEM_CHANCE:
        case ITEM_STOVE:
        case ITEM_FORGE:
        case ITEM_COAL:
        case ITEM_SABOTAGE:
        case ITEM_INSTRUMENT:
            break;

        case ITEM_COOK:
        case ITEM_FOOD:
            /*
             * optional food condition (rotting food)  -Thoric
             * value1 is the max condition of the food
             * value4 is the optional initial condition
             */
            if ( obj->value[4] )
                obj->timer = obj->value[4];
            else
                obj->timer = obj->value[1];
            break;
        case ITEM_BOAT:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
        case ITEM_FOUNTAIN:
        case ITEM_BLOOD:
        case ITEM_BLOODSTAIN:
        case ITEM_SCRAPS:
        case ITEM_PIPE:
        case ITEM_HERB_CON:
        case ITEM_HERB:
        case ITEM_INCENSE:
        case ITEM_FIRE:
        case ITEM_BOOK:
        case ITEM_RAW:
        case ITEM_SWITCH:
        case ITEM_LEVER:
        case ITEM_PULLCHAIN:
        case ITEM_STONE:
        case ITEM_BUTTON:
        case ITEM_DIAL:
        case ITEM_RUNE:
        case ITEM_RUNEPOUCH:
        case ITEM_MATCH:
        case ITEM_TRAP:
        case ITEM_MAP:
        case ITEM_PORTAL:
        case ITEM_PAPER:
        case ITEM_PEN:
        case ITEM_TINDER:
        case ITEM_LOCKPICK:
        case ITEM_SPIKE:
        case ITEM_DISEASE:
        case ITEM_OIL:
        case ITEM_FUEL:
        case ITEM_QUIVER:
        case ITEM_SHOVEL:
            break;
        case ITEM_SALVE:
            obj->value[3] = number_fuzzy( obj->value[3] );
            break;

        case ITEM_SCROLL:
            obj->value[0] = number_fuzzy( obj->value[0] );
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            obj->value[0] = number_fuzzy( obj->value[0] );
            obj->value[1] = number_fuzzy( obj->value[1] );
            obj->value[2] = obj->value[1];
            break;

        case ITEM_MISSILE_WEAPON:
            if ( IS_OBJ_STAT( obj, ITEM_OVERRIDE ) ) {
                obj->value[1] = number_fuzzy( obj->value[1] );
                obj->value[2] = number_fuzzy( obj->value[2] );
            }
            else {
                obj->value[1] = min_archery_chart( obj->level );
                obj->value[2] = max_archery_chart( obj->level );
                obj->pIndexData->value[1] = min_archery_chart( obj->level );
                obj->pIndexData->value[2] = max_archery_chart( obj->level );
            }

            if ( obj->value[0] == 0 )
                obj->value[0] = INIT_WEAPON_CONDITION;
            break;
        case ITEM_WEAPON:
        case ITEM_PROJECTILE:

            if ( IS_OBJ_STAT( obj, ITEM_OVERRIDE ) ) {
                obj->value[1] = number_fuzzy( obj->value[1] );
                obj->value[2] = number_fuzzy( obj->value[2] );
                if ( CAN_SHARPEN( obj ) )
                    obj->value[6] = number_fuzzy( obj->value[6] );
            }
            else if ( obj->value[4] != 0 && obj->value[4] != 4 && obj->value[4] != 8 ) {
                obj->value[1] = set_min_chart( obj->level );
                obj->value[2] = set_max_chart( obj->level );
                obj->pIndexData->value[1] = set_min_chart( obj->level );
                obj->pIndexData->value[2] = set_max_chart( obj->level );
                if ( CAN_SHARPEN( obj ) && !obj->value[6] )
                    obj->value[6] = number_range( 1, 100 );
            }
            // how about more for two-handed weapons.
            else if ( obj->value[4] == 0 || obj->value[4] == 4 || obj->value[4] == 8 ) {
                short                   bonus;

                bonus = obj->level / 5;
                if ( bonus < 1 ) {
                    bonus = 1;
                }
                obj->value[1] =
                    set_min_chart( obj->level ) + bonus * 3 + set_min_chart( obj->level / 2 );
                obj->value[2] =
                    set_max_chart( obj->level ) + bonus + ( set_max_chart( obj->level ) / 2 );
                obj->pIndexData->value[1] =
                    set_min_chart( obj->level ) + bonus * 3 + set_min_chart( obj->level / 2 );
                obj->pIndexData->value[2] =
                    set_max_chart( obj->level ) + bonus + ( set_max_chart( obj->level ) / 2 );
                obj->weight = 15;
                if ( CAN_SHARPEN( obj ) && !obj->value[6] )
                    obj->value[6] = number_range( 1, 100 );
            }
            if ( obj->value[0] == 0 )
                obj->value[0] = INIT_WEAPON_CONDITION;
            break;
        case ITEM_ARMOR:
            if ( !IS_OBJ_STAT( obj, ITEM_OVERRIDE ) ) {
                obj->value[0] = set_min_armor( obj->level );
                obj->value[1] = set_max_armor( obj->level );
                obj->pIndexData->value[0] = set_min_armor( obj->level );
                obj->pIndexData->value[1] = set_max_armor( obj->level );
            }
            else {
                obj->value[0] = number_fuzzy( obj->value[0] );
                obj->value[1] = number_fuzzy( obj->value[1] );
            }
            break;
        case ITEM_PILL:
        case ITEM_POTION:
            break;
        case ITEM_MONEY:
            if ( obj->value[0] <= 0 )
                obj->value[0] = obj->cost;
            obj->value[2] = URANGE( FIRST_CURR, pObjIndex->value[2], LAST_CURR );
            break;
    }
    LINK( obj, first_object, last_object, next, prev );
    ++pObjIndex->count;
    ++numobjsloaded;
    ++physicalobjects;

    return obj;
}

/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA *ch )
{
    ch->hunting = NULL;
    ch->fearing = NULL;
    ch->hating = NULL;
    ch->name = NULL;
    ch->short_descr = NULL;
    ch->long_descr = NULL;
    ch->description = NULL;
    ch->next = NULL;
    ch->prev = NULL;
    ch->reply = NULL;
    ch->retell = NULL;
    ch->variables = NULL;
    ch->first_carrying = NULL;
    ch->last_carrying = NULL;
    ch->next_in_room = NULL;
    ch->prev_in_room = NULL;
    ch->fighting = NULL;
    ch->switched = NULL;
    ch->first_affect = NULL;
    ch->last_affect = NULL;
    ch->last_cmd = NULL;
    ch->alloc_ptr = NULL;
    ch->mount = NULL;
    ch->morph = NULL;
    xCLEAR_BITS( ch->affected_by );
    ch->logon = current_time;
    ch->armor = 100;
    ch->position = POS_STANDING;
    ch->practice = 0;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->height = 72;
    ch->weight = 180;
    xCLEAR_BITS( ch->xflags );
    ch->race = 0;
    ch->Class = 4;
    ch->speaking = LANG_COMMON;
    ch->speaks = LANG_COMMON;
    ch->barenumdie = 1;
    ch->baresizedie = 4;
    ch->substate = 0;
    ch->tempnum = 0;
    ch->perm_str = 13;
    ch->perm_dex = 13;
    ch->perm_int = 13;
    ch->perm_wis = 13;
    ch->perm_cha = 13;
    ch->perm_con = 13;
    ch->perm_lck = 13;
    ch->mod_str = 0;
    ch->mod_dex = 0;
    ch->mod_int = 0;
    ch->mod_wis = 0;
    ch->mod_cha = 0;
    ch->mod_con = 0;
    ch->mod_lck = 0;
}

/* Free a character. */
void free_char( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    AFFECT_DATA            *paf;
    TIMER                  *timer;
    MPROG_ACT_LIST         *mpact,
                           *mpact_next;
    NOTE_DATA              *comments,
                           *comments_next;
    VARIABLE_DATA          *vd,
                           *vd_next;
    bool                    updateGMB = FALSE;

    if ( !ch ) {
        bug( "%s Free_char: null ch!", __FUNCTION__ );
        return;
    }

    if ( greendragon && ch == greendragon )
        greendragon = NULL;

    if ( ch->desc )
        bug( "%s Free_char: char still has descriptor.", __FUNCTION__ );
    if ( ch->morph )
        free_char_morph( ch->morph );
    while ( ( obj = ch->last_carrying ) != NULL )
        extract_obj( obj );
    while ( ( paf = ch->last_affect ) != NULL )
        affect_remove( ch, paf );
    while ( ( timer = ch->first_timer ) != NULL )
        extract_timer( ch, timer );
    if ( VLD_STR( ch->name ) )
        STRFREE( ch->name );
    if ( VLD_STR( ch->short_descr ) )
        STRFREE( ch->short_descr );
    if ( VLD_STR( ch->long_descr ) )
        STRFREE( ch->long_descr );                     // Is this suppose to be a
    if ( VLD_STR( ch->clanname ) )
        STRFREE( ch->clanname );
    if ( VLD_STR( ch->description ) )
        STRFREE( ch->description );
    if ( ch->landmark )
        STRFREE( ch->landmark );
    stop_hunting( ch );
    stop_hating( ch );
    stop_fearing( ch );
    free_fight( ch );
    if ( ch->challenged ) {
        char                    qbuf[MSL];

        ch->challenged->challenge = NULL;
        ch->challenged = NULL;
        challenge = FALSE;
    }
    if ( ch->challenge ) {
        char                    qbuf[MSL];

        ch->challenge->challenged = NULL;
        ch->challenge = NULL;
        challenge = FALSE;
    }
    for ( vd = ch->variables; vd; vd = vd_next ) {
        vd_next = vd->next;
        delete_variable( vd );
    }

    if ( ch->pcdata ) {
        IGNORE_DATA            *temp,
                               *next;

        updateGMB = TRUE;
        if ( VLD_STR( ch->pcdata->sqlpass ) )
            STRFREE( ch->pcdata->sqlpass );

        if ( ch->pcdata->editor )
            stop_editing( ch );

        if ( ch->pcdata->pet ) {
            extract_char( ch->pcdata->pet, TRUE );
            ch->pcdata->pet = NULL;
        }

        /*
         * free up memory allocated to stored ignored names 
         */
        for ( temp = ch->pcdata->first_ignored; temp; temp = next ) {
            next = temp->next;
            UNLINK( temp, ch->pcdata->first_ignored, ch->pcdata->last_ignored, next, prev );
            STRFREE( temp->name );
            DISPOSE( temp );
        }
        free_all_pkills( ch );
        if ( VLD_STR( ch->pcdata->authexp ) )
            STRFREE( ch->pcdata->authexp );
        if ( VLD_STR( ch->pcdata->filename ) )
            STRFREE( ch->pcdata->filename );
        if ( VLD_STR( ch->pcdata->clan_name ) )
            STRFREE( ch->pcdata->clan_name );
        if ( VLD_STR( ch->pcdata->deity_name ) )
            STRFREE( ch->pcdata->deity_name );
        if ( VLD_STR( ch->pcdata->council_name ) )
            STRFREE( ch->pcdata->council_name );
        if ( VLD_STR( ch->pcdata->htown_name ) )
            STRFREE( ch->pcdata->htown_name );
        if ( VLD_STR( ch->pcdata->city_name ) )
            STRFREE( ch->pcdata->city_name );
        if ( VLD_STR( ch->pcdata->pwd ) )
            STRFREE( ch->pcdata->pwd );                /* no hash */
        if ( VLD_STR( ch->pcdata->bamfin ) )
            STRFREE( ch->pcdata->bamfin );             /* no hash */
        if ( VLD_STR( ch->pcdata->bamfout ) )
            STRFREE( ch->pcdata->bamfout );            /* no hash */
        if ( VLD_STR( ch->pcdata->rank ) )
            STRFREE( ch->pcdata->rank );
        if ( VLD_STR( ch->pcdata->title ) )
            STRFREE( ch->pcdata->title );
        if ( VLD_STR( ch->pcdata->bio ) )
            STRFREE( ch->pcdata->bio );
        if ( VLD_STR( ch->pcdata->bestowments ) )
            STRFREE( ch->pcdata->bestowments );        /* no hash */
        if ( VLD_STR( ch->pcdata->homepage ) )
            STRFREE( ch->pcdata->homepage );           /* no hash */
        if ( VLD_STR( ch->pcdata->authed_by ) )
            STRFREE( ch->pcdata->authed_by );
        if ( VLD_STR( ch->pcdata->prompt ) )
            STRFREE( ch->pcdata->prompt );
        if ( VLD_STR( ch->pcdata->fprompt ) )
            STRFREE( ch->pcdata->fprompt );
        if ( VLD_STR( ch->pcdata->email ) )
            STRFREE( ch->pcdata->email );              /* no hash */
        if ( VLD_STR( ch->pcdata->vamp_status ) )
            STRFREE( ch->pcdata->vamp_status );        /* hash */
//    if(VLD_STR(ch->pcdata->trade))
//      STRFREE(ch->pcdata->trade); /* hash */
        if ( ch->pcdata->helled_by )
            STRFREE( ch->pcdata->helled_by );
        if ( ch->pcdata->alt1 )
            STRFREE( ch->pcdata->alt1 );
        if ( ch->pcdata->alt2 )
            STRFREE( ch->pcdata->alt2 );
        if ( ch->pcdata->alt3 )
            STRFREE( ch->pcdata->alt3 );
        if ( ch->pcdata->alt4 )
            STRFREE( ch->pcdata->alt4 );
        if ( ch->pcdata->alt5 )
            STRFREE( ch->pcdata->alt5 );
        if ( ch->pcdata->rand )
            STRFREE( ch->pcdata->rand );
        /*
         * Remove found data 
         */
        {
            FOUND_AREA             *found,
                                   *fnext;

            for ( found = ch->pcdata->first_area; found; found = fnext ) {
                fnext = found->next;
                UNLINK( found, ch->pcdata->first_area, ch->pcdata->last_area, next, prev );
                STRFREE( found->area_name );
                DISPOSE( found );
            }
        }
        if ( ch->pcdata->subprompt )
            STRFREE( ch->pcdata->subprompt );
        if ( VLD_STR( ch->pcdata->chan_listen ) )
            STRFREE( ch->pcdata->chan_listen );
        free_aliases( ch );
        if ( ch->pcdata->say_history ) {
            int                     i;

            for ( i = 0; i < 20; i++ )
                if ( VLD_STR( ch->pcdata->say_history[i] ) )
                    STRFREE( ch->pcdata->say_history[i] );
        }
        if ( ch->pcdata->tell_history ) {
            int                     i;

            for ( i = 0; i < 20; i++ )
                if ( VLD_STR( ch->pcdata->tell_history[i] ) )
                    STRFREE( ch->pcdata->tell_history[i] );
        }
#ifdef I3
        i3_free_chardata( ch );
#endif

        DISPOSE( ch->pcdata );
    }
    for ( mpact = ch->mpact; mpact; mpact = mpact_next ) {
        mpact_next = mpact->next;
        DISPOSE( mpact->buf );
        DISPOSE( mpact );
    }
    for ( comments = ch->comments; comments; comments = comments_next ) {
        comments_next = comments->next;
        STRFREE( comments->text );
        STRFREE( comments->to_list );
        STRFREE( comments->subject );
        STRFREE( comments->sender );
        STRFREE( comments->date );
        DISPOSE( comments );
    }

    DISPOSE( ch );

    if ( updateGMB )
        get_curr_players(  );
}

/* Get an extra description from a list. */
char                   *get_extra_descr( const char *name, EXTRA_DESCR_DATA *ed )
{
    for ( ; ed; ed = ed->next )
        if ( is_name( name, ed->keyword ) )
            return ed->description;
    return NULL;
}

/* Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA         *get_mob_index( int vnum )
{
    MOB_INDEX_DATA         *pMobIndex;

    if ( vnum < 0 )
        vnum = 0;

    for ( pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH]; pMobIndex; pMobIndex = pMobIndex->next )
        if ( pMobIndex->vnum == vnum )
            return pMobIndex;

    if ( fBootDb )
        bug( "Get_mob_index: bad vnum %d.", vnum );

    return NULL;
}

/*
 * Translates obj virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA         *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA         *pObjIndex;

    if ( vnum < 0 )
        vnum = 0;

    for ( pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH]; pObjIndex; pObjIndex = pObjIndex->next )
        if ( pObjIndex->vnum == vnum )
            return pObjIndex;

    if ( fBootDb )
        bug( "Get_obj_index: bad vnum %d.", vnum );

    return NULL;
}

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA        *new_get_room_index( int vnum, const char *filename, int line )
{
    ROOM_INDEX_DATA        *pRoomIndex;

    if ( vnum < 0 )
        vnum = 0;

    for ( pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex;
          pRoomIndex = pRoomIndex->next )
        if ( pRoomIndex->vnum == vnum )
            return pRoomIndex;

    if ( fBootDb )
        bug( "%s: bad vnum %d. Called from file %s line %d", __FUNCTION__, vnum, filename, line );

    return NULL;
}

/*
 * Added lots of EOF checks, as most of the file crashes are based on them.
 * If an area file encounters EOF, the fread_* functions will shutdown the
 * MUD, as all area files should be read in in full or bad things will
 * happen during the game.  Any files loaded in without fBootDb which
 * encounter EOF will return what they have read so far.   These files
 * should include player files, and in-progress areas that are not loaded
 * upon bootup.
 * -- Altrag
 */

/*
 * Read a letter from a file.
 */
char fread_letter( FILE * fp )
{
    char                    c;

    do {
        if ( feof( fp ) ) {
            bug( "fread_letter: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return '\0';
        }
        c = getc( fp );
    }
    while ( isspace( c ) );

    return c;
}

/* Read a number from a file. */
int fread_number( FILE * fp )
{
    int                     number;
    bool                    sign;
    char                    c;

    do {
        if ( feof( fp ) ) {
            bug( "fread_number: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return 0;
        }
        c = getc( fp );
    }
    while ( isspace( c ) );

    number = 0;

    sign = FALSE;
    if ( c == '+' ) {
        c = getc( fp );
    }
    else if ( c == '-' ) {
        sign = TRUE;
        c = getc( fp );
    }

    if ( !isdigit( c ) ) {
        bug( "Fread_number: bad format. (%c)", c );
        if ( fBootDb )
            exit( 1 );
        return 0;
    }

    while ( isdigit( c ) ) {
        if ( feof( fp ) ) {
            bug( "fread_number: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return number;
        }
        number = number * 10 + c - '0';
        c = getc( fp );
    }

    if ( sign )
        number = 0 - number;

    if ( c == '|' )
        number += fread_number( fp );
    else if ( c != ' ' )
        ungetc( c, fp );

    return number;
}

/* Read a time from a file. */
time_t fread_time( FILE * fp )
{
    time_t                  number;
    bool                    sign;
    char                    c;

    do {
        if ( feof( fp ) ) {
            bug( "fread_time: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return 0;
        }
        c = getc( fp );
    }
    while ( isspace( c ) );

    number = 0;

    sign = FALSE;
    if ( c == '+' ) {
        c = getc( fp );
    }
    else if ( c == '-' ) {
        sign = TRUE;
        c = getc( fp );
    }

    if ( !isdigit( c ) ) {
        bug( "Fread_time: bad format. (%c)", c );
        if ( fBootDb )
            exit( 1 );
        return 0;
    }

    while ( isdigit( c ) ) {
        if ( feof( fp ) ) {
            bug( "fread_time: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return number;
        }
        number = number * 10 + c - '0';
        c = getc( fp );
    }

    if ( sign )
        number = 0 - number;

    if ( c == '|' )
        number += fread_number( fp );
    else if ( c != ' ' )
        ungetc( c, fp );

    return number;
}

/*
 * custom str_dup using create     -Thoric
 */
char                   *str_dup( char const *str )
{
    static char            *ret;
    int                     len;

    if ( !str )
        return NULL;

    len = strlen( str ) + 1;

    CREATE( ret, char, len );
    strcpy( ret, str );
    return ret;
}

/* Read a string from file and return it */
char                   *fread_flagstring( FILE * fp )
{
    static char             flagstring[MSL];
    char                   *plast;
    char                    c;
    int                     ln;

    plast = flagstring;
    flagstring[0] = '\0';
    ln = 0;
    /*
     * Skip blanks. Read first char. 
     */
    do {
        if ( feof( fp ) ) {
            bug( "fread_flagstring: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return ( char * ) "";
        }
        c = getc( fp );
    }
    while ( isspace( c ) );
    if ( ( *plast++ = c ) == '~' )
        return ( char * ) "";
    for ( ;; ) {
        if ( ln >= ( MSL - 1 ) ) {
            bug( "fread_flagstring: string too long" );
            *plast = '\0';
            return flagstring;
        }
        switch ( *plast = getc( fp ) ) {
            default:
                plast++;
                ln++;
                break;
            case EOF:
                bug( "Fread_flagstring: EOF" );
                if ( fBootDb )
                    exit( 1 );
                *plast = '\0';
                return flagstring;
                break;
            case '\n':
                plast++;
                ln++;
                *plast++ = '\r';
                ln++;
                break;
            case '\r':
                break;
            case '~':
                *plast = '\0';
                return flagstring;
        }
    }
}

/* Read a string from the file and return NULL if not a valid string else stralloc it */
char                   *fread_string( FILE * fp )
{
    char                    buf[MSL];

    snprintf( buf, MSL, "%s", fread_flagstring( fp ) );
    /*
     * Make sure its a valid string and not (null) 
     */
    if ( VLD_STR( buf ) && str_cmp( buf, "(null)" ) )
        return STRALLOC( buf );
    else
        return NULL;
}

/* Read a string from the file and return NULL if not a valid string else str_dup it */
char                   *fread_string_nohash( FILE * fp )
{
    char                    buf[MSL];

    snprintf( buf, MSL, "%s", fread_flagstring( fp ) );
    /*
     * Make sure its a valid string and not (null) 
     */
    if ( VLD_STR( buf ) && str_cmp( buf, "(null)" ) )
        return str_dup( buf );
    else
        return NULL;
}

/* Read to end of line (for comments). */
void fread_to_eol( FILE * fp )
{
    char                    c;

    do {
        if ( feof( fp ) ) {
            bug( "fread_to_eol: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            return;
        }
        c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );
    do
        c = getc( fp );
    while ( c == '\n' || c == '\r' );
    ungetc( c, fp );
    return;
}

/* Read to end of line into static buffer -Thoric */
char                   *fread_line( FILE * fp )
{
    static char             line[MSL];
    char                   *pline;
    char                    c;
    int                     ln;

    pline = line;
    line[0] = '\0';
    ln = 0;

    do {
        if ( feof( fp ) ) {
            bug( "fread_line: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            mudstrlcpy( line, "", MSL );
            return line;
        }
        c = getc( fp );
    }
    while ( isspace( c ) );
    ungetc( c, fp );
    do {
        if ( feof( fp ) ) {
            bug( "fread_line: EOF encountered on read.\r\n" );
            if ( fBootDb )
                exit( 1 );
            *pline = '\0';
            return line;
        }
        c = getc( fp );
        *pline++ = c;
        ln++;
        if ( ln >= ( MSL - 1 ) ) {
            bug( "fread_line: line too long" );
            break;
        }
    }
    while ( c != '\n' && c != '\r' );
    do
        c = getc( fp );
    while ( c == '\n' || c == '\r' );
    ungetc( c, fp );
    *pline = '\0';
    return line;
}

/* Read one word (into static buffer). */
char                   *fread_word( FILE * fp )
{
    static char             word[MIL];
    char                   *pword;
    char                    cEnd;

    do {
        if ( feof( fp ) ) {
            bug( "%s: EOF encountered on read.", __FUNCTION__ );
            if ( fBootDb )
                exit( 1 );
            word[0] = '\0';
            return word;
        }
        cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );
    if ( cEnd == '\'' || cEnd == '"' )
        pword = word;
    else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }
    for ( ; pword < word + MIL; pword++ ) {
        if ( feof( fp ) ) {
            bug( "%s: EOF encountered on read.", __FUNCTION__ );
            if ( fBootDb )
                exit( 1 );
            *pword = '\0';
            return word;
        }
        *pword = getc( fp );
        if ( cEnd == ' ' ? isspace( *pword ) : *pword == cEnd ) {
            if ( cEnd == ' ' )
                ungetc( *pword, fp );
            *pword = '\0';
            return word;
        }
    }
    bug( "Fread_word: word too long" );
    exit( 1 );
    return NULL;
}

void do_memory( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    int                     hash;

    set_char_color( AT_PLAIN, ch );
    argument = one_argument( argument, arg );
    send_to_char( "\r\n&wSystem Memory [arguments - hash, check, showhigh]\r\n", ch );
    ch_printf( ch, "&wAffects: &W%6d  &wAreas:   &W%6d\r\n", top_affect, top_area );
    ch_printf( ch, "&wExtDes:  &W%6d  &wExits:   &W%6d\r\n", top_ed, top_exit );
    ch_printf( ch, "&wHelps:   &W%6d  &wResets:  &W%6d\r\n", top_help, top_reset );
    ch_printf( ch, "&wIdxMobs: &W%6d  &wMobiles: &W%6d\r\n", top_mob_index, nummobsloaded );
    ch_printf( ch, "&wIdxObjs: &W%6d  &wObjs:    &W%6d(%d)\r\n", top_obj_index, numobjsloaded,
               physicalobjects );
    ch_printf( ch, "&wRooms:   &W%6d\r\n", top_room );
    ch_printf( ch, "&wShops:   &W%6d  &wRepShps: &W%6d\r\n", top_shop, top_repair );
    ch_printf( ch, "&wCurOq's: &W%6d  &wCurCq's: &W%6d\r\n", cur_qobjs, cur_qchars );
    ch_printf( ch, "&wPlayers: &W%6d  &wMaxplrs: &W%6d\r\n", num_descriptors, sysdata.maxplayers );
    ch_printf( ch, "&wMaxEver: &W%6d  &wTopsn:   &W%6d(%d)\r\n", sysdata.alltimemax, top_sn,
               MAX_SKILL );
    ch_printf( ch, "&wMaxEver was recorded on:  &W%s\r\n\r\n", sysdata.time_of_max );
    ch_printf( ch, "&wPotion Val:  &W%-16d   &wScribe/Brew: &W%d/%d\r\n", sysdata.upotion_val,
               sysdata.scribed_used, sysdata.brewed_used );
    ch_printf( ch, "&wPill Val:    &W%-16d\r\n", sysdata.upill_val );
    ch_printf( ch, "Hash statistics:\r\n%s", hash_stats(  ) );

    if ( !str_cmp( arg, "check" ) ) {
#ifdef HASHSTR
        send_to_char( check_hash( argument ), ch );
#else
        send_to_char( "Hash strings not enabled.\r\n", ch );
#endif
        return;
    }
    if ( !str_cmp( arg, "showhigh" ) ) {
#ifdef HASHSTR
        show_high_hash( atoi( argument ) );
#else
        send_to_char( "Hash strings not enabled.\r\n", ch );
#endif
        return;
    }
    if ( argument[0] != '\0' )
        hash = atoi( argument );
    else
        hash = -1;
    if ( !str_cmp( arg, "hash" ) ) {
#ifdef HASHSTR
        if ( hash != -1 )
            hash_dump( hash );
#else
        send_to_char( "Hash strings not enabled.\r\n", ch );
#endif
    }
    return;
}

/* Stick a little fuzz on a number. */
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) ) {
        case 0:
            number -= 1;
            break;
        case 3:
            number += 1;
            break;
    }
    return UMAX( 1, number );
}

/* Generate a random number. */
int number_range( int from, int to )
{
    if ( ( to - from ) < 1 )
        return from;
    return ( ( number_mm(  ) % ( to - from + 1 ) ) + from );
}

//Number_range but for short ints. -Taon
short number_chance( short low, short high )
{
    if ( ( high - low ) < 1 )
        return low;

    return ( ( number_mm(  ) % ( high - low + 1 ) ) + low );
}

/* Generate a percentile roll. */
int number_percent( void )
{
    return ( number_mm(  ) % 100 ) + 1;
}

/* Generate a random door. */
int number_door( void )
{
    int                     door;

    while ( ( door = number_mm(  ) & ( 16 - 1 ) ) > 9 );
    return door;
}

int number_bits( int width )
{
    return number_mm(  ) & ( ( 1 << width ) - 1 );
}

/* I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int              rgiState[2 + 55];

void init_mm(  )
{
    int                    *piState;
    int                     iState;

    piState = &rgiState[2];
    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;
    piState[0] = ( ( int ) current_time ) & ( ( 1 << 30 ) - 1 );
    piState[1] = 1;
    for ( iState = 2; iState < 55; iState++ ) {
        piState[iState] = ( piState[iState - 1] + piState[iState - 2] ) & ( ( 1 << 30 ) - 1 );
    }
    return;
}

int number_mm( void )
{
    int                    *piState;
    int                     iState1;
    int                     iState2;
    int                     iRand;

    piState = &rgiState[2];
    iState1 = piState[-2];
    iState2 = piState[-1];
    iRand = ( piState[iState1] + piState[iState2] ) & ( ( 1 << 30 ) - 1 );
    piState[iState1] = iRand;
    if ( ++iState1 == 55 )
        iState1 = 0;
    if ( ++iState2 == 55 )
        iState2 = 0;
    piState[-2] = iState1;
    piState[-1] = iState2;
    return iRand >> 6;
}

/* Roll some dice. -Thoric */
int dice( int number, int size )
{
    int                     idice;
    int                     sum;

    switch ( size ) {
        case 0:
            return 0;
        case 1:
            return number;
    }
    for ( idice = 0, sum = 0; idice < number; idice++ )
        sum += number_range( 1, size );
    return sum;
}

/* Simple linear interpolation. */
int interpolate( int level, int value_00, int value_32 )
{
    return value_00 + level * ( value_32 - value_00 ) / 32;
}

/* Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
        if ( *str == '~' )
            *str = '-';
    return;
}

/* Encodes the tildes in a string.     -Thoric
 * Used for player-entered strings that go into disk files.
 */
void hide_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
        if ( *str == '~' )
            *str = HIDDEN_TILDE;
    return;
}

char                   *show_tilde( char *str )
{
    static char             buf[MSL];
    char                   *bufptr;

    bufptr = buf;
    for ( ; *str != '\0'; str++, bufptr++ ) {
        if ( *str == HIDDEN_TILDE )
            *bufptr = '~';
        else
            *bufptr = *str;
    }
    *bufptr = '\0';
    return buf;
}

/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool new_str_cmp( const char *astr, const char *bstr, const char *filename, int line )
{
    /*
     * If both are null they are equal no? 
     */
    if ( !astr && !bstr )
        return FALSE;

    if ( !astr ) {
        bug( "%s: NULL astr [%s] bstr (File: %s Line: %d).", __FUNCTION__, !bstr ? "NULL" : bstr,
             filename, line );
        return TRUE;
    }
    if ( !bstr ) {
        bug( "%s: [%s] astr NULL bstr (File: %s Line: %d).", __FUNCTION__, !astr ? "NULL" : astr,
             filename, line );
        return TRUE;
    }
    for ( ; *astr || *bstr; astr++, bstr++ ) {
        if ( LOWER( *astr ) != LOWER( *bstr ) )
            return TRUE;
    }
    return FALSE;
}

/* Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
    if ( !astr ) {
        bug( "%s: NULL astr [%s] bstr.", __FUNCTION__, !bstr ? "NULL" : bstr );
        return TRUE;
    }
    if ( !bstr ) {
        bug( "%s: NULL bstr [%s] astr.", __FUNCTION__, !astr ? "NULL" : astr );
        return TRUE;
    }
    for ( ; *astr; astr++, bstr++ ) {
        if ( LOWER( *astr ) != LOWER( *bstr ) )
            return TRUE;
    }
    return FALSE;
}

/* Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
    int                     sstr1;
    int                     sstr2;
    int                     ichar;
    char                    c0;

    if ( ( c0 = LOWER( astr[0] ) ) == '\0' )
        return FALSE;
    sstr1 = strlen( astr );
    sstr2 = strlen( bstr );
    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
        if ( c0 == LOWER( bstr[ichar] ) && !str_prefix( astr, bstr + ichar ) )
            return FALSE;
    return TRUE;
}

/* Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
    int                     sstr1;
    int                     sstr2;

    sstr1 = strlen( astr );
    sstr2 = strlen( bstr );
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
        return FALSE;
    else
        return TRUE;
}

/* Returns an initial-capped string. */
char                   *capitalize( const char *str )
{
    static char             strcap[MSL];
    int                     i;

    if ( !str )
        return ( char * ) "";
    for ( i = 0; str[i] != '\0'; i++ )
        strcap[i] = LOWER( str[i] );
    strcap[i] = '\0';
    strcap[0] = UPPER( strcap[0] );
    return strcap;
}

/* Returns a lowercase string. */
char                   *strlower( const char *str )
{
    static char             strlow[MSL];
    int                     i;

    if ( !str || !str[0] )
        return ( char * ) "";
    for ( i = 0; str[i] != '\0'; i++ )
        strlow[i] = LOWER( str[i] );
    strlow[i] = '\0';
    return strlow;
}

/* Returns an uppercase string. */
char                   *strupper( const char *str )
{
    static char             strup[MSL];
    int                     i;

    if ( !str || !str[0] )
        return ( char * ) "";
    for ( i = 0; str[i] != '\0'; i++ )
        strup[i] = UPPER( str[i] );
    strup[i] = '\0';
    return strup;
}

/* Returns TRUE or FALSE if a letter is a vowel   -Thoric */
bool isavowel( char letter )
{
    char                    c;

    c = LOWER( letter );
    if ( c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' )
        return TRUE;
    else
        return FALSE;
}

/* Shove either "a " or "an " onto the beginning of a string  -Thoric */
char                   *aoran( const char *str )
{
    static char             temp[MSL];

    if ( !str ) {
        bug( "Aoran(): NULL str" );
        return ( char * ) "";
    }
    if ( isavowel( str[0] )
         || ( strlen( str ) > 1 && LOWER( str[0] ) == 'y' && !isavowel( str[1] ) ) )
        mudstrlcpy( temp, "an ", MSL );
    else
        mudstrlcpy( temp, "a ", MSL );
    mudstrlcat( temp, str, MSL );
    return temp;
}

// Simple function to add a single letter to the end of something.
void add_letter( char *string, char letter )
{
    char                    buf[MAX_STRING_LENGTH];

    sprintf( buf, "%c", letter );
    mudstrlcat( string, buf, MAX_STRING_LENGTH );
    return;
}

const char             *format( const char *fmt, ... )
{
    static char             newstring[MIL];
    char                    buf[MSL * 2];              /* better safe than sorry */
    va_list                 args;

    newstring[0] = '\0';

    if ( fmt[0] == '\0' )
        return " ";

    va_start( args, fmt );
    vsnprintf( buf, MSL * 2, fmt, args );
    va_end( args );

    if ( buf[0] == '\0' )
        return " ";
    mudstrlcpy( newstring, buf, MIL );
    return newstring;
}

/* Append a string to a file. */
void append_file( CHAR_DATA *ch, const char *file, const char *str )
{
    FILE                   *fp;

    if ( IS_NPC( ch ) || str[0] == '\0' )
        return;
    if ( ( fp = FileOpen( file, "a" ) ) == NULL ) {
        perror( file );
        send_to_char( "Could not open the file!\r\n", ch );
    }
    else {
        fprintf( fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
        FileClose( fp );
    }
    return;
}

/* Append a string to a file. */
void append_to_file( const char *file, const char *str )
{
    FILE                   *fp;

    if ( ( fp = FileOpen( file, "a" ) ) == NULL )
        perror( file );
    else {
        fprintf( fp, "%s\n", str );
        FileClose( fp );
    }
    return;
}

void append_to_file_printf( const char *file, const char *fmt, ... )
{
    char                    buf[MSL * 2];
    va_list                 args;

    va_start( args, fmt );
    vsnprintf( buf, MSL * 2, fmt, args );
    va_end( args );
    append_to_file( file, buf );
    return;
}

void progbug( CHAR_DATA *mob, const char *str, ... )
{
    char                    buf[MSL * 2];
    int                     vnum = mob->pIndexData ? mob->pIndexData->vnum : 0;

    {
        va_list                 param;

        va_start( param, str );
        vsnprintf( buf, sizeof( buf ), str, param );
        va_end( param );
    }

    if ( mob == supermob )
        bug( "%s: %s.", !mob->description ? "(Unknown)" : strip_cr( mob->description ), buf );
    else
        bug( "Mob #%d: %s.", vnum, buf );
}

/* Lop replacement function suggested by Remcon for this loop problem */
void bug( const char *str, ... )
{
    char                    buf[MSL];
    FILE                   *fp;
    int                     letter;
    struct stat             fst;

    mudstrlcpy( buf, "[*****] BUG: ", sizeof( buf ) );
    {
        va_list                 param;

        va_start( param, str );
        vsnprintf( buf + strlen( buf ), ( sizeof( buf ) - strlen( buf ) ), str, param );
        va_end( param );
    }
    log_string( buf );

    /*
     * Think we should have a fast way to see the bugs online also 
     */
    if ( ( fp = FileOpen( BUG_FILE, "a" ) ) ) {
        fprintf( fp, "%s &D\n", buf );
        FileClose( fp );
        fp = NULL;
    }

    if ( fpArea ) {
        int                     iLine;
        int                     iChar;

        if ( fpArea == stdin ) {
            iLine = 0;
        }
        else {
            iChar = ftell( fpArea );
            fseek( fpArea, 0, 0 );
            for ( iLine = 0; ftell( fpArea ) < iChar; iLine++ ) {
                while ( ( letter = getc( fpArea ) ) && letter != EOF && letter != '\n' );
            }
            fseek( fpArea, iChar, 0 );
        }

        snprintf( buf, MSL, "[*****] FILE: %s LINE: %d", strArea, iLine );
        log_string( buf );
        if ( stat( SHUTDOWN_FILE, &fst ) != -1 ) {
            if ( ( fp = FileOpen( SHUTDOWN_FILE, "a" ) ) ) {
                fprintf( fp, "[*****] <FILE: %s LINE: %d> %s\n", strArea, iLine, buf );
                FileClose( fp );
                fp = NULL;
            }
        }
    }
}

/*
 * Add a string to the boot-up log     -Thoric
 */
void boot_log( const char *str, ... )
{
    char                    buf[MSL];
    FILE                   *fp;
    va_list                 param;

    mudstrlcpy( buf, "[*****] BOOT: ", MSL );
    va_start( param, str );
    vsnprintf( buf + strlen( buf ), MSL - strlen( buf ), str, param );
    va_end( param );
    log_string( buf );

    if ( ( fp = FileOpen( BOOTLOG_FILE, "a" ) ) != NULL ) {
        fprintf( fp, "%s\n", buf );
        FileClose( fp );
    }

    return;
}

/* Dump a text file to a player, a line at a time  -Thoric */
void show_file( CHAR_DATA *ch, const char *filename )
{
    FILE                   *fp;
    char                    buf[MSL];
    int                     c;
    int                     num = 0;

    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {
        while ( !feof( fp ) ) {
            while ( ( buf[num] = fgetc( fp ) ) != EOF && buf[num] != '\n' && buf[num] != '\r'
                    && num < ( MSL - 2 ) )
                num++;
            c = fgetc( fp );
            if ( ( c != '\n' && c != '\r' ) || c == buf[num] )
                ungetc( c, fp );
            buf[num++] = '\n';
            buf[num++] = '\r';
            buf[num] = '\0';
            send_to_pager_color( buf, ch );
            num = 0;
        }
        /*
         * Thanks to stu <sprice@ihug.co.nz> from the mailing list in pointing
         * *  This out. 
         */
        FileClose( fp );
    }
}

/* Show the boot log file -Thoric */
void do_dmesg( CHAR_DATA *ch, char *argument )
{
    set_pager_color( AT_LOG, ch );
    show_file( ch, BOOTLOG_FILE );
}

void log_string( const char *fmt, ... )
{
    char                    buf[2 * MSL];
    va_list                 args;

    va_start( args, fmt );
    vsnprintf( buf, 2 * MSL, fmt, args );
    va_end( args );
    log_string_plus( buf, LOG_NORMAL, LEVEL_IMMORTAL );
    return;
}

/* Writes a string to the log, extended version -Thoric */
void log_string_plus( const char *str, short log_type, short level )
{
    char                   *strtime;
    int                     offset;
    struct timeval          now_time;
    time_t                  log_time;

    gettimeofday( &now_time, NULL );
    log_time = ( time_t ) now_time.tv_sec;
    strtime = ctime( &log_time );
    strtime[strlen( strtime ) - 1] = '\0';
    fprintf( stderr, "%s :: %s\n", strtime, str );
    if ( strncmp( str, "Log ", 4 ) == 0 )
        offset = 4;
    else
        offset = 0;
    switch ( log_type ) {
        default:
            to_channel( str + offset, "Log", level );
            break;
        case LOG_BUILD:
            to_channel( str + offset, "Build", level );
            break;
        case LOG_COMM:
            to_channel( str + offset, "Comm", level );
            break;
        case LOG_WARN:
            to_channel( str + offset, "Warn", level );
            break;
        case LOG_ALL:
            break;
    }
    return;
}

/* stafflist builder! -Thoric */
void tostafffile( const char *line )
{
    int                     filler,
                            xx;
    char                    outline[MSL];
    FILE                   *wfp;

    outline[0] = '\0';

    if ( line && line[0] != '\0' ) {
        filler = ( 78 - strlen( line ) );
        if ( filler < 1 )
            filler = 1;
        filler /= 2;
        for ( xx = 0; xx < filler; xx++ )
            mudstrlcat( outline, " ", MSL );
        mudstrlcat( outline, line, MSL );
    }
    mudstrlcat( outline, "\r\n", MSL );
    wfp = FileOpen( STAFFLIST_FILE, "a" );
    if ( wfp ) {
        fputs( outline, wfp );
        FileClose( wfp );
    }
}

void add_to_stafflist( char *name, int level )
{
    STAFFENT               *staff,
                           *tmp;

#ifdef DEBUG
    log_string( "Adding to 6 Dragons stafflist..." );
#endif

    CREATE( staff, STAFFENT, 1 );
    staff->name = STRALLOC( name );
    staff->level = level;

    if ( !first_staff ) {
        staff->last = NULL;
        staff->next = NULL;
        first_staff = staff;
        last_staff = staff;
        return;
    }

    /*
     * insert sort, of sorts 
     */
    for ( tmp = first_staff; tmp; tmp = tmp->next )
        if ( level > tmp->level ) {
            if ( !tmp->last )
                first_staff = staff;
            else
                tmp->last->next = staff;
            staff->last = tmp->last;
            staff->next = tmp;
            tmp->last = staff;
            return;
        }

    staff->last = last_staff;
    staff->next = NULL;
    last_staff->next = staff;
    last_staff = staff;
    return;
}

/* Stafflist builder -Thoric */
void make_stafflist(  )
{
    DIR                    *dp;
    struct dirent          *dentry;
    FILE                   *gfp;
    const char             *word;
    int                     ilevel,
                            iflags;
    STAFFENT               *staff,
                           *staffnext;
    char                    buf[MSL];

    first_staff = NULL;
    last_staff = NULL;
    dp = opendir( STAFF_DIR );
    ilevel = 0;
    dentry = readdir( dp );
    while ( dentry ) {
        if ( dentry->d_name[0] != '.' ) {
            snprintf( buf, MSL, "%s%s", STAFF_DIR, dentry->d_name );
            gfp = FileOpen( buf, "r" );
            if ( gfp ) {
                word = feof( gfp ) ? "End" : fread_word( gfp );
                ilevel = fread_number( gfp );
                fread_to_eol( gfp );
                word = feof( gfp ) ? "End" : fread_word( gfp );
                if ( !str_cmp( word, "Pcflags" ) )
                    iflags = fread_number( gfp );
                else
                    iflags = 0;
                FileClose( gfp );
                if ( IS_SET( iflags, PCFLAG_RETIRED ) )
                    ilevel = MAX_LEVEL - 10;
                if ( IS_SET( iflags, PCFLAG_GUEST ) )
                    ilevel = MAX_LEVEL - 9;
                if ( IS_SET( iflags, PCFLAG_TEMP ) )
                    ilevel = MAX_LEVEL - 8;
                add_to_stafflist( dentry->d_name, ilevel );
            }
        }
        dentry = readdir( dp );
    }
    closedir( dp );
    unlink( STAFFLIST_FILE );
    snprintf( buf, MSL, "&C The Staff for %s!", sysdata.mud_name );
    tostafffile( buf );
    buf[0] = '\0';
    ilevel = 65535;
    for ( staff = first_staff; staff; staff = staff->next ) {
        if ( staff->level < ilevel ) {
            if ( buf[0] ) {
                tostafffile( buf );
                buf[0] = '\0';
            }
            tostafffile( "" );
            ilevel = staff->level;

            switch ( ilevel ) {
                case MAX_LEVEL:
                    tostafffile( "      &CGAME ADMIN&c" );
                    break;
                case LEVEL_AJ_COLONEL:
                    tostafffile( "      &CGAME CODER&c" );
                    break;
                case LEVEL_AJ_MAJOR:
                    tostafffile( "      &CGAME COORIDNATOR&c" );
                    break;
                case LEVEL_AJ_CAPTAIN:
                    tostafffile( "      &CGAME COUNCIL HEAD&c" );
                    break;
                case LEVEL_AJ_LT:
                    tostafffile( "      &CGAME COUNCIL HEAD&c" );
                    break;
                case LEVEL_AJ_SGT:
                    tostafffile( "      &CGAME COUNCIL ASSIST&c" );
                    break;
                case LEVEL_AJ_CPL:
                    tostafffile( "      &CGAME LIASON&c" );
                    break;
                case LEVEL_IMMORTAL:
                    tostafffile( "      &CGAME AMBASSADOR&c" );
                    break;
                case MAX_LEVEL - 8:
                    tostafffile( "      &YTEMPORARY&P" );
                    break;
                case MAX_LEVEL - 9:
                    tostafffile( "      &r6 Dragons Guest&P" );
                    break;
                case MAX_LEVEL - 10:
                    tostafffile( "      &C6 Dragons Retired&c" );
                    break;
                default:
                    tostafffile( "      &WServants&P" );
                    break;
            }
        }

        if ( strlen( buf ) + strlen( staff->name ) > 76 ) {
            tostafffile( buf );
            buf[0] = '\0';
        }
        mudstrlcat( buf, " ", MSL );
        mudstrlcat( buf, staff->name, MSL );
        if ( strlen( buf ) > 70 ) {
            tostafffile( buf );
            buf[0] = '\0';
        }
    }
    if ( buf[0] )
        tostafffile( buf );
    for ( staff = first_staff; staff; staff = staffnext ) {
        staffnext = staff->next;
        STRFREE( staff->name );
        DISPOSE( staff );
    }
    first_staff = NULL;
    last_staff = NULL;
}

void do_makestafflist( CHAR_DATA *ch, char *argument )
{
    make_stafflist(  );
    build_wizinfo( FALSE );
}

/* mud prog functions */
/* This routine reads in scripts of MUDprograms from a file */
int mprog_name_to_type( char *name )
{
    if ( !str_cmp( name, "in_file_prog" ) )
        return IN_FILE_PROG;
    if ( !str_cmp( name, "act_prog" ) )
        return ACT_PROG;
    if ( !str_cmp( name, "speech_prog" ) )
        return SPEECH_PROG;
    if ( !str_cmp( name, "rand_prog" ) )
        return RAND_PROG;
    if ( !str_cmp( name, "fight_prog" ) )
        return FIGHT_PROG;
    if ( !str_cmp( name, "hitprcnt_prog" ) )
        return HITPRCNT_PROG;
    if ( !str_cmp( name, "death_prog" ) )
        return DEATH_PROG;
    if ( !str_cmp( name, "entry_prog" ) )
        return ENTRY_PROG;
    if ( !str_cmp( name, "greet_prog" ) )
        return GREET_PROG;
    if ( !str_cmp( name, "all_greet_prog" ) )
        return ALL_GREET_PROG;
    if ( !str_cmp( name, "give_prog" ) )
        return GIVE_PROG;
    if ( !str_cmp( name, "bribe_prog" ) )
        return BRIBE_PROG;
    if ( !str_cmp( name, "time_prog" ) )
        return TIME_PROG;
    if ( !str_cmp( name, "hour_prog" ) )
        return HOUR_PROG;
    if ( !str_cmp( name, "wear_prog" ) )
        return WEAR_PROG;
    if ( !str_cmp( name, "remove_prog" ) )
        return REMOVE_PROG;
    if ( !str_cmp( name, "trash_prog" ) )
        return TRASH_PROG;
    if ( !str_cmp( name, "look_prog" ) )
        return LOOK_PROG;
    if ( !str_cmp( name, "exa_prog" ) )
        return EXA_PROG;
    if ( !str_cmp( name, "zap_prog" ) )
        return ZAP_PROG;
    if ( !str_cmp( name, "get_prog" ) )
        return GET_PROG;
    if ( !str_cmp( name, "drop_prog" ) )
        return DROP_PROG;
    if ( !str_cmp( name, "damage_prog" ) )
        return DAMAGE_PROG;
    if ( !str_cmp( name, "repair_prog" ) )
        return REPAIR_PROG;
    if ( !str_cmp( name, "greet_prog" ) )
        return GREET_PROG;
    if ( !str_cmp( name, "randiw_prog" ) )
        return RANDIW_PROG;
    if ( !str_cmp( name, "speechiw_prog" ) )
        return SPEECHIW_PROG;
    if ( !str_cmp( name, "pull_prog" ) )
        return PULL_PROG;
    if ( !str_cmp( name, "push_prog" ) )
        return PUSH_PROG;
    if ( !str_cmp( name, "sleep_prog" ) )
        return SLEEP_PROG;
    if ( !str_cmp( name, "rest_prog" ) )
        return REST_PROG;
    if ( !str_cmp( name, "rfight_prog" ) )
        return FIGHT_PROG;
    if ( !str_cmp( name, "enter_prog" ) )
        return ENTRY_PROG;
    if ( !str_cmp( name, "leave_prog" ) )
        return LEAVE_PROG;
    if ( !str_cmp( name, "rdeath_prog" ) )
        return DEATH_PROG;
    if ( !str_cmp( name, "script_prog" ) )
        return SCRIPT_PROG;
    if ( !str_cmp( name, "use_prog" ) )
        return USE_PROG;
    if ( !str_cmp( name, "pre_enter_prog" ) )
        return PRE_ENTER_PROG;
    return ( ERROR_PROG );
}

MPROG_DATA             *mprog_file_read( char *f, MPROG_DATA * mprg, MOB_INDEX_DATA *pMobIndex )
{
    char                    MUDProgfile[MIL];
    FILE                   *progfile;
    char                    letter;
    MPROG_DATA             *mprg_next,
                           *mprg2;
    bool                    done = FALSE;

    snprintf( MUDProgfile, MIL, "%s%s", PROG_DIR, f );
    progfile = FileOpen( MUDProgfile, "r" );
    if ( !progfile ) {
        bug( "Mob: %d couldn't open mudprog file", pMobIndex->vnum );
        exit( 1 );
    }
    mprg2 = mprg;
    switch ( letter = fread_letter( progfile ) ) {
        case '>':
            break;
        case '|':
            bug( "empty mudprog file." );
            exit( 1 );
            break;
        default:
            bug( "in mudprog file syntax error." );
            exit( 1 );
            break;
    }
    while ( !done ) {
        mprg2->type = mprog_name_to_type( fread_word( progfile ) );
        switch ( mprg2->type ) {
            case ERROR_PROG:
                bug( "mudprog file type error" );
                exit( 1 );
                break;
            case IN_FILE_PROG:
                bug( "mprog file contains a call to file." );
                exit( 1 );
                break;
            default:
                xSET_BIT( pMobIndex->progtypes, mprg2->type );
                mprg2->arglist = fread_string( progfile );
                mprg2->comlist = fread_string( progfile );
                switch ( letter = fread_letter( progfile ) ) {
                    case '>':
                        CREATE( mprg_next, MPROG_DATA, 1 );
                        mprg_next->next = mprg2;
                        mprg2 = mprg_next;
                        break;
                    case '|':
                        done = TRUE;
                        break;
                    default:
                        bug( "in mudprog file syntax error." );
                        exit( 1 );
                        break;
                }
                break;
        }
    }
    FileClose( progfile );
    return mprg2;
}

/* This procedure is responsible for reading any in_file MUDprograms. */
void mprog_read_programs( FILE * fp, MOB_INDEX_DATA *pMobIndex )
{
    MPROG_DATA             *mprg;
    char                    letter;
    bool                    done = FALSE;

    if ( ( letter = fread_letter( fp ) ) != '>' ) {
        bug( "Load_mobiles: vnum %d MUDPROG char", pMobIndex->vnum );
        exit( 1 );
    }
    CREATE( mprg, MPROG_DATA, 1 );
    pMobIndex->mudprogs = mprg;
    while ( !done ) {
        mprg->type = mprog_name_to_type( fread_word( fp ) );
        switch ( mprg->type ) {
            case ERROR_PROG:
                bug( "Load_mobiles: vnum %d MUDPROG type.", pMobIndex->vnum );
                exit( 1 );
                break;
            case IN_FILE_PROG:
                mprg = mprog_file_read( fread_string( fp ), mprg, pMobIndex );
                fread_to_eol( fp );
                switch ( letter = fread_letter( fp ) ) {
                    case '>':
                        CREATE( mprg->next, MPROG_DATA, 1 );
                        mprg = mprg->next;
                        break;
                    case '|':
                        mprg->next = NULL;
                        fread_to_eol( fp );
                        done = TRUE;
                        break;
                    default:
                        bug( "Load_mobiles: vnum %d bad MUDPROG.", pMobIndex->vnum );
                        exit( 1 );
                        break;
                }
                break;
            default:
                xSET_BIT( pMobIndex->progtypes, mprg->type );
                mprg->arglist = fread_string( fp );
                fread_to_eol( fp );
                mprg->comlist = fread_string( fp );
                fread_to_eol( fp );
                switch ( letter = fread_letter( fp ) ) {
                    case '>':
                        CREATE( mprg->next, MPROG_DATA, 1 );
                        mprg = mprg->next;
                        break;
                    case '|':
                        mprg->next = NULL;
                        fread_to_eol( fp );
                        done = TRUE;
                        break;
                    default:
                        bug( "Load_mobiles: vnum %d bad MUDPROG.", pMobIndex->vnum );
                        exit( 1 );
                        break;
                }
                break;
        }
    }
    return;
}

/*************************************************************/
/* obj prog functions */
/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

/* This routine reads in scripts of OBJprograms from a file */
MPROG_DATA             *oprog_file_read( char *f, MPROG_DATA * mprg, OBJ_INDEX_DATA *pObjIndex )
{
    char                    MUDProgfile[MIL];
    FILE                   *progfile;
    char                    letter;
    MPROG_DATA             *mprg_next,
                           *mprg2;
    bool                    done = FALSE;

    snprintf( MUDProgfile, MIL, "%s%s", PROG_DIR, f );
    progfile = FileOpen( MUDProgfile, "r" );
    if ( !progfile ) {
        bug( "Obj: %d couldnt open mudprog file", pObjIndex->vnum );
        exit( 1 );
    }
    mprg2 = mprg;
    switch ( letter = fread_letter( progfile ) ) {
        case '>':
            break;
        case '|':
            bug( "empty objprog file." );
            exit( 1 );
            break;
        default:
            bug( "in objprog file syntax error." );
            exit( 1 );
            break;
    }
    while ( !done ) {
        mprg2->type = mprog_name_to_type( fread_word( progfile ) );
        switch ( mprg2->type ) {
            case ERROR_PROG:
                bug( "objprog file type error" );
                exit( 1 );
                break;
            case IN_FILE_PROG:
                bug( "objprog file contains a call to file." );
                exit( 1 );
                break;
            default:
                xSET_BIT( pObjIndex->progtypes, mprg2->type );
                mprg2->arglist = fread_string( progfile );
                mprg2->comlist = fread_string( progfile );
                switch ( letter = fread_letter( progfile ) ) {
                    case '>':
                        CREATE( mprg_next, MPROG_DATA, 1 );
                        mprg_next->next = mprg2;
                        mprg2 = mprg_next;
                        break;
                    case '|':
                        done = TRUE;
                        break;
                    default:
                        bug( "in objprog file syntax error." );
                        exit( 1 );
                        break;
                }
                break;
        }
    }
    FileClose( progfile );
    return mprg2;
}

/* This procedure is responsible for reading any in_file OBJprograms. */
void oprog_read_programs( FILE * fp, OBJ_INDEX_DATA *pObjIndex )
{
    MPROG_DATA             *mprg;
    char                    letter;
    bool                    done = FALSE;

    if ( ( letter = fread_letter( fp ) ) != '>' ) {
        bug( "Load_objects: vnum %d OBJPROG char", pObjIndex->vnum );
        exit( 1 );
    }
    CREATE( mprg, MPROG_DATA, 1 );
    pObjIndex->mudprogs = mprg;
    while ( !done ) {
        mprg->type = mprog_name_to_type( fread_word( fp ) );
        switch ( mprg->type ) {
            case ERROR_PROG:
                bug( "Load_objects: vnum %d OBJPROG type.", pObjIndex->vnum );
                exit( 1 );
                break;
            case IN_FILE_PROG:
                mprg = oprog_file_read( fread_string( fp ), mprg, pObjIndex );
                fread_to_eol( fp );
                switch ( letter = fread_letter( fp ) ) {
                    case '>':
                        CREATE( mprg->next, MPROG_DATA, 1 );
                        mprg = mprg->next;
                        break;
                    case '|':
                        mprg->next = NULL;
                        fread_to_eol( fp );
                        done = TRUE;
                        break;
                    default:
                        bug( "Load_objects: vnum %d bad OBJPROG.", pObjIndex->vnum );
                        exit( 1 );
                        break;
                }
                break;
            default:
                xSET_BIT( pObjIndex->progtypes, mprg->type );
                mprg->arglist = fread_string( fp );
                fread_to_eol( fp );
                mprg->comlist = fread_string( fp );
                fread_to_eol( fp );
                switch ( letter = fread_letter( fp ) ) {
                    case '>':
                        CREATE( mprg->next, MPROG_DATA, 1 );
                        mprg = mprg->next;
                        break;
                    case '|':
                        mprg->next = NULL;
                        fread_to_eol( fp );
                        done = TRUE;
                        break;
                    default:
                        bug( "Load_objects: vnum %d bad OBJPROG.", pObjIndex->vnum );
                        exit( 1 );
                        break;
                }
                break;
        }
    }
    return;
}

/*************************************************************/
/* room prog functions */
/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

/* This routine reads in scripts of OBJprograms from a file */
MPROG_DATA             *rprog_file_read( char *f, MPROG_DATA * mprg, ROOM_INDEX_DATA *RoomIndex )
{
    char                    MUDProgfile[MIL];
    FILE                   *progfile;
    char                    letter;
    MPROG_DATA             *mprg_next,
                           *mprg2;
    bool                    done = FALSE;

    snprintf( MUDProgfile, MIL, "%s%s", PROG_DIR, f );
    progfile = FileOpen( MUDProgfile, "r" );
    if ( !progfile ) {
        bug( "Room: %d couldnt open roomprog file", RoomIndex->vnum );
        exit( 1 );
    }
    mprg2 = mprg;
    switch ( letter = fread_letter( progfile ) ) {
        case '>':
            break;
        case '|':
            bug( "empty roomprog file." );
            exit( 1 );
            break;
        default:
            bug( "in roomprog file syntax error." );
            exit( 1 );
            break;
    }
    while ( !done ) {
        mprg2->type = mprog_name_to_type( fread_word( progfile ) );
        switch ( mprg2->type ) {
            case ERROR_PROG:
                bug( "roomprog file type error" );
                exit( 1 );
                break;
            case IN_FILE_PROG:
                bug( "roomprog file contains a call to file." );
                exit( 1 );
                break;
            default:
                xSET_BIT( RoomIndex->progtypes, mprg2->type );
                mprg2->arglist = fread_string( progfile );
                mprg2->comlist = fread_string( progfile );
                switch ( letter = fread_letter( progfile ) ) {
                    case '>':
                        CREATE( mprg_next, MPROG_DATA, 1 );
                        mprg_next->next = mprg2;
                        mprg2 = mprg_next;
                        break;
                    case '|':
                        done = TRUE;
                        break;
                    default:
                        bug( "in roomprog file syntax error." );
                        exit( 1 );
                        break;
                }
                break;
        }
    }
    FileClose( progfile );
    return mprg2;
}

/* This procedure is responsible for reading any in_file ROOMprograms. */
void rprog_read_programs( FILE * fp, ROOM_INDEX_DATA *pRoomIndex )
{
    MPROG_DATA             *mprg;
    char                    letter;
    bool                    done = FALSE;

    if ( ( letter = fread_letter( fp ) ) != '>' ) {
        bug( "Load_rooms: vnum %d ROOMPROG char", pRoomIndex->vnum );
        exit( 1 );
    }
    CREATE( mprg, MPROG_DATA, 1 );
    pRoomIndex->mudprogs = mprg;
    while ( !done ) {
        mprg->type = mprog_name_to_type( fread_word( fp ) );
        switch ( mprg->type ) {
            case ERROR_PROG:
                bug( "Load_rooms: vnum %d ROOMPROG type.", pRoomIndex->vnum );
                exit( 1 );
                break;
            case IN_FILE_PROG:
                mprg = rprog_file_read( fread_string( fp ), mprg, pRoomIndex );
                fread_to_eol( fp );
                switch ( letter = fread_letter( fp ) ) {
                    case '>':
                        CREATE( mprg->next, MPROG_DATA, 1 );
                        mprg = mprg->next;
                        break;
                    case '|':
                        mprg->next = NULL;
                        fread_to_eol( fp );
                        done = TRUE;
                        break;
                    default:
                        bug( "Load_rooms: vnum %d bad ROOMPROG.", pRoomIndex->vnum );
                        exit( 1 );
                        break;
                }
                break;
            default:
                xSET_BIT( pRoomIndex->progtypes, mprg->type );
                mprg->arglist = fread_string( fp );
                fread_to_eol( fp );
                mprg->comlist = fread_string( fp );
                fread_to_eol( fp );
                switch ( letter = fread_letter( fp ) ) {
                    case '>':
                        CREATE( mprg->next, MPROG_DATA, 1 );
                        mprg = mprg->next;
                        break;
                    case '|':
                        mprg->next = NULL;
                        fread_to_eol( fp );
                        done = TRUE;
                        break;
                    default:
                        bug( "Load_rooms: vnum %d bad ROOMPROG.", pRoomIndex->vnum );
                        exit( 1 );
                        break;
                }
                break;
        }
    }
    return;
}

/*************************************************************/
/* Function to delete a room index.  Called from do_rdelete in build.c
 * Narn, May/96
 * Don't ask me why they return bool.. :).. oh well.. -- Alty
 * Don't ask me either, so I changed it to void. - Samson
 */
void delete_room( ROOM_INDEX_DATA *room )
{
    int                     hash;
    ROOM_INDEX_DATA        *prev,
                           *limbo = get_room_index( ROOM_VNUM_LIMBO );
    OBJ_DATA               *o;
    CHAR_DATA              *ch;
    EXTRA_DESCR_DATA       *ed;
    EXIT_DATA              *ex;
    MPROG_ACT_LIST         *mpact;
    MPROG_DATA             *mp;

    UNLINK( room, room->area->first_room, room->area->last_room, next_aroom, prev_aroom );
    AREA_DATA              *pArea;

    while ( ( ch = room->first_person ) != NULL ) {
        if ( !IS_NPC( ch ) ) {
            char_from_room( ch );
            char_to_room( ch, limbo );
        }
        else
            extract_char( ch, TRUE );
    }
    while ( ( o = room->first_content ) != NULL )
        extract_obj( o );
    wipe_resets( room );
    /*
     * Remove area resets to do with the room 
     */
    pArea = room->area;

    while ( ( ed = room->first_extradesc ) != NULL ) {
        room->first_extradesc = ed->next;
        if ( VLD_STR( ed->keyword ) )
            STRFREE( ed->keyword );
        if ( VLD_STR( ed->description ) )
            STRFREE( ed->description );
        DISPOSE( ed );
        --top_ed;
    }
    while ( ( ex = room->first_exit ) != NULL )
        extract_exit( room, ex );
    while ( ( mpact = room->mpact ) != NULL ) {
        room->mpact = mpact->next;
        DISPOSE( mpact->buf );
        DISPOSE( mpact );
    }
    while ( ( mp = room->mudprogs ) != NULL ) {
        room->mudprogs = mp->next;
        if ( VLD_STR( mp->arglist ) )
            STRFREE( mp->arglist );
        if ( VLD_STR( mp->comlist ) )
            STRFREE( mp->comlist );
        DISPOSE( mp );
    }
    if ( VLD_STR( room->name ) )
        STRFREE( room->name );
    if ( VLD_STR( room->description ) )
        STRFREE( room->description );
    hash = room->vnum % MAX_KEY_HASH;
    if ( room == room_index_hash[hash] )
        room_index_hash[hash] = room->next;
    else {
        for ( prev = room_index_hash[hash]; prev; prev = prev->next )
            if ( prev->next == room )
                break;
        if ( prev )
            prev->next = room->next;
        else
            bug( "delete_room: room %d not in hash bucket %d.", room->vnum, hash );
    }
    DISPOSE( room );
    --top_room;
    return;
}

/* See comment on delete_room. */
void delete_obj( OBJ_INDEX_DATA *obj )
{
    int                     hash;
    OBJ_INDEX_DATA         *prev;
    OBJ_DATA               *o,
                           *o_next;
    EXTRA_DESCR_DATA       *ed;
    AFFECT_DATA            *af;
    MPROG_DATA             *mp;

    /*
     * Remove references to object index 
     */
    for ( o = first_object; o; o = o_next ) {
        o_next = o->next;
        if ( o->pIndexData == obj )
            extract_obj( o );
    }
    while ( ( ed = obj->first_extradesc ) != NULL ) {
        obj->first_extradesc = ed->next;
        if ( VLD_STR( ed->keyword ) )
            STRFREE( ed->keyword );
        if ( VLD_STR( ed->description ) )
            STRFREE( ed->description );
        DISPOSE( ed );
        --top_ed;
    }
    while ( ( af = obj->first_affect ) != NULL ) {
        obj->first_affect = af->next;
        DISPOSE( af );
        --top_affect;
    }
    while ( ( mp = obj->mudprogs ) != NULL ) {
        obj->mudprogs = mp->next;
        if ( VLD_STR( mp->arglist ) )
            STRFREE( mp->arglist );
        if ( VLD_STR( mp->comlist ) )
            STRFREE( mp->comlist );
        DISPOSE( mp );
    }
    if ( VLD_STR( obj->name ) )
        STRFREE( obj->name );
    if ( VLD_STR( obj->short_descr ) )
        STRFREE( obj->short_descr );
    if ( VLD_STR( obj->description ) )
        STRFREE( obj->description );
    if ( VLD_STR( obj->action_desc ) )
        STRFREE( obj->action_desc );
    hash = obj->vnum % MAX_KEY_HASH;
    if ( obj == obj_index_hash[hash] )
        obj_index_hash[hash] = obj->next;
    else {
        for ( prev = obj_index_hash[hash]; prev; prev = prev->next )
            if ( prev->next == obj )
                break;
        if ( prev )
            prev->next = obj->next;
        else
            bug( "delete_obj: object %d not in hash bucket %d.", obj->vnum, hash );
    }
    DISPOSE( obj );
    --top_obj_index;
    return;
}

/* See comment on delete_room. */
void delete_mob( MOB_INDEX_DATA *mob )
{
    int                     hash;
    MOB_INDEX_DATA         *prev;
    CHAR_DATA              *ch,
                           *ch_next;
    MPROG_DATA             *mp;

    for ( ch = first_char; ch; ch = ch_next ) {
        ch_next = ch->next;
        if ( ch->pIndexData == mob )
            extract_char( ch, TRUE );
    }
    while ( ( mp = mob->mudprogs ) != NULL ) {
        mob->mudprogs = mp->next;
        if ( mp->arglist )
            STRFREE( mp->arglist );
        if ( mp->comlist )
            STRFREE( mp->comlist );
        DISPOSE( mp );
    }
    if ( mob->pShop ) {
        UNLINK( mob->pShop, first_shop, last_shop, next, prev );
        DISPOSE( mob->pShop );
        --top_shop;
    }
    if ( mob->rShop ) {
        UNLINK( mob->rShop, first_repair, last_repair, next, prev );
        DISPOSE( mob->rShop );
        --top_repair;
    }
    if ( mob->clanname )
        STRFREE( mob->clanname );
    if ( mob->player_name )
        STRFREE( mob->player_name );
    if ( mob->short_descr )
        STRFREE( mob->short_descr );
    if ( mob->long_descr )
        STRFREE( mob->long_descr );
    if ( mob->description )
        STRFREE( mob->description );
    hash = mob->vnum % MAX_KEY_HASH;
    if ( mob == mob_index_hash[hash] )
        mob_index_hash[hash] = mob->next;
    else {
        for ( prev = mob_index_hash[hash]; prev; prev = prev->next )
            if ( prev->next == mob )
                break;
        if ( prev )
            prev->next = mob->next;
        else
            bug( "delete_mob: mobile %d not in hash bucket %d.", mob->vnum, hash );
    }
    DISPOSE( mob );
    --top_mob_index;
    return;
}

ROOM_INDEX_DATA        *make_room( int vnum, AREA_DATA *area )
{
    ROOM_INDEX_DATA        *pRoomIndex;
    int                     iHash;

    CREATE( pRoomIndex, ROOM_INDEX_DATA, 1 );

    pRoomIndex->first_person = NULL;
    pRoomIndex->last_person = NULL;
    pRoomIndex->first_content = NULL;
    pRoomIndex->last_content = NULL;
    pRoomIndex->first_reset = pRoomIndex->last_reset = NULL;
    pRoomIndex->area = area;
    pRoomIndex->vnum = vnum;
    pRoomIndex->name = STRALLOC( "Floating in a void" );
    pRoomIndex->description = STRALLOC( "" );
    pRoomIndex->room_flags = ROOM_PROTOTYPE;
    pRoomIndex->sector_type = 1;
    pRoomIndex->light = 0;
    pRoomIndex->first_exit = NULL;
    pRoomIndex->last_exit = NULL;
    LINK( pRoomIndex, area->first_room, area->last_room, next_aroom, prev_aroom );

    iHash = vnum % MAX_KEY_HASH;
    pRoomIndex->next = room_index_hash[iHash];
    room_index_hash[iHash] = pRoomIndex;
    ++top_room;

    return pRoomIndex;
}

/* Create a new INDEX object (for online building)  -Thoric
 * Option to clone an existing index object.
 */
OBJ_INDEX_DATA         *make_object( int vnum, int cvnum, char *name )
{
    OBJ_INDEX_DATA         *pObjIndex,
                           *cObjIndex;
    char                    buf[MSL];
    int                     iHash;

    if ( cvnum > 0 )
        cObjIndex = get_obj_index( cvnum );
    else
        cObjIndex = NULL;
    CREATE( pObjIndex, OBJ_INDEX_DATA, 1 );

    pObjIndex->vnum = vnum;
    pObjIndex->name = STRALLOC( name );
    pObjIndex->first_affect = NULL;
    pObjIndex->last_affect = NULL;
    pObjIndex->first_extradesc = NULL;
    pObjIndex->last_extradesc = NULL;
    if ( !cObjIndex ) {
        snprintf( buf, MSL, "A newly created %s", name );
        pObjIndex->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "Some god dropped a newly created %s here.", name );
        pObjIndex->description = STRALLOC( buf );
        pObjIndex->short_descr[0] = LOWER( pObjIndex->short_descr[0] );
        pObjIndex->description[0] = UPPER( pObjIndex->description[0] );
        pObjIndex->item_type = ITEM_TRASH;
        xCLEAR_BITS( pObjIndex->extra_flags );
        xSET_BIT( pObjIndex->extra_flags, ITEM_PROTOTYPE );
        pObjIndex->weight = 1;
        pObjIndex->currtype = DEFAULT_CURR;
    }
    else {
        EXTRA_DESCR_DATA       *ed,
                               *ced;
        AFFECT_DATA            *paf,
                               *cpaf;

        pObjIndex->short_descr = QUICKLINK( cObjIndex->short_descr );
        pObjIndex->description = QUICKLINK( cObjIndex->description );
        pObjIndex->action_desc = QUICKLINK( cObjIndex->action_desc );
        pObjIndex->item_type = cObjIndex->item_type;
        pObjIndex->extra_flags = cObjIndex->extra_flags;
        xSET_BIT( pObjIndex->extra_flags, ITEM_PROTOTYPE );
        pObjIndex->wear_flags = cObjIndex->wear_flags;
        pObjIndex->value[0] = cObjIndex->value[0];
        pObjIndex->value[1] = cObjIndex->value[1];
        pObjIndex->value[2] = cObjIndex->value[2];
        pObjIndex->value[3] = cObjIndex->value[3];
        pObjIndex->value[4] = cObjIndex->value[4];
        pObjIndex->value[5] = cObjIndex->value[5];
        pObjIndex->value[6] = cObjIndex->value[6];
        pObjIndex->weight = cObjIndex->weight;
        pObjIndex->cost = cObjIndex->cost;
        pObjIndex->currtype = cObjIndex->currtype;
        pObjIndex->color = cObjIndex->color;
        for ( ced = cObjIndex->first_extradesc; ced; ced = ced->next ) {
            CREATE( ed, EXTRA_DESCR_DATA, 1 );

            ed->keyword = QUICKLINK( ced->keyword );
            ed->description = QUICKLINK( ced->description );
            LINK( ed, pObjIndex->first_extradesc, pObjIndex->last_extradesc, next, prev );
            top_ed++;
        }
        for ( cpaf = cObjIndex->first_affect; cpaf; cpaf = cpaf->next ) {
            CREATE( paf, AFFECT_DATA, 1 );

            paf->type = cpaf->type;
            paf->duration = cpaf->duration;
            paf->location = cpaf->location;
            paf->modifier = cpaf->modifier;
            paf->bitvector = cpaf->bitvector;
            LINK( paf, pObjIndex->first_affect, pObjIndex->last_affect, next, prev );
            top_affect++;
        }
    }
    pObjIndex->count = 0;
    iHash = vnum % MAX_KEY_HASH;
    pObjIndex->next = obj_index_hash[iHash];
    obj_index_hash[iHash] = pObjIndex;
    top_obj_index++;
    return pObjIndex;
}

/* Create a new INDEX mobile (for online building)  -Thoric
 * Option to clone an existing index mobile.
 */
MOB_INDEX_DATA         *make_mobile( int vnum, int cvnum, char *name )
{
    MOB_INDEX_DATA         *pMobIndex,
                           *cMobIndex;
    char                    buf[MSL];
    int                     iHash;

    if ( cvnum > 0 )
        cMobIndex = get_mob_index( cvnum );
    else
        cMobIndex = NULL;
    CREATE( pMobIndex, MOB_INDEX_DATA, 1 );

    pMobIndex->vnum = vnum;
    pMobIndex->count = 0;
    pMobIndex->killed = 0;
    pMobIndex->player_name = STRALLOC( name );
    if ( !cMobIndex ) {
        snprintf( buf, MSL, "A newly created %s", name );
        pMobIndex->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, "Some god abandoned a newly created %s here.\r\n", name );
        pMobIndex->long_descr = STRALLOC( buf );
        if ( VLD_STR( pMobIndex->short_descr ) )
            pMobIndex->short_descr[0] = LOWER( pMobIndex->short_descr[0] );
        if ( VLD_STR( pMobIndex->long_descr ) )
            pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
        if ( VLD_STR( pMobIndex->description ) )
            pMobIndex->description[0] = UPPER( pMobIndex->description[0] );
        xCLEAR_BITS( pMobIndex->act );
        xSET_BIT( pMobIndex->act, ACT_IS_NPC );
        xSET_BIT( pMobIndex->act, ACT_PROTOTYPE );
        xCLEAR_BITS( pMobIndex->affected_by );
/*    xCLEAR_BITS(pMobIndex->affected_by2); */
        xCLEAR_BITS( pMobIndex->progtypes );
        pMobIndex->level = 1;
        pMobIndex->position = POS_STANDING;
        pMobIndex->defposition = POS_STANDING;
        pMobIndex->perm_str = 13;
        pMobIndex->perm_dex = 13;
        pMobIndex->perm_int = 13;
        pMobIndex->perm_wis = 13;
        pMobIndex->perm_cha = 13;
        pMobIndex->perm_con = 13;
        pMobIndex->perm_lck = 13;
        pMobIndex->Class = 4;
        pMobIndex->color = 1;
        xCLEAR_BITS( pMobIndex->xflags );
        xCLEAR_BITS( pMobIndex->attacks );
        xCLEAR_BITS( pMobIndex->defenses );
    }
    else {
        if ( VLD_STR( cMobIndex->short_descr ) )
            pMobIndex->short_descr = QUICKLINK( cMobIndex->short_descr );
        if ( VLD_STR( cMobIndex->long_descr ) )
            pMobIndex->long_descr = QUICKLINK( cMobIndex->long_descr );
        if ( VLD_STR( cMobIndex->description ) )
            pMobIndex->description = QUICKLINK( cMobIndex->description );
        pMobIndex->act = cMobIndex->act;
        xSET_BIT( pMobIndex->act, ACT_PROTOTYPE );
        pMobIndex->affected_by = cMobIndex->affected_by;
/*    pMobIndex->affected_by2 = cMobIndex->affected_by2; */
        pMobIndex->spec_fun = cMobIndex->spec_fun;
        xCLEAR_BITS( pMobIndex->progtypes );
        pMobIndex->alignment = cMobIndex->alignment;
        pMobIndex->level = cMobIndex->level;
        pMobIndex->mobthac0 = cMobIndex->mobthac0;
        pMobIndex->ac = cMobIndex->ac;
        pMobIndex->color = cMobIndex->color;
        pMobIndex->hitnodice = cMobIndex->hitnodice;
        pMobIndex->hitsizedice = cMobIndex->hitsizedice;
        pMobIndex->hitplus = cMobIndex->hitplus;
        pMobIndex->damnodice = cMobIndex->damnodice;
        pMobIndex->damsizedice = cMobIndex->damsizedice;
        pMobIndex->damplus = cMobIndex->damplus;
        pMobIndex->exp = cMobIndex->exp;
        pMobIndex->position = cMobIndex->position;
        pMobIndex->defposition = cMobIndex->defposition;
        pMobIndex->sex = cMobIndex->sex;
        pMobIndex->perm_str = cMobIndex->perm_str;
        pMobIndex->perm_dex = cMobIndex->perm_dex;
        pMobIndex->perm_int = cMobIndex->perm_int;
        pMobIndex->perm_wis = cMobIndex->perm_wis;
        pMobIndex->perm_cha = cMobIndex->perm_cha;
        pMobIndex->perm_con = cMobIndex->perm_con;
        pMobIndex->perm_lck = cMobIndex->perm_lck;
        pMobIndex->race = cMobIndex->race;
        pMobIndex->Class = cMobIndex->Class;
        pMobIndex->xflags = cMobIndex->xflags;
        pMobIndex->resistant = cMobIndex->resistant;
        pMobIndex->immune = cMobIndex->immune;
        pMobIndex->susceptible = cMobIndex->susceptible;
        pMobIndex->numattacks = cMobIndex->numattacks;
        pMobIndex->attacks = cMobIndex->attacks;
        pMobIndex->defenses = cMobIndex->defenses;
    }
    iHash = vnum % MAX_KEY_HASH;
    pMobIndex->next = mob_index_hash[iHash];
    mob_index_hash[iHash] = pMobIndex;
    top_mob_index++;
    return pMobIndex;
}

/* Creates a simple exit with no fields filled but rvnum and optionally
 * to_room and vnum.      -Thoric
 * Exits are inserted into the linked list based on vdir.
 */
EXIT_DATA              *make_exit( ROOM_INDEX_DATA *pRoomIndex, ROOM_INDEX_DATA *to_room,
                                   short door )
{
    EXIT_DATA              *pexit,
                           *texit;
    bool                    broke;

    CREATE( pexit, EXIT_DATA, 1 );

    pexit->vdir = door;
    pexit->rvnum = pRoomIndex->vnum;
    pexit->to_room = to_room;
    if ( to_room ) {
        pexit->vnum = to_room->vnum;
        texit = get_exit_to( to_room, rev_dir[door], pRoomIndex->vnum );
        if ( texit ) {                                 /* assign reverse exit pointers */
            texit->rexit = pexit;
            pexit->rexit = texit;
        }
    }
    broke = FALSE;
    for ( texit = pRoomIndex->first_exit; texit; texit = texit->next )
        if ( door < texit->vdir ) {
            broke = TRUE;
            break;
        }
    if ( !pRoomIndex->first_exit )
        pRoomIndex->first_exit = pexit;
    else {
        /*
         * keep exits in incremental order - insert exit into list 
         */
        if ( broke && texit ) {
            if ( !texit->prev )
                pRoomIndex->first_exit = pexit;
            else
                texit->prev->next = pexit;
            pexit->prev = texit->prev;
            pexit->next = texit;
            texit->prev = pexit;
            top_exit++;
            return pexit;
        }
        pRoomIndex->last_exit->next = pexit;
    }
    pexit->next = NULL;
    pexit->prev = pRoomIndex->last_exit;
    pRoomIndex->last_exit = pexit;
    top_exit++;
    return pexit;
}

void fix_area_exits( AREA_DATA *tarea )
{
    ROOM_INDEX_DATA        *pRoomIndex;
    EXIT_DATA              *pexit,
                           *rev_exit;
    int                     rnum;
    bool                    fexit;

    for ( rnum = tarea->low_r_vnum; rnum <= tarea->hi_r_vnum; rnum++ ) {
        if ( ( pRoomIndex = get_room_index( rnum ) ) == NULL )
            continue;
        fexit = FALSE;
        for ( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next ) {
            fexit = TRUE;
            pexit->rvnum = pRoomIndex->vnum;
            if ( pexit->vnum <= 0 )
                pexit->to_room = NULL;
            else
                pexit->to_room = get_room_index( pexit->vnum );
        }
        if ( !fexit )
            SET_BIT( pRoomIndex->room_flags, ROOM_NO_MOB );
    }
    for ( rnum = tarea->low_r_vnum; rnum <= tarea->hi_r_vnum; rnum++ ) {
        if ( ( pRoomIndex = get_room_index( rnum ) ) == NULL )
            continue;
        for ( pexit = pRoomIndex->first_exit; pexit; pexit = pexit->next ) {
            if ( pexit->to_room && !pexit->rexit ) {
                rev_exit = get_exit_to( pexit->to_room, rev_dir[pexit->vdir], pRoomIndex->vnum );
                if ( rev_exit ) {
                    pexit->rexit = rev_exit;
                    rev_exit->rexit = pexit;
                }
            }
        }
    }
}

void load_area_file( AREA_DATA *tarea, const char *filename )
{
    char                    Newfilename[MSL],
                            check[MSL];
    bool                    colorfound = FALSE;

    if ( fBootDb )
        tarea = last_area;
    if ( !fBootDb && !tarea ) {
        bug( "Load_area: null area!" );
        return;
    }
    snprintf( check, MSL, "%s", BUILD_DIR );
    if ( !str_prefix( check, filename ) )
        snprintf( Newfilename, MSL, "%s", filename );
    else
        snprintf( Newfilename, MSL, "%s%s", AREA_DIR, filename );
    if ( ( fpArea = FileOpen( Newfilename, "r" ) ) == NULL ) {
        perror( Newfilename );
        bug( "load_area: %s error loading file (can't open)", Newfilename );
        return;
    }
    area_version = 0;
    for ( ;; ) {
        char                   *word;

        if ( fread_letter( fpArea ) != '#' ) {
            if ( fBootDb ) {
                bug( "load_area_file: No # found at start of area file." );
                exit( 1 );
            }
            else {
                bug( "load_area_files: No # found at start of area file." );
                FileClose( fpArea );
                fpArea = NULL;
                return;
            }
        }

        word = fread_word( fpArea );
        if ( word[0] == '$' )
            break;
        else if ( !str_cmp( word, "AREA" ) ) {
            if ( fBootDb ) {
                load_area( fpArea );
                tarea = last_area;
            }
            else {
                STRFREE( tarea->name );
                tarea->name = fread_string( fpArea );
            }
            tarea->desc = NULL;
            tarea->color = 11;
        }
        else if ( !str_cmp( word, "AUTHOR" ) )
            load_author( tarea, fpArea );
        else if ( !str_cmp( word, "HTOWN" ) )
            load_homeland( tarea, fpArea );
        else if ( !str_cmp( word, "DERIVATIVES" ) )
            load_derivatives( tarea, fpArea );
        else if ( !str_cmp( word, "COLOR" ) ) {        /* Dont really need a seperate
                                                        * function for this * one */
            tarea->color = fread_number( fpArea );
            colorfound = TRUE;
        }
        else if ( !str_cmp( word, "DESC" ) )
            load_desc( tarea, fpArea );
        else if ( !str_cmp( word, "FLAGS" ) )
            load_flags( tarea, fpArea );
        else if ( !str_cmp( word, "RANGES" ) )
            load_ranges( tarea, fpArea );
        else if ( !str_cmp( word, "ECONOMY" ) )
            load_economy( tarea, fpArea );
        else if ( !str_cmp( word, "CURRENCY" ) )
            load_areacurr( tarea, fpArea );
        else if ( !str_cmp( word, "RESETMSG" ) )
            load_resetmsg( tarea, fpArea );
        else if ( !str_cmp( word, "WEATHERCELL" ) )
            load_weathercell( tarea, fpArea );
        else if ( !str_cmp( word, "HIGHECONOMY" ) )
            load_higheconomy( tarea, fpArea );
        else if ( !str_cmp( word, "LOWECONOMY" ) )
            load_loweconomy( tarea, fpArea );
        else if ( !str_cmp( word, "CLANNAME" ) )
            load_clanname( tarea, fpArea );
        else if ( !str_cmp( word, "INFLUENCE" ) )
            load_influence( tarea, fpArea );
        /*
         * Rennard 
         */
        else if ( !str_cmp( word, "HELPS" ) )
            load_helps( tarea, fpArea );
        else if ( !str_cmp( word, "MOBILES" ) )
            load_mobiles( tarea, fpArea );
        else if ( !str_cmp( word, "OBJECTS" ) )
            load_objects( tarea, fpArea );
        else if ( !str_cmp( word, "RESETS" ) )
            load_resets( tarea, fpArea );
        else if ( !str_cmp( word, "ROOMS" ) )
            load_rooms( tarea, fpArea );
        else if ( !str_cmp( word, "SHOPS" ) )
            load_shops( tarea, fpArea );
        else if ( !str_cmp( word, "REPAIRS" ) )
            load_repairs( tarea, fpArea );
        else if ( !str_cmp( word, "SPECIALS" ) )
            load_specials( tarea, fpArea );
        else if ( !str_cmp( word, "CLIMATE" ) )
            load_climate( tarea, fpArea );
        else if ( !str_cmp( word, "NEIGHBOR" ) )
            load_neighbor( tarea, fpArea );
        else if ( !str_cmp( word, "VERSION" ) )
            load_version( tarea, fpArea );
        else if ( !str_cmp( word, "SPELLLIMIT" ) )
            tarea->spelllimit = fread_number( fpArea );
        else {
            bug( "load_area_file: bad section name: %s", word );
            if ( fBootDb )
                exit( 1 );
            else {
                FileClose( fpArea );
                fpArea = NULL;
                return;
            }
        }
    }
    FileClose( fpArea );
    fpArea = NULL;
    if ( tarea ) {
        if ( fBootDb ) {
            sort_area_by_name( tarea );                /* 4/27/97 */
            sort_area( tarea, FALSE );
        }
        fprintf( stderr, "%-14s: Rooms: %5d - %-5d Objs: %5d - %-5d Mobs: %5d - %d\n",
                 tarea->filename, tarea->low_r_vnum, tarea->hi_r_vnum, tarea->low_o_vnum,
                 tarea->hi_o_vnum, tarea->low_m_vnum, tarea->hi_m_vnum );
        SET_BIT( tarea->status, AREA_LOADED );
    }
    else
        fprintf( stderr, "(%s)\n", filename );
}

void load_reserved( void )
{
    RESERVE_DATA           *res;
    FILE                   *fp;

    if ( !( fp = FileOpen( SYSTEM_DIR RESERVED_LIST, "r" ) ) )
        return;

    for ( ;; ) {
        if ( feof( fp ) ) {
            bug( "Load_reserved: no $ found." );
            FileClose( fp );
            return;
        }
        CREATE( res, RESERVE_DATA, 1 );
        res->name = fread_string( fp );
        if ( *res->name == '$' )
            break;
        sort_reserved( res );
    }
    STRFREE( res->name );
    DISPOSE( res );
    FileClose( fp );
    return;
}

/* Build list of in_progress areas.  Do not load areas.
 * define AREA_READ if you want it to build area names rather than reading
 * them out of the area files. -- Altrag */
void load_buildlist( void )
{
    DIR                    *dp;
    struct dirent          *dentry;
    FILE                   *fp;
    char                    buf[MAX_STRING_LENGTH];
    AREA_DATA              *pArea;
    char                   *fgetsed;
    char                    line[81];
    char                    word[81];
    int                     low,
                            hi;
    int                     mlow,
                            mhi,
                            olow,
                            ohi,
                            rlow,
                            rhi;
    bool                    badfile = FALSE;
    char                    temp;

    dp = opendir( STAFF_DIR );
    dentry = readdir( dp );
    while ( dentry ) {
        if ( dentry->d_name[0] != '.' ) {
            snprintf( buf, MAX_STRING_LENGTH, "%s%s", STAFF_DIR, dentry->d_name );
            if ( !( fp = FileOpen( buf, "r" ) ) ) {
                bug( "Load_buildlist: invalid file" );
                perror( buf );
                dentry = readdir( dp );
                continue;
            }
            log_string( buf );
            badfile = FALSE;
            rlow = rhi = olow = ohi = mlow = mhi = 0;
            while ( !feof( fp ) && !ferror( fp ) ) {
                low = 0;
                hi = 0;
                word[0] = 0;
                line[0] = 0;
                if ( ( temp = fgetc( fp ) ) != EOF )
                    ungetc( temp, fp );
                else
                    break;

                fgetsed = fgets( line, 80, fp );
                sscanf( line, "%s %d %d", word, &low, &hi );
                if ( !strcmp( word, "Level" ) ) {
                    if ( low < LEVEL_IMMORTAL ) {
                        snprintf( buf, MAX_STRING_LENGTH, "%s: God file with level %d < %d",
                                  dentry->d_name, low, LEVEL_IMMORTAL );
                        badfile = TRUE;
                    }
                }
                if ( !strcmp( word, "RoomRange" ) )
                    rlow = low, rhi = hi;
                else if ( !strcmp( word, "MobRange" ) )
                    mlow = low, mhi = hi;
                else if ( !strcmp( word, "ObjRange" ) )
                    olow = low, ohi = hi;
            }
            FileClose( fp );
            if ( rlow && rhi && !badfile ) {
                snprintf( buf, MAX_STRING_LENGTH, "%s%s.are", BUILD_DIR, dentry->d_name );
                if ( !( fp = FileOpen( buf, "r" ) ) ) {
                    bug( "Load_buildlist: cannot open area file for read" );
                    perror( buf );
                    dentry = readdir( dp );
                    continue;
                }
#if !defined(READ_AREA)                                /* Dont always want to read
                                                        * stuff.. dunno.. shrug */
                mudstrlcpy( word, fread_word( fp ), MAX_INPUT_LENGTH );
                if ( word[0] != '#' || strcmp( &word[1], "AREA" ) ) {
                    snprintf( buf, MAX_STRING_LENGTH, "Make_buildlist: %s.are: no #AREA found.",
                              dentry->d_name );
                    FileClose( fp );
                    dentry = readdir( dp );
                    continue;
                }
#endif
                CREATE( pArea, AREA_DATA, 1 );

                snprintf( buf, MAX_STRING_LENGTH, "%s.are", dentry->d_name );
                pArea->author = STRALLOC( dentry->d_name );
                pArea->filename = STRALLOC( buf );
#if !defined(READ_AREA)
                pArea->name = fread_string( fp );
#else
                snprintf( buf, MAX_STRING_LENGTH, "{PROTO} %s's area in progress", dentry->d_name );
                pArea->name = STRALLOC( buf );
#endif
                FileClose( fp );
                pArea->low_r_vnum = rlow;
                pArea->hi_r_vnum = rhi;
                pArea->low_m_vnum = mlow;
                pArea->hi_m_vnum = mhi;
                pArea->low_o_vnum = olow;
                pArea->hi_o_vnum = ohi;
                pArea->low_soft_range = -1;
                pArea->hi_soft_range = -1;
                pArea->low_hard_range = -1;
                pArea->hi_hard_range = -1;
                pArea->weatherx = 0;
                pArea->weathery = 0;
                pArea->first_room = pArea->last_room = NULL;

                SET_BIT( pArea->flags, AFLAG_PROTOTYPE );
                LINK( pArea, first_build, last_build, next, prev );
                LINK( pArea, first_full_area, last_full_area, next_area, prev_area );
                sort_area( pArea, TRUE );
                top_area++;
                fBootDb = FALSE;
                snprintf( buf, MAX_STRING_LENGTH, "%s%s.are", BUILD_DIR, dentry->d_name );
                load_area_file( pArea, buf );
                fBootDb = TRUE;
                mudstrlcpy( strArea, "$", sizeof( strArea ) );
                reset_area( pArea );
            }
        }
        dentry = readdir( dp );
    }
    closedir( dp );
}

/* Rebuilt from broken copy, but bugged - commented out for now - Blod */
void sort_reserved( RESERVE_DATA * pRes )
{
    RESERVE_DATA           *res = NULL;

    if ( !pRes ) {
        bug( "Sort_reserved: NULL pRes" );
        return;
    }
    pRes->next = NULL;
    pRes->prev = NULL;
    for ( res = first_reserved; res; res = res->next ) {
        if ( strcasecmp( pRes->name, res->name ) > 0 ) {
            INSERT( pRes, res, first_reserved, next, prev );
            break;
        }
    }
    if ( !res ) {
        LINK( pRes, first_reserved, last_reserved, next, prev );
    }
    return;
}

/* Sort areas by name alphanumercially
 *      - 4/27/97, Fireblade
 */
void sort_area_by_name( AREA_DATA *pArea )
{
    AREA_DATA              *temp_area;

    if ( !pArea ) {
        bug( "Sort_area_by_name: NULL pArea" );
        return;
    }
    for ( temp_area = first_area_name; temp_area; temp_area = temp_area->next_sort_name ) {
        if ( strcmp( pArea->name, temp_area->name ) < 0 ) {
            INSERT( pArea, temp_area, first_area_name, next_sort_name, prev_sort_name );
            break;
        }
    }
    if ( !temp_area )
        LINK( pArea, first_area_name, last_area_name, next_sort_name, prev_sort_name );
}

/* Sort by room vnums     -Altrag & Thoric */
void sort_area( AREA_DATA *pArea, bool proto )
{
    AREA_DATA              *area = NULL;
    AREA_DATA              *first_sort,
                           *last_sort;
    bool                    found;

    if ( !pArea ) {
        bug( "Sort_area: NULL pArea" );
        return;
    }
    if ( proto ) {
        first_sort = first_bsort;
        last_sort = last_bsort;
    }
    else {
        first_sort = first_asort;
        last_sort = last_asort;
    }
    found = FALSE;
    pArea->next_sort = NULL;
    pArea->prev_sort = NULL;
    if ( !first_sort ) {
        pArea->prev_sort = NULL;
        pArea->next_sort = NULL;
        first_sort = pArea;
        last_sort = pArea;
        found = TRUE;
    }
    else {
        for ( area = first_sort; area; area = area->next_sort ) {
            if ( pArea->low_r_vnum < area->low_r_vnum ) {
                if ( !area->prev_sort )
                    first_sort = pArea;
                else
                    area->prev_sort->next_sort = pArea;
                pArea->prev_sort = area->prev_sort;
                pArea->next_sort = area;
                area->prev_sort = pArea;
                found = TRUE;
                break;
            }
        }
    }
    if ( !found ) {
        pArea->prev_sort = last_sort;
        pArea->next_sort = NULL;
        last_sort->next_sort = pArea;
        last_sort = pArea;
    }
    if ( proto ) {
        first_bsort = first_sort;
        last_bsort = last_sort;
    }
    else {
        first_asort = first_sort;
        last_asort = last_sort;
    }
}

/* Display vnums currently assigned to areas  -Altrag & Thoric
 * Sorted, and flagged if loaded.
 */
void show_vnums( CHAR_DATA *ch, int low, int high, bool proto, bool shownl, const char *loadst,
                 const char *notloadst )
{
    AREA_DATA              *pArea,
                           *first_sort;
    int                     count,
                            loaded;

    count = 0;
    loaded = 0;
    set_pager_color( AT_PLAIN, ch );
    if ( proto )
        first_sort = first_bsort;
    else
        first_sort = first_asort;
    for ( pArea = first_sort; pArea; pArea = pArea->next_sort ) {
        if ( IS_SET( pArea->status, AREA_DELETED ) )
            continue;
        if ( pArea->low_r_vnum < low )
            continue;
        if ( pArea->hi_r_vnum > high )
            break;
        if ( IS_SET( pArea->status, AREA_LOADED ) )
            loaded++;
        else if ( !shownl )
            continue;
        pager_printf( ch, "%-15s| Rooms: %5d - %-5d Objs: %5d - %-5d Mobs: %5d - %-5d%s\r\n",
                      ( pArea->filename ? pArea->filename : "(invalid)" ),
                      pArea->low_r_vnum, pArea->hi_r_vnum, pArea->low_o_vnum, pArea->hi_o_vnum,
                      pArea->low_m_vnum, pArea->hi_m_vnum, IS_SET( pArea->status,
                                                                   AREA_LOADED ) ? loadst :
                      notloadst );
        count++;
    }
    pager_printf( ch, "Areas listed: %d  Loaded: %d\r\n", count, loaded );
}

/* Shows prototype vnums ranges, and if loaded */
void do_vnums( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     low,
                            high;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    low = 0;
    high = MAX_VNUM;
    if ( arg1[0] != '\0' ) {
        low = atoi( arg1 );
        if ( arg2[0] != '\0' )
            high = atoi( arg2 );
    }
    show_vnums( ch, low, high, TRUE, TRUE, " *", "" );
}

/* Shows installed areas, sorted.  Mark unloaded areas with an X */
void do_zones( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     low,
                            high;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    low = 0;
    high = MAX_VNUM;
    if ( arg1[0] != '\0' ) {
        low = atoi( arg1 );
        if ( arg2[0] != '\0' )
            high = atoi( arg2 );
    }
    show_vnums( ch, low, high, FALSE, TRUE, "", " X" );
}

/* Show prototype areas, sorted.  Only show loaded areas */
void do_newzones( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     low,
                            high;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    low = 0;
    high = MAX_VNUM;
    if ( arg1[0] != '\0' ) {
        low = atoi( arg1 );
        if ( arg2[0] != '\0' )
            high = atoi( arg2 );
    }
    show_vnums( ch, low, high, TRUE, FALSE, "", " X" );
}

/* Save system info to data file */
void save_sysdata( SYSTEM_DATA sys )
{
    FILE                   *fp;
    char                    filename[MIL];

    snprintf( filename, MIL, "%ssysdata.dat", SYSTEM_DIR );
    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "save_sysdata: FileOpen" );
        perror( filename );
    }
    else {
        fprintf( fp, "#SYSTEM\n" );
        if ( VLD_STR( sys.mud_name ) )
            fprintf( fp, "MudName        %s~\n", sys.mud_name );
        fprintf( fp, "Highplayers    %d\n", sys.alltimemax );
        if ( VLD_STR( sys.time_of_max ) )
            fprintf( fp, "Highplayertime %s~\n", sys.time_of_max );
        fprintf( fp, "NumberQuotes   %d\n", sys.numquotes );
        fprintf( fp, "CheckImmHost   %d\n", sys.check_imm_host );
        fprintf( fp, "Nameresolving  %d\n", sys.NO_NAME_RESOLVING );
        fprintf( fp, "Waitforauth    %d\n", sys.WAIT_FOR_AUTH );
        fprintf( fp, "Advanced_Player %d\n", sys.Advanced_Player );
        fprintf( fp, "Readallmail    %d\n", sys.read_all_mail );
        fprintf( fp, "Readmailfree   %d\n", sys.read_mail_free );
        fprintf( fp, "Writemailfree  %d\n", sys.write_mail_free );
        fprintf( fp, "Takeothersmail %d\n", sys.take_others_mail );
        fprintf( fp, "Muse           %d\n", sys.muse_level );
        fprintf( fp, "Think          %d\n", sys.think_level );
        fprintf( fp, "Build          %d\n", sys.build_level );
        fprintf( fp, "Log            %d\n", sys.log_level );
        fprintf( fp, "Protoflag      %d\n", sys.level_modify_proto );
        fprintf( fp, "GetObjNotake   %d\n", sys.level_getobjnotake );
        fprintf( fp, "Overridepriv   %d\n", sys.level_override_private );
        fprintf( fp, "Msetplayer     %d\n", sys.level_mset_player );
        fprintf( fp, "Stunplrvsplr   %d\n", sys.stun_plr_vs_plr );
        fprintf( fp, "Stunregular    %d\n", sys.stun_regular );
        fprintf( fp, "Gougepvp       %d\n", sys.gouge_plr_vs_plr );
        fprintf( fp, "Gougenontank   %d\n", sys.gouge_nontank );
        fprintf( fp, "Bashpvp        %d\n", sys.bash_plr_vs_plr );
        fprintf( fp, "Bashnontank    %d\n", sys.bash_nontank );
        fprintf( fp, "Dodgemod       %d\n", sys.dodge_mod );
        fprintf( fp, "Parrymod       %d\n", sys.parry_mod );
        fprintf( fp, "Tumblemod      %d\n", sys.tumble_mod );
        fprintf( fp, "Damplrvsplr    %d\n", sys.dam_plr_vs_plr );
        fprintf( fp, "Damplrvsmob    %d\n", sys.dam_plr_vs_mob );
        fprintf( fp, "Dammobvsplr    %d\n", sys.dam_mob_vs_plr );
        fprintf( fp, "Dammobvsmob    %d\n", sys.dam_mob_vs_mob );
        fprintf( fp, "Doubleexp      %d\n", sys.daydexp );
        fprintf( fp, "Gmb            %d\n", sys.gmb );

        /*
         * Might as well save if double exp is going on or not 
         */
        if ( doubleexp )
            fprintf( fp, "Dexp           %ld\n", starttimedoubleexp );
        if ( happyhouron )
            fprintf( fp, "HappyHour      %ld\n", starthappyhour );

        fprintf( fp, "Forcepc        %d\n", sys.level_forcepc );
        if ( VLD_STR( sys.clan_overseer ) )
            fprintf( fp, "Clanoverseer   %s~\n", sys.clan_overseer );
        if ( VLD_STR( sys.clan_advisor ) )
            fprintf( fp, "Clanadvisor    %s~\n", sys.clan_advisor );
        fprintf( fp, "Clantimer	%d\n", sys.clan_timer );
        fprintf( fp, "Clanlog        %d\n", sys.clanlog );
        fprintf( fp, "Saveflags      %s\n", print_bitvector( &sys.save_flags ) );
        fprintf( fp, "Savefreq       %d\n", sys.save_frequency );
        fprintf( fp, "Bestowdif      %d\n", sys.bestow_dif );
        fprintf( fp, "BanSiteLevel   %d\n", sys.ban_site_level );
        fprintf( fp, "BanRaceLevel   %d\n", sys.ban_race_level );
        fprintf( fp, "BanClassLevel  %d\n", sys.ban_class_level );
        fprintf( fp, "MorphOpt       %d\n", sys.morph_opt );
        fprintf( fp, "PetSave        %d\n", sys.save_pets );
        fprintf( fp, "IdentTries     %d\n", sys.ident_retries );
        fprintf( fp, "Pkloot         %d\n", sys.pk_loot );
        fprintf( fp, "Maxholiday	    %d\n", sys.maxholiday );
        fprintf( fp, "Secpertick	    %d\n", sys.secpertick );
        fprintf( fp, "Pulsepersec	    %d\n", sys.pulsepersec );
        fprintf( fp, "Hoursperday	    %d\n", sys.hoursperday );
        fprintf( fp, "Daysperweek	    %d\n", sys.daysperweek );
        fprintf( fp, "Dayspermonth    %d\n", sys.dayspermonth );
        fprintf( fp, "Monthsperyear   %d\n", sys.monthsperyear );
        // Volk: Added potion lag
        fprintf( fp, "PotionsLag     %d %d %d\n", sys.potionsoob, sys.potionspve, sys.potionspvp );
        fprintf( fp, "ReqWhoArg      %d\n", sys.reqwho_arg );
        fprintf( fp, "WizLock        %d\n", sys.wiz_lock );
        fprintf( fp, "ABootHour      %d\n", sys.autoboot_hour );
        fprintf( fp, "ABootMinute    %d\n", sys.autoboot_minute );
        fprintf( fp, "ABootPeriod    %d\n", sys.autoboot_period );
        fprintf( fp, "Newbie_purge   %d\n", sys.newbie_purge );
        fprintf( fp, "Regular_purge  %d\n", sys.regular_purge );
        fprintf( fp, "Autopurge      %d\n", sys.CLEANPFILES );
        if ( VLD_STR( sys.http ) )
            fprintf( fp, "HTTP           %s~\n", sys.http );
        fprintf( fp, "Class1MaxAdept %d\n", sys.class1maxadept );
        fprintf( fp, "Class2MaxAdept %d\n", sys.class2maxadept );
        fprintf( fp, "Class3MaxAdept %d\n", sys.class3maxadept );
        fprintf( fp, "End\n\n" );
        fprintf( fp, "#END\n" );
    }
    FileClose( fp );
}

void fread_sysdata( SYSTEM_DATA * sys, FILE * fp )
{
    const char             *word;
    bool                    fMatch;

    sys->time_of_max = NULL;
    sys->mud_name = NULL;
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'A':
                KEY( "ABootHour", sys->autoboot_hour, fread_number( fp ) );
                KEY( "ABootMinute", sys->autoboot_minute, fread_number( fp ) );
                KEY( "ABootPeriod", sys->autoboot_period, fread_number( fp ) );
                KEY( "Autopurge", sys->CLEANPFILES, fread_number( fp ) );
                KEY( "Advanced_Player", sys->Advanced_Player, fread_number( fp ) );
                break;
            case 'B':
                KEY( "Bashpvp", sys->bash_plr_vs_plr, fread_number( fp ) );
                KEY( "Bashnontank", sys->bash_nontank, fread_number( fp ) );
                KEY( "Bestowdif", sys->bestow_dif, fread_number( fp ) );
                KEY( "Build", sys->build_level, fread_number( fp ) );
                KEY( "BanSiteLevel", sys->ban_site_level, fread_number( fp ) );
                KEY( "BanClassLevel", sys->ban_class_level, fread_number( fp ) );
                KEY( "BanRaceLevel", sys->ban_race_level, fread_number( fp ) );
                break;
            case 'C':
                KEY( "CheckImmHost", sys->check_imm_host, fread_number( fp ) );
                KEY( "Clanoverseer", sys->clan_overseer, fread_string( fp ) );
                KEY( "Clanadvisor", sys->clan_advisor, fread_string( fp ) );
                KEY( "Clanlog", sys->clanlog, fread_number( fp ) );
                KEY( "Clantimer", sys->clan_timer, fread_number( fp ) );
                KEY( "Class1MaxAdept", sys->class1maxadept, fread_number( fp ) );
                KEY( "Class2MaxAdept", sys->class2maxadept, fread_number( fp ) );
                KEY( "Class3MaxAdept", sys->class3maxadept, fread_number( fp ) );
                break;
            case 'D':
                KEY( "Damplrvsplr", sys->dam_plr_vs_plr, fread_number( fp ) );
                KEY( "Damplrvsmob", sys->dam_plr_vs_mob, fread_number( fp ) );
                KEY( "Dammobvsplr", sys->dam_mob_vs_plr, fread_number( fp ) );
                KEY( "Dammobvsmob", sys->dam_mob_vs_mob, fread_number( fp ) );
                KEY( "Dodgemod", sys->dodge_mod, fread_number( fp ) );
                KEY( "Daysperweek", sys->daysperweek, fread_number( fp ) );
                KEY( "Dayspermonth", sys->dayspermonth, fread_number( fp ) );
                KEY( "Doubleexp", sys->daydexp, fread_number( fp ) );

                if ( !str_cmp( word, "Dexp" ) ) {
                    doubleexp = TRUE;
                    starttimedoubleexp = fread_time( fp );
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !sys->time_of_max )
                        sys->time_of_max = STRALLOC( "(not recorded)" );
                    if ( !sys->mud_name )
                        sys->mud_name = STRALLOC( "(Name Not Set)" );
                    return;
                }
                break;

            case 'F':
                if ( !str_cmp( word, "Fakeplayer" ) ) {
                    fMatch = TRUE;
                    fread_number( fp );
                    break;
                }
                KEY( "Forcepc", sys->level_forcepc, fread_number( fp ) );
                break;
            case 'G':
                KEY( "GetObjNotake", sys->level_getobjnotake, fread_number( fp ) );
                KEY( "Gmb", sys->gmb, fread_number( fp ) );
                KEY( "Gougepvp", sys->gouge_plr_vs_plr, fread_number( fp ) );
                KEY( "Gougenontank", sys->gouge_nontank, fread_number( fp ) );
                break;
            case 'H':
                if ( !str_cmp( word, "HappyHour" ) ) {
                    happyhouron = TRUE;
                    starthappyhour = fread_time( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Highplayers", sys->alltimemax, fread_number( fp ) );
                KEY( "Highplayertime", sys->time_of_max, fread_string( fp ) );
                KEY( "Hoursperday", sys->hoursperday, fread_number( fp ) );
                KEY( "HTTP", sys->http, fread_string( fp ) );
                break;
            case 'I':
                KEY( "IdentTries", sys->ident_retries, fread_number( fp ) );
                break;
            case 'L':
                KEY( "Log", sys->log_level, fread_number( fp ) );
                break;
            case 'M':
                KEY( "MorphOpt", sys->morph_opt, fread_number( fp ) );
                KEY( "Msetplayer", sys->level_mset_player, fread_number( fp ) );
                KEY( "MudName", sys->mud_name, fread_string( fp ) );
                KEY( "Muse", sys->muse_level, fread_number( fp ) );
                KEY( "Maxholiday", sys->maxholiday, fread_number( fp ) );
                KEY( "Monthsperyear", sys->monthsperyear, fread_number( fp ) );
                break;
            case 'N':
                KEY( "NumberQuotes", sys->numquotes, fread_number( fp ) );
                KEY( "Nameresolving", sys->NO_NAME_RESOLVING, fread_number( fp ) );
                KEY( "Newbie_purge", sys->newbie_purge, fread_number( fp ) );
                break;
            case 'O':
                KEY( "Overridepriv", sys->level_override_private, fread_number( fp ) );
                break;
            case 'P':
                KEY( "Parrymod", sys->parry_mod, fread_number( fp ) );
                KEY( "PetSave", sys->save_pets, fread_number( fp ) );
                KEY( "Pkloot", sys->pk_loot, fread_number( fp ) );
                if ( !str_cmp( word, "PotionsLag" ) ) {
                    sys->potionsoob = fread_number( fp );
                    sys->potionspve = fread_number( fp );
                    sys->potionspvp = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                KEY( "Protoflag", sys->level_modify_proto, fread_number( fp ) );
                KEY( "Pulsepersec", sys->pulsepersec, fread_number( fp ) );
                break;
            case 'R':
                KEY( "Readallmail", sys->read_all_mail, fread_number( fp ) );
                KEY( "Readmailfree", sys->read_mail_free, fread_number( fp ) );
                KEY( "Regular_purge", sys->regular_purge, fread_number( fp ) );
                KEY( "ReqWhoArg", sys->reqwho_arg, fread_number( fp ) );
                break;
            case 'S':
                KEY( "Stunplrvsplr", sys->stun_plr_vs_plr, fread_number( fp ) );
                KEY( "Stunregular", sys->stun_regular, fread_number( fp ) );
                KEY( "Saveflags", sys->save_flags, fread_bitvector( fp ) );
                KEY( "Savefreq", sys->save_frequency, fread_number( fp ) );
                KEY( "Secpertick", sys->secpertick, fread_number( fp ) );
                break;
            case 'T':
                KEY( "Takeothersmail", sys->take_others_mail, fread_number( fp ) );
                KEY( "Think", sys->think_level, fread_number( fp ) );
                KEY( "Tumblemod", sys->tumble_mod, fread_number( fp ) );
                break;
            case 'W':
                KEY( "Waitforauth", sys->WAIT_FOR_AUTH, fread_number( fp ) );
                KEY( "WizLock", sys->wiz_lock, fread_number( fp ) );
                KEY( "Writemailfree", sys->write_mail_free, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_sysdata: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

/* Load the sysdata file */
bool load_systemdata( SYSTEM_DATA * sys )
{
    char                    filename[MIL];
    FILE                   *fp;
    bool                    found;

    found = FALSE;
    snprintf( filename, MIL, "%ssysdata.dat", SYSTEM_DIR );
    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {
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
                bug( "Load_sysdata_file: # not found." );
                break;
            }
            word = fread_word( fp );
            if ( !str_cmp( word, "SYSTEM" ) ) {
                fread_sysdata( sys, fp );
                break;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "Load_sysdata_file: bad section." );
                break;
            }
        }
        update_timers(  );
        update_calendar(  );
        FileClose( fp );
    }
    return found;
}

void load_watchlist( void )
{
    WATCH_DATA             *pwatch;
    FILE                   *fp;
    int                     number;
    CMDTYPE                *cmd;

    if ( !( fp = FileOpen( SYSTEM_DIR WATCH_LIST, "r" ) ) )
        return;

    for ( ;; ) {
        if ( feof( fp ) ) {
            bug( "Load_watchlist: no -1 found." );
            FileClose( fp );
            return;
        }
        number = fread_number( fp );
        if ( number == -1 ) {
            FileClose( fp );
            return;
        }

        CREATE( pwatch, WATCH_DATA, 1 );
        pwatch->imm_level = number;
        pwatch->imm_name = fread_string_nohash( fp );
        pwatch->target_name = fread_string_nohash( fp );
        if ( pwatch->target_name && strlen( pwatch->target_name ) < 2 )
            DISPOSE( pwatch->target_name );
        pwatch->player_site = fread_string_nohash( fp );
        if ( pwatch->player_site && strlen( pwatch->player_site ) < 2 )
            DISPOSE( pwatch->player_site );

        /*
         * Check for command watches 
         */
        if ( pwatch->target_name )
            for ( cmd = command_hash[( int ) pwatch->target_name[0]]; cmd; cmd = cmd->next ) {
                if ( !str_cmp( pwatch->target_name, cmd->name ) ) {
                    SET_BIT( cmd->flags, CMD_WATCH );
                    break;
                }
            }

        LINK( pwatch, first_watch, last_watch, next, prev );
    }
}

/* Check to make sure range of vnums is free - Scryn 2/27/96 */
void do_check_vnums( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    buf2[MSL];
    AREA_DATA              *pArea;
    char                    arg1[MSL];
    char                    arg2[MSL];
    bool                    room,
                            mob,
                            obj,
                            all,
                            area_conflict;
    int                     low_range,
                            high_range;

    room = FALSE;
    mob = FALSE;
    obj = FALSE;
    all = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Please specify room, mob, object, or all as your first argument.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "room" ) )
        room = TRUE;

    else if ( !str_cmp( arg1, "mob" ) )
        mob = TRUE;

    else if ( !str_cmp( arg1, "object" ) )
        obj = TRUE;

    else if ( !str_cmp( arg1, "all" ) )
        all = TRUE;
    else {
        send_to_char( "Please specify room, mob, or object as your first argument.\r\n", ch );
        return;
    }

    if ( arg2[0] == '\0' ) {
        send_to_char( "Please specify the low end of the range to be searched.\r\n", ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Please specify the high end of the range to be searched.\r\n", ch );
        return;
    }

    low_range = atoi( arg2 );
    high_range = atoi( argument );

    if ( low_range < 1 || low_range > MAX_VNUM ) {
        send_to_char( "Invalid argument for bottom of range.\r\n", ch );
        return;
    }

    if ( high_range < 1 || high_range > MAX_VNUM ) {
        send_to_char( "Invalid argument for top of range.\r\n", ch );
        return;
    }

    if ( high_range < low_range ) {
        int                     temp = high_range;

        high_range = low_range;
        low_range = temp;
    }

    if ( all ) {
        snprintf( buf, MSL, "room %d %d", low_range, high_range );
        do_check_vnums( ch, buf );
        snprintf( buf, MSL, "mob %d %d", low_range, high_range );
        do_check_vnums( ch, buf );
        snprintf( buf, MSL, "object %d %d", low_range, high_range );
        do_check_vnums( ch, buf );
        return;
    }
    set_char_color( AT_PLAIN, ch );

    for ( pArea = first_asort; pArea; pArea = pArea->next_sort ) {
        area_conflict = FALSE;
        if ( IS_SET( pArea->status, AREA_DELETED ) )
            continue;
        else if ( room ) {
            if ( low_range < pArea->low_r_vnum && pArea->low_r_vnum < high_range )
                area_conflict = TRUE;

            if ( low_range < pArea->hi_r_vnum && pArea->hi_r_vnum < high_range )
                area_conflict = TRUE;

            if ( ( low_range >= pArea->low_r_vnum ) && ( low_range <= pArea->hi_r_vnum ) )
                area_conflict = TRUE;

            if ( ( high_range <= pArea->hi_r_vnum ) && ( high_range >= pArea->low_r_vnum ) )
                area_conflict = TRUE;
        }

        if ( mob ) {
            if ( low_range < pArea->low_m_vnum && pArea->low_m_vnum < high_range )
                area_conflict = TRUE;

            if ( low_range < pArea->hi_m_vnum && pArea->hi_m_vnum < high_range )
                area_conflict = TRUE;
            if ( ( low_range >= pArea->low_m_vnum ) && ( low_range <= pArea->hi_m_vnum ) )
                area_conflict = TRUE;

            if ( ( high_range <= pArea->hi_m_vnum ) && ( high_range >= pArea->low_m_vnum ) )
                area_conflict = TRUE;
        }

        if ( obj ) {
            if ( low_range < pArea->low_o_vnum && pArea->low_o_vnum < high_range )
                area_conflict = TRUE;

            if ( low_range < pArea->hi_o_vnum && pArea->hi_o_vnum < high_range )
                area_conflict = TRUE;

            if ( ( low_range >= pArea->low_o_vnum ) && ( low_range <= pArea->hi_o_vnum ) )
                area_conflict = TRUE;

            if ( ( high_range <= pArea->hi_o_vnum ) && ( high_range >= pArea->low_o_vnum ) )
                area_conflict = TRUE;
        }

        if ( area_conflict ) {
            snprintf( buf, MSL, "Conflict:%-15s| ",
                      ( pArea->filename ? pArea->filename : "(invalid)" ) );
            if ( room )
                snprintf( buf2, MSL, "Rooms: %5d - %-5d\r\n", pArea->low_r_vnum, pArea->hi_r_vnum );
            if ( mob )
                snprintf( buf2, MSL, "Mobs: %5d - %-5d\r\n", pArea->low_m_vnum, pArea->hi_m_vnum );
            if ( obj )
                snprintf( buf2, MSL, "Objects: %5d - %-5d\r\n", pArea->low_o_vnum,
                          pArea->hi_o_vnum );
            mudstrlcat( buf, buf2, MSL );
            send_to_char( buf, ch );
        }
    }
    for ( pArea = first_bsort; pArea; pArea = pArea->next_sort ) {
        area_conflict = FALSE;
        if ( IS_SET( pArea->status, AREA_DELETED ) )
            continue;
        else if ( room ) {
            if ( low_range < pArea->low_r_vnum && pArea->low_r_vnum < high_range )
                area_conflict = TRUE;

            if ( low_range < pArea->hi_r_vnum && pArea->hi_r_vnum < high_range )
                area_conflict = TRUE;

            if ( ( low_range >= pArea->low_r_vnum ) && ( low_range <= pArea->hi_r_vnum ) )
                area_conflict = TRUE;

            if ( ( high_range <= pArea->hi_r_vnum ) && ( high_range >= pArea->low_r_vnum ) )
                area_conflict = TRUE;
        }

        if ( mob ) {
            if ( low_range < pArea->low_m_vnum && pArea->low_m_vnum < high_range )
                area_conflict = TRUE;

            if ( low_range < pArea->hi_m_vnum && pArea->hi_m_vnum < high_range )
                area_conflict = TRUE;
            if ( ( low_range >= pArea->low_m_vnum ) && ( low_range <= pArea->hi_m_vnum ) )
                area_conflict = TRUE;

            if ( ( high_range <= pArea->hi_m_vnum ) && ( high_range >= pArea->low_m_vnum ) )
                area_conflict = TRUE;
        }

        if ( obj ) {
            if ( low_range < pArea->low_o_vnum && pArea->low_o_vnum < high_range )
                area_conflict = TRUE;

            if ( low_range < pArea->hi_o_vnum && pArea->hi_o_vnum < high_range )
                area_conflict = TRUE;

            if ( ( low_range >= pArea->low_o_vnum ) && ( low_range <= pArea->hi_o_vnum ) )
                area_conflict = TRUE;

            if ( ( high_range <= pArea->hi_o_vnum ) && ( high_range >= pArea->low_o_vnum ) )
                area_conflict = TRUE;
        }

        if ( area_conflict ) {
            snprintf( buf, MSL, "Conflict:%-15s| ",
                      ( pArea->filename ? pArea->filename : "(invalid)" ) );
            if ( room )
                snprintf( buf2, MSL, "Rooms: %5d - %-5d\r\n", pArea->low_r_vnum, pArea->hi_r_vnum );
            if ( mob )
                snprintf( buf2, MSL, "Mobs: %5d - %-5d\r\n", pArea->low_m_vnum, pArea->hi_m_vnum );
            if ( obj )
                snprintf( buf2, MSL, "Objects: %5d - %-5d\r\n", pArea->low_o_vnum,
                          pArea->hi_o_vnum );
            mudstrlcat( buf, buf2, MSL );
            send_to_char( buf, ch );
        }
    }

    return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
    return;
}

void load_storages( void )
{
    DIR                    *dp;
    struct dirent          *dentry;

    dp = opendir( STORAGE_DIR );
    dentry = readdir( dp );
    while ( dentry ) {
        if ( dentry->d_name[0] != '.' )
            load_storage( dentry->d_name );

        dentry = readdir( dp );
    }
    closedir( dp );
}

void load_storage( char *filename )
{
    FILE                   *fp;
    OBJ_DATA               *obj;
    char                    buf[MSL];
    bool                    resave = FALSE;

    snprintf( buf, MSL, "%s%s", STORAGE_DIR, filename );
    if ( !( fp = FileOpen( buf, "r" ) ) )
        log_string( "Can't load vault %s", buf );
    else {
        int                     iNest;
        bool                    found;
        OBJ_DATA               *tobj,
                               *tobj_next;
        int                     vnum;
        ROOM_INDEX_DATA        *room = NULL;

        log_printf( "Loading storage room: %s", filename );
        for ( iNest = 0; iNest < MAX_NEST; iNest++ )
            rgObjNest[iNest] = NULL;

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
                bug( "%s", "Load_storeroom: # not found, removing storage file." );
                resave = TRUE;                         /* Save a new one to hopefully fix 
                                                        * this issue */
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "OBJECT" ) ) {
                if ( !( obj = fread_obj( supermob, fp, OS_CARRY ) ) )
                    resave = TRUE;
            }
            else if ( !str_cmp( word, "VNUM" ) ) {
                vnum = fread_number( fp );
                if ( !( room = get_room_index( vnum ) ) ) {
                    FileClose( fp );
                    unlink( buf );
                    bug( "%s", "load_storeroom: NULL room vnum, removing storage file." );
                    return;
                }
                else
                    rset_supermob( room );
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_storeroom: bad section." );
                break;
            }
        }
        FileClose( fp );
        for ( tobj = supermob->first_carrying; tobj; tobj = tobj_next ) {
            tobj_next = tobj->next_content;
            obj_from_char( tobj );
            obj_to_room( tobj, room );
        }
        if ( resave )                                  /* If there was a problem with an
                                                        * object lets resave after all is 
                                                        * said * and done to stop
                                                        * repeating bugs */
            save_clan_storeroom( supermob );
        release_supermob(  );
    }
}

char                   *case_argument( char *argument, char *arg_first )
{
    char                    cEnd;
    int                     count;

    count = 0;

    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' || ++count >= 255 ) {
        if ( *argument == cEnd ) {
            argument++;
            break;
        }
        *arg_first = ( *argument );
        arg_first++;
        argument++;
    }
    *arg_first = '\0';
    while ( isspace( *argument ) )
        argument++;

    return argument;
}

void log_printf( const char *fmt, ... )
{
    char                    buf[2 * MSL];

    va_list                 args;

    va_start( args, fmt );
    vsnprintf( buf, 2 * MSL, fmt, args );
    va_end( args );

    log_string( buf );
}

size_t dbmudstrlcpy( char *dst, const char *src, size_t siz, const char *file, int line )
// size_t mudstrlcpy( char *dst, const char *src, size_t siz )
{
    register char          *d = dst;
    register const char    *s = src;
    register size_t         n = siz;

    if ( !src || !dst ) {
        bug( "mudstrlcpy: dst = [%s] src = [%s] from %s:%d", !dst ? "NULL" : "", !src ? "NULL" : "",
             file, line );
//      bug("mudstrlcpy: dst = [%s] src = [%s]", !dst? "NULL": "", !src? "NULL": "");
        return 0;
    }
    /*
     * Copy as many bytes as will fit
     */
    if ( n != 0 && --n != 0 ) {
        do {
            if ( ( *d++ = *s++ ) == 0 )
                break;
        }
        while ( --n != 0 );
    }

    /*
     * Not enough room in dst, add NUL and traverse rest of src
     */
    if ( n == 0 ) {
        if ( siz != 0 )
            *d = '\0';                                 /* NUL-terminate dst */
        while ( *s++ );
    }
    return ( s - src - 1 );                            /* count does not include NUL */
}

size_t mudstrlcat( char *dst, const char *src, size_t siz )
{
    register char          *d = dst;
    register const char    *s = src;
    register size_t         n = siz;
    size_t                  dlen;

    if ( !src || !dst ) {
        bug( "mudstrlcat: dst = [%s] src = [%s]", !dst ? "NULL" : "", !src ? "NULL" : "" );
        return 0;
    }

    /*
     * Find the end of dst and adjust bytes left but don't go past end
     */
    while ( n-- != 0 && *d != '\0' )
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if ( n == 0 )
        return ( dlen + strlen( s ) );
    while ( *s && *s != '\0' ) {
        if ( n != 1 ) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';
    return ( dlen + ( s - src ) );                     /* count does not include NUL */
}

bool is_valid_filename( CHAR_DATA *ch, const char *direct, const char *filename )
{
    char                    newfilename[256];
    struct stat             fst;

    /*
     * Length restrictions
     */
    if ( !filename || filename[0] == '\0' || strlen( filename ) < 3 ) {
        if ( !filename || !str_cmp( filename, "" ) )
            send_to_char( "Empty filename is not valid.\r\n", ch );
        else
            ch_printf( ch, "%s: Filename is too short.\r\n", filename );
        return FALSE;
    }

    /*
     * Illegal characters
     */
    if ( strstr( filename, ".." ) || strstr( filename, "/" ) || strstr( filename, "\\" ) ) {
        send_to_char( "A filename may not contain a '..', '/', or '\\' in it.\r\n", ch );
        return FALSE;
    }

    /*
     * If that filename is already being used lets not allow it now to be on the safe side
     */
    snprintf( newfilename, sizeof( newfilename ), "%s%s", direct, filename );
    if ( stat( newfilename, &fst ) != -1 ) {
        ch_printf( ch, "%s is already an existing filename.\r\n", newfilename );
        return FALSE;
    }
    /*
     * If we got here assume its valid
     */
    return TRUE;
}
